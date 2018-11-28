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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <stdlib.h>
#include <string.h>

#include <cairo.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gegl.h>

#include "libgimpbase/gimpbase.h"
#include "libgimpmath/gimpmath.h"

#include "core-types.h"

#include "gegl/gimp-babl.h"
#include "gegl/gimp-gegl-loops.h"
#include "gegl/gimp-gegl-utils.h"

#include "gimp.h"
#include "gimp-memsize.h"
#include "gimpimage.h"
#include "gimpmarshal.h"
#include "gimppickable.h"
#include "gimpprojectable.h"
#include "gimpprojection.h"
#include "gimptilehandlerprojectable.h"

#include "gimp-log.h"
#include "gimp-priorities.h"


/*  whether to use adaptive render-chunk size  */
static gboolean GIMP_PROJECTION_ADAPTIVE_CHUNK_SIZE = TRUE;

/*  chunk size for one iteration of the chunk renderer, when the use
 *  of adaptive chunk size is disabled
 */
static gint GIMP_PROJECTION_CHUNK_WIDTH  = 256;
static gint GIMP_PROJECTION_CHUNK_HEIGHT = 128;

/*  the min/max adaptive chunk size  */
#define GIMP_PROJECTION_CHUNK_MIN_WIDTH  128
#define GIMP_PROJECTION_CHUNK_MIN_HEIGHT 128

#define GIMP_PROJECTION_CHUNK_MAX_WIDTH  2048
#define GIMP_PROJECTION_CHUNK_MAX_HEIGHT 2048

/*  the minimal number of processed pixels on the current frame,
 *  above which we calculate a new target pixel count to render
 *  on the next frame
 */
#define GIMP_PROJECTION_MIN_PIXELS_PER_UPDATE 1024

/*  chunk size for area updates  */
static gint GIMP_PROJECTION_UPDATE_CHUNK_WIDTH  = 32;
static gint GIMP_PROJECTION_UPDATE_CHUNK_HEIGHT = 32;

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
  gint            work_height;

  gint            n_pixels;
  gint            target_n_pixels;

  cairo_region_t *update_region;   /*  flushed update region */
};

struct _GimpProjectionPrivate
{
  GimpProjectable           *projectable;

  GeglBuffer                *buffer;
  GimpTileHandlerValidate   *validate_handler;

  gint                       priority;

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
static void        gimp_projection_get_pixel_average     (GimpPickable    *pickable,
                                                          const GeglRectangle *rect,
                                                          const Babl      *format,
                                                          gpointer         pixel);
static void        gimp_projection_pixel_to_srgb         (GimpPickable    *pickable,
                                                          const Babl      *format,
                                                          gpointer         pixel,
                                                          GimpRGB         *color);
static void        gimp_projection_srgb_to_pixel         (GimpPickable    *pickable,
                                                          const GimpRGB   *color,
                                                          const Babl      *format,
                                                          gpointer         pixel);

static void        gimp_projection_allocate_buffer       (GimpProjection  *proj);
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
static void        gimp_projection_chunk_render_reinit   (GimpProjection  *proj);
static gboolean    gimp_projection_chunk_render_iteration(GimpProjection  *proj,
                                                          gboolean         chunk);
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
static void
           gimp_projection_projectable_structure_changed (GimpProjectable *projectable,
                                                          GimpProjection  *proj);
static void   gimp_projection_projectable_bounds_changed (GimpProjectable *projectable,
                                                          gint             old_x,
                                                          gint             old_y,
                                                          gint             old_w,
                                                          gint             old_h,
                                                          GimpProjection  *proj);

static gint        gimp_projection_round_chunk_size      (gdouble          size,
                                                          gboolean         toward_zero);
static gint        gimp_projection_round_chunk_width     (gdouble          width);
static gint        gimp_projection_round_chunk_height    (gdouble          height);


G_DEFINE_TYPE_WITH_CODE (GimpProjection, gimp_projection, GIMP_TYPE_OBJECT,
                         G_ADD_PRIVATE (GimpProjection)
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

  if (g_getenv ("GIMP_NO_ADAPTIVE_CHUNK_SIZE"))
    GIMP_PROJECTION_ADAPTIVE_CHUNK_SIZE = FALSE;

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
  proj->priv = gimp_projection_get_instance_private (proj);
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
  iface->get_pixel_average     = gimp_projection_get_pixel_average;
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
      gint width;
      gint height;

      gimp_projectable_get_size (proj->priv->projectable, &width, &height);

      gimp_projection_allocate_buffer (proj);

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
gimp_projection_get_pixel_average (GimpPickable        *pickable,
                                   const GeglRectangle *rect,
                                   const Babl          *format,
                                   gpointer             pixel)
{
  GeglBuffer *buffer = gimp_projection_get_buffer (pickable);

  return gimp_gegl_average_color (buffer, rect, TRUE, GEGL_ABYSS_NONE, format,
                                  pixel);
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
                           G_CALLBACK (gimp_projection_projectable_structure_changed),
                           proj, 0);
  g_signal_connect_object (projectable, "bounds-changed",
                           G_CALLBACK (gimp_projection_projectable_bounds_changed),
                           proj, 0);

  return proj;
}

void
gimp_projection_set_priority (GimpProjection *proj,
                              gint            priority)
{
  g_return_if_fail (GIMP_IS_PROJECTION (proj));

  proj->priv->priority = priority;
}

gint
gimp_projection_get_priority (GimpProjection *proj)
{
  g_return_val_if_fail (GIMP_IS_PROJECTION (proj), 0);

  return proj->priv->priority;
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
        gimp_projection_chunk_render_reinit (proj);
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

      gimp_projectable_begin_render (proj->priv->projectable);

      while (gimp_projection_chunk_render_iteration (proj, FALSE));

      gimp_projectable_end_render (proj->priv->projectable);
    }
}


