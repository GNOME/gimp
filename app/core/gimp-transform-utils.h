/* The GIMP -- an image manipulation program
 * Copyright (C) 1995-2001 Spencer Kimball, Peter Mattis, and others
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

#ifndef __GIMP_TRANSFORM_UTILS_H__
#define __GIMP_TRANSFORM_UTILS_H__


void   gimp_transform_matrix_flip          (GimpOrientationType  flip_type,
                                            gdouble              axis,
                                            GimpMatrix3         *result);
void   gimp_transform_matrix_flip_free     (gint                 x,
                                            gint                 y,
                                            gint                 width,
                                            gint                 height,
                                            gdouble              x1,
                                            gdouble              y1,
                                            gdouble              x2,
                                            gdouble              y2,
                                            GimpMatrix3         *result);
void   gimp_transform_matrix_rotate        (gint                 x,
                                            gint                 y,
                                            gint                 width,
                                            gint                 height,
                                            gdouble              angle,
                                            GimpMatrix3         *result);
void   gimp_transform_matrix_rotate_center (gdouble              center_x,
                                            gdouble              center_y,
                                            gdouble              angle,
                                            GimpMatrix3         *result);
void   gimp_transform_matrix_scale         (gint                 x,
                                            gint                 y,
                                            gint                 width,
                                            gint                 height,
                                            gdouble              t_x,
                                            gdouble              t_y,
                                            gdouble              t_width,
                                            gdouble              t_height,
                                            GimpMatrix3         *result);
void   gimp_transform_matrix_shear         (gint                 x,
                                            gint                 y,
                                            gint                 width,
                                            gint                 height,
                                            GimpOrientationType  orientation,
                                            gdouble              amount,
                                            GimpMatrix3         *result);
void   gimp_transform_matrix_perspective   (gint                 x,
                                            gint                 y,
                                            gint                 width,
                                            gint                 height,
                                            gdouble              t_x1,
                                            gdouble              t_y1,
                                            gdouble              t_x2,
                                            gdouble              t_y2,
                                            gdouble              t_x3,
                                            gdouble              t_y3,
                                            gdouble              t_x4,
                                            gdouble              t_y4,
                                            GimpMatrix3         *result);


#endif  /*  __GIMP_TRANSFORM_UTILS_H__  */
