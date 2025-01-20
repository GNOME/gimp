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

#include "file-webp-export.h"

#include "libgimp/stdplugins-intl.h"


int           webp_anim_file_writer (FILE              *outfile,
                                     const uint8_t     *data,
                                     size_t             data_size);
int           webp_file_writer      (const uint8_t     *data,
                                     size_t             data_size,
                                     const WebPPicture *picture);
int           webp_file_progress    (int                percent,
                                     const WebPPicture *picture);
gchar *       webp_error_string     (WebPEncodingError  error_code);

static void   webp_decide_output    (GimpImage         *image,
                                     GObject           *config,
                                     GimpColorProfile **profile,
                                     gboolean          *out_linear);

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

gchar *
webp_error_string (WebPEncodingError error_code)
{
  switch (error_code)
    {
    case VP8_ENC_ERROR_OUT_OF_MEMORY:
      return g_strdup (_("out of memory"));
    case VP8_ENC_ERROR_BITSTREAM_OUT_OF_MEMORY:
      return g_strdup (_("not enough memory to flush bits"));
    case VP8_ENC_ERROR_NULL_PARAMETER:
      return g_strdup (_("NULL parameter"));
    case VP8_ENC_ERROR_INVALID_CONFIGURATION:
      return g_strdup (_("invalid configuration"));
    case VP8_ENC_ERROR_BAD_DIMENSION:
      /* TRANSLATORS: widthxheight with UTF-8 encoded multiply sign. */
      return g_strdup_printf (_("bad image dimensions (maximum: %d\xc3\x97%d)"),
                              WEBP_MAX_DIMENSION, WEBP_MAX_DIMENSION);
    case VP8_ENC_ERROR_PARTITION0_OVERFLOW:
      return g_strdup (_("partition is bigger than 512K"));
    case VP8_ENC_ERROR_PARTITION_OVERFLOW:
      return g_strdup (_("partition is bigger than 16M"));
    case VP8_ENC_ERROR_BAD_WRITE:
      return g_strdup (_("unable to flush bytes"));
    case VP8_ENC_ERROR_FILE_TOO_BIG:
      return g_strdup (_("file is larger than 4GiB"));
    case VP8_ENC_ERROR_USER_ABORT:
      return g_strdup (_("user aborted encoding"));
    case VP8_ENC_ERROR_LAST:
      return g_strdup (_("list terminator"));
    default:
      return g_strdup (_("unknown error"));
    }
}

