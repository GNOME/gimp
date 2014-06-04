/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimphelp.c
 * Copyright (C) 1999-2004 Michael Natterer <mitch@gimp.org>
 *                         Henrik Brix Andersen <brix@gimp.org>
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
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <string.h>

#ifdef PLATFORM_OSX
#import <Foundation/Foundation.h>
#endif

#include <gtk/gtk.h>

#include "libgimpbase/gimpbase.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "widgets-types.h"

#include "config/gimpguiconfig.h"

#include "core/gimp.h"
#include "core/gimpparamspecs.h"
#include "core/gimpprogress.h"
#include "core/gimp-utils.h"

#include "pdb/gimppdb.h"
#include "pdb/gimpprocedure.h"

#include "plug-in/gimpplugin.h"
#include "plug-in/gimppluginmanager-help-domain.h"
#include "plug-in/gimptemporaryprocedure.h"

#include "gimphelp.h"
#include "gimphelp-ids.h"
#include "gimpmessagebox.h"
#include "gimpmessagedialog.h"
#include "gimpmessagedialog.h"
#include "gimpwidgets-utils.h"

#include "gimp-log.h"
#include "gimp-intl.h"


typedef struct _GimpIdleHelp GimpIdleHelp;

struct _GimpIdleHelp
{
  Gimp         *gimp;
  GimpProgress *progress;
  gchar        *help_domain;
  gchar        *help_locales;
  gchar        *help_id;
};


/*  local function prototypes  */

static gboolean   gimp_idle_help          (GimpIdleHelp  *idle_help);
static void       gimp_idle_help_free     (GimpIdleHelp  *idle_help);

static gboolean   gimp_help_browser       (Gimp          *gimp,
					   GimpProgress  *progress);
static void       gimp_help_browser_error (Gimp          *gimp,
					   GimpProgress  *progress,
                                           const gchar   *title,
                                           const gchar   *primary,
                                           const gchar   *text);

static void       gimp_help_call          (Gimp          *gimp,
                                           GimpProgress  *progress,
                                           const gchar   *procedure_name,
                                           const gchar   *help_domain,
                                           const gchar   *help_locales,
                                           const gchar   *help_id);

static gint       gimp_help_get_help_domains         (Gimp    *gimp,
                                                      gchar ***domain_names,
                                                      gchar ***domain_uris);
static gchar    * gimp_help_get_default_domain_uri   (Gimp    *gimp);
static gchar    * gimp_help_get_locales              (Gimp    *gimp);

static gchar    * gimp_help_get_user_manual_basedir  (void);

static void       gimp_help_query_user_manual_online (GimpIdleHelp *idle_help);


/*  public functions  */

void
gimp_help_show (Gimp         *gimp,
                GimpProgress *progress,
                const gchar  *help_domain,
                const gchar  *help_id)
{
  GimpGuiConfig *config;

  g_return_if_fail (GIMP_IS_GIMP (gimp));
  g_return_if_fail (progress == NULL || GIMP_IS_PROGRESS (progress));

  config = GIMP_GUI_CONFIG (gimp->config);

  if (config->use_help)
    {
      GimpIdleHelp *idle_help = g_slice_new0 (GimpIdleHelp);

      idle_help->gimp     = gimp;
      idle_help->progress = progress;

      if (help_domain && strlen (help_domain))
        idle_help->help_domain = g_strdup (help_domain);

      idle_help->help_locales = gimp_help_get_locales (gimp);

      if (help_id && strlen (help_id))
        idle_help->help_id = g_strdup (help_id);

      GIMP_LOG (HELP, "request for help-id '%s' from help-domain '%s'",
                help_id     ? help_id      : "(null)",
                help_domain ? help_domain  : "(null)");

      g_idle_add ((GSourceFunc) gimp_idle_help, idle_help);
    }
}

gboolean
gimp_help_user_manual_is_installed (Gimp *gimp)
{
  gchar    *basedir;
  gboolean  found = FALSE;

  g_return_val_if_fail (GIMP_IS_GIMP (gimp), FALSE);

  /*  if GIMP2_HELP_URI is set, assume that the manual can be found there  */
  if (g_getenv ("GIMP2_HELP_URI"))
    return TRUE;

  basedir = gimp_help_get_user_manual_basedir ();

  if (g_file_test (basedir, G_FILE_TEST_IS_DIR))
    {
      gchar       *locales = gimp_help_get_locales (gimp);
      const gchar *s       = locales;
      const gchar *p;

      for (p = strchr (s, ':'); p && !found; p = strchr (s, ':'))
        {
          gchar *locale = g_strndup (s, p - s);
          gchar *path;

          path = g_build_filename (basedir, locale, "gimp-help.xml", NULL);

          found = g_file_test (path, G_FILE_TEST_IS_REGULAR);

          g_free (path);
          g_free (locale);

          s = p + 1;
        }

      g_free (locales);

      if (! found)
        {
          gchar *path = g_build_filename (basedir, "en", "gimp-help.xml", NULL);

          found = g_file_test (path, G_FILE_TEST_IS_REGULAR);

          g_free (path);
        }
    }

  g_free (basedir);

  return found;
}

