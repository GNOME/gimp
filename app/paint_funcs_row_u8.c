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
#include "paint_funcs_area.h"
#include "paint_funcs_row_u8.h"
#include "pixelrow.h"
#include "tag.h"

#define OPAQUE_8BIT       255
#define HALF_OPAQUE_8BIT  127
#define TRANSPARENT_8BIT  0
#define EPSILON           0.0001

#define INT_MULT(a,b,t)  ((t) = (a) * (b) + 0x80, ((((t) >> 8) + (t)) >> 8))

static unsigned char no_mask = OPAQUE_8BIT;


void 
color_row_u8  (
               PixelRow * dest_row,
               PixelRow * col
               )
{
  gint    b;
  guint8 *dest         = (guint8*) pixelrow_data (dest_row);
  guint8 *color        = (guint8*) pixelrow_data (col);
  gint    num_channels = tag_num_channels (pixelrow_tag (dest_row));
  gint    width        = pixelrow_width (dest_row);  

  while (width--)
    {
      for (b = 0; b < num_channels; b++)
	dest[b] = color[b];

      dest += num_channels;
    }
}


void 
blend_row_u8  (
               PixelRow * src1_row,
               PixelRow * src2_row,
               PixelRow * dest_row,
               gfloat blend
               )
{
  gint    alpha; 
  gint    b; 
  Tag     src1_tag     = pixelrow_tag (src1_row); 
  guint8 *dest         = (guint8*)pixelrow_data (dest_row);
  guint8 *src1         = (guint8*)pixelrow_data (src1_row);
  guint8 *src2         = (guint8*)pixelrow_data (src2_row);
  gint    width        = pixelrow_width (dest_row);
  gint    has_alpha    = (tag_alpha (src1_tag)==ALPHA_YES)? TRUE: FALSE;
  gint    num_channels = tag_num_channels (src1_tag);
  gfloat  blend_comp   = 1.0 - blend;

  alpha = has_alpha ? num_channels - 1 : num_channels;
  while (width --)
    {
      for (b = 0; b < alpha; b++)
	dest[b] = src1[b] * blend_comp + src2[b] * blend;

      if (has_alpha)
	dest[alpha] = src1[alpha]; /*alpha channel--assume src2 has none*/

      src1 += num_channels;
      src2 += num_channels;
      dest += num_channels;
    }
}


void 
shade_row_u8  (
               PixelRow * src_row,
               PixelRow * dest_row,
               PixelRow * color,
               gfloat blend
               )
{
  gint    alpha, b;
  Tag     src_tag      = pixelrow_tag (src_row); 
  guint8 *dest         = (guint8*)pixelrow_data (dest_row);
  guint8 *src          = (guint8*)pixelrow_data (src_row);
  gint    width        = pixelrow_width (dest_row);
  gint    num_channels = tag_num_channels (src_tag);
  gint    has_alpha    = (tag_alpha (src_tag)==ALPHA_YES)? TRUE: FALSE;
  guint8 *col          = (guint8*) pixelrow_data (color);
  gfloat  blend_comp   = 1.0 - blend;

  alpha = (has_alpha) ? num_channels - 1 : num_channels;
  while (width --)
    {
      for (b = 0; b < alpha; b++)
	dest[b] = src[b] * blend_comp + col[b] * blend;

      if (has_alpha)
	dest[alpha] = src[alpha];  /* alpha channel */

      src += num_channels;
      dest += num_channels;
    }
}


void
extract_alpha_row_u8 (
		      PixelRow *src_row,
		      PixelRow *mask_row,
		      PixelRow *dest_row
		      )
{
  gint    alpha;
  guint8 *m;
  Tag     src_tag      = pixelrow_tag (src_row); 
  guint8 *dest         = (guint8*)pixelrow_data (dest_row);
  guint8 *src          = (guint8*)pixelrow_data (src_row);
  guint8 *mask         = (guint8*)pixelrow_data (mask_row);
  gint    w            = pixelrow_width (dest_row);
  gint    num_channels = tag_num_channels (src_tag);

  if (mask)
    m = mask;
  else
    m = &no_mask;

  alpha = num_channels - 1;
  while (w --)
    {
      *dest++ = (src[alpha] * *m) / 255;

      if (mask)
	m++;
      src += num_channels;
    }
}


void
darken_row_u8 (
	 	PixelRow *src1_row,
		PixelRow *src2_row,
		PixelRow *dest_row
	       )
{
  gint    b, alpha;
  guint8  s1, s2;
  Tag     src1_tag      = pixelrow_tag (src1_row); 
  Tag     src2_tag      = pixelrow_tag (src2_row); 
  gint    ha1           = (tag_alpha (src1_tag)==ALPHA_YES)? TRUE: FALSE;
  gint    ha2           = (tag_alpha (src2_tag)==ALPHA_YES)? TRUE: FALSE;
  guint8 *dest          = (guint8*)pixelrow_data (dest_row);
  guint8 *src1          = (guint8*)pixelrow_data (src1_row);
  guint8 *src2          = (guint8*)pixelrow_data (src2_row);
  gint    width         = pixelrow_width (dest_row);
  gint    num_channels1 = tag_num_channels (src1_tag);
  gint    num_channels2 = tag_num_channels (src2_tag);
  
  alpha = (ha1 || ha2) ? MAXIMUM (num_channels1, num_channels2) - 1 : num_channels1;

  while (width--)
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

      src1 += num_channels1;
      src2 += num_channels2;
      dest += num_channels2;
    }
}


void
lighten_row_u8 (
		   PixelRow *src1_row,
		   PixelRow *src2_row,
                   PixelRow *dest_row
		   )
{
  gint    b, alpha;
  guint8  s1, s2;
  Tag     src1_tag      = pixelrow_tag (src1_row); 
  Tag     src2_tag      = pixelrow_tag (src2_row); 
  gint    ha1           = (tag_alpha (src1_tag)==ALPHA_YES)? TRUE: FALSE;
  gint    ha2           = (tag_alpha (src2_tag)==ALPHA_YES)? TRUE: FALSE;
  guint8 *dest          = (guint8*)pixelrow_data (dest_row);
  guint8 *src1          = (guint8*)pixelrow_data (src1_row);
  guint8 *src2          = (guint8*)pixelrow_data (src2_row);
  gint    width         = pixelrow_width (dest_row);
  gint    num_channels1 = tag_num_channels (src1_tag);
  gint    num_channels2 = tag_num_channels (src2_tag);

  alpha = (ha1 || ha2) ? 
	MAXIMUM (num_channels1, num_channels2) - 1 : num_channels1;

  while (width--)
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

      src1 += num_channels1;
      src2 += num_channels2;
      dest += num_channels2;
    }
}


void
hsv_only_row_u8 (
		    PixelRow *src1_row,
		    PixelRow *src2_row,
		    PixelRow *dest_row,
		    gint       mode
		    )
{
  gint    r1, g1, b1;
  gint    r2, g2, b2;
  Tag     src1_tag      = pixelrow_tag (src1_row); 
  Tag     src2_tag      = pixelrow_tag (src2_row); 
  gint    ha1           = (tag_alpha (src1_tag)==ALPHA_YES)? TRUE: FALSE;
  gint    ha2           = (tag_alpha (src2_tag)==ALPHA_YES)? TRUE: FALSE;
  guint8 *dest          = (guint8*)pixelrow_data (dest_row);
  guint8 *src1          = (guint8*)pixelrow_data (src1_row);
  guint8 *src2          = (guint8*)pixelrow_data (src2_row);
  gint    width         = pixelrow_width (dest_row);
  gint    num_channels1 = tag_num_channels (src1_tag);
  gint    num_channels2 = tag_num_channels (src2_tag);

  
  /*  assumes inputs are only 4 byte RGBA pixels  */
  while (width--)
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

      src1 += num_channels1;
      src2 += num_channels2;
      dest += num_channels2;
    }
}


