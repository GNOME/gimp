/*
 * "$Id$"
 *
 *   Print plug-in Adobe PostScript driver for the GIMP.
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
 *   ps_parameters()     - Return the parameter values for the given
 *                            parameter.
 *   ps_media_size()     - Return the size of the page.
 *   ps_imageable_area() - Return the imageable area of the page.
 *   ps_print()          - Print an image to a PostScript printer.
 *   ps_hex()            - Print binary data as a series of hexadecimal numbers.
 *   ps_ascii85()        - Print binary data as a series of base-85 numbers.
 *
 * Revision History:
 *
 *   $Log$
 *   Revision 1.8  1998/05/14 00:32:49  yosh
 *   updated print plugin
 *
 *   stubbed out nonworking frac code
 *
 *   -Yosh
 *
 *   Revision 1.12  1998/05/11  23:56:56  mike
 *   Removed unused outptr variable.
 *
 *   Revision 1.11  1998/05/08  19:20:50  mike
 *   Updated to support PPD files, media size, imageable area, and parameter
 *   functions.
 *   Added support for scaling modes - scale by percent or scale by PPI.
 *   Updated Ascii85 output - some Level 2 printers are buggy and won't accept
 *   whitespace in the data stream.
 *   Now use image dictionaries with Level 2 printers - allows interpolation
 *   flag to be sent (not all printers use this flag).
 *
 *   Revision 1.10  1998/01/22  15:38:46  mike
 *   Updated copyright notice.
 *   Whoops - wasn't encoding correctly for portrait output to level 2 printers!
 *
 *   Revision 1.9  1998/01/21  21:33:47  mike
 *   Added support for Level 2 filters; images are now sent in hex or
 *   base-85 ASCII as necessary (faster printing).
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
/*#define DEBUG*/


/*
 * Local variables...
 */

static FILE	*ps_ppd = NULL;
static char	*ps_ppd_file = NULL;


/*
 * Local functions...
 */

static void	ps_hex(FILE *, guchar *, int);
static void	ps_ascii85(FILE *, guchar *, int, int);
static char	*ppd_find(char *, char *, char *, int *);


/*
 * 'ps_parameters()' - Return the parameter values for the given parameter.
 */

char **				/* O - Parameter values */
ps_parameters(int  model,	/* I - Printer model */
              char *ppd_file,	/* I - PPD file (not used) */
              char *name,	/* I - Name of parameter */
              int  *count)	/* O - Number of values */
{
  int		i;
  char		line[1024],
		lname[255],
		loption[255];
  char		**valptrs;
  static char	*media_sizes[] =
		{
		  "Letter",
		  "Legal",
		  "A4",
		  "Tabloid",
		  "A3",
		  "12x18"
		};


  if (count == NULL)
    return (NULL);

  *count = 0;

  if (ppd_file == NULL || name == NULL)
    return (NULL);

  if (ps_ppd_file == NULL || strcmp(ps_ppd_file, ppd_file) != 0)
  {
    if (ps_ppd != NULL)
      fclose(ps_ppd);

    ps_ppd = fopen(ppd_file, "r");

    if (ps_ppd == NULL)
      ps_ppd_file = NULL;
    else
      ps_ppd_file = ppd_file;
  };

  if (ps_ppd == NULL)
  {
    if (strcmp(name, "PageSize") == 0)
    {
      *count = 6;

      valptrs = g_new(char *, 6);
      for (i = 0; i < 6; i ++)
        valptrs[i] = strdup(media_sizes[i]);

      return (valptrs);
    }
    else
      return (NULL);
  };

  rewind(ps_ppd);
  *count = 0;

  valptrs = g_new(char *, 100);

  while (fgets(line, sizeof(line), ps_ppd) != NULL)
  {
    if (line[0] != '*')
      continue;

    if (sscanf(line, "*%s %[^/:]", lname, loption) != 2)
      continue;

    if (strcasecmp(lname, name) == 0)
    {
      valptrs[*count] = strdup(loption);
      (*count) ++;
    };
  };

  if (*count == 0)
  {
    g_free(valptrs);
    return (NULL);
  }
  else
    return (valptrs);
}


