/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995-1997 Spencer Kimball and Peter Mattis
 *
 * gimptaggedcontainer.c
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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <gio/gio.h>

#include "core-types.h"

#include "gimp.h"
#include "gimptag.h"
#include "gimptagged.h"
#include "gimptaggedcontainer.h"


enum
{
  TAG_COUNT_CHANGED,
  LAST_SIGNAL
};


static void      gimp_tagged_container_dispose            (GObject               *object);
static gint64    gimp_tagged_container_get_memsize        (GimpObject            *object,
                                                           gint64                *gui_size);

static void      gimp_tagged_container_clear              (GimpContainer         *container);

static void      gimp_tagged_container_src_add            (GimpFilteredContainer *filtered_container,
                                                           GimpObject            *object);
static void      gimp_tagged_container_src_remove         (GimpFilteredContainer *filtered_container,
                                                           GimpObject            *object);
static void      gimp_tagged_container_src_freeze         (GimpFilteredContainer *filtered_container);
static void      gimp_tagged_container_src_thaw           (GimpFilteredContainer *filtered_container);

static gboolean  gimp_tagged_container_object_matches     (GimpTaggedContainer   *tagged_container,
                                                           GimpObject            *object);

static void      gimp_tagged_container_tag_added          (GimpTagged            *tagged,
                                                           GimpTag               *tag,
                                                           GimpTaggedContainer   *tagged_container);
static void      gimp_tagged_container_tag_removed        (GimpTagged            *tagged,
                                                           GimpTag               *tag,
                                                           GimpTaggedContainer   *tagged_container);
static void      gimp_tagged_container_ref_tag            (GimpTaggedContainer   *tagged_container,
                                                           GimpTag               *tag);
static void      gimp_tagged_container_unref_tag          (GimpTaggedContainer   *tagged_container,
                                                           GimpTag               *tag);
static void      gimp_tagged_container_tag_count_changed  (GimpTaggedContainer   *tagged_container,
                                                           gint                   tag_count);


G_DEFINE_TYPE (GimpTaggedContainer, gimp_tagged_container,
               GIMP_TYPE_FILTERED_CONTAINER)

#define parent_class gimp_tagged_container_parent_class

static guint gimp_tagged_container_signals[LAST_SIGNAL] = { 0, };


static void
gimp_tagged_container_class_init (GimpTaggedContainerClass *klass)
{
  GObjectClass               *g_object_class    = G_OBJECT_CLASS (klass);
  GimpObjectClass            *gimp_object_class = GIMP_OBJECT_CLASS (klass);
  GimpContainerClass         *container_class   = GIMP_CONTAINER_CLASS (klass);
  GimpFilteredContainerClass *filtered_class    = GIMP_FILTERED_CONTAINER_CLASS (klass);

  g_object_class->dispose        = gimp_tagged_container_dispose;

  gimp_object_class->get_memsize = gimp_tagged_container_get_memsize;

  container_class->clear         = gimp_tagged_container_clear;

  filtered_class->src_add        = gimp_tagged_container_src_add;
  filtered_class->src_remove     = gimp_tagged_container_src_remove;
  filtered_class->src_freeze     = gimp_tagged_container_src_freeze;
  filtered_class->src_thaw       = gimp_tagged_container_src_thaw;

  klass->tag_count_changed       = gimp_tagged_container_tag_count_changed;

  gimp_tagged_container_signals[TAG_COUNT_CHANGED] =
    g_signal_new ("tag-count-changed",
                  GIMP_TYPE_TAGGED_CONTAINER,
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (GimpTaggedContainerClass, tag_count_changed),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 1,
                  G_TYPE_INT);
}

static void
gimp_tagged_container_init (GimpTaggedContainer *tagged_container)
{
  tagged_container->tag_ref_counts =
    g_hash_table_new_full ((GHashFunc) gimp_tag_get_hash,
                           (GEqualFunc) gimp_tag_equals,
                           (GDestroyNotify) g_object_unref,
                           (GDestroyNotify) NULL);
}

