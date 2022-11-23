/* LIGMA - The GNU Image Manipulation Program
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

#include "libligmabase/ligmabase.h"
#include "libligmamath/ligmamath.h"

#include "core-types.h"

#include "gegl/ligma-babl.h"
#include "gegl/ligma-gegl-loops.h"
#include "gegl/ligma-gegl-utils.h"

#include "ligma.h"
#include "ligma-memsize.h"
#include "ligmachunkiterator.h"
#include "ligmaimage.h"
#include "ligmamarshal.h"
#include "ligmapickable.h"
#include "ligmaprojectable.h"
#include "ligmaprojection.h"
#include "ligmatilehandlerprojectable.h"

#include "ligma-log.h"
#include "ligma-priorities.h"


/*  chunk size for area updates  */
#define LIGMA_PROJECTION_UPDATE_CHUNK_WIDTH  32
#define LIGMA_PROJECTION_UPDATE_CHUNK_HEIGHT 32


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


struct _LigmaProjectionPrivate
{
  LigmaProjectable           *projectable;

  GeglBuffer                *buffer;
  LigmaTileHandlerValidate   *validate_handler;

  gint                       priority;

  cairo_region_t            *update_region;
  GeglRectangle              priority_rect;
  LigmaChunkIterator         *iter;
  guint                      idle_id;

  gboolean                   invalidate_preview;
};


/*  local function prototypes  */

static void   ligma_projection_pickable_iface_init (LigmaPickableInterface  *iface);

static void        ligma_projection_finalize              (GObject         *object);
static void        ligma_projection_set_property          (GObject         *object,
                                                          guint            property_id,
                                                          const GValue    *value,
                                                          GParamSpec      *pspec);
static void        ligma_projection_get_property          (GObject         *object,
                                                          guint            property_id,
                                                          GValue          *value,
                                                          GParamSpec      *pspec);

static gint64      ligma_projection_get_memsize           (LigmaObject      *object,
                                                          gint64          *gui_size);

static void        ligma_projection_pickable_flush        (LigmaPickable    *pickable);
static LigmaImage * ligma_projection_get_image             (LigmaPickable    *pickable);
static const Babl * ligma_projection_get_format           (LigmaPickable    *pickable);
static GeglBuffer * ligma_projection_get_buffer           (LigmaPickable    *pickable);
static gboolean    ligma_projection_get_pixel_at          (LigmaPickable    *pickable,
                                                          gint             x,
                                                          gint             y,
                                                          const Babl      *format,
                                                          gpointer         pixel);
static gdouble     ligma_projection_get_opacity_at        (LigmaPickable    *pickable,
                                                          gint             x,
                                                          gint             y);
static void        ligma_projection_get_pixel_average     (LigmaPickable    *pickable,
                                                          const GeglRectangle *rect,
                                                          const Babl      *format,
                                                          gpointer         pixel);
static void        ligma_projection_pixel_to_srgb         (LigmaPickable    *pickable,
                                                          const Babl      *format,
                                                          gpointer         pixel,
                                                          LigmaRGB         *color);
static void        ligma_projection_srgb_to_pixel         (LigmaPickable    *pickable,
                                                          const LigmaRGB   *color,
                                                          const Babl      *format,
                                                          gpointer         pixel);

static void        ligma_projection_allocate_buffer       (LigmaProjection  *proj);
static void        ligma_projection_free_buffer           (LigmaProjection  *proj);
static void        ligma_projection_add_update_area       (LigmaProjection  *proj,
                                                          gint             x,
                                                          gint             y,
                                                          gint             w,
                                                          gint             h);
static void        ligma_projection_flush_whenever        (LigmaProjection  *proj,
                                                          gboolean         now,
                                                          gboolean         direct);
static void        ligma_projection_update_priority_rect  (LigmaProjection  *proj);
static void        ligma_projection_chunk_render_start    (LigmaProjection  *proj);
static void        ligma_projection_chunk_render_stop     (LigmaProjection  *proj,
                                                          gboolean         merge);
