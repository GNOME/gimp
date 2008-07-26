/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimptagentry.c
 * Copyright (C) 2008 Aurimas Juška <aurisj@svn.gnome.org>
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

#include <string.h>

#include <gtk/gtk.h>

#include "widgets-types.h"

#include "core/gimpcontainer.h"
#include "core/gimpfilteredcontainer.h"
#include "core/gimpcontext.h"
#include "core/gimpviewable.h"
#include "core/gimptag.h"
#include "core/gimptagged.h"

#include "gimptagentry.h"

#include "gimp-intl.h"

#define TAG_SEPARATOR_STR   ","
#define GIMP_TAG_ENTRY_QUERY_DESC       _("filter")
#define GIMP_TAG_ENTRY_ASSIGN_DESC      _("enter tags")

static void     gimp_tag_entry_dispose                   (GObject              *object);
static void     gimp_tag_entry_activate                  (GtkEntry             *entry,
                                                          gpointer              unused);
static void     gimp_tag_entry_changed                   (GtkEntry             *entry,
                                                          gpointer              unused);
static void     gimp_tag_entry_insert_text               (GtkEditable          *editable,
                                                          gchar                *new_text,
                                                          gint                  text_length,
                                                          gint                 *position,
                                                          gpointer              user_data);
static gboolean gimp_tag_entry_focus_in                  (GtkWidget            *widget,
                                                          GdkEventFocus        *event,
                                                          gpointer              user_data);
static gboolean gimp_tag_entry_focus_out                 (GtkWidget            *widget,
                                                          GdkEventFocus        *event,
                                                          gpointer              user_data);
static gboolean gimp_tag_entry_button_release            (GtkWidget            *widget,
                                                          GdkEventButton       *event);
static void     gimp_tag_entry_backspace                 (GtkEntry             *entry);
static void     gimp_tag_entry_delete_from_cursor        (GtkEntry             *entry,
                                                          GtkDeleteType         delete_type,
                                                          gint                  count);
static void     gimp_tag_entry_query_tag                 (GimpTagEntry         *entry);

static void     gimp_tag_entry_assign_tags               (GimpTagEntry         *tag_entry);
static void     gimp_tag_entry_item_set_tags             (GimpTagged           *entry,
                                                          GList                *tags);

static void     gimp_tag_entry_load_selection            (GimpTagEntry         *tag_entry);


static gchar*   gimp_tag_entry_get_completion_prefix     (GimpTagEntry         *entry);
static GList *  gimp_tag_entry_get_completion_candidates (GimpTagEntry         *tag_entry,
                                                          gchar               **used_tags,
                                                          gchar                *prefix);
static gchar *  gimp_tag_entry_get_completion_string     (GimpTagEntry         *tag_entry,
                                                          GList                *candidates,
                                                          gchar                *prefix);
static gboolean gimp_tag_entry_auto_complete             (GimpTagEntry         *tag_entry);

static void     gimp_tag_entry_toggle_desc               (GimpTagEntry         *widget,
                                                          gboolean              show);
static gboolean gimp_tag_entry_expose                    (GtkWidget            *widget,
                                                          GdkEventExpose       *event,
                                                          gpointer              user_data);

static gboolean gimp_tag_entry_select_jellybean          (GimpTagEntry         *entry);
static gboolean gimp_tag_entry_try_select_jellybean      (GimpTagEntry         *tag_entry);


G_DEFINE_TYPE (GimpTagEntry, gimp_tag_entry, GTK_TYPE_ENTRY);

#define parent_class gimp_tag_entry_parent_class


static void
gimp_tag_entry_class_init (GimpTagEntryClass *klass)
{
  GObjectClass         *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass       *widget_class = GTK_WIDGET_CLASS (klass);
  GtkEntryClass        *entry_class  = GTK_ENTRY_CLASS (klass);

  object_class->dispose                 = gimp_tag_entry_dispose;

  widget_class->button_release_event    = gimp_tag_entry_button_release;

  entry_class->backspace                = gimp_tag_entry_backspace;
  entry_class->delete_from_cursor       = gimp_tag_entry_delete_from_cursor;
}

