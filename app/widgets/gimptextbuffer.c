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

#include <stdlib.h>
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
#include "gimptextbuffer-serialize.h"

#include "gimp-intl.h"


/*  local function prototypes  */

static GObject * gimp_text_buffer_constructor (GType                  type,
                                               guint                  n_params,
                                               GObjectConstructParam *params);
static void      gimp_text_buffer_dispose     (GObject               *object);
static void      gimp_text_buffer_finalize    (GObject               *object);

static void      gimp_text_buffer_mark_set    (GtkTextBuffer         *buffer,
                                               const GtkTextIter     *location,
                                               GtkTextMark           *mark);


G_DEFINE_TYPE (GimpTextBuffer, gimp_text_buffer, GTK_TYPE_TEXT_BUFFER)

#define parent_class gimp_text_buffer_parent_class


static void
gimp_text_buffer_class_init (GimpTextBufferClass *klass)
{
  GObjectClass       *object_class = G_OBJECT_CLASS (klass);
  GtkTextBufferClass *buffer_class = GTK_TEXT_BUFFER_CLASS (klass);

  object_class->constructor = gimp_text_buffer_constructor;
  object_class->dispose     = gimp_text_buffer_dispose;
  object_class->finalize    = gimp_text_buffer_finalize;

  buffer_class->mark_set    = gimp_text_buffer_mark_set;
}

