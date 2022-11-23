/* LIGMA - The GNU Image Manipulation Program
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

#include <libligma/ligma.h>
#include <libligma/ligmaui.h>

#include "libligma/stdplugins-intl.h"

#include "jpeg.h"
#include "jpeg-settings.h"
#include "jpeg-load.h"
#include "jpeg-save.h"


typedef struct _Jpeg      Jpeg;
typedef struct _JpegClass JpegClass;

struct _Jpeg
{
  LigmaPlugIn      parent_instance;
};

struct _JpegClass
{
  LigmaPlugInClass parent_class;
};


#define JPEG_TYPE  (jpeg_get_type ())
#define JPEG (obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), JPEG_TYPE, Jpeg))

GType                   jpeg_get_type         (void) G_GNUC_CONST;

static GList          * jpeg_query_procedures (LigmaPlugIn           *plug_in);
static LigmaProcedure  * jpeg_create_procedure (LigmaPlugIn           *plug_in,
                                               const gchar          *name);

static LigmaValueArray * jpeg_load             (LigmaProcedure        *procedure,
                                               LigmaRunMode           run_mode,
                                               GFile                *file,
                                               const LigmaValueArray *args,
                                               gpointer              run_data);
static LigmaValueArray * jpeg_load_thumb       (LigmaProcedure        *procedure,
                                               GFile                *file,
                                               gint                  size,
                                               const LigmaValueArray *args,
                                               gpointer              run_data);
static LigmaValueArray * jpeg_save             (LigmaProcedure        *procedure,
                                               LigmaRunMode           run_mode,
                                               LigmaImage            *image,
                                               gint                  n_drawables,
                                               LigmaDrawable        **drawables,
                                               GFile                *file,
                                               const LigmaValueArray *args,
                                               gpointer              run_data);


G_DEFINE_TYPE (Jpeg, jpeg, LIGMA_TYPE_PLUG_IN)

LIGMA_MAIN (JPEG_TYPE)
DEFINE_STD_SET_I18N


gboolean         undo_touched      = FALSE;
LigmaDisplay     *display           = NULL;
gboolean         separate_display  = FALSE;
LigmaImage       *orig_image_global = NULL;
LigmaDrawable    *drawable_global   = NULL;


static void
jpeg_class_init (JpegClass *klass)
{
  LigmaPlugInClass *plug_in_class = LIGMA_PLUG_IN_CLASS (klass);

  plug_in_class->query_procedures = jpeg_query_procedures;
  plug_in_class->create_procedure = jpeg_create_procedure;
  plug_in_class->set_i18n         = STD_SET_I18N;
}

static void
jpeg_init (Jpeg *jpeg)
{
}

static GList *
jpeg_query_procedures (LigmaPlugIn *plug_in)
{
  GList *list = NULL;

  list = g_list_append (list, g_strdup (LOAD_THUMB_PROC));
  list = g_list_append (list, g_strdup (LOAD_PROC));
  list = g_list_append (list, g_strdup (SAVE_PROC));

  return list;
}

static LigmaProcedure *
jpeg_create_procedure (LigmaPlugIn  *plug_in,
                       const gchar *name)
{
  LigmaProcedure *procedure = NULL;

  if (! strcmp (name, LOAD_PROC))
    {
      procedure = ligma_load_procedure_new (plug_in, name,
                                           LIGMA_PDB_PROC_TYPE_PLUGIN,
                                           jpeg_load, NULL, NULL);

      ligma_procedure_set_menu_label (procedure, _("JPEG image"));

      ligma_procedure_set_documentation (procedure,
                                        "Loads files in the JPEG file format",
                                        "Loads files in the JPEG file format",
                                        name);
      ligma_procedure_set_attribution (procedure,
                                      "Spencer Kimball, Peter Mattis & others",
                                      "Spencer Kimball & Peter Mattis",
                                      "1995-2007");

      ligma_file_procedure_set_mime_types (LIGMA_FILE_PROCEDURE (procedure),
                                          "image/jpeg");
      ligma_file_procedure_set_extensions (LIGMA_FILE_PROCEDURE (procedure),
                                          "jpg,jpeg,jpe");
      ligma_file_procedure_set_magics (LIGMA_FILE_PROCEDURE (procedure),
                                      "0,string,\xff\xd8\xff");

      ligma_load_procedure_set_thumbnail_loader (LIGMA_LOAD_PROCEDURE (procedure),
                                                LOAD_THUMB_PROC);
    }
  else if (! strcmp (name, LOAD_THUMB_PROC))
    {
      procedure = ligma_thumbnail_procedure_new (plug_in, name,
                                                LIGMA_PDB_PROC_TYPE_PLUGIN,
                                                jpeg_load_thumb, NULL, NULL);

      ligma_procedure_set_documentation (procedure,
                                        "Loads a thumbnail from a JPEG image",
                                        "Loads a thumbnail from a JPEG image, "
                                        "if one exists",
                                        name);
      ligma_procedure_set_attribution (procedure,
                                      "Mukund Sivaraman <muks@mukund.org>, "
                                      "Sven Neumann <sven@ligma.org>",
                                      "Mukund Sivaraman <muks@mukund.org>, "
                                      "Sven Neumann <sven@ligma.org>",
                                      "November 15, 2004");
    }
  else if (! strcmp (name, SAVE_PROC))
    {
      procedure = ligma_save_procedure_new (plug_in, name,
                                           LIGMA_PDB_PROC_TYPE_PLUGIN,
                                           jpeg_save, NULL, NULL);

      ligma_procedure_set_image_types (procedure, "RGB*, GRAY*");

      ligma_procedure_set_menu_label (procedure, _("JPEG image"));

      ligma_procedure_set_documentation (procedure,
                                        "Saves files in the JPEG file format",
                                        "Saves files in the lossy, widely "
                                        "supported JPEG format",
                                        name);
      ligma_procedure_set_attribution (procedure,
                                      "Spencer Kimball, Peter Mattis & others",
                                      "Spencer Kimball & Peter Mattis",
                                      "1995-2007");

      ligma_file_procedure_set_format_name (LIGMA_FILE_PROCEDURE (procedure),
                                           _("JPEG"));
      ligma_file_procedure_set_mime_types (LIGMA_FILE_PROCEDURE (procedure),
                                          "image/jpeg");
      ligma_file_procedure_set_extensions (LIGMA_FILE_PROCEDURE (procedure),
                                          "jpg,jpeg,jpe");

      /* See bugs #63610 and #61088 for a discussion about the quality
       * settings
       */
      LIGMA_PROC_ARG_DOUBLE (procedure, "quality",
                            "_Quality",
                            "Quality of exported image",
                            0.0, 1.0, 0.9,
                            G_PARAM_READWRITE);

      LIGMA_PROC_ARG_DOUBLE (procedure, "smoothing",
                            "S_moothing",
                            "Smoothing factor for exported image",
                            0.0, 1.0, 0.0,
                            G_PARAM_READWRITE);

      LIGMA_PROC_ARG_BOOLEAN (procedure, "optimize",
                             "Optimi_ze",
                             "Use optimized tables during Huffman coding",
                             TRUE,
                             G_PARAM_READWRITE);

      LIGMA_PROC_ARG_BOOLEAN (procedure, "progressive",
                             "_Progressive",
                             "Create progressive JPEG images",
                             TRUE,
                             G_PARAM_READWRITE);

      LIGMA_PROC_ARG_BOOLEAN (procedure, "cmyk",
                             "Export as _CMYK",
                             "Create a CMYK JPEG image using the soft-proofing color profile",
                             FALSE,
                             G_PARAM_READWRITE);

      LIGMA_PROC_ARG_INT (procedure, "sub-sampling",
                         _("Su_bsampling"),
                         "Sub-sampling type { 0 == 4:2:0 (chroma quartered), "
                         "1 == 4:2:2 Horizontal (chroma halved), "
                         "2 == 4:4:4 (best quality), "
                         "3 == 4:2:2 Vertical (chroma halved)",
                         JPEG_SUBSAMPLING_2x2_1x1_1x1,
                         JPEG_SUBSAMPLING_1x2_1x1_1x1,
                         JPEG_SUBSAMPLING_1x1_1x1_1x1,
                         G_PARAM_READWRITE);

      LIGMA_PROC_ARG_BOOLEAN (procedure, "baseline",
                             "Baseline",
                             "Force creation of a baseline JPEG "
                             "(non-baseline JPEGs can't be read by all decoders)",
                             TRUE,
                             G_PARAM_READWRITE);

      LIGMA_PROC_ARG_INT (procedure, "restart",
                         _("Inter_val (MCU rows):"),
                         "Interval of restart markers "
                         "(in MCU rows, 0 = no restart markers)",
                         0, 64, 0,
                         G_PARAM_READWRITE);

      LIGMA_PROC_ARG_INT (procedure, "dct",
                         _("_DCT method"),
                         "DCT method to use { INTEGER (0), FIXED (1), "
                         "FLOAT (2) }",
                         0, 2, 0,
                         G_PARAM_READWRITE);

      /* Some auxiliary arguments mostly for interactive usage. */

      LIGMA_PROC_AUX_ARG_BOOLEAN (procedure, "use-original-quality",
                                 "_Use quality settings from original image",
                                 "If the original image was loaded from a JPEG "
                                 "file using non-standard quality settings "
                                 "(quantization tables), enable this option to "
                                 "get almost the same quality and file size.",
                                 FALSE,
                                 G_PARAM_READWRITE);
      LIGMA_PROC_AUX_ARG_INT (procedure, "original-quality",
                             NULL, NULL,
                             -1, 100, -1,
                             G_PARAM_READWRITE);
      LIGMA_PROC_AUX_ARG_INT (procedure, "original-sub-sampling",
                             NULL, NULL,
                             JPEG_SUBSAMPLING_2x2_1x1_1x1,
                             JPEG_SUBSAMPLING_1x2_1x1_1x1,
                             JPEG_SUBSAMPLING_2x2_1x1_1x1,
                             G_PARAM_READWRITE);
      LIGMA_PROC_AUX_ARG_INT (procedure, "original-num-quant-tables",
                             NULL, NULL,
                             -1, 4, -1,
                             G_PARAM_READWRITE);

      LIGMA_PROC_AUX_ARG_BOOLEAN (procedure, "show-preview",
                                 "Sho_w preview in image window",
                                 "Creates a temporary layer with an export preview",
                                 FALSE,
                                 G_PARAM_READWRITE);
      LIGMA_PROC_AUX_ARG_BOOLEAN (procedure, "use-arithmetic-coding",
                                 "Use _arithmetic coding",
                                 _("Older software may have trouble opening "
                                   "arithmetic-coded images"),
                                 FALSE,
                                 G_PARAM_READWRITE);
      LIGMA_PROC_AUX_ARG_BOOLEAN (procedure, "use-restart",
                                 _("Use restart mar_kers"),
                                 NULL, FALSE,
                                 G_PARAM_READWRITE);

      ligma_save_procedure_set_support_exif      (LIGMA_SAVE_PROCEDURE (procedure), TRUE);
      ligma_save_procedure_set_support_iptc      (LIGMA_SAVE_PROCEDURE (procedure), TRUE);
      ligma_save_procedure_set_support_xmp       (LIGMA_SAVE_PROCEDURE (procedure), TRUE);
      ligma_save_procedure_set_support_profile   (LIGMA_SAVE_PROCEDURE (procedure), TRUE);
      ligma_save_procedure_set_support_thumbnail (LIGMA_SAVE_PROCEDURE (procedure), TRUE);
      ligma_save_procedure_set_support_comment   (LIGMA_SAVE_PROCEDURE (procedure), TRUE);
    }

  return procedure;
}

