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
#include <process.h>		/* For _getpid() */
#include <io.h> /* for _unlink() */
#endif
 
#include "base-types.h"

#include "base.h"
#include "base-config.h"
#include "detect-mmx.h"
#include "temp-buf.h"
#include "tile-swap.h"

#include "paint-funcs/paint-funcs.h"


static void   toast_old_temp_files (void);


/*  public functions  */

void
base_init (void)
{
  gchar *swapfile;
  gchar *path;

#ifdef HAVE_ASM_MMX
  use_mmx = (intel_cpu_features() & (1 << 23)) ? 1 : 0;
  g_print ("using MMX: %s\n", use_mmx ? "yes" : "no");
#endif  

  toast_old_temp_files ();

  /* Add the swap file  */
  if (base_config->swap_path == NULL)
    base_config->swap_path = g_strdup (g_get_tmp_dir ());

  swapfile = g_strdup_printf ("gimpswap.%lu", (unsigned long) getpid ());

  path = g_build_filename (base_config->swap_path, swapfile, NULL);

  g_free (swapfile);

  tile_swap_add (path, NULL, NULL);

  g_free (path);

  paint_funcs_setup ();
}

void
base_exit (void)
{
  swapping_free ();
  paint_funcs_free ();
  tile_swap_exit ();
}


/*  private functions  */

static void
toast_old_temp_files (void)
{
  GDir *dir = NULL;
  const char *entry;

  if (base_config->swap_path)
    dir = g_dir_open (base_config->swap_path, 0, NULL);

  if (!dir)
    return;

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

	    filename = g_build_filename (base_config->swap_path, entry,
                                         NULL);

	    unlink (filename);

            g_free (filename);
	  }
      }

  g_dir_close (dir);
}
