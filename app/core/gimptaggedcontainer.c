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
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <glib-object.h>

#include "core-types.h"

#include "gimp.h"
#include "gimpmarshal.h"
#include "gimptag.h"
#include "gimptagged.h"
#include "gimptaggedcontainer.h"


enum
{
  TAG_COUNT_CHANGED,
  LAST_SIGNAL
};

enum
{
  PROP_0,
  PROP_SRC_CONTAINER
};


typedef struct _MatchParams
{
  GimpTaggedContainer *tagged_container;
  GList               *items_to_add;
  GList               *items_to_remove;
} MatchParams;


static void      gimp_tagged_container_constructed        (GObject             *object);
static void      gimp_tagged_container_dispose            (GObject             *object);
static void      gimp_tagged_container_set_property       (GObject             *object,
                                                           guint                property_id,
                                                           const GValue        *value,
                                                           GParamSpec          *pspec);
static void      gimp_tagged_container_get_property       (GObject             *object,
                                                           guint                property_id,
                                                           GValue              *value,
                                                           GParamSpec          *pspec);

static gint64    gimp_tagged_container_get_memsize        (GimpObject          *object,
                                                           gint64              *gui_size);

static gboolean  gimp_tagged_container_object_matches     (GimpTaggedContainer *tagged_container,
                                                           GimpObject          *object);

static void      gimp_tagged_container_filter             (GimpTaggedContainer *tagged_container);

static void      gimp_tagged_container_src_add            (GimpContainer       *src_container,
                                                           GimpObject          *obj,
                                                           GimpTaggedContainer *tagged_container);
static void      gimp_tagged_container_src_remove         (GimpContainer       *src_container,
                                                           GimpObject          *obj,
                                                           GimpTaggedContainer *tagged_container);
static void      gimp_tagged_container_src_freeze         (GimpContainer       *src_container,
                                                           GimpTaggedContainer *tagged_container);
static void      gimp_tagged_container_src_thaw           (GimpContainer       *src_container,
                                                           GimpTaggedContainer *tagged_container);
static void      gimp_tagged_container_tag_added          (GimpTagged          *tagged,
                                                           GimpTag             *tag,
                                                           GimpTaggedContainer *tagged_container);
static void      gimp_tagged_container_tag_removed        (GimpTagged          *tagged,
                                                           GimpTag             *tag,
                                                           GimpTaggedContainer *tagged_container);
static void      gimp_tagged_container_tagged_item_added  (GimpTagged          *tagged,
                                                           GimpTaggedContainer *tagged_container);
static void      gimp_tagged_container_tagged_item_removed(GimpTagged          *tagged,
                                                           GimpTaggedContainer *tagged_container);
static void      gimp_tagged_container_tag_count_changed  (GimpTaggedContainer *tagged_container,
                                                           gint                 tag_count);


G_DEFINE_TYPE (GimpTaggedContainer, gimp_tagged_container, GIMP_TYPE_LIST)

#define parent_class gimp_tagged_container_parent_class

static guint gimp_tagged_container_signals[LAST_SIGNAL] = { 0, };


static void
gimp_tagged_container_class_init (GimpTaggedContainerClass *klass)
{
  GObjectClass    *g_object_class    = G_OBJECT_CLASS (klass);
  GimpObjectClass *gimp_object_class = GIMP_OBJECT_CLASS (klass);

  g_object_class->constructed    = gimp_tagged_container_constructed;
  g_object_class->dispose        = gimp_tagged_container_dispose;
  g_object_class->set_property   = gimp_tagged_container_set_property;
  g_object_class->get_property   = gimp_tagged_container_get_property;

  gimp_object_class->get_memsize = gimp_tagged_container_get_memsize;

  klass->tag_count_changed       = gimp_tagged_container_tag_count_changed;

  gimp_tagged_container_signals[TAG_COUNT_CHANGED] =
    g_signal_new ("tag-count-changed",
                  GIMP_TYPE_TAGGED_CONTAINER,
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (GimpTaggedContainerClass, tag_count_changed),
                  NULL, NULL,
                  gimp_marshal_VOID__INT,
                  G_TYPE_NONE, 1,
                  G_TYPE_INT);

  g_object_class_install_property (g_object_class, PROP_SRC_CONTAINER,
                                   g_param_spec_object ("src-container",
                                                        NULL, NULL,
                                                        GIMP_TYPE_CONTAINER,
                                                        GIMP_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT_ONLY));
}

