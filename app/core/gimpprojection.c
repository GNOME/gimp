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

static void   gimp_projection_class_init          (GimpProjectionClass   *klass);
static void   gimp_projection_init                (GimpProjection        *proj);
static void   gimp_projection_pickable_iface_init (GimpPickableInterface *pickable_iface);

static void       gimp_projection_finalize              (GObject        *object);

static gint64     gimp_projection_get_memsize           (GimpObject     *object,
                                                         gint64         *gui_size);

static guchar   * gimp_projection_get_color_at          (GimpPickable   *pickable,
                                                         gint            x,
                                                         gint            y);

static void       gimp_projection_alloc_tiles           (GimpProjection *proj);
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

static void       gimp_projection_image_update          (GimpImage      *gimage,
                                                         gint            x,
                                                         gint            y,
                                                         gint            w,
                                                         gint            h,
                                                         GimpProjection *proj);
static void       gimp_projection_image_size_changed    (GimpImage      *gimage,
                                                         GimpProjection *proj);
static void       gimp_projection_image_mode_changed    (GimpImage      *gimage,
                                                         GimpProjection *proj);
static void       gimp_projection_image_flush           (GimpImage      *gimage,
                                                         GimpProjection *proj);


static guint projection_signals[LAST_SIGNAL] = { 0 };

static GimpObjectClass *parent_class = NULL;


GType
gimp_projection_get_type (void)
{
  static GType projection_type = 0;

  if (! projection_type)
    {
      static const GTypeInfo projection_info =
      {
        sizeof (GimpProjectionClass),
        (GBaseInitFunc) NULL,
        (GBaseFinalizeFunc) NULL,
        (GClassInitFunc) gimp_projection_class_init,
        NULL,           /* class_finalize */
        NULL,           /* class_data     */
        sizeof (GimpProjection),
        0,              /* n_preallocs    */
        (GInstanceInitFunc) gimp_projection_init,
      };

      static const GInterfaceInfo pickable_iface_info =
      {
        (GInterfaceInitFunc) gimp_projection_pickable_iface_init,
        NULL,           /* iface_finalize */
        NULL            /* iface_data     */
      };

      projection_type = g_type_register_static (GIMP_TYPE_OBJECT,
                                                "GimpProjection",
                                                &projection_info, 0);

      g_type_add_interface_static (projection_type, GIMP_TYPE_PICKABLE,
                                   &pickable_iface_info);
    }

  return projection_type;
}

static void
gimp_projection_class_init (GimpProjectionClass *klass)
{
  GObjectClass    *object_class      = G_OBJECT_CLASS (klass);
  GimpObjectClass *gimp_object_class = GIMP_OBJECT_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);

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
  proj->gimage                   = NULL;

  proj->type                     = -1;
  proj->bytes                    = 0;
  proj->tiles                    = NULL;

  proj->update_areas             = NULL;

  proj->idle_render.idle_id      = 0;
  proj->idle_render.update_areas = NULL;

  proj->construct_flag           = FALSE;
}