/*
 * 'ps_media_size()' - Return the size of the page.
 */

void
ps_media_size(int  model,		/* I - Printer model */
              char *ppd_file,		/* I - PPD file (not used) */
              char *media_size,		/* I - Media size */
              int  *width,		/* O - Width in points */
              int  *length)		/* O - Length in points */
{
  char	*dimensions;			/* Dimensions of media size */


#ifdef DEBUG
  printf("ps_media_size(%d, \'%s\', \'%s\', %08x, %08x)\n", model, ppd_file,
         media_size, width, length);
#endif /* DEBUG */

  if ((dimensions = ppd_find(ppd_file, "PaperDimension", media_size, NULL)) != NULL)
    sscanf(dimensions, "%d%d", width, length);
  else
    default_media_size(model, ppd_file, media_size, width, length);
}


/*
 * 'ps_imageable_area()' - Return the imageable area of the page.
 */

void
ps_imageable_area(int  model,		/* I - Printer model */
                  char *ppd_file,	/* I - PPD file (not used) */
                  char *media_size,	/* I - Media size */
                  int  *left,		/* O - Left position in points */
                  int  *right,		/* O - Right position in points */
                  int  *bottom,		/* O - Bottom position in points */
                  int  *top)		/* O - Top position in points */
{
  char	*area;				/* Imageable area of media */
  float	fleft,				/* Floating point versions */
	fright,
	fbottom,
	ftop;


  if ((area = ppd_find(ppd_file, "ImageableArea", media_size, NULL)) != NULL)
  {
#ifdef DEBUG
    printf("area = \'%s\'\n", area);
#endif /* DEBUG */
    if (sscanf(area, "%f%f%f%f", &fleft, &fbottom, &fright, &ftop) == 4)
    {
      *left   = (int)fleft;
      *right  = (int)fright;
      *bottom = (int)fbottom;
      *top    = (int)ftop;
    }
    else
      *left = *right = *bottom = *top = 0;
  }
  else
    *left = *right = *bottom = *top = 0;
}


/*
 * 'ps_print()' - Print an image to a PostScript printer.
 */

