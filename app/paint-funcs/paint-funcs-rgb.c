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

#ifdef CRAZY_PAINT_REWORK

#include "config.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include <glib.h>

#include "libgimpcolor/gimpcolor.h"
#include "libgimpmath/gimpmath.h"

#include "apptypes.h"

#include "gimprc.h"
#include "paint_funcsP.h"
#include "paint_funcs_rgb.h"
#include "pixel_processor.h"
#include "pixel_region.h"
#include "tile_manager.h"
#include "tile.h"


#define STD_BUF_SIZE       1021
#define MAXDIFF            195076
#define HASH_TABLE_SIZE    1021
#define RANDOM_TABLE_SIZE  4096
#define RANDOM_SEED        314159265
#define EPSILON            0.0001

#define INT_MULT(a,b,t)  ((t) = (a) * (b) + 0x80, ((((t) >> 8) + (t)) >> 8))

/* This version of INT_MULT3 is very fast, but suffers from some
   slight roundoff errors.  It returns the correct result 99.987
   percent of the time */
#define INT_MULT3(a,b,c,t)  ((t) = (a) * (b) * (c)+ 0x7F5B, \
                            ((((t) >> 7) + (t)) >> 16))
/*
  This version of INT_MULT3 always gives the correct result, but runs at
  approximatly one third the speed. */
/*  #define INT_MULT3(a,b,c,t) (((a) * (b) * (c)+ 32512) / 65025.0)
 */

#define INT_BLEND(a,b,alpha,tmp)  (INT_MULT((a)-(b), alpha, tmp) + (b))

typedef enum
{
  MinifyX_MinifyY,
  MinifyX_MagnifyY,
  MagnifyX_MinifyY,
  MagnifyX_MagnifyY
} ScaleType;


/*  Layer modes information  */
typedef struct _LayerMode LayerMode;
struct _LayerMode
{
  gboolean   affect_alpha;     /*  does the layer mode affect the alpha channel  */
  gboolean   increase_opacity; /*  layer mode can increase opacity */
  gboolean   decrease_opacity; /*  layer mode can decrease opacity */
};

static const LayerMode layer_modes[] =	
                               /* This must obviously be in the same
			        * order as the corresponding values
		         	* in the LayerModeEffects enumeration.
				*/
{
  { TRUE,  TRUE,  FALSE, },  /*  NORMAL_MODE        */
  { TRUE,  TRUE,  FALSE, },  /*  DISSOLVE_MODE      */
  { TRUE,  TRUE,  FALSE, },  /*  BEHIND_MODE        */
  { FALSE, FALSE, FALSE, },  /*  MULTIPLY_MODE      */
  { FALSE, FALSE, FALSE, },  /*  SCREEN_MODE        */
  { FALSE, FALSE, FALSE, },  /*  OVERLAY_MODE       */
  { FALSE, FALSE, FALSE, },  /*  DIFFERENCE_MODE    */
  { FALSE, FALSE, FALSE, },  /*  ADDITION_MODE      */
  { FALSE, FALSE, FALSE, },  /*  SUBTRACT_MODE      */
  { FALSE, FALSE, FALSE, },  /*  DARKEN_ONLY_MODE   */
  { FALSE, FALSE, FALSE, },  /*  LIGHTEN_ONLY_MODE  */
  { FALSE, FALSE, FALSE, },  /*  HUE_MODE           */
  { FALSE, FALSE, FALSE, },  /*  SATURATION_MODE    */
  { FALSE, FALSE, FALSE, },  /*  COLOR_MODE         */
  { FALSE, FALSE, FALSE, },  /*  VALUE_MODE         */
  { FALSE, FALSE, FALSE, },  /*  DIVIDE_MODE        */
  { FALSE, FALSE, FALSE, },  /*  DODGE_MODE         */
  { FALSE, FALSE, FALSE, },  /*  BURN_MODE          */
  { FALSE, FALSE, FALSE, },  /*  HARDLIGHT_MODE     */
  { TRUE,  FALSE, TRUE,  },  /*  ERASE_MODE         */
  { TRUE,  TRUE,  TRUE,  },  /*  REPLACE_MODE       */
  { TRUE,  FALSE, TRUE,  }   /*  ANTI_ERASE_MODE    */
};

/*  ColorHash structure  */
typedef struct _ColorHash ColorHash;

struct _ColorHash
{
  gint       pixel;           /*  R << 16 | G << 8 | B  */
  gint       index;           /*  colormap index        */
  GimpImage *gimage;     
};

static ColorHash  color_hash_table[HASH_TABLE_SIZE];
static gint       random_table[RANDOM_TABLE_SIZE];
static gint       color_hash_misses;
static gint       color_hash_hits;
static guchar    *tmp_buffer;  /* temporary buffer available upon request */
static gint       tmp_buffer_size;
static guchar     no_mask = OPAQUE_OPACITY;
static gint       add_lut[256][256];


