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

#include "config.h"

#include <glib-object.h>

#include "libgimpbase/gimpbase.h"

#include "base/base-enums.h"
#include "core/core-enums.h"

#include "gimpconfig.h"
#include "gimpconfig-params.h"
#include "gimpconfig-types.h"

#include "gimpguiconfig.h"


#define BUILD_DATA_PATH(name)\
  ("${gimp_dir}" G_DIR_SEPARATOR_S name G_SEARCHPATH_SEPARATOR_S \
   "${gimp_datadir}" G_DIR_SEPARATOR_S name)


static void  gimp_gui_config_class_init   (GimpGuiConfigClass *klass);
static void  gimp_gui_config_set_property (GObject            *object,
                                           guint               property_id,
                                           const GValue       *value,
                                           GParamSpec         *pspec);
static void  gimp_gui_config_get_property (GObject            *object,
                                           guint               property_id,
                                           GValue             *value,
                                           GParamSpec         *pspec);

enum
{
  PROP_0,
  PROP_MARCHING_ANTS_SPEED,
  PROP_COLORMAP_CYCLING,
  PROP_LAST_OPENED_SIZE,
  PROP_TRANSPARENCY_SIZE,
  PROP_TRANSPARENCY_TYPE,
  PROP_GAMMA_CORRECTION,
  PROP_PERFECT_MOUSE,
  PROP_INSTALL_COLORMAP,
  PROP_MIN_COLORS,
  PROP_DEFAULT_THRESHOLD,
  PROP_RESIZE_WINDOWS_ON_ZOOM,
  PROP_RESIZE_WINDOWS_ON_RESIZE,
  PROP_PREVIEW_SIZE,
  PROP_NAV_PREVIEW_SIZE,
  PROP_IMAGE_TITLE_FORMAT,
  PROP_SHOW_RULERS,
  PROP_SHOW_STATUSBAR,
  PROP_SHOW_TOOL_TIPS,
  PROP_GLOBAL_PAINT_OPTIONS,
  PROP_CONFIRM_ON_CLOSE,
  PROP_TRUST_DIRTY_FLAG,
  PROP_DEFAULT_DOT_FOR_DOT,
  PROP_NAV_WINDOW_PER_DISPLAY,
  PROP_INFO_WINDOW_PER_DISPLAY,
  PROP_SAVE_DEVICE_STATUS,
  PROP_SAVE_SESSION_INFO,
  PROP_RESTORE_SESSION,
  PROP_SHOW_TIPS,
  PROP_TEAROFF_MENUS,
  PROP_MONITOR_XRESOLUTION,
  PROP_MONITOR_YRESOLUTION,
  PROP_MONITOR_RES_FROM_GDK,
  PROP_MAX_NEW_IMAGE_SIZE,
  PROP_THEME_PATH,
  PROP_THEME
};


GType 
gimp_gui_config_get_type (void)
{
  static GType config_type = 0;

  if (! config_type)
    {
      static const GTypeInfo config_info =
      {
        sizeof (GimpGuiConfigClass),
	NULL,           /* base_init      */
        NULL,           /* base_finalize  */
	(GClassInitFunc) gimp_gui_config_class_init,
	NULL,           /* class_finalize */
	NULL,           /* class_data     */
	sizeof (GimpGuiConfig),
	0,              /* n_preallocs    */
	NULL            /* instance_init  */
      };

      config_type = g_type_register_static (GIMP_TYPE_CORE_CONFIG, 
                                            "GimpGuiConfig", 
                                            &config_info, 0);
    }

  return config_type;
}

