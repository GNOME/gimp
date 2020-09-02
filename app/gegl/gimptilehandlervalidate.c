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

#include <cairo.h>
#include <gio/gio.h>
#include <gegl.h>

#include "libgimpbase/gimpbase.h"

#include "gimp-gegl-types.h"

#include "core/gimpchunkiterator.h"

#include "gimp-gegl-loops.h"
#include "gimp-gegl-utils.h"
#include "gimptilehandlervalidate.h"


enum
{
  INVALIDATED,
  LAST_SIGNAL
};

enum
{
  PROP_0,
  PROP_FORMAT,
  PROP_TILE_WIDTH,
  PROP_TILE_HEIGHT,
  PROP_WHOLE_TILE
};


static void     gimp_tile_handler_validate_finalize             (GObject         *object);
static void     gimp_tile_handler_validate_set_property         (GObject         *object,
                                                                 guint            property_id,
                                                                 const GValue    *value,
                                                                 GParamSpec      *pspec);
static void     gimp_tile_handler_validate_get_property         (GObject         *object,
                                                                 guint            property_id,
                                                                 GValue          *value,
                                                                 GParamSpec      *pspec);

static void     gimp_tile_handler_validate_real_begin_validate  (GimpTileHandlerValidate *validate);
static void     gimp_tile_handler_validate_real_end_validate    (GimpTileHandlerValidate *validate);
static void     gimp_tile_handler_validate_real_validate        (GimpTileHandlerValidate *validate,
                                                                 const GeglRectangle     *rect,
                                                                 const Babl              *format,
                                                                 gpointer                 dest_buf,
                                                                 gint                     dest_stride);
static void     gimp_tile_handler_validate_real_validate_buffer (GimpTileHandlerValidate *validate,
                                                                 const GeglRectangle     *rect,
                                                                 GeglBuffer              *buffer);

static gpointer gimp_tile_handler_validate_command              (GeglTileSource  *source,
                                                                 GeglTileCommand  command,
                                                                 gint             x,
                                                                 gint             y,
                                                                 gint             z,
                                                                 gpointer         data);


G_DEFINE_TYPE (GimpTileHandlerValidate, gimp_tile_handler_validate,
               GEGL_TYPE_TILE_HANDLER)

#define parent_class gimp_tile_handler_validate_parent_class

static guint gimp_tile_handler_validate_signals[LAST_SIGNAL];


static void
gimp_tile_handler_validate_class_init (GimpTileHandlerValidateClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  gimp_tile_handler_validate_signals[INVALIDATED] =
    g_signal_new ("invalidated",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (GimpTileHandlerValidateClass, invalidated),
                  NULL, NULL,
                  g_cclosure_marshal_VOID__BOXED,
                  G_TYPE_NONE, 1,
                  GEGL_TYPE_RECTANGLE);

  object_class->finalize     = gimp_tile_handler_validate_finalize;
  object_class->set_property = gimp_tile_handler_validate_set_property;
  object_class->get_property = gimp_tile_handler_validate_get_property;

  klass->begin_validate      = gimp_tile_handler_validate_real_begin_validate;
  klass->end_validate        = gimp_tile_handler_validate_real_end_validate;
  klass->validate            = gimp_tile_handler_validate_real_validate;
  klass->validate_buffer     = gimp_tile_handler_validate_real_validate_buffer;

  g_object_class_install_property (object_class, PROP_FORMAT,
                                   g_param_spec_pointer ("format", NULL, NULL,
                                                         GIMP_PARAM_READWRITE));

  g_object_class_install_property (object_class, PROP_TILE_WIDTH,
                                   g_param_spec_int ("tile-width", NULL, NULL,
                                                     1, G_MAXINT, 1,
                                                     GIMP_PARAM_READWRITE |
                                                     G_PARAM_CONSTRUCT));

  g_object_class_install_property (object_class, PROP_TILE_HEIGHT,
                                   g_param_spec_int ("tile-height", NULL, NULL,
                                                     1, G_MAXINT, 1,
                                                     GIMP_PARAM_READWRITE |
                                                     G_PARAM_CONSTRUCT));

  g_object_class_install_property (object_class, PROP_WHOLE_TILE,
                                   g_param_spec_boolean ("whole-tile", NULL, NULL,
                                                         FALSE,
                                                         GIMP_PARAM_READWRITE |
                                                         G_PARAM_CONSTRUCT));
}

