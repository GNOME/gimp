/*
 * "$Id$"
 *
 *   Print plug-in Adobe PostScript driver for the GIMP.
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
 *   ps_print() - Print an image to a PostScript printer.
 *   ps_hex()   - Print binary data as a series of hexadecimal numbers.
 *
 * Revision History:
 *
 *   $Log$
 *   Revision 1.1.1.1  1997/11/24 22:04:34  sopwith
 *   Let's try this import one last time.
 *
 *   Revision 1.3  1997/11/18 03:04:26  nobody
 *   fixed ugly comment-bugs introduced by evil darkwing
 *   keep out configuration empty dirs
 *   	--darkwing
 *
 *   Revision 1.2  1997/11/17 05:43:57  nobody
 *   updated ChangeLog
 *   dropped non-working doc/Makefile entries
 *   applied many fixes from the registry as well as the devel ML
 *   applied missing patches by Art Haas
 *
 *   	--darkwing
 *
 *   Revision 1.8  1997/11/12  15:57:48  mike
 *   Minor changes for clean compiles under Digital UNIX.
 *
 *   Revision 1.8  1997/11/12  15:57:48  mike
 *   Minor changes for clean compiles under Digital UNIX.
 *
 *   Revision 1.7  1997/07/30  20:33:05  mike
 *   Final changes for 1.1 release.
 *
 *   Revision 1.7  1997/07/30  20:33:05  mike
 *   Final changes for 1.1 release.
 *
 *   Revision 1.6  1997/07/30  18:47:39  mike
 *   Added scaling, orientation, and offset options.
 *
 *   Revision 1.5  1997/07/26  18:38:55  mike
 *   Bug - was using asctime instead of ctime...  D'oh!
 *
 *   Revision 1.4  1997/07/26  18:19:54  mike
 *   Fixed positioning/scaling bug.
 *
 *   Revision 1.3  1997/07/03  13:26:46  mike
 *   Updated documentation for 1.0 release.
 *
 *   Revision 1.2  1997/07/02  18:49:36  mike
 *   Forgot to free memory buffers...
 *
 *   Revision 1.2  1997/07/02  18:49:36  mike
 *   Forgot to free memory buffers...
 *
 *   Revision 1.1  1997/07/02  13:51:53  mike
 *   Initial revision
 */

#include "print.h"
#include <time.h>


/*
 * Local functions...
 */

static void	ps_hex(FILE *, guchar *, int);


/*
 * 'ps_print()' - Print an image to a PostScript printer.
 */

