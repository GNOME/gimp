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
#include <gdk-pixbuf/gdk-pixbuf.h>

#include "libgimpmath/gimpmath.h"

extern "C"
{

#include "paint-types.h"

#include "core/gimptempbuf.h"

#include "gimpbrushcore.h"
#include "gimpbrushcore-loops.h"
#include "gimpbrushcore-kernels.h"


static inline void
rotate_pointers (gulong  **p,
                 guint32   n)
{
  guint32  i;
  gulong  *tmp;

  tmp = p[0];

  for (i = 0; i < n-1; i++)
    p[i] = p[i+1];

  p[i] = tmp;
}

const GimpTempBuf *
gimp_brush_core_subsample_mask (GimpBrushCore     *core,
                                const GimpTempBuf *mask,
                                gdouble            x,
                                gdouble            y)
{
  GimpTempBuf  *dest;
  gdouble       left;
  const guchar *m;
  guchar       *d;
  const gint   *k;
  gint          index1;
  gint          index2;
  gint          dest_offset_x = 0;
  gint          dest_offset_y = 0;
  const gint   *kernel;
  gint          i, j;
  gint          r, s;
  gulong       *accum[KERNEL_HEIGHT];
  gint          offs;
  gint          mask_width  = gimp_temp_buf_get_width  (mask);
  gint          mask_height = gimp_temp_buf_get_height (mask);
  gint          dest_width;
  gint          dest_height;

  while (x < 0)
    x += mask_width;

  left = x - floor (x);
  index1 = (gint) (left * (gdouble) (KERNEL_SUBSAMPLE + 1));

  while (y < 0)
    y += mask_height;

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


  kernel = subsample[index2][index1];

  if (mask == core->last_subsample_brush_mask &&
      ! core->subsample_cache_invalid)
    {
      if (core->subsample_brushes[index2][index1])
        return core->subsample_brushes[index2][index1];
    }
  else
    {
      for (i = 0; i < KERNEL_SUBSAMPLE + 1; i++)
        for (j = 0; j < KERNEL_SUBSAMPLE + 1; j++)
          g_clear_pointer (&core->subsample_brushes[i][j], gimp_temp_buf_unref);

      core->last_subsample_brush_mask = mask;
      core->subsample_cache_invalid   = FALSE;
    }

  dest = gimp_temp_buf_new (mask_width  + 2,
                            mask_height + 2,
                            gimp_temp_buf_get_format (mask));
  gimp_temp_buf_data_clear (dest);

  dest_width  = gimp_temp_buf_get_width  (dest);
  dest_height = gimp_temp_buf_get_height (dest);

  /* Allocate and initialize the accum buffer */
  for (i = 0; i < KERNEL_HEIGHT ; i++)
    accum[i] = g_new0 (gulong, dest_width + 1);

  core->subsample_brushes[index2][index1] = dest;

  m = gimp_temp_buf_get_data (mask);
  for (i = 0; i < mask_height; i++)
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
      d = gimp_temp_buf_get_data (dest) + (i + dest_offset_y) * dest_width;
      for (j = 0; j < dest_width; j++)
        *d++ = (accum[0][j] + 127) / KERNEL_SUM;

      rotate_pointers (accum, KERNEL_HEIGHT);

      memset (accum[KERNEL_HEIGHT - 1], 0, sizeof (gulong) * dest_width);
    }

  /* store the rest of the accum buffer into the dest mask */
  while (i + dest_offset_y < dest_height)
    {
      d = gimp_temp_buf_get_data (dest) + (i + dest_offset_y) * dest_width;
      for (j = 0; j < dest_width; j++)
        *d++ = (accum[0][j] + (KERNEL_SUM / 2)) / KERNEL_SUM;

      rotate_pointers (accum, KERNEL_HEIGHT);
      i++;
    }

  for (i = 0; i < KERNEL_HEIGHT ; i++)
    g_free (accum[i]);

  return dest;
}

/* #define FANCY_PRESSURE */

