/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995-1997 Spencer Kimball and Peter Mattis
 *
 * gimpfilteredcontainer.c
 * Copyright (C) Aurimas Juška <aurisj@svn.gnome.org>
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

#include <stdlib.h>
#include <string.h> /* strcmp */

#include <glib-object.h>

#include "core-types.h"

#include "gimp.h"
#include "gimptagged.h"
#include "gimplist.h"
#include "gimpfilteredcontainer.h"

static void         gimp_filtered_container_dispose            (GObject               *object);

static gint64       gimp_filtered_container_get_memsize        (GimpObject            *object,
                                                                gint64                *gui_size);

static gboolean     gimp_filtered_container_object_matches     (GimpFilteredContainer *filtered_container,
                                                                GimpObject            *object);

static void         gimp_filtered_container_filter             (GimpFilteredContainer *filtered_container);

static void         gimp_filtered_container_src_add            (GimpContainer         *src_container,
                                                                GimpObject            *obj,
                                                                GimpFilteredContainer *filtered_container);
static void         gimp_filtered_container_src_remove         (GimpContainer         *src_container,
                                                                GimpObject            *obj,
                                                                GimpFilteredContainer *filtered_container);
static void         gimp_filtered_container_src_freeze         (GimpContainer         *src_container,
                                                                GimpFilteredContainer *filtered_container);
static void         gimp_filtered_container_src_thaw           (GimpContainer         *src_container,
                                                                GimpFilteredContainer *filtered_container);
static void         gimp_filtered_container_tag_added          (GimpTagged            *tagged,
                                                                GimpTag                tag,
                                                                GimpFilteredContainer  *filtered_container);
static void         gimp_filtered_container_tag_removed        (GimpTagged            *tagged,
                                                                GimpTag                tag,
                                                                GimpFilteredContainer  *filtered_container);
static void         gimp_filtered_container_tagged_item_added  (GimpTagged             *tagged,
                                                                GimpFilteredContainer  *filtered_container);
static void         gimp_filtered_container_tagged_item_removed(GimpTagged             *tagged,
                                                                GimpFilteredContainer  *filtered_container);


G_DEFINE_TYPE (GimpFilteredContainer, gimp_filtered_container, GIMP_TYPE_LIST)

#define parent_class gimp_filtered_container_parent_class


static void
gimp_filtered_container_class_init (GimpFilteredContainerClass *klass)
{
  GObjectClass       *g_object_class    = G_OBJECT_CLASS (klass);
  GimpObjectClass    *gimp_object_class = GIMP_OBJECT_CLASS (klass);

  g_object_class->dispose             = gimp_filtered_container_dispose;

  gimp_object_class->get_memsize      = gimp_filtered_container_get_memsize;
}

static void
gimp_filtered_container_init (GimpFilteredContainer *filtered_container)
{
  filtered_container->src_container             = NULL;
  filtered_container->filter                    = NULL;
  filtered_container->tag_ref_counts            = NULL;
}

static void
gimp_filtered_container_dispose (GObject     *object)
{
  GimpFilteredContainer        *filtered_container = GIMP_FILTERED_CONTAINER (object);

  g_signal_handlers_disconnect_by_func (filtered_container->src_container,
                                        gimp_filtered_container_src_add, filtered_container);
  g_signal_handlers_disconnect_by_func (filtered_container->src_container,
                                        gimp_filtered_container_src_remove, filtered_container);
  g_signal_handlers_disconnect_by_func (filtered_container->src_container,
                                        gimp_filtered_container_src_freeze, filtered_container);
  g_signal_handlers_disconnect_by_func (filtered_container->src_container,
                                        gimp_filtered_container_src_thaw, filtered_container);

  G_OBJECT_CLASS (parent_class)->dispose (object);
}

static gint64
gimp_filtered_container_get_memsize (GimpObject *object,
                                     gint64     *gui_size)
{
  GimpFilteredContainer *filtered_container    = GIMP_FILTERED_CONTAINER (object);
  gint64    memsize = 0;

  memsize += (gimp_container_num_children (GIMP_CONTAINER (filtered_container)) *
              sizeof (GList));

  return memsize + GIMP_OBJECT_CLASS (parent_class)->get_memsize (object,
                                                                  gui_size);
}

