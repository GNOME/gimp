/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995-1997 Spencer Kimball and Peter Mattis
 *
 * gimpfilteredcontainer.c
 * Copyright (C) 2008 Aurimas Juška <aurisj@svn.gnome.org>
 *               2011 Michael Natterer <mitch@gimp.org>
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

#include "gimpfilteredcontainer.h"


enum
{
  PROP_0,
  PROP_SRC_CONTAINER,
  PROP_FILTER_FUNC,
  PROP_FILTER_DATA
};


static void   gimp_filtered_container_constructed  (GObject               *object);
static void   gimp_filtered_container_dispose      (GObject               *object);
static void   gimp_filtered_container_set_property (GObject               *object,
                                                    guint                  property_id,
                                                    const GValue          *value,
                                                    GParamSpec            *pspec);
static void   gimp_filtered_container_get_property (GObject               *object,
                                                    guint                  property_id,
                                                    GValue                *value,
                                                    GParamSpec            *pspec);

static void   gimp_filtered_container_filter       (GimpFilteredContainer *filtered_container);
static void   gimp_filtered_container_src_add      (GimpContainer         *src_container,
                                                    GimpObject            *obj,
                                                    GimpFilteredContainer *filtered_container);
static void   gimp_filtered_container_src_remove   (GimpContainer         *src_container,
                                                    GimpObject            *obj,
                                                    GimpFilteredContainer *filtered_container);
static void   gimp_filtered_container_src_freeze   (GimpContainer         *src_container,
                                                    GimpFilteredContainer *filtered_container);
static void   gimp_filtered_container_src_thaw     (GimpContainer         *src_container,
                                                    GimpFilteredContainer *filtered_container);


G_DEFINE_TYPE (GimpFilteredContainer, gimp_filtered_container, GIMP_TYPE_LIST)

#define parent_class gimp_filtered_container_parent_class


static void
gimp_filtered_container_class_init (GimpFilteredContainerClass *klass)
{
  GObjectClass *g_object_class = G_OBJECT_CLASS (klass);

  g_object_class->constructed  = gimp_filtered_container_constructed;
  g_object_class->dispose      = gimp_filtered_container_dispose;
  g_object_class->set_property = gimp_filtered_container_set_property;
  g_object_class->get_property = gimp_filtered_container_get_property;

  g_object_class_install_property (g_object_class, PROP_SRC_CONTAINER,
                                   g_param_spec_object ("src-container",
                                                        NULL, NULL,
                                                        GIMP_TYPE_CONTAINER,
                                                        GIMP_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT_ONLY));

  g_object_class_install_property (g_object_class, PROP_FILTER_FUNC,
                                   g_param_spec_pointer ("filter-func",
                                                         NULL, NULL,
                                                         GIMP_PARAM_READWRITE |
                                                         G_PARAM_CONSTRUCT_ONLY));

  g_object_class_install_property (g_object_class, PROP_FILTER_DATA,
                                   g_param_spec_pointer ("filter-data",
                                                         NULL, NULL,
                                                         GIMP_PARAM_READWRITE |
                                                         G_PARAM_CONSTRUCT_ONLY));
}

static void
gimp_filtered_container_init (GimpFilteredContainer *filtered_container)
{
}

static void
gimp_filtered_container_constructed (GObject *object)
{
  GimpFilteredContainer *filtered_container = GIMP_FILTERED_CONTAINER (object);

  if (G_OBJECT_CLASS (parent_class)->constructed)
    G_OBJECT_CLASS (parent_class)->constructed (object);

  g_assert (GIMP_IS_CONTAINER (filtered_container->src_container));
  g_assert (filtered_container->filter_func != NULL);

  if (! gimp_container_frozen (filtered_container->src_container))
    gimp_filtered_container_filter (filtered_container);
}

static void
gimp_filtered_container_dispose (GObject *object)
{
  GimpFilteredContainer *filtered_container = GIMP_FILTERED_CONTAINER (object);

  if (filtered_container->src_container)
    {
      g_signal_handlers_disconnect_by_func (filtered_container->src_container,
                                            gimp_filtered_container_src_add,
                                            filtered_container);
      g_signal_handlers_disconnect_by_func (filtered_container->src_container,
                                            gimp_filtered_container_src_remove,
                                            filtered_container);
      g_signal_handlers_disconnect_by_func (filtered_container->src_container,
                                            gimp_filtered_container_src_freeze,
                                            filtered_container);
      g_signal_handlers_disconnect_by_func (filtered_container->src_container,
                                            gimp_filtered_container_src_thaw,
                                            filtered_container);

      g_object_unref (filtered_container->src_container);
      filtered_container->src_container = NULL;
    }

  G_OBJECT_CLASS (parent_class)->dispose (object);
}

