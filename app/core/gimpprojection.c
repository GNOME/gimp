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

#include <stdlib.h>
#include <string.h>

#include <cairo.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gegl.h>

#include "libgimpbase/gimpbase.h"

#include "core-types.h"

#include "gegl/gimp-babl.h"
#include "gegl/gimp-gegl-utils.h"
#include "gegl/gimptilehandlervalidate.h"

#include "gimp.h"
#include "gimp-memsize.h"
#include "gimpimage.h"
#include "gimpmarshal.h"
#include "gimppickable.h"
#include "gimpprojectable.h"
#include "gimpprojection.h"

#include "gimp-log.h"
#include "gimp-priorities.h"


/*  chunk size for one iteration of the chunk renderer  */
static gint GIMP_PROJECTION_CHUNK_WIDTH  = 256;
static gint GIMP_PROJECTION_CHUNK_HEIGHT = 128;

/*  how much time, in seconds, do we allow chunk rendering to take,
 *  aiming for 15fps
 */
static gdouble GIMP_PROJECTION_CHUNK_TIME = 0.0666;


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


typedef struct _GimpProjectionChunkRender GimpProjectionChunkRender;

struct _GimpProjectionChunkRender
{
  guint           idle_id;

  gint            x;
  gint            y;
  gint            width;
  gint            height;

  gint            work_x;
  gint            work_y;

  cairo_region_t *update_region;   /*  flushed update region */
};

struct _GimpProjectionPrivate
{
  GimpProjectable           *projectable;

  GeglBuffer                *buffer;
  GimpTileHandlerValidate   *validate_handler;

  cairo_region_t            *update_region;
  GimpProjectionChunkRender  chunk_render;
  cairo_rectangle_int_t      priority_rect;

  gboolean                   invalidate_preview;
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
static void        gimp_projection_pixel_to_srgb         (GimpPickable    *pickable,
                                                          const Babl      *format,
                                                          gpointer         pixel,
                                                          GimpRGB         *color);
static void        gimp_projection_srgb_to_pixel         (GimpPickable    *pickable,
                                                          const GimpRGB   *color,
                                                          const Babl      *format,
                                                          gpointer         pixel);

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
  const gchar     *env;

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

  g_type_class_add_private (klass, sizeof (GimpProjectionPrivate));

  env = g_getenv ("GIMP_DISPLAY_RENDER_BUF_SIZE");
  if (env)
    {
      gint width  = atoi (env);
      gint height = width;

      env = strchr (env, 'x');
      if (env)
        height = atoi (env + 1);

      if (width  > 0 && width  <= 8192 &&
          height > 0 && height <= 8192)
        {
          GIMP_PROJECTION_CHUNK_WIDTH  = width;
          GIMP_PROJECTION_CHUNK_HEIGHT = height;
        }
    }
}

static void
gimp_projection_init (GimpProjection *proj)
{
  proj->priv = G_TYPE_INSTANCE_GET_PRIVATE (proj,
                                            GIMP_TYPE_PROJECTION,
                                            GimpProjectionPrivate);
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
  iface->pixel_to_srgb         = gimp_projection_pixel_to_srgb;
  iface->srgb_to_pixel         = gimp_projection_srgb_to_pixel;
}

static void
gimp_projection_finalize (GObject *object)
{
  GimpProjection *proj = GIMP_PROJECTION (object);

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
      g_value_set_object (value, projection->priv->buffer);
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

  memsize += gimp_gegl_pyramid_get_memsize (projection->priv->buffer);

  return memsize + GIMP_OBJECT_CLASS (parent_class)->get_memsize (object,
                                                                  gui_size);
}

/**
 * gimp_projection_estimate_memsize:
 * @type:           the projectable's base type
 * @component_type: the projectable's component type
 * @width:          projection width
 * @height:         projection height
 *
 * Calculates a rough estimate of the memory that is required for the
 * projection of an image with the given @width and @height.
 *
 * Return value: a rough estimate of the memory requirements.
 **/