void
update_tile_rowhints_rgb (Tile *tile,
			  guint  ymin,
			  guint  ymax)
{
  guint y;

  tile_sanitize_rowhints (tile);

  for (y = ymin; y <= ymax; y++)
    tile_set_rowhint (tile, y, TILEROWHINT_OPAQUE);

  return;
}


void
run_length_encode_rgb (const guchar *src,
		       guint   *dest,
		       guint    w)
{
  guint   start;
  guint   i;
  guint   j;
  guchar last;

  last = *src;
  src += 3;
  start = 0;

  for (i = 1; i < w; i++)
    {
      if (*src != last)
	{
	  for (j = start; j < i; j++)
	    {
	      *dest++ = (i - j);
	      *dest++ = last;
	    }
	  start = i;
	  last = *src;
	}
      src += 3;
    }

  for (j = start; j < i; j++)
    {
      *dest++ = (i - j);
      *dest++ = last;
    }
}

void
color_pixels_rgb (guchar       *dest,
		  const guchar *color,
		  guint         w)
{
  register guchar c0, c1, c2;

  c0 = color[0];
  c1 = color[1];
  c2 = color[2];

  while (w--)
    {
      dest[0] = c0;
      dest[1] = c1;
      dest[2] = c2;
      dest += 3;
    }
}


void
blend_pixels_rgb (const guchar *src1,
		  const guchar *src2,
		  guchar       *dest,
		  guint          blend,
		  guint          w)
{
  guint   b;
  guchar blend2 = (255 - blend);

  while (w --)
    {
      for (b = 0; b < 3; b++)
	dest[b] = (src1[b] * blend2 + src2[b] * blend) / 255;

      src1 += 3;
      src2 += 3;
      dest += 3;
    }
}


void
shade_pixels_rgb (const guchar *src,
		  guchar       *dest,
		  const guchar *col,
		  guint          blend,
		  guint          w)
{
  guint   b;
  guchar blend2 = (255 - blend);

  while (w --)
    {
      for (b = 0; b < 3; b++)
	dest[b] = (src[b] * blend2 + col[b] * blend) / 255;

      src += 3;
      dest += 3;
    }
}

void
darken_pixels_rgb_rgb (const guchar *src1,
		       const guchar *src2,
		       guchar       *dest,
		       guint         length)
		       
{
  while (length--)
    {
      dest[0] = (src1[0] < src2[0]) ? src1[0] : src2[0];
      dest[1] = (src1[1] < src2[1]) ? src1[1] : src2[1];
      dest[2] = (src1[2] < src2[2]) ? src1[2] : src2[2];

      src1 += 3;
      src2 += 3;
      dest += 3;
    }
}

void
darken_pixels_rgb_rgba (const guchar *src1,
		        const guchar *src2,
		        guchar       *dest,
		        guint         length)
{
  while (length--)
    {
      dest[0] = (src1[0] < src2[0]) ? src1[0] : src2[0];
      dest[1] = (src1[1] < src2[1]) ? src1[1] : src2[1];
      dest[2] = (src1[2] < src2[2]) ? src1[2] : src2[2];
      dest[3] = src2[3];

      src1 += 3;
      src2 += 4;
      dest += 4;
    }
}

void
lighten_pixels_rgb_rgb (const guchar *src1,
			const guchar *src2,
			guchar       *dest,
			guint         length)
{
  while (length--)
    {
      dest[0] = (src1[0] > src2[0]) ? src1[0] : src2[0];
      dest[1] = (src1[1] > src2[1]) ? src1[1] : src2[1];
      dest[2] = (src1[2] > src2[2]) ? src1[2] : src2[2];

      src1 += 3;
      src2 += 3;
      dest += 3;
    }
}

void
lighten_pixels_rgb_rgba (const guchar *src1,
			 const guchar *src2,
			 guchar       *dest,
			 guint          length)
{
  while (length--)
    {
      dest[0] = (src1[0] > src2[0]) ? src1[0] : src2[0];
      dest[1] = (src1[1] > src2[1]) ? src1[1] : src2[1];
      dest[2] = (src1[2] > src2[2]) ? src1[2] : src2[2];
      dest[3] = src2[3];

      src1 += 3;
      src2 += 4;
      dest += 4;
    }
}

void
multiply_pixels_rgb_rgb (const guchar *src1,
			 const guchar *src2,
			 guchar       *dest,
			 guint         length)
{
  guint tmp;

  while (length --)
    {
      dest[0] = INT_MULT(src1[0], src2[0], tmp);
      dest[1] = INT_MULT(src1[1], src2[1], tmp);
      dest[2] = INT_MULT(src1[2], src2[2], tmp);

      src1 += 3;
      src2 += 3;
      dest += 3;
    }
}

