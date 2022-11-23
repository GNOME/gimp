/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * LIGMA PSD Plug-in
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

#include <libligma/ligma.h>
#include <libligma/ligmaui.h>

#include "psd.h"
#include "psd-load.h"
#include "psd-save.h"
#include "psd-thumb-load.h"

#include "libligma/stdplugins-intl.h"


typedef struct _Psd      Psd;
typedef struct _PsdClass PsdClass;

struct _Psd
{
  LigmaPlugIn      parent_instance;
};

struct _PsdClass
{
  LigmaPlugInClass parent_class;
};


#define PSD_TYPE  (psd_get_type ())
#define PSD (obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), PSD_TYPE, Psd))

GType                   psd_get_type         (void) G_GNUC_CONST;

static GList          * psd_query_procedures (LigmaPlugIn           *plug_in);
static LigmaProcedure  * psd_create_procedure (LigmaPlugIn           *plug_in,
                                              const gchar          *name);

static LigmaValueArray * psd_load             (LigmaProcedure        *procedure,
                                              LigmaRunMode           run_mode,
                                              GFile                *file,
                                              const LigmaValueArray *args,
                                              gpointer              run_data);
static LigmaValueArray * psd_load_thumb       (LigmaProcedure        *procedure,
                                              GFile                *file,
                                              gint                  size,
                                              const LigmaValueArray *args,
                                              gpointer              run_data);
static LigmaValueArray * psd_save             (LigmaProcedure        *procedure,
                                              LigmaRunMode           run_mode,
                                              LigmaImage            *image,
                                              gint                  n_drawables,
                                              LigmaDrawable        **drawables,
                                              GFile                *file,
                                              const LigmaValueArray *args,
                                              gpointer              run_data);


G_DEFINE_TYPE (Psd, psd, LIGMA_TYPE_PLUG_IN)

LIGMA_MAIN (PSD_TYPE)
DEFINE_STD_SET_I18N


static void
psd_class_init (PsdClass *klass)
{
  LigmaPlugInClass *plug_in_class = LIGMA_PLUG_IN_CLASS (klass);

  plug_in_class->query_procedures = psd_query_procedures;
  plug_in_class->create_procedure = psd_create_procedure;
  plug_in_class->set_i18n         = STD_SET_I18N;
}

static void
psd_init (Psd *psd)
{
}

static GList *
psd_query_procedures (LigmaPlugIn *plug_in)
{
  GList *list = NULL;

  list = g_list_append (list, g_strdup (LOAD_THUMB_PROC));
  list = g_list_append (list, g_strdup (LOAD_PROC));
  list = g_list_append (list, g_strdup (LOAD_MERGED_PROC));
  list = g_list_append (list, g_strdup (SAVE_PROC));

  return list;
}

