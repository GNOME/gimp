/* The GIMP -- an image manipulation program
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

#include "config.h"

#include <glib-object.h>

#include "libgimpbase/gimpbase.h"

#include "config-types.h"

#include "gimpconfig-params.h"
#include "gimpconfig-types.h"
#include "gimpconfig-utils.h"

#include "gimpdisplayconfig.h"


static void  gimp_display_config_class_init   (GimpDisplayConfigClass *klass);
static void  gimp_display_config_finalize     (GObject      *object);
static void  gimp_display_config_set_property (GObject      *object,
                                               guint         property_id,
                                               const GValue *value,
                                               GParamSpec   *pspec);
static void  gimp_display_config_get_property (GObject      *object,
                                               guint         property_id,
                                               GValue       *value,
                                               GParamSpec   *pspec);


#define DEFAULT_IMAGE_TITLE_FORMAT  "%f-%p.%i (%t)"
#define DEFAULT_IMAGE_STATUS_FORMAT DEFAULT_IMAGE_TITLE_FORMAT

enum
{
  PROP_0,
  PROP_MARCHING_ANTS_SPEED,
  PROP_COLORMAP_CYCLING,
  PROP_RESIZE_WINDOWS_ON_ZOOM,
  PROP_RESIZE_WINDOWS_ON_RESIZE,
  PROP_DEFAULT_DOT_FOR_DOT,
  PROP_PERFECT_MOUSE,
  PROP_CURSOR_MODE,
  PROP_CURSOR_UPDATING,
  PROP_IMAGE_TITLE_FORMAT,
  PROP_IMAGE_STATUS_FORMAT,
  PROP_SHOW_RULERS,
  PROP_SHOW_STATUSBAR,
  PROP_CONFIRM_ON_CLOSE,
  PROP_MONITOR_XRESOLUTION,
  PROP_MONITOR_YRESOLUTION,
  PROP_MONITOR_RES_FROM_GDK
};

static GObjectClass *parent_class = NULL;


GType 
gimp_display_config_get_type (void)
{
  static GType config_type = 0;

  if (! config_type)
    {
      static const GTypeInfo config_info =
      {
        sizeof (GimpDisplayConfigClass),
	NULL,           /* base_init      */
        NULL,           /* base_finalize  */
	(GClassInitFunc) gimp_display_config_class_init,
	NULL,           /* class_finalize */
	NULL,           /* class_data     */
	sizeof (GimpDisplayConfig),
	0,              /* n_preallocs    */
	NULL            /* instance_init  */
      };

      config_type = g_type_register_static (GIMP_TYPE_CORE_CONFIG, 
                                            "GimpDisplayConfig", 
                                            &config_info, 0);
    }

  return config_type;
}