static void
gimp_tagged_container_dispose (GObject *object)
{
  GimpTaggedContainer *tagged_container = GIMP_TAGGED_CONTAINER (object);

  if (tagged_container->filter)
    {
      g_list_free_full (tagged_container->filter,
                        (GDestroyNotify) gimp_tag_or_null_unref);
      tagged_container->filter = NULL;
    }

  g_clear_pointer (&tagged_container->tag_ref_counts, g_hash_table_unref);

  G_OBJECT_CLASS (parent_class)->dispose (object);
}

static gint64
gimp_tagged_container_get_memsize (GimpObject *object,
                                   gint64     *gui_size)
{
  gint64 memsize = 0;

  /* FIXME take members into account */

  return memsize + GIMP_OBJECT_CLASS (parent_class)->get_memsize (object,
                                                                  gui_size);
}

static void
gimp_tagged_container_clear (GimpContainer *container)
{
  GimpFilteredContainer *filtered_container = GIMP_FILTERED_CONTAINER (container);
  GimpTaggedContainer   *tagged_container   = GIMP_TAGGED_CONTAINER (container);
  GList                 *list;

  for (list = GIMP_LIST (filtered_container->src_container)->queue->head;
       list;
       list = g_list_next (list))
    {
      g_signal_handlers_disconnect_by_func (list->data,
                                            gimp_tagged_container_tag_added,
                                            tagged_container);
      g_signal_handlers_disconnect_by_func (list->data,
                                            gimp_tagged_container_tag_removed,
                                            tagged_container);
    }

  if (tagged_container->tag_ref_counts)
    {
      g_hash_table_remove_all (tagged_container->tag_ref_counts);
      tagged_container->tag_count = 0;
    }

  GIMP_CONTAINER_CLASS (parent_class)->clear (container);
}

static void
gimp_tagged_container_src_add (GimpFilteredContainer *filtered_container,
                               GimpObject            *object)
{
  GimpTaggedContainer *tagged_container = GIMP_TAGGED_CONTAINER (filtered_container);
  GList               *list;

  for (list = gimp_tagged_get_tags (GIMP_TAGGED (object));
       list;
       list = g_list_next (list))
    {
      gimp_tagged_container_ref_tag (tagged_container, list->data);
    }

  g_signal_connect (object, "tag-added",
                    G_CALLBACK (gimp_tagged_container_tag_added),
                    tagged_container);
  g_signal_connect (object, "tag-removed",
                    G_CALLBACK (gimp_tagged_container_tag_removed),
                    tagged_container);

  if (gimp_tagged_container_object_matches (tagged_container, object))
    {
      gimp_container_add (GIMP_CONTAINER (tagged_container), object);
    }
}

static void
gimp_tagged_container_src_remove (GimpFilteredContainer *filtered_container,
                                  GimpObject            *object)
{
  GimpTaggedContainer *tagged_container = GIMP_TAGGED_CONTAINER (filtered_container);
  GList               *list;

  g_signal_handlers_disconnect_by_func (object,
                                        gimp_tagged_container_tag_added,
                                        tagged_container);
  g_signal_handlers_disconnect_by_func (object,
                                        gimp_tagged_container_tag_removed,
                                        tagged_container);

  for (list = gimp_tagged_get_tags (GIMP_TAGGED (object));
       list;
       list = g_list_next (list))
    {
      gimp_tagged_container_unref_tag (tagged_container, list->data);
    }

  if (gimp_tagged_container_object_matches (tagged_container, object))
    {
      gimp_container_remove (GIMP_CONTAINER (tagged_container), object);
    }
}

static void
gimp_tagged_container_src_freeze (GimpFilteredContainer *filtered_container)
{
  gimp_container_clear (GIMP_CONTAINER (filtered_container));
}

