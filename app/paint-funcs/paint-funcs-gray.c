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
#include "paint_funcs_gray.h"
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

static gint       add_lut[256][256];

/*
static void     run_length_encode        (guchar   *src,
					  gint     *dest,
					  gint      w,
					  gint      bytes);

					  
static gdouble  cubic                    (gdouble   dx,
					  gint      jm1,
					  gint      j,
					  gint      jp1,
					  gint      jp2);
static void     apply_layer_mode_replace (guchar   *src1,
					  guchar   *src2,
					  guchar   *dest,
					  guchar   *mask,
					  gint      x,
					  gint      y,
					  gint      opacity,
					  gint      length,
					  gint      bytes1,
					  gint      bytes2,
					  gboolean *affect);
static void     rotate_pointers          (gpointer *p, 
					  guint32   n); */

void
update_tile_rowhints_gray (Tile *tile,
			   guint ymin,
			   guint ymax)
{
  guint        y;

  tile_sanitize_rowhints (tile);

  for (y = ymin; y <= ymax; y++)
    tile_set_rowhint (tile, y, TILEROWHINT_OPAQUE);

  return;
}


/*
 * The equations: g(r) = exp (- r^2 / (2 * sigma^2))
 *                   r = sqrt (x^2 + y ^2)
 */

static gint *
make_curve (gdouble  sigma,
	    gint    *length)
{
  gint    *curve;
  gdouble  sigma2;
  gdouble  l;
  gint     temp;
  gint     i, n;

  sigma2 = 2 * sigma * sigma;
  l = sqrt (-sigma2 * log (1.0 / 255.0));

  n = ceil (l) * 2;
  if ((n % 2) == 0)
    n += 1;

  curve = g_new (gint, n);

  *length = n / 2;
  curve += *length;
  curve[0] = 255;

  for (i = 1; i <= *length; i++)
    {
      temp = (gint) (exp (- (i * i) / sigma2) * 255);
      curve[-i] = temp;
      curve[i] = temp;
    }

  return curve;
}


static void
run_length_encode_gray (guchar *src,
			gint   *dest,
			gint    w)
{
  gint   start;
  gint   i;
  gint   j;
  guchar last;

  last = *src;
  src++;
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
      src++;
    }

  for (j = start; j < i; j++)
    {
      *dest++ = (i - j);
      *dest++ = last;
    }
}

/* Note: cubic function no longer clips result */
static inline gdouble
cubic (gdouble dx,
       gint    jm1,
       gint    j,
       gint    jp1,
       gint    jp2)
{
  /* Catmull-Rom - not bad */
  return (gdouble) ((( ( - jm1 + 3 * j - 3 * jp1 + jp2 ) * dx +
		       ( 2 * jm1 - 5 * j + 4 * jp1 - jp2 ) ) * dx +
		     ( - jm1 + jp1 ) ) * dx + (j + j) ) / 2.0;
}

void
color_pixels_gray (guchar       *dest,
		   const guchar *color,
                   guint         w)
{
  memset (dest, *color, w);
}


void
blend_pixels_gray (const guchar *src1,
		   const guchar *src2,
		   guchar       *dest,
		   guint         blend,
		   guint         w)
{
  guchar blend2 = (255 - blend);

  while (w --)
    {
      dest[0] = (src1[0] * blend2 + src2[0] * blend) / 255;

      src1++;
      src2++;
      dest++;
    }
}


void
shade_pixels_gray (const guchar *src,
		   guchar       *dest,
		   const guchar *col,
		   guint         blend,
		   guint         w)
{
  const guchar blend2 = (255 - blend);

  while (w --)
    {
      dest[0] = (src[0] * blend2 + col[0] * blend) / 255;

      src++;
      dest++;
    }
}


void
darken_pixels_gray_gray (const guchar *src1,
			 const guchar *src2,
			 guchar       *dest,
			 guint         length)
		       
{
  while (length--)
    {
      dest[0] = (src1[0] < src2[0]) ? src1[0] : src2[0];

      src1 += 1;
      src2 += 1;
      dest += 1;
    }
}

