/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * The LIGMA Help Browser
 * Copyright (C) 1999-2008 Sven Neumann <sven@ligma.org>
 *                         Michael Natterer <mitch@ligma.org>
 *                         Henrik Brix Andersen <brix@ligma.org>
 *
 * Some code & ideas taken from the GNOME help browser.
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

#include <string.h>  /*  strlen, strcmp  */

#include <libligma/ligma.h>
#include <libligma/ligmaui.h>

#include "plug-ins/help/ligmahelp.h"

#include "dialog.h"

#include "libligma/stdplugins-intl.h"


#define LIGMA_HELP_BROWSER_EXT_PROC       "extension-ligma-help-browser"
#define LIGMA_HELP_BROWSER_TEMP_EXT_PROC  "extension-ligma-help-browser-temp"
#define PLUG_IN_BINARY                   "help-browser"
#define PLUG_IN_ROLE                     "ligma-help-browser"


#define LIGMA_TYPE_HELP_BROWSER (ligma_help_browser_get_type ())
G_DECLARE_FINAL_TYPE (LigmaHelpBrowser, ligma_help_browser,
                      LIGMA, HELP_BROWSER,
                      LigmaPlugIn)

static LigmaValueArray * help_browser_run              (LigmaProcedure        *procedure,
                                                       const LigmaValueArray *args,
                                                       gpointer              run_data);

static void             temp_proc_install             (LigmaPlugIn           *plug_in);
static LigmaValueArray * temp_proc_run                 (LigmaProcedure        *procedure,
                                                       const LigmaValueArray *args,
                                                       gpointer              run_data);

static LigmaHelpProgress * help_browser_progress_new   (void);

struct _LigmaHelpBrowser
{
  LigmaPlugIn parent_instance;

  GtkApplication *app;
  LigmaHelpBrowserDialog *window;
};

G_DEFINE_TYPE (LigmaHelpBrowser, ligma_help_browser, LIGMA_TYPE_PLUG_IN)

LIGMA_MAIN (LIGMA_TYPE_HELP_BROWSER)
DEFINE_STD_SET_I18N

static GList *
help_browser_query_procedures (LigmaPlugIn *plug_in)
{
  return g_list_append (NULL, g_strdup (LIGMA_HELP_BROWSER_EXT_PROC));
}

static LigmaProcedure *
help_browser_create_procedure (LigmaPlugIn  *plug_in,
                               const gchar *name)
{
  LigmaProcedure *procedure = NULL;

  if (! strcmp (name, LIGMA_HELP_BROWSER_EXT_PROC))
    {
      procedure = ligma_procedure_new (plug_in, name,
                                      LIGMA_PDB_PROC_TYPE_EXTENSION,
                                      help_browser_run, plug_in, NULL);

      ligma_procedure_set_documentation (procedure,
                                        "Browse the LIGMA user manual",
                                        "A small and simple HTML browser "
                                        "optimized for browsing the LIGMA "
                                        "user manual.",
                                        LIGMA_HELP_BROWSER_EXT_PROC);
      ligma_procedure_set_attribution (procedure,
                                      "Sven Neumann <sven@ligma.org>, "
                                      "Michael Natterer <mitch@ligma.org>"
                                      "Henrik Brix Andersen <brix@ligma.org>",
                                      "Sven Neumann, Michael Natterer & "
                                      "Henrik Brix Andersen",
                                      "1999-2008");

      LIGMA_PROC_ARG_ENUM (procedure, "run-mode",
                          "Run mode",
                          "The run mode",
                          LIGMA_TYPE_RUN_MODE,
                          LIGMA_RUN_INTERACTIVE,
                          G_PARAM_READWRITE);

      LIGMA_PROC_ARG_STRV (procedure, "domain-names",
                          "Domain names",
                          "Domain names",
                          G_PARAM_READWRITE);

      LIGMA_PROC_ARG_STRV (procedure, "domain-uris",
                          "Domain URIs",
                          "Domain URIs",
                          G_PARAM_READWRITE);
    }

  return procedure;
}

