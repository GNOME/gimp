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
#include "paint_funcs_rgba.h"
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

static gint       random_table[RANDOM_TABLE_SIZE];
static guchar     no_mask = OPAQUE_OPACITY;
static gint       add_lut[256][256];


/*  Local function prototypes  */

/*static gint *   make_curve               (gdouble  sigma,
					  gint    *length);
static gdouble  cubic                    (gdouble   dx,
					  gint      jm1,
					  gint      j,
					  gint      jp1,
					  gint      jp2);
static void     rotate_pointers          (gpointer *p, 
					  guint32   n);
*/
/* MMX stuff */
extern gboolean use_mmx;

#ifdef HAVE_ASM_MMX
extern int use_mmx;

#define MMX_PIXEL_OP(x) \
void \
x( \
  const unsigned char *src1, \
  const unsigned char *src2, \
  unsigned count, \
  unsigned char *dst) __attribute((regparm(3)));

#define MMX_PIXEL_OP_3A_1A(op) \
  MMX_PIXEL_OP(op##_pixels_3a_3a) \
  MMX_PIXEL_OP(op##_pixels_1a_1a)

#define USE_MMX_PIXEL_OP_3A_1A(op) \
  if (use_mmx) \
    { \
      return op##_pixels_3a_3a(src1, src2, length, dest); \
    } 
#else

#define MMX_PIXEL_OP_3A_1A(op)
#define USE_MMX_PIXEL_OP_3A_1A(op)
#endif


void
update_tile_rowhints_rgba (Tile *tile,
			   gint  ymin,
			   gint  ymax)
{
  gint         ewidth;
  gint         x, y;
  guchar      *ptr;
  guchar       alpha;
  TileRowHint  thishint;

  tile_sanitize_rowhints (tile);

  ewidth = tile_ewidth (tile);

  ptr = tile_data_pointer (tile, 0, ymin);

  for (y = ymin; y <= ymax; y++)
    {
      thishint = tile_get_rowhint (tile, y);

      if (thishint == TILEROWHINT_UNKNOWN)
	{
	  alpha = ptr[3];

	  /* row is all-opaque or all-transparent? */
	  if (alpha == 0 || alpha == 255)
	    {
	      if (ewidth > 1)
		{
		  for (x = 1; x < ewidth; x++)
		    {
		      if (ptr[x * 4 + 3] != alpha)
			{
			  tile_set_rowhint (tile, y, TILEROWHINT_MIXED);
			  goto next_row4;
			}
		    }
		}
	      tile_set_rowhint (tile, y,
				(alpha == 0) ?
				TILEROWHINT_TRANSPARENT :
				TILEROWHINT_OPAQUE);
	    }
	  else
	    {
	      tile_set_rowhint (tile, y, TILEROWHINT_MIXED);
	    }
	}

    next_row4:
      ptr += 4 * ewidth;
    }
}


void
run_length_encode_rgba (const guchar *src,
			guint  *dest,
			guint   w)
{
  gint   start;
  gint   i;
  gint   j;
  guchar last;

  last = *src;
  src += 4;
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
      src += 4;
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
color_pixels_rgba (guchar       *dest,
		   const guchar *color,
		   gint          w)
{
#if defined(sparc) || defined(__sparc__)
  register guchar   c0, c1, c2, c3;
#else
  register guint32 *longd, longc;
#endif

#if defined(sparc) || defined(__sparc__)
  c0 = color[0];
  c1 = color[1];
  c2 = color[2];
  c3 = color[3];
  while (w--)
    {
      dest[0] = c0;
      dest[1] = c1;
      dest[2] = c2;
      dest[3] = c3;
      dest += 4;
    }
#else
  longc = ((guint32 *) color)[0];
  longd = (guint32 *) dest;
  while (w--)
    {
      *longd = longc;
      longd++;
    }
#endif /* sparc || __sparc__ */
}


void
blend_pixels_rgba (const guchar *src1,
		   const guchar *src2,
		   guchar       *dest,
		   gint          blend,
		   gint          w)
{
  gint   b;
  guchar blend2 = (255 - blend);

  while (w --)
    {
      for (b = 0; b < 4; b++)
	dest[b] = (src1[b] * blend2 + src2[b] * blend) / 255;

      src1 += 4;
      src2 += 4;
      dest += 4;
    }
}


void
shade_pixels_rgba (const guchar *src,
		   guchar       *dest,
		   const guchar *col,
		   gint          blend,
		   gint          w)
{
  gint   b;
  guchar blend2 = (255 - blend);

  while (w --)
    {
      for (b = 0; b < 3; b++)
	dest[b] = (src[b] * blend2 + col[b] * blend) / 255;

      dest[3] = src[3];  /* alpha channel */

      src += 4;
      dest += 4;
    }
}


void
extract_alpha_pixels_rgba (const guchar *src,
			   const guchar *mask,
			   guchar       *dest,
			   gint          w)
{
  const guchar *m;
  gint          tmp;

  if (mask)
    {
      m = mask;
      while (w --)
        {
          *dest++ = INT_MULT(src[3], *m, tmp);
          m++;
          src += 4;
        }
    }
  else
    { 
      while (w --)
        { 
          *dest++ = src[3];
          src += 4;
        }
    }
}

void
darken_pixels_rgba_rgb (const guchar *src1,
			const guchar *src2,
			guchar       *dest,
			guint         length)
{
  guchar s1, s2;

  while (length--)
    {
      s1 = src1[0];
      s2 = src2[0];
      dest[0] = (s1 < s2) ? s1 : s2;

      s1 = src1[1];
      s2 = src2[1];
      dest[1] = (s1 < s2) ? s1 : s2;
      
      s1 = src1[2];
      s2 = src2[2];
      dest[2] = (s1 < s2) ? s1 : s2;
      
      src1 += 4;
      src2 += 3;
      dest += 3;
    }
}

MMX_PIXEL_OP_3A_1A(darken)
void
darken_pixels_rgba_rgba (const guchar *src1,
			 const guchar *src2,
			 guchar       *dest,
			 guint         length)
{
  guchar s1, s2;

  USE_MMX_PIXEL_OP_3A_1A(darken)

  while (length--)
    {
      s1 = src1[0];
      s2 = src2[0];
      dest[0] = (s1 < s2) ? s1 : s2;

      s1 = src1[1];
      s2 = src2[1];
      dest[1] = (s1 < s2) ? s1 : s2;
      
      s1 = src1[2];
      s2 = src2[2];
      dest[2] = (s1 < s2) ? s1 : s2;
      
      dest[3] = MIN (src1[3], src2[3]);

      src1 += 4;
      src2 += 4;
      dest += 4;
    }
}


MMX_PIXEL_OP_3A_1A(lighten)
void
lighten_pixels_rgba_rgb (const guchar *src1,
			 const guchar *src2,
			 guchar       *dest,
			 guint         length)
{
  guchar s1, s2;

  USE_MMX_PIXEL_OP_3A_1A(lighten)

  while (length--)
    {
      s1 = src1[0];
      s2 = src2[0];
      dest[0] = (s1 < s2) ? s2 : s1;

      s1 = src1[1];
      s2 = src2[1];
      dest[1] = (s1 < s2) ? s2 : s1;

      s1 = src1[2];
      s2 = src2[2];
      dest[2] = (s1 < s2) ? s2 : s1;

      src1 += 4;
      src2 += 3;
      dest += 3;
    }
}


MMX_PIXEL_OP_3A_1A(lighten)
void
lighten_pixels_rgba_rgba (const guchar *src1,
			  const guchar *src2,
			  guchar       *dest,
			  guint         length)
{
  guchar s1, s2;

  USE_MMX_PIXEL_OP_3A_1A(lighten)

  while (length--)
    {
      s1 = src1[0];
      s2 = src2[0];
      dest[0] = (s1 < s2) ? s2 : s1;

      s1 = src1[1];
      s2 = src2[1];
      dest[1] = (s1 < s2) ? s2 : s1;

      s1 = src1[2];
      s2 = src2[2];
      dest[2] = (s1 < s2) ? s2 : s1;

      dest[3] = MIN (src1[3], src2[3]);

      src1 += 4;
      src2 += 4;
      dest += 4;
    }
}


void
hsv_only_pixels_rgba_rgba (const guchar *src1,
			   const guchar *src2,
			   guchar       *dest,
			   guint         mode,
			   guint         length)
{
  gint r1, g1, b1;
  gint r2, g2, b2;

  while (length--)
    {
      r1 = src1[0]; g1 = src1[1]; b1 = src1[2];
      r2 = src2[0]; g2 = src2[1]; b2 = src2[2];
      gimp_rgb_to_hsv_int (&r1, &g1, &b1);
      gimp_rgb_to_hsv_int (&r2, &g2, &b2);

      switch (mode)
	{
	case HUE_MODE:
	  r1 = r2;
	  break;
	case SATURATION_MODE:
	  g1 = g2;
	  break;
	case VALUE_MODE:
	  b1 = b2;
	  break;
	}

      /*  set the destination  */
      gimp_hsv_to_rgb_int (&r1, &g1, &b1);

      dest[0] = r1; dest[1] = g1; dest[2] = b1;

      dest[3] = MIN (src1[3], src2[3]);

      src1 += 4;
      src2 += 4;
      dest += 4;
    }
}


void
color_only_pixels_rgba_rgba (const guchar *src1,
			     const guchar *src2,
			     guchar       *dest,
			     guint         mode,
			     guint         length)
{
  gint r1, g1, b1;
  gint r2, g2, b2;

  while (length--)
    {
      r1 = src1[0]; g1 = src1[1]; b1 = src1[2];
      r2 = src2[0]; g2 = src2[1]; b2 = src2[2];
      gimp_rgb_to_hls_int (&r1, &g1, &b1);
      gimp_rgb_to_hls_int (&r2, &g2, &b2);

      /*  transfer hue and saturation to the source pixel  */
      r1 = r2;
      b1 = b2;

      /*  set the destination  */
      gimp_hls_to_rgb_int (&r1, &g1, &b1);

      dest[0] = r1; dest[1] = g1; dest[2] = b1;

      dest[3] = MIN (src1[3], src2[3]);

      src1 += 4;
      src2 += 4;
      dest += 4;
    }
}

MMX_PIXEL_OP_3A_1A(multiply)

void
multiply_pixels_rgba_rgba (const guchar *src1,
			   const guchar *src2,
			   guchar       *dest,
			   gint          length)
{
  gint tmp;

  USE_MMX_PIXEL_OP_3A_1A(multiply)

  while (length --)
    {
      dest[0] = INT_MULT(src1[0], src2[0], tmp);
      dest[1] = INT_MULT(src1[1], src2[1], tmp);
      dest[2] = INT_MULT(src1[2], src2[2], tmp);

      dest[3] = MIN (src1[3], src2[3]);

      src1 += 4;
      src2 += 4;
      dest += 4;
    }
}

void
multiply_pixels_rgba_rgb (const guchar *src1,
			  const guchar *src2,
			  guchar       *dest,
			  gint          length)
{
  gint tmp;

  while (length --)
    {
      dest[0] = INT_MULT(src1[0], src2[0], tmp);
      dest[1] = INT_MULT(src1[1], src2[1], tmp);
      dest[2] = INT_MULT(src1[2], src2[2], tmp);

      src1 += 4;
      src2 += 3;
      dest += 3;
    }
}


void
divide_pixels_rgba_rgb (const guchar *src1,
			const guchar *src2,
			guchar       *dest,
			guint         length)
{
  guint result;

  while (length --)
    {
      result = ((src1[0] * 256) / (1+src2[0]));
      dest[0] = MIN (result, 255);

      result = ((src1[1] * 256) / (1+src2[1]));
      dest[1] = MIN (result, 255);

      result = ((src1[2] * 256) / (1+src2[2]));
      dest[2] = MIN (result, 255);

      src1 += 4;
      src2 += 3;
      dest += 3;
    }
}


void
divide_pixels_rgba_rgba (const guchar *src1,
			 const guchar *src2,
			 guchar       *dest,
			 guint         length)
{
  guint result;

  while (length --)
    {
      result = ((src1[0] * 256) / (1+src2[0]));
      dest[0] = MIN (result, 255);

      result = ((src1[1] * 256) / (1+src2[1]));
      dest[1] = MIN (result, 255);

      result = ((src1[2] * 256) / (1+src2[2]));
      dest[2] = MIN (result, 255);

      dest[3] = MIN (src1[3], src2[3]);

      src1 += 4;
      src2 += 4;
      dest += 4;
    }
}


MMX_PIXEL_OP_3A_1A(screen)

void
screen_pixels_rgba_rgba (const guchar *src1,
			 const guchar *src2,
			 guchar       *dest,
			 guint         length)
{
  guint tmp;

  USE_MMX_PIXEL_OP_3A_1A(screen)

  while (length --)
    {
      dest[0] = 255 - INT_MULT((255 - src1[0]), (255 - src2[0]), tmp);
      dest[1] = 255 - INT_MULT((255 - src1[1]), (255 - src2[1]), tmp);
      dest[2] = 255 - INT_MULT((255 - src1[2]), (255 - src2[2]), tmp);

      dest[3] = MIN (src1[3], src2[3]);

      src1 += 4;
      src2 += 4;
      dest += 4;
    }
}

void
screen_pixels_rgba_rgb (const guchar *src1,
			const guchar *src2,
			guchar       *dest,
			guint         length)
{
  guint tmp;

  while (length --)
    {
      dest[0] = 255 - INT_MULT((255 - src1[0]), (255 - src2[0]), tmp);
      dest[1] = 255 - INT_MULT((255 - src1[1]), (255 - src2[1]), tmp);
      dest[2] = 255 - INT_MULT((255 - src1[2]), (255 - src2[2]), tmp);

      src1 += 4;
      src2 += 3;
      dest += 3;
    }
}


MMX_PIXEL_OP_3A_1A(overlay)

void
overlay_pixels_rgba_rgba (const guchar *src1,
			  const guchar *src2,
			  guchar       *dest,
			  guint          length)
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

      dest[3] = MIN (src1[3], src2[3]);

      src1 += 4;
      src2 += 4;
      dest += 4;
    }
}

void
overlay_pixels_rgba_rgb (const guchar *src1,
			 const guchar *src2,
			 guchar       *dest,
			 guint          length)
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

      src1 += 4;
      src2 += 3;
      dest += 3;
    }
}


