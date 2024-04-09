/*
 * DDS GIMP plugin
 *
 * Copyright (C) 2004-2012 Shawn Kirst <skirst@gmail.com>,
 * with parts (C) 2003 Arne Reuter <homepage@arnereuter.de> where specified.
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

#include <libgimp/gimp.h>

#include <libgimp/stdplugins-intl.h>

#include "endian_rw.h"
#include "imath.h"
#include "misc.h"


/*
 * Decoding Functions
 */

static inline gfloat
saturate (gfloat a)
{
  if (a < 0) a = 0;
  if (a > 1) a = 1;
  return a;
}

void
decode_ycocg (GimpDrawable *drawable)
{
  GeglBuffer   *buffer;
  const Babl   *format;
  guchar       *data;
  guint         num_pixels;
  guint         i, w, h;
  const gfloat  offset = 0.5f * 256.0f / 255.0f;
  gfloat        Y, Co, Cg;
  gfloat        R, G, B;

  buffer = gimp_drawable_get_buffer (drawable);

  format = babl_format ("R'G'B'A u8");

  w = gegl_buffer_get_width  (buffer);
  h = gegl_buffer_get_height (buffer);
  num_pixels = w * h;

  data = g_malloc (num_pixels * 4);

  gegl_buffer_get (buffer, GEGL_RECTANGLE (0, 0, w, h), 1.0, format, data,
                   GEGL_AUTO_ROWSTRIDE, GEGL_ABYSS_NONE);

  /* Translators: Do not translate YCoCg, it's the name of a colorspace */
  gimp_progress_init (_("Decoding YCoCg pixels..."));

  for (i = 0; i < num_pixels; ++i)
    {
      Y  = (gfloat) data[4 * i + 3] / 255.0f;
      Co = (gfloat) data[4 * i + 0] / 255.0f;
      Cg = (gfloat) data[4 * i + 1] / 255.0f;

      /* convert YCoCg to RGB */
      Co -= offset;
      Cg -= offset;

      R = saturate (Y + Co - Cg);
      G = saturate (Y + Cg);
      B = saturate (Y - Co - Cg);

      /* copy new alpha from blue */
      data[4 * i + 3] = data[4 * i + 2];

      data[4 * i + 0] = (guchar) (R * 255.0f);
      data[4 * i + 1] = (guchar) (G * 255.0f);
      data[4 * i + 2] = (guchar) (B * 255.0f);

      if ((i & 0x7fff) == 0)
        gimp_progress_update ((gdouble) i / (gdouble) num_pixels);
    }

  gegl_buffer_set (buffer, GEGL_RECTANGLE (0, 0, w, h), 0, format, data,
                   GEGL_AUTO_ROWSTRIDE);

  gimp_progress_update (1.0);

  gegl_buffer_flush (buffer);

  gimp_drawable_update (drawable, 0, 0, w, h);

  g_free (data);

  g_object_unref (buffer);
}

