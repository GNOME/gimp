/*
 * GIMP HEIF loader / write plugin.
 * Copyright (c) 2018 struktur AG, Dirk Farin <farin@struktur.de>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
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

#include <libheif/heif.h>
#include <lcms2.h>
#include <gexiv2/gexiv2.h>

#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

#include "libgimp/stdplugins-intl.h"


#define LOAD_PROC      "file-heif-load"
#define SAVE_PROC      "file-heif-save"
#define SAVE_PROC_AV1  "file-heif-av1-save"
#define PLUG_IN_BINARY "file-heif"


typedef struct _SaveParams SaveParams;

struct _SaveParams
{
  gint     quality;
  gboolean lossless;
  gboolean save_profile;
  gint     save_bit_depth;
};


/*  local function prototypes  */

static void       init           (void);
static void       run            (const gchar          *name,
                                  gint                  nparams,
                                  const GimpParam      *param,
                                  gint                 *nreturn_vals,
                                  GimpParam           **return_vals);

static gint32     load_image     (GFile               *file,
                                  gboolean             interactive,
                                  GError              **error);
static gboolean   save_image     (GFile                        *file,
                                  gint32                        image_ID,
                                  gint32                        drawable_ID,
                                  const SaveParams             *params,
                                  GError                      **error,
                                  enum heif_compression_format  compression);

static gboolean   load_dialog    (struct heif_context  *heif,
                                  uint32_t             *selected_image);
static gboolean   save_dialog    (SaveParams           *params,
                                  gint32                image_ID,
                                  const gchar          *procname);


GimpPlugInInfo PLUG_IN_INFO =
{
  init,  /* init_proc  */
  NULL,  /* quit_proc  */
  NULL,  /* query_proc */
  run,   /* run_proc   */
};


MAIN ()


static void
init (void)
{
  static const GimpParamDef load_args[] =
  {
    { GIMP_PDB_INT32,  "run-mode", "The run mode { RUN-INTERACTIVE (0), RUN-NONINTERACTIVE (1) }" },
    { GIMP_PDB_STRING, "uri",      "The URI of the file to load" },
    { GIMP_PDB_STRING, "raw-uri",  "The URI of the file to load" }
  };

  static const GimpParamDef load_return_vals[] =
  {
    { GIMP_PDB_IMAGE, "image", "Output image" }
  };


  static const GimpParamDef save_args[] =
  {
    { GIMP_PDB_INT32,    "run-mode", "The run mode { RUN-INTERACTIVE (0), RUN-NONINTERACTIVE (1) }" },
    { GIMP_PDB_IMAGE,    "image",    "Input image" },
    { GIMP_PDB_DRAWABLE, "drawable", "Drawable to export" },
    { GIMP_PDB_STRING,   "uri",      "The URI of the file to export the image to" },
    { GIMP_PDB_STRING,   "raw-uri",  "The UTI of the file to export the image to" },
    { GIMP_PDB_INT32,    "quality",  "Quality factor (range: 0-100. 0 = worst, 100 = best)" },
    { GIMP_PDB_INT32,    "lossless", "Use lossless compression (0 = lossy, 1 = lossless)" }
  };

  if (heif_have_decoder_for_format (heif_compression_HEVC)
#if LIBHEIF_HAVE_VERSION(1,8,0)
      || heif_have_decoder_for_format (heif_compression_AV1)
#endif
     )
    {
      GString *extensions = g_string_new (NULL);
      GString *mimetypes = g_string_new (NULL);
      GString *magic = g_string_new ("4,string,ftypmif1");

      if (heif_have_decoder_for_format (heif_compression_HEVC))
        {
          g_string_append (extensions, "heic,heif");
          g_string_append (mimetypes, "image/heif");
          g_string_append (magic, ",4,string,ftypheic,4,string,ftypheix,"
                                  "4,string,ftyphevc,4,string,ftypheim,"
                                  "4,string,ftypheis,4,string,ftyphevm,"
                                  "4,string,ftyphevs,4,string,ftypmsf1");
        }

#if LIBHEIF_HAVE_VERSION(1,8,0)
      if (heif_have_decoder_for_format (heif_compression_AV1))
        {
          g_string_append_printf (extensions, "%savif", extensions->len ? "," : "");
          g_string_append_printf (mimetypes, "%simage/avif", mimetypes->len ? "," : "");
          g_string_append (magic, ",4,string,ftypavif");
        }
#endif

      gimp_install_procedure (LOAD_PROC,
                              _("Loads HEIF images"),
                              _("Load image stored in HEIF format (High "
                                "Efficiency Image File Format). Typical "
                                "suffices for HEIF files are .heif, .heic."),
                              "Dirk Farin <farin@struktur.de>",
                              "Dirk Farin <farin@struktur.de>",
                              "2018",
                              _("HEIF/HEIC"),
                              NULL,
                              GIMP_PLUGIN,
                              G_N_ELEMENTS (load_args),
                              G_N_ELEMENTS (load_return_vals),
                              load_args, load_return_vals);

      gimp_register_load_handler (LOAD_PROC, extensions->str, "");
      gimp_register_file_handler_mime (LOAD_PROC, mimetypes->str);
      gimp_register_file_handler_uri (LOAD_PROC);

      gimp_register_magic_load_handler (LOAD_PROC,
                                        extensions->str, "",
                                        magic->str);

      g_string_free (magic, TRUE);
      g_string_free (mimetypes, TRUE);
      g_string_free (extensions, TRUE);
    }

  if (heif_have_encoder_for_format (heif_compression_HEVC))
    {
      gimp_install_procedure (SAVE_PROC,
                              _("Exports HEIF images"),
                              _("Save image in HEIF format (High Efficiency "
                              "Image File Format)."),
                              "Dirk Farin <farin@struktur.de>",
                              "Dirk Farin <farin@struktur.de>",
                              "2018",
                              _("HEIF/HEIC"),
                              "RGB*",
                              GIMP_PLUGIN,
                              G_N_ELEMENTS (save_args), 0,
                              save_args, NULL);

      gimp_register_save_handler (SAVE_PROC, "heic,heif", "");
      gimp_register_file_handler_mime (SAVE_PROC, "image/heif");
      gimp_register_file_handler_uri (SAVE_PROC);
    }

#if LIBHEIF_HAVE_VERSION(1,8,0)
  if (heif_have_encoder_for_format (heif_compression_AV1))
    {
      gimp_install_procedure (SAVE_PROC_AV1,
                              _("Exports AVIF images"),
                              _("Save image in AV1 Image File Format (AVIF)"),
                              "Daniel Novomesky <dnovomesky@gmail.com>",
                              "Daniel Novomesky <dnovomesky@gmail.com>",
                              "2020",
                              "HEIF/AVIF",
                              "RGB*",
                              GIMP_PLUGIN,
                              G_N_ELEMENTS (save_args), 0,
                              save_args, NULL);

      gimp_register_save_handler (SAVE_PROC_AV1, "avif", "");
      gimp_register_file_handler_mime (SAVE_PROC_AV1, "image/avif");
      gimp_register_file_handler_uri (SAVE_PROC_AV1);
    }
#endif
}

#define LOAD_HEIF_CANCEL -2

