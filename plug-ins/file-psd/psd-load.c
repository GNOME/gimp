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
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <string.h>
#include <errno.h>

#include <glib/gstdio.h>
#include <libgimp/gimp.h>

#include "psd.h"
#include "psd-util.h"
#include "psd-image-res-load.h"
#include "psd-layer-res-load.h"
#include "psd-load.h"

#include "libgimp/stdplugins-intl.h"


#define COMP_MODE_SIZE sizeof(guint16)


/*  Local function prototypes  */
static gint             read_header_block          (PSDimage     *img_a,
                                                    FILE         *f,
                                                    GError      **error);

static gint             read_color_mode_block      (PSDimage     *img_a,
                                                    FILE         *f,
                                                    GError      **error);

static gint             read_image_resource_block  (PSDimage     *img_a,
                                                    FILE         *f,
                                                    GError      **error);

static PSDlayer **      read_layer_block           (PSDimage     *img_a,
                                                    FILE         *f,
                                                    GError      **error);

static gint             read_merged_image_block    (PSDimage     *img_a,
                                                    FILE         *f,
                                                    GError      **error);

static gint32           create_gimp_image          (PSDimage     *img_a,
                                                    const gchar  *filename);

static gint             add_color_map              (const gint32  image_id,
                                                    PSDimage     *img_a);

static gint             add_image_resources        (const gint32  image_id,
                                                    PSDimage     *img_a,
                                                    FILE         *f,
                                                    GError      **error);

static gint             add_layers                 (const gint32  image_id,
                                                    PSDimage     *img_a,
                                                    PSDlayer    **lyr_a,
                                                    FILE         *f,
                                                    GError      **error);

static gint             add_merged_image           (const gint32  image_id,
                                                    PSDimage     *img_a,
                                                    FILE         *f,
                                                    GError      **error);

/*  Local utility function prototypes  */
static gchar          * get_psd_color_mode_name    (PSDColorMode  mode);

static void             psd_to_gimp_color_map      (guchar       *map256);

static GimpImageType    get_gimp_image_type        (const GimpImageBaseType image_base_type,
                                                    const gboolean          alpha);

static gint             read_channel_data          (PSDchannel     *channel,
                                                    const guint16   bps,
                                                    const guint16   compression,
                                                    const guint16  *rle_pack_len,
                                                    FILE           *f,
                                                    GError        **error);

static void             convert_16_bit             (const gchar *src,
                                                    gchar       *dst,
                                                    guint32      len);

static void             convert_1_bit              (const gchar *src,
                                                    gchar       *dst,
                                                    guint32      rows,
                                                    guint32      columns);


/* Main file load function */
gint32
load_image (const gchar  *filename,
            GError      **load_error)
{
  FILE                 *f;
  struct stat           st;
  PSDimage              img_a;
  PSDlayer            **lyr_a;
  gint32                image_id = -1;
  GError               *error = NULL;

  /* ----- Open PSD file ----- */
  if (g_stat (filename, &st) == -1)
    return -1;

  IFDBG(1) g_debug ("Open file %s", gimp_filename_to_utf8 (filename));
  f = g_fopen (filename, "rb");
  if (f == NULL)
    {
      g_set_error (load_error, G_FILE_ERROR, g_file_error_from_errno (errno),
                   _("Could not open '%s' for reading: %s"),
                   gimp_filename_to_utf8 (filename), g_strerror (errno));
      return -1;
    }

  gimp_progress_init_printf (_("Opening '%s'"),
                             gimp_filename_to_utf8 (filename));

  /* ----- Read the PSD file Header block ----- */
  IFDBG(2) g_debug ("Read header block");
  if (read_header_block (&img_a, f, &error) < 0)
    goto load_error;
  gimp_progress_update (0.1);

  /* ----- Read the PSD file Colour Mode block ----- */
  IFDBG(2) g_debug ("Read colour mode block");
  if (read_color_mode_block (&img_a, f, &error) < 0)
    goto load_error;
  gimp_progress_update (0.2);

  /* ----- Read the PSD file Image Resource block ----- */
  IFDBG(2) g_debug ("Read image resource block");
  if (read_image_resource_block (&img_a, f, &error) < 0)
    goto load_error;
  gimp_progress_update (0.3);

  /* ----- Read the PSD file Layer & Mask block ----- */
  IFDBG(2) g_debug ("Read layer & mask block");
  lyr_a = read_layer_block (&img_a, f, &error);
  if (img_a.num_layers != 0 && lyr_a == NULL)
    goto load_error;
  gimp_progress_update (0.4);

  /* ----- Read the PSD file Merged Image Data block ----- */
  IFDBG(2) g_debug ("Read merged image and extra alpha channel block");
  if (read_merged_image_block (&img_a, f, &error) < 0)
    goto load_error;
  gimp_progress_update (0.5);

  /* ----- Create GIMP image ----- */
  IFDBG(2) g_debug ("Create GIMP image");
  image_id = create_gimp_image (&img_a, filename);
  if (image_id < 0)
    goto load_error;
  gimp_progress_update (0.6);

  /* ----- Add colour map ----- */
  IFDBG(2) g_debug ("Add color map");
  if (add_color_map (image_id, &img_a) < 0)
    goto load_error;
  gimp_progress_update (0.7);

  /* ----- Add image resources ----- */
  IFDBG(2) g_debug ("Add image resources");
  if (add_image_resources (image_id, &img_a, f, &error) < 0)
    goto load_error;
  gimp_progress_update (0.8);

  /* ----- Add layers -----*/
  IFDBG(2) g_debug ("Add layers");
  if (add_layers (image_id, &img_a, lyr_a, f, &error) < 0)
    goto load_error;
  gimp_progress_update (0.9);

  /* ----- Add merged image data and extra alpha channels ----- */
  IFDBG(2) g_debug ("Add merged image data and extra alpha channels");
  if (add_merged_image (image_id, &img_a, f, &error) < 0)
    goto load_error;
  gimp_progress_update (1.0);

  IFDBG(2) g_debug ("Close file & return, image id: %d", image_id);
  IFDBG(1) g_debug ("\n----------------------------------------"
                    "----------------------------------------\n");

  gimp_image_clean_all (image_id);
  gimp_image_undo_enable (image_id);
  fclose (f);
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
    {
      g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
                  _("Not a valid photoshop document file"));
      return -1;
    }

  if (version != 1)
    {
      g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
                  _("Unsupported file format version: %d"), version);
      return -1;
    }

  if (img_a->channels > MAX_CHANNELS)
    {
      g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
                  _("Too many channels in file: %d"), img_a->channels);
      return -1;
    }

    /* Photoshop CS (version 8) supports 300000 x 300000, but this
       is currently larger than GIMP_MAX_IMAGE_SIZE */

  if (img_a->rows < 1 || img_a->rows > GIMP_MAX_IMAGE_SIZE)
    {
      g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
                  _("Unsupported or invalid image height: %d"),
                  img_a->rows);
      return -1;
    }

  if (img_a->columns < 1 || img_a->columns > GIMP_MAX_IMAGE_SIZE)
    {
      g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
                  _("Unsupported or invalid image width: %d"),
                  img_a->columns);
      return -1;
    }

  /* img_a->rows is sanitized above, so a division by zero is avoided here */
  if (img_a->columns > G_MAXINT32 / img_a->rows)
    {
      g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
                   _("Unsupported or invalid image size: %dx%d"),
                   img_a->columns, img_a->rows);
      return -1;
    }

  if (img_a->color_mode != PSD_BITMAP
      && img_a->color_mode != PSD_GRAYSCALE
      && img_a->color_mode != PSD_INDEXED
      && img_a->color_mode != PSD_RGB
      && img_a->color_mode != PSD_DUOTONE)
    {
      g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
                   _("Unsupported color mode: %s"),
                   get_psd_color_mode_name (img_a->color_mode));
      return -1;
    }

  /* Warnings for format conversions */
  switch (img_a->bps)
    {
      case 16:
        IFDBG(3) g_debug ("16 Bit Data");
        if (CONVERSION_WARNINGS)
          g_message (_("Warning:\n"
                       "The image you are loading has 16 bits per channel. GIMP "
                       "can only handle 8 bit, so it will be converted for you. "
                       "Information will be lost because of this conversion."));
        break;

      case 8:
        IFDBG(3) g_debug ("8 Bit Data");
        break;

      case 1:
        IFDBG(3) g_debug ("1 Bit Data");
        break;

      default:
        g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
                    _("Unsupported bit depth: %d"), img_a->bps);
        return -1;
        break;
    }

  return 0;
}

