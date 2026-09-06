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
#include "ani-export.h"
#include "ico-export.h"

#include "libgimp/stdplugins-intl.h"

/* Ported from James Huang's ani.c code, under the GPL v3 license */
GimpPDBStatusType
ani_export_image (GFile                *file,
                  GimpImage            *image,
                  GimpProcedure        *procedure,
                  GimpProcedureConfig  *config,
                  gint32                run_mode,
                  gsize                *n_hot_spot_x,
                  const gint32         *hot_spot_x,
                  gint32              **new_hot_spot_x,
                  gsize                *n_hot_spot_y,
                  const gint32         *hot_spot_y,
                  gint32              **new_hot_spot_y,
                  AniFileHeader        *header,
                  AniSaveInfo          *ani_info,
                  GError              **error)
{
  FILE         *fp;
  gint32        i;
  gchar        *str;
  GimpParasite *parasite = NULL;
  gchar         id[5];
  guint32       size;
  guint8        padding       = 0;
  gint32        offset, ofs_size_riff, ofs_size_list, ofs_size_icon;
  gint32        ofs_size_info = 0;
  gint32        ofs_metadata  = 0;
  IcoSaveInfo   info;

  if (! ico_save_init (image, run_mode, &info,
                       *n_hot_spot_x, hot_spot_x,
                       *n_hot_spot_y, hot_spot_y,
                       error))
    {
      return GIMP_PDB_EXECUTION_ERROR;
    }

  /* Save individual frames as .cur so we can retain
   * the hotspot information
   */
  info.is_cursor = TRUE;

  /* Default header values */
  header->size_of = sizeof (*header);
  header->frames = info.num_icons;
  header->steps = info.num_icons;
  header->x = 0;
  header->y = 0;
  if (info.depths[0] == 24)
    {
      header->bpp = 4;
      header->planes = 1;
    }
  else
    {
      header->bpp = 0;
      header->planes = 0;
    }
  header->flags = 1;

  /* Load metadata from parasite */
  parasite = gimp_image_get_parasite (image, "ani-header");
  if (parasite)
    {
      gchar   *parasite_data;
      guint32  parasite_size;
      gint     jif_rate;

      parasite_data = (gchar *) gimp_parasite_get_data (parasite, &parasite_size);
      parasite_data = g_strndup (parasite_data, parasite_size);

#ifndef _UCRT
      if (sscanf (parasite_data, "%i", &jif_rate) == 1)
#else
      if (sscanf_s (parasite_data, "%i", &jif_rate) == 1)
#endif
        {
          header->jif_rate = jif_rate;
        }

      gimp_parasite_free (parasite);
      g_free (parasite_data);
    }

  parasite = gimp_image_get_parasite (image, "ani-info-inam");
  if (parasite)
    {
      guint32  parasite_size;
      gchar   *inam = NULL;

      inam = (gchar *) gimp_parasite_get_data (parasite, &parasite_size);
      ani_info->inam = g_strndup (inam, parasite_size);

      gimp_parasite_free (parasite);
    }

  parasite = gimp_image_get_parasite (image, "ani-info-iart");
  if (parasite)
    {
      guint32  parasite_size;
      gchar   *iart = NULL;

      iart = (gchar *) gimp_parasite_get_data (parasite, &parasite_size);
      ani_info->iart = g_strndup (iart, parasite_size);

      gimp_parasite_free (parasite);
    }

  if (run_mode == GIMP_RUN_INTERACTIVE)
    {
      if (! ico_save_dialog (image, procedure, config, &info,
                             header, ani_info))
        return GIMP_PDB_CANCEL;

      for (i = 1; i < info.num_icons; i++)
        {
          info.depths[i] = info.depths[0];
          info.default_depths[i] = info.default_depths[0];
          info.compress[i] = info.compress[0];
        }
    }

  gimp_progress_init_printf (_("Exporting '%s'"),
                             gimp_file_get_utf8_name (file));

  fp = g_fopen (g_file_peek_path (file), "wb");

  if (! fp)
    {
      g_set_error (error, G_FILE_ERROR, g_file_error_from_errno (errno),
                   _("Could not open '%s' for writing: %s"),
                   gimp_file_get_utf8_name (file), g_strerror (errno));
      return GIMP_PDB_EXECUTION_ERROR;
    }

  /* Writing the .ani header data */
  memcpy (id, "RIFF", 4);
  size = 0;
  fwrite (id, 4, 1, fp);
  ofs_size_riff = ftell (fp);
  fwrite (&size, sizeof (size), 1, fp);

  memcpy (id, "ACON", 4);
  fwrite (id, 4, 1, fp);

  /* RIFF LIST/INFO text is written as UTF-8 (XMP Specification
   * Part 3, section 2.3.2.1).
   */
  if ((ani_info->inam && strlen (ani_info->inam) > 0) ||
      (ani_info->iart && strlen (ani_info->iart) > 0))
    {
      gint32 string_size;

      memcpy (id, "LIST", 4);
      fwrite (id, 4, 1, fp);
      ofs_size_info = ftell (fp);
      fwrite (&size, sizeof (size), 1, fp);

      memcpy (id, "INFO", 4);
      fwrite (id, 4, 1, fp);
      if (ani_info->inam && strlen (ani_info->inam) > 0) /* Cursor name */
        {
          memcpy (id, "INAM", 4);

          fwrite (id, 4, 1, fp);
          string_size = strlen (ani_info->inam) + 1;
          fwrite (&string_size, 4, 1, fp);
          fwrite (ani_info->inam, string_size, 1, fp);
          ofs_metadata += 4;

          /* Length of metadata must be even. */
          if (string_size % 2 != 0)
            fwrite (&padding, sizeof (padding), 1, fp);
        }
      if (ani_info->iart && strlen (ani_info->iart) > 0) /* Author name */
        {
          memcpy (id, "IART", 4);

          fwrite (id, 4, 1, fp);
          string_size = strlen (ani_info->iart) + 1;
          fwrite (&string_size, 4, 1, fp);
          fwrite (ani_info->iart, string_size, 1, fp);
          ofs_metadata += 4;

          if (string_size % 2 != 0)
            fwrite (&padding, sizeof (padding), 1, fp);
        }

      /* Go back and update info list size */
      fseek (fp, 0L, SEEK_END);
      size = ftell (fp) - ofs_size_info - 4;
      fseek (fp, ofs_size_info, SEEK_SET);
      fwrite (&size, sizeof (size), 1, fp);
      fseek (fp, 0L, SEEK_END);
    }

  memcpy (id, "anih", 4);
  size = sizeof (*header);
  fwrite (id, 4, 1, fp);
  fwrite (&size, sizeof (size), 1, fp);
  fwrite (header, sizeof (*header), 1, fp);

  memcpy (id, "LIST", 4);
  fwrite (id, 4, 1, fp);
  ofs_size_list = ftell (fp);
  fwrite (&size, sizeof (size), 1, fp);

  memcpy (id, "fram", 4);
  fwrite (id, 4, 1, fp);

  memcpy (id, "icon", 4);
  for (i = 0; i < info.num_icons; i++)
    {
      GimpPDBStatusType status;

      fwrite (id, 4, 1, fp);
      ofs_size_icon = ftell (fp);
      fwrite (&size, sizeof (size), 1, fp);
      offset = ftell (fp);
      status = shared_save_image (file, fp, image, procedure,
                                  config, run_mode,
                                  n_hot_spot_x, hot_spot_x, new_hot_spot_x,
                                  n_hot_spot_y, hot_spot_y, new_hot_spot_y,
                                  offset, i, error, &info);

      if (status != GIMP_PDB_SUCCESS)
        {
          ico_save_info_free (&info);
          g_clear_pointer (&ani_info->inam, g_free);
          g_clear_pointer (&ani_info->iart, g_free);
          fclose (fp);
          return GIMP_PDB_EXECUTION_ERROR;
        }
      fseek (fp, 0L, SEEK_END);
      size = ftell (fp) - offset;
      fseek (fp, ofs_size_icon, SEEK_SET);
      fwrite (&size, sizeof (size), 1, fp);
      fseek (fp, 0L, SEEK_END);

      gimp_progress_update ((gdouble) i / (gdouble) info.num_icons);
    }
  ico_save_info_free (&info);

  fseek (fp, 0L, SEEK_END);
  size = ftell (fp);
  size -= ofs_metadata;
  fseek (fp, ofs_size_riff, SEEK_SET);
  fwrite (&size, sizeof (size), 1, fp);

  size -= ofs_size_list;
  size += (ofs_metadata - 4);
  fseek (fp, ofs_size_list, SEEK_SET);
  fwrite (&size, sizeof (size), 1, fp);
  fclose (fp);

  /* Update metadata if needed */
  str = g_strdup_printf ("%d", header->jif_rate);
  parasite = gimp_parasite_new ("ani-header",
                                GIMP_PARASITE_PERSISTENT,
                                strlen (str) + 1, (gpointer) str);
  g_free (str);
  gimp_image_attach_parasite (image, parasite);
  gimp_parasite_free (parasite);

  if (ani_info->inam && strlen (ani_info->inam) > 0)
    {
      str = g_strdup_printf ("%s", ani_info->inam);
      parasite = gimp_parasite_new ("ani-info-inam",
                                    GIMP_PARASITE_PERSISTENT,
                                    strlen (ani_info->inam) + 1, (gpointer) str);
      g_free (str);
      gimp_image_attach_parasite (image, parasite);
      gimp_parasite_free (parasite);
    }
  if (ani_info->iart && strlen (ani_info->iart) > 0)
    {
      str = g_strdup_printf ("%s", ani_info->iart);
      parasite = gimp_parasite_new ("ani-info-iart",
                                    GIMP_PARASITE_PERSISTENT,
                                    strlen (ani_info->iart) + 1, (gpointer) str);
      g_free (str);
      gimp_image_attach_parasite (image, parasite);
      gimp_parasite_free (parasite);
    }

  gimp_progress_update (1.0);

  return GIMP_PDB_SUCCESS;
}
