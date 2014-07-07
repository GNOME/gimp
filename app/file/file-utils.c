/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995, 1996, 1997 Spencer Kimball and Peter Mattis
 * Copyright (C) 1997 Josh MacDonald
 *
 * file-utils.c
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <string.h>

#include <gegl.h>
#include <gdk-pixbuf/gdk-pixbuf.h>

#include "libgimpbase/gimpbase.h"
#include "libgimpmath/gimpmath.h"
#include "libgimpthumb/gimpthumb.h"

#include "core/core-types.h"

#include "core/gimp.h"
#include "core/gimpimage.h"
#include "core/gimpimagefile.h"

#include "plug-in/gimppluginmanager.h"

#include "file-procedure.h"
#include "file-utils.h"

#include "gimp-intl.h"


gboolean
file_utils_filename_is_uri (const gchar  *filename,
                            GError      **error)
{
  g_return_val_if_fail (filename != NULL, FALSE);
  g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

  if (strstr (filename, "://"))
    {
      gchar *scheme;
      gchar *canon;

      scheme = g_strndup (filename, (strstr (filename, "://") - filename));
      canon  = g_strdup (scheme);

      g_strcanon (canon, G_CSET_A_2_Z G_CSET_a_2_z G_CSET_DIGITS "+-.", '-');

      if (strcmp (scheme, canon) || ! g_ascii_isgraph (canon[0]))
        {
          g_set_error (error, G_FILE_ERROR, 0,
                       _("'%s:' is not a valid URI scheme"), scheme);

          g_free (scheme);
          g_free (canon);

          return FALSE;
        }

      g_free (scheme);
      g_free (canon);

      if (! g_utf8_validate (filename, -1, NULL))
        {
          g_set_error_literal (error,
			       G_CONVERT_ERROR,
			       G_CONVERT_ERROR_ILLEGAL_SEQUENCE,
			       _("Invalid character sequence in URI"));
          return FALSE;
        }

      return TRUE;
    }

  return FALSE;
}

gchar *
file_utils_filename_to_uri (Gimp         *gimp,
                            const gchar  *filename,
                            GError      **error)
{
  GFile  *file;
  gchar  *absolute;
  gchar  *uri;
  GError *temp_error = NULL;

  g_return_val_if_fail (GIMP_IS_GIMP (gimp), NULL);
  g_return_val_if_fail (filename != NULL, NULL);
  g_return_val_if_fail (error == NULL || *error == NULL, NULL);

  file = g_file_new_for_uri (filename);

  /*  check for prefixes like http or ftp  */
  if (file_procedure_find_by_prefix (gimp->plug_in_manager->load_procs,
                                     file))
    {
      if (g_utf8_validate (filename, -1, NULL))
        {
          g_object_unref (file);

          return g_strdup (filename);
        }
      else
        {
          g_set_error_literal (error,
			       G_CONVERT_ERROR,
			       G_CONVERT_ERROR_ILLEGAL_SEQUENCE,
			       _("Invalid character sequence in URI"));
          g_object_unref (file);
          return NULL;
        }
    }
  else if (file_utils_filename_is_uri (filename, &temp_error))
    {
      g_object_unref (file);

      return g_strdup (filename);
    }
  else if (temp_error)
    {
      g_propagate_error (error, temp_error);
      g_object_unref (file);

      return NULL;
    }

  g_object_unref (file);

  if (! g_path_is_absolute (filename))
    {
      gchar *current;

      current = g_get_current_dir ();
      absolute = g_build_filename (current, filename, NULL);
      g_free (current);
    }
  else
    {
      absolute = g_strdup (filename);
    }

  uri = g_filename_to_uri (absolute, NULL, error);

  g_free (absolute);

  return uri;
}

