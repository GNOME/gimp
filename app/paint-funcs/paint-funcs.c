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

#include "config.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "appenv.h"
#include "gimprc.h"
#include "paint_funcs.h"
#include "boundary.h"
#include "tile_manager.h"

#include "tile_manager_pvt.h"  /* For copy-on-write */
#include "tile.h"			/* ick. */

#include "libgimp/gimpintl.h"
#include "libgimp/gimpmath.h"

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
  int affect_alpha;     /*  does the layer mode affect the alpha channel  */
  int increase_opacity; /*  layer mode can increase opacity */
  int decrease_opacity; /*  layer mode can decrease opacity */
  char *name;           /*  layer mode specification  */
};

LayerMode layer_modes[] =	/* This must obviously be in the same
				 * order as the corresponding values
				 * in the LayerModeEffects enumeration.
				 */
{
  { 1, 1, 0, N_("Normal") },
  { 1, 1, 0, N_("Dissolve") },
  { 1, 1, 0, N_("Behind") },
  { 0, 0, 0, N_("Multiply (Burn)") },
  { 0, 0, 0, N_("Screen") },
  { 0, 0, 0, N_("Overlay") },
  { 0, 0, 0, N_("Difference") },
  { 0, 0, 0, N_("Addition") },
  { 0, 0, 0, N_("Subtraction") },
  { 0, 0, 0, N_("Darken Only") },
  { 0, 0, 0, N_("Lighten Only") },
  { 0, 0, 0, N_("Hue") },
  { 0, 0, 0, N_("Saturation") },
  { 0, 0, 0, N_("Color") },
  { 0, 0, 0, N_("Value") },
  { 0, 0, 0, N_("Divide (Dodge)") },
  { 1, 0, 1, N_("Erase") },
  { 1, 1, 1, N_("Replace") },
  { 1, 0, 1, N_("Anti Erase") }
};

/*  ColorHash structure  */
typedef struct _ColorHash ColorHash;

struct _ColorHash
{
  int pixel;           /*  R << 16 | G << 8 | B  */
  int index;           /*  colormap index        */
  GimpImage* gimage;     
};

static ColorHash color_hash_table [HASH_TABLE_SIZE];
static int random_table [RANDOM_TABLE_SIZE];
static int color_hash_misses;
static int color_hash_hits;
static unsigned char * tmp_buffer;  /* temporary buffer available upon request */
static int tmp_buffer_size;
static unsigned char no_mask = OPAQUE_OPACITY;
static int add_lut[256][256];

/*******************************/
/*  Local function prototypes  */
static int *  make_curve         (double, int *);
static void   run_length_encode  (unsigned char *, int *, int, int);
#if 0
static void   draw_segments      (PixelRegion *, BoundSeg *, int, int, int, int);
#endif
static double cubic              (double, int, int, int, int);
static void apply_layer_mode_replace (unsigned char *, unsigned char *,
				      unsigned char *, unsigned char *,
				      int, int, int,
				      int, int, int, int *);
static void   rotate_pointers(void **p, guint32 n);


