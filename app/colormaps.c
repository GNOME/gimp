/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
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
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */
#include <stdlib.h>
#include <math.h>
#include "appenv.h"
#include "app_procs.h"
#include "gimpbrushlist.h"
#include "colormaps.h"
#include "errors.h"
#include "general.h"
#include "gimprc.h"
#include "gradient.h"
#include "palette.h"
#include "patterns.h"
#include "plug_in.h"
#include "temp_buf.h"
#include "tile_swap.h"


GdkVisual *g_visual = NULL;
GdkColormap *g_cmap = NULL;

gulong g_black_pixel;
gulong g_gray_pixel;
gulong g_white_pixel;
gulong g_color_pixel;
gulong g_normal_guide_pixel;
gulong g_active_guide_pixel;

gulong foreground_pixel;
gulong background_pixel;

gulong old_color_pixel;
gulong new_color_pixel;

gulong marching_ants_pixels[8];

GtkDitherInfo *red_ordered_dither;
GtkDitherInfo *green_ordered_dither;
GtkDitherInfo *blue_ordered_dither;
GtkDitherInfo *gray_ordered_dither;

guchar ***ordered_dither_matrix;

/*  These arrays are calculated for quick 24 bit to 16 color conversions  */
gulong *g_lookup_red;
gulong *g_lookup_green;
gulong *g_lookup_blue;

gulong *color_pixel_vals;
gulong *gray_pixel_vals;

static void make_color (gulong *pixel_ptr,
			int     red,
			int     green,
			int     blue,
			int     readwrite);

static void
set_app_colors (void)
{
  cycled_marching_ants = FALSE;

  make_color (&g_black_pixel, 0, 0, 0, FALSE);
  make_color (&g_gray_pixel, 127, 127, 127, FALSE);
  make_color (&g_white_pixel, 255, 255, 255, FALSE);
  make_color (&g_color_pixel, 255, 255, 0, FALSE);
  make_color (&g_normal_guide_pixel, 0, 127, 255, FALSE);
  make_color (&g_active_guide_pixel, 255, 0, 0, FALSE);

  store_color (&foreground_pixel, 0, 0, 0);
  store_color (&background_pixel, 255, 255, 255);
  store_color (&old_color_pixel, 0, 0, 0);
  store_color (&new_color_pixel, 255, 255, 255);

}

/* This probably doesn't belong here - RLL*/
/* static unsigned int
gamma_correct (int intensity, double gamma)
{
  unsigned int val;
  double ind;
  double one_over_gamma;

  if (gamma != 0.0)
    one_over_gamma = 1.0 / gamma;
  else
    one_over_gamma = 1.0;

  ind = (double) intensity / 256.0;
  val = (int) (256 * pow (ind, one_over_gamma));

  return val;
} */


/*************************************************************************/


gulong
get_color (int red,
	   int green,
	   int blue)
{
  return gdk_rgb_xpixel_from_rgb ((red << 16) | (green << 8) | blue);
}


static void
make_color (gulong *pixel_ptr,
	    int     red,
	    int     green,
	    int     blue,
	    int     readwrite)
{
  *pixel_ptr = get_color (red, green, blue);
}

void
store_color (gulong *pixel_ptr,
	     int     red,
	     int     green,
	     int     blue)
{
  *pixel_ptr = get_color (red, green, blue);
}


void
get_standard_colormaps ()
{
  GtkPreviewInfo *info;

  gtk_widget_set_default_visual (gtk_preview_get_visual ());
  gtk_widget_set_default_colormap (gtk_preview_get_cmap ());

  info = gtk_preview_get_info ();
  g_visual = info->visual;

  g_cmap = info->cmap;
#if 0
  color_pixel_vals = info->color_pixels;
  gray_pixel_vals = info->gray_pixels;
  reserved_pixels = info->reserved_pixels;

  red_ordered_dither = info->dither_red;
  green_ordered_dither = info->dither_green;
  blue_ordered_dither = info->dither_blue;
  gray_ordered_dither = info->dither_gray;

  ordered_dither_matrix = info->dither_matrix;

  g_lookup_red = info->lookup_red;
  g_lookup_green = info->lookup_green;
  g_lookup_blue = info->lookup_blue;
#endif

  set_app_colors ();
}
