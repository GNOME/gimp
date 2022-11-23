/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ligmabrush-mipmap.c
 * Copyright (C) 2020 Ell
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

#include <string.h>

#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gegl.h>

#include "libligmamath/ligmamath.h"

extern "C"
{

#include "core-types.h"

#include "ligmabrush.h"
#include "ligmabrush-mipmap.h"
#include "ligmabrush-private.h"
#include "ligmatempbuf.h"

} /* extern "C" */


#define PIXELS_PER_THREAD \
  (/* each thread costs as much as */ 64.0 * 64.0 /* pixels */)

#define LIGMA_BRUSH_MIPMAP(brush, mipmaps, x, y) \
  ((*(mipmaps))[(y) * (brush)->priv->n_horz_mipmaps + (x)])


/*  local function prototypes  */

static void                ligma_brush_mipmap_clear          (LigmaBrush           *brush,
                                                             LigmaTempBuf       ***mipmaps);

static const LigmaTempBuf * ligma_brush_mipmap_get            (LigmaBrush           *brush,
                                                             const LigmaTempBuf   *source,
                                                             LigmaTempBuf       ***mipmaps,
                                                             gdouble             *scale_x,
                                                             gdouble             *scale_y);

static LigmaTempBuf       * ligma_brush_mipmap_downscale      (const LigmaTempBuf   *source);
static LigmaTempBuf       * ligma_brush_mipmap_downscale_horz (const LigmaTempBuf   *source);
static LigmaTempBuf       * ligma_brush_mipmap_downscale_vert (const LigmaTempBuf   *source);


/*  private functions  */

static void
ligma_brush_mipmap_clear (LigmaBrush     *brush,
                         LigmaTempBuf ***mipmaps)
{
  if (*mipmaps)
    {
      gint i;

      for (i = 0;
           i < brush->priv->n_horz_mipmaps * brush->priv->n_vert_mipmaps;
           i++)
        {
          g_clear_pointer (&(*mipmaps)[i], ligma_temp_buf_unref);
        }

      g_clear_pointer (mipmaps, g_free);
    }
}

static const LigmaTempBuf *
ligma_brush_mipmap_get (LigmaBrush           *brush,
                       const LigmaTempBuf   *source,
                       LigmaTempBuf       ***mipmaps,
                       gdouble             *scale_x,
                       gdouble             *scale_y)
{
  gint x;
  gint y;
  gint i;

  if (! source)
    return NULL;

  if (! *mipmaps)
    {
      gint width  = ligma_temp_buf_get_width  (source);
      gint height = ligma_temp_buf_get_height (source);

      brush->priv->n_horz_mipmaps = floor (log (width)  / M_LN2) + 1;
      brush->priv->n_vert_mipmaps = floor (log (height) / M_LN2) + 1;

      *mipmaps = g_new0 (LigmaTempBuf *, brush->priv->n_horz_mipmaps *
                                        brush->priv->n_vert_mipmaps);

      LIGMA_BRUSH_MIPMAP (brush, mipmaps, 0, 0) = ligma_temp_buf_ref (source);
    }

  x = floor (SAFE_CLAMP (log (1.0 / MAX (*scale_x, 0.0)) / M_LN2,
                         0, brush->priv->n_horz_mipmaps - 1));
  y = floor (SAFE_CLAMP (log (1.0 / MAX (*scale_y, 0.0)) / M_LN2,
                         0, brush->priv->n_vert_mipmaps - 1));

  *scale_x *= pow (2.0, x);
  *scale_y *= pow (2.0, y);

  if (LIGMA_BRUSH_MIPMAP (brush, mipmaps, x, y))
    return LIGMA_BRUSH_MIPMAP (brush, mipmaps, x, y);

  g_return_val_if_fail (x >= 0 || y >= 0, NULL);

  for (i = 1; i <= x + y; i++)
    {
      gint u = x - i;
      gint v = y;

      if (u < 0)
        {
          v += u;
          u  = 0;
        }

      while (u <= x && v >= 0)
        {
          if (LIGMA_BRUSH_MIPMAP (brush, mipmaps, u, v))
            {
              for (; x - u > y - v; u++)
                {
                  LIGMA_BRUSH_MIPMAP (brush, mipmaps, u + 1, v) =
                    ligma_brush_mipmap_downscale_horz (
                      LIGMA_BRUSH_MIPMAP (brush, mipmaps, u, v));
                }

              for (; y - v > x - u; v++)
                {
                  LIGMA_BRUSH_MIPMAP (brush, mipmaps, u, v + 1) =
                    ligma_brush_mipmap_downscale_vert (
                      LIGMA_BRUSH_MIPMAP (brush, mipmaps, u, v));
                }

              for (; u < x; u++, v++)
                {
                  LIGMA_BRUSH_MIPMAP (brush, mipmaps, u + 1, v + 1) =
                    ligma_brush_mipmap_downscale (
                      LIGMA_BRUSH_MIPMAP (brush, mipmaps, u, v));
                }

              return LIGMA_BRUSH_MIPMAP (brush, mipmaps, u, v);
            }

          u++;
          v--;
        }
    }

  g_return_val_if_reached (NULL);
}

