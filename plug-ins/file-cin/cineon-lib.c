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
  GeglBuffer          *cin_buffer;
  guint                width;
  guint                height;
  gint                 depth;
  gint                 bpp;
  gint                 offset;
  guint                buffer_length;
  guint               *buffer = NULL;
  gushort             *pixels = NULL;

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

  if (bpp != 10)
    {
      g_set_error (error, GIMP_PLUG_IN_ERROR, 0,
                   _("Cineon: %d bits per channel not yet supported."),
                   bpp);
      fclose (fp);
      return NULL;
    }

  buffer_length = ((width * depth) + 2) / 3;
  buffer        = g_try_malloc ((gsize) buffer_length * 4);
  pixels        = g_try_malloc ((gsize) buffer_length * 3 * sizeof (gushort));

  if (! buffer || ! pixels)
    {
       g_set_error (error, GIMP_PLUG_IN_ERROR, 0,
                    _("Memory could not be allocated."));
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
      g_free (buffer);
      g_free (pixels);
      return NULL;
    }

  image = gimp_image_new_with_precision (width, height, image_type,
                                         GIMP_PRECISION_U16_NON_LINEAR);

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
      return NULL;
    }

  cin_buffer = gimp_drawable_get_buffer (GIMP_DRAWABLE (layer));
  for (gint y = 0; y < height; y++)
    {
      gint pixel_index = 0;
      gint long_index  = 0;
      gint read_index  = 0;

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

          return image;
        }

      for (long_index = 0; long_index < buffer_length; ++long_index)
        {
          guint t = g_ntohl (buffer[long_index]);

          t = t >> 2;
          pixels[pixel_index + 2] = (gushort) t & 0x3FF;
          t = t >> 10;
          pixels[pixel_index + 1] = (gushort) t & 0x3FF;
          t = t >> 10;
          pixels[pixel_index] = (gushort) t & 0x3FF;
          pixel_index += 3;
        }

      for (pixel_index = 0; pixel_index < (width * depth); ++pixel_index)
        pixels[pixel_index] <<= 6;

      gegl_buffer_set (cin_buffer, GEGL_RECTANGLE (0, y, width, 1), 0,
                       NULL, pixels,
                       GEGL_AUTO_ROWSTRIDE);
    }

  g_object_unref (cin_buffer);
  g_free (buffer);
  g_free (pixels);
  fclose (fp);

  return image;
}