void
decode_ycocg_scaled (GimpDrawable *drawable)
{
  GeglBuffer   *buffer;
  const Babl   *format;
  guchar       *data;
  guint         num_pixels;
  guint         i, w, h;
  const gfloat  offset = 0.5f * 256.0f / 255.0f;
  gfloat        Y, Co, Cg;
  gfloat        R, G, B, s;

  buffer = gimp_drawable_get_buffer (drawable);

  format = babl_format ("R'G'B'A u8");

  w = gegl_buffer_get_width  (buffer);
  h = gegl_buffer_get_height (buffer);
  num_pixels = w * h;

  data = g_malloc (num_pixels * 4);

  gegl_buffer_get (buffer, GEGL_RECTANGLE (0, 0, w, h), 1.0, format, data,
                   GEGL_AUTO_ROWSTRIDE, GEGL_ABYSS_NONE);
                   
  /* Translators: Do not translate YCoCg, it's the name of a colorspace */
  gimp_progress_init (_("Decoding YCoCg (scaled) pixels..."));

  for (i = 0; i < num_pixels; ++i)
    {
      Y  = (gfloat) data[4 * i + 3] / 255.0f;
      Co = (gfloat) data[4 * i + 0] / 255.0f;
      Cg = (gfloat) data[4 * i + 1] / 255.0f;
      s  = (gfloat) data[4 * i + 2] / 255.0f;

      /* convert YCoCg to RGB */
      s = 1.0f / ((255.0f / 8.0f) * s + 1.0f);

      Co = (Co - offset) * s;
      Cg = (Cg - offset) * s;

      R = saturate (Y + Co - Cg);
      G = saturate (Y + Cg);
      B = saturate (Y - Co - Cg);

      data[4 * i + 0] = (guchar) (R * 255.0f);
      data[4 * i + 1] = (guchar) (G * 255.0f);
      data[4 * i + 2] = (guchar) (B * 255.0f);

      /* set alpha to 1 */
      data[4 * i + 3] = 255;

      if ((i & 0x7fff) == 0)
        gimp_progress_update ((gdouble) i / (gdouble) num_pixels);
    }

  gegl_buffer_set (buffer, GEGL_RECTANGLE (0, 0, w, h), 0, format, data,
                   GEGL_AUTO_ROWSTRIDE);

  gimp_progress_update (1.0);

  gegl_buffer_flush (buffer);

  gimp_drawable_update (drawable, 0, 0, w, h);

  g_free (data);

  g_object_unref (buffer);
}

void
decode_alpha_exponent (GimpDrawable *drawable)
{
  GeglBuffer *buffer;
  const Babl *format;
  guchar     *data;
  guint       num_pixels;
  guint       i, w, h;
  gint        R, G, B, A;

  buffer = gimp_drawable_get_buffer (drawable);

  format = babl_format ("R'G'B'A u8");

  w = gegl_buffer_get_width  (buffer);
  h = gegl_buffer_get_height (buffer);
  num_pixels = w * h;

  data = g_malloc (num_pixels * 4);

  gegl_buffer_get (buffer, GEGL_RECTANGLE (0, 0, w, h), 1.0, format, data,
                   GEGL_AUTO_ROWSTRIDE, GEGL_ABYSS_NONE);

  gimp_progress_init (_("Decoding Alpha-exponent pixels..."));

  for (i = 0; i < num_pixels; ++i)
    {
      R = data[4 * i + 0];
      G = data[4 * i + 1];
      B = data[4 * i + 2];
      A = data[4 * i + 3];

      R = (R * A + 1) >> 8;
      G = (G * A + 1) >> 8;
      B = (B * A + 1) >> 8;
      A = 255;

      data[4 * i + 0] = R;
      data[4 * i + 1] = G;
      data[4 * i + 2] = B;
      data[4 * i + 3] = A;

      if ((i & 0x7fff) == 0)
        gimp_progress_update ((gdouble) i / (gdouble) num_pixels);
    }

  gegl_buffer_set (buffer, GEGL_RECTANGLE (0, 0, w, h), 0, format, data,
                   GEGL_AUTO_ROWSTRIDE);

  gimp_progress_update (1.0);

  gegl_buffer_flush (buffer);

  gimp_drawable_update (drawable, 0, 0, w, h);

  g_free (data);

  g_object_unref (buffer);
}


/*
 * Encoding Functions
 */

void
encode_ycocg (guchar *dst,
              gint    r,
              gint    g,
              gint    b)
{
  gint y  = ((r +     (g << 1) + b) + 2) >> 2;
  gint co = ((((r << 1) - (b << 1)) + 2) >> 2) + 128;
  gint cg = (((-r +   (g << 1) - b) + 2) >> 2) + 128;

  dst[0] = 255;
  dst[1] = (cg > 255 ? 255 : (cg < 0 ? 0 : cg));
  dst[2] = (co > 255 ? 255 : (co < 0 ? 0 : co));
  dst[3] = (y  > 255 ? 255 : (y  < 0 ? 0 :  y));
}

