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
#include "gimpguide.h"
#include "gimpimage.h"
#include "gimpimage-grid.h"
#include "gimpimage-guides.h"
#include "gimpimage-snap.h"

#include "vectors/gimpstroke.h"
#include "vectors/gimpvectors.h"

#include "gimp-intl.h"


/*  public functions  */


gboolean
gimp_image_snap_x (GimpImage *image,
                   gdouble    x,
                   gdouble   *tx,
                   gdouble    epsilon_x,
                   gboolean   snap_to_guides,
                   gboolean   snap_to_grid,
                   gboolean   snap_to_canvas)
{
  gdouble    mindist = G_MAXDOUBLE;
  gdouble    dist;
  gboolean   snapped = FALSE;

  g_return_val_if_fail (GIMP_IS_IMAGE (image), FALSE);
  g_return_val_if_fail (tx != NULL, FALSE);

  *tx = x;

  if (! image->guides) snap_to_guides = FALSE;
  if (! image->grid)   snap_to_grid   = FALSE;

  if (! (snap_to_guides || snap_to_grid || snap_to_canvas))
    return FALSE;

  if (x < -epsilon_x || x >= (image->width + epsilon_x))
    return FALSE;

  if (snap_to_guides)
    {
      GList *list;

      for (list = image->guides; list; list = g_list_next (list))
        {
          GimpGuide *guide    = list->data;
          gint       position = gimp_guide_get_position (guide);

          if (position < 0)
            continue;

          if (gimp_guide_get_orientation (guide) == GIMP_ORIENTATION_VERTICAL)
            {
              dist = ABS (position - x);

              if (dist < MIN (epsilon_x, mindist))
                {
                  mindist = dist;
                  *tx = position;
                  snapped = TRUE;
                }
            }
        }
    }

  if (snap_to_grid)
    {
      GimpGrid *grid = gimp_image_get_grid (image);
      gdouble   xspacing;
      gdouble   xoffset;
      gdouble   i;

      g_object_get (grid,
                    "xspacing", &xspacing,
                    "xoffset",  &xoffset,
                    NULL);

      while (xoffset > xspacing)
        xoffset -= xspacing;

      for (i = xoffset; i <= image->width; i += xspacing)
        {
          if (i < 0)
            continue;

          dist = ABS (i - x);

          if (dist < MIN (epsilon_x, mindist))
            {
              mindist = dist;
              *tx = i;
              snapped = TRUE;
            }
        }
    }

  if (snap_to_canvas)
    {
      dist = ABS (x);

      if (dist < MIN (epsilon_x, mindist))
        {
          mindist = dist;
          *tx = 0;
          snapped = TRUE;
        }

      dist = ABS (image->width - x);

      if (dist < MIN (epsilon_x, mindist))
        {
          mindist = dist;
          *tx = image->width;
          snapped = TRUE;
        }
    }

  return snapped;
}

