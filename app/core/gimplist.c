/* The GIMP -- an image manipulation program
 * Copyright (C) 1995-1997 Spencer Kimball and Peter Mattis
 *
 * gimplist.c
 * Copyright (C) 2001 Michael Natterer <mitch@gimp.org>
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

#include "gimplist.h"


static void         gimp_list_class_init         (GimpListClass       *klass);
static void         gimp_list_init               (GimpList            *list);

static void         gimp_list_dispose            (GObject             *object);

static gsize        gimp_list_get_memsize        (GimpObject          *object,
                                                  gsize               *gui_size);

static void         gimp_list_add                (GimpContainer       *container,
						  GimpObject          *object);
static void         gimp_list_remove             (GimpContainer       *container,
						  GimpObject          *object);
static void         gimp_list_reorder            (GimpContainer       *container,
						  GimpObject          *object,
						  gint                 new_index);
static void         gimp_list_clear              (GimpContainer       *container);
static gboolean     gimp_list_have               (const GimpContainer *container,
						  const GimpObject    *object);
static void         gimp_list_foreach            (const GimpContainer *container,
						  GFunc                func,
						  gpointer             user_data);
static GimpObject * gimp_list_get_child_by_name  (const GimpContainer *container,
						  const gchar         *name);
static GimpObject * gimp_list_get_child_by_index (const GimpContainer *container,
						  gint                 index);
static gint         gimp_list_get_child_index    (const GimpContainer *container,
						  const GimpObject    *object);


static GimpContainerClass *parent_class = NULL;


GType
gimp_list_get_type (void)
{
  static GType list_type = 0;

  if (! list_type)
    {
      static const GTypeInfo list_info =
      {
        sizeof (GimpListClass),
	(GBaseInitFunc) NULL,
	(GBaseFinalizeFunc) NULL,
	(GClassInitFunc) gimp_list_class_init,
	NULL,		/* class_finalize */
	NULL,		/* class_data     */
	sizeof (GimpList),
	0,              /* n_preallocs    */
	(GInstanceInitFunc) gimp_list_init,
      };

      list_type = g_type_register_static (GIMP_TYPE_CONTAINER,
					  "GimpList",
					  &list_info, 0);
    }

  return list_type;
}

static void
gimp_list_class_init (GimpListClass *klass)
{
  GObjectClass       *object_class;
  GimpObjectClass    *gimp_object_class;
  GimpContainerClass *container_class;

  object_class      = G_OBJECT_CLASS (klass);
  gimp_object_class = GIMP_OBJECT_CLASS (klass);
  container_class   = GIMP_CONTAINER_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);

  object_class->dispose               = gimp_list_dispose;

  gimp_object_class->get_memsize      = gimp_list_get_memsize;

  container_class->add                = gimp_list_add;
  container_class->remove             = gimp_list_remove;
  container_class->reorder            = gimp_list_reorder;
  container_class->clear              = gimp_list_clear;
  container_class->have               = gimp_list_have;
  container_class->foreach            = gimp_list_foreach;
  container_class->get_child_by_name  = gimp_list_get_child_by_name;
  container_class->get_child_by_index = gimp_list_get_child_by_index;
  container_class->get_child_index    = gimp_list_get_child_index;
}

static void
gimp_list_init (GimpList *list)
{
  list->list = NULL;
}

static void
gimp_list_dispose (GObject *object)
{
  GimpList *list;

  list = GIMP_LIST (object);

  while (list->list)
    gimp_container_remove (GIMP_CONTAINER (list), list->list->data);

  G_OBJECT_CLASS (parent_class)->dispose (object);
}

static gsize
gimp_list_get_memsize (GimpObject *object,
                       gsize      *gui_size)
{
  GimpList *gimp_list;
  gsize     memsize = 0;

  gimp_list = GIMP_LIST (object);

  memsize += (gimp_container_num_children (GIMP_CONTAINER (gimp_list)) *
              sizeof (GList));

  if (gimp_container_policy (GIMP_CONTAINER (gimp_list)) ==
      GIMP_CONTAINER_POLICY_STRONG)
    {
      GList *list;

      for (list = gimp_list->list; list; list = g_list_next (list))
        memsize += gimp_object_get_memsize (GIMP_OBJECT (list->data), gui_size);
    }

  return memsize + GIMP_OBJECT_CLASS (parent_class)->get_memsize (object,
                                                                  gui_size);
}

static void
gimp_list_add (GimpContainer *container,
	       GimpObject    *object)
{
  GimpList *list;

  list = GIMP_LIST (container);

  list->list = g_list_prepend (list->list, object);
}

static void
gimp_list_remove (GimpContainer *container,
		  GimpObject    *object)
{
  GimpList *list;

  list = GIMP_LIST (container);

  list->list = g_list_remove (list->list, object);
}

static void
gimp_list_reorder (GimpContainer *container,
		   GimpObject    *object,
		   gint           new_index)
{
  GimpList *list;

  list = GIMP_LIST (container);

  list->list = g_list_remove (list->list, object);

  if (new_index == -1 || new_index == container->num_children - 1)
    list->list = g_list_append (list->list, object);
  else
    list->list = g_list_insert (list->list, object, new_index);
}

static void
gimp_list_clear (GimpContainer *container)
{
  GimpList *list;

  list = GIMP_LIST (container);

  while (list->list)
    gimp_container_remove (container, list->list->data);
}

static gboolean
gimp_list_have (const GimpContainer *container,
		const GimpObject    *object)
{
  GimpList *list;

  list = GIMP_LIST (container);

  return g_list_find (list->list, object) ? TRUE : FALSE;
}

