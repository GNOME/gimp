/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * gimpbusybox.h
 * Copyright (C) 2018 Ell
 *
 * This library is free software: you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library.  If not, see
 * <https://www.gnu.org/licenses/>.
 */

#if !defined (__GIMP_WIDGETS_H_INSIDE__) && !defined (GIMP_WIDGETS_COMPILATION)
#error "Only <libgimpwidgets/gimpwidgets.h> can be included directly."
#endif

#ifndef __GIMP_BUSY_BOX_H__
#define __GIMP_BUSY_BOX_H__

G_BEGIN_DECLS

#define GIMP_TYPE_BUSY_BOX (gimp_busy_box_get_type ())
G_DECLARE_FINAL_TYPE (GimpBusyBox, gimp_busy_box, GIMP, BUSY_BOX, GtkBox)


GtkWidget   * gimp_busy_box_new         (const gchar *message);

void          gimp_busy_box_set_message (GimpBusyBox *box,
                                         const gchar *message);
const gchar * gimp_busy_box_get_message (GimpBusyBox *box);


G_END_DECLS

#endif /* __GIMP_BUSY_BOX_H__ */
