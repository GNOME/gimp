/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * Datafiles module copyight (C) 1996 Federico Mena Quintero
 * federico@nuclecu.unam.mx
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

#include <glib.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#ifdef HAVE_DIRENT_H
#include <dirent.h>
#endif

#ifdef G_OS_WIN32
#ifndef S_ISDIR
#define S_ISDIR(m) (((m) & _S_IFMT) == _S_IFDIR)
#define S_ISREG(m) (((m) & _S_IFMT) == _S_IFREG)
#endif
#ifndef S_IXUSR
#define S_IXUSR _S_IEXEC
#endif
#endif /* G_OS_WIN32 */

#include "datafiles.h"
#include "gimprc.h"

#include "libgimp/gimpenv.h"

/***** Functions *****/

static gint        filestat_valid = 0;
static struct stat filestat;

#ifdef G_OS_WIN32
/*
 * On Windows there is no concept like the Unix executable flag. There
 * There is a weak emulation provided by the MS C Runtime using file
 * extensions (com, exe, cmd, bat). This needs to be extended to treat
 * scripts (Python, Perl, ...) as executables, too. We use the PATHEXT
 * variable, which is also used by cmd.exe.
 */
gboolean
is_script (const gchar *filename)
{
  const gchar *ext = strrchr (filename, '.');
  gchar *pathext;
  static gchar **exts = NULL;
  gint i;

  if (exts == NULL)
    {
      pathext = g_getenv ("PATHEXT");
      if (pathext != NULL)
	{
	  exts = g_strsplit (pathext, G_SEARCHPATH_SEPARATOR_S, 100);
	  g_free (pathext);
	}
      else
	{
	  exts = g_new (gchar *, 1);
	  exts[0] = NULL;
	}
    }

  i = 0;
  while (exts[i] != NULL)
    {
      if (g_strcasecmp (ext, exts[i]) == 0)
	return TRUE;
      i++;
    }

  return FALSE;
}
#else  /* !G_OS_WIN32 */
#define is_script(filename) FALSE
#endif

void
datafiles_read_directories (gchar                  *path_str,
			    GimpDataFileLoaderFunc  loader_func,
			    gint                    flags)
{
  gchar *local_path;
  GList *path;
  GList *list;
  gchar *filename;
  gint   err;
  DIR   *dir;
  struct dirent *dir_ent;

  if (path_str == NULL)
    return;

  local_path = g_strdup (path_str);

#ifdef __EMX__
  /*
   *  Change drive so opendir works.
   */
  if (local_path[1] == ':')
    {
      _chdrive (local_path[0]);
    }
#endif  

  path = gimp_path_parse (local_path, 16, TRUE, NULL);

  for (list = path; list; list = g_list_next (list))
    {
      /* Open directory */
      dir = opendir ((gchar *) list->data);

      if (!dir)
	{
	  g_message ("error reading datafiles directory \"%s\"",
		     (gchar *) list->data);
	}
      else
	{
	  while ((dir_ent = readdir(dir)))
	    {
	      filename = g_strdup_printf ("%s%s",
					  (gchar *) list->data,
					  dir_ent->d_name);

	      /* Check the file and see that it is not a sub-directory */
	      err = stat (filename, &filestat);

	      if (!err && S_ISREG (filestat.st_mode) &&
		  (!(flags & MODE_EXECUTABLE) ||
		   (filestat.st_mode & S_IXUSR) ||
		   is_script (filename)))
		{
		  filestat_valid = 1;
		  (*loader_func) (filename);
		  filestat_valid = 0;
		}

	      g_free (filename);
	    }

	  closedir (dir);
	}
    }

  gimp_path_free (path);
}

time_t
datafile_atime (void)
{
  if (filestat_valid)
    return filestat.st_atime;
  return 0;
}

time_t
datafile_mtime (void)
{
  if (filestat_valid)
    return filestat.st_mtime;
  return 0;
}

time_t
datafile_ctime (void)
{
  if (filestat_valid)
    return filestat.st_ctime;
  return 0;
}
