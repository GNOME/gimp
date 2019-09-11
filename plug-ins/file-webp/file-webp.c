/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * file-webp - WebP file format plug-in for the GIMP
 * Copyright (C) 2015  Nathan Osman
 * Copyright (C) 2016  Ben Touchette
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

#include <string.h>

#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

#include <webp/encode.h>

#include "file-webp-dialog.h"
#include "file-webp-load.h"
#include "file-webp-save.h"
#include "file-webp.h"

#include "libgimp/stdplugins-intl.h"


typedef struct _Webp      Webp;
typedef struct _WebpClass WebpClass;

struct _Webp
{
  GimpPlugIn      parent_instance;
};

struct _WebpClass
{
  GimpPlugInClass parent_class;
};


#define WEBP_TYPE  (webp_get_type ())
#define WEBP (obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), WEBP_TYPE, Webp))

GType                   webp_get_type         (void) G_GNUC_CONST;

static GList          * webp_query_procedures (GimpPlugIn           *plug_in);
static GimpProcedure  * webp_create_procedure (GimpPlugIn           *plug_in,
                                               const gchar          *name);

static GimpValueArray * webp_load             (GimpProcedure        *procedure,
                                               GimpRunMode           run_mode,
                                               GFile                *file,
                                               const GimpValueArray *args,
                                               gpointer              run_data);
static GimpValueArray * webp_save             (GimpProcedure        *procedure,
                                               GimpRunMode           run_mode,
                                               GimpImage            *image,
                                               GimpDrawable         *drawable,
                                               GFile                *file,
                                               const GimpValueArray *args,
                                               gpointer              run_data);


G_DEFINE_TYPE (Webp, webp, GIMP_TYPE_PLUG_IN)

GIMP_MAIN (WEBP_TYPE)


static void
webp_class_init (WebpClass *klass)
{
  GimpPlugInClass *plug_in_class = GIMP_PLUG_IN_CLASS (klass);

  plug_in_class->query_procedures = webp_query_procedures;
  plug_in_class->create_procedure = webp_create_procedure;
}

static void
webp_init (Webp *webp)
{
}

static GList *
webp_query_procedures (GimpPlugIn *plug_in)
{
  GList *list = NULL;

  list = g_list_append (list, g_strdup (LOAD_PROC));
  list = g_list_append (list, g_strdup (SAVE_PROC));

  return list;
}

