/* The GIMP -- an image manipulation program
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
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "appenv.h"
#include "boundary.h"
#include "tag.h"
#include "gimprc.h"
#include "paint.h"
#include "paint_funcs_row.h"
#include "pixelrow.h"


#define EPSILON            0.0001
#define INT_MULT(a,b,t)  ((t) = (a) * (b) + 0x80, ((((t) >> 8) + (t)) >> 8))


#if 0
static unsigned char no_mask = OPAQUE;
#endif


void 
color_row  (
            PixelRow * row,
            Paint * color
            )
{
  guchar *src   = paint_data (color);
  guchar *dest  = pixelrow_getdata (row, 0);
  int     bytes = paint_bytes (color);
  int     w     = pixelrow_width (row);

  while (w--)
    {
      guchar *s = src;
      int b = bytes;
      while (b--)
        *dest++ = *s++;
    }
}


void 
blend_row  (
            PixelRow * src1,
            PixelRow * src2,
            PixelRow * dest,
            Paint * blend
            )
{
#if 0
  int alpha, b;
  unsigned char blend2 = (255 - blend);

  alpha = (has_alpha) ? bytes - 1 : bytes;
  while (w --)
    {
      for (b = 0; b < alpha; b++)
	dest[b] = (src1[b] * blend2 + src2[b] * blend) / 255;

      if (has_alpha)
	dest[alpha] = src1[alpha];  /*  alpha channel--assume src2 has none  */

      src1 += bytes;
      src2 += bytes;
      dest += bytes;
    }
#endif
}


void 
shade_row  (
            PixelRow * src,
            PixelRow * dest,
            Paint * col
            )
{
#if 0
  int alpha, b;
  unsigned char blend2 = (255 - blend);

  alpha = (has_alpha) ? bytes - 1 : bytes;
  while (w --)
    {
      for (b = 0; b < alpha; b++)
	dest[b] = (src[b] * blend2 + col[b] * blend) / 255;

      if (has_alpha)
	dest[alpha] = src[alpha];  /* alpha channel */

      src += bytes;
      dest += bytes;
    }
#endif
}

void 
extract_alpha_row  (
                    PixelRow * src,
                    PixelRow * mask,
                    PixelRow * dest
                    )
{
#if 0
  int alpha;
  unsigned char * m;

  if (mask)
    m = mask;
  else
    m = &no_mask;

  alpha = bytes - 1;
  while (w --)
    {
      *dest++ = (src[alpha] * *m) / 255;

      if (mask)
	m++;
      src += bytes;
    }
#endif
}


void 
darken_row  (
             PixelRow * src1,
             PixelRow * src2,
             PixelRow * dest
             )
{
#if 0
  int b, alpha;
  unsigned char s1, s2;

  alpha = (ha1 || ha2) ? MAX (b1, b2) - 1 : b1;

  while (length--)
    {
      for (b = 0; b < alpha; b++)
	{
	  s1 = src1[b];
	  s2 = src2[b];
	  dest[b] = (s1 < s2) ? s1 : s2;
	}

      if (ha1 && ha2)
	dest[alpha] = MIN (src1[alpha], src2[alpha]);
      else if (ha2)
	dest[alpha] = src2[alpha];

      src1 += b1;
      src2 += b2;
      dest += b2;
    }
#endif
}


void 
lighten_row  (
              PixelRow * src1,
              PixelRow * src2,
              PixelRow * dest
              )
{
#if 0
  int b, alpha;
  unsigned char s1, s2;

  alpha = (ha1 || ha2) ? MAX (b1, b2) - 1 : b1;

  while (length--)
    {
      for (b = 0; b < alpha; b++)
	{
	  s1 = src1[b];
	  s2 = src2[b];
	  dest[b] = (s1 < s2) ? s2 : s1;
	}

      if (ha1 && ha2)
	dest[alpha] = MIN (src1[alpha], src2[alpha]);
      else if (ha2)
	dest[alpha] = src2[alpha];

      src1 += b1;
      src2 += b2;
      dest += b2;
    }
#endif
}


void 
hsv_only_row  (
               PixelRow * src1,
               PixelRow * src2,
               PixelRow * dest,
               int mode
               )
{
#if 0
  int r1, g1, b1;
  int r2, g2, b2;

  /*  assumes inputs are only 4 byte RGBA pixels  */
  while (length--)
    {
      r1 = src1[0]; g1 = src1[1]; b1 = src1[2];
      r2 = src2[0]; g2 = src2[1]; b2 = src2[2];
      rgb_to_hsv (&r1, &g1, &b1);
      rgb_to_hsv (&r2, &g2, &b2);

      switch (mode)
	{
	case HUE_MODE:
	  r1 = r2;
	  break;
	case SATURATION_MODE:
	  g1 = g2;
	  break;
	case VALUE_MODE:
	  b1 = b2;
	  break;
	}

      /*  set the destination  */
      hsv_to_rgb (&r1, &g1, &b1);

      dest[0] = r1; dest[1] = g1; dest[2] = b1;

      if (ha1 && ha2)
	dest[3] = MIN (src1[3], src2[3]);
      else if (ha2)
	dest[3] = src2[3];

      src1 += bytes1;
      src2 += bytes2;
      dest += bytes2;
    }
#endif
}