static void
gimp_projection_pickable_iface_init (GimpPickableInterface *pickable_iface)
{
  pickable_iface->get_image      = gimp_projection_get_image;
  pickable_iface->get_image_type = gimp_projection_get_image_type;
  pickable_iface->get_tiles      = gimp_projection_get_tiles;
  pickable_iface->get_color_at   = gimp_projection_get_color_at;
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

  if (proj->tiles)
    {
      tile_manager_unref (proj->tiles);
      proj->tiles = NULL;
    }

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static gint64
gimp_projection_get_memsize (GimpObject *object,
                             gint64     *gui_size)
{
  GimpProjection *projection = GIMP_PROJECTION (object);
  gint64          memsize = 0;

  if (projection->tiles)
    memsize += tile_manager_get_memsize (projection->tiles, FALSE);

  return memsize + GIMP_OBJECT_CLASS (parent_class)->get_memsize (object,
                                                                  gui_size);
}

static guchar *
gimp_projection_get_color_at (GimpPickable *pickable,
                              gint          x,
                              gint          y)
{
  GimpProjection *proj = GIMP_PROJECTION (pickable);
  Tile           *tile;
  guchar         *src;
  guchar         *dest;

  if (x < 0 || y < 0 || x >= proj->gimage->width || y >= proj->gimage->height)
    return NULL;

  dest = g_new (guchar, 5);
  tile = tile_manager_get_tile (gimp_projection_get_tiles (proj),
                                x, y, TRUE, FALSE);
  src = tile_data_pointer (tile, x % TILE_WIDTH, y % TILE_HEIGHT);
  gimp_image_get_color (proj->gimage, gimp_projection_get_image_type (proj),
                        src, dest);

  dest[4] = 0;
  tile_release (tile, FALSE);

  return dest;
}

GimpProjection *
gimp_projection_new (GimpImage *gimage)
{
  GimpProjection *proj;

  g_return_val_if_fail (GIMP_IS_IMAGE (gimage), NULL);

  proj = g_object_new (GIMP_TYPE_PROJECTION, NULL);

  proj->gimage = gimage;

  g_signal_connect_object (gimage, "update",
                           G_CALLBACK (gimp_projection_image_update),
                           proj, 0);
  g_signal_connect_object (gimage, "size_changed",
                           G_CALLBACK (gimp_projection_image_size_changed),
                           proj, 0);
  g_signal_connect_object (gimage, "mode_changed",
                           G_CALLBACK (gimp_projection_image_mode_changed),
                           proj, 0);
  g_signal_connect_object (gimage, "flush",
                           G_CALLBACK (gimp_projection_image_flush),
                           proj, 0);

  return proj;
}

TileManager *
gimp_projection_get_tiles (GimpProjection *proj)
{
  g_return_val_if_fail (GIMP_IS_PROJECTION (proj), NULL);

  if (proj->tiles == NULL                                      ||
      tile_manager_width  (proj->tiles) != proj->gimage->width ||
      tile_manager_height (proj->tiles) != proj->gimage->height)
    {
      gimp_projection_alloc_tiles (proj);
    }

  return proj->tiles;
}

GimpImage *
gimp_projection_get_image (const GimpProjection *proj)
{
  g_return_val_if_fail (GIMP_IS_PROJECTION (proj), NULL);

  return proj->gimage;
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
      g_source_remove (proj->idle_render.idle_id);
      proj->idle_render.idle_id = 0;

      while (gimp_projection_idle_render_callback (proj));
    }
}


/*  private functions  */

static void
gimp_projection_alloc_tiles (GimpProjection *proj)
{
  GimpImageType proj_type  = 0;
  gint          proj_bytes = 0;

  g_return_if_fail (GIMP_IS_PROJECTION (proj));

  /*  Find the number of bytes required for the projection.
   *  This includes the intensity channels and an alpha channel
   *  if one doesn't exist.
   */
  switch (gimp_image_base_type (proj->gimage))
    {
    case GIMP_RGB:
    case GIMP_INDEXED:
      proj_bytes = 4;
      proj_type  = GIMP_RGBA_IMAGE;
      break;

    case GIMP_GRAY:
      proj_bytes = 2;
      proj_type  = GIMP_GRAYA_IMAGE;
      break;

    default:
      g_assert_not_reached ();
    }

  if (proj->tiles)
    {
      if (proj_type            != proj->type                       ||
          proj_bytes           != proj->bytes                      ||
          proj->gimage->width  != tile_manager_width (proj->tiles) ||
          proj->gimage->height != tile_manager_height (proj->tiles))
        {
          tile_manager_unref (proj->tiles);
          proj->tiles = NULL;
        }
    }

  if (! proj->tiles)
    {
      proj->type  = proj_type;
      proj->bytes = proj_bytes;

      proj->tiles = tile_manager_new (proj->gimage->width,
                                      proj->gimage->height,
                                      proj->bytes);
      tile_manager_set_user_data (proj->tiles, proj);
      tile_manager_set_validate_proc (proj->tiles,
                                      gimp_projection_validate_tile);
    }
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

  area = gimp_area_new (CLAMP (x, 0, proj->gimage->width),
                        CLAMP (y, 0, proj->gimage->height),
                        CLAMP (x + w, 0, proj->gimage->width),
                        CLAMP (y + h, 0, proj->gimage->height));

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
      proj->update_areas = gimp_area_list_free (proj->update_areas);
    }
}