void
dodge_pixels_rgba_rgba (const guchar *src1,
		        const guchar *src2,
		        guchar	*dest,
		        guint	 length)
{
  guint tmp1;

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

      dest[3] = MIN (src1[3], src2[3]);

      src1 += 4;
      src2 += 4;
      dest += 4;
    }
}

void
dodge_pixels_rgba_rgb (const guchar *src1,
		       const guchar *src2,
		       guchar	*dest,
		       guint	 length)
{
  guint tmp1;

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

      src1 += 4;
      src2 += 3;
      dest += 3;
    }
}

void
burn_pixels_rgba_rgb (const guchar *src1,
		      const guchar *src2,
		      guchar       *dest,
		      guint	     length)
{
  guint tmp1;

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

      src1 += 4;
      src2 += 3;
      dest += 3;
    }
}

void
burn_pixels_rgba_rgba (const guchar *src1,
		       const guchar *src2,
		       guchar       *dest,
		       guint	     length)
{
  guint tmp1;

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

      dest[3] = MIN (src1[3], src2[3]);

      src1 += 4;
      src2 += 4;
      dest += 4;
    }
}

void
hardlight_pixels_rgba_rgba (const guchar *src1,
			    const guchar *src2,
			    guchar	 *dest,
			    guint         length)
{
  guint tmp1;

  while (length --)
    {
      if (src2[0] > 128)
      {
	tmp1 = ((gint)255 - src1[0]) * ((gint)255 - ((src2[0] - 128) << 1));
	dest[0] = (guchar) CLAMP (255 - (tmp1 >> 8), 0, 255);
      } else 
      {
	tmp1 = (gint)src1[0] * ((gint)src2[0] << 1);
	dest[0] = (guchar) CLAMP (tmp1 >> 8, 0, 255);
      }
	
      if (src2[1] > 128) {
	tmp1 = ((gint)255 - src1[1]) * ((gint)255 - ((src2[1] - 128) << 1));
	dest[1] = (guchar) CLAMP (255 - (tmp1 >> 8), 0, 255);
      } else
      {
	tmp1 = (gint)src1[1] * ((gint)src2[1] << 1);
	dest[1] = (guchar) CLAMP (tmp1 >> 8, 0, 255);
      }

      if (src2[2] > 128) {
	tmp1 = ((gint)255 - src1[2]) * ((gint)255 - ((src2[2] - 128) << 1));
	dest[2] = (guchar) CLAMP (255 - (tmp1 >> 8), 0, 255);
      } else
      {
	tmp1 = (gint)src1[2] * ((gint)src2[2] << 1);
	dest[2] = (guchar) CLAMP (tmp1 >> 8, 0, 255);
      }

      dest[3] = MIN (src1[3], src2[3]);

      src1 += 4;
      src2 += 4;
      dest += 4;
    }
}

