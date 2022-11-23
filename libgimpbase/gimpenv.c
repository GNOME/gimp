/* LIBLIGMA - The LIGMA Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * ligmaenv.c
 * Copyright (C) 1999 Tor Lillqvist <tml@iki.fi>
 *
 * This library is free software: you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library.  If not, see
 * <https://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <errno.h>
#include <string.h>
#include <sys/types.h>

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#ifdef PLATFORM_OSX
#include <AppKit/AppKit.h>
#endif

#include <gio/gio.h>
#include <glib/gstdio.h>

#include "ligmabasetypes.h"

#define __LIGMA_ENV_C__
#include "ligmaenv.h"
#include "ligmaversion.h"
#include "ligmareloc.h"

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

#include <shlobj.h>

/* Constant available since Shell32.dll 4.72 */
#ifndef CSIDL_APPDATA
#define CSIDL_APPDATA 0x001a
#endif

#endif


/**
 * SECTION: ligmaenv
 * @title: ligmaenv
 * @short_description: Functions to access the LIGMA environment.
 *
 * A set of functions to find the locations of LIGMA's data directories
 * and configuration files.
 **/


static gchar * ligma_env_get_dir   (const gchar *ligma_env_name,
                                   const gchar *compile_time_dir,
                                   const gchar *relative_subdir);
#ifdef G_OS_WIN32
static gchar * get_special_folder (gint         csidl);
#endif


const guint ligma_major_version = LIGMA_MAJOR_VERSION;
const guint ligma_minor_version = LIGMA_MINOR_VERSION;
const guint ligma_micro_version = LIGMA_MICRO_VERSION;


/**
 * ligma_env_init:
 * @plug_in: must be %TRUE if this function is called from a plug-in
 *
 * You don't need to care about this function. It is being called for
 * you automatically (by means of the MAIN() macro that every plug-in
 * runs). Calling it again will cause a fatal error.
 *
 * Since: 2.4
 */
