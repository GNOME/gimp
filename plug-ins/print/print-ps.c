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
 *   $Log$
 *   Revision 1.17  2000/01/26 16:00:48  neo
 *   updated print plug-in
 *
 *
 *   --Sven
 *
 *   Revision 1.12  2000/01/08 23:30:56  rlk
 *   Y2K copyright
 *
 *   Revision 1.11  2000/01/03 13:25:13  rlk
 *   Fix from Salvador Pinto Abreu <spa@khromeleque.dmat.uevora.pt>
 *
 *   Revision 1.10  1999/11/23 02:11:37  rlk
 *   Rationalize variables, pass 3
 *
 *   Revision 1.9  1999/11/23 01:45:00  rlk
 *   Rationalize variables -- pass 2
 *
 *   Revision 1.8  1999/10/26 23:36:51  rlk
 *   Comment out all remaining 16-bit code, and rename 16-bit functions to "standard" names
 *
 *   Revision 1.7  1999/10/26 02:10:30  rlk
 *   Mostly fix save/load
 *
 *   Move all gimp, glib, gtk stuff into print.c (take it out of everything else).
 *   This should help port it to more general purposes later.
 *
 *   Revision 1.6  1999/10/25 23:31:59  rlk
 *   16-bit clean
 *
 *   Revision 1.5  1999/10/21 01:27:37  rlk
 *   More progress toward full 16-bit rendering
 *
 *   Revision 1.4  1999/10/17 23:44:07  rlk
 *   16-bit everything (untested)
 *
 *   Revision 1.3  1999/10/14 01:59:59  rlk
 *   Saturation
 *
 *   Revision 1.2  1999/09/12 00:12:24  rlk
 *   Current best stuff
 *
 *   Revision 1.14  1999/05/29 16:35:29  yosh
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
 *   Revision 1.13  1999/05/27 19:11:33  asbjoer
 *   use g_strncasecmp()
 *
 *   Revision 1.12  1999/05/01 17:54:09  asbjoer
 *   os2 printing
 *
 *   Revision 1.11  1999/04/15 21:49:01  yosh
 *   * applied gimp-lecorfec-99041[02]-0, changes follow
 *
 *   * plug-ins/FractalExplorer/Dialogs.h (make_color_map):
 *   replaced free with free to fix segfault.
 *
 *   * plug-ins/Lighting/lighting_preview.c (compute_preview):
 *   allocate xpostab and ypostab only when needed (it could also be
 *   allocated on stack with a compilation-fixed size like MapObject).
 *   It avoids to lose some Kb on each preview :)
 *   Also reindented (unfortunate C-c C-q) some other lines.
 *
 *   * plug-ins/Lighting/lighting_main.c (run):
 *   release allocated postabs.
 *
 *   * plug-ins/Lighting/lighting_ui.c:
 *   callbacks now have only one argument because gck widget use
 *   gtk_signal_connect_object. Caused segfault for scale widget.
 *
 *   * plug-ins/autocrop/autocrop.c (doit):
 *   return if image has only background (thus fixing a segfault).
 *
 *   * plug-ins/emboss/emboss.c (pluginCore, emboss_do_preview):
 *   replaced malloc/free with malloc/free (unneeded, but
 *   shouldn't everyone use glib calls ? :)
 *
 *   * plug-ins/flame/flame.c :
 *   replaced a segfaulting free, and several harmless malloc/free pairs.
 *
 *   * plug-ins/flame/megawidget.c (mw_preview_build):
 *   replaced harmless malloc/free pair.
 *   Note : mwp->bits is malloc'ed but seems to be never freed.
 *
 *   * plug-ins/fractaltrace/fractaltrace.c (pixels_free):
 *   replaced a bunch of segfaulting free.
 *   (pixels_get, dialog_show): replaced gtk_signal_connect_object
 *   with gtk_signal_connect to accomodate callbacks (caused STRANGE
 *   dialog behaviour, coz you destroyed buttons one by one).
 *
 *   * plug-ins/illusion/illusion.c (dialog):
 *   same gtk_signal_connect_object replacement for same reasons.
 *
 *   * plug-ins/libgck/gck/gckcolor.c :
 *   changed all gck_rgb_to_color* functions to use a static GdkColor
 *   instead of a malloc'ed area. Provided reentrant functions with
 *   the old behaviour (gck_rgb_to_color*_r). Made some private functions
 *   static, too.
 *   gck_rgb_to_gdkcolor now use the new functions while
 *   gck_rgb_to_gdkcolor_r is the reentrant version.
 *   Also affected by this change: gck_gc_set_foreground and
 *   gck_gc_set_background (no more free(color)).
 *
 *   * plug-ins/libgck/gck/gckcolor.h :
 *   added the gck_rgb_to_gdkcolor_r proto.
 *
 *   * plug-ins/lic/lic.c (ok_button_clicked, cancel_button_clicked) :
 *   segfault on gtk_widget_destroy, now calls gtk_main_quit.
 *   (dialog_destroy) : segfault on window closure when called by
 *   "destroy" event. Now called by "delete_event".
 *
 *   * plug-ins/megawidget/megawidget.c (mw_preview_build):
 *   replaced harmless malloc/free pair.
 *   Note : mwp->bits is malloc'ed but seems to be never freed.
 *
 *   * plug-ins/png/png.c (load_image):
 *   replaced 2 segfaulting free.
 *
 *   * plug-ins/print/print-ps.c (ps_print):
 *   replaced a segfaulting free (called many times :).
 *
 *   * plug-ins/sgi/sgi.c (load_image, save_image):
 *   replaced a bunch of segfaulting free, and did some harmless
 *   inits to avoid a few gcc warnings.
 *
 *   * plug-ins/wind/wind.c (render_wind):
 *   replaced a segfaulting free.
 *   (render_blast): replaced harmless malloc/free pair.
 *
 *   * plug-ins/bmp/bmpread.c (ReadImage):
 *   yet another free()/free() problem fixed.
 *
 *   * plug-ins/exchange/exchange.c (real_exchange):
 *   ditto.
 *
 *   * plug-ins/fp/fp.h: added Frames_Check_Button_In_A_Box proto.
 *   * plug-ins/fp/fp_gtk.c: closing subdialogs via window manager
 *   wasn't handled, thus leading to errors and crashes.
 *   Now delete_event signals the dialog control button
 *   to close a dialog with the good way.
 *
 *   * plug-ins/ifscompose/ifscompose.c (value_pair_create):
 *   tried to set events mask on scale widget (a NO_WINDOW widget).
 *
 *   * plug-ins/png/png.c (save_image):
 *   Replaced 2 free() with free() for malloc'ed memory.
 *   Mysteriously I corrected the loading bug but not the saving one :)
 *
 *   -Yosh
 *
 *   Revision 1.10  1998/08/28 23:01:46  yosh
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
 *   Revision 1.9  1998/05/17 07:16:47  yosh
 *   0.99.31 fun
 *
 *   updated print plugin
 *
 *   -Yosh
 *
 *   Revision 1.13  1998/05/15  21:01:51  mike
 *   Updated image positioning code (invert top and center left/top independently)
 *   Updated ps_imageable_area() to return a default imageable area when no PPD
 *   file is available.
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

