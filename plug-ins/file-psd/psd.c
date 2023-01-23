/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * GIMP PSD Plug-in
 * Copyright 2007 by John Marshall
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

#include <string.h>

#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

#include "psd.h"
#include "psd-load.h"
#include "psd-save.h"
#include "psd-thumb-load.h"

#include "libgimp/stdplugins-intl.h"


typedef struct _Psd      Psd;
typedef struct _PsdClass PsdClass;

struct _Psd
{
  GimpPlugIn      parent_instance;
};

struct _PsdClass
{
  GimpPlugInClass parent_class;
};


#define PSD_TYPE  (psd_get_type ())
#define PSD (obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), PSD_TYPE, Psd))

GType                   psd_get_type         (void) G_GNUC_CONST;

static GList          * psd_query_procedures (GimpPlugIn           *plug_in);
static GimpProcedure  * psd_create_procedure (GimpPlugIn           *plug_in,
                                              const gchar          *name);

static GimpValueArray * psd_load             (GimpProcedure        *procedure,
                                              GimpRunMode           run_mode,
                                              GFile                *file,
                                              const GimpValueArray *args,
                                              gpointer              run_data);
static GimpValueArray * psd_load_thumb       (GimpProcedure        *procedure,
                                              GFile                *file,
                                              gint                  size,
                                              const GimpValueArray *args,
                                              gpointer              run_data);
static GimpValueArray * psd_save             (GimpProcedure        *procedure,
                                              GimpRunMode           run_mode,
                                              GimpImage            *image,
                                              gint                  n_drawables,
                                              GimpDrawable        **drawables,
                                              GFile                *file,
                                              const GimpValueArray *args,
                                              gpointer              run_data);


G_DEFINE_TYPE (Psd, psd, GIMP_TYPE_PLUG_IN)

GIMP_MAIN (PSD_TYPE)
DEFINE_STD_SET_I18N


static void
psd_class_init (PsdClass *klass)
{
  GimpPlugInClass *plug_in_class = GIMP_PLUG_IN_CLASS (klass);

  plug_in_class->query_procedures = psd_query_procedures;
  plug_in_class->create_procedure = psd_create_procedure;
  plug_in_class->set_i18n         = STD_SET_I18N;
}

static void
psd_init (Psd *psd)
{
}

static GList *
psd_query_procedures (GimpPlugIn *plug_in)
{
  GList *list = NULL;

  list = g_list_append (list, g_strdup (LOAD_THUMB_PROC));
  list = g_list_append (list, g_strdup (LOAD_PROC));
  list = g_list_append (list, g_strdup (LOAD_MERGED_PROC));
  list = g_list_append (list, g_strdup (SAVE_PROC));

  return list;
}

