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
#include "drawable.h"
#include "gimage.h"



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


