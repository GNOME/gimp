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
#include "paint_funcs_graya.h"
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

static void     run_length_encode_graya  (guchar   *src,
					  guint     *dest,
					  guint     w);
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
      return op##_pixels_1a_1a(src1, src2, length, dest); \
    } \
  /*fprintf(stderr, "non-MMX: %s(%d, %d, %d, %d)\n", #op, \
    bytes1, bytes2, has_alpha1, has_alpha2);*/
#else

#define MMX_PIXEL_OP_3A_1A(op)
#define USE_MMX_PIXEL_OP_3A_1A(op)
#endif


void
update_tile_rowhints_graya (Tile *tile,
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
	  alpha = ptr[1];

	  /* row is all-opaque or all-transparent? */
	  if (alpha == 0 || alpha == 255)
	    {
	      if (ewidth > 1)
		{
		  for (x = 1; x < ewidth; x++)
		    {
		      if (ptr[x*2 + 1] != alpha)
			{
			  tile_set_rowhint (tile, y, TILEROWHINT_MIXED);
			  goto next_row2;
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

    next_row2:
      ptr += 2 * ewidth;
    }
  
  return;
}


static void
run_length_encode_graya (guchar *src,
			 guint  *dest,
			 guint   w)
{
  gint   start;
  gint   i;
  gint   j;
  guchar last;

  last = *src;
  src += 2;
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
      src += 2;
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
color_pixels_graya (guchar       *dest,
		    const guchar *color,
	            guint         w)
{
  /* dest % bytes and color % bytes must be 0 or we will crash 
     when bytes = 2 or 4.
     Is this safe to assume?  Lets find out.
     This is 4-7X as fast as the simple version.
     */

#if defined(sparc) || defined(__sparc__)
  register guchar   c0, c1;
#else
  register guint16 *shortd, shortc;
#endif

#if defined(sparc) || defined(__sparc__)
  c0 = color[0];
  c1 = color[1];
  while (w--)
    {
      dest[0] = c0;
      dest[1] = c1;
      dest += 2;
    }
#else
  shortc = ((guint16 *) color)[0];
  shortd = (guint16 *) dest;
  while (w--)
    {
      *shortd = shortc;
      shortd++;
    }
#endif /* sparc || __sparc__ */
}


void
blend_pixels_graya (const guchar *src1,
		    const guchar *src2,
		    guchar       *dest,
		    gint          blend,
		    gint          w)
{
  gint   b;
  guchar blend2 = (255 - blend);

  while (w --)
    {
      for (b = 0; b < 2; b++)
	dest[b] = (src1[b] * blend2 + src2[b] * blend) / 255;

      src1 += 2;
      src2 += 2;
      dest += 2;
    }
}


void
shade_pixels_graya (const guchar *src,
		    guchar       *dest,
		    const guchar *col,
		    gint          blend,
		    gint          w)
{
  guchar blend2 = (255 - blend);

  while (w --)
    {
      dest[0] = (src[0] * blend2 + col[0] * blend) / 255;

      dest[1] = src[1];  /* alpha channel */

      src += 2;
      dest += 2;
    }
}


void
extract_alpha_pixels_graya (const guchar *src,
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
          *dest++ = INT_MULT(src[1], *m, tmp);
          m++;
          src += 2;
        }
    }
  else
    { 
      while (w --)
        { 
          *dest++ = src[1];
          src += 2;
        }
    }
}

MMX_PIXEL_OP_3A_1A(darken)

void
darken_pixels_graya_graya (const guchar *src1,
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

      dest[1] = MIN (src1[1], src2[1]);

      src1 += 2;
      src2 += 2;
      dest += 2;
    }
}

void
darken_pixels_graya_gray (const guchar *src1,
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

      src1 += 2;
      src2 += 1;
      dest += 1;
    }
}

MMX_PIXEL_OP_3A_1A(lighten)

void
lighten_pixels_graya_graya (const guchar *src1,
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

      dest[1] = MIN (src1[1], src2[1]);

      src1 += 2;
      src2 += 2;
      dest += 2;
    }
}

void
lighten_pixels_graya_gray (const guchar *src1,
			   const guchar *src2,
			   guchar       *dest,
			   guint         length)
{
  guchar s1, s2;

  while (length--)
    {
      s1 = src1[0];
      s2 = src2[0];
      dest[0] = (src1[0] < src2[0]) ? src2[0] : src1[0];

      src1 += 2;
      src2 += 1;
      dest += 1;
    }
}

MMX_PIXEL_OP_3A_1A(multiply)

void
multiply_pixels_graya_graya (const guchar *src1,
			     const guchar *src2,
			     guchar       *dest,
			     guint         length)
{
  gint tmp;

  USE_MMX_PIXEL_OP_3A_1A(multiply)

  while (length --)
    {
      dest[0] = INT_MULT(src1[0], src2[0], tmp);
      dest[1] = src2[1];

      src1 += 2;
      src2 += 2;
      dest += 2;
    }
}

