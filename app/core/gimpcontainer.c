/* The GIMP -- an image manipulation program
 * Copyright (C) 1995-1997 Spencer Kimball and Peter Mattis
 *
 * gimpcontainer.c
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

#include <gtk/gtk.h>

#include "core-types.h"

#include "gimpcontainer.h"
#include "gimpmarshal.h"


typedef struct _GimpContainerHandler
{
  gchar     *signame;
  GCallback  callback;
  gpointer   callback_data;

  GQuark     quark;  /*  used to attach the signal id's of child signals  */
} GimpContainerHandler;


enum
{
  ADD,
  REMOVE,
  REORDER,
  HAVE,
  FOREACH,
  GET_CHILD_BY_NAME,
  GET_CHILD_BY_INDEX,
  GET_CHILD_INDEX,
  FREEZE,
  THAW,
  LAST_SIGNAL
};


/*  local function prototypes  */

static void   gimp_container_init                   (GimpContainer      *container);
static void   gimp_container_class_init             (GimpContainerClass *klass);
static void   gimp_container_destroy                (GtkObject          *object);
static void   gimp_container_child_destroy_callback (GtkObject          *object,
						     gpointer            data);


static guint   container_signals[LAST_SIGNAL] = { 0 };

static GimpObjectClass *parent_class = NULL;


GType
gimp_container_get_type (void)
{
  static GType container_type = 0;

  if (! container_type)
    {
      GtkTypeInfo container_info =
      {
        "GimpContainer",
        sizeof (GimpContainer),
        sizeof (GimpContainerClass),
        (GtkClassInitFunc) gimp_container_class_init,
        (GtkObjectInitFunc) gimp_container_init,
        /* reserved_1 */ NULL,
        /* reserved_2 */ NULL,
        (GtkClassInitFunc) NULL
      };

      container_type = gtk_type_unique (GIMP_TYPE_OBJECT, &container_info);
    }

  return container_type;
}

static void
gimp_container_init (GimpContainer *container)
{
  container->children_type = GIMP_TYPE_OBJECT;
  container->policy        = GIMP_CONTAINER_POLICY_STRONG;
  container->num_children  = 0;

  container->handlers      = NULL;
  container->freeze_count  = 0;
}

