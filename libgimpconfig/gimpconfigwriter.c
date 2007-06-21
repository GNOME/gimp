/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * GimpConfigWriter
 * Copyright (C) 2003  Sven Neumann <sven@gimp.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#include "config.h"

#include <errno.h>
#include <fcntl.h>
#include <string.h>

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include <sys/types.h>

#include <glib-object.h>
#include <glib/gstdio.h>

#ifdef G_OS_WIN32
#include <io.h>
#endif

#include "libgimpbase/gimpbase.h"

#include "gimpconfigtypes.h"

#include "gimpconfigwriter.h"
#include "gimpconfig-iface.h"
#include "gimpconfig-error.h"
#include "gimpconfig-serialize.h"
#include "gimpconfig-utils.h"

#include "libgimp/libgimp-intl.h"


struct _GimpConfigWriter
{
  gint      fd;
  gchar    *filename;
  gchar    *tmpname;
  GError   *error;
  GString  *buffer;
  gboolean  comment;
  gint      depth;
  gint      marker;
};


static inline void  gimp_config_writer_flush      (GimpConfigWriter  *writer);
static inline void  gimp_config_writer_newline    (GimpConfigWriter  *writer);
static gboolean     gimp_config_writer_close_file (GimpConfigWriter  *writer,
                                                   GError           **error);

static inline void
gimp_config_writer_flush (GimpConfigWriter *writer)
{
  if (write (writer->fd, writer->buffer->str, writer->buffer->len) < 0)
    g_set_error (&writer->error, GIMP_CONFIG_ERROR, GIMP_CONFIG_ERROR_WRITE,
                 _("Error writing to '%s': %s"),
                 gimp_filename_to_utf8 (writer->filename), g_strerror (errno));

  g_string_truncate (writer->buffer, 0);
}

static inline void
gimp_config_writer_newline (GimpConfigWriter *writer)
{
  gint i;

  g_string_append_c (writer->buffer, '\n');

  if (writer->comment)
    g_string_append_len (writer->buffer, "# ", 2);

  for (i = 0; i < writer->depth; i++)
    g_string_append_len (writer->buffer, "    ", 4);
}

/**
 * gimp_config_writer_new_file:
 * @filename: a filename
 * @atomic: if %TRUE the file is written atomically
 * @header: text to include as comment at the top of the file
 * @error: return location for errors
 *
 * Creates a new #GimpConfigWriter and sets it up to write to
 * @filename. If @atomic is %TRUE, a temporary file is used to avoid
 * possible race conditions. The temporary file is then moved to
 * @filename when the writer is closed.
 *
 * Return value: a new #GimpConfigWriter or %NULL in case of an error
 *
 * Since: GIMP 2.4
 **/
GimpConfigWriter *
gimp_config_writer_new_file (const gchar  *filename,
                             gboolean      atomic,
                             const gchar  *header,
                             GError      **error)
{
  GimpConfigWriter *writer;
  gchar            *tmpname = NULL;
  gint              fd;

  g_return_val_if_fail (filename != NULL, NULL);
  g_return_val_if_fail (error == NULL || *error == NULL, NULL);

  if (atomic)
    {
      tmpname = g_strconcat (filename, "XXXXXX", NULL);

      fd = g_mkstemp (tmpname);

      if (fd == -1)
        {
          g_set_error (error, GIMP_CONFIG_ERROR, GIMP_CONFIG_ERROR_WRITE,
                       _("Could not create temporary file for '%s': %s"),
                       gimp_filename_to_utf8 (filename), g_strerror (errno));
          g_free (tmpname);
          return NULL;
        }
    }
  else
    {
      fd = g_creat (filename, 0644);

      if (fd == -1)
        {
          g_set_error (error, GIMP_CONFIG_ERROR, GIMP_CONFIG_ERROR_WRITE,
                       _("Could not open '%s' for writing: %s"),
                       gimp_filename_to_utf8 (filename), g_strerror (errno));
          return NULL;
        }
    }

  writer = g_slice_new0 (GimpConfigWriter);

  writer->fd       = fd;
  writer->filename = g_strdup (filename);
  writer->tmpname  = tmpname;
  writer->buffer   = g_string_new (NULL);

  if (header)
    {
      gimp_config_writer_comment (writer, header);
      gimp_config_writer_linefeed (writer);
    }

  return writer;
}

/**
 * gimp_config_writer_new_fd:
 * @fd:
 *
 * Return value: a new #GimpConfigWriter or %NULL in case of an error
 *
 * Since: GIMP 2.4
 **/
