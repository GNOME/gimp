/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

/*
 * This file is supposed to contain the generic (read: C) implementation
 * of the pixel fiddling paint-functions.
 */

#ifndef __PAINT_FUNCS_GENERIC_H__
#define __PAINT_FUNCS_GENERIC_H__


#define INT_MULT(a,b,t)  ((t) = (a) * (b) + 0x80, ((((t) >> 8) + (t)) >> 8))

/* This version of INT_MULT3 is very fast, but suffers from some
   slight roundoff errors.  It returns the correct result 99.987
   percent of the time */
#define INT_MULT3(a,b,c,t)  ((t) = (a) * (b) * (c) + 0x7F5B, \
                            ((((t) >> 7) + (t)) >> 16))
/*
  This version of INT_MULT3 always gives the correct result, but runs at
  approximatly one third the speed. */
/*  #define INT_MULT3(a,b,c,t) (((a) * (b) * (c) + 32512) / 65025.0)
 */

#define INT_BLEND(a,b,alpha,tmp)  (INT_MULT((a) - (b), alpha, tmp) + (b))

#define RANDOM_TABLE_SIZE  4096

/* A drawable has an alphachannel if contains either 4 or 2 bytes data
 * aka GRAYA and RGBA and thus the macro below works. This will have
 * to change if we support bigger formats. We'll do it so for now because
 * masking is always cheaper than passing parameters over the stack.      */
/* FIXME: Move to a global place */
#define HAS_ALPHA(bytes) (~bytes & 1)

/* FIXME: Move to a more global place */
struct apply_layer_mode_struct
{
  guint              bytes1 : 3;
  guint              bytes2 : 3;
  guchar            *src1;
  guchar            *src2;
  const guchar      *mask;
  guchar           **dest;
  gint               x;
  gint               y;
  guint              opacity;
  guint              length;
  CombinationMode    combine;
};

static guchar  add_lut[511];
static gint32  random_table[RANDOM_TABLE_SIZE];

void
color_pixels (guchar       *dest,
              const guchar *color,
              guint         w,
              guint         bytes)
{
  switch (bytes)
    {
    case 1:
      memset (dest, *color, w);
      break;

    case 2:
#if defined(sparc) || defined(__sparc__)
      {
        const guchar c0 = color[0];
        const guchar c1 = color[1];

        while (w--)
          {
            dest[0] = c0;
            dest[1] = c1;
            dest += 2;
          }
      }
#else
      {
        const guint16   shortc = ((const guint16 *) color)[0];
        guint16        *shortd = (guint16 *) dest;

        while (w--)
          {
            *shortd = shortc;
            shortd++;
          }
      }
#endif /* sparc || __sparc__ */
      break;

    case 3:
      {
        const guchar c0 = color[0];
        const guchar c1 = color[1];
        const guchar c2 = color[2];

        while (w--)
          {
            dest[0] = c0;
            dest[1] = c1;
            dest[2] = c2;
            dest += 3;
          }
      }
      break;

    case 4:
#if defined(sparc) || defined(__sparc__)
      {
        const guchar c0 = color[0];
        const guchar c1 = color[1];
        const guchar c2 = color[2];
        const guchar c3 = color[3];

        while (w--)
          {
            dest[0] = c0;
            dest[1] = c1;
            dest[2] = c2;
            dest[3] = c3;
            dest += 4;
          }
      }
#else
      {
        const guint32   longc = ((const guint32 *) color)[0];
        guint32        *longd = (guint32 *) dest;

        while (w--)
          {
            *longd = longc;
            longd++;
          }
      }
#endif /* sparc || __sparc__ */
      break;

    default:
      while (w--)
        {
          memcpy (dest, color, bytes);
          dest += bytes;
        }
    }
}

