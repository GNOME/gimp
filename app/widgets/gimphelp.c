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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <string.h>

#include <gegl.h>
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
#include "gimplanguagecombobox.h"
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

  GtkDialog    *query_dialog;
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

static void       gimp_help_get_help_domains         (Gimp    *gimp,
                                                      gchar ***domain_names,
                                                      gchar ***domain_uris);
static gchar    * gimp_help_get_default_domain_uri   (Gimp    *gimp);
static gchar    * gimp_help_get_locales              (Gimp    *gimp);

static GFile    * gimp_help_get_user_manual_basedir  (void);

static void       gimp_help_query_alt_user_manual    (GimpIdleHelp *idle_help);

static void       gimp_help_language_combo_changed   (GtkComboBox  *combo,
                                                      GimpIdleHelp *idle_help);

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
gimp_help_browser_is_installed (Gimp *gimp)
{
  g_return_val_if_fail (GIMP_IS_GIMP (gimp), FALSE);

  if (gimp_pdb_lookup_procedure (gimp->pdb, "extension-gimp-help-browser"))
    return TRUE;

  return FALSE;
}

gboolean
gimp_help_user_manual_is_installed (Gimp *gimp)
{
  GFile    *basedir;
  gboolean  found = FALSE;

  g_return_val_if_fail (GIMP_IS_GIMP (gimp), FALSE);

  /*  if GIMP2_HELP_URI is set, assume that the manual can be found there  */
  if (g_getenv ("GIMP2_HELP_URI"))
    return TRUE;

  basedir = gimp_help_get_user_manual_basedir ();

  if (g_file_query_file_type (basedir, G_FILE_QUERY_INFO_NONE, NULL) ==
      G_FILE_TYPE_DIRECTORY)
    {
      gchar       *locales = gimp_help_get_locales (gimp);
      const gchar *s       = locales;
      const gchar *p;

      for (p = strchr (s, ':'); p && !found; p = strchr (s, ':'))
        {
          gchar *locale = g_strndup (s, p - s);
          GFile *file1  = g_file_get_child (basedir, locale);
          GFile *file2  = g_file_get_child (file1, "gimp-help.xml");

          found = (g_file_query_file_type (file2, G_FILE_QUERY_INFO_NONE,
                                           NULL) == G_FILE_TYPE_REGULAR);

          g_object_unref (file1);
          g_object_unref (file2);
          g_free (locale);

          s = p + 1;
        }

      g_free (locales);

      if (! found)
        {
          GFile *file1  = g_file_get_child (basedir, "en");
          GFile *file2  = g_file_get_child (file1, "gimp-help.xml");

          found = (g_file_query_file_type (file2, G_FILE_QUERY_INFO_NONE,
                                           NULL) == G_FILE_TYPE_REGULAR);

          g_object_unref (file1);
          g_object_unref (file2);
        }
    }

  g_object_unref (basedir);

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

GList *
gimp_help_get_installed_languages (void)
{
  GList *manuals = NULL;
  GFile *basedir;

  /*  if GIMP2_HELP_URI is set, assume that the manual can be found there  */
  if (g_getenv ("GIMP2_HELP_URI"))
    basedir = g_file_new_for_uri (g_getenv ("GIMP2_HELP_URI"));
  else
    basedir = gimp_help_get_user_manual_basedir ();

  if (g_file_query_file_type (basedir, G_FILE_QUERY_INFO_NONE, NULL) ==
      G_FILE_TYPE_DIRECTORY)
    {
      GFileEnumerator *enumerator;

      enumerator = g_file_enumerate_children (basedir,
                                              G_FILE_ATTRIBUTE_STANDARD_NAME ","
                                              G_FILE_ATTRIBUTE_STANDARD_TYPE,
                                              G_FILE_QUERY_INFO_NONE,
                                              NULL, NULL);

      if (enumerator)
        {
          GFileInfo *info;

          while ((info = g_file_enumerator_next_file (enumerator,
                                                      NULL, NULL)))
            {
              if (g_file_info_get_attribute_uint32 (info, G_FILE_ATTRIBUTE_STANDARD_TYPE) == G_FILE_TYPE_DIRECTORY)
                {
                  GFile *locale_dir;
                  GFile *file;

                  locale_dir = g_file_enumerator_get_child (enumerator, info);
                  file  = g_file_get_child (locale_dir, "gimp-help.xml");
                  if (g_file_query_file_type (file, G_FILE_QUERY_INFO_NONE,
                                              NULL) == G_FILE_TYPE_REGULAR)
                    {
                      manuals = g_list_prepend (manuals,
                                                g_strdup (g_file_info_get_name (info)));
                    }
                  g_object_unref (locale_dir);
                  g_object_unref (file);
                }
              g_object_unref (info);
            }
          g_object_unref (enumerator);
        }
    }
  g_object_unref (basedir);

  return manuals;
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
      /*  The user manual is not installed locally, propose alternative
       *  manuals (other installed languages or online version).
       */
      gimp_help_query_alt_user_manual (idle_help);

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
      GimpValueArray *args         = NULL;
      gchar         **help_domains = NULL;
      gchar         **help_uris    = NULL;
      GError         *error        = NULL;

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

      gimp_help_get_help_domains (gimp, &help_domains, &help_uris);

      args = gimp_procedure_get_arguments (procedure);
      gimp_value_array_truncate (args, 3);

      g_value_set_enum (gimp_value_array_index (args, 0), GIMP_RUN_INTERACTIVE);
      g_value_take_boxed (gimp_value_array_index (args, 1), help_domains);
      g_value_take_boxed (gimp_value_array_index (args, 2), help_uris);

      gimp_procedure_execute_async (procedure, gimp,
                                    gimp_get_user_context (gimp),
                                    NULL, args, NULL, &error);

      gimp_value_array_unref (args);

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
                               _("You may instead use the web browser "
                                 "for reading the help pages."));
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

  dialog = gimp_message_dialog_new (title, GIMP_ICON_HELP_USER_MANUAL,
                                    NULL, 0,
                                    NULL, NULL,

                                    _("_Cancel"),          GTK_RESPONSE_CANCEL,
                                    _("Use _Web Browser"), GTK_RESPONSE_OK,

                                    NULL);

  gimp_dialog_set_alternative_button_order (GTK_DIALOG (dialog),
                                           GTK_RESPONSE_OK,
                                           GTK_RESPONSE_CANCEL,
                                           -1);

  if (progress)
    gimp_window_set_transient_for (GTK_WINDOW (dialog), progress);

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
      GimpValueArray *return_vals;
      GError         *error = NULL;

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

      gimp_value_array_unref (return_vals);

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
      GimpValueArray  *args         = NULL;
      gchar          **help_domains = NULL;
      gchar          **help_uris    = NULL;
      GError          *error        = NULL;

      procedure = gimp_pdb_lookup_procedure (gimp->pdb, "extension-gimp-help");

      if (! procedure)
        /*  FIXME: error msg  */
        return;

      gimp_help_get_help_domains (gimp, &help_domains, &help_uris);

      args = gimp_procedure_get_arguments (procedure);
      gimp_value_array_truncate (args, 2);

      g_value_take_boxed (gimp_value_array_index (args, 0), help_domains);
      g_value_take_boxed (gimp_value_array_index (args, 1), help_uris);

      gimp_procedure_execute_async (procedure, gimp,
                                    gimp_get_user_context (gimp), progress,
                                    args, NULL, &error);

      gimp_value_array_unref (args);

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
      GimpValueArray *return_vals;
      GError         *error = NULL;

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

      gimp_value_array_unref (return_vals);

      if (error)
        {
          gimp_message_literal (gimp, NULL, GIMP_MESSAGE_ERROR, error->message);
          g_error_free (error);
        }
    }
}

