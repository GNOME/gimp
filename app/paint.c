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

#include <string.h>

#include "paint.h"

/* cmap and num_cols should be removed from GImage and put into a
   distinct ColorMap */
#include "drawable.h"
#include "gimage.h"


struct _Paint
{
  Tag            tag;
  char           data[TAG_MAX_BYTES];
  GimpDrawable * drawable;
};



/*
  Paint Initializers: routines to load a Paint from various sorts of
  buffers.  called from paint_load().

  the idea is that you take raw data and stuff it into a Paint as soon
  as possible.  then you pass the Paint around, and it is hopefully
  smart enough to adapt itself to whatever format the eventual end
  user requires

  it would be good to have a paint_dump() family as well.  you could
  then do arbitrary format conversions by loading each pixel in one
  format, converting format/precision/whatever inside the Paint, and
  dumping in the new format.

*/

#define INTENSITY(r,g,b) (r * 0.30 + g * 0.59 + b * 0.11 + 0.001)

static int load_u8            (Paint *, Tag, guint8 *);
static int load_u16           (Paint *, Tag, guint16 *);

#if 0
static int load_u8_rgb        (Paint *, Tag, guint8 *);
static int load_u8_gray       (Paint *, Tag, guint8 *);
static int load_u8_indexed    (Paint *, Tag, guint8 *);

static int load_u16_rgb       (Paint *, Tag, guint16 *);
static int load_u16_gray      (Paint *, Tag, guint16 *);
static int load_u16_indexed   (Paint *, Tag, guint16 *);
#endif

static int load_float         (Paint *, Tag, gfloat *);
static int load_float_rgb     (Paint *, Tag, gfloat *);
static int load_float_gray    (Paint *, Tag, gfloat *);
static int load_float_indexed (Paint *, Tag, gfloat *);




Paint *
paint_new (
           Tag tag,
           GimpDrawable * drawable
           )
{
  Paint * new_p;
    
  if (tag_bytes (tag) == 0)
    return NULL;

  new_p = g_malloc (sizeof (Paint));
  memset (new_p, 0, sizeof (Paint));
  new_p->tag = tag;
  new_p->drawable = drawable;

  return new_p;
}


void
paint_delete (
              Paint * p
              )
{
  if (p)
    g_free (p);
}


Paint *
paint_clone (
             Paint * p
             )
{
  Paint *new_p = NULL;

  if (p)
    {
      new_p = g_malloc (sizeof (Paint));
      *new_p = *p;
    }
  
  return new_p;
}


void
paint_info (
            Paint * p
            )
{
  if (p)
    {
      trace_begin ("Paint 0x%x", p);
      trace_printf ("%s %s %s",
                    tag_string_precision (tag_precision (paint_tag (p))),
                    tag_string_format (tag_format (paint_tag (p))),
                    tag_string_alpha (tag_alpha (paint_tag (p))));
      trace_printf ("%x%x%x%x%x%x%x%x%x%x%x%x%x%x%x%x",
                    p->data[0],
                    p->data[1],
                    p->data[2],
                    p->data[3],
                    p->data[4],
                    p->data[5],
                    p->data[6],
                    p->data[7],
                    p->data[8],
                    p->data[9],
                    p->data[10],
                    p->data[11],
                    p->data[12],
                    p->data[13],
                    p->data[14],
                    p->data[15]);
      trace_end ();
    }
}


Tag
paint_tag (
           Paint * p
           )
{
  if (p)
    return p->tag;
  return tag_null ();
}


Precision
paint_precision (
                 Paint * p
                 )
{
  return tag_precision (paint_tag (p));
}


Format
paint_format (
              Paint * p
              )
{
  return tag_format (paint_tag (p));
}


Alpha
paint_alpha (
             Paint * p
             )
{
  return tag_alpha (paint_tag (p));
}


Precision
paint_set_precision (
                     Paint *    p,
                     Precision  precision
                     )
{
  g_warning ("finish writing paint_set_precision()");
  return tag_precision (paint_tag (p));
}


Format
paint_set_format (
                  Paint * p,
                  Format  format
                  )
{
  g_warning ("finish writing paint_set_format()");
  return tag_format (paint_tag (p));
}


