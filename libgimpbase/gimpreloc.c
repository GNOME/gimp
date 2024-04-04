/*
 * BinReloc - a library for creating relocatable executables
 * Written by: Hongli Lai <h.lai@chello.nl>
 * http://autopackage.org/
 *
 * This source code is public domain. You can relicense this code
 * under whatever license you want.
 *
 * See http://autopackage.org/docs/binreloc/ for
 * more information and how to use this.
 */

#include "config.h"

#include <stdlib.h>
#include <limits.h>
#include <string.h>

#if defined(ENABLE_RELOCATABLE_RESOURCES) && ! defined(G_OS_WIN32)
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#endif /* ENABLE_RELOCATABLE_RESOURCES && ! G_OS_WIN32 */

#include <gio/gio.h>
#include <glib.h>
#include <glib/gstdio.h>

#include "gimpreloc.h"


/*
 * Find the canonical filename of the executable. Returns the filename
 * (which must be freed) or NULL on error. If the parameter 'error' is
 * not NULL, the error code will be stored there, if an error occurred.
 */
static char *
_br_find_exe (GimpBinrelocInitError *error)
{
#if ! defined(ENABLE_RELOCATABLE_RESOURCES) || defined(G_OS_WIN32) || defined(__APPLE__)
  if (error)
    *error = GIMP_RELOC_INIT_ERROR_DISABLED;
  return NULL;
#else
  GDataInputStream *data_input;
  GInputStream     *input;
  GFile            *file;
  GError           *gerror = NULL;
  gchar            *path;
  gchar            *sym_path;
  gchar            *maps_line;

  sym_path = g_strdup ("/proc/self/exe");

  while (1)
    {
      struct stat stat_buf;
      int         i;

      /* Do not use readlink() with a buffer of size PATH_MAX because
       * some systems actually allow paths of bigger size. Thus this
       * macro is kind of bogus. Some systems like Hurd will not even
       * define it (see MR !424).
       * g_file_read_link() on the other hand will return a size of
       * appropriate size, with newline removed and NUL terminator
       * added.
       */
      path = g_file_read_link (sym_path, &gerror);
      g_free (sym_path);
      if (! path)
        {
          /* Read link fails but we can try reading /proc/self/maps as
           * an alternate method.
           */
          g_printerr ("%s: %s\n", G_STRFUNC, gerror->message);
          g_clear_error (&gerror);

          break;
        }

      /* Check whether the symlink's target is also a symlink.
       * We want to get the final target. */
      i = stat (path, &stat_buf);
      if (i == -1)
        {
          /* Error. */
          break;
        }

      /* stat() success. */
      if (! S_ISLNK (stat_buf.st_mode))
        {
          /* path is not a symlink. Done. */
          return path;
        }

      /* path is a symlink. Continue loop and resolve this. */
      sym_path = path;
    }

  /* readlink() or stat() failed; this can happen when the program is
   * running in Valgrind 2.2. Read from /proc/self/maps as fallback. */

  file = g_file_new_for_path ("/proc/self/maps");
  input = G_INPUT_STREAM (g_file_read (file, NULL, &gerror));
  g_object_unref (file);
  if (! input)
    {
      g_printerr ("%s: %s", G_STRFUNC, gerror->message);
      g_clear_error (&gerror);

      if (error)
        *error = GIMP_RELOC_INIT_ERROR_OPEN_MAPS;

      return NULL;
    }

  data_input = g_data_input_stream_new (input);
  g_object_unref (input);

  /* The first entry with r-xp permission should be the executable name. */
  while ((maps_line = g_data_input_stream_read_line (data_input, NULL, NULL, &gerror)))
    {
      if (maps_line == NULL)
        {
          if (gerror)
            {
              g_printerr ("%s: %s\n", G_STRFUNC, gerror->message);
              g_error_free (gerror);
            }
          g_object_unref (data_input);

          if (error)
            *error = GIMP_RELOC_INIT_ERROR_READ_MAPS;

          return NULL;
        }

      /* Extract the filename; it is always an absolute path. */
      path = strchr (maps_line, '/');

      /* Sanity check. */
      if (path && strstr (maps_line, " r-xp "))
        {
          /* We found the executable name. */
          path = g_strdup (path);
          break;
        }

      g_free (maps_line);
      maps_line = NULL;
      path = NULL;
    }

  if (path == NULL && error)
    *error = GIMP_RELOC_INIT_ERROR_INVALID_MAPS;

  g_object_unref (data_input);
  g_free (maps_line);

  return path;
#endif /* ! ENABLE_RELOCATABLE_RESOURCES || G_OS_WIN32 */
}


