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
 *   gray_to_gray()       - Convert grayscale image data to grayscale.
 *   indexed_to_gray()    - Convert indexed image data to grayscale.
 *   indexed_to_rgb()     - Convert indexed image data to RGB.
 *   rgb_to_gray()        - Convert RGB image data to grayscale.
 *   rgb_to_rgb()         - Convert RGB image data to RGB.
 *   default_media_size() - Return the size of a default page size.
 *
 * Revision History:
 *
 *   See ChangeLog
 */

/* #define PRINT_DEBUG */


#include "print.h"
#include <math.h>

#ifndef __GNUC__
#  define inline
#endif /* !__GNUC__ */

/*
 * EGCS 1.1.2/gcc 2.91 seems to have some particularly nasty inlining bugs
 */
#if (__GNUC__ == 2 && __GNUC_MINOR__ == 91)
#define inline
#endif

/*
 * RGB to grayscale luminance constants...
 */

#define LUM_RED		31
#define LUM_GREEN	61
#define LUM_BLUE	8

/* rgb/hsv conversions taken from Gimp common/autostretch_hsv.c */


#define FMAX(a, b) ((a) > (b) ? (a) : (b))
#define FMIN(a, b) ((a) < (b) ? (a) : (b))

static inline void
calc_rgb_to_hsl(unsigned short *rgb, double *hue, double *sat,
		double *lightness)
{
  double red, green, blue;
  double h, s, l;
  double min, max;
  double delta;

  red   = rgb[0] / 65535.0;
  green = rgb[1] / 65535.0;
  blue  = rgb[2] / 65535.0;

  if (red > green)
    {
      max = FMAX(red, blue);
      min = FMIN(green, blue);
    }
  else
    {
      max = FMAX(green, blue);
      min = FMIN(red, blue);
    }

  l = (max + min) / 2.0;

  if (max == min)
    {
      s = 0.0;
      h = 0.0;
    }
  else
    {
      delta = max - min;

      if (l <= .5)
	s = delta / (max + min);
      else
	s = delta / (2 - max - min);

      if (red == max)
	h = (green - blue) / delta;
      else if (green == max)
	h = 2 + (blue - red) / delta;
      else
	h = 4 + (red - green) / delta;

      h /= 6.0;

      if (h < 0.0)
	h += 1.0;
      else if (h > 1.0)
	h -= 1.0;
    }

  *hue = h;
  *sat = s;
  *lightness = l;
}

static inline double
hsl_value(double n1, double n2, double hue)
{
  if (hue < 0)
    hue += 1.0;
  else if (hue > 1)
    hue -= 1.0;
  if (hue < (1.0 / 6.0))
    return (n1 + (n2 - n1) * (hue * 6.0));
  else if (hue < .5)
    return (n2);
  else if (hue < (4.0 / 6.0))
    return (n1 + (n2 - n1) * (((4.0 / 6.0) - hue) * 6.0));
  else
    return (n1);
}

static inline void
calc_hsl_to_rgb(unsigned short *rgb, double h, double s, double l)
{
  if (s == 0.0)
    {
      if (l > 1)
	l = 1;
      else if (l < 0)
	l = 0;
      rgb[0] = l * 65535;
      rgb[1] = l * 65535;
      rgb[2] = l * 65535;
    }
  else
    {
      double m1, m2;
      double h1 = h + (2.0 / 6.0);
      double h2 = h - (2.0 / 6.0);

      if (l < .5)
	m2 = l * (1 + s);
      else
	m2 = l + s - (l * s);
      m1 = (l * 2) - m2;
      rgb[0] = 65535 * hsl_value(m1, m2, h1);
      rgb[1] = 65535 * hsl_value(m1, m2, h);
      rgb[2] = 65535 * hsl_value(m1, m2, h2);
    }
}

static inline void
update_cmyk(unsigned short *rgb)
{
  unsigned c = 65535 - rgb[0];
  unsigned m = 65535 - rgb[1];
  unsigned y = 65535 - rgb[2];
  unsigned nc, nm, ny;
  unsigned k;
  if (c == m && c == y)
    return;
  k = FMIN(FMIN(c, m), y);

  /*
   * Soften up the CMY primaries
   */

  nc = (c * 4 + FMIN(c, FMAX(m, y)) * 3 + k) / 8;
  nm = (m * 4 + FMIN(m, FMAX(c, y)) * 3 + k) / 8;
  ny = (y * 4 + FMIN(y, FMAX(c, m)) * 3 + k) / 8;

  /*
   * Make sure we didn't go overboard.  We don't want to go too
   * close to white unnecessarily.
   */
  nc = c + (nc - c) / 2;
  nm = m + (nm - m) / 2;
  ny = y + (ny - y) / 2;

  rgb[0] = 65535 - nc;
  rgb[1] = 65535 - nm;
  rgb[2] = 65535 - ny;
}

static inline unsigned short
lookup_value(unsigned short value, int lut_size, unsigned short *lut)
{
  unsigned shiftval;
  switch (lut_size)
    {
    case 65536:
      return value;
      break;
    case 16:
      shiftval = 4;
      break;
    case 32:
      shiftval = 5;
      break;
    case 64:
      shiftval = 6;
      break;
    case 128:
      shiftval = 7;
      break;
    case 256:
      shiftval = 8;
      break;
    case 512:
      shiftval = 9;
      break;
    case 1024:
      shiftval = 10;
      break;
    case 2048:
      shiftval = 11;
      break;
    case 4096:
      shiftval = 12;
      break;
    case 8192:
      shiftval = 13;
      break;
    case 16384:
      shiftval = 14;
      break;
    case 32768:
      shiftval = 15;
      break;
    default:
      {
	unsigned subrange = value / lut_size;
	unsigned remainder = value % lut_size;
	unsigned below = lut[subrange];
	unsigned above;
	if (subrange == lut_size - 1)
	  above = 65535;
	else
	  above = lut[subrange + 1];
	if (above == below)
	  return above;
	else
	  return below + ((above - below) * remainder) / (65536 / lut_size);
      }
      break;
    }
  {
    unsigned subrange = value >> shiftval;
    unsigned remainder = value & (lut_size - 1);
    unsigned below = lut[subrange];
    unsigned above;
    if (remainder == 0)
      return below;
    if (subrange == (lut_size - 1))
      above = 65535;
    else
      above = lut[subrange + 1];
    if (above == below)
      return above;
    else
      return below + (((above - below) * remainder) >> (16 - shiftval));
  }
}

