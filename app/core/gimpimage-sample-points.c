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
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <gegl.h>

#include "libgimpmath/gimpmath.h"

#include "core-types.h"

#include "gimp.h"
#include "gimpimage.h"
#include "gimpimage-private.h"
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
  g_return_val_if_fail (x >= 0 && x < gimp_image_get_width  (image), NULL);
  g_return_val_if_fail (y >= 0 && y < gimp_image_get_height (image), NULL);

  sample_point = gimp_sample_point_new (image->gimp->next_sample_point_ID++);

  if (push_undo)
    gimp_image_undo_push_sample_point (image, C_("undo-type", "Add Sample Point"),
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
  GimpImagePrivate *private;

  g_return_if_fail (GIMP_IS_IMAGE (image));
  g_return_if_fail (sample_point != NULL);

  private = GIMP_IMAGE_GET_PRIVATE (image);

  private->sample_points = g_list_append (private->sample_points, sample_point);

  sample_point->x = x;
  sample_point->y = y;
  gimp_sample_point_ref (sample_point);

  gimp_image_sample_point_added (image, sample_point);
}

void
gimp_image_remove_sample_point (GimpImage       *image,
                                GimpSamplePoint *sample_point,
                                gboolean         push_undo)
{
  GimpImagePrivate *private;

  g_return_if_fail (GIMP_IS_IMAGE (image));
  g_return_if_fail (sample_point != NULL);

  private = GIMP_IMAGE_GET_PRIVATE (image);

  if (push_undo)
    gimp_image_undo_push_sample_point (image,
                                       C_("undo-type", "Remove Sample Point"),
                                       sample_point);

  private->sample_points = g_list_remove (private->sample_points, sample_point);

  gimp_image_sample_point_removed (image, sample_point);

  sample_point->x = -1;
  sample_point->y = -1;
  gimp_sample_point_unref (sample_point);
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
  g_return_if_fail (x < gimp_image_get_width  (image));
  g_return_if_fail (y < gimp_image_get_height (image));

  if (push_undo)
    gimp_image_undo_push_sample_point (image,
                                       C_("undo-type", "Move Sample Point"),
                                       sample_point);

  sample_point->x = x;
  sample_point->y = y;

  gimp_image_sample_point_moved (image, sample_point);
}

GList *
gimp_image_get_sample_points (GimpImage *image)
{
  g_return_val_if_fail (GIMP_IS_IMAGE (image), NULL);

  return GIMP_IMAGE_GET_PRIVATE (image)->sample_points;
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

  if (x < 0 || x >= gimp_image_get_width  (image) ||
      y < 0 || y >= gimp_image_get_height (image))
    {
      return NULL;
    }

  for (list = GIMP_IMAGE_GET_PRIVATE (image)->sample_points;
       list;
       list = g_list_next (list))
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