static void
run (const gchar      *name,
     gint              n_params,
     const GimpParam  *param,
     gint             *nreturn_vals,
     GimpParam       **return_vals)
{
  static GimpParam   values[2];
  GimpRunMode        run_mode;
  GimpPDBStatusType  status = GIMP_PDB_SUCCESS;
  GError            *error  = NULL;

  INIT_I18N ();
  gegl_init (NULL, NULL);

  run_mode = param[0].data.d_int32;

  *nreturn_vals = 1;
  *return_vals  = values;

  values[0].type          = GIMP_PDB_STATUS;
  values[0].data.d_status = GIMP_PDB_EXECUTION_ERROR;

  if (run_mode == GIMP_RUN_INTERACTIVE)
    gimp_ui_init (PLUG_IN_BINARY, FALSE);

  if (strcmp (name, LOAD_PROC) == 0)
    {
      GFile    *file;
      gboolean  interactive;

      if (n_params != 3)
        status = GIMP_PDB_CALLING_ERROR;

      file        = g_file_new_for_uri (param[1].data.d_string);
      interactive = (run_mode == GIMP_RUN_INTERACTIVE);

      if (status == GIMP_PDB_SUCCESS)
        {
          gint32 image_ID;

          image_ID = load_image (file, interactive, &error);

          if (image_ID >= 0)
            {
              *nreturn_vals = 2;
              values[1].type         = GIMP_PDB_IMAGE;
              values[1].data.d_image = image_ID;
            }
          else if (image_ID == LOAD_HEIF_CANCEL)
            {
              status = GIMP_PDB_CANCEL;
            }
          else
            {
              status = GIMP_PDB_CALLING_ERROR;
            }
        }

      g_object_unref (file);
    }
  else if (strcmp (name, SAVE_PROC) == 0)
    {
      gint32                 image_ID    = param[1].data.d_int32;
      gint32                 drawable_ID = param[2].data.d_int32;
      GimpExportReturn       export      = GIMP_EXPORT_CANCEL;
      SaveParams             params;
      GimpMetadata          *metadata = NULL;
      GimpMetadataSaveFlags  metadata_flags;

      metadata = gimp_image_metadata_save_prepare (image_ID,
                                                   "image/heif",
                                                   &metadata_flags);

      params.lossless     = FALSE;
      params.quality      = 50;
      params.save_profile = (metadata_flags & GIMP_METADATA_SAVE_COLOR_PROFILE) != 0;
      params.save_bit_depth = 8;

      switch (run_mode)
        {
        case GIMP_RUN_INTERACTIVE:
        case GIMP_RUN_WITH_LAST_VALS:
          export = gimp_export_image (&image_ID, &drawable_ID, "HEIF",
                                      GIMP_EXPORT_CAN_HANDLE_RGB |
                                      GIMP_EXPORT_CAN_HANDLE_ALPHA);

          if (export == GIMP_EXPORT_CANCEL)
            {
              values[0].data.d_status = GIMP_PDB_CANCEL;
              g_clear_object (&metadata);
              return;
            }
          break;

        default:
          break;
        }

      switch (run_mode)
        {
        case GIMP_RUN_INTERACTIVE:
          gimp_get_data (SAVE_PROC, &params);

          if (! save_dialog (&params, image_ID, name))
            status = GIMP_PDB_CANCEL;
          break;

        case GIMP_RUN_WITH_LAST_VALS:
          gimp_get_data (SAVE_PROC, &params);
          break;

        case GIMP_RUN_NONINTERACTIVE:
          /*  Make sure all the arguments are there!  */
          if (n_params != 7)
            {
              status = GIMP_PDB_CALLING_ERROR;
            }
          else
            {
              params.quality  = (param[5].data.d_int32);
              params.lossless = (param[6].data.d_int32);
            }
          break;
        }

      if (status == GIMP_PDB_SUCCESS)
        {
          GFile *file = g_file_new_for_uri (param[3].data.d_string);

          if (save_image (file, image_ID, drawable_ID,
                          &params,
                          &error,
                          heif_compression_HEVC))
            {
              /* Exiv2 doesn't support HEIC format yet, following code doesn't work
              if (metadata)
                gimp_image_metadata_save_finish (image_ID,
                                                 "image/heif",
                                                 metadata, metadata_flags,
                                                 file, NULL);
              */
              gimp_set_data (SAVE_PROC, &params, sizeof (params));
            }
          else
            {
              status = GIMP_PDB_EXECUTION_ERROR;
            }
          g_object_unref (file);
        }

      g_clear_object (&metadata);
    }
#if LIBHEIF_HAVE_VERSION(1,8,0)
  else if (strcmp (name, SAVE_PROC_AV1) == 0)
    {
      gint32                 image_ID    = param[1].data.d_int32;
      gint32                 drawable_ID = param[2].data.d_int32;
      GimpExportReturn       export      = GIMP_EXPORT_CANCEL;
      SaveParams             params;
      GimpMetadata          *metadata = NULL;
      GimpMetadataSaveFlags  metadata_flags;

      metadata = gimp_image_metadata_save_prepare (image_ID,
                                                   "image/avif",
                                                   &metadata_flags);

      params.lossless     = FALSE;
      params.quality      = 50;
      params.save_profile = (metadata_flags & GIMP_METADATA_SAVE_COLOR_PROFILE) != 0;
      params.save_bit_depth = 8;

      switch (run_mode)
        {
        case GIMP_RUN_INTERACTIVE:
        case GIMP_RUN_WITH_LAST_VALS:
          export = gimp_export_image (&image_ID, &drawable_ID, "AVIF",
                                      GIMP_EXPORT_CAN_HANDLE_RGB |
                                      GIMP_EXPORT_CAN_HANDLE_ALPHA);

          if (export == GIMP_EXPORT_CANCEL)
            {
              values[0].data.d_status = GIMP_PDB_CANCEL;
              g_clear_object (&metadata);
              return;
            }
          break;

        default:
          break;
        }

      switch (run_mode)
        {
        case GIMP_RUN_INTERACTIVE:
          gimp_get_data (SAVE_PROC_AV1, &params);

          if (! save_dialog (&params, image_ID, name))
            status = GIMP_PDB_CANCEL;
          break;

        case GIMP_RUN_WITH_LAST_VALS:
          gimp_get_data (SAVE_PROC_AV1, &params);
          break;

        case GIMP_RUN_NONINTERACTIVE:
          /*  Make sure all the arguments are there!  */
          if (n_params != 7)
            {
              status = GIMP_PDB_CALLING_ERROR;
            }
          else
            {
              params.quality  = (param[5].data.d_int32);
              params.lossless = (param[6].data.d_int32);
            }
          break;
        }

      if (status == GIMP_PDB_SUCCESS)
        {
          GFile *file = g_file_new_for_uri (param[3].data.d_string);

          if (save_image (file, image_ID, drawable_ID,
                          &params,
                          &error,
                          heif_compression_AV1))
            {
              gimp_set_data (SAVE_PROC_AV1, &params, sizeof (params));
            }
          else
            {
              status = GIMP_PDB_EXECUTION_ERROR;
            }
          g_object_unref (file);
        }

      g_clear_object (&metadata);
    }
#endif
  else
    {
      status = GIMP_PDB_CALLING_ERROR;
    }

  if (status != GIMP_PDB_SUCCESS && error)
    {
      *nreturn_vals = 2;
      values[1].type          = GIMP_PDB_STRING;
      values[1].data.d_string = error->message;
    }

  values[0].data.d_status = status;
}

static goffset
get_file_size (GFile   *file,
               GError **error)
{
  GFileInfo *info;
  goffset    size = 1;

  info = g_file_query_info (file,
                            G_FILE_ATTRIBUTE_STANDARD_SIZE,
                            G_FILE_QUERY_INFO_NONE,
                            NULL, error);
  if (info)
    {
      size = g_file_info_get_size (info);

      g_object_unref (info);
    }

  return size;
}

#if LIBHEIF_HAVE_VERSION(1,8,0)
static void
heifplugin_color_profile_set_tag (cmsHPROFILE      profile,
                                  cmsTagSignature  sig,
                                  const gchar     *tag)
{
  cmsMLU *mlu;

  mlu = cmsMLUalloc (NULL, 1);
  cmsMLUsetASCII (mlu, "en", "US", tag);
  cmsWriteTag (profile, sig, mlu);
  cmsMLUfree (mlu);
}