static void
gimp_projection_idle_render_init (GimpProjection *proj)
{
  GSList   *list;
  GimpArea *area;

  /* We need to merge the IdleRender's and the GimpProjection's update_areas list
   * to keep track of which of the updates have been flushed and hence need
   * to be drawn.
   */
  for (list = proj->update_areas; list; list = g_slist_next (list))
    {
      area = g_memdup (list->data, sizeof (GimpArea));

      proj->idle_render.update_areas =
        gimp_area_list_process (proj->idle_render.update_areas, area);
    }

  /* If an idlerender was already running, merge the remainder of its
   * unrendered area with the update_areas list, and make it start work
   * on the next unrendered area in the list.
   */
  if (proj->idle_render.idle_id)
    {
      area =
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

  area = (GimpArea *) proj->idle_render.update_areas->data;

  proj->idle_render.update_areas =
    g_slist_remove (proj->idle_render.update_areas, area);

  proj->idle_render.x      = proj->idle_render.base_x = area->x1;
  proj->idle_render.y      = proj->idle_render.base_y = area->y1;
  proj->idle_render.width  = area->x2 - area->x1;
  proj->idle_render.height = area->y2 - area->y1;

  g_free (area);

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
  x1 = CLAMP (x,     0, proj->gimage->width);
  y1 = CLAMP (y,     0, proj->gimage->height);
  x2 = CLAMP (x + w, 0, proj->gimage->width);
  y2 = CLAMP (y + h, 0, proj->gimage->height);
  x = x1;
  y = y1;
  w = (x2 - x1);
  h = (y2 - y1);

  gimp_projection_invalidate (proj, x, y, w, h);

  g_signal_emit (proj, projection_signals[UPDATE], 0,
                 now, x, y, w, h);
}

static void
gimp_projection_invalidate (GimpProjection *proj,
                            gint            x,
                            gint            y,
                            gint            w,
                            gint            h)
{
  Tile        *tile;
  TileManager *tm;
  gint         i, j;

  tm = gimp_projection_get_tiles (proj);

  for (i = y; i < (y + h); i += (TILE_HEIGHT - (i % TILE_HEIGHT)))
    for (j = x; j < (x + w); j += (TILE_WIDTH - (j % TILE_WIDTH)))
      {
	tile = tile_manager_get_tile (tm, j, i, FALSE, FALSE);

        tile_invalidate_tile (&tile, tm, j, i);
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


/*  image callbacks  */

static void
gimp_projection_image_update (GimpImage      *gimage,
                              gint            x,
                              gint            y,
                              gint            w,
                              gint            h,
                              GimpProjection *proj)
{
  gimp_projection_add_update_area (proj, x, y, w, h);
}

static void
gimp_projection_image_size_changed (GimpImage      *gimage,
                                    GimpProjection *proj)
{
  gimp_projection_alloc_tiles (proj);
  gimp_projection_add_update_area (proj, 0, 0, gimage->width, gimage->height);
}

static void
gimp_projection_image_mode_changed (GimpImage      *gimage,
                                    GimpProjection *proj)
{
  gimp_projection_alloc_tiles (proj);
  gimp_projection_add_update_area (proj, 0, 0, gimage->width, gimage->height);
}

static void
gimp_projection_image_flush (GimpImage      *gimage,
                             GimpProjection *proj)
{
  gimp_projection_flush (proj);
}

