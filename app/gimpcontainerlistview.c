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

static void     gimp_container_list_view_name_changed (GimpViewable           *viewable,
						       GtkLabel               *label);
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
  list_view->scrolled_win = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (list_view->scrolled_win), 
                                  GTK_POLICY_AUTOMATIC,
				  GTK_POLICY_ALWAYS);
  gtk_box_pack_start (GTK_BOX (list_view), list_view->scrolled_win,
		      TRUE, TRUE, 0);

  list_view->gtk_list = gtk_list_new ();
  gtk_scrolled_window_add_with_viewport
    (GTK_SCROLLED_WINDOW (list_view->scrolled_win),
     list_view->gtk_list);

  gtk_list_set_selection_mode (GTK_LIST (list_view->gtk_list),
                               GTK_SELECTION_SINGLE);

  gtk_container_set_focus_vadjustment
    (GTK_CONTAINER (list_view->gtk_list),
     gtk_scrolled_window_get_vadjustment (GTK_SCROLLED_WINDOW
					  (list_view->scrolled_win)));

  GTK_WIDGET_UNSET_FLAGS (GTK_SCROLLED_WINDOW
			  (list_view->scrolled_win)->vscrollbar,
                          GTK_CAN_FOCUS);

  gtk_signal_connect_while_alive
    (GTK_OBJECT (list_view->gtk_list), "select_child",
     GTK_SIGNAL_FUNC (gimp_container_list_view_item_selected),
     list_view,
     GTK_OBJECT (list_view));

  gtk_widget_show (list_view->gtk_list);
  gtk_widget_show (list_view->scrolled_win);
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
			      gint           preview_size,
			      gint           min_items_x,
			      gint           min_items_y)
{
  GimpContainerListView *list_view;
  GimpContainerView     *view;
  gint                   window_border;

  g_return_val_if_fail (container != NULL, NULL);
  g_return_val_if_fail (GIMP_IS_CONTAINER (container), NULL);
  g_return_val_if_fail (! context || GIMP_IS_CONTEXT (context), NULL);
  g_return_val_if_fail (preview_size > 0 && preview_size <= 64, NULL);
  g_return_val_if_fail (min_items_x > 0 && min_items_x <= 64, NULL);
  g_return_val_if_fail (min_items_y > 0 && min_items_y <= 64, NULL);

  list_view = gtk_type_new (GIMP_TYPE_CONTAINER_LIST_VIEW);

  view = GIMP_CONTAINER_VIEW (list_view);

  view->preview_size = preview_size;

  window_border =
    GTK_SCROLLED_WINDOW (list_view->scrolled_win)->vscrollbar->requisition.width +
    GTK_SCROLLED_WINDOW_CLASS (GTK_OBJECT (list_view->scrolled_win)->klass)->scrollbar_spacing +
    list_view->scrolled_win->style->klass->xthickness * 4;

  gtk_widget_set_usize (list_view->gtk_list->parent,
			(preview_size + 2) * min_items_x + window_border,
			(preview_size + 6) * min_items_y + window_border);

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

  hbox = gtk_hbox_new (FALSE, 8);
  gtk_container_set_border_width (GTK_CONTAINER (hbox), 2);
  gtk_container_add (GTK_CONTAINER (list_item), hbox);
  gtk_widget_show (hbox);

  preview = gimp_preview_new (viewable, view->preview_size, 1);
  gtk_box_pack_start (GTK_BOX (hbox), preview, FALSE, FALSE, 0);
  gtk_widget_show (preview);

  gtk_object_set_data (GTK_OBJECT (list_item), "preview", preview);

  label = gtk_label_new (gimp_object_get_name (GIMP_OBJECT (viewable)));
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
  gtk_widget_show (label);

  gtk_object_set_data (GTK_OBJECT (list_item), "label", label);

  gtk_signal_connect_while_alive
    (GTK_OBJECT (viewable), "name_changed",
     GTK_SIGNAL_FUNC (gimp_container_list_view_name_changed),
     label,
     GTK_OBJECT (list_view));

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

  if (insert_data)
    list_item = GTK_WIDGET (insert_data);
  else
    list_item = NULL;

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

  if (insert_data)
    list_item = GTK_WIDGET (insert_data);
  else
    list_item = NULL;

  if (list_item)
    {
      GtkAdjustment *adj;
      gint           item_height;
      gint           index;

      adj = gtk_scrolled_window_get_vadjustment
	(GTK_SCROLLED_WINDOW (list_view->scrolled_win));

      item_height = list_item->allocation.height;

      index = gimp_container_get_child_index (view->container,
					      GIMP_OBJECT (viewable));

      gtk_signal_handler_block_by_func (GTK_OBJECT (list_view->gtk_list),
					gimp_container_list_view_item_selected,
					list_view);

      gtk_list_select_child (GTK_LIST (list_view->gtk_list), list_item);

      if (index * item_height < adj->value)
	{
	  gtk_adjustment_set_value (adj, index * item_height);
	}
      else if ((index + 1) * item_height > adj->value + adj->page_size)
	{
	  gtk_adjustment_set_value (adj,
				    (index + 1) * item_height - adj->page_size);
	}

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

  if (GIMP_CONTAINER_VIEW_CLASS (parent_class)->clear_items)
    GIMP_CONTAINER_VIEW_CLASS (parent_class)->clear_items (view);
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

      gimp_preview_set_size (preview, view->preview_size);
    }

  gtk_widget_queue_resize (list_view->gtk_list);
}

static void
gimp_container_list_view_name_changed (GimpViewable *viewable,
				       GtkLabel     *label)
{
  gtk_label_set_text (label, gimp_object_get_name (GIMP_OBJECT (viewable)));
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
