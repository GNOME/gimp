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
#include <gdk/gdkkeysyms.h>

#include "widgets-types.h"

#include "core/gimpcontainer.h"
#include "core/gimpfilteredcontainer.h"
#include "core/gimpcontext.h"
#include "core/gimpviewable.h"
#include "core/gimptag.h"
#include "core/gimptagged.h"

#include "gimptagentry.h"

#include "gimp-intl.h"

#define GIMP_TAG_ENTRY_QUERY_DESC       _("filter")
#define GIMP_TAG_ENTRY_ASSIGN_DESC      _("enter tags")

#define GIMP_TAG_ENTRY_MAX_RECENT_ITEMS 20

typedef enum GimpTagSearchDir_
{
  TAG_SEARCH_NONE,
  TAG_SEARCH_LEFT,
  TAG_SEARCH_RIGHT,
} GimpTagSearchDir;

enum
{
  PROP_0,

  PROP_FILTERED_CONTAINER,
  PROP_TAG_ENTRY_MODE,
};

static void     gimp_tag_entry_set_property              (GObject              *object,
                                                          guint                 property_id,
                                                          const GValue         *value,
                                                          GParamSpec           *pspec);
static void     gimp_tag_entry_get_property              (GObject              *object,
                                                          guint                 property_id,
                                                          GValue               *value,
                                                          GParamSpec           *pspec);
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
static void     gimp_tag_entry_delete_text               (GtkEditable          *editable,
                                                          gint                  start_pos,
                                                          gint                  end_pos,
                                                          gpointer              user_data);
static gboolean gimp_tag_entry_focus_in                  (GtkWidget            *widget,
                                                          GdkEventFocus        *event,
                                                          gpointer              user_data);
static gboolean gimp_tag_entry_focus_out                 (GtkWidget            *widget,
                                                          GdkEventFocus        *event,
                                                          gpointer              user_data);
static void     gimp_tag_entry_container_changed         (GimpContainer        *container,
                                                          GimpObject           *object,
                                                          GimpTagEntry         *tag_entry);
static gboolean gimp_tag_entry_button_release            (GtkWidget            *widget,
                                                          GdkEventButton       *event);
static gboolean gimp_tag_entry_key_press                 (GtkWidget            *widget,
                                                          GdkEventKey          *event,
                                                          gpointer              user_data);
static void     gimp_tag_entry_query_tag                 (GimpTagEntry         *entry);

static void     gimp_tag_entry_assign_tags               (GimpTagEntry         *tag_entry);
static void     gimp_tag_entry_item_set_tags             (GimpTagged           *entry,
                                                          GList                *tags);

static void     gimp_tag_entry_load_selection            (GimpTagEntry         *tag_entry,
                                                          gboolean              sort);

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
static void     gimp_tag_entry_commit_region             (GString              *tags,
                                                          GString              *mask);
static void     gimp_tag_entry_commit_tags               (GimpTagEntry         *tag_entry);
static gboolean gimp_tag_entry_commit_source_func        (GimpTagEntry         *tag_entry);
static gboolean gimp_tag_entry_select_jellybean          (GimpTagEntry         *entry,
                                                          gint                  selection_start,
                                                          gint                  selection_end,
                                                          GimpTagSearchDir      search_dir);
static gboolean gimp_tag_entry_try_select_jellybean      (GimpTagEntry         *tag_entry);

static gboolean gimp_tag_entry_add_to_recent             (GimpTagEntry         *tag_entry,
                                                          const gchar          *tags_string,
                                                          gboolean              to_front);

static void     gimp_tag_entry_next_tag                  (GimpTagEntry         *tag_entry);
static void     gimp_tag_entry_previous_tag              (GimpTagEntry         *tag_entry);

static void     gimp_tag_entry_select_for_deletion       (GimpTagEntry         *tag_entry,
                                                          GimpTagSearchDir      search_dir);


GType
gimp_tag_entry_mode_get_type (void)
{
  static const GEnumValue values[] =
    {
        { GIMP_TAG_ENTRY_MODE_QUERY, "GIMP_TAG_ENTRY_MODE_QUERY", "query" },
        { GIMP_TAG_ENTRY_MODE_ASSIGN, "GIMP_TAG_ENTRY_MODE_ASSIGN", "assign" },
        { 0, NULL, NULL }
    };

  static const GimpEnumDesc descs[] =
    {
        { GIMP_TAG_ENTRY_MODE_QUERY, N_("Query"), NULL },
        { GIMP_TAG_ENTRY_MODE_ASSIGN, N_("Assign"), NULL },
        { 0, NULL, NULL }
    };

  static GType type = 0;

  if (! type)
    {
      type = g_enum_register_static ("GimpTagEntryMode", values);
      gimp_enum_set_value_descriptions (type, descs);
    }

  return type;
}

G_DEFINE_TYPE (GimpTagEntry, gimp_tag_entry, GTK_TYPE_ENTRY);

#define parent_class gimp_tag_entry_parent_class


static void
gimp_tag_entry_class_init (GimpTagEntryClass *klass)
{
  GObjectClass         *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass       *widget_class = GTK_WIDGET_CLASS (klass);

  object_class->dispose                 = gimp_tag_entry_dispose;
  object_class->get_property            = gimp_tag_entry_get_property;
  object_class->set_property            = gimp_tag_entry_set_property;

  widget_class->button_release_event    = gimp_tag_entry_button_release;

  g_object_class_install_property (object_class,
                                   PROP_FILTERED_CONTAINER,
                                   g_param_spec_object ("filtered-container",
                                                        ("Filtered container"),
                                                        ("The Filtered container"),
                                                        GIMP_TYPE_FILTERED_CONTAINER,
                                                        G_PARAM_CONSTRUCT_ONLY
                                                        | G_PARAM_WRITABLE
                                                        | G_PARAM_READABLE));

  g_object_class_install_property (object_class,
                                   PROP_TAG_ENTRY_MODE,
                                   g_param_spec_enum ("tag-entry-mode",
                                                      ("Working mode"),
                                                      ("Mode in which to work."),
                                                      GIMP_TYPE_TAG_ENTRY_MODE,
                                                      GIMP_TAG_ENTRY_MODE_QUERY,
                                                      G_PARAM_CONSTRUCT_ONLY
                                                      | G_PARAM_WRITABLE
                                                      | G_PARAM_READABLE));
}

