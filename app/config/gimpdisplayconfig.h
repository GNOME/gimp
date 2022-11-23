/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * LigmaDisplayConfig class
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

#ifndef __LIGMA_DISPLAY_CONFIG_H__
#define __LIGMA_DISPLAY_CONFIG_H__

#include "config/ligmacoreconfig.h"


#define LIGMA_CONFIG_DEFAULT_IMAGE_TITLE_FORMAT  "%D*%f-%p.%i (%t, %o, %L) %wx%h"
#define LIGMA_CONFIG_DEFAULT_IMAGE_STATUS_FORMAT "%n (%m)"


#define LIGMA_TYPE_DISPLAY_CONFIG            (ligma_display_config_get_type ())
#define LIGMA_DISPLAY_CONFIG(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), LIGMA_TYPE_DISPLAY_CONFIG, LigmaDisplayConfig))
#define LIGMA_DISPLAY_CONFIG_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), LIGMA_TYPE_DISPLAY_CONFIG, LigmaDisplayConfigClass))
#define LIGMA_IS_DISPLAY_CONFIG(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LIGMA_TYPE_DISPLAY_CONFIG))
#define LIGMA_IS_DISPLAY_CONFIG_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), LIGMA_TYPE_DISPLAY_CONFIG))


typedef struct _LigmaDisplayConfigClass LigmaDisplayConfigClass;

struct _LigmaDisplayConfig
{
  LigmaCoreConfig      parent_instance;

  LigmaCheckSize       transparency_size;
  LigmaCheckType       transparency_type;
  LigmaRGB             transparency_custom_color1;
  LigmaRGB             transparency_custom_color2;
  gint                snap_distance;
  gint                marching_ants_speed;
  gboolean            resize_windows_on_zoom;
  gboolean            resize_windows_on_resize;
  gboolean            default_show_all;
  gboolean            default_dot_for_dot;
  gboolean            initial_zoom_to_fit;
  LigmaDragZoomMode    drag_zoom_mode;
  gboolean            drag_zoom_speed;
  LigmaCursorMode      cursor_mode;
  gboolean            cursor_updating;
  gboolean            show_brush_outline;
  gboolean            snap_brush_outline;
  gboolean            show_paint_tool_cursor;
  gchar              *image_title_format;
  gchar              *image_status_format;
  gdouble             monitor_xres;
  gdouble             monitor_yres;
  gboolean            monitor_res_from_gdk;
  LigmaViewSize        nav_preview_size;
  LigmaDisplayOptions *default_view;
  LigmaDisplayOptions *default_fullscreen_view;
  gboolean            activate_on_focus;
  LigmaSpaceBarAction  space_bar_action;
  LigmaZoomQuality     zoom_quality;
  gboolean            use_event_history;

  GObject            *modifiers_manager;
};

struct _LigmaDisplayConfigClass
{
  LigmaCoreConfigClass  parent_class;
};


GType  ligma_display_config_get_type (void) G_GNUC_CONST;


#endif /* LIGMA_DISPLAY_CONFIG_H__ */
