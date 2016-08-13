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
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include <gegl.h>

#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

#include <webp/decode.h>
#include <webp/demux.h>
#include <webp/mux.h>

#include "file-webp-load.h"

#include "libgimp/stdplugins-intl.h"


static void
create_layer (gint32   image_ID,
              uint8_t *layer_data,
              gint32   position,
              gchar   *name,
              gint     width,
              gint     height,
              gint32   offsetx,
              gint32   offsety)
{
  gint32         layer_ID;
  GeglBuffer    *buffer;
  GeglRectangle  extent;

  layer_ID = gimp_layer_new (image_ID, name,
                             width, height,
                             GIMP_RGBA_IMAGE,
                             100,
                             GIMP_NORMAL_MODE);

  gimp_image_insert_layer (image_ID, layer_ID, -1, position);

  if (offsetx > 0 || offsety > 0)
    {
      gimp_layer_set_offsets (layer_ID, offsetx, offsety);
    }

  /* Retrieve the buffer for the layer */
  buffer = gimp_drawable_get_buffer (layer_ID);

  /* Copy the image data to the region */
  gegl_rectangle_set (&extent, 0, 0, width, height);
  gegl_buffer_set (buffer, &extent, 0, NULL, layer_data,
                   GEGL_AUTO_ROWSTRIDE);

  /* Flush the drawable and detach */
  gegl_buffer_flush (buffer);
  g_object_unref (buffer);
}

gint32
load_image (const gchar *filename,
            gboolean     interactive,
            GError      **error)
{
  uint8_t  *indata = NULL;
  gsize     indatalen;
  gint      width;
  gint      height;
  gint32    image_ID;
  WebPMux  *mux;
  WebPData  wp_data;
  uint32_t  flags;
  gboolean  animation = FALSE;
  gboolean  icc       = FALSE;
  gboolean  exif      = FALSE;
  gboolean  xmp       = FALSE;

  /* Attempt to read the file contents from disk */
  if (! g_file_get_contents (filename,
                             (gchar **) &indata,
                             &indatalen,
                             error))
    {
      return -1;
    }

  g_printerr ("Loading WebP file %s\n", filename);

  /* Validate WebP data */
  if (! WebPGetInfo (indata, indatalen, &width, &height))
    {
      g_printerr ("Invalid WebP file\n");
      return -1;
    }

  wp_data.bytes = indata;
  wp_data.size  = indatalen;

  mux = WebPMuxCreate (&wp_data, 1);
  if (! mux)
    return -1;

  WebPMuxGetFeatures (mux, &flags);

  if (flags & ANIMATION_FLAG)
    animation = TRUE;

  if (flags & ICCP_FLAG)
    icc = TRUE;

  if (flags & EXIF_FLAG)
    exif = TRUE;

  if (flags & XMP_FLAG)
    xmp = TRUE;

  /* TODO: decode the image in "chunks" or "tiles" */
  /* TODO: check if an alpha channel is present */

  /* Create the new image and associated layer */
  image_ID = gimp_image_new (width, height, GIMP_RGB);

  if (! animation)
    {
      uint8_t *outdata;

      /* Attempt to decode the data as a WebP image */
      outdata = WebPDecodeRGBA (indata, indatalen, &width, &height);

      /* Free the original compressed data */
      g_free (indata);

      /* Check to ensure the image data was loaded correctly */
      if (! outdata)
        return -1;

      create_layer (image_ID, outdata, 0, _("Background"),
                    width, height, 0, 0);

      /* Free the image data */
      free (outdata);
    }
  else
    {
      const WebPChunkId id = WEBP_CHUNK_ANMF;
      WebPMuxAnimParams params;
      gint              frames = 0;
      gint              i;

      WebPMuxGetAnimationParams (mux, &params);
      WebPMuxNumChunks (mux, id, &frames);

      /* Attempt to decode the data as a WebP animation image */
      for (i = 0; i < frames; i++)
        {
          WebPMuxFrameInfo thisframe;

          if (WebPMuxGetFrame (mux, i, &thisframe) == WEBP_MUX_OK)
            {
              WebPBitstreamFeatures  features;
              uint8_t               *outdata;
              gchar                 *name;

              WebPGetFeatures (thisframe.bitstream.bytes,
                               thisframe.bitstream.size, &features);

              outdata = WebPDecodeRGBA (thisframe.bitstream.bytes,
                                        thisframe.bitstream.size,
                                        &width, &height);

              if (! outdata)
                return -1;

              name = g_strdup_printf (_("Frame %d"), i + 1);
              create_layer (image_ID, outdata, 0,
                            name, width, height,
                            thisframe.x_offset,
                            thisframe.y_offset);
              g_free (name);

              /* Free the image data */
              free (outdata);
            }

          WebPDataClear (&thisframe.bitstream);
        }
    }

  WebPDataClear (&wp_data);

  if (icc)
    {
      WebPData          icc_profile;
      GimpColorProfile *profile;

      WebPMuxGetChunk (mux, "ICCP", &icc_profile);
      profile = gimp_color_profile_new_from_icc_profile (icc_profile.bytes,
                                                         icc_profile.size, NULL);
      if (profile)
        {
          gimp_image_set_color_profile (image_ID, profile);
          g_object_unref (profile);
        }
    }

  if (exif || xmp)
    {
      GimpMetadata *metadata;
      GFile        *file;

      if (exif)
        {
          WebPData exif;

          WebPMuxGetChunk (mux, "EXIF", &exif);
        }

      if (xmp)
        {
          WebPData xmp;

          WebPMuxGetChunk (mux, "XMP ", &xmp);
        }

      file = g_file_new_for_path (filename);
      metadata = gimp_image_metadata_load_prepare (image_ID, "image/webp",
                                                   file, NULL);
      if (metadata)
        {
          gimp_image_metadata_load_finish (image_ID, "image/webp",
                                           metadata, GIMP_METADATA_LOAD_ALL,
                                           interactive);
          g_object_unref (metadata);
        }

      g_object_unref (file);
    }

  gimp_image_set_filename (image_ID, filename);

  return image_ID;
}
