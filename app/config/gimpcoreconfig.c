/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * GimpCoreConfig class
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

#include "core/core-enums.h"

#include "gimpconfig.h"
#include "gimpconfig-params.h"
#include "gimpconfig-types.h"

#include "gimpcoreconfig.h"


#define BUILD_PLUG_IN_PATH(name)\
  ("${gimp_dir}" G_DIR_SEPARATOR_S name G_SEARCHPATH_SEPARATOR_S \
   "${gimp_plugin_dir}" G_DIR_SEPARATOR_S name)
#define BUILD_DATA_PATH(name)\
  ("${gimp_dir}" G_DIR_SEPARATOR_S name G_SEARCHPATH_SEPARATOR_S \
   "${gimp_datadir}" G_DIR_SEPARATOR_S name)


static void  gimp_core_config_class_init   (GimpCoreConfigClass *klass);
static void  gimp_core_config_set_property (GObject             *object,
                                            guint                property_id,
                                            const GValue        *value,
                                            GParamSpec          *pspec);
static void  gimp_core_config_get_property (GObject             *object,
                                            guint                property_id,
                                            GValue              *value,
                                            GParamSpec          *pspec);

enum
{
  PROP_0,
  PROP_PLUG_IN_PATH,
  PROP_MODULE_PATH,
  PROP_BRUSH_PATH,
  PROP_PATTERN_PATH,
  PROP_PALETTE_PATH,
  PROP_GRADIENT_PATH,
  PROP_DEFAULT_BRUSH,
  PROP_DEFAULT_PATTERN,
  PROP_DEFAULT_PALETTE,
  PROP_DEFAULT_GRADIENT,
  PROP_DEFAULT_COMMENT,
  PROP_DEFAULT_IMAGE_TYPE
};

static GObjectClass *parent_class = NULL;


GType 
gimp_core_config_get_type (void)
{
  static GType config_type = 0;

  if (! config_type)
    {
      static const GTypeInfo config_info =
      {
        sizeof (GimpCoreConfigClass),
	NULL,           /* base_init      */
        NULL,           /* base_finalize  */
	(GClassInitFunc) gimp_core_config_class_init,
	NULL,           /* class_finalize */
	NULL,           /* class_data     */
	sizeof (GimpCoreConfig),
	0,              /* n_preallocs    */
	NULL            /* instance_init  */
      };

      config_type = g_type_register_static (GIMP_TYPE_BASE_CONFIG, 
                                            "GimpCoreConfig", 
                                            &config_info, 0);
    }

  return config_type;
}

static void
gimp_core_config_class_init (GimpCoreConfigClass *klass)
{
  GObjectClass *object_class;

  parent_class = g_type_class_peek_parent (klass);

  object_class = G_OBJECT_CLASS (klass);

  object_class->set_property = gimp_core_config_set_property;
  object_class->get_property = gimp_core_config_get_property;

  GIMP_CONFIG_INSTALL_PROP_PATH (object_class, PROP_PLUG_IN_PATH,
                                 "plug-in-path",
                                 BUILD_PLUG_IN_PATH ("plug-ins"));
  GIMP_CONFIG_INSTALL_PROP_PATH (object_class, PROP_MODULE_PATH,
                                 "module-path",
                                  BUILD_PLUG_IN_PATH ("modules"));
  GIMP_CONFIG_INSTALL_PROP_PATH (object_class, PROP_BRUSH_PATH,
                                 "brush-path",
                                  BUILD_DATA_PATH ("brushes"));
  GIMP_CONFIG_INSTALL_PROP_PATH (object_class, PROP_PATTERN_PATH,
                                 "pattern-path",
                                  BUILD_DATA_PATH ("patterns"));
  GIMP_CONFIG_INSTALL_PROP_PATH (object_class, PROP_PALETTE_PATH,
                                 "palette-path",
                                  BUILD_DATA_PATH ("palettes"));
  GIMP_CONFIG_INSTALL_PROP_PATH (object_class, PROP_GRADIENT_PATH,
                                 "gradient-path",
                                  BUILD_DATA_PATH ("gradients"));
  GIMP_CONFIG_INSTALL_PROP_STRING (object_class, PROP_DEFAULT_BRUSH,
                                   "default-brush",
                                   NULL);
  GIMP_CONFIG_INSTALL_PROP_STRING (object_class, PROP_DEFAULT_PATTERN,
                                   "default-pattern",
                                   NULL);
  GIMP_CONFIG_INSTALL_PROP_STRING (object_class, PROP_DEFAULT_PATTERN,
                                   "default-palette",
                                   NULL);
  GIMP_CONFIG_INSTALL_PROP_STRING (object_class, PROP_DEFAULT_GRADIENT,
                                   "default-gradient",
                                   NULL);
  GIMP_CONFIG_INSTALL_PROP_STRING (object_class, PROP_DEFAULT_COMMENT,
                                   "default-comment",
                                   NULL);
  GIMP_CONFIG_INSTALL_PROP_ENUM (object_class, PROP_DEFAULT_IMAGE_TYPE,
                                 "default-image-type",
                                 GIMP_TYPE_IMAGE_BASE_TYPE, GIMP_RGB);
}

