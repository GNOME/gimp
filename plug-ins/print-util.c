/*
 * "$Id$"
 *
 *   Print plug-in driver utility functions for the GIMP.
 *
 *   Copyright 1997 Michael Sweet (mike@easysw.com)
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
 *   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 * Contents:
 *
 *   dither_black()    - Dither grayscale pixels to black.
 *   dither_cmyk()     - Dither RGB pixels to cyan, magenta, yellow, and black.
 *   gray_to_gray()    - Convert grayscale image data to grayscale (brightness
 *                       adjusted).
 *   indexed_to_gray() - Convert indexed image data to grayscale.
 *   indexed_to_rgb()  - Convert indexed image data to RGB.
 *   media_width()     - Get the addressable width of the page.
 *   media_height()    - Get the addressable height of the page.
 *   rgb_to_gray()     - Convert RGB image data to grayscale.
 *   rgb_to_rgb()      - Convert RGB image data to RGB (brightness adjusted).
 *
 * Revision History:
 *
 *   $Log$
 *   Revision 1.1  1997/11/24 22:03:34  sopwith
 *   Initial revision
 *
 *   Revision 1.3  1997/10/03 22:18:14  nobody
 *   updated print er too the latest version
 *
 *   : ----------------------------------------------------------------------
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

#include "print.h"


/*
 * RGB to grayscale luminance constants...
 */

#define LUM_RED		31
#define LUM_GREEN	61
#define LUM_BLUE	8


/*
 * Error buffer for dither functions.  This needs to be at least 11xMAXDPI
 * (currently 720) to avoid problems...
 */

int	error[2][4][11*720+4] = { 0 };


/*
 * 'dither_black()' - Dither grayscale pixels to black.
 */

void
dither_black(guchar        *gray,	/* I - Grayscale pixels */
             int           row,		/* I - Current Y coordinate */
             int           src_width,	/* I - Width of input row */
             int           dst_width,	/* I - Width of output row */
             unsigned char *black)	/* O - Black bitmap pixels */
{
  int		x,		/* Current X coordinate */
		xerror,		/* X error count */
		xstep,		/* X step */
		xmod,		/* X error modulus */
		length;		/* Length of output bitmap in bytes */
  unsigned char	bit,		/* Current bit */
		*kptr;		/* Current black pixel */
  int		k,		/* Current black error */
		*kerror0,	/* Pointer to current error row */
		*kerror1;	/* Pointer to next error row */


  xstep  = src_width / dst_width;
  xmod   = src_width % dst_width;
  length = (dst_width + 7) / 8;

  kerror0    = error[row & 1][3] + 2;
  kerror1    = error[1 - (row & 1)][3] + 2;
  kerror1[0] = 0;
  kerror1[1] = 0;

  memset(black, 0, length);

  for (x = 0, bit = 128, kptr = black, xerror = 0;
       x < dst_width;
       x ++, kerror0 ++, kerror1 ++)
  {
    k = 255 - *gray + *kerror0;
    if (k > 127)
    {
      *kptr |= bit;
      k -= 255;
    };

    kerror0[1]  += k / 4;
    kerror0[2]  += k / 8;
    kerror1[-2] += k / 16;
    kerror1[-1] += k / 8;
    kerror1[0]  += k / 4;
    kerror1[1]  += k / 8;
    kerror1[2]  = k / 16;

    if (bit == 1)
    {
      kptr ++;
      bit = 128;
    }
    else
      bit >>= 1;

    gray   += xstep;
    xerror += xmod;
    if (xerror >= dst_width)
    {
      xerror -= dst_width;
      gray   ++;
    };
  };
}


/*
 * 'dither_cmyk()' - Dither RGB pixels to cyan, magenta, yellow, and black.
 */

