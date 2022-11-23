/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ligma-3d-transform-utils.h
 * Copyright (C) 2019 Ell
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

#ifndef __LIGMA_TRANSFORM_3D_UTILS_H__
#define __LIGMA_TRANSFORM_3D_UTILS_H__


gdouble       ligma_transform_3d_angle_of_view_to_focal_length   (gdouble            angle_of_view,
                                                                 gdouble            width,
                                                                 gdouble            height);
gdouble       ligma_transform_3d_focal_length_to_angle_of_view   (gdouble            focal_length,
                                                                 gdouble            width,
                                                                 gdouble            height);

gint          ligma_transform_3d_permutation_to_rotation_order   (const gint         permutation[3]);
void          ligma_transform_3d_rotation_order_to_permutation   (gint               rotation_order,
                                                                 gint               permutation[3]);
gint          ligma_transform_3d_rotation_order_reverse          (gint               rotation_order);

void          ligma_transform_3d_vector3_rotate                  (LigmaVector3       *vector,
                                                                 const LigmaVector3 *axis);
LigmaVector3   ligma_transform_3d_vector3_rotate_val              (LigmaVector3        vector,
                                                                 LigmaVector3        axis);

void          ligma_transform_3d_matrix3_to_matrix4              (const LigmaMatrix3 *matrix3,
                                                                 LigmaMatrix4       *matrix4,
                                                                 gint               axis);
void          ligma_transform_3d_matrix4_to_matrix3              (const LigmaMatrix4 *matrix4,
                                                                 LigmaMatrix3       *matrix3,
                                                                 gint               axis);

void          ligma_transform_3d_matrix4_translate               (LigmaMatrix4       *matrix,
                                                                 gdouble            x,
                                                                 gdouble            y,
                                                                 gdouble            z);

void          ligma_transform_3d_matrix4_rotate                  (LigmaMatrix4       *matrix,
                                                                 const LigmaVector3 *axis);
void          ligma_transform_3d_matrix4_rotate_standard         (LigmaMatrix4       *matrix,
                                                                 gint               axis,
                                                                 gdouble            angle);

void          ligma_transform_3d_matrix4_rotate_euler            (LigmaMatrix4       *matrix,
                                                                 gint               rotation_order,
                                                                 gdouble            angle_x,
                                                                 gdouble            angle_y,
                                                                 gdouble            angle_z,
                                                                 gdouble            pivot_x,
                                                                 gdouble            pivot_y,
                                                                 gdouble            pivot_z);
void          ligma_transform_3d_matrix4_rotate_euler_decompose  (LigmaMatrix4       *matrix,
                                                                 gint               rotation_order,
                                                                 gdouble           *angle_x,
                                                                 gdouble           *angle_y,
                                                                 gdouble           *angle_z);

void          ligma_transform_3d_matrix4_perspective             (LigmaMatrix4       *matrix,
                                                                 gdouble            camera_x,
                                                                 gdouble            camera_y,
                                                                 gdouble            camera_z);

void          ligma_transform_3d_matrix                          (LigmaMatrix3       *matrix,
                                                                 gdouble            camera_x,
                                                                 gdouble            camera_y,
                                                                 gdouble            camera_z,
                                                                 gdouble            offset_x,
                                                                 gdouble            offset_y,
                                                                 gdouble            offset_z,
                                                                 gint               rotation_order,
                                                                 gdouble            angle_x,
                                                                 gdouble            angle_y,
                                                                 gdouble            angle_z,
                                                                 gdouble            pivot_x,
                                                                 gdouble            pivot_y,
                                                                 gdouble            pivot_z);


#endif /* __LIGMA_TRANSFORM_3D_UTILS_H__ */