void
multiply_pixels_rgb_rgba (const guchar *src1,
			  const guchar *src2,
			  guchar       *dest,
			  guint          length)
{
  guint tmp;

  while (length --)
    {
      dest[0] = INT_MULT(src1[0], src2[0], tmp);
      dest[1] = INT_MULT(src1[1], src2[1], tmp);
      dest[2] = INT_MULT(src1[2], src2[2], tmp);
      dest[3] = src2[3];

      src1 += 3;
      src2 += 4;
      dest += 4;
    }
}

void
divide_pixels_rgb_rgb (const guchar *src1,
		       const guchar *src2,
		       guchar       *dest,
		       gint          length)
{
  guint b,result;

  while (length --)
    {
      for (b = 0; b < 3; b++)
	{
	  result = ((src1[b] * 256) / (1+src2[b]));
	  dest[b] = MIN (result, 255);
	}

      src1 += 3;
      src2 += 3;
      dest += 3;
    }
}

void
divide_pixels_rgb_rgba (const guchar *src1,
		        const guchar *src2,
		        guchar       *dest,
		        gint          length)
{
  guint b, result;

  while (length --)
    {
      for (b = 0; b < 3; b++)
	{
	  result = ((src1[b] * 256) / (1+src2[b]));
	  dest[b] = MIN (result, 255);
	}

      dest[3] = src2[3];

      src1 += 3;
      src2 += 4;
      dest += 4;
    }
}

void
screen_pixels_rgb_rgb (const guchar *src1,
		       const guchar *src2,
		       guchar       *dest,
		       gint          length)
{
  gint b;
  gint tmp;

  while (length --)
    {
      for (b = 0; b < 3; b++)
	dest[b] = 255 - INT_MULT((255 - src1[b]), (255 - src2[b]), tmp);

      src1 += 3;
      src2 += 3;
      dest += 3;
    }
}

void
screen_pixels_rgb_rgba (const guchar *src1,
		        const guchar *src2,
		        guchar       *dest,
		        gint          length)
{
  gint b;
  gint tmp;

  while (length --)
    {
      for (b = 0; b < 3; b++)
	dest[b] = 255 - INT_MULT((255 - src1[b]), (255 - src2[b]), tmp);

      dest[3] = src2[3];

      src1 += 3;
      src2 += 4;
      dest += 4;
    }
}

void
overlay_pixels_rgb_rgb (const guchar *src1,
			const guchar *src2,
			guchar       *dest,
			gint          length)
{
  guint tmp;

  while (length --)
    {
      dest[0] = INT_MULT(src1[0], src1[0] + INT_MULT(2 * src2[0],
						     255 - src1[0],
						     tmp), tmp);
      dest[1] = INT_MULT(src1[1], src1[1] + INT_MULT(2 * src2[1],
						     255 - src1[1],
						     tmp), tmp);
      dest[2] = INT_MULT(src1[2], src1[2] + INT_MULT(2 * src2[2],
						     255 - src1[2],
						     tmp), tmp);

      src1 += 3;
      src2 += 3;
      dest += 3;
    }
}

void
overlay_pixels_rgb_rgba (const guchar *src1,
			 const guchar *src2,
			 guchar       *dest,
			 guint         length)
{
  gint tmp;

  while (length --)
    {
      dest[0] = INT_MULT(src1[0], src1[0] + INT_MULT(2 * src2[0],
						     255 - src1[0],
						     tmp), tmp);
      dest[1] = INT_MULT(src1[1], src1[1] + INT_MULT(2 * src2[1],
						     255 - src1[1],
						     tmp), tmp);
      dest[2] = INT_MULT(src1[2], src1[2] + INT_MULT(2 * src2[2],
						     255 - src1[2],
						     tmp), tmp);

      dest[3] = src2[3];

      src1 += 3;
      src2 += 4;
      dest += 4;
    }
}


void
dodge_pixels_rgb_rgb (const guchar *src1,
		      const guchar *src2,
		      guchar       *dest,
		      guint         length)
{
  int tmp1;

  while (length --)
    {
      tmp1 = src1[0] << 8;
      tmp1 /= 256 - src2[0];
      dest[0] = (guchar) CLAMP (tmp1, 0, 255);

      tmp1 = src1[1] << 8;
      tmp1 /= 256 - src2[1];
      dest[1] = (guchar) CLAMP (tmp1, 0, 255);
      
      tmp1 = src1[2] << 8;
      tmp1 /= 256 - src2[2];
      dest[2] = (guchar) CLAMP (tmp1, 0, 255);
      
      src1 += 3;
      src2 += 3;
      dest += 3;
    }
}


void
dodge_pixels_rgb_rgba (const guchar *src1,
		       const guchar *src2,
		       guchar       *dest,
		       guint         length)
{
  int tmp1;

  while (length --)
    {
      tmp1 = src1[0] << 8;
      tmp1 /= 256 - src2[0];
      dest[0] = (guchar) CLAMP (tmp1, 0, 255);

      tmp1 = src1[1] << 8;
      tmp1 /= 256 - src2[1];
      dest[1] = (guchar) CLAMP (tmp1, 0, 255);
      
      tmp1 = src1[2] << 8;
      tmp1 /= 256 - src2[2];
      dest[2] = (guchar) CLAMP (tmp1, 0, 255);

      dest[3] = src2[3];

      src1 += 3;
      src2 += 4;
      dest += 4;
    }
}


