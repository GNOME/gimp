/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995-1997 Spencer Kimball and Peter Mattis
 *
 * GimpXmlParser
 * Copyright (C) 2003  Sven Neumann <sven@gimp.org>
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

#include <string.h>

#include <gio/gio.h>

#include "config-types.h"

#include "gimpxmlparser.h"


struct _GimpXmlParser
{
  GMarkupParseContext *context;
};


static gboolean parse_encoding (const gchar  *text,
                                gint          text_len,
                                gchar       **encodind);


/**
 * gimp_xml_parser_new:
 * @markup_parser: a #GMarkupParser
 * @user_data: user data to pass to #GMarkupParser functions
 *
 * GimpXmlParser is a thin wrapper around GMarkupParser. This function
 * creates one for you and sets up a GMarkupParseContext.
 *
 * Returns: a new #GimpXmlParser
 **/
GimpXmlParser *
gimp_xml_parser_new (const GMarkupParser *markup_parser,
                     gpointer             user_data)
{
  GimpXmlParser *parser;

  g_return_val_if_fail (markup_parser != NULL, NULL);

  parser = g_slice_new (GimpXmlParser);

  parser->context = g_markup_parse_context_new (markup_parser,
                                                0, user_data, NULL);

  return parser;
}

/**
 * gimp_xml_parser_parse_file:
 * @parser: a #GimpXmlParser
 * @filename: name of a file to parse
 * @error: return location for possible errors
 *
 * This function creates a GIOChannel for @filename and calls
 * gimp_xml_parser_parse_io_channel() for you.
 *
 * Returns: %TRUE on success, %FALSE otherwise
 **/
gboolean
gimp_xml_parser_parse_file (GimpXmlParser  *parser,
                            const gchar    *filename,
                            GError        **error)
{
  GIOChannel *io;
  gboolean    success;

  g_return_val_if_fail (parser != NULL, FALSE);
  g_return_val_if_fail (filename != NULL, FALSE);
  g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

  io = g_io_channel_new_file (filename, "r", error);
  if (!io)
    return FALSE;

  success = gimp_xml_parser_parse_io_channel (parser, io, error);

  g_io_channel_unref (io);

  return success;
}

/**
 * gimp_xml_parser_parse_gfile:
 * @parser: a #GimpXmlParser
 * @file: the #GFile to parse
 * @error: return location for possible errors
 *
 * This function creates a GIOChannel for @file and calls
 * gimp_xml_parser_parse_io_channel() for you.
 *
 * Returns: %TRUE on success, %FALSE otherwise
 **/
gboolean
gimp_xml_parser_parse_gfile (GimpXmlParser  *parser,
                             GFile          *file,
                             GError        **error)
{
  gchar    *path;
  gboolean  success;

  g_return_val_if_fail (parser != NULL, FALSE);
  g_return_val_if_fail (G_IS_FILE (file), FALSE);

  path = g_file_get_path (file);

  success = gimp_xml_parser_parse_file (parser, path, error);

  g_free (path);

  return success;
}

/**
 * gimp_xml_parser_parse_fd:
 * @parser: a #GimpXmlParser
 * @fd:     a file descriptor
 * @error: return location for possible errors
 *
 * This function creates a GIOChannel for @fd and calls
 * gimp_xml_parser_parse_io_channel() for you.
 *
 * Returns: %TRUE on success, %FALSE otherwise
 **/
gboolean
gimp_xml_parser_parse_fd (GimpXmlParser  *parser,
                          gint            fd,
                          GError        **error)
{
  GIOChannel *io;
  gboolean    success;

  g_return_val_if_fail (parser != NULL, FALSE);
  g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

#ifdef G_OS_WIN32
  io = g_io_channel_win32_new_fd (fd);
#else
  io = g_io_channel_unix_new (fd);
#endif

  success = gimp_xml_parser_parse_io_channel (parser, io, error);

  g_io_channel_unref (io);

  return success;
}

/**
 * gimp_xml_parser_parse_io_channel:
 * @parser: a #GimpXmlParser
 * @io: a #GIOChannel
 * @error: return location for possible errors
 *
 * Makes @parser read from the specified @io channel. This function
 * returns when the GIOChannel becomes empty (end of file) or an
 * error occurs, either reading from @io or parsing the read data.
 *
 * This function tries to determine the character encoding from the
 * XML header and converts the content to UTF-8 for you. For this
 * feature to work, the XML header with the encoding attribute must be
 * contained in the first 4096 bytes read. Otherwise UTF-8 encoding
 * will be assumed and parsing may break later if this assumption
 * was wrong.
 *
 * Returns: %TRUE on success, %FALSE otherwise
 **/
