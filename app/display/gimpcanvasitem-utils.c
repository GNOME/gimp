/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ligmacanvasitem-utils.c
 * Copyright (C) 2010 Michael Natterer <mitch@ligma.org>
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

#include <gegl.h>
#include <gtk/gtk.h>

#include "libligmamath/ligmamath.h"

#include "display-types.h"

#include "core/ligmaimage.h"

#include "vectors/ligmaanchor.h"
#include "vectors/ligmabezierstroke.h"
#include "vectors/ligmavectors.h"

#include "ligmacanvasitem.h"
#include "ligmacanvasitem-utils.h"
#include "ligmadisplay.h"
#include "ligmadisplayshell.h"
#include "ligmadisplayshell-transform.h"


gboolean
ligma_canvas_item_on_handle (LigmaCanvasItem  *item,
                            gdouble           x,
                            gdouble           y,
                            LigmaHandleType    type,
                            gdouble           handle_x,
                            gdouble           handle_y,
                            gint              width,
                            gint              height,
                            LigmaHandleAnchor  anchor)
{
  LigmaDisplayShell *shell;
  gdouble           tx, ty;
  gdouble           handle_tx, handle_ty;

  g_return_val_if_fail (LIGMA_IS_CANVAS_ITEM (item), FALSE);

  shell = ligma_canvas_item_get_shell (item);

  ligma_display_shell_zoom_xy_f (shell,
                                x, y,
                                &tx, &ty);
  ligma_display_shell_zoom_xy_f (shell,
                                handle_x, handle_y,
                                &handle_tx, &handle_ty);

  switch (type)
    {
    case LIGMA_HANDLE_SQUARE:
    case LIGMA_HANDLE_FILLED_SQUARE:
    case LIGMA_HANDLE_CROSS:
    case LIGMA_HANDLE_CROSSHAIR:
      ligma_canvas_item_shift_to_north_west (anchor,
                                            handle_tx, handle_ty,
                                            width, height,
                                            &handle_tx, &handle_ty);

      return (tx == CLAMP (tx, handle_tx, handle_tx + width) &&
              ty == CLAMP (ty, handle_ty, handle_ty + height));

    case LIGMA_HANDLE_CIRCLE:
    case LIGMA_HANDLE_FILLED_CIRCLE:
      ligma_canvas_item_shift_to_center (anchor,
                                        handle_tx, handle_ty,
                                        width, height,
                                        &handle_tx, &handle_ty);

      /* FIXME */
      if (width != height)
        width = (width + height) / 2;

      width /= 2;

      return ((SQR (handle_tx - tx) + SQR (handle_ty - ty)) < SQR (width));

    default:
      g_warning ("%s: invalid handle type %d", G_STRFUNC, type);
      break;
    }

  return FALSE;
}

gboolean
ligma_canvas_item_on_vectors_handle (LigmaCanvasItem    *item,
                                    LigmaVectors       *vectors,
                                    const LigmaCoords  *coord,
                                    gint               width,
                                    gint               height,
                                    LigmaAnchorType     preferred,
                                    gboolean           exclusive,
                                    LigmaAnchor       **ret_anchor,
                                    LigmaStroke       **ret_stroke)
{
  LigmaStroke *stroke       = NULL;
  LigmaStroke *pref_stroke  = NULL;
  LigmaAnchor *anchor       = NULL;
  LigmaAnchor *pref_anchor  = NULL;
  gdouble     dx, dy;
  gdouble     pref_mindist = -1;
  gdouble     mindist      = -1;

  g_return_val_if_fail (LIGMA_IS_CANVAS_ITEM (item), FALSE);
  g_return_val_if_fail (LIGMA_IS_VECTORS (vectors), FALSE);
  g_return_val_if_fail (coord != NULL, FALSE);

  if (ret_anchor) *ret_anchor = NULL;
  if (ret_stroke) *ret_stroke = NULL;

  while ((stroke = ligma_vectors_stroke_get_next (vectors, stroke)))
    {
      GList *anchor_list;
      GList *list;

      anchor_list = g_list_concat (ligma_stroke_get_draw_anchors (stroke),
                                   ligma_stroke_get_draw_controls (stroke));

      for (list = anchor_list; list; list = g_list_next (list))
        {
          dx = coord->x - LIGMA_ANCHOR (list->data)->position.x;
          dy = coord->y - LIGMA_ANCHOR (list->data)->position.y;

          if (mindist < 0 || mindist > dx * dx + dy * dy)
            {
              mindist = dx * dx + dy * dy;
              anchor = LIGMA_ANCHOR (list->data);

              if (ret_stroke)
                *ret_stroke = stroke;
            }

          if ((pref_mindist < 0 || pref_mindist > dx * dx + dy * dy) &&
              LIGMA_ANCHOR (list->data)->type == preferred)
            {
              pref_mindist = dx * dx + dy * dy;
              pref_anchor = LIGMA_ANCHOR (list->data);
              pref_stroke = stroke;
            }
        }

      g_list_free (anchor_list);
    }

  /* If the data passed into ret_anchor is a preferred anchor, return it. */
  if (ret_anchor && *ret_anchor &&
      ligma_canvas_item_on_handle (item,
                                  coord->x,
                                  coord->y,
                                  LIGMA_HANDLE_CIRCLE,
                                  (*ret_anchor)->position.x,
                                  (*ret_anchor)->position.y,
                                  width, height,
                                  LIGMA_HANDLE_ANCHOR_CENTER) &&
      (*ret_anchor)->type == preferred)
    {
      if (ret_stroke) *ret_stroke = pref_stroke;

      return TRUE;
    }

  if (pref_anchor && ligma_canvas_item_on_handle (item,
                                                 coord->x,
                                                 coord->y,
                                                 LIGMA_HANDLE_CIRCLE,
                                                 pref_anchor->position.x,
                                                 pref_anchor->position.y,
                                                 width, height,
                                                 LIGMA_HANDLE_ANCHOR_CENTER))
    {
      if (ret_anchor) *ret_anchor = pref_anchor;
      if (ret_stroke) *ret_stroke = pref_stroke;

      return TRUE;
    }
  else if (!exclusive && anchor &&
           ligma_canvas_item_on_handle (item,
                                       coord->x,
                                       coord->y,
                                       LIGMA_HANDLE_CIRCLE,
                                       anchor->position.x,
                                       anchor->position.y,
                                       width, height,
                                       LIGMA_HANDLE_ANCHOR_CENTER))
    {
      if (ret_anchor)
        *ret_anchor = anchor;

      /* *ret_stroke already set correctly. */
      return TRUE;
    }

  if (ret_anchor)
    *ret_anchor = NULL;
  if (ret_stroke)
    *ret_stroke = NULL;

  return FALSE;
}

