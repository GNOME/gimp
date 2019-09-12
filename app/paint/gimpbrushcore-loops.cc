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

#include <string.h>

#include <gegl.h>
#include <gdk-pixbuf/gdk-pixbuf.h>

#include "libgimpmath/gimpmath.h"

extern "C"
{

#include "paint-types.h"

#include "core/gimptempbuf.h"

#include "gimpbrushcore.h"
#include "gimpbrushcore-loops.h"

} /* extern "C" */

#include "gimpbrushcore-kernels.h"


#define PIXELS_PER_THREAD \
  (/* each thread costs as much as */ 64.0 * 64.0 /* pixels */)

#define EPSILON 1e-6


static void
clear_edges (GimpTempBuf *buf,
             gint         top,
             gint         bottom,
             gint         left,
             gint         right)
{
  guchar     *data;
  const Babl *format = gimp_temp_buf_get_format (buf);
  gint        bpp    = babl_format_get_bytes_per_pixel (format);
  gint        width  = gimp_temp_buf_get_width  (buf);
  gint        height = gimp_temp_buf_get_height (buf);

  if (top + bottom >= height || left + right >= width)
    {
      gimp_temp_buf_data_clear (buf);

      return;
    }

  data = gimp_temp_buf_get_data (buf);

  memset (data, 0, (top * width + left) * bpp);

  if (left + right)
    {
      gint stride = width * bpp;
      gint size   = (left + right) * bpp;
      gint y;

      data = gimp_temp_buf_get_data (buf) +
             ((top + 1) * width - right) * bpp;

      for (y = top; y < height - bottom - 1; y++)
        {
          memset (data, 0, size);

          data += stride;
        }
    }

  data = gimp_temp_buf_get_data (buf) +
         ((height - bottom) * width - right) * bpp;

  memset (data, 0, (bottom * width + right) * bpp);
}

template <class T>
static inline void
rotate_pointers (T    **p,
                 gint   n)
{
  T    *tmp;
  gint  i;

  tmp = p[0];

  for (i = 0; i < n - 1; i++)
    p[i] = p[i + 1];

  p[i] = tmp;
}

template <class T>
static void
gimp_brush_core_subsample_mask_impl (const GimpTempBuf *mask,
                                     GimpTempBuf       *dest,
                                     gint               dest_offset_x,
                                     gint               dest_offset_y,
                                     gint               index1,
                                     gint               index2)
{
  using value_type  = typename Subsample<T>::value_type;
  using kernel_type = typename Subsample<T>::kernel_type;
  using accum_type  = typename Subsample<T>::accum_type;

  Subsample<T>       subsample;
  const kernel_type *kernel      = subsample.kernel[index2][index1];
  gint               mask_width  = gimp_temp_buf_get_width  (mask);
  gint               mask_height = gimp_temp_buf_get_height (mask);
  gint               dest_width  = gimp_temp_buf_get_width  (dest);
  gint               dest_height = gimp_temp_buf_get_height (dest);

  gegl_parallel_distribute_range (
    mask_height, PIXELS_PER_THREAD / mask_width,
    [=] (gint y, gint height)
    {
      const value_type  *m;
      value_type        *d;
      const kernel_type *k;
      gint               y0;
      gint               i, j;
      gint               r, s;
      gint               offs;
      accum_type        *accum[KERNEL_HEIGHT];

      /* Allocate and initialize the accum buffer */
      for (i = 0; i < KERNEL_HEIGHT ; i++)
        accum[i] = gegl_scratch_new0 (accum_type, dest_width + 1);

      y0 = MAX (y - (KERNEL_HEIGHT - 1), 0);

      m = (const value_type *) gimp_temp_buf_get_data (mask) +
          y0 * mask_width;

      for (i = y0; i < y; i++)
        {
          for (j = 0; j < mask_width; j++)
            {
              k = kernel + KERNEL_WIDTH * (y - i);
              for (r = y - i; r < KERNEL_HEIGHT; r++)
                {
                  offs = j + dest_offset_x;
                  s = KERNEL_WIDTH;
                  while (s--)
                    accum[r][offs++] += *m * *k++;
                }
              m++;
            }

          rotate_pointers (accum, KERNEL_HEIGHT);
        }

      for (i = y; i < y + height; i++)
        {
          for (j = 0; j < mask_width; j++)
            {
              k = kernel;
              for (r = 0; r < KERNEL_HEIGHT; r++)
                {
                  offs = j + dest_offset_x;
                  s = KERNEL_WIDTH;
                  while (s--)
                    accum[r][offs++] += *m * *k++;
                }
              m++;
            }

          /* store the accum buffer into the destination mask */
          d = (value_type *) gimp_temp_buf_get_data (dest) +
              (i + dest_offset_y) * dest_width;
          for (j = 0; j < dest_width; j++)
            *d++ = subsample.round (accum[0][j]);

          rotate_pointers (accum, KERNEL_HEIGHT);

          memset (accum[KERNEL_HEIGHT - 1], 0,
                  sizeof (accum_type) * dest_width);
        }

      if (y + height == mask_height)
        {
          /* store the rest of the accum buffer into the dest mask */
          while (i + dest_offset_y < dest_height)
            {
              d = (value_type *) gimp_temp_buf_get_data (dest) +
                  (i + dest_offset_y) * dest_width;
              for (j = 0; j < dest_width; j++)
                *d++ = subsample.round (accum[0][j]);

              rotate_pointers (accum, KERNEL_HEIGHT);
              i++;
            }
        }

      for (i = KERNEL_HEIGHT - 1; i >= 0; i--)
        gegl_scratch_free (accum[i]);
    });
}