void 
color_only_row  (
                 PixelRow * src1,
                 PixelRow * src2,
                 PixelRow * dest,
                 int mode
                 )
{
#if 0
  int r1, g1, b1;
  int r2, g2, b2;

  /*  assumes inputs are only 4 byte RGBA pixels  */
  while (length--)
    {
      r1 = src1[0]; g1 = src1[1]; b1 = src1[2];
      r2 = src2[0]; g2 = src2[1]; b2 = src2[2];
      rgb_to_hls (&r1, &g1, &b1);
      rgb_to_hls (&r2, &g2, &b2);

      /*  transfer hue and saturation to the source pixel  */
      r1 = r2;
      b1 = b2;

      /*  set the destination  */
      hls_to_rgb (&r1, &g1, &b1);

      dest[0] = r1; dest[1] = g1; dest[2] = b1;

      if (ha1 && ha2)
	dest[3] = MIN (src1[3], src2[3]);
      else if (ha2)
	dest[3] = src2[3];

      src1 += bytes1;
      src2 += bytes2;
      dest += bytes2;
    }
#endif
}


void 
multiply_row  (
               PixelRow * src1,
               PixelRow * src2,
               PixelRow * dest
               )
{
#if 0
  int alpha, b;

  alpha = (ha1 || ha2) ? MAX (b1, b2) - 1 : b1;

  while (length --)
    {
      for (b = 0; b < alpha; b++)
	dest[b] = (src1[b] * src2[b]) / 255;

      if (ha1 && ha2)
	dest[alpha] = MIN (src1[alpha], src2[alpha]);
      else if (ha2)
	dest[alpha] = src2[alpha];

      src1 += b1;
      src2 += b2;
      dest += b2;
    }
#endif
}


void 
screen_row  (
             PixelRow * src1,
             PixelRow * src2,
             PixelRow * dest
             )
{
#if 0
  int alpha, b;

  alpha = (ha1 || ha2) ? MAX (b1, b2) - 1 : b1;

  while (length --)
    {
      for (b = 0; b < alpha; b++)
	dest[b] = 255 - ((255 - src1[b]) * (255 - src2[b])) / 255;

      if (ha1 && ha2)
	dest[alpha] = MIN (src1[alpha], src2[alpha]);
      else if (ha2)
	dest[alpha] = src2[alpha];

      src1 += b1;
      src2 += b2;
      dest += b2;
    }
#endif
}


void 
overlay_row  (
              PixelRow * src1,
              PixelRow * src2,
              PixelRow * dest
              )
{
#if 0
  int alpha, b;
  int screen, mult;

  alpha = (ha1 || ha2) ? MAX (b1, b2) - 1 : b1;

  while (length --)
    {
      for (b = 0; b < alpha; b++)
	{
	  screen = 255 - ((255 - src1[b]) * (255 - src2[b])) / 255;
	  mult = (src1[b] * src2[b]) / 255;
	  dest[b] = (screen * src1[b] + mult * (255 - src1[b])) / 255;
	}

      if (ha1 && ha2)
	dest[alpha] = MIN (src1[alpha], src2[alpha]);
      else if (ha2)
	dest[alpha] = src2[alpha];

      src1 += b1;
      src2 += b2;
      dest += b2;
    }
#endif
}


void 
add_row  (
          PixelRow * src1,
          PixelRow * src2,
          PixelRow * dest
          )
{
#if 0
  int alpha, b;
  int sum;

  alpha = (ha1 || ha2) ? MAX (b1, b2) - 1 : b1;

  while (length --)
    {
      for (b = 0; b < alpha; b++)
	{
	  sum = src1[b] + src2[b];
	  dest[b] = (sum > 255) ? 255 : sum;
	}

      if (ha1 && ha2)
	dest[alpha] = MIN (src1[alpha], src2[alpha]);
      else if (ha2)
	dest[alpha] = src2[alpha];

      src1 += b1;
      src2 += b2;
      dest += b2;
    }
#endif
}


void 
subtract_row  (
               PixelRow * src1,
               PixelRow * src2,
               PixelRow * dest
               )
{
#if 0
  int alpha, b;
  int diff;

  alpha = (ha1 || ha2) ? MAX (b1, b2) - 1 : b1;

  while (length --)
    {
      for (b = 0; b < alpha; b++)
	{
	  diff = src1[b] - src2[b];
	  dest[b] = (diff < 0) ? 0 : diff;
	}

      if (ha1 && ha2)
	dest[alpha] = MIN (src1[alpha], src2[alpha]);
      else if (ha2)
	dest[alpha] = src2[alpha];

      src1 += b1;
      src2 += b2;
      dest += b2;
    }
#endif
}


void 
difference_row  (
                 PixelRow * src1,
                 PixelRow * src2,
                 PixelRow * dest
                 )
{
#if 0
  int alpha, b;
  int diff;

  alpha = (ha1 || ha2) ? MAX (b1, b2) - 1 : b1;

  while (length --)
    {
      for (b = 0; b < alpha; b++)
	{
	  diff = src1[b] - src2[b];
	  dest[b] = (diff < 0) ? -diff : diff;
	}

      if (ha1 && ha2)
	dest[alpha] = MIN (src1[alpha], src2[alpha]);
      else if (ha2)
	dest[alpha] = src2[alpha];

      src1 += b1;
      src2 += b2;
      dest += b2;
    }
#endif
}


