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


/* the largest possible value that can come from tag_bytes(). */
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

guint     tag_num_channels      (Tag);
guint     tag_bytes             (Tag);
guint     tag_equal             (Tag, Tag);

guchar *  tag_string_precision  (Precision);
guchar *  tag_string_format     (Format);
guchar *  tag_string_alpha      (Alpha);

Tag       tag_null              (void);
gint      tag_valid             (Tag);



/* Hopefully temp "glue" for libgimp  */ 
gint      tag_to_drawable_type   (Tag);
gint      tag_to_image_type      (Tag);
Tag       tag_from_drawable_type (gint);
Tag       tag_from_image_type    (gint);

/* the image types */
#define RGB_GIMAGE          0
#define RGBA_GIMAGE         1
#define GRAY_GIMAGE         2
#define GRAYA_GIMAGE        3
#define INDEXED_GIMAGE      4
#define INDEXEDA_GIMAGE     5

/* These are the 16bit types */
#define U16_RGB_GIMAGE         6
#define U16_RGBA_GIMAGE        7
#define U16_GRAY_GIMAGE        8
#define U16_GRAYA_GIMAGE       9
#define U16_INDEXED_GIMAGE     10 
#define U16_INDEXEDA_GIMAGE    11 

/* These are the float types */
#define FLOAT_RGB_GIMAGE          12 
#define FLOAT_RGBA_GIMAGE         13 
#define FLOAT_GRAY_GIMAGE         14 
#define FLOAT_GRAYA_GIMAGE        15 

/* base types */
#define RGB              0
#define GRAY             1
#define INDEXED          2
#define U16_RGB     	 3    /*16bit*/
#define U16_GRAY    	 4
#define U16_INDEXED		 5
#define FLOAT_RGB 		 6    /*float*/
#define FLOAT_GRAY 		 7


#endif /* __TAG_H__ */