void
multiply_pixels_graya_gray (const guchar *src1,
			    const guchar *src2,
			    guchar       *dest,
			    guint         length)
{
  gint tmp;

  while (length --)
    {
      dest[0] = INT_MULT(src1[0], src2[0], tmp);

      src1 += 2;
      src2 += 1;
      dest += 1;
    }
}


void
divide_pixels_graya_graya (const guchar *src1,
			   const guchar *src2,
			   guchar       *dest,
			   guint         length)
{
  guint result;

  while (length --)
    {
      result = ((src1[0] * 256) / (1+src2[0]));
      dest[0] = MIN (result, 255);

      dest[1] = MIN (src1[1], src2[1]);

      src1 += 2;
      src2 += 2;
      dest += 2;
    }
}

void
divide_pixels_graya_gray (const guchar *src1,
			  const guchar *src2,
			  guchar       *dest,
			  guint         length)
{
  guint result;

  while (length --)
    {
      result = ((src1[0] * 256) / (1+src2[0]));
      dest[0] = MIN (result, 255);

      src1 += 2;
      src2 += 1;
      dest += 1;
    }
}


MMX_PIXEL_OP_3A_1A(screen)

void
screen_pixels_graya_graya (const guchar *src1,
			   const guchar *src2,
			   guchar       *dest,
			   guint         length)
{
  guint tmp;

  USE_MMX_PIXEL_OP_3A_1A(screen)

  while (length --)
    {
      dest[0] = 255 - INT_MULT((255 - src1[0]), (255 - src2[0]), tmp);
      dest[1] = MIN (src1[1], src2[1]);

      src1 += 2;
      src2 += 2;
      dest += 2;
    }
}

void
screen_pixels_graya_gray (const guchar *src1,
			  const guchar *src2,
			  guchar       *dest,
			  guint         length)
{
  guint tmp;

  while (length --)
    {
      dest[0] = 255 - INT_MULT((255 - src1[0]), (255 - src2[0]), tmp);

      src1 += 2;
      src2 += 1;
      dest += 1;
    }
}


MMX_PIXEL_OP_3A_1A(overlay)

void
overlay_pixels_graya_graya (const guchar *src1,
			    const guchar *src2,
			    guchar       *dest,
			    guint         length)
{
  guint tmp;

  while (length --)
    {
      dest[0] = INT_MULT(src1[0], src1[0] + INT_MULT(2 * src2[0],
						     255 - src1[0],
						     tmp), tmp);
      dest[1] = MIN (src1[1], src2[1]);

      src1 += 2;
      src2 += 2;
      dest += 2;
    }
}

void
overlay_pixels_graya_gray (const guchar *src1,
			   const guchar *src2,
			   guchar       *dest,
			   guint         length)
{
  guint tmp;

  while (length --)
    {
      dest[0] = INT_MULT(src1[0], src1[0] + INT_MULT(2 * src2[0],
						     255 - src1[0],
						     tmp), tmp);

      src1 += 2;
      src2 += 1;
      dest += 1;
    }
}

void
dodge_pixels_graya_graya (const guchar *src1,
			  const guchar *src2,
			  guchar       *dest,
			  guint         length)
{
  gint tmp1;

  while (length --)
    {
      tmp1 = src1[0] << 8;
      tmp1 /= 256 - src2[0];
      dest[0] = (guchar) CLAMP (tmp1, 0, 255);

      dest[1] = MIN (src1[1], src2[1]);

      src1 += 2;
      src2 += 2;
      dest += 2;
    }
}

void
dodge_pixels_graya_gray (const guchar *src1,
			 const guchar *src2,
			 guchar       *dest,
			 guint         length)
{
  gint tmp1;

  while (length --)
    {
      tmp1 = src1[0] << 8;
      tmp1 /= 256 - src2[0];
      dest[0] = (guchar) CLAMP (tmp1, 0, 255);

      src1 += 2;
      src2 += 1;
      dest += 1;
    }
}

void
burn_pixels_graya_graya (const guchar *src1,
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

      dest[1] = MIN (src1[1], src2[1]);

      src1 += 2;
      src2 += 2;
      dest += 2;
    }
}

void
burn_pixels_graya_gray (const guchar *src1,
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

      src1 += 2;
      src2 += 1;
      dest += 1;
    }
}

void
hardlight_pixels_graya_graya (const guchar *src1,
			      const guchar *src2,
			      guchar       *dest,
			      guint         length)
{
  gint tmp1;

  while (length --)
    {
      if (src2[0] > 128)
	{
	  tmp1 = ((gint)255 - src1[0]) * ((gint)255 - ((src2[0] - 128) << 1));
	  dest[0] = (guchar) CLAMP (255 - (tmp1 >> 8), 0, 255);
	} else
	{
	  tmp1 = (gint)src1[0] * ((gint)src2[0] << 1);
	  dest[0] = (guchar) CLAMP(tmp1 >> 8, 0, 255);
	}

      dest[1] = MIN (src1[1], src2[1]);

      src1 += 2;
      src2 += 2;
      dest += 2;
    }
}

