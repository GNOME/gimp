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
#include <gtk/gtk.h>

#include "app_procs.h"
#include "colormaps.h"
#include "gimprc.h"

GdkVisual   *g_visual = NULL;
GdkColormap *g_cmap   = NULL;

gulong g_black_pixel;
gulong g_gray_pixel;
gulong g_white_pixel;
gulong g_color_pixel;
gulong g_normal_guide_pixel;
gulong g_active_guide_pixel;

gulong marching_ants_pixels[8];

static void
set_app_colors (void)
{
  cycled_marching_ants = FALSE;

  g_black_pixel = get_color (0, 0, 0);
  g_gray_pixel  = get_color (127, 127, 127);
  g_white_pixel = get_color (255, 255, 255);
  g_color_pixel = get_color (255, 255, 0);

  g_normal_guide_pixel = get_color (0, 127, 255);
  g_active_guide_pixel = get_color (255, 0, 0);
}

gulong
get_color (int red,
	   int green,
	   int blue)
{
  return gdk_rgb_xpixel_from_rgb ((red << 16) | (green << 8) | blue);
}

void
get_standard_colormaps (void)
{
  g_visual = gdk_rgb_get_visual ();
  g_cmap   = gdk_rgb_get_cmap ();

  gtk_widget_set_default_visual (g_visual);
  gtk_widget_set_default_colormap (g_cmap);

  set_app_colors ();
}
