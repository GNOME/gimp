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

#include "gimp-intl.h"


static gboolean     photoshop_read_char           (GDataInputStream  *input,
                                                   gchar             *value,
                                                   GError           **error);
static gboolean     photoshop_read_short          (GDataInputStream  *input,
                                                  gint16            *value,
                                                  GError           **error);
static gboolean     photoshop_read_long           (GDataInputStream  *input,
                                                  gint32            *value,
                                                  GError           **error);
static gboolean     photoshop_read_ucs2_text     (GDataInputStream  *input,
                                                  gchar            **value,
                                                  GError           **error);

static gboolean first = TRUE;

gint
psd_read (GInputStream  *input,
          gconstpointer  data,
          gint           count,
          GError       **error)
{
  gsize bytes_read = 0;

  /* we allow for 'data == NULL && count == 0', which g_input_stream_read_all()
   * rejects.
   */
  if (count > 0)
    {
      /* We consider reading less bytes than we want an error. */
      if (g_input_stream_read_all (input, (void *) data, count,
                                   &bytes_read, NULL, error) &&
          bytes_read < count)
        g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
                     _("Unexpected end of file"));
    }

  return bytes_read;
}

GList *
gimp_pattern_load_photoshop_pattern (GimpContext   *context,
                                     GFile         *file,
                                     GInputStream  *input,
                                     GError       **error)
{
  guint16           version   = 0;
  guint32           pat_count = 0;
  gint              i;
  goffset           ofs;
  GList            *pattern_list = NULL;
  GDataInputStream *data_input;

  data_input = g_data_input_stream_new (input);
  g_data_input_stream_set_byte_order (data_input,
                                      G_DATA_STREAM_BYTE_ORDER_BIG_ENDIAN);

  /* Reset stream position to right after magic*/
  if (! g_seekable_seek (G_SEEKABLE (input),
                          4, G_SEEK_SET,
                          NULL, error))
    {
      g_prefix_error (error, _("Error reading Photoshop pattern."));
      return NULL;
    }

  if (! photoshop_read_short (data_input, &version, error) ||
      ! photoshop_read_long (data_input, &pat_count, error))
    {
      g_prefix_error (error, _("Error reading Photoshop pattern."));
      return NULL;
    }
  g_printerr ("Photoshop Pattern file. Version: %u, number of patterns: %u\n", version, pat_count);

  for (i = 0; i < pat_count; i++)
    {
      goffset   temp_ofs       = 0;
      guint32   pat_version    = 0;
      guint32   pat_image_type = 0;
      guint16   pat_height     = 0;
      guint16   pat_width      = 0;
      gchar    *pat_name;
      guint32   pat_size       = 0;
      guint32   pat_top, pat_left, pat_bottom, pat_right;
      guint32   pat_depth      = 0;
      guint32   n_channels     = 0;
      gboolean  can_handle;

      can_handle = FALSE;
      pat_name   = NULL;

      if (! photoshop_read_long (data_input,  &pat_version,    error) ||
          ! photoshop_read_long (data_input,  &pat_image_type, error) ||
          ! photoshop_read_short (data_input, &pat_height,     error) ||
          ! photoshop_read_short (data_input, &pat_width,      error) ||
          ! photoshop_read_ucs2_text (data_input, &pat_name,   error))
        {
          g_prefix_error (error, _("Error reading Photoshop pattern."));
          return NULL;
        }
      g_printerr ("Pattern version %u, image type: %u, height x width: %u x %u, name: %s\n",
                   pat_version, pat_image_type, pat_height, pat_width, pat_name);

      /* For now seek past pattern id, ideally we would read the length to make
      * sure the size is as expected (length byte that usually says 36)
      * (it's a Pascal string). */
      if (! g_seekable_seek (G_SEEKABLE (input),
                              37, G_SEEK_CUR,
                              NULL, error))
        {
          g_prefix_error (error, _("Error reading Photoshop pattern."));
          return NULL;
        }

      /* FIXME: Support for reading the palette if this is indexed */
      if (pat_image_type == 2)
        {
          g_prefix_error (error, _("Indexed Photoshop pattern is not supported."));
          return NULL;
        }

      if (! photoshop_read_long (data_input,  &pat_version,    error) ||
          ! photoshop_read_long (data_input,  &pat_size, error))
        {
          g_prefix_error (error, _("Error reading Photoshop pattern."));
          return NULL;
        }
      ofs = g_seekable_tell (G_SEEKABLE (input));
      g_printerr ("Pattern version %u, size: %u, offset: %llx, next %llx\n",
                  pat_version, pat_size, ofs, ofs + pat_size);

      if (! photoshop_read_long (data_input,  &pat_top,    error) ||
          ! photoshop_read_long (data_input,  &pat_left,   error) ||
          ! photoshop_read_long (data_input,  &pat_bottom, error) ||
          ! photoshop_read_long (data_input,  &pat_right,  error) ||
          ! photoshop_read_long (data_input,  &pat_depth,  error))
        {
          g_prefix_error (error, _("Error reading Photoshop pattern."));
          return NULL;
        }
      g_printerr ("(top,left)-(bottom,right): (%d,%d)-(%d,%d), bit depth: %d\n",
                  pat_top, pat_left, pat_bottom, pat_right, pat_depth);

      switch (pat_image_type)
        {
        case 3: /* RGB*/
          if (pat_depth == 24)
            {
              n_channels = 3;
              can_handle = TRUE;
            }
          break;

        default:
          g_prefix_error (error, _("Photoshop pattern type %d is not supported."), pat_image_type);
          return NULL;

          break;
        }

      if (can_handle)
        {
          GimpPattern *pattern = NULL;
          const Babl  *format  = NULL;
          gint32       width, height;
          gsize        size, bytes_read;
          gint         i;
          guchar      *chan_data;
          guchar      *chan_buf;

          chan_buf = NULL;
          /* Load pattern data */
          /* FIXME: Check valid dimensions*/
          /*        Check max pattern name length*/
          g_printerr ("Create pattern object\n");
          pattern = g_object_new (GIMP_TYPE_PATTERN,
                                  "name",      pat_name,
                                  "mime-type", "image/x-gimp-pat",
                                  NULL);
          if (! pattern)
            g_printerr ("Failed to get pattern object!\n");

          switch (n_channels)
            {
            case 1: format = babl_format ("Y' u8");      break;
            case 2: format = babl_format ("Y'A u8");     break;
            case 3: format = babl_format ("R'G'B' u8");  break;
            case 4: format = babl_format ("R'G'B'A u8"); break;
            }

          width  = pat_bottom - pat_top;
          height = pat_right  - pat_left;
          //g_printerr ("Get temp buf\n");
          pattern->mask = gimp_temp_buf_new (width, height, format);
          size = (gsize) width * height;
          //g_printerr ("Malloc chan buf\n");
          chan_buf = g_malloc (size);
          //if (! chan_buf)
          //  g_printerr ("chan_buf NULL!\n");
          //g_printerr ("Get temp buf data\n");
          chan_data = gimp_temp_buf_get_data (pattern->mask);

          for (i = 0; i < n_channels; i++)
            {
              gint      j;
              goffset   cofs             = 0;
              guint32   chan_version     = 0;
              guint32   chan_size        = 0;
              guint16   chan_depth       = 0;
              guint8    chan_compression = 0;
              guint32   chan_top, chan_left, chan_bottom, chan_right;
              guchar   *temp_buf;

              temp_buf = NULL;

              //g_printerr ("call seekable tell...\n");
              temp_ofs = g_seekable_tell (G_SEEKABLE (data_input));
              g_printerr ("Offset: %llx\n", temp_ofs);

              if (! photoshop_read_long (data_input,  &chan_version, error) ||
                  ! photoshop_read_long (data_input,  &chan_size,    error))
                {
                  g_prefix_error (error, _("Error reading channel"));
                  return NULL;
                }
              cofs = g_seekable_tell (G_SEEKABLE (data_input));
              /** Dummy depth */
              if (! photoshop_read_long (data_input,  &chan_depth, error))
                {
                  g_prefix_error (error, _("Error reading channel"));
                  return NULL;
                }
              g_printerr ("Channel version %u, size: %u, offset: %llx, next %llx, dummy: %d\n",
                          chan_version, chan_size, cofs, cofs + chan_size, chan_depth);

              if (! photoshop_read_long (data_input,  &chan_top,         error) ||
                  ! photoshop_read_long (data_input,  &chan_left,        error) ||
                  ! photoshop_read_long (data_input,  &chan_bottom,      error) ||
                  ! photoshop_read_long (data_input,  &chan_right,       error) ||
                  ! photoshop_read_short (data_input, &chan_depth,       error) ||
                  ! photoshop_read_char (data_input,  &chan_compression, error))
                {
                  g_prefix_error (error, _("Error reading Photoshop pattern."));
                  return NULL;
                }
              g_printerr ("(top,left)-(bottom,right): (%d,%d)-(%d,%d), bit depth: %d, compression %d\n",
                          chan_top, chan_left, chan_bottom, chan_right, chan_depth, chan_compression);

              if (chan_compression == 0)
                {
                  if (! g_input_stream_read_all (data_input,
                                                /*&chan_data[i*size]*/ chan_buf, size,
                                                &bytes_read, NULL, error) ||
                      bytes_read != size)
                    {
                      g_prefix_error (error, _("Photoshop pattern appears to be truncated."));
                      // FIXME: Free data!
                      return NULL;
                    }
                  /* Move channel data to correct offset in pixels (TODO: only if channels > 1)*/
                  //temp_buf = &chan_data[i*size];
                  temp_buf = &chan_data[i];
                  //g_printerr ("Set pixels for channel %d...\n", i);
                  for (j = 0; j < size; j++)
                    {
                      //
                      //chan_data[i*size + j] = chan_buf[j];
                      temp_buf[j*n_channels] = chan_buf[j];
                    }
                }
              else
                {
                  g_printerr ("TODO: Read compressed channel data.\n");
                }

              /* Always seek to next offset */
              temp_ofs = g_seekable_tell (G_SEEKABLE (data_input));
              g_printerr ("Offset: %llx -- seek to next channel at %llx\n", temp_ofs, cofs+chan_size);
              if (! g_seekable_seek (G_SEEKABLE (data_input),
                                      cofs+chan_size, G_SEEK_SET,
                                      NULL, error))
                {
                  g_printerr ("seek failed\n");
                  g_prefix_error (error, _("Error reading Photoshop pattern."));
                  return NULL;
                }
            }
          /* TODO: An alpha channel (and possibly mask) may follow the normal channels... */
          //g_printerr ("++ Channels done ++\n");

          g_free (chan_buf);
          //g_printerr ("++ free chan_buf done ++\n");
          //return g_list_prepend (NULL, pattern);
          if (pattern)
            {
              //g_printerr ("+ add pattern\n");
              pattern_list = g_list_prepend (pattern_list, pattern);
            }
          else if (error)
            {
              g_printerr ("We encountered an error: %s\n", (*error)->message);
              //g_prefix_error (error, error->mes);
              break;
            }
          else
            g_printerr ("NO pattern, no error!\n");
        }
      g_free (pat_name);

      /* In case we made a mistake reading a pattern: compute offset next pattern. */
      temp_ofs = g_seekable_tell (G_SEEKABLE (input));
      g_printerr ("Offset: %llx -- seek to next pattern at %llx\n", temp_ofs, ofs+pat_size);
      if (! g_seekable_seek (G_SEEKABLE (input),
                              ofs+pat_size, G_SEEK_SET,
                              NULL, error))
        {
          g_prefix_error (error, _("Error reading Photoshop pattern."));
          return NULL;
        }
      //////////////////////////////////////////////////////
      //break; // FOR NOW
      //////////////////////////////////////////////////////
    }

  return pattern_list;
}

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
  if (memcmp (&header.header_size, "8BPT", 4) == 0 /*&& first*/)
    {
      GList *ps_patterns = NULL;

      first = FALSE;
      //g_printerr ("\t-->This is a Photoshop pattern!\n");
      ps_patterns = gimp_pattern_load_photoshop_pattern (context, file, input, error);
      if (error && *error)
        g_printerr ("Error encountered: %s\n", (*error)->message);
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


/* Copied and adjusted from gimpbrush-load.c*/
/* FIXME: Refactor */

static gboolean
photoshop_read_char (GDataInputStream  *input,
                     gchar             *value,
                     GError           **error)
{
  gchar result;

  result = g_data_input_stream_read_byte (input, NULL, error);
  if (error && *error)
    {
      return FALSE;
    }
  *value = result;
  g_printerr ("%c", result);
  return TRUE;
}

static gboolean
photoshop_read_short (GDataInputStream  *input,
                      gint16            *value,
                      GError           **error)
{
  gint16 result;

  result = g_data_input_stream_read_int16 (input, NULL, error);
  if (error && *error)
    {
      return FALSE;
    }
  *value = result;
  return TRUE;
}

static gboolean
photoshop_read_long (GDataInputStream  *input,
                     gint32            *value,
                     GError           **error)
{
  gint32 result;

  result = g_data_input_stream_read_int32 (input, NULL, error);
  if (error && *error)
    {
      return FALSE;
    }
  *value = result;
  return TRUE;
}

static gboolean
photoshop_read_ucs2_text (GDataInputStream  *input,
                          gchar            **value,
                          GError           **error)
{
  gchar *name_ucs2;
  gint32 pslen = 0;
  gint   len;
  gint   i;

  /* two-bytes characters encoded (UCS-2)
   *  format:
   *   long : number of characters in string
   *   data : zero terminated UCS-2 string
   */

  if (! photoshop_read_long (input, &pslen, error) || pslen <= 0)
    return FALSE;

  len = 2 * pslen;
  g_printerr ("Length: %u\n[", len);

  name_ucs2 = g_new (gchar, len);

  for (i = 0; i < len; i++)
    {
      gchar mychar;

      if (! photoshop_read_char (input, &mychar, error))
        {
          g_free (name_ucs2);
          return FALSE;
        }
      name_ucs2[i] = mychar;
    }

  g_printerr ("] - and now convert\n");
  *value = g_convert (name_ucs2, len,
                      "UTF-8", "UCS-2BE",
                      NULL, NULL, NULL);
  g_printerr ("string: '%s'\n", *value);


  g_free (name_ucs2);

  return (*value != NULL);
}