static gboolean    ligma_projection_chunk_render_callback (LigmaProjection  *proj);
static gboolean    ligma_projection_chunk_render_iteration(LigmaProjection  *proj);
static void        ligma_projection_paint_area            (LigmaProjection  *proj,
                                                          gboolean         now,
                                                          gint             x,
                                                          gint             y,
                                                          gint             w,
                                                          gint             h);

static void        ligma_projection_projectable_invalidate(LigmaProjectable *projectable,
                                                          gint             x,
                                                          gint             y,
                                                          gint             w,
                                                          gint             h,
                                                          LigmaProjection  *proj);
static void        ligma_projection_projectable_flush     (LigmaProjectable *projectable,
                                                          gboolean         invalidate_preview,
                                                          LigmaProjection  *proj);
static void
           ligma_projection_projectable_structure_changed (LigmaProjectable *projectable,
                                                          LigmaProjection  *proj);
static void   ligma_projection_projectable_bounds_changed (LigmaProjectable *projectable,
                                                          gint             old_x,
                                                          gint             old_y,
                                                          LigmaProjection  *proj);


G_DEFINE_TYPE_WITH_CODE (LigmaProjection, ligma_projection, LIGMA_TYPE_OBJECT,
                         G_ADD_PRIVATE (LigmaProjection)
                         G_IMPLEMENT_INTERFACE (LIGMA_TYPE_PICKABLE,
                                                ligma_projection_pickable_iface_init))

#define parent_class ligma_projection_parent_class

static guint projection_signals[LAST_SIGNAL] = { 0 };


static void
ligma_projection_class_init (LigmaProjectionClass *klass)
{
  GObjectClass    *object_class      = G_OBJECT_CLASS (klass);
  LigmaObjectClass *ligma_object_class = LIGMA_OBJECT_CLASS (klass);

  projection_signals[UPDATE] =
    g_signal_new ("update",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (LigmaProjectionClass, update),
                  NULL, NULL,
                  ligma_marshal_VOID__BOOLEAN_INT_INT_INT_INT,
                  G_TYPE_NONE, 5,
                  G_TYPE_BOOLEAN,
                  G_TYPE_INT,
                  G_TYPE_INT,
                  G_TYPE_INT,
                  G_TYPE_INT);

  object_class->finalize         = ligma_projection_finalize;
  object_class->set_property     = ligma_projection_set_property;
  object_class->get_property     = ligma_projection_get_property;

  ligma_object_class->get_memsize = ligma_projection_get_memsize;

  g_object_class_override_property (object_class, PROP_BUFFER, "buffer");
}

static void
ligma_projection_init (LigmaProjection *proj)
{
  proj->priv = ligma_projection_get_instance_private (proj);
}

static void
ligma_projection_pickable_iface_init (LigmaPickableInterface *iface)
{
  iface->flush                 = ligma_projection_pickable_flush;
  iface->get_image             = ligma_projection_get_image;
  iface->get_format            = ligma_projection_get_format;
  iface->get_format_with_alpha = ligma_projection_get_format; /* sic */
  iface->get_buffer            = ligma_projection_get_buffer;
  iface->get_pixel_at          = ligma_projection_get_pixel_at;
  iface->get_opacity_at        = ligma_projection_get_opacity_at;
  iface->get_pixel_average     = ligma_projection_get_pixel_average;
  iface->pixel_to_srgb         = ligma_projection_pixel_to_srgb;
  iface->srgb_to_pixel         = ligma_projection_srgb_to_pixel;
}

