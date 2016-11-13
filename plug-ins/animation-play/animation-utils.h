/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * animation-utils.c
 * Copyright (C) 2015 Jehan <jehan@gimp.org>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef __ANIMATION_UTILS_H__
#define __ANIMATION_UTILS_H__

#include <gtk/gtk.h>

#define PLUG_IN_PROC       "plug-in-animationplay"
#define PLUG_IN_BINARY     "animation-play"
#define PLUG_IN_ROLE       "gimp-animation-playback"

#define MAX_FRAMERATE      300.0
#define DEFAULT_FRAMERATE  24.0

void         total_alpha_preview (guchar            *drawing_data,
                                  guint              drawing_width,
                                  guint              drawing_height);

GeglBuffer * normal_blend        (gint               width,
                                  gint               height,
                                  GeglBuffer        *backdrop,
                                  gdouble            backdrop_scale_ratio,
                                  gint               backdrop_offset_x,
                                  gint               backdrop_offset_y,
                                  GeglBuffer        *source,
                                  gdouble            source_scale_ratio,
                                  gint               source_offset_x,
                                  gint               source_offset_y);

void         show_scrolled_child (GtkScrolledWindow *window,
                                  GtkWidget         *child);
#endif  /*  __ANIMATION_UTILS_H__  */


