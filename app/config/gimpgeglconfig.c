/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * GimpGeglConfig class
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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <gio/gio.h>
#include <gegl.h>

#include "libgimpbase/gimpbase.h"
#include "libgimpconfig/gimpconfig.h"

#include "core/core-types.h" /* eek */

#include "gimprc-blurbs.h"
#include "gimpgeglconfig.h"

#include "core/gimp-utils.h"

#include "gimp-debug.h"

#include "gimp-intl.h"


#define GIMP_DEFAULT_SWAP_COMPRESSION "fast"

#define GIMP_MAX_MEM_PROCESS          (MIN (G_MAXSIZE, GIMP_MAX_MEMSIZE))


enum
{
  PROP_0,
  PROP_TEMP_PATH,
  PROP_SWAP_PATH,
  PROP_SWAP_COMPRESSION,
  PROP_NUM_PROCESSORS,
  PROP_TILE_CACHE_SIZE,
  PROP_USE_OPENCL,

  /* ignored, only for backward compatibility: */
  PROP_STINGY_MEMORY_USE
};


static void   gimp_gegl_config_constructed  (GObject             *object);
static void   gimp_gegl_config_finalize     (GObject             *object);
static void   gimp_gegl_config_set_property (GObject             *object,
                                             guint                property_id,
                                             const GValue        *value,
                                             GParamSpec          *pspec);
static void   gimp_gegl_config_get_property (GObject             *object,
                                             guint                property_id,
                                             GValue              *value,
                                             GParamSpec          *pspec);


G_DEFINE_TYPE (GimpGeglConfig, gimp_gegl_config, G_TYPE_OBJECT)

#define parent_class gimp_gegl_config_parent_class


static void
gimp_gegl_config_class_init (GimpGeglConfigClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  gint          n_threads;
  gint          max_n_threads;
  guint64       memory_size;

  parent_class = g_type_class_peek_parent (klass);

  object_class->constructed  = gimp_gegl_config_constructed;
  object_class->finalize     = gimp_gegl_config_finalize;
  object_class->set_property = gimp_gegl_config_set_property;
  object_class->get_property = gimp_gegl_config_get_property;

  GIMP_CONFIG_PROP_PATH (object_class, PROP_TEMP_PATH,
                         "temp-path",
                         "Temp path",
                         TEMP_PATH_BLURB,
                         GIMP_CONFIG_PATH_DIR,
                         "${gimp_temp_dir}",
                         GIMP_PARAM_STATIC_STRINGS |
                         GIMP_CONFIG_PARAM_RESTART);

  GIMP_CONFIG_PROP_PATH (object_class, PROP_SWAP_PATH,
                         "swap-path",
                         "Swap path",
                         SWAP_PATH_BLURB,
                         GIMP_CONFIG_PATH_DIR,
                         "${gimp_cache_dir}",
                         GIMP_PARAM_STATIC_STRINGS |
                         GIMP_CONFIG_PARAM_RESTART);

  GIMP_CONFIG_PROP_STRING (object_class, PROP_SWAP_COMPRESSION,
                           "swap-compression",
                           "Swap compression",
                            SWAP_COMPRESSION_BLURB,
                           GIMP_DEFAULT_SWAP_COMPRESSION,
                           GIMP_PARAM_STATIC_STRINGS);

  n_threads = g_get_num_processors ();

  max_n_threads =
    G_PARAM_SPEC_INT (g_object_class_find_property (G_OBJECT_GET_CLASS (gegl_config ()),
                                                    "threads"))->maximum;

  n_threads = MIN (n_threads, max_n_threads);

  GIMP_CONFIG_PROP_INT (object_class, PROP_NUM_PROCESSORS,
                        "num-processors",
                        "Number of threads to use",
                        NUM_PROCESSORS_BLURB,
                        1, max_n_threads, n_threads,
                        GIMP_PARAM_STATIC_STRINGS);

  memory_size = gimp_get_physical_memory_size ();

  /* limit to the amount one process can handle */
  memory_size = MIN (GIMP_MAX_MEM_PROCESS, memory_size);

  if (memory_size > 0)
    memory_size = memory_size / 2; /* half the memory */
  else
    memory_size = 1 << 30; /* 1GB */

  GIMP_CONFIG_PROP_MEMSIZE (object_class, PROP_TILE_CACHE_SIZE,
                            "tile-cache-size",
                            "Tile cache size",
                            TILE_CACHE_SIZE_BLURB,
                            0, GIMP_MAX_MEM_PROCESS,
                            memory_size,
                            GIMP_PARAM_STATIC_STRINGS |
                            GIMP_CONFIG_PARAM_CONFIRM);

  GIMP_CONFIG_PROP_BOOLEAN (object_class, PROP_USE_OPENCL,
                            "use-opencl",
                            "Use OpenCL",
                            USE_OPENCL_BLURB,
                            FALSE,
                            GIMP_PARAM_STATIC_STRINGS);

  /*  only for backward compatibility:  */
  GIMP_CONFIG_PROP_BOOLEAN (object_class, PROP_STINGY_MEMORY_USE,
                            "stingy-memory-use",
                            NULL, NULL,
                            FALSE,
                            GIMP_CONFIG_PARAM_IGNORE);
}

