/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpmultiwindowstrategy.c
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

#include <gegl.h>
#include <gtk/gtk.h>

#include "display-types.h"

#include "core/gimp.h"

#include "widgets/gimpdialogfactory.h"
#include "widgets/gimpwindowstrategy.h"

#include "gimpmultiwindowstrategy.h"


static void        gimp_multi_window_strategy_window_strategy_iface_init (GimpWindowStrategyInterface *iface);
static GtkWidget * gimp_multi_window_strategy_show_dockable_dialog       (GimpWindowStrategy          *strategy,
                                                                          Gimp                        *gimp,
                                                                          GimpDialogFactory           *factory,
                                                                          GdkMonitor                  *monitor,
                                                                          const gchar                 *identifiers);


G_DEFINE_TYPE_WITH_CODE (GimpMultiWindowStrategy, gimp_multi_window_strategy, GIMP_TYPE_OBJECT,
                         G_IMPLEMENT_INTERFACE (GIMP_TYPE_WINDOW_STRATEGY,
                                                gimp_multi_window_strategy_window_strategy_iface_init))

#define parent_class gimp_multi_window_strategy_parent_class


static void
gimp_multi_window_strategy_class_init (GimpMultiWindowStrategyClass *klass)
{
}

static void
gimp_multi_window_strategy_init (GimpMultiWindowStrategy *strategy)
{
}

static void
gimp_multi_window_strategy_window_strategy_iface_init (GimpWindowStrategyInterface *iface)
{
  iface->show_dockable_dialog = gimp_multi_window_strategy_show_dockable_dialog;
}

static GtkWidget *
gimp_multi_window_strategy_show_dockable_dialog (GimpWindowStrategy *strategy,
                                                 Gimp               *gimp,
                                                 GimpDialogFactory  *factory,
                                                 GdkMonitor         *monitor,
                                                 const gchar        *identifiers)
{
  return gimp_dialog_factory_dialog_raise (factory, monitor, NULL,
                                           identifiers, -1);
}

GimpObject *
gimp_multi_window_strategy_get_singleton (void)
{
  static GimpObject *singleton = NULL;

  if (! singleton)
    singleton = g_object_new (GIMP_TYPE_MULTI_WINDOW_STRATEGY, NULL);

  return singleton;
}
