/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpsinglewindowstrategy.c
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
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <string.h>

#include <gegl.h>
#include <gtk/gtk.h>

#include "display-types.h"

#include "core/gimp.h"

#include "widgets/gimpdialogfactory.h"
#include "widgets/gimpdock.h"
#include "widgets/gimpdockbook.h"
#include "widgets/gimpdockcolumns.h"
#include "widgets/gimpwindowstrategy.h"

#include "gimpimagewindow.h"
#include "gimpsinglewindowstrategy.h"


static void        gimp_single_window_strategy_window_strategy_iface_init (GimpWindowStrategyInterface *iface);
static GtkWidget * gimp_single_window_strategy_show_dockable_dialog       (GimpWindowStrategy          *strategy,
                                                                           Gimp                        *gimp,
                                                                           GimpDialogFactory           *factory,
                                                                           GdkMonitor                  *monitor,
                                                                           const gchar                 *identifiers);


G_DEFINE_TYPE_WITH_CODE (GimpSingleWindowStrategy, gimp_single_window_strategy, GIMP_TYPE_OBJECT,
                         G_IMPLEMENT_INTERFACE (GIMP_TYPE_WINDOW_STRATEGY,
                                                gimp_single_window_strategy_window_strategy_iface_init))

#define parent_class gimp_single_window_strategy_parent_class


static void
gimp_single_window_strategy_class_init (GimpSingleWindowStrategyClass *klass)
{
}

static void
gimp_single_window_strategy_init (GimpSingleWindowStrategy *strategy)
{
}

static void
gimp_single_window_strategy_window_strategy_iface_init (GimpWindowStrategyInterface *iface)
{
  iface->show_dockable_dialog = gimp_single_window_strategy_show_dockable_dialog;
}

static GtkWidget *
gimp_single_window_strategy_show_dockable_dialog (GimpWindowStrategy *strategy,
                                                  Gimp               *gimp,
                                                  GimpDialogFactory  *factory,
                                                  GdkMonitor         *monitor,
                                                  const gchar        *identifiers)
{
  GList           *windows = gimp_get_image_windows (gimp);
  GtkWidget       *widget  = NULL;
  GimpImageWindow *window;

  g_return_val_if_fail (windows != NULL, NULL);

  /* In single-window mode, there should only be one window... */
  window = GIMP_IMAGE_WINDOW (windows->data);

  if (strcmp ("gimp-toolbox", identifiers) == 0)
    {
      /* Only allow one toolbox... */
      if (! gimp_image_window_has_toolbox (window))
        {
          GimpDockColumns *columns;
          GimpUIManager   *ui_manager = gimp_image_window_get_ui_manager (window);

          widget = gimp_dialog_factory_dialog_new (factory, monitor,
                                                   ui_manager,
                                                   GTK_WIDGET (window),
                                                   "gimp-toolbox",
                                                   -1 /*view_size*/,
                                                   FALSE /*present*/);
          gtk_widget_show (widget);

          columns = gimp_image_window_get_left_docks (window);
          gimp_dock_columns_add_dock (columns,
                                      GIMP_DOCK (widget),
                                      -1 /*index*/);
        }
    }
  else if (gimp_dialog_factory_find_widget (factory, identifiers))
    {
      /* if the dialog is already open, simply raise it */
      return gimp_dialog_factory_dialog_raise (factory, monitor,
                                               GTK_WIDGET (window),
                                               identifiers, -1);
   }
  else
    {
      GtkWidget *dockbook;

      dockbook = gimp_image_window_get_default_dockbook (window);

      if (! dockbook)
        {
          GimpDockColumns *dock_columns;

          /* No dock, need to add one */
          dock_columns = gimp_image_window_get_right_docks (window);
          gimp_dock_columns_prepare_dockbook (dock_columns,
                                              -1 /*index*/,
                                              &dockbook);
        }

      widget = gimp_dockbook_add_from_dialog_factory (GIMP_DOCKBOOK (dockbook),
                                                      identifiers,
                                                      -1 /*index*/);
    }


  g_list_free (windows);

  return widget;
}

GimpObject *
gimp_single_window_strategy_get_singleton (void)
{
  static GimpObject *singleton = NULL;

  if (! singleton)
    singleton = g_object_new (GIMP_TYPE_SINGLE_WINDOW_STRATEGY, NULL);

  return singleton;
}