void
color_pixels_mask (guchar       *dest,
                   const guchar *mask,
                   const guchar *color,
                   guint         w,
                   guint         bytes)
{
  guchar c0, c1, c2;
  gint   alpha;

  alpha = HAS_ALPHA (bytes) ? bytes - 1 : bytes;

  switch (bytes)
    {
    case 1:
      memset (dest, *color, w);
      break;

    case 2:
      c0 = color[0];
      while (w--)
        {
          dest[0] = c0;
          dest[1] = *mask++;
          dest += 2;
        }
      break;

    case 3:
      c0 = color[0];
      c1 = color[1];
      c2 = color[2];
      while (w--)
        {
          dest[0] = c0;
          dest[1] = c1;
          dest[2] = c2;
          dest += 3;
        }
      break;

    case 4:
      c0 = color[0];
      c1 = color[1];
      c2 = color[2];
      while (w--)
        {
          dest[0] = c0;
          dest[1] = c1;
          dest[2] = c2;
          dest[3] = *mask++;
          dest += 4;
        }
      break;
    }
}

void
pattern_pixels_mask (guchar       *dest,
                     const guchar *mask,
                     TempBuf      *pattern,
                     guint         w,
                     guint         bytes,
                     gint          x,
                     gint          y)
{
  const guint   alpha = HAS_ALPHA (bytes) ? bytes - 1 : bytes;
  const guchar *pat;
  guint         i;

  /*  Get a pointer to the appropriate scanline of the pattern buffer  */
  pat = (temp_buf_data (pattern) +
         (y % pattern->height) * pattern->width * pattern->bytes);


  /*
   * image data = pattern data for all but alpha
   *
   * If (image has alpha)
   *   if (there's a mask)
   *     image data = mask for alpha;
   *   else
   *     image data = opaque for alpha.
   *
   *   if (pattern has alpha)
   *     multiply existing alpha channel by pattern alpha
   *     (normalised to (0..1))
   */

  for (i = 0; i < w; i++)
    {
      const guchar *p = pat + ((i + x) % pattern->width) * pattern->bytes;
      guint         b;

      for (b = 0; b < alpha; b++)
        dest[b] = p[b];

      if (HAS_ALPHA (bytes))
        {
          if (mask)
            dest[alpha] = *mask++;
          else
            dest[alpha] = OPAQUE_OPACITY;

          if (HAS_ALPHA (pattern->bytes))
            dest[alpha] = (guchar) (dest[alpha] *
                                    p[alpha] / (gdouble) OPAQUE_OPACITY);
        }

      dest += bytes;
    }
}

/*
 * blend_pixels patched 8-24-05 to fix bug #163721.  Note that this change
 * causes the function to treat src1 and src2 asymmetrically.  This gives the
 * right behavior for the smudge tool, which is the only user of this function
 * at the time of patching.  If you want to use the function for something
 * else, caveat emptor.
 */
inline void
blend_pixels (const guchar *src1,
              const guchar *src2,
              guchar       *dest,
              guchar        blend,
              guint         w,
              guint         bytes)
{
  if (HAS_ALPHA (bytes))
    {
      const guint blend1 = 255 - blend;
      const guint blend2 = blend + 1;
      const guint c      = bytes - 1;

      while (w--)
        {
          const gint  a1 = blend1 * src1[c];
          const gint  a2 = blend2 * src2[c];
          const gint  a  = a1 + a2;
          guint       b;

          if (!a)
            {
              for (b = 0; b < bytes; b++)
                dest[b] = 0;
            }
          else
            {
              for (b = 0; b < c; b++)
                dest[b] =
                  src1[b] + (src1[b] * a1 + src2[b] * a2 - a * src1[b]) / a;

              dest[c] = a >> 8;
            }

          src1 += bytes;
          src2 += bytes;
          dest += bytes;
        }
    }
  else
    {
      const guchar blend1 = 255 - blend;

      while (w--)
        {
          guint b;

          for (b = 0; b < bytes; b++)
            dest[b] =
              src1[b] + (src1[b] * blend1 + src2[b] * blend - src1[b] * 255) / 255;

          src1 += bytes;
          src2 += bytes;
          dest += bytes;
        }
    }
}

inline void
shade_pixels (const guchar *src,
              guchar       *dest,
              const guchar *col,
              guchar        blend,
              guint         w,
              guint         bytes,
              gboolean      has_alpha)
{
  const guchar blend2 = (255 - blend);
  const guint  alpha = (has_alpha) ? bytes - 1 : bytes;

  while (w--)
    {
      guint b;

      for (b = 0; b < alpha; b++)
        dest[b] = (src[b] * blend2 + col[b] * blend) / 255;

      if (has_alpha)
        dest[alpha] = src[alpha];  /* alpha channel */

      src += bytes;
      dest += bytes;
    }
}


