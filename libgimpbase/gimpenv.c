/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-1997 Spencer Kimball and Peter Mattis
 *
 * gimpenv.c
 *
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
#define WIN32_LEAN_AND_MEAN	/* without it DATADIR in objidl.h will collide */
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

static gchar * gimp_env_get_dir (const gchar *gimp_env_name,
                                 const gchar *env_dir);

/**
 * gimp_directory:
 *
 * Returns the user-specific GIMP settings directory. If the
 * environment variable GIMP_DIRECTORY exists, it is used. If it is an
 * absolute path, it is used as is.  If it is a relative path, it is
 * taken to be a subdirectory of the home directory. If it is a
 * relative path, and no home directory can be determined, it is taken
 * to be a subdirectory of gimp_data_directory().
 *
 * The usual case is that no GIMP_DIRECTORY environment variable
 * exists, and then we use the GIMPDIR subdirectory of the home
 * directory. If no home directory exists, we use a per-user
 * subdirectory of gimp_data_directory().  In any case, we always
 * return some non-empty string, whether it corresponds to an existing
 * directory or not.
 *
 * The returned string is allocated just once, and should *NOT* be
 * freed with g_free().
 *
 * Returns: The user-specific GIMP settings directory.
 **/
const gchar *
gimp_directory (void)
{
  static gchar *gimp_dir = NULL;

  const gchar  *env_gimp_dir;
  const gchar  *home_dir;

  if (gimp_dir)
    return gimp_dir;

  env_gimp_dir = g_getenv ("GIMP_DIRECTORY");
  home_dir     = g_get_home_dir ();

  if (env_gimp_dir)
    {
      if (g_path_is_absolute (env_gimp_dir))
        {
          gimp_dir = g_strdup (env_gimp_dir);
        }
      else
	{
	  if (home_dir)
	    {
	      gimp_dir = g_build_filename (home_dir,
                                           env_gimp_dir,
                                           NULL);
	    }
	  else
	    {
	      gimp_dir = g_build_filename (gimp_data_directory (),
                                           env_gimp_dir, NULL);
	    }
	}
    }
  else
    {
#ifdef __EMX__       
	gimp_dir = g_strdup(__XOS2RedirRoot(GIMPDIR));
	return gimp_dir;  
#endif      
	if (home_dir)
	{
	  gimp_dir = g_build_filename (home_dir, GIMPDIR, NULL);
	}
      else
	{
#ifndef G_OS_WIN32
	  g_message ("warning: no home directory.");
#endif
	  gimp_dir = g_build_filename (gimp_data_directory (),
                                       GIMPDIR ".",
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
 **/
gchar *
gimp_personal_rc_file (const gchar *basename)
{
  return g_build_filename (gimp_directory (), basename, NULL);
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
 **/
const gchar *
gimp_data_directory (void)
{
  static gchar *gimp_data_dir = NULL;

  if (gimp_data_dir)
    return gimp_data_dir;

  gimp_data_dir = gimp_env_get_dir ("GIMP_DATADIR", DATADIR);

  return gimp_data_dir;
}

/**
 * gimp_sysconf_directory:
 *
 * Returns the top directory for GIMP config files. If the environment
 * variable GIMP_SYSCONFDIR exists, that is used.  It should be an
 * absolute pathname.  Otherwise, on Unix the compile-time defined
 * directory is used.  On Win32, the installation directory as deduced
 * from the executable's name is used.
 *
 * The returned string is allocated just once, and should *NOT* be
 * freed with g_free().
 *
 * Returns: The top directory for GIMP config files.
 **/
const gchar *
gimp_sysconf_directory (void)
{
  static gchar *gimp_sysconf_dir = NULL;

  if (gimp_sysconf_dir != NULL)
    return gimp_sysconf_dir;

  gimp_sysconf_dir = gimp_env_get_dir ("GIMP_SYSCONFDIR", SYSCONFDIR);

  return gimp_sysconf_dir;
}

/**
 * gimp_plug_in_directory:
 *
 * Returns the top directory for GIMP plug_ins and modules. If the 
 * environment variable GIMP_PLUGINDIR exists, that is used.  It 
 * should be an absolute pathname. Otherwise, on Unix the compile-time 
 * defined directory is used. On Win32, the installation directory as 
 * deduced from the executable's name is used.
 *
 * The returned string is allocated just once, and should *NOT* be
 * freed with g_free().
 *
 * Returns: The top directory for GIMP plug_ins and modules.
 **/
const gchar *
gimp_plug_in_directory (void)
{
  static gchar *gimp_plug_in_dir = NULL;

  if (gimp_plug_in_dir)
    return gimp_plug_in_dir;

  gimp_plug_in_dir = gimp_env_get_dir ("GIMP_PLUGINDIR", PLUGINDIR);

  return gimp_plug_in_dir;
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
 **/ 
const gchar *
gimp_gtkrc (void)
{
  static gchar *gimp_gtkrc_filename = NULL;

  if (! gimp_gtkrc_filename)
    {
      gimp_gtkrc_filename = g_build_filename (gimp_data_directory (),
                                              "themes", "Default", "gtkrc",
                                              NULL);
    }

  return gimp_gtkrc_filename;
}

/**
 * gimp_path_parse:
 * @path:         A list of directories separated by #G_SEARCHPATH_SEPARATOR.
 * @max_paths:    The maximum number of directories to return.
 * @check:        #TRUE if you want the directories to be checked.
 * @check_failed: Returns a #GList of path elements for which the
 *                check failed.
 *
 * Returns: A #GList of all directories in @path.
 **/
GList *
gimp_path_parse (const gchar  *path,
		 gint          max_paths,
		 gboolean      check,
		 GList       **check_failed)
{
  const gchar  *home;
  gchar       **patharray;
  GList        *list      = NULL;
  GList        *fail_list = NULL;
  gint          i;
  gboolean      exists    = TRUE;

  if (!path || !*path || max_paths < 1 || max_paths > 256)
    return NULL;

  home = g_get_home_dir ();

  patharray = g_strsplit (path, G_SEARCHPATH_SEPARATOR_S, max_paths);

  for (i = 0; i < max_paths; i++)
    {
      GString *dir;

      if (! patharray[i])
	break;

#ifndef G_OS_WIN32
      if (*patharray[i] == '~')
	{
	  dir = g_string_new (home);
	  g_string_append (dir, patharray[i] + 1);
	}
      else
#endif
	{
	  dir = g_string_new (patharray[i]);
	}

#ifdef __EMX__
      _fnslashify (dir);
#endif

      if (check)
        exists = g_file_test (dir->str, G_FILE_TEST_IS_DIR);

      if (exists)
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
 **/
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
 **/
void
gimp_path_free (GList *path)
{
  GList *list;

  for (list = path; list; list = g_list_next (list))
    {
      g_free (list->data);
    }

  g_list_free (path);
}

/**
 * gimp_path_get_user_writable_dir:
 * @path: A list of directories as returned by gimp_path_parse().
 *
 * Note that you have to g_free() the returned string.
 *
 * Returns: The first directory in @path where the user has write permission.
 **/
gchar *
gimp_path_get_user_writable_dir (GList *path)
{
  GList *list;

  uid_t euid;
  gid_t egid;

  struct stat filestat;
  gint        err;

  g_return_val_if_fail (path != NULL, NULL);

  euid = geteuid ();
  egid = getegid ();

  for (list = path; list; list = g_list_next (list))
    {
      gchar *dir;

      dir = (gchar *) list->data;

      /*  check if directory exists  */
      err = stat (dir, &filestat);

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
	  return g_strdup (dir);
	}
    }

  return NULL;
}

static gchar *
gimp_env_get_dir (const gchar *gimp_env_name,
                  const gchar *env_dir)
{
  const gchar *env;

  env = g_getenv (gimp_env_name);

  if (env)
    {
      if (! g_path_is_absolute (env))
	g_error ("%s environment variable should be an absolute path.", 
                 gimp_env_name);
#ifndef __EMX__
      return g_strdup (env);
#else
      return g_strdup (__XOS2RedirRoot(env));
#endif
    }
  else
    {
#ifndef G_OS_WIN32
#ifndef __EMX__
      return (gchar *) env_dir;
#else
      return g_strdup (__XOS2RedirRoot(env_dir));
#endif
#else
      /* Figure it out from the executable name */
      gchar filename[MAX_PATH];
      gchar *sep1, *sep2;

      if (GetModuleFileName (NULL, filename, sizeof (filename)) == 0)
	g_error ("GetModuleFilename failed\n");

      /* If the executable file name is of the format
       * <foobar>\bin\gimp.exe or <foobar>\plug-ins\filter.exe, use
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

      return g_strdup (filename);
#endif
    }
}
