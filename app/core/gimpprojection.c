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

#include "config.h"

#include <gegl.h>

#include "core-types.h"

#include "base/tile.h"
#include "base/tile-rowhints.h" /* EEK */
#include "base/tile-private.h"  /* EEK */
#include "base/tile-manager.h"
#include "base/tile-pyramid.h"

#include "gimp.h"
#include "gimparea.h"
#include "gimpimage.h"
#include "gimpmarshal.h"
#include "gimppickable.h"
#include "gimpprojectable.h"
#include "gimpprojection.h"
#include "gimpprojection-construct.h"


/*  halfway between G_PRIORITY_HIGH_IDLE and G_PRIORITY_DEFAULT_IDLE  */
#define  GIMP_PROJECTION_IDLE_PRIORITY  150


enum
{
  UPDATE,
  LAST_SIGNAL
};


/*  local function prototypes  */

static void   gimp_projection_pickable_iface_init (GimpPickableInterface  *iface);

static void        gimp_projection_finalize              (GObject         *object);

static gint64      gimp_projection_get_memsize           (GimpObject      *object,
                                                          gint64          *gui_size);

static void        gimp_projection_pickable_flush        (GimpPickable    *pickable);
static GimpImage * gimp_projection_get_image             (GimpPickable    *pickable);
static GimpImageType gimp_projection_get_image_type      (GimpPickable    *pickable);
static gint        gimp_projection_get_bytes             (GimpPickable    *pickable);
static TileManager * gimp_projection_get_tiles           (GimpPickable    *pickable);
static gboolean    gimp_projection_get_pixel_at          (GimpPickable    *pickable,
                                                          gint             x,
                                                          gint             y,
                                                          guchar          *pixel);
static gint        gimp_projection_get_opacity_at        (GimpPickable    *pickable,
                                                          gint             x,
                                                          gint             y);

static void        gimp_projection_add_update_area       (GimpProjection  *proj,
                                                          gint             x,
                                                          gint             y,
                                                          gint             w,
                                                          gint             h);
static void        gimp_projection_flush_whenever        (GimpProjection  *proj,
                                                          gboolean         now);
static void        gimp_projection_idle_render_init      (GimpProjection  *proj);
static gboolean    gimp_projection_idle_render_callback  (gpointer         data);
static gboolean    gimp_projection_idle_render_next_area (GimpProjection  *proj);
static void        gimp_projection_paint_area            (GimpProjection  *proj,
                                                          gboolean         now,
                                                          gint             x,
                                                          gint             y,
                                                          gint             w,
                                                          gint             h);
static void        gimp_projection_invalidate            (GimpProjection  *proj,
                                                          guint            x,
                                                          guint            y,
                                                          guint            w,
                                                          guint            h);
static void        gimp_projection_validate_tile         (TileManager     *tm,
                                                          Tile            *tile,
                                                          GimpProjection  *proj);

static void        gimp_projection_projectable_invalidate(GimpProjectable *projectable,
                                                          gint             x,
                                                          gint             y,
                                                          gint             w,
                                                          gint             h,
                                                          GimpProjection  *proj);
static void        gimp_projection_projectable_flush     (GimpProjectable *projectable,
                                                          gboolean         invalidate_preview,
                                                          GimpProjection  *proj);
static void        gimp_projection_projectable_changed   (GimpProjectable *projectable,
                                                          GimpProjection  *proj);


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
  proj->projectable              = NULL;
  proj->pyramid                  = NULL;
  proj->update_areas             = NULL;
  proj->idle_render.idle_id      = 0;
  proj->idle_render.update_areas = NULL;
  proj->construct_flag           = FALSE;
}

static void
gimp_projection_pickable_iface_init (GimpPickableInterface *iface)
{
  iface->flush          = gimp_projection_pickable_flush;
  iface->get_image      = gimp_projection_get_image;
  iface->get_image_type = gimp_projection_get_image_type;
  iface->get_bytes      = gimp_projection_get_bytes;
  iface->get_tiles      = gimp_projection_get_tiles;
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

  if (proj->graph)
    {
      g_object_unref (proj->graph);
      proj->graph     = NULL;
      proj->sink_node = NULL;
    }

  if (proj->processor)
    {
      g_object_unref (proj->processor);
      proj->processor = NULL;
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
 * @width:  projection width
 * @height: projection height
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

      gimp_projectable_invalidate_preview (proj->projectable);
    }
}

static GimpImage *
gimp_projection_get_image (GimpPickable *pickable)
{
  GimpProjection *proj = GIMP_PROJECTION (pickable);

  return gimp_projectable_get_image (proj->projectable);
}