static void
gimp_container_class_init (GimpContainerClass* klass)
{
  GtkObjectClass *object_class;

  object_class = (GtkObjectClass *) klass;

  parent_class = g_type_class_peek_parent (klass);

  container_signals[ADD] =
    g_signal_new ("add",
		  G_TYPE_FROM_CLASS (klass),
		  G_SIGNAL_RUN_FIRST,
		  G_STRUCT_OFFSET (GimpContainerClass, add),
		  NULL, NULL,
		  g_cclosure_marshal_VOID__OBJECT,
		  G_TYPE_NONE, 1,
		  GIMP_TYPE_OBJECT);

  container_signals[REMOVE] =
    g_signal_new ("remove",
		  G_TYPE_FROM_CLASS (klass),
		  G_SIGNAL_RUN_FIRST,
		  G_STRUCT_OFFSET (GimpContainerClass, remove),
		  NULL, NULL,
		  g_cclosure_marshal_VOID__OBJECT,
		  G_TYPE_NONE, 1,
		  GIMP_TYPE_OBJECT);

  container_signals[REORDER] =
    g_signal_new ("reorder",
		  G_TYPE_FROM_CLASS (klass),
		  G_SIGNAL_RUN_FIRST,
		  G_STRUCT_OFFSET (GimpContainerClass, reorder),
		  NULL, NULL,
		  gimp_cclosure_marshal_VOID__OBJECT_INT,
		  G_TYPE_NONE, 2,
		  GIMP_TYPE_OBJECT,
		  G_TYPE_INT);

  container_signals[HAVE] =
    g_signal_new ("have",
		  G_TYPE_FROM_CLASS (klass),
		  G_SIGNAL_RUN_LAST,
		  G_STRUCT_OFFSET (GimpContainerClass, have),
		  NULL, NULL,
		  gimp_cclosure_marshal_BOOLEAN__OBJECT,
		  G_TYPE_BOOLEAN, 1,
		  GIMP_TYPE_OBJECT);

  container_signals[FOREACH] =
    g_signal_new ("foreach",
		  G_TYPE_FROM_CLASS (klass),
		  G_SIGNAL_RUN_FIRST,
		  G_STRUCT_OFFSET (GimpContainerClass, foreach),
		  NULL, NULL,
		  gimp_cclosure_marshal_VOID__POINTER_POINTER,
		  G_TYPE_NONE, 2,
		  G_TYPE_POINTER,
		  G_TYPE_POINTER);

  container_signals[GET_CHILD_BY_NAME] =
    g_signal_new ("get_child_by_name",
		  G_TYPE_FROM_CLASS (klass),
		  G_SIGNAL_RUN_LAST,
		  G_STRUCT_OFFSET (GimpContainerClass, get_child_by_name),
		  NULL, NULL,
		  gimp_cclosure_marshal_OBJECT__POINTER,
		  GIMP_TYPE_OBJECT, 1,
		  G_TYPE_POINTER);

  container_signals[GET_CHILD_BY_INDEX] =
    g_signal_new ("get_child_by_index",
		  G_TYPE_FROM_CLASS (klass),
		  G_SIGNAL_RUN_LAST,
		  G_STRUCT_OFFSET (GimpContainerClass, get_child_by_index),
		  NULL, NULL,
		  gimp_cclosure_marshal_OBJECT__INT,
		  GIMP_TYPE_OBJECT, 1,
		  G_TYPE_INT);

  container_signals[GET_CHILD_INDEX] =
    g_signal_new ("get_child_index",
		  G_TYPE_FROM_CLASS (klass),
		  G_SIGNAL_RUN_LAST,
		  G_STRUCT_OFFSET (GimpContainerClass, get_child_index),
		  NULL, NULL,
		  gimp_cclosure_marshal_INT__OBJECT,
		  G_TYPE_INT, 1,
		  GIMP_TYPE_OBJECT);

  container_signals[FREEZE] =
    g_signal_new ("freeze",
		  G_TYPE_FROM_CLASS (klass),
		  G_SIGNAL_RUN_LAST,
		  G_STRUCT_OFFSET (GimpContainerClass, freeze),
		  NULL, NULL,
		  g_cclosure_marshal_VOID__VOID,
		  G_TYPE_NONE, 0);

  container_signals[THAW] =
    g_signal_new ("thaw",
		  G_TYPE_FROM_CLASS (klass),
		  G_SIGNAL_RUN_LAST,
		  G_STRUCT_OFFSET (GimpContainerClass, thaw),
		  NULL, NULL,
		  g_cclosure_marshal_VOID__VOID,
		  G_TYPE_NONE, 0);

  object_class->destroy     = gimp_container_destroy;

  klass->add                = NULL;
  klass->remove             = NULL;
  klass->reorder            = NULL;
  klass->have               = NULL;
  klass->foreach            = NULL;
  klass->get_child_by_name  = NULL;
  klass->get_child_by_index = NULL;
  klass->get_child_index    = NULL;
  klass->freeze             = NULL;
  klass->thaw               = NULL;
}

static void
gimp_container_destroy (GtkObject *object)
{
  GimpContainer *container;

  container = GIMP_CONTAINER (object);

  while (container->handlers)
    {
      gimp_container_remove_handler
	(container,
	 ((GimpContainerHandler *) container->handlers->data)->quark);
    }

  if (GTK_OBJECT_CLASS (parent_class)->destroy)
    GTK_OBJECT_CLASS (parent_class)->destroy (object);
}

static void
gimp_container_child_destroy_callback (GtkObject *object,
				       gpointer   data)
{
  GimpContainer *container;

  container = GIMP_CONTAINER (data);

  gimp_container_remove (container, GIMP_OBJECT (object));
}

GType
gimp_container_children_type (const GimpContainer *container)
{
  return container->children_type;
}

GimpContainerPolicy
gimp_container_policy (const GimpContainer *container)
{
  return container->policy;
}

gint
gimp_container_num_children (const GimpContainer *container)
{
  return container->num_children;
}

gboolean
gimp_container_add (GimpContainer *container,
		    GimpObject    *object)
{
  GimpContainerHandler *handler;
  GList                *list;
  guint                 handler_id;

  g_return_val_if_fail (GIMP_IS_CONTAINER (container), FALSE);
  g_return_val_if_fail (object != NULL, FALSE);
  g_return_val_if_fail (g_type_instance_is_a ((GTypeInstance *) object,
					      container->children_type), FALSE);

  if (gimp_container_have (container, object))
    {
      g_warning ("%s(): container %p already contains object %p",
		 G_GNUC_FUNCTION, container, object);
      return FALSE;
    }

  for (list = container->handlers; list; list = g_list_next (list))
    {
      handler = (GimpContainerHandler *) list->data;

      handler_id = g_signal_connect (G_OBJECT (object),
				     handler->signame,
				     handler->callback,
				     handler->callback_data);

      g_object_set_qdata (G_OBJECT (object), handler->quark,
			  GUINT_TO_POINTER (handler_id));
    }

  switch (container->policy)
    {
    case GIMP_CONTAINER_POLICY_STRONG:
      gtk_object_ref (GTK_OBJECT (object));
      gtk_object_sink (GTK_OBJECT (object));
      break;

    case GIMP_CONTAINER_POLICY_WEAK:
      g_signal_connect (G_OBJECT (object), "destroy",
			G_CALLBACK (gimp_container_child_destroy_callback),
			container);
      break;
    }

  container->num_children++;

  g_signal_emit (GTK_OBJECT (container), container_signals[ADD], 0, object);

  return TRUE;
}

