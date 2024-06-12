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
 * dbbrowser
 * 0.08 26th sept 97  by Thomas NOEL <thomas@minet.net>
 */

/*
 * This plugin gives you the list of available procedure, with the
 * name, description and parameters for each procedure.
 * You can do regexp search (by name and by description)
 * Useful for scripts development.
 *
 * NOTE :
 * this is only a exercice for me (my first "plug-in" (extension))
 * so it's very (very) dirty.
 * Btw, hope it gives you some ideas about gimp possibilities.
 *
 * The core of the plugin is not here. See dbbrowser_utils (shared
 * with script-fu-console).
 *
 * TODO
 * - bug fixes... (my method : rewrite from scratch :)
 */

#include "config.h"

#include <gtk/gtk.h>

#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

#include "libgimp/stdplugins-intl.h"


#define PLUG_IN_PROC   "plug-in-dbbrowser"
#define PLUG_IN_BINARY "procedure-browser"
#define PLUG_IN_ROLE   "gimp-procedure-browser"


typedef struct _Browser      Browser;
typedef struct _BrowserClass BrowserClass;

struct _Browser
{
  GimpPlugIn parent_instance;
};

struct _BrowserClass
{
  GimpPlugInClass parent_class;
};


/* Declare local functions.
 */

#define BROWSER_TYPE  (browser_get_type ())
#define BROWSER(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), BROWSER_TYPE, Browser))

GType                   browser_get_type         (void) G_GNUC_CONST;

static GList          * browser_query_procedures (GimpPlugIn           *plug_in);
static GimpProcedure  * browser_create_procedure (GimpPlugIn           *plug_in,
                                                  const gchar          *name);

static GimpValueArray * browser_run              (GimpProcedure        *procedure,
                                                  GimpProcedureConfig  *config,
                                                  gpointer              run_data);


G_DEFINE_TYPE (Browser, browser, GIMP_TYPE_PLUG_IN)

GIMP_MAIN (BROWSER_TYPE)
DEFINE_STD_SET_I18N


static void
browser_class_init (BrowserClass *klass)
{
  GimpPlugInClass *plug_in_class = GIMP_PLUG_IN_CLASS (klass);

  plug_in_class->query_procedures = browser_query_procedures;
  plug_in_class->create_procedure = browser_create_procedure;
  plug_in_class->set_i18n         = STD_SET_I18N;
}

static void
browser_init (Browser *browser)
{
}

static GList *
browser_query_procedures (GimpPlugIn *plug_in)
{
  return g_list_append (NULL, g_strdup (PLUG_IN_PROC));
}

static GimpProcedure *
browser_create_procedure (GimpPlugIn  *plug_in,
                          const gchar *name)
{
  GimpProcedure *procedure = NULL;

  if (! strcmp (name, PLUG_IN_PROC))
    {
      procedure = gimp_procedure_new (plug_in, name,
                                      GIMP_PDB_PROC_TYPE_PLUGIN,
                                      browser_run, NULL, NULL);

      gimp_procedure_set_menu_label (procedure, _("Procedure _Browser"));
      gimp_procedure_add_menu_path (procedure, "<Image>/Help/[Programming]");

      gimp_procedure_set_documentation (procedure,
                                        _("List available procedures in the PDB"),
                                        NULL,
                                        PLUG_IN_PROC);
      gimp_procedure_set_attribution (procedure,
                                      "Thomas Noel",
                                      "Thomas Noel",
                                      "23th june 1997");

      gimp_procedure_add_enum_argument (procedure, "run-mode",
                                        "Run mode",
                                        "The run mode",
                                        GIMP_TYPE_RUN_MODE,
                                        GIMP_RUN_INTERACTIVE,
                                        G_PARAM_READWRITE);
    }

  return procedure;
}

static GimpValueArray *
browser_run (GimpProcedure        *procedure,
             GimpProcedureConfig  *config,
             gpointer              run_data)
{
  GimpRunMode run_mode;

  g_object_get (config, "run-mode", &run_mode, NULL);
  switch (run_mode)
    {
    case GIMP_RUN_INTERACTIVE:
      {
        GtkWidget *dialog;

        gimp_ui_init (PLUG_IN_BINARY);

        dialog =
          gimp_proc_browser_dialog_new (_("Procedure Browser"), PLUG_IN_BINARY,
                                        gimp_standard_help_func, PLUG_IN_PROC,

                                        _("_Close"), GTK_RESPONSE_CLOSE,

                                        NULL);

        gtk_dialog_run (GTK_DIALOG (dialog));
        gtk_widget_destroy (dialog);
      }
      break;

    case GIMP_RUN_WITH_LAST_VALS:
    case GIMP_RUN_NONINTERACTIVE:
        {
          GError *error = NULL;

          g_set_error (&error, GIMP_PLUG_IN_ERROR, 0,
                       _("Procedure %s allows only interactive invocation."),
                       gimp_procedure_get_name (procedure));

          return gimp_procedure_new_return_values (procedure,
                                                   GIMP_PDB_CALLING_ERROR,
                                                   error);
        }
      break;

    default:
      break;
    }

  return gimp_procedure_new_return_values (procedure, GIMP_PDB_SUCCESS, NULL);
}
