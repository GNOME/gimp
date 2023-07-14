/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
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

#include <stdio.h>
#include <setjmp.h>
#include <string.h>

#include <jpeglib.h>
#include <jerror.h>

#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

#include "libgimp/stdplugins-intl.h"

#include "jpeg.h"
#include "jpeg-settings.h"
#include "jpeg-load.h"
#include "jpeg-save.h"


typedef struct _Jpeg      Jpeg;
typedef struct _JpegClass JpegClass;

struct _Jpeg
{
  GimpPlugIn      parent_instance;
};

struct _JpegClass
{
  GimpPlugInClass parent_class;
};


#define JPEG_TYPE  (jpeg_get_type ())
#define JPEG (obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), JPEG_TYPE, Jpeg))

GType                   jpeg_get_type         (void) G_GNUC_CONST;

static GList          * jpeg_query_procedures (GimpPlugIn           *plug_in);
static GimpProcedure  * jpeg_create_procedure (GimpPlugIn           *plug_in,
                                               const gchar          *name);

static GimpValueArray * jpeg_load             (GimpProcedure        *procedure,
                                               GimpRunMode           run_mode,
                                               GFile                *file,
                                               const GimpValueArray *args,
                                               gpointer              run_data);
static GimpValueArray * jpeg_load_thumb       (GimpProcedure        *procedure,
                                               GFile                *file,
                                               gint                  size,
                                               const GimpValueArray *args,
                                               gpointer              run_data);
static GimpValueArray * jpeg_save             (GimpProcedure        *procedure,
                                               GimpRunMode           run_mode,
                                               GimpImage            *image,
                                               gint                  n_drawables,
                                               GimpDrawable        **drawables,
                                               GFile                *file,
                                               const GimpValueArray *args,
                                               gpointer              run_data);


G_DEFINE_TYPE (Jpeg, jpeg, GIMP_TYPE_PLUG_IN)

GIMP_MAIN (JPEG_TYPE)
DEFINE_STD_SET_I18N


gboolean         undo_touched      = FALSE;
GimpDisplay     *display           = NULL;
gboolean         separate_display  = FALSE;
GimpImage       *orig_image_global = NULL;
GimpDrawable    *drawable_global   = NULL;


static void
jpeg_class_init (JpegClass *klass)
{
  GimpPlugInClass *plug_in_class = GIMP_PLUG_IN_CLASS (klass);

  plug_in_class->query_procedures = jpeg_query_procedures;
  plug_in_class->create_procedure = jpeg_create_procedure;
  plug_in_class->set_i18n         = STD_SET_I18N;
}

static void
jpeg_init (Jpeg *jpeg)
{
}

static GList *
jpeg_query_procedures (GimpPlugIn *plug_in)
{
  GList *list = NULL;

  list = g_list_append (list, g_strdup (LOAD_THUMB_PROC));
  list = g_list_append (list, g_strdup (LOAD_PROC));
  list = g_list_append (list, g_strdup (SAVE_PROC));

  return list;
}

