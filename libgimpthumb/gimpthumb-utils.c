/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * Thumbnail handling according to the Thumbnail Managing Standard.
 * http://triq.net/~pearl/thumbnail-spec/
 *
 * Copyright (C) 2001-2003  Sven Neumann <sven@gimp.org>
 *                          Michael Natterer <mitch@gimp.org>
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
#include <sys/stat.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include <glib-object.h>

#ifdef G_OS_WIN32
#include <libgimpbase/gimpwin32-io.h>
#endif

#include "libgimpmath/gimpmath.h"

#include "gimpthumb-error.h"
#include "gimpthumb-types.h"
#include "gimpthumb-utils.h"

#include "libgimp/libgimp-intl.h"


static gint           gimp_thumb_size     (GimpThumbSize  size);
static const gchar  * gimp_thumb_png_name (const gchar   *uri);
static void           gimp_thumb_exit     (void);



static gboolean    gimp_thumb_initialized = FALSE;
static gint        thumb_num_sizes        = 0;
static gint       *thumb_sizes            = NULL;
static gchar      *thumb_dir              = NULL;
static gchar     **thumb_subdirs          = NULL;
static gchar      *thumb_fail_subdir      = NULL;



/**
 * gimp_thumb_init:
 * @creator: an ASCII string that identifies the thumbnail creator
 * @thumb_basedir: an absolute path or %NULL to use the default
 *
 * This function initializes the thumbnail system. It must be called
 * before any other functions from libgimpthumb are used. You may call
 * it more than once if you want to change the @thumb_basedir but if
 * you do that, you should make sure that no thread is still using the
 * library. Apart from this function, libgimpthumb is multi-thread
 * safe.
 *
 * The @creator string must be 7bit ASCII and should contain the name
 * of the software that creates the thumbnails. It is used to handle
 * thumbnail creation failures. See the spec for more details.
 *
 * Usually you will pass %NULL for @thumb_basedir. Thumbnails will
 * then be stored in the user's personal thumbnail directory as
 * defined in the spec. If you wish to use libgimpthumb to store
 * application-specific thumbnails, you can specify a different base
 * directory here.
 *
 * Return value: %TRUE if the library was successfully initialized.
 **/
gboolean
gimp_thumb_init (const gchar *creator,
                 const gchar *thumb_basedir)
{
  GEnumClass *enum_class;
  GEnumValue *enum_value;
  gint        i;

  g_return_val_if_fail (creator != NULL, FALSE);
  g_return_val_if_fail (thumb_basedir == NULL ||
                        g_path_is_absolute (thumb_basedir), FALSE);

  if (gimp_thumb_initialized)
    gimp_thumb_exit ();

  thumb_dir = (thumb_basedir ?
               g_strdup (thumb_basedir) :
               g_build_filename (g_get_home_dir(), ".thumbnails", NULL));

  enum_class = g_type_class_ref (GIMP_TYPE_THUMB_SIZE);

  thumb_num_sizes = enum_class->n_values;
  thumb_sizes     = g_new (gint, thumb_num_sizes);
  thumb_subdirs   = g_new (gchar *, thumb_num_sizes);

  for (i = 0, enum_value = enum_class->values;
       i < enum_class->n_values;
       i++, enum_value++)
    {
      thumb_sizes[i]   = enum_value->value;
      thumb_subdirs[i] = g_build_filename (thumb_dir,
                                           enum_value->value_nick, NULL);
    }

  thumb_fail_subdir = thumb_subdirs[0];
  thumb_subdirs[0]  = g_build_filename (thumb_fail_subdir, creator, NULL);

  gimp_thumb_initialized = TRUE;

  return gimp_thumb_initialized;
}

/**
 * gimp_thumb_get_thumb_dir:
 * @size: a GimpThumbSize
 *
 * Retrieve the name of a thumbnail folder for a specific size. The
 * returned pointer will become invalid if gimp_thumb_init() is used
 * again. It must not be changed or freed.
 *
 * Return value: the thumbnail directory in the encoding of the filesystem
 **/
const gchar *
gimp_thumb_get_thumb_dir (GimpThumbSize  size)
{
  g_return_val_if_fail (gimp_thumb_initialized, FALSE);

  size = gimp_thumb_size (size);

  return thumb_subdirs[size];
}

/**
 * gimp_thumb_ensure_thumb_dir:
 * @size: a GimpThumbSize
 * @error: return location for possible errors
 *
 * This function checks if the directory that is required to store
 * thumbnails for a particular @size exist and attempts to create it
 * if necessary.
 *
 * You shouldn't have to call this function directly since
 * gimp_thumbnail_save_thumb() and gimp_thumbnail_save_failure() will
 * do this for you.
 *
 * Return value: %TRUE is the directory exists, %FALSE if it could not
 *               be created
 **/
gboolean
gimp_thumb_ensure_thumb_dir (GimpThumbSize   size,
                             GError        **error)
{
  gint i;

  g_return_val_if_fail (gimp_thumb_initialized, FALSE);
  g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

  i = gimp_thumb_size (size);

  if (g_file_test (thumb_subdirs[i], G_FILE_TEST_IS_DIR))
    return TRUE;

  if (g_file_test (thumb_dir, G_FILE_TEST_IS_DIR) ||
      (mkdir (thumb_dir, S_IRUSR | S_IWUSR | S_IXUSR) == 0))
    {
      if (i == 0)
        mkdir (thumb_fail_subdir, S_IRUSR | S_IWUSR | S_IXUSR);

      mkdir (thumb_subdirs[i], S_IRUSR | S_IWUSR | S_IXUSR);
    }

  if (g_file_test (thumb_subdirs[i], G_FILE_TEST_IS_DIR))
    return TRUE;

  g_set_error (error,
               GIMP_THUMB_ERROR, GIMP_THUMB_ERROR_MKDIR,
               _("Failed to create thumbnail folder '%s'."),
               thumb_subdirs[i]);
  return FALSE;
}

