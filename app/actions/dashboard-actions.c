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

#include "widgets/gimpactiongroup.h"
#include "widgets/gimpdashboard.h"
#include "widgets/gimphelp-ids.h"

#include "dashboard-actions.h"
#include "dashboard-commands.h"

#include "gimp-intl.h"


static const GimpActionEntry dashboard_actions[] =
{
  { "dashboard-groups", NULL,
    NC_("dashboard-action", "_Groups") },
  { "dashboard-update-interval", NULL,
    NC_("dashboard-action", "_Update Interval") },
  { "dashboard-history-duration", NULL,
    NC_("dashboard-action", "_History Duration") },

  { "dashboard-log-record", GIMP_ICON_RECORD,
    NC_("dashboard-action", "_Start/Stop Recording..."), NULL, { NULL },
    NC_("dashboard-action", "Start/stop recording performance log"),
    dashboard_log_record_cmd_callback,
    GIMP_HELP_DASHBOARD_LOG_RECORD },
  { "dashboard-log-add-marker", GIMP_ICON_MARKER,
    NC_("dashboard-action", "_Add Marker..."), NULL, { NULL },
    NC_("dashboard-action", "Add an event marker "
                            "to the performance log"),
    dashboard_log_add_marker_cmd_callback,
    GIMP_HELP_DASHBOARD_LOG_ADD_MARKER },
  { "dashboard-log-add-empty-marker", GIMP_ICON_MARKER,
    NC_("dashboard-action", "Add _Empty Marker"), NULL, { NULL },
    NC_("dashboard-action", "Add an empty event marker "
                            "to the performance log"),
    dashboard_log_add_empty_marker_cmd_callback,
    GIMP_HELP_DASHBOARD_LOG_ADD_EMPTY_MARKER },

  { "dashboard-reset", GIMP_ICON_RESET,
    NC_("dashboard-action", "_Reset"), NULL, { NULL },
    NC_("dashboard-action", "Reset cumulative data"),
    dashboard_reset_cmd_callback,
    GIMP_HELP_DASHBOARD_RESET },
};

static const GimpToggleActionEntry dashboard_toggle_actions[] =
{
  { "dashboard-low-swap-space-warning", NULL,
    NC_("dashboard-action", "_Low Swap Space Warning"), NULL, { NULL },
    NC_("dashboard-action", "Raise the dashboard when "
                            "the swap size approaches its limit"),
    dashboard_low_swap_space_warning_cmd_callback,
    FALSE,
    GIMP_HELP_DASHBOARD_LOW_SWAP_SPACE_WARNING }
};

static const GimpRadioActionEntry dashboard_update_interval_actions[] =
{
  { "dashboard-update-interval-0-25-sec", NULL,
    NC_("dashboard-update-interval", "0.25 Seconds"), NULL, { NULL }, NULL,
    GIMP_DASHBOARD_UPDATE_INTERVAL_0_25_SEC,
    GIMP_HELP_DASHBOARD_UPDATE_INTERVAL },

  { "dashboard-update-interval-0-5-sec", NULL,
    NC_("dashboard-update-interval", "0.5 Seconds"), NULL, { NULL }, NULL,
    GIMP_DASHBOARD_UPDATE_INTERVAL_0_5_SEC,
    GIMP_HELP_DASHBOARD_UPDATE_INTERVAL },

  { "dashboard-update-interval-1-sec", NULL,
    NC_("dashboard-update-interval", "1 Second"), NULL, { NULL }, NULL,
    GIMP_DASHBOARD_UPDATE_INTERVAL_1_SEC,
    GIMP_HELP_DASHBOARD_UPDATE_INTERVAL },

  { "dashboard-update-interval-2-sec", NULL,
    NC_("dashboard-update-interval", "2 Seconds"), NULL, { NULL }, NULL,
    GIMP_DASHBOARD_UPDATE_INTERVAL_2_SEC,
    GIMP_HELP_DASHBOARD_UPDATE_INTERVAL },

  { "dashboard-update-interval-4-sec", NULL,
    NC_("dashboard-update-interval", "4 Seconds"), NULL, { NULL }, NULL,
    GIMP_DASHBOARD_UPDATE_INTERVAL_4_SEC,
    GIMP_HELP_DASHBOARD_UPDATE_INTERVAL }
};