/*
 * Find the canonical filename of the executable which owns symbol.
 * Returns a filename which must be freed, or NULL on error.
 */
static char *
_br_find_exe_for_symbol (const void *symbol, GimpBinrelocInitError *error)
{
#if ! defined(ENABLE_RELOCATABLE_RESOURCES) || defined(G_OS_WIN32) || defined(__APPLE__)
  if (error)
    *error = GIMP_RELOC_INIT_ERROR_DISABLED;
  return (char *) NULL;
#else
  GDataInputStream *data_input;
  GInputStream     *input;
  GFile            *file;
  GError           *gerror = NULL;
  gchar            *maps_line;
  char             *found = NULL;
  char             *address_string;
  size_t            address_string_len;

  if (symbol == NULL)
    return (char *) NULL;

  file = g_file_new_for_path ("/proc/self/maps");
  input = G_INPUT_STREAM (g_file_read (file, NULL, &gerror));
  g_object_unref (file);
  if (! input)
    {
      g_printerr ("%s: %s", G_STRFUNC, gerror->message);
      g_error_free (gerror);

      if (error)
        *error = GIMP_RELOC_INIT_ERROR_OPEN_MAPS;

      return NULL;
    }

  data_input = g_data_input_stream_new (input);
  g_object_unref (input);

  address_string_len = 4;
  address_string = g_try_new (char, address_string_len);

  while ((maps_line = g_data_input_stream_read_line (data_input, NULL, NULL, &gerror)))
    {
      char   *start_addr, *end_addr, *end_addr_end;
      char   *path;
      void   *start_addr_p, *end_addr_p;
      size_t  len;

      if (maps_line == NULL)
        {
          if (gerror)
            {
              g_printerr ("%s: %s\n", G_STRFUNC, gerror->message);
              g_error_free (gerror);
            }

          if (error)
            *error = GIMP_RELOC_INIT_ERROR_READ_MAPS;

          break;
        }

      /* Sanity check. */
      /* XXX Early versions of this code would check that the mapped
       * region was with r-xp permission. It might have been true at
       * some point in time, but last I tested, the searched pointer was
       * in a r--p region for libgimpbase. Thus _br_find_exe_for_symbol()
       * would fail to find the executable's path.
       * So now we don't test the region's permission anymore.
       */
      if (strchr (maps_line, '/') == NULL)
        {
          g_free (maps_line);
          continue;
        }

      /* Parse line. */
      start_addr = maps_line;
      end_addr = strchr (maps_line, '-');
      path = strchr (maps_line, '/');

      /* More sanity check. */
      if (!(path > end_addr && end_addr != NULL && end_addr[0] == '-'))
        {
          g_free (maps_line);
          continue;
        }

      end_addr[0] = '\0';
      end_addr++;
      end_addr_end = strchr (end_addr, ' ');
      if (end_addr_end == NULL)
        {
          g_free (maps_line);
          continue;
        }

      end_addr_end[0] = '\0';
      len = strlen (path);
      if (len == 0)
        {
          g_free (maps_line);
          continue;
        }
      if (path[len - 1] == '\n')
        path[len - 1] = '\0';

      /* Get rid of "(deleted)" from the filename. */
      len = strlen (path);
      if (len > 10 && strcmp (path + len - 10, " (deleted)") == 0)
        path[len - 10] = '\0';

      /* I don't know whether this can happen but better safe than sorry. */
      len = strlen (start_addr);
      if (len != strlen (end_addr))
        {
          g_free (maps_line);
          continue;
        }

      /* Transform the addresses into a string in the form of 0xdeadbeef,
       * then transform that into a pointer. */
      if (address_string_len < len + 3)
        {
          address_string_len = len + 3;
          address_string = (char *) g_try_realloc (address_string, address_string_len);
        }

      memcpy (address_string, "0x", 2);
      memcpy (address_string + 2, start_addr, len);
      address_string[2 + len] = '\0';
      sscanf (address_string, "%p", &start_addr_p);

      memcpy (address_string, "0x", 2);
      memcpy (address_string + 2, end_addr, len);
      address_string[2 + len] = '\0';
      sscanf (address_string, "%p", &end_addr_p);

      if (symbol >= start_addr_p && symbol < end_addr_p)
        {
          found = g_strdup (path);
          g_free (maps_line);
          break;
        }

      g_free (maps_line);
    }

  g_free (address_string);
  g_object_unref (data_input);

  return found;
#endif /* ! ENABLE_RELOCATABLE_RESOURCES || G_OS_WIN32 */
}


static gchar *exe = NULL;

static void set_gerror (GError **error, GimpBinrelocInitError errcode);