void
hardlight_pixels_graya_gray (const guchar *src1,
			     const guchar *src2,
			     guchar       *dest,
			     guint         length)
{
  gint tmp1;

  while (length --)
    {
      if (src2[0] > 128)
	{
	  tmp1 = ((gint)255 - src1[0]) * ((gint)255 - ((src2[0] - 128) << 1));
	  dest[0] = (guchar) CLAMP (255 - (tmp1 >> 8), 0, 255);
	} else
	{
	  tmp1 = (gint)src1[0] * ((gint)src2[0] << 1);
	  dest[0] = (guchar) CLAMP(tmp1 >> 8, 0, 255);
	}

      src1 += 2;
      src2 += 1;
      dest += 1;
    }
}


MMX_PIXEL_OP_3A_1A(add)

void
add_pixels_graya_graya (const guchar *src1,
			const guchar *src2,
			guchar       *dest,
			guint         length)
{
  USE_MMX_PIXEL_OP_3A_1A(add)

  while (length --)
    {
      dest[0] = add_lut[(src1[0])] [(src2[0])];
      dest[1] = MIN (src1[1], src2[1]);

      src1 += 2;
      src2 += 2;
      dest += 2;
    }
}

void
add_pixels_graya_gray (const guchar *src1,
		       const guchar *src2,
		       guchar       *dest,
		       guint         length)
{
  while (length --)
    {
      dest[0] = add_lut[(src1[0])] [(src2[0])];
      dest[1] = MIN (src1[1], src2[1]);

      src1 += 2;
      src2 += 1;
      dest += 1;
    }
}


MMX_PIXEL_OP_3A_1A(substract)

void
subtract_pixels_graya_graya (const guchar *src1,
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

      dest[1] = MIN (src1[1], src2[1]);

      src1 += 2;
      src2 += 2;
      dest += 2;
    }
}

void
subtract_pixels_graya_gray (const guchar *src1,
			    const guchar *src2,
			    guchar       *dest,
			    guint         length)
{
  gint diff;

  while (length --)
    {
      diff = src1[0] - src2[0];
      dest[0] = (diff < 0) ? 0 : diff;

      src1 += 2;
      src2 += 1;
      dest += 1;
    }
}


MMX_PIXEL_OP_3A_1A(difference)

void
difference_pixels_graya_graya (const guchar *src1,
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

      dest[1] = MIN (src1[1], src2[1]);

      src1 += 2;
      src2 += 2;
      dest += 2;
    }
}

void
difference_pixels_graya_gray (const guchar *src1,
			      const guchar *src2,
			      guchar       *dest,
			      guint         length)
{
  gint diff;

  while (length --)
    {
      diff = src1[0] - src2[0];
      dest[0] = (diff < 0) ? -diff : diff;

      src1 += 2;
      src2 += 1;
      dest += 1;
    }
}

/* This functions assumes that the destination ALWAYS has an alpha
   channel! */

void
dissolve_pixels_graya (const guchar *src,
		       guchar       *dest,
		       guint          x,
		       guint          y,
		       guint          opacity,
		       guint          length)
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
      dest[0] = src[0];

      /*  dissolve if random value is > opacity  */
#if defined(ENABLE_MP) && defined(__GLIBC__)
      lrand48_r (&seed, &temp_val);
      rand_val = temp_val & 0xff;
#elif defined(ENABLE_MP) && !defined(__GLIBC__)
      rand_val = (rand_r (&seed) & 0xff);
#else
      rand_val = (rand () & 0xff);
#endif
      dest[1] = (rand_val > src[1]) ? 0 : src[1];

      dest += 2;
      src  += 2;
    }
}

/* This function only works on regions with identical number of bytes! */