inline void
extract_alpha_pixels (const guchar *src,
                      const guchar *mask,
                      guchar       *dest,
                      guint         w,
                      guint         bytes)
{
  const guint alpha = bytes - 1;

  if (mask)
    {
      const guchar *m = mask;

      while (w--)
        {
          gint tmp;

          *dest++ = INT_MULT(src[alpha], *m, tmp);
          m++;
          src += bytes;
        }
    }
  else
    {
      while (w--)
        {
          gint tmp;

          *dest++ = INT_MULT(src[alpha], OPAQUE_OPACITY, tmp);
          src += bytes;
        }
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
replace_pixels (const guchar   *src1,
                const guchar   *src2,
                guchar         *dest,
                const guchar   *mask,
                gint            length,
                gint            opacity,
                const gboolean *affect,
                gint            bytes1,
                gint            bytes2)
{
  const gint    alpha        = bytes1 - 1;
  const gdouble norm_opacity = opacity * (1.0 / 65536.0);

  if (bytes1 != bytes2)
    {
      g_warning ("replace_pixels only works on commensurate pixel regions");
      return;
    }

  while (length --)
    {
      guint    b;
      gdouble  mask_val = mask[0] * norm_opacity;

      /* calculate new alpha first. */
      gint     s1_a  = src1[alpha];
      gint     s2_a  = src2[alpha];
      gdouble  a_val = s1_a + mask_val * (s2_a - s1_a);

      if (a_val == 0)
        /* In any case, write out versions of the blending function */
        /* that result when combinations of s1_a, s2_a, and         */
        /* mask_val --> 0 (or mask_val -->1)                        */
        {
          /* Case 1: s1_a, s2_a, AND mask_val all approach 0+:               */
          /* Case 2: s1_a AND s2_a both approach 0+, regardless of mask_val: */

          if (s1_a + s2_a == 0.0)
            {
              for (b = 0; b < alpha; b++)
                {
                  gint new_val = 0.5 + (gdouble) src1[b] +
                    mask_val * ((gdouble) src2[b] - (gdouble) src1[b]);

                  dest[b] = affect[b] ? MIN (new_val, 255) : src1[b];
                }
            }

          /* Case 3: mask_val AND s1_a both approach 0+, regardless of s2_a  */
          else if (s1_a + mask_val == 0.0)
            {
              for (b = 0; b < alpha; b++)
                dest[b] = src1[b];
            }

          /* Case 4: mask_val -->1 AND s2_a -->0, regardless of s1_a         */
          else if (1.0 - mask_val + s2_a == 0.0)
            {
              for (b = 0; b < alpha; b++)
                dest[b] = affect[b] ? src2[b] : src1[b];
            }
        }
      else
        {
          gdouble a_recip = 1.0 / a_val;
          /* possible optimization: fold a_recip into s1_a and s2_a           */
          for (b = 0; b < alpha; b++)
            {
              gint new_val = 0.5 + a_recip * (src1[b] * s1_a + mask_val *
                                              (src2[b] * s2_a - src1[b] * s1_a));
              dest[b] = affect[b] ? MIN (new_val, 255) : src1[b];
            }
        }

      dest[alpha] = affect[alpha] ? a_val + 0.5: s1_a;
      src1 += bytes1;
      src2 += bytes2;
      dest += bytes2;
      mask++;
    }
}

inline void
swap_pixels (guchar *src,
             guchar *dest,
             guint   length)
{
  while (length--)
    {
      guchar tmp = *dest;

      *dest = *src;
      *src = tmp;

      src++;
      dest++;
    }
}

inline void
scale_pixels (const guchar *src,
              guchar       *dest,
              guint         length,
              gint          scale)
{
  gint tmp;

  while (length --)
    {
      *dest++ = (guchar) INT_MULT (*src, scale, tmp);
      src++;
    }
}

inline void
add_alpha_pixels (const guchar *src,
                  guchar       *dest,
                  guint         length,
                  guint         bytes)
{
  const guint alpha = bytes + 1;

  while (length --)
    {
      guint b;

      for (b = 0; b < bytes; b++)
        dest[b] = src[b];

      dest[b] = OPAQUE_OPACITY;

      src += bytes;
      dest += alpha;
    }
}


inline void
flatten_pixels (const guchar *src,
                guchar       *dest,
                const guchar *bg,
                guint         length,
                guint         bytes)
{
  const guint alpha = bytes - 1;

  while (length --)
    {
      guint b;

      for (b = 0; b < alpha; b++)
        {
          gint t1, t2;

          dest[b] = (INT_MULT (src[b], src[alpha], t1) +
                     INT_MULT (bg[b], (255 - src[alpha]), t2));
        }

      src += bytes;
      dest += alpha;
    }
}


inline void
gray_to_rgb_pixels (const guchar *src,
                    guchar       *dest,
                    guint         length,
                    guint         bytes)
{
  const gboolean has_alpha  = (bytes == 2) ? TRUE : FALSE;
  const gint     dest_bytes = (has_alpha) ? 4 : 3;

  while (length --)
    {
      guint b;

      for (b = 0; b < bytes; b++)
        dest[b] = src[0];

      if (has_alpha)
        dest[3] = src[1];

      src += bytes;
      dest += dest_bytes;
    }
}


inline void
apply_mask_to_alpha_channel (guchar       *src,
                             const guchar *mask,
                             guint         opacity,
                             guint         length,
                             guint         bytes)
{
  src += bytes - 1;

  if (opacity == 255)
    {
      while (length --)
        {
          glong tmp;

          *src = INT_MULT(*src, *mask, tmp);
          mask++;
          src += bytes;
        }
    }
  else
    {
      while (length --)
        {
          glong tmp;

          *src = INT_MULT3(*src, *mask, opacity, tmp);
          mask++;
          src += bytes;
        }
    }
}


inline void
combine_mask_and_alpha_channel_stipple (guchar       *src,
                                        const guchar *mask,
                                        guint         opacity,
                                        guint         length,
                                        guint         bytes)
{
  /* align with alpha channel */
  src += bytes - 1;

  if (opacity != 255)
    while (length --)
      {
        gint tmp;
        gint mask_val = INT_MULT(*mask, opacity, tmp);

        *src = *src + INT_MULT((255 - *src) , mask_val, tmp);

        src += bytes;
        mask++;
      }
  else
    while (length --)
      {
        gint tmp;

        *src = *src + INT_MULT((255 - *src) , *mask, tmp);

        src += bytes;
        mask++;
      }
}


inline void
combine_mask_and_alpha_channel_stroke (guchar       *src,
                                       const guchar *mask,
                                       guint         opacity,
                                       guint         length,
                                       guint         bytes)
{
  /* align with alpha channel */
  src += bytes - 1;

  if (opacity != 255)
    while (length --)
      {
        if (opacity > *src)
          {
            gint tmp;
            gint mask_val = INT_MULT (*mask, opacity, tmp);

            *src = *src + INT_MULT ((opacity - *src) , mask_val, tmp);
          }

        src += bytes;
        mask++;
      }
  else
    while (length --)
      {
        gint tmp;

        *src = *src + INT_MULT ((255 - *src) , *mask, tmp);

        src += bytes;
        mask++;
      }
}


inline void
copy_gray_to_inten_a_pixels (const guchar *src,
                             guchar       *dest,
                             guint         length,
                             guint         bytes)
{
  const guint alpha = bytes - 1;

  while (length --)
    {
      guint b;

      for (b = 0; b < alpha; b++)
        dest[b] = *src;
      dest[b] = OPAQUE_OPACITY;

      src ++;
      dest += bytes;
    }
}


inline void
initial_channel_pixels (const guchar *src,
                        guchar       *dest,
                        guint         length,
                        guint         bytes)
{
  const guint alpha = bytes - 1;

  while (length --)
    {
      guint b;

      for (b = 0; b < alpha; b++)
        dest[b] = src[0];

      dest[alpha] = OPAQUE_OPACITY;

      dest += bytes;
      src ++;
    }
}


inline void
initial_indexed_pixels (const guchar *src,
                        guchar       *dest,
                        const guchar *cmap,
                        guint         length)
{
  /*  This function assumes always that we're mapping from
   *  an RGB colormap to an RGBA image...
   */
  while (length--)
    {
      gint col_index = *src++ * 3;

      *dest++ = cmap[col_index++];
      *dest++ = cmap[col_index++];
      *dest++ = cmap[col_index++];
      *dest++ = OPAQUE_OPACITY;
    }
}


inline void
initial_indexed_a_pixels (const guchar *src,
                          guchar       *dest,
                          const guchar *mask,
                          const guchar *no_mask,
                          const guchar *cmap,
                          guint         opacity,
                          guint         length)
{
  const guchar *m = mask ? mask : no_mask;

  while (length --)
    {
      gint   col_index = *src++ * 3;
      glong  tmp;
      guchar new_alpha = INT_MULT3(*src, *m, opacity, tmp);

      src++;

      *dest++ = cmap[col_index++];
      *dest++ = cmap[col_index++];
      *dest++ = cmap[col_index++];
      /*  Set the alpha channel  */
      *dest++ = (new_alpha > 127) ? OPAQUE_OPACITY : TRANSPARENT_OPACITY;

      if (mask)
        m++;
    }
}


inline void
initial_inten_pixels (const guchar   *src,
                      guchar         *dest,
                      const guchar   *mask,
                      const guchar   *no_mask,
                      guint           opacity,
                      const gboolean *affect,
                      guint           length,
                      guint           bytes)
{
  const guchar *srcp;
  const gint    dest_bytes = bytes + 1;
  guchar       *destp;
  gint          b, l;

  if (mask)
    {
      const guchar *m = mask;

      /*  This function assumes the source has no alpha channel and
       *  the destination has an alpha channel.  So dest_bytes = bytes + 1
       */

      if (bytes == 3 && affect[0] && affect[1] && affect[2])
        {
          if (!affect[bytes])
            opacity = 0;

          destp = dest + bytes;

          if (opacity != 0)
            while(length--)
              {
                gint  tmp;

                dest[0] = src[0];
                dest[1] = src[1];
                dest[2] = src[2];
                dest[3] = INT_MULT (opacity, *m, tmp);
                src  += bytes;
                dest += dest_bytes;
                m++;
              }
          else
            while(length--)
              {
                dest[0] = src[0];
                dest[1] = src[1];
                dest[2] = src[2];
                dest[3] = opacity;
                src  += bytes;
                dest += dest_bytes;
              }

          return;
        }

      for (b = 0; b < bytes; b++)
        {
          destp = dest + b;
          srcp = src + b;
          l = length;

          if (affect[b])
            while(l--)
              {
                *destp = *srcp;
                srcp  += bytes;
                destp += dest_bytes;
              }
          else
            while(l--)
              {
                *destp = 0;
                destp += dest_bytes;
              }
        }

      /* fill the alpha channel */
      if (!affect[bytes])
        opacity = 0;

      destp = dest + bytes;

      if (opacity != 0)
        while (length--)
          {
            gint  tmp;

            *destp = INT_MULT(opacity , *m, tmp);
            destp += dest_bytes;
            m++;
          }
      else
        while (length--)
          {
            *destp = opacity;
            destp += dest_bytes;
          }
    }
  else  /* no mask */
    {
      /*  This function assumes the source has no alpha channel and
       *  the destination has an alpha channel.  So dest_bytes = bytes + 1
       */

      if (bytes == 3 && affect[0] && affect[1] && affect[2])
        {
          if (!affect[bytes])
            opacity = 0;

          destp = dest + bytes;

          while(length--)
            {
              dest[0] = src[0];
              dest[1] = src[1];
              dest[2] = src[2];
              dest[3] = opacity;
              src  += bytes;
              dest += dest_bytes;
            }
          return;
        }

      for (b = 0; b < bytes; b++)
        {
          destp = dest + b;
          srcp = src + b;
          l = length;

          if (affect[b])
            while(l--)
              {
                *destp = *srcp;
                srcp  += bytes;
                destp += dest_bytes;
              }
          else
            while(l--)
              {
                *destp = 0;
                destp += dest_bytes;
              }
      }

      /* fill the alpha channel */
      if (!affect[bytes])
        opacity = 0;

      destp = dest + bytes;

      while (length--)
        {
          *destp = opacity;
          destp += dest_bytes;
        }
    }
}


inline void
initial_inten_a_pixels (const guchar   *src,
                        guchar         *dest,
                        const guchar   *mask,
                        guint           opacity,
                        const gboolean *affect,
                        guint           length,
                        guint           bytes)
{
  const guint alpha = bytes - 1;

  if (mask)
    {
      const guchar *m = mask;

      while (length--)
        {
          guint b;
          glong tmp;

          for (b = 0; b < alpha; b++)
            dest[b] = src[b] * affect[b];

          /*  Set the alpha channel  */
          dest[alpha] = (affect [alpha] ?
                         INT_MULT3(opacity, src[alpha], *m, tmp) : 0);

          m++;

          dest += bytes;
          src += bytes;
        }
    }
  else
    {
      while (length--)
        {
          guint b;
          glong tmp;

          for (b = 0; b < alpha; b++)
            dest[b] = src[b] * affect[b];

          /*  Set the alpha channel  */
          dest[alpha] = (affect [alpha] ?
                         INT_MULT(opacity , src[alpha], tmp) : 0);

          dest += bytes;
          src += bytes;
        }
    }
}

inline void
copy_component_pixels (const guchar *src,
                       guchar       *dest,
                       guint         length,
                       guint         bytes,
                       guint         pixel)
{
  src += pixel;

  while (length --)
    {
      *dest = *src;

      src += bytes;
      dest++;
    }
}

inline void
copy_color_pixels (const guchar *src,
                   guchar       *dest,
                   guint         length,
                   guint         bytes)
{
  const guint alpha = bytes - 1;

  while (length --)
    {
      guint b;

      for (b = 0; b < alpha; b++)
        dest[b] = src[b];

      src  += bytes;
      dest += bytes - 1;
    }
}


static void
layer_normal_mode (struct apply_layer_mode_struct *alms)
{
  /*  assumes we're applying src2 TO src1  */
  *(alms->dest) = alms->src2;
}

static void
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

static void
layer_multiply_mode (struct apply_layer_mode_struct *alms)
{
  multiply_pixels (alms->src1, alms->src2, *(alms->dest), alms->length,
                   alms->bytes1, alms->bytes2);
}

static void
layer_divide_mode (struct apply_layer_mode_struct *alms)
{
  divide_pixels (alms->src1, alms->src2, *(alms->dest), alms->length,
                 alms->bytes1, alms->bytes2);
}

static void
layer_screen_mode (struct apply_layer_mode_struct *alms)
{
  screen_pixels (alms->src1, alms->src2, *(alms->dest), alms->length,
                 alms->bytes1, alms->bytes2);
}

static void
layer_overlay_mode (struct apply_layer_mode_struct *alms)
{
  overlay_pixels (alms->src1, alms->src2, *(alms->dest), alms->length,
                  alms->bytes1, alms->bytes2);
}

static void
layer_difference_mode (struct apply_layer_mode_struct *alms)
{
  difference_pixels (alms->src1, alms->src2, *(alms->dest), alms->length,
                     alms->bytes1, alms->bytes2);
}

static void
layer_addition_mode (struct apply_layer_mode_struct *alms)
{
  add_pixels (alms->src1, alms->src2, *(alms->dest), alms->length,
              alms->bytes1, alms->bytes2);
}

static void
layer_subtract_mode (struct apply_layer_mode_struct *alms)
{
  subtract_pixels (alms->src1, alms->src2, *(alms->dest), alms->length,
                   alms->bytes1, alms->bytes2);
}

static void
layer_darken_only_mode (struct apply_layer_mode_struct *alms)
{
  darken_pixels (alms->src1, alms->src2, *(alms->dest), alms->length,
                 alms->bytes1, alms->bytes2);
}

static void
layer_lighten_only_mode (struct apply_layer_mode_struct *alms)
{
  lighten_pixels (alms->src1, alms->src2, *(alms->dest), alms->length,
                  alms->bytes1, alms->bytes2);
}

static void
layer_hue_mode (struct apply_layer_mode_struct *alms)
{
  /*  only works on RGB color images  */
  if (alms->bytes1 > 2)
    hue_only_pixels (alms->src1, alms->src2, *(alms->dest), alms->length,
                     alms->bytes1, alms->bytes2);
  else
    *(alms->dest) = alms->src2;
}

static void
layer_saturation_mode (struct apply_layer_mode_struct *alms)
{
  /*  only works on RGB color images  */
  if (alms->bytes1 > 2)
    saturation_only_pixels (alms->src1, alms->src2, *(alms->dest),
                            alms->length, alms->bytes1, alms->bytes2);
  else
    *(alms->dest) = alms->src2;
}

static void
layer_value_mode (struct apply_layer_mode_struct *alms)
{
  /*  only works on RGB color images  */
  if (alms->bytes1 > 2)
    value_only_pixels (alms->src1, alms->src2, *(alms->dest), alms->length,
                       alms->bytes1, alms->bytes2);
  else
    *(alms->dest) = alms->src2;
}

static void
layer_color_mode (struct apply_layer_mode_struct *alms)
{
  /*  only works on RGB color images  */
  if (alms->bytes1 > 2)
    color_only_pixels (alms->src1, alms->src2, *(alms->dest), alms->length,
                       alms->bytes1, alms->bytes2);
  else
    *(alms->dest) = alms->src2;
}

static void
layer_behind_mode (struct apply_layer_mode_struct *alms)
{
  *(alms->dest) = alms->src2;
  if (HAS_ALPHA (alms->bytes1))
    alms->combine = BEHIND_INTEN;
  else
    alms->combine = NO_COMBINATION;
}

static void
layer_replace_mode (struct apply_layer_mode_struct *alms)
{
  *(alms->dest) = alms->src2;
  alms->combine = REPLACE_INTEN;
}

static void
layer_erase_mode (struct apply_layer_mode_struct *alms)
{
  *(alms->dest) = alms->src2;
  /*  If both sources have alpha channels, call erase function.
   *  Otherwise, just combine in the normal manner
   */
  alms->combine =
    (HAS_ALPHA (alms->bytes1) && HAS_ALPHA (alms->bytes2)) ?  ERASE_INTEN : 0;
}

static void
layer_anti_erase_mode (struct apply_layer_mode_struct *alms)
{
  *(alms->dest) = alms->src2;
  alms->combine =
    (HAS_ALPHA (alms->bytes1) && HAS_ALPHA (alms->bytes2)) ? ANTI_ERASE_INTEN : 0;
}

static void
layer_color_erase_mode (struct apply_layer_mode_struct *alms)
{
  *(alms->dest) = alms->src2;
  alms->combine =
    (HAS_ALPHA (alms->bytes1) && HAS_ALPHA (alms->bytes2)) ? COLOR_ERASE_INTEN : 0;
}

static void
layer_dodge_mode (struct apply_layer_mode_struct *alms)
{
  dodge_pixels (alms->src1, alms->src2, *(alms->dest), alms->length,
                alms->bytes1, alms->bytes2);
}

static void
layer_burn_mode (struct apply_layer_mode_struct *alms)
{
  burn_pixels (alms->src1, alms->src2, *(alms->dest), alms->length,
               alms->bytes1, alms->bytes2);
}

static void
layer_hardlight_mode (struct apply_layer_mode_struct *alms)
{
  hardlight_pixels (alms->src1, alms->src2, *(alms->dest), alms->length,
                    alms->bytes1, alms->bytes2);
}

static void
layer_softlight_mode (struct apply_layer_mode_struct *alms)
{
  softlight_pixels (alms->src1, alms->src2, *(alms->dest), alms->length,
                    alms->bytes1, alms->bytes2);
}

static void
layer_grain_extract_mode (struct apply_layer_mode_struct *alms)
{
  grain_extract_pixels (alms->src1, alms->src2, *(alms->dest), alms->length,
                        alms->bytes1, alms->bytes2);
}

static void
layer_grain_merge_mode (struct apply_layer_mode_struct *alms)
{
  grain_merge_pixels (alms->src1, alms->src2, *(alms->dest), alms->length,
                      alms->bytes1, alms->bytes2);
}

#endif  /*  __PAINT_FUNCS_GENERIC_H__  */
