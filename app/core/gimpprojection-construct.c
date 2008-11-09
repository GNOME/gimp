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

#include <gegl.h>

#include "core-types.h"

#include "base/pixel-region.h"
#include "base/tile-manager.h"

#include "paint-funcs/paint-funcs.h"

#include "gimplayer.h"
#include "gimppickable.h"
#include "gimpprojectable.h"
#include "gimpprojection.h"
#include "gimpprojection-construct.h"


/*  local function prototypes  */

static void   gimp_projection_construct_gegl   (GimpProjection *proj,
                                                gint            x,
                                                gint            y,
                                                gint            w,
                                                gint            h);
static void   gimp_projection_construct_legacy (GimpProjection *proj,
                                                gboolean        with_layers,
                                                gint            x,
                                                gint            y,
                                                gint            w,
                                                gint            h);
static void   gimp_projection_initialize       (GimpProjection *proj,
                                                gint            x,
                                                gint            y,
                                                gint            w,
                                                gint            h);


/*  public functions  */

void
gimp_projection_construct (GimpProjection *proj,
                           gint            x,
                           gint            y,
                           gint            w,
                           gint            h)
{
  g_return_if_fail (GIMP_IS_PROJECTION (proj));

#if 0
  GList *layers = gimp_projectable_get_layers (proj->projectable);

  if (layers && ! layers->next) /* a single layer */
    {
      GimpLayer    *layer    = layers->data;
      GimpDrawable *drawable = GIMP_DRAWABLE (layer);
      GimpItem     *item     = GIMP_ITEM (layer);
      gint          width, height;
      gint          off_x, off_y;

      gimp_projectable_get_size (proj->projectable, &width, &height);

      gimp_item_get_offset (item, &off_x, &off_y);

      if (gimp_drawable_has_alpha (drawable)                    &&
          gimp_item_get_visible (item)                          &&
          gimp_item_get_width  (item) == width                  &&
          gimp_item_get_height (item) == height                 &&
          ! gimp_drawable_is_indexed (layer)                    &&
          gimp_layer_get_opacity (layer) == GIMP_OPACITY_OPAQUE &&
          off_x == 0                                            &&
          off_y == 0)
        {
          PixelRegion srcPR, destPR;

          g_printerr ("cow-projection!");

          pixel_region_init (&srcPR,
                             gimp_drawable_get_tiles (layer),
                             x, y, w,h, FALSE);
          pixel_region_init (&destPR,
                             gimp_pickable_get_tiles (GIMP_PICKABLE (proj)),
                             x, y, w,h, TRUE);

          copy_region (&srcPR, &destPR);

          proj->construct_flag = TRUE;

          gimp_projection_construct_legacy (proj, FALSE, x, y, w, h);

          return;
        }
    }
#endif

  /*  First, determine if the projection image needs to be
   *  initialized--this is the case when there are no visible
   *  layers that cover the entire canvas--either because layers
   *  are offset or only a floating selection is visible
   */
  gimp_projection_initialize (proj, x, y, w, h);

  /*  call functions which process the list of layers and
   *  the list of channels
   */
  if (proj->use_gegl)
    {
      gimp_projection_construct_gegl (proj, x, y, w, h);
    }
  else
    {
      proj->construct_flag = FALSE;

      gimp_projection_construct_legacy (proj, TRUE, x, y, w, h);
    }
}


/*  private functions  */

static void
gimp_projection_construct_gegl (GimpProjection *proj,
                                gint            x,
                                gint            y,
                                gint            w,
                                gint            h)
{
  GeglNode      *sink;
  GeglProcessor *processor;
  GeglRectangle  rect;

  sink = gimp_projection_get_sink_node (proj);

  rect.x      = x;
  rect.y      = y;
  rect.width  = w;
  rect.height = h;

  processor = gegl_node_new_processor (sink, &rect);

  while (gegl_processor_work (processor, NULL));

  g_object_unref (processor);
}

