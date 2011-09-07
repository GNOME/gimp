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

/* This file contains the code necessary for generating on canvas
 * previews, either by connecting a function to process the pixels or
 * by connecting a specified GEGL operation to do the processing. It
 * keeps an undo buffer to allow direct modification of the pixel data
 * (so that it will show up in the projection) and it will restore the
 * source in case the mapping procedure was cancelled.
 *
 * To create a tool that uses this, see /tools/gimpimagemaptool.c for
 * the interface and /tools/gimpcolorbalancetool.c for an example of
 * using that interface.
 *
 * Note that when talking about on canvas preview, we are speaking
 * about non destructive image editing where the operation is previewd
 * before being applied.
 */

#include "config.h"

#include <glib-object.h>
#include <gegl.h>

#include "core-types.h"

#include "base/pixel-region.h"
#include "base/tile-manager.h"
#include "base/tile.h"

#include "paint-funcs/paint-funcs.h"

#include "gimpdrawable.h"
#include "gimpdrawable-shadow.h"
#include "gimpimage.h"
#include "gimpimagemap.h"
#include "gimpmarshal.h"
#include "gimppickable.h"
#include "gimpviewable.h"


enum
{
  FLUSH,
  LAST_SIGNAL
};


struct _GimpImageMap
{
  GimpObject             parent_instance;

  GimpDrawable          *drawable;
  gchar                 *undo_desc;

  TileManager           *undo_tiles;
  gint                   undo_offset_x;
  gint                   undo_offset_y;

  GimpImageMapApplyFunc  apply_func;
  gpointer               apply_data;
  PixelRegion            srcPR;
  PixelRegion            destPR;
  PixelRegionIterator   *PRI;

  GeglNode              *gegl;
  GeglNode              *input;
  GeglNode              *translate;
  GeglNode              *operation;
  GeglNode              *output;
  GeglProcessor         *processor;

  guint                  idle_id;

  GTimer                *timer;
  guint64                pixel_count;
};


static void   gimp_image_map_pickable_iface_init (GimpPickableInterface *iface);

static void            gimp_image_map_dispose        (GObject             *object);
static void            gimp_image_map_finalize       (GObject             *object);

static GimpImage     * gimp_image_map_get_image      (GimpPickable        *pickable);
static GimpImageType   gimp_image_map_get_image_type (GimpPickable        *pickable);
static gint            gimp_image_map_get_bytes      (GimpPickable        *pickable);
static TileManager   * gimp_image_map_get_tiles      (GimpPickable        *pickable);
static gboolean        gimp_image_map_get_pixel_at   (GimpPickable        *pickable,
                                                      gint                 x,
                                                      gint                 y,
                                                      guchar              *pixel);

static void            gimp_image_map_update_undo_tiles
                                                     (GimpImageMap        *image_map,
                                                      const GeglRectangle *rect);
static gboolean        gimp_image_map_do             (GimpImageMap        *image_map);
static void            gimp_image_map_data_written   (GObject             *operation,
                                                      const GeglRectangle *extent,
                                                      GimpImageMap        *image_map);
static void            gimp_image_map_cancel_any_idle_jobs
                                                     (GimpImageMap        *image_map);
static void            gimp_image_map_kill_any_idle_processors
                                                     (GimpImageMap        *image_map);


G_DEFINE_TYPE_WITH_CODE (GimpImageMap, gimp_image_map, GIMP_TYPE_OBJECT,
                         G_IMPLEMENT_INTERFACE (GIMP_TYPE_PICKABLE,
                                                gimp_image_map_pickable_iface_init))

#define parent_class gimp_image_map_parent_class

static guint image_map_signals[LAST_SIGNAL] = { 0 };


static void
gimp_image_map_class_init (GimpImageMapClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  image_map_signals[FLUSH] =
    g_signal_new ("flush",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (GimpImageMapClass, flush),
                  NULL, NULL,
                  gimp_marshal_VOID__VOID,
                  G_TYPE_NONE, 0);

  object_class->dispose  = gimp_image_map_dispose;
  object_class->finalize = gimp_image_map_finalize;
}