static void
gimp_gegl_config_init (GimpGeglConfig *config)
{
}

static void
gimp_gegl_config_constructed (GObject *object)
{
  G_OBJECT_CLASS (parent_class)->constructed (object);

  gimp_debug_add_instance (object, G_OBJECT_GET_CLASS (object));
}

static void
gimp_gegl_config_finalize (GObject *object)
{
  GimpGeglConfig *gegl_config = GIMP_GEGL_CONFIG (object);

  g_free (gegl_config->temp_path);
  g_free (gegl_config->swap_path);
  g_free (gegl_config->swap_compression);

  gimp_debug_remove_instance (object);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
gimp_gegl_config_set_property (GObject      *object,
                               guint         property_id,
                               const GValue *value,
                               GParamSpec   *pspec)
{
  GimpGeglConfig *gegl_config = GIMP_GEGL_CONFIG (object);

  switch (property_id)
    {
    case PROP_TEMP_PATH:
      g_free (gegl_config->temp_path);
      gegl_config->temp_path = g_value_dup_string (value);
      break;
    case PROP_SWAP_PATH:
      g_free (gegl_config->swap_path);
      gegl_config->swap_path = g_value_dup_string (value);
      break;
    case PROP_SWAP_COMPRESSION:
      g_free (gegl_config->swap_compression);
      gegl_config->swap_compression = g_value_dup_string (value);
      break;
    case PROP_NUM_PROCESSORS:
      gegl_config->num_processors = g_value_get_int (value);
      break;
    case PROP_TILE_CACHE_SIZE:
      gegl_config->tile_cache_size = g_value_get_uint64 (value);
      break;
    case PROP_USE_OPENCL:
      gegl_config->use_opencl = g_value_get_boolean (value);
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
gimp_gegl_config_get_property (GObject    *object,
                               guint       property_id,
                               GValue     *value,
                               GParamSpec *pspec)
{
  GimpGeglConfig *gegl_config = GIMP_GEGL_CONFIG (object);

  switch (property_id)
    {
    case PROP_TEMP_PATH:
      g_value_set_string (value, gegl_config->temp_path);
      break;
    case PROP_SWAP_PATH:
      g_value_set_string (value, gegl_config->swap_path);
      break;
    case PROP_SWAP_COMPRESSION:
      g_value_set_string (value, gegl_config->swap_compression);
      break;
    case PROP_NUM_PROCESSORS:
      g_value_set_int (value, gegl_config->num_processors);
      break;
    case PROP_TILE_CACHE_SIZE:
      g_value_set_uint64 (value, gegl_config->tile_cache_size);
      break;
    case PROP_USE_OPENCL:
      g_value_set_boolean (value, gegl_config->use_opencl);
      break;

    case PROP_STINGY_MEMORY_USE:
      /* ignored */
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}