gboolean
gimp_image_snap_y (GimpImage *image,
                   gdouble    y,
                   gdouble   *ty,
                   gdouble    epsilon_y,
                   gboolean   snap_to_guides,
                   gboolean   snap_to_grid,
                   gboolean   snap_to_canvas)
{
  gdouble    mindist = G_MAXDOUBLE;
  gdouble    dist;
  gboolean   snapped = FALSE;

  g_return_val_if_fail (GIMP_IS_IMAGE (image), FALSE);
  g_return_val_if_fail (ty != NULL, FALSE);

  *ty = y;

  if (! image->guides) snap_to_guides = FALSE;
  if (! image->grid)   snap_to_grid   = FALSE;

  if (! (snap_to_guides || snap_to_grid || snap_to_canvas))
    return FALSE;

  if (y < -epsilon_y || y >= (image->height + epsilon_y))
    return FALSE;

  if (snap_to_guides)
    {
      GList *list;

      for (list = image->guides; list; list = g_list_next (list))
        {
          GimpGuide *guide    = list->data;
          gint       position = gimp_guide_get_position (guide);

          if (position < 0)
            continue;

          if (gimp_guide_get_orientation (guide) == GIMP_ORIENTATION_HORIZONTAL)
            {
              dist = ABS (position - y);

              if (dist < MIN (epsilon_y, mindist))
                {
                  mindist = dist;
                  *ty = position;
                  snapped = TRUE;
                }
            }
        }
    }

  if (snap_to_grid)
    {
      GimpGrid *grid = gimp_image_get_grid (image);
      gdouble    yspacing;
      gdouble    yoffset;
      gdouble    i;

      g_object_get (grid,
                    "yspacing", &yspacing,
                    "yoffset",  &yoffset,
                    NULL);

      while (yoffset > yspacing)
        yoffset -= yspacing;

      for (i = yoffset; i <= image->height; i += yspacing)
        {
          if (i < 0)
            continue;

          dist = ABS (i - y);

          if (dist < MIN (epsilon_y, mindist))
            {
              mindist = dist;
              *ty = i;
              snapped = TRUE;
            }
        }
    }

  if (snap_to_canvas)
    {
      dist = ABS (y);

      if (dist < MIN (epsilon_y, mindist))
        {
          mindist = dist;
          *ty = 0;
          snapped = TRUE;
        }

      dist = ABS (image->height - y);

      if (dist < MIN (epsilon_y, mindist))
        {
          mindist = dist;
          *ty = image->height;
          snapped = TRUE;
        }
    }

  return snapped;
}

