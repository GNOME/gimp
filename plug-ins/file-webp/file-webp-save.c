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

#include <errno.h>
#include <glib/gstdio.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#include <gegl.h>

#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

#include <webp/encode.h>
#include <webp/mux.h>

#include "file-webp-save.h"

#include "libgimp/stdplugins-intl.h"


WebPPreset    webp_preset_by_name   (gchar             *name);
int           webp_anim_file_writer (FILE              *outfile,
                                     const uint8_t     *data,
                                     size_t             data_size);
int           webp_file_writer      (const uint8_t     *data,
                                     size_t             data_size,
                                     const WebPPicture *picture);
int           webp_file_progress    (int                percent,
                                     const WebPPicture *picture);
const gchar * webp_error_string     (WebPEncodingError  error_code);

gboolean      save_layer            (const gchar       *filename,
                                     gint32             nLayers,
                                     gint32             image_ID,
                                     gint32             drawable_ID,
                                     WebPSaveParams    *params,
                                     GError           **error);

gboolean      save_animation        (const gchar       *filename,
                                     gint32             nLayers,
                                     gint32            *allLayers,
                                     gint32             image_ID,
                                     gint32             drawable_ID,
                                     WebPSaveParams    *params,
                                     GError           **error);


WebPPreset
webp_preset_by_name (gchar *name)
{
  if( ! strcmp (name, "picture"))
    {
      return WEBP_PRESET_PICTURE;
    }
  else if (! strcmp (name, "photo"))
    {
      return WEBP_PRESET_PHOTO;
    }
  else if (! strcmp (name, "drawing"))
    {
      return WEBP_PRESET_DRAWING;
    }
  else if (! strcmp (name, "icon"))
    {
      return WEBP_PRESET_ICON;
    }
  else if (! strcmp (name, "text"))
    {
      return WEBP_PRESET_TEXT;
    }
  else
    {
      return WEBP_PRESET_DEFAULT;
    }
}

int
webp_anim_file_writer (FILE          *outfile,
                       const uint8_t *data,
                       size_t         data_size)
{
  int ok = 0;

  if (data == NULL)
    return 0;

  ok = (fwrite (data, data_size, 1, outfile) == 1);

  return ok;
}

int
webp_file_writer (const uint8_t     *data,
                  size_t             data_size,
                  const WebPPicture *picture)
{
  FILE *outfile;

  /* Obtain the FILE* and write the data to the file */
  outfile = (FILE *) picture->custom_ptr;

  return fwrite (data, sizeof (uint8_t), data_size, outfile) == data_size;
}

int
webp_file_progress (int                percent,
                    const WebPPicture *picture)
{
  return gimp_progress_update (percent / 100.0);
}

const gchar *
webp_error_string (WebPEncodingError error_code)
{
  switch (error_code)
    {
    case VP8_ENC_ERROR_OUT_OF_MEMORY:
      return _("out of memory");
    case VP8_ENC_ERROR_BITSTREAM_OUT_OF_MEMORY:
      return _("not enough memory to flush bits");
    case VP8_ENC_ERROR_NULL_PARAMETER:
      return _("NULL parameter");
    case VP8_ENC_ERROR_INVALID_CONFIGURATION:
      return _("invalid configuration");
    case VP8_ENC_ERROR_BAD_DIMENSION:
      return _("bad image dimensions");
    case VP8_ENC_ERROR_PARTITION0_OVERFLOW:
      return _("partition is bigger than 512K");
    case VP8_ENC_ERROR_PARTITION_OVERFLOW:
      return _("partition is bigger than 16M");
    case VP8_ENC_ERROR_BAD_WRITE:
      return _("unable to flush bytes");
    case VP8_ENC_ERROR_FILE_TOO_BIG:
      return _("file is larger than 4GiB");
    case VP8_ENC_ERROR_USER_ABORT:
      return _("user aborted encoding");
    case VP8_ENC_ERROR_LAST:
      return _("list terminator");
    default:
      return _("unknown error");
    }
}