void 
dissolve_row  (
               PixelRow * src,
               PixelRow * dest,
               Paint * opacity,
               int x,
               int y
               )
{
#if 0
  int alpha, b;
  int rand_val;

  /*  Set up the random number generator  */
  paint_funcs_area_randomize(y);
  for (b = 0; b < x; b++)
    rand ();

  alpha = db - 1;

  while (length --)
    {
      /*  preserve the intensity values  */
      for (b = 0; b < alpha; b++)
	dest[b] = src[b];

      /*  dissolve if random value is > opacity  */
      rand_val = (rand() & 0xFF);

      if (has_alpha)
	dest[alpha] = (rand_val > opacity) ? 0 : src[alpha];
      else
	dest[alpha] = (rand_val > opacity) ? 0 : OPAQUE;

      dest += db;
      src += sb;
    }
#endif
}


void 
replace_row  (
              PixelRow * src1,
              PixelRow * src2,
              PixelRow * dest,
              PixelRow * mask,
              Paint * opacity
              )
{
#if 0
  int alpha;
  int b;
  double a_val, a_recip, mask_val;
  double norm_opacity;
  int s1_a, s2_a;
  int new_val;

  if (b1 != b2)
    {
      g_warning ("replace_row only works on commensurate pixel regions");
      return;
    }

  alpha = b1 - 1;
  norm_opacity = opacity * (1.0 / 65025.0);

  while (length --)
    {
      mask_val = mask[0] * norm_opacity;
      /* calculate new alpha first. */
      s1_a = src1[alpha];
      s2_a = src2[alpha];
      a_val = s1_a + mask_val * (s2_a - s1_a);
      if (a_val == 0)
	a_recip = 0;
      else
	a_recip = 1.0 / a_val;
      /* possible optimization: fold a_recip into s1_a and s2_a */
      for (b = 0; b < alpha; b++)
	{
	  new_val = 0.5 + a_recip * (src1[b] * s1_a + mask_val *
				     (src2[b] * s2_a - src1[b] * s1_a));
	  dest[b] = affect[b] ? MIN (new_val, 255) : src1[b];
	}
      dest[alpha] = affect[alpha] ? a_val + 0.5: s1_a;
      src1 += b1;
      src2 += b2;
      dest += b2;
      mask++;
    }
#endif
}


/* used in undo */
void 
swap_row  (
           PixelRow * src,
           PixelRow * dest
           )
{
#if 0
  while (length--)
    {
      *src = *src ^ *dest;
      *dest = *dest ^ *src;
      *src = *src ^ *dest;
      src++;
      dest++;
    }
#endif
}

void 
scale_row  (
            PixelRow * src,
            PixelRow * dest,
            gfloat scale
            )
{
#if 0
  while (length --)
    *dest++ = (unsigned char) ((*src++ * scale) / 255);
#endif
}


void 
add_alpha_row  (
                PixelRow * src,
                PixelRow * dest
                )
{
#if 0
  int alpha, b;

  alpha = bytes + 1;
  while (length --)
    {
      for (b = 0; b < bytes; b++)
	dest[b] = src[b];

      dest[b] = OPAQUE;

      src += bytes;
      dest += alpha;
    }
#endif
}


void 
flatten_row  (
              PixelRow * src,
              PixelRow * dest,
              Paint * bgcol
              )
{
#if 0
  int alpha, b;
  int t1, t2;

  alpha = bytes - 1;
  while (length --)
    {
      for (b = 0; b < alpha; b++)
	dest[b] = INT_MULT (src[b], src[alpha], t1) + INT_MULT (bg[b], (255 - src[alpha]), t2);

      src += bytes;
      dest += alpha;
    }
#endif
}



static void 
u8_apply_mask_to_alpha_row  (
                             PixelRow * src,
                             PixelRow * mask,
                             Paint * opacity
                             )
{
  guchar *s_data = pixelrow_getdata (src, 0);
  guchar *m_data = pixelrow_getdata (mask, 0);
  int w          = pixelrow_width (src);
  int o          = ((guchar *) paint_data (opacity))[0];

  switch (tag_format (pixelrow_tag (src)))
    {
    case FORMAT_RGB:
      while (w --)
        {
          s_data[3] = (s_data[3] * *m_data++ * o) / 65025;
          s_data += 4;
        }
      break;

    case FORMAT_GRAY:
    case FORMAT_INDEXED:
      while (w --)
        {
          s_data[1] = (s_data[1] * *m_data++ * o) / 65025;
          s_data += 2;
        }
      break;
      
    case FORMAT_NONE:
    default:
      break;
    }
}


static void 
u16_apply_mask_to_alpha_row  (
                              PixelRow * src,
                              PixelRow * mask,
                              Paint * opacity
                              )
{
}


static void 
float_apply_mask_to_alpha_row  (
                                PixelRow * src,
                                PixelRow * mask,
                                Paint * opacity
                                )
{
}


void 
apply_mask_to_alpha_row  (
                          PixelRow * src,
                          PixelRow * mask,
                          Paint * opacity
                          )
{
  Tag s_tag = pixelrow_tag (src);
  Tag m_tag = pixelrow_tag (mask);
  Tag o_tag = paint_tag (opacity);
  
  if ((tag_precision (s_tag) != tag_precision (m_tag)) ||
      (tag_precision (s_tag) != tag_precision (o_tag)) ||
      (tag_alpha (s_tag)     != ALPHA_YES) ||
      (tag_format (m_tag)    != FORMAT_GRAY) ||
      (tag_format (o_tag)    != FORMAT_GRAY))
    return;

  switch (tag_precision (s_tag))
    {
    case PRECISION_U8:
      return u8_apply_mask_to_alpha_row (src, mask, opacity);
    case PRECISION_U16:
      return u16_apply_mask_to_alpha_row (src, mask, opacity);
    case PRECISION_FLOAT:
      return float_apply_mask_to_alpha_row (src, mask, opacity);
    case PRECISION_NONE:
    default:
      break;
    }
}