static LigmaValueArray *
jpeg_load (LigmaProcedure        *procedure,
           LigmaRunMode           run_mode,
           GFile                *file,
           const LigmaValueArray *args,
           gpointer              run_data)
{
  LigmaValueArray *return_vals;
  LigmaImage      *image;
  gboolean        resolution_loaded = FALSE;
  GError         *error             = NULL;

  gegl_init (NULL, NULL);

  preview_image = NULL;
  preview_layer = NULL;

  switch (run_mode)
    {
    case LIGMA_RUN_INTERACTIVE:
    case LIGMA_RUN_WITH_LAST_VALS:
      ligma_ui_init (PLUG_IN_BINARY);
      break;

    default:
      break;
    }

  image = load_image (file, run_mode, FALSE,
                      &resolution_loaded, &error);

  if (image)
    {
      LigmaMetadata *metadata;

      metadata = ligma_image_metadata_load_prepare (image, "image/jpeg",
                                                   file, NULL);

      if (metadata)
        {
          LigmaMetadataLoadFlags flags = LIGMA_METADATA_LOAD_ALL;

          if (resolution_loaded)
            flags &= ~LIGMA_METADATA_LOAD_RESOLUTION;

          ligma_image_metadata_load_finish (image, "image/jpeg",
                                           metadata, flags);

          g_object_unref (metadata);
        }
    }

  if (! image)
    return ligma_procedure_new_return_values (procedure,
                                             LIGMA_PDB_EXECUTION_ERROR,
                                             error);

  return_vals = ligma_procedure_new_return_values (procedure,
                                                  LIGMA_PDB_SUCCESS,
                                                  NULL);

  LIGMA_VALUES_SET_IMAGE (return_vals, 1, image);

  return return_vals;
}

