/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995-1997 Spencer Kimball and Peter Mattis
 *
 * gimpproptable.h
 * Copyright (C) 2002-2013 Michael Natterer <mitch@gimp.org>
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

#ifndef __GIMP_PROP_TABLE_H__
#define __GIMP_PROP_TABLE_H__


/*  A view on all of an object's properties  */

typedef GtkWidget * (* GimpCreatePickerFunc) (gpointer     creator,
                                              const gchar *property_name,
                                              const gchar *stock_id,
                                              const gchar *tooltip);

GtkWidget * gimp_prop_table_new (GObject              *config,
                                 GType                 owner_type,
                                 GimpContext          *context,
                                 GimpCreatePickerFunc  create_picker_func,
                                 gpointer              picker_creator);


#endif /* __GIMP_PROP_TABLE_H__ */
