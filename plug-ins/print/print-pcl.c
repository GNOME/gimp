/*
 * "$Id$"
 *
 *   Print plug-in HP PCL driver for the GIMP.
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
 *   pcl_parameters()     - Return the parameter values for the given
 *                          parameter.
 *   pcl_imageable_area() - Return the imageable area of the page.
 *   pcl_print()          - Print an image to an HP printer.
 *   pcl_mode0()          - Send PCL graphics using mode 0 (no) compression.
 *   pcl_mode2()          - Send PCL graphics using mode 2 (TIFF) compression.
 *
 * Revision History:
 *
 *   $Log$
 *   Revision 1.12  1999/12/16 19:44:01  olofk
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
 *   Revision 1.13  1999/11/23 02:11:37  rlk
 *   Rationalize variables, pass 3
 *
 *   Revision 1.12  1999/11/23 01:45:00  rlk
 *   Rationalize variables -- pass 2
 *
 *   Revision 1.11  1999/11/10 01:13:44  rlk
 *   multi-pass
 *
 *   Revision 1.10  1999/10/26 23:36:51  rlk
 *   Comment out all remaining 16-bit code, and rename 16-bit functions to "standard" names
 *
 *   Revision 1.9  1999/10/26 02:10:30  rlk
 *   Mostly fix save/load
 *
 *   Move all gimp, glib, gtk stuff into print.c (take it out of everything else).
 *   This should help port it to more general purposes later.
 *
 *   Revision 1.8  1999/10/25 23:31:59  rlk
 *   16-bit clean
 *
 *   Revision 1.7  1999/10/21 01:27:37  rlk
 *   More progress toward full 16-bit rendering
 *
 *   Revision 1.6  1999/10/19 02:04:59  rlk
 *   Merge all of the single-level print_cmyk functions
 *
 *   Revision 1.5  1999/10/17 23:44:07  rlk
 *   16-bit everything (untested)
 *
 *   Revision 1.4  1999/10/17 23:01:01  rlk
 *   Move various dither functions into print-utils.c
 *
 *   Revision 1.3  1999/10/14 01:59:59  rlk
 *   Saturation
 *
 *   Revision 1.2  1999/09/12 00:12:24  rlk
 *   Current best stuff
 *
 *   Revision 1.11  1999/05/29 16:35:27  yosh
 *   * configure.in
 *   * Makefile.am: removed tips files, AC_SUBST GIMP_PLUGINS and
 *   GIMP_MODULES so you can easily skip those parts of the build
 *
 *   * acinclude.m4
 *   * config.sub
 *   * config.guess
 *   * ltconfig
 *   * ltmain.sh: libtool 1.3.2
 *
 *   * app/fileops.c: shuffle #includes to avoid warning about MIN and
 *   MAX
 *
 *   [ The following is a big i18n patch from David Monniaux
 *     <david.monniaux@ens.fr> ]
 *
 *   * tips/gimp_conseils.fr.txt
 *   * tips/gimp_tips.txt
 *   * tips/Makefile.am
 *   * configure.in: moved tips to separate dir
 *
 *   * po-plugins: new dir for plug-in translation files
 *
 *   * configure.in: add po-plugins dir and POTFILES processing
 *
 *   * app/boundary.c
 *   * app/brightness_contrast.c
 *   * app/by_color_select.c
 *   * app/color_balance.c
 *   * app/convert.c
 *   * app/curves.c
 *   * app/free_select.c
 *   * app/gdisplay.c
 *   * app/gimpimage.c
 *   * app/gimpunit.c
 *   * app/gradient.c
 *   * app/gradient_select.c
 *   * app/install.c
 *   * app/session.c: various i18n tweaks
 *
 *   * app/tips_dialog.c: localize tips filename
 *
 *   * libgimp/gimpunit.c
 *   * libgimp/gimpunitmenu.c: #include "config.h"
 *
 *   * plug-ins/CEL
 *   * plug-ins/CML_explorer
 *   * plug-ins/Lighting
 *   * plug-ins/apply_lens
 *   * plug-ins/autostretch_hsv
 *   * plug-ins/blur
 *   * plug-ins/bmp
 *   * plug-ins/borderaverage
 *   * plug-ins/bumpmap
 *   * plug-ins/bz2
 *   * plug-ins/checkerboard
 *   * plug-ins/colorify
 *   * plug-ins/compose
 *   * plug-ins/convmatrix
 *   * plug-ins/cubism
 *   * plug-ins/depthmerge
 *   * plug-ins/destripe
 *   * plug-ins/gif
 *   * plug-ins/gifload
 *   * plug-ins/jpeg
 *   * plug-ins/mail
 *   * plug-ins/oilify
 *   * plug-ins/png
 *   * plug-ins/print
 *   * plug-ins/ps
 *   * plug-ins/xbm
 *   * plug-ins/xpm
 *   * plug-ins/xwd: plug-in i18n stuff
 *
 *   -Yosh
 *
 *   Revision 1.10  1998/08/28 23:01:45  yosh
 *   * acconfig.h
 *   * configure.in
 *   * app/main.c: added check for putenv and #ifdefed it's usage since NeXTStep is
 *   lame
 *
 *   * libgimp/gimp.c
 *   * app/main.c
 *   * app/plug_in.c: conditionally compile shared mem stuff so platforms without
 *   it can still work
 *
 *   * plug-ins/CEL/CEL.c
 *   * plug-ins/palette/palette.c
 *   * plug-ins/print/print-escp2.c
 *   * plug-ins/print/print-pcl.c
 *   * plug-ins/print/print-ps.c: s/strdup/g_strdup/ for portability
 *
 *   -Yosh
 *
 *   Revision 1.9  1998/05/17 07:16:46  yosh
 *   0.99.31 fun
 *
 *   updated print plugin
 *
 *   -Yosh
 *
 *   Revision 1.12  1998/05/16  18:27:59  mike
 *   Added support for 4-level "CRet" mode of 800/1100 series printers.
 *
 *   Revision 1.11  1998/05/15  21:01:51  mike
 *   Updated image positioning code (invert top and center left/top independently)
 *
 *   Revision 1.10  1998/05/08  21:22:00  mike
 *   Added quality mode command for DeskJet printers (high quality for 300
 *   DPI or higher).
 *
 *   Revision 1.9  1998/05/08  19:20:50  mike
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

extern int	error[2][4][14*720+4];


/*
 * 'pcl_parameters()' - Return the parameter values for the given parameter.
 */

