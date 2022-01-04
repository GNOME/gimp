/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * file-jpegxl - JPEG XL file format plug-in for the GIMP
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

#include <glib/gstdio.h>

#include <jxl/decode.h>
#include <jxl/encode.h>
#include <jxl/thread_parallel_runner.h>

#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

#include "libgimp/stdplugins-intl.h"

#define LOAD_PROC       "file-jpegxl-load"
#define SAVE_PROC       "file-jpegxl-save"
#define PLUG_IN_BINARY  "file-jpegxl"

typedef struct _JpegXL      JpegXL;
typedef struct _JpegXLClass JpegXLClass;

struct _JpegXL
{
  GimpPlugIn      parent_instance;
};

struct _JpegXLClass
{
  GimpPlugInClass parent_class;
};


#define JPEGXL_TYPE  (jpegxl_get_type ())
#define JPEGXL (obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), JPEGXL_TYPE, JpegXL))

GType                   jpegxl_get_type (void) G_GNUC_CONST;

static GList           *jpegxl_query_procedures (GimpPlugIn           *plug_in);
static GimpProcedure   *jpegxl_create_procedure (GimpPlugIn           *plug_in,
                                                 const gchar          *name);

static GimpValueArray *jpegxl_load (GimpProcedure        *procedure,
                                    GimpRunMode           run_mode,
                                    GFile                *file,
                                    const GimpValueArray *args,
                                    gpointer              run_data);
static GimpValueArray *jpegxl_save (GimpProcedure        *procedure,
                                    GimpRunMode           run_mode,
                                    GimpImage            *image,
                                    gint                  n_drawables,
                                    GimpDrawable        **drawables,
                                    GFile                *file,
                                    const GimpValueArray *args,
                                    gpointer              run_data);


G_DEFINE_TYPE (JpegXL, jpegxl, GIMP_TYPE_PLUG_IN)

GIMP_MAIN (JPEGXL_TYPE)

static void
jpegxl_class_init (JpegXLClass *klass)
{
  GimpPlugInClass *plug_in_class = GIMP_PLUG_IN_CLASS (klass);

  plug_in_class->query_procedures = jpegxl_query_procedures;
  plug_in_class->create_procedure = jpegxl_create_procedure;
}

static void
jpegxl_init (JpegXL *jpeg_xl)
{
}

static GList *
jpegxl_query_procedures (GimpPlugIn *plug_in)
{
  GList *list = NULL;

  list = g_list_append (list, g_strdup (LOAD_PROC));
  list = g_list_append (list, g_strdup (SAVE_PROC));

  return list;
}

static GimpProcedure *
jpegxl_create_procedure (GimpPlugIn  *plug_in,
                         const gchar *name)
{
  GimpProcedure *procedure = NULL;

  if (! strcmp (name, LOAD_PROC))
    {
      procedure = gimp_load_procedure_new (plug_in, name,
                                           GIMP_PDB_PROC_TYPE_PLUGIN,
                                           jpegxl_load, NULL, NULL);

      gimp_procedure_set_menu_label (procedure, N_("JPEG XL image"));

      gimp_procedure_set_documentation (procedure,
                                        _("Loads files in the JPEG XL file format"),
                                        _("Loads files in the JPEG XL file format"),
                                        name);
      gimp_procedure_set_attribution (procedure,
                                      "Daniel Novomesky",
                                      "(C) 2021 Daniel Novomesky",
                                      "2021");

      gimp_file_procedure_set_mime_types (GIMP_FILE_PROCEDURE (procedure),
                                          "image/jxl");
      gimp_file_procedure_set_extensions (GIMP_FILE_PROCEDURE (procedure),
                                          "jxl");
      gimp_file_procedure_set_magics (GIMP_FILE_PROCEDURE (procedure),
                                      "0,string,\xFF\x0A,0,string,\\000\\000\\000\x0CJXL\\040\\015\\012\x87\\012");

    }
  else if (! strcmp (name, SAVE_PROC))
    {
      procedure = gimp_save_procedure_new (plug_in, name,
                                           GIMP_PDB_PROC_TYPE_PLUGIN,
                                           jpegxl_save, NULL, NULL);

      gimp_procedure_set_image_types (procedure, "RGB*, GRAY*");

      gimp_procedure_set_menu_label (procedure, N_("JPEG XL image"));

      gimp_procedure_set_documentation (procedure,
                                        _("Saves files in the JPEG XL file format"),
                                        _("Saves files in the JPEG XL file format"),
                                        name);
      gimp_procedure_set_attribution (procedure,
                                      "Daniel Novomesky",
                                      "(C) 2021 Daniel Novomesky",
                                      "2021");

      gimp_file_procedure_set_format_name (GIMP_FILE_PROCEDURE (procedure),
                                           "JPEG XL");
      gimp_file_procedure_set_mime_types (GIMP_FILE_PROCEDURE (procedure),
                                          "image/jxl");
      gimp_file_procedure_set_extensions (GIMP_FILE_PROCEDURE (procedure),
                                          "jxl");

      GIMP_PROC_ARG_BOOLEAN (procedure, "lossless",
                             _("L_ossless"),
                             _("Use lossless compression"),
                             FALSE,
                             G_PARAM_READWRITE);

      GIMP_PROC_ARG_DOUBLE (procedure, "compression",
                            _("Co_mpression/maxError"),
                            _("Max. butteraugli distance, lower = higher quality. Range: 0 .. 15. 1.0 = visually lossless."),
                            0, 15, 1,
                            G_PARAM_READWRITE);

      GIMP_PROC_ARG_INT (procedure, "save-bit-depth",
                         _("_Bit depth"),
                         _("Bit depth of exported image"),
                         8, 16, 8,
                         G_PARAM_READWRITE);

      GIMP_PROC_AUX_ARG_INT (procedure, "speed",
                             _("Effort/S_peed"),
                             _("Encoder effort setting"),
                             3, 9,
                             7,
                             G_PARAM_READWRITE);

      GIMP_PROC_ARG_BOOLEAN (procedure, "uses-original-profile",
                             _("Save ori_ginal profile"),
                             _("Store ICC profile to exported JXL file"),
                             FALSE,
                             G_PARAM_READWRITE);
    }

  return procedure;
}