void
gimp_help_user_manual_changed (Gimp *gimp)
{
  GimpProcedure *procedure;

  g_return_if_fail (GIMP_IS_GIMP (gimp));

  /*  Check if a help parser is running  */
  procedure = gimp_pdb_lookup_procedure (gimp->pdb, "extension-gimp-help-temp");

  if (GIMP_IS_TEMPORARY_PROCEDURE (procedure))
    {
      gimp_plug_in_close (GIMP_TEMPORARY_PROCEDURE (procedure)->plug_in, TRUE);
    }
}


/*  private functions  */

static gboolean
gimp_idle_help (GimpIdleHelp *idle_help)
{
  GimpGuiConfig *config         = GIMP_GUI_CONFIG (idle_help->gimp->config);
  const gchar   *procedure_name = NULL;

  if (! idle_help->help_domain       &&
      ! config->user_manual_online   &&
      ! gimp_help_user_manual_is_installed (idle_help->gimp))
    {
      /*  The user manual is not installed locally, ask the user
       *  if the online version should be used instead.
       */
      gimp_help_query_user_manual_online (idle_help);

      return FALSE;
    }

  if (config->help_browser == GIMP_HELP_BROWSER_GIMP)
    {
      if (gimp_help_browser (idle_help->gimp, idle_help->progress))
        procedure_name = "extension-gimp-help-browser-temp";
    }

  if (config->help_browser == GIMP_HELP_BROWSER_WEB_BROWSER)
    {
      /*  FIXME: should check for procedure availability  */
      procedure_name = "plug-in-web-browser";
    }

  if (procedure_name)
    gimp_help_call (idle_help->gimp,
                    idle_help->progress,
                    procedure_name,
                    idle_help->help_domain,
                    idle_help->help_locales,
                    idle_help->help_id);

  gimp_idle_help_free (idle_help);

  return FALSE;
}

static void
gimp_idle_help_free (GimpIdleHelp *idle_help)
{
  g_free (idle_help->help_domain);
  g_free (idle_help->help_locales);
  g_free (idle_help->help_id);

  g_slice_free (GimpIdleHelp, idle_help);
}

static gboolean
gimp_help_browser (Gimp         *gimp,
		   GimpProgress *progress)
{
  static gboolean  busy = FALSE;
  GimpProcedure   *procedure;

  if (busy)
    return TRUE;

  busy = TRUE;

  /*  Check if a help browser is already running  */
  procedure = gimp_pdb_lookup_procedure (gimp->pdb,
                                         "extension-gimp-help-browser-temp");

  if (! procedure)
    {
      GValueArray *args          = NULL;
      gint          n_domains    = 0;
      gchar       **help_domains = NULL;
      gchar       **help_uris    = NULL;
      GError       *error        = NULL;

      procedure = gimp_pdb_lookup_procedure (gimp->pdb,
                                             "extension-gimp-help-browser");

      if (! procedure)
        {
          gimp_help_browser_error (gimp, progress,
                                   _("Help browser is missing"),
                                   _("The GIMP help browser is not available."),
                                   _("The GIMP help browser plug-in appears "
                                     "to be missing from your installation. "
				     "You may instead use the web browser "
				     "for reading the help pages."));
          busy = FALSE;

          return FALSE;
        }

      n_domains = gimp_help_get_help_domains (gimp, &help_domains, &help_uris);

      args = gimp_procedure_get_arguments (procedure);
      gimp_value_array_truncate (args, 5);

      g_value_set_int             (&args->values[0], GIMP_RUN_INTERACTIVE);
      g_value_set_int             (&args->values[1], n_domains);
      gimp_value_take_stringarray (&args->values[2], help_domains, n_domains);
      g_value_set_int             (&args->values[3], n_domains);
      gimp_value_take_stringarray (&args->values[4], help_uris, n_domains);

      gimp_procedure_execute_async (procedure, gimp,
                                    gimp_get_user_context (gimp),
                                    NULL, args, NULL, &error);

      g_value_array_free (args);

      if (error)
        {
          gimp_message_literal (gimp, G_OBJECT (progress), GIMP_MESSAGE_ERROR,
				error->message);
          g_error_free (error);
        }
     }

  /*  Check if the help browser started properly  */
  procedure = gimp_pdb_lookup_procedure (gimp->pdb,
                                         "extension-gimp-help-browser-temp");

  if (! procedure)
    {
      gimp_help_browser_error (gimp, progress,
                               _("Help browser doesn't start"),
                               _("Could not start the GIMP help browser "
                                 "plug-in."),
                               NULL);
      busy = FALSE;

      return FALSE;
    }

  busy = FALSE;

  return TRUE;
}