static void
gimp_tag_entry_init (GimpTagEntry *entry)
{
  entry->tagged_container      = NULL;
  entry->selected_items        = NULL;
  entry->mode                  = GIMP_TAG_ENTRY_MODE_QUERY;
  entry->description_shown     = FALSE;

  g_signal_connect (entry, "activate",
                    G_CALLBACK (gimp_tag_entry_activate),
                    NULL);
  g_signal_connect (entry, "changed",
                    G_CALLBACK (gimp_tag_entry_changed),
                    NULL);
  g_signal_connect (entry, "insert-text",
                    G_CALLBACK (gimp_tag_entry_insert_text),
                    NULL);
  g_signal_connect (entry, "focus-in-event",
                    G_CALLBACK (gimp_tag_entry_focus_in),
                    NULL);
  g_signal_connect (entry, "focus-out-event",
                    G_CALLBACK (gimp_tag_entry_focus_out),
                    NULL);
  g_signal_connect_after (entry, "expose-event",
                    G_CALLBACK (gimp_tag_entry_expose),
                    NULL);
}

static void
gimp_tag_entry_dispose (GObject        *object)
{
  GimpTagEntry         *tag_entry = GIMP_TAG_ENTRY (object);

  if (tag_entry->selected_items)
    {
      g_list_free (tag_entry->selected_items);
      tag_entry->selected_items = NULL;
    }

  G_OBJECT_CLASS (parent_class)->dispose (object);
}

GtkWidget *
gimp_tag_entry_new (GimpFilteredContainer      *tagged_container,
                    GimpTagEntryMode            mode)
{
  GimpTagEntry         *entry;

  g_return_val_if_fail (tagged_container == NULL
                        || GIMP_IS_FILTERED_CONTAINER (tagged_container),
                        NULL);

  entry = g_object_new (GIMP_TYPE_TAG_ENTRY, NULL);
  entry->tagged_container       = tagged_container;
  entry->mode                   = mode;

  gimp_tag_entry_toggle_desc (entry, TRUE);

  return GTK_WIDGET (entry);
}

static void
gimp_tag_entry_activate (GtkEntry              *entry,
                         gpointer               unused)
{
  GimpTagEntry         *tag_entry;
  gint                  selection_start;
  gint                  selection_end;

  tag_entry = GIMP_TAG_ENTRY (entry);

  gimp_tag_entry_toggle_desc (tag_entry, FALSE);

  gtk_editable_get_selection_bounds (GTK_EDITABLE (entry),
                                     &selection_start, &selection_end);
  if (selection_start != selection_end)
    {
      gtk_editable_select_region (GTK_EDITABLE (entry),
                                  selection_end, selection_end);
      return;
    }

  if (tag_entry->mode == GIMP_TAG_ENTRY_MODE_ASSIGN)
    {
      gimp_tag_entry_assign_tags (GIMP_TAG_ENTRY (entry));
    }
}

void
gimp_tag_entry_set_tag_string (GimpTagEntry    *tag_entry,
                               const gchar     *tag_string)
{
  tag_entry->internal_change = TRUE;
  gtk_entry_set_text (GTK_ENTRY (tag_entry), tag_string);
  gtk_editable_set_position (GTK_EDITABLE (tag_entry), -1);
  tag_entry->internal_change = FALSE;

  if (tag_entry->mode == GIMP_TAG_ENTRY_MODE_ASSIGN)
    {
      gimp_tag_entry_assign_tags (tag_entry);
    }
}

static void
gimp_tag_entry_changed (GtkEntry          *entry,
                        gpointer           unused)
{
  GimpTagEntry         *tag_entry = GIMP_TAG_ENTRY (entry);
  gchar                *text;

  text = g_strdup (gtk_entry_get_text (entry));
  text = g_strstrip (text);
  if (! GTK_WIDGET_HAS_FOCUS (GTK_WIDGET (entry))
      && strlen (text) == 0)
    {
      gimp_tag_entry_toggle_desc (tag_entry, TRUE);
    }
  else
    {
      gimp_tag_entry_toggle_desc (tag_entry, FALSE);
    }
  g_free (text);

  if (tag_entry->mode == GIMP_TAG_ENTRY_MODE_QUERY)
    {
      gimp_tag_entry_query_tag (GIMP_TAG_ENTRY (entry));
    }
}