static GimpProcedure *
psd_create_procedure (GimpPlugIn  *plug_in,
                      const gchar *name)
{
  GimpProcedure *procedure = NULL;

  if (! strcmp (name, LOAD_PROC))
    {
      procedure = gimp_load_procedure_new (plug_in, name,
                                           GIMP_PDB_PROC_TYPE_PLUGIN,
                                           psd_load, NULL, NULL);

      gimp_procedure_set_menu_label (procedure, _("Photoshop image"));

      gimp_procedure_set_documentation (procedure,
                                        "Loads images from the Photoshop "
                                        "PSD and PSB file formats",
                                        "This plug-in loads images in Adobe "
                                        "Photoshop (TM) native PSD and PSB format.",
                                        name);
      gimp_procedure_set_attribution (procedure,
                                      "John Marshall",
                                      "John Marshall",
                                      "2007");

      gimp_file_procedure_set_mime_types (GIMP_FILE_PROCEDURE (procedure),
                                          "image/x-psd");
      gimp_file_procedure_set_extensions (GIMP_FILE_PROCEDURE (procedure),
                                          "psd, psb");
      gimp_file_procedure_set_magics (GIMP_FILE_PROCEDURE (procedure),
                                      "0,string,8BPS");

      gimp_load_procedure_set_thumbnail_loader (GIMP_LOAD_PROCEDURE (procedure),
                                                LOAD_THUMB_PROC);
    }
  else if (! strcmp (name, LOAD_MERGED_PROC))
    {
      procedure = gimp_load_procedure_new (plug_in, name,
                                           GIMP_PDB_PROC_TYPE_PLUGIN,
                                           psd_load, NULL, NULL);

      gimp_procedure_set_menu_label (procedure, _("Photoshop image (merged)"));

      gimp_procedure_set_documentation (procedure,
                                        "Loads images from the Photoshop "
                                        "PSD and PSB file formats",
                                        "This plug-in loads the merged image "
                                        "data in Adobe Photoshop (TM) native "
                                        "PSD and PSB format.",
                                        name);
      gimp_procedure_set_attribution (procedure,
                                      "Ell",
                                      "Ell",
                                      "2018");

      gimp_file_procedure_set_mime_types (GIMP_FILE_PROCEDURE (procedure),
                                          "image/x-psd");
      gimp_file_procedure_set_extensions (GIMP_FILE_PROCEDURE (procedure),
                                          "psd, psb");
      gimp_file_procedure_set_magics (GIMP_FILE_PROCEDURE (procedure),
                                      "0,string,8BPS");
      gimp_file_procedure_set_priority (GIMP_FILE_PROCEDURE (procedure), +1);

      gimp_load_procedure_set_thumbnail_loader (GIMP_LOAD_PROCEDURE (procedure),
                                                LOAD_THUMB_PROC);
    }
  else if (! strcmp (name, LOAD_THUMB_PROC))
    {
      procedure = gimp_thumbnail_procedure_new (plug_in, name,
                                                GIMP_PDB_PROC_TYPE_PLUGIN,
                                                psd_load_thumb, NULL, NULL);

      gimp_procedure_set_documentation (procedure,
                                        "Loads thumbnails from the "
                                        "Photoshop PSD file format",
                                        "This plug-in loads thumbnail images "
                                        "from Adobe Photoshop (TM) native "
                                        "PSD format files.",
                                        name);
      gimp_procedure_set_attribution (procedure,
                                      "John Marshall",
                                      "John Marshall",
                                      "2007");
    }
  else if (! strcmp (name, SAVE_PROC))
    {
      procedure = gimp_save_procedure_new (plug_in, name,
                                           GIMP_PDB_PROC_TYPE_PLUGIN,
                                           psd_save, NULL, NULL);

      gimp_procedure_set_image_types (procedure, "*");

      gimp_procedure_set_menu_label (procedure, _("Photoshop image"));

      gimp_procedure_set_documentation (procedure,
                                        "Saves files in the Photoshop(tm) "
                                        "PSD file format",
                                        "This filter saves files of Adobe "
                                        "Photoshop(tm) native PSD format. "
                                        "These files may be of any image type "
                                        "supported by GIMP, with or without "
                                        "layers, layer masks, aux channels "
                                        "and guides.",
                                        name);
      gimp_procedure_set_attribution (procedure,
                                      "Monigotes",
                                      "Monigotes",
                                      "2000");

      gimp_file_procedure_set_mime_types (GIMP_FILE_PROCEDURE (procedure),
                                          "image/x-psd");
      gimp_file_procedure_set_extensions (GIMP_FILE_PROCEDURE (procedure),
                                          "psd");

      GIMP_PROC_ARG_BOOLEAN (procedure, "clippingpath",
                             _("Assign a Clipping _Path"),
                             _("Select a path to be the "
                             "clipping path"),
                             FALSE,
                             G_PARAM_READWRITE);

      GIMP_PROC_ARG_STRING (procedure, "clippingpathname",
                            _("Clipping Path _Name"),
                            _("Clipping path name\n"
                            "(ignored if no clipping path)"),
                            NULL,
                            G_PARAM_READWRITE);

      GIMP_PROC_ARG_DOUBLE (procedure, "clippingpathflatness",
                            _("Path _Flatness"),
                            _("Clipping path flatness in device pixels\n"
                            "(ignored if no clipping path)"),
                            0.0, 100.0, 0.2,
                            G_PARAM_READWRITE);

      GIMP_PROC_ARG_BOOLEAN (procedure, "cmyk",
                             _("Export as _CMYK"),
                             _("Export a CMYK PSD image using the soft-proofing color profile"),
                             FALSE,
                             G_PARAM_READWRITE);

      GIMP_PROC_ARG_BOOLEAN (procedure, "duotone",
                             _("Export as _Duotone"),
                             _("Export as a Duotone PSD file if Duotone color space information "
                             "was attached to the image when originally imported."),
                             FALSE,
                             G_PARAM_READWRITE);
    }

  return procedure;
}