const GimpTempBuf *
gimp_brush_core_subsample_mask (GimpBrushCore     *core,
                                const GimpTempBuf *mask,
                                gdouble            x,
                                gdouble            y)
{
  GimpTempBuf *dest;
  const Babl  *mask_format;
  gdouble      left;
  gint         index1;
  gint         index2;
  gint         dest_offset_x = 0;
  gint         dest_offset_y = 0;
  gint         mask_width  = gimp_temp_buf_get_width  (mask);
  gint         mask_height = gimp_temp_buf_get_height (mask);

  left = x - floor (x);
  index1 = (gint) (left * (gdouble) (KERNEL_SUBSAMPLE + 1));

  left = y - floor (y);
  index2 = (gint) (left * (gdouble) (KERNEL_SUBSAMPLE + 1));

  if ((mask_width % 2) == 0)
    {
      index1 += KERNEL_SUBSAMPLE >> 1;

      if (index1 > KERNEL_SUBSAMPLE)
        {
          index1 -= KERNEL_SUBSAMPLE + 1;
          dest_offset_x = 1;
        }
    }

  if ((mask_height % 2) == 0)
    {
      index2 += KERNEL_SUBSAMPLE >> 1;

      if (index2 > KERNEL_SUBSAMPLE)
        {
          index2 -= KERNEL_SUBSAMPLE + 1;
          dest_offset_y = 1;
        }
    }

  if (mask == core->last_subsample_brush_mask &&
      ! core->subsample_cache_invalid)
    {
      if (core->subsample_brushes[index2][index1])
        return core->subsample_brushes[index2][index1];
    }
  else
    {
      gint i, j;

      for (i = 0; i < KERNEL_SUBSAMPLE + 1; i++)
        for (j = 0; j < KERNEL_SUBSAMPLE + 1; j++)
          g_clear_pointer (&core->subsample_brushes[i][j], gimp_temp_buf_unref);

      core->last_subsample_brush_mask = mask;
      core->subsample_cache_invalid   = FALSE;
    }

  mask_format = gimp_temp_buf_get_format (mask);

  dest = gimp_temp_buf_new (mask_width  + 2,
                            mask_height + 2,
                            mask_format);
  clear_edges (dest, dest_offset_y, 0, 0, 0);

  core->subsample_brushes[index2][index1] = dest;

  if (mask_format == babl_format ("Y u8"))
    {
      gimp_brush_core_subsample_mask_impl<guchar> (mask, dest,
                                                   dest_offset_x, dest_offset_y,
                                                   index1, index2);
    }
  else if (mask_format == babl_format ("Y float"))
    {
      gimp_brush_core_subsample_mask_impl<gfloat> (mask, dest,
                                                   dest_offset_x, dest_offset_y,
                                                   index1, index2);
    }
  else
    {
      g_warn_if_reached ();
    }

  return dest;
}

/* The simple pressure profile
 *
 * It is: I'(I) = MIN (2 * pressure * I, 1)
 */
class SimplePressure
{
  gfloat scale;

public:
  SimplePressure (gdouble pressure)
  {
    scale = 2.0 * pressure;
  }

  guchar
  operator () (guchar x) const
  {
    gint v = RINT (scale * x);

    return MIN (v, 255);
  }