static gint
read_color_mode_block (PSDimage  *img_a,
                       FILE      *f,
                       GError   **error)
{
  static guchar cmap[] = {0, 0, 0, 255, 255, 255};
  guint32       block_len;

  img_a->color_map_entries = 0;
  img_a->color_map_len = 0;
  if (fread (&block_len, 4, 1, f) < 1)
    {
      psd_set_error (feof (f), errno, error);
      return -1;
    }
  block_len = GUINT32_FROM_BE (block_len);

  IFDBG(1) g_debug ("Color map block size = %d", block_len);

  if (block_len == 0)
    {
      if (img_a->color_mode == PSD_INDEXED ||
          img_a->color_mode == PSD_DUOTONE )
        {
          IFDBG(1) g_debug ("No color block for indexed or duotone image");
          g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
                      _("The file is corrupt!"));
          return -1;
        }
    }
  else if (img_a->color_mode == PSD_INDEXED)
    {
      if (block_len != 768)
        {
          IFDBG(1) g_debug ("Invalid color block size for indexed image");
          g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
                      _("The file is corrupt!"));
          return -1;
        }
      else
        {
          img_a->color_map_len = block_len;
          img_a->color_map = g_malloc (img_a->color_map_len);
          if (fread (img_a->color_map, block_len, 1, f) < 1)
            {
              psd_set_error (feof (f), errno, error);
              return -1;
            }
          else
            {
              psd_to_gimp_color_map (img_a->color_map);
              img_a->color_map_entries = 256;
            }
        }
    }
  else if (img_a->color_mode == PSD_DUOTONE)
    {
      img_a->color_map_len = block_len;
      img_a->color_map = g_malloc (img_a->color_map_len);
      if (fread (img_a->color_map, block_len, 1, f) < 1)
        {
          psd_set_error (feof (f), errno, error);
          return -1;
        }
    }

  /* Create color map for bitmap image */
  if (img_a->color_mode == PSD_BITMAP)
    {
      img_a->color_map_len = 6;
      img_a->color_map = g_malloc (img_a->color_map_len);
      memcpy (img_a->color_map, cmap, img_a->color_map_len);
      img_a->color_map_entries = 2;
    }
  IFDBG(2) g_debug ("Colour map data length %d", img_a->color_map_len);

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

