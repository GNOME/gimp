/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * GIMP PSD Plug-in
 * Copyright 2007 by John Marshall
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
#include <errno.h>

#include <glib/gstdio.h>
#include <libgimp/gimp.h>

#include "psd.h"
#include "psd-util.h"
#include "psd-image-res-load.h"
#include "psd-thumb-load.h"

#include "libgimp/stdplugins-intl.h"

/*  Local function prototypes  */
static gint    read_header_block          (PSDimage     *img_a,
                                           FILE         *f,
                                           GError      **error);

static gint    read_color_mode_block      (PSDimage     *img_a,
                                           FILE         *f,
                                           GError      **error);

static gint    read_image_resource_block  (PSDimage     *img_a,
                                           FILE         *f,
                                           GError      **error);

static gint32  create_gimp_image          (PSDimage     *img_a,
                                           const gchar  *filename);

static gint    add_image_resources        (gint32        image_id,
                                           PSDimage     *img_a,
                                           FILE         *f,
                                           GError      **error);

/* Main file load function */
gint32
load_thumbnail_image (const gchar  *filename,
                      gint         *width,
                      gint         *height,
                      GError      **load_error)
{
  FILE        *f;
  GStatBuf     st;
  PSDimage     img_a;
  gint32       image_id = -1;
  GError      *error    = NULL;

  /* ----- Open PSD file ----- */
  if (g_stat (filename, &st) == -1)
    return -1;

  gimp_progress_init_printf (_("Opening thumbnail for '%s'"),
                             gimp_filename_to_utf8 (filename));

  IFDBG(1) g_debug ("Open file %s", gimp_filename_to_utf8 (filename));
  f = g_fopen (filename, "rb");
  if (f == NULL)
    {
      g_set_error (load_error, G_FILE_ERROR, g_file_error_from_errno (errno),
                   _("Could not open '%s' for reading: %s"),
                   gimp_filename_to_utf8 (filename), g_strerror (errno));
      return -1;
    }

  /* ----- Read the PSD file Header block ----- */
  IFDBG(2) g_debug ("Read header block");
  if (read_header_block (&img_a, f, &error) < 0)
    goto load_error;
  gimp_progress_update (0.2);

  /* ----- Read the PSD file Color Mode block ----- */
  IFDBG(2) g_debug ("Read color mode block");
  if (read_color_mode_block (&img_a, f, &error) < 0)
    goto load_error;
  gimp_progress_update (0.4);

  /* ----- Read the PSD file Image Resource block ----- */
  IFDBG(2) g_debug ("Read image resource block");
  if (read_image_resource_block (&img_a, f, &error) < 0)
    goto load_error;
  gimp_progress_update (0.6);

  /* ----- Create GIMP image ----- */
  IFDBG(2) g_debug ("Create GIMP image");
  image_id = create_gimp_image (&img_a, filename);
  if (image_id < 0)
    goto load_error;

  /* ----- Add image resources ----- */
  IFDBG(2) g_debug ("Add image resources");
  if (add_image_resources (image_id, &img_a, f, &error) < 1)
    goto load_error;
  gimp_progress_update (1.0);

  gimp_image_clean_all (image_id);
  gimp_image_undo_enable (image_id);
  fclose (f);

  *width = img_a.columns;
  *height = img_a.rows;
  return image_id;

  /* ----- Process load errors ----- */
 load_error:
  if (error)
    {
      g_set_error (load_error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
                   _("Error loading PSD file: %s"), error->message);
      g_error_free (error);
    }

  /* Delete partially loaded image */
  if (image_id > 0)
    gimp_image_delete (image_id);

  /* Close file if Open */
  if (! (f == NULL))
    fclose (f);

  return -1;
}


/* Local functions */

