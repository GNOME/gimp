/*
 * "$Id$"
 *
 *   Print plug-in for the GIMP.
 *
 *   Copyright 1997-2000 Michael Sweet (mike@easysw.com) and
 *	Robert Krawitz (rlk@alum.mit.edu)
 *   Copyright 2000 Charles Briscoe-Smith <cpbs@debian.org>
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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "print_gimp.h"

#include "print-intl.h"


/*
 * "Image" ADT
 *
 * This file defines an abstract data type called "Image".  An Image wraps
 * a Gimp drawable (or some other application-level image representation)
 * for presentation to the low-level printer drivers (which do CMYK
 * separation, dithering and weaving).  The Image ADT has the ability
 * to perform any combination of flips and rotations on the image,
 * and then deliver individual rows to the driver code.
 *
 * Stuff which might be useful to do in this layer:
 *
 * - Scaling, optionally with interpolation/filtering.
 *
 * - Colour-adjustment.
 *
 * - Multiple-image composition.
 *
 * Also useful might be to break off a thin application-dependent
 * sublayer leaving this layer (which does the interesting stuff)
 * application-independent.
 */


/* Concrete type to represent image */
typedef struct
{
  GDrawable *drawable;
  GPixelRgn  rgn;

  /*
   * Transformations we can impose on the image.  The transformations
   * are considered to be performed in the order given here.
   */

  /* 1: Transpose the x and y axes (flip image over its leading diagonal) */
  int columns;		/* Set if returning columns instead of rows. */

  /* 2: Translate (ox,oy) to the origin */
  int ox, oy;		/* Origin of image */

  /* 3: Flip vertically about the x axis */
  int increment;	/* +1 or -1 for offset of row n+1 from row n. */

  /* 4: Crop to width w, height h */
  int w, h;		/* Width and height of output image */

  /* 5: Flip horizontally about the vertical centre-line of the image */
  int mirror;		/* Set if mirroring rows end-for-end. */

} Gimp_Image_t;


Image
Image_GDrawable_new(GDrawable *drawable)
{
  Gimp_Image_t *i = malloc(sizeof(Gimp_Image_t));
  i->drawable = drawable;
  gimp_pixel_rgn_init(&(i->rgn), drawable, 0, 0,
                      drawable->width, drawable->height, FALSE, FALSE);
  Image_reset((Image) i);
  return i;
}

void
Image_init(Image image)
{
  /* Nothing to do. */
}

void
Image_reset(Image image)
{
  Gimp_Image_t *i = (Gimp_Image_t *) image;
  i->columns = FALSE;
  i->ox = 0;
  i->oy = 0;
  i->increment = 1;
  i->w = i->drawable->width;
  i->h = i->drawable->height;
  i->mirror = FALSE;
}

void
Image_transpose(Image image)
{
  Gimp_Image_t *i = (Gimp_Image_t *) image;
  int tmp;

  if (i->mirror) i->ox += i->w - 1;

  i->columns = !i->columns;

  tmp = i->ox;
  i->ox = i->oy;
  i->oy = tmp;

  tmp = i->mirror;
  i->mirror = i->increment < 0;
  i->increment = tmp ? -1 : 1;

  tmp = i->w;
  i->w = i->h;
  i->h = tmp;

  if (i->mirror) i->ox -= i->w - 1;
}

void
Image_hflip(Image image)
{
  Gimp_Image_t *i = (Gimp_Image_t *) image;
  i->mirror = !i->mirror;
}

void
Image_vflip(Image image)
{
  Gimp_Image_t *i = (Gimp_Image_t *) image;
  i->oy += (i->h-1) * i->increment;
  i->increment = -i->increment;
}

/*
 * Image_crop:
 *
 * Crop the given number of pixels off the LEFT, TOP, RIGHT and BOTTOM
 * of the image.
 */

void
Image_crop(Image image, int left, int top, int right, int bottom)
{
  Gimp_Image_t *i = (Gimp_Image_t *) image;
  int xmax = (i->columns ? i->drawable->height : i->drawable->width) - 1;
  int ymax = (i->columns ? i->drawable->width : i->drawable->height) - 1;

  int nx = i->ox + i->mirror ? right : left;
  int ny = i->oy + top * (i->increment);

  int nw = i->w - left - right;
  int nh = i->h - top - bottom;

  int wmax, hmax;

  if (nx < 0)         nx = 0;
  else if (nx > xmax) nx = xmax;

  if (ny < 0)         ny = 0;
  else if (ny > ymax) ny = ymax;

  wmax = xmax - nx + 1;
  hmax = i->increment ? ny + 1 : ymax - ny + 1;

  if (nw < 1)         nw = 1;
  else if (nw > wmax) nw = wmax;

  if (nh < 1)         nh = 1;
  else if (nh > hmax) nh = hmax;

  i->ox = nx;
  i->oy = ny;
  i->w = nw;
  i->h = nh;
}

void
Image_rotate_ccw(Image image)
{
  Image_transpose(image);
  Image_vflip(image);
}

void
Image_rotate_cw(Image image)
{
  Image_transpose(image);
  Image_hflip(image);
}

void
Image_rotate_180(Image image)
{
  Image_vflip(image);
  Image_hflip(image);
}

int
Image_bpp(Image image)
{
  Gimp_Image_t *i = (Gimp_Image_t *) image;
  return i->drawable->bpp;
}

int
Image_width(Image image)
{
  Gimp_Image_t *i = (Gimp_Image_t *) image;
  return i->w;
}

int
Image_height(Image image)
{
  Gimp_Image_t *i = (Gimp_Image_t *) image;
  return i->h;
}

void
Image_get_row(Image image, unsigned char *data, int row)
{
  Gimp_Image_t *i = (Gimp_Image_t *) image;
  if (i->columns)
    gimp_pixel_rgn_get_col(&(i->rgn), data,
                           i->oy + row * i->increment, i->ox, i->w);
  else
    gimp_pixel_rgn_get_row(&(i->rgn), data,
                           i->ox, i->oy + row * i->increment, i->w);
  if (i->mirror) {
    /* Flip row -- probably inefficiently */
    int f, l, b = i->drawable->bpp;
    for (f = 0, l = i->w - 1; f < l; f++, l--) {
      int c;
      unsigned char tmp;
      for (c = 0; c < b; c++) {
        tmp = data[f*b+c];
        data[f*b+c] = data[l*b+c];
        data[l*b+c] = tmp;
      }
    }
  }
}

void
Image_progress_init(Image image)
{
  gimp_progress_init(_("Printing..."));
}

void
Image_note_progress(Image image, double current, double total)
{
  gimp_progress_update(current / total);
}

void
Image_progress_conclude(Image image)
{
  gimp_progress_update(1);
}

const char *
Image_get_appname(Image image)
{
  static char pluginname[] = PLUG_IN_NAME " plug-in V" PLUG_IN_VERSION
    " for GIMP";
  return pluginname;
}

/*
 * End of "$Id$".
 */
