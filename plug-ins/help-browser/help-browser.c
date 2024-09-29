/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * The GIMP Help Browser
 * Copyright (C) 1999-2008 Sven Neumann <sven@gimp.org>
 *                         Michael Natterer <mitch@gimp.org>
 *                         Henrik Brix Andersen <brix@gimp.org>
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

#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

#include "plug-ins/help/gimphelp.h"

#include "dialog.h"

#include "libgimp/stdplugins-intl.h"


#define GIMP_HELP_BROWSER_EXT_PROC       "extension-gimp-help-browser"
#define GIMP_HELP_BROWSER_TEMP_EXT_PROC  "extension-gimp-help-browser-temp"
#define PLUG_IN_BINARY                   "help-browser"
#define PLUG_IN_ROLE                     "gimp-help-browser"


#define GIMP_TYPE_HELP_BROWSER (gimp_help_browser_get_type ())
G_DECLARE_FINAL_TYPE (GimpHelpBrowser, gimp_help_browser,
                      GIMP, HELP_BROWSER,
                      GimpPlugIn)

static GimpValueArray * help_browser_run              (GimpProcedure        *procedure,
                                                       GimpProcedureConfig  *config,
                                                       gpointer              run_data);

static void             temp_proc_install             (GimpPlugIn           *plug_in);
static GimpValueArray * temp_proc_run                 (GimpProcedure        *procedure,
                                                       GimpProcedureConfig  *config,
                                                       gpointer              run_data);

static GimpHelpProgress * help_browser_progress_new   (void);

struct _GimpHelpBrowser
{
  GimpPlugIn parent_instance;

  GimpProcedureConfig   *config;
  GtkApplication        *app;
  GimpHelpBrowserDialog *window;
};

G_DEFINE_TYPE (GimpHelpBrowser, gimp_help_browser, GIMP_TYPE_PLUG_IN)

GIMP_MAIN (GIMP_TYPE_HELP_BROWSER)
DEFINE_STD_SET_I18N

static GList *
help_browser_query_procedures (GimpPlugIn *plug_in)
{
  return g_list_append (NULL, g_strdup (GIMP_HELP_BROWSER_EXT_PROC));
}

static GimpProcedure *
help_browser_create_procedure (GimpPlugIn  *plug_in,
                               const gchar *name)
{
  GimpProcedure *procedure = NULL;

  if (! strcmp (name, GIMP_HELP_BROWSER_EXT_PROC))
    {
      procedure = gimp_procedure_new (plug_in, name,
                                      GIMP_PDB_PROC_TYPE_PERSISTENT,
                                      help_browser_run, plug_in, NULL);

      gimp_procedure_set_documentation (procedure,
                                        "Browse the GIMP user manual",
                                        "A small and simple HTML browser "
                                        "optimized for browsing the GIMP "
                                        "user manual.",
                                        GIMP_HELP_BROWSER_EXT_PROC);
      gimp_procedure_set_attribution (procedure,
                                      "Sven Neumann <sven@gimp.org>, "
                                      "Michael Natterer <mitch@gimp.org>"
                                      "Henrik Brix Andersen <brix@gimp.org>",
                                      "Sven Neumann, Michael Natterer & "
                                      "Henrik Brix Andersen",
                                      "1999-2008");

      gimp_procedure_add_enum_argument (procedure, "run-mode",
                                        "Run mode",
                                        "The run mode",
                                        GIMP_TYPE_RUN_MODE,
                                        GIMP_RUN_INTERACTIVE,
                                        G_PARAM_READWRITE);

      gimp_procedure_add_string_array_argument (procedure, "domain-names",
                                                "Domain names",
                                                "Domain names",
                                                G_PARAM_READWRITE);

      gimp_procedure_add_string_array_argument (procedure, "domain-uris",
                                                "Domain URIs",
                                                "Domain URIs",
                                                G_PARAM_READWRITE);

      gimp_procedure_add_bytes_aux_argument (procedure, "dialog-data",
                                             "Dialog data",
                                             "Remembering dialog's basic features; this is never meant to be a public argument",
                                             GIMP_PARAM_READWRITE);
    }

  return procedure;
}

