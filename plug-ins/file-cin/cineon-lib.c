/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * Cineon image file format library routines.
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

#include "cineon-lib.h"

#include "libgimp/stdplugins-intl.h"


static gboolean       read_8bpc_line  (GeglBuffer *buffer,
                                       FILE       *fp,
                                       guint      *data,
                                       guint       data_len,
                                       gint        depth,
                                       guchar     *pixels,
                                       guint       num_pixels,
                                       gushort     packing,
                                       gboolean    is_network_order,
                                       GError    **error);

static gboolean       read_10bpc_line (GeglBuffer *buffer,
                                       guint      *data,
                                       guint       data_len,
                                       gint        depth,
                                       gushort    *pixels,
                                       guint       num_pixels,
                                       gushort     packing,
                                       gboolean    is_network_order,
                                       GError    **error);

GimpImage *
cineon_open (GFile   *file,
             GError **error)
{
  CineonGenericHeader  header;
  FILE                *fp;
  GimpImage           *image  = NULL;
  GimpImageBaseType    image_type;
  GimpImageType        layer_type;
  GimpLayer           *layer;
  GimpPrecision        precision;
  GeglBuffer          *cin_buffer;
  guint                width;
  guint                height;
  gint                 depth;
  gint                 bpp;
  gint                 offset;
  gint                 packing = 1;
  guint                buffer_length;
  guint               *buffer = NULL;
  gushort             *pixels = NULL;
  guchar              *pixels_8 = NULL;

  fp = g_fopen (g_file_peek_path (file), "rb");
  if (! fp)
    {
      g_set_error (error, G_FILE_ERROR, g_file_error_from_errno (errno),
                   _("Could not open '%s' for reading: %s"),
                   gimp_file_get_utf8_name (file), g_strerror (errno));
      return NULL;
    }

  if (fread (&header, sizeof (CineonGenericHeader), 1, fp) == 0)
    {
      g_set_error (error, G_FILE_ERROR, g_file_error_from_errno (errno),
                   _("Could not read Cineon header information."));

      fclose (fp);
      return NULL;
    }

  width  = g_ntohl (header.imageInfo.channel[0].pixels_per_line);
  height = g_ntohl (header.imageInfo.channel[0].lines_per_image);
  depth  = header.imageInfo.channels_per_image;
  bpp    = header.imageInfo.channel[0].bits_per_pixel;
  offset = g_ntohl (header.fileInfo.image_offset);

  if (width > GIMP_MAX_IMAGE_SIZE  ||
      height > GIMP_MAX_IMAGE_SIZE)
    {
      g_set_error (error, GIMP_PLUG_IN_ERROR, 0,
                   _("Image dimensions too large: width %d x height %d"),
                   width, height);
      fclose (fp);
      return NULL;
    }

  switch (depth)
    {
    case 1:
      image_type = GIMP_GRAY;
      layer_type = GIMP_GRAY_IMAGE;
      break;

    case 3:
      image_type = GIMP_RGB;
      layer_type = GIMP_RGB_IMAGE;
      break;

    default:
      g_set_error (error, GIMP_PLUG_IN_ERROR, 0,
                   _("Cineon: Image with %d channels not yet supported."),
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
      buffer_length = ((width * depth) + 2) / 3;
      break;

    default:
      g_set_error (error, GIMP_PLUG_IN_ERROR, 0,
                   _("Cineon: Image with %d bpc not yet supported."),
                   bpp);
      fclose (fp);
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
      fclose (fp);
      g_free (buffer);
      g_free (pixels);
      g_free (pixels_8);
      return NULL;
    }

  image = gimp_image_new_with_precision (width, height, image_type,
                                         precision);

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

  cin_buffer = gimp_drawable_get_buffer (GIMP_DRAWABLE (layer));
  for (gint y = 0; y < height; y++)
    {
      gint read_index = 0;

      read_index = fread (buffer, 4, buffer_length, fp);
      if (read_index != buffer_length)
        {
          g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
                       _("Could not read image data from '%s'"),
                       gimp_file_get_utf8_name (file));
          fclose (fp);
          g_object_unref (cin_buffer);
          g_free (buffer);
          g_free (pixels);
          g_free (pixels_8);

          return image;
        }

      if (bpp == 8)
        read_8bpc_line (cin_buffer, fp, buffer, buffer_length, depth,
                        pixels_8, (width * depth), packing, TRUE, error);
      else if (bpp == 10)
        read_10bpc_line (cin_buffer, buffer, buffer_length, depth, pixels,
                         (width * depth), packing, TRUE, error);

      if (bpp > 8)
        gegl_buffer_set (cin_buffer, GEGL_RECTANGLE (0, y, width, 1), 0,
                         NULL, pixels, GEGL_AUTO_ROWSTRIDE);
      else
        gegl_buffer_set (cin_buffer, GEGL_RECTANGLE (0, y, width, 1), 0,
                         NULL, pixels_8, GEGL_AUTO_ROWSTRIDE);
    }

  g_object_unref (cin_buffer);
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
        t = g_ntohl (data[long_index]);
      else
        t = data[long_index];

      pixels[pixel_index]     = (guchar) ((t & 0xFF000000) >> 24) & 0xFF;
      pixels[pixel_index + 1] = (guchar) ((t & 0x00FF0000) >> 16) & 0xFF;
      pixels[pixel_index + 2] = (guchar) ((t & 0x0000FF00) >> 8) & 0xFF;
      pixels[pixel_index + 3] = (guchar) (t & 0x000000FF);

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
