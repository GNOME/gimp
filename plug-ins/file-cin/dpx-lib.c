/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * DPX image file format library routines.
 *
 * Copyright 1999,2000,2001 David Hodson <hodsond@acm.org>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include "config.h"

#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <time.h>        /* strftime() */
#include <sys/types.h>
#include <string.h>      /* memset */

#include <glib/gstdio.h>

#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

#include "dpx-lib.h"

#include "libgimp/stdplugins-intl.h"

#define DPX_FILE_MAGIC 0x53445058

static gboolean       read_8bpc_line    (GeglBuffer *buffer,
                                         FILE       *fp,
                                         guint      *data,
                                         guint       data_len,
                                         gint        depth,
                                         guchar     *pixels,
                                         guint       num_pixels,
                                         gushort     packing,
                                         gboolean    is_network_order,
                                         GError    **error);

static gboolean       read_10bpc_line   (GeglBuffer *buffer,
                                         guint      *data,
                                         guint       data_len,
                                         gint        depth,
                                         gushort    *pixels,
                                         guint       num_pixels,
                                         gushort     packing,
                                         gboolean    is_network_order,
                                         GError    **error);

static gboolean       read_12bpc_line   (GeglBuffer *buffer,
                                         FILE       *fp,
                                         guint      *data,
                                         guint       data_len,
                                         gint        depth,
                                         gushort    *pixels,
                                         guint       num_pixels,
                                         gushort     packing,
                                         gboolean    is_network_order,
                                         GError    **error);

static gboolean       read_12bpc_packed (GeglBuffer *buffer,
                                         FILE       *fp,
                                         guint       width,
                                         guint       height,
                                         gint        depth,
                                         gboolean    is_network_order,
                                         GError    **error);

static gboolean       read_16bpc_line   (GeglBuffer *buffer,
                                         FILE       *fp,
                                         guint      *data,
                                         guint       data_len,
                                         gint        depth,
                                         gushort    *pixels,
                                         guint       num_pixels,
                                         gushort     packing,
                                         gboolean    is_network_order,
                                         GError    **error);