void 
combine_mask_and_alpha_row  (
                             PixelRow * src,
                             PixelRow * mask,
                             Paint * opacity
                             )
{
#if 0
  unsigned char mask_val;
  int alpha;

  alpha = bytes - 1;
  while (length --)
    {
      mask_val = (*mask++ * opacity) / 255;
      src[alpha] = src[alpha] + ((255 - src[alpha]) * mask_val) / 255;
      src += bytes;
    }
#endif
}


void 
copy_gray_to_inten_a_row  (
                           PixelRow * src,
                           PixelRow * dest
                           )
{
#if 0
  int b;
  int alpha;

  alpha = bytes - 1;
  while (length --)
    {
      for (b = 0; b < alpha; b++)
	dest[b] = *src;
      dest[b] = OPAQUE;

      src ++;
      dest += bytes;
    }
#endif
}


void 
initial_channel_row  (
                      PixelRow * src,
                      PixelRow * dest
                      )
{
#if 0
  int alpha, b;

  alpha = bytes - 1;
  while (length --)
    {
      for (b = 0; b < alpha; b++)
	dest[b] = src[0];

      dest[alpha] = OPAQUE;

      dest += bytes;
      src ++;
    }
#endif
}


void 
initial_indexed_row  (
                      PixelRow * src,
                      PixelRow * dest,
                      unsigned char * cmap
                      )
{
#if 0
  int col_index;

  /*  This function assumes always that we're mapping from
   *  an RGB colormap to an RGBA image...
   */
  while (length--)
    {
      col_index = *src++ * 3;
      *dest++ = cmap[col_index++];
      *dest++ = cmap[col_index++];
      *dest++ = cmap[col_index++];
      *dest++ = OPAQUE;
    }
#endif
}


void 
initial_indexed_a_row  (
                        PixelRow * src,
                        PixelRow * dest,
                        PixelRow * mask,
                        Paint * opacity,
                        unsigned char * cmap
                        )
{
#if 0
  int col_index;
  unsigned char new_alpha;
  unsigned char * m;

  if (mask)
    m = mask;
  else
    m = &no_mask;

  while (length --)
    {
      col_index = *src++ * 3;
      new_alpha = (*src++ * *m * opacity) / 65025;

      *dest++ = cmap[col_index++];
      *dest++ = cmap[col_index++];
      *dest++ = cmap[col_index++];
      /*  Set the alpha channel  */
      *dest++ = (new_alpha > 127) ? OPAQUE : TRANSPARENT;

      if (mask)
	m++;
    }
#endif
}


void 
initial_inten_row  (
                    PixelRow * src,
                    PixelRow * dest,
                    PixelRow * mask,
                    Paint * opacity
                    )
{
#if 0
  int b, dest_bytes;
  unsigned char * m;

  if (mask)
    m = mask;
  else
    m = &no_mask;

  /*  This function assumes the source has no alpha channel and
   *  the destination has an alpha channel.  So dest_bytes = bytes + 1
   */
  dest_bytes = bytes + 1;

  if (mask)
    {
      while (length --)
	{
	  for (b = 0; b < bytes; b++)
	    dest [b] = affect [b] ? src [b] : 0;
	    
	  /*  Set the alpha channel  */
	  dest[b] = affect [b] ? (opacity * *m) / 255 : 0;
	    
	  m++;
	  dest += dest_bytes;
	  src += bytes;
	}
    }
  else
    {
      while (length --)
	{
	  for (b = 0; b < bytes; b++)
	    dest [b] = affect [b] ? src [b] : 0;
	    
	  /*  Set the alpha channel  */
	  dest[b] = affect [b] ? opacity : 0;
	    
	  dest += dest_bytes;
	  src += bytes;
	}
    }
#endif
}


void 
initial_inten_a_row  (
                      PixelRow * src,
                      PixelRow * dest,
                      PixelRow * mask,
                      Paint * opacity
                      )
{
#if 0
  int alpha, b;
  unsigned char * m;

  alpha = bytes - 1;
  if (mask)
    {
      m = mask;
      while (length --)
	{
	  for (b = 0; b < alpha; b++)
	    dest[b] = src[b] * affect[b];
	  
	  /*  Set the alpha channel  */
	  dest[alpha] = affect [alpha] ? (opacity * src[alpha] * *m) / 65025 : 0;
	  
	  m++;
	  
	  dest += bytes;
	  src += bytes;
	}
    }
  else
    {
      while (length --)
	{
	  for (b = 0; b < alpha; b++)
	    dest[b] = src[b] * affect[b];
	  
	  /*  Set the alpha channel  */
	  dest[alpha] = affect [alpha] ? (opacity * src[alpha]) / 255 : 0;
	  
	  dest += bytes;
	  src += bytes;
	}
    }
#endif
}