static void
gimp_filtered_container_set_property (GObject      *object,
                                      guint         property_id,
                                      const GValue *value,
                                      GParamSpec   *pspec)
{
  GimpFilteredContainer *filtered_container = GIMP_FILTERED_CONTAINER (object);

  switch (property_id)
    {
    case PROP_SRC_CONTAINER:
      filtered_container->src_container = g_value_dup_object (value);

      g_signal_connect (filtered_container->src_container, "add",
                        G_CALLBACK (gimp_filtered_container_src_add),
                        filtered_container);
      g_signal_connect (filtered_container->src_container, "remove",
                        G_CALLBACK (gimp_filtered_container_src_remove),
                        filtered_container);
      g_signal_connect (filtered_container->src_container, "freeze",
                        G_CALLBACK (gimp_filtered_container_src_freeze),
                        filtered_container);
      g_signal_connect (filtered_container->src_container, "thaw",
                        G_CALLBACK (gimp_filtered_container_src_thaw),
                        filtered_container);
      break;

    case PROP_FILTER_FUNC:
      filtered_container->filter_func = g_value_get_pointer (value);
      break;

    case PROP_FILTER_DATA:
      filtered_container->filter_data = g_value_get_pointer (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_filtered_container_get_property (GObject    *object,
                                      guint       property_id,
                                      GValue     *value,
                                      GParamSpec *pspec)
{
  GimpFilteredContainer *filtered_container = GIMP_FILTERED_CONTAINER (object);

  switch (property_id)
    {
    case PROP_SRC_CONTAINER:
      g_value_set_object (value, filtered_container->src_container);
      break;

    case PROP_FILTER_FUNC:
      g_value_set_pointer (value, filtered_container->filter_func);
      break;

    case PROP_FILTER_DATA:
      g_value_set_pointer (value, filtered_container->filter_data);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

/**
 * gimp_filtered_container_new:
 * @src_container: container to be filtered.
 *
 * Creates a new #GimpFilteredContainer object which creates filtered
 * data view of #GimpTagged objects. It filters @src_container for objects
 * containing all of the filtering tags. Syncronization with @src_container
 * data is performed automatically.
 *
 * Return value: a new #GimpFilteredContainer object.
 **/
GimpContainer *
gimp_filtered_container_new (GimpContainer        *src_container,
                             GimpObjectFilterFunc  filter_func,
                             gpointer              filter_data)
{
  GType        children_type;
  GCompareFunc sort_func;

  g_return_val_if_fail (GIMP_IS_LIST (src_container), NULL);
  g_return_val_if_fail (filter_func != NULL, NULL);

  children_type = gimp_container_get_children_type (src_container);
  sort_func     = GIMP_LIST (src_container)->sort_func;

  return g_object_new (GIMP_TYPE_FILTERED_CONTAINER,
                       "sort-func",     sort_func,
                       "children-type", children_type,
                       "policy",        GIMP_CONTAINER_POLICY_WEAK,
                       "unique-names",  FALSE,
                       "src-container", src_container,
                       "filter-func",   filter_func,
                       "filter-data",   filter_data,
                       NULL);
}

static void
gimp_filtered_container_filter (GimpFilteredContainer *filtered_container)
{
  GList *list;

  for (list = GIMP_LIST (filtered_container->src_container)->list;
       list;
       list = g_list_next (list))
    {
      GimpObject *child = list->data;

      if (filtered_container->filter_func (child,
                                           filtered_container->filter_data))
        {
          gimp_container_add (GIMP_CONTAINER (filtered_container), child);
        }
    }
}

static void
gimp_filtered_container_src_add (GimpContainer         *src_container,
                                 GimpObject            *obj,
                                 GimpFilteredContainer *filtered_container)
{
  if (! gimp_container_frozen (src_container) &&
      filtered_container->filter_func (obj, filtered_container->filter_data))
    {
      gimp_container_add (GIMP_CONTAINER (filtered_container), obj);
    }
}

static void
gimp_filtered_container_src_remove (GimpContainer         *src_container,
                                    GimpObject            *obj,
                                    GimpFilteredContainer *filtered_container)
{
  if (! gimp_container_frozen (src_container) &&
      filtered_container->filter_func (obj, filtered_container->filter_data))
    {
      gimp_container_remove (GIMP_CONTAINER (filtered_container), obj);
    }
}

static void
gimp_filtered_container_src_freeze (GimpContainer         *src_container,
                                    GimpFilteredContainer *filtered_container)
{
  gimp_container_freeze (GIMP_CONTAINER (filtered_container));
  gimp_container_clear (GIMP_CONTAINER (filtered_container));
}

static void
gimp_filtered_container_src_thaw (GimpContainer         *src_container,
                                  GimpFilteredContainer *filtered_container)
{
  gimp_filtered_container_filter (filtered_container);
  gimp_container_thaw (GIMP_CONTAINER (filtered_container));
}