void
burn_pixels_rgb_rgba (const guchar *src1,
		      const guchar *src2,
		      guchar       *dest,
		      guint         length)
{
  gint tmp1;

  while (length --)
    {
      tmp1 = (255 - src1[0]) << 8;
      tmp1 /= src2[0] + 1;
      dest[0] = (guchar) CLAMP (255 - tmp1, 0, 255);

      tmp1 = (255 - src1[1]) << 8;
      tmp1 /= src2[1] + 1;
      dest[1] = (guchar) CLAMP (255 - tmp1, 0, 255);

      tmp1 = (255 - src1[2]) << 8;
      tmp1 /= src2[2] + 1;
      dest[2] = (guchar) CLAMP (255 - tmp1, 0, 255);

      dest[3] = src2[3];

      src1 += 3;
      src2 += 4;
      dest += 4;
    }
}

void
burn_pixels_rgb_rgb (const guchar *src1,
		     const guchar *src2,
		     guchar       *dest,
		     guint         length)
{
  gint tmp1;

  while (length --)
    {
      tmp1 = (255 - src1[0]) << 8;
      tmp1 /= src2[0] + 1;
      dest[0] = (guchar) CLAMP (255 - tmp1, 0, 255);

      tmp1 = (255 - src1[1]) << 8;
      tmp1 /= src2[1] + 1;
      dest[1] = (guchar) CLAMP (255 - tmp1, 0, 255);

      tmp1 = (255 - src1[2]) << 8;
      tmp1 /= src2[2] + 1;
      dest[2] = (guchar) CLAMP (255 - tmp1, 0, 255);

      src1 += 3;
      src2 += 3;
      dest += 3;
    }
}


void
hardlight_pixels_rgb_rgba (const guchar *src1,
			   const guchar *src2,
			   guchar       *dest,
			   gint          length)
{
  gint tmp1;

  while (length --)
    {
      if (src2[0] > 128) {
	tmp1 = ((gint)255 - src1[0]) * ((gint)255 - ((src2[0] - 128) << 1));
	dest[0] = (guchar)CLAMP(255 - (tmp1 >> 8), 0, 255);
      } else {
	tmp1 = (gint)src1[0] * ((gint)src2[0] << 1);
	dest[0] = (guchar)CLAMP(tmp1 >> 8, 0, 255);
      }

      if (src2[1] > 128) {
	tmp1 = ((gint)255 - src1[1]) * ((gint)255 - ((src2[1] - 128) << 1));
	dest[1] = (guchar)CLAMP(255 - (tmp1 >> 8), 0, 255);
      } else {
	tmp1 = (gint)src1[1] * ((gint)src2[1] << 1);
	dest[1] = (guchar)CLAMP(tmp1 >> 8, 0, 255);
      }

      if (src2[2] > 128) {
	tmp1 = ((gint)255 - src1[2]) * ((gint)255 - ((src2[2] - 128) << 1));
	dest[2] = (guchar)CLAMP(255 - (tmp1 >> 8), 0, 255);
      } else {
	tmp1 = (gint)src1[2] * ((gint)src2[2] << 1);
	dest[2] = (guchar)CLAMP(tmp1 >> 8, 0, 255);
      }

      dest[3] = src2[3];

      src1 += 3;
      src2 += 4;
      dest += 4;
    }
}

void
hardlight_pixels_rgb_rgb (const guchar *src1,
			  const guchar *src2,
			  guchar       *dest,
			  gint          length)
{
  gint tmp1;

  while (length --)
    {
      if (src2[0] > 128) {
	tmp1 = ((gint)255 - src1[0]) * ((gint)255 - ((src2[0] - 128) << 1));
	dest[0] = (guchar)CLAMP(255 - (tmp1 >> 8), 0, 255);
      } else {
	tmp1 = (gint)src1[0] * ((gint)src2[0] << 1);
	dest[0] = (guchar)CLAMP(tmp1 >> 8, 0, 255);
      }

      if (src2[1] > 128) {
	tmp1 = ((gint)255 - src1[1]) * ((gint)255 - ((src2[1] - 128) << 1));
	dest[1] = (guchar)CLAMP(255 - (tmp1 >> 8), 0, 255);
      } else {
	tmp1 = (gint)src1[1] * ((gint)src2[1] << 1);
	dest[1] = (guchar)CLAMP(tmp1 >> 8, 0, 255);
      }

      if (src2[2] > 128) {
	tmp1 = ((gint)255 - src1[2]) * ((gint)255 - ((src2[2] - 128) << 1));
	dest[2] = (guchar)CLAMP(255 - (tmp1 >> 8), 0, 255);
      } else {
	tmp1 = (gint)src1[2] * ((gint)src2[2] << 1);
	dest[2] = (guchar)CLAMP(tmp1 >> 8, 0, 255);
      }

      src1 += 3;
      src2 += 3;
      dest += 3;
    }
}


