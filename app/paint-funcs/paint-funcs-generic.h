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

/*
 * This file is supposed to contain the generic (read: C) implementation
 * of the pixel fiddling paint-functions.
 */

#ifndef __PAINT_FUNCS_GENERIC_H__
#define __PAINT_FUNCS_GENERIC_H__


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
  pat = (temp_buf_get_data (pattern) +
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


#endif  /*  __PAINT_FUNCS_GENERIC_H__  */