static void
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

  *domain_names = g_new0 (gchar *, n_domains + 2);
  *domain_uris  = g_new0 (gchar *, n_domains + 2);

  (*domain_names)[0] = g_strdup ("https://www.gimp.org/help");
  (*domain_uris)[0]  = gimp_help_get_default_domain_uri (gimp);

  for (i = 0; i < n_domains; i++)
    {
      (*domain_names)[i + 1] = plug_in_domains[i];
      (*domain_uris)[i + 1]  = plug_in_uris[i];
    }

  g_free (plug_in_domains);
  g_free (plug_in_uris);
}

static gchar *
gimp_help_get_default_domain_uri (Gimp *gimp)
{
  GimpGuiConfig *config = GIMP_GUI_CONFIG (gimp->config);
  GFile         *dir;
  gchar         *uri;

  if (g_getenv ("GIMP2_HELP_URI"))
    return g_strdup (g_getenv ("GIMP2_HELP_URI"));

  if (config->user_manual_online)
    return g_strdup (config->user_manual_online_uri);

  dir = gimp_help_get_user_manual_basedir ();
  uri = g_file_get_uri (dir);
  g_object_unref (dir);

  return uri;
}

static gchar *
gimp_help_get_locales (Gimp *gimp)
{
  GimpGuiConfig  *config = GIMP_GUI_CONFIG (gimp->config);
  gchar         **names;
  gchar          *locales      = NULL;
  GList          *locales_list = NULL;
  GList          *iter;
  gint            i;

  if (config->help_locales && strlen (config->help_locales))
    return g_strdup (config->help_locales);

  /* Process locales. */
  names = (gchar **) g_get_language_names ();
  for (i = 0; names[i]; i++)
    {
      gchar *locale = g_strdup (names[i]);
      gchar *c;

      /* We don't care about encoding in context of our help system. */
      c = strchr (locale, '.');
      if (c)
        *c = '\0';

      /* We don't care about variants either. */
      c = strchr (locale, '@');
      if (c)
        *c = '\0';

      /* Apparently some systems (i.e. Windows) would return a value as
       * IETF language tag, which is a different format from POSIX
       * locale; especially it would separate the lang and the region
       * with an hyphen instead of an underscore.
       * Actually the difference is much deeper, and IETF language tags
       * can have extended language subtags, a script subtag, variants,
       * moreover using different codes.
       * We'd actually need to look into this in details (TODO).
       * this dirty hack should do for easy translation at least (like
       * "en-GB" -> "en_GB).
       * Cf. bug 777754.
       */
      c = strchr (locale, '-');
      if (c)
        *c = '_';

      if (locale && *locale &&
          ! g_list_find_custom (locales_list, locale,
                                (GCompareFunc) g_strcmp0))
        {
          gchar *base;

          /* Adding this locale. */
          locales_list = g_list_prepend (locales_list, locale);

          /* Adding the base language as well. */
          base = strdup (locale);
          c = strchr (base, '_');
          if (c)
            *c = '\0';

          if (base && *base &&
              ! g_list_find_custom (locales_list, base,
                                    (GCompareFunc) g_strcmp0))
            {
              locales_list = g_list_prepend (locales_list, base);
            }
          else
            {
              g_free (base);
            }
        }
      else
        {
          g_free (locale);
        }
    }

  locales_list = g_list_reverse (locales_list);

  /* Finally generate the colon-separated value. */
  if (locales_list)
    {
      locales = g_strdup (locales_list->data);
      for (iter = locales_list->next; iter; iter = iter->next)
        {
          gchar *temp = locales;
          locales = g_strconcat (temp, ":", iter->data, NULL);
          g_free (temp);
        }
    }

  g_list_free_full (locales_list, g_free);

  return locales;
}

