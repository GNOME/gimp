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
#ifndef __TAG_H__
#define __TAG_H__

#include "glib.h"


typedef guint Tag;


/* the largest possible value that can come from tag_bytes().  used to
   statically size some other data structures (eg: Paint) */
#define TAG_MAX_BYTES 16

/* supported precisions */
typedef enum _Precision Precision;
enum _Precision
{  
  PRECISION_NONE,
  PRECISION_U8,
  PRECISION_U16,
  PRECISION_FLOAT
};


/* the supported formats */
typedef enum _Format Format;
enum _Format
{
  FORMAT_NONE,
  FORMAT_RGB,
  FORMAT_GRAY,
  FORMAT_INDEXED
};


/* does the data have an embedded alpha channel */
typedef enum _Alpha Alpha;
enum _Alpha
{
  ALPHA_NONE,
  ALPHA_NO,
  ALPHA_YES
};


Tag       tag_new               (Precision, Format, Alpha);

Precision tag_precision         (Tag);
Format    tag_format            (Tag);
Alpha     tag_alpha             (Tag);

Tag       tag_set_precision     (Tag, Precision);
Tag       tag_set_format        (Tag, Format);
Tag       tag_set_alpha         (Tag, Alpha);

guint     tag_bytes             (Tag);
guint     tag_equal             (Tag, Tag);
guint     tag_num_channels      (Tag);
guchar *  tag_string_precision  (Precision);
guchar *  tag_string_format     (Format);
guchar *  tag_string_alpha      (Alpha);

Tag       tag_null              (void);


/* hack hack */
Tag       tag_by_bytes          (guint);


#endif /* __TAG_H__ */