void
darken_pixels_gray_graya (const guchar *src1,
			  const guchar *src2,
			  guchar       *dest,
			  guint         length)
{
  while (length--)
    {
      dest[0] = (src1[0] < src2[0]) ? src1[0] : src2[0];
      dest[1] = src2[1];

      src1 += 1;
      src2 += 2;
      dest += 2;
    }
}

void
lighten_pixels_gray_gray (const guchar *src1,
			  const guchar *src2,
			  guchar       *dest,
			  guint         length)
{
  while (length--)
    {
      dest[0] = (src1[0] > src2[0]) ? src1[0] : src2[0];

      src1 += 1;
      src2 += 1;
      dest += 1;
    }
}

void
lighten_pixels_gray_graya (const guchar *src1,
			   const guchar *src2,
			   guchar       *dest,
			   guint          length)
{
  while (length--)
    {
      dest[0] = (src1[0] > src2[0]) ? src1[0] : src2[0];
      dest[1] = src2[1];

      src1 += 1;
      src2 += 2;
      dest += 2;
    }
}


void
multiply_pixels_gray_gray (const guchar *src1,
			   const guchar *src2,
			   guchar       *dest,
			   guint         length)
{
  guint tmp;

  while (length --)
    {
      dest[0] = INT_MULT(src1[0], src2[0], tmp);

      src1 += 1;
      src2 += 1;
      dest += 1;
    }
}


void
multiply_pixels_gray_graya (const guchar *src1,
			    const guchar *src2,
			    guchar       *dest,
			    guint         length)
{
  guint tmp;

  while (length --)
    {
      dest[0] = INT_MULT(src1[0], src2[0], tmp);
      dest[1] = src2[1];

      src1 += 1;
      src2 += 2;
      dest += 2;
    }
}

void
divide_pixels_gray_gray (const guchar *src1,
			 const guchar *src2,
			 guchar       *dest,
			 guint          length)
{
  guint result;

  while (length --)
    {
      result = ((src1[0] * 256) / (1+src2[0]));
      dest[0] = MIN (result, 255);

      src1 += 1;
      src2 += 1;
      dest += 1;
    }
}


void
divide_pixels_gray_graya (const guchar *src1,
			  const guchar *src2,
			  guchar       *dest,
			  guint          length)
{
  guint result;

  while (length --)
    {
      result = ((src1[0] * 256) / (1+src2[0]));
      dest[0] = MIN (result, 255);

      dest[1] = src2[1];

      src1 += 1;
      src2 += 2;
      dest += 2;
    }
}


void
screen_pixels_gray_gray (const guchar *src1,
			 const guchar *src2,
			 guchar       *dest,
			 guint          length)
{
  guint tmp;

  while (length --)
    {
      dest[0] = 255 - INT_MULT((255 - src1[0]), (255 - src2[0]), tmp);

      src1 += 1;
      src2 += 1;
      dest += 1;
    }
}


void
screen_pixels_gray_graya (const guchar *src1,
			  const guchar *src2,
			  guchar       *dest,
			  guint         length)
{
  guint tmp;

  while (length --)
    {
      dest[0] = 255 - INT_MULT((255 - src1[0]), (255 - src2[0]), tmp);
      dest[1] = src2[1];

      src1 += 1;
      src2 += 2;
      dest += 2;
    }
}

void
overlay_pixels_gray_gray (const guchar *src1,
			  const guchar *src2,
			  guchar       *dest,
			  guint          length)
{
  guint tmp;

  while (length --)
    {
      dest[0] = INT_MULT(src1[0], src1[0] + INT_MULT(2 * src2[0],
						     255 - src1[0],
						     tmp), tmp);

      src1 += 1;
      src2 += 1;
      dest += 1;
    }
}


