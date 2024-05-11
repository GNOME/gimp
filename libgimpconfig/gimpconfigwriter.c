/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * GimpConfigWriter
 * Copyright (C) 2003  Sven Neumann <sven@gimp.org>
 *
 * This library is free software: you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library.  If not, see
 * <https://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <string.h>

#include <gio/gio.h>

#ifdef G_OS_WIN32
#include <gio/gwin32outputstream.h>
#else
#include <gio/gunixoutputstream.h>
#endif

#include "libgimpbase/gimpbase.h"

#include "gimpconfigtypes.h"

#include "gimpconfigwriter.h"
#include "gimpconfig-iface.h"
#include "gimpconfig-error.h"
#include "gimpconfig-serialize.h"
#include "gimpconfig-utils.h"

#include "libgimp/libgimp-intl.h"


/**
 * SECTION: gimpconfigwriter
 * @title: GimpConfigWriter
 * @short_description: Functions for writing config info to a file for
 *                     libgimpconfig.
 *
 * Functions for writing config info to a file for libgimpconfig.
 **/


struct _GimpConfigWriter
{
  gint           ref_count;
  gboolean       finished;

  GOutputStream *output;
  GFile         *file;
  GError        *error;
  GString       *buffer;
  gboolean       comment;
  gint           depth;
  gint           marker;
};


G_DEFINE_BOXED_TYPE (GimpConfigWriter, gimp_config_writer,
                     gimp_config_writer_ref, gimp_config_writer_unref)


static inline void  gimp_config_writer_flush        (GimpConfigWriter  *writer);
static inline void  gimp_config_writer_newline      (GimpConfigWriter  *writer);
static gboolean     gimp_config_writer_close_output (GimpConfigWriter  *writer,
                                                     GError           **error);

