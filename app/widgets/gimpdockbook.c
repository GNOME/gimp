/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpdockbook.c
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

#include "libgimpwidgets/gimpwidgets.h"

#include "widgets-types.h"

#include "gimpdialogfactory.h"
#include "gimpdnd.h"
#include "gimpdock.h"
#include "gimpdockable.h"
#include "gimpdockbook.h"


#define TAB_WIDGET_SIZE     24
#define MENU_WIDGET_SIZE    16
#define MENU_WIDGET_SPACING  4


static void      gimp_dockbook_class_init       (GimpDockbookClass  *klass);
static void      gimp_dockbook_init             (GimpDockbook       *dockbook);

static void      gimp_dockbook_destroy          (GtkObject          *object);
static gboolean  gimp_dockbook_drag_drop        (GtkWidget          *widget,
						 GdkDragContext     *context,
						 gint                x,
						 gint                y,
						 guint               time);

static void      gimp_dockbook_menu_switch_page (GtkWidget          *widget,
						 GimpDockable       *dockable);
static void      gimp_dockbook_menu_end         (GimpDockbook       *dockbook);
static void      gimp_dockbook_menu_detacher    (GtkWidget          *widget,
						 GtkMenu            *menu);
static gboolean  gimp_dockbook_tab_button_press (GtkWidget          *widget,
						 GdkEventButton     *bevent,
						 gpointer            data);
static void      gimp_dockbook_tab_drag_begin   (GtkWidget          *widget,
						 GdkDragContext     *context,
						 gpointer            data);
static void      gimp_dockbook_tab_drag_end     (GtkWidget          *widget,
						 GdkDragContext     *context,
						 gpointer            data);
static gboolean  gimp_dockbook_tab_drag_drop    (GtkWidget          *widget,
						 GdkDragContext     *context,
						 gint                x,
						 gint                y,
						 guint               time,
						 gpointer            data);


static GtkNotebookClass *parent_class = NULL;

static GtkTargetEntry dialog_target_table[] =
{
  GIMP_TARGET_DIALOG
};
static guint n_dialog_targets = (sizeof (dialog_target_table) /
				 sizeof (dialog_target_table[0]));


GtkType
gimp_dockbook_get_type (void)
{
  static GtkType dockbook_type = 0;

  if (! dockbook_type)
    {
      static const GtkTypeInfo dockbook_info =
      {
	"GimpDockbook",
	sizeof (GimpDockbook),
	sizeof (GimpDockbookClass),
	(GtkClassInitFunc) gimp_dockbook_class_init,
	(GtkObjectInitFunc) gimp_dockbook_init,
	/* reserved_1 */ NULL,
        /* reserved_2 */ NULL,
        (GtkClassInitFunc) NULL,
      };

      dockbook_type = gtk_type_unique (GTK_TYPE_NOTEBOOK, &dockbook_info);
    }

  return dockbook_type;
}

static void
gimp_dockbook_class_init (GimpDockbookClass *klass)
{
  GtkObjectClass *object_class;
  GtkWidgetClass *widget_class;

  object_class = (GtkObjectClass *) klass;
  widget_class = (GtkWidgetClass *) klass;

  parent_class = gtk_type_class (GTK_TYPE_NOTEBOOK);

  object_class->destroy   = gimp_dockbook_destroy;

  widget_class->drag_drop = gimp_dockbook_drag_drop;
}

static void
gimp_dockbook_init (GimpDockbook *dockbook)
{
  dockbook->dock = NULL;

  gtk_notebook_set_tab_border (GTK_NOTEBOOK (dockbook), 0);
  gtk_notebook_popup_enable (GTK_NOTEBOOK (dockbook));
  gtk_notebook_set_scrollable (GTK_NOTEBOOK (dockbook), TRUE);

  gtk_drag_dest_set (GTK_WIDGET (dockbook),
                     GTK_DEST_DEFAULT_ALL,
                     dialog_target_table, n_dialog_targets,
                     GDK_ACTION_MOVE);
}