void
overlay_pixels_gray_graya (const guchar *src1,
			   const guchar *src2,
			   guchar       *dest,
			   guint          length)
{
  guint tmp;

  while (length --)
    {
      dest[0] = INT_MULT(src1[0], src1[0] + INT_MULT(2 * src2[0],
						     255 - src1[0],
						     tmp), tmp);

      dest[1] = src2[1];

      src1 += 1;
      src2 += 2;
      dest += 2;
    }
}



void
dodge_pixels_gray_gray (const guchar *src1,
		        const guchar *src2,
		        guchar	*dest,
		        guint		 length)
{
  guint tmp1;

  while (length --)
    {
      tmp1 = src1[0] << 8;
      tmp1 /= 256 - src2[0];
      dest[0] = (guchar) CLAMP (tmp1, 0, 255);

      src1 += 1;
      src2 += 1;
      dest += 1;
    }
}


void
dodge_pixels_gray_graya (const guchar *src1,
			 const guchar *src2,
			 guchar	*dest,
			 guint		 length)
{
  guint tmp1;

  while (length --)
    {
      tmp1 = src1[0] << 8;
      tmp1 /= 256 - src2[0];
      dest[0] = (guchar) CLAMP (tmp1, 0, 255);

      dest[1] = src2[1];

      src1 += 1;
      src2 += 2;
      dest += 2;
    }
}


void
burn_pixels_gray_gray (const guchar *src1,
		       const guchar *src2,
		       guchar       *dest,
		       guint         length)
{
  guint tmp1;

  while (length --)
    {
      tmp1 = (255 - src1[0]) << 8;
      tmp1 /= src2[0] + 1;
      dest[0] = (guchar) CLAMP (255 - tmp1, 0, 255);

      src1 += 1;
      src2 += 1;
      dest += 1;
    }
}


void
burn_pixels_gray_graya (const guchar *src1,
			const guchar *src2,
			guchar       *dest,
			guint          length)
{
  guint tmp1;

  while (length --)
    {
      tmp1 = (255 - src1[0]) << 8;
      tmp1 /= src2[0] + 1;
      dest[0] = (guchar) CLAMP (255 - tmp1, 0, 255);

      dest[1] = src2[1];

      src1 += 1;
      src2 += 2;
      dest += 2;
    }
}


void
hardlight_pixels_gray_gray (const guchar *src1,
			    const guchar *src2,
			    guchar	    *dest,
			    guint	     length)
{
  guint tmp1;

  while (length --)
    {
      if (src2[0] > 128) {
	tmp1 = ((gint)255 - src1[0]) * ((gint)255 - ((src2[0] - 128) << 1));
	dest[0] = (guchar)CLAMP(255 - (tmp1 >> 8), 0, 255);
      } else {
	tmp1 = (gint)src1[0] * ((gint)src2[0] << 1);
	dest[0] = (guchar)CLAMP(tmp1 >> 8, 0, 255);
      }

      src1 += 1;
      src2 += 1;
      dest += 1;
    }
}


void
hardlight_pixels_gray_graya (const guchar *src1,
			     const guchar *src2,
			     guchar	    *dest,
			     guint	     length)
{
  guint tmp1;

  while (length --)
    {
      if (src2[0] > 128) {
	tmp1 = ((gint)255 - src1[0]) * ((gint)255 - ((src2[0] - 128) << 1));
	dest[0] = (guchar)CLAMP(255 - (tmp1 >> 8), 0, 255);
      } else {
	tmp1 = (gint)src1[0] * ((gint)src2[0] << 1);
	dest[0] = (guchar)CLAMP(tmp1 >> 8, 0, 255);
      }

      dest[1] = src2[1];

      src1 += 1;
      src2 += 2;
      dest += 2;
    }
}


void
add_pixels_gray_gray (const guchar *src1,
		      const guchar *src2,
		      guchar       *dest,
		      guint         length)
{
  while (length --)
    {
      dest[0] = add_lut[(src1[0])] [(src2[0])];

      src1 += 1;
      src2 += 1;
      dest += 1;
    }
}