/*
 * 'gray_to_gray()' - Convert grayscale image data to grayscale (brightness
 *                    adjusted).
 */

static void
gray_to_gray(unsigned char *grayin,	/* I - RGB pixels */
	     unsigned short *grayout,	/* O - RGB pixels */
	     int    	width,		/* I - Width of row */
	     int    	bpp,		/* I - Bytes-per-pixel in grayin */
	     unsigned char *cmap,	/* I - Colormap (unused) */
	     const vars_t	*vars
	     )
{
  while (width > 0)
    {
      if (bpp == 1)
	/*
	 * No alpha in image...
	 */
	*grayout = vars->lut->composite[grayin[0]];
      else
	*grayout = vars->lut->composite[grayin[0] * grayin[1] / 255 +
				      255 - grayin[1]];
      if (vars->density != 1.0 && vars->image_type != IMAGE_MONOCHROME)
	{
	  float t = ((float) *grayout) / 65536.0;
	  t = (1.0 + ((t - 1.0) * vars->density));
	  if (t < 0.0)
	    t = 0.0;
	  *grayout = (unsigned short) (t * 65536.0);
	}
      grayin += bpp;
      grayout ++;
      width --;
    }
}

/*
 * 'indexed_to_gray()' - Convert indexed image data to grayscale.
 */

static void
indexed_to_gray(unsigned char *indexed,		/* I - Indexed pixels */
		unsigned short *gray,		/* O - Grayscale pixels */
		int    width,			/* I - Width of row */
		int    bpp,			/* I - bpp in indexed */
		unsigned char *cmap,		/* I - Colormap */
		const vars_t   *vars
		)
{
  int		i;
  unsigned char	gray_cmap[256];		/* Grayscale colormap */

  /* Really should precompute this silly thing... */
  for (i = 0; i < 256; i ++, cmap += 3)
    gray_cmap[i] = (cmap[0] * LUM_RED +
		    cmap[1] * LUM_GREEN +
		    cmap[2] * LUM_BLUE) / 100;

  while (width > 0)
    {
      if (bpp == 1)
	/*
	 * No alpha in image...
	 */

	*gray = vars->lut->composite[gray_cmap[*indexed]];
      else
	*gray = vars->lut->composite[gray_cmap[indexed[0] * indexed[1] / 255]
				   + 255 - indexed[1]];
      if (vars->density != 1.0 && vars->image_type != IMAGE_MONOCHROME)
	{
	  float t = ((float) *gray) / 65536.0;
	  t = (1.0 + ((t - 1.0) * vars->density));
	  if (t < 0.0)
	    t = 0.0;
	  *gray = (unsigned short) (t * 65536.0);
	}
      indexed += bpp;
      gray ++;
      width --;
    }
}

/*
 * 'rgb_to_gray()' - Convert RGB image data to grayscale.
 */

static void
rgb_to_gray(unsigned char *rgb,		/* I - RGB pixels */
	    unsigned short *gray,	/* O - Grayscale pixels */
	    int    width,		/* I - Width of row */
	    int    bpp,			/* I - Bytes-per-pixel in RGB */
	    unsigned char *cmap,	/* I - Colormap (unused) */
	    const vars_t   *vars
	    )
{
  while (width > 0)
    {
      if (bpp == 3)
	/*
	 * No alpha in image...
	 */
	*gray = vars->lut->composite[(rgb[0] * LUM_RED +
				     rgb[1] * LUM_GREEN +
				     rgb[2] * LUM_BLUE) / 100];
      else
	*gray = vars->lut->composite[((rgb[0] * LUM_RED +
				      rgb[1] * LUM_GREEN +
				      rgb[2] * LUM_BLUE) *
				     rgb[3] / 25500 + 255 - rgb[3])];
      if (vars->density != 1.0 && vars->image_type != IMAGE_MONOCHROME)
	{
	  float t = ((float) *gray) / 65536.0;
	  t = (1.0 + ((t - 1.0) * vars->density));
	  if (t < 0.0)
	    t = 0.0;
	  *gray = (unsigned short) (t * 65536.0);
	}
      rgb += bpp;
      gray ++;
      width --;
    }
}

/*
 * 'rgb_to_rgb()' - Convert rgb image data to RGB.
 */

