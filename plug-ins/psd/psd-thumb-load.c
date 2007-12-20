/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * GIMP PSD Plug-in
 * Copyright 2007 by John Marshall
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */


#include "config.h"

#include <string.h>
#include <errno.h>

#include <glib/gstdio.h>
#include <libgimp/gimp.h>

#include "psd.h"
#include "psd-image-res-load.h"
#include "psd-thumb-load.h"

#include "libgimp/stdplugins-intl.h"


/*  Local function prototypes  */
static gint     read_header_block         (PSDimage     *img_a,
                                           FILE         *f);

static gint     read_color_mode_block     (PSDimage     *img_a,
                                           FILE         *f);

static gint     read_image_resource_block (PSDimage     *img_a,
                                           FILE         *f);

static gint32   create_gimp_image         (PSDimage     *img_a,
                                           const gchar  *filename);

static gint     add_image_resources       (const gint32  image_id,
                                           PSDimage     *img_a,
                                           FILE         *f);

/* Main file load function */
gint32
load_thumbnail_image (const gchar *filename,
                      gint        *width,
                      gint        *height)
{
  FILE        *f;
  struct stat  st;
  PSDimage     img_a;
  gint32       image_id = -1;


  /* ----- Open PSD file ----- */
  if (g_stat (filename, &st) == -1)
    return -1;

  IFDBG(1) g_debug ("Open file %s", gimp_filename_to_utf8 (filename));
  f = g_fopen (filename, "rb");
  if (f == NULL)
    {
      g_message (_("Could not open '%s' for reading: %s"),
                 gimp_filename_to_utf8 (filename), g_strerror (errno));
      return -1;
    }

  gimp_progress_init_printf (_("Loading thumbnail for '%s'"),
                             gimp_filename_to_utf8 (filename));

  IFDBG(2) g_debug ("Read header block");
  /* ----- Read the PSD file Header block ----- */
  if (read_header_block (&img_a, f) < 0)
    {
      fclose(f);
      return -1;
    }
  gimp_progress_update (0.2);

  IFDBG(2) g_debug ("Read colour mode block");
  /* ----- Read the PSD file Colour Mode block ----- */
  if (read_color_mode_block (&img_a, f) < 0)
    {
      fclose(f);
      return -1;
    }
  gimp_progress_update (0.4);

  IFDBG(2) g_debug ("Read image resource block");
  /* ----- Read the PSD file Image Resource block ----- */
  if (read_image_resource_block (&img_a, f) < 0)
    {
      fclose(f);
      return -1;
    }
  gimp_progress_update (0.6);

  IFDBG(2) g_debug ("Create GIMP image");
  /* ----- Create GIMP image ----- */
  image_id = create_gimp_image (&img_a, filename);
  if (image_id == -1)
    {
      fclose(f);
      return -1;
    }
  gimp_progress_update (0.8);

  IFDBG(2) g_debug ("Add image resources");
  /* ----- Add image resources ----- */
  if (add_image_resources (image_id, &img_a, f) < 1)
    {
      gimp_image_delete (image_id);
      fclose(f);
      return -1;
    }
  gimp_progress_update (1.0);

  gimp_image_clean_all (image_id);
  gimp_image_undo_enable (image_id);
  fclose (f);

  *width = img_a.columns;
  *height = img_a.rows;
  return image_id;
}


/* Local functions */

static gint
read_header_block (PSDimage *img_a,
                   FILE     *f)
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
      g_message (_("Error reading file header"));
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
    {
      g_message (_("Incorrect file signature"));
      return -1;
    }

  if (version != 1)
    {
      g_message (_("Unsupported PSD file format version %d"), version);
      return -1;
    }

  if (img_a->channels > MAX_CHANNELS)
    {
      g_message (_("Too many channels in file (%d)"), img_a->channels);
      return -1;
    }

  if (img_a->rows == 0 || img_a->columns == 0)
    {
      g_message (_("Unsupported PSD file version (< 2.5)")); /* FIXME - image size */
                                                             /* in resource block 1000 */
      return -1;                                             /* don't have PS2 file spec */
    }

  return 0;
}

static gint
read_color_mode_block (PSDimage *img_a,
                       FILE     *f)
{
  guint32 block_len;
  guint32 block_start;
  guint32 block_end;

  if (fread (&block_len, 4, 1, f) < 1)
    {
      g_message (_("Error reading color block"));
      return -1;
    }
  block_len = GUINT32_FROM_BE (block_len);

  IFDBG(1) g_debug ("Color map block size = %d", block_len);

  block_start = ftell (f);
  block_end = block_start + block_len;

  if (fseek (f, block_end, SEEK_SET) < 0)
    {
      g_message (_("Error setting file position"));
      return -1;
    }

  return 0;
}

static gint
read_image_resource_block (PSDimage *img_a,
                           FILE     *f)
{
  guint32 block_len;
  guint32 block_end;

  if (fread (&block_len, 4, 1, f) < 1)
    {
      g_message (_("Error reading image resource block"));
      return -1;
    }
  img_a->image_res_len = GUINT32_FROM_BE (block_len);

  IFDBG(1) g_debug ("Image resource block size = %d", (int)img_a->image_res_len);

  img_a->image_res_start = ftell (f);
  block_end = img_a->image_res_start + img_a->image_res_len;

  if (fseek (f, block_end, SEEK_SET) < 0)
    {
      g_message (_("Error setting file position"));
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
  if ((image_id = gimp_image_new (img_a->columns, img_a->rows,
                                  img_a->base_type)) == -1)
    {
      g_message (_("Could not create a new image"));
      return -1;
    }

  gimp_image_undo_disable (image_id);
  gimp_image_set_filename (image_id, filename);
  return image_id;
}

static gint
add_image_resources (const gint32  image_id,
                     PSDimage     *img_a,
                     FILE         *f)
{
  PSDimageres   res_a;
  gint          status;

  if (fseek (f, img_a->image_res_start, SEEK_SET) < 0)
    {
      g_message (_("Error setting file position"));
      return -1;
    }

  while (ftell (f) < img_a->image_res_start + img_a->image_res_len)
    {
      if (get_image_resource_header (&res_a, f) < 0)
        return -1;

      if (res_a.data_start + res_a.data_len >
          img_a->image_res_start + img_a->image_res_len)
        {
          g_message ("Unexpected end of image resource data");
          return 0;
        }
      status = load_thumbnail_resource (&res_a, image_id, f);
      /* Error */
      if (status < 0)
        return -1;
      /* Thumbnail Loaded */
      if (status > 0)
        return 1;
    }

  return 0;
}
