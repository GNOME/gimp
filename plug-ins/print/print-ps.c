/*
 * "$Id$"
 *
 *   Print plug-in Adobe PostScript driver for the GIMP.
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
 *   See ChangeLog
 */

#include "print.h"
#include <time.h>
#include <string.h>
#include <limits.h>

/*#define DEBUG*/


/*
 * Local variables...
 */

static FILE	*ps_ppd = NULL;
static char	*ps_ppd_file = NULL;


/*
 * Local functions...
 */

static void	ps_hex(FILE *, unsigned short *, int);
static void	ps_ascii85(FILE *, unsigned short *, int, int);
static char	*ppd_find(const char *, const char *, const char *, int *);


/*
 * 'ps_parameters()' - Return the parameter values for the given parameter.
 */

char **					/* O - Parameter values */
ps_parameters(const printer_t *printer,	/* I - Printer model */
              char *ppd_file,		/* I - PPD file (not used) */
              char *name,		/* I - Name of parameter */
              int  *count)		/* O - Number of values */
{
  int		i;
  char		line[1024],
		lname[255],
		loption[255];
  char		**valptrs;

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
  }

  if (ps_ppd == NULL)
    {
      if (strcmp(name, "PageSize") == 0)
	{
	  const papersize_t *papersizes = get_papersizes();
	  valptrs = malloc(sizeof(char *) * known_papersizes());
	  *count = 0;
	  for (i = 0; i < known_papersizes(); i++)
	    {
	      if (strlen(papersizes[i].name) > 0)
		{
		  valptrs[*count] = malloc(strlen(papersizes[i].name) + 1);
		  strcpy(valptrs[*count], papersizes[i].name);
		  (*count)++;
		}
	    }
	  return (valptrs);
	}
      else
	return (NULL);
    }

  rewind(ps_ppd);
  *count = 0;

  valptrs = malloc(100 * sizeof(char *));

  while (fgets(line, sizeof(line), ps_ppd) != NULL)
  {
    if (line[0] != '*')
      continue;

    if (sscanf(line, "*%s %[^/:]", lname, loption) != 2)
      continue;

    if (strcasecmp(lname, name) == 0)
    {
      valptrs[(*count)] = malloc(strlen(loption) + 1);
      strcpy(valptrs[(*count)], loption);
      (*count) ++;
    }
  }

  if (*count == 0)
  {
    free(valptrs);
    return (NULL);
  }
  else
    return (valptrs);
}


/*
 * 'ps_media_size()' - Return the size of the page.
 */

void
ps_media_size(const printer_t *printer,	/* I - Printer model */
	      const vars_t *v,		/* I */
              int  *width,		/* O - Width in points */
              int  *length)		/* O - Length in points */
{
  char	*dimensions;			/* Dimensions of media size */


#ifdef DEBUG
  printf("ps_media_size(%d, \'%s\', \'%s\', %08x, %08x)\n", model, ppd_file,
         media_size, width, length);
#endif /* DEBUG */

  if ((dimensions = ppd_find(v->ppd_file, "PaperDimension", v->media_size,
			     NULL))
      != NULL)
    sscanf(dimensions, "%d%d", width, length);
  else
    default_media_size(printer, v, width, length);
}


/*
 * 'ps_imageable_area()' - Return the imageable area of the page.
 */

void
ps_imageable_area(const printer_t *printer,	/* I - Printer model */
		  const vars_t *v,      /* I */
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


  if ((area = ppd_find(v->ppd_file, "ImageableArea", v->media_size, NULL))
      != NULL)
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
  {
    default_media_size(printer, v, right, top);
    *left   = 18;
    *right  -= 18;
    *top    -= 36;
    *bottom = 36;
  }
}

void
ps_limit(const printer_t *printer,	/* I - Printer model */
	    const vars_t *v,  		/* I */
	    int  *width,		/* O - Left position in points */
	    int  *length)		/* O - Top position in points */
{
  *width =	INT_MAX;
    *length =	INT_MAX;
}

const char *
ps_default_resolution(const printer_t *printer)
{
  return "default";
}

/*
 * 'ps_print()' - Print an image to a PostScript printer.
 */

