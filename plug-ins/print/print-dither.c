/*
 * "$Id$"
 *
 *   Print plug-in driver utility functions for the GIMP.
 *
 *   Copyright 1997-2000 Michael Sweet (mike@easysw.com) and
 *	Robert Krawitz (rlk@alum.mit.edu)
 *
 *   This program is free software; you can redistribute it and/or modify it
 *   under the terms of the GNU General Public License as published by the Free
 *   Software Foundation; either version 2 of the License, or (at your option)
 *   any later version.
 *
 *   This program is distributed in the hope that it will be useful, but
 *   WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 *   or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 *   for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 * Revision History:
 *
 *   See ChangeLog
 */

/*
 * This file must include only standard C header files.  The core code must
 * compile on generic platforms that don't support glib, gimp, gtk, etc.
 */


/* #define PRINT_DEBUG */


#include "print.h"
#include <limits.h>
#include <math.h>

/*
 * The GNU C library provides a good rand() function, but most others
 * use a separate function called random() to provide the same quality
 * (and size) numbers...
 */

#ifdef HAVE_RANDOM
#  define rand random
#endif /* HAVE_RANDOM */

#if !defined(__GNUC__) && !(defined(_sgi) && _COMPILER_VERSION >= 210)
#  define inline
#endif /* not GNU C or SGI C */

/* If you don't want detailed performance numbers in this file, 
 * uncomment this:
 */
/*#define QUANT(x) */

#define D_FLOYD_HYBRID 0
#define D_FLOYD 1
#define D_ADAPTIVE_BASE 4
#define D_ADAPTIVE_HYBRID (D_ADAPTIVE_BASE | D_FLOYD_HYBRID)
#define D_ADAPTIVE_RANDOM (D_ADAPTIVE_BASE | D_FLOYD)
#define D_ORDERED_BASE 8
#define D_ORDERED (D_ORDERED_BASE)
#define D_FAST_BASE 16
#define D_FAST (D_FAST_BASE)
#define D_VERY_FAST (D_FAST_BASE + 1)

#define DITHER_FAST_STEPS (6)
#define DITHER_FAST_MASK ((1 << DITHER_FAST_STEPS) - 1)

char *dither_algo_names[] =
{
  "Adaptive Hybrid",
  "Ordered",
  "Fast",
  "Very Fast",
  "Adaptive Random",
  "Hybrid Floyd-Steinberg",
  "Random Floyd-Steinberg",
};

int num_dither_algos = sizeof(dither_algo_names) / sizeof(char *);

#define ERROR_ROWS 2
#define NCOLORS (4)

#define ECOLOR_C 0
#define ECOLOR_M 1
#define ECOLOR_Y 2
#define ECOLOR_K 3

#define MAX_SPREAD 32

/*
 * A segment of the entire 0-65536 intensity range.
 */
typedef struct dither_segment
{
  unsigned range_l;		/* Bottom of range */
  unsigned range_h;		/* Top of range */
  unsigned value_l;		/* Value of lighter ink */
  unsigned value_h;		/* Value of upper ink */
  unsigned bits_l;		/* Bit pattern of lower */
  unsigned bits_h;		/* Bit pattern of upper */
  unsigned range_span;		/* Span (to avoid calculation on the fly) */
  unsigned value_span;		/* Span of values */
  unsigned dot_size_l;		/* Size of lower dot */
  unsigned dot_size_h;		/* Size of upper dot */
  char isdark_l;		/* Is lower value dark ink? */
  char isdark_h;		/* Is upper value dark ink? */
} dither_segment_t;

typedef struct dither_color
{
  int nlevels;
  unsigned bit_max;
  unsigned signif_bits;
  dither_segment_t *ranges;
} dither_color_t;

typedef struct dither_matrix
{
  int base;
  int exp;
  int x_size;
  int y_size;
  int total_size;
  int last_x;
  int last_x_mod;
  int last_y;
  int last_y_mod;
  int index;
  int i_own;
  int x_offset;
  int y_offset;
  unsigned *matrix;
} dither_matrix_t;

typedef struct dither
{
  int src_width;		/* Input width */
  int dst_width;		/* Output width */

  int density;			/* Desired density, 0-1.0 (scaled 0-65536) */
  int k_lower;			/* Transition range (lower/upper) for CMY */
  int k_upper;			/* vs. K */
  int density2;			/* Density * 2 */
  unsigned dlb_range;
  unsigned bound_range;

  int spread;			/* With Floyd-Steinberg, how widely the */
				/* error is distributed.  This should be */
				/* between 12 (very broad distribution) and */
				/* 19 (very narrow) */

  unsigned c_randomizer;	/* With Floyd-Steinberg dithering, control */
  unsigned m_randomizer;	/* how much randomness is applied to the */
  unsigned y_randomizer;	/* threshold values (0-65536).  With ordered */
  unsigned k_randomizer;	/* dithering, how much randomness is added */
				/* to the matrix value. */

  int k_clevel;			/* Amount of each ink (in 64ths) required */
  int k_mlevel;			/* to create equivalent black */
  int k_ylevel;

  int c_darkness;		/* Perceived "darkness" of each ink, */
  int m_darkness;		/* in 64ths, to calculate CMY-K transitions */
  int y_darkness;

  int dither_type;

  int d_cutoff;			/* When ordered dither is used, threshold */
				/* above which no randomness is used. */
  int adaptive_divisor;
  int adaptive_limit;
  int adaptive_lower_limit;

  int x_aspect;			/* Aspect ratio numerator */
  int y_aspect;			/* Aspect ratio denominator */

  dither_color_t c_dither;
  dither_color_t m_dither;
  dither_color_t y_dither;
  dither_color_t k_dither;

  int *errs[ERROR_ROWS][NCOLORS];
  unsigned short *vals[NCOLORS];
  int *offset0_table;
  int *offset1_table;

  int ink_limit;		/* Maximum amount of ink that may be */
				/* deposited */
  int oversampling;
  int last_line_was_empty;

  /* Hardwiring these matrices in here is an abomination.  This */
  /* eventually needs to be cleaned up. */
  dither_matrix_t mat6;
  dither_matrix_t mat7;

  dither_matrix_t c_pick;
  dither_matrix_t c_dithermat;
  dither_matrix_t m_pick;
  dither_matrix_t m_dithermat;
  dither_matrix_t y_pick;
  dither_matrix_t y_dithermat;
  dither_matrix_t k_pick;
  dither_matrix_t k_dithermat;
} dither_t;

/*
 * Bayer's dither matrix using Judice, Jarvis, and Ninke recurrence relation
 * http://www.cs.rit.edu/~sxc7922/Project/CRT.htm
 */

static unsigned sq2[] =
{
  0, 2,
  3, 1
};

#if 0
static unsigned sq3[] =
{
  3, 2, 7,
  8, 4, 0,
  1, 6, 5
};

/*
 * This magic square taken from
 * http://www.pse.che.tohoku.ac.jp/~msuzuki/MagicSquare.5x5.selfsim.html
 *
 * It is magic in the following ways:
 * Rows and columns
 * Major and minor diagonals
 * Self-complementary
 * Four neighbors at distance of 1 or 2 (diagonal or lateral)
 */

static unsigned msq0[] =
{
  00, 14, 21, 17,  8,
  22, 18,  5,  4, 11,
  9,   1, 12, 23, 15,
  13, 20, 19,  6,  2,
  16,  7,  3, 10, 24
};

static unsigned msq1[] =
{
  03, 11, 20, 17,  9,
  22, 19,  8,  1, 10,
  06,  0, 12, 24, 18,
  14, 23, 16,  5,  2,
  15,  7,  4, 13, 21
};

static unsigned short quic0[] = {
#include "quickmatrix199.h"
};

static unsigned short quic1[] = {
#include "quickmatrix199-2.h"
};
#endif

static unsigned int quic2[] = {
#include "quickmatrix257.h"
};

static unsigned int rect2x1[] = {
#include "ran.367.179.h"
};

static inline int
calc_ordered_point(unsigned x, unsigned y, int steps, int multiplier,
		   int size, int *map)
{
  int i, j;
  unsigned retval = 0;
  int divisor = 1;
  int div1;
  for (i = 0; i < steps; i++)
    {
      int xa = (x / divisor) % size;
      int ya = (y / divisor) % size;
      unsigned base;
      base = map[ya + (xa * size)];
      div1 = 1;
      for (j = i; j < steps - 1; j++)
	div1 *= size * size;
      retval += base * div1;
      divisor *= size;
    }
  return retval * multiplier;
}

static void
init_iterated_matrix(dither_matrix_t *mat, int size, int exp,
		     unsigned *array)
{
  int i;
  int x, y;
  mat->base = size;
  mat->exp = exp;
  mat->x_size = 1;
  for (i = 0; i < exp; i++)
    mat->x_size *= mat->base;
  mat->y_size = mat->x_size;
  mat->total_size = mat->x_size * mat->y_size;
  mat->matrix = malloc(sizeof(unsigned) * mat->x_size * mat->y_size);
  for (x = 0; x < mat->x_size; x++)
    for (y = 0; y < mat->y_size; y++)
      {
	mat->matrix[x + y * mat->x_size] =
	  calc_ordered_point(x, y, mat->exp, 1, mat->base, array);
	mat->matrix[x + y * mat->x_size] =
	  (long long) mat->matrix[x + y * mat->x_size] * 65536ll /
	  (long long) (mat->x_size * mat->y_size);
      }
  mat->last_x = mat->last_x_mod = 0;
  mat->last_y = mat->last_y_mod = 0;
  mat->index = 0;
  mat->i_own = 1;
}

static void
init_matrix(dither_matrix_t *mat, int x_size, int y_size,
	    unsigned int *array, int transpose)
{
  int x, y;
  mat->base = x_size;
  mat->exp = 1;
  mat->x_size = x_size;
  mat->y_size = y_size;
  mat->total_size = mat->x_size * mat->y_size;
  mat->matrix = malloc(sizeof(unsigned) * mat->x_size * mat->y_size);
  for (x = 0; x < mat->x_size; x++)
    for (y = 0; y < mat->y_size; y++)
      {
	if (transpose)
	  mat->matrix[x + y * mat->x_size] = array[y + x * mat->y_size];
	else
	  mat->matrix[x + y * mat->x_size] = array[x + y * mat->x_size];
	mat->matrix[x + y * mat->x_size] =
	  (long long) mat->matrix[x + y * mat->x_size] * 65536ll /
	  (long long) (mat->x_size * mat->y_size);
      }
  mat->last_x = mat->last_x_mod = 0;
  mat->last_y = mat->last_y_mod = 0;
  mat->index = 0;
  mat->i_own = 1;
}