void
hardlight_pixels_rgba_rgb (const guchar *src1,
			   const guchar *src2,
			   guchar	*dest,
			   guint         length)
{
  guint tmp1;

  while (length --)
    {
      if (src2[0] > 128) {
	tmp1 = ((gint)255 - src1[0]) * ((gint)255 - ((src2[0] - 128) << 1));
	dest[0] = (guchar) CLAMP (255 - (tmp1 >> 8), 0, 255);
      } else
      {
	tmp1 = (gint)src1[0] * ((gint)src2[0] << 1);
	dest[0] = (guchar) CLAMP (tmp1 >> 8, 0, 255);
      }

      if (src2[1] > 128) {
	tmp1 = ((gint)255 - src1[1]) * ((gint)255 - ((src2[1] - 128) << 1));
	dest[1] = (guchar) CLAMP (255 - (tmp1 >> 8), 0, 255);
      } else
      {
	tmp1 = (gint)src1[1] * ((gint)src2[1] << 1);
	dest[1] = (guchar) CLAMP (tmp1 >> 8, 0, 255);
      }

      if (src2[2] > 128) {
	tmp1 = ((gint)255 - src1[2]) * ((gint)255 - ((src2[2] - 128) << 1));
	dest[2] = (guchar) CLAMP (255 - (tmp1 >> 8), 0, 255);
      } else
      {
	tmp1 = (gint)src1[2] * ((gint)src2[2] << 1);
	dest[2] = (guchar) CLAMP (tmp1 >> 8, 0, 255);
      }

      src1 += 4;
      src2 += 3;
      dest += 3;
    }
}


MMX_PIXEL_OP_3A_1A(add)

void
add_pixels_rgba_rgba (const guchar *src1,
		      const guchar *src2,
		      guchar       *dest,
		      guint         length)
{
  USE_MMX_PIXEL_OP_3A_1A(add)

  while (length --)
    {
      dest[0] = add_lut[(src1[0])] [(src2[0])];
      dest[1] = add_lut[(src1[1])] [(src2[1])];
      dest[2] = add_lut[(src1[2])] [(src2[2])];

      dest[3] = MIN (src1[3], src2[3]);

      src1 += 4;
      src2 += 4;
      dest += 4;
    }
}

void
add_pixels_rgba_rgb (const guchar *src1,
		     const guchar *src2,
		     guchar       *dest,
		     guint         length)
{
  while (length --)
    {
      dest[0] = add_lut[(src1[0])] [(src2[0])];
      dest[1] = add_lut[(src1[1])] [(src2[1])];
      dest[2] = add_lut[(src1[2])] [(src2[2])];

      src1 += 4;
      src2 += 3;
      dest += 3;
    }
}


MMX_PIXEL_OP_3A_1A(substract)

void
subtract_pixels_rgba_rgba (const guchar *src1,
			   const guchar *src2,
			   guchar       *dest,
			   guint         length)
{
  gint diff;

  USE_MMX_PIXEL_OP_3A_1A(substract)

  while (length --)
    {
      diff = src1[0] - src2[0];
      dest[0] = (diff < 0) ? 0 : diff;
      diff = src1[1] - src2[1];
      dest[1] = (diff < 0) ? 0 : diff;
      diff = src1[2] - src2[2];
      dest[2] = (diff < 0) ? 0 : diff;

      dest[3] = MIN (src1[3], src2[3]);

      src1 += 4;
      src2 += 4;
      dest += 4;
    }
}

void
subtract_pixels_rgba_rgb (const guchar *src1,
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

      src1 += 4;
      src2 += 3;
      dest += 3;
    }
}


MMX_PIXEL_OP_3A_1A(difference)

void
difference_pixels_rgba_rgba (const guchar *src1,
			     const guchar *src2,
			     guchar       *dest,
			     guint         length)
{
  gint diff;

  USE_MMX_PIXEL_OP_3A_1A(difference)

  while (length --)
    {
      diff = src1[0] - src2[0];
      dest[0] = (diff < 0) ? -diff : diff;
      diff = src1[1] - src2[1];
      dest[1] = (diff < 0) ? -diff : diff;
      diff = src1[2] - src2[2];
      dest[2] = (diff < 0) ? -diff : diff;

      dest[3] = MIN (src1[3], src2[3]);

      src1 += 4;
      src2 += 4;
      dest += 4;
    }
}