GtkWidget *
gimp_dockbook_new (void)
{
  return GTK_WIDGET (gtk_type_new (GIMP_TYPE_DOCKBOOK));
}

static void
gimp_dockbook_destroy (GtkObject *object)
{
  if (GTK_OBJECT_CLASS (parent_class))
    GTK_OBJECT_CLASS (parent_class)->destroy (object);
}

static gboolean
gimp_dockbook_drag_drop (GtkWidget      *widget,
			 GdkDragContext *context,
			 gint            x,
			 gint            y,
			 guint           time)
{
  GtkWidget *source;

  source = gtk_drag_get_source_widget (context);

  if (source)
    {
      GimpDockable *dockable;

      dockable = (GimpDockable *) gtk_object_get_data (GTK_OBJECT (source),
						       "gimp-dockable");

      if (dockable)
	{
	  gtk_object_set_data (GTK_OBJECT (dockable),
			       "gimp-dock-drag-widget", NULL);

	  if (dockable->dockbook != GIMP_DOCKBOOK (widget))
	    {
	      gtk_object_ref (GTK_OBJECT (dockable));

	      gimp_dockbook_remove (dockable->dockbook, dockable);
	      gimp_dockbook_add (GIMP_DOCKBOOK (widget), dockable, -1);

	      gtk_object_unref (GTK_OBJECT (dockable));

	      return TRUE;
	    }
	}
    }

  return FALSE;
}

void
gimp_dockbook_add (GimpDockbook *dockbook,
		   GimpDockable *dockable,
		   gint          position)
{
  GtkWidget *tab_widget;
  GtkWidget *menu_widget;

  g_return_if_fail (dockbook != NULL);
  g_return_if_fail (GIMP_IS_DOCKBOOK (dockbook));

  g_return_if_fail (dockbook->dock != NULL);

  g_return_if_fail (dockable != NULL);
  g_return_if_fail (GIMP_IS_DOCKABLE (dockable));

  g_return_if_fail (dockable->dockbook == NULL);

  tab_widget = gimp_dockable_get_tab_widget (dockable, dockbook,
					     TAB_WIDGET_SIZE);

  if (GTK_IS_LABEL (tab_widget))
    {
      GtkWidget *event_box;

      event_box = gtk_event_box_new ();
      gtk_container_add (GTK_CONTAINER (event_box), tab_widget);
      gtk_widget_show (tab_widget);

      tab_widget = event_box;
    }

  gimp_help_set_help_data (tab_widget, dockable->name, NULL);

  gtk_object_set_data (GTK_OBJECT (tab_widget), "gimp-dockable", dockable);

  gtk_signal_connect (GTK_OBJECT (tab_widget), "button_press_event",
		      GTK_SIGNAL_FUNC (gimp_dockbook_tab_button_press),
		      dockable);

  menu_widget = gimp_dockable_get_tab_widget (dockable, dockbook,
					      MENU_WIDGET_SIZE);

  if (! GTK_IS_LABEL (menu_widget))
    {
      GtkWidget *hbox;
      GtkWidget *label;

      hbox = gtk_hbox_new (FALSE, MENU_WIDGET_SPACING);

      gtk_box_pack_start (GTK_BOX (hbox), menu_widget, FALSE, FALSE, 0);
      gtk_widget_show (menu_widget);

      label = gtk_label_new (dockable->name);
      gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
      gtk_widget_show (label);

      menu_widget = hbox;
    }
  else
    {
      gtk_widget_destroy (menu_widget);

      menu_widget = gtk_label_new (dockable->name);
      gtk_misc_set_alignment (GTK_MISC (menu_widget), 0.0, 0.5);
    }

  if (position == -1)
    {
      gtk_notebook_append_page_menu (GTK_NOTEBOOK (dockbook),
				     GTK_WIDGET (dockable),
				     tab_widget,
				     menu_widget);
    }
  else
    {
      gtk_notebook_insert_page_menu (GTK_NOTEBOOK (dockbook),
				     GTK_WIDGET (dockable),
				     tab_widget,
				     menu_widget,
				     position);
    }

  {
    GtkWidget *menu_item;
    GList     *list;

    menu_item = menu_widget->parent;

    for (list = GTK_NOTEBOOK (dockbook)->children;
	 list;
	 list = g_list_next (list))
      {
	GtkNotebookPage *page;

	page = (GtkNotebookPage *) list->data;

	if (page->child == (GtkWidget *) dockable)
	  {
	    gtk_signal_disconnect_by_data (GTK_OBJECT (menu_item), page);

	    gtk_signal_connect (GTK_OBJECT (menu_item), "activate",
				GTK_SIGNAL_FUNC (gimp_dockbook_menu_switch_page),
				dockable);

	    break;
	  }
      }
  }

  gtk_widget_show (GTK_WIDGET (dockable));

  gtk_drag_source_set (GTK_WIDGET (tab_widget),
		       GDK_BUTTON1_MASK | GDK_BUTTON2_MASK,
		       dialog_target_table, n_dialog_targets,
		       GDK_ACTION_MOVE);
  gtk_signal_connect (GTK_OBJECT (tab_widget), "drag_begin",
		      GTK_SIGNAL_FUNC (gimp_dockbook_tab_drag_begin),
		      dockable);
  gtk_signal_connect (GTK_OBJECT (tab_widget), "drag_end",
		      GTK_SIGNAL_FUNC (gimp_dockbook_tab_drag_end),
		      dockable);

  gtk_drag_dest_set (GTK_WIDGET (tab_widget),
                     GTK_DEST_DEFAULT_ALL,
                     dialog_target_table, n_dialog_targets,
                     GDK_ACTION_MOVE);
  gtk_signal_connect (GTK_OBJECT (tab_widget), "drag_drop",
                      GTK_SIGNAL_FUNC (gimp_dockbook_tab_drag_drop),
                      dockbook);

  dockable->dockbook = dockbook;

  gimp_dockable_set_context (dockable, dockbook->dock->context);
}

