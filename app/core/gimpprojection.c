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

static gint       gimp_projection_alloc_levels          (GimpProjection *proj,
                                                         gint            top_level);
static void       gimp_projection_release_levels        (GimpProjection *proj);
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
                                                         gint            x,
                                                         gint            y,
                                                         gint            w,
                                                         gint            h);
static void       gimp_projection_validate_tile         (TileManager    *tm,
                                                         Tile           *tile);
static void       gimp_projection_write_quarter         (Tile           *dest,
                                                         Tile           *source,
                                                         gint            i,
                                                         gint            j);
static void       gimp_projection_validate_pyramid_tile (TileManager    *tm,
                                                         Tile           *tile);

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
  gint level;

  proj->image                    = NULL;

  proj->type                     = -1;
  proj->bytes                    = 0;

  for (level = 0; level < PYRAMID_MAX_LEVELS; level++)
    proj->pyramid[level] = NULL;

  proj->top_level                = PYRAMID_BASE_LEVEL;

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
  gint            level;

  if (proj->idle_render.idle_id)
    {
      g_source_remove (proj->idle_render.idle_id);
      proj->idle_render.idle_id = 0;
    }

  gimp_area_list_free (proj->update_areas);
  proj->update_areas = NULL;

  gimp_area_list_free (proj->idle_render.update_areas);
  proj->idle_render.update_areas = NULL;

  for (level = 0; level <= proj->top_level; level++)
    if (proj->pyramid[level])
      {
        tile_manager_unref (proj->pyramid[level]);
        proj->pyramid[level] = NULL;
      }

  proj->top_level = PYRAMID_BASE_LEVEL;

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static gint64
gimp_projection_get_memsize (GimpObject *object,
                             gint64     *gui_size)
{
  GimpProjection *projection = GIMP_PROJECTION (object);
  gint64          memsize = 0;
  gint            level;

  for (level = 0; level <= projection->top_level; level++)
    if (projection->pyramid[level])
      memsize += tile_manager_get_memsize (projection->pyramid[level], FALSE);

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

  /* The levels constitute a geometric sum with a ratio of 1/4. */
  return bytes * (gint64) width * (gint64) height * 1.33;
}


static void
gimp_projection_pickable_flush (GimpPickable *pickable)
{
  GimpProjection *proj = GIMP_PROJECTION (pickable);

  gimp_projection_finish_draw (proj);
  gimp_projection_flush_now (proj);
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
  return gimp_projection_get_tiles_at_level (proj, PYRAMID_BASE_LEVEL);
}

TileManager *
gimp_projection_get_tiles_at_level (GimpProjection *proj,
                                    gint            level)
{
  g_return_val_if_fail (GIMP_IS_PROJECTION (proj), NULL);

  level = gimp_projection_alloc_levels (proj, level);

  g_return_val_if_fail (proj->pyramid[level] != NULL, NULL);

  return proj->pyramid[level];
}

/**
 * gimp_projection_get_level:
 * @proj:    pointer to a GimpProjection
 * @scale_x: horizontal scale factor
 * @scale_y: vertical scale factor
 *
 * This function returns a theoritical optimal pyramid level for a given
 * scale factor. Depending on the size of the image, a smaller level may
 * be used later.
 *
 * Return value: the pyramid level to use for a given display scale factor.
 **/
gint
gimp_projection_get_level (GimpProjection *proj,
                           gdouble         scale_x,
                           gdouble         scale_y)
{
  gdouble scale;
  gdouble next = 1.0;
  gint    level;

  g_return_val_if_fail (GIMP_IS_PROJECTION (proj), PYRAMID_BASE_LEVEL);

  scale = MAX (scale_x, scale_y);

  for (level = PYRAMID_BASE_LEVEL; level < PYRAMID_MAX_LEVELS; level++)
    {
      next /= 2;

      if (next < scale)
        break;
    }

  return level;
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

  return proj->type;
}

