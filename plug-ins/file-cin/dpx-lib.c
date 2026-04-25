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
  guint                buffer_length = 0;
  guint               *buffer = NULL;
  gushort             *pixels = NULL;
  gint                 left_over = 0;

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
      width  = g_ntohl (header.imageInfo.pixels_per_line);
      height = g_ntohl (header.imageInfo.lines_per_image);
      depth  = g_ntohs (header.imageInfo.channels_per_image);
      offset = g_ntohl (header.fileInfo.offset);
    }
  else
    {
      width  = header.imageInfo.pixels_per_line;
      height = header.imageInfo.lines_per_image;
      depth  = header.imageInfo.channels_per_image;
      offset = header.fileInfo.offset;
    }
  bpp = header.imageInfo.channel[0].bits_per_pixel;

  if (depth == 1)
    {
      switch (header.imageInfo.channel[0].designator1)
        {
        case 50:
          depth = 3;
          break;
        case 51:
          depth = 4;
          break;
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
      g_free (buffer);
      g_free (pixels);
      return NULL;
    }

  switch (bpp)
    {
      //case 8:
        //precision = GIMP_PRECISION_U8_NON_LINEAR;
        //break;

    case 10:
      buffer_length = ((width * depth) + 2) / 3;
      precision     = GIMP_PRECISION_U16_NON_LINEAR;
      break;

    case 16:
      buffer_length = (width * depth) / 2;
      precision     = GIMP_PRECISION_U16_NON_LINEAR;
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
  pixels = g_try_malloc ((gsize) buffer_length * 3 * 2);

  if (! buffer || ! pixels)
    {
      g_set_error (error, GIMP_PLUG_IN_ERROR, 0,
                   _("Memory could not be allocated."));
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
      return NULL;
    }

  dpx_buffer = gimp_drawable_get_buffer (GIMP_DRAWABLE (layer));

  for (gint y = 0; y < height; y++)
    {
      gint pixel_index = 0;
      gint long_index  = 0;
      gint read_index  = 0;
      gint read_data   = buffer_length;

      if (bpp == 10)
        {
          read_data   = (((width * depth) - left_over) + 2) / 3;
          pixel_index = left_over;
        }

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

          return image;
        }

      if (bpp == 10)
        {
          for (long_index = 0; long_index < read_data; ++long_index)
            {
              guint t;

              if (is_network_order)
                t = g_ntohl (buffer[long_index]);
              else
                t = buffer[long_index];

              if (depth == 1)
                {
                  pixels[pixel_index] = (gushort) t & 0x3FF;
                  t = t >> 10;
                  pixels[pixel_index + 1] = (gushort) t & 0x3FF;
                  t = t >> 10;
                  pixels[pixel_index + 2] = (gushort) t & 0x3FF;
                }
              else
                {
                  t = t >> 2;
                  pixels[pixel_index + 2] = (gushort) t & 0x3FF;
                  t = t >> 10;
                  pixels[pixel_index + 1] = (gushort) t & 0x3FF;
                  t = t >> 10;
                  pixels[pixel_index] = (gushort) t & 0x3FF;
                }
              pixel_index += 3;
            }
          left_over = pixel_index;
        }
      else if (bpp == 16)
        {
          /* TODO: Handle 1-off indexing for odd widths */
          for (long_index = 0; long_index < read_data; long_index += 2)
            {
              guint t[3] = { 0, 0, 0 };

              for (gint i = 0; i < 3; i++)
                {
                  if ((long_index + i) >= read_data)
                    break;

                  t[i] = buffer[long_index + i];

                  if (is_network_order)
                    t[i] = g_ntohl (t[i]);
                }

              if (is_network_order)
                {
                  pixels[pixel_index]     = (gushort) ((t[0] >> 16) & 0xFFFF);
                  pixels[pixel_index + 1] = (gushort) (t[0] & 0xFFFF);

                  pixels[pixel_index + 2] = (gushort) ((t[1] >> 16) & 0xFFFF);
                  pixels[pixel_index + 3] = (gushort) (t[1] & 0xFFFF);

                  if ((long_index + 2) >= read_data)
                    {
                      fseek (fp, 4, SEEK_CUR);
                      break;
                    }

                  pixels[pixel_index + 4] = (gushort) ((t[2] >> 16) & 0xFFFF);
                  pixels[pixel_index + 5] = (gushort) (t[2] & 0xFFFF);
                }
              else
                {
                  pixels[pixel_index + 5] = (gushort) (t[0] & 0xFFFF);
                  pixels[pixel_index + 4] = (gushort) ((t[0] >> 16) & 0xFFFF);

                  pixels[pixel_index + 3] = (gushort) (t[1] & 0xFFFF);
                  pixels[pixel_index + 2] = (gushort) ((t[1] >> 16) & 0xFFFF);

                  if ((long_index + 2) >= read_data)
                    {
                      fseek (fp, 4, SEEK_CUR);
                      break;
                    }

                  pixels[pixel_index + 1] = (gushort) (t[2] & 0xFFFF);
                  pixels[pixel_index]     = (gushort) ((t[2] >> 16) & 0xFFFF);
                }
              pixel_index += 6;
              long_index++;
            }
        }

      if (bpp == 10)
        {
          for (pixel_index = 0; pixel_index < (width * depth); ++pixel_index)
            pixels[pixel_index] <<= 6;

          while (pixel_index < left_over)
            {
              pixels[pixel_index - (width * depth)] = pixels[pixel_index];
              ++pixel_index;
            }
        }
      left_over -= (width * depth);

      gegl_buffer_set (dpx_buffer, GEGL_RECTANGLE (0, y, width, 1), 0,
                       NULL, pixels,
                       GEGL_AUTO_ROWSTRIDE);
    }

  g_object_unref (dpx_buffer);
  g_free (buffer);
  g_free (pixels);
  fclose (fp);

  return image;
}
