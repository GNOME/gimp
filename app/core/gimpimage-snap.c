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
#include "gimpimage-grid.h"
#include "gimpimage-guides.h"
#include "gimpimage-snap.h"

#include "vectors/gimpstroke.h"
#include "vectors/gimpvectors.h"

#include "gimp-intl.h"


/*  public functions  */


gboolean
gimp_image_snap_x (GimpImage *gimage,
                   gdouble    x,
                   gdouble   *tx,
                   gdouble    epsilon_x,
                   gboolean   snap_to_guides,
                   gboolean   snap_to_grid,
                   gboolean   snap_to_canvas)
{
  GList     *list;
  GimpGuide *guide;
  GimpGrid  *grid;
  gdouble    xspacing;
  gdouble    xoffset;
  gdouble    mindist = G_MAXDOUBLE;
  gdouble    dist;
  gdouble    i;
  gboolean   snapped = FALSE;

  g_return_val_if_fail (GIMP_IS_IMAGE (gimage), FALSE);
  g_return_val_if_fail (tx != NULL, FALSE);

  if (! (snap_to_guides || snap_to_grid || snap_to_canvas))
    return FALSE;

  *tx = x;

  if (x < 0 || x >= gimage->width)
    return FALSE;

  if (snap_to_guides && gimage->guides != NULL)
    {
      for (list = gimage->guides; list; list = g_list_next (list))
        {
          guide = (GimpGuide *) list->data;

          if (guide->position < 0)
            continue;

          if (guide->orientation == GIMP_ORIENTATION_VERTICAL)
            {
              dist = ABS (guide->position - x);

              if (dist < MIN (epsilon_x, mindist))
                {
                  mindist = dist;
                  *tx = guide->position;
                  snapped = TRUE;
                }
            }
        }
    }

  if (snap_to_grid && gimage->grid != NULL)
    {
      grid = gimp_image_get_grid (gimage);

      g_object_get (grid,
                    "xspacing", &xspacing,
                    "xoffset",  &xoffset,
                    NULL);

      for (i = xoffset; i <= gimage->width; i += xspacing)
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

      dist = ABS (gimage->width - x);

      if (dist < MIN (epsilon_x, mindist))
        {
          mindist = dist;
          *tx = gimage->width;
          snapped = TRUE;
        }
    }

  return snapped;
}

gboolean
gimp_image_snap_y (GimpImage *gimage,
                   gdouble    y,
                   gdouble   *ty,
                   gdouble    epsilon_y,
                   gboolean   snap_to_guides,
                   gboolean   snap_to_grid,
                   gboolean   snap_to_canvas)
{
  GList     *list;
  GimpGuide *guide;
  GimpGrid  *grid;
  gdouble    yspacing;
  gdouble    yoffset;
  gdouble    mindist = G_MAXDOUBLE;
  gdouble    dist;
  gdouble    i;
  gboolean   snapped = FALSE;

  g_return_val_if_fail (GIMP_IS_IMAGE (gimage), FALSE);
  g_return_val_if_fail (ty != NULL, FALSE);

  if (! (snap_to_guides || snap_to_grid || snap_to_canvas))
    return FALSE;

  *ty = y;

  if (y < 0 || y >= gimage->height)
    return FALSE;

  if (snap_to_guides && gimage->guides != NULL)
    {
      for (list = gimage->guides; list; list = g_list_next (list))
        {
          guide = (GimpGuide *) list->data;

          if (guide->position < 0)
            continue;

          if (guide->orientation == GIMP_ORIENTATION_HORIZONTAL)
            {
              dist = ABS (guide->position - y);

              if (dist < MIN (epsilon_y, mindist))
                {
                  mindist = dist;
                  *ty = guide->position;
                  snapped = TRUE;
                }
            }
        }
    }

  if (snap_to_grid && gimage->grid != NULL)
    {
      grid = gimp_image_get_grid (gimage);

      g_object_get (grid,
                    "yspacing", &yspacing,
                    "yoffset",  &yoffset,
                    NULL);

      for (i = yoffset; i <= gimage->height; i += yspacing)
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

      dist = ABS (gimage->height - y);

      if (dist < MIN (epsilon_y, mindist))
        {
          mindist = dist;
          *ty = gimage->height;
          snapped = TRUE;
        }
    }

  return snapped;
}