/**
 * gimp_filtered_container_new:
 * @src_container: container to be filtered.
 *
 * Creates a new #GimpFilteredContainer object. Since #GimpFilteredContainer is a #GimpContainer
 * implementation, it holds GimpObjects. It filters @src_container for objects containing
 * all of the filtering tags. Syncronization with @src_container data is performed
 * automatically.
 *
 * Return value: a new #GimpFilteredContainer object
 **/
GimpContainer *
gimp_filtered_container_new (GimpContainer     *src_container)
{
  GimpFilteredContainer        *filtered_container;
  GType                         children_type;

  children_type = gimp_container_children_type (src_container);

  filtered_container = g_object_new (GIMP_TYPE_FILTERED_CONTAINER,
                                     "children-type", children_type,
                                     "policy",        GIMP_CONTAINER_POLICY_WEAK,
                                     "unique-names",  FALSE,
                                     NULL);

  filtered_container->src_container = src_container;
  GIMP_CONTAINER (filtered_container)->children_type = children_type;
  filtered_container->tag_ref_counts = g_hash_table_new (g_direct_hash, g_direct_equal);

  gimp_filtered_container_filter (filtered_container);

  g_signal_connect (src_container, "add",
                    G_CALLBACK (gimp_filtered_container_src_add), filtered_container);
  g_signal_connect (src_container, "remove",
                    G_CALLBACK (gimp_filtered_container_src_remove), filtered_container);
  g_signal_connect (src_container, "freeze",
                    G_CALLBACK (gimp_filtered_container_src_freeze), filtered_container);
  g_signal_connect (src_container, "thaw",
                    G_CALLBACK (gimp_filtered_container_src_thaw), filtered_container);

  return GIMP_CONTAINER (filtered_container);
}

void
gimp_filtered_container_set_filter (GimpFilteredContainer              *filtered_container,
                                    GList                              *tags)
{
  filtered_container->filter = tags;

  gimp_container_freeze (GIMP_CONTAINER (filtered_container));
  gimp_container_clear (GIMP_CONTAINER (filtered_container));
  gimp_filtered_container_filter (filtered_container);
  gimp_container_thaw (GIMP_CONTAINER (filtered_container));
}

GList *
gimp_filtered_container_get_filter (GimpFilteredContainer              *filtered_container)
{
  return filtered_container->filter;
}

static gboolean
gimp_filtered_container_object_matches (GimpFilteredContainer          *filtered_container,
                                        GimpObject                     *object)
{
  GList        *filter_tag;
  GList        *object_tag;

  filter_tag = filtered_container->filter;
  while (filter_tag)
    {
      if (! filter_tag->data)
        {
          return FALSE;
        }

      object_tag = gimp_tagged_get_tags (GIMP_TAGGED (object));
      while (object_tag)
        {
          if (object_tag->data == filter_tag->data)
            {
              break;
            }
          object_tag = g_list_next (object_tag);
        }

      if (! object_tag)
        {
          return FALSE;
        }

      filter_tag = g_list_next (filter_tag);
    }

  return TRUE;
}

static void
add_filtered_item (GimpObject                  *object,
                   GimpFilteredContainer       *filtered_container)
{
  if (gimp_filtered_container_object_matches (filtered_container,
                                              object))
    {
      gimp_container_add (GIMP_CONTAINER (filtered_container), object);
    }
}

static void
gimp_filtered_container_filter (GimpFilteredContainer          *filtered_container)
{
  gimp_container_foreach (filtered_container->src_container,
                          (GFunc) gimp_filtered_container_tagged_item_added,
                          filtered_container);
  gimp_container_foreach (filtered_container->src_container,
                          (GFunc) add_filtered_item, filtered_container);
}

static void
gimp_filtered_container_src_add (GimpContainer         *src_container,
                                 GimpObject            *obj,
                                 GimpFilteredContainer *filtered_container)
{
  gimp_filtered_container_tagged_item_added (GIMP_TAGGED (obj),
                                             filtered_container);

  if (! gimp_container_frozen (src_container))
    {
      gimp_container_add (GIMP_CONTAINER (filtered_container), obj);
    }
}