static void
gimp_tag_entry_init (GimpTagEntry *entry)
{
  entry->filtered_container      = NULL;
  entry->selected_items        = NULL;
  entry->mode                  = GIMP_TAG_ENTRY_MODE_QUERY;
  entry->description_shown     = FALSE;
  entry->mask                  = g_string_new ("");

  g_signal_connect (entry, "activate",
                    G_CALLBACK (gimp_tag_entry_activate),
                    NULL);
  g_signal_connect (entry, "changed",
                    G_CALLBACK (gimp_tag_entry_changed),
                    NULL);
  g_signal_connect (entry, "insert-text",
                    G_CALLBACK (gimp_tag_entry_insert_text),
                    NULL);
  g_signal_connect (entry, "delete-text",
                    G_CALLBACK (gimp_tag_entry_delete_text),
                    NULL);
  g_signal_connect (entry, "key-press-event",
                    G_CALLBACK (gimp_tag_entry_key_press),
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

  if (tag_entry->recent_list)
    {
      g_list_foreach (tag_entry->recent_list, (GFunc) g_free, NULL);
      g_list_free (tag_entry->recent_list);
      tag_entry->recent_list = NULL;
    }

  if (tag_entry->filtered_container)
    {
      g_signal_handlers_disconnect_by_func (tag_entry->filtered_container,
                                            gimp_tag_entry_container_changed, tag_entry);
      g_object_unref (tag_entry->filtered_container);
      tag_entry->filtered_container = NULL;
    }

  if (tag_entry->mask)
    {
      g_string_free (tag_entry->mask, TRUE);
      tag_entry->mask = NULL;
    }

  G_OBJECT_CLASS (parent_class)->dispose (object);
}

static void
gimp_tag_entry_set_property    (GObject              *object,
                                guint                 property_id,
                                const GValue         *value,
                                GParamSpec           *pspec)
{
  GimpTagEntry         *tag_entry = GIMP_TAG_ENTRY (object);

  switch (property_id)
    {
      case PROP_FILTERED_CONTAINER:
          tag_entry->filtered_container = g_value_get_object (value);
          g_assert (GIMP_IS_FILTERED_CONTAINER (tag_entry->filtered_container));
          g_object_ref (tag_entry->filtered_container);
          g_signal_connect (tag_entry->filtered_container, "add",
                            G_CALLBACK (gimp_tag_entry_container_changed), tag_entry);
          g_signal_connect (tag_entry->filtered_container, "remove",
                            G_CALLBACK (gimp_tag_entry_container_changed), tag_entry);
          break;

      case PROP_TAG_ENTRY_MODE:
          tag_entry->mode = g_value_get_enum (value);
          gimp_tag_entry_toggle_desc (tag_entry, TRUE);
          break;

      default:
          G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
          break;
    }
}

static void
gimp_tag_entry_get_property    (GObject              *object,
                                guint                 property_id,
                                GValue               *value,
                                GParamSpec           *pspec)
{
  GimpTagEntry         *tag_entry = GIMP_TAG_ENTRY (object);

  switch (property_id)
    {
      case PROP_FILTERED_CONTAINER:
          g_value_set_object (value, tag_entry->filtered_container);
          break;

      case PROP_TAG_ENTRY_MODE:
          g_value_set_enum (value, tag_entry->mode);
          break;

      default:
          G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
          break;
    }
}

/**
 * gimp_tag_entry_new:
 * @filtered_container: a #GimpFilteredContainer object
 * @mode:               #GimpTagEntryMode to work in.
 *
 * #GimpTagEntry is a widget which can query and assign tags to tagged objects.
 * When operating in query mode, @filtered_container is kept up to date with
 * tags selected. When operating in assignment mode, tags are assigned to
 * objects selected and visible in @filtered_container.
 *
 * Return value: a new GimpTagEntry widget.
 **/
GtkWidget *
gimp_tag_entry_new (GimpFilteredContainer      *filtered_container,
                    GimpTagEntryMode            mode)
{
  GimpTagEntry         *entry;

  g_return_val_if_fail (GIMP_IS_FILTERED_CONTAINER (filtered_container),
                        NULL);

  entry = g_object_new (GIMP_TYPE_TAG_ENTRY,
                        "filtered-container", filtered_container,
                        "tag-entry-mode", mode,
                        NULL);
  return GTK_WIDGET (entry);
}

static void
gimp_tag_entry_activate (GtkEntry              *entry,
                         gpointer               unused)
{
  GimpTagEntry         *tag_entry;
  gint                  selection_start;
  gint                  selection_end;
  GList                *iterator;

  tag_entry = GIMP_TAG_ENTRY (entry);

  gimp_tag_entry_toggle_desc (tag_entry, FALSE);

  gtk_editable_get_selection_bounds (GTK_EDITABLE (entry),
                                     &selection_start, &selection_end);
  if (selection_start != selection_end)
    {
      gtk_editable_select_region (GTK_EDITABLE (entry),
                                  selection_end, selection_end);
    }

  iterator = tag_entry->selected_items;
  while (iterator)
    {
      if (gimp_container_have (GIMP_CONTAINER (tag_entry->filtered_container),
                               GIMP_OBJECT(iterator->data)))
        {
          break;
        }

      iterator = g_list_next (iterator);
    }

  if (tag_entry->mode == GIMP_TAG_ENTRY_MODE_ASSIGN
      && iterator)
    {
      gimp_tag_entry_assign_tags (GIMP_TAG_ENTRY (entry));
    }
}

/**
 * gimp_tag_entry_set_tag_string:
 * @tag_entry:  a #GimpTagEntry object.
 * @tag_string: string of tags, separated by any terminal punctuation
 *              character.
 *
 * Sets tags from @tag_string to @tag_entry. Given tags do not need to
 * be valid as they can be fixed or dropped automatically. Depending on
 * selected #GimpTagEntryMode, appropriate action is peformed.
 **/
void
gimp_tag_entry_set_tag_string (GimpTagEntry    *tag_entry,
                               const gchar     *tag_string)
{
  g_return_if_fail (GIMP_IS_TAG_ENTRY (tag_entry));

  tag_entry->internal_operation++;
  gtk_entry_set_text (GTK_ENTRY (tag_entry), tag_string);
  gtk_editable_set_position (GTK_EDITABLE (tag_entry), -1);
  tag_entry->internal_operation--;
  gimp_tag_entry_commit_tags (tag_entry);

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
  GimpTagEntry *tag_entry = GIMP_TAG_ENTRY (editable);
  const gchar  *entry_text;
  gboolean      is_tag[2];
  gint          i;
  gint          insert_pos;

  printf ("insert mask (b): '%s'\n", tag_entry->mask->str);

  entry_text = gtk_entry_get_text (GTK_ENTRY (editable));

  is_tag[0] = FALSE;
  if (*position > 0)
    {
      is_tag[0] = (tag_entry->mask->str[*position - 1] == 't' || tag_entry->mask->str[*position - 1] == 's');
    }
  is_tag[1] = (tag_entry->mask->str[*position] == 't' || tag_entry->mask->str[*position] == 's');
  if (is_tag[0] && is_tag[1])
    {
      g_signal_stop_emission_by_name (editable, "insert_text");
    }
  else if (! tag_entry->suppress_mask_update)
    {
      insert_pos = *position;
      for (i = 0; i < text_length; i++)
        {
          g_string_insert_c (tag_entry->mask, insert_pos + i, 'u');
        }
    }

  printf ("insert mask (a): '%s'\n", tag_entry->mask->str);

  if (! tag_entry->internal_operation)
    {
      g_idle_add ((GSourceFunc)gimp_tag_entry_auto_complete,
                  editable);
    }
}

static void
gimp_tag_entry_delete_text     (GtkEditable          *editable,
                                gint                  start_pos,
                                gint                  end_pos,
                                gpointer              user_data)
{
  GimpTagEntry *tag_entry = GIMP_TAG_ENTRY (editable);

  printf ("delete mask (b): '%s'\n", tag_entry->mask->str);

  if (! tag_entry->internal_operation)
    {
      g_signal_handlers_block_by_func (editable,
                                       gimp_tag_entry_delete_text, NULL);

      if (end_pos > start_pos
          && (tag_entry->mask->str[end_pos - 1] == 't'
              || tag_entry->mask->str[end_pos - 1] == 's'))
        {
          while (end_pos <= tag_entry->mask->len
                 && (tag_entry->mask->str[end_pos] == 's'))
            {
              end_pos++;
            }
        }

      gtk_editable_delete_text (editable, start_pos, end_pos);
      if (! tag_entry->suppress_mask_update)
        {
          g_string_erase (tag_entry->mask, start_pos, end_pos - start_pos);
        }

      g_signal_handlers_unblock_by_func (editable,
                                         gimp_tag_entry_delete_text, NULL);

      g_signal_stop_emission_by_name (editable, "delete_text");
    }
  else
    {
      if (! tag_entry->suppress_mask_update)
        {
          g_string_erase (tag_entry->mask, start_pos, end_pos - start_pos);
        }
    }

  printf ("delete mask (a): '%s'\n", tag_entry->mask->str);
}

static void
gimp_tag_entry_query_tag (GimpTagEntry         *entry)
{
  gchar                       **parsed_tags;
  gint                          count;
  gint                          i;
  GimpTag                      *tag;
  GList                        *query_list = NULL;

  parsed_tags = gimp_tag_entry_parse_tags (entry);
  count = g_strv_length (parsed_tags);
  for (i = 0; i < count; i++)
    {
      if (strlen (parsed_tags[i]) > 0)
        {
          tag = gimp_tag_try_new (parsed_tags[i]);
          if (tag)
            {
              query_list = g_list_append (query_list, tag);
            }
        }
    }
  g_strfreev (parsed_tags);

  gimp_filtered_container_set_filter (GIMP_FILTERED_CONTAINER (entry->filtered_container),
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
  GimpTag              *tag;
  GList                *tag_list = NULL;

  parsed_tags = gimp_tag_entry_parse_tags (tag_entry);
  count = g_strv_length (parsed_tags);
  for (i = 0; i < count; i++)
    {
      tag = gimp_tag_new (parsed_tags[i]);
      if (tag)
        {
          tag_list = g_list_append (tag_list, tag);
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
      gimp_tagged_remove_tag (tagged, GIMP_TAG (tags_iterator->data));
      tags_iterator = g_list_next (tags_iterator);
    }
  g_list_free (old_tags);

  tags_iterator = tags;
  while (tags_iterator)
    {
      printf ("tagged: %s\n", gimp_tag_get_name (GIMP_TAG (tags_iterator->data)));
      gimp_tagged_add_tag (tagged, GIMP_TAG (tags_iterator->data));
      tags_iterator = g_list_next (tags_iterator);
    }
}

/**
 * gimp_tag_entry_parse_tags:
 * @entry:      a #GimpTagEntry widget.
 *
 * Parses currently entered tags from @entry. Tags do not need to be valid as
 * they are fixed when necessary. Only valid tags are returned.
 *
 * Return value: a newly allocated NULL terminated list of strings. It should
 * be freed using g_strfreev().
 **/
gchar **
gimp_tag_entry_parse_tags (GimpTagEntry        *entry)
{
  gchar               **parsed_tags;
  gint                  length;
  gint                  i;
  GString              *parsed_tag;
  const gchar          *cursor;
  GList                *tag_list = NULL;
  GList                *iterator;
  gunichar              c;

  g_return_val_if_fail (GIMP_IS_TAG_ENTRY (entry), NULL);

  parsed_tag = g_string_new ("");
  cursor = gtk_entry_get_text (GTK_ENTRY (entry));
  do
    {
      c = g_utf8_get_char (cursor);
      cursor = g_utf8_next_char (cursor);

      if (! c || g_unichar_is_terminal_punctuation (c))
        {
          if (parsed_tag->len > 0)
            {
              gchar    *validated_tag = gimp_tag_string_make_valid (parsed_tag->str);
              if (validated_tag)
                {
                  tag_list = g_list_append (tag_list, validated_tag);
                }

              g_string_set_size (parsed_tag, 0);
            }
        }
      else
        {
          g_string_append_unichar (parsed_tag, c);
        }
    } while (c);
  g_string_free (parsed_tag, TRUE);

  length = g_list_length (tag_list);
  parsed_tags = g_malloc ((length + 1) * sizeof (gchar **));
  iterator = tag_list;
  for (i = 0; i < length; i++)
    {
      parsed_tags[i] = (gchar *) iterator->data;

      iterator = g_list_next (iterator);
    }
  parsed_tags[length] = NULL;

  return parsed_tags;
}

/**
 * gimp_tag_entry_set_selected_items:
 * @entry:      a #GimpTagEntry widget.
 * @items:      a list of #GimpTagged objects.
 *
 * Set list of currently selected #GimpTagged objects. Only selected and
 * visible (not filtered out) #GimpTagged objects are assigned tags when
 * operating in tag assignment mode.
 **/
void
gimp_tag_entry_set_selected_items (GimpTagEntry            *entry,
                                   GList                   *items)
{
  GList        *iterator;

  g_return_if_fail (GIMP_IS_TAG_ENTRY (entry));

  if (entry->selected_items)
    {
      g_list_free (entry->selected_items);
      entry->selected_items = NULL;
    }

  entry->selected_items = g_list_copy (items);

  iterator = entry->selected_items;
  while (iterator)
    {
      if (gimp_tagged_get_tags (GIMP_TAGGED (iterator->data))
          && gimp_container_have (GIMP_CONTAINER (entry->filtered_container),
                                  GIMP_OBJECT(iterator->data)))
        {
          break;
        }

      iterator = g_list_next (iterator);
    }

  if (entry->mode == GIMP_TAG_ENTRY_MODE_ASSIGN)
    {
      if (iterator)
        {
          gimp_tag_entry_load_selection (entry, TRUE);
          gimp_tag_entry_toggle_desc (entry, FALSE);
        }
      else
        {
          entry->internal_operation++;
          gtk_editable_delete_text (GTK_EDITABLE (entry), 0, -1);
          entry->internal_operation--;
          gimp_tag_entry_toggle_desc (entry, TRUE);
        }
    }
}

static void
gimp_tag_entry_load_selection (GimpTagEntry             *tag_entry,
                               gboolean                  sort)
{
  GimpTagged   *selected_item;
  GList        *tag_list;
  GList        *tag_iterator;
  gint          insert_pos;
  GimpTag      *tag;
  gchar        *text;

  tag_entry->internal_operation++;
  gtk_editable_delete_text (GTK_EDITABLE (tag_entry), 0, -1);
  tag_entry->internal_operation--;

  if (! tag_entry->selected_items)
    {
      return;
    }

  selected_item = GIMP_TAGGED (tag_entry->selected_items->data);
  insert_pos = 0;

  tag_list = g_list_copy (gimp_tagged_get_tags (selected_item));
  if (sort)
    {
      tag_list = g_list_sort (tag_list, gimp_tag_compare_func);
    }
  tag_iterator = tag_list;
  while (tag_iterator)
    {
      tag = GIMP_TAG (tag_iterator->data);
      text = g_strdup_printf ("%s%s ", gimp_tag_get_name (tag), gimp_tag_entry_get_separator ());
      tag_entry->internal_operation++;
      gtk_editable_insert_text (GTK_EDITABLE (tag_entry), text, strlen (text),
                                &insert_pos);
      tag_entry->internal_operation--;
      g_free (text);

      tag_iterator = g_list_next (tag_iterator);
    }
  g_list_free (tag_list);

  gimp_tag_entry_commit_tags (tag_entry);
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
  gunichar      c;

  position = gtk_editable_get_position (GTK_EDITABLE (entry));
  if (position < 1
      || entry->mask->str[position - 1] != 'u')
    {
      return g_strdup ("");
    }

  original_string = g_strdup (gtk_entry_get_text (GTK_ENTRY (entry)));
  cursor = original_string;
  prefix_start = original_string;
  for (i = 0; i < position; i++)
    {
      c = g_utf8_get_char (cursor);
      cursor = g_utf8_next_char (cursor);
      if (g_unichar_is_terminal_punctuation (c))
        {
          prefix_start = cursor;
        }
    }
  *cursor = '\0';

  prefix = g_strdup (g_strstrip (prefix_start));
  g_free (original_string);

  return prefix;
}

static GList *
gimp_tag_entry_get_completion_candidates (GimpTagEntry         *tag_entry,
                                          gchar               **used_tags,
                                          gchar                *src_prefix)
{
  GList        *candidates = NULL;
  GList        *all_tags;
  GList        *tag_iterator;
  GimpTag      *tag;
  const gchar  *tag_name;
  gint          i;
  gint          length;
  gchar        *prefix;

  if (!src_prefix
      || strlen (src_prefix) < 1)
    {
      return NULL;
    }

  prefix = g_utf8_normalize (src_prefix, -1, G_NORMALIZE_ALL);
  if (! prefix)
    {
      return NULL;
    }

  all_tags = g_hash_table_get_keys (tag_entry->filtered_container->tag_ref_counts);
  tag_iterator = all_tags;
  length = g_strv_length (used_tags);
  while (tag_iterator)
    {
      tag = GIMP_TAG (tag_iterator->data);
      tag_name = gimp_tag_get_name (tag);
      if (g_str_has_prefix (tag_name, prefix))
        {
          /* check if tag is not already entered */
          for (i = 0; i < length; i++)
            {
              if (! gimp_tag_compare_with_string (tag, used_tags[i]))
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
  g_free (prefix);

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
      candidate_string = gimp_tag_get_name (GIMP_TAG (candidates->data));
      return g_strdup (candidate_string + prefix_length);
    }

  completions = g_malloc (length * sizeof (gchar*));
  candidate_iterator = candidates;
  for (i = 0; i < length; i++)
    {
      candidate_string = gimp_tag_get_name (GIMP_TAG (candidate_iterator->data));
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
              candidate_string = gimp_tag_get_name (GIMP_TAG (candidates->data));
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

  candidate_string = gimp_tag_get_name (GIMP_TAG (candidates->data));
  return g_strdup (candidate_string + prefix_length);
}

static gboolean
gimp_tag_entry_focus_in        (GtkWidget         *widget,
                                GdkEventFocus     *event,
                                gpointer           user_data)
{
  gimp_tag_entry_toggle_desc (GIMP_TAG_ENTRY (widget), FALSE);

  return FALSE;
}

static gboolean
gimp_tag_entry_focus_out       (GtkWidget         *widget,
                                GdkEventFocus     *event,
                                gpointer           user_data)
{
  GimpTagEntry  *tag_entry = GIMP_TAG_ENTRY (widget);

  gimp_tag_entry_commit_tags (tag_entry);
  gimp_tag_entry_add_to_recent (tag_entry,
                                gtk_entry_get_text (GTK_ENTRY (widget)),
                                TRUE);

  gimp_tag_entry_toggle_desc (tag_entry, TRUE);
  return FALSE;
}


static void
gimp_tag_entry_container_changed       (GimpContainer        *container,
                                        GimpObject           *object,
                                        GimpTagEntry         *tag_entry)
{
  if (tag_entry->mode == GIMP_TAG_ENTRY_MODE_ASSIGN)
    {
      GList        *selected_iterator = tag_entry->selected_items;

      while (selected_iterator)
        {
          if (gimp_tagged_get_tags (GIMP_TAGGED (selected_iterator->data))
              && gimp_container_have (GIMP_CONTAINER (tag_entry->filtered_container),
                                      GIMP_OBJECT(selected_iterator->data)))
            {
              break;
            }
          selected_iterator = g_list_next (selected_iterator);
        }
      if (! selected_iterator)
        {
          tag_entry->internal_operation++;
          gtk_editable_delete_text (GTK_EDITABLE (tag_entry), 0, -1);
          tag_entry->internal_operation--;
        }
    }
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
      gchar        *current_text;
      size_t        len;

      current_text = g_strdup (gtk_entry_get_text (GTK_ENTRY (tag_entry)));
      current_text = g_strstrip (current_text);
      len = strlen (current_text);
      g_free (current_text);

      if (len > 0)
        {
          return;
        }

      tag_entry->description_shown = TRUE;
      gtk_widget_queue_draw (widget);
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
  gint                  layout_width;
  gint                  layout_height;
  gint                  window_width;
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
  gdk_drawable_get_size (GDK_DRAWABLE (event->window),
                         &window_width, &window_height);
  pango_layout_get_size (layout,
                         &layout_width, &layout_height);
  offset = (window_height * PANGO_SCALE - layout_height) / 2;
  if (gtk_widget_get_direction (widget) == GTK_TEXT_DIR_RTL)
    {
      pango_renderer_draw_layout (renderer, layout,
                                  window_width * PANGO_SCALE - layout_width - offset,
                                  offset);
    }
  else
    {
      pango_renderer_draw_layout (renderer, layout, offset, offset);
    }
  gdk_pango_renderer_set_drawable (GDK_PANGO_RENDERER (renderer), NULL);
  gdk_pango_renderer_set_gc (GDK_PANGO_RENDERER (renderer), NULL);
  g_object_unref (layout);
  g_object_unref (context);

  return FALSE;
}

static gboolean
gimp_tag_entry_key_press       (GtkWidget            *widget,
                                GdkEventKey          *event,
                                gpointer              user_data)
{
  GimpTagEntry         *tag_entry = GIMP_TAG_ENTRY (widget);
  guchar                c;

  c = gdk_keyval_to_unicode (event->keyval);
  if (g_unichar_is_terminal_punctuation (c))
    {
      g_idle_add ((GSourceFunc) gimp_tag_entry_commit_source_func, tag_entry);
      return FALSE;
    }

  switch (event->keyval)
    {
      case GDK_Tab:
            {
              gint      selection_start;
              gint      selection_end;

              gtk_editable_get_selection_bounds (GTK_EDITABLE (widget),
                                                 &selection_start, &selection_end);
              if (selection_start != selection_end)
                {
                  gtk_editable_select_region (GTK_EDITABLE (widget),
                                              selection_end, selection_end);
                }

              g_idle_add ((GSourceFunc)gimp_tag_entry_auto_complete,
                          tag_entry);
            }
          return TRUE;

      case GDK_Return:
          gimp_tag_entry_commit_tags (tag_entry);
          break;

      case GDK_Left:
          gimp_tag_entry_previous_tag (tag_entry);
          return TRUE;

      case GDK_Right:
          gimp_tag_entry_next_tag (tag_entry);
          return TRUE;

      case GDK_BackSpace:
            {
              gint      selection_start;
              gint      selection_end;

              gtk_editable_get_selection_bounds (GTK_EDITABLE (tag_entry),
                                                 &selection_start, &selection_end);
              if (gimp_tag_entry_select_jellybean (tag_entry,
                                                   selection_start, selection_end,
                                                   TAG_SEARCH_LEFT))
                {
                  return TRUE;
                }
              else
                {
                  gimp_tag_entry_select_for_deletion (tag_entry, TAG_SEARCH_LEFT);
                }
            }
          break;

      case GDK_Delete:
            {
              gint      selection_start;
              gint      selection_end;

              gtk_editable_get_selection_bounds (GTK_EDITABLE (tag_entry),
                                                 &selection_start, &selection_end);
              if (gimp_tag_entry_select_jellybean (tag_entry,
                                                   selection_start, selection_end,
                                                   TAG_SEARCH_RIGHT))
                {
                  return TRUE;
                }
              else
                {
                  gimp_tag_entry_select_for_deletion (tag_entry, TAG_SEARCH_RIGHT);
                }
            }
          break;

      case GDK_Up:
      case GDK_Down:
          if (tag_entry->recent_list != NULL)
            {
              gchar    *recent_item;
              gchar    *very_recent_item;

              very_recent_item = g_strdup (gtk_entry_get_text (GTK_ENTRY (tag_entry)));
              gimp_tag_entry_add_to_recent (tag_entry, very_recent_item, TRUE);
              g_free (very_recent_item);

              if (event->keyval == GDK_Up)
                {
                  recent_item = (gchar *) g_list_first (tag_entry->recent_list)->data;
                  tag_entry->recent_list = g_list_remove (tag_entry->recent_list, recent_item);
                  tag_entry->recent_list = g_list_append (tag_entry->recent_list, recent_item);
                }
              else
                {
                  recent_item = (gchar *) g_list_last (tag_entry->recent_list)->data;
                  tag_entry->recent_list = g_list_remove (tag_entry->recent_list, recent_item);
                  tag_entry->recent_list = g_list_prepend (tag_entry->recent_list, recent_item);
                }

              recent_item = (gchar *) g_list_first (tag_entry->recent_list)->data;
              tag_entry->internal_operation++;
              gtk_entry_set_text (GTK_ENTRY (tag_entry), recent_item);
              gtk_editable_set_position (GTK_EDITABLE (tag_entry), -1);
              tag_entry->internal_operation--;
            }
          return TRUE;

      default:
          break;
    }

  return FALSE;
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
  gint selection_start;
  gint selection_end;
  gint selection_pos = gtk_editable_get_position (GTK_EDITABLE (tag_entry));
  gint char_count = g_utf8_strlen (gtk_entry_get_text (GTK_ENTRY (tag_entry)), -1);
  if (selection_pos == char_count)
    {
      return FALSE;
    }

  gtk_editable_get_selection_bounds (GTK_EDITABLE (tag_entry),
                                     &selection_start, &selection_end);
  gimp_tag_entry_select_jellybean (tag_entry, selection_start, selection_end, TAG_SEARCH_NONE);
  return FALSE;
}

static gboolean
gimp_tag_entry_select_jellybean (GimpTagEntry          *tag_entry,
                                 gint                   selection_start,
                                 gint                   selection_end,
                                 GimpTagSearchDir       search_dir)
{
  gint          prev_selection_start;
  gint          prev_selection_end;

  if (! tag_entry->mask->len)
    {
      return FALSE;
    }

  if (selection_start >= tag_entry->mask->len)
    {
      selection_start = tag_entry->mask->len - 1;
    }

  if (tag_entry->mask->str[selection_start] == 'u')
    {
      return FALSE;
    }

  switch (search_dir)
    {
      case TAG_SEARCH_NONE:
            {
              if (selection_start > 0
                  && tag_entry->mask->str[selection_start] == 's')
                {
                  selection_start--;
                }
            }
          break;

      case TAG_SEARCH_LEFT:
            {
              if ((tag_entry->mask->str[selection_start] == 'w'
                   || tag_entry->mask->str[selection_start] == 's')
                  && selection_start > 0)
                {
                  while ((tag_entry->mask->str[selection_start] == 'w'
                          || tag_entry->mask->str[selection_start] == 's')
                         && selection_start > 0)
                    {
                      selection_start--;
                    }
                  selection_end = selection_start + 1;
                }
            }
          break;

      case TAG_SEARCH_RIGHT:
            {
              if ((tag_entry->mask->str[selection_start] == 'w'
                   || tag_entry->mask->str[selection_start] == 's')
                  && selection_start < tag_entry->mask->len - 1)
                {
                  while ((tag_entry->mask->str[selection_start] == 'w'
                          || tag_entry->mask->str[selection_start] == 's')
                         && selection_start < tag_entry->mask->len - 1)
                    {
                      selection_start++;
                    }
                  selection_end = selection_start + 1;
                }
            }
          break;
    }

  if (selection_start < tag_entry->mask->len
      && selection_start == selection_end)
    {
      selection_end = selection_start + 1;
    }

  gtk_editable_get_selection_bounds (GTK_EDITABLE (tag_entry),
                                     &prev_selection_start,
                                     &prev_selection_end);

  if (tag_entry->mask->str[selection_start] == 't')
    {
      while (selection_start > 0
             && (tag_entry->mask->str[selection_start - 1] == 't'))
        {
          selection_start--;
        }
    }

  if (selection_end > selection_start
      && (tag_entry->mask->str[selection_end - 1] == 't'))
    {
      while (selection_end <= tag_entry->mask->len
             && (tag_entry->mask->str[selection_end] == 't'))
        {
          selection_end++;
        }
    }

  if ((selection_start != prev_selection_start
      || selection_end != prev_selection_end)
      && (tag_entry->mask->str[selection_start] == 't'))
    {
      gtk_editable_select_region (GTK_EDITABLE (tag_entry),
                                  selection_start, selection_end);
      return TRUE;
    }
  else
    {
      return FALSE;
    }
}

static gboolean
gimp_tag_entry_add_to_recent   (GimpTagEntry         *tag_entry,
                                const gchar          *tags_string,
                                gboolean              to_front)
{
  gchar        *recent_item = NULL;
  GList        *tags_iterator;
  gchar        *stripped_string;
  gint          stripped_length;

  stripped_string = g_strdup (tags_string);
  stripped_string = g_strstrip (stripped_string);
  stripped_length = strlen (stripped_string);
  g_free (stripped_string);

  if (stripped_length <= 0)
    {
      /* there is no content in the string,
       * therefore don't add to recent list. */
      return FALSE;
    }

  if (g_list_length (tag_entry->recent_list) >= GIMP_TAG_ENTRY_MAX_RECENT_ITEMS)
    {
      gchar *last_item = (gchar *) g_list_last (tag_entry->recent_list)->data;
      tag_entry->recent_list = g_list_remove (tag_entry->recent_list, last_item);
      g_free (last_item);
    }

  tags_iterator = tag_entry->recent_list;
  while (tags_iterator)
    {
      if (! strcmp (tags_string, tags_iterator->data))
        {
          recent_item = tags_iterator->data;
          tag_entry->recent_list = g_list_remove (tag_entry->recent_list,
                                                  recent_item);
          break;
        }
      tags_iterator = g_list_next (tags_iterator);
    }

  if (! recent_item)
    {
      recent_item = g_strdup (tags_string);
    }

  if (to_front)
    {
      tag_entry->recent_list = g_list_prepend (tag_entry->recent_list,
                                               recent_item);
    }
  else
    {
      tag_entry->recent_list = g_list_append (tag_entry->recent_list,
                                              recent_item);
    }

  return TRUE;
}

const gchar *
gimp_tag_entry_get_separator  (void)
{
  /* IMPORTANT: use only one of Unicode terminal punctuation chars.
   * http://unicode.org/review/pr-23.html */
  return _(",");
}

static void
gimp_tag_entry_commit_region   (GString              *tags,
                                GString              *mask)
{
  gint          i = 0;
  gint          j;
  gint          stage = 0;
  gunichar      c;
  gchar        *cursor;
  GString      *out_tags;
  GString      *out_mask;
  GString      *tag_buffer;

  out_tags = g_string_new ("");
  out_mask = g_string_new ("");
  tag_buffer = g_string_new ("");

  cursor = tags->str;
  for (i = 0; i <= mask->len; i++)
    {
      c = g_utf8_get_char (cursor);
      cursor = g_utf8_next_char (cursor);

      if (stage == 0)
        {
          /* whitespace before tag */
          if (g_unichar_isspace (c))
            {
              g_string_append_unichar (out_tags, c);
              g_string_append_c (out_mask, 'w');
            }
          else
            {
              stage++;
            }
        }

      if (stage == 1)
        {
          /* tag */
          if (c && ! g_unichar_is_terminal_punctuation (c))
            {
              g_string_append_unichar (tag_buffer, c);
            }
          else
            {
              gchar    *valid_tag = gimp_tag_string_make_valid (tag_buffer->str);
              gsize     tag_length;

              if (valid_tag)
                {
                  tag_length = g_utf8_strlen (valid_tag, -1);
                  g_string_append (out_tags, valid_tag);
                  for (j = 0; j < tag_length; j++)
                    {
                      g_string_append_c (out_mask, 't');
                    }
                  g_free (valid_tag);

                  if (! c)
                    {
                      g_string_append (out_tags, gimp_tag_entry_get_separator ());
                      g_string_append_c (out_mask, 's');
                    }

                  stage++;
                }
              else
                {
                  stage = 0;
                }

              g_string_set_size (tag_buffer, 0);

            }
        }

      if (stage == 2)
        {
          if (g_unichar_is_terminal_punctuation (c))
            {
              g_string_append_unichar (out_tags, c);
              g_string_append_c (out_mask, 's');
            }
          else
            {
              if (g_unichar_isspace (c))
                {
                  g_string_append_unichar (out_tags, c);
                  g_string_append_c (out_mask, 'w');
                }

              stage = 0;
            }
        }
    }

  g_string_assign (tags, out_tags->str);
  g_string_assign (mask, out_mask->str);

  g_string_free (tag_buffer, TRUE);
  g_string_free (out_tags, TRUE);
  g_string_free (out_mask, TRUE);
}

static void
gimp_tag_entry_commit_tags     (GimpTagEntry         *tag_entry)
{
  gint          i;
  gint          region_start;
  gint          region_end;
  gint          position;
  gboolean      found_region;
  gint          cursor_position;

  printf ("commiting tags ...\n");
  printf ("commit mask (b): '%s'\n", tag_entry->mask->str);

  cursor_position = gtk_editable_get_position (GTK_EDITABLE (tag_entry));

  do
    {
      found_region = FALSE;

      for (i = 0; i < tag_entry->mask->len; i++)
        {
          if (tag_entry->mask->str[i] == 'u')
            {
              found_region = TRUE;
              region_start = i;
              region_end = i + 1;
              for (i++; i < tag_entry->mask->len; i++)
                {
                  if (tag_entry->mask->str[i] == 'u')
                    {
                      region_end = i + 1;
                    }
                  else
                    {
                      break;
                    }
                }
              break;
            }
        }

      if (found_region)
        {
          gchar        *tags_string;
          GString      *tags;
          GString      *mask;

          tags_string = gtk_editable_get_chars (GTK_EDITABLE (tag_entry), region_start, region_end);
          tags = g_string_new (tags_string);
          g_free (tags_string);

          mask = g_string_new_len (tag_entry->mask->str + region_start, region_end - region_start);

          gimp_tag_entry_commit_region (tags, mask);

          /* prepend space before if needed */
          if (region_start > 0
              && tag_entry->mask->str[region_start - 1] != 'w'
              && mask->len > 0
              && mask->str[0] != 'w')
            {
              g_string_prepend_c (tags, ' ');
              g_string_prepend_c (mask, 'w');
            }

          /* append space after if needed */
          if (region_end <= tag_entry->mask->len
              && tag_entry->mask->str[region_end] != 'w'
              && mask->len > 0
              && mask->str[mask->len - 1] != 'w')
            {
              g_string_append_c (tags, ' ');
              g_string_append_c (mask, 'w');
            }

          if (cursor_position >= region_end)
            {
              cursor_position += mask->len - (region_end - region_start);
            }

          tag_entry->internal_operation++;
          tag_entry->suppress_mask_update++;
          gtk_editable_delete_text (GTK_EDITABLE (tag_entry), region_start, region_end);
          position = region_start;
          gtk_editable_insert_text (GTK_EDITABLE (tag_entry), tags->str, mask->len, &position);
          tag_entry->suppress_mask_update--;
          tag_entry->internal_operation--;

          g_string_erase (tag_entry->mask, region_start, region_end - region_start);
          g_string_insert_len (tag_entry->mask, region_start, mask->str, mask->len);

          g_string_free (mask, TRUE);
          g_string_free (tags, TRUE);
        }
    } while (found_region);

  gtk_editable_set_position (GTK_EDITABLE (tag_entry), cursor_position);


  printf ("commit mask (a): '%s'\n", tag_entry->mask->str);
}

static gboolean
gimp_tag_entry_commit_source_func        (GimpTagEntry         *tag_entry)
{
  gimp_tag_entry_commit_tags (GIMP_TAG_ENTRY (tag_entry));
  return FALSE;
}


static void
gimp_tag_entry_next_tag                  (GimpTagEntry         *tag_entry)
{
  gint  position = gtk_editable_get_position (GTK_EDITABLE (tag_entry));
  if (tag_entry->mask->str[position] != 'u')
    {
      while (position < tag_entry->mask->len
             && (tag_entry->mask->str[position] != 's'))
        {
          position++;
        }

      if (tag_entry->mask->str[position] == 's')
        {
          position++;
        }
    }
  else if (position < tag_entry->mask->len)
    {
      position++;
    }

  gtk_editable_set_position (GTK_EDITABLE (tag_entry), position);
}

static void
gimp_tag_entry_previous_tag              (GimpTagEntry         *tag_entry)
{
  gint  position = gtk_editable_get_position (GTK_EDITABLE (tag_entry));
  if (position >= 1
         && tag_entry->mask->str[position - 1] == 's')
    {
      position--;
    }
  if (position < 1)
    {
      return;
    }
  if (tag_entry->mask->str[position - 1] != 'u')
    {
      while (position > 0
             && (tag_entry->mask->str[position - 1] != 's'))
        {
          if (tag_entry->mask->str[position - 1] == 'u')
            {
              break;
            }

          position--;
        }
    }
  else
    {
      position--;
    }

  gtk_editable_set_position (GTK_EDITABLE (tag_entry), position);
}

static void
gimp_tag_entry_select_for_deletion       (GimpTagEntry         *tag_entry,
                                          GimpTagSearchDir      search_dir)
{
  gint          start_pos;
  gint          end_pos;

  gtk_editable_get_selection_bounds (GTK_EDITABLE (tag_entry), &start_pos, &end_pos);
  while (start_pos > 0
         && (tag_entry->mask->str[start_pos - 1] == 't'))
    {
      start_pos--;
    }
  if (search_dir == TAG_SEARCH_LEFT)
    {
      while (start_pos > 0
             && (tag_entry->mask->str[start_pos - 1] == 'w'))
        {
          start_pos--;
        }
    }

  if (end_pos > start_pos
      && (tag_entry->mask->str[end_pos - 1] == 't'
          || tag_entry->mask->str[end_pos - 1] == 's'))
    {
      while (end_pos <= tag_entry->mask->len
             && (tag_entry->mask->str[end_pos] == 's'))
        {
          end_pos++;
        }
      if (search_dir == TAG_SEARCH_RIGHT)
        {
          while (end_pos <= tag_entry->mask->len
                 && (tag_entry->mask->str[end_pos] == 'w'))
            {
              end_pos++;
            }
        }
    }

  gtk_editable_select_region (GTK_EDITABLE (tag_entry), start_pos, end_pos);
}

