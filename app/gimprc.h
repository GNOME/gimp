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
  gint               marching_speed;
  gint               last_opened_size;
  gdouble            gamma_val;
  gint               transparency_type;
  gboolean           perfectmouse;
  gint               transparency_size;
  gint               min_colors;
  gboolean           install_cmap;
  gboolean           cycled_marching_ants;
  gint               default_threshold;
  gboolean           allow_resize_windows;
  gboolean           no_cursor_updating;
  gint               preview_size;
  gint               nav_preview_size;
  gboolean           show_rulers;
  gboolean           show_statusbar;
  gboolean           auto_save;
  gboolean           confirm_on_close;
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
  gboolean           show_indicators;
  guint              max_new_image_size;
  gboolean           trust_dirty_flag;
  gboolean           use_help;
  gboolean           nav_window_per_display;
  gboolean           info_window_follows_mouse;
  gint               help_browser;
  gint               cursor_mode;
  gboolean           disable_tearoff_menus;
  gchar             *theme_path;
  gchar             *theme;
};

extern GimpRc gimprc;

/* this has to be called before any file is parsed
 */
gboolean      gimprc_init         (Gimp         *gimp);

void          gimprc_parse        (Gimp         *gimp);
void          gimprc_save         (GList       **updated_options,
				   GList       **conflicting_options);

gboolean      gimprc_parse_file   (const gchar  *filename);

const gchar * gimprc_find_token   (const gchar  *token);
gchar       * gimprc_value_to_str (const gchar  *name);
void          save_gimprc_strings (const gchar  *token,
				   const gchar  *value);


#endif  /*  __GIMPRC_H__  */