GimpImage *
dpx_open (GFile   *file,
          GError **error)
{
  DpxMainHeader        header;
  FILE                *fp;
  GimpImage           *image  = NULL;
  GimpImageBaseType    image_type;
  GimpImageType        layer_type;
  GimpPrecision        precision;
  GimpLayer           *layer;
  GeglBuffer          *dpx_buffer;
  gboolean             is_network_order = TRUE;
  guint                width;
  guint                height;
  gint                 depth;
  gint                 bpp;
  gint                 offset;
  gushort              packing;
  guint                buffer_length = 0;
  guint               *buffer        = NULL;
  gushort             *pixels        = NULL;
  guchar              *pixels_8      = NULL;

  fp = g_fopen (g_file_peek_path (file), "rb");
  if (! fp)
    {
      g_set_error (error, G_FILE_ERROR, g_file_error_from_errno (errno),
                   _("Could not open '%s' for reading: %s"),
                   gimp_file_get_utf8_name (file), g_strerror (errno));
      return NULL;
    }

  if (fread (&header, sizeof (DpxMainHeader), 1, fp) == 0)
    {
      g_set_error (error, G_FILE_ERROR, g_file_error_from_errno (errno),
                   _("Could not read DPX header information."));

      fclose (fp);
      return NULL;
    }

  /* The order of the magic number determines the byte order */
  if (header.fileInfo.magic_num == g_ntohl (DPX_FILE_MAGIC))
    {
      is_network_order = TRUE;
    }
  else if (header.fileInfo.magic_num == DPX_FILE_MAGIC)
    {
      is_network_order = FALSE;
    }
  else
    {
      g_set_error (error, G_FILE_ERROR, g_file_error_from_errno (errno),
                   _("Could not read DPX header information."));

      fclose (fp);
      return NULL;
    }

  if (is_network_order)
    {
      width       = g_ntohl (header.imageInfo.pixels_per_line);
      height      = g_ntohl (header.imageInfo.lines_per_image);
      depth       = g_ntohs (header.imageInfo.channels_per_image);
      offset      = g_ntohl (header.fileInfo.offset);
      packing     = g_ntohs (header.imageInfo.channel[0].packing);
    }
  else
    {
      width       = header.imageInfo.pixels_per_line;
      height      = header.imageInfo.lines_per_image;
      depth       = header.imageInfo.channels_per_image;
      offset      = header.fileInfo.offset;
      packing     = header.imageInfo.channel[0].packing;
    }
  bpp = header.imageInfo.channel[0].bits_per_pixel;

  if (width > GIMP_MAX_IMAGE_SIZE  ||
      height > GIMP_MAX_IMAGE_SIZE)
    {
      g_set_error (error, GIMP_PLUG_IN_ERROR, 0,
                   _("Image dimensions too large: width %d x height %d"),
                   width, height);
      fclose (fp);
      return NULL;
    }

  if (depth == 1)
    {
      switch (header.imageInfo.channel[0].designator1)
        {
        case 50:
          depth = 3;
          break;

        case 51:
        case 52:
          depth = 4;
          break;

        default:
          break;
        }
    }

  switch (depth)
    {
    case 1:
      image_type = GIMP_GRAY;
      layer_type = GIMP_GRAY_IMAGE;
      break;

    case 2:
      image_type = GIMP_GRAY;
      layer_type = GIMP_GRAYA_IMAGE;
      break;

    case 3:
      image_type = GIMP_RGB;
      layer_type = GIMP_RGB_IMAGE;
      break;

    case 4:
      image_type = GIMP_RGB;
      layer_type = GIMP_RGBA_IMAGE;
      break;

    default:
      g_set_error (error, GIMP_PLUG_IN_ERROR, 0,
                   _("DPX: Image with %d channels not yet supported."),
                   depth);
      fclose (fp);
      return NULL;
    }

  switch (bpp)
    {
    case 8:
      precision     = GIMP_PRECISION_U8_NON_LINEAR;
      buffer_length = ceil ((width * depth) / 4.0);
      break;

    case 10:
      precision     = GIMP_PRECISION_U16_NON_LINEAR;
      buffer_length = ceil (((width * depth) + 2) / 3.0);
      break;

    case 12:
      precision     = GIMP_PRECISION_U16_NON_LINEAR;
      buffer_length = ceil ((width * depth) / 2.0);
      break;

    case 16:
      precision     = GIMP_PRECISION_U16_NON_LINEAR;
      buffer_length = ceil ((width * depth) / 2.0);
      break;

    default:
      g_set_error (error, GIMP_PLUG_IN_ERROR, 0,
                   _("DPX: Image with %d bpc not yet supported."),
                   bpp);
      fclose (fp);
      g_free (buffer);
      g_free (pixels);
      return NULL;
    }

  buffer = g_try_malloc ((gsize) buffer_length * 4);
  if (bpp > 8)
    pixels = g_try_malloc ((gsize) buffer_length * 3 * 2);
  else
    pixels_8 = g_try_malloc ((gsize) buffer_length * 3 * 2);

  if (! buffer || (! pixels && ! pixels_8))
    {
      g_set_error (error, GIMP_PLUG_IN_ERROR, 0,
                   _("Memory could not be allocated."));
      g_free (buffer);
      g_free (pixels);
      g_free (pixels_8);
      fclose (fp);
      return NULL;
    }

  image = gimp_image_new_with_precision (width, height, image_type, precision);
  layer = gimp_layer_new (image, NULL, width, height,
                          layer_type, 100,
                          gimp_image_get_default_new_layer_mode (image));
  gimp_image_insert_layer (image, layer, NULL, 0);

  /* Jump to Image data */
  if (fseek (fp, offset, SEEK_SET) != 0)
    {
      g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
                   _("Could not read image data from '%s'"),
                   gimp_file_get_utf8_name (file));
      fclose (fp);
      g_free (buffer);
      g_free (pixels);
      g_free (pixels_8);
      return NULL;
    }

  dpx_buffer = gimp_drawable_get_buffer (GIMP_DRAWABLE (layer));

  if (packing == 0 &&
      (bpp == 10   ||
       bpp == 12))
    {
      if (bpp == 12 &&
          ! read_12bpc_packed (dpx_buffer, fp, width, height, depth,
                                 is_network_order, error))
        {
           g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
                        _("Could not read image data from '%s'"),
                        gimp_file_get_utf8_name (file));
           fclose (fp);
           g_free (buffer);
           g_free (pixels);
           g_free (pixels_8);
           return NULL;
        }
    }
  else
    {
      for (gint y = 0; y < height; y++)
        {
          gint read_index  = 0;
          gint read_data   = buffer_length;

          if (bpp == 10)
            read_data = ((width * depth) + 2) / 3;

          read_index = fread (buffer, 4, read_data, fp);
          if (read_index != read_data)
            {
              g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
                           _("Could not read image data from '%s'"),
                           gimp_file_get_utf8_name (file));
              fclose (fp);
              g_object_unref (dpx_buffer);
              g_free (buffer);
              g_free (pixels);
              g_free (pixels_8);

              return image;
            }

          if (bpp == 8)
            {
              read_8bpc_line (dpx_buffer, fp, buffer, read_data, depth, pixels_8,
                              (width * depth), packing, is_network_order, error);
            }
          else if (bpp == 10)
            {
              read_10bpc_line (dpx_buffer, buffer, read_data, depth, pixels,
                               (width * depth), packing, is_network_order,
                               error);
            }
          else if (bpp == 12)
            {
              read_12bpc_line (dpx_buffer, fp, buffer, read_data, depth, pixels,
                               (width * depth), packing, is_network_order, error);
            }
          else if (bpp == 16)
            {
              read_16bpc_line (dpx_buffer, fp, buffer, read_data, depth, pixels,
                               (width * depth), packing, is_network_order,
                               error);
            }

          if (bpp > 8)
            gegl_buffer_set (dpx_buffer, GEGL_RECTANGLE (0, y, width, 1), 0,
                             NULL, pixels, GEGL_AUTO_ROWSTRIDE);
          else
            gegl_buffer_set (dpx_buffer, GEGL_RECTANGLE (0, y, width, 1), 0,
                             NULL, pixels_8, GEGL_AUTO_ROWSTRIDE);
        }
    }

  g_object_unref (dpx_buffer);
  g_free (buffer);
  g_free (pixels);
  g_free (pixels_8);
  fclose (fp);

  return image;
}