void
color_only_row_u8 (
		      PixelRow *src1_row,
		      PixelRow *src2_row,
		      PixelRow *dest_row,
		      gint       mode
		     )
{
  gint    r1, g1, b1;
  gint    r2, g2, b2;
  Tag     src1_tag      = pixelrow_tag (src1_row); 
  Tag     src2_tag      = pixelrow_tag (src2_row); 
  gint    ha1           = (tag_alpha (src1_tag)==ALPHA_YES)? TRUE: FALSE;
  gint    ha2           = (tag_alpha (src2_tag)==ALPHA_YES)? TRUE: FALSE;
  guint8 *dest          = (guint8*)pixelrow_data (dest_row);
  guint8 *src1          = (guint8*)pixelrow_data (src1_row);
  guint8 *src2          = (guint8*)pixelrow_data (src2_row);
  gint    width         = pixelrow_width (dest_row);
  gint    num_channels1 = tag_num_channels (src1_tag);
  gint    num_channels2 = tag_num_channels (src2_tag);

  /*  assumes inputs are only 4 byte RGBA pixels  */
  while (width--)
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

      src1 += num_channels1;
      src2 += num_channels2;
      dest += num_channels2;
    }
}


void
multiply_row_u8 (
		 PixelRow *src1_row,
		 PixelRow *src2_row,
		 PixelRow *dest_row
		 )
{
  gint    alpha, b;
  Tag     src1_tag      = pixelrow_tag (src1_row); 
  Tag     src2_tag      = pixelrow_tag (src2_row); 
  gint    ha1           = (tag_alpha (src1_tag)==ALPHA_YES)? TRUE: FALSE;
  gint    ha2           = (tag_alpha (src2_tag)==ALPHA_YES)? TRUE: FALSE;
  guint8 *dest          = (guint8*)pixelrow_data (dest_row);
  guint8 *src1          = (guint8*)pixelrow_data (src1_row);
  guint8 *src2          = (guint8*)pixelrow_data (src2_row);
  gint    width         = pixelrow_width (dest_row);
  gint    num_channels1 = tag_num_channels (src1_tag);
  gint    num_channels2 = tag_num_channels (src2_tag);

  alpha = (ha1  || ha2 ) ? 
	MAXIMUM (num_channels1, num_channels2) - 1 : num_channels1;

  while (width --)
    {
      for (b = 0; b < alpha; b++)
	dest[b] = (src1[b] * src2[b]) / 255;

      if (ha1 && ha2)
	dest[alpha] = MIN (src1[alpha], src2[alpha]);
      else if (ha2)
	dest[alpha] = src2[alpha];

      src1 += num_channels1;
      src2 += num_channels2;
      dest += num_channels2;
    }
}


void
screen_row_u8 (
		  PixelRow *src1_row,
		  PixelRow *src2_row,
		  PixelRow *dest_row
	          )
{
  gint    alpha, b;
  Tag     src1_tag      = pixelrow_tag (src1_row); 
  Tag     src2_tag      = pixelrow_tag (src2_row); 
  gint    ha1           = (tag_alpha (src1_tag)==ALPHA_YES)? TRUE: FALSE;
  gint    ha2           = (tag_alpha (src2_tag)==ALPHA_YES)? TRUE: FALSE;
  guint8 *dest          = (guint8*)pixelrow_data (dest_row);
  guint8 *src1          = (guint8*)pixelrow_data (src1_row);
  guint8 *src2          = (guint8*)pixelrow_data (src2_row);
  gint    width         = pixelrow_width (dest_row);
  gint    num_channels1 = tag_num_channels (src1_tag);
  gint    num_channels2 = tag_num_channels (src2_tag);
  
  alpha = (ha1 || ha2) ?
	MAXIMUM (num_channels1, num_channels2) - 1 : num_channels1;

  while (width --)
    {
      for (b = 0; b < alpha; b++)
	dest[b] = 255 - ((255 - src1[b]) * (255 - src2[b])) / 255;

      if (ha1 && ha2)
	dest[alpha] = MIN (src1[alpha], src2[alpha]);
      else if (ha2)
	dest[alpha] = src2[alpha];

      src1 += num_channels1;
      src2 += num_channels2;
      dest += num_channels2;
    }
}


void
overlay_row_u8 (
		   PixelRow *src1_row,
		   PixelRow *src2_row,
		   PixelRow *dest_row
		   )
{
  gint    alpha, b;
  gint    screen, mult;
  Tag     src1_tag      = pixelrow_tag (src1_row); 
  Tag     src2_tag      = pixelrow_tag (src2_row); 
  gint    ha1           = (tag_alpha (src1_tag)==ALPHA_YES)? TRUE: FALSE;
  gint    ha2           = (tag_alpha (src2_tag)==ALPHA_YES)? TRUE: FALSE;
  guint8 *dest          = (guint8*)pixelrow_data (dest_row);
  guint8 *src1          = (guint8*)pixelrow_data (src1_row);
  guint8 *src2          = (guint8*)pixelrow_data (src2_row);
  gint    width         = pixelrow_width (dest_row);
  gint    num_channels1 = tag_num_channels (src1_tag);
  gint    num_channels2 = tag_num_channels (src2_tag);

  alpha = (ha1 || ha2) ? MAXIMUM (num_channels1, num_channels2) - 1 : num_channels1;

  while (width --)
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

      src1 += num_channels1;
      src2 += num_channels2;
      dest += num_channels2;
    }
}


void
add_row_u8 ( 
	       PixelRow *src1_row,
	       PixelRow *src2_row,
	       PixelRow *dest_row
	      )
{
  gint    alpha, b;
  gint    sum;
  Tag     src1_tag      = pixelrow_tag (src1_row); 
  Tag     src2_tag      = pixelrow_tag (src2_row); 
  gint    ha1           = (tag_alpha (src1_tag)==ALPHA_YES)? TRUE: FALSE;
  gint    ha2           = (tag_alpha (src2_tag)==ALPHA_YES)? TRUE: FALSE;
  guint8 *dest          = (guint8*)pixelrow_data (dest_row);
  guint8 *src1          = (guint8*)pixelrow_data (src1_row);
  guint8 *src2          = (guint8*)pixelrow_data (src2_row);
  gint    width         = pixelrow_width (dest_row);
  gint    num_channels1 = tag_num_channels (src1_tag);
  gint    num_channels2 = tag_num_channels (src2_tag);
  
  alpha = (ha1 || ha2) ? MAXIMUM (num_channels1, num_channels2) - 1 : num_channels1;

  while (width --)
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

      src1 += num_channels1;
      src2 += num_channels2;
      dest += num_channels2;
    }
}


void
subtract_row_u8 (
		    PixelRow *src1_row,
		    PixelRow *src2_row,
		    PixelRow *dest_row
		    )
{
  gint    alpha, b;
  gint    diff;
  Tag     src1_tag      = pixelrow_tag (src1_row); 
  Tag     src2_tag      = pixelrow_tag (src2_row); 
  gint    ha1           = (tag_alpha (src1_tag)==ALPHA_YES)? TRUE: FALSE;
  gint    ha2           = (tag_alpha (src2_tag)==ALPHA_YES)? TRUE: FALSE;
  guint8 *dest          = (guint8*)pixelrow_data (dest_row);
  guint8 *src1          = (guint8*)pixelrow_data (src1_row);
  guint8 *src2          = (guint8*)pixelrow_data (src2_row);
  gint    width         = pixelrow_width (dest_row);
  gint    num_channels1 = tag_num_channels (src1_tag);
  gint    num_channels2 = tag_num_channels (src2_tag);

  alpha = (ha1 || ha2) ? MAXIMUM (num_channels1, num_channels2) - 1 : num_channels1;

  while (width --)
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

      src1 += num_channels1;
      src2 += num_channels2;
      dest += num_channels2;
    }
}


