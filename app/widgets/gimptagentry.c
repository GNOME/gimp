/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimptagentry.c
 * Copyright (C) 2008 Aurimas Juška <aurisj@svn.gnome.org>
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

#include <string.h>

#include <gegl.h>
#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>

#include "libgimpwidgets/gimpwidgets.h"

#include "widgets-types.h"

#include "core/gimp-utils.h"
#include "core/gimpcontainer.h"
#include "core/gimpcontext.h"
#include "core/gimptag.h"
#include "core/gimptagged.h"
#include "core/gimptaggedcontainer.h"
#include "core/gimpviewable.h"

#include "gimptagentry.h"

#include "gimp-intl.h"


#define GIMP_TAG_ENTRY_QUERY_DESC       _("filter")
#define GIMP_TAG_ENTRY_ASSIGN_DESC      _("enter tags")

#define GIMP_TAG_ENTRY_MAX_RECENT_ITEMS 20


typedef enum
{
  TAG_SEARCH_NONE,
  TAG_SEARCH_LEFT,
  TAG_SEARCH_RIGHT,
} GimpTagSearchDir;

enum
{
  PROP_0,
  PROP_CONTAINER,
  PROP_MODE
};


static void     gimp_tag_entry_dispose                   (GObject          *object);
static void     gimp_tag_entry_set_property              (GObject          *object,
                                                          guint             property_id,
                                                          const GValue     *value,
                                                          GParamSpec       *pspec);
static void     gimp_tag_entry_get_property              (GObject          *object,
                                                          guint             property_id,
                                                          GValue           *value,
                                                          GParamSpec       *pspec);
static void     gimp_tag_entry_activate                  (GtkEntry         *entry);
static void     gimp_tag_entry_changed                   (GtkEntry         *entry);
static void     gimp_tag_entry_insert_text               (GtkEditable      *editable,
                                                          gchar            *new_text,
                                                          gint              text_length,
                                                          gint             *position);
static void     gimp_tag_entry_delete_text               (GtkEditable      *editable,
                                                          gint              start_pos,
                                                          gint              end_pos);
static gboolean gimp_tag_entry_focus_in                  (GtkWidget        *widget,
                                                          GdkEventFocus    *event);
static gboolean gimp_tag_entry_focus_out                 (GtkWidget        *widget,
                                                          GdkEventFocus    *event);
static void     gimp_tag_entry_container_changed         (GimpContainer    *container,
                                                          GimpObject       *object,
                                                          GimpTagEntry     *entry);
static gboolean gimp_tag_entry_button_release            (GtkWidget        *widget,
                                                          GdkEventButton   *event);
static gboolean gimp_tag_entry_key_press                 (GtkWidget        *widget,
                                                          GdkEventKey      *event);
static gboolean gimp_tag_entry_query_tag                 (GimpTagEntry     *entry);

static void     gimp_tag_entry_assign_tags               (GimpTagEntry     *entry);
static void     gimp_tag_entry_load_selection            (GimpTagEntry     *entry,
                                                          gboolean          sort);
static void     gimp_tag_entry_find_common_tags          (gpointer          key,
                                                          gpointer          value,
                                                          gpointer          user_data);

static gchar *  gimp_tag_entry_get_completion_prefix     (GimpTagEntry     *entry);
static GList *  gimp_tag_entry_get_completion_candidates (GimpTagEntry     *entry,
                                                          gchar           **used_tags,
                                                          gchar            *prefix);
static gchar *  gimp_tag_entry_get_completion_string     (GimpTagEntry     *entry,
                                                          GList            *candidates,
                                                          gchar            *prefix);
static gboolean gimp_tag_entry_auto_complete             (GimpTagEntry     *entry);

static void     gimp_tag_entry_toggle_desc               (GimpTagEntry     *widget,
                                                          gboolean          show);
static gboolean gimp_tag_entry_draw                      (GtkWidget        *widget,
                                                          cairo_t          *cr);
static void     gimp_tag_entry_commit_region             (GString          *tags,
                                                          GString          *mask);
static void     gimp_tag_entry_commit_tags               (GimpTagEntry     *entry);
static gboolean gimp_tag_entry_commit_source_func        (GimpTagEntry     *entry);
static gboolean gimp_tag_entry_select_jellybean          (GimpTagEntry     *entry,
                                                          gint              selection_start,
                                                          gint              selection_end,
                                                          GimpTagSearchDir  search_dir);
static gboolean gimp_tag_entry_try_select_jellybean      (GimpTagEntry     *entry);

static gboolean gimp_tag_entry_add_to_recent             (GimpTagEntry     *entry,
                                                          const gchar      *tags_string,
                                                          gboolean          to_front);

static void     gimp_tag_entry_next_tag                  (GimpTagEntry     *entry,
                                                          gboolean          select);
static void     gimp_tag_entry_previous_tag              (GimpTagEntry     *entry,
                                                          gboolean          select);

static void     gimp_tag_entry_select_for_deletion       (GimpTagEntry     *entry,
                                                          GimpTagSearchDir  search_dir);
static gboolean gimp_tag_entry_strip_extra_whitespace    (GimpTagEntry     *entry);



G_DEFINE_TYPE (GimpTagEntry, gimp_tag_entry, GTK_TYPE_ENTRY)

#define parent_class gimp_tag_entry_parent_class


static void
gimp_tag_entry_class_init (GimpTagEntryClass *klass)
{
  GObjectClass   *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  object_class->dispose              = gimp_tag_entry_dispose;
  object_class->get_property         = gimp_tag_entry_get_property;
  object_class->set_property         = gimp_tag_entry_set_property;

  widget_class->button_release_event = gimp_tag_entry_button_release;

  g_object_class_install_property (object_class,
                                   PROP_CONTAINER,
                                   g_param_spec_object ("container",
                                                        "Tagged container",
                                                        "The Tagged container",
                                                        GIMP_TYPE_TAGGED_CONTAINER,
                                                        G_PARAM_CONSTRUCT_ONLY |
                                                        G_PARAM_READWRITE));

  g_object_class_install_property (object_class,
                                   PROP_MODE,
                                   g_param_spec_enum ("mode",
                                                      "Working mode",
                                                      "Mode in which to work.",
                                                      GIMP_TYPE_TAG_ENTRY_MODE,
                                                      GIMP_TAG_ENTRY_MODE_QUERY,
                                                      G_PARAM_CONSTRUCT_ONLY |
                                                      G_PARAM_READWRITE));
}

static void
gimp_tag_entry_init (GimpTagEntry *entry)
{
  entry->container            = NULL;
  entry->selected_items       = NULL;
  entry->common_tags          = NULL;
  entry->tab_completion_index = -1;
  entry->mode                 = GIMP_TAG_ENTRY_MODE_QUERY;
  entry->description_shown    = FALSE;
  entry->has_invalid_tags     = FALSE;
  entry->mask                 = g_string_new ("");

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
  g_signal_connect_after (entry, "draw",
                          G_CALLBACK (gimp_tag_entry_draw),
                          NULL);
}

