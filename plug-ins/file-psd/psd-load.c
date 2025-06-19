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
#include <zlib.h>
#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

#include "psd.h"
#include "psd-util.h"
#include "psd-image-res-load.h"
#include "psd-layer-res-load.h"
#include "psd-load.h"

#include "libgimp/stdplugins-intl.h"


#define COMP_MODE_SIZE sizeof(guint16)

typedef struct
{
  gint32  group_index; /* first layer from the top that has clipping */
  gint32  last_index;  /* last layer that will be part of the clipping group */
} ClippingInfo;


/*  Local function prototypes  */
static gint             read_header_block          (PSDimage       *img_a,
                                                    GInputStream   *input,
                                                    GError        **error);

static gint             read_color_mode_block      (PSDimage       *img_a,
                                                    GInputStream   *input,
                                                    GError        **error);

static gint             read_image_resource_block  (PSDimage       *img_a,
                                                    GInputStream   *input,
                                                    GError        **error);

static PSDlayer **      read_layer_block           (PSDimage       *img_a,
                                                    GInputStream   *input,
                                                    GError        **error);

static PSDlayer **      read_layer_info            (PSDimage       *img_a,
                                                    GInputStream   *input,
                                                    GError        **error);

static gint             read_merged_image_block    (PSDimage       *img_a,
                                                    GInputStream   *input,
                                                    GError        **error);

static GimpImage *      create_gimp_image          (PSDimage       *img_a,
                                                    GFile          *file);

static gint             add_color_map              (GimpImage      *image,
                                                    PSDimage       *img_a);

static gint             add_image_resources        (GimpImage      *image,
                                                    PSDimage       *img_a,
                                                    GInputStream   *input,
                                                    gboolean       *resolution_loaded,
                                                    gboolean       *profile_loaded,
                                                    GError        **error);

static gint             add_layers                 (GimpImage      *image,
                                                    PSDimage       *img_a,
                                                    PSDlayer      **lyr_a,
                                                    GInputStream   *input,
                                                    GError        **error);

static void             add_legacy_layer_effects   (GimpLayer      *layer,
                                                    PSDlayer       *lyr_a,
                                                    gboolean        ibm_pc_format);

static gint             add_merged_image           (GimpImage      *image,
                                                    PSDimage       *img_a,
                                                    GInputStream   *input,
                                                    GError        **error);

/*  Local utility function prototypes  */
static void       check_duplicate_clipping_group   (PSDlayer      **lyr_a,
                                                    gint16          num_layers,
                                                    ClippingInfo   *clipping_info);
static void             mark_clipping_groups       (PSDimage       *img_a,
                                                    PSDlayer      **lyr_a);
static GimpLayer      * add_clipping_group         (GimpImage      *image,
                                                    GimpLayer      *parent);

static gchar          * get_psd_color_mode_name    (PSDColorMode    mode);

static void             convert_legacy_psd_color   (GeglColor      *color,
                                                    guint16        *psd_color,
                                                    const Babl     *space,
                                                    gboolean        ibm_pc_format);

static void             psd_to_gimp_color_map      (guchar         *map256);

static GimpImageType    get_gimp_image_type        (GimpImageBaseType image_base_type,
                                                    gboolean          alpha);

static void             free_lyr_a                 (PSDlayer      **lyr_a,
                                                    gint            layer_count);

static void             free_lyr_chn               (PSDchannel    **lyr_chn,
                                                    gint            channel_count);

static gboolean         read_RLE_channel           (PSDimage       *img_a,
                                                    PSDchannel     *lyr_chn,
                                                    guint64         channel_data_len,
                                                    GInputStream   *input,
                                                    GError        **error);

static gint             read_channel_data          (PSDchannel     *channel,
                                                    guint16         bps,
                                                    guint16         compression,
                                                    const guint32  *rle_pack_len,
                                                    GInputStream   *input,
                                                    guint32         comp_len,
                                                    GError        **error);

static void             decode_32_bit_predictor    (gchar          *src,
                                                    gchar          *dst,
                                                    guint32         rows,
                                                    guint32         columns);

static void             convert_1_bit              (const gchar    *src,
                                                    gchar          *dst,
                                                    guint32         rows,
                                                    guint32         columns);

static const Babl*      get_layer_format           (PSDimage       *img_a,
                                                    gboolean        alpha);
static const Babl*      get_channel_format         (PSDimage       *img_a);
static const Babl*      get_mask_format            (PSDimage       *img_a);

static void             initialize_unsupported     (PSDSupport     *supported_features);

/* Main file load function */
GimpImage *
load_image (GFile        *file,
            gboolean      merged_image_only,
            gboolean     *resolution_loaded,
            gboolean     *profile_loaded,
            PSDSupport   *unsupported_features,
            GError      **load_error)
{
  GInputStream  *input;
  PSDimage       img_a;
  PSDlayer     **lyr_a;
  GimpImage     *image = NULL;
  GError        *error = NULL;

  img_a.ibm_pc_format  = FALSE;
  img_a.cmyk_transform = img_a.cmyk_transform_alpha = NULL;
  img_a.cmyk_profile = NULL;

  initialize_unsupported (unsupported_features);
  img_a.unsupported_features = unsupported_features;

  /* ----- Open PSD file ----- */

  input = G_INPUT_STREAM (g_file_read (file, NULL, &error));
  if (! input)
    {
      if (! error)
        g_set_error (load_error, G_FILE_ERROR, g_file_error_from_errno (errno),
                     _("Could not open '%s' for reading: %s"),
                     gimp_file_get_utf8_name (file), g_strerror (errno));
      else
        {
          g_set_error (load_error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
                       _("Could not open '%s' for reading: %s"),
                       gimp_file_get_utf8_name (file), error->message);
          g_error_free (error);
        }

      return NULL;
    }

  gimp_progress_init_printf (_("Opening '%s'"),
                             gimp_file_get_utf8_name (file));

  IFDBG(1) g_debug ("Open file %s", gimp_file_get_utf8_name (file));

  img_a.merged_image_only = merged_image_only;

  /* ----- Read the PSD file Header block ----- */
  IFDBG(2) g_debug ("Read header block at offset %" G_GOFFSET_FORMAT,
                    PSD_TELL(input));
  if (read_header_block (&img_a, input, &error) < 0)
    goto load_error;
  gimp_progress_update (0.1);

  /* ----- Read the PSD file Color Mode block ----- */
  IFDBG(2) g_debug ("Read color mode block at offset %" G_GOFFSET_FORMAT,
                    PSD_TELL(input));
  if (read_color_mode_block (&img_a, input, &error) < 0)
    goto load_error;
  gimp_progress_update (0.2);

  /* ----- Read the PSD file Image Resource block ----- */
  IFDBG(2) g_debug ("Read image resource block at offset %" G_GOFFSET_FORMAT,
                    PSD_TELL(input));
  if (read_image_resource_block (&img_a, input, &error) < 0)
    goto load_error;
  gimp_progress_update (0.3);

  /* ----- Read the PSD file Layer & Mask block ----- */
  IFDBG(2) g_debug ("Read layer & mask block at offset %" G_GOFFSET_FORMAT,
                    PSD_TELL(input));
  img_a.mask_layer_len = 0;
  lyr_a = read_layer_block (&img_a, input, &error);

  if (merged_image_only)
    {
      if (error)
        {
          g_debug ("Error loading layer block: %s", error->message);
          g_clear_error (&error);
        }
    }
  else if (error)
    {
      goto load_error;
    }
  else if (lyr_a == NULL)
    {
      if (img_a.num_layers > 0)
        {
          g_warning ("No error message but loading layer failed. This should not happen!");
          goto load_error;
        }
    }
  gimp_progress_update (0.4);

  /* ----- Read the PSD file Merged Image Data block ----- */
  IFDBG(2) g_debug ("Read merged image and extra alpha channel block at offset %" G_GOFFSET_FORMAT,
                    PSD_TELL(input));
  if (read_merged_image_block (&img_a, input, &error) < 0)
    goto load_error;
  gimp_progress_update (0.5);

  /* ----- Create GIMP image ----- */
  IFDBG(2) g_debug ("Create GIMP image");
  image = create_gimp_image (&img_a, file);
  if (! image)
    goto load_error;
  gimp_progress_update (0.6);

  /* ----- Add color map ----- */
  IFDBG(2) g_debug ("Add color map");
  if (add_color_map (image, &img_a) < 0)
    goto load_error;
  gimp_progress_update (0.7);

  /* ----- Add image resources ----- */
  IFDBG(2) g_debug ("Add image resources");
  if (add_image_resources (image, &img_a, input,
                           resolution_loaded, profile_loaded,
                           &error) < 0)
    goto load_error;
  gimp_progress_update (0.8);

  /* ----- Add layers -----*/
  IFDBG(2) g_debug ("Add layers");
  if (add_layers (image, &img_a, lyr_a, input, &error) < 0)
    goto load_error;
  gimp_progress_update (0.9);

  /* ----- Add merged image data and extra alpha channels ----- */
  IFDBG(2) g_debug ("Add merged image data and extra alpha channels");
  if (add_merged_image (image, &img_a, input, &error) < 0)
    goto load_error;
  gimp_progress_update (1.0);

  IFDBG(2) g_debug ("Close file & return, image id: %d", gimp_image_get_id (image));
  IFDBG(1) g_debug ("\n----------------------------------------"
                    "----------------------------------------\n");

  gimp_image_clean_all (image);
  gimp_image_undo_enable (image);
  g_object_unref (input);
  return image;

  /* ----- Process load errors ----- */
 load_error:
  if (error)
    {
      g_set_error (load_error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
                   _("Error loading PSD file: %s"), error->message);
      g_error_free (error);
    }

  /* Delete partially loaded image */
  if (image)
    gimp_image_delete (image);

  /* Close file if Open */
  g_object_unref (input);

  return NULL;
}

/* Loading metadata for external file formats */
GimpImage *
load_image_metadata (GFile        *file,
                     gint          data_length,
                     GimpImage    *image,
                     gboolean      for_layers,
                     gboolean      is_cmyk,
                     PSDSupport   *unsupported_features,
                     GError      **error)
{
  GInputStream   *input;
  PSDimage        img_a;
  gboolean        profile_loaded;
  gboolean        resolution_loaded;

  /* Convert metadata file to PSD format */
  input = G_INPUT_STREAM (g_file_read (file, NULL, error));
  if (! input)
    {
      g_object_unref (input);
      return image;
    }

  /* Load metadata for external file format */
  img_a.image_res_len       = data_length;
  img_a.image_res_start     = 0;
  img_a.version             = 1;
  img_a.layer_selection     = NULL;
  img_a.columns             = gimp_image_get_width (image);
  img_a.rows                = gimp_image_get_height (image);
  img_a.base_type           = gimp_image_get_base_type (image);
  img_a.merged_image_only   = FALSE;
  img_a.ibm_pc_format       = FALSE;
  img_a.alpha_names         = NULL;
  img_a.alpha_display_info  = NULL;
  img_a.alpha_display_count = 0;
  img_a.alpha_id            = NULL;
  img_a.alpha_id_count      = 0;
  img_a.quick_mask_id       = 0;
  img_a.cmyk_profile        = NULL;

  initialize_unsupported (unsupported_features);
  img_a.unsupported_features = unsupported_features;

  if (! for_layers)
    {
      PSDimageres res_a;

      while (PSD_TELL (input) < img_a.image_res_start + img_a.image_res_len)
        {
          if (get_image_resource_header (&res_a, input, error) < 0)
            break;

          if (res_a.data_start + res_a.data_len >
              img_a.image_res_start + img_a.image_res_len)
            {
              IFDBG(1) g_debug ("Unexpected end of image resource data");
              break;
            }

          if (load_image_resource (&res_a, image, &img_a, input,
                                   &resolution_loaded, &profile_loaded,
                                   error) < 0)
            break;
        }
    }
  else
    {
      PSDlayer  **lyr_a = NULL;
      gchar       sig[4];
      gchar       key[4];

      if (psd_read (input, &sig, 4, error) < 4)
        {
          g_object_unref (input);
          return image;
        }
      if (psd_read (input, &key, 4, error) < 4)
        {
          g_object_unref (input);
          return image;
        }
      if (data_length > 8)
        data_length -= 8;
      else
        data_length  = 0;

      /* Treat labels/ints as Little Endian */
      if (memcmp (sig, "MIB8", 4) == 0)
        img_a.ibm_pc_format = TRUE;

      /* Setting up PSDImage structure */
      if (memcmp (key, "Layr", 4) == 0 ||
          memcmp (key, "ryaL", 4) == 0)
        {
          img_a.bps = 8;
        }
      else if (memcmp (key, "Lr16", 4) == 0 ||
               memcmp (key, "61rL", 4) == 0)
        {
          img_a.bps = 16;
        }
      else if (memcmp (key, "Lr32", 4) == 0 ||
               memcmp (key, "23rL", 4) == 0)
        {
          img_a.bps = 32;
        }
      else
        {
          /* Get BPC from existing image */
          switch (gimp_image_get_precision (image))
            {
            case GIMP_PRECISION_U8_LINEAR:
            case GIMP_PRECISION_U8_NON_LINEAR:
            case GIMP_PRECISION_U8_PERCEPTUAL:
              img_a.bps = 8;
              break;

            case GIMP_PRECISION_U16_LINEAR:
            case GIMP_PRECISION_U16_NON_LINEAR:
            case GIMP_PRECISION_U16_PERCEPTUAL:
            case GIMP_PRECISION_HALF_LINEAR:
            case GIMP_PRECISION_HALF_NON_LINEAR:
            case GIMP_PRECISION_HALF_PERCEPTUAL:
              img_a.bps = 16;
              break;

            case GIMP_PRECISION_U32_LINEAR:
            case GIMP_PRECISION_U32_NON_LINEAR:
            case GIMP_PRECISION_U32_PERCEPTUAL:
            case GIMP_PRECISION_FLOAT_LINEAR:
            case GIMP_PRECISION_FLOAT_NON_LINEAR:
            case GIMP_PRECISION_FLOAT_PERCEPTUAL:
              img_a.bps = 32;
              break;

            default:
              g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
                           _("Invalid PSD metadata layer format"));
              return image;
            }
        }

      switch (gimp_image_get_base_type (image))
        {
          case GIMP_RGB:
            img_a.color_mode = PSD_RGB;
            break;

          case GIMP_GRAY:
            img_a.color_mode = PSD_GRAYSCALE;
            break;

          case GIMP_INDEXED:
            img_a.color_mode = PSD_INDEXED;
            break;
        }
      if (is_cmyk)
        img_a.color_mode = PSD_CMYK;

      /* Set layer block size from metadata */
      img_a.mask_layer_len = data_length;
      lyr_a = read_layer_block (&img_a, input, error);

      if (! lyr_a)
        {
          g_object_unref (input);
          return image;
        }

      add_layers (image, &img_a, lyr_a, input, error);
    }

  g_object_unref (input);

  return image;
}


/* Local functions */

