/*
 * "$Id$"
 *
 *   Print plug-in EPSON ESC/P2 driver for the GIMP.
 *
 *   Copyright 1997-1998 Michael Sweet (mike@easysw.com)
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
 *   escp2_parameters()     - Return the parameter values for the given
 *                            parameter.
 *   escp2_imageable_area() - Return the imageable area of the page.
 *   escp2_print()          - Print an image to an EPSON printer.
 *   escp2_write()          - Send ESC/P2 graphics using TIFF packbits compression.
 *
 * Revision History:
 *
 *   $Log$
 *   Revision 1.7  1998/05/11 19:49:56  neo
 *   Updated print plug-in to version 2.0
 *
 *
 *   --Sven
 *
 *   Revision 1.10  1998/05/08  21:18:34  mike
 *   Now enable microweaving in 720 DPI mode.
 *
 *   Revision 1.9  1998/05/08  20:49:43  mike
 *   Updated to support media size, imageable area, and parameter functions.
 *   Added support for scaling modes - scale by percent or scale by PPI.
 *
 *   Revision 1.8  1998/01/21  21:33:47  mike
 *   Updated copyright.
 *
 *   Revision 1.7  1997/11/12  15:57:48  mike
 *   Minor changes for clean compiles under Digital UNIX.
 *
 *   Revision 1.7  1997/11/12  15:57:48  mike
 *   Minor changes for clean compiles under Digital UNIX.
 *
 *   Revision 1.6  1997/07/30  20:33:05  mike
 *   Final changes for 1.1 release.
 *
 *   Revision 1.6  1997/07/30  20:33:05  mike
 *   Final changes for 1.1 release.
 *
 *   Revision 1.5  1997/07/30  18:47:39  mike
 *   Added scaling, orientation, and offset options.
 *
 *   Revision 1.4  1997/07/15  20:57:11  mike
 *   Updated ESC 800/1520/3000 output code to use vertical spacing of 5 instead of 40.
 *
 *   Revision 1.3  1997/07/03  13:21:15  mike
 *   Updated documentation for 1.0 release.
 *
 *   Revision 1.2  1997/07/03  13:03:57  mike
 *   Added horizontal offset to try to center image.
 *   Got rid of initial vertical positioning since the top margin is
 *   now set properly.
 *
 *   Revision 1.2  1997/07/03  13:03:57  mike
 *   Added horizontal offset to try to center image.
 *   Got rid of initial vertical positioning since the top margin is
 *   now set properly.
 *
 *   Revision 1.1  1997/07/02  13:51:53  mike
 *   Initial revision
 */

#include "print.h"


/*
 * Local functions...
 */

static void	escp2_write(FILE *, unsigned char *, int, int, int, int, int, int);


/*
 * 'escp2_parameters()' - Return the parameter values for the given parameter.
 */

char **					/* O - Parameter values */
escp2_parameters(int  model,		/* I - Printer model */
                 char *ppd_file,	/* I - PPD file (not used) */
                 char *name,		/* I - Name of parameter */
                 int  *count)		/* O - Number of values */
{
  int		i;
  char		**p,
		**valptrs;
  static char	*media_sizes[] =
		{
		  "Letter",
		  "Legal",
		  "A4",
		  "Tabloid",
		  "A3",
		  "12x18"
		};
  static char	*resolutions[] =
		{
		  "360 DPI",
		  "720 DPI"
		};


  if (count == NULL)
    return (NULL);

  *count = 0;

  if (name == NULL)
    return (NULL);

  if (strcmp(name, "PageSize") == 0)
  {
    if (model == 5 || model == 2)
      *count = 6;
    else
      *count = 3;

    p = media_sizes;
  }
  else if (strcmp(name, "Resolution") == 0)
  {
    *count = 2;
    p = resolutions;
  }
  else
    return (NULL);

  valptrs = g_new(char *, *count);
  for (i = 0; i < *count; i ++)
    valptrs[i] = strdup(p[i]);

  return (valptrs);
}


/*
 * 'escp2_imageable_area()' - Return the imageable area of the page.
 */

