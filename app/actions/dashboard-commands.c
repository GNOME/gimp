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
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <gegl.h>
#include <gtk/gtk.h>

#include "libgimpwidgets/gimpwidgets.h"

#include "actions-types.h"

#include "widgets/gimpdashboard.h"
#include "widgets/gimphelp-ids.h"

#include "dashboard-commands.h"

#include "gimp-intl.h"


/*  public functionss */


void
dashboard_update_interval_cmd_callback (GtkAction *action,
                                        GtkAction *current,
                                        gpointer   data)
{
  GimpDashboard              *dashboard = GIMP_DASHBOARD (data);
  GimpDashboardUpdateInteval  update_interval;

  update_interval = gtk_radio_action_get_current_value (GTK_RADIO_ACTION (action));

  gimp_dashboard_set_update_interval (dashboard, update_interval);
}

void
dashboard_history_duration_cmd_callback (GtkAction *action,
                                         GtkAction *current,
                                         gpointer   data)
{
  GimpDashboard                *dashboard = GIMP_DASHBOARD (data);
  GimpDashboardHistoryDuration  history_duration;

  history_duration = gtk_radio_action_get_current_value (GTK_RADIO_ACTION (action));

  gimp_dashboard_set_history_duration (dashboard, history_duration);
}

void
dashboard_reset_cmd_callback (GtkAction *action,
                              gpointer   data)
{
  GimpDashboard *dashboard = GIMP_DASHBOARD (data);

  gimp_dashboard_reset (dashboard);
}

void
dashboard_low_swap_space_warning_cmd_callback (GtkAction *action,
                                               gpointer   data)
{
  GimpDashboard *dashboard = GIMP_DASHBOARD (data);
  gboolean       low_swap_space_warning;

  low_swap_space_warning = gtk_toggle_action_get_active (GTK_TOGGLE_ACTION (action));

  gimp_dashboard_set_low_swap_space_warning (dashboard, low_swap_space_warning);
}
