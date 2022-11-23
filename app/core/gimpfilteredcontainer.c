/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995-1997 Spencer Kimball and Peter Mattis
 *
 * ligmafilteredcontainer.c
 * Copyright (C) 2008 Aurimas Juška <aurisj@svn.gnome.org>
 *               2011 Michael Natterer <mitch@ligma.org>
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

#include "libligmabase/ligmabase.h"

#include "core-types.h"

#include "ligmafilteredcontainer.h"


enum
{
  PROP_0,
  PROP_SRC_CONTAINER,
  PROP_FILTER_FUNC,
  PROP_FILTER_DATA
};


static void     ligma_filtered_container_constructed     (GObject               *object);
static void     ligma_filtered_container_dispose         (GObject               *object);
static void     ligma_filtered_container_finalize        (GObject               *object);
static void     ligma_filtered_container_set_property    (GObject               *object,
                                                         guint                  property_id,
                                                         const GValue          *value,
                                                         GParamSpec            *pspec);
static void     ligma_filtered_container_get_property    (GObject               *object,
                                                         guint                  property_id,
                                                         GValue                *value,
                                                         GParamSpec            *pspec);

static void     ligma_filtered_container_real_src_add    (LigmaFilteredContainer *filtered_container,
                                                         LigmaObject            *object);
static void     ligma_filtered_container_real_src_remove (LigmaFilteredContainer *filtered_container,
                                                         LigmaObject            *object);
static void     ligma_filtered_container_real_src_freeze (LigmaFilteredContainer *filtered_container);
static void     ligma_filtered_container_real_src_thaw   (LigmaFilteredContainer *filtered_container);

static gboolean ligma_filtered_container_object_matches  (LigmaFilteredContainer *filtered_container,
                                                         LigmaObject            *object);
static void     ligma_filtered_container_src_add         (LigmaContainer         *src_container,
                                                         LigmaObject            *obj,
                                                         LigmaFilteredContainer *filtered_container);
static void     ligma_filtered_container_src_remove      (LigmaContainer         *src_container,
                                                         LigmaObject            *obj,
                                                         LigmaFilteredContainer *filtered_container);
static void     ligma_filtered_container_src_freeze      (LigmaContainer         *src_container,
                                                         LigmaFilteredContainer *filtered_container);
static void     ligma_filtered_container_src_thaw        (LigmaContainer         *src_container,
                                                         LigmaFilteredContainer *filtered_container);


G_DEFINE_TYPE (LigmaFilteredContainer, ligma_filtered_container, LIGMA_TYPE_LIST)

#define parent_class ligma_filtered_container_parent_class


static void
ligma_filtered_container_class_init (LigmaFilteredContainerClass *klass)
{
  GObjectClass               *g_object_class = G_OBJECT_CLASS (klass);
  LigmaFilteredContainerClass *filtered_class = LIGMA_FILTERED_CONTAINER_CLASS (klass);

  g_object_class->constructed  = ligma_filtered_container_constructed;
  g_object_class->dispose      = ligma_filtered_container_dispose;
  g_object_class->finalize     = ligma_filtered_container_finalize;
  g_object_class->set_property = ligma_filtered_container_set_property;
  g_object_class->get_property = ligma_filtered_container_get_property;

  filtered_class->src_add      = ligma_filtered_container_real_src_add;
  filtered_class->src_remove   = ligma_filtered_container_real_src_remove;
  filtered_class->src_freeze   = ligma_filtered_container_real_src_freeze;
  filtered_class->src_thaw     = ligma_filtered_container_real_src_thaw;

  g_object_class_install_property (g_object_class, PROP_SRC_CONTAINER,
                                   g_param_spec_object ("src-container",
                                                        NULL, NULL,
                                                        LIGMA_TYPE_CONTAINER,
                                                        LIGMA_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT_ONLY));

  g_object_class_install_property (g_object_class, PROP_FILTER_FUNC,
                                   g_param_spec_pointer ("filter-func",
                                                         NULL, NULL,
                                                         LIGMA_PARAM_READWRITE |
                                                         G_PARAM_CONSTRUCT_ONLY));

  g_object_class_install_property (g_object_class, PROP_FILTER_DATA,
                                   g_param_spec_pointer ("filter-data",
                                                         NULL, NULL,
                                                         LIGMA_PARAM_READWRITE |
                                                         G_PARAM_CONSTRUCT_ONLY));
}

