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

#include <stdio.h>

#include <gtk/gtk.h>

#include "apptypes.h"

#include "appenv.h"
#include "gimpcontainer.h"
#include "gimpcontainerlistview.h"
#include "gimpcontext.h"
#include "gimplist.h"
#include "gimppreview.h"


static void     gimp_container_list_view_class_init   (GimpContainerListViewClass *klass);
static void     gimp_container_list_view_init         (GimpContainerListView      *panel);
static void     gimp_container_list_view_destroy      (GtkObject                  *object);

static gpointer gimp_container_list_view_insert_item  (GimpContainerView      *view,
						       GimpViewable           *viewable,
						       gint                    index);
static void     gimp_container_list_view_remove_item  (GimpContainerView      *view,
						       GimpViewable           *viewable,
						       gpointer                insert_data);
static void     gimp_container_list_view_select_item  (GimpContainerView      *view,
						       GimpViewable           *viewable,
						       gpointer                insert_data);
static void     gimp_container_list_view_clear_items  (GimpContainerView      *view);
static void     gimp_container_list_view_set_preview_size (GimpContainerView  *view);

static void     gimp_container_list_view_name_changed (GimpContainerListView  *list_view,
						       GimpViewable           *viewable);
static void    gimp_container_list_view_item_selected (GtkWidget              *widget,
						       GtkWidget              *child,
						       gpointer                data);


static GimpContainerViewClass *parent_class = NULL;


GtkType
gimp_container_list_view_get_type (void)
{
  static guint list_view_type = 0;

  if (! list_view_type)
    {
      GtkTypeInfo list_view_info =
      {
	"GimpContainerListView",
	sizeof (GimpContainerListView),
	sizeof (GimpContainerListViewClass),
	(GtkClassInitFunc) gimp_container_list_view_class_init,
	(GtkObjectInitFunc) gimp_container_list_view_init,
	/* reserved_1 */ NULL,
	/* reserved_2 */ NULL,
        (GtkClassInitFunc) NULL
      };

      list_view_type = gtk_type_unique (GIMP_TYPE_CONTAINER_VIEW,
					&list_view_info);
    }

  return list_view_type;
}

static void
gimp_container_list_view_class_init (GimpContainerListViewClass *klass)
{
  GtkObjectClass         *object_class;
  GimpContainerViewClass *container_view_class;

  object_class         = (GtkObjectClass *) klass;
  container_view_class = (GimpContainerViewClass *) klass;
  
  parent_class = gtk_type_class (GIMP_TYPE_CONTAINER_VIEW);

  object_class->destroy = gimp_container_list_view_destroy;

  container_view_class->insert_item      = gimp_container_list_view_insert_item;
  container_view_class->remove_item      = gimp_container_list_view_remove_item;
  container_view_class->select_item      = gimp_container_list_view_select_item;
  container_view_class->clear_items      = gimp_container_list_view_clear_items;
  container_view_class->set_preview_size = gimp_container_list_view_set_preview_size;
}

static void
gimp_container_list_view_init (GimpContainerListView *list_view)
{
  GtkWidget *scrolled_win;

  scrolled_win = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_win), 
                                  GTK_POLICY_AUTOMATIC,
				  GTK_POLICY_ALWAYS);
  gtk_box_pack_start (GTK_BOX (list_view), scrolled_win, TRUE, TRUE, 0);

  list_view->gtk_list = gtk_list_new ();
  gtk_scrolled_window_add_with_viewport (GTK_SCROLLED_WINDOW (scrolled_win),
                                         list_view->gtk_list);

  gtk_list_set_selection_mode (GTK_LIST (list_view->gtk_list),
                               GTK_SELECTION_BROWSE);

  gtk_container_set_focus_vadjustment
    (GTK_CONTAINER (list_view->gtk_list),
     gtk_scrolled_window_get_vadjustment (GTK_SCROLLED_WINDOW (scrolled_win)));

  GTK_WIDGET_UNSET_FLAGS (GTK_SCROLLED_WINDOW (scrolled_win)->vscrollbar,
                          GTK_CAN_FOCUS);

  gtk_signal_connect (GTK_OBJECT (list_view->gtk_list), "select_child",
		      GTK_SIGNAL_FUNC (gimp_container_list_view_item_selected),
		      list_view);

  gtk_widget_show (list_view->gtk_list);
  gtk_widget_show (scrolled_win);
}

static void
gimp_container_list_view_destroy (GtkObject *object)
{
  GimpContainerListView *list_view;

  list_view = GIMP_CONTAINER_LIST_VIEW (object);

  if (GTK_OBJECT_CLASS (parent_class)->destroy)
    GTK_OBJECT_CLASS (parent_class)->destroy (object);
}

GtkWidget *
gimp_container_list_view_new (GimpContainer *container,
			      GimpContext   *context,
			      gint           preview_width,
			      gint           preview_height,
			      gint           min_items_x,
			      gint           min_items_y)
{
  GimpContainerListView *list_view;
  GimpContainerView     *view;

  g_return_val_if_fail (container != NULL, NULL);
  g_return_val_if_fail (GIMP_IS_CONTAINER (container), NULL);
  g_return_val_if_fail (! context || GIMP_IS_CONTEXT (context), NULL);
  g_return_val_if_fail (preview_width  > 0 && preview_width  <= 64, NULL);
  g_return_val_if_fail (preview_height > 0 && preview_height <= 64, NULL);
  g_return_val_if_fail (min_items_x > 0 && min_items_x <= 64, NULL);
  g_return_val_if_fail (min_items_y > 0 && min_items_y <= 64, NULL);

  list_view = gtk_type_new (GIMP_TYPE_CONTAINER_LIST_VIEW);

  view = GIMP_CONTAINER_VIEW (list_view);

  view->preview_width  = preview_width;
  view->preview_height = preview_height;

  gtk_widget_set_usize (list_view->gtk_list->parent,
			preview_width  * min_items_x,
			preview_height * min_items_y);

  gimp_container_view_set_container (view, container);

  gimp_container_view_set_context (view, context);

  return GTK_WIDGET (list_view);
}

