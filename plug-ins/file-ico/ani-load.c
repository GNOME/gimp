/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995-1997 Spencer Kimball and Peter Mattis
 *
 * GIMP Plug-in for Windows Icon files.
 * Copyright (C) 2002 Christian Kreibich <christian@whoop.org>.
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

#include <errno.h>
#include <string.h>

#include <glib/gstdio.h>

#include <libgimp/gimp.h>

#include "ico.h"
#include "ani.h"
#include "ani-load.h"
#include "ico-load.h"

#include "libgimp/stdplugins-intl.h"

/* Ported from James Huang's ani.c code, under the GPL license, version 3
 * or any later version of the license */
GimpImage *
ani_load_image (GFile    *file,
                gboolean  load_thumb,
                gint     *width,
                gint     *height,
                GError  **error)
{
  FILE         *fp;
  GimpImage    *image = NULL;
  GimpParasite *parasite;
  gchar         id[4];
  guint32       size;
  guint8        padding;
  gint32        file_offset;
  guint         frame = 1;
  AniFileHeader header;
  gchar        *inam  = NULL;
  gchar        *iart  = NULL;
  gchar        *str;

  gimp_progress_init_printf (_("Opening '%s'"),
                             gimp_file_get_utf8_name (file));

  fp = g_fopen (g_file_peek_path (file), "rb");

  if (! fp)
    {
      g_set_error (error, G_FILE_ERROR, g_file_error_from_errno (errno),
                   _("Could not open '%s' for reading: %s"),
                   gimp_file_get_utf8_name (file), g_strerror (errno));
      return NULL;
    }

  while (fread (id, 1, 4, fp) == 4)
    {
      if (memcmp (id, "RIFF", 4) == 0)
        {
          fread (&size, sizeof (size), 1, fp);
        }
      else if (memcmp (id, "anih", 4) == 0)
        {
          fread (&size, sizeof (size), 1, fp);
          fread (&header, sizeof (header), 1, fp);
        }
      else if (memcmp (id, "rate", 4) == 0)
        {
          fread (&size, sizeof (size), 1, fp);
          fseek (fp, size, SEEK_CUR);
        }
      else if (memcmp (id, "seq ", 4) == 0)
        {
          fread (&size, sizeof (size), 1, fp);
          fseek (fp, size, SEEK_CUR);
        }
      else if (memcmp (id, "LIST", 4) == 0)
        {
          fread (&size, sizeof (size), 1, fp);
        }
      else if (memcmp (id, "INAM", 4) == 0)
        {
          gint n_read = -1;

          fread (&size, sizeof (size), 1, fp);
          if (size > 0)
            {
              if (inam)
                g_free (inam);

              inam = g_try_new0 (gchar, size + 1);
              if (inam == NULL)
                {
                  fclose (fp);
                  g_set_error (error, G_FILE_ERROR,
                               g_file_error_from_errno (errno),
                               _("Invalid ANI metadata"));
                  return NULL;
                }

              n_read = fread (inam, sizeof (gchar), size, fp);
              inam[size] = '\0';
            }

          if (n_read < 1)
            {
              fclose (fp);
              g_set_error (error, G_FILE_ERROR, g_file_error_from_errno (errno),
                           _("Invalid ANI metadata"));
              return NULL;
            }

          if (inam && ! g_utf8_validate (inam, -1, NULL))
            {
              /* RIFF LIST/INFO text is nominally UTF-8 (XMP
               * Specification Part 3, section 2.3.2.1), but legacy
               * ANI files store INAM/IART as Windows-1252 (CP_ACP).
               * Convert rather than rejecting the whole file; if
               * conversion also fails, drop this field and keep
               * loading.
               */
              gchar *converted = g_convert (inam, -1, "UTF-8",
                                            "WINDOWS-1252",
                                            NULL, NULL, NULL);
              g_free (inam);
              inam = converted;
            }

          /* Metadata length must be even. If data itself is odd,
           * then an extra 0x00 is added for padding. We read in
           * that extra byte to keep loading properly.
           * See discussion in #8562.
           */
          if (size % 2 != 0)
            fread (&padding, sizeof (padding), 1, fp);
        }
      else if (memcmp (id, "IART", 4) == 0)
        {
          gint n_read = -1;

          fread (&size, sizeof (size), 1, fp);
          if (size > 0)
            {
              if (iart)
                g_free (iart);

              iart = g_try_new0 (gchar, size + 1);
              if (iart == NULL)
                {
                  fclose (fp);
                  g_set_error (error, G_FILE_ERROR,
                               g_file_error_from_errno (errno),
                               _("Invalid ANI metadata"));
                  return NULL;
                }

              n_read = fread (iart, sizeof (gchar), size, fp);
              iart[size] = '\0';
            }

          if (n_read < 1)
            {
              fclose (fp);
              g_set_error (error, G_FILE_ERROR, g_file_error_from_errno (errno),
                           _("Invalid ANI metadata"));
              return NULL;
            }

          if (iart && ! g_utf8_validate (iart, -1, NULL))
            {
              /* RIFF LIST/INFO text is nominally UTF-8 (XMP
               * Specification Part 3, section 2.3.2.1), but legacy
               * ANI files store INAM/IART as Windows-1252 (CP_ACP).
               * Convert rather than rejecting the whole file; if
               * conversion also fails, drop this field and keep
               * loading.
               */
              gchar *converted = g_convert (iart, -1, "UTF-8",
                                            "WINDOWS-1252",
                                            NULL, NULL, NULL);
              g_free (iart);
              iart = converted;
            }

          if (size % 2 != 0)
            fread (&padding, sizeof (padding), 1, fp);
        }
      else if (memcmp (id, "icon", 4) == 0)
        {
          fread (&size, sizeof (size), 1, fp);
          file_offset = ftell (fp);
          if (load_thumb)
            {
              image = ico_load_thumbnail_image (file, width, height, file_offset, error);
              break;
            }
          else
            {
              if (! image)
                {
                  image = ico_load_image (file, &file_offset, 1, error);
                }
              else
                {
                  GimpImage    *temp_image = NULL;
                  GimpLayer   **layers;
                  GimpLayer    *new_layer;

                  temp_image = ico_load_image (file, &file_offset, frame + 1,
                                               error);
                  layers = gimp_image_get_layers (temp_image);
                  if (layers)
                    {
                      for (gint i = 0; layers[i]; i++)
                        {
                          new_layer = gimp_layer_new_from_drawable (GIMP_DRAWABLE (layers[i]),
                                                                    image);
                          gimp_image_insert_layer (image, new_layer, NULL, frame);
                          frame++;
                        }
                      g_free (layers);
                    }
                  gimp_image_delete (temp_image);
                }

              /* Update position after reading icon data */
              fseek (fp, file_offset, SEEK_SET);
              if (header.frames > 0)
                gimp_progress_update ((gdouble) frame /
                                      (gdouble) header.frames);
            }
        }
    }
  fclose (fp);

  /* Saving header metadata */
  str = g_strdup_printf ("%d", header.jif_rate);
  parasite = gimp_parasite_new ("ani-header",
                                GIMP_PARASITE_PERSISTENT,
                                strlen (str) + 1, (gpointer) str);
  g_free (str);
  gimp_image_attach_parasite (image, parasite);
  gimp_parasite_free (parasite);

  /* Saving INFO block */
  if (inam && strlen (inam) > 0)
    {
      str = g_strdup_printf ("%s", inam);
      parasite = gimp_parasite_new ("ani-info-inam",
                                    GIMP_PARASITE_PERSISTENT,
                                    strlen (str) + 1, (gpointer) str);
      g_free (str);
      g_free (inam);
      gimp_image_attach_parasite (image, parasite);
      gimp_parasite_free (parasite);
    }

  if (iart && strlen (iart) > 0)
    {
      str = g_strdup_printf ("%s", iart);
      parasite = gimp_parasite_new ("ani-info-iart",
                                    GIMP_PARASITE_PERSISTENT,
                                    strlen (str) + 1, (gpointer) str);
      g_free (str);
      g_free (iart);
      gimp_image_attach_parasite (image, parasite);
      gimp_parasite_free (parasite);
    }

  gimp_progress_update (1.0);

  return image;
}