static PSDlayer **
read_layer_block (PSDimage  *img_a,
                  FILE      *f,
                  GError   **error)
{
  PSDlayer **lyr_a;
  guint32    block_len;
  guint32    block_end;
  guint32    block_rem;
  gint32     read_len;
  gint32     write_len;
  gint       lidx;                  /* Layer index */
  gint       cidx;                  /* Channel index */

  if (fread (&block_len, 4, 1, f) < 1)
    {
      psd_set_error (feof (f), errno, error);
      img_a->num_layers = -1;
      return NULL;
    }
  img_a->mask_layer_len = GUINT32_FROM_BE (block_len);

  IFDBG(1) g_debug ("Layer and mask block size = %d", img_a->mask_layer_len);

  img_a->transparency = FALSE;
  img_a->layer_data_len = 0;

  if (!img_a->mask_layer_len)
    {
      img_a->num_layers = 0;
      return NULL;
    }
  else
    {
      img_a->mask_layer_start = ftell (f);
      block_end = img_a->mask_layer_start + img_a->mask_layer_len;

      /* Get number of layers */
      if (fread (&block_len, 4, 1, f) < 1
          || fread (&img_a->num_layers, 2, 1, f) < 1)
        {
          psd_set_error (feof (f), errno, error);
          img_a->num_layers = -1;
          return NULL;
        }
      img_a->num_layers = GINT16_FROM_BE (img_a->num_layers);
      IFDBG(2) g_debug ("Number of layers: %d", img_a->num_layers);

      if (img_a->num_layers < 0)
        {
          img_a->transparency = TRUE;
          img_a->num_layers = -img_a->num_layers;
        }

      if (img_a->num_layers)
        {
          /* Read layer records */
          PSDlayerres           res_a;

          /* Create pointer array for the layer records */
          lyr_a = g_new (PSDlayer *, img_a->num_layers);
          for (lidx = 0; lidx < img_a->num_layers; ++lidx)
            {
              /* Allocate layer record */
              lyr_a[lidx] = (PSDlayer *) g_malloc (sizeof (PSDlayer) );

              /* Initialise record */
              lyr_a[lidx]->drop = FALSE;
              lyr_a[lidx]->id = 0;

              if (fread (&lyr_a[lidx]->top, 4, 1, f) < 1
                  || fread (&lyr_a[lidx]->left, 4, 1, f) < 1
                  || fread (&lyr_a[lidx]->bottom, 4, 1, f) < 1
                  || fread (&lyr_a[lidx]->right, 4, 1, f) < 1
                  || fread (&lyr_a[lidx]->num_channels, 2, 1, f) < 1)
                {
                  psd_set_error (feof (f), errno, error);
                  return NULL;
                }
              lyr_a[lidx]->top = GINT32_FROM_BE (lyr_a[lidx]->top);
              lyr_a[lidx]->left = GINT32_FROM_BE (lyr_a[lidx]->left);
              lyr_a[lidx]->bottom = GINT32_FROM_BE (lyr_a[lidx]->bottom);
              lyr_a[lidx]->right = GINT32_FROM_BE (lyr_a[lidx]->right);
              lyr_a[lidx]->num_channels = GUINT16_FROM_BE (lyr_a[lidx]->num_channels);

              if (lyr_a[lidx]->num_channels > MAX_CHANNELS)
                {
                  g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
                              _("Too many channels in layer: %d"),
                              lyr_a[lidx]->num_channels);
                  return NULL;
                }
              if (lyr_a[lidx]->bottom < lyr_a[lidx]->top ||
                  lyr_a[lidx]->bottom - lyr_a[lidx]->top > GIMP_MAX_IMAGE_SIZE)
                {
                  g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
                              _("Unsupported or invalid layer height: %d"),
                              lyr_a[lidx]->bottom - lyr_a[lidx]->top);
                  return NULL;
                }
              if (lyr_a[lidx]->right < lyr_a[lidx]->left ||
                  lyr_a[lidx]->right - lyr_a[lidx]->left > GIMP_MAX_IMAGE_SIZE)
                {
                  g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
                              _("Unsupported or invalid layer width: %d"),
                              lyr_a[lidx]->right - lyr_a[lidx]->left);
                  return NULL;
                }

              if ((lyr_a[lidx]->right - lyr_a[lidx]->left) >
                  G_MAXINT32 / MAX (lyr_a[lidx]->bottom - lyr_a[lidx]->top, 1))
                {
                  g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
                               _("Unsupported or invalid layer size: %dx%d"),
                               lyr_a[lidx]->right - lyr_a[lidx]->left,
                               lyr_a[lidx]->bottom - lyr_a[lidx]->top);
                  return NULL;
                }

              IFDBG(2) g_debug ("Layer %d, Coords %d %d %d %d, channels %d, ",
                                 lidx, lyr_a[lidx]->left, lyr_a[lidx]->top,
                                 lyr_a[lidx]->right, lyr_a[lidx]->bottom,
                                 lyr_a[lidx]->num_channels);

              lyr_a[lidx]->chn_info = g_new (ChannelLengthInfo, lyr_a[lidx]->num_channels);
              for (cidx = 0; cidx < lyr_a[lidx]->num_channels; ++cidx)
                {
                  if (fread (&lyr_a[lidx]->chn_info[cidx].channel_id, 2, 1, f) < 1
                      || fread (&lyr_a[lidx]->chn_info[cidx].data_len, 4, 1, f) < 1)
                    {
                      psd_set_error (feof (f), errno, error);
                      return NULL;
                    }
                  lyr_a[lidx]->chn_info[cidx].channel_id =
                    GINT16_FROM_BE (lyr_a[lidx]->chn_info[cidx].channel_id);
                  lyr_a[lidx]->chn_info[cidx].data_len =
                    GUINT32_FROM_BE (lyr_a[lidx]->chn_info[cidx].data_len);
                  img_a->layer_data_len += lyr_a[lidx]->chn_info[cidx].data_len;
                  IFDBG(3) g_debug ("Channel ID %d, data len %d",
                                     lyr_a[lidx]->chn_info[cidx].channel_id,
                                     lyr_a[lidx]->chn_info[cidx].data_len);
                }

              if (fread (lyr_a[lidx]->mode_key, 4, 1, f) < 1
                  || fread (lyr_a[lidx]->blend_mode, 4, 1, f) < 1
                  || fread (&lyr_a[lidx]->opacity, 1, 1, f) < 1
                  || fread (&lyr_a[lidx]->clipping, 1, 1, f) < 1
                  || fread (&lyr_a[lidx]->flags, 1, 1, f) < 1
                  || fread (&lyr_a[lidx]->filler, 1, 1, f) < 1
                  || fread (&lyr_a[lidx]->extra_len, 4, 1, f) < 1)
                {
                  psd_set_error (feof (f), errno, error);
                  return NULL;
                }
              if (memcmp (lyr_a[lidx]->mode_key, "8BIM", 4) != 0)
                {
                  IFDBG(1) g_debug ("Incorrect layer mode signature %.4s",
                                    lyr_a[lidx]->mode_key);
                  g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
                              _("The file is corrupt!"));
                  return NULL;
                }

              lyr_a[lidx]->layer_flags.trans_prot = lyr_a[lidx]->flags & 1 ? TRUE : FALSE;
              lyr_a[lidx]->layer_flags.visible = lyr_a[lidx]->flags & 2 ? FALSE : TRUE;
              if (lyr_a[lidx]->flags & 8)
                lyr_a[lidx]->layer_flags.irrelevant = lyr_a[lidx]->flags & 16 ? TRUE : FALSE;
              else
                lyr_a[lidx]->layer_flags.irrelevant = FALSE;

              lyr_a[lidx]->extra_len = GUINT32_FROM_BE (lyr_a[lidx]->extra_len);
              block_rem = lyr_a[lidx]->extra_len;
              IFDBG(2) g_debug ("\n\tLayer mode sig: %.4s\n\tBlend mode: %.4s\n\t"
                                "Opacity: %d\n\tClipping: %d\n\tExtra data len: %d\n\t"
                                "Alpha lock: %d\n\tVisible: %d\n\tIrrelevant: %d",
                                    lyr_a[lidx]->mode_key,
                                    lyr_a[lidx]->blend_mode,
                                    lyr_a[lidx]->opacity,
                                    lyr_a[lidx]->clipping,
                                    lyr_a[lidx]->extra_len,
                                    lyr_a[lidx]->layer_flags.trans_prot,
                                    lyr_a[lidx]->layer_flags.visible,
                                    lyr_a[lidx]->layer_flags.irrelevant);
              IFDBG(3) g_debug ("Remaining length %d", block_rem);

              /* Layer mask data */
              if (fread (&block_len, 4, 1, f) < 1)
                {
                  psd_set_error (feof (f), errno, error);
                  return NULL;
                }
              block_len = GUINT32_FROM_BE (block_len);
              block_rem -= (block_len + 4);
              IFDBG(3) g_debug ("Remaining length %d", block_rem);

              lyr_a[lidx]->layer_mask_extra.top = 0;
              lyr_a[lidx]->layer_mask_extra.left = 0;
              lyr_a[lidx]->layer_mask_extra.bottom = 0;
              lyr_a[lidx]->layer_mask_extra.right = 0;
              lyr_a[lidx]->layer_mask.top = 0;
              lyr_a[lidx]->layer_mask.left = 0;
              lyr_a[lidx]->layer_mask.bottom = 0;
              lyr_a[lidx]->layer_mask.right = 0;
              lyr_a[lidx]->layer_mask.def_color = 0;
              lyr_a[lidx]->layer_mask.extra_def_color = 0;
              lyr_a[lidx]->layer_mask.mask_flags.relative_pos = FALSE;
              lyr_a[lidx]->layer_mask.mask_flags.disabled = FALSE;
              lyr_a[lidx]->layer_mask.mask_flags.invert = FALSE;

              switch (block_len)
                {
                  case 0:
                    break;

                  case 20:
                    if (fread (&lyr_a[lidx]->layer_mask.top, 4, 1, f) < 1
                        || fread (&lyr_a[lidx]->layer_mask.left, 4, 1, f) < 1
                        || fread (&lyr_a[lidx]->layer_mask.bottom, 4, 1, f) < 1
                        || fread (&lyr_a[lidx]->layer_mask.right, 4, 1, f) < 1
                        || fread (&lyr_a[lidx]->layer_mask.def_color, 1, 1, f) < 1
                        || fread (&lyr_a[lidx]->layer_mask.flags, 1, 1, f) < 1
                        || fread (&lyr_a[lidx]->layer_mask.extra_def_color, 1, 1, f) < 1
                        || fread (&lyr_a[lidx]->layer_mask.extra_flags, 1, 1, f) < 1)
                      {
                        psd_set_error (feof (f), errno, error);
                        return NULL;
                      }
                    lyr_a[lidx]->layer_mask.top =
                      GINT32_FROM_BE (lyr_a[lidx]->layer_mask.top);
                    lyr_a[lidx]->layer_mask.left =
                      GINT32_FROM_BE (lyr_a[lidx]->layer_mask.left);
                    lyr_a[lidx]->layer_mask.bottom =
                      GINT32_FROM_BE (lyr_a[lidx]->layer_mask.bottom);
                    lyr_a[lidx]->layer_mask.right =
                      GINT32_FROM_BE (lyr_a[lidx]->layer_mask.right);
                    lyr_a[lidx]->layer_mask.mask_flags.relative_pos =
                      lyr_a[lidx]->layer_mask.flags & 1 ? TRUE : FALSE;
                    lyr_a[lidx]->layer_mask.mask_flags.disabled =
                      lyr_a[lidx]->layer_mask.flags & 2 ? TRUE : FALSE;
                    lyr_a[lidx]->layer_mask.mask_flags.invert =
                      lyr_a[lidx]->layer_mask.flags & 4 ? TRUE : FALSE;
                    break;
                  case 36: /* If we have a 36 byte mask record assume second data set is correct */
                    if (fread (&lyr_a[lidx]->layer_mask_extra.top, 4, 1, f) < 1
                        || fread (&lyr_a[lidx]->layer_mask_extra.left, 4, 1, f) < 1
                        || fread (&lyr_a[lidx]->layer_mask_extra.bottom, 4, 1, f) < 1
                        || fread (&lyr_a[lidx]->layer_mask_extra.right, 4, 1, f) < 1
                        || fread (&lyr_a[lidx]->layer_mask.extra_def_color, 1, 1, f) < 1
                        || fread (&lyr_a[lidx]->layer_mask.extra_flags, 1, 1, f) < 1
                        || fread (&lyr_a[lidx]->layer_mask.def_color, 1, 1, f) < 1
                        || fread (&lyr_a[lidx]->layer_mask.flags, 1, 1, f) < 1
                        || fread (&lyr_a[lidx]->layer_mask.top, 4, 1, f) < 1
                        || fread (&lyr_a[lidx]->layer_mask.left, 4, 1, f) < 1
                        || fread (&lyr_a[lidx]->layer_mask.bottom, 4, 1, f) < 1
                        || fread (&lyr_a[lidx]->layer_mask.right, 4, 1, f) < 1)
                      {
                        psd_set_error (feof (f), errno, error);
                        return NULL;
                      }
                    lyr_a[lidx]->layer_mask_extra.top =
                      GINT32_FROM_BE (lyr_a[lidx]->layer_mask_extra.top);
                    lyr_a[lidx]->layer_mask_extra.left =
                      GINT32_FROM_BE (lyr_a[lidx]->layer_mask_extra.left);
                    lyr_a[lidx]->layer_mask_extra.bottom =
                      GINT32_FROM_BE (lyr_a[lidx]->layer_mask_extra.bottom);
                    lyr_a[lidx]->layer_mask_extra.right =
                      GINT32_FROM_BE (lyr_a[lidx]->layer_mask_extra.right);
                    lyr_a[lidx]->layer_mask.top =
                      GINT32_FROM_BE (lyr_a[lidx]->layer_mask.top);
                    lyr_a[lidx]->layer_mask.left =
                      GINT32_FROM_BE (lyr_a[lidx]->layer_mask.left);
                    lyr_a[lidx]->layer_mask.bottom =
                      GINT32_FROM_BE (lyr_a[lidx]->layer_mask.bottom);
                    lyr_a[lidx]->layer_mask.right =
                      GINT32_FROM_BE (lyr_a[lidx]->layer_mask.right);
                    lyr_a[lidx]->layer_mask.mask_flags.relative_pos =
                      lyr_a[lidx]->layer_mask.flags & 1 ? TRUE : FALSE;
                    lyr_a[lidx]->layer_mask.mask_flags.disabled =
                      lyr_a[lidx]->layer_mask.flags & 2 ? TRUE : FALSE;
                    lyr_a[lidx]->layer_mask.mask_flags.invert =
                      lyr_a[lidx]->layer_mask.flags & 4 ? TRUE : FALSE;
                    break;

                  default:
                    IFDBG(1) g_debug ("Unknown layer mask record size ... skipping");
                    if (fseek (f, block_len, SEEK_CUR) < 0)
                      {
                        psd_set_error (feof (f), errno, error);
                        return NULL;
                      }
                }

              /* sanity checks */
              if (lyr_a[lidx]->layer_mask.bottom < lyr_a[lidx]->layer_mask.top ||
                  lyr_a[lidx]->layer_mask.bottom - lyr_a[lidx]->layer_mask.top > GIMP_MAX_IMAGE_SIZE)
                {
                  g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
                               _("Unsupported or invalid layer mask height: %d"),
                               lyr_a[lidx]->layer_mask.bottom - lyr_a[lidx]->layer_mask.top);
                  return NULL;
                }
              if (lyr_a[lidx]->layer_mask.right < lyr_a[lidx]->layer_mask.left ||
                  lyr_a[lidx]->layer_mask.right - lyr_a[lidx]->layer_mask.left > GIMP_MAX_IMAGE_SIZE)
                {
                  g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
                               _("Unsupported or invalid layer mask width: %d"),
                               lyr_a[lidx]->layer_mask.right - lyr_a[lidx]->layer_mask.left);
                  return NULL;
                }

              if ((lyr_a[lidx]->layer_mask.right - lyr_a[lidx]->layer_mask.left) >
                  G_MAXINT32 / MAX (lyr_a[lidx]->layer_mask.bottom - lyr_a[lidx]->layer_mask.top, 1))
                {
                  g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
                               _("Unsupported or invalid layer mask size: %dx%d"),
                               lyr_a[lidx]->layer_mask.right - lyr_a[lidx]->layer_mask.left,
                               lyr_a[lidx]->layer_mask.bottom - lyr_a[lidx]->layer_mask.top);
                  return NULL;
                }

              IFDBG(2) g_debug ("Layer mask coords %d %d %d %d, Rel pos %d",
                                lyr_a[lidx]->layer_mask.left,
                                lyr_a[lidx]->layer_mask.top,
                                lyr_a[lidx]->layer_mask.right,
                                lyr_a[lidx]->layer_mask.bottom,
                                lyr_a[lidx]->layer_mask.mask_flags.relative_pos);

              IFDBG(3) g_debug ("Default mask color, %d, %d",
                                lyr_a[lidx]->layer_mask.def_color,
                                lyr_a[lidx]->layer_mask.extra_def_color);

              /* Layer blending ranges */           /* FIXME  */
              if (fread (&block_len, 4, 1, f) < 1)
                {
                  psd_set_error (feof (f), errno, error);
                  return NULL;
                }
              block_len = GUINT32_FROM_BE (block_len);
              block_rem -= (block_len + 4);
              IFDBG(3) g_debug ("Remaining length %d", block_rem);
              if (block_len > 0)
                {
                  if (fseek (f, block_len, SEEK_CUR) < 0)
                    {
                      psd_set_error (feof (f), errno, error);
                      return NULL;
                    }
                }

              lyr_a[lidx]->name = fread_pascal_string (&read_len, &write_len,
                                                       4, f, error);
              if (*error)
                return NULL;
              block_rem -= read_len;
              IFDBG(3) g_debug ("Remaining length %d", block_rem);

              /* Adjustment layer info */           /* FIXME */

              while (block_rem > 7)
                {
                  if (get_layer_resource_header (&res_a, f, error) < 0)
                    return NULL;
                  block_rem -= 12;

                  if (res_a.data_len > block_rem)
                    {
                      IFDBG(1) g_debug ("Unexpected end of layer resource data");
                      g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
                                  _("The file is corrupt!"));
                      return NULL;
                    }

                  if (load_layer_resource (&res_a, lyr_a[lidx], f, error) < 0)
                    return NULL;
                  block_rem -= res_a.data_len;
                }
              if (block_rem > 0)
                {
                  if (fseek (f, block_rem, SEEK_CUR) < 0)
                    {
                      psd_set_error (feof (f), errno, error);
                      return NULL;
                    }
                }
            }

          img_a->layer_data_start = ftell(f);
          if (fseek (f, img_a->layer_data_len, SEEK_CUR) < 0)
            {
              psd_set_error (feof (f), errno, error);
              return NULL;
            }

          IFDBG(1) g_debug ("Layer image data block size %d",
                             img_a->layer_data_len);
        }
      else
        lyr_a = NULL;

      /* Read global layer mask record */       /* FIXME */

      /* Skip to end of block */
      if (fseek (f, block_end, SEEK_SET) < 0)
        {
          psd_set_error (feof (f), errno, error);
          return NULL;
        }
    }

  return lyr_a;
}