gboolean
save_layer (GFile         *file,
            GimpImage     *image,
            GimpDrawable  *drawable,
            GObject       *config,
            GError       **error)
{
  gboolean          status      = FALSE;
  FILE             *outfile     = NULL;
  WebPConfig        webp_config = { 0, };
  WebPPicture       picture     = { 0, };
  guchar           *buffer      = NULL;
  gint              w, h;
  gboolean          has_alpha;
  const gchar      *encoding;
  const Babl       *format;
  const Babl       *space      = NULL;
  gint              bpp;
  GimpColorProfile *profile    = NULL;
  GeglBuffer       *geglbuffer = NULL;
  GeglRectangle     extent;
  gchar            *indata;
  gsize             indatalen;
  struct            stat stsz;
  int               fd_outfile;
  WebPData          chunk;
  gboolean          out_linear = FALSE;
  int               res;
  WebPPreset        preset;
  gboolean          lossless;
  gdouble           quality;
  gdouble           alpha_quality;
  gboolean          use_sharp_yuv;

  g_object_get (config,
                "lossless",      &lossless,
                "quality",       &quality,
                "alpha-quality", &alpha_quality,
                "use-sharp-yuv", &use_sharp_yuv,
                NULL);
  preset = gimp_procedure_config_get_choice_id (GIMP_PROCEDURE_CONFIG (config),
                                                "preset");

  webp_decide_output (image, config, &profile, &out_linear);
  if (profile)
    {
      space = gimp_color_profile_get_space (profile,
                                            GIMP_COLOR_RENDERING_INTENT_RELATIVE_COLORIMETRIC,
                                            error);
      if (error && *error)
        {
          /* Don't make this a hard failure yet still output the error.
           */
          g_printerr ("%s: error getting the profile space: %s",
                      G_STRFUNC, (*error)->message);
          g_clear_error (error);
        }

    }
  if (! space)
    space = gimp_drawable_get_format (drawable);

  /* The do...while() loop is a neat little trick that makes it easier
   * to jump to error handling code while still ensuring proper
   * cleanup
   */

  do
    {
      /* Begin displaying export progress */
      gimp_progress_init_printf (_("Saving '%s'"),
                                 gimp_file_get_utf8_name (file));

      /* Attempt to open the output file */
      outfile = g_fopen (g_file_peek_path (file), "w+b");

      if (! outfile)
        {
          g_set_error (error, G_FILE_ERROR,
                       g_file_error_from_errno (errno),
                       _("Unable to open '%s' for writing: %s"),
                       gimp_file_get_utf8_name (file),
                       g_strerror (errno));
          break;
        }

      /* Obtain the drawable type */
      has_alpha = gimp_drawable_has_alpha (drawable);

      if (has_alpha)
        {
          if (out_linear)
            encoding = "RGBA u8";
          else
            encoding = "R'G'B'A u8";
        }
      else
        {
          if (out_linear)
            encoding = "RGB u8";
          else
            encoding = "R'G'B' u8";
        }

      format = babl_format_with_space (encoding, space);
      bpp = babl_format_get_bytes_per_pixel (format);

      /* Retrieve the buffer for the layer */
      geglbuffer = gimp_drawable_get_buffer (drawable);
      extent = *gegl_buffer_get_extent (geglbuffer);
      w = extent.width;
      h = extent.height;

      /* Initialize the WebP configuration with a preset and fill in the
       * remaining values */
      WebPConfigPreset (&webp_config, preset, quality);

      webp_config.lossless       = lossless;
      webp_config.method         = 6;  /* better quality */
      webp_config.alpha_quality  = alpha_quality;
      webp_config.use_sharp_yuv  = use_sharp_yuv ? 1 : 0;

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
          status = WebPPictureImportRGB (&picture, buffer, w * bpp);
        }
      else
        {
          status = WebPPictureImportRGBA (&picture, buffer, w * bpp);
        }

      g_free (buffer);
      if (! status)
        {
          g_printerr ("%s: memory error in WebPPictureImportRGB(A)().",
                      G_STRFUNC);
          break;
        }

      /* Perform the actual encode */
      if (! WebPEncode (&webp_config, &picture))
        {
          gchar *error_str = webp_error_string (picture.error_code);

          g_printerr ("WebP error: '%s'", error_str);
          g_set_error (error, G_FILE_ERROR,
                       picture.error_code,
                       _("WebP error: '%s'"),
                       error_str);
          g_free (error_str);
          status = FALSE;
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
              /* Save ICC data */
              if (profile)
                {
                  const guint8 *icc_data;
                  gsize         icc_data_size;

                  icc_data = gimp_color_profile_get_icc_profile (profile,
                                                                 &icc_data_size);
                  chunk.bytes = icc_data;
                  chunk.size = icc_data_size;
                  WebPMuxSetChunk(mux, "ICCP", &chunk, 1);

                  WebPMuxAssemble (mux, &wp_data);
                  rewind (outfile);
                  webp_anim_file_writer (outfile, wp_data.bytes, wp_data.size);
                }

              WebPMuxDelete (mux);
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
  g_clear_object (&profile);

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
get_layer_delay (GimpLayer *layer)
{
  gchar *layer_name;
  gint   delay_ms;

  layer_name = gimp_item_get_name (GIMP_ITEM (layer));
  delay_ms   = parse_ms_tag (layer_name);
  g_free (layer_name);

  return delay_ms;
}

static gboolean
parse_combine (const char* str)
{
  gint offset = 0;
  gint length = strlen (str);

  while ((offset + 9) <= length)
    {
      if (strncmp (&str[offset], "(combine)", 9) == 0)
        return TRUE;

      if (strncmp (&str[offset], "(replace)", 9) == 0)
        return FALSE;

      offset++;
    }

  return FALSE;
}

static gboolean
get_layer_needs_combine (GimpLayer *layer)
{
  gchar     *layer_name;
  gboolean   needs_combine;

  layer_name    = gimp_item_get_name (GIMP_ITEM (layer));
  needs_combine = parse_combine (layer_name);
  g_free (layer_name);

  return needs_combine;
}

static GeglBuffer *
combine_buffers (GeglBuffer *layer_buffer,
                 GeglBuffer *prev_frame_buffer)
{
  GeglBuffer *buffer;
  GeglNode   *graph;
  GeglNode   *source;
  GeglNode   *backdrop;
  GeglNode   *over;
  GeglNode   *target;

  graph  = gegl_node_new ();
  buffer = gegl_buffer_new (gegl_buffer_get_extent (prev_frame_buffer),
                            gegl_buffer_get_format (prev_frame_buffer));

  source = gegl_node_new_child (graph,
                                "operation", "gegl:buffer-source",
                                "buffer", layer_buffer,
                                NULL);
  backdrop = gegl_node_new_child (graph,
                                  "operation", "gegl:buffer-source",
                                  "buffer", prev_frame_buffer,
                                  NULL);

  over =  gegl_node_new_child (graph,
                               "operation", "gegl:over",
                                NULL);
  target = gegl_node_new_child (graph,
                                "operation", "gegl:write-buffer",
                                "buffer", buffer,
                                NULL);
  gegl_node_link_many (backdrop, over, target, NULL);
  gegl_node_connect (source, "output", over, "aux");
  gegl_node_process (target);
  g_object_unref (graph);

  return buffer;
}

gboolean
save_animation (GFile         *file,
                GimpImage     *image,
                gint           n_drawables,
                GList         *drawables,
                GObject       *config,
                GError       **error)
{
  GList                 *layers;
  gint32                 n_layers;
  gboolean               status      = TRUE;
  FILE                  *outfile     = NULL;
  guchar                *buffer      = NULL;
  gint                   buffer_size = 0;
  gint                   w, h;
  gint                   bpp;
  gboolean               has_alpha;
  const gchar           *encoding;
  const Babl            *format;
  const Babl            *space   = NULL;
  GimpColorProfile      *profile = NULL;
  WebPAnimEncoderOptions enc_options;
  WebPData               webp_data;
  int                    frame_timestamp = 0;
  WebPAnimEncoder       *enc             = NULL;
  GeglBuffer            *prev_frame      = NULL;
  gboolean               out_linear      = FALSE;
  WebPPreset             preset;
  gboolean               lossless;
  gboolean               animation;
  gboolean               loop;
  gboolean               minimize_size;
  gint                   keyframe_distance;
  gdouble                quality;
  gdouble                alpha_quality;
  gint                   default_delay;
  gboolean               force_delay;
  gboolean               use_sharp_yuv;

  g_return_val_if_fail (n_drawables > 0, FALSE);

  g_object_get (config,
                "lossless",          &lossless,
                "animation",         &animation,
                "animation-loop",    &loop,
                "minimize-size",     &minimize_size,
                "keyframe-distance", &keyframe_distance,
                "quality",           &quality,
                "alpha-quality",     &alpha_quality,
                "default-delay",     &default_delay,
                "force-delay",       &force_delay,
                "use-sharp-yuv",     &use_sharp_yuv,
                NULL);
  preset = gimp_procedure_config_get_choice_id (GIMP_PROCEDURE_CONFIG (config),
                                                "preset");

  layers = gimp_image_list_layers (image);

  if (! layers)
    return FALSE;

  layers   = g_list_reverse (layers);
  n_layers = g_list_length (layers);

  webp_decide_output (image, config, &profile, &out_linear);
  if (profile)
    {
      space = gimp_color_profile_get_space (profile,
                                            GIMP_COLOR_RENDERING_INTENT_RELATIVE_COLORIMETRIC,
                                            error);
      if (error && *error)
        {
          /* Don't make this a hard failure yet still output the error.
           */
          g_printerr ("%s: error getting the profile space: %s",
                      G_STRFUNC, (*error)->message);
          g_clear_error (error);
        }

    }

  if (! space)
    space = gimp_drawable_get_format (drawables->data);

  gimp_image_undo_freeze (image);

  WebPDataInit (&webp_data);

  do
    {
      GList *list;
      gint   i;

      /* Begin displaying export progress */
      gimp_progress_init_printf (_("Saving '%s'"),
                                 gimp_file_get_utf8_name (file));

      /* Attempt to open the output file */
      outfile = g_fopen (g_file_peek_path (file), "wb");

      if (! outfile)
        {
          g_set_error (error, G_FILE_ERROR,
                       g_file_error_from_errno (errno),
                       _("Unable to open '%s' for writing: %s"),
                       gimp_file_get_utf8_name (file),
                       g_strerror (errno));
          status = FALSE;
          break;
        }

      if (! WebPAnimEncoderOptionsInit (&enc_options))
        {
          g_printerr ("ERROR: version mismatch\n");
          status = FALSE;
          break;
        }

      enc_options.anim_params.loop_count = 0;
      if (! loop)
        enc_options.anim_params.loop_count = 1;

      enc_options.allow_mixed   = lossless      ? 0 : 1;
      enc_options.minimize_size = minimize_size ? 1 : 0;
      if (! minimize_size)
        {
          enc_options.kmax = keyframe_distance;
          /* explicitly force minimum key-frame distance too, for good measure */
          enc_options.kmin = keyframe_distance - 1;
        }

      for (list = layers, i = 0;
           list;
           list = g_list_next (list), i++)
        {
          GeglBuffer       *geglbuffer;
          GeglBuffer       *current_frame;
          GeglRectangle     extent;
          WebPConfig        webp_config;
          WebPPicture       picture;
          WebPMemoryWriter  mw       = { 0 };
          GimpDrawable     *drawable = list->data;
          gint              delay;
          gboolean          needs_combine;

          delay         = get_layer_delay (GIMP_LAYER (drawable));
          needs_combine = get_layer_needs_combine (GIMP_LAYER (drawable));

          /* Obtain the drawable type */
          has_alpha = gimp_drawable_has_alpha (drawable);

          if (has_alpha)
            {
              if (out_linear)
                encoding = "RGBA u8";
              else
                encoding = "R'G'B'A u8";
            }
          else
            {
              if (out_linear)
                encoding = "RGB u8";
              else
                encoding = "R'G'B' u8";
            }

          format = babl_format_with_space (encoding, space);
          bpp = babl_format_get_bytes_per_pixel (format);

          /* fix layers to avoid offset errors */
          gimp_layer_resize_to_image_size (GIMP_LAYER (drawable));

          /* Retrieve the buffer for the layer */
          geglbuffer = gimp_drawable_get_buffer (drawable);
          extent = *gegl_buffer_get_extent (geglbuffer);
          w = extent.width;
          h = extent.height;

          if (i == 0)
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
          if (! buffer || buffer_size < w * h * bpp)
            {
              buffer = g_try_realloc (buffer, w * h * bpp);

              if (! buffer)
                {
                  g_printerr ("Buffer error: 'buffer null'\n");
                  status = FALSE;
                  break;
                }
              else
                {
                  buffer_size = w * h * bpp;
                }
            }

          WebPConfigPreset (&webp_config, preset, quality);

          webp_config.lossless      = lossless;
          webp_config.method        = 6;  /* better quality */
          webp_config.alpha_quality = alpha_quality;
          webp_config.exact         = 1;
          webp_config.use_sharp_yuv = use_sharp_yuv ? 1 : 0;

          WebPMemoryWriterInit (&mw);

          /* Prepare the WebP structure */
          WebPPictureInit (&picture);
          picture.use_argb      = 1;
          picture.argb_stride   = w * bpp;
          picture.width         = w;
          picture.height        = h;
          picture.custom_ptr    = &mw;
          picture.writer        = WebPMemoryWrite;

          if (i == 0 || ! needs_combine)
            {
              g_clear_object (&prev_frame);
              current_frame = geglbuffer;
            }
          else
            {
              current_frame = combine_buffers (geglbuffer, prev_frame);

              /* release resources. */
              g_object_unref (geglbuffer);
              g_clear_object (&prev_frame);
            }
          prev_frame = current_frame;

          /* Read the region into the buffer */
          gegl_buffer_get (current_frame, &extent, 1.0, format, buffer,
                           GEGL_AUTO_ROWSTRIDE, GEGL_ABYSS_NONE);

          /* Use the appropriate function to import the data from the buffer */
          if (! has_alpha)
            {
              status = WebPPictureImportRGB (&picture, buffer, w * bpp);
            }
          else
            {
              status = WebPPictureImportRGBA (&picture, buffer, w * bpp);
            }

          if (! status)
            {
              g_printerr ("%s: memory error in WebPPictureImportRGB(A)().",
                          G_STRFUNC);
            }
          /* Perform the actual encode */
          else if (! WebPAnimEncoderAdd (enc, &picture, frame_timestamp,
                                         &webp_config))
            {
              gchar *error_str = webp_error_string (picture.error_code);
              g_printerr ("ERROR[%d]: line %d: %s\n",
                          picture.error_code, __LINE__,
                          error_str);
              g_free (error_str);
              status = FALSE;
            }

          WebPMemoryWriterClear (&mw);
          WebPPictureFree (&picture);

          if (status == FALSE)
            break;

          gimp_progress_update ((i + 1.0) / n_layers);
          frame_timestamp += (delay <= 0 || force_delay) ? default_delay : delay;
        }

      g_free (buffer);

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
  g_clear_object (&profile);

  if (prev_frame != NULL)
    {
      g_object_unref (prev_frame);
    }

  if (outfile)
    fclose (outfile);

  g_list_free (layers);

  return status;
}

static void
webp_decide_output (GimpImage         *image,
                    GObject           *config,
                    GimpColorProfile **profile,
                    gboolean          *out_linear)
{
  gboolean save_profile;

  g_return_if_fail (profile && *profile == NULL);

  g_object_get (config,
                "include-color-profile", &save_profile,
                NULL);

  *out_linear = FALSE;

  if (save_profile)
    {
      *profile = gimp_image_get_color_profile (image);

      /* If a profile is explicitly set, follow its TRC, whatever the
       * storage format.
       */
      if (*profile && gimp_color_profile_is_linear (*profile))
        *out_linear = TRUE;

      /* When no profile was explicitly set, since WebP is apparently
       * 8-bit max, we export it as sRGB to avoid shadow posterization
       * (we don't care about storage TRC).
       * We do an exception for 8-bit linear work image to avoid
       * conversion loss while the precision is the same.
       */
      if (! *profile)
        {
          /* There is always an effective profile. */
          *profile = gimp_image_get_effective_color_profile (image);

          if (gimp_color_profile_is_linear (*profile))
            {
              if (gimp_image_get_precision (image) != GIMP_PRECISION_U8_LINEAR)
                {
                  /* If stored data was linear, let's convert the profile. */
                  GimpColorProfile *saved_profile;

                  saved_profile = gimp_color_profile_new_srgb_trc_from_color_profile (*profile);
                  g_clear_object (profile);
                  *profile = saved_profile;
                }
              else
                {
                  /* Keep linear profile as-is for 8-bit linear image. */
                  *out_linear = TRUE;
                }
            }
        }
    }
}