void
difference_row_u8 (
		      PixelRow *src1_row,
		      PixelRow *src2_row,
		      PixelRow *dest_row
		      )
{
  gint    alpha, b;
  gint    diff;
  Tag     src1_tag      = pixelrow_tag (src1_row); 
  Tag     src2_tag      = pixelrow_tag (src2_row); 
  gint    ha1           = (tag_alpha (src1_tag)==ALPHA_YES)? TRUE: FALSE;
  gint    ha2           = (tag_alpha (src2_tag)==ALPHA_YES)? TRUE: FALSE;
  guint8 *dest          = (guint8*)pixelrow_data (dest_row);
  guint8 *src1          = (guint8*)pixelrow_data (src1_row);
  guint8 *src2          = (guint8*)pixelrow_data (src2_row);
  gint    width         = pixelrow_width (dest_row);
  gint    num_channels1 = tag_num_channels (src1_tag);
  gint    num_channels2 = tag_num_channels (src2_tag);

  alpha = (ha1 || ha2) ? MAXIMUM (num_channels1, num_channels2) - 1 : num_channels1;

  while (width --)
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

      src1 += num_channels1;
      src2 += num_channels2;
      dest += num_channels2;
    }
}


void 
dissolve_row_u8  (
                  PixelRow * src_row,
                  PixelRow * dest_row,
                  gint x,
                  gint y,
                  gfloat opacity
                  )
{
  gint    alpha, b;
  gint    rand_val;
  gint    rand_opac        = opacity * 255;
  Tag     src_tag          = pixelrow_tag (src_row); 
  Tag     dest_tag         = pixelrow_tag (dest_row); 
  gint    has_alpha        = (tag_alpha (src_tag)==ALPHA_YES)? TRUE: FALSE;
  guint8 *dest             = (guint8*)pixelrow_data (dest_row);
  guint8 *src              = (guint8*)pixelrow_data (src_row);
  gint    width            = pixelrow_width (dest_row);
  gint    dest_num_channels = tag_num_channels (dest_tag);
  gint    src_num_channels = tag_num_channels (src_tag);


  alpha = dest_num_channels - 1;

  while (width --)
    {
      /*  preserve the intensity values  */
      for (b = 0; b < alpha; b++)
	dest[b] = src[b];

      /*  dissolve if random value is > *opacity  */
      rand_val = (rand() & 0xFF);

      if (has_alpha)
	dest[alpha] = (rand_val > rand_opac) ? 0 : src[alpha];
      else
	dest[alpha] = (rand_val > rand_opac) ? 0 : OPAQUE_8BIT;

      dest += dest_num_channels;
      src += src_num_channels;
    }
}


void 
replace_row_u8  (
                 PixelRow * src1_row,
                 PixelRow * src2_row,
                 PixelRow * dest_row,
                 PixelRow * mask_row,
                 gfloat opacity,
                 gint * affect
                 )
{
  gint    alpha;
  gint    b;
  double  a_val, a_recip, mask_val;
  int     s1_a, s2_a;
  int     new_val;
  Tag     src1_tag      = pixelrow_tag (src1_row); 
  Tag     src2_tag      = pixelrow_tag (src2_row); 
  guint8 *dest          = (guint8*)pixelrow_data (dest_row);
  guint8 *src1          = (guint8*)pixelrow_data (src1_row);
  guint8 *src2          = (guint8*)pixelrow_data (src2_row);
  guint8 *mask          = (guint8*)pixelrow_data (mask_row);
  gint    width         = pixelrow_width (dest_row);
  gint    num_channels1 = tag_num_channels (src1_tag);
  gint    num_channels2 = tag_num_channels (src2_tag);

  if (num_channels1 != num_channels2)
    {
      g_warning ("replace_pixels_u8 only works on commensurate pixel regions");
      return;
    }

  alpha = num_channels1 - 1;

  while (width --)
    {
      mask_val = mask[0] * opacity;
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
      src1 += num_channels1;
      src2 += num_channels2;
      dest += num_channels2;
      mask++;
    }
}


void
swap_row_u8 (
                PixelRow *src_row,
	        PixelRow *dest_row
	        )
{
  guint8 *dest = (guint8*)pixelrow_data (dest_row);
  guint8 *src  = (guint8*)pixelrow_data (src_row);
  gint    width = pixelrow_width (dest_row);
  
  while (width--)
    {
      *src = *src ^ *dest;
      *dest = *dest ^ *src;
      *src = *src ^ *dest;
      src++;
      dest++;
    }
}


void
scale_row_u8 (
		 PixelRow *src_row,
		 PixelRow *dest_row,
		 gfloat      scale
		  )
{
  guint8 *dest  = (guint8*)pixelrow_data (dest_row);
  guint8 *src   = (guint8*)pixelrow_data (src_row);
  gint    width = pixelrow_width (dest_row);
  
  while (width --)
    *dest++ = (guint8) ((*src++ * scale) / 255);
}


void
add_alpha_row_u8 (
		  PixelRow *src_row,
		  PixelRow *dest_row
		  )
{
  gint    alpha, b;
  Tag     src_tag      = pixelrow_tag (src_row); 
  guint8 *dest         = (guint8*)pixelrow_data (dest_row);
  guint8 *src          = (guint8*)pixelrow_data (src_row);
  gint    width        = pixelrow_width (dest_row);
  gint    num_channels = tag_num_channels (src_tag);
  
  alpha = num_channels + 1;
  
  while (width --)
    {
      for (b = 0; b < num_channels; b++)
	dest[b] = src[b];

      dest[b] = OPAQUE_8BIT;

      src += num_channels;
      dest += alpha;
    }
}


void
flatten_row_u8 (
		   PixelRow *src_row,
		   PixelRow *dest_row,
 		   PixelRow *background
		  )
{
  int     alpha, b;
  int     t1, t2;
  Tag     src_tag      = pixelrow_tag (src_row); 
  guint8 *dest         = (guint8*)pixelrow_data (dest_row);
  guint8 *src          = (guint8*)pixelrow_data (src_row);
  gint    width        = pixelrow_width (dest_row);
  gint    num_channels = tag_num_channels (src_tag);
  guint8 *bg           = (guint8*) pixelrow_data (background);

  alpha = num_channels - 1;
  while (width --)
    {
      for (b = 0; b < alpha; b++)
	dest[b] = INT_MULT (src[b], src[alpha], t1) +
			 INT_MULT (bg[b], (255 - src[alpha]), t2);

      src += num_channels;
      dest += alpha;
    }
}


void
multiply_alpha_row_u8( 
		      PixelRow * src_row
		     )
{
  int     b;
  int     alpha_val;
  guint8 *src          =(guint8*)pixelrow_data (src_row);
  gint    num_channels = tag_num_channels (pixelrow_tag (src_row));
  gint    width        = pixelrow_width (src_row);

  while(width --)
    {
      alpha_val = src[num_channels - 1] * (1.0 / 255.0);
      for (b = 0; b < num_channels - 1; b++)
	src[b] = 0.5 + src[b] * alpha_val;
      src += num_channels;
    }
}

void separate_alpha_row_u8( 
			    PixelRow *src_row
                           )
{
  double  alpha_recip;
  gint    new_val;
  int     b, alpha;
  guint8 *src          =(guint8*)pixelrow_data (src_row);
  gint    num_channels = tag_num_channels (pixelrow_tag (src_row));
  gint    width        = pixelrow_width (src_row);
  
  alpha = num_channels-1;
   
  while (width --) 
    {
      if (src[alpha] != 0 && src[alpha] != 255)
	{
	  alpha_recip = 255.0 / src[alpha];
	  for (b = 0; b < alpha; b++)
	    {
	      new_val = 0.5 + src[b] * alpha_recip;
	      new_val = MIN (new_val, 255);
	      src[b] = new_val;
	    }
	}
      src += num_channels;
    }
} 

void
gray_to_rgb_row_u8 (
		       PixelRow *src_row,
		       PixelRow *dest_row
		       )
{
  gint    b;
  gint    dest_num_channels;
  gint    has_alpha;
  guint8 *src          = (guint8*)pixelrow_data (src_row);
  guint8 *dest         = (guint8*)pixelrow_data (dest_row);
  gint    num_channels = tag_num_channels (pixelrow_tag (src_row));
  gint    width        = pixelrow_width (src_row);
  
  has_alpha = (num_channels == 2) ? 1 : 0;
  dest_num_channels = (has_alpha) ? 4 : 3;

  while (width --)
    {
      for (b = 0; b < num_channels; b++)
	dest[b] = src[0];

      if (has_alpha)
	dest[3] = src[1];

      src += num_channels;
      dest += dest_num_channels;
    }
}


