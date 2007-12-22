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

#include "config.h"

#include <glib-object.h>

#include "libgimpmath/gimpmath.h"

#include "core-types.h"

#include "gimp.h"
#include "gimpimage.h"
#include "gimpimage-sample-points.h"
#include "gimpimage-undo-push.h"
#include "gimpsamplepoint.h"

#include "gimp-intl.h"


/*  public functions  */

GimpSamplePoint *
gimp_image_add_sample_point_at_pos (GimpImage *image,
                                    gint       x,
                                    gint       y,
                                    gboolean   push_undo)
{
  GimpSamplePoint *sample_point;

  g_return_val_if_fail (GIMP_IS_IMAGE (image), NULL);
  g_return_val_if_fail (x >= 0 && x < image->width, NULL);
  g_return_val_if_fail (y >= 0 && y < image->height, NULL);

  sample_point = gimp_sample_point_new (image->gimp->next_sample_point_ID++);

  if (push_undo)
    gimp_image_undo_push_sample_point (image, _("Add Sample Point"),
                                       sample_point);

  gimp_image_add_sample_point (image, sample_point, x, y);
  gimp_sample_point_unref (sample_point);

  return sample_point;
}

void
gimp_image_add_sample_point (GimpImage       *image,
                             GimpSamplePoint *sample_point,
                             gint             x,
                             gint             y)
{
  g_return_if_fail (GIMP_IS_IMAGE (image));
  g_return_if_fail (sample_point != NULL);
  g_return_if_fail (x >= 0);
  g_return_if_fail (y >= 0);
  g_return_if_fail (x < image->width);
  g_return_if_fail (y < image->height);

  image->sample_points = g_list_append (image->sample_points, sample_point);

  sample_point->x = x;
  sample_point->y = y;
  gimp_sample_point_ref (sample_point);

  gimp_image_sample_point_added (image, sample_point);
  gimp_image_update_sample_point (image, sample_point);
}

void
gimp_image_remove_sample_point (GimpImage       *image,
                                GimpSamplePoint *sample_point,
                                gboolean         push_undo)
{
  GList *list;

  g_return_if_fail (GIMP_IS_IMAGE (image));
  g_return_if_fail (sample_point != NULL);

  gimp_image_update_sample_point (image, sample_point);

  if (push_undo)
    gimp_image_undo_push_sample_point (image, _("Remove Sample Point"),
                                       sample_point);

  list = g_list_find (image->sample_points, sample_point);
  if (list)
    list = g_list_next (list);

  image->sample_points = g_list_remove (image->sample_points, sample_point);

  gimp_image_sample_point_removed (image, sample_point);

  sample_point->x = -1;
  sample_point->y = -1;
  gimp_sample_point_unref (sample_point);

  while (list)
    {
      gimp_image_update_sample_point (image, list->data);
      list = g_list_next (list);
    }
}

void
gimp_image_move_sample_point (GimpImage       *image,
                              GimpSamplePoint *sample_point,
                              gint             x,
                              gint             y,
                              gboolean         push_undo)
{
  g_return_if_fail (GIMP_IS_IMAGE (image));
  g_return_if_fail (sample_point != NULL);
  g_return_if_fail (x >= 0);
  g_return_if_fail (y >= 0);
  g_return_if_fail (x < image->width);
  g_return_if_fail (y < image->height);

  if (push_undo)
    gimp_image_undo_push_sample_point (image, _("Move Sample Point"),
                                       sample_point);

  gimp_image_update_sample_point (image, sample_point);
  sample_point->x = x;
  sample_point->y = y;
  gimp_image_update_sample_point (image, sample_point);
}

GimpSamplePoint *
gimp_image_find_sample_point (GimpImage *image,
                              gdouble    x,
                              gdouble    y,
                              gdouble    epsilon_x,
                              gdouble    epsilon_y)
{
  GList           *list;
  GimpSamplePoint *ret     = NULL;
  gdouble          mindist = G_MAXDOUBLE;

  g_return_val_if_fail (GIMP_IS_IMAGE (image), NULL);
  g_return_val_if_fail (epsilon_x > 0 && epsilon_y > 0, NULL);

  if (x < 0 || x >= image->width ||
      y < 0 || y >= image->height)
    {
      return NULL;
    }

  for (list = image->sample_points; list; list = g_list_next (list))
    {
      GimpSamplePoint *sample_point = list->data;
      gdouble          dist;

      if (sample_point->x < 0 || sample_point->y < 0)
        continue;

      dist = hypot ((sample_point->x + 0.5) - x,
                    (sample_point->y + 0.5) - y);
      if (dist < MIN (epsilon_y, mindist))
        {
          mindist = dist;
          ret = sample_point;
        }
    }

  return ret;
}