gboolean
gimp_image_snap_point (GimpImage *image,
                       gdouble    x,
                       gdouble    y,
                       gdouble   *tx,
                       gdouble   *ty,
                       gdouble    epsilon_x,
                       gdouble    epsilon_y,
                       gboolean   snap_to_guides,
                       gboolean   snap_to_grid,
                       gboolean   snap_to_canvas,
                       gboolean   snap_to_vectors)
{
  gdouble  mindist_x = G_MAXDOUBLE;
  gdouble  mindist_y = G_MAXDOUBLE;
  gdouble  dist;
  gboolean snapped   = FALSE;

  g_return_val_if_fail (GIMP_IS_IMAGE (image), FALSE);
  g_return_val_if_fail (tx != NULL, FALSE);
  g_return_val_if_fail (ty != NULL, FALSE);

  *tx = x;
  *ty = y;

  if (! image->guides)         snap_to_guides  = FALSE;
  if (! image->grid)           snap_to_grid    = FALSE;
  if (! image->active_vectors) snap_to_vectors = FALSE;

  if (! (snap_to_guides || snap_to_grid || snap_to_canvas || snap_to_vectors))
    return FALSE;

  if (x < -epsilon_x || x >= (image->width  + epsilon_x) ||
      y < -epsilon_y || y >= (image->height + epsilon_y))
    {
      return FALSE;
    }

  if (snap_to_guides)
    {
      GList *list;

      for (list = image->guides; list; list = g_list_next (list))
        {
          GimpGuide *guide    = list->data;
          gint       position = gimp_guide_get_position (guide);

          if (position < 0)
            continue;

          switch (gimp_guide_get_orientation (guide))
            {
            case GIMP_ORIENTATION_HORIZONTAL:
              dist = ABS (position - y);

              if (dist < MIN (epsilon_y, mindist_y))
                {
                  mindist_y = dist;
                  *ty = position;
                  snapped = TRUE;
                }
              break;

            case GIMP_ORIENTATION_VERTICAL:
              dist = ABS (position - x);

              if (dist < MIN (epsilon_x, mindist_x))
                {
                  mindist_x = dist;
                  *tx = position;
                  snapped = TRUE;
                }
              break;

            default:
              break;
            }
        }
    }

  if (snap_to_grid)
    {
      GimpGrid *grid = gimp_image_get_grid (image);
      gdouble   xspacing, yspacing;
      gdouble   xoffset, yoffset;
      gdouble   i;

      g_object_get (grid,
                    "xspacing", &xspacing,
                    "yspacing", &yspacing,
                    "xoffset",  &xoffset,
                    "yoffset",  &yoffset,
                    NULL);

      while (xoffset > xspacing)
        xoffset -= xspacing;

      while (yoffset > yspacing)
        yoffset -= yspacing;

      for (i = xoffset; i <= image->width; i += xspacing)
        {
          if (i < 0)
            continue;

          dist = ABS (i - x);

          if (dist < MIN (epsilon_x, mindist_x))
            {
              mindist_x = dist;
              *tx = i;
              snapped = TRUE;
            }
        }

      for (i = yoffset; i <= image->height; i += yspacing)
        {
          if (i < 0)
            continue;

          dist = ABS (i - y);

          if (dist < MIN (epsilon_y, mindist_y))
            {
              mindist_y = dist;
              *ty = i;
              snapped = TRUE;
            }
        }
    }

  if (snap_to_canvas)
    {
      dist = ABS (x);

      if (dist < MIN (epsilon_x, mindist_x))
        {
          mindist_x = dist;
          *tx = 0;
          snapped = TRUE;
        }

      dist = ABS (image->width - x);

      if (dist < MIN (epsilon_x, mindist_x))
        {
          mindist_x = dist;
          *tx = image->width;
          snapped = TRUE;
        }

      dist = ABS (y);

      if (dist < MIN (epsilon_y, mindist_y))
        {
          mindist_y = dist;
          *ty = 0;
          snapped = TRUE;
        }

      dist = ABS (image->height - y);

      if (dist < MIN (epsilon_y, mindist_y))
        {
          mindist_y = dist;
          *ty = image->height;
          snapped = TRUE;
        }
    }

  if (snap_to_vectors)
    {
      GimpVectors *vectors = gimp_image_get_active_vectors (image);
      GimpStroke  *stroke  = NULL;
      GimpCoords   coords  = { 0, 0, 0, 0, 0 };

      coords.x = x;
      coords.y = y;

      while ((stroke = gimp_vectors_stroke_get_next (vectors, stroke)))
        {
          GimpCoords nearest;

          if (gimp_stroke_nearest_point_get (stroke, &coords, 1.0,
                                             &nearest,
                                             NULL, NULL, NULL) >= 0)
            {
              dist = ABS (nearest.x - x);

              if (dist < MIN (epsilon_x, mindist_x))
                {
                  mindist_x = dist;
                  *tx = nearest.x;
                  snapped = TRUE;
                }

              dist = ABS (nearest.y - y);

              if (dist < MIN (epsilon_y, mindist_y))
                {
                  mindist_y = dist;
                  *ty = nearest.y;
                  snapped = TRUE;
                }
            }
        }
    }

  return snapped;
}

