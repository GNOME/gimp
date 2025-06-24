/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
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

#include <cairo.h>
#include <gegl.h>
#include <gdk-pixbuf/gdk-pixbuf.h>

#include "libgimpbase/gimpbase.h"
#include "libgimpcolor/gimpcolor.h"

#include "core-types.h"

#include "gimppattern.h"
#include "gimppattern-header.h"
#include "gimppattern-load.h"
#include "gimptempbuf.h"
#include "gimp-utils.h"

#include "gimp-intl.h"


static gboolean gimp_pattern_load_photoshop_channel  (GDataInputStream  *data_input,
                                                      gint               chan,
                                                      guchar            *chan_buf,
                                                      gsize              size,
                                                      guchar            *chan_data,
                                                      guint32            n_channels,
                                                      GError           **error);

static GList * gimp_pattern_load_photoshop_pattern   (GimpContext       *context,
                                                      GFile             *file,
                                                      GInputStream      *input,
                                                      GError           **error);


GList *
gimp_pattern_load (GimpContext   *context,
                   GFile         *file,
                   GInputStream  *input,
                   GError       **error)
{
  GimpPattern       *pattern = NULL;
  const Babl        *format  = NULL;
  GimpPatternHeader  header;
  gsize              size;
  gsize              bytes_read;
  gsize              bn_size;
  gchar             *name = NULL;

  g_return_val_if_fail (G_IS_FILE (file), NULL);
  g_return_val_if_fail (G_IS_INPUT_STREAM (input), NULL);
  g_return_val_if_fail (error == NULL || *error == NULL, NULL);

  /*  read the size  */
  if (! g_input_stream_read_all (input, &header, sizeof (header),
                                 &bytes_read, NULL, error) ||
      bytes_read != sizeof (header))
    {
      g_prefix_error (error, _("File appears truncated: "));
      goto error;
    }

  /* Check whether it is a Photoshop pattern */
  if (memcmp (&header.header_size, "8BPT", 4) == 0)
    {
      GList *ps_patterns = NULL;

      ps_patterns = gimp_pattern_load_photoshop_pattern (context, file, input, error);

      return ps_patterns;
    }

  /*  rearrange the bytes in each unsigned int  */
  header.header_size  = g_ntohl (header.header_size);
  header.version      = g_ntohl (header.version);
  header.width        = g_ntohl (header.width);
  header.height       = g_ntohl (header.height);
  header.bytes        = g_ntohl (header.bytes);
  header.magic_number = g_ntohl (header.magic_number);

  /*  Check for correct file format */
  if (header.magic_number != GIMP_PATTERN_MAGIC ||
      header.version      != 1                  ||
      header.header_size  <= sizeof (header))
    {
      g_set_error (error, GIMP_DATA_ERROR, GIMP_DATA_ERROR_READ,
                   _("Unknown pattern format version %d."),
                   header.version);
      goto error;
    }

  /*  Check for supported bit depths  */
  if (header.bytes < 1 || header.bytes > 4)
    {
      g_set_error (error, GIMP_DATA_ERROR, GIMP_DATA_ERROR_READ,
                   _("Unsupported pattern depth %d.\n"
                     "GIMP Patterns must be GRAY or RGB."),
                   header.bytes);
      goto error;
    }

  /*  Validate dimensions  */
  if ((header.width  == 0) || (header.width  > GIMP_PATTERN_MAX_SIZE) ||
      (header.height == 0) || (header.height > GIMP_PATTERN_MAX_SIZE) ||
      (G_MAXSIZE / header.width / header.height / header.bytes < 1))
    {
      g_set_error (error, GIMP_DATA_ERROR, GIMP_DATA_ERROR_READ,
                   _("Invalid header data in '%s': width=%lu (maximum %lu), "
                     "height=%lu (maximum %lu), bytes=%lu"),
                   gimp_file_get_utf8_name (file),
                   (gulong) header.width,  (gulong) GIMP_PATTERN_MAX_SIZE,
                   (gulong) header.height, (gulong) GIMP_PATTERN_MAX_SIZE,
                   (gulong) header.bytes);
      goto error;
    }

  /*  Read in the pattern name  */
  if ((bn_size = (header.header_size - sizeof (header))))
    {
      gchar *utf8;

      if (bn_size > GIMP_PATTERN_MAX_NAME)
        {
          g_set_error (error, GIMP_DATA_ERROR, GIMP_DATA_ERROR_READ,
                       _("Invalid header data in '%s': "
                         "Pattern name is too long: %lu"),
                       gimp_file_get_utf8_name (file),
                       (gulong) bn_size);
          goto error;
        }

      name = g_new0 (gchar, bn_size + 1);

      if (! g_input_stream_read_all (input, name, bn_size,
                                     &bytes_read, NULL, error) ||
          bytes_read != bn_size)
        {
          g_prefix_error (error, _("File appears truncated."));
          g_free (name);
          goto error;
        }

      utf8 = gimp_any_to_utf8 (name, bn_size - 1,
                               _("Invalid UTF-8 string in pattern file '%s'."),
                               gimp_file_get_utf8_name (file));
      g_free (name);
      name = utf8;
    }

  if (! name)
    name = g_strdup (_("Unnamed"));

  pattern = g_object_new (GIMP_TYPE_PATTERN,
                          "name",      name,
                          "mime-type", "image/x-gimp-pat",
                          NULL);

  g_free (name);

  switch (header.bytes)
    {
    case 1: format = babl_format ("Y' u8");      break;
    case 2: format = babl_format ("Y'A u8");     break;
    case 3: format = babl_format ("R'G'B' u8");  break;
    case 4: format = babl_format ("R'G'B'A u8"); break;
    }

  pattern->mask = gimp_temp_buf_new (header.width, header.height, format);
  size = (gsize) header.width * header.height * header.bytes;

  if (! g_input_stream_read_all (input,
                                 gimp_temp_buf_get_data (pattern->mask), size,
                                 &bytes_read, NULL, error) ||
      bytes_read != size)
    {
      g_prefix_error (error, _("File appears truncated."));
      goto error;
    }

  return g_list_prepend (NULL, pattern);

 error:

  if (pattern)
    g_object_unref (pattern);

  g_prefix_error (error, _("Fatal parse error in pattern file: "));

  return NULL;
}

