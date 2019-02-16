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


/*  local function prototypes  */

static void       query       (void);
static void       run         (const gchar      *name,
                               gint              nparams,
                               const GimpParam  *param,
                               gint             *nreturn_vals,
                               GimpParam       **return_vals);

static gboolean   save_dialog (void);


const GimpPlugInInfo PLUG_IN_INFO =
{
  NULL,  /* init_proc  */
  NULL,  /* quit_proc  */
  query, /* query_proc */
  run,   /* run_proc   */
};


/*  private variables  */

static gchar description[256] = "GIMP Pattern";


MAIN ()

static void
query (void)
{
  static const GimpParamDef save_args[] =
  {
    { GIMP_PDB_INT32,    "run-mode",    "The run mode { RUN-INTERACTIVE (0), RUN-NONINTERACTIVE (1) }"     },
    { GIMP_PDB_IMAGE,    "image",       "Input image"                      },
    { GIMP_PDB_DRAWABLE, "drawable",    "Drawable to export"                 },
    { GIMP_PDB_STRING,   "uri",         "The URI of the file to export the image in" },
    { GIMP_PDB_STRING,   "raw-uri",     "The URI of the file to export the image in" },
    { GIMP_PDB_STRING,   "description", "Short description of the pattern" }
  };

  gimp_install_procedure (SAVE_PROC,
                          "Exports Gimp pattern file (.PAT)",
                          "New Gimp patterns can be created by exporting them "
                          "in the appropriate place with this plug-in.",
                          "Tim Newsome",
                          "Tim Newsome",
                          "1997",
                          N_("GIMP pattern"),
                          "RGB*, GRAY*, INDEXED*",
                          GIMP_PLUGIN,
                          G_N_ELEMENTS (save_args), 0,
                          save_args, NULL);

  gimp_plugin_icon_register (SAVE_PROC, GIMP_ICON_TYPE_ICON_NAME,
                             (const guint8 *) GIMP_ICON_PATTERN);
  gimp_register_file_handler_mime (SAVE_PROC, "image/x-gimp-pat");
  gimp_register_file_handler_uri (SAVE_PROC);
  gimp_register_save_handler (SAVE_PROC, "pat", "");
}

