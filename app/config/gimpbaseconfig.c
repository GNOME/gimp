/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * GimpBaseConfig class
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

#include "base/base-enums.h"

#include "gimpconfig.h"
#include "gimpconfig-params.h"
#include "gimpconfig-types.h"

#include "gimpbaseconfig.h"


static void  gimp_base_config_class_init   (GimpBaseConfigClass *klass);
static void  gimp_base_config_set_property (GObject             *object,
                                            guint                property_id,
                                            const GValue        *value,
                                            GParamSpec          *pspec);
static void  gimp_base_config_get_property (GObject             *object,
                                            guint                property_id,
                                            GValue              *value,
                                            GParamSpec          *pspec);


enum
{
  PROP_0,
  PROP_TEMP_PATH,
  PROP_SWAP_PATH,
  PROP_STINGY_MEMORY_USE,
  PROP_NUM_PROCESSORS,
  PROP_TILE_CACHE_SIZE,
  PROP_INTERPOLATION_TYPE,
};

static GObjectClass *parent_class = NULL;


GType 
gimp_base_config_get_type (void)
{
  static GType config_type = 0;

  if (! config_type)
    {
      static const GTypeInfo config_info =
      {
        sizeof (GimpBaseConfigClass),
	NULL,           /* base_init      */
        NULL,           /* base_finalize  */
	(GClassInitFunc) gimp_base_config_class_init,
	NULL,           /* class_finalize */
	NULL,           /* class_data     */
	sizeof (GimpBaseConfig),
	0,              /* n_preallocs    */
	NULL            /* instance_init  */
      };
      static const GInterfaceInfo config_iface_info = { NULL, NULL, NULL };

      config_type = g_type_register_static (G_TYPE_OBJECT, 
                                            "GimpBaseConfig", 
                                            &config_info, 0);

      g_type_add_interface_static (config_type, 
                                   GIMP_TYPE_CONFIG_INTERFACE,
                                   &config_iface_info);
    }

  return config_type;
}

static void
gimp_base_config_class_init (GimpBaseConfigClass *klass)
{
  GObjectClass *object_class;

  parent_class = g_type_class_peek_parent (klass);

  object_class = G_OBJECT_CLASS (klass);

  object_class->set_property = gimp_base_config_set_property;
  object_class->get_property = gimp_base_config_get_property;

  GIMP_CONFIG_INSTALL_PROP_PATH (object_class, PROP_TEMP_PATH,
                                 "temp-path",
                                 "${gimp_dir}" G_DIR_SEPARATOR_S "tmp");
  GIMP_CONFIG_INSTALL_PROP_PATH (object_class, PROP_SWAP_PATH,
                                 "swap-path",
                                 "${gimp_dir}");
  GIMP_CONFIG_INSTALL_PROP_BOOLEAN (object_class, PROP_STINGY_MEMORY_USE,
                                    "stingy-memory-use",
                                    FALSE);
  GIMP_CONFIG_INSTALL_PROP_UINT (object_class, PROP_NUM_PROCESSORS,
                                 "num-processors",
                                 1, 30, 1);
  GIMP_CONFIG_INSTALL_PROP_MEMSIZE (object_class, PROP_TILE_CACHE_SIZE,
                                    "tile-cache-size",
                                    0, G_MAXUINT, 1 << 25);
  GIMP_CONFIG_INSTALL_PROP_ENUM (object_class, PROP_INTERPOLATION_TYPE,
                                 "interpolation-type",
                                 GIMP_TYPE_INTERPOLATION_TYPE, 
                                 GIMP_LINEAR_INTERPOLATION);
}

static void
gimp_base_config_set_property (GObject      *object,
                               guint         property_id,
                               const GValue *value,
                               GParamSpec   *pspec)
{
  GimpBaseConfig *base_config;

  base_config = GIMP_BASE_CONFIG (object);

  switch (property_id)
    {
    case PROP_TEMP_PATH:
      g_free (base_config->temp_path);
      base_config->temp_path = g_value_dup_string (value);
      break;
    case PROP_SWAP_PATH:
      g_free (base_config->swap_path);
      base_config->swap_path = g_value_dup_string (value);
      break;
    case PROP_STINGY_MEMORY_USE:
      base_config->stingy_memory_use = g_value_get_boolean (value);
      break;
    case PROP_NUM_PROCESSORS:
      base_config->num_processors = g_value_get_uint (value);
      break;
    case PROP_TILE_CACHE_SIZE:
      base_config->tile_cache_size = g_value_get_uint (value);
      break;
    case PROP_INTERPOLATION_TYPE:
      base_config->interpolation_type = g_value_get_enum (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_base_config_get_property (GObject    *object,
                               guint       property_id,
                               GValue     *value,
                               GParamSpec *pspec)
{
  GimpBaseConfig *base_config;

  base_config = GIMP_BASE_CONFIG (object);

  switch (property_id)
    {
    case PROP_TEMP_PATH:
      g_value_set_string (value, base_config->temp_path);
      break;
    case PROP_SWAP_PATH:
      g_value_set_string (value, base_config->swap_path);
      break;
    case PROP_STINGY_MEMORY_USE:
      g_value_set_boolean (value, base_config->stingy_memory_use);
      break;
    case PROP_NUM_PROCESSORS:
      g_value_set_uint (value, base_config->num_processors);
      break;
    case PROP_TILE_CACHE_SIZE:
      g_value_set_uint (value, base_config->tile_cache_size);
      break;
    case PROP_INTERPOLATION_TYPE:
      g_value_set_enum (value, base_config->interpolation_type);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}