static void
gimp_tagged_container_init (GimpTaggedContainer *tagged_container)
{
  tagged_container->src_container  = NULL;
  tagged_container->filter         = NULL;
  tagged_container->tag_ref_counts = NULL;
  tagged_container->tag_count      = 0;
}

static void
gimp_tagged_container_constructed (GObject *object)
{
  GimpTaggedContainer *tagged_container = GIMP_TAGGED_CONTAINER (object);

  if (G_OBJECT_CLASS (parent_class)->constructed)
    G_OBJECT_CLASS (parent_class)->constructed (object);

  tagged_container->tag_ref_counts =
    g_hash_table_new ((GHashFunc) gimp_tag_get_hash,
                      (GEqualFunc) gimp_tag_equals);

  gimp_container_foreach (tagged_container->src_container,
                          (GFunc) gimp_tagged_container_tagged_item_added,
                          tagged_container);
  gimp_tagged_container_filter (tagged_container);
}

static void
gimp_tagged_container_dispose (GObject *object)
{
  GimpTaggedContainer *tagged_container = GIMP_TAGGED_CONTAINER (object);

  if (tagged_container->tag_ref_counts)
    {
      g_hash_table_unref (tagged_container->tag_ref_counts);
      tagged_container->tag_ref_counts = NULL;
    }

  if (tagged_container->src_container)
    {
      g_signal_handlers_disconnect_by_func (tagged_container->src_container,
                                            gimp_tagged_container_src_add,
                                            tagged_container);
      g_signal_handlers_disconnect_by_func (tagged_container->src_container,
                                            gimp_tagged_container_src_remove,
                                            tagged_container);
      g_signal_handlers_disconnect_by_func (tagged_container->src_container,
                                            gimp_tagged_container_src_freeze,
                                            tagged_container);
      g_signal_handlers_disconnect_by_func (tagged_container->src_container,
                                            gimp_tagged_container_src_thaw,
                                            tagged_container);

      g_object_unref (tagged_container->src_container);
      tagged_container->src_container = NULL;
    }

  G_OBJECT_CLASS (parent_class)->dispose (object);
}

