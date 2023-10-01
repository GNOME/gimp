/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * gimpbrush.c
 * Copyright (C) 2023 Jehan
 *
 * This library is free software: you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library.  If not, see
 * <https://www.gnu.org/licenses/>.
 */

#include "config.h"

#include "gimp.h"

#include "gimpbrush.h"

struct _GimpBrush
{
  GimpResource  parent_instance;

  /* Native size buffers of the brush contents. */
  GeglBuffer   *buffer;
  GeglBuffer   *mask;
};

G_DEFINE_TYPE (GimpBrush, gimp_brush, GIMP_TYPE_RESOURCE);

static void         gimp_brush_finalize        (GObject    *object);
static void         gimp_brush_get_data        (GimpBrush  *brush);
static GeglBuffer * gimp_brush_scale           (GeglBuffer *buffer,
                                                gint        max_width,
                                                gint        max_height);
static const Babl * gimp_brush_data_get_format (gint        bpp,
                                                gboolean    with_alpha);


static void  gimp_brush_class_init (GimpBrushClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->finalize = gimp_brush_finalize;
}

static void  gimp_brush_init (GimpBrush *brush)
{
  brush->buffer = NULL;
  brush->mask   = NULL;
}

static void
gimp_brush_finalize (GObject *object)
{
  GimpBrush *brush = GIMP_BRUSH (object);

  g_clear_object (&brush->buffer);
  g_clear_object (&brush->mask);

  G_OBJECT_CLASS (gimp_brush_parent_class)->finalize (object);
}


/**
 * gimp_brush_get_buffer:
 * @brush:      a [class@Brush].
 * @max_width:  a maximum width for the returned buffer.
 * @max_height: a maximum height for the returned buffer.
 * @format:     an optional Babl format.
 *
 * Gets pixel data of the brush within the bounding box specified by @max_width
 * and @max_height. The data will be scaled down so that it fits within this
 * size without changing its ratio. If the brush is smaller than this size to
 * begin with, it will not be scaled up.
 *
 * If @max_width or @max_height are %NULL, the buffer is returned in the brush's
 * native size.
 *
 * When the brush is parametric or a raster mask, only the mask (as returned by
 * [method@Gimp.Brush.get_mask]) will be set. The returned buffer will be NULL.
 *
 * Make sure you called [func@Gegl.init] before calling any function using
 * `GEGL`.
 *
 * Returns: (transfer full): a [class@Gegl.Buffer] of %NULL if the brush is parametric
 *                           or mask only.
*/
GeglBuffer *
gimp_brush_get_buffer (GimpBrush  *brush,
                       gint        max_width,
                       gint        max_height,
                       const Babl *format)
{
  gimp_brush_get_data (brush);

  if (brush->buffer == NULL)
    return NULL;

  if (max_width == 0 || max_height == 0 ||
      (gegl_buffer_get_width (brush->buffer) <= max_width &&
       gegl_buffer_get_height (brush->buffer) <= max_height))
    return gegl_buffer_dup (brush->buffer);

  return gimp_brush_scale (brush->buffer, max_width, max_height);
}

/**
 * gimp_brush_get_mask:
 * @brush:      a [class@Brush].
 * @max_width:  a maximum width for the returned buffer.
 * @max_height: a maximum height for the returned buffer.
 * @format:     an optional Babl format.
 *
 * Gets mask data of the brush within the bounding box specified by @max_width
 * and @max_height. The data will be scaled down so that it fits within this
 * size without changing its ratio. If the brush is smaller than this size to
 * begin with, it will not be scaled up.
 *
 * If @max_width or @max_height are %NULL, the buffer is returned in the brush's
 * native size.
 *
 * Make sure you called [func@Gegl.init] before calling any function using
 * `GEGL`.
 *
 * Returns: (transfer full): a [class@Gegl.Buffer] representing the @brush mask.
*/
GeglBuffer *
gimp_brush_get_mask (GimpBrush  *brush,
                     gint        max_width,
                     gint        max_height,
                     const Babl *format)
{
  gimp_brush_get_data (brush);

  g_return_val_if_fail (brush->mask != NULL, NULL);

  if (max_width == 0 || max_height == 0 ||
      (gegl_buffer_get_width (brush->mask) <= max_width &&
       gegl_buffer_get_height (brush->mask) <= max_height))
    return gegl_buffer_dup (brush->mask);

  return gimp_brush_scale (brush->mask, max_width, max_height);
}