void
update_tile_rowhints (Tile* tile, int ymin, int ymax)
{
  int bpp, ewidth, eheight;
  int x,y;
  guchar* ptr;
  guchar alpha;
  TileRowHint thishint;

#ifdef HINTS_SANITY
  g_assert(tile!=NULL);
#endif

  bpp = tile_bpp (tile);
  ewidth = tile_ewidth (tile);
  eheight = tile_eheight (tile);

  if (bpp == 1 || bpp == 3)
    {
      for (y=ymin; y<=ymax; y++)
	tile_set_rowhint (tile, y, TILEROWHINT_OPAQUE);

      return;
    }

  if (bpp == 4)
    {
#ifdef HINTS_SANITY
      g_assert(tile!=NULL);
#endif

      ptr = tile_data_pointer (tile, 0, ymin);

#ifdef HINTS_SANITY
      g_assert(ptr!=NULL);
#endif

      for (y = ymin; y <= ymax; y++)
	{
	  thishint = tile_get_rowhint (tile, y);

#ifdef HINTS_SANITY
	  if (thishint==TILEROWHINT_BROKEN)
	    g_error("BROKEN y=%d",y);
	  if (thishint==TILEROWHINT_OUTOFRANGE)
	    g_error("OOR y=%d",y);
	  if (thishint==TILEROWHINT_UNDEFINED)
	    g_error("UNDEFINED y=%d - bpp=%d ew=%d eh=%d",
		    y,bpp,ewidth,eheight);
#endif

#ifdef HINTS_SANITY
	  if (thishint == TILEROWHINT_TRANSPARENT ||
	      thishint == TILEROWHINT_MIXED ||
	      thishint == TILEROWHINT_OPAQUE)
	    {
	      goto next_row4;
	    }

	  if (thishint != TILEROWHINT_UNKNOWN)
	    {
	      g_error("MEGABOGUS y=%d - bpp=%d ew=%d eh=%d",
		      y,bpp,ewidth,eheight);
	    }
#endif

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
			  if (ptr[x*4 + 3] != alpha)
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
      
      return;
    }

  if (bpp == 2)
    {
#ifdef HINTS_SANITY
      g_assert(tile!=NULL);
#endif

      ptr = tile_data_pointer (tile, 0, ymin);

#ifdef HINTS_SANITY
      g_assert(ptr!=NULL);
#endif

      for (y = ymin; y <= ymax; y++)
	{
	  thishint = tile_get_rowhint (tile, y);

#ifdef HINTS_SANITY
	  if (thishint==TILEROWHINT_BROKEN)
	    g_error("BROKEN y=%d",y);
	  if (thishint==TILEROWHINT_OUTOFRANGE)
	    g_error("OOR y=%d",y);
	  if (thishint==TILEROWHINT_UNDEFINED)
	    g_error("UNDEFINED y=%d - bpp=%d ew=%d eh=%d",
		    y,bpp,ewidth,eheight);
#endif

#ifdef HINTS_SANITY
	  if (thishint == TILEROWHINT_TRANSPARENT ||
	      thishint == TILEROWHINT_MIXED ||
	      thishint == TILEROWHINT_OPAQUE)
	    {
	      goto next_row2;
	    }

	  if (thishint != TILEROWHINT_UNKNOWN)
	    {
	      g_error("MEGABOGUS y=%d - bpp=%d ew=%d eh=%d",
		      y,bpp,ewidth,eheight);
	    }
#endif

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

  g_warning ("update_tile_rowhints: Don't know about tiles with bpp==%d", bpp);
}


static unsigned char *
paint_funcs_get_buffer (int size)
{
  if (size > tmp_buffer_size)
    {
      tmp_buffer_size = size;
      tmp_buffer = (unsigned char *) g_realloc (tmp_buffer, size);
    }

  return tmp_buffer;
}


/*
 * The equations: g(r) = exp (- r^2 / (2 * sigma^2))
 *                   r = sqrt (x^2 + y ^2)
 */

static int *
make_curve (double  sigma,
	    int    *length)
{
  int *curve;
  double sigma2;
  double l;
  int temp;
  int i, n;

  sigma2 = 2 * sigma * sigma;
  l = sqrt (-sigma2 * log (1.0 / 255.0));

  n = ceil (l) * 2;
  if ((n % 2) == 0)
    n += 1;

  curve = g_malloc (sizeof (int) * n);

  *length = n / 2;
  curve += *length;
  curve[0] = 255;

  for (i = 1; i <= *length; i++)
    {
      temp = (int) (exp (- (i * i) / sigma2) * 255);
      curve[-i] = temp;
      curve[i] = temp;
    }

  return curve;
}


static void
run_length_encode (unsigned char *src,
		   int           *dest,
		   int            w,
		   int            bytes)
{
  int start;
  int i;
  int j;
  unsigned char last;

  last = *src;
  src += bytes;
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
      src += bytes;
    }

  for (j = start; j < i; j++)
    {
      *dest++ = (i - j);
      *dest++ = last;
    }
}

#if 0
static void
draw_segments (PixelRegion *destPR,
	       BoundSeg    *bs,
	       int          num_segs,
	       int          off_x,
	       int          off_y,
	       int          opacity)
{
  int x1, y1, x2, y2;
  int tmp, i, length;
  unsigned char *line;

  length = MAXIMUM (destPR->w, destPR->h);
  line = paint_funcs_get_buffer (length);
  memset (line, opacity, length);

  for (i = 0; i < num_segs; i++)
    {
      x1 = bs[i].x1 + off_x;
      y1 = bs[i].y1 + off_y;
      x2 = bs[i].x2 + off_x;
      y2 = bs[i].y2 + off_y;

      if (bs[i].open == 0)
	{
	  /*  If it is vertical  */
	  if (x1 == x2)
	    {
	      x1 -= 1;
	      x2 -= 1;
	    }
	  else
	    {
	      y1 -= 1;
	      y2 -= 1;
	    }
	}

      /*  render segment  */
      x1 = BOUNDS (x1, 0, destPR->w - 1);
      y1 = BOUNDS (y1, 0, destPR->h - 1);
      x2 = BOUNDS (x2, 0, destPR->w - 1);
      y2 = BOUNDS (y2, 0, destPR->h - 1);

      if (x1 == x2)
	{
	  if (y2 < y1)
	    {
	      tmp = y1;
	      y1 = y2;
	      y2 = tmp;
	    }
	  pixel_region_set_col (destPR, x1, y1, (y2 - y1), line);
	}
      else
	{
	  if (x2 < x1)
	    {
	      tmp = x1;
	      x1 = x2;
	      x2 = tmp;
	    }
	  pixel_region_set_row (destPR, x1, y1, (x2 - x1), line);
	}
    }
}
#endif

/* Note: cubic function no longer clips result */
static double
cubic (double dx,
       int    jm1,
       int    j,
       int    jp1,
       int    jp2)
{
  double result;

#if 0
  /* Equivalent to Gimp 1.1.1 and earlier - some ringing */
  result = ((( ( - jm1 + j - jp1 + jp2 ) * dx +
               ( jm1 + jm1 - j - j + jp1 - jp2 ) ) * dx +
               ( - jm1 + jp1 ) ) * dx + j );
  /* Recommended by Mitchell and Netravali - too blurred? */
  result = ((( ( - 7 * jm1 + 21 * j - 21 * jp1 + 7 * jp2 ) * dx +
               ( 15 * jm1 - 36 * j + 27 * jp1 - 6 * jp2 ) ) * dx +
               ( - 9 * jm1 + 9 * jp1 ) ) * dx + (jm1 + 16 * j + jp1) ) / 18.0;
#else

  /* Catmull-Rom - not bad */
  result = ((( ( - jm1 + 3 * j - 3 * jp1 + jp2 ) * dx +
               ( 2 * jm1 - 5 * j + 4 * jp1 - jp2 ) ) * dx +
               ( - jm1 + jp1 ) ) * dx + (j + j) ) / 2.0;

#endif

  return result;
}

/*********************/
/*  FUNCTIONS        */
/*********************/

void
paint_funcs_setup ()
{
  int i;
  int j,k;
  int tmp_sum;

  /*  allocate the temporary buffer  */
  tmp_buffer = (unsigned char *) g_malloc (STD_BUF_SIZE);
  tmp_buffer_size = STD_BUF_SIZE;

  /*  initialize the color hash table--invalidate all entries  */
  for (i = 0; i < HASH_TABLE_SIZE; i++)
    color_hash_table[i].gimage = NULL;
  color_hash_misses = 0;
  color_hash_hits = 0;

  /*  generate a table of random seeds  */
  srand (RANDOM_SEED);
  for (i = 0; i < RANDOM_TABLE_SIZE; i++)
    random_table[i] = rand ();

  for (i = 0; i < RANDOM_TABLE_SIZE; i++)
    {
      int tmp;
      int swap = i + rand () % (RANDOM_TABLE_SIZE - i);
      tmp = random_table[i];
      random_table[i] = random_table[swap];
      random_table[swap] = tmp;
    }

  for (j = 0; j < 256; j++)
    {    /* rows */
      for (k = 0; k < 256; k++)
	{   /* column */
	  tmp_sum = j + k;
	  /* printf("tmp_sum: %d", tmp_sum); */
	  if(tmp_sum > 255)
	    tmp_sum = 255;
	  /* printf("  max: %d  \n", add_lut[j][k]); */
	  add_lut[j][k] = tmp_sum; 
	}
    }

/*   for (j = 0; j < 255; j++) */
/*     {    //rows */
/*       for (k = 0; k < 255; k++) */
/* 	{   //column */
/* 	  printf ("%d",add_lut[j][k]); */
/* 	  printf(" "); */
/* 	} */
/*       printf("\n"); */
/*     } */
  
}


void
paint_funcs_free ()
{
  /*  free the temporary buffer  */
  g_free (tmp_buffer);

  /*  print out the hash table statistics
      printf ("RGB->indexed hash table lookups: %d\n", color_hash_hits + color_hash_misses);
      printf ("RGB->indexed hash table hits: %d\n", color_hash_hits);
      printf ("RGB->indexed hash table misses: %d\n", color_hash_misses);
      printf ("RGB->indexed hash table hit rate: %f\n",
      100.0 * color_hash_hits / (color_hash_hits + color_hash_misses));
      */
}


void
color_pixels (unsigned char *dest,
	      const unsigned char *color,
	      int            w,
	      int            bytes)
{
  /* dest % bytes and color % bytes must be 0 or we will crash 
     when bytes = 2 or 4.
     Is this safe to assume?  Lets find out.
     This is 4-7X as fast as the simple version.
     */
  register unsigned char c0, c1, c2;
  register guint32 *longd, longc;
  register guint16 *shortd, shortc;

  switch (bytes)
  {
   case 1:
     memset(dest, *color, w);
     break;
   case 2:
     shortc = ((guint16 *)color)[0];
     shortd = (guint16 *)dest;
     while (w--)
     {
       *shortd = shortc;
       shortd++;
     }
     break;
   case 3:
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
     break;
   case 4:
     longc = ((guint32 *)color)[0];
     longd = (guint32 *)dest;
     while (w--)
     {
       *longd = longc;
       longd++;
     }
     break;
   default:
   {
     int b;
     while (w--)
     {
       for (b = 0; b < bytes; b++)
	 dest[b] = color[b];

       dest += bytes;
     }
   }
  }
}


void
blend_pixels (const unsigned char *src1,
	      const unsigned char *src2,
	      unsigned char *dest,
	      int            blend,
	      int            w,
	      int            bytes,
	      int            has_alpha)
{
  int b;
  unsigned char blend2 = (255 - blend);

  while (w --)
    {
      for (b = 0; b < bytes; b++)
	dest[b] = (src1[b] * blend2 + src2[b] * blend) / 255;

      src1 += bytes;
      src2 += bytes;
      dest += bytes;
    }
}


void
shade_pixels (const unsigned char *src,
	      unsigned char *dest,
	      const unsigned char *col,
	      int            blend,
	      int            w,
	      int            bytes,
	      int            has_alpha)
{
  int alpha, b;
  unsigned char blend2 = (255 - blend);

  alpha = (has_alpha) ? bytes - 1 : bytes;
  while (w --)
    {
      for (b = 0; b < alpha; b++)
	dest[b] = (src[b] * blend2 + col[b] * blend) / 255;

      if (has_alpha)
	dest[alpha] = src[alpha];  /* alpha channel */

      src += bytes;
      dest += bytes;
    }
}


void
extract_alpha_pixels (const unsigned char *src,
		      const unsigned char *mask,
		      unsigned char *dest,
		      int            w,
		      int            bytes)
{
  int alpha;
  const unsigned char * m;
  int tmp;
  /*  printf("[eap:%d]", w);*/

  if (mask)
    m = mask;
  else
    m = &no_mask;

  alpha = bytes - 1;
  while (w --)
    {
      *dest++ = INT_MULT(src[alpha], *m, tmp);

      if (mask)
	m++;
      src += bytes;
    }
}


void
darken_pixels (const unsigned char *src1,
	       const unsigned char *src2,
	       unsigned char *dest,
	       int            length,
	       int            bytes1,
	       int            bytes2,
	       int            has_alpha1,
	       int            has_alpha2)
{
  int b, alpha;
  unsigned char s1, s2;

  alpha = (has_alpha1 || has_alpha2) ? MAXIMUM (bytes1, bytes2) - 1 : bytes1;

  while (length--)
    {
      for (b = 0; b < alpha; b++)
	{
	  s1 = src1[b];
	  s2 = src2[b];
	  dest[b] = (s1 < s2) ? s1 : s2;
	}

      if (has_alpha1 && has_alpha2)
	dest[alpha] = MIN (src1[alpha], src2[alpha]);
      else if (has_alpha2)
	dest[alpha] = src2[alpha];

      src1 += bytes1;
      src2 += bytes2;
      dest += bytes2;
    }
}


void
lighten_pixels (const unsigned char *src1,
		const unsigned char *src2,
		unsigned char *dest,
		int            length,
		int            bytes1,
		int            bytes2,
		int            has_alpha1,
		int            has_alpha2)
{
  int b, alpha;
  unsigned char s1, s2;

  alpha = (has_alpha1 || has_alpha2) ? MAXIMUM (bytes1, bytes2) - 1 : bytes1;

  while (length--)
    {
      for (b = 0; b < alpha; b++)
	{
	  s1 = src1[b];
	  s2 = src2[b];
	  dest[b] = (s1 < s2) ? s2 : s1;
	}

      if (has_alpha1 && has_alpha2)
	dest[alpha] = MIN (src1[alpha], src2[alpha]);
      else if (has_alpha2)
	dest[alpha] = src2[alpha];

      src1 += bytes1;
      src2 += bytes2;
      dest += bytes2;
    }
}


void
hsv_only_pixels (const unsigned char *src1,
		 const unsigned char *src2,
		 unsigned char *dest,
		 int            mode,
		 int            length,
		 int            bytes1,
		 int            bytes2,
		 int            has_alpha1,
		 int            has_alpha2)
{
  int r1, g1, b1;
  int r2, g2, b2;

  /*  assumes inputs are only 4 byte RGBA pixels  */
  while (length--)
    {
      r1 = src1[0]; g1 = src1[1]; b1 = src1[2];
      r2 = src2[0]; g2 = src2[1]; b2 = src2[2];
      rgb_to_hsv (&r1, &g1, &b1);
      rgb_to_hsv (&r2, &g2, &b2);

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
      hsv_to_rgb (&r1, &g1, &b1);

      dest[0] = r1; dest[1] = g1; dest[2] = b1;

      if (has_alpha1 && has_alpha2)
	dest[3] = MIN (src1[3], src2[3]);
      else if (has_alpha2)
	dest[3] = src2[3];

      src1 += bytes1;
      src2 += bytes2;
      dest += bytes2;
    }
}


void
color_only_pixels (const unsigned char *src1,
		   const unsigned char *src2,
		   unsigned char *dest,
		   int            mode,
		   int            length,
		   int            bytes1,
		   int            bytes2,
		   int            has_alpha1,
		   int            has_alpha2)
{
  int r1, g1, b1;
  int r2, g2, b2;

  /*  assumes inputs are only 4 byte RGBA pixels  */
  while (length--)
    {
      r1 = src1[0]; g1 = src1[1]; b1 = src1[2];
      r2 = src2[0]; g2 = src2[1]; b2 = src2[2];
      rgb_to_hls (&r1, &g1, &b1);
      rgb_to_hls (&r2, &g2, &b2);

      /*  transfer hue and saturation to the source pixel  */
      r1 = r2;
      b1 = b2;

      /*  set the destination  */
      hls_to_rgb (&r1, &g1, &b1);

      dest[0] = r1; dest[1] = g1; dest[2] = b1;

      if (has_alpha1 && has_alpha2)
	dest[3] = MIN (src1[3], src2[3]);
      else if (has_alpha2)
	dest[3] = src2[3];

      src1 += bytes1;
      src2 += bytes2;
      dest += bytes2;
    }
}

void
multiply_pixels (const unsigned char *src1,
		 const unsigned char *src2,
		 unsigned char *dest,
		 int            length,
		 int            bytes1,
		 int            bytes2,
		 int            has_alpha1,
		 int            has_alpha2)
{
  int alpha, b;
  int tmp;
  alpha = (has_alpha1 || has_alpha2) ? MAXIMUM (bytes1, bytes2) - 1 : bytes1;

  if (has_alpha1 && has_alpha2)
    while (length --)
    {
      for (b = 0; b < alpha; b++)
	dest[b] = INT_MULT(src1[b], src2[b], tmp);

      dest[alpha] = MIN (src1[alpha], src2[alpha]);

      src1 += bytes1;
      src2 += bytes2;
      dest += bytes2;
    }
  else if (has_alpha2)
    while (length --)
    {
      for (b = 0; b < alpha; b++)
	dest[b] = INT_MULT(src1[b], src2[b], tmp);

      dest[alpha] = src2[alpha];

      src1 += bytes1;
      src2 += bytes2;
      dest += bytes2;
    }
  else
    while (length --)
    {
      for (b = 0; b < alpha; b++)
	dest[b] = INT_MULT(src1[b], src2[b], tmp);

      src1 += bytes1;
      src2 += bytes2;
      dest += bytes2;
    }
}


void
divide_pixels (const unsigned char *src1,
		 const unsigned char *src2,
		 unsigned char *dest,
		 int            length,
		 int            bytes1,
		 int            bytes2,
		 int            has_alpha1,
		 int            has_alpha2)
{
  int alpha, b, result;

  alpha = (has_alpha1 || has_alpha2) ? MAXIMUM (bytes1, bytes2) - 1 : bytes1;

  while (length --)
    {
      for (b = 0; b < alpha; b++) {
	result = ((src1[b] * 256) / (1+src2[b]));
        dest[b] = MINIMUM(result, 255);
      }

      if (has_alpha1 && has_alpha2)
	dest[alpha] = MIN (src1[alpha], src2[alpha]);
      else if (has_alpha2)
	dest[alpha] = src2[alpha];

      src1 += bytes1;
      src2 += bytes2;
      dest += bytes2;
    }
}


void
screen_pixels (const unsigned char *src1,
	       const unsigned char *src2,
	       unsigned char *dest,
	       int            length,
	       int            bytes1,
	       int            bytes2,
	       int            has_alpha1,
	       int            has_alpha2)
{
  int alpha, b;
  int tmp;

  alpha = (has_alpha1 || has_alpha2) ? MAXIMUM (bytes1, bytes2) - 1 : bytes1;

  while (length --)
    {
      for (b = 0; b < alpha; b++)
	dest[b] = 255 - INT_MULT((255 - src1[b]), (255 - src2[b]), tmp);

      if (has_alpha1 && has_alpha2)
	dest[alpha] = MIN (src1[alpha], src2[alpha]);
      else if (has_alpha2)
	dest[alpha] = src2[alpha];

      src1 += bytes1;
      src2 += bytes2;
      dest += bytes2;
    }
}


void
overlay_pixels (const unsigned char *src1,
		const unsigned char *src2,
		unsigned char *dest,
		int            length,
		int            bytes1,
		int            bytes2,
		int            has_alpha1,
		int            has_alpha2)
{
  int alpha, b;
  int tmp;

  alpha = (has_alpha1 || has_alpha2) ? MAXIMUM (bytes1, bytes2) - 1 : bytes1;

  while (length --)
    {
/*      for (b = 0; b < alpha; b++)
	{
	  screen = 255 - INT_MULT((255 - src1[b]), (255 - src2[b]), tmp);
	  mult = INT_MULT(src1[b] ,src2[b], tmp);
	  dest[b] = INT_BLEND(screen , mult, src1[b], tmp);
	}
	*/
      for (b = 0; b < alpha; b++)
	{
	  dest[b] = INT_MULT(src1[b], src1[b] + INT_MULT(2 * src2[b],
							 255 - src1[b],
							 tmp), tmp);
	}

      if (has_alpha1 && has_alpha2)
	dest[alpha] = MIN (src1[alpha], src2[alpha]);
      else if (has_alpha2)
	dest[alpha] = src2[alpha];

      src1 += bytes1;
      src2 += bytes2;
      dest += bytes2;
    }
}


void
add_pixels (const unsigned char *src1,
	    const unsigned char *src2,
	    unsigned char *dest,
	    int            length,
	    int            bytes1,
	    int            bytes2,
	    int            has_alpha1,
	    int            has_alpha2)
{
  int alpha, b;
  /* int sum; */

  alpha = (has_alpha1 || has_alpha2) ? MAXIMUM (bytes1, bytes2) - 1 : bytes1;

  while (length --)
    {
      for (b = 0; b < alpha; b++)
	{
	  /* sum = src1[b] + src2[b]; */
	  dest[b] = add_lut[ (src1[b])] [(src2[b])];
	  /* dest[b] = MAX255 (sum); */
	  /* dest[b] = sum | ((sum&256) - ((sum&256) >> 8)); */
	  /* dest[b] = (sum > 255) ? 255 : sum; */ /* older, little slower */
	}

      if (has_alpha1 && has_alpha2)
	dest[alpha] = MIN (src1[alpha], src2[alpha]);
      else if (has_alpha2)
	dest[alpha] = src2[alpha];

      src1 += bytes1;
      src2 += bytes2;
      dest += bytes2;
    }
}


void
subtract_pixels (const unsigned char *src1,
		 const unsigned char *src2,
		 unsigned char *dest,
		 int            length,
		 int            bytes1,
		 int            bytes2,
		 int            has_alpha1,
		 int            has_alpha2)
{
  int alpha, b;
  int diff;

  alpha = (has_alpha1 || has_alpha2) ? MAXIMUM (bytes1, bytes2) - 1 : bytes1;

  while (length --)
    {
      for (b = 0; b < alpha; b++)
	{
	  diff = src1[b] - src2[b];
	  dest[b] = (diff < 0) ? 0 : diff;
	}

      if (has_alpha1 && has_alpha2)
	dest[alpha] = MIN (src1[alpha], src2[alpha]);
      else if (has_alpha2)
	dest[alpha] = src2[alpha];

      src1 += bytes1;
      src2 += bytes2;
      dest += bytes2;
    }
}


void
difference_pixels (const unsigned char *src1,
		   const unsigned char *src2,
		   unsigned char *dest,
		   int            length,
		   int            bytes1,
		   int            bytes2,
		   int            has_alpha1,
		   int            has_alpha2)
{
  int alpha, b;
  int diff;

  alpha = (has_alpha1 || has_alpha2) ? MAXIMUM (bytes1, bytes2) - 1 : bytes1;

  while (length --)
    {
      for (b = 0; b < alpha; b++)
	{
	  diff = src1[b] - src2[b];
	  dest[b] = (diff < 0) ? -diff : diff;
	}

      if (has_alpha1 && has_alpha2)
	dest[alpha] = MIN (src1[alpha], src2[alpha]);
      else if (has_alpha2)
	dest[alpha] = src2[alpha];

      src1 += bytes1;
      src2 += bytes2;
      dest += bytes2;
    }
}


void
dissolve_pixels (const unsigned char *src,
		 unsigned char *dest,
		 int            x,
		 int            y,
		 int            opacity,
		 int            length,
		 int            sb,
		 int            db,
		 int            has_alpha)
{
  int alpha, b;
  int rand_val;
  
#if defined(ENABLE_MP) && defined(__GLIBC__)
  /* The glibc 2.1 documentation recommends using the SVID random functions
   * instead of rand_r
   */
  struct drand48_data seed;
  long temp_val;

  srand48_r (random_table[y % RANDOM_TABLE_SIZE], &seed);
  for (b = 0; b < x; b++)
    lrand48_r (&seed, &temp_val);
#elif defined(ENABLE_MP) && !defined(__GLIBC__)
  /* If we are running with multiple threads rand_r give _much_ better
   * performance than rand
   */
  unsigned int seed;
  seed = random_table[y % RANDOM_TABLE_SIZE];
  for (b = 0; b < x; b++)
    rand_r (&seed);
#else
  /* Set up the random number generator */
  srand (random_table[y % RANDOM_TABLE_SIZE]);
  for (b = 0; b < x; b++)
    rand ();
#endif

  alpha = db - 1;

  while (length--)
    {
      /*  preserve the intensity values  */
      for (b = 0; b < alpha; b++)
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
      if (has_alpha)
	dest[alpha] = (rand_val > src[alpha]) ? 0 : src[alpha];
      else
	dest[alpha] = (rand_val > opacity) ? 0 : OPAQUE_OPACITY;

      dest += db;
      src  += sb;
    }
}

void
replace_pixels (unsigned char *src1,
		unsigned char *src2,
		unsigned char *dest,
		unsigned char *mask,
		int            length,
		int            opacity,
		int           *affect,
		int            bytes1,
		int            bytes2)
{
  int alpha;
  int b;
  double a_val, a_recip, mask_val;
  double norm_opacity;
  int s1_a, s2_a;
  int new_val;

  if (bytes1 != bytes2)
    {
      g_warning ("replace_pixels only works on commensurate pixel regions");
      return;
    }

  alpha = bytes1 - 1;
  norm_opacity = opacity * (1.0 / 65025.0);

  while (length --)
    {
      mask_val = mask[0] * norm_opacity;
      /* calculate new alpha first. */
      s1_a = src1[alpha];
      s2_a = src2[alpha];
      a_val = s1_a + mask_val * (s2_a - s1_a);
      if (a_val == 0)
	a_recip = 0;
      else
	a_recip = 1.0 / a_val;
      /* possible optimization: fold a_recip into s1_a and s2_a */
      for (b = 0; b < alpha; b++)
	{
	  new_val = 0.5 + a_recip * (src1[b] * s1_a + mask_val *
				     (src2[b] * s2_a - src1[b] * s1_a));
	  dest[b] = affect[b] ? MIN (new_val, 255) : src1[b];
	}
      dest[alpha] = affect[alpha] ? a_val + 0.5: s1_a;
      src1 += bytes1;
      src2 += bytes2;
      dest += bytes2;
      mask++;
    }
}


void
swap_pixels (unsigned char *src,
	     unsigned char *dest,
	     int            length)
{
  while (length--)
    {
      *src = *src ^ *dest;
      *dest = *dest ^ *src;
      *src = *src ^ *dest;
      src++;
      dest++;
    }
}


void
scale_pixels (const unsigned char *src,
	      unsigned char *dest,
	      int            length,
	      int            scale)
{
  int tmp;
  while (length --)
  {
    *dest++ = (unsigned char) INT_MULT(*src,scale,tmp);
    src++;
  }
}


void
add_alpha_pixels (const unsigned char *src,
		  unsigned char *dest,
		  int            length,
		  int            bytes)
{
  int alpha, b;

  alpha = bytes + 1;
  while (length --)
    {
      for (b = 0; b < bytes; b++)
	dest[b] = src[b];

      dest[b] = OPAQUE_OPACITY;

      src += bytes;
      dest += alpha;
    }
}


void
flatten_pixels (const unsigned char *src,
		unsigned char *dest,
		const unsigned char *bg,
		int            length,
		int            bytes)
{
  int alpha, b;
  int t1, t2;

  alpha = bytes - 1;
  while (length --)
    {
      for (b = 0; b < alpha; b++)
	dest[b] = INT_MULT (src[b], src[alpha], t1) + INT_MULT (bg[b], (255 - src[alpha]), t2);

      src += bytes;
      dest += alpha;
    }
}


void
gray_to_rgb_pixels (const unsigned char *src,
		    unsigned char *dest,
		    int            length,
		    int            bytes)
{
  int b;
  int dest_bytes;
  int has_alpha;

  has_alpha = (bytes == 2) ? 1 : 0;
  dest_bytes = (has_alpha) ? 4 : 3;

  while (length --)
    {
      for (b = 0; b < bytes; b++)
	dest[b] = src[0];

      if (has_alpha)
	dest[3] = src[1];

      src += bytes;
      dest += dest_bytes;
    }
}


void
apply_mask_to_alpha_channel (unsigned char *src,
			     const unsigned char *mask,
			     int            opacity,
			     int            length,
			     int            bytes)
{
  long tmp;
  src += bytes - 1;
  if (opacity == 255)
    while (length --)
    {
      *src = INT_MULT(*src, *mask, tmp);
      mask++;
      src += bytes;
    }
  else
    while (length --)
    {
      *src = INT_MULT3(*src, *mask, opacity, tmp);
      mask++;
      src += bytes;
    }
}


void
combine_mask_and_alpha_channel (unsigned char *src,
				const unsigned char *mask,
				int            opacity,
				int            length,
				int            bytes)
{
  int mask_val;
  int alpha;
  int tmp;
  alpha = bytes - 1;
  src += alpha;

  if (opacity != 255)
    while (length --)
    {
      mask_val = INT_MULT(*mask, opacity, tmp);
      mask++;
      *src = *src + INT_MULT((255 - *src) , mask_val, tmp);
      src += bytes;
    }
  else
    while (length --)
    {
      *src = *src + INT_MULT((255 - *src) , *mask, tmp);
      src += bytes;
      mask++;
    }
}


void
copy_gray_to_inten_a_pixels (const unsigned char *src,
			     unsigned char *dest,
			     int            length,
			     int            bytes)
{
  int b;
  int alpha;

  alpha = bytes - 1;
  while (length --)
    {
      for (b = 0; b < alpha; b++)
	dest[b] = *src;
      dest[b] = OPAQUE_OPACITY;

      src ++;
      dest += bytes;
    }
}


void
initial_channel_pixels (const unsigned char *src,
			unsigned char *dest,
			int            length,
			int            bytes)
{
  int alpha, b;

  alpha = bytes - 1;
  while (length --)
    {
      for (b = 0; b < alpha; b++)
	dest[b] = src[0];

      dest[alpha] = OPAQUE_OPACITY;

      dest += bytes;
      src ++;
    }
}


void
initial_indexed_pixels (const unsigned char *src,
			unsigned char *dest,
			const unsigned char *cmap,
			int            length)
{
  int col_index;

  /*  This function assumes always that we're mapping from
   *  an RGB colormap to an RGBA image...
   */
  while (length--)
    {
      col_index = *src++ * 3;
      *dest++ = cmap[col_index++];
      *dest++ = cmap[col_index++];
      *dest++ = cmap[col_index++];
      *dest++ = OPAQUE_OPACITY;
    }
}


void
initial_indexed_a_pixels (const unsigned char *src,
			  unsigned char *dest,
			  const unsigned char *mask,
			  const unsigned char *cmap,
			  int            opacity,
			  int            length)
{
  int col_index;
  unsigned char new_alpha;
  const unsigned char * m;
  long tmp;
  if (mask)
    m = mask;
  else
    m = &no_mask;

  while (length --)
    {
      col_index = *src++ * 3;
      new_alpha = INT_MULT3(*src, *m, opacity, tmp);
      src++;
      *dest++ = cmap[col_index++];
      *dest++ = cmap[col_index++];
      *dest++ = cmap[col_index++];
      /*  Set the alpha channel  */
      *dest++ = (new_alpha > 127) ? OPAQUE_OPACITY : TRANSPARENT_OPACITY;

      if (mask)
	m++;
    }
}


void
initial_inten_pixels (const unsigned char *src,
		      unsigned char *dest,
		      const unsigned char *mask,
		      int            opacity,
		      const int     *affect,
		      int            length,
		      int            bytes)
{
  int b, dest_bytes;
  const unsigned char * m;
  int tmp;
  int l;
  unsigned char *destp;
  const unsigned char *srcp;

  if (mask)
    m = mask;
  else
    m = &no_mask;

  /*  This function assumes the source has no alpha channel and
   *  the destination has an alpha channel.  So dest_bytes = bytes + 1
   */
  dest_bytes = bytes + 1;

  if (bytes == 3 && affect[0] && affect[1] && affect[2])
  {
    if (!affect[bytes])
      opacity = 0;
    destp = dest + bytes;
    if (mask && opacity != 0)
      while(length--)
      {
	dest[0] = src[0];
	dest[1] = src[1];
	dest[2] = src[2];
	dest[3] = INT_MULT(opacity, *m, tmp);
	src  += bytes;
	dest += dest_bytes;
	m++;
      }
    else
      while(length--)
      {
	dest[0] = src[0];
	dest[1] = src[1];
	dest[2] = src[2];
	dest[3] = opacity;
	src  += bytes;
	dest += dest_bytes;
      }
    return;
  }
  for (b =0; b < bytes; b++)
  {
    destp = dest + b;
    srcp = src + b;
    l = length;
    if (affect[b])
      while(l--)
      {
	*destp = *srcp;
	srcp  += bytes;
	destp += dest_bytes;
      }
    else
      while(l--)
      {
	*destp = 0;
	destp += dest_bytes;
      }
  }
/* fill the alpha channel */ 
  if (!affect[bytes])
    opacity = 0;
  destp = dest + bytes;
  if (mask && opacity != 0)
    while (length--)
    {
      *destp = INT_MULT(opacity , *m, tmp);
      destp += dest_bytes;
      m++;
    }
  else
    while (length--)
    {
      *destp = opacity;
      destp += dest_bytes;
    }
}


void
initial_inten_a_pixels (const unsigned char *src,
			unsigned char       *dest,
			const unsigned char *mask,
			int                  opacity,
			const int           *affect,
			int                  length,
			int                  bytes)
{
  int alpha, b;
  const unsigned char * m;
  long tmp;

  alpha = bytes - 1;
  if (mask)
    {
      m = mask;
      while (length --)
	{
	  for (b = 0; b < alpha; b++)
	    dest[b] = src[b] * affect[b];
	  
	  /*  Set the alpha channel  */
	  dest[alpha] = affect [alpha] ? INT_MULT3(opacity, src[alpha], *m, tmp)
	    : 0;
	  
	  m++;
	  
	  dest += bytes;
	  src += bytes;
	}
    }
  else
    {
      while (length --)
	{
	  for (b = 0; b < alpha; b++)
	    dest[b] = src[b] * affect[b];
	  
	  /*  Set the alpha channel  */
	  dest[alpha] = affect [alpha] ? INT_MULT(opacity , src[alpha], tmp) : 0;
	  
	  dest += bytes;
	  src += bytes;
	}
    }
}


void
combine_indexed_and_indexed_pixels (const unsigned char *src1,
				    const unsigned char *src2,
				    unsigned char       *dest,
				    const unsigned char *mask,
				    int                  opacity,
				    const int           *affect,
				    int                  length,
				    int                  bytes)
{
  int b;
  unsigned char new_alpha;
  const unsigned char * m;
  int tmp;
  if (mask)
    {
      m = mask;
      while (length --)
	{
	  new_alpha = INT_MULT(*m , opacity, tmp);
	  
	  for (b = 0; b < bytes; b++)
	    dest[b] = (affect[b] && new_alpha > 127) ? src2[b] : src1[b];
	  
	  m++;
	  
	  src1 += bytes;
	  src2 += bytes;
	  dest += bytes;
	}
    }
  else
    {
      while (length --)
	{
	  new_alpha = opacity;
	  
	  for (b = 0; b < bytes; b++)
	    dest[b] = (affect[b] && new_alpha > 127) ? src2[b] : src1[b];
	  
	  src1 += bytes;
	  src2 += bytes;
	  dest += bytes;
	}
    }
}


void
combine_indexed_and_indexed_a_pixels (const unsigned char *src1,
				      const unsigned char *src2,
				      unsigned char       *dest,
				      const unsigned char *mask,
				      int                  opacity,
				      const int           *affect,
				      int                  length,
				      int                  bytes)
{
  int b, alpha;
  unsigned char new_alpha;
  const unsigned char * m;
  int src2_bytes;
  long tmp;
  alpha = 1;
  src2_bytes = 2;

  if (mask)
    {
      m = mask;
      while (length --)
	{
	  new_alpha = INT_MULT3(src2[alpha], *m, opacity, tmp);

	  for (b = 0; b < bytes; b++)
	    dest[b] = (affect[b] && new_alpha > 127) ? src2[b] : src1[b];

	  m++;

	  src1 += bytes;
	  src2 += src2_bytes;
	  dest += bytes;
	}
    }
  else
    {
      while (length --)
	{
	  new_alpha = INT_MULT(src2[alpha], opacity, tmp);

	  for (b = 0; b < bytes; b++)
	    dest[b] = (affect[b] && new_alpha > 127) ? src2[b] : src1[b];

	  src1 += bytes;
	  src2 += src2_bytes;
	  dest += bytes;
	}
    }
}


void
combine_indexed_a_and_indexed_a_pixels (const unsigned char *src1,
					const unsigned char *src2,
					unsigned char       *dest,
					const unsigned char *mask,
					int                  opacity,
					const int           *affect,
					int                  length,
					int                  bytes)
{
  int b, alpha;
  unsigned char new_alpha;
  const unsigned char * m;
  long tmp;

  alpha = 1;

  if (mask)
    {
      m = mask;
      while (length --)
	{
	  new_alpha = INT_MULT3(src2[alpha], *m, opacity, tmp);

	  for (b = 0; b < alpha; b++)
	    dest[b] = (affect[b] && new_alpha > 127) ? src2[b] : src1[b];

	  dest[alpha] = (affect[alpha] && new_alpha > 127) ? OPAQUE_OPACITY : src1[alpha];

	  m++;

	  src1 += bytes;
	  src2 += bytes;
	  dest += bytes;
	}
    }
  else
    {
      while (length --)
	{
	  new_alpha = INT_MULT(src2[alpha], opacity, tmp);

	  for (b = 0; b < alpha; b++)
	    dest[b] = (affect[b] && new_alpha > 127) ? src2[b] : src1[b];

	  dest[alpha] = (affect[alpha] && new_alpha > 127) ? OPAQUE_OPACITY : src1[alpha];

	  src1 += bytes;
	  src2 += bytes;
	  dest += bytes;
	}
    }
}


void
combine_inten_a_and_indexed_a_pixels (const unsigned char *src1,
				      const unsigned char *src2,
				      unsigned char       *dest,
				      const unsigned char *mask,
				      const unsigned char *cmap,
				      int                  opacity,
				      int                  length,
				      int                  bytes)
{
  int b, alpha;
  unsigned char new_alpha;
  int src2_bytes;
  int index;
  long tmp;

  alpha = 1;
  src2_bytes = 2;

  if (mask)
    {
      const unsigned char *m = mask;
      while (length --)
	{
	  new_alpha = INT_MULT3(src2[alpha], *m, opacity, tmp);

	  index = src2[0] * 3;

	  for (b = 0; b < bytes-1; b++)
	    dest[b] = (new_alpha > 127) ? cmap[index + b] : src1[b];

	  dest[b] = (new_alpha > 127) ? OPAQUE_OPACITY : src1[b];  /*  alpha channel is opaque  */

	  m++;

	  src1 += bytes;
	  src2 += src2_bytes;
	  dest += bytes;
	}
    }
  else
    {
      while (length --)
	{
	  new_alpha = INT_MULT(src2[alpha], opacity, tmp);

	  index = src2[0] * 3;

	  for (b = 0; b < bytes-1; b++)
	    dest[b] = (new_alpha > 127) ? cmap[index + b] : src1[b];

	  dest[b] = (new_alpha > 127) ? OPAQUE_OPACITY : src1[b];  /*  alpha channel is opaque  */

	  /* m++; /Per */

	  src1 += bytes;
	  src2 += src2_bytes;
	  dest += bytes;
	}
    }
}


void
combine_inten_and_inten_pixels (const unsigned char *src1,
				const unsigned char *src2,
				unsigned char       *dest,
				const unsigned char *mask,
				int                  opacity,
				const int           *affect,
				int                  length,
				int                  bytes)
{
  int b;
  unsigned char new_alpha;
  const unsigned char * m;
  int tmp;
  if (mask)
    {
      m = mask;
      while (length --)
	{
	  new_alpha = INT_MULT(*m, opacity, tmp);

	  for (b = 0; b < bytes; b++)
	    dest[b] = (affect[b]) ?
	      INT_BLEND(src2[b], src1[b], new_alpha, tmp) :
	    src1[b];

	  m++;

	  src1 += bytes;
	  src2 += bytes;
	  dest += bytes;
	}
    }
  else
    {
      while (length --)
	{

	  for (b = 0; b < bytes; b++)
	    dest[b] = (affect[b]) ?
	      INT_BLEND(src2[b], src1[b], opacity, tmp) :
	    src1[b];

	  src1 += bytes;
	  src2 += bytes;
	  dest += bytes;
	}
    }
}


void
combine_inten_and_inten_a_pixels (const unsigned char *src1,
				  const unsigned char *src2,
				  unsigned char       *dest,
				  const unsigned char *mask,
				  int                  opacity,
				  const int           *affect,
				  int                  length,
				  int                  bytes)
{
  int alpha, b;
  int src2_bytes;
  unsigned char new_alpha;
  const unsigned char * m;
  register long t1;
  alpha = bytes;
  src2_bytes = bytes + 1;

  if (mask)
    {
      m = mask;
      while (length --)
	{
	  new_alpha = INT_MULT3(src2[alpha], *m, opacity, t1);

	  for (b = 0; b < bytes; b++)
	    dest[b] = (affect[b]) ?
	      INT_BLEND(src2[b], src1[b], new_alpha, t1) :
	      src1[b];

	  m++;
	  src1 += bytes;
	  src2 += src2_bytes;
	  dest += bytes;
	}
    }
  else
    {
      if (bytes == 3 && affect[0] && affect[1] && affect[2])
	while (length --)
	{
	  new_alpha = INT_MULT(src2[alpha],opacity,t1);
	  dest[0] = INT_BLEND(src2[0] , src1[0] , new_alpha, t1);
	  dest[1] = INT_BLEND(src2[1] , src1[1] , new_alpha, t1);
	  dest[2] = INT_BLEND(src2[2] , src1[2] , new_alpha, t1);
	  src1 += bytes;
	  src2 += src2_bytes;
	  dest += bytes;
	}
      else
	while (length --)
	{
	  new_alpha = INT_MULT(src2[alpha],opacity,t1);
	  for (b = 0; b < bytes; b++)
	    dest[b] = (affect[b]) ?
	      INT_BLEND(src2[b] , src1[b] , new_alpha, t1) :
	    src1[b];

	  src1 += bytes;
	  src2 += src2_bytes;
	  dest += bytes;
	}
    }
}

/*orig #define alphify(src2_alpha,new_alpha) \
	if (new_alpha == 0 || src2_alpha == 0)							\
	  {											\
	    for (b = 0; b < alpha; b++)								\
	      dest[b] = src1 [b];								\
	  }											\
	else if (src2_alpha == new_alpha){							\
	  for (b = 0; b < alpha; b++)								\
	    dest [b] = affect [b] ? src2 [b] : src1 [b];					\
	} else {										\
	  ratio = (float) src2_alpha / new_alpha;						\
	  compl_ratio = 1.0 - ratio;								\
	  											\
	  for (b = 0; b < alpha; b++)								\
	    dest[b] = affect[b] ?								\
	      (unsigned char) (src2[b] * ratio + src1[b] * compl_ratio + EPSILON) : src1[b];	\
	}*/

/*shortened #define alphify(src2_alpha,new_alpha) \
	if (src2_alpha != 0 && new_alpha != 0)							\
	  {											\
	    if (src2_alpha == new_alpha){							\
	      for (b = 0; b < alpha; b++)							\
	      dest [b] = affect [b] ? src2 [b] : src1 [b];					\
	    } else {										\
	      ratio = (float) src2_alpha / new_alpha;						\
	      compl_ratio = 1.0 - ratio;							\
	  											\
	      for (b = 0; b < alpha; b++)							\
	        dest[b] = affect[b] ?								\
	          (unsigned char) (src2[b] * ratio + src1[b] * compl_ratio + EPSILON) : src1[b];\
	    }                                                                                   \
	  }*/

#define alphify(src2_alpha,new_alpha) \
	if (src2_alpha != 0 && new_alpha != 0)							\
	  {											\
            b = alpha; \
	    if (src2_alpha == new_alpha){							\
	      do { \
	      b--; dest [b] = affect [b] ? src2 [b] : src1 [b];} while (b);	\
	    } else {										\
	      ratio = (float) src2_alpha / new_alpha;						\
	      compl_ratio = 1.0 - ratio;							\
	  											\
              do { b--; \
	        dest[b] = affect[b] ?								\
	          (unsigned char) (src2[b] * ratio + src1[b] * compl_ratio + EPSILON) : src1[b];\
         	  } while (b); \
	    }    \
	  }

/*special #define alphify4(src2_alpha,new_alpha) \
	if (src2_alpha != 0 && new_alpha != 0)							\
	  {											\
	    if (src2_alpha == new_alpha){							\
	      dest [0] = affect [0] ? src2 [0] : src1 [0];					\
	      dest [1] = affect [1] ? src2 [1] : src1 [1];					\
	      dest [2] = affect [2] ? src2 [2] : src1 [2];					\
	    } else {										\
	      ratio = (float) src2_alpha / new_alpha;						\
	      compl_ratio = 1.0 - ratio;							\
	  											\
	      dest[0] = affect[0] ?								\
	        (unsigned char) (src2[0] * ratio + src1[0] * compl_ratio + EPSILON) : src1[0];  \
	      dest[1] = affect[1] ?								\
	        (unsigned char) (src2[1] * ratio + src1[1] * compl_ratio + EPSILON) : src1[1];  \
	      dest[2] = affect[2] ?								\
	        (unsigned char) (src2[2] * ratio + src1[2] * compl_ratio + EPSILON) : src1[2];  \
	    }                                                                                   \
	  }*/
	
void
combine_inten_a_and_inten_pixels (const unsigned char *src1,
				  const unsigned char *src2,
				  unsigned char       *dest,
				  const unsigned char *mask,
				  int                  opacity,
				  const int           *affect,
				  int                  mode_affect,  /*  how does the combination mode affect alpha?  */
				  int                  length,
				  int                  bytes)  /*  4 or 2 depending on RGBA or GRAYA  */
{
  int alpha, b;
  int src2_bytes;
  unsigned char src2_alpha;
  unsigned char new_alpha;
  const unsigned char * m;
  float ratio, compl_ratio;
  long tmp;

  src2_bytes = bytes - 1;
  alpha = bytes - 1;

  if (mask)
    {
      m = mask;
      if (opacity == OPAQUE_OPACITY) /* HAS MASK, FULL OPACITY */
	{
	  while (length--)
	    {
	      src2_alpha = *m;
	      new_alpha = src1[alpha] +
		INT_MULT((255 - src1[alpha]), src2_alpha, tmp);
	      alphify (src2_alpha, new_alpha);
	      
	      if (mode_affect)
		{
		  dest[alpha] = (affect[alpha]) ? new_alpha : src1[alpha];
		}
	      else
		{
		  dest[alpha] = (src1[alpha]) ? src1[alpha] :
		    (affect[alpha] ? new_alpha : src1[alpha]);
		}
		
	      m++;
	      src1 += bytes;
	      src2 += src2_bytes;
	      dest += bytes;
	    }
	}
      else /* HAS MASK, SEMI-OPACITY */
	{
	  while (length--)
	    {
	      src2_alpha = INT_MULT(*m, opacity, tmp);
	      new_alpha = src1[alpha] +
		INT_MULT((255 - src1[alpha]), src2_alpha, tmp);
	      alphify (src2_alpha, new_alpha);
	      
	      if (mode_affect)
		{
		  dest[alpha] = (affect[alpha]) ? new_alpha : src1[alpha];
		}
	      else
		{
		  dest[alpha] = (src1[alpha]) ? src1[alpha] :
		    (affect[alpha] ? new_alpha : src1[alpha]);
		}
		
	      m++;
	      src1 += bytes;
	      src2 += src2_bytes;
	      dest += bytes;
	    }
	}
    }
  else /* NO MASK */
    {
      while (length --)
	{
	  src2_alpha = opacity;
	  new_alpha = src1[alpha] +
	    INT_MULT((255 - src1[alpha]), src2_alpha, tmp);
	  alphify (src2_alpha, new_alpha);
	  
	  if (mode_affect)
	    dest[alpha] = (affect[alpha]) ? new_alpha : src1[alpha];
	  else
	    dest[alpha] = (src1[alpha]) ? src1[alpha] : (affect[alpha] ? new_alpha : src1[alpha]);

	    src1 += bytes;
	    src2 += src2_bytes;
	    dest += bytes;
	}
    }
}


void
combine_inten_a_and_inten_a_pixels (const unsigned char *src1,
				    const unsigned char *src2,
				    unsigned char       *dest,
				    const unsigned char *mask,
				    int                 opacity,
				    const int          *affect,
				    int                 mode_affect,  /*  how does the combination mode affect alpha?  */
				    int                 length,
				    int                 bytes)  /*  4 or 2 depending on RGBA or GRAYA  */
{
  int alpha, b;
  unsigned char src2_alpha;
  unsigned char new_alpha;
  const unsigned char * m;
  float ratio, compl_ratio;
  long tmp;
  alpha = bytes - 1;

  if (mask)
    {
      m = mask;

      if (opacity == OPAQUE_OPACITY) /* HAS MASK, FULL OPACITY */
	{
	  const int* mask_ip;
	  int i,j;

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
		      src2_alpha = INT_MULT(src2[alpha], *m, tmp);
		      new_alpha = src1[alpha] +
			INT_MULT((255 - src1[alpha]), src2_alpha, tmp);
		  
		      alphify (src2_alpha, new_alpha);
		  
		      if (mode_affect)
			{
			  dest[alpha] = (affect[alpha]) ? new_alpha : src1[alpha];
			}
		      else
			{
			  dest[alpha] = (src1[alpha]) ? src1[alpha] :
			    (affect[alpha] ? new_alpha : src1[alpha]);      
			}
		  
		      m++;
		      src1 += bytes;
		      src2 += bytes;
		      dest += bytes;	
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
		      m = (const unsigned char*)mask_ip;
		      j = sizeof(int);
		      while (j--)
			{
			  /* GUTS */
			  src2_alpha = INT_MULT(src2[alpha], *m, tmp);
			  new_alpha = src1[alpha] +
			    INT_MULT((255 - src1[alpha]), src2_alpha, tmp);
		  
			  alphify (src2_alpha, new_alpha);
		  
			  if (mode_affect)
			    {
			      dest[alpha] = (affect[alpha]) ? new_alpha : src1[alpha];
			    }
			  else
			    {
			      dest[alpha] = (src1[alpha]) ? src1[alpha] :
				(affect[alpha] ? new_alpha : src1[alpha]);      
			    }
		  
			  m++;
			  src1 += bytes;
			  src2 += bytes;
			  dest += bytes;	
			  /* GUTS END */
			}
		    }
		  else
		    {
		      j = bytes * sizeof(int);
		      src2 += j;
		      while (j--)
			{
			  *(dest++) = *(src1++);
			}
		    }
		  mask_ip++;
		}

	      m = (const unsigned char*)mask_ip;
	    }

	  /* TAIL */
	  while (length--)
	    {
	      /* GUTS */
	      src2_alpha = INT_MULT(src2[alpha], *m, tmp);
	      new_alpha = src1[alpha] +
		INT_MULT((255 - src1[alpha]), src2_alpha, tmp);
		  
	      alphify (src2_alpha, new_alpha);
		  
	      if (mode_affect)
		{
		  dest[alpha] = (affect[alpha]) ? new_alpha : src1[alpha];
		}
	      else
		{
		  dest[alpha] = (src1[alpha]) ? src1[alpha] :
		    (affect[alpha] ? new_alpha : src1[alpha]);      
		}
		  
	      m++;
	      src1 += bytes;
	      src2 += bytes;
	      dest += bytes;	
	      /* GUTS END */
	    }
	}
      else /* HAS MASK, SEMI-OPACITY */
	{
	  const int* mask_ip;
	  int i,j;

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
		      src2_alpha = INT_MULT3(src2[alpha], *m, opacity, tmp);
		      new_alpha = src1[alpha] +
			INT_MULT((255 - src1[alpha]), src2_alpha, tmp);
		  
		      alphify (src2_alpha, new_alpha);
		  
		      if (mode_affect)
			{
			  dest[alpha] = (affect[alpha]) ? new_alpha : src1[alpha];
			}
		      else
			{
			  dest[alpha] = (src1[alpha]) ? src1[alpha] :
			    (affect[alpha] ? new_alpha : src1[alpha]);
			}
		  
		      m++;
		      src1 += bytes;
		      src2 += bytes;
		      dest += bytes;
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
		      m = (const unsigned char*)mask_ip;
		      j = sizeof(int);
		      while (j--)
			{
			  /* GUTS */
			  src2_alpha = INT_MULT3(src2[alpha], *m, opacity, tmp);
			  new_alpha = src1[alpha] +
			    INT_MULT((255 - src1[alpha]), src2_alpha, tmp);
		      
			  alphify (src2_alpha, new_alpha);
		      
			  if (mode_affect)
			    {
			      dest[alpha] = (affect[alpha]) ? new_alpha : src1[alpha];
			    }
			  else
			    {
			      dest[alpha] = (src1[alpha]) ? src1[alpha] :
				(affect[alpha] ? new_alpha : src1[alpha]);
			    }

			  m++;
			  src1 += bytes;
			  src2 += bytes;
			  dest += bytes;
			  /* GUTS END */
			}
		    }
		  else
		    {
		      j = bytes * sizeof(int);
		      src2 += j;
		      while (j--)
			{
			  *(dest++) = *(src1++);
			}
		    }
		  mask_ip++;
		}

	      m = (const unsigned char*)mask_ip;
	    }
	
	  /* TAIL */
	  while (length--)
	    {
	      /* GUTS */
	      src2_alpha = INT_MULT3(src2[alpha], *m, opacity, tmp);
	      new_alpha = src1[alpha] +
		INT_MULT((255 - src1[alpha]), src2_alpha, tmp);
	      
	      alphify (src2_alpha, new_alpha);
	      
	      if (mode_affect)
		{
		  dest[alpha] = (affect[alpha]) ? new_alpha : src1[alpha];
		}
	      else
		{
		  dest[alpha] = (src1[alpha]) ? src1[alpha] :
		    (affect[alpha] ? new_alpha : src1[alpha]);
		}
	      
	      m++;
	      src1 += bytes;
	      src2 += bytes;
	      dest += bytes;
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
	      src2_alpha = src2[alpha];
	      new_alpha = src1[alpha] +
		INT_MULT((255 - src1[alpha]), src2_alpha, tmp);
	      
	      alphify (src2_alpha, new_alpha);
	      
	      if (mode_affect)
		{
		  dest[alpha] = (affect[alpha]) ? new_alpha : src1[alpha];
		}
	      else
		{
		  dest[alpha] = (src1[alpha]) ? src1[alpha] :
		    (affect[alpha] ? new_alpha : src1[alpha]);
		}
		
	      src1 += bytes;
	      src2 += bytes;
	      dest += bytes;
	    }
	}
      else /* NO MASK, SEMI OPACITY */
	{
	  while (length --)
	    {
	      src2_alpha = INT_MULT(src2[alpha], opacity, tmp);
	      new_alpha = src1[alpha] +
		INT_MULT((255 - src1[alpha]), src2_alpha, tmp);
	      
	      alphify (src2_alpha, new_alpha);
	      
	      if (mode_affect)
		{
		  dest[alpha] = (affect[alpha]) ? new_alpha : src1[alpha];
		}
	      else
		{
		  dest[alpha] = (src1[alpha]) ? src1[alpha] :
		    (affect[alpha] ? new_alpha : src1[alpha]);
		}
		
	      src1 += bytes;
	      src2 += bytes;
	      dest += bytes;	  
	    }
	}
    }
}
#undef alphify

