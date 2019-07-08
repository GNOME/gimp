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
#if ! defined(ENABLE_RELOCATABLE_RESOURCES) || defined(G_OS_WIN32)
  if (error)
    *error = GIMP_RELOC_INIT_ERROR_DISABLED;
  return NULL;
#else
  char *path, *path2, *line, *result;
  size_t buf_size;
  ssize_t size;
  struct stat stat_buf;
  FILE *f;

  /* Read from /proc/self/exe (symlink) */
  if (sizeof (path) > SSIZE_MAX)
    buf_size = SSIZE_MAX - 1;
  else
    buf_size = PATH_MAX - 1;
  path = g_try_new (char, buf_size);
  if (path == NULL)
    {
      /* Cannot allocate memory. */
      if (error)
        *error = GIMP_RELOC_INIT_ERROR_NOMEM;
      return NULL;
    }
  path2 = g_try_new (char, buf_size);
  if (path2 == NULL)
    {
      /* Cannot allocate memory. */
      if (error)
        *error = GIMP_RELOC_INIT_ERROR_NOMEM;
      g_free (path);
      return NULL;
    }

  strncpy (path2, "/proc/self/exe", buf_size - 1);

  while (1)
    {
      int i;

      size = readlink (path2, path, buf_size - 1);
      if (size == -1)
        {
          /* Error. */
          g_free (path2);
          break;
        }

      /* readlink() success. */
      path[size] = '\0';

      /* Check whether the symlink's target is also a symlink.
       * We want to get the final target. */
      i = stat (path, &stat_buf);
      if (i == -1)
        {
          /* Error. */
          g_free (path2);
          break;
        }

      /* stat() success. */
      if (!S_ISLNK (stat_buf.st_mode))
        {
          /* path is not a symlink. Done. */
          g_free (path2);
          return path;
        }

      /* path is a symlink. Continue loop and resolve this. */
      strncpy (path, path2, buf_size - 1);
    }


  /* readlink() or stat() failed; this can happen when the program is
   * running in Valgrind 2.2. Read from /proc/self/maps as fallback. */

  buf_size = PATH_MAX + 128;
  line = (char *) g_try_realloc (path, buf_size);
  if (line == NULL)
    {
      /* Cannot allocate memory. */
      g_free (path);
      if (error)
        *error = GIMP_RELOC_INIT_ERROR_NOMEM;
      return NULL;
    }

  f = g_fopen ("/proc/self/maps", "r");
  if (f == NULL)
    {
      g_free (line);
      if (error)
        *error = GIMP_RELOC_INIT_ERROR_OPEN_MAPS;
      return NULL;
    }

  /* The first entry should be the executable name. */
  result = fgets (line, (int) buf_size, f);
  if (result == NULL)
    {
      fclose (f);
      g_free (line);
      if (error)
        *error = GIMP_RELOC_INIT_ERROR_READ_MAPS;
      return NULL;
    }

  /* Get rid of newline character. */
  buf_size = strlen (line);
  if (buf_size == 0)
    {
      /* Huh? An empty string? */
      fclose (f);
      g_free (line);
      if (error)
        *error = GIMP_RELOC_INIT_ERROR_INVALID_MAPS;
      return NULL;
    }
  if (line[buf_size - 1] == 10)
    line[buf_size - 1] = 0;

  /* Extract the filename; it is always an absolute path. */
  path = strchr (line, '/');

  /* Sanity check. */
  if (strstr (line, " r-xp ") == NULL || path == NULL)
    {
      fclose (f);
      g_free (line);
      if (error)
        *error = GIMP_RELOC_INIT_ERROR_INVALID_MAPS;
      return NULL;
    }

  path = g_strdup (path);
  g_free (line);
  fclose (f);
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
#if ! defined(ENABLE_RELOCATABLE_RESOURCES) || defined(G_OS_WIN32)
  if (error)
    *error = GIMP_RELOC_INIT_ERROR_DISABLED;
  return (char *) NULL;
#else
#define SIZE PATH_MAX + 100
  FILE *f;
  size_t address_string_len;
  char *address_string, line[SIZE], *found;

  if (symbol == NULL)
    return (char *) NULL;

  f = g_fopen ("/proc/self/maps", "r");
  if (f == NULL)
    return (char *) NULL;

  address_string_len = 4;
  address_string = g_try_new (char, address_string_len);
  found = (char *) NULL;

  while (!feof (f))
    {
      char *start_addr, *end_addr, *end_addr_end, *file;
      void *start_addr_p, *end_addr_p;
      size_t len;

      if (fgets (line, SIZE, f) == NULL)
        break;

      /* Sanity check. */
      if (strstr (line, " r-xp ") == NULL || strchr (line, '/') == NULL)
        continue;

      /* Parse line. */
      start_addr = line;
      end_addr = strchr (line, '-');
      file = strchr (line, '/');

      /* More sanity check. */
      if (!(file > end_addr && end_addr != NULL && end_addr[0] == '-'))
        continue;

      end_addr[0] = '\0';
      end_addr++;
      end_addr_end = strchr (end_addr, ' ');
      if (end_addr_end == NULL)
        continue;

      end_addr_end[0] = '\0';
      len = strlen (file);
      if (len == 0)
        continue;
      if (file[len - 1] == '\n')
        file[len - 1] = '\0';

      /* Get rid of "(deleted)" from the filename. */
      len = strlen (file);
      if (len > 10 && strcmp (file + len - 10, " (deleted)") == 0)
        file[len - 10] = '\0';

      /* I don't know whether this can happen but better safe than sorry. */
      len = strlen (start_addr);
      if (len != strlen (end_addr))
        continue;


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
          found = file;
          break;
        }
    }

  g_free (address_string);
  fclose (f);

  if (found == NULL)
    return (char *) NULL;
  else
    return g_strdup (found);
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
    /* Success! */
    return TRUE;
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
  g_free (dir1);
  return dir2;
}
