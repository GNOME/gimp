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


#endif  /*  __PAINT_FUNCS_GENERIC_H__  */