void
difference_pixels_rgba_rgb (const guchar *src1,
			    const guchar *src2,
			    guchar       *dest,
			    guint         length)
{
  gint diff;

  USE_MMX_PIXEL_OP_3A_1A(difference)

  while (length --)
    {
      diff = src1[0] - src2[0];
      dest[0] = (diff < 0) ? -diff : diff;
      diff = src1[1] - src2[1];
      dest[1] = (diff < 0) ? -diff : diff;
      diff = src1[2] - src2[2];
      dest[2] = (diff < 0) ? -diff : diff;

      src1 += 4;
      src2 += 3;
      dest += 3;
    }
}

void
dissolve_pixels_rgba (const guchar *src,
		      guchar	   *dest,
		      guint	    x,
		      guint	    y,
		      guint	    opacity,
		      guint	    length)
{
  gint b;
  gint rand_val;
  
#if defined(ENABLE_MP) && defined(__GLIBC__)
  /* The glibc 2.1 documentation recommends using the SVID random functions
   * instead of rand_r
   */
  struct drand48_data seed;
  glong               temp_val;

  srand48_r (random_table[y % RANDOM_TABLE_SIZE], &seed);
  for (b = 0; b < x; b++)
    lrand48_r (&seed, &temp_val);
#elif defined(ENABLE_MP) && !defined(__GLIBC__)
  /* If we are running with multiple threads rand_r give _much_ better
   * performance than rand
   */
  guint seed;
  seed = random_table[y % RANDOM_TABLE_SIZE];
  for (b = 0; b < x; b++)
    rand_r (&seed);
#else
  /* Set up the random number generator */
  srand (random_table[y % RANDOM_TABLE_SIZE]);
  for (b = 0; b < x; b++)
    rand ();
#endif

  while (length--)
    {
      /*  preserve the intensity values  */
      for (b = 0; b < 3; b++)
	dest[b] = src[b];

      /*  dissolve if random value is > opacity  */
#if defined(ENABLE_MP) && defined(__GLIBC__)
      lrand48_r (&seed, &temp_val);
      rand_val = temp_val & 0xff;
#elif defined(ENABLE_MP) && !defined(__GLIBC__)
      rand_val = (rand_r (&seed) & 0xff);
#else
      rand_val = (rand () & 0xff);
#endif
      dest[3] = (rand_val > src[3]) ? 0 : src[3];

      dest += 4;
      src  += 4;
    }
}

void
replace_pixels_rgba (const guchar *src1,
		     const guchar *src2,
		     guchar	  *dest,
		     const guchar *mask,
		     guint	   length,
		     guint	   opacity,
		     gboolean     *affect)
{
  gint    b;
  gdouble a_val, a_recip, mask_val;
  gint    s1_a, s2_a;
  gint    new_val;
  const gdouble norm_opacity = opacity * (1.0 / 65536.0);

  while (length --)
    {
      mask_val = mask[0] * norm_opacity;
      /* calculate new alpha first. */
      s1_a = src1[3];
      s2_a = src2[3];
      a_val = s1_a + mask_val * (s2_a - s1_a);
 
      if (a_val == 0) /* In any case, write out versions of the blending function */
                      /* that result when combinations of s1_a, s2_a, and         */
	              /* mask_val --> 0 (or mask_val -->1)                        */
        {
          /* Case 1: s1_a, s2_a, AND mask_val all approach 0+:               */
	  /* Case 2: s1_a AND s2_a both approach 0+, regardless of mask_val: */

          if (s1_a + s2_a == 0.0)
            {
              for (b = 0; b < 3; b++)
	        {
                  new_val = 0.5 + (gdouble) src1[b] + 
		    mask_val * ((gdouble) src2[b] - (gdouble) src1[b]);
 
                  dest[b] = affect[b] ? MIN (new_val, 255) : src1[b];
                }
            }

	  /* Case 3: mask_val AND s1_a both approach 0+, regardless of s2_a  */
          else if (s1_a + mask_val == 0.0)
            {
              for (b = 0; b < 3; b++)
	        {
                  dest[b] = src1[b]; 
                }
            }

	  /* Case 4: mask_val -->1 AND s2_a -->0, regardless of s1_a         */
          else if (1.0 - mask_val + s2_a == 0.0)
            {
              for (b = 0; b < 3; b++)
	        {
                  dest[b] = affect[b] ? src2[b] : src1[b]; 
                }
            }
	}
      else
	{
	  a_recip = 1.0 / a_val;
          /* possible optimization: fold a_recip into s1_a and s2_a              */
          for (b = 0; b < 3; b++)
	    {
	      new_val = 0.5 + a_recip * (src1[b] * s1_a + mask_val *
		 		        (src2[b] * s2_a - src1[b] * s1_a));
	      dest[b] = affect[b] ? MIN (new_val, 255) : src1[b];
            }
        }

      dest[3] = affect[3] ? a_val + 0.5: s1_a;
      src1 += 4;
      src2 += 4;
      dest += 4;
      mask++;
    }
}


void
flatten_pixels_rgba (const guchar *src,
		     guchar	  *dest,
		     const guchar *bg,
		     guint	   length)
{
  gint b;
  gint t1, t2;

  while (length --)
    {
      for (b = 0; b < 3; b++)
	dest[b] = INT_MULT (src[b], src[3], t1) + 
	          INT_MULT (bg[b], (255 - src[3]), t2);

      src += 4;
      dest += 3;
    }
}


void
apply_mask_to_alpha_channel_rgba (guchar       *src,
				  const guchar *mask,
				  guint		opacity,
				  guint		length)
{
  gulong tmp;

  src += 3;

  if (opacity == 255)
    {
      while (length --)
	{
	  *src = INT_MULT(*src, *mask, tmp);
	  mask++;
	  src += 4;
	}
    }
  else
    {
      while (length --)
	{
	  *src = INT_MULT3(*src, *mask, opacity, tmp);
	  mask++;
	  src += 4;
	}
    }
}


void
combine_mask_and_alpha_channel_rgba (guchar       *src,
				     const guchar *mask,
				     guint	   opacity,
				     guint	   length)
{
  guint mask_val;
  guint tmp;

  src += 3;

  if (opacity != 255)
    while (length --)
    {
      mask_val = INT_MULT(*mask, opacity, tmp);
      mask++;
      *src = *src + INT_MULT((255 - *src) , mask_val, tmp);
      src += 4;
    }
  else
    while (length --)
    {
      *src = *src + INT_MULT((255 - *src) , *mask, tmp);
      src += 4;
      mask++;
    }
}


void
initial_channel_pixels_rgba (const guchar *src,
			     guchar       *dest,
			     guint	   length)
{
  while (length --)
    {
      dest[0] = src[0];
      dest[1] = src[0];
      dest[2] = src[0];

      dest[3] = OPAQUE_OPACITY;

      dest += 4;
      src ++;
    }
}

void
initial_pixels_rgba (const guchar   *src,
		     guchar	    *dest,
		     const guchar   *mask,
		     guint	     opacity,
		     const gboolean *affect,
		     guint	     length)
{
  gint          b;
  const guchar *m;
  glong         tmp;

  if (mask)
    {
      m = mask;
      while (length --)
	{
	  for (b = 0; b < 3; b++)
	    dest[b] = src[b] * affect[b];
	  
	  /*  Set the alpha channel  */
	  dest[3] = affect [3] ? INT_MULT3(opacity, src[3], *m, tmp) : 0;
	  
	  m++;
	  
	  dest += 4;
	  src += 4;
	}
    }
  else
    {
      while (length --)
	{
	  for (b = 0; b < 3; b++)
	    dest[b] = src[b] * affect[b];
	  
	  /*  Set the alpha channel  */
	  dest[3] = affect [3] ? INT_MULT(opacity , src[3], tmp) : 0;
	  
	  dest += 4;
	  src += 4;
	}
    }
}


