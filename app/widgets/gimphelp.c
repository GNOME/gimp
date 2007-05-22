/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimphelp.c
 * Copyright (C) 1999-2004 Michael Natterer <mitch@gimp.org>
 *                         Henrik Brix Andersen <brix@gimp.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include "config.h"

#include <string.h>

#include <gtk/gtk.h>

#include "libgimpbase/gimpbase.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "widgets-types.h"

#include "config/gimpguiconfig.h"

#include "core/gimp.h"
#include "core/gimp-utils.h"
#include "core/gimpparamspecs.h"

#include "pdb/gimppdb.h"
#include "pdb/gimpprocedure.h"

#include "plug-in/gimppluginmanager-help-domain.h"

#include "gimphelp.h"
#include "gimphelp-ids.h"
#include "gimpmessagebox.h"
#include "gimpmessagedialog.h"

#include "gimp-intl.h"


/*  #define GIMP_HELP_DEBUG  */


typedef struct _GimpIdleHelp GimpIdleHelp;

struct _GimpIdleHelp
{
  Gimp  *gimp;
  gchar *help_domain;
  gchar *help_locales;
  gchar *help_id;
};


/*  local function prototypes  */

static gint      gimp_idle_help          (GimpIdleHelp  *idle_help);

static gboolean  gimp_help_browser       (Gimp          *gimp);
static void      gimp_help_browser_error (Gimp          *gimp,
                                          const gchar   *title,
                                          const gchar   *primary,
                                          const gchar   *text);

static void      gimp_help_call          (Gimp          *gimp,
                                          const gchar   *procedure_name,
                                          const gchar   *help_domain,
                                          const gchar   *help_locales,
                                          const gchar   *help_id);
static gchar *   gimp_help_get_locales   (GimpGuiConfig *config);


/*  public functions  */

void
gimp_help_show (Gimp        *gimp,
                const gchar *help_domain,
                const gchar *help_id)
{
  GimpGuiConfig *config;

  g_return_if_fail (GIMP_IS_GIMP (gimp));

  config = GIMP_GUI_CONFIG (gimp->config);

  if (config->use_help)
    {
      GimpIdleHelp *idle_help = g_slice_new0 (GimpIdleHelp);

      idle_help->gimp = gimp;

      if (help_domain && strlen (help_domain))
        idle_help->help_domain = g_strdup (help_domain);

      idle_help->help_locales = gimp_help_get_locales (config);

      if (help_id && strlen (help_id))
        idle_help->help_id = g_strdup (help_id);

      g_idle_add ((GSourceFunc) gimp_idle_help, idle_help);

      if (gimp->be_verbose)
        g_print ("HELP: request for help-id '%s' from help-domain '%s'\n",
                 help_id     ? help_id      : "(null)",
                 help_domain ? help_domain  : "(null)");
    }
}


/*  private functions  */

static gboolean
gimp_idle_help (GimpIdleHelp *idle_help)
{
  GimpGuiConfig *config         = GIMP_GUI_CONFIG (idle_help->gimp->config);
  const gchar   *procedure_name = NULL;

#ifdef GIMP_HELP_DEBUG
  g_printerr ("Help Domain: %s\n",
              idle_help->help_domain ? idle_help->help_domain : "NULL");
  g_printerr ("Help ID: %s\n\n",
              idle_help->help_id     ? idle_help->help_id     : "NULL");
#endif

  if (config->help_browser == GIMP_HELP_BROWSER_GIMP)
    {
      if (gimp_help_browser (idle_help->gimp))
        procedure_name = "extension-gimp-help-browser-temp";
    }

  if (config->help_browser == GIMP_HELP_BROWSER_WEB_BROWSER)
    {
      /*  FIXME: should check for procedure availability  */
      procedure_name = "plug-in-web-browser";
    }

  if (procedure_name)
    gimp_help_call (idle_help->gimp,
                    procedure_name,
                    idle_help->help_domain,
                    idle_help->help_locales,
                    idle_help->help_id);

  g_free (idle_help->help_domain);
  g_free (idle_help->help_locales);
  g_free (idle_help->help_id);

  g_slice_free (GimpIdleHelp, idle_help);

  return FALSE;
}

