/*
 * "$Id$"
 *
 *   Print plug-in driver utility functions for the GIMP.
 *
 *   Copyright 1997-1999 Michael Sweet (mike@easysw.com) and
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
 *   $Log$
 *   Revision 1.11  1999/12/16 19:44:01  olofk
 *   Thu Dec 16 20:15:25 CET 1999  Olof S Kylande <olof@gimp.org>
 *
 *           Fix of KDE/Kwm  selection add/sub/inter problem
 *           NOTE: This is a workaround, not a real fix.
 *           Many Thanks to Matthias Ettrich
 *
 *           * app/disp_callbacks.c
 *
 *           Updated unsharp-mask to version 0.10
 *
 *           * plug-ins/unsharp/dialog_f.c
 *           * plug-ins/unsharp/dialog_f.h
 *           * plug-ins/unsharp/dialog_i.c
 *           * plug-ins/unsharp/dialog_i.h
 *           * plug-ins/unsharp/unsharp.c
 *
 *           Updated print plug-in to version 3.0.1
 *
 *           * plug-ins/print/README (new file)
 *           * plug-ins/print/print-escp2.c
 *           * plug-ins/print/print-pcl.c
 *           * plug-ins/print/print-ps.c
 *           * plug-ins/print/print-util.c
 *           * plug-ins/print/print.c
 *           * plug-ins/print/print.h
 *
 *           Updated all files in the help/C/dialogs dir. This is
 *           a first alpha glimpse of the help system. Please give
 *           me feedback of the content. However since it's in alpha
 *           stage it means that there is spell, grammatical, etc errors.
 *           There is may also be pure errors which I hope "you" will
 *           report to either olof@gimp.org or karin@gimp.org. Please
 *           don't report spell, grammatical, etc error at this stage in dev.
 *
 *           If you have any plans to commit to the help system please write
 *           to olof@gimp.org. (This is mandatory not a please ;-).
 *
 *           * help/C/welcome.html
 *           * help/C/dialogs/about.html ..............
 *
 *   Revision 1.37  1999/12/05 23:24:08  rlk
 *   don't want PRINT_LUT in release
 *
 *   Revision 1.36  1999/12/05 04:33:34  rlk
 *   Good results for the night.
 *
 *   Revision 1.35  1999/12/04 19:01:05  rlk
 *   better use of light colors
 *
 *   Revision 1.34  1999/12/02 02:09:45  rlk
 *   .
 *
 *   Revision 1.33  1999/11/25 00:02:03  rlk
 *   Revamped many controls
 *
 *   Revision 1.32  1999/11/23 02:11:37  rlk
 *   Rationalize variables, pass 3
 *
 *   Revision 1.31  1999/11/23 01:33:37  rlk
 *   First stage of simplifying the variable stuff
 *
 *   Revision 1.30  1999/11/16 00:59:00  rlk
 *   More fine tuning
 *
 *   Revision 1.29  1999/11/14 21:37:13  rlk
 *   Revamped contrast
 *
 *   Revision 1.28  1999/11/14 18:59:22  rlk
 *   Final preparations for release to Olof
 *
 *   Revision 1.27  1999/11/14 00:57:11  rlk
 *   Mix black in sooner gives better density.
 *
 *   Revision 1.26  1999/11/13 02:31:29  rlk
 *   Finally!  Good settings!
 *
 *   Revision 1.25  1999/11/12 03:34:40  rlk
 *   More tweaking
 *
 *   Revision 1.24  1999/11/12 02:18:32  rlk
 *   Stubs for dynamic memory allocation
 *
 *   Revision 1.23  1999/11/12 01:53:37  rlk
 *   Remove silly spurious stuff
 *
 *   Revision 1.22  1999/11/12 01:51:47  rlk
 *   Much better black
 *
 *   Revision 1.21  1999/11/10 01:13:06  rlk
 *   Support up to 2880 dpi
 *
 *   Revision 1.20  1999/11/07 22:16:42  rlk
 *   Bug fixes; try to improve dithering slightly
 *
 *   Revision 1.19  1999/10/29 01:01:16  rlk
 *   Smoother rendering of darker colors
 *
 *   Revision 1.18  1999/10/28 02:01:15  rlk
 *   One bug, two effects:
 *
 *   1) Handle 4-color correctly (it was treating the 4-color too much like the
 *   6-color).
 *
 *   2) An attempt to handle both cases with the same code path led to a
 *   discontinuity that depending upon the orientation of a color gradient would
 *   lead to either white or dark lines at the point that the dark version of
 *   the color would kick in.
 *
 *   Revision 1.17  1999/10/26 23:58:31  rlk
 *   indentation
 *
 *   Revision 1.16  1999/10/26 23:36:51  rlk
 *   Comment out all remaining 16-bit code, and rename 16-bit functions to "standard" names
 *
 *   Revision 1.15  1999/10/26 02:10:30  rlk
 *   Mostly fix save/load
 *
 *   Move all gimp, glib, gtk stuff into print.c (take it out of everything else).
 *   This should help port it to more general purposes later.
 *
 *   Revision 1.14  1999/10/25 23:31:59  rlk
 *   16-bit clean
 *
 *   Revision 1.13  1999/10/25 00:14:46  rlk
 *   Remove more of the 8-bit code, now that it is tested
 *
 *   Revision 1.12  1999/10/23 20:26:48  rlk
 *   Move LUT calculation to print-util
 *
 *   Revision 1.11  1999/10/21 01:27:37  rlk
 *   More progress toward full 16-bit rendering
 *
 *   Revision 1.10  1999/10/19 02:04:59  rlk
 *   Merge all of the single-level print_cmyk functions
 *
 *   Revision 1.9  1999/10/18 01:37:02  rlk
 *   Remove spurious stuff
 *
 *   Revision 1.8  1999/10/17 23:44:07  rlk
 *   16-bit everything (untested)
 *
 *   Revision 1.7  1999/10/17 23:01:01  rlk
 *   Move various dither functions into print-utils.c
 *
 *   Revision 1.6  1999/10/14 01:59:59  rlk
 *   Saturation
 *
 *   Revision 1.5  1999/10/03 23:57:20  rlk
 *   Various improvements
 *
 *   Revision 1.4  1999/09/18 15:18:47  rlk
 *   A bit more random
 *
 *   Revision 1.3  1999/09/14 21:43:43  rlk
 *   Some hoped-for improvements
 *
 *   Revision 1.2  1999/09/12 00:12:24  rlk
 *   Current best stuff
 *
 *   Revision 1.10  1998/05/17 07:16:49  yosh
 *   0.99.31 fun
 *
 *   updated print plugin
 *
 *   -Yosh
 *
 *   Revision 1.14  1998/05/16  18:27:59  mike
 *   Cleaned up dithering functions - unnecessary extra data in dither buffer.
 *
 *   Revision 1.13  1998/05/13  17:00:36  mike
 *   Minor change to CMYK generation code - now cube black difference value
 *   for better colors.
 *
 *   Revision 1.12  1998/05/08  19:20:50  mike
 *   Updated CMYK generation code to use new method.
 *   Updated dithering algorithm (slightly more uniform now, less speckling)
 *   Added default media size function.
 *
 *   Revision 1.11  1998/03/01  18:03:27  mike
 *   Whoops - need to add 255 - alpha to the output values (transparent to white
 *   and not transparent to black...)
 *
 *   Revision 1.10  1998/03/01  17:20:48  mike
 *   Updated alpha code to do alpha computation before gamma/brightness lut.
 *
 *   Revision 1.9  1998/03/01  17:13:46  mike
 *   Updated CMY/CMYK conversion code for dynamic BG and hue adjustment.
 *   Added alpha channel support to color conversion functions.
 *
 *   Revision 1.8  1998/01/21  21:33:47  mike
 *   Replaced Burkes dither with stochastic (random) dither.
 *
 *   Revision 1.7  1997/10/02  17:57:26  mike
 *   Replaced ordered dither with Burkes dither (error-diffusion).
 *   Now dither K separate from CMY.
 *
 *   Revision 1.6  1997/07/30  20:33:05  mike
 *   Final changes for 1.1 release.
 *
 *   Revision 1.5  1997/07/26  18:43:04  mike
 *   Fixed dither_black and dither_cmyk - wasn't clearing extra bits
 *   (caused problems with A3/A4 size output).
 *
 *   Revision 1.5  1997/07/26  18:43:04  mike
 *   Fixed dither_black and dither_cmyk - wasn't clearing extra bits
 *   (caused problems with A3/A4 size output).
 *
 *   Revision 1.4  1997/07/02  18:46:26  mike
 *   Fixed stupid bug in dither_black() - wasn't comparing against gray
 *   pixels (comparing against the first black byte - d'oh!)
 *   Changed 255 in dither matrix to 254 to shade correctly.
 *
 *   Revision 1.4  1997/07/02  18:46:26  mike
 *   Fixed stupid bug in dither_black() - wasn't comparing against gray
 *   pixels (comparing against the first black byte - d'oh!)
 *   Changed 255 in dither matrix to 254 to shade correctly.
 *
 *   Revision 1.3  1997/07/02  13:51:53  mike
 *   Added rgb_to_rgb and gray_to_gray conversion functions.
 *   Standardized calling args to conversion functions.
 *
 *   Revision 1.2  1997/07/01  19:28:44  mike
 *   Updated dither matrix.
 *   Fixed scaling bugs in dither_*() functions.
 *
 *   Revision 1.1  1997/06/19  02:18:15  mike
 *   Initial revision
 */

