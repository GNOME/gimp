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
#include "apptypes.h"

/*  global gimprc variables  */
extern gchar             *plug_in_path;
extern gchar             *temp_path;
extern gchar             *swap_path;
extern gchar             *brush_path;
extern gchar             *brush_vbr_path;
extern gchar             *default_brush;
extern gchar             *pattern_path;
extern gchar             *default_pattern;
extern gchar             *palette_path;
extern gchar             *default_palette;
extern gchar             *gradient_path;
extern gchar             *default_gradient;
extern gchar             *pluginrc_path;
extern gchar             *module_path;
extern gint               tile_cache_size;
extern gint               marching_speed;
extern gint               last_opened_size;
extern gdouble            gamma_val;
extern gint               transparency_type;
extern gboolean           perfectmouse;
extern gint               transparency_size;
extern gint               levels_of_undo;
extern gint               color_cube_shades[];
extern gboolean           install_cmap;
extern gboolean           cycled_marching_ants;
extern gint               default_threshold;
extern gboolean           stingy_memory_use;
extern gboolean           allow_resize_windows;
extern gboolean           no_cursor_updating;
extern gint               preview_size;
extern gint               nav_preview_size;
extern gboolean           show_rulers;
extern GimpUnit           default_units;
extern gboolean           show_statusbar;
extern gboolean           auto_save;
extern InterpolationType  interpolation_type;
extern gboolean           confirm_on_close;
extern gint               default_width, default_height;
extern gint               default_type;
extern GimpUnit           default_resolution_units;
extern gdouble            default_xresolution;
extern gdouble            default_yresolution;
extern gchar             *default_comment;
extern gboolean           default_dot_for_dot;
extern gboolean           save_session_info;
extern gboolean           save_device_status;
extern gboolean           always_restore_session;
extern gboolean           show_tips;
extern gint               last_tip;
extern gboolean           show_tool_tips;
extern gdouble            monitor_xres;
extern gdouble            monitor_yres;
extern gboolean           using_xserver_resolution;
extern gint               num_processors;
extern gchar             *image_title_format;
extern gboolean           global_paint_options;
extern gboolean           show_indicators;
extern gint               max_new_image_size;
extern gint               thumbnail_mode;
extern gboolean           trust_dirty_flag;
extern gboolean           use_help;
extern gboolean           nav_window_per_display;
extern gboolean           info_window_follows_mouse;
extern gint               help_browser;

/*  function prototypes  */
gboolean    parse_buffers_init  (void); /* this has to be called before any file
					 * is parsed
					 */
void        parse_gimprc        (void);
gboolean    parse_gimprc_file   (gchar  *filename);
void        save_gimprc         (GList **updated_options,
				 GList **conflicting_options);
gchar     * gimprc_find_token   (gchar  *token);
gchar     * gimprc_value_to_str (gchar  *name);
void        save_gimprc_strings (gchar  *token,
				 gchar  *value);

#endif  /*  __GIMPRC_H__  */
