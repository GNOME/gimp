/* The GIMP -- an image manipulation program
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

#ifdef G_OS_WIN32
#include <process.h>	/*  for _getpid()  */
#include <io.h>         /*  for _unlink()  */
#endif

#include "base-types.h"

#include "config/gimpbaseconfig.h"
#include "config/gimpconfig-path.h"

#include "paint-funcs/paint-funcs.h"
#include "composite/gimp-composite.h"

#include "base.h"
#include "temp-buf.h"
#include "tile-cache.h"
#include "tile-swap.h"


GimpBaseConfig *base_config = NULL;


static void   base_toast_old_temp_files   (GimpBaseConfig *config);
static void   base_tile_cache_size_notify (GObject        *config,
                                           GParamSpec     *param_spec,
                                           gpointer        data);


/*  public functions  */

gboolean
base_init (GimpBaseConfig *config,
           gboolean        be_verbose,
           gboolean        use_cpu_accel)
{
  gchar    *swapfile;
  gchar    *swapdir;
  gchar    *path;
  gboolean  swap_is_ok;

  g_return_val_if_fail (GIMP_IS_BASE_CONFIG (config), FALSE);
  g_return_val_if_fail (base_config == NULL, FALSE);

  base_config = g_object_ref (config);

  tile_cache_init (config->tile_cache_size);

  g_signal_connect (config, "notify::tile-cache-size",
                    G_CALLBACK (base_tile_cache_size_notify),
                    NULL);

  base_toast_old_temp_files (config);

  /* Add the swap file */
  if (! config->swap_path)
    g_object_set (config, "swap_path", "${gimp_dir}", NULL);

  swapdir  = gimp_config_path_expand (config->swap_path, TRUE, NULL);
  swapfile = g_strdup_printf ("gimpswap.%lu", (unsigned long) getpid ());

  path = g_build_filename (swapdir, swapfile, NULL);

  g_free (swapfile);
  g_free (swapdir);

  tile_swap_add (path, NULL, NULL);

  g_free (path);

  swap_is_ok = tile_swap_test ();

  gimp_composite_init (be_verbose, use_cpu_accel);

  paint_funcs_setup ();

  return swap_is_ok;
}

void
base_exit (void)
{
  g_return_if_fail (base_config != NULL);

  swapping_free ();
  paint_funcs_free ();
  tile_swap_exit ();
  tile_cache_exit ();

  g_signal_handlers_disconnect_by_func (base_config,
                                        base_tile_cache_size_notify,
                                        NULL);

  g_object_unref (base_config);
  base_config = NULL;
}


/*  private functions  */

static void
base_toast_old_temp_files (GimpBaseConfig *config)
{
  GDir       *dir = NULL;
  gchar      *dirname;
  const char *entry;

  if (!config->swap_path)
    return;

  dirname = gimp_config_path_expand (config->swap_path, TRUE, NULL);
  if (!dirname)
    return;

  dir = g_dir_open (dirname, 0, NULL);

  if (!dir)
    {
      g_free (dirname);
      return;
    }

  while ((entry = g_dir_read_name (dir)) != NULL)
    if (! strncmp (entry, "gimpswap.", 9))
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
            gchar *filename;

	    filename = g_build_filename (dirname, entry, NULL);
	    unlink (filename);
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
