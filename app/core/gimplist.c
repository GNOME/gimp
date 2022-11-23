/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995-1997 Spencer Kimball and Peter Mattis
 *
 * ligmalist.c
 * Copyright (C) 2001-2016 Michael Natterer <mitch@ligma.org>
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

#include <stdlib.h>
#include <string.h> /* strcmp */

#include <gio/gio.h>

#include "libligmabase/ligmabase.h"

#include "core-types.h"

#include "ligma-memsize.h"
#include "ligmalist.h"


enum
{
  PROP_0,
  PROP_UNIQUE_NAMES,
  PROP_SORT_FUNC,
  PROP_APPEND
};


static void         ligma_list_finalize           (GObject                 *object);
static void         ligma_list_set_property       (GObject                 *object,
                                                  guint                    property_id,
                                                  const GValue            *value,
                                                  GParamSpec              *pspec);
static void         ligma_list_get_property       (GObject                 *object,
                                                  guint                    property_id,
                                                  GValue                  *value,
                                                  GParamSpec              *pspec);

static gint64       ligma_list_get_memsize        (LigmaObject              *object,
                                                  gint64                  *gui_size);

static void         ligma_list_add                (LigmaContainer           *container,
                                                  LigmaObject              *object);
static void         ligma_list_remove             (LigmaContainer           *container,
                                                  LigmaObject              *object);
static void         ligma_list_reorder            (LigmaContainer           *container,
                                                  LigmaObject              *object,
                                                  gint                     new_index);
static void         ligma_list_clear              (LigmaContainer           *container);
static gboolean     ligma_list_have               (LigmaContainer           *container,
                                                  LigmaObject              *object);
static void         ligma_list_foreach            (LigmaContainer           *container,
                                                  GFunc                    func,
                                                  gpointer                 user_data);
static LigmaObject * ligma_list_search             (LigmaContainer           *container,
                                                  LigmaContainerSearchFunc  func,
                                                  gpointer                 user_data);
static gboolean     ligma_list_get_unique_names   (LigmaContainer           *container);
static LigmaObject * ligma_list_get_child_by_name  (LigmaContainer           *container,
                                                  const gchar             *name);
static LigmaObject * ligma_list_get_child_by_index (LigmaContainer           *container,
                                                  gint                     index);
static gint         ligma_list_get_child_index    (LigmaContainer           *container,
                                                  LigmaObject              *object);

static void         ligma_list_uniquefy_name      (LigmaList                *ligma_list,
                                                  LigmaObject              *object);
static void         ligma_list_object_renamed     (LigmaObject              *object,
                                                  LigmaList                *list);


G_DEFINE_TYPE (LigmaList, ligma_list, LIGMA_TYPE_CONTAINER)

#define parent_class ligma_list_parent_class


static void
ligma_list_class_init (LigmaListClass *klass)
{
  GObjectClass       *object_class      = G_OBJECT_CLASS (klass);
  LigmaObjectClass    *ligma_object_class = LIGMA_OBJECT_CLASS (klass);
  LigmaContainerClass *container_class   = LIGMA_CONTAINER_CLASS (klass);

  object_class->finalize              = ligma_list_finalize;
  object_class->set_property          = ligma_list_set_property;
  object_class->get_property          = ligma_list_get_property;

  ligma_object_class->get_memsize      = ligma_list_get_memsize;

  container_class->add                = ligma_list_add;
  container_class->remove             = ligma_list_remove;
  container_class->reorder            = ligma_list_reorder;
  container_class->clear              = ligma_list_clear;
  container_class->have               = ligma_list_have;
  container_class->foreach            = ligma_list_foreach;
  container_class->search             = ligma_list_search;
  container_class->get_unique_names   = ligma_list_get_unique_names;
  container_class->get_child_by_name  = ligma_list_get_child_by_name;
  container_class->get_child_by_index = ligma_list_get_child_by_index;
  container_class->get_child_index    = ligma_list_get_child_index;

  g_object_class_install_property (object_class, PROP_UNIQUE_NAMES,
                                   g_param_spec_boolean ("unique-names",
                                                         NULL, NULL,
                                                         FALSE,
                                                         LIGMA_PARAM_READWRITE |
                                                         G_PARAM_CONSTRUCT_ONLY));

  g_object_class_install_property (object_class, PROP_SORT_FUNC,
                                   g_param_spec_pointer ("sort-func",
                                                         NULL, NULL,
                                                         LIGMA_PARAM_READWRITE |
                                                         G_PARAM_CONSTRUCT));

  g_object_class_install_property (object_class, PROP_APPEND,
                                   g_param_spec_boolean ("append",
                                                         NULL, NULL,
                                                         FALSE,
                                                         LIGMA_PARAM_READWRITE |
                                                         G_PARAM_CONSTRUCT));
}

