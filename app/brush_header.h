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
#ifndef __BRUSH_HEADER_H__
#define __BRUSH_HEADER_H__

typedef struct _BrushHeader BrushHeader;

#define FILE_VERSION   3
#define GBRUSH_MAGIC   (('G' << 24) + ('I' << 16) + ('M' << 8) + ('P' << 0))
#define sz_BrushHeader (sizeof (BrushHeader))

#define BRUSH_HEADER_C_2_cw
/*  All field entries are MSB  */

struct _BrushHeader
{
  unsigned int   header_size; /*  header_size = sz_BrushHeader + brush name  */
  unsigned int   version;     /*  brush file version #  */
  unsigned int   width;       /*  width of brush  */
  unsigned int   height;      /*  height of brush  */
  unsigned int   type;        /*  = bpp in  version <= 2, but layer type in  version >= 3*/
  unsigned int   magic_number;/*  GIMP brush magic number  */
  unsigned int   spacing;     /*  brush spacing  */
};

/*  In a brush file, next comes the brush name, null-terminated.  After that
 *  comes the brush data--width * height * bytes bytes of it...
 */

#endif  /*  __BRUSH_HEADER_H__  */