static void
gimp_help_browser_error (Gimp         *gimp,
			 GimpProgress *progress,
                         const gchar  *title,
                         const gchar  *primary,
                         const gchar  *text)
{
  GtkWidget *dialog;

  dialog = gimp_message_dialog_new (title, GIMP_STOCK_USER_MANUAL,
				    NULL, 0,
				    NULL, NULL,

				    GTK_STOCK_CANCEL,      GTK_RESPONSE_CANCEL,
				    _("Use _Web Browser"), GTK_RESPONSE_OK,

				    NULL);

  gtk_dialog_set_alternative_button_order (GTK_DIALOG (dialog),
                                           GTK_RESPONSE_OK,
                                           GTK_RESPONSE_CANCEL,
                                           -1);

  if (progress)
    {
      guint32 window_id = gimp_progress_get_window_id (progress);

      if (window_id)
        gimp_window_set_transient_for (GTK_WINDOW (dialog), window_id);
    }

  gimp_message_box_set_primary_text (GIMP_MESSAGE_DIALOG (dialog)->box,
                                     "%s", primary);
  gimp_message_box_set_text (GIMP_MESSAGE_DIALOG (dialog)->box, "%s", text);

  if (gimp_dialog_run (GIMP_DIALOG (dialog)) == GTK_RESPONSE_OK)
    {
      g_object_set (gimp->config,
                    "help-browser", GIMP_HELP_BROWSER_WEB_BROWSER,
                    NULL);
    }

  gtk_widget_destroy (dialog);
}

static void
gimp_help_call (Gimp         *gimp,
                GimpProgress *progress,
                const gchar  *procedure_name,
                const gchar  *help_domain,
                const gchar  *help_locales,
                const gchar  *help_id)
{
  GimpProcedure *procedure;

  /*  Special case the help browser  */
  if (! strcmp (procedure_name, "extension-gimp-help-browser-temp"))
    {
      GValueArray *return_vals;
      GError      *error = NULL;

      GIMP_LOG (HELP, "Calling help via %s: %s %s %s",
                procedure_name,
                help_domain  ? help_domain  : "(null)",
                help_locales ? help_locales : "(null)",
                help_id      ? help_id      : "(null)");

      return_vals =
        gimp_pdb_execute_procedure_by_name (gimp->pdb,
                                            gimp_get_user_context (gimp),
                                            progress, &error,
                                            procedure_name,
                                            G_TYPE_STRING, help_domain,
                                            G_TYPE_STRING, help_locales,
                                            G_TYPE_STRING, help_id,
                                            G_TYPE_NONE);

      g_value_array_free (return_vals);

      if (error)
        {
          gimp_message_literal (gimp, NULL, GIMP_MESSAGE_ERROR, error->message);
          g_error_free (error);
        }

      return;
    }

  /*  Check if a help parser is already running  */
  procedure = gimp_pdb_lookup_procedure (gimp->pdb, "extension-gimp-help-temp");

  if (! procedure)
    {
      GValueArray  *args         = NULL;
      gint          n_domains    = 0;
      gchar       **help_domains = NULL;
      gchar       **help_uris    = NULL;
      GError       *error        = NULL;

      procedure = gimp_pdb_lookup_procedure (gimp->pdb, "extension-gimp-help");

      if (! procedure)
        /*  FIXME: error msg  */
        return;

      n_domains = gimp_help_get_help_domains (gimp, &help_domains, &help_uris);

      args = gimp_procedure_get_arguments (procedure);
      gimp_value_array_truncate (args, 4);

      g_value_set_int             (&args->values[0], n_domains);
      gimp_value_take_stringarray (&args->values[1], help_domains, n_domains);
      g_value_set_int             (&args->values[2], n_domains);
      gimp_value_take_stringarray (&args->values[3], help_uris, n_domains);

      gimp_procedure_execute_async (procedure, gimp,
                                    gimp_get_user_context (gimp), progress,
                                    args, NULL, &error);

      g_value_array_free (args);

      if (error)
        {
          gimp_message_literal (gimp, NULL, GIMP_MESSAGE_ERROR, error->message);
          g_error_free (error);
        }
    }

  /*  Check if the help parser started properly  */
  procedure = gimp_pdb_lookup_procedure (gimp->pdb, "extension-gimp-help-temp");

  if (procedure)
    {
      GValueArray *return_vals;
      GError      *error = NULL;

      GIMP_LOG (HELP, "Calling help via %s: %s %s %s",
                procedure_name,
                help_domain  ? help_domain  : "(null)",
                help_locales ? help_locales : "(null)",
                help_id      ? help_id      : "(null)");

      return_vals =
        gimp_pdb_execute_procedure_by_name (gimp->pdb,
                                            gimp_get_user_context (gimp),
                                            progress, &error,
                                            "extension-gimp-help-temp",
                                            G_TYPE_STRING, procedure_name,
                                            G_TYPE_STRING, help_domain,
                                            G_TYPE_STRING, help_locales,
                                            G_TYPE_STRING, help_id,
                                            G_TYPE_NONE);

      g_value_array_free (return_vals);

      if (error)
        {
          gimp_message_literal (gimp, NULL, GIMP_MESSAGE_ERROR, error->message);
          g_error_free (error);
        }
    }
}

