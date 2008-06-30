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

static void     gimp_tag_entry_activate        (GtkEntry          *entry,
                                                gpointer           unused);
static void     gimp_tag_entry_changed         (GtkEntry          *entry,
                                                gpointer           unused);

static void     gimp_tag_entry_query_tag       (GimpTagEntry      *entry);

static void     gimp_tag_entry_assign_tags     (GimpTagEntry      *tag_entry);
static void     gimp_tag_entry_item_set_tags   (GimpTagged        *entry,
                                                GList             *tags);

static gchar ** gimp_tag_entry_parse_tags      (GimpTagEntry      *entry);

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
  entry->selected_items          = NULL;
  entry->mode                  = GIMP_TAG_ENTRY_MODE_QUERY;

  g_signal_connect (entry, "activate",
                    G_CALLBACK (gimp_tag_entry_activate),
                    NULL);
  g_signal_connect (entry, "changed",
                    G_CALLBACK (gimp_tag_entry_changed),
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

  tag_entry = GIMP_TAG_ENTRY (entry);

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
}