void
add_pixels_gray_graya (const guchar *src1,
		       const guchar *src2,
		       guchar       *dest,
		       guint         length)
{
  while (length --)
    {
      dest[0] = add_lut[(src1[0])] [(src2[0])];
      dest[1] = src2[1];

      src1 += 1;
      src2 += 2;
      dest += 2;
    }
}


void
subtract_pixels_gray_gray (const guchar *src1,
			   const guchar *src2,
			   guchar       *dest,
			   guint         length)
{
  gint diff;

  while (length --)
    {
      diff = src1[0] - src2[0];
      dest[0] = (diff < 0) ? 0 : diff;

      src1 += 1;
      src2 += 1;
      dest += 1;
    }
}


void
subtract_pixels_gray_graya (const guchar *src1,
			    const guchar *src2,
			    guchar       *dest,
			    guint         length)
{
  gint diff;

  while (length --)
    {
      diff = src1[0] - src2[0];
      dest[0] = (diff < 0) ? 0 : diff;

      dest[1] = src2[1];

      src1 += 1;
      src2 += 2;
      dest += 2;
    }
}


void
difference_pixels_gray_gray (const guchar *src1,
			     const guchar *src2,
			     guchar       *dest,
			     guint         length)
{
  gint diff;

  while (length --)
    {
      diff = src1[0] - src2[0];
      dest[0] = (diff < 0) ? -diff : diff;

      src1 += 1;
      src2 += 1;
      dest += 1;
    }
}


void
difference_pixels_gray_graya (const guchar *src1,
			      const guchar *src2,
			      guchar       *dest,
			      guint         length)
{
  gint diff;

  while (length --)
    {
      diff = src1[0] - src2[0];
      dest[0] = (diff < 0) ? -diff : diff;

      dest[1] = src2[1];

      src1 += 1;
      src2 += 2;
      dest += 2;
    }
}


void
scale_pixels_gray (const guchar *src,
		   guchar       *dest,
		   guint         length,
		   guint         scale)
{
  guint tmp;

  while (length --)
    {
      *dest++ = (guchar) INT_MULT (*src, scale, tmp);
      src++;
    }
}

void
add_alpha_pixels_gray (const guchar *src,
		       guchar       *dest,
		       guint         length)
{
  while (length --)
    {
      dest[0] = src[0];
      dest[1] = OPAQUE_OPACITY;

      src++;
      dest += 2;
    }
}


void
gray_to_rgb_pixels (const guchar *src,
		    guchar       *dest,
		    guint         length)
{
  while (length --)
    {
      dest[0] = src[0];
      dest[1] = src[0];
      dest[2] = src[0];

      src++;
      dest += 3;
    }
}


void
copy_gray_to_rgba_pixels (const guchar *src,
			  guchar       *dest,
			  guint         length)
{
  while (length --)
    {
      dest[0] = src[0];
      dest[1] = src[0];
      dest[2] = src[0];
      dest[3] = OPAQUE_OPACITY;

      src ++;
      dest += 4;
    }
}


void
initial_channel_pixels_gray (const guchar *src,
			     guchar       *dest,
			     guint         length)
{
  memset (dest, OPAQUE_OPACITY, length);
}


void
initial_pixels_gray (const guchar   *src,
		     guchar         *dest,
		     const guchar   *mask,
		     guint           opacity,
		     const gboolean *affect,
		     guint           length) 
{
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
   
     destp = dest;
     srcp = src;
     l = length;

     if (affect[0])
       {
	 while(l--)
	    {
	      *destp = *srcp;
	      srcp++;
	      destp += 2;
	    }
        }
      else
	{
	  while(l--)
	    {
	      *destp = 0;
	      destp += 2;
	    }
           opacity = 0;
        }
    
    /* fill the alpha channel */ 
 
    destp = dest + 1;
   
    if (opacity != 0)
      while (length--)
	{
	  *destp = INT_MULT(opacity , *m, tmp);
	  destp += 2;
	  m++;
	}
    else
      while (length--)
	{
	  *destp = 0;
	  destp += 2;
	}
  } 
  
  /* If no mask */
  else
    {
      /*  This function assumes the source has no alpha channel and
       *  the destination has an alpha channel.  So dest_bytes = bytes + 1
       */
      
      destp = dest;
      srcp = src;
      l = length;

      if (affect[0])
	{  
	  while(l--)
	    {
	      *destp = *srcp;
	      srcp++;
	      destp += 2;
	    }
	 }
      else
	{
	  while(l--)
	    {
	      *destp = 0;
	      destp += 2;
	    }
	  opacity = 0;
        }
      /* fill the alpha channel */ 

      destp = dest + 1;
    
      while (length--)
	{
	  *destp = 0;
	  destp += 2;
	}
    }
}