void
replace_graya_pixels (guchar   *src1,
		      guchar   *src2,
		      guchar   *dest,
		      guchar   *mask,
		      gint      length,
		      gint      opacity,
		      gboolean *affect,
		      gint      bytes2)
{
  gdouble a_val, a_recip, mask_val;
  gdouble norm_opacity;
  gint    s1_a, s2_a;
  gint    new_val;

  if (2 != bytes2)
    {
      g_warning ("replace_pixels only works on commensurate pixel regions");
      return;
    }

  norm_opacity = opacity * (1.0 / 65536.0);

  while (length --)
    {
      mask_val = mask[0] * norm_opacity;
      /* calculate new alpha first. */
      s1_a = src1[1];
      s2_a = src2[1];
      a_val = s1_a + mask_val * (s2_a - s1_a);
 
      if (a_val == 0) /* In any case, write out versions of the blending function */
                      /* that result when combinations of s1_a, s2_a, and         */
	              /* mask_val --> 0 (or mask_val -->1)                        */
        {
          /* Case 1: s1_a, s2_a, AND mask_val all approach 0+:               */
	  /* Case 2: s1_a AND s2_a both approach 0+, regardless of mask_val: */

          if (s1_a + s2_a == 0.0)
            {
	      new_val = 0.5 + (gdouble) src1[0] + 
		mask_val * ((gdouble) src2[0] - (gdouble) src1[0]);

	      dest[1] = affect[1] ? MIN (new_val, 255) : src1[1];
            }

	  /* Case 3: mask_val AND s1_a both approach 0+, regardless of s2_a  */
          else if (s1_a + mask_val == 0.0)
            {
	      dest[0] = src1[0]; 
            }

	  /* Case 4: mask_val -->1 AND s2_a -->0, regardless of s1_a         */
          else if (1.0 - mask_val + s2_a == 0.0)
            {
	      dest[0] = affect[0] ? src2[0] : src1[0]; 
            }
	}
      else
	{
	  a_recip = 1.0 / a_val;
          /* possible optimization: fold a_recip into s1_a and s2_a              */
	  new_val = 0.5 + a_recip * (src1[0] * s1_a + mask_val *
				    (src2[0] * s2_a - src1[0] * s1_a));
	  dest[0] = affect[0] ? MIN (new_val, 255) : src1[0];
        }

      dest[1] = affect[1] ? a_val + 0.5: s1_a;
      src1 += 2;
      src2 += bytes2;
      dest += bytes2;
      mask++;
    }
}


void
flatten_graya_pixels (const guchar *src,
		      guchar       *dest,
		      const guchar *bg,
		      gint          length)
{
  gint t1, t2;

  while (length --)
    {
      dest[0] = INT_MULT (src[0], src[1], t1) + 
		INT_MULT (bg[0], (255 - src[1]), t2);

      src += 2;
      dest++;
    }
}


void
graya_to_rgba_pixels (const guchar *src,
		      guchar       *dest,
		      gint          length)
{
  gint     b;

  while (length --)
    {
      for (b = 0; b < 3; b++)
	dest[b] = src[0];

      dest[3] = src[1];

      src += 2;
      dest += 4;
    }
}


void
apply_mask_to_alpha_channel_graya (guchar       *src,
				   const guchar *mask,
				   gint          opacity,
				   gint          length)
{
  glong tmp;

  src++;

  if (opacity == 255)
    {
      while (length --)
	{
	  *src = INT_MULT(*src, *mask, tmp);
	  mask++;
	  src += 2;
	}
    }
  else
    {
      while (length --)
	{
	  *src = INT_MULT3(*src, *mask, opacity, tmp);
	  mask++;
	  src += 2;
	}
    }
}


void
combine_mask_and_alpha_channel_graya (guchar       *src,
				      const guchar *mask,
				      gint          opacity,
				      gint          length)
{
  gint mask_val;
  gint tmp;

  src++;

  if (opacity != 255)
    while (length --)
    {
      mask_val = INT_MULT(*mask, opacity, tmp);
      mask++;
      *src = *src + INT_MULT((255 - *src) , mask_val, tmp);
      src += 2;
    }
  else
    while (length --)
    {
      *src = *src + INT_MULT((255 - *src) , *mask, tmp);
      src += 2;
      mask++;
    }
}


void
copy_graya_to_inten_a_pixels (const guchar *src,
			      guchar       *dest,
			      gint          length)
{
  while (length --)
    {
      dest[0] = src[0];
      dest[1] = OPAQUE_OPACITY;

      src ++;
      dest += 2;
    }
}


void
initial_channel_pixels_graya (const guchar *src,
			      guchar       *dest,
			      gint          length)
{
  while (length --)
    {
      dest[0] = src[0];
      dest[1] = OPAQUE_OPACITY;

      src ++;
      dest += 2;
    }
}


void
initial_pixels_graya (const guchar   *src,
		      guchar         *dest,
		      const guchar   *mask,
		      gint            opacity,
		      const gboolean *affect,
		      gint            length)
{
  const guchar *m;
  glong         tmp;

  if (mask)
    {
      m = mask;
      while (length --)
	{
	  dest[0] = src[0] * affect[0];
	  
	  /*  Set the alpha channel  */
	  dest[1] = affect [1] ? INT_MULT3(opacity, src[1], *m, tmp) : 0;
	  
	  m++;
	  
	  dest += 2;
	  src += 2;
	}
    }
  else
    {
      while (length --)
	{
	  dest[0] = src[0] * affect[0];
	  
	  /*  Set the alpha channel  */
	  dest[1] = affect [1] ? INT_MULT(opacity , src[1], tmp) : 0;
	  
	  dest += 2;
	  src += 2;
	}
    }
}


