/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpcontainerview.c
 * Copyright (C) 2001 Michael Natterer
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

#include "widgets-types.h"

#include "gimpcontainerview.h"
#include "gimpdnd.h"

#include "gimpcontainer.h"
#include "gimpcontext.h"
#include "gimpmarshal.h"
#include "gimpviewable.h"


enum
{
  SET_CONTAINER,
  INSERT_ITEM,
  REMOVE_ITEM,
  REORDER_ITEM,
  SELECT_ITEM,
  ACTIVATE_ITEM,
  CONTEXT_ITEM,
  CLEAR_ITEMS,
  SET_PREVIEW_SIZE,
  LAST_SIGNAL
};


static void   gimp_container_view_class_init  (GimpContainerViewClass *klass);
static void   gimp_container_view_init        (GimpContainerView      *panel);
static void   gimp_container_view_destroy     (GtkObject              *object);

static void   gimp_container_view_real_set_container (GimpContainerView *view,
						      GimpContainer     *container);

static void   gimp_container_view_clear_items (GimpContainerView      *view);
static void   gimp_container_view_real_clear_items (GimpContainerView *view);

static void   gimp_container_view_add_foreach (GimpViewable           *viewable,
					       GimpContainerView      *view);
static void   gimp_container_view_add         (GimpContainerView      *view,
					       GimpViewable           *viewable,
					       GimpContainer          *container);
static void   gimp_container_view_remove      (GimpContainerView      *view,
					       GimpViewable           *viewable,
					       GimpContainer          *container);
static void   gimp_container_view_reorder     (GimpContainerView      *view,
					       GimpViewable           *viewable,
					       gint                    new_index,
					       GimpContainer          *container);

static void   gimp_container_view_context_changed (GimpContext        *context,
						   GimpViewable       *viewable,
						   GimpContainerView  *view);
static void   gimp_container_view_drop_viewable_callback (GtkWidget    *widget,
							  GimpViewable *viewable,
							  gpointer      data);


static guint  view_signals[LAST_SIGNAL] = { 0 };

static GtkVBoxClass *parent_class = NULL;


GtkType
gimp_container_view_get_type (void)
{
  static guint view_type = 0;

  if (! view_type)
    {
      GtkTypeInfo view_info =
      {
	"GimpContainerView",
	sizeof (GimpContainerView),
	sizeof (GimpContainerViewClass),
	(GtkClassInitFunc) gimp_container_view_class_init,
	(GtkObjectInitFunc) gimp_container_view_init,
	/* reserved_1 */ NULL,
	/* reserved_2 */ NULL,
        (GtkClassInitFunc) NULL
      };

      view_type = gtk_type_unique (GTK_TYPE_VBOX, &view_info);
    }

  return view_type;
}

