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

/* define this so these routines get compiled */
#define DEBUG_PIXELROW

#include "pixelrow.h"

void 
pixelrow_init  (
                PixelRow * p,
                Tag tag,
                guchar * buffer,
                int width
                )
{
  g_return_if_fail (p != NULL);
  p->tag = tag;
  p->buffer = buffer;
  p->width = width;
}

Tag 
pixelrow_tag  (
               PixelRow * p
               )
{
  g_return_val_if_fail (p != NULL, tag_null());
  return p->tag;
}


int 
pixelrow_width  (
                 PixelRow * p
                 )
{
  g_return_val_if_fail (p != NULL, 0);
  return p->width;
}


guchar * 
pixelrow_data  (
                PixelRow * p
                )
{
  g_return_val_if_fail (p != NULL, 0);
  return p->buffer;
}



