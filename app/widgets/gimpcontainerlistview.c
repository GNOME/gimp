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
#include "gimplist.h"
#include "gimppreview.h"


static void   gimp_container_list_view_class_init   (GimpContainerListViewClass *klass);
static void   gimp_container_list_view_init         (GimpContainerListView      *panel);
static void   gimp_container_list_view_destroy      (GtkObject                  *object);

static void   gimp_container_list_view_add_foreach  (GimpViewable               *viewable,
						     GimpContainerListView      *list_view);
static void   gimp_container_list_view_add          (GimpContainerListView      *list_view,
						     GimpViewable               *viewable,
						     GimpContainer              *container);
static void   gimp_container_list_view_remove       (GimpContainerListView      *list_view,
						     GimpViewable               *viewable,
						     GimpContainer              *container);
static void   gimp_container_list_view_name_changed (GimpContainerListView      *list_view,
						     GimpViewable               *viewable);


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

  gtk_widget_show (list_view->gtk_list);
  gtk_widget_show (scrolled_win);

  list_view->hash_table = g_hash_table_new (g_direct_hash, g_direct_equal);
}

static void
gimp_container_list_view_destroy (GtkObject *object)
{
  GimpContainerListView *list_view;

  list_view = GIMP_CONTAINER_LIST_VIEW (object);

  g_hash_table_destroy (list_view->hash_table);

  if (GTK_OBJECT_CLASS (parent_class)->destroy)
    GTK_OBJECT_CLASS (parent_class)->destroy (object);
}

GtkWidget *
gimp_container_list_view_new (GimpContainer *container)
{
  GimpContainerListView *list_view;
  GimpContainerView     *view;

  g_return_val_if_fail (container != NULL, NULL);
  g_return_val_if_fail (GIMP_IS_CONTAINER (container), NULL);

  list_view = gtk_type_new (GIMP_TYPE_CONTAINER_LIST_VIEW);

  view = GIMP_CONTAINER_VIEW (list_view);

  view->container = container;

  gimp_container_foreach (container,
			  (GFunc) gimp_container_list_view_add_foreach,
			  list_view);

  gtk_signal_connect_object_while_alive (GTK_OBJECT (container), "add",
					 GTK_SIGNAL_FUNC (gimp_container_list_view_add),
					 GTK_OBJECT (list_view));

  gtk_signal_connect_object_while_alive (GTK_OBJECT (container), "remove",
					 GTK_SIGNAL_FUNC (gimp_container_list_view_remove),
					 GTK_OBJECT (list_view));

  return GTK_WIDGET (list_view);
}

static void
gimp_container_list_view_insert (GimpContainerListView *list_view,
				 GimpViewable          *viewable,
				 gint                   index)
{
  GtkWidget *list_item;
  GtkWidget *hbox;
  GtkWidget *preview;
  GtkWidget *label;
  GList     *list;

  list_item = gtk_list_item_new ();

  g_hash_table_insert (list_view->hash_table, viewable, list_item);

  gtk_signal_connect_object_while_alive (GTK_OBJECT (viewable), "name_changed",
					 GTK_SIGNAL_FUNC (gimp_container_list_view_name_changed),
					 GTK_OBJECT (list_view));

  hbox = gtk_hbox_new (FALSE, 8);
  gtk_container_add (GTK_CONTAINER (list_item), hbox);
  gtk_widget_show (hbox);

  preview = gimp_preview_new (viewable);
  gtk_widget_set_usize (GTK_WIDGET (preview), 64, 64);
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

}

static void
gimp_container_list_view_add_foreach (GimpViewable          *viewable,
				      GimpContainerListView *list_view)
{
  gimp_container_list_view_insert (list_view, viewable, -1);
}

static void
gimp_container_list_view_add (GimpContainerListView *list_view,
			      GimpViewable          *viewable,
			      GimpContainer         *container)
{
  gint index;

  index = gimp_container_get_child_index (container,
					  GIMP_OBJECT (viewable));

  gimp_container_list_view_insert (list_view, viewable, index);
}

static void
gimp_container_list_view_remove (GimpContainerListView *list_view,
				 GimpViewable          *viewable,
				 GimpContainer         *container)
{
  GtkWidget *list_item;

  list_item = g_hash_table_lookup (list_view->hash_table, viewable);

  if (list_item)
    {
      GList *list;

      list = g_list_prepend (NULL, list_item);

      gtk_list_remove_items (GTK_LIST (list_view->gtk_list), list);

      g_hash_table_remove (list_view->hash_table, viewable);
    }
}

static void
gimp_container_list_view_name_changed (GimpContainerListView *list_view,
				       GimpViewable          *viewable)
{
  GtkWidget *list_item;

  list_item = g_hash_table_lookup (list_view->hash_table, viewable);

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