static gpointer
gimp_container_list_view_insert_item (GimpContainerView *view,
				      GimpViewable      *viewable,
				      gint               index)
{
  GimpContainerListView *list_view;
  GtkWidget             *list_item;
  GtkWidget             *hbox;
  GtkWidget             *preview;
  GtkWidget             *label;
  GList                 *list;

  list_view = GIMP_CONTAINER_LIST_VIEW (view);

  list_item = gtk_list_item_new ();

  gtk_signal_connect_object_while_alive
    (GTK_OBJECT (viewable), "name_changed",
     GTK_SIGNAL_FUNC (gimp_container_list_view_name_changed),
     GTK_OBJECT (list_view));

  hbox = gtk_hbox_new (FALSE, 8);
  gtk_container_add (GTK_CONTAINER (list_item), hbox);
  gtk_widget_show (hbox);

  preview = gimp_preview_new (viewable,
			      view->preview_width,
			      view->preview_height,
			      FALSE,
			      FALSE);
  gtk_box_pack_start (GTK_BOX (hbox), preview, FALSE, FALSE, 0);
  gtk_widget_show (preview);

  gtk_object_set_data (GTK_OBJECT (list_item), "preview", preview);

  label = gtk_label_new (gimp_object_get_name (GIMP_OBJECT (viewable)));
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
  gtk_widget_show (label);

  gtk_object_set_data (GTK_OBJECT (list_item), "label", label);

  gtk_widget_show (list_item);

  list = g_list_prepend (NULL, list_item);

  if (index == -1)
    gtk_list_append_items (GTK_LIST (list_view->gtk_list), list);
  else
    gtk_list_insert_items (GTK_LIST (list_view->gtk_list), list, index);

  return (gpointer) list_item;
}

static void
gimp_container_list_view_remove_item (GimpContainerView *view,
				      GimpViewable      *viewable,
				      gpointer           insert_data)
{
  GimpContainerListView *list_view;
  GtkWidget             *list_item;

  list_view = GIMP_CONTAINER_LIST_VIEW (view);
  list_item = GTK_WIDGET (insert_data);

  if (list_item)
    {
      GList *list;

      list = g_list_prepend (NULL, list_item);

      gtk_list_remove_items (GTK_LIST (list_view->gtk_list), list);
    }
}

static void
gimp_container_list_view_select_item (GimpContainerView *view,
				      GimpViewable      *viewable,
				      gpointer           insert_data)
{
  GimpContainerListView *list_view;
  GtkWidget             *list_item;

  list_view = GIMP_CONTAINER_LIST_VIEW (view);
  list_item = GTK_WIDGET (insert_data);

  if (list_item)
    {
      gtk_signal_handler_block_by_func (GTK_OBJECT (list_view->gtk_list),
					gimp_container_list_view_item_selected,
					list_view);

      gtk_list_select_child (GTK_LIST (list_view->gtk_list), list_item);

      gtk_signal_handler_unblock_by_func (GTK_OBJECT (list_view->gtk_list),
					  gimp_container_list_view_item_selected,
					  list_view);
    }
}

static void
gimp_container_list_view_clear_items (GimpContainerView *view)
{
  GimpContainerListView *list_view;

  list_view = GIMP_CONTAINER_LIST_VIEW (view);

  gtk_list_clear_items (GTK_LIST (list_view->gtk_list), 0, -1);
}

static void
gimp_container_list_view_set_preview_size (GimpContainerView  *view)
{
  GimpContainerListView *list_view;
  GList                 *list;

  list_view = GIMP_CONTAINER_LIST_VIEW (view);

  for (list = GTK_LIST (list_view->gtk_list)->children;
       list;
       list = g_list_next (list))
    {
      GimpPreview *preview;

      preview = gtk_object_get_data (GTK_OBJECT (list->data), "preview");

      gtk_preview_size (GTK_PREVIEW (preview),
			view->preview_width,
			view->preview_height);
    }

  gtk_widget_queue_resize (list_view->gtk_list);
}

static void
gimp_container_list_view_name_changed (GimpContainerListView *list_view,
				       GimpViewable          *viewable)
{
  GtkWidget *list_item;

  list_item = g_hash_table_lookup (GIMP_CONTAINER_VIEW (list_view)->hash_table,
				   viewable);

  if (list_item)
    {
      GtkWidget *label;

      label = gtk_object_get_data (GTK_OBJECT (list_item), "label");

      if (label)
	{
	  gtk_label_set_text (GTK_LABEL (label),
			      gimp_object_get_name (GIMP_OBJECT (viewable)));
	}
    }
}

static void
gimp_container_list_view_item_selected (GtkWidget *widget,
					GtkWidget *child,
					gpointer   data)
{
  GimpViewable *viewable;

  viewable = GIMP_PREVIEW (gtk_object_get_data (GTK_OBJECT (child),
						"preview"))->viewable;

  gimp_container_view_item_selected (GIMP_CONTAINER_VIEW (data),
				     viewable);
}