GList *
gimp_pattern_load_pixbuf (GimpContext   *context,
                          GFile         *file,
                          GInputStream  *input,
                          GError       **error)
{
  GimpPattern *pattern;
  GdkPixbuf   *pixbuf;
  gchar       *name;

  g_return_val_if_fail (G_IS_FILE (file), NULL);
  g_return_val_if_fail (G_IS_INPUT_STREAM (input), NULL);
  g_return_val_if_fail (error == NULL || *error == NULL, NULL);

  pixbuf = gdk_pixbuf_new_from_stream (input, NULL, error);
  if (! pixbuf)
    return NULL;

  name = g_strdup (gdk_pixbuf_get_option (pixbuf, "tEXt::Title"));

  if (! name)
    name = g_strdup (gdk_pixbuf_get_option (pixbuf, "tEXt::Comment"));

  if (! name)
    name = g_path_get_basename (gimp_file_get_utf8_name (file));

  pattern = g_object_new (GIMP_TYPE_PATTERN,
                          "name",      name,
                          "mime-type", NULL, /* FIXME!! */
                          NULL);
  g_free (name);

  pattern->mask = gimp_temp_buf_new_from_pixbuf (pixbuf, NULL);

  g_object_unref (pixbuf);

  return g_list_prepend (NULL, pattern);
}


/* Private functions */
#define update_last_error() if (last_error) g_clear_error (last_error); last_error = error; error = NULL;