static void
gimp_container_view_class_init (GimpContainerViewClass *klass)
{
  GtkObjectClass *object_class;

  object_class = (GtkObjectClass *) klass;
  
  parent_class = gtk_type_class (GTK_TYPE_VBOX);

  view_signals[SET_CONTAINER] =
    gtk_signal_new ("set_container",
                    GTK_RUN_LAST,
                    object_class->type,
                    GTK_SIGNAL_OFFSET (GimpContainerViewClass,
                                       set_container),
                    gtk_marshal_NONE__OBJECT,
                    GTK_TYPE_NONE, 1,
                    GIMP_TYPE_OBJECT);

  view_signals[INSERT_ITEM] =
    gtk_signal_new ("insert_item",
                    GTK_RUN_LAST,
                    object_class->type,
                    GTK_SIGNAL_OFFSET (GimpContainerViewClass,
                                       insert_item),
                    gimp_marshal_POINTER__POINTER_INT,
                    GTK_TYPE_POINTER, 2,
                    GIMP_TYPE_OBJECT,
		    GTK_TYPE_INT);

  view_signals[REMOVE_ITEM] =
    gtk_signal_new ("remove_item",
                    GTK_RUN_FIRST,
                    object_class->type,
                    GTK_SIGNAL_OFFSET (GimpContainerViewClass,
                                       remove_item),
                    gtk_marshal_NONE__POINTER_POINTER,
                    GTK_TYPE_NONE, 2,
                    GIMP_TYPE_OBJECT,
		    GTK_TYPE_POINTER);

  view_signals[REORDER_ITEM] =
    gtk_signal_new ("reorder_item",
                    GTK_RUN_FIRST,
                    object_class->type,
                    GTK_SIGNAL_OFFSET (GimpContainerViewClass,
                                       reorder_item),
                    gtk_marshal_NONE__POINTER_INT_POINTER,
                    GTK_TYPE_NONE, 3,
                    GIMP_TYPE_OBJECT,
		    GTK_TYPE_INT,
		    GTK_TYPE_POINTER);

  view_signals[SELECT_ITEM] =
    gtk_signal_new ("select_item",
                    GTK_RUN_FIRST,
                    object_class->type,
                    GTK_SIGNAL_OFFSET (GimpContainerViewClass,
                                       select_item),
                    gtk_marshal_NONE__POINTER_POINTER,
                    GTK_TYPE_NONE, 2,
                    GIMP_TYPE_OBJECT,
		    GTK_TYPE_POINTER);

  view_signals[ACTIVATE_ITEM] =
    gtk_signal_new ("activate_item",
                    GTK_RUN_FIRST,
                    object_class->type,
                    GTK_SIGNAL_OFFSET (GimpContainerViewClass,
                                       activate_item),
                    gtk_marshal_NONE__POINTER_POINTER,
                    GTK_TYPE_NONE, 2,
                    GIMP_TYPE_OBJECT,
		    GTK_TYPE_POINTER);

  view_signals[CONTEXT_ITEM] =
    gtk_signal_new ("context_item",
                    GTK_RUN_FIRST,
                    object_class->type,
                    GTK_SIGNAL_OFFSET (GimpContainerViewClass,
                                       context_item),
                    gtk_marshal_NONE__POINTER_POINTER,
                    GTK_TYPE_NONE, 2,
                    GIMP_TYPE_OBJECT,
		    GTK_TYPE_POINTER);

  view_signals[CLEAR_ITEMS] =
    gtk_signal_new ("clear_items",
                    GTK_RUN_FIRST,
                    object_class->type,
                    GTK_SIGNAL_OFFSET (GimpContainerViewClass,
                                       clear_items),
                    gtk_signal_default_marshaller,
                    GTK_TYPE_NONE, 0);

  view_signals[SET_PREVIEW_SIZE] =
    gtk_signal_new ("set_preview_size",
                    GTK_RUN_FIRST,
                    object_class->type,
                    GTK_SIGNAL_OFFSET (GimpContainerViewClass,
                                       set_preview_size),
                    gtk_signal_default_marshaller,
                    GTK_TYPE_NONE, 0);

  gtk_object_class_add_signals (object_class, view_signals, LAST_SIGNAL);

  object_class->destroy = gimp_container_view_destroy;

  klass->set_container    = gimp_container_view_real_set_container;
  klass->insert_item      = NULL;
  klass->remove_item      = NULL;
  klass->reorder_item     = NULL;
  klass->select_item      = NULL;
  klass->activate_item    = NULL;
  klass->context_item     = NULL;
  klass->clear_items      = gimp_container_view_real_clear_items;
  klass->set_preview_size = NULL;
}

static void
gimp_container_view_init (GimpContainerView *view)
{
  view->container    = NULL;
  view->context      = NULL;

  view->hash_table   = g_hash_table_new (g_direct_hash, g_direct_equal);

  view->preview_size = 0;
}

static void
gimp_container_view_destroy (GtkObject *object)
{
  GimpContainerView *view;

  view = GIMP_CONTAINER_VIEW (object);

  gimp_container_view_set_container (view, NULL);

  gimp_container_view_set_context (view, NULL);

  g_hash_table_destroy (view->hash_table);

  view->hash_table = NULL;

  if (GTK_OBJECT_CLASS (parent_class)->destroy)
    GTK_OBJECT_CLASS (parent_class)->destroy (object);
}