Alpha
paint_set_alpha (
                 Paint * p,
                 Alpha   alpha
                 )
{
  g_warning ("finish writing paint_set_alpha()");
  return tag_alpha (paint_tag (p));
}


void *
paint_component (
                 Paint *  p,
                 guint    component
                 )
{
  g_warning ("finish writing paint_component()");
  return NULL;
}


guint
paint_set_component (
                     Paint *   p,
                     guint     component,
                     void *    value
                     )
{
  g_warning ("finish writing paint_set_component()");
  return FALSE;
}


int
paint_load (
            Paint *  paint,
            Tag      tag,
            void *   data
            )
{
  switch (tag_precision (tag))
    {
    case PRECISION_U8:
      return load_u8 (paint, tag, (guint8 *) data);
    case PRECISION_U16:
      return load_u16 (paint, tag, (guint16 *) data);
    case PRECISION_FLOAT:
      return load_float (paint, tag, (gfloat *) data);      
    case PRECISION_NONE:
      break;
    }
  return FALSE;
}


GimpDrawable *
paint_drawable (
                Paint * p
                )
{
  if (p)
    return p->drawable;
  return NULL;
}


GimpDrawable *
paint_set_drawable (
                    Paint * p,
                    GimpDrawable * d
                    )
{
  if (p)
    return (p->drawable = d);
  return NULL;
}


void *
paint_data (
            Paint * p
            )
{
  if (p)
    return &p->data;
  return NULL;
}


guint
paint_bytes (
             Paint * p
             )
{
  return tag_bytes (paint_tag (p));
}









/* paint initializers */

static int
load_u8 (
         Paint *  paint,
         Tag      tag,
         guint8 * data
         )
{
  g_warning ("finish writing load_u8()");
  return FALSE;
}
     

static int
load_u16 (
          Paint *   paint,
          Tag       tag,
          guint16 * data
          )
{
  g_warning ("finish writing load_u16()");
  return FALSE;
}
     

static int
load_float (
            Paint *  paint,
            Tag      tag,
            gfloat * data
            )
{
  switch (tag_format (tag))
    {
    case FORMAT_RGB:
      return load_float_rgb (paint, tag, data);
    case FORMAT_GRAY:
      return load_float_gray (paint, tag, data);
    case FORMAT_INDEXED:
      return load_float_indexed (paint, tag, data);
    case FORMAT_NONE:
      break;
    }
  return FALSE;
}


static int
load_float_rgb (
                Paint *  paint,
                Tag      tag,
                gfloat * data
                )
{
  switch (paint_precision (paint))
    {
    case PRECISION_U8:  /*----------------------------------------*/
      {
        guint8 *d = (guint8 *) paint_data (paint);
        guint8  a;

        if (tag_alpha (tag) == ALPHA_YES)
            a = data[3] * 255;
        else
            a = 255;
        
        switch (paint_format (paint))
          {
          case FORMAT_RGB:
            d[0] = data[0] * 255;
            d[1] = data[1] * 255;
            d[2] = data[2] * 255;
            if (paint_alpha (paint) == ALPHA_YES)
              d[3] = a;
            break;
            
          case FORMAT_GRAY:
            d[0] = INTENSITY (data[0] * 255,
                              data[1] * 255,
                              data[2] * 255);
            if (paint_alpha (paint) == ALPHA_YES)
              d[1] = a;
            break;

          case FORMAT_INDEXED:
            {
              GImage *g;
              if ((g = drawable_gimage (paint_drawable (paint))) == NULL)
                return FALSE;
              d[0] = map_rgb_to_indexed (g->cmap,
                                         g->num_cols,
                                         g->ID,
                                         (int) (data[0] * 255),
                                         (int) (data[1] * 255),
                                         (int) (data[2] * 255));
              if (paint_alpha (paint) == ALPHA_YES)
                d[1] = a;
            }
            break;
          default:
            return FALSE;        
          }
      }
      break;

      
    case PRECISION_U16:    /*----------------------------------------*/
      {
        guint16 *d = (guint16 *) paint_data (paint);
        switch (paint_format (paint))
          {
          case FORMAT_RGB:
            d[0] = data[0] * 65535;
            d[1] = data[1] * 65535;
            d[2] = data[2] * 65535;
            if (paint_alpha (paint) == ALPHA_YES)
              d[3] = 65535;
            break;
            
          case FORMAT_GRAY:
            d[0] = INTENSITY (data[0] * 65535,
                              data[1] * 65535,
                              data[2] * 65535);
            if (paint_alpha (paint) == ALPHA_YES)
              d[1] = 65535;
            break;

          case FORMAT_INDEXED:
            g_warning ("finish writing load_float_rgb() u16 indexed");
          default:
            return FALSE;        
          }

      }
      break;

      
    case PRECISION_FLOAT: /*----------------------------------------*/
      {
        gfloat *d = (gfloat *) paint_data (paint);

        switch (paint_format (paint))
          {
          case FORMAT_RGB:
            d[0] = data[0];
            d[1] = data[1];
            d[2] = data[2];
            if (paint_alpha (paint) == ALPHA_YES)
              d[3] = 1;
            break;
            
          case FORMAT_GRAY:
            d[0] = INTENSITY (data[0],
                              data[1],
                              data[2]);
            if (paint_alpha (paint) == ALPHA_YES)
              d[1] = 1;
            break;

          case FORMAT_INDEXED:
            g_warning ("finish writing load_float_rgb() float indexed");
          default:
            return FALSE;        
          }
      }

    default:
      return FALSE;
    }
  return TRUE;
}
     