static gint
read_merged_image_block (PSDimage  *img_a,
                         FILE      *f,
                         GError   **error)
{
  img_a->merged_image_start = ftell(f);
  if (fseek (f, 0, SEEK_END) < 0)
    {
      psd_set_error (feof (f), errno, error);
      return -1;
    }

  img_a->merged_image_len = ftell(f) - img_a->merged_image_start;

  IFDBG(1) g_debug ("Merged image data block: Start: %d, len: %d",
                     img_a->merged_image_start, img_a->merged_image_len);

  return 0;
}

static gint32
create_gimp_image (PSDimage    *img_a,
                   const gchar *filename)
{
  gint32 image_id = -1;

  switch (img_a->color_mode)
    {
      case PSD_GRAYSCALE:
      case PSD_DUOTONE:
        img_a->base_type = GIMP_GRAY;
        break;

      case PSD_BITMAP:
      case PSD_INDEXED:
        img_a->base_type = GIMP_INDEXED;
        break;

      case PSD_RGB:
        img_a->base_type = GIMP_RGB;
        break;

      default:
        /* Color mode already validated - should not be here */
        g_warning ("Invalid color mode");
        return -1;
        break;
    }

  /* Create gimp image */
  IFDBG(2) g_debug ("Create image");
  image_id = gimp_image_new (img_a->columns, img_a->rows, img_a->base_type);

  gimp_image_set_filename (image_id, filename);
  gimp_image_undo_disable (image_id);

  return image_id;
}

static gint
add_color_map (const gint32  image_id,
               PSDimage     *img_a)
{
  GimpParasite *parasite;

  if (img_a->color_map_len)
    {
      if (img_a->color_mode != PSD_DUOTONE)
        gimp_image_set_colormap (image_id, img_a->color_map, img_a->color_map_entries);
      else
        {
           /* Add parasite for Duotone color data */
          IFDBG(2) g_debug ("Add Duotone color data parasite");
          parasite = gimp_parasite_new (PSD_PARASITE_DUOTONE_DATA, 0,
                                        img_a->color_map_len, img_a->color_map);
          gimp_image_parasite_attach (image_id, parasite);
          gimp_parasite_free (parasite);
        }
      g_free (img_a->color_map);
    }

  return 0;
}

static gint
add_image_resources (const gint32  image_id,
                     PSDimage     *img_a,
                     FILE         *f,
                     GError      **error)
{
  PSDimageres  res_a;

  if (fseek (f, img_a->image_res_start, SEEK_SET) < 0)
    {
      psd_set_error (feof (f), errno, error);
      return -1;
    }

  /* Initialise image resource variables */
  img_a->no_icc = FALSE;
  img_a->layer_state = 0;
  img_a->alpha_names = NULL;
  img_a->alpha_display_info = NULL;
  img_a->alpha_display_count = 0;
  img_a->alpha_id = NULL;
  img_a->alpha_id_count = 0;
  img_a->quick_mask_id = 0;

  while (ftell (f) < img_a->image_res_start + img_a->image_res_len)
    {
      if (get_image_resource_header (&res_a, f, error) < 0)
        return -1;

      if (res_a.data_start + res_a.data_len >
          img_a->image_res_start + img_a->image_res_len)
        {
          IFDBG(1) g_debug ("Unexpected end of image resource data");
          return 0;
        }

      if (load_image_resource (&res_a, image_id, img_a, f, error) < 0)
        return -1;
    }

  return 0;
}