static void
gimp_gui_config_class_init (GimpGuiConfigClass *klass)
{
  GObjectClass *object_class;

  object_class = G_OBJECT_CLASS (klass);

  object_class->set_property = gimp_gui_config_set_property;
  object_class->get_property = gimp_gui_config_get_property;

  GIMP_CONFIG_INSTALL_PROP_INT (object_class, PROP_MARCHING_ANTS_SPEED,
                                "marching-ants-speed",
                                50, G_MAXINT, 300);
  GIMP_CONFIG_INSTALL_PROP_BOOLEAN (object_class, PROP_COLORMAP_CYCLING,
                                    "colormap-cycling",
                                    FALSE);
  GIMP_CONFIG_INSTALL_PROP_INT (object_class, PROP_LAST_OPENED_SIZE,
                                "last-opened-size",
                                0, G_MAXINT, 4);
  GIMP_CONFIG_INSTALL_PROP_ENUM (object_class, PROP_TRANSPARENCY_SIZE,
                                 "transparency-size",
                                 GIMP_TYPE_CHECK_SIZE, GIMP_MEDIUM_CHECKS);
  GIMP_CONFIG_INSTALL_PROP_ENUM (object_class, PROP_TRANSPARENCY_TYPE,
                                 "transparency-type",
                                 GIMP_TYPE_CHECK_TYPE, GIMP_GRAY_CHECKS);
  GIMP_CONFIG_INSTALL_PROP_DOUBLE (object_class, PROP_GAMMA_CORRECTION,
                                   "gamma-correction",
                                   0.0, 100.0, 1.0);
  GIMP_CONFIG_INSTALL_PROP_BOOLEAN (object_class, PROP_PERFECT_MOUSE,
                                    "perfect-mouse",
                                    FALSE);
  GIMP_CONFIG_INSTALL_PROP_BOOLEAN (object_class, PROP_INSTALL_COLORMAP,
                                    "install-colormap",
                                    FALSE);
  GIMP_CONFIG_INSTALL_PROP_INT (object_class, PROP_MIN_COLORS,
                                "min-colors",
                                27, 256, 144);
  GIMP_CONFIG_INSTALL_PROP_INT (object_class, PROP_DEFAULT_THRESHOLD,
                                "default-threshold",
                                0, 255, 15);
  GIMP_CONFIG_INSTALL_PROP_BOOLEAN (object_class, PROP_RESIZE_WINDOWS_ON_ZOOM,
                                    "resize-windows-on-zoom",
                                    FALSE);
  GIMP_CONFIG_INSTALL_PROP_BOOLEAN (object_class, PROP_RESIZE_WINDOWS_ON_RESIZE,
                                    "resize-windows-on-resize",
                                    FALSE);
  GIMP_CONFIG_INSTALL_PROP_ENUM (object_class, PROP_PREVIEW_SIZE,
                                 "preview-size",
                                 GIMP_TYPE_PREVIEW_SIZE, GIMP_PREVIEW_SIZE_SMALL);
  GIMP_CONFIG_INSTALL_PROP_ENUM (object_class, PROP_NAV_PREVIEW_SIZE,
                                 "navigation-preview-size",
                                 GIMP_TYPE_PREVIEW_SIZE, GIMP_PREVIEW_SIZE_HUGE);
  GIMP_CONFIG_INSTALL_PROP_STRING (object_class, PROP_IMAGE_TITLE_FORMAT,
                                   "image-title-format",
                                   "%f-%p.%i (%t)");
  GIMP_CONFIG_INSTALL_PROP_BOOLEAN (object_class, PROP_SHOW_RULERS,
                                    "show-rulers",
                                    TRUE);
  GIMP_CONFIG_INSTALL_PROP_BOOLEAN (object_class, PROP_SHOW_STATUSBAR,
                                    "show-statusbar",
                                    TRUE);
  GIMP_CONFIG_INSTALL_PROP_BOOLEAN (object_class, PROP_SHOW_TOOL_TIPS,
                                    "show-tool-tips",
                                    TRUE);
  GIMP_CONFIG_INSTALL_PROP_BOOLEAN (object_class, PROP_GLOBAL_PAINT_OPTIONS,
                                    "global-paint-options",
                                    FALSE);
  GIMP_CONFIG_INSTALL_PROP_BOOLEAN (object_class, PROP_CONFIRM_ON_CLOSE,
                                    "confirm-on-close",
                                    TRUE);
  GIMP_CONFIG_INSTALL_PROP_BOOLEAN (object_class, PROP_TRUST_DIRTY_FLAG,
                                    "trust-dirty-flag",
                                    FALSE);
  GIMP_CONFIG_INSTALL_PROP_BOOLEAN (object_class, PROP_DEFAULT_DOT_FOR_DOT,
                                    "default-dot-for-dot",
                                    TRUE);
  GIMP_CONFIG_INSTALL_PROP_BOOLEAN (object_class, PROP_NAV_WINDOW_PER_DISPLAY,
                                    "navigation-window-per-display",
                                    FALSE);
  GIMP_CONFIG_INSTALL_PROP_BOOLEAN (object_class, PROP_INFO_WINDOW_PER_DISPLAY,
                                    "info-window-per-display",
                                    FALSE);
  GIMP_CONFIG_INSTALL_PROP_BOOLEAN (object_class, PROP_SAVE_DEVICE_STATUS,
                                    "save-device-status",
                                    FALSE);
  GIMP_CONFIG_INSTALL_PROP_BOOLEAN (object_class, PROP_SAVE_SESSION_INFO,
                                    "save-session-info",
                                    TRUE);
  GIMP_CONFIG_INSTALL_PROP_BOOLEAN (object_class, PROP_RESTORE_SESSION,
                                    "restore-session",
                                    TRUE);
  GIMP_CONFIG_INSTALL_PROP_BOOLEAN (object_class, PROP_SHOW_TIPS,
                                    "show-tips",
                                    TRUE);
  GIMP_CONFIG_INSTALL_PROP_BOOLEAN (object_class, PROP_TEAROFF_MENUS,
                                    "tearoff-menus",
                                    TRUE);
  GIMP_CONFIG_INSTALL_PROP_DOUBLE (object_class, PROP_MONITOR_XRESOLUTION,
                                   "monitor-xresolution",
                                   GIMP_MIN_RESOLUTION, G_MAXDOUBLE, 72.0);
  GIMP_CONFIG_INSTALL_PROP_DOUBLE (object_class, PROP_MONITOR_YRESOLUTION,
                                   "monitor-yresolution",
                                   GIMP_MIN_RESOLUTION, G_MAXDOUBLE, 72.0);
  GIMP_CONFIG_INSTALL_PROP_BOOLEAN (object_class, PROP_MONITOR_RES_FROM_GDK,
                                    "monitor-resolution-from-windowing-system",
                                    TRUE);
  GIMP_CONFIG_INSTALL_PROP_MEMSIZE (object_class, PROP_MAX_NEW_IMAGE_SIZE,
                                   "max-new-image-size",
                                   0, G_MAXUINT, 1 << 25);
  GIMP_CONFIG_INSTALL_PROP_PATH (object_class, PROP_THEME_PATH,
                                 "theme-path",
                                 BUILD_DATA_PATH ("themes"));
  GIMP_CONFIG_INSTALL_PROP_STRING (object_class, PROP_THEME,
                                   "theme",
                                   NULL);
}