static gint
read_header_block (PSDimage      *img_a,
                   GInputStream  *input,
                   GError       **error)
{
  gchar sig[4] = {0};
  gchar buf[6];

  if (psd_read (input, sig,                4, error) < 4 ||
      psd_read (input, &img_a->version,    2, error) < 2 ||
      psd_read (input, buf,                6, error) < 6 ||
      psd_read (input, &img_a->channels,   2, error) < 2 ||
      psd_read (input, &img_a->rows,       4, error) < 4 ||
      psd_read (input, &img_a->columns,    4, error) < 4 ||
      psd_read (input, &img_a->bps,        2, error) < 2 ||
      psd_read (input, &img_a->color_mode, 2, error) < 2)
    {
      psd_set_error (error);
      return -1;
    }
  img_a->version = GUINT16_FROM_BE (img_a->version);
  img_a->channels = GUINT16_FROM_BE (img_a->channels);
  img_a->rows = GUINT32_FROM_BE (img_a->rows);
  img_a->columns = GUINT32_FROM_BE (img_a->columns);
  img_a->bps = GUINT16_FROM_BE (img_a->bps);
  img_a->color_mode = GUINT16_FROM_BE (img_a->color_mode);

  IFDBG(1) g_debug ("\n\n\tSig: %.4s\n\tVer: %d\n\tChannels: "
                    "%d\n\tSize: %dx%d\n\tBPS: %d\n\tMode: %d\n",
                    sig, img_a->version, img_a->channels,
                    img_a->columns, img_a->rows,
                    img_a->bps, img_a->color_mode);

  if (memcmp (sig, "8BPS", 4) != 0)
    {
      g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
                  _("Not a valid Photoshop document file"));
      return -1;
    }

  if (img_a->version != 1 && img_a->version != 2)
    {
      g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
                  _("Unsupported file format version: %d"), img_a->version);
      return -1;
    }

  if (img_a->channels > MAX_CHANNELS)
    {
      g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
                  _("Too many channels in file: %d"), img_a->channels);
      return -1;
    }

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
      && img_a->color_mode != PSD_MULTICHANNEL
      && img_a->color_mode != PSD_CMYK
      && img_a->color_mode != PSD_LAB
      && img_a->color_mode != PSD_DUOTONE)
    {
      g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
                   _("Unsupported color mode: %s"),
                   get_psd_color_mode_name (img_a->color_mode));
      return -1;
    }

  if (img_a->color_mode == PSD_CMYK || img_a->color_mode == PSD_LAB)
    {
      if (img_a->bps != 8 && img_a->bps != 16)
        {
          g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
                       _("Unsupported color mode: %s"),
                       get_psd_color_mode_name (img_a->color_mode));
          return -1;
        }
    }

  /* Warning for unsupported bit depth */
  switch (img_a->bps)
    {
      case 32:
        IFDBG(3) g_debug ("32 Bit Data");
        break;

      case 16:
        IFDBG(3) g_debug ("16 Bit Data");
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
read_color_mode_block (PSDimage      *img_a,
                       GInputStream  *input,
                       GError       **error)
{
  static guchar cmap[]    = { 0, 0, 0, 255, 255, 255 };
  guint32       block_len = 0;

  img_a->color_map_entries = 0;
  img_a->color_map_len = 0;
  if (psd_read (input, &block_len, 4, error) < 4)
    {
      psd_set_error (error);
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
          if (psd_read (input, img_a->color_map, block_len, error) < block_len)
            {
              psd_set_error (error);
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
      if (psd_read (input, img_a->color_map, block_len, error) < block_len)
        {
          psd_set_error (error);
          return -1;
        }
    }
  else
    {
      /* Apparently it's possible to have a non zero block_len here. */
      if (! psd_seek (input, block_len, G_SEEK_CUR, error))
        {
          psd_set_error (error);
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
  IFDBG(2) g_debug ("Color map data length %d", img_a->color_map_len);

  return 0;
}

static gint
read_image_resource_block (PSDimage      *img_a,
                           GInputStream  *input,
                           GError       **error)
{
  guint64 block_len = 0;
  guint64 block_end;

  if (psd_read (input, &block_len, 4, error) < 4)
    {
      psd_set_error (error);
      return -1;
    }
  img_a->image_res_len = GUINT32_FROM_BE (block_len);

  IFDBG(1) g_debug ("Image resource block size = %d", (int)img_a->image_res_len);

  img_a->image_res_start = PSD_TELL(input);
  block_end = img_a->image_res_start + img_a->image_res_len;

  if (! psd_seek (input, block_end, G_SEEK_SET, error))
    {
      psd_set_error (error);
      return -1;
    }

  return 0;
}

static void
free_lyr_a (PSDlayer **lyr_a, gint layer_count)
{
  gint lidx;

  for (lidx = 0; lidx < layer_count; ++lidx)
    if (lyr_a[lidx])
      g_free (lyr_a[lidx]);
  g_free (lyr_a);
}

static PSDlayer **
read_layer_info (PSDimage      *img_a,
                 GInputStream  *input,
                 GError       **error)
{
  PSDlayer **lyr_a = NULL;
  guint64    block_len;
  guint64    block_rem;
  gint32     read_len;
  gint32     write_len;
  gint       lidx;                  /* Layer index */
  gint       cidx;                  /* Channel index */

  IFDBG(1) g_debug ("Reading layer info at offset %" G_GOFFSET_FORMAT,
                    PSD_TELL(input));
  /* Get number of layers */
  if (psd_read (input, &img_a->num_layers, 2, error) < 2)
    {
      psd_set_error (error);
      img_a->num_layers = -1;
      return NULL;
    }

  if (! img_a->ibm_pc_format)
    img_a->num_layers = GINT16_FROM_BE (img_a->num_layers);
  IFDBG(2) g_debug ("Number of layers: %d (negative means transparency present)", img_a->num_layers);

  if (img_a->num_layers < 0)
    {
      img_a->transparency = TRUE;
      img_a->num_layers = -img_a->num_layers;
    }

  if (! img_a->merged_image_only && img_a->num_layers)
    {
      /* Read layer records */
      PSDlayerres  res_a;

      /* Create pointer array for the layer records */
      lyr_a = g_new0 (PSDlayer *, img_a->num_layers);

      for (lidx = 0; lidx < img_a->num_layers; ++lidx)
        {
          IFDBG(3) g_debug ("Read info for layer %d at offset %" G_GOFFSET_FORMAT, lidx, PSD_TELL(input));

          /* Allocate layer record */
          lyr_a[lidx] = (PSDlayer *) g_malloc (sizeof (PSDlayer) );

          /* Initialise record */
          lyr_a[lidx]->id = 0;
          lyr_a[lidx]->group_type = 0;

          if (psd_read (input, &lyr_a[lidx]->top,          4, error) < 4 ||
              psd_read (input, &lyr_a[lidx]->left,         4, error) < 4 ||
              psd_read (input, &lyr_a[lidx]->bottom,       4, error) < 4 ||
              psd_read (input, &lyr_a[lidx]->right,        4, error) < 4 ||
              psd_read (input, &lyr_a[lidx]->num_channels, 2, error) < 2)
            {
              psd_set_error (error);
              free_lyr_a (lyr_a, img_a->num_layers);
              return NULL;
            }

          if (! img_a->ibm_pc_format)
            {
              lyr_a[lidx]->top = GINT32_FROM_BE (lyr_a[lidx]->top);
              lyr_a[lidx]->left = GINT32_FROM_BE (lyr_a[lidx]->left);
              lyr_a[lidx]->bottom = GINT32_FROM_BE (lyr_a[lidx]->bottom);
              lyr_a[lidx]->right = GINT32_FROM_BE (lyr_a[lidx]->right);
              lyr_a[lidx]->num_channels =
                GUINT16_FROM_BE (lyr_a[lidx]->num_channels);
            }

          if (lyr_a[lidx]->num_channels > MAX_CHANNELS)
            {
              g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
                           _("Too many channels in layer: %d"),
                           lyr_a[lidx]->num_channels);
              free_lyr_a (lyr_a, img_a->num_layers);
              return NULL;
            }
          IFDBG(2) g_debug ("Layer %d, Coords %d %d %d %d, channels %d, ",
                            lidx, lyr_a[lidx]->left, lyr_a[lidx]->top,
                            lyr_a[lidx]->right, lyr_a[lidx]->bottom,
                            lyr_a[lidx]->num_channels);

          lyr_a[lidx]->chn_info = g_new (ChannelLengthInfo, lyr_a[lidx]->num_channels);

          for (cidx = 0; cidx < lyr_a[lidx]->num_channels; ++cidx)
            {
              if (psd_read (input, &lyr_a[lidx]->chn_info[cidx].channel_id, 2, error) < 2 ||
                  ! psd_read_len (input, &lyr_a[lidx]->chn_info[cidx].data_len,
                                  img_a->version, error))
                {
                  psd_set_error (error);
                  free_lyr_a (lyr_a, img_a->num_layers);
                  return NULL;
                }

              if (! img_a->ibm_pc_format)
                lyr_a[lidx]->chn_info[cidx].channel_id =
                  GINT16_FROM_BE (lyr_a[lidx]->chn_info[cidx].channel_id);
              else
                lyr_a[lidx]->chn_info[cidx].data_len =
                  GUINT32_FROM_LE (lyr_a[lidx]->chn_info[cidx].data_len);

              img_a->layer_data_len += lyr_a[lidx]->chn_info[cidx].data_len;
              IFDBG(3) g_debug ("Channel ID %d, data len %" G_GSIZE_FORMAT,
                                lyr_a[lidx]->chn_info[cidx].channel_id,
                                lyr_a[lidx]->chn_info[cidx].data_len);
            }

          if (psd_read (input, lyr_a[lidx]->mode_key,   4, error) < 4 ||
              psd_read (input, lyr_a[lidx]->blend_mode, 4, error) < 4 ||
              psd_read (input, &lyr_a[lidx]->opacity,   1, error) < 1 ||
              psd_read (input, &lyr_a[lidx]->clipping,  1, error) < 1 ||
              psd_read (input, &lyr_a[lidx]->flags,     1, error) < 1 ||
              psd_read (input, &lyr_a[lidx]->filler,    1, error) < 1 ||
              psd_read (input, &lyr_a[lidx]->extra_len, 4, error) < 4)
            {
              psd_set_error (error);
              free_lyr_a (lyr_a, img_a->num_layers);
              return NULL;
            }

          /* Reverse relevant IBM PC Format entries */
          if (img_a->ibm_pc_format)
            {
              gchar blend_mode[4];

              for (gint i = 0; i < 4; i++)
                blend_mode[i] = lyr_a[lidx]->blend_mode[3 - i];
              for (gint i = 0; i < 4; i++)
                lyr_a[lidx]->blend_mode[i] = blend_mode[i];

              lyr_a[lidx]->extra_len =
                GUINT32_FROM_LE (lyr_a[lidx]->extra_len);
            }

          /* Not sure if 8B64 is possible here but it won't hurt to check. */
          if (memcmp (lyr_a[lidx]->mode_key, "8BIM", 4) != 0 &&
              memcmp (lyr_a[lidx]->mode_key, "MIB8", 4) != 0 &&
              memcmp (lyr_a[lidx]->mode_key, "8B64", 4) != 0)
            {
              IFDBG(1) g_debug ("Incorrect layer mode signature %.4s",
                                lyr_a[lidx]->mode_key);
              g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
                           _("The file is corrupt!"));
              free_lyr_a (lyr_a, img_a->num_layers);
              return NULL;
            }

          lyr_a[lidx]->layer_flags.trans_prot = lyr_a[lidx]->flags & 1 ? TRUE : FALSE;
          lyr_a[lidx]->layer_flags.visible = lyr_a[lidx]->flags & 2 ? FALSE : TRUE;

          if (lyr_a[lidx]->flags & 8)
            lyr_a[lidx]->layer_flags.irrelevant = lyr_a[lidx]->flags & 16 ? TRUE : FALSE;
          else
            lyr_a[lidx]->layer_flags.irrelevant = FALSE;

          if (! img_a->ibm_pc_format)
            lyr_a[lidx]->extra_len = GUINT32_FROM_BE (lyr_a[lidx]->extra_len);
          block_rem = lyr_a[lidx]->extra_len;
          IFDBG(2) g_debug ("\n\tLayer mode sig: %.4s\n\tBlend mode: %.4s\n\t"
                            "Opacity: %d\n\tClipping: %d\n\tExtra data len: %" G_GSIZE_FORMAT "\n\t"
                            "Alpha lock: %d\n\tVisible: %d\n\tIrrelevant: %d",
                            lyr_a[lidx]->mode_key,
                            lyr_a[lidx]->blend_mode,
                            lyr_a[lidx]->opacity,
                            lyr_a[lidx]->clipping,
                            lyr_a[lidx]->extra_len,
                            lyr_a[lidx]->layer_flags.trans_prot,
                            lyr_a[lidx]->layer_flags.visible,
                            lyr_a[lidx]->layer_flags.irrelevant);
          IFDBG(3) g_debug ("Offset: %" G_GOFFSET_FORMAT ", Remaining length %" G_GSIZE_FORMAT,
                            PSD_TELL(input), block_rem);

          if (! lyr_a[lidx]->layer_flags.irrelevant)
            {
              if (lyr_a[lidx]->bottom < lyr_a[lidx]->top ||
                  lyr_a[lidx]->bottom - lyr_a[lidx]->top > GIMP_MAX_IMAGE_SIZE)
                {
                  g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
                              _("Unsupported or invalid layer height: %d"),
                              lyr_a[lidx]->bottom - lyr_a[lidx]->top);
                  free_lyr_a (lyr_a, img_a->num_layers);
                  return NULL;
                }
              if (lyr_a[lidx]->right < lyr_a[lidx]->left ||
                  lyr_a[lidx]->right - lyr_a[lidx]->left > GIMP_MAX_IMAGE_SIZE)
                {
                  g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
                              _("Unsupported or invalid layer width: %d"),
                              lyr_a[lidx]->right - lyr_a[lidx]->left);
                  free_lyr_a (lyr_a, img_a->num_layers);
                  return NULL;
                }

              if ((lyr_a[lidx]->right - lyr_a[lidx]->left) >
                  G_MAXINT32 / MAX (lyr_a[lidx]->bottom - lyr_a[lidx]->top, 1))
                {
                  g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
                              _("Unsupported or invalid layer size: %dx%d"),
                              lyr_a[lidx]->right - lyr_a[lidx]->left,
                              lyr_a[lidx]->bottom - lyr_a[lidx]->top);
                  free_lyr_a (lyr_a, img_a->num_layers);
                  return NULL;
                }
            }

          /* Layer mask data */
          if (psd_read (input, &block_len, 4, error) < 4)
            {
              psd_set_error (error);
              free_lyr_a (lyr_a, img_a->num_layers);
              return NULL;
            }

          if (! img_a->ibm_pc_format)
            block_len = GUINT32_FROM_BE (block_len);
          else
            block_len = GUINT32_FROM_LE (block_len);

          IFDBG(3) g_debug ("Layer mask record size %" G_GOFFSET_FORMAT, block_len);
          if (block_len + 4 > block_rem)
            {
              g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
                           _("Unsupported or invalid mask info size."));
              /* Translations have problems with using G_GSIZE_FORMAT, let's use g_debug. */
              g_debug ("Unsupported or invalid mask info size: %" G_GSIZE_FORMAT, block_len);
              free_lyr_a (lyr_a, img_a->num_layers);
              return NULL;
            }

          block_rem -= (block_len + 4);
          IFDBG(3) g_debug ("Offset: %" G_GOFFSET_FORMAT ", Remaining length %" G_GSIZE_FORMAT,
                            PSD_TELL(input), block_rem);

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
          lyr_a[lidx]->layer_mask.flags = 0;
          lyr_a[lidx]->layer_mask.extra_flags = 0;
          lyr_a[lidx]->layer_mask.mask_params = 0;
          lyr_a[lidx]->layer_mask.mask_flags.relative_pos = FALSE;
          lyr_a[lidx]->layer_mask.mask_flags.disabled = FALSE;
          lyr_a[lidx]->layer_mask.mask_flags.invert = FALSE;
          lyr_a[lidx]->layer_mask.mask_flags.rendered = FALSE;
          lyr_a[lidx]->layer_mask.mask_flags.params_present = FALSE;

          if (block_len > 0)
            {
              if (psd_read (input, &lyr_a[lidx]->layer_mask.top,       4, error) < 4 ||
                  psd_read (input, &lyr_a[lidx]->layer_mask.left,      4, error) < 4 ||
                  psd_read (input, &lyr_a[lidx]->layer_mask.bottom,    4, error) < 4 ||
                  psd_read (input, &lyr_a[lidx]->layer_mask.right,     4, error) < 4 ||
                  psd_read (input, &lyr_a[lidx]->layer_mask.def_color, 1, error) < 1 ||
                  psd_read (input, &lyr_a[lidx]->layer_mask.flags,     1, error) < 1)
                {
                  psd_set_error (error);
                  free_lyr_a (lyr_a, img_a->num_layers);
                  return NULL;
                }

              if (! img_a->ibm_pc_format)
                {
                  lyr_a[lidx]->layer_mask.top =
                    GINT32_FROM_BE (lyr_a[lidx]->layer_mask.top);
                  lyr_a[lidx]->layer_mask.left =
                    GINT32_FROM_BE (lyr_a[lidx]->layer_mask.left);
                  lyr_a[lidx]->layer_mask.bottom =
                    GINT32_FROM_BE (lyr_a[lidx]->layer_mask.bottom);
                  lyr_a[lidx]->layer_mask.right =
                    GINT32_FROM_BE (lyr_a[lidx]->layer_mask.right);
                }
              lyr_a[lidx]->layer_mask.mask_flags.relative_pos =
                lyr_a[lidx]->layer_mask.flags & 1 ? TRUE : FALSE;
              lyr_a[lidx]->layer_mask.mask_flags.disabled =
                lyr_a[lidx]->layer_mask.flags & 2 ? TRUE : FALSE;
              lyr_a[lidx]->layer_mask.mask_flags.invert =
                lyr_a[lidx]->layer_mask.flags & 4 ? TRUE : FALSE;
              lyr_a[lidx]->layer_mask.mask_flags.rendered =
                lyr_a[lidx]->layer_mask.flags & 8 ? TRUE : FALSE;
              lyr_a[lidx]->layer_mask.mask_flags.params_present =
                lyr_a[lidx]->layer_mask.flags & 16 ? TRUE : FALSE;
              IFDBG(3)
                {
                  if (lyr_a[lidx]->layer_mask.flags &  32) g_debug ("Layer mask flags bit 5 set.");
                  if (lyr_a[lidx]->layer_mask.flags &  64) g_debug ("Layer mask flags bit 6 set.");
                  if (lyr_a[lidx]->layer_mask.flags & 128) g_debug ("Layer mask flags bit 7 set.");
                }

              block_len -= 18;

              /* According to psd-tools this part comes before reading the
               * mask parameters. However if all mask parameter flags are
               * set the size of that would also be more than 18. I'm not
               * sure if all those flags could be set at the same time or
               * how to distinguish them. */
              if (block_len >= 18)
                {
                  if (psd_read (input, &lyr_a[lidx]->layer_mask.extra_flags,     1, error) < 1 ||
                      psd_read (input, &lyr_a[lidx]->layer_mask.extra_def_color, 1, error) < 1 ||
                      psd_read (input, &lyr_a[lidx]->layer_mask_extra.top,       4, error) < 4 ||
                      psd_read (input, &lyr_a[lidx]->layer_mask_extra.left,      4, error) < 4 ||
                      psd_read (input, &lyr_a[lidx]->layer_mask_extra.bottom,    4, error) < 4 ||
                      psd_read (input, &lyr_a[lidx]->layer_mask_extra.right,     4, error) < 4)
                    {
                      psd_set_error (error);
                      free_lyr_a (lyr_a, img_a->num_layers);
                      return NULL;
                    }
                  block_len -= 18;

                  if (! img_a->ibm_pc_format)
                    {
                      lyr_a[lidx]->layer_mask_extra.top =
                        GINT32_FROM_BE (lyr_a[lidx]->layer_mask_extra.top);
                      lyr_a[lidx]->layer_mask_extra.left =
                        GINT32_FROM_BE (lyr_a[lidx]->layer_mask_extra.left);
                      lyr_a[lidx]->layer_mask_extra.bottom =
                        GINT32_FROM_BE (lyr_a[lidx]->layer_mask_extra.bottom);
                      lyr_a[lidx]->layer_mask_extra.right =
                        GINT32_FROM_BE (lyr_a[lidx]->layer_mask_extra.right);
                    }

                  IFDBG(2) g_debug ("Rect enclosing Layer mask %d %d %d %d",
                                    lyr_a[lidx]->layer_mask_extra.left,
                                    lyr_a[lidx]->layer_mask_extra.top,
                                    lyr_a[lidx]->layer_mask_extra.right,
                                    lyr_a[lidx]->layer_mask_extra.bottom);
                }

              if (block_len > 2 && lyr_a[lidx]->layer_mask.mask_flags.params_present)
                {
                  gint extra_bytes = 0;

                  if (psd_read (input, &lyr_a[lidx]->layer_mask.mask_params, 1, error) < 1)
                    {
                      psd_set_error (error);
                      free_lyr_a (lyr_a, img_a->num_layers);
                      return NULL;
                    }
                  block_len--;
                  IFDBG(3) g_debug ("Mask params: %u", lyr_a[lidx]->layer_mask.mask_params);

                  /* FIXME Extra bytes with user/vector mask density and feather follow.
                   * We currently can't handle that so we will skip it. */
                  extra_bytes += (lyr_a[lidx]->layer_mask.mask_params & 1 ? 1 : 0);
                  extra_bytes += (lyr_a[lidx]->layer_mask.mask_params & 2 ? 8 : 0);
                  extra_bytes += (lyr_a[lidx]->layer_mask.mask_params & 4 ? 1 : 0);
                  extra_bytes += (lyr_a[lidx]->layer_mask.mask_params & 8 ? 8 : 0);
                  IFDBG(3) g_debug ("Extra bytes according to mask parameters %d", extra_bytes);
                  if (! psd_seek (input, extra_bytes, G_SEEK_CUR, error))
                    {
                      psd_set_error (error);
                      free_lyr_a (lyr_a, img_a->num_layers);
                      return NULL;
                    }
                  block_len -= extra_bytes;
                }

              if (block_len > 0)
                {
                  /* We have some remaining unknown mask data.
                   * If size is less than 4 it is most likely padding. */
                  IFDBG(1)
                    {
                      if (block_len > 3)
                        g_debug ("Skipping %" G_GOFFSET_FORMAT " bytes of unknown layer mask data", block_len);
                    }

                  if (! psd_seek (input, block_len, G_SEEK_CUR, error))
                    {
                      psd_set_error (error);
                      free_lyr_a (lyr_a, img_a->num_layers);
                      return NULL;
                    }
                }

              IFDBG(2) g_debug ("Layer mask coords %d %d %d %d",
                                lyr_a[lidx]->layer_mask.left,
                                lyr_a[lidx]->layer_mask.top,
                                lyr_a[lidx]->layer_mask.right,
                                lyr_a[lidx]->layer_mask.bottom);

              IFDBG(3) g_debug ("Default mask color %d, real color %d",
                                lyr_a[lidx]->layer_mask.def_color,
                                lyr_a[lidx]->layer_mask.extra_def_color);

              IFDBG(3) g_debug ("Mask flags %u, real flags %u, mask params %u",
                                lyr_a[lidx]->layer_mask.flags,
                                lyr_a[lidx]->layer_mask.extra_flags,
                                lyr_a[lidx]->layer_mask.mask_params);

              /* Rendered masks can have invalid dimensions: 0, 0, 0, -1 */
              if (! lyr_a[lidx]->layer_mask.mask_flags.rendered)
                {
                  /* sanity checks */
                  if (lyr_a[lidx]->layer_mask.bottom < lyr_a[lidx]->layer_mask.top ||
                      lyr_a[lidx]->layer_mask.bottom - lyr_a[lidx]->layer_mask.top > GIMP_MAX_IMAGE_SIZE)
                    {
                      g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
                                   _("Unsupported or invalid layer mask height: %d"),
                                   lyr_a[lidx]->layer_mask.bottom - lyr_a[lidx]->layer_mask.top);
                      free_lyr_a (lyr_a, img_a->num_layers);
                      return NULL;
                    }
                  if (lyr_a[lidx]->layer_mask.right < lyr_a[lidx]->layer_mask.left ||
                      lyr_a[lidx]->layer_mask.right - lyr_a[lidx]->layer_mask.left > GIMP_MAX_IMAGE_SIZE)
                    {
                      g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
                                   _("Unsupported or invalid layer mask width: %d"),
                                   lyr_a[lidx]->layer_mask.right - lyr_a[lidx]->layer_mask.left);
                      free_lyr_a (lyr_a, img_a->num_layers);
                      return NULL;
                    }

                  if ((lyr_a[lidx]->layer_mask.right - lyr_a[lidx]->layer_mask.left) >
                      G_MAXINT32 / MAX (lyr_a[lidx]->layer_mask.bottom - lyr_a[lidx]->layer_mask.top, 1))
                    {
                      g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
                                   _("Unsupported or invalid layer mask size: %dx%d"),
                                   lyr_a[lidx]->layer_mask.right - lyr_a[lidx]->layer_mask.left,
                                   lyr_a[lidx]->layer_mask.bottom - lyr_a[lidx]->layer_mask.top);
                      free_lyr_a (lyr_a, img_a->num_layers);
                      return NULL;
                    }
                }
            }

          /* Layer blending ranges */           /* FIXME  */
          if (psd_read (input, &block_len, 4, error) < 4)
            {
              psd_set_error (error);
              free_lyr_a (lyr_a, img_a->num_layers);
              return NULL;
            }

          if (! img_a->ibm_pc_format)
            block_len = GUINT32_FROM_BE (block_len);
          block_rem -= (block_len + 4);
          IFDBG(3) g_debug ("Blending ranges size %" G_GSIZE_FORMAT
                            " (not imported)", block_len);
          IFDBG(3) g_debug ("Offset: %" G_GOFFSET_FORMAT ", Remaining length %" G_GSIZE_FORMAT,
                            PSD_TELL(input), block_rem);

          if (block_len > 0)
            {
              if (! psd_seek (input, block_len, G_SEEK_CUR, error))
                {
                  psd_set_error (error);
                  free_lyr_a (lyr_a, img_a->num_layers);
                  return NULL;
                }
            }

          lyr_a[lidx]->name = fread_pascal_string (&read_len, &write_len,
                                                   4, input, error);
          if (*error)
            {
              free_lyr_a (lyr_a, img_a->num_layers);
              return NULL;
            }

          block_rem -= read_len;
          IFDBG(3) g_debug ("Offset: %" G_GOFFSET_FORMAT ", Remaining length %" G_GSIZE_FORMAT,
                            PSD_TELL(input), block_rem);

          /* Adjustment layer info */           /* FIXME */
          lyr_a[lidx]->layer_styles = g_new (PSDLayerStyles, 1);
          lyr_a[lidx]->layer_styles->count = 0;

          while (block_rem > 7)
            {
              gint header_size;
              IFDBG(3) g_debug ("Offset: %" G_GOFFSET_FORMAT ", Remaining length %" G_GSIZE_FORMAT,
                                PSD_TELL(input), block_rem);
              header_size = get_layer_resource_header (&res_a, img_a->version, input, error);
              if (header_size < 0)
                {
                  psd_set_error (error);
                  free_lyr_a (lyr_a, img_a->num_layers);
                  return NULL;
                }

              block_rem -= header_size;

              if (res_a.data_len % 2 != 0)
                {
                  /*  Warn the user about an invalid length value but
                   *  try to recover graciously. See bug #771558.
                   */
                  g_printerr ("psd-load: Layer extra data length should "
                              "be even, but it is %"G_GOFFSET_FORMAT".", res_a.data_len);
                }

              if (res_a.data_len > block_rem)
                {
                  IFDBG(1) g_debug ("Unexpected end of layer resource data");
                  g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
                               _("The file is corrupt!"));
                  free_lyr_a (lyr_a, img_a->num_layers);
                  return NULL;
                }

              lyr_a[lidx]->unsupported_features = img_a->unsupported_features;

              if (load_layer_resource (&res_a, lyr_a[lidx], input, error) < 0)
                {
                  free_lyr_a (lyr_a, img_a->num_layers);
                  return NULL;
                }
              block_rem -= res_a.data_len;
              IFDBG(3) g_debug ("Remaining length in block: %" G_GSIZE_FORMAT, block_rem);
            }
          if (block_rem > 0)
            {
              if (! psd_seek (input, block_rem, G_SEEK_CUR, error))
                {
                  psd_set_error (error);
                  free_lyr_a (lyr_a, img_a->num_layers);
                  return NULL;
                }
            }
        }

      img_a->layer_data_start = PSD_TELL(input);
      if (! psd_seek (input, img_a->layer_data_len, G_SEEK_CUR, error))
        {
          psd_set_error (error);
          free_lyr_a (lyr_a, img_a->num_layers);
          return NULL;
        }

      IFDBG(1) g_debug ("Layer image data block start %" G_GOFFSET_FORMAT ", size %" G_GSIZE_FORMAT
                        ", end: %" G_GOFFSET_FORMAT,
                        img_a->layer_data_start, img_a->layer_data_len, PSD_TELL(input));
    }

  return lyr_a;
}