void 
combine_indexed_and_indexed_row  (
                                  PixelRow * src1,
                                  PixelRow * src2,
                                  PixelRow * dest,
                                  PixelRow * mask,
                                  Paint * opacity
                                  )
{
#if 0
  int b;
  unsigned char new_alpha;
  unsigned char * m;

  if (mask)
    {
      m = mask;
      while (length --)
	{
	  new_alpha = (*m * opacity) / 255;
	  
	  for (b = 0; b < bytes; b++)
	    dest[b] = (affect[b] && new_alpha > 127) ? src2[b] : src1[b];
	  
	  m++;
	  
	  src1 += bytes;
	  src2 += bytes;
	  dest += bytes;
	}
    }
  else
    {
      while (length --)
	{
	  new_alpha = opacity;
	  
	  for (b = 0; b < bytes; b++)
	    dest[b] = (affect[b] && new_alpha > 127) ? src2[b] : src1[b];
	  
	  src1 += bytes;
	  src2 += bytes;
	  dest += bytes;
	}
    }
#endif
}


void 
combine_indexed_and_indexed_a_row  (
                                    PixelRow * src1,
                                    PixelRow * src2,
                                    PixelRow * dest,
                                    PixelRow * mask,
                                    Paint * opacity
                                    )
{
#if 0
  int b, alpha;
  unsigned char new_alpha;
  unsigned char * m;
  int src2_bytes;

  alpha = 1;
  src2_bytes = 2;

  if (mask)
    {
      m = mask;
      while (length --)
	{
	  new_alpha = (src2[alpha] * *m * opacity) / 65025;

	  for (b = 0; b < bytes; b++)
	    dest[b] = (affect[b] && new_alpha > 127) ? src2[b] : src1[b];

	  m++;

	  src1 += bytes;
	  src2 += src2_bytes;
	  dest += bytes;
	}
    }
  else
    {
      while (length --)
	{
	  new_alpha = (src2[alpha] * opacity) / 255;

	  for (b = 0; b < bytes; b++)
	    dest[b] = (affect[b] && new_alpha > 127) ? src2[b] : src1[b];

	  src1 += bytes;
	  src2 += src2_bytes;
	  dest += bytes;
	}
    }
#endif
}


void 
combine_indexed_a_and_indexed_a_row  (
                                      PixelRow * src1,
                                      PixelRow * src2,
                                      PixelRow * dest,
                                      PixelRow * mask,
                                      Paint * opacity
                                      )
{
#if 0
  int b, alpha;
  unsigned char new_alpha;
  unsigned char * m;

  alpha = 1;

  if (mask)
    {
      m = mask;
      while (length --)
	{
	  new_alpha = (src2[alpha] * *m * opacity) / 65025;

	  for (b = 0; b < alpha; b++)
	    dest[b] = (affect[b] && new_alpha > 127) ? src2[b] : src1[b];

	  dest[alpha] = (affect[alpha] && new_alpha > 127) ? OPAQUE : src1[alpha];

	  m++;

	  src1 += bytes;
	  src2 += bytes;
	  dest += bytes;
	}
    }
  else
    {
      while (length --)
	{
	  new_alpha = (src2[alpha] * opacity) / 255;

	  for (b = 0; b < alpha; b++)
	    dest[b] = (affect[b] && new_alpha > 127) ? src2[b] : src1[b];

	  dest[alpha] = (affect[alpha] && new_alpha > 127) ? OPAQUE : src1[alpha];

	  src1 += bytes;
	  src2 += bytes;
	  dest += bytes;
	}
    }
#endif
}


void 
combine_inten_a_and_indexed_a_row  (
                                    PixelRow * src1,
                                    PixelRow * src2,
                                    PixelRow * dest,
                                    PixelRow * mask,
                                    Paint * opacity
                                    )
{
#if 0
  int b, alpha;
  unsigned char new_alpha;
  int src2_bytes;
  int index;

  alpha = 1;
  src2_bytes = 2;

  if (mask)
    {
      unsigned char *m = mask;
      while (length --)
	{
	  new_alpha = (src2[alpha] * *m * opacity) / 65025;

	  index = src2[0] * 3;

	  for (b = 0; b < bytes-1; b++)
	    dest[b] = (new_alpha > 127) ? cmap[index + b] : src1[b];

	  dest[b] = (new_alpha > 127) ? OPAQUE : src1[b];  /*  alpha channel is opaque  */

	  m++;

	  src1 += bytes;
	  src2 += src2_bytes;
	  dest += bytes;
	}
    }
  else
    {
      while (length --)
	{
	  new_alpha = (src2[alpha] * opacity) / 255;

	  index = src2[0] * 3;

	  for (b = 0; b < bytes-1; b++)
	    dest[b] = (new_alpha > 127) ? cmap[index + b] : src1[b];

	  dest[b] = (new_alpha > 127) ? OPAQUE : src1[b];  /*  alpha channel is opaque  */

	  /* m++; /Per */

	  src1 += bytes;
	  src2 += src2_bytes;
	  dest += bytes;
	}
    }
#endif
}


void 
combine_inten_and_inten_row  (
                              PixelRow * src1,
                              PixelRow * src2,
                              PixelRow * dest,
                              PixelRow * mask,
                              Paint * opacity
                              )
{
#if 0
  int b;
  unsigned char new_alpha;
  unsigned char * m;

  if (mask)
    {
      m = mask;
      while (length --)
	{
	  new_alpha = (*m * opacity) / 255;

	  for (b = 0; b < bytes; b++)
	    dest[b] = (affect[b]) ?
	      (src2[b] * new_alpha + src1[b] * (255 - new_alpha)) / 255 :
	    src1[b];

	  m++;

	  src1 += bytes;
	  src2 += bytes;
	  dest += bytes;
	}
    }
  else
    {
      while (length --)
	{
	  new_alpha = opacity;

	  for (b = 0; b < bytes; b++)
	    dest[b] = (affect[b]) ?
	      (src2[b] * new_alpha + src1[b] * (255 - new_alpha)) / 255 :
	    src1[b];

	  src1 += bytes;
	  src2 += bytes;
	  dest += bytes;
	}
    }
#endif
}