static LigmaProcedure *
psd_create_procedure (LigmaPlugIn  *plug_in,
                      const gchar *name)
{
  LigmaProcedure *procedure = NULL;

  if (! strcmp (name, LOAD_PROC))
    {
      procedure = ligma_load_procedure_new (plug_in, name,
                                           LIGMA_PDB_PROC_TYPE_PLUGIN,
                                           psd_load, NULL, NULL);

      ligma_procedure_set_menu_label (procedure, _("Photoshop image"));

      ligma_procedure_set_documentation (procedure,
                                        "Loads images from the Photoshop "
                                        "PSD and PSB file formats",
                                        "This plug-in loads images in Adobe "
                                        "Photoshop (TM) native PSD and PSB format.",
                                        name);
      ligma_procedure_set_attribution (procedure,
                                      "John Marshall",
                                      "John Marshall",
                                      "2007");

      ligma_file_procedure_set_mime_types (LIGMA_FILE_PROCEDURE (procedure),
                                          "image/x-psd");
      ligma_file_procedure_set_extensions (LIGMA_FILE_PROCEDURE (procedure),
                                          "psd, psb");
      ligma_file_procedure_set_magics (LIGMA_FILE_PROCEDURE (procedure),
                                      "0,string,8BPS");

      ligma_load_procedure_set_thumbnail_loader (LIGMA_LOAD_PROCEDURE (procedure),
                                                LOAD_THUMB_PROC);
    }
  else if (! strcmp (name, LOAD_MERGED_PROC))
    {
      procedure = ligma_load_procedure_new (plug_in, name,
                                           LIGMA_PDB_PROC_TYPE_PLUGIN,
                                           psd_load, NULL, NULL);

      ligma_procedure_set_menu_label (procedure, _("Photoshop image (merged)"));

      ligma_procedure_set_documentation (procedure,
                                        "Loads images from the Photoshop "
                                        "PSD and PSB file formats",
                                        "This plug-in loads the merged image "
                                        "data in Adobe Photoshop (TM) native "
                                        "PSD and PSB format.",
                                        name);
      ligma_procedure_set_attribution (procedure,
                                      "Ell",
                                      "Ell",
                                      "2018");

      ligma_file_procedure_set_mime_types (LIGMA_FILE_PROCEDURE (procedure),
                                          "image/x-psd");
      ligma_file_procedure_set_extensions (LIGMA_FILE_PROCEDURE (procedure),
                                          "psd, psb");
      ligma_file_procedure_set_magics (LIGMA_FILE_PROCEDURE (procedure),
                                      "0,string,8BPS");
      ligma_file_procedure_set_priority (LIGMA_FILE_PROCEDURE (procedure), +1);

      ligma_load_procedure_set_thumbnail_loader (LIGMA_LOAD_PROCEDURE (procedure),
                                                LOAD_THUMB_PROC);
    }
  else if (! strcmp (name, LOAD_THUMB_PROC))
    {
      procedure = ligma_thumbnail_procedure_new (plug_in, name,
                                                LIGMA_PDB_PROC_TYPE_PLUGIN,
                                                psd_load_thumb, NULL, NULL);

      ligma_procedure_set_documentation (procedure,
                                        "Loads thumbnails from the "
                                        "Photoshop PSD file format",
                                        "This plug-in loads thumbnail images "
                                        "from Adobe Photoshop (TM) native "
                                        "PSD format files.",
                                        name);
      ligma_procedure_set_attribution (procedure,
                                      "John Marshall",
                                      "John Marshall",
                                      "2007");
    }
  else if (! strcmp (name, SAVE_PROC))
    {
      procedure = ligma_save_procedure_new (plug_in, name,
                                           LIGMA_PDB_PROC_TYPE_PLUGIN,
                                           psd_save, NULL, NULL);

      ligma_procedure_set_image_types (procedure, "*");

      ligma_procedure_set_menu_label (procedure, _("Photoshop image"));

      ligma_procedure_set_documentation (procedure,
                                        "Saves files in the Photoshop(tm) "
                                        "PSD file format",
                                        "This filter saves files of Adobe "
                                        "Photoshop(tm) native PSD format. "
                                        "These files may be of any image type "
                                        "supported by LIGMA, with or without "
                                        "layers, layer masks, aux channels "
                                        "and guides.",
                                        name);
      ligma_procedure_set_attribution (procedure,
                                      "Monigotes",
                                      "Monigotes",
                                      "2000");

      ligma_file_procedure_set_mime_types (LIGMA_FILE_PROCEDURE (procedure),
                                          "image/x-psd");
      ligma_file_procedure_set_extensions (LIGMA_FILE_PROCEDURE (procedure),
                                          "psd");

      LIGMA_PROC_ARG_BOOLEAN (procedure, "cmyk",
                             "Export as _CMYK",
                             "Export a CMYK PSD image using the soft-proofing color profile",
                             FALSE,
                             G_PARAM_READWRITE);

      LIGMA_PROC_ARG_BOOLEAN (procedure, "duotone",
                             "Export as _Duotone",
                             "Export as a Duotone PSD file if Duotone color space information "
                             "was attached to the image when originally imported.",
                             FALSE,
                             G_PARAM_READWRITE);
    }

  return procedure;
}