static PSDlayer **
read_layer_block (PSDimage      *img_a,
                  GInputStream  *input,
                  GError       **error)
{
  PSDlayer **lyr_a = NULL;
  guint64    block_len;
  guint64    block_end;
  gint       block_len_size = (img_a->version == 1 ? 4 : 8);
  gboolean   enclosed_data = FALSE;

  /* If layer data is being loaded for non-PSD files (like TIFF),
   * then mask_layer_len will have its size preloaded */
  if (img_a->mask_layer_len == 0)
    {
      if (! psd_read_len (input, &block_len, img_a->version, error))
        {
          img_a->num_layers = -1;
          return NULL;
        }
      else
        {
          img_a->mask_layer_len = block_len;
        }
    }
  else
    {
      enclosed_data = TRUE;
    }

  IFDBG(1) g_debug ("Layer and mask block size = %" G_GOFFSET_FORMAT, img_a->mask_layer_len);

  img_a->transparency = FALSE;
  img_a->layer_data_len = 0;
  img_a->num_layers = 0;

  if (!img_a->mask_layer_len)
    {
      img_a->num_layers = 0;
      return NULL;
    }
  else
    {
      gsize total_len = img_a->mask_layer_len;

      img_a->mask_layer_start = PSD_TELL(input);
      block_end = img_a->mask_layer_start + img_a->mask_layer_len;

      /* Layer info */
      if (! psd_read_len (input, &block_len, img_a->version, error))
        {
          lyr_a = NULL;
          return NULL;
        }
      else
        {
          guint64 adjusted_block_len;

          if (img_a->ibm_pc_format)
            {
              block_len = GUINT64_FROM_BE (block_len);
              IFDBG(1) g_debug ("IBM PC format data detected");
            }

          IFDBG(1) g_debug ("Layer info size = %" G_GOFFSET_FORMAT, block_len);
          /* To make computations easier add the size of block_len */
          block_len += block_len_size;

          /* Actual block size is a multiple of 2 */
          adjusted_block_len = (block_len + 1) / 2 * 2;

          if (block_len > block_len_size)
            {
              goffset cur_ofs;

              lyr_a = read_layer_info (img_a, input, error);

              if (error && *error)
                return NULL;

              cur_ofs = PSD_TELL(input);
              if (img_a->mask_layer_start + block_len != cur_ofs && ! img_a->merged_image_only)
                {
                  g_debug ("Unexpected offset after reading layer info: %" G_GOFFSET_FORMAT
                          " instead of %" G_GOFFSET_FORMAT,
                          cur_ofs, img_a->mask_layer_start + block_len);
                }
              if (img_a->mask_layer_start + adjusted_block_len != cur_ofs)
                {
                  /* Correct the offset. */
                  if (! psd_seek (input, img_a->mask_layer_start + adjusted_block_len,
                                  G_SEEK_SET, error))
                    return NULL;
                }
            }
          if (block_len > total_len)
            {
              IFDBG(1) g_debug ("Unexpectedly block_len %" G_GSIZE_FORMAT
                                " is larger than total_len %" G_GSIZE_FORMAT,
                                block_len, total_len);
              block_len = total_len;
            }
          total_len -= block_len;
        }
      IFDBG(3) g_debug ("Offset: %" G_GOFFSET_FORMAT ", remaining len: %" G_GSIZE_FORMAT,
                        PSD_TELL(input), total_len);

      if (total_len >= 4 && ! enclosed_data)
        {
          /* Global layer mask info */
          if (psd_read (input, &block_len, 4, error) == 4)
            {
              if (! img_a->ibm_pc_format)
                block_len = GUINT32_FROM_BE (block_len);
              else
                block_len = GUINT32_TO_LE (block_len);

              IFDBG(1) g_debug ("Global layer mask info size = %" G_GOFFSET_FORMAT, block_len);

              if (block_len > total_len)
                {
                  g_debug ("Invalid global layer mask size. Skipping remainder of layer data.");
                  total_len = 0;
                }
              else
                {
                  /* read_global_layer_mask_info (img_a, f, error); */
                  /* For now skipping ... */
                  if (block_len > 0)
                    if (! psd_seek (input, block_len, G_SEEK_CUR, error))
                      {
                        psd_set_error (error);
                        return NULL;
                      }

                  total_len -= block_len + 4;
                }
            }
          else
            {
              psd_set_error (error);
              return NULL;
            }
        }

      /* Additional Layer Information */
      g_debug ("Reading layer resources...");
      while (total_len >= 12)
        {
          gsize        adjusted_len;
          gint         header_size;
          PSDlayerres  res_a;

          IFDBG(3) g_debug ("Offset: %" G_GOFFSET_FORMAT ", remaining len: %" G_GSIZE_FORMAT,
                            PSD_TELL(input), total_len);

          header_size = get_layer_resource_header (&res_a, img_a->version, input, error);
          if (header_size < 0)
            {
              psd_set_error (error);
              return NULL;
            }

          total_len -= header_size;

          if (res_a.data_len > total_len)
            {
              /* Not fatal just skip remainder of this block. */
              g_debug ("Invalid resource block length: %" G_GSIZE_FORMAT
                       " is larger than maximum %" G_GSIZE_FORMAT,
                       res_a.data_len, total_len);
              break;
            }

          if (res_a.data_len > 0)
            {
            if (memcmp (res_a.key, "Lr16", 4) == 0 ||
                memcmp (res_a.key, "Lr32", 4) == 0)
              {
                lyr_a = read_layer_info (img_a, input, error);
                if (lyr_a == NULL)
                  {
                    img_a->num_layers = 0;
                  }
                if (error && *error)
                  return NULL;
              }
            else
              {
                /* FIXME Loading layer resources here to a specific layer is probably
                 * not right. We need to figure out what to do instead.
                 * Most likely these resources are not connected to a layer so
                 * lyr_a being NULL should not matter. e.g. PATT (pattern)
                 * e.g. 0layers.psb has resources when lyr_a == NULL
                 */
                if (load_layer_resource (&res_a, NULL, input, error) < 0)
                  {
                    return NULL;
                  }
              }
            }

          /* Seems the length here needs to be a multiple of 4? */
          adjusted_len = (res_a.data_len + 3) / 4 * 4;
          total_len -= adjusted_len;

          /* Seek to correct offset */
          if (! psd_seek (input, res_a.data_start + adjusted_len, G_SEEK_SET, error))
            {
              g_debug ("Failed to seek to start of next layer resource at offset %" G_GOFFSET_FORMAT,
                       res_a.data_start + adjusted_len);
              psd_set_error (error);
              return NULL;
            }
        }

      /* Skip to end of block */
      if (! psd_seek (input, block_end, G_SEEK_SET, error))
        {
          psd_set_error (error);
          return NULL;
        }
      IFDBG(3) g_debug ("Finished read_layer_block. Now at offset: %" G_GOFFSET_FORMAT,
                        PSD_TELL(input));
    }

  return lyr_a;
}

