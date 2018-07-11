/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimp-gegl-loops-sse2.h
 * Copyright (C) 2012 Michael Natterer <mitch@gimp.org>
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

#ifndef __GIMP_GEGL_LOOPS_SSE2_H__
#define __GIMP_GEGL_LOOPS_SSE2_H__


#if COMPILE_SSE2_INTRINISICS

void   gimp_gegl_smudge_with_paint_process_sse2 (gfloat       *accum,
                                                 const gfloat *canvas,
                                                 gfloat       *paint,
                                                 gint          count,
                                                 const gfloat *brush_color,
                                                 gfloat        brush_a,
                                                 gboolean      no_erasing,
                                                 gfloat        flow,
                                                 gfloat        rate);

#endif /* COMPILE_SSE2_INTRINISICS */


#endif /* __GIMP_GEGL_LOOPS_SSE2_H__ */