void
combine_graya_and_indexeda_pixels (apply_combine_layer_info *info)
{
  const guchar *src1 = info->src1;
  const guchar *src2 = info->src2;
  guchar       *dest = info->dest;
  const guchar *mask = info->mask;
  const guchar *cmap = info->coldata;
  guint		opacity = info->opacity;
  guint		length = info->length;
  
  guchar new_alpha;
  gint   index;
  glong  tmp;
  const guchar *m;

  if (mask)
    {
      m = mask;
 
      while (length --)
	{
	  new_alpha = INT_MULT3(src2[1], *m, opacity, tmp);

	  index = src2[0] * 3;

	  dest[0] = (new_alpha > 127) ? cmap[index] : src1[0];
	  dest[1] = (new_alpha > 127) ? OPAQUE_OPACITY : src1[1];  
	  /*  alpha channel is opaque  */

	  m++;

	  src1 += 2;
	  src2 += 2;
	  dest += 2;
	}
    }
  else
    {
      while (length --)
	{
	  new_alpha = INT_MULT(src2[1], opacity, tmp);

	  index = src2[0] * 3;

	  dest[0] = (new_alpha > 127) ? cmap[index] : src1[0];
	  dest[1] = (new_alpha > 127) ? OPAQUE_OPACITY : src1[1];  

	  src1 += 2;
	  src2 += 2;
	  dest += 2;
	}
    }
}


#define alphify(src2_alpha,new_alpha) \
	if (src2_alpha != 0 && new_alpha != 0)							\
	  {											\
	    if (src2_alpha == new_alpha){							\
	      dest[0] = affect[0] ? src2[0] : src1[0];						\
	    } else {										\
	      ratio = (float) src2_alpha / new_alpha;						\
	      compl_ratio = 1.0 - ratio;							\
	  											\
	      dest[0] = affect[0] ?								\
		(guchar) (src2[0] * ratio + src1[0] * compl_ratio + EPSILON) : src1[0];		\
	    }											\
	  }

void
combine_graya_and_gray_pixels (const guchar   *src1,
			       const guchar   *src2,
			       guchar         *dest,
			       const guchar   *mask,
			       gint            opacity,
			       const gboolean *affect,
			       gint            mode_affect,  /*  how does the combination mode affect alpha?  */
			       gint            length)
{
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
	      new_alpha = src1[1] +
		INT_MULT((255 - src1[1]), src2_alpha, tmp);
	      alphify (src2_alpha, new_alpha);
	      
	      if (mode_affect)
		{
		  dest[1] = (affect[1]) ? new_alpha : src1[1];
		}
	      else
		{
		  dest[1] = (src1[1]) ? src1[1] :
		    (affect[1] ? new_alpha : src1[1]);
		}
		
	      m++;
	      src2++;
	      src1 += 2;
	      dest += 2;
	    }
	}
      else /* HAS MASK, SEMI-OPACITY */
	{
	  while (length--)
	    {
	      src2_alpha = INT_MULT(*m, opacity, tmp);
	      new_alpha = src1[1] + INT_MULT((255 - src1[1]), src2_alpha, tmp);
	      alphify (src2_alpha, new_alpha);
	      
	      if (mode_affect)
		{
		  dest[1] = (affect[1]) ? new_alpha : src1[1];
		}
	      else
		{
		  dest[1] = (src1[1]) ? src1[1] :
		    (affect[1] ? new_alpha : src1[1]);
		}
		
	      m++;
	      src2++;
	      src1 += 2;
	      dest += 2;
	    }
	}
    }
  else /* NO MASK */
    {
      while (length --)
	{
	  src2_alpha = opacity;
	  new_alpha = src1[1] +
	    INT_MULT((255 - src1[1]), src2_alpha, tmp);
	  alphify (src2_alpha, new_alpha);
	  
	  if (mode_affect)
	    dest[1] = (affect[1]) ? new_alpha : src1[1];
	  else
	    dest[1] = (src1[1]) ? src1[1] : (affect[1] ? new_alpha : src1[1]);

	    src1 += 2;
	    src2 += 1;
	    dest += 2;
	}
    }
}


void
combine_graya_and_graya_pixels (const guchar   *src1,
				const guchar   *src2,
				guchar         *dest,
				const guchar   *mask,
				gint            opacity,
				const gboolean *affect,
				const gint      mode_affect,  /*  how does the combination mode affect alpha?  */
				gint            length)
				
