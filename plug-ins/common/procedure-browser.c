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
 * Btw, hope it gives you some ideas about ligma possibilities.
 *
 * The core of the plugin is not here. See dbbrowser_utils (shared
 * with script-fu-console).
 *
 * TODO
 * - bug fixes... (my method : rewrite from scratch :)
 */

#include "config.h"

#include <gtk/gtk.h>

#include <libligma/ligma.h>
#include <libligma/ligmaui.h>

#include "libligma/stdplugins-intl.h"


#define PLUG_IN_PROC   "plug-in-dbbrowser"
#define PLUG_IN_BINARY "procedure-browser"
#define PLUG_IN_ROLE   "ligma-procedure-browser"


typedef struct _Browser      Browser;
typedef struct _BrowserClass BrowserClass;

struct _Browser
{
  LigmaPlugIn parent_instance;
};

struct _BrowserClass
{
  LigmaPlugInClass parent_class;
};


/* Declare local functions.
 */

#define BROWSER_TYPE  (browser_get_type ())
#define BROWSER (obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), BROWSER_TYPE, Browser))

GType                   browser_get_type         (void) G_GNUC_CONST;

static GList          * browser_query_procedures (LigmaPlugIn           *plug_in);
static LigmaProcedure  * browser_create_procedure (LigmaPlugIn           *plug_in,
                                                  const gchar          *name);

static LigmaValueArray * browser_run              (LigmaProcedure        *procedure,
                                                  const LigmaValueArray *args,
                                                  gpointer              run_data);


G_DEFINE_TYPE (Browser, browser, LIGMA_TYPE_PLUG_IN)

LIGMA_MAIN (BROWSER_TYPE)
DEFINE_STD_SET_I18N


static void
browser_class_init (BrowserClass *klass)
{
  LigmaPlugInClass *plug_in_class = LIGMA_PLUG_IN_CLASS (klass);

  plug_in_class->query_procedures = browser_query_procedures;
  plug_in_class->create_procedure = browser_create_procedure;
  plug_in_class->set_i18n         = STD_SET_I18N;
}

static void
browser_init (Browser *browser)
{
}

static GList *
browser_query_procedures (LigmaPlugIn *plug_in)
{
  return g_list_append (NULL, g_strdup (PLUG_IN_PROC));
}

static LigmaProcedure *
browser_create_procedure (LigmaPlugIn  *plug_in,
                          const gchar *name)
{
  LigmaProcedure *procedure = NULL;

  if (! strcmp (name, PLUG_IN_PROC))
    {
      procedure = ligma_procedure_new (plug_in, name,
                                      LIGMA_PDB_PROC_TYPE_PLUGIN,
                                      browser_run, NULL, NULL);

      ligma_procedure_set_menu_label (procedure, _("Procedure _Browser"));
      ligma_procedure_add_menu_path (procedure, "<Image>/Help/Programming");

      ligma_procedure_set_documentation (procedure,
                                        _("List available procedures in the PDB"),
                                        NULL,
                                        PLUG_IN_PROC);
      ligma_procedure_set_attribution (procedure,
                                      "Thomas Noel",
                                      "Thomas Noel",
                                      "23th june 1997");

      LIGMA_PROC_ARG_ENUM (procedure, "run-mode",
                          "Run mode",
                          "The run mode",
                          LIGMA_TYPE_RUN_MODE,
                          LIGMA_RUN_INTERACTIVE,
                          G_PARAM_READWRITE);
    }

  return procedure;
}

static LigmaValueArray *
browser_run (LigmaProcedure        *procedure,
             const LigmaValueArray *args,
             gpointer              run_data)
{
  LigmaRunMode run_mode = LIGMA_VALUES_GET_ENUM (args, 0);

  switch (run_mode)
    {
    case LIGMA_RUN_INTERACTIVE:
      {
        GtkWidget *dialog;

        ligma_ui_init (PLUG_IN_BINARY);

        dialog =
          ligma_proc_browser_dialog_new (_("Procedure Browser"), PLUG_IN_BINARY,
                                        ligma_standard_help_func, PLUG_IN_PROC,

                                        _("_Close"), GTK_RESPONSE_CLOSE,

                                        NULL);

        gtk_dialog_run (GTK_DIALOG (dialog));
        gtk_widget_destroy (dialog);
      }
      break;

    case LIGMA_RUN_WITH_LAST_VALS:
    case LIGMA_RUN_NONINTERACTIVE:
      g_printerr (PLUG_IN_PROC " allows only interactive invocation");

      return ligma_procedure_new_return_values (procedure,
                                               LIGMA_PDB_CALLING_ERROR,
                                               NULL);
      break;

    default:
      break;
    }

  return ligma_procedure_new_return_values (procedure, LIGMA_PDB_SUCCESS, NULL);
}