static GimpColorProfile *
nclx_to_gimp_profile (const struct heif_color_profile_nclx *nclx)
{
  const gchar *primaries_name = "";
  const gchar *trc_name = "";
  cmsHPROFILE profile = NULL;
  cmsCIExyY whitepoint;
  cmsCIExyYTRIPLE primaries;
  cmsToneCurve *curve[3];

  cmsFloat64Number srgb_parameters[5] =
  { 2.4, 1.0 / 1.055,  0.055 / 1.055, 1.0 / 12.92, 0.04045 };

  cmsFloat64Number rec709_parameters[5] =
  { 2.2, 1.0 / 1.099,  0.099 / 1.099, 1.0 / 4.5, 0.081 };

  if (nclx == NULL)
    {
      return NULL;
    }

  if (nclx->color_primaries == heif_color_primaries_unspecified)
    {
      return NULL;
    }

  if (nclx->color_primaries == heif_color_primaries_ITU_R_BT_709_5)
    {
      if (nclx->transfer_characteristics == heif_transfer_characteristic_IEC_61966_2_1)
        {
          return gimp_color_profile_new_rgb_srgb();
        }

      if (nclx->transfer_characteristics == heif_transfer_characteristic_linear)
        {
          return gimp_color_profile_new_rgb_srgb_linear();
        }
    }

  whitepoint.x = nclx->color_primary_white_x;
  whitepoint.y = nclx->color_primary_white_y;
  whitepoint.Y = 1.0f;

  primaries.Red.x = nclx->color_primary_red_x;
  primaries.Red.y = nclx->color_primary_red_y;
  primaries.Red.Y = 1.0f;

  primaries.Green.x = nclx->color_primary_green_x;
  primaries.Green.y = nclx->color_primary_green_y;
  primaries.Green.Y = 1.0f;

  primaries.Blue.x = nclx->color_primary_blue_x;
  primaries.Blue.y = nclx->color_primary_blue_y;
  primaries.Blue.Y = 1.0f;

  switch (nclx->color_primaries)
    {
    case heif_color_primaries_ITU_R_BT_709_5:
      primaries_name = "BT.709";
      break;
    case   heif_color_primaries_ITU_R_BT_470_6_System_M:
      primaries_name = "BT.470-6 System M";
      break;
    case heif_color_primaries_ITU_R_BT_470_6_System_B_G:
      primaries_name = "BT.470-6 System BG";
      break;
    case heif_color_primaries_ITU_R_BT_601_6:
      primaries_name = "BT.601";
      break;
    case heif_color_primaries_SMPTE_240M:
      primaries_name = "SMPTE 240M";
      break;
    case 8:
      primaries_name = "Generic film";
      break;
    case 9:
      primaries_name = "BT.2020";
      break;
    case 10:
      primaries_name = "XYZ";
      break;
    case 11:
      primaries_name = "SMPTE RP 431-2";
      break;
    case 12:
      primaries_name = "SMPTE EG 432-1 (DCI P3)";
      break;
    case 22:
      primaries_name = "EBU Tech. 3213-E";
      break;
    default:
      g_warning ("%s: Unsupported color_primaries value %d.",
                 G_STRFUNC, nclx->color_primaries);
      return NULL;
      break;
    }

  switch (nclx->transfer_characteristics)
    {
    case heif_transfer_characteristic_ITU_R_BT_709_5:
      curve[0] = curve[1] = curve[2] = cmsBuildParametricToneCurve (NULL, 4,
                                                                    rec709_parameters);
      profile = cmsCreateRGBProfile (&whitepoint, &primaries, curve);
      cmsFreeToneCurve (curve[0]);
      trc_name = "Rec709 RGB";
      break;
    case heif_transfer_characteristic_ITU_R_BT_470_6_System_M:
      curve[0] = curve[1] = curve[2] = cmsBuildGamma (NULL, 2.2f);
      profile = cmsCreateRGBProfile (&whitepoint, &primaries, curve);
      cmsFreeToneCurve (curve[0]);
      trc_name = "Gamma2.2 RGB";
      break;
    case heif_transfer_characteristic_ITU_R_BT_470_6_System_B_G:
      curve[0] = curve[1] = curve[2] = cmsBuildGamma (NULL, 2.8f);
      profile = cmsCreateRGBProfile (&whitepoint, &primaries, curve);
      cmsFreeToneCurve (curve[0]);
      trc_name = "Gamma2.8 RGB";
      break;
    case heif_transfer_characteristic_linear:
      curve[0] = curve[1] = curve[2] = cmsBuildGamma (NULL, 1.0f);
      profile = cmsCreateRGBProfile (&whitepoint, &primaries, curve);
      cmsFreeToneCurve (curve[0]);
      trc_name = "linear RGB";
      break;
    case heif_transfer_characteristic_IEC_61966_2_1:
      /* same as default */
    default:
      curve[0] = curve[1] = curve[2] = cmsBuildParametricToneCurve (NULL, 4,
                                                                    srgb_parameters);
      profile = cmsCreateRGBProfile (&whitepoint, &primaries, curve);
      cmsFreeToneCurve (curve[0]);
      trc_name = "sRGB-TRC RGB";
      break;
    }

  if (profile)
    {
      GimpColorProfile *new_profile;
      gchar *description = g_strdup_printf ("%s %s", primaries_name, trc_name);

      heifplugin_color_profile_set_tag (profile, cmsSigProfileDescriptionTag,
                                        description);
      heifplugin_color_profile_set_tag (profile, cmsSigDeviceMfgDescTag,
                                        "GIMP");
      heifplugin_color_profile_set_tag (profile, cmsSigDeviceModelDescTag,
                                        description);
      heifplugin_color_profile_set_tag (profile, cmsSigCopyrightTag,
                                        "Public Domain");

      new_profile = gimp_color_profile_new_from_lcms_profile (profile, NULL);

      cmsCloseProfile (profile);
      g_free (description);
      return new_profile;
  }

  return NULL;
}
#endif

gint32
load_image (GFile     *file,
            gboolean   interactive,
            GError   **error)
{
  GInputStream             *input;
  goffset                   file_size;
  guchar                   *file_buffer;
  gsize                     bytes_read;
  struct heif_context      *ctx;
  struct heif_error         err;
  struct heif_image_handle *handle  = NULL;
  struct heif_image        *img     = NULL;
  GimpColorProfile         *profile = NULL;
  gint                      n_images;
  heif_item_id              primary;
  heif_item_id              selected_image;
  gboolean                  has_alpha;
  gint                      width;
  gint                      height;
  gint32                    image_ID;
  gint32                    layer_ID;
  GeglBuffer               *buffer;
  const Babl               *format;
  const guint8             *data;
  gint                      stride;
  gint                      bit_depth = 8;
  enum heif_chroma          chroma    = heif_chroma_interleaved_RGB;
  GimpPrecision             precision;
  gboolean                  load_linear;
  const char               *encoding;
  char                     *filename = g_file_get_parse_name (file);

  gimp_progress_init_printf (_("Opening '%s'"),
                             filename);
  g_free (filename);

  file_size = get_file_size (file, error);
  if (file_size <= 0)
    return -1;

  input = G_INPUT_STREAM (g_file_read (file, NULL, error));
  if (! input)
    return -1;

  file_buffer = g_malloc (file_size);

  if (! g_input_stream_read_all (input, file_buffer, file_size,
                                 &bytes_read, NULL, error) &&
      bytes_read == 0)
    {
      g_free (file_buffer);
      g_object_unref (input);
      return -1;
    }

  gimp_progress_update (0.25);

  ctx = heif_context_alloc ();
  if (!ctx)
    {
      g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
                   "cannot allocate heif_context");
      g_free (file_buffer);
      g_object_unref (input);
      return -1;
    }

  err = heif_context_read_from_memory (ctx, file_buffer, file_size, NULL);
  if (err.code)
    {
      g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
                   _("Loading HEIF image failed: %s"),
                   err.message);
      heif_context_free (ctx);
      g_free (file_buffer);
      g_object_unref (input);

      return -1;
    }

  g_free (file_buffer);
  g_object_unref (input);

  gimp_progress_update (0.5);

  /* analyze image content
   * Is there more than one image? Which image is the primary image?
   */

  n_images = heif_context_get_number_of_top_level_images (ctx);
  if (n_images == 0)
    {
      g_set_error_literal (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
                           _("Loading HEIF image failed: "
                             "Input file contains no readable images"));
      heif_context_free (ctx);

      return -1;
    }

  err = heif_context_get_primary_image_ID (ctx, &primary);
  if (err.code)
    {
      g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
                   _("Loading HEIF image failed: %s"),
                   err.message);
      heif_context_free (ctx);

      return -1;
    }

  /* if primary image is no top level image or not present (invalid
   * file), just take the first image
   */

  if (! heif_context_is_top_level_image_ID (ctx, primary))
    {
      gint n = heif_context_get_list_of_top_level_image_IDs (ctx, &primary, 1);
      g_assert (n == 1);
    }

  selected_image = primary;

  /* if there are several images in the file and we are running
   * interactive, let the user choose a picture
   */

  if (interactive && n_images > 1)
    {
      if (! load_dialog (ctx, &selected_image))
        {
          heif_context_free (ctx);

          return LOAD_HEIF_CANCEL;
        }
    }

  /* load the picture */

  err = heif_context_get_image_handle (ctx, selected_image, &handle);
  if (err.code)
    {
      g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
                   _("Loading HEIF image failed: %s"),
                   err.message);
      heif_context_free (ctx);

      return -1;
    }

  has_alpha = heif_image_handle_has_alpha_channel (handle);

