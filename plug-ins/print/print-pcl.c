/*
 * "$Id$"
 *
 *   Print plug-in HP PCL driver for the GIMP.
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
 *   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 * Contents:
 *
 *   pcl_print() - Print an image to an HP printer.
 *   pcl_mode0() - Send PCL graphics using mode 0 (no) compression.
 *   pcl_mode2() - Send PCL graphics using mode 2 (TIFF) compression.
 *
 * Revision History:
 *
 *   $Log$
 *   Revision 1.2  1998/01/25 09:29:26  yosh
 *   Plugin updates
 *   Properly generated aa Makefile (still not built by default)
 *   Sven's no args script patch
 *
 *   -Yosh
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
 *   Revision 1.6  1997/10/02  17:57:26  mike
 *   Updated positioning code to use "decipoint" commands.
 *
 *   Revision 1.5  1997/07/30  20:33:05  mike
 *   Final changes for 1.1 release.
 *
 *   Revision 1.4  1997/07/30  18:47:39  mike
 *   Added scaling, orientation, and offset options.
 *
 *   Revision 1.3  1997/07/03  13:24:12  mike
 *   Updated documentation for 1.0 release.
 *
 *   Revision 1.2  1997/07/02  18:48:14  mike
 *   Added mode 2 compression code.
 *   Fixed bug in pcl_mode0 and pcl_mode2 - wasn't sending 'V' or 'W' at
 *   the right times.
 *
 *   Revision 1.2  1997/07/02  18:48:14  mike
 *   Added mode 2 compression code.
 *   Fixed bug in pcl_mode0 and pcl_mode2 - wasn't sending 'V' or 'W' at
 *   the right times.
 *
 *   Revision 1.1  1997/07/02  13:51:53  mike
 *   Initial revision
 */

#include "print.h"


/*
 * Local functions...
 */

static void	pcl_mode0(FILE *, unsigned char *, int, int);
static void	pcl_mode2(FILE *, unsigned char *, int, int);


/*
 * 'pcl_print()' - Print an image to an HP printer.
 */