void
ps_print(const printer_t *printer,		/* I - Model (Level 1 or 2) */
         int       copies,		/* I - Number of copies */
         FILE      *prn,		/* I - File to print to */
         Image     image,		/* I - Image to print */
	 const vars_t    *v)
{
  unsigned char *cmap = v->cmap;
  int		model = printer->model;
  char 		*ppd_file = v->ppd_file;
  char 		*resolution = v->resolution;
  char 		*media_size = v->media_size;
  char 		*media_type = v->media_type;
  char 		*media_source = v->media_source;
  int 		output_type = v->output_type;
  int		orientation = v->orientation;
  float 	scaling = v->scaling;
  int		top = v->top;
  int		left = v->left;
  int		i, j;		/* Looping vars */
  int		y;		/* Looping vars */
  unsigned char	*in;		/* Input pixels from image */
  unsigned short	*out;		/* Output pixels for printer */
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
		out_offset;	/* Output offset (Level 2 output) */
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
  int           image_height,
                image_width,
                image_bpp;
  vars_t	nv;

  memcpy(&nv, v, sizeof(vars_t));
 /*
  * Setup a read-only pixel region for the entire image...
  */

  Image_init(image);
  image_height = Image_height(image);
  image_width = Image_width(image);
  image_bpp = Image_bpp(image);

 /*
  * Choose the correct color conversion function...
  */

  if (image_bpp < 3 && cmap == NULL && output_type == OUTPUT_COLOR)
    output_type = OUTPUT_GRAY_COLOR;		/* Force grayscale output */

  colorfunc = choose_colorfunc(output_type, image_bpp, cmap, &out_bpp, &nv);

 /*
  * Compute the output size...
  */

  ps_imageable_area(printer, &nv, &page_left, &page_right,
                    &page_bottom, &page_top);
  compute_page_parameters(page_right, page_left, page_top, page_bottom,
			  scaling, image_width, image_height, image,
			  &orientation, &page_width, &page_height,
			  &out_width, &out_height, &left, &top);

  /*
   * Recompute the image height and width.  If the image has been
   * rotated, these will change from previously.
   */
  image_height = Image_height(image);
  image_width = Image_width(image);

 /*
  * Let the user know what we're doing...
  */

  Image_progress_init(image);

 /*
  * Output a standard PostScript header with DSC comments...
  */

  curtime = time(NULL);

  if (left < 0)
    left = (page_width - out_width) / 2 + page_left;
  else
    left += page_left;

  if (top < 0)
    top  = (page_height + out_height) / 2 + page_bottom;
  else
    top = page_height - top + page_bottom;

#ifdef DEBUG
  printf("out_width = %d, out_height = %d\n", out_width, out_height);
  printf("page_left = %d, page_right = %d, page_bottom = %d, page_top = %d\n",
         page_left, page_right, page_bottom, page_top);
  printf("left = %d, top = %d\n", left, top);
#endif /* DEBUG */

#ifdef __EMX__
  _fsetmode(prn, "t");
#endif
  fputs("%!PS-Adobe-3.0\n", prn);
  fprintf(prn, "%%%%Creator: %s\n", Image_get_appname(image));
  fprintf(prn, "%%%%CreationDate: %s", ctime(&curtime));
  fputs("%%Copyright: 1997-2000 by Michael Sweet (mike@easysw.com) and Robert Krawitz (rlk@alum.mit.edu)\n", prn);
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
    commands[num_commands].command = malloc(strlen(command) + 1);
    strcpy(commands[num_commands].command, command);
    commands[num_commands].order   = order;
    num_commands ++;
  }

  if ((command = ppd_find(ppd_file, "InputSlot", media_source, &order)) != NULL)
  {
    commands[num_commands].command = malloc(strlen(command) + 1);
    strcpy(commands[num_commands].command, command);
    commands[num_commands].order   = order;
    num_commands ++;
  }

  if ((command = ppd_find(ppd_file, "MediaType", media_type, &order)) != NULL)
  {
    commands[num_commands].command = malloc(strlen(command) + 1);
    strcpy(commands[num_commands].command, command);
    commands[num_commands].order   = order;
    num_commands ++;
  }

  if ((command = ppd_find(ppd_file, "Resolution", resolution, &order)) != NULL)
  {
    commands[num_commands].command = malloc(strlen(command) + 1);
    strcpy(commands[num_commands].command, command);
    commands[num_commands].order   = order;
    num_commands ++;
  }

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
      }

 /*
  * Send the commands...
  */

  if (num_commands > 0)
  {
    fputs("%%BeginProlog\n", prn);

    for (i = 0; i < num_commands; i ++)
    {
      fputs(commands[i].command, prn);
      fputs("\n", prn);
      free(commands[i].command);
    }

    fputs("%%EndProlog\n", prn);
  }

 /*
  * Output the page...
  */

  fputs("%%Page: 1\n", prn);
  fputs("gsave\n", prn);

  fprintf(prn, "%d %d translate\n", left, top);
  fprintf(prn, "%.3f %.3f scale\n",
          (float)out_width / ((float)image_width),
          (float)out_height / ((float)image_height));

  in  = malloc(image_width * image_bpp);
  out = malloc((image_width * out_bpp + 3) * 2);

  compute_lut(256, &nv);

  if (model == 0)
  {
    fprintf(prn, "/picture %d string def\n", image_width * out_bpp);

    fprintf(prn, "%d %d 8\n", image_width, image_height);

    fputs("[ 1 0 0 -1 0 1 ]\n", prn);

    if (output_type == OUTPUT_GRAY)
      fputs("{currentfile picture readhexstring pop} image\n", prn);
    else
      fputs("{currentfile picture readhexstring pop} false 3 colorimage\n", prn);

    for (y = 0; y < image_height; y ++)
    {
      if ((y & 15) == 0)
	Image_note_progress(image, y, image_height);

      Image_get_row(image, in, y);
      (*colorfunc)(in, out, image_width, image_bpp, cmap, &nv);

      ps_hex(prn, out, image_width * out_bpp);
    }
  }
  else
  {
    if (output_type == OUTPUT_GRAY)
      fputs("/DeviceGray setcolorspace\n", prn);
    else
      fputs("/DeviceRGB setcolorspace\n", prn);

    fputs("<<\n", prn);
    fputs("\t/ImageType 1\n", prn);

    fprintf(prn, "\t/Width %d\n", image_width);
    fprintf(prn, "\t/Height %d\n", image_height);
    fputs("\t/BitsPerComponent 8\n", prn);

    if (output_type == OUTPUT_GRAY)
      fputs("\t/Decode [ 0 1 ]\n", prn);
    else
      fputs("\t/Decode [ 0 1 0 1 0 1 ]\n", prn);

    fputs("\t/DataSource currentfile /ASCII85Decode filter\n", prn);

    if ((image_width * 72 / out_width) < 100)
      fputs("\t/Interpolate true\n", prn);

    fputs("\t/ImageMatrix [ 1 0 0 -1 0 1 ]\n", prn);

    fputs(">>\n", prn);
    fputs("image\n", prn);

    for (y = 0, out_offset = 0; y < image_height; y ++)
    {
      if ((y & 15) == 0)
	Image_note_progress(image, y, image_height);

      Image_get_row(image, in, y);
      (*colorfunc)(in, out + out_offset, image_width, image_bpp, cmap, &nv);

      out_length = out_offset + image_width * out_bpp;

      if (y < (image_height - 1))
      {
        ps_ascii85(prn, out, out_length & ~3, 0);
        out_offset = out_length & 3;
      }
      else
      {
        ps_ascii85(prn, out, out_length, 1);
        out_offset = 0;
      }

      if (out_offset > 0)
        memcpy(out, out + out_length - out_offset, out_offset);
    }
  }
  Image_progress_conclude(image);

  free_lut(&nv);
  free(in);
  free(out);

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
       unsigned short *data,	/* I - Data to print */
       int    length)	/* I - Number of bytes to print */
{
  int		col;	/* Current column */
  static char	*hex = "0123456789ABCDEF";


  col = 0;
  while (length > 0)
  {
    unsigned char pixel = (*data & 0xff00) >> 8;
   /*
    * Put the hex chars out to the file; note that we don't use fprintf()
    * for speed reasons...
    */

    putc(hex[pixel >> 4], prn);
    putc(hex[pixel & 15], prn);

    data ++;
    length --;

    col = (col + 1) & 31;
    if (col == 0)
      putc('\n', prn);
  }

  if (col > 0)
    putc('\n', prn);
}


