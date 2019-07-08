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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef __GIMP_BRUSH_HEADER_H__
#define __GIMP_BRUSH_HEADER_H__


#define GIMP_BRUSH_MAGIC    (('G' << 24) + ('I' << 16) + \
                             ('M' << 8)  + ('P' << 0))
#define GIMP_BRUSH_MAX_SIZE 10000 /* Max size in either dimension in px */
#define GIMP_BRUSH_MAX_NAME 256   /* Max length of the brush's name     */


/*  All field entries are MSB  */

typedef struct _GimpBrushHeader GimpBrushHeader;

struct _GimpBrushHeader
{
  guint32   header_size;  /*  = sizeof (GimpBrushHeader) + brush name  */
  guint32   version;      /*  brush file version #  */
  guint32   width;        /*  width of brush  */
  guint32   height;       /*  height of brush  */
  guint32   bytes;        /*  depth of brush in bytes  */
  guint32   magic_number; /*  GIMP brush magic number  */
  guint32   spacing;      /*  brush spacing  */
};

/*  In a brush file, next comes the brush name, null-terminated.
 *  After that comes the brush data -- width * height * bytes bytes of
 *  it...
 */


#endif /* __GIMP_BRUSH_HEADER_H__ */
