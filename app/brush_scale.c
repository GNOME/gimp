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
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include <glib.h>
#include "brush_scale.h"

MaskBuf *
brush_scale_mask (MaskBuf *brush_mask,
		  int      dest_width, 
		  int      dest_height)
{
  MaskBuf *scale_brush;
  int src_width;
  int src_height;
  int value;
  int area;
  int i, j;
  int x, x0, y, y0;
  int dx, dx0, dy, dy0;
  int fx, fx0, fy, fy0;
  unsigned char *src, *dest;

  g_return_val_if_fail (brush_mask != NULL &&
			dest_width != 0 && dest_height != 0, NULL);

  src_width  = brush_mask->width;
  src_height = brush_mask->height;

  scale_brush = mask_buf_new (dest_width, dest_height);
  g_return_val_if_fail (scale_brush != NULL, NULL);

  /*  get the data  */
  dest = mask_buf_data (scale_brush);
  src  = mask_buf_data (brush_mask);

  fx = fx0 = (256.0 * src_width) / dest_width;
  fy = fy0 = (256.0 * src_height) / dest_height;
  area = fx0 * fy0;

  x = x0 = 0;
  y = y0 = 0;
  dx = dx0 = 0;
  dy = dy0 = 0;

  for (i=0; i<dest_height; i++)
    {
      for (j=0; j<dest_width; j++)
	{
	  value  = 0;

	  fy = fy0;
	  y  = y0;
	  dy = dy0;
	   
	  if (dy)
	    {
	      fx = fx0;
	      x  = x0;
	      dx = dx0;
	      
	      if (dx)
		{
		  value += dx * dy * src[x + src_width * y]; 
		  x++;
		  fx -= dx;
		  dx = 0;
		}
	      while (fx >= 256)
		{
		  value += 256 * dy * src[x + src_width * y];  
		  x++;
		  fx -= 256;
		}
	      if (fx)
		{
		  value += fx * dy * src[x + src_width * y];
		  dx = 256 - fx;
		}	      
	      y++;
	      fy -= dy;
	      dy = 0;
	    }  
	  
	  while (fy >= 256)
	    {
	      fx = fx0;
	      x  = x0;
	      dx = dx0;
	      
	      if (dx)
		{
		  value += dx * 256 * src[x + src_width * y]; 
		  x++;
		  fx -= dx;
		  dx = 0;
		}
	      while (fx >= 256)
		{
		  value += 256 * 256 * src[x + src_width * y];  
		  x++;
		  fx -= 256;
		}
	      if (fx)
		{
		  value += fx * 256 * src[x + src_width * y];
		  dx = 256 - fx;
		}
	      y++;
	      fy -= 256;
	    }
	  
	  if (fy)
	    {
	      fx = fx0;
	      x  = x0;
	      dx = dx0;
	      
	      if (dx)
		{
		  value += dx * fy * src[x + src_width * y]; 
		  x++;
		  fx -= dx;
		  dx = 0;
		}
	      while (fx >= 256)
		{
		  value += 256 * fy * src[x + src_width * y];  
		  x++;
		  fx -= 256;
		}
	      if (fx)
		{
		  value += fx * fy * src[x + src_width * y];
		  dx = 256 - fx;
		}
	      dy = 256 - fy;
	    }  	  
	  
	  *dest++ = MIN ((value / area), 255);
	
	  x0  = x;
	  dx0 = dx;
	}
      x0  = 0;
      dx0 = 0;
      y0  = y;
      dy0 = dy;
    }

  return scale_brush;
}