void
add_pixels_rgb_rgb (const guchar *src1,
		    const guchar *src2,
		    guchar       *dest,
		    guint         length)
{
  while (length --)
    {
      dest[0] = add_lut[(src1[0])] [(src2[0])];
      dest[1] = add_lut[(src1[1])] [(src2[1])];
      dest[2] = add_lut[(src1[2])] [(src2[2])];

      src1 += 3;
      src2 += 3;
      dest += 3;
    }
}


void
add_pixels_rgb_rgba (const guchar *src1,
		     const guchar *src2,
		     guchar       *dest,
		     guint         length)
{
  while (length --)
    {
      dest[0] = add_lut[(src1[0])] [(src2[0])];
      dest[1] = add_lut[(src1[1])] [(src2[1])];
      dest[2] = add_lut[(src1[2])] [(src2[2])];
      dest[3] = src2[3];

      src1 += 3;
      src2 += 4;
      dest += 4;
    }
}


void
subtract_pixels_rgb_rgb (const guchar *src1,
			 const guchar *src2,
			 guchar       *dest,
			 guint         length)
{
  gint diff;

  while (length --)
    {
      diff = src1[0] - src2[0];
      dest[0] = (diff < 0) ? 0 : diff;

      diff = src1[1] - src2[1];
      dest[1] = (diff < 0) ? 0 : diff;
      
      diff = src1[2] - src2[2];
      dest[2] = (diff < 0) ? 0 : diff;
      
      src1 += 3;
      src2 += 3;
      dest += 3;
    }
}


void
subtract_pixels_rgb_rgba (const guchar *src1,
			  const guchar *src2,
			  guchar       *dest,
			  guint         length)
{
  gint diff;

  while (length --)
    {
      diff = src1[0] - src2[0];
      dest[0] = (diff < 0) ? 0 : diff;

      diff = src1[1] - src2[1];
      dest[1] = (diff < 0) ? 0 : diff;
      
      diff = src1[2] - src2[2];
      dest[2] = (diff < 0) ? 0 : diff;
      
      dest[3] = src2[3];

      src1 += 3;
      src2 += 4;
      dest += 4;
    }
}


void
difference_pixels_rgb_rgb (const guchar *src1,
			   const guchar *src2,
			   guchar       *dest,
			   guint         length)
{
  gint diff;

  while (length --)
    {
      diff = src1[0] - src2[0];
      dest[0] = (diff < 0) ? -diff : diff;
      
      diff = src1[1] - src2[1];
      dest[1] = (diff < 0) ? -diff : diff;
      
      diff = src1[2] - src2[2];
      dest[2] = (diff < 0) ? -diff : diff;

      src1 += 3;
      src2 += 3;
      dest += 3;
    }
}


void
difference_pixels_rgb_rgba (const guchar *src1,
			    const guchar *src2,
			    guchar       *dest,
			    guint         length)
{
  gint diff;

  while (length --)
    {
      diff = src1[0] - src2[0];
      dest[0] = (diff < 0) ? -diff : diff;
      
      diff = src1[1] - src2[1];
      dest[1] = (diff < 0) ? -diff : diff;
      
      diff = src1[2] - src2[2];
      dest[2] = (diff < 0) ? -diff : diff;

      dest[3] = src2[3];

      src1 += 3;
      src2 += 4;
      dest += 4;
    }
}

void
add_alpha_pixels_rgb (const guchar *src,
		      guchar       *dest,
		      gint          length)
{
  while (length --)
    {
      dest[0] = src[0];
      dest[1] = src[1];
      dest[2] = src[2];

      dest[3] = OPAQUE_OPACITY;

      src += 3;
      dest += 4;
    }
}