static gint
gimp_help_get_help_domains (Gimp    *gimp,
                            gchar ***domain_names,
                            gchar ***domain_uris)
{
  gchar **plug_in_domains = NULL;
  gchar **plug_in_uris    = NULL;
  gint    i, n_domains;

  n_domains = gimp_plug_in_manager_get_help_domains (gimp->plug_in_manager,
                                                     &plug_in_domains,
                                                     &plug_in_uris);

  *domain_names = g_new0 (gchar *, n_domains + 1);
  *domain_uris  = g_new0 (gchar *, n_domains + 1);

  (*domain_names)[0] = g_strdup ("http://www.gimp.org/help");
  (*domain_uris)[0]  = gimp_help_get_default_domain_uri (gimp);

  for (i = 0; i < n_domains; i++)
    {
      (*domain_names)[i + 1] = plug_in_domains[i];
      (*domain_uris)[i + 1]  = plug_in_uris[i];
    }

  g_free (plug_in_domains);
  g_free (plug_in_uris);

  return n_domains + 1;
}

static gchar *
gimp_help_get_default_domain_uri (Gimp *gimp)
{
  GimpGuiConfig *config = GIMP_GUI_CONFIG (gimp->config);
  gchar         *dir;
  gchar         *uri;

  if (g_getenv ("GIMP2_HELP_URI"))
    return g_strdup (g_getenv ("GIMP2_HELP_URI"));

  if (config->user_manual_online)
    return g_strdup (config->user_manual_online_uri);

  dir = gimp_help_get_user_manual_basedir ();
  uri = g_filename_to_uri (dir, NULL, NULL);
  g_free (dir);

  return uri;
}

/**
 * gimp_help_get_locales:
 *
 * @gimp:   the GIMP instance with the configuration to read from
 *
 * Returns: a colon (:) separated list of language identifiers in descending
 * order of priority. The function evaluates in this order:
 * 1. (help-locales "") in the gimprc file
 * 2. the UI language from Preferences/Interface.
 * On OS X: If the user has set the UI language to 'System Language', then
 * the UI language from the OS X's System Preferences is returned.
 */