  gfloat
  operator () (gfloat x) const
  {
    gfloat v = scale * x;

    return MIN (v, 1.0f);
  }

  template <class T>
  T
  operator () (T x) const = delete;
};

/* The fancy pressure profile
 *
 * It is: I'(I) = tanh (20 * (pressure - 0.5) * I)           : pressure > 0.5
 *        I'(I) = 1 - tanh (20 * (0.5 - pressure) * (1 - I)) : pressure < 0.5
 *
 * It looks like:
 *
 *    low pressure      medium pressure     high pressure
 *
 *         |                   /                  --
 *         |                  /                  /
 *        /                  /                  |
 *      --                  /                   |
 */
class FancyPressure
{
  gfloat map[257];

public:
  FancyPressure (gdouble pressure)
  {
    gdouble ds, s, c;
    gint    i;

    ds = (pressure - 0.5) * (20.0 / 256.0);
    s  = 0;
    c  = 1.0;

    if (ds > 0)
      {
        for (i = 0; i < 256; i++)
          {
            map[i] = s / c;
            s += c * ds;
            c += s * ds;
          }

        for (i = 0; i < 256; i++)
          map[i] = map[i] / map[255];
      }
    else
      {
        ds = -ds;

        for (i = 255; i >= 0; i--)
          {
            map[i] = s / c;
            s += c * ds;
            c += s * ds;
          }

        for (i = 255; i >= 0; i--)
          map[i] = 1.0f - map[i] / map[0];
      }

    map[256] = map[255];
  }

  guchar
  operator () (guchar x) const
  {
    return RINT (255.0f * map[x]);
  }

  gfloat
  operator () (gfloat x) const
  {
    gint   i;
    gfloat f;

    x *= 255.0f;

    i = floorf (x);
    f = x - i;

    return map[i] + (map[i + 1] - map[i]) * f;
  }

  template <class T>
  T
  operator () (T x) const = delete;
};

template <class T>
class CachedPressure
{
  T map[T (~0) + 1];

public:
  template <class Pressure>
  CachedPressure (Pressure pressure)
  {
    gint i;

    for (i = 0; i < (gint) G_N_ELEMENTS (map); i++)
      map[i] = pressure (T (i));
  }

  T
  operator () (T x) const
  {
    return map[x];
  }

  template <class U>
  U
  operator () (U x) const = delete;
};

template <class T,
          class Pressure>
void
gimp_brush_core_pressurize_mask_impl (const GimpTempBuf *mask,
                                      GimpTempBuf       *dest,
                                      Pressure           pressure)
{
  gegl_parallel_distribute_range (
    gimp_temp_buf_get_width (mask) * gimp_temp_buf_get_height (mask),
    PIXELS_PER_THREAD,
    [=] (gint offset, gint size)
    {
      const T *m;
      T       *d;
      gint     i;

      m = (const T *) gimp_temp_buf_get_data (mask) + offset;
      d = (      T *) gimp_temp_buf_get_data (dest) + offset;

      for (i = 0; i < size; i++)
        *d++ = pressure (*m++);
    });
}

/* #define FANCY_PRESSURE */

const GimpTempBuf *
gimp_brush_core_pressurize_mask (GimpBrushCore     *core,
                                 const GimpTempBuf *brush_mask,
                                 gdouble            x,
                                 gdouble            y,
                                 gdouble            pressure)
{
  const GimpTempBuf *subsample_mask;
  const Babl        *subsample_mask_format;

  /* Get the raw subsampled mask */
  subsample_mask = gimp_brush_core_subsample_mask (core,
                                                   brush_mask,
                                                   x, y);

  /* Special case pressure = 0.5 */
  if (fabs (pressure - 0.5) <= EPSILON)
    return subsample_mask;

  g_clear_pointer (&core->pressure_brush, gimp_temp_buf_unref);

  subsample_mask_format = gimp_temp_buf_get_format (subsample_mask);

  core->pressure_brush =
    gimp_temp_buf_new (gimp_temp_buf_get_width  (brush_mask) + 2,
                       gimp_temp_buf_get_height (brush_mask) + 2,
                       subsample_mask_format);

#ifdef FANCY_PRESSURE
  using Pressure = FancyPressure;
#else
  using Pressure = SimplePressure;
#endif

  if (subsample_mask_format == babl_format ("Y u8"))
    {
      gimp_brush_core_pressurize_mask_impl<guchar> (subsample_mask,
                                                    core->pressure_brush,
                                                    CachedPressure<guchar> (
                                                      Pressure (pressure)));
    }
  else if (subsample_mask_format == babl_format ("Y float"))
    {
      gimp_brush_core_pressurize_mask_impl<gfloat> (subsample_mask,
                                                    core->pressure_brush,
                                                    Pressure (pressure));
    }
  else
    {
      g_warn_if_reached ();
    }

  return core->pressure_brush;
}