void
initial_pixels_rgb (const guchar *src,
		    guchar       *dest,
		    const guchar *mask,
		    guint         opacity,
		    const guint  *affect,
 		    guint         length)
{
  gint  b;
  gint  tmp;
  gint  l;
  const guchar *m;
  guchar       *destp;
  const guchar *srcp;

  if (mask)
  {
    m = mask;
    
    /*  This function assumes the source has no alpha channel and
     *  the destination has an alpha channel.  So dest_bytes = bytes + 1
     */
   
    if (affect[0] && affect[1] && affect[2])
      {
	if (!affect[3])
	  opacity = 0;
    
	destp = dest + 3;
	
	if (opacity != 0)
	  while(length--)
	    {
	      dest[0] = src[0];
	      dest[1] = src[1];
	      dest[2] = src[2];
	      dest[3] = INT_MULT(opacity, *m, tmp);
	      src  += 3;
	      dest += 4;
	      m++;
	    }
	else
	  while(length--)
	    {
	      dest[0] = src[0];
	      dest[1] = src[1];
	      dest[2] = src[2];
	      dest[3] = 0;
	      src  += 3;
	      dest += 4;
	    }
	return;
      }

    for (b = 0; b < 3; b++)
      {
	destp = dest + b;
	srcp = src + b;
	l = length;

	if (affect[b])
	  while(l--)
	    {
	      *destp = *srcp;
	      srcp  += 3;
	      destp += 4;
	    }
	else
	  while(l--)
	    {
	      *destp = 0;
	      destp += 4;
	    }
      }
    
    /* fill the alpha channel */ 
    if (!affect[3])
      opacity = 0;
 
    destp = dest + 3;
   
    if (opacity != 0)
      while (length--)
	{
	  *destp = INT_MULT(opacity , *m, tmp);
	  destp += 4;
	  m++;
	}
    else
      while (length--)
	{
	  *destp = 0;
	  destp += 4;
	}
  }
  
  /* If no mask */
  else
    {
      m = &no_mask;
      
      /*  This function assumes the source has no alpha channel and
       *  the destination has an alpha channel.  So dest_bytes = bytes + 1
       */
      
      if (affect[0] && affect[1] && affect[2])
	{
	  if (!affect[3])
	    opacity = 0;

	  destp = dest + 3;
	  
	  while(length--)
	    {
	      dest[0] = src[0];
	      dest[1] = src[1];
	      dest[2] = src[2];
	      dest[3] = opacity;
	      src  += 3;
	      dest += 4;
	    }
	  return;
	}
      
      for (b = 0; b < 3; b++)
	{
	  destp = dest + b;
	  srcp = src + b;
	  l = length;

	  if (affect[b])
	    while(l--)
	      {
		*destp = *srcp;
		srcp  += 3;
		destp += 4;
	      }
	  else
	    while(l--)
	      {
		*destp = 0;
		destp += 4;
	      }
      }
      
      /* fill the alpha channel */ 
      if (!affect[3])
	opacity = 0;

      destp = dest + 3;
    
      while (length--)
	{
	  *destp = opacity;
	  destp += 4;
	}
    }
}


void
combine_rgb_and_rgb_pixels (const guchar   *src1,
			    const guchar   *src2,
			    guchar         *dest,
			    const guchar   *mask,
			    gint            opacity,
			    const gboolean *affect,
			    gint            length)
{
  const guchar * m;
  gint   b;
  guchar new_alpha;
  gint   tmp;

  if (mask)
    {
      m = mask;
      while (length --)
	{
	  new_alpha = INT_MULT(*m, opacity, tmp);

	  for (b = 0; b < 3; b++)
	    dest[b] = (affect[b]) ?
	      INT_BLEND(src2[b], src1[b], new_alpha, tmp) :
	    src1[b];

	  m++;

	  src1 += 3;
	  src2 += 3;
	  dest += 3;
	}
    }
  else
    {
      while (length --)
	{

	  for (b = 0; b < 3; b++)
	    dest[b] = (affect[b]) ?
	      INT_BLEND(src2[b], src1[b], opacity, tmp) :
	    src1[b];

	  src1 += 3;
	  src2 += 3;
	  dest += 3;
	}
    }
}


void
combine_rgb_and_rgba_pixels (const guchar   *src1,
			     const guchar   *src2,
			     guchar         *dest,
			     const guchar   *mask,
			     gint            opacity,
			     const gboolean *affect,
			     gint            length)
{
  gint   b;
  guchar new_alpha;
  const guchar   *m;
  register glong  t1;

  if (mask)
    {
      m = mask;
      while (length --)
	{
	  new_alpha = INT_MULT3(src2[3], *m, opacity, t1);

	  for (b = 0; b < 3; b++)
	    dest[b] = (affect[b]) ?
	      INT_BLEND(src2[b], src1[b], new_alpha, t1) :
	      src1[b];

	  m++;
	  src1 += 3;
	  src2 += 4;
	  dest += 3;
	}
    }
  else
    {
      if (affect[0] && affect[1] && affect[2])
	while (length --)
	{
	  new_alpha = INT_MULT(src2[3], opacity, t1);
	  dest[0] = INT_BLEND(src2[0] , src1[0] , new_alpha, t1);
	  dest[1] = INT_BLEND(src2[1] , src1[1] , new_alpha, t1);
	  dest[2] = INT_BLEND(src2[2] , src1[2] , new_alpha, t1);
	  src1 += 3;
	  src2 += 4;
	  dest += 3;
	}
      else
	while (length --)
	{
	  new_alpha = INT_MULT(src2[3],opacity,t1);
	  for (b = 0; b < 3; b++)
	    dest[b] = (affect[b]) ?
	      INT_BLEND(src2[b] , src1[b] , new_alpha, t1) :
	    src1[b];

	  src1 += 3;
	  src2 += 4;
	  dest += 3;
	}
    }
}