/*  apply the mask data to the alpha channel of the pixel data  */
void 
apply_mask_to_alpha_channel_row_u8  (
                                     PixelRow * src_row,
                                     PixelRow * mask_row,
                                     gfloat opacity
                                     )
{
  gint    alpha;
  guint8 *src          = (guint8*)pixelrow_data (src_row);
  guint8 *mask         = (guint8*)pixelrow_data (mask_row);
  gint    num_channels = tag_num_channels (pixelrow_tag (src_row));
  gint    width        = pixelrow_width (src_row);


  alpha = num_channels - 1;
  while (width --)
    {
      src[alpha] = (src[alpha] * *mask++ * opacity) / 255;
      src += num_channels;
    }
}


/*  combine the mask data with the alpha channel of the pixel data  */
void 
combine_mask_and_alpha_channel_row_u8  (
                                        PixelRow * src_row,
                                        PixelRow * mask_row,
                                        gfloat opacity
                                        )
{
  guint8  mask_val;
  gint    alpha;
  guint8 *src          = (guint8*)pixelrow_data (src_row);
  guint8 *mask         = (guint8*)pixelrow_data (mask_row);
  gint    num_channels = tag_num_channels (pixelrow_tag (src_row));
  gint    width        = pixelrow_width (src_row);

  alpha = num_channels - 1;
  while (width --)
    {
      mask_val = *mask++ * opacity;
      src[alpha] = src[alpha] + ((255 - src[alpha]) * mask_val) / 255;
      src += num_channels;
    }
}


/*  copy gray pixels to intensity-alpha pixels.  This function
 *  essentially takes a source that is only a grayscale image and
 *  copies it to the destination, expanding to RGB if necessary and
 *  adding an alpha channel.  (OPAQUE)
 */
void
copy_gray_to_inten_a_row_u8 (
				PixelRow *src_row,
				PixelRow *dest_row
			        )
{
  gint    b;
  gint    alpha;
  guint8 *src          = (guint8*)pixelrow_data (src_row);
  guint8 *dest         = (guint8*)pixelrow_data (dest_row);
  gint    num_channels = tag_num_channels (pixelrow_tag (dest_row));
  gint    width        = pixelrow_width (src_row);

  alpha = num_channels - 1;
  while (width --)
    {
      for (b = 0; b < alpha; b++)
	dest[b] = *src;
      dest[b] = OPAQUE_8BIT;

      src ++;
      dest += num_channels;
    }
}


/*  lay down the initial pixels in the case of only one
 *  channel being visible and no layers...In this singular
 *  case, we want to display a grayscale image w/o transparency
 */
void
initial_channel_row_u8 (
			   PixelRow *src_row,
			   PixelRow *dest_row
			  )
{
  gint    alpha, b;
  guint8 *src          = (guint8*)pixelrow_data (src_row);
  guint8 *dest         = (guint8*)pixelrow_data (dest_row);
  gint    num_channels = tag_num_channels (pixelrow_tag (dest_row));
  gint    width        = pixelrow_width (src_row);

  alpha = num_channels - 1;
  while (width --)
    {
      for (b = 0; b < alpha; b++)
	dest[b] = src[0];

      dest[alpha] = OPAQUE_8BIT;

      dest += num_channels;
      src ++;
    }
}


/*  lay down the initial pixels in the case of an indexed image.
 *  This process obviously requires no composition
 */
void
initial_indexed_row_u8 (
			    PixelRow *src_row,
			    PixelRow *dest_row,
			    unsigned char *cmap
			   )
{
  gint    col_index;
  guint8 *src          = (guint8*)pixelrow_data (src_row);
  guint8 *dest         = (guint8*)pixelrow_data (dest_row);
  gint    width        = pixelrow_width (src_row);

  /*  This function assumes always that we're mapping from
   *  an RGB colormap to an RGBA image...
   */
  while (width--)
    {
      col_index = *src++ * 3;
      *dest++ = cmap[col_index++];
      *dest++ = cmap[col_index++];
      *dest++ = cmap[col_index++];
      *dest++ = OPAQUE_8BIT;
    }
}


/*  lay down the initial pixels in the case of an indexed image.
 *  This process obviously requires no composition
 */
void 
initial_indexed_a_row_u8  (
                           PixelRow * src_row,
                           PixelRow * dest_row,
                           PixelRow * mask_row,
                           unsigned char * cmap,
                           gfloat opacity
                           )
{
  gint    col_index;
  guint8  new_alpha;
  guint8 *m;
  guint8 *src          = (guint8*)pixelrow_data (src_row);
  guint8 *dest         = (guint8*)pixelrow_data (dest_row);
  guint8 *mask         = (guint8*)pixelrow_data (mask_row);
  gint    width        = pixelrow_width (src_row);

  if (mask)
    m = mask;
  else
    m = &no_mask;

  while (width --)
    {
      col_index = *src++ * 3;
      new_alpha = (*src++ * *m * opacity) / 255;

      *dest++ = cmap[col_index++];
      *dest++ = cmap[col_index++];
      *dest++ = cmap[col_index++];
      /*  Set the alpha channel  */
      *dest++ = (new_alpha > 127) ? OPAQUE_8BIT : TRANSPARENT_8BIT;

      if (mask)
	m++;
    }
}


/*  lay down the initial pixels for the base layer.
 *  This process obviously requires no composition.
 */
void 
initial_inten_row_u8  (
                       PixelRow * src_row,
                       PixelRow * dest_row,
                       PixelRow * mask_row,
                       gfloat opacity,
                       gint * affect
                       )
{
  int b, dest_num_channels;
  guint8 * m;
  guint8 *src          = (guint8*)pixelrow_data (src_row);
  guint8 *dest         = (guint8*)pixelrow_data (dest_row);
  guint8 *mask         = (guint8*)pixelrow_data (mask_row);
  gint    width        = pixelrow_width (src_row);
  gint    num_channels = tag_num_channels (pixelrow_tag (src_row));

  if (mask)
    m = mask;
  else
    m = &no_mask;

  /*  This function assumes the source has no alpha channel and
   *  the destination has an alpha channel.  So dest_num_channels = num_channels + 1
   */
  dest_num_channels = num_channels + 1;

  if (mask)
    {
      while (width --)
	{
	  for (b = 0; b < num_channels; b++)
	    dest [b] = affect [b] ? src [b] : 0;
	    
	  /*  Set the alpha channel  */
	  dest[b] = affect [b] ? (opacity * *m) : 0;
	    
	  m++;
	  dest += dest_num_channels;
	  src += num_channels;
	}
    }
  else
    {
      while (width --)
	{
	  for (b = 0; b < num_channels; b++)
	    dest [b] = affect [b] ? src [b] : 0;
	    
	  /*  Set the alpha channel  */
	  dest[b] = affect [b] ? opacity*255 : 0;

	  dest += dest_num_channels;
	  src += num_channels;
	}
    }
}


/*  lay down the initial pixels for the base layer.
 *  This process obviously requires no composition.
 */
void 
initial_inten_a_row_u8  (
                         PixelRow * src_row,
                         PixelRow * dest_row,
                         PixelRow * mask_row,
                         gfloat opacity,
                         gint * affect
                         )
{
  gint alpha, b;
  guint8 * m;
  guint8 *src          = (guint8*)pixelrow_data (src_row);
  guint8 *dest         = (guint8*)pixelrow_data (dest_row);
  guint8 *mask         = (guint8*)pixelrow_data (mask_row);
  gint    width        = pixelrow_width (src_row);
  gint    num_channels = tag_num_channels (pixelrow_tag (src_row));

  alpha = num_channels - 1;
  if (mask)
    {
      m = mask;
      while (width --)
	{
	  for (b = 0; b < alpha; b++)
	    dest[b] = src[b] * affect[b];
	  
	  /*  Set the alpha channel  */
	  dest[alpha] = affect [alpha] ? (opacity * src[alpha] * *m) / 255 : 0;
	  
	  m++;
	  
	  dest += num_channels;
	  src += num_channels;
	}
    }
  else
    {
      while (width --)
	{
	  for (b = 0; b < alpha; b++)
	    dest[b] = src[b] * affect[b];
	  
	  /*  Set the alpha channel  */
	  dest[alpha] = affect [alpha] ? (opacity * src[alpha]) : 0;
	  
	  dest += num_channels;
	  src += num_channels;
	}
    }
}