static GimpValueArray *
psd_load (GimpProcedure        *procedure,
          GimpRunMode           run_mode,
          GFile                *file,
          const GimpValueArray *args,
          gpointer              run_data)
{
  GimpValueArray *return_vals;
  gboolean        resolution_loaded = FALSE;
  gboolean        profile_loaded    = FALSE;
  GimpImage      *image;
  GimpMetadata   *metadata;
  GimpParasite   *parasite = NULL;
  GError         *error = NULL;
  PSDSupport      unsupported_features;

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

  image = load_image (file,
                      strcmp (gimp_procedure_get_name (procedure),
                              LOAD_MERGED_PROC) == 0,
                      &resolution_loaded,
                      &profile_loaded,
                      &unsupported_features,
                      &error);

  if (! image)
    return gimp_procedure_new_return_values (procedure,
                                             GIMP_PDB_EXECUTION_ERROR,
                                             error);

  /* If image was Duotone, notify user of compatibility */
  if (run_mode == GIMP_RUN_INTERACTIVE)
    {
      parasite = gimp_image_get_parasite (image, PSD_PARASITE_DUOTONE_DATA);
      if (parasite)
        unsupported_features.duotone_mode = TRUE;

      if (unsupported_features.duotone_mode ||
          unsupported_features.show_gui)
        load_dialog (&unsupported_features);

      if (parasite)
        gimp_parasite_free (parasite);
    }

  metadata = gimp_image_metadata_load_prepare (image, "image/x-psd",
                                               file, NULL);
  if (metadata)
    {
      GimpMetadataLoadFlags flags = GIMP_METADATA_LOAD_ALL;

      if (resolution_loaded)
        flags &= ~GIMP_METADATA_LOAD_RESOLUTION;

      if (profile_loaded)
        flags &= ~GIMP_METADATA_LOAD_COLORSPACE;

      gimp_image_metadata_load_finish (image, "image/x-psd",
                                       metadata, flags);

      g_object_unref (metadata);
    }

  return_vals = gimp_procedure_new_return_values (procedure,
                                                  GIMP_PDB_SUCCESS,
                                                  NULL);

  GIMP_VALUES_SET_IMAGE (return_vals, 1, image);

  return return_vals;
}

static GimpValueArray *
psd_load_thumb (GimpProcedure        *procedure,
                GFile                *file,
                gint                  size,
                const GimpValueArray *args,
                gpointer              run_data)
{
  GimpValueArray *return_vals;
  gint            width  = 0;
  gint            height = 0;
  GimpImage      *image;
  GError         *error = NULL;

  gegl_init (NULL, NULL);

  image = load_thumbnail_image (file, &width, &height, &error);

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

  gimp_value_array_truncate (return_vals, 4);

  return return_vals;
}

static GimpValueArray *
psd_save (GimpProcedure        *procedure,
          GimpRunMode           run_mode,
          GimpImage            *image,
          gint                  n_drawables,
          GimpDrawable        **drawables,
          GFile                *file,
          const GimpValueArray *args,
          gpointer              run_data)
{
  GimpProcedureConfig   *config;
  GimpPDBStatusType      status = GIMP_PDB_SUCCESS;
  GimpMetadata          *metadata;
  GimpExportReturn       export = GIMP_EXPORT_IGNORE;
  GError                *error = NULL;

  gegl_init (NULL, NULL);

  config = gimp_procedure_create_config (procedure);
  metadata = gimp_procedure_config_begin_export (config, image, run_mode,
                                                 args, "image/x-psd");

  switch (run_mode)
    {
    case GIMP_RUN_INTERACTIVE:
    case GIMP_RUN_WITH_LAST_VALS:
      gimp_ui_init (PLUG_IN_BINARY);

      export = gimp_export_image (&image, &n_drawables, &drawables, "PSD",
                                  GIMP_EXPORT_CAN_HANDLE_RGB     |
                                  GIMP_EXPORT_CAN_HANDLE_GRAY    |
                                  GIMP_EXPORT_CAN_HANDLE_INDEXED |
                                  GIMP_EXPORT_CAN_HANDLE_ALPHA   |
                                  GIMP_EXPORT_CAN_HANDLE_LAYERS  |
                                  GIMP_EXPORT_CAN_HANDLE_LAYER_MASKS);

      if (export == GIMP_EXPORT_CANCEL)
        return gimp_procedure_new_return_values (procedure, GIMP_PDB_CANCEL,
                                                 NULL);
      break;

    default:
      break;
    }

  if (run_mode == GIMP_RUN_INTERACTIVE)
    {
      if (! save_dialog (image, procedure, G_OBJECT (config)))
        return gimp_procedure_new_return_values (procedure, GIMP_PDB_CANCEL,
                                                 NULL);
    }

  if (save_image (file, image, G_OBJECT (config), &error))
    {
      if (metadata)
        gimp_metadata_set_bits_per_sample (metadata, 8);
    }
  else
    {
      status = GIMP_PDB_EXECUTION_ERROR;
    }

  gimp_procedure_config_end_export (config, image, file, status);
  g_object_unref (config);

  if (export == GIMP_EXPORT_EXPORT)
    {
      gimp_image_delete (image);
      g_free (drawables);
    }

  return gimp_procedure_new_return_values (procedure, status, error);
}
