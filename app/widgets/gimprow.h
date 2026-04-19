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

#pragma once


#define GIMP_TYPE_ROW (gimp_row_get_type ())
G_DECLARE_DERIVABLE_TYPE (GimpRow,
                          gimp_row,
                          GIMP, ROW,
                          GtkListBoxRow)


struct _GimpRowClass
{
  GtkListBoxRowClass  parent_class;

  /*  virtual functions  */
  void     (* set_context)     (GimpRow      *row,
                                GimpContext  *context);
  void     (* set_viewable)    (GimpRow      *row,
                                GimpViewable *viewable);
  void     (* set_view_size)   (GimpRow      *row);

  void     (* icon_changed)    (GimpRow      *row);
  void     (* name_changed)    (GimpRow      *row);
  void     (* monitor_changed) (GimpRow      *row);

  /*  signal for keybinding  */
  void     (* edit_name)       (GimpRow      *row);

  /*  virtual functions  */
  gboolean (* name_edited)     (GimpRow      *row,
                                const gchar  *new_name);
};


GtkWidget    * gimp_row_new             (GimpContext  *context,
                                         GimpViewable *viewable,
                                         gint          view_size,
                                         gint          view_border_width);

void           gimp_row_set_context     (GimpRow      *row,
                                         GimpContext  *context);
GimpContext  * gimp_row_get_context     (GimpRow      *row);

void           gimp_row_set_viewable    (GimpRow      *row,
                                         GimpViewable *viewable);
GimpViewable * gimp_row_get_viewable    (GimpRow      *row);

void           gimp_row_set_view_size   (GimpRow      *row,
                                         gint          view_size,
                                         gint          view_border_width);
gint           gimp_row_get_view_size   (GimpRow      *row,
                                         gint         *view_border_width);

void           gimp_row_monitor_changed (GimpRow      *row);


/*  protected  */

GtkWidget    * _gimp_row_get_box        (GimpRow      *row);
GtkWidget    * _gimp_row_get_icon       (GimpRow      *row);
GtkWidget    * _gimp_row_get_view       (GimpRow      *row);
GtkWidget    * _gimp_row_get_label      (GimpRow      *row);