#if 0
static void
init_matrix_short(dither_matrix_t *mat, int x_size, int y_size,
		  unsigned short *array, int transpose)
{
  int x, y;
  mat->base = x_size;
  mat->exp = 1;
  mat->x_size = x_size;
  mat->y_size = y_size;
  mat->total_size = mat->x_size * mat->y_size;
  mat->matrix = malloc(sizeof(unsigned) * mat->x_size * mat->y_size);
  for (x = 0; x < mat->x_size; x++)
    for (y = 0; y < mat->y_size; y++)
      {
	if (transpose)
	  mat->matrix[x + y * mat->x_size] = array[y + x * mat->x_size];
	else
	  mat->matrix[x + y * mat->x_size] = array[x + y * mat->x_size];
	mat->matrix[x + y * mat->x_size] =
	  (long long) mat->matrix[x + y * mat->x_size] * 65536ll /
	  (long long) (mat->x_size * mat->y_size);
      }
  mat->last_x = mat->last_x_mod = 0;
  mat->last_y = mat->last_y_mod = 0;
  mat->index = 0;
  mat->i_own = 1;
}
#endif

static void
destroy_matrix(dither_matrix_t *mat)
{
  if (mat->i_own && mat->matrix)
    free(mat->matrix);
  mat->matrix = NULL;
  mat->base = 0;
  mat->exp = 0;
  mat->x_size = 0;
  mat->y_size = 0;
  mat->total_size = 0;
  mat->i_own = 0;
}

static void
clone_matrix(const dither_matrix_t *src, dither_matrix_t *dest,
	     int x_offset, int y_offset)
{
  dest->base = src->base;
  dest->exp = src->exp;
  dest->x_size = src->x_size;
  dest->y_size = src->y_size;
  dest->total_size = src->total_size;
  dest->matrix = src->matrix;
  dest->x_offset = x_offset;
  dest->y_offset = y_offset;
  dest->last_x = 0;
  dest->last_x_mod = dest->x_offset % dest->x_size;
  dest->last_y = 0;
  dest->last_y_mod = dest->x_size * (dest->y_offset % dest->y_size);
  dest->index = dest->last_x_mod + dest->last_y_mod;
  dest->i_own = 0;
}

static void
copy_matrix(const dither_matrix_t *src, dither_matrix_t *dest)
{
  int x;
  dest->base = src->base;
  dest->exp = src->exp;
  dest->x_size = src->x_size;
  dest->y_size = src->y_size;
  dest->total_size = src->total_size;
  dest->matrix = malloc(sizeof(unsigned) * dest->x_size * dest->y_size);
  for (x = 0; x < dest->x_size * dest->y_size; x++)
    dest->matrix[x] = src->matrix[x];
  dest->x_offset = 0;
  dest->y_offset = 0;
  dest->last_x = 0;
  dest->last_x_mod = 0;
  dest->last_y = 0;
  dest->last_y_mod = 0;
  dest->index = 0;
  dest->i_own = 1;
}

static void
exponential_scale_matrix(dither_matrix_t *mat, double exponent)
{
  int i;
  for (i = 0; i < mat->x_size * mat->y_size; i++)
    {
      double dd = mat->matrix[i] / 65535.0;
      dd = pow(dd, exponent);
      mat->matrix[i] = 65535 * dd;
    }
}

static void
matrix_set_row(const dither_t *d, dither_matrix_t *mat, int y)
{
  mat->last_y = y;
  mat->last_y_mod = mat->x_size * ((y + mat->y_offset) % mat->y_size);
  mat->index = mat->last_x_mod + mat->last_y_mod;
}

static inline unsigned
ditherpoint(const dither_t *d, dither_matrix_t *mat, int x)
{
  /*
   * This rather bizarre code is an attempt to avoid having to compute a lot
   * of modulus and multiplication operations, which are typically slow.
   */

  if (x == mat->last_x + 1)
    {
      mat->last_x = x;
      mat->last_x_mod++;
      mat->index++;
      if (mat->last_x_mod >= mat->x_size)
	{
	  mat->last_x_mod -= mat->x_size;
	  mat->index -= mat->x_size;
	}
    }
  else if (x == mat->last_x - 1)
    {
      mat->last_x = x;
      mat->last_x_mod--;
      mat->index--;
      if (mat->last_x_mod < 0)
	{
	  mat->last_x_mod += mat->x_size;
	  mat->index += mat->x_size;
	}
    }
  else if (x == mat->last_x)
    {
    }
  else
    {
      mat->last_x = x;
      mat->last_x_mod = (x + mat->x_offset) % mat->x_size;
      mat->index = mat->last_x_mod + mat->last_y_mod;
    }
  return mat->matrix[mat->index];
}

static inline unsigned
ditherpoint_fast(const dither_t *d, dither_matrix_t *mat, int x)
{
  if (mat->exp == 1)
    return ditherpoint(d, mat, x);
  else
    return mat->matrix[(mat->last_y_mod +
			((x + mat->x_offset) & DITHER_FAST_MASK))];
}

void *
init_dither(int in_width, int out_width, int horizontal_aspect,
	    int vertical_aspect, vars_t *v)
{
  int x_3, y_3;
  dither_t *d = malloc(sizeof(dither_t));
  simple_dither_range_t r;
  memset(d, 0, sizeof(dither_t));
  r.value = 1.0;
  r.bit_pattern = 1;
  r.is_dark = 1;
  r.dot_size = 1;
  dither_set_c_ranges(d, 1, &r, 1.0);
  dither_set_m_ranges(d, 1, &r, 1.0);
  dither_set_y_ranges(d, 1, &r, 1.0);
  dither_set_k_ranges(d, 1, &r, 1.0);
  d->offset0_table = NULL;
  d->offset1_table = NULL;
  d->x_aspect = horizontal_aspect;
  d->y_aspect = vertical_aspect;

  if (!strcmp(v->dither_algorithm, "Hybrid Floyd-Steinberg"))
    d->dither_type = D_FLOYD_HYBRID;
  else if (!strcmp(v->dither_algorithm, "Random Floyd-Steinberg"))
    d->dither_type = D_FLOYD;
  else if (!strcmp(v->dither_algorithm, "Ordered"))
    d->dither_type = D_ORDERED;
  else if (!strcmp(v->dither_algorithm, "Adaptive Hybrid"))
    d->dither_type = D_ADAPTIVE_HYBRID;
  else if (!strcmp(v->dither_algorithm, "Adaptive Random"))
    d->dither_type = D_ADAPTIVE_RANDOM;
  else if (!strcmp(v->dither_algorithm, "Fast"))
    d->dither_type = D_FAST;
  else if (!strcmp(v->dither_algorithm, "Very Fast"))
    d->dither_type = D_VERY_FAST;
  else
    d->dither_type = D_FLOYD_HYBRID;

  if (d->dither_type == D_VERY_FAST)
    init_iterated_matrix(&(d->mat6), 2, DITHER_FAST_STEPS, sq2);
  else
    {
      if (d->y_aspect / d->x_aspect == 2)
	init_matrix(&(d->mat6), 367, 179, rect2x1, 0);
      else if (d->x_aspect / d->y_aspect == 2)
	init_matrix(&(d->mat6), 179, 367, rect2x1, 1);
      else
	init_matrix(&(d->mat6), 257, 257, quic2, 0);
    }

  x_3 = d->mat6.x_size / 3;
  y_3 = d->mat6.y_size / 3;

  clone_matrix(&(d->mat6), &(d->c_dithermat), 2 * x_3, y_3);
  clone_matrix(&(d->mat6), &(d->m_dithermat), x_3, 2 * y_3);
  clone_matrix(&(d->mat6), &(d->y_dithermat), 0, y_3);
  clone_matrix(&(d->mat6), &(d->k_dithermat), 0, 0);
  dither_set_transition(d, .6);

  d->src_width = in_width;
  d->dst_width = out_width;
  d->adaptive_divisor = 2;

  dither_set_max_ink(d, INT_MAX, 1.0);
  dither_set_ink_spread(d, 13);
  dither_set_black_lower(d, .4);
  dither_set_black_upper(d, .7);
  dither_set_black_levels(d, 1.0, 1.0, 1.0);
  dither_set_randomizers(d, 1.0, 1.0, 1.0, 1.0);
  dither_set_ink_darkness(d, .4, .3, .2);
  dither_set_density(d, 1.0);
  return d;
}

void
dither_set_transition(void *vd, double exponent)
{
  dither_t *d = (dither_t *) vd;
  int x_3 = d->mat6.x_size / 3;
  int y_3 = d->mat6.y_size / 3;
  destroy_matrix(&(d->c_pick));
  destroy_matrix(&(d->m_pick));
  destroy_matrix(&(d->y_pick));
  destroy_matrix(&(d->k_pick));
  destroy_matrix(&(d->mat7));
  copy_matrix(&(d->mat6), &(d->mat7));
  exponential_scale_matrix(&(d->mat7), exponent);
  clone_matrix(&(d->mat7), &(d->c_pick), x_3, 0);
  clone_matrix(&(d->mat7), &(d->m_pick), 0, 2 * y_3);
  clone_matrix(&(d->mat7), &(d->y_pick), 2 * x_3, 0);
  clone_matrix(&(d->mat7), &(d->k_pick), x_3, 2 * y_3);
}

void
dither_set_density(void *vd, double density)
{
  dither_t *d = (dither_t *) vd;
  if (density > 1)
    density = 1;
  else if (density < 0)
    density = 0;
  d->k_upper = d->k_upper * density;
  d->k_lower = d->k_lower * density;
  d->density = (int) ((65536 * density) + .5);
  d->density2 = 2 * d->density;
  d->dlb_range = d->density - d->k_lower;
  d->bound_range = d->k_upper - d->k_lower;
  d->d_cutoff = d->density / 16;
  d->adaptive_limit = d->density / d->adaptive_divisor;
  d->adaptive_lower_limit = d->adaptive_limit / 4;
}

static double
imax(double a, double b)
{
  return ((a > b) ? a : b);
}

void
dither_set_max_ink(void *vd, int levels, double max_ink)
{
  dither_t *d = (dither_t *) vd;
  d->ink_limit = imax(max_ink, 1)*levels;
  d->ink_limit = max_ink*levels+0.5;
#ifdef VERBOSE
  fprintf(stderr, "Maxink: %f %d\n", max_ink, d->ink_limit);
#endif
}

