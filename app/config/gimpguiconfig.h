/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * GimpGuiConfig class
 * Copyright (C) 2001  Sven Neumann <sven@gimp.org>
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

#ifndef __GIMP_GUI_CONFIG_H__
#define __GIMP_GUI_CONFIG_H__

#include "gimpcoreconfig.h"


#define GIMP_TYPE_GUI_CONFIG            (gimp_gui_config_get_type ())
#define GIMP_GUI_CONFIG(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_GUI_CONFIG, GimpGuiConfig))
#define GIMP_GUI_CONFIG_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_GUI_CONFIG, GimpGuiConfigClass))
#define GIMP_IS_GUI_CONFIG(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_GUI_CONFIG))
#define GIMP_IS_GUI_CONFIG_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_GUI_CONFIG))


typedef struct _GimpGuiConfig      GimpGuiConfig;
typedef struct _GimpGuiConfigClass GimpGuiConfigClass;

struct _GimpGuiConfig
{
  GimpCoreConfig       parent_instance;

  gint                 marching_ants_speed;
  gboolean             colormap_cycling;

  gint                 last_opened_size;

  GimpCheckSize        transparency_size;
  GimpCheckType        transparency_type;

  gdouble              gamma_val;

  gboolean             perfect_mouse;

  gboolean             install_cmap;
  gint                 min_colors;

  gint                 default_threshold;

  gboolean             resize_windows_on_zoom;
  gboolean             resize_windows_on_resize;
  GimpPreviewSize      preview_size;
  GimpPreviewSize      nav_preview_size;
  gchar               *image_title_format;
  gboolean             show_rulers;
  gboolean             show_statusbar;
  gboolean             show_tool_tips;
  gboolean             global_paint_options;

  /* the fields below have not yet been implemented as properties */

  gboolean             confirm_on_close;
  gboolean             default_dot_for_dot;
  gboolean             save_device_status;
  gboolean             save_session_info;
  gboolean             always_restore_session;
  gboolean             show_tips;
  gint                 last_tip;
  gdouble              monitor_xres;
  gdouble              monitor_yres;
  gboolean             using_xserver_resolution;
  gboolean             show_indicators;
  guint                max_new_image_size;
  gboolean             trust_dirty_flag;
  gboolean             nav_window_per_display;
  gboolean             info_window_follows_mouse;
  gboolean             use_help;
  gint                 help_browser;
  gint                 cursor_mode;
  gboolean             no_cursor_updating;
  gboolean             disable_tearoff_menus;
  gchar               *theme_path;
  gchar               *theme;
};

struct _GimpGuiConfigClass
{
  GimpCoreConfigClass  parent_class;
};


GType  gimp_gui_config_get_type (void) G_GNUC_CONST;


#endif /* GIMP_GUI_CONFIG_H__ */
