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
#include "paint_funcs_indexeda.h"
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
static gint       color_hash_misses;
static gint       color_hash_hits;
static guchar    *tmp_buffer;  /* temporary buffer available upon request */
static gint       tmp_buffer_size;
static guchar     no_mask = OPAQUE_OPACITY;
static gint       add_lut[256][256];


/*  Local function prototypes  */

static gint *   make_curve               (gdouble  sigma,
					  gint    *length);
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
					  guint32   n);

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
  if (use_mmx && has_alpha1 && has_alpha2) \
    { \
      if (bytes1==2 && bytes2==2) \
	return op##_pixels_1a_1a(src1, src2, length, dest); \
      if (bytes1==4 && bytes2==4) \
	return op##_pixels_3a_3a(src1, src2, length, dest); \
    } \
  /*fprintf(stderr, "non-MMX: %s(%d, %d, %d, %d)\n", #op, \
    bytes1, bytes2, has_alpha1, has_alpha2);*/
#else

#define MMX_PIXEL_OP_3A_1A(op)
#define USE_MMX_PIXEL_OP_3A_1A(op)
#endif


static guchar *
paint_funcs_get_buffer (gint size)
{
  if (size > tmp_buffer_size)
    {
      tmp_buffer_size = size;
      tmp_buffer = (guchar *) g_realloc (tmp_buffer, size);
    }

  return tmp_buffer;
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
run_length_encode (guchar *src,
		   gint   *dest,
		   gint    w,
		   gint    bytes)
{
  gint   start;
  gint   i;
  gint   j;
  guchar last;

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
extract_alpha_pixels_indexeda (const guchar *src,
			       const guchar *mask,
			       guchar       *dest,
			       guint         w)
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
      m = &no_mask;
      while (w --)
        { 
          *dest++ = INT_MULT(src[1], *m, tmp);
          src += 2;
        }
    }
}

void
initial_pixels_indexeda (const guchar *src,
			 guchar       *dest,
			 const guchar *mask,
			 const guchar *cmap,
			 guint         opacity,
			 guint         length)
{
  gint          col_index;
  guchar        new_alpha;
  const guchar *m;
  glong         tmp;

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
combine_indexeda_and_indexeda_pixels (const guchar   *src1,
				      const guchar   *src2,
				      guchar         *dest,
				      const guchar   *mask,
				      guint           opacity,
				      const gboolean *affect,
				      guint           length)
{
  const guchar * m;
  gint   b;
  guchar new_alpha;
  glong  tmp;

  if (mask)
    {
      m = mask;
   
      while (length --)
	{
	  new_alpha = INT_MULT3(src2[1], *m, opacity, tmp);

	  dest[0] = (affect[0] && new_alpha > 127) ? src2[0] : src1[0];

	  dest[0] = (affect[0] && new_alpha > 127) ? 
	    OPAQUE_OPACITY : src1[1];

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

	  dest[0] = (affect[0] && new_alpha > 127) ? src2[0] : src1[0];
	  dest[1] = (affect[1] && new_alpha > 127) ? 
	    OPAQUE_OPACITY : src1[1];

	  src1 += 2;
	  src2 += 2;
	  dest += 2;
	}
    }
}



void
behind_indexeda_pixels (const guchar   *src1,
		        const guchar   *src2,
		        guchar         *dest,
		        const guchar   *mask,
		        guint           opacity,
		        const gboolean *affect,
		        guint           length,
		        guint           bytes2,
		        guint           has_alpha2)
{
  gint          b;
  guchar        src1_alpha;
  guchar        src2_alpha;
  guchar        new_alpha;
  const guchar *m;
  glong         tmp;

  if (mask)
    m = mask;
  else
    m = &no_mask;

  while (length --)
    {
      src1_alpha = src1[1];
      src2_alpha = INT_MULT3(src2[1], *m, opacity, tmp);
      new_alpha = (src2_alpha > 127) ? OPAQUE_OPACITY : TRANSPARENT_OPACITY;

      for (b = 0; b < 2; b++)
	dest[b] = (affect[b] && new_alpha == OPAQUE_OPACITY && (src1_alpha > 127)) ?
	  src2[b] : src1[b];

      if (mask)
	m++;

      src1 += 2;
      src2 += bytes2;
      dest += 2;
    }
}


void
replace_indexeda_pixels (const guchar   *src1,
			 const guchar   *src2,
			 guchar         *dest,
			 const guchar   *mask,
			 guint           opacity,
			 const gboolean *affect,
			 guint           length,
			 guint           bytes2,
			 guint           has_alpha2)
{
  guint         b;
  guchar        mask_alpha;
  const guchar *m;
  gint          tmp;

  if (mask)
    m = mask;
  else
    m = &no_mask;

  while (length --)
    {
      mask_alpha = INT_MULT(*m, opacity, tmp);

      for (b = 0; b < bytes2; b++)
	dest[b] = (affect[b] && mask_alpha) ? src2[b] : src1[b];

      if (!has_alpha2)
	dest[b] = src1[b];

      if (mask)
	m++;

      src1 += 2;
      src2 += bytes2;
      dest += 2;
    }
}



void
erase_indexeda_pixels (const guchar   *src1,
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

      src2_alpha = INT_MULT3(src2[1], *m, opacity, tmp);
      dest[1] = (src2_alpha > 127) ? TRANSPARENT_OPACITY : src1[1];

      if (mask)
	m++;

      src1 += 2;
      src2 += 2;
      dest += 2;
    }
}