static void
gimp_filtered_container_src_remove (GimpContainer         *src_container,
                                    GimpObject            *obj,
                                    GimpFilteredContainer *filtered_container)
{
  gimp_filtered_container_tagged_item_removed (GIMP_TAGGED (obj),
                                               filtered_container);

  if (! gimp_container_frozen (src_container)
      && gimp_container_have (GIMP_CONTAINER (filtered_container), obj))
    {
      gimp_container_remove (GIMP_CONTAINER (filtered_container), obj);
    }
}

static void
gimp_filtered_container_src_freeze (GimpContainer              *src_container,
                                    GimpFilteredContainer      *filtered_container)
{
  gimp_container_freeze (GIMP_CONTAINER (filtered_container));
  gimp_container_clear (GIMP_CONTAINER (filtered_container));
  g_hash_table_remove_all (filtered_container->tag_ref_counts);
}

static void
gimp_filtered_container_src_thaw (GimpContainer                *src_container,
                                  GimpFilteredContainer        *filtered_container)
{
  gimp_filtered_container_filter (filtered_container);
  gimp_container_thaw (GIMP_CONTAINER (filtered_container));
}

static void
gimp_filtered_container_tagged_item_added (GimpTagged                  *tagged,
                                           GimpFilteredContainer       *filtered_container)
{
  GList        *tag_iterator;

  tag_iterator = gimp_tagged_get_tags (tagged);
  while (tag_iterator)
    {
      gimp_filtered_container_tag_added (tagged,
                                         GPOINTER_TO_UINT (tag_iterator->data),
                                         filtered_container);
      tag_iterator = g_list_next (tag_iterator);
    }

  g_signal_connect (tagged, "tag-added",
                    G_CALLBACK (gimp_filtered_container_tag_added),
                    filtered_container);
  g_signal_connect (tagged, "tag-removed",
                    G_CALLBACK (gimp_filtered_container_tag_removed),
                    filtered_container);
}

static void
gimp_filtered_container_tagged_item_removed (GimpTagged                *tagged,
                                             GimpFilteredContainer     *filtered_container)
{
  GList        *tag_iterator;

  g_signal_handlers_disconnect_by_func (tagged,
                                        G_CALLBACK (gimp_filtered_container_tag_added),
                                        filtered_container);
  g_signal_handlers_disconnect_by_func (tagged,
                                        G_CALLBACK (gimp_filtered_container_tag_removed),
                                        filtered_container);

  tag_iterator = gimp_tagged_get_tags (tagged);
  while (tag_iterator)
    {
      gimp_filtered_container_tag_removed (tagged,
                                           GPOINTER_TO_UINT (tag_iterator->data),
                                           filtered_container);
      tag_iterator = g_list_next (tag_iterator);
    }

}

static void
gimp_filtered_container_tag_added (GimpTagged            *tagged,
                                   GimpTag                tag,
                                   GimpFilteredContainer  *filtered_container)
{
  gint                  ref_count;

  ref_count = GPOINTER_TO_INT (g_hash_table_lookup (filtered_container->tag_ref_counts,
                                                    GUINT_TO_POINTER (tag)));
  ref_count++;
  g_hash_table_insert (filtered_container->tag_ref_counts,
                       GUINT_TO_POINTER (tag),
                       GINT_TO_POINTER (ref_count));
}

static void
gimp_filtered_container_tag_removed (GimpTagged                  *tagged,
                                     GimpTag                      tag,
                                     GimpFilteredContainer       *filtered_container)
{
  gint                  ref_count;

  ref_count = GPOINTER_TO_INT (g_hash_table_lookup (filtered_container->tag_ref_counts,
                                                    GUINT_TO_POINTER (tag)));
  ref_count--;
  if (ref_count > 0)
    {
      g_hash_table_insert (filtered_container->tag_ref_counts,
                           GUINT_TO_POINTER (tag),
                           GINT_TO_POINTER (ref_count));
    }
  else
    {
      g_hash_table_remove (filtered_container->tag_ref_counts,
                           GUINT_TO_POINTER (tag));
    }
}