void
ps_print(FILE      *prn,	/* I - File to print to */
         GDrawable *drawable,	/* I - Image to print */
         int       media_size,	/* I - Size of output */
         int       xdpi,	/* I - Horizontal resolution (always 72) */
         int       ydpi,	/* I - Vertical resolution (always 72) */
         int       output_type,	/* I - Output type (color/grayscale) */
         int       model,	/* I - Model (ignored) */
         guchar    *lut,	/* I - Brightness lookup table */
         guchar    *cmap,	/* I - Colormap (for indexed images) */
         int       orientation,	/* I - Orientation of image */
         int       scaling,	/* I - Scaling of image */
         int       left,	/* I - Left offset of image (10ths) */
         int       top)		/* I - Top offset of image (10ths) */
{
  int		x, y;		/* Looping vars */
  GPixelRgn	rgn;		/* Image region */
  guchar	*in,		/* Input pixels from image */
		*out,		/* Output pixels for printer */
		*outptr;	/* Current output pixel */
  int		page_width,	/* Width of page */
		page_height,	/* Height of page */
		out_width,	/* Width of image on page */
		out_height,	/* Height of image on page */
		out_bpp,	/* Output bytes per pixel */
		temp_width,	/* Temporary width of image on page */
		temp_height,	/* Temporary height of image on page */
		landscape;	/* True if we rotate the output 90 degrees */
  time_t	curtime;	/* Current time of day */
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
  * Output a standard PostScript header with DSC comments...
  */

  curtime = time(NULL);

  fputs("%!PS-Adobe-3.0\n", prn);
  fputs("%%Creator: " PLUG_IN_NAME " plug-in V" PLUG_IN_VERSION " for GIMP.\n", prn);
  fprintf(prn, "%%%%CreationDate: %s", ctime(&curtime));
  fputs("%%Copyright: 1997 by Michael Sweet (mike@easysw.com)\n", prn);
  fprintf(prn, "%%%%BoundingBox: %d %d %d %d\n",
          (page_width - out_width) / 2 + 18, (page_height - out_height) / 2 + 36,
          (page_width + out_width) / 2 + 18, (page_height + out_height) / 2 + 36);
  fputs("%%DocumentData: Clean7Bit\n", prn);
  fprintf(prn, "%%%%LanguageLevel: %d\n", output_type + 1);
  fputs("%%Pages: 1\n", prn);
  fputs("%%Orientation: Portrait\n", prn);
  fputs("%%EndComments\n", prn);

 /*
  * Output the page, rotating as necessary...
  */

  fputs("%%Page: 1\n", prn);
  fputs("gsave\n", prn);

  if (top < 0 || left < 0)
  {
    left = (page_width - out_width) / 2 + 18;
    top  = (page_height - out_height) / 2 + 36;
  }
  else
  {
    left = 72 * left / 10 + 18;
    top  = page_height - out_height - 72 * top / 10 + 36;
  };

  fprintf(prn, "%d %d translate\n", left, top);
  fprintf(prn, "%d %d scale\n", out_width, out_height);

  if (landscape)
  {
    in  = g_malloc(drawable->height * drawable->bpp);
    out = g_malloc(drawable->height * out_bpp);

    fprintf(prn, "/picture %d string def\n", drawable->height * out_bpp);

    if (output_type == OUTPUT_GRAY)
      fprintf(prn, "%d %d 8 [%d 0 0 %d 0 %d] {currentfile picture readhexstring pop} image\n",
              drawable->height, drawable->width,
              drawable->height, drawable->width, 0);
    else
      fprintf(prn, "%d %d 8 [%d 0 0 %d 0 %d] {currentfile picture readhexstring pop} false 3 colorimage\n",
              drawable->height, drawable->width,
              drawable->height, drawable->width, 0);

    for (x = 0; x < drawable->width; x ++)
    {
      if ((x & 15) == 0)
        gimp_progress_update((double)x / (double)drawable->width);

      gimp_pixel_rgn_get_col(&rgn, in, x, 0, drawable->height);
      (*colorfunc)(in, out, drawable->height, drawable->bpp, lut, cmap);

      ps_hex(prn, out, drawable->height * out_bpp);
    };
  }
  else
  {
    in  = g_malloc(drawable->width * drawable->bpp);
    out = g_malloc(drawable->width * out_bpp);

    fprintf(prn, "/picture %d string def\n", drawable->width * out_bpp);

    if (output_type == OUTPUT_GRAY)
      fprintf(prn, "%d %d 8 [%d 0 0 %d 0 %d] {currentfile picture readhexstring pop} image\n",
              drawable->width, drawable->height,
              drawable->width, -drawable->height, drawable->height); 
    else
      fprintf(prn, "%d %d 8 [%d 0 0 %d 0 %d] {currentfile picture readhexstring pop} false 3 colorimage\n",
              drawable->width, drawable->height,
              drawable->width, -drawable->height, drawable->height); 

    for (y = 0; y < drawable->height; y ++)
    {
      if ((y & 15) == 0)
        gimp_progress_update((double)y / (double)drawable->height);

      gimp_pixel_rgn_get_row(&rgn, in, 0, y, drawable->width);
      (*colorfunc)(in, out, drawable->width, drawable->bpp, lut, cmap);

      ps_hex(prn, out, drawable->width * out_bpp);
    };
  };

  g_free(in);
  g_free(out);

  fputs("grestore\n", prn);
  fputs("showpage\n", prn);
  fputs("%%EndPage\n", prn);
  fputs("%%EOF\n", prn);
}


/*
 * 'ps_hex()' - Print binary data as a series of hexadecimal numbers.
 */

static void
ps_hex(FILE   *prn,	/* I - File to print to */
       guchar *data,	/* I - Data to print */
       int    length)	/* I - Number of bytes to print */
{
  static char	*hex = "0123456789ABCDEF";


  while (length > 0)
  {
   /*
    * Put the hex chars out to the file; note that we don't use fprintf()
    * for speed reasons...
    */

    putc(hex[*data >> 4], prn);
    putc(hex[*data & 15], prn);

    data ++;
    length --;
  };

  putc('\n', prn);
}


/*
 * End of "$Id$".
 */
