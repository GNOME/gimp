/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * GimpTextBuffer-serialize
 * Copyright (C) 2010  Michael Natterer <mitch@gimp.org>
 *
 * inspired by
 * gtktextbufferserialize.c
 * Copyright (C) 2004  Nokia Corporation.
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

#include <gtk/gtk.h>

#include "widgets-types.h"

#include "gimptextbuffer.h"
#include "gimptextbuffer-serialize.h"

#include "gimp-intl.h"


/*  serialize  */

static gboolean
open_tag (GimpTextBuffer *buffer,
          GString        *string,
          GtkTextTag     *tag)
{
  const gchar *name;
  const gchar *attribute;
  gchar       *attribute_value;

  name = gimp_text_buffer_tag_to_name (buffer, tag,
                                       &attribute,
                                       &attribute_value);

  if (name)
    {
      if (attribute && attribute_value)
        {
          gchar *escaped = g_markup_escape_text (attribute_value, -1);

          g_string_append_printf (string, "<%s %s=\"%s\">",
                                  name, attribute, escaped);

          g_free (escaped);
          g_free (attribute_value);
        }
      else
        {
          g_string_append_printf (string, "<%s>", name);
        }

      return TRUE;
    }

  return FALSE;
}

static gboolean
close_tag (GimpTextBuffer *buffer,
           GString        *string,
           GtkTextTag     *tag)
{
  const gchar *name = gimp_text_buffer_tag_to_name (buffer, tag, NULL, NULL);

  if (name)
    {
      g_string_append_printf (string, "</%s>", name);

      return TRUE;
    }

  return FALSE;
}

guint8 *
gimp_text_buffer_serialize (GtkTextBuffer     *register_buffer,
                            GtkTextBuffer     *content_buffer,
                            const GtkTextIter *start,
                            const GtkTextIter *end,
                            gsize             *length,
                            gpointer           user_data)
{
  GString     *string;
  GtkTextIter  iter, old_iter;
  GSList      *tag_list;
  GSList      *active_tags;

  string = g_string_new ("<markup>");

  iter        = *start;
  tag_list    = NULL;
  active_tags = NULL;

  do
    {
      GSList *tmp;
      gchar *tmp_text, *escaped_text;

      active_tags = NULL;
      tag_list = gtk_text_iter_get_tags (&iter);

      /* Handle added tags */
      for (tmp = tag_list; tmp; tmp = tmp->next)
        {
          GtkTextTag *tag = tmp->data;

          open_tag (GIMP_TEXT_BUFFER (register_buffer), string, tag);

          active_tags = g_slist_prepend (active_tags, tag);
        }

      g_slist_free (tag_list);
      old_iter = iter;

      /* Now try to go to either the next tag toggle, or if a pixbuf appears */
      while (TRUE)
        {
          gunichar ch = gtk_text_iter_get_char (&iter);

          if (ch == 0xFFFC)
            {
              /* pixbuf? can't happen! */
            }
          else if (ch == 0)
            {
              break;
            }
          else
            {
              gtk_text_iter_forward_char (&iter);
            }

          if (gtk_text_iter_toggles_tag (&iter, NULL))
            break;
        }

      /* We might have moved too far */
      if (gtk_text_iter_compare (&iter, end) > 0)
        iter = *end;

      /* Append the text */
      tmp_text = gtk_text_iter_get_slice (&old_iter, &iter);
      escaped_text = g_markup_escape_text (tmp_text, -1);
      g_free (tmp_text);

      g_string_append (string, escaped_text);
      g_free (escaped_text);

      /* Close any open tags */
      for (tmp = active_tags; tmp; tmp = tmp->next)
        close_tag (GIMP_TEXT_BUFFER (register_buffer), string, tmp->data);

      g_slist_free (active_tags);
    }
  while (! gtk_text_iter_equal (&iter, end));

  g_string_append (string, "</markup>");

  *length = string->len;

  return (guint8 *) g_string_free (string, FALSE);
}