/* #define PRINT_DEBUG */


#include "print.h"
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
  length = (dst_width) / 8;

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
#define DE_C 3
#define NU_M 1
#define DE_M 3
#define NU_Y 1
#define DE_Y 3

#define I_RATIO_C NU_C / DE_C
#define I_RATIO_C1 NU_C / (DE_C + NU_C)
#define RATIO_C DE_C / NU_C
#define RATIO_C1 (DE_C + NU_C) / NU_C

#define I_RATIO_M NU_M / DE_M
#define I_RATIO_M1 NU_M / (DE_M + NU_M)
#define RATIO_M DE_M / NU_M
#define RATIO_M1 (DE_M + NU_M) / NU_M

#define I_RATIO_Y NU_Y / DE_Y
#define I_RATIO_Y1 NU_Y / (DE_Y + NU_Y)
#define RATIO_Y DE_Y / NU_Y
#define RATIO_Y1 (DE_Y + NU_Y) / NU_Y

/*
 * Lower and upper bounds for mixing CMY with K to produce gray scale.
 * Reducing KDARKNESS_LOWER results in more black being used with relatively
 * light grays, which causes speckling.  Increasing KDARKNESS_UPPER results
 * in more CMY being used in dark tones, which results in less pure black.
 * Decreasing the gap too much results in sharp crossover and stairstepping.
 */