static LigmaValueArray *
jpeg_load_thumb (LigmaProcedure        *procedure,
                 GFile                *file,
                 gint                  size,
                 const LigmaValueArray *args,
                 gpointer              run_data)
{
  LigmaValueArray *return_vals;
  LigmaImage      *image;
  gint            width  = 0;
  gint            height = 0;
  LigmaImageType   type   = -1;
  GError         *error  = NULL;

  gegl_init (NULL, NULL);

  preview_image = NULL;
  preview_layer = NULL;

  image = load_thumbnail_image (file, &width, &height, &type,
                                &error);


  if (! image)
    return ligma_procedure_new_return_values (procedure,
                                             LIGMA_PDB_EXECUTION_ERROR,
                                             error);

  return_vals = ligma_procedure_new_return_values (procedure,
                                                  LIGMA_PDB_SUCCESS,
                                                  NULL);

  LIGMA_VALUES_SET_IMAGE (return_vals, 1, image);
  LIGMA_VALUES_SET_INT   (return_vals, 2, width);
  LIGMA_VALUES_SET_INT   (return_vals, 3, height);
  LIGMA_VALUES_SET_ENUM  (return_vals, 4, type);
  LIGMA_VALUES_SET_INT   (return_vals, 5, 1); /* 1 layer */

  return return_vals;
}