void
dither_cmyk(guchar        *rgb,		/* I - RGB pixels */
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
  int		c, m, y, k, ik;	/* CMYK values */
  unsigned char	bit,		/* Current bit */
		*cptr,		/* Current cyan pixel */
		*mptr,		/* Current magenta pixel */
		*yptr,		/* Current yellow pixel */
		*kptr;		/* Current black pixel */
  int		cerror,		/* Current cyan error */
		*cerror0,	/* Pointer to current error row */
		*cerror1;	/* Pointer to next error row */
  int		yerror,		/* Current yellow error */
		*yerror0,	/* Pointer to current error row */
		*yerror1;	/* Pointer to next error row */
  int		merror,		/* Current magenta error */
		*merror0,	/* Pointer to current error row */
		*merror1;	/* Pointer to next error row */
  int		kerror,		/* Current black error */
		*kerror0,	/* Pointer to current error row */
		*kerror1;	/* Pointer to next error row */


  xstep  = 3 * (src_width / dst_width);
  xmod   = src_width % dst_width;
  length = (dst_width + 7) / 8;

  cerror0    = error[row & 1][0] + 2;
  cerror1    = error[1 - (row & 1)][0] + 2;
  cerror1[0] = 0;
  cerror1[1] = 0;

  merror0    = error[row & 1][1] + 2;
  merror1    = error[1 - (row & 1)][1] + 2;
  merror1[0] = 0;
  merror1[1] = 0;

  yerror0    = error[row & 1][2] + 2;
  yerror1    = error[1 - (row & 1)][2] + 2;
  yerror1[0] = 0;
  yerror1[1] = 0;

  kerror0    = error[row & 1][3] + 2;
  kerror1    = error[1 - (row & 1)][3] + 2;
  kerror1[0] = 0;
  kerror1[1] = 0;

  memset(cyan, 0, length);
  memset(magenta, 0, length);
  memset(yellow, 0, length);
  if (black != NULL)
    memset(black, 0, length);

  for (x = 0, bit = 128, cptr = cyan, mptr = magenta, yptr = yellow, kptr=black,
           xerror = 0;
       x < dst_width;
       x ++, cerror0 ++, cerror1 ++, merror0 ++, merror1 ++, yerror0 ++,
           yerror1 ++, kerror0 ++, kerror1 ++)
  {
    c = 255 - rgb[0];
    m = 255 - rgb[1];
    y = 255 - rgb[2];

    if (black != NULL)
    {
      k = MIN(c, MIN(m, y));
      if (k >= 255)
        c = m = y = 0;
      else if (k > 0)
      {
        ik = 255 - k;
        c  = 255 * (c - k) / ik;
        m  = 255 * (m - k) / ik;
        y  = 255 * (y - k) / ik;
      };

      k += *kerror0;
      if (k > 127)
      {
	*kptr |= bit;
	k -= 255;
      };

      kerror0[1]  += k / 4;
      kerror0[2]  += k / 8;
      kerror1[-2] += k / 16;
      kerror1[-1] += k / 8;
      kerror1[0]  += k / 4;
      kerror1[1]  += k / 8;
      kerror1[2]  = k / 16;

      if (bit == 1)
        kptr ++;
    };

    c += *cerror0;
    if (c > 127)
    {
      *cptr |= bit;
      c -= 255;
    };

    cerror0[1]  += c / 4;
    cerror0[2]  += c / 8;
    cerror1[-2] += c / 16;
    cerror1[-1] += c / 8;
    cerror1[0]  += c / 4;
    cerror1[1]  += c / 8;
    cerror1[2]  = c / 16;

    m += *merror0;
    if (m > 127)
    {
      *mptr |= bit;
      m -= 255;
    };

    merror0[1]  += m / 4;
    merror0[2]  += m / 8;
    merror1[-2] += m / 16;
    merror1[-1] += m / 8;
    merror1[0]  += m / 4;
    merror1[1]  += m / 8;
    merror1[2]  = m / 16;

    y += *yerror0;
    if (y > 127)
    {
      *yptr |= bit;
      y -= 255;
    };

    yerror0[1]  += y / 4;
    yerror0[2]  += y / 8;
    yerror1[-2] += y / 16;
    yerror1[-1] += y / 8;
    yerror1[0]  += y / 4;
    yerror1[1]  += y / 8;
    yerror1[2]  = y / 16;

    if (bit == 1)
    {
      cptr ++;
      mptr ++;
      yptr ++;
      bit = 128;
    }
    else
      bit >>= 1;

    rgb    += xstep;
    xerror += xmod;
    if (xerror >= dst_width)
    {
      xerror -= dst_width;
      rgb    += 3;
    };
  };
}


/*
 * 'gray_to_gray()' - Convert grayscale image data to grayscale (brightness
 *                    adjusted).
 */

void
gray_to_gray(guchar *grayin,	/* I - RGB pixels */
             guchar *grayout,	/* O - RGB pixels */
             int    width,	/* I - Width of row */
             int    bpp,	/* I - Bytes-per-pixel in grayin */
             guchar *lut,	/* I - Brightness lookup table */
             guchar *cmap)	/* I - Colormap (unused) */
{
  while (width > 0)
  {
    *grayout = lut[*grayin];

    grayout ++;
    grayin  += bpp;
    width --;
  };
}


/*
 * 'indexed_to_gray()' - Convert indexed image data to grayscale.
 */