char **				/* O - Parameter values */
pcl_parameters(int  model,	/* I - Printer model */
               char *ppd_file,	/* I - PPD file (not used) */
               char *name,	/* I - Name of parameter */
               int  *count)	/* O - Number of values */
{
  int		i;
  char		**p,
		**valptrs;
  static char	*media_sizes[] =
		{
		  ("Letter"),
		  ("Legal"),
		  ("A4"),
		  ("Tabloid"),
		  ("A3"),
		  ("12x18")
		};
  static char	*media_types[] =
		{
		  ("Plain"),
		  ("Premium"),
		  ("Glossy"),
		  ("Transparency")
		};
  static char	*media_sources[] =
		{
		  ("Manual"),
		  ("Tray 1"),
		  ("Tray 2"),
		  ("Tray 3"),
		  ("Tray 4"),
		};
  static char	*resolutions[] =
		{
		  ("150 DPI"),
		  ("300 DPI"),
		  ("600 DPI")
		};


  if (count == NULL)
    return (NULL);

  *count = 0;

  if (name == NULL)
    return (NULL);

  if (strcmp(name, "PageSize") == 0)
  {
    if (model == 5 || model == 1100)
      *count = 6;
    else
      *count = 3;

    p = media_sizes;
  }
  else if (strcmp(name, "MediaType") == 0)
  {
    if (model < 500)
    {
      *count = 0;
      return (NULL);
    }
    else
    {
      *count = 4;
      p = media_types;
    }
  }
  else if (strcmp(name, "InputSlot") == 0)
  {
    if (model < 500)
    {
      *count = 5;
      p = media_sources;
    }
    else
    {
      *count = 0;
      return (NULL);
    }
  }
  else if (strcmp(name, "Resolution") == 0)
  {
    if (model == 4 || model == 5 || model == 800 || model == 600)
      *count = 3;
    else
      *count = 2;

    p = resolutions;
  }
  else
    return (NULL);

  valptrs = malloc(*count * sizeof(char *));
  for (i = 0; i < *count; i ++)
    {
      /* strdup doesn't appear to be POSIX... */
      valptrs[i] = malloc(strlen(p[i]) + 1);
      strcpy(valptrs[i], p[i]);
    }

  return (valptrs);
}


/*
 * 'pcl_imageable_area()' - Return the imageable area of the page.
 */

