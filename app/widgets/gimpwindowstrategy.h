/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ligmawindowstrategy.h
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

#ifndef __LIGMA_WINDOW_STRATEGY_H__
#define __LIGMA_WINDOW_STRATEGY_H__


#define LIGMA_TYPE_WINDOW_STRATEGY (ligma_window_strategy_get_type ())
G_DECLARE_INTERFACE (LigmaWindowStrategy, ligma_window_strategy, LIGMA, WINDOW_STRATEGY, GObject)


struct _LigmaWindowStrategyInterface
{
  GTypeInterface base_iface;

  /*  virtual functions  */
  GtkWidget * (* show_dockable_dialog) (LigmaWindowStrategy *strategy,
                                        Ligma               *ligma,
                                        LigmaDialogFactory  *factory,
                                        GdkMonitor         *monitor,
                                        const gchar        *identifiers);
};


GtkWidget * ligma_window_strategy_show_dockable_dialog (LigmaWindowStrategy *strategy,
                                                       Ligma               *ligma,
                                                       LigmaDialogFactory  *factory,
                                                       GdkMonitor         *monitor,
                                                       const gchar        *identifiers);


#endif  /*  __LIGMA_WINDOW_STRATEGY_H__  */
