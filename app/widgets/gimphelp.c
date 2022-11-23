/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ligmahelp.c
 * Copyright (C) 1999-2004 Michael Natterer <mitch@ligma.org>
 *                         Henrik Brix Andersen <brix@ligma.org>
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

#include "libligmabase/ligmabase.h"
#include "libligmawidgets/ligmawidgets.h"

#include "widgets-types.h"

#include "config/ligmaguiconfig.h"

#include "core/ligma.h"
#include "core/ligmaparamspecs.h"
#include "core/ligmaprogress.h"
#include "core/ligma-utils.h"

#include "pdb/ligmapdb.h"
#include "pdb/ligmaprocedure.h"

#include "plug-in/ligmaplugin.h"
#include "plug-in/ligmapluginmanager-help-domain.h"
#include "plug-in/ligmatemporaryprocedure.h"

#include "ligmahelp.h"
#include "ligmahelp-ids.h"
#include "ligmalanguagecombobox.h"
#include "ligmalanguagestore-parser.h"
#include "ligmamessagebox.h"
#include "ligmamessagedialog.h"
#include "ligmamessagedialog.h"
#include "ligmawidgets-utils.h"

#include "ligma-log.h"
#include "ligma-intl.h"


typedef struct _LigmaIdleHelp LigmaIdleHelp;

struct _LigmaIdleHelp
{
  Ligma         *ligma;
  LigmaProgress *progress;
  gchar        *help_domain;
  gchar        *help_locales;
  gchar        *help_id;

  GtkDialog    *query_dialog;
};


/*  local function prototypes  */

static gboolean   ligma_idle_help          (LigmaIdleHelp  *idle_help);
static void       ligma_idle_help_free     (LigmaIdleHelp  *idle_help);

static gboolean   ligma_help_browser       (Ligma          *ligma,
                                           LigmaProgress  *progress);
static void       ligma_help_browser_error (Ligma          *ligma,
                                           LigmaProgress  *progress,
                                           const gchar   *title,
                                           const gchar   *primary,
                                           const gchar   *text);

static void       ligma_help_call          (Ligma          *ligma,
                                           LigmaProgress  *progress,
                                           const gchar   *procedure_name,
                                           const gchar   *help_domain,
                                           const gchar   *help_locales,
                                           const gchar   *help_id);

static void       ligma_help_get_help_domains         (Ligma    *ligma,
                                                      gchar ***domain_names,
                                                      gchar ***domain_uris);
static gchar    * ligma_help_get_default_domain_uri   (Ligma    *ligma);
static gchar    * ligma_help_get_locales              (Ligma    *ligma);

static GFile    * ligma_help_get_user_manual_basedir  (void);

static void       ligma_help_query_alt_user_manual    (LigmaIdleHelp *idle_help);

static void       ligma_help_language_combo_changed   (GtkComboBox  *combo,
                                                      LigmaIdleHelp *idle_help);

/*  public functions  */

void
ligma_help_show (Ligma         *ligma,
                LigmaProgress *progress,
                const gchar  *help_domain,
                const gchar  *help_id)
{
  LigmaGuiConfig *config;

  g_return_if_fail (LIGMA_IS_LIGMA (ligma));
  g_return_if_fail (progress == NULL || LIGMA_IS_PROGRESS (progress));

  config = LIGMA_GUI_CONFIG (ligma->config);

  if (config->use_help)
    {
      LigmaIdleHelp *idle_help = g_slice_new0 (LigmaIdleHelp);

      idle_help->ligma     = ligma;
      idle_help->progress = progress;

      if (help_domain && strlen (help_domain))
        idle_help->help_domain = g_strdup (help_domain);

      idle_help->help_locales = ligma_help_get_locales (ligma);

      if (help_id && strlen (help_id))
        idle_help->help_id = g_strdup (help_id);

      LIGMA_LOG (HELP, "request for help-id '%s' from help-domain '%s'",
                help_id     ? help_id      : "(null)",
                help_domain ? help_domain  : "(null)");

      g_idle_add ((GSourceFunc) ligma_idle_help, idle_help);
    }
}

gboolean
ligma_help_browser_is_installed (Ligma *ligma)
{
  g_return_val_if_fail (LIGMA_IS_LIGMA (ligma), FALSE);

  if (ligma_pdb_lookup_procedure (ligma->pdb, "extension-ligma-help-browser"))
    return TRUE;

  return FALSE;
}