void
combine_rgba_and_indexeda_pixels (apply_combine_layer_info *info)
{
  const guchar *src1 = info->src1;
  const guchar *src2 = info->src2;
  guchar       *dest = info->dest;
  const guchar *mask = info->mask;
  const guchar *cmap = info->coldata;
  guint		opacity = info->opacity;
  guint		length = info->length;
  
  guchar new_alpha;
  guint  index;
  glong  tmp;

  if (mask)
    {
      while (length --)
	{
	  new_alpha = INT_MULT3(src2[1], *mask, opacity, tmp);

	  index = src2[0] * 3;

	  dest[0] = (new_alpha > 127) ? cmap[index + 0] : src1[0];
	  dest[1] = (new_alpha > 127) ? cmap[index + 1] : src1[1];
	  dest[2] = (new_alpha > 127) ? cmap[index + 2] : src1[2];

	  dest[3] = (new_alpha > 127) ? OPAQUE_OPACITY : src1[3];  
	  /*  alpha channel is opaque  */

	  mask++;

	  src1 += 4;
	  src2 += 2;
	  dest += 4;
	}
    }
  else
    {
      while (length --)
	{
	  new_alpha = INT_MULT(src2[1], opacity, tmp);

	  index = src2[0] * 3;

	  dest[0] = (new_alpha > 127) ? cmap[index + 0] : src1[0];
	  dest[1] = (new_alpha > 127) ? cmap[index + 1] : src1[1];
	  dest[2] = (new_alpha > 127) ? cmap[index + 2] : src1[2];

	  dest[3] = (new_alpha > 127) ? OPAQUE_OPACITY : src1[3];  
	  /*  alpha channel is opaque  */

	  src1 += 4;
	  src2 += 2;
	  dest += 4;
	}
    }
}

#define alphify(src2_alpha,new_alpha) \
	if (src2_alpha != 0 && new_alpha != 0)							\
	  {											\
            b = 3; \
	    if (src2_alpha == new_alpha){							\
	      do { \
	      b--; dest [b] = affect [b] ? src2 [b] : src1 [b];} while (b);	\
	    } else {										\
	      ratio = (float) src2_alpha / new_alpha;						\
	      compl_ratio = 1.0 - ratio;							\
	  											\
              do { b--; \
	        dest[b] = affect[b] ?								\
	          (guchar) (src2[b] * ratio + src1[b] * compl_ratio + EPSILON) : src1[b];\
         	  } while (b); \
	    }    \
	  }

void
combine_rgba_and_rgb_pixels (const guchar   *src1,
			     const guchar   *src2,
			     guchar         *dest,
			     const guchar   *mask,
			     guint           opacity,
			     const gboolean *affect,
			     guint           mode_affect,  /*  how does the combination mode affect alpha?  */
			     guint           length)
{
  guint         b;
  guchar        src2_alpha;
  guchar        new_alpha;
  const guchar *m;
  gfloat        ratio, compl_ratio;
  glong         tmp;

  if (mask)
    {
      m = mask;
      if (opacity == OPAQUE_OPACITY) /* HAS MASK, FULL OPACITY */
	{
	  while (length--)
	    {
	      src2_alpha = *m;
	      new_alpha = src1[3] +
		INT_MULT((255 - src1[3]), src2_alpha, tmp);
	      alphify (src2_alpha, new_alpha);
	      
	      if (mode_affect)
		{
		  dest[3] = (affect[3]) ? new_alpha : src1[3];
		}
	      else
		{
		  dest[3] = (src1[3]) ? src1[3] :
		    (affect[3] ? new_alpha : src1[3]);
		}
		
	      m++;
	      src1 += 4;
	      src2 += 3;
	      dest += 4;
	    }
	}
      else /* HAS MASK, SEMI-OPACITY */
	{
	  while (length--)
	    {
	      src2_alpha = INT_MULT(*m, opacity, tmp);
	      new_alpha = src1[3] +
		INT_MULT((255 - src1[3]), src2_alpha, tmp);
	      alphify (src2_alpha, new_alpha);
	      
	      if (mode_affect)
		{
		  dest[3] = (affect[3]) ? new_alpha : src1[3];
		}
	      else
		{
		  dest[3] = (src1[3]) ? src1[3] :
		    (affect[3] ? new_alpha : src1[3]);
		}
		
	      m++;
	      src1 += 4;
	      src2 += 3;
	      dest += 4;
	    }
	}
    }
  else /* NO MASK */
    {
      while (length --)
	{
	  src2_alpha = opacity;
	  new_alpha = src1[3] +
	    INT_MULT((255 - src1[3]), src2_alpha, tmp);
	  alphify (src2_alpha, new_alpha);
	  
	  if (mode_affect)
	    dest[3] = (affect[3]) ? new_alpha : src1[3];
	  else
	    dest[3] = (src1[3]) ? src1[3] : (affect[3] ? new_alpha : src1[3]);

	    src1 += 4;
	    src2 += 3;
	    dest += 4;
	}
    }
}

