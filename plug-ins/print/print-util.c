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
 * Contents:
 *
 *   dither_black()       - Dither grayscale pixels to black.
 *   dither_cmyk()        - Dither RGB pixels to cyan, magenta, yellow, and
 *                          black.
 *   dither_black4()      - Dither grayscale pixels to 4 levels of black.
 *   dither_cmyk4()       - Dither RGB pixels to 4 levels of cyan, magenta,
 *                          yellow, and black.
 *   gray_to_gray()       - Convert grayscale image data to grayscale.
 *   indexed_to_gray()    - Convert indexed image data to grayscale.
 *   indexed_to_rgb()     - Convert indexed image data to RGB.
 *   rgb_to_gray()        - Convert RGB image data to grayscale.
 *   rgb_to_rgb()         - Convert RGB image data to RGB.
 *   default_media_size() - Return the size of a default page size.
 *
 * Revision History:
 *
 *  See ChangeLog
 */

/* #define PRINT_DEBUG */


#include "print.h"
#include <libgimp/gimp.h>
#include "libgimp/gimpcolorspace.h"
#include <math.h>

/*
 * RGB to grayscale luminance constants...
 */

#define LUM_RED		31
#define LUM_GREEN	61
#define LUM_BLUE	8


/*
 * Error buffer for dither functions.  This needs to be at least 14xMAXDPI
 * (currently 720) to avoid problems...
 *
 * Want to dynamically allocate this so we can save memory!
 */

#define ERROR_ROWS 2

typedef union error
{
  struct
  {
    int c[ERROR_ROWS];
    int m[ERROR_ROWS];
    int y[ERROR_ROWS];
    int k[ERROR_ROWS];
  } c;
  int v[4][ERROR_ROWS];
} error_t;

error_t *nerror = 0;

int	error[2][4][14*2880+1];

/*
 * 'dither_black()' - Dither grayscale pixels to black.
 */

void
dither_black(unsigned short     *gray,		/* I - Grayscale pixels */
	     int           	row,		/* I - Current Y coordinate */
	     int           	src_width,	/* I - Width of input row */
	     int           	dst_width,	/* I - Width of output row */
	     unsigned char 	*black)		/* O - Black bitmap pixels */
{
  int		x,		/* Current X coordinate */
		xerror,		/* X error count */
		xstep,		/* X step */
		xmod,		/* X error modulus */
		length;		/* Length of output bitmap in bytes */
  unsigned char	bit,		/* Current bit */
		*kptr;		/* Current black pixel */
  int		k,		/* Current black error */
		ditherk,	/* Next error value in buffer */
		*kerror0,	/* Pointer to current error row */
		*kerror1;	/* Pointer to next error row */
  int		ditherbit;	/* Random dithering bitmask */


  xstep  = src_width / dst_width;
  xmod   = src_width % dst_width;
  length = (dst_width + 7) / 8;

  kerror0 = error[row & 1][3];
  kerror1 = error[1 - (row & 1)][3];

  memset(black, 0, length);

  for (x = 0, bit = 128, kptr = black, xerror = 0,
           ditherbit = rand(), ditherk = *kerror0;
       x < dst_width;
       x ++, kerror0 ++, kerror1 ++)
  {
    k = 65535 - *gray + ditherk / 8;

    if (k > 32767)
    {
      *kptr |= bit;
      k -= 65535;
    }

    if (ditherbit & bit)
    {
      kerror1[0] = 5 * k;
      ditherk    = kerror0[1] + 3 * k;
    }
    else
    {
      kerror1[0] = 3 * k;
      ditherk    = kerror0[1] + 5 * k;
    }

    if (bit == 1)
    {
      kptr ++;
      bit       = 128;
      ditherbit = rand();
    }
    else
      bit >>= 1;

    gray   += xstep;
    xerror += xmod;
    if (xerror >= dst_width)
    {
      xerror -= dst_width;
      gray   ++;
    }
  }
}

/*
 * 'dither_cmyk6()' - Dither RGB pixels to cyan, magenta, light cyan,
 * light magenta, yellow, and black.
 *
 * Added by Robert Krawitz <rlk@alum.mit.edu> August 30, 1999.
 *
 * Let's be really aggressive and use a single routine for ALL cmyk dithering,
 * including 6 and 7 color.
 *
 * Note that this is heavily tuned for Epson Stylus Photo printers.
 * This should be generalized for other CMYK and CcMmYK printers.  All
 * of these constants were empirically determined, and are subject to review.
 */

/*
 * Ratios of dark to light inks.  The darker ink should be DE / NU darker
 * than the light ink.
 *
 * It is essential to be very careful about use of parentheses with these
 * macros!
 *
 * Increasing the denominators results in use of more dark ink.  This creates
 * more saturated colors.
 */

#define NU_C 1
#define DE_C 1
#define NU_M 1
#define DE_M 1
#define NU_Y 1
#define DE_Y 1

#define I_RATIO_C NU_C / DE_C
#define I_RATIO_C1 NU_C / (DE_C + NU_C)
#define RATIO_C DE_C / NU_C
#define RATIO_C1 (DE_C + NU_C) / NU_C

const static int C_CONST_0 = 65536 * I_RATIO_C1;
const static int C_CONST_1 = 65536 * I_RATIO_C1;

#define I_RATIO_M NU_M / DE_M
#define I_RATIO_M1 NU_M / (DE_M + NU_M)
#define RATIO_M DE_M / NU_M
#define RATIO_M1 (DE_M + NU_M) / NU_M

const static int M_CONST_0 = 65536 * I_RATIO_M1;
const static int M_CONST_1 = 65536 * I_RATIO_M1;

#define I_RATIO_Y NU_Y / DE_Y
#define I_RATIO_Y1 NU_Y / (DE_Y + NU_Y)
#define RATIO_Y DE_Y / NU_Y
#define RATIO_Y1 (DE_Y + NU_Y) / NU_Y

const static int Y_CONST_0 = 65536 * I_RATIO_Y1;
const static int Y_CONST_1 = 65536 * I_RATIO_Y1;

/*
 * Lower and upper bounds for mixing CMY with K to produce gray scale.
 * Reducing KDARKNESS_LOWER results in more black being used with relatively
 * light grays, which causes speckling.  Increasing KDARKNESS_UPPER results
 * in more CMY being used in dark tones, which results in less pure black.
 * Decreasing the gap too much results in sharp crossover and stairstepping.
 */
#define KDARKNESS_LOWER (12 * 256)
#define KDARKNESS_UPPER (128 * 256)

/*
 * Randomizing values for deciding when to output a bit.  Normally with the
 * error diffusion algorithm a bit is not output until the accumulated value
 * of the pixel crosses a threshold.  This randomizes the threshold, which
 * results in fewer obnoxious diagonal jaggies in pale regions.  Smaller values
 * result in greater randomizing.  We use less randomness for black output
 * to avoid production of black speckles in light regions.
 */
#define C_RANDOMIZER 0
#define M_RANDOMIZER 0
#define Y_RANDOMIZER 0
#define K_RANDOMIZER 4

#ifdef PRINT_DEBUG
#define UPDATE_COLOR_DBG(r)			\
do {						\
  od##r = dither##r;				\
  d##r = r;					\
} while (0)

