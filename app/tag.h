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


/* fields within a Tag */
#define MASK_PRECISION  0xff
#define SHIFT_PRECISION 24

#define MASK_FORMAT     0xff
#define SHIFT_FORMAT    16

#define MASK_ALPHA      0xff
#define SHIFT_ALPHA     8


/* supported precisions */
typedef enum
{  
  PRECISION_NONE,
  PRECISION_U8,
  PRECISION_U16,
  PRECISION_FLOAT
} Precision;


/* the supported formats */
typedef enum
{
  FORMAT_NONE,
  FORMAT_RGB,
  FORMAT_GRAY,
  FORMAT_INDEXED
} Format;


/* does the data have an embedded alpha channel */
typedef enum
{
  ALPHA_NONE,
  ALPHA_NO,
  ALPHA_YES
} Alpha;

/* if you define DEBUG_TAGS, then actual functions which check
   inputs will be used.  otherwise, the macros below will be used */

#ifdef DEBUG_TAGS

Tag       tag_new               (Precision, Format, Alpha);
Tag       tag_null              (void);
Precision tag_precision         (Tag);
Format    tag_format            (Tag);
Alpha     tag_alpha             (Tag);
Tag       tag_set_precision     (Tag, Precision);
Tag       tag_set_format        (Tag, Format);
Tag       tag_set_alpha         (Tag, Alpha);

#else

#define tag_new(p,f,a) \
        ((((p) & MASK_PRECISION) << SHIFT_PRECISION) | \
         (((f) & MASK_FORMAT) << SHIFT_FORMAT) | \
         (((a) & MASK_ALPHA) << SHIFT_ALPHA))

#define tag_null() \
        tag_new (PRECISION_NONE, FORMAT_NONE, ALPHA_NONE)

#define tag_precision(t) \
        (((t) >> SHIFT_PRECISION) & MASK_PRECISION)

#define tag_format(t) \
        (((t) >> SHIFT_FORMAT) & MASK_FORMAT)

#define tag_alpha(t) \
        (((t) >> SHIFT_ALPHA) & MASK_ALPHA)

#define tag_set_precision(t,p) \
        (((t) & ~(MASK_PRECISION << SHIFT_PRECISION)) | \
         (((p) &  MASK_PRECISION) << SHIFT_PRECISION))

#define tag_set_format(t,f) \
        (((t) & ~(MASK_FORMAT << SHIFT_FORMAT)) | \
         (((f) &  MASK_FORMAT) << SHIFT_FORMAT))

#define tag_set_alpha(t,a) \
        (((t) & ~(MASK_ALPHA << SHIFT_ALPHA)) | \
         (((a) &  MASK_ALPHA) << SHIFT_ALPHA))
#endif

guint     tag_num_channels      (Tag);
guint     tag_bytes             (Tag);
guint     tag_equal             (Tag, Tag);
gint      tag_valid             (Tag);
gchar *   tag_string_precision  (Precision);
gchar *   tag_string_format     (Format);
gchar *   tag_string_alpha      (Alpha);



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