static GimpImage *
load_image (GFile        *file,
            GimpRunMode   runmode,
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
  GimpColorProfile *profile = NULL;
  gboolean          loadlinear = FALSE;
  size_t            channel_depth;
  size_t            result_size;
  gpointer          picture_buffer;

  GimpImage        *image;
  GimpLayer        *layer;
  GeglBuffer       *buffer;
  GimpPrecision     precision_linear;
  GimpPrecision     precision_non_linear;

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

  runner = JxlThreadParallelRunnerCreate (NULL, gimp_get_num_processors ());
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
      precision_linear = GIMP_PRECISION_FLOAT_LINEAR;
      precision_non_linear = GIMP_PRECISION_FLOAT_NON_LINEAR;
    }
  else if (basicinfo.bits_per_sample <= 8)
    {
      pixel_format.data_type = JXL_TYPE_UINT8;
      channel_depth = 1;
      precision_linear = GIMP_PRECISION_U8_LINEAR;
      precision_non_linear = GIMP_PRECISION_U8_NON_LINEAR;
    }
  else
    {
      pixel_format.data_type = JXL_TYPE_UINT16;
      channel_depth = 2;
      precision_linear = GIMP_PRECISION_U16_LINEAR;
      precision_non_linear = GIMP_PRECISION_U16_NON_LINEAR;
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
                  profile = gimp_color_profile_new_from_icc_profile (raw_icc_profile, icc_size, error);
                  if (profile)
                    {
                      loadlinear = gimp_color_profile_is_linear (profile);
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
      image = gimp_image_new_with_precision (basicinfo.xsize, basicinfo.ysize, GIMP_GRAY,
                                             loadlinear ? precision_linear : precision_non_linear);

      if (profile)
        {
          if (gimp_color_profile_is_gray (profile))
            {
              gimp_image_set_color_profile (image, profile);
            }
        }

      layer = gimp_layer_new (image, "Background",
                              basicinfo.xsize, basicinfo.ysize,
                              (basicinfo.alpha_bits > 0) ? GIMP_GRAYA_IMAGE : GIMP_GRAY_IMAGE, 100,
                              gimp_image_get_default_new_layer_mode (image));
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

      layer = gimp_layer_new (image, "Background",
                              basicinfo.xsize, basicinfo.ysize,
                              (basicinfo.alpha_bits > 0) ? GIMP_RGBA_IMAGE : GIMP_RGB_IMAGE, 100,
                              gimp_image_get_default_new_layer_mode (image));

    }

  gimp_image_insert_layer (image, layer, NULL, 0);

  buffer = gimp_drawable_get_buffer (GIMP_DRAWABLE (layer));

  gegl_buffer_set (buffer, GEGL_RECTANGLE (0, 0, basicinfo.xsize, basicinfo.ysize), 0,
                   NULL, picture_buffer, GEGL_AUTO_ROWSTRIDE);

  g_object_unref (buffer);


  g_free (picture_buffer);
  if (profile)
    {
      g_object_unref (profile);
    }
  JxlThreadParallelRunnerDestroy (runner);
  JxlDecoderDestroy (decoder);
  g_free (memory);
  return image;
}

