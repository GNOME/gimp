/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * file-jpegxl - JPEG XL file format plug-in for the GIMP
 * Copyright (C) 2022  Daniel Novomesky
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <gexiv2/gexiv2.h>
#include <glib/gstdio.h>

#include <jxl/decode.h>
#include <jxl/encode.h>
#include <jxl/thread_parallel_runner.h>

#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

#include "libgimp/stdplugins-intl.h"

#define LOAD_PROC      "file-jpegxl-load"
#define SAVE_PROC      "file-jpegxl-save"
#define PLUG_IN_BINARY "file-jpegxl"

static void query (void);
static void run (const gchar     *name,
                 gint             nparams,
                 const GimpParam *param,
                 gint            *nreturn_vals,
                 GimpParam      **return_vals);

GimpPlugInInfo PLUG_IN_INFO = {
  NULL,  /* init_proc  */
  NULL,  /* quit_proc  */
  query, /* query_proc */
  run,   /* run_proc   */
};

MAIN ()

static void
query (void)
{
  static const GimpParamDef load_args[] = {
    { GIMP_PDB_INT32,  "run-mode",     "The run mode { RUN-INTERACTIVE (0), RUN-NONINTERACTIVE (1) }"},
    { GIMP_PDB_STRING, "filename",     "The name of the file to load"                                },
    { GIMP_PDB_STRING, "raw-filename", "The name of the file to load"                                }
  };
  static const GimpParamDef load_return_vals[] = {
    {GIMP_PDB_IMAGE, "image", "Output image"}
  };

  static const GimpParamDef save_args[] =
  {
    { GIMP_PDB_INT32,    "run-mode",     "The run mode { RUN-INTERACTIVE (0), RUN-NONINTERACTIVE (1) }" },
    { GIMP_PDB_IMAGE,    "image",        "Input image" },
    { GIMP_PDB_DRAWABLE, "drawable",     "Drawable to save" },
    { GIMP_PDB_STRING,   "filename",     "The name of the file to save the image in" },
    { GIMP_PDB_STRING,   "raw-filename", "The name of the file to save the image in" }
  };

  gimp_install_procedure (LOAD_PROC,
                          "Loads files in the JPEG XL file format",
                          "Loads files in the JPEG XL file format",
                          "Daniel Novomesky",
                          "(C) 2022 Daniel Novomesky",
                          "2022",
                          N_("JPEG XL image"),
                          NULL,
                          GIMP_PLUGIN,
                          G_N_ELEMENTS (load_args),
                          G_N_ELEMENTS (load_return_vals),
                          load_args, load_return_vals);

  gimp_register_file_handler_mime (LOAD_PROC, "image/jxl");
  gimp_register_magic_load_handler (LOAD_PROC, "jxl", "", "0,string,\xFF\x0A,0,string,\\000\\000\\000\x0CJXL\\040\\015\\012\x87\\012");
  gimp_register_file_handler_priority (LOAD_PROC, 100);

  gimp_install_procedure (SAVE_PROC,
                          "Saves files in the JPEG XL file format",
                          "Saves files in the JPEG XL file format",
                          "Daniel Novomesky",
                          "(C) 2022 Daniel Novomesky",
                          "2022",
                          N_("JPEG XL image"),
                          "RGB*, GRAY*",
                          GIMP_PLUGIN,
                          G_N_ELEMENTS (save_args), 0,
                          save_args, NULL);

  gimp_register_file_handler_mime (SAVE_PROC, "image/jxl");
  gimp_register_save_handler (SAVE_PROC, "jxl", "");
  gimp_register_file_handler_uri (SAVE_PROC);
  gimp_register_file_handler_priority (SAVE_PROC, 100);
}