static GimpProcedure *
jpeg_create_procedure (GimpPlugIn  *plug_in,
                       const gchar *name)
{
  GimpProcedure *procedure = NULL;

  if (! strcmp (name, LOAD_PROC))
    {
      procedure = gimp_load_procedure_new (plug_in, name,
                                           GIMP_PDB_PROC_TYPE_PLUGIN,
                                           jpeg_load, NULL, NULL);

      gimp_procedure_set_menu_label (procedure, _("JPEG image"));

      gimp_procedure_set_documentation (procedure,
                                        "Loads files in the JPEG file format",
                                        "Loads files in the JPEG file format",
                                        name);
      gimp_procedure_set_attribution (procedure,
                                      "Spencer Kimball, Peter Mattis & others",
                                      "Spencer Kimball & Peter Mattis",
                                      "1995-2007");

      gimp_file_procedure_set_mime_types (GIMP_FILE_PROCEDURE (procedure),
                                          "image/jpeg");
      gimp_file_procedure_set_extensions (GIMP_FILE_PROCEDURE (procedure),
                                          "jpg,jpeg,jpe");
      gimp_file_procedure_set_magics (GIMP_FILE_PROCEDURE (procedure),
                                      "0,string,\xff\xd8\xff");

      gimp_load_procedure_set_thumbnail_loader (GIMP_LOAD_PROCEDURE (procedure),
                                                LOAD_THUMB_PROC);
    }
  else if (! strcmp (name, LOAD_THUMB_PROC))
    {
      procedure = gimp_thumbnail_procedure_new (plug_in, name,
                                                GIMP_PDB_PROC_TYPE_PLUGIN,
                                                jpeg_load_thumb, NULL, NULL);

      gimp_procedure_set_documentation (procedure,
                                        "Loads a thumbnail from a JPEG image",
                                        "Loads a thumbnail from a JPEG image, "
                                        "if one exists",
                                        name);
      gimp_procedure_set_attribution (procedure,
                                      "Mukund Sivaraman <muks@mukund.org>, "
                                      "Sven Neumann <sven@gimp.org>",
                                      "Mukund Sivaraman <muks@mukund.org>, "
                                      "Sven Neumann <sven@gimp.org>",
                                      "November 15, 2004");
    }
  else if (! strcmp (name, SAVE_PROC))
    {
      procedure = gimp_save_procedure_new (plug_in, name,
                                           GIMP_PDB_PROC_TYPE_PLUGIN,
                                           jpeg_save, NULL, NULL);

      gimp_procedure_set_image_types (procedure, "RGB*, GRAY*");

      gimp_procedure_set_menu_label (procedure, _("JPEG image"));

      gimp_procedure_set_documentation (procedure,
                                        "Saves files in the JPEG file format",
                                        "Saves files in the lossy, widely "
                                        "supported JPEG format",
                                        name);
      gimp_procedure_set_attribution (procedure,
                                      "Spencer Kimball, Peter Mattis & others",
                                      "Spencer Kimball & Peter Mattis",
                                      "1995-2007");

      gimp_file_procedure_set_format_name (GIMP_FILE_PROCEDURE (procedure),
                                           _("JPEG"));
      gimp_file_procedure_set_mime_types (GIMP_FILE_PROCEDURE (procedure),
                                          "image/jpeg");
      gimp_file_procedure_set_extensions (GIMP_FILE_PROCEDURE (procedure),
                                          "jpg,jpeg,jpe");

      /* See bugs #63610 and #61088 for a discussion about the quality
       * settings
       */
      GIMP_PROC_ARG_DOUBLE (procedure, "quality",
                            "_Quality",
                            "Quality of exported image",
                            0.0, 1.0, 0.9,
                            G_PARAM_READWRITE);

      GIMP_PROC_ARG_DOUBLE (procedure, "smoothing",
                            "S_moothing",
                            "Smoothing factor for exported image",
                            0.0, 1.0, 0.0,
                            G_PARAM_READWRITE);

      GIMP_PROC_ARG_BOOLEAN (procedure, "optimize",
                             "Optimi_ze",
                             "Use optimized tables during Huffman coding",
                             TRUE,
                             G_PARAM_READWRITE);

      GIMP_PROC_ARG_BOOLEAN (procedure, "progressive",
                             "_Progressive",
                             "Create progressive JPEG images",
                             TRUE,
                             G_PARAM_READWRITE);

      GIMP_PROC_ARG_BOOLEAN (procedure, "cmyk",
                             "Export as CM_YK",
                             "Create a CMYK JPEG image using the soft-proofing color profile",
                             FALSE,
                             G_PARAM_READWRITE);

      GIMP_PROC_ARG_INT (procedure, "sub-sampling",
                         _("Su_bsampling"),
                         "Sub-sampling type { 0 == 4:2:0 (chroma quartered), "
                         "1 == 4:2:2 (chroma halved horizontally), "
                         "2 == 4:4:4 (best quality), "
                         "3 == 4:4:0 (chroma halved vertically)",
                         JPEG_SUBSAMPLING_2x2_1x1_1x1,
                         JPEG_SUBSAMPLING_1x2_1x1_1x1,
                         JPEG_SUBSAMPLING_1x1_1x1_1x1,
                         G_PARAM_READWRITE);

      GIMP_PROC_ARG_BOOLEAN (procedure, "baseline",
                             "Baseline",
                             "Force creation of a baseline JPEG "
                             "(non-baseline JPEGs can't be read by all decoders)",
                             TRUE,
                             G_PARAM_READWRITE);

      GIMP_PROC_ARG_INT (procedure, "restart",
                         _("Inter_val (MCU rows):"),
                         "Interval of restart markers "
                         "(in MCU rows, 0 = no restart markers)",
                         0, 64, 0,
                         G_PARAM_READWRITE);

      GIMP_PROC_ARG_INT (procedure, "dct",
                         _("_DCT method"),
                         "DCT method to use { INTEGER (0), FIXED (1), "
                         "FLOAT (2) }",
                         0, 2, 0,
                         G_PARAM_READWRITE);

      /* Some auxiliary arguments mostly for interactive usage. */

      GIMP_PROC_AUX_ARG_BOOLEAN (procedure, "use-original-quality",
                                 "_Use quality settings from original image",
                                 "If the original image was loaded from a JPEG "
                                 "file using non-standard quality settings "
                                 "(quantization tables), enable this option to "
                                 "get almost the same quality and file size.",
                                 FALSE,
                                 G_PARAM_READWRITE);
      GIMP_PROC_AUX_ARG_INT (procedure, "original-quality",
                             NULL, NULL,
                             -1, 100, -1,
                             G_PARAM_READWRITE);
      GIMP_PROC_AUX_ARG_INT (procedure, "original-sub-sampling",
                             NULL, NULL,
                             JPEG_SUBSAMPLING_2x2_1x1_1x1,
                             JPEG_SUBSAMPLING_1x2_1x1_1x1,
                             JPEG_SUBSAMPLING_2x2_1x1_1x1,
                             G_PARAM_READWRITE);
      GIMP_PROC_AUX_ARG_INT (procedure, "original-num-quant-tables",
                             NULL, NULL,
                             -1, 4, -1,
                             G_PARAM_READWRITE);

      GIMP_PROC_AUX_ARG_BOOLEAN (procedure, "show-preview",
                                 "Sho_w preview in image window",
                                 "Creates a temporary layer with an export preview",
                                 FALSE,
                                 G_PARAM_READWRITE);
      GIMP_PROC_AUX_ARG_BOOLEAN (procedure, "use-arithmetic-coding",
                                 "Use _arithmetic coding",
                                 _("Older software may have trouble opening "
                                   "arithmetic-coded images"),
                                 FALSE,
                                 G_PARAM_READWRITE);
      GIMP_PROC_AUX_ARG_BOOLEAN (procedure, "use-restart",
                                 _("Use restart mar_kers"),
                                 NULL, FALSE,
                                 G_PARAM_READWRITE);

      gimp_save_procedure_set_support_exif      (GIMP_SAVE_PROCEDURE (procedure), TRUE);
      gimp_save_procedure_set_support_iptc      (GIMP_SAVE_PROCEDURE (procedure), TRUE);
      gimp_save_procedure_set_support_xmp       (GIMP_SAVE_PROCEDURE (procedure), TRUE);
      gimp_save_procedure_set_support_profile   (GIMP_SAVE_PROCEDURE (procedure), TRUE);
      gimp_save_procedure_set_support_thumbnail (GIMP_SAVE_PROCEDURE (procedure), TRUE);
      gimp_save_procedure_set_support_comment   (GIMP_SAVE_PROCEDURE (procedure), TRUE);
    }

  return procedure;
}