static gint
add_layers (const gint32  image_id,
            PSDimage     *img_a,
            PSDlayer    **lyr_a,
            FILE         *f,
            GError      **error)
{
  PSDchannel          **lyr_chn;
  guchar               *pixels;
  guint16               alpha_chn;
  guint16               user_mask_chn;
  guint16               layer_channels;
  guint16               channel_idx[MAX_CHANNELS];
  guint16              *rle_pack_len;
  gint32                l_x;                   /* Layer x */
  gint32                l_y;                   /* Layer y */
  gint32                l_w;                   /* Layer width */
  gint32                l_h;                   /* Layer height */
  gint32                lm_x;                  /* Layer mask x */
  gint32                lm_y;                  /* Layer mask y */
  gint32                lm_w;                  /* Layer mask width */
  gint32                lm_h;                  /* Layer mask height */
  gint32                layer_size;
  gint32                layer_id = -1;
  gint32                mask_id = -1;
  gint                  lidx;                  /* Layer index */
  gint                  cidx;                  /* Channel index */
  gint                  rowi;                  /* Row index */
  gint                  coli;                  /* Column index */
  gint                  i;
  gboolean              alpha;
  gboolean              user_mask;
  gboolean              empty;
  gboolean              empty_mask;
  GimpDrawable         *drawable;
  GimpPixelRgn          pixel_rgn;
  GimpImageType         image_type;
  GimpLayerModeEffects  layer_mode;


  IFDBG(2) g_debug ("Number of layers: %d", img_a->num_layers);

  if (img_a->num_layers == 0)
    {
      IFDBG(2) g_debug ("No layers to process");
      return 0;
    }

  /* Layered image - Photoshop 3 style */
  if (fseek (f, img_a->layer_data_start, SEEK_SET) < 0)
    {
      psd_set_error (feof (f), errno, error);
      return -1;
    }

  for (lidx = 0; lidx < img_a->num_layers; ++lidx)
    {
      IFDBG(2) g_debug ("Process Layer No %d.", lidx);

      if (lyr_a[lidx]->drop)
        {
          IFDBG(2) g_debug ("Drop layer %d", lidx);

          /* Step past layer data */
          for (cidx = 0; cidx < lyr_a[lidx]->num_channels; ++cidx)
            {
              if (fseek (f, lyr_a[lidx]->chn_info[cidx].data_len, SEEK_CUR) < 0)
                {
                  psd_set_error (feof (f), errno, error);
                  return -1;
                }
            }
          g_free (lyr_a[lidx]->chn_info);
          g_free (lyr_a[lidx]->name);
        }

      else
        {
          /* Empty layer */
          if (lyr_a[lidx]->bottom - lyr_a[lidx]->top == 0
              || lyr_a[lidx]->right - lyr_a[lidx]->left == 0)
              empty = TRUE;
          else
              empty = FALSE;

          /* Empty mask */
          if (lyr_a[lidx]->layer_mask.bottom - lyr_a[lidx]->layer_mask.top == 0
              || lyr_a[lidx]->layer_mask.right - lyr_a[lidx]->layer_mask.left == 0)
              empty_mask = TRUE;
          else
              empty_mask = FALSE;

          IFDBG(3) g_debug ("Empty mask %d, size %d %d", empty_mask,
                            lyr_a[lidx]->layer_mask.bottom - lyr_a[lidx]->layer_mask.top,
                            lyr_a[lidx]->layer_mask.right - lyr_a[lidx]->layer_mask.left);

          /* Load layer channel data */
          IFDBG(2) g_debug ("Number of channels: %d", lyr_a[lidx]->num_channels);
          /* Create pointer array for the channel records */
          lyr_chn = g_new (PSDchannel *, lyr_a[lidx]->num_channels);
          for (cidx = 0; cidx < lyr_a[lidx]->num_channels; ++cidx)
            {
              guint16 comp_mode = PSD_COMP_RAW;

              /* Allocate channel record */
              lyr_chn[cidx] = g_malloc (sizeof (PSDchannel) );

              lyr_chn[cidx]->id = lyr_a[lidx]->chn_info[cidx].channel_id;
              lyr_chn[cidx]->rows = lyr_a[lidx]->bottom - lyr_a[lidx]->top;
              lyr_chn[cidx]->columns = lyr_a[lidx]->right - lyr_a[lidx]->left;

              if (lyr_chn[cidx]->id == PSD_CHANNEL_MASK)
                {
                  /* Works around a bug in panotools psd files where the layer mask
                     size is given as 0 but data exists. Set mask size to layer size.
                  */
                  if (empty_mask && lyr_a[lidx]->chn_info[cidx].data_len - 2 > 0)
                    {
                      empty_mask = FALSE;
                      if (lyr_a[lidx]->layer_mask.top == lyr_a[lidx]->layer_mask.bottom)
                        {
                          lyr_a[lidx]->layer_mask.top = lyr_a[lidx]->top;
                          lyr_a[lidx]->layer_mask.bottom = lyr_a[lidx]->bottom;
                        }
                      if (lyr_a[lidx]->layer_mask.right == lyr_a[lidx]->layer_mask.left)
                        {
                          lyr_a[lidx]->layer_mask.right = lyr_a[lidx]->right;
                          lyr_a[lidx]->layer_mask.left = lyr_a[lidx]->left;
                        }
                    }
                  lyr_chn[cidx]->rows = (lyr_a[lidx]->layer_mask.bottom -
                                        lyr_a[lidx]->layer_mask.top);
                  lyr_chn[cidx]->columns = (lyr_a[lidx]->layer_mask.right -
                                           lyr_a[lidx]->layer_mask.left);
                }

              IFDBG(3) g_debug ("Channel id %d, %dx%d",
                                lyr_chn[cidx]->id,
                                lyr_chn[cidx]->columns,
                                lyr_chn[cidx]->rows);

              /* Only read channel data if there is any channel
               * data. Note that the channel data can contain a
               * compression method but no actual data.
               */
              if (lyr_a[lidx]->chn_info[cidx].data_len >= COMP_MODE_SIZE)
                {
                  if (fread (&comp_mode, COMP_MODE_SIZE, 1, f) < 1)
                    {
                      psd_set_error (feof (f), errno, error);
                      return -1;
                    }
                  comp_mode = GUINT16_FROM_BE (comp_mode);
                  IFDBG(3) g_debug ("Compression mode: %d", comp_mode);
                }
              if (lyr_a[lidx]->chn_info[cidx].data_len > COMP_MODE_SIZE)
                {
                  switch (comp_mode)
                    {
                      case PSD_COMP_RAW:        /* Planar raw data */
                        IFDBG(3) g_debug ("Raw data length: %d",
                                          lyr_a[lidx]->chn_info[cidx].data_len - 2);
                        if (read_channel_data (lyr_chn[cidx], img_a->bps,
                            PSD_COMP_RAW, NULL, f, error) < 1)
                          return -1;
                        break;

                      case PSD_COMP_RLE:        /* Packbits */
                        IFDBG(3) g_debug ("RLE channel length %d, RLE length data: %d, "
                                          "RLE data block: %d",
                                          lyr_a[lidx]->chn_info[cidx].data_len - 2,
                                          lyr_chn[cidx]->rows * 2,
                                          (lyr_a[lidx]->chn_info[cidx].data_len - 2 -
                                           lyr_chn[cidx]->rows * 2));
                        rle_pack_len = g_malloc (lyr_chn[cidx]->rows * 2);
                        for (rowi = 0; rowi < lyr_chn[cidx]->rows; ++rowi)
                          {
                            if (fread (&rle_pack_len[rowi], 2, 1, f) < 1)
                              {
                                psd_set_error (feof (f), errno, error);
                                return -1;
                              }
                            rle_pack_len[rowi] = GUINT16_FROM_BE (rle_pack_len[rowi]);
                          }

                        IFDBG(3) g_debug ("RLE decode - data");
                        if (read_channel_data (lyr_chn[cidx], img_a->bps,
                            PSD_COMP_RLE, rle_pack_len, f, error) < 1)
                          return -1;

                        g_free (rle_pack_len);
                        break;

                      case PSD_COMP_ZIP:                 /* ? */
                      case PSD_COMP_ZIP_PRED:
                      default:
                        g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
                                    _("Unsupported compression mode: %d"), comp_mode);
                        return -1;
                        break;
                    }
                }
            }
          g_free (lyr_a[lidx]->chn_info);

          /* Draw layer */

          alpha = FALSE;
          alpha_chn = -1;
          user_mask = FALSE;
          user_mask_chn = -1;
          layer_channels = 0;
          l_x = 0;
          l_y = 0;
          l_w = img_a->columns;
          l_h = img_a->rows;

          IFDBG(3) g_debug ("Re-hash channel indices");
          for (cidx = 0; cidx < lyr_a[lidx]->num_channels; ++cidx)
            {
              if (lyr_chn[cidx]->id == PSD_CHANNEL_MASK)
                {
                  user_mask = TRUE;
                  user_mask_chn = cidx;
                }
              else if (lyr_chn[cidx]->id == PSD_CHANNEL_ALPHA)
                {
                  alpha = TRUE;
                  alpha_chn = cidx;
                }
              else
                {
                  channel_idx[layer_channels] = cidx;   /* Assumes in sane order */
                  layer_channels++;                     /* RGB, Lab, CMYK etc.   */
                }
            }
          if (alpha)
            {
              channel_idx[layer_channels] = alpha_chn;
              layer_channels++;
            }

          if (empty)
            {
              IFDBG(2) g_debug ("Create blank layer");
              image_type = get_gimp_image_type (img_a->base_type, TRUE);
              layer_id = gimp_layer_new (image_id, lyr_a[lidx]->name,
                                         img_a->columns, img_a->rows,
                                         image_type, 0, GIMP_NORMAL_MODE);
              g_free (lyr_a[lidx]->name);
              gimp_image_add_layer (image_id, layer_id, -1);
              drawable = gimp_drawable_get (layer_id);
              gimp_drawable_fill (drawable->drawable_id, GIMP_TRANSPARENT_FILL);
              gimp_item_set_visible (drawable->drawable_id, lyr_a[lidx]->layer_flags.visible);
              if (lyr_a[lidx]->id)
                gimp_item_set_tattoo (drawable->drawable_id, lyr_a[lidx]->id);
              if (lyr_a[lidx]->layer_flags.irrelevant)
                gimp_item_set_visible (drawable->drawable_id, FALSE);
              gimp_drawable_flush (drawable);
              gimp_drawable_detach (drawable);
            }
          else
            {
              l_x = lyr_a[lidx]->left;
              l_y = lyr_a[lidx]->top;
              l_w = lyr_a[lidx]->right - lyr_a[lidx]->left;
              l_h = lyr_a[lidx]->bottom - lyr_a[lidx]->top;

              IFDBG(3) g_debug ("Draw layer");
              image_type = get_gimp_image_type (img_a->base_type, alpha);
              IFDBG(3) g_debug ("Layer type %d", image_type);
              layer_size = l_w * l_h;
              pixels = g_malloc (layer_size * layer_channels);
              for (cidx = 0; cidx < layer_channels; ++cidx)
                {
                  IFDBG(3) g_debug ("Start channel %d", channel_idx[cidx]);
                  for (i = 0; i < layer_size; ++i)
                    pixels[(i * layer_channels) + cidx] = lyr_chn[channel_idx[cidx]]->data[i];
                  g_free (lyr_chn[channel_idx[cidx]]->data);
                }

              layer_mode = psd_to_gimp_blend_mode (lyr_a[lidx]->blend_mode);
              layer_id = gimp_layer_new (image_id, lyr_a[lidx]->name, l_w, l_h,
                                         image_type, lyr_a[lidx]->opacity * 100 / 255,
                                         layer_mode);
              IFDBG(3) g_debug ("Layer tattoo: %d", layer_id);
              g_free (lyr_a[lidx]->name);
              gimp_image_add_layer (image_id, layer_id, -1);
              gimp_layer_set_offsets (layer_id, l_x, l_y);
              gimp_layer_set_lock_alpha  (layer_id, lyr_a[lidx]->layer_flags.trans_prot);
              drawable = gimp_drawable_get (layer_id);
              gimp_pixel_rgn_init (&pixel_rgn, drawable, 0, 0,
                                   drawable->width, drawable->height, TRUE, FALSE);
              gimp_pixel_rgn_set_rect (&pixel_rgn, pixels,
                                       0, 0, drawable->width, drawable->height);
              gimp_item_set_visible (drawable->drawable_id, lyr_a[lidx]->layer_flags.visible);
              if (lyr_a[lidx]->id)
                gimp_item_set_tattoo (drawable->drawable_id, lyr_a[lidx]->id);
              gimp_drawable_flush (drawable);
              gimp_drawable_detach (drawable);
              g_free (pixels);
            }

          /* Layer mask */
          if (user_mask)
            {
              if (empty_mask)
                {
                  IFDBG(3) g_debug ("Create empty mask");
                  if (lyr_a[lidx]->layer_mask.def_color == 255)
                    mask_id = gimp_layer_create_mask (layer_id, GIMP_ADD_WHITE_MASK);
                  else
                    mask_id = gimp_layer_create_mask (layer_id, GIMP_ADD_BLACK_MASK);
                  gimp_layer_add_mask (layer_id, mask_id);
                  gimp_layer_set_apply_mask (layer_id,
                    ! lyr_a[lidx]->layer_mask.mask_flags.disabled);
                }
              else
                {
                  /* Load layer mask data */
                  if (lyr_a[lidx]->layer_mask.mask_flags.relative_pos)
                    {
                      lm_x = lyr_a[lidx]->layer_mask.left;
                      lm_y = lyr_a[lidx]->layer_mask.top;
                      lm_w = lyr_a[lidx]->layer_mask.right - lyr_a[lidx]->layer_mask.left;
                      lm_h = lyr_a[lidx]->layer_mask.bottom - lyr_a[lidx]->layer_mask.top;
                    }
                  else
                    {
                      lm_x = lyr_a[lidx]->layer_mask.left - l_x;
                      lm_y = lyr_a[lidx]->layer_mask.top - l_y;
                      lm_w = lyr_a[lidx]->layer_mask.right - lyr_a[lidx]->layer_mask.left;
                      lm_h = lyr_a[lidx]->layer_mask.bottom - lyr_a[lidx]->layer_mask.top;
                    }
                  IFDBG(3) g_debug ("Mask channel index %d", user_mask_chn);
                  IFDBG(3) g_debug ("Relative pos %d",
                                    lyr_a[lidx]->layer_mask.mask_flags.relative_pos);
                  layer_size = lm_w * lm_h;
                  pixels = g_malloc (layer_size);
                  IFDBG(3) g_debug ("Allocate Pixels %d", layer_size);
                  /* Crop mask at layer boundry */
                  IFDBG(3) g_debug ("Original Mask %d %d %d %d", lm_x, lm_y, lm_w, lm_h);
                  if (lm_x < 0
                      || lm_y < 0
                      || lm_w + lm_x > l_w
                      || lm_h + lm_y > l_h)
                    {
                      if (CONVERSION_WARNINGS)
                        g_message ("Warning\n"
                                   "The layer mask is partly outside the "
                                   "layer boundary. The mask will be "
                                   "cropped which may result in data loss.");
                      i = 0;
                      for (rowi = 0; rowi < lm_h; ++rowi)
                        {
                          if (rowi + lm_y >= 0 && rowi + lm_y < l_h)
                            {
                              for (coli = 0; coli < lm_w; ++coli)
                                {
                                  if (coli + lm_x >= 0 && coli + lm_x < l_w)
                                    {
                                      pixels[i] =
                                        lyr_chn[user_mask_chn]->data[(rowi * lm_w) + coli];
                                      i++;
                                    }
                                }
                            }
                        }
                      if (lm_x < 0)
                        {
                          lm_w += lm_x;
                          lm_x = 0;
                        }
                      if (lm_y < 0)
                        {
                          lm_h += lm_y;
                          lm_y = 0;
                        }
                      if (lm_w + lm_x > l_w)
                        lm_w = l_w - lm_x;
                      if (lm_h + lm_y > l_h)
                        lm_h = l_h - lm_y;
                    }
                  else
                    memcpy (pixels, lyr_chn[user_mask_chn]->data, layer_size);
                  g_free (lyr_chn[user_mask_chn]->data);
                  /* Draw layer mask data */
                  IFDBG(3) g_debug ("Layer %d %d %d %d", l_x, l_y, l_w, l_h);
                  IFDBG(3) g_debug ("Mask %d %d %d %d", lm_x, lm_y, lm_w, lm_h);

                  if (lyr_a[lidx]->layer_mask.def_color == 255)
                    mask_id = gimp_layer_create_mask (layer_id, GIMP_ADD_WHITE_MASK);
                  else
                    mask_id = gimp_layer_create_mask (layer_id, GIMP_ADD_BLACK_MASK);

                  IFDBG(3) g_debug ("New layer mask %d", mask_id);
                  gimp_layer_add_mask (layer_id, mask_id);
                  drawable = gimp_drawable_get (mask_id);
                  gimp_pixel_rgn_init (&pixel_rgn, drawable, 0 , 0,
                                       drawable->width, drawable->height, TRUE, FALSE);
                  gimp_pixel_rgn_set_rect (&pixel_rgn, pixels, lm_x, lm_y, lm_w, lm_h);
                  gimp_drawable_flush (drawable);
                  gimp_drawable_detach (drawable);
                  gimp_layer_set_apply_mask (layer_id,
                    ! lyr_a[lidx]->layer_mask.mask_flags.disabled);
                  g_free (pixels);
                }
            }
          for (cidx = 0; cidx < lyr_a[lidx]->num_channels; ++cidx)
            if (lyr_chn[cidx])
              g_free (lyr_chn[cidx]);
          g_free (lyr_chn);
        }
      g_free (lyr_a[lidx]);
    }
  g_free (lyr_a);

  return 0;
}