static void
run (const gchar      *name,
     gint              nparams,
     const GimpParam  *param,
     gint             *nreturn_vals,
     GimpParam       **return_vals)
{
  static GimpParam   values[2];
  GimpRunMode        run_mode;
  GimpPDBStatusType  status = GIMP_PDB_SUCCESS;
  gint32             image_ID;
  gint32             drawable_ID;
  GimpExportReturn   export = GIMP_EXPORT_CANCEL;
  GError            *error  = NULL;

  INIT_I18N ();

  run_mode = param[0].data.d_int32;

  *nreturn_vals = 1;
  *return_vals  = values;

  values[0].type          = GIMP_PDB_STATUS;
  values[0].data.d_status = GIMP_PDB_EXECUTION_ERROR;

  if (strcmp (name, SAVE_PROC) == 0)
    {
      GFile        *file;
      GimpParasite *parasite;
      gint32        orig_image_ID;

      image_ID    = param[1].data.d_int32;
      drawable_ID = param[2].data.d_int32;
      file        = g_file_new_for_uri (param[3].data.d_string);

      orig_image_ID = image_ID;

      switch (run_mode)
        {
        case GIMP_RUN_INTERACTIVE:
        case GIMP_RUN_WITH_LAST_VALS:
          gimp_ui_init (PLUG_IN_BINARY, FALSE);

          export = gimp_export_image (&image_ID, &drawable_ID, "PAT",
                                      GIMP_EXPORT_CAN_HANDLE_GRAY    |
                                      GIMP_EXPORT_CAN_HANDLE_RGB     |
                                      GIMP_EXPORT_CAN_HANDLE_INDEXED |
                                      GIMP_EXPORT_CAN_HANDLE_ALPHA);

          if (export == GIMP_EXPORT_CANCEL)
            {
              values[0].data.d_status = GIMP_PDB_CANCEL;
              return;
            }

          /*  Possibly retrieve data  */
          gimp_get_data (SAVE_PROC, description);

          parasite = gimp_image_get_parasite (orig_image_ID,
                                              "gimp-pattern-name");
          if (parasite)
            {
              strncpy (description,
                       gimp_parasite_data (parasite),
                       MIN (sizeof (description),
                            gimp_parasite_data_size (parasite)));
              description[sizeof (description) - 1] = '\0';

              gimp_parasite_free (parasite);
            }
          else
            {
              gchar *name = g_path_get_basename (gimp_file_get_utf8_name (file));

              if (g_str_has_suffix (name, ".pat"))
                name[strlen (name) - 4] = '\0';

              if (strlen (name))
                {
                  strncpy (description, name, sizeof (description));
                  description[sizeof (description) - 1] = '\0';
                }

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
            status = GIMP_PDB_CANCEL;
          break;

        case GIMP_RUN_NONINTERACTIVE:
          if (nparams != 6)
            {
              status = GIMP_PDB_CALLING_ERROR;
            }
          else
            {
              strncpy (description, param[5].data.d_string,
                       sizeof (description));
              description[sizeof (description) - 1] = '\0';
            }
          break;

        default:
          break;
        }

      if (status == GIMP_PDB_SUCCESS)
        {
          GimpParam *save_retvals;
          gint       n_save_retvals;

          save_retvals =
            gimp_run_procedure ("file-pat-save-internal",
                                &n_save_retvals,
                                GIMP_PDB_INT32,    GIMP_RUN_NONINTERACTIVE,
                                GIMP_PDB_IMAGE,    image_ID,
                                GIMP_PDB_DRAWABLE, drawable_ID,
                                GIMP_PDB_STRING,   param[3].data.d_string,
                                GIMP_PDB_STRING,   param[4].data.d_string,
                                GIMP_PDB_STRING,   description,
                                GIMP_PDB_END);


          if (save_retvals[0].data.d_status == GIMP_PDB_SUCCESS)
            {
              gimp_set_data (SAVE_PROC, description, sizeof (description));
            }
          else
            {
              g_set_error (&error, 0, 0,
                           "Running procedure 'file-pat-save-internal' "
                           "failed: %s",
                           gimp_get_pdb_error ());

              status = GIMP_PDB_EXECUTION_ERROR;
            }

          gimp_destroy_params (save_retvals, n_save_retvals);
        }

      if (export == GIMP_EXPORT_EXPORT)
        gimp_image_delete (image_ID);

      if (strlen (description))
        {
          GimpParasite *parasite;

          parasite = gimp_parasite_new ("gimp-pattern-name",
                                        GIMP_PARASITE_PERSISTENT,
                                        strlen (description) + 1,
                                        description);
          gimp_image_attach_parasite (orig_image_ID, parasite);
          gimp_parasite_free (parasite);
        }
      else
        {
          gimp_image_detach_parasite (orig_image_ID, "gimp-pattern-name");
        }
    }
  else
    {
      status = GIMP_PDB_CALLING_ERROR;
    }

  if (status != GIMP_PDB_SUCCESS && error)
    {
      *nreturn_vals = 2;
      values[1].type          = GIMP_PDB_STRING;
      values[1].data.d_string = error->message;
    }

  values[0].data.d_status = status;
}

static gboolean
save_dialog (void)
{
  GtkWidget *dialog;
  GtkWidget *table;
  GtkWidget *entry;
  gboolean   run;

  dialog = gimp_export_dialog_new (_("Pattern"), PLUG_IN_BINARY, SAVE_PROC);

  /* The main table */
  table = gtk_table_new (1, 2, FALSE);
  gtk_table_set_col_spacings (GTK_TABLE (table), 6);
  gtk_container_set_border_width (GTK_CONTAINER (table), 12);
  gtk_box_pack_start (GTK_BOX (gimp_export_dialog_get_content_area (dialog)),
                      table, TRUE, TRUE, 0);
  gtk_widget_show (table);

  entry = gtk_entry_new ();
  gtk_entry_set_width_chars (GTK_ENTRY (entry), 20);
  gtk_entry_set_activates_default (GTK_ENTRY (entry), TRUE);
  gtk_entry_set_text (GTK_ENTRY (entry), description);
  gimp_table_attach_aligned (GTK_TABLE (table), 0, 0,
                             _("Description:"), 1.0, 0.5,
                             entry, 1, FALSE);
  gtk_widget_show (entry);

  gtk_widget_show (dialog);

  run = (gimp_dialog_run (GIMP_DIALOG (dialog)) == GTK_RESPONSE_OK);

  if (run)
    {
      strncpy (description, gtk_entry_get_text (GTK_ENTRY (entry)),
               sizeof (description));
      description[sizeof (description) - 1] = '\0';
    }

  gtk_widget_destroy (dialog);

  return run;
}
