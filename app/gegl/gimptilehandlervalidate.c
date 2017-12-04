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

#include "gimp-gegl-types.h"

#include "gimptilehandlervalidate.h"


enum
{
  PROP_0,
  PROP_FORMAT,
  PROP_TILE_WIDTH,
  PROP_TILE_HEIGHT,
  PROP_WHOLE_TILE
};


static void     gimp_tile_handler_validate_finalize      (GObject         *object);
static void     gimp_tile_handler_validate_set_property  (GObject         *object,
                                                          guint            property_id,
                                                          const GValue    *value,
                                                          GParamSpec      *pspec);
static void     gimp_tile_handler_validate_get_property  (GObject         *object,
                                                          guint            property_id,
                                                          GValue          *value,
                                                          GParamSpec      *pspec);

static void     gimp_tile_handler_validate_real_validate (GimpTileHandlerValidate *validate,
                                                          const GeglRectangle     *rect,
                                                          const Babl              *format,
                                                          gpointer                 dest_buf,
                                                          gint                     dest_stride);

static gpointer gimp_tile_handler_validate_command       (GeglTileSource  *source,
                                                          GeglTileCommand  command,
                                                          gint             x,
                                                          gint             y,
                                                          gint             z,
                                                          gpointer         data);


G_DEFINE_TYPE (GimpTileHandlerValidate, gimp_tile_handler_validate,
               GEGL_TYPE_TILE_HANDLER)

#define parent_class gimp_tile_handler_validate_parent_class


static void
gimp_tile_handler_validate_class_init (GimpTileHandlerValidateClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->finalize     = gimp_tile_handler_validate_finalize;
  object_class->set_property = gimp_tile_handler_validate_set_property;
  object_class->get_property = gimp_tile_handler_validate_get_property;

  klass->validate            = gimp_tile_handler_validate_real_validate;

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

static GeglTile *
gimp_tile_handler_validate_validate (GeglTileSource *source,
                                     GeglTile       *tile,
                                     gint            x,
                                     gint            y)
{
  GimpTileHandlerValidate *validate = GIMP_TILE_HANDLER_VALIDATE (source);
  cairo_rectangle_int_t    tile_rect;

  if (cairo_region_is_empty (validate->dirty_region))
    return tile;

  tile_rect.x      = x * validate->tile_width;
  tile_rect.y      = y * validate->tile_height;
  tile_rect.width  = validate->tile_width;
  tile_rect.height = validate->tile_height;

  if (validate->whole_tile)
    {
      if (cairo_region_contains_rectangle (validate->dirty_region, &tile_rect)
          != CAIRO_REGION_OVERLAP_OUT)
        {
          gint tile_bpp;
          gint tile_stride;

          if (! tile)
            tile = gegl_tile_handler_create_tile (GEGL_TILE_HANDLER (source),
                                                  x, y, 0);

          cairo_region_subtract_rectangle (validate->dirty_region, &tile_rect);

          tile_bpp    = babl_format_get_bytes_per_pixel (validate->format);
          tile_stride = tile_bpp * validate->tile_width;

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
        }
    }
  else
    {
      cairo_region_t *tile_region = cairo_region_copy (validate->dirty_region);

      cairo_region_intersect_rectangle (tile_region, &tile_rect);

      if (! cairo_region_is_empty (tile_region))
        {
          gint tile_bpp;
          gint tile_stride;
          gint n_rects;
          gint i;

          if (! tile)
            tile = gegl_tile_handler_create_tile (GEGL_TILE_HANDLER (source),
                                                  x, y, 0);

          cairo_region_subtract_rectangle (validate->dirty_region, &tile_rect);

          tile_bpp    = babl_format_get_bytes_per_pixel (validate->format);
          tile_stride = tile_bpp * validate->tile_width;

          gegl_tile_lock (tile);

          n_rects = cairo_region_num_rectangles (tile_region);

#if 0
          g_printerr ("%d chunks\n", n_rects);
#endif

          for (i = 0; i < n_rects; i++)
            {
              cairo_rectangle_int_t blit_rect;

              cairo_region_get_rectangle (tile_region, i, &blit_rect);

              GIMP_TILE_HANDLER_VALIDATE_GET_CLASS (validate)->validate
                (validate,
                 GEGL_RECTANGLE (blit_rect.x,
                                 blit_rect.y,
                                 blit_rect.width,
                                 blit_rect.height),
                 validate->format,
                 gegl_tile_get_data (tile) +
                 (blit_rect.y % validate->tile_height) * tile_stride +
                 (blit_rect.x % validate->tile_width)  * tile_bpp,
                 tile_stride);
            }

          gegl_tile_unlock (tile);
        }

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
  GimpTileHandlerValidate *validate = GIMP_TILE_HANDLER_VALIDATE (source);
  gpointer                 retval;

  validate->max_z = MAX (validate->max_z, z);

  retval = gegl_tile_handler_source_command (source, command, x, y, z, data);

  if (command == GEGL_TILE_GET && z == 0)
    retval = gimp_tile_handler_validate_validate (source, retval, x, y);

  return retval;
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

  if (validate->max_z > 0)
    {
      GeglTileSource *source  = GEGL_TILE_SOURCE (validate);
      gint            tile_x1 = rect->x / validate->tile_width;
      gint            tile_y1 = rect->y / validate->tile_height;
      gint            tile_x2 = (rect->x + rect->width  - 1) /
                                validate->tile_width + 1;
      gint            tile_y2 = (rect->y + rect->height - 1) /
                                validate->tile_height + 1;
      gint            tile_x;
      gint            tile_y;
      gint            tile_z;

      for (tile_z = 1; tile_z <= validate->max_z; tile_z++)
        {
          tile_y1 = tile_y1 / 2;
          tile_y2 = (tile_y2 + 1) / 2;
          tile_x1 = tile_x1 / 2;
          tile_x2 = (tile_x2 + 1) / 2;

          for (tile_y = tile_y1; tile_y < tile_y2; tile_y++)
            for (tile_x = tile_x1; tile_x < tile_x2; tile_x++)
              gegl_tile_source_void (source, tile_x, tile_y, tile_z);
        }
    }
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