void
replace_rgb_pixels (const guchar   *src1,
		    const guchar   *src2,
		    guchar         *dest,
		    const guchar   *mask,
		    gint            opacity,
		    const gboolean *affect,
		    gint            length,
		    gint            bytes2,
		    gint            has_alpha2)
{
  gint  b;
  gint  tmp;

  if (mask)
    {
      guchar        mask_alpha;
      const guchar *m = mask;

      while (length --)
        {
	  mask_alpha = INT_MULT(*m, opacity, tmp);
	  
	  for (b = 0; b < 3; b++)
	    dest[b] = (affect[b]) ?
	      INT_BLEND(src2[b], src1[b], mask_alpha, tmp) :
	      src1[b];

	  m++;

	  src1 += 3;
	  src2 += bytes2;
	  dest += 3;
	}
    }
  else
    {
      static const guchar mask_alpha = OPAQUE_OPACITY ;

      while (length --)
        {
	  for (b = 0; b < 3; b++)
	    dest[b] = (affect[b]) ?
	      INT_BLEND(src2[b], src1[b], mask_alpha, tmp) :
	      src1[b];

	  src1 += 3;
	  src2 += bytes2;
	  dest += 3;
	}
    }
}

void
extract_from_pixels_rgb (guchar       *src,
			 guchar       *dest,
			 const guchar *mask,
			 const guchar *bg,
			 gboolean      cut,
			 guint         length)
{
  gint          b;
  const guchar *m;
  gint          tmp;

  if (mask)
    m = mask;
  else
    m = &no_mask;

  while (length --)
    {
      for (b = 0; b < 3; b++)
	dest[b] = src[b];

	dest[3] = *m;
	if (cut)
	  for (b = 0; b < 3; b++)
	    src[b] = INT_BLEND(bg[b], src[b], *m, tmp);

      if (mask)
	m++;

      src += 3;
      dest += 4;
    }
}


gint
map_rgb_to_indexed (const guchar    *cmap,
		    gint             num_cols,
		    const GimpImage *gimage,
		    gint             r,
		    gint             g,
		    gint             b)
{
  guint pixel;
  gint  hash_index;
  gint  cmap_index;

  pixel = (r << 16) | (g << 8) | b;
  hash_index = pixel % HASH_TABLE_SIZE;

  /*  Hash table lookup hit  */
  if (color_hash_table[hash_index].gimage == gimage &&
      color_hash_table[hash_index].pixel == pixel)
    {
      cmap_index = color_hash_table[hash_index].index;
      color_hash_hits++;
    }
  /*  Hash table lookup miss  */
  else
    {
      const guchar *col;
      gint          diff, sum, max;
      gint          i;

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
      color_hash_table[hash_index].pixel  = pixel;
      color_hash_table[hash_index].index  = cmap_index;
      color_hash_table[hash_index].gimage = (GimpImage *) gimage;
      color_hash_misses++;
    }

  return cmap_index;
}

static void
shrink_line_rgb (gdouble           *dest,
		 gdouble           *src,
		 gint               old_width,
		 gint               width,
		 InterpolationType  interp)
{
  gint x, b;
  gdouble *source, *destp;
  register gdouble accum;
  register guint max;
  register gdouble mant, tmp;
  register const gdouble step = old_width / (gdouble) width;
  register const gdouble inv_step = 1.0 / step;
  gdouble position;

  for (b = 0; b < 3; b++)
  {
    
    source = &src[b];
    destp = &dest[b];
    position = -1;

    mant = *source;

    for (x = 0; x < width; x++)
    {
      source += 3;
      accum = 0;
      max = ((int)(position+step)) - ((int)(position));
      max--;
      while (max)
      {
	accum += *source;
	source += 3;
	max--;
      }
      tmp = accum + mant;
      mant = ((position+step) - (int)(position + step));
      mant *= *source;
      tmp += mant;
      tmp *= inv_step;
      mant = *source - mant;
      *(destp) = tmp;
      destp += 3;
      position += step;
	
    }
  }
}