static gint
read_merged_image_block (PSDimage      *img_a,
                         GInputStream  *input,
                         GError       **error)
{
  img_a->merged_image_start = PSD_TELL(input);
  if (! psd_seek (input, 0, G_SEEK_END, error))
    {
      psd_set_error (error);
      return -1;
    }

  img_a->merged_image_len = PSD_TELL(input) - img_a->merged_image_start;

  IFDBG(1) g_debug ("Merged image data block: Start: %" G_GOFFSET_FORMAT ", len: %" G_GOFFSET_FORMAT,
                     img_a->merged_image_start, img_a->merged_image_len);

  return 0;
}

static GimpImage *
create_gimp_image (PSDimage *img_a,
                   GFile    *file)
{
  GimpImage     *image = NULL;
  GimpPrecision  precision;

  switch (img_a->color_mode)
    {
    case PSD_MULTICHANNEL:
    case PSD_GRAYSCALE:
    case PSD_DUOTONE:
      img_a->base_type = GIMP_GRAY;
      break;

    case PSD_BITMAP:
    case PSD_INDEXED:
      img_a->base_type = GIMP_INDEXED;
      break;

    case PSD_LAB:
    case PSD_CMYK:
    case PSD_RGB:
      img_a->base_type = GIMP_RGB;
      break;

    default:
      /* Color mode already validated - should not be here */
      g_warning ("Invalid color mode");
      return NULL;
      break;
    }

    switch (img_a->bps)
      {
      case 32:
        precision = GIMP_PRECISION_FLOAT_NON_LINEAR;
        break;

      case 16:
        if (img_a->color_mode == PSD_CMYK || img_a->color_mode == PSD_LAB)
          precision = GIMP_PRECISION_FLOAT_NON_LINEAR;
        else
          precision = GIMP_PRECISION_U16_NON_LINEAR;
        break;

      case 8:
      case 1:
        if (img_a->color_mode == PSD_CMYK || img_a->color_mode == PSD_LAB)
          precision = GIMP_PRECISION_FLOAT_NON_LINEAR;
        else
          precision = GIMP_PRECISION_U8_NON_LINEAR;
        break;

      default:
        /* Precision not supported */
        g_warning ("Invalid precision");
        return NULL;
        break;
      }

  /* Create gimp image */
  IFDBG(2) g_debug ("Create image");
  image = gimp_image_new_with_precision (img_a->columns, img_a->rows,
                                         img_a->base_type, precision);
  gimp_image_undo_disable (image);

  return image;
}

static gint
add_color_map (GimpImage *image,
               PSDimage  *img_a)
{
  GimpParasite *parasite;

  if (img_a->color_map_len)
    {
      if (img_a->color_mode != PSD_DUOTONE)
        {
          gimp_palette_set_colormap (gimp_image_get_palette (image),
                                     babl_format ("R'G'B' u8"),
                                     img_a->color_map, img_a->color_map_len);
        }
      else
        {
           /* Add parasite for Duotone color data */
          IFDBG(2) g_debug ("Add Duotone color data parasite");
          parasite = gimp_parasite_new (PSD_PARASITE_DUOTONE_DATA, 0,
                                        img_a->color_map_len, img_a->color_map);
          gimp_image_attach_parasite (image, parasite);
          gimp_parasite_free (parasite);
        }
      g_free (img_a->color_map);
    }

  return 0;
}

static gint
add_image_resources (GimpImage     *image,
                     PSDimage      *img_a,
                     GInputStream  *input,
                     gboolean      *resolution_loaded,
                     gboolean      *profile_loaded,
                     GError       **error)
{
  PSDimageres  res_a;

  if (! psd_seek (input, img_a->image_res_start, G_SEEK_SET, error))
    {
      psd_set_error (error);
      g_debug ("Failed to seek in add_image_resources");
      return -1;
    }

  /* Initialise image resource variables */
  img_a->no_icc = FALSE;
  img_a->layer_state = 0;
  img_a->layer_selection = NULL;
  img_a->alpha_names = NULL;
  img_a->alpha_display_info = NULL;
  img_a->alpha_display_count = 0;
  img_a->alpha_id = NULL;
  img_a->alpha_id_count = 0;
  img_a->quick_mask_id = 0;

  while (PSD_TELL(input) < img_a->image_res_start + img_a->image_res_len)
    {
      if (get_image_resource_header (&res_a, input, error) < 0)
        return -1;

      if (res_a.data_start + res_a.data_len >
          img_a->image_res_start + img_a->image_res_len)
        {
          IFDBG(1) g_debug ("Unexpected end of image resource data");
          return 0;
        }

      if (load_image_resource (&res_a, image, img_a, input,
                               resolution_loaded, profile_loaded,
                               error) < 0)
        {
          return -1;
        }
    }

  return 0;
}

static guchar *
psd_convert_cmyk_to_srgb (PSDimage *img_a,
                          guchar   *dst,
                          guchar   *src,
                          gint      width,
                          gint      height,
                          gboolean  alpha,
                          GError  **error)
{
  const Babl *fish        = NULL;
  const Babl *base_format = NULL;
  const Babl *space       = NULL;

  if (img_a->cmyk_profile &&
      gimp_color_profile_is_cmyk (img_a->cmyk_profile))
    {
      /* If no CMYK profile attached, babl_format_with_space ()
       * will ignore the NULL space value and process the image
       * as if it were just using babl_format ()
       */
      space = gimp_color_profile_get_space (img_a->cmyk_profile,
                                            GIMP_COLOR_RENDERING_INTENT_RELATIVE_COLORIMETRIC,
                                            error);
    }

  if (img_a->bps == 8)
    base_format = babl_format_with_space (alpha ? "cmykA u8" : "cmyk u8", space);
  else
    base_format = babl_format_with_space (alpha ? "cmykA u16" : "cmyk u16", space);

  if (! alpha)
    fish = babl_fish (base_format, "R'G'B' float");
  else
    fish = babl_fish (base_format, "R'G'B'A float");

  babl_process (fish, src, dst, width * height);

  return (guchar*) dst;
}

static guchar *
psd_convert_lab_to_srgb (PSDimage *img_a,
                         guchar   *dst,
                         guchar   *src,
                         gint      width,
                         gint      height,
                         gboolean  alpha)
{
  const Babl *fish;
  const Babl *base_format = NULL;

  if (img_a->bps == 8)
    base_format = babl_format (alpha ? "CIE Lab alpha u8" : "CIE Lab u8");
  else
    base_format = babl_format (alpha ? "CIE Lab alpha u16" : "CIE Lab u16");

  if (alpha)
    fish = babl_fish (base_format, "R'G'B'A float");
  else
    fish = babl_fish (base_format, "R'G'B' float");

  babl_process (fish, src, dst, width * height);

  return (guchar*) dst;
}

static gboolean
read_RLE_channel (PSDimage      *img_a,
                  PSDchannel    *lyr_chn,
                  guint64        channel_data_len,
                  GInputStream  *input,
                  GError       **error)
{
  gint      rle_count_size = (img_a->version == 1 ? 2 : 4);
  gint      rle_row_size   = lyr_chn->rows * rle_count_size;
  guint32  *rle_pack_len;
  gint      rowi;

  IFDBG(4) g_debug ("RLE channel length %" G_GSIZE_FORMAT
                    ", RLE length data: %d, "
                    "RLE data block: %" G_GSIZE_FORMAT,
                    channel_data_len - 2,
                    rle_row_size,
                    (channel_data_len - 2 - rle_row_size));
  rle_pack_len = g_malloc (lyr_chn->rows * 4); /* Always 4 since this is the data size in memory. */
  for (rowi = 0; rowi < lyr_chn->rows; ++rowi)
    {
      if (psd_read (input, &rle_pack_len[rowi], rle_count_size,
                    error) < rle_count_size)
        {
          psd_set_error (error);
          g_free (rle_pack_len);
          return FALSE;
        }
      if (img_a->version == 1)
        rle_pack_len[rowi] = img_a->ibm_pc_format                 ?
                             GUINT16_FROM_LE (rle_pack_len[rowi]) :
                             GUINT16_FROM_BE (rle_pack_len[rowi]);
      else
        rle_pack_len[rowi] = img_a->ibm_pc_format                 ?
                             GUINT32_FROM_LE (rle_pack_len[rowi]) :
                             GUINT32_FROM_BE (rle_pack_len[rowi]);
    }

  if (read_channel_data (lyr_chn, img_a->bps,
                         PSD_COMP_RLE, rle_pack_len, input, 0,
                         error) < 1)
    {
      psd_set_error (error);
      g_free (rle_pack_len);
      return FALSE;
    }

  g_free (rle_pack_len);
  return TRUE;
}

static void
free_lyr_chn (PSDchannel **lyr_chn, gint channel_count)
{
  gint cidx;

  for (cidx = 0; cidx < channel_count; ++cidx)
    if (lyr_chn[cidx])
      g_free (lyr_chn[cidx]);
  g_free (lyr_chn);
}

static void
check_duplicate_clipping_group (PSDlayer     **lyr_a,
                                gint16         num_layers,
                                ClippingInfo  *clipping_info)
{
  /* Check whether there is a Photoshop group containing the same layers as
   * the new GIMP clipping group that we intend to add. If that is the case
   * there is no reason to add an extra clipping group.
   * Example image: see layers 55-52 and 40-37 */

  /* 1. There should be a PS group just above where our group_index is */
  if (clipping_info->group_index < num_layers-1 &&
      (lyr_a[clipping_info->group_index+1]->group_type == 1 ||
       lyr_a[clipping_info->group_index+1]->group_type == 2))
    {
      /* 2. The PS group should end right below where our last_index is */
      if (clipping_info->last_index > 0 &&
          lyr_a[clipping_info->last_index-1]->group_type == 3)
        {
          IFDBG(3) g_debug ("\tClipping group is a duplicate, removing extra group from %d to %d",
                     clipping_info->last_index, clipping_info->group_index);
          lyr_a[clipping_info->group_index]->clipping_group_type = 0;
          lyr_a[clipping_info->last_index]->clipping_group_type = 0;
        }
    }
}