#if LIBHEIF_HAVE_VERSION(1,8,0)
  bit_depth = heif_image_handle_get_luma_bits_per_pixel (handle);
  if (bit_depth < 0)
    {
      g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
                   "Input image has undefined bit-depth");
      heif_image_handle_release (handle);
      heif_context_free (ctx);

      return -1;
    }
#endif

  if (bit_depth == 8)
    {
      if (has_alpha)
        {
          chroma = heif_chroma_interleaved_RGBA;
        }
      else
        {
          chroma = heif_chroma_interleaved_RGB;
        }
    }
  else /* high bit depth */
    {
#if LIBHEIF_HAVE_VERSION(1,8,0)
#if ( G_BYTE_ORDER == G_LITTLE_ENDIAN )
      if (has_alpha)
        {
          chroma = heif_chroma_interleaved_RRGGBBAA_LE;
        }
      else
        {
          chroma = heif_chroma_interleaved_RRGGBB_LE;
        }
#else
      if (has_alpha)
        {
          chroma = heif_chroma_interleaved_RRGGBBAA_BE;
        }
      else
        {
          chroma = heif_chroma_interleaved_RRGGBB_BE;
        }
#endif
#endif
    }

  err = heif_decode_image (handle,
                           &img,
                           heif_colorspace_RGB,
                           chroma,
                           NULL);
  if (err.code)
    {
      g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
                   _("Loading HEIF image failed: %s"),
                   err.message);
      heif_image_handle_release (handle);
      heif_context_free (ctx);

      return -1;
    }

#if LIBHEIF_HAVE_VERSION(1,4,0)
  switch (heif_image_handle_get_color_profile_type (handle))
    {
    case heif_color_profile_type_not_present:
      break;
    case heif_color_profile_type_rICC:
    case heif_color_profile_type_prof:
      /* I am unsure, but it looks like both these types represent an
       * ICC color profile. XXX
       */
        {
          void   *profile_data;
          size_t  profile_size;

          profile_size = heif_image_handle_get_raw_color_profile_size (handle);
          profile_data = g_malloc0 (profile_size);
          err = heif_image_handle_get_raw_color_profile (handle, profile_data);

          if (err.code)
            g_warning ("%s: color profile loading failed and discarded.",
                       G_STRFUNC);
          else
            profile = gimp_color_profile_new_from_icc_profile ((guint8 *) profile_data,
                                                               profile_size, NULL);

          g_free (profile_data);
        }
      break;
#if LIBHEIF_HAVE_VERSION(1,8,0)
    case heif_color_profile_type_nclx:
      {
        struct heif_color_profile_nclx *nclx = NULL;

        err = heif_image_handle_get_nclx_color_profile (handle, &nclx);
        if (err.code)
          {
            g_warning ("%s: NCLX profile loading failed and discarded.",
                       G_STRFUNC);
          }
        else
          {
            profile = nclx_to_gimp_profile (nclx);
            heif_nclx_color_profile_free (nclx);
          }
      }
      break;
#endif
    default:
      g_warning ("%s: unknown color profile type has been discarded.",
                 G_STRFUNC);
      break;
    }