static LigmaValueArray *
psd_load (LigmaProcedure        *procedure,
          LigmaRunMode           run_mode,
          GFile                *file,
          const LigmaValueArray *args,
          gpointer              run_data)
{
  LigmaValueArray *return_vals;
  gboolean        resolution_loaded = FALSE;
  gboolean        profile_loaded    = FALSE;
  LigmaImage      *image;
  LigmaMetadata   *metadata;
  LigmaParasite   *parasite = NULL;
  GError         *error = NULL;

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

  image = load_image (file,
                      strcmp (ligma_procedure_get_name (procedure),
                              LOAD_MERGED_PROC) == 0,
                      &resolution_loaded,
                      &profile_loaded,
                      &error);

  if (! image)
    return ligma_procedure_new_return_values (procedure,
                                             LIGMA_PDB_EXECUTION_ERROR,
                                             error);

  /* If image was Duotone, notify user of compatibility */
  if (run_mode == LIGMA_RUN_INTERACTIVE)
    {
      parasite = ligma_image_get_parasite (image, PSD_PARASITE_DUOTONE_DATA);
      if (parasite)
        {
          load_dialog ();
          ligma_parasite_free (parasite);
        }
    }

  metadata = ligma_image_metadata_load_prepare (image, "image/x-psd",
                                               file, NULL);
  if (metadata)
    {
      LigmaMetadataLoadFlags flags = LIGMA_METADATA_LOAD_ALL;

      if (resolution_loaded)
        flags &= ~LIGMA_METADATA_LOAD_RESOLUTION;

      if (profile_loaded)
        flags &= ~LIGMA_METADATA_LOAD_COLORSPACE;

      ligma_image_metadata_load_finish (image, "image/x-psd",
                                       metadata, flags);

      g_object_unref (metadata);
    }

  return_vals = ligma_procedure_new_return_values (procedure,
                                                  LIGMA_PDB_SUCCESS,
                                                  NULL);

  LIGMA_VALUES_SET_IMAGE (return_vals, 1, image);

  return return_vals;
}

static LigmaValueArray *
psd_load_thumb (LigmaProcedure        *procedure,
                GFile                *file,
                gint                  size,
                const LigmaValueArray *args,
                gpointer              run_data)
{
  LigmaValueArray *return_vals;
  gint            width  = 0;
  gint            height = 0;
  LigmaImage      *image;
  GError         *error = NULL;

  gegl_init (NULL, NULL);

  image = load_thumbnail_image (file, &width, &height, &error);

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

  ligma_value_array_truncate (return_vals, 4);

  return return_vals;
}

static LigmaValueArray *
psd_save (LigmaProcedure        *procedure,
          LigmaRunMode           run_mode,
          LigmaImage            *image,
          gint                  n_drawables,
          LigmaDrawable        **drawables,
          GFile                *file,
          const LigmaValueArray *args,
          gpointer              run_data)
{
  LigmaProcedureConfig   *config;
  LigmaPDBStatusType      status = LIGMA_PDB_SUCCESS;
  LigmaMetadata          *metadata;
  LigmaExportReturn       export = LIGMA_EXPORT_IGNORE;
  GError                *error = NULL;

  gegl_init (NULL, NULL);

  config = ligma_procedure_create_config (procedure);
  metadata = ligma_procedure_config_begin_export (config, image, run_mode,
                                                 args, "image/x-psd");

  switch (run_mode)
    {
    case LIGMA_RUN_INTERACTIVE:
    case LIGMA_RUN_WITH_LAST_VALS:
      ligma_ui_init (PLUG_IN_BINARY);

      export = ligma_export_image (&image, &n_drawables, &drawables, "PSD",
                                  LIGMA_EXPORT_CAN_HANDLE_RGB     |
                                  LIGMA_EXPORT_CAN_HANDLE_GRAY    |
                                  LIGMA_EXPORT_CAN_HANDLE_INDEXED |
                                  LIGMA_EXPORT_CAN_HANDLE_ALPHA   |
                                  LIGMA_EXPORT_CAN_HANDLE_LAYERS  |
                                  LIGMA_EXPORT_CAN_HANDLE_LAYER_MASKS);

      if (export == LIGMA_EXPORT_CANCEL)
        return ligma_procedure_new_return_values (procedure, LIGMA_PDB_CANCEL,
                                                 NULL);
      break;

    default:
      break;
    }

  if (run_mode == LIGMA_RUN_INTERACTIVE)
    {
      if (! save_dialog (image, procedure, G_OBJECT (config)))
        return ligma_procedure_new_return_values (procedure, LIGMA_PDB_CANCEL,
                                                 NULL);
    }

  if (save_image (file, image, G_OBJECT (config), &error))
    {
      if (metadata)
        ligma_metadata_set_bits_per_sample (metadata, 8);
    }
  else
    {
      status = LIGMA_PDB_EXECUTION_ERROR;
    }

  ligma_procedure_config_end_export (config, image, file, status);
  g_object_unref (config);

  if (export == LIGMA_EXPORT_EXPORT)
    {
      ligma_image_delete (image);
      g_free (drawables);
    }

  return ligma_procedure_new_return_values (procedure, status, error);
}