gboolean
ligma_help_user_manual_is_installed (Ligma *ligma)
{
  GFile    *basedir;
  gboolean  found = FALSE;

  g_return_val_if_fail (LIGMA_IS_LIGMA (ligma), FALSE);

  /*  if LIGMA2_HELP_URI is set, assume that the manual can be found there  */
  if (g_getenv ("LIGMA2_HELP_URI"))
    return TRUE;

  basedir = ligma_help_get_user_manual_basedir ();

  if (g_file_query_file_type (basedir, G_FILE_QUERY_INFO_NONE, NULL) ==
      G_FILE_TYPE_DIRECTORY)
    {
      gchar       *locales = ligma_help_get_locales (ligma);
      const gchar *s       = locales;
      const gchar *p;

      for (p = strchr (s, ':'); p && !found; p = strchr (s, ':'))
        {
          gchar *locale = g_strndup (s, p - s);
          GFile *file1  = g_file_get_child (basedir, locale);
          GFile *file2  = g_file_get_child (file1, "ligma-help.xml");

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
          GFile *file2  = g_file_get_child (file1, "ligma-help.xml");

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
ligma_help_user_manual_changed (Ligma *ligma)
{
  LigmaProcedure *procedure;

  g_return_if_fail (LIGMA_IS_LIGMA (ligma));

  /*  Check if a help parser is running  */
  procedure = ligma_pdb_lookup_procedure (ligma->pdb, "extension-ligma-help-temp");

  if (LIGMA_IS_TEMPORARY_PROCEDURE (procedure))
    {
      ligma_plug_in_close (LIGMA_TEMPORARY_PROCEDURE (procedure)->plug_in, TRUE);
    }
}

GList *
ligma_help_get_installed_languages (void)
{
  GList *manuals = NULL;
  GFile *basedir;

  /*  if LIGMA2_HELP_URI is set, assume that the manual can be found there  */
  if (g_getenv ("LIGMA2_HELP_URI"))
    basedir = g_file_new_for_uri (g_getenv ("LIGMA2_HELP_URI"));
  else
    basedir = ligma_help_get_user_manual_basedir ();

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
              if (g_file_info_get_file_type (info) == G_FILE_TYPE_DIRECTORY)
                {
                  GFile *locale_dir;
                  GFile *file;

                  locale_dir = g_file_enumerator_get_child (enumerator, info);
                  file  = g_file_get_child (locale_dir, "ligma-help.xml");
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
ligma_idle_help (LigmaIdleHelp *idle_help)
{
  LigmaGuiConfig *config         = LIGMA_GUI_CONFIG (idle_help->ligma->config);
  const gchar   *procedure_name = NULL;

  if (! idle_help->help_domain       &&
      ! config->user_manual_online   &&
      ! ligma_help_user_manual_is_installed (idle_help->ligma))
    {
      /*  The user manual is not installed locally, propose alternative
       *  manuals (other installed languages or online version).
       */
      ligma_help_query_alt_user_manual (idle_help);

      return FALSE;
    }

  if (config->help_browser == LIGMA_HELP_BROWSER_LIGMA)
    {
      if (ligma_help_browser (idle_help->ligma, idle_help->progress))
        procedure_name = "extension-ligma-help-browser-temp";
    }

  if (config->help_browser == LIGMA_HELP_BROWSER_WEB_BROWSER)
    {
      /*  FIXME: should check for procedure availability  */
      procedure_name = "plug-in-web-browser";
    }

  if (procedure_name)
    ligma_help_call (idle_help->ligma,
                    idle_help->progress,
                    procedure_name,
                    idle_help->help_domain,
                    idle_help->help_locales,
                    idle_help->help_id);

  ligma_idle_help_free (idle_help);

  return FALSE;
}

static void
ligma_idle_help_free (LigmaIdleHelp *idle_help)
{
  g_free (idle_help->help_domain);
  g_free (idle_help->help_locales);
  g_free (idle_help->help_id);

  g_slice_free (LigmaIdleHelp, idle_help);
}

static gboolean
ligma_help_browser (Ligma         *ligma,
                   LigmaProgress *progress)
{
  static gboolean  busy = FALSE;
  LigmaProcedure   *procedure;

  if (busy)
    return TRUE;

  busy = TRUE;

  /*  Check if a help browser is already running  */
  procedure = ligma_pdb_lookup_procedure (ligma->pdb,
                                         "extension-ligma-help-browser-temp");

  if (! procedure)
    {
      LigmaValueArray *args         = NULL;
      gchar         **help_domains = NULL;
      gchar         **help_uris    = NULL;
      GError         *error        = NULL;

      procedure = ligma_pdb_lookup_procedure (ligma->pdb,
                                             "extension-ligma-help-browser");

      if (! procedure)
        {
          ligma_help_browser_error (ligma, progress,
                                   _("Help browser is missing"),
                                   _("The LIGMA help browser is not available."),
                                   _("The LIGMA help browser plug-in appears "
                                     "to be missing from your installation. "
                                     "You may instead use the web browser "
                                     "for reading the help pages."));
          busy = FALSE;

          return FALSE;
        }

      ligma_help_get_help_domains (ligma, &help_domains, &help_uris);

      args = ligma_procedure_get_arguments (procedure);
      ligma_value_array_truncate (args, 3);

      g_value_set_enum (ligma_value_array_index (args, 0), LIGMA_RUN_INTERACTIVE);
      g_value_take_boxed (ligma_value_array_index (args, 1), help_domains);
      g_value_take_boxed (ligma_value_array_index (args, 2), help_uris);

      ligma_procedure_execute_async (procedure, ligma,
                                    ligma_get_user_context (ligma),
                                    NULL, args, NULL, &error);

      ligma_value_array_unref (args);

      if (error)
        {
          ligma_message_literal (ligma, G_OBJECT (progress), LIGMA_MESSAGE_ERROR,
                                error->message);
          g_error_free (error);
        }
     }

  /*  Check if the help browser started properly  */
  procedure = ligma_pdb_lookup_procedure (ligma->pdb,
                                         "extension-ligma-help-browser-temp");

  if (! procedure)
    {
      ligma_help_browser_error (ligma, progress,
                               _("Help browser doesn't start"),
                               _("Could not start the LIGMA help browser "
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
ligma_help_browser_error (Ligma         *ligma,
                         LigmaProgress *progress,
                         const gchar  *title,
                         const gchar  *primary,
                         const gchar  *text)
{
  GtkWidget *dialog;

  dialog = ligma_message_dialog_new (title, LIGMA_ICON_HELP_USER_MANUAL,
                                    NULL, 0,
                                    NULL, NULL,

                                    _("_Cancel"),          GTK_RESPONSE_CANCEL,
                                    _("Use _Web Browser"), GTK_RESPONSE_OK,

                                    NULL);

  ligma_dialog_set_alternative_button_order (GTK_DIALOG (dialog),
                                           GTK_RESPONSE_OK,
                                           GTK_RESPONSE_CANCEL,
                                           -1);

  if (progress)
    {
      guint32 window_id = ligma_progress_get_window_id (progress);

      if (window_id)
        ligma_window_set_transient_for (GTK_WINDOW (dialog), window_id);
    }

  ligma_message_box_set_primary_text (LIGMA_MESSAGE_DIALOG (dialog)->box,
                                     "%s", primary);
  ligma_message_box_set_text (LIGMA_MESSAGE_DIALOG (dialog)->box, "%s", text);

  if (ligma_dialog_run (LIGMA_DIALOG (dialog)) == GTK_RESPONSE_OK)
    {
      g_object_set (ligma->config,
                    "help-browser", LIGMA_HELP_BROWSER_WEB_BROWSER,
                    NULL);
    }

  gtk_widget_destroy (dialog);
}

static void
ligma_help_call (Ligma         *ligma,
                LigmaProgress *progress,
                const gchar  *procedure_name,
                const gchar  *help_domain,
                const gchar  *help_locales,
                const gchar  *help_id)
{
  LigmaProcedure *procedure;

  /*  Special case the help browser  */
  if (! strcmp (procedure_name, "extension-ligma-help-browser-temp"))
    {
      LigmaValueArray *return_vals;
      GError         *error = NULL;

      LIGMA_LOG (HELP, "Calling help via %s: %s %s %s",
                procedure_name,
                help_domain  ? help_domain  : "(null)",
                help_locales ? help_locales : "(null)",
                help_id      ? help_id      : "(null)");

      return_vals =
        ligma_pdb_execute_procedure_by_name (ligma->pdb,
                                            ligma_get_user_context (ligma),
                                            progress, &error,
                                            procedure_name,
                                            G_TYPE_STRING, help_domain,
                                            G_TYPE_STRING, help_locales,
                                            G_TYPE_STRING, help_id,
                                            G_TYPE_NONE);

      ligma_value_array_unref (return_vals);

      if (error)
        {
          ligma_message_literal (ligma, NULL, LIGMA_MESSAGE_ERROR, error->message);
          g_error_free (error);
        }

      return;
    }

  /*  Check if a help parser is already running  */
  procedure = ligma_pdb_lookup_procedure (ligma->pdb, "extension-ligma-help-temp");

  if (! procedure)
    {
      LigmaValueArray  *args         = NULL;
      gchar          **help_domains = NULL;
      gchar          **help_uris    = NULL;
      GError          *error        = NULL;

      procedure = ligma_pdb_lookup_procedure (ligma->pdb, "extension-ligma-help");

      if (! procedure)
        /*  FIXME: error msg  */
        return;

      ligma_help_get_help_domains (ligma, &help_domains, &help_uris);

      args = ligma_procedure_get_arguments (procedure);
      ligma_value_array_truncate (args, 2);

      g_value_take_boxed (ligma_value_array_index (args, 0), help_domains);
      g_value_take_boxed (ligma_value_array_index (args, 1), help_uris);

      ligma_procedure_execute_async (procedure, ligma,
                                    ligma_get_user_context (ligma), progress,
                                    args, NULL, &error);

      ligma_value_array_unref (args);

      if (error)
        {
          ligma_message_literal (ligma, NULL, LIGMA_MESSAGE_ERROR, error->message);
          g_error_free (error);
        }
    }

  /*  Check if the help parser started properly  */
  procedure = ligma_pdb_lookup_procedure (ligma->pdb, "extension-ligma-help-temp");

  if (procedure)
    {
      LigmaValueArray *return_vals;
      GError         *error = NULL;

      LIGMA_LOG (HELP, "Calling help via %s: %s %s %s",
                procedure_name,
                help_domain  ? help_domain  : "(null)",
                help_locales ? help_locales : "(null)",
                help_id      ? help_id      : "(null)");

      return_vals =
        ligma_pdb_execute_procedure_by_name (ligma->pdb,
                                            ligma_get_user_context (ligma),
                                            progress, &error,
                                            "extension-ligma-help-temp",
                                            G_TYPE_STRING, procedure_name,
                                            G_TYPE_STRING, help_domain,
                                            G_TYPE_STRING, help_locales,
                                            G_TYPE_STRING, help_id,
                                            G_TYPE_NONE);

      ligma_value_array_unref (return_vals);

      if (error)
        {
          ligma_message_literal (ligma, NULL, LIGMA_MESSAGE_ERROR, error->message);
          g_error_free (error);
        }
    }
}

static void
ligma_help_get_help_domains (Ligma    *ligma,
                            gchar ***domain_names,
                            gchar ***domain_uris)
{
  gchar **plug_in_domains = NULL;
  gchar **plug_in_uris    = NULL;
  gint    i, n_domains;

  n_domains = ligma_plug_in_manager_get_help_domains (ligma->plug_in_manager,
                                                     &plug_in_domains,
                                                     &plug_in_uris);

  *domain_names = g_new0 (gchar *, n_domains + 2);
  *domain_uris  = g_new0 (gchar *, n_domains + 2);

  (*domain_names)[0] = g_strdup ("https://www.ligma.org/help");
  (*domain_uris)[0]  = ligma_help_get_default_domain_uri (ligma);

  for (i = 0; i < n_domains; i++)
    {
      (*domain_names)[i + 1] = plug_in_domains[i];
      (*domain_uris)[i + 1]  = plug_in_uris[i];
    }

  g_free (plug_in_domains);
  g_free (plug_in_uris);
}

static gchar *
ligma_help_get_default_domain_uri (Ligma *ligma)
{
  LigmaGuiConfig *config = LIGMA_GUI_CONFIG (ligma->config);
  GFile         *dir;
  gchar         *uri;

  if (g_getenv ("LIGMA2_HELP_URI"))
    return g_strdup (g_getenv ("LIGMA2_HELP_URI"));

  if (config->user_manual_online)
    return g_strdup (config->user_manual_online_uri);

  dir = ligma_help_get_user_manual_basedir ();
  uri = g_file_get_uri (dir);
  g_object_unref (dir);

  return uri;
}

static gchar *
ligma_help_get_locales (Ligma *ligma)
{
  LigmaGuiConfig  *config = LIGMA_GUI_CONFIG (ligma->config);
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
ligma_help_get_user_manual_basedir (void)
{
  return ligma_data_directory_file ("help", NULL);
}

static void
ligma_help_query_online_response (GtkWidget    *dialog,
                                 gint          response,
                                 LigmaIdleHelp *idle_help)
{
  gtk_widget_destroy (dialog);

  if (response == GTK_RESPONSE_ACCEPT)
    {
      g_object_set (idle_help->ligma->config,
                    "user-manual-online", TRUE,
                    NULL);
    }
  if (response != GTK_RESPONSE_YES)
    {
      g_object_set (idle_help->ligma->config,
                    "help-locales", "",
                    NULL);
    }

  if (response == GTK_RESPONSE_ACCEPT ||
      response == GTK_RESPONSE_YES)
    {
      ligma_help_show (idle_help->ligma,
                      idle_help->progress,
                      idle_help->help_domain,
                      idle_help->help_id);
    }

  ligma_idle_help_free (idle_help);
}

static void
ligma_help_query_alt_user_manual (LigmaIdleHelp *idle_help)
{
  GtkWidget *dialog;
  GList     *manuals;

  dialog = ligma_message_dialog_new (_("LIGMA user manual is missing"),
                                    LIGMA_ICON_HELP_USER_MANUAL,
                                    NULL, 0, NULL, NULL,
                                    _("_Cancel"), GTK_RESPONSE_CANCEL,
                                    NULL);
  idle_help->query_dialog = GTK_DIALOG (dialog);

  if (idle_help->progress)
    {
      guint32 window_id = ligma_progress_get_window_id (idle_help->progress);

      if (window_id)
        ligma_window_set_transient_for (GTK_WINDOW (dialog), window_id);
    }

  ligma_message_box_set_primary_text (LIGMA_MESSAGE_DIALOG (dialog)->box,
                                     _("The LIGMA user manual is not installed "
                                       "in your language."));

  /* Add a list of available manuals instead, if any. */
  manuals = ligma_help_get_installed_languages ();
  if (manuals != NULL)
    {
      GtkWidget *lang_combo;

      /* Add an additional button. */
      gtk_dialog_add_button (GTK_DIALOG (dialog),
                             _("Read Selected _Language"),
                             GTK_RESPONSE_YES);
      /* And a dropdown list of available manuals. */
      lang_combo = ligma_language_combo_box_new (TRUE,
                                                _("Available manuals..."));
      gtk_combo_box_set_active (GTK_COMBO_BOX (lang_combo), 0);
      gtk_dialog_set_response_sensitive (idle_help->query_dialog,
                                         GTK_RESPONSE_YES, FALSE);
      g_signal_connect (lang_combo, "changed",
                        G_CALLBACK (ligma_help_language_combo_changed),
                        idle_help);
      gtk_box_pack_start (GTK_BOX (gtk_dialog_get_content_area (GTK_DIALOG (dialog))),
                          lang_combo, TRUE, TRUE, 0);
      gtk_widget_show (lang_combo);

      ligma_message_box_set_text (LIGMA_MESSAGE_DIALOG (dialog)->box,
                                 _("You may either select a manual in another "
                                   "language or read the online version."));
    }
  else
    {
      ligma_message_box_set_text (LIGMA_MESSAGE_DIALOG (dialog)->box,
                                 _("You may either install the additional help "
                                   "package or change your preferences to use "
                                   "the online version."));
    }
  gtk_dialog_add_button (GTK_DIALOG (dialog),
                         _("Read _Online"), GTK_RESPONSE_ACCEPT);
  gtk_dialog_set_default_response (GTK_DIALOG (dialog), GTK_RESPONSE_ACCEPT);
  if (manuals != NULL)
    {
      ligma_dialog_set_alternative_button_order (GTK_DIALOG (dialog),
                                               GTK_RESPONSE_ACCEPT,
                                               GTK_RESPONSE_YES,
                                               GTK_RESPONSE_CANCEL,
                                               -1);
      g_list_free_full (manuals, g_free);
    }
  else
    {
      ligma_dialog_set_alternative_button_order (GTK_DIALOG (dialog),
                                               GTK_RESPONSE_ACCEPT,
                                               GTK_RESPONSE_CANCEL,
                                               -1);
    }
  g_signal_connect (dialog, "response",
                    G_CALLBACK (ligma_help_query_online_response),
                    idle_help);
  gtk_widget_show (dialog);
}

static void
ligma_help_language_combo_changed (GtkComboBox  *combo,
                                  LigmaIdleHelp *idle_help)
{
  gchar *help_locales = NULL;
  gchar *code;

  code = ligma_language_combo_box_get_code (LIGMA_LANGUAGE_COMBO_BOX (combo));
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
  g_object_set (idle_help->ligma->config,
                "help-locales", help_locales? help_locales : "",
                NULL);

  g_free (code);
  if (help_locales)
    g_free (help_locales);
}
