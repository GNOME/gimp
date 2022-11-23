/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ligmamultiwindowstrategy.c
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

#include "core/ligma.h"

#include "widgets/ligmadialogfactory.h"
#include "widgets/ligmawindowstrategy.h"

#include "ligmamultiwindowstrategy.h"


static void        ligma_multi_window_strategy_window_strategy_iface_init (LigmaWindowStrategyInterface *iface);
static GtkWidget * ligma_multi_window_strategy_show_dockable_dialog       (LigmaWindowStrategy          *strategy,
                                                                          Ligma                        *ligma,
                                                                          LigmaDialogFactory           *factory,
                                                                          GdkMonitor                  *monitor,
                                                                          const gchar                 *identifiers);


G_DEFINE_TYPE_WITH_CODE (LigmaMultiWindowStrategy, ligma_multi_window_strategy, LIGMA_TYPE_OBJECT,
                         G_IMPLEMENT_INTERFACE (LIGMA_TYPE_WINDOW_STRATEGY,
                                                ligma_multi_window_strategy_window_strategy_iface_init))

#define parent_class ligma_multi_window_strategy_parent_class


static void
ligma_multi_window_strategy_class_init (LigmaMultiWindowStrategyClass *klass)
{
}

static void
ligma_multi_window_strategy_init (LigmaMultiWindowStrategy *strategy)
{
}

static void
ligma_multi_window_strategy_window_strategy_iface_init (LigmaWindowStrategyInterface *iface)
{
  iface->show_dockable_dialog = ligma_multi_window_strategy_show_dockable_dialog;
}

static GtkWidget *
ligma_multi_window_strategy_show_dockable_dialog (LigmaWindowStrategy *strategy,
                                                 Ligma               *ligma,
                                                 LigmaDialogFactory  *factory,
                                                 GdkMonitor         *monitor,
                                                 const gchar        *identifiers)
{
  return ligma_dialog_factory_dialog_raise (factory, monitor, NULL,
                                           identifiers, -1);
}

LigmaObject *
ligma_multi_window_strategy_get_singleton (void)
{
  static LigmaObject *singleton = NULL;

  if (! singleton)
    singleton = g_object_new (LIGMA_TYPE_MULTI_WINDOW_STRATEGY, NULL);

  return singleton;
}