static inline void
gimp_config_writer_flush (GimpConfigWriter *writer)
{
  GError *error = NULL;

  if (! writer->output)
    return;

  if (! g_output_stream_write_all (writer->output,
                                   writer->buffer->str,
                                   writer->buffer->len,
                                   NULL, NULL, &error))
    {
      g_set_error (&writer->error, GIMP_CONFIG_ERROR, GIMP_CONFIG_ERROR_WRITE,
                   _("Error writing to '%s': %s"),
                   writer->file ?
                   gimp_file_get_utf8_name (writer->file) : "output stream",
                   error->message);
      g_clear_error (&error);
    }

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
 * gimp_config_writer_new_from_file:
 * @file: a #GFile
 * @atomic: if %TRUE the file is written atomically
 * @header: text to include as comment at the top of the file
 * @error: return location for errors
 *
 * Creates a new #GimpConfigWriter and sets it up to write to
 * @file. If @atomic is %TRUE, a temporary file is used to avoid
 * possible race conditions. The temporary file is then moved to @file
 * when the writer is closed.
 *
 * Returns: (nullable): a new #GimpConfigWriter or %NULL in case of an error
 *
 * Since: 2.10
 **/
GimpConfigWriter *
gimp_config_writer_new_from_file (GFile        *file,
                                  gboolean      atomic,
                                  const gchar  *header,
                                  GError      **error)
{
  GimpConfigWriter *writer;
  GOutputStream    *output;
  GFile            *dir;

  g_return_val_if_fail (G_IS_FILE (file), NULL);
  g_return_val_if_fail (error == NULL || *error == NULL, NULL);

  dir = g_file_get_parent (file);
  if (dir && ! g_file_query_exists (dir, NULL))
    {
      if (! g_file_make_directory_with_parents (dir, NULL, error))
        g_prefix_error (error,
                        _("Could not create directory '%s' for '%s': "),
                        gimp_file_get_utf8_name (dir),
                        gimp_file_get_utf8_name (file));
    }
  g_object_unref (dir);

  if (error && *error)
    return NULL;

  if (atomic)
    {
      output = G_OUTPUT_STREAM (g_file_replace (file,
                                                NULL, FALSE, G_FILE_CREATE_NONE,
                                                NULL, error));
      if (! output)
        g_prefix_error (error,
                        _("Could not create temporary file for '%s': "),
                        gimp_file_get_utf8_name (file));
    }
  else
    {
      output = G_OUTPUT_STREAM (g_file_replace (file,
                                                NULL, FALSE,
                                                G_FILE_CREATE_REPLACE_DESTINATION,
                                                NULL, error));
    }

  if (! output)
    return NULL;

  writer = g_slice_new0 (GimpConfigWriter);

  writer->ref_count = 1;
  writer->output    = output;
  writer->file      = g_object_ref (file);
  writer->buffer    = g_string_new (NULL);

  if (header)
    {
      gimp_config_writer_comment (writer, header);
      gimp_config_writer_linefeed (writer);
    }

  return writer;
}

/**
 * gimp_config_writer_new_from_stream:
 * @output: a #GOutputStream
 * @header: text to include as comment at the top of the file
 * @error: return location for errors
 *
 * Creates a new #GimpConfigWriter and sets it up to write to
 * @output.
 *
 * Returns: (nullable): a new #GimpConfigWriter or %NULL in case of an error
 *
 * Since: 2.10
 **/
GimpConfigWriter *
gimp_config_writer_new_from_stream (GOutputStream  *output,
                                    const gchar    *header,
                                    GError        **error)
{
  GimpConfigWriter *writer;

  g_return_val_if_fail (G_IS_OUTPUT_STREAM (output), NULL);
  g_return_val_if_fail (error == NULL || *error == NULL, NULL);

  writer = g_slice_new0 (GimpConfigWriter);

  writer->ref_count = 1;
  writer->output    = g_object_ref (output);
  writer->buffer    = g_string_new (NULL);

  if (header)
    {
      gimp_config_writer_comment (writer, header);
      gimp_config_writer_linefeed (writer);
    }

  return writer;
}

/**
 * gimp_config_writer_new_from_fd:
 * @fd:
 *
 * Returns: (nullable): a new #GimpConfigWriter or %NULL in case of an error
 *
 * Since: 2.4
 **/
GimpConfigWriter *
gimp_config_writer_new_from_fd (gint fd)
{
  GimpConfigWriter *writer;

  g_return_val_if_fail (fd > 0, NULL);

  writer = g_slice_new0 (GimpConfigWriter);

  writer->ref_count = 1;

#ifdef G_OS_WIN32
  writer->output = g_win32_output_stream_new ((gpointer) fd, FALSE);
#else
  writer->output = g_unix_output_stream_new (fd, FALSE);
#endif

  writer->buffer = g_string_new (NULL);

  return writer;
}

/**
 * gimp_config_writer_new_from_string:
 * @string:
 *
 * Returns: (nullable): a new #GimpConfigWriter or %NULL in case of an error
 *
 * Since: 2.4
 **/
GimpConfigWriter *
gimp_config_writer_new_from_string (GString *string)
{
  GimpConfigWriter *writer;

  g_return_val_if_fail (string != NULL, NULL);

  writer = g_slice_new0 (GimpConfigWriter);

  writer->ref_count = 1;
  writer->buffer    = string;

  return writer;
}

/**
 * gimp_config_writer_ref:
 * @writer: #GimpConfigWriter to ref
 *
 * Adds a reference to a #GimpConfigWriter.
 *
 * Returns: the same @writer.
 *
 * Since: 3.0
 */
GimpConfigWriter *
gimp_config_writer_ref (GimpConfigWriter *writer)
{
  g_return_val_if_fail (writer != NULL, NULL);

  writer->ref_count++;

  return writer;
}

/**
 * gimp_config_writer_unref:
 * @writer: #GimpConfigWriter to unref
 *
 * Unref a #GimpConfigWriter. If the reference count drops to zero, the
 * writer is freed.
 *
 * Note that at least one of the references has to be dropped using
 * gimp_config_writer_finish().
 *
 * Since: 3.0
 */
void
gimp_config_writer_unref (GimpConfigWriter *writer)
{
  g_return_if_fail (writer != NULL);

  writer->ref_count--;

  if (writer->ref_count < 1)
    {
      if (! writer->finished)
        {
          GError *error = NULL;

          g_printerr ("%s: dropping last reference via unref(), you should "
                      "call gimp_config_writer_finish()\n", G_STRFUNC);

          if (! gimp_config_writer_finish (writer, NULL, &error))
            {
              g_printerr ("%s: error on finishing writer: %s\n",
                          G_STRFUNC, error->message);
            }
        }
      else
        {
          g_slice_free (GimpConfigWriter, writer);
        }
    }
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
 * function will insert a newline if necessary.
 *
 * Since: 2.4
 **/
void
gimp_config_writer_comment_mode (GimpConfigWriter *writer,
                                 gboolean          enable)
{
  g_return_if_fail (writer != NULL);
  g_return_if_fail (writer->finished == FALSE);

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
 * This function writes the opening parenthesis followed by @name.
 * It also increases the indentation level and sets a mark that
 * can be used by gimp_config_writer_revert().
 *
 * Since: 2.4
 **/
void
gimp_config_writer_open (GimpConfigWriter *writer,
                         const gchar      *name)
{
  g_return_if_fail (writer != NULL);
  g_return_if_fail (writer->finished == FALSE);
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
 * Since: 2.4
 **/
void
gimp_config_writer_print (GimpConfigWriter  *writer,
                          const gchar       *string,
                          gint               len)
{
  g_return_if_fail (writer != NULL);
  g_return_if_fail (writer->finished == FALSE);
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
 * gimp_config_writer_printf: (skip)
 * @writer: a #GimpConfigWriter
 * @format: a format string as described for g_strdup_printf().
 * @...: list of arguments according to @format
 *
 * A printf-like function for #GimpConfigWriter.
 *
 * Since: 2.4
 **/
void
gimp_config_writer_printf (GimpConfigWriter *writer,
                           const gchar      *format,
                           ...)
{
  gchar   *buffer;
  va_list  args;

  g_return_if_fail (writer != NULL);
  g_return_if_fail (writer->finished == FALSE);
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
 * Since: 2.4
 **/
void
gimp_config_writer_string (GimpConfigWriter *writer,
                           const gchar      *string)
{
  g_return_if_fail (writer != NULL);
  g_return_if_fail (writer->finished == FALSE);

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
 * Since: 2.4
 **/
void
gimp_config_writer_identifier (GimpConfigWriter *writer,
                               const gchar      *identifier)
{
  g_return_if_fail (writer != NULL);
  g_return_if_fail (writer->finished == FALSE);
  g_return_if_fail (identifier != NULL);

  if (writer->error)
    return;

  g_string_append_printf (writer->buffer, " %s", identifier);
}


/**
 * gimp_config_writer_data:
 * @writer: a #GimpConfigWriter
 * @length:                    : The size of @data
 * @data: (array length=length): The data to write
 *
 * Writes data to @writer.
 *
 * Since: 2.4
 **/
void
gimp_config_writer_data (GimpConfigWriter *writer,
                         gint              length,
                         const guint8     *data)
{
  gint i;

  g_return_if_fail (writer != NULL);
  g_return_if_fail (writer->finished == FALSE);
  g_return_if_fail (length >= 0);
  g_return_if_fail (data != NULL || length == 0);

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
 * Since: 2.4
 **/
void
gimp_config_writer_revert (GimpConfigWriter *writer)
{
  g_return_if_fail (writer != NULL);
  g_return_if_fail (writer->finished == FALSE);

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
 * Since: 2.4
 **/
void
gimp_config_writer_close (GimpConfigWriter *writer)
{
  g_return_if_fail (writer != NULL);
  g_return_if_fail (writer->finished == FALSE);

  if (writer->error)
    return;

  g_return_if_fail (writer->depth > 0);

  g_string_append_c (writer->buffer, ')');

  if (--writer->depth == 0)
    {
      g_string_append_c (writer->buffer, '\n');

      gimp_config_writer_flush (writer);
    }
}

/**
 * gimp_config_writer_finish:
 * @writer: a #GimpConfigWriter
 * @footer: text to include as comment at the bottom of the file
 * @error: return location for possible errors
 *
 * This function finishes the work of @writer and unrefs it
 * afterwards.  It closes all open elements, appends an optional
 * comment and releases all resources allocated by @writer.
 *
 * Using any function except gimp_config_writer_ref() or
 * gimp_config_writer_unref() after this function is forbidden
 * and will trigger warnings.
 *
 * Returns: %TRUE if everything could be successfully written,
 *          %FALSE otherwise
 *
 * Since: 2.4
 **/
gboolean
gimp_config_writer_finish (GimpConfigWriter  *writer,
                           const gchar       *footer,
                           GError           **error)
{
  gboolean success = TRUE;

  g_return_val_if_fail (writer != NULL, FALSE);
  g_return_val_if_fail (writer->finished == FALSE, FALSE);
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

  if (writer->output)
    {
      success = gimp_config_writer_close_output (writer, error);

      g_clear_object (&writer->file);

      g_string_free (writer->buffer, TRUE);
      writer->buffer = NULL;
    }

  if (writer->error)
    {
      if (error && *error == NULL)
        g_propagate_error (error, writer->error);
      else
        g_clear_error (&writer->error);

      success = FALSE;
    }

  writer->finished = TRUE;

  gimp_config_writer_unref (writer);

  return success;
}

void
gimp_config_writer_linefeed (GimpConfigWriter *writer)
{
  g_return_if_fail (writer != NULL);
  g_return_if_fail (writer->finished == FALSE);

  if (writer->error)
    return;

  if (writer->output && writer->buffer->len == 0 && !writer->comment)
    {
      GError *error = NULL;

      if (! g_output_stream_write_all (writer->output, "\n", 1,
                                       NULL, NULL, &error))
        {
          g_set_error (&writer->error, GIMP_CONFIG_ERROR, GIMP_CONFIG_ERROR_WRITE,
                       _("Error writing to '%s': %s"),
                       writer->file ?
                       gimp_file_get_utf8_name (writer->file) : "output stream",
                       error->message);
          g_clear_error (&error);
        }
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
 * Since: 2.4
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
  g_return_if_fail (writer->finished == FALSE);

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
gimp_config_writer_close_output (GimpConfigWriter  *writer,
                                 GError           **error)
{
  g_return_val_if_fail (writer->output != NULL, FALSE);

  if (writer->error)
    {
      GCancellable *cancellable = g_cancellable_new ();

      /* Cancel the overwrite initiated by g_file_replace(). */
      g_cancellable_cancel (cancellable);
      g_output_stream_close (writer->output, cancellable, NULL);
      g_object_unref (cancellable);

      g_clear_object (&writer->output);

      return FALSE;
    }

  if (writer->file)
    {
      GError *my_error = NULL;

      if (! g_output_stream_close (writer->output, NULL, &my_error))
        {
          g_set_error (error, GIMP_CONFIG_ERROR, GIMP_CONFIG_ERROR_WRITE,
                       _("Error writing '%s': %s"),
                       gimp_file_get_utf8_name (writer->file),
                       my_error->message);
          g_clear_error (&my_error);

          g_clear_object (&writer->output);

          return FALSE;
        }
    }

  g_clear_object (&writer->output);

  return TRUE;
}
