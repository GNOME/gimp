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

#ifndef __GIMP_PATTERN_HEADER_H__
#define __GIMP_PATTERN_HEADER_H__

#define GPATTERN_FILE_VERSION    1
#define GPATTERN_MAGIC    (('G' << 24) + ('P' << 16) + ('A' << 8) + ('T' << 0))

/*  All field entries are MSB  */

typedef struct _PatternHeader PatternHeader;

struct _PatternHeader
{
  guint32   header_size;  /*  header_size = sizeof(PatternHeader) + pattern name  */
  guint32   version;      /*  pattern file version #  */
  guint32   width;        /*  width of pattern  */
  guint32   height;       /*  height of pattern  */
  guint32   bytes;        /*  depth of pattern in bytes  */
  guint32   magic_number; /*  GIMP pattern magic number  */
};

/*  In a pattern file, next comes the pattern name, null-terminated.  After that
 *  comes the pattern data--width * height * bytes bytes of it...
 */

#endif  /*  __GIMP_PATTERN_HEADER_H__  */
