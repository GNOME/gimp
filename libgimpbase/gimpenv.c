/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * gimpenv.c
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
 * <http://www.gnu.org/licenses/>.
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

#undef GIMP_DISABLE_DEPRECATED
#include "gimpbasetypes.h"

#define __GIMP_ENV_C__
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

/* This is a hack for Windows known directory support.
 * DATADIR (autotools-generated constant) is a type defined in objidl.h
 * so we must #undef it before including shlobj.h in order to avoid a
 * name clash. */
static const char* datadir = DATADIR;
#undef DATADIR
#include <shlobj.h>
#define DATADIR datadir
/* Constant available since Shell32.dll 4.72 */
#ifndef CSIDL_APPDATA
#define CSIDL_APPDATA 0x001a
#endif

#endif


/**
 * SECTION: gimpenv
 * @title: gimpenv
 * @short_description: Functions to access the GIMP environment.
 *
 * A set of functions to find the locations of GIMP's data directories
 * and configuration files.
 **/


static gchar * gimp_env_get_dir   (const gchar *gimp_env_name,
                                   const gchar *compile_time_dir,
                                   const gchar *relative_subdir);
#ifdef G_OS_WIN32
static gchar * get_special_folder (gint         csidl);
#endif


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
 * Since: 2.4
 */
void
gimp_env_init (gboolean plug_in)
{
  static gboolean  gimp_env_initialized = FALSE;
  const gchar     *data_home = g_get_user_data_dir ();

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
      gchar       *libdir = g_build_filename (gimp_installation_directory (),
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
 * gimp_directory:
 *
 * Returns the user-specific GIMP settings directory. If the
 * environment variable GIMP3_DIRECTORY exists, it is used. If it is
 * an absolute path, it is used as is.  If it is a relative path, it
 * is taken to be a subdirectory of the home directory. If it is a
 * relative path, and no home directory can be determined, it is taken
 * to be a subdirectory of gimp_data_directory().
 *
 * The usual case is that no GIMP3_DIRECTORY environment variable
 * exists, and then we use the GIMPDIR subdirectory of the local
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
 * The returned string is owned by GIMP and must not be modified or
 * freed. The returned string is in the encoding used for filenames by
 * GLib, which isn't necessarily UTF-8 (on Windows it is always
 * UTF-8.)
 *
 * Returns: The user-specific GIMP settings directory.
 **/
const gchar *
gimp_directory (void)
{
  static gchar *gimp_dir          = NULL;
  static gchar *last_env_gimp_dir = NULL;

  const gchar  *env_gimp_dir;

  env_gimp_dir = g_getenv ("GIMP3_DIRECTORY");

  if (gimp_dir)
    {
      gboolean gimp3_directory_changed = FALSE;

      /* We have constructed the gimp_dir already. We can return
       * gimp_dir unless some parameter gimp_dir depends on has
       * changed. For now we just check for changes to GIMP3_DIRECTORY
       */
      gimp3_directory_changed =
        (env_gimp_dir == NULL &&
         last_env_gimp_dir != NULL) ||
        (env_gimp_dir != NULL &&
         last_env_gimp_dir == NULL) ||
        (env_gimp_dir != NULL &&
         last_env_gimp_dir != NULL &&
         strcmp (env_gimp_dir, last_env_gimp_dir) != 0);

      if (! gimp3_directory_changed)
        {
          return gimp_dir;
        }
      else
        {
          /* Free the old gimp_dir and go on to update it */
          g_free (gimp_dir);
          gimp_dir = NULL;
        }
    }

  /* Remember the GIMP3_DIRECTORY to next invocation so we can check
   * if it changes
   */
  g_free (last_env_gimp_dir);
  last_env_gimp_dir = g_strdup (env_gimp_dir);

  if (env_gimp_dir)
    {
      if (g_path_is_absolute (env_gimp_dir))
        {
          gimp_dir = g_strdup (env_gimp_dir);
        }
      else
        {
          const gchar *home_dir = g_get_home_dir ();

          if (home_dir)
            gimp_dir = g_build_filename (home_dir, env_gimp_dir, NULL);
          else
            gimp_dir = g_build_filename (gimp_data_directory (), env_gimp_dir, NULL);
        }
    }
  else if (g_path_is_absolute (GIMPDIR))
    {
      gimp_dir = g_strdup (GIMPDIR);
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

      gimp_dir = g_build_filename ([library_dir UTF8String],
                                   GIMPDIR, GIMP_USER_VERSION, NULL);

      [pool drain];

#elif defined G_OS_WIN32

      gchar *conf_dir = get_special_folder (CSIDL_APPDATA);

      gimp_dir = g_build_filename (conf_dir,
                                   GIMPDIR, GIMP_USER_VERSION, NULL);
      g_free(conf_dir);

#else /* UNIX */

      /* g_get_user_config_dir () always returns a path as a non-null
       * and non-empty string
       */
      gimp_dir = g_build_filename (g_get_user_config_dir (),
                                   GIMPDIR, GIMP_USER_VERSION, NULL);

#endif /* PLATFORM_OSX */
    }

  return gimp_dir;
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

static HMODULE libgimpbase_dll = NULL;

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
      libgimpbase_dll = hinstDLL;
      break;
    }

  return TRUE;
}

