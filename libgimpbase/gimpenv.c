/* LIBGIMP - The GIMP Library
 *
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 * Copyright (C) 1999 Tor Lillqvist
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include "config.h"

#include <glib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include "gimpenv.h"

#ifdef G_OS_WIN32
#define STRICT
#include <windows.h>		/* For GetModuleFileName */
#include <io.h>
#ifndef S_IWUSR
# define S_IWUSR _S_IWRITE
#endif
#ifndef S_IWGRP
#define S_IWGRP (_S_IWRITE>>3)
#define S_IWOTH (_S_IWRITE>>6)
#endif
#ifndef S_ISDIR
# define __S_ISTYPE(mode, mask)	(((mode) & _S_IFMT) == (mask))
# define S_ISDIR(mode)	__S_ISTYPE((mode), _S_IFDIR)
#endif
#define uid_t gint
#define gid_t gint
#define geteuid() 0
#define getegid() 0
#endif

#ifdef __EMX__
extern const char *__XOS2RedirRoot(const char *);
#endif

/**
 * gimp_directory:
 *
 * Returns the user-specific GIMP settings directory. If the environment 
 * variable GIMP_DIRECTORY exists, it is used. If it is an absolute path, 
 * it is used as is.  If it is a relative path, it is taken to be a 
 * subdirectory of the home directory. If it is relative path, and no home 
 * directory can be determined, it is taken to be a subdirectory of
 * gimp_data_directory().
 *
 * The usual case is that no GIMP_DIRECTORY environment variable exists, 
 * and then we use the GIMPDIR subdirectory of the home directory. If no 
 * home directory exists, we use a per-user subdirectory of
 * gimp_data_directory().
 * In any case, we always return some non-empty string, whether it
 * corresponds to an existing directory or not.
 *
 * The returned string is allocated just once, and should *NOT* be
 * freed with g_free().
 *
 * Returns: The user-specific GIMP settings directory.
 */