static void
mark_clipping_groups (PSDimage  *img_a,
                      PSDlayer **lyr_a)
{
  ClippingInfo  parent_info = {-1, -1};
  GArray       *clipping_group_stack;
  gboolean      use_clipping_group;
  gint          lidx;

  IFDBG(3) g_debug ("Pre process layers to find and mark clipping groups");

  clipping_group_stack = g_array_new (FALSE, FALSE, sizeof (ClippingInfo));
  g_array_append_val (clipping_group_stack, parent_info);

  use_clipping_group = FALSE;

  for (lidx = img_a->num_layers-1; lidx >= 0; --lidx)
    {
      lyr_a[lidx]->unsupported_features = img_a->unsupported_features;

      if (lyr_a[lidx]->clipping == 1)
        {
          /* Photoshop handles layers with clipping differently than GIMP does.
           * To correctly show these layers we need to make a new group
           * starting with the first non-clipping layer and including all
           * the clipping layers above it.
           */
          if (use_clipping_group)
            {
              /* continuation of the clipping group */
              IFDBG(4) g_debug ("[%d] clipping continues", lidx);
            }
          else
            {
              /* top of the clipping group */
              lyr_a[lidx]->clipping_group_type = 2;
              use_clipping_group = TRUE;
              parent_info.group_index = lidx;
              parent_info.last_index = lidx;
              IFDBG(3) g_debug ("[%d] clipping START", lidx);
            }
          if (lyr_a[lidx]->group_type == 1 || lyr_a[lidx]->group_type == 2)
            {
              /* We are going into a group, add parent_info for clipping */
              g_array_append_val (clipping_group_stack, parent_info);
              use_clipping_group = FALSE;
              IFDBG(4) g_debug ("\tDescending into a group");
              parent_info.last_index = -1;
            }
          else if (lyr_a[lidx]->group_type == 3)
            {
              /* bottom of group should never be reached here! */
              g_critical ("Unexpected group end marker, should not happen!");
            }
        }
      else
        {
          if (use_clipping_group)
            {
              /* bottom of the clipping group, unless this is a group,
               * in which case we need to find the group start marker. */
              parent_info.last_index = lidx;
              use_clipping_group = FALSE;
              if (lyr_a[lidx]->group_type == 1 || lyr_a[lidx]->group_type == 2)
                {
                  /* We are going into a group, add parent_info for clipping */
                  g_array_append_val (clipping_group_stack, parent_info);
                  IFDBG(3) g_debug ("[%d] Descending into a group at the bottom of a clipping group",
                                    lidx);
                }
              else
                {
                  lyr_a[lidx]->clipping_group_type = 1; /* start clipping group */
                  IFDBG(3) g_debug ("[%d] Clipping group END: Add group from here to layer %d",
                                    lidx, parent_info.group_index);
                  check_duplicate_clipping_group (lyr_a, img_a->num_layers, &parent_info);
                }
              parent_info.group_index = -1;
              parent_info.last_index = -1;
            }
          else
            {
              /* continuation of normal (non clipping) layers */
              lyr_a[lidx]->clipping_group_type = 0;

              if (lyr_a[lidx]->group_type == 3)
                {
                  /* bottom of group: go back in the stack one level */
                  if (clipping_group_stack->len)
                    {
                      parent_info = g_array_index (clipping_group_stack, ClippingInfo,
                                                  clipping_group_stack->len - 1);
                      g_array_remove_index (clipping_group_stack,
                                            clipping_group_stack->len - 1);
                      if (parent_info.group_index > -1)
                        {
                          /* We can be nested multiple levels deep, in which
                           * case the parent group is also without clipping.
                           * (Issue 13642) */
                          if (parent_info.last_index == -1)
                            {
                              IFDBG(4) g_debug ("[%d] No clipping group ending, going level up", lidx);
                            }
                          /* Test if we were in a group that was the start of a
                           * clipping group, but is itself not clipping!
                           * (example image layers 96-27) */
                          else if (lyr_a[parent_info.last_index]->clipping == 0)
                            {
                              /* found the bottom of the clipping group */
                              lyr_a[lidx]->clipping_group_type = 1; /* start clipping group */
                              parent_info.last_index = lidx;
                              use_clipping_group = FALSE;
                              IFDBG(3) g_debug ("[%d] Clipping group END: Add group from here to layer %d",
                                                lidx, parent_info.group_index);
                              check_duplicate_clipping_group (lyr_a, img_a->num_layers,
                                                              &parent_info);
                              parent_info.group_index = -1;
                              parent_info.last_index  = -1;
                            }
                          else
                            {
                              IFDBG(4) g_debug ("[%d] No clipping group ending, going level up", lidx);
                              use_clipping_group = TRUE;
                            }
                        }
                      else
                        {
                          IFDBG(4) g_debug ("[%d] No clipping group ending, going level up", lidx);
                        }
                    }
                  else
                    {
                      g_critical ("Unmatched group layer start marker at layer %d.", lidx);
                      parent_info.group_index = -1;
                      parent_info.last_index  = -1;
                    }
                }
              else
                {
                  IFDBG(4) g_debug ("[%d] No clipping", lidx);
                  if (lyr_a[lidx]->group_type == 1 || lyr_a[lidx]->group_type == 2)
                    {
                      if (clipping_group_stack->len)
                        {
                          IFDBG(4) g_debug ("\tDescending into a group");
                          /* We are going into a group, add parent_info for clipping */
                          g_array_append_val (clipping_group_stack, parent_info);
                          parent_info.group_index = -1;
                          parent_info.last_index = -1;
                          use_clipping_group = FALSE;
                        }
                    }
                }
            }
        }
    }
  g_array_free (clipping_group_stack, FALSE);
}

