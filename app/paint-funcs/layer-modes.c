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

#include <glib-object.h>

#include "libgimpcolor/gimpcolor.h"

#include "paint-funcs-types.h"

#include "layer-modes.h"
#include "paint-funcs-utils.h"


#define RANDOM_SEED   314159265
#define RANDOM_TABLE_SIZE  4096

static guchar  add_lut[511];
static gint32  random_table[RANDOM_TABLE_SIZE];


/** FIXME: should be static inline **/
void
dissolve_pixels (const guchar *src,
                 const guchar *mask,
                 guchar       *dest,
                 gint          x,
                 gint          y,
                 gint          opacity,
                 gint          length,
                 gint          sb,
                 gint          db,
                 gboolean      has_alpha)
{
  const gint  alpha = db - 1;
  gint        b;
  GRand      *gr;

  gr = g_rand_new_with_seed (random_table[y % RANDOM_TABLE_SIZE]);

  /* Ignore x random values so we get a deterministic result */
  for (b = 0; b < x; b ++)
    g_rand_int (gr);

  while (length--)
    {
      gint    combined_opacity;
      gint32  rand_val;

      /*  preserve the intensity values  */
      for (b = 0; b < alpha; b++)
        dest[b] = src[b];

      /*  dissolve if random value is >= opacity  */
      rand_val = g_rand_int_range (gr, 0, 255);

      if (mask)
        {
          if (has_alpha)
            combined_opacity = opacity * src[alpha] * *mask / (255 * 255);
          else
            combined_opacity = opacity * *mask / 255;

          mask++;
        }
      else
        {
          if (has_alpha)
            combined_opacity = opacity * src[alpha] / 255;
          else
            combined_opacity = opacity;
        }

      dest[alpha] = (rand_val >= combined_opacity) ? 0 : OPAQUE_OPACITY;

      src  += sb;
      dest += db;
    }

  g_rand_free (gr);

}

static inline void
multiply_pixels (const guchar *src1,
                 const guchar *src2,
                 guchar       *dest,
                 guint         length,
                 guint         bytes1,
                 guint         bytes2)
{
  const guint has_alpha1 = HAS_ALPHA (bytes1);
  const guint has_alpha2 = HAS_ALPHA (bytes2);
  const guint alpha      = ((has_alpha1 || has_alpha2) ?
                            MAX (bytes1, bytes2) - 1 : bytes1);

  if (has_alpha1 && has_alpha2)
    {
      while (length --)
        {
          guint b;

          for (b = 0; b < alpha; b++)
            {
              guint tmp;

              dest[b] = INT_MULT(src1[b], src2[b], tmp);
            }

          dest[alpha] = MIN (src1[alpha], src2[alpha]);

          src1 += bytes1;
          src2 += bytes2;
          dest += bytes2;
        }
    }
  else if (has_alpha2)
    {
      while (length --)
        {
          guint b;

          for (b = 0; b < alpha; b++)
            {
              guint tmp;

              dest[b] = INT_MULT(src1[b], src2[b], tmp);
            }

          dest[alpha] = src2[alpha];

          src1 += bytes1;
          src2 += bytes2;
          dest += bytes2;
        }
    }
  else
    {
      while (length --)
        {
          guint b;

          for (b = 0; b < alpha; b++)
            {
              guint tmp;

              dest[b] = INT_MULT (src1[b], src2[b], tmp);
            }

          src1 += bytes1;
          src2 += bytes2;
          dest += bytes2;
        }
    }
}

static inline void
divide_pixels (const guchar *src1,
               const guchar *src2,
               guchar       *dest,
               guint         length,
               guint         bytes1,
               guint         bytes2)
{
  const guint has_alpha1 = HAS_ALPHA (bytes1);
  const guint has_alpha2 = HAS_ALPHA (bytes2);
  const guint alpha      = ((has_alpha1 || has_alpha2) ?
                            MAX (bytes1, bytes2) - 1 : bytes1);
  while (length--)
    {
      guint b;

      for (b = 0; b < alpha; b++)
        {
          guint result = ((src1[b] * 256) / (1 + src2[b]));

          dest[b] = MIN (result, 255);
        }

      if (has_alpha1 && has_alpha2)
        dest[alpha] = MIN (src1[alpha], src2[alpha]);
      else if (has_alpha2)
        dest[alpha] = src2[alpha];

      src1 += bytes1;
      src2 += bytes2;
      dest += bytes2;
    }
}


