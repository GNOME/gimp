/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * GimpConfigWriter
 * Copyright (C) 2003  Sven Neumann <sven@gimp.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include "config.h"

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#include <sys/types.h>
#include <sys/stat.h>

#include <glib-object.h>

#ifdef G_OS_WIN32
#include <io.h>
#endif

#include "config-types.h"

#include "gimpconfig.h"
#include "gimpconfig-serialize.h"
#include "gimpconfig-utils.h"
#include "gimpconfigwriter.h"

#include "gimp-intl.h"


static gboolean  gimp_config_writer_close_file (GimpConfigWriter  *writer,
						GError           **error);


GimpConfigWriter *
gimp_config_writer_new (const gchar  *filename,
			gboolean      safe,
			const gchar  *header,
			GError      **error)
{
  GimpConfigWriter *writer;
  gchar            *tmpname = NULL;
  gint              fd;

  g_return_val_if_fail (filename != NULL, NULL);
  g_return_val_if_fail (error == NULL || *error == NULL, NULL);

  if (safe)
    {
      tmpname = g_strconcat (filename, "XXXXXX", NULL);
  
      fd = g_mkstemp (tmpname);

      if (fd == -1)
	{
	  g_set_error (error, 
		       GIMP_CONFIG_ERROR, GIMP_CONFIG_ERROR_WRITE, 
		       _("Failed to create temporary file for '%s': %s"),
		       filename, g_strerror (errno));
	  g_free (tmpname);
	  return NULL;
	}
    }
  else
    {
      fd = creat (filename, 0644);

      if (fd == -1)
	{
	  g_set_error (error, 
		       GIMP_CONFIG_ERROR, GIMP_CONFIG_ERROR_WRITE, 
		       _("Failed to open '%s' for writing: %s"),
		       filename, g_strerror (errno));
	  return NULL;
	}
    }

  writer = g_new0 (GimpConfigWriter, 1);

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

GimpConfigWriter *
gimp_config_writer_new_from_fd (gint fd)
{
  GimpConfigWriter *writer;

  g_return_val_if_fail (fd > 0, NULL);

  writer = g_new0 (GimpConfigWriter, 1);

  writer->fd     = fd;
  writer->buffer = g_string_new (NULL);

  return writer;
}

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
    {
      g_string_append_c (writer->buffer, '\n');
      gimp_config_string_indent (writer->buffer, writer->depth);
    }

  writer->depth++;

  g_string_append_printf (writer->buffer, "(%s", name);
}

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

void
gimp_config_writer_revert (GimpConfigWriter *writer)
{
  g_return_if_fail (writer != NULL);
  g_return_if_fail (writer->depth > 0);
  g_return_if_fail (writer->marker != -1);

  if (writer->error)
    return;

  g_string_truncate (writer->buffer, writer->marker);

  writer->depth--;
  writer->marker = -1;
}

void
gimp_config_writer_close (GimpConfigWriter *writer)
{
  g_return_if_fail (writer != NULL);
  g_return_if_fail (writer->depth > 0);

  if (writer->error)
    return;

  g_string_append_c (writer->buffer, ')');

  if (--writer->depth == 0)
    {
      g_string_append_c (writer->buffer, '\n');
      
      if (write (writer->fd, writer->buffer->str, writer->buffer->len) < 0)
	g_set_error (&writer->error,
                     GIMP_CONFIG_ERROR, GIMP_CONFIG_ERROR_WRITE,
		     g_strerror (errno));

      g_string_truncate (writer->buffer, 0);
    }
}

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

  success = gimp_config_writer_close_file (writer, error);

  g_free (writer->filename);
  g_free (writer->tmpname);

  g_string_free (writer->buffer, TRUE);

  g_free (writer);

  return success;
}

void
gimp_config_writer_linefeed (GimpConfigWriter *writer)
{
  g_return_if_fail (writer != NULL);
 
  if (writer->error)
    return;

  if (writer->buffer->len == 0)
    {
      if (write (writer->fd, "\n", 1) < 0)
        g_set_error (&writer->error,
                     GIMP_CONFIG_ERROR, GIMP_CONFIG_ERROR_WRITE,
                     g_strerror (errno));
    }
  else
    {
      g_string_append_c (writer->buffer, '\n');
      gimp_config_string_indent (writer->buffer, writer->depth);
    }
}

void
gimp_config_writer_comment (GimpConfigWriter *writer,
			    const gchar      *comment)
{
  g_return_if_fail (writer != NULL);
  g_return_if_fail (writer->depth == 0);
  g_return_if_fail (writer->buffer->len == 0);

  if (writer->error)
    return;

  if (!comment)
    return;

  gimp_config_serialize_comment (writer->buffer, comment);

  if (write (writer->fd, writer->buffer->str, writer->buffer->len) < 0)
    g_set_error (&writer->error,
                 GIMP_CONFIG_ERROR, GIMP_CONFIG_ERROR_WRITE,
		 g_strerror (errno));
  
  g_string_truncate (writer->buffer, 0);
}

static gboolean
gimp_config_writer_close_file (GimpConfigWriter  *writer,
			       GError           **error)
{
  if (! writer->filename)
    return TRUE;

  if (writer->error)
    {
      close (writer->fd);

      if (writer->tmpname)
	unlink (writer->tmpname);

      return TRUE;
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
			   writer->filename, g_strerror (errno));
	    }
	  else
	    {
	      g_set_error (error, GIMP_CONFIG_ERROR, GIMP_CONFIG_ERROR_WRITE,
			   _("Error writing to temporary file for '%s': %s\n"
			     "No file has been created."),
			   writer->filename, g_strerror (errno));
	    }

	  unlink (writer->tmpname);
	}
      else
	{
	  g_set_error (error, GIMP_CONFIG_ERROR, GIMP_CONFIG_ERROR_WRITE,
		       _("Error writing to '%s': %s"),
		       writer->filename, g_strerror (errno));
	}

      return FALSE;
    }

  if (writer->tmpname)
    {
#ifdef G_OS_WIN32
      /* win32 rename can't overwrite */
      unlink (writer->filename);
#endif

      if (rename (writer->tmpname, writer->filename) == -1)
	{
	  g_set_error (error, 
		       GIMP_CONFIG_ERROR, GIMP_CONFIG_ERROR_WRITE,
		       _("Failed to create file '%s': %s"),
		       writer->filename, g_strerror (errno));
	  
	  unlink (writer->tmpname);
	  return FALSE;
	}
    }

  return TRUE;
}