gboolean
gimp_image_snap_point (GimpImage *gimage,
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
  GList     *list;
  GimpGuide *guide;
  GimpGrid  *grid;
  gdouble    xspacing, yspacing;
  gdouble    xoffset, yoffset;
  gdouble    minxdist, minydist;
  gdouble    dist;
  gdouble    i;
  gboolean   snapped = FALSE;

  g_return_val_if_fail (GIMP_IS_IMAGE (gimage), FALSE);
  g_return_val_if_fail (tx != NULL, FALSE);
  g_return_val_if_fail (ty != NULL, FALSE);

  if (! (snap_to_guides || snap_to_grid || snap_to_canvas || snap_to_vectors))
    return FALSE;

  *tx = x;
  *ty = y;

  if (x < 0 || x >= gimage->width ||
      y < 0 || y >= gimage->height)
    {
      return FALSE;
    }

  minxdist = G_MAXDOUBLE;
  minydist = G_MAXDOUBLE;

  if (snap_to_guides && gimage->guides != NULL)
    {
      for (list = gimage->guides; list; list = g_list_next (list))
        {
          guide = (GimpGuide *) list->data;

          if (guide->position < 0)
            continue;

          switch (guide->orientation)
            {
            case GIMP_ORIENTATION_HORIZONTAL:
              dist = ABS (guide->position - y);

              if (dist < MIN (epsilon_y, minydist))
                {
                  minydist = dist;
                  *ty = guide->position;
                  snapped = TRUE;
                }
              break;

            case GIMP_ORIENTATION_VERTICAL:
              dist = ABS (guide->position - x);

              if (dist < MIN (epsilon_x, minxdist))
                {
                  minxdist = dist;
                  *tx = guide->position;
                  snapped = TRUE;
                }
              break;

            default:
              break;
            }
        }
    }

  if (snap_to_grid && gimage->grid != NULL)
    {
      grid = gimp_image_get_grid (gimage);

      g_object_get (grid,
                    "xspacing", &xspacing,
                    "yspacing", &yspacing,
                    "xoffset",  &xoffset,
                    "yoffset",  &yoffset,
                    NULL);

      for (i = xoffset; i <= gimage->width; i += xspacing)
        {
          if (i < 0)
            continue;

          dist = ABS (i - x);

          if (dist < MIN (epsilon_x, minxdist))
            {
              minxdist = dist;
              *tx = i;
              snapped = TRUE;
            }
        }

      for (i = yoffset; i <= gimage->height; i += yspacing)
        {
          if (i < 0)
            continue;

          dist = ABS (i - y);

          if (dist < MIN (epsilon_y, minydist))
            {
              minydist = dist;
              *ty = i;
              snapped = TRUE;
            }
        }
    }

  if (snap_to_canvas)
    {
      dist = ABS (x);

      if (dist < MIN (epsilon_x, minxdist))
        {
          minxdist = dist;
          *tx = 0;
          snapped = TRUE;
        }

      dist = ABS (gimage->width - x);

      if (dist < MIN (epsilon_x, minxdist))
        {
          minxdist = dist;
          *tx = gimage->width;
          snapped = TRUE;
        }

      dist = ABS (y);

      if (dist < MIN (epsilon_y, minydist))
        {
          minydist = dist;
          *ty = 0;
          snapped = TRUE;
        }

      dist = ABS (gimage->height - y);

      if (dist < MIN (epsilon_y, minydist))
        {
          minydist = dist;
          *ty = gimage->height;
          snapped = TRUE;
        }
    }

  if (snap_to_vectors && gimage->active_vectors != NULL)
    {
      GimpVectors *vectors = gimp_image_get_active_vectors (gimage);
      GimpStroke  *stroke  = NULL;
      GimpCoords   coords  = { x, y, 0, 0, 0 };

      while ((stroke = gimp_vectors_stroke_get_next (vectors, stroke)))
        {
          GimpCoords nearest;

          if (gimp_stroke_nearest_point_get (stroke, &coords, 1.0,
                                             &nearest,
                                             NULL, NULL, NULL) >= 0)
            {
              dist = ABS (nearest.x - x);

              if (dist < MIN (epsilon_x, minxdist))
                {
                  minxdist = dist;
                  *tx = nearest.x;
                  snapped = TRUE;
                }

              dist = ABS (nearest.y - y);

              if (dist < MIN (epsilon_y, minydist))
                {
                  minydist = dist;
                  *ty = nearest.y;
                  snapped = TRUE;
                }
            }
        }
    }

  return snapped;
}

gboolean
gimp_image_snap_rectangle (GimpImage *gimage,
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
  gdouble  nx1, ny1;
  gdouble  nx2, ny2;
  gboolean snap1, snap2, snap3, snap4;

  g_return_val_if_fail (GIMP_IS_IMAGE (gimage), FALSE);
  g_return_val_if_fail (tx1 != NULL, FALSE);
  g_return_val_if_fail (ty1 != NULL, FALSE);

  if (! (snap_to_guides || snap_to_grid || snap_to_canvas || snap_to_vectors))
    return FALSE;

#ifdef __GNUC__
#warning FIXME: implement rectangle snapping to vectors
#endif

  *tx1 = x1;
  *ty1 = y1;

  snap1 = gimp_image_snap_x (gimage, x1, &nx1,
                             epsilon_x,
                             snap_to_guides,
                             snap_to_grid,
                             snap_to_canvas);
  snap2 = gimp_image_snap_x (gimage, x2, &nx2,
                             epsilon_x,
                             snap_to_guides,
                             snap_to_grid,
                             snap_to_canvas);
  snap3 = gimp_image_snap_y (gimage, y1, &ny1,
                             epsilon_y,
                             snap_to_guides,
                             snap_to_grid,
                             snap_to_canvas);
  snap4 = gimp_image_snap_y (gimage, y2, &ny2,
                             epsilon_y,
                             snap_to_guides,
                             snap_to_grid,
                             snap_to_canvas);

  if (snap1 && snap2)
    {
      if (ABS (x1 - nx1) > ABS (x2 - nx2))
        snap1 = FALSE;
      else
        snap2 = FALSE;
    }

  if (snap1)
    *tx1 = nx1;
  else if (snap2)
    *tx1 = RINT (x1 + (nx2 - x2));

  if (snap3 && snap4)
    {
      if (ABS (y1 - ny1) > ABS (y2 - ny2))
        snap3 = FALSE;
      else
        snap4 = FALSE;
    }

  if (snap3)
    *ty1 = ny1;
  else if (snap4)
    *ty1 = RINT (y1 + (ny2 - y2));

  return (snap1 || snap2 || snap3 || snap4);
}