gboolean
gimp_image_snap_rectangle (GimpImage *image,
                           gdouble    x1,
                           gdouble    y1,
                           gdouble    x2,
                           gdouble    y2,
                           gdouble   *tx1,
                           gdouble   *ty1,
                           gdouble    epsilon_x,
                           gdouble    epsilon_y,
                           gboolean   snap_to_guides,
                           gboolean   snap_to_grid,
                           gboolean   snap_to_canvas,
                           gboolean   snap_to_vectors)
{
  gdouble  nx, ny;
  gdouble  mindist_x = G_MAXDOUBLE;
  gdouble  mindist_y = G_MAXDOUBLE;
  gboolean snapped   = FALSE;

  g_return_val_if_fail (GIMP_IS_IMAGE (image), FALSE);
  g_return_val_if_fail (tx1 != NULL, FALSE);
  g_return_val_if_fail (ty1 != NULL, FALSE);

  *tx1 = x1;
  *ty1 = y1;

  if (! image->guides)         snap_to_guides  = FALSE;
  if (! image->grid)           snap_to_grid    = FALSE;
  if (! image->active_vectors) snap_to_vectors = FALSE;

  if (! (snap_to_guides || snap_to_grid || snap_to_canvas || snap_to_vectors))
    return FALSE;

  /*  left edge  */
  if (gimp_image_snap_x (image, x1, &nx,
                         MIN (epsilon_x, mindist_x),
                         snap_to_guides,
                         snap_to_grid,
                         snap_to_canvas))
    {
      mindist_x = ABS (nx - x1);
      *tx1 = nx;
      snapped = TRUE;
    }

  /*  right edge  */
  if (gimp_image_snap_x (image, x2, &nx,
                         MIN (epsilon_x, mindist_x),
                         snap_to_guides,
                         snap_to_grid,
                         snap_to_canvas))
    {
      mindist_x = ABS (nx - x2);
      *tx1 = RINT (x1 + (nx - x2));
      snapped = TRUE;
    }

  /*  top edge  */
  if (gimp_image_snap_y (image, y1, &ny,
                         MIN (epsilon_y, mindist_y),
                         snap_to_guides,
                         snap_to_grid,
                         snap_to_canvas))
    {
      mindist_y = ABS (ny - y1);
      *ty1 = ny;
      snapped = TRUE;
    }

  /*  bottom edge  */
  if (gimp_image_snap_y (image, y2, &ny,
                         MIN (epsilon_y, mindist_y),
                         snap_to_guides,
                         snap_to_grid,
                         snap_to_canvas))
    {
      mindist_y = ABS (ny - y2);
      *ty1 = RINT (y1 + (ny - y2));
      snapped = TRUE;
    }

  if (snap_to_vectors)
    {
      GimpVectors *vectors = gimp_image_get_active_vectors (image);
      GimpStroke  *stroke  = NULL;
      GimpCoords   coords1 = GIMP_COORDS_DEFAULT_VALUES;
      GimpCoords   coords2 = GIMP_COORDS_DEFAULT_VALUES;

      while ((stroke = gimp_vectors_stroke_get_next (vectors, stroke)))
        {
          GimpCoords nearest;
          gdouble    dist;

          /*  top edge  */

          coords1.x = x1;
          coords1.y = y1;
          coords2.x = x2;
          coords2.y = y1;

          if (gimp_stroke_nearest_tangent_get (stroke, &coords1, &coords2,
                                               1.0, &nearest,
                                               NULL, NULL, NULL) >= 0)
            {
              dist = ABS (nearest.y - y1);

              if (dist < MIN (epsilon_y, mindist_y))
                {
                  mindist_y = dist;
                  *ty1 = nearest.y;
                  snapped = TRUE;
                }
            }

          if (gimp_stroke_nearest_intersection_get (stroke, &coords1, &coords2,
                                                    1.0, &nearest,
                                                    NULL, NULL, NULL) >= 0)
            {
              dist = ABS (nearest.x - x1);

              if (dist < MIN (epsilon_x, mindist_x))
                {
                  mindist_x = dist;
                  *tx1 = nearest.x;
                  snapped = TRUE;
                }
            }

          if (gimp_stroke_nearest_intersection_get (stroke, &coords2, &coords1,
                                                    1.0, &nearest,
                                                    NULL, NULL, NULL) >= 0)
            {
              dist = ABS (nearest.x - x2);

              if (dist < MIN (epsilon_x, mindist_x))
                {
                  mindist_x = dist;
                  *tx1 = RINT (x1 + (nearest.x - x2));
                  snapped = TRUE;
                }
            }

          /*  bottom edge  */

          coords1.x = x1;
          coords1.y = y2;
          coords2.x = x2;
          coords2.y = y2;

          if (gimp_stroke_nearest_tangent_get (stroke, &coords1, &coords2,
                                               1.0, &nearest,
                                               NULL, NULL, NULL) >= 0)
            {
              dist = ABS (nearest.y - y2);

              if (dist < MIN (epsilon_y, mindist_y))
                {
                  mindist_y = dist;
                  *ty1 = RINT (y1 + (nearest.y - y2));
                  snapped = TRUE;
                }
            }

          if (gimp_stroke_nearest_intersection_get (stroke, &coords1, &coords2,
                                                    1.0, &nearest,
                                                    NULL, NULL, NULL) >= 0)
            {
              dist = ABS (nearest.x - x1);

              if (dist < MIN (epsilon_x, mindist_x))
                {
                  mindist_x = dist;
                  *tx1 = nearest.x;
                  snapped = TRUE;
                }
            }

          if (gimp_stroke_nearest_intersection_get (stroke, &coords2, &coords1,
                                                    1.0, &nearest,
                                                    NULL, NULL, NULL) >= 0)
            {
              dist = ABS (nearest.x - x2);

              if (dist < MIN (epsilon_x, mindist_x))
                {
                  mindist_x = dist;
                  *tx1 = RINT (x1 + (nearest.x - x2));
                  snapped = TRUE;
                }
            }

          /*  left edge  */

          coords1.x = x1;
          coords1.y = y1;
          coords2.x = x1;
          coords2.y = y2;

          if (gimp_stroke_nearest_tangent_get (stroke, &coords1, &coords2,
                                               1.0, &nearest,
                                               NULL, NULL, NULL) >= 0)
            {
              dist = ABS (nearest.x - x1);

              if (dist < MIN (epsilon_x, mindist_x))
                {
                  mindist_x = dist;
                  *tx1 = nearest.x;
                  snapped = TRUE;
                }
            }

          if (gimp_stroke_nearest_intersection_get (stroke, &coords1, &coords2,
                                                    1.0, &nearest,
                                                    NULL, NULL, NULL) >= 0)
            {
              dist = ABS (nearest.y - y1);

              if (dist < MIN (epsilon_y, mindist_y))
                {
                  mindist_y = dist;
                  *ty1 = nearest.y;
                  snapped = TRUE;
                }
            }

          if (gimp_stroke_nearest_intersection_get (stroke, &coords2, &coords1,
                                                    1.0, &nearest,
                                                    NULL, NULL, NULL) >= 0)
            {
              dist = ABS (nearest.y - y2);

              if (dist < MIN (epsilon_y, mindist_y))
                {
                  mindist_y = dist;
                  *ty1 = RINT (y1 + (nearest.y - y2));
                  snapped = TRUE;
                }
            }

          /*  right edge  */

          coords1.x = x2;
          coords1.y = y1;
          coords2.x = x2;
          coords2.y = y2;

          if (gimp_stroke_nearest_tangent_get (stroke, &coords1, &coords2,
                                               1.0, &nearest,
                                               NULL, NULL, NULL) >= 0)
            {
              dist = ABS (nearest.x - x2);

              if (dist < MIN (epsilon_x, mindist_x))
                {
                  mindist_x = dist;
                  *tx1 = RINT (x1 + (nearest.x - x2));
                  snapped = TRUE;
                }
            }

          if (gimp_stroke_nearest_intersection_get (stroke, &coords1, &coords2,
                                                    1.0, &nearest,
                                                    NULL, NULL, NULL) >= 0)
            {
              dist = ABS (nearest.y - y1);

              if (dist < MIN (epsilon_y, mindist_y))
                {
                  mindist_y = dist;
                  *ty1 = nearest.y;
                  snapped = TRUE;
                }
            }

          if (gimp_stroke_nearest_intersection_get (stroke, &coords2, &coords1,
                                                    1.0, &nearest,
                                                    NULL, NULL, NULL) >= 0)
            {
              dist = ABS (nearest.y - y2);

              if (dist < MIN (epsilon_y, mindist_y))
                {
                  mindist_y = dist;
                  *ty1 = RINT (y1 + (nearest.y - y2));
                  snapped = TRUE;
                }
            }
        }
    }

  return snapped;
}
