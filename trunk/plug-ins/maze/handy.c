/* $Id$
 * These routines are useful for working with GIMP and need not be
 * specific to plug-in-maze.
 *
 * Kevin Turner <acapnotic@users.sourceforge.net>
 * http://gimp-plug-ins.sourceforge.net/maze/
 */

/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */

#include "config.h"

#include <string.h>

#include "libgimp/gimp.h"

#include "maze.h"


/* get_colors Returns the current foreground and background colors in
   nice little arrays.  It works nicely for RGB and grayscale images,
   however handling of indexed images is somewhat broken.  Patches
   appreciated. */

void
get_colors (GimpDrawable *drawable,
	    guint8       *fg,
	    guint8       *bg)
{
  GimpRGB foreground;
  GimpRGB background;

  gimp_context_get_foreground (&foreground);
  gimp_context_get_background (&background);

  fg[0] = fg[1] = fg[2] = fg[3] = 255;
  bg[0] = bg[1] = bg[2] = bg[3] = 255;

  switch ( gimp_drawable_type (drawable->drawable_id) )
    {
    case GIMP_RGB_IMAGE:
    case GIMP_RGBA_IMAGE:
      gimp_rgb_get_uchar (&foreground, &fg[0], &fg[1], &fg[2]);
      gimp_rgb_get_uchar (&background, &bg[0], &bg[1], &bg[2]);
      break;

    case GIMP_GRAYA_IMAGE:
    case GIMP_GRAY_IMAGE:
      fg[0] = gimp_rgb_luminance_uchar (&foreground);
      bg[0] = gimp_rgb_luminance_uchar (&background);
      break;

    case GIMP_INDEXEDA_IMAGE:
    case GIMP_INDEXED_IMAGE:     /* FIXME: Should use current fg/bg colors.  */
	g_warning("maze: get_colors: Indexed image.  Using colors 15 and 0.\n");
	fg[0] = 15;      /* As a plugin, I protest.  *I* shouldn't be the */
	bg[0] = 0;       /* one who has to deal with this colormapcrap.   */
	break;

    default:
      break;
    }
}

/* Draws a solid color box in a GimpPixelRgn. */
/* Optimization assumptions:
 * (Or, "Why Maze is Faster Than Checkerboard.")
 *
 * Assuming calling memcpy is faster than using loops.
 * Row buffers are nice...
 *
 * Assume allocating memory for row buffers takes a significant amount
 * of time.  Assume drawbox will be called many times.
 * Only allocate memory once.
 *
 * Do not assume the row buffer will always be the same size.  Allow
 * for reallocating to make it bigger if needed.  However, I don't see
 * reason to bother ever shrinking it again.
 * (Under further investigation, assuming the row buffer never grows
 * may be a safe assumption in this case.)
 *
 * Also assume that the program calling drawbox is short-lived, so
 * memory leaks aren't of particular concern-- the memory allocated to
 * the row buffer is never set free.
 */

/* Further optimizations that could be made...
 *  Currently, the row buffer is re-filled with every call.  However,
 *  plug-ins such as maze and checkerboard only use two colors, and
 *  for the most part, have rows of the same size with every call.
 *  We could keep a row of each color on hand so we wouldn't have to
 *  re-fill it every time...  */

#include "config.h"

#include <stdlib.h>

#include "libgimp/gimp.h"

#include "maze.h"


void
drawbox( GimpPixelRgn *dest_rgn,
	 guint x, guint y, guint w, guint h,
	 guint8 clr[4])
{
     const guint bpp = dest_rgn->bpp;
     const guint x_min = x * bpp;

     /* x_max = dest_rgn->bpp * MIN(dest_rgn->w, (x + w)); */
     /* rowsize = x_max - x_min */
     const guint rowsize = bpp * MIN(dest_rgn->w, (x + w)) - x_min;

     /* The maximum [xy] value is that of the far end of the box, or
      * the edge of the region, whichever comes first. */
     const guint y_max = dest_rgn->rowstride * MIN(dest_rgn->h, (y + h));

     static guint8 *rowbuf;
     static guint high_size = 0;

     guint xx, yy;

     /* Does the row buffer need to be (re)allocated? */
     if (high_size == 0)
       {
	 rowbuf = g_new (guint8, rowsize);
       }
     else if (rowsize > high_size)
       {
	 rowbuf = g_renew (guint8, rowbuf, rowsize);
       }

     high_size = MAX(high_size, rowsize);

     /* Fill the row buffer with the color. */
     for (xx = 0; xx < rowsize; xx += bpp)
       {
	 memcpy (&rowbuf[xx], clr, bpp);
       }

     /* Fill in the box in the region with rows... */
     for (yy = dest_rgn->rowstride * y; yy < y_max; yy += dest_rgn->rowstride)
       {
	 memcpy (&dest_rgn->data[yy + x_min], rowbuf, rowsize);
       }
}
