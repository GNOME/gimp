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

#include "core-types.h"

#include "base/tile.h"
#include "base/tile-manager.h"
#include "base/tile-pyramid.h"

#include "gimp.h"
#include "gimparea.h"
#include "gimpimage.h"
#include "gimpmarshal.h"
#include "gimppickable.h"
#include "gimpprojection.h"
#include "gimpprojection-construct.h"


enum
{
  UPDATE,
  LAST_SIGNAL
};


/*  local function prototypes  */

static void   gimp_projection_pickable_iface_init (GimpPickableInterface *iface);

static void       gimp_projection_finalize              (GObject        *object);

static gint64     gimp_projection_get_memsize           (GimpObject     *object,
                                                         gint64         *gui_size);

static void       gimp_projection_pickable_flush        (GimpPickable   *pickable);
static gboolean   gimp_projection_get_pixel_at          (GimpPickable   *pickable,
                                                         gint            x,
                                                         gint            y,
                                                         guchar         *pixel);
static gint       gimp_projection_get_opacity_at        (GimpPickable   *pickable,
                                                         gint            x,
                                                         gint            y);

static void       gimp_projection_add_update_area       (GimpProjection *proj,
                                                         gint            x,
                                                         gint            y,
                                                         gint            w,
                                                         gint            h);
static void       gimp_projection_flush_whenever        (GimpProjection *proj,
                                                         gboolean        now);
static void       gimp_projection_idle_render_init      (GimpProjection *proj);
static gboolean   gimp_projection_idle_render_callback  (gpointer        data);
static gboolean   gimp_projection_idle_render_next_area (GimpProjection *proj);
static void       gimp_projection_paint_area            (GimpProjection *proj,
                                                         gboolean        now,
                                                         gint            x,
                                                         gint            y,
                                                         gint            w,
                                                         gint            h);
static void       gimp_projection_invalidate            (GimpProjection *proj,
                                                         guint           x,
                                                         guint           y,
                                                         guint           w,
                                                         guint           h);
static void       gimp_projection_validate_tile         (TileManager    *tm,
                                                         Tile           *tile,
                                                         GimpProjection *proj);
static void       gimp_projection_image_update          (GimpImage      *image,
                                                         gint            x,
                                                         gint            y,
                                                         gint            w,
                                                         gint            h,
                                                         GimpProjection *proj);
static void       gimp_projection_image_size_changed    (GimpImage      *image,
                                                         GimpProjection *proj);
static void       gimp_projection_image_mode_changed    (GimpImage      *image,
                                                         GimpProjection *proj);
static void       gimp_projection_image_flush           (GimpImage      *image,
                                                         gboolean        invalidate_preview,
                                                         GimpProjection *proj);


G_DEFINE_TYPE_WITH_CODE (GimpProjection, gimp_projection, GIMP_TYPE_OBJECT,
                         G_IMPLEMENT_INTERFACE (GIMP_TYPE_PICKABLE,
                                                gimp_projection_pickable_iface_init))

#define parent_class gimp_projection_parent_class

static guint projection_signals[LAST_SIGNAL] = { 0 };


static void
gimp_projection_class_init (GimpProjectionClass *klass)
{
  GObjectClass    *object_class      = G_OBJECT_CLASS (klass);
  GimpObjectClass *gimp_object_class = GIMP_OBJECT_CLASS (klass);

  projection_signals[UPDATE] =
    g_signal_new ("update",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (GimpProjectionClass, update),
                  NULL, NULL,
                  gimp_marshal_VOID__BOOLEAN_INT_INT_INT_INT,
                  G_TYPE_NONE, 5,
                  G_TYPE_BOOLEAN,
                  G_TYPE_INT,
                  G_TYPE_INT,
                  G_TYPE_INT,
                  G_TYPE_INT);

  object_class->finalize         = gimp_projection_finalize;

  gimp_object_class->get_memsize = gimp_projection_get_memsize;
}

static void
gimp_projection_init (GimpProjection *proj)
{
  proj->image                    = NULL;
  proj->pyramid                  = NULL;
  proj->update_areas             = NULL;
  proj->idle_render.idle_id      = 0;
  proj->idle_render.update_areas = NULL;
  proj->construct_flag           = FALSE;
}