static gint
add_layers (GimpImage     *image,
            PSDimage      *img_a,
            PSDlayer     **lyr_a,
            GInputStream  *input,
            GError       **error)
{
  PSDchannel          **lyr_chn;
  GArray               *parent_group_stack;
  GimpLayer            *parent_group = NULL;
  guint16               alpha_chn;
  guint16               user_mask_chn;
  guint16               layer_channels, base_channels;
  guint16               channel_idx[MAX_CHANNELS];
  guint16               bps;
  gint32                l_x;                   /* Layer x */
  gint32                l_y;                   /* Layer y */
  gint32                l_w;                   /* Layer width */
  gint32                l_h;                   /* Layer height */
  gint32                lm_x;                  /* Layer mask x */
  gint32                lm_y;                  /* Layer mask y */
  gint32                lm_w;                  /* Layer mask width */
  gint32                lm_h;                  /* Layer mask height */
  GimpLayer            *layer           = NULL;
  GimpLayerMask        *mask            = NULL;
  GList                *selected_layers = NULL;
  gint                  lidx;                  /* Layer index */
  gint                  cidx;                  /* Channel index */
  gboolean              alpha;
  gboolean              user_mask;
  gboolean              empty;
  gboolean              empty_mask;
  GeglBuffer           *buffer;
  GimpImageType         image_type;
  LayerModeInfo         mode_info;


  IFDBG(2) g_debug ("Number of layers: %d", img_a->num_layers);

  if (img_a->merged_image_only || img_a->num_layers == 0)
    {
      IFDBG(2) g_debug ("No layers to process");
      return 0;
    }

  /* Layered image - Photoshop 3 style */
  if (! psd_seek (input, img_a->layer_data_start, G_SEEK_SET, error))
    {
      psd_set_error (error);
      return -1;
    }

  mark_clipping_groups (img_a, lyr_a);

  /* set the root of the group hierarchy */
  parent_group_stack = g_array_new (FALSE, FALSE, sizeof (GimpLayer *));
  g_array_append_val (parent_group_stack, parent_group);

  for (lidx = 0; lidx < img_a->num_layers; ++lidx)
    {
      IFDBG(2) g_debug ("Process Layer No %d (%s).", lidx, lyr_a[lidx]->name);

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
      lyr_chn = g_new0 (PSDchannel *, lyr_a[lidx]->num_channels);
      for (cidx = 0; cidx < lyr_a[lidx]->num_channels; ++cidx)
        {
          guint16 comp_mode = PSD_COMP_RAW;

          /* Allocate channel record */
          lyr_chn[cidx] = g_malloc (sizeof (PSDchannel) );

          lyr_chn[cidx]->id = lyr_a[lidx]->chn_info[cidx].channel_id;
          lyr_chn[cidx]->rows = lyr_a[lidx]->bottom - lyr_a[lidx]->top;
          lyr_chn[cidx]->columns = lyr_a[lidx]->right - lyr_a[lidx]->left;
          lyr_chn[cidx]->data = NULL;

          if (lyr_chn[cidx]->id == PSD_CHANNEL_EXTRA_MASK)
            {
              if (! psd_seek (input, lyr_a[lidx]->chn_info[cidx].data_len,
                              G_SEEK_CUR, error))
                {
                  psd_set_error (error);
                  free_lyr_chn (lyr_chn, lyr_a[lidx]->num_channels);
                  return -1;
                }

              continue;
            }
          else if (lyr_chn[cidx]->id == PSD_CHANNEL_MASK)
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
              if (psd_read (input, &comp_mode, COMP_MODE_SIZE, error) < COMP_MODE_SIZE)
                {
                  psd_set_error (error);
                  free_lyr_chn (lyr_chn, lyr_a[lidx]->num_channels);
                  return -1;
                }

              if (! img_a->ibm_pc_format)
                comp_mode = GUINT16_FROM_BE (comp_mode);
              else
                comp_mode = GUINT16_FROM_LE (comp_mode);
              IFDBG(3) g_debug ("Compression mode: %d", comp_mode);
            }
          if (lyr_a[lidx]->chn_info[cidx].data_len > COMP_MODE_SIZE)
            {
              switch (comp_mode)
                {
                  case PSD_COMP_RAW:        /* Planar raw data */
                    IFDBG(3) g_debug ("Raw data length: %" G_GSIZE_FORMAT,
                                      lyr_a[lidx]->chn_info[cidx].data_len - 2);
                    if (read_channel_data (lyr_chn[cidx], img_a->bps,
                                           PSD_COMP_RAW, NULL, input, 0,
                                           error) < 1)
                      {
                        psd_set_error (error);
                        free_lyr_chn (lyr_chn, lyr_a[lidx]->num_channels);
                        return -1;
                      }
                    break;

                  case PSD_COMP_RLE:        /* Packbits */
                    if (! read_RLE_channel (img_a, lyr_chn[cidx],
                                            lyr_a[lidx]->chn_info[cidx].data_len,
                                            input, error))
                      {
                        psd_set_error (error);
                        free_lyr_chn (lyr_chn, lyr_a[lidx]->num_channels);
                        return -1;
                      }
                    break;

                  case PSD_COMP_ZIP:                 /* ? */
                  case PSD_COMP_ZIP_PRED:
                    if (read_channel_data (lyr_chn[cidx], img_a->bps,
                                           comp_mode, NULL, input,
                                           lyr_a[lidx]->chn_info[cidx].data_len - 2,
                                           error) < 1)
                      {
                        psd_set_error (error);
                        free_lyr_chn (lyr_chn, lyr_a[lidx]->num_channels);
                        return -1;
                      }
                    break;

                  default:
                    g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
                                 _("Unsupported compression mode: %d"), comp_mode);
                    free_lyr_chn (lyr_chn, lyr_a[lidx]->num_channels);
                    return -1;
                    break;
                }
            }
        }

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
      if (parent_group_stack->len > 0)
        parent_group = g_array_index (parent_group_stack, GimpLayer *,
                                      parent_group_stack->len - 1);
      else
        parent_group = NULL; /* root */

      if (img_a->color_mode == PSD_CMYK)
        base_channels = 4;
      else if (img_a->color_mode == PSD_RGB || img_a->color_mode == PSD_LAB)
        base_channels = 3;
      else
        base_channels = 1;

      IFDBG(3) g_debug ("Re-hash channel indices");
      for (cidx = 0; cidx < lyr_a[lidx]->num_channels; ++cidx)
        {
          IFDBG(3) g_debug ("Channel: %d - id: %d", cidx, lyr_chn[cidx]->id);
          if (lyr_chn[cidx]->id == PSD_CHANNEL_MASK ||
              lyr_chn[cidx]->id == PSD_CHANNEL_EXTRA_MASK)
            {
              /* According to the specs the extra mask (which they call
               * real user supplied layer mask) is used "when both a user
               * mask and a vector mask are present".
               * I haven't seen an example that has the extra mask, so not
               * sure which of the masks would appear first.
               * For now assuming that the extra mask will be first. */
              if (! user_mask)
                {
                  user_mask = TRUE;
                  user_mask_chn = cidx;
                }
              else
                {
                  /* Not using this mask, make sure it gets freed. */
                  g_free (lyr_chn[cidx]->data);
                }
            }
          else if (lyr_chn[cidx]->id == PSD_CHANNEL_ALPHA)
            {
              alpha = TRUE;
              alpha_chn = cidx;
            }
          else if (lyr_chn[cidx]->data)
            {
              if (layer_channels < base_channels)
                {
                  channel_idx[layer_channels] = cidx;   /* Assumes in sane order */
                  layer_channels++;                     /* RGB, Lab, CMYK etc.   */
                }
              else
                {
                  /* channel_idx[base_channels] is reserved for alpha channel,
                   * but this layer apparently has extra channels.
                   * From the one example I have (see #8411) it looks like
                   * that channel is the same as the alpha channel. */
                  IFDBG(2) g_debug ("This layer has an extra channel (id: %d)", lyr_chn[cidx]->id);
                  if (layer_channels == base_channels)
                    {
                      /* Issue: 11131: More than one extra channel is possible.
                       * First extra channel: skip alpha channel */
                      channel_idx[layer_channels+1] = cidx;
                      layer_channels += 2;
                    }
                  else
                    {
                      channel_idx[layer_channels] = cidx;
                      layer_channels++;
                    }
                }
            }
          else
            {
              IFDBG(4) g_debug ("Channel %d (id: %d) has no data", cidx, lyr_chn[cidx]->id);
            }
        }

      if (alpha)
        {
          if (layer_channels <= base_channels)
            {
              channel_idx[layer_channels] = alpha_chn;
              layer_channels++;
            }
          else
            {
              channel_idx[base_channels] = alpha_chn;
            }
          base_channels++;
        }
      else if (layer_channels > base_channels)
        {
          IFDBG(3) g_debug ("No alpha channel, but we have extra channels!");
          /* We have a gap for the non existent alpha channel.
           * Just move the last extra channel there (untested, we don't have examples). */
          channel_idx[base_channels] = channel_idx[layer_channels-1];
          layer_channels--;
        }

      IFDBG(4) g_debug ("Create the layer (group type: %d, clipping group type: %d)",
                        lyr_a[lidx]->group_type, lyr_a[lidx]->clipping_group_type);

      /* Create the layer */

      /* Handle clipping groups first if present, since they don't have
       * actual layer data attached to them. Except if we end with a
       * group, since that group needs to be completed first. */
      if (lyr_a[lidx]->clipping_group_type == 1)
        {
          /* Add a new clipping group */
          IFDBG(2) g_debug ("[%d] Create placeholder clipping group layer", lidx);
          parent_group = add_clipping_group (image, parent_group);
          g_array_append_val (parent_group_stack, parent_group);
        }
      else if (lyr_a[lidx]->clipping_group_type == 2 &&
               lyr_a[lidx]->group_type != 1 &&
               lyr_a[lidx]->group_type != 2)
        {
          /* End of clipping group */
          if (parent_group_stack->len)
            {
              parent_group = g_array_index (parent_group_stack, GimpLayer *,
                                            parent_group_stack->len - 1);
              IFDBG(2) g_debug ("[%d] End clipping group layer.", lidx);
              g_array_remove_index (parent_group_stack,
                                    parent_group_stack->len - 1);
            }
          else
            {
              IFDBG(1) g_debug ("WARNING: Unmatched group layer start marker.");
              layer = NULL;
            }
        }

      if (lyr_a[lidx]->group_type != 0)
        {
          if (lyr_a[lidx]->group_type == 3)
            {
              /* the </Layer group> marker layers are used to
               * assemble the layer structure in a single pass
               */
              IFDBG(2) g_debug ("Create placeholder group layer");
              layer = GIMP_LAYER (gimp_group_layer_new (image, NULL));
              /* add this group layer as the new parent */
              g_array_append_val (parent_group_stack, layer);
            }
          else /* group-type == 1 || group_type == 2 */
            {
              if (parent_group_stack->len)
                {
                  layer = g_array_index (parent_group_stack, GimpLayer *,
                                         parent_group_stack->len - 1);
                  IFDBG(2) g_debug ("End group layer id %d.", gimp_item_get_id (GIMP_ITEM (layer)));
                  /* since the layers are stored in reverse, the group
                   * layer start marker actually means we're done with
                   * that layer group
                   */
                  g_array_remove_index (parent_group_stack,
                                        parent_group_stack->len - 1);

                  gimp_drawable_get_offsets (GIMP_DRAWABLE (layer), &l_x, &l_y);

                  l_w = gimp_drawable_get_width  (GIMP_DRAWABLE (layer));
                  l_h = gimp_drawable_get_height (GIMP_DRAWABLE (layer));
                }
              else
                {
                  IFDBG(1) g_debug ("WARNING: Unmatched group layer start marker.");
                  layer = NULL;
                }
              if (lyr_a[lidx]->clipping_group_type == 2)
                {
                  /* This group is also the top of a clipping group, which we
                   * now need to finish. */
                  if (parent_group_stack->len)
                    {
                      parent_group = g_array_index (parent_group_stack, GimpLayer *,
                                                    parent_group_stack->len - 1);
                      IFDBG(2) g_debug ("[%d] End clipping group layer.", lidx);
                      g_array_remove_index (parent_group_stack,
                                            parent_group_stack->len - 1);
                    }
                  else
                    {
                      IFDBG(1) g_debug ("WARNING: Unmatched group layer start marker.");
                      layer = NULL;
                    }
                }
            }
        }
      else
        {
          if (empty)
            {
              IFDBG(2) g_debug ("Create blank layer");
            }
          else
            {
              IFDBG(2) g_debug ("Create normal layer");
              l_x = lyr_a[lidx]->left;
              l_y = lyr_a[lidx]->top;
              l_w = lyr_a[lidx]->right - lyr_a[lidx]->left;
              l_h = lyr_a[lidx]->bottom - lyr_a[lidx]->top;

              if (l_h <= 0)
                {
                  /* For fill layers this can be -1 apparently;
                     e.g. test file CT_SkillTest_v1.psd
                     Mark as empty and set height to 1.
                   */
                  empty = TRUE;
                  l_h = 1;
                }
              if (l_w <= 0)
                {
                  g_warning ("Unexpected layer width: %d", l_w);
                  empty = TRUE;
                  l_w = 1;
                }
            }

          image_type = get_gimp_image_type (img_a->base_type, TRUE);
          IFDBG(3) g_debug ("Layer type %d", image_type);

          layer = gimp_layer_new (image, lyr_a[lidx]->name,
                                  l_w, l_h, image_type,
                                  100, GIMP_LAYER_MODE_NORMAL);
        }

      if (layer != NULL)
        {
          /* Set the layer name.  Note that we do this even for group-end
           * markers, to avoid having the default group name collide with
           * subsequent layers; the real group name is set by the group
           * start marker.
           */
          if (lyr_a[lidx]->name)
            gimp_item_set_name (GIMP_ITEM (layer), lyr_a[lidx]->name);

          /* Set the layer properties (skip this for layer group end
           * markers; we set their properties when processing the start
           * marker.)
           */
          if (lyr_a[lidx]->group_type != 3)
            {
              /* Mode */
              psd_to_gimp_blend_mode (lyr_a[lidx], &mode_info);
              gimp_layer_set_mode (layer, mode_info.mode);
              gimp_layer_set_blend_space (layer, mode_info.blend_space);
              gimp_layer_set_composite_space (layer, mode_info.composite_space);
              gimp_layer_set_composite_mode (layer, mode_info.composite_mode);

              /* Opacity */
              gimp_layer_set_opacity (layer,
                                      lyr_a[lidx]->opacity * 100.0 / 255.0);

              /* Flags */
              gimp_layer_set_lock_alpha  (layer, lyr_a[lidx]->layer_flags.trans_prot);
              gimp_item_set_visible (GIMP_ITEM (layer), lyr_a[lidx]->layer_flags.visible);
#if 0
              /* according to the spec, the 'irrelevant' flag indicates
               * that the layer's "pixel data is irrelevant to the
               * appearance of the document".  what this seems to mean is
               * not that the layer doesn't contribute to the image, but
               * rather that its appearance can be entirely derived from
               * sources other than the pixel data, such as vector data.
               * in particular, this doesn't mean that the layer is
               * invisible. since we only support raster layers atm, we can
               * just ignore this flag.
               */
              if (lyr_a[lidx]->layer_flags.irrelevant &&
                  lyr_a[lidx]->group_type == 0)
                {
                  gimp_item_set_visible (GIMP_ITEM (layer), FALSE);
                }
#endif

              /* Position */
              if (l_x != 0 || l_y != 0)
                gimp_layer_set_offsets (layer, l_x, l_y);

              /* Color tag */
              gimp_item_set_color_tag (GIMP_ITEM (layer),
                                       psd_to_gimp_layer_color_tag (lyr_a[lidx]->color_tag[0]));

              /* Tattoo */
              if (lyr_a[lidx]->id)
                gimp_item_set_tattoo (GIMP_ITEM (layer), lyr_a[lidx]->id);

              /* For layer groups, expand or collapse the group */
              if (lyr_a[lidx]->group_type != 0)
                {
                  gimp_item_set_expanded (GIMP_ITEM (layer),
                                          lyr_a[lidx]->group_type == 1);
                }
            }

          /* Remember the selected layers:
           * - Layer Selection ID(s) (0x042D) are prioritary;
           * - Layer state information (0x0400) is used instead
           *   otherwise.
           */
          if (img_a->layer_selection)
            {
              if (g_list_find (img_a->layer_selection, GINT_TO_POINTER (lyr_a[lidx]->id)) != NULL)
                {
                  selected_layers = g_list_prepend (selected_layers, layer);
                }
            }
          else if (lidx == img_a->layer_state)
            {
              selected_layers = g_list_prepend (selected_layers, layer);
            }

          /* Set the layer data */
          if (lyr_a[lidx]->group_type == 0)
            {
              IFDBG(3) g_debug ("Draw layer");

              if (empty)
                {
                  gimp_drawable_fill (GIMP_DRAWABLE (layer), GIMP_FILL_TRANSPARENT);
                }
              else
                {
                  GeglBufferIterator *iter;

                  bps = img_a->bps / 8;
                  if (bps == 0)
                    bps++;

                  buffer = gimp_drawable_get_buffer (GIMP_DRAWABLE (layer));

                  iter = gegl_buffer_iterator_new (
                    buffer, NULL, 0, get_layer_format (img_a, alpha),
                    GEGL_ACCESS_WRITE, GEGL_ABYSS_NONE, 1);

                  while (gegl_buffer_iterator_next (iter))
                    {
                      const GeglRectangle *roi      = &iter->items[0].roi;
                      guint8              *dst0     = iter->items[0].data;
                      gint                 src_step = bps;
                      gint                 dst_step = bps * base_channels;

                      if (img_a->color_mode == PSD_CMYK || img_a->color_mode == PSD_LAB)
                        {
                          dst0 = gegl_scratch_alloc (base_channels * bps *
                                                     iter->length);
                        }

                      for (cidx = 0; cidx < base_channels; ++cidx)
                        {
                          gint b;

                          if (roi->x == 0 && roi->y == 0)
                            IFDBG(3) g_debug ("Start channel %d", channel_idx[cidx]);

                          for (b = 0; b < bps; ++b)
                            {
                              guint8 *dst;
                              gint    y;

                              dst = &dst0[cidx * bps + b];

                              for (y = 0; y < roi->height; y++)
                                {
                                  const guint8 *src;
                                  gint          x;

                                  src = (const guint8 *)
                                    &lyr_chn[channel_idx[cidx]]->data[
                                      ((roi->y + y) * l_w +
                                        roi->x)      * bps +
                                      b];

                                  for (x = 0; x < roi->width; ++x)
                                    {
                                      *dst = *src;

                                      src += src_step;
                                      dst += dst_step;
                                    }
                                }
                            }
                        }

                      if (img_a->color_mode == PSD_CMYK)
                        {
                          psd_convert_cmyk_to_srgb (
                            img_a,
                            iter->items[0].data, dst0,
                            roi->width, roi->height,
                            alpha, error);

                          gegl_scratch_free (dst0);
                        }
                      else if (img_a->color_mode == PSD_LAB)
                        {
                          psd_convert_lab_to_srgb (
                            img_a,
                            iter->items[0].data, dst0,
                            roi->width, roi->height,
                            alpha);

                          gegl_scratch_free (dst0);
                        }
                    }

                  for (cidx = 0; cidx < layer_channels; ++cidx)
                    g_free (lyr_chn[channel_idx[cidx]]->data);

                  g_object_unref (buffer);
                }
            }

          /* Layer mask */
          if (user_mask && lyr_a[lidx]->group_type != 3)
            {
              if (empty_mask)
                {
                  IFDBG(3) g_debug ("Create empty mask");
                  if (lyr_a[lidx]->layer_mask.def_color == 255)
                    mask = gimp_layer_create_mask (layer,
                                                   GIMP_ADD_MASK_WHITE);
                  else
                    mask = gimp_layer_create_mask (layer,
                                                   GIMP_ADD_MASK_BLACK);
                  gimp_layer_add_mask (layer, mask);
                  gimp_layer_set_apply_mask (layer,
                                             ! lyr_a[lidx]->layer_mask.mask_flags.disabled);
                }
              else
                {
                  GeglRectangle mask_rect;

                  /* Load layer mask data */
                  lm_x = lyr_a[lidx]->layer_mask.left - l_x;
                  lm_y = lyr_a[lidx]->layer_mask.top - l_y;
                  lm_w = lyr_a[lidx]->layer_mask.right - lyr_a[lidx]->layer_mask.left;
                  lm_h = lyr_a[lidx]->layer_mask.bottom - lyr_a[lidx]->layer_mask.top;
                  IFDBG(3) g_debug ("Mask channel index %d", user_mask_chn);
                  IFDBG(3) g_debug ("Original Mask %d %d %d %d", lm_x, lm_y, lm_w, lm_h);
                  /* Crop mask at layer boundary, and draw layer mask data,
                   * if any
                   */
                  if (gegl_rectangle_intersect (
                        &mask_rect,
                        GEGL_RECTANGLE (0, 0, l_w, l_h),
                        GEGL_RECTANGLE (lm_x, lm_y, lm_w, lm_h)))
                    {
                      IFDBG(3) g_debug ("Layer %d %d %d %d", l_x, l_y, l_w, l_h);
                      IFDBG(3) g_debug ("Mask %d %d %d %d",
                                        mask_rect.x,     mask_rect.y,
                                        mask_rect.width, mask_rect.height);

                      if (lyr_a[lidx]->layer_mask.def_color == 255)
                        mask = gimp_layer_create_mask (layer,
                                                       GIMP_ADD_MASK_WHITE);
                      else
                        mask = gimp_layer_create_mask (layer,
                                                       GIMP_ADD_MASK_BLACK);

                      bps = img_a->bps / 8;
                      if (bps == 0)
                        bps++;

                      IFDBG(3) g_debug ("New layer mask %d", gimp_item_get_id (GIMP_ITEM (mask)));
                      gimp_layer_add_mask (layer, mask);
                      buffer = gimp_drawable_get_buffer (GIMP_DRAWABLE (mask));
                      gegl_buffer_set (buffer,
                                        &mask_rect,
                                        0, get_mask_format (img_a),
                                        lyr_chn[user_mask_chn]->data + (
                                          (mask_rect.y - lm_y)  * lm_w +
                                          (mask_rect.x - lm_x)) * bps,
                                        lm_w * bps);
                      g_object_unref (buffer);
                      gimp_layer_set_apply_mask (layer,
                                                 ! lyr_a[lidx]->layer_mask.mask_flags.disabled);
                    }
                  g_free (lyr_chn[user_mask_chn]->data);
                }
            }

            /* Add legacy layer styles if applicable.
             * TODO: When we can load modern layer styles, only load these if
             * the file doesn't have modern layer style data. */
            if (lyr_a[lidx]->layer_styles->count > 0)
              add_legacy_layer_effects (layer, lyr_a[lidx],
                                        img_a->ibm_pc_format);

            /* Insert the layer */
            if (lyr_a[lidx]->group_type == 0 || /* normal layer */
                lyr_a[lidx]->group_type == 3    /* group layer end marker */)
              {
                gimp_image_insert_layer (image, layer, parent_group, 0);
              }
        }

      free_lyr_chn (lyr_chn, lyr_a[lidx]->num_channels);

      g_free (lyr_a[lidx]->chn_info);
      g_free (lyr_a[lidx]->name);
      g_free (lyr_a[lidx]->layer_styles);
      g_free (lyr_a[lidx]);
    }
  g_free (lyr_a);
  g_array_free (parent_group_stack, FALSE);

  /* Set the selected layers */
  gimp_image_take_selected_layers (image, selected_layers);
  g_list_free (img_a->layer_selection);

  return 0;
}