void
combine_inten_a_and_channel_mask_pixels (const unsigned char *src,
					 const unsigned char *channel,
					 unsigned char       *dest,
					 const unsigned char *col,
					 int                  opacity,
					 int                  length,
					 int                  bytes)
{
  int alpha, b;
  unsigned char channel_alpha;
  unsigned char new_alpha;
  unsigned char compl_alpha;
  int t, s;

  alpha = bytes - 1;
  while (length --)
    {
      channel_alpha = INT_MULT (255 - *channel, opacity, t);
      if (channel_alpha)
	{
	  new_alpha = src[alpha] + INT_MULT ((255 - src[alpha]), channel_alpha, t);

	  if (new_alpha != 255)
	    channel_alpha = (channel_alpha * 255) / new_alpha;
	  compl_alpha = 255 - channel_alpha;

	  for (b = 0; b < alpha; b++)
	    dest[b] = INT_MULT (col[b], channel_alpha, t) +
	      INT_MULT (src[b], compl_alpha, s);
	  dest[b] = new_alpha;
	}
      else
	memcpy(dest, src, bytes);

      /*  advance pointers  */
      src+=bytes;
      dest+=bytes;
      channel++;
    }
}


void
combine_inten_a_and_channel_selection_pixels (const unsigned char *src,
					      const unsigned char *channel,
					      unsigned char       *dest,
					      const unsigned char *col,
					      int                  opacity,
					      int                  length,
					      int                  bytes)
{
  int alpha, b;
  unsigned char channel_alpha;
  unsigned char new_alpha;
  unsigned char compl_alpha;
  int t, s;

  alpha = bytes - 1;
  while (length --)
    {
      channel_alpha = INT_MULT (*channel, opacity, t);
      if (channel_alpha)
	{
	  new_alpha = src[alpha] + INT_MULT ((255 - src[alpha]), channel_alpha, t);

	  if (new_alpha != 255)
	    channel_alpha = (channel_alpha * 255) / new_alpha;
	  compl_alpha = 255 - channel_alpha;

	  for (b = 0; b < alpha; b++)
	    dest[b] = INT_MULT (col[b], channel_alpha, t) +
	      INT_MULT (src[b], compl_alpha, s);
	  dest[b] = new_alpha;
	}
      else
	memcpy(dest, src, bytes);

      /*  advance pointers  */
      src+=bytes;
      dest+=bytes;
      channel++;
    }
}


