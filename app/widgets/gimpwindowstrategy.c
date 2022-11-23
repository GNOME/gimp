 /* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ligmawindowstrategy.c
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

#include <gtk/gtk.h>

#include "widgets-types.h"

#include "ligmawindowstrategy.h"


G_DEFINE_INTERFACE (LigmaWindowStrategy, ligma_window_strategy, G_TYPE_OBJECT)


/*  private functions  */


static void
ligma_window_strategy_default_init (LigmaWindowStrategyInterface *iface)
{
}


/*  public functions  */


GtkWidget *
ligma_window_strategy_show_dockable_dialog (LigmaWindowStrategy *strategy,
                                           Ligma               *ligma,
                                           LigmaDialogFactory  *factory,
                                           GdkMonitor         *monitor,
                                           const gchar        *identifiers)
{
  LigmaWindowStrategyInterface *iface;

  g_return_val_if_fail (LIGMA_IS_WINDOW_STRATEGY (strategy), NULL);

  iface = LIGMA_WINDOW_STRATEGY_GET_IFACE (strategy);

  if (iface->show_dockable_dialog)
    return iface->show_dockable_dialog (strategy,
                                        ligma,
                                        factory,
                                        monitor,
                                        identifiers);

  return NULL;
}