template <class T>
static void
gimp_brush_core_solidify_mask_impl (const GimpTempBuf *mask,
                                    GimpTempBuf       *dest,
                                    gint               dest_offset_x,
                                    gint               dest_offset_y)
{
  gint mask_width  = gimp_temp_buf_get_width  (mask);
  gint mask_height = gimp_temp_buf_get_height (mask);
  gint dest_width  = gimp_temp_buf_get_width  (dest);

  gegl_parallel_distribute_area (
    GEGL_RECTANGLE (0, 0, mask_width, mask_height),
    PIXELS_PER_THREAD,
    [=] (const GeglRectangle *area)
    {
      const T *m;
      gfloat  *d;
      gint     i, j;

      m = (const T *) gimp_temp_buf_get_data (mask) +
          area->y * mask_width + area->x;
      d = ((gfloat *) gimp_temp_buf_get_data (dest) +
           ((dest_offset_y + 1 + area->y) * dest_width +
            (dest_offset_x + 1 + area->x)));

      for (i = 0; i < area->height; i++)
        {
          for (j = 0; j < area->width; j++)
            *d++ = (*m++) ? 1.0 : 0.0;

          m += mask_width - area->width;
          d += dest_width - area->width;
        }
    });
}

const GimpTempBuf *
gimp_brush_core_solidify_mask (GimpBrushCore     *core,
                               const GimpTempBuf *brush_mask,
                               gdouble            x,
                               gdouble            y)
{
  GimpTempBuf  *dest;
  const Babl   *brush_mask_format;
  gint          dest_offset_x     = 0;
  gint          dest_offset_y     = 0;
  gint          brush_mask_width  = gimp_temp_buf_get_width  (brush_mask);
  gint          brush_mask_height = gimp_temp_buf_get_height (brush_mask);

  if ((brush_mask_width % 2) == 0)
    {
      if (x < 0.0)
        x = fmod (x, brush_mask_width) + brush_mask_width;

      if ((x - floor (x)) >= 0.5)
        dest_offset_x++;
    }

  if ((brush_mask_height % 2) == 0)
    {
      if (y < 0.0)
        y = fmod (y, brush_mask_height) + brush_mask_height;

      if ((y - floor (y)) >= 0.5)
        dest_offset_y++;
    }

  if (! core->solid_cache_invalid &&
      brush_mask == core->last_solid_brush_mask)
    {
      if (core->solid_brushes[dest_offset_y][dest_offset_x])
        return core->solid_brushes[dest_offset_y][dest_offset_x];
    }
  else
    {
      gint i, j;

      for (i = 0; i < BRUSH_CORE_SOLID_SUBSAMPLE; i++)
        for (j = 0; j < BRUSH_CORE_SOLID_SUBSAMPLE; j++)
          g_clear_pointer (&core->solid_brushes[i][j], gimp_temp_buf_unref);

      core->last_solid_brush_mask = brush_mask;
      core->solid_cache_invalid   = FALSE;
    }

  brush_mask_format = gimp_temp_buf_get_format (brush_mask);

  dest = gimp_temp_buf_new (brush_mask_width  + 2,
                            brush_mask_height + 2,
                            babl_format ("Y float"));
  clear_edges (dest,
               1 + dest_offset_y, 1 - dest_offset_y,
               1 + dest_offset_x, 1 - dest_offset_x);

  core->solid_brushes[dest_offset_y][dest_offset_x] = dest;

  if (brush_mask_format == babl_format ("Y u8"))
    {
      gimp_brush_core_solidify_mask_impl<guchar> (brush_mask, dest,
                                                  dest_offset_x, dest_offset_y);
    }
  else if (brush_mask_format == babl_format ("Y float"))
    {
      gimp_brush_core_solidify_mask_impl<gfloat> (brush_mask, dest,
                                                  dest_offset_x, dest_offset_y);
    }
  else
    {
      g_warn_if_reached ();
    }

  return dest;
}
