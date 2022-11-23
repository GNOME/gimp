/* LIGMA - The GNU Image Manipulation Program
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

#ifndef __LIGMA_TRANSFORM_UTILS_H__
#define __LIGMA_TRANSFORM_UTILS_H__


#define LIGMA_TRANSFORM_NEAR_Z 0.02


void       ligma_transform_get_rotate_center    (gint                 x,
                                                gint                 y,
                                                gint                 width,
                                                gint                 height,
                                                gboolean             auto_center,
                                                gdouble             *center_x,
                                                gdouble             *center_y);
void       ligma_transform_get_flip_axis        (gint                 x,
                                                gint                 y,
                                                gint                 width,
                                                gint                 height,
                                                LigmaOrientationType  flip_type,
                                                gboolean             auto_center,
                                                gdouble             *axis);

void       ligma_transform_matrix_flip          (LigmaMatrix3         *matrix,
                                                LigmaOrientationType  flip_type,
                                                gdouble              axis);
void       ligma_transform_matrix_flip_free     (LigmaMatrix3         *matrix,
                                                gdouble              x1,
                                                gdouble              y1,
                                                gdouble              x2,
                                                gdouble              y2);
void       ligma_transform_matrix_rotate        (LigmaMatrix3         *matrix,
                                                LigmaRotationType     rotate_type,
                                                gdouble              center_x,
                                                gdouble              center_y);
void       ligma_transform_matrix_rotate_rect   (LigmaMatrix3         *matrix,
                                                gint                 x,
                                                gint                 y,
                                                gint                 width,
                                                gint                 height,
                                                gdouble              angle);
void       ligma_transform_matrix_rotate_center (LigmaMatrix3         *matrix,
                                                gdouble              center_x,
                                                gdouble              center_y,
                                                gdouble              angle);
void       ligma_transform_matrix_scale         (LigmaMatrix3         *matrix,
                                                gint                 x,
                                                gint                 y,
                                                gint                 width,
                                                gint                 height,
                                                gdouble              t_x,
                                                gdouble              t_y,
                                                gdouble              t_width,
                                                gdouble              t_height);
void       ligma_transform_matrix_shear         (LigmaMatrix3         *matrix,
                                                gint                 x,
                                                gint                 y,
                                                gint                 width,
                                                gint                 height,
                                                LigmaOrientationType  orientation,
                                                gdouble              amount);
void       ligma_transform_matrix_perspective   (LigmaMatrix3         *matrix,
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
gboolean   ligma_transform_matrix_generic       (LigmaMatrix3         *matrix,
                                                const LigmaVector2    input_points[4],
                                                const LigmaVector2    output_points[4]);

gboolean   ligma_transform_polygon_is_convex    (gdouble              x1,
                                                gdouble              y1,
                                                gdouble              x2,
                                                gdouble              y2,
                                                gdouble              x3,
                                                gdouble              y3,
                                                gdouble              x4,
                                                gdouble              y4);

void       ligma_transform_polygon              (const LigmaMatrix3   *matrix,
                                                const LigmaVector2   *vertices,
                                                gint                 n_vertices,
                                                gboolean             closed,
                                                LigmaVector2         *t_vertices,
                                                gint                *n_t_vertices);
void       ligma_transform_polygon_coords       (const LigmaMatrix3   *matrix,
                                                const LigmaCoords    *vertices,
                                                gint                 n_vertices,
                                                gboolean             closed,
                                                LigmaCoords          *t_vertices,
                                                gint                *n_t_vertices);

void       ligma_transform_bezier_coords        (const LigmaMatrix3   *matrix,
                                                const LigmaCoords     bezier[4],
                                                GQueue              *t_beziers[2],
                                                gint                *n_t_beziers,
                                                gboolean            *start_in,
                                                gboolean            *end_in);


#endif  /*  __LIGMA_TRANSFORM_UTILS_H__  */