static void
gimp_image_map_pickable_iface_init (GimpPickableInterface *iface)
{
  iface->get_image      = gimp_image_map_get_image;
  iface->get_image_type = gimp_image_map_get_image_type;
  iface->get_bytes      = gimp_image_map_get_bytes;
  iface->get_tiles      = gimp_image_map_get_tiles;
  iface->get_pixel_at   = gimp_image_map_get_pixel_at;
}

static void
gimp_image_map_init (GimpImageMap *image_map)
{
  image_map->drawable      = NULL;
  image_map->undo_desc     = NULL;
  image_map->undo_tiles    = NULL;
  image_map->undo_offset_x = 0;
  image_map->undo_offset_y = 0;
  image_map->apply_func    = NULL;
  image_map->apply_data    = NULL;
  image_map->PRI           = NULL;
  image_map->idle_id       = 0;

#ifdef GIMP_UNSTABLE
  image_map->timer         = g_timer_new ();
#else
  image_map->timer         = NULL;
#endif

  image_map->pixel_count   = 0;

  if (image_map->timer)
    g_timer_stop (image_map->timer);
}

static void
gimp_image_map_dispose (GObject *object)
{
  GimpImageMap *image_map = GIMP_IMAGE_MAP (object);

  if (image_map->drawable)
    gimp_viewable_preview_thaw (GIMP_VIEWABLE (image_map->drawable));

  G_OBJECT_CLASS (parent_class)->dispose (object);
}