static GimpValueArray *
jpeg_load (GimpProcedure        *procedure,
           GimpRunMode           run_mode,
           GFile                *file,
           const GimpValueArray *args,
           gpointer              run_data)
{
  GimpValueArray *return_vals;
  GimpImage      *image;
  gboolean        resolution_loaded  = FALSE;
  gboolean        ps_metadata_loaded = FALSE;
  GError         *error              = NULL;

  gegl_init (NULL, NULL);

  preview_image = NULL;
  preview_layer = NULL;

  switch (run_mode)
    {
    case GIMP_RUN_INTERACTIVE:
    case GIMP_RUN_WITH_LAST_VALS:
      gimp_ui_init (PLUG_IN_BINARY);
      break;

    default:
      break;
    }

  image = load_image (file, run_mode, FALSE,
                      &resolution_loaded, &ps_metadata_loaded, &error);

  if (image)
    {
      GimpMetadata *metadata;

      metadata = gimp_image_metadata_load_prepare (image, "image/jpeg",
                                                   file, NULL);

      if (metadata)
        {
          GimpMetadataLoadFlags flags = GIMP_METADATA_LOAD_ALL;

          if (resolution_loaded)
            flags &= ~GIMP_METADATA_LOAD_RESOLUTION;

          gimp_image_metadata_load_finish (image, "image/jpeg",
                                           metadata, flags);

          g_object_unref (metadata);
        }
    }

  if (! image)
    return gimp_procedure_new_return_values (procedure,
                                             GIMP_PDB_EXECUTION_ERROR,
                                             error);

  return_vals = gimp_procedure_new_return_values (procedure,
                                                  GIMP_PDB_SUCCESS,
                                                  NULL);

  GIMP_VALUES_SET_IMAGE (return_vals, 1, image);

  return return_vals;
}