void
behind_inten_pixels (const unsigned char *src1,
		     const unsigned char *src2,
		     unsigned char       *dest,
		     const unsigned char *mask,
		     int                  opacity,
		     const int           *affect,
		     int                  length,
		     int                  bytes1,
		     int                  bytes2,
		     int                  has_alpha1,
		     int                  has_alpha2)
{
  int alpha, b;
  unsigned char src1_alpha;
  unsigned char src2_alpha;
  unsigned char new_alpha;
  const unsigned char * m;
  float ratio, compl_ratio;
  long tmp;
  if (mask)
    m = mask;
  else
    m = &no_mask;

  /*  the alpha channel  */
  alpha = bytes1 - 1;

  while (length --)
    {
      src1_alpha = src1[alpha];
      src2_alpha = INT_MULT3(src2[alpha], *m, opacity, tmp);
      new_alpha = src2_alpha +
	INT_MULT((255 - src2_alpha), src1_alpha, tmp);
      if (new_alpha)
	ratio = (float) src1_alpha / new_alpha;
      else
	ratio = 0.0;
      compl_ratio = 1.0 - ratio;

      for (b = 0; b < alpha; b++)
	dest[b] = (affect[b]) ?
	  (unsigned char) (src1[b] * ratio + src2[b] * compl_ratio + EPSILON) :
	src1[b];

      dest[alpha] = (affect[alpha]) ? new_alpha : src1[alpha];

      if (mask)
	m++;

      src1 += bytes1;
      src2 += bytes2;
      dest += bytes1;
    }
}


void
behind_indexed_pixels (const unsigned char *src1,
		       const unsigned char *src2,
		       unsigned char       *dest,
		       const unsigned char *mask,
		       int                  opacity,
		       const int           *affect,
		       int                  length,
		       int                  bytes1,
		       int                  bytes2,
		       int                  has_alpha1,
		       int                  has_alpha2)
{
  int alpha, b;
  unsigned char src1_alpha;
  unsigned char src2_alpha;
  unsigned char new_alpha;
  const unsigned char * m;
  long tmp;

  if (mask)
    m = mask;
  else
    m = &no_mask;

  /*  the alpha channel  */
  alpha = bytes1 - 1;

  while (length --)
    {
      src1_alpha = src1[alpha];
      src2_alpha = INT_MULT3(src2[alpha], *m, opacity, tmp);
      new_alpha = (src2_alpha > 127) ? OPAQUE_OPACITY : TRANSPARENT_OPACITY;

      for (b = 0; b < bytes1; b++)
	dest[b] = (affect[b] && new_alpha == OPAQUE_OPACITY && (src1_alpha > 127)) ?
	  src2[b] : src1[b];

      if (mask)
	m++;

      src1 += bytes1;
      src2 += bytes2;
      dest += bytes1;
    }
}


void
replace_inten_pixels (const unsigned char *src1,
		      const unsigned char *src2,
		      unsigned char       *dest,
		      const unsigned char *mask,
		      int                  opacity,
		      const int           *affect,
		      int                  length,
		      int                  bytes1,
		      int                  bytes2,
		      int                  has_alpha1,
		      int                  has_alpha2)
{
  int bytes, b;
  unsigned char mask_alpha;
  const unsigned char * m;
  int tmp;
  if (mask)
    m = mask;
  else
    m = &no_mask;

  bytes = MINIMUM (bytes1, bytes2);
  while (length --)
    {
      mask_alpha = INT_MULT(*m, opacity, tmp);

      for (b = 0; b < bytes; b++)
	dest[b] = (affect[b]) ?
	  INT_BLEND(src2[b], src1[b], mask_alpha, tmp) :
	src1[b];

      if (has_alpha1 && !has_alpha2)
	dest[b] = src1[b];

      if (mask)
	m++;

      src1 += bytes1;
      src2 += bytes2;
      dest += bytes1;
    }
}


void
replace_indexed_pixels (const unsigned char *src1,
			const unsigned char *src2,
			unsigned char       *dest,
			const unsigned char *mask,
			int                  opacity,
			const int           *affect,
			int                  length,
			int                  bytes1,
			int                  bytes2,
			int                  has_alpha1,
			int                  has_alpha2)
{
  int bytes, b;
  unsigned char mask_alpha;
  const unsigned char * m;
  int tmp;
  if (mask)
    m = mask;
  else
    m = &no_mask;

  bytes = MINIMUM (bytes1, bytes2);
  while (length --)
    {
      mask_alpha = INT_MULT(*m, opacity, tmp);

      for (b = 0; b < bytes; b++)
	dest[b] = (affect[b] && mask_alpha) ?
	  src2[b] : src1[b];

      if (has_alpha1 && !has_alpha2)
	dest[b] = src1[b];

      if (mask)
	m++;

      src1 += bytes1;
      src2 += bytes2;
      dest += bytes1;
    }
}


void
erase_inten_pixels (const unsigned char *src1,
		    const unsigned char *src2,
		    unsigned char       *dest,
		    const unsigned char *mask,
		    int                  opacity,
		    const int           *affect,
		    int                  length,
		    int                  bytes)
{
  int alpha, b;
  unsigned char src2_alpha;
  const unsigned char * m;
  long tmp;

  if (mask)
    m = mask;
  else
    m = &no_mask;

  alpha = bytes - 1;
  while (length --)
    {
      for (b = 0; b < alpha; b++)
	dest[b] = src1[b];

      src2_alpha = INT_MULT3(src2[alpha], *m, opacity, tmp);
      dest[alpha] = src1[alpha] - INT_MULT(src1[alpha], src2_alpha, tmp);

      if (mask)
	m++;

      src1 += bytes;
      src2 += bytes;
      dest += bytes;
    }
}


void
erase_indexed_pixels (const unsigned char *src1,
		      const unsigned char *src2,
		      unsigned char       *dest,
		      const unsigned char *mask,
		      int                  opacity,
		      const int           *affect,
		      int                  length,
		      int                  bytes)
{
  int alpha, b;
  unsigned char src2_alpha;
  const unsigned char * m;
  long tmp;
  if (mask)
    m = mask;
  else
    m = &no_mask;

  alpha = bytes - 1;
  while (length --)
    {
      for (b = 0; b < alpha; b++)
	dest[b] = src1[b];

      src2_alpha = INT_MULT3(src2[alpha], *m, opacity, tmp);
      dest[alpha] = (src2_alpha > 127) ? TRANSPARENT_OPACITY : src1[alpha];

      if (mask)
	m++;

      src1 += bytes;
      src2 += bytes;
      dest += bytes;
    }
}

void
anti_erase_inten_pixels (const unsigned char *src1,
			 const unsigned char *src2,
			 unsigned char       *dest,
			 const unsigned char *mask,
			 int                  opacity,
			 const int           *affect,
			 int                  length,
			 int                  bytes)
{
  int alpha, b;
  unsigned char src2_alpha;
  const unsigned char * m;
  long tmp;

  if (mask)
    m = mask;
  else
    m = &no_mask;

  alpha = bytes - 1;
  while (length --)
    {
      for (b = 0; b < alpha; b++)
	dest[b] = src1[b];

      src2_alpha = INT_MULT3(src2[alpha], *m, opacity, tmp);
      dest[alpha] = src1[alpha] + INT_MULT((255 - src1[alpha]), src2_alpha, tmp);

      if (mask)
	m++;

      src1 += bytes;
      src2 += bytes;
      dest += bytes;
    }
}

void
anti_erase_indexed_pixels (const unsigned char *src1,
			   const unsigned char *src2,
			   unsigned char       *dest,
			   const unsigned char *mask,
			   int                  opacity,
			   const int           *affect,
			   int                  length,
			   int                  bytes)
{
  int alpha, b;
  unsigned char src2_alpha;
  const unsigned char * m;
  long tmp;
  if (mask)
    m = mask;
  else
    m = &no_mask;

  alpha = bytes - 1;
  while (length --)
    {
      for (b = 0; b < alpha; b++)
	dest[b] = src1[b];

      src2_alpha = INT_MULT3(src2[alpha], *m, opacity, tmp);
      dest[alpha] = (src2_alpha > 127) ? OPAQUE_OPACITY : src1[alpha];

      if (mask)
	m++;

      src1 += bytes;
      src2 += bytes;
      dest += bytes;
    }
}


void
extract_from_inten_pixels (unsigned char *src,
			   unsigned char       *dest,
			   const unsigned char *mask,
			   const unsigned char *bg,
			   int                  cut,
			   int                  length,
			   int                  bytes,
			   int                  has_alpha)
{
  int b, alpha;
  int dest_bytes;
  const unsigned char * m;
  int tmp;
  if (mask)
    m = mask;
  else
    m = &no_mask;

  alpha = (has_alpha) ? bytes - 1 : bytes;
  dest_bytes = (has_alpha) ? bytes : bytes + 1;
  while (length --)
    {
      for (b = 0; b < alpha; b++)
	dest[b] = src[b];

      if (has_alpha)
	{
	  dest[alpha] = INT_MULT(*m, src[alpha], tmp);
	  if (cut)
	    src[alpha] = INT_MULT((255 - *m), src[alpha], tmp);
	}
      else
	{
	  dest[alpha] = *m;
	  if (cut)
	    for (b = 0; b < bytes; b++)
	      src[b] = INT_BLEND(bg[b], src[b], *m, tmp);
	}

      if (mask)
	m++;

      src += bytes;
      dest += dest_bytes;
    }
}


void
extract_from_indexed_pixels (unsigned char       *src,
			     unsigned char       *dest,
			     const unsigned char *mask,
			     const unsigned char *cmap,
			     const unsigned char *bg,
			     int                  cut,
			     int                  length,
			     int                  bytes,
			     int                  has_alpha)
{
  int b;
  int index;
  const unsigned char * m;
  int t;

  if (mask)
    m = mask;
  else
    m = &no_mask;

  while (length --)
    {
      index = src[0] * 3;
      for (b = 0; b < 3; b++)
	dest[b] = cmap[index + b];

      if (has_alpha)
	{
	  dest[3] = INT_MULT (*m, src[1], t);
	  if (cut)
	    src[1] = INT_MULT ((255 - *m), src[1], t);
	}
      else
	{
	  dest[3] = *m;
	  if (cut)
	    src[0] = (*m > 127) ? bg[0] : src[0];
	}

      if (mask)
	m++;

      src += bytes;
      dest += 4;
    }
}


void
map_to_color (int                  src_type,
	      const unsigned char *cmap,
	      const unsigned char *src,
	      unsigned char       *rgb)
{
  switch (src_type)
    {
    case 0:  /*  RGB      */
      /*  Straight copy  */
      *rgb++ = *src++;
      *rgb++ = *src++;
      *rgb   = *src;
      break;
    case 1:  /*  GRAY     */
      *rgb++ = *src;
      *rgb++ = *src;
      *rgb   = *src;
      break;
    case 2:  /*  INDEXED  */
      {
	int index = *src * 3;
	*rgb++ = cmap [index++];
	*rgb++ = cmap [index++];
	*rgb   = cmap [index++];
      }
      break;
    }
}


int
map_rgb_to_indexed (const unsigned char *cmap,
		    int            num_cols,
		    GimpImage*     gimage,
		    int            r,
		    int            g,
		    int            b)
{
  unsigned int pixel;
  int hash_index;
  int cmap_index;

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
      const unsigned char *col;
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
      color_hash_table[hash_index].gimage = gimage;
      color_hash_misses++;
    }

  return cmap_index;
}



/**************************************************/
/*    REGION FUNCTIONS                            */
/**************************************************/

void
color_region (PixelRegion   *dest,
	      const unsigned char *col)
{
  int h;
  unsigned char * s;
  void * pr;

  for (pr = pixel_regions_register (1, dest); pr != NULL; pr = pixel_regions_process (pr))
    {
      h = dest->h;
      s = dest->data;

      if (dest->w*dest->bytes == dest->rowstride)
      {
	/* do it all in one function call if we can */
	/* this hasn't been tested to see if it is a
	   signifigant speed gain yet */
	color_pixels (s, col, dest->w*h, dest->bytes);
      }
      else
      {
	while (h--)
	{
	  color_pixels (s, col, dest->w, dest->bytes);
	  s += dest->rowstride;
	}
      }
    }
}


void
blend_region (PixelRegion *src1,
	      PixelRegion *src2,
	      PixelRegion *dest,
	      int          blend)
{
  int h;
  unsigned char *s1, *s2, * d;
  void * pr;

  for (pr = pixel_regions_register (3, src1, src2, dest); pr != NULL; pr = pixel_regions_process (pr))
    {
      s1 = src1->data;
      s2 = src2->data;
      d = dest->data;
      h = src1->h;

      while (h --)
	{
          blend_pixels (s1, s2, d, blend, src1->w, src1->bytes, FALSE);
	  s1 += src1->rowstride;
	  s2 += src2->rowstride;
	  d += dest->rowstride;
	}
    }
}


void
shade_region (PixelRegion   *src,
	      PixelRegion   *dest,
	      unsigned char *col,
	      int            blend)
{
  int h;
  unsigned char * s, * d;

  s = src->data;
  d = dest->data;
  h = src->h;

  while (h --)
    {
/*      blend_pixels (s, d, col, blend, src->w, src->bytes);*/
      s += src->rowstride;
      d += dest->rowstride;
    }
}