static inline void
screen_pixels (const guchar *src1,
               const guchar *src2,
               guchar       *dest,
               guint         length,
               guint         bytes1,
               guint         bytes2)
{
  const guint has_alpha1 = HAS_ALPHA (bytes1);
  const guint has_alpha2 = HAS_ALPHA (bytes2);
  const guint alpha      = ((has_alpha1 || has_alpha2) ?
                            MAX (bytes1, bytes2) - 1 : bytes1);

  while (length --)
    {
      guint b;

      for (b = 0; b < alpha; b++)
        {
          guint tmp;

          dest[b] = 255 - INT_MULT((255 - src1[b]), (255 - src2[b]), tmp);
        }

      if (has_alpha1 && has_alpha2)
        dest[alpha] = MIN (src1[alpha], src2[alpha]);
      else if (has_alpha2)
        dest[alpha] = src2[alpha];

      src1 += bytes1;
      src2 += bytes2;
      dest += bytes2;
    }
}


static inline void
overlay_pixels (const guchar *src1,
                const guchar *src2,
                guchar       *dest,
                guint         length,
                guint         bytes1,
                guint         bytes2)
{
  const guint has_alpha1 = HAS_ALPHA (bytes1);
  const guint has_alpha2 = HAS_ALPHA (bytes2);
  const guint alpha      = ((has_alpha1 || has_alpha2) ?
                            MAX (bytes1, bytes2) - 1 : bytes1);

  while (length --)
    {
      guint b;

      for (b = 0; b < alpha; b++)
        {
          guint tmp, tmpM;

          dest[b] = INT_MULT(src1[b], src1[b] + INT_MULT (2 * src2[b],
                                                          255 - src1[b],
                                                          tmpM), tmp);
        }

      if (has_alpha1 && has_alpha2)
        dest[alpha] = MIN (src1[alpha], src2[alpha]);
      else if (has_alpha2)
        dest[alpha] = src2[alpha];

      src1 += bytes1;
      src2 += bytes2;
      dest += bytes2;
    }
}

static inline void
difference_pixels (const guchar *src1,
                   const guchar *src2,
                   guchar       *dest,
                   guint         length,
                   guint         bytes1,
                   guint         bytes2)
{
  const guint has_alpha1 = HAS_ALPHA (bytes1);
  const guint has_alpha2 = HAS_ALPHA (bytes2);
  const guint alpha      = ((has_alpha1 || has_alpha2) ?
                            MAX (bytes1, bytes2) - 1 : bytes1);

  while (length --)
    {
      guint b;

      for (b = 0; b < alpha; b++)
        {
          gint diff = src1[b] - src2[b];

          dest[b] = (diff < 0) ? -diff : diff;
        }

      if (has_alpha1 && has_alpha2)
        dest[alpha] = MIN (src1[alpha], src2[alpha]);
      else if (has_alpha2)
        dest[alpha] = src2[alpha];

      src1 += bytes1;
      src2 += bytes2;
      dest += bytes2;
    }
}

static inline void
add_pixels (const guchar *src1,
            const guchar *src2,
            guchar       *dest,
            guint         length,
            guint         bytes1,
            guint         bytes2)
{
  const guint has_alpha1 = HAS_ALPHA (bytes1);
  const guint has_alpha2 = HAS_ALPHA (bytes2);
  const guint alpha      = ((has_alpha1 || has_alpha2) ?
                            MAX (bytes1, bytes2) - 1 : bytes1);

  while (length --)
    {
      guint b;

      for (b = 0; b < alpha; b++)
          dest[b] = add_lut[src1[b] + src2[b]];

      if (has_alpha1 && has_alpha2)
        dest[alpha] = MIN (src1[alpha], src2[alpha]);
      else if (has_alpha2)
        dest[alpha] = src2[alpha];

      src1 += bytes1;
      src2 += bytes2;
      dest += bytes2;
    }
}


static inline void
subtract_pixels (const guchar *src1,
                 const guchar *src2,
                 guchar       *dest,
                 guint         length,
                 guint         bytes1,
                 guint         bytes2)
{
  const guint has_alpha1 = HAS_ALPHA (bytes1);
  const guint has_alpha2 = HAS_ALPHA (bytes2);
  const guint alpha      = ((has_alpha1 || has_alpha2) ?
                            MAX (bytes1, bytes2) - 1 : bytes1);

  while (length --)
    {
      guint b;

      for (b = 0; b < alpha; b++)
        {
          gint diff = src1[b] - src2[b];

          dest[b] = (diff < 0) ? 0 : diff;
        }

      if (has_alpha1 && has_alpha2)
        dest[alpha] = MIN (src1[alpha], src2[alpha]);
      else if (has_alpha2)
        dest[alpha] = src2[alpha];

      src1 += bytes1;
      src2 += bytes2;
      dest += bytes2;
    }
}

