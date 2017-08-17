/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpoperationlayermode-composite.h
 * Copyright (C) 2017 Michael Natterer <mitch@gimp.org>
 *               2017 Øyvind Kolås <pippin@gimp.org>
 *               2017 Ell
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
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef __GIMP_OPERATION_LAYER_MODE_COMPOSITE_H__
#define __GIMP_OPERATION_LAYER_MODE_COMPOSITE_H__


void gimp_operation_layer_mode_composite_src_over      (const gfloat        *in,
                                                        const gfloat        *layer,
                                                        const gfloat        *comp,
                                                        const gfloat        *mask,
                                                        gfloat               opacity,
                                                        gfloat              *out,
                                                        gint                 samples);
void gimp_operation_layer_mode_composite_src_atop      (const gfloat        *in,
                                                        const gfloat        *layer,
                                                        const gfloat        *comp,
                                                        const gfloat        *mask,
                                                        gfloat               opacity,
                                                        gfloat              *out,
                                                        gint                 samples);
void gimp_operation_layer_mode_composite_dst_atop      (const gfloat        *in,
                                                        const gfloat        *layer,
                                                        const gfloat        *comp,
                                                        const gfloat        *mask,
                                                        gfloat               opacity,
                                                        gfloat              *out,
                                                        gint                 samples);
void gimp_operation_layer_mode_composite_src_in        (const gfloat        *in,
                                                        const gfloat        *layer,
                                                        const gfloat        *comp,
                                                        const gfloat        *mask,
                                                        gfloat               opacity,
                                                        gfloat              *out,
                                                        gint                 samples);

void gimp_operation_layer_mode_composite_src_over_sub  (const gfloat        *in,
                                                        const gfloat        *layer,
                                                        const gfloat        *comp,
                                                        const gfloat        *mask,
                                                        gfloat               opacity,
                                                        gfloat              *out,
                                                        gint                 samples);
void gimp_operation_layer_mode_composite_src_atop_sub  (const gfloat        *in,
                                                        const gfloat        *layer,
                                                        const gfloat        *comp,
                                                        const gfloat        *mask,
                                                        gfloat               opacity,
                                                        gfloat              *out,
                                                        gint                 samples);
void gimp_operation_layer_mode_composite_dst_atop_sub  (const gfloat        *in,
                                                        const gfloat        *layer,
                                                        const gfloat        *comp,
                                                        const gfloat        *mask,
                                                        gfloat               opacity,
                                                        gfloat              *out,
                                                        gint                 samples);
void gimp_operation_layer_mode_composite_src_in_sub    (const gfloat        *in,
                                                        const gfloat        *layer,
                                                        const gfloat        *comp,
                                                        const gfloat        *mask,
                                                        gfloat               opacity,
                                                        gfloat              *out,
                                                        gint                 samples);

#if COMPILE_SSE2_INTRINISICS

void gimp_operation_layer_mode_composite_src_atop_sse2 (const gfloat        *in,
                                                        const gfloat        *layer,
                                                        const gfloat        *comp,
                                                        const gfloat        *mask,
                                                        gfloat               opacity,
                                                        gfloat              *out,
                                                        gint                 samples);

#endif /* COMPILE_SSE2_INTRINISICS */


#endif /* __GIMP_OPERATION_LAYER_MODE_COMPOSITE_H__ */