gboolean
gimp_container_remove (GimpContainer *container,
		       GimpObject    *object)
{
  GimpContainerHandler *handler;
  GList                *list;
  guint                 handler_id;

  g_return_val_if_fail (GIMP_IS_CONTAINER (container), FALSE);
  g_return_val_if_fail (object != NULL, FALSE);
  g_return_val_if_fail (g_type_instance_is_a ((GTypeInstance *) object,
					      container->children_type), FALSE);

  if (! gimp_container_have (container, object))
    {
      g_warning ("%s(): container %p does not contain object %p",
		 G_GNUC_FUNCTION, container, object);
      return FALSE;
    }

  for (list = container->handlers; list; list = g_list_next (list))
    {
      handler = (GimpContainerHandler *) list->data;

      handler_id = GPOINTER_TO_UINT
	(g_object_get_qdata (G_OBJECT (object), handler->quark));

      if (handler_id)
	{
	  g_signal_handler_disconnect (G_OBJECT (object), handler_id);

	  g_object_set_qdata (G_OBJECT (object), handler->quark, NULL);
	}
    }

  g_object_ref (G_OBJECT (object));

  switch (container->policy)
    {
    case GIMP_CONTAINER_POLICY_STRONG:
      g_object_unref (G_OBJECT (object));
      break;

    case GIMP_CONTAINER_POLICY_WEAK:
      g_signal_handlers_disconnect_by_func
	(G_OBJECT (object),
	 G_CALLBACK (gimp_container_child_destroy_callback),
	 container);
      break;
    }

  container->num_children--;

  g_signal_emit (G_OBJECT (container), container_signals[REMOVE], 0,
		 object);

  g_object_unref (G_OBJECT (object));

  return TRUE;
}

gboolean
gimp_container_insert (GimpContainer *container,
		       GimpObject    *object,
		       gint           index)
{
  g_return_val_if_fail (GIMP_IS_CONTAINER (container), FALSE);
  g_return_val_if_fail (object != NULL, FALSE);
  g_return_val_if_fail (g_type_instance_is_a ((GTypeInstance *) object,
					      container->children_type), FALSE);

  g_return_val_if_fail (index >= -1 &&
			index <= container->num_children, FALSE);

  if (gimp_container_have (container, object))
    {
      g_warning ("%s(): container %p already contains object %p",
		 G_GNUC_FUNCTION, container, object);
      return FALSE;
    }

  if (gimp_container_add (container, object))
    {
      return gimp_container_reorder (container, object, index);
    }

  return FALSE;
}

gboolean
gimp_container_reorder (GimpContainer *container,
			GimpObject    *object,
			gint           new_index)
{
  g_return_val_if_fail (GIMP_IS_CONTAINER (container), FALSE);
  g_return_val_if_fail (object != NULL, FALSE);
  g_return_val_if_fail (g_type_instance_is_a ((GTypeInstance *) object,
					      container->children_type), FALSE);

  g_return_val_if_fail (new_index >= -1 &&
			new_index < container->num_children, FALSE);

  if (! gimp_container_have (container, object))
    {
      g_warning ("%s(): container %p does not contain object %p",
		 G_GNUC_FUNCTION, container, object);
      return FALSE;
    }

  g_signal_emit (G_OBJECT (container), container_signals[REORDER], 0,
		 object, new_index);

  return TRUE;
}

gboolean
gimp_container_have (GimpContainer *container,
		     GimpObject    *object)
{
  gboolean have = FALSE;

  g_return_val_if_fail (GIMP_IS_CONTAINER (container), FALSE);

  if (container->num_children < 1)
    return FALSE;

  g_signal_emit (G_OBJECT (container), container_signals[HAVE], 0,
		 object, &have);

  return have;
}

void
gimp_container_foreach (GimpContainer  *container,
			GFunc           func,
			gpointer        user_data)
{
  g_return_if_fail (GIMP_IS_CONTAINER (container));
  g_return_if_fail (func != NULL);

  if (container->num_children > 0)
    g_signal_emit (G_OBJECT (container), container_signals[FOREACH], 0,
		   func, user_data);
}

GimpObject *
gimp_container_get_child_by_name (const GimpContainer *container,
				  const gchar         *name)
{
  GimpObject *object = NULL;

  g_return_val_if_fail (GIMP_IS_CONTAINER (container), NULL);
  g_return_val_if_fail (name != NULL, NULL);

  g_signal_emit (G_OBJECT (container), container_signals[GET_CHILD_BY_NAME], 0,
		 name, &object);

  return object;
}

