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
#include "brushes.h"
#include "colormaps.h"
#include "errors.h"
#include "general.h"
#include "gimprc.h"
#include "gradient.h"
#include "palette.h"
#include "patterns.h"
#include "plug_in.h"
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

static int reserved_entries = 4;  /* extra colors aside from color cube */
static gulong *reserved_pixels;

static void make_color (gulong *pixel_ptr,
			int     red,
			int     green,
			int     blue,
			int     readwrite);

static void
set_app_colors ()
{
  int i;

  if ((g_visual->type == GDK_VISUAL_PSEUDO_COLOR) ||
      (g_visual->type == GDK_VISUAL_GRAYSCALE))
    {
      foreground_pixel = reserved_pixels[0];
      background_pixel = reserved_pixels[1];
      old_color_pixel = reserved_pixels[2];
      new_color_pixel = reserved_pixels[3];
    }
  else
    {
      cycled_marching_ants = FALSE;
    }

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

  /* marching ants pixels--if enabled */
  if (cycled_marching_ants)
    for (i = 0; i < 8; i++)
      {
	marching_ants_pixels[i] = reserved_pixels[i + reserved_entries - 8];
	if (i < 4)
	  store_color (&marching_ants_pixels[i], 0, 0, 0);
	else
	  store_color (&marching_ants_pixels[i], 255, 255, 255);
      }
}


static unsigned int
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
}


/*************************************************************************/


gulong
get_color (int red,
	   int green,
	   int blue)
{
  gulong pixel;

  if ((g_visual->type == GDK_VISUAL_PSEUDO_COLOR) ||
      (g_visual->type == GDK_VISUAL_GRAYSCALE))
    pixel = color_pixel_vals [(red_ordered_dither[red].s[1] +
			       green_ordered_dither[green].s[1] +
			       blue_ordered_dither[blue].s[1])];
  else
    store_color (&pixel, red, green, blue);

  return pixel;
}


static void
make_color (gulong *pixel_ptr,
	    int     red,
	    int     green,
	    int     blue,
	    int     readwrite)
{
  GdkColor col;

  red = gamma_correct (red, gamma_val);
  green = gamma_correct (green, gamma_val);
  blue = gamma_correct (blue, gamma_val);

  col.red = red * (65535 / 255);
  col.green = green * (65535 / 255);
  col.blue = blue * (65535 / 255);
  col.pixel = *pixel_ptr;

  if (readwrite && ((g_visual->type == GDK_VISUAL_PSEUDO_COLOR) ||
		    (g_visual->type == GDK_VISUAL_GRAYSCALE)))
    gdk_color_change (g_cmap, &col);
  else
    gdk_color_alloc (g_cmap, &col);

  *pixel_ptr = col.pixel;
}

void
store_color (gulong *pixel_ptr,
	     int     red,
	     int     green,
	     int     blue)
{
  make_color (pixel_ptr, red, green, blue, TRUE);
}


void
get_standard_colormaps ()
{
  GtkPreviewInfo *info;

  if (cycled_marching_ants)
    reserved_entries += 8;

  gtk_preview_set_gamma (gamma_val);
  gtk_preview_set_color_cube (color_cube_shades[0], color_cube_shades[1],
			      color_cube_shades[2], color_cube_shades[3]);
  gtk_preview_set_install_cmap (install_cmap);
  gtk_preview_set_reserved (reserved_entries);

  /* so we can reinit the colormaps */
  gtk_preview_reset ();

  gtk_widget_set_default_visual (gtk_preview_get_visual ());
  gtk_widget_set_default_colormap (gtk_preview_get_cmap ());

  info = gtk_preview_get_info ();
  g_visual = info->visual;

  if (((g_visual->type == GDK_VISUAL_PSEUDO_COLOR) ||
       (g_visual->type == GDK_VISUAL_GRAYSCALE)) &&
      info->reserved_pixels == NULL) {
    g_print("GIMP cannot get enough colormaps to boot.\n");
    g_print("Try exiting other color intensive applications.\n");
    g_print("Also try enabling the (install-colormap) option in gimprc.\n");
    swapping_free ();
    brushes_free ();
    patterns_free ();
    palettes_free ();
    gradients_free ();
    palette_free ();
    procedural_db_free ();
    plug_in_kill ();
    tile_swap_exit ();
    gtk_exit(0);
  }

  g_cmap = info->cmap;
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

  set_app_colors ();
}