void
dither_set_adaptive_divisor(void *vd, unsigned divisor)
{
  dither_t *d = (dither_t *) vd;
  d->adaptive_divisor = divisor;
  d->adaptive_limit = d->density / d->adaptive_divisor;
  d->adaptive_lower_limit = d->adaptive_limit / 4;
}

void
dither_set_black_lower(void *vd, double k_lower)
{
  dither_t *d = (dither_t *) vd;
  d->k_lower = (int) (k_lower * 65536);
}

void
dither_set_black_upper(void *vd, double k_upper)
{
  dither_t *d = (dither_t *) vd;
  d->k_upper = (int) (k_upper * 65536);
}

void
dither_set_ink_spread(void *vd, int spread)
{
  dither_t *d = (dither_t *) vd;
  if (d->offset0_table)
    {
      free(d->offset0_table);
      d->offset0_table = NULL;
    }
  if (d->offset1_table)
    {
      free(d->offset1_table);
      d->offset1_table = NULL;
    }
  if (spread >= 16)
    {
      d->spread = 16;
    }
  else
    {
      int max_offset;
      int i;
      d->spread = spread;
      max_offset = (1 << (16 - spread)) + 1;
      d->offset0_table = malloc(sizeof(int) * max_offset);
      d->offset1_table = malloc(sizeof(int) * max_offset);
      for (i = 0; i < max_offset; i++)
	{
	  d->offset0_table[i] = (i + 1) * (i + 1);
	  d->offset1_table[i] = ((i + 1) * i) / 2;
	}
    }

  d->adaptive_limit = d->density / d->adaptive_divisor;
  d->adaptive_lower_limit = d->adaptive_limit / 4;
}

void
dither_set_black_levels(void *vd, double c, double m, double y)
{
  dither_t *d = (dither_t *) vd;
  d->k_clevel = (int) (c * 64);
  d->k_mlevel = (int) (m * 64);
  d->k_ylevel = (int) (y * 64);
}

void
dither_set_randomizers(void *vd, double c, double m, double y, double k)
{
  dither_t *d = (dither_t *) vd;
  d->c_randomizer = c * 65536;
  d->m_randomizer = m * 65536;
  d->y_randomizer = y * 65536;
  d->k_randomizer = k * 65536;
}

void
dither_set_ink_darkness(void *vd, double c, double m, double y)
{
  dither_t *d = (dither_t *) vd;
  d->c_darkness = (int) (c * 64);
  d->m_darkness = (int) (m * 64);
  d->y_darkness = (int) (y * 64);
}

void
dither_set_light_inks(void *vd, double c, double m, double y, double density)
{
  simple_dither_range_t range[2];
  range[0].bit_pattern = 1;
  range[0].is_dark = 0;
  range[1].value = 1;
  range[1].bit_pattern = 1;
  range[1].is_dark = 1;
  range[1].dot_size = 1;
  if (c > 0)
    {
      range[0].value = c;
      range[0].dot_size = 1;
      dither_set_c_ranges(vd, 2, range, density);
    }
  if (m > 0)
    {
      range[0].value = m;
      range[0].dot_size = 1;
      dither_set_m_ranges(vd, 2, range, density);
    }
  if (y > 0)
    {
      range[0].value = y;
      range[0].dot_size = 1;
      dither_set_y_ranges(vd, 2, range, density);
    }
}

static void
dither_set_ranges(dither_color_t *s, int nlevels,
		  const simple_dither_range_t *ranges, double density)
{
  int i;
  unsigned lbit;
  if (s->ranges)
    free(s->ranges);
  s->nlevels = nlevels > 1 ? nlevels + 1 : nlevels;
  s->ranges = (dither_segment_t *)
    malloc(s->nlevels * sizeof(dither_segment_t));
  s->bit_max = 0;
#ifdef VERBOSE
  fprintf(stderr, "dither_set_ranges nlevels %d density %f\n", nlevels, density);
  for (i = 0; i < nlevels; i++)
    fprintf(stderr, "  level %d value %f pattern %x is_dark %d\n", i,
	    ranges[i].value, ranges[i].bit_pattern, ranges[i].is_dark);
#endif
  s->ranges[0].range_l = 0;
  s->ranges[0].value_l = ranges[0].value * 65536.0;
  s->ranges[0].bits_l = ranges[0].bit_pattern;
  s->ranges[0].isdark_l = ranges[0].is_dark;
  s->ranges[0].dot_size_l = ranges[0].dot_size;
  if (nlevels == 1)
    s->ranges[0].range_h = 65536;
  else
    s->ranges[0].range_h = ranges[0].value * 65536.0 * density;
  if (s->ranges[0].range_h > 65536)
    s->ranges[0].range_h = 65536;
  s->ranges[0].value_h = ranges[0].value * 65536.0;
  if (s->ranges[0].value_h > 65536)
    s->ranges[0].value_h = 65536;
  s->ranges[0].bits_h = ranges[0].bit_pattern;
  if (ranges[0].bit_pattern > s->bit_max)
    s->bit_max = ranges[0].bit_pattern;
  s->ranges[0].isdark_h = ranges[0].is_dark;
  s->ranges[0].dot_size_h = ranges[0].dot_size;
  s->ranges[0].range_span = s->ranges[0].range_h;
  s->ranges[0].value_span = 0;
  if (s->nlevels > 1)
    {
      for (i = 0; i < nlevels - 1; i++)
	{
	  int l = i + 1;
	  s->ranges[l].range_l = s->ranges[i].range_h;
	  s->ranges[l].value_l = s->ranges[i].value_h;
	  s->ranges[l].bits_l = s->ranges[i].bits_h;
	  s->ranges[l].isdark_l = s->ranges[i].isdark_h;
	  s->ranges[l].dot_size_l = s->ranges[i].dot_size_h;
	  if (i == nlevels - 1)
	    s->ranges[l].range_h = 65536;
	  else
	    s->ranges[l].range_h =
	      (ranges[l].value + ranges[l].value) * 65536.0 * density / 2;
	  if (s->ranges[l].range_h > 65536)
	    s->ranges[l].range_h = 65536;
	  s->ranges[l].value_h = ranges[l].value * 65536.0;
	  if (s->ranges[l].value_h > 65536)
	    s->ranges[l].value_h = 65536;
	  s->ranges[l].bits_h = ranges[l].bit_pattern;
	  if (ranges[l].bit_pattern > s->bit_max)
	    s->bit_max = ranges[l].bit_pattern;
	  s->ranges[l].isdark_h = ranges[l].is_dark;
	  s->ranges[l].dot_size_h = ranges[l].dot_size;
	  s->ranges[l].range_span =
	    s->ranges[l].range_h - s->ranges[l].range_l;
	  s->ranges[l].value_span =
	    s->ranges[l].value_h - s->ranges[l].value_l;
	}
      i++;
      s->ranges[i].range_l = s->ranges[i - 1].range_h;
      s->ranges[i].value_l = s->ranges[i - 1].value_h;
      s->ranges[i].bits_l = s->ranges[i - 1].bits_h;
      s->ranges[i].isdark_l = s->ranges[i - 1].isdark_h;
      s->ranges[i].dot_size_l = s->ranges[i - 1].dot_size_h;
      s->ranges[i].range_h = 65536;
      s->ranges[i].value_h = s->ranges[i].value_l;
      s->ranges[i].bits_h = s->ranges[i].bits_l;
      s->ranges[i].isdark_h = s->ranges[i].isdark_l;
      s->ranges[i].dot_size_h = s->ranges[i].dot_size_l;
      s->ranges[i].range_span = s->ranges[i].range_h - s->ranges[i].range_l;
      s->ranges[i].value_span = s->ranges[i].value_h - s->ranges[i].value_l;
    }
  lbit = s->bit_max;
  s->signif_bits = 0;
  while (lbit > 0)
    {
      s->signif_bits++;
      lbit >>= 1;
    }
#ifdef VERBOSE
  for (i = 0; i < s->nlevels; i++)
    {
      fprintf(stderr, "    level %d value_l %d value_h %d range_l %d range_h %d\n",
	      i, s->ranges[i].value_l, s->ranges[i].value_h,
	      s->ranges[i].range_l, s->ranges[i].range_h);
      fprintf(stderr, "       bits_l %d bits_h %d isdark_l %d isdark_h %d\n",
	      s->ranges[i].bits_l, s->ranges[i].bits_h,
	      s->ranges[i].isdark_l, s->ranges[i].isdark_h);
      fprintf(stderr, "       rangespan %d valuespan %d\n",
	      s->ranges[i].range_span, s->ranges[i].value_span);
    }
  fprintf(stderr, "  bit_max %d signif_bits %d\n", s->bit_max, s->signif_bits);
#endif
}

