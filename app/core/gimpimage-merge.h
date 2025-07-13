/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
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


GList       * gimp_image_merge_visible_layers  (GimpImage      *image,
                                                GimpContext    *context,
                                                GimpMergeType   merge_type,
                                                gboolean        merge_active_group,
                                                gboolean        discard_invisible,
                                                GimpProgress   *progress);
GList       * gimp_image_merge_down            (GimpImage      *image,
                                                GList          *layers,
                                                GimpContext    *context,
                                                GimpMergeType   merge_type,
                                                GimpProgress   *progress,
                                                GError        **error);
GimpLayer   * gimp_image_merge_group_layer     (GimpImage      *image,
                                                GimpGroupLayer *group);

GimpLayer   * gimp_image_flatten               (GimpImage      *image,
                                                GimpContext    *context,
                                                GimpProgress   *progress,
                                                GError        **error);

GimpPath    * gimp_image_merge_visible_paths   (GimpImage      *image,
                                                GError        **error);
