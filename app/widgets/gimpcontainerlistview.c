/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpcontainerlistview.c
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

#include <stdio.h>

#include <gtk/gtk.h>

#include "widgets-types.h"

#include "core/gimpcontainer.h"
#include "core/gimpcontext.h"

#include "gimpcontainerlistview.h"
#include "gimpdnd.h"
#include "gimplistitem.h"
#include "gimppreview.h"

#ifdef __GNUC__
#warning GTK_DISABLE_DEPRECATED
#endif
#undef GTK_DISABLE_DEPRECATED
#include <gtk/gtklist.h>


static void     gimp_container_list_view_class_init   (GimpContainerListViewClass *klass);
static void     gimp_container_list_view_init         (GimpContainerListView      *panel);

static gpointer gimp_container_list_view_insert_item  (GimpContainerView      *view,
						       GimpViewable           *viewable,
						       gint                    index);
static void     gimp_container_list_view_remove_item  (GimpContainerView      *view,
						       GimpViewable           *viewable,
						       gpointer                insert_data);
static void     gimp_container_list_view_reorder_item (GimpContainerView      *view,
						       GimpViewable           *viewable,
						       gint                    new_index,
						       gpointer                insert_data);
static void     gimp_container_list_view_select_item  (GimpContainerView      *view,
						       GimpViewable           *viewable,
						       gpointer                insert_data);
static void     gimp_container_list_view_clear_items  (GimpContainerView      *view);
static void     gimp_container_list_view_set_preview_size (GimpContainerView  *view);

static void    gimp_container_list_view_item_selected (GtkWidget              *widget,
						       GtkWidget              *child,
						       gpointer                data);
static gint   gimp_container_list_view_item_activated (GtkWidget              *widget,
						       GdkEventButton         *bevent,
						       gpointer                data);


static GimpContainerViewClass *parent_class = NULL;


GType
gimp_container_list_view_get_type (void)
{
  static GType view_type = 0;

  if (! view_type)
    {
      static const GTypeInfo view_info =
      {
        sizeof (GimpContainerListViewClass),
        NULL,           /* base_init */
        NULL,           /* base_finalize */
        (GClassInitFunc) gimp_container_list_view_class_init,
        NULL,           /* class_finalize */
        NULL,           /* class_data */
        sizeof (GimpContainerListView),
        0,              /* n_preallocs */
        (GInstanceInitFunc) gimp_container_list_view_init,
      };

      view_type = g_type_register_static (GIMP_TYPE_CONTAINER_VIEW,
                                          "GimpContainerListView",
                                          &view_info, 0);
    }

  return view_type;
}

static void
gimp_container_list_view_class_init (GimpContainerListViewClass *klass)
{
  GimpContainerViewClass *container_view_class;

  container_view_class = GIMP_CONTAINER_VIEW_CLASS (klass);
  
  parent_class = g_type_class_peek_parent (klass);

  container_view_class->insert_item      = gimp_container_list_view_insert_item;
  container_view_class->remove_item      = gimp_container_list_view_remove_item;
  container_view_class->reorder_item     = gimp_container_list_view_reorder_item;
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
				  GTK_POLICY_AUTOMATIC);
  gtk_box_pack_start (GTK_BOX (list_view), list_view->scrolled_win,
		      TRUE, TRUE, 0);
  gtk_widget_show (list_view->scrolled_win);

  GIMP_CONTAINER_VIEW (list_view)->dnd_widget = list_view->scrolled_win;

  list_view->gtk_list = gtk_list_new ();
  gtk_list_set_selection_mode (GTK_LIST (list_view->gtk_list),
                               GTK_SELECTION_SINGLE);
  gtk_scrolled_window_add_with_viewport
    (GTK_SCROLLED_WINDOW (list_view->scrolled_win),
     list_view->gtk_list);
  gtk_widget_show (list_view->gtk_list);

  gtk_container_set_focus_vadjustment
    (GTK_CONTAINER (list_view->gtk_list),
     gtk_scrolled_window_get_vadjustment (GTK_SCROLLED_WINDOW
					  (list_view->scrolled_win)));

  GTK_WIDGET_UNSET_FLAGS (GTK_SCROLLED_WINDOW
			  (list_view->scrolled_win)->vscrollbar,
                          GTK_CAN_FOCUS);

  g_signal_connect_object (G_OBJECT (list_view->gtk_list), "select_child",
			   G_CALLBACK (gimp_container_list_view_item_selected),
			   G_OBJECT (list_view),
			   0);

}