static inline void
darken_pixels (const guchar *src1,
               const guchar *src2,
               guchar       *dest,
               guint         length,
               guint         bytes1,
               guint         bytes2)
{
  const guint has_alpha1 = HAS_ALPHA (bytes1);
  const guint has_alpha2 = HAS_ALPHA (bytes2);
  const guint alpha      = ((has_alpha1 || has_alpha2) ?
                            MAX (bytes1, bytes2) - 1 : bytes1);

  while (length--)
    {
      guint b;

      for (b = 0; b < alpha; b++)
        {
          guchar s1 = src1[b];
          guchar s2 = src2[b];

          dest[b] = (s1 < s2) ? s1 : s2;
        }

      if (has_alpha1 && has_alpha2)
        dest[alpha] = MIN (src1[alpha], src2[alpha]);
      else if (has_alpha2)
        dest[alpha] = src2[alpha];

      src1 += bytes1;
      src2 += bytes2;
      dest += bytes2;
    }
}


static inline void
lighten_pixels (const guchar *src1,
                const guchar *src2,
                guchar       *dest,
                guint         length,
                guint         bytes1,
                guint         bytes2)
{
  const guint has_alpha1 = HAS_ALPHA (bytes1);
  const guint has_alpha2 = HAS_ALPHA (bytes2);
  const guint alpha      = ((has_alpha1 || has_alpha2) ?
                            MAX (bytes1, bytes2) - 1 : bytes1);

  while (length--)
    {
      guint b;

      for (b = 0; b < alpha; b++)
        {
          guchar s1 = src1[b];
          guchar s2 = src2[b];

          dest[b] = (s1 < s2) ? s2 : s1;
        }

      if (has_alpha1 && has_alpha2)
        dest[alpha] = MIN (src1[alpha], src2[alpha]);
      else if (has_alpha2)
        dest[alpha] = src2[alpha];

      src1 += bytes1;
      src2 += bytes2;
      dest += bytes2;
    }
}


static inline void
hue_only_pixels (const guchar *src1,
                 const guchar *src2,
                 guchar       *dest,
                 guint         length,
                 guint         bytes1,
                 guint         bytes2)
{
  const guint has_alpha1 = HAS_ALPHA (bytes1);
  const guint has_alpha2 = HAS_ALPHA (bytes2);

  /*  assumes inputs are only 4 byte RGBA pixels  */
  while (length--)
    {
      gint r1, g1, b1;
      gint r2, g2, b2;

      r1 = src1[0]; g1 = src1[1]; b1 = src1[2];
      r2 = src2[0]; g2 = src2[1]; b2 = src2[2];

      gimp_rgb_to_hsv_int (&r1, &g1, &b1);
      gimp_rgb_to_hsv_int (&r2, &g2, &b2);

      r1 = r2;

      /*  set the destination  */
      gimp_hsv_to_rgb_int (&r1, &g1, &b1);

      dest[0] = r1; dest[1] = g1; dest[2] = b1;

      if (has_alpha1 && has_alpha2)
        dest[3] = MIN (src1[3], src2[3]);
      else if (has_alpha2)
        dest[3] = src2[3];

      src1 += bytes1;
      src2 += bytes2;
      dest += bytes2;
    }
}


static inline void
saturation_only_pixels (const guchar *src1,
                        const guchar *src2,
                        guchar       *dest,
                        guint         length,
                        guint         bytes1,
                        guint         bytes2)
{
  const guint has_alpha1 = HAS_ALPHA (bytes1);
  const guint has_alpha2 = HAS_ALPHA (bytes2);

  /*  assumes inputs are only 4 byte RGBA pixels  */
  while (length--)
    {
      gint r1, g1, b1;
      gint r2, g2, b2;

      r1 = src1[0]; g1 = src1[1]; b1 = src1[2];
      r2 = src2[0]; g2 = src2[1]; b2 = src2[2];

      gimp_rgb_to_hsv_int (&r1, &g1, &b1);
      gimp_rgb_to_hsv_int (&r2, &g2, &b2);

      g1 = g2;

      /*  set the destination  */
      gimp_hsv_to_rgb_int (&r1, &g1, &b1);

      dest[0] = r1; dest[1] = g1; dest[2] = b1;

      if (has_alpha1 && has_alpha2)
        dest[3] = MIN (src1[3], src2[3]);
      else if (has_alpha2)
        dest[3] = src2[3];

      src1 += bytes1;
      src2 += bytes2;
      dest += bytes2;
    }
}