void
combine_rgba_and_rgba_pixels (const guchar   *src1,
			      const guchar   *src2,
			      guchar         *dest,
			      const guchar   *mask,
			      guint	      opacity,
			      const gboolean *affect,
			      guint	      mode_affect,  /*  how does the combination mode affect alpha?  */
			      guint	      length)
{
  gint b;
  guchar src2_alpha;
  guchar new_alpha;
  const guchar * m;
  gfloat ratio, compl_ratio;
  glong tmp;

  if (mask)
    {
      m = mask;

      if (opacity == OPAQUE_OPACITY) /* HAS MASK, FULL OPACITY */
	{
	  const gint* mask_ip;
	  gint i,j;

	  if (length >= sizeof(int))
	    {
	      /* HEAD */
	      i =  (((int)m) & (sizeof(int)-1));
	      if (i != 0)
		{
		  i = sizeof(int) - i;
		  length -= i;
		  while (i--)
		    {
		      /* GUTS */
		      src2_alpha = INT_MULT(src2[3], *m, tmp);
		      new_alpha = src1[3] +
			INT_MULT((255 - src1[3]), src2_alpha, tmp);
		  
		      alphify (src2_alpha, new_alpha);
		  
		      if (mode_affect)
			{
			  dest[3] = (affect[3]) ? new_alpha : src1[3];
			}
		      else
			{
			  dest[3] = (src1[3]) ? src1[3] :
			    (affect[3] ? new_alpha : src1[3]);      
			}
		  
		      m++;
		      src1 += 4;
		      src2 += 4;
		      dest += 4;	
		      /* GUTS END */
		    }
		}

	      /* BODY */
	      mask_ip = (int*)m;
	      i = length / sizeof(int);
	      length %= sizeof(int);
	      while (i--)
		{
		  if (*mask_ip)
		    {
		      m = (const guchar*)mask_ip;
		      j = sizeof(int);
		      while (j--)
			{
			  /* GUTS */
			  src2_alpha = INT_MULT(src2[3], *m, tmp);
			  new_alpha = src1[3] +
			    INT_MULT((255 - src1[3]), src2_alpha, tmp);
		  
			  alphify (src2_alpha, new_alpha);
		  
			  if (mode_affect)
			    {
			      dest[3] = (affect[3]) ? new_alpha : src1[3];
			    }
			  else
			    {
			      dest[3] = (src1[3]) ? src1[3] :
				(affect[3] ? new_alpha : src1[3]);      
			    }

			  m++;
			  src1 += 4;
			  src2 += 4;
			  dest += 4;
			  /* GUTS END */
			}
		    }
		  else
		    {
		      j = 4 * sizeof(int);
		      src2 += j;
		      while (j--)
			{
			  *(dest++) = *(src1++);
			}
		    }
		  mask_ip++;
		}

	      m = (const guchar*)mask_ip;
	    }

	  /* TAIL */
	  while (length--)
	    {
	      /* GUTS */
	      src2_alpha = INT_MULT(src2[3], *m, tmp);
	      new_alpha = src1[3] +
		INT_MULT((255 - src1[3]), src2_alpha, tmp);
		  
	      alphify (src2_alpha, new_alpha);
		  
	      if (mode_affect)
		{
		  dest[3] = (affect[3]) ? new_alpha : src1[3];
		}
	      else
		{
		  dest[3] = (src1[3]) ? src1[3] :
		    (affect[3] ? new_alpha : src1[3]);      
		}
		  
	      m++;
	      src1 += 4;
	      src2 += 4;
	      dest += 4;	
	      /* GUTS END */
	    }
	}
      else /* HAS MASK, SEMI-OPACITY */
	{
	  const gint* mask_ip;
	  gint i,j;

	  if (length >= sizeof(int))
	    {
	      /* HEAD */
	      i = (((int)m) & (sizeof(int)-1));
	      if (i != 0)
		{
		  i = sizeof(int) - i;
		  length -= i;
		  while (i--)
		    {
		      /* GUTS */
		      src2_alpha = INT_MULT3(src2[3], *m, opacity, tmp);
		      new_alpha = src1[3] +
			INT_MULT((255 - src1[3]), src2_alpha, tmp);
		  
		      alphify (src2_alpha, new_alpha);
		  
		      if (mode_affect)
			{
			  dest[3] = (affect[3]) ? new_alpha : src1[3];
			}
		      else
			{
			  dest[3] = (src1[3]) ? src1[3] :
			    (affect[3] ? new_alpha : src1[3]);
			}
		  
		      m++;
		      src1 += 4;
		      src2 += 4;
		      dest += 4;
		      /* GUTS END */
		    }
		}
	  
	      /* BODY */
	      mask_ip = (int*)m;
	      i = length / sizeof(int);
	      length %= sizeof(int);
	      while (i--)
		{
		  if (*mask_ip)
		    {
		      m = (const guchar*)mask_ip;
		      j = sizeof(int);
		      while (j--)
			{
			  /* GUTS */
			  src2_alpha = INT_MULT3(src2[3], *m, opacity, tmp);
			  new_alpha = src1[3] +
			    INT_MULT((255 - src1[3]), src2_alpha, tmp);
		      
			  alphify (src2_alpha, new_alpha);
		      
			  if (mode_affect)
			    {
			      dest[3] = (affect[3]) ? new_alpha : src1[3];
			    }
			  else
			    {
			      dest[3] = (src1[3]) ? src1[3] :
				(affect[3] ? new_alpha : src1[3]);
			    }

			  m++;
			  src1 += 4;
			  src2 += 4;
			  dest += 4;
			  /* GUTS END */
			}
		    }
		  else
		    {
		      j = 4 * sizeof(int);
		      src2 += j;
		      while (j--)
			{
			  *(dest++) = *(src1++);
			}
		    }
		  mask_ip++;
		}

	      m = (const guchar*)mask_ip;
	    }
	
	  /* TAIL */
	  while (length--)
	    {
	      /* GUTS */
	      src2_alpha = INT_MULT3(src2[3], *m, opacity, tmp);
	      new_alpha = src1[3] +
		INT_MULT((255 - src1[3]), src2_alpha, tmp);
	      
	      alphify (src2_alpha, new_alpha);
	      
	      if (mode_affect)
		{
		  dest[3] = (affect[3]) ? new_alpha : src1[3];
		}
	      else
		{
		  dest[3] = (src1[3]) ? src1[3] :
		    (affect[3] ? new_alpha : src1[3]);
		}
	      
	      m++;
	      src1 += 4;
	      src2 += 4;
	      dest += 4;
	      /* GUTS END */
	    }
	}
    }
  else
    {
      if (opacity == OPAQUE_OPACITY) /* NO MASK, FULL OPACITY */
	{
	  while (length --)
	    {
	      src2_alpha = src2[3];
	      new_alpha = src1[3] +
		INT_MULT((255 - src1[3]), src2_alpha, tmp);
	      
	      alphify (src2_alpha, new_alpha);
	      
	      if (mode_affect)
		{
		  dest[3] = (affect[3]) ? new_alpha : src1[3];
		}
	      else
		{
		  dest[3] = (src1[3]) ? src1[3] :
		    (affect[3] ? new_alpha : src1[3]);
		}
		
	      src1 += 4;
	      src2 += 4;
	      dest += 4;
	    }
	}
      else /* NO MASK, SEMI OPACITY */
	{
	  while (length --)
	    {
	      src2_alpha = INT_MULT(src2[3], opacity, tmp);
	      new_alpha = src1[3] +
		INT_MULT((255 - src1[3]), src2_alpha, tmp);
	      
	      alphify (src2_alpha, new_alpha);
	      
	      if (mode_affect)
		{
		  dest[3] = (affect[3]) ? new_alpha : src1[3];
		}
	      else
		{
		  dest[3] = (src1[3]) ? src1[3] :
		    (affect[3] ? new_alpha : src1[3]);
		}
		
	      src1 += 4;
	      src2 += 4;
	      dest += 4;	  
	    }
	}
    }
}
#undef alphify


void
combine_rgba_and_channel_mask_pixels (apply_combine_layer_info *info)
{
  const guchar *src = info->src1;
  const guchar *channel = info->src2;
  const guchar *col = info->coldata;
  guchar *dest = info->dest;
  guint opacity = info->opacity;
  guint length = info->length;

  guint  channel_alpha;
  guchar new_alpha;
  guchar compl_alpha;
  gint   t, s;

  while (length --)
    {
      channel_alpha = INT_MULT (255 - *channel, opacity, t);
      if (channel_alpha)
	{
	  new_alpha = src[3] + INT_MULT ((255 - src[3]), channel_alpha, t);

	  if (new_alpha != 255)
	    channel_alpha = (channel_alpha * 255) / new_alpha;
	  compl_alpha = 255 - channel_alpha;

	  dest[0] = INT_MULT (col[0], channel_alpha, t) 
	    	    + INT_MULT (src[0], compl_alpha, s);
	  dest[1] = INT_MULT (col[1], channel_alpha, t) 
	    	    + INT_MULT (src[1], compl_alpha, s);
	  dest[2] = INT_MULT (col[2], channel_alpha, t) 
	    	    + INT_MULT (src[2], compl_alpha, s);
	  dest[3] = new_alpha;
	}
      else
	{
	  dest[0] = src[0];
	  dest[1] = src[1];
	  dest[2] = src[2];
	  dest[3] = src[3];
	}

      /*  advance pointers  */
      src += 4;
      dest += 4;
      channel++;
    }
}


