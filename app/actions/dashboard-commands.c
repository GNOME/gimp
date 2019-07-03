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


/*  local function prototypes  */

static void   dashboard_log_record_response     (GtkWidget     *dialog,
                                                 int            response_id,
                                                 GimpDashboard *dashboard);

static void   dashboard_log_add_marker_response (GtkWidget     *dialog,
                                                 const gchar   *description,
                                                 GimpDashboard *dashboard);


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
      GtkWidget *dialog;

      #define LOG_RECORD_KEY "gimp-dashboard-log-record-dialog"

      dialog = dialogs_get_dialog (G_OBJECT (dashboard), LOG_RECORD_KEY);

      if (! dialog)
        {
          GtkFileFilter *filter;
          GFile         *folder;

          dialog = gtk_file_chooser_dialog_new (
            "Record Performance Log", NULL, GTK_FILE_CHOOSER_ACTION_SAVE,

            _("_Cancel"), GTK_RESPONSE_CANCEL,
            _("_Record"), GTK_RESPONSE_OK,

            NULL);

          gtk_dialog_set_default_response (GTK_DIALOG (dialog),
                                           GTK_RESPONSE_OK);
          gtk_dialog_set_alternative_button_order (GTK_DIALOG (dialog),
                                                   GTK_RESPONSE_OK,
                                                   GTK_RESPONSE_CANCEL,
                                                   -1);

          gtk_window_set_screen (
            GTK_WINDOW (dialog),
            gtk_widget_get_screen (GTK_WIDGET (dashboard)));
          gtk_window_set_role (GTK_WINDOW (dialog),
                               "gimp-dashboard-log-record");
          gtk_window_set_position (GTK_WINDOW (dialog), GTK_WIN_POS_MOUSE);

          gtk_file_chooser_set_do_overwrite_confirmation (
            GTK_FILE_CHOOSER (dialog), TRUE);

          filter = gtk_file_filter_new ();
          gtk_file_filter_set_name (filter, _("All Files"));
          gtk_file_filter_add_pattern (filter, "*");
          gtk_file_chooser_add_filter (GTK_FILE_CHOOSER (dialog), filter);

          filter = gtk_file_filter_new ();
          gtk_file_filter_set_name (filter, _("Log Files (*.log)"));
          gtk_file_filter_add_pattern (filter, "*.log");
          gtk_file_chooser_add_filter (GTK_FILE_CHOOSER (dialog), filter);

          gtk_file_chooser_set_filter (GTK_FILE_CHOOSER (dialog), filter);

          folder = g_object_get_data (G_OBJECT (dashboard),
                                      "gimp-dashboard-log-record-folder");

          if (folder)
            {
              gtk_file_chooser_set_current_folder_file (
                GTK_FILE_CHOOSER (dialog), folder, NULL);
            }

          gtk_file_chooser_set_current_name (GTK_FILE_CHOOSER (dialog),
                                             "gimp-performance.log");

          g_signal_connect (dialog, "response",
                            G_CALLBACK (dashboard_log_record_response),
                            dashboard);
          g_signal_connect (dialog, "delete-event",
                            G_CALLBACK (gtk_true),
                            NULL);

          gimp_help_connect (dialog, gimp_standard_help_func,
                             GIMP_HELP_DASHBOARD_LOG_RECORD, NULL);

          dialogs_attach_dialog (G_OBJECT (dashboard), LOG_RECORD_KEY, dialog);

          g_signal_connect_object (dashboard, "destroy",
                                   G_CALLBACK (gtk_widget_destroy),
                                   dialog,
                                   G_CONNECT_SWAPPED);

          #undef LOG_RECORD_KEY
        }

      gtk_window_present (GTK_WINDOW (dialog));
    }
  else
    {
      GError *error = NULL;

      if (! gimp_dashboard_log_stop_recording (dashboard, &error))
        {
          gimp_message_literal (
            gimp_editor_get_ui_manager (GIMP_EDITOR (dashboard))->gimp,
            NULL, GIMP_MESSAGE_ERROR, error->message);
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
        dashboard);

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
dashboard_log_record_response (GtkWidget     *dialog,
                               int            response_id,
                               GimpDashboard *dashboard)
{
  if (response_id == GTK_RESPONSE_OK)
    {
      GFile  *file  = gtk_file_chooser_get_file (GTK_FILE_CHOOSER (dialog));
      GError *error = NULL;

      g_object_set_data_full (G_OBJECT (dashboard),
                              "gimp-dashboard-log-record-folder",
                              g_file_get_parent (file),
                              g_object_unref);

      if (! gimp_dashboard_log_start_recording (dashboard, file, &error))
        {
          gimp_message_literal (
            gimp_editor_get_ui_manager (GIMP_EDITOR (dashboard))->gimp,
            NULL, GIMP_MESSAGE_ERROR, error->message);

          g_clear_error (&error);
        }

      g_object_unref (file);
    }

  gtk_widget_destroy (dialog);
}

static void
dashboard_log_add_marker_response (GtkWidget     *dialog,
                                   const gchar   *description,
                                   GimpDashboard *dashboard)
{
  gimp_dashboard_log_add_marker (dashboard, description);
}