static void
gimp_tag_entry_insert_text     (GtkEditable       *editable,
                                gchar             *new_text,
                                gint               text_length,
                                gint              *position,
                                gpointer           user_data)
{
  if (! GIMP_TAG_ENTRY (editable)->internal_change)
    {
      g_idle_add ((GSourceFunc)gimp_tag_entry_auto_complete,
                  editable);
    }
}

static void
gimp_tag_entry_query_tag (GimpTagEntry         *entry)
{
  gchar                       **parsed_tags;
  gint                          count;
  gint                          i;
  GimpTag                       tag;
  GList                        *query_list = NULL;

  parsed_tags = gimp_tag_entry_parse_tags (entry);
  count = g_strv_length (parsed_tags);
  for (i = 0; i < count; i++)
    {
      if (strlen (parsed_tags[i]) > 0)
        {
          tag = g_quark_try_string (parsed_tags[i]);
          query_list = g_list_append (query_list, GUINT_TO_POINTER (tag));
        }
    }
  g_strfreev (parsed_tags);

  gimp_filtered_container_set_filter (GIMP_FILTERED_CONTAINER (entry->tagged_container),
                                      query_list);
}

static gboolean
gimp_tag_entry_auto_complete (GimpTagEntry     *tag_entry)
{
  gchar                *completion_prefix;
  GList                *completion_candidates;
  gchar               **tags;
  gchar                *completion;
  gint                  start_position;
  gint                  end_position;
  GtkEntry             *entry;

  entry = GTK_ENTRY (tag_entry);

  gtk_editable_get_selection_bounds (GTK_EDITABLE (tag_entry), &start_position, &end_position);
  if (start_position != end_position)
    {
      /* only autocomplete what user types,
       * not was autocompleted in the previous step. */
      return FALSE;
    }

  completion_prefix =
      gimp_tag_entry_get_completion_prefix (GIMP_TAG_ENTRY (entry));
  tags = gimp_tag_entry_parse_tags (GIMP_TAG_ENTRY (entry));
  completion_candidates =
      gimp_tag_entry_get_completion_candidates (GIMP_TAG_ENTRY (entry),
                                                tags,
                                                completion_prefix);
  completion =
      gimp_tag_entry_get_completion_string (GIMP_TAG_ENTRY (entry),
                                            completion_candidates,
                                            completion_prefix);

  if (completion
      && strlen (completion) > 0)
    {
      start_position = gtk_editable_get_position (GTK_EDITABLE (entry));
      end_position = start_position;
      gtk_editable_insert_text (GTK_EDITABLE (entry),
                                completion, strlen (completion),
                                &end_position);
      gtk_editable_select_region (GTK_EDITABLE (entry),
                                  start_position, end_position);
    }

  g_free (completion);
  g_strfreev (tags);
  g_list_free (completion_candidates);
  g_free (completion_prefix);

  return FALSE;
}

static void
gimp_tag_entry_assign_tags (GimpTagEntry       *tag_entry)
{
  GList                *selected_iterator = NULL;
  GimpTagged           *selected_item;
  gchar               **parsed_tags;
  gint                  count;
  gint                  i;
  GimpTag               tag;
  GList                *tag_list = NULL;

  parsed_tags = gimp_tag_entry_parse_tags (tag_entry);
  count = g_strv_length (parsed_tags);
  for (i = 0; i < count; i++)
    {
      if (strlen (parsed_tags[i]) > 0)
        {
          tag = g_quark_from_string (parsed_tags[i]);
          tag_list = g_list_append (tag_list, GUINT_TO_POINTER (tag));
        }
    }
  g_strfreev (parsed_tags);

  selected_iterator = tag_entry->selected_items;
  while (selected_iterator)
    {
      selected_item = GIMP_TAGGED (selected_iterator->data);
      gimp_tag_entry_item_set_tags (selected_item, tag_list);
      selected_iterator = g_list_next (selected_iterator);
    }
  g_list_free (tag_list);
}