static LigmaValueArray *
jpeg_save (LigmaProcedure        *procedure,
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
  LigmaMetadata          *metadata;
  LigmaImage             *orig_image;
  LigmaExportReturn       export = LIGMA_EXPORT_CANCEL;
  GError                *error  = NULL;

  gint                   orig_num_quant_tables = -1;
  gint                   orig_quality          = -1;
  JpegSubsampling        orig_subsmp           = JPEG_SUBSAMPLING_2x2_1x1_1x1;

  gegl_init (NULL, NULL);

  config = ligma_procedure_create_config (procedure);
  metadata = ligma_procedure_config_begin_export (config, image, run_mode,
                                                 args, "image/jpeg");
  preview_image = NULL;
  preview_layer = NULL;

  orig_image = image;

  switch (run_mode)
    {
    case LIGMA_RUN_INTERACTIVE:
    case LIGMA_RUN_WITH_LAST_VALS:
      ligma_ui_init (PLUG_IN_BINARY);

      export = ligma_export_image (&image, &n_drawables, &drawables, "JPEG",
                                  LIGMA_EXPORT_CAN_HANDLE_RGB |
                                  LIGMA_EXPORT_CAN_HANDLE_GRAY);

      switch (export)
        {
        case LIGMA_EXPORT_EXPORT:
          {
            gchar *tmp = g_filename_from_utf8 (_("Export Preview"), -1,
                                               NULL, NULL, NULL);
            if (tmp)
              {
                GFile *file = g_file_new_for_path (tmp);
                ligma_image_set_file (image, file);
                g_object_unref (file);
                g_free (tmp);
              }

            display = NULL;
            separate_display = TRUE;
          }
          break;

        case LIGMA_EXPORT_IGNORE:
          break;

        case LIGMA_EXPORT_CANCEL:
          return ligma_procedure_new_return_values (procedure,
                                                   LIGMA_PDB_CANCEL,
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

      return ligma_procedure_new_return_values (procedure,
                                               LIGMA_PDB_CALLING_ERROR,
                                               error);
    }

  /* Override preferences from JPG export defaults (if saved). */

  switch (run_mode)
    {
    case LIGMA_RUN_NONINTERACTIVE:
      g_object_set (config, "show-preview", FALSE, NULL);
      break;

    case LIGMA_RUN_INTERACTIVE:
    case LIGMA_RUN_WITH_LAST_VALS:
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

  if (run_mode == LIGMA_RUN_INTERACTIVE)
    {
      gboolean show_preview = FALSE;

      g_object_get (config, "show-preview", &show_preview, NULL);
      if (show_preview)
        {
          /* we freeze undo saving so that we can avoid sucking up
           * tile cache with our unneeded preview steps. */
          ligma_image_undo_freeze (image);

          undo_touched = TRUE;
        }

      /* prepare for the preview */
      preview_image     = image;
      orig_image_global = orig_image;
      drawable_global   = drawables[0];

      /*  First acquire information with a dialog  */
      if (! save_dialog (procedure, config, drawables[0], orig_image))
        {
          status = LIGMA_PDB_CANCEL;
        }

      if (undo_touched)
        {
          /* thaw undo saving and flush the displays to have them
           * reflect the current shortcuts
           */
          ligma_image_undo_thaw (image);
          ligma_displays_flush ();
        }
    }

  if (status == LIGMA_PDB_SUCCESS)
    {
      if (! save_image (file, config,
                        image, drawables[0], orig_image, FALSE,
                        &error))
        {
          status = LIGMA_PDB_EXECUTION_ERROR;
        }
    }

  if (status == LIGMA_PDB_SUCCESS)
    {
      if (metadata)
        ligma_metadata_set_bits_per_sample (metadata, 8);
    }

  ligma_procedure_config_end_export (config, image, file, status);
  g_object_unref (config);

  if (export == LIGMA_EXPORT_EXPORT)
    {
      /* If the image was exported, delete the new display. This also
       * deletes the image.
       */
      if (display)
        {
          ligma_display_delete (display);
          ligma_display_present (ligma_default_display ());
        }
      else
        {
          ligma_image_delete (image);
        }

      g_free (drawables);
    }

  return ligma_procedure_new_return_values (procedure, status, error);
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
