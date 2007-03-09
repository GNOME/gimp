/* GIMP - The GNU Image Manipulation Program
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

#include "widgets/widgets-enums.h"

#include "config/gimpdisplayconfig.h"


#define GIMP_TYPE_GUI_CONFIG            (gimp_gui_config_get_type ())
#define GIMP_GUI_CONFIG(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_GUI_CONFIG, GimpGuiConfig))
#define GIMP_GUI_CONFIG_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_GUI_CONFIG, GimpGuiConfigClass))
#define GIMP_IS_GUI_CONFIG(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_GUI_CONFIG))
#define GIMP_IS_GUI_CONFIG_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_GUI_CONFIG))


typedef struct _GimpGuiConfigClass GimpGuiConfigClass;

struct _GimpGuiConfig
{
  GimpDisplayConfig    parent_instance;

  gint                 default_threshold;
  gboolean             move_tool_changes_active;
  gboolean             trust_dirty_flag;
  gboolean             save_device_status;
  gboolean             save_session_info;
  gboolean             restore_session;
  gboolean             save_tool_options;
  gboolean             show_tips;
  gboolean             show_tooltips;
  gboolean             tearoff_menus;
  gboolean             can_change_accels;
  gboolean             save_accels;
  gboolean             restore_accels;
  gboolean             menu_mnemonics;
  gint                 last_opened_size;
  guint64              max_new_image_size;
  gboolean             toolbox_color_area;
  gboolean             toolbox_foo_area;
  gboolean             toolbox_image_area;
  gchar               *theme_path;
  gchar               *theme;
  gboolean             use_help;
  gboolean             show_help_button;
  gchar               *help_locales;
  GimpHelpBrowserType  help_browser;
  gchar               *web_browser;
  GimpWindowHint       toolbox_window_hint;
  GimpWindowHint       dock_window_hint;
  gboolean             transient_docks;
  GimpCursorFormat     cursor_format;

  gint                 last_tip;  /* saved in sessionrc */
};

struct _GimpGuiConfigClass
{
  GimpDisplayConfigClass  parent_class;
};


GType  gimp_gui_config_get_type (void) G_GNUC_CONST;


#endif /* GIMP_GUI_CONFIG_H__ */
