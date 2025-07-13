/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimp-3d-transform-utils.h
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

#pragma once


gdouble       gimp_transform_3d_angle_of_view_to_focal_length   (gdouble            angle_of_view,
                                                                 gdouble            width,
                                                                 gdouble            height);
gdouble       gimp_transform_3d_focal_length_to_angle_of_view   (gdouble            focal_length,
                                                                 gdouble            width,
                                                                 gdouble            height);

gint          gimp_transform_3d_permutation_to_rotation_order   (const gint         permutation[3]);
void          gimp_transform_3d_rotation_order_to_permutation   (gint               rotation_order,
                                                                 gint               permutation[3]);
gint          gimp_transform_3d_rotation_order_reverse          (gint               rotation_order);

void          gimp_transform_3d_vector3_rotate                  (GimpVector3       *vector,
                                                                 const GimpVector3 *axis);
GimpVector3   gimp_transform_3d_vector3_rotate_val              (GimpVector3        vector,
                                                                 GimpVector3        axis);

void          gimp_transform_3d_matrix3_to_matrix4              (const GimpMatrix3 *matrix3,
                                                                 GimpMatrix4       *matrix4,
                                                                 gint               axis);
void          gimp_transform_3d_matrix4_to_matrix3              (const GimpMatrix4 *matrix4,
                                                                 GimpMatrix3       *matrix3,
                                                                 gint               axis);

void          gimp_transform_3d_matrix4_translate               (GimpMatrix4       *matrix,
                                                                 gdouble            x,
                                                                 gdouble            y,
                                                                 gdouble            z);

void          gimp_transform_3d_matrix4_rotate                  (GimpMatrix4       *matrix,
                                                                 const GimpVector3 *axis);
void          gimp_transform_3d_matrix4_rotate_standard         (GimpMatrix4       *matrix,
                                                                 gint               axis,
                                                                 gdouble            angle);

void          gimp_transform_3d_matrix4_rotate_euler            (GimpMatrix4       *matrix,
                                                                 gint               rotation_order,
                                                                 gdouble            angle_x,
                                                                 gdouble            angle_y,
                                                                 gdouble            angle_z,
                                                                 gdouble            pivot_x,
                                                                 gdouble            pivot_y,
                                                                 gdouble            pivot_z);
void          gimp_transform_3d_matrix4_rotate_euler_decompose  (GimpMatrix4       *matrix,
                                                                 gint               rotation_order,
                                                                 gdouble           *angle_x,
                                                                 gdouble           *angle_y,
                                                                 gdouble           *angle_z);

void          gimp_transform_3d_matrix4_perspective             (GimpMatrix4       *matrix,
                                                                 gdouble            camera_x,
                                                                 gdouble            camera_y,
                                                                 gdouble            camera_z);

void          gimp_transform_3d_matrix                          (GimpMatrix3       *matrix,
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
