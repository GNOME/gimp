/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpwindowstrategy.h
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

#ifndef __GIMP_WINDOW_STRATEGY_H__
#define __GIMP_WINDOW_STRATEGY_H__


#define GIMP_TYPE_WINDOW_STRATEGY (gimp_window_strategy_get_type ())
G_DECLARE_INTERFACE (GimpWindowStrategy, gimp_window_strategy, GIMP, WINDOW_STRATEGY, GObject)


struct _GimpWindowStrategyInterface
{
  GTypeInterface base_iface;

  /*  virtual functions  */
  GtkWidget * (* show_dockable_dialog) (GimpWindowStrategy *strategy,
                                        Gimp               *gimp,
                                        GimpDialogFactory  *factory,
                                        GdkMonitor         *monitor,
                                        const gchar        *identifiers);
};


GtkWidget * gimp_window_strategy_show_dockable_dialog (GimpWindowStrategy *strategy,
                                                       Gimp               *gimp,
                                                       GimpDialogFactory  *factory,
                                                       GdkMonitor         *monitor,
                                                       const gchar        *identifiers);


#endif  /*  __GIMP_WINDOW_STRATEGY_H__  */
