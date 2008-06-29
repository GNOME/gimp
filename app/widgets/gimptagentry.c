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

#include "gimptagentry.h"


static void     gimp_tag_entry_changed         (GtkEntry          *entry,
                                                GimpTagEntry      *view);

static void     gimp_tag_entry_query_tag       (GimpTagEntry      *entry);

static gchar ** gimp_tag_entry_parse_tags      (GimpTagEntry      *entry);

G_DEFINE_TYPE (GimpTagEntry, gimp_tag_entry, GTK_TYPE_ENTRY);

#define parent_class gimp_tag_entry_parent_class


static void
gimp_tag_entry_class_init (GimpTagEntryClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
}

static void
gimp_tag_entry_init (GimpTagEntry *entry)
{
  g_signal_connect (entry, "changed",
                    G_CALLBACK (gimp_tag_entry_changed),
                    entry);
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
gimp_tag_entry_changed (GtkEntry          *entry,
                        GimpTagEntry      *view)
{
  GimpTagEntry         *tag_entry;

  tag_entry = GIMP_TAG_ENTRY (entry);

  switch (tag_entry->mode)
    {
      case GIMP_TAG_ENTRY_MODE_QUERY:
          gimp_tag_entry_query_tag (entry);
          break;
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

static gchar **
gimp_tag_entry_parse_tags (GimpTagEntry        *entry)
{
  const gchar          *tag_str;
  gchar               **split_tags;
  gchar               **parsed_tags;
  gint                  length;
  gint                  i;

  tag_str = gtk_entry_get_text (GIMP_TAG_ENTRY (entry));
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

