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


#include "paint.h"
#include "pixelrow.h"


void 
pixelrow_init  (
                PixelRow * p,
                Tag tag,
                guchar * buffer,
                int width
                )
{
  if (p)
    {
      p->tag = tag;
      p->buffer = buffer;
      p->width = width;

      p->bytes = tag_bytes (tag);
    }
}


guchar * 
pixelrow_getdata  (
                   PixelRow * p,
                   int x
                   )
{
  if (p && (x < p->width))
    return (p->buffer + (x * p->bytes));
  return NULL;
}


Tag 
pixelrow_tag  (
               PixelRow * p
               )
{
  if (p)
    return p->tag;
  return tag_null ();
}


int 
pixelrow_width  (
                 PixelRow * p
                 )
{
  if (p)
    return p->width;
  return 0;
}


guchar * 
pixelrow_data  (
                PixelRow * p
                )
{
  if (p)
    return p->buffer;
  return NULL;
}



