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

#include "core/gimpcontainer.h"
#include "core/gimpcontext.h"
#include "core/gimpmarshal.h"
#include "core/gimpviewable.h"

#include "gimpcontainerview.h"
#include "gimpcontainerview-utils.h"
#include "gimpdnd.h"


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
static void   gimp_container_view_style_set   (GtkWidget              *widget,
					       GtkStyle               *prev_style);

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


GType
gimp_container_view_get_type (void)
{
  static GType view_type = 0;

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
  GtkWidgetClass *widget_class;

  object_class = (GtkObjectClass *) klass;
  widget_class = (GtkWidgetClass *) klass;
  
  parent_class = g_type_class_peek_parent (klass);

  view_signals[SET_CONTAINER] =
    g_signal_new ("set_container",
		  G_TYPE_FROM_CLASS (klass),
		  G_SIGNAL_RUN_LAST,
		  G_STRUCT_OFFSET (GimpContainerViewClass, set_container),
		  NULL, NULL,
		  g_cclosure_marshal_VOID__OBJECT,
		  G_TYPE_NONE, 1,
		  GIMP_TYPE_OBJECT);

  view_signals[INSERT_ITEM] =
    g_signal_new ("insert_item",
		  G_TYPE_FROM_CLASS (klass),
		  G_SIGNAL_RUN_LAST,
		  G_STRUCT_OFFSET (GimpContainerViewClass, insert_item),
		  NULL, NULL,
		  gimp_cclosure_marshal_POINTER__OBJECT_INT,
		  G_TYPE_POINTER, 2,
		  GIMP_TYPE_OBJECT,
		  G_TYPE_INT);

  view_signals[REMOVE_ITEM] =
    g_signal_new ("remove_item",
		  G_TYPE_FROM_CLASS (klass),
		  G_SIGNAL_RUN_FIRST,
		  G_STRUCT_OFFSET (GimpContainerViewClass, remove_item),
		  NULL, NULL,
		  gimp_cclosure_marshal_VOID__OBJECT_POINTER,
		  G_TYPE_NONE, 2,
		  GIMP_TYPE_OBJECT,
		  G_TYPE_POINTER);

  view_signals[REORDER_ITEM] =
    g_signal_new ("reorder_item",
		  G_TYPE_FROM_CLASS (klass),
		  G_SIGNAL_RUN_FIRST,
		  G_STRUCT_OFFSET (GimpContainerViewClass, reorder_item),
		  NULL, NULL,
		  gimp_cclosure_marshal_VOID__OBJECT_INT_POINTER,
		  G_TYPE_NONE, 3,
		  GIMP_TYPE_OBJECT,
		  G_TYPE_INT,
		  G_TYPE_POINTER);

  view_signals[SELECT_ITEM] =
    g_signal_new ("select_item",
		  G_TYPE_FROM_CLASS (klass),
		  G_SIGNAL_RUN_FIRST,
		  G_STRUCT_OFFSET (GimpContainerViewClass, select_item),
		  NULL, NULL,
		  gimp_cclosure_marshal_VOID__OBJECT_POINTER,
		  G_TYPE_NONE, 2,
		  GIMP_TYPE_OBJECT,
		  G_TYPE_POINTER);

  view_signals[ACTIVATE_ITEM] =
    g_signal_new ("activate_item",
		  G_TYPE_FROM_CLASS (klass),
		  G_SIGNAL_RUN_FIRST,
		  G_STRUCT_OFFSET (GimpContainerViewClass, activate_item),
		  NULL, NULL,
		  gimp_cclosure_marshal_VOID__OBJECT_POINTER,
		  G_TYPE_NONE, 2,
		  GIMP_TYPE_OBJECT,
		  G_TYPE_POINTER);

  view_signals[CONTEXT_ITEM] =
    g_signal_new ("context_item",
		  G_TYPE_FROM_CLASS (klass),
		  G_SIGNAL_RUN_FIRST,
		  G_STRUCT_OFFSET (GimpContainerViewClass, context_item),
		  NULL, NULL,
		  gimp_cclosure_marshal_VOID__OBJECT_POINTER,
		  G_TYPE_NONE, 2,
		  GIMP_TYPE_OBJECT,
		  G_TYPE_POINTER);

  view_signals[CLEAR_ITEMS] =
    g_signal_new ("clear_items",
		  G_TYPE_FROM_CLASS (klass),
		  G_SIGNAL_RUN_FIRST,
		  G_STRUCT_OFFSET (GimpContainerViewClass, clear_items),
		  NULL, NULL,
		  g_cclosure_marshal_VOID__VOID,
		  G_TYPE_NONE, 0);

  view_signals[SET_PREVIEW_SIZE] =
    g_signal_new ("set_preview_size",
		  G_TYPE_FROM_CLASS (klass),
		  G_SIGNAL_RUN_FIRST,
		  G_STRUCT_OFFSET (GimpContainerViewClass, set_preview_size),
		  NULL, NULL,
		  g_cclosure_marshal_VOID__VOID,
		  G_TYPE_NONE, 0);

  object_class->destroy   = gimp_container_view_destroy;

  widget_class->style_set = gimp_container_view_style_set;

  klass->set_container    = gimp_container_view_real_set_container;
  klass->insert_item      = NULL;
  klass->remove_item      = NULL;
  klass->reorder_item     = NULL;
  klass->select_item      = NULL;
  klass->activate_item    = NULL;
  klass->context_item     = NULL;
  klass->clear_items      = gimp_container_view_real_clear_items;
  klass->set_preview_size = NULL;

  gtk_widget_class_install_style_property (widget_class,
                                           g_param_spec_int ("content_spacing",
                                                             NULL, NULL,
                                                             0,
                                                             G_MAXINT,
                                                             0,
                                                             G_PARAM_READABLE));

  gtk_widget_class_install_style_property (widget_class,
                                           g_param_spec_int ("button_spacing",
                                                             NULL, NULL,
                                                             0,
                                                             G_MAXINT,
                                                             0,
                                                             G_PARAM_READABLE));
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

  if (view->container)
    gimp_container_view_set_container (view, NULL);

  if (view->context)
    gimp_container_view_set_context (view, NULL);

  if (view->hash_table)
    {
      g_hash_table_destroy (view->hash_table);
      view->hash_table = NULL;
    }

  if (GTK_OBJECT_CLASS (parent_class)->destroy)
    GTK_OBJECT_CLASS (parent_class)->destroy (object);
}