void
pcl_imageable_area(int  model,		/* I - Printer model */
                   char *ppd_file,	/* I - PPD file (not used) */
                   char *media_size,	/* I - Media size */
                   int  *left,		/* O - Left position in points */
                   int  *right,		/* O - Right position in points */
                   int  *bottom,	/* O - Bottom position in points */
                   int  *top)		/* O - Top position in points */
{
  int	width, length;			/* Size of page */


  default_media_size(model, ppd_file, media_size, &width, &length);

  switch (model)
  {
    default :
        *left   = 18;
        *right  = width - 18;
        *top    = length - 12;
        *bottom = 12;
        break;

    case 500 :
        *left   = 18;
        *right  = width - 18;
        *top    = length - 7;
        *bottom = 41;
        break;

    case 501 :
        *left   = 18;
        *right  = width - 18;
        *top    = length - 7;
        *bottom = 33;
        break;

    case 550 :
    case 800 :
    case 1100 :
        *left   = 18;
        *right  = width - 18;
        *top    = length - 3;
        *bottom = 33;
        break;

    case 600 :
        *left   = 18;
        *right  = width - 18;
        *top    = length - 0;
        *bottom = 33;
        break;
  }
}


/*
 * 'pcl_print()' - Print an image to an HP printer.
 */

void
pcl_print(int       model,		/* I - Model */
          int       copies,		/* I - Number of copies */
          FILE      *prn,		/* I - File to print to */
          Image     image,		/* I - Image to print */
	  unsigned char    *cmap,	/* I - Colormap (for indexed images) */
	  lut_t     *lut,		/* I - Brightness lookup table */
	  vars_t    *v)
{
  char 		*ppd_file = v->ppd_file;
  char 		*resolution = v->resolution;
  char 		*media_size = v->media_size;
  char 		*media_type = v->media_type;
  int 		output_type = v->output_type;
  int		orientation = v->orientation;
  float 	scaling = v->scaling;
  int		top = v->top;
  int		left = v->left;
  int		x, y;		/* Looping vars */
  int		xdpi, ydpi;	/* Resolution */
  unsigned short *out;
  unsigned char	*in,		/* Input pixels */
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
  int           image_height,
                image_width,
                image_bpp;


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

  if ((image_bpp < 3 && cmap == NULL) || model <= 500)
    output_type = OUTPUT_GRAY;		/* Force grayscale output */

  if (output_type == OUTPUT_COLOR)
  {
    out_bpp = 3;

    if (image_bpp >= 3)
      colorfunc = rgb_to_rgb;
    else
      colorfunc = indexed_to_rgb;
  }
  else
  {
    out_bpp = 1;

    if (image_bpp >= 3)
      colorfunc = rgb_to_gray;
    else if (cmap == NULL)
      colorfunc = gray_to_gray;
    else
      colorfunc = indexed_to_gray;
  }

 /*
  * Figure out the output resolution...
  */

  xdpi = atoi(resolution);

  if ((model == 800 || model == 1100) &&
      output_type == OUTPUT_COLOR && xdpi == 600)
    xdpi = 300;

  if (model == 600 && xdpi == 600)
    ydpi = 300;
  else
    ydpi = xdpi;

 /*
  * Compute the output size...
  */

  landscape = 0;
  pcl_imageable_area(model, ppd_file, media_size, &page_left, &page_right,
                     &page_bottom, &page_top);

  page_width  = page_right - page_left;
  page_height = page_top - page_bottom;

 /*
  * Portrait width/height...
  */

  if (scaling < 0.0)
  {
   /*
    * Scale to pixels per inch...
    */

    out_width  = image_width * -72.0 / scaling;
    out_height = image_height * -72.0 / scaling;
  }
  else
  {
   /*
    * Scale by percent...
    */

    out_width  = page_width * scaling / 100.0;
    out_height = out_width * image_height / image_width;
    if (out_height > page_height)
    {
      out_height = page_height * scaling / 100.0;
      out_width  = out_height * image_width / image_height;
    }
  }

 /*
  * Landscape width/height...
  */

  if (scaling < 0.0)
  {
   /*
    * Scale to pixels per inch...
    */

    temp_width  = image_height * -72.0 / scaling;
    temp_height = image_width * -72.0 / scaling;
  }
  else
  {
   /*
    * Scale by percent...
    */

    temp_width  = page_width * scaling / 100.0;
    temp_height = temp_width * image_width / image_height;
    if (temp_height > page_height)
    {
      temp_height = page_height;
      temp_width  = temp_height * image_height / image_width;
    }
  }

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
    }
  }

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
  }

  if (left < 0)
    left = (page_width - out_width) / 2 + page_left;

  if (top < 0)
    top  = (page_height + out_height) / 2 + page_bottom;
  else
    top = page_height - top + page_bottom;

