/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * file-jpegxl - JPEG XL file format plug-in for the LIGMA
 * Copyright (C) 2021  Daniel Novomesky
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

#include <libligma/ligma.h>
#include <libligma/ligmaui.h>

#include "libligma/stdplugins-intl.h"

#define LOAD_PROC       "file-jpegxl-load"
#define SAVE_PROC       "file-jpegxl-save"
#define PLUG_IN_BINARY  "file-jpegxl"

typedef struct _JpegXL      JpegXL;
typedef struct _JpegXLClass JpegXLClass;

struct _JpegXL
{
  LigmaPlugIn      parent_instance;
};

struct _JpegXLClass
{
  LigmaPlugInClass parent_class;
};


#define JPEGXL_TYPE  (jpegxl_get_type ())
#define JPEGXL (obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), JPEGXL_TYPE, JpegXL))

GType                   jpegxl_get_type (void) G_GNUC_CONST;

static GList           *jpegxl_query_procedures (LigmaPlugIn           *plug_in);
static LigmaProcedure   *jpegxl_create_procedure (LigmaPlugIn           *plug_in,
                                                 const gchar          *name);

static LigmaValueArray *jpegxl_load (LigmaProcedure        *procedure,
                                    LigmaRunMode           run_mode,
                                    GFile                *file,
                                    const LigmaValueArray *args,
                                    gpointer              run_data);
static LigmaValueArray *jpegxl_save (LigmaProcedure        *procedure,
                                    LigmaRunMode           run_mode,
                                    LigmaImage            *image,
                                    gint                  n_drawables,
                                    LigmaDrawable        **drawables,
                                    GFile                *file,
                                    const LigmaValueArray *args,
                                    gpointer              run_data);


G_DEFINE_TYPE (JpegXL, jpegxl, LIGMA_TYPE_PLUG_IN)

LIGMA_MAIN (JPEGXL_TYPE)
DEFINE_STD_SET_I18N

static void
jpegxl_class_init (JpegXLClass *klass)
{
  LigmaPlugInClass *plug_in_class = LIGMA_PLUG_IN_CLASS (klass);

  plug_in_class->query_procedures = jpegxl_query_procedures;
  plug_in_class->create_procedure = jpegxl_create_procedure;
  plug_in_class->set_i18n         = STD_SET_I18N;
}

static void
jpegxl_init (JpegXL *jpeg_xl)
{
}

static GList *
jpegxl_query_procedures (LigmaPlugIn *plug_in)
{
  GList *list = NULL;

  list = g_list_append (list, g_strdup (LOAD_PROC));
  list = g_list_append (list, g_strdup (SAVE_PROC));

  return list;
}

static LigmaProcedure *
jpegxl_create_procedure (LigmaPlugIn  *plug_in,
                         const gchar *name)
{
  LigmaProcedure *procedure = NULL;

  if (! strcmp (name, LOAD_PROC))
    {
      procedure = ligma_load_procedure_new (plug_in, name,
                                           LIGMA_PDB_PROC_TYPE_PLUGIN,
                                           jpegxl_load, NULL, NULL);

      ligma_procedure_set_menu_label (procedure, _("JPEG XL image"));

      ligma_procedure_set_documentation (procedure,
                                        _("Loads files in the JPEG XL file format"),
                                        _("Loads files in the JPEG XL file format"),
                                        name);
      ligma_procedure_set_attribution (procedure,
                                      "Daniel Novomesky",
                                      "(C) 2021 Daniel Novomesky",
                                      "2021");

      ligma_file_procedure_set_mime_types (LIGMA_FILE_PROCEDURE (procedure),
                                          "image/jxl");
      ligma_file_procedure_set_extensions (LIGMA_FILE_PROCEDURE (procedure),
                                          "jxl");
      ligma_file_procedure_set_magics (LIGMA_FILE_PROCEDURE (procedure),
                                      "0,string,\xFF\x0A,0,string,\\000\\000\\000\x0CJXL\\040\\015\\012\x87\\012");

    }
  else if (! strcmp (name, SAVE_PROC))
    {
      procedure = ligma_save_procedure_new (plug_in, name,
                                           LIGMA_PDB_PROC_TYPE_PLUGIN,
                                           jpegxl_save, NULL, NULL);

      ligma_procedure_set_image_types (procedure, "RGB*, GRAY*");

      ligma_procedure_set_menu_label (procedure, _("JPEG XL image"));

      ligma_procedure_set_documentation (procedure,
                                        _("Saves files in the JPEG XL file format"),
                                        _("Saves files in the JPEG XL file format"),
                                        name);
      ligma_procedure_set_attribution (procedure,
                                      "Daniel Novomesky",
                                      "(C) 2021 Daniel Novomesky",
                                      "2021");

      ligma_file_procedure_set_format_name (LIGMA_FILE_PROCEDURE (procedure),
                                           "JPEG XL");
      ligma_file_procedure_set_mime_types (LIGMA_FILE_PROCEDURE (procedure),
                                          "image/jxl");
      ligma_file_procedure_set_extensions (LIGMA_FILE_PROCEDURE (procedure),
                                          "jxl");

      LIGMA_PROC_ARG_BOOLEAN (procedure, "lossless",
                             _("L_ossless"),
                             _("Use lossless compression"),
                             FALSE,
                             G_PARAM_READWRITE);

      LIGMA_PROC_ARG_DOUBLE (procedure, "compression",
                            _("Co_mpression/maxError"),
                            _("Max. butteraugli distance, lower = higher quality. Range: 0 .. 15. 1.0 = visually lossless."),
                            0.1, 15, 1,
                            G_PARAM_READWRITE);

      LIGMA_PROC_ARG_INT (procedure, "save-bit-depth",
                         _("_Bit depth"),
                         _("Bit depth of exported image"),
                         8, 16, 8,
                         G_PARAM_READWRITE);

      LIGMA_PROC_ARG_INT (procedure, "speed",
                         _("Effort/S_peed"),
                         _("Encoder effort setting"),
                         1, 9,
                         7,
                         G_PARAM_READWRITE);

      LIGMA_PROC_ARG_BOOLEAN (procedure, "uses-original-profile",
                             _("Save ori_ginal profile"),
                             _("Store ICC profile to exported JXL file"),
                             FALSE,
                             G_PARAM_READWRITE);

      LIGMA_PROC_ARG_BOOLEAN (procedure, "save-exif",
                             _("Save Exi_f"),
                             _("Toggle saving Exif data"),
                             ligma_export_exif (),
                             G_PARAM_READWRITE);

      LIGMA_PROC_ARG_BOOLEAN (procedure, "save-xmp",
                             _("Save _XMP"),
                             _("Toggle saving XMP data"),
                             ligma_export_xmp (),
                             G_PARAM_READWRITE);
    }

  return procedure;
}