void
gimp_container_view_set_container (GimpContainerView *view,
				   GimpContainer     *container)
{
  g_return_if_fail (view != NULL);
  g_return_if_fail (GIMP_IS_CONTAINER_VIEW (view));
  g_return_if_fail (!container || GIMP_IS_CONTAINER (container));

  if (container == view->container)
    return;

  gtk_signal_emit (GTK_OBJECT (view), view_signals[SET_CONTAINER],
		   container);
}

static void
gimp_container_view_real_set_container (GimpContainerView *view,
					GimpContainer     *container)
{
  if (view->container)
    {
      gimp_container_view_select_item (view, NULL);

      gimp_container_view_clear_items (view);

      gtk_signal_disconnect_by_func (GTK_OBJECT (view->container),
				     gimp_container_view_add,
				     view);
      gtk_signal_disconnect_by_func (GTK_OBJECT (view->container),
				     gimp_container_view_remove,
				     view);
      gtk_signal_disconnect_by_func (GTK_OBJECT (view->container),
				     gimp_container_view_reorder,
				     view);

      g_hash_table_destroy (view->hash_table);

      view->hash_table = g_hash_table_new (g_direct_hash, g_direct_equal);

      if (view->context)
	{
	  gtk_signal_disconnect_by_func (GTK_OBJECT (view->context),
					 gimp_container_view_context_changed,
					 view);

	  gtk_drag_dest_unset (GTK_WIDGET (view));
	  gimp_dnd_viewable_dest_unset (GTK_WIDGET (view),
					view->container->children_type);
	}
    }

  view->container = container;

  if (view->container)
    {
      gimp_container_foreach (view->container,
			      (GFunc) gimp_container_view_add_foreach,
			      view);

      gtk_signal_connect_object (GTK_OBJECT (view->container), "add",
				 GTK_SIGNAL_FUNC (gimp_container_view_add),
				 GTK_OBJECT (view));

      gtk_signal_connect_object (GTK_OBJECT (view->container), "remove",
				 GTK_SIGNAL_FUNC (gimp_container_view_remove),
				 GTK_OBJECT (view));

      gtk_signal_connect_object (GTK_OBJECT (view->container), "reorder",
				 GTK_SIGNAL_FUNC (gimp_container_view_reorder),
				 GTK_OBJECT (view));

      if (view->context)
	{
	  GimpObject  *object;
	  const gchar *signal_name;

	  signal_name =
	    gimp_context_type_to_signal_name (view->container->children_type);

	  gtk_signal_connect
	    (GTK_OBJECT (view->context), signal_name,
	     GTK_SIGNAL_FUNC (gimp_container_view_context_changed),
	     view);

	  object = gimp_context_get_by_type (view->context,
					     view->container->children_type);

	  gimp_container_view_select_item (view,
					   object ? GIMP_VIEWABLE (object): NULL);

	  gimp_gtk_drag_dest_set_by_type (GTK_WIDGET (view),
					  GTK_DEST_DEFAULT_ALL,
					  view->container->children_type,
					  GDK_ACTION_COPY);
	  gimp_dnd_viewable_dest_set (GTK_WIDGET (view),
				      view->container->children_type,
				      gimp_container_view_drop_viewable_callback,
				      NULL);
	}
    }
}

