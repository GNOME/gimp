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

static void                     dashboard_log_record_response      (GtkWidget              *dialog,
                                                                    int                     response_id,
                                                                    GimpDashboard          *dashboard);

static void                     dashboard_log_add_marker_response  (GtkWidget              *dialog,
                                                                    const gchar            *description,
                                                                    GimpDashboard          *dashboard);

static DashboardLogDialogInfo * dashboard_log_dialog_info_new      (GimpDashboard          *dashboard);
static void                     dashboard_log_dialog_info_free     (DashboardLogDialogInfo *info);


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
          GtkFileFilter          *filter;
          DashboardLogDialogInfo *info;
          GtkWidget              *hbox;
          GtkWidget              *hbox2;
          GtkWidget              *label;
          GtkWidget              *spinbutton;
          GtkWidget              *toggle;

          dialog = gtk_file_chooser_dialog_new (
            "Record Performance Log", NULL, GTK_FILE_CHOOSER_ACTION_SAVE,

            _("_Cancel"), GTK_RESPONSE_CANCEL,
            _("_Record"), GTK_RESPONSE_OK,

            NULL);

          gtk_dialog_set_default_response (GTK_DIALOG (dialog),
                                           GTK_RESPONSE_OK);
          gimp_dialog_set_alternative_button_order (GTK_DIALOG (dialog),
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

          info = g_object_get_data (G_OBJECT (dashboard),
                                    "gimp-dashboard-log-dialog-info");

          if (! info)
            {
              info = dashboard_log_dialog_info_new (dashboard);

              g_object_set_data_full (
                G_OBJECT (dashboard),
                "gimp-dashboard-log-dialog-info", info,
                (GDestroyNotify) dashboard_log_dialog_info_free);
            }

          if (info->folder)
            {
              gtk_file_chooser_set_current_folder_file (
                GTK_FILE_CHOOSER (dialog), info->folder, NULL);
            }

          gtk_file_chooser_set_current_name (GTK_FILE_CHOOSER (dialog),
                                             "gimp-performance.log");

          hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 8);
          gtk_file_chooser_set_extra_widget (GTK_FILE_CHOOSER (dialog), hbox);
          gtk_widget_show (hbox);

          hbox2 = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 6);
          gimp_help_set_help_data (hbox2, _("Log samples per second"), NULL);
          gtk_box_pack_start (GTK_BOX (hbox), hbox2, FALSE, FALSE, 0);
          gtk_widget_show (hbox2);

          label = gtk_label_new_with_mnemonic (_("Sample fre_quency:"));
          gtk_box_pack_start (GTK_BOX (hbox2), label, FALSE, FALSE, 0);
          gtk_widget_show (label);

          spinbutton = gimp_spin_button_new_with_range (1, 1000, 1);
          gtk_box_pack_start (GTK_BOX (hbox2), spinbutton, FALSE, FALSE, 0);
          gtk_widget_show (spinbutton);

          gtk_spin_button_set_value (GTK_SPIN_BUTTON (spinbutton),
                                     info->params.sample_frequency);

          g_signal_connect (gtk_spin_button_get_adjustment (
                              GTK_SPIN_BUTTON (spinbutton)),
                            "value-changed",
                            G_CALLBACK (gimp_int_adjustment_update),
                            &info->params.sample_frequency);

          gtk_label_set_mnemonic_widget (GTK_LABEL (label), spinbutton);

          toggle = gtk_check_button_new_with_mnemonic (_("_Backtrace"));
          gimp_help_set_help_data (toggle, _("Include backtraces in log"),
                                   NULL);
          gtk_box_pack_start (GTK_BOX (hbox), toggle, FALSE, FALSE, 0);
          gtk_widget_show (toggle);

          gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (toggle),
                                        info->params.backtrace);

          g_signal_connect (toggle, "toggled",
                            G_CALLBACK (gimp_toggle_button_update),
                            &info->params.backtrace);

          toggle = gtk_check_button_new_with_mnemonic (_("_Messages"));
          gimp_help_set_help_data (toggle,
                                   _("Include diagnostic messages in log"),
                                   NULL);
          gtk_box_pack_start (GTK_BOX (hbox), toggle, FALSE, FALSE, 0);
          gtk_widget_show (toggle);

          gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (toggle),
                                        info->params.messages);

          g_signal_connect (toggle, "toggled",
                            G_CALLBACK (gimp_toggle_button_update),
                            &info->params.messages);

          toggle = gtk_check_button_new_with_mnemonic (_("Progressi_ve"));
          gimp_help_set_help_data (toggle,
                                   _("Produce complete log "
                                     "even if not properly terminated"),
                                   NULL);
          gtk_box_pack_start (GTK_BOX (hbox), toggle, FALSE, FALSE, 0);
          gtk_widget_show (toggle);

          gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (toggle),
                                        info->params.progressive);

          g_signal_connect (toggle, "toggled",
                            G_CALLBACK (gimp_toggle_button_update),
                            &info->params.progressive);

          g_signal_connect (dialog, "response",
                            G_CALLBACK (dashboard_log_record_response),
                            dashboard);
          g_signal_connect (dialog, "delete-event",
                            G_CALLBACK (gtk_true),
                            NULL);

          gimp_help_connect (dialog, NULL, gimp_standard_help_func,
                             GIMP_HELP_DASHBOARD_LOG_RECORD, NULL, NULL);

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
dashboard_log_record_response (GtkWidget     *dialog,
                               int            response_id,
                               GimpDashboard *dashboard)
{
  if (response_id == GTK_RESPONSE_OK)
    {
      GFile                  *file;
      DashboardLogDialogInfo *info;
      GError                 *error = NULL;

      file = gtk_file_chooser_get_file (GTK_FILE_CHOOSER (dialog));

      info = g_object_get_data (G_OBJECT (dashboard),
                                "gimp-dashboard-log-dialog-info");

      g_return_if_fail (info != NULL);

      g_set_object (&info->folder, g_file_get_parent (file));

      if (! gimp_dashboard_log_start_recording (dashboard,
                                                file, &info->params,
                                                &error))
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

static DashboardLogDialogInfo *
dashboard_log_dialog_info_new (GimpDashboard *dashboard)
{
  DashboardLogDialogInfo *info = g_slice_new (DashboardLogDialogInfo);

  info->folder = NULL;
  info->params = *gimp_dashboard_log_get_default_params (dashboard);

  return info;
}

static void
dashboard_log_dialog_info_free (DashboardLogDialogInfo *info)
{
  g_clear_object (&info->folder);

  g_slice_free (DashboardLogDialogInfo, info);
}