static void
dither_set_ranges_full(dither_color_t *s, int nlevels,
		       const full_dither_range_t *ranges, double density,
		       int max_ink)
{
  int i, j;
  unsigned lbit;
  if (s->ranges)
    free(s->ranges);
  s->nlevels = nlevels > 1 ? nlevels + 1 : nlevels;
  s->nlevels = nlevels+1;
  s->ranges = (dither_segment_t *)
    malloc(s->nlevels * sizeof(dither_segment_t));
  s->bit_max = 0;
#ifdef VERBOSE
  fprintf(stderr, "dither_set_ranges nlevels %d density %f\n", nlevels, density);
  for (i = 0; i < nlevels; i++)
    fprintf(stderr, "  level %d value: low %f high %f pattern low %x high %x is_dark low %d high %d\n", i,
	    ranges[i].value_l, ranges[i].value_h, ranges[i].bits_l, ranges[i].bits_h,ranges[i].isdark_l, ranges[i].isdark_h);
#endif
  for(i=j=0; i < nlevels; i++) {
    if (ranges[i].bits_h > s->bit_max)
      s->bit_max = ranges[i].bits_h;
    if (ranges[i].bits_l > s->bit_max)
      s->bit_max = ranges[i].bits_l;
    s->ranges[j].dot_size_l = ranges[i].bits_l; /* FIXME */
    s->ranges[j].dot_size_h = ranges[i].bits_h;
	/*if(s->ranges[j].dot_size_l > max_ink || s->ranges[j].dot_size_h > max_ink)
		   continue;*/
    s->ranges[j].value_l = ranges[i].value_l * 65535;
    s->ranges[j].value_h = ranges[i].value_h * 65535;
    s->ranges[j].range_l = s->ranges[j].value_l*density;
    s->ranges[j].range_h = s->ranges[j].value_h*density;
    s->ranges[j].bits_l = ranges[i].bits_l;
    s->ranges[j].bits_h = ranges[i].bits_h;
    s->ranges[j].isdark_l = ranges[i].isdark_l;
    s->ranges[j].isdark_h = ranges[i].isdark_h;
    s->ranges[j].range_span = s->ranges[j].range_h-s->ranges[j].range_l;
    s->ranges[j].value_span = s->ranges[j].value_h-s->ranges[j].value_l;
	j++;
  }
  s->ranges[j].range_l = s->ranges[j - 1].range_h;
  s->ranges[j].value_l = s->ranges[j - 1].value_h;
  s->ranges[j].bits_l = s->ranges[j - 1].bits_h;
  s->ranges[j].isdark_l = s->ranges[j - 1].isdark_h;
  s->ranges[j].dot_size_l = s->ranges[j - 1].dot_size_h;
  s->ranges[j].range_h = 65535;
  s->ranges[j].value_h = 65535;
  s->ranges[j].bits_h = s->ranges[j].bits_l;
  s->ranges[j].isdark_h = s->ranges[j].isdark_l;
  s->ranges[j].dot_size_h = s->ranges[j].dot_size_l;
  s->ranges[j].range_span = s->ranges[j].range_h - s->ranges[j].range_l;
  s->ranges[j].value_span = 0;
  s->nlevels = j+1;
  lbit = s->bit_max;
  s->signif_bits = 0;
  while (lbit > 0)
    {
      s->signif_bits++;
      lbit >>= 1;
    }
#ifdef VERBOSE
  for (i = 0; i < s->nlevels; i++)
    {
      fprintf(stderr, "    level %d value_l %d value_h %d range_l %d range_h %d\n",
	      i, s->ranges[i].value_l, s->ranges[i].value_h,
	      s->ranges[i].range_l, s->ranges[i].range_h);
      fprintf(stderr, "       bits_l %d bits_h %d isdark_l %d isdark_h %d\n",
	      s->ranges[i].bits_l, s->ranges[i].bits_h,
	      s->ranges[i].isdark_l, s->ranges[i].isdark_h);
      fprintf(stderr, "       rangespan %d valuespan %d\n",
	      s->ranges[i].range_span, s->ranges[i].value_span);
    }
  fprintf(stderr, "  bit_max %d signif_bits %d\n", s->bit_max, s->signif_bits);
#endif
}

void
dither_set_c_ranges(void *vd, int nlevels, const simple_dither_range_t *ranges,
		    double density)
{
  dither_t *d = (dither_t *) vd;
  dither_set_ranges(&(d->c_dither), nlevels, ranges, density);
}

void
dither_set_c_ranges_simple(void *vd, int nlevels, const double *levels,
			   double density)
{
  simple_dither_range_t *r = malloc(nlevels * sizeof(simple_dither_range_t));
  int i;
  for (i = 0; i < nlevels; i++)
    {
      r[i].bit_pattern = i + 1;
      r[i].dot_size = i + 1;
      r[i].value = levels[i];
      r[i].is_dark = 1;
    }
  dither_set_c_ranges(vd, nlevels, r, density);
  free(r);
}

void
dither_set_c_ranges_full(void *vd, int nlevels,
			 const full_dither_range_t *ranges, double density)
{
  dither_t *d = (dither_t *) vd;
  dither_set_ranges_full(&(d->c_dither), nlevels, ranges, density,
			 d->ink_limit);
}

void
dither_set_m_ranges(void *vd, int nlevels, const simple_dither_range_t *ranges,
		    double density)
{
  dither_t *d = (dither_t *) vd;
  dither_set_ranges(&(d->m_dither), nlevels, ranges, density);
}

void
dither_set_m_ranges_simple(void *vd, int nlevels, const double *levels,
			   double density)
{
  simple_dither_range_t *r = malloc(nlevels * sizeof(simple_dither_range_t));
  int i;
  for (i = 0; i < nlevels; i++)
    {
      r[i].bit_pattern = i + 1;
      r[i].dot_size = i + 1;
      r[i].value = levels[i];
      r[i].is_dark = 1;
    }
  dither_set_m_ranges(vd, nlevels, r, density);
  free(r);
}

void
dither_set_m_ranges_full(void *vd, int nlevels,
			 const full_dither_range_t *ranges, double density)
{
  dither_t *d = (dither_t *) vd;
  dither_set_ranges_full(&(d->m_dither), nlevels, ranges, density,
			 d->ink_limit);
}

void
dither_set_y_ranges(void *vd, int nlevels, const simple_dither_range_t *ranges,
		    double density)
{
  dither_t *d = (dither_t *) vd;
  dither_set_ranges(&(d->y_dither), nlevels, ranges, density);
}

void
dither_set_y_ranges_simple(void *vd, int nlevels, const double *levels,
			   double density)
{
  simple_dither_range_t *r = malloc(nlevels * sizeof(simple_dither_range_t));
  int i;
  for (i = 0; i < nlevels; i++)
    {
      r[i].bit_pattern = i + 1;
      r[i].dot_size = i + 1;
      r[i].value = levels[i];
      r[i].is_dark = 1;
    }
  dither_set_y_ranges(vd, nlevels, r, density);
  free(r);
}

void
dither_set_y_ranges_full(void *vd, int nlevels,
			 const full_dither_range_t *ranges, double density)
{
  dither_t *d = (dither_t *) vd;
  dither_set_ranges_full(&(d->y_dither), nlevels, ranges, density,
			 d->ink_limit);
}

void
dither_set_k_ranges(void *vd, int nlevels, const simple_dither_range_t *ranges,
		    double density)
{
  dither_t *d = (dither_t *) vd;
  dither_set_ranges(&(d->k_dither), nlevels, ranges, density);
}

void
dither_set_k_ranges_simple(void *vd, int nlevels, const double *levels,
			   double density)
{
  simple_dither_range_t *r = malloc(nlevels * sizeof(simple_dither_range_t));
  int i;
  for (i = 0; i < nlevels; i++)
    {
      r[i].bit_pattern = i + 1;
      r[i].dot_size = i + 1;
      r[i].value = levels[i];
      r[i].is_dark = 1;
    }
  dither_set_k_ranges(vd, nlevels, r, density);
  free(r);
}

void
dither_set_k_ranges_full(void *vd, int nlevels,
			 const full_dither_range_t *ranges, double density)
{
  dither_t *d = (dither_t *) vd;
  dither_set_ranges_full(&(d->k_dither), nlevels, ranges, density,
			 d->ink_limit);
}

void
free_dither(void *vd)
{
  dither_t *d = (dither_t *) vd;
  int i;
  int j;
  for (j = 0; j < NCOLORS; j++)
    {
      if (d->vals[j])
	{
	  free(d->vals[j]);
	  d->vals[j] = NULL;
	}
      for (i = 0; i < ERROR_ROWS; i++)
	{
	  if (d->errs[i][j])
	    {
	      free(d->errs[i][j]);
	      d->errs[i][j] = NULL;
	    }
	}
    }
  free(d->c_dither.ranges);
  d->c_dither.ranges = NULL;
  free(d->m_dither.ranges);
  d->m_dither.ranges = NULL;
  free(d->y_dither.ranges);
  d->y_dither.ranges = NULL;
  free(d->k_dither.ranges);
  d->k_dither.ranges = NULL;
  if (d->offset0_table)
    {
      free(d->offset0_table);
      d->offset0_table = NULL;
    }
  if (d->offset1_table)
    {
      free(d->offset1_table);
      d->offset1_table = NULL;
    }
  destroy_matrix(&(d->c_pick));
  destroy_matrix(&(d->c_dithermat));
  destroy_matrix(&(d->m_pick));
  destroy_matrix(&(d->m_dithermat));
  destroy_matrix(&(d->y_pick));
  destroy_matrix(&(d->y_dithermat));
  destroy_matrix(&(d->k_pick));
  destroy_matrix(&(d->k_dithermat));
  destroy_matrix(&(d->mat6));
  destroy_matrix(&(d->mat7));
  free(d);
}

static int *
get_errline(dither_t *d, int row, int color)
{
  if (row < 0 || color < 0 || color >= NCOLORS)
    return NULL;
  if (d->errs[row & 1][color])
    return d->errs[row & 1][color] + MAX_SPREAD;
  else
    {
      int size = 2 * MAX_SPREAD + (16 * ((d->dst_width + 7) / 8));
      d->errs[row & 1][color] = malloc(size * sizeof(int));
      memset(d->errs[row & 1][color], 0, size * sizeof(int));
      return d->errs[row & 1][color] + MAX_SPREAD;
    }
}

static unsigned short *
get_valueline(dither_t *d, int color)
{
  if (color < 0 || color >= NCOLORS)
    return NULL;
  if (d->vals[color])
    return d->vals[color];
  else
    {
      int size = (8 * ((d->dst_width + 7) / 8));
      d->vals[color] = malloc(size * sizeof(unsigned short));
      return d->vals[color];
    }
}

/*
 * Add the error to the input value.  Notice that we micro-optimize this
 * to save a division when appropriate.
 */

#define UPDATE_COLOR(color, dither) (\
        ((dither) >= 0)? \
                (color) + ((dither) >> 3): \
                (color) - ((-(dither)) >> 3))

/*
 * For Floyd-Steinberg, distribute the error residual.  We spread the
 * error to nearby points, spreading more broadly in lighter regions to
 * achieve more uniform distribution of color.  The actual distribution
 * is a triangular function.
 */

static inline int
update_dither(int r, int o, int width, int odb, int odb_mask,
	      int direction, int *error0, int *error1, dither_t *d)
{
  int tmp = r;
  if (tmp != 0)
    {
      int i, dist;
      int dist1;
      int offset;
      int delta, delta1;
      int nextspread = 4;
      int thisspread = 8 - nextspread;
      if (tmp > 65535)
	tmp = 65535;
      if (odb >= 16 || o >= 2048)
	offset = 0;
      else
	{
	  int tmpo = o * 32;
	  offset = (65535 - (tmpo & 0xffff)) >> odb;
	  if ((rand() & odb_mask) > (tmpo & odb_mask))
	    offset++;
	  if (offset > MAX_SPREAD - 1)
	    offset = MAX_SPREAD - 1;
	}
      if (offset == 0)
	{
	  dist = nextspread * tmp;
	  error1[0] += dist;
	  return error0[direction] + thisspread * tmp;
	}
      else
	{
	  dist = nextspread * tmp / d->offset0_table[offset];
	  dist1 = thisspread * tmp / d->offset1_table[offset];
	  delta1 = dist1 * offset;
	}
      delta = dist;
      for (i = -offset; i <= offset; i++)
	{
	  error1[i] += delta;
	  if ((i > 0 && direction > 0) || (i < 0 && direction < 0))
	    {
	      error0[i] += delta1;
	      delta1 -= dist1;
	    }
	  if (i < 0)
	    delta += dist;
	  else
	    delta -= dist;
	}
    }
  return error0[direction];
}

