/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995-2001 Spencer Kimball, Peter Mattis, and others
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

#ifndef __GIMP_TRANSFORM_UTILS_H__
#define __GIMP_TRANSFORM_UTILS_H__


#define GIMP_TRANSFORM_NEAR_Z 0.02


void       gimp_transform_get_rotate_center    (gint                 x,
                                                gint                 y,
                                                gint                 width,
                                                gint                 height,
                                                gboolean             auto_center,
                                                gdouble             *center_x,
                                                gdouble             *center_y);
void       gimp_transform_get_flip_axis        (gint                 x,
                                                gint                 y,
                                                gint                 width,
                                                gint                 height,
                                                GimpOrientationType  flip_type,
                                                gboolean             auto_center,
                                                gdouble             *axis);

void       gimp_transform_matrix_flip          (GimpMatrix3         *matrix,
                                                GimpOrientationType  flip_type,
                                                gdouble              axis);
void       gimp_transform_matrix_flip_free     (GimpMatrix3         *matrix,
                                                gdouble              x1,
                                                gdouble              y1,
                                                gdouble              x2,
                                                gdouble              y2);
void       gimp_transform_matrix_rotate        (GimpMatrix3         *matrix,
                                                GimpRotationType     rotate_type,
                                                gdouble              center_x,
                                                gdouble              center_y);
void       gimp_transform_matrix_rotate_rect   (GimpMatrix3         *matrix,
                                                gint                 x,
                                                gint                 y,
                                                gint                 width,
                                                gint                 height,
                                                gdouble              angle);
void       gimp_transform_matrix_rotate_center (GimpMatrix3         *matrix,
                                                gdouble              center_x,
                                                gdouble              center_y,
                                                gdouble              angle);
void       gimp_transform_matrix_scale         (GimpMatrix3         *matrix,
                                                gint                 x,
                                                gint                 y,
                                                gint                 width,
                                                gint                 height,
                                                gdouble              t_x,
                                                gdouble              t_y,
                                                gdouble              t_width,
                                                gdouble              t_height);
void       gimp_transform_matrix_shear         (GimpMatrix3         *matrix,
                                                gint                 x,
                                                gint                 y,
                                                gint                 width,
                                                gint                 height,
                                                GimpOrientationType  orientation,
                                                gdouble              amount);
void       gimp_transform_matrix_perspective   (GimpMatrix3         *matrix,
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
gboolean   gimp_transform_matrix_generic       (GimpMatrix3         *matrix,
                                                const GimpVector2    input_points[4],
                                                const GimpVector2    output_points[4]);

gboolean   gimp_transform_polygon_is_convex    (gdouble              x1,
                                                gdouble              y1,
                                                gdouble              x2,
                                                gdouble              y2,
                                                gdouble              x3,
                                                gdouble              y3,
                                                gdouble              x4,
                                                gdouble              y4);

void       gimp_transform_polygon              (const GimpMatrix3   *matrix,
                                                const GimpVector2   *vertices,
                                                gint                 n_vertices,
                                                gboolean             closed,
                                                GimpVector2         *t_vertices,
                                                gint                *n_t_vertices);
void       gimp_transform_polygon_coords       (const GimpMatrix3   *matrix,
                                                const GimpCoords    *vertices,
                                                gint                 n_vertices,
                                                gboolean             closed,
                                                GimpCoords          *t_vertices,
                                                gint                *n_t_vertices);

void       gimp_transform_bezier_coords        (const GimpMatrix3   *matrix,
                                                const GimpCoords     bezier[4],
                                                GQueue              *t_beziers[2],
                                                gint                *n_t_beziers,
                                                gboolean            *start_in,
                                                gboolean            *end_in);


#endif  /*  __GIMP_TRANSFORM_UTILS_H__  */
