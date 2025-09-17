/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimprow.h
 * Copyright (C) 2025 Michael Natterer <mitch@gimp.org>
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

#ifndef __GIMP_ROW_H__
#define __GIMP_ROW_H__


#define GIMP_TYPE_ROW (gimp_row_get_type ())
G_DECLARE_DERIVABLE_TYPE (GimpRow,
                          gimp_row,
                          GIMP, ROW,
                          GtkListBoxRow)


struct _GimpRowClass
{
  GtkListBoxRowClass  parent_class;
};


GtkWidget    * gimp_row_new          (GimpViewable *viewable);

void           gimp_row_set_viewable (GimpRow      *row,
                                      GimpViewable *viewable);
GimpViewable * gimp_row_get_viewable (GimpRow      *row);


/*  a generic GtkListBoxCreateWidgetFunc  */
GtkWidget    * gimp_row_create       (gpointer      item,
                                      gpointer      user_data);


#endif /* __GIMP_ROW_H__ */