/*
 * Print a single dot.  This routine has become awfully complicated
 * awfully fast!
 *
 * Note that the ink budget is both an input and an output parameter
 */

static inline int
print_color(dither_t *d, dither_color_t *rv, int base, int density,
	    int adjusted, int x, int y, unsigned char *c, unsigned char *lc,
	    unsigned char bit, int length, unsigned randomizer, int dontprint,
	    int *ink_budget, dither_matrix_t *pick_matrix,
	    dither_matrix_t *dither_matrix, int dither_type)
{
  unsigned rangepoint;
  unsigned virtual_value;
  unsigned vmatrix;
  int i;
  int j;
  int isdark;
  unsigned char *tptr;
  unsigned bits;
  unsigned v;
  unsigned dot_size;
  int levels = rv->nlevels - 1;
  int dither_value = adjusted;
  dither_segment_t *dd;

  if ((adjusted <= 0 && !(dither_type & D_ADAPTIVE_BASE)) ||
      base <= 0 || density <= 0)
    return adjusted;
  if (density > 65536)
    density = 65536;

  /*
   * Look for the appropriate range into which the input value falls.
   * Notice that we use the input, not the error, to decide what dot type
   * to print (if any).  We actually use the "density" input to permit
   * the caller to use something other that simply the input value, if it's
   * desired to use some function of overall density, rather than just
   * this color's input, for this purpose.
   */
  for (i = levels; i >= 0; i--)
    {
      dd = &(rv->ranges[i]);

      if (density <= dd->range_l)
	continue;

      /*
       * If we're using an adaptive dithering method, decide whether
       * to use the Floyd-Steinberg or the ordered method based on the
       * input value.  The choice of 1/128 is somewhat arbitrary and
       * could stand to be parameterized.  Another possibility would be
       * to scale to something less than pure ordered at 0 input value.
       */
      if (dither_type & D_ADAPTIVE_BASE)
	{
	  dither_type -= D_ADAPTIVE_BASE;
	  if (base <= d->adaptive_limit)
	    {
	      dither_type = D_ORDERED;
	      dither_value = base;
	    }
	  else if (adjusted <= 0)
	    return adjusted;
	}

      /*
       * Where are we within the range.  If we're going to print at
       * all, this determines the probability of printing the darker
       * vs. the lighter ink.  If the inks are identical (same value
       * and darkness), it doesn't matter.
       *
       * We scale the input linearly against the top and bottom of the
       * range.
       */
      if (dd->range_span == 0 ||
	  (dd->value_span == 0 && dd->isdark_l == dd->isdark_h))
	rangepoint = 32768;
      else
	rangepoint =
	  ((unsigned) (density - dd->range_l)) * 65536 / dd->range_span;

      /*
       * Compute the virtual dot size that we're going to print.
       * This is somewhere between the two candidate dot sizes.
       * This is scaled between the high and low value.
       */

      if (dd->value_span == 0)
	virtual_value = dd->value_h;
      else if (dd->range_span == 0)
	virtual_value = (dd->value_h + dd->value_l) / 2;
      else if (dd->value_h == 65536 && rangepoint == 65536)
	virtual_value = 65536;
      else
	virtual_value = dd->value_l + (dd->value_span * rangepoint / 65536);

      /*
       * Reduce the randomness as the base value increases, to get
       * smoother output in the midtones.  Idea suggested by
       * Thomas Tonino.
       */
      if (!(dither_type & D_ORDERED_BASE))
	{
	  if (randomizer > 0)
	    {
	      if (base > d->d_cutoff)
		randomizer = 0;
	      else if (base > d->d_cutoff / 2)
		randomizer = randomizer * 2 * (d->d_cutoff - base) / d->d_cutoff;
	    }
	}
      else
	randomizer = 65536;	/* With ordered dither, we need this */

      /*
       * Compute the comparison value to decide whether to print at
       * all.  If there is no randomness, simply divide the virtual
       * dotsize by 2 to get standard "pure" Floyd-Steinberg (or "pure"
       * matrix dithering, which degenerates to a threshold).
       */
      if (randomizer == 0)
	vmatrix = virtual_value / 2;
      else
	{
	  /*
	   * First, compute a value between 0 and 65536 that will be
	   * scaled to produce an offset from the desired threshold.
	   */
	  switch (dither_type)
	    {
	    case D_FLOYD:
	      /*
	       * Floyd-Steinberg: use a mildly Gaussian random number.
	       * This might be a bit too Gaussian.
	       */
	      vmatrix = ((rand() & 0xffff000) +
			 (rand() & 0xffff000)) >> 13;
	      break;
	    case D_FLOYD_HYBRID:
	      /*
	       * Hybrid Floyd-Steinberg: use a matrix to generate the offset.
	       */
	    case D_ORDERED:
	    default:
	      vmatrix = ditherpoint(d, dither_matrix, x);
	    }

	  if (vmatrix == 65536 && virtual_value == 65536)
	    /*
	     * These numbers will break 32-bit unsigned arithmetic!
	     * Maybe this is so rare that we'd be better off using
	     * long long arithmetic, but that's likely to be much more
	     * expensive on 32-bit architectures.
	     */
	    vmatrix = 65536;
	  else
	    {
	      /*
	       * Now, scale the virtual dot size appropriately.  Note that
	       * we'll get something evenly distributed between 0 and
	       * the virtual dot size, centered on the dot size / 2,
	       * which is the normal threshold value.
	       */
	      vmatrix = vmatrix * virtual_value / 65536;
	      if (randomizer != 65536)
		{
		  /*
		   * We want vmatrix to be scaled between 0 and
		   * virtual_value when randomizer is 65536 (fully random).
		   * When it's less, we want it to scale through part of
		   * that range. In all cases, it should center around
		   * virtual_value / 2.
		   *
		   * vbase is the bottom of the scaling range.
		   */
		  unsigned vbase = virtual_value * (65536u - randomizer) /
		    131072u;
		  vmatrix = vmatrix * randomizer / 65536;
		  vmatrix += vbase;
		}
	    }
	} /* randomizer != 0 */

      /*
       * After all that, printing is almost an afterthought.
       * Pick the actual dot size (using a matrix here) and print it.
       */
      if (dither_value >= vmatrix)
	{
	  if (dd->isdark_h == dd->isdark_l && dd->bits_h == dd->bits_l)
	    {
	      isdark = dd->isdark_h;
	      bits = dd->bits_h;
	      v = dd->value_h;
	      dot_size = dd->dot_size_h;
	    }
	  else if (rangepoint >= ditherpoint(d, pick_matrix, x))
	    {
	      isdark = dd->isdark_h;
	      bits = dd->bits_h;
	      v = dd->value_h;
	      dot_size = dd->dot_size_h;
	    }
	  else
	    {
	      isdark = dd->isdark_l;
	      bits = dd->bits_l;
	      v = dd->value_l;
	      dot_size = dd->dot_size_l;
	    }
	  tptr = isdark ? c : lc;

	  /*
	   * Lay down all of the bits in the pixel.
	   */
	  if (dontprint < v && (!ink_budget || *ink_budget >= dot_size))
	    {
	      for (j = 1; j <= bits; j += j, tptr += length)
		{
		  if (j & bits)
		    tptr[0] |= bit;
		}
	      if (ink_budget)
		*ink_budget -= dot_size;
	    }
	  if (dither_type & D_ORDERED_BASE)
	    adjusted = -(int) (2 * v / 4);
	  else
	    adjusted -= v;
	}
      return adjusted;
    }
  return adjusted;
}

static inline void
print_color_fast(dither_t *d, dither_color_t *rv, int base,
		 int adjusted, int x, int y, unsigned char *c,
		 unsigned char *lc, unsigned char bit, int length,
		 dither_matrix_t *dither_matrix, int very_fast)
{
  int i;
  int levels = rv->nlevels - 1;
  int j;
  unsigned char *tptr;
  unsigned bits;

  if (adjusted <= 0 || base <= 0)
    return;
  if (very_fast)
    {
      if (adjusted >= ditherpoint_fast(d, dither_matrix, x))
	c[0] |= bit;
      return;
    }
  /*
   * Look for the appropriate range into which the input value falls.
   * Notice that we use the input, not the error, to decide what dot type
   * to print (if any).  We actually use the "density" input to permit
   * the caller to use something other that simply the input value, if it's
   * desired to use some function of overall density, rather than just
   * this color's input, for this purpose.
   */
  for (i = levels; i >= 0; i--)
    {
      dither_segment_t *dd = &(rv->ranges[i]);
      unsigned vmatrix;
      if (base <= dd->range_l)
	continue;

      vmatrix = (dd->value_h * ditherpoint_fast(d, dither_matrix, x)) >> 16;

      /*
       * After all that, printing is almost an afterthought.
       * Pick the actual dot size (using a matrix here) and print it.
       */
      if (adjusted >= vmatrix)
	{
	  bits = dd->bits_h;
	  tptr = dd->isdark_h ? c : lc;

	  /*
	   * Lay down all of the bits in the pixel.
	   */
	  if (bits == 1)
	    {
	      tptr[0] |= bit;
	    }
	  else
	    {
	      for (j = 1; j <= bits; j += j, tptr += length)
		{
		  if (j & bits)
		    tptr[0] |= bit;
		}
	    }
	}
      return;
    }
  return;
}

/*
 * Dithering functions!
 *
 * Documentation moved to README.dither
 */

/*
 * 'dither_monochrome()' - Dither grayscale pixels to black using a hard
 * threshold.  This is for use with predithered output, or for text
 * or other pure black and white only.
 */

