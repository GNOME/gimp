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

#ifdef HAVE_DIRENT_H
#include <dirent.h>
#endif
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#include <sys/types.h>

#include <glib.h>

#ifdef G_OS_WIN32
#include <process.h>		/* For _getpid() */
#include <io.h> /* for _unlink() */
#endif
 
#include "base-types.h"

#include "base.h"
#include "base-config.h"
#include "temp-buf.h"
#include "tile-swap.h"

#include "paint-funcs/paint-funcs.h"


static void   toast_old_temp_files (void);


/*  public functions  */

void
base_init (void)
{
  gchar *path;

  toast_old_temp_files ();

  /* Add the swap file  */
  if (base_config->swap_path == NULL)
    base_config->swap_path = g_get_tmp_dir ();

  path = g_strdup_printf ("%s" G_DIR_SEPARATOR_S "gimpswap.%lu",
			  base_config->swap_path,
			  (unsigned long) getpid ());
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
  DIR           *dir;
  struct dirent *entry;
  GString       *filename = g_string_new ("");

  dir = opendir (base_config->swap_path);

  if (!dir)
    return;

  while ((entry = readdir (dir)) != NULL)
    if (!strncmp (entry->d_name, "gimpswap.", 9))
      {
        /* don't try to kill swap files of running processes
         * yes, I know they might not all be gimp processes, and when you
         * unlink, it's refcounted, but lets not confuse the user by
         * "where did my disk space go?" cause the filename is gone
         * if the kill succeeds, and there running process isn't gimp
         * we'll probably get it the next time around
         */

	gint pid = atoi (entry->d_name + 9);
#ifndef G_OS_WIN32
	if (kill (pid, 0))
#endif
	  {
	    /*  On Windows, you can't remove open files anyhow,
	     *  so no harm trying.
	     */
	    g_string_sprintf (filename, "%s" G_DIR_SEPARATOR_S "%s",
			      base_config->swap_path, entry->d_name);
	    unlink (filename->str);
	  }
      }

  closedir (dir);
  
  g_string_free (filename, TRUE);
}
