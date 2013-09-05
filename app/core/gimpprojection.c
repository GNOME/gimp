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

#include <cairo.h>
#include <gegl.h>

#include "core-types.h"

#include "gegl/gimp-babl.h"
#include "gegl/gimp-gegl-utils.h"
#include "gegl/gimptilehandlerprojection.h"

#include "gimp.h"
#include "gimp-utils.h"
#include "gimparea.h"
#include "gimpimage.h"
#include "gimpmarshal.h"
#include "gimppickable.h"
#include "gimpprojectable.h"
#include "gimpprojection.h"

#include "gimp-log.h"


/*  just a bit less than GDK_PRIORITY_REDRAW  */
#define GIMP_PROJECTION_IDLE_PRIORITY (G_PRIORITY_HIGH_IDLE + 20 + 1)

/*  chunk size for one iteration of the chunk renderer  */
#define GIMP_PROJECTION_CHUNK_WIDTH  256
#define GIMP_PROJECTION_CHUNK_HEIGHT 128

/*  how much time, in seconds, do we allow chunk rendering to take  */
#define GIMP_PROJECTION_CHUNK_TIME 0.01


enum
{
  UPDATE,
  LAST_SIGNAL
};

enum
{
  PROP_0,
  PROP_BUFFER
};


/*  local function prototypes  */

static void   gimp_projection_pickable_iface_init (GimpPickableInterface  *iface);

static void        gimp_projection_finalize              (GObject         *object);
static void        gimp_projection_set_property          (GObject         *object,
                                                          guint            property_id,
                                                          const GValue    *value,
                                                          GParamSpec      *pspec);
static void        gimp_projection_get_property          (GObject         *object,
                                                          guint            property_id,
                                                          GValue          *value,
                                                          GParamSpec      *pspec);

static gint64      gimp_projection_get_memsize           (GimpObject      *object,
                                                          gint64          *gui_size);

static void        gimp_projection_pickable_flush        (GimpPickable    *pickable);
static GimpImage * gimp_projection_get_image             (GimpPickable    *pickable);
static const Babl * gimp_projection_get_format           (GimpPickable    *pickable);
static GeglBuffer * gimp_projection_get_buffer           (GimpPickable    *pickable);
static gboolean    gimp_projection_get_pixel_at          (GimpPickable    *pickable,
                                                          gint             x,
                                                          gint             y,
                                                          const Babl      *format,
                                                          gpointer         pixel);
static gdouble     gimp_projection_get_opacity_at        (GimpPickable    *pickable,
                                                          gint             x,
                                                          gint             y);

static void        gimp_projection_free_buffer           (GimpProjection  *proj);
static void        gimp_projection_add_update_area       (GimpProjection  *proj,
                                                          gint             x,
                                                          gint             y,
                                                          gint             w,
                                                          gint             h);
static void        gimp_projection_flush_whenever        (GimpProjection  *proj,
                                                          gboolean         now);
static void        gimp_projection_chunk_render_start    (GimpProjection  *proj);
static void        gimp_projection_chunk_render_stop     (GimpProjection  *proj);
static gboolean    gimp_projection_chunk_render_callback (gpointer         data);
static void        gimp_projection_chunk_render_init     (GimpProjection  *proj);
static gboolean    gimp_projection_chunk_render_iteration(GimpProjection  *proj);
static gboolean    gimp_projection_chunk_render_next_area(GimpProjection  *proj);
static void        gimp_projection_paint_area            (GimpProjection  *proj,
                                                          gboolean         now,
                                                          gint             x,
                                                          gint             y,
                                                          gint             w,
                                                          gint             h);

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
  object_class->set_property     = gimp_projection_set_property;
  object_class->get_property     = gimp_projection_get_property;

  gimp_object_class->get_memsize = gimp_projection_get_memsize;

  g_object_class_override_property (object_class, PROP_BUFFER, "buffer");
}

static void
gimp_projection_init (GimpProjection *proj)
{
}

static void
gimp_projection_pickable_iface_init (GimpPickableInterface *iface)
{
  iface->flush                 = gimp_projection_pickable_flush;
  iface->get_image             = gimp_projection_get_image;
  iface->get_format            = gimp_projection_get_format;
  iface->get_format_with_alpha = gimp_projection_get_format; /* sic */
  iface->get_buffer            = gimp_projection_get_buffer;
  iface->get_pixel_at          = gimp_projection_get_pixel_at;
  iface->get_opacity_at        = gimp_projection_get_opacity_at;
}