static void
on_app_activate (GApplication *gapp, gpointer user_data)
{
  LigmaHelpBrowser *browser = LIGMA_HELP_BROWSER (user_data);
  GtkApplication *app = GTK_APPLICATION (gapp);

  browser->window = ligma_help_browser_dialog_new (PLUG_IN_BINARY, gapp);

  gtk_application_set_accels_for_action (app, "win.back", (const char*[]) { "<alt>Left", NULL });
  gtk_application_set_accels_for_action (app, "win.forward", (const char*[]) { "<alt>Right", NULL });
  gtk_application_set_accels_for_action (app, "win.reload", (const char*[]) { "<control>R", NULL });
  gtk_application_set_accels_for_action (app, "win.stop", (const char*[]) { "Escape", NULL });
  gtk_application_set_accels_for_action (app, "win.home", (const char*[]) { "<alt>Home", NULL });
  gtk_application_set_accels_for_action (app, "win.copy-selection", (const char*[]) { "<control>C", NULL });
  gtk_application_set_accels_for_action (app, "win.zoom-in", (const char*[]) { "<control>plus", NULL });
  gtk_application_set_accels_for_action (app, "win.zoom-out", (const char*[]) { "<control>minus", NULL });
  gtk_application_set_accels_for_action (app, "win.find", (const char*[]) { "<control>F", NULL });
  gtk_application_set_accels_for_action (app, "win.find-again", (const char*[]) { "<control>G", NULL });
  gtk_application_set_accels_for_action (app, "win.close", (const char*[]) { "<control>W", "<control>Q", NULL });
  gtk_application_set_accels_for_action (app, "win.show-index", (const char*[]) { "<control>I", NULL });
}

static LigmaValueArray *
help_browser_run (LigmaProcedure        *procedure,
                  const LigmaValueArray *args,
                  gpointer              user_data)
{
  LigmaHelpBrowser *browser = LIGMA_HELP_BROWSER (user_data);

  if (! ligma_help_init (LIGMA_VALUES_GET_STRV (args, 1),
                        LIGMA_VALUES_GET_STRV (args, 2)))
    {
      return ligma_procedure_new_return_values (procedure,
                                               LIGMA_PDB_CALLING_ERROR,
                                               NULL);
    }

  temp_proc_install (ligma_procedure_get_plug_in (procedure));

  ligma_procedure_extension_ready (procedure);
  ligma_plug_in_extension_enable (ligma_procedure_get_plug_in (procedure));

  browser->app = gtk_application_new (NULL, G_APPLICATION_FLAGS_NONE);
  g_signal_connect (browser->app, "activate", G_CALLBACK (on_app_activate), browser);

  g_application_run (G_APPLICATION (browser->app), 0, NULL);

  g_clear_object (&browser->app);

  return ligma_procedure_new_return_values (procedure, LIGMA_PDB_SUCCESS, NULL);
}

static void
temp_proc_install (LigmaPlugIn *plug_in)
{
  LigmaProcedure *procedure;

  procedure = ligma_procedure_new (plug_in, LIGMA_HELP_BROWSER_TEMP_EXT_PROC,
                                  LIGMA_PDB_PROC_TYPE_TEMPORARY,
                                  temp_proc_run, plug_in, NULL);

  ligma_procedure_set_documentation (procedure,
                                    "DON'T USE THIS ONE",
                                    "(Temporary procedure)",
                                    NULL);
  ligma_procedure_set_attribution (procedure,
                                  "Sven Neumann <sven@ligma.org>, "
                                  "Michael Natterer <mitch@ligma.org>"
                                  "Henrik Brix Andersen <brix@ligma.org>",
                                  "Sven Neumann, Michael Natterer & "
                                  "Henrik Brix Andersen",
                                  "1999-2008");

  LIGMA_PROC_ARG_STRING (procedure, "help-domain",
                        "Help domain",
                        "Help domain to use",
                        NULL,
                        G_PARAM_READWRITE);

  LIGMA_PROC_ARG_STRING (procedure, "help-locales",
                        "Help locales",
                        "Language to use",
                        NULL,
                        G_PARAM_READWRITE);

  LIGMA_PROC_ARG_STRING (procedure, "help-id",
                        "Help ID",
                        "Help ID to open",
                        NULL,
                        G_PARAM_READWRITE);

  ligma_plug_in_add_temp_procedure (plug_in, procedure);
  g_object_unref (procedure);
}

