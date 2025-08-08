/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995-1997 Spencer Kimball and Peter Mattis
 *
 * gimplist.c
 * Copyright (C) 2001-2016 Michael Natterer <mitch@gimp.org>
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

#include "libgimpbase/gimpbase.h"

#include "core-types.h"

#include "gimp-memsize.h"
#include "gimplist.h"


enum
{
  PROP_0,
  PROP_UNIQUE_NAMES,
  PROP_SORT_FUNC,
  PROP_APPEND
};


static void         gimp_list_finalize           (GObject                 *object);
static void         gimp_list_set_property       (GObject                 *object,
                                                  guint                    property_id,
                                                  const GValue            *value,
                                                  GParamSpec              *pspec);
static void         gimp_list_get_property       (GObject                 *object,
                                                  guint                    property_id,
                                                  GValue                  *value,
                                                  GParamSpec              *pspec);

static gint64       gimp_list_get_memsize        (GimpObject              *object,
                                                  gint64                  *gui_size);

static void         gimp_list_add                (GimpContainer           *container,
                                                  GimpObject              *object);
static void         gimp_list_remove             (GimpContainer           *container,
                                                  GimpObject              *object);
static void         gimp_list_reorder            (GimpContainer           *container,
                                                  GimpObject              *object,
                                                  gint                     old_index,
                                                  gint                     new_index);
static void         gimp_list_clear              (GimpContainer           *container);
static gboolean     gimp_list_have               (GimpContainer           *container,
                                                  GimpObject              *object);
static void         gimp_list_foreach            (GimpContainer           *container,
                                                  GFunc                    func,
                                                  gpointer                 user_data);
static GimpObject * gimp_list_search             (GimpContainer           *container,
                                                  GimpContainerSearchFunc  func,
                                                  gpointer                 user_data);
static gboolean     gimp_list_get_unique_names   (GimpContainer           *container);
static GList    * gimp_list_get_children_by_name (GimpContainer           *container,
                                                  const gchar             *name);
static GimpObject * gimp_list_get_child_by_name  (GimpContainer           *container,
                                                  const gchar             *name);
static GimpObject * gimp_list_get_child_by_index (GimpContainer           *container,
                                                  gint                     index);
static gint         gimp_list_get_child_index    (GimpContainer           *container,
                                                  GimpObject              *object);

static void         gimp_list_uniquefy_name      (GimpList                *gimp_list,
                                                  GimpObject              *object);
static void         gimp_list_object_renamed     (GimpObject              *object,
                                                  GimpList                *list);


G_DEFINE_TYPE (GimpList, gimp_list, GIMP_TYPE_CONTAINER)

#define parent_class gimp_list_parent_class


static void
gimp_list_class_init (GimpListClass *klass)
{
  GObjectClass       *object_class      = G_OBJECT_CLASS (klass);
  GimpObjectClass    *gimp_object_class = GIMP_OBJECT_CLASS (klass);
  GimpContainerClass *container_class   = GIMP_CONTAINER_CLASS (klass);

  object_class->finalize                = gimp_list_finalize;
  object_class->set_property            = gimp_list_set_property;
  object_class->get_property            = gimp_list_get_property;

  gimp_object_class->get_memsize        = gimp_list_get_memsize;

  container_class->add                  = gimp_list_add;
  container_class->remove               = gimp_list_remove;
  container_class->reorder              = gimp_list_reorder;
  container_class->clear                = gimp_list_clear;
  container_class->have                 = gimp_list_have;
  container_class->foreach              = gimp_list_foreach;
  container_class->search               = gimp_list_search;
  container_class->get_unique_names     = gimp_list_get_unique_names;
  container_class->get_children_by_name = gimp_list_get_children_by_name;
  container_class->get_child_by_name    = gimp_list_get_child_by_name;
  container_class->get_child_by_index   = gimp_list_get_child_by_index;
  container_class->get_child_index      = gimp_list_get_child_index;

  g_object_class_install_property (object_class, PROP_UNIQUE_NAMES,
                                   g_param_spec_boolean ("unique-names",
                                                         NULL, NULL,
                                                         FALSE,
                                                         GIMP_PARAM_READWRITE |
                                                         G_PARAM_CONSTRUCT_ONLY));

  g_object_class_install_property (object_class, PROP_SORT_FUNC,
                                   g_param_spec_pointer ("sort-func",
                                                         NULL, NULL,
                                                         GIMP_PARAM_READWRITE |
                                                         G_PARAM_CONSTRUCT));

  g_object_class_install_property (object_class, PROP_APPEND,
                                   g_param_spec_boolean ("append",
                                                         NULL, NULL,
                                                         FALSE,
                                                         GIMP_PARAM_READWRITE |
                                                         G_PARAM_CONSTRUCT));
}