GFile *
file_utils_file_with_new_ext (GFile *file,
                              GFile *ext_file)
{
  gchar       *uri;
  const gchar *uri_ext;
  gchar       *ext_uri     = NULL;
  const gchar *ext_uri_ext = NULL;
  gchar       *uri_without_ext;
  gchar       *new_uri;
  GFile       *ret;

  g_return_val_if_fail (G_IS_FILE (file), NULL);
  g_return_val_if_fail (ext_file == NULL || G_IS_FILE (ext_file), NULL);

  uri     = g_file_get_uri (file);
  uri_ext = file_utils_uri_get_ext (uri);

  if (ext_file)
    {
      ext_uri     = g_file_get_uri (ext_file);
      ext_uri_ext = file_utils_uri_get_ext (ext_uri);
    }

  uri_without_ext = g_strndup (uri, uri_ext - uri);

  new_uri = g_strconcat (uri_without_ext, ext_uri_ext, NULL);

  ret = g_file_new_for_uri (new_uri);

  g_free (uri_without_ext);
  g_free (new_uri);

  return ret;
}


/**
 * file_utils_uri_get_ext:
 * @uri:
 *
 * Returns the position of the extension (including the .) for an URI. If
 * there is no extension the returned position is right after the
 * string, at the terminating NULL character.
 *
 * Returns:
 **/
const gchar *
file_utils_uri_get_ext (const gchar *uri)
{
  const gchar *ext        = NULL;
  int          uri_len    = strlen (uri);
  int          search_len = 0;

  if (g_strrstr (uri, ".gz"))
    search_len = uri_len - 3;
  else if (g_strrstr (uri, ".bz2"))
    search_len = uri_len - 4;
  else if (g_strrstr (uri, ".xz"))
    search_len = uri_len - 3;
  else
    search_len = uri_len;

  ext = g_strrstr_len (uri, search_len, ".");

  if (! ext)
    ext = uri + uri_len;

  return ext;
}

GdkPixbuf *
file_utils_load_thumbnail (const gchar *filename)
{
  GimpThumbnail *thumbnail = NULL;
  GdkPixbuf     *pixbuf    = NULL;
  gchar         *uri;

  g_return_val_if_fail (filename != NULL, NULL);

  uri = g_filename_to_uri (filename, NULL, NULL);

  if (uri)
    {
      thumbnail = gimp_thumbnail_new ();
      gimp_thumbnail_set_uri (thumbnail, uri);

      pixbuf = gimp_thumbnail_load_thumb (thumbnail,
                                          GIMP_THUMBNAIL_SIZE_NORMAL,
                                          NULL);
    }

  g_free (uri);

  if (pixbuf)
    {
      gint width  = gdk_pixbuf_get_width (pixbuf);
      gint height = gdk_pixbuf_get_height (pixbuf);

      if (gdk_pixbuf_get_n_channels (pixbuf) != 3)
        {
          GdkPixbuf *tmp = gdk_pixbuf_new (GDK_COLORSPACE_RGB, FALSE, 8,
                                           width, height);

          gdk_pixbuf_composite_color (pixbuf, tmp,
                                      0, 0, width, height, 0, 0, 1.0, 1.0,
                                      GDK_INTERP_NEAREST, 255,
                                      0, 0, GIMP_CHECK_SIZE_SM,
                                      0x66666666, 0x99999999);

          g_object_unref (pixbuf);
          pixbuf = tmp;
        }
    }

  return pixbuf;
}

gboolean
file_utils_save_thumbnail (GimpImage   *image,
                           const gchar *filename)
{
  GFile    *file;
  gboolean  success = FALSE;

  g_return_val_if_fail (GIMP_IS_IMAGE (image), FALSE);
  g_return_val_if_fail (filename != NULL, FALSE);

  file = gimp_image_get_file (image);

  if (file)
    {
      gchar *image_uri = g_file_get_uri (file);
      gchar *uri       = g_filename_to_uri (filename, NULL, NULL);

      if (uri && image_uri && ! strcmp (uri, image_uri))
        {
          GimpImagefile *imagefile;

          imagefile = gimp_imagefile_new (image->gimp, file);
          success = gimp_imagefile_save_thumbnail (imagefile, NULL, image,
                                                   NULL);
          g_object_unref (imagefile);
        }

      g_free (image_uri);
      g_free (uri);
    }

  return success;
}