static inline void
value_only_pixels (const guchar *src1,
                   const guchar *src2,
                   guchar       *dest,
                   guint         length,
                   guint         bytes1,
                   guint         bytes2)
{
  const guint has_alpha1 = HAS_ALPHA (bytes1);
  const guint has_alpha2 = HAS_ALPHA (bytes2);

  /*  assumes inputs are only 4 byte RGBA pixels  */
  while (length--)
    {
      gint r1, g1, b1;
      gint r2, g2, b2;

      r1 = src1[0]; g1 = src1[1]; b1 = src1[2];
      r2 = src2[0]; g2 = src2[1]; b2 = src2[2];

      gimp_rgb_to_hsv_int (&r1, &g1, &b1);
      gimp_rgb_to_hsv_int (&r2, &g2, &b2);

      b1 = b2;

      /*  set the destination  */
      gimp_hsv_to_rgb_int (&r1, &g1, &b1);

      dest[0] = r1; dest[1] = g1; dest[2] = b1;

      if (has_alpha1 && has_alpha2)
        dest[3] = MIN (src1[3], src2[3]);
      else if (has_alpha2)
        dest[3] = src2[3];

      src1 += bytes1;
      src2 += bytes2;
      dest += bytes2;
    }
}


static inline void
color_only_pixels (const guchar *src1,
                   const guchar *src2,
                   guchar       *dest,
                   guint         length,
                   guint         bytes1,
                   guint         bytes2)
{
  const guint has_alpha1 = HAS_ALPHA (bytes1);
  const guint has_alpha2 = HAS_ALPHA (bytes2);

  /*  assumes inputs are only 4 byte RGBA pixels  */
  while (length--)
    {
      gint r1, g1, b1;
      gint r2, g2, b2;

      r1 = src1[0]; g1 = src1[1]; b1 = src1[2];
      r2 = src2[0]; g2 = src2[1]; b2 = src2[2];

      gimp_rgb_to_hsl_int (&r1, &g1, &b1);
      gimp_rgb_to_hsl_int (&r2, &g2, &b2);

      /*  transfer hue and saturation to the source pixel  */
      r1 = r2;
      g1 = g2;

      /*  set the destination  */
      gimp_hsl_to_rgb_int (&r1, &g1, &b1);

      dest[0] = r1; dest[1] = g1; dest[2] = b1;

      if (has_alpha1 && has_alpha2)
        dest[3] = MIN (src1[3], src2[3]);
      else if (has_alpha2)
        dest[3] = src2[3];

      src1 += bytes1;
      src2 += bytes2;
      dest += bytes2;
    }
}

static inline void
dodge_pixels (const guchar *src1,
              const guchar *src2,
              guchar           *dest,
              guint         length,
              guint         bytes1,
              guint         bytes2)
{
  const guint has_alpha1 = HAS_ALPHA (bytes1);
  const guint has_alpha2 = HAS_ALPHA (bytes2);
  const guint alpha      = ((has_alpha1 || has_alpha2) ?
                            MAX (bytes1, bytes2) - 1 : bytes1);
  while (length --)
    {
      guint b;

      for (b = 0; b < alpha; b++)
        {
          guint tmp;

          tmp = src1[b] << 8;
          tmp /= 256 - src2[b];

          dest[b] = (guchar) MIN (tmp, 255);
        }

      if (has_alpha1 && has_alpha2)
        dest[alpha] = MIN (src1[alpha], src2[alpha]);
      else if (has_alpha2)
        dest[alpha] = src2[alpha];

      src1 += bytes1;
      src2 += bytes2;
      dest += bytes2;
    }
}