static void
on_app_activate (GApplication *gapp,
                 gpointer      user_data)
{
  GimpHelpBrowser *browser = GIMP_HELP_BROWSER (user_data);
  GtkApplication  *app     = GTK_APPLICATION (gapp);

  browser->window = gimp_help_browser_dialog_new (PLUG_IN_BINARY, gapp, browser->config);

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

static GimpValueArray *
help_browser_run (GimpProcedure        *procedure,
                  GimpProcedureConfig  *config,
                  gpointer              user_data)
{
  GimpHelpBrowser  *browser      = GIMP_HELP_BROWSER (user_data);
  gchar           **domain_names = NULL;
  gchar           **domain_uris  = NULL;

  browser->config = config;
  g_object_get (config,
                "domain-names", &domain_names,
                "domain-uris",  &domain_uris,
                NULL);
  if (! gimp_help_init ((const gchar **) domain_names,
                        (const gchar **) domain_uris))
    {
      g_strfreev (domain_names);
      g_strfreev (domain_uris);

      return gimp_procedure_new_return_values (procedure,
                                               GIMP_PDB_CALLING_ERROR,
                                               NULL);
    }
  g_strfreev (domain_names);
  g_strfreev (domain_uris);

  temp_proc_install (gimp_procedure_get_plug_in (procedure));

  gimp_procedure_persistent_ready (procedure);
  gimp_plug_in_persistent_enable (gimp_procedure_get_plug_in (procedure));

#if GLIB_CHECK_VERSION(2,74,0)
  browser->app = gtk_application_new (NULL, G_APPLICATION_DEFAULT_FLAGS);
#else
  browser->app = gtk_application_new (NULL, G_APPLICATION_FLAGS_NONE);
#endif
  g_signal_connect (browser->app, "activate", G_CALLBACK (on_app_activate), browser);

  g_application_run (G_APPLICATION (browser->app), 0, NULL);

  g_clear_object (&browser->app);

  return gimp_procedure_new_return_values (procedure, GIMP_PDB_SUCCESS, NULL);
}

static void
temp_proc_install (GimpPlugIn *plug_in)
{
  GimpProcedure *procedure;

  procedure = gimp_procedure_new (plug_in, GIMP_HELP_BROWSER_TEMP_EXT_PROC,
                                  GIMP_PDB_PROC_TYPE_TEMPORARY,
                                  temp_proc_run, plug_in, NULL);

  gimp_procedure_set_documentation (procedure,
                                    "DON'T USE THIS ONE",
                                    "(Temporary procedure)",
                                    NULL);
  gimp_procedure_set_attribution (procedure,
                                  "Sven Neumann <sven@gimp.org>, "
                                  "Michael Natterer <mitch@gimp.org>"
                                  "Henrik Brix Andersen <brix@gimp.org>",
                                  "Sven Neumann, Michael Natterer & "
                                  "Henrik Brix Andersen",
                                  "1999-2008");

  gimp_procedure_add_string_argument (procedure, "help-domain",
                                      "Help domain",
                                      "Help domain to use",
                                      NULL,
                                      G_PARAM_READWRITE);

  gimp_procedure_add_string_argument (procedure, "help-locales",
                                      "Help locales",
                                      "Language to use",
                                      NULL,
                                      G_PARAM_READWRITE);

  gimp_procedure_add_string_argument (procedure, "help-id",
                                      "Help ID",
                                      "Help ID to open",
                                      NULL,
                                      G_PARAM_READWRITE);

  gimp_plug_in_add_temp_procedure (plug_in, procedure);
  g_object_unref (procedure);
}

typedef struct _IdleClosure
{
  GimpHelpBrowser *browser;
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
  GimpHelpDomain   *domain;
  GimpHelpProgress *progress = NULL;
  GimpHelpLocale   *locale;
  GList            *locales;
  char             *uri;
  gboolean          fatal_error;

  /* First get the URI to load */
  domain = gimp_help_lookup_domain (closure->help_domain);
  if (!domain)
    return G_SOURCE_REMOVE;

  locales = gimp_help_parse_locales (closure->help_locales);

  if (! g_str_has_prefix (domain->help_uri, "file:"))
    progress = help_browser_progress_new ();

  uri = gimp_help_domain_map (domain, locales, closure->help_id,
                              progress, &locale, &fatal_error);

  if (progress)
    gimp_help_progress_free (progress);

  g_list_free_full (locales, (GDestroyNotify) g_free);

  /* Now actually load it */
  if (uri)
    {
      gimp_help_browser_dialog_make_index (closure->browser->window, domain, locale);
      gimp_help_browser_dialog_load (closure->browser->window, uri);

      g_free (uri);
    }

  return G_SOURCE_REMOVE;
}

static GimpValueArray *
temp_proc_run (GimpProcedure        *procedure,
               GimpProcedureConfig  *config,
               gpointer              user_data)
{
  GimpHelpBrowser *browser = GIMP_HELP_BROWSER (user_data);
  IdleClosure     *closure;
  gchar           *str;

  closure = g_new0 (IdleClosure, 1);
  closure->browser = browser;

  g_object_get (config, "help-domain", &str, NULL);
  closure->help_domain = g_strdup ((str && *str)? str : GIMP_HELP_DEFAULT_DOMAIN);
  g_free (str);

  g_object_get (config, "help-locales", &str, NULL);
  if (str && *str)
    closure->help_locales = g_strdup (str);
  g_free (str);

  g_object_get (config, "help-id", &str, NULL);
  closure->help_id = g_strdup ((str && *str)? str : GIMP_HELP_DEFAULT_ID);
  g_free (str);

  /* Do this on idle, to make sure everything is initialized already */
  g_idle_add_full (G_PRIORITY_DEFAULT_IDLE,
                   show_help_on_idle,
                   closure, idle_closure_free);

  return gimp_procedure_new_return_values (procedure, GIMP_PDB_SUCCESS, NULL);
}


static void
help_browser_progress_start (const gchar *message,
                             gboolean     cancelable,
                             gpointer     user_data)
{
  gimp_progress_init (message);
}

static void
help_browser_progress_update (gdouble  value,
                              gpointer user_data)
{
  gimp_progress_update (value);
}

static void
help_browser_progress_end (gpointer user_data)
{
  gimp_progress_end ();
}

static GimpHelpProgress *
help_browser_progress_new (void)
{
  static const GimpHelpProgressVTable vtable =
  {
    help_browser_progress_start,
    help_browser_progress_end,
    help_browser_progress_update
  };

  return gimp_help_progress_new (&vtable, NULL);
}

static void
gimp_help_browser_class_init (GimpHelpBrowserClass *klass)
{
  GimpPlugInClass *plug_in_class = GIMP_PLUG_IN_CLASS (klass);

  plug_in_class->query_procedures = help_browser_query_procedures;
  plug_in_class->create_procedure = help_browser_create_procedure;
  plug_in_class->set_i18n         = STD_SET_I18N;
}

static void
gimp_help_browser_init (GimpHelpBrowser *help_browser)
{
}