static GimpValueArray *
jpeg_load_thumb (GimpProcedure        *procedure,
                 GFile                *file,
                 gint                  size,
                 const GimpValueArray *args,
                 gpointer              run_data)
{
  GimpValueArray *return_vals;
  GimpImage      *image;
  gint            width  = 0;
  gint            height = 0;
  GimpImageType   type   = -1;
  GError         *error  = NULL;

  gegl_init (NULL, NULL);

  preview_image = NULL;
  preview_layer = NULL;

  image = load_thumbnail_image (file, &width, &height, &type,
                                &error);


  if (! image)
    return gimp_procedure_new_return_values (procedure,
                                             GIMP_PDB_EXECUTION_ERROR,
                                             error);

  return_vals = gimp_procedure_new_return_values (procedure,
                                                  GIMP_PDB_SUCCESS,
                                                  NULL);

  GIMP_VALUES_SET_IMAGE (return_vals, 1, image);
  GIMP_VALUES_SET_INT   (return_vals, 2, width);
  GIMP_VALUES_SET_INT   (return_vals, 3, height);
  GIMP_VALUES_SET_ENUM  (return_vals, 4, type);
  GIMP_VALUES_SET_INT   (return_vals, 5, 1); /* 1 layer */

  return return_vals;
}

static GimpValueArray *
jpeg_save (GimpProcedure        *procedure,
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
  GimpMetadata          *metadata;
  GimpImage             *orig_image;
  GimpExportReturn       export = GIMP_EXPORT_CANCEL;
  GError                *error  = NULL;

  gboolean               has_alpha             = FALSE;
  gint                   orig_num_quant_tables = -1;
  gint                   orig_quality          = -1;
  JpegSubsampling        orig_subsmp           = JPEG_SUBSAMPLING_2x2_1x1_1x1;

  gegl_init (NULL, NULL);

  config = gimp_procedure_create_config (procedure);
  metadata = gimp_procedure_config_begin_export (config, image, run_mode,
                                                 args, "image/jpeg");
  preview_image = NULL;
  preview_layer = NULL;

  orig_image = image;
  has_alpha = gimp_drawable_has_alpha (drawables[0]);

  switch (run_mode)
    {
    case GIMP_RUN_INTERACTIVE:
    case GIMP_RUN_WITH_LAST_VALS:
      gimp_ui_init (PLUG_IN_BINARY);

      export = gimp_export_image (&image, &n_drawables, &drawables, "JPEG",
                                  GIMP_EXPORT_CAN_HANDLE_RGB |
                                  GIMP_EXPORT_CAN_HANDLE_GRAY);

      switch (export)
        {
        case GIMP_EXPORT_EXPORT:
          display = NULL;
          separate_display = TRUE;
          break;

        case GIMP_EXPORT_IGNORE:
          break;

        case GIMP_EXPORT_CANCEL:
          return gimp_procedure_new_return_values (procedure,
                                                   GIMP_PDB_CANCEL,
                                                   NULL);
          break;
        }
      break;

    default:
      break;
    }

  if (n_drawables != 1)
    {
      g_set_error (&error, G_FILE_ERROR, 0,
                   _("JPEG format does not support multiple layers."));

      return gimp_procedure_new_return_values (procedure,
                                               GIMP_PDB_CALLING_ERROR,
                                               error);
    }

  /* Override preferences from JPG export defaults (if saved). */

  switch (run_mode)
    {
    case GIMP_RUN_NONINTERACTIVE:
      g_object_set (config, "show-preview", FALSE, NULL);
      break;

    case GIMP_RUN_INTERACTIVE:
    case GIMP_RUN_WITH_LAST_VALS:
        {
          /* restore the values found when loading the file (if available) */
          gdouble dquality;
          gint    quality;
          gint    subsmp;

          jpeg_restore_original_settings (orig_image,
                                          &orig_quality,
                                          &orig_subsmp,
                                          &orig_num_quant_tables);

          g_object_get (config,
                        "quality",      &dquality,
                        "sub-sampling", &subsmp,
                        NULL);

          quality = (gint) (dquality * 100.0);

          /* If this image was loaded from a JPEG file and has not
           * been saved yet, try to use some of the settings from the
           * original file if they are better than the default values.
           */
          if (orig_quality > quality)
            {
              quality = orig_quality;
              dquality = (gdouble) quality / 100.0;
              g_object_set (config, "quality", dquality, NULL);
            }

          if (orig_quality > 0)
            {
              /* Skip changing subsampling to original if we already have
               * best setting or if original have worst setting
               */
              if (!(subsmp == JPEG_SUBSAMPLING_1x1_1x1_1x1 ||
                    orig_subsmp == JPEG_SUBSAMPLING_2x2_1x1_1x1))
                {
                  subsmp = orig_subsmp;
                  g_object_set (config, "sub-sampling", orig_subsmp, NULL);
                }

              if (orig_quality == quality && orig_subsmp == subsmp)
                {
                  g_object_set (config, "use-original-quality", TRUE, NULL);
                }
            }
        }
      break;
    }
  g_object_set (config,
                "original-sub-sampling",     orig_subsmp,
                "original-quality",          orig_quality,
                "original-num-quant-tables", orig_num_quant_tables,
                NULL);

  if (run_mode == GIMP_RUN_INTERACTIVE)
    {
      gboolean show_preview = FALSE;

      g_object_get (config, "show-preview", &show_preview, NULL);
      if (show_preview)
        {
          /* we freeze undo saving so that we can avoid sucking up
           * tile cache with our unneeded preview steps. */
          gimp_image_undo_freeze (image);

          undo_touched = TRUE;
        }

      /* prepare for the preview */
      preview_image     = image;
      orig_image_global = orig_image;
      drawable_global   = drawables[0];

      /*  First acquire information with a dialog  */
      if (! save_dialog (procedure, config, drawables[0], orig_image, has_alpha))
        {
          status = GIMP_PDB_CANCEL;
        }

      if (undo_touched)
        {
          /* thaw undo saving and flush the displays to have them
           * reflect the current shortcuts
           */
          gimp_image_undo_thaw (image);
          gimp_displays_flush ();
        }
    }

  if (status == GIMP_PDB_SUCCESS)
    {
      if (! save_image (file, config,
                        image, drawables[0], orig_image, FALSE,
                        &error))
        {
          status = GIMP_PDB_EXECUTION_ERROR;
        }
    }

  if (status == GIMP_PDB_SUCCESS)
    {
      if (metadata)
        gimp_metadata_set_bits_per_sample (metadata, 8);
    }

  gimp_procedure_config_end_export (config, image, file, status);
  g_object_unref (config);

  if (export == GIMP_EXPORT_EXPORT)
    {
      /* If the image was exported, delete the new display. This also
       * deletes the image.
       */
      if (display)
        {
          gimp_display_delete (display);
          gimp_display_present (gimp_default_display ());
        }
      else
        {
          gimp_image_delete (image);
        }

      g_free (drawables);
    }

  return gimp_procedure_new_return_values (procedure, status, error);
}

/*
 * Here's the routine that will replace the standard error_exit method:
 */

void
my_error_exit (j_common_ptr cinfo)
{
  /* cinfo->err really points to a my_error_mgr struct, so coerce pointer */
  my_error_ptr myerr = (my_error_ptr) cinfo->err;

  /* Always display the message. */
  /* We could postpone this until after returning, if we chose. */
  (*cinfo->err->output_message) (cinfo);

  /* Return control to the setjmp point */
  longjmp (myerr->setjmp_buffer, 1);
}


void
my_output_message (j_common_ptr cinfo)
{
  gchar  buffer[JMSG_LENGTH_MAX + 1];

  (*cinfo->err->format_message)(cinfo, buffer);

  g_message ("%s", buffer);
}