void 
combine_inten_and_inten_a_row  (
                                PixelRow * src1,
                                PixelRow * src2,
                                PixelRow * dest,
                                PixelRow * mask,
                                Paint * opacity
                                )
{
#if 0
  int alpha, b;
  int src2_bytes;
  unsigned char new_alpha;
  unsigned char * m;

  alpha = bytes;
  src2_bytes = bytes + 1;

  if (mask)
    {
      m = mask;
      while (length --)
	{
	  new_alpha = (src2[alpha] * *m * opacity) / 65025;

	  for (b = 0; b < bytes; b++)
	    dest[b] = (affect[b]) ?
	      (src2[b] * new_alpha + src1[b] * (255 - new_alpha)) / 255 :
	    src1[b];

	  m++;
	  src1 += bytes;
	  src2 += src2_bytes;
	  dest += bytes;
	}
    }
  else
    {
      while (length --)
	{
	  new_alpha = (src2[alpha] * opacity) / 255;

	  for (b = 0; b < bytes; b++)
	    dest[b] = (affect[b]) ?
	      (src2[b] * new_alpha + src1[b] * (255 - new_alpha)) / 255 :
	    src1[b];

	  src1 += bytes;
	  src2 += src2_bytes;
	  dest += bytes;
	}
    }
#endif
}

#define alphify(src2_alpha,new_alpha) \
	if (new_alpha == 0 || src2_alpha == 0)							\
	  {											\
	    for (b = 0; b < alpha; b++)								\
	      dest[b] = src1 [b];								\
	  }											\
	else if (src2_alpha == new_alpha){							\
	  for (b = 0; b < alpha; b++)								\
	    dest [b] = affect [b] ? src2 [b] : src1 [b];					\
	} else {										\
	  ratio = (float) src2_alpha / new_alpha;						\
	  compl_ratio = 1.0 - ratio;								\
	  											\
	  for (b = 0; b < alpha; b++)								\
	    dest[b] = affect[b] ?								\
	      (unsigned char) (src2[b] * ratio + src1[b] * compl_ratio + EPSILON) : src1[b];	\
	}
	
void 
combine_inten_a_and_inten_row  (
                                PixelRow * src1,
                                PixelRow * src2,
                                PixelRow * dest,
                                PixelRow * mask,
                                Paint * opacity,
                                int mode_affect
                                )
{
#if 0
  int alpha, b;
  int src2_bytes;
  unsigned char src2_alpha;
  unsigned char new_alpha;
  unsigned char * m;
  float ratio, compl_ratio;

  src2_bytes = bytes - 1;
  alpha = bytes - 1;

  if (mask)
    {
      m = mask;
      while (length --)
	{
	  src2_alpha = (*m * opacity) / 255;
	  new_alpha = src1[alpha] + ((255 - src1[alpha]) * src2_alpha) / 255;
	  alphify (src2_alpha, new_alpha);
	  
	  if (mode_affect)
	    dest[alpha] = (affect[alpha]) ? new_alpha : src1[alpha];
	  else
	    dest[alpha] = (src1[alpha]) ? src1[alpha] : (affect[alpha] ? new_alpha : src1[alpha]);

	  m++;

	  src1 += bytes;
	  src2 += src2_bytes;
	  dest += bytes;
	}
    }
  else
    {
      while (length --)
	{
	  src2_alpha = opacity;
	  new_alpha = src1[alpha] + ((255 - src1[alpha]) * src2_alpha) / 255;
	  alphify (src2_alpha, new_alpha);
	  
	  if (mode_affect)
	    dest[alpha] = (affect[alpha]) ? new_alpha : src1[alpha];
	  else
	    dest[alpha] = (src1[alpha]) ? src1[alpha] : (affect[alpha] ? new_alpha : src1[alpha]);

	  src1 += bytes;
	  src2 += src2_bytes;
	  dest += bytes;
	}
    }
#endif
}


void 
combine_inten_a_and_inten_a_row  (
                                  PixelRow * src1,
                                  PixelRow * src2,
                                  PixelRow * dest,
                                  PixelRow * mask,
                                  Paint * opacity,
                                  int mode_affect
                                  )
{
#if 0
  int alpha, b;
  unsigned char src2_alpha;
  unsigned char new_alpha;
  unsigned char * m;
  float ratio, compl_ratio;

  alpha = bytes - 1;
  if (mask){
    m = mask;
    while (length --)
      {
	src2_alpha = (src2[alpha] * *m * opacity) / 65025;
	new_alpha = src1[alpha] + ((255 - src1[alpha]) * src2_alpha) / 255;

	alphify (src2_alpha, new_alpha);
	
	if (mode_affect)
	  dest[alpha] = (affect[alpha]) ? new_alpha : src1[alpha];
	else
	  dest[alpha] = (src1[alpha]) ? src1[alpha] : (affect[alpha] ? new_alpha : src1[alpha]);
	
	m++;
	
	src1 += bytes;
	src2 += bytes;
	dest += bytes;
      }
  } else {
    while (length --)
      {
	src2_alpha = (src2[alpha] * opacity) / 255;
	new_alpha = src1[alpha] + ((255 - src1[alpha]) * src2_alpha) / 255;

	alphify (src2_alpha, new_alpha);
	
	if (mode_affect)
	  dest[alpha] = (affect[alpha]) ? new_alpha : src1[alpha];
	else
	  dest[alpha] = (src1[alpha]) ? src1[alpha] : (affect[alpha] ? new_alpha : src1[alpha]);

	src1 += bytes;
	src2 += bytes;
	dest += bytes;
      }
  }
#endif
}
#undef alphify