void
dither_monochrome(const unsigned short  *gray,	/* I - Grayscale pixels */
		 int           	    row,	/* I - Current Y coordinate */
		 void 		    *vd,
		 unsigned char 	    *black,	/* O - Black bitmap pixels */
		 int		    duplicate_line)
{
  int		x,		/* Current X coordinate */
		xerror,		/* X error count */
		xstep,		/* X step */
		xmod,		/* X error modulus */
		length;		/* Length of output bitmap in bytes */
  unsigned char	bit,		/* Current bit */
		*kptr;		/* Current black pixel */
  dither_t *d = (dither_t *) vd;
  dither_matrix_t *kdither = &(d->k_dithermat);
  unsigned bits = d->k_dither.signif_bits;
  int j;
  unsigned char *tptr;
  int dst_width = d->dst_width;

  bit = 128;
  x = 0;

  xstep  = d->src_width / d->dst_width;
  xmod   = d->src_width % d->dst_width;
  length = (d->dst_width + 7) / 8;

  memset(black, 0, length * bits);
  kptr = black;
  xerror = 0;
  matrix_set_row(d, kdither, row);
  for (x = 0; x < dst_width; x++)
    {
      if (!gray[0])
	{
	  if (d->density >= ditherpoint_fast(d, kdither, x))
	    {
	      tptr = kptr;
	      for (j = 0; j < bits; j++, tptr += length)
		tptr[0] |= bit;
	    }
	}

      gray   += xstep;
      xerror += xmod;
      if (bit == 1)
	{
	  kptr ++;
	  bit = 128;
	}
      else
	bit >>= 1;
      if (xerror >= dst_width)
	{
	  xerror -= dst_width;
	  gray++;
	}
    }
}

/*
 * 'dither_black()' - Dither grayscale pixels to black.
 * This is for grayscale output.
 */

void
dither_black_fast(const unsigned short   *gray,	/* I - Grayscale pixels */
		  int           row,		/* I - Current Y coordinate */
		  void 		*vd,
		  unsigned char *black,		/* O - Black bitmap pixels */
		  int		duplicate_line)
{

  int		x,		/* Current X coordinate */
		xerror,		/* X error count */
		xstep,		/* X step */
		xmod,		/* X error modulus */
		length;		/* Length of output bitmap in bytes */
  unsigned char	bit,		/* Current bit */
		*kptr;		/* Current black pixel */
  int		k;
  dither_t *d = (dither_t *) vd;
  dither_color_t *kd = &(d->k_dither);
  dither_matrix_t *kdither = &(d->k_dithermat);
  int dst_width = d->dst_width;
  int dither_very_fast = 0;
  if (kd->nlevels == 1 && kd->ranges[0].bits_h == 1 && kd->ranges[0].isdark_h)
    dither_very_fast = 1;

  bit = 128;

  xstep  = d->src_width / d->dst_width;
  xmod   = d->src_width % d->dst_width;
  length = (d->dst_width + 7) / 8;

  memset(black, 0, length * d->k_dither.signif_bits);
  kptr = black;
  xerror = 0;
  matrix_set_row(d, &(d->k_dithermat), row);

  for (x = 0; x < dst_width; x++)
    {
      k = 65535 - *gray;
      print_color_fast(d, kd, k, k, x, row, kptr, NULL, bit, length, kdither,
		       dither_very_fast);

      gray   += xstep;
      xerror += xmod;
      if (bit == 1)
	{
	  kptr ++;
	  bit = 128;
	}
      else
	bit >>= 1;
      if (xerror >= dst_width)
	{
	  xerror -= dst_width;
	  gray++;
	}
    }
}

void
dither_black(const unsigned short   *gray,	/* I - Grayscale pixels */
	     int           	row,		/* I - Current Y coordinate */
	     void 		*vd,
	     unsigned char 	*black,		/* O - Black bitmap pixels */
	     int		duplicate_line)
{

  int		x,		/* Current X coordinate */
		xerror,		/* X error count */
		xstep,		/* X step */
		xmod,		/* X error modulus */
		length;		/* Length of output bitmap in bytes */
  unsigned char	bit,		/* Current bit */
		*kptr;		/* Current black pixel */
  int		k, ok,		/* Current black error */
		ditherk,	/* Next error value in buffer */
		*kerror0,	/* Pointer to current error row */
		*kerror1;	/* Pointer to next error row */
  dither_t *d = (dither_t *) vd;
  int terminate;
  int direction = row & 1 ? 1 : -1;
  int odb = d->spread;
  int odb_mask = (1 << odb) - 1;
  int ink_budget;

  if (d->dither_type & D_FAST_BASE)
    {
      dither_black_fast(gray, row, vd, black, duplicate_line);
      return;
    }

  bit = (direction == 1) ? 128 : 1 << (7 - ((d->dst_width - 1) & 7));
  x = (direction == 1) ? 0 : d->dst_width - 1;
  terminate = (direction == 1) ? d->dst_width : -1;

  xstep  = d->src_width / d->dst_width;
  xmod   = d->src_width % d->dst_width;
  length = (d->dst_width + 7) / 8;

  kerror0 = get_errline(d, row, ECOLOR_K);
  kerror1 = get_errline(d, row + 1, ECOLOR_K);
  memset(kerror1, 0, d->dst_width * sizeof(int));

  memset(black, 0, length * d->k_dither.signif_bits);
  kptr = black;
  xerror = 0;
  if (direction == -1)
    {
      kerror0 += d->dst_width - 1;
      kerror1 += d->dst_width - 1;
      kptr = black + length - 1;
      xstep = -xstep;
      gray += d->src_width - 1;
      xerror = ((d->dst_width - 1) * xmod) % d->dst_width;
      xmod = -xmod;
    }
  matrix_set_row(d, &(d->k_dithermat), row);
  matrix_set_row(d, &(d->k_pick), row);

  for (ditherk = kerror0[0];
       x != terminate;
       x += direction,
	 kerror0 += direction,
	 kerror1 += direction)
    {
      ink_budget = d->ink_limit;

      k = 65535 - *gray;
      ok = k;
      if (d->dither_type & D_ORDERED_BASE)
	print_color(d, &(d->k_dither), k, k, k, x, row, kptr, NULL, bit,
		    length, d->k_randomizer, 0, &ink_budget,
		    &(d->k_pick), &(d->k_dithermat), d->dither_type);
      else
	{
	  k = UPDATE_COLOR(k, ditherk);
	  k = print_color(d, &(d->k_dither), ok, ok, k, x, row, kptr, NULL,
			  bit, length, d->k_randomizer, 0, &ink_budget,
			  &(d->k_pick), &(d->k_dithermat), d->dither_type);
	  if (!(d->dither_type & D_ORDERED_BASE))
	    ditherk = update_dither(k, ok, d->src_width, odb, odb_mask,
				    direction, kerror0, kerror1, d);
	}

      gray   += xstep;
      xerror += xmod;
      if (direction == 1)
	{
	  if (bit == 1)
	    {
	      kptr ++;
	      bit = 128;
	    }
	  else
	    bit >>= 1;
	  if (xerror >= d->dst_width)
	    {
	      xerror -= d->dst_width;
	      gray++;
	    }
	}
      else
	{
	  if (bit == 128)
	    {
	      kptr --;
	      bit = 1;
	    }
	  else
	    bit <<= 1;
	  if (xerror < 0)
	    {
	      xerror += d->dst_width;
	      gray--;
	    }
	}
    }
}

#define USMIN(a, b) ((a) < (b) ? (a) : (b))

static void
generate_cmy(dither_t *d,
	     const unsigned short *rgb,
	     int *nonzero,
	     int row)
{
  register unsigned short cc, mm, yy;
  register unsigned short *c = get_valueline(d, ECOLOR_C);
  register unsigned short *m = get_valueline(d, ECOLOR_M);
  register unsigned short *y = get_valueline(d, ECOLOR_Y);
  register int lnonzero = 0;
  register int x, xerror, xstep, xmod;

  if (d->src_width == d->dst_width)
    {
      for (x = d->dst_width; x > 0; x--)
	{
	  cc = *c++ = 65535 - *rgb++;
	  mm = *m++ = 65535 - *rgb++;
	  yy = *y++ = 65535 - *rgb++;

	  lnonzero |= cc || mm || yy;
	}
    }
  else
    {
      xstep  = 3 * (d->src_width / d->dst_width);
      xmod   = d->src_width % d->dst_width;
      xerror = 0;
      for (x = d->dst_width; x > 0; x--)
	{
	  cc = *c++ = 65535 - rgb[0];
	  mm = *m++ = 65535 - rgb[1];
	  yy = *y++ = 65535 - rgb[2];

	  lnonzero |= cc || mm || yy;

	  rgb += xstep;
	  xerror += xmod;
	  if (xerror >= d->dst_width)
	    {
	      xerror -= d->dst_width;
	      rgb += 3;
	    }
	}
    }

  *nonzero = lnonzero;
}

static inline void
update_cmy(const dither_t *d, int c, int m, int y, int k,
	    int *nc, int *nm, int *ny)
{
  /*
   * We're not printing black, but let's adjust the CMY levels to
   * produce better reds, greens, and blues...
   *
   * This code needs to be tuned
   */

  unsigned ck = c - k;
  unsigned mk = m - k;
  unsigned yk = y - k;

  *nc  = ((unsigned) (65535 - c / 4)) * ck / 65535 + k;
  *nm  = ((unsigned) (65535 - m / 4)) * mk / 65535 + k;
  *ny  = ((unsigned) (65535 - y / 4)) * yk / 65535 + k;
}

static inline void
update_cmyk(const dither_t *d, int c, int m, int y, int k,
	    int *nc, int *nm, int *ny, int *nk, int *jk)
{
  int ak;
  int kdarkness;
  unsigned ks, kl;
  int ub, lb;
  int ok;
  int bk;

  ub = d->k_upper;    /* Upper bound */
  lb = d->k_lower;    /* Lower bound */

  /*
   * Calculate total ink amount.
   * If there is a lot of ink, black gets added sooner. Saves ink
   * and with a lot of ink the black doesn't show as speckles.
   *
   * k already contains the grey contained in CMY.
   * First we find out if the color is darker than the K amount
   * suggests, and we look up where is value is between
   * lowerbound and density:
   */

  kdarkness = c + c + m + m + y - d->density2;
  if (kdarkness > (k + k + k))
    ok = kdarkness / 3;
  else
    ok = k;
  if ( ok > lb )
    kl = (unsigned) ( ok - lb ) * (unsigned) d->density /
      d->dlb_range;
  else
    kl = 0;
  if (kl > d->density)
    kl = d->density;

  /*
   * We have a second value, ks, that will be the scaler.
   * ks is initially showing where the original black
   * amount is between upper and lower bounds:
   */

  if ( k > ub )
    ks = d->density;
  else if ( k < lb )
    ks = 0;
  else
    ks = (unsigned) (k - lb) * (unsigned) d->density /
      d->bound_range;
  if (ks > d->density)
    ks = d->density;

  /*
   * ks is then processed by a second order function that produces
   * an S curve: 2ks - ks^2. This is then multiplied by the
   * darkness value in kl. If we think this is too complex the
   * following line can be tried instead:
   * ak = ks;
   */
  ak = ks;
  k = (unsigned) kl * (unsigned) ak / (unsigned) d->density;
  if (k > d->density)
    k = d->density;
  ok = k;
  bk = k;

  if (k && ak)
    {
      /*
       * Because black is always fairly neutral, we do not have to
       * calculate the amount to take out of CMY. The result will
       * be a bit dark but that is OK. If things are okay CMY
       * cannot go negative here - unless extra K is added in the
       * previous block. We multiply by ak to prevent taking out
       * too much. This prevents dark areas from becoming very
       * dull.
       */

      ok = (unsigned) k * (unsigned) ak / (unsigned) d->density;
      c -= ok;
      m -= ok;
      y -= ok;

      if (c < 0)
	c = 0;
      if (m < 0)
	m = 0;
      if (y < 0)
	y = 0;
    }
  *nc = c;
  *nm = m;
  *ny = y;
  *nk = bk;
  *jk = k;
}