static void
add_legacy_layer_effects (GimpLayer *layer,
                          PSDlayer  *lyr_a,
                          gboolean   ibm_pc_format)
{
  const Babl *format = gimp_drawable_get_format (GIMP_DRAWABLE (layer));
  const Babl *space  = babl_format_get_space (format);

  if (lyr_a->layer_styles->count == 7 &&
      lyr_a->layer_styles->sofi.enabled == 1)
    {
      PSDLayerStyleSolidFill  sofi;
      GimpLayerMode           mode;
      GimpDrawableFilter     *filter;
      GeglColor              *color = gegl_color_new ("none");

      sofi = lyr_a->layer_styles->sofi;

      convert_legacy_psd_color (color, sofi.natcolor, space, ibm_pc_format);
      convert_psd_mode (sofi.blend, &mode);

      filter = gimp_drawable_append_new_filter (GIMP_DRAWABLE (layer),
                                                "gegl:color-overlay",
                                                NULL,
                                                mode,
                                                sofi.opacity / 255.0,
                                                "value", color,
                                                NULL);

      g_object_unref (filter);
      g_object_unref (color);
    }

  if (lyr_a->layer_styles->isdw.effecton == 1)
    {
      PSDLayerStyleShadow  isdw;
      GimpLayerMode        mode;
      GimpDrawableFilter  *filter;
      GeglColor           *color = gegl_color_new ("none");
      gchar               *filter_name;
      gdouble              x;
      gdouble              y;
      gdouble              blur;
      gdouble              radians;

      isdw = lyr_a->layer_styles->isdw;

      filter_name = g_strdup_printf ("%s (%s)", _("Inner Shadow"),
                                     _("imported"));

      radians = (M_PI / 180) * isdw.angle;
      x       = isdw.distance * cos (radians);
      y       = isdw.distance * sin (radians);

      if (isdw.angle >= 0xFF00)
        isdw.angle = (isdw.angle - 0xFF00) * -1;

      if (isdw.angle > 90 && isdw.angle < 180)
        y *= -1;
      else if (isdw.angle < -90 && isdw.angle > -180)
        x *= -1;

      blur = (isdw.blur / 250.0) * 50.0;

      if (isdw.ver == 0)
        convert_legacy_psd_color (color, isdw.color, space, ibm_pc_format);
      else if (isdw.ver == 2)
        convert_legacy_psd_color (color, isdw.natcolor, space, ibm_pc_format);

      convert_psd_mode (isdw.blendsig, &mode);

      filter = gimp_drawable_append_new_filter (GIMP_DRAWABLE (layer),
                                                "gegl:inner-glow",
                                                filter_name,
                                                mode,
                                                1.0,
                                                "x",           x,
                                                "y",           y,
                                                "grow-radius", 1.0,
                                                "radius",      blur,
                                                "value",       color,
                                                "opacity",     isdw.opacity / 255.0,
                                                NULL);

      g_free (filter_name);
      g_object_unref (filter);
      g_object_unref (color);
    }

  if (lyr_a->layer_styles->dsdw.effecton == 1)
    {
      PSDLayerStyleShadow  dsdw;
      GimpLayerMode        mode;
      GimpDrawableFilter  *filter;
      GeglColor           *color = gegl_color_new ("none");
      gdouble              x;
      gdouble              y;
      gdouble              blur;
      gdouble              radians;

      dsdw = lyr_a->layer_styles->dsdw;

      /* Photoshop uses an angle slider that goes from 0 to 180,
       * then -179 to 0. Since GEGL uses X/Y coordinates for distance,
       * we convert the Photoshop angle and distance and then flip the
       * sign based on the quadrant of the angle */
      radians = (M_PI / 180) * dsdw.angle;
      x       = dsdw.distance * cos (radians);
      y       = dsdw.distance * sin (radians);

      if (dsdw.angle >= 0xFF00)
        dsdw.angle = (dsdw.angle - 0xFF00) * -1;

      if (dsdw.angle > 90 && dsdw.angle < 180)
        y *= -1;
      else if (dsdw.angle < -90 && dsdw.angle > -180)
        x *= -1;

      blur = (dsdw.blur / 250.0) * 100.0;

      if (dsdw.ver == 0)
        convert_legacy_psd_color (color, dsdw.color, space, ibm_pc_format);
      else if (dsdw.ver == 2)
        convert_legacy_psd_color (color, dsdw.natcolor, space, ibm_pc_format);

      convert_psd_mode (dsdw.blendsig, &mode);

      filter = gimp_drawable_append_new_filter (GIMP_DRAWABLE (layer),
                                                "gegl:dropshadow",
                                                NULL,
                                                GIMP_LAYER_MODE_REPLACE,
                                                1.0,
                                                "x",       x,
                                                "y",       y,
                                                "radius",  blur,
                                                "color",   color,
                                                "opacity", dsdw.opacity / 255.0,
                                                NULL);

      g_object_unref (filter);
      g_object_unref (color);
    }
}

static gint
add_merged_image (GimpImage     *image,
                  PSDimage      *img_a,
                  GInputStream  *input,
                  GError       **error)
{
  PSDchannel            chn_a[MAX_CHANNELS];
  gchar                *alpha_name;
  guchar               *pixels;
  guint16               comp_mode = 0;
  guint16               base_channels;
  guint16               extra_channels;
  guint16               total_channels;
  guint16               bps;
  guint32              *rle_pack_len[MAX_CHANNELS];
  guint32               alpha_id;
  gint32                layer_size;
  GimpLayer            *layer   = NULL;
  GimpChannel          *channel = NULL;
  gint16                alpha_opacity;
  gint                  cidx;                  /* Channel index */
  gint                  rowi;                  /* Row index */
  gint                  offset;
  gint                  i;
  gboolean              alpha_visible;
  gboolean              alpha_channel = FALSE;
  GeglBuffer           *buffer;
  GimpImageType         image_type;
  GeglColor            *alpha_rgb;

  total_channels = img_a->channels;
  extra_channels = 0;
  bps = img_a->bps / 8;
  if (bps == 0)
    bps++;

  if ((img_a->color_mode == PSD_BITMAP ||
       img_a->color_mode == PSD_MULTICHANNEL ||
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

  if (extra_channels > 0)
    {
      if (img_a->transparency)
        {
          extra_channels--;
        }
      else if (img_a->alpha_names)
        {
          gchar *alpha_name;

          /* If first alpha_name is Transparency consider it the alpha channel. */
          alpha_name = g_ptr_array_index (img_a->alpha_names, 0);
          if (! g_strcmp0 (alpha_name, "Transparency"))
            {
              alpha_channel = TRUE;
              extra_channels--;
              IFDBG(3) g_debug ("Merged image alpha channel present.");
            }
        }
    }

  base_channels = total_channels - extra_channels;
  /* ----- Read merged image & extra channel pixel data ----- */
  if (img_a->merged_image_only ||
      img_a->num_layers == 0   ||
      extra_channels > 0)
    {
      guint32 block_len;
      guint32 block_start;

      block_start = img_a->merged_image_start;
      block_len = img_a->merged_image_len;

      if (! psd_seek (input, block_start, G_SEEK_SET, error) ||
          psd_read (input, &comp_mode, COMP_MODE_SIZE, error) < COMP_MODE_SIZE)
        {
          psd_set_error (error);
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
                                       PSD_COMP_RAW, NULL, input, 0,
                                       error) < 1)
                  return -1;
              }
            break;

          case PSD_COMP_RLE:        /* Packbits */
            /* Image data is stored as packed scanlines in planar order
               with all compressed length counters stored first */
            {
              gint  rle_count_size = (img_a->version == 1 ? 2 : 4);
              gint  rle_row_size   = img_a->rows * rle_count_size;

              IFDBG(3) g_debug ("RLE length data: %d, RLE data block: %d",
                                total_channels * rle_row_size,
                                block_len - (total_channels * rle_row_size));
              for (cidx = 0; cidx < total_channels; ++cidx)
                {
                  chn_a[cidx].columns = img_a->columns;
                  chn_a[cidx].rows = img_a->rows;
                  rle_pack_len[cidx] = g_malloc (img_a->rows * 4); /* Always 4 since this is the data size in memory. */
                  for (rowi = 0; rowi < img_a->rows; ++rowi)
                    {
                      if (psd_read (input, &rle_pack_len[cidx][rowi], rle_count_size, error) < rle_count_size)
                        {
                          psd_set_error (error);
                          return -1;
                        }
                      if (img_a->version == 1)
                        rle_pack_len[cidx][rowi] = GUINT16_FROM_BE (rle_pack_len[cidx][rowi]);
                      else
                        rle_pack_len[cidx][rowi] = GUINT32_FROM_BE (rle_pack_len[cidx][rowi]);
                    }
                }

              /* Skip channel length data for unloaded channels */
              if (! psd_seek (input, (img_a->channels - total_channels) * rle_row_size,
                              G_SEEK_CUR, error))
                {
                  psd_set_error (error);
                  return -1;
                }

              for (cidx = 0; cidx < total_channels; ++cidx)
                {
                  if (read_channel_data (&chn_a[cidx], img_a->bps,
                                        PSD_COMP_RLE, rle_pack_len[cidx], input, 0,
                                        error) < 1)
                    return -1;
                  g_free (rle_pack_len[cidx]);
                }
              break;
            }

          case PSD_COMP_ZIP:                 /* ? */
          case PSD_COMP_ZIP_PRED:
          default:
            g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
                        _("Unsupported compression mode: %d"), comp_mode);
            return -1;
            break;
        }
    }

  /* ----- Draw merged image ----- */
  if (img_a->merged_image_only ||
      img_a->num_layers == 0)            /* Merged image - Photoshop 2 style */
    {
      image_type = get_gimp_image_type (img_a->base_type,
                                        img_a->transparency || alpha_channel);

      layer_size = img_a->columns * img_a->rows;
      pixels = g_malloc (layer_size * base_channels * bps);
      for (cidx = 0; cidx < base_channels; ++cidx)
        {
          for (i = 0; i < layer_size; ++i)
            {
              memcpy (&pixels[((i * base_channels) + cidx) * bps],
                      &chn_a[cidx].data[i * bps], bps);
            }
          g_free (chn_a[cidx].data);
        }

      /* Add background layer */
      IFDBG(2) g_debug ("Draw merged image");
      layer = gimp_layer_new (image, _("Background"),
                              img_a->columns, img_a->rows,
                              image_type,
                              100,
                              gimp_image_get_default_new_layer_mode (image));
      gimp_image_insert_layer (image, layer, NULL, 0);

      buffer = gimp_drawable_get_buffer (GIMP_DRAWABLE (layer));

      if (img_a->color_mode == PSD_CMYK)
        {
          guchar *dst0;

          dst0 = g_malloc (base_channels * layer_size * sizeof(float));
          psd_convert_cmyk_to_srgb (img_a,
                                    dst0, pixels,
                                    img_a->columns, img_a->rows,
                                    img_a->transparency || alpha_channel,
                                    error);
          g_free (pixels);
          pixels = dst0;
       }
      else if (img_a->color_mode == PSD_LAB)
        {
          guchar *dst0;

          dst0 = g_malloc (base_channels * layer_size * sizeof(float));
          psd_convert_lab_to_srgb (img_a,
                                   dst0, pixels,
                                   img_a->columns, img_a->rows,
                                   img_a->transparency || alpha_channel);
          g_free (pixels);
          pixels = dst0;
        }

      gegl_buffer_set (buffer,
                       GEGL_RECTANGLE (0, 0,
                                       gegl_buffer_get_width (buffer),
                                       gegl_buffer_get_height (buffer)),
                       0, get_layer_format (img_a,
                                            img_a->transparency ||
                                            alpha_channel),
                       pixels, GEGL_AUTO_ROWSTRIDE);

      if (img_a->color_mode == PSD_CMYK)
        img_a->color_mode = PSD_RGB;

      /* Merged image data is blended against white.  Unblend it. */
      if (img_a->transparency)
        {
          GeglBufferIterator *iter;

          IFDBG(4) g_debug ("Unblend merged transparency");
          iter = gegl_buffer_iterator_new (buffer, NULL, 0,
                                           babl_format ("R'G'B'A float"),
                                           GEGL_ACCESS_READWRITE,
                                           GEGL_ABYSS_NONE, 1);

          while (gegl_buffer_iterator_next (iter))
            {
              gfloat *data = iter->items[0].data;

              for (i = 0; i < iter->length; i++)
                {
                  gint c;

                  if (data[3])
                    {
                      for (c = 0; c < 3; c++)
                        data[c] = (data[c] + data[3] - 1.0f) / data[3];
                    }

                  data += 4;
                }
            }
        }

      g_object_unref (buffer);
      g_free (pixels);
    }
  else
    {
      /* Free merged image data for layered image */
      if (extra_channels)
        for (cidx = 0; cidx < base_channels; ++cidx)
          g_free (chn_a[cidx].data);
    }

  if (img_a->transparency)
    {
      /* Free "Transparency" channel name */
      if (img_a->alpha_names)
        {
          alpha_name = g_ptr_array_index (img_a->alpha_names, 0);
          if (alpha_name)
            g_free (alpha_name);
        }
    }

  /* ----- Draw extra alpha channels ----- */
  if (extra_channels                   /* Extra alpha channels */
      && image)
    {
      IFDBG(2) g_debug ("Add extra channels");
      pixels = g_malloc(0);

      /* Get channel resource data */
      if (img_a->transparency)
        offset = 1;
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
              alpha_rgb = gegl_color_duplicate (img_a->alpha_display_info[i + offset]->gimp_color);
              alpha_opacity = img_a->alpha_display_info[i + offset]->opacity;
            }
          else
            {
              alpha_rgb = gegl_color_new ("red");
              alpha_opacity = 50;
            }

          cidx = base_channels + i;
          pixels = g_realloc (pixels, chn_a[cidx].columns * chn_a[cidx].rows * bps);
          memcpy (pixels, chn_a[cidx].data, chn_a[cidx].columns * chn_a[cidx].rows * bps);
          channel = gimp_channel_new (image, alpha_name,
                                      chn_a[cidx].columns, chn_a[cidx].rows,
                                      alpha_opacity, alpha_rgb);
          gimp_image_insert_channel (image, channel, NULL, i);
          g_object_unref (alpha_rgb);
          g_free (alpha_name);
          buffer = gimp_drawable_get_buffer (GIMP_DRAWABLE (channel));
          if (alpha_id)
            gimp_item_set_tattoo (GIMP_ITEM (channel), alpha_id);
          gimp_item_set_visible (GIMP_ITEM (channel), alpha_visible);
          gegl_buffer_set (buffer,
                           GEGL_RECTANGLE (0, 0,
                                           gegl_buffer_get_width (buffer),
                                           gegl_buffer_get_height (buffer)),
                           0, get_channel_format (img_a),
                           pixels, GEGL_AUTO_ROWSTRIDE);
          g_object_unref (buffer);
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
            {
              g_clear_object (&img_a->alpha_display_info[cidx]->gimp_color);
              g_free (img_a->alpha_display_info[cidx]);
            }
          g_free (img_a->alpha_display_info);
        }
    }

  /* FIXME gimp image tattoo state */

  return 0;
}