#ifdef DEBUG
  printf("page_width = %d, page_height = %d\n", page_width, page_height);
  printf("out_width = %d, out_height = %d\n", out_width, out_height);
  printf("xdpi = %d, ydpi = %d, landscape = %d\n", xdpi, ydpi, landscape);
#endif /* DEBUG */

 /*
  * Let the user know what we're doing...
  */

  Image_progress_init(image);

 /*
  * Send PCL initialization commands...
  */

  fputs("\033E", prn); 				/* PCL reset */

  if (strcmp(media_size, "Letter") == 0)	/* Set media size */
  {
    fputs("\033&l2A", prn);
    top = 792 - top;
  }
  else if (strcmp(media_size, "Legal") == 0)
  {
    fputs("\033&l3A", prn);
    top = 1008 - top;
  }
  else if (strcmp(media_size, "Tabloid") == 0)
  {
    fputs("\033&l6A", prn);
    top = 1214 - top;
  }
  else if (strcmp(media_size, "A4") == 0)
  {
    fputs("\033&l26A", prn);
    top = 842 - top;
  }
  else if (strcmp(media_size, "A3") == 0)
  {
    fputs("\033&l27A", prn);
    top = 1191 - top;
  }

  fputs("\033&l0L", prn);			/* Turn off perforation skip */
  fputs("\033&l0E", prn);			/* Reset top margin to 0 */

  if (strcmp(media_type, "Plain") == 0)		/* Set media type */
    fputs("\033&l0M", prn);
  else if (strcmp(media_type, "Premium") == 0)
    fputs("\033&l2M", prn);
  else if (strcmp(media_type, "Glossy") == 0)
    fputs("\033&l3M", prn);
  else if (strcmp(media_type, "Transparency") == 0)
    fputs("\033&l4M", prn);

  if (strcmp(media_type, "Manual") == 0)	/* Set media source */
    fputs("\033&l2H", prn);
  else if (strcmp(media_type, "Tray 1") == 0)
    fputs("\033&l8H", prn);
  else if (strcmp(media_type, "Tray 2") == 0)
    fputs("\033&l1H", prn);
  else if (strcmp(media_type, "Tray 3") == 0)
    fputs("\033&l4H", prn);
  else if (strcmp(media_type, "Tray 4") == 0)
    fputs("\033&l5H", prn);

  if (model >= 500 && model < 1200 && xdpi >= 300)
    fputs("\033*r2Q", prn);
  else if (model == 1200 && xdpi >= 300)
    fputs("\033*o1Q", prn);

  if (xdpi != ydpi)				/* Set resolution */
  {
   /*
    * Send 26-byte configure image data command with horizontal and
    * vertical resolutions as well as a color count...
    */

    fputs("\033*g26W", prn);
    putc(2, prn);				/* Format 2 */
    if (output_type == OUTPUT_COLOR)
      putc(4, prn);				/* # output planes */
    else
      putc(1, prn);				/* # output planes */

    putc(xdpi >> 8, prn);			/* Black resolution */
    putc(xdpi, prn);
    putc(ydpi >> 8, prn);
    putc(ydpi, prn);
    putc(0, prn);
    putc(2, prn);				/* # of black levels */

    putc(xdpi >> 8, prn);			/* Cyan resolution */
    putc(xdpi, prn);
    putc(ydpi >> 8, prn);
    putc(ydpi, prn);
    putc(0, prn);
    putc(2, prn);				/* # of cyan levels */

    putc(xdpi >> 8, prn);			/* Magenta resolution */
    putc(xdpi, prn);
    putc(ydpi >> 8, prn);
    putc(ydpi, prn);
    putc(0, prn);
    putc(2, prn);				/* # of magenta levels */

    putc(xdpi >> 8, prn);			/* Yellow resolution */
    putc(xdpi, prn);
    putc(ydpi >> 8, prn);
    putc(ydpi, prn);
    putc(0, prn);
    putc(2, prn);				/* # of yellow levels */
  }
  else if (xdpi == 300 && model == 800)		/* 300 DPI CRet */
  {
   /*
    * Send 26-byte configure image data command with horizontal and
    * vertical resolutions as well as a color count...
    */

    fputs("\033*g26W", prn);
    putc(2, prn);				/* Format 2 */
    if (output_type == OUTPUT_COLOR)
      putc(4, prn);				/* # output planes */
    else
      putc(1, prn);				/* # output planes */

    putc(xdpi >> 8, prn);			/* Black resolution */
    putc(xdpi, prn);
    putc(ydpi >> 8, prn);
    putc(ydpi, prn);
    putc(0, prn);
    putc(4, prn);				/* # of black levels */

    putc(xdpi >> 8, prn);			/* Cyan resolution */
    putc(xdpi, prn);
    putc(ydpi >> 8, prn);
    putc(ydpi, prn);
    putc(0, prn);
    putc(4, prn);				/* # of cyan levels */

    putc(xdpi >> 8, prn);			/* Magenta resolution */
    putc(xdpi, prn);
    putc(ydpi >> 8, prn);
    putc(ydpi, prn);
    putc(0, prn);
    putc(4, prn);				/* # of magenta levels */

    putc(xdpi >> 8, prn);			/* Yellow resolution */
    putc(xdpi, prn);
    putc(ydpi >> 8, prn);
    putc(ydpi, prn);
    putc(0, prn);
    putc(4, prn);				/* # of yellow levels */
  }
  else
  {
    fprintf(prn, "\033*t%dR", xdpi);		/* Simple resolution */
    if (output_type == OUTPUT_COLOR)
    {
      if (model == 501 || model == 1200)
        fputs("\033*r-3U", prn);		/* Simple CMY color */
      else
        fputs("\033*r-4U", prn);		/* Simple KCMY color */
    }
  }

  if (model < 3 || model == 500)
    fputs("\033*b0M", prn);			/* Mode 0 (no compression) */
  else
    fputs("\033*b2M", prn);			/* Mode 2 (TIFF) */

 /*
  * Convert image size to printer resolution and setup the page for printing...
  */

  out_width  = xdpi * out_width / 72;
  out_height = ydpi * out_height / 72;

  fprintf(prn, "\033&a%dH", 10 * left - 180);	/* Set left raster position */
  fprintf(prn, "\033&a%dV", 10 * top);		/* Set top raster position */
  fprintf(prn, "\033*r%dS", out_width);		/* Set raster width */
  fprintf(prn, "\033*r%dT", out_height);	/* Set raster height */

  fputs("\033*r1A", prn); 			/* Start GFX */

 /*
  * Allocate memory for the raster data...
  */

  length = (out_width + 7) / 8;
  if (xdpi == 300 && model == 800)
    length *= 2;

  if (output_type == OUTPUT_GRAY)
  {
    black   = malloc(length);
    cyan    = NULL;
    magenta = NULL;
    yellow  = NULL;
  }
  else
  {
    cyan    = malloc(length);
    magenta = malloc(length);
    yellow  = malloc(length);
  
    if (model != 501 && model != 1200)
      black = malloc(length);
    else
      black = NULL;
  }
    
 /*
  * Output the page, rotating as necessary...
  */

  if (model < 3 || model == 500)
    writefunc = pcl_mode0;
  else
    writefunc = pcl_mode2;

  if (landscape)
  {
    in  = malloc(image_height * image_bpp);
    out = malloc(image_height * out_bpp * 2);

    errdiv  = image_width / out_height;
    errmod  = image_width % out_height;
    errval  = 0;
    errlast = -1;
    errline  = image_width - 1;
    
    for (x = 0; x < out_height; x ++)
    {
#ifdef DEBUG
      printf("pcl_print: x = %d, line = %d, val = %d, mod = %d, height = %d\n",
             x, errline, errval, errmod, out_height);
#endif /* DEBUG */

      if ((x & 255) == 0)
	Image_note_progress(image, x, out_height);

      if (errline != errlast)
      {
        errlast = errline;
	Image_get_col(image, in, errline);
      }

      (*colorfunc)(in, out, image_height, image_bpp, lut, cmap, v);

      if (xdpi == 300 && model == 800)
      {
       /*
        * 4-level (CRet) dithers...
	*/

	if (output_type == OUTPUT_GRAY)
	{
          dither_black4(out, x, image_height, out_width, black);
          (*writefunc)(prn, black, length / 2, 0);
          (*writefunc)(prn, black + length / 2, length / 2, 1);
	}
	else 
	{
          dither_cmyk4(out, x, image_height, out_width, cyan, magenta,
			  yellow, black);

          (*writefunc)(prn, black, length / 2, 0);
          (*writefunc)(prn, black + length / 2, length / 2, 0);
          (*writefunc)(prn, cyan, length / 2, 0);
          (*writefunc)(prn, cyan + length / 2, length / 2, 0);
          (*writefunc)(prn, magenta, length / 2, 0);
          (*writefunc)(prn, magenta + length / 2, length / 2, 0);
          (*writefunc)(prn, yellow, length / 2, 0);
          (*writefunc)(prn, yellow + length / 2, length / 2, 1);
	}
      }
      else
      {
       /*
        * Standard 2-level dithers...
	*/

	if (output_type == OUTPUT_GRAY)
	{
          dither_black(out, x, image_height, out_width, black);
          (*writefunc)(prn, black, length, 1);
	}
	else
	{
          dither_cmyk(out, x, image_height, out_width, cyan, 0, magenta,
			0, yellow, 0, black, 1);

          if (black != NULL)
            (*writefunc)(prn, black, length, 0);
          (*writefunc)(prn, cyan, length, 0);
          (*writefunc)(prn, magenta, length, 0);
          (*writefunc)(prn, yellow, length, 1);
	}
      }

      errval += errmod;
      errline -= errdiv;
      if (errval >= out_height)
      {
        errval -= out_height;
        errline --;
      }
    }
  }
  else
  {
    in  = malloc(image_width * image_bpp);
    out = malloc(image_width * out_bpp * 2);

    errdiv  = image_height / out_height;
    errmod  = image_height % out_height;
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
	Image_note_progress(image, y, out_height);

      if (errline != errlast)
      {
        errlast = errline;
	Image_get_row(image, in, errline);
      }

      (*colorfunc)(in, out, image_width, image_bpp, lut, cmap, v);

      if (xdpi == 300 && model == 800)
      {
       /*
        * 4-level (CRet) dithers...
	*/

	if (output_type == OUTPUT_GRAY)
	{
          dither_black4(out, y, image_width, out_width, black);
          (*writefunc)(prn, black, length / 2, 0);
          (*writefunc)(prn, black + length / 2, length / 2, 1);
	}
	else 
	{
          dither_cmyk4(out, y, image_width, out_width, cyan, magenta,
			  yellow, black);

          (*writefunc)(prn, black, length / 2, 0);
          (*writefunc)(prn, black + length / 2, length / 2, 0);
          (*writefunc)(prn, cyan, length / 2, 0);
          (*writefunc)(prn, cyan + length / 2, length / 2, 0);
          (*writefunc)(prn, magenta, length / 2, 0);
          (*writefunc)(prn, magenta + length / 2, length / 2, 0);
          (*writefunc)(prn, yellow, length / 2, 0);
          (*writefunc)(prn, yellow + length / 2, length / 2, 1);
	}
      }
      else
      {
       /*
        * Standard 2-level dithers...
	*/

	if (output_type == OUTPUT_GRAY)
	{
          dither_black(out, y, image_width, out_width, black);
          (*writefunc)(prn, black, length, 1);
	}
	else
	{
          dither_cmyk(out, y, image_width, out_width, cyan, 0, magenta,
			0, yellow, 0, black, 1);

          if (black != NULL)
            (*writefunc)(prn, black, length, 0);
          (*writefunc)(prn, cyan, length, 0);
          (*writefunc)(prn, magenta, length, 0);
          (*writefunc)(prn, yellow, length, 1);
	}
      }

      errval += errmod;
      errline += errdiv;
      if (errval >= out_height)
      {
        errval -= out_height;
        errline ++;
      }
    }
  }

 /*
  * Cleanup...
  */

  free(in);
  free(out);

  if (black != NULL)
    free(black);
  if (cyan != NULL)
  {
    free(cyan);
    free(magenta);
    free(yellow);
  }

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
  }

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
    }

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
    }

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
    }

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
    }
  }

 /*
  * Send a line of raster graphics...
  */

  fprintf(prn, "\033*b%d%c", (int)(comp_ptr - comp_buf), last_plane ? 'W' : 'V');
  fwrite(comp_buf, comp_ptr - comp_buf, 1, prn);
}


/*
 * End of "$Id$".
 */