static void
rgb_to_rgb(unsigned char	*rgbin,		/* I - RGB pixels */
	   unsigned short 	*rgbout,	/* O - RGB pixels */
	   int    		width,		/* I - Width of row */
	   int    		bpp,		/* I - Bytes/pix in indexed */
	   unsigned char 	*cmap,		/* I - Colormap */
	   const vars_t  	*vars
	   )
{
  unsigned ld = vars->density * 65536;
  double isat = 1.0;
  double ssat = sqrt(vars->saturation * 2);
  if (ssat > 1)
    isat = 1.0 / ssat;
  while (width > 0)
    {
      double h, s, v;
      switch (bpp)
	{
	case 1:
	  /*
	   * No alpha in image, using colormap...
	   */

	  rgbout[0] = cmap[*rgbin * 3 + 0] * 257;
	  rgbout[1] = cmap[*rgbin * 3 + 1] * 257;
	  rgbout[2] = cmap[*rgbin * 3 + 2] * 257;
	  break;
	case 2:
	  rgbout[0] = (cmap[rgbin[0] * 3 + 0] *
		       rgbin[1] / 255 + 255 - rgbin[1]) * 257;
	  rgbout[1] = (cmap[rgbin[1] * 3 + 0] *
		     rgbin[1] / 255 + 255 - rgbin[1]) * 257;
	  rgbout[2] = (cmap[rgbin[2] * 3 + 0] *
		     rgbin[1] / 255 + 255 - rgbin[1]) * 257;
	  break;
	case 3:
	  /*
	   * No alpha in image...
	   */
	  rgbout[0] = rgbin[0] * 257;
	  rgbout[1] = rgbin[1] * 257;
	  rgbout[2] = rgbin[2] * 257;
	  break;
	case 4:
	  rgbout[0] = (rgbin[0] * rgbin[3] / 255 + 255 - rgbin[3]) * 257;
	  rgbout[1] = (rgbin[1] * rgbin[3] / 255 + 255 - rgbin[3]) * 257;
	  rgbout[2] = (rgbin[2] * rgbin[3] / 255 + 255 - rgbin[3]) * 257;
	  break;
	}
      if (ssat != 1.0)
	{
	  rgbout[0] = 65535 - rgbout[0];
	  rgbout[1] = 65535 - rgbout[1];
	  rgbout[2] = 65535 - rgbout[2];
	  calc_rgb_to_hsl(rgbout, &h, &s, &v);
	  if (ssat < 1)
	    s *= ssat;
	  else
	    s = pow(s, isat);
	  if (s > 1)
	    s = 1.0;
	  calc_hsl_to_rgb(rgbout, h, s, v);
	  rgbout[0] = 65535 - rgbout[0];
	  rgbout[1] = 65535 - rgbout[1];
	  rgbout[2] = 65535 - rgbout[2];
	}
      update_cmyk(rgbout);	/* Fiddle with the INPUT */
      rgbout[0] = lookup_value(rgbout[0], vars->lut->steps, vars->lut->red);
      rgbout[1] = lookup_value(rgbout[1], vars->lut->steps, vars->lut->green);
      rgbout[2] = lookup_value(rgbout[2], vars->lut->steps, vars->lut->blue);
      if (ssat != 1.0)
	{
	  rgbout[0] = 65535 - rgbout[0];
	  rgbout[1] = 65535 - rgbout[1];
	  rgbout[2] = 65535 - rgbout[2];
	  calc_rgb_to_hsl(rgbout, &h, &s, &v);
	  if (ssat < 1)
	    s *= ssat;
	  else
	    s = pow(s, isat);
	  if (s > 1)
	    s = 1.0;
	  calc_hsl_to_rgb(rgbout, h, s, v);
	  rgbout[0] = 65535 - rgbout[0];
	  rgbout[1] = 65535 - rgbout[1];
	  rgbout[2] = 65535 - rgbout[2];
	}
      if (ld < 65536)
	{
	  int i;
	  for (i = 0; i < 3; i++)
	    {
	      unsigned t = rgbout[i];
	      t = 65535 - (65535 - t) * ld / 65536;
	      rgbout[i] = (unsigned short) t;
	    }
	}
      rgbin += bpp;
      rgbout += 3;
      width --;
    }
}

static void
indexed_to_rgb(unsigned char *indexed,	/* I - Indexed pixels */
	       unsigned short *rgb,	/* O - RGB pixels */
	       int    width,		/* I - Width of row */
	       int    bpp,		/* I - Bytes-per-pixel in indexed */
	       unsigned char *cmap,	/* I - Colormap */
	       const vars_t   *vars
	       )
{
  rgb_to_rgb(indexed, rgb, width, bpp, cmap, vars);
}

/*
 * 'gray_to_rgb()' - Convert gray image data to RGB.
 */

static void
gray_to_rgb(unsigned char	*grayin,	/* I - grayscale pixels */
	    unsigned short 	*rgbout,	/* O - RGB pixels */
	    int    		width,		/* I - Width of row */
	    int    		bpp,		/* I - Bytes/pix in indexed */
	    unsigned char 	*cmap,		/* I - Colormap */
	    const vars_t  	*vars
	    )
{
  while (width > 0)
    {
      unsigned short trgb[3];
      if (bpp == 1)
	{
	  /*
	   * No alpha in image...
	   */
	  trgb[0] = grayin[0] * 257;
	  trgb[1] = grayin[0] * 257;
	  trgb[2] = grayin[0] * 257;
	}
      else
	{
	  int lookup = (grayin[0] * grayin[1] / 255 + 255 - grayin[1]) * 257;
	  trgb[0] = lookup;
	  trgb[1] = lookup;
	  trgb[2] = lookup;
	}
      update_cmyk(trgb);
      rgbout[0] = lookup_value(trgb[0], vars->lut->steps, vars->lut->red);
      rgbout[1] = lookup_value(trgb[1], vars->lut->steps, vars->lut->green);
      rgbout[2] = lookup_value(trgb[2], vars->lut->steps, vars->lut->blue);
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
      grayin += bpp;
      rgbout += 3;
      width --;
    }
}