/* Initialize the BinReloc library (for applications).
 *
 * This function must be called before using any other BinReloc functions.
 * It attempts to locate the application's canonical filename.
 *
 * @note If you want to use BinReloc for a library, then you should call
 *       _gimp_reloc_init_lib() instead.
 * @note Initialization failure is not fatal. BinReloc functions will just
 *       fallback to the supplied default path.
 *
 * @param error  If BinReloc failed to initialize, then the error report will
 *               be stored in this variable. Set to NULL if you don't want an
 *               error report. See the #GimpBinrelocInitError for a list of error
 *               codes.
 *
 * @returns TRUE on success, FALSE if BinReloc failed to initialize.
 */
gboolean
_gimp_reloc_init (GError **error)
{
  GimpBinrelocInitError errcode;

  /* Shut up compiler warning about uninitialized variable. */
  errcode = GIMP_RELOC_INIT_ERROR_NOMEM;

  /* Locate the application's filename. */
  exe = _br_find_exe (&errcode);
  if (exe != NULL)
    /* Success! */
    return TRUE;
  else
    {
      /* Failed :-( */
      set_gerror (error, errcode);
      return FALSE;
    }
}


/* Initialize the BinReloc library (for libraries).
 *
 * This function must be called before using any other BinReloc functions.
 * It attempts to locate the calling library's canonical filename.
 *
 * @note The BinReloc source code MUST be included in your library, or this
 *       function won't work correctly.
 * @note Initialization failure is not fatal. BinReloc functions will just
 *       fallback to the supplied default path.
 *
 * @returns TRUE on success, FALSE if a filename cannot be found.
 */
gboolean
_gimp_reloc_init_lib (GError **error)
{
  GimpBinrelocInitError errcode;

  /* Shut up compiler warning about uninitialized variable. */
  errcode = GIMP_RELOC_INIT_ERROR_NOMEM;

  exe = _br_find_exe_for_symbol ((const void *) "", &errcode);
  if (exe != NULL)
    {
      /* Success! */
      return TRUE;
    }
  else
    {
      /* Failed :-( */
      set_gerror (error, errcode);
      return exe != NULL;
    }
}

static void
set_gerror (GError **error, GimpBinrelocInitError errcode)
{
  const gchar *error_message;

  if (error == NULL)
    return;

  switch (errcode)
    {
    case GIMP_RELOC_INIT_ERROR_NOMEM:
      error_message = "Cannot allocate memory.";
      break;
    case GIMP_RELOC_INIT_ERROR_OPEN_MAPS:
      error_message = "Unable to open /proc/self/maps for reading.";
      break;
    case GIMP_RELOC_INIT_ERROR_READ_MAPS:
      error_message = "Unable to read from /proc/self/maps.";
      break;
    case GIMP_RELOC_INIT_ERROR_INVALID_MAPS:
      error_message = "The file format of /proc/self/maps is invalid.";
      break;
    case GIMP_RELOC_INIT_ERROR_DISABLED:
      error_message = "Binary relocation support is disabled.";
      break;
    default:
      error_message = "Unknown error.";
      break;
    };
  g_set_error (error, g_quark_from_static_string ("GBinReloc"),
               errcode, "%s", error_message);
}


/* Locate the prefix in which the current application is installed.
 *
 * The prefix is generated by the following pseudo-code evaluation:
 * \code
 * dirname(dirname(exename))
 * \endcode
 *
 * @param default_prefix  A default prefix which will used as fallback.
 * @return A string containing the prefix, which must be freed when no
 *         longer necessary. If BinReloc is not initialized, or if the
 *         initialization function failed, then a copy of default_prefix
 *         will be returned. If default_prefix is NULL, then NULL will be
 *         returned.
 */
gchar *
_gimp_reloc_find_prefix (const gchar *default_prefix)
{
  gchar *dir1, *dir2;
  gchar *exe_dir;

  if (exe == NULL)
    {
      /* BinReloc not initialized. */
      if (default_prefix != NULL)
        return g_strdup (default_prefix);
      else
        return NULL;
    }

  dir1 = g_path_get_dirname (exe);
  dir2 = g_path_get_dirname (dir1);

  exe_dir = g_path_get_basename (dir1);
  if (g_strcmp0 (exe_dir, "bin") != 0 && ! g_str_has_prefix (exe_dir, "lib"))
    {
      g_free (exe_dir);
      exe_dir = g_path_get_basename (dir2);
      if (g_str_has_prefix (exe_dir, "lib"))
        {
          /* Supporting multiarch folders, such as lib/x86_64-linux-gnu/ */
          gchar *dir3 = g_path_get_dirname (dir2);

          g_free (dir2);
          dir2 = dir3;
        }
    }

  g_free (dir1);
  g_free (exe_dir);

  return dir2;
}