gboolean
ligma_canvas_item_on_vectors_curve (LigmaCanvasItem    *item,
                                   LigmaVectors       *vectors,
                                   const LigmaCoords  *coord,
                                   gint               width,
                                   gint               height,
                                   LigmaCoords        *ret_coords,
                                   gdouble           *ret_pos,
                                   LigmaAnchor       **ret_segment_start,
                                   LigmaAnchor       **ret_segment_end,
                                   LigmaStroke       **ret_stroke)
{
  LigmaStroke *stroke = NULL;
  LigmaAnchor *segment_start;
  LigmaAnchor *segment_end;
  LigmaCoords  min_coords = LIGMA_COORDS_DEFAULT_VALUES;
  LigmaCoords  cur_coords;
  gdouble     min_dist, cur_dist, cur_pos;

  g_return_val_if_fail (LIGMA_IS_CANVAS_ITEM (item), FALSE);
  g_return_val_if_fail (LIGMA_IS_VECTORS (vectors), FALSE);
  g_return_val_if_fail (coord != NULL, FALSE);

  if (ret_coords)        *ret_coords        = *coord;
  if (ret_pos)           *ret_pos           = -1.0;
  if (ret_segment_start) *ret_segment_start = NULL;
  if (ret_segment_end)   *ret_segment_end   = NULL;
  if (ret_stroke)        *ret_stroke        = NULL;

  min_dist = -1.0;

  while ((stroke = ligma_vectors_stroke_get_next (vectors, stroke)))
    {
      cur_dist = ligma_stroke_nearest_point_get (stroke, coord, 1.0,
                                                &cur_coords,
                                                &segment_start,
                                                &segment_end,
                                                &cur_pos);

      if (cur_dist >= 0 && (min_dist < 0 || cur_dist < min_dist))
        {
          min_dist   = cur_dist;
          min_coords = cur_coords;

          if (ret_coords)        *ret_coords        = cur_coords;
          if (ret_pos)           *ret_pos           = cur_pos;
          if (ret_segment_start) *ret_segment_start = segment_start;
          if (ret_segment_end)   *ret_segment_end   = segment_end;
          if (ret_stroke)        *ret_stroke        = stroke;
        }
    }

  if (min_dist >= 0 &&
      ligma_canvas_item_on_handle (item,
                                  coord->x,
                                  coord->y,
                                  LIGMA_HANDLE_CIRCLE,
                                  min_coords.x,
                                  min_coords.y,
                                  width, height,
                                  LIGMA_HANDLE_ANCHOR_CENTER))
    {
      return TRUE;
    }

  return FALSE;
}