static LigmaImage *
load_image (GFile        *file,
            LigmaRunMode   runmode,
            GError      **error)
{
  FILE             *inputFile = g_fopen (g_file_peek_path (file), "rb");

  gsize             inputFileSize;
  gpointer          memory;

  JxlSignature      signature;
  JxlDecoder       *decoder;
  void             *runner;
  JxlBasicInfo      basicinfo;
  JxlDecoderStatus  status;
  JxlPixelFormat    pixel_format;
  JxlColorEncoding  color_encoding;
  size_t            icc_size = 0;
  LigmaColorProfile *profile = NULL;
  gboolean          loadlinear = FALSE;
  size_t            channel_depth;
  size_t            result_size;
  gpointer          picture_buffer;

  LigmaImage        *image;
  LigmaLayer        *layer;
  GeglBuffer       *buffer;
  LigmaPrecision     precision_linear;
  LigmaPrecision     precision_non_linear;

  if (!inputFile)
    {
      g_set_error (error, G_FILE_ERROR, 0,
                   "Cannot open file for read: %s\n",
                   g_file_peek_path (file));
      return NULL;
    }

  fseek (inputFile, 0, SEEK_END);
  inputFileSize = ftell (inputFile);
  fseek (inputFile, 0, SEEK_SET);

  if (inputFileSize < 1)
    {
      g_set_error (error, G_FILE_ERROR, 0,
                   "File too small: %s\n",
                   g_file_peek_path (file));
      fclose (inputFile);
      return NULL;
    }

  memory = g_malloc (inputFileSize);
  if (fread (memory, 1, inputFileSize, inputFile) != inputFileSize)
    {
      g_set_error (error, G_FILE_ERROR, 0,
                   "Failed to read %zu bytes: %s\n", inputFileSize,
                   g_file_peek_path (file));
      fclose (inputFile);
      g_free (memory);
      return NULL;
    }

  fclose (inputFile);

  signature = JxlSignatureCheck (memory, inputFileSize);
  if (signature != JXL_SIG_CODESTREAM && signature != JXL_SIG_CONTAINER)
    {
      g_set_error (error, G_FILE_ERROR, 0,
                   "File %s is probably not in JXL format!\n",
                   g_file_peek_path (file));
      g_free (memory);
      return NULL;
    }

  decoder = JxlDecoderCreate (NULL);
  if (!decoder)
    {
      g_set_error (error, G_FILE_ERROR, 0,
                   "ERROR: JxlDecoderCreate failed");
      g_free (memory);
      return NULL;
    }

  runner = JxlThreadParallelRunnerCreate (NULL, ligma_get_num_processors ());
  if (JxlDecoderSetParallelRunner (decoder, JxlThreadParallelRunner, runner) != JXL_DEC_SUCCESS)
    {
      g_set_error (error, G_FILE_ERROR, 0,
                   "ERROR: JxlDecoderSetParallelRunner failed");
      JxlThreadParallelRunnerDestroy (runner);
      JxlDecoderDestroy (decoder);
      g_free (memory);
      return NULL;
    }

  if (JxlDecoderSetInput (decoder, memory, inputFileSize) != JXL_DEC_SUCCESS)
    {
      g_set_error (error, G_FILE_ERROR, 0,
                   "ERROR: JxlDecoderSetInput failed");
      JxlThreadParallelRunnerDestroy (runner);
      JxlDecoderDestroy (decoder);
      g_free (memory);
      return NULL;
    }

  JxlDecoderCloseInput (decoder);

  if (JxlDecoderSubscribeEvents (decoder, JXL_DEC_BASIC_INFO | JXL_DEC_COLOR_ENCODING | JXL_DEC_FULL_IMAGE) != JXL_DEC_SUCCESS)
    {
      g_set_error (error, G_FILE_ERROR, 0,
                   "ERROR: JxlDecoderSubscribeEvents failed");
      JxlThreadParallelRunnerDestroy (runner);
      JxlDecoderDestroy (decoder);
      g_free (memory);
      return NULL;
    }

  status = JxlDecoderProcessInput (decoder);
  if (status == JXL_DEC_ERROR)
    {
      g_set_error (error, G_FILE_ERROR, 0,
                   "ERROR: JXL decoding failed");
      JxlThreadParallelRunnerDestroy (runner);
      JxlDecoderDestroy (decoder);
      g_free (memory);
      return NULL;
    }

  if (status == JXL_DEC_NEED_MORE_INPUT)
    {
      g_set_error (error, G_FILE_ERROR, 0,
                   "ERROR: JXL data incomplete");
      JxlThreadParallelRunnerDestroy (runner);
      JxlDecoderDestroy (decoder);
      g_free (memory);
      return NULL;
    }

  status = JxlDecoderGetBasicInfo (decoder, &basicinfo);
  if (status != JXL_DEC_SUCCESS)
    {
      g_set_error (error, G_FILE_ERROR, 0,
                   "ERROR: JXL basic info not available");
      JxlThreadParallelRunnerDestroy (runner);
      JxlDecoderDestroy (decoder);
      g_free (memory);
      return NULL;
    }

  if (basicinfo.xsize == 0 || basicinfo.ysize == 0)
    {
      g_set_error (error, G_FILE_ERROR, 0,
                   "ERROR: JXL image has zero dimensions");
      JxlThreadParallelRunnerDestroy (runner);
      JxlDecoderDestroy (decoder);
      g_free (memory);
      return NULL;
    }

  status = JxlDecoderProcessInput (decoder);
  if (status != JXL_DEC_COLOR_ENCODING)
    {
      g_set_error (error, G_FILE_ERROR, 0,
                   "Unexpected event %d instead of JXL_DEC_COLOR_ENCODING", status);
      JxlThreadParallelRunnerDestroy (runner);
      JxlDecoderDestroy (decoder);
      g_free (memory);
      return NULL;
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
  pixel_format.align = 0;

  if (basicinfo.uses_original_profile == JXL_FALSE || basicinfo.bits_per_sample > 16)
    {
      pixel_format.data_type = JXL_TYPE_FLOAT;
      channel_depth = 4;
      precision_linear = LIGMA_PRECISION_FLOAT_LINEAR;
      precision_non_linear = LIGMA_PRECISION_FLOAT_NON_LINEAR;
    }
  else if (basicinfo.bits_per_sample <= 8)
    {
      pixel_format.data_type = JXL_TYPE_UINT8;
      channel_depth = 1;
      precision_linear = LIGMA_PRECISION_U8_LINEAR;
      precision_non_linear = LIGMA_PRECISION_U8_NON_LINEAR;
    }
  else
    {
      pixel_format.data_type = JXL_TYPE_UINT16;
      channel_depth = 2;
      precision_linear = LIGMA_PRECISION_U16_LINEAR;
      precision_non_linear = LIGMA_PRECISION_U16_NON_LINEAR;
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

  result_size = channel_depth * pixel_format.num_channels * (size_t) basicinfo.xsize * (size_t) basicinfo.ysize;

  if (JxlDecoderGetColorAsEncodedProfile (decoder, &pixel_format,
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
                  profile = ligma_color_profile_new_rgb_srgb_linear ();
                  break;
                case JXL_COLOR_SPACE_GRAY:
                  profile = ligma_color_profile_new_d65_gray_linear ();
                  break;
                default:
                  break;
                }
              break;
            case JXL_TRANSFER_FUNCTION_SRGB:
              switch (color_encoding.color_space)
                {
                case JXL_COLOR_SPACE_RGB:
                  profile = ligma_color_profile_new_rgb_srgb ();
                  break;
                case JXL_COLOR_SPACE_GRAY:
                  profile = ligma_color_profile_new_d65_gray_srgb_trc ();
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

  if (!profile)
    {
      if (JxlDecoderGetICCProfileSize (decoder, &pixel_format,
                                       JXL_COLOR_PROFILE_TARGET_DATA,
                                       &icc_size) == JXL_DEC_SUCCESS)
        {
          if (icc_size > 0)
            {
              gpointer raw_icc_profile = g_malloc (icc_size);

              if (JxlDecoderGetColorAsICCProfile (decoder, &pixel_format,
                                                  JXL_COLOR_PROFILE_TARGET_DATA,
                                                  raw_icc_profile, icc_size)
                  == JXL_DEC_SUCCESS)
                {
                  profile = ligma_color_profile_new_from_icc_profile (raw_icc_profile, icc_size, error);
                  if (profile)
                    {
                      loadlinear = ligma_color_profile_is_linear (profile);
                    }
                  else
                    {
                      g_printerr ("%s: Failed to read ICC profile: %s\n", G_STRFUNC, (*error)->message);
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
      g_set_error (error, G_FILE_ERROR, 0,
                   "Unexpected event %d instead of JXL_DEC_NEED_IMAGE_OUT_BUFFER", status);
      if (profile)
        {
          g_object_unref (profile);
        }
      JxlThreadParallelRunnerDestroy (runner);
      JxlDecoderDestroy (decoder);
      g_free (memory);
      return NULL;
    }

  picture_buffer = g_try_malloc (result_size);
  if (!picture_buffer)
    {
      g_set_error (error, G_FILE_ERROR, 0, "Memory could not be allocated.");
      if (profile)
        {
          g_object_unref (profile);
        }
      JxlThreadParallelRunnerDestroy (runner);
      JxlDecoderDestroy (decoder);
      g_free (memory);
      return NULL;
    }

  if (JxlDecoderSetImageOutBuffer (decoder, &pixel_format, picture_buffer, result_size) != JXL_DEC_SUCCESS)
    {
      g_set_error (error, G_FILE_ERROR, 0,
                   "ERROR: JxlDecoderSetImageOutBuffer failed");
      if (profile)
        {
          g_object_unref (profile);
        }
      JxlThreadParallelRunnerDestroy (runner);
      JxlDecoderDestroy (decoder);
      g_free (memory);
      return NULL;
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
      return NULL;
    }


  if (basicinfo.num_color_channels == 1) /* grayscale */
    {
      image = ligma_image_new_with_precision (basicinfo.xsize, basicinfo.ysize, LIGMA_GRAY,
                                             loadlinear ? precision_linear : precision_non_linear);

      if (profile)
        {
          if (ligma_color_profile_is_gray (profile))
            {
              ligma_image_set_color_profile (image, profile);
            }
        }

      layer = ligma_layer_new (image, "Background",
                              basicinfo.xsize, basicinfo.ysize,
                              (basicinfo.alpha_bits > 0) ? LIGMA_GRAYA_IMAGE : LIGMA_GRAY_IMAGE, 100,
                              ligma_image_get_default_new_layer_mode (image));
    }
  else /* RGB */
    {
      image = ligma_image_new_with_precision (basicinfo.xsize, basicinfo.ysize, LIGMA_RGB,
                                             loadlinear ? precision_linear : precision_non_linear);

      if (profile)
        {
          if (ligma_color_profile_is_rgb (profile))
            {
              ligma_image_set_color_profile (image, profile);
            }
        }

      layer = ligma_layer_new (image, "Background",
                              basicinfo.xsize, basicinfo.ysize,
                              (basicinfo.alpha_bits > 0) ? LIGMA_RGBA_IMAGE : LIGMA_RGB_IMAGE, 100,
                              ligma_image_get_default_new_layer_mode (image));

    }

  ligma_image_insert_layer (image, layer, NULL, 0);

  buffer = ligma_drawable_get_buffer (LIGMA_DRAWABLE (layer));

  gegl_buffer_set (buffer, GEGL_RECTANGLE (0, 0, basicinfo.xsize, basicinfo.ysize), 0,
                   NULL, picture_buffer, GEGL_AUTO_ROWSTRIDE);

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
                  LigmaMetadata *metadata = ligma_metadata_new ();

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
                      if (! ligma_metadata_set_from_xmp (metadata, xml_box->data, xml_box->len, error))
                        {
                          g_printerr ("%s: Failed to set XMP metadata: %s\n", G_STRFUNC, (*error)->message);
                          g_clear_error (error);
                        }
                    }

                  gexiv2_metadata_try_set_orientation (GEXIV2_METADATA (metadata),
                                                       GEXIV2_ORIENTATION_NORMAL, NULL);
                  gexiv2_metadata_try_set_metadata_pixel_width (GEXIV2_METADATA (metadata),
                                                                basicinfo.xsize, NULL);
                  gexiv2_metadata_try_set_metadata_pixel_height (GEXIV2_METADATA (metadata),
                                                                 basicinfo.ysize, NULL);
                  ligma_image_metadata_load_finish (image, "image/jxl", metadata,
                                                   LIGMA_METADATA_LOAD_COMMENT | LIGMA_METADATA_LOAD_RESOLUTION);
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

static LigmaValueArray *
jpegxl_load (LigmaProcedure        *procedure,
             LigmaRunMode           run_mode,
             GFile                *file,
             const LigmaValueArray *args,
             gpointer              run_data)
{
  LigmaValueArray *return_vals;
  LigmaImage      *image;
  GError         *error             = NULL;

  gegl_init (NULL, NULL);

  switch (run_mode)
    {
    case LIGMA_RUN_INTERACTIVE:
    case LIGMA_RUN_WITH_LAST_VALS:
      ligma_ui_init (PLUG_IN_BINARY);
      break;

    default:
      break;
    }

  image = load_image (file, run_mode, &error);

  if (! image)
    {
      return ligma_procedure_new_return_values (procedure,
                                               LIGMA_PDB_EXECUTION_ERROR,
                                               error);
    }
  return_vals = ligma_procedure_new_return_values (procedure,
                                                  LIGMA_PDB_SUCCESS,
                                                  NULL);

  LIGMA_VALUES_SET_IMAGE (return_vals, 1, image);

  return return_vals;
}

static gboolean
save_image (GFile               *file,
            LigmaProcedureConfig *config,
            LigmaImage           *image,
            LigmaDrawable        *drawable,
            LigmaMetadata        *metadata,
            GError             **error)
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
  LigmaImageType            drawable_type;

  gint                     drawable_width;
  gint                     drawable_height;
  gpointer                 picture_buffer;

  LigmaColorProfile        *profile = NULL;
  const Babl              *file_format = NULL;
  const Babl              *space = NULL;
  gboolean                 out_linear = FALSE;

  size_t                   offset = 0;
  uint8_t                 *next_out;
  size_t                   avail_out;

  gdouble                  compression = 1.0;
  gboolean                 lossless = FALSE;
  gint                     speed = 7;
  gint                     bit_depth = 8;
  gboolean                 uses_original_profile = FALSE;
  gboolean                 save_exif = FALSE;
  gboolean                 save_xmp = FALSE;

  ligma_progress_init_printf (_("Exporting '%s'"),
                             ligma_file_get_utf8_name (file));

  g_object_get (config,
                "lossless",              &lossless,
                "compression",           &compression,
                "speed",                 &speed,
                "save-bit-depth",        &bit_depth,
                "uses-original-profile", &uses_original_profile,
                "save-exif",             &save_exif,
                "save-xmp",              &save_xmp,
                NULL);

  if (lossless)
    {
      /* JPEG XL developers recommend enabling uses_original_profile
       * for better lossless compression efficiency. */
      uses_original_profile = TRUE;
    }
  else
    {
      /* 0.1 is actually minimal value for lossy in libjxl 0.5
       * 0.01 is allowed in libjxl 0.6 but
       * using too low value with lossy compression is not wise */
      if (compression < 0.1)
        {
          compression = 0.1;
        }
    }

  drawable_type   = ligma_drawable_type (drawable);
  drawable_width  = ligma_drawable_get_width (drawable);
  drawable_height = ligma_drawable_get_height (drawable);

  JxlEncoderInitBasicInfo(&output_info);

  if (uses_original_profile)
    {
      output_info.uses_original_profile = JXL_TRUE;

      profile = ligma_image_get_effective_color_profile (image);
      out_linear = ligma_color_profile_is_linear (profile);

      space = ligma_color_profile_get_space (profile,
                                            LIGMA_COLOR_RENDERING_INTENT_RELATIVE_COLORIMETRIC,
                                            error);

      if (error && *error)
        {
          g_printerr ("%s: error getting the profile space: %s\n",
                      G_STRFUNC, (*error)->message);
          return FALSE;
        }
    }
  else
    {
      output_info.uses_original_profile = JXL_FALSE;
      space = babl_space ("sRGB");
      out_linear = FALSE;
    }

  if (bit_depth > 8)
    {
      pixel_format.data_type = JXL_TYPE_UINT16;
      output_info.bits_per_sample = 16;
    }
  else
    {
      pixel_format.data_type = JXL_TYPE_UINT8;
      output_info.bits_per_sample = 8;
    }

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
    case LIGMA_GRAYA_IMAGE:
      if (uses_original_profile && out_linear)
        {
          file_format = babl_format ( (bit_depth > 8) ? "YA u16" : "YA u8");
          JxlColorEncodingSetToLinearSRGB (&color_profile, JXL_TRUE);
        }
      else
        {
          file_format = babl_format ( (bit_depth > 8) ? "Y'A u16" : "Y'A u8");
          JxlColorEncodingSetToSRGB (&color_profile, JXL_TRUE);
        }
      pixel_format.num_channels = 2;
      output_info.num_color_channels = 1;
      output_info.alpha_bits = (bit_depth > 8) ? 16 : 8;
      output_info.alpha_exponent_bits = 0;
      output_info.num_extra_channels = 1;

      uses_original_profile = FALSE;
      break;
    case LIGMA_GRAY_IMAGE:
      if (uses_original_profile && out_linear)
        {
          file_format = babl_format ( (bit_depth > 8) ? "Y u16" : "Y u8");
          JxlColorEncodingSetToLinearSRGB (&color_profile, JXL_TRUE);
        }
      else
        {
          file_format = babl_format ( (bit_depth > 8) ? "Y' u16" : "Y' u8");
          JxlColorEncodingSetToSRGB (&color_profile, JXL_TRUE);
        }
      pixel_format.num_channels = 1;
      output_info.num_color_channels = 1;
      output_info.alpha_bits = 0;

      uses_original_profile = FALSE;
      break;
    case LIGMA_RGBA_IMAGE:
      if (bit_depth > 8)
        {
          file_format = babl_format_with_space (out_linear ? "RGBA u16" : "R'G'B'A u16", space);
          output_info.alpha_bits = 16;
        }
      else
        {
          file_format = babl_format_with_space (out_linear ? "RGBA u8" : "R'G'B'A u8", space);
          output_info.alpha_bits = 8;
        }
      pixel_format.num_channels = 4;
      JxlColorEncodingSetToSRGB (&color_profile, JXL_FALSE);
      output_info.num_color_channels = 3;
      output_info.alpha_exponent_bits = 0;
      output_info.num_extra_channels = 1;
      break;
    case LIGMA_RGB_IMAGE:
      if (bit_depth > 8)
        {
          file_format = babl_format_with_space (out_linear ? "RGB u16" : "R'G'B' u16", space);
        }
      else
        {
          file_format = babl_format_with_space (out_linear ? "RGB u8" : "R'G'B' u8", space);
        }
      pixel_format.num_channels = 3;
      JxlColorEncodingSetToSRGB (&color_profile, JXL_FALSE);
      output_info.num_color_channels = 3;
      output_info.alpha_bits = 0;
      break;
    default:
      if (profile)
        {
          g_object_unref (profile);
        }
      return FALSE;
      break;
    }


  if (bit_depth > 8)
    {
      buffer_size = 2 * pixel_format.num_channels * (size_t) output_info.xsize * (size_t) output_info.ysize;
    }
  else
    {
      buffer_size = pixel_format.num_channels * (size_t) output_info.xsize * (size_t) output_info.ysize;
    }
  picture_buffer = g_malloc (buffer_size);

  ligma_progress_update (0.3);

  buffer = ligma_drawable_get_buffer (drawable);
  gegl_buffer_get (buffer, GEGL_RECTANGLE (0, 0,
                   drawable_width, drawable_height), 1.0,
                   file_format, picture_buffer,
                   GEGL_AUTO_ROWSTRIDE, GEGL_ABYSS_NONE);

  g_object_unref (buffer);

  ligma_progress_update (0.4);

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

  if ( (output_info.bits_per_sample > 12 && (output_info.uses_original_profile || output_info.alpha_bits > 12)) || (metadata && (save_exif || save_xmp)))
    {
      output_info.have_container = JXL_TRUE;
      JxlEncoderUseContainer (encoder, JXL_TRUE);

      if (output_info.bits_per_sample > 12 && (output_info.uses_original_profile || output_info.alpha_bits > 12))
        {
          JxlEncoderSetCodestreamLevel (encoder, 10);
        }

      if (metadata && (save_exif || save_xmp))
        {
          JxlEncoderUseBoxes (encoder);
        }
    }

  runner = JxlThreadParallelRunnerCreate (NULL, ligma_get_num_processors ());
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

  if (uses_original_profile)
    {
      const uint8_t *icc_data = NULL;
      size_t         icc_length = 0;

      icc_data = ligma_color_profile_get_icc_profile (profile, &icc_length);
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

  if (lossless)
    {
      JxlEncoderSetFrameDistance (encoder_options, 0);
      JxlEncoderSetFrameLossless (encoder_options, JXL_TRUE);
    }
  else
    {
      JxlEncoderSetFrameDistance (encoder_options, compression);
      JxlEncoderSetFrameLossless (encoder_options, JXL_FALSE);
    }

  status = JxlEncoderFrameSettingsSetOption (encoder_options, JXL_ENC_FRAME_SETTING_EFFORT, speed);
  if (status != JXL_ENC_SUCCESS)
    {
      g_printerr ("JxlEncoderFrameSettingsSetOption failed to set effort %d", speed);
    }

  ligma_progress_update (0.5);

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

  ligma_progress_update (0.65);

  if (metadata && (save_exif || save_xmp))
    {
      LigmaMetadata         *filtered_metadata;
      LigmaMetadataSaveFlags metadata_flags = 0;

      if (save_exif)
        {
          metadata_flags |= LIGMA_METADATA_SAVE_EXIF;
        }

      if (save_xmp)
        {
          metadata_flags |= LIGMA_METADATA_SAVE_XMP;
        }

      filtered_metadata = ligma_image_metadata_save_filter (image, "image/jxl", metadata, metadata_flags, NULL, error);
      if (! filtered_metadata)
        {
          if (error && *error)
            {
              g_printerr ("%s: error filtering metadata: %s",
                          G_STRFUNC, (*error)->message);
              g_clear_error (error);
            }
        }
      else
        {
          GExiv2Metadata *filtered_g2metadata = GEXIV2_METADATA (filtered_metadata);

          /*  EXIF metadata  */
          if (save_exif && gexiv2_metadata_has_exif (filtered_g2metadata))
            {
              GBytes *raw_exif_data;

              raw_exif_data = gexiv2_metadata_get_exif_data (filtered_g2metadata, GEXIV2_BYTE_ORDER_LITTLE, error);
              if (raw_exif_data)
                {
                  gsize exif_size = 0;
                  gconstpointer exif_buffer = g_bytes_get_data (raw_exif_data, &exif_size);

                  if (exif_size >= 4)
                    {
                      const JxlBoxType exif_box_type = { 'E', 'x', 'i', 'f' };
                      uint8_t         *content = g_new (uint8_t, exif_size + 4);

                      content[0] = 0;
                      content[1] = 0;
                      content[2] = 0;
                      content[3] = 0;
                      memcpy (content + 4, exif_buffer, exif_size);

                      if (JxlEncoderAddBox (encoder, exif_box_type, content, exif_size + 4, JXL_FALSE) != JXL_ENC_SUCCESS)
                        {
                          g_printerr ("%s: Failed to save EXIF metadata.\n", G_STRFUNC);
                        }

                      g_free (content);
                    }
                  g_bytes_unref (raw_exif_data);
                }
              else
                {
                  if (error && *error)
                    {
                      g_printerr ("%s: error preparing EXIF metadata: %s",
                                  G_STRFUNC, (*error)->message);
                      g_clear_error (error);
                    }
                }
            }

          /*  XMP metadata  */
          if (save_xmp && gexiv2_metadata_has_xmp (filtered_g2metadata))
            {
              gchar *xmp_packet;

              xmp_packet = gexiv2_metadata_try_generate_xmp_packet (filtered_g2metadata, GEXIV2_USE_COMPACT_FORMAT | GEXIV2_OMIT_ALL_FORMATTING, 0, NULL);
              if (xmp_packet)
                {
                  int xmp_size = strlen (xmp_packet);
                  if (xmp_size > 0)
                    {
                      const JxlBoxType xml_box_type = { 'x', 'm', 'l', ' ' };
                      if (JxlEncoderAddBox (encoder, xml_box_type, (const uint8_t *) xmp_packet, xmp_size, JXL_FALSE) != JXL_ENC_SUCCESS)
                        {
                          g_printerr ("%s: Failed to save XMP metadata.\n", G_STRFUNC);
                        }
                    }
                  g_free (xmp_packet);
                }
            }

          g_object_unref (filtered_metadata);
        }
    }

  JxlEncoderCloseInput (encoder);

  ligma_progress_update (0.7);

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

  ligma_progress_update (0.8);

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

      ligma_progress_update (1.0);

      g_byte_array_free (compressed, TRUE);
      return TRUE;
    }

  g_set_error (error, G_FILE_ERROR, 0,
               "No data to write");
  g_byte_array_free (compressed, TRUE);
  return FALSE;
}

static gboolean
save_dialog (LigmaImage     *image,
             LigmaProcedure *procedure,
             GObject       *config)
{
  GtkWidget    *dialog;
  GtkListStore *store;
  GtkWidget    *compression_scale;
  GtkWidget    *orig_profile_check;
  gboolean      run;

  dialog = ligma_save_procedure_dialog_new (LIGMA_SAVE_PROCEDURE (procedure),
                                           LIGMA_PROCEDURE_CONFIG (config),
                                           image);

  ligma_procedure_dialog_get_widget (LIGMA_PROCEDURE_DIALOG (dialog),
                                    "lossless", GTK_TYPE_CHECK_BUTTON);

  compression_scale = ligma_procedure_dialog_get_widget (LIGMA_PROCEDURE_DIALOG (dialog),
                                                        "compression",
                                                        LIGMA_TYPE_SCALE_ENTRY);

  g_object_bind_property (config,            "lossless",
                          compression_scale, "sensitive",
                          G_BINDING_SYNC_CREATE |
                          G_BINDING_INVERT_BOOLEAN);

  store = ligma_int_store_new (_("lightning (fastest)"), 1,
                              _("thunder"),             2,
                              _("falcon (faster)"),     3,
                              _("cheetah"),             4,
                              _("hare"),                5,
                              _("wombat"),              6,
                              _("squirrel"),            7,
                              _("kitten"),              8,
                              _("tortoise (slower)"),   9,
                              NULL);

  ligma_procedure_dialog_get_int_combo (LIGMA_PROCEDURE_DIALOG (dialog),
                                       "speed", LIGMA_INT_STORE (store));

  store = ligma_int_store_new (_("8 bit/channel"),   8,
                              _("16 bit/channel"), 16,
                              NULL);

  ligma_procedure_dialog_get_int_combo (LIGMA_PROCEDURE_DIALOG (dialog),
                                       "save-bit-depth", LIGMA_INT_STORE (store));

  orig_profile_check = ligma_procedure_dialog_get_widget (LIGMA_PROCEDURE_DIALOG (dialog),
                                                         "uses-original-profile",
                                                         GTK_TYPE_CHECK_BUTTON);

  g_object_bind_property (config,             "lossless",
                          orig_profile_check, "sensitive",
                          G_BINDING_SYNC_CREATE |
                          G_BINDING_INVERT_BOOLEAN);

  ligma_procedure_dialog_fill (LIGMA_PROCEDURE_DIALOG (dialog),
                              "lossless", "compression",
                              "speed", "save-bit-depth",
                              "uses-original-profile",
                              "save-exif", "save-xmp",
                              NULL);

  run = ligma_procedure_dialog_run (LIGMA_PROCEDURE_DIALOG (dialog));

  gtk_widget_destroy (dialog);

  return run;
}


static LigmaValueArray *
jpegxl_save (LigmaProcedure        *procedure,
             LigmaRunMode           run_mode,
             LigmaImage            *image,
             gint                  n_drawables,
             LigmaDrawable        **drawables,
             GFile                *file,
             const LigmaValueArray *args,
             gpointer              run_data)
{
  LigmaPDBStatusType      status = LIGMA_PDB_SUCCESS;
  LigmaProcedureConfig   *config;
  LigmaExportReturn       export = LIGMA_EXPORT_CANCEL;
  GError                *error  = NULL;

  gegl_init (NULL, NULL);

  config = ligma_procedure_create_config (procedure);
  ligma_procedure_config_begin_run (config, image, run_mode, args);

  switch (run_mode)
    {
    case LIGMA_RUN_INTERACTIVE:
    case LIGMA_RUN_WITH_LAST_VALS:
      ligma_ui_init (PLUG_IN_BINARY);

      export = ligma_export_image (&image, &n_drawables, &drawables, "JPEG XL",
                                  LIGMA_EXPORT_CAN_HANDLE_RGB |
                                  LIGMA_EXPORT_CAN_HANDLE_GRAY |
                                  LIGMA_EXPORT_CAN_HANDLE_ALPHA);

      if (export == LIGMA_EXPORT_CANCEL)
        {
          return ligma_procedure_new_return_values (procedure,
                 LIGMA_PDB_CANCEL,
                 NULL);
        }
      break;

    default:
      break;
    }

  if (n_drawables < 1)
    {
      g_set_error (&error, G_FILE_ERROR, 0,
                   "No drawables to export");

      return ligma_procedure_new_return_values (procedure,
             LIGMA_PDB_CALLING_ERROR,
             error);
    }

  if (run_mode == LIGMA_RUN_INTERACTIVE)
    {
      if (! save_dialog (image, procedure, G_OBJECT (config)))
        {
          status = LIGMA_PDB_CANCEL;
        }
    }

  if (status == LIGMA_PDB_SUCCESS)
    {
      LigmaMetadataSaveFlags metadata_flags;

      LigmaMetadata *metadata = ligma_image_metadata_save_prepare (image, "image/jxl", &metadata_flags);

      if (! save_image (file, config, image, drawables[0], metadata, &error))
        {
          status = LIGMA_PDB_EXECUTION_ERROR;
        }

      if (metadata)
        {
          g_object_unref (metadata);
        }
    }


  ligma_procedure_config_end_run (config, status);
  g_object_unref (config);

  if (export == LIGMA_EXPORT_EXPORT)
    {
      g_free (drawables);
      ligma_image_delete (image);
    }

  return ligma_procedure_new_return_values (procedure, status, error);
}
