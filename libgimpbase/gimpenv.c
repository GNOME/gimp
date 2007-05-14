/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * gimpenv.c
 * Copyright (C) 1999 Tor Lillqvist <tml@iki.fi>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#include "config.h"

#include <string.h>
#include <sys/types.h>

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include <glib-object.h>
#include <glib/gstdio.h>

#ifndef LIBGIMP_COMPILATION
#define LIBGIMP_COMPILATION
#endif

#include "gimpbasetypes.h"

#include "gimpenv.h"
#include "gimpversion.h"
#include "gimpreloc.h"

#ifdef G_OS_WIN32
#define STRICT
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <io.h>
#ifndef S_IWUSR
# define S_IWUSR _S_IWRITE
#endif
#ifndef S_IWGRP
#define S_IWGRP (_S_IWRITE>>3)
#define S_IWOTH (_S_IWRITE>>6)
#endif
#ifndef S_ISDIR
# define __S_ISTYPE(mode, mask) (((mode) & _S_IFMT) == (mask))
# define S_ISDIR(mode)  __S_ISTYPE((mode), _S_IFDIR)
#endif
#define uid_t gint
#define gid_t gint
#define geteuid() 0
#define getegid() 0
#endif

#ifndef G_OS_WIN32
#ifndef HAVE_CARBON
#include "xdg-user-dir.h"
#endif
#endif

#ifdef HAVE_CARBON
#include <CoreServices/CoreServices.h>
#endif


static gchar * gimp_env_get_dir (const gchar *gimp_env_name,
                                 const gchar *env_dir);


const guint gimp_major_version = GIMP_MAJOR_VERSION;
const guint gimp_minor_version = GIMP_MINOR_VERSION;
const guint gimp_micro_version = GIMP_MICRO_VERSION;


/**
 * gimp_env_init:
 * @plug_in: must be %TRUE if this function is called from a plug-in
 *
 * You don't need to care about this function. It is being called for
 * you automatically (by means of the MAIN() macro that every plug-in
 * runs). Calling it again will cause a fatal error.
 *
 * Since: GIMP 2.4
 */
void
gimp_env_init (gboolean plug_in)
{
  static gboolean gimp_env_initialized = FALSE;

  if (gimp_env_initialized)
    g_error ("gimp_env_init() must only be called once!");

  gimp_env_initialized = TRUE;

#ifndef G_OS_WIN32
  if (plug_in)
    {
      _gimp_reloc_init_lib (NULL);
    }
  else if (_gimp_reloc_init (NULL))
    {
      /* Set $LD_LIBRARY_PATH to ensure that plugins can be loaded. */

      const gchar *ldpath = g_getenv ("LD_LIBRARY_PATH");
      gchar       *libdir = _gimp_reloc_find_lib_dir (NULL);

      if (ldpath && *ldpath)
        {
          gchar *tmp = g_strconcat (libdir, ":", ldpath, NULL);

          g_setenv ("LD_LIBRARY_PATH", tmp, TRUE);

          g_free (tmp);
        }
      else
        {
          g_setenv ("LD_LIBRARY_PATH", libdir, TRUE);
        }

      g_free (libdir);
    }
#endif
}

/**
 * gimp_directory:
 *
 * Returns the user-specific GIMP settings directory. If the
 * environment variable GIMP2_DIRECTORY exists, it is used. If it is
 * an absolute path, it is used as is.  If it is a relative path, it
 * is taken to be a subdirectory of the home directory. If it is a
 * relative path, and no home directory can be determined, it is taken
 * to be a subdirectory of gimp_data_directory().
 *
 * The usual case is that no GIMP2_DIRECTORY environment variable
 * exists, and then we use the GIMPDIR subdirectory of the home
 * directory. If no home directory exists, we use a per-user
 * subdirectory of gimp_data_directory().  In any case, we always
 * return some non-empty string, whether it corresponds to an existing
 * directory or not.
 *
 * The returned string is owned by GIMP and must not be modified or
 * freed. The returned string is in the encoding used for filenames by
 * the system, which isn't necessarily UTF-8 (never is on Windows).
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

  env_gimp_dir = g_getenv ("GIMP2_DIRECTORY");
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
      if (home_dir)
        {
          gimp_dir = g_build_filename (home_dir, GIMPDIR, NULL);
        }
      else
        {
          gchar *user_name = g_strdup (g_get_user_name ());
          gchar *subdir_name;

#ifdef G_OS_WIN32
          gchar *p = user_name;

          while (*p)
            {
              /* Replace funny characters in the user name with an
               * underscore. The code below also replaces some
               * characters that in fact are legal in file names, but
               * who cares, as long as the definitely illegal ones are
               * caught.
               */
              if (!g_ascii_isalnum (*p) && !strchr ("-.,@=", *p))
                *p = '_';
              p++;
            }
