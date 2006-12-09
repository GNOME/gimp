/* GIMP - The GNU Image Manipulation Program
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


void   gimp_transform_matrix_flip          (GimpMatrix3         *matrix,
                                            GimpOrientationType  flip_type,
                                            gdouble              axis);
void   gimp_transform_matrix_flip_free     (GimpMatrix3         *matrix,
                                            gdouble              x1,
                                            gdouble              y1,
                                            gdouble              x2,
                                            gdouble              y2);
void   gimp_transform_matrix_rotate        (GimpMatrix3         *matrix,
                                            GimpRotationType     rotate_type,
                                            gdouble              center_x,
                                            gdouble              center_y);
void   gimp_transform_matrix_rotate_rect   (GimpMatrix3         *matrix,
                                            gint                 x,
                                            gint                 y,
                                            gint                 width,
                                            gint                 height,
                                            gdouble              angle);
void   gimp_transform_matrix_rotate_center (GimpMatrix3         *matrix,
                                            gdouble              center_x,
                                            gdouble              center_y,
                                            gdouble              angle);
void   gimp_transform_matrix_scale         (GimpMatrix3         *matrix,
                                            gint                 x,
                                            gint                 y,
                                            gint                 width,
                                            gint                 height,
                                            gdouble              t_x,
                                            gdouble              t_y,
                                            gdouble              t_width,
                                            gdouble              t_height);
void   gimp_transform_matrix_shear         (GimpMatrix3         *matrix,
                                            gint                 x,
                                            gint                 y,
                                            gint                 width,
                                            gint                 height,
                                            GimpOrientationType  orientation,
                                            gdouble              amount);
void   gimp_transform_matrix_perspective   (GimpMatrix3         *matrix,
                                            gint                 x,
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
                                            gdouble              t_y4);


#endif  /*  __GIMP_TRANSFORM_UTILS_H__  */