static void
gimp_tagged_container_src_thaw (GimpFilteredContainer *filtered_container)
{
  GList *list;

  for (list = GIMP_LIST (filtered_container->src_container)->queue->head;
       list;
       list = g_list_next (list))
    {
      gimp_tagged_container_src_add (filtered_container, list->data);
    }
}

/**
 * gimp_tagged_container_new:
 * @src_container: container to be filtered.
 *
 * Creates a new #GimpTaggedContainer object which creates filtered
 * data view of #GimpTagged objects. It filters @src_container for
 * objects containing all of the filtering tags. Synchronization with
 * @src_container data is performed automatically.
 *
 * Returns: a new #GimpTaggedContainer object.
 **/
GimpContainer *
gimp_tagged_container_new (GimpContainer *src_container)
{
  GimpTaggedContainer *tagged_container;
  GType                child_type;
  GCompareFunc         sort_func;

  g_return_val_if_fail (GIMP_IS_LIST (src_container), NULL);

  child_type = gimp_container_get_child_type (src_container);
  sort_func  = gimp_list_get_sort_func (GIMP_LIST (src_container));

  tagged_container = g_object_new (GIMP_TYPE_TAGGED_CONTAINER,
                                   "sort-func",     sort_func,
                                   "child-type",    child_type,
                                   "policy",        GIMP_CONTAINER_POLICY_WEAK,
                                   "unique-names",  FALSE,
                                   "src-container", src_container,
                                   NULL);

  return GIMP_CONTAINER (tagged_container);
}

/**
 * gimp_tagged_container_set_filter:
 * @tagged_container: a #GimpTaggedContainer object.
 * @tags:             list of #GimpTag objects.
 *
 * Sets list of tags to be used for filtering. Only objects which have
 * all of the tags assigned match filtering criteria.
 **/
void
gimp_tagged_container_set_filter (GimpTaggedContainer *tagged_container,
                                  GList               *tags)
{
  GList *new_filter;

  g_return_if_fail (GIMP_IS_TAGGED_CONTAINER (tagged_container));

  if (tags)
    {
      GList *list;

      for (list = tags; list; list = g_list_next (list))
        g_return_if_fail (list->data == NULL || GIMP_IS_TAG (list->data));
    }

  if (! gimp_container_frozen (GIMP_FILTERED_CONTAINER (tagged_container)->src_container))
    {
      gimp_tagged_container_src_freeze (GIMP_FILTERED_CONTAINER (tagged_container));
    }

  /*  ref new tags first, they could be the same as the old ones  */
  new_filter = g_list_copy (tags);
  g_list_foreach (new_filter, (GFunc) gimp_tag_or_null_ref, NULL);

  g_list_free_full (tagged_container->filter,
                    (GDestroyNotify) gimp_tag_or_null_unref);
  tagged_container->filter = new_filter;

  if (! gimp_container_frozen (GIMP_FILTERED_CONTAINER (tagged_container)->src_container))
    {
      gimp_tagged_container_src_thaw (GIMP_FILTERED_CONTAINER (tagged_container));
    }
}

/**
 * gimp_tagged_container_get_filter:
 * @tagged_container: a #GimpTaggedContainer object.
 *
 * Returns current tag filter. Tag filter is a list of GimpTag objects, which
 * must be contained by each object matching filter criteria.
 *
 * Returns: a list of GimpTag objects used as filter. This value should
 *               not be modified or freed.
 **/
const GList *
gimp_tagged_container_get_filter (GimpTaggedContainer *tagged_container)
{
  g_return_val_if_fail (GIMP_IS_TAGGED_CONTAINER (tagged_container), NULL);

  return tagged_container->filter;
}