void 
combine_inten_a_and_channel_mask_row  (
                                       PixelRow * src,
                                       PixelRow * channel,
                                       PixelRow * dest,
                                       Paint * col,
                                       Paint * opacity
                                       )
{
#if 0
  int alpha, b;
  unsigned char channel_alpha;
  unsigned char new_alpha;
  unsigned char compl_alpha;
  int t, s;

  alpha = bytes - 1;
  while (length --)
    {
      channel_alpha = INT_MULT (255 - *channel, opacity, t);
      if (channel_alpha)
	{
	  new_alpha = src[alpha] + INT_MULT ((255 - src[alpha]), channel_alpha, t);

	  if (new_alpha != 255)
	    channel_alpha = (channel_alpha * 255) / new_alpha;
	  compl_alpha = 255 - channel_alpha;

	  for (b = 0; b < alpha; b++)
	    dest[b] = INT_MULT (col[b], channel_alpha, t) +
	      INT_MULT (src[b], compl_alpha, s);
	  dest[b] = new_alpha;
	}
      else
	for (b = 0; b < bytes; b++)
	  dest[b] = src[b];

      /*  advance pointers  */
      src+=bytes;
      dest+=bytes;
      channel++;
    }
#endif
}


void 
combine_inten_a_and_channel_selection_row  (
                                            PixelRow * src,
                                            PixelRow * channel,
                                            PixelRow * dest,
                                            Paint * col,
                                            Paint * opacity
                                            )
{
#if 0
  int alpha, b;
  unsigned char channel_alpha;
  unsigned char new_alpha;
  unsigned char compl_alpha;
  int t, s;

  alpha = bytes - 1;
  while (length --)
    {
      channel_alpha = INT_MULT (*channel, opacity, t);
      if (channel_alpha)
	{
	  new_alpha = src[alpha] + INT_MULT ((255 - src[alpha]), channel_alpha, t);

	  if (new_alpha != 255)
	    channel_alpha = (channel_alpha * 255) / new_alpha;
	  compl_alpha = 255 - channel_alpha;

	  for (b = 0; b < alpha; b++)
	    dest[b] = INT_MULT (col[b], channel_alpha, t) +
	      INT_MULT (src[b], compl_alpha, s);
	  dest[b] = new_alpha;
	}
      else
	for (b = 0; b < bytes; b++)
	  dest[b] = src[b];

      /*  advance pointers  */
      src+=bytes;
      dest+=bytes;
      channel++;
    }
#endif
}


void 
behind_inten_row  (
                   PixelRow * src1,
                   PixelRow * src2,
                   PixelRow * dest,
                   PixelRow * mask,
                   Paint * opacity
                   )
{
#if 0
  int alpha, b;
  unsigned char src1_alpha;
  unsigned char src2_alpha;
  unsigned char new_alpha;
  unsigned char * m;
  float ratio, compl_ratio;

  if (mask)
    m = mask;
  else
    m = &no_mask;

  /*  the alpha channel  */
  alpha = b1 - 1;

  while (length --)
    {
      src1_alpha = src1[alpha];
      src2_alpha = (src2[alpha] * *m * opacity) / 65025;
      new_alpha = src2_alpha + ((255 - src2_alpha) * src1_alpha) / 255;
      if (new_alpha)
	ratio = (float) src1_alpha / new_alpha;
      else
	ratio = 0.0;
      compl_ratio = 1.0 - ratio;

      for (b = 0; b < alpha; b++)
	dest[b] = (affect[b]) ?
	  (unsigned char) (src1[b] * ratio + src2[b] * compl_ratio + EPSILON) :
	src1[b];

      dest[alpha] = (affect[alpha]) ? new_alpha : src1[alpha];

      if (mask)
	m++;

      src1 += b1;
      src2 += b2;
      dest += b1;
    }
#endif
}


void 
behind_indexed_row  (
                     PixelRow * src1,
                     PixelRow * src2,
                     PixelRow * dest,
                     PixelRow * mask,
                     Paint * opacity
                     )
{
#if 0
  int alpha, b;
  unsigned char src1_alpha;
  unsigned char src2_alpha;
  unsigned char new_alpha;
  unsigned char * m;

  if (mask)
    m = mask;
  else
    m = &no_mask;

  /*  the alpha channel  */
  alpha = b1 - 1;

  while (length --)
    {
      src1_alpha = src1[alpha];
      src2_alpha = (src2[alpha] * *m * opacity) / 65025;
      new_alpha = (src2_alpha > 127) ? OPAQUE : TRANSPARENT;

      for (b = 0; b < b1; b++)
	dest[b] = (affect[b] && new_alpha == OPAQUE && (src1_alpha > 127)) ?
	  src2[b] : src1[b];

      if (mask)
	m++;

      src1 += b1;
      src2 += b2;
      dest += b1;
    }
#endif
}


