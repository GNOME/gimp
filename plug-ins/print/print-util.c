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
 */

/*
 * This file must include only standard C header files.  The core code must
 * compile on generic platforms that don't support glib, gimp, gtk, etc.
 */

/* #define PRINT_DEBUG */


#include "print.h"
#include <math.h>
#include <limits.h>

#ifndef __GNUC__
#  define inline
#endif /* !__GNUC__ */

typedef struct
{
  unsigned steps;
  unsigned short *composite;
  unsigned short *red;
  unsigned short *green;
  unsigned short *blue;
  unsigned shiftval;
  unsigned bin_size;
  unsigned bin_shift;
} lut_t;

/*
 * RGB to grayscale luminance constants...
 */

#define LUM_RED		31
#define LUM_GREEN	61
#define LUM_BLUE	8

/* rgb/hsv conversions taken from Gimp common/autostretch_hsv.c */

static vars_t default_vars =
{
	"",			/* Name of file or command to print to */
	"ps2",			/* Name of printer "driver" */
	"",			/* Name of PPD file */
	OUTPUT_COLOR,		/* Color or grayscale output */
	"",			/* Output resolution */
	"",			/* Size of output media */
	"",			/* Type of output media */
	"",			/* Source of output media */
	"",			/* Ink type */
	"",			/* Dither algorithm */
	1.0,			/* Output brightness */
	100.0,			/* Scaling (100% means entire printable area, */
				/*          -XXX means scale by PPI) */
	-1,			/* Orientation (-1 = automatic) */
	-1,			/* X offset (-1 = center) */
	-1,			/* Y offset (-1 = center) */
	1.0,			/* Screen gamma */
	1.0,			/* Contrast */
	1.0,			/* Cyan */
	1.0,			/* Magenta */
	1.0,			/* Yellow */
	0,			/* Linear */
	1.0,			/* Output saturation */
	1.0,			/* Density */
	IMAGE_CONTINUOUS,	/* Image type */
	0,			/* Unit 0=Inch */
	1.0,			/* Application gamma placeholder */
	0,			/* Page width */
	0			/* Page height */
};

static vars_t min_vars =
{
	"",			/* Name of file or command to print to */
	"ps2",			/* Name of printer "driver" */
	"",			/* Name of PPD file */
	OUTPUT_COLOR,		/* Color or grayscale output */
	"",			/* Output resolution */
	"",			/* Size of output media */
	"",			/* Type of output media */
	"",			/* Source of output media */
	"",			/* Ink type */
	"",			/* Dither algorithm */
	0,			/* Output brightness */
	5.0,			/* Scaling (100% means entire printable area, */
				/*          -XXX means scale by PPI) */
	-1,			/* Orientation (-1 = automatic) */
	-1,			/* X offset (-1 = center) */
	-1,			/* Y offset (-1 = center) */
	0.1,			/* Screen gamma */
	0,			/* Contrast */
	0,			/* Cyan */
	0,			/* Magenta */
	0,			/* Yellow */
	0,			/* Linear */
	0,			/* Output saturation */
	.1,			/* Density */
	IMAGE_CONTINUOUS,	/* Image type */
	0,			/* Unit 0=Inch */
	1.0,			/* Application gamma placeholder */
	0,			/* Page width */
	0			/* Page height */
};

static vars_t max_vars =
{
	"",			/* Name of file or command to print to */
	"ps2",			/* Name of printer "driver" */
	"",			/* Name of PPD file */
	OUTPUT_COLOR,		/* Color or grayscale output */
	"",			/* Output resolution */
	"",			/* Size of output media */
	"",			/* Type of output media */
	"",			/* Source of output media */
	"",			/* Ink type */
	"",			/* Dither algorithm */
	2.0,			/* Output brightness */
	100.0,			/* Scaling (100% means entire printable area, */
				/*          -XXX means scale by PPI) */
	-1,			/* Orientation (-1 = automatic) */
	-1,			/* X offset (-1 = center) */
	-1,			/* Y offset (-1 = center) */
	4.0,			/* Screen gamma */
	4.0,			/* Contrast */
	4.0,			/* Cyan */
	4.0,			/* Magenta */
	4.0,			/* Yellow */
	0,			/* Linear */
	9.0,			/* Output saturation */
	2.0,			/* Density */
	IMAGE_CONTINUOUS,	/* Image type */
	0,			/* Unit 0=Inch */
	1.0,			/* Application gamma placeholder */
	0,			/* Page width */
	0			/* Page height */
};

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
  int maxval;

  red   = rgb[0] / 65535.0;
  green = rgb[1] / 65535.0;
  blue  = rgb[2] / 65535.0;

  if (red > green)
    {
      if (red > blue)
	{
	  max = red;
	  maxval = 0;
	}
      else
	{
	  max = blue;
	  maxval = 2;
	}
      min = FMIN(green, blue);
    }
  else
    {
      if (green > blue)
	{
	  max = green;
	  maxval = 1;
	}
      else
	{
	  max = blue;
	  maxval = 2;
	}
      min = FMIN(red, blue);
    }

  l = (max + min) / 2.0;
  delta = max - min;

  if (delta < .000001)	/* Suggested by Eugene Anikin <eugene@anikin.com> */
    {
      s = 0.0;
      h = 0.0;
    }
  else
    {
      if (l <= .5)
	s = delta / (max + min);
      else
	s = delta / (2 - max - min);

      if (maxval == 0)
	h = (green - blue) / delta;
      else if (maxval == 1)
	h = 2 + (blue - red) / delta;
      else
	h = 4 + (red - green) / delta;

      if (h < 0.0)
	h += 6.0;
      else if (h > 6.0)
	h -= 6.0;
    }

  *hue = h;
  *sat = s;
  *lightness = l;
}

static inline double
hsl_value(double n1, double n2, double hue)
{
  if (hue < 0)
    hue += 6.0;
  else if (hue > 6)
    hue -= 6.0;
  if (hue < 1.0)
    return (n1 + (n2 - n1) * hue);
  else if (hue < 3.0)
    return (n2);
  else if (hue < 4.0)
    return (n1 + (n2 - n1) * (4.0 - hue));
  else
    return (n1);
}

