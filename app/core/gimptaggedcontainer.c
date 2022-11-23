/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995-1997 Spencer Kimball and Peter Mattis
 *
 * ligmataggedcontainer.c
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

#include "ligma.h"
#include "ligmatag.h"
#include "ligmatagged.h"
#include "ligmataggedcontainer.h"


enum
{
  TAG_COUNT_CHANGED,
  LAST_SIGNAL
};


static void      ligma_tagged_container_dispose            (GObject               *object);
static gint64    ligma_tagged_container_get_memsize        (LigmaObject            *object,
                                                           gint64                *gui_size);

static void      ligma_tagged_container_clear              (LigmaContainer         *container);

static void      ligma_tagged_container_src_add            (LigmaFilteredContainer *filtered_container,
                                                           LigmaObject            *object);
static void      ligma_tagged_container_src_remove         (LigmaFilteredContainer *filtered_container,
                                                           LigmaObject            *object);
static void      ligma_tagged_container_src_freeze         (LigmaFilteredContainer *filtered_container);
static void      ligma_tagged_container_src_thaw           (LigmaFilteredContainer *filtered_container);

static gboolean  ligma_tagged_container_object_matches     (LigmaTaggedContainer   *tagged_container,
                                                           LigmaObject            *object);

static void      ligma_tagged_container_tag_added          (LigmaTagged            *tagged,
                                                           LigmaTag               *tag,
                                                           LigmaTaggedContainer   *tagged_container);
static void      ligma_tagged_container_tag_removed        (LigmaTagged            *tagged,
                                                           LigmaTag               *tag,
                                                           LigmaTaggedContainer   *tagged_container);
static void      ligma_tagged_container_ref_tag            (LigmaTaggedContainer   *tagged_container,
                                                           LigmaTag               *tag);
static void      ligma_tagged_container_unref_tag          (LigmaTaggedContainer   *tagged_container,
                                                           LigmaTag               *tag);
static void      ligma_tagged_container_tag_count_changed  (LigmaTaggedContainer   *tagged_container,
                                                           gint                   tag_count);


G_DEFINE_TYPE (LigmaTaggedContainer, ligma_tagged_container,
               LIGMA_TYPE_FILTERED_CONTAINER)

#define parent_class ligma_tagged_container_parent_class

static guint ligma_tagged_container_signals[LAST_SIGNAL] = { 0, };


static void
ligma_tagged_container_class_init (LigmaTaggedContainerClass *klass)
{
  GObjectClass               *g_object_class    = G_OBJECT_CLASS (klass);
  LigmaObjectClass            *ligma_object_class = LIGMA_OBJECT_CLASS (klass);
  LigmaContainerClass         *container_class   = LIGMA_CONTAINER_CLASS (klass);
  LigmaFilteredContainerClass *filtered_class    = LIGMA_FILTERED_CONTAINER_CLASS (klass);

  g_object_class->dispose        = ligma_tagged_container_dispose;

  ligma_object_class->get_memsize = ligma_tagged_container_get_memsize;

  container_class->clear         = ligma_tagged_container_clear;

  filtered_class->src_add        = ligma_tagged_container_src_add;
  filtered_class->src_remove     = ligma_tagged_container_src_remove;
  filtered_class->src_freeze     = ligma_tagged_container_src_freeze;
  filtered_class->src_thaw       = ligma_tagged_container_src_thaw;

  klass->tag_count_changed       = ligma_tagged_container_tag_count_changed;

  ligma_tagged_container_signals[TAG_COUNT_CHANGED] =
    g_signal_new ("tag-count-changed",
                  LIGMA_TYPE_TAGGED_CONTAINER,
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (LigmaTaggedContainerClass, tag_count_changed),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 1,
                  G_TYPE_INT);
}

static void
ligma_tagged_container_init (LigmaTaggedContainer *tagged_container)
{
  tagged_container->tag_ref_counts =
    g_hash_table_new_full ((GHashFunc) ligma_tag_get_hash,
                           (GEqualFunc) ligma_tag_equals,
                           (GDestroyNotify) g_object_unref,
                           (GDestroyNotify) NULL);
}

static void
ligma_tagged_container_dispose (GObject *object)
{
  LigmaTaggedContainer *tagged_container = LIGMA_TAGGED_CONTAINER (object);

  if (tagged_container->filter)
    {
      g_list_free_full (tagged_container->filter,
                        (GDestroyNotify) ligma_tag_or_null_unref);
      tagged_container->filter = NULL;
    }

  g_clear_pointer (&tagged_container->tag_ref_counts, g_hash_table_unref);

  G_OBJECT_CLASS (parent_class)->dispose (object);
}