void
dither_cmyk_fast(const unsigned short  *rgb,	/* I - RGB pixels */
		 int           row,	/* I - Current Y coordinate */
		 void 	    *vd,
		 unsigned char *cyan,	/* O - Cyan bitmap pixels */
		 unsigned char *lcyan,	/* O - Light cyan bitmap pixels */
		 unsigned char *magenta, /* O - Magenta bitmap pixels */
		 unsigned char *lmagenta, /* O - Light magenta bitmap pixels */
		 unsigned char *yellow,	/* O - Yellow bitmap pixels */
		 unsigned char *lyellow, /* O - Light yellow bitmap pixels */
		 unsigned char *black,	/* O - Black bitmap pixels */
		 int	       duplicate_line)
{
  int		x,		/* Current X coordinate */
		length;		/* Length of output bitmap in bytes */
  int		c, m, y, k,	/* CMYK values */
    		oc, om, oy, ok;
  unsigned char	bit,		/* Current bit */
    		*cptr,		/* Current cyan pixel */
    		*mptr,		/* Current magenta pixel */
    		*yptr,		/* Current yellow pixel */
    		*lmptr,		/* Current light magenta pixel */
    		*lcptr,		/* Current light cyan pixel */
    		*lyptr,		/* Current light yellow pixel */
    		*kptr;		/* Current black pixel */
  dither_t	*d = (dither_t *) vd;
  const unsigned short *cline = get_valueline(d, ECOLOR_C);
  const unsigned short *mline = get_valueline(d, ECOLOR_M);
  const unsigned short *yline = get_valueline(d, ECOLOR_Y);
  int nonzero;

  dither_color_t *cd = &(d->c_dither);
  dither_matrix_t *cdither = &(d->c_dithermat);
  dither_color_t *md = &(d->m_dither);
  dither_matrix_t *mdither = &(d->m_dithermat);
  dither_color_t *yd = &(d->y_dither);
  dither_matrix_t *ydither = &(d->y_dithermat);
  dither_color_t *kd = &(d->k_dither);
  dither_matrix_t *kdither = &(d->k_dithermat);
  int dst_width = d->dst_width;
  int cdither_very_fast = 0;
  int mdither_very_fast = 0;
  int ydither_very_fast = 0;
  int kdither_very_fast = 0;

  if (cd->nlevels == 1 && cd->ranges[0].bits_h == 1 && cd->ranges[0].isdark_h)
    cdither_very_fast = 1;
  if (md->nlevels == 1 && md->ranges[0].bits_h == 1 && md->ranges[0].isdark_h)
    mdither_very_fast = 1;
  if (yd->nlevels == 1 && yd->ranges[0].bits_h == 1 && yd->ranges[0].isdark_h)
    ydither_very_fast = 1;
  if (kd->nlevels == 1 && kd->ranges[0].bits_h == 1 && kd->ranges[0].isdark_h)
    kdither_very_fast = 1;

  length = (d->dst_width + 7) / 8;

  memset(cyan, 0, length * d->c_dither.signif_bits);
  if (lcyan)
    memset(lcyan, 0, length * d->c_dither.signif_bits);
  memset(magenta, 0, length * d->m_dither.signif_bits);
  if (lmagenta)
    memset(lmagenta, 0, length * d->m_dither.signif_bits);
  memset(yellow, 0, length * d->y_dither.signif_bits);
  if (lyellow)
    memset(lyellow, 0, length * d->y_dither.signif_bits);
  if (black)
    memset(black, 0, length * d->k_dither.signif_bits);
  /*
   * First, generate the CMYK separation.  If there's nothing in
   * this row, and we're using an ordered dither, there's no reason
   * to do anything at all.
   */
  if (!duplicate_line)
    {
      generate_cmy(d, rgb, &nonzero, row);
      if (nonzero)
	d->last_line_was_empty = 0;
      else
	d->last_line_was_empty++;
    }
  else if (d->last_line_was_empty)
    d->last_line_was_empty++;
  /*
   * First, generate the CMYK separation.  If there's nothing in
   * this row, and we're using an ordered dither, there's no reason
   * to do anything at all.
   */
  if (d->last_line_was_empty)
    return;

  /*
   * Boilerplate
   */

  bit = 128;
  x = 0;
  cptr = cyan;
  mptr = magenta;
  yptr = yellow;
  lcptr = lcyan;
  lmptr = lmagenta;
  lyptr = lyellow;
  kptr = black;

  k = 0;			/* Shut up the compiler */
  ok = 0;
  matrix_set_row(d, &(d->k_dithermat), row);
  matrix_set_row(d, &(d->c_dithermat), row);
  matrix_set_row(d, &(d->m_dithermat), row);
  matrix_set_row(d, &(d->y_dithermat), row);

  /*
   * Main loop starts here!
   */
  QUANT(14);
  for (; x != dst_width; x++)
    {
      /*
       * First get the standard CMYK separation color values.
       */

      c = cline[x];
      m = mline[x];
      y = yline[x];
      oc = c;
      om = m;
      oy = y;

      /*
       * If we're doing ordered dither, and there's no ink, we aren't
       * going to print anything.
       */
      if (c == 0 && m == 0 && y == 0)
	{
	  goto advance;
	}

      if (black)
	{
	  k = USMIN(c, USMIN(m, y));
	  if (k < 32768)
	    k = 0;
	  else
	    k = 65535 - ((65535 - k) * 2);
	  c -= k;
	  m -= k;
	  y -= k;
	  ok = k;
	}
      QUANT(15);

      if (black)
	print_color_fast(d, kd, ok,
			 k, x, row, kptr, NULL, bit, length,
			 kdither, kdither_very_fast);
      print_color_fast(d, kd, oc,
		       c, x, row, cptr, lcptr, bit, length,
		       cdither, cdither_very_fast);
      print_color_fast(d, kd, om,
		       m, x, row, mptr, lmptr, bit, length,
		       mdither, mdither_very_fast);
      print_color_fast(d, kd, oy,
		       y, x, row, yptr, lyptr, bit, length,
		       ydither, ydither_very_fast);
      QUANT(16);

      /*****************************************************************
       * Advance the loop
       *****************************************************************/

    advance:
      if (bit == 1)
	{
	  cptr ++;
	  if (lcptr)
	    lcptr ++;
	  mptr ++;
	  if (lmptr)
	    lmptr ++;
	  yptr ++;
	  if (lyptr)
	    lyptr ++;
	  if (kptr)
	    kptr ++;
	  bit       = 128;
	}
      else
	bit >>= 1;
      QUANT(17);
    }
  /*
   * Main loop ends here!
   */
}

