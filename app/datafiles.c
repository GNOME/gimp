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


#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/types.h>
#include <dirent.h>

#include <glib.h>
#include "datafiles.h"
#include "errors.h"
#include "general.h"
#include "gimprc.h"


/***** Functions *****/

/*****/

static int filestat_valid = 0;
static struct stat filestat;

void
datafiles_read_directories (char *path_str,
			    datafile_loader_t loader_func,
			    int flags)
{
  char          *home;
  char 	        *local_path;
  char          *path;
  char 	        *filename;
  char 	        *token;
  char          *next_token;
  int            err;
  DIR           *dir;
  struct dirent *dir_ent;

  if (path_str == NULL)
    return;

  /* Set local path to contain temp_path, where (supposedly)
   * there may be working files.
   */
  home = getenv("HOME");
  local_path = g_strdup (path_str);

  /* Search through all directories in the local path */

  next_token = local_path;

  token = xstrsep(&next_token, ":");

  while (token)
    {
      if (*token == '~')
	{
	  path = g_malloc(strlen(home) + strlen(token) + 2);
	  sprintf(path, "%s%s", home, token + 1);
	}
      else
	{
	  path = g_malloc(strlen(token) + 2);
	  strcpy(path, token);
	} /* else */

      /* Check if directory exists and if it has any items in it */
      err = stat(path, &filestat);

      if (!err && S_ISDIR(filestat.st_mode))
	{
	  if (path[strlen(path) - 1] != '/')
	    strcat(path, "/");

	  /* Open directory */
	  dir = opendir(path);

	  if (!dir)
	    g_message ("error reading datafiles directory \"%s\"", path);
	  else
	    {
	      while ((dir_ent = readdir(dir)))
		{
		  filename = g_malloc(strlen(path) + strlen(dir_ent->d_name) + 1);

		  sprintf(filename, "%s%s", path, dir_ent->d_name);

		  /* Check the file and see that it is not a sub-directory */
		  err = stat(filename, &filestat);

		  if (!err && S_ISREG(filestat.st_mode) &&
		      (!(flags & MODE_EXECUTABLE) || (filestat.st_mode & S_IXUSR)))
		    {
		      filestat_valid = 1;
		      (*loader_func) (filename);
		      filestat_valid = 0;
		    }

		  g_free(filename);
		} /* while */

	      closedir(dir);
	    } /* else */
	} /* if */

      g_free(path);

      token = xstrsep(&next_token, ":");
    } /* while */

  g_free(local_path);
} /* datafiles_read_directories */

time_t
datafile_atime ()
{
  if (filestat_valid)
    return filestat.st_atime;
  return 0;
}

time_t
datafile_mtime ()
{
  if (filestat_valid)
    return filestat.st_mtime;
  return 0;
}

time_t
datafile_ctime ()
{
  if (filestat_valid)
    return filestat.st_ctime;
  return 0;
}
