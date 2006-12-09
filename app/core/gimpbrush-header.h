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

#ifndef __GIMP_BRUSH_HEADER_H__
#define __GIMP_BRUSH_HEADER_H__

#define GBRUSH_FILE_VERSION    2
#define GBRUSH_MAGIC    (('G' << 24) + ('I' << 16) + ('M' << 8) + ('P' << 0))

/*  All field entries are MSB  */

typedef struct _BrushHeader BrushHeader;

struct _BrushHeader
{
  guint32   header_size;  /*  header_size = sizeof (BrushHeader) + brush name  */
  guint32   version;      /*  brush file version #  */
  guint32   width;        /*  width of brush  */
  guint32   height;       /*  height of brush  */
  guint32   bytes;        /*  depth of brush in bytes--always 1 */
  guint32   magic_number; /*  GIMP brush magic number  */
  guint32   spacing;      /*  brush spacing  */
};

/*  In a brush file, next comes the brush name, null-terminated.  After that
 *  comes the brush data--width * height * bytes bytes of it...
 */

#endif  /*  __GIMP_BRUSH_HEADER_H__  */