void
pcl_print(FILE      *prn,		/* I - Print file or command */
          GDrawable *drawable,		/* I - Image to print */
          int       media_size,		/* I - Output size */
          int       xdpi,		/* I - Horizontal resolution */
          int       ydpi,		/* I - Vertical resolution */
          int       output_type,	/* I - Color or grayscale? */
          int       model,		/* I - Model of printer */
          guchar    *lut,		/* I - Brightness lookup table */
          guchar    *cmap,		/* I - Colormap (for indexed images) */
          int       orientation,	/* I - Orientation of image */
          int       scaling,		/* I - Scaling of image */
          int       left,		/* I - Left offset of image (10ths) */
          int       top)		/* I - Top offset of image (10ths) */
{
  int		x, y;		/* Looping vars */
  GPixelRgn	rgn;		/* Image region */
  unsigned char	*in,		/* Input pixels */
		*out,		/* Output pixels */
		*black,		/* Black bitmap data */
		*cyan,		/* Cyan bitmap data */
		*magenta,	/* Magenta bitmap data */
		*yellow;	/* Yellow bitmap data */
  int		page_width,	/* Width of page */
		page_height,	/* Height of page */
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
  void		(*writefunc)(FILE *, unsigned char *, int, int);
				/* PCL output function */


 /*
  * Setup a read-only pixel region for the entire image...
  */

  gimp_pixel_rgn_init(&rgn, drawable, 0, 0, drawable->width, drawable->height,
                      FALSE, FALSE);

 /*
  * Choose the correct color conversion function...
  */

  if ((drawable->bpp < 3 && cmap == NULL) || model <= 500)
    output_type = OUTPUT_GRAY;		/* Force grayscale output */

  if (output_type == OUTPUT_COLOR)
  {
    out_bpp = 3;

    if (drawable->bpp >= 3)
      colorfunc = rgb_to_rgb;
    else
      colorfunc = indexed_to_rgb;

    if (model == 800)
      xdpi = ydpi = 300;
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
  * Compute the output size...
  */

  landscape   = 0;
  page_width  = media_width(media_size, xdpi);
  page_height = media_height(media_size, ydpi);

 /*
  * Portrait width/height...
  */

  out_width  = page_width * scaling / 100;
  out_height = out_width * ydpi / xdpi * drawable->height / drawable->width;
  if (out_height > page_height)
  {
    out_height = page_height;
    out_width  = out_height * xdpi / ydpi * drawable->width / drawable->height;
  };

 /*
  * Landscape width/height...
  */

  temp_width  = page_width * scaling / 100;
  temp_height = temp_width * ydpi / xdpi * drawable->width / drawable->height;
  if (temp_height > page_height)
  {
    temp_height = page_height;
    temp_width  = temp_height * xdpi / ydpi * drawable->height / drawable->width;
  };

 /*
  * See which orientation has the greatest area...
  */

  if ((temp_width * temp_height) > (out_width * out_height) &&
      orientation != ORIENT_PORTRAIT)
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

 /*
  * Let the user know what we're doing...
  */

  gimp_progress_init("Printing...");

 /*
  * Send PCL initialization commands...
  */

  fputs("\033E", prn); 		/* PCL reset */

  switch (media_size)		/* Set media size... */
  {
    case MEDIA_LETTER :
        fputs("\033&l2A", prn);
        break;
    case MEDIA_LEGAL :
        fputs("\033&l3A", prn);
        break;
    case MEDIA_TABLOID :
        fputs("\033&l6A", prn);
        break;
    case MEDIA_A4 :
        fputs("\033&l26A", prn);
        break;
    case MEDIA_A3 :
        fputs("\033&l27A", prn);
        break;
  };

  if (xdpi != ydpi)		/* Set resolution */
  {
   /*
    * Send 26-byte configure image data command with horizontal and
    * vertical resolutions as well as a color count...
    */

    fputs("\033*g26W", prn);
    putc(2, prn);			/* Format 2 */
    if (output_type == OUTPUT_COLOR)
      putc(4, prn);			/* # output planes */
    else
      putc(1, prn);			/* # output planes */

    putc(xdpi >> 8, prn);		/* Black resolution */
    putc(xdpi, prn);
    putc(ydpi >> 8, prn);
    putc(ydpi, prn);
    putc(0, prn);
    putc(2, prn);			/* # of black levels */

    putc(xdpi >> 8, prn);		/* Cyan resolution */
    putc(xdpi, prn);
    putc(ydpi >> 8, prn);
    putc(ydpi, prn);
    putc(0, prn);
    putc(2, prn);			/* # of cyan levels */

    putc(xdpi >> 8, prn);		/* Magenta resolution */
    putc(xdpi, prn);
    putc(ydpi >> 8, prn);
    putc(ydpi, prn);
    putc(0, prn);
    putc(2, prn);			/* # of magenta levels */

    putc(xdpi >> 8, prn);		/* Yellow resolution */
    putc(xdpi, prn);
    putc(ydpi >> 8, prn);
    putc(ydpi, prn);
    putc(0, prn);
    putc(2, prn);			/* # of yellow levels */
  }
  else
  {
    fprintf(prn, "\033*t%dR", xdpi);	/* Simple resolution */
    if (output_type == OUTPUT_COLOR)
    {
      if (model == 501 || model == 1200)
        fputs("\033*r-3U", prn);	/* Simple CMY color */
      else
        fputs("\033*r-4U", prn);	/* Simple KCMY color */
    };
  };

  if (model < 3 || model == 500)
    fputs("\033*b0M", prn);	/* Mode 0 (no compression) */
  else
    fputs("\033*b2M", prn);	/* Mode 2 (TIFF) */

  if (left < 0 || top < 0)
  {
    left = (page_width - out_width) / 2;
    top  = (page_height - out_height) / 2;
  }
  else
  {
    left *= 30;
    top  *= 30;
  };

  fprintf(prn, "\033&a%dH", 720 * left / xdpi);	/* Set left raster position */
  fprintf(prn, "\033&a%dV", 720 * top / ydpi);	/* Set top raster position */
  fprintf(prn, "\033*r%dS", out_width);		/* Set raster width */
  fprintf(prn, "\033*r%dT", out_height);	/* Set raster height */

  fputs("\033*r1A", prn); 	/* Start GFX */

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
  
    if (model != 501 && model != 1200)
      black = g_malloc(length);
    else
      black = NULL;
  };
    
 /*
  * Output the page, rotating as necessary...
  */

  if (model < 3 || model == 500)
    writefunc = pcl_mode0;
  else
    writefunc = pcl_mode2;

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
      printf("pcl_print: x = %d, line = %d, val = %d, mod = %d, height = %d\n",
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
        (*writefunc)(prn, black, length, 1);
      }
      else
      {
        dither_cmyk(out, x, drawable->height, out_width, cyan, magenta,
                    yellow, black);

        if (black != NULL)
          (*writefunc)(prn, black, length, 0);
        (*writefunc)(prn, cyan, length, 0);
        (*writefunc)(prn, magenta, length, 0);
        (*writefunc)(prn, yellow, length, 1);
      };

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
      printf("pcl_print: y = %d, line = %d, val = %d, mod = %d, height = %d\n",
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
        (*writefunc)(prn, black, length, 1);
      }
      else
      {
        dither_cmyk(out, y, drawable->width, out_width, cyan, magenta,
                    yellow, black);

        if (black != NULL)
          (*writefunc)(prn, black, length, 0);
        (*writefunc)(prn, cyan, length, 0);
        (*writefunc)(prn, magenta, length, 0);
        (*writefunc)(prn, yellow, length, 1);
      };

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

  switch (model)			/* End raster graphics */
  {
    case 1 :
    case 2 :
    case 3 :
    case 500 :
        fputs("\033*rB", prn);
        break;
    default :
        fputs("\033*rbC", prn);
        break;
  };

  fputs("\033&l0H", prn);		/* Eject page */
  fputs("\033E", prn);			/* PCL reset */
}


/*
 * 'pcl_mode0()' - Send PCL graphics using mode 0 (no) compression.
 */

void
pcl_mode0(FILE          *prn,		/* I - Print file or command */
          unsigned char *line,		/* I - Output bitmap data */
          int           length,		/* I - Length of bitmap data */
          int           last_plane)	/* I - True if this is the last plane */
{
  fprintf(prn, "\033*b%d%c", length, last_plane ? 'W' : 'V');
  fwrite(line, length, 1, prn);
}


/*
 * 'pcl_mode2()' - Send PCL graphics using mode 2 (TIFF) compression.
 */

void
pcl_mode2(FILE          *prn,		/* I - Print file or command */
          unsigned char *line,		/* I - Output bitmap data */
          int           length,		/* I - Length of bitmap data */
          int           last_plane)	/* I - True if this is the last plane */
{
  unsigned char	comp_buf[1536],		/* Compression buffer */
		*comp_ptr,		/* Current slot in buffer */
		*start,			/* Start of compressed data */
		repeat;			/* Repeating char */
  int		count,			/* Count of compressed bytes */
		tcount;			/* Temporary count < 128 */


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
  * Send a line of raster graphics...
  */

  fprintf(prn, "\033*b%d%c", (int)(comp_ptr - comp_buf), last_plane ? 'W' : 'V');
  fwrite(comp_buf, comp_ptr - comp_buf, 1, prn);
}


/*
 * End of "$Id$".
 */