static gboolean
gimp_help_browser (Gimp *gimp)
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

      procedure = gimp_pdb_lookup_procedure (gimp->pdb,
                                             "extension-gimp-help-browser");

      if (! procedure)
        {
          gimp_help_browser_error (gimp,
                                   _("Help browser not found"),
                                   _("Could not find GIMP help browser."),
                                   _("The GIMP help browser plug-in appears "
                                     "to be missing from your installation."));
          busy = FALSE;

          return FALSE;
        }

      n_domains = gimp_plug_in_manager_get_help_domains (gimp->plug_in_manager,
                                                         &help_domains,
                                                         &help_uris);

      args = gimp_procedure_get_arguments (procedure);
      gimp_value_array_truncate (args, 5);

      g_value_set_int             (&args->values[0], GIMP_RUN_INTERACTIVE);
      g_value_set_int             (&args->values[1], n_domains);
      gimp_value_take_stringarray (&args->values[2], help_domains, n_domains);
      g_value_set_int             (&args->values[3], n_domains);
      gimp_value_take_stringarray (&args->values[4], help_uris, n_domains);

      gimp_procedure_execute_async (procedure, gimp,
                                    gimp_get_user_context (gimp),
                                    NULL, args, NULL);

      g_value_array_free (args);
    }

  /*  Check if the help browser started properly  */
  procedure = gimp_pdb_lookup_procedure (gimp->pdb,
                                         "extension-gimp-help-browser-temp");

  if (! procedure)
    {
      gimp_help_browser_error (gimp,
                               _("Help browser doesn't start"),
                               _("Could not start the GIMP help browser plug-in."),
                               NULL);
      busy = FALSE;

      return FALSE;
    }

  busy = FALSE;

  return TRUE;
}

static void
gimp_help_browser_error (Gimp        *gimp,
                         const gchar *title,
                         const gchar *primary,
                         const gchar *text)
{
  GtkWidget *dialog;

  dialog =
    gimp_message_dialog_new (title, GIMP_STOCK_WARNING,
                             NULL, 0,
                             NULL, NULL,

                             GTK_STOCK_CANCEL,              GTK_RESPONSE_CANCEL,
                             _("Use _web browser instead"), GTK_RESPONSE_OK,

                             NULL);

  gimp_message_box_set_primary_text (GIMP_MESSAGE_DIALOG (dialog)->box, primary);
  gimp_message_box_set_text (GIMP_MESSAGE_DIALOG (dialog)->box, text);

  if (gimp_dialog_run (GIMP_DIALOG (dialog)) == GTK_RESPONSE_OK)
    g_object_set (gimp->config,
                  "help-browser", GIMP_HELP_BROWSER_WEB_BROWSER,
                  NULL);

  gtk_widget_destroy (dialog);
}

static void
gimp_help_call (Gimp        *gimp,
                const gchar *procedure_name,
                const gchar *help_domain,
                const gchar *help_locales,
                const gchar *help_id)
{
  GimpProcedure *procedure;

  /*  Special case the help browser  */
  if (! strcmp (procedure_name, "extension-gimp-help-browser-temp"))
    {
      GValueArray *return_vals;

      return_vals = gimp_pdb_execute_procedure_by_name (gimp->pdb,
                                                        gimp_get_user_context (gimp),
                                                        NULL,
                                                        procedure_name,
                                                        G_TYPE_STRING, help_domain,
                                                        G_TYPE_STRING, help_locales,
                                                        G_TYPE_STRING, help_id,
                                                        G_TYPE_NONE);

      g_value_array_free (return_vals);

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

      procedure = gimp_pdb_lookup_procedure (gimp->pdb, "extension-gimp-help");

      if (! procedure)
        /*  FIXME: error msg  */
        return;

      n_domains = gimp_plug_in_manager_get_help_domains (gimp->plug_in_manager,
                                                         &help_domains,
                                                         &help_uris);

      args = gimp_procedure_get_arguments (procedure);
      gimp_value_array_truncate (args, 4);

      g_value_set_int             (&args->values[0], n_domains);
      gimp_value_take_stringarray (&args->values[1], help_domains, n_domains);
      g_value_set_int             (&args->values[2], n_domains);
      gimp_value_take_stringarray (&args->values[3], help_uris, n_domains);

      gimp_procedure_execute_async (procedure, gimp,
                                    gimp_get_user_context (gimp),
                                    NULL, args, NULL);

      g_value_array_free (args);
    }

  /*  Check if the help parser started properly  */
  procedure = gimp_pdb_lookup_procedure (gimp->pdb, "extension-gimp-help-temp");

  if (procedure)
    {
      GValueArray *return_vals;

#ifdef GIMP_HELP_DEBUG
      g_printerr ("Calling help via %s: %s %s %s\n",
                  procedure_name,
                  help_domain  ? help_domain  : "(null)",
                  help_locales ? help_locales : "(null)",
                  help_id      ? help_id      : "(null)");
#endif

      return_vals = gimp_pdb_execute_procedure_by_name (gimp->pdb,
                                                        gimp_get_user_context (gimp),
                                                        NULL,
                                                        "extension-gimp-help-temp",
                                                        G_TYPE_STRING, procedure_name,
                                                        G_TYPE_STRING, help_domain,
                                                        G_TYPE_STRING, help_locales,
                                                        G_TYPE_STRING, help_id,
                                                        G_TYPE_NONE);

      g_value_array_free (return_vals);
    }
}

static gchar *
gimp_help_get_locales (GimpGuiConfig *config)
{
  if (config->help_locales && strlen (config->help_locales))
    return g_strdup (config->help_locales);

  return g_strjoinv (":", (gchar **) g_get_language_names ());
}