static gboolean
gimp_pattern_load_photoshop_channel (GDataInputStream  *data_input,
                                     gint               chan,
                                     guchar            *chan_buf,
                                     gsize              size,
                                     guchar            *chan_data,
                                     guint32            n_channels,
                                     GError           **error)
{
  gint      j;
  goffset   cofs;
  gint32    chan_version;
  gint32    chan_size;
  gint32    chan_dummy;
  gint16    chan_depth;
  gint8     chan_compression;
  gint32    chan_top, chan_left, chan_bottom, chan_right;
  guchar   *temp_buf;
  gsize     bytes_read;
  GError  **last_error = NULL;

  temp_buf = NULL;

  if (! gimp_data_input_stream_read_long (data_input,  &chan_version, error) ||
      ! gimp_data_input_stream_read_long (data_input,  &chan_size,    error))
    {
      g_printerr ("Error reading channel %d\n", chan);
      update_last_error ();
      return FALSE;
    }
  cofs = g_seekable_tell (G_SEEKABLE (data_input));

  /** Dummy 32-bit depth */
  if (! gimp_data_input_stream_read_long (data_input,  &chan_dummy, error))
    {
      g_printerr ("Error reading channel %d\n", chan);
      update_last_error ();
      return FALSE;
    }

  if (! gimp_data_input_stream_read_long (data_input,  &chan_top,    error) ||
      ! gimp_data_input_stream_read_long (data_input,  &chan_left,   error) ||
      ! gimp_data_input_stream_read_long (data_input,  &chan_bottom, error) ||
      ! gimp_data_input_stream_read_long (data_input,  &chan_right,  error) ||
      ! gimp_data_input_stream_read_short (data_input, &chan_depth,  error) ||
      ! gimp_data_input_stream_read_char (data_input,  (gchar *) &chan_compression, error))
    {
      g_printerr ("Error reading channel %d\n", chan);
      update_last_error ();
      return FALSE;
    }
  if (chan_depth != 8)
    {
      g_printerr ("Unsupported channel depth %d\n", chan_depth);
      update_last_error ();
      return FALSE;
    }

  if (chan_compression == 0)
    {
      if (! g_input_stream_read_all ((GInputStream *) data_input,
                                     chan_buf, size,
                                     &bytes_read, NULL, error) ||
          bytes_read != size)
        {
          g_printerr ("Error reading channel %d\n", chan);
          update_last_error ();
          return FALSE;
        }
    }
  else
    {
      if (! gimp_data_input_stream_rle_decode (data_input,
                                               (gchar *) chan_buf, size,
                                               chan_bottom-chan_top, error))
        {
          g_printerr ("Failed to decode channel %d\n", chan);
          update_last_error ();
          return FALSE;
        }
    }

  /* Move channel data to correct offset */
  temp_buf = &chan_data[chan];
  for (j = 0; j < size; j++)
    {
      temp_buf[j*n_channels] = chan_buf[j];
    }

  /* Always seek to next offset to reduce chance of errors */
  if (! g_seekable_seek (G_SEEKABLE (data_input),
                         cofs+chan_size, G_SEEK_SET,
                         NULL, error))
    {
      g_printerr ("Error reading Photoshop pattern.\n");
      update_last_error ();
      return FALSE;
    }
  return TRUE;
}