#endif /* LIBHEIF_HAVE_VERSION(1,4,0) */

  gimp_progress_update (0.75);

  width  = heif_image_get_width  (img, heif_channel_interleaved);
  height = heif_image_get_height (img, heif_channel_interleaved);

  /* create GIMP image and copy HEIF image into the GIMP image
   * (converting it to RGB)
   */

  if (profile)
    {
      load_linear = gimp_color_profile_is_linear (profile);
    }
  else
    {
      load_linear = FALSE;
    }

  if (load_linear)
    {
      if (bit_depth == 8)
        {
          precision = GIMP_PRECISION_U8_LINEAR;
          encoding = has_alpha ? "RGBA u8" : "RGB u8";
        }
      else
        {
          precision = GIMP_PRECISION_U16_LINEAR;
          encoding = has_alpha ? "RGBA u16" : "RGB u16";
        }
    }
  else /* non-linear profiles */
    {
      if (bit_depth == 8)
        {
          precision = GIMP_PRECISION_U8_GAMMA;
          encoding = has_alpha ? "R'G'B'A u8" : "R'G'B' u8";
        }
      else
        {
          precision = GIMP_PRECISION_U16_GAMMA;
          encoding = has_alpha ? "R'G'B'A u16" : "R'G'B' u16";
        }
    }

  image_ID = gimp_image_new_with_precision (width, height, GIMP_RGB, precision);
  gimp_image_set_filename (image_ID, g_file_get_uri (file));

  if (profile)
    {
      if (gimp_color_profile_is_rgb (profile))
        {
          gimp_image_set_color_profile (image_ID, profile);
        }
      else if (gimp_color_profile_is_gray (profile))
        {
          g_warning ("Gray ICC profile was not applied to the imported image.");
        }
      else
        {
          g_warning ("ICC profile was not applied to the imported image.");
        }
    }

  layer_ID = gimp_layer_new (image_ID,
                             _("image content"),
                             width, height,
                             has_alpha ? GIMP_RGBA_IMAGE : GIMP_RGB_IMAGE,
                             100.0,
                             gimp_image_get_default_new_layer_mode (image_ID));

  gimp_image_insert_layer (image_ID, layer_ID, -1, 0);

  buffer = gimp_drawable_get_buffer (layer_ID);

  data = heif_image_get_plane_readonly (img, heif_channel_interleaved,
                                        &stride);

  format = babl_format_with_space (encoding,
                                   gegl_buffer_get_format (buffer));

  if (bit_depth == 8)
    {
      gegl_buffer_set (buffer,
                       GEGL_RECTANGLE (0, 0, width, height),
                       0, format, data, stride);
    }
  else /* high bit depth */
    {
      uint16_t       *data16;
      const uint16_t *src16;
      uint16_t       *dest16;
      gint            x, y, rowentries;
      int             tmp_pixelval;

      if (has_alpha)
        {
          rowentries = width * 4;
        }
      else /* no alpha */
        {
          rowentries = width * 3;
        }

      data16 = g_malloc_n (height, rowentries * 2);
      dest16 = data16;

      switch (bit_depth)
        {
        case 10:
          for (y = 0; y < height; y++)
            {
              src16 = (const uint16_t *) (y * stride + data);
              for (x = 0; x < rowentries; x++)
                {
                  tmp_pixelval = (int) ( ( (float) (0x03ff & (*src16)) / 1023.0f) * 65535.0f + 0.5f);
                  *dest16 = CLAMP (tmp_pixelval, 0, 65535);
                  dest16++;
                  src16++;
                }
            }
          break;
        case 12:
          for (y = 0; y < height; y++)
            {
              src16 = (const uint16_t *) (y * stride + data);
              for (x = 0; x < rowentries; x++)
                {
                  tmp_pixelval = (int) ( ( (float) (0x0fff & (*src16))  / 4095.0f) * 65535.0f + 0.5f);
                  *dest16 = CLAMP (tmp_pixelval, 0, 65535);
                  dest16++;
                  src16++;
                }
            }
          break;
        default:
          for (y = 0; y < height; y++)
            {
              src16 = (const uint16_t *) (y * stride + data);
              for (x = 0; x < rowentries; x++)
                {
                  *dest16 = *src16;
                  dest16++;
                  src16++;
                }
            }
          break;
        }

      gegl_buffer_set (buffer,
                       GEGL_RECTANGLE (0, 0, width, height),
                       0, format, data16, GEGL_AUTO_ROWSTRIDE);

      g_free (data16);
    }

  g_object_unref (buffer);

  {
    size_t        exif_data_size = 0;
    uint8_t      *exif_data      = NULL;
    size_t        xmp_data_size  = 0;
    uint8_t      *xmp_data       = NULL;
    gint          n_metadata;
    heif_item_id  metadata_id;

    n_metadata =
      heif_image_handle_get_list_of_metadata_block_IDs (handle,
                                                        "Exif",
                                                        &metadata_id, 1);
    if (n_metadata > 0)
      {
        exif_data_size = heif_image_handle_get_metadata_size (handle,
                                                              metadata_id);
        exif_data = g_alloca (exif_data_size);

        err = heif_image_handle_get_metadata (handle, metadata_id, exif_data);
        if (err.code != 0)
          {
            exif_data      = NULL;
            exif_data_size = 0;
          }
      }

    n_metadata =
      heif_image_handle_get_list_of_metadata_block_IDs (handle,
                                                        "mime",
                                                        &metadata_id, 1);
    if (n_metadata > 0)
      {
        if (g_strcmp0 (
              heif_image_handle_get_metadata_content_type (handle, metadata_id),
              "application/rdf+xml") == 0)
          {
            xmp_data_size = heif_image_handle_get_metadata_size (handle,
                            metadata_id);
            xmp_data = g_alloca (xmp_data_size);

            err = heif_image_handle_get_metadata (handle, metadata_id, xmp_data);
            if (err.code != 0)
              {
                xmp_data      = NULL;
                xmp_data_size = 0;
              }
          }
      }

    if (exif_data || xmp_data)
      {
        GimpMetadata          *metadata = gimp_metadata_new ();
        GimpMetadataLoadFlags  flags    = GIMP_METADATA_LOAD_COMMENT | GIMP_METADATA_LOAD_RESOLUTION;

        if (exif_data)
          {
            const guint8 tiffHeaderBE[4] = { 'M', 'M', 0, 42 };
            const guint8 tiffHeaderLE[4] = { 'I', 'I', 42, 0 };
            GExiv2Metadata *exif_metadata = GEXIV2_METADATA (metadata);
            const guint8 *tiffheader = exif_data;
            glong new_exif_size = exif_data_size;

            while (new_exif_size >= 4)  /*Searching for TIFF Header*/
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

            if (new_exif_size > 4)   /* TIFF header + some data found*/
              {
                if (! gexiv2_metadata_open_buf (exif_metadata, tiffheader, new_exif_size, error))
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


        if (xmp_data)
          {
            if (!gimp_metadata_set_from_xmp (metadata, xmp_data, xmp_data_size, error))
              {
                g_printerr ("%s: Failed to set XMP metadata: %s\n", G_STRFUNC, (*error)->message);
                g_clear_error (error);
              }
          }

        gexiv2_metadata_set_orientation (GEXIV2_METADATA (metadata),
                                         GEXIV2_ORIENTATION_NORMAL);
        gexiv2_metadata_set_metadata_pixel_width (GEXIV2_METADATA (metadata),
                                                  width);
        gexiv2_metadata_set_metadata_pixel_height (GEXIV2_METADATA (metadata),
                                                   height);
        gimp_image_metadata_load_finish (image_ID, "image/heif",
                                         metadata, flags,
                                         interactive);
      }
  }

  if (profile)
    g_object_unref (profile);

  heif_image_handle_release (handle);
  heif_context_free (ctx);
  heif_image_release (img);

  gimp_progress_update (1.0);

  return image_ID;
}

static struct heif_error
write_callback (struct heif_context *ctx,
                const void          *data,
                size_t               size,
                void                *userdata)
{
  GOutputStream     *output = userdata;
  GError            *error  = NULL;
  struct heif_error  heif_error;

  heif_error.code    = heif_error_Ok;
  heif_error.subcode = heif_suberror_Unspecified;
  heif_error.message = "";

  if (! g_output_stream_write_all (output, data, size, NULL, NULL, &error))
    {
      heif_error.code    = 99; /* hmm */
      heif_error.message = error->message;
    }

  return heif_error;
}

static gboolean
save_image (GFile                        *file,
            gint32                        image_ID,
            gint32                        drawable_ID,
            const SaveParams             *params,
            GError                      **error,
            enum heif_compression_format  compression)
{
  struct heif_image                    *image   = NULL;
  struct heif_context                  *context = heif_context_alloc ();
  struct heif_encoder                  *encoder = NULL;
  const struct heif_encoder_descriptor *encoder_descriptor;
  const char                           *encoder_name;
  struct heif_image_handle             *handle  = NULL;
  struct heif_writer                    writer;
  struct heif_error                     err;
  GOutputStream                        *output;
  GeglBuffer                           *buffer;
  const gchar                          *encoding;
  const Babl                           *format;
  const Babl                           *space   = NULL;
  guint8                               *data;
  gint                                  stride;
  gint                                  width;
  gint                                  height;
  gboolean                              has_alpha;
  gboolean                              out_linear = FALSE;
  char                                 *filename;

  if (!context)
    {
      g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
                   "cannot allocate heif_context");
      return FALSE;
    }

  if (compression == heif_compression_HEVC)
    {
      if (heif_context_get_encoder_descriptors (context,
                                                heif_compression_HEVC,
                                                NULL,
                                                &encoder_descriptor, 1) == 1)
        {
          encoder_name = heif_encoder_descriptor_get_id_name (encoder_descriptor);
        }
      else
        {
          g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
                       "Unable to find suitable HEIF encoder");
          heif_context_free (context);
          return FALSE;
        }
    }
  else /* AV1 compression */
    {
      if (heif_context_get_encoder_descriptors (context,
                                                compression,
                                                "aom", /* we prefer aom rather than rav1e */
                                                &encoder_descriptor, 1) == 1)
        {
          encoder_name = heif_encoder_descriptor_get_id_name (encoder_descriptor);
        }
      else if (heif_context_get_encoder_descriptors (context,
                                                     compression,
                                                     NULL,
                                                     &encoder_descriptor, 1) == 1)
        {
          encoder_name = heif_encoder_descriptor_get_id_name (encoder_descriptor);
        }
      else
        {
          g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
                       "Unable to find suitable AVIF encoder");
          heif_context_free (context);
          return FALSE;
        }
    }

  filename = g_file_get_parse_name (file);
  gimp_progress_init_printf (_("Exporting '%s' using %s encoder"),
                             filename, encoder_name);
  g_free (filename);

  width   = gimp_drawable_width  (drawable_ID);
  height  = gimp_drawable_height (drawable_ID);

  has_alpha = gimp_drawable_has_alpha (drawable_ID);

  switch (params->save_bit_depth)
    {
    case 8:
      err = heif_image_create (width, height,
                               heif_colorspace_RGB,
                               has_alpha ?
                               heif_chroma_interleaved_RGBA :
                               heif_chroma_interleaved_RGB,
                               &image);
      break;
#if LIBHEIF_HAVE_VERSION(1,8,0)
    case 10:
    case 12:
#if ( G_BYTE_ORDER == G_LITTLE_ENDIAN )
      err = heif_image_create (width, height,
                               heif_colorspace_RGB,
                               has_alpha ?
                               heif_chroma_interleaved_RRGGBBAA_LE :
                               heif_chroma_interleaved_RRGGBB_LE,
                               &image);
#else
      err = heif_image_create (width, height,
                               heif_colorspace_RGB,
                               has_alpha ?
                               heif_chroma_interleaved_RRGGBBAA_BE :
                               heif_chroma_interleaved_RRGGBB_BE,
                               &image);
#endif
      break;
#endif
    default:
      g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
                   "Unsupported bit depth: %d",
                   params->save_bit_depth);
      heif_context_free (context);
      return FALSE;
      break;
    }

  if (err.code != 0)
    {
      g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
                   _("Encoding HEIF image failed: %s"),
                   err.message);
      heif_context_free (context);
      return FALSE;
    }

