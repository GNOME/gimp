/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimppath-compat.c
 * Copyright (C) 2003 Michael Natterer <mitch@gimp.org>
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

#include "config.h"

#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gegl.h>

#include "path-types.h"

#include "core/gimpimage.h"

#include "gimpanchor.h"
#include "gimpbezierstroke.h"
#include "gimppath.h"
#include "gimppath-compat.h"


enum
{
  GIMP_PATH_COMPAT_ANCHOR     = 1,
  GIMP_PATH_COMPAT_CONTROL    = 2,
  GIMP_PATH_COMPAT_NEW_STROKE = 3
};


static const GimpCoords default_coords = GIMP_COORDS_DEFAULT_VALUES;


GimpPath *
gimp_path_compat_new (GimpImage           *image,
                      const gchar         *name,
                      GimpPathCompatPoint *points,
                      gint                 n_points,
                      gboolean             closed)
{
  GimpPath    *path;
  GimpStroke  *stroke;
  GimpCoords  *coords;
  GimpCoords  *curr_stroke;
  GimpCoords  *curr_coord;
  gint         i;

  g_return_val_if_fail (GIMP_IS_IMAGE (image), NULL);
  g_return_val_if_fail (name != NULL, NULL);
  g_return_val_if_fail (points != NULL || n_points == 0, NULL);
  g_return_val_if_fail (n_points >= 0, NULL);

  path = gimp_path_new (image, name);

  coords = g_new0 (GimpCoords, n_points + 1);

  curr_stroke = curr_coord = coords;

  /*  skip the first control point, will set it later  */
  curr_coord++;

  for (i = 0; i < n_points; i++)
    {
      *curr_coord = default_coords;

      curr_coord->x = points[i].x;
      curr_coord->y = points[i].y;

      /*  copy the first anchor to be the first control point  */
      if (curr_coord == curr_stroke + 1)
        *curr_stroke = *curr_coord;

      /*  found new stroke start  */
      if (points[i].type == GIMP_PATH_COMPAT_NEW_STROKE)
        {
          /*  copy the last control point to the beginning of the stroke  */
          *curr_stroke = *(curr_coord - 1);

          stroke =
            gimp_bezier_stroke_new_from_coords (curr_stroke,
                                                curr_coord - curr_stroke - 1,
                                                TRUE);
          gimp_path_stroke_add (path, stroke);
          g_object_unref (stroke);

          /*  start a new stroke  */
          curr_stroke = curr_coord - 1;

          /*  copy the first anchor to be the first control point  */
          *curr_stroke = *curr_coord;
        }

      curr_coord++;
    }

  if (closed)
    {
      /*  copy the last control point to the beginning of the stroke  */
      curr_coord--;
      *curr_stroke = *curr_coord;
    }

  stroke = gimp_bezier_stroke_new_from_coords (curr_stroke,
                                               curr_coord - curr_stroke,
                                               closed);
  gimp_path_stroke_add (path, stroke);
  g_object_unref (stroke);

  g_free (coords);

  return path;
}

gboolean
gimp_path_compat_is_compatible (GimpImage *image)
{
  GList *list;

  g_return_val_if_fail (GIMP_IS_IMAGE (image), FALSE);

  for (list = gimp_image_get_path_iter (image);
       list;
       list = g_list_next (list))
    {
      GimpPath *path       = GIMP_PATH (list->data);
      GList    *strokes;
      gint      open_count = 0;

      if (gimp_item_get_visible (GIMP_ITEM (path)))
        return FALSE;

      for (strokes = path->strokes->head;
           strokes;
           strokes = g_list_next (strokes))
        {
           GimpStroke *stroke = GIMP_STROKE (strokes->data);

          if (! GIMP_IS_BEZIER_STROKE (stroke))
            return FALSE;

          if (!stroke->closed)
            open_count++;
        }

      if (open_count >= 2)
        return FALSE;
    }

  return TRUE;
}

GimpPathCompatPoint *
gimp_path_compat_get_points (GimpPath *path,
                             gint32   *n_points,
                             gint32   *closed)
{
  GimpPathCompatPoint *points;
  GList               *strokes;
  gint                 i;
  GList               *postponed = NULL;  /* for the one open stroke... */
  gint                 open_count;
  gboolean             first_stroke = TRUE;

  g_return_val_if_fail (GIMP_IS_PATH (path), NULL);
  g_return_val_if_fail (n_points != NULL, NULL);
  g_return_val_if_fail (closed != NULL, NULL);

  *n_points = 0;
  *closed   = TRUE;

  open_count = 0;

  for (strokes = path->strokes->head;
       strokes;
       strokes = g_list_next (strokes))
    {
      GimpStroke *stroke = strokes->data;
      gint        n_anchors;

      if (! stroke->closed)
        {
          open_count++;
          postponed = strokes;
          *closed = FALSE;

          if (open_count >= 2)
            {
              g_warning ("gimp_path_compat_get_points(): convert failed");
              *n_points = 0;
              return NULL;
            }
        }

      n_anchors = g_queue_get_length (stroke->anchors);

      if (! stroke->closed)
        n_anchors--;

      *n_points += n_anchors;
    }

  points = g_new0 (GimpPathCompatPoint, *n_points);

  i = 0;

  for (strokes = path->strokes->head;
       strokes || postponed;
       strokes = g_list_next (strokes))
    {
      GimpStroke *stroke;
      GList      *anchors;

      if (strokes)
        {
          if (postponed && strokes == postponed)
            /* we need to visit the open stroke last... */
            continue;
          else
            stroke = GIMP_STROKE (strokes->data);
        }
      else
        {
          stroke = GIMP_STROKE (postponed->data);
          postponed = NULL;
        }

      for (anchors = stroke->anchors->head;
           anchors;
           anchors = g_list_next (anchors))
        {
          GimpAnchor *anchor = anchors->data;

          /*  skip the first anchor, will add it at the end if needed  */
          if (! anchors->prev)
            continue;

          switch (anchor->type)
            {
            case GIMP_ANCHOR_ANCHOR:
              if (anchors->prev == stroke->anchors->head && ! first_stroke)
                points[i].type = GIMP_PATH_COMPAT_NEW_STROKE;
              else
                points[i].type = GIMP_PATH_COMPAT_ANCHOR;
              break;

            case GIMP_ANCHOR_CONTROL:
              points[i].type = GIMP_PATH_COMPAT_CONTROL;
              break;
            }

          points[i].x = anchor->position.x;
          points[i].y = anchor->position.y;

          i++;

          /*  write the skipped control point  */
          if (! anchors->next && stroke->closed)
            {
              anchor = g_queue_peek_head (stroke->anchors);

              points[i].type = GIMP_PATH_COMPAT_CONTROL;
              points[i].x    = anchor->position.x;
              points[i].y    = anchor->position.y;

              i++;
            }
        }
      first_stroke = FALSE;
    }

  return points;
}
