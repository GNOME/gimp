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

typedef struct _GimpRc GimpRc;

/*  global gimprc variables  - need some comments on this stuff */
struct _GimpRc 
{
  gchar             *plug_in_path;
  gchar             *brush_path;
  gchar             *default_brush;
  gchar             *pattern_path;
  gchar             *default_pattern;
  gchar             *palette_path;
  gchar             *default_palette;
  gchar             *gradient_path;
  gchar             *default_gradient;
  gchar             *pluginrc_path;
  gchar             *module_path;
  gint               marching_speed;
  gint               last_opened_size;
  gdouble            gamma_val;
  gint               transparency_type;
  gboolean           perfectmouse;
  gint               transparency_size;
  gint               levels_of_undo;
  gint               min_colors;
  gboolean           install_cmap;
  gboolean           cycled_marching_ants;
  gint               default_threshold;
  gboolean           allow_resize_windows;
  gboolean           no_cursor_updating;
  gint               preview_size;
  gint               nav_preview_size;
  gboolean           show_rulers;
  GimpUnit           default_units;
  gboolean           show_statusbar;
  gboolean           auto_save;
  gboolean           confirm_on_close;
  gint               default_width, default_height;
  gint               default_type;
  GimpUnit           default_resolution_units;
  gdouble            default_xresolution;
  gdouble            default_yresolution;
  gchar             *default_comment;
  gboolean           default_dot_for_dot;
  gboolean           save_session_info;
  gboolean           save_device_status;
  gboolean           always_restore_session;
  gboolean           show_tips;
  gint               last_tip;
  gboolean           show_tool_tips;
  gdouble            monitor_xres;
  gdouble            monitor_yres;
  gboolean           using_xserver_resolution;
  gchar             *image_title_format;
  gboolean           global_paint_options;
  gchar             *module_db_load_inhibit;
  gboolean           show_indicators;
  guint              max_new_image_size;
  gint               thumbnail_mode;
  gboolean           trust_dirty_flag;
  gboolean           use_help;
  gboolean           nav_window_per_display;
  gboolean           info_window_follows_mouse;
  gint               help_browser;
  gint               cursor_mode;
  gboolean           disable_tearoff_menus;
};

extern GimpRc gimprc;

/*  function prototypes  */
gboolean    gimprc_init         (void); /* this has to be called before any file
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