void
indexed_to_gray(guchar *indexed,	/* I - Indexed pixels */
                guchar *gray,		/* O - Grayscale pixels */
        	int    width,		/* I - Width of row */
        	int    bpp,		/* I - Bytes-per-pixel in indexed */
                guchar *lut,		/* I - Brightness lookup table */
                guchar *cmap)		/* I - Colormap */
{
  int		i;			/* Looping var */
  unsigned char	gray_cmap[256];		/* Grayscale colormap */


  for (i = 0; i < 256; i ++, cmap += 3)
    gray_cmap[i] = lut[(cmap[0] * LUM_RED + cmap[1] * LUM_GREEN + cmap[2] * LUM_BLUE) / 100];

  while (width > 0)
  {
    *gray = gray_cmap[*indexed];
    gray ++;
    indexed += bpp;
    width --;
  };
}


/*
 * 'indexed_to_rgb()' - Convert indexed image data to RGB.
 */

void
indexed_to_rgb(guchar *indexed,		/* I - Indexed pixels */
               guchar *rgb,		/* O - RGB pixels */
               int    width,		/* I - Width of row */
               int    bpp,		/* I - Bytes-per-pixel in indexed */
               guchar *lut,		/* I - Brightness lookup table */
               guchar *cmap)		/* I - Colormap */
{
  while (width > 0)
  {
    rgb[0] = lut[cmap[*indexed * 3 + 0]];
    rgb[1] = lut[cmap[*indexed * 3 + 1]];
    rgb[2] = lut[cmap[*indexed * 3 + 2]];
    rgb += 3;
    indexed += bpp;
    width --;
  };
}


/*
 * 'media_width()' - Get the addressable width of the page.
 *
 * This function assumes a standard left/right margin of 0.25".
 */

int
media_width(int media_size,		/* I - Media size code */
            int dpi)			/* I - Resolution in dots-per-inch */
{
  switch (media_size)
  {
    case MEDIA_LETTER :
    case MEDIA_LEGAL :
        return (8 * dpi);

    case MEDIA_TABLOID :
        return ((int)(10.5 * dpi + 0.5));

    case MEDIA_A4 :
        return ((int)(7.77 * dpi + 0.5));

    case MEDIA_A3 :
        return ((int)(11.09 * dpi + 0.5));

    default :
        return (0);
  };
}


/*
 * 'media_height()' - Get the addressable height of the page.
 *
 * This function assumes a standard top/bottom margin of 0.5".
 */

int
media_height(int media_size,		/* I - Media size code */
             int dpi)			/* I - Resolution in dots-per-inch */
{
  switch (media_size)
  {
    case MEDIA_LETTER :
        return (10 * dpi);

    case MEDIA_LEGAL :
        return (13 * dpi);

    case MEDIA_TABLOID :
        return (16 * dpi);

    case MEDIA_A4 :
        return ((int)(10.69 * dpi + 0.5));

    case MEDIA_A3 :
        return ((int)(15.54 * dpi + 0.5));

    default :
        return (0);
  };
}


/*
 * 'rgb_to_gray()' - Convert RGB image data to grayscale.
 */

void
rgb_to_gray(guchar *rgb,		/* I - RGB pixels */
            guchar *gray,		/* O - Grayscale pixels */
            int    width,		/* I - Width of row */
            int    bpp,			/* I - Bytes-per-pixel in RGB */
            guchar *lut,		/* I - Brightness lookup table */
            guchar *cmap)		/* I - Colormap (unused) */
{
  while (width > 0)
  {
    *gray = lut[(rgb[0] * LUM_RED + rgb[1] * LUM_GREEN + rgb[2] * LUM_BLUE) / 100];
    gray ++;
    rgb += bpp;
    width --;
  };
}


/*
 * 'rgb_to_rgb()' - Convert RGB image data to RGB (brightness adjusted).
 */

void
rgb_to_rgb(guchar *rgbin,	/* I - RGB pixels */
           guchar *rgbout,	/* O - RGB pixels */
           int    width,	/* I - Width of row */
           int    bpp,		/* I - Bytes-per-pixel in RGB */
           guchar *lut,		/* I - Brightness lookup table */
           guchar *cmap)	/* I - Colormap (unused) */
{
  while (width > 0)
  {
    rgbout[0] = lut[rgbin[0]];
    rgbout[1] = lut[rgbin[1]];
    rgbout[2] = lut[rgbin[2]];

    rgbout += 3;
    rgbin  += bpp;
    width --;
  };
}


/*
 * End of "$Id$".
 */