static inline void
calc_hsl_to_rgb(unsigned short *rgb, double h, double s, double l)
{
  if (s < .0000001)
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
      double h1, h2;
      h1 = h + 2;
      h2 = h - 2;

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
  int c = 65535 - rgb[0];
  int m = 65535 - rgb[1];
  int y = 65535 - rgb[2];
  int nc, nm, ny;
  int k;
  if (c == m && c == y)
    return;
  k = FMIN(FMIN(c, m), y);

  /*
   * This is an attempt to achieve better color balance.  The goal
   * is to weaken the pure cyan, magenta, and yellow and strengthen
   * pure red, green, and blue.
   *
   * We also don't want S=1 V=1 cyan to be 100% cyan; it's simply
   * too dark.
   */

  nc = (c * 3 + FMIN(c, FMAX(m, y)) * 5 + FMAX(m, y) * 0 + k) / 8;
  nm = (m * 3 + FMIN(m, FMAX(c, y)) * 5 + FMAX(c, y) * 0 + k) / 8;
  ny = (y * 3 + FMIN(y, FMAX(c, m)) * 5 + FMAX(c, m) * 0 + k) / 8;

  /*
   * Make sure we didn't go overboard.  We don't want to go too
   * close to white unnecessarily.
   */
  nc = c + (nc - c) / 3;
  nm = m + (nm - m) / 3;
  ny = y + (ny - y) / 3;

  if (nc > 65535)
    nc = 65535;
  if (nm > 65535)
    nm = 65535;
  if (ny > 65535)
    ny = 65535;

  rgb[0] = 65535 - nc;
  rgb[1] = 65535 - nm;
  rgb[2] = 65535 - ny;
}

/*
 * A lot of this stuff needs to be factored out of here
 */
static inline unsigned short
lookup_value(unsigned short value, int lut_size, unsigned short *lut,
	     unsigned shiftval, unsigned bin_size, unsigned bin_shift)
{
  unsigned subrange;
  unsigned remainder;
  unsigned below;
  unsigned above;
  if (lut_size == 65536)
    return lut[value];
  subrange = value >> bin_shift;
  remainder = value & (bin_size - 1);
  below = lut[subrange];
  if (remainder == 0)
    return below;
  if (subrange == (bin_size - 1))
    above = lut[subrange];
  else
    above = lut[subrange + 1];
  if (above == below)
    return above;
  else
    return below + (((above - below) * remainder) >> bin_shift);
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
  int i0 = -1;
  int i1 = -1;
  int use_previous = 0;
  int o0 = 0;
  lut_t *lut = (lut_t *)(vars->lut);
  while (width > 0)
    {
      if (bpp == 1)
	{
	  /*
	   * No alpha in image...
	   */
	  if (i0 == grayin[0])
	    use_previous = 1;
	  else
	    {
	      use_previous = 0;
	      i0 = grayin[0];
	      grayout[0] = lut->composite[grayin[0]];
	    }
	}
      else
	{
	  if (i0 == grayin[0] && i1 == grayin[1])
	    use_previous = 1;
	  else
	    {
	      use_previous = 0;
	      i0 = grayin[0];
	      i1 = grayin[1];
	      grayout[0] = lut->composite[grayin[0] * grayin[1] / 255 +
					       255 - grayin[1]];
	    }
	}
      if (use_previous)
	{
	  grayout[0] = o0;
	}
      else
	{
	  if (vars->density != 1.0)
	    {
	      double t = (65535.0 + ((grayout[0] - 65535.0) * vars->density));
	      if (t < 0.0)
		t = 0.0;
	      grayout[0] = t + .5;
	    }
	  o0 = grayout[0];
	}
      grayin += bpp;
      grayout ++;
      width --;
    }
}