void
encode_alpha_exponent (guchar *dst,
                       gint    r,
                       gint    g,
                       gint    b,
                       gint    a)
{
  gfloat ar, ag, ab, aa;

  ar = (gfloat) r / 255.0f;
  ag = (gfloat) g / 255.0f;
  ab = (gfloat) b / 255.0f;

  aa = MAX (ar, MAX (ag, ab));

  if (aa < 1e-04f)
    {
      dst[0] = b;
      dst[1] = g;
      dst[2] = r;
      dst[3] = 255;
      return;
    }

  ar /= aa;
  ag /= aa;
  ab /= aa;

  r = (gint) floorf (255.0f * ar + 0.5f);
  g = (gint) floorf (255.0f * ag + 0.5f);
  b = (gint) floorf (255.0f * ab + 0.5f);
  a = (gint) floorf (255.0f * aa + 0.5f);

  dst[0] = MAX (0, MIN (255, b));
  dst[1] = MAX (0, MIN (255, g));
  dst[2] = MAX (0, MIN (255, r));
  dst[3] = MAX (0, MIN (255, a));
}


/*
 * Compression Functions
 */

static void
get_min_max_YCoCg (const guchar *block,
                   guchar       *mincolor,
                   guchar       *maxcolor)
{
  gint i;

  mincolor[2] = mincolor[1] = 255;
  maxcolor[2] = maxcolor[1] = 0;

  for (i = 0; i < 16; ++i)
    {
      if (block[4 * i + 2] < mincolor[2]) mincolor[2] = block[4 * i + 2];
      if (block[4 * i + 1] < mincolor[1]) mincolor[1] = block[4 * i + 1];
      if (block[4 * i + 2] > maxcolor[2]) maxcolor[2] = block[4 * i + 2];
      if (block[4 * i + 1] > maxcolor[1]) maxcolor[1] = block[4 * i + 1];
    }
}

static void
scale_YCoCg (guchar *block,
             guchar *mincolor,
             guchar *maxcolor)
{
  const gint s0 = 128 / 2 - 1;
  const gint s1 = 128 / 4 - 1;
  gint       m0, m1, m2, m3;
  gint       mask0, mask1, scale;
  gint       i;

  m0 = abs (mincolor[2] - 128);
  m1 = abs (mincolor[1] - 128);
  m2 = abs (maxcolor[2] - 128);
  m3 = abs (maxcolor[1] - 128);

  if (m1 > m0) m0 = m1;
  if (m3 > m2) m2 = m3;
  if (m2 > m0) m0 = m2;

  mask0 = -(m0 <= s0);
  mask1 = -(m0 <= s1);
  scale = 1 + (1 & mask0) + (2 & mask1);

  mincolor[2] = (mincolor[2] - 128) * scale + 128;
  mincolor[1] = (mincolor[1] - 128) * scale + 128;
  mincolor[0] = (scale - 1) << 3;

  maxcolor[2] = (maxcolor[2] - 128) * scale + 128;
  maxcolor[1] = (maxcolor[1] - 128) * scale + 128;
  maxcolor[0] = (scale - 1) << 3;

  for (i = 0; i < 16; ++i)
    {
      block[i * 4 + 2] = (block[i * 4 + 2] - 128) * scale + 128;
      block[i * 4 + 1] = (block[i * 4 + 1] - 128) * scale + 128;
    }
}

#define INSET_SHIFT  4

static void
inset_bbox_YCoCg (guchar *mincolor,
                  guchar *maxcolor)
{
  gint inset[4], mini[4], maxi[4];

  inset[2] = (maxcolor[2] - mincolor[2]) - ((1 << (INSET_SHIFT - 1)) - 1);
  inset[1] = (maxcolor[1] - mincolor[1]) - ((1 << (INSET_SHIFT - 1)) - 1);

  mini[2] = ((mincolor[2] << INSET_SHIFT) + inset[2]) >> INSET_SHIFT;
  mini[1] = ((mincolor[1] << INSET_SHIFT) + inset[1]) >> INSET_SHIFT;

  maxi[2] = ((maxcolor[2] << INSET_SHIFT) - inset[2]) >> INSET_SHIFT;
  maxi[1] = ((maxcolor[1] << INSET_SHIFT) - inset[1]) >> INSET_SHIFT;

  mini[2] = (mini[2] >= 0) ? mini[2] : 0;
  mini[1] = (mini[1] >= 0) ? mini[1] : 0;

  maxi[2] = (maxi[2] <= 255) ? maxi[2] : 255;
  maxi[1] = (maxi[1] <= 255) ? maxi[1] : 255;

  mincolor[2] = (mini[2] & 0xf8) | (mini[2] >> 5);
  mincolor[1] = (mini[1] & 0xfc) | (mini[1] >> 6);

  maxcolor[2] = (maxi[2] & 0xf8) | (maxi[2] >> 5);
  maxcolor[1] = (maxi[1] & 0xfc) | (maxi[1] >> 6);
}