/*  combine indexed images with an optional mask which
 *  is interpreted as binary...destination is indexed...
 */
void 
combine_indexed_and_indexed_row_u8  (
                                     PixelRow * src1_row,
                                     PixelRow * src2_row,
                                     PixelRow * dest_row,
                                     PixelRow * mask_row,
                                     gfloat opacity,
                                     gint * affect
                                     )
{
  gint    b;
  guint8  new_alpha;
  guint8 *m;
  guint8 *src1         = (guint8*)pixelrow_data (src1_row);
  guint8 *src2         = (guint8*)pixelrow_data (src2_row);
  guint8 *dest         = (guint8*)pixelrow_data (dest_row);
  guint8 *mask         = (guint8*)pixelrow_data (mask_row);
  gint    width        = pixelrow_width (src1_row);
  gint    num_channels = tag_num_channels (pixelrow_tag (src1_row));

  if (mask)
    {
      m = mask;
      while (width --)
	{
	  new_alpha = (*m * opacity);
	  
	  for (b = 0; b < num_channels; b++)
	    dest[b] = (affect[b] && new_alpha > 127) ? src2[b] : src1[b];
	  
	  m++;
	  
	  src1 += num_channels;
	  src2 += num_channels;
	  dest += num_channels;
	}
    }
  else
    {
      while (width --)
	{
	  new_alpha = opacity * 255;
	  
	  for (b = 0; b < num_channels; b++)
	    dest[b] = (affect[b] && new_alpha > 127) ? src2[b] : src1[b];
	  
	  src1 += num_channels;
	  src2 += num_channels;
	  dest += num_channels;
	}
    }
}


/*  combine indexed images with indexed-alpha images
 *  result is an indexed image
 */
void 
combine_indexed_and_indexed_a_row_u8  (
                                       PixelRow * src1_row,
                                       PixelRow * src2_row,
                                       PixelRow * dest_row,
                                       PixelRow * mask_row,
                                       gfloat opacity,
                                       gint * affect
                                       )
{
  gint    b, alpha;
  guint8  new_alpha;
  guint8 *m;
  gint    src2_num_channels;
  guint8 *src1         = (guint8*)pixelrow_data (src1_row);
  guint8 *src2         = (guint8*)pixelrow_data (src2_row);
  guint8 *dest         = (guint8*)pixelrow_data (dest_row);
  guint8 *mask         = (guint8*)pixelrow_data (mask_row);
  gint    width        = pixelrow_width (src1_row);
  gint    num_channels = tag_num_channels (pixelrow_tag (src1_row));

  alpha = 1;
  src2_num_channels = 2;

  if (mask)
    {
      m = mask;
      while (width --)
	{
	  new_alpha = (src2[alpha] * *m * opacity) / 255;

	  for (b = 0; b < num_channels; b++)
	    dest[b] = (affect[b] && new_alpha > 127) ? src2[b] : src1[b];

	  m++;

	  src1 += num_channels;
	  src2 += src2_num_channels;
	  dest += num_channels;
	}
    }
  else
    {
      while (width --)
	{
	  new_alpha = (src2[alpha] * opacity);

	  for (b = 0; b < num_channels; b++)
	    dest[b] = (affect[b] && new_alpha > 127) ? src2[b] : src1[b];

	  src1 += num_channels;
	  src2 += src2_num_channels;
	  dest += num_channels;
	}
    }
}


/*  combine indexed-alpha images with indexed-alpha images
 *  result is an indexed-alpha image.  use this for painting
 *  to an indexed floating sel
 */
void 
combine_indexed_a_and_indexed_a_row_u8  (
                                         PixelRow * src1_row,
                                         PixelRow * src2_row,
                                         PixelRow * dest_row,
                                         PixelRow * mask_row,
                                         gfloat opacity,
                                         gint * affect
                                         )
{
  gint    b, alpha;
  guint8  new_alpha;
  guint8 *m;
  guint8 *src1         = (guint8*)pixelrow_data (src1_row);
  guint8 *src2         = (guint8*)pixelrow_data (src2_row);
  guint8 *dest         = (guint8*)pixelrow_data (dest_row);
  guint8 *mask         = (guint8*)pixelrow_data (mask_row);
  gint    width        = pixelrow_width (src1_row);
  gint    num_channels = tag_num_channels (pixelrow_tag (src1_row));

  alpha = 1;

  if (mask)
    {
      m = mask;
      while (width --)
	{
	  new_alpha = (src2[alpha] * *m * opacity) / 255;

	  for (b = 0; b < alpha; b++)
	    dest[b] = (affect[b] && new_alpha > 127) ? src2[b] : src1[b];

	  dest[alpha] = (affect[alpha] && new_alpha > 127) ? OPAQUE_8BIT : src1[alpha];

	  m++;

	  src1 += num_channels;
	  src2 += num_channels;
	  dest += num_channels;
	}
    }
  else
    {
      while (width --)
	{
	  new_alpha = (src2[alpha] * opacity);

	  for (b = 0; b < alpha; b++)
	    dest[b] = (affect[b] && new_alpha > 127) ? src2[b] : src1[b];

	  dest[alpha] = (affect[alpha] && new_alpha > 127) ? OPAQUE_8BIT : src1[alpha];

	  src1 += num_channels;
	  src2 += num_channels;
	  dest += num_channels;
	}
    }
}


/*  combine intensity with indexed, destination is
 *  intensity-alpha...use this for an indexed floating sel
 */
void 
combine_inten_a_and_indexed_a_row_u8  (
                                       PixelRow * src1_row,
                                       PixelRow * src2_row,
                                       PixelRow * dest_row,
                                       PixelRow * mask_row,
                                       unsigned char * cmap,
                                       gfloat opacity
                                       )
{
  gint b, alpha;
  guint8  new_alpha;
  gint    src2_num_channels;
  gint    index;
  guint8 *src1         = (guint8*)pixelrow_data (src1_row);
  guint8 *src2         = (guint8*)pixelrow_data (src2_row);
  guint8 *dest         = (guint8*)pixelrow_data (dest_row);
  guint8 *mask         = (guint8*)pixelrow_data (mask_row);
  gint    width        = pixelrow_width (src1_row);
  gint    num_channels = tag_num_channels (pixelrow_tag (src1_row));

  alpha = 1;
  src2_num_channels = 2;

  if (mask)
    {
      guint8 *m = mask;
      while (width --)
	{
	  new_alpha = (src2[alpha] * *m * opacity) / 255;

	  index = src2[0] * 3;

	  for (b = 0; b < num_channels-1; b++)
	    dest[b] = (new_alpha > 127) ? cmap[index + b] : src1[b];

	  dest[b] = (new_alpha > 127) ? OPAQUE_8BIT : src1[b];  /*  alpha channel is opaque  */

	  m++;

	  src1 += num_channels;
	  src2 += src2_num_channels;
	  dest += num_channels;
	}
    }
  else
    {
      while (width --)
	{
	  new_alpha = (src2[alpha] * opacity);

	  index = src2[0] * 3;

	  for (b = 0; b < num_channels-1; b++)
	    dest[b] = (new_alpha > 127) ? cmap[index + b] : src1[b];

	  dest[b] = (new_alpha > 127) ? OPAQUE_8BIT : src1[b];  /*  alpha channel is opaque  */

	  /* m++; /Per */

	  src1 += num_channels;
	  src2 += src2_num_channels;
	  dest += num_channels;
	}
    }
}


/*  combine RGB image with RGB or GRAY with GRAY
 *  destination is intensity-only...
 */
