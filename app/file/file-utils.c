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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
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

#include "plug-in/gimppluginmanager-file.h"

#include "file-utils.h"

#include "gimp-intl.h"


static gboolean
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

GFile *
file_utils_filename_to_file (Gimp         *gimp,
                             const gchar  *filename,
                             GError      **error)
{
  GFile  *file;
  gchar  *absolute;
  GError *temp_error = NULL;

  g_return_val_if_fail (GIMP_IS_GIMP (gimp), NULL);
  g_return_val_if_fail (filename != NULL, NULL);
  g_return_val_if_fail (error == NULL || *error == NULL, NULL);

  file = g_file_new_for_uri (filename);

  if (! file)
    {
      /* Despite the docs says it never fails, it actually can on Windows.
       * See issue #3093 (and glib#1819).
       */
      g_set_error_literal (error,
                           G_CONVERT_ERROR,
                           G_CONVERT_ERROR_ILLEGAL_SEQUENCE,
                           _("Invalid character sequence in URI"));
      return NULL;
    }

  /*  check for prefixes like http or ftp  */
  if (gimp_plug_in_manager_file_procedure_find_by_prefix (gimp->plug_in_manager,
                                                          GIMP_FILE_PROCEDURE_GROUP_OPEN,
                                                          file))
    {
      if (g_utf8_validate (filename, -1, NULL))
        {
          return file;
        }
      else
        {
          g_set_error_literal (error,
                               G_CONVERT_ERROR,
                               G_CONVERT_ERROR_ILLEGAL_SEQUENCE,
                               _("Invalid character sequence in URI"));
          return NULL;
        }
    }
  else if (file_utils_filename_is_uri (filename, &temp_error))
    {
      return file;
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

  file = g_file_new_for_path (absolute);

  g_free (absolute);

  return file;
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
                                          (GimpThumbSize) GIMP_THUMBNAIL_SIZE_NORMAL,
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