GimpConfigWriter *
gimp_config_writer_new_fd (gint fd)
{
  GimpConfigWriter *writer;

  g_return_val_if_fail (fd > 0, NULL);

  writer = g_slice_new0 (GimpConfigWriter);

  writer->fd     = fd;
  writer->buffer = g_string_new (NULL);

  return writer;
}

/**
 * gimp_config_writer_new_string:
 * @string:
 *
 * Return value: a new #GimpConfigWriter or %NULL in case of an error
 *
 * Since: GIMP 2.4
 **/
GimpConfigWriter *
gimp_config_writer_new_string (GString *string)
{
  GimpConfigWriter *writer;

  g_return_val_if_fail (string != NULL, NULL);

  writer = g_slice_new0 (GimpConfigWriter);

  writer->buffer = string;

  return writer;
}

/**
 * gimp_config_writer_comment_mode:
 * @writer: a #GimpConfigWriter
 * @enable: %TRUE to enable comment mode, %FALSE to disable it
 *
 * This function toggles whether the @writer should create commented
 * or uncommented output. This feature is used to generate the
 * system-wide installed gimprc that documents the default settings.
 *
 * Since comments have to start at the beginning of a line, this
 * funtion will insert a newline if necessary.
 *
 * Since: GIMP 2.4
 **/
void
gimp_config_writer_comment_mode (GimpConfigWriter *writer,
                                 gboolean          enable)
{
  g_return_if_fail (writer != NULL);

  if (writer->error)
    return;

  enable = (enable ? TRUE : FALSE);

  if (writer->comment == enable)
    return;

  writer->comment = enable;

  if (enable)
    {
     if (writer->buffer->len == 0)
       g_string_append_len (writer->buffer, "# ", 2);
     else
       gimp_config_writer_newline (writer);
    }
}


/**
 * gimp_config_writer_open:
 * @writer: a #GimpConfigWriter
 * @name: name of the element to open
 *
 * This function writes the opening parenthese followed by @name.
 * It also increases the indentation level and sets a mark that
 * can be used by gimp_config_writer_revert().
 *
 * Since: GIMP 2.4
 **/
void
gimp_config_writer_open (GimpConfigWriter *writer,
                         const gchar      *name)
{
  g_return_if_fail (writer != NULL);
  g_return_if_fail (name != NULL);

  if (writer->error)
    return;

  /* store the current buffer length so we can revert to this state */
  writer->marker = writer->buffer->len;

  if (writer->depth > 0)
    gimp_config_writer_newline (writer);

  writer->depth++;

  g_string_append_printf (writer->buffer, "(%s", name);
}

/**
 * gimp_config_writer_print:
 * @writer: a #GimpConfigWriter
 * @string: a string to write
 * @len: number of bytes from @string or -1 if @string is NUL-terminated.
 *
 * Appends a space followed by @string to the @writer. Note that string
 * must not contain any special characters that might need to be escaped.
 *
 * Since: GIMP 2.4
 **/
void
gimp_config_writer_print (GimpConfigWriter  *writer,
                          const gchar       *string,
                          gint               len)
{
  g_return_if_fail (writer != NULL);
  g_return_if_fail (len == 0 || string != NULL);

  if (writer->error)
    return;

  if (len < 0)
    len = strlen (string);

  if (len)
    {
      g_string_append_c (writer->buffer, ' ');
      g_string_append_len (writer->buffer, string, len);
    }
}

/**
 * gimp_config_writer_printf:
 * @writer: a #GimpConfigWriter
 * @format: a format string as described for g_strdup_printf().
 * @Varargs: list of arguments according to @format
 *
 * A printf-like function for #GimpConfigWriter.
 *
 * Since: GIMP 2.4
 **/
void
gimp_config_writer_printf (GimpConfigWriter *writer,
                           const gchar      *format,
                           ...)
{
  gchar   *buffer;
  va_list  args;

  g_return_if_fail (writer != NULL);
  g_return_if_fail (format != NULL);

  if (writer->error)
    return;

  va_start (args, format);
  buffer = g_strdup_vprintf (format, args);
  va_end (args);

  g_string_append_c (writer->buffer, ' ');
  g_string_append (writer->buffer, buffer);

  g_free (buffer);
}

/**
 * gimp_config_writer_string:
 * @writer: a #GimpConfigWriter
 * @string: a NUL-terminated string
 *
 * Writes a string value to @writer. The @string is quoted and special
 * characters are escaped.
 *
 * Since: GIMP 2.4
 **/