void 
combine_inten_and_inten_row_u8  (
                                 PixelRow * src1_row,
                                 PixelRow * src2_row,
                                 PixelRow * dest_row,
                                 PixelRow * mask_row,
                                 gfloat opacity,
                                 gint * affect
                                 )
{
  gint    b;
  guint8  new_alpha;
  guint8 *m;
  guint8 *src1         = (guint8*)pixelrow_data (src1_row);
  guint8 *src2         = (guint8*)pixelrow_data (src2_row);
  guint8 *dest         = (guint8*)pixelrow_data (dest_row);
  guint8 *mask         = (guint8*)pixelrow_data (mask_row);
  gint    width        = pixelrow_width (src1_row);
  gint    num_channels = tag_num_channels (pixelrow_tag (src1_row));

  if (mask)
    {
      m = mask;
      while (width --)
	{
	  new_alpha = (*m * opacity);

	  for (b = 0; b < num_channels; b++)
	    dest[b] = (affect[b]) ?
	      (src2[b] * new_alpha + src1[b] * (255 - new_alpha)) / 255 :
	    src1[b];

	  m++;

	  src1 += num_channels;
	  src2 += num_channels;
	  dest += num_channels;
	}
    }
  else
    {
      while (width --)
	{
	  new_alpha = opacity * 255;

	  for (b = 0; b < num_channels; b++)
	    dest[b] = (affect[b]) ?
	      (src2[b] * new_alpha + src1[b] * (255 - new_alpha)) / 255 :
	    src1[b];

	  src1 += num_channels;
	  src2 += num_channels;
	  dest += num_channels;
	}
    }
}


/*  combine an RGBA or GRAYA image with an RGB or GRAY image
 *  destination is intensity-only...
 */
void 
combine_inten_and_inten_a_row_u8  (
                                   PixelRow * src1_row,
                                   PixelRow * src2_row,
                                   PixelRow * dest_row,
                                   PixelRow * mask_row,
                                   gfloat opacity,
                                   gint * affect
                                   )
{
  gint    alpha, b;
  gint    src2_num_channels;
  guint8  new_alpha;
  guint8 *m;
  guint8 *src1         = (guint8*)pixelrow_data (src1_row);
  guint8 *src2         = (guint8*)pixelrow_data (src2_row);
  guint8 *dest         = (guint8*)pixelrow_data (dest_row);
  guint8 *mask         = (guint8*)pixelrow_data (mask_row);
  gint    width        = pixelrow_width (src1_row);
  gint    num_channels = tag_num_channels (pixelrow_tag (src1_row));

  alpha = num_channels;
  src2_num_channels = num_channels + 1;

  if (mask)
    {
      m = mask;
      while (width --)
	{
	  new_alpha = (src2[alpha] * *m * opacity) / 255;

	  for (b = 0; b < num_channels; b++)
	    dest[b] = (affect[b]) ?
	      (src2[b] * new_alpha + src1[b] * (255 - new_alpha)) / 255 :
	    src1[b];

	  m++;
	  src1 += num_channels;
	  src2 += src2_num_channels;
	  dest += num_channels;
	}
    }
  else
    {
      while (width --)
	{
	  new_alpha = (src2[alpha] * opacity);

	  for (b = 0; b < num_channels; b++)
	    dest[b] = (affect[b]) ?
	      (src2[b] * new_alpha + src1[b] * (255 - new_alpha)) / 255 :
	    src1[b];

	  src1 += num_channels;
	  src2 += src2_num_channels;
	  dest += num_channels;
	}
    }
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
	      (guint8) (src2[b] * ratio + src1[b] * compl_ratio + EPSILON) : src1[b];	\
	}


	
/*  combine an RGB or GRAY image with an RGBA or GRAYA image
 *  destination is intensity-alpha...
 */
void 
combine_inten_a_and_inten_row_u8  (
                                   PixelRow * src1_row,
                                   PixelRow * src2_row,
                                   PixelRow * dest_row,
                                   PixelRow * mask_row,
                                   gfloat opacity,
                                   gint * affect,
                                   gint mode_affect /* how does the combination mode affect alpha ? */
                                   )
{
  gint    alpha, b;
  gint    src2_num_channels;
  guint8  src2_alpha;
  guint8  new_alpha;
  guint8 *m;
  gfloat  ratio, compl_ratio;
  guint8 *src1         = (guint8*)pixelrow_data (src1_row);
  guint8 *src2         = (guint8*)pixelrow_data (src2_row);
  guint8 *dest         = (guint8*)pixelrow_data (dest_row);
  guint8 *mask         = (guint8*)pixelrow_data (mask_row);
  gint    width        = pixelrow_width (src1_row);
  gint    num_channels = tag_num_channels (pixelrow_tag (src1_row));

  src2_num_channels = num_channels - 1;
  alpha = num_channels - 1;

  if (mask)
    {
      m = mask;
      while (width --)
	{
	  src2_alpha = (*m * opacity);
	  new_alpha = src1[alpha] + ((255 - src1[alpha]) * src2_alpha) / 255;
	  alphify (src2_alpha, new_alpha);
	  
	  if (mode_affect)
	    dest[alpha] = (affect[alpha]) ? new_alpha : src1[alpha];
	  else
	    dest[alpha] = (src1[alpha]) ? src1[alpha] : (affect[alpha] ? new_alpha : src1[alpha]);

	  m++;

	  src1 += num_channels;
	  src2 += src2_num_channels;
	  dest += num_channels;
	}
    }
  else
    {
      while (width --)
	{
	  src2_alpha = opacity * 255;
	  new_alpha = src1[alpha] + ((255 - src1[alpha]) * src2_alpha) / 255;
	  alphify (src2_alpha, new_alpha);
	  
	  if (mode_affect)
	    dest[alpha] = (affect[alpha]) ? new_alpha : src1[alpha];
	  else
	    dest[alpha] = (src1[alpha]) ? src1[alpha] : (affect[alpha] ? new_alpha : src1[alpha]);

	  src1 += num_channels;
	  src2 += src2_num_channels;
	  dest += num_channels;
	}
    }
}


/*  combine an RGBA or GRAYA image with an RGBA or GRAYA image
 *  destination is of course intensity-alpha...
 */
void 
combine_inten_a_and_inten_a_row_u8  (
                                     PixelRow * src1_row,
                                     PixelRow * src2_row,
                                     PixelRow * dest_row,
                                     PixelRow * mask_row,
                                     gfloat opacity,
                                     gint * affect,
                                     gint mode_affect
                                     )
{
  gint    alpha, b;
  guint8  src2_alpha;
  guint8  new_alpha;
  guint8 *m;
  gfloat  ratio, compl_ratio;
  guint8 *src1         = (guint8*)pixelrow_data (src1_row);
  guint8 *src2         = (guint8*)pixelrow_data (src2_row);
  guint8 *dest         = (guint8*)pixelrow_data (dest_row);
  guint8 *mask         = (guint8*)pixelrow_data (mask_row);
  gint    width        = pixelrow_width (src1_row);
  gint    num_channels = tag_num_channels (pixelrow_tag (src1_row));

  alpha = num_channels - 1;
  if (mask){
    m = mask;
    while (width --)
      {
	src2_alpha = (src2[alpha] * *m * opacity) / 255;
	new_alpha = src1[alpha] + ((255 - src1[alpha]) * src2_alpha) / 255;

	alphify (src2_alpha, new_alpha);
	
	if (mode_affect)
	  dest[alpha] = (affect[alpha]) ? new_alpha : src1[alpha];
	else
	  dest[alpha] = (src1[alpha]) ? src1[alpha] : (affect[alpha] ? new_alpha : src1[alpha]);
	
	m++;
	
	src1 += num_channels;
	src2 += num_channels;
	dest += num_channels;
      }
  } else {
    while (width --)
      {
	src2_alpha = (src2[alpha] * opacity);
	new_alpha = src1[alpha] + ((255 - src1[alpha]) * src2_alpha) / 255;

	alphify (src2_alpha, new_alpha);
	
	if (mode_affect)
	  dest[alpha] = (affect[alpha]) ? new_alpha : src1[alpha];
	else
	  dest[alpha] = (src1[alpha]) ? src1[alpha] : (affect[alpha] ? new_alpha : src1[alpha]);

	src1 += num_channels;
	src2 += num_channels;
	dest += num_channels;
      }
  }
}
#undef alphify