void
combine_rgba_and_channel_selection_pixels (apply_combine_layer_info *info)
{
  const guchar *src = info->src1;
  const guchar *channel = info->src2;
  const guchar *col = info->coldata;
  guchar *dest = info->dest;
  guint opacity = info->opacity;
  guint length = info->length;

  guchar channel_alpha;
  guchar new_alpha;
  guchar compl_alpha;
  gint   t, s;

  while (length --)
    {
      channel_alpha = INT_MULT (*channel, opacity, t);
      if (channel_alpha)
	{
	  new_alpha = src[3] + INT_MULT ((255 - src[3]), channel_alpha, t);

	  if (new_alpha != 255)
	    channel_alpha = (channel_alpha * 255) / new_alpha;
	  compl_alpha = 255 - channel_alpha;

	  dest[0] = INT_MULT (col[0], channel_alpha, t) +
	    INT_MULT (src[0], compl_alpha, s);
	  dest[1] = INT_MULT (col[1], channel_alpha, t) +
	    INT_MULT (src[1], compl_alpha, s);
	  dest[2] = INT_MULT (col[2], channel_alpha, t) +
	    INT_MULT (src[2], compl_alpha, s);
	  dest[3] = new_alpha;
	}
      else
	{
	  dest[0] = src[0];
	  dest[1] = src[1];
	  dest[2] = src[2];
	  dest[3] = src[3];
	}

      /*  advance pointers  */
      src += 4;
      dest += 4;
      channel++;
    }
}


void
behind_rgba_pixels (const guchar   *src1,
		    const guchar   *src2,
		    guchar         *dest,
		    const guchar   *mask,
		    guint           opacity,
		    const gboolean *affect,
		    guint           length,
		    guint           bytes2,
		    gboolean        has_alpha2)
{
  gint          b;
  guchar        src1_alpha;
  guchar        src2_alpha;
  guchar        new_alpha;
  const guchar *m;
  gfloat        ratio, compl_ratio;
  glong         tmp;

  if (mask)
    m = mask;
  else
    m = &no_mask;

  /*  the alpha channel  */

  while (length --)
    {
      src1_alpha = src1[3];
      src2_alpha = INT_MULT3(src2[3], *m, opacity, tmp);
      new_alpha = src2_alpha +
	INT_MULT((255 - src2_alpha), src1_alpha, tmp);
      if (new_alpha)
	ratio = (float) src1_alpha / new_alpha;
      else
	ratio = 0.0;
      compl_ratio = 1.0 - ratio;

      for (b = 0; b < 3; b++)
	dest[b] = (affect[b]) ?
	  (guchar) (src1[b] * ratio + src2[b] * compl_ratio + EPSILON) :
	src1[b];

      dest[3] = (affect[3]) ? new_alpha : src1[3];

      if (mask)
	m++;

      src1 += 4;
      src2 += bytes2;
      dest += 4;
    }
}


void
replace_rgba_pixels (const guchar   *src1,
		     const guchar   *src2,
		     guchar         *dest,
		     const guchar   *mask,
		     guint           opacity,
		     const gboolean *affect,
		     guint           length,
		     guint           bytes2,
		     gboolean        has_alpha2)
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
	  
	  for (b = 0; b < bytes2; b++)
	    dest[b] = (affect[b]) ?
	      INT_BLEND(src2[b], src1[b], mask_alpha, tmp) :
	      src1[b];

	  if (!has_alpha2)
	    dest[b] = src1[b];

	  m++;

	  src1 += 4;
	  src2 += bytes2;
	  dest += 4;
	}
    }
  else
    {
      static const guchar mask_alpha = OPAQUE_OPACITY ;

      while (length --)
        {
	  for (b = 0; b < bytes2; b++)
	    dest[b] = (affect[b]) ?
	      INT_BLEND(src2[b], src1[b], mask_alpha, tmp) :
	      src1[b];

	  if (!has_alpha2)
	    dest[b] = src1[b];

	  src1 += 4;
	  src2 += bytes2;
	  dest += 4;
	}
    }
}


void
erase_rgba_pixels (const guchar   *src1,
		   const guchar   *src2,
		   guchar         *dest,
		   const guchar   *mask,
		   guint           opacity,
		   const gboolean *affect,
		   guint           length)
{
  gint       b;
  guchar     src2_alpha;
  glong      tmp;

  if (mask)
    {
      const guchar *m = mask;
 
      while (length --)
        {
	  for (b = 0; b < 3; b++)
	    dest[b] = src1[b];

	  src2_alpha = INT_MULT3(src2[3], *m, opacity, tmp);
	  dest[3] = src1[3] - INT_MULT(src1[3], src2_alpha, tmp);

	  m++;

	  src1 += 4;
	  src2 += 4;
	  dest += 4;
	}
    }
  else
    {
      while (length --)
        {
	  dest[0] = src1[0];
	  dest[1] = src1[1];
	  dest[2] = src1[2];

	  dest[3] = src1[3] - INT_MULT3(src1[3], src2[3], opacity, tmp);

	  src1 += 4;
	  src2 += 4;
	  dest += 4;
	}
    }
}


void
anti_erase_rgba_pixels (const guchar   *src1,
			const guchar   *src2,
			guchar         *dest,
			const guchar   *mask,
			guint           opacity,
			const gboolean *affect,
			guint           length)
{
  guchar        src2_alpha;
  const guchar *m;
  glong         tmp;

  if (mask)
    m = mask;
  else
    m = &no_mask;

  while (length --)
    {
      dest[0] = src1[0];
      dest[1] = src1[1];
      dest[2] = src1[2];

      src2_alpha = INT_MULT3(src2[3], *m, opacity, tmp);
      dest[3] = src1[3] + INT_MULT((255 - src1[3]), src2_alpha, tmp);

      if (mask)
	m++;

      src1 += 4;
      src2 += 4;
      dest += 4;
    }
}


void
extract_from_pixels_rgba (guchar       *src,
			  guchar       *dest,
			  const guchar *mask,
			  const guchar *bg,
			  gboolean      cut,
			  guint         length)
{
  const guchar *m;
  gint          tmp;

  if (mask)
    m = mask;
  else
    m = &no_mask;

  while (length --)
    {
      dest[0] = src[0];
      dest[1] = src[1];
      dest[2] = src[2];

      dest[3] = INT_MULT(*m, src[3], tmp);
      if (cut)
	src[3] = INT_MULT((255 - *m), src[3], tmp);

      if (mask)
	m++;

      src += 4;
      dest += 4;
    }
}