void
combine_pixels_gray_gray (const guchar   *src1,
			  const guchar   *src2,
			  guchar         *dest,
			  const guchar   *mask,
			  guint           opacity,
			  const gboolean *affect,
			  guint           length)
{
  const guchar * m;
  guint  new_alpha;
  gint   tmp;

  if (mask)
    {
      m = mask;
      while (length --)
	{
	  new_alpha = INT_MULT(*m, opacity, tmp);

	  dest[0] = (affect[0]) ?
	    INT_BLEND(src2[0], src1[0], new_alpha, tmp) : src1[0];

	  m++;
	  src1++;
	  src2++;
	  dest++;
	}
    }
  else
    {
      while (length --)
	{
	  dest[0] = (affect[0]) ?
	    INT_BLEND(src2[0], src1[0], opacity, tmp) : src1[0];

	  src1++;
	  src2++;
	  dest++;
	}
    }
}


void
combine_pixels_gray_graya (const guchar   *src1,
			   const guchar   *src2,
			   guchar         *dest,
			   const guchar   *mask,
			   guint           opacity,
			   const gboolean *affect,
			   guint           length)
{
  guint new_alpha;
  register gulong t1;

  if (mask)
    {
      while (length --)
	{
	  new_alpha = INT_MULT3(src2[1], *mask, opacity, t1);

	  dest[0] = (affect[0]) ?
	    INT_BLEND(src2[0], src1[0], new_alpha, t1) : src1[0];

	  mask++;
	  src1 += 1;
	  src2 += 2;
	  dest += 1;
	}
    }
  else
    {
      new_alpha = INT_MULT(src2[1], opacity, t1);
      
      while (length --)
      {
	dest[0] = (affect[0]) ?
	  INT_BLEND(src2[0] , src1[0] , new_alpha, t1) : src1[0];

	src1 += 1;
	src2 += 2;
	dest += 1;
      }
    }
}


void
replace_pixels_gray_gray (const guchar   *src1,
			  const guchar   *src2,
			  guchar         *dest,
			  const guchar   *mask,
			  guint           opacity,
			 const gboolean *affect,
			  guint           length)
{
  guint  tmp;

  if (mask)
    {
      guchar        mask_alpha;

      while (length --)
        {
	  mask_alpha = INT_MULT(*mask, opacity, tmp);
	  
	  dest[0] = (affect[0]) ?
	    INT_BLEND(src2[0], src1[0], mask_alpha, tmp) :
	    src1[0];

	  mask++;
	  src1++;
	  src2++;
	  dest++;
	}
    }
  else
    {
      static const guchar mask_alpha = OPAQUE_OPACITY;

      while (length --)
        {
	  dest[0] = (affect[0]) ?
	    INT_BLEND(src2[0], src1[0], mask_alpha, tmp) :
	    src1[0];

	  src1++;
	  src2++;
	  dest++;
	}
    }
}


