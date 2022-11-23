/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ligmavectors-compat.c
 * Copyright (C) 2003 Michael Natterer <mitch@ligma.org>
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

#include "vectors-types.h"

#include "core/ligmaimage.h"

#include "ligmaanchor.h"
#include "ligmabezierstroke.h"
#include "ligmavectors.h"
#include "ligmavectors-compat.h"


enum
{
  LIGMA_VECTORS_COMPAT_ANCHOR     = 1,
  LIGMA_VECTORS_COMPAT_CONTROL    = 2,
  LIGMA_VECTORS_COMPAT_NEW_STROKE = 3
};


static const LigmaCoords default_coords = LIGMA_COORDS_DEFAULT_VALUES;


LigmaVectors *
ligma_vectors_compat_new (LigmaImage              *image,
                         const gchar            *name,
                         LigmaVectorsCompatPoint *points,
                         gint                    n_points,
                         gboolean                closed)
{
  LigmaVectors *vectors;
  LigmaStroke  *stroke;
  LigmaCoords  *coords;
  LigmaCoords  *curr_stroke;
  LigmaCoords  *curr_coord;
  gint         i;

  g_return_val_if_fail (LIGMA_IS_IMAGE (image), NULL);
  g_return_val_if_fail (name != NULL, NULL);
  g_return_val_if_fail (points != NULL || n_points == 0, NULL);
  g_return_val_if_fail (n_points >= 0, NULL);

  vectors = ligma_vectors_new (image, name);

  coords = g_new0 (LigmaCoords, n_points + 1);

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
      if (points[i].type == LIGMA_VECTORS_COMPAT_NEW_STROKE)
        {
          /*  copy the last control point to the beginning of the stroke  */
          *curr_stroke = *(curr_coord - 1);

          stroke =
            ligma_bezier_stroke_new_from_coords (curr_stroke,
                                                curr_coord - curr_stroke - 1,
                                                TRUE);
          ligma_vectors_stroke_add (vectors, stroke);
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

  stroke = ligma_bezier_stroke_new_from_coords (curr_stroke,
                                               curr_coord - curr_stroke,
                                               closed);
  ligma_vectors_stroke_add (vectors, stroke);
  g_object_unref (stroke);

  g_free (coords);

  return vectors;
}

gboolean
ligma_vectors_compat_is_compatible (LigmaImage *image)
{
  GList *list;

  g_return_val_if_fail (LIGMA_IS_IMAGE (image), FALSE);

  for (list = ligma_image_get_vectors_iter (image);
       list;
       list = g_list_next (list))
    {
      LigmaVectors *vectors    = LIGMA_VECTORS (list->data);
      GList       *strokes;
      gint         open_count = 0;

      if (ligma_item_get_visible (LIGMA_ITEM (vectors)))
        return FALSE;

      for (strokes = vectors->strokes->head;
           strokes;
           strokes = g_list_next (strokes))
        {
           LigmaStroke *stroke = LIGMA_STROKE (strokes->data);

          if (! LIGMA_IS_BEZIER_STROKE (stroke))
            return FALSE;

          if (!stroke->closed)
            open_count++;
        }

      if (open_count >= 2)
        return FALSE;
    }

  return TRUE;
}

LigmaVectorsCompatPoint *
ligma_vectors_compat_get_points (LigmaVectors *vectors,
                                gint32      *n_points,
                                gint32      *closed)
{
  LigmaVectorsCompatPoint *points;
  GList                  *strokes;
  gint                    i;
  GList                  *postponed = NULL;  /* for the one open stroke... */
  gint                    open_count;
  gboolean                first_stroke = TRUE;

  g_return_val_if_fail (LIGMA_IS_VECTORS (vectors), NULL);
  g_return_val_if_fail (n_points != NULL, NULL);
  g_return_val_if_fail (closed != NULL, NULL);

  *n_points = 0;
  *closed   = TRUE;

  open_count = 0;

  for (strokes = vectors->strokes->head;
       strokes;
       strokes = g_list_next (strokes))
    {
      LigmaStroke *stroke = strokes->data;
      gint        n_anchors;

      if (! stroke->closed)
        {
          open_count++;
          postponed = strokes;
          *closed = FALSE;

          if (open_count >= 2)
            {
              g_warning ("ligma_vectors_compat_get_points(): convert failed");
              *n_points = 0;
              return NULL;
            }
        }

      n_anchors = g_queue_get_length (stroke->anchors);

      if (! stroke->closed)
        n_anchors--;

      *n_points += n_anchors;
    }

  points = g_new0 (LigmaVectorsCompatPoint, *n_points);

  i = 0;

  for (strokes = vectors->strokes->head;
       strokes || postponed;
       strokes = g_list_next (strokes))
    {
      LigmaStroke *stroke;
      GList      *anchors;

      if (strokes)
        {
          if (postponed && strokes == postponed)
            /* we need to visit the open stroke last... */
            continue;
          else
            stroke = LIGMA_STROKE (strokes->data);
        }
      else
        {
          stroke = LIGMA_STROKE (postponed->data);
          postponed = NULL;
        }

      for (anchors = stroke->anchors->head;
           anchors;
           anchors = g_list_next (anchors))
        {
          LigmaAnchor *anchor = anchors->data;

          /*  skip the first anchor, will add it at the end if needed  */
          if (! anchors->prev)
            continue;

          switch (anchor->type)
            {
            case LIGMA_ANCHOR_ANCHOR:
              if (anchors->prev == stroke->anchors->head && ! first_stroke)
                points[i].type = LIGMA_VECTORS_COMPAT_NEW_STROKE;
              else
                points[i].type = LIGMA_VECTORS_COMPAT_ANCHOR;
              break;

            case LIGMA_ANCHOR_CONTROL:
              points[i].type = LIGMA_VECTORS_COMPAT_CONTROL;
              break;
            }

          points[i].x = anchor->position.x;
          points[i].y = anchor->position.y;

          i++;

          /*  write the skipped control point  */
          if (! anchors->next && stroke->closed)
            {
              anchor = g_queue_peek_head (stroke->anchors);

              points[i].type = LIGMA_VECTORS_COMPAT_CONTROL;
              points[i].x    = anchor->position.x;
              points[i].y    = anchor->position.y;

              i++;
            }
        }
      first_stroke = FALSE;
    }

  return points;
}
