/* GIMP - The GNU Image Manipulation Program
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
 * Loads/exports version 2 GIMP .gbr files, by Tim Newsome <drz@frody.bloke.com>
 * Some bits stolen from the .99.7 source tree.
 *
 * Added in GBR version 1 support after learning that there wasn't a
 * tool to read them.
 * July 6, 1998 by Seth Burgess <sjburges@gimp.org>
 *
 * Dec 17, 2000
 * Load and save GIMP brushes in GRAY or RGBA.  jtl + neo
 *
 *
 * TODO: Give some better error reporting on not opening files/bad headers
 *       etc.
 */

#include "config.h"

#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

#include "libgimp/stdplugins-intl.h"


#define SAVE_PROC      "file-gbr-save"
#define PLUG_IN_BINARY "file-gbr"
#define PLUG_IN_ROLE   "gimp-file-gbr"


typedef struct
{
  gchar description[256];
  gint  spacing;
} BrushInfo;


typedef struct _Gbr      Gbr;
typedef struct _GbrClass GbrClass;

struct _Gbr
{
  GimpPlugIn      parent_instance;
};

struct _GbrClass
{
  GimpPlugInClass parent_class;
};


#define GBR_TYPE  (gbr_get_type ())
#define GBR (obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), GBR_TYPE, Gbr))

GType                   gbr_get_type         (void) G_GNUC_CONST;

static GList          * gbr_query_procedures (GimpPlugIn           *plug_in);
static GimpProcedure  * gbr_create_procedure (GimpPlugIn           *plug_in,
                                              const gchar          *name);

static GimpValueArray * gbr_save             (GimpProcedure        *procedure,
                                              GimpRunMode           run_mode,
                                              GimpImage            *image,
                                              GimpDrawable         *drawable,
                                              GFile                *file,
                                              const GimpValueArray *args,
                                              gpointer              run_data);

static gboolean         save_dialog          (void);
static void             entry_callback       (GtkWidget            *widget,
                                              gpointer              data);


G_DEFINE_TYPE (Gbr, gbr, GIMP_TYPE_PLUG_IN)

GIMP_MAIN (GBR_TYPE)

static BrushInfo info =
{
  "GIMP Brush",
  10
};


static void
gbr_class_init (GbrClass *klass)
{
  GimpPlugInClass *plug_in_class = GIMP_PLUG_IN_CLASS (klass);

  plug_in_class->query_procedures = gbr_query_procedures;
  plug_in_class->create_procedure = gbr_create_procedure;
}

static void
gbr_init (Gbr *gbr)
{
}

static GList *
gbr_query_procedures (GimpPlugIn *plug_in)
{
  return g_list_append (NULL, g_strdup (SAVE_PROC));
}

static GimpProcedure *
gbr_create_procedure (GimpPlugIn  *plug_in,
                      const gchar *name)
{
  GimpProcedure *procedure = NULL;

  if (! strcmp (name, SAVE_PROC))
    {
      procedure = gimp_save_procedure_new (plug_in, name,
                                           GIMP_PDB_PROC_TYPE_PLUGIN,
                                           gbr_save, NULL, NULL);

      gimp_procedure_set_image_types (procedure, "*");

      gimp_procedure_set_menu_label (procedure, N_("GIMP brush"));
      gimp_procedure_set_icon_name (procedure, GIMP_ICON_BRUSH);

      gimp_procedure_set_documentation (procedure,
                                        "Exports files in the GIMP brush "
                                        "file format",
                                        "Exports files in the GIMP brush "
                                        "file format",
                                        SAVE_PROC);
      gimp_procedure_set_attribution (procedure,
                                      "Tim Newsome, Jens Lautenbacher, "
                                      "Sven Neumann",
                                      "Tim Newsome, Jens Lautenbacher, "
                                      "Sven Neumann",
                                      "1997-2000");

      gimp_file_procedure_set_mime_types (GIMP_FILE_PROCEDURE (procedure),
                                          "image/x-gimp-gbr");
      gimp_file_procedure_set_extensions (GIMP_FILE_PROCEDURE (procedure),
                                          "gbr");
      gimp_file_procedure_set_handles_remote (GIMP_FILE_PROCEDURE (procedure),
                                              TRUE);

      GIMP_PROC_ARG_INT (procedure, "spacing",
                         "Spacing",
                         "Spacing of the brush",
                         1, 1000, 10,
                         GIMP_PARAM_READWRITE);

      GIMP_PROC_ARG_STRING (procedure, "description",
                            "Description",
                            "Short description of the brush",
                            "GIMP Brush",
                            GIMP_PARAM_READWRITE);
    }

  return procedure;
}