/* Helper methods */

static gboolean
read_8bpc_line (GeglBuffer *buffer,
                FILE       *fp,
                guint      *data,
                guint       data_len,
                gint        depth,
                guchar     *pixels,
                guint       num_pixels,
                gushort     packing,
                gboolean    is_network_order,
                GError    **error)
{
  gint pixel_index = 0;

  for (gint long_index = 0; long_index < data_len; ++long_index)
    {
      guint t;

      if (is_network_order)
        {
          t = g_ntohl (data[long_index]);

          pixels[pixel_index]     = (guchar) ((t & 0xFF000000) >> 24) & 0xFF;
          pixels[pixel_index + 1] = (guchar) ((t & 0x00FF0000) >> 16) & 0xFF;
          pixels[pixel_index + 2] = (guchar) ((t & 0x0000FF00) >> 8) & 0xFF;
          pixels[pixel_index + 3] = (guchar) (t & 0x000000FF);
        }
      else
        {
          t = data[long_index];

          pixels[pixel_index + 3] = (guchar) ((t & 0xFF000000) >> 24) & 0xFF;
          pixels[pixel_index + 2] = (guchar) ((t & 0x00FF0000) >> 16) & 0xFF;
          pixels[pixel_index + 1] = (guchar) ((t & 0x0000FF00) >> 8) & 0xFF;
          pixels[pixel_index]     = (guchar) (t & 0x000000FF);
        }
      pixel_index += 4;
    }

  return TRUE;
}

static gboolean
read_10bpc_line (GeglBuffer *buffer,
                 guint      *data,
                 guint       data_len,
                 gint        depth,
                 gushort    *pixels,
                 guint       num_pixels,
                 gushort     packing,
                 gboolean    is_network_order,
                 GError    **error)
{
  gint pixel_index = 0;

  for (gint long_index = 0; long_index < data_len; ++long_index)
    {
      guint t;

      if (is_network_order)
        t = g_ntohl (data[long_index]);
      else
        t = data[long_index];

      t = t >> 2;
      pixels[pixel_index + 2] = ((gushort) t & 0x3FF) << 6;
      t = t >> 10;
      pixels[pixel_index + 1] = ((gushort) t & 0x3FF) << 6;
      t = t >> 10;
      pixels[pixel_index]     = ((gushort) t & 0x3FF) << 6;

      pixel_index += 3;
    }

  return TRUE;
}

static gboolean
read_12bpc_line (GeglBuffer *buffer,
                 FILE       *fp,
                 guint      *data,
                 guint       data_len,
                 gint        depth,
                 gushort    *pixels,
                 guint       num_pixels,
                 gushort     packing,
                 gboolean    is_network_order,
                 GError    **error)
{
  gint pixel_index = 0;

  for (gint long_index = 0; long_index < data_len; ++long_index)
    {
      guint t;

      if (is_network_order)
        {
          t = g_ntohl (data[long_index]);

          t = t >> 4;
          pixels[pixel_index + 1] = ((gushort) t & 0xFFF) << 4;
          t = t >> 16;
          pixels[pixel_index]     = ((gushort) t & 0xFFF) << 4;
        }
      else
        {
          t = data[long_index];

          t = t >> 4;
          pixels[pixel_index]     = ((gushort) t & 0xFFF) << 4;
          t = t >> 16;
          pixels[pixel_index + 1] = ((gushort) t & 0xFFF) << 4;
        }
      pixel_index += 2;
    }

  return TRUE;
}

