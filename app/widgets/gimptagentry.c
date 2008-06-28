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

static void     gimp_tag_entry_query_tag       (GtkEntry             *entry);

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
/*  completion = g_object_new (GTK_TYPE_ENTRY_COMPLETION,
                             "inline-completion",  TRUE,
                             "popup-single-match", FALSE,
                             "popup-set-width",    FALSE,
                             NULL);

  store = gtk_list_store_new (GIMP_TAG_ENTRY_NUM_COLUMNS,
                              GIMP_TYPE_VIEW_RENDERER,
                              G_TYPE_STRING);

  gtk_entry_completion_set_model (completion, GTK_TREE_MODEL (store));
  g_object_unref (store);

  gtk_entry_set_completion (GTK_ENTRY (entry), completion);

  g_signal_connect (completion, "match-selected",
                    G_CALLBACK (gimp_tag_entry_match_selected),
                    entry);

  g_object_unref (completion); */

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
gimp_tag_entry_match_selected (GtkEntryCompletion *widget,
                               GtkTreeModel       *model,
                               GtkTreeIter        *iter,
                               GimpTagEntry       *view)
{
}

static void
gimp_tag_entry_query_tag (GtkEntry             *entry)
{
  GimpTagEntry                 *tag_entry;
  GQuark                        tag;
  GList                        *tag_list = NULL;

  tag_entry = GIMP_TAG_ENTRY (entry);

  if (strlen (gtk_entry_get_text (entry)) > 0)
    {
      tag = g_quark_from_string (gtk_entry_get_text (entry));
      tag_list = g_list_append (tag_list, GUINT_TO_POINTER (tag));
    }

  gimp_filtered_container_set_filter (GIMP_FILTERED_CONTAINER (tag_entry->tagged_container),
                                      tag_list);
}