static void
gimp_list_init (GimpList *list)
{
  list->queue        = g_queue_new ();
  list->unique_names = FALSE;
  list->sort_func    = NULL;
  list->append       = FALSE;
}

static void
gimp_list_finalize (GObject *object)
{
  GimpList *list = GIMP_LIST (object);

  if (list->queue)
    {
      g_queue_free (list->queue);
      list->queue = NULL;
    }

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
gimp_list_set_property (GObject      *object,
                        guint         property_id,
                        const GValue *value,
                        GParamSpec   *pspec)
{
  GimpList *list = GIMP_LIST (object);

  switch (property_id)
    {
    case PROP_UNIQUE_NAMES:
      list->unique_names = g_value_get_boolean (value);
      break;
    case PROP_SORT_FUNC:
      gimp_list_set_sort_func (list, g_value_get_pointer (value));
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
gimp_list_get_property (GObject    *object,
                        guint       property_id,
                        GValue     *value,
                        GParamSpec *pspec)
{
  GimpList *list = GIMP_LIST (object);

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
gimp_list_get_memsize (GimpObject *object,
                       gint64     *gui_size)
{
  GimpList *list    = GIMP_LIST (object);
  gint64    memsize = 0;

  if (gimp_container_get_policy (GIMP_CONTAINER (list)) ==
      GIMP_CONTAINER_POLICY_STRONG)
    {
      memsize += gimp_g_queue_get_memsize_foreach (list->queue,
                                                   (GimpMemsizeFunc) gimp_object_get_memsize,
                                                   gui_size);
    }
  else
    {
      memsize += gimp_g_queue_get_memsize (list->queue, 0);
    }

  return memsize + GIMP_OBJECT_CLASS (parent_class)->get_memsize (object,
                                                                  gui_size);
}

static gint
gimp_list_sort_func (gconstpointer a,
                     gconstpointer b,
                     gpointer      user_data)
{
  GCompareFunc func = user_data;

  return func (a, b);
}

static void
gimp_list_add (GimpContainer *container,
               GimpObject    *object)
{
  GimpList *list = GIMP_LIST (container);

  if (list->unique_names)
    gimp_list_uniquefy_name (list, object);

  if (list->unique_names || list->sort_func)
    g_signal_connect (object, "name-changed",
                      G_CALLBACK (gimp_list_object_renamed),
                      list);

  if (list->sort_func)
    {
      g_queue_insert_sorted (list->queue, object, gimp_list_sort_func,
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

  GIMP_CONTAINER_CLASS (parent_class)->add (container, object);
}

static void
gimp_list_remove (GimpContainer *container,
                  GimpObject    *object)
{
  GimpList *list = GIMP_LIST (container);

  if (list->unique_names || list->sort_func)
    g_signal_handlers_disconnect_by_func (object,
                                          gimp_list_object_renamed,
                                          list);

  g_queue_remove (list->queue, object);

  GIMP_CONTAINER_CLASS (parent_class)->remove (container, object);
}

static void
gimp_list_reorder (GimpContainer *container,
                   GimpObject    *object,
                   gint           old_index,
                   gint           new_index)
{
  GimpList *list = GIMP_LIST (container);

  g_queue_remove (list->queue, object);

  if (new_index == gimp_container_get_n_children (container) - 1)
    g_queue_push_tail (list->queue, object);
  else
    g_queue_push_nth (list->queue, object, new_index);

  GIMP_CONTAINER_CLASS (parent_class)->reorder (container, object,
                                                old_index, new_index);
}

static void
gimp_list_clear (GimpContainer *container)
{
  GimpList *list = GIMP_LIST (container);

  while (g_queue_peek_head (list->queue))
    gimp_container_remove (container, g_queue_peek_head (list->queue));
}

static gboolean
gimp_list_have (GimpContainer *container,
                GimpObject    *object)
{
  GimpList *list = GIMP_LIST (container);

  return g_queue_find (list->queue, object) ? TRUE : FALSE;
}

static void
gimp_list_foreach (GimpContainer *container,
                   GFunc          func,
                   gpointer       user_data)
{
  GimpList *list = GIMP_LIST (container);

  g_queue_foreach (list->queue, func, user_data);
}

static GimpObject *
gimp_list_search (GimpContainer           *container,
                  GimpContainerSearchFunc  func,
                  gpointer                 user_data)
{
  GimpList *list = GIMP_LIST (container);
  GList    *iter;

  iter = list->queue->head;

  while (iter)
    {
      GimpObject *object = iter->data;

      iter = g_list_next (iter);

      if (func (object, user_data))
        return object;
    }

  return NULL;
}

static gboolean
gimp_list_get_unique_names (GimpContainer *container)
{
  GimpList *list = GIMP_LIST (container);

  return list->unique_names;
}

static GList *
gimp_list_get_children_by_name (GimpContainer *container,
                                const gchar   *name)
{
  GimpList *list     = GIMP_LIST (container);
  GList    *children = NULL;
  GList    *iter;

  for (iter = list->queue->head; iter; iter = g_list_next (iter))
    {
      GimpObject *object = iter->data;

      if (! strcmp (gimp_object_get_name (object), name))
        {
          children = g_list_prepend (children, object);
          if (list->unique_names)
            return children;
        }
    }

  return children;
}

static GimpObject *
gimp_list_get_child_by_name (GimpContainer *container,
                             const gchar   *name)
{
  GimpList *list = GIMP_LIST (container);
  GList    *glist;

  for (glist = list->queue->head; glist; glist = g_list_next (glist))
    {
      GimpObject *object = glist->data;

      if (! strcmp (gimp_object_get_name (object), name))
        return object;
    }

  return NULL;
}

static GimpObject *
gimp_list_get_child_by_index (GimpContainer *container,
                              gint           index)
{
  GimpList *list = GIMP_LIST (container);

  return g_queue_peek_nth (list->queue, index);
}

static gint
gimp_list_get_child_index (GimpContainer *container,
                           GimpObject    *object)
{
  GimpList *list = GIMP_LIST (container);

  return g_queue_index (list->queue, (gpointer) object);
}

/**
 * gimp_list_new:
 * @child_type:   the #GType of objects the list is going to hold
 * @unique_names: if the list should ensure that all its children
 *                have unique names.
 *
 * Creates a new #GimpList object. Since #GimpList is a #GimpContainer
 * implementation, it holds GimpObjects. Thus @child_type must be
 * GIMP_TYPE_OBJECT or a type derived from it.
 *
 * The returned list has the #GIMP_CONTAINER_POLICY_STRONG.
 *
 * Returns: a new #GimpList object
 **/
GimpContainer *
gimp_list_new (GType    child_type,
               gboolean unique_names)
{
  GimpList *list;

  g_return_val_if_fail (g_type_is_a (child_type, GIMP_TYPE_OBJECT), NULL);

  list = g_object_new (GIMP_TYPE_LIST,
                       "child-type",   child_type,
                       "policy",       GIMP_CONTAINER_POLICY_STRONG,
                       "unique-names", unique_names ? TRUE : FALSE,
                       NULL);

  /* for debugging purposes only */
  gimp_object_set_static_name (GIMP_OBJECT (list), g_type_name (child_type));

  return GIMP_CONTAINER (list);
}

/**
 * gimp_list_new_weak:
 * @child_type:   the #GType of objects the list is going to hold
 * @unique_names: if the list should ensure that all its children
 *                have unique names.
 *
 * Creates a new #GimpList object. Since #GimpList is a #GimpContainer
 * implementation, it holds GimpObjects. Thus @child_type must be
 * GIMP_TYPE_OBJECT or a type derived from it.
 *
 * The returned list has the #GIMP_CONTAINER_POLICY_WEAK.
 *
 * Returns: a new #GimpList object
 **/
GimpContainer *
gimp_list_new_weak (GType    child_type,
                    gboolean unique_names)
{
  GimpList *list;

  g_return_val_if_fail (g_type_is_a (child_type, GIMP_TYPE_OBJECT), NULL);

  list = g_object_new (GIMP_TYPE_LIST,
                       "child-type",   child_type,
                       "policy",       GIMP_CONTAINER_POLICY_WEAK,
                       "unique-names", unique_names ? TRUE : FALSE,
                       NULL);

  /* for debugging purposes only */
  gimp_object_set_static_name (GIMP_OBJECT (list), g_type_name (child_type));

  return GIMP_CONTAINER (list);
}

/**
 * gimp_list_reverse:
 * @list: a #GimpList
 *
 * Reverses the order of elements in a #GimpList.
 **/
void
gimp_list_reverse (GimpList *list)
{
  g_return_if_fail (GIMP_IS_LIST (list));

  if (gimp_container_get_n_children (GIMP_CONTAINER (list)) > 1)
    {
      gimp_container_freeze (GIMP_CONTAINER (list));
      g_queue_reverse (list->queue);
      gimp_container_thaw (GIMP_CONTAINER (list));
    }
}

/**
 * gimp_list_set_sort_func:
 * @list:      a #GimpList
 * @sort_func: a #GCompareFunc
 *
 * Sorts the elements of @list using gimp_list_sort() and remembers the
 * passed @sort_func in order to keep the list ordered across inserting
 * or renaming children.
 **/
void
gimp_list_set_sort_func (GimpList     *list,
                         GCompareFunc  sort_func)
{
  g_return_if_fail (GIMP_IS_LIST (list));

  if (sort_func != list->sort_func)
    {
      if (sort_func)
        gimp_list_sort (list, sort_func);

      list->sort_func = sort_func;
      g_object_notify (G_OBJECT (list), "sort-func");
    }
}

/**
 * gimp_list_get_sort_func:
 * @list: a #GimpList
 *
 * Returns the @list's sort function, see gimp_list_set_sort_func().
 *
 * Returns: The @list's sort function.
 **/
GCompareFunc
gimp_list_get_sort_func (GimpList *list)
{
  g_return_val_if_fail (GIMP_IS_LIST (list), NULL);

  return list->sort_func;
}

/**
 * gimp_list_sort:
 * @list: a #GimpList
 * @sort_func: a #GCompareFunc
 *
 * Sorts the elements of a #GimpList according to the given @sort_func.
 * See g_list_sort() for a detailed description of this function.
 **/
void
gimp_list_sort (GimpList     *list,
                GCompareFunc  sort_func)
{
  g_return_if_fail (GIMP_IS_LIST (list));
  g_return_if_fail (sort_func != NULL);

  if (gimp_container_get_n_children (GIMP_CONTAINER (list)) > 1)
    {
      gimp_container_freeze (GIMP_CONTAINER (list));
      g_queue_sort (list->queue, gimp_list_sort_func, sort_func);
      gimp_container_thaw (GIMP_CONTAINER (list));
    }
}

/**
 * gimp_list_sort_by_name:
 * @list: a #GimpList
 *
 * Sorts the #GimpObject elements of a #GimpList by their names.
 **/
void
gimp_list_sort_by_name (GimpList *list)
{
  g_return_if_fail (GIMP_IS_LIST (list));

  gimp_list_sort (list, (GCompareFunc) gimp_object_name_collate);
}


/*  private functions  */

static void
gimp_list_uniquefy_name (GimpList   *gimp_list,
                         GimpObject *object)
{
  gchar *name = (gchar *) gimp_object_get_name (object);
  GList *list;

  if (! name)
    return;

  for (list = gimp_list->queue->head; list; list = g_list_next (list))
    {
      GimpObject  *object2 = list->data;
      const gchar *name2   = gimp_object_get_name (object2);

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

          for (list = gimp_list->queue->head; list; list = g_list_next (list))
            {
              GimpObject  *object2 = list->data;
              const gchar *name2   = gimp_object_get_name (object2);

              if (object != object2 &&
                  name2             &&
                  ! strcmp (new_name, name2))
                break;
            }
        }
      while (list);

      g_free (name);

      gimp_object_take_name (object, new_name);
    }
}

static void
gimp_list_object_renamed (GimpObject *object,
                          GimpList   *list)
{
  if (list->unique_names)
    {
      g_signal_handlers_block_by_func (object,
                                       gimp_list_object_renamed,
                                       list);

      gimp_list_uniquefy_name (list, object);

      g_signal_handlers_unblock_by_func (object,
                                         gimp_list_object_renamed,
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
          GimpObject *object2 = GIMP_OBJECT (glist->data);

          if (object == object2)
            continue;

          if (list->sort_func (object, object2) > 0)
            new_index++;
          else
            break;
        }

      if (new_index != old_index)
        gimp_container_reorder (GIMP_CONTAINER (list), object, new_index);
    }
}
