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

#include <webp/mux.h>

#include "file-webp-save.h"

#include "libgimp/stdplugins-intl.h"


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
  gboolean          has_alpha;
  const Babl       *format;
  gint              bpp;
  GimpColorProfile *profile;
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
      has_alpha = gimp_drawable_has_alpha (drawable_ID);

      if (has_alpha)
        format = babl_format ("R'G'B'A u8");
      else
        format = babl_format ("R'G'B' u8");

      bpp = babl_format_get_bytes_per_pixel (format);

      /* Retrieve the buffer for the layer */
      geglbuffer = gimp_drawable_get_buffer (drawable_ID);
      extent = *gegl_buffer_get_extent (geglbuffer);
      w = extent.width;
      h = extent.height;

      /* Initialize the WebP configuration with a preset and fill in the
       * remaining values */
      WebPConfigPreset (&config, params->preset, params->quality);

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
      buffer = g_try_malloc (w * h * bpp);
      if (! buffer)
        break;

      /* Read the region into the buffer */
      gegl_buffer_get (geglbuffer, &extent, 1.0, format, buffer,
                       GEGL_AUTO_ROWSTRIDE, GEGL_ABYSS_NONE);

      /* Use the appropriate function to import the data from the buffer */
      if (! has_alpha)
        {
          WebPPictureImportRGB (&picture, buffer, w * bpp);
        }
      else
        {
          WebPPictureImportRGBA (&picture, buffer, w * bpp);
        }

      g_free (buffer);

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
                  WebPMuxSetChunk (mux, "ICCP", &chunk, 1);
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

  WebPPictureFree (&picture);

  return status;
}

static gint
parse_ms_tag (const gchar *str)
{
  gint sum    = 0;
  gint offset = 0;
  gint length;

  length = strlen (str);

 find_another_bra:

  while ((offset < length) && (str[offset] != '('))
    offset++;

  if (offset >= length)
    return -1;

  if (! g_ascii_isdigit (str[++offset]))
    goto find_another_bra;

  do
    {
      sum *= 10;
      sum += str[offset] - '0';
      offset++;
    }
  while ((offset < length) && (g_ascii_isdigit (str[offset])));

  if (length - offset <= 2)
    return -3;

  if ((g_ascii_toupper (str[offset])     != 'M') ||
      (g_ascii_toupper (str[offset + 1]) != 'S'))
    return -4;

  return sum;
}

