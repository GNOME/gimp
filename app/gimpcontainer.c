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


typedef struct _GimpContainerHandler
{
  gchar         *signame;
  GtkSignalFunc  func;
  gpointer       user_data;

  GQuark         quark;  /*  used to attach the signal id's of child signals  */
} GimpContainerHandler;


enum
{
  ADD,
  REMOVE,
  LAST_SIGNAL
};


/*  local function prototypes  */

static void   gimp_container_init                   (GimpContainer      *container);
static void   gimp_container_class_init             (GimpContainerClass *klass);
static void   gimp_container_destroy                (GtkObject          *object);
static void   gimp_container_child_destroy_callback (GtkObject          *object,
						     gpointer            data);


static guint            container_signals[LAST_SIGNAL] = { 0 };
static GimpObjectClass *parent_class = NULL;


GtkType
gimp_container_get_type (void)
{
  static GtkType container_type = 0;

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

  container->children      = NULL;
  container->handlers      = NULL;
}

static void
gimp_container_class_init (GimpContainerClass* klass)
{
  GtkObjectClass *object_class;

  object_class = (GtkObjectClass *) klass;

  parent_class = gtk_type_class (GIMP_TYPE_OBJECT);

  container_signals[ADD] =
    gtk_signal_new ("add",
                    GTK_RUN_FIRST,
                    object_class->type,
                    GTK_SIGNAL_OFFSET (GimpContainerClass,
				       add),
                    gtk_marshal_NONE__OBJECT,
                    GTK_TYPE_NONE, 1,
                    GIMP_TYPE_OBJECT);

  container_signals[REMOVE] =
    gtk_signal_new ("remove",
                    GTK_RUN_FIRST,
                    object_class->type,
                    GTK_SIGNAL_OFFSET (GimpContainerClass,
                                       remove),
                    gtk_marshal_NONE__OBJECT,
                    GTK_TYPE_NONE, 1,
                    GIMP_TYPE_OBJECT);

  gtk_object_class_add_signals (object_class, container_signals, LAST_SIGNAL);

  object_class->destroy = gimp_container_destroy;

  klass->add            = NULL;
  klass->remove         = NULL;
}

static void
gimp_container_destroy (GtkObject *object)
{
  GimpContainer *container;

  container = GIMP_CONTAINER (object);

  while (container->children)
    {
      gimp_container_remove (container,
			     GIMP_OBJECT (container->children->data));
    }

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

GimpContainer *
gimp_container_new (GtkType             children_type,
		    GimpContainerPolicy policy)
{
  GimpContainer *container;

  g_return_val_if_fail (gtk_type_is_a (children_type, GIMP_TYPE_OBJECT), NULL);
  g_return_val_if_fail (policy == GIMP_CONTAINER_POLICY_STRONG ||
			policy == GIMP_CONTAINER_POLICY_WEAK, NULL);

  container = gtk_type_new (GIMP_TYPE_CONTAINER);

  container->children_type = children_type;
  container->policy        = policy;

  return container;
}

GtkType
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

  g_return_val_if_fail (container != NULL, FALSE);
  g_return_val_if_fail (GIMP_IS_CONTAINER (container), FALSE);

  g_return_val_if_fail (object != NULL, FALSE);
  g_return_val_if_fail (GTK_CHECK_TYPE (object, container->children_type),
			FALSE);

  if (gimp_container_lookup (container, object))
    return FALSE;

  container->children = g_list_prepend (container->children, object);
  container->num_children++;

  for (list = container->handlers; list; list = g_list_next (list))
    {
      handler = (GimpContainerHandler *) list->data;

      handler_id = gtk_signal_connect (GTK_OBJECT (object),
				       handler->signame,
				       handler->func,
				       handler->user_data);

      gtk_object_set_data_by_id (GTK_OBJECT (object), handler->quark,
				 GUINT_TO_POINTER (handler_id));
    }

  switch (container->policy)
    {
    case GIMP_CONTAINER_POLICY_STRONG:
      gtk_object_ref (GTK_OBJECT (object));
      gtk_object_sink (GTK_OBJECT (object));
      break;

    case GIMP_CONTAINER_POLICY_WEAK:
      gtk_signal_connect (GTK_OBJECT (object), "destroy",
			  GTK_SIGNAL_FUNC (gimp_container_child_destroy_callback),
			  container);
      break;
    }

  gtk_signal_emit (GTK_OBJECT (container), container_signals[ADD], object);

  return TRUE;
}