static void
ligma_projection_finalize (GObject *object)
{
  LigmaProjection *proj = LIGMA_PROJECTION (object);

  ligma_projection_free_buffer (proj);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
ligma_projection_set_property (GObject      *object,
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
ligma_projection_get_property (GObject    *object,
                              guint       property_id,
                              GValue     *value,
                              GParamSpec *pspec)
{
  LigmaProjection *projection = LIGMA_PROJECTION (object);

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
ligma_projection_get_memsize (LigmaObject *object,
                             gint64     *gui_size)
{
  LigmaProjection *projection = LIGMA_PROJECTION (object);
  gint64          memsize    = 0;

  memsize += ligma_gegl_pyramid_get_memsize (projection->priv->buffer);

  return memsize + LIGMA_OBJECT_CLASS (parent_class)->get_memsize (object,
                                                                  gui_size);
}

/**
 * ligma_projection_estimate_memsize:
 * @type:           the projectable's base type
 * @component_type: the projectable's component type
 * @width:          projection width
 * @height:         projection height
 *
 * Calculates a rough estimate of the memory that is required for the
 * projection of an image with the given @width and @height.
 *
 * Returns: a rough estimate of the memory requirements.
 **/
gint64
ligma_projection_estimate_memsize (LigmaImageBaseType type,
                                  LigmaComponentType component_type,
                                  gint              width,
                                  gint              height)
{
  const Babl *format;
  gint64      bytes;

  if (type == LIGMA_INDEXED)
    type = LIGMA_RGB;

  format = ligma_babl_format (type,
                             ligma_babl_precision (component_type, FALSE),
                             TRUE, NULL);
  bytes  = babl_format_get_bytes_per_pixel (format);

  /* The pyramid levels constitute a geometric sum with a ratio of 1/4. */
  return bytes * (gint64) width * (gint64) height * 1.33;
}


static void
ligma_projection_pickable_flush (LigmaPickable *pickable)
{
  LigmaProjection *proj = LIGMA_PROJECTION (pickable);

  /* create the buffer if it doesn't exist */
  ligma_projection_get_buffer (pickable);

  ligma_projection_finish_draw (proj);
  ligma_projection_flush_now (proj, FALSE);

  if (proj->priv->invalidate_preview)
    {
      /* invalidate the preview here since it is constructed from
       * the projection
       */
      proj->priv->invalidate_preview = FALSE;

      ligma_projectable_invalidate_preview (proj->priv->projectable);
    }
}

static LigmaImage *
ligma_projection_get_image (LigmaPickable *pickable)
{
  LigmaProjection *proj = LIGMA_PROJECTION (pickable);

  return ligma_projectable_get_image (proj->priv->projectable);
}

static const Babl *
ligma_projection_get_format (LigmaPickable *pickable)
{
  LigmaProjection *proj = LIGMA_PROJECTION (pickable);

  return ligma_projectable_get_format (proj->priv->projectable);
}

static GeglBuffer *
ligma_projection_get_buffer (LigmaPickable *pickable)
{
  LigmaProjection *proj = LIGMA_PROJECTION (pickable);

  if (! proj->priv->buffer)
    {
      GeglRectangle bounding_box;

      bounding_box =
        ligma_projectable_get_bounding_box (proj->priv->projectable);

      ligma_projection_allocate_buffer (proj);

      /*  This used to call ligma_tile_handler_validate_invalidate()
       *  which forced the entire projection to be constructed in one
       *  go for new images, causing a potentially huge delay. Now we
       *  initially validate stuff the normal way, which makes the
       *  image appear incrementally, but it keeps everything
       *  responsive.
       */
      ligma_projection_add_update_area (proj,
                                       bounding_box.x,     bounding_box.y,
                                       bounding_box.width, bounding_box.height);
      proj->priv->invalidate_preview = TRUE;
      ligma_projection_flush (proj);
    }

  return proj->priv->buffer;
}

static gboolean
ligma_projection_get_pixel_at (LigmaPickable *pickable,
                              gint          x,
                              gint          y,
                              const Babl   *format,
                              gpointer      pixel)
{
  LigmaProjection *proj   = LIGMA_PROJECTION (pickable);
  GeglBuffer     *buffer = ligma_projection_get_buffer (pickable);
  GeglRectangle   bounding_box;

  bounding_box = ligma_projectable_get_bounding_box (proj->priv->projectable);

  if (x <  bounding_box.x                      ||
      y <  bounding_box.y                      ||
      x >= bounding_box.x + bounding_box.width ||
      y >= bounding_box.y + bounding_box.height)
    {
      return FALSE;
    }

  gegl_buffer_sample (buffer, x, y, NULL, pixel, format,
                      GEGL_SAMPLER_NEAREST, GEGL_ABYSS_NONE);

  return TRUE;
}

static gdouble
ligma_projection_get_opacity_at (LigmaPickable *pickable,
                                gint          x,
                                gint          y)
{
  return LIGMA_OPACITY_OPAQUE;
}

static void
ligma_projection_get_pixel_average (LigmaPickable        *pickable,
                                   const GeglRectangle *rect,
                                   const Babl          *format,
                                   gpointer             pixel)
{
  GeglBuffer *buffer = ligma_projection_get_buffer (pickable);

  return ligma_gegl_average_color (buffer, rect, TRUE, GEGL_ABYSS_NONE, format,
                                  pixel);
}

static void
ligma_projection_pixel_to_srgb (LigmaPickable *pickable,
                               const Babl   *format,
                               gpointer      pixel,
                               LigmaRGB      *color)
{
  LigmaProjection *proj  = LIGMA_PROJECTION (pickable);
  LigmaImage      *image = ligma_projectable_get_image (proj->priv->projectable);

  ligma_pickable_pixel_to_srgb (LIGMA_PICKABLE (image), format, pixel, color);
}

static void
ligma_projection_srgb_to_pixel (LigmaPickable  *pickable,
                               const LigmaRGB *color,
                               const Babl    *format,
                               gpointer       pixel)
{
  LigmaProjection *proj  = LIGMA_PROJECTION (pickable);
  LigmaImage      *image = ligma_projectable_get_image (proj->priv->projectable);

  ligma_pickable_srgb_to_pixel (LIGMA_PICKABLE (image), color, format, pixel);
}


/*  public functions  */

LigmaProjection *
ligma_projection_new (LigmaProjectable *projectable)
{
  LigmaProjection *proj;

  g_return_val_if_fail (LIGMA_IS_PROJECTABLE (projectable), NULL);

  proj = g_object_new (LIGMA_TYPE_PROJECTION, NULL);

  proj->priv->projectable = projectable;

  g_signal_connect_object (projectable, "invalidate",
                           G_CALLBACK (ligma_projection_projectable_invalidate),
                           proj, 0);
  g_signal_connect_object (projectable, "flush",
                           G_CALLBACK (ligma_projection_projectable_flush),
                           proj, 0);
  g_signal_connect_object (projectable, "structure-changed",
                           G_CALLBACK (ligma_projection_projectable_structure_changed),
                           proj, 0);
  g_signal_connect_object (projectable, "bounds-changed",
                           G_CALLBACK (ligma_projection_projectable_bounds_changed),
                           proj, 0);

  return proj;
}

void
ligma_projection_set_priority (LigmaProjection *proj,
                              gint            priority)
{
  g_return_if_fail (LIGMA_IS_PROJECTION (proj));

  proj->priv->priority = priority;
}

gint
ligma_projection_get_priority (LigmaProjection *proj)
{
  g_return_val_if_fail (LIGMA_IS_PROJECTION (proj), 0);

  return proj->priv->priority;
}

void
ligma_projection_set_priority_rect (LigmaProjection *proj,
                                   gint            x,
                                   gint            y,
                                   gint            w,
                                   gint            h)
{
  g_return_if_fail (LIGMA_IS_PROJECTION (proj));

  proj->priv->priority_rect = *GEGL_RECTANGLE (x, y, w, h);

  ligma_projection_update_priority_rect (proj);
}

void
ligma_projection_stop_rendering (LigmaProjection *proj)
{
  g_return_if_fail (LIGMA_IS_PROJECTION (proj));

  ligma_projection_chunk_render_stop (proj, TRUE);
}

void
ligma_projection_flush (LigmaProjection *proj)
{
  g_return_if_fail (LIGMA_IS_PROJECTION (proj));

  /* Construct in chunks */
  ligma_projection_flush_whenever (proj, FALSE, FALSE);
}

void
ligma_projection_flush_now (LigmaProjection *proj,
                           gboolean        direct)
{
  g_return_if_fail (LIGMA_IS_PROJECTION (proj));

  /* Construct NOW */
  ligma_projection_flush_whenever (proj, TRUE, direct);
}

void
ligma_projection_finish_draw (LigmaProjection *proj)
{
  g_return_if_fail (LIGMA_IS_PROJECTION (proj));

  if (proj->priv->iter)
    {
      ligma_chunk_iterator_set_priority_rect (proj->priv->iter, NULL);

      ligma_tile_handler_validate_begin_validate (proj->priv->validate_handler);

      while (ligma_projection_chunk_render_iteration (proj));

      ligma_tile_handler_validate_end_validate (proj->priv->validate_handler);

      ligma_projection_chunk_render_stop (proj, FALSE);
    }
}


/*  private functions  */

static void
ligma_projection_allocate_buffer (LigmaProjection *proj)
{
  const Babl    *format;
  GeglRectangle  bounding_box;

  if (proj->priv->buffer)
    return;

  format       = ligma_projection_get_format (LIGMA_PICKABLE (proj));
  bounding_box =
    ligma_projectable_get_bounding_box (proj->priv->projectable);

  proj->priv->buffer = gegl_buffer_new (&bounding_box, format);

  proj->priv->validate_handler =
    LIGMA_TILE_HANDLER_VALIDATE (
      ligma_tile_handler_projectable_new (proj->priv->projectable));

  ligma_tile_handler_validate_assign (proj->priv->validate_handler,
                                     proj->priv->buffer);

  g_object_notify (G_OBJECT (proj), "buffer");
}

static void
ligma_projection_free_buffer (LigmaProjection  *proj)
{
  ligma_projection_chunk_render_stop (proj, FALSE);

  g_clear_pointer (&proj->priv->update_region, cairo_region_destroy);

  if (proj->priv->buffer)
    {
      ligma_tile_handler_validate_unassign (proj->priv->validate_handler,
                                           proj->priv->buffer);

      g_clear_object (&proj->priv->buffer);
      g_clear_object (&proj->priv->validate_handler);

      g_object_notify (G_OBJECT (proj), "buffer");
    }
}

static void
ligma_projection_add_update_area (LigmaProjection *proj,
                                 gint            x,
                                 gint            y,
                                 gint            w,
                                 gint            h)
{
  cairo_rectangle_int_t rect;
  GeglRectangle         bounding_box;

  bounding_box = ligma_projectable_get_bounding_box (proj->priv->projectable);

  /*  align the rectangle to the UPDATE_CHUNK_WIDTH x UPDATE_CHUNK_HEIGHT grid,
   *  to decrease the complexity of the update area.
   */
  w = ceil  ((gdouble) (x + w) / LIGMA_PROJECTION_UPDATE_CHUNK_WIDTH ) * LIGMA_PROJECTION_UPDATE_CHUNK_WIDTH;
  h = ceil  ((gdouble) (y + h) / LIGMA_PROJECTION_UPDATE_CHUNK_HEIGHT) * LIGMA_PROJECTION_UPDATE_CHUNK_HEIGHT;
  x = floor ((gdouble) x       / LIGMA_PROJECTION_UPDATE_CHUNK_WIDTH ) * LIGMA_PROJECTION_UPDATE_CHUNK_WIDTH;
  y = floor ((gdouble) y       / LIGMA_PROJECTION_UPDATE_CHUNK_HEIGHT) * LIGMA_PROJECTION_UPDATE_CHUNK_HEIGHT;

  w -= x;
  h -= y;

  if (gegl_rectangle_intersect ((GeglRectangle *) &rect,
                                GEGL_RECTANGLE (x, y, w, h), &bounding_box))
    {
      if (proj->priv->update_region)
        cairo_region_union_rectangle (proj->priv->update_region, &rect);
      else
        proj->priv->update_region = cairo_region_create_rectangle (&rect);
    }
}

static void
ligma_projection_flush_whenever (LigmaProjection *proj,
                                gboolean        now,
                                gboolean        direct)
{
  if (proj->priv->update_region)
    {
      /* Make sure we have a buffer */
      ligma_projection_allocate_buffer (proj);

      if (now)  /* Synchronous */
        {
          gint n_rects = cairo_region_num_rectangles (proj->priv->update_region);
          gint i;

          for (i = 0; i < n_rects; i++)
            {
              cairo_rectangle_int_t rect;

              cairo_region_get_rectangle (proj->priv->update_region,
                                          i, &rect);

              ligma_projection_paint_area (proj,
                                          direct,
                                          rect.x,
                                          rect.y,
                                          rect.width,
                                          rect.height);
            }

          /*  Free the update region  */
          g_clear_pointer (&proj->priv->update_region, cairo_region_destroy);
        }
      else  /* Asynchronous */
        {
          /*  Consumes the update region  */
          ligma_projection_chunk_render_start (proj);
        }
    }
  else if (! now && ! proj->priv->iter && proj->priv->invalidate_preview)
    {
      /* invalidate the preview here since it is constructed from
       * the projection
       */
      proj->priv->invalidate_preview = FALSE;

      ligma_projectable_invalidate_preview (proj->priv->projectable);
    }
}

static void
ligma_projection_update_priority_rect (LigmaProjection *proj)
{
  if (proj->priv->iter)
    {
      GeglRectangle rect;
      GeglRectangle bounding_box;
      gint          off_x, off_y;

      rect = proj->priv->priority_rect;

      ligma_projectable_get_offset (proj->priv->projectable, &off_x, &off_y);
      bounding_box = ligma_projectable_get_bounding_box (proj->priv->projectable);

      /*  subtract the projectable's offsets because the list of update
       *  areas is in tile-pyramid coordinates, but our external API is
       *  always in terms of image coordinates.
       */
      rect.x -= off_x;
      rect.y -= off_y;

      gegl_rectangle_intersect (&rect, &rect, &bounding_box);

      ligma_chunk_iterator_set_priority_rect (proj->priv->iter, &rect);
    }
}

static void
ligma_projection_chunk_render_start (LigmaProjection *proj)
{
  cairo_region_t *region             = proj->priv->update_region;
  gboolean        invalidate_preview = FALSE;

  if (proj->priv->iter)
    {
      region = ligma_chunk_iterator_stop (proj->priv->iter, FALSE);

      proj->priv->iter = NULL;

      if (cairo_region_is_empty (region))
        invalidate_preview = proj->priv->invalidate_preview;

      if (proj->priv->update_region)
        {
          cairo_region_union (region, proj->priv->update_region);

          cairo_region_destroy (proj->priv->update_region);
        }
    }

  proj->priv->update_region = NULL;

  if (region && ! cairo_region_is_empty (region))
    {
      proj->priv->iter = ligma_chunk_iterator_new (region);

      ligma_projection_update_priority_rect (proj);

      if (! proj->priv->idle_id)
        {
          proj->priv->idle_id = g_idle_add_full (
            LIGMA_PRIORITY_PROJECTION_IDLE + proj->priv->priority,
            (GSourceFunc) ligma_projection_chunk_render_callback,
            proj, NULL);
        }
    }
  else
    {
      if (region)
        cairo_region_destroy (region);

      if (proj->priv->idle_id)
        {
          g_source_remove (proj->priv->idle_id);
          proj->priv->idle_id = 0;
        }

      if (invalidate_preview)
        {
          /* invalidate the preview here since it is constructed from
           * the projection
           */
          proj->priv->invalidate_preview = FALSE;

          ligma_projectable_invalidate_preview (proj->priv->projectable);
        }
    }
}

static void
ligma_projection_chunk_render_stop (LigmaProjection *proj,
                                   gboolean        merge)
{
  if (proj->priv->idle_id)
    {
      g_source_remove (proj->priv->idle_id);
      proj->priv->idle_id = 0;
    }

  if (proj->priv->iter)
    {
      if (merge)
        {
          cairo_region_t *region;

          region = ligma_chunk_iterator_stop (proj->priv->iter, FALSE);

          if (proj->priv->update_region)
            {
              cairo_region_union (proj->priv->update_region, region);

              cairo_region_destroy (region);
            }
          else
            {
              proj->priv->update_region = region;
            }
        }
      else
        {
          ligma_chunk_iterator_stop (proj->priv->iter, TRUE);
        }

      proj->priv->iter = NULL;
    }
}

static gboolean
ligma_projection_chunk_render_callback (LigmaProjection *proj)
{
  if (ligma_projection_chunk_render_iteration (proj))
    {
      return G_SOURCE_CONTINUE;
    }
  else
    {
      proj->priv->idle_id = 0;

      return G_SOURCE_REMOVE;
    }
}

static gboolean
ligma_projection_chunk_render_iteration (LigmaProjection *proj)
{
  if (ligma_chunk_iterator_next (proj->priv->iter))
    {
      GeglRectangle rect;

      ligma_tile_handler_validate_begin_validate (proj->priv->validate_handler);

      while (ligma_chunk_iterator_get_rect (proj->priv->iter, &rect))
        {
          ligma_projection_paint_area (proj, TRUE,
                                      rect.x, rect.y, rect.width, rect.height);
        }

      ligma_tile_handler_validate_end_validate (proj->priv->validate_handler);

      /* Still work to do. */
      return TRUE;
    }
  else
    {
      proj->priv->iter = NULL;

      if (proj->priv->invalidate_preview)
        {
          /* invalidate the preview here since it is constructed from
           * the projection
           */
          proj->priv->invalidate_preview = FALSE;

          ligma_projectable_invalidate_preview (proj->priv->projectable);
        }

      /* FINISHED */
      return FALSE;
    }
}

static void
ligma_projection_paint_area (LigmaProjection *proj,
                            gboolean        now,
                            gint            x,
                            gint            y,
                            gint            w,
                            gint            h)
{
  gint          off_x, off_y;
  GeglRectangle bounding_box;
  GeglRectangle rect;

  ligma_projectable_get_offset (proj->priv->projectable, &off_x, &off_y);
  bounding_box = ligma_projectable_get_bounding_box (proj->priv->projectable);

  if (gegl_rectangle_intersect (&rect,
                                GEGL_RECTANGLE (x, y, w, h), &bounding_box))
    {
      if (now)
        {
          ligma_tile_handler_validate_validate (
            proj->priv->validate_handler,
            proj->priv->buffer,
            &rect,
            FALSE, FALSE);
        }
      else
        {
          ligma_tile_handler_validate_invalidate (
            proj->priv->validate_handler,
            &rect);
        }

      /*  add the projectable's offsets because the list of update areas
       *  is in tile-pyramid coordinates, but our external API is always
       *  in terms of image coordinates.
       */
      g_signal_emit (proj, projection_signals[UPDATE], 0,
                     now,
                     rect.x + off_x,
                     rect.y + off_y,
                     rect.width,
                     rect.height);
    }
}


/*  image callbacks  */

static void
ligma_projection_projectable_invalidate (LigmaProjectable *projectable,
                                        gint             x,
                                        gint             y,
                                        gint             w,
                                        gint             h,
                                        LigmaProjection  *proj)
{
  gint off_x, off_y;

  ligma_projectable_get_offset (proj->priv->projectable, &off_x, &off_y);

  /*  subtract the projectable's offsets because the list of update
   *  areas is in tile-pyramid coordinates, but our external API is
   *  always in terms of image coordinates.
   */
  x -= off_x;
  y -= off_y;

  ligma_projection_add_update_area (proj, x, y, w, h);
}

static void
ligma_projection_projectable_flush (LigmaProjectable *projectable,
                                   gboolean         invalidate_preview,
                                   LigmaProjection  *proj)
{
  if (invalidate_preview)
    proj->priv->invalidate_preview = TRUE;

  ligma_projection_flush (proj);
}

static void
ligma_projection_projectable_structure_changed (LigmaProjectable *projectable,
                                               LigmaProjection  *proj)
{
  GeglRectangle bounding_box;

  ligma_projection_free_buffer (proj);

  bounding_box = ligma_projectable_get_bounding_box (projectable);

  ligma_projection_add_update_area (proj,
                                   bounding_box.x,     bounding_box.y,
                                   bounding_box.width, bounding_box.height);
}

static void
ligma_projection_projectable_bounds_changed (LigmaProjectable *projectable,
                                            gint             old_x,
                                            gint             old_y,
                                            LigmaProjection  *proj)
{
  GeglBuffer              *old_buffer = proj->priv->buffer;
  LigmaTileHandlerValidate *old_validate_handler;
  GeglRectangle            old_bounding_box;
  GeglRectangle            bounding_box;
  GeglRectangle            old_bounds;
  GeglRectangle            bounds;
  GeglRectangle            int_bounds;
  gint                     x, y;
  gint                     dx, dy;

  if (! old_buffer)
    {
      ligma_projection_projectable_structure_changed (projectable, proj);

      return;
    }

  old_bounding_box = *gegl_buffer_get_extent (old_buffer);

  ligma_projectable_get_offset (projectable, &x, &y);
  bounding_box = ligma_projectable_get_bounding_box (projectable);

  if (x == old_x && y == old_y &&
      gegl_rectangle_equal (&bounding_box, &old_bounding_box))
    {
      return;
    }

  old_bounds    = old_bounding_box;
  old_bounds.x += old_x;
  old_bounds.y += old_y;

  bounds        = bounding_box;
  bounds.x     += x;
  bounds.y     += y;

  if (! gegl_rectangle_intersect (&int_bounds, &bounds, &old_bounds))
    {
      ligma_projection_projectable_structure_changed (projectable, proj);

      return;
    }

  dx = x - old_x;
  dy = y - old_y;

#if 1
  /* FIXME:  when there's an offset between the new bounds and the old bounds,
   * use ligma_projection_projectable_structure_changed(), instead of copying a
   * shifted version of the old buffer, since the synchronous copy can take a
   * notable amount of time for big buffers, when the offset is such that tiles
   * are not COW-ed.  while ligma_projection_projectable_structure_changed()
   * causes the projection to be re-rendered, which is overall slower, it's
   * done asynchronously.
   *
   * this needs to be improved.
   */
  if (dx || dy)
    {
      ligma_projection_projectable_structure_changed (projectable, proj);

      return;
    }
#endif

  /* reallocate the buffer, and copy the old buffer to the corresponding
   * region of the new buffer.
   */

  ligma_projection_chunk_render_stop (proj, TRUE);

  if (dx == 0 && dy == 0)
    {
      ligma_tile_handler_validate_buffer_set_extent (old_buffer, &bounding_box);
    }
  else
    {
      old_validate_handler = proj->priv->validate_handler;

      proj->priv->buffer           = NULL;
      proj->priv->validate_handler = NULL;

      ligma_projection_allocate_buffer (proj);

      ligma_tile_handler_validate_buffer_copy (
        old_buffer,
        GEGL_RECTANGLE (int_bounds.x - old_x,
                        int_bounds.y - old_y,
                        int_bounds.width,
                        int_bounds.height),
        proj->priv->buffer,
        GEGL_RECTANGLE (int_bounds.x - x,
                        int_bounds.y - y,
                        int_bounds.width,
                        int_bounds.height));

      ligma_tile_handler_validate_unassign (old_validate_handler,
                                           old_buffer);

      g_object_unref (old_validate_handler);
      g_object_unref (old_buffer);
    }

  if (proj->priv->update_region)
    {
      cairo_region_translate (proj->priv->update_region, dx, dy);
      cairo_region_intersect_rectangle (
        proj->priv->update_region,
        (const cairo_rectangle_int_t *) &bounding_box);
    }

  int_bounds.x -= x;
  int_bounds.y -= y;

  if (int_bounds.x > bounding_box.x)
    {
      ligma_projection_add_update_area (proj,
                                       bounding_box.x,
                                       bounding_box.y,
                                       int_bounds.x - bounding_box.x,
                                       bounding_box.height);
    }
  if (int_bounds.y > bounding_box.y)
    {
      ligma_projection_add_update_area (proj,
                                       bounding_box.x,
                                       bounding_box.y,
                                       bounding_box.width,
                                       int_bounds.y - bounding_box.y);
    }
  if (int_bounds.x + int_bounds.width < bounding_box.x + bounding_box.width)
    {
      ligma_projection_add_update_area (proj,
                                       int_bounds.x + int_bounds.width,
                                       bounding_box.y,
                                       bounding_box.x + bounding_box.width -
                                       (int_bounds.x  + int_bounds.width),
                                       bounding_box.height);
    }
  if (int_bounds.y + int_bounds.height < bounding_box.y + bounding_box.height)
    {
      ligma_projection_add_update_area (proj,
                                       bounding_box.x,
                                       int_bounds.y + int_bounds.height,
                                       bounding_box.width,
                                       bounding_box.y + bounding_box.height -
                                       (int_bounds.y  + int_bounds.height));
    }

  proj->priv->invalidate_preview = TRUE;
}