static void
gimp_display_config_class_init (GimpDisplayConfigClass *klass)
{
  GObjectClass *object_class;

  parent_class = g_type_class_peek_parent (klass);

  object_class = G_OBJECT_CLASS (klass);

  object_class->finalize     = gimp_display_config_finalize;
  object_class->set_property = gimp_display_config_set_property;
  object_class->get_property = gimp_display_config_get_property;

  GIMP_CONFIG_INSTALL_PROP_INT (object_class, PROP_MARCHING_ANTS_SPEED,
                                "marching-ants-speed",
                                50, G_MAXINT, 300);
  GIMP_CONFIG_INSTALL_PROP_BOOLEAN (object_class, PROP_COLORMAP_CYCLING,
                                    "colormap-cycling",
                                    FALSE);
  GIMP_CONFIG_INSTALL_PROP_BOOLEAN (object_class, PROP_RESIZE_WINDOWS_ON_ZOOM,
                                    "resize-windows-on-zoom",
                                    FALSE);
  GIMP_CONFIG_INSTALL_PROP_BOOLEAN (object_class, PROP_RESIZE_WINDOWS_ON_RESIZE,
                                    "resize-windows-on-resize",
                                    FALSE);
  GIMP_CONFIG_INSTALL_PROP_BOOLEAN (object_class, PROP_DEFAULT_DOT_FOR_DOT,
                                    "default-dot-for-dot",
                                    TRUE);
  GIMP_CONFIG_INSTALL_PROP_BOOLEAN (object_class, PROP_PERFECT_MOUSE,
                                    "perfect-mouse",
                                    FALSE);
  GIMP_CONFIG_INSTALL_PROP_ENUM (object_class, PROP_CURSOR_MODE,
                                 "cursor-mode",
                                 GIMP_TYPE_CURSOR_MODE, GIMP_CURSOR_MODE_TOOL_ICON);
  GIMP_CONFIG_INSTALL_PROP_BOOLEAN (object_class, PROP_CURSOR_UPDATING,
                                    "cursor-updating",
                                    TRUE);
  GIMP_CONFIG_INSTALL_PROP_STRING (object_class, PROP_IMAGE_TITLE_FORMAT,
                                   "image-title-format",
                                   DEFAULT_IMAGE_TITLE_FORMAT);
  GIMP_CONFIG_INSTALL_PROP_STRING (object_class, PROP_IMAGE_STATUS_FORMAT,
                                   "image-status-format",
                                   DEFAULT_IMAGE_STATUS_FORMAT);
  GIMP_CONFIG_INSTALL_PROP_BOOLEAN (object_class, PROP_SHOW_RULERS,
                                    "show-rulers",
                                    TRUE);
  GIMP_CONFIG_INSTALL_PROP_BOOLEAN (object_class, PROP_SHOW_STATUSBAR,
                                    "show-statusbar",
                                    TRUE);
  GIMP_CONFIG_INSTALL_PROP_BOOLEAN (object_class, PROP_CONFIRM_ON_CLOSE,
                                    "confirm-on-close",
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
}

static void
gimp_display_config_finalize (GObject *object)
{
  GimpDisplayConfig *display_config;

  display_config = GIMP_DISPLAY_CONFIG (object);
  
  g_free (display_config->image_title_format);
  g_free (display_config->image_status_format);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
gimp_display_config_set_property (GObject      *object,
				  guint         property_id,
				  const GValue *value,
				  GParamSpec   *pspec)
{
  GimpDisplayConfig *display_config;

  display_config = GIMP_DISPLAY_CONFIG (object);

  switch (property_id)
    {
    case PROP_MARCHING_ANTS_SPEED:
      display_config->marching_ants_speed = g_value_get_int (value);
      break;
    case PROP_COLORMAP_CYCLING:
      display_config->colormap_cycling = g_value_get_boolean (value);
      break;
    case PROP_RESIZE_WINDOWS_ON_ZOOM:
      display_config->resize_windows_on_zoom = g_value_get_boolean (value);
      break;
    case PROP_RESIZE_WINDOWS_ON_RESIZE:
      display_config->resize_windows_on_resize = g_value_get_boolean (value);
      break;
    case PROP_DEFAULT_DOT_FOR_DOT:
      display_config->default_dot_for_dot = g_value_get_boolean (value);
      break;
    case PROP_PERFECT_MOUSE:
      display_config->perfect_mouse = g_value_get_boolean (value);
      break;
    case PROP_CURSOR_MODE:
      display_config->cursor_mode = g_value_get_enum (value);
      break;
    case PROP_CURSOR_UPDATING:
      display_config->cursor_updating = g_value_get_boolean (value);
      break;
    case PROP_IMAGE_TITLE_FORMAT:
      g_free (display_config->image_title_format);
      display_config->image_title_format = g_value_dup_string (value);
      break;
    case PROP_IMAGE_STATUS_FORMAT:
      g_free (display_config->image_status_format);
      display_config->image_status_format = g_value_dup_string (value);
      break;
    case PROP_SHOW_RULERS:
      display_config->show_rulers = g_value_get_boolean (value);
      break;
    case PROP_SHOW_STATUSBAR:
      display_config->show_statusbar = g_value_get_boolean (value);
      break;
    case PROP_CONFIRM_ON_CLOSE:
      display_config->confirm_on_close = g_value_get_boolean (value);
      break;
    case PROP_MONITOR_XRESOLUTION:
      display_config->monitor_xres = g_value_get_double (value);
      break;
    case PROP_MONITOR_YRESOLUTION:
      display_config->monitor_yres = g_value_get_double (value);
      break;
    case PROP_MONITOR_RES_FROM_GDK:
      display_config->monitor_res_from_gdk = g_value_get_boolean (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_display_config_get_property (GObject    *object,
				  guint       property_id,
				  GValue     *value,
				  GParamSpec *pspec)
{
  GimpDisplayConfig *display_config;

  display_config = GIMP_DISPLAY_CONFIG (object);

  switch (property_id)
    {
    case PROP_MARCHING_ANTS_SPEED:
      g_value_set_int (value, display_config->marching_ants_speed);
      break;
    case PROP_COLORMAP_CYCLING:
      g_value_set_boolean (value, display_config->colormap_cycling);
      break;
    case PROP_RESIZE_WINDOWS_ON_ZOOM:
      g_value_set_boolean (value, display_config->resize_windows_on_zoom);
      break;
    case PROP_RESIZE_WINDOWS_ON_RESIZE:
      g_value_set_boolean (value, display_config->resize_windows_on_resize);
      break;
    case PROP_DEFAULT_DOT_FOR_DOT:
      g_value_set_boolean (value, display_config->default_dot_for_dot);
      break;
    case PROP_PERFECT_MOUSE:
      g_value_set_boolean (value, display_config->perfect_mouse);
      break;
    case PROP_CURSOR_MODE:
      g_value_set_enum (value, display_config->cursor_mode);
      break;
    case PROP_CURSOR_UPDATING:
      g_value_set_boolean (value, display_config->cursor_updating);
      break;
    case PROP_IMAGE_TITLE_FORMAT:
      g_value_set_string (value, display_config->image_title_format);
      break;
    case PROP_IMAGE_STATUS_FORMAT:
      g_value_set_string (value, display_config->image_status_format);
      break;
    case PROP_SHOW_RULERS:
      g_value_set_boolean (value, display_config->show_rulers);
      break;
    case PROP_SHOW_STATUSBAR:
      g_value_set_boolean (value, display_config->show_statusbar);
      break;
    case PROP_CONFIRM_ON_CLOSE:
      g_value_set_boolean (value, display_config->confirm_on_close);
      break;
    case PROP_MONITOR_XRESOLUTION:
      g_value_set_double (value, display_config->monitor_xres);
      break;
    case PROP_MONITOR_YRESOLUTION:
      g_value_set_double (value, display_config->monitor_yres);
      break;
    case PROP_MONITOR_RES_FROM_GDK:
      g_value_set_boolean (value, display_config->monitor_res_from_gdk);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}