void
gimp_container_view_set_context (GimpContainerView *view,
				 GimpContext       *context)
{
  g_return_if_fail (view != NULL);
  g_return_if_fail (GIMP_IS_CONTAINER_VIEW (view));
  g_return_if_fail (! context || GIMP_IS_CONTEXT (context));

  if (context == view->context)
    return;

  if (view->context && view->container)
    {
      gtk_signal_disconnect_by_func (GTK_OBJECT (view->context),
				     gimp_container_view_context_changed,
				     view);

      gtk_drag_dest_unset (GTK_WIDGET (view));
      gimp_dnd_viewable_dest_unset (GTK_WIDGET (view),
				    view->container->children_type);
    }

  view->context = context;

  if (view->context && view->container)
    {
      GimpObject  *object;
      const gchar *signal_name;

      signal_name =
	gimp_context_type_to_signal_name (view->container->children_type);

      gtk_signal_connect (GTK_OBJECT (view->context), signal_name,
			  GTK_SIGNAL_FUNC (gimp_container_view_context_changed),
			  view);

      object = gimp_context_get_by_type (view->context,
					 view->container->children_type);

      gimp_container_view_select_item (view,
				       object ? GIMP_VIEWABLE (object) : NULL);

      gimp_gtk_drag_dest_set_by_type (GTK_WIDGET (view),
				      GTK_DEST_DEFAULT_ALL,
				      view->container->children_type,
				      GDK_ACTION_COPY);
      gimp_dnd_viewable_dest_set (GTK_WIDGET (view),
				  view->container->children_type,
				  gimp_container_view_drop_viewable_callback,
				  NULL);
    }
}

void
gimp_container_view_set_preview_size (GimpContainerView *view,
				      gint               preview_size)
{
  g_return_if_fail (view != NULL);
  g_return_if_fail (GIMP_IS_CONTAINER_VIEW (view));
  g_return_if_fail (preview_size > 0 && preview_size <= 256 /* FIXME: 64 */);

  view->preview_size = preview_size;

  gtk_signal_emit (GTK_OBJECT (view), view_signals[SET_PREVIEW_SIZE]);
}

void
gimp_container_view_set_name_func (GimpContainerView   *view,
				   GimpItemGetNameFunc  get_name_func)
{
  g_return_if_fail (view != NULL);
  g_return_if_fail (GIMP_IS_CONTAINER_VIEW (view));

  if (view->get_name_func != get_name_func)
    {
      view->get_name_func = get_name_func;
    }
}

void
gimp_container_view_select_item (GimpContainerView *view,
				 GimpViewable      *viewable)
{
  gpointer insert_data;

  g_return_if_fail (view != NULL);
  g_return_if_fail (GIMP_IS_CONTAINER_VIEW (view));
  g_return_if_fail (! viewable || GIMP_IS_VIEWABLE (viewable));

  insert_data = g_hash_table_lookup (view->hash_table, viewable);

  gtk_signal_emit (GTK_OBJECT (view), view_signals[SELECT_ITEM],
		   viewable, insert_data);
}

void
gimp_container_view_activate_item (GimpContainerView *view,
				   GimpViewable      *viewable)
{
  gpointer insert_data;

  g_return_if_fail (view != NULL);
  g_return_if_fail (GIMP_IS_CONTAINER_VIEW (view));
  g_return_if_fail (viewable != NULL);
  g_return_if_fail (GIMP_IS_VIEWABLE (viewable));

  insert_data = g_hash_table_lookup (view->hash_table, viewable);

  gtk_signal_emit (GTK_OBJECT (view), view_signals[ACTIVATE_ITEM],
		   viewable, insert_data);
}

void
gimp_container_view_context_item (GimpContainerView *view,
				  GimpViewable      *viewable)
{
  gpointer insert_data;

  g_return_if_fail (view != NULL);
  g_return_if_fail (GIMP_IS_CONTAINER_VIEW (view));
  g_return_if_fail (viewable != NULL);
  g_return_if_fail (GIMP_IS_VIEWABLE (viewable));

  insert_data = g_hash_table_lookup (view->hash_table, viewable);

  gtk_signal_emit (GTK_OBJECT (view), view_signals[CONTEXT_ITEM],
		   viewable, insert_data);
}

void
gimp_container_view_item_selected (GimpContainerView *view,
				   GimpViewable      *viewable)
{
  g_return_if_fail (view != NULL);
  g_return_if_fail (GIMP_IS_CONTAINER_VIEW (view));
  g_return_if_fail (viewable != NULL);
  g_return_if_fail (GIMP_IS_VIEWABLE (viewable));

  if (view->container && view->context)
    {
      gimp_context_set_by_type (view->context,
				view->container->children_type,
				GIMP_OBJECT (viewable));
    }

  gimp_container_view_select_item (view, viewable);
}