#if LIBHEIF_HAVE_VERSION(1,4,0)
  if (params->save_profile)
    {
      GimpColorProfile *profile = NULL;
      const guint8     *icc_data;
      gsize             icc_length;

      profile = gimp_image_get_color_profile (image_ID);
      if (profile && gimp_color_profile_is_linear (profile))
        out_linear = TRUE;

      if (! profile)
        {
          profile = gimp_image_get_effective_color_profile (image_ID);

          if (gimp_color_profile_is_linear (profile))
            {
              if (gimp_image_get_precision (image_ID) != GIMP_PRECISION_U8_LINEAR)
                {
                  /* If stored data was linear, let's convert the profile. */
                  GimpColorProfile *saved_profile;

                  saved_profile = gimp_color_profile_new_srgb_trc_from_color_profile (profile);
                  g_clear_object (&profile);
                  profile = saved_profile;
                }
              else
                {
                  /* Keep linear profile as-is for 8-bit linear image. */
                  out_linear = TRUE;
                }
            }
        }

      icc_data = gimp_color_profile_get_icc_profile (profile, &icc_length);
      heif_image_set_raw_color_profile (image, "prof", icc_data, icc_length);
      space = gimp_color_profile_get_space (profile,
                                            GIMP_COLOR_RENDERING_INTENT_RELATIVE_COLORIMETRIC,
                                            error);
      if (error && *error)
        {
          /* Don't make this a hard failure yet output the error. */
          g_printerr ("%s: error getting the profile space: %s",
                      G_STRFUNC, (*error)->message);
          g_clear_error (error);
        }

      g_object_unref (profile);
    }
  else
    {
#if LIBHEIF_HAVE_VERSION(1,8,0)
      /* We save as sRGB */
      struct heif_color_profile_nclx nclx_profile;

      nclx_profile.version = 1;
      nclx_profile.color_primaries = heif_color_primaries_ITU_R_BT_709_5;
      nclx_profile.transfer_characteristics = heif_transfer_characteristic_IEC_61966_2_1;
      nclx_profile.matrix_coefficients = heif_matrix_coefficients_ITU_R_BT_601_6;
      nclx_profile.full_range_flag = 1;

      heif_image_set_nclx_color_profile (image, &nclx_profile);

      space = babl_space ("sRGB");
      out_linear = FALSE;
#endif
    }
#endif /* LIBHEIF_HAVE_VERSION(1,4,0) */

  if (! space)
    space = gimp_drawable_get_format (drawable_ID);

  if (params->save_bit_depth > 8)
    {
      uint16_t       *data16;
      const uint16_t *src16;
      uint16_t       *dest16;
      gint            x, y, rowentries;
      int             tmp_pixelval;

      if (has_alpha)
        {
          rowentries = width * 4;

          if (out_linear)
            encoding = "RGBA u16";
          else
            encoding = "R'G'B'A u16";
        }
      else /* no alpha */
        {
          rowentries = width * 3;

          if (out_linear)
            encoding = "RGB u16";
          else
            encoding = "R'G'B' u16";
        }

      data16 = g_malloc_n (height, rowentries * 2);
      src16 = data16;

      format = babl_format_with_space (encoding, space);

      buffer = gimp_drawable_get_buffer (drawable_ID);

      gegl_buffer_get (buffer,
                       GEGL_RECTANGLE (0, 0, width, height),
                       1.0, format, data16, GEGL_AUTO_ROWSTRIDE, GEGL_ABYSS_NONE);

      g_object_unref (buffer);

      heif_image_add_plane (image, heif_channel_interleaved,
                            width, height, params->save_bit_depth);

      data = heif_image_get_plane (image, heif_channel_interleaved, &stride);

      switch (params->save_bit_depth)
        {
        case 10:
          for (y = 0; y < height; y++)
            {
              dest16 = (uint16_t *) (y * stride + data);
              for (x = 0; x < rowentries; x++)
                {
                  tmp_pixelval = (int) ( ( (float) (*src16) / 65535.0f) * 1023.0f + 0.5f);
                  *dest16 = CLAMP (tmp_pixelval, 0, 1023);
                  dest16++;
                  src16++;
                }
            }
          break;
        case 12:
          for (y = 0; y < height; y++)
            {
              dest16 = (uint16_t *) (y * stride + data);
              for (x = 0; x < rowentries; x++)
                {
                  tmp_pixelval = (int) ( ( (float) (*src16) / 65535.0f) * 4095.0f + 0.5f);
                  *dest16 = CLAMP (tmp_pixelval, 0, 4095);
                  dest16++;
                  src16++;
                }
            }
          break;
        default:
          for (y = 0; y < height; y++)
            {
              dest16 = (uint16_t *) (y * stride + data);
              for (x = 0; x < rowentries; x++)
                {
                  *dest16 = *src16;
                  dest16++;
                  src16++;
                }
            }
          break;
        }
      g_free (data16);
    }
  else /* save_bit_depth == 8 */
    {
#if LIBHEIF_HAVE_VERSION(1,8,0)
      heif_image_add_plane (image, heif_channel_interleaved,
                            width, height, 8);
#else
      /* old style settings */
      heif_image_add_plane (image, heif_channel_interleaved,
                            width, height, has_alpha ? 32 : 24);
#endif

      data = heif_image_get_plane (image, heif_channel_interleaved, &stride);

      buffer = gimp_drawable_get_buffer (drawable_ID);

      if (has_alpha)
        {
          if (out_linear)
            encoding = "RGBA u8";
          else
            encoding = "R'G'B'A u8";
        }
      else
        {
          if (out_linear)
            encoding = "RGB u8";
          else
            encoding = "R'G'B' u8";
        }
      format = babl_format_with_space (encoding, space);

      gegl_buffer_get (buffer,
                       GEGL_RECTANGLE (0, 0, width, height),
                       1.0, format, data, stride, GEGL_ABYSS_NONE);

      g_object_unref (buffer);
    }

  gimp_progress_update (0.33);

  /*  encode to HEIF file  */

  err = heif_context_get_encoder (context,
                                  encoder_descriptor,
                                  &encoder);

  if (err.code != 0)
    {
      g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
                   "Unable to get an encoder instance");
      heif_image_release (image);
      heif_context_free (context);
      return FALSE;
    }

  /* workaround for a bug in libheif when heif_encoder_set_lossless is not working
     (known problem with encoding via rav1e) */
  if (params->lossless)
    {
      heif_encoder_set_lossy_quality (encoder, 100);
    }
  else
    {
      heif_encoder_set_lossy_quality (encoder, params->quality);
    }

  heif_encoder_set_lossless (encoder, params->lossless);
  /* heif_encoder_set_logging_level (encoder, logging_level); */

#if LIBHEIF_HAVE_VERSION(1,10,0)
  if (params->lossless)
    {
      err = heif_encoder_set_parameter_string (encoder, "chroma", "444");
      if (err.code != 0)
        {
          g_printerr ("Failed to set chroma=444 for %s encoder: %s", encoder_name, err.message);
        }
    }
#endif

  err = heif_context_encode_image (context,
                                   image,
                                   encoder,
                                   NULL,
                                   &handle);
  if (err.code != 0)
    {
      g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
                   _("Encoding HEIF image failed: %s"),
                   err.message);
      heif_encoder_release (encoder);
      heif_image_release (image);
      heif_context_free (context);
      return FALSE;
    }

  heif_image_handle_release (handle);

  gimp_progress_update (0.66);

  writer.writer_api_version = 1;
  writer.write              = write_callback;

  output = G_OUTPUT_STREAM (g_file_replace (file,
                                            NULL, FALSE, G_FILE_CREATE_NONE,
                                            NULL, error));
  if (! output)
    {
      heif_encoder_release (encoder);
      heif_image_release (image);
      heif_context_free (context);
      return FALSE;
    }

  err = heif_context_write (context, &writer, output);

  if (err.code != 0)
    {
      GCancellable *cancellable = g_cancellable_new ();

      g_cancellable_cancel (cancellable);
      g_output_stream_close (output, cancellable, NULL);
      g_object_unref (cancellable);

      g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
                   _("Writing HEIF image failed: %s"),
                   err.message);

      heif_encoder_release (encoder);
      heif_image_release (image);
      heif_context_free (context);
      return FALSE;
    }

  g_object_unref (output);

  heif_encoder_release (encoder);
  heif_image_release (image);
  heif_context_free (context);

  gimp_progress_update (1.0);

  return TRUE;
}