void
copy_region (PixelRegion *src,
	     PixelRegion *dest)
{
  int h;
  int pixelwidth;
  unsigned char * s, * d;
  void * pr;

#ifdef COWSHOW
  fputc('[',stderr);
#endif
  for (pr = pixel_regions_register (2, src, dest);
       pr != NULL;
       pr = pixel_regions_process (pr))
    {
      if (src->tiles && dest->tiles &&
	  src->curtile && dest->curtile &&
	  src->offx == 0 && dest->offx == 0 &&
	  src->offy == 0 && dest->offy == 0 &&
	  src->w == tile_ewidth(src->curtile) && 
	  dest->w == tile_ewidth(dest->curtile) &&
	  src->h == tile_eheight(src->curtile) && 
	  dest->h == tile_eheight(dest->curtile)) 
	{
#ifdef COWSHOW
	  fputc('!',stderr);
#endif
	  tile_manager_map_over_tile (dest->tiles, dest->curtile, src->curtile);
	}
      else 
	{
#ifdef COWSHOW
	  fputc('.',stderr);
#endif
	  pixelwidth = src->w * src->bytes;
	  s = src->data;
	  d = dest->data;
	  h = src->h;
	  
	  while (h --)
	    {
	      memcpy (d, s, pixelwidth);
	      s += src->rowstride;
	      d += dest->rowstride;
	    }
	}
    }
#ifdef COWSHOW
  fputc(']',stderr);
  fputc('\n',stderr);
#endif
}


void
add_alpha_region (PixelRegion *src,
		  PixelRegion *dest)
{
  int h;
  unsigned char * s, * d;
  void * pr;

  for (pr = pixel_regions_register (2, src, dest); pr != NULL; pr = pixel_regions_process (pr))
    {
      s = src->data;
      d = dest->data;
      h = src->h;

      while (h --)
	{
	  add_alpha_pixels (s, d, src->w, src->bytes);
	  s += src->rowstride;
	  d += dest->rowstride;
	}
    }
}


void
flatten_region (PixelRegion   *src,
		PixelRegion   *dest,
		unsigned char *bg)
{
  int h;
  unsigned char * s, * d;

  s = src->data;
  d = dest->data;
  h = src->h;

  while (h --)
    {
      flatten_pixels (s, d, bg, src->w, src->bytes);
      s += src->rowstride;
      d += dest->rowstride;
    }
}


void
extract_alpha_region (PixelRegion *src,
		      PixelRegion *mask,
		      PixelRegion *dest)
{
  int h;
  unsigned char * s, * m, * d;
  void * pr;

  for (pr = pixel_regions_register (3, src, mask, dest); pr != NULL; pr = pixel_regions_process (pr))
    {
      s = src->data;
      d = dest->data;
      if (mask)
	m = mask->data;
      else
	m = NULL;

      h = src->h;
      while (h --)
	{
	  extract_alpha_pixels (s, m, d, src->w, src->bytes);
	  s += src->rowstride;
	  d += dest->rowstride;
	  if (mask)
	    m += mask->rowstride;
	}
    }
}


void
extract_from_region (PixelRegion   *src,
		     PixelRegion   *dest,
		     PixelRegion   *mask,
		     unsigned char *cmap,
		     unsigned char *bg,
		     int            type,
		     int            has_alpha,
		     int            cut)
{
  int h;
  unsigned char * s, * d, * m;
  void * pr;

  for (pr = pixel_regions_register (3, src, dest, mask); pr != NULL; pr = pixel_regions_process (pr))
    {
      s = src->data;
      d = dest->data;
      m = (mask) ? mask->data : NULL;
      h = src->h;

      while (h --)
	{
	  switch (type)
	    {
	    case 0:  /*  RGB      */
	    case 1:  /*  GRAY     */
	      extract_from_inten_pixels (s, d, m, bg, cut, src->w,
					 src->bytes, has_alpha);
	      break;
	    case 2:  /*  INDEXED  */
	      extract_from_indexed_pixels (s, d, m, cmap, bg, cut, src->w,
					   src->bytes, has_alpha);
	      break;
	    }

	  s += src->rowstride;
	  d += dest->rowstride;
	  if (mask)
	    m += mask->rowstride;
	}
    }
}


void
convolve_region (PixelRegion	*srcR,
		 PixelRegion	*destR,
		 int		*matrix,
		 int		 size,
		 int		 divisor,
		 ConvolutionType mode)
{
  /*  Convolve the src image using the convolution matrix, writing to dest  */
  /*  Convolve is not tile-enabled--use accordingly  */
  unsigned char *src, *s_row, * s;
  unsigned char *dest, * d;
  int * m;
  int total [4];
  int b, bytes;
  int length;
  int wraparound;
  int margin;      /*  margin imposed by size of conv. matrix  */
  int i, j;
  int x, y;
  int offset;

  /*  If the mode is NEGATIVE_CONVOL, the offset should be 128  */
  if (mode == NEGATIVE_CONVOL)
    {
      offset = 128;
      mode = NORMAL_CONVOL;
    }
  else
    offset = 0;

  /*  check for the boundary cases  */
  if (srcR->w < (size - 1) || srcR->h < (size - 1))
    return;

  /*  Initialize some values  */
  bytes = srcR->bytes;
  length = bytes * srcR->w;
  margin = size / 2;
  src = srcR->data;
  dest = destR->data;

  /*  calculate the source wraparound value  */
  wraparound = srcR->rowstride - size * bytes;

  /* copy the first (size / 2) scanlines of the src image... */
  for (i = 0; i < margin; i++)
    {
      memcpy (dest, src, length);
      src += srcR->rowstride;
      dest += destR->rowstride;
    }

  src = srcR->data;

  for (y = margin; y < srcR->h - margin; y++)
    {
      s_row = src;
      s = s_row + srcR->rowstride*margin;
      d = dest;

      /* handle the first margin pixels... */
      b = bytes * margin;
      while (b --)
	*d++ = *s++;

      /* now, handle the center pixels */
      x = srcR->w - margin*2;
      while (x--)
	{
	  s = s_row;

	  m = matrix;
	  total [0] = total [1] = total [2] = total [3] = 0;
	  i = size;
	  while (i --)
	    {
	      j = size;
	      while (j --)
		{
		  for (b = 0; b < bytes; b++)
		    total [b] += *m * *s++;
		  m ++;
		}

	      s += wraparound;
	    }

	  for (b = 0; b < bytes; b++)
	    {
	      total [b] = total [b] / divisor + offset;

	      if (total [b] < 0 && mode != NORMAL_CONVOL)
		total [b] = - total [b];

	      if (total [b] < 0)
		*d++ = 0;
	      else
		*d++ = (total [b] > 255) ? 255 : (unsigned char) total [b];
	    }

	  s_row += bytes;

	}

      /* handle the last pixel... */
      s = s_row + (srcR->rowstride + bytes) * margin;
      b = bytes * margin;
      while (b --)
	*d++ = *s++;

      /* set the memory pointers */
      src += srcR->rowstride;
      dest += destR->rowstride;
    }

  src += srcR->rowstride*margin;

  /* copy the last (margin) scanlines of the src image... */
  for (i = 0; i < margin; i++)
    {
      memcpy (dest, src, length);
      src += srcR->rowstride;
      dest += destR->rowstride;
    }
}

/* Convert from separated alpha to premultiplied alpha. Only works on
   non-tiled regions! */
void
multiply_alpha_region (PixelRegion *srcR)
{
  unsigned char *src, *s;
  int x, y;
  int width, height;
  int b, bytes;
  double alpha_val;

  width = srcR->w;
  height = srcR->h;
  bytes = srcR->bytes;

  src = srcR->data;

  for (y = 0; y < height; y++)
    {
      s = src;
      for (x = 0; x < width; x++)
	{
	  alpha_val = s[bytes - 1] * (1.0 / 255.0);
	  for (b = 0; b < bytes - 1; b++)
	    s[b] = 0.5 + s[b] * alpha_val;
	  s += bytes;
	}
      src += srcR->rowstride;
    }
}

/* Convert from premultiplied alpha to separated alpha. Only works on
   non-tiled regions! */
void
separate_alpha_region (PixelRegion *srcR)
{
  unsigned char *src, *s;
  int x, y;
  int width, height;
  int b, bytes;
  double alpha_recip;
  int new_val;

  width = srcR->w;
  height = srcR->h;
  bytes = srcR->bytes;

  src = srcR->data;

  for (y = 0; y < height; y++)
    {
      s = src;
      for (x = 0; x < width; x++)
	{
	  /* predicate is equivalent to:
	     (((s[bytes - 1] - 1) & 255) + 2) & 256
	     */
	  if (s[bytes - 1] != 0 && s[bytes - 1] != 255)
	    {
	      alpha_recip = 255.0 / s[bytes - 1];
	      for (b = 0; b < bytes - 1; b++)
		{
		  new_val = 0.5 + s[b] * alpha_recip;
		  new_val = MIN (new_val, 255);
		  s[b] = new_val;
		}
	    }
	  s += bytes;
	}
      src += srcR->rowstride;
    }
}

void
gaussian_blur_region (PixelRegion *srcR,
		      double       radius_x,
		      double       radius_y)
{
  double std_dev;
  long width, height;
  int bytes;
  unsigned char *src, *sp;
  unsigned char *dest, *dp;
  unsigned char *data;
  int *buf, *b;
  int pixels;
  int total;
  int i, row, col;
  int start, end;
  int *curve;
  int *sum;
  int val;
  int length;
  int alpha;
  int initial_p, initial_m;

  if (radius_x == 0.0 && radius_y == 0.0) return;    /* zero blur is a no-op */

  /*  allocate the result buffer  */
  length = MAXIMUM (srcR->w, srcR->h) * srcR->bytes;
  data = paint_funcs_get_buffer (length * 2);
  src = data;
  dest = data + length;

  width = srcR->w;
  height = srcR->h;
  bytes = srcR->bytes;
  alpha = bytes - 1;

  buf = g_malloc (sizeof (int) * MAXIMUM (width, height) * 2);

  if (radius_y != 0.0)
    {
      std_dev = sqrt (-(radius_y * radius_y) / (2 * log (1.0 / 255.0)));
      curve = make_curve (std_dev, &length);
      sum = g_malloc (sizeof (int) * (2 * length + 1));
      sum[0] = 0;

      for (i = 1; i <= length*2; i++)
	sum[i] = curve[i-length-1] + sum[i-1];
      sum += length;

      total = sum[length] - sum[-length];

      for (col = 0; col < width; col++)
	{
	  pixel_region_get_col (srcR, col + srcR->x, srcR->y, height, src, 1);
	  sp = src + alpha;

	  initial_p = sp[0];
	  initial_m = sp[(height-1) * bytes];

	  /*  Determine a run-length encoded version of the column  */
	  run_length_encode (sp, buf, height, bytes);

	  for (row = 0; row < height; row++)
	    {
	      start = (row < length) ? -row : -length;
	      end = (height <= (row + length)) ? (height - row - 1) : length;

	      val = 0;
	      i = start;
	      b = buf + (row + i) * 2;

	      if (start != -length)
		val += initial_p * (sum[start] - sum[-length]);

	      while (i < end)
		{
		  pixels = b[0];
		  i += pixels;
		  if (i > end)
		    i = end;
		  val += b[1] * (sum[i] - sum[start]);
		  b += (pixels * 2);
		  start = i;
		}

	      if (end != length)
		val += initial_m * (sum[length] - sum[end]);

	      sp[row * bytes] = val / total;
	    }

	  pixel_region_set_col (srcR, col + srcR->x, srcR->y, height, src);
	}

      g_free (sum - length);
      g_free (curve - length);
    }

  if (radius_x != 0.0)
    {
      std_dev = sqrt (-(radius_x * radius_x) / (2 * log (1.0 / 255.0)));
      curve = make_curve (std_dev, &length);
      sum = g_malloc (sizeof (int) * (2 * length + 1));
      sum[0] = 0;

      for (i = 1; i <= length*2; i++)
	sum[i] = curve[i-length-1] + sum[i-1];
      sum += length;

      total = sum[length] - sum[-length];

      for (row = 0; row < height; row++)
	{
	  pixel_region_get_row (srcR, srcR->x, row + srcR->y, width, src, 1);
	  sp = src + alpha;
	  dp = dest + alpha;

	  initial_p = sp[0];
	  initial_m = sp[(width-1) * bytes];

	  /*  Determine a run-length encoded version of the row  */
	  run_length_encode (sp, buf, width, bytes);

	  for (col = 0; col < width; col++)
	    {
	      start = (col < length) ? -col : -length;
	      end = (width <= (col + length)) ? (width - col - 1) : length;

	      val = 0;
	      i = start;
	      b = buf + (col + i) * 2;

	      if (start != -length)
		val += initial_p * (sum[start] - sum[-length]);

	      while (i < end)
		{
		  pixels = b[0];
		  i += pixels;
		  if (i > end)
		    i = end;
		  val += b[1] * (sum[i] - sum[start]);
		  b += (pixels * 2);
		  start = i;
		}

	      if (end != length)
		val += initial_m * (sum[length] - sum[end]);

	      val = val / total;

	      dp[col * bytes] = val;
	    }
      
	  pixel_region_set_row (srcR, srcR->x, row + srcR->y, width, dest);
	}

      g_free (sum - length);
      g_free (curve - length);
    }

  g_free (buf);
}


/* non-interpolating scale_region.  [adam]
 */
void
scale_region_no_resample (PixelRegion *srcPR,
			  PixelRegion *destPR)
{
  int * x_src_offsets;
  int * y_src_offsets;
  unsigned char * src;
  unsigned char * dest;
  int width, height, orig_width, orig_height;
  int last_src_y;
  int row_bytes;
  int x,y,b;
  char bytes;

  orig_width = srcPR->w;
  orig_height = srcPR->h;

  width = destPR->w;
  height = destPR->h;

  bytes = srcPR->bytes;

  /*  the data pointers...  */
  x_src_offsets = (int *) g_malloc (width * bytes * sizeof(int));
  y_src_offsets = (int *) g_malloc (height * sizeof(int));
  src  = (unsigned char *) g_malloc (orig_width * bytes);
  dest = (unsigned char *) g_malloc (width * bytes);
  
  /*  pre-calc the scale tables  */
  for (b = 0; b < bytes; b++)
    {
      for (x = 0; x < width; x++)
	{
	  x_src_offsets [b + x * bytes] = b + bytes * ((x * orig_width + orig_width / 2) / width);
	}
    }
  for (y = 0; y < height; y++)
    {
      y_src_offsets [y] = (y * orig_height + orig_height / 2) / height;
    }
  
  /*  do the scaling  */
  row_bytes = width * bytes;
  last_src_y = -1;
  for (y = 0; y < height; y++)
    {
      /* if the source of this line was the same as the source
       *  of the last line, there's no point in re-rescaling.
       */
      if (y_src_offsets[y] != last_src_y)
	{
	  pixel_region_get_row (srcPR, 0, y_src_offsets[y], orig_width, src, 1);
	  for (x = 0; x < row_bytes ; x++)
	    {
	      dest[x] = src[x_src_offsets[x]];
	    }
	  last_src_y = y_src_offsets[y];
	}

      pixel_region_set_row (destPR, 0, y, width, dest);
    }
  
  g_free (x_src_offsets);
  g_free (y_src_offsets);
  g_free (src);
  g_free (dest);
}


static void
get_premultiplied_double_row(PixelRegion *srcPR, int x, int y,
			     int w, double *row, guchar *tmp_src, int n)
{
  int b;
  int bytes = srcPR->bytes;
  pixel_region_get_row (srcPR, x, y, w, tmp_src, n);
  if (pixel_region_has_alpha(srcPR))
  { /* premultiply the alpha into the double array */
    double *irow = row;
    int alpha = bytes - 1;
    double mod_alpha;
    for (x = 0; x < w; x++)
    {
      mod_alpha = tmp_src[alpha] / 255.0;
      for (b = 0; b < alpha; b++)
	irow[b] = mod_alpha * tmp_src[b];
      irow[b] = tmp_src[alpha];
      irow += bytes;
      tmp_src += bytes;
    }
  }
  else /* no alpha */
  {
    for (x = 0; x <  w*bytes; x++)
      row[x] = tmp_src[x];
  }

  /* set the off edge pixels to their nearest neighbor */
  for (b = 0; b < 2 * bytes; b++)
    row[-2*bytes + b] = row[(b%bytes)];
  for (b = 0; b < bytes * 2; b++)
    row[w*bytes + b] = row[(w - 1) * bytes + (b%bytes)];
}


static
void expand_line(double *dest, double *src, int bytes,
		 int old_width, int width, InterpolationType interp)
{
  double ratio;
  int x,b;
  int src_col;
  double frac;
  double *s;
  ratio = old_width/(double)width;

/* this could be opimized much more by precalculating the coeficients for
   each x */
  switch(interp)
  {
    case CUBIC_INTERPOLATION:
      for (x = 0; x < width; x++)
      {
	src_col = ((int)((x - 0.5) * ratio + 2.0)) - 2;
	/* +2, -2 is there because (int) rounds towards 0 and we need 
	   to round down */
	frac =          ((x - 0.5) * ratio) - src_col;
	s = &src[src_col * bytes];
	for (b = 0; b < bytes; b++)
	  dest[b] = cubic (frac, s[b - bytes], s[b], s[b+bytes], s[b+bytes*2]);
	dest += bytes;
      }
      break;
    case LINEAR_INTERPOLATION:
      for (x = 0; x < width; x++)
      {
	src_col = ((int)((x - 0.5) * ratio + 2.0)) - 2;
	/* +2, -2 is there because (int) rounds towards 0 and we need 
	   to round down */
	frac =          ((x - 0.5) * ratio) - src_col;
	s = &src[src_col * bytes];
	for (b = 0; b < bytes; b++)
	  dest[b] = ((s[b + bytes] - s[b]) * frac + s[b]);
	dest += bytes;
      }
      break;
   case NEAREST_NEIGHBOR_INTERPOLATION:
     g_error("sampling_type can't be "
	     "NEAREST_NEIGHBOR_INTERPOLATION");
  }
}


static void shrink_line(double *dest, double *src, int bytes,
			int old_width, int width, InterpolationType interp)
{
  int x, b;
  double *source, *destp;
  register double accum;
  register unsigned int max;
  register double mant, tmp;
  register const double step = old_width/(double)width;
  register const double inv_step = 1.0/step;
  double position;
  for (b = 0; b < bytes; b++)
  {
    
    source = &src[b];
    destp = &dest[b];
    position = -1;

    mant = *source;

    for (x = 0; x < width; x++)
    {
      source+= bytes;
      accum = 0;
      max = ((int)(position+step)) - ((int)(position));
      max--;
      while (max)
      {
	accum += *source;
	source += bytes;
	max--;
      }
      tmp = accum + mant;
      mant = ((position+step) - (int)(position + step));
      mant *= *source;
      tmp += mant;
      tmp *= inv_step;
      mant = *source - mant;
      *(destp) = tmp;
      destp += bytes;
      position += step;
	
    }
  }
}