template <class T>
struct MipmapTraits;

template <>
struct MipmapTraits<guint8>
{
  static guint8
  mix (guint8 a,
       guint8 b)
  {
    return ((guint32) a + (guint32) b + 1) / 2;
  }

  static guint8
  mix (guint8 a,
       guint8 b,
       guint8 c,
       guint8 d)
  {
    return ((guint32) a + (guint32) b + (guint32) c + (guint32) d + 2) / 4;
  }
};

template <>
struct MipmapTraits<gfloat>
{
  static gfloat
  mix (gfloat a,
       gfloat b)
  {
    return (a + b) / 2.0;
  }

  static gfloat
  mix (gfloat a,
       gfloat b,
       gfloat c,
       gfloat d)
  {
    return (a + b + c + d) / 4.0;
  }
};

template <class T,
          gint  N>
struct MipmapAlgorithms
{
  static LigmaTempBuf *
  downscale (const LigmaTempBuf *source)
  {
    LigmaTempBuf *destination;
    gint         width  = ligma_temp_buf_get_width  (source);
    gint         height = ligma_temp_buf_get_height (source);

    width  /= 2;
    height /= 2;

    destination = ligma_temp_buf_new (width, height,
                                     ligma_temp_buf_get_format (source));

    gegl_parallel_distribute_area (
      GEGL_RECTANGLE (0, 0, width, height), PIXELS_PER_THREAD,
      [=] (const GeglRectangle *area)
      {
        const T *src0        = (const T *) ligma_temp_buf_get_data (source);
        T       *dest0       = (T       *) ligma_temp_buf_get_data (destination);
        gint     src_stride  = N * ligma_temp_buf_get_width (source);
        gint     dest_stride = N * ligma_temp_buf_get_width (destination);
        gint     y;

        src0  += 2 * (area->y * src_stride  + N * area->x);
        dest0 +=      area->y * dest_stride + N * area->x;

        for (y = 0; y < area->height; y++)
          {
            const T *src  = src0;
            T       *dest = dest0;
            gint     x;

            for (x = 0; x < area->width; x++)
              {
                gint c;

                for (c = 0; c < N; c++)
                  {
                    dest[c] = MipmapTraits<T>::mix (src[c],
                                                    src[N + c],
                                                    src[src_stride + c],
                                                    src[src_stride + N + c]);
                  }

                src  += 2 * N;
                dest += N;
              }

            src0  += 2 * src_stride;
            dest0 += dest_stride;
          }
      });

    return destination;
  }

  static LigmaTempBuf *
  downscale_horz (const LigmaTempBuf *source)
  {
    LigmaTempBuf *destination;
    gint         width  = ligma_temp_buf_get_width  (source);
    gint         height = ligma_temp_buf_get_height (source);

    width /= 2;

    destination = ligma_temp_buf_new (width, height,
                                     ligma_temp_buf_get_format (source));

    gegl_parallel_distribute_range (
      height, PIXELS_PER_THREAD / width,
      [=] (gint offset,
           gint size)
      {
        const T *src0        = (const T *) ligma_temp_buf_get_data (source);
        T       *dest0       = (T       *) ligma_temp_buf_get_data (destination);
        gint     src_stride  = N * ligma_temp_buf_get_width (source);
        gint     dest_stride = N * ligma_temp_buf_get_width (destination);
        gint     y;

        src0  += offset * src_stride;
        dest0 += offset * dest_stride;

        for (y = 0; y < size; y++)
          {
            const T *src  = src0;
            T       *dest = dest0;
            gint     x;

            for (x = 0; x < width; x++)
              {
                gint c;

                for (c = 0; c < N; c++)
                  dest[c] = MipmapTraits<T>::mix (src[c], src[N + c]);

                src  += 2 * N;
                dest += N;
              }

            src0  += src_stride;
            dest0 += dest_stride;
          }
      });

    return destination;
  }