static GimpImageType
gimp_projection_get_image_type (GimpPickable *pickable)
{
  GimpProjection *proj = GIMP_PROJECTION (pickable);
  GimpImageType   type;

  type = gimp_projectable_get_image_type (proj->projectable);

  switch (GIMP_IMAGE_TYPE_BASE_TYPE (type))
    {
    case GIMP_RGB:
    case GIMP_INDEXED:
      return GIMP_RGBA_IMAGE;

    case GIMP_GRAY:
      return GIMP_GRAYA_IMAGE;
    }

  g_assert_not_reached ();

  return 0;
}

static gint
gimp_projection_get_bytes (GimpPickable *pickable)
{
  return GIMP_IMAGE_TYPE_BYTES (gimp_projection_get_image_type (pickable));
}

static TileManager *
gimp_projection_get_tiles (GimpPickable *pickable)
{
  return gimp_projection_get_tiles_at_level (GIMP_PROJECTION (pickable),
                                             0, NULL);

}

static gboolean
gimp_projection_get_pixel_at (GimpPickable *pickable,
                              gint          x,
                              gint          y,
                              guchar       *pixel)
{
  TileManager *tiles = gimp_projection_get_tiles (pickable);

  if (x <  0                           ||
      y <  0                           ||
      x >= tile_manager_width  (tiles) ||
      y >= tile_manager_height (tiles))
    return FALSE;

  tile_manager_read_pixel_data_1 (tiles, x, y, pixel);

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
gimp_projection_new (GimpProjectable *projectable)
{
  GimpProjection *proj;

  g_return_val_if_fail (GIMP_IS_PROJECTABLE (projectable), NULL);

  proj = g_object_new (GIMP_TYPE_PROJECTION, NULL);

  proj->projectable = projectable;

  g_signal_connect_object (projectable, "invalidate",
                           G_CALLBACK (gimp_projection_projectable_invalidate),
                           proj, 0);
  g_signal_connect_object (projectable, "flush",
                           G_CALLBACK (gimp_projection_projectable_flush),
                           proj, 0);
  g_signal_connect_object (projectable, "structure-changed",
                           G_CALLBACK (gimp_projection_projectable_changed),
                           proj, 0);

  return proj;
}

GeglNode *
gimp_projection_get_sink_node (GimpProjection *proj)
{
  GeglNode *graph;

  g_return_val_if_fail (GIMP_IS_PROJECTION (proj), NULL);

  if (proj->sink_node)
    return proj->sink_node;

  proj->graph = gegl_node_new ();

  g_object_set (proj->graph,
                "dont-cache", TRUE,
                NULL);

  graph = gimp_projectable_get_graph (proj->projectable);
  gegl_node_add_child (proj->graph, graph);

  proj->sink_node =
    gegl_node_new_child (proj->graph,
                         "operation",    "gimp:tilemanager-sink",
                         "tile-manager", gimp_projection_get_tiles (GIMP_PICKABLE (proj)),
                         "linear",       TRUE,
                         NULL);

  gegl_node_connect_to (graph,           "output",
                        proj->sink_node, "input");

  return proj->sink_node;
}

TileManager *
gimp_projection_get_tiles_at_level (GimpProjection *proj,
                                    gint            level,
                                    gboolean       *is_premult)
{
  g_return_val_if_fail (GIMP_IS_PROJECTION (proj), NULL);

  if (! proj->pyramid)
    {
      GimpImageType type;
      gint          width;
      gint          height;

      type = gimp_projection_get_image_type (GIMP_PICKABLE (proj));
      gimp_projectable_get_size (proj->projectable, &width, &height);

      proj->pyramid = tile_pyramid_new (type, width, height);

      tile_pyramid_set_validate_proc (proj->pyramid,
                                      (TileValidateProc) gimp_projection_validate_tile,
                                      proj);

      if (proj->sink_node)
        {
          TileManager *tiles = tile_pyramid_get_tiles (proj->pyramid, 0, NULL);

          gegl_node_set (proj->sink_node,
                         "tile-manager", tiles,
                         NULL);
        }
    }

  return tile_pyramid_get_tiles (proj->pyramid, level, is_premult);
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
  gint width, height;

  gimp_projectable_get_size (proj->projectable, &width, &height);

  return tile_pyramid_get_level (width, height, MAX (scale_x, scale_y));
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
  gint      off_x, off_y;
  gint      width, height;

  gimp_projectable_get_offset (proj->projectable, &off_x, &off_y);
  gimp_projectable_get_size   (proj->projectable, &width, &height);

  /*  subtract the projectable's offsets because the list of update
   *  areas is in tile-pyramid coordinates, but our external API is
   *  always in terms of image coordinates.
   */
  x -= off_x;
  y -= off_y;

  area = gimp_area_new (CLAMP (x,     0, width),
                        CLAMP (y,     0, height),
                        CLAMP (x + w, 0, width),
                        CLAMP (y + h, 0, height));

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

      gimp_projectable_invalidate_preview (proj->projectable);
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
        g_idle_add_full (GIMP_PROJECTION_IDLE_PRIORITY,
                         gimp_projection_idle_render_callback, proj,
                         NULL);
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

                  gimp_projectable_invalidate_preview (proj->projectable);
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
  gint off_x, off_y;
  gint width, height;
  gint x1, y1, x2, y2;

  gimp_projectable_get_offset (proj->projectable, &off_x, &off_y);
  gimp_projectable_get_size   (proj->projectable, &width, &height);

  /*  Bounds check  */
  x1 = CLAMP (x,     0, width);
  y1 = CLAMP (y,     0, height);
  x2 = CLAMP (x + w, 0, width);
  y2 = CLAMP (y + h, 0, height);

  gimp_projection_invalidate (proj, x1, y1, x2 - x1, y2 - y1);

  /*  add the projectable's offsets because the list of update areas
   *  is in tile-pyramid coordinates, but our external API is always
   *  in terms of image coordinates.
   */
  g_signal_emit (proj, projection_signals[UPDATE], 0,
                 now,
                 x1 + off_x,
                 y1 + off_y,
                 x2 - x1,
                 y2 - y1);
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
  Tile *additional[7];
  gint  n_additional = 0;
  gint  x, y;
  gint  width, height;
  gint  tile_width, tile_height;
  gint  col, row;
  gint  i;

  /*  Find the coordinates of this tile  */
  tile_manager_get_tile_coordinates (tm, tile, &x, &y);

  width  = tile_width  = tile_ewidth (tile);
  height = tile_height = tile_eheight (tile);

  tile_manager_get_tile_col_row (tm, tile, &col, &row);

  /*  try to validate up to 8 invalid tiles in a row  */
  while (tile_width == TILE_WIDTH && n_additional < 7)
    {
      Tile *t;

      col++;

      /*  get the next tile without any read or write access, so it
       *  won't be locked (and validated)
       */
      t = tile_manager_get_at (tm, col, row, FALSE, FALSE);

      /*  if we hit the right border, or a valid tile, bail out
       */
      if (! t || tile_is_valid (t))
        break;

      /*  HACK: mark the tile as valid, so locking it with r/w access
       *  won't validate it
       */
      t->valid = TRUE;
      t = tile_manager_get_at (tm, col, row, TRUE, TRUE);

      /*  add the tile's width to the chunk to validate  */
      tile_width = tile_ewidth (t);
      width += tile_width;

      additional[n_additional++] = t;
    }

  gimp_projection_construct (proj, x, y, width, height);

  for (i = 0; i < n_additional; i++)
    {
      /*  HACK: mark the tile as valid, because we know it is  */
      additional[i]->valid = TRUE;
      tile_release (additional[i], TRUE);
    }
}

/*  image callbacks  */

static void
gimp_projection_projectable_invalidate (GimpProjectable *projectable,
                                        gint             x,
                                        gint             y,
                                        gint             w,
                                        gint             h,
                                        GimpProjection  *proj)
{
  gimp_projection_add_update_area (proj, x, y, w, h);
}

static void
gimp_projection_projectable_flush (GimpProjectable *projectable,
                                   gboolean         invalidate_preview,
                                   GimpProjection  *proj)
{
  if (invalidate_preview)
    proj->invalidate_preview = TRUE;

  gimp_projection_flush (proj);
}

static void
gimp_projection_projectable_changed (GimpProjectable *projectable,
                                     GimpProjection  *proj)
{
  gint off_x, off_y;
  gint width, height;

  if (proj->idle_render.idle_id)
    {
      g_source_remove (proj->idle_render.idle_id);
      proj->idle_render.idle_id = 0;
    }

  gimp_area_list_free (proj->update_areas);
  proj->update_areas = NULL;

  if (proj->pyramid)
    {
      tile_pyramid_destroy (proj->pyramid);
      proj->pyramid = NULL;
    }

  gimp_projectable_get_offset (proj->projectable, &off_x, &off_y);
  gimp_projectable_get_size (projectable, &width, &height);

  gimp_projection_add_update_area (proj, off_x, off_y, width, height);
}