static gint
add_merged_image (const gint32  image_id,
                  PSDimage     *img_a,
                  FILE         *f,
                  GError      **error)
{
  PSDchannel            chn_a[MAX_CHANNELS];
  gchar                *alpha_name;
  guchar               *pixels;
  guint16               comp_mode;
  guint16               base_channels;
  guint16               extra_channels;
  guint16               total_channels;
  guint16              *rle_pack_len[MAX_CHANNELS];
  guint32               block_len;
  guint32               block_start;
  guint32               block_end;
  guint32               alpha_id;
  gint32                layer_size;
  gint32                layer_id = -1;
  gint32                channel_id = -1;
  gint32                active_layer;
  gint16                alpha_opacity;
  gint                 *lyr_lst;
  gint                  cidx;                  /* Channel index */
  gint                  rowi;                  /* Row index */
  gint                  lyr_count;
  gint                  offset;
  gint                  i;
  gboolean              alpha_visible;
  GimpDrawable         *drawable;
  GimpPixelRgn          pixel_rgn;
  GimpImageType         image_type;
  GimpRGB               alpha_rgb;

  total_channels = img_a->channels;
  extra_channels = 0;

  if ((img_a->color_mode == PSD_BITMAP ||
       img_a->color_mode == PSD_GRAYSCALE ||
       img_a->color_mode == PSD_DUOTONE ||
       img_a->color_mode == PSD_INDEXED) &&
       total_channels > 1)
    {
      extra_channels = total_channels - 1;
    }
  else if ((img_a->color_mode == PSD_RGB ||
            img_a->color_mode == PSD_LAB) &&
            total_channels > 3)
    {
      extra_channels = total_channels - 3;
    }
  else if ((img_a->color_mode == PSD_CMYK) &&
            total_channels > 4)
    {
      extra_channels = total_channels - 4;
    }
  if (img_a->transparency && extra_channels > 0)
    extra_channels--;
  base_channels = total_channels - extra_channels;

  /* ----- Read merged image & extra channel pixel data ----- */
  if (img_a->num_layers == 0
      || extra_channels > 0)
    {
      block_start = img_a->merged_image_start;
      block_len = img_a->merged_image_len;
      block_end = block_start + block_len;
      fseek (f, block_start, SEEK_SET);

      if (fread (&comp_mode, COMP_MODE_SIZE, 1, f) < 1)
        {
          psd_set_error (feof (f), errno, error);
          return -1;
        }
      comp_mode = GUINT16_FROM_BE (comp_mode);

      switch (comp_mode)
        {
          case PSD_COMP_RAW:        /* Planar raw data */
            IFDBG(3) g_debug ("Raw data length: %d", block_len);
            for (cidx = 0; cidx < total_channels; ++cidx)
              {
                chn_a[cidx].columns = img_a->columns;
                chn_a[cidx].rows = img_a->rows;
                if (read_channel_data (&chn_a[cidx], img_a->bps,
                    PSD_COMP_RAW, NULL, f, error) < 1)
                  return -1;
              }
            break;

          case PSD_COMP_RLE:        /* Packbits */
            /* Image data is stored as packed scanlines in planar order
               with all compressed length counters stored first */
            IFDBG(3) g_debug ("RLE length data: %d, RLE data block: %d",
                               total_channels * img_a->rows * 2,
                               block_len - (total_channels * img_a->rows * 2));
            for (cidx = 0; cidx < total_channels; ++cidx)
              {
                chn_a[cidx].columns = img_a->columns;
                chn_a[cidx].rows = img_a->rows;
                rle_pack_len[cidx] = g_malloc (img_a->rows * 2);
                for (rowi = 0; rowi < img_a->rows; ++rowi)
                  {
                    if (fread (&rle_pack_len[cidx][rowi], 2, 1, f) < 1)
                      {
                        psd_set_error (feof (f), errno, error);
                        return -1;
                      }
                    rle_pack_len[cidx][rowi] = GUINT16_FROM_BE (rle_pack_len[cidx][rowi]);
                  }
              }

            IFDBG(3) g_debug ("RLE decode - data");
            for (cidx = 0; cidx < total_channels; ++cidx)
              {
                if (read_channel_data (&chn_a[cidx], img_a->bps,
                    PSD_COMP_RLE, rle_pack_len[cidx], f, error) < 1)
                  return -1;
                g_free (rle_pack_len[cidx]);
              }
            break;

          case PSD_COMP_ZIP:                 /* ? */
          case PSD_COMP_ZIP_PRED:
            g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
                        _("Unsupported compression mode: %d"), comp_mode);
            return -1;
            break;
        }
    }

  /* ----- Draw merged image ----- */
  if (img_a->num_layers == 0)            /* Merged image - Photoshop 2 style */
    {
      image_type = get_gimp_image_type (img_a->base_type, img_a->transparency);

      layer_size = img_a->columns * img_a->rows;
      pixels = g_malloc (layer_size * base_channels);
      for (cidx = 0; cidx < base_channels; ++cidx)
        {
          for (i = 0; i < layer_size; ++i)
            {
              pixels[(i * base_channels) + cidx] = chn_a[cidx].data[i];
            }
          g_free (chn_a[cidx].data);
        }

      /* Add background layer */
      IFDBG(2) g_debug ("Draw merged image");
      layer_id = gimp_layer_new (image_id, _("Background"),
                                 img_a->columns, img_a->rows,
                                 image_type,
                                 100, GIMP_NORMAL_MODE);
      gimp_image_add_layer (image_id, layer_id, 0);
      drawable = gimp_drawable_get (layer_id);
      gimp_pixel_rgn_init (&pixel_rgn, drawable, 0, 0,
                           drawable->width, drawable->height, TRUE, FALSE);
      gimp_pixel_rgn_set_rect (&pixel_rgn, pixels,
                               0, 0, drawable->width, drawable->height);
      gimp_drawable_flush (drawable);
      gimp_drawable_detach (drawable);
      g_free (pixels);
    }
  else
    {
      /* Free merged image data for layered image */
      if (extra_channels)
        for (cidx = 0; cidx < base_channels; ++cidx)
          g_free (chn_a[cidx].data);
    }

  /* ----- Draw extra alpha channels ----- */
  if ((extra_channels                   /* Extra alpha channels */
      || img_a->transparency)           /* Transparency alpha channel */
      && image_id > -1)
    {
      IFDBG(2) g_debug ("Add extra channels");
      pixels = g_malloc(0);

      /* Get channel resource data */
      if (img_a->transparency)
        {
          offset = 1;

          /* Free "Transparency" channel name */
          if (img_a->alpha_names)
            {
              alpha_name = g_ptr_array_index (img_a->alpha_names, 0);
              if (alpha_name)
                g_free (alpha_name);
            }
        }
      else
        offset = 0;

      /* Draw channels */
      IFDBG(2) g_debug ("Number of channels: %d", extra_channels);
      for (i = 0; i < extra_channels; ++i)
        {
          /* Alpha channel name */
          alpha_name = NULL;
          alpha_visible = FALSE;
          /* Quick mask channel*/
          if (img_a->quick_mask_id)
            if (i == img_a->quick_mask_id - base_channels + offset)
              {
                /* Free "Quick Mask" channel name */
                alpha_name = g_ptr_array_index (img_a->alpha_names, i + offset);
                if (alpha_name)
                  g_free (alpha_name);
                alpha_name = g_strdup (GIMP_IMAGE_QUICK_MASK_NAME);
                alpha_visible = TRUE;
              }
          if (! alpha_name && img_a->alpha_names)
            if (offset < img_a->alpha_names->len
                && i + offset <= img_a->alpha_names->len)
              alpha_name = g_ptr_array_index (img_a->alpha_names, i + offset);
          if (! alpha_name)
            alpha_name = g_strdup (_("Extra"));

          if (offset < img_a->alpha_id_count &&
              offset + i <= img_a->alpha_id_count)
            alpha_id = img_a->alpha_id[i + offset];
          else
            alpha_id = 0;
          if (offset < img_a->alpha_display_count &&
              i + offset <= img_a->alpha_display_count)
            {
              alpha_rgb = img_a->alpha_display_info[i + offset]->gimp_color;
              alpha_opacity = img_a->alpha_display_info[i + offset]->opacity;
            }
          else
            {
              gimp_rgba_set (&alpha_rgb, 1.0, 0.0, 0.0, 1.0);
              alpha_opacity = 50;
            }

          cidx = base_channels + i;
          pixels = g_realloc (pixels, chn_a[cidx].columns * chn_a[cidx].rows);
          memcpy (pixels, chn_a[cidx].data, chn_a[cidx].columns * chn_a[cidx].rows);
          channel_id = gimp_channel_new (image_id, alpha_name,
                                         chn_a[cidx].columns, chn_a[cidx].rows,
                                         alpha_opacity, &alpha_rgb);
          gimp_image_add_channel (image_id, channel_id, 0);
          g_free (alpha_name);
          drawable = gimp_drawable_get (channel_id);
          if (alpha_id)
            gimp_item_set_tattoo (drawable->drawable_id, alpha_id);
          gimp_item_set_visible (drawable->drawable_id, alpha_visible);
          gimp_pixel_rgn_init (&pixel_rgn, drawable, 0, 0,
                                drawable->width, drawable->height,
                                TRUE, FALSE);
          gimp_pixel_rgn_set_rect (&pixel_rgn, pixels,
                                   0, 0, drawable->width,
                                   drawable->height);
          gimp_drawable_flush (drawable);
          gimp_drawable_detach (drawable);
          g_free (chn_a[cidx].data);
        }

      g_free (pixels);
      if (img_a->alpha_names)
        g_ptr_array_free (img_a->alpha_names, TRUE);

      if (img_a->alpha_id)
        g_free (img_a->alpha_id);

      if (img_a->alpha_display_info)
        {
          for (cidx = 0; cidx < img_a->alpha_display_count; ++cidx)
            g_free (img_a->alpha_display_info[cidx]);
          g_free (img_a->alpha_display_info);
        }
    }

  /* Set active layer */
  lyr_lst = gimp_image_get_layers (image_id, &lyr_count);
  if (img_a->layer_state + 1 > lyr_count ||
      img_a->layer_state + 1 < 0)
    img_a->layer_state = 0;
  active_layer = lyr_lst[lyr_count - img_a->layer_state - 1];
  gimp_image_set_active_layer (image_id, active_layer);
  g_free (lyr_lst);

  /* FIXME gimp image tattoo state */

  return 0;
}