static void
ligma_filtered_container_init (LigmaFilteredContainer *filtered_container)
{
}

static void
ligma_filtered_container_constructed (GObject *object)
{
  LigmaFilteredContainer *filtered_container = LIGMA_FILTERED_CONTAINER (object);

  G_OBJECT_CLASS (parent_class)->constructed (object);

  ligma_assert (LIGMA_IS_CONTAINER (filtered_container->src_container));

  if (! ligma_container_frozen (filtered_container->src_container))
    {
      /*  a freeze/thaw can't hurt on a newly created container because
       *  we can't have any views yet. This way we get away without
       *  having a virtual function for initializing the container.
       */
      ligma_filtered_container_src_freeze (filtered_container->src_container,
                                          filtered_container);
      ligma_filtered_container_src_thaw (filtered_container->src_container,
                                        filtered_container);
    }
}

static void
ligma_filtered_container_dispose (GObject *object)
{
  LigmaFilteredContainer *filtered_container = LIGMA_FILTERED_CONTAINER (object);

  if (filtered_container->src_container)
    {
      g_signal_handlers_disconnect_by_func (filtered_container->src_container,
                                            ligma_filtered_container_src_add,
                                            filtered_container);
      g_signal_handlers_disconnect_by_func (filtered_container->src_container,
                                            ligma_filtered_container_src_remove,
                                            filtered_container);
      g_signal_handlers_disconnect_by_func (filtered_container->src_container,
                                            ligma_filtered_container_src_freeze,
                                            filtered_container);
      g_signal_handlers_disconnect_by_func (filtered_container->src_container,
                                            ligma_filtered_container_src_thaw,
                                            filtered_container);
    }

  G_OBJECT_CLASS (parent_class)->dispose (object);
}