gboolean
save_layer (const gchar    *filename,
            gint32          nLayers,
            gint32          image_ID,
            gint32          drawable_ID,
            WebPSaveParams *params,
            GError        **error)
{
  gboolean          status   = FALSE;
  FILE             *outfile  = NULL;
  WebPConfig        config   = {0};
  WebPPicture       picture  = {0};
  guchar           *buffer   = NULL;
  gint              w, h;
  gint              bpp;
  GimpColorProfile *profile;
  GimpImageType     drawable_type;
  GeglBuffer       *geglbuffer = NULL;
  GeglRectangle     extent;
  gchar            *indata;
  gsize             indatalen;
  struct            stat stsz;
  int               fd_outfile;
  WebPData          chunk;
  int               res;

  /* The do...while() loop is a neat little trick that makes it easier
   * to jump to error handling code while still ensuring proper
   * cleanup
   */

  do
    {
      /* Begin displaying export progress */
      gimp_progress_init_printf (_("Saving '%s'"),
                                 gimp_filename_to_utf8(filename));

      /* Attempt to open the output file */
      if ((outfile = g_fopen (filename, "wb+")) == NULL)
        {
          g_set_error (error, G_FILE_ERROR,
                       g_file_error_from_errno (errno),
                       _("Unable to open '%s' for writing"),
                       gimp_filename_to_utf8 (filename));
          break;
        }

      /* Obtain the drawable type */
      drawable_type = gimp_drawable_type (drawable_ID);

      /* Retrieve the buffer for the layer */
      geglbuffer = gimp_drawable_get_buffer (drawable_ID);
      extent = *gegl_buffer_get_extent (geglbuffer);
      bpp = gimp_drawable_bpp (drawable_ID);
      w = extent.width;
      h = extent.height;

      /* Initialize the WebP configuration with a preset and fill in the
       * remaining values */
      WebPConfigPreset (&config,
                        webp_preset_by_name (params->preset),
                        params->quality);

      config.lossless      = params->lossless;
      config.method        = 6;  /* better quality */
      config.alpha_quality = params->alpha_quality;

      /* Prepare the WebP structure */
      WebPPictureInit (&picture);
      picture.use_argb      = 1;
      picture.width         = w;
      picture.height        = h;
      picture.writer        = webp_file_writer;
      picture.custom_ptr    = outfile;
      picture.progress_hook = webp_file_progress;

      /* Attempt to allocate a buffer of the appropriate size */
      buffer = (guchar *) g_malloc (w * h * bpp);
      if(! buffer)
        break;

      /* Read the region into the buffer */
      gegl_buffer_get (geglbuffer, &extent, 1.0, NULL, buffer,
                       GEGL_AUTO_ROWSTRIDE, GEGL_ABYSS_NONE);

      /* Use the appropriate function to import the data from the buffer */
      if (drawable_type == GIMP_RGB_IMAGE)
        {
          WebPPictureImportRGB (&picture, buffer, w * bpp);
        }
      else
        {
          WebPPictureImportRGBA (&picture, buffer, w * bpp);
        }

      /* Perform the actual encode */
      if (! WebPEncode (&config, &picture))
        {
          g_printerr ("WebP error: '%s'",
                      webp_error_string (picture.error_code));
          g_set_error (error, G_FILE_ERROR,
                       picture.error_code,
                       _("WebP error: '%s'"),
                       webp_error_string (picture.error_code));
          break;
        }

      /* The cleanup stuff still needs to run but indicate that everything
       * completed successfully
       */
      status = TRUE;

    }
  while (0);

  /* Flush the drawable and detach */
  if (geglbuffer)
    {
      gegl_buffer_flush (geglbuffer);
      g_object_unref (geglbuffer);
    }

  fflush (outfile);
  fd_outfile = fileno (outfile);
  fstat (fd_outfile, &stsz);
  indatalen = stsz.st_size;
  if (indatalen > 0)
    {
      indata = (gchar*) g_malloc (indatalen);
      rewind (outfile);
      res = fread (indata, 1, indatalen, outfile);
      if (res > 0)
        {
          WebPMux *mux;
          WebPData wp_data;

          wp_data.bytes = (uint8_t*) indata;
          wp_data.size = indatalen;
          mux = WebPMuxCreate (&wp_data, 1);

          if (mux)
            {
              gboolean saved = FALSE;

              /* Save ICC data */
              profile = gimp_image_get_color_profile (image_ID);
              if (profile)
                {
                  const guint8 *icc_data;
                  gsize         icc_data_size;

                  saved = TRUE;

                  icc_data = gimp_color_profile_get_icc_profile (profile,
                                                                 &icc_data_size);
                  chunk.bytes = icc_data;
                  chunk.size = icc_data_size;
                  WebPMuxSetChunk(mux, "ICCP", &chunk, 1);
                  g_object_unref (profile);
                }

              if (saved == TRUE)
                {
                  WebPMuxAssemble (mux, &wp_data);
                  rewind (outfile);
                  webp_anim_file_writer (outfile, wp_data.bytes, wp_data.size);
                }
            }
          else
            {
              g_printerr ("ERROR: Cannot create mux. Can't save features update.\n");
            }

          WebPDataClear (&wp_data);
        }
      else
        {
          g_printerr ("ERROR: No data read for features. Can't save features update.\n");
        }
    }
  else
    {
      g_printerr ("ERROR: No data for features. Can't save features update.\n");
    }

  /* Free any resources */
  if (outfile)
    fclose (outfile);

  if (buffer)
    free (buffer);

  WebPPictureFree (&picture);

  return status;
}

