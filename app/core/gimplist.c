/* The GIMP -- an image manipulation program
 * Copyright (C) 1995-1997 Spencer Kimball and Peter Mattis
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

#include <gtk/gtk.h>

#include "apptypes.h"

#include "gimpcontainer.h"
#include "gimplist.h"


static void       gimp_list_class_init (GimpListClass *klass);
static void       gimp_list_init       (GimpList      *list);
static void       gimp_list_destroy    (GtkObject     *object);
static void       gimp_list_add        (GimpContainer *container,
				        GimpObject    *object);
static void       gimp_list_remove     (GimpContainer *container,
				        GimpObject    *object);
static gboolean   gimp_list_have       (GimpContainer *container,
				        GimpObject    *object);
static void       gimp_list_foreach    (GimpContainer *container,
				        GFunc          func,
				        gpointer       user_data);


static GimpContainerClass *parent_class = NULL;


GtkType
gimp_list_get_type (void)
{
  static GtkType list_type = 0;

  if (! list_type)
    {
      GtkTypeInfo list_info =
      {
	"GimpList",
	sizeof (GimpList),
	sizeof (GimpListClass),
	(GtkClassInitFunc) gimp_list_class_init,
	(GtkObjectInitFunc) gimp_list_init,
	/* reserved_1 */ NULL,
	/* reserved_2 */ NULL,
	(GtkClassInitFunc) NULL
      };

      list_type = gtk_type_unique (GIMP_TYPE_CONTAINER, &list_info);
    }

  return list_type;
}

static void
gimp_list_class_init (GimpListClass *klass)
{
  GtkObjectClass     *object_class;
  GimpContainerClass *container_class;

  object_class    = (GtkObjectClass *) klass;
  container_class = (GimpContainerClass *) klass;

  parent_class = gtk_type_class (GIMP_TYPE_CONTAINER);

  object_class->destroy = gimp_list_destroy;

  container_class->add     = gimp_list_add;
  container_class->remove  = gimp_list_remove;
  container_class->have    = gimp_list_have;
  container_class->foreach = gimp_list_foreach;
}

static void
gimp_list_init (GimpList *list)
{
  list->list = NULL;
}

static void
gimp_list_destroy (GtkObject *object)
{
  GimpList *list;

  list = GIMP_LIST (object);

  while (list->list)
    {
      gimp_container_remove (GIMP_CONTAINER (list),
			     GIMP_OBJECT (list->list->data));
    }

  if (GTK_OBJECT_CLASS (parent_class)->destroy)
    GTK_OBJECT_CLASS (parent_class)->destroy (object);
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

static gboolean
gimp_list_have (GimpContainer *container,
		GimpObject    *object)
{
  GimpList *list;

  list = GIMP_LIST (container);

  return g_list_find (list->list, object) ? TRUE : FALSE;
}

static void
gimp_list_foreach (GimpContainer *container,
		   GFunc          func,
		   gpointer       user_data)
{
  GimpList *list;

  list = GIMP_LIST (container);

  g_list_foreach (list->list, func, user_data);
}

GimpList *
gimp_list_new (GtkType              children_type,
	       GimpContainerPolicy  policy)
{
  GimpList *list;

  g_return_val_if_fail (gtk_type_is_a (children_type, GIMP_TYPE_OBJECT), NULL);
  g_return_val_if_fail (policy == GIMP_CONTAINER_POLICY_STRONG ||
                        policy == GIMP_CONTAINER_POLICY_WEAK, NULL);

  list = gtk_type_new (GIMP_TYPE_LIST);

  GIMP_CONTAINER (list)->children_type = children_type;
  GIMP_CONTAINER (list)->policy        = policy;

  return list;
}

GimpObject *
gimp_list_get_child_by_name (const GimpList *list,
			     const gchar    *name)
{
  GimpObject *object;
  GList      *glist;

  g_return_val_if_fail (list != NULL, NULL);
  g_return_val_if_fail (GIMP_IS_LIST (list), NULL);
  g_return_val_if_fail (name != NULL, NULL);

  for (glist = list->list; glist; glist = g_list_next (glist))
    {
      object = (GimpObject *) glist->data;

      if (! strcmp (object->name, name))
	return object;
    }

  return NULL;
}

GimpObject *
gimp_list_get_child_by_index (const GimpList *list,
			      gint            index)
{
  GList *glist;

  g_return_val_if_fail (list != NULL, NULL);
  g_return_val_if_fail (GIMP_IS_LIST (list), NULL);

  glist = g_list_nth (list->list, index);

  if (glist)
    return (GimpObject *) glist->data;

  return NULL;
}

gint
gimp_list_get_child_index (const GimpList   *list,
			   const GimpObject *object)
{
  g_return_val_if_fail (list != NULL, 0);
  g_return_val_if_fail (GIMP_IS_LIST (list), 0);
  g_return_val_if_fail (object != NULL, 0);
  g_return_val_if_fail (GIMP_IS_OBJECT (list), 0);

  return g_list_index (list->list, (gpointer) object);
}