void
replace_pixels_gray_graya (const guchar   *src1,
			   const guchar   *src2,
			   guchar         *dest,
			   const guchar   *mask,
			   guint           opacity,
			   const gboolean *affect,
			   guint           length)
{
  guint  tmp;

  if (mask)
    {
      guchar        mask_alpha;

      while (length --)
        {
	  mask_alpha = INT_MULT(*mask, opacity, tmp);
	  
	  dest[0] = (affect[0]) ?
	    INT_BLEND(src2[0], src1[0], mask_alpha, tmp) :
	    src1[0];

	  mask++;
	  src1++;
	  src2 += 2;
	  dest++;
	}
    }
  else
    {
      static const guchar mask_alpha = OPAQUE_OPACITY;

      while (length --)
        {
	  dest[0] = (affect[0]) ?
	    INT_BLEND(src2[0], src1[0], mask_alpha, tmp) :
	    src1[0];

	  src1++;
	  src2 += 2;
	  dest++;
	}
    }
}


void
extract_from_pixels_gray (guchar       *src,
			  guchar       *dest,
			  const guchar *mask,
			  const guchar *bg,
			  gboolean      cut,
			  guint         length)
{
  guint          tmp;

  if (mask)
    {
      memcpy (dest, mask, length); 
      
      if (cut)
	while (length --)
	  {
	    src[0] = INT_BLEND(bg[0], src[0], *mask, tmp);

	    mask++;
	    src++;
	  }
    } else
    {
      memset (dest, OPAQUE_OPACITY, length); 
      
      if (cut)
	while (length --)
	  {
	    src[0] = INT_BLEND(bg[0], src[0], OPAQUE_OPACITY, tmp);
	    src++;
	  }
    }
}


static void
expand_line_gray (gdouble           *dest,
		  gdouble           *src,
		  gint               old_width,
		  gint               width,
		  InterpolationType  interp)
{
  gdouble  ratio;
  gint     x;
  gint     src_col;
  gdouble  frac;
  gdouble *s;

  ratio = old_width / (gdouble) width;

/* this could be opimized much more by precalculating the coeficients for
   each x */
  switch(interp)
  {
    case CUBIC_INTERPOLATION:
      for (x = 0; x < width; x++)
      {
	src_col = ((int)((x) * ratio  + 2.0 - 0.5)) - 2;
	/* +2, -2 is there because (int) rounds towards 0 and we need 
	   to round down */
	frac =          ((x) * ratio - 0.5) - src_col;
	s = &src[src_col];
	dest[0] = cubic (frac, s[-1], s[0], s[1], s[2]);
	dest++;
      }
      break;
    case LINEAR_INTERPOLATION:
      for (x = 0; x < width; x++)
      {
	src_col = ((int)((x) * ratio + 2.0 - 0.5)) - 2;
	/* +2, -2 is there because (int) rounds towards 0 and we need 
	   to round down */
	frac =          ((x) * ratio - 0.5) - src_col;
	s = &src[src_col];
	dest[0] = ((s[1] - s[0]) * frac + s[0]);
	dest++;
      }
      break;
   case NEAREST_NEIGHBOR_INTERPOLATION:
     g_error("sampling_type can't be "
	     "NEAREST_NEIGHBOR_INTERPOLATION");
  }
}


static void
shrink_line_gray (gdouble           *dest,
		  gdouble           *src,
		  gint               old_width,
		  gint               width,
		  InterpolationType  interp)
{
  gint x;
  gdouble *source, *destp;
  register gdouble accum;
  register guint max;
  register gdouble mant, tmp;
  register const gdouble step = old_width / (gdouble) width;
  register const gdouble inv_step = 1.0 / step;
  gdouble position;

  source = &src[0];
  destp = &dest[0];
  position = -1;

  mant = *source;

  for (x = 0; x < width; x++)
    {
      source++;
      accum = 0;
      max = ((int)(position+step)) - ((int)(position));
      max--;
      do 
	{
	  accum += *source;
	  source ++;
	  max--;
	} while (max--);
      
      tmp = accum + mant;
      mant = ((position+step) - (int)(position + step));
      mant *= *source;
      tmp += mant;
      tmp *= inv_step;
      mant = *source - mant;
      *(destp) = tmp;
      destp ++;
      position += step;
	
    }
}