gint
gimp_projection_get_bytes (const GimpProjection *proj)
{
  g_return_val_if_fail (GIMP_IS_PROJECTION (proj), 0);

  return proj->bytes;
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

/* This function make sure that levels are allocated up to the level
 * it returns. The return value may be smaller than the level that
 * was actually requested.
 */
static gint
gimp_projection_alloc_levels (GimpProjection *proj,
                              gint            top_level)
{
  gint level;

  top_level = MIN (top_level, PYRAMID_MAX_LEVELS - 1);

  if (! proj->pyramid[PYRAMID_BASE_LEVEL])
    {
      gint width  = proj->image->width;
      gint height = proj->image->height;

      /*  Find the number of bytes required for the projection.
       *  This includes the intensity channels and an alpha channel
       *  if one doesn't exist.
       */
      switch (gimp_image_base_type (proj->image))
        {
        case GIMP_RGB:
        case GIMP_INDEXED:
          proj->type  = GIMP_RGBA_IMAGE;
          proj->bytes = 4;
          break;

        case GIMP_GRAY:
          proj->type  = GIMP_GRAYA_IMAGE;
          proj->bytes = 2;
          break;

        default:
          g_assert_not_reached ();
        }

      proj->pyramid[PYRAMID_BASE_LEVEL] = tile_manager_new (width, height,
                                                            proj->bytes);

      tile_manager_set_user_data (proj->pyramid[PYRAMID_BASE_LEVEL], proj);

      /* Validate tiles by building from the layers of the image. */
      tile_manager_set_validate_proc (proj->pyramid[PYRAMID_BASE_LEVEL],
                                      gimp_projection_validate_tile);

      proj->top_level = PYRAMID_BASE_LEVEL;
    }

  if (top_level <= proj->top_level)
    return top_level;

  for (level = proj->top_level + 1; level <= top_level; level++)
    {
      gint width  = proj->image->width  / (1 << level);
      gint height = proj->image->height / (1 << level);

      if (width == 0 || height == 0)
        return proj->top_level;

      /* There is no use having levels that have the same number of
       * tiles as the parent level.
       */
      if (width <= TILE_WIDTH / 2 && height <= TILE_HEIGHT / 2)
        return proj->top_level;

      proj->top_level      = level;
      proj->pyramid[level] = tile_manager_new (width, height, proj->bytes);

      tile_manager_set_user_data (proj->pyramid[level], proj);

      /* Use the level below to validate tiles. */
      tile_manager_set_validate_proc (proj->pyramid[level],
                                      gimp_projection_validate_pyramid_tile);

      tile_manager_set_level_below (proj->pyramid[level],
                                    proj->pyramid[level - 1]);
    }

  return proj->top_level;
}

static void
gimp_projection_release_levels (GimpProjection *proj)
{
  gint level;

  for (level = 0; level <= proj->top_level; level++)
    if (proj->pyramid[level])
      {
        tile_manager_unref (proj->pyramid[level]);
        proj->pyramid[level] = NULL;
      }

  proj->top_level = 0;
}

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
  gint x1, y1, x2, y2;

  /*  Bounds check  */
  x1 = CLAMP (x,     0, proj->image->width);
  y1 = CLAMP (y,     0, proj->image->height);
  x2 = CLAMP (x + w, 0, proj->image->width);
  y2 = CLAMP (y + h, 0, proj->image->height);

  gimp_projection_invalidate (proj, x1, y1, x2 - x1, y2 - y1);

  g_signal_emit (proj, projection_signals[UPDATE], 0,
                 now, x1, y1, x2 - x1, y2 - y1);
}