gboolean
gimp_xml_parser_parse_io_channel (GimpXmlParser  *parser,
                                  GIOChannel     *io,
                                  GError        **error)
{
  GIOStatus    status;
  gchar        buffer[4096];
  gsize        len = 0;
  gsize        bytes;
  const gchar *io_encoding;
  gchar       *encoding = NULL;

  g_return_val_if_fail (parser != NULL, FALSE);
  g_return_val_if_fail (io != NULL, FALSE);
  g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

  io_encoding = g_io_channel_get_encoding (io);
  if (g_strcmp0 (io_encoding, "UTF-8"))
    {
      g_warning ("gimp_xml_parser_parse_io_channel():\n"
                 "The encoding has already been set on this GIOChannel!");
      return FALSE;
    }

  /* try to determine the encoding */

  g_io_channel_set_encoding (io, NULL, NULL);

  while (len < sizeof (buffer))
    {
      status = g_io_channel_read_chars (io, buffer + len, 1, &bytes, error);
      len += bytes;

      if (status == G_IO_STATUS_ERROR)
        return FALSE;
      if (status == G_IO_STATUS_EOF)
        break;

      if (parse_encoding (buffer, len, &encoding))
        break;
    }

  if (encoding)
    {
      if (! g_io_channel_set_encoding (io, encoding, error))
        return FALSE;

      g_free (encoding);
    }
  else
    {
      g_io_channel_set_encoding (io, "UTF-8", NULL);
    }

  while (TRUE)
    {
      if (!g_markup_parse_context_parse (parser->context, buffer, len, error))
        return FALSE;

      status = g_io_channel_read_chars (io,
                                        buffer, sizeof (buffer), &len, error);

      switch (status)
        {
        case G_IO_STATUS_ERROR:
          return FALSE;
        case G_IO_STATUS_EOF:
          return g_markup_parse_context_end_parse (parser->context, error);
        case G_IO_STATUS_NORMAL:
        case G_IO_STATUS_AGAIN:
          break;
        }
    }
}

/**
 * gimp_xml_parser_parse_buffer:
 * @parser: a #GimpXmlParser
 * @buffer: a string buffer
 * @len: the number of byes in @buffer or -1 if @buffer is nul-terminated
 * @error: return location for possible errors
 *
 * This function uses the given @parser to parse the XML in @buffer.
 *
 * Returns: %TRUE on success, %FALSE otherwise
 **/
gboolean
gimp_xml_parser_parse_buffer (GimpXmlParser  *parser,
                              const gchar    *buffer,
                              gssize          len,
                              GError        **error)
{
  gchar    *encoding = NULL;
  gchar    *conv     = NULL;
  gboolean  success;

  g_return_val_if_fail (parser != NULL, FALSE);
  g_return_val_if_fail (buffer != NULL || len == 0, FALSE);
  g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

  if (len < 0)
    len = strlen (buffer);

  if (parse_encoding (buffer, len, &encoding) && encoding)
    {
      if (g_ascii_strcasecmp (encoding, "UTF-8") &&
          g_ascii_strcasecmp (encoding, "UTF8"))
        {
          gsize written;

          conv = g_convert (buffer, len,
                            "UTF-8", encoding, NULL, &written, error);
          if (! conv)
            {
              g_free (encoding);
              return FALSE;
            }

          len = written;
        }

      g_free (encoding);
    }

  success = g_markup_parse_context_parse (parser->context,
                                          conv ? conv : buffer, len, error);

  if (conv)
    g_free (conv);

  return success;
}

/**
 * gimp_xml_parser_free:
 * @parser: a #GimpXmlParser
 *
 * Frees the resources allocated for @parser. You must not access
 * @parser after calling this function.
 **/
void
gimp_xml_parser_free (GimpXmlParser *parser)
{
  g_return_if_fail (parser != NULL);

  g_markup_parse_context_free (parser->context);
  g_slice_free (GimpXmlParser, parser);
}


/* Try to determine encoding from XML header.  This function returns
   FALSE when it doesn't have enough text to parse.  It returns TRUE
   and sets encoding when the XML header has been parsed.
 */
static gboolean
parse_encoding (const gchar  *text,
                gint          text_len,
                gchar       **encoding)
{
  const gchar *start;
  const gchar *end;
  gint         i;

  g_return_val_if_fail (text, FALSE);

  if (text_len < 20)
    return FALSE;

  start = g_strstr_len (text, text_len, "<?xml");
  if (!start)
    return FALSE;

  end = g_strstr_len (start, text_len - (start - text), "?>");
  if (!end)
    return FALSE;

  *encoding = NULL;

  text_len = end - start;
  if (text_len < 12)
    return TRUE;

  start = g_strstr_len (start + 1, text_len - 1, "encoding");
  if (!start)
    return TRUE;

  start += 8;

  while (start < end && *start == ' ')
    start++;

  if (*start != '=')
    return TRUE;

  start++;

  while (start < end && *start == ' ')
    start++;

  if (*start != '\"' && *start != '\'')
    return TRUE;

  text_len = end - start;
  if (text_len < 1)
    return TRUE;

  for (i = 1; i < text_len; i++)
    if (start[i] == start[0])
      break;

  if (i == text_len || i < 3)
    return TRUE;

  *encoding = g_strndup (start + 1, i - 1);

  return TRUE;
}