static void
select_diagonal_YCoCg (const guchar *block,
                       guchar       *mincolor,
                       guchar       *maxcolor)
{
  guchar mid0, mid1, side, mask, b0, b1, c0, c1;
  gint   i;

  mid0 = ((gint) mincolor[2] + maxcolor[2] + 1) >> 1;
  mid1 = ((gint) mincolor[1] + maxcolor[1] + 1) >> 1;

  side = 0;
  for (i = 0; i < 16; ++i)
    {
      b0 = block[i * 4 + 2] >= mid0;
      b1 = block[i * 4 + 1] >= mid1;
      side += (b0 ^ b1);
    }

  mask  = -(side > 8);
  mask &= -(mincolor[2] != maxcolor[2]);

  c0 = mincolor[1];
  c1 = maxcolor[1];

  c0 ^= c1;
  c1 ^= c0 & mask;
  c0 ^= c1;

  mincolor[1] = c0;
  maxcolor[1] = c1;
}

void
encode_YCoCg_block (guchar *dst,
                    guchar *block)
{
  guchar colors[4][3], *maxcolor, *mincolor;
  guint  mask;
  gint   c0, c1, d0, d1, d2, d3;
  gint   b0, b1, b2, b3, b4;
  gint   x0, x1, x2;
  gint   i, idx;

  maxcolor = &colors[0][0];
  mincolor = &colors[1][0];

  get_min_max_YCoCg (block, mincolor, maxcolor);
  scale_YCoCg (block, mincolor, maxcolor);
  inset_bbox_YCoCg (mincolor, maxcolor);
  select_diagonal_YCoCg (block, mincolor, maxcolor);

  colors[2][0] = (2 * maxcolor[0] + mincolor[0]) / 3;
  colors[2][1] = (2 * maxcolor[1] + mincolor[1]) / 3;
  colors[2][2] = (2 * maxcolor[2] + mincolor[2]) / 3;

  colors[3][0] = (2 * mincolor[0] + maxcolor[0]) / 3;
  colors[3][1] = (2 * mincolor[1] + maxcolor[1]) / 3;
  colors[3][2] = (2 * mincolor[2] + maxcolor[2]) / 3;

  mask = 0;

  for (i = 0; i < 16; ++i)
    {
      c0 = block[4 * i + 2];
      c1 = block[4 * i + 1];

      d0 = abs (colors[0][2] - c0) + abs (colors[0][1] - c1);
      d1 = abs (colors[1][2] - c0) + abs (colors[1][1] - c1);
      d2 = abs (colors[2][2] - c0) + abs (colors[2][1] - c1);
      d3 = abs (colors[3][2] - c0) + abs (colors[3][1] - c1);

      b0 = d0 > d3;
      b1 = d1 > d2;
      b2 = d0 > d2;
      b3 = d1 > d3;
      b4 = d2 > d3;

      x0 = b1 & b2;
      x1 = b0 & b3;
      x2 = b0 & b4;

      idx = (x2 | ((x0 | x1) << 1));

      mask |= idx << (2 * i);
    }

  PUTL16 (dst + 0, (mul8bit (maxcolor[2], 31) << 11) |
                   (mul8bit (maxcolor[1], 63) <<  5) |
                   (mul8bit (maxcolor[0], 31)      ));
  PUTL16 (dst + 2, (mul8bit (mincolor[2], 31) << 11) |
                   (mul8bit (mincolor[1], 63) <<  5) |
                   (mul8bit (mincolor[0], 31)      ));
  PUTL32 (dst + 4, mask);
}