static void
get_scaled_row(void **src, int y, int new_width, PixelRegion *srcPR, double *row,
	       guchar *src_tmp)
{
/* get the necesary lines from the source image, scale them,
   and put them into src[] */
  rotate_pointers(src, 4);
  if (y < 0)
    y = 0;
  if (y < srcPR->h)
  {
    get_premultiplied_double_row(srcPR, 0, y, srcPR->w,
				 row, src_tmp, 1);
    if (new_width > srcPR->w)
      expand_line(src[3], row, srcPR->bytes, 
		  srcPR->w, new_width, interpolation_type);
    else if (srcPR->w > new_width)
      shrink_line(src[3], row, srcPR->bytes, 
		  srcPR->w, new_width, interpolation_type);
    else /* no scailing needed */
      memcpy(src[3], row, sizeof (double) * new_width * srcPR->bytes);
  }
  else
    memcpy(src[3], src[2], sizeof (double) * new_width * srcPR->bytes);
}
void
scale_region (PixelRegion *srcPR,
	      PixelRegion *destPR)
{
  double  *src[4];
  guchar * src_tmp;
  guchar * dest;
  double * row, *accum;
  int bytes, b;
  int width, height;
  int orig_width, orig_height;
  double x_rat, y_rat;
  int i;
  int old_y = -3, new_y;
  int x, y;

  if (interpolation_type == NEAREST_NEIGHBOR_INTERPOLATION)
  {
    scale_region_no_resample (srcPR, destPR);
    return;
   }

  orig_width = srcPR->w;
  orig_height = srcPR->h;

  width = destPR->w;
  height = destPR->h;

  /*  find the ratios of old x to new x and old y to new y  */
  x_rat = (double) orig_width / (double) width;
  y_rat = (double) orig_height / (double) height;

  /*  Some calculations...  */
  bytes = destPR->bytes;

  /*  the data pointers...  */
  for (i = 0; i < 4; i++)
    src[i]    = g_new (double, (width) * bytes);
  dest   = g_new (guchar, width * bytes);

  src_tmp = g_new (guchar, orig_width * bytes);

 /* offset the row pointer by 2*bytes so the range of the array 
    is [-2*bytes] to [(orig_width + 2)*bytes] */
  row = g_new(double, (orig_width + 2*2) * bytes);
  row += bytes*2;

  accum = g_new(double, (width) * bytes);

  /*  Scale the selected region  */
  
  for (y = 0; y < height;  y++)
  {
    if (height < orig_height)
    {
      int max;
      double frac;
      const double inv_ratio = 1.0 / y_rat;
      if (y == 0) /* load the first row if this it the first time through  */
	get_scaled_row((void **)&src[0], 0, width, srcPR, row,
		       src_tmp);
      new_y = (int)((y) * y_rat);
      frac = 1.0 - (y*y_rat - new_y);
      for (x  = 0; x < width*bytes; x++)
	accum[x] = src[3][x] * frac;

      max = ((int)((y+1) *y_rat)) - (new_y);
      max--;

      get_scaled_row((void **)&src[0], ++new_y, width, srcPR, row,
		     src_tmp);
      
      while (max > 0)
      {
	for (x  = 0; x < width*bytes; x++)
	  accum[x] += src[3][x];
	get_scaled_row((void **)&src[0], ++new_y, width, srcPR, row,
		       src_tmp);
	max--;
      }
      frac = (y + 1)*y_rat - ((int)((y + 1)*y_rat));
      for (x  = 0; x < width*bytes; x++)
      {
	accum[x] += frac * src[3][x];
	accum[x] *= inv_ratio;
      }
    }      
    else if (height > orig_height)
    {
      new_y = floor((y - 0.5) * y_rat);
    
      while (old_y <= new_y)
      { /* get the necesary lines from the source image, scale them,
	   and put them into src[] */
	get_scaled_row((void **)&src[0], old_y + 2, width, srcPR, row,
		       src_tmp);
	old_y++;
      }
      switch(interpolation_type)
      {
       case CUBIC_INTERPOLATION:
       {
	 double p0, p1, p2, p3;
	 double dy = ((y - 0.5) * y_rat) - new_y;
	 p0 = cubic(dy, 1, 0, 0, 0);
	 p1 = cubic(dy, 0, 1, 0, 0);
	 p2 = cubic(dy, 0, 0, 1, 0);
	 p3 = cubic(dy, 0, 0, 0, 1);
	 for (x = 0; x < width * bytes; x++)
	   accum[x] = p0 * src[0][x] + p1 * src[1][x] +
	     p2 * src[2][x] + p3 * src[3][x];
       } break;
       case LINEAR_INTERPOLATION:
       {
	 double idy = ((y - 0.5) * y_rat) - new_y;
	 double dy = 1.0 - idy;
	 for (x = 0; x < width * bytes; x++)
	   accum[x] = dy * src[1][x] + idy * src[2][x];
       } break;
       case NEAREST_NEIGHBOR_INTERPOLATION:
	 g_error("sampling_type can't be "
		 "NEAREST_NEIGHBOR_INTERPOLATION");

      }
    }
    else /* height == orig_height */
    {
      get_scaled_row((void **)&src[0], y, width, srcPR, row,
		     src_tmp);
      memcpy(accum, src[3], sizeof(double) * width * bytes);
    }
    if (pixel_region_has_alpha(srcPR))
    { /* unmultiply the alpha */
      double inv_alpha;
      double *p = accum;
      int alpha = bytes - 1;
      int result;
      guchar *d = dest;
      for (x = 0; x < width; x++)
      {
	inv_alpha = 255.0 / p[alpha];
	for (b = 0; b < alpha; b++)
	{
	  result = RINT(inv_alpha * p[b]);
	  if (result < 0)
	    d[b] = 0;
	  else if (result > 255)
	    d[b] = 255;
	  else
	    d[b] = result;
	}
	result = RINT(p[alpha]);
	if (result < 0)
	  d[alpha] = 0;
	else if (result > 255)
	  d[alpha] = 255;
	else
	  d[alpha] = result;
	d += bytes;
	p += bytes;
      }
    }
    else
    {
      int w = width * bytes;
      for (x = 0; x < w; x++)
      {
	if (accum[x] < 0.0)
	  dest[x] = 0;
	else if (accum[x] > 255.0)
	  dest[x] = 255;
	else
	  dest[x] = RINT(accum[x]);
      }
    }
    pixel_region_set_row (destPR, 0, y, width, dest);
  }

  /*  free up temporary arrays  */
  g_free (accum);
  for (i = 0; i < 4; i++)
    g_free (src[i]);
  g_free (src_tmp);
  g_free (dest);
  row -= 2*bytes;
  g_free (row);
}

void
subsample_region (PixelRegion *srcPR,
		  PixelRegion *destPR,
		  int          subsample)
{
  unsigned char * src, * s;
  unsigned char * dest, * d;
  double * row, * r;
  int destwidth;
  int src_row, src_col;
  int bytes, b;
  int width, height;
  int orig_width, orig_height;
  double x_rat, y_rat;
  double x_cum, y_cum;
  double x_last, y_last;
  double * x_frac, y_frac, tot_frac;
  int i, j;
  int frac;
  int advance_dest;

  orig_width = srcPR->w / subsample;
  orig_height = srcPR->h / subsample;
  width = destPR->w;
  height = destPR->h;

  /*  Some calculations...  */
  bytes = destPR->bytes;
  destwidth = destPR->rowstride;

  /*  the data pointers...  */
  src = (unsigned char *) g_malloc (orig_width * bytes);
  dest = destPR->data;

  /*  find the ratios of old x to new x and old y to new y  */
  x_rat = (double) orig_width / (double) width;
  y_rat = (double) orig_height / (double) height;

  /*  allocate an array to help with the calculations  */
  row    = (double *) g_malloc (sizeof (double) * width * bytes);
  x_frac = (double *) g_malloc (sizeof (double) * (width + orig_width));

  /*  initialize the pre-calculated pixel fraction array  */
  src_col = 0;
  x_cum = (double) src_col;
  x_last = x_cum;

  for (i = 0; i < width + orig_width; i++)
    {
      if (x_cum + x_rat <= (src_col + 1 + EPSILON))
	{
	  x_cum += x_rat;
	  x_frac[i] = x_cum - x_last;
	}
      else
	{
	  src_col ++;
	  x_frac[i] = src_col - x_last;
	}
      x_last += x_frac[i];
    }

  /*  clear the "row" array  */
  memset (row, 0, sizeof (double) * width * bytes);

  /*  counters...  */
  src_row = 0;
  y_cum = (double) src_row;
  y_last = y_cum;

  pixel_region_get_row (srcPR, 0, src_row * subsample, orig_width * subsample, src, subsample);

  /*  Scale the selected region  */
  for (i = 0; i < height; )
    {
      src_col = 0;
      x_cum = (double) src_col;

      /* determine the fraction of the src pixel we are using for y */
      if (y_cum + y_rat <= (src_row + 1 + EPSILON))
	{
	  y_cum += y_rat;
	  y_frac = y_cum - y_last;
	  advance_dest = TRUE;
	}
      else
	{
	  src_row ++;
	  y_frac = src_row - y_last;
	  advance_dest = FALSE;
	}

      y_last += y_frac;

      s = src;
      r = row;

      frac = 0;
      j = width;

      while (j)
	{
	  tot_frac = x_frac[frac++] * y_frac;

	  for (b = 0; b < bytes; b++)
	    r[b] += s[b] * tot_frac;

	  /*  increment the destination  */
	  if (x_cum + x_rat <= (src_col + 1 + EPSILON))
	    {
	      r += bytes;
	      x_cum += x_rat;
	      j--;
	    }

	  /* increment the source */
	  else
	    {
	      s += bytes;
	      src_col++;
	    }
	}

      if (advance_dest)
	{
	  tot_frac = 1.0 / (x_rat * y_rat);

	  /*  copy "row" to "dest"  */
	  d = dest;
	  r = row;

	  j = width;
	  while (j--)
	    {
	      b = bytes;
	      while (b--)
		*d++ = (unsigned char) (*r++ * tot_frac + 0.5);
	    }

	  dest += destwidth;

	  /*  clear the "row" array  */
	  memset (row, 0, sizeof (double) * destwidth);

	  i++;
	}
      else
	pixel_region_get_row (srcPR, 0, src_row * subsample, orig_width * subsample, src, subsample);
    }

  /*  free up temporary arrays  */
  g_free (row);
  g_free (x_frac);
  g_free (src);
}


float
shapeburst_region (PixelRegion *srcPR,
		   PixelRegion *distPR)
{
  Tile *tile;
  unsigned char *tile_data;
  float max_iterations;
  float *distp_cur;
  float *distp_prev;
  float *tmp;
  float min_prev;
  float float_tmp;
  int min;
  int min_left;
  int length;
  int i, j, k;
  int src;
  int fraction;
  int prev_frac;
  int x, y;
  int end;
  int boundary;
  int inc;

  src = 0;

  max_iterations = 0.0;

  length = distPR->w + 1;
  distp_prev = (float *) paint_funcs_get_buffer (sizeof (float) * length * 2);
  for (i = 0; i < length; i++)
    distp_prev[i] = 0.0;

  distp_prev += 1;
  distp_cur = distp_prev + length;

  for (i = 0; i < srcPR->h; i++)
    {
      /*  set the current dist row to 0's  */
      memset(distp_cur - 1, 0, sizeof(float) * (length - 1));

      for (j = 0; j < srcPR->w; j++)
	{
	  min_prev = MINIMUM (distp_cur[j-1], distp_prev[j]);
	  min_left = MINIMUM ((srcPR->w - j - 1), (srcPR->h - i - 1));
	  min = (int) MINIMUM (min_left, min_prev);
	  fraction = 255;

	  /*  This might need to be changed to 0 instead of k = (min) ? (min - 1) : 0  */
	  for (k = (min) ? (min - 1) : 0; k <= min; k++)
	    {
	      x = j;
	      y = i + k;
	      end = y - k;

	      while (y >= end)
		{
		  tile = tile_manager_get_tile (srcPR->tiles, x, y, TRUE, FALSE);
		  tile_data = tile_data_pointer (tile, x%TILE_WIDTH, y%TILE_HEIGHT);
		  boundary = MINIMUM ((y % TILE_HEIGHT), (tile_ewidth(tile) - (x % TILE_WIDTH) - 1));
		  boundary = MINIMUM (boundary, (y - end)) + 1;
		  inc = 1 - tile_ewidth (tile);

		  while (boundary--)
		    {
		      src = *tile_data;
		      if (src == 0)
			{
			  min = k;
			  y = -1;
			  break;
			}
		      if (src < fraction)
			fraction = src;

		      x++;
		      y--;
		      tile_data += inc;
		    }

		  tile_release (tile, FALSE);
		}
	    }

	  if (src != 0)
	    {
	      /*  If min_left != min_prev use the previous fraction
	       *   if it is less than the one found
	       */
	      if (min_left != min)
		{
		  prev_frac = (int) (255 * (min_prev - min));
		  if (prev_frac == 255)
		    prev_frac = 0;
		  fraction = MINIMUM (fraction, prev_frac);
		}
	      min++;
	    }

	  float_tmp = distp_cur[j] = min + fraction / 256.0;

	  if (float_tmp > max_iterations)
	    max_iterations = float_tmp;
	}

      /*  set the dist row  */
      pixel_region_set_row (distPR, distPR->x, distPR->y + i, distPR->w, (unsigned char *) distp_cur);

      /*  swap pointers around  */
      tmp = distp_prev;
      distp_prev = distp_cur;
      distp_cur = tmp;
    }

  return max_iterations;
}

static void
rotate_pointers(void **p, guint32 n)
{
  guint32 i;
  void *tmp;
  tmp = p[0];
  for (i = 0; i < n-1; i++)
  {
    p[i] = p[i+1];
  }
  p[i] = tmp;
}

static void
compute_border(gint16 *circ, guint16 xradius, guint16 yradius)
{
  gint32 i;
  gint32 diameter = xradius*2 +1;
  gdouble tmp;
  for (i = 0; i < diameter; i++)
  {
    if (i > xradius)
      tmp = (i - xradius) - .5;
    else if (i < xradius)
      tmp = (xradius - i) - .5;
    else
      tmp = 0.0;
    circ[i] = RINT(yradius/(double)xradius *
		   sqrt((xradius)*(xradius) - (tmp)*(tmp)));
  }
}

void
fatten_region(PixelRegion *src, gint16 xradius, gint16 yradius)
{
/*
  Any bugs in this fuction are probably also in thin_region  
  Blame all bugs in this function on jaycox@earthlink.net
*/
  register gint32 i, j, x, y;
  guchar **buf; /* caches the region's pixel data */
  guchar *out;  /* holds the new scan line we are computing */
  guchar **max; /* caches the largest values for each column */
  gint16 *circ; /* holds the y coords of the filter's mask */
  gint16 last_max, last_index;

  guchar *buffer;

  if (xradius <= 0 || yradius <= 0)
    return;

  max = (guchar **)g_malloc ((src->w + 2*xradius) * sizeof(void *));
  buf = (guchar **)g_malloc((yradius + 1) * sizeof(void *));
  for (i = 0; i < yradius+1; i++)
  {
    buf[i] = (guchar *)g_malloc(src->w * sizeof(guchar));
  }
  buffer = g_malloc((src->w + 2*xradius)*(yradius + 1) * sizeof(guchar));
  for (i = 0; i < src->w + 2*xradius; i++)
  {
    if (i < xradius)
      max[i] = buffer;
    else if (i < src->w + xradius)
      max[i] = &buffer[(yradius+1)*(i - xradius)];
    else
      max[i] = &buffer[(yradius+1)*(src->w + xradius - 1)];

    for (j = 0 ; j < xradius + 1; j++)
      max[i][j] = 0;
  }
  max += xradius;
  out =  (guchar *)g_malloc (src->w * sizeof(guchar));

  circ = (short *)g_malloc ((2*xradius + 1) * sizeof(gint16));
  compute_border (circ, xradius, yradius);
  circ += xradius;

  memset (buf[0], 0, src->w);
  for (i = 0; i < yradius && i < src->h; i++) /* load top of image */
    pixel_region_get_row (src, src->x, src->y + i, src->w, buf[i+1], 1);

  for (x = 0; x < src->w; x++) /* set up max for top of image */
  {
    max[x][0] = buf[0][x];
    for (j = 1; j < yradius+1; j++)
      if (max[x][j] < buf[j][x])
	max[x][j] = buf[j][x];
      else
	max[x][j] = max[x][j-1];
  }
  for (y = 0; y < src->h; y++)
  {
    rotate_pointers((void **)buf, yradius+1);
    if (y < src->h - (yradius))
      pixel_region_get_row (src, src->x, src->y + y + yradius, src->w,
			    buf[yradius], 1);
    else
      memset (buf[yradius], 0, src->w);
    for (x = 0 ; x < src->w; x++) /* update max array */
    {
      for (i = yradius; i > 0; i--)
      {
	max[x][i] = (MAX (MAX (max[x][i - 1], buf[i-1][x]), buf[i][x]));
      }
      max[x][0] = buf[0][x];
    }
    last_max =  max[0][circ[-1]];
    last_index = 1;
    for (x = 0 ; x < src->w; x++) /* render scan line */
    {
      last_index--;
      if (last_index >= 0)
      {
	if (last_max == 255)
	  out[x] = 255;
	else
	{
	  last_max = 0;
	  for (i = xradius; i >= 0; i--)
	    if (last_max < max[x+i][circ[i]])
	    {
	      last_max = max[x+i][circ[i]];
	      last_index = i;
	    }
	  out[x] = last_max;
	}
      }
      else
      {
	last_index = xradius;
	last_max = max[x+xradius][circ[xradius]];
	for (i = xradius-1; i >= -xradius; i--)
	  if (last_max < max[x+i][circ[i]])
	  {
	    last_max = max[x+i][circ[i]];
	    last_index = i;
	  }
	out[x] = last_max;
      }
    }
    pixel_region_set_row (src, src->x, src->y + y, src->w, out);
  }
  circ -= xradius;
  max -= xradius;
  g_free (circ);
  g_free (buffer);
  g_free (max);
  for (i = 0; i < yradius + 1; i++)
    g_free (buf[i]);
  g_free (buf);
  g_free (out);
}