#define PRINT_D1(r, R, d1, d2)						\
do {									\
  fprintf(dbg, "Case 4: o" #r " %lld " #r				\
	  " %lld ditherbit" #d1 " %d ditherbit" #d2 " %d "		\
	  "num %lld den %lld test1 %lld test2 %lld\n",			\
	  o##r, r, ditherbit##d1, ditherbit##d2,			\
	  o##r, 65536ll,						\
	  ((32767 + (((long long) ditherbit##d2 / 1) - 32768)) * o##r	\
	   / 65536),							\
	  ((o##r - (65536 * I_RATIO_##R##1 * 3 / 4)) * 65536 /		\
	   (65536 - (65536 * I_RATIO_##R##1 * 3 / 4))));		\
} while (0)

#define PRINT_D2(r, R, d1, d2)						\
do {									\
  fprintf(dbg, "Case 1: o" #r " %lld " #r " %lld test %lld\n", o##r, r,	\
	  (32767 + (((long long) ditherbit##d2 / 1) - 32768)) *		\
	  I_RATIO_##R##1);						\
} while (0)

#define PRINT_D3(n, r, R, d1, d2)					\
do {									\
  fprintf(dbg, "Case %d: o" #r " %lld " #r				\
	  " %lld ditherbit" #d1 " %d ditherbit" #d2 " %d "		\
	  "num %lld den %lld test1 %lld test2 %lld\n", n,		\
	  o##r, r, ditherbit##d1, ditherbit##d2,			\
	  o##r, 65536ll,						\
	  ((32767 + (((long long) ditherbit##d2 / 1) - 32768)) * o##r /	\
	   65536), cutoff);						\
} while (0)

#else /* !PRINT_DEBUG */

#define UPDATE_COLOR_DBG(r) do {} while (0)
#define PRINT_D1(r, R, d1, d2) do {} while (0)
#define PRINT_D2(r, R, d1, d2) do {} while (0)
#define PRINT_D3(n, r, R, d1, d2) do {} while (0)

#endif

#define UPDATE_COLOR(r)				\
do {						\
  o##r = r;					\
  r += dither##r / 8;				\
  UPDATE_COLOR_DBG(r);				\
} while (0)

#define DO_PRINT_COLOR(color)			\
do {						\
  if (color##bits++ == horizontal_overdensity)	\
    {						\
      *color##ptr |= bit;			\
      color##bits = 1;				\
    }						\
} while(0)

#define PRINT_COLOR(color, r, R, d1, d2)				    \
do {									    \
  int comp0 = (32767 + ((ditherbit##d2 >> R##_RANDOMIZER) -		    \
			(32768 >> R##_RANDOMIZER)));			    \
  if (!l##color)							    \
    {									    \
      if (r > comp0)							    \
	{								    \
	  PRINT_D1(r, R, d1, d2);					    \
	  DO_PRINT_COLOR(r);						    \
	  r -= 65535;							    \
	}								    \
    }									    \
  else									    \
    {									    \
      int compare = comp0 * I_RATIO_##R##1;				    \
      if (r <= (R##_CONST_1))						    \
	{								    \
	  if (r > compare)						    \
	    {								    \
	      PRINT_D2(r, R, d1, d2);					    \
	      DO_PRINT_COLOR(l##r);					    \
	      r -= R##_CONST_0;						    \
	    }								    \
	}								    \
      else if (r > compare)						    \
	{								    \
	  int cutoff = ((density - R##_CONST_1) * 65536 /		    \
			(65536 - R##_CONST_1));				    \
	  long long sub;						    \
	  if (cutoff >= 0)						    \
	    sub = R##_CONST_0 + (((65535ll - R##_CONST_0) * cutoff) >> 16); \
	  else								    \
	    sub = R##_CONST_0 + ((65535ll - R##_CONST_0) * cutoff / 65536); \
	  if (ditherbit##d1 > cutoff)					    \
	    {								    \
	      PRINT_D3(3, r, R, d1, d2);				    \
	      DO_PRINT_COLOR(l##r);					    \
	    }								    \
	  else								    \
	    {								    \
	      PRINT_D3(4, r, R, d1, d2);				    \
	      DO_PRINT_COLOR(r);					    \
	    }								    \
	  if (sub < R##_CONST_0)					    \
	    r -= R##_CONST_0;						    \
	  else if (sub > 65535)						    \
	    r -= 65535;							    \
	  else								    \
	    r -= sub;							    \
	}								    \
    }									    \
} while (0)

#if 1
#define UPDATE_DITHER(r, d2, x, width)					 \
do {									 \
  int offset = (15 - (((o##r & 0xf000) >> 12)) * horizontal_overdensity) \
				       >> 1;				 \
  if (x < offset)							 \
    offset = x;								 \
  else if (x > dst_width - offset - 1)					 \
    offset = dst_width - x - 1;						 \
  if (ditherbit##d2 & bit)						 \
    {									 \
      r##error1[-offset] += r;						 \
      r##error1[0] += 3 * r;						 \
      r##error1[offset] += r;						 \
      dither##r    = r##error0[direction] + 3 * r;			 \
    }									 \
  else									 \
    {									 \
      r##error1[-offset] += r;						 \
      r##error1[0] +=  r;						 \
      r##error1[offset] += r;						 \
      dither##r    = r##error0[direction] + 5 * r;			 \
    }									 \
} while (0)
#else
#define UPDATE_DITHER(r, d2, x, width)			\
do {							\
  if (ditherbit##d2 & bit)				\
    {							\
      r##error1[0] = 5 * r;				\
      dither##r    = r##error0[direction] + 3 * r;	\
    }							\
  else							\
    {							\
      r##error1[0] = 3 * r;				\
      dither##r    = r##error0[direction] + 5 * r;	\
    }							\
} while (0)
#endif

void
dither_cmyk(unsigned short  *rgb,	/* I - RGB pixels */
	    int           row,		/* I - Current Y coordinate */
	    int           src_width,	/* I - Width of input row */
	    int           dst_width,	/* I - Width of output rows */
	    unsigned char *cyan,	/* O - Cyan bitmap pixels */
	    unsigned char *lcyan,	/* O - Light cyan bitmap pixels */
	    unsigned char *magenta,	/* O - Magenta bitmap pixels */
	    unsigned char *lmagenta,	/* O - Light magenta bitmap pixels */
	    unsigned char *yellow,	/* O - Yellow bitmap pixels */
	    unsigned char *lyellow,	/* O - Light yellow bitmap pixels */
	    unsigned char *black,	/* O - Black bitmap pixels */
	    int horizontal_overdensity)
{
  int		x,		/* Current X coordinate */
		xerror,		/* X error count */
		xstep,		/* X step */
		xmod,		/* X error modulus */
		length;		/* Length of output bitmap in bytes */
  long long	c, m, y, k,	/* CMYK values */
		oc, om, ok, oy,
		divk;		/* Inverse of K */
  long long     diff;		/* Average color difference */
  unsigned char	bit,		/* Current bit */
		*cptr,		/* Current cyan pixel */
		*mptr,		/* Current magenta pixel */
		*yptr,		/* Current yellow pixel */
		*lmptr,		/* Current light magenta pixel */
		*lcptr,		/* Current light cyan pixel */
		*lyptr,		/* Current light yellow pixel */
		*kptr;		/* Current black pixel */
  int		ditherc,	/* Next error value in buffer */
		*cerror0,	/* Pointer to current error row */
		*cerror1;	/* Pointer to next error row */
  int		dithery,	/* Next error value in buffer */
		*yerror0,	/* Pointer to current error row */
		*yerror1;	/* Pointer to next error row */
  int		ditherm,	/* Next error value in buffer */
		*merror0,	/* Pointer to current error row */
		*merror1;	/* Pointer to next error row */
  int		ditherk,	/* Next error value in buffer */
		*kerror0,	/* Pointer to current error row */
		*kerror1;	/* Pointer to next error row */
  int		ditherbit;	/* Random dither bitmask */
  int nk;
  int ck;
  int bk;
  int ub, lb;
  int ditherbit0, ditherbit1, ditherbit2, ditherbit3;
  long long	density;

  /*
   * If horizontal_overdensity is > 1, we want to output a bit only so many
   * times that a bit would be generated.  These serve as counters for making
   * that decision.  We make these variable static rather than reinitializing
   * at zero each line to avoid having a line of bits near the edge of the
   * image.
   */
  static int cbits = 1;
  static int mbits = 1;
  static int ybits = 1;
  static int kbits = 1;
  static int lcbits = 1;
  static int lmbits = 1;
  static int lybits = 1;
  int overdensity_bits = 0;

#ifdef PRINT_DEBUG
  long long odk, odc, odm, ody, dk, dc, dm, dy, xk, xc, xm, xy, yc, ym, yy;
  FILE *dbg;
#endif

  int terminate;
  int direction = row & 1 ? 1 : -1;

  switch (horizontal_overdensity)
    {
    case 0:
    case 1:
      overdensity_bits = 0;
      break;
    case 2:
      overdensity_bits = 1;
      break;
    case 4:
      overdensity_bits = 2;
      break;
    case 8:
      overdensity_bits = 3;
      break;
    }

  bit = (direction == 1) ? 128 : 1 << (7 - ((dst_width - 1) & 7));
  x = (direction == 1) ? 0 : dst_width - 1;
  terminate = (direction == 1) ? dst_width : -1;

  xstep  = 3 * (src_width / dst_width);
  xmod   = src_width % dst_width;
  length = (dst_width + 7) / 8;

  cerror0 = error[row & 1][0];
  cerror1 = error[1 - (row & 1)][0];

  merror0 = error[row & 1][1];
  merror1 = error[1 - (row & 1)][1];

  yerror0 = error[row & 1][2];
  yerror1 = error[1 - (row & 1)][2];

  kerror0 = error[row & 1][3];
  kerror1 = error[1 - (row & 1)][3];
  memset(kerror1, 0, dst_width * sizeof(int));
  memset(cerror1, 0, dst_width * sizeof(int));
  memset(merror1, 0, dst_width * sizeof(int));
  memset(yerror1, 0, dst_width * sizeof(int));
  cptr = cyan;
  mptr = magenta;
  yptr = yellow;
  lcptr = lcyan;
  lmptr = lmagenta;
  lyptr = lyellow;
  kptr = black;
  xerror = 0;
  if (direction == -1)
    {
      cerror0 += dst_width - 1;
      cerror1 += dst_width - 1;
      merror0 += dst_width - 1;
      merror1 += dst_width - 1;
      yerror0 += dst_width - 1;
      yerror1 += dst_width - 1;
      kerror0 += dst_width - 1;
      kerror1 += dst_width - 1;
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
      xstep = -xstep;
      rgb += 3 * (src_width - 1);
      xerror = ((dst_width - 1) * xmod) % dst_width;
      xmod = -xmod;
    }

  memset(cyan, 0, length);
  if (lcyan)
    memset(lcyan, 0, length);
  memset(magenta, 0, length);
  if (lmagenta)
    memset(lmagenta, 0, length);
  memset(yellow, 0, length);
  if (lyellow)
    memset(lyellow, 0, length);
  if (black)
    memset(black, 0, length);

#ifdef PRINT_DEBUG
  dbg = fopen("/mnt1/dbg", "a");
#endif

  /*
   * Main loop starts here!
   */
  for (ditherbit = rand(),
	 ditherc = cerror0[0], ditherm = merror0[0], dithery = yerror0[0],
	 ditherk = kerror0[0],
	 ditherbit0 = ditherbit & 0xffff,
	 ditherbit1 = ((ditherbit >> 8) & 0xffff),
	 ditherbit2 = (((ditherbit >> 16) & 0x7fff) +
		       ((ditherbit & 0x100) << 7)),
	 ditherbit3 = (((ditherbit >> 24) & 0x7f) + ((ditherbit & 1) << 7) +
		       ((ditherbit >> 8) & 0xff00));
       x != terminate;
       x += direction,
	 cerror0 += direction,
	 cerror1 += direction,
	 merror0 += direction,
	 merror1 += direction,
	 yerror0 += direction,
	 yerror1 += direction,
	 kerror0 += direction,
	 kerror1 += direction)
  {

   /*
    * First compute the standard CMYK separation color values...
    */
		   
    int maxlevel;
    int ak;
    int kdarkness;

    c = 65535 - (unsigned) rgb[0];
    m = 65535 - (unsigned) rgb[1];
    y = 65535 - (unsigned) rgb[2];
    oc = c;
    om = m;
    oy = y;
    k = MIN(c, MIN(m, y));
#ifdef PRINT_DEBUG
    xc = c;
    xm = m;
    xy = y;
    xk = k;
    yc = c;
    ym = m;
    yy = y;
#endif
    maxlevel = MAX(c, MAX(m, y));

    if (black != NULL)
    {
     /*
      * Since we're printing black, adjust the black level based upon
      * the amount of color in the pixel (colorful pixels get less black)...
      */
      int xdiff = (abs(c - m) + abs(c - y) + abs(m - y)) / 3;

      diff = 65536 - xdiff;
      diff = (diff * diff * diff) >> 32; /* diff = diff^3 */
      diff--;
      if (diff < 0)
	diff = 0;
      k    = (diff * k) >> 16;
      ak = k;
      divk = 65535 - k;
      if (divk == 0)
        c = m = y = 0;	/* Grayscale */
      else
      {
       /*
        * Full color; update the CMY values for the black value and reduce
        * CMY as necessary to give better blues, greens, and reds... :)
        */

        c  = (65535 - ((rgb[2] + rgb[1]) >> 3)) * (c - k) / divk;
        m  = (65535 - ((rgb[1] + rgb[0]) >> 3)) * (m - k) / divk;
        y  = (65535 - ((rgb[0] + rgb[2]) >> 3)) * (y - k) / divk;
      }
#ifdef PRINT_DEBUG
      yc = c;
      ym = m;
      yy = y;
#endif

      /*
       * kdarkness is an artificially computed darkness value for deciding
       * how much black vs. CMY to use for the k component.  This is
       * empirically determined.
       */
      ok = k;
      nk = k + (ditherk) / 8;
      kdarkness = MAX((c + ((c + c + c) >> 3) + m +
		       ((y + y + y + y + y) >> 3)) >> 2, ak);
      if (kdarkness < KDARKNESS_UPPER)
	{
	  int rb;
	  ub = KDARKNESS_UPPER;
	  lb = KDARKNESS_LOWER;
	  rb = ub - lb;
#ifdef PRINT_DEBUG
	  fprintf(dbg, "Black: kd %d ub %d lb %d rb %d test %d range %d\n",
		  kdarkness, ub, lb, rb, ditherbit % rb, kdarkness - lb);
#endif
	  if (kdarkness <= lb)
	    {
	      bk = 0;
	      ub = 0;
	      lb = 1;
	    }
	  else if (kdarkness < ub)
	    {
	      if (rb == 0 || (ditherbit % rb) < (kdarkness - lb))
		bk = nk;
	      else
		bk = 0;
	    }
	  else
	    {
	      ub = 1;
	      lb = 1;
	      bk = nk;
	    }
	}
      else
	{
#ifdef PRINT_DEBUG
	  fprintf(dbg, "Black real\n");
#endif
	  bk = nk;
	}
      ck = nk - bk;
    
      /*
       * These constants are empirically determined to produce a CMY value
       * that looks reasonably gray and is reasonably well balanced tonally
       * with black.  As usual, this is very ad hoc and needs to be
       * generalized.
       */
      if (lmagenta)
	{
	  int addon = 2 * ck;
	  c += addon;
	  m += addon;
	  y += addon;
	}
      else
	{
	  c += ck;
	  m += ck;
	  y += ck;
	}
      /*
       * Don't allow cmy to grow without bound.
       */
      if (c > 65535)
	c = 65535;
      if (m > 65535)
	m = 65535;
      if (y > 65535)
	y = 65535;
      k = bk;
#ifdef PRINT_DEBUG
      odk = ditherk;
      dk = k;
#endif
      if (k > (32767 + ((ditherbit0 >> K_RANDOMIZER) -
			(32768 >> K_RANDOMIZER))))
	{
	  DO_PRINT_COLOR(k);
	  k -= 65535;
	}

      UPDATE_DITHER(k, 1, x, src_width);
    }
    else
    {
     /*
      * We're not printing black, but let's adjust the CMY levels to produce
      * better reds, greens, and blues...
      */

      ok = 0;
      c  = (65535 - rgb[1] / 4) * (c - k) / 65535 + k;
      m  = (65535 - rgb[2] / 4) * (m - k) / 65535 + k;
      y  = (65535 - rgb[0] / 4) * (y - k) / 65535 + k;
    }

    density = (c + m + y) >> overdensity_bits;
    UPDATE_COLOR(c);
    UPDATE_COLOR(m);
    UPDATE_COLOR(y);
    density += (c + m + y) >> overdensity_bits;
/*     density >>= 1; */

    if (!kptr || !(*kptr & bit))
      {
	PRINT_COLOR(cyan, c, C, 1, 2);
	PRINT_COLOR(magenta, m, M, 2, 3);
	PRINT_COLOR(yellow, y, Y, 3, 0);
      }

    UPDATE_DITHER(c, 2, x, dst_width);
    UPDATE_DITHER(m, 3, x, dst_width);
    UPDATE_DITHER(y, 0, x, dst_width);

    /*****************************************************************
     * Advance the loop
     *****************************************************************/
#ifdef PRINT_DEBUG
    fprintf(dbg, "   x %d y %d  r %d g %d b %d  xc %lld xm %lld xy %lld yc "
	    "%lld ym %lld yy %lld xk %lld  diff %lld divk %lld  oc %lld om "
	    "%lld oy %lld ok %lld  c %lld m %lld y %lld k %lld  %c%c%c%c%c%c%c"
	    "  dk %lld dc %lld dm %lld dy %lld  kd %d ck %d bk %d nk %d ub %d "
	    "lb %d\n",
	    x, row,
	    rgb[0], rgb[1], rgb[2],
	    xc, xm, xy, yc, ym, yy, xk, diff, divk,
	    oc, om, oy, ok,
	    dc, dm, dy, dk,
	    (*cptr & bit) ? 'c' : ' ',
	    (lcyan && (*lcptr & bit)) ? 'C' : ' ',
	    (*mptr & bit) ? 'm' : ' ',
	    (lmagenta && (*lmptr & bit)) ? 'M' : ' ',
	    (*yptr & bit) ? 'y' : ' ',
	    (lyellow && (*lyptr & bit)) ? 'Y' : ' ',
	    (black && (*kptr & bit)) ? 'k' : ' ',
	    odk, odc, odm, ody,
	    kdarkness, ck, bk, nk, ub, lb);
    fprintf(dbg, "x %d dir %d c %x %x m %x %x y %x %x k %x %x rgb %x bit %x\n",
	    x, direction, cptr, cyan, mptr, magenta, yptr, yellow, kptr, black,
	    rgb, bit);
#endif

    ditherbit = rand();
    ditherbit0 = ditherbit & 0xffff;
    ditherbit1 = ((ditherbit >> 8) & 0xffff);
    ditherbit2 = ((ditherbit >> 16) & 0x7fff) + ((ditherbit & 0x100) << 7);
    ditherbit3 = ((ditherbit >> 24) & 0x7f) + ((ditherbit & 1) << 7) +
      ((ditherbit >> 8) & 0xff00);
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
      }

    rgb    += xstep;
    xerror += xmod;
    if (xerror >= dst_width)
      {
	xerror -= dst_width;
	rgb    += 3 * direction;
      }
    else if (xerror < 0)
      {
	xerror += dst_width;
	rgb    += 3 * direction;
      }      
  }
  /*
   * Main loop ends here!
   */
#ifdef PRINT_DEBUG
  fprintf(dbg, "\n");
  fclose(dbg);
#endif
}


/*
 * Constants for 4-level dithering functions...
 * NOTE that these constants are HP-specific!
 */

#define LEVEL_3	65535
#define LEVEL_2	(213 * 65536)
#define LEVEL_1	32767
#define LEVEL_0	0


/*
 * 'dither_black4()' - Dither grayscale pixels to 4 levels of black.
 */

void
dither_black4(unsigned short    *gray,		/* I - Grayscale pixels */
	      int           	row,		/* I - Current Y coordinate */
	      int           	src_width,	/* I - Width of input row */
	      int           	dst_width,	/* I - Width of output rows */
	      unsigned char 	*black)		/* O - Black bitmap pixels */
{
  int		x,		/* Current X coordinate */
		xerror,		/* X error count */
		xstep,		/* X step */
		xmod,		/* X error modulus */
		length;		/* Length of output bitmap in bytes */
  unsigned char	bit,		/* Current bit */
		*kptr;		/* Current black pixel */
  int		k,		/* Current black value */
		ditherk,	/* Next error value in buffer */
		*kerror0,	/* Pointer to current error row */
		*kerror1;	/* Pointer to next error row */
  int		ditherbit;	/* Random dither bitmask */


  xstep  = src_width / dst_width;
  xmod   = src_width % dst_width;
  length = (dst_width + 7) / 8;

  kerror0 = error[row & 1][3];
  kerror1 = error[1 - (row & 1)][3];

  memset(black, 0, length * 2);

  for (x = 0, bit = 128, kptr = black, xerror = 0, ditherbit = rand(),
           ditherk = kerror0[0];
       x < dst_width;
       x ++, kerror0 ++, kerror1 ++)
  {
    k = 65535 - *gray + ditherk / 8;

    if (k > ((LEVEL_2 + LEVEL_3) / 2))
    {
      kptr[0]      |= bit;
      kptr[length] |= bit;
      k -= LEVEL_3;
    }
    else if (k > ((LEVEL_1 + LEVEL_2) / 2))
    {
      kptr[length] |= bit;
      k -= LEVEL_2;
    }
    else if (k > ((LEVEL_0 + LEVEL_1) / 2))
    {
      kptr[0] |= bit;
      k -= LEVEL_1;
    }

    if (ditherbit & bit)
    {
      kerror1[0] = 5 * k;
      ditherk    = kerror0[1] + 3 * k;
    }
    else
    {
      kerror1[0] = 3 * k;
      ditherk    = kerror0[1] + 5 * k;
    }

    if (bit == 1)
    {
      kptr ++;

      bit       = 128;
      ditherbit = rand();
    }
    else
      bit >>= 1;

    gray   += xstep;
    xerror += xmod;
    if (xerror >= dst_width)
    {
      xerror -= dst_width;
      gray ++;
    }
  }
}


/*
 * 'dither_cmyk4()' - Dither RGB pixels to 4 levels of cyan, magenta, yellow,
 *                    and black.
 */

void
dither_cmyk4(unsigned short     *rgb,		/* I - RGB pixels */
	     int           	row,		/* I - Current Y coordinate */
	     int           	src_width,	/* I - Width of input row */
	     int           	dst_width,	/* I - Width of output rows */
	     unsigned char	*cyan,		/* O - Cyan bitmap pixels */
	     unsigned char 	*magenta,	/* O - Magenta bitmap pixels */
	     unsigned char 	*yellow,	/* O - Yellow bitmap pixels */
	     unsigned char 	*black)		/* O - Black bitmap pixels */
{
  int		x,		/* Current X coordinate */
		xerror,		/* X error count */
		xstep,		/* X step */
		xmod,		/* X error modulus */
		length;		/* Length of output bitmap in bytes */
  long long	c, m, y, k,	/* CMYK values */
		divk,		/* Inverse of K */
		diff;		/* Average color difference */
  unsigned char	bit,		/* Current bit */
		*cptr,		/* Current cyan pixel */
		*mptr,		/* Current magenta pixel */
		*yptr,		/* Current yellow pixel */
		*kptr;		/* Current black pixel */
  int		ditherc,	/* Next error value in buffer */
		*cerror0,	/* Pointer to current error row */
		*cerror1;	/* Pointer to next error row */
  int		dithery,	/* Next error value in buffer */
		*yerror0,	/* Pointer to current error row */
		*yerror1;	/* Pointer to next error row */
  int		ditherm,	/* Next error value in buffer */
		*merror0,	/* Pointer to current error row */
		*merror1;	/* Pointer to next error row */
  int		ditherk,	/* Next error value in buffer */
		*kerror0,	/* Pointer to current error row */
		*kerror1;	/* Pointer to next error row */
  int		ditherbit;	/* Random dither bitmask */


  xstep  = 3 * (src_width / dst_width);
  xmod   = src_width % dst_width;
  length = (dst_width + 7) / 8;

  cerror0 = error[row & 1][0];
  cerror1 = error[1 - (row & 1)][0];

  merror0 = error[row & 1][1];
  merror1 = error[1 - (row & 1)][1];

  yerror0 = error[row & 1][2];
  yerror1 = error[1 - (row & 1)][2];

  kerror0 = error[row & 1][3];
  kerror1 = error[1 - (row & 1)][3];

  memset(cyan, 0, length * 2);
  memset(magenta, 0, length * 2);
  memset(yellow, 0, length * 2);
  memset(black, 0, length * 2);

  for (x = 0, bit = 128, cptr = cyan, mptr = magenta, yptr = yellow,
           kptr = black, xerror = 0, ditherbit = rand(), ditherc = cerror0[0],
           ditherm = merror0[0], dithery = yerror0[0], ditherk = kerror0[0];
       x < dst_width;
       x ++, cerror0 ++, cerror1 ++, merror0 ++, merror1 ++, yerror0 ++,
           yerror1 ++, kerror0 ++, kerror1 ++)
  {
   /*
    * First compute the standard CMYK separation color values...
    */

    c = 65535 - rgb[0];
    m = 65535 - rgb[1];
    y = 65535 - rgb[2];
    k = MIN(c, MIN(m, y));

   /*
    * Since we're printing black, adjust the black level based upon
    * the amount of color in the pixel (colorful pixels get less black)...
    */

    diff = 65536 - (abs(c - m) + abs(c - y) + abs(m - y)) / 3;
    diff = diff * diff * diff / (65536ll * 65536ll); /* diff = diff^3 */
    diff--;
    k    = diff * k / 65535ll;
    divk = 65535 - k;

    if (divk == 0)
      c = m = y = 0;	/* Grayscale */
    else
    {
     /*
      * Full color; update the CMY values for the black value and reduce
      * CMY as necessary to give better blues, greens, and reds... :)
      */

      c  = (65535 - rgb[1] / 4) * (c - k) / divk;
      m  = (65535 - rgb[2] / 4) * (m - k) / divk;
      y  = (65535 - rgb[0] / 4) * (y - k) / divk;
    }

    k += ditherk / 8;
    if (k > ((LEVEL_2 + LEVEL_3) / 2))
    {
      kptr[0]      |= bit;
      kptr[length] |= bit;
      k -= LEVEL_3;
    }
    else if (k > ((LEVEL_1 + LEVEL_2) / 2))
    {
      kptr[length] |= bit;
      k -= LEVEL_2;
    }
    else if (k > ((LEVEL_0 + LEVEL_1) / 2))
    {
      kptr[0] |= bit;
      k -= LEVEL_1;
    }

    if (ditherbit & bit)
    {
      kerror1[0] = 5 * k;
      ditherk    = kerror0[1] + 3 * k;
    }
    else
    {
      kerror1[0] = 3 * k;
      ditherk    = kerror0[1] + 5 * k;
    }

    c += ditherc / 8;
    if (c > ((LEVEL_2 + LEVEL_3) / 2))
    {
      cptr[0]      |= bit;
      cptr[length] |= bit;
      c -= LEVEL_3;
    }
    else if (c > ((LEVEL_1 + LEVEL_2) / 2))
    {
      cptr[length] |= bit;
      c -= LEVEL_2;
    }
    else if (c > ((LEVEL_0 + LEVEL_1) / 2))
    {
      cptr[0] |= bit;
      c -= LEVEL_1;
    }

    if (ditherbit & bit)
    {
      cerror1[0] = 5 * c;
      ditherc    = cerror0[1] + 3 * c;
    }
    else
    {
      cerror1[0] = 3 * c;
      ditherc    = cerror0[1] + 5 * c;
    }

    m += ditherm / 8;
    if (m > ((LEVEL_2 + LEVEL_3) / 2))
    {
      mptr[0]      |= bit;
      mptr[length] |= bit;
      m -= LEVEL_3;
    }
    else if (m > ((LEVEL_1 + LEVEL_2) / 2))
    {
      mptr[length] |= bit;
      m -= LEVEL_2;
    }
    else if (m > ((LEVEL_0 + LEVEL_1) / 2))
    {
      mptr[0] |= bit;
      m -= LEVEL_1;
    }

    if (ditherbit & bit)
    {
      merror1[0] = 5 * m;
      ditherm    = merror0[1] + 3 * m;
    }
    else
    {
      merror1[0] = 3 * m;
      ditherm    = merror0[1] + 5 * m;
    }

    y += dithery / 8;
    if (y > ((LEVEL_2 + LEVEL_3) / 2))
    {
      yptr[0]      |= bit;
      yptr[length] |= bit;
      y -= LEVEL_3;
    }
    else if (y > ((LEVEL_1 + LEVEL_2) / 2))
    {
      yptr[length] |= bit;
      y -= LEVEL_2;
    }
    else if (y > ((LEVEL_0 + LEVEL_1) / 2))
    {
      yptr[0] |= bit;
      y -= LEVEL_1;
    }

    if (ditherbit & bit)
    {
      yerror1[0] = 5 * y;
      dithery    = yerror0[1] + 3 * y;
    }
    else
    {
      yerror1[0] = 3 * y;
      dithery    = yerror0[1] + 5 * y;
    }

    if (bit == 1)
    {
      cptr ++;
      mptr ++;
      yptr ++;
      kptr ++;

      bit       = 128;
      ditherbit = rand();
    }
    else
      bit >>= 1;

    rgb    += xstep;
    xerror += xmod;
    if (xerror >= dst_width)
    {
      xerror -= dst_width;
      rgb    += 3;
    }
  }
}

/*
 * 'gray_to_gray()' - Convert grayscale image data to grayscale (brightness
 *                    adjusted).
 */

void
gray_to_gray(unsigned char *grayin,	/* I - RGB pixels */
	     unsigned short *grayout,	/* O - RGB pixels */
	     int    	width,		/* I - Width of row */
	     int    	bpp,		/* I - Bytes-per-pixel in grayin */
	     lut_t  	*lut,		/* I - Brightness lookup table */
	     unsigned char *cmap,	/* I - Colormap (unused) */
	     vars_t	*vars
	     )
{
  if (bpp == 1)
  {
   /*
    * No alpha in image...
    */

    while (width > 0)
    {
      *grayout = lut->composite[*grayin];

      grayin ++;
      grayout ++;
      width --;
    }
  }
  else
  {
   /*
    * Handle alpha in image...
    */

    while (width > 0)
    {
      *grayout = lut->composite[grayin[0] * grayin[1] / 255] + 255 - grayin[1];

      grayin += bpp;
      grayout ++;
      width --;
    }
  }
}


/*
 * 'indexed_to_gray()' - Convert indexed image data to grayscale.
 */

void
indexed_to_gray(unsigned char *indexed,		/* I - Indexed pixels */
		  unsigned short *gray,		/* O - Grayscale pixels */
		  int    width,			/* I - Width of row */
		  int    bpp,			/* I - bpp in indexed */
		  lut_t  *lut,			/* I - Brightness LUT */
		  unsigned char *cmap,		/* I - Colormap */
		  vars_t   *vars		/* I - Saturation */
		  )
{
  int		i;			/* Looping var */
  unsigned char	gray_cmap[256];		/* Grayscale colormap */


  for (i = 0; i < 256; i ++, cmap += 3)
    gray_cmap[i] = (cmap[0] * LUM_RED +
		    cmap[1] * LUM_GREEN +
		    cmap[2] * LUM_BLUE) / 100;

  if (bpp == 1)
  {
   /*
    * No alpha in image...
    */

    while (width > 0)
    {
      *gray = lut->composite[gray_cmap[*indexed]];
      indexed ++;
      gray ++;
      width --;
    }
  }
  else
  {
   /*
    * Handle alpha in image...
    */

    while (width > 0)
    {
      *gray = lut->composite[gray_cmap[indexed[0] * indexed[1] / 255] +
			    255 - indexed[1]];
      indexed += bpp;
      gray ++;
      width --;
    }
  }
}


void
indexed_to_rgb(unsigned char *indexed,	/* I - Indexed pixels */
		 unsigned short *rgb,	/* O - RGB pixels */
		 int    width,		/* I - Width of row */
		 int    bpp,		/* I - Bytes-per-pixel in indexed */
		 lut_t  *lut,		/* I - Brightness lookup table */
		 unsigned char *cmap,	/* I - Colormap */
		 vars_t   *vars	/* I - Saturation */
		 )
{
  if (bpp == 1)
  {
   /*
    * No alpha in image...
    */

    while (width > 0)
    {
      rgb[0] = lut->red[cmap[*indexed * 3 + 0]];
      rgb[1] = lut->green[cmap[*indexed * 3 + 1]];
      rgb[2] = lut->blue[cmap[*indexed * 3 + 2]];
      if (vars->saturation != 1.0)
	{
	  double h, s, v;
	  unsigned char rgb1[3];
	  rgb1[0] = rgb[0];
	  rgb1[1] = rgb[1];
	  rgb1[2] = rgb[2];
	  gimp_rgb_to_hsv4 (rgb1, &h, &s, &v);
	  s = pow(s, 1.0 / vars->saturation);
	  gimp_hsv_to_rgb4 (rgb1, h, s, v);
	  rgb[0] = rgb1[0];
	  rgb[1] = rgb1[1];
	  rgb[2] = rgb1[2];
	}
      rgb += 3;
      indexed ++;
      width --;
    }
  }
  else
  {
   /*
    * RGBA image...
    */

    while (width > 0)
    {
      rgb[0] = lut->red[cmap[indexed[0] * 3 + 0] * indexed[1] / 255 +
		       255 - indexed[1]];
      rgb[1] = lut->green[cmap[indexed[0] * 3 + 1] * indexed[1] / 255 +
			 255 - indexed[1]];
      rgb[2] = lut->blue[cmap[indexed[0] * 3 + 2] * indexed[1] / 255 +
			255 - indexed[1]];
      if (vars->saturation != 1.0)
	{
	  double h, s, v;
	  unsigned char rgb1[3];
	  rgb1[0] = rgb[0];
	  rgb1[1] = rgb[1];
	  rgb1[2] = rgb[2];
	  gimp_rgb_to_hsv4 (rgb1, &h, &s, &v);
	  s = pow(s, 1.0 / vars->saturation);
	  gimp_hsv_to_rgb4 (rgb1, h, s, v);
	  rgb[0] = rgb1[0];
	  rgb[1] = rgb1[1];
	  rgb[2] = rgb1[2];
	}
      rgb += 3;
      indexed += bpp;
      width --;
    }
  }
}



/*
 * 'rgb_to_gray()' - Convert RGB image data to grayscale.
 */

void
rgb_to_gray(unsigned char *rgb,		/* I - RGB pixels */
	      unsigned short *gray,	/* O - Grayscale pixels */
	      int    width,		/* I - Width of row */
	      int    bpp,		/* I - Bytes-per-pixel in RGB */
	      lut_t  *lut,		/* I - Brightness lookup table */
	      unsigned char *cmap,	/* I - Colormap (unused) */
	      vars_t   *vars		/* I - Saturation */
	      )
{
  if (bpp == 3)
  {
   /*
    * No alpha in image...
    */

    while (width > 0)
    {
      *gray = lut->composite[(rgb[0] * LUM_RED +
			      rgb[1] * LUM_GREEN +
			      rgb[2] * LUM_BLUE) / 100];
      gray ++;
      rgb += 3;
      width --;
    }
  }
  else
  {
   /*
    * Image has alpha channel...
    */

    while (width > 0)
    {
      *gray = lut->composite[((rgb[0] * LUM_RED +
			       rgb[1] * LUM_GREEN +
			       rgb[2] * LUM_BLUE) *
			      rgb[3] / 25500 + 255 - rgb[3])];
      gray ++;
      rgb += bpp;
      width --;
    }
  }
}

/*
 * 'rgb_to_rgb()' - Convert rgb image data to RGB.
 */

void
rgb_to_rgb(unsigned char	*rgbin,		/* I - RGB pixels */
	   unsigned short 	*rgbout,	/* O - RGB pixels */
	   int    		width,		/* I - Width of row */
	   int    		bpp,		/* I - Bytes/pix in indexed */
	   lut_t 		*lut,		/* I - Brightness LUT */
	   unsigned char 	*cmap,		/* I - Colormap */
	   vars_t  		*vars		/* I - Saturation */
	   )
{
  if (bpp == 3)
  {
   /*
    * No alpha in image...
    */

    while (width > 0)
    {
      rgbout[0] = lut->red[rgbin[0]];
      rgbout[1] = lut->green[rgbin[1]];
      rgbout[2] = lut->blue[rgbin[2]];
      if (vars->saturation != 1.0 || vars->contrast != 100)
	{
	  double h, s, v;
	  unsigned char rgb1[3];
	  rgb1[0] = rgbout[0];
	  rgb1[1] = rgbout[1];
	  rgb1[2] = rgbout[2];
	  gimp_rgb_to_hsv4 (rgb1, &h, &s, &v);
	  if (vars->saturation != 1.0)
	    s = pow(s, 1.0 / vars->saturation);
#if 0
	  if (vars->contrast != 100)
	    {
	      double contrast = vars->contrast / 100.0;
	      double tv = fabs(v - .5) * 2.0;
	      tv = pow(tv, 1.0 / (contrast * contrast));
	      if (v < .5)
		tv = - tv;
	      v = (tv / 2.0) + .5;
	    }
#endif
	  gimp_hsv_to_rgb4 (rgb1, h, s, v);
	  rgbout[0] = rgb1[0];
	  rgbout[1] = rgb1[1];
	  rgbout[2] = rgb1[2];
	}
      if (vars->density != 1.0)
	{
	  float t;
	  int i;
	  for (i = 0; i < 3; i++)
	    {
	      t = ((float) rgbout[i]) / 65536.0;
	      t = (1.0 + ((t - 1.0) * vars->density));
	      if (t < 0.0)
		t = 0.0;
	      rgbout[i] = (unsigned short) (t * 65536.0);
	    }
	}
      rgbin += 3;
      rgbout += 3;
      width --;
    }
  }
  else
  {
   /*
    * RGBA image...
    */

    while (width > 0)
    {
      rgbout[0] = lut->red[rgbin[0] * rgbin[3] / 255 + 255 - rgbin[3]];
      rgbout[1] = lut->green[rgbin[1] * rgbin[3] / 255 + 255 - rgbin[3]];
      rgbout[2] = lut->blue[rgbin[2] * rgbin[3] / 255 + 255 - rgbin[3]];
      if (vars->saturation != 1.0 || vars->contrast != 100 ||
	  vars->density != 1.0)
	{
	  double h, s, v;
	  unsigned char rgb1[3];
	  rgb1[0] = rgbout[0];
	  rgb1[1] = rgbout[1];
	  rgb1[2] = rgbout[2];
	  gimp_rgb_to_hsv4 (rgb1, &h, &s, &v);
	  if (vars->saturation != 1.0)
	    s = pow(s, 1.0 / vars->saturation);
#if 0
	  if (vars->contrast != 100)
	    {
	      double contrast = vars->contrast / 100.0;
	      double tv = fabs(v - .5) * 2.0;
	      tv = pow(tv, 1.0 / (contrast * contrast));
	      if (v < .5)
		tv = - tv;
	      v = (tv / 2.0) + .5;
	    }
#endif
	  gimp_hsv_to_rgb4 (rgb1, h, s, v);
	  rgbout[0] = rgb1[0];
	  rgbout[1] = rgb1[1];
	  rgbout[2] = rgb1[2];
	}
      if (vars->density != 1.0)
	{
	  float t;
	  int i;
	  for (i = 0; i < 3; i++)
	    {
	      t = ((float) rgbout[i]) / 65536.0;
	      t = (1.0 + ((t - 1.0) * vars->density));
	      if (t < 0.0)
		t = 0.0;
	      rgbout[i] = (unsigned short) (t * 65536.0);
	    }
	}
      rgbin += bpp;
      rgbout += 3;
      width --;
    }
  }
}

/* #define PRINT_LUT */

void
compute_lut(lut_t *lut,
	    float print_gamma,
	    float app_gamma,
	    vars_t *v)
{
  float		brightness,	/* Computed brightness */
		screen_gamma,	/* Screen gamma correction */
		pixel,		/* Pixel value */
		red_pixel,	/* Pixel value */
		green_pixel,	/* Pixel value */
		blue_pixel;	/* Pixel value */
  int i;
#ifdef PRINT_LUT
  FILE *ltfile = fopen("/mnt1/lut", "w");
#endif
  /*
   * Got an output file/command, now compute a brightness lookup table...
   */

  float red = 100.0 / v->red ;
  float green = 100.0 / v->green;
  float blue = 100.0 / v->blue;
  float contrast;
  contrast = v->contrast / 100.0;
  if (red < 0.01)
    red = 0.01;
  if (green < 0.01)
    green = 0.01;
  if (blue < 0.01)
    blue = 0.01;

  if (v->linear)
    {
      screen_gamma = app_gamma / 1.7;
      brightness   = v->brightness / 100.0;
    }
  else
    {
      brightness   = 100.0 / v->brightness;
      screen_gamma = app_gamma * brightness / 1.7;
    }

  print_gamma = v->gamma / print_gamma;

  for (i = 0; i < 256; i ++)
    {
      if (v->linear)
	{
	  double adjusted_pixel;
	  pixel = adjusted_pixel = (float) i / 255.0;

	  if (brightness < 1.0)
	    adjusted_pixel = adjusted_pixel * brightness;
	  else if (brightness > 1.0)
	    adjusted_pixel = 1.0 - ((1.0 - adjusted_pixel) / brightness);

	  if (pixel < 0)
	    adjusted_pixel = 0;
	  else if (pixel > 1.0)
	    adjusted_pixel = 1.0;

	  adjusted_pixel = pow(adjusted_pixel,
			       print_gamma * screen_gamma * print_gamma);

	  adjusted_pixel *= 65535.0;

	  red_pixel = green_pixel = blue_pixel = adjusted_pixel;
	  lut->composite[i] = adjusted_pixel;
	  lut->red[i] = adjusted_pixel;
	  lut->green[i] = adjusted_pixel;
	  lut->blue[i] = adjusted_pixel;
	}
      else
	{
	  float temp_pixel;
	  pixel = (float) i / 255.0;

	  /*
	   * First, correct contrast
	   */
	  temp_pixel = fabs((pixel - .5) * 2.0);
	  temp_pixel = pow(temp_pixel, 1.0 / contrast);
	  if (pixel < .5)
	    temp_pixel = -temp_pixel;
	  pixel = (temp_pixel / 2.0) + .5;

	  /*
	   * Second, perform screen gamma correction
	   */
	  pixel = 1.0 - pow(pixel, screen_gamma);

	  /*
	   * Third, fix up red, green, blue values
	   *
	   * I don't know how to do this correctly.  I think that what I'll do
	   * is if the correction is less than 1 to multiply it by the
	   * correction; if it's greater than 1, hinge it around 64K.
	   * Doubtless we can do better.  Oh well.
	   */
	  if (pixel < 0.0)
	    pixel = 0.0;
	  else if (pixel > 1.0)
	    pixel = 1.0;

	  red_pixel = pow(pixel, 1.0 / (red * red));
	  green_pixel = pow(pixel, 1.0 / (green * green));
	  blue_pixel = pow(pixel, 1.0 / (blue * blue));

	  /*
	   * Finally, fix up print gamma and scale
	   */

	  pixel = 256.0 * (256.0 - 256.0 *
			   pow(pixel, print_gamma));
	  red_pixel = 256.0 * (256.0 - 256.0 *
			       pow(red_pixel, print_gamma));
	  green_pixel = 256.0 * (256.0 - 256.0 *
				 pow(green_pixel, print_gamma));
	  blue_pixel = 256.0 * (256.0 - 256.0 *
				pow(blue_pixel, print_gamma));

	  if (pixel <= 0.0)
	    lut->composite[i] = 0;
	  else if (pixel >= 65535.0)
	    lut->composite[i] = 65535;
	  else
	    lut->composite[i] = (unsigned)(pixel);

	  if (red_pixel <= 0.0)
	    lut->red[i] = 0;
	  else if (red_pixel >= 65535.0)
	    lut->red[i] = 65535;
	  else
	    lut->red[i] = (unsigned)(red_pixel);

	  if (green_pixel <= 0.0)
	    lut->green[i] = 0;
	  else if (green_pixel >= 65535.0)
	    lut->green[i] = 65535;
	  else
	    lut->green[i] = (unsigned)(green_pixel);

	  if (blue_pixel <= 0.0)
	    lut->blue[i] = 0;
	  else if (blue_pixel >= 65535.0)
	    lut->blue[i] = 65535;
	  else
	    lut->blue[i] = (unsigned)(blue_pixel);
	}
#ifdef PRINT_LUT
      fprintf(ltfile, "%3i  %5d  %5d  %5d  %5d  %f %f %f %f  %f %f %f  %f\n",
	      i, lut->composite[i], lut->red[i], lut->green[i],
	      lut->blue[i], pixel, red_pixel, green_pixel, blue_pixel,
	      print_gamma, screen_gamma, print_gamma, app_gamma);
#endif
    }

#ifdef PRINT_LUT
  fclose(ltfile);
#endif
}

/*
 * 'default_media_size()' - Return the size of a default page size.
 */

void
default_media_size(int  model,		/* I - Printer model */
        	   char *ppd_file,	/* I - PPD file (not used) */
        	   char *media_size,	/* I - Media size */
        	   int  *width,		/* O - Width in points */
        	   int  *length)	/* O - Length in points */
{
  if (strcmp(media_size, "Letter") == 0)
  {
    *width  = 612;
    *length = 792;
  }
  else if (strcmp(media_size, "Legal") == 0)
  {
    *width  = 612;
    *length = 1008;
  }
  else if (strcmp(media_size, "Tabloid") == 0)
  {
    *width  = 792;
    *length = 1214;
  }
  else if (strcmp(media_size, "12x18") == 0)
  {
    *width  = 864;
    *length = 1296;
  }
  else if (strcmp(media_size, "A4") == 0)
  {
    *width  = 595;
    *length = 842;
  }
  else if (strcmp(media_size, "A3") == 0)
  {
    *width  = 842;
    *length = 1191;
  }
  else
  {
    *width  = 0;
    *length = 0;
  }
}



#ifdef LEFTOVER_8_BIT
/*
 * Everything here and below goes away when this is tested on all printers.
 */

#define LEVEL_3	255
#define LEVEL_2	213
#define LEVEL_1	127
#define LEVEL_0	0

void
dither_black4(unsigned char *gray,	/* I - Grayscale pixels */
              int           row,	/* I - Current Y coordinate */
              int           src_width,	/* I - Width of input row */
              int           dst_width,	/* I - Width of output rows */
              unsigned char *black)	/* O - Black bitmap pixels */
{
  int		x,		/* Current X coordinate */
		xerror,		/* X error count */
		xstep,		/* X step */
		xmod,		/* X error modulus */
		length;		/* Length of output bitmap in bytes */
  unsigned char	bit,		/* Current bit */
		*kptr;		/* Current black pixel */
  int		k,		/* Current black value */
		ditherk,	/* Next error value in buffer */
		*kerror0,	/* Pointer to current error row */
		*kerror1;	/* Pointer to next error row */
  int		ditherbit;	/* Random dither bitmask */


  xstep  = src_width / dst_width;
  xmod   = src_width % dst_width;
  length = (dst_width + 7) / 8;

  kerror0 = error[row & 1][3];
  kerror1 = error[1 - (row & 1)][3];

  memset(black, 0, length * 2);

  for (x = 0, bit = 128, kptr = black, xerror = 0, ditherbit = rand(),
           ditherk = kerror0[0];
       x < dst_width;
       x ++, kerror0 ++, kerror1 ++)
  {
    k = 255 - *gray + ditherk / 8;

    if (k > ((LEVEL_2 + LEVEL_3) / 2))
    {
      kptr[0]      |= bit;
      kptr[length] |= bit;
      k -= LEVEL_3;
    }
    else if (k > ((LEVEL_1 + LEVEL_2) / 2))
    {
      kptr[length] |= bit;
      k -= LEVEL_2;
    }
    else if (k > ((LEVEL_0 + LEVEL_1) / 2))
    {
      kptr[0] |= bit;
      k -= LEVEL_1;
    }

    if (ditherbit & bit)
    {
      kerror1[0] = 5 * k;
      ditherk    = kerror0[1] + 3 * k;
    }
    else
    {
      kerror1[0] = 3 * k;
      ditherk    = kerror0[1] + 5 * k;
    }

    if (bit == 1)
    {
      kptr ++;

      bit       = 128;
      ditherbit = rand();
    }
    else
      bit >>= 1;

    gray   += xstep;
    xerror += xmod;
    if (xerror >= dst_width)
    {
      xerror -= dst_width;
      gray ++;
    }
  }
}

void
dither_cmyk4(unsigned char *rgb,	/* I - RGB pixels */
             int           row,		/* I - Current Y coordinate */
             int           src_width,	/* I - Width of input row */
             int           dst_width,	/* I - Width of output rows */
             unsigned char *cyan,	/* O - Cyan bitmap pixels */
             unsigned char *magenta,	/* O - Magenta bitmap pixels */
             unsigned char *yellow,	/* O - Yellow bitmap pixels */
             unsigned char *black)	/* O - Black bitmap pixels */
{
  int		x,		/* Current X coordinate */
		xerror,		/* X error count */
		xstep,		/* X step */
		xmod,		/* X error modulus */
		length;		/* Length of output bitmap in bytes */
  int		c, m, y, k,	/* CMYK values */
		divk,		/* Inverse of K */
		diff;		/* Average color difference */
  unsigned char	bit,		/* Current bit */
		*cptr,		/* Current cyan pixel */
		*mptr,		/* Current magenta pixel */
		*yptr,		/* Current yellow pixel */
		*kptr;		/* Current black pixel */
  int		ditherc,	/* Next error value in buffer */
		*cerror0,	/* Pointer to current error row */
		*cerror1;	/* Pointer to next error row */
  int		dithery,	/* Next error value in buffer */
		*yerror0,	/* Pointer to current error row */
		*yerror1;	/* Pointer to next error row */
  int		ditherm,	/* Next error value in buffer */
		*merror0,	/* Pointer to current error row */
		*merror1;	/* Pointer to next error row */
  int		ditherk,	/* Next error value in buffer */
		*kerror0,	/* Pointer to current error row */
		*kerror1;	/* Pointer to next error row */
  int		ditherbit;	/* Random dither bitmask */


  xstep  = 3 * (src_width / dst_width);
  xmod   = src_width % dst_width;
  length = (dst_width + 7) / 8;

  cerror0 = error[row & 1][0];
  cerror1 = error[1 - (row & 1)][0];

  merror0 = error[row & 1][1];
  merror1 = error[1 - (row & 1)][1];

  yerror0 = error[row & 1][2];
  yerror1 = error[1 - (row & 1)][2];

  kerror0 = error[row & 1][3];
  kerror1 = error[1 - (row & 1)][3];

  memset(cyan, 0, length * 2);
  memset(magenta, 0, length * 2);
  memset(yellow, 0, length * 2);
  memset(black, 0, length * 2);

  for (x = 0, bit = 128, cptr = cyan, mptr = magenta, yptr = yellow,
           kptr = black, xerror = 0, ditherbit = rand(), ditherc = cerror0[0],
           ditherm = merror0[0], dithery = yerror0[0], ditherk = kerror0[0];
       x < dst_width;
       x ++, cerror0 ++, cerror1 ++, merror0 ++, merror1 ++, yerror0 ++,
           yerror1 ++, kerror0 ++, kerror1 ++)
  {
   /*
    * First compute the standard CMYK separation color values...
    */

    c = 255 - rgb[0];
    m = 255 - rgb[1];
    y = 255 - rgb[2];
    k = MIN(c, MIN(m, y));

   /*
    * Since we're printing black, adjust the black level based upon
    * the amount of color in the pixel (colorful pixels get less black)...
    */

    diff = 255 - (abs(c - m) + abs(c - y) + abs(m - y)) / 3;
    diff = diff * diff * diff / 65025; /* diff = diff^3 */
    k    = diff * k / 255;
    divk = 255 - k;

    if (divk == 0)
      c = m = y = 0;	/* Grayscale */
    else
    {
     /*
      * Full color; update the CMY values for the black value and reduce
      * CMY as necessary to give better blues, greens, and reds... :)
      */

      c  = (255 - rgb[1] / 4) * (c - k) / divk;
      m  = (255 - rgb[2] / 4) * (m - k) / divk;
      y  = (255 - rgb[0] / 4) * (y - k) / divk;
    }

    k += ditherk / 8;
    if (k > ((LEVEL_2 + LEVEL_3) / 2))
    {
      kptr[0]      |= bit;
      kptr[length] |= bit;
      k -= LEVEL_3;
    }
    else if (k > ((LEVEL_1 + LEVEL_2) / 2))
    {
      kptr[length] |= bit;
      k -= LEVEL_2;
    }
    else if (k > ((LEVEL_0 + LEVEL_1) / 2))
    {
      kptr[0] |= bit;
      k -= LEVEL_1;
    }

    if (ditherbit & bit)
    {
      kerror1[0] = 5 * k;
      ditherk    = kerror0[1] + 3 * k;
    }
    else
    {
      kerror1[0] = 3 * k;
      ditherk    = kerror0[1] + 5 * k;
    }

    c += ditherc / 8;
    if (c > ((LEVEL_2 + LEVEL_3) / 2))
    {
      cptr[0]      |= bit;
      cptr[length] |= bit;
      c -= LEVEL_3;
    }
    else if (c > ((LEVEL_1 + LEVEL_2) / 2))
    {
      cptr[length] |= bit;
      c -= LEVEL_2;
    }
    else if (c > ((LEVEL_0 + LEVEL_1) / 2))
    {
      cptr[0] |= bit;
      c -= LEVEL_1;
    }

    if (ditherbit & bit)
    {
      cerror1[0] = 5 * c;
      ditherc    = cerror0[1] + 3 * c;
    }
    else
    {
      cerror1[0] = 3 * c;
      ditherc    = cerror0[1] + 5 * c;
    }

    m += ditherm / 8;
    if (m > ((LEVEL_2 + LEVEL_3) / 2))
    {
      mptr[0]      |= bit;
      mptr[length] |= bit;
      m -= LEVEL_3;
    }
    else if (m > ((LEVEL_1 + LEVEL_2) / 2))
    {
      mptr[length] |= bit;
      m -= LEVEL_2;
    }
    else if (m > ((LEVEL_0 + LEVEL_1) / 2))
    {
      mptr[0] |= bit;
      m -= LEVEL_1;
    }

    if (ditherbit & bit)
    {
      merror1[0] = 5 * m;
      ditherm    = merror0[1] + 3 * m;
    }
    else
    {
      merror1[0] = 3 * m;
      ditherm    = merror0[1] + 5 * m;
    }

    y += dithery / 8;
    if (y > ((LEVEL_2 + LEVEL_3) / 2))
    {
      yptr[0]      |= bit;
      yptr[length] |= bit;
      y -= LEVEL_3;
    }
    else if (y > ((LEVEL_1 + LEVEL_2) / 2))
    {
      yptr[length] |= bit;
      y -= LEVEL_2;
    }
    else if (y > ((LEVEL_0 + LEVEL_1) / 2))
    {
      yptr[0] |= bit;
      y -= LEVEL_1;
    }

    if (ditherbit & bit)
    {
      yerror1[0] = 5 * y;
      dithery    = yerror0[1] + 3 * y;
    }
    else
    {
      yerror1[0] = 3 * y;
      dithery    = yerror0[1] + 5 * y;
    }

    if (bit == 1)
    {
      cptr ++;
      mptr ++;
      yptr ++;
      kptr ++;

      bit       = 128;
      ditherbit = rand();
    }
    else
      bit >>= 1;

    rgb    += xstep;
    xerror += xmod;
    if (xerror >= dst_width)
    {
      xerror -= dst_width;
      rgb    += 3;
    }
  }
}

void
gray_to_gray(unsigned char *grayin,	/* I - RGB pixels */
             unsigned char *grayout,	/* O - RGB pixels */
             int   	width,		/* I - Width of row */
             int    	bpp,		/* I - Bytes-per-pixel in grayin */
             unsigned char *cmap,	/* I - Colormap (unused) */
	     float  	saturation	/* I - Saturation */
	     )
{
  if (bpp == 1)
  {
   /*
    * No alpha in image...
    */

    while (width > 0)
    {
      *grayout = lut->composite[*grayin];

      grayin ++;
      grayout ++;
      width --;
    }
  }
  else
  {
   /*
    * Handle alpha in image...
    */

    while (width > 0)
    {
      *grayout = lut->composite[grayin[0] * grayin[1] / 255] + 255 - grayin[1];

      grayin += bpp;
      grayout ++;
      width --;
    }
  }
}

void
indexed_to_gray(unsigned char 	*indexed,	/* I - Indexed pixels */
                unsigned char 	*gray,		/* O - Grayscale pixels */
        	int    		width,		/* I - Width of row */
        	int    		bpp,		/* I - Bytes/pix in indexed */
                unsigned char 	*cmap,		/* I - Colormap */
		float  		saturation	/* I - Saturation */
		)
{
  int		i;			/* Looping var */
  unsigned char	gray_cmap[256];		/* Grayscale colormap */


  for (i = 0; i < 256; i ++, cmap += 3)
    gray_cmap[i] = (cmap[0] * LUM_RED +
		    cmap[1] * LUM_GREEN +
		    cmap[2] * LUM_BLUE) / 100;

  if (bpp == 1)
  {
   /*
    * No alpha in image...
    */

    while (width > 0)
    {
      *gray = lut->composite[gray_cmap[*indexed]];
      indexed ++;
      gray ++;
      width --;
    }
  }
  else
  {
   /*
    * Handle alpha in image...
    */

    while (width > 0)
    {
      *gray = lut->composite[gray_cmap[indexed[0] * indexed[1] / 255] +
			    255 - indexed[1]];
      indexed += bpp;
      gray ++;
      width --;
    }
  }
}

#endif

/*
 * End of "$Id$".
 */