const GimpTempBuf *
gimp_brush_core_pressurize_mask (GimpBrushCore     *core,
                                 const GimpTempBuf *brush_mask,
                                 gdouble            x,
                                 gdouble            y,
                                 gdouble            pressure)
{
  static guchar      mapi[256];
  const guchar      *source;
  guchar            *dest;
  const GimpTempBuf *subsample_mask;
  gint               i;

  /* Get the raw subsampled mask */
  subsample_mask = gimp_brush_core_subsample_mask (core,
                                                   brush_mask,
                                                   x, y);

  /* Special case pressure = 0.5 */
  if ((gint) (pressure * 100 + 0.5) == 50)
    return subsample_mask;

  g_clear_pointer (&core->pressure_brush, gimp_temp_buf_unref);

  core->pressure_brush =
    gimp_temp_buf_new (gimp_temp_buf_get_width  (brush_mask) + 2,
                       gimp_temp_buf_get_height (brush_mask) + 2,
                       gimp_temp_buf_get_format (brush_mask));
  gimp_temp_buf_data_clear (core->pressure_brush);

#ifdef FANCY_PRESSURE

  /* Create the pressure profile
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
  {
    gdouble  map[256];
    gdouble  ds, s, c;

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
          mapi[i] = (gint) (255 * map[i] / map[255]);
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

        for (i = 0; i < 256; i++)
          mapi[i] = (gint) (255 * (1 - map[i] / map[0]));
      }
  }

#else /* ! FANCY_PRESSURE */

  {
    gdouble j, k;

    j = pressure + pressure;
    k = 0;

    for (i = 0; i < 256; i++)
      {
        if (k > 255)
          mapi[i] = 255;
        else
          mapi[i] = (guchar) k;

        k += j;
      }
  }

#endif /* FANCY_PRESSURE */

  /* Now convert the brush */

  source = gimp_temp_buf_get_data (subsample_mask);
  dest   = gimp_temp_buf_get_data (core->pressure_brush);

  i = gimp_temp_buf_get_width  (subsample_mask) *
      gimp_temp_buf_get_height (subsample_mask);

  while (i--)
    *dest++ = mapi[(*source++)];

  return core->pressure_brush;
}

const GimpTempBuf *
gimp_brush_core_solidify_mask (GimpBrushCore     *core,
                               const GimpTempBuf *brush_mask,
                               gdouble            x,
                               gdouble            y)
{
  GimpTempBuf  *dest;
  const guchar *m;
  gfloat       *d;
  gint          dest_offset_x     = 0;
  gint          dest_offset_y     = 0;
  gint          brush_mask_width  = gimp_temp_buf_get_width  (brush_mask);
  gint          brush_mask_height = gimp_temp_buf_get_height (brush_mask);
  gint          i, j;

  if ((brush_mask_width % 2) == 0)
    {
      while (x < 0)
        x += brush_mask_width;

      if ((x - floor (x)) >= 0.5)
        dest_offset_x++;
    }

  if ((brush_mask_height % 2) == 0)
    {
      while (y < 0)
        y += brush_mask_height;

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
      for (i = 0; i < BRUSH_CORE_SOLID_SUBSAMPLE; i++)
        for (j = 0; j < BRUSH_CORE_SOLID_SUBSAMPLE; j++)
          g_clear_pointer (&core->solid_brushes[i][j], gimp_temp_buf_unref);

      core->last_solid_brush_mask = brush_mask;
      core->solid_cache_invalid   = FALSE;
    }

  dest = gimp_temp_buf_new (brush_mask_width  + 2,
                            brush_mask_height + 2,
                            babl_format ("Y float"));
  gimp_temp_buf_data_clear (dest);

  core->solid_brushes[dest_offset_y][dest_offset_x] = dest;

  m = gimp_temp_buf_get_data (brush_mask);
  d = ((gfloat *) gimp_temp_buf_get_data (dest) +
       ((dest_offset_y + 1) * gimp_temp_buf_get_width (dest) +
        (dest_offset_x + 1)));

  for (i = 0; i < brush_mask_height; i++)
    {
      for (j = 0; j < brush_mask_width; j++)
        *d++ = (*m++) ? 1.0 : 0.0;

      d += 2;
    }

  return dest;
}

} /* extern "C" */