/*  deserialize  */

typedef enum
{
  STATE_START,
  STATE_MARKUP,
  STATE_TAG,
  STATE_UNKNOWN
} ParseState;

typedef struct
{
  GSList        *states;
  GtkTextBuffer *register_buffer;
  GtkTextBuffer *content_buffer;
  GSList        *tag_stack;
  GList         *spans;
} ParseInfo;

typedef struct
{
  gchar  *text;
  GSList *tags;
} TextSpan;

static void set_error (GError              **err,
                       GMarkupParseContext  *context,
                       int                   error_domain,
                       int                   error_code,
                       const char           *format,
                       ...) G_GNUC_PRINTF (5, 6);

static void
set_error (GError              **err,
           GMarkupParseContext  *context,
           int                   error_domain,
           int                   error_code,
           const char           *format,
           ...)
{
  gint     line, ch;
  va_list  args;
  gchar   *str;

  g_markup_parse_context_get_position (context, &line, &ch);

  va_start (args, format);
  str = g_strdup_vprintf (format, args);
  va_end (args);

  g_set_error (err, error_domain, error_code,
               ("Line %d character %d: %s"),
               line, ch, str);

  g_free (str);
}

static void
push_state (ParseInfo  *info,
            ParseState  state)
{
  info->states = g_slist_prepend (info->states, GINT_TO_POINTER (state));
}

static void
pop_state (ParseInfo *info)
{
  g_return_if_fail (info->states != NULL);

  info->states = g_slist_remove (info->states, info->states->data);
}

static ParseState
peek_state (ParseInfo *info)
{
  g_return_val_if_fail (info->states != NULL, STATE_START);

  return GPOINTER_TO_INT (info->states->data);
}

static gboolean
check_no_attributes (GMarkupParseContext  *context,
                     const char           *element_name,
                     const char          **attribute_names,
                     const char          **attribute_values,
                     GError              **error)
{
  if (attribute_names[0] != NULL)
    {
      set_error (error, context,
                 G_MARKUP_ERROR,
                 G_MARKUP_ERROR_PARSE,
                 _("Attribute \"%s\" is invalid on <%s> element in this context"),
                 attribute_names[0], element_name);
      return FALSE;
    }

  return TRUE;
}

static void
parse_tag_element (GMarkupParseContext  *context,
                   const gchar          *element_name,
                   const gchar         **attribute_names,
                   const gchar         **attribute_values,
                   ParseInfo            *info,
                   GError              **error)
{
  GtkTextTag  *tag;
  const gchar *attribute_name  = NULL;
  const gchar *attribute_value = NULL;

  gimp_assert (peek_state (info) == STATE_MARKUP ||
               peek_state (info) == STATE_TAG    ||
               peek_state (info) == STATE_UNKNOWN);

  if (attribute_names)
    attribute_name = attribute_names[0];

  if (attribute_values)
    attribute_value = attribute_values[0];

  tag = gimp_text_buffer_name_to_tag (GIMP_TEXT_BUFFER (info->register_buffer),
                                      element_name,
                                      attribute_name, attribute_value);

  if (tag)
    {
      info->tag_stack = g_slist_prepend (info->tag_stack, tag);

      push_state (info, STATE_TAG);
    }
  else
    {
      push_state (info, STATE_UNKNOWN);
    }
}

static void
start_element_handler (GMarkupParseContext  *context,
                       const gchar          *element_name,
                       const gchar         **attribute_names,
                       const gchar         **attribute_values,
                       gpointer              user_data,
                       GError              **error)
{
  ParseInfo *info = user_data;

  switch (peek_state (info))
    {
    case STATE_START:
      if (! strcmp (element_name, "markup"))
        {
          if (! check_no_attributes (context, element_name,
                                     attribute_names, attribute_values,
                                     error))
            return;

          push_state (info, STATE_MARKUP);
          break;
        }
      else
        {
          set_error (error, context, G_MARKUP_ERROR, G_MARKUP_ERROR_PARSE,
                     _("Outermost element in text must be <markup> not <%s>"),
                     element_name);
        }
      break;

    case STATE_MARKUP:
    case STATE_TAG:
    case STATE_UNKNOWN:
      parse_tag_element (context, element_name,
                         attribute_names, attribute_values,
                         info, error);
      break;

    default:
      gimp_assert_not_reached ();
      break;
    }
}