void
dither_cmyk(const unsigned short  *rgb,	/* I - RGB pixels */
	    int           row,	/* I - Current Y coordinate */
	    void 	    *vd,
	    unsigned char *cyan,	/* O - Cyan bitmap pixels */
	    unsigned char *lcyan,	/* O - Light cyan bitmap pixels */
	    unsigned char *magenta,	/* O - Magenta bitmap pixels */
	    unsigned char *lmagenta,	/* O - Light magenta bitmap pixels */
	    unsigned char *yellow,	/* O - Yellow bitmap pixels */
	    unsigned char *lyellow,	/* O - Light yellow bitmap pixels */
	    unsigned char *black,	/* O - Black bitmap pixels */
	    int		  duplicate_line)
{
  int		x,		/* Current X coordinate */
		length;		/* Length of output bitmap in bytes */
  int		c, m, y, k,	/* CMYK values */
		oc, om, oy;
  unsigned char	bit,		/* Current bit */
		*cptr,		/* Current cyan pixel */
		*mptr,		/* Current magenta pixel */
		*yptr,		/* Current yellow pixel */
		*lmptr,		/* Current light magenta pixel */
		*lcptr,		/* Current light cyan pixel */
		*lyptr,		/* Current light yellow pixel */
		*kptr;		/* Current black pixel */
  int		ditherc = 0,	/* Next error value in buffer */
		*cerror0 = 0,	/* Pointer to current error row */
		*cerror1 = 0;	/* Pointer to next error row */
  int		dithery = 0,	/* Next error value in buffer */
		*yerror0 = 0,	/* Pointer to current error row */
		*yerror1 = 0;	/* Pointer to next error row */
  int		ditherm = 0,	/* Next error value in buffer */
		*merror0 = 0,	/* Pointer to current error row */
		*merror1 = 0;	/* Pointer to next error row */
  int		ditherk = 0,	/* Next error value in buffer */
		*kerror0 = 0,	/* Pointer to current error row */
		*kerror1 = 0;	/* Pointer to next error row */
  int		bk = 0;
  dither_t	*d = (dither_t *) vd;
  const unsigned short *cline = get_valueline(d, ECOLOR_C);
  const unsigned short *mline = get_valueline(d, ECOLOR_M);
  const unsigned short *yline = get_valueline(d, ECOLOR_Y);
  int nonzero;

  int		terminate;
  int		direction = row & 1 ? 1 : -1;
  int		odb = d->spread;
  int		odb_mask = (1 << odb) - 1;
  int		first_color = row % 3;
  int		printed_black;
  int		ink_budget;

  if (d->dither_type & D_FAST_BASE)
    {
      dither_cmyk_fast(rgb, row, vd, cyan, lcyan, magenta, lmagenta,
		       yellow, lyellow, black, duplicate_line);
      return;
    }

  length = (d->dst_width + 7) / 8;

  memset(cyan, 0, length * d->c_dither.signif_bits);
  if (lcyan)
    memset(lcyan, 0, length * d->c_dither.signif_bits);
  memset(magenta, 0, length * d->m_dither.signif_bits);
  if (lmagenta)
    memset(lmagenta, 0, length * d->m_dither.signif_bits);
  memset(yellow, 0, length * d->y_dither.signif_bits);
  if (lyellow)
    memset(lyellow, 0, length * d->y_dither.signif_bits);
  if (black)
    memset(black, 0, length * d->k_dither.signif_bits);
  /*
   * First, generate the CMYK separation.  If there's nothing in
   * this row, and we're using an ordered dither, there's no reason
   * to do anything at all.
   */
  if (!duplicate_line)
    {
      generate_cmy(d, rgb, &nonzero, row);
      if (nonzero)
	d->last_line_was_empty = 0;
      else
	d->last_line_was_empty++;
    }
  else if (d->last_line_was_empty)
    d->last_line_was_empty++;
  if ((d->last_line_was_empty && (d->dither_type & D_ORDERED_BASE)) ||
      d->last_line_was_empty >= 5)
    return;

  /*
   * Boilerplate
   */

  if (d->dither_type & D_ORDERED_BASE)
    direction = 1;

  bit = (direction == 1) ? 128 : 1 << (7 - ((d->dst_width - 1) & 7));
  x = (direction == 1) ? 0 : d->dst_width - 1;
  terminate = (direction == 1) ? d->dst_width : -1;

  if (! (d->dither_type & D_ORDERED_BASE))
    {
      cerror0 = get_errline(d, row, ECOLOR_C);
      cerror1 = get_errline(d, row + 1, ECOLOR_C);

      merror0 = get_errline(d, row, ECOLOR_M);
      merror1 = get_errline(d, row + 1, ECOLOR_M);

      yerror0 = get_errline(d, row, ECOLOR_Y);
      yerror1 = get_errline(d, row + 1, ECOLOR_Y);

      kerror0 = get_errline(d, row, ECOLOR_K);
      kerror1 = get_errline(d, row + 1, ECOLOR_K);
      memset(kerror1, 0, d->dst_width * sizeof(int));
      memset(cerror1, 0, d->dst_width * sizeof(int));
      memset(merror1, 0, d->dst_width * sizeof(int));
      memset(yerror1, 0, d->dst_width * sizeof(int));
      if (d->last_line_was_empty >= 4)
	{
	  if (d->last_line_was_empty == 4)
	    {
	      memset(kerror0, 0, d->dst_width * sizeof(int));
	      memset(cerror0, 0, d->dst_width * sizeof(int));
	      memset(merror0, 0, d->dst_width * sizeof(int));
	      memset(yerror0, 0, d->dst_width * sizeof(int));
	    }
	  return;
	}
    }
  cptr = cyan;
  mptr = magenta;
  yptr = yellow;
  lcptr = lcyan;
  lmptr = lmagenta;
  lyptr = lyellow;
  kptr = black;
  if (direction == -1)
    {
      if (! (d->dither_type & D_ORDERED_BASE))
	{
	  cerror0 += d->dst_width - 1;
	  cerror1 += d->dst_width - 1;
	  merror0 += d->dst_width - 1;
	  merror1 += d->dst_width - 1;
	  yerror0 += d->dst_width - 1;
	  yerror1 += d->dst_width - 1;
	  kerror0 += d->dst_width - 1;
	  kerror1 += d->dst_width - 1;
	}
      cptr = cyan + length - 1;
      if (lcptr)
	lcptr = lcyan + length - 1;
      mptr = magenta + length - 1;
      if (lmptr)
	lmptr = lmagenta + length - 1;
      yptr = yellow + length - 1;
      if (lyptr)
	lyptr = lyellow + length - 1;
      if (kptr)
	kptr = black + length - 1;
      first_color = (first_color + d->dst_width - 1) % 3;
    }

  if (! (d->dither_type & D_ORDERED_BASE))
    {
      ditherc = cerror0[0];
      ditherm = merror0[0];
      dithery = yerror0[0];
      ditherk = kerror0[0];
    }
  matrix_set_row(d, &(d->k_dithermat), row);
  matrix_set_row(d, &(d->c_dithermat), row);
  matrix_set_row(d, &(d->m_dithermat), row);
  matrix_set_row(d, &(d->y_dithermat), row);
  matrix_set_row(d, &(d->k_pick), row);
  matrix_set_row(d, &(d->c_pick), row);
  matrix_set_row(d, &(d->m_pick), row);
  matrix_set_row(d, &(d->y_pick), row);
   QUANT(6);
  /*
   * Main loop starts here!
   */
  for (; x != terminate; x += direction)
    {
      /*
       * First get the standard CMYK separation color values.
       */

      c = cline[x];
      m = mline[x];
      y = yline[x];
      oc = c;
      om = m;
      oy = y;

      /*
       * If we're doing ordered dither, and there's no ink, we aren't
       * going to print anything.
       */
      if (c == 0 && m == 0 && y == 0)
	{
	  if (d->dither_type & D_ORDERED_BASE)
	    goto advance;
	  else
	    {
	      c = UPDATE_COLOR(c, ditherc);
	      m = UPDATE_COLOR(m, ditherm);
	      y = UPDATE_COLOR(y, dithery);
	      goto out;
	    }
	}

      QUANT(7);

      k = USMIN(c, USMIN(m, y));

      /*
       * At this point we've computed the basic CMYK separations.
       * Now we adjust the levels of each to improve the print quality.
       */

      if (k > 0)
	{
	  if (black != NULL)
	    update_cmyk(d, oc, om, oy, k, &c, &m, &y, &bk, &k);
	  else
	    update_cmy(d, oc, om, oy, k, &c, &m, &y);
	}

      QUANT(8);
      /*
       * We've done all of the cmyk separations at this point.
       * Now to do the dithering.
       *
       * At this point:
       *
       * bk = Amount of black printed with black ink
       * ak = Adjusted "raw" K value
       * k = raw K value derived from CMY
       * oc, om, oy = raw CMY values assuming no K component
       * c, m, y = CMY values adjusted for the presence of K
       *
       * The main reason for this rather elaborate setup, where we have
       * 8 channels at this point, is to handle variable intensities
       * (in particular light and dark variants) of inks.  Very dark regions
       * with slight color tints should be printed with dark inks, not with
       * the light inks that would be implied by the small amount of remnant
       * CMY.
       *
       * It's quite likely that for simple four-color printers ordinary
       * CMYK separations would work.  It's possible that they would work
       * for variable dot sizes, too.
       */

      if (!(d->dither_type & D_ORDERED_BASE))
	{
	  c = UPDATE_COLOR(c, ditherc);
	  m = UPDATE_COLOR(m, ditherm);
	  y = UPDATE_COLOR(y, dithery);
	}

      QUANT(9);

      ink_budget = d->ink_limit;

      if (black)
	{
	  int tk = print_color(d, &(d->k_dither), bk, bk, k, x, row, kptr,
			       NULL, bit, length, 0, 0, &ink_budget,
			       &(d->k_pick), &(d->k_dithermat), D_ORDERED);
	  printed_black = k - tk;
	  k = tk;
	}
      else
        printed_black = 0;

      QUANT(10);
      /*
       * If the printed density is high, ink reduction loses too much
       * ink.  However, at low densities it seems to be safe.  Of course,
       * at low densities it won't do as much.
       */
      if (d->density > 45000)
	printed_black = 0;

      /*
       * Uh oh spaghetti-o!
       *
       * It has been determined experimentally that inlining print_color
       * saves a substantial amount of time.  However, expanding this out
       * as a switch drastically increases the code volume by about 10 KB.
       * The solution for now (until we do this properly, via an array)
       * is to use this ugly code.
       */

      if (first_color == ECOLOR_M)
	goto ecm;
      else if (first_color == ECOLOR_Y)
	goto ecy;
    ecc:
      c = print_color(d, &(d->c_dither), oc, oc,
		      c, x, row, cptr, lcptr, bit, length,
		      d->c_randomizer, printed_black, &ink_budget,
		      &(d->c_pick), &(d->c_dithermat), d->dither_type);
      if (first_color == ECOLOR_M)
	goto out;
    ecm:
      m = print_color(d, &(d->m_dither), om, om,
		      m, x, row, mptr, lmptr, bit, length,
		      d->m_randomizer, printed_black, &ink_budget,
		      &(d->m_pick), &(d->m_dithermat), d->dither_type);
      if (first_color == ECOLOR_Y)
	goto out;
    ecy:
      y = print_color(d, &(d->y_dither), oy, oy,
		      y, x, row, yptr, lyptr, bit, length,
		      d->y_randomizer, printed_black, &ink_budget,
		      &(d->y_pick), &(d->y_dithermat), d->dither_type);
      if (first_color != ECOLOR_C)
	goto ecc;
    out:

      QUANT(11);
      if (!(d->dither_type & D_ORDERED_BASE))
	{
	  ditherc = update_dither(c, oc, d->src_width, odb, odb_mask,
				  direction, cerror0, cerror1, d);
	  ditherm = update_dither(m, om, d->src_width, odb, odb_mask,
				  direction, merror0, merror1, d);
	  dithery = update_dither(y, oy, d->src_width, odb, odb_mask,
				  direction, yerror0, yerror1, d);
	}

      /*****************************************************************
       * Advance the loop
       *****************************************************************/

      QUANT(12);
    advance:
      if (direction == 1)
	{
	  if (bit == 1)
	    {
	      cptr ++;
	      if (lcptr)
		lcptr ++;
	      mptr ++;
	      if (lmptr)
		lmptr ++;
	      yptr ++;
	      if (lyptr)
		lyptr ++;
	      if (kptr)
		kptr ++;
	      bit       = 128;
	    }
	  else
	    bit >>= 1;
	  first_color++;
	  if (first_color >= 3)
	    first_color = 0;
	}
      else
	{
	  if (bit == 128)
	    {
	      cptr --;
	      if (lcptr)
		lcptr --;
	      mptr --;
	      if (lmptr)
		lmptr --;
	      yptr --;
	      if (lyptr)
		lyptr --;
	      if (kptr)
		kptr --;
	      bit       = 1;
	    }
	  else
	    bit <<= 1;
	  first_color--;
	  if (first_color <= 0)
	    first_color = 2;
	}
      if (!(d->dither_type & D_ORDERED_BASE))
	{
	  cerror0 += direction;
	  cerror1 += direction;
	  merror0 += direction;
	  merror1 += direction;
	  yerror0 += direction;
	  yerror1 += direction;
	  kerror0 += direction;
	  kerror1 += direction;
	}
      QUANT(13);
  }
  /*
   * Main loop ends here!
   */
}