/*  private functions  */

static void
gimp_projection_allocate_buffer (GimpProjection *proj)
{
  const Babl *format;
  gint        width;
  gint        height;

  if (proj->priv->buffer)
    return;

  format = gimp_projection_get_format (GIMP_PICKABLE (proj));
  gimp_projectable_get_size (proj->priv->projectable, &width, &height);

  proj->priv->buffer = gegl_buffer_new (GEGL_RECTANGLE (0, 0, width, height),
                                        format);

  proj->priv->validate_handler =
    GIMP_TILE_HANDLER_VALIDATE (
      gimp_tile_handler_projectable_new (proj->priv->projectable));

  gimp_tile_handler_validate_assign (proj->priv->validate_handler,
                                     proj->priv->buffer);

  g_object_notify (G_OBJECT (proj), "buffer");
}

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
        {
          gimp_tile_handler_validate_unassign (proj->priv->validate_handler,
                                               proj->priv->buffer);
        }

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
  cairo_rectangle_int_t rect;
  gint                  width, height;

  gimp_projectable_get_size   (proj->priv->projectable, &width, &height);

  /*  align the rectangle to the UPDATE_CHUNK_WIDTH x UPDATE_CHUNK_HEIGHT grid,
   *  to decrease the complexity of the update area.
   */
  w = ceil  ((gdouble) (x + w) / GIMP_PROJECTION_UPDATE_CHUNK_WIDTH ) * GIMP_PROJECTION_UPDATE_CHUNK_WIDTH;
  h = ceil  ((gdouble) (y + h) / GIMP_PROJECTION_UPDATE_CHUNK_HEIGHT) * GIMP_PROJECTION_UPDATE_CHUNK_HEIGHT;
  x = floor ((gdouble) x       / GIMP_PROJECTION_UPDATE_CHUNK_WIDTH ) * GIMP_PROJECTION_UPDATE_CHUNK_WIDTH;
  y = floor ((gdouble) y       / GIMP_PROJECTION_UPDATE_CHUNK_HEIGHT) * GIMP_PROJECTION_UPDATE_CHUNK_HEIGHT;

  w -= x;
  h -= y;

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
  if (! proj->priv->buffer)
    return;

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
    g_idle_add_full (GIMP_PRIORITY_PROJECTION_IDLE + proj->priv->priority,
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
  GimpProjection            *proj         = data;
  GimpProjectionChunkRender *chunk_render = &proj->priv->chunk_render;
  GTimer                    *timer        = g_timer_new ();
  gint                       chunks       = 0;
  gboolean                   retval       = TRUE;

  /* reset the rendered pixel count, so that we count the number of pixels
   * processed during this frame
   */
  chunk_render->n_pixels = 0;

  gimp_projectable_begin_render (proj->priv->projectable);

  do
    {
      if (! gimp_projection_chunk_render_iteration (proj, TRUE))
        {
          gimp_projection_chunk_render_stop (proj);

          retval = FALSE;

          break;
        }

      chunks++;
    }
  while (g_timer_elapsed (timer, NULL) < GIMP_PROJECTION_CHUNK_TIME);

  gimp_projectable_end_render (proj->priv->projectable);

  /* adjust the target number of pixels to be processed on the next frame,
   * according to the number of pixels processed during this frame and the
   * elapsed time, in order to match the desired frame rate.
   */
  if (chunk_render->n_pixels >= GIMP_PROJECTION_MIN_PIXELS_PER_UPDATE)
    {
      chunk_render->target_n_pixels = floor (chunk_render->n_pixels     *
                                             GIMP_PROJECTION_CHUNK_TIME /
                                             g_timer_elapsed (timer, NULL));
    }

  GIMP_LOG (PROJECTION, "%d chunks in %f seconds\n",
            chunks, g_timer_elapsed (timer, NULL));
  g_timer_destroy (timer);

  return retval;
}