static GimpProcedure *
webp_create_procedure (GimpPlugIn  *plug_in,
                      const gchar *name)
{
  GimpProcedure *procedure = NULL;

  if (! strcmp (name, LOAD_PROC))
    {
      procedure = gimp_load_procedure_new (plug_in, name,
                                           GIMP_PDB_PROC_TYPE_PLUGIN,
                                           webp_load, NULL, NULL);

      gimp_procedure_set_menu_label (procedure, N_("WebP image"));

      gimp_procedure_set_documentation (procedure,
                                        "Loads images in the WebP file format",
                                        "Loads images in the WebP file format",
                                        name);
      gimp_procedure_set_attribution (procedure,
                                      "Nathan Osman, Ben Touchette",
                                      "(C) 2015-2016 Nathan Osman, "
                                      "(C) 2016 Ben Touchette",
                                      "2015,2016");

      gimp_file_procedure_set_mime_types (GIMP_FILE_PROCEDURE (procedure),
                                          "image/webp");
      gimp_file_procedure_set_extensions (GIMP_FILE_PROCEDURE (procedure),
                                          "webp");
      gimp_file_procedure_set_magics (GIMP_FILE_PROCEDURE (procedure),
                                      "8,string,WEBP");
    }
  else if (! strcmp (name, SAVE_PROC))
    {
      procedure = gimp_save_procedure_new (plug_in, name,
                                           GIMP_PDB_PROC_TYPE_PLUGIN,
                                           webp_save, NULL, NULL);

      gimp_procedure_set_image_types (procedure, "*");

      gimp_procedure_set_menu_label (procedure, N_("WebP image"));

      gimp_procedure_set_documentation (procedure,
                                        "Saves files in the WebP image format",
                                        "Saves files in the WebP image format",
                                        name);
      gimp_procedure_set_attribution (procedure,
                                      "Nathan Osman, Ben Touchette",
                                      "(C) 2015-2016 Nathan Osman, "
                                      "(C) 2016 Ben Touchette",
                                      "2015,2016");

      gimp_file_procedure_set_mime_types (GIMP_FILE_PROCEDURE (procedure),
                                          "image/webp");
      gimp_file_procedure_set_extensions (GIMP_FILE_PROCEDURE (procedure),
                                          "webp");

      GIMP_PROC_ARG_INT (procedure, "preset",
                         "Preset",
                         "Preset (Default=0, Picture=1, Photo=2, Drawing=3, "
                         "Icon=4, Text=5)",
                         0, 5, WEBP_PRESET_DEFAULT,
                         G_PARAM_READWRITE);

      GIMP_PROC_ARG_BOOLEAN (procedure, "lossless",
                             "Lossless",
                             "Use lossless encoding",
                             FALSE,
                             G_PARAM_READWRITE);

      GIMP_PROC_ARG_DOUBLE (procedure, "quality",
                            "Quality",
                            "Quality of the image",
                            0, 100, 90,
                            G_PARAM_READWRITE);

      GIMP_PROC_ARG_DOUBLE (procedure, "alpha-quality",
                            "Alpha quality",
                            "Quality of the image's alpha channel",
                            0, 100, 100,
                            G_PARAM_READWRITE);

      GIMP_PROC_ARG_BOOLEAN (procedure, "animation",
                             "Animation",
                             "Use layers for animation",
                             FALSE,
                             G_PARAM_READWRITE);

      GIMP_PROC_ARG_BOOLEAN (procedure, "animation-loop",
                             "Animation loop",
                             "Loop animation infinitely",
                             TRUE,
                             G_PARAM_READWRITE);

      GIMP_PROC_ARG_BOOLEAN (procedure, "minimize-size",
                             "Minimize size",
                             "Minimize animation size",
                             TRUE,
                             G_PARAM_READWRITE);

      GIMP_PROC_ARG_INT (procedure, "kf-distance",
                         "KF distance",
                         "Maximum distance between key-frames",
                         0, G_MAXINT, 50,
                         G_PARAM_READWRITE);

      GIMP_PROC_ARG_BOOLEAN (procedure, "exif",
                             "EXIF",
                             "Toggle saving exif data",
                             FALSE,
                             G_PARAM_READWRITE);

      GIMP_PROC_ARG_BOOLEAN (procedure, "iptc",
                             "IPTC",
                             "Toggle saving iptc data",
                             FALSE,
                             G_PARAM_READWRITE);

      GIMP_PROC_ARG_BOOLEAN (procedure, "xmp",
                             "XMP",
                             "Toggle saving xmp data",
                             FALSE,
                             G_PARAM_READWRITE);

      GIMP_PROC_ARG_INT (procedure, "delay",
                         "Delay",
                         "Delay to use when timestamps are not available "
                         "or forced",
                         0, G_MAXINT, 200,
                         G_PARAM_READWRITE);

      GIMP_PROC_ARG_BOOLEAN (procedure, "force-delay",
                             "Force delay",
                             "Force delay on all frames",
                             FALSE,
                             G_PARAM_READWRITE);
    }

  return procedure;
}

static GimpValueArray *
webp_load (GimpProcedure        *procedure,
          GimpRunMode           run_mode,
          GFile                *file,
          const GimpValueArray *args,
          gpointer              run_data)
{
  GimpValueArray    *return_vals;
  GimpPDBStatusType  status = GIMP_PDB_SUCCESS;
  GimpImage         *image;
  GError            *error = NULL;

  INIT_I18N ();
  gegl_init (NULL, NULL);

  image = load_image (file, FALSE, &error);

  if (! image)
    return gimp_procedure_new_return_values (procedure, status, error);

  return_vals = gimp_procedure_new_return_values (procedure,
                                                  GIMP_PDB_SUCCESS,
                                                  NULL);

  GIMP_VALUES_SET_IMAGE (return_vals, 1, image);

  return return_vals;
}

