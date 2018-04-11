/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimp-gegl.c
 * Copyright (C) 2007 Øyvind Kolås <pippin@gimp.org>
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
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <gio/gio.h>
#include <gegl.h>

#include "gimp-gegl-types.h"

#include "config/gimpgeglconfig.h"

#include "operations/gimp-operations.h"

#include "core/gimp.h"
#include "core/gimp-parallel.h"

#include "gimp-babl.h"
#include "gimp-gegl.h"


static void  gimp_gegl_notify_tile_cache_size (GimpGeglConfig *config);
static void  gimp_gegl_notify_num_processors  (GimpGeglConfig *config);
static void  gimp_gegl_notify_use_opencl      (GimpGeglConfig *config);

#include <operation/gegl-operation.h>

void
gimp_gegl_init (Gimp *gimp)
{
  GimpGeglConfig *config;

  g_return_if_fail (GIMP_IS_GIMP (gimp));

  config = GIMP_GEGL_CONFIG (gimp->config);

  g_object_set (gegl_config (),
                "tile-cache-size", (guint64) config->tile_cache_size,
                "threads",         config->num_processors,
                "use-opencl",      config->use_opencl,
                NULL);

  g_signal_connect (config, "notify::tile-cache-size",
                    G_CALLBACK (gimp_gegl_notify_tile_cache_size),
                    NULL);
  g_signal_connect (config, "notify::num-processors",
                    G_CALLBACK (gimp_gegl_notify_num_processors),
                    NULL);
  g_signal_connect (config, "notify::use-opencl",
                    G_CALLBACK (gimp_gegl_notify_use_opencl),
                    NULL);

  gimp_parallel_init (gimp);

  gimp_babl_init ();

  gimp_operations_init (gimp);
}

void
gimp_gegl_exit (Gimp *gimp)
{
  g_return_if_fail (GIMP_IS_GIMP (gimp));

  gimp_parallel_exit (gimp);
}

static void
gimp_gegl_notify_tile_cache_size (GimpGeglConfig *config)
{
  g_object_set (gegl_config (),
                "tile-cache-size", (guint64) config->tile_cache_size,
                NULL);
}

static void
gimp_gegl_notify_num_processors (GimpGeglConfig *config)
{
  g_object_set (gegl_config (),
                "threads", config->num_processors,
                NULL);
}

static void
gimp_gegl_notify_use_opencl (GimpGeglConfig *config)
{
  g_object_set (gegl_config (),
                "use-opencl", config->use_opencl,
                NULL);
}