{
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
		      src2_alpha = INT_MULT(src2[1], *m, tmp);
		      new_alpha = src1[1] +
			INT_MULT((255 - src1[1]), src2_alpha, tmp);
		  
		      alphify (src2_alpha, new_alpha);
		  
		      if (mode_affect)
			{
			  dest[1] = (affect[1]) ? new_alpha : src1[1];
			}
		      else
			{
			  dest[1] = (src1[1]) ? src1[1] :
			    (affect[1] ? new_alpha : src1[1]);      
			}
		  
		      m++;
		      src1 += 2;
		      src2 += 2;
		      dest += 2;	
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
			  src2_alpha = INT_MULT(src2[1], *m, tmp);
			  new_alpha = src1[1] +
			    INT_MULT((255 - src1[1]), src2_alpha, tmp);
		  
			  alphify (src2_alpha, new_alpha);
		  
			  if (mode_affect)
			    {
			      dest[1] = (affect[1]) ? new_alpha : src1[1];
			    }
			  else
			    {
			      dest[1] = (src1[1]) ? src1[1] :
				(affect[1] ? new_alpha : src1[1]);      
			    }

			  m++;
			  src1 += 2;
			  src2 += 2;
			  dest += 2;
			  /* GUTS END */
			}
		    }
		  else
		    {
		      j = 2 * sizeof(int);
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
	      src2_alpha = INT_MULT(src2[1], *m, tmp);
	      new_alpha = src1[1] +
		INT_MULT((255 - src1[1]), src2_alpha, tmp);
		  
	      alphify (src2_alpha, new_alpha);
		  
	      if (mode_affect)
		{
		  dest[1] = (affect[1]) ? new_alpha : src1[1];
		}
	      else
		{
		  dest[1] = (src1[1]) ? src1[1] :
		    (affect[1] ? new_alpha : src1[1]);      
		}
		  
	      m++;
	      src1 += 2;
	      src2 += 2;
	      dest += 2;	
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
		      src2_alpha = INT_MULT3(src2[1], *m, opacity, tmp);
		      new_alpha = src1[1] +
			INT_MULT((255 - src1[1]), src2_alpha, tmp);
		  
		      alphify (src2_alpha, new_alpha);
		  
		      if (mode_affect)
			{
			  dest[1] = (affect[1]) ? new_alpha : src1[1];
			}
		      else
			{
			  dest[1] = (src1[1]) ? src1[1] :
			    (affect[1] ? new_alpha : src1[1]);
			}
		  
		      m++;
		      src1 += 2;
		      src2 += 2;
		      dest += 2;
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
			  src2_alpha = INT_MULT3(src2[1], *m, opacity, tmp);
			  new_alpha = src1[1] +
			    INT_MULT((255 - src1[1]), src2_alpha, tmp);
		      
			  alphify (src2_alpha, new_alpha);
		      
			  if (mode_affect)
			    {
			      dest[1] = (affect[1]) ? new_alpha : src1[1];
			    }
			  else
			    {
			      dest[1] = (src1[1]) ? src1[1] :
				(affect[1] ? new_alpha : src1[1]);
			    }

			  m++;
			  src1 += 2;
			  src2 += 2;
			  dest += 2;
			  /* GUTS END */
			}
		    }
		  else
		    {
		      j = 2 * sizeof(int);
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
	      src2_alpha = INT_MULT3(src2[1], *m, opacity, tmp);
	      new_alpha = src1[1] +
		INT_MULT((255 - src1[1]), src2_alpha, tmp);
	      
	      alphify (src2_alpha, new_alpha);
	      
	      if (mode_affect)
		{
		  dest[1] = (affect[1]) ? new_alpha : src1[1];
		}
	      else
		{
		  dest[1] = (src1[1]) ? src1[1] :
		    (affect[1] ? new_alpha : src1[1]);
		}
	      
	      m++;
	      src1 += 2;
	      src2 += 2;
	      dest += 2;
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
	      src2_alpha = src2[1];
	      new_alpha = src1[1] + INT_MULT((255 - src1[1]), src2_alpha, tmp);
	      
	      alphify (src2_alpha, new_alpha);
	      
	      if (mode_affect)
		{
		  dest[1] = (affect[1]) ? new_alpha : src1[1];
		}
	      else
		{
		  dest[1] = (src1[1]) ? src1[1] :
		    (affect[1] ? new_alpha : src1[1]);
		}
		
	      src1 += 2;
	      src2 += 2;
	      dest += 2;
	    }
	}
      else /* NO MASK, SEMI OPACITY */
	{
	  while (length --)
	    {
	      src2_alpha = INT_MULT(src2[1], opacity, tmp);
	      new_alpha = src1[1] +
		INT_MULT((255 - src1[1]), src2_alpha, tmp);
	      
	      alphify (src2_alpha, new_alpha);
	      
	      if (mode_affect)
		{
		  dest[1] = (affect[1]) ? new_alpha : src1[1];
		}
	      else
		{
		  dest[1] = (src1[1]) ? src1[1] :
		    (affect[1] ? new_alpha : src1[1]);
		}
		
	      src1 += 2;
	      src2 += 2;
	      dest += 2;	  
	    }
	}
    }
}
#undef alphify

void
combine_graya_and_channel_mask_pixels (apply_combine_layer_info *info)
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
      channel_alpha = INT_MULT (255 - *channel, opacity, t);
      if (channel_alpha)
	{
	  new_alpha = src[1] + INT_MULT ((255 - src[1]), channel_alpha, t);

	  if (new_alpha != 255)
	    channel_alpha = (channel_alpha * 255) / new_alpha;
	  compl_alpha = 255 - channel_alpha;

	  dest[0] = INT_MULT (col[0], channel_alpha, t) +
	    INT_MULT (src[0], compl_alpha, s);
	  dest[1] = new_alpha;
	}
      else
	{
	  dest[0] = src[0];
	  dest[1] = src[1];
	}

      /*  advance pointers  */
      src += 2;
      dest += 2;
      channel++;
    }
}