void
gimp_container_view_item_activated (GimpContainerView *view,
				    GimpViewable      *viewable)
{
  g_return_if_fail (view != NULL);
  g_return_if_fail (GIMP_IS_CONTAINER_VIEW (view));
  g_return_if_fail (viewable != NULL);
  g_return_if_fail (GIMP_IS_VIEWABLE (viewable));

  gimp_container_view_activate_item (view, viewable);
}

void
gimp_container_view_item_context (GimpContainerView *view,
				  GimpViewable      *viewable)
{
  g_return_if_fail (view != NULL);
  g_return_if_fail (GIMP_IS_CONTAINER_VIEW (view));
  g_return_if_fail (viewable != NULL);
  g_return_if_fail (GIMP_IS_VIEWABLE (viewable));

  gimp_container_view_context_item (view, viewable);
}

static void
gimp_container_view_clear_items (GimpContainerView *view)
{
  gtk_signal_emit (GTK_OBJECT (view), view_signals[CLEAR_ITEMS]);
}

static void
gimp_container_view_real_clear_items (GimpContainerView *view)
{
  g_hash_table_destroy (view->hash_table);

  view->hash_table = g_hash_table_new (g_direct_hash, g_direct_equal);
}

static void
gimp_container_view_add_foreach (GimpViewable      *viewable,
				 GimpContainerView *view)
{
  gpointer insert_data = NULL;

  gtk_signal_emit (GTK_OBJECT (view), view_signals[INSERT_ITEM],
		   viewable, -1, &insert_data);

  g_hash_table_insert (view->hash_table, viewable, insert_data);
}

static void
gimp_container_view_add (GimpContainerView *view,
			 GimpViewable      *viewable,
			 GimpContainer     *container)
{
  gpointer insert_data = NULL;
  gint     index;

  index = gimp_container_get_child_index (container,
                                          GIMP_OBJECT (viewable));

  gtk_signal_emit (GTK_OBJECT (view), view_signals[INSERT_ITEM],
		   viewable, index, &insert_data);

  g_hash_table_insert (view->hash_table, viewable, insert_data);
}

static void
gimp_container_view_remove (GimpContainerView *view,
			    GimpViewable      *viewable,
			    GimpContainer     *container)
{
  gpointer insert_data;

  insert_data = g_hash_table_lookup (view->hash_table, viewable);

  if (insert_data)
    {
      g_hash_table_remove (view->hash_table, viewable);

      gtk_signal_emit (GTK_OBJECT (view), view_signals[REMOVE_ITEM],
		       viewable, insert_data);
    }
}

static void
gimp_container_view_reorder (GimpContainerView *view,
			     GimpViewable      *viewable,
			     gint               new_index,
			     GimpContainer     *container)
{
  gpointer insert_data;

  insert_data = g_hash_table_lookup (view->hash_table, viewable);

  if (insert_data)
    {
      gtk_signal_emit (GTK_OBJECT (view), view_signals[REORDER_ITEM],
		       viewable, new_index, insert_data);
    }
}

static void
gimp_container_view_context_changed (GimpContext       *context,
				     GimpViewable      *viewable,
				     GimpContainerView *view)
{
  gpointer insert_data;

  insert_data = g_hash_table_lookup (view->hash_table, viewable);

  gtk_signal_emit (GTK_OBJECT (view), view_signals[SELECT_ITEM],
		   viewable, insert_data);
}

static void
gimp_container_view_drop_viewable_callback (GtkWidget    *widget,
					    GimpViewable *viewable,
					    gpointer      data)
{
  GimpContainerView *view;

  view = GIMP_CONTAINER_VIEW (widget);

  gimp_context_set_by_type (view->context,
                            view->container->children_type,
                            GIMP_OBJECT (viewable));
}
