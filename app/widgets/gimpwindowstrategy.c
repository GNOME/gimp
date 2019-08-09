 /* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpwindowstrategy.c
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

#include "gimpwindowstrategy.h"


G_DEFINE_INTERFACE (GimpWindowStrategy, gimp_window_strategy, G_TYPE_OBJECT)


/*  private functions  */


static void
gimp_window_strategy_default_init (GimpWindowStrategyInterface *iface)
{
}


/*  public functions  */


GtkWidget *
gimp_window_strategy_show_dockable_dialog (GimpWindowStrategy *strategy,
                                           Gimp               *gimp,
                                           GimpDialogFactory  *factory,
                                           GdkMonitor         *monitor,
                                           const gchar        *identifiers)
{
  GimpWindowStrategyInterface *iface;

  g_return_val_if_fail (GIMP_IS_WINDOW_STRATEGY (strategy), NULL);

  iface = GIMP_WINDOW_STRATEGY_GET_IFACE (strategy);

  if (iface->show_dockable_dialog)
    return iface->show_dockable_dialog (strategy,
                                        gimp,
                                        factory,
                                        monitor,
                                        identifiers);

  return NULL;
}