static void
gray_to_monochrome(unsigned char *grayin,	/* I - RGB pixels */
		   unsigned short *grayout,	/* O - RGB pixels */
		   int    	width,		/* I - Width of row */
		   int    	bpp,		/* I - Bytes-per-pixel in grayin */
		   unsigned char *cmap,		/* I - Colormap (unused) */
		   const vars_t	*vars
		   )
{
  int i0 = -1;
  int i1 = -1;
  int use_previous = 0;
  int o0 = 0;
  lut_t *lut = (lut_t *)(vars->lut);
  while (width > 0)
    {
      if (bpp == 1)
	{
	  /*
	   * No alpha in image...
	   */
	  if (i0 == grayin[0])
	    use_previous = 1;
	  else
	    {
	      use_previous = 0;
	      i0 = grayin[0];
	      grayout[0] = lut->composite[grayin[0]];
	    }
	}
      else
	{
	  if (i0 == grayin[0] && i1 == grayin[1])
	    use_previous = 1;
	  else
	    {
	      use_previous = 0;
	      i0 = grayin[0];
	      i1 = grayin[1];
	      grayout[0] = lut->composite[grayin[0] * grayin[1] / 255 +
					       255 - grayin[1]];
	    }
	}
      if (use_previous)
	{
	  grayout[0] = o0;
	}
      else
	{
	  if (grayout[0] < 32768)
	    grayout[0] = 0;
	  else
	    grayout[0] = 65535;
	  o0 = grayout[0];
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
  int i0 = -1;
  int i1 = -1;
  int o0 = 0;
  int use_previous = 0;
  int i;
  lut_t *lut = (lut_t *)(vars->lut);
  unsigned char	gray_cmap[256];		/* Grayscale colormap */

  /* Really should precompute this silly thing... */
  for (i = 0; i < 256; i ++, cmap += 3)
    gray_cmap[i] = (cmap[0] * LUM_RED +
		    cmap[1] * LUM_GREEN +
		    cmap[2] * LUM_BLUE) / 100;

  while (width > 0)
    {
      if (bpp == 1)
	{
	  /*
	   * No alpha in image...
	   */
	  if (i0 == indexed[0])
	    use_previous = 1;
	  else
	    {
	      use_previous = 0;
	      i0 = indexed[0];
	      gray[0] = lut->composite[gray_cmap[i0]];
	    }
	}
      else
	{
	  if (i0 == indexed[0] && i1 == indexed[1])
	    use_previous = 1;
	  else
	    {
	      use_previous = 0;
	      i0 = indexed[0];
	      i1 = indexed[1];
	      gray[0] = lut->composite[gray_cmap[i0 * i1 / 255]
					    + 255 - i1];
	    }
	}
      if (use_previous)
	{
	  gray[0] = o0;
	}
      else
	{
	  if (vars->density != 1.0)
	    {
	      double t = (65535.0 + ((gray[0] - 65535.0) * vars->density));
	      if (t < 0.0)
		t = 0.0;
	      gray[0] = t + .5;
	    }
	  o0 = gray[0];
	}
      indexed += bpp;
      gray ++;
      width --;
    }
}

static void
indexed_to_monochrome(unsigned char *indexed,	/* I - Indexed pixels */
		      unsigned short *gray,	/* O - Grayscale pixels */
		      int    width,		/* I - Width of row */
		      int    bpp,		/* I - bpp in indexed */
		      unsigned char *cmap,	/* I - Colormap */
		      const vars_t   *vars
		      )
{
  int i0 = -1;
  int i1 = -1;
  int o0 = 0;
  int use_previous = 0;
  int i;
  lut_t *lut = (lut_t *)(vars->lut);
  unsigned char	gray_cmap[256];		/* Grayscale colormap */

  /* Really should precompute this silly thing... */
  for (i = 0; i < 256; i ++, cmap += 3)
    gray_cmap[i] = (cmap[0] * LUM_RED +
		    cmap[1] * LUM_GREEN +
		    cmap[2] * LUM_BLUE) / 100;

  while (width > 0)
    {
      if (bpp == 1)
	{
	  /*
	   * No alpha in image...
	   */
	  if (i0 == indexed[0])
	    use_previous = 1;
	  else
	    {
	      use_previous = 0;
	      i0 = indexed[0];
	      gray[0] = lut->composite[gray_cmap[i0]];
	    }
	}
      else
	{
	  if (i0 == indexed[0] && i1 == indexed[1])
	    use_previous = 1;
	  else
	    {
	      use_previous = 0;
	      i0 = indexed[0];
	      i1 = indexed[1];
	      gray[0] = lut->composite[gray_cmap[i0 * i1 / 255]
				      + 255 - i1];
	    }
	}
      if (use_previous)
	{
	  gray[0] = o0;
	}
      else
	{
	  if (gray[0] < 32768)
	    gray[0] = 0;
	  else
	    gray[0] = 65535;
	  o0 = gray[0];
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
  int i0 = -1;
  int i1 = -1;
  int i2 = -1;
  int i3 = -1;
  int o0 = 0;
  int use_previous = 0;
  lut_t *lut = (lut_t *)(vars->lut);
  while (width > 0)
    {
      if (bpp == 3)
	{
	  /*
	   * No alpha in image...
	   */
	  if (i0 == rgb[0] && i1 == rgb[1] && i2 == rgb[2])
	    use_previous = 1;
	  else
	    {
	      use_previous = 0;
	      i0 = rgb[0];
	      i1 = rgb[1];
	      i2 = rgb[2];
	      gray[0] = lut->composite[(rgb[0] * LUM_RED +
					rgb[1] * LUM_GREEN +
					rgb[2] * LUM_BLUE) / 100];
	    }
	}
      else
	{
	  if (i0 == rgb[0] && i1 == rgb[1] && i2 == rgb[2] && i3 == rgb[3])
	    use_previous = 1;
	  else
	    {
	      use_previous = 0;
	      i0 = rgb[0];
	      i1 = rgb[1];
	      i2 = rgb[2];
	      i3 = rgb[3];
	  
	      gray[0] = lut->composite[((rgb[0] * LUM_RED +
					 rgb[1] * LUM_GREEN +
					 rgb[2] * LUM_BLUE) *
					rgb[3] / 25500 + 255 - rgb[3])];
	    }
	}
      if (use_previous)
	{
	  gray[0] = o0;
	}
      else
	{
	  if (vars->density != 1.0)
	    {
	      double t = (65535.0 + ((gray[0] - 65535.0) * vars->density));
	      if (t < 0.0)
		t = 0.0;
	      gray[0] = t + .5;
	    }
	  o0 = gray[0];
	}
      rgb += bpp;
      gray ++;
      width --;
    }
}

static void
rgb_to_monochrome(unsigned char *rgb,	/* I - RGB pixels */
		  unsigned short *gray,	/* O - Grayscale pixels */
		  int    width,		/* I - Width of row */
		  int    bpp,		/* I - Bytes-per-pixel in RGB */
		  unsigned char *cmap,	/* I - Colormap (unused) */
		  const vars_t   *vars
		  )
{
  int i0 = -1;
  int i1 = -1;
  int i2 = -1;
  int i3 = -1;
  int o0 = 0;
  int use_previous = 0;
  lut_t *lut = (lut_t *)(vars->lut);
  while (width > 0)
    {
      if (bpp == 3)
	{
	  /*
	   * No alpha in image...
	   */
	  if (i0 == rgb[0] && i1 == rgb[1] && i2 == rgb[2])
	    use_previous = 1;
	  else
	    {
	      use_previous = 0;
	      i0 = rgb[0];
	      i1 = rgb[1];
	      i2 = rgb[2];
	      gray[0] = lut->composite[(rgb[0] * LUM_RED +
					rgb[1] * LUM_GREEN +
					rgb[2] * LUM_BLUE) / 100];
	    }
	}
      else
	{
	  if (i0 == rgb[0] && i1 == rgb[1] && i2 == rgb[2] && i3 == rgb[3])
	    use_previous = 1;
	  else
	    {
	      use_previous = 0;
	      i0 = rgb[0];
	      i1 = rgb[1];
	      i2 = rgb[2];
	      i3 = rgb[3];
	  
	      gray[0] = lut->composite[((rgb[0] * LUM_RED +
					 rgb[1] * LUM_GREEN +
					 rgb[2] * LUM_BLUE) *
					rgb[3] / 25500 + 255 - rgb[3])];
	    }
	}
      if (use_previous)
	{
	  gray[0] = o0;
	}
      else
	{
	  if (gray[0] < 32768)
	    gray[0] = 0;
	  else
	    gray[0] = 65535;
	  o0 = gray[0];
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
  double ssat = vars->saturation;
  int i0 = -1;
  int i1 = -1;
  int i2 = -1;
  int i3 = -1;
  int o0 = 0;
  int o1 = 0;
  int o2 = 0;
  int use_previous = 0;
  lut_t *lut = (lut_t *)(vars->lut);
  int compute_saturation = ssat <= .99999 || ssat >= 1.00001;
  int split_saturation = ssat > 1.4;
  if (split_saturation)
    ssat = sqrt(ssat);
  if (ssat > 1)
    isat = 1.0 / ssat;
#if 0
printf("rgb-to-rgb: ssat=%f, isat=%f, do-sat=%d, split-sat=%d\n", ssat, isat, compute_saturation, split_saturation);
#endif
  while (width > 0)
    {
      double h, s, v;
      switch (bpp)
	{
	case 1:
	  /*
	   * No alpha in image, using colormap...
	   */
	  if (i0 == rgbin[0])
	    use_previous = 1;
	  else
	    {
	      use_previous = 0;
	      i0 = rgbin[0];
	      rgbout[0] = cmap[rgbin[0] * 3 + 0] * 257;
	      rgbout[1] = cmap[rgbin[0] * 3 + 1] * 257;
	      rgbout[2] = cmap[rgbin[0] * 3 + 2] * 257;
	    }
	  break;
	case 2:
	  if (i0 == rgbin[0] && i1 == rgbin[1])
	    use_previous = 1;
	  else
	    {
	      use_previous = 0;
	      i0 = rgbin[0];
	      i1 = rgbin[1];
	      rgbout[0] = (cmap[rgbin[0] * 3 + 0] *
			   rgbin[1] / 255 + 255 - rgbin[1]) * 257;
	      rgbout[1] = (cmap[rgbin[0] * 3 + 0] *
			   rgbin[1] / 255 + 255 - rgbin[1]) * 257;
	      rgbout[2] = (cmap[rgbin[0] * 3 + 0] *
			   rgbin[1] / 255 + 255 - rgbin[1]) * 257;
	    }
	  break;
	case 3:
	  /*
	   * No alpha in image...
	   */
	  if (i0 == rgbin[0] && i1 == rgbin[1] && i2 == rgbin[2])
	    use_previous = 1;
	  else
	    {
	      use_previous = 0;
	      i0 = rgbin[0];
	      i1 = rgbin[1];
	      i2 = rgbin[2];
	      rgbout[0] = rgbin[0] * 257;
	      rgbout[1] = rgbin[1] * 257;
	      rgbout[2] = rgbin[2] * 257;
	    }
	  break;
	case 4:
	  if (i0 == rgbin[0] && i1 == rgbin[1] && i2 == rgbin[2] &&
	      i3 == rgbin[3])
	    use_previous = 1;
	  else
	    {
	      use_previous = 0;
	      i0 = rgbin[0];
	      i1 = rgbin[1];
	      i2 = rgbin[2];
	      i3 = rgbin[3];
	      rgbout[0] = (rgbin[0] * rgbin[3] / 255 + 255 - rgbin[3]) * 257;
	      rgbout[1] = (rgbin[1] * rgbin[3] / 255 + 255 - rgbin[3]) * 257;
	      rgbout[2] = (rgbin[2] * rgbin[3] / 255 + 255 - rgbin[3]) * 257;
	    }
	  break;
	}
      if (use_previous)
	{
	  rgbout[0] = o0;
	  rgbout[1] = o1;
	  rgbout[2] = o2;
	}
      else
	{
	  if (compute_saturation &&
	      (rgbout[0] != rgbout[1] || rgbout[0] != rgbout[2]))
	    {
	      rgbout[0] = 65535 - rgbout[0];
	      rgbout[1] = 65535 - rgbout[1];
	      rgbout[2] = 65535 - rgbout[2];
	      calc_rgb_to_hsl(rgbout, &h, &s, &v);
	      if (ssat < 1)
		s *= ssat;
	      else
		{
		  double s1 = s * ssat;
		  double s2 = 1.0 - ((1.0 - s) * isat);
		  s = FMIN(s1, s2);
		}
	      if (s > 1)
		s = 1.0;
	      calc_hsl_to_rgb(rgbout, h, s, v);
	      rgbout[0] = 65535 - rgbout[0];
	      rgbout[1] = 65535 - rgbout[1];
	      rgbout[2] = 65535 - rgbout[2];
	    }
	  update_cmyk(rgbout);	/* Fiddle with the INPUT */
	  rgbout[0] = lookup_value(rgbout[0], lut->steps,
				   lut->red, lut->shiftval,
				   lut->bin_size, lut->bin_shift);
	  rgbout[1] = lookup_value(rgbout[1], lut->steps,
				   lut->green, lut->shiftval,
				   lut->bin_size, lut->bin_shift);
	  rgbout[2] = lookup_value(rgbout[2], lut->steps,
				   lut->blue, lut->shiftval,
				   lut->bin_size, lut->bin_shift);
	  if (split_saturation &&
	      (rgbout[0] != rgbout[1] || rgbout[0] != rgbout[2]))
	    {
	      rgbout[0] = 65535 - rgbout[0];
	      rgbout[1] = 65535 - rgbout[1];
	      rgbout[2] = 65535 - rgbout[2];
	      calc_rgb_to_hsl(rgbout, &h, &s, &v);
	      if (ssat < 1)
		s *= ssat;
	      else
		{
		  double s1 = s * ssat;
		  double s2 = 1.0 - ((1.0 - s) * isat);
		  s = FMIN(s1, s2);
		}
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
	  o0 = rgbout[0];
	  o1 = rgbout[1];
	  o2 = rgbout[2];
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
  int use_previous = 0;
  int i0 = -1;
  int i1 = -1;
  int o0 = 0;
  int o1 = 0;
  int o2 = 0;
  lut_t *lut = (lut_t *)(vars->lut);
  while (width > 0)
    {
      unsigned short trgb[3];
      if (bpp == 1)
	{
	  /*
	   * No alpha in image...
	   */
	  if (i0 == grayin[0])
	    use_previous = 1;
	  else
	    {
	      use_previous = 0;
	      i0 = grayin[0];
	      trgb[0] = grayin[0] * 257;
	      trgb[1] = grayin[0] * 257;
	      trgb[2] = grayin[0] * 257;
	    }
	}
      else
	{
	  if (i0 == grayin[0] && i1 == grayin[1])
	    use_previous = 1;
	  else
	    {
	      int lookup = (grayin[0] * grayin[1] / 255 + 255 - grayin[1]) *
		257;
	      use_previous = 0;
	      i0 = grayin[0];
	      i1 = grayin[1];
	      trgb[0] = lookup;
	      trgb[1] = lookup;
	      trgb[2] = lookup;
	    }
	}
      if (use_previous)
	{
	  rgbout[0] = o0;
	  rgbout[1] = o1;
	  rgbout[2] = o2;
	}
      else
	{
	  update_cmyk(trgb);
	  rgbout[0] = lookup_value(trgb[0], lut->steps,
				   lut->red, lut->shiftval,
				   lut->bin_size, lut->bin_shift);
	  rgbout[1] = lookup_value(trgb[1], lut->steps,
				   lut->green, lut->shiftval,
				   lut->bin_size, lut->bin_shift);
	  rgbout[2] = lookup_value(trgb[2], lut->steps,
				   lut->blue, lut->shiftval,
				   lut->bin_size, lut->bin_shift);
	  if (vars->saturation != 1.0)
	    {
	      double t;
	      int i;
	      for (i = 0; i < 3; i++)
		{
		  t = (65535.0 + ((rgbout[i] - 65535.0) * vars->density));
		  if (t < 0.0)
		    t = 0.0;
		  rgbout[i] = t + .5;
		}
	    }
	  o0 = rgbout[0];
	  o1 = rgbout[1];
	  o2 = rgbout[2];
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
  int i0 = -1;
  int i1 = -1;
  int o0 = 0;
  int o1 = 0;
  int o2 = 0;
  lut_t *lut = (lut_t *)(vars->lut);
  int use_previous = 0;
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
	  if (i0 == indexed[0])
	    use_previous = 1;
	  else
	    {
	      use_previous = 0;
	      i0 = indexed[0];
	      rgb[0] = lut->red[cmap[i0 * 3 + 0]];
	      rgb[1] = lut->green[cmap[i0 * 3 + 1]];
	      rgb[2] = lut->blue[cmap[i0 * 3 + 2]];
	    }
	}
      else
	{
	  if (i0 == indexed[0] && i1 == indexed[1])
	    use_previous = 1;
	  else
	    {
	      use_previous = 0;
	      i0 = indexed[0];
	      i1 = indexed[1];
	      rgb[0] = lut->red[cmap[i0 * 3 + 0] * i1 / 255 + 255 - i1];
	      rgb[1] = lut->green[cmap[i0 * 3 + 1] * i1 / 255 + 255 -i1];
	      rgb[2] = lut->blue[cmap[i0 * 3 + 2] * i1 / 255 + 255 - i1];
	    }
	}
      if (use_previous)
	{
	  rgb[0] = o0;
	  rgb[1] = o1;
	  rgb[2] = o2;
	}
      else
	{
	  if (vars->saturation != 1.0)
	    {
	      calc_rgb_to_hsl(rgb, &h, &s, &v);
	      if (vars->saturation < 1)
		s *= vars->saturation;
	      else
		{
		  double s1 = s * vars->saturation;
		  double s2 = 1.0 - ((1.0 - s) * isat);
		  s = FMIN(s1, s2);
		}
	      if (s > 1)
		s = 1.0;
	      calc_hsl_to_rgb(rgb, h, s, v);
	    }
	  if (vars->density != 1.0)
	    {
	      double t;
	      int i;
	      for (i = 0; i < 3; i++)
		{
		  t = (65535.0 + ((rgb[i] - 65535.0) * vars->density));
		  if (t < 0.0)
		    t = 0.0;
		  rgb[i] = t + .5;
		}
	    }
	  o0 = rgb[0];
	  o1 = rgb[1];
	  o2 = rgb[2];
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
  int i0 = -1;
  int i1 = -1;
  int i2 = -1;
  int i3 = -1;
  int o0 = 0;
  int o1 = 0;
  int o2 = 0;
  lut_t *lut = (lut_t *)(vars->lut);
  int use_previous = 0;
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
	  if (i0 == rgbin[0] && i1 == rgbin[1] && i2 == rgbin[2])
	    use_previous = 1;
	  else
	    {
	      use_previous = 0;
	      i0 = rgbin[0];
	      i1 = rgbin[1];
	      i2 = rgbin[2];
	      rgbout[0] = lut->red[rgbin[0]];
	      rgbout[1] = lut->green[rgbin[1]];
	      rgbout[2] = lut->blue[rgbin[2]];
	    }
	}
      else
	{
	  if (i0 == rgbin[0] && i1 == rgbin[1] && i2 == rgbin[2] &&
	      i3 == rgbin[3])
	    use_previous = 1;
	  else
	    {
	      use_previous = 0;
	      i0 = rgbin[0];
	      i1 = rgbin[1];
	      i2 = rgbin[2];
	      i3 = rgbin[3];
	      rgbout[0] = lut->red[i0 * i3 / 255 + 255 - i3];
	      rgbout[1] = lut->green[i1 * i3 / 255 + 255 - i3];
	      rgbout[2] = lut->blue[i2 * i3 / 255 + 255 - i3];
	    }
	}
      if (use_previous)
	{
	  rgbout[0] = o0;
	  rgbout[1] = o1;
	  rgbout[2] = o2;
	}
      else
	{
	  if (vars->saturation != 1.0)
	    {
	      calc_rgb_to_hsl(rgbout, &h, &s, &v);
	      if (vars->saturation < 1)
		s *= vars->saturation;
	      else
		{
		  double s1 = s * vars->saturation;
		  double s2 = 1.0 - ((1.0 - s) * isat);
		  s = FMIN(s1, s2);
		}
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
	  o0 = rgbout[0];
	  o1 = rgbout[1];
	  o2 = rgbout[2];
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
  int use_previous = 0;
  int i0 = -1;
  int i1 = -1;
  int o0 = 0;
  int o1 = 0;
  int o2 = 0;
  lut_t *lut = (lut_t *)(vars->lut);
  while (width > 0)
    {
      if (bpp == 1)
	{
	  /*
	   * No alpha in image...
	   */
	  if (i0 == grayin[0])
	    use_previous = 1;
	  else
	    {
	      use_previous = 0;
	      i0 = grayin[0];
	      rgbout[0] = lut->red[grayin[0]];
	      rgbout[1] = lut->green[grayin[0]];
	      rgbout[2] = lut->blue[grayin[0]];
	    }
	}
      else
	{
	  if (i0 == grayin[0] && i1 == grayin[1])
	    use_previous = 1;
	  else
	    {
	      int lookup = (grayin[0] * grayin[1] / 255 +
			    255 - grayin[1]);
	      use_previous = 0;
	      i0 = grayin[0];
	      i1 = grayin[1];
	      rgbout[0] = lut->red[lookup];
	      rgbout[1] = lut->green[lookup];
	      rgbout[2] = lut->blue[lookup];
	    }
	}
      if (use_previous)
	{
	  rgbout[0] = o0;
	  rgbout[1] = o1;
	  rgbout[2] = o2;
	}
      else
	{
	  if (vars->density != 1.0)
	    {
	      double t;
	      int i;
	      for (i = 0; i < 3; i++)
		{
		  t = (65535.0 + ((rgbout[i] - 65535.0) * vars->density));
		  if (t < 0.0)
		    t = 0.0;
		  rgbout[i] = t + .5;
		}
	    }
      	  o0 = rgbout[0];
	  o1 = rgbout[1];
	  o2 = rgbout[2];
	}
      grayin += bpp;
      rgbout += 3;
      width --;
    }
}

#define ICLAMP(value)				\
do						\
{						\
  if (user->value < min->value)			\
    user->value = min->value;			\
  else if (user->value > max->value)		\
    user->value = max->value;			\
} while (0)

void
merge_printvars(vars_t *user, const vars_t *print)
{
  const vars_t *max = print_maximum_settings();
  const vars_t *min = print_minimum_settings();
  user->cyan = (user->cyan * print->cyan);
  ICLAMP(cyan);
  user->magenta = (user->magenta * print->magenta);
  ICLAMP(magenta);
  user->yellow = (user->yellow * print->yellow);
  ICLAMP(yellow);
  user->contrast = (user->contrast * print->contrast);
  ICLAMP(contrast);
  user->brightness = (user->brightness * print->brightness);
  ICLAMP(brightness);
  user->gamma /= print->gamma;
  ICLAMP(gamma);
  user->saturation *= print->saturation;
  ICLAMP(saturation);
  user->density *= print->density;
  ICLAMP(density);
}

static lut_t *
allocate_lut(size_t steps)
{
  int i;
  lut_t *ret = malloc(sizeof(lut_t));
  ret->steps = steps;
  ret->composite = malloc(sizeof(unsigned short) * steps);
  ret->red = malloc(sizeof(unsigned short) * steps);
  ret->green = malloc(sizeof(unsigned short) * steps);
  ret->blue = malloc(sizeof(unsigned short) * steps);
  ret->shiftval = 0;
  for (i = 1; i < steps; i += i)
    ret->shiftval++;
  ret->bin_size = 65536 / steps;
  ret->bin_shift = 16 - ret->shiftval;
  return ret;
}

void
free_lut(vars_t *v)
{
  if (v->lut)
    {
      lut_t *lut = (lut_t *)(v->lut);
      if (lut->composite)
	free(lut->composite);
      if (lut->red)
	free(lut->red);
      if (lut->green)
	free(lut->green);
      if (lut->blue)
	free(lut->blue);
      lut->steps = 0;
      lut->composite = NULL;
      lut->red = NULL;
      lut->green = NULL;
      lut->blue = NULL;
      free(v->lut);
      v->lut = NULL;
    }
}

/* #define PRINT_LUT */

void
compute_lut(size_t steps, vars_t *uv)
{
  double	pixel,		/* Pixel value */
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

  double cyan = uv->cyan;
  double magenta = uv->magenta;
  double yellow = uv->yellow;
  double print_gamma = uv->gamma;
  double contrast = uv->contrast;
  double app_gamma = uv->app_gamma;
  double brightness = uv->brightness;
  double screen_gamma = app_gamma / 1.7;	/* Why 1.7??? */
  lut_t *lut;

  /*
   * Monochrome mode simply thresholds the input
   * to decide whether to print at all.  The printer gamma
   * is intended to represent the analog response of the printer.
   * Using it shifts the threshold, which is not the intent
   * of how this works.
   */
  if (uv->image_type == IMAGE_MONOCHROME)
    print_gamma = 1.0;

  uv->lut = allocate_lut(steps);
  lut = (lut_t *)(uv->lut);
  for (i = 0; i < steps; i ++)
    {
      double temp_pixel;
      pixel = (double) i / (double) (steps - 1);

      /*
       * First, correct contrast
       */
      if (pixel >= .5)
	temp_pixel = 1.0 - pixel;
      else
	temp_pixel = pixel;
      if (temp_pixel <= .000001 && contrast <= .0001)
	temp_pixel = .5;
      else if (temp_pixel > 1)
	temp_pixel = .5 * pow(2 * temp_pixel, pow(contrast, contrast));
      else if (temp_pixel < 1)
	temp_pixel = 0.5 -
	  ((0.5 - .5 * pow(2 * temp_pixel, contrast)) * contrast);
      if (temp_pixel > .5)
	temp_pixel = .5;
      else if (temp_pixel < 0)
	temp_pixel = 0;
      if (pixel < .5)
	pixel = temp_pixel;
      else
	pixel = 1 - temp_pixel;

      /*
       * Second, do brightness
       */
      if (brightness < 1)
	pixel = pixel * brightness;
      else
	pixel = 1 - ((1 - pixel) * (2 - brightness));

      /*
       * Third, correct for the screen gamma
       */
      pixel = 1.0 - pow(pixel, screen_gamma);

      /*
       * Third, fix up cyan, magenta, yellow values
       */
      if (pixel < 0.0)
	pixel = 0.0;
      else if (pixel > 1.0)
	pixel = 1.0;

      if (pixel > .9999 && cyan < .00001)
	red_pixel = 0;
      else
	red_pixel = 1 - pow(1 - pixel, cyan);
      if (pixel > .9999 && magenta < .00001)
	green_pixel = 0;
      else
	green_pixel = 1 - pow(1 - pixel, magenta);
      if (pixel > .9999 && yellow < .00001)
	blue_pixel = 0;
      else
	blue_pixel = 1 - pow(1 - pixel, yellow);

      /*
       * Finally, fix up print gamma and scale
       */

      pixel = 65535 * (1 - pow(pixel, print_gamma)) + .5;
      red_pixel = 65535 * (1 - pow(red_pixel, print_gamma)) + .5;
      green_pixel = 65535 * (1 - pow(green_pixel, print_gamma)) + .5;
      blue_pixel = 65535 * (1 - pow(blue_pixel, print_gamma)) + .5;

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
#ifdef PRINT_LUT
      fprintf(ltfile, "%3i  %5d  %5d  %5d  %5d  %f %f %f %f  %f %f %f  %f\n",
	      i, lut->composite[i], lut->red[i],
	      lut->green[i], lut->blue[i], pixel, red_pixel,
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
  { "Letter",   612,  792, PAPERSIZE_ENGLISH },	/* 8.5in x 11in */
  { "Legal",    612, 1008, PAPERSIZE_ENGLISH },	/* 8.5in x 14in */
  { "Tabloid",  792, 1224, PAPERSIZE_ENGLISH },	/*  11in x 17in */
  { "Executive", 522, 756, PAPERSIZE_ENGLISH },	/* 7.25 * 10.5in */
  { "Postcard", 283,  416, PAPERSIZE_ENGLISH },	/* 100mm x 147mm */
  { "3x5",	216,  360, PAPERSIZE_ENGLISH },
  { "4x6",      288,  432, PAPERSIZE_ENGLISH },
  { "Epson 4x6 Photo Paper", 306, 495, PAPERSIZE_ENGLISH },
  { "5x7",      360,  504, PAPERSIZE_ENGLISH },
  { "5x8",      360,  576, PAPERSIZE_ENGLISH },
  { "HalfLetter", 396, 612, PAPERSIZE_ENGLISH },
  { "6x8",      432,  576, PAPERSIZE_ENGLISH },
  { "8x10",     576,  720, PAPERSIZE_ENGLISH },
  { "Manual",   396,  612, PAPERSIZE_ENGLISH },	/* 5.5in x 8.5in */
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
  { "4A",       4768, 6749, PAPERSIZE_METRIC },	/* 1682mm x 2378mm */
  { "2A",       3370, 4768, PAPERSIZE_METRIC },	/* 1189mm x 1682mm */
  { "A0",       2384, 3370, PAPERSIZE_METRIC },	/*  841mm x 1189mm */
  { "A1",       1684, 2384, PAPERSIZE_METRIC },	/*  594mm x  841mm */
  { "A2",       1191, 1684, PAPERSIZE_METRIC },	/*  420mm x  594mm */
  { "A3",        842, 1191, PAPERSIZE_METRIC },	/*  297mm x  420mm */
  { "A4",        595,  842, PAPERSIZE_METRIC },	/*  210mm x  297mm */
  { "A5",        420,  595, PAPERSIZE_METRIC },	/*  148mm x  210mm */
  { "A6",        297,  420, PAPERSIZE_METRIC },	/*  105mm x  148mm */
  { "A7",        210,  297, PAPERSIZE_METRIC },	/*   74mm x  105mm */
  { "A8",        148,  210, PAPERSIZE_METRIC },	/*   52mm x   74mm */
  { "A9",        105,  148, PAPERSIZE_METRIC },	/*   37mm x   52mm */
  { "A10",        73,  105, PAPERSIZE_METRIC },	/*   26mm x   37mm */

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
  { "B6-C4",     354,  918, PAPERSIZE_METRIC },	/*  125mm x  324mm */
  { "C6",        323,  459, PAPERSIZE_METRIC },	/*  114mm x  162mm */
  { "DL",        311,  623, PAPERSIZE_METRIC },	/*  110mm x  220mm */
  { "C7-6",      229,  459, PAPERSIZE_METRIC },	/*   81mm x  162mm */
  { "C7",        229,  323, PAPERSIZE_METRIC },	/*   81mm x  114mm */
  { "C8",        161,  229, PAPERSIZE_METRIC },	/*   57mm x   81mm */
  { "C9",        113,  161, PAPERSIZE_METRIC },	/*   40mm x   57mm */
  { "C10",        79,  113, PAPERSIZE_METRIC },	/*   28mm x   40mm */

  /*
   * US CAD standard paper sizes
   */
  { "ArchA",	 648,  864, PAPERSIZE_ENGLISH },
  { "ArchB",	 864, 1296, PAPERSIZE_ENGLISH },
  { "ArchC",	1296, 1728, PAPERSIZE_ENGLISH },
  { "ArchD",	1728, 2592, PAPERSIZE_ENGLISH },
  { "ArchE",	2592, 3456, PAPERSIZE_ENGLISH },

  /*
   * Foolscap
   */
  { "flsa",	 612,  936, PAPERSIZE_ENGLISH }, /* American foolscap */
  { "flse",	 648,  936, PAPERSIZE_ENGLISH }, /* European foolscap */

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

  /* Miscellaneous sizes */
  { "Hagaki Card", 283, 420, PAPERSIZE_METRIC }, /* 100 x 148 mm */
  { "Oufuku Card", 420, 567, PAPERSIZE_METRIC }, /* 148 x 200 mm */
  { "Long 3", 340, 666, PAPERSIZE_METRIC }, /* Japanese long envelope #3 */
  { "Long 4", 255, 581, PAPERSIZE_METRIC }, /* Japanese long envelope #4 */
  { "Kaku", 680, 941, PAPERSIZE_METRIC }, /* Japanese Kaku envelope #4 */
  { "Commercial 10", 297, 684, PAPERSIZE_ENGLISH }, /* US Commercial 10 env */
  { "A2 Invitation", 315, 414, PAPERSIZE_ENGLISH }, /* US A2 invitation */

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
      if (!strcasecmp(val->name, name))
	return val;
      val++;
    }
  return NULL;
}

#define IABS(a) ((a) > 0 ? a : -(a))

static int
paper_size_mismatch(int l, int w, const papersize_t *val)
{
  int hdiff = IABS(l - (int) val->length);
  int vdiff = fabs(w - (int) val->width);
  return hdiff + vdiff;
}

const papersize_t *
get_papersize_by_size(int l, int w)
{
  int score = INT_MAX;
  const papersize_t *ref = NULL;
  const papersize_t *val = &(paper_sizes[0]);
  while (strlen(val->name) > 0)
    {
      if (val->width == w && val->length == l)
	return val;
      else
	{
	  int myscore = paper_size_mismatch(l, w, val);
	  if (myscore < score && myscore < 20)
	    {
	      ref = val;
	      score = myscore;
	    }
	}
      val++;
    }
  return ref;
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
  if (v->image_type == IMAGE_MONOCHROME)
    {
      *out_bpp = 1;

      if (image_bpp >= 3)
	return rgb_to_monochrome;
      else if (cmap == NULL)
	return gray_to_monochrome;
      else
	return indexed_to_monochrome;
    }
  else if (output_type == OUTPUT_COLOR)
    {
      *out_bpp = 3;

      if (image_bpp >= 3)
	{
	  if (v->image_type == IMAGE_CONTINUOUS)
	    return rgb_to_rgb;
	  else
	    return fast_rgb_to_rgb;
	}
      else if (cmap == NULL)
        {
          if (v->image_type == IMAGE_CONTINUOUS)
	    return gray_to_rgb;
          else
	    return fast_gray_to_rgb;
        }
      else
	{
	  if (v->image_type == IMAGE_CONTINUOUS)
	    return indexed_to_rgb;
	  else
	    return fast_indexed_to_rgb;
	}
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
			double scaling, /* I */
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
      Image_rotate_ccw(image);
  else if (*orientation == ORIENT_UPSIDEDOWN)
      Image_rotate_180(image);
  else if (*orientation == ORIENT_SEASCAPE)
      Image_rotate_cw(image);

  image_width  = Image_width(image);
  image_height = Image_height(image);

  /*
   * Calculate width/height...
   */

  if (scaling == 0.0)
    {
      *out_width  = *page_width;
      *out_height = *page_height;
    }
  else if (scaling < 0.0)
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
    }

  if (*out_width == 0)
    *out_width = 1;
  if (*out_height == 0)
    *out_height = 1;

  /*
   * Adjust offsets depending on the page orientation...
   */

  if (*orientation == ORIENT_LANDSCAPE || *orientation == ORIENT_SEASCAPE)
    {
      int x;

      x     = *left;
      *left = *top;
      *top  = x;
    }

  if ((*orientation == ORIENT_UPSIDEDOWN || *orientation == ORIENT_SEASCAPE)
      && *left >= 0)
    {
      *left = *page_width - *left - *out_width;
      if (*left < 0) {
	*left = 0;
      }
    }

  if ((*orientation == ORIENT_UPSIDEDOWN || *orientation == ORIENT_LANDSCAPE)
      && *top >= 0)
    {
      *top = *page_height - *top - *out_height;
      if (*top < 0) {
	*top = 0;
      }
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

const vars_t *
print_default_settings()
{
  return &default_vars;
}

const vars_t *
print_maximum_settings()
{
  return &max_vars;
}

const vars_t *
print_minimum_settings()
{
  return &min_vars;
}

#ifdef QUANTIFY
unsigned quantify_counts[NUM_QUANTIFY_BUCKETS] = {0};
struct timeval quantify_buckets[NUM_QUANTIFY_BUCKETS] = {{0,0}};
int quantify_high_index = 0;
int quantify_first_time = 1;
struct timeval quantify_cur_time;
struct timeval quantify_prev_time;

void print_timers() 
{
    int i;

    printf("Quantify timers:\n");
    for (i = 0; i <= quantify_high_index; i++) {
       if (quantify_counts[i] == 0) continue;
        printf("Bucket %d:\t%ld.%ld s\thit %u times\n", i, quantify_buckets[i].tv_sec, quantify_buckets[i].tv_usec, quantify_counts[i]);
        quantify_buckets[i].tv_sec = 0;
        quantify_buckets[i].tv_usec = 0;
        quantify_counts[i] = 0;
    }
}
#endif
