/* The GIMP -- an image manipulation program
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

#include "core-types.h"

#include "gimp.h"
#include "gimpimage.h"
#include "gimpimage-sample-points.h"
#include "gimpimage-undo-push.h"

#include "gimp-intl.h"


/*  public functions  */

GimpSamplePoint *
gimp_image_add_sample_point_at_pos (GimpImage *gimage,
                                    gint       x,
                                    gint       y,
                                    gboolean   push_undo)
{
  GimpSamplePoint *sample_point;

  g_return_val_if_fail (GIMP_IS_IMAGE (gimage), NULL);
  g_return_val_if_fail (x >= 0 && x < gimage->width, NULL);
  g_return_val_if_fail (y >= 0 && y < gimage->height, NULL);

  sample_point = g_new0 (GimpSamplePoint, 1);

  sample_point->ref_count       = 1;
  sample_point->x               = -1;
  sample_point->y               = -1;
  sample_point->sample_point_ID = gimage->gimp->next_sample_point_ID++;

  if (push_undo)
    gimp_image_undo_push_image_sample_point (gimage, _("Add Sample_Point"),
                                             sample_point);

  gimp_image_add_sample_point (gimage, sample_point, x, y);
  gimp_image_sample_point_unref (sample_point);

  return sample_point;
}

GimpSamplePoint *
gimp_image_sample_point_ref (GimpSamplePoint *sample_point)
{
  g_return_val_if_fail (sample_point != NULL, NULL);

  sample_point->ref_count++;

  return sample_point;
}

void
gimp_image_sample_point_unref (GimpSamplePoint *sample_point)
{
  g_return_if_fail (sample_point != NULL);

  sample_point->ref_count--;

  if (sample_point->ref_count < 1)
    g_free (sample_point);
}

void
gimp_image_add_sample_point (GimpImage       *gimage,
                             GimpSamplePoint *sample_point,
                             gint             x,
                             gint             y)
{
  g_return_if_fail (GIMP_IS_IMAGE (gimage));
  g_return_if_fail (sample_point != NULL);
  g_return_if_fail (x >= 0);
  g_return_if_fail (y >= 0);
  g_return_if_fail (x < gimage->width);
  g_return_if_fail (y < gimage->height);

  gimage->sample_points = g_list_append (gimage->sample_points, sample_point);

  sample_point->x = x;
  sample_point->y = y;
  gimp_image_sample_point_ref (sample_point);

  gimp_image_sample_point_added (gimage, sample_point);
  gimp_image_update_sample_point (gimage, sample_point);
}

void
gimp_image_remove_sample_point (GimpImage       *gimage,
                                GimpSamplePoint *sample_point,
                                gboolean         push_undo)
{
  GList *list;

  g_return_if_fail (GIMP_IS_IMAGE (gimage));
  g_return_if_fail (sample_point != NULL);

  gimp_image_update_sample_point (gimage, sample_point);

  if (push_undo)
    gimp_image_undo_push_image_sample_point (gimage, _("Remove Sample Point"),
                                             sample_point);

  list = g_list_find (gimage->sample_points, sample_point);
  if (list)
    list = g_list_next (list);

  gimage->sample_points = g_list_remove (gimage->sample_points, sample_point);

  gimp_image_sample_point_removed (gimage, sample_point);

  sample_point->x = -1;
  sample_point->y = -1;
  gimp_image_sample_point_unref (sample_point);

  while (list)
    {
      gimp_image_update_sample_point (gimage, list->data);
      list = g_list_next (list);
    }
}

void
gimp_image_move_sample_point (GimpImage       *gimage,
                              GimpSamplePoint *sample_point,
                              gint             x,
                              gint             y,
                              gboolean         push_undo)
{
  g_return_if_fail (GIMP_IS_IMAGE (gimage));
  g_return_if_fail (sample_point != NULL);
  g_return_if_fail (x >= 0);
  g_return_if_fail (y >= 0);
  g_return_if_fail (x < gimage->width);
  g_return_if_fail (y < gimage->height);

  if (push_undo)
    gimp_image_undo_push_image_sample_point (gimage, _("Move Sample Point"),
                                             sample_point);

  gimp_image_update_sample_point (gimage, sample_point);
  sample_point->x = x;
  sample_point->y = y;
  gimp_image_update_sample_point (gimage, sample_point);
}

GimpSamplePoint *
gimp_image_find_sample_point (GimpImage *gimage,
                              gdouble    x,
                              gdouble    y,
                              gdouble    epsilon_x,
                              gdouble    epsilon_y)
{
  GList           *list;
  GimpSamplePoint *ret     = NULL;
  gdouble          mindist = G_MAXDOUBLE;

  g_return_val_if_fail (GIMP_IS_IMAGE (gimage), NULL);
  g_return_val_if_fail (epsilon_x > 0 && epsilon_y > 0, NULL);

  if (x < 0 || x >= gimage->width ||
      y < 0 || y >= gimage->height)
    {
      return NULL;
    }

  for (list = gimage->sample_points; list; list = g_list_next (list))
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