void
gimp_dockbook_remove (GimpDockbook *dockbook,
		      GimpDockable *dockable)
{
  g_return_if_fail (dockbook != NULL);
  g_return_if_fail (GIMP_IS_DOCKBOOK (dockbook));

  g_return_if_fail (dockable != NULL);
  g_return_if_fail (GIMP_IS_DOCKABLE (dockable));

  g_return_if_fail (dockable->dockbook != NULL);
  g_return_if_fail (dockable->dockbook == dockbook);

  dockable->dockbook = NULL;

  gimp_dockable_set_context (dockable, NULL);

  gtk_container_remove (GTK_CONTAINER (dockbook), GTK_WIDGET (dockable));

  if (! g_list_length (gtk_container_children (GTK_CONTAINER (dockbook))))
    {
      gimp_dock_remove_book (dockbook->dock, dockbook);
    }
}

static void
gimp_dockbook_menu_switch_page (GtkWidget    *widget,
				GimpDockable *dockable)
{
  gint page_num;

  page_num = gtk_notebook_page_num (GTK_NOTEBOOK (dockable->dockbook),
				    GTK_WIDGET (dockable));

  gtk_notebook_set_page (GTK_NOTEBOOK (dockable->dockbook), page_num);
}

static void
gimp_dockbook_menu_end (GimpDockbook *dockbook)
{
  GtkWidget *notebook_menu;

  notebook_menu = GTK_NOTEBOOK (dockbook)->menu;

  gtk_object_ref (GTK_OBJECT (notebook_menu));
  gtk_menu_detach (GTK_MENU (notebook_menu));

  gtk_menu_attach_to_widget (GTK_MENU (notebook_menu),
			     GTK_WIDGET (dockbook),
			     gimp_dockbook_menu_detacher);

  gtk_object_unref (GTK_OBJECT (notebook_menu));

  /*  release gimp_dockbook_tab_button_press()'s reference  */
  gtk_object_unref (GTK_OBJECT (dockbook));
}