static gint32
load_image (const gchar *filename,
            GimpRunMode  run_mode,
            GError     **error)
{
  FILE *inputFile = g_fopen (filename, "rb");

  gsize    inputFileSize;
  gpointer memory;

  JxlSignature      signature;
  JxlDecoder       *decoder;
  void             *runner;
  JxlBasicInfo      basicinfo;
  JxlDecoderStatus  status;
  JxlPixelFormat    pixel_format;
  JxlColorEncoding  color_encoding;
  size_t            icc_size   = 0;
  GimpColorProfile *profile    = NULL;
  gboolean          loadlinear = FALSE;
  size_t            channel_depth;
  size_t            result_size;
  gpointer          picture_buffer;
  gint32            image = -1;
  gint32            layer;
  GeglBuffer       *buffer;
  GimpPrecision     precision_linear;
  GimpPrecision     precision_non_linear;
  size_t            num_worker_threads = 1;

  if (! inputFile)
    {
      g_set_error (error, G_FILE_ERROR, 0, "Cannot open file for read: %s\n", filename);
      return -1;
    }

  fseek (inputFile, 0, SEEK_END);
  inputFileSize = ftell (inputFile);
  fseek (inputFile, 0, SEEK_SET);

  if (inputFileSize < 1)
    {
      g_set_error (error, G_FILE_ERROR, 0, "File too small: %s\n", filename);
      fclose (inputFile);
      return -1;
    }

  memory = g_malloc (inputFileSize);
  if (fread (memory, 1, inputFileSize, inputFile) != inputFileSize)
    {
      g_set_error (error, G_FILE_ERROR, 0, "Failed to read %zu bytes: %s\n",
                   inputFileSize, filename);
      fclose (inputFile);
      g_free (memory);
      return -1;
    }

  fclose (inputFile);

  signature = JxlSignatureCheck (memory, inputFileSize);
  if (signature != JXL_SIG_CODESTREAM && signature != JXL_SIG_CONTAINER)
    {
      g_set_error (error, G_FILE_ERROR, 0,
                   "File %s is probably not in JXL format!\n", filename);
      g_free (memory);
      return -1;
    }

  decoder = JxlDecoderCreate (NULL);
  if (! decoder)
    {
      g_set_error (error, G_FILE_ERROR, 0, "ERROR: JxlDecoderCreate failed");
      g_free (memory);
      return -1;
    }

  num_worker_threads = g_get_num_processors ();
  if (num_worker_threads > 16)
    {
      num_worker_threads = 16;
    }
  runner = JxlThreadParallelRunnerCreate (NULL, num_worker_threads);
  if (JxlDecoderSetParallelRunner (decoder, JxlThreadParallelRunner, runner) != JXL_DEC_SUCCESS)
    {
      g_set_error (error, G_FILE_ERROR, 0, "ERROR: JxlDecoderSetParallelRunner failed");
      JxlThreadParallelRunnerDestroy (runner);
      JxlDecoderDestroy (decoder);
      g_free (memory);
      return -1;
    }

  if (JxlDecoderSetInput (decoder, memory, inputFileSize) != JXL_DEC_SUCCESS)
    {
      g_set_error (error, G_FILE_ERROR, 0, "ERROR: JxlDecoderSetInput failed");
      JxlThreadParallelRunnerDestroy (runner);
      JxlDecoderDestroy (decoder);
      g_free (memory);
      return -1;
    }

  JxlDecoderCloseInput (decoder);

  if (JxlDecoderSubscribeEvents (decoder, JXL_DEC_BASIC_INFO | JXL_DEC_COLOR_ENCODING | JXL_DEC_FULL_IMAGE)
      != JXL_DEC_SUCCESS)
    {
      g_set_error (error, G_FILE_ERROR, 0, "ERROR: JxlDecoderSubscribeEvents failed");
      JxlThreadParallelRunnerDestroy (runner);
      JxlDecoderDestroy (decoder);
      g_free (memory);
      return -1;
    }

  status = JxlDecoderProcessInput (decoder);
  if (status == JXL_DEC_ERROR)
    {
      g_set_error (error, G_FILE_ERROR, 0, "ERROR: JXL decoding failed");
      JxlThreadParallelRunnerDestroy (runner);
      JxlDecoderDestroy (decoder);
      g_free (memory);
      return -1;
    }

  if (status == JXL_DEC_NEED_MORE_INPUT)
    {
      g_set_error (error, G_FILE_ERROR, 0, "ERROR: JXL data incomplete");
      JxlThreadParallelRunnerDestroy (runner);
      JxlDecoderDestroy (decoder);
      g_free (memory);
      return -1;
    }

  status = JxlDecoderGetBasicInfo (decoder, &basicinfo);
  if (status != JXL_DEC_SUCCESS)
    {
      g_set_error (error, G_FILE_ERROR, 0, "ERROR: JXL basic info not available");
      JxlThreadParallelRunnerDestroy (runner);
      JxlDecoderDestroy (decoder);
      g_free (memory);
      return -1;
    }

  if (basicinfo.xsize == 0 || basicinfo.ysize == 0)
    {
      g_set_error (error, G_FILE_ERROR, 0, "ERROR: JXL image has zero dimensions");
      JxlThreadParallelRunnerDestroy (runner);
      JxlDecoderDestroy (decoder);
      g_free (memory);
      return -1;
    }

  status = JxlDecoderProcessInput (decoder);
  if (status != JXL_DEC_COLOR_ENCODING)
    {
      g_set_error (error, G_FILE_ERROR, 0,
                   "Unexpected event %d instead of JXL_DEC_COLOR_ENCODING", status);
      JxlThreadParallelRunnerDestroy (runner);
      JxlDecoderDestroy (decoder);
      g_free (memory);
      return -1;
    }

  if (basicinfo.uses_original_profile == JXL_FALSE)
    {
      if (basicinfo.num_color_channels == 3)
        {
          JxlColorEncodingSetToSRGB (&color_encoding, JXL_FALSE);
          JxlDecoderSetPreferredColorProfile (decoder, &color_encoding);
        }
      else if (basicinfo.num_color_channels == 1)
        {
          JxlColorEncodingSetToSRGB (&color_encoding, JXL_TRUE);
          JxlDecoderSetPreferredColorProfile (decoder, &color_encoding);
        }
    }

  pixel_format.endianness = JXL_NATIVE_ENDIAN;
  pixel_format.align      = 0;

  if (basicinfo.uses_original_profile == JXL_FALSE || basicinfo.bits_per_sample > 16)
    {
      pixel_format.data_type = JXL_TYPE_FLOAT;
      channel_depth          = 4;
      precision_linear       = GIMP_PRECISION_FLOAT_LINEAR;
      precision_non_linear   = GIMP_PRECISION_FLOAT_GAMMA;
    }
  else if (basicinfo.bits_per_sample <= 8)
    {
      pixel_format.data_type = JXL_TYPE_UINT8;
      channel_depth          = 1;
      precision_linear       = GIMP_PRECISION_U8_LINEAR;
      precision_non_linear   = GIMP_PRECISION_U8_GAMMA;
    }
  else
    {
      pixel_format.data_type = JXL_TYPE_UINT16;
      channel_depth          = 2;
      precision_linear       = GIMP_PRECISION_U16_LINEAR;
      precision_non_linear   = GIMP_PRECISION_U16_GAMMA;
    }

  if (basicinfo.num_color_channels == 1) /* grayscale */
    {
      if (basicinfo.alpha_bits > 0)
        {
          pixel_format.num_channels = 2;
        }
      else
        {
          pixel_format.num_channels = 1;
        }
    }
  else /* RGB */
    {

      if (basicinfo.alpha_bits > 0) /* RGB with alpha */
        {
          pixel_format.num_channels = 4;
        }
      else /* RGB no alpha */
        {
          pixel_format.num_channels = 3;
        }
    }

  result_size = channel_depth * pixel_format.num_channels
                * (size_t) basicinfo.xsize * (size_t) basicinfo.ysize;

  if (JxlDecoderGetColorAsEncodedProfile (decoder,
#if JPEGXL_NUMERIC_VERSION < JPEGXL_COMPUTE_NUMERIC_VERSION(0,9,0)
                                          &pixel_format,
#endif
                                          JXL_COLOR_PROFILE_TARGET_DATA,
                                          &color_encoding) == JXL_DEC_SUCCESS)
    {
      if (color_encoding.white_point == JXL_WHITE_POINT_D65)
        {
          switch (color_encoding.transfer_function)
            {
            case JXL_TRANSFER_FUNCTION_LINEAR:
              loadlinear = TRUE;

              switch (color_encoding.color_space)
                {
                case JXL_COLOR_SPACE_RGB:
                  profile = gimp_color_profile_new_rgb_srgb_linear ();
                  break;
                case JXL_COLOR_SPACE_GRAY:
                  profile = gimp_color_profile_new_d65_gray_linear ();
                  break;
                default:
                  break;
                }
              break;
            case JXL_TRANSFER_FUNCTION_SRGB:
              switch (color_encoding.color_space)
                {
                case JXL_COLOR_SPACE_RGB:
                  profile = gimp_color_profile_new_rgb_srgb ();
                  break;
                case JXL_COLOR_SPACE_GRAY:
                  profile = gimp_color_profile_new_d65_gray_srgb_trc ();
                  break;
                default:
                  break;
                }
              break;
            default:
              break;
            }
        }
    }

  if (! profile)
    {
      if (JxlDecoderGetICCProfileSize (decoder,
#if JPEGXL_NUMERIC_VERSION < JPEGXL_COMPUTE_NUMERIC_VERSION(0,9,0)
                                       &pixel_format,
#endif
                                       JXL_COLOR_PROFILE_TARGET_DATA,
                                       &icc_size) == JXL_DEC_SUCCESS)
        {
          if (icc_size > 0)
            {
              gpointer raw_icc_profile = g_malloc (icc_size);

              if (JxlDecoderGetColorAsICCProfile (decoder,
#if JPEGXL_NUMERIC_VERSION < JPEGXL_COMPUTE_NUMERIC_VERSION(0,9,0)
                                                  &pixel_format,
#endif
                                                  JXL_COLOR_PROFILE_TARGET_DATA,
                                                  raw_icc_profile, icc_size)
                  == JXL_DEC_SUCCESS)
                {
                  profile = gimp_color_profile_new_from_icc_profile (raw_icc_profile,
                                                                     icc_size, error);
                  if (profile)
                    {
                      loadlinear = gimp_color_profile_is_linear (profile);
                    }
                  else
                    {
                      g_printerr ("%s: Failed to read ICC profile: %s\n",
                                  G_STRFUNC, (*error)->message);
                      g_clear_error (error);
                    }
                }
              else
                {
                  g_printerr ("Failed to obtain data from JPEG XL decoder");
                }

              g_free (raw_icc_profile);
            }
          else
            {
              g_printerr ("Empty ICC data");
            }
        }
      else
        {
          g_message ("no ICC, other color profile");
        }
    }

  status = JxlDecoderProcessInput (decoder);
  if (status != JXL_DEC_NEED_IMAGE_OUT_BUFFER)
    {
      g_set_error (error, G_FILE_ERROR,
                   0, "Unexpected event %d instead of JXL_DEC_NEED_IMAGE_OUT_BUFFER", status);
      if (profile)
        {
          g_object_unref (profile);
        }
      JxlThreadParallelRunnerDestroy (runner);
      JxlDecoderDestroy (decoder);
      g_free (memory);
      return -1;
    }

  picture_buffer = g_try_malloc (result_size);
  if (! picture_buffer)
    {
      g_set_error (error, G_FILE_ERROR, 0, "Memory could not be allocated.");
      if (profile)
        {
          g_object_unref (profile);
        }
      JxlThreadParallelRunnerDestroy (runner);
      JxlDecoderDestroy (decoder);
      g_free (memory);
      return -1;
    }

  if (JxlDecoderSetImageOutBuffer (decoder, &pixel_format, picture_buffer, result_size) != JXL_DEC_SUCCESS)
    {
      g_set_error (error, G_FILE_ERROR, 0, "ERROR: JxlDecoderSetImageOutBuffer failed");
      if (profile)
        {
          g_object_unref (profile);
        }
      JxlThreadParallelRunnerDestroy (runner);
      JxlDecoderDestroy (decoder);
      g_free (memory);
      return -1;
    }

  status = JxlDecoderProcessInput (decoder);
  if (status != JXL_DEC_FULL_IMAGE)
    {
      g_set_error (error, G_FILE_ERROR, 0,
                   "Unexpected event %d instead of JXL_DEC_FULL_IMAGE", status);
      g_free (picture_buffer);
      if (profile)
        {
          g_object_unref (profile);
        }
      JxlThreadParallelRunnerDestroy (runner);
      JxlDecoderDestroy (decoder);
      g_free (memory);
      return -1;
    }

  if (basicinfo.num_color_channels == 1) /* grayscale */
    {
      image = gimp_image_new_with_precision (basicinfo.xsize, basicinfo.ysize, GIMP_GRAY,
                                             loadlinear ? precision_linear : precision_non_linear);

      if (profile)
        {
          if (gimp_color_profile_is_gray (profile))
            {
              gimp_image_set_color_profile (image, profile);
            }
        }

      layer = gimp_layer_new (image, "Background", basicinfo.xsize, basicinfo.ysize,
                              (basicinfo.alpha_bits > 0) ? GIMP_GRAYA_IMAGE : GIMP_GRAY_IMAGE,
                              100, gimp_image_get_default_new_layer_mode (image));
    }
  else /* RGB */
    {
      image = gimp_image_new_with_precision (basicinfo.xsize, basicinfo.ysize, GIMP_RGB,
                                             loadlinear ? precision_linear : precision_non_linear);

      if (profile)
        {
          if (gimp_color_profile_is_rgb (profile))
            {
              gimp_image_set_color_profile (image, profile);
            }
        }

      layer = gimp_layer_new (image, "Background", basicinfo.xsize, basicinfo.ysize,
                              (basicinfo.alpha_bits > 0) ? GIMP_RGBA_IMAGE : GIMP_RGB_IMAGE,
                              100, gimp_image_get_default_new_layer_mode (image));
    }

  gimp_image_insert_layer (image, layer, -1, 0);

  buffer = gimp_drawable_get_buffer (layer);

  gegl_buffer_set (buffer, GEGL_RECTANGLE (0, 0, basicinfo.xsize, basicinfo.ysize),
                   0, NULL, picture_buffer, GEGL_AUTO_ROWSTRIDE);

  g_object_unref (buffer);

  g_free (picture_buffer);
  if (profile)
    {
      g_object_unref (profile);
    }

  if (basicinfo.have_container)
    {
      JxlDecoderReleaseInput (decoder);
      JxlDecoderRewind (decoder);

      if (JxlDecoderSetInput (decoder, memory, inputFileSize) != JXL_DEC_SUCCESS)
        {
          g_printerr ("%s: JxlDecoderSetInput failed after JxlDecoderRewind\n", G_STRFUNC);
        }
      else
        {
          JxlDecoderCloseInput (decoder);
          if (JxlDecoderSubscribeEvents (decoder, JXL_DEC_BOX) != JXL_DEC_SUCCESS)
            {
              g_printerr ("%s: JxlDecoderSubscribeEvents for JXL_DEC_BOX failed\n", G_STRFUNC);
            }
          else
            {
              gboolean    search_exif  = TRUE;
              gboolean    search_xmp   = TRUE;
              gboolean    success_exif = FALSE;
              gboolean    success_xmp  = FALSE;
              JxlBoxType  box_type     = { 0, 0, 0, 0 };
              GByteArray *exif_box     = NULL;
              GByteArray *xml_box      = NULL;
              size_t      exif_remains = 0;
              size_t      xml_remains  = 0;

              while (search_exif || search_xmp)
                {
                  status = JxlDecoderProcessInput (decoder);
                  switch (status)
                    {
                    case JXL_DEC_SUCCESS:
                      if (box_type[0] == 'E' && box_type[1] == 'x' && box_type[2] == 'i' && box_type[3] == 'f' && search_exif)
                        {
                          exif_remains = JxlDecoderReleaseBoxBuffer (decoder);
                          g_byte_array_set_size (exif_box, exif_box->len - exif_remains);
                          success_exif = TRUE;
                        }
                      else if (box_type[0] == 'x' && box_type[1] == 'm' && box_type[2] == 'l' && box_type[3] == ' ' && search_xmp)
                        {
                          xml_remains = JxlDecoderReleaseBoxBuffer (decoder);
                          g_byte_array_set_size (xml_box, xml_box->len - xml_remains);
                          success_xmp = TRUE;
                        }

                      search_exif = FALSE;
                      search_xmp  = FALSE;
                      break;
                    case JXL_DEC_ERROR:
                      search_exif = FALSE;
                      search_xmp  = FALSE;
                      g_printerr ("%s: Metadata decoding error\n", G_STRFUNC);
                      break;
                    case JXL_DEC_NEED_MORE_INPUT:
                      search_exif = FALSE;
                      search_xmp  = FALSE;
                      g_printerr ("%s: JXL metadata are probably incomplete\n", G_STRFUNC);
                      break;
                    case JXL_DEC_BOX:
                      JxlDecoderSetDecompressBoxes (decoder, JXL_TRUE);

                      if (box_type[0] == 'E' && box_type[1] == 'x' && box_type[2] == 'i' && box_type[3] == 'f' && search_exif && exif_box)
                        {
                          exif_remains = JxlDecoderReleaseBoxBuffer (decoder);
                          g_byte_array_set_size (exif_box, exif_box->len - exif_remains);

                          search_exif  = FALSE;
                          success_exif = TRUE;
                        }
                      else if (box_type[0] == 'x' && box_type[1] == 'm' && box_type[2] == 'l' && box_type[3] == ' ' && search_xmp && xml_box)
                        {
                          xml_remains = JxlDecoderReleaseBoxBuffer (decoder);
                          g_byte_array_set_size (xml_box, xml_box->len - xml_remains);

                          search_xmp  = FALSE;
                          success_xmp = TRUE;
                        }

                      if (JxlDecoderGetBoxType (decoder, box_type, JXL_TRUE) == JXL_DEC_SUCCESS)
                        {
                          if (box_type[0] == 'E' && box_type[1] == 'x' && box_type[2] == 'i' && box_type[3] == 'f' && search_exif)
                            {
                              exif_box = g_byte_array_sized_new (4096);
                              g_byte_array_set_size (exif_box, 4096);

                              JxlDecoderSetBoxBuffer (decoder, exif_box->data, exif_box->len);
                            }
                          else if (box_type[0] == 'x' && box_type[1] == 'm' && box_type[2] == 'l' && box_type[3] == ' ' && search_xmp)
                            {
                              xml_box = g_byte_array_sized_new (4096);
                              g_byte_array_set_size (xml_box, 4096);

                              JxlDecoderSetBoxBuffer (decoder, xml_box->data, xml_box->len);
                            }
                        }
                      else
                        {
                          search_exif = FALSE;
                          search_xmp  = FALSE;
                          g_printerr ("%s: Error in JxlDecoderGetBoxType\n", G_STRFUNC);
                        }
                      break;
                    case JXL_DEC_BOX_NEED_MORE_OUTPUT:
                      if (box_type[0] == 'E' && box_type[1] == 'x' && box_type[2] == 'i' && box_type[3] == 'f' && search_exif)
                        {
                          exif_remains = JxlDecoderReleaseBoxBuffer (decoder);
                          g_byte_array_set_size (exif_box, exif_box->len + 4096);
                          JxlDecoderSetBoxBuffer (decoder, exif_box->data + exif_box->len - (4096 + exif_remains), 4096 + exif_remains);
                        }
                      else if (box_type[0] == 'x' && box_type[1] == 'm' && box_type[2] == 'l' && box_type[3] == ' ' && search_xmp)
                        {
                          xml_remains = JxlDecoderReleaseBoxBuffer (decoder);
                          g_byte_array_set_size (xml_box, xml_box->len + 4096);
                          JxlDecoderSetBoxBuffer (decoder, xml_box->data + xml_box->len - (4096 + xml_remains), 4096 + xml_remains);
                        }
                      else
                        {
                          search_exif = FALSE;
                          search_xmp  = FALSE;
                        }
                      break;
                    default:
                      break;
                    }
                }

              if (success_exif || success_xmp)
                {
                  GimpMetadata *metadata = gimp_metadata_new ();

                  if (success_exif && exif_box)
                    {
                      const guint8  tiffHeaderBE[4] = { 'M', 'M', 0, 42 };
                      const guint8  tiffHeaderLE[4] = { 'I', 'I', 42, 0 };
                      const guint8 *tiffheader      = exif_box->data;
                      glong         new_exif_size   = exif_box->len;

                      while (new_exif_size >= 4) /*Searching for TIFF Header*/
                        {
                          if (tiffheader[0] == tiffHeaderBE[0] && tiffheader[1] == tiffHeaderBE[1] &&
                              tiffheader[2] == tiffHeaderBE[2] && tiffheader[3] == tiffHeaderBE[3])
                            {
                              break;
                            }
                          if (tiffheader[0] == tiffHeaderLE[0] && tiffheader[1] == tiffHeaderLE[1] &&
                              tiffheader[2] == tiffHeaderLE[2] && tiffheader[3] == tiffHeaderLE[3])
                            {
                              break;
                            }
                          new_exif_size--;
                          tiffheader++;
                        }

                      if (new_exif_size > 4) /* TIFF header + some data found*/
                        {
                          if (! gexiv2_metadata_open_buf (GEXIV2_METADATA (metadata), tiffheader, new_exif_size, error))
                            {
                              g_printerr ("%s: Failed to set EXIF metadata: %s\n", G_STRFUNC, (*error)->message);
                              g_clear_error (error);
                            }
                        }
                      else
                        {
                          g_printerr ("%s: EXIF metadata not set\n", G_STRFUNC);
                        }
                    }

                  if (success_xmp && xml_box)
                    {
                      if (! gimp_metadata_set_from_xmp (metadata, xml_box->data, xml_box->len, error))
                        {
                          g_printerr ("%s: Failed to set XMP metadata: %s\n", G_STRFUNC, (*error)->message);
                          g_clear_error (error);
                        }
                    }

                  gexiv2_metadata_set_orientation (GEXIV2_METADATA (metadata),
                                                   GEXIV2_ORIENTATION_NORMAL);
                  gexiv2_metadata_set_metadata_pixel_width (GEXIV2_METADATA (metadata),
                                                            basicinfo.xsize);
                  gexiv2_metadata_set_metadata_pixel_height (GEXIV2_METADATA (metadata),
                                                             basicinfo.ysize);
                  gimp_image_metadata_load_finish (image, "image/jxl", metadata,
                                                   GIMP_METADATA_LOAD_COMMENT | GIMP_METADATA_LOAD_RESOLUTION,
                                                   (run_mode == GIMP_RUN_INTERACTIVE));
                }

              if (exif_box)
                {
                  g_byte_array_free (exif_box, TRUE);
                }

              if (xml_box)
                {
                  g_byte_array_free (xml_box, TRUE);
                }
            }
        }
    }

  JxlThreadParallelRunnerDestroy (runner);
  JxlDecoderDestroy (decoder);
  g_free (memory);
  return image;
}