void
escp2_imageable_area(int  model,	/* I - Printer model */
                     char *ppd_file,	/* I - PPD file (not used) */
                     char *media_size,	/* I - Media size */
                     int  *left,	/* O - Left position in points */
                     int  *right,	/* O - Right position in points */
                     int  *bottom,	/* O - Bottom position in points */
                     int  *top)		/* O - Top position in points */
{
  int	width, length;			/* Size of page */


  default_media_size(model, ppd_file, media_size, &width, &length);

  switch (model)
  {
    default :
        *left   = 14;
        *right  = width - 14;
        *top    = length - 14;
        *bottom = 40;
        break;

    case 3 :
    case 4 :
    case 5 :
        *left   = 8;
        *right  = width - 9;
        *top    = length - 32;
        *bottom = 40;
        break;
  };
}


/*
 * 'escp2_print()' - Print an image to an EPSON printer.
 */

void
escp2_print(int       model,		/* I - Model */
            char      *ppd_file,	/* I - PPD file (not used) */
            char      *resolution,	/* I - Resolution */
            char      *media_size,	/* I - Media size */
            char      *media_type,	/* I - Media type */
            char      *media_source,	/* I - Media source */
            int       output_type,	/* I - Output type (color/grayscale) */
            int       orientation,	/* I - Orientation of image */
            float     scaling,		/* I - Scaling of image */
            int       left,		/* I - Left offset of image (points) */
            int       top,		/* I - Top offset of image (points) */
            int       copies,		/* I - Number of copies */
            FILE      *prn,		/* I - File to print to */
            GDrawable *drawable,	/* I - Image to print */
            guchar    *lut,		/* I - Brightness lookup table */
            guchar    *cmap)		/* I - Colormap (for indexed images) */
{
  int		x, y;		/* Looping vars */
  int		xdpi, ydpi;	/* Resolution */
  int		n;		/* Output number */
  GPixelRgn	rgn;		/* Image region */
  unsigned char	*in,		/* Input pixels */
		*out,		/* Output pixels */
		*black,		/* Black bitmap data */
		*cyan,		/* Cyan bitmap data */
		*magenta,	/* Magenta bitmap data */
		*yellow;	/* Yellow bitmap data */
  int		page_left,	/* Left margin of page */
		page_right,	/* Right margin of page */
		page_top,	/* Top of page */
		page_bottom,	/* Bottom of page */
		page_width,	/* Width of page */
		page_height,	/* Height of page */
		page_length,	/* True length of page */
		out_width,	/* Width of image on page */
		out_height,	/* Height of image on page */
		out_bpp,	/* Output bytes per pixel */
		temp_width,	/* Temporary width of image on page */
		temp_height,	/* Temporary height of image on page */
		landscape,	/* True if we rotate the output 90 degrees */
		length,		/* Length of raster data */
		errdiv,		/* Error dividend */
		errmod,		/* Error modulus */
		errval,		/* Current error value */
		errline,	/* Current raster line */
		errlast;	/* Last raster line loaded */
  convert_t	colorfunc;	/* Color conversion function... */


 /*
  * Setup a read-only pixel region for the entire image...
  */

  gimp_pixel_rgn_init(&rgn, drawable, 0, 0, drawable->width, drawable->height,
                      FALSE, FALSE);

 /*
  * Choose the correct color conversion function...
  */

  if (drawable->bpp < 3 && cmap == NULL)
    output_type = OUTPUT_GRAY;		/* Force grayscale output */

  if (output_type == OUTPUT_COLOR)
  {
    out_bpp = 3;

    if (drawable->bpp >= 3)
      colorfunc = rgb_to_rgb;
    else
      colorfunc = indexed_to_rgb;
  }
  else
  {
    out_bpp = 1;

    if (drawable->bpp >= 3)
      colorfunc = rgb_to_gray;
    else if (cmap == NULL)
      colorfunc = gray_to_gray;
    else
      colorfunc = indexed_to_gray;
  };

 /*
  * Figure out the output resolution...
  */

  xdpi = ydpi = atoi(resolution);

 /*
  * Compute the output size...
  */

  landscape   = 0;
  escp2_imageable_area(model, ppd_file, media_size, &page_left, &page_right,
                       &page_bottom, &page_top);

  page_width  = page_right - page_left;
  page_height = page_top - page_bottom;

  default_media_size(model, ppd_file, media_size, &n, &page_length);

 /*
  * Portrait width/height...
  */

  if (scaling < 0.0)
  {
   /*
    * Scale to pixels per inch...
    */

    out_width  = drawable->width * -72.0 / scaling;
    out_height = drawable->height * -72.0 / scaling;
  }
  else
  {
   /*
    * Scale by percent...
    */

    out_width  = page_width * scaling / 100.0;
    out_height = out_width * drawable->height / drawable->width;
    if (out_height > page_height)
    {
      out_height = page_height * scaling / 100.0;
      out_width  = out_height * drawable->width / drawable->height;
    };
  };

 /*
  * Landscape width/height...
  */

  if (scaling < 0.0)
  {
   /*
    * Scale to pixels per inch...
    */

    temp_width  = drawable->height * -72.0 / scaling;
    temp_height = drawable->width * -72.0 / scaling;
  }
  else
  {
   /*
    * Scale by percent...
    */

    temp_width  = page_width * scaling / 100.0;
    temp_height = temp_width * drawable->width / drawable->height;
    if (temp_height > page_height)
    {
      temp_height = page_height;
      temp_width  = temp_height * drawable->height / drawable->width;
    };
  };

 /*
  * See which orientation has the greatest area (or if we need to rotate the
  * image to fit it on the page...)
  */

  if (orientation == ORIENT_AUTO)
  {
    if (scaling < 0.0)
    {
      if ((out_width > page_width && out_height < page_width) ||
          (out_height > page_height && out_width < page_height))
	orientation = ORIENT_LANDSCAPE;
      else
	orientation = ORIENT_PORTRAIT;
    }
    else
    {
      if ((temp_width * temp_height) > (out_width * out_height))
	orientation = ORIENT_LANDSCAPE;
      else
	orientation = ORIENT_PORTRAIT;
    };
  };

  if (orientation == ORIENT_LANDSCAPE)
  {
    out_width  = temp_width;
    out_height = temp_height;
    landscape  = 1;

   /*
    * Swap left/top offsets...
    */

    x    = top;
    top  = left;
    left = x;
  };

  if (top < 0 || left < 0)
  {
    left = (page_width - out_width) / 2;
    top  = (page_height + out_height) / 2;
  };

 /*
  * Let the user know what we're doing...
  */

  gimp_progress_init("Printing...");

 /*
  * Send ESC/P2 initialization commands...
  */

  fputs("\033@", prn); 				/* ESC/P2 reset */

  fwrite("\033(G\001\000\001", 6, 1, prn);	/* Enter graphics mode */
  switch (ydpi)					/* Set line feed increment */
  {
    case 180 :
        fwrite("\033(U\001\000\024", 6, 1, prn);
        break;

    case 360 :
        fwrite("\033(U\001\000\012", 6, 1, prn);
        break;

    case 720 :
        fwrite("\033(U\001\000\005", 6, 1, prn);
        break;
  };

  fwrite("\033(C\002\000", 5, 1, prn);		/* Page length */
  n = ydpi * page_length / 72;
  putc(n & 255, prn);
  putc(n >> 8, prn);

  fwrite("\033(c\004\000", 5, 1, prn);		/* Top/bottom margins */
  n = ydpi * (page_length - page_top) / 72;
  putc(n & 255, prn);
  putc(n >> 8, prn);
  n = ydpi * (page_length - page_bottom) / 72;
  putc(n & 255, prn);
  putc(n >> 8, prn);

  fwrite("\033(V\002\000", 5, 1, prn);		/* Absolute vertical position */
  n = ydpi * (page_length - top) / 72;
  putc(n & 255, prn);
  putc(n >> 8, prn);

  switch (model)				/* Printer specific initialization */
  {
    case 0 : /* ESC */
        if (output_type == OUTPUT_COLOR && ydpi > 360)
      	  fwrite("\033(i\001\000\001", 6, 1, prn);	/* Microweave mode on */
        break;

    case 1 : /* ESC Pro, Pro XL, 400, 500 */
        fwrite("\033(e\002\000\000\001", 7, 1, prn);	/* Small dots */

        if (ydpi > 360)
      	  fwrite("\033(i\001\000\001", 6, 1, prn);	/* Microweave mode on */
        break;

    case 2 : /* ESC 1500 */
        fwrite("\033(e\002\000\000\001", 7, 1, prn);	/* Small dots */

        if (ydpi > 360)
      	  fwrite("\033(i\001\000\001", 6, 1, prn);	/* Microweave mode on */
        break;

    case 3 : /* ESC 600 */
    case 4 : /* ESC 800 */
    case 5 : /* 1520, 3000 */
	if (output_type == OUTPUT_GRAY)
	  fwrite("\033(K\002\000\000\001", 7, 1, prn);	/* Fast black printing */
	else
	  fwrite("\033(K\002\000\000\002", 7, 1, prn);	/* Color printing */

        fwrite("\033(e\002\000\000\002", 7, 1, prn);	/* Small dots */

        if (ydpi > 360)
      	  fwrite("\033(i\001\000\001", 6, 1, prn);	/* Microweave mode on */
        break;
  };

 /*
  * Convert image size to printer resolution...
  */

  out_width  = xdpi * out_width / 72;
  out_height = ydpi * out_height / 72;

  left = ydpi * left / 72;

 /*
  * Allocate memory for the raster data...
  */

  length = (out_width + 7) / 8;

  if (output_type == OUTPUT_GRAY)
  {
    black   = g_malloc(length);
    cyan    = NULL;
    magenta = NULL;
    yellow  = NULL;
  }
  else
  {
    cyan    = g_malloc(length);
    magenta = g_malloc(length);
    yellow  = g_malloc(length);
  
    if (model != 2)
      black = g_malloc(length);
    else
      black = NULL;
  };
    
 /*
  * Output the page, rotating as necessary...
  */

  if (landscape)
  {
    in  = g_malloc(drawable->height * drawable->bpp);
    out = g_malloc(drawable->height * out_bpp);

    errdiv  = drawable->width / out_height;
    errmod  = drawable->width % out_height;
    errval  = 0;
    errlast = -1;
    errline  = drawable->width - 1;
    
    for (x = 0; x < out_height; x ++)
    {
#ifdef DEBUG
      printf("escp2_print: x = %d, line = %d, val = %d, mod = %d, height = %d\n",
             x, errline, errval, errmod, out_height);
#endif /* DEBUG */

      if ((x & 255) == 0)
        gimp_progress_update((double)x / (double)out_height);

      if (errline != errlast)
      {
        errlast = errline;
        gimp_pixel_rgn_get_col(&rgn, in, errline, 0, drawable->height);
      };

      (*colorfunc)(in, out, drawable->height, drawable->bpp, lut, cmap);

      if (output_type == OUTPUT_GRAY)
      {
        dither_black(out, x, drawable->height, out_width, black);
        escp2_write(prn, black, length, 0, ydpi, model, out_width, left);
      }
      else
      {
        dither_cmyk(out, x, drawable->height, out_width, cyan, magenta,
                    yellow, black);

        escp2_write(prn, cyan, length, 2, ydpi, model, out_width, left);
        escp2_write(prn, magenta, length, 1, ydpi, model, out_width, left);
        escp2_write(prn, yellow, length, 4, ydpi, model, out_width, left);
        if (black != NULL)
          escp2_write(prn, black, length, 0, ydpi, model, out_width, left);
      };

      fwrite("\033(v\002\000\001\000", 7, 1, prn);	/* Feed one line */

      errval += errmod;
      errline -= errdiv;
      if (errval >= out_height)
      {
        errval -= out_height;
        errline --;
      };
    };
  }
  else
  {
    in  = g_malloc(drawable->width * drawable->bpp);
    out = g_malloc(drawable->width * out_bpp);

    errdiv  = drawable->height / out_height;
    errmod  = drawable->height % out_height;
    errval  = 0;
    errlast = -1;
    errline  = 0;
    
    for (y = 0; y < out_height; y ++)
    {
#ifdef DEBUG
      printf("escp2_print: y = %d, line = %d, val = %d, mod = %d, height = %d\n",
             y, errline, errval, errmod, out_height);
#endif /* DEBUG */

      if ((y & 255) == 0)
        gimp_progress_update((double)y / (double)out_height);

      if (errline != errlast)
      {
        errlast = errline;
        gimp_pixel_rgn_get_row(&rgn, in, 0, errline, drawable->width);
      };

      (*colorfunc)(in, out, drawable->width, drawable->bpp, lut, cmap);

      if (output_type == OUTPUT_GRAY)
      {
        dither_black(out, y, drawable->width, out_width, black);
        escp2_write(prn, black, length, 0, ydpi, model, out_width, left);
      }
      else
      {
        dither_cmyk(out, y, drawable->width, out_width, cyan, magenta,
                    yellow, black);

        escp2_write(prn, cyan, length, 2, ydpi, model, out_width, left);
        escp2_write(prn, magenta, length, 1, ydpi, model, out_width, left);
        escp2_write(prn, yellow, length, 4, ydpi, model, out_width, left);
        if (black != NULL)
          escp2_write(prn, black, length, 0, ydpi, model, out_width, left);
      };

      fwrite("\033(v\002\000\001\000", 7, 1, prn);	/* Feed one line */

      errval += errmod;
      errline += errdiv;
      if (errval >= out_height)
      {
        errval -= out_height;
        errline ++;
      };
    };
  };

 /*
  * Cleanup...
  */

  g_free(in);
  g_free(out);

  if (black != NULL)
    g_free(black);
  if (cyan != NULL)
  {
    g_free(cyan);
    g_free(magenta);
    g_free(yellow);
  };

  putc('\014', prn);			/* Eject page */
  fputs("\033@", prn);			/* ESC/P2 reset */
}