static void
ligma_list_init (LigmaList *list)
{
  list->queue        = g_queue_new ();
  list->unique_names = FALSE;
  list->sort_func    = NULL;
  list->append       = FALSE;
}

static void
ligma_list_finalize (GObject *object)
{
  LigmaList *list = LIGMA_LIST (object);

  if (list->queue)
    {
      g_queue_free (list->queue);
      list->queue = NULL;
    }

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
ligma_list_set_property (GObject      *object,
                        guint         property_id,
                        const GValue *value,
                        GParamSpec   *pspec)
{
  LigmaList *list = LIGMA_LIST (object);

  switch (property_id)
    {
    case PROP_UNIQUE_NAMES:
      list->unique_names = g_value_get_boolean (value);
      break;
    case PROP_SORT_FUNC:
      ligma_list_set_sort_func (list, g_value_get_pointer (value));
      break;
    case PROP_APPEND:
      list->append = g_value_get_boolean (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
ligma_list_get_property (GObject    *object,
                        guint       property_id,
                        GValue     *value,
                        GParamSpec *pspec)
{
  LigmaList *list = LIGMA_LIST (object);

  switch (property_id)
    {
    case PROP_UNIQUE_NAMES:
      g_value_set_boolean (value, list->unique_names);
      break;
    case PROP_SORT_FUNC:
      g_value_set_pointer (value, list->sort_func);
      break;
    case PROP_APPEND:
      g_value_set_boolean (value, list->append);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static gint64
ligma_list_get_memsize (LigmaObject *object,
                       gint64     *gui_size)
{
  LigmaList *list    = LIGMA_LIST (object);
  gint64    memsize = 0;

  if (ligma_container_get_policy (LIGMA_CONTAINER (list)) ==
      LIGMA_CONTAINER_POLICY_STRONG)
    {
      memsize += ligma_g_queue_get_memsize_foreach (list->queue,
                                                   (LigmaMemsizeFunc) ligma_object_get_memsize,
                                                   gui_size);
    }
  else
    {
      memsize += ligma_g_queue_get_memsize (list->queue, 0);
    }

  return memsize + LIGMA_OBJECT_CLASS (parent_class)->get_memsize (object,
                                                                  gui_size);
}

static gint
ligma_list_sort_func (gconstpointer a,
                     gconstpointer b,
                     gpointer      user_data)
{
  GCompareFunc func = user_data;

  return func (a, b);
}

static void
ligma_list_add (LigmaContainer *container,
               LigmaObject    *object)
{
  LigmaList *list = LIGMA_LIST (container);

  if (list->unique_names)
    ligma_list_uniquefy_name (list, object);

  if (list->unique_names || list->sort_func)
    g_signal_connect (object, "name-changed",
                      G_CALLBACK (ligma_list_object_renamed),
                      list);

  if (list->sort_func)
    {
      g_queue_insert_sorted (list->queue, object, ligma_list_sort_func,
                             list->sort_func);
    }
  else if (list->append)
    {
      g_queue_push_tail (list->queue, object);
    }
  else
    {
      g_queue_push_head (list->queue, object);
    }

  LIGMA_CONTAINER_CLASS (parent_class)->add (container, object);
}

static void
ligma_list_remove (LigmaContainer *container,
                  LigmaObject    *object)
{
  LigmaList *list = LIGMA_LIST (container);

  if (list->unique_names || list->sort_func)
    g_signal_handlers_disconnect_by_func (object,
                                          ligma_list_object_renamed,
                                          list);

  g_queue_remove (list->queue, object);

  LIGMA_CONTAINER_CLASS (parent_class)->remove (container, object);
}

static void
ligma_list_reorder (LigmaContainer *container,
                   LigmaObject    *object,
                   gint           new_index)
{
  LigmaList *list = LIGMA_LIST (container);

  g_queue_remove (list->queue, object);

  if (new_index == ligma_container_get_n_children (container) - 1)
    g_queue_push_tail (list->queue, object);
  else
    g_queue_push_nth (list->queue, object, new_index);
}

static void
ligma_list_clear (LigmaContainer *container)
{
  LigmaList *list = LIGMA_LIST (container);

  while (g_queue_peek_head (list->queue))
    ligma_container_remove (container, g_queue_peek_head (list->queue));
}

static gboolean
ligma_list_have (LigmaContainer *container,
                LigmaObject    *object)
{
  LigmaList *list = LIGMA_LIST (container);

  return g_queue_find (list->queue, object) ? TRUE : FALSE;
}

static void
ligma_list_foreach (LigmaContainer *container,
                   GFunc          func,
                   gpointer       user_data)
{
  LigmaList *list = LIGMA_LIST (container);

  g_queue_foreach (list->queue, func, user_data);
}

static LigmaObject *
ligma_list_search (LigmaContainer           *container,
                  LigmaContainerSearchFunc  func,
                  gpointer                 user_data)
{
  LigmaList *list = LIGMA_LIST (container);
  GList    *iter;

  iter = list->queue->head;

  while (iter)
    {
      LigmaObject *object = iter->data;

      iter = g_list_next (iter);

      if (func (object, user_data))
        return object;
    }

  return NULL;
}

static gboolean
ligma_list_get_unique_names (LigmaContainer *container)
{
  LigmaList *list = LIGMA_LIST (container);

  return list->unique_names;
}

static LigmaObject *
ligma_list_get_child_by_name (LigmaContainer *container,
                             const gchar   *name)
{
  LigmaList *list = LIGMA_LIST (container);
  GList    *glist;

  for (glist = list->queue->head; glist; glist = g_list_next (glist))
    {
      LigmaObject *object = glist->data;

      if (! strcmp (ligma_object_get_name (object), name))
        return object;
    }

  return NULL;
}

static LigmaObject *
ligma_list_get_child_by_index (LigmaContainer *container,
                              gint           index)
{
  LigmaList *list = LIGMA_LIST (container);

  return g_queue_peek_nth (list->queue, index);
}

static gint
ligma_list_get_child_index (LigmaContainer *container,
                           LigmaObject    *object)
{
  LigmaList *list = LIGMA_LIST (container);

  return g_queue_index (list->queue, (gpointer) object);
}

/**
 * ligma_list_new:
 * @children_type: the #GType of objects the list is going to hold
 * @unique_names:  if the list should ensure that all its children
 *                 have unique names.
 *
 * Creates a new #LigmaList object. Since #LigmaList is a #LigmaContainer
 * implementation, it holds LigmaObjects. Thus @children_type must be
 * LIGMA_TYPE_OBJECT or a type derived from it.
 *
 * The returned list has the #LIGMA_CONTAINER_POLICY_STRONG.
 *
 * Returns: a new #LigmaList object
 **/
LigmaContainer *
ligma_list_new (GType    children_type,
               gboolean unique_names)
{
  LigmaList *list;

  g_return_val_if_fail (g_type_is_a (children_type, LIGMA_TYPE_OBJECT), NULL);

  list = g_object_new (LIGMA_TYPE_LIST,
                       "children-type", children_type,
                       "policy",        LIGMA_CONTAINER_POLICY_STRONG,
                       "unique-names",  unique_names ? TRUE : FALSE,
                       NULL);

  /* for debugging purposes only */
  ligma_object_set_static_name (LIGMA_OBJECT (list), g_type_name (children_type));

  return LIGMA_CONTAINER (list);
}

/**
 * ligma_list_new_weak:
 * @children_type: the #GType of objects the list is going to hold
 * @unique_names:  if the list should ensure that all its children
 *                 have unique names.
 *
 * Creates a new #LigmaList object. Since #LigmaList is a #LigmaContainer
 * implementation, it holds LigmaObjects. Thus @children_type must be
 * LIGMA_TYPE_OBJECT or a type derived from it.
 *
 * The returned list has the #LIGMA_CONTAINER_POLICY_WEAK.
 *
 * Returns: a new #LigmaList object
 **/
LigmaContainer *
ligma_list_new_weak (GType    children_type,
                    gboolean unique_names)
{
  LigmaList *list;

  g_return_val_if_fail (g_type_is_a (children_type, LIGMA_TYPE_OBJECT), NULL);

  list = g_object_new (LIGMA_TYPE_LIST,
                       "children-type", children_type,
                       "policy",        LIGMA_CONTAINER_POLICY_WEAK,
                       "unique-names",  unique_names ? TRUE : FALSE,
                       NULL);

  /* for debugging purposes only */
  ligma_object_set_static_name (LIGMA_OBJECT (list), g_type_name (children_type));

  return LIGMA_CONTAINER (list);
}

/**
 * ligma_list_reverse:
 * @list: a #LigmaList
 *
 * Reverses the order of elements in a #LigmaList.
 **/
void
ligma_list_reverse (LigmaList *list)
{
  g_return_if_fail (LIGMA_IS_LIST (list));

  if (ligma_container_get_n_children (LIGMA_CONTAINER (list)) > 1)
    {
      ligma_container_freeze (LIGMA_CONTAINER (list));
      g_queue_reverse (list->queue);
      ligma_container_thaw (LIGMA_CONTAINER (list));
    }
}

/**
 * ligma_list_set_sort_func:
 * @list:      a #LigmaList
 * @sort_func: a #GCompareFunc
 *
 * Sorts the elements of @list using ligma_list_sort() and remembers the
 * passed @sort_func in order to keep the list ordered across inserting
 * or renaming children.
 **/
void
ligma_list_set_sort_func (LigmaList     *list,
                         GCompareFunc  sort_func)
{
  g_return_if_fail (LIGMA_IS_LIST (list));

  if (sort_func != list->sort_func)
    {
      if (sort_func)
        ligma_list_sort (list, sort_func);

      list->sort_func = sort_func;
      g_object_notify (G_OBJECT (list), "sort-func");
    }
}

/**
 * ligma_list_get_sort_func:
 * @list: a #LigmaList
 *
 * Returns the @list's sort function, see ligma_list_set_sort_func().
 *
 * Returns: The @list's sort function.
 **/
GCompareFunc
ligma_list_get_sort_func (LigmaList *list)
{
  g_return_val_if_fail (LIGMA_IS_LIST (list), NULL);

  return list->sort_func;
}

/**
 * ligma_list_sort:
 * @list: a #LigmaList
 * @sort_func: a #GCompareFunc
 *
 * Sorts the elements of a #LigmaList according to the given @sort_func.
 * See g_list_sort() for a detailed description of this function.
 **/
void
ligma_list_sort (LigmaList     *list,
                GCompareFunc  sort_func)
{
  g_return_if_fail (LIGMA_IS_LIST (list));
  g_return_if_fail (sort_func != NULL);

  if (ligma_container_get_n_children (LIGMA_CONTAINER (list)) > 1)
    {
      ligma_container_freeze (LIGMA_CONTAINER (list));
      g_queue_sort (list->queue, ligma_list_sort_func, sort_func);
      ligma_container_thaw (LIGMA_CONTAINER (list));
    }
}

/**
 * ligma_list_sort_by_name:
 * @list: a #LigmaList
 *
 * Sorts the #LigmaObject elements of a #LigmaList by their names.
 **/
void
ligma_list_sort_by_name (LigmaList *list)
{
  g_return_if_fail (LIGMA_IS_LIST (list));

  ligma_list_sort (list, (GCompareFunc) ligma_object_name_collate);
}


/*  private functions  */

static void
ligma_list_uniquefy_name (LigmaList   *ligma_list,
                         LigmaObject *object)
{
  gchar *name = (gchar *) ligma_object_get_name (object);
  GList *list;

  if (! name)
    return;

  for (list = ligma_list->queue->head; list; list = g_list_next (list))
    {
      LigmaObject  *object2 = list->data;
      const gchar *name2   = ligma_object_get_name (object2);

      if (object != object2 &&
          name2             &&
          ! strcmp (name, name2))
        break;
    }

  if (list)
    {
      gchar *ext;
      gchar *new_name   = NULL;
      gint   unique_ext = 0;

      name = g_strdup (name);

      ext = strrchr (name, '#');

      if (ext)
        {
          gchar ext_str[8];

          unique_ext = atoi (ext + 1);

          g_snprintf (ext_str, sizeof (ext_str), "%d", unique_ext);

          /*  check if the extension really is of the form "#<n>"  */
          if (! strcmp (ext_str, ext + 1))
            {
              if (ext > name && *(ext - 1) == ' ')
                ext--;

              *ext = '\0';
            }
          else
            {
              unique_ext = 0;
            }
        }

      do
        {
          unique_ext++;

          g_free (new_name);

          new_name = g_strdup_printf ("%s #%d", name, unique_ext);

          for (list = ligma_list->queue->head; list; list = g_list_next (list))
            {
              LigmaObject  *object2 = list->data;
              const gchar *name2   = ligma_object_get_name (object2);

              if (object != object2 &&
                  name2             &&
                  ! strcmp (new_name, name2))
                break;
            }
        }
      while (list);

      g_free (name);

      ligma_object_take_name (object, new_name);
    }
}

static void
ligma_list_object_renamed (LigmaObject *object,
                          LigmaList   *list)
{
  if (list->unique_names)
    {
      g_signal_handlers_block_by_func (object,
                                       ligma_list_object_renamed,
                                       list);

      ligma_list_uniquefy_name (list, object);

      g_signal_handlers_unblock_by_func (object,
                                         ligma_list_object_renamed,
                                         list);
    }

  if (list->sort_func)
    {
      GList *glist;
      gint   old_index;
      gint   new_index = 0;

      old_index = g_queue_index (list->queue, object);

      for (glist = list->queue->head; glist; glist = g_list_next (glist))
        {
          LigmaObject *object2 = LIGMA_OBJECT (glist->data);

          if (object == object2)
            continue;

          if (list->sort_func (object, object2) > 0)
            new_index++;
          else
            break;
        }

      if (new_index != old_index)
        ligma_container_reorder (LIGMA_CONTAINER (list), object, new_index);
    }
}