/**
 * gimp_thumb_name_from_uri:
 * @uri: an escaped URI in UTF-8 encoding
 * @size: a #GimpThumbSize
 *
 * Creates the name of the thumbnail file of the specified @size that
 * belongs to an image file located at the given @uri.
 *
 * Return value: a newly allocated filename in the encoding of the
 *               filesystem or %NULL if you attempt to create thumbnails
 *               for files in the thumbnail directory.
 **/
gchar *
gimp_thumb_name_from_uri (const gchar   *uri,
                          GimpThumbSize  size)
{
  gint i;

  g_return_val_if_fail (gimp_thumb_initialized, NULL);
  g_return_val_if_fail (uri != NULL, NULL);

  if (strstr (uri, thumb_dir))
    return NULL;

  i = gimp_thumb_size (size);

  return g_build_filename (thumb_subdirs[i], gimp_thumb_png_name (uri), NULL);
}

/**
 * gimp_thumb_find_thumb:
 * @uri: an escaped URI in UTF-8 encoding
 * @size: pointer to a #GimpThumbSize
 *
 * This function attempts to locate a thumbnail for the given
 * @url. First it tries the size that is stored at @size. If no
 * thumbnail of that size is found, it will look for a larger
 * thumbnail, then falling back to a smaller size. If a thumbnail is
 * found, it's size is written to the variable pointer to by @size
 * and the file location is returned.
 *
 * Return value: a newly allocated string in the encoding of the
 *               filesystem or %NULL if no thumbnail for @uri was found
 **/
gchar *
gimp_thumb_find_thumb (const gchar   *uri,
                       GimpThumbSize *size)
{
  const gchar *name;
  gchar       *thumb_name;
  gint         i, n;

  g_return_val_if_fail (gimp_thumb_initialized, NULL);
  g_return_val_if_fail (uri != NULL, NULL);
  g_return_val_if_fail (size != NULL, NULL);
  g_return_val_if_fail (*size > GIMP_THUMB_SIZE_FAIL, NULL);

  name = gimp_thumb_png_name (uri);

  i = n = gimp_thumb_size (*size);

  for (; i < thumb_num_sizes; i++)
    {
      thumb_name = g_build_filename (thumb_subdirs[i], name, NULL);

      if (gimp_thumb_file_test (thumb_name, NULL, NULL))
        {
          *size = thumb_sizes[i];
          return thumb_name;
        }

      g_free (thumb_name);
    }

  for (i = n - 1; i >= 0; i--)
    {
      thumb_name = g_build_filename (thumb_subdirs[i], name, NULL);

      if (gimp_thumb_file_test (thumb_name, NULL, NULL))
        {
          *size = thumb_sizes[i];
          return thumb_name;
        }

      g_free (thumb_name);
    }

  return NULL;
}

/**
 * gimp_thumb_file_test:
 * @filename: a filename in the encoding of the filesystem
 * @mtime: return location for modification time
 * @size: return location for file size
 *
 * This is a convenience and portability wrapper around stat(). It
 * checks if the given @filename exists and returns modification time
 * and file size in 64bit integer values.
 *
 * Return value: %TRUE if the file exists, %FALSE otherwise
 **/
gboolean
gimp_thumb_file_test (const gchar *filename,
                      gint64      *mtime,
                      gint64      *size)
{
  struct stat s;

  g_return_val_if_fail (filename != NULL, FALSE);

  if (stat (filename, &s) == 0 && (S_ISREG (s.st_mode)))
    {
      if (mtime)
        *mtime = s.st_mtime;
      if (size)
        *size = s.st_size;

      return TRUE;
    }

  return FALSE;
}

static void
gimp_thumb_exit (void)
{
  gint i;

  g_free (thumb_dir);
  g_free (thumb_sizes);
  for (i = 0; i < thumb_num_sizes; i++)
    g_free (thumb_subdirs[i]);
  g_free (thumb_subdirs);
  g_free (thumb_fail_subdir);

  thumb_num_sizes        = 0;
  thumb_sizes            = NULL;
  thumb_dir              = NULL;
  thumb_subdirs          = NULL;
  thumb_fail_subdir      = NULL;
  gimp_thumb_initialized = FALSE;
}

static gint
gimp_thumb_size (GimpThumbSize size)
{
  gint i = 0;

  if (size > GIMP_THUMB_SIZE_FAIL)
    {
      for (i = 1;
           i < thumb_num_sizes && thumb_sizes[i] < size;
           i++)
        /* nothing */;

      if (i == thumb_num_sizes)
        i--;
    }

  return i;
}

static const gchar *
gimp_thumb_png_name (const gchar *uri)
{
  static gchar name[40];
  guchar       digest[16];
  guchar       n;
  gint         i;

  gimp_md5_get_digest (uri, -1, digest);

  for (i = 0; i < 16; i++)
    {
      n = (digest[i] >> 4) & 0xF;
      name[i * 2]     = (n > 9) ? 'a' + n - 10 : '0' + n;

      n = digest[i] & 0xF;
      name[i * 2 + 1] = (n > 9) ? 'a' + n - 10 : '0' + n;
    }

  strncpy (name + 32, ".png", 5);

  return (const gchar *) name;
}
