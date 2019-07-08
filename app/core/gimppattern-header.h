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

#ifndef __GIMP_PATTERN_HEADER_H__
#define __GIMP_PATTERN_HEADER_H__


#define GIMP_PATTERN_MAGIC    (('G' << 24) + ('P' << 16) + \
                               ('A' << 8)  + ('T' << 0))
#define GIMP_PATTERN_MAX_SIZE 10000 /* Max size in either dimension in px */
#define GIMP_PATTERN_MAX_NAME 256   /* Max length of the pattern's name   */


/*  All field entries are MSB  */

typedef struct _GimpPatternHeader GimpPatternHeader;

struct _GimpPatternHeader
{
  guint32   header_size;  /*  = sizeof (GimpPatternHeader) + pattern name  */
  guint32   version;      /*  pattern file version #  */
  guint32   width;        /*  width of pattern  */
  guint32   height;       /*  height of pattern  */
  guint32   bytes;        /*  depth of pattern in bytes  */
  guint32   magic_number; /*  GIMP pattern magic number  */
};

/*  In a pattern file, next comes the pattern name, null-terminated.
 *  After that comes the pattern data -- width * height * bytes bytes
 *  of it...
 */


#endif  /* __GIMP_PATTERN_HEADER_H__ */