static GimpValueArray *
webp_save (GimpProcedure        *procedure,
          GimpRunMode           run_mode,
          GimpImage            *image,
          GimpDrawable         *drawable,
          GFile                *file,
          const GimpValueArray *args,
          gpointer              run_data)
{
  GimpPDBStatusType      status   = GIMP_PDB_SUCCESS;
  GimpMetadata          *metadata = NULL;
  GimpMetadataSaveFlags  metadata_flags;
  WebPSaveParams         params;
  GimpExportReturn       export   = GIMP_EXPORT_CANCEL;
  GError                *error    = NULL;

  INIT_I18N ();
  gegl_init (NULL, NULL);

  if (run_mode == GIMP_RUN_INTERACTIVE ||
      run_mode == GIMP_RUN_WITH_LAST_VALS)
    gimp_ui_init (PLUG_IN_BINARY, FALSE);

  /* Default settings */
  params.preset        = WEBP_PRESET_DEFAULT;
  params.lossless      = FALSE;
  params.animation     = FALSE;
  params.loop          = TRUE;
  params.minimize_size = TRUE;
  params.kf_distance   = 50;
  params.quality       = 90.0f;
  params.alpha_quality = 100.0f;
  params.exif          = FALSE;
  params.iptc          = FALSE;
  params.xmp           = FALSE;
  params.delay         = 200;
  params.force_delay   = FALSE;

  /* Override the defaults with preferences. */
  metadata = gimp_image_metadata_save_prepare (image,
                                               "image/webp",
                                               &metadata_flags);
  params.exif    = (metadata_flags & GIMP_METADATA_SAVE_EXIF) != 0;
  params.xmp     = (metadata_flags & GIMP_METADATA_SAVE_XMP) != 0;
  params.iptc    = (metadata_flags & GIMP_METADATA_SAVE_IPTC) != 0;
  params.profile = (metadata_flags & GIMP_METADATA_SAVE_COLOR_PROFILE) != 0;

  switch (run_mode)
    {
    case GIMP_RUN_WITH_LAST_VALS:
      /*  Possibly override with session data  */
      gimp_get_data (SAVE_PROC, &params);
      break;

    case GIMP_RUN_INTERACTIVE:
      /*  Possibly override with session data  */
      gimp_get_data (SAVE_PROC, &params);

      if (! save_dialog (&params, image))
        return gimp_procedure_new_return_values (procedure,
                                                 GIMP_PDB_CANCEL,
                                                 NULL);
      break;

    case GIMP_RUN_NONINTERACTIVE:
      params.preset        = GIMP_VALUES_GET_INT     (args, 0);
      params.lossless      = GIMP_VALUES_GET_BOOLEAN (args, 1);
      params.quality       = GIMP_VALUES_GET_DOUBLE  (args, 2);
      params.alpha_quality = GIMP_VALUES_GET_DOUBLE  (args, 3);
      params.animation     = GIMP_VALUES_GET_BOOLEAN (args, 4);
      params.loop          = GIMP_VALUES_GET_BOOLEAN (args, 5);
      params.minimize_size = GIMP_VALUES_GET_BOOLEAN (args, 6);
      params.kf_distance   = GIMP_VALUES_GET_INT     (args, 7);
      params.exif          = GIMP_VALUES_GET_BOOLEAN (args, 8);
      params.iptc          = GIMP_VALUES_GET_BOOLEAN (args, 9);
      params.xmp           = GIMP_VALUES_GET_BOOLEAN (args, 10);
      params.delay         = GIMP_VALUES_GET_INT     (args, 11);
      params.force_delay   = GIMP_VALUES_GET_BOOLEAN (args, 12);
      break;

    default:
      break;
    }

  if (run_mode == GIMP_RUN_INTERACTIVE ||
      run_mode == GIMP_RUN_WITH_LAST_VALS)
    {
      GimpExportCapabilities capabilities = (GIMP_EXPORT_CAN_HANDLE_RGB     |
                                             GIMP_EXPORT_CAN_HANDLE_GRAY    |
                                             GIMP_EXPORT_CAN_HANDLE_INDEXED |
                                             GIMP_EXPORT_CAN_HANDLE_ALPHA);

      if (params.animation)
        capabilities |= GIMP_EXPORT_CAN_HANDLE_LAYERS_AS_ANIMATION;

      export = gimp_export_image (&image, &drawable, "WebP",
                                  capabilities);

      if (export == GIMP_EXPORT_CANCEL)
        return gimp_procedure_new_return_values (procedure,
                                                 GIMP_PDB_CANCEL,
                                                 NULL);
    }

  if (! save_image (file, image, drawable,
                    metadata, metadata_flags,
                    &params,
                    &error))
    {
      status = GIMP_PDB_EXECUTION_ERROR;
    }

  if (export == GIMP_EXPORT_EXPORT)
    gimp_image_delete (image);

  if (metadata)
    g_object_unref (metadata);

  if (status == GIMP_PDB_SUCCESS)
    {
      /* save parameters for later */
      gimp_set_data (SAVE_PROC, &params, sizeof (params));
    }

  return gimp_procedure_new_return_values (procedure, status, error);
}
