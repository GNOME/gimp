/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
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

#include "gimpsignal.h"
#include "gimpsetP.h"

/* Yep, this can be optimized quite a lot */

typedef struct _GimpSetHandler
{
  const gchar   *signame;
  GtkSignalFunc  func;
  gpointer       user_data;
} GimpSetHandler;

typedef struct
{
  gpointer  object;
  GArray   *handlers;
  guint     destroy_handler;
} Node;

enum
{
  ADD,
  REMOVE,
  ACTIVE_CHANGED,
  LAST_SIGNAL
};

static Node * gimp_set_find_node (GimpSet* set, gpointer object);
static Node * gimp_set_node_new  (GimpSet* set, gpointer obbject);
static void   gimp_set_node_free (GimpSet* set, Node* node);

static guint gimp_set_signals [LAST_SIGNAL];

static GimpObjectClass* parent_class;

static void
gimp_set_destroy (GtkObject* object)
{
  GimpSet* set = GIMP_SET (object);
  GSList* list;

  for (list = set->list; list; list = list->next)
    gimp_set_node_free (set, list->data);

  g_slist_free (set->list);
  g_array_free (set->handlers, TRUE);

  if (GTK_OBJECT_CLASS (parent_class)->destroy)
    (* GTK_OBJECT_CLASS (parent_class)->destroy) (object);
}

static void
gimp_set_init (GimpSet* set)
{
  set->list = NULL;
  set->type = GTK_TYPE_OBJECT;
  set->handlers = g_array_new (FALSE, FALSE, sizeof (GimpSetHandler));
  set->active_element = NULL;
}

static void
gimp_set_class_init (GimpSetClass* klass)
{
  GtkObjectClass *object_class;

  object_class = GTK_OBJECT_CLASS (klass);

  parent_class = gtk_type_class (gimp_object_get_type ());
	
  gimp_set_signals[ADD]=
    gimp_signal_new ("add",
		     GTK_RUN_FIRST,
		     object_class->type,
                     GTK_SIGNAL_OFFSET (GimpSetClass,
					add),
		     gimp_sigtype_pointer);

  gimp_set_signals[REMOVE]=
    gimp_signal_new ("remove",
		     GTK_RUN_FIRST,
		     object_class->type,
                     GTK_SIGNAL_OFFSET (GimpSetClass,
					remove),
		     gimp_sigtype_pointer);

  gimp_set_signals[ACTIVE_CHANGED]=
    gimp_signal_new ("active_changed",
		     GTK_RUN_FIRST,
		     object_class->type,
                     GTK_SIGNAL_OFFSET (GimpSetClass,
					active_changed),
		     gimp_sigtype_pointer);

  gtk_object_class_add_signals (object_class, gimp_set_signals, LAST_SIGNAL);

  object_class->destroy = gimp_set_destroy;

  klass->add            = NULL;
  klass->remove         = NULL;
  klass->active_changed = NULL;
}

GtkType
gimp_set_get_type (void)
{
  static GtkType gimpset_type = 0;

  GIMP_TYPE_INIT(gimpset_type,
		 GimpSet,
		 GimpSetClass,
		 gimp_set_init,
		 gimp_set_class_init,
		 GIMP_TYPE_OBJECT);

  return gimpset_type;
}


GimpSet *
gimp_set_new (GtkType   type,
	      gboolean  weak)
{
  GimpSet *set;

  /*  untyped sets must not be weak, since we can't attach a
   *  destroy handler
   */
  g_assert (!(type == GTK_TYPE_NONE && weak == TRUE));

  set = gtk_type_new (gimp_set_get_type ());
  set->type = type;
  set->weak = weak;

  return set;
}

static void
gimp_set_destroy_cb (GtkObject *object,
		     gpointer   data)
{
  GimpSet *set = GIMP_SET (data);

  gimp_set_remove (set, object);
}

static Node *
gimp_set_find_node (GimpSet  *set,
		    gpointer  object)
{
  GSList *list = set->list;

  for(list = set->list; list; list = list->next)
    {
      Node *node = list->data;

      if (node->object == object)
	return node;
    }

  return NULL;
}

static Node*
gimp_set_node_new (GimpSet  *set,
		   gpointer  object)
{
  gint i;
  Node *node = g_new (Node, 1);

  node->object = object;
  node->handlers = g_array_new (FALSE, FALSE, sizeof (guint));
  g_array_set_size (node->handlers, set->handlers->len);

  for (i = 0; i < node->handlers->len; i++)
    {
      GimpSetHandler *handler =
	&g_array_index (set->handlers, GimpSetHandler, i);

      if (handler->signame)
	g_array_index (node->handlers, guint, i)
	  = gtk_signal_connect (GTK_OBJECT(object),
				handler->signame,
				handler->func,
				handler->user_data);
    }

  if(set->weak)
    node->destroy_handler =
      gtk_signal_connect(GTK_OBJECT (object), "destroy",
			 GTK_SIGNAL_FUNC (gimp_set_destroy_cb),
			 set);

  return node;
}