static void
gimp_tag_entry_item_set_tags (GimpTagged       *tagged,
                              GList            *tags)
{
  GList        *old_tags;
  GList        *tags_iterator;

  old_tags = g_list_copy (gimp_tagged_get_tags (tagged));
  tags_iterator = old_tags;
  while (tags_iterator)
    {
      gimp_tagged_remove_tag (tagged,
                              GPOINTER_TO_UINT (tags_iterator->data));
      tags_iterator = g_list_next (tags_iterator);
    }
  g_list_free (old_tags);

  tags_iterator = tags;
  while (tags_iterator)
    {
      printf ("tagged: %s\n", g_quark_to_string (GPOINTER_TO_UINT (tags_iterator->data)));
      gimp_tagged_add_tag (tagged, GPOINTER_TO_UINT (tags_iterator->data));
      tags_iterator = g_list_next (tags_iterator);
    }
}

gchar **
gimp_tag_entry_parse_tags (GimpTagEntry        *entry)
{
  const gchar          *tag_str;
  gchar               **split_tags;
  gchar               **parsed_tags;
  gint                  length;
  gint                  i;

  tag_str = gtk_entry_get_text (GTK_ENTRY (entry));
  split_tags = g_strsplit (tag_str, ",", 0);
  length = g_strv_length (split_tags);
  parsed_tags = g_malloc ((length + 1) * sizeof (gchar **));
  for (i = 0; i < length; i++)
    {
      parsed_tags[i] = g_strdup (g_strstrip (split_tags[i]));
    }
  parsed_tags[length] = NULL;

  g_strfreev (split_tags);
  return parsed_tags;
}

void
gimp_tag_entry_set_selected_items (GimpTagEntry            *entry,
                                   GList                   *items)
{
  if (entry->selected_items)
    {
      g_list_free (entry->selected_items);
      entry->selected_items = NULL;
    }

  entry->selected_items = g_list_copy (items);

  if (! entry->description_shown)
    {
      gimp_tag_entry_load_selection (entry);
      gimp_tag_entry_toggle_desc (entry, TRUE);
    }
}

static void
gimp_tag_entry_load_selection (GimpTagEntry             *tag_entry)
{
  GimpTagged   *selected_item;
  GList        *tag_list;
  GList        *tag_iterator;
  gint          insert_pos;
  GimpTag       tag;
  gchar        *text;

  gtk_editable_delete_text (GTK_EDITABLE (tag_entry), 0, -1);

  if (! tag_entry->selected_items)
    {
      return;
    }

  selected_item = GIMP_TAGGED (tag_entry->selected_items->data);
  insert_pos = 0;

  tag_list = g_list_copy (gimp_tagged_get_tags (selected_item));
  tag_list = g_list_sort (tag_list, gimp_tag_compare_func);
  tag_iterator = tag_list;
  while (tag_iterator)
    {
      tag = GPOINTER_TO_UINT (tag_iterator->data);
      text = g_strdup_printf ("%s, ", g_quark_to_string (tag));
      gtk_editable_insert_text (GTK_EDITABLE (tag_entry), text, strlen (text),
                                &insert_pos);
      g_free (text);

      tag_iterator = g_list_next (tag_iterator);
    }
  g_list_free (tag_list);
}

static gchar*
gimp_tag_entry_get_completion_prefix (GimpTagEntry             *entry)
{
  gchar        *original_string;
  gchar        *prefix_start;
  gchar        *prefix;
  gchar        *cursor;
  gint          position;
  gint          i;
  gunichar      separator;
  gunichar      c;

  original_string = g_strdup (gtk_entry_get_text (GTK_ENTRY (entry)));
  position = gtk_editable_get_position (GTK_EDITABLE (entry));
  cursor = original_string;
  prefix_start = original_string;
  separator = g_utf8_get_char (TAG_SEPARATOR_STR);
  for (i = 0; i < position; i++)
    {
      c = g_utf8_get_char (cursor);
      cursor = g_utf8_next_char (cursor);
      if (c == separator)
        {
          prefix_start = cursor;
        }
    }
  do
    {
      c = g_utf8_get_char (cursor);
      if (c == separator)
        {
          *cursor = '\0';
          break;
        }
      cursor = g_utf8_next_char (cursor);
    } while (c);

  prefix = g_strdup (g_strstrip (prefix_start));
  g_free (original_string);

  return prefix;
}