static int
load_float_gray (
                 Paint *  paint,
                 Tag      tag,
                 gfloat * data
                 )
{
  switch (paint_precision (paint))
    {
    case PRECISION_U8:  /*----------------------------------------*/
      {
        guint8 *d = (guint8 *) paint_data (paint);
        guint8  a;

        if (tag_alpha (tag) == ALPHA_YES)
            a = data[1] * 255;
        else
            a = 255;
        
        switch (paint_format (paint))
          {
          case FORMAT_RGB:
            d[0] = data[0] * 255;
            d[1] = data[0] * 255;
            d[2] = data[0] * 255;
            if (paint_alpha (paint) == ALPHA_YES)
              d[3] = a;
            break;
            
          case FORMAT_GRAY:
            d[0] = data[0] * 255;
            if (paint_alpha (paint) == ALPHA_YES)
              d[1] = a;
            break;

          case FORMAT_INDEXED:
            {
              GImage *g;
              if ((g = drawable_gimage (paint_drawable (paint))) == NULL)
                return FALSE;
              d[0] = map_rgb_to_indexed (g->cmap,
                                         g->num_cols,
                                         g->ID,
                                         (int) (data[0] * 255),
                                         (int) (data[0] * 255),
                                         (int) (data[0] * 255));
              if (paint_alpha (paint) == ALPHA_YES)
                d[1] = a;
            }
            break;
          default:
            return FALSE;        
          }
      }
      break;

      
    case PRECISION_U16:    /*----------------------------------------*/
      {
        guint16 *d = (guint16 *) paint_data (paint);
        switch (paint_format (paint))
          {
          case FORMAT_RGB:
            d[0] = data[0] * 65535;
            d[1] = data[0] * 65535;
            d[2] = data[0] * 65535;
            if (paint_alpha (paint) == ALPHA_YES)
              d[3] = 65535;
            break;
            
          case FORMAT_GRAY:
            d[0] = data[0] * 65535;
            if (paint_alpha (paint) == ALPHA_YES)
              d[1] = 65535;
            break;

          case FORMAT_INDEXED:
            g_warning ("finish writing load_float_rgb() u16 indexed");
          default:
            return FALSE;        
          }

      }
      break;

      
    case PRECISION_FLOAT: /*----------------------------------------*/
      {
        gfloat *d = (gfloat *) paint_data (paint);

        switch (paint_format (paint))
          {
          case FORMAT_RGB:
            d[0] = data[0];
            d[1] = data[0];
            d[2] = data[0];
            if (paint_alpha (paint) == ALPHA_YES)
              d[3] = 1;
            break;
            
          case FORMAT_GRAY:
            d[0] = data[0];
            if (paint_alpha (paint) == ALPHA_YES)
              d[1] = 1;
            break;

          case FORMAT_INDEXED:
            g_warning ("finish writing load_float_rgb() float indexed");
          default:
            return FALSE;        
          }
      }

    default:
      return FALSE;
    }
  return TRUE;
}