static void
gimp_tag_entry_dispose (GObject *object)
{
  GimpTagEntry *entry = GIMP_TAG_ENTRY (object);

  if (entry->selected_items)
    {
      g_list_free (entry->selected_items);
      entry->selected_items = NULL;
    }

  if (entry->common_tags)
    {
      g_list_free_full (entry->common_tags, (GDestroyNotify) g_object_unref);
      entry->common_tags = NULL;
    }

  if (entry->recent_list)
    {
      g_list_free_full (entry->recent_list, (GDestroyNotify) g_free);
      entry->recent_list = NULL;
    }

  if (entry->container)
    {
      g_signal_handlers_disconnect_by_func (entry->container,
                                            gimp_tag_entry_container_changed,
                                            entry);
      g_object_unref (entry->container);
      entry->container = NULL;
    }

  if (entry->mask)
    {
      g_string_free (entry->mask, TRUE);
      entry->mask = NULL;
    }

  if (entry->tag_query_idle_id)
    {
      g_source_remove (entry->tag_query_idle_id);
      entry->tag_query_idle_id = 0;
    }

  G_OBJECT_CLASS (parent_class)->dispose (object);
}

static void
gimp_tag_entry_set_property (GObject      *object,
                             guint         property_id,
                             const GValue *value,
                             GParamSpec   *pspec)
{
  GimpTagEntry *entry = GIMP_TAG_ENTRY (object);

  switch (property_id)
    {
    case PROP_CONTAINER:
      entry->container = g_value_dup_object (value);
      g_signal_connect (entry->container, "add",
                        G_CALLBACK (gimp_tag_entry_container_changed),
                        entry);
      g_signal_connect (entry->container, "remove",
                        G_CALLBACK (gimp_tag_entry_container_changed),
                        entry);
      break;

    case PROP_MODE:
      entry->mode = g_value_get_enum (value);
      gimp_tag_entry_toggle_desc (entry, TRUE);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_tag_entry_get_property (GObject    *object,
                             guint       property_id,
                             GValue     *value,
                             GParamSpec *pspec)
{
  GimpTagEntry *entry = GIMP_TAG_ENTRY (object);

  switch (property_id)
    {
    case PROP_CONTAINER:
      g_value_set_object (value, entry->container);
      break;

    case PROP_MODE:
      g_value_set_enum (value, entry->mode);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

/**
 * gimp_tag_entry_new:
 * @container: a #GimpTaggedContainer object
 * @mode:      #GimpTagEntryMode to work in.
 *
 * #GimpTagEntry is a widget which can query and assign tags to tagged objects.
 * When operating in query mode, @container is kept up to date with
 * tags selected. When operating in assignment mode, tags are assigned to
 * objects selected and visible in @container.
 *
 * Return value: a new GimpTagEntry widget.
 **/
GtkWidget *
gimp_tag_entry_new (GimpTaggedContainer *container,
                    GimpTagEntryMode     mode)
{
  g_return_val_if_fail (GIMP_IS_TAGGED_CONTAINER (container), NULL);

  return g_object_new (GIMP_TYPE_TAG_ENTRY,
                       "container", container,
                       "mode",      mode,
                       NULL);
}

static void
gimp_tag_entry_activate (GtkEntry *entry)
{
  GimpTagEntry *tag_entry = GIMP_TAG_ENTRY (entry);
  gint          selection_start;
  gint          selection_end;
  GList        *list;

  gimp_tag_entry_toggle_desc (tag_entry, FALSE);

  gtk_editable_get_selection_bounds (GTK_EDITABLE (entry),
                                     &selection_start, &selection_end);
  if (selection_start != selection_end)
    {
      gtk_editable_select_region (GTK_EDITABLE (entry),
                                  selection_end, selection_end);
    }

  for (list = tag_entry->selected_items; list; list = g_list_next (list))
    {
      if (gimp_container_have (GIMP_CONTAINER (tag_entry->container),
                               GIMP_OBJECT (list->data)))
        {
          break;
        }
    }

  if (tag_entry->mode == GIMP_TAG_ENTRY_MODE_ASSIGN && list)
    {
      gimp_tag_entry_assign_tags (GIMP_TAG_ENTRY (entry));
    }
}

/**
 * gimp_tag_entry_set_tag_string:
 * @entry:      a #GimpTagEntry object.
 * @tag_string: string of tags, separated by any terminal punctuation
 *              character.
 *
 * Sets tags from @tag_string to @tag_entry. Given tags do not need to
 * be valid as they can be fixed or dropped automatically. Depending on
 * selected #GimpTagEntryMode, appropriate action is performed.
 **/
void
gimp_tag_entry_set_tag_string (GimpTagEntry *entry,
                               const gchar  *tag_string)
{
  g_return_if_fail (GIMP_IS_TAG_ENTRY (entry));

  entry->internal_operation++;
  entry->suppress_tag_query++;

  gtk_entry_set_text (GTK_ENTRY (entry), tag_string);
  gtk_editable_set_position (GTK_EDITABLE (entry), -1);

  entry->suppress_tag_query--;
  entry->internal_operation--;

  gimp_tag_entry_commit_tags (entry);

  if (entry->mode == GIMP_TAG_ENTRY_MODE_ASSIGN)
    {
      gimp_tag_entry_assign_tags (entry);
    }
  else if (entry->mode == GIMP_TAG_ENTRY_MODE_QUERY)
    {
      gimp_tag_entry_query_tag (entry);
    }
}

static void
gimp_tag_entry_changed (GtkEntry *entry)
{
  GimpTagEntry *tag_entry = GIMP_TAG_ENTRY (entry);
  gchar        *text;

  text = g_strdup (gtk_entry_get_text (entry));
  text = g_strstrip (text);

  if (! gtk_widget_has_focus (GTK_WIDGET (entry)) &&
      strlen (text) == 0)
    {
      gimp_tag_entry_toggle_desc (tag_entry, TRUE);
    }
  else
    {
      gimp_tag_entry_toggle_desc (tag_entry, FALSE);
    }

  g_free (text);

  if (tag_entry->mode == GIMP_TAG_ENTRY_MODE_QUERY &&
      ! tag_entry->suppress_tag_query              &&
      ! tag_entry->tag_query_idle_id)
    {
      tag_entry->tag_query_idle_id =
        g_idle_add ((GSourceFunc) gimp_tag_entry_query_tag, entry);
    }
}

static void
gimp_tag_entry_insert_text (GtkEditable *editable,
                            gchar       *new_text,
                            gint         text_length,
                            gint        *position)
{
  GimpTagEntry *entry = GIMP_TAG_ENTRY (editable);
  gboolean      is_tag[2];
  gint          i;
  gint          insert_pos = *position;
  glong         num_chars;

  num_chars = g_utf8_strlen (new_text, text_length);

  if (! entry->internal_operation)
    {
      /* suppress tag queries until auto completion runs */
      entry->suppress_tag_query++;
    }

  is_tag[0] = FALSE;
  if (*position > 0)
    {
      is_tag[0] = (entry->mask->str[*position - 1] == 't' ||
                   entry->mask->str[*position - 1] == 's');
    }

  is_tag[1] = (entry->mask->str[*position] == 't' ||
               entry->mask->str[*position] == 's');

  if (is_tag[0] && is_tag[1])
    {
      g_signal_stop_emission_by_name (editable, "insert-text");
    }
  else if (num_chars > 0)
    {
      gunichar  c = g_utf8_get_char (new_text);

      if (! entry->internal_operation            &&
          *position > 0                          &&
          entry->mask->str[*position - 1] == 's' &&
          ! g_unichar_isspace (c))
        {
          if (! entry->suppress_mask_update)
            {
              g_string_insert_c (entry->mask, *position, 'u');
            }

          g_signal_handlers_block_by_func (editable,
                                           gimp_tag_entry_insert_text,
                                           NULL);

          gtk_editable_insert_text (editable, " ", 1, position);
          gtk_editable_insert_text (editable, new_text, text_length, position);

          g_signal_handlers_unblock_by_func (editable,
                                             gimp_tag_entry_insert_text,
                                             NULL);

          g_signal_stop_emission_by_name (editable, "insert-text");
        }
      else if (! entry->internal_operation        &&
               num_chars == 1                     &&
               *position < entry->mask->len       &&
               entry->mask->str[*position] == 't' &&
               ! g_unichar_isspace (c))
        {
          if (! entry->suppress_mask_update)
            {
              g_string_insert_c (entry->mask, *position, 'u');
            }

          g_signal_handlers_block_by_func (editable,
                                           gimp_tag_entry_insert_text,
                                           NULL);

          gtk_editable_insert_text (editable, new_text, text_length, position);
          gtk_editable_insert_text (editable, " ", 1, position);
          (*position)--;

          g_signal_handlers_unblock_by_func (editable,
                                             gimp_tag_entry_insert_text,
                                             NULL);

          g_signal_stop_emission_by_name (editable, "insert-text");
        }

      if (! entry->suppress_mask_update)
        {
          for (i = 0; i < num_chars; i++)
            {
              g_string_insert_c (entry->mask, insert_pos + i, 'u');
            }
        }
    }

  if (! entry->internal_operation)
    {
      entry->tab_completion_index = -1;
      g_idle_add ((GSourceFunc) gimp_tag_entry_auto_complete, editable);
    }
}

static void
gimp_tag_entry_delete_text (GtkEditable *editable,
                            gint         start_pos,
                            gint         end_pos)
{
  GimpTagEntry *entry = GIMP_TAG_ENTRY (editable);

  if (! entry->internal_operation)
    {
      g_signal_handlers_block_by_func (editable,
                                       gimp_tag_entry_delete_text,
                                       NULL);

      if (end_pos > start_pos &&
          (entry->mask->str[end_pos - 1] == 't' ||
           entry->mask->str[end_pos - 1] == 's'))
        {
          while (end_pos <= entry->mask->len &&
                 (entry->mask->str[end_pos] == 's'))
            {
              end_pos++;
            }
        }

      gtk_editable_delete_text (editable, start_pos, end_pos);
      if (! entry->suppress_mask_update)
        {
          g_string_erase (entry->mask, start_pos, end_pos - start_pos);
        }

      g_signal_handlers_unblock_by_func (editable,
                                         gimp_tag_entry_delete_text,
                                         NULL);

      g_signal_stop_emission_by_name (editable, "delete-text");
    }
  else
    {
      if (! entry->suppress_mask_update)
        {
          g_string_erase (entry->mask, start_pos, end_pos - start_pos);
        }
    }
}

static gboolean
gimp_tag_entry_query_tag (GimpTagEntry *entry)
{
  gchar    **parsed_tags;
  gint       count;
  gint       i;
  GList     *query_list = NULL;
  gboolean   has_invalid_tags;

  entry->tag_query_idle_id = 0;

  if (entry->suppress_tag_query)
    return FALSE;

  has_invalid_tags = FALSE;

  parsed_tags = gimp_tag_entry_parse_tags (entry);
  count = g_strv_length (parsed_tags);
  for (i = 0; i < count; i++)
    {
      if (strlen (parsed_tags[i]) > 0)
        {
          GimpTag *tag = gimp_tag_try_new (parsed_tags[i]);

          if (! tag)
            has_invalid_tags = TRUE;

          query_list = g_list_append (query_list, tag);
        }
    }
  g_strfreev (parsed_tags);

  gimp_tagged_container_set_filter (GIMP_TAGGED_CONTAINER (entry->container),
                                    query_list);

  g_list_free_full (query_list, (GDestroyNotify) gimp_tag_or_null_unref);

  if (has_invalid_tags != entry->has_invalid_tags)
    {
      entry->has_invalid_tags = has_invalid_tags;
      gtk_widget_queue_draw (GTK_WIDGET (entry));
    }

  return FALSE;
}

static gboolean
gimp_tag_entry_auto_complete (GimpTagEntry *tag_entry)
{
  GtkEntry  *entry = GTK_ENTRY (tag_entry);
  gchar     *completion_prefix;
  GList     *completion_candidates;
  gint       candidate_count = 0;
  gchar    **tags;
  gchar     *completion;
  gint       start_position;
  gint       end_position;

  tag_entry->suppress_tag_query--;
  if (tag_entry->mode == GIMP_TAG_ENTRY_MODE_QUERY)
    {
      /* tag query was suppressed until we got to auto completion (here),
       * now queue tag query
       */
      tag_entry->tag_query_idle_id =
        g_idle_add ((GSourceFunc) gimp_tag_entry_query_tag, tag_entry);
    }

  if (tag_entry->tab_completion_index >= 0)
    {
      tag_entry->internal_operation++;
      tag_entry->suppress_tag_query++;
      gtk_editable_delete_selection (GTK_EDITABLE (tag_entry));
      tag_entry->suppress_tag_query--;
      tag_entry->internal_operation--;
    }

  gtk_editable_get_selection_bounds (GTK_EDITABLE (tag_entry),
                                     &start_position, &end_position);
  if (start_position != end_position)
    {
      /* only autocomplete what user types,
       * not was autocompleted in the previous step.
       */
      return FALSE;
    }

  completion_prefix =
    gimp_tag_entry_get_completion_prefix (GIMP_TAG_ENTRY (entry));
  tags = gimp_tag_entry_parse_tags (GIMP_TAG_ENTRY (entry));
  completion_candidates =
    gimp_tag_entry_get_completion_candidates (GIMP_TAG_ENTRY (entry),
                                              tags,
                                              completion_prefix);
  completion_candidates = g_list_sort (completion_candidates,
                                       gimp_tag_compare_func);

  if (tag_entry->tab_completion_index >= 0 && completion_candidates)
    {
      GimpTag *the_chosen_one;

      candidate_count = g_list_length (completion_candidates);
      tag_entry->tab_completion_index %= candidate_count;
      the_chosen_one = g_list_nth_data (completion_candidates,
                                        tag_entry->tab_completion_index);
      g_list_free (completion_candidates);
      completion_candidates = NULL;
      completion_candidates = g_list_append (completion_candidates,
                                             the_chosen_one);
    }

  completion = gimp_tag_entry_get_completion_string (GIMP_TAG_ENTRY (entry),
                                                     completion_candidates,
                                                     completion_prefix);

  if (completion && strlen (completion) > 0)
    {
      start_position = gtk_editable_get_position (GTK_EDITABLE (entry));
      end_position = start_position;
      tag_entry->internal_operation++;
      gtk_editable_insert_text (GTK_EDITABLE (entry),
                                completion, strlen (completion),
                                &end_position);
      tag_entry->internal_operation--;
      if (tag_entry->tab_completion_index >= 0 &&
          candidate_count == 1)
        {
          gtk_editable_set_position (GTK_EDITABLE (entry), end_position);
        }
      else
        {
          gtk_editable_select_region (GTK_EDITABLE (entry),
                                      start_position, end_position);
        }
    }

  g_free (completion);
  g_strfreev (tags);
  g_list_free (completion_candidates);
  g_free (completion_prefix);

  return FALSE;
}

static void
gimp_tag_entry_assign_tags (GimpTagEntry *tag_entry)
{
  gchar **parsed_tags;
  gint    count;
  gint    i;
  GList  *resource_iter;
  GList  *tag_iter;
  GList  *dont_remove_list = NULL;
  GList  *remove_list      = NULL;
  GList  *add_list         = NULL;
  GList  *common_tags      = NULL;

  parsed_tags = gimp_tag_entry_parse_tags (tag_entry);

  count = g_strv_length (parsed_tags);
  for (i = 0; i < count; i++)
    {
      GimpTag *tag = gimp_tag_new (parsed_tags[i]);

      if (tag)
        {
          if (g_list_find_custom (tag_entry->common_tags, tag,
                                  gimp_tag_compare_func))
            {
              dont_remove_list = g_list_prepend (dont_remove_list, tag);
            }
          else
            {
              add_list = g_list_prepend (add_list, g_object_ref (tag));
            }

          common_tags = g_list_prepend (common_tags, tag);
        }
    }

  g_strfreev (parsed_tags);

  /* find common tags which were removed. */
  for (tag_iter = tag_entry->common_tags;
       tag_iter;
       tag_iter = g_list_next (tag_iter))
    {
      if (! g_list_find_custom (dont_remove_list, tag_iter->data,
                                gimp_tag_compare_func))
        {
          remove_list = g_list_prepend (remove_list,
                                        g_object_ref (tag_iter->data));
        }
    }

  g_list_free (dont_remove_list);

  for (resource_iter = tag_entry->selected_items;
       resource_iter;
       resource_iter = g_list_next (resource_iter))
    {
      GimpTagged *tagged = GIMP_TAGGED (resource_iter->data);

      for (tag_iter = remove_list; tag_iter; tag_iter = g_list_next (tag_iter))
        {
          gimp_tagged_remove_tag (tagged, tag_iter->data);
        }

      for (tag_iter = add_list; tag_iter; tag_iter = g_list_next (tag_iter))
        {
          gimp_tagged_add_tag (tagged, tag_iter->data);
        }
    }

  g_list_free_full (add_list, (GDestroyNotify) g_object_unref);
  g_list_free_full (remove_list, (GDestroyNotify) g_object_unref);

  /* common tags list with changes applied. */
  g_list_free_full (tag_entry->common_tags, (GDestroyNotify) g_object_unref);
  tag_entry->common_tags = common_tags;
}

/**
 * gimp_tag_entry_parse_tags:
 * @entry: a #GimpTagEntry widget.
 *
 * Parses currently entered tags from @entry. Tags do not need to be
 * valid as they are fixed when necessary. Only valid tags are
 * returned.
 *
 * Return value: a newly allocated NULL terminated list of strings. It
 *               should be freed using g_strfreev().
 **/
gchar **
gimp_tag_entry_parse_tags (GimpTagEntry *entry)
{
  gchar       **parsed_tags;
  gint          length;
  gint          i;
  GString      *parsed_tag;
  const gchar  *cursor;
  GList        *tag_list = NULL;
  GList        *iterator;
  gunichar      c;

  g_return_val_if_fail (GIMP_IS_TAG_ENTRY (entry), NULL);

  parsed_tag = g_string_new ("");
  cursor = gtk_entry_get_text (GTK_ENTRY (entry));
  do
    {
      c = g_utf8_get_char (cursor);
      cursor = g_utf8_next_char (cursor);

      if (! c || gimp_tag_is_tag_separator (c))
        {
          if (parsed_tag->len > 0)
            {
              gchar *validated_tag = gimp_tag_string_make_valid (parsed_tag->str);

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
    }
  while (c);

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

  g_list_free (tag_list);

  return parsed_tags;
}

/**
 * gimp_tag_entry_set_selected_items:
 * @tag_entry:  a #GimpTagEntry widget.
 * @items:      a list of #GimpTagged objects.
 *
 * Set list of currently selected #GimpTagged objects. Only selected and
 * visible (not filtered out) #GimpTagged objects are assigned tags when
 * operating in tag assignment mode.
 **/
void
gimp_tag_entry_set_selected_items (GimpTagEntry *tag_entry,
                                   GList        *items)
{
  g_return_if_fail (GIMP_IS_TAG_ENTRY (tag_entry));

  if (tag_entry->selected_items)
    {
      g_list_free (tag_entry->selected_items);
      tag_entry->selected_items = NULL;
    }

  if (tag_entry->common_tags)
    {
      g_list_free_full (tag_entry->common_tags, (GDestroyNotify) g_object_unref);
      tag_entry->common_tags = NULL;
    }

  tag_entry->selected_items = g_list_copy (items);

  if (tag_entry->mode == GIMP_TAG_ENTRY_MODE_ASSIGN)
    {
      gimp_tag_entry_load_selection (tag_entry, TRUE);
    }
}

static void
gimp_tag_entry_load_selection (GimpTagEntry *tag_entry,
                               gboolean      sort)
{
  GList      *list;
  gint        insert_pos;
  GHashTable *refcounts;
  GList      *resource;
  GList      *tag;

  tag_entry->internal_operation++;
  gtk_editable_delete_text (GTK_EDITABLE (tag_entry), 0, -1);
  tag_entry->internal_operation--;

  if (! tag_entry->selected_items)
    {
      gimp_tag_entry_toggle_desc (tag_entry, FALSE);
      return;
    }

  refcounts = g_hash_table_new ((GHashFunc) gimp_tag_get_hash,
                                (GEqualFunc) gimp_tag_equals);

  /* find set of tags common to all resources. */
  for (resource = tag_entry->selected_items;
       resource;
       resource = g_list_next (resource))
    {
      for (tag = gimp_tagged_get_tags (GIMP_TAGGED (resource->data));
           tag;
           tag = g_list_next (tag))
        {
          /* count refcount for each tag */
          guint refcount = GPOINTER_TO_UINT (g_hash_table_lookup (refcounts,
                                                                  tag->data));

          g_hash_table_insert (refcounts, tag->data,
                               GUINT_TO_POINTER (refcount + 1));
        }
    }

  g_hash_table_foreach (refcounts, gimp_tag_entry_find_common_tags, tag_entry);

  g_hash_table_destroy (refcounts);

  tag_entry->common_tags = g_list_sort (tag_entry->common_tags,
                                        gimp_tag_compare_func);

  insert_pos = gtk_editable_get_position (GTK_EDITABLE (tag_entry));

  for (list = tag_entry->common_tags; list; list = g_list_next (list))
    {
      GimpTag *tag = list->data;
      gchar   *text;

      text = g_strdup_printf ("%s%s ",
                              gimp_tag_get_name (tag),
                              gimp_tag_entry_get_separator ());

      tag_entry->internal_operation++;
      gtk_editable_insert_text (GTK_EDITABLE (tag_entry), text, strlen (text),
                                &insert_pos);
      tag_entry->internal_operation--;

      g_free (text);
    }

  gimp_tag_entry_commit_tags (tag_entry);
}

static void
gimp_tag_entry_find_common_tags (gpointer key,
                                 gpointer value,
                                 gpointer user_data)
{
  guint         ref_count = GPOINTER_TO_UINT (value);
  GimpTagEntry *tag_entry = GIMP_TAG_ENTRY (user_data);

  /* FIXME: more efficient list length */
  if (ref_count == g_list_length (tag_entry->selected_items))
    {
      tag_entry->common_tags = g_list_prepend (tag_entry->common_tags,
                                               g_object_ref (key));
    }
}

static gchar *
gimp_tag_entry_get_completion_prefix (GimpTagEntry *entry)
{
  gchar *original_string;
  gchar *prefix_start;
  gchar *prefix;
  gchar *cursor;
  gint   position;
  gint   i;

  position = gtk_editable_get_position (GTK_EDITABLE (entry));
  if (position < 1 ||
      entry->mask->str[position - 1] != 'u')
    {
      return g_strdup ("");
    }

  original_string = g_strdup (gtk_entry_get_text (GTK_ENTRY (entry)));
  cursor = original_string;
  prefix_start = original_string;
  for (i = 0; i < position; i++)
    {
      gunichar c;

      c = g_utf8_get_char (cursor);
      cursor = g_utf8_next_char (cursor);

      if (gimp_tag_is_tag_separator (c))
        prefix_start = cursor;
    }
  *cursor = '\0';

  prefix = g_strdup (g_strchug (prefix_start));
  g_free (original_string);

  return prefix;
}

static GList *
gimp_tag_entry_get_completion_candidates (GimpTagEntry  *tag_entry,
                                          gchar        **used_tags,
                                          gchar         *src_prefix)
{
  GList *candidates = NULL;
  GList *all_tags;
  GList *list;
  gint   length;
  gchar *prefix;

  if (! src_prefix || strlen (src_prefix) < 1)
    return NULL;

  prefix = g_utf8_normalize (src_prefix, -1, G_NORMALIZE_ALL);
  if (! prefix)
    return NULL;

  all_tags = g_hash_table_get_keys (tag_entry->container->tag_ref_counts);
  length = g_strv_length (used_tags);

  for (list = all_tags; list; list = g_list_next (list))
    {
      GimpTag *tag = list->data;

      if (gimp_tag_has_prefix (tag, prefix))
        {
          gint i;

          /* check if tag is not already entered */
          for (i = 0; i < length; i++)
            {
              if (! gimp_tag_compare_with_string (tag, used_tags[i]))
                break;
            }

          if (i == length)
            candidates = g_list_append (candidates, list->data);
        }
    }

  g_list_free (all_tags);

  g_free (prefix);

  return candidates;
}

static gchar *
gimp_tag_entry_get_completion_string (GimpTagEntry *tag_entry,
                                      GList        *candidates,
                                      gchar        *prefix)
{
  const gchar **completions;
  guint         length;
  guint         i;
  GList        *candidate_iterator;
  const gchar  *candidate_string;
  gint          prefix_length;
  gunichar      c;
  gint          num_chars_match;
  gchar        *completion;
  gchar        *completion_end;
  gint          completion_length;
  gchar        *normalized_prefix;

  if (! candidates)
    return NULL;

  normalized_prefix = g_utf8_normalize (prefix, -1, G_NORMALIZE_ALL);
  if (! normalized_prefix)
    return NULL;

  prefix_length = strlen (normalized_prefix);
  g_free (normalized_prefix);

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
      if (! c)
        break;

      for (i = 1; i < length; i++)
        {
          gunichar d = g_utf8_get_char (completions[i]);

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
    }
  while (c);

  g_free (completions);

  candidate_string = gimp_tag_get_name (GIMP_TAG (candidates->data));

  return g_strdup (candidate_string + prefix_length);
}

static gboolean
gimp_tag_entry_focus_in (GtkWidget     *widget,
                         GdkEventFocus *event)
{
  gimp_tag_entry_toggle_desc (GIMP_TAG_ENTRY (widget), FALSE);

  return FALSE;
}

static gboolean
gimp_tag_entry_focus_out (GtkWidget     *widget,
                          GdkEventFocus *event)
{
  GimpTagEntry  *tag_entry = GIMP_TAG_ENTRY (widget);

  gimp_tag_entry_commit_tags (tag_entry);
  if (tag_entry->mode == GIMP_TAG_ENTRY_MODE_ASSIGN)
    {
      gimp_tag_entry_assign_tags (GIMP_TAG_ENTRY (widget));
    }

  gimp_tag_entry_add_to_recent (tag_entry,
                                gtk_entry_get_text (GTK_ENTRY (widget)),
                                TRUE);

  gimp_tag_entry_toggle_desc (tag_entry, TRUE);

  return FALSE;
}

static void
gimp_tag_entry_container_changed (GimpContainer *container,
                                  GimpObject    *object,
                                  GimpTagEntry  *tag_entry)
{
  GList *list;

  if (! gimp_container_have (GIMP_CONTAINER (tag_entry->container),
                             object))
    {
      GList *selected_items = NULL;

      for (list = tag_entry->selected_items; list; list = g_list_next (list))
        {
          if (list->data != object)
            selected_items = g_list_prepend (selected_items, list->data);
        }

      selected_items = g_list_reverse (selected_items);
      gimp_tag_entry_set_selected_items (tag_entry, selected_items);
      g_list_free (selected_items);
    }

  if (tag_entry->mode == GIMP_TAG_ENTRY_MODE_ASSIGN)
    {
      for (list = tag_entry->selected_items; list; list = g_list_next (list))
        {
          if (gimp_tagged_get_tags (GIMP_TAGGED (list->data)) &&
              gimp_container_have (GIMP_CONTAINER (tag_entry->container),
                                   GIMP_OBJECT (list->data)))
            {
              break;
            }
        }

      if (! list)
        {
          tag_entry->internal_operation++;
          gtk_editable_delete_text (GTK_EDITABLE (tag_entry), 0, -1);
          tag_entry->internal_operation--;
        }
    }
}

static void
gimp_tag_entry_toggle_desc (GimpTagEntry *tag_entry,
                            gboolean      show)
{
  GtkWidget *widget = GTK_WIDGET (tag_entry);

  if (! (show ^ tag_entry->description_shown))
    return;

  if (show)
    {
      gchar  *current_text;
      size_t  len;

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
gimp_tag_entry_draw (GtkWidget *widget,
                     cairo_t   *cr)
{
  GimpTagEntry   *tag_entry = GIMP_TAG_ENTRY (widget);
  GdkWindow      *window;
  PangoLayout    *layout;
  PangoAttrList  *attr_list;
  PangoAttribute *attribute;
  gint            layout_width;
  gint            layout_height;
  gint            window_width;
  gint            window_height;
  gint            offset;
  const char     *display_text;

  window = gtk_entry_get_text_window (GTK_ENTRY (widget));

  if (! gtk_cairo_should_draw_window (cr, window))
    return FALSE;

  if (! GIMP_TAG_ENTRY (widget)->description_shown)
    return FALSE;

  if (tag_entry->mode == GIMP_TAG_ENTRY_MODE_QUERY)
    {
      display_text = GIMP_TAG_ENTRY_QUERY_DESC;
    }
  else
    {
      display_text = GIMP_TAG_ENTRY_ASSIGN_DESC;
    }

  layout = gtk_widget_create_pango_layout (widget, display_text);

  attr_list = pango_attr_list_new ();
  attribute = pango_attr_style_new (PANGO_STYLE_ITALIC);
  pango_attr_list_insert (attr_list, attribute);

  pango_layout_set_attributes (layout, attr_list);
  pango_attr_list_unref (attr_list);

  window_width  = gdk_window_get_width  (window);
  window_height = gdk_window_get_height (window);
  pango_layout_get_size (layout,
                         &layout_width, &layout_height);
  offset = (window_height - PANGO_PIXELS (layout_height)) / 2;

  gtk_paint_layout (gtk_widget_get_style (widget),
                    cr,
                    GTK_STATE_INSENSITIVE,
                    TRUE,
                    widget,
                    NULL,
                    (gtk_widget_get_direction (widget) == GTK_TEXT_DIR_RTL) ?
                    window_width - PANGO_PIXELS (layout_width) - offset :
                    offset,
                    offset,
                    layout);

  g_object_unref (layout);

  return FALSE;
}

static gboolean
gimp_tag_entry_key_press (GtkWidget   *widget,
                          GdkEventKey *event)
{
  GimpTagEntry    *entry = GIMP_TAG_ENTRY (widget);
  GdkModifierType  extend_mask;
  guchar           c;

  extend_mask =
    gtk_widget_get_modifier_mask (widget,
                                  GDK_MODIFIER_INTENT_EXTEND_SELECTION);

  c = gdk_keyval_to_unicode (event->keyval);
  if (gimp_tag_is_tag_separator (c))
    {
      g_idle_add ((GSourceFunc) gimp_tag_entry_commit_source_func, entry);
      return FALSE;
    }

  switch (event->keyval)
    {
    case GDK_KEY_Tab:
    case GDK_KEY_KP_Tab:
    case GDK_KEY_ISO_Left_Tab:
      /*  allow to leave the widget with Ctrl+Tab  */
      if (! (event->state & GDK_CONTROL_MASK))
        {
          entry->tab_completion_index++;
          entry->suppress_tag_query++;
          g_idle_add ((GSourceFunc) gimp_tag_entry_auto_complete, entry);
        }
      else
        {
          gimp_tag_entry_commit_tags (entry);
          g_signal_emit_by_name (widget, "move-focus",
                                 (event->state & GDK_SHIFT_MASK) ?
                                 GTK_DIR_TAB_BACKWARD : GTK_DIR_TAB_FORWARD);
        }
      return TRUE;

    case GDK_KEY_Return:
      gimp_tag_entry_commit_tags (entry);
      break;

    case GDK_KEY_Left:
      gimp_tag_entry_previous_tag (entry,
                                   (event->state & extend_mask) ? TRUE : FALSE);
      return TRUE;

    case GDK_KEY_Right:
      gimp_tag_entry_next_tag (entry,
                               (event->state & extend_mask) ? TRUE : FALSE);
      return TRUE;

    case GDK_KEY_BackSpace:
      {
        gint selection_start;
        gint selection_end;

        gtk_editable_get_selection_bounds (GTK_EDITABLE (entry),
                                           &selection_start, &selection_end);
        if (gimp_tag_entry_select_jellybean (entry,
                                             selection_start, selection_end,
                                             TAG_SEARCH_LEFT))
          {
            return TRUE;
          }
        else
          {
            gimp_tag_entry_select_for_deletion (entry, TAG_SEARCH_LEFT);
            /* FIXME: need to remove idle handler in dispose */
            g_idle_add ((GSourceFunc) gimp_tag_entry_strip_extra_whitespace,
                        entry);
          }
      }
      break;

    case GDK_KEY_Delete:
      {
        gint selection_start;
        gint selection_end;

        gtk_editable_get_selection_bounds (GTK_EDITABLE (entry),
                                           &selection_start, &selection_end);
        if (gimp_tag_entry_select_jellybean (entry,
                                             selection_start, selection_end,
                                             TAG_SEARCH_RIGHT))
          {
            return TRUE;
          }
        else
          {
            gimp_tag_entry_select_for_deletion (entry, TAG_SEARCH_RIGHT);
            /* FIXME: need to remove idle handler in dispose */
            g_idle_add ((GSourceFunc) gimp_tag_entry_strip_extra_whitespace,
                        entry);
          }
      }
      break;

    case GDK_KEY_Up:
    case GDK_KEY_Down:
      if (entry->recent_list != NULL)
        {
          gchar *recent_item;
          gchar *very_recent_item;

          very_recent_item = g_strdup (gtk_entry_get_text (GTK_ENTRY (entry)));
          gimp_tag_entry_add_to_recent (entry, very_recent_item, TRUE);
          g_free (very_recent_item);

          if (event->keyval == GDK_KEY_Up)
            {
              recent_item = (gchar *) g_list_first (entry->recent_list)->data;
              entry->recent_list = g_list_remove (entry->recent_list, recent_item);
              entry->recent_list = g_list_append (entry->recent_list, recent_item);
            }
          else
            {
              recent_item = (gchar *) g_list_last (entry->recent_list)->data;
              entry->recent_list = g_list_remove (entry->recent_list, recent_item);
              entry->recent_list = g_list_prepend (entry->recent_list, recent_item);
            }

          recent_item = (gchar *) g_list_first (entry->recent_list)->data;
          entry->internal_operation++;
          gtk_entry_set_text (GTK_ENTRY (entry), recent_item);
          gtk_editable_set_position (GTK_EDITABLE (entry), -1);
          entry->internal_operation--;
        }
      return TRUE;

    default:
      break;
    }

  return FALSE;
}

static gboolean
gimp_tag_entry_button_release (GtkWidget      *widget,
                               GdkEventButton *event)
{
  if (event->button == 1)
    {
      /* FIXME: need to remove idle handler in dispose */
      g_idle_add ((GSourceFunc) gimp_tag_entry_try_select_jellybean,
                  widget);
    }

  return GTK_WIDGET_CLASS (parent_class)->button_release_event (widget, event);
}

static gboolean
gimp_tag_entry_try_select_jellybean (GimpTagEntry *entry)
{
  gint selection_start;
  gint selection_end;
  gint selection_pos = gtk_editable_get_position (GTK_EDITABLE (entry));
  gint char_count    = g_utf8_strlen (gtk_entry_get_text (GTK_ENTRY (entry)), -1);

  if (selection_pos == char_count)
    {
      return FALSE;
    }

  gtk_editable_get_selection_bounds (GTK_EDITABLE (entry),
                                     &selection_start, &selection_end);
  gimp_tag_entry_select_jellybean (entry, selection_start, selection_end,
                                   TAG_SEARCH_NONE);

  return FALSE;
}

static gboolean
gimp_tag_entry_select_jellybean (GimpTagEntry     *entry,
                                 gint              selection_start,
                                 gint              selection_end,
                                 GimpTagSearchDir  search_dir)
{
  gint prev_selection_start;
  gint prev_selection_end;

  if (! entry->mask->len)
    {
      return FALSE;
    }

  if (selection_start >= entry->mask->len)
    {
      selection_start = entry->mask->len - 1;
      selection_end   = selection_start;
    }

  if (entry->mask->str[selection_start] == 'u')
    {
      return FALSE;
    }

  switch (search_dir)
    {
    case TAG_SEARCH_NONE:
      if (selection_start > 0 &&
          entry->mask->str[selection_start] == 's')
        {
          selection_start--;
        }

      if (selection_start > 0                            &&
          (entry->mask->str[selection_start - 1] == 'w') &&
          (entry->mask->str[selection_start] == 't'))
        {
          /* between whitespace and tag,
           * should allow to select tag.
           */
          selection_start--;
        }
      break;

    case TAG_SEARCH_LEFT:
      if (selection_start == selection_end)
        {
          if (selection_start > 0                      &&
              entry->mask->str[selection_start] == 't' &&
              entry->mask->str[selection_start - 1] == 'w')
            {
              selection_start--;
            }

          if ((entry->mask->str[selection_start] == 'w' ||
               entry->mask->str[selection_start] == 's') &&
              selection_start > 0)
            {
              while ((entry->mask->str[selection_start] == 'w' ||
                      entry->mask->str[selection_start] == 's') &&
                     selection_start > 0)
                {
                  selection_start--;
                }

              selection_end = selection_start + 1;
            }
        }
      break;

    case TAG_SEARCH_RIGHT:
      if (selection_start == selection_end)
        {
          if ((entry->mask->str[selection_start] == 'w' ||
               entry->mask->str[selection_start] == 's') &&
              selection_start < entry->mask->len - 1)
            {
              while ((entry->mask->str[selection_start] == 'w' ||
                      entry->mask->str[selection_start] == 's') &&
                     selection_start < entry->mask->len - 1)
                {
                  selection_start++;
                }

              selection_end = selection_start + 1;
            }
        }
      break;
    }

  if (selection_start < entry->mask->len &&
      selection_start == selection_end)
    {
      selection_end = selection_start + 1;
    }

  gtk_editable_get_selection_bounds (GTK_EDITABLE (entry),
                                     &prev_selection_start,
                                     &prev_selection_end);

  if (entry->mask->str[selection_start] == 't')
    {
      while (selection_start > 0 &&
             (entry->mask->str[selection_start - 1] == 't'))
        {
          selection_start--;
        }
    }

  if (selection_end > selection_start &&
      (entry->mask->str[selection_end - 1] == 't'))
    {
      while (selection_end <= entry->mask->len &&
             (entry->mask->str[selection_end] == 't'))
        {
          selection_end++;
        }
    }

  if (search_dir == TAG_SEARCH_NONE        &&
      selection_end - selection_start == 1 &&
      entry->mask->str[selection_start] == 'w')
    {
      gtk_editable_set_position (GTK_EDITABLE (entry), selection_end);
      return TRUE;
    }

  if ((selection_start != prev_selection_start ||
       selection_end != prev_selection_end)      &&
      (entry->mask->str[selection_start] == 't') &&
      selection_start < selection_end)
    {
      if (search_dir == TAG_SEARCH_LEFT)
        {
          gtk_editable_select_region (GTK_EDITABLE (entry),
                                      selection_end, selection_start);
        }
      else
        {
          gtk_editable_select_region (GTK_EDITABLE (entry),
                                      selection_start, selection_end);
        }

      return TRUE;
    }
  else
    {
      return FALSE;
    }
}

static gboolean
gimp_tag_entry_add_to_recent (GimpTagEntry *entry,
                              const gchar  *tags_string,
                              gboolean      to_front)
{
  gchar *recent_item = NULL;
  gchar *stripped_string;
  gint   stripped_length;
  GList *list;

  if (entry->mode == GIMP_TAG_ENTRY_MODE_ASSIGN)
    return FALSE;

  stripped_string = g_strdup (tags_string);
  stripped_string = g_strstrip (stripped_string);
  stripped_length = strlen (stripped_string);
  g_free (stripped_string);

  if (stripped_length <= 0)
    {
      /* there is no content in the string,
       * therefore don't add to recent list.
       */
      return FALSE;
    }

  if (g_list_length (entry->recent_list) >= GIMP_TAG_ENTRY_MAX_RECENT_ITEMS)
    {
      gchar *last_item = g_list_last (entry->recent_list)->data;
      entry->recent_list = g_list_remove (entry->recent_list, last_item);
      g_free (last_item);
    }

  for (list = entry->recent_list; list; list = g_list_next (list))
    {
      if (! strcmp (tags_string, list->data))
        {
          recent_item = list->data;
          entry->recent_list = g_list_remove (entry->recent_list, recent_item);
          break;
        }
    }

  if (! recent_item)
    {
      recent_item = g_strdup (tags_string);
    }

  if (to_front)
    {
      entry->recent_list = g_list_prepend (entry->recent_list, recent_item);
    }
  else
    {
      entry->recent_list = g_list_append (entry->recent_list, recent_item);
    }

  return TRUE;
}

/**
 * gimp_tag_entry_get_separator:
 *
 * Tag separator is a single Unicode terminal punctuation
 * character.
 *
 * Return value: returns locale dependent tag separator.
 **/
const gchar *
gimp_tag_entry_get_separator (void)
{
  /* Separator for tags
   * IMPORTANT: use only one of Unicode terminal punctuation chars.
   * http://unicode.org/review/pr-23.html
   */
  return _(",");
}

static void
gimp_tag_entry_commit_region (GString *tags,
                              GString *mask)
{
  gint      i = 0;
  gint      j;
  gint      stage = 0;
  gunichar  c;
  gchar    *cursor;
  GString  *out_tags;
  GString  *out_mask;
  GString  *tag_buffer;

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
          if (c && ! gimp_tag_is_tag_separator (c))
            {
              g_string_append_unichar (tag_buffer, c);
            }
          else
            {
              gchar *valid_tag = gimp_tag_string_make_valid (tag_buffer->str);
              gsize  tag_length;

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
          if (gimp_tag_is_tag_separator (c))
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
gimp_tag_entry_commit_tags (GimpTagEntry *entry)
{
  gboolean found_region;
  gint     cursor_position;

  cursor_position = gtk_editable_get_position (GTK_EDITABLE (entry));

  do
    {
      gint  region_start;
      gint  region_end;
      gint  position;
      glong length_before;
      gint  i;

      found_region = FALSE;

      for (i = 0; i < entry->mask->len; i++)
        {
          if (entry->mask->str[i] == 'u')
            {
              found_region = TRUE;
              region_start = i;
              region_end = i + 1;
              for (i++; i < entry->mask->len; i++)
                {
                  if (entry->mask->str[i] == 'u')
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
          gchar   *tags_string;
          GString *tags;
          GString *mask;

          tags_string = gtk_editable_get_chars (GTK_EDITABLE (entry),
                                                region_start, region_end);
          tags = g_string_new (tags_string);
          g_free (tags_string);
          length_before = region_end - region_start;

          mask = g_string_new_len (entry->mask->str + region_start, region_end - region_start);

          gimp_tag_entry_commit_region (tags, mask);

          /* prepend space before if needed */
          if (region_start > 0
              && entry->mask->str[region_start - 1] != 'w'
              && mask->len > 0
              && mask->str[0] != 'w')
            {
              g_string_prepend_c (tags, ' ');
              g_string_prepend_c (mask, 'w');
            }

          /* append space after if needed */
          if (region_end <= entry->mask->len
              && entry->mask->str[region_end] != 'w'
              && mask->len > 0
              && mask->str[mask->len - 1] != 'w')
            {
              g_string_append_c (tags, ' ');
              g_string_append_c (mask, 'w');
            }

          if (cursor_position >= region_start)
            {
              cursor_position += g_utf8_strlen (tags->str, tags->len) - length_before;
            }

          entry->internal_operation++;
          entry->suppress_mask_update++;
          entry->suppress_tag_query++;
          gtk_editable_delete_text (GTK_EDITABLE (entry),
                                    region_start, region_end);
          position = region_start;
          gtk_editable_insert_text (GTK_EDITABLE (entry),
                                    tags->str, tags->len, &position);
          entry->suppress_tag_query--;
          entry->suppress_mask_update--;
          entry->internal_operation--;

          g_string_erase (entry->mask, region_start, region_end - region_start);
          g_string_insert_len (entry->mask, region_start, mask->str, mask->len);

          g_string_free (mask, TRUE);
          g_string_free (tags, TRUE);
        }
    }
  while (found_region);

  gtk_editable_set_position (GTK_EDITABLE (entry), cursor_position);
  gimp_tag_entry_strip_extra_whitespace (entry);
}

static gboolean
gimp_tag_entry_commit_source_func (GimpTagEntry *entry)
{
  gimp_tag_entry_commit_tags (entry);

  return FALSE;
}

static void
gimp_tag_entry_next_tag (GimpTagEntry *entry,
                         gboolean      select)
{
  gint position = gtk_editable_get_position (GTK_EDITABLE (entry));

  if (entry->mask->str[position] != 'u')
    {
      while (position < entry->mask->len &&
             (entry->mask->str[position] != 'w'))
        {
          position++;
        }

      if (entry->mask->str[position] == 'w')
        {
          position++;
        }
    }
  else if (position < entry->mask->len)
    {
      position++;
    }

  if (select)
    {
      gint  current_position;
      gint  selection_start;
      gint  selection_end;

      current_position = gtk_editable_get_position (GTK_EDITABLE (entry));
      gtk_editable_get_selection_bounds (GTK_EDITABLE (entry),
                                         &selection_start, &selection_end);

      if (current_position == selection_end)
        {
          gtk_editable_select_region (GTK_EDITABLE (entry),
                                      selection_start, position);
        }
      else if (current_position == selection_start)
        {
          gtk_editable_select_region (GTK_EDITABLE (entry),
                                      selection_end, position);
        }
    }
  else
    {
      gtk_editable_set_position (GTK_EDITABLE (entry), position);
    }
}

static void
gimp_tag_entry_previous_tag (GimpTagEntry *entry,
                             gboolean      select)
{
  gint  position = gtk_editable_get_position (GTK_EDITABLE (entry));

  if (position >= 1 &&
      entry->mask->str[position - 1] == 'w')
    {
      position--;
    }
  if (position < 1)
    {
      return;
    }
  if (entry->mask->str[position - 1] != 'u')
    {
      while (position > 0 &&
             (entry->mask->str[position - 1] != 'w'))
        {
          if (entry->mask->str[position - 1] == 'u')
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

  if (select)
    {
      gint  current_position;
      gint  selection_start;
      gint  selection_end;

      current_position = gtk_editable_get_position (GTK_EDITABLE (entry));
      gtk_editable_get_selection_bounds (GTK_EDITABLE (entry),
                                         &selection_start, &selection_end);

      if (current_position == selection_start)
        {
          gtk_editable_select_region (GTK_EDITABLE (entry),
                                      selection_end, position);
        }
      else if (current_position == selection_end)
        {
          gtk_editable_select_region (GTK_EDITABLE (entry),
                                      selection_start, position);
        }
    }
  else
    {
      gtk_editable_set_position (GTK_EDITABLE (entry), position);
    }
}

static void
gimp_tag_entry_select_for_deletion (GimpTagEntry     *entry,
                                    GimpTagSearchDir  search_dir)
{
  gint start_pos;
  gint end_pos;

  /* make sure the whole tag is selected,
   * including a  separator
   */
  gtk_editable_get_selection_bounds (GTK_EDITABLE (entry),
                                     &start_pos, &end_pos);
  while (start_pos > 0 &&
         (entry->mask->str[start_pos - 1] == 't'))
    {
      start_pos--;
    }

  if (end_pos > start_pos &&
      (entry->mask->str[end_pos - 1] == 't' ||
       entry->mask->str[end_pos - 1] == 's'))
    {
      while (end_pos <= entry->mask->len &&
             (entry->mask->str[end_pos] == 's'))
        {
          end_pos++;
        }
    }

  /* ensure there is no unnecessary whitespace selected */
  while (start_pos < end_pos &&
         entry->mask->str[start_pos] == 'w')
    {
      start_pos++;
    }

  while (start_pos < end_pos &&
         entry->mask->str[end_pos - 1] == 'w')
    {
      end_pos--;
    }

  /* delete spaces in one side */
  if (search_dir == TAG_SEARCH_LEFT)
    {
      gtk_editable_select_region (GTK_EDITABLE (entry), end_pos, start_pos);
    }
  else if (end_pos > start_pos            &&
           search_dir == TAG_SEARCH_RIGHT &&
           (entry->mask->str[end_pos - 1] == 't' ||
            entry->mask->str[end_pos - 1] == 's'))
    {
      gtk_editable_select_region (GTK_EDITABLE (entry), start_pos, end_pos);
    }
}

static gboolean
gimp_tag_entry_strip_extra_whitespace (GimpTagEntry *entry)
{
  gint  i;
  gint  position;

  position = gtk_editable_get_position (GTK_EDITABLE (entry));

  entry->internal_operation++;
  entry->suppress_tag_query++;

  /* strip whitespace in front */
  while (entry->mask->len > 0 &&
         entry->mask->str[0] == 'w')
    {
      gtk_editable_delete_text (GTK_EDITABLE (entry), 0, 1);
    }

  /* strip whitespace in back */
  while (entry->mask->len > 1                          &&
         entry->mask->str[entry->mask->len - 1] == 'w' &&
         entry->mask->str[entry->mask->len - 2] == 'w')
    {
      gtk_editable_delete_text (GTK_EDITABLE (entry),
                                entry->mask->len - 1, entry->mask->len);

      if (position == entry->mask->len)
        {
          position--;
        }
    }

  /* strip extra whitespace in the middle */
  for (i = entry->mask->len - 1; i > 0; i--)
    {
      if (entry->mask->str[i] == 'w' &&
          entry->mask->str[i - 1] == 'w')
        {
          gtk_editable_delete_text (GTK_EDITABLE (entry), i, i + 1);

          if (position >= i)
            {
              position--;
            }
        }
    }

  /* special case when cursor is in the last position:
   * it must be positioned after the last whitespace.
   */
  if (position == entry->mask->len - 1 &&
      entry->mask->str[position] == 'w')
    {
      position++;
    }

  gtk_editable_set_position (GTK_EDITABLE (entry), position);

  entry->suppress_tag_query--;
  entry->internal_operation--;

  return FALSE;
}
