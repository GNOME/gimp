/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * gimpenumlabel.h
 * Copyright (C) 2005  Sven Neumann <sven@gimp.org>
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

#ifndef __GIMP_ENUM__LABEL_H__
#define __GIMP_ENUM__LABEL_H__

G_BEGIN_DECLS


#define GIMP_TYPE_ENUM_LABEL (gimp_enum_label_get_type ())
G_DECLARE_FINAL_TYPE (GimpEnumLabel, gimp_enum_label, GIMP, ENUM_LABEL, GtkScale)


GtkWidget * gimp_enum_label_new              (GType          enum_type,
                                              gint           value);
void        gimp_enum_label_set_value        (GimpEnumLabel *label,
                                              gint           value);


G_END_DECLS

#endif  /* __GIMP_ENUM_LABEL_H__ */