static gboolean
save_image (GFile   *file,
            gint32   image,
            gint32   drawable,
            GError **error)
{
  JxlEncoder              *encoder;
  void                    *runner;
  JxlEncoderFrameSettings *encoder_options;
  JxlPixelFormat           pixel_format;
  JxlBasicInfo             output_info;
  JxlColorEncoding         color_profile;
  JxlEncoderStatus         status;
  size_t                   buffer_size;

  GByteArray              *compressed;

  FILE                    *outfile;
  GeglBuffer              *buffer;
  GimpImageType            drawable_type;

  gint                     drawable_width;
  gint                     drawable_height;
  gpointer                 picture_buffer;

  GimpColorProfile        *profile = NULL;
  const Babl              *file_format = NULL;
  gboolean                 out_linear = FALSE;

  size_t                   offset = 0;
  uint8_t                 *next_out;
  size_t                   avail_out;

  gboolean                 save_icc;
  size_t                   num_worker_threads = 1;

  gimp_progress_init_printf (_("Exporting '%s'"),
                             gimp_file_get_utf8_name (file));

  drawable_type   = gimp_drawable_type (drawable);
  buffer = gimp_drawable_get_buffer (drawable);
  drawable_width  = gegl_buffer_get_width (buffer);
  drawable_height = gegl_buffer_get_height (buffer);

  JxlEncoderInitBasicInfo(&output_info);

  output_info.uses_original_profile = JXL_TRUE;

  profile = gimp_image_get_color_profile (image);
  if (! profile)
    {
      profile = gimp_image_get_effective_color_profile (image);
    }

  out_linear = gimp_color_profile_is_linear (profile);

  pixel_format.data_type = JXL_TYPE_UINT8;
  output_info.bits_per_sample = 8;

  pixel_format.endianness = JXL_NATIVE_ENDIAN;
  pixel_format.align = 0;

  output_info.xsize = drawable_width;
  output_info.ysize = drawable_height;
  output_info.exponent_bits_per_sample = 0;
  output_info.orientation = JXL_ORIENT_IDENTITY;
  output_info.animation.tps_numerator = 10;
  output_info.animation.tps_denominator = 1;

  switch (drawable_type)
    {
    case GIMP_GRAYA_IMAGE:
      if (out_linear)
        {
          file_format = babl_format ( "YA u8");
          JxlColorEncodingSetToLinearSRGB (&color_profile, JXL_TRUE);
        }
      else
        {
          file_format = babl_format ("Y'A u8");
          JxlColorEncodingSetToSRGB (&color_profile, JXL_TRUE);
        }
      pixel_format.num_channels = 2;
      output_info.num_color_channels = 1;
      output_info.alpha_bits = 8;
      output_info.alpha_exponent_bits = 0;
      output_info.num_extra_channels = 1;

      save_icc = FALSE;
      break;
    case GIMP_GRAY_IMAGE:
      if (out_linear)
        {
          file_format = babl_format ("Y u8");
          JxlColorEncodingSetToLinearSRGB (&color_profile, JXL_TRUE);
        }
      else
        {
          file_format = babl_format ("Y' u8");
          JxlColorEncodingSetToSRGB (&color_profile, JXL_TRUE);
        }
      pixel_format.num_channels = 1;
      output_info.num_color_channels = 1;
      output_info.alpha_bits = 0;

      save_icc = FALSE;
      break;
    case GIMP_RGBA_IMAGE:
      file_format = babl_format (out_linear ? "RGBA u8" : "R'G'B'A u8");
      output_info.alpha_bits = 8;
      pixel_format.num_channels = 4;
      output_info.num_color_channels = 3;
      output_info.alpha_exponent_bits = 0;
      output_info.num_extra_channels = 1;

      save_icc = TRUE;
      break;
    case GIMP_RGB_IMAGE:
      file_format = babl_format (out_linear ? "RGB u8" : "R'G'B' u8");
      pixel_format.num_channels = 3;
      output_info.num_color_channels = 3;
      output_info.alpha_bits = 0;

      save_icc = TRUE;
      break;
    default:
      if (profile)
        {
          g_object_unref (profile);
        }
      g_object_unref (buffer);
      return FALSE;
      break;
    }

  buffer_size = pixel_format.num_channels * (size_t) output_info.xsize * (size_t) output_info.ysize;
  picture_buffer = g_malloc (buffer_size);

  gimp_progress_update (0.3);

  gegl_buffer_get (buffer, GEGL_RECTANGLE (0, 0,
                   drawable_width, drawable_height), 1.0,
                   file_format, picture_buffer,
                   GEGL_AUTO_ROWSTRIDE, GEGL_ABYSS_NONE);

  g_object_unref (buffer);

  gimp_progress_update (0.4);

  encoder = JxlEncoderCreate (NULL);
  if (!encoder)
    {
      g_set_error (error, G_FILE_ERROR, 0,
                   "Failed to create Jxl encoder");
      g_free (picture_buffer);
      if (profile)
        {
          g_object_unref (profile);
        }
      return FALSE;
    }

  num_worker_threads = g_get_num_processors ();
  if (num_worker_threads > 16)
    {
      num_worker_threads = 16;
    }
  runner = JxlThreadParallelRunnerCreate (NULL, num_worker_threads);
  if (JxlEncoderSetParallelRunner (encoder, JxlThreadParallelRunner, runner) != JXL_ENC_SUCCESS)
    {
      g_set_error (error, G_FILE_ERROR, 0,
                   "JxlEncoderSetParallelRunner failed");
      JxlThreadParallelRunnerDestroy (runner);
      JxlEncoderDestroy (encoder);
      g_free (picture_buffer);
      if (profile)
        {
          g_object_unref (profile);
        }
      return FALSE;
    }

  status = JxlEncoderSetBasicInfo (encoder, &output_info);
  if (status != JXL_ENC_SUCCESS)
    {
      g_set_error (error, G_FILE_ERROR, 0,
                   "JxlEncoderSetBasicInfo failed!");
      JxlThreadParallelRunnerDestroy (runner);
      JxlEncoderDestroy (encoder);
      g_free (picture_buffer);
      if (profile)
        {
          g_object_unref (profile);
        }
      return FALSE;
    }

  if (save_icc)
    {
      const uint8_t *icc_data = NULL;
      size_t         icc_length = 0;

      icc_data = gimp_color_profile_get_icc_profile (profile, &icc_length);
      status = JxlEncoderSetICCProfile (encoder, icc_data, icc_length);
      g_object_unref (profile);
      profile = NULL;

      if (status != JXL_ENC_SUCCESS)
        {
          g_set_error (error, G_FILE_ERROR, 0,
                       "JxlEncoderSetICCProfile failed!");
          JxlThreadParallelRunnerDestroy (runner);
          JxlEncoderDestroy (encoder);
          g_free (picture_buffer);
          return FALSE;
        }
    }
  else
    {
      if (profile)
        {
          g_object_unref (profile);
          profile = NULL;
        }

      status = JxlEncoderSetColorEncoding (encoder, &color_profile);
      if (status != JXL_ENC_SUCCESS)
        {
          g_set_error (error, G_FILE_ERROR, 0,
                       "JxlEncoderSetColorEncoding failed!");
          JxlThreadParallelRunnerDestroy (runner);
          JxlEncoderDestroy (encoder);
          g_free (picture_buffer);
          return FALSE;
        }
    }

  encoder_options = JxlEncoderFrameSettingsCreate (encoder, NULL);
  JxlEncoderSetFrameDistance (encoder_options, 0);
  JxlEncoderSetFrameLossless (encoder_options, JXL_TRUE);

  gimp_progress_update (0.5);

  status = JxlEncoderAddImageFrame (encoder_options, &pixel_format, picture_buffer, buffer_size);
  if (status != JXL_ENC_SUCCESS)
    {
      g_set_error (error, G_FILE_ERROR, 0,
                   "JxlEncoderAddImageFrame failed!");
      JxlThreadParallelRunnerDestroy (runner);
      JxlEncoderDestroy (encoder);
      g_free (picture_buffer);
      return FALSE;
    }

  gimp_progress_update (0.65);

  JxlEncoderCloseInput (encoder);

  gimp_progress_update (0.7);

  compressed = g_byte_array_sized_new (4096);
  g_byte_array_set_size (compressed, 4096);
  do
    {
      next_out = compressed->data + offset;
      avail_out = compressed->len - offset;
      status = JxlEncoderProcessOutput (encoder, &next_out, &avail_out);

      if (status == JXL_ENC_NEED_MORE_OUTPUT)
        {
          offset = next_out - compressed->data;
          g_byte_array_set_size (compressed, compressed->len * 2);
        }
      else if (status == JXL_ENC_ERROR)
        {
          g_set_error (error, G_FILE_ERROR, 0,
                       "JxlEncoderProcessOutput failed!");
          JxlThreadParallelRunnerDestroy (runner);
          JxlEncoderDestroy (encoder);
          g_free (picture_buffer);
          return FALSE;
        }
    }
  while (status != JXL_ENC_SUCCESS);

  JxlThreadParallelRunnerDestroy (runner);
  JxlEncoderDestroy (encoder);

  g_free (picture_buffer);

  g_byte_array_set_size (compressed, next_out - compressed->data);

  gimp_progress_update (0.8);

  if (compressed->len > 0)
    {
      outfile = g_fopen (g_file_peek_path (file), "wb");
      if (!outfile)
        {
          g_set_error (error, G_FILE_ERROR, 0,
                       "Could not open '%s' for writing!\n",
                       g_file_peek_path (file));
          g_byte_array_free (compressed, TRUE);
          return FALSE;
        }

      fwrite (compressed->data, 1, compressed->len, outfile);
      fclose (outfile);

      gimp_progress_update (1.0);

      g_byte_array_free (compressed, TRUE);
      return TRUE;
    }

  g_set_error (error, G_FILE_ERROR, 0,
               "No data to write");
  g_byte_array_free (compressed, TRUE);
  return FALSE;
}

