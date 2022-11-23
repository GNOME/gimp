/* LIGMA - The GNU Image Manipulation Program
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

#include "libligmabase/ligmabase.h"
#include "libligmamath/ligmamath.h"
#include "libligmathumb/ligmathumb.h"

#include "core/core-types.h"

#include "core/ligma.h"
#include "core/ligmaimage.h"
#include "core/ligmaimagefile.h"

#include "plug-in/ligmapluginmanager-file.h"

#include "file-utils.h"

#include "ligma-intl.h"


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
file_utils_filename_to_file (Ligma         *ligma,
                             const gchar  *filename,
                             GError      **error)
{
  GFile  *file;
  gchar  *absolute;
  GError *temp_error = NULL;

  g_return_val_if_fail (LIGMA_IS_LIGMA (ligma), NULL);
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
  if (ligma_plug_in_manager_file_procedure_find_by_prefix (ligma->plug_in_manager,
                                                          LIGMA_FILE_PROCEDURE_GROUP_OPEN,
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
file_utils_load_thumbnail (GFile *file)
{
  LigmaThumbnail *thumbnail = NULL;
  GdkPixbuf     *pixbuf    = NULL;
  gchar         *uri;

  g_return_val_if_fail (G_IS_FILE (file), NULL);

  uri = g_file_get_uri (file);

  thumbnail = ligma_thumbnail_new ();
  ligma_thumbnail_set_uri (thumbnail, uri);

  g_free (uri);

  pixbuf = ligma_thumbnail_load_thumb (thumbnail,
                                      (LigmaThumbSize) LIGMA_THUMBNAIL_SIZE_NORMAL,
                                      NULL);

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
                                      0, 0, LIGMA_CHECK_SIZE_SM,
                                      0x66666666, 0x99999999);

          g_object_unref (pixbuf);
          pixbuf = tmp;
        }
    }

  return pixbuf;
}

gboolean
file_utils_save_thumbnail (LigmaImage *image,
                           GFile     *file)
{
  GFile    *image_file;
  gboolean  success = FALSE;

  g_return_val_if_fail (LIGMA_IS_IMAGE (image), FALSE);
  g_return_val_if_fail (G_IS_FILE (file), FALSE);

  image_file = ligma_image_get_file (image);

  if (image_file)
    {
      gchar *image_uri = g_file_get_uri (image_file);
      gchar *uri       = g_file_get_uri (file);

      if (uri && image_uri && ! strcmp (uri, image_uri))
        {
          LigmaImagefile *imagefile;

          imagefile = ligma_imagefile_new (image->ligma, file);
          success = ligma_imagefile_save_thumbnail (imagefile, NULL, image,
                                                   NULL);
          g_object_unref (imagefile);
        }

      g_free (image_uri);
      g_free (uri);
    }

  return success;
}
