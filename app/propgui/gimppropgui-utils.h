/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995-1997 Spencer Kimball and Peter Mattis
 *
 * ligmapropgui-utils.h
 * Copyright (C) 2002-2017  Michael Natterer <mitch@ligma.org>
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

#ifndef __LIGMA_PROP_GUI_UTILS_H__
#define __LIGMA_PROP_GUI_UTILS_H__


GtkWidget * ligma_prop_kelvin_presets_new (GObject     *config,
                                          const gchar *property_name);

GtkWidget * ligma_prop_random_seed_new    (GObject     *config,
                                          const gchar *property_name);


#endif /* __LIGMA_PROP_GUI_UTILS_H__ */