#endif

#ifndef G_OS_WIN32
          g_message ("warning: no home directory.");
#endif
          subdir_name = g_strconcat (GIMPDIR ".", user_name, NULL);
          gimp_dir = g_build_filename (gimp_data_directory (),
                                       subdir_name,
                                       NULL);
          g_free (user_name);
          g_free (subdir_name);
        }
    }

  return gimp_dir;
}

static const gchar *
gimp_toplevel_directory (void)
{
  static gchar *toplevel = NULL;

  if (toplevel)
    return toplevel;

#ifdef G_OS_WIN32
  {
    /* Figure it out from the executable name */
    gchar *filename;
    gchar *sep1, *sep2;

    if (G_WIN32_HAVE_WIDECHAR_API ())
      {
        wchar_t w_filename[MAX_PATH];

        if (GetModuleFileNameW (NULL,
                                w_filename, G_N_ELEMENTS (w_filename)) == 0)
          g_error ("GetModuleFilenameW failed");

        filename = g_utf16_to_utf8 (w_filename, -1, NULL, NULL, NULL);
        if (filename == NULL)
          g_error ("Converting module filename to UTF-8 failed");
      }
    else
      {
        gchar cp_filename[MAX_PATH];

        if (GetModuleFileNameA (NULL,
                                cp_filename, G_N_ELEMENTS (cp_filename)) == 0)
          g_error ("GetModuleFilenameA failed");

        filename = g_locale_to_utf8 (cp_filename, -1, NULL, NULL, NULL);
        if (filename == NULL)
          g_error ("Converting module filename to UTF-8 failed");
      }

    /* If the executable file name is of the format
     * <foobar>\bin\*.exe or
     * <foobar>\lib\gimp\GIMP_API_VERSION\plug-ins\*.exe, use <foobar>.
     * Otherwise, use the directory where the executable is.
     */

    sep1 = strrchr (filename, '\\');
    *sep1 = '\0';

    sep2 = strrchr (filename, '\\');
    if (sep2 != NULL)
      {
        if (g_ascii_strcasecmp (sep2 + 1, "bin") == 0)
          {
            *sep2 = '\0';
          }
        else
          {
            gchar test[MAX_PATH];

            g_snprintf (test, sizeof (test) - 1,
                        "\\lib\\gimp\\%s\\plug-ins", GIMP_API_VERSION);

            if (strlen (filename) > strlen (test) &&
                g_ascii_strcasecmp (filename + strlen (filename) - strlen (test),
                                    test) == 0)
              {
                filename[strlen (filename) - strlen (test)] = '\0';
              }
          }
      }

    toplevel = filename;
  }
#else
  toplevel = _gimp_reloc_find_prefix (PREFIX);
#endif

  return toplevel;
}

/**
 * gimp_data_directory:
 *
 * Returns the top directory for GIMP data. If the environment
 * variable GIMP2_DATADIR exists, that is used.  It should be an
 * absolute pathname.  Otherwise, on Unix the compile-time defined
 * directory is used.  On Win32, the installation directory as deduced
 * from the executable's name is used.
 *
 * The returned string is owned by GIMP and must not be modified or
 * freed. The returned string is in the encoding used for filenames by
 * the system, which isn't necessarily UTF-8 (never is on Windows).
 *
 * Returns: The top directory for GIMP data.
 **/
const gchar *
gimp_data_directory (void)
{
  static gchar *gimp_data_dir = NULL;

  if (! gimp_data_dir)
    {
      gchar *tmp = _gimp_reloc_find_data_dir (DATADIR);

      gimp_data_dir = gimp_env_get_dir ("GIMP2_DATADIR", tmp);
      g_free (tmp);
    }

  return gimp_data_dir;
}

/**
 * gimp_locale_directory:
 *
 * Returns the top directory for GIMP locale files. If the environment
 * variable GIMP2_LOCALEDIR exists, that is used.  It should be an
 * absolute pathname.  Otherwise, on Unix the compile-time defined
 * directory is used.  On Win32, the installation directory as deduced
 * from the executable's name is used.
 *
 * The returned string is owned by GIMP and must not be modified or
 * freed. The returned string is in the encoding used for filenames by
 * the system, which isn't necessarily UTF-8 (never is on Windows).
 *
 * Returns: The top directory for GIMP locale files.
 */
