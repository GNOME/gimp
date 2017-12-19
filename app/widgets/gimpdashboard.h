/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpdashboard.h
 * Copyright (C) 2017 Ell
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

#ifndef __GIMP_DASHBOARD_H__
#define __GIMP_DASHBOARD_H__


#include "gimpeditor.h"


#define GIMP_TYPE_DASHBOARD            (gimp_dashboard_get_type ())
#define GIMP_DASHBOARD(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_DASHBOARD, GimpDashboard))
#define GIMP_DASHBOARD_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_DASHBOARD, GimpDashboardClass))
#define GIMP_IS_DASHBOARD(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_DASHBOARD))
#define GIMP_IS_DASHBOARD_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_DASHBOARD))
#define GIMP_DASHBOARD_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_DASHBOARD, GimpDashboardClass))


typedef struct _GimpDashboardClass GimpDashboardClass;

struct _GimpDashboard
{
  GimpEditor                    parent_instance;

  Gimp                         *gimp;

  GtkWidget                    *box;

  GtkWidget                    *cache_meter;
  GtkWidget                    *cache_occupied_label;
  GtkWidget                    *cache_limit_label;

  GtkWidget                    *swap_meter;
  GtkWidget                    *swap_occupied_label;
  GtkWidget                    *swap_size_label;
  GtkWidget                    *swap_limit_label;

  gint                          timeout_id;
  gint                          low_swap_space_idle_id;

  GThread                      *thread;
  GMutex                        mutex;
  GCond                         cond;
  gboolean                      quit;

  GimpDashboardUpdateInteval    update_interval;
  GimpDashboardHistoryDuration  history_duration;
  gboolean                      low_swap_space_warning;
};

struct _GimpDashboardClass
{
  GimpEditorClass  parent_class;
};


GType       gimp_dashboard_get_type                   (void) G_GNUC_CONST;

GtkWidget * gimp_dashboard_new                        (Gimp                         *gimp,
                                                       GimpMenuFactory              *menu_factory);

void        gimp_dashboard_set_update_interval        (GimpDashboard                *dashboard,
                                                       GimpDashboardUpdateInteval    update_interval);
void        gimp_dashboard_set_history_duration       (GimpDashboard                *dashboard,
                                                       GimpDashboardHistoryDuration  history_duration);
void        gimp_dashboard_set_low_swap_space_warning (GimpDashboard                *dashboard,
                                                       gboolean                      low_swap_space_warning);


#endif  /*  __GIMP_DASHBOARD_H__  */