static void
end_element_handler (GMarkupParseContext  *context,
                     const gchar          *element_name,
                     gpointer              user_data,
                     GError              **error)
{
  ParseInfo *info = user_data;

  switch (peek_state (info))
    {
    case STATE_UNKNOWN:
      pop_state (info);
      gimp_assert (peek_state (info) == STATE_UNKNOWN ||
                   peek_state (info) == STATE_TAG     ||
                   peek_state (info) == STATE_MARKUP);
      break;

    case STATE_TAG:
      pop_state (info);
      gimp_assert (peek_state (info) == STATE_UNKNOWN ||
                   peek_state (info) == STATE_TAG     ||
                   peek_state (info) == STATE_MARKUP);

      /* Pop tag */
      info->tag_stack = g_slist_delete_link (info->tag_stack,
                                             info->tag_stack);
      break;

    case STATE_MARKUP:
      pop_state (info);
      gimp_assert (peek_state (info) == STATE_START);

      info->spans = g_list_reverse (info->spans);
      break;

    default:
      gimp_assert_not_reached ();
      break;
    }
}

static gboolean
all_whitespace (const char *text,
                gint        text_len)
{
  const char *p   = text;
  const char *end = text + text_len;

  while (p != end)
    {
      if (! g_ascii_isspace (*p))
        return FALSE;

      p = g_utf8_next_char (p);
    }

  return TRUE;
}

static void
text_handler (GMarkupParseContext  *context,
              const gchar          *text,
              gsize                 text_len,
              gpointer              user_data,
              GError              **error)
{
  ParseInfo *info = user_data;
  TextSpan  *span;

  if (all_whitespace (text, text_len)   &&
      peek_state (info) != STATE_MARKUP &&
      peek_state (info) != STATE_TAG    &&
      peek_state (info) != STATE_UNKNOWN)
    return;

  switch (peek_state (info))
    {
    case STATE_START:
      gimp_assert_not_reached (); /* gmarkup shouldn't do this */
      break;

    case STATE_MARKUP:
    case STATE_TAG:
    case STATE_UNKNOWN:
      if (text_len == 0)
        return;

      span = g_new0 (TextSpan, 1);
      span->text = g_strndup (text, text_len);
      span->tags = g_slist_copy (info->tag_stack);

      info->spans = g_list_prepend (info->spans, span);
      break;

    default:
      gimp_assert_not_reached ();
      break;
    }
}

static void
parse_info_init (ParseInfo     *info,
                 GtkTextBuffer *register_buffer,
                 GtkTextBuffer *content_buffer)
{
  info->states          = g_slist_prepend (NULL, GINT_TO_POINTER (STATE_START));
  info->tag_stack       = NULL;
  info->spans           = NULL;
  info->register_buffer = register_buffer;
  info->content_buffer  = content_buffer;
}

static void
text_span_free (TextSpan *span)
{
  g_free (span->text);
  g_slist_free (span->tags);
  g_free (span);
}

static void
parse_info_free (ParseInfo *info)
{
  g_slist_free (info->tag_stack);
  g_slist_free (info->states);

  g_list_free_full (info->spans, (GDestroyNotify) text_span_free);
}