#define KDARKNESS_LOWER (32 * 256)
#define KDARKNESS_UPPER (96 * 256)

/*
 * Randomizing values for deciding when to output a bit.  Normally with the
 * error diffusion algorithm a bit is not output until the accumulated value
 * of the pixel crosses a threshold.  This randomizes the threshold, which
 * results in fewer obnoxious diagonal jaggies in pale regions.  Smaller values
 * result in greater randomizing.  We use less randomness for black output
 * to avoid production of black speckles in light regions.
 */
#define C_RANDOMIZER 2
#define M_RANDOMIZER 2
#define Y_RANDOMIZER 2
#define K_RANDOMIZER 8

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

#define PRINT_D3(r, R, d1, d2)						\
do {									\
  fprintf(dbg, "Case 2: o" #r " %lld " #r				\
	  " %lld ditherbit" #d1 " %d ditherbit" #d2 " %d "		\
	  "num %lld den %lld test1 %lld test2 %lld\n",			\
	  o##r, r, ditherbit##d1, ditherbit##d2,			\
	  o##r, 65536ll,						\
	  ((32767 + (((long long) ditherbit##d2 / 1) - 32768)) * o##r /	\
	   65536), cutoff);						\
} while (0)

#define PRINT_D4(r, R, d1, d2)						\
do {									\
  fprintf(dbg, "Case 3: o" #r " %lld " #r				\
	  " %lld ditherbit" #d1 " %d ditherbit" #d2 " %d "		\
	  "num %lld den %lld test1 %lld test2 %lld\n",			\
	  o##r, r, ditherbit##d1, ditherbit##d2,			\
	  o##r, 65536ll,						\
	  ((32767 + (((long long) ditherbit##d2 / 1) - 32768)) * o##r /	\
	   65536), cutoff);						\
} while (0)

#else /* !PRINT_DEBUG */

#define UPDATE_COLOR_DBG(r) do {} while (0)
#define PRINT_D1(r, R, d1, d2) do {} while (0)
#define PRINT_D2(r, R, d1, d2) do {} while (0)
#define PRINT_D3(r, R, d1, d2) do {} while (0)
#define PRINT_D4(r, R, d1, d2) do {} while (0)

#endif

#define UPDATE_COLOR(r)				\
do {						\
  o##r = r;					\
  r += dither##r / 8;				\
  UPDATE_COLOR_DBG(r);				\
} while (0)

#define PRINT_COLOR(color, r, R, d1, d2)				     \
do {									     \
  if (!l##color)							     \
    {									     \
      if (r > (32767 + (((long long) ditherbit##d2 / R##_RANDOMIZER) -	     \
			(32768 / R##_RANDOMIZER))))			     \
	{								     \
	  PRINT_D1(r, R, d1, d2);					     \
	  if (r##bits++ % horizontal_overdensity == 0)			     \
	    if (! (*kptr & bit))					     \
	      *r##ptr |= bit;						     \
	  r -= 65535;							     \
	}								     \
    }									     \
  else									     \
    {									     \
      if (r <= (65536 * I_RATIO_##R##1 * 2 / 3))			     \
	{								     \
	  if (r > (32767 + (((long long) ditherbit##d2 / R##_RANDOMIZER) -   \
			    (32768 / R##_RANDOMIZER))) * I_RATIO_##R##1)     \
	    {								     \
	      PRINT_D2(r, R, d1, d2);					     \
	      if (l##r##bits++ % horizontal_overdensity == 0)		     \
		if (! (*kptr & bit))					     \
		  *l##r##ptr |= bit;					     \
	      r -= 65535 * I_RATIO_##R##1;				     \
	    }								     \
	}								     \
      else if (r > (32767 + (((long long) ditherbit##d2 / R##_RANDOMIZER) -  \
			     (32768 / R##_RANDOMIZER))) * I_RATIO_##R##1)    \
	{								     \
	  int cutoff = ((density - (65536 * I_RATIO_##R##1 * 2 / 3)) *	     \
			65536 / (65536 - (65536 * I_RATIO_##R##1 * 2 / 3))); \
	  long long sub = (65535ll * I_RATIO_##R##1) +			     \
	    ((65535ll - (65535ll * I_RATIO_##R##1)) * cutoff / 65536);	     \
	  if (ditherbit##d1 > cutoff)					     \
	    {								     \
	      PRINT_D3(r, R, d1, d2);					     \
	      if (l##r##bits++ % horizontal_overdensity == 0)		     \
		if (! (*kptr & bit))					     \
		  *l##r##ptr |= bit;					     \
	    }								     \
	  else								     \
	    {								     \
	      PRINT_D4(r, R, d1, d2);					     \
	      if (r##bits++ % horizontal_overdensity == 0)		     \
		if (! (*kptr & bit))					     \
		  *r##ptr |= bit;					     \
	    }								     \
	  if (sub < (65535 * I_RATIO_##R##1))				     \
	    r -= (65535 * I_RATIO_##R##1);				     \
	  else if (sub > 65535)						     \
	    r -= 65535;							     \
	  else								     \
	    r -= sub;							     \
	}								     \
    }									     \
} while (0)

#if 1
#define UPDATE_DITHER(r, d2, x, width)		\
do {						\
  if (ditherbit##d2 & bit)			\
    {						\
      if (x > 0)				\
	r##error1[-1] += r;			\
      else					\
	r##error1[0] = r;			\
      r##error1[0] += 3 * r;			\
      r##error1[1] = r;				\
      dither##r    = r##error0[1] + 3 * r;	\
    }						\
  else						\
    {						\
      if (x > 0)				\
	r##error1[-1] += r * 3 / 4;		\
      else					\
	r##error1[0] = r * 3 / 4;		\
      r##error1[0] +=  r * 3 / 2;		\
      r##error1[1] = r * 3 / 4;			\
      dither##r    = r##error0[1] + 5 * r;	\
    }						\
} while (0)  
#else
#define UPDATE_DITHER(r, d2, x, width)		\
do {						\
  if (ditherbit##d2 & bit)			\
    {						\
      r##error1[0] = 5 * r;			\
      dither##r    = r##error0[1] + 3 * r;	\
    }						\
  else						\
    {						\
      r##error1[0] = 3 * r;			\
      dither##r    = r##error0[1] + 5 * r;	\
    }						\
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
  static int cbits = 0;
  static int mbits = 0;
  static int ybits = 0;
  static int kbits = 0;
  static int lcbits = 0;
  static int lmbits = 0;
  static int lybits = 0;

#ifdef PRINT_DEBUG
  long long odk, odc, odm, ody, dk, dc, dm, dy, xk, xc, xm, xy, yc, ym, yy;
  FILE *dbg;
#endif

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
  for (x = 0, bit = 128,
	 cptr = cyan, mptr = magenta, yptr = yellow, lcptr = lcyan,
	 lmptr = lmagenta, lyptr = lyellow, kptr = black, xerror = 0,
	 ditherbit = rand(),
	 ditherc = cerror0[0], ditherm = merror0[0], dithery = yerror0[0],
	 ditherk = kerror0[0],
	 ditherbit0 = ditherbit & 0xffff,
	 ditherbit1 = ((ditherbit >> 8) & 0xffff),
	 ditherbit2 = (((ditherbit >> 16) & 0x7fff) +
		       ((ditherbit & 0x100) << 7)),
	 ditherbit3 = (((ditherbit >> 24) & 0x7f) + ((ditherbit & 1) << 7) +
		       ((ditherbit >> 8) & 0xff00));
       x < dst_width;
       x ++, cerror0 ++, cerror1 ++, merror0 ++, merror1 ++, yerror0 ++,
           yerror1 ++, kerror0 ++, kerror1 ++)
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
      long long xdiff = (abs(c - m) + abs(c - y) + abs(m - y)) / 3;

      diff = 65536 - xdiff;
      diff = diff * diff * diff / (65536ll * 65536ll); /* diff = diff^3 */
      diff--;
      if (diff < 0)
	diff = 0;
      k    = diff * k / 65535ll;
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

        c  = (65535 - (unsigned) rgb[1] / 4) * (c - k) / divk;
        m  = (65535 - (unsigned) rgb[2] / 4) * (m - k) / divk;
        y  = (65535 - (unsigned) rgb[0] / 4) * (y - k) / divk;
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
      kdarkness = MAX((c + c / 3 + m + 2 * y / 3) / 4, ak);
/*
      kdarkness = ak;
*/
      if (kdarkness < KDARKNESS_UPPER)
	{
	  int rb;
/*
	  ub = KDARKNESS_UPPER - kdarkness;
	  lb = ub * KDARKNESS_LOWER / KDARKNESS_UPPER;
*/
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
	  c += ck * 10 / 8;
	  m += ck * 19 / 16;
	  y += ck * 3 / 2;
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
      if (k > (32767 + ((ditherbit0 / K_RANDOMIZER) - (32768 / K_RANDOMIZER))))
	{
	  if (kbits++ % horizontal_overdensity == 0)
	    *kptr |= bit;
	  k -= 65535;
	}

      UPDATE_DITHER(k, 1, x, src_width);
#if 0
      if (ditherbit0 & bit)
	{
	  if (x > 0)
	    kerror1[-1] += k;
	  else
	    kerror1[0] = k;
	  kerror1[0] += 2 * k;
	  kerror1[1] = k;
	  ditherk    = kerror0[1] + 3 * k;
	}
      else
	{
	  if (x > 0)
	    kerror1[-1] += k / 2;
	  else
	    kerror1[0] = k / 2;
	  kerror1[0] += k;
	  kerror1[1] = k / 2;
	  ditherk    = kerror0[1] + 5 * k;
	}
#endif
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


    UPDATE_COLOR(c);
    UPDATE_COLOR(m);
    UPDATE_COLOR(y);
    density = (c + m + y) / horizontal_overdensity;

    /*****************************************************************
     * Cyan
     *****************************************************************/
    if (! (*kptr & bit))
      PRINT_COLOR(cyan, c, C, 1, 2);
    UPDATE_DITHER(c, 2, x, dst_width);

    /*****************************************************************
     * Magenta
     *****************************************************************/
    if (! (*kptr & bit))
      PRINT_COLOR(magenta, m, M, 2, 3);
    UPDATE_DITHER(m, 3, x, dst_width);

    /*****************************************************************
     * Yellow
     *****************************************************************/
    if (! (*kptr & bit))
      PRINT_COLOR(yellow, y, Y, 3, 0);
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
#endif
	    
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
	ditherbit = rand();
	ditherbit0 = ditherbit & 0xffff;
	ditherbit1 = ((ditherbit >> 8) & 0xffff);
	ditherbit2 = ((ditherbit >> 16) & 0x7fff) + ((ditherbit & 0x100) << 7);
	ditherbit3 = ((ditherbit >> 24) & 0x7f) + ((ditherbit & 1) << 7) +
	  ((ditherbit >> 8) & 0xff00);
	bit       = 128;
      }
    else
      {
	ditherbit = rand();
	ditherbit0 = ditherbit & 0xffff;
	ditherbit1 = ((ditherbit >> 8) & 0xffff);
	ditherbit2 = ((ditherbit >> 16) & 0x7fff) + ((ditherbit & 0x100) << 7);
	ditherbit3 = ((ditherbit >> 24) & 0x7f) + ((ditherbit & 1) << 7) +
	  ((ditherbit >> 8) & 0xff00);
#if 0
	int dithertmp0 = (ditherbit1 >> 14) ^ ((ditherbit3 &0x3fff) << 2);
	int dithertmp1 = (ditherbit2 >> 14) ^ ((ditherbit2 &0x3fff) << 2);
	int dithertmp2 = (ditherbit3 >> 14) ^ ((ditherbit1 &0x3fff) << 2);
	int dithertmp3 = (ditherbit0 >> 14) ^ ((ditherbit0 &0x3fff) << 2);
	ditherbit0 = dithertmp0;
	ditherbit1 = dithertmp1;
	ditherbit2 = dithertmp2;
	ditherbit3 = dithertmp3;
#endif
	bit >>= 1;
      }

    rgb    += xstep;
    xerror += xmod;
    if (xerror >= dst_width)
    {
      xerror -= dst_width;
      rgb    += 3;
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

/* rgb/hsv conversions taken from Gimp common/autostretch_hsv.c */


static void
calc_rgb_to_hsv(unsigned short *rgb, double *hue, double *sat, double *val)
{
  double red, green, blue;
  double h, s, v;
  double min, max;
  double delta;

  red   = rgb[0] / 65535.0;
  green = rgb[1] / 65535.0;
  blue  = rgb[2] / 65535.0;

  h = 0.0; /* Shut up -Wall */

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

  if (max != 0.0)
    s = (max - min) / max;
  else
    s = 0.0;

  if (s == 0.0)
    h = 0.0;
  else
    {
      delta = max - min;

      if (red == max)
	h = (green - blue) / delta;
      else if (green == max)
	h = 2 + (blue - red) / delta;
      else if (blue == max)
	h = 4 + (red - green) / delta;

      h /= 6.0;

      if (h < 0.0)
	h += 1.0;
      else if (h > 1.0)
	h -= 1.0;
    }

  *hue = h;
  *sat = s;
  *val = v;
}

static void
calc_hsv_to_rgb(unsigned short *rgb, double h, double s, double v)
{
  double hue, saturation, value;
  double f, p, q, t;

  if (s == 0.0)
    {
      h = v;
      s = v;
      v = v; /* heh */
    }
  else
    {
      hue        = h * 6.0;
      saturation = s;
      value      = v;

      if (hue == 6.0)
	hue = 0.0;

      f = hue - (int) hue;
      p = value * (1.0 - saturation);
      q = value * (1.0 - saturation * f);
      t = value * (1.0 - saturation * (1.0 - f));

      switch ((int) hue)
	{
	case 0:
	  h = value;
	  s = t;
	  v = p;
	  break;

	case 1:
	  h = q;
	  s = value;
	  v = p;
	  break;

	case 2:
	  h = p;
	  s = value;
	  v = t;
	  break;

	case 3:
	  h = p;
	  s = q;
	  v = value;
	  break;

	case 4:
	  h = t;
	  s = p;
	  v = value;
	  break;

	case 5:
	  h = value;
	  s = p;
	  v = q;
	  break;
	}
    }

  rgb[0] = h*65535;
  rgb[1] = s*65535;
  rgb[2] = v*65535;
  
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
      double h, s, v;
      rgb[0] = lut->red[cmap[*indexed * 3 + 0]];
      rgb[1] = lut->green[cmap[*indexed * 3 + 1]];
      rgb[2] = lut->blue[cmap[*indexed * 3 + 2]];
      if (vars->saturation != 1.0)
	{
	  calc_rgb_to_hsv(rgb, &h, &s, &v);
	  s = pow(s, 1.0 / vars->saturation);
	  calc_hsv_to_rgb(rgb, h, s, v);
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
      double h, s, v;
      rgb[0] = lut->red[cmap[indexed[0] * 3 + 0] * indexed[1] / 255 +
		       255 - indexed[1]];
      rgb[1] = lut->green[cmap[indexed[0] * 3 + 1] * indexed[1] / 255 +
			 255 - indexed[1]];
      rgb[2] = lut->blue[cmap[indexed[0] * 3 + 2] * indexed[1] / 255 +
			255 - indexed[1]];
      if (vars->saturation != 1.0)
	{
	  calc_rgb_to_hsv(rgb, &h, &s, &v);
	  s = pow(s, 1.0 / vars->saturation);
	  calc_hsv_to_rgb(rgb, h, s, v);
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
      double h, s, v;
      rgbout[0] = lut->red[rgbin[0]];
      rgbout[1] = lut->green[rgbin[1]];
      rgbout[2] = lut->blue[rgbin[2]];
      if (vars->saturation != 1.0 || vars->contrast != 100)
	{
	  calc_rgb_to_hsv(rgbout, &h, &s, &v);
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
	  calc_hsv_to_rgb(rgbout, h, s, v);
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
      double h, s, v;
      rgbout[0] = lut->red[rgbin[0] * rgbin[3] / 255 + 255 - rgbin[3]];
      rgbout[1] = lut->green[rgbin[1] * rgbin[3] / 255 + 255 - rgbin[3]];
      rgbout[2] = lut->blue[rgbin[2] * rgbin[3] / 255 + 255 - rgbin[3]];
      if (vars->saturation != 1.0 || vars->contrast != 100 ||
	  vars->density != 1.0)
	{
	  calc_rgb_to_hsv(rgbout, &h, &s, &v);
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
	  calc_hsv_to_rgb(rgbout, h, s, v);
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
#if 0
	  if (red > 1.0)
	    red_pixel = 65536.0 + ((pixel - 65536.0) / red);
	  else
	    red_pixel = pixel * red;
	  if (green > 1.0)
	    green_pixel = 65536.0 + ((pixel - 65536.0) / green);
	  else
	    green_pixel = pixel * green;
	  if (blue > 1.0)
	    blue_pixel = 65536.0 + ((pixel - 65536.0) / blue);
	  else
	    blue_pixel = pixel * blue;
#endif

	  if (pixel <= 0.0)
	    {
	      lut->composite[i] = 0;
	    }
	  else if (pixel >= 65535.0)
	    {
	      lut->composite[i] = 65535;
	    }
	  else
	    {
	      lut->composite[i] = (unsigned)(pixel + 0.5);
	    }

	  if (red_pixel <= 0.0)
	    {
	      lut->red[i] = 0;
	    }
	  else if (red_pixel >= 65535.0)
	    {
	      lut->red[i] = 65535;
	    }
	  else
	    {
	      lut->red[i] = (unsigned)(red_pixel + 0.5);
	    }

	  if (green_pixel <= 0.0)
	    {
	      lut->green[i] = 0;
	    }
	  else if (green_pixel >= 65535.0)
	    {
	      lut->green[i] = 65535;
	    }
	  else
	    {
	      lut->green[i] = (unsigned)(green_pixel + 0.5);
	    }

	  if (blue_pixel <= 0.0)
	    {
	      lut->blue[i] = 0;
	    }
	  else if (blue_pixel >= 65535.0)
	    {
	      lut->blue[i] = 65535;
	    }
	  else
	    {
	      lut->blue[i] = (unsigned)(blue_pixel + 0.5);
	    }
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