void
combine_graya_and_channel_selection_pixels (apply_combine_layer_info *info)
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
	  new_alpha = src[1] + INT_MULT ((255 - src[1]), channel_alpha, t);

	  if (new_alpha != 255)
	    channel_alpha = (channel_alpha * 255) / new_alpha;
	  compl_alpha = 255 - channel_alpha;

	  dest[0] = INT_MULT (col[0], channel_alpha, t) +
	    INT_MULT (src[0], compl_alpha, s);
	  dest[1] = new_alpha;
	}
      else
	{
	  dest[0] = src[0];
	  dest[1] = src[1];
	}

      /*  advance pointers  */
      src += 2;
      dest += 2;
      channel++;
    }
}


void
behind_inten_pixels_graya (const guchar   *src1,
			   const guchar   *src2,
			   guchar         *dest,
			   const guchar   *mask,
			   gint            opacity,
			   const gboolean *affect,
			   gint            length,
			   gint            bytes2,
			   gint            has_alpha2)
{
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

  while (length --)
    {
      src1_alpha = src1[1];
      src2_alpha = INT_MULT3(src2[1], *m, opacity, tmp);
      new_alpha = src2_alpha +
	INT_MULT((255 - src2_alpha), src1_alpha, tmp);
      if (new_alpha)
	ratio = (float) src1_alpha / new_alpha;
      else
	ratio = 0.0;
      compl_ratio = 1.0 - ratio;

      dest[0] = (affect[0]) ?
	(guchar) (src1[0] * ratio + src2[0] * compl_ratio + EPSILON) : src1[0];

      dest[1] = (affect[1]) ? new_alpha : src1[1];

      if (mask)
	m++;

      src1 += 2;
      src2 += bytes2;
      dest += 2;
    }
}


void
replace_inten_pixels_graya (const guchar   *src1,
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
  const gint bytes = bytes2;

  if (mask)
    {
      guchar        mask_alpha;
      const guchar *m = mask;

      while (length --)
        {
	  mask_alpha = INT_MULT(*m, opacity, tmp);
	  
	  for (b = 0; b < bytes; b++)
	    dest[b] = (affect[b]) ?
	      INT_BLEND(src2[b], src1[b], mask_alpha, tmp) :
	      src1[b];

	  if (!has_alpha2)
	    dest[b] = src1[b];

	  m++;

	  src1 += 2;
	  src2 += bytes2;
	  dest += 2;
	}
    }
  else
    {
      static const guchar mask_alpha = OPAQUE_OPACITY ;

      while (length --)
        {
	  for (b = 0; b < bytes; b++)
	    dest[b] = (affect[b]) ?
	      INT_BLEND(src2[b], src1[b], mask_alpha, tmp) :
	      src1[b];

	  if (!has_alpha2)
	    dest[b] = src1[b];

	  src1 += 2;
	  src2 += bytes2;
	  dest += 2;
	}
    }
}


void
erase_graya_pixels (const guchar   *src1,
		    const guchar   *src2,
		    guchar         *dest,
		    const guchar   *mask,
		    gint            opacity,
		    const gboolean *affect,
		    gint            length)
{
  guchar     src2_alpha;
  glong      tmp;

  if (mask)
    {
      const guchar *m = mask;
 
      while (length --)
        {
	  dest[0] = src1[0];

	  src2_alpha = INT_MULT3(src2[1], *m, opacity, tmp);
	  dest[1] = src1[1] - INT_MULT(src1[1], src2_alpha, tmp);

	  m++;

	  src1 += 2;
	  src2 += 2;
	  dest += 2;
	}
    }
  else
    {
      while (length --)
        {
	  dest[0] = src1[0];

	  src2_alpha = INT_MULT (src2[1], opacity, tmp);
	  dest[1] = src1[1] - INT_MULT (src1[1], src2_alpha, tmp);

	  src1 += 2;
	  src2 += 2;
	  dest += 2;
	}
    }
}


void
anti_erase_graya_pixels (const guchar   *src1,
			 const guchar   *src2,
			 guchar         *dest,
			 const guchar   *mask,
			 gint            opacity,
			 const gboolean *affect,
			 gint            length)
{
  guchar        src2_alpha;
  const guchar *m;
  glong         tmp;

  
  if (mask)
    {
      m = mask;
       
      while (length --)
	{
	  dest[0] = src1[0];

	  src2_alpha = INT_MULT3(src2[1], *m, opacity, tmp);
	  dest[1] = src1[1] + INT_MULT((255 - src1[1]), src2_alpha, tmp);
	  
	  m++;

	  src1 += 2;
	  src2 += 2;
	  dest += 2;
	}
    } else
    {
      while (length --)
	{
	  dest[0] = src1[0];

	  src2_alpha = INT_MULT(src2[1], opacity, tmp);
	  dest[1] = src1[1] + INT_MULT((255 - src1[1]), src2_alpha, tmp);

	  src1 += 2;
	  src2 += 2;
	  dest += 2;
	}
    }
}