#endif

/**
 * gimp_installation_directory:
 *
 * Returns the top installation directory of GIMP. On Unix the
 * compile-time defined installation prefix is used. On Windows, the
 * installation directory as deduced from the executable's full
 * filename is used. On OSX we ask [NSBundle mainBundle] for the
 * resource path to check if GIMP is part of a relocatable bundle.
 *
 * The returned string is owned by GIMP and must not be modified or
 * freed. The returned string is in the encoding used for filenames by
 * GLib, which isn't necessarily UTF-8. (On Windows it always is
 * UTF-8.)
 *
 * Since: 2.8
 *
 * Returns: The toplevel installation directory of GIMP.
 **/
const gchar *
gimp_installation_directory (void)
{
  static gchar *toplevel = NULL;

  if (toplevel)
    return toplevel;

#ifdef G_OS_WIN32

  toplevel = g_win32_get_package_installation_directory_of_module (libgimpbase_dll);
  if (! toplevel)
    g_error ("g_win32_get_package_installation_directory_of_module() failed");

#elif PLATFORM_OSX

  {
    NSAutoreleasePool *pool;
    NSString          *resource_path;
    gchar             *basename;
    gchar             *dirname;

    pool = [[NSAutoreleasePool alloc] init];

    resource_path = [[NSBundle mainBundle] resourcePath];

    basename = g_path_get_basename ([resource_path UTF8String]);
    dirname  = g_path_get_dirname ([resource_path UTF8String]);

    if (! strcmp (basename, ".libs"))
      {
        /*  we are running from the source dir, do normal unix things  */

        toplevel = _gimp_reloc_find_prefix (PREFIX);
      }
    else if (! strcmp (basename, "bin"))
      {
        /*  we are running the main app, but not from a bundle, the resource
         *  path is the directory which contains the executable
         */

        toplevel = g_strdup (dirname);
      }
    else if (! strcmp (basename, "plug-ins"))
      {
        /*  same for plug-ins, go three levels up from prefix/lib/gimp/x.y  */

        gchar *tmp  = g_path_get_dirname (dirname);
        gchar *tmp2 = g_path_get_dirname (tmp);

        toplevel = g_path_get_dirname (tmp2);

        g_free (tmp);
        g_free (tmp2);
      }
    else
      {
        /*  if none of the above match, we assume that we are really in a bundle  */

        toplevel = g_strdup ([resource_path UTF8String]);
      }

    g_free (basename);
    g_free (dirname);

    [pool drain];
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
 * variable GIMP3_DATADIR exists, that is used.  It should be an
 * absolute pathname.  Otherwise, on Unix the compile-time defined
 * directory is used. On Windows, the installation directory as deduced
 * from the executable's full filename is used.
 *
 * The returned string is owned by GIMP and must not be modified or
 * freed. The returned string is in the encoding used for filenames by
 * GLib, which isn't necessarily UTF-8. (On Windows it always is
 * UTF-8.)
 *
 * Returns: The top directory for GIMP data.
 **/
const gchar *
gimp_data_directory (void)
{
  static gchar *gimp_data_dir = NULL;

  if (! gimp_data_dir)
    {
      gchar *tmp = g_build_filename ("share",
                                     GIMP_PACKAGE,
                                     GIMP_DATA_VERSION,
                                     NULL);

      gimp_data_dir = gimp_env_get_dir ("GIMP3_DATADIR", DATADIR, tmp);
      g_free (tmp);
    }

  return gimp_data_dir;
}

/**
 * gimp_locale_directory:
 *
 * Returns the top directory for GIMP locale files. If the environment
 * variable GIMP3_LOCALEDIR exists, that is used.  It should be an
 * absolute pathname.  Otherwise, on Unix the compile-time defined
 * directory is used. On Windows, the installation directory as deduced
 * from the executable's full filename is used.
 *
 * The returned string is owned by GIMP and must not be modified or
 * freed. The returned string is in the encoding used for filenames by
 * the C library, which isn't necessarily UTF-8. (On Windows, unlike
 * the other similar functions here, the return value from this
 * function is in the system codepage, never in UTF-8. It can thus be
 * passed directly to the bindtextdomain() function from libintl which
 * does not handle UTF-8.)
 *
 * Returns: The top directory for GIMP locale files.
 */
const gchar *
gimp_locale_directory (void)
{
  static gchar *gimp_locale_dir = NULL;

  if (! gimp_locale_dir)
    {
      gchar *tmp = g_build_filename ("share",
                                     "locale",
                                     NULL);

      gimp_locale_dir = gimp_env_get_dir ("GIMP3_LOCALEDIR", LOCALEDIR, tmp);
      g_free (tmp);

#ifdef G_OS_WIN32
      tmp = g_win32_locale_filename_from_utf8 (gimp_locale_dir);
      g_free (gimp_locale_dir);
      gimp_locale_dir = tmp;
#endif
    }

  return gimp_locale_dir;
}

/**
 * gimp_sysconf_directory:
 *
 * Returns the top directory for GIMP config files. If the environment
 * variable GIMP3_SYSCONFDIR exists, that is used.  It should be an
 * absolute pathname.  Otherwise, on Unix the compile-time defined
 * directory is used. On Windows, the installation directory as deduced
 * from the executable's full filename is used.
 *
 * The returned string is owned by GIMP and must not be modified or
 * freed. The returned string is in the encoding used for filenames by
 * GLib, which isn't necessarily UTF-8. (On Windows it always is
 * UTF-8.).
 *
 * Returns: The top directory for GIMP config files.
 **/
const gchar *
gimp_sysconf_directory (void)
{
  static gchar *gimp_sysconf_dir = NULL;

  if (! gimp_sysconf_dir)
    {
      gchar *tmp = g_build_filename ("etc",
                                     GIMP_PACKAGE,
                                     GIMP_SYSCONF_VERSION,
                                     NULL);

      gimp_sysconf_dir = gimp_env_get_dir ("GIMP3_SYSCONFDIR", SYSCONFDIR, tmp);
      g_free (tmp);
    }

  return gimp_sysconf_dir;
}

/**
 * gimp_plug_in_directory:
 *
 * Returns the top directory for GIMP plug_ins and modules. If the
 * environment variable GIMP3_PLUGINDIR exists, that is used.  It
 * should be an absolute pathname. Otherwise, on Unix the compile-time
 * defined directory is used. On Windows, the installation directory as
 * deduced from the executable's full filename is used.
 *
 * The returned string is owned by GIMP and must not be modified or
 * freed. The returned string is in the encoding used for filenames by
 * GLib, which isn't necessarily UTF-8. (On Windows it always is
 * UTF-8.)
 *
 * Returns: The top directory for GIMP plug_ins and modules.
 **/
const gchar *
gimp_plug_in_directory (void)
{
  static gchar *gimp_plug_in_dir = NULL;

  if (! gimp_plug_in_dir)
    {
      gchar *tmp = g_build_filename ("lib",
                                     GIMP_PACKAGE,
                                     GIMP_PLUGIN_VERSION,
                                     NULL);

      gimp_plug_in_dir = gimp_env_get_dir ("GIMP3_PLUGINDIR", PLUGINDIR, tmp);
      g_free (tmp);
    }

  return gimp_plug_in_dir;
}

static GFile *
gimp_child_file (const gchar *parent,
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
 * gimp_directory_file:
 * @first_element: the first element of a path to a file in the
 *                 user's GIMP directory, or %NULL.
 * @...: a %NULL terminated list of the remaining elements of the path
 *       to the file.
 *
 * Returns a #GFile in the user's GIMP directory, or the data
 * directory itself if @first_element is %NULL.
 *
 * See also: gimp_directory().
 *
 * Since: 2.10
 *
 * Returns: a new @GFile for the path, Free with g_object_unref().
 **/
GFile *
gimp_directory_file (const gchar *first_element,
                     ...)
{
  GFile   *file;
  va_list  args;

  va_start (args, first_element);
  file = gimp_child_file (gimp_directory (), first_element, args);
  va_end (args);

  return file;
}

/**
 * gimp_data_directory_file:
 * @first_element: the first element of a path to a file in the
 *                 data directory, or %NULL.
 * @...: a %NULL terminated list of the remaining elements of the path
 *       to the file.
 *
 * Returns a #GFile in the data directory, or the data directory
 * itself if @first_element is %NULL.
 *
 * See also: gimp_data_directory().
 *
 * Since: 2.10
 *
 * Returns: a new @GFile for the path, Free with g_object_unref().
 **/
GFile *
gimp_data_directory_file (const gchar *first_element,
                          ...)
{
  GFile   *file;
  va_list  args;

  va_start (args, first_element);
  file = gimp_child_file (gimp_data_directory (), first_element, args);
  va_end (args);

  return file;
}

/**
 * gimp_locale_directory_file:
 * @first_element: the first element of a path to a file in the
 *                 locale directory, or %NULL.
 * @...: a %NULL terminated list of the remaining elements of the path
 *       to the file.
 *
 * Returns a #GFile in the locale directory, or the locale directory
 * itself if @first_element is %NULL.
 *
 * See also: gimp_locale_directory().
 *
 * Since: 2.10
 *
 * Returns: a new @GFile for the path, Free with g_object_unref().
 **/
GFile *
gimp_locale_directory_file (const gchar *first_element,
                            ...)
{
  GFile   *file;
  va_list  args;

  va_start (args, first_element);
  file = gimp_child_file (gimp_locale_directory (), first_element, args);
  va_end (args);

  return file;
}

/**
 * gimp_sysconf_directory_file:
 * @first_element: the first element of a path to a file in the
 *                 sysconf directory, or %NULL.
 * @...: a %NULL terminated list of the remaining elements of the path
 *       to the file.
 *
 * Returns a #GFile in the sysconf directory, or the sysconf directory
 * itself if @first_element is %NULL.
 *
 * See also: gimp_sysconf_directory().
 *
 * Since: 2.10
 *
 * Returns: a new @GFile for the path, Free with g_object_unref().
 **/
GFile *
gimp_sysconf_directory_file (const gchar *first_element,
                             ...)
{
  GFile   *file;
  va_list  args;

  va_start (args, first_element);
  file = gimp_child_file (gimp_sysconf_directory (), first_element, args);
  va_end (args);

  return file;
}

/**
 * gimp_plug_in_directory_file:
 * @first_element: the first element of a path to a file in the
 *                 plug-in directory, or %NULL.
 * @...: a %NULL terminated list of the remaining elements of the path
 *       to the file.
 *
 * Returns a #GFile in the plug-in directory, or the plug-in directory
 * itself if @first_element is %NULL.
 *
 * See also: gimp_plug_in_directory().
 *
 * Since: 2.10
 *
 * Returns: a new @GFile for the path, Free with g_object_unref().
 **/
GFile *
gimp_plug_in_directory_file (const gchar *first_element,
                             ...)
{
  GFile   *file;
  va_list  args;

  va_start (args, first_element);
  file = gimp_child_file (gimp_plug_in_directory (), first_element, args);
  va_end (args);

  return file;
}

/**
 * gimp_user_directory:
 * @type: the type of user directory to retrieve
 *
 * This procedure is deprecated! Use g_get_user_special_dir() instead.
 *
 * Returns: The path to the specified user directory, or %NULL if the
 *          logical ID was not found.
 *
 * Since: 2.4
 **/
const gchar *
gimp_user_directory (GimpUserDirectory type)
{
  return g_get_user_special_dir ((GUserDirectory) type);
}

/**
 * gimp_personal_rc_file:
 * @basename: The basename of a rc_file.
 *
 * Returns the name of a file in the user-specific GIMP settings directory.
 *
 * The returned string is newly allocated and should be freed with
 * g_free() after use. The returned string is in the encoding used for
 * filenames by GLib, which isn't necessarily UTF-8. (On Windows it
 * always is UTF-8.)
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
 * Returns the name of GIMP's application-specific gtkrc file.
 *
 * The returned string is owned by GIMP and must not be modified or
 * freed. The returned string is in the encoding used for filenames by
 * GLib, which isn't necessarily UTF-8. (On Windows it always is
 * UTF-8.)
 *
 * Returns: The name of GIMP's application-specific gtkrc file.
 **/
const gchar *
gimp_gtkrc (void)
{
  static gchar *gimp_gtkrc_filename = NULL;

  if (! gimp_gtkrc_filename)
    gimp_gtkrc_filename = g_build_filename (gimp_data_directory (),
                                            "themes", "System", "gtkrc",
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
      *path = g_strconcat (gimp_installation_directory (),
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
      *path = g_build_filename (gimp_installation_directory (), *path, NULL);
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
      *path = g_build_filename (gimp_installation_directory (),
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
  g_list_free_full (path, (GDestroyNotify) g_free);
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
                  const gchar *compile_time_dir,
                  const gchar *relative_subdir)
{
  const gchar *env = g_getenv (gimp_env_name);

  if (env)
    {
      if (! g_path_is_absolute (env))
        g_error ("%s environment variable should be an absolute path.",
                 gimp_env_name);

      return g_strdup (env);
    }
  else if (compile_time_dir)
    {
      gchar *retval = g_strdup (compile_time_dir);

      gimp_path_runtime_fix (&retval);

      return retval;
    }

  return g_build_filename (gimp_installation_directory (),
                           relative_subdir,
                           NULL);
}