static GList *
gimp_tag_entry_get_completion_candidates (GimpTagEntry         *tag_entry,
                                          gchar               **used_tags,
                                          gchar                *prefix)
{
  GList        *candidates = NULL;
  GList        *all_tags;
  GList        *tag_iterator;
  GimpTag       tag;
  gint          i;
  gint          length;

  if (!prefix
      || strlen (prefix) < 1)
    {
      return NULL;
    }

  all_tags = g_hash_table_get_keys (tag_entry->tagged_container->tag_ref_counts);
  tag_iterator = all_tags;
  length = g_strv_length (used_tags);
  while (tag_iterator)
    {
      tag = GPOINTER_TO_UINT (tag_iterator->data);
      if (g_str_has_prefix (g_quark_to_string (tag), prefix))
        {
          /* check if tag is not already entered */
          for (i = 0; i < length; i++)
            {
              if (! strcmp (g_quark_to_string (tag), used_tags[i]))
                {
                  break;
                }
            }

          if (i == length)
            {
              candidates = g_list_append (candidates, tag_iterator->data);
            }
        }
      tag_iterator = g_list_next (tag_iterator);
    }
  g_list_free (all_tags);

  return candidates;
}

static gchar *
gimp_tag_entry_get_completion_string (GimpTagEntry             *tag_entry,
                                      GList                    *candidates,
                                      gchar                    *prefix)
{
  const gchar **completions;
  guint         length;
  guint         i;
  GList        *candidate_iterator;
  const gchar  *candidate_string;
  gint          prefix_length;
  gunichar      c;
  gunichar      d;
  gint          num_chars_match;
  gchar        *completion;
  gchar        *completion_end;
  gint          completion_length;

  if (! candidates)
    {
      return NULL;
    }

  prefix_length = strlen (prefix);
  length = g_list_length (candidates);
  if (length < 2)
    {
      candidate_string = g_quark_to_string (GPOINTER_TO_UINT (candidates->data));
      return g_strdup (candidate_string + prefix_length);
    }

  completions = g_malloc (length * sizeof (gchar*));
  candidate_iterator = candidates;
  for (i = 0; i < length; i++)
    {
      candidate_string =
          g_quark_to_string (GPOINTER_TO_UINT (candidate_iterator->data));
      completions[i] = candidate_string + prefix_length;
      candidate_iterator = g_list_next (candidate_iterator);
    }

  num_chars_match = 0;
  do
    {
      c = g_utf8_get_char (completions[0]);
      if (!c)
        {
          break;
        }

      for (i = 1; i < length; i++)
        {
          d = g_utf8_get_char (completions[i]);
          if (c != d)
            {
              candidate_string = g_quark_to_string (GPOINTER_TO_UINT (candidates->data));
              candidate_string += prefix_length;
              completion_end = g_utf8_offset_to_pointer (candidate_string,
                                                         num_chars_match);
              completion_length = completion_end - candidate_string;
              completion = g_malloc (completion_length + 1);
              memcpy (completion, candidate_string, completion_length);
              completion[completion_length] = '\0';

              g_free (completions);
              return completion;
            }
          completions[i] = g_utf8_next_char (completions[i]);
        }
      completions[0] = g_utf8_next_char (completions[0]);
      num_chars_match++;
    } while (c);
  g_free (completions);

  candidate_string = g_quark_to_string (GPOINTER_TO_UINT (candidates->data));
  return g_strdup (candidate_string + prefix_length);
}

static gboolean
gimp_tag_entry_focus_in        (GtkWidget         *widget,
                                GdkEventFocus     *event,
                                gpointer           user_data)
{
  GimpTagEntry         *tag_entry = GIMP_TAG_ENTRY (widget);
  gimp_tag_entry_toggle_desc (GIMP_TAG_ENTRY (widget), FALSE);
  if (tag_entry->mode == GIMP_TAG_ENTRY_MODE_ASSIGN)
    {
      gimp_tag_entry_load_selection (tag_entry);
    }

  return FALSE;
}