static void
gimp_set_node_free (GimpSet *set,
		    Node    *node)
{
  gint i;
  GimpObject *object = node->object;

  for (i=0; i < set->handlers->len; i++)
    {
      GimpSetHandler* handler =
	&g_array_index (set->handlers, GimpSetHandler, i);

      if (handler->signame)
	gtk_signal_disconnect (GTK_OBJECT (object),
			       g_array_index (node->handlers, guint, i));
    }

  if (set->weak)
    gtk_signal_disconnect (GTK_OBJECT (object),
			   node->destroy_handler);

  g_array_free (node->handlers, TRUE);
  g_free (node);
}

gboolean
gimp_set_add (GimpSet  *set,
	      gpointer  val)
{
  g_return_val_if_fail (set, FALSE);

  if (set->type != GTK_TYPE_NONE)
    g_return_val_if_fail (GTK_CHECK_TYPE (val, set->type), FALSE);
	
  if (gimp_set_find_node (set, val))
    return FALSE;
	
  set->list = g_slist_prepend (set->list, gimp_set_node_new (set, val));

  gtk_signal_emit (GTK_OBJECT (set), gimp_set_signals[ADD], val);

  return TRUE;
}

gboolean
gimp_set_remove (GimpSet  *set,
		 gpointer  val)
{
  Node *node;
	
  g_return_val_if_fail (set, FALSE);

  node = gimp_set_find_node (set, val);
  g_return_val_if_fail (node, FALSE);

  gimp_set_node_free (set, node);
  set->list = g_slist_remove (set->list, node);

  gtk_signal_emit (GTK_OBJECT (set), gimp_set_signals[REMOVE], val);

  return TRUE;
}

gboolean
gimp_set_have (GimpSet  *set,
	       gpointer  val)
{
  return !!gimp_set_find_node (set, val);
}

void
gimp_set_foreach (GimpSet  *set,
		  GFunc     func,
		  gpointer  user_data)
{
  GSList* list;

  for (list = set->list; list; list = list->next)
    {
      Node *node = list->data;

      func (node->object, user_data);
    }
}

GtkType
gimp_set_type (GimpSet* set)
{
  return set->type;
}

void
gimp_set_set_active (GimpSet  *set,
		     gpointer  object)
{
  if (object != set->active_element && gimp_set_have (set, object))
    {
      set->active_element = object;
      gtk_signal_emit (GTK_OBJECT (set),
		       gimp_set_signals[ACTIVE_CHANGED],
		       object);
    }
}

gpointer
gimp_set_get_active (GimpSet* set)
{
  if (gimp_set_have (set, set->active_element))
    return set->active_element;

  return NULL;
}

GimpSetHandlerId
gimp_set_add_handler (GimpSet       *set,
		      const gchar   *signame,
		      GtkSignalFunc  handler,
		      gpointer       user_data)
{
  GimpSetHandler set_handler;
  GSList *list;
  guint a;

  g_assert (signame);

  /*  you can't set a handler on something that's not a GTK object  */
  g_assert (set->type != GTK_TYPE_NONE);

  set_handler.signame = signame;
  set_handler.func = handler;
  set_handler.user_data = user_data;
	
  for (a = 0; a < set->handlers->len; a++)
    if (! g_array_index (set->handlers, GimpSetHandler, a).signame)
      break;

  if (a < set->handlers->len)
    {
      g_array_index (set->handlers, GimpSetHandler, a) = set_handler;
      for (list = set->list; list; list = list->next)
	{
	  Node *node = list->data;
	  guint i = gtk_signal_connect (GTK_OBJECT (node->object), signame,
					handler,
					user_data);
	  g_array_index (node->handlers, guint, a) = i;
	}
    }
  else
    {
      g_array_append_val (set->handlers, set_handler);

      for (list = set->list; list; list = list->next)
	{
	  Node* node = list->data;

	  guint i = gtk_signal_connect (GTK_OBJECT(node->object), signame,
					handler,
					user_data);
	  g_array_append_val (node->handlers, i);
	}
    }

  return a;
}

void
gimp_set_remove_handler (GimpSet          *set,
			 GimpSetHandlerId  id)
{
  GSList* list;

  /*  you can't remove a signal handler on something that's not a GTK object  */
  g_return_if_fail (set->type != GTK_TYPE_NONE);

  for (list = set->list; list; list = list->next)
    {
      Node *node = list->data;

      gtk_signal_disconnect (GTK_OBJECT (node->object),
			     g_array_index (node->handlers, guint, id));
    }

  g_array_index (set->handlers, GimpSetHandler, id).signame = NULL;
}