typedef struct _IdleClosure
{
  LigmaHelpBrowser *browser;
  char            *help_domain;
  char            *help_locales;
  char            *help_id;
} IdleClosure;

static void
idle_closure_free (gpointer data)
{
  IdleClosure *closure = data;

  g_free (closure->help_domain);
  g_free (closure->help_locales);
  g_free (closure->help_id);
  g_free (closure);
}

static gboolean
show_help_on_idle (gpointer user_data)
{
  IdleClosure      *closure = user_data;
  LigmaHelpDomain   *domain;
  LigmaHelpProgress *progress = NULL;
  LigmaHelpLocale   *locale;
  GList            *locales;
  char             *uri;
  gboolean          fatal_error;

  /* First get the URI to load */
  domain = ligma_help_lookup_domain (closure->help_domain);
  if (!domain)
    return G_SOURCE_REMOVE;

  locales = ligma_help_parse_locales (closure->help_locales);

  if (! g_str_has_prefix (domain->help_uri, "file:"))
    progress = help_browser_progress_new ();

  uri = ligma_help_domain_map (domain, locales, closure->help_id,
                              progress, &locale, &fatal_error);

  if (progress)
    ligma_help_progress_free (progress);

  g_list_free_full (locales, (GDestroyNotify) g_free);

  /* Now actually load it */
  if (uri)
    {
      ligma_help_browser_dialog_make_index (closure->browser->window, domain, locale);
      ligma_help_browser_dialog_load (closure->browser->window, uri);

      g_free (uri);
    }

  return G_SOURCE_REMOVE;
}

static LigmaValueArray *
temp_proc_run (LigmaProcedure        *procedure,
               const LigmaValueArray *args,
               gpointer              user_data)
{
  LigmaHelpBrowser *browser = LIGMA_HELP_BROWSER (user_data);
  IdleClosure     *closure;
  const char      *str;

  closure = g_new0 (IdleClosure, 1);
  closure->browser = browser;

  str = LIGMA_VALUES_GET_STRING (args, 0);
  closure->help_domain = g_strdup ((str && *str)? str : LIGMA_HELP_DEFAULT_DOMAIN);

  str = LIGMA_VALUES_GET_STRING (args, 1);
  if (str && *str)
    closure->help_locales = g_strdup (str);

  str = LIGMA_VALUES_GET_STRING (args, 2);
  closure->help_id = g_strdup ((str && *str)? str : LIGMA_HELP_DEFAULT_ID);

  /* Do this on idle, to make sure everything is initialized already */
  g_idle_add_full (G_PRIORITY_DEFAULT_IDLE,
                   show_help_on_idle,
                   closure, idle_closure_free);

  return ligma_procedure_new_return_values (procedure, LIGMA_PDB_SUCCESS, NULL);
}


static void
help_browser_progress_start (const gchar *message,
                             gboolean     cancelable,
                             gpointer     user_data)
{
  ligma_progress_init (message);
}

static void
help_browser_progress_update (gdouble  value,
                              gpointer user_data)
{
  ligma_progress_update (value);
}

static void
help_browser_progress_end (gpointer user_data)
{
  ligma_progress_end ();
}

static LigmaHelpProgress *
help_browser_progress_new (void)
{
  static const LigmaHelpProgressVTable vtable =
  {
    help_browser_progress_start,
    help_browser_progress_end,
    help_browser_progress_update
  };

  return ligma_help_progress_new (&vtable, NULL);
}

static void
ligma_help_browser_class_init (LigmaHelpBrowserClass *klass)
{
  LigmaPlugInClass *plug_in_class = LIGMA_PLUG_IN_CLASS (klass);

  plug_in_class->query_procedures = help_browser_query_procedures;
  plug_in_class->create_procedure = help_browser_create_procedure;
  plug_in_class->set_i18n         = STD_SET_I18N;
}

static void
ligma_help_browser_init (LigmaHelpBrowser *help_browser)
{
}