static gint
read_header_block (PSDimage  *img_a,
                   FILE      *f,
                   GError   **error)
{
  guint16  version;
  gchar    sig[4];
  gchar    buf[6];

  if (fread (sig, 4, 1, f) < 1
      || fread (&version, 2, 1, f) < 1
      || fread (buf, 6, 1, f) < 1
      || fread (&img_a->channels, 2, 1, f) < 1
      || fread (&img_a->rows, 4, 1, f) < 1
      || fread (&img_a->columns, 4, 1, f) < 1
      || fread (&img_a->bps, 2, 1, f) < 1
      || fread (&img_a->color_mode, 2, 1, f) < 1)
    {
      psd_set_error (feof (f), errno, error);
      return -1;
    }
  version = GUINT16_FROM_BE (version);
  img_a->channels = GUINT16_FROM_BE (img_a->channels);
  img_a->rows = GUINT32_FROM_BE (img_a->rows);
  img_a->columns = GUINT32_FROM_BE (img_a->columns);
  img_a->bps = GUINT16_FROM_BE (img_a->bps);
  img_a->color_mode = GUINT16_FROM_BE (img_a->color_mode);

  IFDBG(1) g_debug ("\n\n\tSig: %.4s\n\tVer: %d\n\tChannels: "
                    "%d\n\tSize: %dx%d\n\tBPS: %d\n\tMode: %d\n",
                    sig, version, img_a->channels,
                    img_a->columns, img_a->rows,
                    img_a->bps, img_a->color_mode);

  if (memcmp (sig, "8BPS", 4) != 0)
    return -1;

  if (version != 1)
    return -1;

  if (img_a->channels > MAX_CHANNELS)
    return -1;

  if (img_a->rows < 1 || img_a->rows > GIMP_MAX_IMAGE_SIZE)
    return -1;

  if (img_a->columns < 1 || img_a->columns > GIMP_MAX_IMAGE_SIZE)
    return -1;

  return 0;
}

static gint
read_color_mode_block (PSDimage  *img_a,
                       FILE      *f,
                       GError   **error)
{
  guint32 block_len;
  guint32 block_start;
  guint32 block_end;

  if (fread (&block_len, 4, 1, f) < 1)
    {
      psd_set_error (feof (f), errno, error);
      return -1;
    }
  block_len = GUINT32_FROM_BE (block_len);

  block_start = ftell (f);
  block_end = block_start + block_len;

  if (fseek (f, block_end, SEEK_SET) < 0)
    {
      psd_set_error (feof (f), errno, error);
      return -1;
    }

  return 0;
}

static gint
read_image_resource_block (PSDimage  *img_a,
                           FILE      *f,
                           GError   **error)
{
  guint32 block_len;
  guint32 block_end;

  if (fread (&block_len, 4, 1, f) < 1)
    {
      psd_set_error (feof (f), errno, error);
      return -1;
    }
  img_a->image_res_len = GUINT32_FROM_BE (block_len);

  IFDBG(1) g_debug ("Image resource block size = %d", (int)img_a->image_res_len);

  img_a->image_res_start = ftell (f);
  block_end = img_a->image_res_start + img_a->image_res_len;

  if (fseek (f, block_end, SEEK_SET) < 0)
    {
      psd_set_error (feof (f), errno, error);
      return -1;
    }

  return 0;
}

static gint32
create_gimp_image (PSDimage    *img_a,
                   const gchar *filename)
{
  gint32 image_id = -1;

  img_a->base_type = GIMP_RGB;

  /* Create gimp image */
  IFDBG(2) g_debug ("Create image");
  image_id = gimp_image_new (img_a->columns, img_a->rows, img_a->base_type);

  gimp_image_set_filename (image_id, filename);
  gimp_image_undo_disable (image_id);

  return image_id;
}

static gint
add_image_resources (gint32     image_id,
                     PSDimage  *img_a,
                     FILE      *f,
                     GError   **error)
{
  PSDimageres   res_a;
  gint          status;

  if (fseek (f, img_a->image_res_start, SEEK_SET) < 0)
    {
      psd_set_error (feof (f), errno, error);
      return -1;
    }

  while (ftell (f) < img_a->image_res_start + img_a->image_res_len)
    {
      if (get_image_resource_header (&res_a, f, error) < 0)
        return -1;

      if (res_a.data_start + res_a.data_len >
          img_a->image_res_start + img_a->image_res_len)
        return 0;

      status = load_thumbnail_resource (&res_a, image_id, f, error);
      /* Error */
      if (status < 0)
        return -1;
      /* Thumbnail Loaded */
      if (status > 0)
        return 1;
    }

  return 0;
}