void 
replace_inten_row  (
                    PixelRow * src1,
                    PixelRow * src2,
                    PixelRow * dest,
                    PixelRow * mask,
                    Paint * opacity
                    )
{
#if 0
  int bytes, b;
  unsigned char mask_alpha;
  unsigned char * m;

  if (mask)
    m = mask;
  else
    m = &no_mask;

  bytes = MIN (b1, b2);
  while (length --)
    {
      mask_alpha = (*m * opacity) / 255;

      for (b = 0; b < bytes; b++)
	dest[b] = (affect[b]) ?
	  (src2[b] * mask_alpha + src1[b] * (255 - mask_alpha)) / 255 :
	src1[b];

      if (ha1 && !ha2)
	dest[b] = src1[b];

      if (mask)
	m++;

      src1 += b1;
      src2 += b2;
      dest += b1;
    }
#endif
}


void 
replace_indexed_row  (
                      PixelRow * src1,
                      PixelRow * src2,
                      PixelRow * dest,
                      PixelRow * mask,
                      Paint * opacity
                      )
{
#if 0
  int bytes, b;
  unsigned char mask_alpha;
  unsigned char * m;

  if (mask)
    m = mask;
  else
    m = &no_mask;

  bytes = MIN (b1, b2);
  while (length --)
    {
      mask_alpha = (*m * opacity) / 255;

      for (b = 0; b < bytes; b++)
	dest[b] = (affect[b] && mask_alpha) ?
	  src2[b] : src1[b];

      if (ha1 && !ha2)
	dest[b] = src1[b];

      if (mask)
	m++;

      src1 += b1;
      src2 += b2;
      dest += b1;
    }
#endif
}


void 
erase_inten_row  (
                  PixelRow * src1,
                  PixelRow * src2,
                  PixelRow * dest,
                  PixelRow * mask,
                  Paint * opacity
                  )
{
#if 0
  int alpha, b;
  unsigned char src2_alpha;
  unsigned char * m;

  if (mask)
    m = mask;
  else
    m = &no_mask;

  alpha = bytes - 1;
  while (length --)
    {
      for (b = 0; b < alpha; b++)
	dest[b] = src1[b];

      src2_alpha = (src2[alpha] * *m * opacity) / 65025;
      dest[alpha] = src1[alpha] - (src1[alpha] * src2_alpha) / 255;

      if (mask)
	m++;

      src1 += bytes;
      src2 += bytes;
      dest += bytes;
    }
#endif
}


void 
erase_indexed_row  (
                    PixelRow * src1,
                    PixelRow * src2,
                    PixelRow * dest,
                    PixelRow * mask,
                    Paint * opacity
                    )
{
#if 0
  int alpha, b;
  unsigned char src2_alpha;
  unsigned char * m;

  if (mask)
    m = mask;
  else
    m = &no_mask;

  alpha = bytes - 1;
  while (length --)
    {
      for (b = 0; b < alpha; b++)
	dest[b] = src1[b];

      src2_alpha = (src2[alpha] * *m * opacity) / 65025;
      dest[alpha] = (src2_alpha > 127) ? TRANSPARENT : src1[alpha];

      if (mask)
	m++;

      src1 += bytes;
      src2 += bytes;
      dest += bytes;
    }
#endif
}


void 
extract_from_inten_row  (
                         PixelRow * src,
                         PixelRow * dest,
                         PixelRow * mask,
                         Paint * bg,
                         int cut
                         )
{
#if 0
  int b, alpha;
  int dest_bytes;
  unsigned char * m;

  if (mask)
    m = mask;
  else
    m = &no_mask;

  alpha = (has_alpha) ? bytes - 1 : bytes;
  dest_bytes = (has_alpha) ? bytes : bytes + 1;
  while (length --)
    {
      for (b = 0; b < alpha; b++)
	dest[b] = src[b];

      if (has_alpha)
	{
	  dest[alpha] = (*m * src[alpha]) / 255;
	  if (cut)
	    src[alpha] = ((255 - *m) * src[alpha]) / 255;
	}
      else
	{
	  dest[alpha] = *m;
	  if (cut)
	    for (b = 0; b < bytes; b++)
	      src[b] = (*m * bg[b] + (255 - *m) * src[b]) / 255;
	}

      if (mask)
	m++;

      src += bytes;
      dest += dest_bytes;
    }
#endif
}


void 
extract_from_indexed_row  (
                           PixelRow * src,
                           PixelRow * dest,
                           PixelRow * mask,
                           Paint * bg,
                           int cut
                           )
{
#if 0
  int b;
  int index;
  unsigned char * m;
  int t;

  if (mask)
    m = mask;
  else
    m = &no_mask;

  while (length --)
    {
      index = src[0] * 3;
      for (b = 0; b < 3; b++)
	dest[b] = cmap[index + b];

      if (has_alpha)
	{
	  dest[3] = INT_MULT (*m, src[1], t);
	  if (cut)
	    src[1] = INT_MULT ((255 - *m), src[1], t);
	}
      else
	{
	  dest[3] = *m;
	  if (cut)
	    src[0] = (*m > 127) ? bg[0] : src[0];
	}

      if (mask)
	m++;

      src += bytes;
      dest += 4;
    }
#endif
}


