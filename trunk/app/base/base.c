/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
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

#include <signal.h>
#include <stdlib.h>
#include <string.h>

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include <sys/types.h>

#include <glib-object.h>
#include <glib/gstdio.h>

#ifdef G_OS_WIN32
#include <process.h>        /*  for _getpid()  */
#include "libgimpbase/gimpwin32-io.h"
#endif

#include "libgimpconfig/gimpconfig.h"

#include "base-types.h"

#include "config/gimpbaseconfig.h"

#include "paint-funcs/paint-funcs.h"
#include "composite/gimp-composite.h"

#include "base.h"
#include "pixel-processor.h"
#include "tile-cache.h"
#include "tile-swap.h"


static void   base_toast_old_swap_files   (const gchar *swap_path);

static void   base_tile_cache_size_notify (GObject     *config,
                                           GParamSpec  *param_spec,
                                           gpointer     data);
static void   base_num_processors_notify  (GObject     *config,
                                           GParamSpec  *param_spec,
                                           gpointer     data);


static GimpBaseConfig *base_config = NULL;


/*  public functions  */

gboolean
base_init (GimpBaseConfig *config,
           gboolean        be_verbose,
           gboolean        use_cpu_accel)
{
  gboolean  swap_is_ok;
  gchar    *temp_dir;

  g_return_val_if_fail (GIMP_IS_BASE_CONFIG (config), FALSE);
  g_return_val_if_fail (base_config == NULL, FALSE);

  base_config = g_object_ref (config);

  tile_cache_init (config->tile_cache_size);
  g_signal_connect (config, "notify::tile-cache-size",
                    G_CALLBACK (base_tile_cache_size_notify),
                    NULL);

  if (! config->swap_path || ! *config->swap_path)
    gimp_config_reset_property (G_OBJECT (config), "swap-path");

  base_toast_old_swap_files (config->swap_path);

  tile_swap_init (config->swap_path);

  swap_is_ok = tile_swap_test ();

  /*  create the temp directory if it doesn't exist  */
  if (! config->temp_path || ! *config->temp_path)
    gimp_config_reset_property (G_OBJECT (config), "temp-path");

  temp_dir = gimp_config_path_expand (config->temp_path, TRUE, NULL);

  if (! g_file_test (temp_dir, G_FILE_TEST_EXISTS))
    g_mkdir_with_parents (temp_dir,
                          S_IRUSR | S_IXUSR | S_IWUSR |
                          S_IRGRP | S_IXGRP |
                          S_IROTH | S_IXOTH);

  g_free (temp_dir);

  pixel_processor_init (config->num_processors);
  g_signal_connect (config, "notify::num-processors",
                    G_CALLBACK (base_num_processors_notify),
                    NULL);

  gimp_composite_init (be_verbose, use_cpu_accel);

  paint_funcs_setup ();

  return swap_is_ok;
}

void
base_exit (void)
{
  g_return_if_fail (base_config != NULL);

  pixel_processor_exit ();
  paint_funcs_free ();
  tile_cache_exit ();
  tile_swap_exit ();

  g_signal_handlers_disconnect_by_func (base_config,
                                        base_tile_cache_size_notify,
                                        NULL);

  g_object_unref (base_config);
  base_config = NULL;
}


/*  private functions  */

static void
base_toast_old_swap_files (const gchar *swap_path)
{
  GDir       *dir = NULL;
  gchar      *dirname;
  const char *entry;

  if (! swap_path)
    return;

  dirname = gimp_config_path_expand (swap_path, TRUE, NULL);
  if (!dirname)
    return;

  dir = g_dir_open (dirname, 0, NULL);

  if (!dir)
    {
      g_free (dirname);
      return;
    }

  while ((entry = g_dir_read_name (dir)) != NULL)
    if (g_str_has_prefix (entry, "gimpswap."))
      {
        /* don't try to kill swap files of running processes
         * yes, I know they might not all be gimp processes, and when you
         * unlink, it's refcounted, but lets not confuse the user by
         * "where did my disk space go?" cause the filename is gone
         * if the kill succeeds, and there running process isn't gimp
         * we'll probably get it the next time around
         */

        gint pid = atoi (entry + 9);

        /*  On Windows, you can't remove open files anyhow,
         *  so no harm trying.
         */
#ifndef G_OS_WIN32
        if (kill (pid, 0))
#endif
          {
            gchar *filename = g_build_filename (dirname, entry, NULL);

            g_unlink (filename);
            g_free (filename);
          }
      }

  g_dir_close (dir);

  g_free (dirname);
}

static void
base_tile_cache_size_notify (GObject    *config,
                             GParamSpec *param_spec,
                             gpointer    data)
{
  tile_cache_set_size (GIMP_BASE_CONFIG (config)->tile_cache_size);
}

static void
base_num_processors_notify (GObject    *config,
                            GParamSpec *param_spec,
                            gpointer    data)
{
  pixel_processor_set_num_threads (GIMP_BASE_CONFIG (config)->num_processors);
}