/*
 * 'escp2_write()' - Send ESC/P2 graphics using TIFF packbits compression.
 */

void
escp2_write(FILE          *prn,		/* I - Print file or command */
            unsigned char *line,	/* I - Output bitmap data */
            int           length,	/* I - Length of bitmap data */
            int           plane,	/* I - True if this is the last plane */
            int           ydpi,		/* I - Vertical resolution */
            int           model,	/* I - Printer model */
            int           width,	/* I - Printed width */
            int           offset)	/* I - Offset from left side */
{
  unsigned char	comp_buf[1536],		/* Compression buffer */
		*comp_ptr,		/* Current slot in buffer */
		*start,			/* Start of compressed data */
		repeat;			/* Repeating char */
  int		count,			/* Count of compressed bytes */
		tcount;			/* Temporary count < 128 */
  static int	last_plane = 0;		/* Last color plane printed */


 /*
  * Don't send blank lines...
  */

  if (line[0] == 0 && memcmp(line, line + 1, length - 1) == 0)
    return;

 /*
  * Compress using TIFF "packbits" run-length encoding...
  */

  comp_ptr = comp_buf;

  while (length > 0)
  {
   /*
    * Get a run of non-repeated chars...
    */

    start  = line;
    line   += 2;
    length -= 2;

    while (length > 0 && (line[-2] != line[-1] || line[-1] != line[0]))
    {
      line ++;
      length --;
    };

    line   -= 2;
    length += 2;

   /*
    * Output the non-repeated sequences (max 128 at a time).
    */

    count = line - start;
    while (count > 0)
    {
      tcount = count > 128 ? 128 : count;

      comp_ptr[0] = tcount - 1;
      memcpy(comp_ptr + 1, start, tcount);

      comp_ptr += tcount + 1;
      start    += tcount;
      count    -= tcount;
    };

    if (length <= 0)
      break;

   /*
    * Find the repeated sequences...
    */

    start  = line;
    repeat = line[0];

    line ++;
    length --;

    while (length > 0 && *line == repeat)
    {
      line ++;
      length --;
    };

   /*
    * Output the repeated sequences (max 128 at a time).
    */

    count = line - start;
    while (count > 0)
    {
      tcount = count > 128 ? 128 : count;

      comp_ptr[0] = 1 - tcount;
      comp_ptr[1] = repeat;

      comp_ptr += 2;
      count    -= tcount;
    };
  };

 /*
  * Set the print head position.
  */

  putc('\r', prn);
  fprintf(prn, "\033\\%c%c", offset & 255, offset >> 8);

 /*
  * Set the color if necessary...
  */

  if (last_plane != plane)
  {
    last_plane = plane;
    fprintf(prn, "\033r%c", plane);
  };

 /*
  * Send a line of raster graphics...
  */

  switch (ydpi)				/* Raster graphics header */
  {
    case 180 :
        fwrite("\033.\001\024\024\001", 6, 1, prn);
        break;
    case 360 :
        fwrite("\033.\001\012\012\001", 6, 1, prn);
        break;
    case 720 :
        if (model == 3)
          fwrite("\033.\001\050\005\001", 6, 1, prn);
        else
          fwrite("\033.\001\005\005\001", 6, 1, prn);
        break;
  };

  putc(width & 255, prn);		/* Width of raster line in pixels */
  putc(width >> 8, prn);

  fwrite(comp_buf, comp_ptr - comp_buf, 1, prn);
}


/*
 * End of "$Id$".
 */