static void
gimp_projection_chunk_render_init (GimpProjection *proj)
{
  GimpProjectionChunkRender *chunk_render = &proj->priv->chunk_render;

  chunk_render->target_n_pixels = GIMP_PROJECTION_CHUNK_WIDTH *
                                  GIMP_PROJECTION_CHUNK_HEIGHT;

  gimp_projection_chunk_render_reinit (proj);
}

static void
gimp_projection_chunk_render_reinit (GimpProjection *proj)
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
      gint                  work_h = 0;

      if (chunk_render->work_x != chunk_render->x)
        {
          work_h = MIN (chunk_render->work_height,
                        chunk_render->y + chunk_render->height -
                        chunk_render->work_y);

          rect.x      = chunk_render->work_x;
          rect.y      = chunk_render->work_y;
          rect.width  = chunk_render->x + chunk_render->width -
                        chunk_render->work_x;
          rect.height = work_h;

          if (chunk_render->update_region)
            cairo_region_union_rectangle (chunk_render->update_region, &rect);
          else
            chunk_render->update_region = cairo_region_create_rectangle (&rect);
        }

      rect.x      = chunk_render->x;
      rect.y      = chunk_render->work_y + work_h;
      rect.width  = chunk_render->width;
      rect.height = chunk_render->y + chunk_render->height - rect.y;

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

      proj->priv->chunk_render.target_n_pixels = GIMP_PROJECTION_CHUNK_WIDTH *
                                                 GIMP_PROJECTION_CHUNK_HEIGHT;

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
gimp_projection_chunk_render_iteration (GimpProjection *proj,
                                        gboolean        chunk)
{
  GimpProjectionChunkRender *chunk_render = &proj->priv->chunk_render;
  gint                       work_x       = chunk_render->work_x;
  gint                       work_y       = chunk_render->work_y;
  gint                       work_w;
  gint                       work_h;

  work_w = chunk_render->x + chunk_render->width  - work_x;
  work_h = chunk_render->y + chunk_render->height - work_y;

  if (chunk)
    {
      if (GIMP_PROJECTION_ADAPTIVE_CHUNK_SIZE)
        {
          gint chunk_w;
          gint chunk_h;

          /* try to render in square chunks */
          chunk_h = gimp_projection_round_chunk_height (
            sqrt (chunk_render->target_n_pixels));

          work_h = MIN (work_h, chunk_h);

          chunk_w = gimp_projection_round_chunk_width (
            chunk_render->target_n_pixels / work_h);

          work_w = MIN (work_w, chunk_w);
        }
      else
        {
          work_w = MIN (work_w, GIMP_PROJECTION_CHUNK_WIDTH);
          work_h = MIN (work_h, GIMP_PROJECTION_CHUNK_HEIGHT);
        }
    }
  else
    {
      if (work_x != chunk_render->x)
        work_h = MIN (work_h, chunk_render->work_height);
    }

  if (work_h != chunk_render->work_height)
    {
      /* if the chunk height changed in the middle of a row, refetch the
       * current area, so that we're back at the beginning of a row
       */
      if (work_x != chunk_render->x)
        gimp_projection_chunk_render_reinit (proj);

      chunk_render->work_height = work_h;
    }

  gimp_projection_paint_area (proj, TRUE /* sic! */,
                              work_x, work_y, work_w, work_h);

  chunk_render->n_pixels += work_w * work_h;

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
                                               GEGL_RECTANGLE (x, y, w, h));
      if (now)
        {
          GeglNode *graph = gimp_projectable_get_graph (proj->priv->projectable);

          if (proj->priv->validate_handler)
            gimp_tile_handler_validate_undo_invalidate (proj->priv->validate_handler,
                                                        GEGL_RECTANGLE (x, y, w, h));

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
  gint off_x, off_y;

  gimp_projectable_get_offset (proj->priv->projectable, &off_x, &off_y);

  /*  subtract the projectable's offsets because the list of update
   *  areas is in tile-pyramid coordinates, but our external API is
   *  always in terms of image coordinates.
   */
  x -= off_x;
  y -= off_y;

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
gimp_projection_projectable_structure_changed (GimpProjectable *projectable,
                                               GimpProjection  *proj)
{
  gint width, height;

  gimp_projection_free_buffer (proj);

  gimp_projectable_get_size (projectable, &width, &height);

  gimp_projection_add_update_area (proj, 0, 0, width, height);

  proj->priv->priority_rect.x      = 0;
  proj->priv->priority_rect.y      = 0;
  proj->priv->priority_rect.width  = width;
  proj->priv->priority_rect.height = height;
}

static void
gimp_projection_projectable_bounds_changed (GimpProjectable *projectable,
                                            gint             old_x,
                                            gint             old_y,
                                            gint             old_w,
                                            gint             old_h,
                                            GimpProjection  *proj)
{
  GeglBuffer              *old_buffer = proj->priv->buffer;
  GimpTileHandlerValidate *old_validate_handler;
  gint                     x, y, w, h;
  gint                     dx, dy;

  if (! old_buffer)
    {
      gimp_projection_projectable_structure_changed (projectable, proj);

      return;
    }

  gimp_projectable_get_offset (projectable, &x, &y);
  gimp_projectable_get_size   (projectable, &w, &h);

  if (x == old_x && y == old_y && w == old_w && h == old_h)
    return;

  if (! gimp_rectangle_intersect (x,     y,     w,     h,
                                  old_x, old_y, old_w, old_h,
                                  NULL,  NULL,  NULL,  NULL))
    {
      gimp_projection_projectable_structure_changed (projectable, proj);

      return;
    }

  dx = old_x - x;
  dy = old_y - y;

#if 1
  /* FIXME:  when there's an offset between the new bounds and the old bounds,
   * use gimp_projection_projectable_structure_changed(), instead of copying a
   * shifted version of the old buffer, since the synchronous copy can take a
   * notable amount of time for big buffers, when the offset is such that tiles
   * are not COW-ed.  while gimp_projection_projectable_structure_changed()
   * causes the projection to be re-rendered, which is overall slower, it's
   * done asynchronously.
   *
   * this needs to be improved.
   */
  if (dx || dy)
    {
      gimp_projection_projectable_structure_changed (projectable, proj);

      return;
    }
#endif

  /* reallocate the buffer, and copy the old buffer to the corresponding
   * region of the new buffer.  additionally, shift and clip all outstanding
   * update regions as necessary.
   */

  old_validate_handler = proj->priv->validate_handler;

  proj->priv->buffer           = NULL;
  proj->priv->validate_handler = NULL;

  gimp_projection_allocate_buffer (proj);

  gimp_tile_handler_validate_buffer_copy (old_buffer,
                                          GEGL_RECTANGLE (0, 0, old_w, old_h),
                                          proj->priv->buffer,
                                          GEGL_RECTANGLE (dx, dy, old_w, old_h));

  if (old_validate_handler)
    {
      gimp_tile_handler_validate_unassign (old_validate_handler, old_buffer);

      g_object_unref (old_validate_handler);
    }

  g_object_unref (old_buffer);

  if (proj->priv->update_region)
    {
      const cairo_rectangle_int_t bounds = {0, 0, w, h};

      cairo_region_translate           (proj->priv->update_region, dx, dy);
      cairo_region_intersect_rectangle (proj->priv->update_region, &bounds);
    }

  if (proj->priv->chunk_render.idle_id)
    {
      const cairo_rectangle_int_t bounds = {0, 0, w, h};

      proj->priv->chunk_render.x += dx;
      proj->priv->chunk_render.y += dy;

      proj->priv->chunk_render.work_x += dx;
      proj->priv->chunk_render.work_y += dx;

      if (proj->priv->chunk_render.update_region)
        {
          cairo_region_translate
            (proj->priv->chunk_render.update_region, dx, dy);
          cairo_region_intersect_rectangle
            (proj->priv->chunk_render.update_region, &bounds);
        }

      if (! gimp_rectangle_intersect (proj->priv->chunk_render.x,
                                      proj->priv->chunk_render.y,
                                      proj->priv->chunk_render.width,
                                      proj->priv->chunk_render.height,

                                      0, 0, w, h,

                                      &proj->priv->chunk_render.x,
                                      &proj->priv->chunk_render.y,
                                      &proj->priv->chunk_render.width,
                                      &proj->priv->chunk_render.height))
        {
          if (! gimp_projection_chunk_render_next_area (proj))
            gimp_projection_chunk_render_stop (proj);
        }
    }

  if (proj->priv->priority_rect.width  > 0 &&
      proj->priv->priority_rect.height > 0)
    {
      proj->priv->priority_rect.x += dx;
      proj->priv->priority_rect.y += dy;

      gimp_rectangle_intersect (proj->priv->priority_rect.x,
                                proj->priv->priority_rect.y,
                                proj->priv->priority_rect.width,
                                proj->priv->priority_rect.height,

                                0, 0, w, h,

                                &proj->priv->priority_rect.x,
                                &proj->priv->priority_rect.y,
                                &proj->priv->priority_rect.width,
                                &proj->priv->priority_rect.height);
    }

  if (dx > 0)
    gimp_projection_add_update_area (proj, 0, 0, dx, h);
  if (dy > 0)
    gimp_projection_add_update_area (proj, 0, 0, w, dy);
  if (dx + old_w < w)
    gimp_projection_add_update_area (proj, dx + old_w, 0, w - (dx + old_w), h);
  if (dy + old_h < h)
    gimp_projection_add_update_area (proj, 0, dy + old_h, w, h - (dy + old_h));

  proj->priv->invalidate_preview = TRUE;
}

static gint
gimp_projection_round_chunk_size (gdouble  size,
                                  gboolean toward_zero)
{
  /* round 'size' (up or down, depending on 'toward_zero') to the closest power
   * of 2
   */

  if (size < 0.0)
    {
      return -gimp_projection_round_chunk_size (-size, toward_zero);
    }
  else if (size == 0.0)
    {
      return 0;
    }
  else if (size < 1.0)
    {
      return toward_zero ? 0 : 1;
    }
  else
    {
      gdouble log2_size = log (size) / G_LN2;

      if (toward_zero)
        log2_size = floor (log2_size);
      else
        log2_size = ceil  (log2_size);

      return 1 << (gint) log2_size;
    }
}

static gint
gimp_projection_round_chunk_width (gdouble width)
{
  gint w = gimp_projection_round_chunk_size (width, FALSE);

  return CLAMP (w, GIMP_PROJECTION_CHUNK_MIN_WIDTH,
                   GIMP_PROJECTION_CHUNK_MAX_WIDTH);
}

static gint
gimp_projection_round_chunk_height (gdouble height)
{
  gint h = gimp_projection_round_chunk_size (height, TRUE);

  return CLAMP (h, GIMP_PROJECTION_CHUNK_MIN_HEIGHT,
                   GIMP_PROJECTION_CHUNK_MAX_HEIGHT);
}
