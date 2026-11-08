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

#include "config.h"

#include <gegl.h>
#include <gtk/gtk.h>

#include "libgimpwidgets/gimpwidgets.h"

#include "actions-types.h"

#include "core/gimp.h"

#include "widgets/gimpdashboard.h"
#include "widgets/gimphelp-ids.h"
#include "widgets/gimpuimanager.h"

#include "dialogs/dialogs.h"

#include "dashboard-commands.h"

#include "gimp-intl.h"


typedef struct
{
  GFile                  *folder;
  GimpDashboardLogParams  params;
} DashboardLogDialogInfo;


/*  local function prototypes  */

static void                     dashboard_log_record_response      (GtkNativeDialog        *dialog,
                                                                    int                     response_id,
                                                                    GimpDashboard          *dashboard);

static void                     dashboard_log_add_marker_response  (GtkWidget              *dialog,
                                                                    const gchar            *description,
                                                                    GimpDashboard          *dashboard);


/*  public functions */


void
dashboard_update_interval_cmd_callback (GimpAction *action,
                                        GVariant   *value,
                                        gpointer    data)
{
  GimpDashboard              *dashboard = GIMP_DASHBOARD (data);
  GimpDashboardUpdateInteval  update_interval;

  update_interval = g_variant_get_int32 (value);

  gimp_dashboard_set_update_interval (dashboard, update_interval);
}

void
dashboard_history_duration_cmd_callback (GimpAction *action,
                                         GVariant   *value,
                                         gpointer    data)
{
  GimpDashboard                *dashboard = GIMP_DASHBOARD (data);
  GimpDashboardHistoryDuration  history_duration;

  history_duration = g_variant_get_int32 (value);

  gimp_dashboard_set_history_duration (dashboard, history_duration);
}

void
dashboard_log_record_cmd_callback (GimpAction *action,
                                   GVariant   *value,
                                   gpointer    data)
{
  GimpDashboard *dashboard = GIMP_DASHBOARD (data);

  if (! gimp_dashboard_log_is_recording (dashboard))
    {
      GtkFileChooserNative *dialog;

      #define LOG_RECORD_KEY "gimp-dashboard-log-record-dialog"

      dialog =
        GTK_FILE_CHOOSER_NATIVE (dialogs_get_native_dialog (G_OBJECT (dashboard),
                                 LOG_RECORD_KEY));

      if (! dialog)
        {
          GtkWidget     *window;
          GtkFileFilter *filter;
          GDateTime     *datetime;
          gchar         *timestamp;
          gchar         *default_filename;

          window = gtk_widget_get_toplevel (GTK_WIDGET (dashboard));

          dialog = gtk_file_chooser_native_new (_("Record Performance Log"),
                                                GTK_WINDOW (window),
                                                GTK_FILE_CHOOSER_ACTION_SAVE,
                                                _("_Record"), _("_Cancel"));

          gtk_file_chooser_set_do_overwrite_confirmation (GTK_FILE_CHOOSER (dialog),
                                                          TRUE);

          filter = gtk_file_filter_new ();
          gtk_file_filter_set_name (filter, _("All Files"));
          gtk_file_filter_add_pattern (filter, "*");
          gtk_file_chooser_add_filter (GTK_FILE_CHOOSER (dialog), filter);

          filter = gtk_file_filter_new ();
          gtk_file_filter_set_name (filter, _("Log Files (*.log)"));
          gtk_file_filter_add_pattern (filter, "*.log");
          gtk_file_chooser_add_filter (GTK_FILE_CHOOSER (dialog), filter);

          gtk_file_chooser_set_filter (GTK_FILE_CHOOSER (dialog), filter);

          datetime = g_date_time_new_now_local ();

          /* Since Windows doesn't allow colons in filenames,
           * we use ISO 8601 basic format for the timestamp */
          timestamp        = g_date_time_format (datetime, "%Y%m%dT%H%M%SZ");
          default_filename = g_strdup_printf ("gimp-performance-%s.log",
                                              timestamp);
          g_date_time_unref (datetime);
          g_free (timestamp);

          gtk_file_chooser_set_current_name (GTK_FILE_CHOOSER (dialog),
                                             default_filename);
          g_free (default_filename);

          dialogs_attach_native_dialog (G_OBJECT (dashboard), LOG_RECORD_KEY,
                                        GTK_NATIVE_DIALOG (dialog));

          g_signal_connect (dialog, "response",
                            G_CALLBACK (dashboard_log_record_response),
                            dashboard);

          #undef LOG_RECORD_KEY
        }

      gtk_native_dialog_show (GTK_NATIVE_DIALOG (dialog));
    }
  else
    {
      GError *error = NULL;

      if (! gimp_dashboard_log_stop_recording (dashboard, &error))
        {
          Gimp *gimp;

          gimp = gimp_editor_get_ui_manager (GIMP_EDITOR (dashboard))->gimp;


          gimp_message_literal (gimp, NULL, GIMP_MESSAGE_ERROR,
                                error->message);
        }
    }
}