static int
load_float_indexed (
                    Paint *  paint,
                    Tag      tag,
                    gfloat * data
                    )
{
  g_warning ("finish writing load_float_indexed()");
  return FALSE;
}



/*********************************
 *   color conversion routines   *
 *********************************/

void
rgb_to_hsv (
            int *r,
	    int *g,
	    int *b
            )
{
  int red, green, blue;
  float h, s, v;
  int min, max;
  int delta;

  h = 0.0;

  red = *r;
  green = *g;
  blue = *b;

  if (red > green)
    {
      if (red > blue)
	max = red;
      else
	max = blue;

      if (green < blue)
	min = green;
      else
	min = blue;
    }
  else
    {
      if (green > blue)
	max = green;
      else
	max = blue;

      if (red < blue)
	min = red;
      else
	min = blue;
    }

  v = max;

  if (max != 0)
    s = ((max - min) * 255) / (float) max;
  else
    s = 0;

  if (s == 0)
    h = 0;
  else
    {
      delta = max - min;
      if (red == max)
	h = (green - blue) / (float) delta;
      else if (green == max)
	h = 2 + (blue - red) / (float) delta;
      else if (blue == max)
	h = 4 + (red - green) / (float) delta;
      h *= 42.5;

      if (h < 0)
	h += 255;
      if (h > 255)
	h -= 255;
    }

  *r = h;
  *g = s;
  *b = v;
}


void
hsv_to_rgb (
            int *h,
	    int *s,
	    int *v
            )
{
  float hue, saturation, value;
  float f, p, q, t;

  if (*s == 0)
    {
      *h = *v;
      *s = *v;
      *v = *v;
    }
  else
    {
      hue = *h * 6.0 / 255.0;
      saturation = *s / 255.0;
      value = *v / 255.0;

      f = hue - (int) hue;
      p = value * (1.0 - saturation);
      q = value * (1.0 - (saturation * f));
      t = value * (1.0 - (saturation * (1.0 - f)));

      switch ((int) hue)
	{
	case 0:
	  *h = value * 255;
	  *s = t * 255;
	  *v = p * 255;
	  break;
	case 1:
	  *h = q * 255;
	  *s = value * 255;
	  *v = p * 255;
	  break;
	case 2:
	  *h = p * 255;
	  *s = value * 255;
	  *v = t * 255;
	  break;
	case 3:
	  *h = p * 255;
	  *s = q * 255;
	  *v = value * 255;
	  break;
	case 4:
	  *h = t * 255;
	  *s = p * 255;
	  *v = value * 255;
	  break;
	case 5:
	  *h = value * 255;
	  *s = p * 255;
	  *v = q * 255;
	  break;
	}
    }
}


void
rgb_to_hls (
            int *r,
	    int *g,
	    int *b
            )
{
  int red, green, blue;
  float h, l, s;
  int min, max;
  int delta;

  red = *r;
  green = *g;
  blue = *b;

  if (red > green)
    {
      if (red > blue)
	max = red;
      else
	max = blue;

      if (green < blue)
	min = green;
      else
	min = blue;
    }
  else
    {
      if (green > blue)
	max = green;
      else
	max = blue;

      if (red < blue)
	min = red;
      else
	min = blue;
    }

  l = (max + min) / 2.0;

  if (max == min)
    {
      s = 0.0;
      h = 0.0;
    }
  else
    {
      delta = (max - min);

      if (l < 128)
	s = 255 * (float) delta / (float) (max + min);
      else
	s = 255 * (float) delta / (float) (511 - max - min);

      if (red == max)
	h = (green - blue) / (float) delta;
      else if (green == max)
	h = 2 + (blue - red) / (float) delta;
      else
	h = 4 + (red - green) / (float) delta;

      h = h * 42.5;

      if (h < 0)
	h += 255;
      if (h > 255)
	h -= 255;
    }

  *r = h;
  *g = l;
  *b = s;
}


static int
hls_value (
           float n1,
	   float n2,
	   float hue
           )
{
  float value;

  if (hue > 255)
    hue -= 255;
  else if (hue < 0)
    hue += 255;
  if (hue < 42.5)
    value = n1 + (n2 - n1) * (hue / 42.5);
  else if (hue < 127.5)
    value = n2;
  else if (hue < 170)
    value = n1 + (n2 - n1) * ((170 - hue) / 42.5);
  else
    value = n1;

  return (int) (value * 255);
}


