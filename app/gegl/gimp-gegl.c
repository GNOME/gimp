/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ligma-gegl.c
 * Copyright (C) 2007 Øyvind Kolås <pippin@ligma.org>
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

#include "libligmaconfig/ligmaconfig.h"

#include "ligma-gegl-types.h"

#include "config/ligmageglconfig.h"

#include "operations/ligma-operations.h"

#include "core/ligma.h"
#include "core/ligma-parallel.h"

#include "ligma-babl.h"
#include "ligma-gegl.h"

#include <operation/gegl-operation.h>


static void  ligma_gegl_notify_temp_path        (LigmaGeglConfig *config);
static void  ligma_gegl_notify_swap_path        (LigmaGeglConfig *config);
static void  ligma_gegl_notify_swap_compression (LigmaGeglConfig *config);
static void  ligma_gegl_notify_tile_cache_size  (LigmaGeglConfig *config);
static void  ligma_gegl_notify_num_processors   (LigmaGeglConfig *config);
static void  ligma_gegl_notify_use_opencl       (LigmaGeglConfig *config);


/*  public functions  */

void
ligma_gegl_init (Ligma *ligma)
{
  LigmaGeglConfig *config;

  g_return_if_fail (LIGMA_IS_LIGMA (ligma));

  config = LIGMA_GEGL_CONFIG (ligma->config);

  /* make sure temp and swap directories exist */
  ligma_gegl_notify_temp_path (config);
  ligma_gegl_notify_swap_path (config);

  g_object_set (gegl_config (),
                "swap-compression", config->swap_compression,
                "tile-cache-size",  (guint64) config->tile_cache_size,
                "threads",          config->num_processors,
                "use-opencl",       config->use_opencl,
                NULL);

  ligma_parallel_init (ligma);

  g_signal_connect (config, "notify::temp-path",
                    G_CALLBACK (ligma_gegl_notify_temp_path),
                    NULL);
  g_signal_connect (config, "notify::swap-path",
                    G_CALLBACK (ligma_gegl_notify_swap_path),
                    NULL);
  g_signal_connect (config, "notify::swap-compression",
                    G_CALLBACK (ligma_gegl_notify_swap_compression),
                    NULL);
  g_signal_connect (config, "notify::num-processors",
                    G_CALLBACK (ligma_gegl_notify_num_processors),
                    NULL);
  g_signal_connect (config, "notify::tile-cache-size",
                    G_CALLBACK (ligma_gegl_notify_tile_cache_size),
                    NULL);
  g_signal_connect (config, "notify::num-processors",
                    G_CALLBACK (ligma_gegl_notify_num_processors),
                    NULL);
  g_signal_connect (config, "notify::use-opencl",
                    G_CALLBACK (ligma_gegl_notify_use_opencl),
                    NULL);

  ligma_babl_init ();

  ligma_operations_init (ligma);
}

void
ligma_gegl_exit (Ligma *ligma)
{
  g_return_if_fail (LIGMA_IS_LIGMA (ligma));

  ligma_operations_exit (ligma);
  ligma_parallel_exit (ligma);
}


/*  private functions  */

static void
ligma_gegl_notify_temp_path (LigmaGeglConfig *config)
{
  GFile *file = ligma_file_new_for_config_path (config->temp_path, NULL);

  if (! g_file_query_exists (file, NULL))
    g_file_make_directory_with_parents (file, NULL, NULL);

  g_object_unref (file);
}

static void
ligma_gegl_notify_swap_path (LigmaGeglConfig *config)
{
  GFile *file = ligma_file_new_for_config_path (config->swap_path, NULL);
  gchar *path = g_file_get_path (file);

  if (! g_file_query_exists (file, NULL))
    g_file_make_directory_with_parents (file, NULL, NULL);

  g_object_set (gegl_config (),
                "swap", path,
                NULL);

  g_free (path);
  g_object_unref (file);
}

static void
ligma_gegl_notify_swap_compression (LigmaGeglConfig *config)
{
  g_object_set (gegl_config (),
                "swap-compression", config->swap_compression,
                NULL);
}

static void
ligma_gegl_notify_tile_cache_size (LigmaGeglConfig *config)
{
  g_object_set (gegl_config (),
                "tile-cache-size", (guint64) config->tile_cache_size,
                NULL);
}

static void
ligma_gegl_notify_num_processors (LigmaGeglConfig *config)
{
  g_object_set (gegl_config (),
                "threads", config->num_processors,
                NULL);
}

static void
ligma_gegl_notify_use_opencl (LigmaGeglConfig *config)
{
  g_object_set (gegl_config (),
                "use-opencl", config->use_opencl,
                NULL);
}