/* Local utility functions */
static gchar *
get_psd_color_mode_name (PSDColorMode mode)
{
  static gchar * const psd_color_mode_names[] =
  {
    "BITMAP",
    "GRAYSCALE",
    "INDEXED",
    "RGB",
    "CMYK",
    "UNKNOWN (5)",
    "UNKNOWN (6)",
    "MULTICHANNEL",
    "DUOTONE",
    "LAB"
  };

  static gchar *err_name = NULL;

  if (mode >= PSD_BITMAP && mode <= PSD_LAB)
    return psd_color_mode_names[mode];

  g_free (err_name);
  err_name = g_strdup_printf ("UNKNOWN (%d)", mode);

  return err_name;
}

static void
psd_to_gimp_color_map (guchar *map256)
{
  guchar *tmpmap;
  gint    i;

  tmpmap = g_malloc (3 * 256);

  for (i = 0; i < 256; ++i)
    {
      tmpmap[i*3  ] = map256[i];
      tmpmap[i*3+1] = map256[i+256];
      tmpmap[i*3+2] = map256[i+512];
    }

  memcpy (map256, tmpmap, 3 * 256);
  g_free (tmpmap);
}

static GimpImageType
get_gimp_image_type (const GimpImageBaseType image_base_type,
                     const gboolean          alpha)
{
  GimpImageType image_type;

  switch (image_base_type)
    {
      case GIMP_GRAY:
        image_type = (alpha) ? GIMP_GRAYA_IMAGE : GIMP_GRAY_IMAGE;
        break;

      case GIMP_INDEXED:
        image_type = (alpha) ? GIMP_INDEXEDA_IMAGE : GIMP_INDEXED_IMAGE;
        break;

      case GIMP_RGB:
        image_type = (alpha) ? GIMP_RGBA_IMAGE : GIMP_RGB_IMAGE;
        break;

      default:
        image_type = -1;
        break;
    }

  return image_type;
}