void
apply_layer_mode_rgb_rgb (apply_combine_layer_info *info)
{
  const guchar *src1 = info->src1;
  const guchar *src2 = info->src2;
  guchar *dest = info->dest;
  guint x = info->x;
  guint y = info->y;
  guint opacity = opacity;
  guint length = info->length;
  LayerModeEffects mode = info->mode;
  guint *mode_affect = info->mode_affect;

  gint combine = COMBINE_INTEN_INTEN;

  /*  assumes we're applying src2 TO src1  */
  switch (mode)
    {
      case NORMAL_MODE:
      case ERASE_MODE:
      case ANTI_ERASE_MODE:
	dest = (guchar *) src2;
	break;

      case DISSOLVE_MODE:
	/*  Since dissolve requires an alpha channels...  */
	add_alpha_pixels_rgb (src2, dest, length);

	dissolve_pixels_rgba (src2, dest, x, y, opacity, length);
	combine = COMBINE_INTEN_INTEN_A;
	break;

      case MULTIPLY_MODE:
	multiply_pixels_rgb_rgb (src1, src2, dest, length);
	break;

      case DIVIDE_MODE:
	divide_pixels_rgb_rgb (src1, src2, dest, length);
	break;

      case SCREEN_MODE:
	screen_pixels_rgb_rgb (src1, src2, dest, length);
	break;

      case OVERLAY_MODE:
	overlay_pixels_rgb_rgb (src1, src2, dest, length);
	break;

      case DIFFERENCE_MODE:
	difference_pixels_rgb_rgb (src1, src2, dest, length);
	break;

      case ADDITION_MODE:
	add_pixels_rgb_rgb (src1, src2, dest, length);
	break;

      case SUBTRACT_MODE:
	subtract_pixels_rgb_rgb (src1, src2, dest, length);
	break;

      case DARKEN_ONLY_MODE:
	darken_pixels_rgb_rgb (src1, src2, dest, length);
	break;

      case LIGHTEN_ONLY_MODE:
	lighten_pixels_rgb_rgb (src1, src2, dest, length);
	break;

      case HUE_MODE: case SATURATION_MODE: case VALUE_MODE:
	hsv_only_pixels_rgba_rgba (src1, src2, dest, mode, length);
	break;

      case COLOR_MODE:
	color_only_pixels_rgba_rgba (src1, src2, dest, mode, length);
	break;

      case BEHIND_MODE:
	dest = src2;
	combine = NO_COMBINATION;
	break;

      case REPLACE_MODE:
	dest = src2;
	combine = REPLACE_INTEN;
	break;

      case DODGE_MODE:
	dodge_pixels_rgb_rgb (src1, src2, dest, length);
	break;

      case BURN_MODE:
	burn_pixels_rgb_rgb (src1, src2, dest, length);
	break;

      case HARDLIGHT_MODE:
	hardlight_pixels_rgb_rgb (src1, src2, dest, length);
	break;

      default:
	break;
    }

  /*  Determine whether the alpha channel of the destination can be affected
   *  by the specified mode--This keeps consistency with varying opacities
   */
  *mode_affect = layer_modes[mode].affect_alpha;
}

void
apply_layer_mode_rgb_rgba (apply_combine_layer_info *info)
{
  const guchar *src1 = info->src1;
  const guchar *src2 = info->src2;
  guchar *dest = info->dest;
  guint x = info->x;
  guint y = info->y;
  guint opacity = opacity;
  guint length = info->length;
  LayerModeEffects mode = info->mode;
  guint *mode_affect = info->mode_affect;

  gint combine = COMBINE_INTEN_INTEN_A;

  switch (mode)
    {
      case NORMAL_MODE:
      case ERASE_MODE:
      case ANTI_ERASE_MODE:
	dest = src2;
	break;

      case DISSOLVE_MODE:
	dissolve_pixels_rgba (src2, dest, x, y, opacity, length);
	break;

      case MULTIPLY_MODE:
	multiply_pixels_rgb_rgba (src1, src2, dest, length);
	break;

      case DIVIDE_MODE:
	divide_pixels_rgb_rgba (src1, src2, dest, length);
	break;

      case SCREEN_MODE:
	screen_pixels_rgb_rgba (src1, src2, dest, length);
	break;

      case OVERLAY_MODE:
	overlay_pixels_rgb_rgba (src1, src2, dest, length);
	break;

      case DIFFERENCE_MODE:
	difference_pixels_rgb_rgba (src1, src2, dest, length);
	break;

      case ADDITION_MODE:
	add_pixels_rgb_rgba (src1, src2, dest, length);
	break;

      case SUBTRACT_MODE:
	subtract_pixels_rgb_rgba (src1, src2, dest, length);
	break;

      case DARKEN_ONLY_MODE:
	darken_pixels_rgb_rgba (src1, src2, dest, length);
	break;

      case LIGHTEN_ONLY_MODE:
	lighten_pixels_rgb_rgba (src1, src2, dest, length);
	break;

      case HUE_MODE: case SATURATION_MODE: case VALUE_MODE:
	g_warning ("Cannot calculate hsv only view of nonalpha nonRGB images!\n");	
	break;

      case COLOR_MODE:
	g_warning ("Cannot calculate color only view of nonalpha nonRGB images!\n");	
	break;

      case BEHIND_MODE:
	dest = src2;
	combine = NO_COMBINATION;
	break;

      case REPLACE_MODE:
	dest = src2;
	combine = REPLACE_INTEN;
	break;

      case DODGE_MODE:
	dodge_pixels_rgb_rgba (src1, src2, dest, length);
	break;

      case BURN_MODE:
	burn_pixels_rgb_rgba (src1, src2, dest, length);
	break;

      case HARDLIGHT_MODE:
	hardlight_pixels_rgb_rgba (src1, src2, dest, length);
	break;

      default:
	break;
    }

  /*  Determine whether the alpha channel of the destination can be affected
   *  by the specified mode--This keeps consistency with varying opacities
   */
  *mode_affect = layer_modes[mode].affect_alpha;
}

#endif