gint64
gimp_projection_estimate_memsize (GimpImageBaseType type,
                                  GimpComponentType component_type,
                                  gint              width,
                                  gint              height)
{
  const Babl *format;
  gint64      bytes;

  if (type == GIMP_INDEXED)
    type = GIMP_RGB;

  format = gimp_babl_format (type,
                             gimp_babl_precision (component_type, FALSE),
                             TRUE);
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

  if (proj->priv->invalidate_preview)
    {
      /* invalidate the preview here since it is constructed from
       * the projection
       */
      proj->priv->invalidate_preview = FALSE;

      gimp_projectable_invalidate_preview (proj->priv->projectable);
    }
}

static GimpImage *
gimp_projection_get_image (GimpPickable *pickable)
{
  GimpProjection *proj = GIMP_PROJECTION (pickable);

  return gimp_projectable_get_image (proj->priv->projectable);
}

static const Babl *
gimp_projection_get_format (GimpPickable *pickable)
{
  GimpProjection *proj = GIMP_PROJECTION (pickable);

  return gimp_projectable_get_format (proj->priv->projectable);
}

static GeglBuffer *
gimp_projection_get_buffer (GimpPickable *pickable)
{
  GimpProjection *proj = GIMP_PROJECTION (pickable);

  if (! proj->priv->buffer)
    {
      GeglNode   *graph;
      const Babl *format;
      gint        width;
      gint        height;

      graph = gimp_projectable_get_graph (proj->priv->projectable);
      format = gimp_projection_get_format (GIMP_PICKABLE (proj));
      gimp_projectable_get_size (proj->priv->projectable, &width, &height);

      proj->priv->buffer = gegl_buffer_new (GEGL_RECTANGLE (0, 0, width, height),
                                            format);

      proj->priv->validate_handler =
        GIMP_TILE_HANDLER_VALIDATE (gimp_tile_handler_validate_new (graph));

      gimp_tile_handler_validate_assign (proj->priv->validate_handler,
                                         proj->priv->buffer);

      /*  This used to call gimp_tile_handler_validate_invalidate()
       *  which forced the entire projection to be constructed in one
       *  go for new images, causing a potentially huge delay. Now we
       *  initially validate stuff the normal way, which makes the
       *  image appear incrementally, but it keeps everything
       *  responsive.
       */
      gimp_projection_add_update_area (proj, 0, 0, width, height);
      proj->priv->invalidate_preview = TRUE;
      gimp_projection_flush (proj);

      g_object_notify (G_OBJECT (pickable), "buffer");
    }

  return proj->priv->buffer;
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

static void
gimp_projection_pixel_to_srgb (GimpPickable *pickable,
                               const Babl   *format,
                               gpointer      pixel,
                               GimpRGB      *color)
{
  GimpProjection *proj  = GIMP_PROJECTION (pickable);
  GimpImage      *image = gimp_projectable_get_image (proj->priv->projectable);

  gimp_pickable_pixel_to_srgb (GIMP_PICKABLE (image), format, pixel, color);
}

static void
gimp_projection_srgb_to_pixel (GimpPickable  *pickable,
                               const GimpRGB *color,
                               const Babl    *format,
                               gpointer       pixel)
{
  GimpProjection *proj  = GIMP_PROJECTION (pickable);
  GimpImage      *image = gimp_projectable_get_image (proj->priv->projectable);

  gimp_pickable_srgb_to_pixel (GIMP_PICKABLE (image), color, format, pixel);
}


/*  public functions  */

GimpProjection *
gimp_projection_new (GimpProjectable *projectable)
{
  GimpProjection *proj;

  g_return_val_if_fail (GIMP_IS_PROJECTABLE (projectable), NULL);

  proj = g_object_new (GIMP_TYPE_PROJECTION, NULL);

  proj->priv->projectable = projectable;

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
gimp_projection_set_priority_rect (GimpProjection *proj,
                                   gint            x,
                                   gint            y,
                                   gint            w,
                                   gint            h)
{
  cairo_rectangle_int_t rect;
  gint                  off_x, off_y;
  gint                  width, height;

  g_return_if_fail (GIMP_IS_PROJECTION (proj));

  gimp_projectable_get_offset (proj->priv->projectable, &off_x, &off_y);
  gimp_projectable_get_size   (proj->priv->projectable, &width, &height);

  /*  subtract the projectable's offsets because the list of update
   *  areas is in tile-pyramid coordinates, but our external API is
   *  always in terms of image coordinates.
   */
  x -= off_x;
  y -= off_y;

  if (gimp_rectangle_intersect (x, y, w, h,
                                0, 0, width, height,
                                &rect.x, &rect.y, &rect.width, &rect.height))
    {
      proj->priv->priority_rect = rect;

      if (proj->priv->chunk_render.idle_id)
        gimp_projection_chunk_render_init (proj);
    }
}

void
gimp_projection_stop_rendering (GimpProjection *proj)
{
  GimpProjectionChunkRender *chunk_render;
  cairo_rectangle_int_t      rect;

  g_return_if_fail (GIMP_IS_PROJECTION (proj));

  chunk_render = &proj->priv->chunk_render;

  if (! chunk_render->idle_id)
    return;

  if (chunk_render->update_region)
    {
      if (proj->priv->update_region)
        {
          cairo_region_union (proj->priv->update_region,
                              chunk_render->update_region);
        }
      else
        {
          proj->priv->update_region =
            cairo_region_copy (chunk_render->update_region);
        }

      g_clear_pointer (&chunk_render->update_region, cairo_region_destroy);
    }

  rect.x      = chunk_render->x;
  rect.y      = chunk_render->work_y;
  rect.width  = chunk_render->width;
  rect.height = chunk_render->height - (chunk_render->work_y - chunk_render->y);

  /* FIXME this is too much, the entire current row */
  if (proj->priv->update_region)
    cairo_region_union_rectangle (proj->priv->update_region, &rect);
  else
    proj->priv->update_region = cairo_region_create_rectangle (&rect);

  gimp_projection_chunk_render_stop (proj);
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

  if (proj->priv->chunk_render.idle_id)
    {
      gimp_projection_chunk_render_stop (proj);

      while (gimp_projection_chunk_render_iteration (proj));
    }
}


/*  private functions  */

static void
gimp_projection_free_buffer (GimpProjection  *proj)
{
  if (proj->priv->chunk_render.idle_id)
    gimp_projection_chunk_render_stop (proj);

  g_clear_pointer (&proj->priv->update_region, cairo_region_destroy);
  g_clear_pointer (&proj->priv->chunk_render.update_region, cairo_region_destroy);

  if (proj->priv->buffer)
    {
      if (proj->priv->validate_handler)
        gegl_buffer_remove_handler (proj->priv->buffer,
                                    proj->priv->validate_handler);

      g_clear_object (&proj->priv->buffer);
    }

  g_clear_object (&proj->priv->validate_handler);
}

static void
gimp_projection_add_update_area (GimpProjection *proj,
                                 gint            x,
                                 gint            y,
                                 gint            w,
                                 gint            h)
{
  cairo_rectangle_int_t  rect;
  gint                   off_x, off_y;
  gint                   width, height;

  gimp_projectable_get_offset (proj->priv->projectable, &off_x, &off_y);
  gimp_projectable_get_size   (proj->priv->projectable, &width, &height);

  /*  subtract the projectable's offsets because the list of update
   *  areas is in tile-pyramid coordinates, but our external API is
   *  always in terms of image coordinates.
   */
  x -= off_x;
  y -= off_y;

  if (gimp_rectangle_intersect (x, y, w, h,
                                0, 0, width, height,
                                &rect.x, &rect.y, &rect.width, &rect.height))
    {
      if (proj->priv->update_region)
        cairo_region_union_rectangle (proj->priv->update_region, &rect);
      else
        proj->priv->update_region = cairo_region_create_rectangle (&rect);
    }
}

static void
gimp_projection_flush_whenever (GimpProjection *proj,
                                gboolean        now)
{
  if (proj->priv->update_region)
    {
      if (now)  /* Synchronous */
        {
          gint n_rects = cairo_region_num_rectangles (proj->priv->update_region);
          gint i;

          for (i = 0; i < n_rects; i++)
            {
              cairo_rectangle_int_t rect;

              cairo_region_get_rectangle (proj->priv->update_region,
                                          i, &rect);

              gimp_projection_paint_area (proj,
                                          FALSE, /* sic! */
                                          rect.x,
                                          rect.y,
                                          rect.width,
                                          rect.height);
            }
        }
      else  /* Asynchronous */
        {
          gimp_projection_chunk_render_init (proj);
        }

      /*  Free the update region  */
      g_clear_pointer (&proj->priv->update_region, cairo_region_destroy);
    }
  else if (! now && proj->priv->invalidate_preview)
    {
      /* invalidate the preview here since it is constructed from
       * the projection
       */
      proj->priv->invalidate_preview = FALSE;

      gimp_projectable_invalidate_preview (proj->priv->projectable);
    }
}

static void
gimp_projection_chunk_render_start (GimpProjection *proj)
{
  g_return_if_fail (proj->priv->chunk_render.idle_id == 0);

  proj->priv->chunk_render.idle_id =
    g_idle_add_full (GIMP_PRIORITY_PROJECTION_IDLE,
                     gimp_projection_chunk_render_callback, proj,
                     NULL);
}

static void
gimp_projection_chunk_render_stop (GimpProjection *proj)
{
  g_return_if_fail (proj->priv->chunk_render.idle_id != 0);

  g_source_remove (proj->priv->chunk_render.idle_id);
  proj->priv->chunk_render.idle_id = 0;
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
  GimpProjectionChunkRender *chunk_render = &proj->priv->chunk_render;

  /* We need to merge the ChunkRender's and the GimpProjection's
   * update_regions list to keep track of which of the updates have
   * been flushed and hence need to be drawn.
   */
  if (proj->priv->update_region)
    {
      if (chunk_render->update_region)
        {
          cairo_region_union (chunk_render->update_region,
                              proj->priv->update_region);
        }
      else
        {
          chunk_render->update_region =
            cairo_region_copy (proj->priv->update_region);
        }
    }

  /* If a chunk renderer was already running, merge the remainder of
   * its unrendered area with the update_areas list, and make it start
   * work on the next unrendered area in the list.
   */
  if (chunk_render->idle_id)
    {
      cairo_rectangle_int_t rect;

      rect.x      = chunk_render->x;
      rect.y      = chunk_render->work_y;
      rect.width  = chunk_render->width;
      rect.height = (chunk_render->height -
                     (chunk_render->work_y - chunk_render->y));

      if (chunk_render->update_region)
        cairo_region_union_rectangle (chunk_render->update_region, &rect);
      else
        chunk_render->update_region = cairo_region_create_rectangle (&rect);

      gimp_projection_chunk_render_next_area (proj);
    }
  else
    {
      if (chunk_render->update_region == NULL)
        {
          g_warning ("%s: wanted to start chunk render with no update_region",
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
  GimpProjectionChunkRender *chunk_render = &proj->priv->chunk_render;
  gint                       work_x       = chunk_render->work_x;
  gint                       work_y       = chunk_render->work_y;
  gint                       work_w;
  gint                       work_h;

  work_w = MIN (GIMP_PROJECTION_CHUNK_WIDTH,
                chunk_render->x + chunk_render->width - work_x);

  work_h = MIN (GIMP_PROJECTION_CHUNK_HEIGHT,
                chunk_render->y + chunk_render->height - work_y);

  gimp_projection_paint_area (proj, TRUE /* sic! */,
                              work_x, work_y, work_w, work_h);

  chunk_render->work_x += work_w;

  if (chunk_render->work_x >= chunk_render->x + chunk_render->width)
    {
      chunk_render->work_x = chunk_render->x;

      chunk_render->work_y += work_h;

      if (chunk_render->work_y >= chunk_render->y + chunk_render->height)
        {
          if (! gimp_projection_chunk_render_next_area (proj))
            {
              if (proj->priv->invalidate_preview)
                {
                  /* invalidate the preview here since it is constructed from
                   * the projection
                   */
                  proj->priv->invalidate_preview = FALSE;

                  gimp_projectable_invalidate_preview (proj->priv->projectable);
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
  GimpProjectionChunkRender *chunk_render = &proj->priv->chunk_render;
  cairo_region_t            *next_region;
  cairo_rectangle_int_t      rect;

  if (! chunk_render->update_region)
    return FALSE;

  if (cairo_region_is_empty (chunk_render->update_region))
    {
      g_clear_pointer (&chunk_render->update_region, cairo_region_destroy);

      return FALSE;
    }

  next_region = cairo_region_copy (chunk_render->update_region);
  cairo_region_intersect_rectangle (next_region, &proj->priv->priority_rect);

  if (cairo_region_is_empty (next_region))
    cairo_region_get_rectangle (chunk_render->update_region, 0, &rect);
  else
    cairo_region_get_rectangle (next_region, 0, &rect);

  cairo_region_destroy (next_region);

  cairo_region_subtract_rectangle (chunk_render->update_region, &rect);

  if (cairo_region_is_empty (chunk_render->update_region))
    {
      g_clear_pointer (&chunk_render->update_region, cairo_region_destroy);
    }

  chunk_render->x      = rect.x;
  chunk_render->y      = rect.y;
  chunk_render->width  = rect.width;
  chunk_render->height = rect.height;

  chunk_render->work_x = chunk_render->x;
  chunk_render->work_y = chunk_render->y;

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

  gimp_projectable_get_offset (proj->priv->projectable, &off_x, &off_y);
  gimp_projectable_get_size   (proj->priv->projectable, &width, &height);

  if (gimp_rectangle_intersect (x, y, w, h,
                                0, 0, width, height,
                                &x, &y, &w, &h))
    {
      if (proj->priv->validate_handler)
        gimp_tile_handler_validate_invalidate (proj->priv->validate_handler,
                                               x, y, w, h);
      if (now)
        {
          GeglNode *graph = gimp_projectable_get_graph (proj->priv->projectable);

          if (proj->priv->validate_handler)
            gimp_tile_handler_validate_undo_invalidate (proj->priv->validate_handler,
                                                        x, y, w, h);

          gegl_node_blit_buffer (graph, proj->priv->buffer,
                                 GEGL_RECTANGLE (x, y, w, h), 0, GEGL_ABYSS_NONE);
        }

      /*  add the projectable's offsets because the list of update areas
       *  is in tile-pyramid coordinates, but our external API is always
       *  in terms of image coordinates.
       */
      g_signal_emit (proj, projection_signals[UPDATE], 0,
                     now,
                     x + off_x,
                     y + off_y,
                     w,
                     h);
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
    proj->priv->invalidate_preview = TRUE;

  gimp_projection_flush (proj);
}

static void
gimp_projection_projectable_changed (GimpProjectable *projectable,
                                     GimpProjection  *proj)
{
  gint off_x, off_y;
  gint width, height;

  gimp_projection_free_buffer (proj);

  gimp_projectable_get_offset (proj->priv->projectable, &off_x, &off_y);
  gimp_projectable_get_size   (projectable, &width, &height);

  gimp_projection_add_update_area (proj, off_x, off_y, width, height);

  proj->priv->priority_rect.x      = 0;
  proj->priv->priority_rect.y      = 0;
  proj->priv->priority_rect.width  = width;
  proj->priv->priority_rect.height = height;
}