static void
gimp_tile_handler_validate_init (GimpTileHandlerValidate *validate)
{
  GeglTileSource *source = GEGL_TILE_SOURCE (validate);

  source->command = gimp_tile_handler_validate_command;

  validate->dirty_region = cairo_region_create ();
}

static void
gimp_tile_handler_validate_finalize (GObject *object)
{
  GimpTileHandlerValidate *validate = GIMP_TILE_HANDLER_VALIDATE (object);

  g_clear_object (&validate->graph);
  g_clear_pointer (&validate->dirty_region, cairo_region_destroy);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
gimp_tile_handler_validate_set_property (GObject      *object,
                                         guint         property_id,
                                         const GValue *value,
                                         GParamSpec   *pspec)
{
  GimpTileHandlerValidate *validate = GIMP_TILE_HANDLER_VALIDATE (object);

  switch (property_id)
    {
    case PROP_FORMAT:
      validate->format = g_value_get_pointer (value);
      break;
    case PROP_TILE_WIDTH:
      validate->tile_width = g_value_get_int (value);
      break;
    case PROP_TILE_HEIGHT:
      validate->tile_height = g_value_get_int (value);
      break;
    case PROP_WHOLE_TILE:
      validate->whole_tile = g_value_get_boolean (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_tile_handler_validate_get_property (GObject    *object,
                                         guint       property_id,
                                         GValue     *value,
                                         GParamSpec *pspec)
{
  GimpTileHandlerValidate *validate = GIMP_TILE_HANDLER_VALIDATE (object);

  switch (property_id)
    {
    case PROP_FORMAT:
      g_value_set_pointer (value, (gpointer) validate->format);
      break;
    case PROP_TILE_WIDTH:
      g_value_set_int (value, validate->tile_width);
      break;
    case PROP_TILE_HEIGHT:
      g_value_set_int (value, validate->tile_height);
      break;
    case PROP_WHOLE_TILE:
      g_value_set_boolean (value, validate->whole_tile);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_tile_handler_validate_real_begin_validate (GimpTileHandlerValidate *validate)
{
  validate->suspend_validate++;
}

static void
gimp_tile_handler_validate_real_end_validate (GimpTileHandlerValidate *validate)
{
  validate->suspend_validate--;
}

static void
gimp_tile_handler_validate_real_validate (GimpTileHandlerValidate *validate,
                                          const GeglRectangle     *rect,
                                          const Babl              *format,
                                          gpointer                 dest_buf,
                                          gint                     dest_stride)
{
#if 0
  g_printerr ("validating at %d %d %d %d\n",
              rect.x,
              rect.y,
              rect.width,
              rect.height);
#endif

  gegl_node_blit (validate->graph, 1.0, rect, format,
                  dest_buf, dest_stride,
                  GEGL_BLIT_DEFAULT);
}

static void
gimp_tile_handler_validate_real_validate_buffer (GimpTileHandlerValidate *validate,
                                                 const GeglRectangle     *rect,
                                                 GeglBuffer              *buffer)
{
  GimpTileHandlerValidateClass *klass;

  klass = GIMP_TILE_HANDLER_VALIDATE_GET_CLASS (validate);

  if (klass->validate == gimp_tile_handler_validate_real_validate)
    {
      gegl_node_blit_buffer (validate->graph, buffer, rect, 0,
                             GEGL_ABYSS_NONE);
    }
  else
    {
      const Babl *format = gegl_buffer_get_format (buffer);
      gpointer    data;
      gint        stride;

      data = gegl_buffer_linear_open (buffer, rect, &stride, format);

      klass->validate (validate, rect, format, data, stride);

      gegl_buffer_linear_close (buffer, data);
    }
}

static GeglTile *
gimp_tile_handler_validate_validate_tile (GeglTileSource *source,
                                          gint            x,
                                          gint            y)
{
  GimpTileHandlerValidate *validate = GIMP_TILE_HANDLER_VALIDATE (source);
  GeglTile                *tile;
  cairo_rectangle_int_t    tile_rect;
  cairo_region_overlap_t   overlap;

  if (validate->suspend_validate ||
      cairo_region_is_empty (validate->dirty_region))
    {
      return gegl_tile_handler_source_command (source,
                                               GEGL_TILE_GET, x, y, 0, NULL);
    }

  tile_rect.x      = x * validate->tile_width;
  tile_rect.y      = y * validate->tile_height;
  tile_rect.width  = validate->tile_width;
  tile_rect.height = validate->tile_height;

  overlap = cairo_region_contains_rectangle (validate->dirty_region,
                                             &tile_rect);

  if (overlap == CAIRO_REGION_OVERLAP_OUT)
    {
      return gegl_tile_handler_source_command (source,
                                               GEGL_TILE_GET, x, y, 0, NULL);
    }

  if (overlap == CAIRO_REGION_OVERLAP_IN || validate->whole_tile)
    {
      gint tile_bpp;
      gint tile_stride;

      cairo_region_subtract_rectangle (validate->dirty_region, &tile_rect);

      tile_bpp    = babl_format_get_bytes_per_pixel (validate->format);
      tile_stride = tile_bpp * validate->tile_width;

      tile = gegl_tile_handler_get_source_tile (GEGL_TILE_HANDLER (source),
                                                x, y, 0, FALSE);

      gimp_tile_handler_validate_begin_validate (validate);

      gegl_tile_lock (tile);

      GIMP_TILE_HANDLER_VALIDATE_GET_CLASS (validate)->validate
        (validate,
         GEGL_RECTANGLE (tile_rect.x,
                         tile_rect.y,
                         tile_rect.width,
                         tile_rect.height),
         validate->format,
         gegl_tile_get_data (tile),
         tile_stride);

      gegl_tile_unlock (tile);

      gimp_tile_handler_validate_end_validate (validate);
    }
  else
    {
      cairo_region_t *tile_region;
      gint            tile_bpp;
      gint            tile_stride;
      gint            n_rects;
      gint            i;

      tile_region = cairo_region_copy (validate->dirty_region);
      cairo_region_intersect_rectangle (tile_region, &tile_rect);

      cairo_region_subtract_rectangle (validate->dirty_region, &tile_rect);

      tile_bpp    = babl_format_get_bytes_per_pixel (validate->format);
      tile_stride = tile_bpp * validate->tile_width;

      tile = gegl_tile_handler_source_command (source,
                                               GEGL_TILE_GET, x, y, 0, NULL);

      if (! tile)
        {
          tile = gegl_tile_handler_create_tile (GEGL_TILE_HANDLER (source),
                                                x, y, 0);

          memset (gegl_tile_get_data (tile),
                  0, tile_stride * validate->tile_height);
        }

      gimp_tile_handler_validate_begin_validate (validate);

      gegl_tile_lock (tile);

      n_rects = cairo_region_num_rectangles (tile_region);

#if 0
      g_printerr ("%d chunks\n", n_rects);
#endif

      for (i = 0; i < n_rects; i++)
        {
          cairo_rectangle_int_t blit_rect;
          gint                  tile_x;
          gint                  tile_y;

          cairo_region_get_rectangle (tile_region, i, &blit_rect);

          tile_x = blit_rect.x % validate->tile_width;
          if (tile_x < 0) tile_x += validate->tile_width;

          tile_y = blit_rect.y % validate->tile_height;
          if (tile_y < 0) tile_y += validate->tile_height;

          GIMP_TILE_HANDLER_VALIDATE_GET_CLASS (validate)->validate
            (validate,
             GEGL_RECTANGLE (blit_rect.x,
                             blit_rect.y,
                             blit_rect.width,
                             blit_rect.height),
             validate->format,
             gegl_tile_get_data (tile) +
             tile_y * tile_stride      +
             tile_x * tile_bpp,
             tile_stride);
        }

      gegl_tile_unlock (tile);

      gimp_tile_handler_validate_end_validate (validate);

      cairo_region_destroy (tile_region);
    }

  return tile;
}

static gpointer
gimp_tile_handler_validate_command (GeglTileSource  *source,
                                    GeglTileCommand  command,
                                    gint             x,
                                    gint             y,
                                    gint             z,
                                    gpointer         data)
{
  if (command == GEGL_TILE_GET && z == 0)
    return gimp_tile_handler_validate_validate_tile (source, x, y);

  return gegl_tile_handler_source_command (source, command, x, y, z, data);
}


/*  public functions  */

GeglTileHandler *
gimp_tile_handler_validate_new (GeglNode *graph)
{
  GimpTileHandlerValidate *validate;

  g_return_val_if_fail (GEGL_IS_NODE (graph), NULL);

  validate = g_object_new (GIMP_TYPE_TILE_HANDLER_VALIDATE, NULL);

  validate->graph = g_object_ref (graph);

  return GEGL_TILE_HANDLER (validate);
}

void
gimp_tile_handler_validate_assign (GimpTileHandlerValidate *validate,
                                   GeglBuffer              *buffer)
{
  g_return_if_fail (GIMP_IS_TILE_HANDLER_VALIDATE (validate));
  g_return_if_fail (GEGL_IS_BUFFER (buffer));
  g_return_if_fail (gimp_tile_handler_validate_get_assigned (buffer) == NULL);

  gegl_buffer_add_handler (buffer, validate);

  g_object_get (buffer,
                "format",      &validate->format,
                "tile-width",  &validate->tile_width,
                "tile-height", &validate->tile_height,
                NULL);

  g_object_set_data (G_OBJECT (buffer),
                     "gimp-tile-handler-validate", validate);
}

void
gimp_tile_handler_validate_unassign (GimpTileHandlerValidate *validate,
                                     GeglBuffer              *buffer)
{
  g_return_if_fail (GIMP_IS_TILE_HANDLER_VALIDATE (validate));
  g_return_if_fail (GEGL_IS_BUFFER (buffer));
  g_return_if_fail (gimp_tile_handler_validate_get_assigned (buffer) == validate);

  g_object_set_data (G_OBJECT (buffer),
                     "gimp-tile-handler-validate", NULL);

  gegl_buffer_remove_handler (buffer, validate);
}

GimpTileHandlerValidate *
gimp_tile_handler_validate_get_assigned (GeglBuffer *buffer)
{
  g_return_val_if_fail (GEGL_IS_BUFFER (buffer), NULL);

  return g_object_get_data (G_OBJECT (buffer),
                            "gimp-tile-handler-validate");
}

void
gimp_tile_handler_validate_invalidate (GimpTileHandlerValidate *validate,
                                       const GeglRectangle     *rect)
{
  g_return_if_fail (GIMP_IS_TILE_HANDLER_VALIDATE (validate));
  g_return_if_fail (rect != NULL);

  cairo_region_union_rectangle (validate->dirty_region,
                                (cairo_rectangle_int_t *) rect);

  gegl_tile_handler_damage_rect (GEGL_TILE_HANDLER (validate), rect);

  g_signal_emit (validate, gimp_tile_handler_validate_signals[INVALIDATED],
                 0, rect, NULL);
}

void
gimp_tile_handler_validate_undo_invalidate (GimpTileHandlerValidate *validate,
                                            const GeglRectangle     *rect)
{
  g_return_if_fail (GIMP_IS_TILE_HANDLER_VALIDATE (validate));
  g_return_if_fail (rect != NULL);

  cairo_region_subtract_rectangle (validate->dirty_region,
                                   (cairo_rectangle_int_t *) rect);
}

void
gimp_tile_handler_validate_begin_validate (GimpTileHandlerValidate *validate)
{
  g_return_if_fail (GIMP_IS_TILE_HANDLER_VALIDATE (validate));

  if (validate->validating++ == 0)
    GIMP_TILE_HANDLER_VALIDATE_GET_CLASS (validate)->begin_validate (validate);
}

void
gimp_tile_handler_validate_end_validate (GimpTileHandlerValidate *validate)
{
  g_return_if_fail (GIMP_IS_TILE_HANDLER_VALIDATE (validate));
  g_return_if_fail (validate->validating > 0);

  if (--validate->validating == 0)
    GIMP_TILE_HANDLER_VALIDATE_GET_CLASS (validate)->end_validate (validate);
}

void
gimp_tile_handler_validate_validate (GimpTileHandlerValidate *validate,
                                     GeglBuffer              *buffer,
                                     const GeglRectangle     *rect,
                                     gboolean                 intersect,
                                     gboolean                 chunked)
{
  GimpTileHandlerValidateClass *klass;
  cairo_region_t               *region = NULL;

  g_return_if_fail (GIMP_IS_TILE_HANDLER_VALIDATE (validate));
  g_return_if_fail (gimp_tile_handler_validate_get_assigned (buffer) ==
                    validate);

  klass = GIMP_TILE_HANDLER_VALIDATE_GET_CLASS (validate);

  if (! rect)
    rect = gegl_buffer_get_extent (buffer);

  if (intersect)
    {
      region = cairo_region_copy (validate->dirty_region);

      cairo_region_intersect_rectangle (region,
                                        (const cairo_rectangle_int_t *) rect);
    }
  else if (chunked)
    {
      region = cairo_region_create_rectangle (
        (const cairo_rectangle_int_t *) rect);
    }

  if (region)
    {
      if (! cairo_region_is_empty (region))
        {
          gimp_tile_handler_validate_begin_validate (validate);

          if (chunked)
            {
              GimpChunkIterator *iter;

              iter   = gimp_chunk_iterator_new (region);
              region = NULL;

              while (gimp_chunk_iterator_next (iter))
                {
                  GeglRectangle blit_rect;

                  while (gimp_chunk_iterator_get_rect (iter, &blit_rect))
                    klass->validate_buffer (validate, &blit_rect, buffer);
                }
            }
          else
            {
              gint n_rects;
              gint i;

              n_rects = cairo_region_num_rectangles (region);

              for (i = 0; i < n_rects; i++)
                {
                  cairo_rectangle_int_t blit_rect;

                  cairo_region_get_rectangle (region, i, &blit_rect);

                  klass->validate_buffer (validate,
                                          (const GeglRectangle *) &blit_rect,
                                          buffer);
                }
              }

          gimp_tile_handler_validate_end_validate (validate);

          cairo_region_subtract_rectangle (
            validate->dirty_region,
            (const cairo_rectangle_int_t *) rect);
        }

      g_clear_pointer (&region, cairo_region_destroy);
    }
  else
    {
      gimp_tile_handler_validate_begin_validate (validate);

      klass->validate_buffer (validate, rect, buffer);

      gimp_tile_handler_validate_end_validate (validate);

      cairo_region_subtract_rectangle (
            validate->dirty_region,
            (const cairo_rectangle_int_t *) rect);
    }
}

gboolean
gimp_tile_handler_validate_buffer_set_extent (GeglBuffer          *buffer,
                                              const GeglRectangle *extent)
{
  GimpTileHandlerValidate *validate;

  g_return_val_if_fail (GEGL_IS_BUFFER (buffer), FALSE);
  g_return_val_if_fail (extent != NULL, FALSE);

  validate = gimp_tile_handler_validate_get_assigned (buffer);

  g_return_val_if_fail (validate != NULL, FALSE);

  validate->suspend_validate++;

  if (gimp_gegl_buffer_set_extent (buffer, extent))
    {
      validate->suspend_validate--;

      cairo_region_intersect_rectangle (validate->dirty_region,
                                        (const cairo_rectangle_int_t *) extent);

      return TRUE;
    }

  validate->suspend_validate--;

  return FALSE;
}

void
gimp_tile_handler_validate_buffer_copy (GeglBuffer          *src_buffer,
                                        const GeglRectangle *src_rect,
                                        GeglBuffer          *dst_buffer,
                                        const GeglRectangle *dst_rect)
{
  GimpTileHandlerValidate *src_validate;
  GimpTileHandlerValidate *dst_validate;
  GeglRectangle            real_src_rect;
  GeglRectangle            real_dst_rect;

  g_return_if_fail (GEGL_IS_BUFFER (src_buffer));
  g_return_if_fail (GEGL_IS_BUFFER (dst_buffer));
  g_return_if_fail (src_rect != dst_rect);

  src_validate = gimp_tile_handler_validate_get_assigned (src_buffer);
  dst_validate = gimp_tile_handler_validate_get_assigned (dst_buffer);

  g_return_if_fail (dst_validate != NULL);

  if (! src_rect)
    src_rect = gegl_buffer_get_extent (src_buffer);

  if (! dst_rect)
    dst_rect = src_rect;

  real_src_rect = *src_rect;

  gegl_rectangle_intersect (&real_dst_rect,
                            dst_rect, gegl_buffer_get_extent (dst_buffer));

  real_src_rect.x      += real_dst_rect.x - dst_rect->x;
  real_src_rect.y      += real_dst_rect.y - dst_rect->y;
  real_src_rect.width  -= real_dst_rect.x - dst_rect->x;
  real_src_rect.height -= real_dst_rect.y - dst_rect->y;

  real_src_rect.width  = CLAMP (real_src_rect.width,  0, real_dst_rect.width);
  real_src_rect.height = CLAMP (real_src_rect.height, 0, real_dst_rect.height);

  /* temporarily remove the source buffer's validate handler, so that
   * gegl_buffer_copy() can use fast tile copying, using the TILE_COPY command.
   * currently, gegl only uses TILE_COPY when the source buffer has no user-
   * provided tile handlers.
   */
  if (src_validate)
    {
      g_object_ref (src_validate);

      gimp_tile_handler_validate_unassign (src_validate, src_buffer);
    }

  dst_validate->suspend_validate++;

  gimp_gegl_buffer_copy (src_buffer, &real_src_rect, GEGL_ABYSS_NONE,
                         dst_buffer, &real_dst_rect);

  dst_validate->suspend_validate--;

  if (src_validate)
    {
      gimp_tile_handler_validate_assign (src_validate, src_buffer);

      g_object_unref (src_validate);
    }

  cairo_region_subtract_rectangle (dst_validate->dirty_region,
                                   (cairo_rectangle_int_t *) &real_dst_rect);

  if (src_validate)
    {
      if (real_src_rect.x == real_dst_rect.x &&
          real_src_rect.y == real_dst_rect.y &&
          gegl_rectangle_equal (&real_src_rect,
                                gegl_buffer_get_extent (src_buffer)))
        {
          cairo_region_union (dst_validate->dirty_region,
                              src_validate->dirty_region);
        }
      else if (cairo_region_contains_rectangle (
                 src_validate->dirty_region,
                 (cairo_rectangle_int_t *) &real_src_rect) !=
               CAIRO_REGION_OVERLAP_OUT)
        {
          cairo_region_t *region;

          region = cairo_region_copy (src_validate->dirty_region);

          if (! gegl_rectangle_equal (&real_src_rect,
                                      gegl_buffer_get_extent (src_buffer)))
            {
              cairo_region_intersect_rectangle (
                region, (cairo_rectangle_int_t *) &real_src_rect);
            }

          cairo_region_translate (region,
                                  real_dst_rect.x - real_src_rect.x,
                                  real_dst_rect.y - real_src_rect.y);

          if (cairo_region_is_empty (dst_validate->dirty_region))
            {
              cairo_region_destroy (dst_validate->dirty_region);

              dst_validate->dirty_region = region;
            }
          else
            {
              cairo_region_union (dst_validate->dirty_region, region);

              cairo_region_destroy (region);
            }
        }
    }
}
