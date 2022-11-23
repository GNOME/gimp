/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * LigmaGuiConfig class
 * Copyright (C) 2001  Sven Neumann <sven@ligma.org>
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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef __LIGMA_GUI_CONFIG_H__
#define __LIGMA_GUI_CONFIG_H__

#include "config/ligmadisplayconfig.h"


#define LIGMA_CONFIG_DEFAULT_THEME          "Default"
#define LIGMA_CONFIG_DEFAULT_ICON_THEME     "Symbolic"


#define LIGMA_TYPE_GUI_CONFIG            (ligma_gui_config_get_type ())
#define LIGMA_GUI_CONFIG(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), LIGMA_TYPE_GUI_CONFIG, LigmaGuiConfig))
#define LIGMA_GUI_CONFIG_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), LIGMA_TYPE_GUI_CONFIG, LigmaGuiConfigClass))
#define LIGMA_IS_GUI_CONFIG(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LIGMA_TYPE_GUI_CONFIG))
#define LIGMA_IS_GUI_CONFIG_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), LIGMA_TYPE_GUI_CONFIG))


typedef struct _LigmaGuiConfigClass LigmaGuiConfigClass;

struct _LigmaGuiConfig
{
  LigmaDisplayConfig    parent_instance;

  gboolean             edit_non_visible;
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
  gboolean             can_change_accels;
  gboolean             save_accels;
  gboolean             restore_accels;
  gint                 last_opened_size;
  guint64              max_new_image_size;
  gboolean             toolbox_color_area;
  gboolean             toolbox_foo_area;
  gboolean             toolbox_image_area;
  gboolean             toolbox_wilber;
  gboolean             toolbox_groups;
  gchar               *theme_path;
  gchar               *theme;
  gboolean             prefer_dark_theme;
  gchar               *icon_theme_path;
  gchar               *icon_theme;
  gboolean             prefer_symbolic_icons;
  gboolean             override_icon_size;
  LigmaIconSize         custom_icon_size;
  gboolean             use_help;
  gboolean             show_help_button;
  gchar               *help_locales;
  LigmaHelpBrowserType  help_browser;
  gboolean             user_manual_online;
  gchar               *user_manual_online_uri;
  gint                 action_history_size;
  LigmaWindowHint       dock_window_hint;
  LigmaHandedness       cursor_handedness;

  /* experimental playground */
  gboolean             playground_npd_tool;
  gboolean             playground_seamless_clone_tool;
  gboolean             playground_paint_select_tool;

  /* saved in sessionrc */
  gboolean             hide_docks;
  gboolean             single_window_mode;
  gboolean             show_tabs;
  LigmaPosition         tabs_position;
  gint                 last_tip_shown;
};

struct _LigmaGuiConfigClass
{
  LigmaDisplayConfigClass  parent_class;
};


GType  ligma_gui_config_get_type (void) G_GNUC_CONST;


#endif /* LIGMA_GUI_CONFIG_H__ */