void
thin_region(PixelRegion *src, gint16 xradius, gint16 yradius, int edge_lock)
{
/*
  pretty much the same as fatten_region only different
  blame all bugs in this function on jaycox@earthlink.net
*/
  /* If edge_lock is true  we assume that pixels outside the region
     we are passed are identical to the edge pixels.
     If edge_lock is false, we assume that pixels outside the region are 0
  */
  register gint32 i, j, x, y;
  guchar **buf; /* caches the the region's pixels */
  guchar *out;  /* holds the new scan line we are computing */
  guchar **max; /* caches the smallest values for each column */
  gint16 *circ; /* holds the y coords of the filter's mask */
  gint16 last_max, last_index;

  guchar *buffer;

  if (xradius <= 0 || yradius <= 0)
    return;

  max = (guchar **)g_malloc ((src->w+2*xradius) * sizeof(void *));
  buf = (guchar **)g_malloc ((yradius+1) * sizeof(void *));
  for (i = 0; i < yradius+1; i++)
  {
    buf[i] = (guchar *)g_malloc (src->w * sizeof(guchar));
  }
  buffer = g_malloc ((src->w+2*xradius + 1)*(yradius+1));
  for (i = 0; i < src->w+2*xradius; i++)
  {
    if (i < xradius)
      if (edge_lock)
	max[i] = buffer;
      else
	max[i] = &buffer[(yradius+1)*(src->w + xradius)];
    else if (i < src->w + xradius)
      max[i] = &buffer[(yradius+1)*(i - xradius)];
    else
      if (edge_lock)
	max[i] = &buffer[(yradius+1)*(src->w + xradius - 1)];
      else
	max[i] = &buffer[(yradius+1)*(src->w + xradius)];
    for (j = 0 ; j < xradius+1; j++)
      max[i][j] = 255;
  }
  if (!edge_lock)
    for (j = 0 ; j < xradius+1; j++)
      max[0][j] = 0;
  max += xradius;
  out = (guchar *)g_malloc(src->w);

  circ = (short *)g_malloc((2*xradius + 1)*sizeof(gint16));
  compute_border(circ, xradius, yradius);
  circ += xradius;

  for (i = 0; i < yradius && i < src->h; i++) /* load top of image */
    pixel_region_get_row (src, src->x, src->y + i, src->w, buf[i+1], 1);
  if (edge_lock)
    memcpy (buf[0], buf[1], src->w);
  else
    memset(buf[0], 0, src->w);
  for (x = 0; x < src->w; x++) /* set up max for top of image */
  {
    max[x][0] = buf[0][x];
    for (j = 1; j < yradius+1; j++)
      if (max[x][j] > buf[j][x])
	max[x][j] = buf[j][x];
      else
	max[x][j] = max[x][j-1];
  }
  for (y = 0; y < src->h; y++)
  {
    rotate_pointers ((void **)buf, yradius+1);
    if (y < src->h - (yradius))
      pixel_region_get_row (src, src->x, src->y + y + yradius, src->w,
			    buf[yradius], 1);
    else if (edge_lock)
      memcpy (buf[yradius], buf[yradius -1], src->w);
    else
      memset (buf[yradius], 0, src->w);
    for (x = 0 ; x < src->w; x++) /* update max array */
    {
      for (i = yradius; i > 0; i--)
      {
	max[x][i] = (MIN (MIN (max[x][i - 1], buf[i-1][x]), buf[i][x]));
      }
      max[x][0] = buf[0][x];
    }
    last_max =  max[0][circ[-1]];
    last_index = 0;
    for (x = 0 ; x < src->w; x++) /* render scan line */
    {
      last_index--;
      if (last_index >= 0)
      {
	if (last_max == 0)
	  out[x] = 0;
	else
	{
	  last_max = 255;
	  for (i = xradius; i >= 0; i--)
	    if (last_max > max[x+i][circ[i]])
	    {
	      last_max = max[x+i][circ[i]];
	      last_index = i;
	    }
	  out[x] = last_max;
	}
      }
      else
      {
	last_index = xradius;
	last_max = max[x+xradius][circ[xradius]];
	for (i = xradius-1; i >= -xradius; i--)
	  if (last_max > max[x+i][circ[i]])
	  {
	    last_max = max[x+i][circ[i]];
	    last_index = i;
	  }
	out[x] = last_max;
      }
    }
    pixel_region_set_row (src, src->x, src->y + y, src->w, out);
  }
  /* undo the offsets to the pointers so we can free the malloced memmory */
  circ -= xradius;
  max -= xradius;
  /* free the memmory */
  g_free (circ);
  g_free (buffer);
  g_free (max);
  for (i = 0; i < yradius + 1; i++)
    g_free (buf[i]);
  g_free (buf);
  g_free (out);
}

static void
compute_transition(guchar *transition, guchar **buf, gint32 width)
{
  register gint32 x = 0;
  if (width == 1)
  {
    if (buf[1][x] > 127 && (buf[0][x] < 128 || buf[2][x] < 128))
      transition[x] = 255;
    else
      transition[x] = 0;
    return;
  }
  if (buf[1][x] > 127)
  {
    if ( buf[0][x] < 128 || buf[0][x+1] < 128 ||
	                    buf[1][x+1] < 128 ||
	 buf[2][x] < 128 || buf[2][x+1] < 128 )
      transition[x] = 255;
    else
      transition[x] = 0;
  }
  else
    transition[x] = 0;
  for (x = 1; x < width - 1; x++)
  {
    if (buf[1][x] >= 128)
    {
      if (buf[0][x-1] < 128 || buf[0][x] < 128 || buf[0][x+1] < 128 ||
	  buf[1][x-1] < 128           ||          buf[1][x+1] < 128 ||
	  buf[2][x-1] < 128 || buf[2][x] < 128 || buf[2][x+1] < 128)
	transition[x] = 255;
      else
	transition[x] = 0;
    }
    else
      transition[x] = 0;
  }
  if (buf[1][x] >= 128)
  {
    if ( buf[0][x-1] < 128 || buf[0][x] < 128 ||
	 buf[1][x-1] < 128 ||
	 buf[2][x-1] < 128 || buf[2][x] < 128)
      transition[x] = 255;
    else
      transition[x] = 0;
  }
  else
    transition[x] = 0;
}

void
border_region(PixelRegion *src, gint16 xradius, gint16 yradius)
{
/*
  This function has no bugs, but if you imagine some you can
  blame them on jaycox@earthlink.net
*/
  register gint32 i, j, x, y;
  guchar **buf, *out;
  gint16 *max;
  guchar **density;
  guchar **transition;
  guchar last_max;
  gint16 last_index;

  if (xradius < 0 || yradius < 0)
  {
    g_warning ("border_region: negative radius specified.");
    return;
  }
  if (xradius == 0 || yradius == 0)
  {
    unsigned char color[] = "\0\0\0\0";
    color_region(src, color);
    return;
  }
  if (xradius == 1 && yradius == 1) /* optimize this case specifically */
  {
    guchar *transition;
    guchar *source[3];
    for (i = 0; i < 3; i++)
      source[i] = (guchar *)g_malloc ((src->w)*sizeof(guchar));

    transition = (guchar *)g_malloc ((src->w)*sizeof(guchar));

    pixel_region_get_row (src, src->x, src->y + 0, src->w, source[0], 1);
    memcpy (source[1], source[0], src->w);
    if (src->h > 1)
      pixel_region_get_row (src, src->x, src->y + 1, src->w, source[2], 1);
    else
      memcpy (source[2], source[1], src->w);

    compute_transition (transition, source, src->w);
    pixel_region_set_row (src, src->x, src->y , src->w, transition);
    
    for (y = 1; y < src->h; y++)
    {
      rotate_pointers ((void **)source, 3);
      if (y +1 < src->h)
	pixel_region_get_row (src, src->x, src->y +y +1, src->w, source[2], 1);
      else
	memcpy(source[2], source[1], src->w);
      compute_transition (transition, source, src->w);
      pixel_region_set_row (src, src->x, src->y + y, src->w, transition);
      
    }
    for (i = 0; i < 3; i++)
      g_free(source[i]);
    g_free(transition);
    return;
  } /* end of if (xradius == 1 && yradius == 1) */
  max = (gint16 *)g_malloc ((src->w+2*xradius)*sizeof(gint16 *));
  for (i = 0; i < (src->w+2*xradius); i++)
    max[i] = yradius+2;
  max += xradius;

  buf = (guchar **)g_malloc ((3)*sizeof(void *));
  for (i = 0; i < 3; i++)
  {
    buf[i] = (guchar *)g_malloc ((src->w)*sizeof(guchar));
  }
  transition = (guchar **)g_malloc ((yradius+1)*sizeof(void*));
  for (i = 0; i < yradius +1; i++)
  {
    transition[i] = (guchar *)g_malloc (src->w+2*xradius);
    memset(transition[i], 0, src->w+2*xradius);
    transition[i] += xradius;
  }
  out = (guchar *)g_malloc ((src->w)*sizeof(guchar));
  density = (guchar **)g_malloc ((2*xradius + 1)*sizeof(void *));
  density += xradius;

  for (x = 0; x < (xradius+1); x++) /* allocate density[][] */
  {
    density[ x]  = (guchar *)g_malloc (2*yradius +1);
    density[ x] += yradius;
    density[-x]  = density[x];
  }
  for (x = 0; x < (xradius+1); x++) /* compute density[][] */
  {
    register double tmpx, tmpy, dist;
    guchar a;
    if (x > 0)
      tmpx = x - 0.5;
    else if (x < 0)
      tmpx = x + 0.5;
    else
      tmpx = 0.0;
    for (y = 0; y < (yradius+1); y++)
    {
      if (y > 0)
	tmpy = y - 0.5;
      else if (y < 0)
	tmpy = y + 0.5;
      else
	tmpy = 0.0;
      dist = (tmpy*tmpy)/(yradius*yradius) + (tmpx*tmpx)/(xradius*xradius);
      if (dist < 1.0)
	a = 255*(1.0 - sqrt (dist));
      else
	a = 0;
      density[ x][ y] = a;
      density[ x][-y] = a;
      density[-x][ y] = a;
      density[-x][-y] = a;
    }
  }
  pixel_region_get_row (src, src->x, src->y + 0, src->w, buf[0], 1);
  memcpy (buf[1], buf[0], src->w);
  if (src->h > 1)
    pixel_region_get_row (src, src->x, src->y + 1, src->w, buf[2], 1);
  else
    memcpy (buf[2], buf[1], src->w);
  compute_transition (transition[1], buf, src->w);

  for (y = 1; y < yradius && y + 1< src->h; y++) /* set up top of image */
  {
    rotate_pointers ((void **)buf, 3);
    pixel_region_get_row (src, src->x, src->y + y + 1, src->w, buf[2], 1);
    compute_transition (transition[y + 1], buf, src->w);
  }
  for (x = 0; x < src->w; x++) /* set up max[] for top of image */
  {
    max[x] = -(yradius+7); 
    for (j = 1; j < yradius+1; j++)
      if (transition[j][x])
      {
	max[x] = j;
	break;
      }
  }
  for (y = 0; y < src->h; y++) /* main calculation loop */
  {
    rotate_pointers ((void **)buf, 3);
    rotate_pointers ((void **)transition, yradius + 1);
    if (y < src->h - (yradius+1))
    {
      pixel_region_get_row (src, src->x, src->y + y + yradius + 1, src->w,
			    buf[2], 1);
      compute_transition (transition[yradius], buf, src->w);
    }
    else
      memcpy (transition[yradius], transition[yradius - 1], src->w);

    for (x = 0; x < src->w; x++) /* update max array */
    {
      if (max[x] < 1)
      {
	if (max[x] <= -yradius)
	{
	  if (transition[yradius][x])
	    max[x] = yradius;
	  else
	    max[x]--;
	}
	else
	  if (transition[-max[x]][x])
	    max[x] = -max[x];
	  else if (transition[-max[x]+1][x])
	    max[x] = -max[x]+1;
	  else
	    max[x]--;
      }
      else
	max[x]--;
      if (max[x] < -yradius - 1)
	max[x] = -yradius -1;
    }
    last_max =  max[0][density[-1]];
    last_index = 1;
    for (x = 0 ; x < src->w; x++) /* render scan line */
    {
      last_index--;
      if (last_index >= 0)
      {
	last_max = 0;
	for (i = xradius; i >= 0; i--)
	  if (max[x+i] <= yradius && max[x+i] >= -yradius &&
	      density[i][max[x+i]] > last_max)
	  {
	    last_max = density[i][max[x+i]];
	    last_index = i;
	  }
	out[x] = last_max;
      }
      else
      {
	last_max = 0;
	for (i = xradius; i >= -xradius; i--)
	  if (max[x+i] <= yradius && max[x+i] >= -yradius &&
	      density[i][max[x+i]] > last_max)
	  {
	    last_max = density[i][max[x+i]];
	    last_index = i;
	  }
	out[x] = last_max;
      }
      if (last_max == 0)
      {
	for (i = x+1; i < src->w; i++)
	{
	  if (max[i] >= -yradius)
	    break;
	}
	if (i - x > xradius)
	{
	  for (; x < i - xradius; x++)
	    out[x] = 0;
	  x--;
	}
	last_index = xradius;
      }
    }
    pixel_region_set_row (src, src->x, src->y + y, src->w, out);
  }
  g_free(out);

  for (i = 0; i < 3; i++)
    g_free(buf[i]);

  g_free (buf);
  max -= xradius;
  g_free (max);

  for (i = 0; i < yradius +1; i++)
  {
    transition[i] -= xradius;
    g_free (transition[i]);
  }
  g_free (transition);

  for (i = 0; i < xradius +1 ; i++)
  {
    density[i]-= yradius;
    g_free(density[i]);
  }
  density -= xradius;
  g_free(density);
}

void
swap_region (PixelRegion *src,
	     PixelRegion *dest)
{
  int h;
  int length;
  unsigned char * s, * d;
  void * pr;

  for (pr = pixel_regions_register (2, src, dest); pr != NULL; pr = pixel_regions_process (pr))
    {
      s = src->data;
      h = src->h;
      d = dest->data;
      length = src->w * src->bytes;

      while (h --)
	{
	  swap_pixels (s, d, length);
	  s += src->rowstride;
	  d += dest->rowstride;
	}
    }
}


static void
apply_mask_to_sub_region (int *opacityp,
			  PixelRegion *src,
			  PixelRegion *mask)
{
  int h;
  unsigned char * s, * m;
  int opacity = *opacityp;
  
  s = src->data;
  m = mask->data;
  h = src->h;

  while (h --)
  {
    apply_mask_to_alpha_channel (s, m, opacity, src->w, src->bytes);
    s += src->rowstride;
    m += mask->rowstride;
  }
}

void
apply_mask_to_region (PixelRegion *src,
		      PixelRegion *mask,
		      int          opacity)
{
  pixel_regions_process_parallel ((p_func)apply_mask_to_sub_region,
				  &opacity, 2, src, mask);
}


static void
combine_mask_and_sub_region (int *opacityp,
			     PixelRegion *src,
			     PixelRegion *mask)
{
  int h;
  unsigned char * s, * m;
  int opacity = *opacityp;
  
  s = src->data;
  m = mask->data;
  h = src->h;

  while (h --)
  {
    combine_mask_and_alpha_channel (s, m, opacity, src->w, src->bytes);
    s += src->rowstride;
    m += mask->rowstride;
  }
}

void
combine_mask_and_region (PixelRegion *src,
			 PixelRegion *mask,
			 int          opacity)
{

  pixel_regions_process_parallel ((p_func)combine_mask_and_sub_region,
				  &opacity, 2, src, mask);
}


void
copy_gray_to_region (PixelRegion *src,
		     PixelRegion *dest)
{
  int h;
  unsigned char * s, * d;
  void * pr;

  for (pr = pixel_regions_register (2, src, dest); pr != NULL; pr = pixel_regions_process (pr))
    {
      s = src->data;
      d = dest->data;
      h = src->h;

      while (h --)
	{
	  copy_gray_to_inten_a_pixels (s, d, src->w, dest->bytes);
	  s += src->rowstride;
	  d += dest->rowstride;
	}
    }
}


struct initial_regions_struct
{
  int opacity;
  LayerModeEffects mode;
  int *affect;
  int type;
  unsigned char *data;
};

void
initial_sub_region(struct initial_regions_struct *st, 
		   PixelRegion   *src,
		   PixelRegion   *dest,
		   PixelRegion   *mask)
{
  int h;
  unsigned char * s, * d, * m;
  unsigned char    buf[512];
  unsigned char   *data;
  int              opacity;
  LayerModeEffects mode;
  int             *affect;
  int              type;

  data = st->data;
  opacity = st->opacity;
  mode = st->mode;
  affect = st->affect;
  type = st->type;

  if (src->w * (src->bytes + 1) > 512)
    fprintf(stderr, "initial_sub_region:: error :: src->w * (src->bytes + 1) > 512\n");

  s = src->data;
  d = dest->data;
  m = (mask) ? mask->data : NULL;

  for (h = 0; h < src->h; h++)
  {
    /*  based on the type of the initial image...  */
    switch (type)
    {
     case INITIAL_CHANNEL_MASK:
     case INITIAL_CHANNEL_SELECTION:
       initial_channel_pixels (s, d, src->w, dest->bytes);
       break;

     case INITIAL_INDEXED:
       initial_indexed_pixels (s, d, data, src->w);
       break;

     case INITIAL_INDEXED_ALPHA:
       initial_indexed_a_pixels (s, d, m, data, opacity, src->w);
       break;

     case INITIAL_INTENSITY:
       if (mode == DISSOLVE_MODE)
       {
	 dissolve_pixels (s, buf, src->x, src->y + h, opacity, src->w, src->bytes,
			  src->bytes + 1, 0);
	 initial_inten_pixels (buf, d, m, opacity, affect,
			       src->w, src->bytes);
       }
       else
	 initial_inten_pixels (s, d, m, opacity, affect, src->w, src->bytes);
       break;

     case INITIAL_INTENSITY_ALPHA:
       if (mode == DISSOLVE_MODE)
       {
	 dissolve_pixels (s, buf, src->x, src->y + h, opacity, src->w, src->bytes,
			  src->bytes, 1);
	 initial_inten_a_pixels (buf, d, m, opacity, affect,
				 src->w, src->bytes);
       }
       else
	 initial_inten_a_pixels (s, d, m, opacity, affect, src->w, src->bytes);
       break;
    }

    s += src->rowstride;
    d += dest->rowstride;
    if (mask)
      m += mask->rowstride;
  }
}

void
initial_region (PixelRegion	*src,
		PixelRegion	*dest,
		PixelRegion	*mask,
		unsigned char	*data,
		int		 opacity,
		LayerModeEffects mode,
		int		*affect,
		int		 type)
{
  struct initial_regions_struct st;

  st.opacity = opacity;
  st.mode = mode;
  st.affect = affect;
  st.type = type;
  st.data = data;
  
  pixel_regions_process_parallel ((p_func)initial_sub_region, &st, 3,
				    src, dest, mask);
}

