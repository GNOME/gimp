/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpcanvasitem-utils.c
 * Copyright (C) 2010 Michael Natterer <mitch@gimp.org>
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

#include "libgimpmath/gimpmath.h"

#include "display-types.h"

#include "core/gimpimage.h"

#include "path/gimpanchor.h"
#include "path/gimpbezierstroke.h"
#include "path/gimppath.h"

#include "gimpcanvasitem.h"
#include "gimpcanvasitem-utils.h"
#include "gimpdisplay.h"
#include "gimpdisplayshell.h"
#include "gimpdisplayshell-transform.h"


gboolean
gimp_canvas_item_on_handle (GimpCanvasItem  *item,
                            gdouble           x,
                            gdouble           y,
                            GimpHandleType    type,
                            gdouble           handle_x,
                            gdouble           handle_y,
                            gint              width,
                            gint              height,
                            GimpHandleAnchor  anchor)
{
  GimpDisplayShell *shell;
  gdouble           tx, ty;
  gdouble           handle_tx, handle_ty;

  g_return_val_if_fail (GIMP_IS_CANVAS_ITEM (item), FALSE);

  shell = gimp_canvas_item_get_shell (item);

  gimp_display_shell_zoom_xy_f (shell,
                                x, y,
                                &tx, &ty);
  gimp_display_shell_zoom_xy_f (shell,
                                handle_x, handle_y,
                                &handle_tx, &handle_ty);

  switch (type)
    {
    case GIMP_HANDLE_SQUARE:
    case GIMP_HANDLE_FILLED_SQUARE:
    case GIMP_HANDLE_CROSS:
    case GIMP_HANDLE_CROSSHAIR:
      gimp_canvas_item_shift_to_north_west (anchor,
                                            handle_tx, handle_ty,
                                            width, height,
                                            &handle_tx, &handle_ty);

      return (tx == CLAMP (tx, handle_tx, handle_tx + width) &&
              ty == CLAMP (ty, handle_ty, handle_ty + height));

    case GIMP_HANDLE_CIRCLE:
    case GIMP_HANDLE_FILLED_CIRCLE:
      gimp_canvas_item_shift_to_center (anchor,
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
gimp_canvas_item_on_path_handle (GimpCanvasItem    *item,
                                 GimpPath          *path,
                                 const GimpCoords  *coord,
                                 gint               width,
                                 gint               height,
                                 GimpAnchorType     preferred,
                                 gboolean           exclusive,
                                 GimpAnchor       **ret_anchor,
                                 GimpStroke       **ret_stroke)
{
  GimpStroke *stroke       = NULL;
  GimpStroke *pref_stroke  = NULL;
  GimpAnchor *anchor       = NULL;
  GimpAnchor *pref_anchor  = NULL;
  gdouble     dx, dy;
  gdouble     pref_mindist = -1;
  gdouble     mindist      = -1;

  g_return_val_if_fail (GIMP_IS_CANVAS_ITEM (item), FALSE);
  g_return_val_if_fail (GIMP_IS_PATH (path), FALSE);
  g_return_val_if_fail (coord != NULL, FALSE);

  if (ret_anchor) *ret_anchor = NULL;
  if (ret_stroke) *ret_stroke = NULL;

  while ((stroke = gimp_path_stroke_get_next (path, stroke)))
    {
      GList *anchor_list;
      GList *list;

      anchor_list = g_list_concat (gimp_stroke_get_draw_anchors (stroke),
                                   gimp_stroke_get_draw_controls (stroke));

      for (list = anchor_list; list; list = g_list_next (list))
        {
          dx = coord->x - GIMP_ANCHOR (list->data)->position.x;
          dy = coord->y - GIMP_ANCHOR (list->data)->position.y;

          if (mindist < 0 || mindist > dx * dx + dy * dy)
            {
              mindist = dx * dx + dy * dy;
              anchor = GIMP_ANCHOR (list->data);

              if (ret_stroke)
                *ret_stroke = stroke;
            }

          if ((pref_mindist < 0 || pref_mindist > dx * dx + dy * dy) &&
              GIMP_ANCHOR (list->data)->type == preferred)
            {
              pref_mindist = dx * dx + dy * dy;
              pref_anchor = GIMP_ANCHOR (list->data);
              pref_stroke = stroke;
            }
        }

      g_list_free (anchor_list);
    }

  /* If the data passed into ret_anchor is a preferred anchor, return it. */
  if (ret_anchor && *ret_anchor &&
      gimp_canvas_item_on_handle (item,
                                  coord->x,
                                  coord->y,
                                  GIMP_HANDLE_CIRCLE,
                                  (*ret_anchor)->position.x,
                                  (*ret_anchor)->position.y,
                                  width, height,
                                  GIMP_HANDLE_ANCHOR_CENTER) &&
      (*ret_anchor)->type == preferred)
    {
      if (ret_stroke) *ret_stroke = pref_stroke;

      return TRUE;
    }

  if (pref_anchor && gimp_canvas_item_on_handle (item,
                                                 coord->x,
                                                 coord->y,
                                                 GIMP_HANDLE_CIRCLE,
                                                 pref_anchor->position.x,
                                                 pref_anchor->position.y,
                                                 width, height,
                                                 GIMP_HANDLE_ANCHOR_CENTER))
    {
      if (ret_anchor) *ret_anchor = pref_anchor;
      if (ret_stroke) *ret_stroke = pref_stroke;

      return TRUE;
    }
  else if (!exclusive && anchor &&
           gimp_canvas_item_on_handle (item,
                                       coord->x,
                                       coord->y,
                                       GIMP_HANDLE_CIRCLE,
                                       anchor->position.x,
                                       anchor->position.y,
                                       width, height,
                                       GIMP_HANDLE_ANCHOR_CENTER))
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
gimp_canvas_item_on_path_curve (GimpCanvasItem    *item,
                                GimpPath          *path,
                                const GimpCoords  *coord,
                                gint               width,
                                gint               height,
                                GimpCoords        *ret_coords,
                                gdouble           *ret_pos,
                                GimpAnchor       **ret_segment_start,
                                GimpAnchor       **ret_segment_end,
                                GimpStroke       **ret_stroke)
{
  GimpStroke *stroke = NULL;
  GimpAnchor *segment_start;
  GimpAnchor *segment_end;
  GimpCoords  min_coords = GIMP_COORDS_DEFAULT_VALUES;
  GimpCoords  cur_coords;
  gdouble     min_dist, cur_dist, cur_pos;

  g_return_val_if_fail (GIMP_IS_CANVAS_ITEM (item), FALSE);
  g_return_val_if_fail (GIMP_IS_PATH (path), FALSE);
  g_return_val_if_fail (coord != NULL, FALSE);

  if (ret_coords)        *ret_coords        = *coord;
  if (ret_pos)           *ret_pos           = -1.0;
  if (ret_segment_start) *ret_segment_start = NULL;
  if (ret_segment_end)   *ret_segment_end   = NULL;
  if (ret_stroke)        *ret_stroke        = NULL;

  min_dist = -1.0;

  while ((stroke = gimp_path_stroke_get_next (path, stroke)))
    {
      cur_dist = gimp_stroke_nearest_point_get (stroke, coord, 1.0,
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
      gimp_canvas_item_on_handle (item,
                                  coord->x,
                                  coord->y,
                                  GIMP_HANDLE_CIRCLE,
                                  min_coords.x,
                                  min_coords.y,
                                  width, height,
                                  GIMP_HANDLE_ANCHOR_CENTER))
    {
      return TRUE;
    }

  return FALSE;
}

gboolean
gimp_canvas_item_on_path (GimpCanvasItem    *item,
                          const GimpCoords  *coords,
                          gint               width,
                          gint               height,
                          GimpCoords        *ret_coords,
                          gdouble           *ret_pos,
                          GimpAnchor       **ret_segment_start,
                          GimpAnchor       **ret_segment_end,
                          GimpStroke       **ret_stroke,
                          GimpPath         **ret_path)
{
  GimpDisplayShell *shell;
  GimpImage        *image;
  GList            *all_path;
  GList            *list;

  g_return_val_if_fail (GIMP_IS_CANVAS_ITEM (item), FALSE);
  g_return_val_if_fail (coords != NULL, FALSE);

  shell = gimp_canvas_item_get_shell (item);
  image = gimp_display_get_image (shell->display);

  if (ret_coords)        *ret_coords         = *coords;
  if (ret_pos)           *ret_pos            = -1.0;
  if (ret_segment_start) *ret_segment_start  = NULL;
  if (ret_segment_end)   *ret_segment_end    = NULL;
  if (ret_stroke)        *ret_stroke         = NULL;
  if (ret_path)          *ret_path           = NULL;

  all_path = gimp_image_get_path_list (image);

  for (list = all_path; list; list = g_list_next (list))
    {
      GimpPath *path = list->data;

      if (! gimp_item_get_visible (GIMP_ITEM (path)))
        continue;

      if (gimp_canvas_item_on_path_curve (item,
                                          path, coords,
                                          width, height,
                                          ret_coords,
                                          ret_pos,
                                          ret_segment_start,
                                          ret_segment_end,
                                          ret_stroke))
        {
          if (ret_path)
            *ret_path = path;

          g_list_free (all_path);

          return TRUE;
        }
    }

  g_list_free (all_path);

  return FALSE;
}

void
gimp_canvas_item_shift_to_north_west (GimpHandleAnchor  anchor,
                                      gdouble           x,
                                      gdouble           y,
                                      gint              width,
                                      gint              height,
                                      gdouble          *shifted_x,
                                      gdouble          *shifted_y)
{
  switch (anchor)
    {
    case GIMP_HANDLE_ANCHOR_CENTER:
      x -= width  / 2;
      y -= height / 2;
      break;

    case GIMP_HANDLE_ANCHOR_NORTH:
      x -= width / 2;
      break;

    case GIMP_HANDLE_ANCHOR_NORTH_WEST:
      /*  nothing, this is the default  */
      break;

    case GIMP_HANDLE_ANCHOR_NORTH_EAST:
      x -= width;
      break;

    case GIMP_HANDLE_ANCHOR_SOUTH:
      x -= width / 2;
      y -= height;
      break;

    case GIMP_HANDLE_ANCHOR_SOUTH_WEST:
      y -= height;
      break;

    case GIMP_HANDLE_ANCHOR_SOUTH_EAST:
      x -= width;
      y -= height;
      break;

    case GIMP_HANDLE_ANCHOR_WEST:
      y -= height / 2;
      break;

    case GIMP_HANDLE_ANCHOR_EAST:
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
gimp_canvas_item_shift_to_center (GimpHandleAnchor  anchor,
                                  gdouble           x,
                                  gdouble           y,
                                  gint              width,
                                  gint              height,
                                  gdouble          *shifted_x,
                                  gdouble          *shifted_y)
{
  switch (anchor)
    {
    case GIMP_HANDLE_ANCHOR_CENTER:
      /*  nothing, this is the default  */
      break;

    case GIMP_HANDLE_ANCHOR_NORTH:
      y += height / 2;
      break;

    case GIMP_HANDLE_ANCHOR_NORTH_WEST:
      x += width  / 2;
      y += height / 2;
      break;

    case GIMP_HANDLE_ANCHOR_NORTH_EAST:
      x -= width  / 2;
      y += height / 2;
      break;

    case GIMP_HANDLE_ANCHOR_SOUTH:
      y -= height / 2;
      break;

    case GIMP_HANDLE_ANCHOR_SOUTH_WEST:
      x += width  / 2;
      y -= height / 2;
      break;

    case GIMP_HANDLE_ANCHOR_SOUTH_EAST:
      x -= width  / 2;
      y -= height / 2;
      break;

    case GIMP_HANDLE_ANCHOR_WEST:
      x += width / 2;
      break;

    case GIMP_HANDLE_ANCHOR_EAST:
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