static inline void
burn_pixels (const guchar *src1,
             const guchar *src2,
             guchar       *dest,
             guint         length,
             guint         bytes1,
             guint         bytes2)
{
  const guint has_alpha1 = HAS_ALPHA (bytes1);
  const guint has_alpha2 = HAS_ALPHA (bytes2);
  const guint alpha      = ((has_alpha1 || has_alpha2) ?
                            MAX (bytes1, bytes2) - 1 : bytes1);

  while (length --)
    {
      guint b;

      for (b = 0; b < alpha; b++)
        {
          /* FIXME: Is the burn effect supposed to be dependant on the sign
           * of this temporary variable?
           */
          gint tmp;

          tmp = (255 - src1[b]) << 8;
          tmp /= src2[b] + 1;

          dest[b] = (guchar) CLAMP (255 - tmp, 0, 255);
        }

      if (has_alpha1 && has_alpha2)
        dest[alpha] = MIN (src1[alpha], src2[alpha]);
      else if (has_alpha2)
        dest[alpha] = src2[alpha];

      src1 += bytes1;
      src2 += bytes2;
      dest += bytes2;
    }
}


static inline void
hardlight_pixels (const guchar *src1,
                  const guchar *src2,
                  guchar       *dest,
                  guint         length,
                  guint         bytes1,
                  guint         bytes2)
{
  const guint has_alpha1 = HAS_ALPHA (bytes1);
  const guint has_alpha2 = HAS_ALPHA (bytes2);
  const guint alpha      = ((has_alpha1 || has_alpha2) ?
                            MAX (bytes1, bytes2) - 1 : bytes1);

  while (length --)
    {
      guint b;

      for (b = 0; b < alpha; b++)
        {
          guint tmp;

          if (src2[b] > 128)
            {
              tmp = (((gint)255 - src1[b]) *
                     ((gint)255 - ((src2[b] - 128) << 1)));
              dest[b] = (guchar) MIN (255 - (tmp >> 8), 255);
            }
          else
            {
              tmp = (gint) src1[b] * ((gint)src2[b] << 1);
              dest[b] = (guchar) MIN (tmp >> 8, 255);
            }
        }

      if (has_alpha1 && has_alpha2)
        dest[alpha] = MIN (src1[alpha], src2[alpha]);
      else if (has_alpha2)
        dest[alpha] = src2[alpha];

      src1 += bytes1;
      src2 += bytes2;
      dest += bytes2;
    }
}


static inline void
softlight_pixels (const guchar *src1,
                  const guchar *src2,
                  guchar       *dest,
                  guint         length,
                  guint         bytes1,
                  guint         bytes2)
{
  const guint has_alpha1 = HAS_ALPHA (bytes1);
  const guint has_alpha2 = HAS_ALPHA (bytes2);
  const guint alpha      = ((has_alpha1 || has_alpha2) ?
                            MAX (bytes1, bytes2) - 1 : bytes1);

  while (length --)
    {
      guint b;

        for (b = 0; b < alpha; b++)
        {
          guint tmp1, tmp2, tmp3;

          /* Mix multiply and screen */
         guint tmpM = INT_MULT (src1[b], src2[b], tmpM);
         guint tmpS = 255 - INT_MULT((255 - src1[b]), (255 - src2[b]), tmp1);

          dest[b] = INT_MULT ((255 - src1[b]), tmpM, tmp2) +
            INT_MULT (src1[b], tmpS, tmp3);
        }

      if (has_alpha1 && has_alpha2)
        dest[alpha] = MIN (src1[alpha], src2[alpha]);
      else if (has_alpha2)
        dest[alpha] = src2[alpha];

      src1 += bytes1;
      src2 += bytes2;
      dest += bytes2;
    }
}


static inline void
grain_extract_pixels (const guchar *src1,
                      const guchar *src2,
                      guchar       *dest,
                      guint         length,
                      guint         bytes1,
                      guint         bytes2)
{
  const guint has_alpha1 = HAS_ALPHA (bytes1);
  const guint has_alpha2 = HAS_ALPHA (bytes2);
  const guint alpha      = ((has_alpha1 || has_alpha2) ?
                            MAX (bytes1, bytes2) - 1 : bytes1);

  while (length --)
    {
      guint b;

      for (b = 0; b < alpha; b++)
        {
          gint diff = src1[b] - src2[b] + 128;

          dest[b] = (guchar) CLAMP (diff, 0, 255);
        }

      if (has_alpha1 && has_alpha2)
        dest[alpha] = MIN (src1[alpha], src2[alpha]);
      else if (has_alpha2)
        dest[alpha] = src2[alpha];

      src1 += bytes1;
      src2 += bytes2;
      dest += bytes2;
    }
}


