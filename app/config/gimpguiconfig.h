/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * GimpGuiConfig class
 * Copyright (C) 2001  Sven Neumann <sven@gimp.org>
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

#ifndef __GIMP_GUI_CONFIG_H__
#define __GIMP_GUI_CONFIG_H__

#include "config/gimpdisplayconfig.h"


#define GIMP_CONFIG_DEFAULT_THEME          "Dark"
#define GIMP_CONFIG_DEFAULT_ICON_THEME     "Symbolic"


#define GIMP_TYPE_GUI_CONFIG            (gimp_gui_config_get_type ())
#define GIMP_GUI_CONFIG(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_GUI_CONFIG, GimpGuiConfig))
#define GIMP_GUI_CONFIG_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_GUI_CONFIG, GimpGuiConfigClass))
#define GIMP_IS_GUI_CONFIG(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_GUI_CONFIG))
#define GIMP_IS_GUI_CONFIG_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_GUI_CONFIG))


typedef struct _GimpGuiConfigClass GimpGuiConfigClass;

struct _GimpGuiConfig
{
  GimpDisplayConfig    parent_instance;

  gboolean             move_tool_changes_active;
  gint                 filter_tool_max_recent;
  gboolean             filter_tool_use_last_settings;
  gboolean             filter_tool_show_color_options;
  gboolean             trust_dirty_flag;
  gboolean             save_device_status;
  gboolean             devices_share_tool;
  gboolean             save_session_info;
  gboolean             restore_session;
  gboolean             restore_monitor;
  gboolean             save_tool_options;
  gboolean             show_tooltips;
  gboolean             can_change_accels;
  gboolean             save_accels;
  gboolean             restore_accels;
  gint                 last_opened_size;
  guint64              max_new_image_size;
  gboolean             toolbox_color_area;
  gboolean             toolbox_foo_area;
  gboolean             toolbox_image_area;
  gboolean             toolbox_wilber;
  gchar               *theme_path;
  gchar               *theme;
  gboolean             prefer_dark_theme;
  gchar               *icon_theme_path;
  gchar               *icon_theme;
  GimpIconSize         icon_size;
  gboolean             use_help;
  gboolean             show_help_button;
  gchar               *help_locales;
  GimpHelpBrowserType  help_browser;
  gboolean             user_manual_online;
  gchar               *user_manual_online_uri;
  gboolean             search_show_unavailable;
  gint                 action_history_size;
  GimpWindowHint       dock_window_hint;
  GimpHandedness       cursor_handedness;

  /* experimental playground */
  gboolean             playground_npd_tool;
  gboolean             playground_seamless_clone_tool;

  /* saved in sessionrc */
  gboolean             hide_docks;
  gboolean             single_window_mode;
  GimpPosition         tabs_position;
  gint                 last_tip_shown;
};

struct _GimpGuiConfigClass
{
  GimpDisplayConfigClass  parent_class;

  void (* size_changed) (GimpGuiConfig *config);
};


GType  gimp_gui_config_get_type (void) G_GNUC_CONST;

GimpIconSize gimp_gui_config_detect_icon_size (GimpGuiConfig *config);

#endif /* GIMP_GUI_CONFIG_H__ */