/* sorry for the evil casts */

static void
gimp_projection_pickable_iface_init (GimpPickableInterface *iface)
{
  iface->flush          = gimp_projection_pickable_flush;
  iface->get_image      = (GimpImage     * (*) (GimpPickable *pickable)) gimp_projection_get_image;
  iface->get_image_type = (GimpImageType   (*) (GimpPickable *pickable)) gimp_projection_get_image_type;
  iface->get_bytes      = (gint            (*) (GimpPickable *pickable)) gimp_projection_get_bytes;
  iface->get_tiles      = (TileManager   * (*) (GimpPickable *pickable)) gimp_projection_get_tiles;
  iface->get_pixel_at   = gimp_projection_get_pixel_at;
  iface->get_opacity_at = gimp_projection_get_opacity_at;
}

static void
gimp_projection_finalize (GObject *object)
{
  GimpProjection *proj = GIMP_PROJECTION (object);

  if (proj->idle_render.idle_id)
    {
      g_source_remove (proj->idle_render.idle_id);
      proj->idle_render.idle_id = 0;
    }

  gimp_area_list_free (proj->update_areas);
  proj->update_areas = NULL;

  gimp_area_list_free (proj->idle_render.update_areas);
  proj->idle_render.update_areas = NULL;

  if (proj->pyramid)
    {
      tile_pyramid_destroy (proj->pyramid);
      proj->pyramid = NULL;
    }

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static gint64
gimp_projection_get_memsize (GimpObject *object,
                             gint64     *gui_size)
{
  GimpProjection *projection = GIMP_PROJECTION (object);
  gint64          memsize    = 0;

  if (projection->pyramid)
    memsize = tile_pyramid_get_memsize (projection->pyramid);

  return memsize + GIMP_OBJECT_CLASS (parent_class)->get_memsize (object,
                                                                  gui_size);
}

/**
 * gimp_projection_estimate_memsize:
 * @type:   the image base type
 * @width:  image width
 * @height: image height
 *
 * Calculates a rough estimate of the memory that is required for the
 * projection of an image with the given @width and @height.
 *
 * Return value: a rough estimate of the memory requirements.
 **/
gint64
gimp_projection_estimate_memsize (GimpImageBaseType type,
                                  gint              width,
                                  gint              height)
{
  gint64 bytes = 0;

  switch (type)
    {
    case GIMP_RGB:
    case GIMP_INDEXED:
      bytes = 4;
      break;

    case GIMP_GRAY:
      bytes = 2;
      break;
    }

  /* The pyramid levels constitute a geometric sum with a ratio of 1/4. */
  return bytes * (gint64) width * (gint64) height * 1.33;
}


static void
gimp_projection_pickable_flush (GimpPickable *pickable)
{
  GimpProjection *proj = GIMP_PROJECTION (pickable);

  gimp_projection_finish_draw (proj);
  gimp_projection_flush_now (proj);

  if (proj->invalidate_preview)
    {
      /* invalidate the preview here since it is constructed from
       * the projection
       */
      proj->invalidate_preview = FALSE;

      gimp_viewable_invalidate_preview (GIMP_VIEWABLE (proj->image));
    }
}

static gboolean
gimp_projection_get_pixel_at (GimpPickable *pickable,
                              gint          x,
                              gint          y,
                              guchar       *pixel)
{
  GimpProjection *proj = GIMP_PROJECTION (pickable);

  if (x < 0 || y < 0 || x >= proj->image->width || y >= proj->image->height)
    return FALSE;

  read_pixel_data_1 (gimp_projection_get_tiles (proj), x, y, pixel);

  return TRUE;
}

static gint
gimp_projection_get_opacity_at (GimpPickable *pickable,
                                gint          x,
                                gint          y)
{
  return OPAQUE_OPACITY;
}

GimpProjection *
gimp_projection_new (GimpImage *image)
{
  GimpProjection *proj;

  g_return_val_if_fail (GIMP_IS_IMAGE (image), NULL);

  proj = g_object_new (GIMP_TYPE_PROJECTION, NULL);

  proj->image = image;

  g_signal_connect_object (image, "update",
                           G_CALLBACK (gimp_projection_image_update),
                           proj, 0);
  g_signal_connect_object (image, "size-changed",
                           G_CALLBACK (gimp_projection_image_size_changed),
                           proj, 0);
  g_signal_connect_object (image, "mode-changed",
                           G_CALLBACK (gimp_projection_image_mode_changed),
                           proj, 0);
  g_signal_connect_object (image, "flush",
                           G_CALLBACK (gimp_projection_image_flush),
                           proj, 0);

  return proj;
}

TileManager *
gimp_projection_get_tiles (GimpProjection *proj)
{
  return gimp_projection_get_tiles_at_level (proj, 0);

}

TileManager *
gimp_projection_get_tiles_at_level (GimpProjection *proj,
                                    gint            level)
{
  g_return_val_if_fail (GIMP_IS_PROJECTION (proj), NULL);

  if (! proj->pyramid)
    {
      proj->pyramid = tile_pyramid_new (gimp_projection_get_image_type (proj),
                                        proj->image->width,
                                        proj->image->height);

      tile_pyramid_set_validate_proc (proj->pyramid,
                                      (TileValidateProc) gimp_projection_validate_tile,
                                      proj);
    }

  return tile_pyramid_get_tiles (proj->pyramid, level);
}

/**
 * gimp_projection_get_level:
 * @proj:    pointer to a GimpProjection
 * @scale_x: horizontal scale factor
 * @scale_y: vertical scale factor
 *
 * This function returns the optimal pyramid level for a given scale factor.
 *
 * Return value: the pyramid level to use for a given display scale factor.
 **/
gint
gimp_projection_get_level (GimpProjection *proj,
                           gdouble         scale_x,
                           gdouble         scale_y)
{
  return tile_pyramid_get_level (proj->image->width,
                                 proj->image->height,
                                 MAX (scale_x, scale_y));
}

GimpImage *
gimp_projection_get_image (const GimpProjection *proj)
{
  g_return_val_if_fail (GIMP_IS_PROJECTION (proj), NULL);

  return proj->image;
}

GimpImageType
gimp_projection_get_image_type (const GimpProjection *proj)
{
  g_return_val_if_fail (GIMP_IS_PROJECTION (proj), -1);

  switch (gimp_image_base_type (proj->image))
    {
    case GIMP_RGB:
    case GIMP_INDEXED:
      return GIMP_RGBA_IMAGE;

    case GIMP_GRAY:
      return GIMP_GRAYA_IMAGE;
    }

  g_assert_not_reached ();
}

gint
gimp_projection_get_bytes (const GimpProjection *proj)
{
  g_return_val_if_fail (GIMP_IS_PROJECTION (proj), 0);

  return GIMP_IMAGE_TYPE_BYTES (gimp_projection_get_image_type (proj));
}

gdouble
gimp_projection_get_opacity (const GimpProjection *proj)
{
  g_return_val_if_fail (GIMP_IS_PROJECTION (proj), GIMP_OPACITY_OPAQUE);

  return GIMP_OPACITY_OPAQUE;
}

void
gimp_projection_flush (GimpProjection *proj)
{
  g_return_if_fail (GIMP_IS_PROJECTION (proj));

  /* Construct on idle time */
  gimp_projection_flush_whenever (proj, FALSE);
}

void
gimp_projection_flush_now (GimpProjection *proj)
{
  g_return_if_fail (GIMP_IS_PROJECTION (proj));

  /* Construct NOW */
  gimp_projection_flush_whenever (proj, TRUE);
}

void
gimp_projection_finish_draw (GimpProjection *proj)
{
  g_return_if_fail (GIMP_IS_PROJECTION (proj));

  if (proj->idle_render.idle_id)
    {
#if 0
      g_printerr ("%s: flushing idle render queue\n", G_STRFUNC);
#endif

      g_source_remove (proj->idle_render.idle_id);
      proj->idle_render.idle_id = 0;

      while (gimp_projection_idle_render_callback (proj));
    }
}


/*  private functions  */

static void
gimp_projection_add_update_area (GimpProjection *proj,
                                 gint            x,
                                 gint            y,
                                 gint            w,
                                 gint            h)
{
  GimpArea *area;

  g_return_if_fail (GIMP_IS_PROJECTION (proj));

  area = gimp_area_new (CLAMP (x, 0, proj->image->width),
                        CLAMP (y, 0, proj->image->height),
                        CLAMP (x + w, 0, proj->image->width),
                        CLAMP (y + h, 0, proj->image->height));

  proj->update_areas = gimp_area_list_process (proj->update_areas, area);
}

static void
gimp_projection_flush_whenever (GimpProjection *proj,
                                gboolean        now)
{
  /*  First the updates...  */
  if (proj->update_areas)
    {
      if (now)  /* Synchronous */
        {
          GSList *list;

          for (list = proj->update_areas; list; list = g_slist_next (list))
            {
              GimpArea *area = list->data;

              if ((area->x1 != area->x2) && (area->y1 != area->y2))
                {
                  gimp_projection_paint_area (proj,
                                              FALSE, /* sic! */
                                              area->x1,
                                              area->y1,
                                              (area->x2 - area->x1),
                                              (area->y2 - area->y1));
                }
            }
        }
      else  /* Asynchronous */
        {
          gimp_projection_idle_render_init (proj);
        }

      /*  Free the update lists  */
      gimp_area_list_free (proj->update_areas);
      proj->update_areas = NULL;
    }
  else if (! now && proj->invalidate_preview)
    {
      /* invalidate the preview here since it is constructed from
       * the projection
       */
      proj->invalidate_preview = FALSE;

      gimp_viewable_invalidate_preview (GIMP_VIEWABLE (proj->image));
    }
}

static void
gimp_projection_idle_render_init (GimpProjection *proj)
{
  GSList *list;

  /* We need to merge the IdleRender's and the GimpProjection's update_areas
   * list to keep track of which of the updates have been flushed and hence
   * need to be drawn.
   */
  for (list = proj->update_areas; list; list = g_slist_next (list))
    {
      GimpArea *area = list->data;

      proj->idle_render.update_areas =
        gimp_area_list_process (proj->idle_render.update_areas,
                                gimp_area_new (area->x1, area->y1,
                                               area->x2, area->y2));
    }

  /* If an idlerender was already running, merge the remainder of its
   * unrendered area with the update_areas list, and make it start work
   * on the next unrendered area in the list.
   */
  if (proj->idle_render.idle_id)
    {
      GimpArea *area =
        gimp_area_new (proj->idle_render.base_x,
                       proj->idle_render.y,
                       proj->idle_render.base_x + proj->idle_render.width,
                       proj->idle_render.y + (proj->idle_render.height -
                                               (proj->idle_render.y -
                                                proj->idle_render.base_y)));

      proj->idle_render.update_areas =
        gimp_area_list_process (proj->idle_render.update_areas, area);

      gimp_projection_idle_render_next_area (proj);
    }
  else
    {
      if (proj->idle_render.update_areas == NULL)
        {
          g_warning ("%s: wanted to start idle render with no update_areas",
                     G_STRFUNC);
          return;
        }

      gimp_projection_idle_render_next_area (proj);

      proj->idle_render.idle_id =
        g_idle_add_full (G_PRIORITY_LOW,
                         gimp_projection_idle_render_callback,
                         proj, NULL);
    }
}

/* Unless specified otherwise, projection re-rendering is organised by
 * IdleRender, which amalgamates areas to be re-rendered and breaks
 * them into bite-sized chunks which are chewed on in a low- priority
 * idle thread.  This greatly improves responsiveness for many GIMP
 * operations.  -- Adam
 */
static gboolean
gimp_projection_idle_render_callback (gpointer data)
{
  GimpProjection *proj = data;
  gint            workx, worky;
  gint            workw, workh;

#define CHUNK_WIDTH  256
#define CHUNK_HEIGHT 128

  workw = CHUNK_WIDTH;
  workh = CHUNK_HEIGHT;
  workx = proj->idle_render.x;
  worky = proj->idle_render.y;

  if (workx + workw > proj->idle_render.base_x + proj->idle_render.width)
    {
      workw = proj->idle_render.base_x + proj->idle_render.width - workx;
    }

  if (worky + workh > proj->idle_render.base_y + proj->idle_render.height)
    {
      workh = proj->idle_render.base_y + proj->idle_render.height - worky;
    }

  gimp_projection_paint_area (proj, TRUE /* sic! */,
                              workx, worky, workw, workh);

  proj->idle_render.x += CHUNK_WIDTH;

  if (proj->idle_render.x >=
      proj->idle_render.base_x + proj->idle_render.width)
    {
      proj->idle_render.x = proj->idle_render.base_x;
      proj->idle_render.y += CHUNK_HEIGHT;

      if (proj->idle_render.y >=
          proj->idle_render.base_y + proj->idle_render.height)
        {
          if (! gimp_projection_idle_render_next_area (proj))
            {
              /* FINISHED */
              proj->idle_render.idle_id = 0;

              if (proj->invalidate_preview)
                {
                  /* invalidate the preview here since it is constructed from
                   * the projection
                   */
                  proj->invalidate_preview = FALSE;

                  gimp_viewable_invalidate_preview (GIMP_VIEWABLE (proj->image));
                }

              return FALSE;
            }
        }
    }

  /* Still work to do. */
  return TRUE;
}

static gboolean
gimp_projection_idle_render_next_area (GimpProjection *proj)
{
  GimpArea *area;

  if (! proj->idle_render.update_areas)
    return FALSE;

  area = proj->idle_render.update_areas->data;

  proj->idle_render.update_areas =
    g_slist_remove (proj->idle_render.update_areas, area);

  proj->idle_render.x      = proj->idle_render.base_x = area->x1;
  proj->idle_render.y      = proj->idle_render.base_y = area->y1;
  proj->idle_render.width  = area->x2 - area->x1;
  proj->idle_render.height = area->y2 - area->y1;

  gimp_area_free (area);

  return TRUE;
}

static void
gimp_projection_paint_area (GimpProjection *proj,
                            gboolean        now,
                            gint            x,
                            gint            y,
                            gint            w,
                            gint            h)
{
  /*  Bounds check  */
  gint x1 = CLAMP (x,     0, proj->image->width);
  gint y1 = CLAMP (y,     0, proj->image->height);
  gint x2 = CLAMP (x + w, 0, proj->image->width);
  gint y2 = CLAMP (y + h, 0, proj->image->height);

  gimp_projection_invalidate (proj, x1, y1, x2 - x1, y2 - y1);

  g_signal_emit (proj, projection_signals[UPDATE], 0,
                 now, x1, y1, x2 - x1, y2 - y1);
}

static void
gimp_projection_invalidate (GimpProjection *proj,
                            guint           x,
                            guint           y,
                            guint           w,
                            guint           h)
{
  if (proj->pyramid)
    tile_pyramid_invalidate_area (proj->pyramid, x, y, w, h);
}

static void
gimp_projection_validate_tile (TileManager    *tm,
                               Tile           *tile,
                               GimpProjection *proj)
{
  gint x, y;

  /*  Find the coordinates of this tile  */
  tile_manager_get_tile_coordinates (tm, tile, &x, &y);

  gimp_projection_construct (proj,
                             x, y,
                             tile_ewidth (tile), tile_eheight (tile));
}

/*  image callbacks  */

static void
gimp_projection_image_update (GimpImage      *image,
                              gint            x,
                              gint            y,
                              gint            w,
                              gint            h,
                              GimpProjection *proj)
{
  gimp_projection_add_update_area (proj, x, y, w, h);
}

static void
gimp_projection_image_size_changed (GimpImage      *image,
                                    GimpProjection *proj)
{
  if (proj->pyramid)
    {
      tile_pyramid_destroy (proj->pyramid);
      proj->pyramid = NULL;
    }

  gimp_projection_add_update_area (proj, 0, 0, image->width, image->height);
}

static void
gimp_projection_image_mode_changed (GimpImage      *image,
                                    GimpProjection *proj)
{
  if (proj->pyramid)
    {
      tile_pyramid_destroy (proj->pyramid);
      proj->pyramid = NULL;
    }

  gimp_projection_add_update_area (proj, 0, 0, image->width, image->height);
}

static void
gimp_projection_image_flush (GimpImage      *image,
                             gboolean        invalidate_preview,
                             GimpProjection *proj)
{
  if (invalidate_preview)
    proj->invalidate_preview = TRUE;

  gimp_projection_flush (proj);
}