static void
gimp_container_view_style_set (GtkWidget *widget,
			       GtkStyle  *prev_style)
{
  gint content_spacing;

  gtk_widget_style_get (widget,
                        "content_spacing",
                        &content_spacing,
			NULL);

  gtk_box_set_spacing (GTK_BOX (widget), content_spacing);

  if (GTK_WIDGET_CLASS (parent_class)->style_set)
    GTK_WIDGET_CLASS (parent_class)->style_set (widget, prev_style);
}

void
gimp_container_view_set_container (GimpContainerView *view,
				   GimpContainer     *container)
{
  g_return_if_fail (GIMP_IS_CONTAINER_VIEW (view));
  g_return_if_fail (! container || GIMP_IS_CONTAINER (container));

  if (container == view->container)
    return;

  g_signal_emit (G_OBJECT (view), view_signals[SET_CONTAINER], 0,
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

      g_signal_handlers_disconnect_by_func (G_OBJECT (view->container),
					    gimp_container_view_add,
					    view);
      g_signal_handlers_disconnect_by_func (G_OBJECT (view->container),
					    gimp_container_view_remove,
					    view);
      g_signal_handlers_disconnect_by_func (G_OBJECT (view->container),
					    gimp_container_view_reorder,
					    view);

      g_hash_table_destroy (view->hash_table);

      view->hash_table = g_hash_table_new (g_direct_hash, g_direct_equal);

      if (view->context)
	{
	  g_signal_handlers_disconnect_by_func (G_OBJECT (view->context),
						gimp_container_view_context_changed,
						view);

	  gtk_drag_dest_unset (GTK_WIDGET (view));
	  gimp_dnd_viewable_dest_unset (GTK_WIDGET (view),
					view->container->children_type);
	}

      if (view->get_name_func &&
	  gimp_container_view_is_built_in_name_func (view->get_name_func))
	{
	  gimp_container_view_set_name_func (view, NULL);
	}
    }

  view->container = container;

  if (view->container)
    {
      if (! view->get_name_func)
	{
	  GimpItemGetNameFunc get_name_func;

	  get_name_func = gimp_container_view_get_built_in_name_func (view->container->children_type);

	  gimp_container_view_set_name_func (view, get_name_func);
	}

      gimp_container_foreach (view->container,
			      (GFunc) gimp_container_view_add_foreach,
			      view);

      g_signal_connect_object (G_OBJECT (view->container), "add",
			       G_CALLBACK (gimp_container_view_add),
			       G_OBJECT (view),
			       G_CONNECT_SWAPPED);

      g_signal_connect_object (G_OBJECT (view->container), "remove",
			       G_CALLBACK (gimp_container_view_remove),
			       G_OBJECT (view),
			       G_CONNECT_SWAPPED);

      g_signal_connect_object (G_OBJECT (view->container), "reorder",
			       G_CALLBACK (gimp_container_view_reorder),
			       G_OBJECT (view),
			       G_CONNECT_SWAPPED);

      if (view->context)
	{
	  GimpObject  *object;
	  const gchar *signal_name;

	  signal_name =
	    gimp_context_type_to_signal_name (view->container->children_type);

	  g_signal_connect_object (G_OBJECT (view->context), signal_name,
				   G_CALLBACK (gimp_container_view_context_changed),
				   G_OBJECT (view),
				   0);

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
  g_return_if_fail (GIMP_IS_CONTAINER_VIEW (view));
  g_return_if_fail (! context || GIMP_IS_CONTEXT (context));

  if (context == view->context)
    return;

  if (view->context && view->container)
    {
      g_signal_handlers_disconnect_by_func (G_OBJECT (view->context),
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

      g_signal_connect_object (G_OBJECT (view->context), signal_name,
			       G_CALLBACK (gimp_container_view_context_changed),
			       G_OBJECT (view),
			       0);

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
  g_return_if_fail (GIMP_IS_CONTAINER_VIEW (view));
  g_return_if_fail (preview_size > 0 && preview_size <= 256 /* FIXME: 64 */);

  view->preview_size = preview_size;

  g_signal_emit (G_OBJECT (view), view_signals[SET_PREVIEW_SIZE], 0);
}

void
gimp_container_view_set_name_func (GimpContainerView   *view,
				   GimpItemGetNameFunc  get_name_func)
{
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

  g_return_if_fail (GIMP_IS_CONTAINER_VIEW (view));
  g_return_if_fail (! viewable || GIMP_IS_VIEWABLE (viewable));

  insert_data = g_hash_table_lookup (view->hash_table, viewable);

  g_signal_emit (G_OBJECT (view), view_signals[SELECT_ITEM], 0,
		 viewable, insert_data);
}

void
gimp_container_view_activate_item (GimpContainerView *view,
				   GimpViewable      *viewable)
{
  gpointer insert_data;

  g_return_if_fail (GIMP_IS_CONTAINER_VIEW (view));
  g_return_if_fail (GIMP_IS_VIEWABLE (viewable));

  insert_data = g_hash_table_lookup (view->hash_table, viewable);

  g_signal_emit (G_OBJECT (view), view_signals[ACTIVATE_ITEM], 0,
		 viewable, insert_data);
}

void
gimp_container_view_context_item (GimpContainerView *view,
				  GimpViewable      *viewable)
{
  gpointer insert_data;

  g_return_if_fail (GIMP_IS_CONTAINER_VIEW (view));
  g_return_if_fail (GIMP_IS_VIEWABLE (viewable));

  insert_data = g_hash_table_lookup (view->hash_table, viewable);

  g_signal_emit (G_OBJECT (view), view_signals[CONTEXT_ITEM], 0,
		 viewable, insert_data);
}

void
gimp_container_view_item_selected (GimpContainerView *view,
				   GimpViewable      *viewable)
{
  g_return_if_fail (GIMP_IS_CONTAINER_VIEW (view));
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
  g_return_if_fail (GIMP_IS_CONTAINER_VIEW (view));
  g_return_if_fail (GIMP_IS_VIEWABLE (viewable));

  gimp_container_view_activate_item (view, viewable);
}

void
gimp_container_view_item_context (GimpContainerView *view,
				  GimpViewable      *viewable)
{
  g_return_if_fail (GIMP_IS_CONTAINER_VIEW (view));
  g_return_if_fail (GIMP_IS_VIEWABLE (viewable));

  gimp_container_view_context_item (view, viewable);
}

static void
gimp_container_view_clear_items (GimpContainerView *view)
{
  g_signal_emit (G_OBJECT (view), view_signals[CLEAR_ITEMS], 0);
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

  g_signal_emit (G_OBJECT (view), view_signals[INSERT_ITEM], 0,
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

  g_signal_emit (G_OBJECT (view), view_signals[INSERT_ITEM], 0,
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

      g_signal_emit (G_OBJECT (view), view_signals[REMOVE_ITEM], 0,
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
      g_signal_emit (G_OBJECT (view), view_signals[REORDER_ITEM], 0,
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

  g_signal_emit (G_OBJECT (view), view_signals[SELECT_ITEM], 0,
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
