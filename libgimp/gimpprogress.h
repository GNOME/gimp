/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * gimpprogress.h
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#ifndef __GIMP_PROGRESS_H__
#define __GIMP_PROGRESS_H__

G_BEGIN_DECLS


typedef void (* GimpProgressStartCallback) (const gchar *message,
                                            gboolean     cancelable,
                                            gpointer     user_data);
typedef void (* GimpProgressEndCallback)   (gpointer     user_data);
typedef void (* GimpProgressTextCallback)  (const gchar *message,
                                            gpointer     user_data);
typedef void (* GimpProgressValueCallback) (gdouble      percentage,
                                            gpointer     user_data);


const gchar * gimp_progress_install   (GimpProgressStartCallback start_callback,
                                       GimpProgressEndCallback   end_callback,
                                       GimpProgressTextCallback  text_callback,
                                       GimpProgressValueCallback value_callback,
                                       gpointer                  user_data);
gpointer      gimp_progress_uninstall (const gchar              *progress_callback);


G_END_DECLS

#endif /* __GIMP_PROGRESS_H__ */