static inline void
grain_merge_pixels (const guchar *src1,
                    const guchar *src2,
                    guchar       *dest,
                    guint         length,
                    guint         bytes1,
                    guint         bytes2)
{
  const guint has_alpha1 = HAS_ALPHA (bytes1);
  const guint has_alpha2 = HAS_ALPHA (bytes2);
  const guint alpha      = ((has_alpha1 || has_alpha2) ?
                            MAX (bytes1, bytes2) - 1 : bytes1);

  while (length --)
    {
      guint  b;

      for (b = 0; b < alpha; b++)
        {
          /* Add, re-center and clip. */
          gint sum = src1[b] + src2[b] - 128;

          dest[b] = (guchar) CLAMP (sum, 0, 255);
        }

      if (has_alpha1 && has_alpha2)
        dest[alpha] = MIN (src1[alpha], src2[alpha]);
      else if (has_alpha2)
        dest[alpha] = src2[alpha];

      src1 += bytes1;
      src2 += bytes2;
      dest += bytes2;
    }
}



/**********************************
 * Layer mode interface functions *
 **********************************/

void
layer_modes_setup (void)
{
  GRand *gr;
  gint   i;

  /*  generate a table of random seeds  */
  gr = g_rand_new_with_seed (RANDOM_SEED);

  for (i = 0; i < RANDOM_TABLE_SIZE; i++)
    random_table[i] = g_rand_int (gr);

  for (i = 0; i < 256; i++)
    add_lut[i] = i;

  for (i = 256; i <= 510; i++)
    add_lut[i] = 255;

  g_rand_free (gr);
}

void
layer_normal_mode (struct apply_layer_mode_struct *alms)
{
  /*  assumes we're applying src2 TO src1  */
  *(alms->dest) = alms->src2;
}

void
layer_dissolve_mode (struct apply_layer_mode_struct *alms)
{
  const guint has_alpha1 = HAS_ALPHA (alms->bytes1);
  const guint has_alpha2 = HAS_ALPHA (alms->bytes2);
  guint       dest_bytes;

  /*  Since dissolve requires an alpha channel...  */
  if (has_alpha2)
    dest_bytes = alms->bytes2;
  else
    dest_bytes = alms->bytes2 + 1;

  dissolve_pixels (alms->src2, alms->mask, *(alms->dest),
                   alms->x, alms->y,
                   alms->opacity, alms->length,
                   alms->bytes2, dest_bytes,
                   has_alpha2);

  alms->combine = has_alpha1 ? COMBINE_INTEN_A_INTEN_A : COMBINE_INTEN_INTEN_A;
}

void
layer_multiply_mode (struct apply_layer_mode_struct *alms)
{
  multiply_pixels (alms->src1, alms->src2, *(alms->dest), alms->length,
                   alms->bytes1, alms->bytes2);
}

void
layer_divide_mode (struct apply_layer_mode_struct *alms)
{
  divide_pixels (alms->src1, alms->src2, *(alms->dest), alms->length,
                 alms->bytes1, alms->bytes2);
}

void
layer_screen_mode (struct apply_layer_mode_struct *alms)
{
  screen_pixels (alms->src1, alms->src2, *(alms->dest), alms->length,
                 alms->bytes1, alms->bytes2);
}

void
layer_overlay_mode (struct apply_layer_mode_struct *alms)
{
  overlay_pixels (alms->src1, alms->src2, *(alms->dest), alms->length,
                  alms->bytes1, alms->bytes2);
}

void
layer_difference_mode (struct apply_layer_mode_struct *alms)
{
  difference_pixels (alms->src1, alms->src2, *(alms->dest), alms->length,
                     alms->bytes1, alms->bytes2);
}

void
layer_addition_mode (struct apply_layer_mode_struct *alms)
{
  add_pixels (alms->src1, alms->src2, *(alms->dest), alms->length,
              alms->bytes1, alms->bytes2);
}

void
layer_subtract_mode (struct apply_layer_mode_struct *alms)
{
  subtract_pixels (alms->src1, alms->src2, *(alms->dest), alms->length,
                   alms->bytes1, alms->bytes2);
}

void
layer_darken_only_mode (struct apply_layer_mode_struct *alms)
{
  darken_pixels (alms->src1, alms->src2, *(alms->dest), alms->length,
                 alms->bytes1, alms->bytes2);
}

