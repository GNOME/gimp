/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ligmadashboard.h
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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef __LIGMA_DASHBOARD_H__
#define __LIGMA_DASHBOARD_H__


#include "ligmaeditor.h"


struct _LigmaDashboardLogParams
{
  gint     sample_frequency;
  gboolean backtrace;
  gboolean messages;
  gboolean progressive;
};


#define LIGMA_TYPE_DASHBOARD            (ligma_dashboard_get_type ())
#define LIGMA_DASHBOARD(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), LIGMA_TYPE_DASHBOARD, LigmaDashboard))
#define LIGMA_DASHBOARD_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), LIGMA_TYPE_DASHBOARD, LigmaDashboardClass))
#define LIGMA_IS_DASHBOARD(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LIGMA_TYPE_DASHBOARD))
#define LIGMA_IS_DASHBOARD_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), LIGMA_TYPE_DASHBOARD))
#define LIGMA_DASHBOARD_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), LIGMA_TYPE_DASHBOARD, LigmaDashboardClass))


typedef struct _LigmaDashboardPrivate LigmaDashboardPrivate;
typedef struct _LigmaDashboardClass   LigmaDashboardClass;

struct _LigmaDashboard
{
  LigmaEditor            parent_instance;

  LigmaDashboardPrivate *priv;
};

struct _LigmaDashboardClass
{
  LigmaEditorClass  parent_class;
};


GType                          ligma_dashboard_get_type                   (void) G_GNUC_CONST;

GtkWidget                    * ligma_dashboard_new                        (Ligma                          *ligma,
                                                                          LigmaMenuFactory               *menu_factory);

gboolean                       ligma_dashboard_log_start_recording        (LigmaDashboard                 *dashboard,
                                                                          GFile                         *file,
                                                                          const LigmaDashboardLogParams  *params,
                                                                          GError                       **error);
gboolean                       ligma_dashboard_log_stop_recording         (LigmaDashboard                 *dashboard,
                                                                          GError                       **error);
gboolean                       ligma_dashboard_log_is_recording           (LigmaDashboard                 *dashboard);
const LigmaDashboardLogParams * ligma_dashboard_log_get_default_params     (LigmaDashboard                 *dashboard);
void                           ligma_dashboard_log_add_marker             (LigmaDashboard                 *dashboard,
                                                                          const gchar                   *description);

void                           ligma_dashboard_reset                      (LigmaDashboard                 *dashboard);

void                           ligma_dashboard_set_update_interval        (LigmaDashboard                 *dashboard,
                                                                          LigmaDashboardUpdateInteval     update_interval);
LigmaDashboardUpdateInteval     ligma_dashboard_get_update_interval        (LigmaDashboard                 *dashboard);

void                           ligma_dashboard_set_history_duration       (LigmaDashboard                 *dashboard,
                                                                          LigmaDashboardHistoryDuration   history_duration);
LigmaDashboardHistoryDuration   ligma_dashboard_get_history_duration       (LigmaDashboard                 *dashboard);

void                           ligma_dashboard_set_low_swap_space_warning (LigmaDashboard                 *dashboard,
                                                                          gboolean                       low_swap_space_warning);
gboolean                       ligma_dashboard_get_low_swap_space_warning (LigmaDashboard                 *dashboard);

void                           ligma_dashboard_menu_setup                 (LigmaUIManager                 *manager,
                                                                          const gchar                   *ui_path);


#endif  /*  __LIGMA_DASHBOARD_H__  */
