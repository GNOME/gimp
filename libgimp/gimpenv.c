/* LIBGIMP - The GIMP Library
 *
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 * Copyright (C) 1999 Tor Lillqvist
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
#include <string.h>
#include "gimpenv.h"

#ifdef G_OS_WIN32
#define STRICT
#include <windows.h>		/* For GetModuleFileName */
#endif

#ifdef __EMX__
extern const char *__XOS2RedirRoot(const char *);
#endif

char *
gimp_directory ()
{
  static char *gimp_dir = NULL;
  char *env_gimp_dir;
  char *home_dir;

  if (gimp_dir != NULL)
    return gimp_dir;

  env_gimp_dir = g_getenv ("GIMP_DIRECTORY");
  home_dir = g_get_home_dir ();

  if (NULL != env_gimp_dir)
    {
      if (g_path_is_absolute (env_gimp_dir))
	gimp_dir = g_strdup(env_gimp_dir);
      else
	{
	  if (NULL != home_dir)
	    {
	      gimp_dir = g_strconcat (home_dir,
				      G_DIR_SEPARATOR_S,
				      env_gimp_dir,
				      NULL);
	    }
	  else
	    {
	      gimp_dir = g_strconcat (gimp_data_directory (),
				      G_DIR_SEPARATOR_S,
				      env_gimp_dir,
				      NULL);
	    }
	}
    }
  else
    {
#ifdef __EMX__       
	gimp_dir = g_strdup(__XOS2RedirRoot(GIMPDIR));
	return gimp_dir;  
#endif      
	if (NULL != home_dir)
	{
	  gimp_dir = g_strconcat (home_dir, G_DIR_SEPARATOR_S,
				  GIMPDIR, NULL);
	}
      else
	{
#ifndef G_OS_WIN32
	  g_message ("warning: no home directory.");
#endif
	  gimp_dir = g_strconcat (gimp_data_directory (),
				  G_DIR_SEPARATOR_S,
				  GIMPDIR,
				  ".",
				  g_get_user_name (),
				  NULL);
	}
    }

  return gimp_dir;
}

char *
gimp_personal_rc_file (char *basename)
{
  return g_strconcat (gimp_directory (),
		      G_DIR_SEPARATOR_S,
		      basename,
		      NULL);
}

char *
gimp_data_directory ()
{
  static char *gimp_data_dir = NULL;
  char *env_gimp_data_dir = NULL;
  
  if (gimp_data_dir != NULL)
    return gimp_data_dir;

  env_gimp_data_dir = g_getenv ("GIMP_DATADIR");

  if (NULL != env_gimp_data_dir)
    {
      if (!g_path_is_absolute (env_gimp_data_dir))
	g_error ("GIMP_DATADIR environment variable should be an absolute path.");
#ifndef __EMX__
      gimp_data_dir = g_strdup (env_gimp_data_dir);
#else      
      gimp_data_dir = g_strdup (__XOS2RedirRoot(env_gimp_data_dir));
#endif      
    }
  else
    {
#ifndef G_OS_WIN32
#ifndef __EMX__
      gimp_data_dir = DATADIR;
#else
      gimp_data_dir = g_strdup(__XOS2RedirRoot(DATADIR));
#endif
#else
      /* Figure it out from the executable name */
      char filename[MAX_PATH];
      char *sep1, *sep2;

      if (GetModuleFileName (NULL, filename, sizeof (filename)) == 0)
	g_error ("GetModuleFilename failed\n");
      
      /* If the executable file name is of the format
       * <foobar>\bin\gimp.exe of <foobar>\plug-ins\filter.exe, * use
       * <foobar>. Otherwise, use the directory where the executable
       * is.
       */

      sep1 = strrchr (filename, G_DIR_SEPARATOR);

      *sep1 = '\0';

      sep2 = strrchr (filename, G_DIR_SEPARATOR);

      if (sep2 != NULL)
	{
	  if (g_strcasecmp (sep2 + 1, "bin") == 0
	      || g_strcasecmp (sep2 + 1, "plug-ins") == 0)
	    *sep2 = '\0';
	}

      gimp_data_dir = g_strdup (filename);
#endif
    }
  return gimp_data_dir;
}

/* gimp_gtkrc returns the name of the GIMP's application-specific
 * gtkrc file.
 *
 * The returned string is allocated just once, and should *NOT* be
 * freed with g_free().
 */ 
char*
gimp_gtkrc ()
{
  static char *gimp_gtkrc_filename = NULL;

  if (gimp_gtkrc_filename != NULL)
    return gimp_gtkrc_filename;
  

  gimp_gtkrc_filename = g_strconcat (gimp_directory (),
				     G_DIR_SEPARATOR_S,
				     "gtkrc",
				     NULL);
  return gimp_gtkrc_filename;
}
