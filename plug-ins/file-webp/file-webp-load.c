/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * file-webp - WebP file format plug-in for the GIMP
 * Copyright (C) 2015  Nathan Osman
 * Copyright (C) 2016  Ben Touchette
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
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

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include <webp/decode.h>
#include <webp/demux.h>
#include <webp/mux.h>

#include <gegl.h>

#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

#include "file-webp-load.h"

#include "libgimp/stdplugins-intl.h"


static void
create_layer (GimpImage *image,
              uint8_t   *layer_data,
              gint32     position,
              gchar     *name,
              gint       width,
              gint       height)
{
  GimpLayer     *layer;
  GeglBuffer    *buffer;
  GeglRectangle  extent;

  layer = gimp_layer_new (image, name,
                          width, height,
                          GIMP_RGBA_IMAGE,
                          100,
                          gimp_image_get_default_new_layer_mode (image));

  gimp_image_insert_layer (image, layer, NULL, position);

  /* Retrieve the buffer for the layer */
  buffer = gimp_drawable_get_buffer (GIMP_DRAWABLE (layer));

  /* Copy the image data to the region */
  gegl_rectangle_set (&extent, 0, 0, width, height);
  gegl_buffer_set (buffer, &extent, 0, NULL, layer_data,
                   GEGL_AUTO_ROWSTRIDE);

  /* Flush the drawable and detach */
  gegl_buffer_flush (buffer);
  g_object_unref (buffer);
}

GimpImage *
load_image (GFile                  *file,
            gboolean                interactive,
            GimpMetadataLoadFlags  *metadata_flags,
            GError                **error)
{
  uint8_t          *indata = NULL;
  gsize             indatalen;
  gint              width;
  gint              height;
  GimpImage        *image;
  WebPMux          *mux;
  WebPData          wp_data;
  GimpColorProfile *profile   = NULL;
  uint32_t          flags;
  gboolean          animation = FALSE;
  gboolean          icc       = FALSE;

  /* Attempt to read the file contents from disk */
  if (! g_file_get_contents (g_file_peek_path (file),
                             (gchar **) &indata,
                             &indatalen,
                             error))
    {
      return NULL;
    }

  /* Validate WebP data */
  if (! WebPGetInfo (indata, indatalen, &width, &height))
    {
      g_set_error (error, G_FILE_ERROR, 0,
                   _("Invalid WebP file '%s'"),
                   gimp_file_get_utf8_name (file));
      return NULL;
    }

  wp_data.bytes = indata;
  wp_data.size  = indatalen;

  mux = WebPMuxCreate (&wp_data, 1);
  if (! mux)
    {
      return NULL;
    }

  WebPMuxGetFeatures (mux, &flags);

  if (flags & ANIMATION_FLAG)
    animation = TRUE;

  if (flags & ICCP_FLAG)
    icc = TRUE;

  /* TODO: decode the image in "chunks" or "tiles" */
  /* TODO: check if an alpha channel is present */

  /* Create the new image and associated layer */
  image = gimp_image_new (width, height, GIMP_RGB);

  if (icc)
    {
      WebPData icc_profile;

      WebPMuxGetChunk (mux, "ICCP", &icc_profile);
      profile = gimp_color_profile_new_from_icc_profile (icc_profile.bytes,
                                                         icc_profile.size, NULL);
      if (profile)
        gimp_image_set_color_profile (image, profile);
    }

  if (! animation)
    {
      uint8_t *outdata;

      /* Attempt to decode the data as a WebP image */
      outdata = WebPDecodeRGBA (indata, indatalen, &width, &height);

      /* Check to ensure the image data was loaded correctly */
      if (! outdata)
        {
          WebPMuxDelete (mux);
          return NULL;
        }

      create_layer (image, outdata, 0, _("Background"),
                    width, height);

      /* Free the image data */
      free (outdata);
    }
  else
    {
      WebPAnimDecoder       *dec = NULL;
      WebPAnimInfo           anim_info;
      WebPAnimDecoderOptions dec_options;
      gint                   frame_num = 1;
      WebPDemuxer           *demux     = NULL;
      WebPIterator           iter      = { 0, };

      if (! WebPAnimDecoderOptionsInit (&dec_options))
        {
        error:
          if (dec)
            WebPAnimDecoderDelete (dec);

          if (demux)
            {
              WebPDemuxReleaseIterator (&iter);
              WebPDemuxDelete (demux);
            }

          WebPMuxDelete (mux);
          return NULL;
        }

      /* dec_options.color_mode is MODE_RGBA by default here */
      dec = WebPAnimDecoderNew (&wp_data, &dec_options);
      if (! dec)
        {
          g_set_error (error, G_FILE_ERROR, 0,
                       _("Failed to decode animated WebP file '%s'"),
                       gimp_file_get_utf8_name (file));
          goto error;
        }

      if (! WebPAnimDecoderGetInfo (dec, &anim_info))
        {
          g_set_error (error, G_FILE_ERROR, 0,
                       _("Failed to decode animated WebP information from '%s'"),
                       gimp_file_get_utf8_name (file));
          goto error;
        }

      demux = WebPDemux (&wp_data);
      if (! demux || ! WebPDemuxGetFrame (demux, 1, &iter))
        goto error;

      /* Attempt to decode the data as a WebP animation image */
      while (WebPAnimDecoderHasMoreFrames (dec))
        {
          uint8_t *outdata;
          int      timestamp;
          gchar   *name;

          if (! WebPAnimDecoderGetNext (dec, &outdata, &timestamp))
            {
              g_set_error (error, G_FILE_ERROR, 0,
                           _("Failed to decode animated WebP frame from '%s'"),
                           gimp_file_get_utf8_name (file));
              goto error;
            }

          name = g_strdup_printf (_("Frame %d (%dms)"), frame_num, iter.duration);
          create_layer (image, outdata, 0, name, width, height);
          g_free (name);

          frame_num++;
          WebPDemuxNextFrame (&iter);
        }

      WebPAnimDecoderDelete (dec);
      WebPDemuxReleaseIterator (&iter);
      WebPDemuxDelete (demux);
    }

  /* Free the original compressed data */
  g_free (indata);

  if (profile)
    *metadata_flags &= ~GIMP_METADATA_LOAD_COLORSPACE;

  WebPMuxDelete (mux);

  if (profile)
    g_object_unref (profile);

  return image;
}
