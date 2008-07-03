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
#include "core/gimptagged.h"

#include "gimptagentry.h"

#define TAG_SEPARATOR_STR   ","

static void     gimp_tag_entry_activate        (GtkEntry          *entry,
                                                gpointer           unused);
static void     gimp_tag_entry_changed         (GtkEntry          *entry,
                                                gpointer           unused);
static void     gimp_tag_entry_insert_text     (GtkEditable       *editable,
                                                gchar             *new_text,
                                                gint               text_length,
                                                gint              *position,
                                                gpointer           user_data);

static void     gimp_tag_entry_query_tag       (GimpTagEntry      *entry);

static void     gimp_tag_entry_assign_tags     (GimpTagEntry      *tag_entry);
static void     gimp_tag_entry_item_set_tags   (GimpTagged        *entry,
                                                GList             *tags);

static void     gimp_tag_entry_load_selection  (GimpTagEntry      *tag_entry);

static gchar ** gimp_tag_entry_parse_tags      (GimpTagEntry      *entry);

static gchar*   gimp_tag_entry_get_completion_prefix     (GimpTagEntry         *entry);
static GList *  gimp_tag_entry_get_completion_candidates (GimpTagEntry         *tag_entry,
                                                          gchar               **used_tags,
                                                          gchar                *prefix);
static gchar *  gimp_tag_entry_get_completion_string     (GimpTagEntry         *tag_entry,
                                                          GList                *candidates,
                                                          gchar                *prefix);
static gboolean gimp_tag_entry_auto_complete             (GimpTagEntry         *tag_entry);


G_DEFINE_TYPE (GimpTagEntry, gimp_tag_entry, GTK_TYPE_ENTRY);

#define parent_class gimp_tag_entry_parent_class


static void
gimp_tag_entry_class_init (GimpTagEntryClass *klass)
{
  /*GObjectClass *object_class = G_OBJECT_CLASS (klass);*/
}

static void
gimp_tag_entry_init (GimpTagEntry *entry)
{
  entry->tagged_container      = NULL;
  entry->selected_items        = NULL;
  entry->mode                  = GIMP_TAG_ENTRY_MODE_QUERY;

  g_signal_connect (entry, "activate",
                    G_CALLBACK (gimp_tag_entry_activate),
                    NULL);
  g_signal_connect (entry, "changed",
                    G_CALLBACK (gimp_tag_entry_changed),
                    NULL);
  g_signal_connect (entry, "insert-text",
                    G_CALLBACK (gimp_tag_entry_insert_text),
                    NULL);
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
  entry->tagged_container = tagged_container;
  entry->mode             = mode;

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

static void
gimp_tag_entry_changed (GtkEntry          *entry,
                        gpointer           unused)
{
  GimpTagEntry         *tag_entry;

  tag_entry = GIMP_TAG_ENTRY (entry);

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
  g_idle_add ((GSourceFunc)gimp_tag_entry_auto_complete,
              editable);
}

static void
gimp_tag_entry_query_tag (GimpTagEntry         *entry)
{
  GimpTagEntry                 *tag_entry;
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

  gimp_filtered_container_set_filter (GIMP_FILTERED_CONTAINER (tag_entry->tagged_container),
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

static gchar **
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

  gimp_tag_entry_load_selection (entry);
}

static void
gimp_tag_entry_load_selection (GimpTagEntry             *tag_entry)
{
  GimpTagged   *selected_item;
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

  tag_iterator = gimp_tagged_get_tags (selected_item);
  while (tag_iterator)
    {
      tag = GPOINTER_TO_UINT (tag_iterator->data);
      text = g_strdup_printf ("%s, ", g_quark_to_string (tag));
      gtk_editable_insert_text (GTK_EDITABLE (tag_entry), text, strlen (text),
                                &insert_pos);
      g_free (text);

      tag_iterator = g_list_next (tag_iterator);
    }
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

