/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
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

#ifndef __GIMP_RULER_H__
#define __GIMP_RULER_H__

G_BEGIN_DECLS

#define GIMP_TYPE_RULER (gimp_ruler_get_type ())
G_DECLARE_FINAL_TYPE (GimpRuler, gimp_ruler, GIMP, RULER, GtkWidget)


GtkWidget * gimp_ruler_new                 (GtkOrientation  orientation);

void        gimp_ruler_add_track_widget    (GimpRuler      *ruler,
                                            GtkWidget      *widget);
void        gimp_ruler_remove_track_widget (GimpRuler      *ruler,
                                            GtkWidget      *widget);

void        gimp_ruler_set_unit            (GimpRuler      *ruler,
                                            GimpUnit       *unit);
GimpUnit  * gimp_ruler_get_unit            (GimpRuler      *ruler);
void        gimp_ruler_set_position        (GimpRuler      *ruler,
                                            gdouble         position);
gdouble     gimp_ruler_get_position        (GimpRuler      *ruler);
void        gimp_ruler_set_range           (GimpRuler      *ruler,
                                            gdouble         lower,
                                            gdouble         upper,
                                            gdouble         max_size);
void        gimp_ruler_get_range           (GimpRuler      *ruler,
                                            gdouble        *lower,
                                            gdouble        *upper,
                                            gdouble        *max_size);

G_END_DECLS

#endif /* __GIMP_RULER_H__ */