static void
ligma_filtered_container_finalize (GObject *object)
{
  LigmaFilteredContainer *filtered_container = LIGMA_FILTERED_CONTAINER (object);

  g_clear_object (&filtered_container->src_container);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
ligma_filtered_container_set_property (GObject      *object,
                                      guint         property_id,
                                      const GValue *value,
                                      GParamSpec   *pspec)
{
  LigmaFilteredContainer *filtered_container = LIGMA_FILTERED_CONTAINER (object);

  switch (property_id)
    {
    case PROP_SRC_CONTAINER:
      filtered_container->src_container = g_value_dup_object (value);

      g_signal_connect (filtered_container->src_container, "add",
                        G_CALLBACK (ligma_filtered_container_src_add),
                        filtered_container);
      g_signal_connect (filtered_container->src_container, "remove",
                        G_CALLBACK (ligma_filtered_container_src_remove),
                        filtered_container);
      g_signal_connect (filtered_container->src_container, "freeze",
                        G_CALLBACK (ligma_filtered_container_src_freeze),
                        filtered_container);
      g_signal_connect (filtered_container->src_container, "thaw",
                        G_CALLBACK (ligma_filtered_container_src_thaw),
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
ligma_filtered_container_get_property (GObject    *object,
                                      guint       property_id,
                                      GValue     *value,
                                      GParamSpec *pspec)
{
  LigmaFilteredContainer *filtered_container = LIGMA_FILTERED_CONTAINER (object);

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

static void
ligma_filtered_container_real_src_add (LigmaFilteredContainer *filtered_container,
                                      LigmaObject            *object)
{
  if (ligma_filtered_container_object_matches (filtered_container, object))
    {
      ligma_container_add (LIGMA_CONTAINER (filtered_container), object);
    }
}

static void
ligma_filtered_container_real_src_remove (LigmaFilteredContainer *filtered_container,
                                         LigmaObject            *object)
{
  if (ligma_filtered_container_object_matches (filtered_container, object))
    {
      ligma_container_remove (LIGMA_CONTAINER (filtered_container), object);
    }
}

static void
ligma_filtered_container_real_src_freeze (LigmaFilteredContainer *filtered_container)
{
  ligma_container_clear (LIGMA_CONTAINER (filtered_container));
}

static void
ligma_filtered_container_real_src_thaw (LigmaFilteredContainer *filtered_container)
{
  GList *list;

  for (list = LIGMA_LIST (filtered_container->src_container)->queue->head;
       list;
       list = g_list_next (list))
    {
      LigmaObject *object = list->data;

      if (ligma_filtered_container_object_matches (filtered_container, object))
        {
          ligma_container_add (LIGMA_CONTAINER (filtered_container), object);
        }
    }
}

/**
 * ligma_filtered_container_new:
 * @src_container: container to be filtered.
 *
 * Creates a new #LigmaFilteredContainer object which creates filtered
 * data view of #LigmaTagged objects. It filters @src_container for objects
 * containing all of the filtering tags. Synchronization with @src_container
 * data is performed automatically.
 *
 * Returns: a new #LigmaFilteredContainer object.
 **/
LigmaContainer *
ligma_filtered_container_new (LigmaContainer        *src_container,
                             LigmaObjectFilterFunc  filter_func,
                             gpointer              filter_data)
{
  GType        children_type;
  GCompareFunc sort_func;

  g_return_val_if_fail (LIGMA_IS_LIST (src_container), NULL);

  children_type = ligma_container_get_children_type (src_container);
  sort_func     = ligma_list_get_sort_func (LIGMA_LIST (src_container));

  return g_object_new (LIGMA_TYPE_FILTERED_CONTAINER,
                       "sort-func",     sort_func,
                       "children-type", children_type,
                       "policy",        LIGMA_CONTAINER_POLICY_WEAK,
                       "unique-names",  FALSE,
                       "src-container", src_container,
                       "filter-func",   filter_func,
                       "filter-data",   filter_data,
                       NULL);
}

static gboolean
ligma_filtered_container_object_matches (LigmaFilteredContainer *filtered_container,
                                        LigmaObject            *object)
{
  return (! filtered_container->filter_func ||
          filtered_container->filter_func (object,
                                           filtered_container->filter_data));
}

static void
ligma_filtered_container_src_add (LigmaContainer         *src_container,
                                 LigmaObject            *object,
                                 LigmaFilteredContainer *filtered_container)
{
  if (! ligma_container_frozen (filtered_container->src_container))
    {
      LIGMA_FILTERED_CONTAINER_GET_CLASS (filtered_container)->src_add (filtered_container,
                                                                       object);
    }
}

static void
ligma_filtered_container_src_remove (LigmaContainer         *src_container,
                                    LigmaObject            *object,
                                    LigmaFilteredContainer *filtered_container)
{
  if (! ligma_container_frozen (filtered_container->src_container))
    {
      LIGMA_FILTERED_CONTAINER_GET_CLASS (filtered_container)->src_remove (filtered_container,
                                                                          object);
    }
}

static void
ligma_filtered_container_src_freeze (LigmaContainer         *src_container,
                                    LigmaFilteredContainer *filtered_container)
{
  ligma_container_freeze (LIGMA_CONTAINER (filtered_container));

  LIGMA_FILTERED_CONTAINER_GET_CLASS (filtered_container)->src_freeze (filtered_container);
}

static void
ligma_filtered_container_src_thaw (LigmaContainer         *src_container,
                                  LigmaFilteredContainer *filtered_container)
{
  LIGMA_FILTERED_CONTAINER_GET_CLASS (filtered_container)->src_thaw (filtered_container);

  ligma_container_thaw (LIGMA_CONTAINER (filtered_container));
}
