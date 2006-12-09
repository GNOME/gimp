/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
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

#ifndef __GIMP_IMAGE_SAMPLE_POINTS_H__
#define __GIMP_IMAGE_SAMPLE_POINTS_H__

#define GIMP_SAMPLE_POINT_DRAW_SIZE 10

struct _GimpSamplePoint
{
  gint                 ref_count;
  gint                 x;
  gint                 y;
  guint32              sample_point_ID;
};


GimpSamplePoint * gimp_image_add_sample_point_at_pos (GimpImage        *image,
                                                      gint              x,
                                                      gint              y,
                                                      gboolean          push_undo);
GimpSamplePoint * gimp_image_sample_point_ref        (GimpSamplePoint *sample_point);
void              gimp_image_sample_point_unref      (GimpSamplePoint *sample_point);

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

GimpSamplePoint * gimp_image_find_sample_point       (GimpImage       *image,
                                                      gdouble          x,
                                                      gdouble          y,
                                                      gdouble          epsilon_x,
                                                      gdouble          epsilon_y);


#endif /* __GIMP_IMAGE_SAMPLE_POINTS_H__ */