static void
gimp_text_buffer_init (GimpTextBuffer *buffer)
{
  buffer->markup_atom =
    gtk_text_buffer_register_serialize_format (GTK_TEXT_BUFFER (buffer),
                                               "application/x-gimp-pango-markup",
                                               gimp_text_buffer_serialize,
                                               NULL, NULL);

  gtk_text_buffer_register_deserialize_format (GTK_TEXT_BUFFER (buffer),
                                               "application/x-gimp-pango-markup",
                                               gimp_text_buffer_deserialize,
                                               NULL, NULL);
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

  buffer->bold_tag = gtk_text_buffer_create_tag (GTK_TEXT_BUFFER (buffer),
                                                 "bold",
                                                 "weight", PANGO_WEIGHT_BOLD,
                                                 NULL);

  buffer->italic_tag = gtk_text_buffer_create_tag (GTK_TEXT_BUFFER (buffer),
                                                   "italic",
                                                   "style", PANGO_STYLE_ITALIC,
                                                   NULL);

  buffer->underline_tag = gtk_text_buffer_create_tag (GTK_TEXT_BUFFER (buffer),
                                                      "underline",
                                                      "underline", PANGO_UNDERLINE_SINGLE,
                                                      NULL);

  buffer->strikethrough_tag = gtk_text_buffer_create_tag (GTK_TEXT_BUFFER (buffer),
                                                          "strikethrough",
                                                          "strikethrough", TRUE,
                                                          NULL);

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
  GimpTextBuffer *buffer = GIMP_TEXT_BUFFER (object);

  if (buffer->baseline_tags)
    {
      g_list_free (buffer->baseline_tags);
      buffer->baseline_tags = NULL;
    }

  if (buffer->spacing_tags)
    {
      g_list_free (buffer->spacing_tags);
      buffer->spacing_tags = NULL;
    }

  gimp_text_buffer_clear_insert_tags (buffer);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
gimp_text_buffer_mark_set (GtkTextBuffer     *buffer,
                           const GtkTextIter *location,
                           GtkTextMark       *mark)
{
  gimp_text_buffer_clear_insert_tags (GIMP_TEXT_BUFFER (buffer));

  GTK_TEXT_BUFFER_CLASS (parent_class)->mark_set (buffer, location, mark);
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

  gimp_text_buffer_clear_insert_tags (buffer);
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

void
gimp_text_buffer_set_markup (GimpTextBuffer *buffer,
                             const gchar    *markup)
{
  g_return_if_fail (GIMP_IS_TEXT_BUFFER (buffer));

  gimp_text_buffer_set_text (buffer, NULL);

  if (markup)
    {
      GtkTextIter  start;
      GError      *error = NULL;

      gtk_text_buffer_get_start_iter (GTK_TEXT_BUFFER (buffer), &start);

      if (! gtk_text_buffer_deserialize (GTK_TEXT_BUFFER (buffer),
                                         GTK_TEXT_BUFFER (buffer),
                                         buffer->markup_atom,
                                         &start,
                                         (const guint8 *) markup, -1,
                                         &error))
        {
          g_printerr ("EEK: %s\n", error->message);
          g_clear_error (&error);
        }
    }

  gimp_text_buffer_clear_insert_tags (buffer);
}

gchar *
gimp_text_buffer_get_markup (GimpTextBuffer *buffer)
{
  GtkTextIter start, end;
  gsize       length;

  g_return_val_if_fail (GIMP_IS_TEXT_BUFFER (buffer), NULL);

  gtk_text_buffer_get_bounds (GTK_TEXT_BUFFER (buffer), &start, &end);

  return (gchar *) gtk_text_buffer_serialize (GTK_TEXT_BUFFER (buffer),
                                              GTK_TEXT_BUFFER (buffer),
                                              buffer->markup_atom,
                                              &start, &end,
                                              &length);
}

static gint
get_baseline_at_iter (GimpTextBuffer     *buffer,
                      const GtkTextIter  *iter,
                      GtkTextTag        **baseline_tag)
{
  GList *list;

  for (list = buffer->baseline_tags; list; list = g_list_next (list))
    {
      GtkTextTag *tag = list->data;

      if (gtk_text_iter_has_tag (iter, tag))
        {
          gint baseline;

          *baseline_tag = tag;

          g_object_get (tag,
                        "rise", &baseline,
                        NULL);

          return baseline;
        }
    }

  *baseline_tag = NULL;

  return 0;
}

static GtkTextTag *
get_baseline_tag (GimpTextBuffer *buffer,
                  gint            baseline)
{
  GList      *list;
  GtkTextTag *tag;
  gchar       name[32];

  for (list = buffer->baseline_tags; list; list = g_list_next (list))
    {
      gint tag_baseline;

      tag = list->data;

      g_object_get (tag,
                    "rise", &tag_baseline,
                    NULL);

      if (tag_baseline == baseline)
        return tag;
    }

  g_snprintf (name, sizeof (name), "baseline-%d", baseline);

  tag = gtk_text_buffer_create_tag (GTK_TEXT_BUFFER (buffer),
                                    name,
                                    "rise", baseline,
                                    NULL);

  buffer->baseline_tags = g_list_prepend (buffer->baseline_tags, tag);

  return tag;
}

void
gimp_text_buffer_change_baseline (GimpTextBuffer    *buffer,
                                  const GtkTextIter *start,
                                  const GtkTextIter *end,
                                  gint               count)
{
  GtkTextIter  iter;
  GtkTextIter  span_start;
  GtkTextIter  span_end;
  gint         span_baseline;
  GtkTextTag  *span_tag;

  g_return_if_fail (GIMP_IS_TEXT_BUFFER (buffer));
  g_return_if_fail (start != NULL);
  g_return_if_fail (end != NULL);

  if (gtk_text_iter_equal (start, end))
    return;

  iter          = *start;
  span_start    = *start;
  span_baseline = get_baseline_at_iter (buffer, &iter, &span_tag);

  gtk_text_buffer_begin_user_action (GTK_TEXT_BUFFER (buffer));

  do
    {
      gint        iter_baseline;
      GtkTextTag *iter_tag;

      gtk_text_iter_forward_char (&iter);

      iter_baseline = get_baseline_at_iter (buffer, &iter, &iter_tag);

      span_end = iter;

      if (iter_baseline != span_baseline ||
          gtk_text_iter_compare (&iter, end) >= 0)
        {
          if (span_baseline != 0)
            {
              gtk_text_buffer_remove_tag (GTK_TEXT_BUFFER (buffer), span_tag,
                                          &span_start, &span_end);
            }

          if (span_baseline + count != 0)
            {
              span_tag = get_baseline_tag (buffer, span_baseline + count);

              gtk_text_buffer_apply_tag (GTK_TEXT_BUFFER (buffer), span_tag,
                                         &span_start, &span_end);
            }

          span_start    = iter;
          span_baseline = iter_baseline;
          span_tag      = iter_tag;
        }

      /* We might have moved too far */
      if (gtk_text_iter_compare (&iter, end) > 0)
        iter = *end;
    }
  while (! gtk_text_iter_equal (&iter, end));

  gtk_text_buffer_end_user_action (GTK_TEXT_BUFFER (buffer));
}

static gint
get_spacing_at_iter (GimpTextBuffer     *buffer,
                     const GtkTextIter  *iter,
                     GtkTextTag        **spacing_tag)
{
  GList *list;

  for (list = buffer->spacing_tags; list; list = g_list_next (list))
    {
      GtkTextTag *tag = list->data;

      if (gtk_text_iter_has_tag (iter, tag))
        {
          gint spacing;

          *spacing_tag = tag;

          g_object_get (tag,
                        "rise", &spacing, /* FIXME */
                        NULL);

          return spacing;
        }
    }

  *spacing_tag = NULL;

  return 0;
}

static GtkTextTag *
get_spacing_tag (GimpTextBuffer *buffer,
                 gint            spacing)
{
  GList      *list;
  GtkTextTag *tag;
  gchar       name[32];

  for (list = buffer->spacing_tags; list; list = g_list_next (list))
    {
      gint tag_spacing;

      tag = list->data;

      g_object_get (tag,
                    "rise", &tag_spacing, /* FIXME */
                    NULL);

      if (tag_spacing == spacing)
        return tag;
    }

  g_snprintf (name, sizeof (name), "spacing-%d", spacing);

  tag = gtk_text_buffer_create_tag (GTK_TEXT_BUFFER (buffer),
                                    name,
                                    "rise", spacing, /* FIXME */
                                    NULL);

  buffer->spacing_tags = g_list_prepend (buffer->spacing_tags, tag);

  return tag;
}

void
gimp_text_buffer_change_spacing (GimpTextBuffer    *buffer,
                                 const GtkTextIter *start,
                                 const GtkTextIter *end,
                                 gint               count)
{
  GtkTextIter  iter;
  GtkTextIter  span_start;
  GtkTextIter  span_end;
  gint         span_spacing;
  GtkTextTag  *span_tag;

  g_return_if_fail (GIMP_IS_TEXT_BUFFER (buffer));
  g_return_if_fail (start != NULL);
  g_return_if_fail (end != NULL);

  if (gtk_text_iter_equal (start, end))
    return;

  iter         = *start;
  span_start   = *start;
  span_spacing = get_spacing_at_iter (buffer, &iter, &span_tag);

  gtk_text_buffer_begin_user_action (GTK_TEXT_BUFFER (buffer));

  do
    {
      gint        iter_spacing;
      GtkTextTag *iter_tag;

      gtk_text_iter_forward_char (&iter);

      iter_spacing = get_spacing_at_iter (buffer, &iter, &iter_tag);

      span_end = iter;

      if (iter_spacing != span_spacing ||
          gtk_text_iter_compare (&iter, end) >= 0)
        {
          if (span_spacing != 0)
            {
              gtk_text_buffer_remove_tag (GTK_TEXT_BUFFER (buffer), span_tag,
                                          &span_start, &span_end);
            }

          if (span_spacing + count != 0)
            {
              span_tag = get_spacing_tag (buffer, span_spacing + count);

              gtk_text_buffer_apply_tag (GTK_TEXT_BUFFER (buffer), span_tag,
                                         &span_start, &span_end);
            }

          span_start   = iter;
          span_spacing = iter_spacing;
          span_tag     = iter_tag;
        }

      /* We might have moved too far */
      if (gtk_text_iter_compare (&iter, end) > 0)
        iter = *end;
    }
  while (! gtk_text_iter_equal (&iter, end));

  gtk_text_buffer_end_user_action (GTK_TEXT_BUFFER (buffer));
}

const gchar *
gimp_text_buffer_tag_to_name (GimpTextBuffer  *buffer,
                              GtkTextTag      *tag,
                              const gchar    **attribute,
                              gchar          **value)
{
  g_return_val_if_fail (GIMP_IS_TEXT_BUFFER (buffer), NULL);
  g_return_val_if_fail (GTK_IS_TEXT_TAG (tag), NULL);

  if (attribute)
    *attribute = NULL;

  if (value)
    *value = NULL;

  if (tag == buffer->bold_tag)
    {
      return "b";
    }
  else if (tag == buffer->italic_tag)
    {
      return "i";
    }
  else if (tag == buffer->underline_tag)
    {
      return "u";
    }
  else if (tag == buffer->strikethrough_tag)
    {
      return "s";
    }
  else if (g_list_find (buffer->baseline_tags, tag))
    {
      if (attribute)
        *attribute = "rise";

      if (value)
        {
          gint baseline;

          g_object_get (tag,
                        "rise", &baseline,
                        NULL);

          *value = g_strdup_printf ("%d", baseline);
        }

      return "span";
    }
  else if (g_list_find (buffer->spacing_tags, tag))
    {
      if (attribute)
        *attribute = "letter_spacing";

      if (value)
        {
          gint spacing;

          g_object_get (tag,
                        "rise", &spacing, /* FIXME */
                        NULL);

          *value = g_strdup_printf ("%d", spacing);
        }

      return "span";
    }

  return NULL;
}

GtkTextTag *
gimp_text_buffer_name_to_tag (GimpTextBuffer *buffer,
                              const gchar    *name,
                              const gchar    *attribute,
                              const gchar    *value)
{
  g_return_val_if_fail (GIMP_IS_TEXT_BUFFER (buffer), NULL);
  g_return_val_if_fail (name != NULL, NULL);

  if (! strcmp (name, "b"))
    {
      return buffer->bold_tag;
    }
  else if (! strcmp (name, "i"))
    {
      return buffer->italic_tag;
    }
  else if (! strcmp (name, "u"))
    {
      return buffer->underline_tag;
    }
  else if (! strcmp (name, "s"))
    {
      return buffer->strikethrough_tag;
    }
  else if (! strcmp (name, "span") &&
           attribute != NULL       &&
           value     != NULL)
    {
      if (! strcmp (attribute, "rise"))
        {
          return get_baseline_tag (buffer, atoi (value));
        }
      else if (! strcmp (attribute, "letter_spacing"))
        {
          return get_spacing_tag (buffer, atoi (value));
        }
    }

  return NULL;
}

void
gimp_text_buffer_set_insert_tags (GimpTextBuffer *buffer,
                                  GList          *style)
{
  g_return_if_fail (GIMP_IS_TEXT_BUFFER (buffer));

  g_list_free (buffer->insert_tags);
  buffer->insert_tags = style;
  buffer->insert_tags_set = TRUE;
}

void
gimp_text_buffer_clear_insert_tags (GimpTextBuffer *buffer)
{
  g_return_if_fail (GIMP_IS_TEXT_BUFFER (buffer));

  g_list_free (buffer->insert_tags);
  buffer->insert_tags = NULL;
  buffer->insert_tags_set = FALSE;
}

void
gimp_text_buffer_insert (GimpTextBuffer *buffer,
                         const gchar    *text)
{
  GtkTextIter  iter, start;
  gint         start_offset;
  GList       *insert_tags;
  gboolean     insert_tags_set;
  GSList      *tags_off = NULL;

  g_return_if_fail (GIMP_IS_TEXT_BUFFER (buffer));

  gtk_text_buffer_get_iter_at_mark (GTK_TEXT_BUFFER (buffer), &iter,
                                    gtk_text_buffer_get_insert (GTK_TEXT_BUFFER (buffer)));

  start_offset = gtk_text_iter_get_offset (&iter);

  insert_tags     = buffer->insert_tags;
  insert_tags_set = buffer->insert_tags_set;
  buffer->insert_tags     = NULL;
  buffer->insert_tags_set = FALSE;

  if (! insert_tags_set)
    tags_off = gtk_text_iter_get_toggled_tags (&iter, FALSE);

  gtk_text_buffer_begin_user_action (GTK_TEXT_BUFFER (buffer));

  gtk_text_buffer_insert (GTK_TEXT_BUFFER (buffer), &iter, text, -1);

  gtk_text_buffer_get_iter_at_offset (GTK_TEXT_BUFFER (buffer), &start,
                                      start_offset);

  if (insert_tags_set)
    {
      GList *list;

      gtk_text_buffer_remove_all_tags (GTK_TEXT_BUFFER (buffer),
                                       &start, &iter);

      for (list = insert_tags; list; list = g_list_next (list))
        {
          gtk_text_buffer_apply_tag (GTK_TEXT_BUFFER (buffer), list->data,
                                     &start, &iter);
        }

      g_list_free (insert_tags);
    }
  else
    {
      GSList *list;

      for (list = tags_off; list; list = g_slist_next (list))
        {
          gtk_text_buffer_apply_tag (GTK_TEXT_BUFFER (buffer), list->data,
                                     &start, &iter);
        }

      g_slist_free (tags_off);
    }

  gtk_text_buffer_end_user_action (GTK_TEXT_BUFFER (buffer));
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

  gtk_text_buffer_begin_user_action (GTK_TEXT_BUFFER (buffer));

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

  gtk_text_buffer_end_user_action (GTK_TEXT_BUFFER (buffer));

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