gboolean
save_animation (const gchar    *filename,
                gint32          nLayers,
                gint32         *allLayers,
                gint32          image_ID,
                gint32          drawable_ID,
                WebPSaveParams *params,
                GError        **error)
{
  gboolean               status   = FALSE;
  FILE                  *outfile  = NULL;
  guchar                *buffer   = NULL;
  gint                   w, h, bpp;
  GimpImageType          drawable_type;
  GimpColorProfile      *profile;
  WebPAnimEncoderOptions enc_options;
  WebPData               webp_data;
  int                    frame_timestamp = 0;
  WebPAnimEncoder       *enc;
  WebPMux               *mux;
  WebPMuxAnimParams      anim_params = {0};

  if (nLayers < 1)
    return FALSE;

  do
    {
      gint loop;

      /* Begin displaying export progress */
      gimp_progress_init_printf (_("Saving '%s'"),
                                 gimp_filename_to_utf8 (filename));

      /* Attempt to open the output file */
      if ((outfile = g_fopen (filename, "wb")) == NULL)
        {
          g_set_error (error, G_FILE_ERROR,
                       g_file_error_from_errno (errno),
                       _("Unable to open '%s' for writing"),
                       gimp_filename_to_utf8 (filename));
          break;
        }

      WebPDataInit (&webp_data);
      if (! WebPAnimEncoderOptionsInit (&enc_options))
        {
          g_printerr ("ERROR: verion mismatch\n");
          break;
        }

      for (loop = 0; loop < nLayers; loop++)
        {
          GeglBuffer       *geglbuffer;
          GeglRectangle     extent;
          WebPConfig        config;
          WebPPicture       picture;
          WebPMemoryWriter  mw = { 0 };

          /* Obtain the drawable type */
          drawable_type = gimp_drawable_type (allLayers[loop]);

          /* Retrieve the buffer for the layer */
          geglbuffer = gimp_drawable_get_buffer (allLayers[loop]);
          extent = *gegl_buffer_get_extent (geglbuffer);
          bpp = gimp_drawable_bpp (allLayers[loop]);
          w = extent.width;
          h = extent.height;

          if (loop == 0)
            {
              enc = WebPAnimEncoderNew (w, h, &enc_options);
              if (! enc)
                {
                  g_printerr ("ERROR: enc == null\n");
                  break;
                }
            }

          /* Attempt to allocate a buffer of the appropriate size */
          buffer = (guchar *) g_malloc (w * h * bpp);
          if(! buffer)
            {
              g_printerr ("Buffer error: 'buffer null'\n");
              status = FALSE;
              break;
            }

          WebPConfigInit (&config);
          WebPConfigPreset (&config,
                            webp_preset_by_name (params->preset),
                            params->quality);

          config.lossless      = params->lossless;
          config.method        = 6;  /* better quality */
          config.alpha_quality = params->alpha_quality;
          config.exact         = 1;

          WebPMemoryWriterInit (&mw);

          /* Prepare the WebP structure */
          WebPPictureInit (&picture);
          picture.use_argb      = 1;
          picture.argb_stride   = w * bpp;
          picture.width         = w;
          picture.height        = h;
          picture.custom_ptr    = &mw;
          picture.writer        = WebPMemoryWrite;
          picture.progress_hook = webp_file_progress;

          /* Read the region into the buffer */
          gegl_buffer_get (geglbuffer, &extent, 1.0, NULL, buffer,
                           GEGL_AUTO_ROWSTRIDE, GEGL_ABYSS_NONE);

          /* Use the appropriate function to import the data from the buffer */
          if (drawable_type == GIMP_RGB_IMAGE)
            {
              WebPPictureImportRGB (&picture, buffer, w * bpp);
            }
          else
            {
              WebPPictureImportRGBA (&picture, buffer, w * bpp);
            }

          /* Perform the actual encode */
          if (! WebPAnimEncoderAdd (enc, &picture, frame_timestamp, &config))
            {
              g_printerr ("ERROR[%d]: %s\n",
                          picture.error_code,
                          webp_error_string (picture.error_code));
            }

          WebPMemoryWriterClear (&mw);
          WebPPictureFree (&picture);

          if (buffer)
            free (buffer);

          /* Flush the drawable and detach */
          gegl_buffer_flush (geglbuffer);
          g_object_unref (geglbuffer);
        }

      WebPAnimEncoderAdd (enc, NULL, frame_timestamp, NULL);

      if (! WebPAnimEncoderAssemble (enc, &webp_data))
        {
          g_printerr ("ERROR: %s\n",
                      WebPAnimEncoderGetError (enc));
        }

      /* Set animations parameters */
      mux = WebPMuxCreate (&webp_data, 1);

      anim_params.loop_count = 0;
      if (params->loop == FALSE)
        {
          anim_params.loop_count = 1;
        }

      WebPMuxSetAnimationParams (mux, &anim_params);

      /* Save ICC data */
      profile = gimp_image_get_color_profile (image_ID);
      if (profile)
        {
          WebPData      chunk;
          const guint8 *icc_data;
          gsize         icc_data_size;

          icc_data = gimp_color_profile_get_icc_profile (profile, &icc_data_size);
          chunk.bytes = icc_data;
          chunk.size  = icc_data_size;
          WebPMuxSetChunk (mux, "ICCP", &chunk, 1);
          g_object_unref (profile);
        }

      WebPMuxAssemble (mux, &webp_data);

      webp_anim_file_writer (outfile, webp_data.bytes, webp_data.size);

      WebPDataClear (&webp_data);
      WebPAnimEncoderDelete (enc);

      status = TRUE;
    }
  while (0);

  /* Free any resources */
  if (outfile)
    fclose (outfile);

  return status;
}