static gchar *
gimp_help_get_locales (Gimp *gimp)
{
  GimpGuiConfig *config = GIMP_GUI_CONFIG(gimp->config);

#ifdef PLATFORM_OSX
  NSAutoreleasePool *pool;
  NSUserDefaults *defaults;
  NSArray *langArray;
  NSString *languagesList;
  gchar *languages = NULL;
#endif

  /* First check for preferred help locales in gimprc. */
  if (config->help_locales && strlen (config->help_locales))
    return g_strdup (config->help_locales);

  /* If no help locale is preferred, use the locale from Preferences/Interface*/
#ifdef PLATFORM_OSX
  /* If GIMP's preferred locale is set to 'System language', then
   * g_get_language_names()[0] is 'C' and we have to get the value from
   * the OS X System Preferences.
   */
  if (!strcmp(g_get_language_names()[0],"C"))
    {
      pool = [[NSAutoreleasePool alloc] init];

      defaults = [NSUserDefaults standardUserDefaults];
      langArray = [defaults stringArrayForKey: @"AppleLanguages"];
      /* Although we only need the first entry, get them all. */
      languagesList = [langArray componentsJoinedByString: @":"];

      /* Translate them from Mac OS X (new UNIX style) to old UNIX style.*/
       languagesList = [languagesList
       stringByReplacingOccurrencesOfString: @"az-Latn"
       withString: @"az"];

       languagesList = [languagesList
       stringByReplacingOccurrencesOfString: @"ga-dots"
       withString: @"ga"];

       languagesList = [languagesList
       stringByReplacingOccurrencesOfString: @"mn-Cyrl"
       withString: @"mn"];

       languagesList = [languagesList
       stringByReplacingOccurrencesOfString: @"ms-Latn"
       withString: @"ms"];

       languagesList = [languagesList
       stringByReplacingOccurrencesOfString: @"tg-Cyrl"
       withString: @"tz"];

       languagesList = [languagesList
       stringByReplacingOccurrencesOfString: @"tt-Cyrl"
       withString: @"tt"];

       languagesList = [languagesList
       stringByReplacingOccurrencesOfString: @"zh-Hans"
       withString: @"zh_CN"];

       languagesList = [languagesList
       stringByReplacingOccurrencesOfString: @"zh-Hant"
       withString: @"zh_TW"];

       languagesList = [languagesList
       stringByReplacingOccurrencesOfString: @"Arab"
       withString: @"arabic"];

       languagesList = [languagesList
       stringByReplacingOccurrencesOfString: @"Cyrl"
       withString: @"cyrillic"];

       languagesList = [languagesList
       stringByReplacingOccurrencesOfString: @"Mong"
       withString: @"mongolian"];

       languagesList = [languagesList
       stringByReplacingOccurrencesOfString: @"-"
       withString: @"_"];

       /* Ensure that the default language 'en' is there. */
      languagesList = [languagesList stringByAppendingString: @":en"];

      languages = g_strdup ([languagesList UTF8String]);

      [pool drain];
      return languages;
    }
  else
  return g_strjoinv (":", (gchar **) g_get_language_names ());
#else
  return g_strjoinv (":", (gchar **) g_get_language_names ());
#endif

}

static gchar *
gimp_help_get_user_manual_basedir (void)
{
  return g_build_filename (gimp_data_directory (), "help", NULL);
}


static void
gimp_help_query_online_response (GtkWidget    *dialog,
                                 gint          response,
                                 GimpIdleHelp *idle_help)
{
  gtk_widget_destroy (dialog);

  if (response == GTK_RESPONSE_ACCEPT)
    {
      g_object_set (idle_help->gimp->config,
                    "user-manual-online", TRUE,
                    NULL);

      gimp_help_show (idle_help->gimp,
                      idle_help->progress,
                      idle_help->help_domain,
                      idle_help->help_id);
    }

  gimp_idle_help_free (idle_help);
}

static void
gimp_help_query_user_manual_online (GimpIdleHelp *idle_help)
{
  GtkWidget *dialog;
  GtkWidget *button;

  dialog = gimp_message_dialog_new (_("GIMP user manual is missing"),
                                    GIMP_STOCK_USER_MANUAL,
                                    NULL, 0, NULL, NULL,
                                    GTK_STOCK_CANCEL,  GTK_RESPONSE_CANCEL,
                                    NULL);

  button = gtk_dialog_add_button (GTK_DIALOG (dialog),
                                  _("_Read Online"), GTK_RESPONSE_ACCEPT);
  gtk_button_set_image (GTK_BUTTON (button),
                        gtk_image_new_from_stock (GIMP_STOCK_WEB,
                                                  GTK_ICON_SIZE_BUTTON));

  gtk_dialog_set_alternative_button_order (GTK_DIALOG (dialog),
                                           GTK_RESPONSE_ACCEPT,
                                           GTK_RESPONSE_CANCEL,
                                           -1);
  gtk_dialog_set_default_response (GTK_DIALOG (dialog), GTK_RESPONSE_ACCEPT);

  if (idle_help->progress)
    {
      guint32 window_id = gimp_progress_get_window_id (idle_help->progress);

      if (window_id)
        gimp_window_set_transient_for (GTK_WINDOW (dialog), window_id);
    }

  g_signal_connect (dialog, "response",
                    G_CALLBACK (gimp_help_query_online_response),
                    idle_help);

  gimp_message_box_set_primary_text (GIMP_MESSAGE_DIALOG (dialog)->box,
                                     _("The GIMP user manual is not installed "
                                       "on your computer."));
  gimp_message_box_set_text (GIMP_MESSAGE_DIALOG (dialog)->box,
                             _("You may either install the additional help "
                               "package or change your preferences to use "
                               "the online version."));

  gtk_widget_show (dialog);
}