static void
fast_indexed_to_rgb(unsigned char *indexed,	/* I - Indexed pixels */
		    unsigned short *rgb,	/* O - RGB pixels */
		    int    width,		/* I - Width of row */
		    int    bpp,		/* I - Bytes-per-pixel in indexed */
		    unsigned char *cmap,	/* I - Colormap */
		    const vars_t   *vars
		    )
{
  double isat = 1.0;
  if (vars->saturation > 1)
    isat = 1.0 / vars->saturation;
  while (width > 0)
    {
      double h, s, v;
      if (bpp == 1)
	{
	  /*
	   * No alpha in image...
	   */

	  rgb[0] = vars->lut->red[cmap[*indexed * 3 + 0]];
	  rgb[1] = vars->lut->green[cmap[*indexed * 3 + 1]];
	  rgb[2] = vars->lut->blue[cmap[*indexed * 3 + 2]];
	}
      else
	{
	  rgb[0] = vars->lut->red[cmap[indexed[0] * 3 + 0] * indexed[1] / 255
				+ 255 - indexed[1]];
	  rgb[1] = vars->lut->green[cmap[indexed[0] * 3 + 1] * indexed[1] / 255
				  + 255 - indexed[1]];
	  rgb[2] = vars->lut->blue[cmap[indexed[0] * 3 + 2] * indexed[1] / 255
				 + 255 - indexed[1]];
	}
      if (vars->saturation != 1.0)
	{
	  calc_rgb_to_hsl(rgb, &h, &s, &v);
	  if (vars->saturation < 1)
	    s *= vars->saturation;
	  else
	    s = pow(s, isat);
	  if (s > 1)
	    s = 1.0;
	  calc_hsl_to_rgb(rgb, h, s, v);
	}
      if (vars->density != 1.0)
	{
	  float t;
	  int i;
	  for (i = 0; i < 3; i++)
	    {
	      t = ((float) rgb[i]) / 65536.0;
	      t = (1.0 + ((t - 1.0) * vars->density));
	      if (t < 0.0)
		t = 0.0;
	      rgb[i] = (unsigned short) (t * 65536.0);
	    }
	}
      indexed += bpp;
      rgb += 3;
      width --;
    }
}

/*
 * 'rgb_to_rgb()' - Convert rgb image data to RGB.
 */

static void
fast_rgb_to_rgb(unsigned char	*rgbin,		/* I - RGB pixels */
		unsigned short 	*rgbout,	/* O - RGB pixels */
		int    		width,		/* I - Width of row */
		int    		bpp,		/* I - Bytes/pix in indexed */
		unsigned char 	*cmap,		/* I - Colormap */
		const vars_t  	*vars
		)
{
  unsigned ld = vars->density * 65536;
  double isat = 1.0;
  if (vars->saturation > 1)
    isat = 1.0 / vars->saturation;
  while (width > 0)
    {
      double h, s, v;
      if (bpp == 3)
	{
	  /*
	   * No alpha in image...
	   */
	  rgbout[0] = vars->lut->red[rgbin[0]];
	  rgbout[1] = vars->lut->green[rgbin[1]];
	  rgbout[2] = vars->lut->blue[rgbin[2]];
	}
      else
	{
	  rgbout[0] = vars->lut->red[rgbin[0] * rgbin[3] / 255 +
				   255 - rgbin[3]];
	  rgbout[1] = vars->lut->green[rgbin[1] * rgbin[3] / 255 +
				     255 - rgbin[3]];
	  rgbout[2] = vars->lut->blue[rgbin[2] * rgbin[3] / 255 +
				    255 - rgbin[3]];
	}
      if (vars->saturation != 1.0)
	{
	  calc_rgb_to_hsl(rgbout, &h, &s, &v);
	  if (vars->saturation < 1)
	    s *= vars->saturation;
	  else
	    s = pow(s, isat);
	  if (s > 1)
	    s = 1.0;
	  calc_hsl_to_rgb(rgbout, h, s, v);
	}
      if (ld < 65536)
	{
	  int i;
	  for (i = 0; i < 3; i++)
	    {
	      unsigned t = rgbout[i];
	      t = 65535 - (65535 - t) * ld / 65536;
	      rgbout[i] = (unsigned short) t;
	    }
	}
      rgbin += bpp;
      rgbout += 3;
      width --;
    }
}

/*
 * 'gray_to_rgb()' - Convert gray image data to RGB.
 */

