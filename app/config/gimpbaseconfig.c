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

#include "libgimpbase/gimpbase.h"
#include "libgimpconfig/gimpconfig.h"

#include "config-types.h"

#include "base/pixel-processor.h"

#include "gimprc-blurbs.h"
#include "gimpbaseconfig.h"

#include "gimp-intl.h"


enum
{
  PROP_0,
  PROP_TEMP_PATH,
  PROP_SWAP_PATH,
  PROP_NUM_PROCESSORS,
  PROP_TILE_CACHE_SIZE,

  /* ignored, only for backward compatibility: */
  PROP_STINGY_MEMORY_USE
};


static void   gimp_base_config_finalize     (GObject      *object);
static void   gimp_base_config_set_property (GObject      *object,
                                             guint         property_id,
                                             const GValue *value,
                                             GParamSpec   *pspec);
static void   gimp_base_config_get_property (GObject      *object,
                                             guint         property_id,
                                             GValue       *value,
                                             GParamSpec   *pspec);


G_DEFINE_TYPE (GimpBaseConfig, gimp_base_config, G_TYPE_OBJECT)

#define parent_class gimp_base_config_parent_class


static void
gimp_base_config_class_init (GimpBaseConfigClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->finalize     = gimp_base_config_finalize;
  object_class->set_property = gimp_base_config_set_property;
  object_class->get_property = gimp_base_config_get_property;

  GIMP_CONFIG_INSTALL_PROP_PATH (object_class, PROP_TEMP_PATH,
                                 "temp-path", TEMP_PATH_BLURB,
                                 GIMP_CONFIG_PATH_DIR,
                                 "${gimp_dir}" G_DIR_SEPARATOR_S "tmp",
                                 GIMP_PARAM_STATIC_STRINGS |
                                 GIMP_CONFIG_PARAM_RESTART);
  GIMP_CONFIG_INSTALL_PROP_PATH (object_class, PROP_SWAP_PATH,
                                 "swap-path", SWAP_PATH_BLURB,
                                 GIMP_CONFIG_PATH_DIR,
                                 "${gimp_dir}",
                                 GIMP_PARAM_STATIC_STRINGS |
                                 GIMP_CONFIG_PARAM_RESTART);
  GIMP_CONFIG_INSTALL_PROP_UINT (object_class, PROP_NUM_PROCESSORS,
                                 "num-processors", NUM_PROCESSORS_BLURB,
                                 1, GIMP_MAX_NUM_THREADS, 2,
                                 GIMP_PARAM_STATIC_STRINGS);
  GIMP_CONFIG_INSTALL_PROP_MEMSIZE (object_class, PROP_TILE_CACHE_SIZE,
                                    "tile-cache-size", TILE_CACHE_SIZE_BLURB,
                                    0, MIN (G_MAXULONG, GIMP_MAX_MEMSIZE),
                                    1 << 28, /* 256MB */
                                    GIMP_PARAM_STATIC_STRINGS |
                                    GIMP_CONFIG_PARAM_CONFIRM);

  /*  only for backward compatibility:  */
  GIMP_CONFIG_INSTALL_PROP_BOOLEAN (object_class, PROP_STINGY_MEMORY_USE,
                                    "stingy-memory-use", NULL,
                                    FALSE,
                                    GIMP_CONFIG_PARAM_IGNORE);
}

static void
gimp_base_config_init (GimpBaseConfig *config)
{
}

static void
gimp_base_config_finalize (GObject *object)
{
  GimpBaseConfig *base_config = GIMP_BASE_CONFIG (object);

  g_free (base_config->temp_path);
  g_free (base_config->swap_path);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
gimp_base_config_set_property (GObject      *object,
                               guint         property_id,
                               const GValue *value,
                               GParamSpec   *pspec)
{
  GimpBaseConfig *base_config = GIMP_BASE_CONFIG (object);

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
    case PROP_NUM_PROCESSORS:
      base_config->num_processors = g_value_get_uint (value);
      break;
    case PROP_TILE_CACHE_SIZE:
      base_config->tile_cache_size = g_value_get_uint64 (value);
      break;

    case PROP_STINGY_MEMORY_USE:
      /* ignored */
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
  GimpBaseConfig *base_config = GIMP_BASE_CONFIG (object);

  switch (property_id)
    {
    case PROP_TEMP_PATH:
      g_value_set_string (value, base_config->temp_path);
      break;
    case PROP_SWAP_PATH:
      g_value_set_string (value, base_config->swap_path);
      break;
    case PROP_NUM_PROCESSORS:
      g_value_set_uint (value, base_config->num_processors);
      break;
    case PROP_TILE_CACHE_SIZE:
      g_value_set_uint64 (value, base_config->tile_cache_size);
      break;

    case PROP_STINGY_MEMORY_USE:
      /* ignored */
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}