static gint
read_channel_data (PSDchannel     *channel,
                   const guint16   bps,
                   const guint16   compression,
                   const guint16  *rle_pack_len,
                   FILE           *f,
                   GError        **error)
{
  gchar    *raw_data;
  gchar    *src;
  gchar    *dst;
  guint32   readline_len;
  gint      i;

  if (bps == 1)
    readline_len = ((channel->columns + 7) >> 3);
  else
    readline_len = (channel->columns * bps >> 3);

  IFDBG(3) g_debug ("raw data size %d x %d = %d", readline_len,
                    channel->rows, readline_len * channel->rows);

  /* sanity check, int overflow check (avoid divisions by zero) */
  if ((channel->rows == 0) || (channel->columns == 0) ||
      (channel->rows > G_MAXINT32 / channel->columns / MAX (bps >> 3, 1)))
    {
      g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
                   _("Unsupported or invalid channel size"));
      return -1;
    }

  raw_data = g_malloc (readline_len * channel->rows);
  switch (compression)
    {
      case PSD_COMP_RAW:
        if (fread (raw_data, readline_len, channel->rows, f) < 1)
          {
            psd_set_error (feof (f), errno, error);
            return -1;
          }
        break;

      case PSD_COMP_RLE:
        for (i = 0; i < channel->rows; ++i)
          {
            src = g_malloc (rle_pack_len[i]);
            dst = g_malloc (readline_len);
/*      FIXME check for over-run
            if (ftell (f) + rle_pack_len[i] > block_end)
              {
                psd_set_error (TRUE, errno, error);
                return -1;
              }
*/
            if (fread (src, rle_pack_len[i], 1, f) < 1)
              {
                psd_set_error (feof (f), errno, error);
                return -1;
              }
            /* FIXME check for errors returned from decode packbits */
            decode_packbits (src, dst, rle_pack_len[i], readline_len);
            g_free (src);
            memcpy (raw_data + i * readline_len, dst, readline_len);
            g_free (dst);
          }
        break;
    }

  /* Convert channel data to GIMP format */
  switch (bps)
    {
      case 16:
        channel->data = (gchar *) g_malloc (channel->rows * channel->columns);
        convert_16_bit (raw_data, channel->data, (channel->rows * channel->columns) << 1);
        break;

      case 8:
        channel->data = (gchar *) g_malloc (channel->rows * channel->columns);
        memcpy (channel->data, raw_data, (channel->rows * channel->columns));
        break;

      case 1:
        channel->data = (gchar *) g_malloc (channel->rows * channel->columns);
        convert_1_bit (raw_data, channel->data, channel->rows, channel->columns);
        break;
    }

  g_free (raw_data);

  return 1;
}

static void
convert_16_bit (const gchar *src,
                gchar       *dst,
                guint32     len)
{
/* Convert 16 bit to 8 bit dropping low byte
*/
  gint      i;

  IFDBG(3)  g_debug ("Start 16 bit conversion");

  for (i = 0; i < len >> 1; ++i)
    {
      *dst = *src;
      dst++;
      src += 2;
    }

  IFDBG(3)  g_debug ("End 16 bit conversion");
}

static void
convert_1_bit (const gchar *src,
               gchar       *dst,
               guint32      rows,
               guint32      columns)
{
/* Convert bits to bytes left to right by row.
   Rows are padded out to a byte boundry.
*/
  guint32 row_pos = 0;
  gint    i, j;

  IFDBG(3)  g_debug ("Start 1 bit conversion");

  for (i = 0; i < rows * ((columns + 7) >> 3); ++i)
    {
      guchar    mask = 0x80;
      for (j = 0; j < 8 && row_pos < columns; ++j)
        {
          *dst = (*src & mask) ? 0 : 1;
          IFDBG(3) g_debug ("byte %d, bit %d, offset %d, src %d, dst %d",
            i , j, row_pos, *src, *dst);
          dst++;
          mask >>= 1;
          row_pos++;
        }
      if (row_pos >= columns)
        row_pos = 0;
      src++;
    }
  IFDBG(3)  g_debug ("End 1 bit conversion");
}