static void
gimp_dockbook_menu_detacher (GtkWidget *widget,
			     GtkMenu   *menu)
{
  GtkNotebook *notebook;

  g_return_if_fail (widget != NULL);
  g_return_if_fail (GTK_IS_NOTEBOOK (widget));

  notebook = GTK_NOTEBOOK (widget);
  g_return_if_fail (notebook->menu == (GtkWidget*) menu);

  notebook->menu = NULL;
}

static gboolean
gimp_dockbook_tab_button_press (GtkWidget      *widget,
				GdkEventButton *bevent,
				gpointer        data)
{
  GimpDockable *dockable;
  GimpDockbook *dockbook;
  gint          page_num;

  dockable = GIMP_DOCKABLE (data);
  dockbook = dockable->dockbook;

  page_num = gtk_notebook_page_num (GTK_NOTEBOOK (dockbook),
				    GTK_WIDGET (dockable));

  gtk_notebook_set_page (GTK_NOTEBOOK (dockbook), page_num);

  if (bevent->button == 3)
    {
      GtkItemFactory *ifactory;
      GtkWidget      *add_widget;
      GtkWidget      *notebook_menu;
      gint            origin_x;
      gint            origin_y;
      gint            x, y;

      ifactory      = GTK_ITEM_FACTORY (dockbook->dock->factory->item_factory);
      add_widget    = gtk_item_factory_get_widget (ifactory, "/Select Tab");
      notebook_menu = GTK_NOTEBOOK (dockbook)->menu;

      gtk_object_ref (GTK_OBJECT (notebook_menu));
      gtk_menu_detach (GTK_MENU (notebook_menu));

      GTK_NOTEBOOK (dockbook)->menu = notebook_menu;

      gtk_menu_item_set_submenu (GTK_MENU_ITEM (add_widget), notebook_menu);

      gtk_object_unref (GTK_OBJECT (notebook_menu));

      gdk_window_get_origin (widget->window, &origin_x, &origin_y);

      x = bevent->x + origin_x;
      y = bevent->y + origin_y;

      if (x + ifactory->widget->requisition.width > gdk_screen_width ())
	x -= ifactory->widget->requisition.width;

      if (y + ifactory->widget->requisition.height > gdk_screen_height ())
	y -= ifactory->widget->requisition.height;

      /*  an item factory callback may destroy the dockbook, so reference
       *  if for gimp_dockbook_menu_end()
       */
      gtk_object_ref (GTK_OBJECT (dockbook));

      gtk_item_factory_popup_with_data (ifactory,
					dockbook,
					(GtkDestroyNotify) gimp_dockbook_menu_end,
					x, y,
					3, bevent->time);
    }

  return FALSE;
}

static void
gimp_dockbook_tab_drag_begin (GtkWidget      *widget,
			      GdkDragContext *context,
			      gpointer        data)
{
  GimpDockable *dockable;
  GtkWidget    *window;
  GtkWidget    *frame;
  GtkWidget    *preview;

  dockable = GIMP_DOCKABLE (data);

  window = gtk_window_new (GTK_WINDOW_POPUP);

  frame = gtk_frame_new (NULL);
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_OUT);
  gtk_container_add (GTK_CONTAINER (window), frame);
  gtk_widget_show (frame);

  preview = gimp_dockable_get_tab_widget (dockable, dockable->dockbook,
					  TAB_WIDGET_SIZE);

  if (! GTK_IS_LABEL (preview))
    {
      GtkWidget *hbox;
      GtkWidget *label;

      hbox = gtk_hbox_new (FALSE, 2);
      gtk_container_set_border_width (GTK_CONTAINER (hbox), 2);

      gtk_box_pack_start (GTK_BOX (hbox), preview, FALSE, FALSE, 0);
      gtk_widget_show (preview);

      label = gtk_label_new (dockable->name);
      gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 2);
      gtk_widget_show (label);

      preview = hbox;
    }
  else
    {
      gtk_widget_destroy (preview);

      preview = gtk_label_new (dockable->name);
      gtk_misc_set_padding (GTK_MISC (preview), 10, 5);
    }

  gtk_container_add (GTK_CONTAINER (frame), preview);
  gtk_widget_show (preview);

  gtk_widget_show (window);

  gtk_object_set_data_full (GTK_OBJECT (dockable), "gimp-dock-drag-widget",
			    window,
			    (GtkDestroyNotify) gtk_widget_destroy);

  gtk_drag_set_icon_widget (context, window,
			    -8, -8);
}