GimpObject *
gimp_container_get_child_by_index (const GimpContainer *container,
				   gint                 index)
{
  GimpObject *object = NULL;

  g_return_val_if_fail (GIMP_IS_CONTAINER (container), NULL);
  g_return_val_if_fail (index >= 0 && index < container->num_children, NULL);

  g_signal_emit (G_OBJECT (container), container_signals[GET_CHILD_BY_INDEX], 0,
		 index, &object);

  return object;
}

gint
gimp_container_get_child_index (const GimpContainer *container,
				const GimpObject    *object)
{
  gint index = -1;

  g_return_val_if_fail (GIMP_IS_CONTAINER (container), -1);
  g_return_val_if_fail (object != NULL, -1);
  g_return_val_if_fail (g_type_instance_is_a ((GTypeInstance *) object,
					      container->children_type), -1);

  g_signal_emit (G_OBJECT (container), container_signals[GET_CHILD_INDEX], 0,
		 object, &index);

  return index;
}

void
gimp_container_freeze (GimpContainer *container)
{
  g_return_if_fail (GIMP_IS_CONTAINER (container));

  container->freeze_count++;

  if (container->freeze_count == 1)
    g_signal_emit (G_OBJECT (container), container_signals[FREEZE], 0);
}

void
gimp_container_thaw (GimpContainer *container)
{
  g_return_if_fail (GIMP_IS_CONTAINER (container));

  if (container->freeze_count > 0)
    container->freeze_count--;

  if (container->freeze_count == 0)
    g_signal_emit (G_OBJECT (container), container_signals[THAW], 0);
}

gboolean
gimp_container_frozen (GimpContainer *container)
{
  g_return_val_if_fail (GIMP_IS_CONTAINER (container), FALSE);

  return (container->freeze_count > 0) ? TRUE : FALSE;
}

static void
gimp_container_add_handler_foreach_func (GimpObject           *object,
					 GimpContainerHandler *handler)
{
  guint  handler_id;

  handler_id = g_signal_connect (G_OBJECT (object),
				 handler->signame,
				 handler->callback,
				 handler->callback_data);

  g_object_set_qdata (G_OBJECT (object), handler->quark,
		      GUINT_TO_POINTER (handler_id));
}

GQuark
gimp_container_add_handler (GimpContainer *container,
			    const gchar   *signame,
			    GCallback      callback,
			    gpointer       callback_data)
{
  GimpContainerHandler *handler;
  gchar                *key;

  g_return_val_if_fail (GIMP_IS_CONTAINER (container), 0);

  g_return_val_if_fail (signame != NULL, 0);
  g_return_val_if_fail (g_signal_lookup (signame, container->children_type), 0);
  g_return_val_if_fail (callback != NULL, 0);

  handler = g_new0 (GimpContainerHandler, 1);

  /*  create a unique key for this handler  */
  key = g_strdup_printf ("%s-%p", signame, handler);

  handler->signame       = g_strdup (signame);
  handler->callback      = callback;
  handler->callback_data = callback_data;
  handler->quark         = g_quark_from_string (key);

  g_free (key);

  container->handlers = g_list_prepend (container->handlers, handler);

  gimp_container_foreach (container,
			  (GFunc) gimp_container_add_handler_foreach_func,
			  handler);

  return handler->quark;
}

static void
gimp_container_remove_handler_foreach_func (GimpObject           *object,
					    GimpContainerHandler *handler)
{
  guint  handler_id;

  handler_id =
    GPOINTER_TO_UINT (g_object_get_qdata (G_OBJECT (object),
					  handler->quark));

  if (handler_id)
    {
      g_signal_handler_disconnect (G_OBJECT (object), handler_id);

      g_object_set_qdata (G_OBJECT (object), handler->quark, NULL);
    }
}

void
gimp_container_remove_handler (GimpContainer *container,
			       GQuark         id)
{
  GimpContainerHandler *handler;
  GList                *list;

  g_return_if_fail (container != NULL);
  g_return_if_fail (GIMP_IS_CONTAINER (container));
  g_return_if_fail (id != 0);

  for (list = container->handlers; list; list = g_list_next (list))
    {
      handler = (GimpContainerHandler *) list->data;

      if (handler->quark == id)
	break;
    }

  if (! list)
    {
      g_warning ("tried to disconnect handler which is not connected");
      return;
    }

  gimp_container_foreach (container,
			  (GFunc) gimp_container_remove_handler_foreach_func,
			  handler);

  container->handlers = g_list_remove (container->handlers, handler);

  g_free (handler->signame);
  g_free (handler);
}