void
layer_lighten_only_mode (struct apply_layer_mode_struct *alms)
{
  lighten_pixels (alms->src1, alms->src2, *(alms->dest), alms->length,
                  alms->bytes1, alms->bytes2);
}

void
layer_hue_mode (struct apply_layer_mode_struct *alms)
{
  /*  only works on RGB color images  */
  if (alms->bytes1 > 2)
    hue_only_pixels (alms->src1, alms->src2, *(alms->dest), alms->length,
                     alms->bytes1, alms->bytes2);
  else
    *(alms->dest) = alms->src2;
}

void
layer_saturation_mode (struct apply_layer_mode_struct *alms)
{
  /*  only works on RGB color images  */
  if (alms->bytes1 > 2)
    saturation_only_pixels (alms->src1, alms->src2, *(alms->dest),
                            alms->length, alms->bytes1, alms->bytes2);
  else
    *(alms->dest) = alms->src2;
}

void
layer_value_mode (struct apply_layer_mode_struct *alms)
{
  /*  only works on RGB color images  */
  if (alms->bytes1 > 2)
    value_only_pixels (alms->src1, alms->src2, *(alms->dest), alms->length,
                       alms->bytes1, alms->bytes2);
  else
    *(alms->dest) = alms->src2;
}

void
layer_color_mode (struct apply_layer_mode_struct *alms)
{
  /*  only works on RGB color images  */
  if (alms->bytes1 > 2)
    color_only_pixels (alms->src1, alms->src2, *(alms->dest), alms->length,
                       alms->bytes1, alms->bytes2);
  else
    *(alms->dest) = alms->src2;
}

void
layer_behind_mode (struct apply_layer_mode_struct *alms)
{
  *(alms->dest) = alms->src2;
  if (HAS_ALPHA (alms->bytes1))
    alms->combine = BEHIND_INTEN;
  else
    alms->combine = NO_COMBINATION;
}

void
layer_replace_mode (struct apply_layer_mode_struct *alms)
{
  *(alms->dest) = alms->src2;
  alms->combine = REPLACE_INTEN;
}

void
layer_erase_mode (struct apply_layer_mode_struct *alms)
{
  *(alms->dest) = alms->src2;
  /*  If both sources have alpha channels, call erase function.
   *  Otherwise, just combine in the normal manner
   */
  alms->combine =
    (HAS_ALPHA (alms->bytes1) && HAS_ALPHA (alms->bytes2)) ?  ERASE_INTEN : 0;
}

void
layer_anti_erase_mode (struct apply_layer_mode_struct *alms)
{
  *(alms->dest) = alms->src2;
  alms->combine =
    (HAS_ALPHA (alms->bytes1) && HAS_ALPHA (alms->bytes2)) ? ANTI_ERASE_INTEN : 0;
}

void
layer_color_erase_mode (struct apply_layer_mode_struct *alms)
{
  *(alms->dest) = alms->src2;
  alms->combine =
    (HAS_ALPHA (alms->bytes1) && HAS_ALPHA (alms->bytes2)) ? COLOR_ERASE_INTEN : 0;
}

void
layer_dodge_mode (struct apply_layer_mode_struct *alms)
{
  dodge_pixels (alms->src1, alms->src2, *(alms->dest), alms->length,
                alms->bytes1, alms->bytes2);
}

void
layer_burn_mode (struct apply_layer_mode_struct *alms)
{
  burn_pixels (alms->src1, alms->src2, *(alms->dest), alms->length,
               alms->bytes1, alms->bytes2);
}

void
layer_hardlight_mode (struct apply_layer_mode_struct *alms)
{
  hardlight_pixels (alms->src1, alms->src2, *(alms->dest), alms->length,
                    alms->bytes1, alms->bytes2);
}

void
layer_softlight_mode (struct apply_layer_mode_struct *alms)
{
  softlight_pixels (alms->src1, alms->src2, *(alms->dest), alms->length,
                    alms->bytes1, alms->bytes2);
}

void
layer_grain_extract_mode (struct apply_layer_mode_struct *alms)
{
  grain_extract_pixels (alms->src1, alms->src2, *(alms->dest), alms->length,
                        alms->bytes1, alms->bytes2);
}

void
layer_grain_merge_mode (struct apply_layer_mode_struct *alms)
{
  grain_merge_pixels (alms->src1, alms->src2, *(alms->dest), alms->length,
                      alms->bytes1, alms->bytes2);
}