const gchar *
gimp_locale_directory (void)
{
  static gchar *gimp_locale_dir = NULL;

  if (! gimp_locale_dir)
    {
      gchar *tmp = _gimp_reloc_find_locale_dir (LOCALEDIR);

      gimp_locale_dir = gimp_env_get_dir ("GIMP2_LOCALEDIR", tmp);
      g_free (tmp);
    }

  return gimp_locale_dir;
}

/**
 * gimp_sysconf_directory:
 *
 * Returns the top directory for GIMP config files. If the environment
 * variable GIMP2_SYSCONFDIR exists, that is used.  It should be an
 * absolute pathname.  Otherwise, on Unix the compile-time defined
 * directory is used.  On Win32, the installation directory as deduced
 * from the executable's name is used.
 *
 * The returned string is owned by GIMP and must not be modified or
 * freed. The returned string is in the encoding used for filenames by
 * the system, which isn't necessarily UTF-8 (never is on Windows).
 *
 * Returns: The top directory for GIMP config files.
 **/
const gchar *
gimp_sysconf_directory (void)
{
  static gchar *gimp_sysconf_dir = NULL;

  if (! gimp_sysconf_dir)
    {
      gchar *tmp = _gimp_reloc_find_etc_dir (SYSCONFDIR);

      gimp_sysconf_dir = gimp_env_get_dir ("GIMP2_SYSCONFDIR", tmp);
      g_free (tmp);
    }

  return gimp_sysconf_dir;
}

#ifdef G_OS_WIN32

#undef DATADIR			/* Collision otherwise */

#include <shlobj.h>

#ifndef CSIDL_MYDOCUMENTS
#define CSIDL_MYDOCUMENTS 0x000C
#endif
#ifndef CSIDL_MYMUSIC
#define CSIDL_MYMUSIC 0x000D
#endif
#ifndef CSIDL_MYVIDEO
#define CSIDL_MYVIDEO 0x000E
#endif

static gchar *
get_special_folder (int csidl)
{
  union {
    char c[MAX_PATH+1];
    wchar_t wc[MAX_PATH+1];
  } path;
  HRESULT hr;
  LPITEMIDLIST pidl = NULL;
  BOOL b;
  gchar *retval = NULL;

  hr = SHGetSpecialFolderLocation (NULL, csidl, &pidl);
  if (hr == S_OK)
    {
      b = SHGetPathFromIDListW (pidl, path.wc);
      if (b)
	retval = g_utf16_to_utf8 (path.wc, -1, NULL, NULL, NULL);
      CoTaskMemFree (pidl);
    }

  return retval;
}
#endif

#ifdef HAVE_CARBON
static gchar *
find_folder (OSType type)
{
  gchar *filename = NULL;
  FSRef  found;

  if (FSFindFolder (kUserDomain, type, kDontCreateFolder, &found) == noErr)
    {
      CFURLRef url = CFURLCreateFromFSRef (kCFAllocatorSystemDefault, &found);

      if (url)
	{
	  CFStringRef path = CFURLCopyFileSystemPath (url, kCFURLPOSIXPathStyle);

	  if (path)
	    {
	      filename = g_strdup (CFStringGetCStringPtr (path, kCFStringEncodingUTF8));

	      if (! filename)
		{
		  filename = g_new0 (gchar, CFStringGetLength (path) * 3 + 1);

		  CFStringGetCString (path, filename,
				      CFStringGetLength (path) * 3 + 1,
				      kCFStringEncodingUTF8);
		}

	      CFRelease (path);
	    }

	  CFRelease (url);
	}
    }

  return filename;
}
#endif

/**
 * gimp_user_directory:
 * @type: the type of user directory to retrieve
 *
 * Identifies special folders used frequently by applications, but
 * which may not have the same name or location on any given system.

 * Plug-ins may want to use this function to add shortcuts to such
 * folders to a file-chooser.
 *
 * Returns: a newly allocated directory name in filesystem encoding,
 *          or %NULL
 *
 * Since: GIMP 2.4
 **/