static const GimpRadioActionEntry dashboard_history_duration_actions[] =
{
  { "dashboard-history-duration-15-sec", NULL,
    NC_("dashboard-history-duration", "15 Seconds"), NULL, { NULL }, NULL,
    GIMP_DASHBOARD_HISTORY_DURATION_15_SEC,
    GIMP_HELP_DASHBOARD_HISTORY_DURATION },

  { "dashboard-history-duration-30-sec", NULL,
    NC_("dashboard-history-duration", "30 Seconds"), NULL, { NULL }, NULL,
    GIMP_DASHBOARD_HISTORY_DURATION_30_SEC,
    GIMP_HELP_DASHBOARD_HISTORY_DURATION },

  { "dashboard-history-duration-60-sec", NULL,
    NC_("dashboard-history-duration", "60 Seconds"), NULL, { NULL }, NULL,
    GIMP_DASHBOARD_HISTORY_DURATION_60_SEC,
    GIMP_HELP_DASHBOARD_HISTORY_DURATION },

  { "dashboard-history-duration-120-sec", NULL,
    NC_("dashboard-history-duration", "120 Seconds"), NULL, { NULL }, NULL,
    GIMP_DASHBOARD_HISTORY_DURATION_120_SEC,
    GIMP_HELP_DASHBOARD_HISTORY_DURATION },

  { "dashboard-history-duration-240-sec", NULL,
    NC_("dashboard-history-duration", "240 Seconds"), NULL, { NULL }, NULL,
    GIMP_DASHBOARD_HISTORY_DURATION_240_SEC,
    GIMP_HELP_DASHBOARD_HISTORY_DURATION }
};


void
dashboard_actions_setup (GimpActionGroup *group)
{
  gimp_action_group_add_actions (group, "dashboard-action",
                                 dashboard_actions,
                                 G_N_ELEMENTS (dashboard_actions));

  gimp_action_group_add_toggle_actions (group, "dashboard-action",
                                        dashboard_toggle_actions,
                                        G_N_ELEMENTS (dashboard_toggle_actions));

  gimp_action_group_add_radio_actions (group, "dashboard-update-interval",
                                       dashboard_update_interval_actions,
                                       G_N_ELEMENTS (dashboard_update_interval_actions),
                                       NULL,
                                       0,
                                       dashboard_update_interval_cmd_callback);

  gimp_action_group_add_radio_actions (group, "dashboard-history-duration",
                                       dashboard_history_duration_actions,
                                       G_N_ELEMENTS (dashboard_history_duration_actions),
                                       NULL,
                                       0,
                                       dashboard_history_duration_cmd_callback);
}

void
dashboard_actions_update (GimpActionGroup *group,
                          gpointer         data)
{
  GimpDashboard *dashboard = GIMP_DASHBOARD (data);
  gboolean       recording;

  recording = gimp_dashboard_log_is_recording (dashboard);

#define SET_SENSITIVE(action,condition) \
        gimp_action_group_set_action_sensitive (group, action, (condition) != 0, NULL)
#define SET_ACTIVE(action,condition) \
        gimp_action_group_set_action_active (group, action, (condition) != 0)

  switch (gimp_dashboard_get_update_interval (dashboard))
    {
    case GIMP_DASHBOARD_UPDATE_INTERVAL_0_25_SEC:
      SET_ACTIVE ("dashboard-update-interval-0-25-sec", TRUE);
      break;
    case GIMP_DASHBOARD_UPDATE_INTERVAL_0_5_SEC:
      SET_ACTIVE ("dashboard-update-interval-0-5-sec", TRUE);
      break;
    case GIMP_DASHBOARD_UPDATE_INTERVAL_1_SEC:
      SET_ACTIVE ("dashboard-update-interval-1-sec", TRUE);
      break;
    case GIMP_DASHBOARD_UPDATE_INTERVAL_2_SEC:
      SET_ACTIVE ("dashboard-update-interval-2-sec", TRUE);
      break;
    case GIMP_DASHBOARD_UPDATE_INTERVAL_4_SEC:
      SET_ACTIVE ("dashboard-update-interval-4-sec", TRUE);
      break;
    }

  switch (gimp_dashboard_get_history_duration (dashboard))
    {
    case GIMP_DASHBOARD_HISTORY_DURATION_15_SEC:
      SET_ACTIVE ("dashboard-history-duration-15-sec", TRUE);
      break;
    case GIMP_DASHBOARD_HISTORY_DURATION_30_SEC:
      SET_ACTIVE ("dashboard-history-duration-30-sec", TRUE);
      break;
    case GIMP_DASHBOARD_HISTORY_DURATION_60_SEC:
      SET_ACTIVE ("dashboard-history-duration-60-sec", TRUE);
      break;
    case GIMP_DASHBOARD_HISTORY_DURATION_120_SEC:
      SET_ACTIVE ("dashboard-history-duration-120-sec", TRUE);
      break;
    case GIMP_DASHBOARD_HISTORY_DURATION_240_SEC:
      SET_ACTIVE ("dashboard-history-duration-240-sec", TRUE);
      break;
    }

  SET_SENSITIVE ("dashboard-log-add-marker",       recording);
  SET_SENSITIVE ("dashboard-log-add-empty-marker", recording);
  SET_SENSITIVE ("dashboard-reset",                !recording);

  SET_ACTIVE ("dashboard-low-swap-space-warning",
              gimp_dashboard_get_low_swap_space_warning (dashboard));

#undef SET_SENSITIVE
#undef SET_ACTIVE
}