/* Local utility functions */
static GimpLayer *
add_clipping_group (GimpImage  *image,
                    GimpLayer  *parent)
{
  GimpLayer * clipping_group = NULL;

  /* We need to create a group because GIMP handles clipping and
   * composition mode in a different manner than PS. */
  IFDBG(2) g_debug ("Creating a layer group to handle PS transparency clipping correctly.");

  clipping_group = GIMP_LAYER (gimp_group_layer_new (image, "Group added by GIMP"));

  gimp_layer_set_blend_space (clipping_group, GIMP_LAYER_COLOR_SPACE_RGB_PERCEPTUAL);
  gimp_layer_set_composite_space (clipping_group, GIMP_LAYER_COLOR_SPACE_RGB_PERCEPTUAL);
  gimp_layer_set_composite_mode (clipping_group, GIMP_LAYER_COMPOSITE_UNION);

  gimp_image_insert_layer (image, clipping_group, parent, 0);

  return clipping_group;
}

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
convert_legacy_psd_color (GeglColor  *color,
                          guint16    *psd_color,
                          const Babl *space,
                          gboolean    ibm_pc_format)
{
  guint16 pixel[4];

  for (gint i = 0; i < 4; i++)
    {
      pixel[i] = ibm_pc_format ? psd_color[i + 1] :
                                 GUINT16_TO_BE (psd_color[i + 1]);
    }

  /* This is not specified in the documentation, but based on sample files,
   * the color is assumed to be in the drawable's color space. */
  gegl_color_set_pixel (color, babl_format_with_space ("R'G'B' u16", space),
                        pixel);
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
get_gimp_image_type (GimpImageBaseType image_base_type,
                     gboolean          alpha)
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

static voidpf
zzalloc (voidpf opaque, uInt items, uInt size)
{
  /* overflow check missing */
  return g_malloc (items * size);
}

static void
zzfree (voidpf opaque, voidpf address)
{
  g_free (address);
}

static gint
read_channel_data (PSDchannel     *channel,
                   guint16         bps,
                   guint16         compression,
                   const guint32  *rle_pack_len,
                   GInputStream   *input,
                   guint32         comp_len,
                   GError        **error)
{
  gchar    *raw_data = NULL;
  gchar    *src;
  guint32   readline_len;
  gint      i, j;

  if (bps == 1)
    readline_len = ((channel->columns + 7) / 8);
  else
    readline_len = (channel->columns * bps / 8);

  IFDBG(4) g_debug ("raw data size %d x %d = %d", readline_len,
                    channel->rows, readline_len * channel->rows);

  /* sanity check, int overflow check (avoid divisions by zero) */
  if ((channel->rows == 0) || (channel->columns == 0) ||
      (channel->rows > G_MAXINT32 / channel->columns / MAX (bps / 8, 1)))
    {
      g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
                   _("Unsupported or invalid channel size"));
      return -1;
    }

  raw_data = g_malloc (readline_len * channel->rows);
  switch (compression)
    {
      case PSD_COMP_RAW:
        if (psd_read (input, raw_data, readline_len * channel->rows, error) < readline_len * channel->rows)
          {
            psd_set_error (error);
            g_free (raw_data);
            return -1;
          }
        break;

      case PSD_COMP_RLE:
        for (i = 0; i < channel->rows; ++i)
          {
            src = gegl_scratch_alloc (rle_pack_len[i]);
/*      FIXME check for over-run
            if (PSD_TELL(input) + rle_pack_len[i] > block_end)
              {
                psd_set_error (TRUE, errno, error);
                return -1;
              }
*/
            if (psd_read (input, src, rle_pack_len[i], error) < rle_pack_len[i])
              {
                psd_set_error (error);
                gegl_scratch_free (src);
                g_free (raw_data);
                return -1;
              }
            /* FIXME check for errors returned from decode packbits */
            decode_packbits (src, raw_data + i * readline_len,
                             rle_pack_len[i], readline_len);
            gegl_scratch_free (src);
          }
        break;
      case PSD_COMP_ZIP:
      case PSD_COMP_ZIP_PRED:
        {
          z_stream zs;

          src = g_malloc (comp_len);
          if (psd_read (input, src, comp_len, error) < comp_len)
            {
              psd_set_error (error);
              g_free (src);
              g_free (raw_data);
              return -1;
            }

          zs.next_in = (guchar*) src;
          zs.avail_in = comp_len;
          zs.next_out = (guchar*) raw_data;
          zs.avail_out = readline_len * channel->rows;
          zs.zalloc = zzalloc;
          zs.zfree = zzfree;

          if (inflateInit (&zs) == Z_OK &&
              inflate (&zs, Z_FINISH) == Z_STREAM_END)
            {
              inflateEnd (&zs);
            }
          else
            {
              g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
                           _("Failed to decompress data"));
              g_free (src);
              g_free (raw_data);
              return -1;
            }

          g_free (src);
          break;
        }
    }

  /* Convert channel data to GIMP format */
  switch (bps)
    {
    case 32:
      {
        guint32 *data;
        guint64  pos;

        if (compression == PSD_COMP_ZIP_PRED)
          {
            IFDBG(3) g_debug ("Converting 32 bit predictor data");
            channel->data = (gchar *) g_malloc0 (channel->rows * channel->columns * 4);
            decode_32_bit_predictor (raw_data, channel->data,
                                     channel->rows, channel->columns);
          }
        else
          {
            IFDBG(3) g_debug ("32 bit channel data without predictor");
            channel->data = raw_data;
            raw_data      = NULL;
          }

        data = (guint32*) channel->data;
        for (pos = 0; pos < channel->rows * channel->columns; ++pos)
          data[pos] = GUINT32_FROM_BE (data[pos]);

        break;
      }

    case 16:
      {
        guint16 *data = (guint16*) raw_data;

        channel->data = raw_data;
        raw_data      = NULL;

        for (i = 0; i < channel->rows * channel->columns; ++i)
          data[i] = GUINT16_FROM_BE (data[i]);

        if (compression == PSD_COMP_ZIP_PRED)
          {
            IFDBG(3) g_debug ("Converting 16 bit predictor data");
            for (i = 0; i < channel->rows; ++i)
              for (j = 1; j < channel->columns; ++j)
                data[i * channel->columns + j] += data[i * channel->columns + j - 1];
          }
        break;
      }

      case 8:
        channel->data = raw_data;
        raw_data      = NULL;

        if (compression == PSD_COMP_ZIP_PRED)
          {
            IFDBG(3) g_debug ("Converting 8 bit predictor data");
            for (i = 0; i < channel->rows; ++i)
              for (j = 1; j < channel->columns; ++j)
                channel->data[i * channel->columns + j] += channel->data[i * channel->columns + j - 1];
          }
        break;

      case 1:
        channel->data = (gchar *) g_malloc (channel->rows * channel->columns);
        convert_1_bit (raw_data, channel->data, channel->rows, channel->columns);
        break;

      default:
        g_free (raw_data);
        return -1;
        break;
    }

  g_free (raw_data);

  return 1;
}

/*
 * For reference on zip predictor see:
 * - TIFFTN3d1.pdf
 * - psd_tools
 */

static void
decode_32_bit_predictor (gchar   *src,
                         gchar   *dst,
                         guint32  rows,
                         guint32  columns)
{
  guint32 rowsize;
  guint64 row, dstpos;

  rowsize = columns * 4;

  /* decode delta */
  for (row = 0; row < rows; ++row)
    {
      guint32 j;
      guint64 offset;

      offset = row * rowsize;
      for (j = 0; j < rowsize-1; ++j)
        {
          guint64 pos;

          pos = offset + j;
          src[pos + 1] += src[pos];
        }
    }

  /* restore byte order */
  dstpos = 0;
  for (row = 0; row < rows * rowsize; row += rowsize)
    {
      guint64 offset;

      for (offset = row; offset < row + columns; offset++)
        {
          guint64 x;

          for (x = offset; x < offset + rowsize; x += columns)
            {
              dst[dstpos] = src[x];
              dstpos++;
            }
        }
    }
}

static void
convert_1_bit (const gchar *src,
               gchar       *dst,
               guint32      rows,
               guint32      columns)
{
/* Convert bits to bytes left to right by row.
   Rows are padded out to a byte boundary.
*/
  guint32 row_pos = 0;
  gint    i, j;

  IFDBG(3)  g_debug ("Start 1 bit conversion");

  for (i = 0; i < rows * ((columns + 7) / 8); ++i)
    {
      guchar    mask = 0x80;
      for (j = 0; j < 8 && row_pos < columns; ++j)
        {
          *dst = (*src & mask) ? 0 : 1;
          IFDBG(4) g_debug ("byte %d, bit %d, offset %d, src %d, dst %d",
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

static const Babl*
get_layer_format (PSDimage *img_a,
                  gboolean  alpha)
{
  const Babl *format = NULL;

  switch (get_gimp_image_type (img_a->base_type, alpha))
    {
    case GIMP_GRAY_IMAGE:
      switch (img_a->bps)
        {
        case 32:
          format = babl_format ("Y' float");
          break;

        case 16:
          format = babl_format ("Y' u16");
          break;

        case 8:
        case 1:
          format = babl_format ("Y' u8");

          break;

        default:
          return NULL;
          break;
        }
      break;

    case GIMP_GRAYA_IMAGE:
      switch (img_a->bps)
        {
        case 32:
          format = babl_format ("Y'A float");
          break;

        case 16:
          format = babl_format ("Y'A u16");
          break;

        case 8:
        case 1:
          format = babl_format ("Y'A u8");
          break;

        default:
          return NULL;
          break;
        }
      break;

    case GIMP_RGB_IMAGE:
      switch (img_a->bps)
        {
        case 32:
          format = babl_format ("R'G'B' float");
          break;

        case 16:
          format = babl_format ((img_a->color_mode == PSD_CMYK || img_a->color_mode == PSD_LAB) ?
                                "R'G'B' float" : "R'G'B' u16");
          break;

        case 8:
        case 1:
          format = babl_format ((img_a->color_mode == PSD_CMYK || img_a->color_mode == PSD_LAB) ?
                                "R'G'B' float" : "R'G'B' u8");
          break;

        default:
          return NULL;
          break;
        }
      break;

    case GIMP_RGBA_IMAGE:
      switch (img_a->bps)
        {
        case 32:
          format = babl_format ("R'G'B'A float");
          break;

        case 16:
          format = babl_format ((img_a->color_mode == PSD_CMYK || img_a->color_mode == PSD_LAB) ?
                                "R'G'B'A float" : "R'G'B'A u16");
          break;

        case 8:
        case 1:
          format = babl_format ((img_a->color_mode == PSD_CMYK || img_a->color_mode == PSD_LAB) ?
                                "R'G'B'A float" : "R'G'B'A u8");
          break;

        default:
          return NULL;
          break;
        }
      break;

    default:
      return NULL;
      break;
    }

  return format;
}

static const Babl*
get_channel_format (PSDimage *img_a)
{
  const Babl *format = NULL;

  switch (img_a->bps)
    {
    case 32:
      format = babl_format ("Y float");
      break;

    case 16:
      format = babl_format ("Y u16");
      break;

    case 8:
    case 1:
      /* see gimp_image_get_channel_format() */
      format = babl_format ("Y' u8");
      break;

    default:
      break;
    }

  return format;
}

static const Babl*
get_mask_format (PSDimage *img_a)
{
  const Babl *format = NULL;

  switch (img_a->bps)
    {
    case 32:
      format = babl_format ("Y float");
      break;

    case 16:
      format = babl_format ("Y u16");
      break;

    case 8:
    case 1:
      format = babl_format ("Y u8");
      break;

    default:
      break;
    }

  return format;
}

static void
initialize_unsupported (PSDSupport *unsupported_features)
{
  unsupported_features->show_gui         = FALSE;
  unsupported_features->duotone_mode     = FALSE;

  unsupported_features->adjustment_layer = FALSE;
  unsupported_features->fill_layer       = FALSE;
  unsupported_features->text_layer       = FALSE;
  unsupported_features->linked_layer     = FALSE;
  unsupported_features->vector_mask      = FALSE;
  unsupported_features->smart_object     = FALSE;
  unsupported_features->stroke           = FALSE;
  unsupported_features->layer_effect     = FALSE;
  unsupported_features->layer_comp       = FALSE;
  unsupported_features->psd_metadata     = FALSE;
}

void
load_dialog (const gchar *title,
             PSDSupport  *unsupported_features)
{
  GtkWidget *dialog;
  GtkWidget *label;
  GtkWidget *vbox;

  dialog = gimp_dialog_new (title,
                            "import-psd",
                            NULL, 0, NULL, NULL,
                            _("_OK"), GTK_RESPONSE_OK,
                            NULL);

  gimp_window_set_transient (GTK_WINDOW (dialog));

  vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 2);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), 12);
  gtk_box_pack_start (GTK_BOX (gtk_dialog_get_content_area (GTK_DIALOG (dialog))),
                      vbox, TRUE, TRUE, 0);
  gtk_widget_show (vbox);

  /* Duotone import notification */
  if (unsupported_features->duotone_mode)
    {
      gchar *label_text;

      label_text = g_strdup_printf ("<b>%s</b>\n%s", _("Duotone Import"),
                                    _("Image will be imported as Grayscale.\n"
                                      "Duotone color space data has been saved\n"
                                      "and can be reapplied on export."));
      label = gtk_label_new (NULL);
      gtk_label_set_markup (GTK_LABEL (label), label_text);

      gtk_label_set_selectable (GTK_LABEL (label), TRUE);
      gtk_label_set_justify (GTK_LABEL (label), GTK_JUSTIFY_LEFT);
      gtk_label_set_line_wrap (GTK_LABEL (label), TRUE);
      gtk_label_set_yalign (GTK_LABEL (label), 0.0);
      gtk_box_pack_start (GTK_BOX (vbox), label, TRUE, TRUE, 0);
      gtk_widget_show (label);

      g_free (label_text);
    }

  if (unsupported_features->show_gui)
    {
      GtkWidget *scrolled_window;
      GtkWidget *title_label;
      gchar     *message = g_strdup ("");
      gchar     *title   = g_strdup_printf ("<b>%s</b>\n%s\n",
                                            _("Compatibility Notice"),
                                            _("This PSD file contains features "
                                              "that\nare not yet fully supported "
                                              "in GIMP:"));

      title_label = gtk_label_new (NULL);
      gtk_label_set_markup (GTK_LABEL (title_label), title);
      gtk_label_set_justify (GTK_LABEL (title_label), GTK_JUSTIFY_LEFT);
      gtk_label_set_line_wrap (GTK_LABEL (title_label), TRUE);
      gtk_label_set_yalign (GTK_LABEL (title_label), 0.0);
      gtk_box_pack_start (GTK_BOX (vbox), title_label, FALSE, FALSE, 0);
      gtk_widget_show (title_label);

#define ADD_UNSUPPORTED_MESSAGE(message, text)                                \
        {                                                                     \
          gchar *old_message = message;                                       \
          message = g_strdup_printf ("%s\n\xE2\x80\xA2 %s", old_message,      \
                                     _(text));                                \
          g_free (old_message);                                               \
        }                                                                     \

      if (unsupported_features->adjustment_layer)
        ADD_UNSUPPORTED_MESSAGE (message,
                                 "Adjustment layers are not yet "
                                 "supported and will show up as empty layers.");
      if (unsupported_features->psd_metadata)
        ADD_UNSUPPORTED_MESSAGE (message,
                                 "Metadata non-raster layers are not yet "
                                 "supported and will show up as empty layers.");
      if (unsupported_features->fill_layer)
        ADD_UNSUPPORTED_MESSAGE (message,
                                 "Fill layers are partially "
                                 "supported and will be converted "
                                 "to raster layers.");
      if (unsupported_features->text_layer)
        ADD_UNSUPPORTED_MESSAGE (message,
                                 "Text layers are partially "
                                 "supported and will be converted "
                                 "to raster layers.");
      if (unsupported_features->linked_layer)
        ADD_UNSUPPORTED_MESSAGE (message,
                                 "Linked layers are not yet "
                                 "supported and will show up as empty layers.");
      if (unsupported_features->vector_mask)
        ADD_UNSUPPORTED_MESSAGE (message,
                                 "Vector masks are partially "
                                 "supported and will be converted "
                                 "to raster layers.");
      if (unsupported_features->stroke)
        ADD_UNSUPPORTED_MESSAGE (message,
                                 "Vector strokes are not yet "
                                 "supported and will show up as empty layers.");
      if (unsupported_features->layer_effect)
        ADD_UNSUPPORTED_MESSAGE (message,
                                 "Layer effects are not yet "
                                 "supported and will show up as empty layers.");
      if (unsupported_features->smart_object)
        ADD_UNSUPPORTED_MESSAGE (message,
                                 "Smart objects are not yet "
                                 "supported and will show up as empty layers.");
      if (unsupported_features->layer_comp)
        ADD_UNSUPPORTED_MESSAGE (message,
                                 /* Translators: short for layer compositions */
                                 "Layer comps are not yet "
                                 "supported and will show up as empty layers.");
#undef ADD_UNSUPPORTED_MESSAGE

      scrolled_window = gtk_scrolled_window_new (NULL, NULL);
      gtk_widget_set_size_request (scrolled_window, -1, 100);
      gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (scrolled_window), GTK_SHADOW_OUT);
      gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_window),
                                      GTK_POLICY_NEVER,
                                      GTK_POLICY_AUTOMATIC);
      gtk_box_pack_start (GTK_BOX (vbox), scrolled_window, TRUE, TRUE, 0);
      gtk_widget_show (scrolled_window);

      label = gtk_label_new (NULL);
      gtk_label_set_markup (GTK_LABEL (label), message);
      gtk_label_set_justify (GTK_LABEL (label), GTK_JUSTIFY_LEFT);
      gtk_label_set_line_wrap (GTK_LABEL (label), TRUE);
      gtk_label_set_selectable (GTK_LABEL (label), TRUE);
      gtk_label_set_yalign (GTK_LABEL (label), 0.0);
      gtk_container_add (GTK_CONTAINER (scrolled_window), label);
      gtk_widget_show (label);

      g_free (title);
      g_free (message);
    }

  gtk_widget_show (dialog);

  /* run the dialog */
  gimp_dialog_run (GIMP_DIALOG (dialog));

  gtk_widget_destroy (dialog);
}