static void
run (const gchar     *name,
     gint             nparams,
     const GimpParam *param,
     gint            *nreturn_vals,
     GimpParam      **return_vals)
{
  static GimpParam  values[6];
  GimpRunMode       run_mode;
  GimpPDBStatusType status = GIMP_PDB_SUCCESS;
  gint32            image_ID;
  GError           *error = NULL;

  run_mode = param[0].data.d_int32;

  INIT_I18N ();
  gegl_init (NULL, NULL);

  *nreturn_vals           = 1;
  *return_vals            = values;
  values[0].type          = GIMP_PDB_STATUS;
  values[0].data.d_status = GIMP_PDB_EXECUTION_ERROR;

  if (strcmp (name, LOAD_PROC) == 0)
    {
      switch (run_mode)
        {
        case GIMP_RUN_INTERACTIVE:
        case GIMP_RUN_WITH_LAST_VALS:
          gimp_ui_init (PLUG_IN_BINARY, FALSE);
          break;
        default:
          break;
        }

      image_ID = load_image (param[1].data.d_string, run_mode, &error);

      if (image_ID != -1)
        {
          *nreturn_vals          = 2;
          values[1].type         = GIMP_PDB_IMAGE;
          values[1].data.d_image = image_ID;
        }
      else
        {
          status = GIMP_PDB_EXECUTION_ERROR;
        }
    }
  else if (strcmp (name, SAVE_PROC) == 0)
    {
      gint32                 image_ID    = param[1].data.d_int32;
      gint32                 drawable_ID = param[2].data.d_int32;
      GimpExportReturn       export      = GIMP_EXPORT_CANCEL;

      switch (run_mode)
        {
        case GIMP_RUN_INTERACTIVE:
        case GIMP_RUN_WITH_LAST_VALS:
          gimp_ui_init (PLUG_IN_BINARY, FALSE);

          export = gimp_export_image (&image_ID, &drawable_ID, "JPEG XL",
                                      GIMP_EXPORT_CAN_HANDLE_RGB |
                                      GIMP_EXPORT_CAN_HANDLE_GRAY |
                                      GIMP_EXPORT_CAN_HANDLE_ALPHA);

          if (export == GIMP_EXPORT_CANCEL)
            {
              values[0].data.d_status = GIMP_PDB_CANCEL;
              return;
            }
          break;

        default:
          break;
        }

      if (status == GIMP_PDB_SUCCESS)
        {
          GFile *file = g_file_new_for_uri (param[3].data.d_string);

          if (!save_image (file, image_ID, drawable_ID, &error))
            {
              status = GIMP_PDB_EXECUTION_ERROR;
            }

          g_object_unref (file);
        }

      if (export == GIMP_EXPORT_EXPORT)
        gimp_image_delete (image_ID);
    }
  else
    {
      status = GIMP_PDB_CALLING_ERROR;
    }

  if (status != GIMP_PDB_SUCCESS && error)
    {
      *nreturn_vals           = 2;
      values[1].type          = GIMP_PDB_STRING;
      values[1].data.d_string = error->message;
    }

  values[0].data.d_status = status;
}