static void
gimp_tagged_container_set_property (GObject      *object,
                                    guint         property_id,
                                    const GValue *value,
                                    GParamSpec   *pspec)
{
  GimpTaggedContainer *tagged_container = GIMP_TAGGED_CONTAINER (object);

  switch (property_id)
    {
    case PROP_SRC_CONTAINER:
      tagged_container->src_container = g_value_dup_object (value);

      g_signal_connect (tagged_container->src_container, "add",
                        G_CALLBACK (gimp_tagged_container_src_add),
                        tagged_container);
      g_signal_connect (tagged_container->src_container, "remove",
                        G_CALLBACK (gimp_tagged_container_src_remove),
                        tagged_container);
      g_signal_connect (tagged_container->src_container, "freeze",
                        G_CALLBACK (gimp_tagged_container_src_freeze),
                        tagged_container);
      g_signal_connect (tagged_container->src_container, "thaw",
                        G_CALLBACK (gimp_tagged_container_src_thaw),
                        tagged_container);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_tagged_container_get_property (GObject    *object,
                                    guint       property_id,
                                    GValue     *value,
                                    GParamSpec *pspec)
{
  GimpTaggedContainer *tagged_container = GIMP_TAGGED_CONTAINER (object);

  switch (property_id)
    {
    case PROP_SRC_CONTAINER:
      g_value_set_object (value, tagged_container->src_container);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
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

/**
 * gimp_tagged_container_new:
 * @src_container: container to be filtered.
 *
 * Creates a new #GimpTaggedContainer object which creates filtered
 * data view of #GimpTagged objects. It filters @src_container for objects
 * containing all of the filtering tags. Syncronization with @src_container
 * data is performed automatically.
 *
 * Return value: a new #GimpTaggedContainer object.
 **/
GimpContainer *
gimp_tagged_container_new (GimpContainer *src_container,
                           GCompareFunc   sort_func)
{
  GimpTaggedContainer *tagged_container;
  GType                  children_type;

  g_return_val_if_fail (GIMP_IS_CONTAINER (src_container), NULL);

  children_type = gimp_container_get_children_type (src_container);

  tagged_container = g_object_new (GIMP_TYPE_TAGGED_CONTAINER,
                                   "sort-func",     sort_func,
                                   "children-type", children_type,
                                   "policy",        GIMP_CONTAINER_POLICY_WEAK,
                                   "unique-names",  FALSE,
                                   "src-container", src_container,
                                   NULL);

  return GIMP_CONTAINER (tagged_container);
}

/**
 * gimp_tagged_container_set_filter:
 * @tagged_container: a #GimpTaggedContainer object.
 * @tags:               list of #GimpTag objects.
 *
 * Sets list of tags to be used for filtering. Only objects which have all of
 * the tags assigned match filtering criteria.
 **/
void
gimp_tagged_container_set_filter (GimpTaggedContainer *tagged_container,
                                  GList               *tags)
{
  g_return_if_fail (GIMP_IS_TAGGED_CONTAINER (tagged_container));

  tagged_container->filter = tags;

  gimp_container_freeze (GIMP_CONTAINER (tagged_container));
  gimp_tagged_container_filter (tagged_container);
  gimp_container_thaw (GIMP_CONTAINER (tagged_container));
}

/**
 * gimp_tagged_container_get_filter:
 * @tagged_container: a #GimpTaggedContainer object.
 *
 * Returns current tag filter. Tag filter is a list of GimpTag objects, which
 * must be contained by each object matching filter criteria.
 *
 * Return value: a list of GimpTag objects used as filter. This value should
 * not be modified or freed.
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
  GList *filter_tag;
  GList *object_tag;

  filter_tag = tagged_container->filter;
  while (filter_tag)
    {
      if (! filter_tag->data)
        {
          /* invalid tag - does not match */
          return FALSE;
        }

      object_tag = gimp_tagged_get_tags (GIMP_TAGGED (object));
      while (object_tag)
        {
          if (gimp_tag_equals (GIMP_TAG (object_tag->data),
                               GIMP_TAG (filter_tag->data)))
            {
              /* found match for the tag */
              break;
            }
          object_tag = g_list_next (object_tag);
        }

      if (! object_tag)
        {
          /* match for the tag was not found.
           * since query is of type AND, it whole fails. */
          return FALSE;
        }

      filter_tag = g_list_next (filter_tag);
    }

  return TRUE;
}

static void
gimp_tagged_container_check_needs_remove (GimpObject  *object,
                                          MatchParams *match_params)
{
  if (! gimp_tagged_container_object_matches (match_params->tagged_container,
                                              object))
    {
      match_params->items_to_remove = g_list_prepend (match_params->items_to_remove,
                                                      object);
    }
}

static void
gimp_tagged_container_check_needs_add (GimpObject  *object,
                                         MatchParams *match_params)
{
  if (gimp_tagged_container_object_matches (match_params->tagged_container,
                                            object) &&
      ! gimp_container_have (GIMP_CONTAINER (match_params->tagged_container),
                             object))
    {
      match_params->items_to_add = g_list_prepend (match_params->items_to_add,
                                                   object);
    }
}

static void
gimp_tagged_container_filter (GimpTaggedContainer *tagged_container)
{
  MatchParams  match_params;
  GList       *list;

  match_params.tagged_container = tagged_container;
  match_params.items_to_add       = NULL;
  match_params.items_to_remove    = NULL;

  gimp_container_foreach (GIMP_CONTAINER (tagged_container),
                          (GFunc) gimp_tagged_container_check_needs_remove,
                          &match_params);
  gimp_container_foreach (GIMP_CONTAINER (tagged_container->src_container),
                          (GFunc) gimp_tagged_container_check_needs_add,
                          &match_params);

  for (list = match_params.items_to_remove; list; list = g_list_next (list))
    {
      gimp_container_remove (GIMP_CONTAINER (tagged_container),
                             GIMP_OBJECT (list->data));
    }

  for (list = match_params.items_to_add; list; list = g_list_next (list))
    {
      gimp_container_add (GIMP_CONTAINER (tagged_container),
                          GIMP_OBJECT (list->data));
    }

  g_list_free (match_params.items_to_add);
  g_list_free (match_params.items_to_remove);
}

static void
gimp_tagged_container_src_add (GimpContainer       *src_container,
                               GimpObject          *obj,
                               GimpTaggedContainer *tagged_container)
{
  gimp_tagged_container_tagged_item_added (GIMP_TAGGED (obj),
                                           tagged_container);

  if (! gimp_container_frozen (src_container))
    {
      gimp_container_add (GIMP_CONTAINER (tagged_container), obj);
    }
}

static void
gimp_tagged_container_src_remove (GimpContainer       *src_container,
                                  GimpObject          *obj,
                                  GimpTaggedContainer *tagged_container)
{
  gimp_tagged_container_tagged_item_removed (GIMP_TAGGED (obj),
                                             tagged_container);

  if (! gimp_container_frozen (src_container) &&
      gimp_container_have (GIMP_CONTAINER (tagged_container), obj))
    {
      gimp_container_remove (GIMP_CONTAINER (tagged_container), obj);
    }
}

static void
gimp_tagged_container_src_freeze (GimpContainer       *src_container,
                                  GimpTaggedContainer *tagged_container)
{
  gimp_container_freeze (GIMP_CONTAINER (tagged_container));
  gimp_container_clear (GIMP_CONTAINER (tagged_container));
}

static void
gimp_tagged_container_src_thaw (GimpContainer       *src_container,
                                GimpTaggedContainer *tagged_container)
{
  gimp_tagged_container_filter (tagged_container);
  gimp_container_thaw (GIMP_CONTAINER (tagged_container));
}

static void
gimp_tagged_container_tagged_item_added (GimpTagged          *tagged,
                                         GimpTaggedContainer *tagged_container)
{
  GList *list;

  for (list = gimp_tagged_get_tags (tagged); list; list = g_list_next (list))
    {
      gimp_tagged_container_tag_added (tagged,
                                       list->data,
                                       tagged_container);
    }

  g_signal_connect (tagged, "tag-added",
                    G_CALLBACK (gimp_tagged_container_tag_added),
                    tagged_container);
  g_signal_connect (tagged, "tag-removed",
                    G_CALLBACK (gimp_tagged_container_tag_removed),
                    tagged_container);
}

static void
gimp_tagged_container_tagged_item_removed (GimpTagged          *tagged,
                                           GimpTaggedContainer *tagged_container)
{
  GList *list;

  g_signal_handlers_disconnect_by_func (tagged,
                                        gimp_tagged_container_tag_added,
                                        tagged_container);
  g_signal_handlers_disconnect_by_func (tagged,
                                        gimp_tagged_container_tag_removed,
                                        tagged_container);

  for (list = gimp_tagged_get_tags (tagged); list; list = g_list_next (list))
    {
      gimp_tagged_container_tag_removed (tagged,
                                         GIMP_TAG (list->data),
                                         tagged_container);
    }

}

static void
gimp_tagged_container_tag_added (GimpTagged          *tagged,
                                 GimpTag             *tag,
                                 GimpTaggedContainer *tagged_container)
{
  gint ref_count;

  ref_count = GPOINTER_TO_INT (g_hash_table_lookup (tagged_container->tag_ref_counts,
                                                    tag));
  ref_count++;
  g_hash_table_insert (tagged_container->tag_ref_counts,
                       tag, GINT_TO_POINTER (ref_count));
  if (ref_count == 1)
    {
      tagged_container->tag_count++;
      g_signal_emit (tagged_container,
                     gimp_tagged_container_signals[TAG_COUNT_CHANGED], 0,
                     tagged_container->tag_count);
    }
}

static void
gimp_tagged_container_tag_removed (GimpTagged          *tagged,
                                   GimpTag             *tag,
                                   GimpTaggedContainer *tagged_container)
{
  gint ref_count;

  ref_count = GPOINTER_TO_INT (g_hash_table_lookup (tagged_container->tag_ref_counts,
                                                    tag));
  ref_count--;

  if (ref_count > 0)
    {
      g_hash_table_insert (tagged_container->tag_ref_counts,
                           tag, GINT_TO_POINTER (ref_count));
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
 * Return value: number of distinct tags assigned to all objects in the
 *               container.
 **/
gint
gimp_tagged_container_get_tag_count (GimpTaggedContainer *container)
{
  g_return_val_if_fail (GIMP_IS_TAGGED_CONTAINER (container), 0);

  return container->tag_count;
}