static void
gimp_brush_get_data (GimpBrush *brush)
{
  gint          width;
  gint          height;
  gint          mask_bpp;
  GBytes       *mask_bytes;
  gint          color_bpp;
  GBytes       *color_bytes;
  const guchar *mask;
  gsize         mask_size;
  const guchar *color;
  gsize         color_size;

  /* We check the mask because the buffer might be NULL.
   *
   * This check assumes that the brush contents doesn't change, which is not a
   * perfect assumption. We could maybe add a PDB call which would return
   * the new brush data only if it changed since last call (which can be
   * verified with some kind of internal runtime version to pass from one call
   * to another). TODO
   */
  if (brush->mask != NULL)
    return;

  g_clear_object (&brush->buffer);
  g_clear_object (&brush->mask);

  _gimp_brush_get_pixels (brush, &width, &height,
                          &mask_bpp, &mask_bytes,
                          &color_bpp, &color_bytes);

  mask  = g_bytes_unref_to_data (mask_bytes, &mask_size);
  color = g_bytes_unref_to_data (color_bytes, &color_size);

  brush->mask = gegl_buffer_linear_new_from_data ((const gpointer) mask,
                                                  gimp_brush_data_get_format (mask_bpp, FALSE),
                                                  GEGL_RECTANGLE (0, 0, width, height),
                                                  0, g_free, NULL);
  if (color_bpp > 0)
    {
      GeglBufferIterator *gi;
      GeglBuffer         *buffer;

      buffer = gegl_buffer_linear_new_from_data ((const gpointer) color,
                                                 gimp_brush_data_get_format (color_bpp, FALSE),
                                                 GEGL_RECTANGLE (0, 0, width, height),
                                                 0, g_free, NULL);
      brush->buffer = gegl_buffer_new (GEGL_RECTANGLE (0, 0, width, height),
                                       gimp_brush_data_get_format (color_bpp, TRUE));

      gi = gegl_buffer_iterator_new (brush->buffer, NULL, 0, NULL,
                                     GEGL_ACCESS_WRITE, GEGL_ABYSS_NONE, 3);
      gegl_buffer_iterator_add (gi, buffer, GEGL_RECTANGLE (0, 0, width, height),
                                0, NULL, GEGL_ACCESS_READ, GEGL_ABYSS_NONE);
      gegl_buffer_iterator_add (gi, brush->mask, GEGL_RECTANGLE (0, 0, width, height),
                                0, NULL, GEGL_ACCESS_READ, GEGL_ABYSS_NONE);
      while (gegl_buffer_iterator_next (gi))
        {
          guint8 *out    = gi->items[0].data;
          guint8 *color  = gi->items[1].data;
          guint8 *alpha  = gi->items[2].data;

          for (gint i = 0; i < gi->length; i++)
            {
              gint c;

              for (c = 0; c < color_bpp; c++)
                out[c] = color[c];

              out[c] = alpha[0];

              out   += color_bpp + 1;
              color += color_bpp;
              alpha += 1;
            }
        }

      g_object_unref (buffer);
    }
}

static GeglBuffer *
gimp_brush_scale (GeglBuffer *buffer,
                  gint        max_width,
                  gint        max_height)
{
  GeglBuffer *scaled = NULL;
  GeglNode   *graph;
  GeglNode   *source;
  GeglNode   *op;
  GeglNode   *sink;
  gdouble     width;
  gdouble     height;
  gdouble     scale;

  height = (gdouble) max_height;
  width  = (gdouble) gegl_buffer_get_width (buffer) / gegl_buffer_get_height (buffer) * height;
  if (width > (gdouble) max_width)
    {
      width  = (gdouble) max_width;
      height = (gdouble) gegl_buffer_get_height (buffer) / gegl_buffer_get_width (buffer) * width;
    }
  scale = width / gegl_buffer_get_width (buffer);

  graph = gegl_node_new ();
  source = gegl_node_new_child (graph,
                                "operation", "gegl:buffer-source",
                                "buffer",    buffer,
                                NULL);
  op = gegl_node_new_child (graph,
                            "operation",   "gegl:scale-ratio",
                            "origin-x",     0.0,
                            "origin-y",     0.0,
                            "sampler",      GIMP_INTERPOLATION_LINEAR,
                            "abyss-policy", GEGL_ABYSS_CLAMP,
                            "x",            scale,
                            "y",            scale,
                            NULL);
  sink = gegl_node_new_child (graph,
                              "operation", "gegl:buffer-sink",
                              "buffer",    &scaled,
                              "format",    gegl_buffer_get_format (buffer),
                              NULL);
  gegl_node_link_many (source, op, sink, NULL);
  gegl_node_process (sink);

  g_object_unref (graph);

  return scaled;
}

static const Babl *
gimp_brush_data_get_format (gint     bpp,
                            gboolean with_alpha)
{
  /* It's an ugly way to determine the proper format but gimp_brush_get_pixels()
   * doesn't give more info.
   */
  switch (bpp)
    {
    case 1:
      if (! with_alpha)
        return babl_format ("Y' u8");
      /* fallthrough */
    case 2:
      return babl_format ("Y'A u8");
    case 3:
      if (! with_alpha)
        return babl_format ("R'G'B' u8");
      /* fallthrough */
    case 4:
      return babl_format ("R'G'B'A u8");
    default:
      g_return_val_if_reached (NULL);
    }

  g_return_val_if_reached (NULL);
}