static GimpValueArray *
gbr_save (GimpProcedure        *procedure,
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

      export = gimp_export_image (&image, &drawable, "GBR",
                                  GIMP_EXPORT_CAN_HANDLE_GRAY    |
                                  GIMP_EXPORT_CAN_HANDLE_RGB     |
                                  GIMP_EXPORT_CAN_HANDLE_INDEXED |
                                  GIMP_EXPORT_CAN_HANDLE_ALPHA);

      if (export == GIMP_EXPORT_CANCEL)
        return gimp_procedure_new_return_values (procedure,
                                                 GIMP_PDB_CANCEL,
                                                 NULL);

      /*  Possibly retrieve data  */
      gimp_get_data (SAVE_PROC, &info);

      parasite = gimp_image_get_parasite (orig_image,
                                          "gimp-brush-name");
      if (parasite)
        {
          g_strlcpy (info.description,
                     gimp_parasite_data (parasite),
                     MIN (sizeof (info.description),
                          gimp_parasite_data_size (parasite)));

          gimp_parasite_free (parasite);
        }
      else
        {
          gchar *name = g_path_get_basename (gimp_file_get_utf8_name (file));

          if (g_str_has_suffix (name, ".gbr"))
            name[strlen (name) - 4] = '\0';

          if (strlen (name))
            g_strlcpy (info.description, name, sizeof (info.description));

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
      info.spacing = GIMP_VALUES_GET_INT (args, 0);
      g_strlcpy (info.description,
                 GIMP_VALUES_GET_STRING (args, 1),
                 sizeof (info.description));
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
                                "file-gbr-save-internal",
                                GIMP_TYPE_RUN_MODE, GIMP_RUN_NONINTERACTIVE,
                                GIMP_TYPE_IMAGE,    image,
                                GIMP_TYPE_DRAWABLE, drawable,
                                G_TYPE_STRING,      uri,
                                G_TYPE_STRING,      uri,
                                G_TYPE_INT,         info.spacing,
                                G_TYPE_STRING,      info.description,
                                G_TYPE_NONE);

      g_free (uri);

      if (GIMP_VALUES_GET_ENUM (save_retvals, 0) == GIMP_PDB_SUCCESS)
        {
          gimp_set_data (SAVE_PROC, &info, sizeof (info));
        }
      else
        {
          g_set_error (&error, 0, 0,
                       "Running procedure 'file-gbr-save-internal' "
                       "failed: %s",
                       gimp_pdb_get_last_error (gimp_get_pdb ()));

          status = GIMP_PDB_EXECUTION_ERROR;
        }

      gimp_value_array_unref (save_retvals);
    }

  if (strlen (info.description))
    {
      GimpParasite *parasite;

      parasite = gimp_parasite_new ("gimp-brush-name",
                                    GIMP_PARASITE_PERSISTENT,
                                    strlen (info.description) + 1,
                                    info.description);
      gimp_image_attach_parasite (orig_image, parasite);
      gimp_parasite_free (parasite);
    }
  else
    {
      gimp_image_detach_parasite (orig_image, "gimp-brush-name");
    }

 out:
  if (export == GIMP_EXPORT_EXPORT)
    gimp_image_delete (image);

  return gimp_procedure_new_return_values (procedure, status, error);
}

static gboolean
save_dialog (void)
{
  GtkWidget     *dialog;
  GtkWidget     *grid;
  GtkWidget     *entry;
  GtkWidget     *spinbutton;
  GtkAdjustment *adj;
  gboolean       run;

  dialog = gimp_export_dialog_new (_("Brush"), PLUG_IN_BINARY, SAVE_PROC);

  /* The main grid */
  grid = gtk_grid_new ();
  gtk_container_set_border_width (GTK_CONTAINER (grid), 12);
  gtk_grid_set_row_spacing (GTK_GRID (grid), 6);
  gtk_grid_set_column_spacing (GTK_GRID (grid), 6);
  gtk_box_pack_start (GTK_BOX (gimp_export_dialog_get_content_area (dialog)),
                      grid, TRUE, TRUE, 0);
  gtk_widget_show (grid);

  entry = gtk_entry_new ();
  gtk_entry_set_width_chars (GTK_ENTRY (entry), 20);
  gtk_entry_set_activates_default (GTK_ENTRY (entry), TRUE);
  gtk_entry_set_text (GTK_ENTRY (entry), info.description);
  gimp_grid_attach_aligned (GTK_GRID (grid), 0, 0,
                            _("Description:"), 1.0, 0.5,
                            entry, 1);

  g_signal_connect (entry, "changed",
                    G_CALLBACK (entry_callback),
                    info.description);

  adj = gtk_adjustment_new (info.spacing, 1, 1000, 1, 10, 0);
  spinbutton = gimp_spin_button_new (adj, 1.0, 0);
  gtk_spin_button_set_numeric (GTK_SPIN_BUTTON (spinbutton), TRUE);
  gtk_entry_set_activates_default (GTK_ENTRY (spinbutton), TRUE);
  gimp_grid_attach_aligned (GTK_GRID (grid), 0, 1,
                            _("Spacing:"), 1.0, 0.5,
                            spinbutton, 1);

  g_signal_connect (adj, "value-changed",
                    G_CALLBACK (gimp_int_adjustment_update),
                    &info.spacing);

  gtk_widget_show (dialog);

  run = (gimp_dialog_run (GIMP_DIALOG (dialog)) == GTK_RESPONSE_OK);

  gtk_widget_destroy (dialog);

  return run;
}

static void
entry_callback (GtkWidget *widget,
                gpointer   data)
{
  g_strlcpy (info.description, gtk_entry_get_text (GTK_ENTRY (widget)),
             sizeof (info.description));
}