gboolean
ligma_canvas_item_on_vectors (LigmaCanvasItem    *item,
                             const LigmaCoords  *coords,
                             gint               width,
                             gint               height,
                             LigmaCoords        *ret_coords,
                             gdouble           *ret_pos,
                             LigmaAnchor       **ret_segment_start,
                             LigmaAnchor       **ret_segment_end,
                             LigmaStroke       **ret_stroke,
                             LigmaVectors      **ret_vectors)
{
  LigmaDisplayShell *shell;
  LigmaImage        *image;
  GList            *all_vectors;
  GList            *list;

  g_return_val_if_fail (LIGMA_IS_CANVAS_ITEM (item), FALSE);
  g_return_val_if_fail (coords != NULL, FALSE);

  shell = ligma_canvas_item_get_shell (item);
  image = ligma_display_get_image (shell->display);

  if (ret_coords)        *ret_coords         = *coords;
  if (ret_pos)           *ret_pos            = -1.0;
  if (ret_segment_start) *ret_segment_start  = NULL;
  if (ret_segment_end)   *ret_segment_end    = NULL;
  if (ret_stroke)        *ret_stroke         = NULL;
  if (ret_vectors)       *ret_vectors        = NULL;

  all_vectors = ligma_image_get_vectors_list (image);

  for (list = all_vectors; list; list = g_list_next (list))
    {
      LigmaVectors *vectors = list->data;

      if (! ligma_item_get_visible (LIGMA_ITEM (vectors)))
        continue;

      if (ligma_canvas_item_on_vectors_curve (item,
                                             vectors, coords,
                                             width, height,
                                             ret_coords,
                                             ret_pos,
                                             ret_segment_start,
                                             ret_segment_end,
                                             ret_stroke))
        {
          if (ret_vectors)
            *ret_vectors = vectors;

          g_list_free (all_vectors);

          return TRUE;
        }
    }

  g_list_free (all_vectors);

  return FALSE;
}

void
ligma_canvas_item_shift_to_north_west (LigmaHandleAnchor  anchor,
                                      gdouble           x,
                                      gdouble           y,
                                      gint              width,
                                      gint              height,
                                      gdouble          *shifted_x,
                                      gdouble          *shifted_y)
{
  switch (anchor)
    {
    case LIGMA_HANDLE_ANCHOR_CENTER:
      x -= width  / 2;
      y -= height / 2;
      break;

    case LIGMA_HANDLE_ANCHOR_NORTH:
      x -= width / 2;
      break;

    case LIGMA_HANDLE_ANCHOR_NORTH_WEST:
      /*  nothing, this is the default  */
      break;

    case LIGMA_HANDLE_ANCHOR_NORTH_EAST:
      x -= width;
      break;

    case LIGMA_HANDLE_ANCHOR_SOUTH:
      x -= width / 2;
      y -= height;
      break;

    case LIGMA_HANDLE_ANCHOR_SOUTH_WEST:
      y -= height;
      break;

    case LIGMA_HANDLE_ANCHOR_SOUTH_EAST:
      x -= width;
      y -= height;
      break;

    case LIGMA_HANDLE_ANCHOR_WEST:
      y -= height / 2;
      break;

    case LIGMA_HANDLE_ANCHOR_EAST:
      x -= width;
      y -= height / 2;
      break;

    default:
      break;
    }

  if (shifted_x)
    *shifted_x = x;

  if (shifted_y)
    *shifted_y = y;
}

void
ligma_canvas_item_shift_to_center (LigmaHandleAnchor  anchor,
                                  gdouble           x,
                                  gdouble           y,
                                  gint              width,
                                  gint              height,
                                  gdouble          *shifted_x,
                                  gdouble          *shifted_y)
{
  switch (anchor)
    {
    case LIGMA_HANDLE_ANCHOR_CENTER:
      /*  nothing, this is the default  */
      break;

    case LIGMA_HANDLE_ANCHOR_NORTH:
      y += height / 2;
      break;

    case LIGMA_HANDLE_ANCHOR_NORTH_WEST:
      x += width  / 2;
      y += height / 2;
      break;

    case LIGMA_HANDLE_ANCHOR_NORTH_EAST:
      x -= width  / 2;
      y += height / 2;
      break;

    case LIGMA_HANDLE_ANCHOR_SOUTH:
      y -= height / 2;
      break;

    case LIGMA_HANDLE_ANCHOR_SOUTH_WEST:
      x += width  / 2;
      y -= height / 2;
      break;

    case LIGMA_HANDLE_ANCHOR_SOUTH_EAST:
      x -= width  / 2;
      y -= height / 2;
      break;

    case LIGMA_HANDLE_ANCHOR_WEST:
      x += width / 2;
      break;

    case LIGMA_HANDLE_ANCHOR_EAST:
      x -= width / 2;
      break;

    default:
      break;
    }

  if (shifted_x)
    *shifted_x = x;

  if (shifted_y)
    *shifted_y = y;
}