/*  the load dialog  */

#define MAX_THUMBNAIL_SIZE 320

typedef struct _HeifImage HeifImage;

struct _HeifImage
{
  uint32_t           ID;
  gchar              caption[100];
  struct heif_image *thumbnail;
  gint               width;
  gint               height;
};

static gboolean
load_thumbnails (struct heif_context *heif,
                 HeifImage           *images)
{
  guint32 *IDs;
  gint     n_images;
  gint     i;

  n_images = heif_context_get_number_of_top_level_images (heif);

  /* get list of all (top level) image IDs */

  IDs = g_alloca (n_images * sizeof (guint32));

  heif_context_get_list_of_top_level_image_IDs (heif, IDs, n_images);


  /* Load a thumbnail for each image. */

  for (i = 0; i < n_images; i++)
    {
      struct heif_image_handle *handle = NULL;
      struct heif_error         err;
      gint                      width;
      gint                      height;
      struct heif_image_handle *thumbnail_handle = NULL;
      heif_item_id              thumbnail_ID;
      gint                      n_thumbnails;
      struct heif_image        *thumbnail_img = NULL;
      gint                      thumbnail_width;
      gint                      thumbnail_height;

      images[i].ID         = IDs[i];
      images[i].caption[0] = 0;
      images[i].thumbnail  = NULL;

      /* get image handle */

      err = heif_context_get_image_handle (heif, IDs[i], &handle);
      if (err.code)
        {
          gimp_message (err.message);
          continue;
        }

      /* generate image caption */

      width  = heif_image_handle_get_width  (handle);
      height = heif_image_handle_get_height (handle);

      if (heif_image_handle_is_primary_image (handle))
        {
          g_snprintf (images[i].caption, sizeof (images[i].caption),
                      "%dx%d (%s)", width, height, _("primary"));
        }
      else
        {
          g_snprintf (images[i].caption, sizeof (images[i].caption),
                      "%dx%d", width, height);
        }

      /* get handle to thumbnail image
       *
       * if there is no thumbnail image, just the the image itself
       * (will be scaled down later)
       */

      n_thumbnails = heif_image_handle_get_list_of_thumbnail_IDs (handle,
                                                                  &thumbnail_ID,
                                                                  1);

      if (n_thumbnails > 0)
        {
          err = heif_image_handle_get_thumbnail (handle, thumbnail_ID,
                                                 &thumbnail_handle);
          if (err.code)
            {
              gimp_message (err.message);
              continue;
            }
        }
      else
        {
          err = heif_context_get_image_handle (heif, IDs[i], &thumbnail_handle);
          if (err.code)
            {
              gimp_message (err.message);
              continue;
            }
        }

      /* decode the thumbnail image */

      err = heif_decode_image (thumbnail_handle,
                               &thumbnail_img,
                               heif_colorspace_RGB,
                               heif_chroma_interleaved_RGB,
                               NULL);
      if (err.code)
        {
          gimp_message (err.message);
          continue;
        }

      /* if thumbnail image size exceeds the maximum, scale it down */

      thumbnail_width  = heif_image_handle_get_width  (thumbnail_handle);
      thumbnail_height = heif_image_handle_get_height (thumbnail_handle);

      if (thumbnail_width  > MAX_THUMBNAIL_SIZE ||
          thumbnail_height > MAX_THUMBNAIL_SIZE)
        {
          /* compute scaling factor to fit into a max sized box */

          gfloat factor_h = thumbnail_width  / (gfloat) MAX_THUMBNAIL_SIZE;
          gfloat factor_v = thumbnail_height / (gfloat) MAX_THUMBNAIL_SIZE;
          gint   new_width, new_height;
          struct heif_image *scaled_img = NULL;

          if (factor_v > factor_h)
            {
              new_height = MAX_THUMBNAIL_SIZE;
              new_width  = thumbnail_width / factor_v;
            }
          else
            {
              new_height = thumbnail_height / factor_h;
              new_width  = MAX_THUMBNAIL_SIZE;
            }

          /* scale the image */

          err = heif_image_scale_image (thumbnail_img,
                                        &scaled_img,
                                        new_width, new_height,
                                        NULL);
          if (err.code)
            {
              gimp_message (err.message);
              continue;
            }

          /* release the old image and only keep the scaled down version */

          heif_image_release (thumbnail_img);
          thumbnail_img = scaled_img;

          thumbnail_width  = new_width;
          thumbnail_height = new_height;
        }

      heif_image_handle_release (thumbnail_handle);
      heif_image_handle_release (handle);

      /* remember the HEIF thumbnail image (we need it for the GdkPixbuf) */

      images[i].thumbnail = thumbnail_img;

      images[i].width  = thumbnail_width;
      images[i].height = thumbnail_height;
    }

  return TRUE;
}

static void
load_dialog_item_activated (GtkIconView *icon_view,
                            GtkTreePath *path,
                            GtkDialog   *dialog)
{
  gtk_dialog_response (dialog, GTK_RESPONSE_OK);
}