struct combine_regions_struct
{
  int opacity;
  LayerModeEffects mode;
  int *affect;
  int type;
  unsigned char *data;
  int has_alpha1, has_alpha2;
  gboolean opacity_quickskip_possible;
  gboolean transparency_quickskip_possible;
};



void
combine_sub_region(struct combine_regions_struct *st,
		   PixelRegion *src1, PixelRegion *src2,
		   PixelRegion *dest, PixelRegion *mask)
{
  unsigned char   *data;
  int              opacity;
  LayerModeEffects mode;
  int             *affect;
  int              type;
  int h;
  int has_alpha1, has_alpha2;
  int combine = 0;
  int mode_affect;
  unsigned char * s, * s1, * s2;
  unsigned char * d, * m;
  unsigned char buf[512];
  gboolean opacity_quickskip_possible;
  gboolean transparency_quickskip_possible;
  TileRowHint hint;

  opacity = st->opacity;
  mode = st->mode;
  affect = st->affect;
  type = st->type;
  data = st->data;
  has_alpha1 = st->has_alpha1;
  has_alpha2 = st->has_alpha2;

  opacity_quickskip_possible = (st->opacity_quickskip_possible &&
				src2->tiles);
  transparency_quickskip_possible = (st->transparency_quickskip_possible &&
				     src2->tiles);

  s1 = src1->data;
  s2 = src2->data;
  d = dest->data;
  m = (mask) ? mask->data : NULL;

  if (src1->w > 128)
    g_error("combine_sub_region::src1->w = %d\n", src1->w);

  if (transparency_quickskip_possible || opacity_quickskip_possible)
    {
#ifdef HINTS_SANITY
      if (src1->h != src2->h)
	g_error("HEIGHTS SUCK!!");
      if (src1->offy != dest->offy)
	g_error("SRC1 OFFSET != DEST OFFSET");
#endif
      update_tile_rowhints (src2->curtile,
			    src2->offy, src2->offy + (src1->h - 1));
    }
  /* else it's probably a brush-composite */

  for (h = 0; h < src1->h; h++)
    {
      hint = TILEROWHINT_UNDEFINED;

      if (transparency_quickskip_possible)
	{
	  hint = tile_get_rowhint (src2->curtile, (src2->offy + h));

	  if (hint == TILEROWHINT_TRANSPARENT)
	    {
	      goto next_row;
	    }
	}
      else
	{
	  if (opacity_quickskip_possible)
	    {
	      hint = tile_get_rowhint (src2->curtile, (src2->offy + h));
	    }
	}
      
      s = buf;

      /*  apply the paint mode based on the combination type & mode  */
      switch (type)
	{
	case COMBINE_INTEN_A_INDEXED_A:
	case COMBINE_INTEN_A_CHANNEL_MASK:
	case COMBINE_INTEN_A_CHANNEL_SELECTION:
	  combine = type;
	  break;

	case COMBINE_INDEXED_INDEXED:
	case COMBINE_INDEXED_INDEXED_A:
	case COMBINE_INDEXED_A_INDEXED_A:
	  /*  Now, apply the paint mode--for indexed images  */
	  combine = apply_indexed_layer_mode (s1, s2, &s, mode,
					      has_alpha1, has_alpha2);
	  break;

	case COMBINE_INTEN_INTEN_A:
	case COMBINE_INTEN_A_INTEN:
	case COMBINE_INTEN_INTEN:
	case COMBINE_INTEN_A_INTEN_A:
	  /*  Now, apply the paint mode  */
	  combine = apply_layer_mode (s1, s2, &s, src1->x, src1->y + h,
				      opacity, src1->w, mode,
				      src1->bytes, src2->bytes,
				      has_alpha1, has_alpha2, &mode_affect);
	  break;

	default:
	  g_warning ("combine_sub_region: unhandled combine-type.");
	  break;
	}

      /*  based on the type of the initial image...  */
      switch (combine)
	{
	case COMBINE_INDEXED_INDEXED:
	  combine_indexed_and_indexed_pixels (s1, s2, d, m, opacity,
					      affect, src1->w,
					      src1->bytes);
	  break;
	    
	case COMBINE_INDEXED_INDEXED_A:
	  combine_indexed_and_indexed_a_pixels (s1, s2, d, m, opacity,
						affect, src1->w,
						src1->bytes);
	  break;
	    
	case COMBINE_INDEXED_A_INDEXED_A:
	  combine_indexed_a_and_indexed_a_pixels (s1, s2, d, m, opacity,
						  affect, src1->w,
						  src1->bytes);
	  break;
	    
	case COMBINE_INTEN_A_INDEXED_A:
	  /*  assume the data passed to this procedure is the
	   *  indexed layer's colormap
	   */
	  combine_inten_a_and_indexed_a_pixels (s1, s2, d, m, data, opacity,
						src1->w, dest->bytes);
	  break;
	    
	case COMBINE_INTEN_A_CHANNEL_MASK:
	  /*  assume the data passed to this procedure is the
	   *  indexed layer's colormap
	   */
	  combine_inten_a_and_channel_mask_pixels (s1, s2, d, data, opacity,
						   src1->w, dest->bytes);
	  break;
	    
	case COMBINE_INTEN_A_CHANNEL_SELECTION:
	  combine_inten_a_and_channel_selection_pixels (s1, s2, d, data,
							opacity,
							src1->w,
							src1->bytes);
	  break;
	    
	case COMBINE_INTEN_INTEN:
	  if ((hint == TILEROWHINT_OPAQUE) &&
	      opacity_quickskip_possible)
	    {
	      memcpy (d, s, dest->w * dest->bytes);
	    }
	  else
	    combine_inten_and_inten_pixels (s1, s, d, m, opacity,
					    affect, src1->w, src1->bytes);
	  break;
	    
	case COMBINE_INTEN_INTEN_A:
	  combine_inten_and_inten_a_pixels (s1, s, d, m, opacity,
					    affect, src1->w, src1->bytes);
	  break;
	    
	case COMBINE_INTEN_A_INTEN:
	  combine_inten_a_and_inten_pixels (s1, s, d, m, opacity,
					    affect, mode_affect, src1->w,
					    src1->bytes);
	  break;
	    
	case COMBINE_INTEN_A_INTEN_A:
	  if ((hint == TILEROWHINT_OPAQUE) &&
	      opacity_quickskip_possible)
	    {
	      memcpy (d, s, dest->w * dest->bytes);
	    }
	  else
	    combine_inten_a_and_inten_a_pixels (s1, s, d, m, opacity,
						affect, mode_affect,
						src1->w, src1->bytes);
	  break;
	    
	case BEHIND_INTEN:
	  behind_inten_pixels (s1, s, d, m, opacity,
			       affect, src1->w, src1->bytes,
			       src2->bytes, has_alpha1, has_alpha2);
	  break;
	    
	case BEHIND_INDEXED:
	  behind_indexed_pixels (s1, s, d, m, opacity,
				 affect, src1->w, src1->bytes,
				 src2->bytes, has_alpha1, has_alpha2);
	  break;
	    
	case REPLACE_INTEN:
	  replace_inten_pixels (s1, s, d, m, opacity,
				affect, src1->w, src1->bytes,
				src2->bytes, has_alpha1, has_alpha2);
	  break;
	    
	case REPLACE_INDEXED:
	  replace_indexed_pixels (s1, s, d, m, opacity,
				  affect, src1->w, src1->bytes,
				  src2->bytes, has_alpha1, has_alpha2);
	  break;
	    
	case ERASE_INTEN:
	  erase_inten_pixels (s1, s, d, m, opacity,
			      affect, src1->w, src1->bytes);
	  break;
	    
	case ERASE_INDEXED:
	  erase_indexed_pixels (s1, s, d, m, opacity,
				affect, src1->w, src1->bytes);
	  break;
	    
	case ANTI_ERASE_INTEN:
	  anti_erase_inten_pixels (s1, s, d, m, opacity,
				   affect, src1->w, src1->bytes);
	  break;
	    
	case ANTI_ERASE_INDEXED:
	  anti_erase_indexed_pixels (s1, s, d, m, opacity,
				     affect, src1->w, src1->bytes);
	  break;
	    
	case NO_COMBINATION:
	  g_warning("NO_COMBINATION");
	  break;
	    
	default:
	  g_warning("UNKNOWN COMBINATION");
	  break;
	}
    
    next_row:
      s1 += src1->rowstride;
      s2 += src2->rowstride;
      d += dest->rowstride;
      if (mask)
	m += mask->rowstride;
    }
}


void
combine_regions (PixelRegion	 *src1,
		 PixelRegion	 *src2,
		 PixelRegion   	 *dest,
		 PixelRegion  	 *mask,
		 unsigned char	 *data,
		 int		  opacity,
		 LayerModeEffects mode,
		 int		 *affect,
		 int		  type)
{
  int has_alpha1, has_alpha2;
  int i;
  struct combine_regions_struct st;

  /*  Determine which sources have alpha channels  */
  switch (type)
    {
    case COMBINE_INTEN_INTEN:
    case COMBINE_INDEXED_INDEXED:
      has_alpha1 = has_alpha2 = 0;
      break;
    case COMBINE_INTEN_A_INTEN:
      has_alpha1 = 1;
      has_alpha2 = 0;
      break;
    case COMBINE_INTEN_INTEN_A:
    case COMBINE_INDEXED_INDEXED_A:
      has_alpha1 = 0;
      has_alpha2 = 1;
      break;
    case COMBINE_INTEN_A_INTEN_A:
    case COMBINE_INDEXED_A_INDEXED_A:
      has_alpha1 = has_alpha2 = 1;
      break;
    default:
      has_alpha1 = has_alpha2 = 0;
    }

  st.opacity = opacity;
  st.mode = mode;
  st.affect = affect;
  st.type = type;
  st.data = data;
  st.has_alpha1 = has_alpha1;
  st.has_alpha2 = has_alpha2;

  /* cheap and easy when the row of src2 is completely opaque/transparent
     and the wind is otherwise blowing in the right direction.
  */

  /* First check - we can't do an opacity quickskip if the drawable
     has a mask, or non-full opacity, or the layer mode dictates
     that we might gain transparency.
  */
  st.opacity_quickskip_possible = ((!mask) && (opacity==255) &&
				   (!layer_modes[mode].decrease_opacity) &&
				   (layer_modes[mode].affect_alpha &&
				    has_alpha1 &&
				    affect[src1->bytes-1]) );

  /* Second check - if any single colour channel can't be affected,
     we can't use the opacity quickskip.
  */
  if (st.opacity_quickskip_possible)
    {
      for (i=0; i<src1->bytes-1; i++)
	{
	  if (!affect[i])
	    {
	      st.opacity_quickskip_possible = FALSE;
	      break;
	    }
	}
    }

  /* transparency quickskip is only possible if the layer mode
     dictates that we cannot possibly gain opacity, or the 'overall'
     opacity of the layer is set to zero anyway.
   */
  st.transparency_quickskip_possible = ((!layer_modes[mode].increase_opacity)
					|| (opacity==0));

  /* Start the actual processing.
   */
  pixel_regions_process_parallel ((p_func)combine_sub_region, &st, 4,
				    src1, src2, dest, mask);
}

void
combine_regions_replace (PixelRegion   *src1,
			 PixelRegion   *src2,
			 PixelRegion   *dest,
			 PixelRegion   *mask,
			 unsigned char *data,
			 int            opacity,
			 int           *affect,
			 int            type)
{
  int h;
  unsigned char * s1, * s2;
  unsigned char * d, * m;
  void * pr;

  for (pr = pixel_regions_register (4, src1, src2, dest, mask); pr != NULL; pr = pixel_regions_process (pr))
    {
      s1 = src1->data;
      s2 = src2->data;
      d = dest->data;
      m = mask->data;

      for (h = 0; h < src1->h; h++)
	{

	  /*  Now, apply the paint mode  */
	  apply_layer_mode_replace (s1, s2, d, m, src1->x, src1->y + h, opacity, src1->w,
					      src1->bytes, src2->bytes, affect);

	  s1 += src1->rowstride;
	  s2 += src2->rowstride;
	  d += dest->rowstride;
	  m += mask->rowstride;
	}
    }
}


/*********************************
 *   color conversion routines   *
 *********************************/

void
rgb_to_hsv (int *r,
	    int *g,
	    int *b)
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
hsv_to_rgb (int *h,
	    int *s,
	    int *v)
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
rgb_to_hls (int *r,
	    int *g,
	    int *b)
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
      else if (h > 255)
	h -= 255;
    }

  *r = h;
  *g = l;
  *b = s;
}


/* Just compute the luminosity component. */
int
rgb_to_l (int red,
	  int green,
	  int blue)
{
  int min, max;

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

  return (max + min) / 2.0;
}


static int
hls_value (float n1,
	   float n2,
	   float hue)
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
hls_to_rgb (int *h,
	    int *l,
	    int *s)
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


/************************************/
/*       apply layer modes          */
/************************************/

int
apply_layer_mode (unsigned char   *src1,
		  unsigned char   *src2,
		  unsigned char  **dest,
		  int              x,
		  int              y,
		  int              opacity,
		  int              length,
		  LayerModeEffects mode,
		  int              bytes1,   /* bytes */
		  int              bytes2,   /* bytes */
		  int              has_alpha1,  /* has alpha */
		  int              has_alpha2,  /* has alpha */
		  int             *mode_affect)
{
  int combine;

  if (!has_alpha1 && !has_alpha2)
    combine = COMBINE_INTEN_INTEN;
  else if (!has_alpha1 && has_alpha2)
    combine = COMBINE_INTEN_INTEN_A;
  else if (has_alpha1 && !has_alpha2)
    combine = COMBINE_INTEN_A_INTEN;
  else
    combine = COMBINE_INTEN_A_INTEN_A;

  /*  assumes we're applying src2 TO src1  */
  switch (mode)
    {
    case NORMAL_MODE:
      *dest = src2;
      break;

    case DISSOLVE_MODE:
      /*  Since dissolve requires an alpha channels...  */
      if (! has_alpha2)
	add_alpha_pixels (src2, *dest, length, bytes2);

      dissolve_pixels (src2, *dest, x, y, opacity, length, bytes2,
		       ((has_alpha2) ? bytes2 : bytes2 + 1), has_alpha2);
      combine = (has_alpha1) ? COMBINE_INTEN_A_INTEN_A : COMBINE_INTEN_INTEN_A;
      break;

    case MULTIPLY_MODE:
      multiply_pixels (src1, src2, *dest, length, bytes1, bytes2, has_alpha1, has_alpha2);
      break;

    case DIVIDE_MODE:
      divide_pixels (src1, src2, *dest, length, bytes1, bytes2, has_alpha1, has_alpha2);
      break;

    case SCREEN_MODE:
      screen_pixels (src1, src2, *dest, length, bytes1, bytes2, has_alpha1, has_alpha2);
      break;

    case OVERLAY_MODE:
      overlay_pixels (src1, src2, *dest, length, bytes1, bytes2, has_alpha1, has_alpha2);
      break;

    case DIFFERENCE_MODE:
      difference_pixels (src1, src2, *dest, length, bytes1, bytes2, has_alpha1, has_alpha2);
      break;

    case ADDITION_MODE:
      add_pixels (src1, src2, *dest, length, bytes1, bytes2, has_alpha1, has_alpha2);
      break;

    case SUBTRACT_MODE:
      subtract_pixels (src1, src2, *dest, length, bytes1, bytes2, has_alpha1, has_alpha2);
      break;

    case DARKEN_ONLY_MODE:
      darken_pixels (src1, src2, *dest, length, bytes1, bytes2, has_alpha1, has_alpha2);
      break;

    case LIGHTEN_ONLY_MODE:
      lighten_pixels (src1, src2, *dest, length, bytes1, bytes2, has_alpha1, has_alpha2);
      break;

    case HUE_MODE: case SATURATION_MODE: case VALUE_MODE:
      /*  only works on RGB color images  */
      if (bytes1 > 2)
	hsv_only_pixels (src1, src2, *dest, mode, length, bytes1, bytes2, has_alpha1, has_alpha2);
      else
	*dest = src2;
      break;

    case COLOR_MODE:
      /*  only works on RGB color images  */
      if (bytes1 > 2)
	color_only_pixels (src1, src2, *dest, mode, length, bytes1, bytes2, has_alpha1, has_alpha2);
      else
	*dest = src2;
      break;

    case BEHIND_MODE:
      *dest = src2;
      if (has_alpha1)
	combine = BEHIND_INTEN;
      else
	combine = NO_COMBINATION;
      break;

    case REPLACE_MODE:
      *dest = src2;
      combine = REPLACE_INTEN;
      break;

    case ERASE_MODE:
      *dest = src2;
      /*  If both sources have alpha channels, call erase function.
       *  Otherwise, just combine in the normal manner
       */
      combine = (has_alpha1 && has_alpha2) ? ERASE_INTEN : combine;
      break;

    case ANTI_ERASE_MODE:
      *dest = src2;
      combine = (has_alpha1 && has_alpha2) ? ANTI_ERASE_INTEN : combine;
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


int
apply_indexed_layer_mode (unsigned char   *src1,
			  unsigned char   *src2,
			  unsigned char  **dest,
			  LayerModeEffects mode,
			  int              has_alpha1, /* has alpha */
			  int              has_alpha2) /* has alpha */
{
  int combine;

  if (!has_alpha1 && !has_alpha2)
    combine = COMBINE_INDEXED_INDEXED;
  else if (!has_alpha1 && has_alpha2)
    combine = COMBINE_INDEXED_INDEXED_A;
  else if (has_alpha1 && has_alpha2)
    combine = COMBINE_INDEXED_A_INDEXED_A;
  else
    combine = NO_COMBINATION;

  /*  assumes we're applying src2 TO src1  */
  switch (mode)
    {
    case REPLACE_MODE:
      *dest = src2;
      combine = REPLACE_INDEXED;
      break;

    case BEHIND_MODE:
      *dest = src2;
      if (has_alpha1)
	combine = BEHIND_INDEXED;
      else
	combine = NO_COMBINATION;
      break;

    case ERASE_MODE:
      *dest = src2;
      /*  If both sources have alpha channels, call erase function.
       *  Otherwise, just combine in the normal manner
       */
      combine = (has_alpha1 && has_alpha2) ? ERASE_INDEXED : combine;
      break;

    default:
      break;
    }

  return combine;
}

static void
apply_layer_mode_replace (unsigned char  *src1,
			  unsigned char  *src2,
			  unsigned char  *dest,
			  unsigned char  *mask,
			  int             x,
			  int             y,
			  int             opacity,
			  int             length,
			  int             bytes1,   /* bytes */
			  int             bytes2,   /* bytes */
			  int            *affect)
{
  replace_pixels (src1, src2, dest, mask, length, opacity, affect, bytes1, bytes2);
}
