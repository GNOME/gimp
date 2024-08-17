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


#define EXPORT_PROC    "file-pat-export"
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
#define PAT(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), PAT_TYPE, Pat))

GType                   pat_get_type         (void) G_GNUC_CONST;

static GList          * pat_query_procedures (GimpPlugIn           *plug_in);
static GimpProcedure  * pat_create_procedure (GimpPlugIn           *plug_in,
                                              const gchar          *name);

static GimpValueArray * pat_export           (GimpProcedure        *procedure,
                                              GimpRunMode           run_mode,
                                              GimpImage            *image,
                                              GFile                *file,
                                              GimpExportOptions    *options,
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
  return g_list_append (NULL, g_strdup (EXPORT_PROC));
}

static GimpProcedure *
pat_create_procedure (GimpPlugIn  *plug_in,
                      const gchar *name)
{
  GimpProcedure *procedure = NULL;

  if (! strcmp (name, EXPORT_PROC))
    {
      procedure = gimp_export_procedure_new (plug_in, name,
                                             GIMP_PDB_PROC_TYPE_PLUGIN,
                                             FALSE, pat_export, NULL, NULL);

      gimp_procedure_set_image_types (procedure, "*");

      gimp_procedure_set_menu_label (procedure, _("GIMP pattern"));
      gimp_procedure_set_icon_name (procedure, GIMP_ICON_PATTERN);

      gimp_procedure_set_documentation (procedure,
                                        _("Exports GIMP pattern file (.PAT)"),
                                        _("New GIMP patterns can be created "
                                          "by exporting them in the "
                                          "appropriate place with this plug-in."),
                                        EXPORT_PROC);
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

      gimp_export_procedure_set_capabilities (GIMP_EXPORT_PROCEDURE (procedure),
                                              GIMP_EXPORT_CAN_HANDLE_GRAY    |
                                              GIMP_EXPORT_CAN_HANDLE_RGB     |
                                              GIMP_EXPORT_CAN_HANDLE_INDEXED |
                                              GIMP_EXPORT_CAN_HANDLE_ALPHA,
                                              NULL, NULL, NULL);

      gimp_procedure_add_string_argument (procedure, "description",
                                          _("_Description"),
                                          _("Short description of the pattern"),
                                          _("GIMP Pattern"),
                                          GIMP_PARAM_READWRITE);
    }

  return procedure;
}

static GimpValueArray *
pat_export (GimpProcedure        *procedure,
            GimpRunMode           run_mode,
            GimpImage            *image,
            GFile                *file,
            GimpExportOptions    *options,
            GimpMetadata         *metadata,
            GimpProcedureConfig  *config,
            gpointer              run_data)
{
  GimpPDBStatusType  status = GIMP_PDB_SUCCESS;
  GimpExportReturn   export = GIMP_EXPORT_IGNORE;
  GList             *drawables;
  gint               n_drawables;
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

  if (run_mode == GIMP_RUN_INTERACTIVE)
    {
      gimp_ui_init (PLUG_IN_BINARY);

      if (! save_dialog (image, procedure, G_OBJECT (config)))
        status = GIMP_PDB_CANCEL;
    }

  export = gimp_export_options_get_image (options, &image);
  drawables = gimp_image_list_layers (image);
  n_drawables = g_list_length (drawables);

  if (status == GIMP_PDB_SUCCESS)
    {
      GimpProcedure   *procedure;
      GimpValueArray  *save_retvals;
      GimpObjectArray *drawables_array;

      drawables_array = gimp_object_array_new (GIMP_TYPE_DRAWABLE, (GObject **) drawables, n_drawables, FALSE);

      g_object_get (config,
                    "description", &description,
                    NULL);

      procedure    = gimp_pdb_lookup_procedure (gimp_get_pdb (),
                                                "file-pat-export-internal");
      save_retvals = gimp_procedure_run (procedure,
                                         "image",         image,
                                         "num-drawables", n_drawables,
                                         "drawables",     drawables_array,
                                         "file",          file,
                                         "name",          description,
                                         NULL);

      if (GIMP_VALUES_GET_ENUM (save_retvals, 0) != GIMP_PDB_SUCCESS)
        {
          g_set_error (&error, 0, 0,
                       "Running procedure 'file-pat-export-internal' "
                       "failed: %s",
                       gimp_pdb_get_last_error (gimp_get_pdb ()));

          status = GIMP_PDB_EXECUTION_ERROR;
        }

      gimp_value_array_unref (save_retvals);
      gimp_object_array_free (drawables_array);
    }

  if (export == GIMP_EXPORT_EXPORT)
    gimp_image_delete (image);

  g_list_free (drawables);
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

  dialog = gimp_export_procedure_dialog_new (GIMP_EXPORT_PROCEDURE (procedure),
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