gchar *
gimp_user_directory (GimpUserDirectory type)
{
  switch (type)
    {
#ifdef G_OS_WIN32
    case GIMP_USER_DIRECTORY_DESKTOP:
      return get_special_folder (CSIDL_DESKTOPDIRECTORY);

    case GIMP_USER_DIRECTORY_DOCUMENTS:
      return get_special_folder (CSIDL_MYDOCUMENTS);

    case GIMP_USER_DIRECTORY_MUSIC:
      return get_special_folder (CSIDL_MYMUSIC);

    case GIMP_USER_DIRECTORY_PICTURES:
      return get_special_folder (CSIDL_MYPICTURES);

    case GIMP_USER_DIRECTORY_TEMPLATES:
      return get_special_folder (CSIDL_TEMPLATES);

    case GIMP_USER_DIRECTORY_VIDEOS:
      return get_special_folder (CSIDL_MYVIDEO);

#elif HAVE_CARBON
    case GIMP_USER_DIRECTORY_DESKTOP:
      return find_folder (kDesktopFolderType);

    case GIMP_USER_DIRECTORY_DOCUMENTS:
      return find_folder (kDocumentsFolderType);

    case GIMP_USER_DIRECTORY_MUSIC:
      return find_folder (kMusicDocumentsFolderType);

    case GIMP_USER_DIRECTORY_PICTURES:
      return find_folder (kPictureDocumentsFolderType);

    case GIMP_USER_DIRECTORY_TEMPLATES:
      return NULL;

    case GIMP_USER_DIRECTORY_VIDEOS:
      return find_folder (kMovieDocumentsFolderType);

#else
    case GIMP_USER_DIRECTORY_DESKTOP:
      return _xdg_user_dir_lookup ("DESKTOP");

    case GIMP_USER_DIRECTORY_DOCUMENTS:
      return _xdg_user_dir_lookup ("DOCUMENTS");

    case GIMP_USER_DIRECTORY_MUSIC:
      return _xdg_user_dir_lookup ("MUSIC");

    case GIMP_USER_DIRECTORY_PICTURES:
      return _xdg_user_dir_lookup ("PICTURES");

    case GIMP_USER_DIRECTORY_TEMPLATES:
      return _xdg_user_dir_lookup ("TEMPLATES");

    case GIMP_USER_DIRECTORY_VIDEOS:
      return _xdg_user_dir_lookup ("VIDEOS");

#endif
    default:
      return NULL;
    }
}

/**
 * gimp_plug_in_directory:
 *
 * Returns the top directory for GIMP plug_ins and modules. If the
 * environment variable GIMP2_PLUGINDIR exists, that is used.  It
 * should be an absolute pathname. Otherwise, on Unix the compile-time
 * defined directory is used. On Win32, the installation directory as
 * deduced from the executable's name is used.
 *
 * The returned string is owned by GIMP and must not be modified or
 * freed. The returned string is in the encoding used for filenames by
 * the system, which isn't necessarily UTF-8 (never is on Windows).
 *
 * Returns: The top directory for GIMP plug_ins and modules.
 **/
const gchar *
gimp_plug_in_directory (void)
{
  static gchar *gimp_plug_in_dir = NULL;

  if (! gimp_plug_in_dir)
    {
      gchar *tmp = _gimp_reloc_find_plugin_dir (PLUGINDIR);

      gimp_plug_in_dir = gimp_env_get_dir ("GIMP2_PLUGINDIR", tmp);
      g_free (tmp);
    }

  return gimp_plug_in_dir;
}

/**
 * gimp_personal_rc_file:
 * @basename: The basename of a rc_file.
 *
 * Returns the name of a file in the user-specific GIMP settings directory.
 *
 * The returned string is allocated dynamically and *SHOULD* be freed
 * with g_free() after use. The returned string is in the encoding used
 * for filenames by the system, which isn't necessarily UTF-8 (never
 * is on Windows).
 *
 * Returns: The name of a file in the user-specific GIMP settings directory.
 **/
gchar *
gimp_personal_rc_file (const gchar *basename)
{
  return g_build_filename (gimp_directory (), basename, NULL);
}

/**
 * gimp_gtkrc:
 *
 * Returns the name of the GIMP's application-specific gtkrc file.
 *
 * The returned string is owned by GIMP and must not be modified or
 * freed. The returned string is in the encoding used for filenames by
 * the system, which isn't necessarily UTF-8 (never is on Windows).
 *
 * Returns: The name of the GIMP's application-specific gtkrc file.
 **/
