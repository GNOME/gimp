/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ligmasinglewindowstrategy.c
 * Copyright (C) 2011 Martin Nordholts <martinn@src.gnome.org>
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

#include "display-types.h"

#include "core/ligma.h"

#include "widgets/ligmadialogfactory.h"
#include "widgets/ligmadock.h"
#include "widgets/ligmadockbook.h"
#include "widgets/ligmadockcolumns.h"
#include "widgets/ligmawindowstrategy.h"

#include "ligmaimagewindow.h"
#include "ligmasinglewindowstrategy.h"


static void        ligma_single_window_strategy_window_strategy_iface_init (LigmaWindowStrategyInterface *iface);
static GtkWidget * ligma_single_window_strategy_show_dockable_dialog       (LigmaWindowStrategy          *strategy,
                                                                           Ligma                        *ligma,
                                                                           LigmaDialogFactory           *factory,
                                                                           GdkMonitor                  *monitor,
                                                                           const gchar                 *identifiers);


G_DEFINE_TYPE_WITH_CODE (LigmaSingleWindowStrategy, ligma_single_window_strategy, LIGMA_TYPE_OBJECT,
                         G_IMPLEMENT_INTERFACE (LIGMA_TYPE_WINDOW_STRATEGY,
                                                ligma_single_window_strategy_window_strategy_iface_init))

#define parent_class ligma_single_window_strategy_parent_class


static void
ligma_single_window_strategy_class_init (LigmaSingleWindowStrategyClass *klass)
{
}

static void
ligma_single_window_strategy_init (LigmaSingleWindowStrategy *strategy)
{
}

static void
ligma_single_window_strategy_window_strategy_iface_init (LigmaWindowStrategyInterface *iface)
{
  iface->show_dockable_dialog = ligma_single_window_strategy_show_dockable_dialog;
}

static GtkWidget *
ligma_single_window_strategy_show_dockable_dialog (LigmaWindowStrategy *strategy,
                                                  Ligma               *ligma,
                                                  LigmaDialogFactory  *factory,
                                                  GdkMonitor         *monitor,
                                                  const gchar        *identifiers)
{
  GList           *windows = ligma_get_image_windows (ligma);
  GtkWidget       *widget  = NULL;
  LigmaImageWindow *window;

  g_return_val_if_fail (windows != NULL, NULL);

  /* In single-window mode, there should only be one window... */
  window = LIGMA_IMAGE_WINDOW (windows->data);

  if (strcmp ("ligma-toolbox", identifiers) == 0)
    {
      /* Only allow one toolbox... */
      if (! ligma_image_window_has_toolbox (window))
        {
          LigmaDockColumns *columns;
          LigmaUIManager   *ui_manager = ligma_image_window_get_ui_manager (window);

          widget = ligma_dialog_factory_dialog_new (factory, monitor,
                                                   ui_manager,
                                                   GTK_WIDGET (window),
                                                   "ligma-toolbox",
                                                   -1 /*view_size*/,
                                                   FALSE /*present*/);
          gtk_widget_show (widget);

          columns = ligma_image_window_get_left_docks (window);
          ligma_dock_columns_add_dock (columns,
                                      LIGMA_DOCK (widget),
                                      -1 /*index*/);
        }
      else
        {
          widget = ligma_dialog_factory_find_widget (factory, "ligma-toolbox");
        }
    }
  else if (ligma_dialog_factory_find_widget (factory, identifiers))
    {
      /* if the dialog is already open, simply raise it */
      return ligma_dialog_factory_dialog_raise (factory, monitor,
                                               GTK_WIDGET (window),
                                               identifiers, -1);
   }
  else
    {
      GtkWidget *dockbook;

      dockbook = ligma_image_window_get_default_dockbook (window);

      if (! dockbook)
        {
          LigmaDockColumns *dock_columns;

          /* No dock, need to add one */
          dock_columns = ligma_image_window_get_right_docks (window);
          ligma_dock_columns_prepare_dockbook (dock_columns,
                                              -1 /*index*/,
                                              &dockbook);
        }

      widget = ligma_dockbook_add_from_dialog_factory (LIGMA_DOCKBOOK (dockbook),
                                                      identifiers);
    }


  g_list_free (windows);

  return widget;
}

LigmaObject *
ligma_single_window_strategy_get_singleton (void)
{
  static LigmaObject *singleton = NULL;

  if (! singleton)
    singleton = g_object_new (LIGMA_TYPE_SINGLE_WINDOW_STRATEGY, NULL);

  return singleton;
}
