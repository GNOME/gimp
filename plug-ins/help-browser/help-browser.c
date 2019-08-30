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


typedef struct _HelpBrowser      HelpBrowser;
typedef struct _HelpBrowserClass HelpBrowserClass;

struct _HelpBrowser
{
  GimpPlugIn      parent_instance;
};

struct _HelpBrowserClass
{
  GimpPlugInClass parent_class;
};


#define HELP_BROWSER_TYPE  (help_browser_get_type ())
#define HELP_BROWSER (obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), HELP_BROWSER_TYPE, HelpBrowser))

GType                   help_browser_get_type         (void) G_GNUC_CONST;

static GList          * help_browser_query_procedures (GimpPlugIn           *plug_in);
static GimpProcedure  * help_browser_create_procedure (GimpPlugIn           *plug_in,
                                                       const gchar          *name);

static GimpValueArray * help_browser_run              (GimpProcedure        *procedure,
                                                       const GimpValueArray *args,
                                                       gpointer              run_data);

static void             temp_proc_install             (GimpPlugIn           *plug_in);
static GimpValueArray * temp_proc_run                 (GimpProcedure        *procedure,
                                                       const GimpValueArray *args,
                                                       gpointer              run_data);

static gboolean         help_browser_show_help        (const gchar          *help_domain,
                                                       const gchar          *help_locales,
                                                       const gchar          *help_id);

static GimpHelpProgress * help_browser_progress_new   (void);



G_DEFINE_TYPE (HelpBrowser, help_browser, GIMP_TYPE_PLUG_IN)

GIMP_MAIN (HELP_BROWSER_TYPE)


static void
help_browser_class_init (HelpBrowserClass *klass)
{
  GimpPlugInClass *plug_in_class = GIMP_PLUG_IN_CLASS (klass);

  plug_in_class->query_procedures = help_browser_query_procedures;
  plug_in_class->create_procedure = help_browser_create_procedure;
}

static void
help_browser_init (HelpBrowser *help_browser)
{
}

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
                                      GIMP_PDB_PROC_TYPE_EXTENSION,
                                      help_browser_run, NULL, NULL);

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

      GIMP_PROC_ARG_ENUM (procedure, "run-mode",
                          "Run mode",
                          "The run mode",
                          GIMP_TYPE_RUN_MODE,
                          GIMP_RUN_INTERACTIVE,
                          G_PARAM_READWRITE);

      GIMP_PROC_ARG_INT (procedure, "num-domain-names",
                         "Num domain names",
                         "Num domain names",
                         0, G_MAXINT, 0,
                         G_PARAM_READWRITE);

      GIMP_PROC_ARG_STRING_ARRAY (procedure, "domain-names",
                                  "Domain names",
                                  "Domain names",
                                  G_PARAM_READWRITE);

      GIMP_PROC_ARG_INT (procedure, "num-domain-uris",
                         "Num domain URIs",
                         "Num domain URIs",
                         0, G_MAXINT, 0,
                         G_PARAM_READWRITE);

      GIMP_PROC_ARG_STRING_ARRAY (procedure, "domain-uris",
                                  "Domain URIs",
                                  "Domain URIs",
                                  G_PARAM_READWRITE);
    }

  return procedure;
}

static GimpValueArray *
help_browser_run (GimpProcedure        *procedure,
                  const GimpValueArray *args,
                  gpointer              run_data)
{
  INIT_I18N ();

  if (! gimp_help_init (GIMP_VALUES_GET_INT          (args, 1),
                        GIMP_VALUES_GET_STRING_ARRAY (args, 2),
                        GIMP_VALUES_GET_INT          (args, 3),
                        GIMP_VALUES_GET_STRING_ARRAY (args, 4)))
    {
      return gimp_procedure_new_return_values (procedure,
                                               GIMP_PDB_CALLING_ERROR,
                                               NULL);
    }

  browser_dialog_open (PLUG_IN_BINARY);

  temp_proc_install (gimp_procedure_get_plug_in (procedure));

  gimp_procedure_extension_ready (procedure);
  gimp_plug_in_extension_enable (gimp_procedure_get_plug_in (procedure));

  gtk_main ();

  return gimp_procedure_new_return_values (procedure, GIMP_PDB_SUCCESS, NULL);
}

static void
temp_proc_install (GimpPlugIn *plug_in)
{
  GimpProcedure *procedure;

  procedure = gimp_procedure_new (plug_in, GIMP_HELP_BROWSER_TEMP_EXT_PROC,
                                  GIMP_PDB_PROC_TYPE_TEMPORARY,
                                  temp_proc_run, NULL, NULL);

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

  GIMP_PROC_ARG_STRING (procedure, "help-domain",
                        "Help domain",
                        "Help domain to use",
                        NULL,
                        G_PARAM_READWRITE);

  GIMP_PROC_ARG_STRING (procedure, "help-locales",
                        "Help locales",
                        "Language to use",
                        NULL,
                        G_PARAM_READWRITE);

  GIMP_PROC_ARG_STRING (procedure, "help-id",
                        "Help ID",
                        "Help ID to open",
                        NULL,
                        G_PARAM_READWRITE);

  gimp_plug_in_add_temp_procedure (plug_in, procedure);
  g_object_unref (procedure);
}

static GimpValueArray *
temp_proc_run (GimpProcedure        *procedure,
               const GimpValueArray *args,
               gpointer              run_data)
{
  const gchar *help_domain  = GIMP_HELP_DEFAULT_DOMAIN;
  const gchar *help_locales = NULL;
  const gchar *help_id      = GIMP_HELP_DEFAULT_ID;
  const gchar *string;

  string = GIMP_VALUES_GET_STRING (args, 0);
  if (string && strlen (string))
    help_domain = string;

  string = GIMP_VALUES_GET_STRING (args, 1);
  if (string && strlen (string))
    help_locales = string;

  string = GIMP_VALUES_GET_STRING (args, 2);
  if (string && strlen (string))
    help_id = string;

  if (! help_browser_show_help (help_domain, help_locales, help_id))
    {
      gtk_main_quit ();
    }

  return gimp_procedure_new_return_values (procedure, GIMP_PDB_SUCCESS, NULL);
}

static gboolean
help_browser_show_help (const gchar *help_domain,
                        const gchar *help_locales,
                        const gchar *help_id)
{
  GimpHelpDomain *domain;
  gboolean        success = TRUE;

  domain = gimp_help_lookup_domain (help_domain);

  if (domain)
    {
      GimpHelpProgress *progress = NULL;
      GimpHelpLocale   *locale;
      GList            *locales;
      gchar            *uri;
      gboolean          fatal_error;

      locales = gimp_help_parse_locales (help_locales);

      if (! g_str_has_prefix (domain->help_uri, "file:"))
        progress = help_browser_progress_new ();

      uri = gimp_help_domain_map (domain, locales, help_id,
                                  progress, &locale, &fatal_error);

      if (progress)
        gimp_help_progress_free (progress);

      g_list_free_full (locales, (GDestroyNotify) g_free);

      if (uri)
        {
          browser_dialog_make_index (domain, locale);
          browser_dialog_load (uri);

          g_free (uri);
        }
      else if (fatal_error)
        {
          success = FALSE;
        }
    }

  return success;
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
