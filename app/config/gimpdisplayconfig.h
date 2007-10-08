/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * GimpDisplayConfig class
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

#ifndef __GIMP_DISPLAY_CONFIG_H__
#define __GIMP_DISPLAY_CONFIG_H__

#include "display/display-enums.h"

#include "config/gimpcoreconfig.h"


#define GIMP_CONFIG_DEFAULT_IMAGE_TITLE_FORMAT  "%D*%f-%p.%i (%t, %L) %wx%h"
#define GIMP_CONFIG_DEFAULT_IMAGE_STATUS_FORMAT "%n (%m)"


#define GIMP_TYPE_DISPLAY_CONFIG            (gimp_display_config_get_type ())
#define GIMP_DISPLAY_CONFIG(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_DISPLAY_CONFIG, GimpDisplayConfig))
#define GIMP_DISPLAY_CONFIG_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_DISPLAY_CONFIG, GimpDisplayConfigClass))
#define GIMP_IS_DISPLAY_CONFIG(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_DISPLAY_CONFIG))
#define GIMP_IS_DISPLAY_CONFIG_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_DISPLAY_CONFIG))


typedef struct _GimpDisplayConfigClass GimpDisplayConfigClass;

struct _GimpDisplayConfig
{
  GimpCoreConfig      parent_instance;

  GimpCheckSize       transparency_size;
  GimpCheckType       transparency_type;
  gint                snap_distance;
  gint                marching_ants_speed;
  gboolean            resize_windows_on_zoom;
  gboolean            resize_windows_on_resize;
  gboolean            default_dot_for_dot;
  gboolean            initial_zoom_to_fit;
  gboolean            perfect_mouse;
  GimpCursorMode      cursor_mode;
  gboolean            cursor_updating;
  gboolean            show_brush_outline;
  gboolean            show_paint_tool_cursor;
  gchar              *image_title_format;
  gchar              *image_status_format;
  gboolean            confirm_on_close;
  gdouble             monitor_xres;
  gdouble             monitor_yres;
  gboolean            monitor_res_from_gdk;
  GimpViewSize        nav_preview_size;
  GimpDisplayOptions *default_view;
  GimpDisplayOptions *default_fullscreen_view;
  gboolean            activate_on_focus;
  GimpSpaceBarAction  space_bar_action;
  GimpRGB             xor_color;
  GimpZoomQuality     zoom_quality;
};

struct _GimpDisplayConfigClass
{
  GimpCoreConfigClass  parent_class;
};


GType  gimp_display_config_get_type (void) G_GNUC_CONST;


#endif /* GIMP_DISPLAY_CONFIG_H__ */
