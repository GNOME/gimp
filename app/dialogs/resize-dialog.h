/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#ifndef __RESIZE_DIALOG_H__
#define __RESIZE_DIALOG_H__


typedef void (* GimpResizeCallback) (GtkWidget    *dialog,
                                     GimpViewable *viewable,
                                     gint          width,
                                     gint          height,
                                     GimpUnit      unit,
                                     gint          offset_x,
                                     gint          offset_y,
                                     GimpItemSet   layer_set,
                                     gpointer      user_data);


GtkWidget * resize_dialog_new (GimpViewable       *viewable,
                               GimpContext        *context,
                               const gchar        *title,
                               const gchar        *role,
                               GtkWidget          *parent,
                               GimpHelpFunc        help_func,
                               const gchar        *help_id,
                               GimpUnit            unit,
                               GimpResizeCallback  callback,
                               gpointer            user_data);


#endif  /*  __RESIZE_DIALOG_H__  */