static void
gimp_projection_invalidate (GimpProjection *proj,
                            gint            x,
                            gint            y,
                            gint            w,
                            gint            h)
{
  TileManager *tm;
  gint         level;

  for (level = 0; level <= proj->top_level; level++)
    {
      gint c                   = (1 << level);

      /* Tile invalidation must propagate all the way up in the pyramid,
       * so keep width and height > 0.
       */
      gint invalidation_width  = MAX (w / c, 1);
      gint invalidation_height = MAX (h / c, 1);

      tm = gimp_projection_get_tiles_at_level (proj, level);

      tile_manager_invalidate_area (tm,
                                    x / c,
                                    y / c,
                                    invalidation_width,
                                    invalidation_height);
    }
}

static void
gimp_projection_validate_tile (TileManager *tm,
                               Tile        *tile)
{
  GimpProjection *proj = tile_manager_get_user_data (tm);
  gint            x, y;
  gint            w, h;

  /*  Find the coordinates of this tile  */
  tile_manager_get_tile_coordinates (tm, tile, &x, &y);

  w = tile_ewidth  (tile);
  h = tile_eheight (tile);

  gimp_projection_construct (proj, x, y, w, h);
}

static void
gimp_projection_write_quarter (Tile *dest,
                               Tile *src,
                               gint  i,
                               gint  j)
{
  const guchar *src_data    = tile_data_pointer (src, 0, 0);
  guchar       *dest_data   = tile_data_pointer (dest, 0, 0);
  const gint    src_ewidth  = tile_ewidth  (src);
  const gint    src_eheight = tile_eheight (src);
  const gint    dest_ewidth = tile_ewidth  (dest);
  const gint    bpp         = tile_bpp     (dest);
  gint          y;

  /* Adjust dest pointer to the right quadrant. */
  dest_data += i * bpp * (TILE_WIDTH / 2) +
               j * bpp * dest_ewidth * (TILE_HEIGHT / 2);

  for (y = 0; y < src_eheight / 2; y++)
    {
      const guchar *src0 = src_data;
      const guchar *src1 = src_data + bpp;
      const guchar *src2 = src0 + bpp * src_ewidth;
      const guchar *src3 = src1 + bpp * src_ewidth;
      guchar       *dst  = dest_data;
      gint          x;

      for (x = 0; x < src_ewidth / 2; x++)
        {
          gint i;

          for (i = 0; i < bpp; i++)
            dst[i] = (src0[i] + src1[i] + src2[i] + src3[i]) / 4;

          dst += bpp;
          src0 += bpp * 2;
          src1 += bpp * 2;
          src2 += bpp * 2;
          src3 += bpp * 2;
        }

      dest_data += dest_ewidth * bpp;
      src_data += src_ewidth * bpp * 2;
    }
}

static void
gimp_projection_validate_pyramid_tile (TileManager *tm,
                                       Tile        *tile)
{
  gint         tile_col;
  gint         tile_row;
  gint         i;
  gint         j;
  TileManager *level_below  = tile_manager_get_level_below (tm);
  Tile        *source[2][2] = { { NULL, NULL },
                                { NULL, NULL } };

  tile_manager_get_tile_col_row (tm, tile, &tile_col, &tile_row);

  for (i = 0; i < 2; i++)
    for (j = 0; j < 2; j++)
        source[i][j] = tile_manager_get_at (level_below,
                                            tile_col * 2 + i,
                                            tile_row * 2 + j,
                                            TRUE,
                                            FALSE);
  for (i = 0; i < 2; i++)
    for (j = 0; j < 2; j++)
      if (source[i][j])
        {
          gimp_projection_write_quarter (tile, source[i][j], i, j);

          tile_release (source[i][j], FALSE);
        }
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
  gimp_projection_release_levels (proj);
  gimp_projection_add_update_area (proj, 0, 0, image->width, image->height);
}

static void
gimp_projection_image_mode_changed (GimpImage      *image,
                                    GimpProjection *proj)
{
  gimp_projection_release_levels (proj);
  gimp_projection_add_update_area (proj, 0, 0, image->width, image->height);
}

static void
gimp_projection_image_flush (GimpImage      *image,
                             GimpProjection *proj)
{
  gimp_projection_flush (proj);
}
