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
#include "paint_funcs_row_float.h"
#include "pixelrow.h"
#include "tag.h"

#define OPAQUE_FLOAT       1.0
#define HALF_OPAQUE_FLOAT  .5
#define TRANSPARENT_FLOAT  0.0

void 
absdiff_row_float  (
                    PixelRow * image,
                    PixelRow * mask,
                    PixelRow * col,
                    gfloat threshold,
                    int antialias
                    )
{
  gfloat *src           = (gfloat*) pixelrow_data (image);
  gfloat *dest          = (gfloat*) pixelrow_data (mask);
  gfloat *color         = (gfloat*) pixelrow_data (col);

  Tag     tag           = pixelrow_tag (image);
  int     has_alpha     = (tag_alpha (tag) == ALPHA_YES) ? 1 : 0;

  gint    width         = pixelrow_width (image);  

  gint    src_channels  = tag_num_channels (pixelrow_tag (image));
  gint    dest_channels = tag_num_channels (pixelrow_tag (mask));

  

  while (width--)
    {
      /*  if there is an alpha channel, never select transparent regions  */
      if (has_alpha && src[src_channels] == 0)
        {
          *dest = 0;
        }
      else
        {
          gint b;
          gfloat diff;
          gfloat max = 0;
          
          for (b = 0; b < src_channels; b++)
            {
              diff = src[b] - color[b];
              diff = abs (diff);
              if (diff > max)
                max = diff;
            }
      
          if (antialias && threshold > 0)
            {
              float aa;

              aa = 1.5 - ((float) max / threshold);
              if (aa <= 0)
                *dest = 0;
              else if (aa < 0.5)
                *dest = (aa * 2.0);
              else
                *dest = 1.0;
            }
          else
            {
              if (max > threshold)
                *dest = 0;
              else
                *dest = 1.0;
            }
          
          src += src_channels;
          dest += dest_channels;
        }
    }
}