/*  combine a channel with intensity-alpha pixels based
 *  on some opacity, and a channel color...
 *  destination is intensity-alpha
 */
void 
combine_inten_a_and_channel_mask_row_u8  (
                                          PixelRow * src_row,
                                          PixelRow * channel_row,
                                          PixelRow * dest_row,
                                          PixelRow * col,
                                          gfloat opacity
                                          )
{
  gint    alpha, b;
  guint8  channel_alpha;
  guint8  new_alpha;
  guint8  compl_alpha;
  gint    t, s;
  guint8 *src          = (guint8*)pixelrow_data (src_row);
  guint8 *dest         = (guint8*)pixelrow_data (dest_row);
  guint8 *channel      = (guint8*)pixelrow_data (channel_row);
  gint    width        = pixelrow_width (src_row);
  gint    num_channels = tag_num_channels (pixelrow_tag (src_row));
  guint8 *color        = (guint8*) pixelrow_data (col);

  alpha = num_channels - 1;
  while (width --)
    {
      channel_alpha = INT_MULT (255 - *channel, opacity*255, t);
      if (channel_alpha)
	{
	  new_alpha = src[alpha] + INT_MULT ((255 - src[alpha]), channel_alpha, t);

	  if (new_alpha != 255)
	    channel_alpha = (channel_alpha * 255) / new_alpha;
	  compl_alpha = 255 - channel_alpha;

	  for (b = 0; b < alpha; b++)
	    dest[b] = INT_MULT (color[b], channel_alpha, t) +
	      INT_MULT (src[b], compl_alpha, s);
	  dest[b] = new_alpha;
	}
      else
	for (b = 0; b < num_channels; b++)
	  dest[b] = src[b];

      /*  advance pointers  */
      src+=num_channels;
      dest+=num_channels;
      channel++;
    }
}


void 
combine_inten_a_and_channel_selection_row_u8  (
                                               PixelRow * src_row,
                                               PixelRow * channel_row,
                                               PixelRow * dest_row,
                                               PixelRow * col,
                                               gfloat opacity
                                               )
{
  gint    alpha, b;
  guint8  channel_alpha;
  guint8  new_alpha;
  guint8  compl_alpha;
  gint t, s;
  guint8 *src          = (guint8*)pixelrow_data (src_row);
  guint8 *dest         = (guint8*)pixelrow_data (dest_row);
  guint8 *channel      = (guint8*)pixelrow_data (channel_row);
  gint    width        = pixelrow_width (src_row);
  gint    num_channels = tag_num_channels (pixelrow_tag (src_row));
  guint8 *color        = (guint8*) pixelrow_data (col);

  alpha = num_channels - 1;
  while (width --)
    {
      channel_alpha = INT_MULT (*channel, opacity*255, t);
      if (channel_alpha)
	{
	  new_alpha = src[alpha] + INT_MULT ((255 - src[alpha]), channel_alpha, t);

	  if (new_alpha != 255)
	    channel_alpha = (channel_alpha * 255) / new_alpha;
	  compl_alpha = 255 - channel_alpha;

	  for (b = 0; b < alpha; b++)
	    dest[b] = INT_MULT (color[b], channel_alpha, t) +
	      INT_MULT (src[b], compl_alpha, s);
	  dest[b] = new_alpha;
	}
      else
	for (b = 0; b < num_channels; b++)
	  dest[b] = src[b];

      /*  advance pointers  */
      src+=num_channels;
      dest+=num_channels;
      channel++;
    }
}


/*  paint "behind" the existing pixel row.
 *  This is similar in appearance to painting on a layer below
 *  the existing pixels.
 */
void 
behind_inten_row_u8  (
                      PixelRow * src1_row,
                      PixelRow * src2_row,
                      PixelRow * dest_row,
                      PixelRow * mask_row,
                      gfloat opacity,
                      gint * affect
                      )
{
  gint    alpha, b;
  guint8  src1_alpha;
  guint8  src2_alpha;
  guint8  new_alpha;
  guint8 *m;
  gfloat  ratio, compl_ratio;
  guint8 *src1          = (guint8*)pixelrow_data (src1_row);
  guint8 *src2          = (guint8*)pixelrow_data (src2_row);
  guint8 *dest          = (guint8*)pixelrow_data (dest_row);
  guint8 *mask          = (guint8*)pixelrow_data (mask_row);
  gint    width         = pixelrow_width (src1_row);
  gint    num_channels1 = tag_num_channels (pixelrow_tag (src1_row));
  gint    num_channels2 = tag_num_channels (pixelrow_tag (src2_row));

  if (mask)
    m = mask;
  else
    m = &no_mask;

  /*  the alpha channel  */
  alpha = num_channels1 - 1;

  while (width --)
    {
      src1_alpha = src1[alpha];
      src2_alpha = (src2[alpha] * *m * opacity) / 255;
      new_alpha = src2_alpha + ((255 - src2_alpha) * src1_alpha) / 255;
      if (new_alpha)
	ratio = (float) src1_alpha / new_alpha;
      else
	ratio = 0.0;
      compl_ratio = 1.0 - ratio;

      for (b = 0; b < alpha; b++)
	dest[b] = (affect[b]) ?
	  (guint8) (src1[b] * ratio + src2[b] * compl_ratio + EPSILON) :
	src1[b];

      dest[alpha] = (affect[alpha]) ? new_alpha : src1[alpha];

      if (mask)
	m++;

      src1 += num_channels1;
      src2 += num_channels2;
      dest += num_channels1;
    }
}


/*  paint "behind" the existing pixel row (for indexed images).
 *  This is similar in appearance to painting on a layer below
 *  the existing pixels.
 */
void 
behind_indexed_row_u8  (
                        PixelRow * src1_row,
                        PixelRow * src2_row,
                        PixelRow * dest_row,
                        PixelRow * mask_row,
                        gfloat opacity,
                        gint * affect
                        )
{
  int alpha, b;
  guint8 src1_alpha;
  guint8 src2_alpha;
  guint8 new_alpha;
  guint8 * m;
  guint8 *src1          = (guint8*)pixelrow_data (src1_row);
  guint8 *src2          = (guint8*)pixelrow_data (src2_row);
  guint8 *dest          = (guint8*)pixelrow_data (dest_row);
  guint8 *mask          = (guint8*)pixelrow_data (mask_row);
  gint    width         = pixelrow_width (src1_row);
  gint    num_channels1 = tag_num_channels (pixelrow_tag (src1_row));
  gint    num_channels2 = tag_num_channels (pixelrow_tag (src2_row));

  if (mask)
    m = mask;
  else
    m = &no_mask;

  /*  the alpha channel  */
  alpha = num_channels1 - 1;

  while (width --)
    {
      src1_alpha = src1[alpha];
      src2_alpha = (src2[alpha] * *m * opacity) / 255;
      new_alpha = (src2_alpha > 127) ? OPAQUE_8BIT : TRANSPARENT_8BIT;

      for (b = 0; b < num_channels1; b++)
	dest[b] = (affect[b] && new_alpha == OPAQUE_8BIT && (src1_alpha > 127)) ?
	  src2[b] : src1[b];

      if (mask)
	m++;

      src1 += num_channels1;
      src2 += num_channels2;
      dest += num_channels1;
    }
}


/*  replace the contents of one pixel row with the other
 *  The operation is still bounded by mask/opacity constraints
 */