/*
 * 'ps_ascii85()' - Print binary data as a series of base-85 numbers.
 */

static void
ps_ascii85(FILE   *prn,		/* I - File to print to */
	   unsigned short *data,	/* I - Data to print */
	   int    length,	/* I - Number of bytes to print */
	   int    last_line)	/* I - Last line of raster data? */
{
  int		i;		/* Looping var */
  unsigned	b;		/* Binary data word */
  unsigned char	c[5];		/* ASCII85 encoded chars */


  while (length > 3)
  {
    unsigned char d0 = (data[0] & 0xff00) >> 8;
    unsigned char d1 = (data[1] & 0xff00) >> 8;
    unsigned char d2 = (data[2] & 0xff00) >> 8;
    unsigned char d3 = (data[3] & 0xff00) >> 8;
    b = (((((d0 << 8) | d1) << 8) | d2) << 8) | d3;

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
    }

    data += 4;
    length -= 4;
  }

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
    }

    fputs("~>\n", prn);
  }
}


/*
 * 'ppd_find()' - Find a control string with the specified name & parameters.
 */

static char *			/* O - Control string */
ppd_find(const char *ppd_file,	/* I - Name of PPD file */
         const char *name,		/* I - Name of parameter */
         const char *option,		/* I - Value of parameter */
         int  *order)		/* O - Order of the control string */
{
  char		line[1024],	/* Line from file */
		lname[255],	/* Name from line */
		loption[255],	/* Value from line */
		*opt;		/* Current control string pointer */
  static char	*value = NULL;	/* Current control string value */


  if (ppd_file == NULL || name == NULL || option == NULL)
    return (NULL);
  if (!value)
    value = malloc(32768);

  if (ps_ppd_file == NULL || strcmp(ps_ppd_file, ppd_file) != 0)
  {
    if (ps_ppd != NULL)
      fclose(ps_ppd);

    ps_ppd = fopen(ppd_file, "r");

    if (ps_ppd == NULL)
      ps_ppd_file = NULL;
    else
      ps_ppd_file = ppd_file;
  }

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
          }
        }
      }
      else
        *opt = '\0';

      return (value);
    }
  }

  return (NULL);
}