void
dashboard_log_add_marker_cmd_callback (GimpAction *action,
                                       GVariant   *value,
                                       gpointer    data)
{
  GimpDashboard *dashboard = GIMP_DASHBOARD (data);
  GtkWidget     *dialog;

  #define LOG_ADD_MARKER_KEY "gimp-dashboard-log-add-marker-dialog"

  dialog = dialogs_get_dialog (G_OBJECT (dashboard), LOG_ADD_MARKER_KEY);

  if (! dialog)
    {
      dialog = gimp_query_string_box (
        _("Add Marker"), GTK_WIDGET (dashboard),
        gimp_standard_help_func, GIMP_HELP_DASHBOARD_LOG_ADD_MARKER,
        _("Enter a description for the marker"),
        NULL,
        G_OBJECT (dashboard), "destroy",
        (GimpQueryStringCallback) dashboard_log_add_marker_response,
        dashboard, NULL);

      dialogs_attach_dialog (G_OBJECT (dashboard), LOG_ADD_MARKER_KEY, dialog);

      #undef LOG_ADD_MARKER_KEY
    }

  gtk_window_present (GTK_WINDOW (dialog));
}

void
dashboard_log_add_empty_marker_cmd_callback (GimpAction *action,
                                             GVariant   *value,
                                             gpointer    data)
{
  GimpDashboard *dashboard = GIMP_DASHBOARD (data);

  gimp_dashboard_log_add_marker (dashboard, NULL);
}

void
dashboard_reset_cmd_callback (GimpAction *action,
                              GVariant   *value,
                              gpointer    data)
{
  GimpDashboard *dashboard = GIMP_DASHBOARD (data);

  gimp_dashboard_reset (dashboard);
}

void
dashboard_low_swap_space_warning_cmd_callback (GimpAction *action,
                                               GVariant   *value,
                                               gpointer    data)
{
  GimpDashboard *dashboard              = GIMP_DASHBOARD (data);
  gboolean       low_swap_space_warning = g_variant_get_boolean (value);

  gimp_dashboard_set_low_swap_space_warning (dashboard, low_swap_space_warning);
}


/*  private functions  */

static void
dashboard_log_record_response (GtkNativeDialog *dialog,
                               int              response_id,
                               GimpDashboard   *dashboard)
{
  if (response_id == GTK_RESPONSE_ACCEPT)
    {
      GFile  *file;
      GError *error = NULL;

      file = gtk_file_chooser_get_file (GTK_FILE_CHOOSER (dialog));
      if (! gimp_dashboard_log_start_recording (dashboard, file, NULL, &error))
        {
          Gimp *gimp;

          gimp = gimp_editor_get_ui_manager (GIMP_EDITOR (dashboard))->gimp;
          gimp_message_literal (gimp, NULL, GIMP_MESSAGE_ERROR,
                                error->message);

          g_clear_error (&error);
        }
      g_object_unref (file);
    }

  dialogs_detach_native_dialog (G_OBJECT (dashboard), dialog);
  gtk_native_dialog_destroy (dialog);
}

static void
dashboard_log_add_marker_response (GtkWidget     *dialog,
                                   const gchar   *description,
                                   GimpDashboard *dashboard)
{
  gimp_dashboard_log_add_marker (dashboard, description);
}