void
extract_from_pixels_graya (guchar       *src,
			   guchar       *dest,
			   const guchar *mask,
			   const guchar *bg,
			   gint          cut,
			   gint          length)
{
  const guchar *m;
  gint          tmp;

  if (mask)
    {
      m = mask;
      while (length --)
	{
	  dest[0] = src[0];
	  dest[1] = INT_MULT(*m, src[1], tmp);
	  
	  if (cut)
	    src[1] = INT_MULT((255 - *m), src[1], tmp);

	  m++;

	  src += 2;
	  dest += 2;
	}
    } 
  else
    {
      while (length --)
	{
	  dest[0] = src[0];
	  dest[1] = src[1];
	  
	  if (cut)
	    src[1] = 0;

	  src += 2;
	  dest += 2;
	}
    }
}

void
apply_layer_mode_graya_gray (apply_combine_layer_info *info)
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
	dest = src2;
	break;

      case DISSOLVE_MODE:
	add_alpha_pixels_gray (src2, dest, length);
	dissolve_pixels_graya (src2, dest, x, y, opacity, length);
	combine = COMBINE_INTEN_A_INTEN_A;
	break;

      case MULTIPLY_MODE:
	multiply_pixels_graya_gray (src1, src2, dest, length);
	break;

      case DIVIDE_MODE:
	divide_pixels_graya_gray (src1, src2, dest, length);
	break;

      case SCREEN_MODE:
	screen_pixels_graya_gray (src1, src2, dest, length);
	break;

      case OVERLAY_MODE:
	overlay_pixels_graya_gray (src1, src2, dest, length);
	break;

      case DIFFERENCE_MODE:
	difference_pixels_graya_gray (src1, src2, dest, length);
	break;

      case ADDITION_MODE:
	add_pixels_graya_gray (src1, src2, dest, length);
	break;

      case SUBTRACT_MODE:
	subtract_pixels_graya_gray (src1, src2, dest, length);
	break;

      case DARKEN_ONLY_MODE:
	darken_pixels_graya_gray (src1, src2, dest, length);
	break;

      case LIGHTEN_ONLY_MODE:
	lighten_pixels_graya_gray (src1, src2, dest, length);
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
	dodge_pixels_graya_gray (src1, src2, dest, length);
	break;

      case BURN_MODE:
	burn_pixels_graya_gray (src1, src2, dest, length);
	break;

      case HARDLIGHT_MODE:
	hardlight_pixels_graya_gray (src1, src2, dest, length);
	break;

      default :
	break;
    }

  /*  Determine whether the alpha channel of the destination can be affected
   *  by the specified mode--This keeps consistency with varying opacities
   */
  *mode_affect = layer_modes[mode].affect_alpha;
}

void
apply_layer_mode_graya_graya (apply_combine_layer_info *info)
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

  gint combine = COMBINE_INTEN_A_INTEN_A;

  /*  assumes we're applying src2 TO src1  */
  switch (mode)
    {
    case NORMAL_MODE:
    case HUE_MODE:
    case SATURATION_MODE:
    case VALUE_MODE:
    case COLOR_MODE:
      dest = src2;
      break;

    case DISSOLVE_MODE:
      dissolve_pixels_graya (src2, dest, x, y, opacity, length);
      break;

    case MULTIPLY_MODE:
      multiply_pixels_graya_graya (src1, src2, dest, length);
      break;

    case DIVIDE_MODE:
      divide_pixels_graya_graya (src1, src2, dest, length);
      break;

    case SCREEN_MODE:
      screen_pixels_graya_graya (src1, src2, dest, length);
      break;

    case OVERLAY_MODE:
      overlay_pixels_graya_graya (src1, src2, dest, length);
      break;

    case DIFFERENCE_MODE:
      difference_pixels_graya_graya (src1, src2, dest, length);
      break;

    case ADDITION_MODE:
      add_pixels_graya_graya (src1, src2, dest, length);
      break;

    case SUBTRACT_MODE:
      subtract_pixels_graya_graya (src1, src2, dest, length);
      break;

    case DARKEN_ONLY_MODE:
      darken_pixels_graya_graya (src1, src2, dest, length);
      break;

    case LIGHTEN_ONLY_MODE:
      lighten_pixels_graya_graya (src1, src2, dest, length);
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
      dodge_pixels_graya_graya (src1, src2, dest, length);
      break;

    case BURN_MODE:
      burn_pixels_graya_graya (src1, src2, dest, length);
      break;

    case HARDLIGHT_MODE:
      hardlight_pixels_graya_graya (src1, src2, dest, length);
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

#endif