void
gimp_config_writer_string (GimpConfigWriter *writer,
                           const gchar      *string)
{
  g_return_if_fail (writer != NULL);

  if (writer->error)
    return;

  g_string_append_c (writer->buffer, ' ');
  gimp_config_string_append_escaped (writer->buffer, string);
}

/**
 * gimp_config_writer_identifier:
 * @writer:     a #GimpConfigWriter
 * @identifier: a NUL-terminated string
 *
 * Writes an identifier to @writer. The @string is *not* quoted and special
 * characters are *not* escaped.
 *
 * Since: GIMP 2.4
 **/
void
gimp_config_writer_identifier (GimpConfigWriter *writer,
                               const gchar      *identifier)
{
  g_return_if_fail (writer != NULL);
  g_return_if_fail (identifier != NULL);

  if (writer->error)
    return;

  g_string_append_printf (writer->buffer, " %s", identifier);
}


/**
 * gimp_config_writer_data:
 * @writer: a #GimpConfigWriter
 * @length:
 * @data:
 *
 * Since: GIMP 2.4
 **/
void
gimp_config_writer_data (GimpConfigWriter *writer,
                         gint              length,
                         const guint8     *data)
{
  gint i;

  g_return_if_fail (writer != NULL);
  g_return_if_fail (length > 0);
  g_return_if_fail (data != NULL);

  if (writer->error)
    return;

  g_string_append (writer->buffer, " \"");

  for (i = 0; i < length; i++)
    {
      if (g_ascii_isalpha (data[i]))
        g_string_append_c (writer->buffer, data[i]);
      else
        g_string_append_printf (writer->buffer, "\\%o", data[i]);
    }

  g_string_append (writer->buffer, "\"");
}

/**
 * gimp_config_writer_revert:
 * @writer: a #GimpConfigWriter
 *
 * Reverts all changes to @writer that were done since the last call
 * to gimp_config_writer_open(). This can only work if you didn't call
 * gimp_config_writer_close() yet.
 *
 * Since: GIMP 2.4
 **/
void
gimp_config_writer_revert (GimpConfigWriter *writer)
{
  g_return_if_fail (writer != NULL);

  if (writer->error)
    return;

  g_return_if_fail (writer->depth > 0);
  g_return_if_fail (writer->marker != -1);

  g_string_truncate (writer->buffer, writer->marker);

  writer->depth--;
  writer->marker = -1;
}

/**
 * gimp_config_writer_close:
 * @writer: a #GimpConfigWriter
 *
 * Closes an element opened with gimp_config_writer_open().
 *
 * Since: GIMP 2.4
 **/
void
gimp_config_writer_close (GimpConfigWriter *writer)
{
  g_return_if_fail (writer != NULL);

  if (writer->error)
    return;

  g_return_if_fail (writer->depth > 0);

  g_string_append_c (writer->buffer, ')');

  if (--writer->depth == 0)
    {
      g_string_append_c (writer->buffer, '\n');

      if (writer->fd)
        gimp_config_writer_flush (writer);
    }
}

/**
 * gimp_config_writer_finish:
 * @writer: a #GimpConfigWriter
 * @footer: text to include as comment at the bottom of the file
 * @error: return location for possible errors
 *
 * This function finishes the work of @writer and frees it afterwards.
 * It closes all open elements, appends an optional comment and
 * releases all resources allocated by @writer. You must not access
 * the @writer afterwards.
 *
 * Return value: %TRUE if everything could be successfully written,
 *               %FALSE otherwise
 *
 * Since: GIMP 2.4
 **/
gboolean
gimp_config_writer_finish (GimpConfigWriter  *writer,
                           const gchar       *footer,
                           GError           **error)
{
  gboolean success = TRUE;

  g_return_val_if_fail (writer != NULL, FALSE);
  g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

  if (writer->depth < 0)
    {
      g_warning ("gimp_config_writer_finish: depth < 0 !!");
    }
  else
    {
      while (writer->depth)
        gimp_config_writer_close (writer);
    }

  if (footer)
    {
      gimp_config_writer_linefeed (writer);
      gimp_config_writer_comment (writer, footer);
    }

  if (writer->fd)
    {
      success = gimp_config_writer_close_file (writer, error);

      g_free (writer->filename);
      g_free (writer->tmpname);

      g_string_free (writer->buffer, TRUE);
    }
  else
    {
      success = TRUE;
    }

  g_slice_free (GimpConfigWriter, writer);

  if (writer->error)
    {
      g_propagate_error (error, writer->error);
      return FALSE;
    }

  return success;
}