static void
insert_text (ParseInfo   *info,
             GtkTextIter *iter)
{
  GtkTextIter  start_iter;
  GtkTextMark *mark;
  GList       *tmp;
  GSList      *tags;

  start_iter = *iter;

  mark = gtk_text_buffer_create_mark (info->content_buffer,
                                      "deserialize-insert-point",
                                      &start_iter, TRUE);

  for (tmp = info->spans; tmp; tmp = tmp->next)
    {
      TextSpan *span = tmp->data;

      if (span->text)
        gtk_text_buffer_insert (info->content_buffer, iter, span->text, -1);

      gtk_text_buffer_get_iter_at_mark (info->content_buffer, &start_iter, mark);

      /* Apply tags */
      for (tags = span->tags; tags; tags = tags->next)
        {
          GtkTextTag *tag = tags->data;

          gtk_text_buffer_apply_tag (info->content_buffer, tag,
                                     &start_iter, iter);
        }

      gtk_text_buffer_move_mark (info->content_buffer, mark, iter);
    }

  gtk_text_buffer_delete_mark (info->content_buffer, mark);
}

gboolean
gimp_text_buffer_deserialize (GtkTextBuffer *register_buffer,
                              GtkTextBuffer *content_buffer,
                              GtkTextIter   *iter,
                              const guint8  *text,
                              gsize          length,
                              gboolean       create_tags,
                              gpointer       user_data,
                              GError       **error)
{
  GMarkupParseContext *context;
  ParseInfo            info;
  gboolean             retval = FALSE;

  static const GMarkupParser markup_parser =
  {
    start_element_handler,
    end_element_handler,
    text_handler,
    NULL,
    NULL
  };

  parse_info_init (&info, register_buffer, content_buffer);

  context = g_markup_parse_context_new (&markup_parser, 0, &info, NULL);

  if (! g_markup_parse_context_parse (context,
                                      (const gchar *) text,
                                      length,
                                      error))
    goto out;

  if (! g_markup_parse_context_end_parse (context, error))
    goto out;

  retval = TRUE;

  insert_text (&info, iter);

 out:
  parse_info_free (&info);

  g_markup_parse_context_free (context);

  return retval;
}

void
gimp_text_buffer_pre_serialize (GimpTextBuffer *buffer,
                                GtkTextBuffer  *content)
{
  GtkTextIter iter;

  g_return_if_fail (GIMP_IS_TEXT_BUFFER (buffer));
  g_return_if_fail (GTK_IS_TEXT_BUFFER (content));

  gtk_text_buffer_get_start_iter (content, &iter);

  do
    {
      GSList *tags = gtk_text_iter_get_tags (&iter);
      GSList *list;

      for (list = tags; list; list = g_slist_next (list))
        {
          GtkTextTag *tag = list->data;

          if (g_list_find (buffer->kerning_tags, tag))
            {
              GtkTextIter end;

              gtk_text_buffer_insert_with_tags (content, &iter,
                                                WORD_JOINER, -1,
                                                tag, NULL);

              end = iter;
              gtk_text_iter_forward_char (&end);

              gtk_text_buffer_remove_tag (content, tag, &iter, &end);
              break;
            }
        }

      g_slist_free (tags);
    }
  while (gtk_text_iter_forward_char (&iter));
}

void
gimp_text_buffer_post_deserialize (GimpTextBuffer *buffer,
                                   GtkTextBuffer  *content)
{
  GtkTextIter iter;

  g_return_if_fail (GIMP_IS_TEXT_BUFFER (buffer));
  g_return_if_fail (GTK_IS_TEXT_BUFFER (content));

  gtk_text_buffer_get_start_iter (content, &iter);

  do
    {
      GSList *tags = gtk_text_iter_get_tags (&iter);
      GSList *list;

      for (list = tags; list; list = g_slist_next (list))
        {
          GtkTextTag *tag = list->data;

          if (g_list_find (buffer->kerning_tags, tag))
            {
              GtkTextIter end;

              gtk_text_iter_forward_char (&iter);
              gtk_text_buffer_backspace (content, &iter, FALSE, TRUE);

              end = iter;
              gtk_text_iter_forward_char (&end);

              gtk_text_buffer_apply_tag (content, tag, &iter, &end);
              break;
            }
        }

      g_slist_free (tags);
    }
  while (gtk_text_iter_forward_char (&iter));
}