gboolean
gimp_container_remove (GimpContainer *container,
		       GimpObject    *object)
{
  GimpContainerHandler *handler;
  GList                *list;
  guint                 handler_id;

  g_return_val_if_fail (container, FALSE);
  g_return_val_if_fail (GIMP_IS_CONTAINER (container), FALSE);

  g_return_val_if_fail (object != NULL, FALSE);
  g_return_val_if_fail (GTK_CHECK_TYPE (object, container->children_type),
			FALSE);

  if (! gimp_container_lookup (container, object))
    return FALSE;

  for (list = container->handlers; list; list = g_list_next (list))
    {
      handler = (GimpContainerHandler *) list->data;

      handler_id = GPOINTER_TO_UINT
	(gtk_object_get_data_by_id (GTK_OBJECT (object), handler->quark));

      if (handler_id)
	{
	  gtk_signal_disconnect (GTK_OBJECT (object), handler_id);

	  gtk_object_set_data_by_id (GTK_OBJECT (object), handler->quark, NULL);
	}
    }

  container->children = g_list_remove (container->children, object);
  container->num_children--;

  gtk_object_ref (GTK_OBJECT (object));

  switch (container->policy)
    {
    case GIMP_CONTAINER_POLICY_STRONG:
      gtk_object_unref (GTK_OBJECT (object));
      break;

    case GIMP_CONTAINER_POLICY_WEAK:
      gtk_signal_disconnect_by_func
	(GTK_OBJECT (object),
	 GTK_SIGNAL_FUNC (gimp_container_child_destroy_callback),
	 container);
      break;
    }

  gtk_signal_emit (GTK_OBJECT (container), container_signals[REMOVE], object);

  gtk_object_unref (GTK_OBJECT (object));

  return TRUE;
}

const GList *
gimp_container_lookup (const GimpContainer *container,
		       const GimpObject    *object)
{
  g_return_val_if_fail (container != NULL, NULL);
  g_return_val_if_fail (GIMP_IS_CONTAINER (container), NULL);

  return g_list_find (container->children, (gpointer) object);
}

void
gimp_container_foreach (GimpContainer  *container,
			GFunc           func,
			gpointer        user_data)
{
  g_return_if_fail (container != NULL);
  g_return_if_fail (GIMP_IS_CONTAINER (container));
  g_return_if_fail (func != NULL);

  g_list_foreach (container->children, func, user_data);
}

GQuark
gimp_container_add_handler (GimpContainer *container,
			    const gchar   *signame,
			    GtkSignalFunc  callback,
			    gpointer       callback_data)
{
  GimpContainerHandler *handler;
  GtkObject            *object;
  GList                *list;
  gchar                *key;
  guint                 handler_id;

  g_return_val_if_fail (container != NULL, 0);
  g_return_val_if_fail (GIMP_IS_CONTAINER (container), 0);

  g_return_val_if_fail (signame != NULL, 0);
  g_return_val_if_fail (gtk_signal_lookup (signame,
					   container->children_type), 0);
  g_return_val_if_fail (callback != NULL, 0);

  handler = g_new (GimpContainerHandler, 1);

  /*  create a unique key for this handler  */
  key = g_strdup_printf ("%s-%p", signame, handler);

  handler->signame   = g_strdup (signame);
  handler->func      = callback;
  handler->user_data = callback_data;
  handler->quark     = g_quark_from_string (key);

  g_free (key);

  container->handlers = g_list_prepend (container->handlers, handler);

  for (list = container->children; list; list = g_list_next (list))
    {
      object = GTK_OBJECT (list->data);

      handler_id = gtk_signal_connect (object,
				       handler->signame,
				       handler->func,
				       handler->user_data);

      gtk_object_set_data_by_id (object, handler->quark,
				 GUINT_TO_POINTER (handler_id));
    }

  return handler->quark;
}

void
gimp_container_remove_handler (GimpContainer *container,
			       GQuark         id)
{
  GimpContainerHandler *handler;
  GtkObject            *object;
  GList                *list;
  guint                 handler_id;

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

  for (list = container->children; list; list = g_list_next (list))
    {
      object = GTK_OBJECT (list->data);

      handler_id = GPOINTER_TO_UINT
	(gtk_object_get_data_by_id (object, handler->quark));

      if (handler_id)
	{
	  gtk_signal_disconnect (object, handler_id);

	  gtk_object_set_data_by_id (object, handler->quark, NULL);
	}
    }

  container->handlers = g_list_remove (container->handlers, handler);

  g_free (handler->signame);
  g_free (handler);
}