void
gimp_config_writer_linefeed (GimpConfigWriter *writer)
{
  g_return_if_fail (writer != NULL);

  if (writer->error)
    return;

  if (writer->buffer->len == 0 && !writer->comment)
    {
      if (write (writer->fd, "\n", 1) < 0)
        g_set_error (&writer->error, GIMP_CONFIG_ERROR, GIMP_CONFIG_ERROR_WRITE,
                     g_strerror (errno));
    }
  else
    {
      gimp_config_writer_newline (writer);
    }
}

/**
 * gimp_config_writer_comment:
 * @writer: a #GimpConfigWriter
 * @comment: the comment to write (ASCII only)
 *
 * Appends the @comment to @str and inserts linebreaks and hash-marks to
 * format it as a comment. Note that this function does not handle non-ASCII
 * characters.
 *
 * Since: GIMP 2.4
 **/
void
gimp_config_writer_comment (GimpConfigWriter *writer,
                            const gchar      *comment)
{
  const gchar *s;
  gboolean     comment_mode;
  gint         i, len, space;

#define LINE_LENGTH 75

  g_return_if_fail (writer != NULL);

  if (writer->error)
    return;

  g_return_if_fail (writer->depth == 0);

  if (!comment)
    return;

  comment_mode = writer->comment;
  gimp_config_writer_comment_mode (writer, TRUE);

  len = strlen (comment);

  while (len > 0)
    {
      for (s = comment, i = 0, space = 0;
           *s != '\n' && (i <= LINE_LENGTH || space == 0) && i < len;
           s++, i++)
        {
          if (g_ascii_isspace (*s))
            space = i;
        }

      if (i > LINE_LENGTH && space && *s != '\n')
        i = space;

      g_string_append_len (writer->buffer, comment, i);

      i++;

      comment += i;
      len     -= i;

      if (len > 0)
        gimp_config_writer_newline (writer);
    }

  gimp_config_writer_comment_mode (writer, comment_mode);
  gimp_config_writer_newline (writer);

  if (writer->depth == 0)
    gimp_config_writer_flush (writer);

#undef LINE_LENGTH
}

static gboolean
gimp_config_writer_close_file (GimpConfigWriter  *writer,
                               GError           **error)
{
  g_return_val_if_fail (writer->fd != 0, FALSE);

  if (! writer->filename)
    return TRUE;

  if (writer->error)
    {
      close (writer->fd);

      if (writer->tmpname)
        g_unlink (writer->tmpname);

      return FALSE;
    }

  if (close (writer->fd) != 0)
    {
      if (writer->tmpname)
        {
          if (g_file_test (writer->filename, G_FILE_TEST_EXISTS))
            {
              g_set_error (error, GIMP_CONFIG_ERROR, GIMP_CONFIG_ERROR_WRITE,
                           _("Error writing to temporary file for '%s': %s\n"
                             "The original file has not been touched."),
                           gimp_filename_to_utf8 (writer->filename),
                           g_strerror (errno));
            }
          else
            {
              g_set_error (error, GIMP_CONFIG_ERROR, GIMP_CONFIG_ERROR_WRITE,
                           _("Error writing to temporary file for '%s': %s\n"
                             "No file has been created."),
                           gimp_filename_to_utf8 (writer->filename),
                           g_strerror (errno));
            }

          g_unlink (writer->tmpname);
        }
      else
        {
          g_set_error (error, GIMP_CONFIG_ERROR, GIMP_CONFIG_ERROR_WRITE,
                       _("Error writing to '%s': %s"),
                       gimp_filename_to_utf8 (writer->filename),
                       g_strerror (errno));
        }

      return FALSE;
    }

  if (writer->tmpname)
    {
#ifdef G_OS_WIN32
      /* win32 rename can't overwrite */
      g_unlink (writer->filename);
#endif

      if (g_rename (writer->tmpname, writer->filename) == -1)
        {
          g_set_error (error, GIMP_CONFIG_ERROR, GIMP_CONFIG_ERROR_WRITE,
                       _("Could not create '%s': %s"),
                       gimp_filename_to_utf8 (writer->filename),
                       g_strerror (errno));

          g_unlink (writer->tmpname);
          return FALSE;
        }
    }

  return TRUE;
}
