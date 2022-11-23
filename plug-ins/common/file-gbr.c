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

/*
 * gbr plug-in version 1.00
 * Loads/exports version 2 LIGMA .gbr files, by Tim Newsome <drz@frody.bloke.com>
 * Some bits stolen from the .99.7 source tree.
 *
 * Added in GBR version 1 support after learning that there wasn't a
 * tool to read them.
 * July 6, 1998 by Seth Burgess <sjburges@ligma.org>
 *
 * Dec 17, 2000
 * Load and save LIGMA brushes in GRAY or RGBA.  jtl + neo
 *
 *
 * TODO: Give some better error reporting on not opening files/bad headers
 *       etc.
 */

#include "config.h"

#include <libligma/ligma.h>
#include <libligma/ligmaui.h>

#include "libligma/stdplugins-intl.h"


#define SAVE_PROC      "file-gbr-save"
#define PLUG_IN_BINARY "file-gbr"


typedef struct _Gbr      Gbr;
typedef struct _GbrClass GbrClass;

struct _Gbr
{
  LigmaPlugIn      parent_instance;
};

struct _GbrClass
{
  LigmaPlugInClass parent_class;
};


#define GBR_TYPE  (gbr_get_type ())
#define GBR (obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), GBR_TYPE, Gbr))

GType                   gbr_get_type         (void) G_GNUC_CONST;

static GList          * gbr_query_procedures (LigmaPlugIn           *plug_in);
static LigmaProcedure  * gbr_create_procedure (LigmaPlugIn           *plug_in,
                                              const gchar          *name);

static LigmaValueArray * gbr_save             (LigmaProcedure        *procedure,
                                              LigmaRunMode           run_mode,
                                              LigmaImage            *image,
                                              gint                  n_drawables,
                                              LigmaDrawable        **drawables,
                                              GFile                *file,
                                              const LigmaValueArray *args,
                                              gpointer              run_data);

static gboolean         save_dialog          (LigmaProcedure        *procedure,
                                              GObject              *config);


G_DEFINE_TYPE (Gbr, gbr, LIGMA_TYPE_PLUG_IN)

LIGMA_MAIN (GBR_TYPE)
DEFINE_STD_SET_I18N


static void
gbr_class_init (GbrClass *klass)
{
  LigmaPlugInClass *plug_in_class = LIGMA_PLUG_IN_CLASS (klass);

  plug_in_class->query_procedures = gbr_query_procedures;
  plug_in_class->create_procedure = gbr_create_procedure;
  plug_in_class->set_i18n         = STD_SET_I18N;
}

static void
gbr_init (Gbr *gbr)
{
}

static GList *
gbr_query_procedures (LigmaPlugIn *plug_in)
{
  return g_list_append (NULL, g_strdup (SAVE_PROC));
}

static LigmaProcedure *
gbr_create_procedure (LigmaPlugIn  *plug_in,
                      const gchar *name)
{
  LigmaProcedure *procedure = NULL;

  if (! strcmp (name, SAVE_PROC))
    {
      procedure = ligma_save_procedure_new (plug_in, name,
                                           LIGMA_PDB_PROC_TYPE_PLUGIN,
                                           gbr_save, NULL, NULL);

      ligma_procedure_set_image_types (procedure, "*");

      ligma_procedure_set_menu_label (procedure, _("LIGMA brush"));
      ligma_procedure_set_icon_name (procedure, LIGMA_ICON_BRUSH);

      ligma_procedure_set_documentation (procedure,
                                        "Exports files in the LIGMA brush "
                                        "file format",
                                        "Exports files in the LIGMA brush "
                                        "file format",
                                        SAVE_PROC);
      ligma_procedure_set_attribution (procedure,
                                      "Tim Newsome, Jens Lautenbacher, "
                                      "Sven Neumann",
                                      "Tim Newsome, Jens Lautenbacher, "
                                      "Sven Neumann",
                                      "1997-2000");

      ligma_file_procedure_set_mime_types (LIGMA_FILE_PROCEDURE (procedure),
                                          "image/x-ligma-gbr");
      ligma_file_procedure_set_extensions (LIGMA_FILE_PROCEDURE (procedure),
                                          "gbr");
      ligma_file_procedure_set_handles_remote (LIGMA_FILE_PROCEDURE (procedure),
                                              TRUE);

      LIGMA_PROC_ARG_INT (procedure, "spacing",
                         "Spacing",
                         "Spacing of the brush",
                         1, 1000, 10,
                         LIGMA_PARAM_READWRITE);

      LIGMA_PROC_ARG_STRING (procedure, "description",
                            "Description",
                            "Short description of the brush",
                            "LIGMA Brush",
                            LIGMA_PARAM_READWRITE);
    }

  return procedure;
}