static gfloat no_mask = OPAQUE_FLOAT;
void 
color_row_float  (
                  PixelRow * dest_row,
                  PixelRow * col
                  )
{
  int b;
  gfloat *dest         = (gfloat*) pixelrow_data (dest_row);
  gfloat *color        = (gfloat*) pixelrow_data (col);
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
blend_row_float  (
                  PixelRow * src1_row,
                  PixelRow * src2_row,
                  PixelRow * dest_row,
                  gfloat blend
                  )
{
  gint alpha, b;
  Tag     src1_tag     = pixelrow_tag (src1_row); 
  gfloat *dest         = (gfloat*)pixelrow_data (dest_row);
  gfloat *src1         = (gfloat*)pixelrow_data (src1_row);
  gfloat *src2         = (gfloat*)pixelrow_data (src2_row);
  gint    width        = pixelrow_width (dest_row);
  gint    has_alpha    = (tag_alpha (src1_tag)==ALPHA_YES)? TRUE: FALSE;
  gint    num_channels = tag_num_channels (src1_tag);
  gfloat  blend_comp   = (1.0 - blend);

  alpha = (has_alpha) ? num_channels - 1 : num_channels;
  while (width --)
    {
      for (b = 0; b < alpha; b++)
	dest[b] = src1[b] * blend_comp + src2[b] * blend;

      if (has_alpha)
	dest[alpha] = src1[alpha];  /*  alpha channel--assume src2 has none  */

      src1 += num_channels;
      src2 += num_channels;
      dest += num_channels;
    }
}


void 
shade_row_float  (
                  PixelRow * src_row,
                  PixelRow * dest_row,
                  PixelRow * color,
                  gfloat blend
                  )
{
  gint alpha, b;
  Tag     src_tag      = pixelrow_tag (src_row); 
  gfloat *dest         = (gfloat*)pixelrow_data (dest_row);
  gfloat *src          = (gfloat*)pixelrow_data (src_row);
  gint    width        = pixelrow_width (dest_row);
  gint    num_channels = tag_num_channels (src_tag);
  gint    has_alpha    = (tag_alpha (src_tag)==ALPHA_YES)? TRUE: FALSE;
  gfloat  blend_comp   = (1.0 - blend);
  gfloat *col          = (gfloat*) pixelrow_data (color);

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
extract_alpha_row_float (
		      PixelRow *src_row,
		      PixelRow *mask_row,
		      PixelRow *dest_row
		      )
{
  gint alpha;
  gfloat * m;
  Tag     src_tag      = pixelrow_tag (src_row); 
  gfloat *dest         = (gfloat*)pixelrow_data (dest_row);
  gfloat *src          = (gfloat*)pixelrow_data (src_row);
  gfloat *mask         = (gfloat*)pixelrow_data (mask_row);
  gint    width            = pixelrow_width (dest_row);
  gint    num_channels = tag_num_channels (src_tag);

  if (mask)
    m = mask;
  else
    m = &no_mask;

  alpha = num_channels - 1;
  while (width --)
    {
      *dest++ = src[alpha] * *m;

      if (mask)
	m++;
      src += num_channels;
    }
}


void
darken_row_float (
	 	PixelRow *src1_row,
		PixelRow *src2_row,
		PixelRow *dest_row
	       )
{
  gint b, alpha;
  gfloat s1, s2;
  Tag     src1_tag      = pixelrow_tag (src1_row); 
  Tag     src2_tag      = pixelrow_tag (src2_row); 
  gint    ha1           = (tag_alpha (src1_tag)==ALPHA_YES)? TRUE: FALSE;
  gint    ha2           = (tag_alpha (src2_tag)==ALPHA_YES)? TRUE: FALSE;
  gfloat *dest          = (gfloat*)pixelrow_data (dest_row);
  gfloat *src1          = (gfloat*)pixelrow_data (src1_row);
  gfloat *src2          = (gfloat*)pixelrow_data (src2_row);
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
lighten_row_float (
		   PixelRow *src1_row,
		   PixelRow *src2_row,
                   PixelRow *dest_row
		   )
{

  gint b, alpha;
  gfloat s1, s2;
  Tag     src1_tag      = pixelrow_tag (src1_row); 
  Tag     src2_tag      = pixelrow_tag (src2_row); 
  gint    ha1           = (tag_alpha (src1_tag)==ALPHA_YES)? TRUE: FALSE;
  gint    ha2           = (tag_alpha (src2_tag)==ALPHA_YES)? TRUE: FALSE;
  gfloat *dest          = (gfloat*)pixelrow_data (dest_row);
  gfloat *src1          = (gfloat*)pixelrow_data (src1_row);
  gfloat *src2          = (gfloat*)pixelrow_data (src2_row);
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
hsv_only_row_float (
		    PixelRow *src1_row,
		    PixelRow *src2_row,
		    PixelRow *dest_row,
		    int       mode
		    )
{
  gfloat r1, g1, b1;
  gfloat r2, g2, b2;
  Tag     src1_tag      = pixelrow_tag (src1_row); 
  Tag     src2_tag      = pixelrow_tag (src2_row); 
  gint    ha1           = (tag_alpha (src1_tag)==ALPHA_YES)? TRUE: FALSE;
  gint    ha2           = (tag_alpha (src2_tag)==ALPHA_YES)? TRUE: FALSE;
  gfloat *dest          = (gfloat*)pixelrow_data (dest_row);
  gfloat *src1          = (gfloat*)pixelrow_data (src1_row);
  gfloat *src2          = (gfloat*)pixelrow_data (src2_row);
  gint    width         = pixelrow_width (dest_row);
  gint    num_channels1 = tag_num_channels (src1_tag);
  gint    num_channels2 = tag_num_channels (src2_tag);

  /*  assumes inputs are only 4 channel RGBA pixels  */
  while (width--)
    {
      r1 = src1[0]; g1 = src1[1]; b1 = src1[2];
      r2 = src2[0]; g2 = src2[1]; b2 = src2[2];
/*    rgb_to_hsv (&r1, &g1, &b1); */
/*    rgb_to_hsv (&r2, &g2, &b2); */

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
/*    hsv_to_rgb (&r1, &g1, &b1); */

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
color_only_row_float (
		      PixelRow *src1_row,
		      PixelRow *src2_row,
		      PixelRow *dest_row,
		      int       mode
		     )
{
  gfloat r1, g1, b1;
  gfloat r2, g2, b2;
  Tag     src1_tag      = pixelrow_tag (src1_row); 
  Tag     src2_tag      = pixelrow_tag (src2_row); 
  gint    ha1           = (tag_alpha (src1_tag)==ALPHA_YES)? TRUE: FALSE;
  gint    ha2           = (tag_alpha (src2_tag)==ALPHA_YES)? TRUE: FALSE;
  gfloat *dest          = (gfloat*)pixelrow_data (dest_row);
  gfloat *src1          = (gfloat*)pixelrow_data (src1_row);
  gfloat *src2          = (gfloat*)pixelrow_data (src2_row);
  gint    width         = pixelrow_width (dest_row);
  gint    num_channels1 = tag_num_channels (src1_tag);
  gint    num_channels2 = tag_num_channels (src2_tag);
  
  /*  assumes inputs are only 4 byte RGBA pixels  */
  while (width--)
    {
      r1 = src1[0]; g1 = src1[1]; b1 = src1[2];
      r2 = src2[0]; g2 = src2[1]; b2 = src2[2];
/*    rgb_to_hls (&r1, &g1, &b1); */
/*    rgb_to_hls (&r2, &g2, &b2); */

      /*  transfer hue and saturation to the source pixel  */
      r1 = r2;
      b1 = b2;

      /*  set the destination  */
/*    hls_to_rgb (&r1, &g1, &b1); */

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
multiply_row_float (
		 PixelRow *src1_row,
		 PixelRow *src2_row,
		 PixelRow *dest_row
		 )
{
  gint alpha, b;
  Tag     src1_tag      = pixelrow_tag (src1_row); 
  Tag     src2_tag      = pixelrow_tag (src2_row); 
  gint    ha1           = (tag_alpha (src1_tag)==ALPHA_YES)? TRUE: FALSE;
  gint    ha2           = (tag_alpha (src2_tag)==ALPHA_YES)? TRUE: FALSE;
  gfloat *dest          = (gfloat*)pixelrow_data (dest_row);
  gfloat *src1          = (gfloat*)pixelrow_data (src1_row);
  gfloat *src2          = (gfloat*)pixelrow_data (src2_row);
  gint    width         = pixelrow_width (dest_row);
  gint    num_channels1 = tag_num_channels (src1_tag);
  gint    num_channels2 = tag_num_channels (src2_tag);

  alpha = (ha1  || ha2 ) ? 
	MAXIMUM (num_channels1, num_channels2) - 1 : num_channels1;

  while (width --)
    {
      for (b = 0; b < alpha; b++)
	dest[b] = src1[b] * src2[b];

      if (ha1 == ALPHA_YES && ha2 == ALPHA_YES)
	dest[alpha] = MIN (src1[alpha], src2[alpha]);
      else if (ha2 == ALPHA_YES)
	dest[alpha] = src2[alpha];

      src1 += num_channels1;
      src2 += num_channels2;
      dest += num_channels2;
    }
}


void
screen_row_float (
		  PixelRow *src1_row,
		  PixelRow *src2_row,
		  PixelRow *dest_row
	          )
{
  gint alpha, b;
  Tag     src1_tag      = pixelrow_tag (src1_row); 
  Tag     src2_tag      = pixelrow_tag (src2_row); 
  gint    ha1           = (tag_alpha (src1_tag)==ALPHA_YES)? TRUE: FALSE;
  gint    ha2           = (tag_alpha (src2_tag)==ALPHA_YES)? TRUE: FALSE;
  gfloat *dest          = (gfloat*)pixelrow_data (dest_row);
  gfloat *src1          = (gfloat*)pixelrow_data (src1_row);
  gfloat *src2          = (gfloat*)pixelrow_data (src2_row);
  gint    width         = pixelrow_width (dest_row);
  gint    num_channels1 = tag_num_channels (src1_tag);
  gint    num_channels2 = tag_num_channels (src2_tag);

  alpha = (ha1 || ha2) ? MAXIMUM (num_channels1, num_channels2) - 1 : num_channels1;

  while (width --)
    {
      for (b = 0; b < alpha; b++)
	dest[b] = 1.0 - ((1.0 - src1[b]) * (1.0 - src2[b]));

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
overlay_row_float (
		   PixelRow *src1_row,
		   PixelRow *src2_row,
		   PixelRow *dest_row
		   )
{
  gint alpha, b;
  gfloat screen, mult;
  Tag     src1_tag      = pixelrow_tag (src1_row); 
  Tag     src2_tag      = pixelrow_tag (src2_row); 
  gint    ha1           = (tag_alpha (src1_tag)==ALPHA_YES)? TRUE: FALSE;
  gint    ha2           = (tag_alpha (src2_tag)==ALPHA_YES)? TRUE: FALSE;
  gfloat *dest          = (gfloat*)pixelrow_data (dest_row);
  gfloat *src1          = (gfloat*)pixelrow_data (src1_row);
  gfloat *src2          = (gfloat*)pixelrow_data (src2_row);
  gint    width         = pixelrow_width (dest_row);
  gint    num_channels1 = tag_num_channels (src1_tag);
  gint    num_channels2 = tag_num_channels (src2_tag);

  alpha = (ha1 || ha2) ? MAXIMUM (num_channels1, num_channels2) - 1 : num_channels1;

  while (width --)
    {
      for (b = 0; b < alpha; b++)
	{
	  screen = 1.0 - ((1.0 - src1[b]) * (1.0 - src2[b])); 
	  mult = src1[b] * src2[b] ;
	  dest[b] = screen * src1[b] + mult * (1.0 - src1[b]);
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
add_row_float ( 
	       PixelRow *src1_row,
	       PixelRow *src2_row,
	       PixelRow *dest_row
	      )
{
  gint alpha, b;
  gfloat sum;
  Tag     src1_tag      = pixelrow_tag (src1_row); 
  Tag     src2_tag      = pixelrow_tag (src2_row); 
  gint    ha1           = (tag_alpha (src1_tag)==ALPHA_YES)? TRUE: FALSE;
  gint    ha2           = (tag_alpha (src2_tag)==ALPHA_YES)? TRUE: FALSE;
  gfloat *dest          = (gfloat*)pixelrow_data (dest_row);
  gfloat *src1          = (gfloat*)pixelrow_data (src1_row);
  gfloat *src2          = (gfloat*)pixelrow_data (src2_row);
  gint    width         = pixelrow_width (dest_row);
  gint    num_channels1 = tag_num_channels (src1_tag);
  gint    num_channels2 = tag_num_channels (src2_tag);

  alpha = (ha1 || ha2) ? MAXIMUM (num_channels1, num_channels2) - 1 : num_channels1;

  while (width --)
    {
      for (b = 0; b < alpha; b++)
	{
	  sum = src1[b] + src2[b];
	  dest[b] = (sum > 1.0) ? 1.0 : sum;
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
subtract_row_float (
		    PixelRow *src1_row,
		    PixelRow *src2_row,
		    PixelRow *dest_row
		    )
{
  gint alpha, b;
  gfloat diff;
  Tag     src1_tag      = pixelrow_tag (src1_row); 
  Tag     src2_tag      = pixelrow_tag (src2_row); 
  gint    ha1           = (tag_alpha (src1_tag)==ALPHA_YES)? TRUE: FALSE;
  gint    ha2           = (tag_alpha (src2_tag)==ALPHA_YES)? TRUE: FALSE;
  gfloat *dest          = (gfloat*)pixelrow_data (dest_row);
  gfloat *src1          = (gfloat*)pixelrow_data (src1_row);
  gfloat *src2          = (gfloat*)pixelrow_data (src2_row);
  gint    width         = pixelrow_width (dest_row);
  gint    num_channels1 = tag_num_channels (src1_tag);
  gint    num_channels2 = tag_num_channels (src2_tag);

  alpha = (ha1 || ha2) ? MAXIMUM (num_channels1, num_channels2) - 1 : num_channels1;

  while (width --)
    {
      for (b = 0; b < alpha; b++)
	{
	  diff = src1[b] - src2[b];
	  dest[b] = (diff < 0.0) ? 0.0 : diff;
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
difference_row_float (
		      PixelRow *src1_row,
		      PixelRow *src2_row,
		      PixelRow *dest_row
		      )
{
  gint alpha, b;
  gfloat diff;
  Tag     src1_tag      = pixelrow_tag (src1_row); 
  Tag     src2_tag      = pixelrow_tag (src2_row); 
  gint    ha1           = (tag_alpha (src1_tag)==ALPHA_YES)? TRUE: FALSE;
  gint    ha2           = (tag_alpha (src2_tag)==ALPHA_YES)? TRUE: FALSE;
  gfloat *dest          = (gfloat*)pixelrow_data (dest_row);
  gfloat *src1          = (gfloat*)pixelrow_data (src1_row);
  gfloat *src2          = (gfloat*)pixelrow_data (src2_row);
  gint    width         = pixelrow_width (dest_row);
  gint    num_channels1 = tag_num_channels (src1_tag);
  gint    num_channels2 = tag_num_channels (src2_tag);

  alpha = (ha1 || ha2) ? MAXIMUM (num_channels1, num_channels2) - 1 : num_channels1;

  while (width --)
    {
      for (b = 0; b < alpha; b++)
	{
	  diff = src1[b] - src2[b];
	  dest[b] = (diff < 0.0) ? -diff : diff;
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
dissolve_row_float  (
                     PixelRow * src_row,
                     PixelRow * dest_row,
                     int x,
                     int y,
                     gfloat opacity
                     )
{
  gint alpha, b;
  gfloat rand_val;
  Tag     src_tag          = pixelrow_tag (src_row); 
  Tag     dest_tag         = pixelrow_tag (dest_row); 
  gint    has_alpha        = (tag_alpha (src_tag)==ALPHA_YES)? TRUE: FALSE;
  gfloat *dest             = (gfloat*)pixelrow_data (dest_row);
  gfloat *src              = (gfloat*)pixelrow_data (src_row);
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
      rand_val = rand() / (gfloat)RAND_MAX;

      if (has_alpha)
	dest[alpha] = (rand_val > opacity) ? 0.0 : src[alpha];
      else
	dest[alpha] = (rand_val > opacity) ? 0.0 : OPAQUE_FLOAT;

      dest += dest_num_channels;
      src += src_num_channels;
    }
}


void 
replace_row_float  (
                    PixelRow * src1_row,
                    PixelRow * src2_row,
                    PixelRow * dest_row,
                    PixelRow * mask_row,
                    gfloat opacity,
                    int * affect
                    )
{
  gint alpha;
  gint b;
  double a_val, a_recip, mask_val;
  gfloat s1_a, s2_a;
  gfloat new_val;
  Tag     src1_tag      = pixelrow_tag (src1_row); 
  Tag     src2_tag      = pixelrow_tag (src2_row); 
  gfloat *dest          = (gfloat*)pixelrow_data (dest_row);
  gfloat *src1          = (gfloat*)pixelrow_data (src1_row);
  gfloat *src2          = (gfloat*)pixelrow_data (src2_row);
  gfloat *mask          = (gfloat*)pixelrow_data (mask_row);
  gint    width         = pixelrow_width (dest_row);
  gint    num_channels1 = tag_num_channels (src1_tag);
  gint    num_channels2 = tag_num_channels (src2_tag);

  if (num_channels1 != num_channels2)
    {
      g_warning ("replace_pixels only works on commensurate pixel regions");
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
      if (a_val == 0.0)
	a_recip = 0.0;
      else
	a_recip = 1.0 / a_val;
      /* possible optimization: fold a_recip into s1_a and s2_a */
      for (b = 0; b < alpha; b++)
	{
	  new_val =  a_recip * (src1[b] * s1_a + mask_val * (src2[b] * s2_a - src1[b] * s1_a));
	  dest[b] = affect[b] ? MIN (new_val, 1.0) : src1[b];
	}
      dest[alpha] = affect[alpha] ? a_val : s1_a;
      src1 += num_channels1;
      src2 += num_channels2;
      dest += num_channels2;
      mask++;
    }
}


void
swap_row_float (
                PixelRow *src_row,
	        PixelRow *dest_row
	        )
{
  gfloat *dest = (gfloat*)pixelrow_data (dest_row);
  gfloat *src  = (gfloat*)pixelrow_data (src_row);
  gfloat temp;
  gint    width = pixelrow_width (dest_row) * tag_num_channels (pixelrow_tag (src_row));
  
  while (width--)
    {
       temp = *dest;
      *dest = *src;
      *src = temp;

      src++;
      dest++;
    }
}


void
scale_row_float (
		 PixelRow *src_row,
		 PixelRow *dest_row,
		 gfloat       scale
		  )
{
  gfloat *dest  = (gfloat*)pixelrow_data (dest_row);
  gfloat *src   = (gfloat*)pixelrow_data (src_row);
  gint    width = pixelrow_width (dest_row);
  
  while (width --)
    *dest++ = *src++ * scale;
}


void
add_alpha_row_float (
		  PixelRow *src_row,
		  PixelRow *dest_row
		  )
{
  gint alpha, b;
  Tag     src_tag      = pixelrow_tag (src_row); 
  gfloat *dest         = (gfloat*)pixelrow_data (dest_row);
  gfloat *src          = (gfloat*)pixelrow_data (src_row);
  gint    width        = pixelrow_width (dest_row);
  gint    num_channels = tag_num_channels (src_tag);

  alpha = num_channels + 1;
  while (width --)
    {
      for (b = 0; b < num_channels; b++)
	dest[b] = src[b];

      dest[b] = OPAQUE_FLOAT;

      src += num_channels;
      dest += alpha;
    }
}


void
flatten_row_float (
		   PixelRow *src_row,
		   PixelRow *dest_row,
 		   PixelRow *background
		  )
{
  gint alpha, b;
  Tag     src_tag      = pixelrow_tag (src_row); 
  gfloat *dest         = (gfloat*)pixelrow_data (dest_row);
  gfloat *src          = (gfloat*)pixelrow_data (src_row);
  gint    width        = pixelrow_width (dest_row);
  gint    num_channels = tag_num_channels (src_tag);
  gfloat *bg           = (gfloat*) pixelrow_data (background);

  alpha = num_channels - 1;
  while (width --)
    {
      for (b = 0; b < alpha; b++)
	dest[b] = src[b] * src[alpha] + bg[b] * (1.0 - src[alpha]);

      src += num_channels;
      dest += alpha;
    }
}


void
multiply_alpha_row_float( 
		      PixelRow * src_row
		     )
{
  gint b;
  gfloat alpha;
  gfloat *src          =(gfloat*)pixelrow_data (src_row);
  gint    num_channels = tag_num_channels (pixelrow_tag (src_row));
  gint    width        = pixelrow_width (src_row);

  while (width --)
    {
      alpha = src[num_channels-1];
      for (b = 0; b < num_channels - 1; b++)
	src[b] =  alpha *  src[b]; 
      src += num_channels;
    }
}


void separate_alpha_row_float( 
			    PixelRow *src_row
                           )
{
  gint x, b;
  double alpha_recip;
  gfloat new_val; 
  int alpha;
  gfloat *src          =(gfloat*)pixelrow_data (src_row);
  gint    num_channels = tag_num_channels (pixelrow_tag (src_row));
  gint    width        = pixelrow_width (src_row);
  
  alpha = num_channels-1;
  for (x = 0; x < width; x++)
    {
      if (src[alpha] != 0.0 && src[alpha] != 1.0)
	{
	  alpha_recip = 1.0 / src[alpha];
	  for (b = 0; b < num_channels - 1; b++)
	    {
	      new_val =  src[b] * alpha_recip;
	      new_val = MIN (new_val, 1.0);
	      src[b] = new_val;
	    }
	}
      src += num_channels;
    }
} 


void
gray_to_rgb_row_float (
		       PixelRow *src_row,
		       PixelRow *dest_row
		       )
{
  gint b;
  gint dest_num_channels;
  gint has_alpha;
  gfloat *src          = (gfloat*)pixelrow_data (src_row);
  gfloat *dest         = (gfloat*)pixelrow_data (dest_row);
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
apply_mask_to_alpha_channel_row_float  (
                                        PixelRow * src_row,
                                        PixelRow * mask_row,
                                        gfloat opacity
                                        )
{
  gint alpha;
  gfloat *src          = (gfloat*)pixelrow_data (src_row);
  gfloat *mask         = (gfloat*)pixelrow_data (mask_row);
  gint    num_channels = tag_num_channels (pixelrow_tag (src_row));
  gint    width        = pixelrow_width (src_row);

  alpha = num_channels - 1;
  while (width --)
    {
      src[alpha] = src[alpha] * *mask++ * opacity;
      src += num_channels;
    }
}


/*  combine the mask data with the alpha channel of the pixel data  */
void 
combine_mask_and_alpha_channel_row_float  (
                                           PixelRow * src_row,
                                           PixelRow * mask_row,
                                           gfloat opacity
                                           )
{
  gfloat mask_val;
  gint alpha;
  gfloat *src          = (gfloat*)pixelrow_data (src_row);
  gfloat *mask         = (gfloat*)pixelrow_data (mask_row);
  gint    num_channels = tag_num_channels (pixelrow_tag (src_row));
  gint    width        = pixelrow_width (src_row);

  alpha = num_channels - 1;
  while (width --)
    {
      mask_val = *mask++ * opacity;
      src[alpha] = src[alpha] + (1.0 - src[alpha]) * mask_val;
      src += num_channels;
    }
}


/*  copy gray pixels to intensity-alpha pixels.  This function
 *  essentially takes a source that is only a grayscale image and
 *  copies it to the destination, expanding to RGB if necessary and
 *  adding an alpha channel.  (OPAQUE)
 */
void
copy_gray_to_inten_a_row_float (
				PixelRow *src_row,
				PixelRow *dest_row
			        )
{
  gint b;
  gint alpha;
  gfloat *src          = (gfloat*)pixelrow_data (src_row);
  gfloat *dest         = (gfloat*)pixelrow_data (dest_row);
  gint    num_channels = tag_num_channels (pixelrow_tag (dest_row));
  gint    width        = pixelrow_width (src_row);

  alpha = num_channels - 1;
  while (width --)
    {
      for (b = 0; b < alpha; b++)
	dest[b] = *src;
      dest[b] = OPAQUE_FLOAT;

      src ++;
      dest += num_channels;
    }
}


/*  lay down the initial pixels in the case of only one
 *  channel being visible and no layers...In this singular
 *  case, we want to display a grayscale image w/o transparency
 */
void
initial_channel_row_float (
			   PixelRow *src_row,
			   PixelRow *dest_row
			  )
{
  gint alpha, b;
  gfloat *src          = (gfloat*)pixelrow_data (src_row);
  gfloat *dest         = (gfloat*)pixelrow_data (dest_row);
  gint    num_channels = tag_num_channels (pixelrow_tag (dest_row));
  gint    width        = pixelrow_width (src_row);

  alpha = num_channels - 1;
  while (width --)
    {
      for (b = 0; b < alpha; b++)
	dest[b] = src[0];

      dest[alpha] = OPAQUE_FLOAT;

      dest += num_channels;
      src ++;
    }
}


/*  lay down the initial pixels for the base layer.
 *  This process obviously requires no composition.
 */
void 
initial_inten_row_float  (
                          PixelRow * src_row,
                          PixelRow * dest_row,
                          PixelRow * mask_row,
                          gfloat opacity,
                          int * affect
                          )
{
  gint b, dest_num_channels;
  gfloat * m;
  gfloat *src          = (gfloat*)pixelrow_data (src_row);
  gfloat *dest         = (gfloat*)pixelrow_data (dest_row);
  gfloat *mask         = (gfloat*)pixelrow_data (mask_row);
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
	    dest [b] = affect [b] ? src [b] : 0.0;
	    
	  /*  Set the alpha channel  */
	  dest[b] = affect [b] ? opacity * *m : 0.0;
	    
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
	    dest [b] = affect [b] ? src [b] : 0.0;
	    
	  /*  Set the alpha channel  */
	  dest[b] = affect [b] ? opacity : 0.0;
	    
	  dest += dest_num_channels;
	  src += num_channels;
	}
    }
}


/*  lay down the initial pixels for the base layer.
 *  This process obviously requires no composition.
 */
void 
initial_inten_a_row_float  (
                            PixelRow * src_row,
                            PixelRow * dest_row,
                            PixelRow * mask_row,
                            gfloat opacity,
                            int * affect
                            )
{
  gint alpha, b;
  gfloat * m;
  gfloat *src          = (gfloat*)pixelrow_data (src_row);
  gfloat *dest         = (gfloat*)pixelrow_data (dest_row);
  gfloat *mask         = (gfloat*)pixelrow_data (mask_row);
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
	  dest[alpha] = affect [alpha] ? (opacity * src[alpha] * *m)  : 0.0;
	  
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
	  dest[alpha] = affect [alpha] ? (opacity * src[alpha]) : 0.0;
	  
	  dest += num_channels;
	  src += num_channels;
	}
    }
}


/*  combine RGB image with RGB or GRAY with GRAY
 *  destination is intensity-only...
 */
void 
combine_inten_and_inten_row_float  (
                                    PixelRow * src1_row,
                                    PixelRow * src2_row,
                                    PixelRow * dest_row,
                                    PixelRow * mask_row,
                                    gfloat opacity,
                                    int * affect
                                    )
{
  gint b;
  gfloat new_alpha;
  gfloat * m;
  gfloat *src1         = (gfloat*)pixelrow_data (src1_row);
  gfloat *src2         = (gfloat*)pixelrow_data (src2_row);
  gfloat *dest         = (gfloat*)pixelrow_data (dest_row);
  gfloat *mask         = (gfloat*)pixelrow_data (mask_row);
  gint    width        = pixelrow_width (src1_row);
  gint    num_channels = tag_num_channels (pixelrow_tag (src1_row));

  if (mask)
    {
      m = mask;
      while (width --)
	{
	  new_alpha = *m * opacity;

	  for (b = 0; b < num_channels; b++)
	    dest[b] = (affect[b]) ?
	      (src2[b] * new_alpha + src1[b] * (1.0 - new_alpha)): src1[b];

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
	  new_alpha = opacity;

	  for (b = 0; b < num_channels; b++)
	    dest[b] = (affect[b]) ?
	      (src2[b] * new_alpha + src1[b] * (1.0 - new_alpha)): src1[b];

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
combine_inten_and_inten_a_row_float  (
                                      PixelRow * src1_row,
                                      PixelRow * src2_row,
                                      PixelRow * dest_row,
                                      PixelRow * mask_row,
                                      gfloat opacity,
                                      int * affect
                                      )
{
  gint alpha, b;
  gint src2_num_channels;
  gfloat new_alpha;
  gfloat * m;
  gfloat *src1         = (gfloat*)pixelrow_data (src1_row);
  gfloat *src2         = (gfloat*)pixelrow_data (src2_row);
  gfloat *dest         = (gfloat*)pixelrow_data (dest_row);
  gfloat *mask         = (gfloat*)pixelrow_data (mask_row);
  gint    width        = pixelrow_width (src1_row);
  gint    num_channels = tag_num_channels (pixelrow_tag (src1_row));

  alpha = num_channels;
  src2_num_channels = num_channels + 1;

  if (mask)
    {
      m = mask;
      while (width --)
	{
	  new_alpha = src2[alpha] * *m * opacity ;

	  for (b = 0; b < num_channels; b++)
	    dest[b] = (affect[b]) ?
	      src2[b] * new_alpha + src1[b] * (1.0 - new_alpha) : src1[b];

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
	  new_alpha = src2[alpha] * opacity;

	  for (b = 0; b < num_channels; b++)
	    dest[b] = (affect[b]) ?
	      src2[b] * new_alpha + src1[b] * (1.0 - new_alpha) : src1[b];

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
	       (src2[b] * ratio + src1[b] * compl_ratio) : src1[b];	                        \
	}


/*  combine an RGB or GRAY image with an RGBA or GRAYA image
 *  destination is intensity-alpha...
 */
void 
combine_inten_a_and_inten_row_float  (
                                      PixelRow * src1_row,
                                      PixelRow * src2_row,
                                      PixelRow * dest_row,
                                      PixelRow * mask_row,
                                      gfloat opacity,
                                      int * affect,
                                      int mode_affect /* how does the combination mode affect alpha ? */
                                      )
{
  gint alpha, b;
  gint src2_num_channels;
  gfloat src2_alpha;
  gfloat new_alpha;
  gfloat * m;
  gfloat ratio, compl_ratio;
  gfloat *src1         = (gfloat*)pixelrow_data (src1_row);
  gfloat *src2         = (gfloat*)pixelrow_data (src2_row);
  gfloat *dest         = (gfloat*)pixelrow_data (dest_row);
  gfloat *mask         = (gfloat*)pixelrow_data (mask_row);
  gint    width        = pixelrow_width (src1_row);
  gint    num_channels = tag_num_channels (pixelrow_tag (src1_row));

  src2_num_channels = num_channels - 1;
  alpha = num_channels - 1;

  if (mask)
    {
      m = mask;
      while (width --)
	{
	  src2_alpha = *m * opacity ;
	  new_alpha = src1[alpha] + (1.0 - src1[alpha]) * src2_alpha;
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
	  src2_alpha = opacity;
	  new_alpha = src1[alpha] + (1.0 - src1[alpha]) * src2_alpha;
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
combine_inten_a_and_inten_a_row_float  (
                                        PixelRow * src1_row,
                                        PixelRow * src2_row,
                                        PixelRow * dest_row,
                                        PixelRow * mask_row,
                                        gfloat opacity,
                                        int * affect,
                                        int mode_affect
                                        )
{
  gint alpha, b;
  gfloat src2_alpha;
  gfloat new_alpha;
  gfloat * m;
  float ratio, compl_ratio;
  gfloat *src1         = (gfloat*)pixelrow_data (src1_row);
  gfloat *src2         = (gfloat*)pixelrow_data (src2_row);
  gfloat *dest         = (gfloat*)pixelrow_data (dest_row);
  gfloat *mask         = (gfloat*)pixelrow_data (mask_row);
  gint    width        = pixelrow_width (src1_row);
  gint    num_channels = tag_num_channels (pixelrow_tag (src1_row));

  alpha = num_channels - 1;
  if (mask){
    m = mask;
    while (width --)
      {
	src2_alpha = src2[alpha] * *m * opacity;
	new_alpha = src1[alpha] + (1.0 - src1[alpha]) * src2_alpha;

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
	src2_alpha = src2[alpha] * opacity;
	new_alpha = src1[alpha] + (1.0 - src1[alpha]) * src2_alpha;

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
combine_inten_a_and_channel_mask_row_float  (
                                             PixelRow * src_row,
                                             PixelRow * channel_row,
                                             PixelRow * dest_row,
                                             PixelRow * col,
                                             gfloat opacity
                                             )
{
  gint alpha, b;
  gfloat channel_alpha;
  gfloat new_alpha;
  gfloat compl_alpha;
  gfloat *src          = (gfloat*)pixelrow_data (src_row);
  gfloat *dest         = (gfloat*)pixelrow_data (dest_row);
  gfloat *channel      = (gfloat*)pixelrow_data (channel_row);
  gint    width        = pixelrow_width (src_row);
  gint    num_channels = tag_num_channels (pixelrow_tag (src_row));
  gfloat *color        = (gfloat*) pixelrow_data (col);

  alpha = num_channels - 1;
  while (width --)
    {
      channel_alpha =(1.0 - *channel) * opacity;
      if (channel_alpha)
	{
	  new_alpha = src[alpha] + (1.0 - src[alpha]) * channel_alpha;

	  if (new_alpha != 1.0)
	    channel_alpha = channel_alpha / new_alpha;
	  compl_alpha = 1.0 - channel_alpha;

	  for (b = 0; b < alpha; b++)
	    dest[b] = color[b] * channel_alpha + src[b] * compl_alpha;
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
combine_inten_a_and_channel_selection_row_float  (
                                                  PixelRow * src_row,
                                                  PixelRow * channel_row,
                                                  PixelRow * dest_row,
                                                  PixelRow * col,
                                                  gfloat opacity
                                                  )
{
  gint alpha, b;
  gfloat channel_alpha;
  gfloat new_alpha;
  gfloat compl_alpha;
  gfloat *src          = (gfloat*)pixelrow_data (src_row);
  gfloat *dest         = (gfloat*)pixelrow_data (dest_row);
  gfloat *channel      = (gfloat*)pixelrow_data (channel_row);
  gint    width        = pixelrow_width (src_row);
  gint    num_channels = tag_num_channels (pixelrow_tag (src_row));
  gfloat *color        = (gfloat*) pixelrow_data (col);

  alpha = num_channels - 1;
  while (width --)
    {
      channel_alpha = *channel * opacity;
      if (channel_alpha)
	{
	  new_alpha = src[alpha] + (1.0 - src[alpha]) * channel_alpha;

	  if (new_alpha != 1.0)
	    channel_alpha = channel_alpha  / new_alpha;
	  compl_alpha = 1.0 - channel_alpha;

	  for (b = 0; b < alpha; b++)
	    dest[b] = color[b] * channel_alpha + src[b]* compl_alpha;
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
behind_inten_row_float  (
                         PixelRow * src1_row,
                         PixelRow * src2_row,
                         PixelRow * dest_row,
                         PixelRow * mask_row,
                         gfloat opacity,
                         int * affect
                         )
{
  gint alpha, b;
  gfloat src1_alpha;
  gfloat src2_alpha;
  gfloat new_alpha;
  gfloat * m;
  gfloat ratio, compl_ratio;
  gfloat *src1          = (gfloat*)pixelrow_data (src1_row);
  gfloat *src2          = (gfloat*)pixelrow_data (src2_row);
  gfloat *dest          = (gfloat*)pixelrow_data (dest_row);
  gfloat *mask          = (gfloat*)pixelrow_data (mask_row);
  gint    width         = pixelrow_width (src1_row);
  gint    src1_num_channels = tag_num_channels (pixelrow_tag (src1_row));
  gint    src2_num_channels = tag_num_channels (pixelrow_tag (src2_row));

  if (mask)
    m = mask;
  else
    m = &no_mask;

  /*  the alpha channel  */
  alpha = src1_num_channels - 1;

  while (width --)
    {
      src1_alpha = src1[alpha];
      src2_alpha = src2[alpha] * *m * opacity;
      new_alpha = src2_alpha + (1.0 - src2_alpha) * src1_alpha; 
      if (new_alpha)
	ratio = src1_alpha / new_alpha;
      else
	ratio = 0.0;
      compl_ratio = 1.0 - ratio;

      for (b = 0; b < alpha; b++)
	dest[b] = (affect[b]) ?  src1[b] * ratio + src2[b] * compl_ratio : src1[b];

      dest[alpha] = (affect[alpha]) ? new_alpha : src1[alpha];

      if (mask)
	m++;

      src1 += src1_num_channels;
      src2 += src2_num_channels;
      dest += src1_num_channels;
    }
}


/*  replace the contents of one pixel row with the other
 *  The operation is still bounded by mask/opacity constraints
 */
void 
replace_inten_row_float  (
                          PixelRow * src1_row,
                          PixelRow * src2_row,
                          PixelRow * dest_row,
                          PixelRow * mask_row,
                          gfloat opacity,
                          int * affect
                          )
{
  gint num_channels, b;
  gfloat mask_alpha;
  gfloat * m;
  Tag     src1_tag      = pixelrow_tag (src1_row); 
  Tag     src2_tag      = pixelrow_tag (src2_row); 
  gfloat *src1          = (gfloat*)pixelrow_data (src1_row);
  gfloat *src2          = (gfloat*)pixelrow_data (src2_row);
  gfloat *dest          = (gfloat*)pixelrow_data (dest_row);
  gfloat *mask          = (gfloat*)pixelrow_data (mask_row);
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
      mask_alpha = *m * opacity ;

      for (b = 0; b < num_channels; b++)
	dest[b] = (affect[b]) ?  (src2[b] * mask_alpha + src1[b] *(1.0 - mask_alpha)): src1[b];

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
erase_inten_row_float  (
                        PixelRow * src1_row,
                        PixelRow * src2_row,
                        PixelRow * dest_row,
                        PixelRow * mask_row,
                        gfloat opacity,
                        int * affect
                        )
{
  gint alpha, b;
  gfloat src2_alpha;
  gfloat * m;
  gfloat *src1          = (gfloat*)pixelrow_data (src1_row);
  gfloat *src2          = (gfloat*)pixelrow_data (src2_row);
  gfloat *dest          = (gfloat*)pixelrow_data (dest_row);
  gfloat *mask          = (gfloat*)pixelrow_data (mask_row);
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

      src2_alpha = src2[alpha] * *m * opacity ;
      dest[alpha] = src1[alpha] - (src1[alpha] * src2_alpha) ;

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
extract_from_inten_row_float (
			      PixelRow *src_row,
			      PixelRow *dest_row,
			      PixelRow *mask_row,
			      PixelRow *background,
			      int       cut
			      )
{
  gint b, alpha;
  gint dest_num_channels;
  gfloat * m;
  Tag     src_tag       = pixelrow_tag (src_row); 
  gfloat *src           = (gfloat*)pixelrow_data (src_row);
  gfloat *dest          = (gfloat*)pixelrow_data (dest_row);
  gfloat *mask          = (gfloat*)pixelrow_data (mask_row);
  gint    width         = pixelrow_width (src_row);
  gint    num_channels  = tag_num_channels (src_tag);
  gint    has_alpha      = (tag_alpha (src_tag)==ALPHA_YES)? TRUE: FALSE;
  gfloat *bg            = (gfloat*) pixelrow_data (background);

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
	  dest[alpha] = *m * src[alpha];
	  if (cut)
	    src[alpha] = (1.0 - *m) * src[alpha] ;
	}
      else
	{
	  dest[alpha] = *m;
	  if (cut)
	    for (b = 0; b < num_channels; b++)
	      src[b] = *m * bg[b] + (1.0 - *m) * src[b] ;
	}

      if (mask)
	m++;

      src += num_channels;
      dest += dest_num_channels;
    }
}


/*******************************************************
                     copy routines
********************************************************/

#define INTENSITY(r,g,b) (r * 0.30 + g * 0.59 + b * 0.11 + 0.001)

static void
copy_row_rgb_to_u8_rgb (
                        PixelRow * src_row,
                        PixelRow * dest_row
                        )
{
  int w = MIN (pixelrow_width (src_row), pixelrow_width (dest_row));
  Tag stag = pixelrow_tag (src_row);
  Tag dtag = pixelrow_tag (dest_row);
  gfloat * s = (gfloat*) pixelrow_data (src_row);
  guint8 * d = (guint8*) pixelrow_data (dest_row);

  if (tag_alpha (stag) == ALPHA_YES)
    {
      if (tag_alpha (dtag) == ALPHA_YES)
        while (w--)
          {
            d[0] = s[0] * 255;
            d[1] = s[1] * 255;
            d[2] = s[2] * 255;
            d[3] = s[3] * 255;
            s += 4;
            d += 4;
          }
      else
        while (w--)
          {
            d[0] = s[0] * 255;
            d[1] = s[1] * 255;
            d[2] = s[2] * 255;
            s += 4;
            d += 3;
          }
    }
  else
    {
      if (tag_alpha (dtag) == ALPHA_YES)
        while (w--)
          {
            d[0] = s[0] * 255;
            d[1] = s[1] * 255;
            d[2] = s[2] * 255;
            d[3] = 255;
            s += 3;
            d += 4;
          }
      else
        while (w--)
          {
            d[0] = s[0] * 255;
            d[1] = s[1] * 255;
            d[2] = s[2] * 255;
            s += 3;
            d += 3;
          }
    }
}


static void 
copy_row_rgb_to_u8_gray  (
                          PixelRow * src_row,
                          PixelRow * dest_row
                          )
{
  int w = MIN (pixelrow_width (src_row), pixelrow_width (dest_row));
  Tag stag = pixelrow_tag (src_row);
  Tag dtag = pixelrow_tag (dest_row);
  gfloat * s = (gfloat*) pixelrow_data (src_row);
  guint8 * d = (guint8*) pixelrow_data (dest_row);

  if (tag_alpha (stag) == ALPHA_YES)
    {
      if (tag_alpha (dtag) == ALPHA_YES)
        while (w--)
          {
            d[0] = INTENSITY (s[0]*255, s[1]*255, s[2]*255);
            d[1] = s[3] * 255;
            s += 4;
            d += 2;
          }
      else
        while (w--)
          {
            d[0] = INTENSITY (s[0]*255, s[1]*255, s[2]*255);
            s += 4;
            d += 1;
          }
    }
  else
    {
      if (tag_alpha (dtag) == ALPHA_YES)
        while (w--)
          {
            d[0] = INTENSITY (s[0]*255, s[1]*255, s[2]*255);
            d[1] = 255;
            s += 3;
            d += 2;
          }
      else
        while (w--)
          {
            d[0] = INTENSITY (s[0]*255, s[1]*255, s[2]*255);
            s += 3;
            d += 1;
          }
    }
}

static void 
copy_row_rgb_to_u16_rgb  (
                          PixelRow * src_row,
                          PixelRow * dest_row
                          )
{
  int w = MIN (pixelrow_width (src_row), pixelrow_width (dest_row));
  Tag stag = pixelrow_tag (src_row);
  Tag dtag = pixelrow_tag (dest_row);
  gfloat * s = (gfloat*) pixelrow_data (src_row);
  guint16 * d = (guint16*) pixelrow_data (dest_row);

  if (tag_alpha (stag) == ALPHA_YES)
    {
      if (tag_alpha (dtag) == ALPHA_YES)
        while (w--)
          {
            d[0] = s[0] * 65535;
            d[1] = s[1] * 65535;
            d[2] = s[2] * 65535;
            d[3] = s[3] * 65535;
            s += 4;
            d += 4;
          }
      else
        while (w--)
          {
            d[0] = s[0] * 65535;
            d[1] = s[1] * 65535;
            d[2] = s[2] * 65535;
            s += 4;
            d += 3;
          }
    }
  else
    {
      if (tag_alpha (dtag) == ALPHA_YES)
        while (w--)
          {
            d[0] = s[0] * 65535;
            d[1] = s[1] * 65535;
            d[2] = s[2] * 65535;
            d[3] = 65535;
            s += 3;
            d += 4;
          }
      else
        while (w--)
          {
            d[0] = s[0] * 65535;
            d[1] = s[1] * 65535;
            d[2] = s[2] * 65535;
            s += 3;
            d += 3;
          }
    }
}


static void 
copy_row_rgb_to_u16_gray  (
                           PixelRow * src_row,
                           PixelRow * dest_row
                           )
{
  int w = MIN (pixelrow_width (src_row), pixelrow_width (dest_row));
  Tag stag = pixelrow_tag (src_row);
  Tag dtag = pixelrow_tag (dest_row);
  gfloat * s = (gfloat*) pixelrow_data (src_row);
  guint16 * d = (guint16*) pixelrow_data (dest_row);

  if (tag_alpha (stag) == ALPHA_YES)
    {
      if (tag_alpha (dtag) == ALPHA_YES)
        while (w--)
          {
            d[0] = INTENSITY ((s[0]*65535), (s[1]*65535), (s[2]*65535));
            d[1] = s[3] * 65535;
            s += 4;
            d += 2;
          }
      else
        while (w--)
          {
            d[0] = INTENSITY ((s[0]*65535), (s[1]*65535), (s[2]*65535));
            s += 4;
            d += 1;
          }
    }
  else
    {
      if (tag_alpha (dtag) == ALPHA_YES)
        while (w--)
          {
            d[0] = INTENSITY ((s[0]*65535), (s[1]*65535), (s[2]*65535));
            d[1] = 65535;
            s += 3;
            d += 2;
          }
      else
        while (w--)
          {
            d[0] = INTENSITY ((s[0]*65535), (s[1]*65535), (s[2]*65535));
            s += 3;
            d += 1;
          }
    }
}


static void 
copy_row_rgb_to_float_rgb  (
                            PixelRow * src_row,
                            PixelRow * dest_row
                            )
{
  int w = MIN (pixelrow_width (src_row), pixelrow_width (dest_row));
  Tag stag = pixelrow_tag (src_row);
  Tag dtag = pixelrow_tag (dest_row);
  gfloat * s = (gfloat*) pixelrow_data (src_row);
  gfloat * d = (gfloat*) pixelrow_data (dest_row);

  if (tag_alpha (stag) == ALPHA_YES)
    {
      if (tag_alpha (dtag) == ALPHA_YES)
        while (w--)
          {
            d[0] = s[0];
            d[1] = s[1];
            d[2] = s[2];
            d[3] = s[3];
            s += 4;
            d += 4;
          }
      else
        while (w--)
          {
            d[0] = s[0];
            d[1] = s[1];
            d[2] = s[2];
            s += 4;
            d += 3;
          }
    }
  else
    {
      if (tag_alpha (dtag) == ALPHA_YES)
        while (w--)
          {
            d[0] = s[0];
            d[1] = s[1];
            d[2] = s[2];
            d[3] = 1.0;
            s += 3;
            d += 4;
          }
      else
        while (w--)
          {
            d[0] = s[0];
            d[1] = s[1];
            d[2] = s[2];
            s += 3;
            d += 3;
          }
    }
}


static void 
copy_row_rgb_to_float_gray  (
                             PixelRow * src_row,
                             PixelRow * dest_row
                             )
{
  int w = MIN (pixelrow_width (src_row), pixelrow_width (dest_row));
  Tag stag = pixelrow_tag (src_row);
  Tag dtag = pixelrow_tag (dest_row);
  gfloat * s = (gfloat*) pixelrow_data (src_row);
  gfloat * d = (gfloat*) pixelrow_data (dest_row);

  if (tag_alpha (stag) == ALPHA_YES)
    {
      if (tag_alpha (dtag) == ALPHA_YES)
        while (w--)
          {
            d[0] = INTENSITY (s[0], s[1], s[2]);
            d[1] = s[3];
            s += 4;
            d += 2;
          }
      else
        while (w--)
          {
            d[0] = INTENSITY (s[0], s[1], s[2]);
            s += 4;
            d += 1;
          }
    }
  else
    {
      if (tag_alpha (dtag) == ALPHA_YES)
        while (w--)
          {
            d[0] = INTENSITY (s[0], s[1], s[2]);
            d[1] = 1.0;
            s += 3;
            d += 2;
          }
      else
        while (w--)
          {
            d[0] = INTENSITY (s[0], s[1], s[2]);
            s += 3;
            d += 1;
          }
    }
}


static void 
copy_row_gray_to_u8_rgb  (
                          PixelRow * src_row,
                          PixelRow * dest_row
                          )
{
  int w = MIN (pixelrow_width (src_row), pixelrow_width (dest_row));
  Tag stag = pixelrow_tag (src_row);
  Tag dtag = pixelrow_tag (dest_row);
  gfloat * s = (gfloat*) pixelrow_data (src_row);
  guint8 * d = (guint8*) pixelrow_data (dest_row);

  if (tag_alpha (stag) == ALPHA_YES)
    {
      if (tag_alpha (dtag) == ALPHA_YES)
        while (w--)
          {
            d[0] = d[1] = d[2] = s[0] * 255;
            d[3] = s[1] * 255;
            s += 2;
            d += 4;
          }
      else
        while (w--)
          {
            d[0] = d[1] = d[2] = s[0] * 255;
            s += 2;
            d += 3;
          }
    }
  else
    {
      if (tag_alpha (dtag) == ALPHA_YES)
        while (w--)
          {
            d[0] = d[1] = d[2] = s[0] * 255;
            d[3] = 255;
            s += 1;
            d += 4;
          }
      else
        while (w--)
          {
            d[0] = d[1] = d[2] = s[0] * 255;
            s += 1;
            d += 3;
          }
    }
}

static void 
copy_row_gray_to_u8_gray  (
                           PixelRow * src_row,
                           PixelRow * dest_row
                           )
{
  int w = MIN (pixelrow_width (src_row), pixelrow_width (dest_row));
  Tag stag = pixelrow_tag (src_row);
  Tag dtag = pixelrow_tag (dest_row);
  gfloat * s = (gfloat*) pixelrow_data (src_row);
  guint8 * d = (guint8*) pixelrow_data (dest_row);

  if (tag_alpha (stag) == ALPHA_YES)
    {
      if (tag_alpha (dtag) == ALPHA_YES)
        while (w--)
          {
            d[0] = s[0] * 255;
            d[1] = s[1] * 255;
            s += 2;
            d += 2;
          }
      else
        while (w--)
          {
            d[0] = s[0] * 255;
            s += 2;
            d += 1;
          }
    }
  else
    {
      if (tag_alpha (dtag) == ALPHA_YES)
        while (w--)
          {
            d[0] = s[0] * 255;
            d[1] = 255;
            s += 1;
            d += 2;
          }
      else
        while (w--)
          {
            d[0] = s[0] * 255;
            s += 1;
            d += 1;
          }
    }
}

static void 
copy_row_gray_to_u16_rgb  (
                           PixelRow * src_row,
                           PixelRow * dest_row
                           )
{
  int w = MIN (pixelrow_width (src_row), pixelrow_width (dest_row));
  Tag stag = pixelrow_tag (src_row);
  Tag dtag = pixelrow_tag (dest_row);
  gfloat * s = (gfloat*) pixelrow_data (src_row);
  guint16 * d = (guint16*) pixelrow_data (dest_row);

  if (tag_alpha (stag) == ALPHA_YES)
    {
      if (tag_alpha (dtag) == ALPHA_YES)
        while (w--)
          {
            d[0] = d[1] = d[2] = s[0] * 65535;
            d[3] = s[1] * 65535;
            s += 2;
            d += 4;
          }
      else
        while (w--)
          {
            d[0] = d[1] = d[2] = s[0] * 65535;
            s += 2;
            d += 3;
          }
    }
  else
    {
      if (tag_alpha (dtag) == ALPHA_YES)
        while (w--)
          {
            d[0] = d[1] = d[2] = s[0] * 65535;
            d[3] = 65535;
            s += 1;
            d += 4;
          }
      else
        while (w--)
          {
            d[0] = d[1] = d[2] = s[0] * 65535;
            s += 1;
            d += 3;
          }
    }
}


static void
copy_row_gray_to_u16_gray (
                           PixelRow * src_row,
                           PixelRow * dest_row
                           )
{
  int w = MIN (pixelrow_width (src_row), pixelrow_width (dest_row));
  Tag stag = pixelrow_tag (src_row);
  Tag dtag = pixelrow_tag (dest_row);
  gfloat * s = (gfloat*) pixelrow_data (src_row);
  guint16 * d = (guint16*) pixelrow_data (dest_row);

  if (tag_alpha (stag) == ALPHA_YES)
    {
      if (tag_alpha (dtag) == ALPHA_YES)
        while (w--)
          {
            d[0] = s[0] * 65535;
            d[1] = s[1] * 65535;
            s += 2;
            d += 2;
          }
      else
        while (w--)
          {
            d[0] = s[0] * 65535;
            s += 2;
            d += 1;
          }
    }
  else
    {
      if (tag_alpha (dtag) == ALPHA_YES)
        while (w--)
          {
            d[0] = s[0] * 65535;
            d[1] = 65535;
            s += 1;
            d += 2;
          }
      else
        while (w--)
          {
            d[0] = s[0] * 65535;
            s += 1;
            d += 1;
          }
    }
}

static void 
copy_row_gray_to_float_rgb  (
                             PixelRow * src_row,
                             PixelRow * dest_row
                             )
{
  int w = MIN (pixelrow_width (src_row), pixelrow_width (dest_row));
  Tag stag = pixelrow_tag (src_row);
  Tag dtag = pixelrow_tag (dest_row);
  gfloat * s = (gfloat*) pixelrow_data (src_row);
  gfloat * d = (gfloat*) pixelrow_data (dest_row);

  if (tag_alpha (stag) == ALPHA_YES)
    {
      if (tag_alpha (dtag) == ALPHA_YES)
        while (w--)
          {
            d[0] = d[1] = d[2] = s[0];
            d[3] = s[1];
            s += 2;
            d += 4;
          }
      else
        while (w--)
          {
            d[0] = d[1] = d[2] = s[0];
            s += 2;
            d += 3;
          }
    }
  else
    {
      if (tag_alpha (dtag) == ALPHA_YES)
        while (w--)
          {
            d[0] = d[1] = d[2] = s[0];
            d[3] = 1.0;
            s += 1;
            d += 4;
          }
      else
        while (w--)
          {
            d[0] = d[1] = d[2] = s[0];
            s += 1;
            d += 3;
          }
    }
}


static void 
copy_row_gray_to_float_gray  (
                              PixelRow * src_row,
                              PixelRow * dest_row
                              )
{
  int w = MIN (pixelrow_width (src_row), pixelrow_width (dest_row));
  Tag stag = pixelrow_tag (src_row);
  Tag dtag = pixelrow_tag (dest_row);
  gfloat * s = (gfloat*) pixelrow_data (src_row);
  gfloat * d = (gfloat*) pixelrow_data (dest_row);

  if (tag_alpha (stag) == ALPHA_YES)
    {
      if (tag_alpha (dtag) == ALPHA_YES)
        while (w--)
          {
            d[0] = s[0];
            d[1] = s[1];
            s += 2;
            d += 2;
          }
      else
        while (w--)
          {
            d[0] = s[0];
            s += 2;
            d += 1;
          }
    }
  else
    {
      if (tag_alpha (dtag) == ALPHA_YES)
        while (w--)
          {
            d[0] = s[0];
            d[1] = 1.0;
            s += 1;
            d += 2;
          }
      else
        while (w--)
          {
            d[0] = s[0];
            s += 1;
            d += 1;
          }
    }
}


typedef void (*CopyRowFunc) (PixelRow *, PixelRow *);

static CopyRowFunc funcs[2][3][2] =
{
  {
    {
      copy_row_rgb_to_u8_rgb,
      copy_row_rgb_to_u8_gray
    },
    {
      copy_row_rgb_to_u16_rgb,
      copy_row_rgb_to_u16_gray
    },
    {
      copy_row_rgb_to_float_rgb,
      copy_row_rgb_to_float_gray
    }
  },
  
  {
    {
      copy_row_gray_to_u8_rgb,
      copy_row_gray_to_u8_gray
    },
    {
      copy_row_gray_to_u16_rgb,
      copy_row_gray_to_u16_gray
    },
    {
      copy_row_gray_to_float_rgb,
      copy_row_gray_to_float_gray
    }
  }
};


void
copy_row_float (
             PixelRow * src_row,
             PixelRow * dest_row
             )
{
  int x, y, z;
  
  switch (tag_format (pixelrow_tag (src_row)))
    {
    case FORMAT_RGB:      x = 0; break;
    case FORMAT_GRAY:     x = 1; break;
    case FORMAT_INDEXED:
    case FORMAT_NONE:
    default:
      g_warning ("unsupported src format in copy_row_float()");
      return;
    }

  switch (tag_precision (pixelrow_tag (dest_row)))
    {
    case PRECISION_U8:    y = 0; break;
    case PRECISION_U16:   y = 1; break;
    case PRECISION_FLOAT: y = 2; break;
    case PRECISION_NONE:
    default:
      g_warning ("unsupported dest precision in copy_row_float()");
      return;
    }
  
  switch (tag_format (pixelrow_tag (dest_row)))
    {
    case FORMAT_RGB:      z = 0; break;
    case FORMAT_GRAY:     z = 1; break;
    case FORMAT_INDEXED:
    case FORMAT_NONE:
    default:
      g_warning ("unsupported dest format in copy_row_float()");
      return;
    }

  funcs[x][y][z] (src_row, dest_row);
}