void
ps_print(int       model,		/* I - Model (Level 1 or 2) */
         char      *ppd_file,		/* I - PPD file */
         char      *resolution,		/* I - Resolution */
         char      *media_size,		/* I - Media size */
         char      *media_type,		/* I - Media type */
         char      *media_source,	/* I - Media source */
         int       output_type,		/* I - Output type (color/grayscale) */
         int       orientation,		/* I - Orientation of image */
         float     scaling,		/* I - Scaling of image */
         int       left,		/* I - Left offset of image (points) */
         int       top,			/* I - Top offset of image (points) */
         int       copies,		/* I - Number of copies */
         FILE      *prn,		/* I - File to print to */
         GDrawable *drawable,		/* I - Image to print */
         guchar    *lut,		/* I - Brightness lookup table */
         guchar    *cmap)		/* I - Colormap (for indexed images) */
{
  int		i, j;		/* Looping vars */
  int		x, y;		/* Looping vars */
  GPixelRgn	rgn;		/* Image region */
  guchar	*in,		/* Input pixels from image */
		*out;		/* Output pixels for printer */
  int		page_left,	/* Left margin of page */
		page_right,	/* Right margin of page */
		page_top,	/* Top of page */
		page_bottom,	/* Bottom of page */
		page_width,	/* Width of page */
		page_height,	/* Height of page */
		out_width,	/* Width of image on page */
		out_height,	/* Height of image on page */
		out_bpp,	/* Output bytes per pixel */
		out_length,	/* Output length (Level 2 output) */
		out_offset,	/* Output offset (Level 2 output) */
		temp_width,	/* Temporary width of image on page */
		temp_height,	/* Temporary height of image on page */
		landscape;	/* True if we rotate the output 90 degrees */
  time_t	curtime;	/* Current time of day */
  convert_t	colorfunc;	/* Color conversion function... */
  char		*command;	/* PostScript command */
  int		order,		/* Order of command */
		num_commands;	/* Number of commands */
  struct			/* PostScript commands... */
  {
    char	*command;
    int		order;
  }		commands[4];


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

  landscape = 0;
  ps_imageable_area(model, ppd_file, media_size, &page_left, &page_right,
                    &page_bottom, &page_top);

  page_width  = page_right - page_left;
  page_height = page_top - page_bottom;

#ifdef DEBUG
  printf("page_width = %d, page_height = %d\n", page_width, page_height);
  printf("drawable->width = %d, drawable->height = %d\n", drawable->width, drawable->height);
  printf("scaling = %.1f\n", scaling);
#endif /* DEBUG */

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

 /*
  * Let the user know what we're doing...
  */

  gimp_progress_init("Printing...");

 /*
  * Output a standard PostScript header with DSC comments...
  */

  curtime = time(NULL);

  if (top < 0 || left < 0)
  {
    left = (page_width - out_width) / 2 + page_left;
    top  = (page_height + out_height) / 2 + page_bottom;
  };

#ifdef DEBUG
  printf("out_width = %d, out_height = %d\n", out_width, out_height);
  printf("page_left = %d, page_right = %d, page_bottom = %d, page_top = %d\n",
         page_left, page_right, page_bottom, page_top);
  printf("left = %d, top = %d\n", left, top);
#endif /* DEBUG */

  fputs("%!PS-Adobe-3.0\n", prn);
  fputs("%%Creator: " PLUG_IN_NAME " plug-in V" PLUG_IN_VERSION " for GIMP.\n", prn);
  fprintf(prn, "%%%%CreationDate: %s", ctime(&curtime));
  fputs("%%Copyright: 1997-1998 by Michael Sweet (mike@easysw.com)\n", prn);
  fprintf(prn, "%%%%BoundingBox: %d %d %d %d\n",
          left, top - out_height, left + out_width, top);
  fputs("%%DocumentData: Clean7Bit\n", prn);
  fprintf(prn, "%%%%LanguageLevel: %d\n", model + 1);
  fputs("%%Pages: 1\n", prn);
  fputs("%%Orientation: Portrait\n", prn);
  fputs("%%EndComments\n", prn);

 /*
  * Find any printer-specific commands...
  */

  num_commands = 0;

  if ((command = ppd_find(ppd_file, "PageSize", media_size, &order)) != NULL)
  {
    commands[num_commands].command = strdup(command);
    commands[num_commands].order   = order;
    num_commands ++;
  };

  if ((command = ppd_find(ppd_file, "InputSlot", media_source, &order)) != NULL)
  {
    commands[num_commands].command = strdup(command);
    commands[num_commands].order   = order;
    num_commands ++;
  };

  if ((command = ppd_find(ppd_file, "MediaType", media_type, &order)) != NULL)
  {
    commands[num_commands].command = strdup(command);
    commands[num_commands].order   = order;
    num_commands ++;
  };

  if ((command = ppd_find(ppd_file, "Resolution", resolution, &order)) != NULL)
  {
    commands[num_commands].command = strdup(command);
    commands[num_commands].order   = order;
    num_commands ++;
  };

 /*
  * Sort the commands using the OrderDependency value...
  */

  for (i = 0; i < (num_commands - 1); i ++)
    for (j = i + 1; j < num_commands; j ++)
      if (commands[j].order < commands[i].order)
      {
        order               = commands[i].order;
        command             = commands[i].command;
        commands[i].command = commands[j].command;
        commands[i].order   = commands[j].order;
        commands[j].command = command;
        commands[j].order   = order;
      };

 /*
  * Send the commands...
  */

  if (num_commands > 0)
  {
    fputs("%%BeginProlog\n", prn);

    for (i = 0; i < num_commands; i ++)
    {
      fputs(commands[i].command, prn);
      free(commands[i].command);
    };

    fputs("%%EndProlog\n", prn);
  };

 /*
  * Output the page, rotating as necessary...
  */

  fputs("%%Page: 1\n", prn);
  fputs("gsave\n", prn);

  if (landscape)
  {
    fprintf(prn, "%d %d translate\n", left, top - out_height);
    fprintf(prn, "%.3f %.3f scale\n",
            (float)out_width / ((float)drawable->height),
            (float)out_height / ((float)drawable->width));
  }
  else
  {
    fprintf(prn, "%d %d translate\n", left, top);
    fprintf(prn, "%.3f %.3f scale\n",
            (float)out_width / ((float)drawable->width),
            (float)out_height / ((float)drawable->height));
  };

  in  = g_malloc(drawable->width * drawable->bpp);
  out = g_malloc(drawable->width * out_bpp + 3);

  if (model == 0)
  {
    fprintf(prn, "/picture %d string def\n", drawable->width * out_bpp);

    fprintf(prn, "%d %d 8\n", drawable->width, drawable->height);

    if (landscape)
      fputs("[ 0 1 1 0 0 0 ]\n", prn);
    else
      fputs("[ 1 0 0 -1 0 1 ]\n", prn);

    if (output_type == OUTPUT_GRAY)
      fputs("{currentfile picture readhexstring pop} image\n", prn);
    else
      fputs("{currentfile picture readhexstring pop} false 3 colorimage\n", prn);

    for (y = 0; y < drawable->height; y ++)
    {
      if ((y & 15) == 0)
        gimp_progress_update((double)y / (double)drawable->height);

      gimp_pixel_rgn_get_row(&rgn, in, 0, y, drawable->width);
      (*colorfunc)(in, out, drawable->width, drawable->bpp, lut, cmap);

      ps_hex(prn, out, drawable->width * out_bpp);
    };
  }
  else
  {
    if (output_type == OUTPUT_GRAY)
      fputs("/DeviceGray setcolorspace\n", prn);
    else
      fputs("/DeviceRGB setcolorspace\n", prn);

    fputs("<<\n", prn);
    fputs("\t/ImageType 1\n", prn);

    fprintf(prn, "\t/Width %d\n", drawable->width);
    fprintf(prn, "\t/Height %d\n", drawable->height);
    fputs("\t/BitsPerComponent 8\n", prn);

    if (output_type == OUTPUT_GRAY)
      fputs("\t/Decode [ 0 1 ]\n", prn);
    else
      fputs("\t/Decode [ 0 1 0 1 0 1 ]\n", prn);

    fputs("\t/DataSource currentfile /ASCII85Decode filter\n", prn);

    if ((drawable->width * 72 / out_width) < 100)
      fputs("\t/Interpolate true\n", prn);

    if (landscape)
      fputs("\t/ImageMatrix [ 0 1 1 0 0 0 ]\n", prn);
    else
      fputs("\t/ImageMatrix [ 1 0 0 -1 0 1 ]\n", prn);

    fputs(">>\n", prn);
    fputs("image\n", prn);

    for (y = 0, out_offset = 0; y < drawable->height; y ++)
    {
      if ((y & 15) == 0)
        gimp_progress_update((double)y / (double)drawable->height);

      gimp_pixel_rgn_get_row(&rgn, in, 0, y, drawable->width);
      (*colorfunc)(in, out + out_offset, drawable->width, drawable->bpp, lut, cmap);

      out_length = out_offset + drawable->width * out_bpp;

      if (y < (drawable->height - 1))
      {
        ps_ascii85(prn, out, out_length & ~3, 0);
        out_offset = out_length & 3;
      }
      else
      {
        ps_ascii85(prn, out, out_length, 1);
        out_offset = 0;
      };

      if (out_offset > 0)
        memcpy(out, out + out_length - out_offset, out_offset);
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
  int		col;	/* Current column */
  static char	*hex = "0123456789ABCDEF";


  col = 0;
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

    col = (col + 1) & 31;
    if (col == 0)
      putc('\n', prn);
  };

  if (col > 0)
    putc('\n', prn);
}


/*
 * 'ps_ascii85()' - Print binary data as a series of base-85 numbers.
 */

static void
ps_ascii85(FILE   *prn,		/* I - File to print to */
	   guchar *data,	/* I - Data to print */
	   int    length,	/* I - Number of bytes to print */
	   int    last_line)	/* I - Last line of raster data? */
{
  int		i;		/* Looping var */
  unsigned	b;		/* Binary data word */
  unsigned char	c[5];		/* ASCII85 encoded chars */


  while (length > 3)
  {
    b = (((((data[0] << 8) | data[1]) << 8) | data[2]) << 8) | data[3];

    if (b == 0)
      putc('z', prn);
    else
    {
      c[4] = (b % 85) + '!';
      b /= 85;
      c[3] = (b % 85) + '!';
      b /= 85;
      c[2] = (b % 85) + '!';
      b /= 85;
      c[1] = (b % 85) + '!';
      b /= 85;
      c[0] = b + '!';

      fwrite(c, 5, 1, prn);
    };

    data += 4;
    length -= 4;
  };

  if (last_line)
  {
    if (length > 0)
    {
      for (b = 0, i = length; i > 0; b = (b << 8) | data[0], data ++, i --);

      c[4] = (b % 85) + '!';
      b /= 85;
      c[3] = (b % 85) + '!';
      b /= 85;
      c[2] = (b % 85) + '!';
      b /= 85;
      c[1] = (b % 85) + '!';
      b /= 85;
      c[0] = b + '!';

      fwrite(c, length + 1, 1, prn);
    };

    fputs("~>\n", prn);
  };
}


/*
 * 'ppd_find()' - Find a control string with the specified name & parameters.
 */

static char *			/* O - Control string */
ppd_find(char *ppd_file,	/* I - Name of PPD file */
         char *name,		/* I - Name of parameter */
         char *option,		/* I - Value of parameter */
         int  *order)		/* O - Order of the control string */
{
  char		line[1024],	/* Line from file */
		lname[255],	/* Name from line */
		loption[255],	/* Value from line */
		*opt;		/* Current control string pointer */
  static char	value[32768];	/* Current control string value */


  if (ppd_file == NULL || name == NULL || option == NULL)
    return (NULL);

  if (ps_ppd_file == NULL || strcmp(ps_ppd_file, ppd_file) != 0)
  {
    if (ps_ppd != NULL)
      fclose(ps_ppd);

    ps_ppd = fopen(ppd_file, "r");

    if (ps_ppd == NULL)
      ps_ppd_file = NULL;
    else
      ps_ppd_file = ppd_file;
  };

  if (ps_ppd == NULL)
    return (NULL);

  if (order != NULL)
    *order = 1000;

  rewind(ps_ppd);
  while (fgets(line, sizeof(line), ps_ppd) != NULL)
  {
    if (line[0] != '*')
      continue;

    if (strncasecmp(line, "*OrderDependency:", 17) == 0 && order != NULL)
    {
      sscanf(line, "%*s%d", order);
      continue;
    }
    else if (sscanf(line, "*%s %[^/:]", lname, loption) != 2)
      continue;

    if (strcasecmp(lname, name) == 0 &&
        strcasecmp(loption, option) == 0)
    {
      opt = strchr(line, ':') + 1;
      while (*opt == ' ' || *opt == '\t')
        opt ++;
      if (*opt != '\"')
        continue;

      strcpy(value, opt + 1);
      if ((opt = strchr(value, '\"')) == NULL)
      {
        while (fgets(line, sizeof(line), ps_ppd) != NULL)
        {
          strcat(value, line);
          if (strchr(line, '\"') != NULL)
          {
            strcpy(strchr(value, '\"'), "\n");
            break;
          };
        };
      }
      else
        *opt = '\0';

      return (value);
    };
  };

  return (NULL);
}


/*
 * End of "$Id$".
 */