void
apply_layer_mode_gray_gray (apply_combine_layer_info *info)
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

  gint combine;
  combine = COMBINE_INTEN_INTEN;

  /*  assumes we're applying src2 TO src1  */
  switch (mode)
    {
      case NORMAL_MODE:
      case HUE_MODE:
      case SATURATION_MODE:
      case VALUE_MODE:
      case COLOR_MODE:
      case ERASE_MODE:
      case ANTI_ERASE_MODE:
	*dest = src2;
	break;

      case DISSOLVE_MODE:
	/*  Since dissolve requires an alpha channels...  */
	add_alpha_pixels_gray (src2, dest, length);

	dissolve_pixels_graya (src2, dest, x, y, opacity, length);
	combine = COMBINE_INTEN_INTEN_A;
	break;

      case MULTIPLY_MODE:
	multiply_pixels_gray_gray (src1, src2, dest, length);
	break;

      case DIVIDE_MODE:
	divide_pixels_gray_gray (src1, src2, dest, length);
	break;

      case SCREEN_MODE:
	screen_pixels_gray_gray (src1, src2, dest, length);
	break;

      case OVERLAY_MODE:
	overlay_pixels_gray_gray (src1, src2, dest, length);
	break;

      case DIFFERENCE_MODE:
	difference_pixels_gray_gray (src1, src2, dest, length);
	break;

      case ADDITION_MODE:
	add_pixels_gray_gray (src1, src2, dest, length);
	break;

      case SUBTRACT_MODE:
	subtract_pixels_gray_gray (src1, src2, dest, length);
	break;

      case DARKEN_ONLY_MODE:
	darken_pixels_gray_gray (src1, src2, dest, length);
	break;

      case LIGHTEN_ONLY_MODE:
	lighten_pixels_gray_gray (src1, src2, dest, length);
	break;

      case BEHIND_MODE:
	*dest = src2;
	combine = NO_COMBINATION;
	break;

      case REPLACE_MODE:
	*dest = src2;
	combine = REPLACE_INTEN;
	break;

      case DODGE_MODE:
	dodge_pixels_gray_gray (src1, src2, dest, length);
	break;

      case BURN_MODE:
	burn_pixels_gray_gray (src1, src2, dest, length);
	break;

      case HARDLIGHT_MODE:
	hardlight_pixels_gray_gray (src1, src2, dest, length);
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
apply_layer_mode_gray_graya (apply_combine_layer_info *info)
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
      case HUE_MODE: case SATURATION_MODE: case VALUE_MODE:
      case COLOR_MODE:
      case ERASE_MODE:
      case ANTI_ERASE_MODE:
	dest = src2;
	break;

      case DISSOLVE_MODE:
	dissolve_pixels_graya (src2, dest, x, y, opacity, length);
	break;

      case MULTIPLY_MODE:
	multiply_pixels_gray_graya (src1, src2, dest, length);
	break;

      case DIVIDE_MODE:
	divide_pixels_gray_graya (src1, src2, dest, length);
	break;

      case SCREEN_MODE:
	screen_pixels_gray_graya (src1, src2, dest, length);
	break;

      case OVERLAY_MODE:
	overlay_pixels_gray_graya (src1, src2, dest, length);
	break;

      case DIFFERENCE_MODE:
	difference_pixels_gray_graya (src1, src2, dest, length);
	break;

      case ADDITION_MODE:
	add_pixels_gray_graya (src1, src2, dest, length);
	break;

      case SUBTRACT_MODE:
	subtract_pixels_gray_graya (src1, src2, dest, length);
	break;

      case DARKEN_ONLY_MODE:
	darken_pixels_gray_graya (src1, src2, dest, length);
	break;

      case LIGHTEN_ONLY_MODE:
	lighten_pixels_gray_graya (src1, src2, dest, length);
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
	dodge_pixels_gray_graya (src1, src2, dest, length);
	break;

      case BURN_MODE:
	burn_pixels_gray_graya (src1, src2, dest, length);
	break;

      case HARDLIGHT_MODE:
	hardlight_pixels_gray_graya (src1, src2, dest, length);
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