static void	ps_hex(FILE *, unsigned short *, int);
static void	ps_ascii85(FILE *, unsigned short *, int, int);
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
		  ("Letter"),
		  ("Legal"),
		  ("A4"),
		  ("Tabloid"),
		  ("A3"),
		  ("12x18")
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
  }

  if (ps_ppd == NULL)
  {
    if (strcmp(name, "PageSize") == 0)
    {
      *count = 6;

      valptrs = malloc(*count * sizeof(char *));
      for (i = 0; i < *count; i ++)
	{
	  /* strdup doesn't appear to be POSIX... */
	  valptrs[i] = malloc(strlen(media_sizes[i]) + 1);
	  strcpy(valptrs[i], media_sizes[i]);
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
  {
    default_media_size(model, ppd_file, media_size, right, top);
    *left   = 18;
    *right  -= 18;
    *top    -= 36;
    *bottom = 36;
  }
}


/*
 * 'ps_print()' - Print an image to a PostScript printer.
 */

void
ps_print(int       model,		/* I - Model (Level 1 or 2) */
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
  char 		*media_source = v->media_source;
  int 		output_type = v->output_type;
  int		orientation = v->orientation;
  float 	scaling = v->scaling;
  int		top = v->top;
  int		left = v->left;
  int		i, j;		/* Looping vars */
  int		x, y;		/* Looping vars */
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

  if (image_bpp < 3 && cmap == NULL)
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
  * Compute the output size...
  */

  landscape = 0;
  ps_imageable_area(model, ppd_file, media_size, &page_left, &page_right,
                    &page_bottom, &page_top);

  page_width  = page_right - page_left;
  page_height = page_top - page_bottom;

#ifdef DEBUG
  printf("page_width = %d, page_height = %d\n", page_width, page_height);
  printf("image_width = %d, image_height = %d\n", image_width, image_height);
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
  fprintf(prn, "%%Creator: %s\n", Image_get_pluginname(image));
  fprintf(prn, "%%%%CreationDate: %s", ctime(&curtime));
  fputs("%%Copyright: 1997-1999 by Michael Sweet (mike@easysw.com) and Robert Krawitz (rlk@alum.mit.edu)\n", prn);
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
      free(commands[i].command);
    }

    fputs("%%EndProlog\n", prn);
  }

 /*
  * Output the page, rotating as necessary...
  */

  fputs("%%Page: 1\n", prn);
  fputs("gsave\n", prn);

  if (landscape)
  {
    fprintf(prn, "%d %d translate\n", left, top - out_height);
    fprintf(prn, "%.3f %.3f scale\n",
            (float)out_width / ((float)image_height),
            (float)out_height / ((float)image_width));
  }
  else
  {
    fprintf(prn, "%d %d translate\n", left, top);
    fprintf(prn, "%.3f %.3f scale\n",
            (float)out_width / ((float)image_width),
            (float)out_height / ((float)image_height));
  }

  in  = malloc(image_width * image_bpp);
  out = malloc((image_width * out_bpp + 3) * 2);

  if (model == 0)
  {
    fprintf(prn, "/picture %d string def\n", image_width * out_bpp);

    fprintf(prn, "%d %d 8\n", image_width, image_height);

    if (landscape)
      fputs("[ 0 1 1 0 0 0 ]\n", prn);
    else
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
      (*colorfunc)(in, out, image_width, image_bpp, lut, cmap, v);

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

    if (landscape)
      fputs("\t/ImageMatrix [ 0 1 1 0 0 0 ]\n", prn);
    else
      fputs("\t/ImageMatrix [ 1 0 0 -1 0 1 ]\n", prn);

    fputs(">>\n", prn);
    fputs("image\n", prn);

    for (y = 0, out_offset = 0; y < image_height; y ++)
    {
      if ((y & 15) == 0)
	Image_note_progress(image, y, image_height);

      Image_get_row(image, in, y);
      (*colorfunc)(in, out + out_offset, image_width, image_bpp, lut, cmap, v);

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


/*
 * End of "$Id$".
 */