void
ligma_env_init (gboolean plug_in)
{
  static gboolean  ligma_env_initialized = FALSE;
  const gchar     *data_home = g_get_user_data_dir ();

  if (ligma_env_initialized)
    g_error ("ligma_env_init() must only be called once!");

  ligma_env_initialized = TRUE;

#ifndef G_OS_WIN32
  if (plug_in)
    {
      _ligma_reloc_init_lib (NULL);
    }
  else if (_ligma_reloc_init (NULL))
    {
      /* Set $LD_LIBRARY_PATH to ensure that plugins can be loaded. */

      const gchar *ldpath = g_getenv ("LD_LIBRARY_PATH");
      gchar       *libdir = g_build_filename (ligma_installation_directory (),
                                              "lib",
                                              NULL);

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

  /* The user data directory (XDG_DATA_HOME on Unix) is used to store
   * various data, like crash logs (win32) or recently used file history
   * (by GTK+). Yet it may be absent, in particular on non-Linux
   * platforms. Make sure it exists.
   */
  if (! g_file_test (data_home, G_FILE_TEST_IS_DIR))
    {
      if (g_mkdir_with_parents (data_home, S_IRUSR | S_IWUSR | S_IXUSR) != 0)
        {
          g_warning ("Failed to create the data directory '%s': %s",
                     data_home, g_strerror (errno));
        }
    }
}

/**
 * ligma_directory:
 *
 * Returns the user-specific LIGMA settings directory. If the
 * environment variable LIGMA3_DIRECTORY exists, it is used. If it is
 * an absolute path, it is used as is.  If it is a relative path, it
 * is taken to be a subdirectory of the home directory. If it is a
 * relative path, and no home directory can be determined, it is taken
 * to be a subdirectory of ligma_data_directory().
 *
 * The usual case is that no LIGMA3_DIRECTORY environment variable
 * exists, and then we use the LIGMADIR subdirectory of the local
 * configuration directory:
 *
 * - UNIX: $XDG_CONFIG_HOME (defaults to $HOME/.config/)
 *
 * - Windows: CSIDL_APPDATA
 *
 * - OSX (UNIX exception): the Application Support Directory.
 *
 * If neither the configuration nor home directory exist,
 * g_get_user_config_dir() will return {tmp}/{user_name}/.config/ where
 * the temporary directory {tmp} and the {user_name} are determined
 * according to platform rules.
 *
 * In any case, we always return some non-empty string, whether it
 * corresponds to an existing directory or not.
 *
 * In config files such as ligmarc, the string ${ligma_dir} expands to
 * this directory.
 *
 * The returned string is owned by LIGMA and must not be modified or
 * freed. The returned string is in the encoding used for filenames by
 * GLib, which isn't necessarily UTF-8 (on Windows it is always
 * UTF-8.)
 *
 * Returns: The user-specific LIGMA settings directory.
 **/
const gchar *
ligma_directory (void)
{
  static gchar *ligma_dir          = NULL;
  static gchar *last_env_ligma_dir = NULL;

  const gchar  *env_ligma_dir;

  env_ligma_dir = g_getenv ("LIGMA3_DIRECTORY");

  if (ligma_dir)
    {
      gboolean ligma3_directory_changed = FALSE;

      /* We have constructed the ligma_dir already. We can return
       * ligma_dir unless some parameter ligma_dir depends on has
       * changed. For now we just check for changes to LIGMA3_DIRECTORY
       */
      ligma3_directory_changed =
        (env_ligma_dir == NULL &&
         last_env_ligma_dir != NULL) ||
        (env_ligma_dir != NULL &&
         last_env_ligma_dir == NULL) ||
        (env_ligma_dir != NULL &&
         last_env_ligma_dir != NULL &&
         strcmp (env_ligma_dir, last_env_ligma_dir) != 0);

      if (! ligma3_directory_changed)
        {
          return ligma_dir;
        }
      else
        {
          /* Free the old ligma_dir and go on to update it */
          g_free (ligma_dir);
          ligma_dir = NULL;
        }
    }

  /* Remember the LIGMA3_DIRECTORY to next invocation so we can check
   * if it changes
   */
  g_free (last_env_ligma_dir);
  last_env_ligma_dir = g_strdup (env_ligma_dir);

  if (env_ligma_dir)
    {
      if (g_path_is_absolute (env_ligma_dir))
        {
          ligma_dir = g_strdup (env_ligma_dir);
        }
      else
        {
          const gchar *home_dir = g_get_home_dir ();

          if (home_dir)
            ligma_dir = g_build_filename (home_dir, env_ligma_dir, NULL);
          else
            ligma_dir = g_build_filename (ligma_data_directory (), env_ligma_dir, NULL);
        }
    }
  else if (g_path_is_absolute (LIGMADIR))
    {
      ligma_dir = g_strdup (LIGMADIR);
    }
  else
    {
#ifdef PLATFORM_OSX

      NSAutoreleasePool *pool;
      NSArray           *path;
      NSString          *library_dir;

      pool = [[NSAutoreleasePool alloc] init];

      path = NSSearchPathForDirectoriesInDomains (NSApplicationSupportDirectory,
                                                  NSUserDomainMask, YES);
      library_dir = [path objectAtIndex:0];

      ligma_dir = g_build_filename ([library_dir UTF8String],
                                   LIGMADIR, LIGMA_USER_VERSION, NULL);

      [pool drain];

#elif defined G_OS_WIN32

      gchar *conf_dir = get_special_folder (CSIDL_APPDATA);

      ligma_dir = g_build_filename (conf_dir,
                                   LIGMADIR, LIGMA_USER_VERSION, NULL);
      g_free(conf_dir);

#else /* UNIX */

      /* g_get_user_config_dir () always returns a path as a non-null
       * and non-empty string
       */
      ligma_dir = g_build_filename (g_get_user_config_dir (),
                                   LIGMADIR, LIGMA_USER_VERSION, NULL);

#endif /* PLATFORM_OSX */
    }

  return ligma_dir;
}

#ifdef G_OS_WIN32

/* Taken from glib 2.35 code. */
static gchar *
get_special_folder (int csidl)
{
  wchar_t      path[MAX_PATH+1];
  HRESULT      hr;
  LPITEMIDLIST pidl = NULL;
  BOOL         b;
  gchar       *retval = NULL;

  hr = SHGetSpecialFolderLocation (NULL, csidl, &pidl);
  if (hr == S_OK)
    {
      b = SHGetPathFromIDListW (pidl, path);
      if (b)
        retval = g_utf16_to_utf8 (path, -1, NULL, NULL, NULL);
      CoTaskMemFree (pidl);
    }

  return retval;
}

static HMODULE libligmabase_dll = NULL;

/* Minimal DllMain that just stores the handle to this DLL */

BOOL WINAPI /* Avoid silly "no previous prototype" gcc warning */
DllMain (HINSTANCE hinstDLL,
         DWORD     fdwReason,
         LPVOID    lpvReserved);

BOOL WINAPI
DllMain (HINSTANCE hinstDLL,
         DWORD     fdwReason,
         LPVOID    lpvReserved)
{
  switch (fdwReason)
    {
    case DLL_PROCESS_ATTACH:
      libligmabase_dll = hinstDLL;
      break;
    }

  return TRUE;
}

#endif

/**
 * ligma_installation_directory:
 *
 * Returns the top installation directory of LIGMA. On Unix the
 * compile-time defined installation prefix is used. On Windows, the
 * installation directory as deduced from the executable's full
 * filename is used. On OSX we ask [NSBundle mainBundle] for the
 * resource path to check if LIGMA is part of a relocatable bundle.
 *
 * In config files such as ligmarc, the string ${ligma_installation_dir}
 * expands to this directory.
 *
 * The returned string is owned by LIGMA and must not be modified or
 * freed. The returned string is in the encoding used for filenames by
 * GLib, which isn't necessarily UTF-8. (On Windows it always is
 * UTF-8.)
 *
 * Since: 2.8
 *
 * Returns: The toplevel installation directory of LIGMA.
 **/
const gchar *
ligma_installation_directory (void)
{
  static gchar *toplevel = NULL;

  if (toplevel)
    return toplevel;

#ifdef G_OS_WIN32

  toplevel = g_win32_get_package_installation_directory_of_module (libligmabase_dll);
  if (! toplevel)
    g_error ("g_win32_get_package_installation_directory_of_module() failed");

#elif PLATFORM_OSX

  {
    NSAutoreleasePool *pool;
    NSString          *resource_path;
    gchar             *basename;
    gchar             *basepath;
    gchar             *dirname;

    pool = [[NSAutoreleasePool alloc] init];

    resource_path = [[NSBundle mainBundle] resourcePath];

    basename = g_path_get_basename ([resource_path UTF8String]);
    basepath = g_path_get_dirname ([resource_path UTF8String]);
    dirname  = g_path_get_basename (basepath);

    if (! strcmp (basename, ".libs"))
      {
        /*  we are running from the source dir, do normal unix things  */

        toplevel = _ligma_reloc_find_prefix (PREFIX);
      }
    else if (! strcmp (basename, "bin"))
      {
        /*  we are running the main app, but not from a bundle, the resource
         *  path is the directory which contains the executable
         */

        toplevel = g_strdup (basepath);
      }
    else if (! strcmp (dirname, "plug-ins") ||
             ! strcmp (dirname, "extensions"))
      {
        /*  same for plug-ins and extensions in subdirectory, go three
         *  levels up from prefix/lib/ligma/x.y
         */

        gchar *tmp  = g_path_get_dirname (basepath);
        gchar *tmp2 = g_path_get_dirname (tmp);
        gchar *tmp3 = g_path_get_dirname (tmp2);

        toplevel = g_path_get_dirname (tmp3);

        g_free (tmp);
        g_free (tmp2);
        g_free (tmp3);
      }
    else if (strstr (basepath, "/Cellar/"))
      {
        /*  we are running from a Python.framework bundle built in homebrew
         *  during the build phase
         */

        gchar *fulldir = g_strdup (basepath);
        gchar *lastdir = g_path_get_basename (fulldir);
        gchar *tmp_fulldir;

        while (strcmp (lastdir, "Cellar"))
          {
            tmp_fulldir = g_path_get_dirname (fulldir);

            g_free (lastdir);
            g_free (fulldir);

            fulldir = tmp_fulldir;
            lastdir = g_path_get_basename (fulldir);
          }
        toplevel = g_path_get_dirname (fulldir);

        g_free (fulldir);
        g_free (lastdir);
      }
    else
      {
        /*  if none of the above match, we assume that we are really in a bundle  */

        toplevel = g_strdup ([resource_path UTF8String]);
      }

    g_free (basename);
    g_free (basepath);
    g_free (dirname);

    [pool drain];
  }

#else

  toplevel = _ligma_reloc_find_prefix (PREFIX);

#endif

  return toplevel;
}

/**
 * ligma_data_directory:
 *
 * Returns the default top directory for LIGMA data. If the environment
 * variable LIGMA3_DATADIR exists, that is used.  It should be an
 * absolute pathname.  Otherwise, on Unix the compile-time defined
 * directory is used. On Windows, the installation directory as
 * deduced from the executable's full filename is used.
 *
 * Note that the actual directories used for LIGMA data files can be
 * overridden by the user in the preferences dialog.
 *
 * In config files such as ligmarc, the string ${ligma_data_dir} expands
 * to this directory.
 *
 * The returned string is owned by LIGMA and must not be modified or
 * freed. The returned string is in the encoding used for filenames by
 * GLib, which isn't necessarily UTF-8. (On Windows it always is
 * UTF-8.)
 *
 * Returns: The top directory for LIGMA data.
 **/
const gchar *
ligma_data_directory (void)
{
  static gchar *ligma_data_dir = NULL;

  if (! ligma_data_dir)
    {
      gchar *tmp = g_build_filename ("share",
                                     LIGMA_PACKAGE,
                                     LIGMA_DATA_VERSION,
                                     NULL);

      ligma_data_dir = ligma_env_get_dir ("LIGMA3_DATADIR", LIGMADATADIR, tmp);
      g_free (tmp);
    }

  return ligma_data_dir;
}

/**
 * ligma_locale_directory:
 *
 * Returns the top directory for LIGMA locale files. If the environment
 * variable LIGMA3_LOCALEDIR exists, that is used.  It should be an
 * absolute pathname.  Otherwise, on Unix the compile-time defined
 * directory is used. On Windows, the installation directory as deduced
 * from the executable's full filename is used.
 *
 * The returned string is owned by LIGMA and must not be modified or
 * freed. The returned string is in the encoding used for filenames by
 * the C library, which isn't necessarily UTF-8. (On Windows, unlike
 * the other similar functions here, the return value from this
 * function is in the system codepage, never in UTF-8. It can thus be
 * passed directly to the bindtextdomain() function from libintl which
 * does not handle UTF-8.)
 *
 * Returns: The top directory for LIGMA locale files.
 */
const gchar *
ligma_locale_directory (void)
{
  static gchar *ligma_locale_dir = NULL;

  if (! ligma_locale_dir)
    {
      gchar *tmp = g_build_filename ("share",
                                     "locale",
                                     NULL);

      ligma_locale_dir = ligma_env_get_dir ("LIGMA3_LOCALEDIR", LOCALEDIR, tmp);
      g_free (tmp);

#ifdef G_OS_WIN32
      /* FIXME: g_win32_locale_filename_from_utf8() can actually return
       * NULL (we had actual cases of this). Not sure exactly what
       * ligma_locale_directory() should do when this happens. Anyway
       * that's really broken, and something should be done some day
       * about this!
       */
      tmp = g_win32_locale_filename_from_utf8 (ligma_locale_dir);
      g_free (ligma_locale_dir);
      ligma_locale_dir = tmp;
#endif
    }

  return ligma_locale_dir;
}

/**
 * ligma_sysconf_directory:
 *
 * Returns the top directory for LIGMA config files. If the environment
 * variable LIGMA3_SYSCONFDIR exists, that is used.  It should be an
 * absolute pathname.  Otherwise, on Unix the compile-time defined
 * directory is used. On Windows, the installation directory as deduced
 * from the executable's full filename is used.
 *
 * In config files such as ligmarc, the string ${ligma_sysconf_dir}
 * expands to this directory.
 *
 * The returned string is owned by LIGMA and must not be modified or
 * freed. The returned string is in the encoding used for filenames by
 * GLib, which isn't necessarily UTF-8. (On Windows it always is
 * UTF-8.).
 *
 * Returns: The top directory for LIGMA config files.
 **/
const gchar *
ligma_sysconf_directory (void)
{
  static gchar *ligma_sysconf_dir = NULL;

  if (! ligma_sysconf_dir)
    {
      gchar *tmp = g_build_filename ("etc",
                                     LIGMA_PACKAGE,
                                     LIGMA_SYSCONF_VERSION,
                                     NULL);

      ligma_sysconf_dir = ligma_env_get_dir ("LIGMA3_SYSCONFDIR", LIGMASYSCONFDIR, tmp);
      g_free (tmp);
    }

  return ligma_sysconf_dir;
}

/**
 * ligma_plug_in_directory:
 *
 * Returns the default top directory for LIGMA plug-ins and modules. If
 * the environment variable LIGMA3_PLUGINDIR exists, that is used.  It
 * should be an absolute pathname. Otherwise, on Unix the compile-time
 * defined directory is used. On Windows, the installation directory
 * as deduced from the executable's full filename is used.
 *
 * Note that the actual directories used for LIGMA plug-ins and modules
 * can be overridden by the user in the preferences dialog.
 *
 * In config files such as ligmarc, the string ${ligma_plug_in_dir}
 * expands to this directory.
 *
 * The returned string is owned by LIGMA and must not be modified or
 * freed. The returned string is in the encoding used for filenames by
 * GLib, which isn't necessarily UTF-8. (On Windows it always is
 * UTF-8.)
 *
 * Returns: The top directory for LIGMA plug_ins and modules.
 **/
const gchar *
ligma_plug_in_directory (void)
{
  static gchar *ligma_plug_in_dir = NULL;

  if (! ligma_plug_in_dir)
    {
      gchar *tmp = g_build_filename ("lib",
                                     LIGMA_PACKAGE,
                                     LIGMA_PLUGIN_VERSION,
                                     NULL);

      ligma_plug_in_dir = ligma_env_get_dir ("LIGMA3_PLUGINDIR", PLUGINDIR, tmp);
      g_free (tmp);
    }

  return ligma_plug_in_dir;
}

/**
 * ligma_cache_directory:
 *
 * Returns the default top directory for LIGMA cached files. If the
 * environment variable LIGMA3_CACHEDIR exists, that is used.  It
 * should be an absolute pathname.  Otherwise, a subdirectory of the
 * directory returned by g_get_user_cache_dir() is used.
 *
 * Note that the actual directories used for LIGMA caches files can
 * be overridden by the user in the preferences dialog.
 *
 * In config files such as ligmarc, the string ${ligma_cache_dir}
 * expands to this directory.
 *
 * The returned string is owned by LIGMA and must not be modified or
 * freed. The returned string is in the encoding used for filenames by
 * GLib, which isn't necessarily UTF-8. (On Windows it always is
 * UTF-8.).
 *
 * Since: 2.10.10
 *
 * Returns: The default top directory for LIGMA cached files.
 **/
const gchar *
ligma_cache_directory (void)
{
  static gchar *ligma_cache_dir = NULL;

  if (! ligma_cache_dir)
    {
      gchar *tmp = g_build_filename (g_get_user_cache_dir (),
                                     LIGMA_PACKAGE,
                                     LIGMA_USER_VERSION,
                                     NULL);

      ligma_cache_dir = ligma_env_get_dir ("LIGMA3_CACHEDIR", NULL, tmp);
      g_free (tmp);
    }

  return ligma_cache_dir;
}

/**
 * ligma_temp_directory:
 *
 * Returns the default top directory for LIGMA temporary files. If the
 * environment variable LIGMA3_TEMPDIR exists, that is used.  It
 * should be an absolute pathname.  Otherwise, a subdirectory of the
 * directory returned by g_get_tmp_dir() is used.
 *
 * In config files such as ligmarc, the string ${ligma_temp_dir} expands
 * to this directory.
 *
 * Note that the actual directories used for LIGMA temporary files can
 * be overridden by the user in the preferences dialog.
 *
 * The returned string is owned by LIGMA and must not be modified or
 * freed. The returned string is in the encoding used for filenames by
 * GLib, which isn't necessarily UTF-8. (On Windows it always is
 * UTF-8.).
 *
 * Since: 2.10.10
 *
 * Returns: The default top directory for LIGMA temporary files.
 **/
const gchar *
ligma_temp_directory (void)
{
  static gchar *ligma_temp_dir = NULL;

  if (! ligma_temp_dir)
    {
      gchar *tmp = g_build_filename (g_get_tmp_dir (),
                                     LIGMA_PACKAGE,
                                     LIGMA_USER_VERSION,
                                     NULL);

      ligma_temp_dir = ligma_env_get_dir ("LIGMA3_TEMPDIR", NULL, tmp);
      g_free (tmp);
    }

  return ligma_temp_dir;
}

static GFile *
ligma_child_file (const gchar *parent,
                 const gchar *element,
                 va_list      args)
{
  GFile *file = g_file_new_for_path (parent);

  while (element)
    {
      GFile *child = g_file_get_child (file, element);

      g_object_unref (file);
      file = child;

      element = va_arg (args, const gchar *);
    }

  return file;
}

/**
 * ligma_directory_file: (skip)
 * @first_element: the first element of a path to a file in the
 *                 user's LIGMA directory, or %NULL.
 * @...: a %NULL terminated list of the remaining elements of the path
 *       to the file.
 *
 * Returns a #GFile in the user's LIGMA directory, or the LIGMA
 * directory itself if @first_element is %NULL.
 *
 * See also: ligma_directory().
 *
 * Since: 2.10
 *
 * Returns: (transfer full):
 *          a new @GFile for the path, Free with g_object_unref().
 **/
GFile *
ligma_directory_file (const gchar *first_element,
                     ...)
{
  GFile   *file;
  va_list  args;

  va_start (args, first_element);
  file = ligma_child_file (ligma_directory (), first_element, args);
  va_end (args);

  return file;
}

/**
 * ligma_installation_directory_file: (skip)
 * @first_element: the first element of a path to a file in the
 *                 top installation directory, or %NULL.
 * @...: a %NULL terminated list of the remaining elements of the path
 *       to the file.
 *
 * Returns a #GFile in the installation directory, or the installation
 * directory itself if @first_element is %NULL.
 *
 * See also: ligma_installation_directory().
 *
 * Since: 2.10.10
 *
 * Returns: (transfer full):
 *          a new @GFile for the path, Free with g_object_unref().
 **/
GFile *
ligma_installation_directory_file (const gchar *first_element,
                                  ...)
{
  GFile   *file;
  va_list  args;

  va_start (args, first_element);
  file = ligma_child_file (ligma_installation_directory (), first_element, args);
  va_end (args);

  return file;
}

/**
 * ligma_data_directory_file: (skip)
 * @first_element: the first element of a path to a file in the
 *                 data directory, or %NULL.
 * @...: a %NULL terminated list of the remaining elements of the path
 *       to the file.
 *
 * Returns a #GFile in the data directory, or the data directory
 * itself if @first_element is %NULL.
 *
 * See also: ligma_data_directory().
 *
 * Since: 2.10
 *
 * Returns: (transfer full):
 *          a new @GFile for the path, Free with g_object_unref().
 **/
GFile *
ligma_data_directory_file (const gchar *first_element,
                          ...)
{
  GFile   *file;
  va_list  args;

  va_start (args, first_element);
  file = ligma_child_file (ligma_data_directory (), first_element, args);
  va_end (args);

  return file;
}

/**
 * ligma_locale_directory_file: (skip)
 * @first_element: the first element of a path to a file in the
 *                 locale directory, or %NULL.
 * @...: a %NULL terminated list of the remaining elements of the path
 *       to the file.
 *
 * Returns a #GFile in the locale directory, or the locale directory
 * itself if @first_element is %NULL.
 *
 * See also: ligma_locale_directory().
 *
 * Since: 2.10
 *
 * Returns: (transfer full):
 *          a new @GFile for the path, Free with g_object_unref().
 **/
GFile *
ligma_locale_directory_file (const gchar *first_element,
                            ...)
{
  GFile   *file;
  va_list  args;

  va_start (args, first_element);
  file = ligma_child_file (ligma_locale_directory (), first_element, args);
  va_end (args);

  return file;
}

/**
 * ligma_sysconf_directory_file:
 * @first_element: the first element of a path to a file in the
 *                 sysconf directory, or %NULL.
 * @...: a %NULL terminated list of the remaining elements of the path
 *       to the file.
 *
 * Returns a #GFile in the sysconf directory, or the sysconf directory
 * itself if @first_element is %NULL.
 *
 * See also: ligma_sysconf_directory().
 *
 * Since: 2.10
 *
 * Returns: (transfer full):
 *          a new @GFile for the path, Free with g_object_unref().
 **/
GFile *
ligma_sysconf_directory_file (const gchar *first_element,
                             ...)
{
  GFile   *file;
  va_list  args;

  va_start (args, first_element);
  file = ligma_child_file (ligma_sysconf_directory (), first_element, args);
  va_end (args);

  return file;
}

/**
 * ligma_plug_in_directory_file:
 * @first_element: the first element of a path to a file in the
 *                 plug-in directory, or %NULL.
 * @...: a %NULL terminated list of the remaining elements of the path
 *       to the file.
 *
 * Returns a #GFile in the plug-in directory, or the plug-in directory
 * itself if @first_element is %NULL.
 *
 * See also: ligma_plug_in_directory().
 *
 * Since: 2.10
 *
 * Returns: (transfer full):
 *          a new @GFile for the path, Free with g_object_unref().
 **/
GFile *
ligma_plug_in_directory_file (const gchar *first_element,
                             ...)
{
  GFile   *file;
  va_list  args;

  va_start (args, first_element);
  file = ligma_child_file (ligma_plug_in_directory (), first_element, args);
  va_end (args);

  return file;
}

/**
 * ligma_path_runtime_fix:
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
ligma_path_runtime_fix (gchar **path)
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
      *path = g_strconcat (ligma_installation_directory (),
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
      *path = g_build_filename (ligma_installation_directory (), *path, NULL);
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
      *path = g_build_filename (ligma_installation_directory (),
                                *path + strlen (PREFIX G_DIR_SEPARATOR_S),
                                NULL);
      g_free (p);
    }
#endif
}

/**
 * ligma_path_parse:
 * @path:         A list of directories separated by #G_SEARCHPATH_SEPARATOR.
 * @max_paths:    The maximum number of directories to return.
 * @check:        %TRUE if you want the directories to be checked.
 * @check_failed: (element-type filename) (out callee-allocates):
                  Returns a #GList of path elements for which the check failed.
 *
 * Returns: (element-type filename) (transfer full):
            A #GList of all directories in @path.
 **/
GList *
ligma_path_parse (const gchar  *path,
                 gint          max_paths,
                 gboolean      check,
                 GList       **check_failed)
{
  gchar    **patharray;
  GList     *list      = NULL;
  GList     *fail_list = NULL;
  gint       i;
  gboolean   exists    = TRUE;

  if (!path || !*path || max_paths < 1 || max_paths > 256)
    return NULL;

  patharray = g_strsplit (path, G_SEARCHPATH_SEPARATOR_S, max_paths);

  for (i = 0; i < max_paths; i++)
    {
      GString *dir;

      if (! patharray[i])
        break;

#ifndef G_OS_WIN32
      if (*patharray[i] == '~')
        {
          dir = g_string_new (g_get_home_dir ());
          g_string_append (dir, patharray[i] + 1);
        }
      else
#endif
        {
          ligma_path_runtime_fix (&patharray[i]);
          dir = g_string_new (patharray[i]);
        }

      if (check)
        exists = g_file_test (dir->str, G_FILE_TEST_IS_DIR);

      if (exists)
        {
          GList *dup;

          /*  check for duplicate entries, see bug #784502  */
          for (dup = list; dup; dup = g_list_next (dup))
            {
              if (! strcmp (dir->str, dup->data))
                break;
            }

          /*  only add to the list if it's not a duplicate  */
          if (! dup)
            list = g_list_prepend (list, g_strdup (dir->str));
        }
      else if (check_failed)
        {
          fail_list = g_list_prepend (fail_list, g_strdup (dir->str));
        }

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
 * ligma_path_to_str:
 * @path: (element-type filename):
 *        A list of directories as returned by ligma_path_parse().
 *
 * Returns: (type filename) (transfer full):
 *          A searchpath string separated by #G_SEARCHPATH_SEPARATOR.
 **/
gchar *
ligma_path_to_str (GList *path)
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
 * ligma_path_free:
 * @path: (element-type filename):
 *        A list of directories as returned by ligma_path_parse().
 *
 * This function frees the memory allocated for the list and the strings
 * it contains.
 **/
void
ligma_path_free (GList *path)
{
  g_list_free_full (path, (GDestroyNotify) g_free);
}

/**
 * ligma_path_get_user_writable_dir:
 * @path: (element-type filename):
 *        A list of directories as returned by ligma_path_parse().
 *
 * Note that you have to g_free() the returned string.
 *
 * Returns: The first directory in @path where the user has write permission.
 **/
gchar *
ligma_path_get_user_writable_dir (GList *path)
{
  GList    *list;
  uid_t     euid;
  gid_t     egid;
  GStatBuf  filestat;
  gint      err;

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
       *  of their group's or other's write permissions
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
ligma_env_get_dir (const gchar *ligma_env_name,
                  const gchar *compile_time_dir,
                  const gchar *relative_subdir)
{
  const gchar *env = g_getenv (ligma_env_name);

  if (env)
    {
      if (! g_path_is_absolute (env))
        g_error ("%s environment variable should be an absolute path.",
                 ligma_env_name);

      return g_strdup (env);
    }
  else if (compile_time_dir)
    {
      gchar *retval = g_strdup (compile_time_dir);

      ligma_path_runtime_fix (&retval);

      return retval;
    }
  else if (! g_path_is_absolute (relative_subdir))
    {
      return g_build_filename (ligma_installation_directory (),
                               relative_subdir,
                               NULL);
    }

  return g_strdup (relative_subdir);
}