static gboolean
gimp_tag_entry_focus_out       (GtkWidget         *widget,
                                GdkEventFocus     *event,
                                gpointer           user_data)
{
  gimp_tag_entry_toggle_desc (GIMP_TAG_ENTRY (widget), TRUE);
  return FALSE;
}

static void
gimp_tag_entry_toggle_desc     (GimpTagEntry      *tag_entry,
                                gboolean           show)
{
  GtkWidget            *widget = GTK_WIDGET (tag_entry);
  const gchar          *display_text;

  if (! (show ^ tag_entry->description_shown))
    {
      return;
    }

  if (tag_entry->mode == GIMP_TAG_ENTRY_MODE_QUERY)
    {
      display_text = GIMP_TAG_ENTRY_QUERY_DESC;
    }
  else
    {
      display_text = GIMP_TAG_ENTRY_ASSIGN_DESC;
    }

  if (show)
    {
      gchar           **tags;
      gint              tag_count;
      gint              i;
      gboolean          has_valid_tag = FALSE;

      tags = gimp_tag_entry_parse_tags (tag_entry);
      tag_count = g_strv_length (tags);
      for (i = 0; i < tag_count; i++)
        {
          if (tags[i] && *tags[i])
            {
              has_valid_tag = TRUE;
              break;
            }
        }
      g_strfreev (tags);

      if (! has_valid_tag)
        {
          tag_entry->description_shown = TRUE;
          gtk_widget_queue_draw (widget);
        }
    }
  else
    {
      tag_entry->description_shown = FALSE;
      gtk_widget_queue_draw (widget);
    }
}

static gboolean
gimp_tag_entry_expose (GtkWidget       *widget,
                       GdkEventExpose  *event,
                       gpointer         user_data)
{
  GimpTagEntry         *tag_entry = GIMP_TAG_ENTRY (widget);
  PangoContext         *context;
  PangoLayout          *layout;
  PangoAttrList        *attr_list;
  PangoAttribute       *attribute;
  PangoRenderer        *renderer;
  gint                  layout_height;
  gint                  window_height;
  gint                  offset;
  const char           *display_text;

  /* eeeeeek */
  if (widget->window == event->window)
    {
      return FALSE;
    }

  if (! GIMP_TAG_ENTRY (widget)->description_shown)
    {
      return FALSE;
    }

  if (tag_entry->mode == GIMP_TAG_ENTRY_MODE_QUERY)
    {
      display_text = GIMP_TAG_ENTRY_QUERY_DESC;
    }
  else
    {
      display_text = GIMP_TAG_ENTRY_ASSIGN_DESC;
    }

  context = gtk_widget_create_pango_context (GTK_WIDGET (widget));
  layout = pango_layout_new (context);
  attr_list = pango_attr_list_new ();
  attribute = pango_attr_style_new (PANGO_STYLE_ITALIC);
  pango_attr_list_insert (attr_list, attribute);
  pango_layout_set_attributes (layout, attr_list);
  renderer = gdk_pango_renderer_get_default (gtk_widget_get_screen (widget));
  gdk_pango_renderer_set_drawable (GDK_PANGO_RENDERER (renderer), event->window);
  gdk_pango_renderer_set_gc (GDK_PANGO_RENDERER (renderer),
                             widget->style->text_gc[GTK_STATE_INSENSITIVE]);
  pango_layout_set_text (layout, display_text, -1);
  gdk_drawable_get_size (GDK_DRAWABLE (event->window), NULL, &window_height);
  pango_layout_get_size (layout, NULL, &layout_height);
  offset = (window_height * PANGO_SCALE - layout_height) / 2;
  pango_renderer_draw_layout (renderer, layout, offset, offset);
  gdk_pango_renderer_set_drawable (GDK_PANGO_RENDERER (renderer), NULL);
  gdk_pango_renderer_set_gc (GDK_PANGO_RENDERER (renderer), NULL);
  g_object_unref (layout);
  g_object_unref (context);

  return FALSE;
}