static GList *
gimp_pattern_load_photoshop_pattern (GimpContext   *context,
                                     GFile         *file,
                                     GInputStream  *input,
                                     GError       **error)
{
  gint16             version;
  gint32             pat_count;
  gint               i;
  goffset            ofs;
  GList             *pattern_list     = NULL;
  GDataInputStream  *data_input;
  GError           **last_error       = NULL;
  gboolean           show_unsupported = TRUE;

  data_input = g_data_input_stream_new (input);

  /* Photoshop data is big endian */
  g_data_input_stream_set_byte_order (data_input,
                                      G_DATA_STREAM_BYTE_ORDER_BIG_ENDIAN);

  /* Reset stream position to right after magic (offset 4) */
  if (! g_seekable_seek (G_SEEKABLE (input),
                         4, G_SEEK_SET,
                         NULL, error))
    {
      g_prefix_error (error, _("Error reading Photoshop pattern."));
      return NULL;
    }

  if (! gimp_data_input_stream_read_short (data_input, &version, error) ||
      ! gimp_data_input_stream_read_long (data_input, &pat_count, error))
    {
      g_prefix_error (error, _("Error reading Photoshop pattern."));
      return NULL;
    }

  g_debug ("\nDetected Photoshop pattern file. Version: %u, number of patterns: %u",
           version, pat_count);

  for (i = 0; i < pat_count; i++)
    {
      gint32    pat_version;
      gint32    pat_image_type;
      gint16    pat_height;
      gint16    pat_width;
      gchar    *pat_name;
      gint32    pat_size;
      gint32    pat_top, pat_left, pat_bottom, pat_right;
      gint32    pat_depth;
      guint32   n_channels;
      gint32    width, height;
      gboolean  can_handle;

      can_handle = FALSE;
      pat_name   = NULL;
      n_channels = 0;

      if (! gimp_data_input_stream_read_long (data_input,  &pat_version,    error) ||
          ! gimp_data_input_stream_read_long (data_input,  &pat_image_type, error) ||
          ! gimp_data_input_stream_read_short (data_input, &pat_height,     error) ||
          ! gimp_data_input_stream_read_short (data_input, &pat_width,      error) ||
          ! gimp_data_input_stream_read_ucs2_text (data_input, &pat_name,   error))
        {
          g_printerr ("Error reading Photoshop pattern %d.\n", i);
          update_last_error ();
          break;
        }
      g_debug ("Pattern version %u, image type: %u, width x height: %u x %u, name: %s",
               pat_version, pat_image_type, pat_width, pat_height, pat_name);

      /* For now seek past pattern id, ideally we would read the length to make
       * sure the size is as expected (length byte that usually says 36) */
      if (! g_seekable_seek (G_SEEKABLE (input),
                              37, G_SEEK_CUR,
                              NULL, error))
        {
          g_printerr ("Error reading Photoshop pattern %d.\n", i);
          update_last_error ();
          break;
        }

      /* FIXME: Support for reading the palette if this is indexed */
      if (pat_image_type == 2)
        {
          if (show_unsupported)
            {
              g_printerr ("Indexed Photoshop pattern is not yet supported.\n");
              show_unsupported = FALSE;
            }
          break;
        }

      if (! gimp_data_input_stream_read_long (data_input,  &pat_version,    error) ||
          ! gimp_data_input_stream_read_long (data_input,  &pat_size, error))
        {
          g_printerr ("Error reading Photoshop pattern %d.\n", i);
          update_last_error ();
          break;
        }
      ofs = g_seekable_tell (G_SEEKABLE (input));

      g_debug ("Pattern version %u, size: %u, offset: %" G_GOFFSET_FORMAT ", next %" G_GOFFSET_FORMAT,
               pat_version, pat_size, ofs, ofs + pat_size);

      if (! gimp_data_input_stream_read_long (data_input,  &pat_top,    error) ||
          ! gimp_data_input_stream_read_long (data_input,  &pat_left,   error) ||
          ! gimp_data_input_stream_read_long (data_input,  &pat_bottom, error) ||
          ! gimp_data_input_stream_read_long (data_input,  &pat_right,  error) ||
          ! gimp_data_input_stream_read_long (data_input,  &pat_depth,  error))
        {
          g_printerr ("Error reading Photoshop pattern %d.\n", i);
          update_last_error ();
          break;
        }
      g_debug ("(top,left)-(bottom,right): (%d,%d)-(%d,%d), bit depth: %d",
               pat_top, pat_left, pat_bottom, pat_right, pat_depth);

      /* It would be nice to have extra safety checks based on the reported depth,
       * but it seems Photoshop sets depth to 24 even for grayscale and cmyk,
       * independent of the actual number of channels; so we can't do that.
       * (Possibly it will be 48 instead of 24 in case of 16-bit per channel.)
       * We always include an alpha channel, since we can't know before
       * reading the other channels whether there is an alpha channel or not.
       */
      if (pat_depth == 24)
        {
          switch (pat_image_type)
            {
            case 1: /* Grayscale+A */
              n_channels = 2;
              can_handle = TRUE;
              break;
            case 3: /* RGB+A */
              n_channels = 4;
              can_handle = TRUE;
              break;
            }
        }
      else
        {
          g_printerr ("Unsupported Photoshop pattern depth %d.\n", pat_depth);
        }

      width  = pat_right  - pat_left;
      height = pat_bottom - pat_top;

      if ((width  <= 0) || (width  > GIMP_PATTERN_MAX_SIZE) ||
          (height <= 0) || (height > GIMP_PATTERN_MAX_SIZE))
        {
          g_printerr ("Invalid or unsupported Photoshop pattern size %d x %d\n", width, height);
          can_handle = FALSE;
        }

      if (can_handle)
        {
          GimpPattern *pattern;
          const Babl  *format;
          gsize        size;
          gint         chan;
          guchar      *chan_data;
          guchar      *chan_buf;
          goffset      alpha_ofs;

          chan_buf = NULL;
          format   = NULL;
          pattern  = NULL;

          /* Load pattern data */

          pattern = g_object_new (GIMP_TYPE_PATTERN,
                                  "name",      pat_name,
                                  "mime-type", "image/x-gimp-pat",
                                  NULL);

          switch (n_channels)
            {
            case 2: format = babl_format ("Y'A u8");     break;
            case 4: format = babl_format ("R'G'B'A u8"); break;
            }

          pattern->mask = gimp_temp_buf_new (width, height, format);
          size = (gsize) width * height;

          chan_buf = g_malloc (size);
          if (! chan_buf)
            {
              g_printerr ("Cannot create buffer!\n");
              break;
            }
          chan_data = gimp_temp_buf_get_data (pattern->mask);

          for (chan = 0; chan < n_channels-1; chan++)
            gimp_pattern_load_photoshop_channel (data_input, chan, chan_buf,
                                                 size, chan_data, n_channels,
                                                 error);

          /* An alpha channel (and possibly mask) may follow the normal channels...
           * Only way to tell is to check if there is enough space after the
           * color channels. There appears to also be an 88-byte empty space
           * before the alpha channel for reasons only known to Photoshop.
           */
          alpha_ofs = g_seekable_tell (G_SEEKABLE (data_input)) + 88;
          /* Minimum size of a channel would be at least room for the channel
           * header, so 31 bytes. */
          if (alpha_ofs + 31 < ofs+pat_size)
            {
              if (! g_seekable_seek (G_SEEKABLE (data_input),
                                     alpha_ofs, G_SEEK_SET,
                                     NULL, error))
                {
                  g_printerr ("Error reading Photoshop pattern.\n");
                  update_last_error ();
                  break;
                }
              gimp_pattern_load_photoshop_channel (data_input, n_channels-1, chan_buf,
                                                   size, chan_data, n_channels,
                                                   error);
            }
          else
            {
              guchar *temp_buf;
              gint    j;

              temp_buf = &chan_data[n_channels-1];
              for (j = 0; j < size; j++)
                {
                  temp_buf[j*n_channels] = 255;
                }
            }

          g_free (chan_buf);

          if (pattern)
            pattern_list = g_list_prepend (pattern_list, pattern);
        }
      else if (show_unsupported)
        {
          /* Format not handled yet */
          g_printerr ("Loading Photoshop patterns of type %d is not yet supported.\n",
                      pat_image_type);
          show_unsupported = FALSE;
        }
      g_free (pat_name);

      /* In case we made a mistake reading a pattern: compute offset next pattern. */
      if (! g_seekable_seek (G_SEEKABLE (input),
                             ofs+pat_size, G_SEEK_SET,
                             NULL, error))
        {
          g_printerr ("Failed to seek to next Photoshop pattern offset.\n");
          update_last_error ();
          break;
        }
    }

  if (last_error && *last_error)
    {
      if (pattern_list == NULL)
        {
          /* Only return error if we didn't get any patterns. */
          error = last_error;
        }
      else
        {
          g_clear_error (last_error);
        }
    }

  return pattern_list;
}
