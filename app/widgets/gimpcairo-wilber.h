/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * Wilber Cairo rendering
 * Copyright (C) 2008  Sven Neumann <sven@gimp.org>
 *
 * Some code here is based on code from librsvg that was originally
 * written by Raph Levien <raph@artofcode.com> for Gill.
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

#ifndef __GIMP_CAIRO_WILBER_H__
#define __GIMP_CAIRO_WILBER_H__


void   gimp_cairo_wilber_toggle_pointer_eyes (void);


void   gimp_cairo_draw_toolbox_wilber        (GtkWidget *widget,
                                              cairo_t   *cr);
void   gimp_cairo_draw_drop_wilber           (GtkWidget *widget,
                                              cairo_t   *cr,
                                              gboolean   blink);

void   gimp_cairo_wilber                     (cairo_t   *cr,
                                              gdouble    x,
                                              gdouble    y);
void   gimp_cairo_wilber_get_size            (cairo_t   *cr,
                                              gdouble   *width,
                                              gdouble   *height);


#endif /* __GIMP_CAIRO_WILBER_H__ */
