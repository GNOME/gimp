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


/*  public sample point adding API
 */
GimpSamplePoint * gimp_image_add_sample_point_at_pos (GimpImage        *image,
                                                      gint              x,
                                                      gint              y,
                                                      gboolean          push_undo);

/*  internal sample point adding API, does not check the sample
 *  point's position and is publicly declared only to be used from
 *  undo
 */
void              gimp_image_add_sample_point        (GimpImage       *image,
                                                      GimpSamplePoint *sample_point,
                                                      gint             x,
                                                      gint             y);

void              gimp_image_remove_sample_point     (GimpImage       *image,
                                                      GimpSamplePoint *sample_point,
                                                      gboolean         push_undo);
void              gimp_image_move_sample_point       (GimpImage       *image,
                                                      GimpSamplePoint *sample_point,
                                                      gint             x,
                                                      gint             y,
                                                      gboolean         push_undo);
void              gimp_image_set_sample_point_pick_mode
                                                     (GimpImage       *image,
                                                      GimpSamplePoint *sample_point,
                                                      GimpColorPickMode pick_mode,
                                                      gboolean         push_undo);

GList           * gimp_image_get_sample_points       (GimpImage       *image);
GimpSamplePoint * gimp_image_get_sample_point        (GimpImage       *image,
                                                      guint32          id);
GimpSamplePoint * gimp_image_get_next_sample_point   (GimpImage       *image,
                                                      guint32          id,
                                                      gboolean        *sample_point_found);