static gboolean
load_dialog (struct heif_context *heif,
             uint32_t            *selected_image)
{
  GtkWidget       *dialog;
  GtkWidget       *main_vbox;
  GtkWidget       *frame;
  HeifImage       *heif_images;
  GtkListStore    *list_store;
  GtkTreeIter      iter;
  GtkWidget       *scrolled_window;
  GtkWidget       *icon_view;
  GtkCellRenderer *renderer;
  gint             n_images;
  gint             i;
  gint             selected_idx = -1;
  gboolean         run          = FALSE;

  n_images = heif_context_get_number_of_top_level_images (heif);

  heif_images = g_alloca (n_images * sizeof (HeifImage));

  if (! load_thumbnails (heif, heif_images))
    return FALSE;

  dialog = gimp_dialog_new (_("Load HEIF Image"), PLUG_IN_BINARY,
                            NULL, 0,
                            gimp_standard_help_func, LOAD_PROC,

                            _("_Cancel"), GTK_RESPONSE_CANCEL,
                            _("_OK"),     GTK_RESPONSE_OK,

                            NULL);

  main_vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 12);
  gtk_container_set_border_width (GTK_CONTAINER (main_vbox), 12);
  gtk_box_pack_start (GTK_BOX (gtk_dialog_get_content_area (GTK_DIALOG (dialog))),
                      main_vbox, TRUE, TRUE, 0);

  frame = gimp_frame_new (_("Select Image"));
  gtk_box_pack_start (GTK_BOX (main_vbox), frame, TRUE, TRUE, 0);
  gtk_widget_show (frame);

  /* prepare list store with all thumbnails and caption */

  list_store = gtk_list_store_new (2, G_TYPE_STRING, GDK_TYPE_PIXBUF);

  for (i = 0; i < n_images; i++)
    {
      GdkPixbuf    *pixbuf;
      const guint8 *data;
      gint          stride;

      gtk_list_store_append (list_store, &iter);
      gtk_list_store_set (list_store, &iter, 0, heif_images[i].caption, -1);

      data = heif_image_get_plane_readonly (heif_images[i].thumbnail,
                                            heif_channel_interleaved,
                                            &stride);

      pixbuf = gdk_pixbuf_new_from_data (data,
                                         GDK_COLORSPACE_RGB,
                                         FALSE,
                                         8,
                                         heif_images[i].width,
                                         heif_images[i].height,
                                         stride,
                                         NULL,
                                         NULL);

      gtk_list_store_set (list_store, &iter, 1, pixbuf, -1);
    }

  scrolled_window = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (scrolled_window),
                                       GTK_SHADOW_IN);
  gtk_widget_set_size_request (scrolled_window,
                               2   * MAX_THUMBNAIL_SIZE,
                               1.5 * MAX_THUMBNAIL_SIZE);
  gtk_container_add (GTK_CONTAINER (frame), scrolled_window);
  gtk_widget_show (scrolled_window);

  icon_view = gtk_icon_view_new_with_model (GTK_TREE_MODEL (list_store));
  gtk_container_add (GTK_CONTAINER (scrolled_window), icon_view);
  gtk_widget_show (icon_view);

  renderer = gtk_cell_renderer_pixbuf_new ();
  gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (icon_view), renderer, FALSE);
  gtk_cell_layout_set_attributes (GTK_CELL_LAYOUT (icon_view), renderer,
                                  "pixbuf", 1,
                                  NULL);

  renderer = gtk_cell_renderer_text_new ();
  gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (icon_view), renderer, FALSE);
  gtk_cell_layout_set_attributes (GTK_CELL_LAYOUT (icon_view), renderer,
                                  "text", 0,
                                  NULL);
  g_object_set (renderer,
                "alignment", PANGO_ALIGN_CENTER,
                "wrap-mode", PANGO_WRAP_WORD_CHAR,
                "xalign",    0.5,
                "yalign",    0.0,
                NULL);

  g_signal_connect (icon_view, "item-activated",
                    G_CALLBACK (load_dialog_item_activated),
                    dialog);

  /* pre-select the primary image */

  for (i = 0; i < n_images; i++)
    {
      if (heif_images[i].ID == *selected_image)
        {
          selected_idx = i;
          break;
        }
    }

  if (selected_idx != -1)
    {
      GtkTreePath *path = gtk_tree_path_new_from_indices (selected_idx, -1);

      gtk_icon_view_select_path (GTK_ICON_VIEW (icon_view), path);
      gtk_tree_path_free (path);
    }

  gtk_widget_show (main_vbox);
  gtk_widget_show (dialog);

  run = (gimp_dialog_run (GIMP_DIALOG (dialog)) == GTK_RESPONSE_OK);

  if (run)
    {
      GList *selected_items =
        gtk_icon_view_get_selected_items (GTK_ICON_VIEW (icon_view));

      if (selected_items)
        {
          GtkTreePath *path    = selected_items->data;
          gint        *indices = gtk_tree_path_get_indices (path);

          *selected_image = heif_images[indices[0]].ID;

          g_list_free_full (selected_items,
                            (GDestroyNotify) gtk_tree_path_free);
        }
    }

  gtk_widget_destroy (dialog);

  /* release thumbnail images */

  for (i = 0 ; i < n_images; i++)
    heif_image_release (heif_images[i].thumbnail);

  return run;
}


/*  the save dialog  */

static void
save_dialog_lossless_button_toggled (GtkToggleButton *source,
                                     GtkWidget       *hbox)
{
  gboolean lossless = gtk_toggle_button_get_active (source);

  gtk_widget_set_sensitive (hbox, ! lossless);
}

gboolean
save_dialog (SaveParams  *params,
             gint32       image_ID,
             const gchar *procname)
{
  GtkWidget *dialog;
  GtkWidget *main_vbox;
  GtkWidget *hbox;
  GtkWidget *label;
  GtkWidget *lossless_button;
  GtkWidget *frame;
#if LIBHEIF_HAVE_VERSION(1,4,0)
  GtkWidget *profile_button;
#endif
  GtkWidget *quality_slider;
#if LIBHEIF_HAVE_VERSION(1,8,0)
  GtkWidget *table;
  GtkWidget *label2;
  GtkWidget *combo;
#endif
  gboolean   run = FALSE;

  dialog = gimp_export_dialog_new (g_strcmp0 (procname, SAVE_PROC_AV1) == 0?
                                   "AVIF" : "HEIF",
                                   PLUG_IN_BINARY, procname);

  main_vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 12);
  gtk_container_set_border_width (GTK_CONTAINER (main_vbox), 12);
  gtk_box_pack_start (GTK_BOX (gimp_export_dialog_get_content_area (dialog)),
                      main_vbox, TRUE, TRUE, 0);

  frame = gimp_frame_new (NULL);
  gtk_box_pack_start (GTK_BOX (main_vbox), frame, FALSE, FALSE, 0);

  lossless_button = gtk_check_button_new_with_mnemonic (_("Nearly _lossless"));
  gtk_frame_set_label_widget (GTK_FRAME (frame), lossless_button);

  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 6);
  label = gtk_label_new_with_mnemonic (_("_Quality:"));
  quality_slider = gtk_hscale_new_with_range (0, 100, 5);
  gtk_scale_set_value_pos (GTK_SCALE (quality_slider), GTK_POS_RIGHT);
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (hbox), quality_slider, TRUE, TRUE, 0);
  gtk_container_add (GTK_CONTAINER (frame), hbox);

  gtk_range_set_value (GTK_RANGE (quality_slider), params->quality);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (lossless_button),
                                params->lossless);
  gtk_widget_set_sensitive (hbox, !params->lossless);
  gtk_label_set_mnemonic_widget (GTK_LABEL (label), quality_slider);

  g_signal_connect (lossless_button, "toggled",
                    G_CALLBACK (save_dialog_lossless_button_toggled),
                    hbox);

#if LIBHEIF_HAVE_VERSION(1,8,0)
  switch (gimp_image_get_precision (image_ID))
    {
    case GIMP_PRECISION_U8_LINEAR:
    case GIMP_PRECISION_U8_GAMMA:
      /* image is 8bit depth */
      params->save_bit_depth = 8;
      break;
    default:
      /* high bit depth */
      if (params->save_bit_depth < 12)
        {
          params->save_bit_depth = 10;
        }
      else if (params->save_bit_depth > 12)
        {
          params->save_bit_depth = 12;
        }
      break;
    }

  table = gtk_table_new (1, 2, FALSE);
  gtk_table_set_col_spacings (GTK_TABLE (table), 6);
  gtk_box_pack_start (GTK_BOX (main_vbox), table, FALSE, FALSE, 0);
  gtk_widget_show (table);

  label2 = gtk_label_new (_("Bit depth:"));
  gtk_label_set_xalign (GTK_LABEL (label2), 0.0);
  gtk_table_attach (GTK_TABLE (table), label2, 0, 1, 0, 1,
                    GTK_FILL, GTK_FILL, 0, 0);
  gtk_widget_show (label2);

  combo = gimp_int_combo_box_new (_("8 bit/channel"), 8,
                                  _("10 bit/channel"), 10,
                                  _("12 bit/channel"), 12,
                                  NULL);
  gtk_table_attach (GTK_TABLE (table), combo, 1, 2, 0, 1,
                    GTK_FILL | GTK_EXPAND, GTK_FILL, 0, 0);
  gtk_widget_show (combo);
  gimp_int_combo_box_set_active (GIMP_INT_COMBO_BOX (combo), params->save_bit_depth);
#endif

#if LIBHEIF_HAVE_VERSION(1,4,0)
  profile_button = gtk_check_button_new_with_mnemonic (_("Save color _profile"));
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (profile_button),
                                params->save_profile);
  g_signal_connect (profile_button, "toggled",
                    G_CALLBACK (gimp_toggle_button_update),
                    &params->save_profile);
  gtk_box_pack_start (GTK_BOX (main_vbox), profile_button, FALSE, FALSE, 0);
#endif

  gtk_widget_show_all (dialog);

  run = (gimp_dialog_run (GIMP_DIALOG (dialog)) == GTK_RESPONSE_OK);

  if (run)
    {
      params->quality  = gtk_range_get_value (GTK_RANGE (quality_slider));
      params->lossless = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (lossless_button));
#if LIBHEIF_HAVE_VERSION(1,8,0)
      gimp_int_combo_box_get_active (GIMP_INT_COMBO_BOX (combo), &params->save_bit_depth);
#endif
    }

  gtk_widget_destroy (dialog);

  return run;
}