static void
gimp_gui_config_set_property (GObject      *object,
                              guint         property_id,
                              const GValue *value,
                              GParamSpec   *pspec)
{
  GimpGuiConfig *gui_config;

  gui_config = GIMP_GUI_CONFIG (object);

  switch (property_id)
    {
    case PROP_MARCHING_ANTS_SPEED:
      gui_config->marching_ants_speed = g_value_get_int (value);
      break;
    case PROP_COLORMAP_CYCLING:
      gui_config->colormap_cycling = g_value_get_boolean (value);
      break;
    case PROP_LAST_OPENED_SIZE:
      gui_config->last_opened_size = g_value_get_int (value);
      break;
    case PROP_TRANSPARENCY_SIZE:
      gui_config->transparency_size = g_value_get_enum (value);
      break;
    case PROP_TRANSPARENCY_TYPE:
      gui_config->transparency_type = g_value_get_enum (value);
      break;
    case PROP_GAMMA_CORRECTION:
      gui_config->gamma_val = g_value_get_double (value);
      break;
    case PROP_PERFECT_MOUSE:
      gui_config->perfect_mouse = g_value_get_boolean (value);
      break;
    case PROP_INSTALL_COLORMAP:
      gui_config->install_cmap = g_value_get_boolean (value);
      break;
    case PROP_MIN_COLORS:
      gui_config->min_colors = g_value_get_int (value);
      break;
    case PROP_DEFAULT_THRESHOLD:
      gui_config->default_threshold = g_value_get_int (value);
      break;
    case PROP_RESIZE_WINDOWS_ON_ZOOM:
      gui_config->resize_windows_on_zoom = g_value_get_boolean (value);
      break;
    case PROP_RESIZE_WINDOWS_ON_RESIZE:
      gui_config->resize_windows_on_resize = g_value_get_boolean (value);
      break;
    case PROP_PREVIEW_SIZE:
      gui_config->preview_size = g_value_get_enum (value);
      break;
    case PROP_NAV_PREVIEW_SIZE:
      gui_config->nav_preview_size = g_value_get_enum (value);
      break;
    case PROP_IMAGE_TITLE_FORMAT:
      g_free (gui_config->image_title_format);
      gui_config->image_title_format = g_value_dup_string (value);
      break;
    case PROP_SHOW_RULERS:
      gui_config->show_rulers = g_value_get_boolean (value);
      break;
    case PROP_SHOW_STATUSBAR:
      gui_config->show_statusbar = g_value_get_boolean (value);
      break;
    case PROP_SHOW_TOOL_TIPS:
      gui_config->show_tool_tips = g_value_get_boolean (value);
      break;
    case PROP_GLOBAL_PAINT_OPTIONS:
      gui_config->global_paint_options = g_value_get_boolean (value);
      break;
    case PROP_CONFIRM_ON_CLOSE:
      gui_config->confirm_on_close = g_value_get_boolean (value);
      break;
    case PROP_TRUST_DIRTY_FLAG:
      gui_config->trust_dirty_flag = g_value_get_boolean (value);
      break;
    case PROP_DEFAULT_DOT_FOR_DOT:
      gui_config->default_dot_for_dot = g_value_get_boolean (value);
      break;
    case PROP_NAV_WINDOW_PER_DISPLAY:
      gui_config->nav_window_per_display = g_value_get_boolean (value);
      break;      
    case PROP_INFO_WINDOW_PER_DISPLAY:
      gui_config->info_window_per_display = g_value_get_boolean (value);
      break;      
    case PROP_SAVE_DEVICE_STATUS:
      gui_config->save_device_status = g_value_get_boolean (value);
      break;
    case PROP_SAVE_SESSION_INFO:
      gui_config->save_session_info = g_value_get_boolean (value);
      break;
    case PROP_RESTORE_SESSION:
      gui_config->restore_session = g_value_get_boolean (value);
      break;
    case PROP_SHOW_TIPS:
      gui_config->show_tips = g_value_get_boolean (value);
      break;
    case PROP_TEAROFF_MENUS:
      gui_config->tearoff_menus = g_value_get_boolean (value);
      break;
    case PROP_MONITOR_XRESOLUTION:
      gui_config->monitor_xres = g_value_get_double (value);
      break;
    case PROP_MONITOR_YRESOLUTION:
      gui_config->monitor_yres = g_value_get_double (value);
      break;
    case PROP_MONITOR_RES_FROM_GDK:
      gui_config->monitor_res_from_gdk = g_value_get_boolean (value);
      break;
    case PROP_MAX_NEW_IMAGE_SIZE:
      gui_config->max_new_image_size = g_value_get_uint (value);
      break;
    case PROP_THEME_PATH:
      g_free (gui_config->theme_path);
      gui_config->theme_path = g_value_dup_string (value);
      break;
    case PROP_THEME:
      g_free (gui_config->theme);
      gui_config->theme = g_value_dup_string (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_gui_config_get_property (GObject    *object,
                              guint       property_id,
                              GValue     *value,
                              GParamSpec *pspec)
{
  GimpGuiConfig *gui_config;

  gui_config = GIMP_GUI_CONFIG (object);

  switch (property_id)
    {
    case PROP_MARCHING_ANTS_SPEED:
      g_value_set_int (value, gui_config->marching_ants_speed);
      break;
    case PROP_COLORMAP_CYCLING:
      g_value_set_boolean (value, gui_config->colormap_cycling);
      break;
    case PROP_LAST_OPENED_SIZE:
      g_value_set_int (value, gui_config->last_opened_size);
      break;
    case PROP_TRANSPARENCY_SIZE:
      g_value_set_enum (value, gui_config->transparency_size);
      break;
    case PROP_TRANSPARENCY_TYPE:
      g_value_set_enum (value, gui_config->transparency_type);
      break;
    case PROP_GAMMA_CORRECTION:
      g_value_set_double (value, gui_config->gamma_val);
      break;
    case PROP_PERFECT_MOUSE:
      g_value_set_boolean (value, gui_config->perfect_mouse);
      break;
    case PROP_INSTALL_COLORMAP:
      g_value_set_boolean (value, gui_config->install_cmap);
      break;
    case PROP_MIN_COLORS:
      g_value_set_int (value, gui_config->min_colors);
      break;
    case PROP_DEFAULT_THRESHOLD:
      g_value_set_int (value, gui_config->default_threshold);
      break;
    case PROP_RESIZE_WINDOWS_ON_ZOOM:
      g_value_set_boolean (value, gui_config->resize_windows_on_zoom);
      break;
    case PROP_RESIZE_WINDOWS_ON_RESIZE:
      g_value_set_boolean (value, gui_config->resize_windows_on_resize);
      break;
    case PROP_PREVIEW_SIZE:
      g_value_set_enum (value, gui_config->preview_size);
      break;
    case PROP_NAV_PREVIEW_SIZE:
      g_value_set_enum (value, gui_config->nav_preview_size);
      break;
    case PROP_IMAGE_TITLE_FORMAT:
      g_value_set_string (value, gui_config->image_title_format);
      break;
    case PROP_SHOW_RULERS:
      g_value_set_boolean (value, gui_config->show_rulers);
      break;
    case PROP_SHOW_STATUSBAR:
      g_value_set_boolean (value, gui_config->show_statusbar);
      break;
    case PROP_SHOW_TOOL_TIPS:
      g_value_set_boolean (value, gui_config->show_tool_tips);
      break;
    case PROP_GLOBAL_PAINT_OPTIONS:
      g_value_set_boolean (value, gui_config->global_paint_options);
      break;
    case PROP_CONFIRM_ON_CLOSE:
      g_value_set_boolean (value, gui_config->confirm_on_close);
      break;
    case PROP_TRUST_DIRTY_FLAG:
      g_value_set_boolean (value, gui_config->trust_dirty_flag);
      break;
    case PROP_DEFAULT_DOT_FOR_DOT:
      g_value_set_boolean (value, gui_config->default_dot_for_dot);
      break;
    case PROP_NAV_WINDOW_PER_DISPLAY:
      g_value_set_boolean (value, gui_config->nav_window_per_display);
      break;
    case PROP_INFO_WINDOW_PER_DISPLAY:
      g_value_set_boolean (value, gui_config->info_window_per_display);
      break;
    case PROP_SAVE_DEVICE_STATUS:
      g_value_set_boolean (value, gui_config->save_device_status);
      break;
    case PROP_SAVE_SESSION_INFO:
      g_value_set_boolean (value, gui_config->save_session_info);
      break;
    case PROP_RESTORE_SESSION:
      g_value_set_boolean (value, gui_config->restore_session);
      break;
    case PROP_SHOW_TIPS:
      g_value_set_boolean (value, gui_config->show_tips);
      break;
    case PROP_TEAROFF_MENUS:
      g_value_set_boolean (value, gui_config->tearoff_menus);
      break;
    case PROP_MONITOR_XRESOLUTION:
      g_value_set_double (value, gui_config->monitor_xres);
      break;
    case PROP_MONITOR_YRESOLUTION:
      g_value_set_double (value, gui_config->monitor_yres);
      break;
    case PROP_MONITOR_RES_FROM_GDK:
      g_value_set_boolean (value, gui_config->monitor_res_from_gdk);
      break;
    case PROP_MAX_NEW_IMAGE_SIZE:
      g_value_set_uint (value, gui_config->max_new_image_size);
      break;
    case PROP_THEME_PATH:
      g_value_set_string (value, gui_config->theme_path);
      break;
    case PROP_THEME:
      g_value_set_string (value, gui_config->theme);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}
