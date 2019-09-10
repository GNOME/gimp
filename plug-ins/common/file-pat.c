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
#define PLUG_IN_ROLE   "gimp-file-pat"


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
                                              GimpDrawable         *drawable,
                                              GFile                *file,
                                              const GimpValueArray *args,
                                              gpointer              run_data);

static gboolean         save_dialog          (void);


G_DEFINE_TYPE (Pat, pat, GIMP_TYPE_PLUG_IN)

GIMP_MAIN (PAT_TYPE)

static gchar description[256] = "GIMP Pattern";


static void
pat_class_init (PatClass *klass)
{
  GimpPlugInClass *plug_in_class = GIMP_PLUG_IN_CLASS (klass);

  plug_in_class->query_procedures = pat_query_procedures;
  plug_in_class->create_procedure = pat_create_procedure;
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
                                           pat_save, NULL, NULL);

      gimp_procedure_set_image_types (procedure, "*");

      gimp_procedure_set_menu_label (procedure, N_("GIMP pattern"));
      gimp_procedure_set_icon_name (procedure, GIMP_ICON_PATTERN);

      gimp_procedure_set_documentation (procedure,
                                        "Exports Gimp pattern file (.PAT)",
                                        "New Gimp patterns can be created "
                                        "by exporting them in the "
                                        "appropriate place with this plug-in.",
                                        SAVE_PROC);
      gimp_procedure_set_attribution (procedure,
                                      "Tim Newsome",
                                      "Tim Newsome",
                                      "1997");

      gimp_file_procedure_set_mime_types (GIMP_FILE_PROCEDURE (procedure),
                                          "image/x-gimp-pat");
      gimp_file_procedure_set_extensions (GIMP_FILE_PROCEDURE (procedure),
                                          "pat");
      gimp_file_procedure_set_handles_remote (GIMP_FILE_PROCEDURE (procedure),
                                              TRUE);

      GIMP_PROC_ARG_STRING (procedure, "description",
                            "Description",
                            "Short description of the pattern",
                            "GIMP Pattern",
                            GIMP_PARAM_READWRITE);
    }

  return procedure;
}

