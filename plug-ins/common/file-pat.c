/*
 * pat plug-in version 1.01
 * Loads/exports version 1 GIMP .pat files, by Tim Newsome <drz@frody.bloke.com>
 *
 * GIMP - The GNU Image Manipulation Program
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

#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

#include "libgimp/stdplugins-intl.h"


#define SAVE_PROC      "file-pat-save"
#define PLUG_IN_BINARY "file-pat"


typedef struct _Pat      Pat;
typedef struct _PatClass PatClass;

struct _Pat
{
  GimpPlugIn      parent_instance;
};

struct _PatClass
{
  GimpPlugInClass parent_class;
};


#define PAT_TYPE  (pat_get_type ())
#define PAT (obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), PAT_TYPE, Pat))

GType                   pat_get_type         (void) G_GNUC_CONST;

static GList          * pat_query_procedures (GimpPlugIn           *plug_in);
static GimpProcedure  * pat_create_procedure (GimpPlugIn           *plug_in,
                                              const gchar          *name);

static GimpValueArray * pat_save             (GimpProcedure        *procedure,
                                              GimpRunMode           run_mode,
                                              GimpImage            *image,
                                              gint                  n_drawables,
                                              GimpDrawable        **drawables,
                                              GFile                *file,
                                              GimpMetadata         *metadata,
                                              GimpProcedureConfig  *config,
                                              gpointer              run_data);

static gboolean         save_dialog          (GimpImage            *image,
                                              GimpProcedure        *procedure,
                                              GObject              *config);


G_DEFINE_TYPE (Pat, pat, GIMP_TYPE_PLUG_IN)

GIMP_MAIN (PAT_TYPE)
DEFINE_STD_SET_I18N


static void
pat_class_init (PatClass *klass)
{
  GimpPlugInClass *plug_in_class = GIMP_PLUG_IN_CLASS (klass);

  plug_in_class->query_procedures = pat_query_procedures;
  plug_in_class->create_procedure = pat_create_procedure;
  plug_in_class->set_i18n         = STD_SET_I18N;
}

static void
pat_init (Pat *pat)
{
}

static GList *
pat_query_procedures (GimpPlugIn *plug_in)
{
  return g_list_append (NULL, g_strdup (SAVE_PROC));
}

static GimpProcedure *
pat_create_procedure (GimpPlugIn  *plug_in,
                      const gchar *name)
{
  GimpProcedure *procedure = NULL;

  if (! strcmp (name, SAVE_PROC))
    {
      procedure = gimp_save_procedure_new (plug_in, name,
                                           GIMP_PDB_PROC_TYPE_PLUGIN,
                                           FALSE, pat_save, NULL, NULL);

      gimp_procedure_set_image_types (procedure, "*");

      gimp_procedure_set_menu_label (procedure, _("GIMP pattern"));
      gimp_procedure_set_icon_name (procedure, GIMP_ICON_PATTERN);

      gimp_procedure_set_documentation (procedure,
                                        _("Exports GIMP pattern file (.PAT)"),
                                        _("New GIMP patterns can be created "
                                          "by exporting them in the "
                                          "appropriate place with this plug-in."),
                                        SAVE_PROC);
      gimp_procedure_set_attribution (procedure,
                                      "Tim Newsome",
                                      "Tim Newsome",
                                      "1997");

      gimp_file_procedure_set_format_name (GIMP_FILE_PROCEDURE (procedure),
                                           _("Pattern"));
      gimp_file_procedure_set_mime_types (GIMP_FILE_PROCEDURE (procedure),
                                          "image/x-gimp-pat");
      gimp_file_procedure_set_extensions (GIMP_FILE_PROCEDURE (procedure),
                                          "pat");
      gimp_file_procedure_set_handles_remote (GIMP_FILE_PROCEDURE (procedure),
                                              TRUE);

      GIMP_PROC_ARG_STRING (procedure, "description",
                            _("_Description"),
                            _("Short description of the pattern"),
                            _("GIMP Pattern"),
                            GIMP_PARAM_READWRITE);
    }

  return procedure;
}

static GimpValueArray *
pat_save (GimpProcedure        *procedure,
          GimpRunMode           run_mode,
          GimpImage            *image,
          gint                  n_drawables,
          GimpDrawable        **drawables,
          GFile                *file,
          GimpMetadata         *metadata,
          GimpProcedureConfig  *config,
          gpointer              run_data)
{
  GimpPDBStatusType  status = GIMP_PDB_SUCCESS;
  GimpExportReturn   export = GIMP_EXPORT_CANCEL;
  gchar             *description;
  GError            *error  = NULL;

  g_object_get (config,
                "description", &description,
                NULL);

  if (! description || ! strlen (description))
    {
      gchar *name = g_path_get_basename (gimp_file_get_utf8_name (file));

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
    case GIMP_RUN_INTERACTIVE:
    case GIMP_RUN_WITH_LAST_VALS:
      gimp_ui_init (PLUG_IN_BINARY);

      export = gimp_export_image (&image, &n_drawables, &drawables, "PAT",
                                  GIMP_EXPORT_CAN_HANDLE_GRAY    |
                                  GIMP_EXPORT_CAN_HANDLE_RGB     |
                                  GIMP_EXPORT_CAN_HANDLE_INDEXED |
                                  GIMP_EXPORT_CAN_HANDLE_ALPHA);

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
        status = GIMP_PDB_CANCEL;
    }

  if (status == GIMP_PDB_SUCCESS)
    {
      GimpValueArray *save_retvals;
      GimpValueArray *args;

      g_object_get (config,
                    "description", &description,
                    NULL);

      args = gimp_value_array_new_from_types (NULL,
                                              GIMP_TYPE_RUN_MODE,     GIMP_RUN_NONINTERACTIVE,
                                              GIMP_TYPE_IMAGE,        image,
                                              G_TYPE_INT,             n_drawables,
                                              GIMP_TYPE_OBJECT_ARRAY, NULL,
                                              G_TYPE_FILE,            file,
                                              G_TYPE_STRING,          description,
                                              G_TYPE_NONE);
      gimp_value_set_object_array (gimp_value_array_index (args, 3),
                                   GIMP_TYPE_ITEM, (GObject **) drawables, n_drawables);

      save_retvals = gimp_pdb_run_procedure_array (gimp_get_pdb (),
                                                   "file-pat-save-internal",
                                                   args);
      gimp_value_array_unref (args);

      if (GIMP_VALUES_GET_ENUM (save_retvals, 0) != GIMP_PDB_SUCCESS)
        {
          g_set_error (&error, 0, 0,
                       "Running procedure 'file-pat-save-internal' "
                       "failed: %s",
                       gimp_pdb_get_last_error (gimp_get_pdb ()));

          status = GIMP_PDB_EXECUTION_ERROR;
        }

      gimp_value_array_unref (save_retvals);
    }

  if (export == GIMP_EXPORT_EXPORT)
    {
      gimp_image_delete (image);
      g_free (drawables);
    }

  return gimp_procedure_new_return_values (procedure, status, error);
}

static gboolean
save_dialog (GimpImage     *image,
             GimpProcedure *procedure,
             GObject       *config)
{
  GtkWidget *dialog;
  GtkWidget *entry;
  GtkWidget *real_entry;
  gboolean   run;

  dialog = gimp_save_procedure_dialog_new (GIMP_SAVE_PROCEDURE (procedure),
                                           GIMP_PROCEDURE_CONFIG (config),
                                           image);

  entry = gimp_procedure_dialog_get_widget (GIMP_PROCEDURE_DIALOG (dialog),
                                            "description", GIMP_TYPE_LABEL_ENTRY);
  real_entry = gimp_label_entry_get_entry (GIMP_LABEL_ENTRY (entry));
  gtk_entry_set_max_length (GTK_ENTRY (real_entry), 256);
  gtk_entry_set_width_chars (GTK_ENTRY (real_entry), 20);
  gtk_entry_set_activates_default (GTK_ENTRY (real_entry), TRUE);

  gimp_procedure_dialog_fill (GIMP_PROCEDURE_DIALOG (dialog), NULL);

  gtk_widget_show (dialog);

  run = gimp_procedure_dialog_run (GIMP_PROCEDURE_DIALOG (dialog));

  gtk_widget_destroy (dialog);

  return run;
}