static void
gimp_tag_entry_backspace (GtkEntry     *entry)
{
  gint          selection_start;
  gint          selection_end;

  gtk_editable_get_selection_bounds (GTK_EDITABLE (entry),
                                     &selection_start, &selection_end);
  if (selection_start == selection_end
      && selection_start > 0)
    {
      gtk_editable_set_position (GTK_EDITABLE (entry), selection_start - 1);
    }

  if (! gimp_tag_entry_select_jellybean (GIMP_TAG_ENTRY (entry)))
    {
      GTK_ENTRY_CLASS (parent_class)->backspace (entry);
    }
}

static void
gimp_tag_entry_delete_from_cursor (GtkEntry            *entry,
                                   GtkDeleteType        delete_type,
                                   gint                 count)
{
  if (count != 1
      || ! gimp_tag_entry_select_jellybean (GIMP_TAG_ENTRY (entry)))
    {
      GTK_ENTRY_CLASS (parent_class)->delete_from_cursor (entry, delete_type,
                                                          count);
    }
}

static gboolean
gimp_tag_entry_button_release  (GtkWidget         *widget,
                                GdkEventButton    *event)
{
  if (event->button == 1)
    {
      g_idle_add ((GSourceFunc) gimp_tag_entry_try_select_jellybean,
                  widget);
    }
  return GTK_WIDGET_CLASS (parent_class)->button_release_event (widget, event);
}

static gboolean
gimp_tag_entry_try_select_jellybean (GimpTagEntry      *tag_entry)
{
  gimp_tag_entry_select_jellybean (tag_entry);
  return FALSE;
}

static gboolean
gimp_tag_entry_jellybean_is_valid (const gchar *jellybean)
{
  gunichar      c;
  gunichar      separator = g_utf8_get_char (TAG_SEPARATOR_STR);

  do
    {
      c = g_utf8_get_char (jellybean);
      jellybean = g_utf8_next_char (jellybean);
      if (c
          && c != separator
          && !g_unichar_isspace (c))
        {
          return TRUE;
        }
    } while (c);

  return FALSE;
}

static gboolean
gimp_tag_entry_select_jellybean (GimpTagEntry             *entry)
{
  gchar        *original_string;
  gchar        *jellybean_start;
  gchar        *jellybean;
  gchar        *cursor;
  gint          position;
  gint          i;
  gunichar      separator;
  gunichar      c;
  gint          selection_start;
  gint          selection_end;
  gchar        *previous_jellybean;
  gboolean      jellybean_valid = FALSE;

  gtk_editable_get_selection_bounds (GTK_EDITABLE (entry),
                                     &selection_start, &selection_end);
  if (selection_start != selection_end)
    {
      return FALSE;
    }

  original_string = g_strdup (gtk_entry_get_text (GTK_ENTRY (entry)));
  position = gtk_editable_get_position (GTK_EDITABLE (entry));
  cursor = original_string;
  jellybean_start = original_string;
  previous_jellybean = original_string;
  separator = g_utf8_get_char (TAG_SEPARATOR_STR);
  for (i = 0; i < position; i++)
    {
      c = g_utf8_get_char (cursor);
      if (! jellybean_valid
          && c != separator
          && !g_unichar_isspace (c))
        {
          jellybean_valid = TRUE;
        }
      cursor = g_utf8_next_char (cursor);
      if (c == separator
          && jellybean_valid)
        {
          previous_jellybean = jellybean_start;
          jellybean_start = cursor;
          jellybean_valid = FALSE;
        }
    }
  do
    {
      c = g_utf8_get_char (cursor);
      cursor = g_utf8_next_char (cursor);
      if (c == separator)
        {
          *cursor = '\0';
          break;
        }
    } while (c);

  jellybean = jellybean_start;
  if (! gimp_tag_entry_jellybean_is_valid (jellybean))
    {
      jellybean = previous_jellybean;
      if (! gimp_tag_entry_jellybean_is_valid (jellybean))
        {
          return FALSE;
        }
    }

  selection_start = g_utf8_pointer_to_offset (original_string, jellybean);
  selection_end = g_utf8_pointer_to_offset (original_string,
                                            jellybean + strlen (jellybean));
  g_free (original_string);

  gtk_editable_select_region (GTK_EDITABLE (entry),
                             selection_start, selection_end);

  return TRUE;
}