static gint
get_layer_delay (gint32 layer)
{
  gchar *layer_name;
  gint   delay_ms;

  layer_name = gimp_item_get_name (layer);
  delay_ms   = parse_ms_tag (layer_name);
  g_free (layer_name);

  return delay_ms;
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
  gboolean               status   = TRUE;
  FILE                  *outfile  = NULL;
  guchar                *buffer   = NULL;
  gint                   w, h;
  gint                   bpp;
  gboolean               has_alpha;
  const Babl            *format;
  GimpColorProfile      *profile;
  WebPAnimEncoderOptions enc_options;
  WebPData               webp_data;
  int                    frame_timestamp = 0;
  WebPAnimEncoder       *enc = NULL;

  if (nLayers < 1)
    return FALSE;

  gimp_image_undo_freeze (image_ID);

  WebPDataInit (&webp_data);

  do
    {
      gint     loop;
      gint     default_delay = params->delay;
      gboolean force_delay = params->force_delay;

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
          status = FALSE;
          break;
        }

      if (! WebPAnimEncoderOptionsInit (&enc_options))
        {
          g_printerr ("ERROR: verion mismatch\n");
          status = FALSE;
          break;
        }

      enc_options.anim_params.loop_count = 0;
      if (! params->loop)
        enc_options.anim_params.loop_count = 1;

      enc_options.allow_mixed   = params->lossless ? 0 : 1;
      enc_options.minimize_size = 1;

      for (loop = 0; loop < nLayers; loop++)
        {
          GeglBuffer       *geglbuffer;
          GeglRectangle     extent;
          WebPConfig        config;
          WebPPicture       picture;
          WebPMemoryWriter  mw = { 0 };
          gint32            drawable = allLayers[nLayers - 1 - loop];
          gint              delay = get_layer_delay (drawable);

          /* Obtain the drawable type */
          has_alpha = gimp_drawable_has_alpha (drawable);

          if (has_alpha)
            format = babl_format ("R'G'B'A u8");
          else
            format = babl_format ("R'G'B' u8");

          bpp = babl_format_get_bytes_per_pixel (format);

          /* fix layers to avoid offset errors */
          gimp_layer_resize_to_image_size (drawable);

          /* Retrieve the buffer for the layer */
          geglbuffer = gimp_drawable_get_buffer (drawable);
          extent = *gegl_buffer_get_extent (geglbuffer);
          w = extent.width;
          h = extent.height;

          if (loop == 0)
            {
              enc = WebPAnimEncoderNew (w, h, &enc_options);
              if (! enc)
                {
                  g_printerr ("ERROR: enc == null\n");
                  status = FALSE;
                  break;
                }
            }

          /* Attempt to allocate a buffer of the appropriate size */
          buffer = g_try_malloc (w * h * bpp);
          if (! buffer)
            {
              g_printerr ("Buffer error: 'buffer null'\n");
              status = FALSE;
              break;
            }

          WebPConfigPreset (&config, params->preset, params->quality);

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

          /* Read the region into the buffer */
          gegl_buffer_get (geglbuffer, &extent, 1.0, format, buffer,
                           GEGL_AUTO_ROWSTRIDE, GEGL_ABYSS_NONE);

          /* Use the appropriate function to import the data from the buffer */
          if (! has_alpha)
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
              status = FALSE;
            }

          WebPMemoryWriterClear (&mw);
          WebPPictureFree (&picture);

          if (buffer)
            g_free (buffer);

          if (status == FALSE)
            break;

          /* Flush the drawable and detach */
          gegl_buffer_flush (geglbuffer);
          g_object_unref (geglbuffer);

          gimp_progress_update ((loop + 1.0) / nLayers);
          frame_timestamp += (delay <= 0 || force_delay) ? default_delay : delay;
        }

      if (status == FALSE)
        break;

      WebPAnimEncoderAdd (enc, NULL, frame_timestamp, NULL);

      if (! WebPAnimEncoderAssemble (enc, &webp_data))
        {
          g_printerr ("ERROR: %s\n",
                      WebPAnimEncoderGetError (enc));
          status = FALSE;
          break;
        }

      /* Create a mux object if profile is present */
      profile = gimp_image_get_color_profile (image_ID);
      if (profile)
        {
          WebPMux      *mux;
          WebPData      chunk;
          const guint8 *icc_data;
          gsize         icc_data_size;

          mux = WebPMuxCreate (&webp_data, 1);
          if (mux == NULL)
            {
               g_printerr ("ERROR: could not extract muxing object\n");
               status = FALSE;
               break;
            }

          /* Save ICC data */
          icc_data = gimp_color_profile_get_icc_profile (profile, &icc_data_size);
          chunk.bytes = icc_data;
          chunk.size  = icc_data_size;
          WebPMuxSetChunk (mux, "ICCP", &chunk, 1);
          g_object_unref (profile);

          WebPDataClear (&webp_data);
          if (WebPMuxAssemble (mux, &webp_data) != WEBP_MUX_OK)
            {
              g_printerr ("ERROR: could not assemble final bytestream\n");
              status = FALSE;
              break;
            }
        }

      webp_anim_file_writer (outfile, webp_data.bytes, webp_data.size);
    }
  while (0);

  /* Free any resources */
  WebPDataClear (&webp_data);
  WebPAnimEncoderDelete (enc);

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

  g_printerr ("Saving WebP file %s\n", filename);

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

      if (params->exif)
        metadata_flags |= GIMP_METADATA_SAVE_EXIF;
      else
        metadata_flags &= ~GIMP_METADATA_SAVE_EXIF;

      /* WebP doesn't support iptc natively and
         sets it via xmp */
      if (params->xmp)
        {
          metadata_flags |= GIMP_METADATA_SAVE_XMP;
          metadata_flags |= GIMP_METADATA_SAVE_IPTC;
        }
      else
        {
          metadata_flags &= ~GIMP_METADATA_SAVE_XMP;
          metadata_flags &= ~GIMP_METADATA_SAVE_IPTC;
        }

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