static gint64
ligma_tagged_container_get_memsize (LigmaObject *object,
                                   gint64     *gui_size)
{
  gint64 memsize = 0;

  /* FIXME take members into account */

  return memsize + LIGMA_OBJECT_CLASS (parent_class)->get_memsize (object,
                                                                  gui_size);
}

static void
ligma_tagged_container_clear (LigmaContainer *container)
{
  LigmaFilteredContainer *filtered_container = LIGMA_FILTERED_CONTAINER (container);
  LigmaTaggedContainer   *tagged_container   = LIGMA_TAGGED_CONTAINER (container);
  GList                 *list;

  for (list = LIGMA_LIST (filtered_container->src_container)->queue->head;
       list;
       list = g_list_next (list))
    {
      g_signal_handlers_disconnect_by_func (list->data,
                                            ligma_tagged_container_tag_added,
                                            tagged_container);
      g_signal_handlers_disconnect_by_func (list->data,
                                            ligma_tagged_container_tag_removed,
                                            tagged_container);
    }

  if (tagged_container->tag_ref_counts)
    {
      g_hash_table_remove_all (tagged_container->tag_ref_counts);
      tagged_container->tag_count = 0;
    }

  LIGMA_CONTAINER_CLASS (parent_class)->clear (container);
}

static void
ligma_tagged_container_src_add (LigmaFilteredContainer *filtered_container,
                               LigmaObject            *object)
{
  LigmaTaggedContainer *tagged_container = LIGMA_TAGGED_CONTAINER (filtered_container);
  GList               *list;

  for (list = ligma_tagged_get_tags (LIGMA_TAGGED (object));
       list;
       list = g_list_next (list))
    {
      ligma_tagged_container_ref_tag (tagged_container, list->data);
    }

  g_signal_connect (object, "tag-added",
                    G_CALLBACK (ligma_tagged_container_tag_added),
                    tagged_container);
  g_signal_connect (object, "tag-removed",
                    G_CALLBACK (ligma_tagged_container_tag_removed),
                    tagged_container);

  if (ligma_tagged_container_object_matches (tagged_container, object))
    {
      ligma_container_add (LIGMA_CONTAINER (tagged_container), object);
    }
}

static void
ligma_tagged_container_src_remove (LigmaFilteredContainer *filtered_container,
                                  LigmaObject            *object)
{
  LigmaTaggedContainer *tagged_container = LIGMA_TAGGED_CONTAINER (filtered_container);
  GList               *list;

  g_signal_handlers_disconnect_by_func (object,
                                        ligma_tagged_container_tag_added,
                                        tagged_container);
  g_signal_handlers_disconnect_by_func (object,
                                        ligma_tagged_container_tag_removed,
                                        tagged_container);

  for (list = ligma_tagged_get_tags (LIGMA_TAGGED (object));
       list;
       list = g_list_next (list))
    {
      ligma_tagged_container_unref_tag (tagged_container, list->data);
    }

  if (ligma_tagged_container_object_matches (tagged_container, object))
    {
      ligma_container_remove (LIGMA_CONTAINER (tagged_container), object);
    }
}

static void
ligma_tagged_container_src_freeze (LigmaFilteredContainer *filtered_container)
{
  ligma_container_clear (LIGMA_CONTAINER (filtered_container));
}

static void
ligma_tagged_container_src_thaw (LigmaFilteredContainer *filtered_container)
{
  GList *list;

  for (list = LIGMA_LIST (filtered_container->src_container)->queue->head;
       list;
       list = g_list_next (list))
    {
      ligma_tagged_container_src_add (filtered_container, list->data);
    }
}

/**
 * ligma_tagged_container_new:
 * @src_container: container to be filtered.
 *
 * Creates a new #LigmaTaggedContainer object which creates filtered
 * data view of #LigmaTagged objects. It filters @src_container for
 * objects containing all of the filtering tags. Synchronization with
 * @src_container data is performed automatically.
 *
 * Returns: a new #LigmaTaggedContainer object.
 **/
LigmaContainer *
ligma_tagged_container_new (LigmaContainer *src_container)
{
  LigmaTaggedContainer *tagged_container;
  GType                children_type;
  GCompareFunc         sort_func;

  g_return_val_if_fail (LIGMA_IS_LIST (src_container), NULL);

  children_type = ligma_container_get_children_type (src_container);
  sort_func     = ligma_list_get_sort_func (LIGMA_LIST (src_container));

  tagged_container = g_object_new (LIGMA_TYPE_TAGGED_CONTAINER,
                                   "sort-func",     sort_func,
                                   "children-type", children_type,
                                   "policy",        LIGMA_CONTAINER_POLICY_WEAK,
                                   "unique-names",  FALSE,
                                   "src-container", src_container,
                                   NULL);

  return LIGMA_CONTAINER (tagged_container);
}