static void
gimp_list_foreach (const GimpContainer *container,
		   GFunc                func,
		   gpointer             user_data)
{
  GimpList *list;

  list = GIMP_LIST (container);

  g_list_foreach (list->list, func, user_data);
}

static GimpObject *
gimp_list_get_child_by_name (const GimpContainer *container,
			     const gchar         *name)
{
  GimpList   *list;
  GimpObject *object;
  GList      *glist;

  list = GIMP_LIST (container);

  for (glist = list->list; glist; glist = g_list_next (glist))
    {
      object = (GimpObject *) glist->data;

      if (! strcmp (object->name, name))
	return object;
    }

  return NULL;
}

static GimpObject *
gimp_list_get_child_by_index (const GimpContainer *container,
			      gint                 index)
{
  GimpList *list;
  GList    *glist;

  list = GIMP_LIST (container);

  glist = g_list_nth (list->list, index);

  if (glist)
    return (GimpObject *) glist->data;

  return NULL;
}

static gint
gimp_list_get_child_index (const GimpContainer *container,
			   const GimpObject    *object)
{
  GimpList *list;

  list = GIMP_LIST (container);

  return g_list_index (list->list, (gpointer) object);
}

/**
 * gimp_list_new:
 * @children_type: the #GType of objects the list is going to hold
 * @policy: the #GimpContainerPolicy for the new list
 *
 * Creates a new #GimpList object. Since #GimpList is a #GimpContainer
 * implementation, it holds GimpObjects. Thus @children_type must be
 * GIMP_TYPE_OBJECT or a type derived from it.
 *
 * Return value: a new #GimpList object
 **/
GimpContainer *
gimp_list_new (GType                children_type,
	       GimpContainerPolicy  policy)
{
  GimpList *list;

  g_return_val_if_fail (g_type_is_a (children_type, GIMP_TYPE_OBJECT), NULL);
  g_return_val_if_fail (policy == GIMP_CONTAINER_POLICY_STRONG ||
                        policy == GIMP_CONTAINER_POLICY_WEAK, NULL);

  list = g_object_new (GIMP_TYPE_LIST,
                       "children_type", children_type,
                       "policy",        policy,
                       NULL);

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

  if (GIMP_CONTAINER (list)->num_children > 1)
    {
      gimp_container_freeze (GIMP_CONTAINER (list));
      list->list = g_list_reverse (list->list);
      gimp_container_thaw (GIMP_CONTAINER (list));
    }
}

/**
 * gimp_list_sort:
 * @list: a #GimpList
 * @compare_func: a #GCompareFunc
 *
 * Sorts the elements of a #GimpList according to the given @compare_func.
 * See g_list_sort() for a detailed description of this function.
 **/
void
gimp_list_sort (GimpList     *list,
                GCompareFunc  compare_func)
{
  g_return_if_fail (GIMP_IS_LIST (list));
  g_return_if_fail (compare_func != NULL);

  if (GIMP_CONTAINER (list)->num_children > 1)
    {
      gimp_container_freeze (GIMP_CONTAINER (list));
      list->list = g_list_sort (list->list, compare_func);
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

/**
 * gimp_list_uniquefy_name:
 * @gimp_list: a #GimpList
 * @object:    a #GimpObject
 * @notify:    whether to notify listeners about the name change
 *
 * This function ensures that @object has a name that isn't already
 * used by another object in @gimp_list. If the name of @object needs
 * to be changed, the value of @notify decides if the "name_changed"
 * signal should be emitted or if the name should be changed silently.
 * The latter might be useful under certain circumstances in order to
 * avoid recursion.
 **/
void
gimp_list_uniquefy_name (GimpList   *gimp_list,
                         GimpObject *object,
                         gboolean    notify)
{
  GList *list;
  GList *list2;
  gint   unique_ext = 0;
  gchar *new_name   = NULL;
  gchar *ext;

  g_return_if_fail (GIMP_IS_LIST (gimp_list));
  g_return_if_fail (GIMP_IS_OBJECT (object));

  for (list = gimp_list->list; list; list = g_list_next (list))
    {
      GimpObject *object2 = GIMP_OBJECT (list->data);

      if (object != object2 &&
	  strcmp (gimp_object_get_name (GIMP_OBJECT (object)),
		  gimp_object_get_name (GIMP_OBJECT (object2))) == 0)
	{
          ext = strrchr (object->name, '#');

          if (ext)
            {
              gchar *ext_str;

              unique_ext = atoi (ext + 1);

              ext_str = g_strdup_printf ("%d", unique_ext);

              /*  check if the extension really is of the form "#<n>"  */
              if (! strcmp (ext_str, ext + 1))
                {
                  *ext = '\0';
                }
              else
                {
                  unique_ext = 0;
                }

              g_free (ext_str);
            }
          else
            {
              unique_ext = 0;
            }

          do
            {
              unique_ext++;

              g_free (new_name);

              new_name = g_strdup_printf ("%s#%d", object->name, unique_ext);

              for (list2 = gimp_list->list; list2; list2 = g_list_next (list2))
                {
                  object2 = GIMP_OBJECT (list2->data);

                  if (object == object2)
                    continue;

                  if (! strcmp (object2->name, new_name))
		    break;
                }
            }
          while (list2);

          if (notify)
            {
              gimp_object_set_name (object, new_name);
              g_free (new_name);
            }
          else
            {
              gimp_object_name_free (object);
              object->name = new_name;
            }

	  break;
	}
    }
}