static GimpValueArray *
pat_save (GimpProcedure        *procedure,
          GimpRunMode           run_mode,
          GimpImage            *image,
          GimpDrawable         *drawable,
          GFile                *file,
          const GimpValueArray *args,
          gpointer              run_data)
{
  GimpPDBStatusType  status = GIMP_PDB_SUCCESS;
  GimpExportReturn   export = GIMP_EXPORT_CANCEL;
  GimpParasite      *parasite;
  GimpImage         *orig_image;
  GError            *error  = NULL;

  INIT_I18N ();

  orig_image = image;

  switch (run_mode)
    {
    case GIMP_RUN_INTERACTIVE:
    case GIMP_RUN_WITH_LAST_VALS:
      gimp_ui_init (PLUG_IN_BINARY, FALSE);

      export = gimp_export_image (&image, &drawable, "PAT",
                                  GIMP_EXPORT_CAN_HANDLE_GRAY    |
                                  GIMP_EXPORT_CAN_HANDLE_RGB     |
                                  GIMP_EXPORT_CAN_HANDLE_INDEXED |
                                  GIMP_EXPORT_CAN_HANDLE_ALPHA);

      if (export == GIMP_EXPORT_CANCEL)
        return gimp_procedure_new_return_values (procedure, GIMP_PDB_CANCEL,
                                                 NULL);

      /*  Possibly retrieve data  */
      gimp_get_data (SAVE_PROC, description);

      parasite = gimp_image_get_parasite (orig_image, "gimp-pattern-name");
      if (parasite)
        {
          g_strlcpy (description,
                     gimp_parasite_data (parasite),
                     MIN (sizeof (description),
                          gimp_parasite_data_size (parasite)));

          gimp_parasite_free (parasite);
        }
      else
        {
          gchar *name = g_path_get_basename (gimp_file_get_utf8_name (file));

          if (g_str_has_suffix (name, ".pat"))
            name[strlen (name) - 4] = '\0';

          if (strlen (name))
            g_strlcpy (description, name, sizeof (description));

          g_free (name);
        }
      break;

    default:
      break;
    }

  switch (run_mode)
    {
    case GIMP_RUN_INTERACTIVE:
      if (! save_dialog ())
        {
          status = GIMP_PDB_CANCEL;
          goto out;
        }
      break;

    case GIMP_RUN_NONINTERACTIVE:
      g_strlcpy (description,
                 GIMP_VALUES_GET_STRING (args, 0),
                 sizeof (description));
      break;

    default:
      break;
    }

  if (status == GIMP_PDB_SUCCESS)
    {
      GimpValueArray *save_retvals;
      gchar          *uri = g_file_get_uri (file);

      save_retvals =
        gimp_pdb_run_procedure (gimp_get_pdb (),
                                "file-pat-save-internal",
                                GIMP_TYPE_RUN_MODE, GIMP_RUN_NONINTERACTIVE,
                                GIMP_TYPE_IMAGE,    image,
                                GIMP_TYPE_DRAWABLE, drawable,
                                G_TYPE_STRING,      uri,
                                G_TYPE_STRING,      description,
                                G_TYPE_NONE);

      g_free (uri);

      if (GIMP_VALUES_GET_ENUM (save_retvals, 0) == GIMP_PDB_SUCCESS)
        {
          gimp_set_data (SAVE_PROC, description, sizeof (description));
        }
      else
        {
          g_set_error (&error, 0, 0,
                       "Running procedure 'file-pat-save-internal' "
                       "failed: %s",
                       gimp_pdb_get_last_error (gimp_get_pdb ()));

          status = GIMP_PDB_EXECUTION_ERROR;
        }

      gimp_value_array_unref (save_retvals);
    }

  if (strlen (description))
    {
      GimpParasite *parasite;

      parasite = gimp_parasite_new ("gimp-pattern-name",
                                    GIMP_PARASITE_PERSISTENT,
                                    strlen (description) + 1,
                                    description);
      gimp_image_attach_parasite (orig_image, parasite);
      gimp_parasite_free (parasite);
    }
  else
    {
      gimp_image_detach_parasite (orig_image, "gimp-pattern-name");
    }

 out:
  if (export == GIMP_EXPORT_EXPORT)
    gimp_image_delete (image);

  return gimp_procedure_new_return_values (procedure, status, error);
}

static gboolean
save_dialog (void)
{
  GtkWidget *dialog;
  GtkWidget *grid;
  GtkWidget *entry;
  gboolean   run;

  dialog = gimp_export_dialog_new (_("Pattern"), PLUG_IN_BINARY, SAVE_PROC);

  /* The main grid */
  grid = gtk_grid_new ();
  gtk_grid_set_column_spacing (GTK_GRID (grid), 6);
  gtk_container_set_border_width (GTK_CONTAINER (grid), 12);
  gtk_box_pack_start (GTK_BOX (gimp_export_dialog_get_content_area (dialog)),
                      grid, TRUE, TRUE, 0);
  gtk_widget_show (grid);

  entry = gtk_entry_new ();
  gtk_entry_set_width_chars (GTK_ENTRY (entry), 20);
  gtk_entry_set_activates_default (GTK_ENTRY (entry), TRUE);
  gtk_entry_set_text (GTK_ENTRY (entry), description);
  gimp_grid_attach_aligned (GTK_GRID (grid), 0, 0,
                            _("_Description:"), 1.0, 0.5,
                            entry, 1);
  gtk_widget_show (entry);

  gtk_widget_show (dialog);

  run = (gimp_dialog_run (GIMP_DIALOG (dialog)) == GTK_RESPONSE_OK);

  if (run)
    g_strlcpy (description, gtk_entry_get_text (GTK_ENTRY (entry)),
               sizeof (description));

  gtk_widget_destroy (dialog);

  return run;
}