static gboolean
read_12bpc_packed (GeglBuffer *buffer,
                   FILE       *fp,
                   guint       width,
                   guint       height,
                   gint        depth,
                   gboolean    is_network_order,
                   GError    **error)
{
  gsize    current;
  gsize    data_len;
  gsize    num_pixels;
  gsize    pixel_len;
  guint   *data;
  gushort *pixels;
  guint    pixel_index = 0;
  guint    leftover    = 0;
  gint     shift_1     = 12;
  gint     shift_2     = 0;
  guint    mask        = 1;
  guint    odd_offset  = 0;

  current = ftell (fp);
  fseek (fp, 0, SEEK_END);
  data_len = ftell (fp);
  fseek (fp, current, SEEK_SET);

  data_len -= current;
  data = g_try_malloc0 (data_len);
  if (data == NULL)
    return FALSE;

  data_len /= 4;
  if (fread (data, 4, data_len, fp) == 0                    ||
      ! g_size_checked_mul (&num_pixels, width, height)     ||
      ! g_size_checked_mul (&num_pixels, num_pixels, depth) ||
      ! g_size_checked_mul (&pixel_len, num_pixels, 2)      ||
      ! g_size_checked_add (&pixel_len, pixel_len, 8))
   {
     g_free (data);
     return FALSE;
   }

  pixels = g_try_malloc0 (pixel_len);
  if (pixels == NULL)
    {
      g_free (data);
      return FALSE;
    }

  for (gint long_index = 0; long_index < data_len; ++long_index)
    {
      guint t;

      if (is_network_order)
        {
          t = g_ntohl (data[long_index]);

          /* Get remaining bytes from prior run */
          if (shift_1 < 12)
            {
              leftover += ((gushort) t & mask) << shift_1;
              pixels[pixel_index] = (leftover & 0xFFF) << 4;
              t = t >> shift_2;

              pixel_index++;
              odd_offset++;

              /* If the dimensions of the image are odd, the remaining
               * packed bits are not part of the image and should be
               * skipped */
              if (odd_offset >= (width * depth))
                {
                  odd_offset = 0;
                  shift_1    = 12;
                  shift_2    = 0;
                  mask       = 0;
                  continue;
                }
            }

          /* Read remaining 12 bit pixel values */
          pixels[pixel_index] = ((gushort) t & 0xFFF) << 4;
          t = t >> 12;
          pixels[pixel_index + 1] = ((gushort) t & 0xFFF) << 4;
          t = t >> 12;
          leftover = (gushort) t;

          pixel_index += 2;
          odd_offset  += 2;
          shift_1     -= 4;
          shift_2     += 4;
          mask        = (mask << 4) + 0xF;
          if (shift_1 == 0)
            {
              shift_1 = 12;
              shift_2 = 0;
              mask    = 0;
            }

          if (odd_offset >= (width * depth))
            odd_offset = 0;

          if (pixel_index + 3 > num_pixels)
            break;
        }
    }

  gegl_buffer_set (buffer, GEGL_RECTANGLE (0, 0, width, height), 0,
                   NULL, pixels, GEGL_AUTO_ROWSTRIDE);

  return TRUE;
}

static gboolean
read_16bpc_line (GeglBuffer *buffer,
                 FILE       *fp,
                 guint      *data,
                 guint       data_len,
                 gint        depth,
                 gushort    *pixels,
                 guint       num_pixels,
                 gushort     packing,
                 gboolean    is_network_order,
                 GError    **error)
{
  gint pixel_index = 0;

  for (gint long_index = 0; long_index < data_len; ++long_index)
    {
      guint t;

      if (is_network_order)
        {
          t = g_ntohl (data[long_index]);

          pixels[pixel_index]     = (gushort) ((t >> 16) & 0xFFFF);
          pixels[pixel_index + 1] = (gushort) (t & 0xFFFF);
        }
      else
        {
          t = data[long_index];

          pixels[pixel_index + 1] = (gushort) ((t >> 16) & 0xFFFF);
          pixels[pixel_index]     = (gushort) (t & 0xFFFF);
        }
      pixel_index += 2;
    }

  return TRUE;
}