static LigmaValueArray *
gbr_save (LigmaProcedure        *procedure,
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

      if (g_str_has_suffix (name, ".gbr"))
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

      export = ligma_export_image (&image, &n_drawables, &drawables, "GBR",
                                  LIGMA_EXPORT_CAN_HANDLE_GRAY    |
                                  LIGMA_EXPORT_CAN_HANDLE_RGB     |
                                  LIGMA_EXPORT_CAN_HANDLE_INDEXED |
                                  LIGMA_EXPORT_CAN_HANDLE_ALPHA);

      if (export == LIGMA_EXPORT_CANCEL)
        return ligma_procedure_new_return_values (procedure,
                                                 LIGMA_PDB_CANCEL,
                                                 NULL);
      break;

    default:
      break;
    }

  if (n_drawables != 1)
    {
      g_set_error (&error, G_FILE_ERROR, 0,
                   _("GBR format does not support multiple layers."));

      return ligma_procedure_new_return_values (procedure,
                                               LIGMA_PDB_CALLING_ERROR,
                                               error);
    }

  if (run_mode == LIGMA_RUN_INTERACTIVE)
    {
      if (! save_dialog (procedure, G_OBJECT (config)))
        status = LIGMA_PDB_CANCEL;
    }

  if (status == LIGMA_PDB_SUCCESS)
    {
      LigmaValueArray *save_retvals;
      gint            spacing;

      g_object_get (config,
                    "description", &description,
                    "spacing",     &spacing,
                    NULL);

      save_retvals =
        ligma_pdb_run_procedure (ligma_get_pdb (),
                                "file-gbr-save-internal",
                                LIGMA_TYPE_RUN_MODE, LIGMA_RUN_NONINTERACTIVE,
                                LIGMA_TYPE_IMAGE,    image,
                                LIGMA_TYPE_DRAWABLE, drawables[0],
                                G_TYPE_FILE,        file,
                                G_TYPE_INT,         spacing,
                                G_TYPE_STRING,      description,
                                G_TYPE_NONE);

      g_free (description);

      if (LIGMA_VALUES_GET_ENUM (save_retvals, 0) != LIGMA_PDB_SUCCESS)
        {
          g_set_error (&error, 0, 0,
                       "Running procedure 'file-gbr-save-internal' "
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
  GtkWidget *scale;
  gboolean   run;

  dialog = ligma_procedure_dialog_new (procedure,
                                      LIGMA_PROCEDURE_CONFIG (config),
                                      _("Export Image as Brush"));

  /* The main grid */
  grid = gtk_grid_new ();
  gtk_container_set_border_width (GTK_CONTAINER (grid), 12);
  gtk_grid_set_row_spacing (GTK_GRID (grid), 6);
  gtk_grid_set_column_spacing (GTK_GRID (grid), 6);
  gtk_box_pack_start (GTK_BOX (gtk_dialog_get_content_area (GTK_DIALOG (dialog))),
                      grid, TRUE, TRUE, 0);
  gtk_widget_show (grid);

  entry = ligma_prop_entry_new (config, "description", 256);
  gtk_entry_set_width_chars (GTK_ENTRY (entry), 20);
  gtk_entry_set_activates_default (GTK_ENTRY (entry), TRUE);

  ligma_grid_attach_aligned (GTK_GRID (grid), 0, 0,
                            _("_Description:"), 1.0, 0.5,
                            entry, 2);

  scale = ligma_prop_scale_entry_new (config, "spacing",
                                     NULL, 1.0, FALSE, 0, 0);
  gtk_widget_hide (ligma_labeled_get_label (LIGMA_LABELED (scale)));
  ligma_grid_attach_aligned (GTK_GRID (grid), 0, 1,
                            _("_Spacing:"), 0.0, 0.5, scale, 4);

  gtk_widget_show (dialog);

  run = ligma_procedure_dialog_run (LIGMA_PROCEDURE_DIALOG (dialog));

  gtk_widget_destroy (dialog);

  return run;
}