static void
gimp_core_config_set_property (GObject      *object,
                               guint         property_id,
                               const GValue *value,
                               GParamSpec   *pspec)
{
  GimpCoreConfig *core_config;

  core_config = GIMP_CORE_CONFIG (object);

  switch (property_id)
    {
    case PROP_PLUG_IN_PATH:
      g_free (core_config->plug_in_path);
      core_config->plug_in_path = g_value_dup_string (value);
      break;
    case PROP_MODULE_PATH:
      g_free (core_config->module_path);
      core_config->module_path = g_value_dup_string (value);
      break;
    case PROP_BRUSH_PATH:
      g_free (core_config->brush_path);
      core_config->brush_path = g_value_dup_string (value);
      break;
    case PROP_PATTERN_PATH:
      g_free (core_config->pattern_path);
      core_config->pattern_path = g_value_dup_string (value);
      break;
    case PROP_PALETTE_PATH:
      g_free (core_config->palette_path);
      core_config->palette_path = g_value_dup_string (value);
      break;
    case PROP_GRADIENT_PATH:
      g_free (core_config->gradient_path);
      core_config->gradient_path = g_value_dup_string (value);
      break;
    case PROP_DEFAULT_BRUSH:
      g_free (core_config->default_brush);
      core_config->default_brush = g_value_dup_string (value);
      break;
    case PROP_DEFAULT_PATTERN:
      g_free (core_config->default_pattern);
      core_config->default_pattern = g_value_dup_string (value);
      break;
    case PROP_DEFAULT_PALETTE:
      g_free (core_config->default_palette);
      core_config->default_palette = g_value_dup_string (value);
      break;
    case PROP_DEFAULT_GRADIENT:
      g_free (core_config->default_gradient);
      core_config->default_gradient = g_value_dup_string (value);
      break;
    case PROP_DEFAULT_COMMENT:
      g_free (core_config->default_comment);
      core_config->default_comment = g_value_dup_string (value);
      break;
    case PROP_DEFAULT_IMAGE_TYPE:
      core_config->default_image_type = g_value_get_enum (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_core_config_get_property (GObject    *object,
                               guint       property_id,
                               GValue     *value,
                               GParamSpec *pspec)
{
  GimpCoreConfig *core_config;

  core_config = GIMP_CORE_CONFIG (object);

  switch (property_id)
    {
    case PROP_PLUG_IN_PATH:
      g_value_set_string (value, core_config->plug_in_path);
      break;
    case PROP_MODULE_PATH:
      g_value_set_string (value, core_config->module_path);
      break;
    case PROP_BRUSH_PATH:
      g_value_set_string (value, core_config->brush_path);
      break;
    case PROP_PATTERN_PATH:
      g_value_set_string (value, core_config->pattern_path);
      break;
    case PROP_PALETTE_PATH:
      g_value_set_string (value, core_config->palette_path);
      break;
    case PROP_GRADIENT_PATH:
      g_value_set_string (value, core_config->gradient_path);
      break;
    case PROP_DEFAULT_BRUSH:
      g_value_set_string (value, core_config->default_brush);
      break;
    case PROP_DEFAULT_PATTERN:
      g_value_set_string (value, core_config->default_pattern);
      break;
    case PROP_DEFAULT_PALETTE:
      g_value_set_string (value, core_config->default_palette);
      break;
    case PROP_DEFAULT_GRADIENT:
      g_value_set_string (value, core_config->default_gradient);
      break;
    case PROP_DEFAULT_COMMENT:
      g_value_set_string (value, core_config->default_comment);
      break;
    case PROP_DEFAULT_IMAGE_TYPE:
      g_value_set_enum (value, core_config->default_image_type);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}