void
hls_to_rgb (
            int *h,
	    int *l,
	    int *s
            )
{
  float hue, lightness, saturation;
  float m1, m2;

  hue = *h;
  lightness = *l;
  saturation = *s;

  if (saturation == 0)
    {
      /*  achromatic case  */
      *h = lightness;
      *l = lightness;
      *s = lightness;
    }
  else
    {
      if (lightness < 128)
	m2 = (lightness * (255 + saturation)) / 65025.0;
      else
	m2 = (lightness + saturation - (lightness * saturation)/255.0) / 255.0;

      m1 = (lightness / 127.5) - m2;

      /*  chromatic case  */
      *h = hls_value (m1, m2, hue + 85);
      *l = hls_value (m1, m2, hue);
      *s = hls_value (m1, m2, hue - 85);
    }
}


void
map_to_color (
              Format          src_type,
	      unsigned char * cmap,
	      unsigned char * src,
	      unsigned char * rgb
              )
{
  switch (src_type)
    {
    case FORMAT_RGB:
      *rgb++ = *src++;
      *rgb++ = *src++;
      *rgb   = *src;
      break;
    case FORMAT_GRAY:
      *rgb++ = *src;
      *rgb++ = *src;
      *rgb   = *src;
      break;
    case FORMAT_INDEXED:
      {
	int index = *src * 3;
	*rgb++ = cmap [index++];
	*rgb++ = cmap [index++];
	*rgb   = cmap [index++];
      }
      break;
    case FORMAT_NONE:
      break;
    }
}

/*  ColorHash structure  */
typedef struct _ColorHash ColorHash;

struct _ColorHash
{
  int pixel;           /*  R << 16 | G << 8 | B  */
  int index;           /*  colormap index        */
  int colormap_ID;     /*  colormap ID           */
};

#define HASH_TABLE_SIZE    1021
#define MAXDIFF            195076

static ColorHash color_hash_table [HASH_TABLE_SIZE];
static int color_hash_misses;
static int color_hash_hits;


void
paint_setup (
             void
             )
{
  int i;
  
  for (i = 0; i < HASH_TABLE_SIZE; i++)
    color_hash_table[i].colormap_ID = -1;
  color_hash_misses = 0;
  color_hash_hits = 0;
}


void
paint_free (
            void
            )
{
  /*  print out the hash table statistics
      printf ("RGB->indexed hash table lookups: %d\n", color_hash_hits + color_hash_misses);
      printf ("RGB->indexed hash table hits: %d\n", color_hash_hits);
      printf ("RGB->indexed hash table misses: %d\n", color_hash_misses);
      printf ("RGB->indexed hash table hit rate: %f\n",
      100.0 * color_hash_hits / (color_hash_hits + color_hash_misses));
      */
}


int
map_rgb_to_indexed (
                    unsigned char *cmap,
		    int            num_cols,
		    int            ID,
		    int            r,
		    int            g,
		    int            b
                    )
{
  unsigned int pixel;
  int hash_index;
  int cmap_index;

  pixel = (r << 16) | (g << 8) | b;
  hash_index = pixel % HASH_TABLE_SIZE;

  /*  Hash table lookup hit  */
  if (color_hash_table[hash_index].colormap_ID == ID &&
      color_hash_table[hash_index].pixel == pixel)
    {
      cmap_index = color_hash_table[hash_index].index;
      color_hash_hits++;
    }
  /*  Hash table lookup miss  */
  else
    {
      unsigned char *col;
      int diff, sum, max;
      int i;

      max = MAXDIFF;
      cmap_index = 0;

      col = cmap;
      for (i = 0; i < num_cols; i++)
	{
	  diff = r - *col++;
	  sum = diff * diff;
	  diff = g - *col++;
	  sum += diff * diff;
	  diff = b - *col++;
	  sum += diff * diff;

	  if (sum < max)
	    {
	      cmap_index = i;
	      max = sum;
	    }
	}

      /*  update the hash table  */
      color_hash_table[hash_index].pixel = pixel;
      color_hash_table[hash_index].index = cmap_index;
      color_hash_table[hash_index].colormap_ID = ID;
      color_hash_misses++;
    }

  return cmap_index;
}


