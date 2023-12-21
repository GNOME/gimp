/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimp-cairo.h
 * Copyright (C) 2010-2012  Michael Natterer <mitch@gimp.org>
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

#ifndef __APP_GIMP_CAIRO_H__
#define __APP_GIMP_CAIRO_H__


cairo_pattern_t * gimp_cairo_pattern_create_stipple (GeglColor   *fg,
                                                     GeglColor   *bg,
                                                     gint         index,
                                                     gdouble      offset_x,
                                                     gdouble      offset_y,
                                                     const Babl  *render_space);

void              gimp_cairo_arc                    (cairo_t     *cr,
                                                     gdouble      center_x,
                                                     gdouble      center_y,
                                                     gdouble      radius,
                                                     gdouble      start_angle,
                                                     gdouble      slice_angle);
void              gimp_cairo_rounded_rectangle      (cairo_t     *cr,
                                                     gdouble      x,
                                                     gdouble      y,
                                                     gdouble      width,
                                                     gdouble      height,
                                                     gdouble      corner_radius);
void              gimp_cairo_segments               (cairo_t     *cr,
                                                     GimpSegment *segs,
                                                     gint         n_segs);


#endif /* __APP_GIMP_CAIRO_H__ */