static gboolean
gimp_tagged_container_object_matches (GimpTaggedContainer *tagged_container,
                                      GimpObject          *object)
{
  GList *filter_tags;

  for (filter_tags = tagged_container->filter;
       filter_tags;
       filter_tags = g_list_next (filter_tags))
    {
      if (! filter_tags->data)
        {
          /* invalid tag - does not match */
          return FALSE;
        }

      if (! gimp_tagged_has_tag (GIMP_TAGGED (object),
                                 filter_tags->data))
        {
          /* match for the tag was not found.
           * since query is of type AND, it whole fails.
           */
          return FALSE;
        }
    }

  return TRUE;
}

static void
gimp_tagged_container_tag_added (GimpTagged          *tagged,
                                 GimpTag             *tag,
                                 GimpTaggedContainer *tagged_container)
{
  gimp_tagged_container_ref_tag (tagged_container, tag);

  if (gimp_tagged_container_object_matches (tagged_container,
                                            GIMP_OBJECT (tagged)) &&
      ! gimp_container_have (GIMP_CONTAINER (tagged_container),
                             GIMP_OBJECT (tagged)))
    {
      gimp_container_add (GIMP_CONTAINER (tagged_container),
                          GIMP_OBJECT (tagged));
    }
}

static void
gimp_tagged_container_tag_removed (GimpTagged          *tagged,
                                   GimpTag             *tag,
                                   GimpTaggedContainer *tagged_container)
{
  gimp_tagged_container_unref_tag (tagged_container, tag);

  if (! gimp_tagged_container_object_matches (tagged_container,
                                              GIMP_OBJECT (tagged)) &&
      gimp_container_have (GIMP_CONTAINER (tagged_container),
                           GIMP_OBJECT (tagged)))
    {
      gimp_container_remove (GIMP_CONTAINER (tagged_container),
                             GIMP_OBJECT (tagged));
    }
}

static void
gimp_tagged_container_ref_tag (GimpTaggedContainer *tagged_container,
                               GimpTag             *tag)
{
  gint ref_count;

  ref_count = GPOINTER_TO_INT (g_hash_table_lookup (tagged_container->tag_ref_counts,
                                                    tag));
  ref_count++;
  g_hash_table_insert (tagged_container->tag_ref_counts,
                       g_object_ref (tag),
                       GINT_TO_POINTER (ref_count));

  if (ref_count == 1)
    {
      tagged_container->tag_count++;
      g_signal_emit (tagged_container,
                     gimp_tagged_container_signals[TAG_COUNT_CHANGED], 0,
                     tagged_container->tag_count);
    }
}

static void
gimp_tagged_container_unref_tag (GimpTaggedContainer *tagged_container,
                                 GimpTag             *tag)
{
  gint ref_count;

  ref_count = GPOINTER_TO_INT (g_hash_table_lookup (tagged_container->tag_ref_counts,
                                                    tag));
  ref_count--;

  if (ref_count > 0)
    {
      g_hash_table_insert (tagged_container->tag_ref_counts,
                           g_object_ref (tag),
                           GINT_TO_POINTER (ref_count));
    }
  else
    {
      if (g_hash_table_remove (tagged_container->tag_ref_counts, tag))
        {
          tagged_container->tag_count--;
          g_signal_emit (tagged_container,
                         gimp_tagged_container_signals[TAG_COUNT_CHANGED], 0,
                         tagged_container->tag_count);
        }
    }
}

static void
gimp_tagged_container_tag_count_changed (GimpTaggedContainer *container,
                                         gint                 tag_count)
{
}

/**
 * gimp_tagged_container_get_tag_count:
 * @container:  a #GimpTaggedContainer object.
 *
 * Get number of distinct tags that are currently assigned to all
 * objects in the container. The count is independent of currently
 * used filter, it is provided for all available objects (ie. empty
 * filter).
 *
 * Returns: number of distinct tags assigned to all objects in the
 *               container.
 **/
gint
gimp_tagged_container_get_tag_count (GimpTaggedContainer *container)
{
  g_return_val_if_fail (GIMP_IS_TAGGED_CONTAINER (container), 0);

  return container->tag_count;
}