gchar*
gimp_directory (void)
{
  static gchar *gimp_dir = NULL;
  gchar *env_gimp_dir;
  gchar *home_dir;

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

/**
 * gimp_personal_rc_file:
 * @basename: The basename of a rc_file.
 *
 * Returns the name of a file in the user-specific GIMP settings directory.
 *
 * The returned string is allocated dynamically and *SHOULD* be freed
 * with g_free() after use.
 *
 * Returns: The name of a file in the user-specific GIMP settings directory.
 */
gchar*
gimp_personal_rc_file (gchar *basename)
{
  return g_strconcat (gimp_directory (),
		      G_DIR_SEPARATOR_S,
		      basename,
		      NULL);
}

/**
 * gimp_data_directory:
 *
 * Returns the top directory for GIMP data. If the environment variable 
 * GIMP_DATADIR exists, that is used.  It should be an absolute pathname.
 * Otherwise, on Unix the compile-time defined directory is used.  On
 * Win32, the installation directory as deduced from the executable's
 * name is used.
 *
 * The returned string is allocated just once, and should *NOT* be
 * freed with g_free().
 *
 * Returns: The top directory for GIMP data.
 */
gchar*
gimp_data_directory (void)
{
  static gchar *gimp_data_dir = NULL;
  gchar *env_gimp_data_dir = NULL;
  
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
      gchar filename[MAX_PATH];
      gchar *sep1, *sep2;

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

/**
 * gimp_gtkrc:
 *
 * Returns the name of the GIMP's application-specific gtkrc file.
 *
 * The returned string is allocated just once, and should *NOT* be
 * freed with g_free().
 *
 * Returns: The name of the GIMP's application-specific gtkrc file.
 */ 
gchar*
gimp_gtkrc (void)
{
  static gchar *gimp_gtkrc_filename = NULL;

  if (gimp_gtkrc_filename != NULL)
    return gimp_gtkrc_filename;
  

  gimp_gtkrc_filename = g_strconcat (gimp_directory (),
				     G_DIR_SEPARATOR_S,
				     "gtkrc",
				     NULL);
  return gimp_gtkrc_filename;
}

/**
 * gimp_path_parse:
 * @path: A list of directories separated by #G_SEARCHPATH_SEPARATOR.
 * @max_paths: The maximum number of directories to return.
 * @check: #TRUE if you want the directories to be checked.
 * @check_failed: Returns a #GList of path elements for which the
 *                check failed. Each list element is guaranteed
 *		  to end with a #G_PATH_SEPARATOR.
 *
 * Returns: A #GList of all directories in @path. Each list element
 *	    is guaranteed to end with a #G_PATH_SEPARATOR.
 */
GList *
gimp_path_parse (gchar     *path,
		 gint       max_paths,
		 gboolean   check,
		 GList    **check_failed)
{
  gchar  *home;
  gchar **patharray;
  GList  *list = NULL;
  GList  *fail_list = NULL;
  gint    i;

  struct stat filestat;
  gint        err = FALSE;

  if (!path || !*path || max_paths < 1 || max_paths > 256)
    return NULL;

  home = g_get_home_dir ();

  patharray = g_strsplit (path, G_SEARCHPATH_SEPARATOR_S, max_paths);

  for (i = 0; i < max_paths; i++)
    {
      GString *dir;

      if (!patharray[i])
	break;

      if (*patharray[i] == '~')
	{
	  dir = g_string_new (home);
	  g_string_append (dir, patharray[i] + 1);
	}
      else
	{
	  dir = g_string_new (patharray[i]);
	}

#ifdef __EMX__
      _fnslashify (dir);
#endif

      if (check)
	{
	  /*  check if directory exists  */
	  err = stat (dir->str, &filestat);

	  if (!err && S_ISDIR (filestat.st_mode))
	    {
	      if (dir->str[dir->len - 1] != G_DIR_SEPARATOR)
		g_string_append_c (dir, G_DIR_SEPARATOR);
	    }
	}

      if (!err)
	list = g_list_prepend (list, g_strdup (dir->str));
      else if (check_failed)
	fail_list = g_list_prepend (fail_list, g_strdup (dir->str));

      g_string_free (dir, TRUE);
    }

  g_strfreev (patharray);

  list = g_list_reverse (list);

  if (check && check_failed)
    {
      fail_list = g_list_reverse (fail_list);
      *check_failed = fail_list;
    }

  return list;
}

/**
 * gimp_path_to_str:
 * @path: A list of directories as returned by gimp_path_parse().
 *
 * Returns: A searchpath string separated by #G_SEARCHPATH_SEPARATOR.
 *
 */
gchar *
gimp_path_to_str (GList *path)
{
  GString *str = NULL;
  GList   *list;
  gchar   *retval = NULL;

  for (list = path; list; list = g_list_next (list))
    {
      if (str)
	{
	  g_string_append_c (str, G_SEARCHPATH_SEPARATOR);
	  g_string_append (str, (gchar *) list->data);
	}
      else
	{
	  str = g_string_new ((gchar *) list->data);
	}
    }

  if (str)
    {
      retval = str->str;
      g_string_free (str, FALSE);
    }

  return retval;
}

/**
 * gimp_path_free:
 * @path: A list of directories as returned by gimp_path_parse().
 *
 * This function frees the memory allocated for the list and it's strings.
 *
 */
void
gimp_path_free (GList *path)
{
  GList *list;

  if (path)
    {
      for (list = path; list; list = g_list_next (list))
	{
	  g_free (list->data);
	}

      g_list_free (path);
    }
}

/**
 * gimp_path_get_user_writable_dir:
 * @path: A list of directories as returned by gimp_path_parse().
 *
 * Note that you have to g_free() the returned string.
 *
 * Returns: The first directory in @path where the user has write permission.
 *
 */
gchar *
gimp_path_get_user_writable_dir (GList *path)
{
  GList *list;

  uid_t euid;
  gid_t egid;

  struct stat filestat;
  gint        err;

  euid = geteuid ();
  egid = getegid ();

  for (list = path; list; list = g_list_next (list))
    {
      /*  check if directory exists  */

      /* ugly hack to handle paths with an extra G_DIR_SEPARATOR
       * attached. The stat() in MSVCRT doesn't like that.
       */
      gchar *dir = g_strdup ((gchar *) list->data);
      gchar *p = dir;
      gint pl;

      if (g_path_is_absolute)
	p = g_path_skip_root (dir);
      pl = strlen (p);
      if (pl > 0 && p[pl-1] == G_DIR_SEPARATOR)
	p[pl-1] = '\0';
      err = stat (dir, &filestat);
      g_free (dir);

      /*  this is tricky:
       *  if a file is e.g. owned by the current user but not user-writable,
       *  the user has no permission to write to the file regardless
       *  of his group's or other's write permissions
       */
      if (!err && S_ISDIR (filestat.st_mode) &&

	  ((filestat.st_mode & S_IWUSR) ||

	   ((filestat.st_mode & S_IWGRP) &&
	    (euid != filestat.st_uid)) ||

	   ((filestat.st_mode & S_IWOTH) &&
	    (euid != filestat.st_uid) &&
	    (egid != filestat.st_gid))))
	{
	  return g_strdup ((gchar *) list->data);
	}
    }

  return NULL;
}