static void
fast_gray_to_rgb(unsigned char	*grayin,	/* I - grayscale pixels */
		 unsigned short *rgbout,	/* O - RGB pixels */
		 int    	width,		/* I - Width of row */
		 int    	bpp,		/* I - Bytes/pix in indexed */
		 unsigned char 	*cmap,		/* I - Colormap */
		 const vars_t  	*vars
		 )
{
  while (width > 0)
    {
      if (bpp == 1)
	{
	  /*
	   * No alpha in image...
	   */

	  rgbout[0] = vars->lut->red[grayin[0]];
	  rgbout[1] = vars->lut->green[grayin[0]];
	  rgbout[2] = vars->lut->blue[grayin[0]];
	}
      else
	{
	  int lookup = (grayin[0] * grayin[1] / 255 +
			255 - grayin[1]);
	  rgbout[0] = vars->lut->red[lookup];
	  rgbout[1] = vars->lut->green[lookup];
	  rgbout[2] = vars->lut->blue[lookup];
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
      grayin += bpp;
      rgbout += 3;
      width --;
    }
}

void
merge_printvars(vars_t *user, const vars_t *print)
{
  user->red = (user->red * print->red) / 100.0;
  user->green = (user->green * print->green) / 100.0;
  user->blue = (user->blue * print->blue) / 100.0;
  user->contrast = (user->contrast * print->contrast) / 100.0;
  user->brightness = (user->brightness * print->brightness) / 100.0;
  user->gamma /= print->gamma;
  user->saturation *= print->saturation;
  user->density *= print->density;
  /* Application gamma comes from the user variables */
}

static lut_t *
allocate_lut(size_t steps)
{
  lut_t *ret = malloc(sizeof(lut_t));
  ret->steps = steps;
  ret->composite = malloc(sizeof(unsigned short) * steps);
  ret->red = malloc(sizeof(unsigned short) * steps);
  ret->green = malloc(sizeof(unsigned short) * steps);
  ret->blue = malloc(sizeof(unsigned short) * steps);
  return ret;
}

void
free_lut(vars_t *v)
{
  if (v->lut)
    {
      if (v->lut->composite)
	free(v->lut->composite);
      if (v->lut->red)
	free(v->lut->red);
      if (v->lut->green)
	free(v->lut->green);
      if (v->lut->blue)
	free(v->lut->blue);
      v->lut->steps = 0;
      v->lut->composite = NULL;
      v->lut->red = NULL;
      v->lut->green = NULL;
      v->lut->blue = NULL;
      free(v->lut);
    }
  v->lut = NULL;
}

/* #define PRINT_LUT */

void
compute_lut(size_t steps, vars_t *uv)
{
  float		pixel,		/* Pixel value */
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

  float red = 100.0 / uv->red;
  float green = 100.0 / uv->green;
  float blue = 100.0 / uv->blue;
  float print_gamma = uv->gamma;
  float contrast = uv->contrast / 100.0;
  float app_gamma = uv->app_gamma;
  float brightness = 100.0 / uv->brightness;
  float screen_gamma = app_gamma * brightness / 1.7;

  if (red < 0.01)
    red = 0.01;
  if (green < 0.01)
    green = 0.01;
  if (blue < 0.01)
    blue = 0.01;

  uv->lut = allocate_lut(steps);
  for (i = 0; i < steps; i ++)
    {
      float temp_pixel;
      float fsteps = steps;
      pixel = (float) i / (float) (steps - 1);

      /*
       * First, correct contrast
       */
      if (pixel >= .5)
	temp_pixel = 1.0 - pixel;
      else
	temp_pixel = pixel;
      if (contrast >= 1)
	temp_pixel = .5 * pow(2 * temp_pixel, 1.0 / contrast);
      else
	temp_pixel = temp_pixel * contrast;
      if (pixel < .5)
	pixel = temp_pixel;
      else
	pixel = 1 - temp_pixel;

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

      pixel = fsteps * (fsteps - fsteps *
		       pow(pixel, print_gamma));
      red_pixel = fsteps * (fsteps - fsteps *
			   pow(red_pixel, print_gamma));
      green_pixel = fsteps * (fsteps - fsteps *
			     pow(green_pixel, print_gamma));
      blue_pixel = fsteps * (fsteps - fsteps *
			    pow(blue_pixel, print_gamma));

      if (pixel <= 0.0)
	uv->lut->composite[i] = 0;
      else if (pixel >= 65535.0)
	uv->lut->composite[i] = 65535;
      else
	uv->lut->composite[i] = (unsigned)(pixel);

      if (red_pixel <= 0.0)
	uv->lut->red[i] = 0;
      else if (red_pixel >= 65535.0)
	uv->lut->red[i] = 65535;
      else
	uv->lut->red[i] = (unsigned)(red_pixel);

      if (green_pixel <= 0.0)
	uv->lut->green[i] = 0;
      else if (green_pixel >= 65535.0)
	uv->lut->green[i] = 65535;
      else
	uv->lut->green[i] = (unsigned)(green_pixel);

      if (blue_pixel <= 0.0)
	uv->lut->blue[i] = 0;
      else if (blue_pixel >= 65535.0)
	uv->lut->blue[i] = 65535;
      else
	uv->lut->blue[i] = (unsigned)(blue_pixel);
#ifdef PRINT_LUT
      fprintf(ltfile, "%3i  %5d  %5d  %5d  %5d  %f %f %f %f  %f %f %f  %f\n",
	      i, uv->lut->composite[i], uv->lut->red[i],
	      uv->lut->green[i], uv->lut->blue[i], pixel, red_pixel,
	      green_pixel, blue_pixel, print_gamma, screen_gamma,
	      print_gamma, app_gamma);
#endif
    }

#ifdef PRINT_LUT
  fclose(ltfile);
#endif
}

/*
 * 'default_media_size()' - Return the size of a default page size.
 */

/*
 * Sizes are converted to 1/72in, then rounded down so that we don't
 * print off the edge of the paper.
 */
const static papersize_t paper_sizes[] =
{
  /* Common imperial page sizes */
  { "Postcard", 283,  416, PAPERSIZE_ENGLISH },	/* 100mm x 147mm */
  { "4x6",      288,  432, PAPERSIZE_ENGLISH },
  { "5x7",      360,  504, PAPERSIZE_ENGLISH },
  { "5x8",      360,  576, PAPERSIZE_ENGLISH },
  { "6x8",      432,  576, PAPERSIZE_ENGLISH },
  { "8x10",     576,  720, PAPERSIZE_ENGLISH },
  { "Manual",   396,  612, PAPERSIZE_ENGLISH },	/* 5.5in x 8.5in */
  { "Letter",   612,  792, PAPERSIZE_ENGLISH },	/* 8.5in x 11in */
  { "Legal",    612, 1008, PAPERSIZE_ENGLISH },	/* 8.5in x 14in */
  { "Tabloid",  792, 1224, PAPERSIZE_ENGLISH },	/*  11in x 17in */
  { "12x18",    864, 1296, PAPERSIZE_ENGLISH },
  { "13x19",    936, 1368, PAPERSIZE_ENGLISH },

  /* Other common photographic paper sizes */
  { "8x12",	576,  864, PAPERSIZE_ENGLISH }, /* Sometimes used for 35 mm */
  { "11x14",    792, 1008, PAPERSIZE_ENGLISH },
  { "16x20",   1152, 1440, PAPERSIZE_ENGLISH },
  { "16x24",   1152, 1728, PAPERSIZE_ENGLISH }, /* 20x24 for 35 mm */
  { "20x24",   1440, 1728, PAPERSIZE_ENGLISH },
  { "20x30",   1440, 2160, PAPERSIZE_ENGLISH },	/* 24x30 for 35 mm */
  { "24x30",   1728, 2160, PAPERSIZE_ENGLISH },
  { "24x36",   1728, 2592, PAPERSIZE_ENGLISH }, /* Sometimes used for 35 mm */
  { "30x40",   2160, 2880, PAPERSIZE_ENGLISH },

  /* International Paper Sizes (mostly taken from BS4000:1968) */

  /*
   * "A" series: Paper and boards, trimmed sizes
   *
   * "A" sizes are in the ratio 1 : sqrt(2).  A0 has a total area
   * of 1 square metre.  Everything is rounded to the nearest
   * millimetre.  Thus, A0 is 841mm x 1189mm.  Every other A
   * size is obtained by doubling or halving another A size.
   */
  { "4A",       4767, 6740, PAPERSIZE_METRIC },	/* 1682mm x 2378mm */
  { "2A",       3370, 4767, PAPERSIZE_METRIC },	/* 1189mm x 1682mm */
  { "A0",       2383, 3370, PAPERSIZE_METRIC },	/*  841mm x 1189mm */
  { "A1",       1683, 2383, PAPERSIZE_METRIC },	/*  594mm x  841mm */
  { "A2",       1190, 1683, PAPERSIZE_METRIC },	/*  420mm x  594mm */
  { "A3",        841, 1190, PAPERSIZE_METRIC },	/*  297mm x  420mm */
  { "A4",        595,  841, PAPERSIZE_METRIC },	/*  210mm x  297mm */
  { "A5",        419,  595, PAPERSIZE_METRIC },	/*  148mm x  210mm */
  { "A6",        297,  419, PAPERSIZE_METRIC },	/*  105mm x  148mm */
  { "A7",        209,  297, PAPERSIZE_METRIC },	/*   74mm x  105mm */
  { "A8",        147,  209, PAPERSIZE_METRIC },	/*   52mm x   74mm */
  { "A9",        104,  147, PAPERSIZE_METRIC },	/*   37mm x   52mm */
  { "A10",        73,  104, PAPERSIZE_METRIC },	/*   26mm x   37mm */

  /*
   * Stock sizes for normal trims.
   * Allowance for trim is 3 millimetres.
   */
  { "RA0",      2437, 3458, PAPERSIZE_METRIC },	/*  860mm x 1220mm */
  { "RA1",      1729, 2437, PAPERSIZE_METRIC },	/*  610mm x  860mm */
  { "RA2",      1218, 1729, PAPERSIZE_METRIC },	/*  430mm x  610mm */
  { "RA3",       864, 1218, PAPERSIZE_METRIC },	/*  305mm x  430mm */
  { "RA4",       609,  864, PAPERSIZE_METRIC },	/*  215mm x  305mm */

  /*
   * Stock sizes for bled work or extra trims.
   */
  { "SRA0",     2551, 3628, PAPERSIZE_METRIC },	/*  900mm x 1280mm */
  { "SRA1",     1814, 2551, PAPERSIZE_METRIC },	/*  640mm x  900mm */
  { "SRA2",     1275, 1814, PAPERSIZE_METRIC },	/*  450mm x  640mm */
  { "SRA3",      907, 1275, PAPERSIZE_METRIC },	/*  320mm x  450mm */
  { "SRA4",      637,  907, PAPERSIZE_METRIC },	/*  225mm x  320mm */

  /*
   * "B" series: Posters, wall charts and similar items.
   */
  { "4B ISO",   5669, 8016, PAPERSIZE_METRIC },	/* 2000mm x 2828mm */
  { "2B ISO",   4008, 5669, PAPERSIZE_METRIC },	/* 1414mm x 2000mm */
  { "B0 ISO",   2834, 4008, PAPERSIZE_METRIC },	/* 1000mm x 1414mm */
  { "B1 ISO",   2004, 2834, PAPERSIZE_METRIC },	/*  707mm x 1000mm */
  { "B2 ISO",   1417, 2004, PAPERSIZE_METRIC },	/*  500mm x  707mm */
  { "B3 ISO",   1000, 1417, PAPERSIZE_METRIC },	/*  353mm x  500mm */
  { "B4 ISO",    708, 1000, PAPERSIZE_METRIC },	/*  250mm x  353mm */
  { "B5 ISO",    498,  708, PAPERSIZE_METRIC },	/*  176mm x  250mm */
  { "B6 ISO",    354,  498, PAPERSIZE_METRIC },	/*  125mm x  176mm */
  { "B7 ISO",    249,  354, PAPERSIZE_METRIC },	/*   88mm x  125mm */
  { "B8 ISO",    175,  249, PAPERSIZE_METRIC },	/*   62mm x   88mm */
  { "B9 ISO",    124,  175, PAPERSIZE_METRIC },	/*   44mm x   62mm */
  { "B10 ISO",    87,  124, PAPERSIZE_METRIC },	/*   31mm x   44mm */
  
  { "B0 JIS",   2919, 4127, PAPERSIZE_METRIC },
  { "B1 JIS",   2063, 2919, PAPERSIZE_METRIC },
  { "B2 JIS",   1459, 2063, PAPERSIZE_METRIC },
  { "B3 JIS",   1029, 1459, PAPERSIZE_METRIC },
  { "B4 JIS",    727, 1029, PAPERSIZE_METRIC },
  { "B5 JIS",    518,  727, PAPERSIZE_METRIC },
  { "B6 JIS",    362,  518, PAPERSIZE_METRIC },
  { "B7 JIS",    257,  362, PAPERSIZE_METRIC },
  { "B8 JIS",    180,  257, PAPERSIZE_METRIC },
  { "B9 JIS",    127,  180, PAPERSIZE_METRIC },
  { "B10 JIS",    90,  127, PAPERSIZE_METRIC },

  /*
   * "C" series: Envelopes or folders suitable for A size stationery.
   */
  { "C0",       2599, 3676, PAPERSIZE_METRIC },	/*  917mm x 1297mm */
  { "C1",       1836, 2599, PAPERSIZE_METRIC },	/*  648mm x  917mm */
  { "C2",       1298, 1836, PAPERSIZE_METRIC },	/*  458mm x  648mm */
  { "C3",        918, 1298, PAPERSIZE_METRIC },	/*  324mm x  458mm */
  { "C4",        649,  918, PAPERSIZE_METRIC },	/*  229mm x  324mm */
  { "C5",        459,  649, PAPERSIZE_METRIC },	/*  162mm x  229mm */
  { "B6/C4",     354,  918, PAPERSIZE_METRIC },	/*  125mm x  324mm */
  { "C6",        323,  459, PAPERSIZE_METRIC },	/*  114mm x  162mm */
  { "DL",        311,  623, PAPERSIZE_METRIC },	/*  110mm x  220mm */
  { "C7/6",      229,  459, PAPERSIZE_METRIC },	/*   81mm x  162mm */
  { "C7",        229,  323, PAPERSIZE_METRIC },	/*   81mm x  114mm */
  { "C8",        161,  229, PAPERSIZE_METRIC },	/*   57mm x   81mm */
  { "C9",        113,  161, PAPERSIZE_METRIC },	/*   40mm x   57mm */
  { "C10",        79,  113, PAPERSIZE_METRIC },	/*   28mm x   40mm */

  /*
   * Sizes for book production
   * The BPIF and the Publishers Association jointly recommend ten
   * standard metric sizes for case-bound titles as follows:
   */
  { "Crown Quarto",       535,  697, PAPERSIZE_METRIC }, /* 189mm x 246mm */
  { "Large Crown Quarto", 569,  731, PAPERSIZE_METRIC }, /* 201mm x 258mm */
  { "Demy Quarto",        620,  782, PAPERSIZE_METRIC }, /* 219mm x 276mm */
  { "Royal Quarto",       671,  884, PAPERSIZE_METRIC }, /* 237mm x 312mm */
/*{ "ISO A4",             595,  841, PAPERSIZE_METRIC },    210mm x 297mm */
  { "Crown Octavo",       348,  527, PAPERSIZE_METRIC }, /* 123mm x 186mm */
  { "Large Crown Octavo", 365,  561, PAPERSIZE_METRIC }, /* 129mm x 198mm */
  { "Demy Octavo",        391,  612, PAPERSIZE_METRIC }, /* 138mm x 216mm */
  { "Royal Octavo",       442,  663, PAPERSIZE_METRIC }, /* 156mm x 234mm */
/*{ "ISO A5",             419,  595, PAPERSIZE_METRIC },    148mm x 210mm */

  /* Paperback sizes in common usage */
  { "Small paperback",         314, 504, PAPERSIZE_METRIC }, /* 111mm x 178mm */
  { "Penguin small paperback", 314, 513, PAPERSIZE_METRIC }, /* 111mm x 181mm */
  { "Penguin large paperback", 365, 561, PAPERSIZE_METRIC }, /* 129mm x 198mm */

  { "",           0,    0, PAPERSIZE_METRIC }
};

int
known_papersizes(void)
{
  return sizeof(paper_sizes) / sizeof(papersize_t);
}

const papersize_t *
get_papersizes(void)
{
  return paper_sizes;
}

const papersize_t *
get_papersize_by_name(const char *name)
{
  const papersize_t *val = &(paper_sizes[0]);
  while (strlen(val->name) > 0)
    {
      if (!strcmp(val->name, name))
	return val;
      val++;
    }
  return NULL;
}

const papersize_t *
get_papersize_by_size(int l, int w)
{
  const papersize_t *val = &(paper_sizes[0]);
  while (strlen(val->name) > 0)
    {
      if (val->width == w && val->length == l)
	return val;
      val++;
    }
  return NULL;
}

void
default_media_size(const printer_t *printer,
					/* I - Printer model (not used) */
		   const vars_t *v,	/* I */
        	   int  *width,		/* O - Width in points */
        	   int  *length)	/* O - Length in points */
{
  if (v->page_width > 0 && v->page_height > 0)
    {
      *width = v->page_width;
      *length = v->page_height;
    }
  else
    {
      const papersize_t *papersize = get_papersize_by_name(v->media_size);
      if (!papersize)
	{
	  *width = 1;
	  *length = 1;
	}
      else
	{
	  *width = papersize->width;
	  *length = papersize->length;
	}
    }
}

/*
 * The list of printers has been moved to printers.c
 */
#include "print-printers.c"

int
known_printers(void)
{
  return printer_count;
}

const printer_t *
get_printers(void)
{
  return printers;
}

const printer_t *
get_printer_by_index(int idx)
{
  return &(printers[idx]);
}

const printer_t *
get_printer_by_long_name(const char *long_name)
{
  const printer_t *val = &(printers[0]);
  int i;
  for (i = 0; i < known_printers(); i++)
    {
      if (!strcmp(val->long_name, long_name))
	return val;
      val++;
    }
  return NULL;
}

const printer_t *
get_printer_by_driver(const char *driver)
{
  const printer_t *val = &(printers[0]);
  int i;
  for (i = 0; i < known_printers(); i++)
    {
      if (!strcmp(val->driver, driver))
	return val;
      val++;
    }
  return NULL;
}

int
get_printer_index_by_driver(const char *driver)
{
  int idx = 0;
  const printer_t *val = &(printers[0]);
  for (idx = 0; idx < known_printers(); idx++)
    {
      if (!strcmp(val->driver, driver))
	return idx;
      val++;
    }
  return -1;
}

const char *
default_dither_algorithm(void)
{
  return dither_algo_names[0];
}

convert_t
choose_colorfunc(int output_type,
		 int image_bpp,
		 const unsigned char *cmap,
		 int *out_bpp,
		 const vars_t *v)
{
  if (output_type == OUTPUT_COLOR)
    {
      *out_bpp = 3;

      if (image_bpp >= 3)
	{
	  if (v->image_type == IMAGE_CONTINUOUS)
	    return rgb_to_rgb;
	  else
	    return fast_rgb_to_rgb;
	}
      else
	{
	  if (v->image_type == IMAGE_CONTINUOUS)
	    return indexed_to_rgb;
	  else
	    return fast_indexed_to_rgb;
	}
    }
  else if (output_type == OUTPUT_GRAY_COLOR)
    {
      *out_bpp = 3;
      if (v->image_type == IMAGE_CONTINUOUS)
	return gray_to_rgb;
      else
	return fast_gray_to_rgb;
    }
  else
    {
      *out_bpp = 1;

      if (image_bpp >= 3)
	return rgb_to_gray;
      else if (cmap == NULL)
	return gray_to_gray;
      else
	return indexed_to_gray;
    }
}

void
compute_page_parameters(int page_right,	/* I */
			int page_left, /* I */
			int page_top, /* I */
			int page_bottom, /* I */
			int scaling, /* I */
			int image_width, /* I */
			int image_height, /* I */
			Image image, /* IO */
			int *orientation, /* IO */
			int *page_width, /* O */
			int *page_height, /* O */
			int *out_width,	/* O */
			int *out_height, /* O */
			int *left, /* O */
			int *top) /* O */
{
  *page_width  = page_right - page_left;
  *page_height = page_top - page_bottom;

  /* In AUTO orientation, just orient the paper the same way as the image. */

  if (*orientation == ORIENT_AUTO)
    {
      if ((*page_width >= *page_height && image_width >= image_height)
         || (*page_height >= *page_width && image_height >= image_width))
        *orientation = ORIENT_PORTRAIT;
      else
        *orientation = ORIENT_LANDSCAPE;
    }

  if (*orientation == ORIENT_LANDSCAPE)
    {
      Image_rotate_ccw(image);

      image_width  = Image_width(image);
      image_height = Image_height(image);
    }

  /*
   * Calculate width/height...
   */

  if (scaling < 0.0)
    {
      /*
       * Scale to pixels per inch...
       */

      *out_width  = image_width * -72.0 / scaling;
      *out_height = image_height * -72.0 / scaling;
    }
  else
    {
      /*
       * Scale by percent...
       */

      /*
       * Decide which orientation gives the proper fit
       * If we ask for 50%, we do not want to exceed that
       * in either dimension!
       */

      int twidth0 = *page_width * scaling / 100.0;
      int theight0 = twidth0 * image_height / image_width;
      int theight1 = *page_height * scaling / 100.0;
      int twidth1 = theight1 * image_width / image_height;

      *out_width = FMIN(twidth0, twidth1);
      *out_height = FMIN(theight0, theight1);

      if (*out_height > *page_height)
	{
	  *out_height = *page_height * scaling / 100.0;
	  *out_width  = *out_height * image_width / image_height;
	}
    }

  if (*out_width == 0)
    *out_width = 1;
  if (*out_height == 0)
    *out_height = 1;

  if (*orientation == ORIENT_LANDSCAPE)
    {
      int x;

      /*
       * Swap left/top offsets...
       */

      x     = *left;
      *left = *top;
      *top  = *page_height - x - *out_height;
    }

  if (*left < 0)
    *left = (*page_width - *out_width) / 2;

  if (*top < 0)
    *top  = (*page_height - *out_height) / 2;
}

int
verify_printer_params(const printer_t *p, const vars_t *v)
{
  char **vptr;
  int count;
  int i;
  int answer = 1;

  if (strlen(v->media_size) > 0)
    {
      vptr = (*p->parameters)(p, NULL, "PageSize", &count);
      if (count > 0)
	{
	  for (i = 0; i < count; i++)
	    if (!strcmp(v->media_size, vptr[i]))
	      goto good_page_size;
	  answer = 0;
	  fprintf(stderr, "%s is not a valid page size\n", v->media_size);
	}
    good_page_size:
      for (i = 0; i < count; i++)
	free(vptr[i]);
      free(vptr);
    }
  else
    {
      int height, width;
      (*p->limit)(p, v, &width, &height);
#if 0
      fprintf(stderr, "limit %d %d dims %d %d\n", width, height,
	      v->page_width, v->page_height);
#endif
      if (v->page_height <= 0 || v->page_height > height ||
	  v->page_width <= 0 || v->page_width > width)
	{
	  answer = 0;
	  fprintf(stderr, "Image size is not valid\n");
	}
    }

  if (strlen(v->media_type) > 0)
    {
      vptr = (*p->parameters)(p, NULL, "MediaType", &count);
      if (count > 0)
	{
	  for (i = 0; i < count; i++)
	    if (!strcmp(v->media_type, vptr[i]))
	      goto good_media_type;
	  answer = 0;
	  fprintf(stderr, "%s is not a valid media type\n", v->media_type);
	}
    good_media_type:
      for (i = 0; i < count; i++)
	free(vptr[i]);
      free(vptr);
    }

  if (strlen(v->media_source) > 0)
    {
      vptr = (*p->parameters)(p, NULL, "InputSlot", &count);
      if (count > 0)
	{
	  for (i = 0; i < count; i++)
	    if (!strcmp(v->media_source, vptr[i]))
	      goto good_media_source;
	  answer = 0;
	  fprintf(stderr, "%s is not a valid media source\n", v->media_source);
	}
    good_media_source:
      for (i = 0; i < count; i++)
	free(vptr[i]);
      free(vptr);
    }

  if (strlen(v->resolution) > 0)
    {
      vptr = (*p->parameters)(p, NULL, "Resolution", &count);
      if (count > 0)
	{
	  for (i = 0; i < count; i++)
	    if (!strcmp(v->resolution, vptr[i]))
	      goto good_resolution;
	  answer = 0;
	  fprintf(stderr, "%s is not a valid resolution\n", v->resolution);
	}
    good_resolution:
      for (i = 0; i < count; i++)
	free(vptr[i]);
      free(vptr);
    }

  if (strlen(v->ink_type) > 0)
    {
      vptr = (*p->parameters)(p, NULL, "InkType", &count);
      if (count > 0)
	{
	  for (i = 0; i < count; i++)
	    if (!strcmp(v->ink_type, vptr[i]))
	      goto good_ink_type;
	  answer = 0;
	  fprintf(stderr, "%s is not a valid ink type\n", v->ink_type);
	}
    good_ink_type:
      for (i = 0; i < count; i++)
	free(vptr[i]);
      free(vptr);
    }

  for (i = 0; i < num_dither_algos; i++)
    if (!strcmp(v->dither_algorithm, dither_algo_names[i]))
      return answer;

  fprintf(stderr, "%s is not a valid dither algorithm\n", v->dither_algorithm);
  return 0;
}