static void
expand_line_rgba (gdouble           *dest,
		  const gdouble     *src,
		  guint              old_width,
		  guint              width,
		  InterpolationType  interp)
{
  gdouble  ratio;
  gint     x,b;
  guint    src_col;
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
	s = (gdouble *) &src[src_col * 4];
	for (b = 0; b < 4; b++)
	  dest[b] = cubic (frac, s[b - 4], s[b], s[b+4], s[b+4*2]);
	dest += 4;
      }
      break;
    case LINEAR_INTERPOLATION:
      for (x = 0; x < width; x++)
      {
	src_col = ((int)((x) * ratio + 2.0 - 0.5)) - 2;
	/* +2, -2 is there because (int) rounds towards 0 and we need 
	   to round down */
	frac =          ((x) * ratio - 0.5) - src_col;
	s = (gdouble *) &src[src_col * 4];
	for (b = 0; b < 4; b++)
	  dest[b] = ((s[b + 4] - s[b]) * frac + s[b]);
	dest += 4;
      }
      break;
   case NEAREST_NEIGHBOR_INTERPOLATION:
     g_error("sampling_type can't be "
	     "NEAREST_NEIGHBOR_INTERPOLATION");
  }
}


static void
shrink_line_rgba (gdouble	    *dest,
		  const gdouble     *src,
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

  for (b = 0; b < 4; b++)
  {
    
    source = &src[b];
    destp = &dest[b];
    position = -1;

    mant = *source;

    for (x = 0; x < width; x++)
    {
      source+= 4;
      accum = 0;
      max = ((int)(position+step)) - ((int)(position));
      max--;
      while (max)
      {
	accum += *source;
	source += 4;
	max--;
      }
      tmp = accum + mant;
      mant = ((position+step) - (int)(position + step));
      mant *= *source;
      tmp += mant;
      tmp *= inv_step;
      mant = *source - mant;
      *(destp) = tmp;
      destp += 4;
      position += step;
	
    }
  }
}

void
apply_layer_mode_rgba_rgb (apply_combine_layer_info *info)
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

  guint combine = COMBINE_INTEN_A_INTEN;

  switch (mode)
    {
      case NORMAL_MODE:
      case ERASE_MODE:
      case ANTI_ERASE_MODE:
	dest = src2;
	break;

      case DISSOLVE_MODE:
	add_alpha_pixels_rgb (src2, dest, length);
	dissolve_pixels_rgba (src2, dest, x, y, opacity, length);
	combine = COMBINE_INTEN_A_INTEN_A;
	break;

      case MULTIPLY_MODE:
	multiply_pixels_rgba_rgb (src1, src2, dest, length);
	break;

      case DIVIDE_MODE:
	divide_pixels_rgba_rgb (src1, src2, dest, length);
	break;

      case SCREEN_MODE:
	screen_pixels_rgba_rgb (src1, src2, dest, length);
	break;

      case OVERLAY_MODE:
	overlay_pixels_rgba_rgb (src1, src2, dest, length);
	break;

      case DIFFERENCE_MODE:
	difference_pixels_rgba_rgb (src1, src2, dest, length);
	break;

      case ADDITION_MODE:
	add_pixels_rgba_rgb (src1, src2, dest, length);
	break;

      case SUBTRACT_MODE:
	subtract_pixels_rgba_rgb (src1, src2, dest, length);
	break;

      case DARKEN_ONLY_MODE:
	darken_pixels_rgba_rgb (src1, src2, dest, length);
	break;

      case LIGHTEN_ONLY_MODE:
	lighten_pixels_rgba_rgb (src1, src2, dest, length);
	break;

      case HUE_MODE: case SATURATION_MODE: case VALUE_MODE:
	g_warning ("Cannot calculate hsv only view of nonalpha nonRGB images!\n");	
	break;

      case COLOR_MODE:
	g_warning ("Cannot calculate color only view of nonalpha nonRGB images!\n");	
	break;

      case BEHIND_MODE:
	dest = src2;
	combine = BEHIND_INTEN;
	break;

      case REPLACE_MODE:
	dest = src2;
	combine = REPLACE_INTEN;
	break;

      case DODGE_MODE:
	dodge_pixels_rgba_rgb (src1, src2, dest, length);
	break;

      case BURN_MODE:
	burn_pixels_rgba_rgb (src1, src2, dest, length);
	break;

      case HARDLIGHT_MODE:
	hardlight_pixels_rgba_rgb (src1, src2, dest, length);
	break;

      default :
	break;
    }

  /*  Determine whether the alpha channel of the destination can be affected
   *  by the specified mode--This keeps consistency with varying opacities
   */
  *mode_affect = layer_modes[mode].affect_alpha;

  return combine;
}

void
apply_layer_mode_rgba_rgba (apply_combine_layer_info *info)
{
  const guchar *src1 = info->src1;
  const guchar *src2 = info->src2;
  guchar *dest = info->dest;
  guint x = info->x;
  guint y = info->y;
  guint opacity = opacity;
  guint length = info->length;
  LayerModeEffects mode = info->mode;
  gint combine = COMBINE_INTEN_A_INTEN_A;

  switch (mode)
    {
      case NORMAL_MODE:
	dest = src2;
	break;

      case DISSOLVE_MODE:
	dissolve_pixels_rgba (src2, dest, x, y, opacity, length);
	break;

      case MULTIPLY_MODE:
	multiply_pixels_rgba_rgba (src1, src2, dest, length);
	break;

      case DIVIDE_MODE:
	divide_pixels_rgba_rgba (src1, src2, dest, length);
	break;

      case SCREEN_MODE:
	screen_pixels_rgba_rgba (src1, src2, dest, length);
	break;

      case OVERLAY_MODE:
	overlay_pixels_rgba_rgba (src1, src2, dest, length);
	break;

      case DIFFERENCE_MODE:
	difference_pixels_rgba_rgba (src1, src2, dest, length);
	break;

      case ADDITION_MODE:
	add_pixels_rgba_rgba (src1, src2, dest, length);
	break;

      case SUBTRACT_MODE:
	subtract_pixels_rgba_rgba (src1, src2, dest, length);
	break;

      case DARKEN_ONLY_MODE:
	darken_pixels_rgba_rgba (src1, src2, dest, length);
	break;

      case LIGHTEN_ONLY_MODE:
	lighten_pixels_rgba_rgba (src1, src2, dest, length);
	break;

      case HUE_MODE: case SATURATION_MODE: case VALUE_MODE:
	hsv_only_pixels_rgba_rgba (src1, src2, dest, mode, length);
	break;

      case COLOR_MODE:
	color_only_pixels_rgba_rgba (src1, src2, dest, mode, length);
	break;

      case BEHIND_MODE:
	dest = src2;
	combine = BEHIND_INTEN;
	break;

      case REPLACE_MODE:
	dest = src2;
	combine = REPLACE_INTEN;
	break;

      case ERASE_MODE:
	dest = src2;
	combine = ERASE_INTEN;
	break;

      case ANTI_ERASE_MODE:
	dest = src2;
	combine = ANTI_ERASE_INTEN;
	break;

      case DODGE_MODE:
	dodge_pixels_rgba_rgba (src1, src2, dest, length);
	break;

      case BURN_MODE:
	burn_pixels_rgba_rgba (src1, src2, dest, length);
	break;

      case HARDLIGHT_MODE:
	hardlight_pixels_rgba_rgba (src1, src2, dest, length);
	break;

      default :
	break;
    }

  
   combine_rgb_and_rgba_pixels (s1, s, d, m, opacity, affect, src1->w, src1->bytes);
	    
  /*  Determine whether the alpha channel of the destination can be affected
   *  by the specified mode--This keeps consistency with varying opacities
   */
  /* *mode_affect = layer_modes[mode].affect_alpha; */
  // FIXME: mode_affect needs to be passed to combine functions.
}
#endif