const gchar *
gimp_gtkrc (void)
{
  static gchar *gimp_gtkrc_filename = NULL;

  if (! gimp_gtkrc_filename)
    gimp_gtkrc_filename = g_build_filename (gimp_data_directory (),
                                            "themes", "Default", "gtkrc",
                                            NULL);

  return gimp_gtkrc_filename;
}

/**
 * gimp_path_runtime_fix:
 * @path: A pointer to a string (allocated with g_malloc) that is
 *        (or could be) a pathname.
 *
 * On Windows, this function checks if the string pointed to by @path
 * starts with the compile-time prefix, and in that case, replaces the
 * prefix with the run-time one.  @path should be a pointer to a
 * dynamically allocated (with g_malloc, g_strconcat, etc) string. If
 * the replacement takes place, the original string is deallocated,
 * and *@path is replaced with a pointer to a new string with the
 * run-time prefix spliced in.
 *
 * On Linux, it does the same thing, but only if BinReloc support is enabled.
 * On other Unices, it does nothing because those platforms don't have a
 * way to find out where our binary is.
 */
static void
gimp_path_runtime_fix (gchar **path)
{
#if defined (G_OS_WIN32) && defined (PREFIX)
  gchar *p;

  /* Yes, I do mean forward slashes below */
  if (strncmp (*path, PREFIX "/", strlen (PREFIX "/")) == 0)
    {
      /* This is a compile-time entry. Replace the path with the
       * real one on this machine.
       */
      p = *path;
      *path = g_strconcat (gimp_toplevel_directory (),
                           "\\",
                           *path + strlen (PREFIX "/"),
                           NULL);
      g_free (p);
    }
  /* Replace forward slashes with backslashes, just for
   * completeness */
  p = *path;
  while ((p = strchr (p, '/')) != NULL)
    {
      *p = '\\';
      p++;
    }
#elif defined (G_OS_WIN32)
  /* without defineing PREFIX do something useful too */
  gchar *p = *path;
  if (!g_path_is_absolute (p))
    {
      *path = g_build_filename (gimp_toplevel_directory (), *path, NULL);
      g_free (p);
    }
#else
  gchar *p;

  if (strncmp (*path, PREFIX G_DIR_SEPARATOR_S,
               strlen (PREFIX G_DIR_SEPARATOR_S)) == 0)
    {
      /* This is a compile-time entry. Replace the path with the
       * real one on this machine.
       */
      p = *path;
      *path = g_build_filename (gimp_toplevel_directory (),
                                *path + strlen (PREFIX G_DIR_SEPARATOR_S),
                                NULL);
      g_free (p);
    }
#endif
}

/**
 * gimp_path_parse:
 * @path:         A list of directories separated by #G_SEARCHPATH_SEPARATOR.
 * @max_paths:    The maximum number of directories to return.
 * @check:        %TRUE if you want the directories to be checked.
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
          gimp_path_runtime_fix (&patharray[i]);
          dir = g_string_new (patharray[i]);
        }

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
  GString *str    = NULL;
  GList   *list;
  gchar   *retval = NULL;

  for (list = path; list; list = g_list_next (list))
    {
      gchar *dir = list->data;

      if (str)
        {
          g_string_append_c (str, G_SEARCHPATH_SEPARATOR);
          g_string_append (str, dir);
        }
      else
        {
          str = g_string_new (dir);
        }
    }

  if (str)
    retval = g_string_free (str, FALSE);

  return retval;
}

/**
 * gimp_path_free:
 * @path: A list of directories as returned by gimp_path_parse().
 *
 * This function frees the memory allocated for the list and the strings
 * it contains.
 **/
void
gimp_path_free (GList *path)
{
  g_list_foreach (path, (GFunc) g_free, NULL);
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
  GList       *list;
  uid_t        euid;
  gid_t        egid;
  struct stat  filestat;
  gint         err;

  g_return_val_if_fail (path != NULL, NULL);

  euid = geteuid ();
  egid = getegid ();

  for (list = path; list; list = g_list_next (list))
    {
      gchar *dir = list->data;

      /*  check if directory exists  */
      err = g_stat (dir, &filestat);

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
  const gchar *env = g_getenv (gimp_env_name);

  if (env)
    {
      if (! g_path_is_absolute (env))
        g_error ("%s environment variable should be an absolute path.",
                 gimp_env_name);

      return g_strdup (env);
    }
  else
    {
      gchar *retval = g_strdup (env_dir);

      gimp_path_runtime_fix (&retval);

      return retval;
    }
}
