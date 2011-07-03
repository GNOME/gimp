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

#include <gegl.h>
#include <gtk/gtk.h>

#include "display-types.h"

#include "core/gimp.h"

#include "widgets/gimpdockcolumns.h"
#include "widgets/gimpdockbook.h"

#include "display/gimpimagewindow.h"

#include "gimpsinglewindowstrategy.h"
#include "gimpwindowstrategy.h"


static void        gimp_single_window_strategy_window_strategy_iface_init (GimpWindowStrategyInterface *iface);
static GtkWidget * gimp_single_window_strategy_create_dockable_dialog     (GimpWindowStrategy          *strategy,
                                                                           Gimp                        *gimp,
                                                                           GimpDialogFactory           *factory,
                                                                           GdkScreen                   *screen,
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
  iface->create_dockable_dialog = gimp_single_window_strategy_create_dockable_dialog;
}

static GtkWidget *
gimp_single_window_strategy_create_dockable_dialog (GimpWindowStrategy *strategy,
                                                    Gimp               *gimp,
                                                    GimpDialogFactory  *factory,
                                                    GdkScreen          *screen,
                                                    const gchar        *identifiers)
{
  GList           *windows = gimp_get_image_windows (gimp);
  GtkWidget       *dockbook;
  GtkWidget       *dockable;
  GimpImageWindow *window;

  g_return_val_if_fail (g_list_length (windows) > 0, NULL);

  /* In single-window mode, there should only be one window... */
  window = GIMP_IMAGE_WINDOW (windows->data);

  /* There shall not is more than one window in single-window mode */
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

  dockable = gimp_dockbook_add_from_dialog_factory (GIMP_DOCKBOOK (dockbook),
                                                    identifiers,
                                                    -1 /*index*/);

  g_list_free (windows);

  return dockable;
}

GimpObject *
gimp_single_window_strategy_get_singleton (void)
{
  static GimpObject *singleton = NULL;

  if (! singleton)
    singleton = g_object_new (GIMP_TYPE_SINGLE_WINDOW_STRATEGY, NULL);

  return singleton;
}