static void
gimp_projection_finalize (GObject *object)
{
  GimpProjection *proj = GIMP_PROJECTION (object);

  if (proj->chunk_render.running)
    gimp_projection_chunk_render_stop (proj);

  gimp_area_list_free (proj->update_areas);
  proj->update_areas = NULL;

  gimp_area_list_free (proj->chunk_render.update_areas);
  proj->chunk_render.update_areas = NULL;

  gimp_projection_free_buffer (proj);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
gimp_projection_set_property (GObject      *object,
                              guint         property_id,
                              const GValue *value,
                              GParamSpec   *pspec)
{
  switch (property_id)
    {
    case PROP_BUFFER:
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_projection_get_property (GObject    *object,
                              guint       property_id,
                              GValue     *value,
                              GParamSpec *pspec)
{
  GimpProjection *projection = GIMP_PROJECTION (object);

  switch (property_id)
    {
    case PROP_BUFFER:
      g_value_set_object (value, projection->buffer);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static gint64
gimp_projection_get_memsize (GimpObject *object,
                             gint64     *gui_size)
{
  GimpProjection *projection = GIMP_PROJECTION (object);
  gint64          memsize    = 0;

  memsize += gimp_gegl_buffer_get_memsize (projection->buffer);

  return memsize + GIMP_OBJECT_CLASS (parent_class)->get_memsize (object,
                                                                  gui_size);
}

/**
 * gimp_projection_estimate_memsize:
 * @type:      the projectable's base type
 * @precision: the projectable's precision
 * @width:     projection width
 * @height:    projection height
 *
 * Calculates a rough estimate of the memory that is required for the
 * projection of an image with the given @width and @height.
 *
 * Return value: a rough estimate of the memory requirements.
 **/
gint64
gimp_projection_estimate_memsize (GimpImageBaseType type,
                                  GimpPrecision     precision,
                                  gint              width,
                                  gint              height)
{
  const Babl *format;
  gint64      bytes;

  if (type == GIMP_INDEXED)
    type = GIMP_RGB;

  format = gimp_babl_format (type, precision, TRUE);
  bytes  = babl_format_get_bytes_per_pixel (format);

  /* The pyramid levels constitute a geometric sum with a ratio of 1/4. */
  return bytes * (gint64) width * (gint64) height * 1.33;
}


static void
gimp_projection_pickable_flush (GimpPickable *pickable)
{
  GimpProjection *proj = GIMP_PROJECTION (pickable);

  /* create the buffer if it doesn't exist */
  gimp_projection_get_buffer (pickable);

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

static const Babl *
gimp_projection_get_format (GimpPickable *pickable)
{
  GimpProjection *proj = GIMP_PROJECTION (pickable);

  return gimp_projectable_get_format (proj->projectable);
}

static GeglBuffer *
gimp_projection_get_buffer (GimpPickable *pickable)
{
  GimpProjection *proj = GIMP_PROJECTION (pickable);

  if (! proj->buffer)
    {
      GeglNode   *graph;
      const Babl *format;
      gint        width;
      gint        height;

      graph = gimp_projectable_get_graph (proj->projectable);
      format = gimp_projection_get_format (GIMP_PICKABLE (proj));
      gimp_projectable_get_size (proj->projectable, &width, &height);

      proj->buffer = gegl_buffer_new (GEGL_RECTANGLE (0, 0, width, height),
                                      format);

      proj->validate_handler = gimp_tile_handler_projection_new (graph,
                                                                 width, height);
      gegl_buffer_add_handler (proj->buffer, proj->validate_handler);

      /*  This used to call gimp_tile_handler_projection_invalidate()
       *  which forced the entire projection to be constructed in one
       *  go for new images, causing a potentially huge delay. Now we
       *  initially validate stuff the normal way, which makes the
       *  image appear incrementally, but it keeps everything
       *  responsive.
       */
      gimp_projection_add_update_area (proj, 0, 0, width, height);
      proj->invalidate_preview = TRUE;
      gimp_projection_flush (proj);

      g_object_notify (G_OBJECT (pickable), "buffer");
    }

  return proj->buffer;
}

static gboolean
gimp_projection_get_pixel_at (GimpPickable *pickable,
                              gint          x,
                              gint          y,
                              const Babl   *format,
                              gpointer      pixel)
{
  GeglBuffer *buffer = gimp_projection_get_buffer (pickable);

  if (x <  0                               ||
      y <  0                               ||
      x >= gegl_buffer_get_width  (buffer) ||
      y >= gegl_buffer_get_height (buffer))
    return FALSE;

  gegl_buffer_sample (buffer, x, y, NULL, pixel, format,
                      GEGL_SAMPLER_NEAREST, GEGL_ABYSS_NONE);

  return TRUE;
}

static gdouble
gimp_projection_get_opacity_at (GimpPickable *pickable,
                                gint          x,
                                gint          y)
{
  return GIMP_OPACITY_OPAQUE;
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

void
gimp_projection_flush (GimpProjection *proj)
{
  g_return_if_fail (GIMP_IS_PROJECTION (proj));

  /* Construct in chunks */
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

  if (proj->chunk_render.running)
    {
      gimp_projection_chunk_render_stop (proj);

      while (gimp_projection_chunk_render_iteration (proj));
    }
}


/*  private functions  */

static void
gimp_projection_free_buffer (GimpProjection  *proj)
{
  if (proj->buffer)
    {
      if (proj->validate_handler)
        gegl_buffer_remove_handler (proj->buffer, proj->validate_handler);

      g_object_unref (proj->buffer);
      proj->buffer = NULL;
    }

  if (proj->validate_handler)
    {
      g_object_unref (proj->validate_handler);
      proj->validate_handler = NULL;
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
          gimp_projection_chunk_render_init (proj);
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
gimp_projection_chunk_render_start (GimpProjection *proj)
{
  g_return_if_fail (proj->chunk_render.running == FALSE);

  proj->chunk_render_idle_id =
    g_idle_add_full (GIMP_PROJECTION_IDLE_PRIORITY,
                     gimp_projection_chunk_render_callback, proj,
                     NULL);

  proj->chunk_render.running = TRUE;
}

static void
gimp_projection_chunk_render_stop (GimpProjection *proj)
{
  g_return_if_fail (proj->chunk_render.running == TRUE);

  g_source_remove (proj->chunk_render_idle_id);
  proj->chunk_render_idle_id = 0;

  proj->chunk_render.running = FALSE;
}

static gboolean
gimp_projection_chunk_render_callback (gpointer data)
{
  GimpProjection *proj   = data;
  GTimer         *timer  = g_timer_new ();
  gint            chunks = 0;
  gboolean        retval = TRUE;

  do
    {
      if (! gimp_projection_chunk_render_iteration (proj))
        {
          gimp_projection_chunk_render_stop (proj);

          retval = FALSE;

          break;
        }

      chunks++;
    }
  while (g_timer_elapsed (timer, NULL) < GIMP_PROJECTION_CHUNK_TIME);

  GIMP_LOG (PROJECTION, "%d chunks in %f seconds\n",
            chunks, g_timer_elapsed (timer, NULL));
  g_timer_destroy (timer);

  return retval;
}

static void
gimp_projection_chunk_render_init (GimpProjection *proj)
{
  GSList *list;

  /* We need to merge the ChunkRender's and the GimpProjection's
   * update_areas list to keep track of which of the updates have been
   * flushed and hence need to be drawn.
   */
  for (list = proj->update_areas; list; list = g_slist_next (list))
    {
      GimpArea *area = list->data;

      proj->chunk_render.update_areas =
        gimp_area_list_process (proj->chunk_render.update_areas,
                                gimp_area_new (area->x1, area->y1,
                                               area->x2, area->y2));
    }

  /* If a chunk renderer was already running, merge the remainder of
   * its unrendered area with the update_areas list, and make it start
   * work on the next unrendered area in the list.
   */
  if (proj->chunk_render.running)
    {
      GimpArea *area =
        gimp_area_new (proj->chunk_render.base_x,
                       proj->chunk_render.y,
                       proj->chunk_render.base_x + proj->chunk_render.width,
                       proj->chunk_render.y + (proj->chunk_render.height -
                                               (proj->chunk_render.y -
                                                proj->chunk_render.base_y)));

      proj->chunk_render.update_areas =
        gimp_area_list_process (proj->chunk_render.update_areas, area);

      gimp_projection_chunk_render_next_area (proj);
    }
  else
    {
      if (proj->chunk_render.update_areas == NULL)
        {
          g_warning ("%s: wanted to start chunk render with no update_areas",
                     G_STRFUNC);
          return;
        }

      gimp_projection_chunk_render_next_area (proj);

      gimp_projection_chunk_render_start (proj);
    }
}

/* Unless specified otherwise, projection re-rendering is organised by
 * ChunkRender, which amalgamates areas to be re-rendered and breaks
 * them into bite-sized chunks which are chewed on in an idle
 * function. This greatly improves responsiveness for many GIMP
 * operations.  -- Adam
 */
static gboolean
gimp_projection_chunk_render_iteration (GimpProjection *proj)
{
  gint workx = proj->chunk_render.x;
  gint worky = proj->chunk_render.y;
  gint workw = GIMP_PROJECTION_CHUNK_WIDTH;
  gint workh = GIMP_PROJECTION_CHUNK_HEIGHT;

  if (workx + workw > proj->chunk_render.base_x + proj->chunk_render.width)
    {
      workw = proj->chunk_render.base_x + proj->chunk_render.width - workx;
    }

  if (worky + workh > proj->chunk_render.base_y + proj->chunk_render.height)
    {
      workh = proj->chunk_render.base_y + proj->chunk_render.height - worky;
    }

  gimp_projection_paint_area (proj, TRUE /* sic! */,
                              workx, worky, workw, workh);

  proj->chunk_render.x += GIMP_PROJECTION_CHUNK_WIDTH;

  if (proj->chunk_render.x >=
      proj->chunk_render.base_x + proj->chunk_render.width)
    {
      proj->chunk_render.x = proj->chunk_render.base_x;
      proj->chunk_render.y += GIMP_PROJECTION_CHUNK_HEIGHT;

      if (proj->chunk_render.y >=
          proj->chunk_render.base_y + proj->chunk_render.height)
        {
          if (! gimp_projection_chunk_render_next_area (proj))
            {
              if (proj->invalidate_preview)
                {
                  /* invalidate the preview here since it is constructed from
                   * the projection
                   */
                  proj->invalidate_preview = FALSE;

                  gimp_projectable_invalidate_preview (proj->projectable);
                }

              /* FINISHED */
              return FALSE;
            }
        }
    }

  /* Still work to do. */
  return TRUE;
}

static gboolean
gimp_projection_chunk_render_next_area (GimpProjection *proj)
{
  GimpArea *area;

  if (! proj->chunk_render.update_areas)
    return FALSE;

  area = proj->chunk_render.update_areas->data;

  proj->chunk_render.update_areas =
    g_slist_remove (proj->chunk_render.update_areas, area);

  proj->chunk_render.x      = proj->chunk_render.base_x = area->x1;
  proj->chunk_render.y      = proj->chunk_render.base_y = area->y1;
  proj->chunk_render.width  = area->x2 - area->x1;
  proj->chunk_render.height = area->y2 - area->y1;

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

  if (proj->validate_handler)
    gimp_tile_handler_projection_invalidate (proj->validate_handler,
                                             x1, y1, x2 - x1, y2 - y1);
  if (now)
    {
      GeglNode *graph = gimp_projectable_get_graph (proj->projectable);

      if (proj->validate_handler)
        gimp_tile_handler_projection_undo_invalidate (proj->validate_handler,
                                                      x1, y1, x2 - x1, y2 - y1);

      gegl_node_blit_buffer (graph, proj->buffer,
                             GEGL_RECTANGLE (x1, y1, x2 - x1, y2 - y1));
    }

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

  if (proj->chunk_render.running)
    gimp_projection_chunk_render_stop (proj);

  gimp_area_list_free (proj->update_areas);
  proj->update_areas = NULL;

  gimp_projection_free_buffer (proj);

  gimp_projectable_get_offset (proj->projectable, &off_x, &off_y);
  gimp_projectable_get_size (projectable, &width, &height);

  gimp_projection_add_update_area (proj, off_x, off_y, width, height);
}
