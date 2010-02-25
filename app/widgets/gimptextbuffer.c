/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * GimpTextBuffer
 * Copyright (C) 2010  Michael Natterer <mitch@gimp.org>
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
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <sys/types.h>

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include <glib/gstdio.h>

#include <glib.h>

#ifdef G_OS_WIN32
#include "libgimpbase/gimpwin32-io.h"
#endif

#include <gegl.h>
#include <gtk/gtk.h>

#include "libgimpbase/gimpbase.h"

#include "widgets-types.h"

#include "gimptextbuffer.h"

#include "gimp-intl.h"


/*  local function prototypes  */

static GObject * gimp_text_buffer_constructor (GType                  type,
                                               guint                  n_params,
                                               GObjectConstructParam *params);
static void      gimp_text_buffer_dispose     (GObject               *object);
static void      gimp_text_buffer_finalize    (GObject               *object);


G_DEFINE_TYPE (GimpTextBuffer, gimp_text_buffer, GTK_TYPE_TEXT_BUFFER)

#define parent_class gimp_text_buffer_parent_class


static void
gimp_text_buffer_class_init (GimpTextBufferClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->constructor = gimp_text_buffer_constructor;
  object_class->dispose     = gimp_text_buffer_dispose;
  object_class->finalize    = gimp_text_buffer_finalize;
}

static void
gimp_text_buffer_init (GimpTextBuffer *buffer)
{
}

static GObject *
gimp_text_buffer_constructor (GType                  type,
                              guint                  n_params,
                              GObjectConstructParam *params)
{
  GObject        *object;
  GimpTextBuffer *buffer;

  object = G_OBJECT_CLASS (parent_class)->constructor (type, n_params, params);

  buffer = GIMP_TEXT_BUFFER (object);

  gtk_text_buffer_set_text (GTK_TEXT_BUFFER (buffer), "", -1);

  return object;
}

static void
gimp_text_buffer_dispose (GObject *object)
{
  /* GimpTextBuffer *buffer = GIMP_TEXT_BUFFER (object); */

  G_OBJECT_CLASS (parent_class)->dispose (object);
}

static void
gimp_text_buffer_finalize (GObject *object)
{
  /* GimpTextBuffer *buffer = GIMP_TEXT_BUFFER (object); */

  G_OBJECT_CLASS (parent_class)->finalize (object);
}


/*  public functions  */

GimpTextBuffer *
gimp_text_buffer_new (void)
{
  return g_object_new (GIMP_TYPE_TEXT_BUFFER, NULL);
}

void
gimp_text_buffer_set_text (GimpTextBuffer *buffer,
                           const gchar    *text)
{
  g_return_if_fail (GIMP_IS_TEXT_BUFFER (buffer));

  if (text == NULL)
    text = "";

  gtk_text_buffer_set_text (GTK_TEXT_BUFFER (buffer), text, -1);
}

gchar *
gimp_text_buffer_get_text (GimpTextBuffer *buffer)
{
  GtkTextIter start, end;

  g_return_val_if_fail (GIMP_IS_TEXT_BUFFER (buffer), NULL);

  gtk_text_buffer_get_bounds (GTK_TEXT_BUFFER (buffer), &start, &end);

  return gtk_text_buffer_get_text (GTK_TEXT_BUFFER (buffer),
                                   &start, &end, TRUE);
}

gint
gimp_text_buffer_get_iter_index (GimpTextBuffer *buffer,
                                 GtkTextIter    *iter)
{
  GtkTextIter  start;
  gchar       *string;
  gint         index;

  g_return_val_if_fail (GIMP_IS_TEXT_BUFFER (buffer), 0);

  gtk_text_buffer_get_start_iter (GTK_TEXT_BUFFER (buffer), &start);

  string = gtk_text_buffer_get_text (GTK_TEXT_BUFFER (buffer),
                                     &start, iter, TRUE);
  index = strlen (string);
  g_free (string);

  return index;
}

gboolean
gimp_text_buffer_load (GimpTextBuffer *buffer,
                       const gchar    *filename,
                       GError        **error)
{
  FILE        *file;
  gchar        buf[2048];
  gint         remaining = 0;
  GtkTextIter  iter;

  g_return_val_if_fail (GIMP_IS_TEXT_BUFFER (buffer), FALSE);
  g_return_val_if_fail (filename != NULL, FALSE);
  g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

  file = g_fopen (filename, "r");

  if (! file)
    {
      g_set_error_literal (error, G_FILE_ERROR,
                           g_file_error_from_errno (errno),
                           g_strerror (errno));
      return FALSE;
    }

  gimp_text_buffer_set_text (buffer, NULL);
  gtk_text_buffer_get_end_iter (GTK_TEXT_BUFFER (buffer), &iter);

  while (! feof (file))
    {
      const char *leftover;
      gint        count;
      gint        to_read = sizeof (buf) - remaining - 1;

      count = fread (buf + remaining, 1, to_read, file);
      buf[count + remaining] = '\0';

      g_utf8_validate (buf, count + remaining, &leftover);

      gtk_text_buffer_insert (GTK_TEXT_BUFFER (buffer), &iter,
                              buf, leftover - buf);
      gtk_text_buffer_get_end_iter (GTK_TEXT_BUFFER (buffer), &iter);

      remaining = (buf + remaining + count) - leftover;
      g_memmove (buf, leftover, remaining);

      if (remaining > 6 || count < to_read)
        break;
    }

  if (remaining)
    g_message (_("Invalid UTF-8 data in file '%s'."),
               gimp_filename_to_utf8 (filename));

  fclose (file);

  return TRUE;
}

gboolean
gimp_text_buffer_save (GimpTextBuffer *buffer,
                       const gchar    *filename,
                       gboolean        selection_only,
                       GError        **error)
{
  GtkTextIter  start_iter;
  GtkTextIter  end_iter;
  gint         fd;
  gchar       *text_contents;

  g_return_val_if_fail (GIMP_IS_TEXT_BUFFER (buffer), FALSE);
  g_return_val_if_fail (filename != NULL, FALSE);
  g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

  fd = g_open (filename, O_WRONLY | O_CREAT | O_APPEND, 0666);

  if (fd == -1)
    {
      g_set_error_literal (error, G_FILE_ERROR,
                           g_file_error_from_errno (errno),
                           g_strerror (errno));
      return FALSE;
    }

  if (selection_only)
    gtk_text_buffer_get_selection_bounds (GTK_TEXT_BUFFER (buffer),
                                          &start_iter, &end_iter);
  else
    gtk_text_buffer_get_bounds (GTK_TEXT_BUFFER (buffer),
                                &start_iter, &end_iter);

  text_contents = gtk_text_buffer_get_text (GTK_TEXT_BUFFER (buffer),
                                            &start_iter, &end_iter, TRUE);

  if (text_contents)
    {
      gint text_length = strlen (text_contents);

      if (text_length > 0)
        {
          gint bytes_written;

          bytes_written = write (fd, text_contents, text_length);

          if (bytes_written != text_length)
            {
              g_free (text_contents);
              close (fd);
              g_set_error_literal (error, G_FILE_ERROR,
                                   g_file_error_from_errno (errno),
                                   g_strerror (errno));
              return FALSE;
            }
        }

      g_free (text_contents);
    }

  close (fd);

  return TRUE;
}