/**
 * ligma_tagged_container_set_filter:
 * @tagged_container: a #LigmaTaggedContainer object.
 * @tags:             list of #LigmaTag objects.
 *
 * Sets list of tags to be used for filtering. Only objects which have
 * all of the tags assigned match filtering criteria.
 **/
void
ligma_tagged_container_set_filter (LigmaTaggedContainer *tagged_container,
                                  GList               *tags)
{
  GList *new_filter;

  g_return_if_fail (LIGMA_IS_TAGGED_CONTAINER (tagged_container));

  if (tags)
    {
      GList *list;

      for (list = tags; list; list = g_list_next (list))
        g_return_if_fail (list->data == NULL || LIGMA_IS_TAG (list->data));
    }

  if (! ligma_container_frozen (LIGMA_FILTERED_CONTAINER (tagged_container)->src_container))
    {
      ligma_tagged_container_src_freeze (LIGMA_FILTERED_CONTAINER (tagged_container));
    }

  /*  ref new tags first, they could be the same as the old ones  */
  new_filter = g_list_copy (tags);
  g_list_foreach (new_filter, (GFunc) ligma_tag_or_null_ref, NULL);

  g_list_free_full (tagged_container->filter,
                    (GDestroyNotify) ligma_tag_or_null_unref);
  tagged_container->filter = new_filter;

  if (! ligma_container_frozen (LIGMA_FILTERED_CONTAINER (tagged_container)->src_container))
    {
      ligma_tagged_container_src_thaw (LIGMA_FILTERED_CONTAINER (tagged_container));
    }
}

/**
 * ligma_tagged_container_get_filter:
 * @tagged_container: a #LigmaTaggedContainer object.
 *
 * Returns current tag filter. Tag filter is a list of LigmaTag objects, which
 * must be contained by each object matching filter criteria.
 *
 * Returns: a list of LigmaTag objects used as filter. This value should
 *               not be modified or freed.
 **/
const GList *
ligma_tagged_container_get_filter (LigmaTaggedContainer *tagged_container)
{
  g_return_val_if_fail (LIGMA_IS_TAGGED_CONTAINER (tagged_container), NULL);

  return tagged_container->filter;
}

static gboolean
ligma_tagged_container_object_matches (LigmaTaggedContainer *tagged_container,
                                      LigmaObject          *object)
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

      if (! ligma_tagged_has_tag (LIGMA_TAGGED (object),
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
ligma_tagged_container_tag_added (LigmaTagged          *tagged,
                                 LigmaTag             *tag,
                                 LigmaTaggedContainer *tagged_container)
{
  ligma_tagged_container_ref_tag (tagged_container, tag);

  if (ligma_tagged_container_object_matches (tagged_container,
                                            LIGMA_OBJECT (tagged)) &&
      ! ligma_container_have (LIGMA_CONTAINER (tagged_container),
                             LIGMA_OBJECT (tagged)))
    {
      ligma_container_add (LIGMA_CONTAINER (tagged_container),
                          LIGMA_OBJECT (tagged));
    }
}

static void
ligma_tagged_container_tag_removed (LigmaTagged          *tagged,
                                   LigmaTag             *tag,
                                   LigmaTaggedContainer *tagged_container)
{
  ligma_tagged_container_unref_tag (tagged_container, tag);

  if (! ligma_tagged_container_object_matches (tagged_container,
                                              LIGMA_OBJECT (tagged)) &&
      ligma_container_have (LIGMA_CONTAINER (tagged_container),
                           LIGMA_OBJECT (tagged)))
    {
      ligma_container_remove (LIGMA_CONTAINER (tagged_container),
                             LIGMA_OBJECT (tagged));
    }
}

static void
ligma_tagged_container_ref_tag (LigmaTaggedContainer *tagged_container,
                               LigmaTag             *tag)
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
                     ligma_tagged_container_signals[TAG_COUNT_CHANGED], 0,
                     tagged_container->tag_count);
    }
}

static void
ligma_tagged_container_unref_tag (LigmaTaggedContainer *tagged_container,
                                 LigmaTag             *tag)
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
                         ligma_tagged_container_signals[TAG_COUNT_CHANGED], 0,
                         tagged_container->tag_count);
        }
    }
}

static void
ligma_tagged_container_tag_count_changed (LigmaTaggedContainer *container,
                                         gint                 tag_count)
{
}

/**
 * ligma_tagged_container_get_tag_count:
 * @container:  a #LigmaTaggedContainer object.
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
ligma_tagged_container_get_tag_count (LigmaTaggedContainer *container)
{
  g_return_val_if_fail (LIGMA_IS_TAGGED_CONTAINER (container), 0);

  return container->tag_count;
}