void 
replace_inten_row_u8  (
                       PixelRow * src1_row,
                       PixelRow * src2_row,
                       PixelRow * dest_row,
                       PixelRow * mask_row,
                       gfloat opacity,
                       gint * affect
                       )
{
  gint    num_channels, b;
  guint8  mask_alpha;
  guint8 *m;
  Tag     src1_tag      = pixelrow_tag (src1_row); 
  Tag     src2_tag      = pixelrow_tag (src2_row); 
  guint8 *src1          = (guint8*)pixelrow_data (src1_row);
  guint8 *src2          = (guint8*)pixelrow_data (src2_row);
  guint8 *dest          = (guint8*)pixelrow_data (dest_row);
  guint8 *mask          = (guint8*)pixelrow_data (mask_row);
  gint    width         = pixelrow_width (src1_row);
  gint    num_channels1 = tag_num_channels (src1_tag);
  gint    num_channels2 = tag_num_channels (src2_tag);
  gint    ha1           = (tag_alpha (src1_tag)==ALPHA_YES)? TRUE: FALSE;
  gint    ha2           = (tag_alpha (src2_tag)==ALPHA_YES)? TRUE: FALSE;

  if (mask)
    m = mask;
  else
    m = &no_mask;

  num_channels = MINIMUM (num_channels1, num_channels2);
  while (width --)
    {
      mask_alpha = (*m * opacity);

      for (b = 0; b < num_channels; b++)
	dest[b] = (affect[b]) ?
	  (src2[b] * mask_alpha + src1[b] * (255 - mask_alpha)) / 255 :
	src1[b];

      if (ha1 && !ha2)
	dest[b] = src1[b];

      if (mask)
	m++;

      src1 += num_channels1;
      src2 += num_channels2;
      dest += num_channels1;
    }
}


/*  replace the contents of one pixel row with the other
 *  The operation is still bounded by mask/opacity constraints
 */
void 
replace_indexed_row_u8  (
                         PixelRow * src1_row,
                         PixelRow * src2_row,
                         PixelRow * dest_row,
                         PixelRow * mask_row,
                         gfloat opacity,
                         gint * affect
                         )
{
  gint    num_channels, b;
  guint8  mask_alpha;
  guint8 *m;
  Tag     src1_tag      = pixelrow_tag (src1_row); 
  Tag     src2_tag      = pixelrow_tag (src2_row); 
  guint8 *src1          = (guint8*)pixelrow_data (src1_row);
  guint8 *src2          = (guint8*)pixelrow_data (src2_row);
  guint8 *dest          = (guint8*)pixelrow_data (dest_row);
  guint8 *mask          = (guint8*)pixelrow_data (mask_row);
  gint    width         = pixelrow_width (src1_row);
  gint    num_channels1 = tag_num_channels (src1_tag);
  gint    num_channels2 = tag_num_channels (src2_tag);
  gint    ha1           = (tag_alpha (src1_tag)==ALPHA_YES)? TRUE: FALSE;
  gint    ha2           = (tag_alpha (src2_tag)==ALPHA_YES)? TRUE: FALSE;

  if (mask)
    m = mask;
  else
    m = &no_mask;

  num_channels = MINIMUM (num_channels1, num_channels2);
  while (width --)
    {
      mask_alpha = (*m * opacity);

      for (b = 0; b < num_channels; b++)
	dest[b] = (affect[b] && mask_alpha) ?
	  src2[b] : src1[b];

      if (ha1 && !ha2)
	dest[b] = src1[b];

      if (mask)
	m++;

      src1 += num_channels1;
      src2 += num_channels2;
      dest += num_channels1;
    }
}


/*  apply source 2 to source 1, but in a non-additive way,
 *  multiplying alpha channels  (works for intensity)
 */
void 
erase_inten_row_u8  (
                     PixelRow * src1_row,
                     PixelRow * src2_row,
                     PixelRow * dest_row,
                     PixelRow * mask_row,
                     gfloat opacity,
                     gint * affect
                     )
{
  gint    alpha, b;
  guint8  src2_alpha;
  guint8 *m;
  guint8 *src1          = (guint8*)pixelrow_data (src1_row);
  guint8 *src2          = (guint8*)pixelrow_data (src2_row);
  guint8 *dest          = (guint8*)pixelrow_data (dest_row);
  guint8 *mask          = (guint8*)pixelrow_data (mask_row);
  gint    width         = pixelrow_width (src1_row);
  gint    num_channels  = tag_num_channels (pixelrow_tag (src1_row));

  if (mask)
    m = mask;
  else
    m = &no_mask;

  alpha = num_channels - 1;
  while (width --)
    {
      for (b = 0; b < alpha; b++)
	dest[b] = src1[b];

      src2_alpha = (src2[alpha] * *m * opacity) / 255;
      dest[alpha] = src1[alpha] - (src1[alpha] * src2_alpha) / 255;

      if (mask)
	m++;

      src1 += num_channels;
      src2 += num_channels;
      dest += num_channels;
    }
}


/*  apply source 2 to source 1, but in a non-additive way,
 *  multiplying alpha channels  (works for indexed)
 */
void 
erase_indexed_row_u8  (
                       PixelRow * src1_row,
                       PixelRow * src2_row,
                       PixelRow * dest_row,
                       PixelRow * mask_row,
                       gfloat opacity,
                       gint * affect
                       )
{
  gint    alpha, b;
  guint8  src2_alpha;
  guint8 *m;
  guint8 *src1          = (guint8*)pixelrow_data (src1_row);
  guint8 *src2          = (guint8*)pixelrow_data (src2_row);
  guint8 *dest          = (guint8*)pixelrow_data (dest_row);
  guint8 *mask          = (guint8*)pixelrow_data (mask_row);
  gint    width         = pixelrow_width (src1_row);
  gint    num_channels  = tag_num_channels (pixelrow_tag (src1_row));

  if (mask)
    m = mask;
  else
    m = &no_mask;

  alpha = num_channels - 1;
  while (width --)
    {
      for (b = 0; b < alpha; b++)
	dest[b] = src1[b];

      src2_alpha = (src2[alpha] * *m * opacity) / 255;
      dest[alpha] = (src2_alpha > 127) ? TRANSPARENT_8BIT : src1[alpha];

      if (mask)
	m++;

      src1 += num_channels;
      src2 += num_channels;
      dest += num_channels;
    }
}


/*  extract information from intensity pixels based on
 *  a mask.
 */
void
extract_from_inten_row_u8 (
			      PixelRow *src_row,
			      PixelRow *dest_row,
			      PixelRow *mask_row,
			      PixelRow *background,
			      gint      cut
			      )
{
  gint    b, alpha;
  gint    dest_num_channels;
  guint8 *m;
  Tag     src_tag       = pixelrow_tag (src_row); 
  guint8 *src           = (guint8*)pixelrow_data (src_row);
  guint8 *dest          = (guint8*)pixelrow_data (dest_row);
  guint8 *mask          = (guint8*)pixelrow_data (mask_row);
  gint    width         = pixelrow_width (src_row);
  gint    num_channels  = tag_num_channels (src_tag);
  gint    has_alpha     = (tag_alpha (src_tag)==ALPHA_YES)? TRUE: FALSE;
  guint8 *bg            = (guint8*) pixelrow_data (background);

  if (mask)
    m = mask;
  else
    m = &no_mask;

  alpha = (has_alpha) ? num_channels - 1 : num_channels;
  dest_num_channels = (has_alpha) ? num_channels : num_channels + 1;
  while (width --)
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
	    for (b = 0; b < num_channels; b++)
	      src[b] = (*m * bg[b] + (255 - *m) * src[b]) / 255;
	}

      if (mask)
	m++;

      src += num_channels;
      dest += dest_num_channels;
    }
}


/*  extract information from indexed pixels based on
 *  a mask.
 */
void
extract_from_indexed_row_u8 (
				PixelRow *src_row,
				PixelRow *dest_row,
				PixelRow *mask_row,
				unsigned char *cmap,
				PixelRow *background,
				gint      cut
				)
{
  gint    b;
  gint    index;
  guint8 *m;
  gint    t;
  Tag     src_tag       = pixelrow_tag (src_row); 
  guint8 *src           = (guint8*)pixelrow_data (src_row);
  guint8 *dest          = (guint8*)pixelrow_data (dest_row);
  guint8 *mask          = (guint8*)pixelrow_data (mask_row);
  gint    width         = pixelrow_width (src_row);
  gint    num_channels  = tag_num_channels (src_tag);
  gint    has_alpha     = (tag_alpha (src_tag)==ALPHA_YES)? TRUE: FALSE;
  guint8 *bg            = (guint8*) pixelrow_data (background);

  if (mask)
    m = mask;
  else
    m = &no_mask;

  while (width --)
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

      src += num_channels;
      dest += 4;
    }
}