static GFile *
gimp_help_get_user_manual_basedir (void)
{
  return gimp_data_directory_file ("help", NULL);
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
    }
  if (response != GTK_RESPONSE_YES)
    {
      g_object_set (idle_help->gimp->config,
                    "help-locales", "",
                    NULL);
    }

  if (response == GTK_RESPONSE_ACCEPT ||
      response == GTK_RESPONSE_YES)
    {
      gimp_help_show (idle_help->gimp,
                      idle_help->progress,
                      idle_help->help_domain,
                      idle_help->help_id);
    }

  gimp_idle_help_free (idle_help);
}

static void
gimp_help_query_alt_user_manual (GimpIdleHelp *idle_help)
{
  GtkWidget *dialog;
  GList     *manuals;

  dialog = gimp_message_dialog_new (_("GIMP user manual is missing"),
                                    GIMP_ICON_HELP_USER_MANUAL,
                                    NULL, 0, NULL, NULL,
                                    _("_Cancel"), GTK_RESPONSE_CANCEL,
                                    NULL);
  idle_help->query_dialog = GTK_DIALOG (dialog);

  if (idle_help->progress)
    gimp_window_set_transient_for (GTK_WINDOW (dialog), idle_help->progress);

  gimp_message_box_set_primary_text (GIMP_MESSAGE_DIALOG (dialog)->box,
                                     _("The GIMP user manual is not installed "
                                       "in your language."));

  /* Add a list of available manuals instead, if any. */
  manuals = gimp_help_get_installed_languages ();
  if (manuals != NULL)
    {
      GtkWidget *lang_combo;

      /* Add an additional button. */
      gtk_dialog_add_button (GTK_DIALOG (dialog),
                             _("Read Selected _Language"),
                             GTK_RESPONSE_YES);
      /* And a dropdown list of available manuals. */
      lang_combo = gimp_language_combo_box_new (TRUE,
                                                _("Available manuals..."));
      gtk_combo_box_set_active (GTK_COMBO_BOX (lang_combo), 0);
      gtk_dialog_set_response_sensitive (idle_help->query_dialog,
                                         GTK_RESPONSE_YES, FALSE);
      g_signal_connect (lang_combo, "changed",
                        G_CALLBACK (gimp_help_language_combo_changed),
                        idle_help);
      gtk_box_pack_start (GTK_BOX (gtk_dialog_get_content_area (GTK_DIALOG (dialog))),
                          lang_combo, TRUE, TRUE, 0);
      gtk_widget_show (lang_combo);

      gimp_message_box_set_text (GIMP_MESSAGE_DIALOG (dialog)->box,
                                 _("You may either select a manual in another "
                                   "language or read the online version."));
    }
  else
    {
      gimp_message_box_set_text (GIMP_MESSAGE_DIALOG (dialog)->box,
                                 _("You may either install the additional help "
                                   "package or change your preferences to use "
                                   "the online version."));
    }
  gtk_dialog_add_button (GTK_DIALOG (dialog),
                         _("Read _Online"), GTK_RESPONSE_ACCEPT);
  gtk_dialog_set_default_response (GTK_DIALOG (dialog), GTK_RESPONSE_ACCEPT);
  if (manuals != NULL)
    {
      gimp_dialog_set_alternative_button_order (GTK_DIALOG (dialog),
                                               GTK_RESPONSE_ACCEPT,
                                               GTK_RESPONSE_YES,
                                               GTK_RESPONSE_CANCEL,
                                               -1);
      g_list_free_full (manuals, g_free);
    }
  else
    {
      gimp_dialog_set_alternative_button_order (GTK_DIALOG (dialog),
                                               GTK_RESPONSE_ACCEPT,
                                               GTK_RESPONSE_CANCEL,
                                               -1);
    }
  g_signal_connect (dialog, "response",
                    G_CALLBACK (gimp_help_query_online_response),
                    idle_help);
  gtk_widget_show (dialog);
}

static void
gimp_help_language_combo_changed (GtkComboBox  *combo,
                                  GimpIdleHelp *idle_help)
{
  gchar *help_locales = NULL;
  gchar *code;

  code = gimp_language_combo_box_get_code (GIMP_LANGUAGE_COMBO_BOX (combo));
  if (code && g_strcmp0 ("", code) != 0)
    {
      help_locales = g_strdup_printf ("%s:", code);
      gtk_dialog_set_response_sensitive (idle_help->query_dialog,
                                         GTK_RESPONSE_YES, TRUE);
    }
  else
    {
      gtk_dialog_set_response_sensitive (idle_help->query_dialog,
                                         GTK_RESPONSE_YES, FALSE);
    }
  g_object_set (idle_help->gimp->config,
                "help-locales", help_locales? help_locales : "",
                NULL);

  g_free (code);
  if (help_locales)
    g_free (help_locales);
}