static GimpValueArray *
jpegxl_load (GimpProcedure        *procedure,
             GimpRunMode           run_mode,
             GFile                *file,
             const GimpValueArray *args,
             gpointer              run_data)
{
  GimpValueArray *return_vals;
  GimpImage      *image;
  GError         *error             = NULL;

  INIT_I18N ();
  gegl_init (NULL, NULL);

  switch (run_mode)
    {
    case GIMP_RUN_INTERACTIVE:
    case GIMP_RUN_WITH_LAST_VALS:
      gimp_ui_init (PLUG_IN_BINARY);
      break;

    default:
      break;
    }

  image = load_image (file, run_mode, &error);

  if (! image)
    {
      return gimp_procedure_new_return_values (procedure,
                                               GIMP_PDB_EXECUTION_ERROR,
                                               error);
    }
  return_vals = gimp_procedure_new_return_values (procedure,
                                                  GIMP_PDB_SUCCESS,
                                                  NULL);

  GIMP_VALUES_SET_IMAGE (return_vals, 1, image);

  return return_vals;
}


static gboolean
save_image (GFile                *file,
            GimpProcedureConfig  *config,
            GimpImage            *image,
            GimpDrawable         *drawable,
            GError              **error)
{
  JxlEncoder        *encoder;
  void              *runner;
  JxlEncoderOptions *encoder_options;
  JxlPixelFormat     pixel_format;
  JxlBasicInfo       output_info;
  JxlColorEncoding   color_profile;
  JxlEncoderStatus   status;
  size_t             buffer_size;

  GByteArray        *compressed;

  FILE              *outfile;
  GeglBuffer        *buffer;
  GimpImageType      drawable_type;

  gint               drawable_width;
  gint               drawable_height;
  gpointer           picture_buffer;

  GimpColorProfile  *profile = NULL;
  const Babl        *file_format = NULL;
  const Babl        *space = NULL;
  gboolean           out_linear = FALSE;

  size_t             offset = 0;
  uint8_t           *next_out;
  size_t             avail_out;

  gdouble            compression = 1.0;
  gboolean           lossless = FALSE;
  gint               speed = 7;
  gint               bit_depth = 8;
  gboolean           uses_original_profile = FALSE;

  gimp_progress_init_printf (_("Exporting '%s'"),
                             gimp_file_get_utf8_name (file));

  g_object_get (config,
                "lossless",              &lossless,
                "compression",           &compression,
                "speed",                 &speed,
                "save-bit-depth",        &bit_depth,
                "uses-original-profile", &uses_original_profile,
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

  drawable_type   = gimp_drawable_type (drawable);
  drawable_width  = gimp_drawable_get_width (drawable);
  drawable_height = gimp_drawable_get_height (drawable);

  JxlEncoderInitBasicInfo(&output_info);

  if (uses_original_profile)
    {
      output_info.uses_original_profile = JXL_TRUE;

      profile = gimp_image_get_effective_color_profile (image);
      out_linear = gimp_color_profile_is_linear (profile);

      space = gimp_color_profile_get_space (profile,
                                            GIMP_COLOR_RENDERING_INTENT_RELATIVE_COLORIMETRIC,
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
  output_info.intensity_target = 255.0f;
  output_info.orientation = JXL_ORIENT_IDENTITY;
  output_info.animation.tps_numerator = 10;
  output_info.animation.tps_denominator = 1;

  switch (drawable_type)
    {
    case GIMP_GRAYA_IMAGE:
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
    case GIMP_GRAY_IMAGE:
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
    case GIMP_RGBA_IMAGE:
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
    case GIMP_RGB_IMAGE:
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

  gimp_progress_update (0.3);

  buffer = gimp_drawable_get_buffer (drawable);
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

  runner = JxlThreadParallelRunnerCreate (NULL, gimp_get_num_processors ());
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

  encoder_options = JxlEncoderOptionsCreate (encoder, NULL);

  if (lossless)
    {
      JxlEncoderOptionsSetDistance (encoder_options, 0);
      JxlEncoderOptionsSetLossless (encoder_options, JXL_TRUE);
    }
  else
    {
      JxlEncoderOptionsSetDistance (encoder_options, compression);
      JxlEncoderOptionsSetLossless (encoder_options, JXL_FALSE);
    }

  status = JxlEncoderOptionsSetEffort (encoder_options, speed);
  if (status != JXL_ENC_SUCCESS)
    {
      g_printerr ("JxlEncoderOptionsSetEffort failed to set effort %d", speed);
    }

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

static gboolean
save_dialog (GimpImage     *image,
             GimpProcedure *procedure,
             GObject       *config)
{
  GtkWidget    *dialog;
  GtkListStore *store;
  gboolean      run;

  dialog = gimp_save_procedure_dialog_new (GIMP_SAVE_PROCEDURE (procedure),
                                           GIMP_PROCEDURE_CONFIG (config),
                                           image);

  gimp_procedure_dialog_get_widget (GIMP_PROCEDURE_DIALOG (dialog),
                                    "lossless", GTK_TYPE_CHECK_BUTTON);

  gimp_procedure_dialog_get_widget (GIMP_PROCEDURE_DIALOG (dialog),
                                    "compression", GIMP_TYPE_SCALE_ENTRY);

  store = gimp_int_store_new (_("falcon (faster)"),   3,
                              _("cheetah"),           4,
                              _("hare"),              5,
                              _("wombat"),            6,
                              _("squirrel"),          7,
                              _("kitten"),            8,
                              _("tortoise (slower)"), 9,
                              NULL);

  gimp_procedure_dialog_get_int_combo (GIMP_PROCEDURE_DIALOG (dialog),
                                       "speed", GIMP_INT_STORE (store));
  g_object_unref (store);

  store = gimp_int_store_new (_("8 bit/channel"),   8,
                              _("16 bit/channel"), 16,
                              NULL);

  gimp_procedure_dialog_get_int_combo (GIMP_PROCEDURE_DIALOG (dialog),
                                       "save-bit-depth", GIMP_INT_STORE (store));
  g_object_unref (store);

  gimp_procedure_dialog_get_widget (GIMP_PROCEDURE_DIALOG (dialog),
                                    "uses-original-profile", GTK_TYPE_CHECK_BUTTON);

  gimp_procedure_dialog_fill (GIMP_PROCEDURE_DIALOG (dialog),
                              "lossless", "compression",
                              "speed", "save-bit-depth",
                              "uses-original-profile",
                              NULL);

  run = gimp_procedure_dialog_run (GIMP_PROCEDURE_DIALOG (dialog));

  gtk_widget_destroy (dialog);

  return run;
}


static GimpValueArray *
jpegxl_save (GimpProcedure        *procedure,
             GimpRunMode           run_mode,
             GimpImage            *image,
             gint                  n_drawables,
             GimpDrawable        **drawables,
             GFile                *file,
             const GimpValueArray *args,
             gpointer              run_data)
{
  GimpPDBStatusType      status = GIMP_PDB_SUCCESS;
  GimpProcedureConfig   *config;
  GimpExportReturn       export = GIMP_EXPORT_CANCEL;
  GError                *error  = NULL;

  INIT_I18N ();
  gegl_init (NULL, NULL);

  config = gimp_procedure_create_config (procedure);
  gimp_procedure_config_begin_run (config, image, run_mode, args);

  switch (run_mode)
    {
    case GIMP_RUN_INTERACTIVE:
    case GIMP_RUN_WITH_LAST_VALS:
      gimp_ui_init (PLUG_IN_BINARY);

      export = gimp_export_image (&image, &n_drawables, &drawables, "JPEG XL",
                                  GIMP_EXPORT_CAN_HANDLE_RGB |
                                  GIMP_EXPORT_CAN_HANDLE_GRAY |
                                  GIMP_EXPORT_CAN_HANDLE_ALPHA);

      if (export == GIMP_EXPORT_CANCEL)
        {
          return gimp_procedure_new_return_values (procedure,
                 GIMP_PDB_CANCEL,
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

      return gimp_procedure_new_return_values (procedure,
             GIMP_PDB_CALLING_ERROR,
             error);
    }

  if (run_mode == GIMP_RUN_INTERACTIVE)
    {
      if (! save_dialog (image, procedure, G_OBJECT (config)))
        {
          status = GIMP_PDB_CANCEL;
        }
    }

  if (status == GIMP_PDB_SUCCESS)
    {
      if (! save_image (file, config,
                        image, drawables[0], &error))
        {
          status = GIMP_PDB_EXECUTION_ERROR;
        }
    }


  gimp_procedure_config_end_run (config, status);
  g_object_unref (config);

  if (export == GIMP_EXPORT_EXPORT)
    {
      g_free (drawables);
      gimp_image_delete (image);
    }

  return gimp_procedure_new_return_values (procedure, status, error);
}