static void
gimp_dockbook_tab_drag_end (GtkWidget      *widget,
			    GdkDragContext *context,
			    gpointer        data)
{
  GimpDockable *dockable;
  GtkWidget    *drag_widget;

  dockable = GIMP_DOCKABLE (data);

  drag_widget = gtk_object_get_data (GTK_OBJECT (dockable),
				     "gimp-dock-drag-widget");

  if (drag_widget)
    {
      GtkWidget *dock;
      GtkWidget *dockbook;

      gtk_object_set_data (GTK_OBJECT (dockable), "gimp-dock-drag-widget", NULL);

      dock = gimp_dialog_factory_dock_new (dockable->dockbook->dock->factory);

      gtk_window_set_position (GTK_WINDOW (dock), GTK_WIN_POS_MOUSE);

      dockbook = gimp_dockbook_new ();

      gimp_dock_add_book (GIMP_DOCK (dock), GIMP_DOCKBOOK (dockbook), 0);

      gtk_object_ref (GTK_OBJECT (dockable));

      gimp_dockbook_remove (dockable->dockbook, dockable);
      gimp_dockbook_add (GIMP_DOCKBOOK (dockbook), dockable, 0);

      gtk_object_unref (GTK_OBJECT (dockable));

      gtk_widget_show (dock);
    }
}

static gboolean
gimp_dockbook_tab_drag_drop (GtkWidget      *widget,
			     GdkDragContext *context,
			     gint            x,
			     gint            y,
			     guint           time,
			     gpointer        data)
{
  GimpDockable *dest_dockable;
  GtkWidget    *source;

  dest_dockable = (GimpDockable *) gtk_object_get_data (GTK_OBJECT (widget),
							"gimp-dockable");

  source = gtk_drag_get_source_widget (context);

  if (dest_dockable && source)
    {
      GimpDockable *src_dockable;

      src_dockable = (GimpDockable *) gtk_object_get_data (GTK_OBJECT (source),
							   "gimp-dockable");

      if (src_dockable)
	{
	  gint dest_index;

	  dest_index =
	    gtk_notebook_page_num (GTK_NOTEBOOK (dest_dockable->dockbook),
				   GTK_WIDGET (dest_dockable));

	  gtk_object_set_data (GTK_OBJECT (src_dockable),
			       "gimp-dock-drag-widget", NULL);

	  if (src_dockable->dockbook != dest_dockable->dockbook)
	    {
	      gtk_object_ref (GTK_OBJECT (src_dockable));

	      gimp_dockbook_remove (src_dockable->dockbook, src_dockable);
	      gimp_dockbook_add (dest_dockable->dockbook, src_dockable,
				 dest_index);

	      gtk_object_unref (GTK_OBJECT (src_dockable));

	      return TRUE;
	    }
	  else if (src_dockable != dest_dockable)
	    {
	      gtk_notebook_reorder_child (GTK_NOTEBOOK (src_dockable->dockbook),
					  GTK_WIDGET (src_dockable),
					  dest_index);

	      return TRUE;
	    }
	}
    }

  return FALSE;
}