static void
gimp_image_map_finalize (GObject *object)
{
  GimpImageMap *image_map = GIMP_IMAGE_MAP (object);

  if (image_map->undo_desc)
    {
      g_free (image_map->undo_desc);
      image_map->undo_desc = NULL;
    }

  if (image_map->undo_tiles)
    {
      tile_manager_unref (image_map->undo_tiles);
      image_map->undo_tiles = NULL;
    }

  gimp_image_map_cancel_any_idle_jobs (image_map);

  if (image_map->gegl)
    {
      g_object_unref (image_map->gegl);
      image_map->gegl      = NULL;
      image_map->input     = NULL;
      image_map->translate = NULL;
      image_map->output    = NULL;
    }

  if (image_map->operation)
    {
      g_object_unref (image_map->operation);
      image_map->operation = NULL;
    }

  if (image_map->drawable)
    {
      gimp_drawable_free_shadow_tiles (image_map->drawable);

      g_object_unref (image_map->drawable);
      image_map->drawable = NULL;
    }

  if (image_map->timer)
    {
      g_timer_destroy (image_map->timer);
      image_map->timer = NULL;
    }

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static GimpImage *
gimp_image_map_get_image (GimpPickable *pickable)
{
  GimpImageMap *image_map = GIMP_IMAGE_MAP (pickable);

  return gimp_pickable_get_image (GIMP_PICKABLE (image_map->drawable));
}

static GimpImageType
gimp_image_map_get_image_type (GimpPickable *pickable)
{
  GimpImageMap *image_map = GIMP_IMAGE_MAP (pickable);

  return gimp_pickable_get_image_type (GIMP_PICKABLE (image_map->drawable));
}

static gint
gimp_image_map_get_bytes (GimpPickable *pickable)
{
  GimpImageMap *image_map = GIMP_IMAGE_MAP (pickable);

  return gimp_pickable_get_bytes (GIMP_PICKABLE (image_map->drawable));
}

static TileManager *
gimp_image_map_get_tiles (GimpPickable *pickable)
{
  GimpImageMap *image_map = GIMP_IMAGE_MAP (pickable);

  if (image_map->undo_tiles)
    return image_map->undo_tiles;
  else
    return gimp_pickable_get_tiles (GIMP_PICKABLE (image_map->drawable));
}

static gboolean
gimp_image_map_get_pixel_at (GimpPickable *pickable,
                             gint          x,
                             gint          y,
                             guchar       *pixel)
{
  GimpImageMap *image_map = GIMP_IMAGE_MAP (pickable);
  GimpItem     *item      = GIMP_ITEM (image_map->drawable);

  if (x >= 0 && x < gimp_item_get_width  (item) &&
      y >= 0 && y < gimp_item_get_height (item))
    {
      /* Check if done damage to original image */
      if (image_map->undo_tiles)
        {
          gint offset_x = image_map->undo_offset_x;
          gint offset_y = image_map->undo_offset_y;
          gint width    = tile_manager_width (image_map->undo_tiles);
          gint height   = tile_manager_height (image_map->undo_tiles);

          if (x >= offset_x && x < offset_x + width &&
              y >= offset_y && y < offset_y + height)
            {
              tile_manager_read_pixel_data_1 (image_map->undo_tiles,
                                              x - offset_x,
                                              y - offset_y,
                                              pixel);

              return TRUE;
            }
        }

      return gimp_pickable_get_pixel_at (GIMP_PICKABLE (image_map->drawable),
                                         x, y, pixel);
    }
  else /* out of bounds error */
    {
      return FALSE;
    }
}

GimpImageMap *
gimp_image_map_new (GimpDrawable          *drawable,
                    const gchar           *undo_desc,
                    GeglNode              *operation,
                    GimpImageMapApplyFunc  apply_func,
                    gpointer               apply_data)
{
  GimpImageMap *image_map;

  g_return_val_if_fail (GIMP_IS_DRAWABLE (drawable), NULL);
  g_return_val_if_fail (gimp_item_is_attached (GIMP_ITEM (drawable)), NULL);
  g_return_val_if_fail (operation == NULL || GEGL_IS_NODE (operation), NULL);
  g_return_val_if_fail (operation != NULL || apply_func != NULL, NULL);

  image_map = g_object_new (GIMP_TYPE_IMAGE_MAP, NULL);

  image_map->drawable  = g_object_ref (drawable);
  image_map->undo_desc = g_strdup (undo_desc);

  if (operation)
    image_map->operation = g_object_ref (operation);

  image_map->apply_func = apply_func;
  image_map->apply_data = apply_data;

  gimp_viewable_preview_freeze (GIMP_VIEWABLE (drawable));

  return image_map;
}

void
gimp_image_map_apply (GimpImageMap        *image_map,
                      const GeglRectangle *visible)
{
  GeglRectangle rect;

  g_return_if_fail (GIMP_IS_IMAGE_MAP (image_map));

  /*  If we're still working, remove the timer  */
  gimp_image_map_cancel_any_idle_jobs (image_map);

  /*  Make sure the drawable is still valid  */
  if (! gimp_item_is_attached (GIMP_ITEM (image_map->drawable)))
    return;

  /*  The application should occur only within selection bounds  */
  if (! gimp_item_mask_intersect (GIMP_ITEM (image_map->drawable),
                                  &rect.x, &rect.y,
                                  &rect.width, &rect.height))
    return;

  /*  If undo tiles don't exist, or change size, (re)allocate  */
  gimp_image_map_update_undo_tiles (image_map,
                                    &rect);

  if (image_map->operation)
    {
      if (! image_map->gegl)
        {
          image_map->gegl = gegl_node_new ();

          g_object_set (image_map->gegl,
                        "dont-cache", TRUE,
                        NULL);

          image_map->input =
            gegl_node_new_child (image_map->gegl,
                                 "operation", "gimp:tilemanager-source",
                                 NULL);

          image_map->translate =
            gegl_node_new_child (image_map->gegl,
                                 "operation", "gegl:translate",
                                 NULL);

          gegl_node_add_child (image_map->gegl, image_map->operation);

          image_map->output =
            gegl_node_new_child (image_map->gegl,
                                 "operation", "gimp:tilemanager-sink",
                                 NULL);

          {
            GObject *sink_operation;

            g_object_get (image_map->output,
                          "gegl-operation", &sink_operation,
                          NULL);

            g_signal_connect (sink_operation, "data-written",
                              G_CALLBACK (gimp_image_map_data_written),
                              image_map);

            g_object_unref (sink_operation);
          }

          if (gegl_node_has_pad (image_map->operation, "input") &&
              gegl_node_has_pad (image_map->operation, "output"))
            {
              /*  if there are input and output pads we probably have a
               *  filter OP, connect it on both ends.
               */
              gegl_node_link_many (image_map->input,
                                   image_map->translate,
                                   image_map->operation,
                                   image_map->output,
                                   NULL);
            }
          else if (gegl_node_has_pad (image_map->operation, "output"))
            {
              /*  if there is only an output pad we probably have a
               *  source OP, blend its result on top of the original
               *  pixels.
               */
              GeglNode *over = gegl_node_new_child (image_map->gegl,
                                                    "operation", "gegl:over",
                                                    NULL);

              gegl_node_link_many (image_map->input,
                                   image_map->translate,
                                   over,
                                   image_map->output,
                                   NULL);

              gegl_node_connect_to (image_map->operation, "output",
                                    over, "aux");
            }
          else
            {
              /* otherwise we just construct a silly nop pipleline
               */
              gegl_node_link_many (image_map->input,
                                   image_map->translate,
                                   image_map->output,
                                   NULL);
            }
        }

      gegl_node_set (image_map->input,
                     "tile-manager", image_map->undo_tiles,
                     "linear",       TRUE,
                     NULL);

      gegl_node_set (image_map->translate,
                     "x", (gdouble) rect.x,
                     "y", (gdouble) rect.y,
                     NULL);

      gegl_node_set (image_map->output,
                     "tile-manager", gimp_drawable_get_shadow_tiles (image_map->drawable),
                     "linear",       TRUE,
                     NULL);

      image_map->processor = gegl_node_new_processor (image_map->output,
                                                      &rect);
    }
  else
    {
      /*  Configure the src from the drawable data  */
      pixel_region_init (&image_map->srcPR,
                         image_map->undo_tiles,
                         0, 0,
                         rect.width, rect.height,
                         FALSE);

      /*  Configure the dest as the shadow buffer  */
      pixel_region_init (&image_map->destPR,
                         gimp_drawable_get_shadow_tiles (image_map->drawable),
                         rect.x, rect.y,
                         rect.width, rect.height,
                         TRUE);

      /*  Apply the image transformation to the pixels  */
      image_map->PRI = pixel_regions_register (2,
                                               &image_map->srcPR,
                                               &image_map->destPR);
    }

  if (image_map->timer)
    {
      image_map->pixel_count = 0;
      g_timer_start (image_map->timer);
      g_timer_stop (image_map->timer);
    }

  /*  Start the intermittant work procedure  */
  image_map->idle_id = g_idle_add ((GSourceFunc) gimp_image_map_do, image_map);
}

void
gimp_image_map_commit (GimpImageMap *image_map)
{
  g_return_if_fail (GIMP_IS_IMAGE_MAP (image_map));

  if (image_map->idle_id)
    {
      g_source_remove (image_map->idle_id);
      image_map->idle_id = 0;

      /*  Finish the changes  */
      while (gimp_image_map_do (image_map));
    }

  /*  Make sure the drawable is still valid  */
  if (! gimp_item_is_attached (GIMP_ITEM (image_map->drawable)))
    return;

  /*  Register an undo step  */
  if (image_map->undo_tiles)
    {
      gint x      = image_map->undo_offset_x;
      gint y      = image_map->undo_offset_y;
      gint width  = tile_manager_width  (image_map->undo_tiles);
      gint height = tile_manager_height (image_map->undo_tiles);

      gimp_drawable_push_undo (image_map->drawable,
                               image_map->undo_desc,
                               x, y, width, height,
                               image_map->undo_tiles, FALSE);

      tile_manager_unref (image_map->undo_tiles);
      image_map->undo_tiles = NULL;
    }
}

void
gimp_image_map_clear (GimpImageMap *image_map)
{
  g_return_if_fail (GIMP_IS_IMAGE_MAP (image_map));

  gimp_image_map_cancel_any_idle_jobs (image_map);

  /*  Make sure the drawable is still valid  */
  if (! gimp_item_is_attached (GIMP_ITEM (image_map->drawable)))
    return;

  /*  restore the original image  */
  if (image_map->undo_tiles)
    {
      PixelRegion srcPR;
      PixelRegion destPR;
      gint        x      = image_map->undo_offset_x;
      gint        y      = image_map->undo_offset_y;
      gint        width  = tile_manager_width  (image_map->undo_tiles);
      gint        height = tile_manager_height (image_map->undo_tiles);

      /*  Copy from the drawable to the tiles  */
      pixel_region_init (&srcPR, image_map->undo_tiles,
                         0, 0, width, height,
                         FALSE);
      pixel_region_init (&destPR, gimp_drawable_get_tiles (image_map->drawable),
                         x, y, width, height,
                         TRUE);

      /* if the user has changed the image depth get out quickly */
      if (destPR.bytes != srcPR.bytes)
        {
          g_message ("image depth change, unable to restore original image");
        }
      else
        {
          copy_region (&srcPR, &destPR);

          gimp_drawable_update (image_map->drawable, x, y, width, height);
        }

      /*  Free the undo_tiles tile manager  */
      tile_manager_unref (image_map->undo_tiles);
      image_map->undo_tiles = NULL;
    }
}

void
gimp_image_map_abort (GimpImageMap *image_map)
{
  g_return_if_fail (GIMP_IS_IMAGE_MAP (image_map));

  gimp_image_map_cancel_any_idle_jobs (image_map);

  if (! gimp_item_is_attached (GIMP_ITEM (image_map->drawable)))
    return;

  gimp_image_map_clear (image_map);
}


/*  private functions  */

static void
gimp_image_map_update_undo_tiles (GimpImageMap        *image_map,
                                  const GeglRectangle *rect)
{
  gint undo_offset_x;
  gint undo_offset_y;
  gint undo_width;
  gint undo_height;

  if (image_map->undo_tiles)
    {
      undo_offset_x = image_map->undo_offset_x;
      undo_offset_y = image_map->undo_offset_y;
      undo_width    = tile_manager_width  (image_map->undo_tiles);
      undo_height   = tile_manager_height (image_map->undo_tiles);
    }
  else
    {
      undo_offset_x = 0;
      undo_offset_y = 0;
      undo_width    = 0;
      undo_height   = 0;
    }

  if (! image_map->undo_tiles     ||
      undo_offset_x != rect->x     ||
      undo_offset_y != rect->y     ||
      undo_width    != rect->width ||
      undo_height   != rect->height)
    {
      /* If either the extents changed or the tiles don't exist,
       * allocate new
       */
      if (! image_map->undo_tiles   ||
          undo_width  != rect->width ||
          undo_height != rect->height)
        {
          /*  Destroy old tiles  */
          if (image_map->undo_tiles)
            tile_manager_unref (image_map->undo_tiles);

          /*  Allocate new tiles  */
          image_map->undo_tiles =
            tile_manager_new (rect->width, rect->height,
                              gimp_drawable_bytes (image_map->drawable));
        }

      /*  Copy from the image to the new tiles  */
      pixel_region_init (&image_map->srcPR,
                         gimp_drawable_get_tiles (image_map->drawable),
                         rect->x, rect->y,
                         rect->width, rect->height,
                         FALSE);
      pixel_region_init (&image_map->destPR,
                         image_map->undo_tiles,
                         0, 0,
                         rect->width, rect->height,
                         TRUE);

      copy_region (&image_map->srcPR, &image_map->destPR);

      /*  Set the offsets  */
      image_map->undo_offset_x = rect->x;
      image_map->undo_offset_y = rect->y;
    }
}

static gboolean
gimp_image_map_do (GimpImageMap *image_map)
{
  if (! gimp_item_is_attached (GIMP_ITEM (image_map->drawable)))
    {
      image_map->idle_id = 0;

      gimp_image_map_kill_any_idle_processors (image_map);

      return FALSE;
    }

  if (image_map->gegl)
    {
      gboolean pending;

      if (image_map->timer)
        g_timer_continue (image_map->timer);

      pending = gegl_processor_work (image_map->processor, NULL);

      if (image_map->timer)
        g_timer_stop (image_map->timer);

      if (! pending)
        {
          if (image_map->timer)
            g_printerr ("%s: %g MPixels/sec\n",
                        image_map->undo_desc,
                        (gdouble) image_map->pixel_count /
                        (1000000.0 *
                         g_timer_elapsed (image_map->timer, NULL)));

          g_object_unref (image_map->processor);
          image_map->processor = NULL;

          image_map->idle_id = 0;

          g_signal_emit (image_map, image_map_signals[FLUSH], 0);

          return FALSE;
        }
    }
  else
    {
      gint i;

      /*  Process up to 16 tiles in one go. This reduces the overhead
       *  caused by updating the display while the imagemap is being
       *  applied and gives us a tiny speedup.
       */
      for (i = 0; i < 16; i++)
        {
          PixelRegion  srcPR;
          PixelRegion  destPR;
          gint         x, y, w, h;

          if (image_map->timer)
            g_timer_continue (image_map->timer);

          x = image_map->destPR.x;
          y = image_map->destPR.y;
          w = image_map->destPR.w;
          h = image_map->destPR.h;

          /* Reset to initial drawable conditions. */
          pixel_region_init (&srcPR, image_map->undo_tiles,
                             x - image_map->undo_offset_x,
                             y - image_map->undo_offset_y,
                             w, h, FALSE);
          pixel_region_init (&destPR,
                             gimp_drawable_get_tiles (image_map->drawable),
                             x, y, w, h, TRUE);
          copy_region (&srcPR, &destPR);

          image_map->apply_func (image_map->apply_data,
                                 &image_map->srcPR,
                                 &image_map->destPR);


          pixel_region_init (&srcPR,
                             gimp_drawable_get_shadow_tiles (image_map->drawable),
                             x, y, w, h, FALSE);

          gimp_drawable_apply_region (image_map->drawable, &srcPR,
                                      FALSE, NULL,
                                      GIMP_OPACITY_OPAQUE, GIMP_REPLACE_MODE,
                                      NULL, NULL,
                                      x, y);

          gimp_drawable_update (image_map->drawable, x, y, w, h);

          image_map->PRI = pixel_regions_process (image_map->PRI);

          if (image_map->timer)
            {
              g_timer_stop (image_map->timer);
              image_map->pixel_count += w * h;
            }

          if (image_map->PRI == NULL)
            {
              if (image_map->timer)
                g_printerr ("%s: %g MPixels/sec\n",
                            image_map->undo_desc,
                            (gdouble) image_map->pixel_count /
                            (1000000.0 *
                             g_timer_elapsed (image_map->timer, NULL)));

              image_map->idle_id = 0;

              g_signal_emit (image_map, image_map_signals[FLUSH], 0);

              return FALSE;
            }
        }
    }

  g_signal_emit (image_map, image_map_signals[FLUSH], 0);

  return TRUE;
}

static void
gimp_image_map_data_written (GObject             *operation,
                             const GeglRectangle *extent,
                             GimpImageMap        *image_map)
{
  PixelRegion srcPR;
  PixelRegion destPR;

#if 0
  g_print ("%s: rect = { %d, %d, %d, %d }\n",
           G_STRFUNC, extent->x, extent->y, extent->width, extent->height);
#endif

  /* Reset to initial drawable conditions. */
  pixel_region_init (&srcPR, image_map->undo_tiles,
                     extent->x - image_map->undo_offset_x,
                     extent->y - image_map->undo_offset_y,
                     extent->width,
                     extent->height,
                     FALSE);
  pixel_region_init (&destPR, gimp_drawable_get_tiles (image_map->drawable),
                     extent->x,
                     extent->y,
                     extent->width,
                     extent->height,
                     TRUE);
  copy_region (&srcPR, &destPR);

  /* Apply the result of the gegl graph. */
  pixel_region_init (&srcPR,
                     gimp_drawable_get_shadow_tiles (image_map->drawable),
                     extent->x,
                     extent->y,
                     extent->width,
                     extent->height,
                     FALSE);

  gimp_drawable_apply_region (image_map->drawable, &srcPR,
                              FALSE, NULL,
                              GIMP_OPACITY_OPAQUE, GIMP_REPLACE_MODE,
                              NULL, NULL,
                              extent->x, extent->y);

  gimp_drawable_update (image_map->drawable,
                        extent->x, extent->y,
                        extent->width, extent->height);

  if (image_map->timer)
    image_map->pixel_count += extent->width * extent->height;
}

static void
gimp_image_map_cancel_any_idle_jobs (GimpImageMap *image_map)
{
  if (image_map->idle_id)
    {
      g_source_remove (image_map->idle_id);
      image_map->idle_id = 0;

      gimp_image_map_kill_any_idle_processors (image_map);
    }
}

static void
gimp_image_map_kill_any_idle_processors (GimpImageMap *image_map)
{
  if (image_map->processor)
    {
      g_object_unref (image_map->processor);
      image_map->processor = NULL;
    }

  if (image_map->PRI)
    {
      pixel_regions_process_stop (image_map->PRI);
      image_map->PRI = NULL;
    }
}