gboolean
save_image (const gchar    *filename,
            gint32          nLayers,
            gint32         *allLayers,
            gint32          image_ID,
            gint32          drawable_ID,
            WebPSaveParams *params,
            GError        **error)
{
  GimpMetadata          *metadata;
  GimpMetadataSaveFlags  metadata_flags;
  gboolean               status = FALSE;
  GFile                 *file;

  if (nLayers == 0)
    return FALSE;

  g_printerr("Saving WebP file %s\n", filename);

  if (nLayers == 1)
    {
      status = save_layer (filename, nLayers, image_ID, drawable_ID, params,
                           error);
    }
  else
    {
      if (! params->animation)
        {
          status = save_layer (filename,
                               nLayers, image_ID, drawable_ID, params,
                               error);
        }
      else
        {
          status = save_animation (filename,
                                   nLayers, allLayers, image_ID, drawable_ID,
                                   params, error);
        }
    }

  metadata = gimp_image_metadata_save_prepare (image_ID,
                                               "image/webp",
                                               &metadata_flags);

  if (metadata)
    {
      gimp_metadata_set_bits_per_sample (metadata, 8);
      file = g_file_new_for_path (filename);
      gimp_image_metadata_save_finish (image_ID,
                                       "image/webp",
                                       metadata, metadata_flags,
                                       file, NULL);
      g_object_unref (file);
    }

  /* Return the status */
  return status;
}