  static LigmaTempBuf *
  downscale_vert (const LigmaTempBuf *source)
  {
    LigmaTempBuf *destination;
    gint         width  = ligma_temp_buf_get_width  (source);
    gint         height = ligma_temp_buf_get_height (source);

    height /= 2;

    destination = ligma_temp_buf_new (width, height,
                                     ligma_temp_buf_get_format (source));

    gegl_parallel_distribute_range (
      width, PIXELS_PER_THREAD / height,
      [=] (gint offset,
           gint size)
      {
        const T *src0        = (const T *) ligma_temp_buf_get_data (source);
        T       *dest0       = (T       *) ligma_temp_buf_get_data (destination);
        gint     src_stride  = N * ligma_temp_buf_get_width (source);
        gint     dest_stride = N * ligma_temp_buf_get_width (destination);
        gint     x;

        src0  += offset * N;
        dest0 += offset * N;

        for (x = 0; x < size; x++)
          {
            const T *src  = src0;
            T       *dest = dest0;
            gint     y;

            for (y = 0; y < height; y++)
              {
                gint c;

                for (c = 0; c < N; c++)
                  dest[c] = MipmapTraits<T>::mix (src[c], src[src_stride + c]);

                src  += 2 * src_stride;
                dest += dest_stride;
              }

            src0  += N;
            dest0 += N;
          }
      });

    return destination;
  }
};

template <class Func>
static LigmaTempBuf *
ligma_brush_mipmap_dispatch (const LigmaTempBuf *source,
                            Func               func)
{
  const Babl *format = ligma_temp_buf_get_format (source);
  const Babl *type;
  gint        n_components;

  type         = babl_format_get_type (format, 0);
  n_components = babl_format_get_n_components (format);

  if (type == babl_type ("u8"))
    {
      switch (n_components)
        {
        case 1:
          return func (MipmapAlgorithms<guint8, 1> ());

        case 3:
          return func (MipmapAlgorithms<guint8, 3> ());
        }
    }
  else if (type == babl_type ("float"))
    {
      switch (n_components)
        {
        case 1:
          return func (MipmapAlgorithms<gfloat, 1> ());

        case 3:
          return func (MipmapAlgorithms<gfloat, 3> ());
        }
    }

  g_return_val_if_reached (NULL);
}

static LigmaTempBuf *
ligma_brush_mipmap_downscale (const LigmaTempBuf *source)
{
  return ligma_brush_mipmap_dispatch (
    source,
    [&] (auto algorithms)
    {
      return algorithms.downscale (source);
    });
}

static LigmaTempBuf *
ligma_brush_mipmap_downscale_horz (const LigmaTempBuf *source)
{
  return ligma_brush_mipmap_dispatch (
    source,
    [&] (auto algorithms)
    {
      return algorithms.downscale_horz (source);
    });
}

static LigmaTempBuf *
ligma_brush_mipmap_downscale_vert (const LigmaTempBuf *source)
{
  return ligma_brush_mipmap_dispatch (
    source,
    [&] (auto algorithms)
    {
      return algorithms.downscale_vert (source);
    });
}


/*  public functions  */

void
ligma_brush_mipmap_clear (LigmaBrush *brush)
{
  ligma_brush_mipmap_clear (brush, &brush->priv->mask_mipmaps);
  ligma_brush_mipmap_clear (brush, &brush->priv->pixmap_mipmaps);
}

const LigmaTempBuf *
ligma_brush_mipmap_get_mask (LigmaBrush *brush,
                            gdouble   *scale_x,
                            gdouble   *scale_y)
{
  return ligma_brush_mipmap_get (brush,
                                brush->priv->mask,
                                &brush->priv->mask_mipmaps,
                                scale_x, scale_y);
}

const LigmaTempBuf *
ligma_brush_mipmap_get_pixmap (LigmaBrush *brush,
                              gdouble   *scale_x,
                              gdouble   *scale_y)
{
  return ligma_brush_mipmap_get (brush,
                                brush->priv->pixmap,
                                &brush->priv->pixmap_mipmaps,
                                scale_x, scale_y);
}

gsize
ligma_brush_mipmap_get_memsize (LigmaBrush *brush)
{
  gsize memsize = 0;

  if (brush->priv->mask_mipmaps)
    {
      gint i;

      for (i = 1;
           i < brush->priv->n_horz_mipmaps * brush->priv->n_vert_mipmaps;
           i++)
        {
          memsize += ligma_temp_buf_get_memsize (brush->priv->mask_mipmaps[i]);
        }
    }

  if (brush->priv->pixmap_mipmaps)
    {
      gint i;

      for (i = 1;
           i < brush->priv->n_horz_mipmaps * brush->priv->n_vert_mipmaps;
           i++)
        {
          memsize += ligma_temp_buf_get_memsize (brush->priv->pixmap_mipmaps[i]);
        }
    }

  return memsize;
}
