/*
 * pat plug-in version 1.01
 * Loads/exports version 1 LIGMA .pat files, by Tim Newsome <drz@frody.bloke.com>
 *
 * LIGMA - The GNU Image Manipulation Program
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

#include <libligma/ligma.h>
#include <libligma/ligmaui.h>

#include "libligma/stdplugins-intl.h"


#define SAVE_PROC      "file-pat-save"
#define PLUG_IN_BINARY "file-pat"


typedef struct _Pat      Pat;
typedef struct _PatClass PatClass;

struct _Pat
{
  LigmaPlugIn      parent_instance;
};

struct _PatClass
{
  LigmaPlugInClass parent_class;
};


#define PAT_TYPE  (pat_get_type ())
#define PAT (obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), PAT_TYPE, Pat))

GType                   pat_get_type         (void) G_GNUC_CONST;

static GList          * pat_query_procedures (LigmaPlugIn           *plug_in);
static LigmaProcedure  * pat_create_procedure (LigmaPlugIn           *plug_in,
                                              const gchar          *name);

static LigmaValueArray * pat_save             (LigmaProcedure        *procedure,
                                              LigmaRunMode           run_mode,
                                              LigmaImage            *image,
                                              gint                  n_drawables,
                                              LigmaDrawable        **drawables,
                                              GFile                *file,
                                              const LigmaValueArray *args,
                                              gpointer              run_data);

static gboolean         save_dialog          (LigmaProcedure        *procedure,
                                              GObject              *config);


G_DEFINE_TYPE (Pat, pat, LIGMA_TYPE_PLUG_IN)

LIGMA_MAIN (PAT_TYPE)
DEFINE_STD_SET_I18N


static void
pat_class_init (PatClass *klass)
{
  LigmaPlugInClass *plug_in_class = LIGMA_PLUG_IN_CLASS (klass);

  plug_in_class->query_procedures = pat_query_procedures;
  plug_in_class->create_procedure = pat_create_procedure;
  plug_in_class->set_i18n         = STD_SET_I18N;
}

static void
pat_init (Pat *pat)
{
}

static GList *
pat_query_procedures (LigmaPlugIn *plug_in)
{
  return g_list_append (NULL, g_strdup (SAVE_PROC));
}

static LigmaProcedure *
pat_create_procedure (LigmaPlugIn  *plug_in,
                      const gchar *name)
{
  LigmaProcedure *procedure = NULL;

  if (! strcmp (name, SAVE_PROC))
    {
      procedure = ligma_save_procedure_new (plug_in, name,
                                           LIGMA_PDB_PROC_TYPE_PLUGIN,
                                           pat_save, NULL, NULL);

      ligma_procedure_set_image_types (procedure, "*");

      ligma_procedure_set_menu_label (procedure, _("LIGMA pattern"));
      ligma_procedure_set_icon_name (procedure, LIGMA_ICON_PATTERN);

      ligma_procedure_set_documentation (procedure,
                                        "Exports Ligma pattern file (.PAT)",
                                        "New Ligma patterns can be created "
                                        "by exporting them in the "
                                        "appropriate place with this plug-in.",
                                        SAVE_PROC);
      ligma_procedure_set_attribution (procedure,
                                      "Tim Newsome",
                                      "Tim Newsome",
                                      "1997");

      ligma_file_procedure_set_mime_types (LIGMA_FILE_PROCEDURE (procedure),
                                          "image/x-ligma-pat");
      ligma_file_procedure_set_extensions (LIGMA_FILE_PROCEDURE (procedure),
                                          "pat");
      ligma_file_procedure_set_handles_remote (LIGMA_FILE_PROCEDURE (procedure),
                                              TRUE);

      LIGMA_PROC_ARG_STRING (procedure, "description",
                            "Description",
                            "Short description of the pattern",
                            "LIGMA Pattern",
                            LIGMA_PARAM_READWRITE);
    }

  return procedure;
}

static LigmaValueArray *
pat_save (LigmaProcedure        *procedure,
          LigmaRunMode           run_mode,
          LigmaImage            *image,
          gint                  n_drawables,
          LigmaDrawable        **drawables,
          GFile                *file,
          const LigmaValueArray *args,
          gpointer              run_data)
{
  LigmaProcedureConfig *config;
  LigmaPDBStatusType    status = LIGMA_PDB_SUCCESS;
  LigmaExportReturn     export = LIGMA_EXPORT_CANCEL;
  gchar               *description;
  GError              *error  = NULL;

  config = ligma_procedure_create_config (procedure);
  ligma_procedure_config_begin_run (config, image, run_mode, args);

  g_object_get (config,
                "description", &description,
                NULL);

  if (! description || ! strlen (description))
    {
      gchar *name = g_path_get_basename (ligma_file_get_utf8_name (file));

      if (g_str_has_suffix (name, ".pat"))
        name[strlen (name) - 4] = '\0';

      if (strlen (name))
        g_object_set (config,
                      "description", name,
                      NULL);

      g_free (name);
    }

  g_free (description);

  switch (run_mode)
    {
    case LIGMA_RUN_INTERACTIVE:
    case LIGMA_RUN_WITH_LAST_VALS:
      ligma_ui_init (PLUG_IN_BINARY);

      export = ligma_export_image (&image, &n_drawables, &drawables, "PAT",
                                  LIGMA_EXPORT_CAN_HANDLE_GRAY    |
                                  LIGMA_EXPORT_CAN_HANDLE_RGB     |
                                  LIGMA_EXPORT_CAN_HANDLE_INDEXED |
                                  LIGMA_EXPORT_CAN_HANDLE_ALPHA);

      if (export == LIGMA_EXPORT_CANCEL)
        return ligma_procedure_new_return_values (procedure, LIGMA_PDB_CANCEL,
                                                 NULL);
      break;

    default:
      break;
    }

  if (run_mode == LIGMA_RUN_INTERACTIVE)
    {
      if (! save_dialog (procedure, G_OBJECT (config)))
        status = LIGMA_PDB_CANCEL;
    }

  if (status == LIGMA_PDB_SUCCESS)
    {
      LigmaValueArray *save_retvals;
      LigmaValueArray *args;

      g_object_get (config,
                    "description", &description,
                    NULL);

      args = ligma_value_array_new_from_types (NULL,
                                              LIGMA_TYPE_RUN_MODE,     LIGMA_RUN_NONINTERACTIVE,
                                              LIGMA_TYPE_IMAGE,        image,
                                              G_TYPE_INT,             n_drawables,
                                              LIGMA_TYPE_OBJECT_ARRAY, NULL,
                                              G_TYPE_FILE,            file,
                                              G_TYPE_STRING,          description,
                                              G_TYPE_NONE);
      ligma_value_set_object_array (ligma_value_array_index (args, 3),
                                   LIGMA_TYPE_ITEM, (GObject **) drawables, n_drawables);

      save_retvals = ligma_pdb_run_procedure_array (ligma_get_pdb (),
                                                   "file-pat-save-internal",
                                                   args);
      ligma_value_array_unref (args);

      if (LIGMA_VALUES_GET_ENUM (save_retvals, 0) != LIGMA_PDB_SUCCESS)
        {
          g_set_error (&error, 0, 0,
                       "Running procedure 'file-pat-save-internal' "
                       "failed: %s",
                       ligma_pdb_get_last_error (ligma_get_pdb ()));

          status = LIGMA_PDB_EXECUTION_ERROR;
        }

      ligma_value_array_unref (save_retvals);
    }

  ligma_procedure_config_end_run (config, status);
  g_object_unref (config);

  if (export == LIGMA_EXPORT_EXPORT)
    {
      ligma_image_delete (image);
      g_free (drawables);
    }

  return ligma_procedure_new_return_values (procedure, status, error);
}

static gboolean
save_dialog (LigmaProcedure *procedure,
             GObject       *config)
{
  GtkWidget *dialog;
  GtkWidget *grid;
  GtkWidget *entry;
  gboolean   run;

  dialog = ligma_procedure_dialog_new (procedure,
                                      LIGMA_PROCEDURE_CONFIG (config),
                                      _("Export Image as Pattern"));

  grid = gtk_grid_new ();
  gtk_container_set_border_width (GTK_CONTAINER (grid), 12);
  gtk_grid_set_column_spacing (GTK_GRID (grid), 6);
  gtk_box_pack_start (GTK_BOX (gtk_dialog_get_content_area (GTK_DIALOG (dialog))),
                      grid, TRUE, TRUE, 0);
  gtk_widget_show (grid);

  entry = ligma_prop_entry_new (config, "description", 256);
  gtk_entry_set_width_chars (GTK_ENTRY (entry), 20);
  gtk_entry_set_activates_default (GTK_ENTRY (entry), TRUE);

  ligma_grid_attach_aligned (GTK_GRID (grid), 0, 0,
                            _("_Description:"), 1.0, 0.5,
                            entry, 1);

  gtk_widget_show (dialog);

  run = ligma_procedure_dialog_run (LIGMA_PROCEDURE_DIALOG (dialog));

  gtk_widget_destroy (dialog);

  return run;
}
