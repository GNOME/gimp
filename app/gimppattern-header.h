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
#ifndef __PATTERN_HEADER_H__
#define __PATTERN_HEADER_H__

typedef struct _PatternHeader PatternHeader;

#define FILE_VERSION   2
#define GPATTERN_MAGIC   (('G' << 24) + ('P' << 16) + ('A' << 8) + ('T' << 0))
#define sz_PatternHeader (sizeof (PatternHeader))

/*  All field entries are MSB  */

struct _PatternHeader
{
  unsigned int   header_size;  /*  header_size = sz_PatternHeader + pattern name  */
  unsigned int   version;      /*  pattern file version #  */
  unsigned int   width;        /*  width of pattern  */
  unsigned int   height;       /*  height of pattern  */
  unsigned int   type;        /*  = bpp in version 1,  but layer type in version >=2 */
  unsigned int   magic_number; /*  GIMP pattern magic number  */
};

/*  In a pattern file, next comes the pattern name, null-terminated.  After that
 *  comes the pattern data--width * height * bytes bytes of it...
 */

#endif  /*  __PATTERN_HEADER_H__  */
