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

#include "config.h"

#include <gtk/gtk.h>

#include "apptypes.h"

#include "gimpcontainer.h"
#include "gimpcontainerview.h"
#include "gimpmarshal.h"


enum
{
  INSERT_ITEM,
  REMOVE_ITEM,
  CLEAR_ITEMS,
  SET_PREVIEW_SIZE,
  LAST_SIGNAL
};


static void   gimp_container_view_class_init  (GimpContainerViewClass *klass);
static void   gimp_container_view_init        (GimpContainerView      *panel);
static void   gimp_container_view_destroy     (GtkObject              *object);

static void   gimp_container_view_clear       (GimpContainerView      *view);

static void   gimp_container_view_add_foreach (GimpViewable           *viewable,
					       GimpContainerView      *view);
static void   gimp_container_view_add         (GimpContainerView      *view,
					       GimpViewable           *viewable,
					       GimpContainer          *container);
static void   gimp_container_view_remove      (GimpContainerView      *view,
					       GimpViewable           *viewable,
					       GimpContainer          *container);


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
}

static void
gimp_container_view_init (GimpContainerView *view)
{
  view->container  = NULL;
  view->hash_table = NULL;

  view->preview_width  = 0;
  view->preview_height = 0;
}

static void
gimp_container_view_destroy (GtkObject *object)
{
  GimpContainerView *view;

  view = GIMP_CONTAINER_VIEW (object);

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

  if (view->container)
    {
      gtk_signal_disconnect_by_func (GTK_OBJECT (container),
				     gimp_container_view_add,
				     view);
      gtk_signal_disconnect_by_func (GTK_OBJECT (container),
				     gimp_container_view_remove,
				     view);

      g_hash_table_destroy (view->hash_table);

      gimp_container_view_clear (view);
    }

  view->container = container;

  if (view->container)
    {
      view->hash_table = g_hash_table_new (g_direct_hash, g_direct_equal);

      gimp_container_foreach (container,
			      (GFunc) gimp_container_view_add_foreach,
			      view);

      gtk_signal_connect_object_while_alive
	(GTK_OBJECT (container), "add",
	 GTK_SIGNAL_FUNC (gimp_container_view_add),
	 GTK_OBJECT (view));

      gtk_signal_connect_object_while_alive
	(GTK_OBJECT (container), "remove",
	 GTK_SIGNAL_FUNC (gimp_container_view_remove),
	 GTK_OBJECT (view));
    }
}

void
gimp_container_view_set_preview_size (GimpContainerView *view,
				      gint               width,
				      gint               height)
{
  g_return_if_fail (view != NULL);
  g_return_if_fail (GIMP_IS_CONTAINER_VIEW (view));
  g_return_if_fail (width  > 0 && width  <= 256 /* FIXME: 64 */);
  g_return_if_fail (height > 0 && height <= 256 /* FIXME: 64 */);

  view->preview_width  = width;
  view->preview_height = height;

  gtk_signal_emit (GTK_OBJECT (view), view_signals[SET_PREVIEW_SIZE]);
}

static void
gimp_container_view_clear (GimpContainerView *view)
{
  gtk_signal_emit (GTK_OBJECT (view), view_signals[CLEAR_ITEMS]);
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