static void
gimp_projection_construct_legacy (GimpProjection *proj,
                                  gboolean        with_layers,
                                  gint            x,
                                  gint            y,
                                  gint            w,
                                  gint            h)
{
  GList *list;
  GList *reverse_list = NULL;

  for (list = gimp_projectable_get_channels (proj->projectable);
       list;
       list = g_list_next (list))
    {
      if (gimp_item_get_visible (GIMP_ITEM (list->data)))
        {
          reverse_list = g_list_prepend (reverse_list, list->data);
        }
    }

  if (with_layers)
    {
      for (list = gimp_projectable_get_layers (proj->projectable);
           list;
           list = g_list_next (list))
        {
          GimpLayer *layer = list->data;

          if (! gimp_layer_is_floating_sel (layer) &&
              gimp_item_get_visible (GIMP_ITEM (layer)))
            {
              /*  only add layers that are visible and not floating selections
               *  to the list
               */
              reverse_list = g_list_prepend (reverse_list, layer);
            }
        }
    }

  for (list = reverse_list; list; list = g_list_next (list))
    {
      GimpItem    *item = list->data;
      PixelRegion  projPR;
      gint         x1, y1;
      gint         x2, y2;
      gint         off_x;
      gint         off_y;

      gimp_item_get_offset (item, &off_x, &off_y);

      x1 = CLAMP (off_x,                               x, x + w);
      y1 = CLAMP (off_y,                               y, y + h);
      x2 = CLAMP (off_x + gimp_item_get_width  (item), x, x + w);
      y2 = CLAMP (off_y + gimp_item_get_height (item), y, y + h);

      pixel_region_init (&projPR,
                         gimp_pickable_get_tiles (GIMP_PICKABLE (proj)),
                         x1, y1, x2 - x1, y2 - y1,
                         TRUE);

      gimp_drawable_project_region (GIMP_DRAWABLE (item),
                                    x1 - off_x, y1 - off_y,
                                    x2 - x1,    y2 - y1,
                                    &projPR,
                                    proj->construct_flag);

      proj->construct_flag = TRUE;  /*  something was projected  */
    }

  g_list_free (reverse_list);
}

/**
 * gimp_projection_initialize:
 * @proj: A #GimpProjection.
 * @x:
 * @y:
 * @w:
 * @h:
 *
 * This function determines whether a visible layer with combine mode
 * Normal provides complete coverage over the specified area.  If not,
 * the projection is initialized to transparent black.
 */
static void
gimp_projection_initialize (GimpProjection *proj,
                            gint            x,
                            gint            y,
                            gint            w,
                            gint            h)
{
  GList    *list;
  gboolean  coverage = FALSE;

  for (list = gimp_projectable_get_layers (proj->projectable);
       list;
       list = g_list_next (list))
    {
      GimpLayer    *layer    = list->data;
      GimpDrawable *drawable = GIMP_DRAWABLE (layer);
      GimpItem     *item     = GIMP_ITEM (layer);
      gint          off_x, off_y;

      gimp_item_get_offset (item, &off_x, &off_y);

      if (gimp_item_get_visible (item)                          &&
          ! gimp_drawable_has_alpha (drawable)                  &&
          ! gimp_layer_get_mask (layer)                         &&
          gimp_layer_get_mode (layer) == GIMP_NORMAL_MODE       &&
          gimp_layer_get_opacity (layer) == GIMP_OPACITY_OPAQUE &&
          off_x <= x                                            &&
          off_y <= y                                            &&
          (off_x + gimp_item_get_width  (item)) >= (x + w)      &&
          (off_y + gimp_item_get_height (item)) >= (y + h))
        {
          coverage = TRUE;
          break;
        }
    }

  if (! coverage)
    {
      PixelRegion region;

      pixel_region_init (&region,
                         gimp_pickable_get_tiles (GIMP_PICKABLE (proj)),
                         x, y, w, h, TRUE);
      clear_region (&region);
    }
}