void
anti_erase_indexed_apixels (const guchar   *src1,
			    const guchar   *src2,
			    guchar         *dest,
			    const guchar   *mask,
			    guint           opacity,
			    const gboolean *affect,
			    guint           length)
{
  gint          b;
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

      src2_alpha = INT_MULT3(src2[1], *m, opacity, tmp);
      dest[1] = (src2_alpha > 127) ? OPAQUE_OPACITY : src1[1];

      if (mask)
	m++;

      src1 += 2;
      src2 += 2;
      dest += 2;
    }
}


void
extract_from_indexeda_pixels (guchar       *src,
			      guchar       *dest,
			      const guchar *mask,
			      const guchar *cmap,
			      const guchar *bg,
			      gboolean      cut,
			      guint         length)
{
  gint          b;
  gint          index;
  const guchar *m;
  gint          t;

  if (mask)
    m = mask;
  else
    m = &no_mask;

  while (length --)
    {
      index = src[0] * 3;
      for (b = 0; b < 3; b++)
	dest[b] = cmap[index + b];

      dest[3] = INT_MULT (*m, src[1], t);
      if (cut)
	src[1] = INT_MULT ((255 - *m), src[1], t);

      if (mask)
	m++;

      src += 2;
      dest += 4;
    }
}



static void
expand_line (gdouble           *dest,
	     gdouble           *src,
	     gint               bytes,
	     gint               old_width,
	     gint               width,
	     InterpolationType  interp)
{
  gdouble  ratio;
  gint     x,b;
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
	s = &src[src_col * bytes];
	for (b = 0; b < bytes; b++)
	  dest[b] = cubic (frac, s[b - bytes], s[b], s[b+bytes], s[b+bytes*2]);
	dest += bytes;
      }
      break;
    case LINEAR_INTERPOLATION:
      for (x = 0; x < width; x++)
      {
	src_col = ((int)((x) * ratio + 2.0 - 0.5)) - 2;
	/* +2, -2 is there because (int) rounds towards 0 and we need 
	   to round down */
	frac =          ((x) * ratio - 0.5) - src_col;
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


static void
shrink_line (gdouble           *dest,
	     gdouble           *src,
	     gint               bytes,
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

  g_printerr ("shrink_line bytes=%d old_width=%d width=%d interp=%d "
              "step=%f inv_step=%f\n",
              bytes, old_width, width, interp, step, inv_step);

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
compute_transition (guchar  *transition,
		    guchar **buf,
		    gint32   width)
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


#endif