GtkWidget *
gimp_container_list_view_new (GimpContainer *container,
			      GimpContext   *context,
			      gint           preview_size,
                              gboolean       reorderable,
			      gint           min_items_x,
			      gint           min_items_y)
{
  GimpContainerListView *list_view;
  GimpContainerView     *view;
  gint                   window_border;

  g_return_val_if_fail (! container || GIMP_IS_CONTAINER (container), NULL);
  g_return_val_if_fail (! context || GIMP_IS_CONTEXT (context), NULL);
  g_return_val_if_fail (preview_size > 0 && preview_size <= 64, NULL);
  g_return_val_if_fail (min_items_x > 0 && min_items_x <= 64, NULL);
  g_return_val_if_fail (min_items_y > 0 && min_items_y <= 64, NULL);

  list_view = g_object_new (GIMP_TYPE_CONTAINER_LIST_VIEW, NULL);

  view = GIMP_CONTAINER_VIEW (list_view);

  view->preview_size = preview_size;
  view->reorderable  = reorderable ? TRUE : FALSE;

  window_border =
    GTK_SCROLLED_WINDOW (list_view->scrolled_win)->vscrollbar->requisition.width +
    GTK_SCROLLED_WINDOW_GET_CLASS (list_view->scrolled_win)->scrollbar_spacing +
    list_view->scrolled_win->style->xthickness * 4;

  gtk_widget_set_size_request (list_view->gtk_list->parent,
                               (preview_size + 2) * min_items_x + window_border,
                               (preview_size + 6) * min_items_y + window_border);

  if (container)
    gimp_container_view_set_container (view, container);

  gimp_container_view_set_context (view, context);

  return GTK_WIDGET (list_view);
}


/*  GimpContainerView methods  */

static gpointer
gimp_container_list_view_insert_item (GimpContainerView *view,
				      GimpViewable      *viewable,
				      gint               index)
{
  GimpContainerListView *list_view;
  GtkWidget             *list_item;
  GList                 *list;

  list_view = GIMP_CONTAINER_LIST_VIEW (view);

  list_item = gimp_list_item_new (viewable, view->preview_size);

  gimp_list_item_set_name_func (GIMP_LIST_ITEM (list_item),
				view->get_name_func);

  if (view->reorderable)
    gimp_list_item_set_reorderable (GIMP_LIST_ITEM (list_item), TRUE,
                                    view->container);

  g_signal_connect (G_OBJECT (list_item), "button_press_event",
		    G_CALLBACK (gimp_container_list_view_item_activated),
		    list_view);

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
gimp_container_list_view_reorder_item (GimpContainerView *view,
				       GimpViewable      *viewable,
				       gint               new_index,
				       gpointer           insert_data)
{
  GimpContainerListView *list_view;
  GtkWidget             *list_item;
  gboolean               selected;

  list_view = GIMP_CONTAINER_LIST_VIEW (view);

  if (insert_data)
    list_item = GTK_WIDGET (insert_data);
  else
    list_item = NULL;

  if (list_item)
    {
      GList *list;

      list = g_list_prepend (NULL, list_item);

      selected = GTK_WIDGET_STATE (list_item) == GTK_STATE_SELECTED;

      g_object_ref (G_OBJECT (list_item));

      gtk_list_remove_items (GTK_LIST (list_view->gtk_list), list);

      if (new_index == -1 || new_index == view->container->num_children - 1)
	gtk_list_append_items (GTK_LIST (list_view->gtk_list), list);
      else
	gtk_list_insert_items (GTK_LIST (list_view->gtk_list), list, new_index);

      if (selected)
	gimp_container_view_select_item (view, viewable);

      g_object_unref (G_OBJECT (list_item));
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

      item_height = list_item->requisition.height;

      index = gimp_container_get_child_index (view->container,
					      GIMP_OBJECT (viewable));

      g_signal_handlers_block_by_func (G_OBJECT (list_view->gtk_list),
				       gimp_container_list_view_item_selected,
				       list_view);

      gtk_list_select_child (GTK_LIST (list_view->gtk_list), list_item);

      g_signal_handlers_unblock_by_func (G_OBJECT (list_view->gtk_list),
					 gimp_container_list_view_item_selected,
					 list_view);

      if (index * item_height < adj->value)
	{
	  gtk_adjustment_set_value (adj, index * item_height);
	}
      else if ((index + 1) * item_height > adj->value + adj->page_size)
	{
	  gtk_adjustment_set_value (adj,
				    (index + 1) * item_height - adj->page_size);
	}
    }
  else
    {
      gtk_list_unselect_all (GTK_LIST (list_view->gtk_list));
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
      gimp_list_item_set_preview_size (GIMP_LIST_ITEM (list->data),
				       view->preview_size);
    }

  gtk_widget_queue_resize (list_view->gtk_list);
}


/*  GtkClist callbacks  */

static void
gimp_container_list_view_item_selected (GtkWidget *widget,
					GtkWidget *child,
					gpointer   data)
{
  GimpViewable *viewable;

  viewable = GIMP_PREVIEW (GIMP_LIST_ITEM (child)->preview)->viewable;

  gimp_container_view_item_selected (GIMP_CONTAINER_VIEW (data), viewable);
}


/*  GtkListItem callbacks  */

static gint
gimp_container_list_view_item_activated (GtkWidget      *widget,
					 GdkEventButton *bevent,
					 gpointer        data)
{
  GimpViewable *viewable;

  viewable = GIMP_PREVIEW (GIMP_LIST_ITEM (widget)->preview)->viewable;

  switch (bevent->button)
    {
    case 1:
      if (bevent->type == GDK_2BUTTON_PRESS)
        {
          gimp_container_view_item_activated (GIMP_CONTAINER_VIEW (data),
                                              viewable);

          return TRUE;
        }
      else if (widget->state == GTK_STATE_SELECTED)
        {
          return TRUE;
        }
      break;

    case 3:
      gimp_container_view_item_selected (GIMP_CONTAINER_VIEW (data),
					 viewable);
      gimp_container_view_item_context (GIMP_CONTAINER_VIEW (data),
					viewable);
      return TRUE;
    }

  return FALSE;
}
