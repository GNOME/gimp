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
#ifndef __GIMPRC_H__
#define __GIMPRC_H__

#include <glib.h>
#include "libgimp/gimpunit.h"

/*  global gimprc variables  */
extern char *    plug_in_path;
extern char *    temp_path;
extern char *    swap_path;
extern char *    brush_path;
extern char *    default_brush;
extern char *    pattern_path;
extern char *    default_pattern;
extern char *    palette_path;
extern char *    default_palette;
extern char *    gradient_path;
extern char *    default_gradient;
extern char *    pluginrc_path;
extern char *    module_path;
extern int       tile_cache_size;
extern int       marching_speed;
extern int       last_opened_size;
extern double    gamma_val;
extern int       transparency_type;
extern int       perfectmouse;
extern int       transparency_size;
extern int       levels_of_undo;
extern int       color_cube_shades[];
extern int       install_cmap;
extern int       cycled_marching_ants;
extern int       default_threshold;
extern int       stingy_memory_use;
extern int       allow_resize_windows;
extern int       no_cursor_updating;
extern int       preview_size;
extern int       show_rulers;
extern GUnit     default_units;
extern int       show_statusbar;
extern int       auto_save;
extern int       cubic_interpolation;
extern int       confirm_on_close;
extern int       default_width, default_height;
extern int       default_type;
extern GUnit     default_resolution_units;
extern double    default_xresolution;
extern double    default_yresolution;
extern int       save_session_info;
extern int       save_device_status;
extern int       always_restore_session;
extern int       show_tips;
extern int       last_tip;
extern int       show_tool_tips;
extern double    monitor_xres;
extern double    monitor_yres;
extern int       using_xserver_resolution;
extern int       num_processors;
extern char *    image_title_format;
extern int       global_paint_options;
extern int       max_new_image_size;


/*  function prototypes  */
void    init_parse_buffers (); /* this has to be called before any file
				* is parsed
				*/
void    parse_gimprc (void);
void    parse_gimprc_file (char *filename);
void    save_gimprc (GList **updated_options, GList **conflicting_options);
char *  gimprc_find_token (char *token);
void    save_gimprc_strings (gchar *token, gchar *value);

#endif  /*  __GIMPRC_H__  */
