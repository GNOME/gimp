/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpdockbook.c
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

#include <string.h>

#include <gtk/gtk.h>

#include "libgimpwidgets/gimpwidgets.h"

#include "widgets-types.h"

#include "gimpdialogfactory.h"
#include "gimpdnd.h"
#include "gimpdockable.h"
#include "gimpdockbook.h"
#include "gimpimagedock.h"
#include "gimpitemfactory.h"
#include "gimppreview.h"


#define TAB_WIDGET_SIZE     24
#define MENU_WIDGET_SIZE    16
#define MENU_WIDGET_SPACING  4


static void      gimp_dockbook_class_init       (GimpDockbookClass  *klass);
static void      gimp_dockbook_init             (GimpDockbook       *dockbook);

static void      gimp_dockbook_style_set        (GtkWidget          *widget,
                                                 GtkStyle           *prev_style);
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


GType
gimp_dockbook_get_type (void)
{
  static GType dockbook_type = 0;

  if (! dockbook_type)
    {
      static const GTypeInfo dockbook_info =
      {
        sizeof (GimpDockbookClass),
        NULL,           /* base_init */
        NULL,           /* base_finalize */
        (GClassInitFunc) gimp_dockbook_class_init,
        NULL,           /* class_finalize */
        NULL,           /* class_data */
        sizeof (GimpDockbook),
        0,              /* n_preallocs */
        (GInstanceInitFunc) gimp_dockbook_init,
      };

      dockbook_type = g_type_register_static (GTK_TYPE_NOTEBOOK,
                                              "GimpDockbook",
                                              &dockbook_info, 0);
    }

  return dockbook_type;
}

static void
gimp_dockbook_class_init (GimpDockbookClass *klass)
{
  GtkWidgetClass *widget_class;

  widget_class = GTK_WIDGET_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);

  widget_class->style_set = gimp_dockbook_style_set;
  widget_class->drag_drop = gimp_dockbook_drag_drop;

  gtk_widget_class_install_style_property (widget_class,
                                           g_param_spec_int ("tab_border",
                                                             NULL, NULL,
                                                             0,
                                                             G_MAXINT,
                                                             0,
                                                             G_PARAM_READABLE));
}

static void
gimp_dockbook_init (GimpDockbook *dockbook)
{
  dockbook->dock = NULL;

  gtk_notebook_popup_enable (GTK_NOTEBOOK (dockbook));
  gtk_notebook_set_scrollable (GTK_NOTEBOOK (dockbook), TRUE);

  gtk_drag_dest_set (GTK_WIDGET (dockbook),
                     GTK_DEST_DEFAULT_ALL,
                     dialog_target_table, G_N_ELEMENTS (dialog_target_table),
                     GDK_ACTION_MOVE);
}

GtkWidget *
gimp_dockbook_new (void)
{
  return GTK_WIDGET (g_object_new (GIMP_TYPE_DOCKBOOK, NULL));
}

static void
gimp_dockbook_style_set (GtkWidget *widget,
                         GtkStyle  *prev_style)
{
  gint tab_border;

  if (GTK_WIDGET_CLASS (parent_class)->style_set)
    GTK_WIDGET_CLASS (parent_class)->style_set (widget, prev_style);

  gtk_widget_style_get (widget,
                        "tab_border", &tab_border,
                        NULL);

  g_object_set (G_OBJECT (widget),
                "tab_border", tab_border,
                NULL);
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

      dockable = (GimpDockable *) g_object_get_data (G_OBJECT (source),
						     "gimp-dockable");

      if (dockable)
	{
	  g_object_set_data (G_OBJECT (dockable),
			     "gimp-dock-drag-widget", NULL);

	  if (dockable->dockbook != GIMP_DOCKBOOK (widget))
	    {
	      g_object_ref (G_OBJECT (dockable));

	      gimp_dockbook_remove (dockable->dockbook, dockable);
	      gimp_dockbook_add (GIMP_DOCKBOOK (widget), dockable, -1);

	      g_object_unref (G_OBJECT (dockable));

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

  g_return_if_fail (GIMP_IS_DOCKBOOK (dockbook));
  g_return_if_fail (dockbook->dock != NULL);
  g_return_if_fail (GIMP_IS_DOCKABLE (dockable));
  g_return_if_fail (dockable->dockbook == NULL);

  tab_widget = gimp_dockable_get_tab_widget (dockable, dockbook,
					     TAB_WIDGET_SIZE);

  g_return_if_fail (GTK_IS_WIDGET (tab_widget));

  if (GTK_WIDGET_NO_WINDOW (tab_widget))
    {
      GtkWidget *event_box;

      event_box = gtk_event_box_new ();
      gtk_container_add (GTK_CONTAINER (event_box), tab_widget);
      gtk_widget_show (tab_widget);

      tab_widget = event_box;
    }

  gimp_help_set_help_data (tab_widget, dockable->name, NULL);

  g_object_set_data (G_OBJECT (tab_widget), "gimp-dockable", dockable);

  /*  set the drag source *before* connecting button_press because we
   *  stop button_press emission by returning TRUE from the callback
   */
  gtk_drag_source_set (GTK_WIDGET (tab_widget),
		       GDK_BUTTON1_MASK | GDK_BUTTON2_MASK,
		       dialog_target_table, G_N_ELEMENTS (dialog_target_table),
		       GDK_ACTION_MOVE);
  g_signal_connect (G_OBJECT (tab_widget), "drag_begin",
		    G_CALLBACK (gimp_dockbook_tab_drag_begin),
		    dockable);
  g_signal_connect (G_OBJECT (tab_widget), "drag_end",
		    G_CALLBACK (gimp_dockbook_tab_drag_end),
		    dockable);

  gtk_drag_dest_set (GTK_WIDGET (tab_widget),
                     GTK_DEST_DEFAULT_ALL,
                     dialog_target_table, G_N_ELEMENTS (dialog_target_table),
                     GDK_ACTION_MOVE);
  g_signal_connect (G_OBJECT (tab_widget), "drag_drop",
		    G_CALLBACK (gimp_dockbook_tab_drag_drop),
		    dockbook);

  g_signal_connect (G_OBJECT (tab_widget), "button_press_event",
		    G_CALLBACK (gimp_dockbook_tab_button_press),
		    dockable);

  menu_widget = gimp_dockable_get_tab_widget (dockable, dockbook,
					      MENU_WIDGET_SIZE);

  g_return_if_fail (GTK_IS_WIDGET (menu_widget));

  if (GIMP_IS_PREVIEW (menu_widget))
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
  else if (GTK_IS_LABEL (menu_widget))
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

  /*  Now this is evil: GtkNotebook's menu_item "activate" callback
   *  gets it's notebook context from gtk_menu_get_attach_widget()
   *  which badly fails because we hijacked the menu and attached it
   *  to our own item_factory menu. As a workaround, we disconnect
   *  gtk's handler and install our own. --Mitch
   */
  {
    GtkWidget *menu_item;
    GList     *widget_list, *free_list;
    GList     *page_list;

    menu_item = menu_widget->parent;

    free_list = gtk_container_get_children (GTK_CONTAINER (dockbook));

    /*  EEK: we rely a 1:1 and left-to-right mapping of
     *  gtk_container_get_children() and notebook->children
     */
    for (widget_list = free_list,
	   page_list = GTK_NOTEBOOK (dockbook)->children;
	 widget_list && page_list;
	 widget_list = g_list_next (widget_list),
	   page_list = g_list_next (page_list))
      {
	GtkNotebookPage *page;

	page = (GtkNotebookPage *) page_list->data;

	if ((GtkWidget *) widget_list->data == (GtkWidget *) dockable)
	  {
	    /*  disconnect GtkNotebook's handler  */
	    g_signal_handlers_disconnect_matched (G_OBJECT (menu_item),
						  G_SIGNAL_MATCH_DATA,
						  0, 0,
						  NULL, NULL, page);

	    /*  and install our own  */
	    g_signal_connect (G_OBJECT (menu_item), "activate",
			      G_CALLBACK (gimp_dockbook_menu_switch_page),
			      dockable);

	    break;
	  }
      }

    g_list_free (free_list);
  }

  gtk_widget_show (GTK_WIDGET (dockable));

  dockable->dockbook = dockbook;

  gimp_dockable_set_context (dockable, dockbook->dock->context);
}

void
gimp_dockbook_remove (GimpDockbook *dockbook,
		      GimpDockable *dockable)
{
  GList *children;

  g_return_if_fail (GIMP_IS_DOCKBOOK (dockbook));
  g_return_if_fail (GIMP_IS_DOCKABLE (dockable));

  g_return_if_fail (dockable->dockbook != NULL);
  g_return_if_fail (dockable->dockbook == dockbook);

  dockable->dockbook = NULL;

  gimp_dockable_set_context (dockable, NULL);

  gtk_container_remove (GTK_CONTAINER (dockbook), GTK_WIDGET (dockable));

  children = gtk_container_get_children (GTK_CONTAINER (dockbook));

  if (! g_list_length (children))
    {
      gimp_dock_remove_book (dockbook->dock, dockbook);
    }

  g_list_free (children);
}

static void
gimp_dockbook_menu_switch_page (GtkWidget    *widget,
				GimpDockable *dockable)
{
  gint page_num;

  page_num = gtk_notebook_page_num (GTK_NOTEBOOK (dockable->dockbook),
				    GTK_WIDGET (dockable));

  gtk_notebook_set_current_page (GTK_NOTEBOOK (dockable->dockbook), page_num);
}

static void
gimp_dockbook_menu_end (GimpDockbook *dockbook)
{
  GtkWidget *notebook_menu;

  notebook_menu = GTK_NOTEBOOK (dockbook)->menu;

  g_object_ref (G_OBJECT (notebook_menu));

  gtk_menu_detach (GTK_MENU (notebook_menu));

  gtk_menu_attach_to_widget (GTK_MENU (notebook_menu),
			     GTK_WIDGET (dockbook),
			     gimp_dockbook_menu_detacher);

  g_object_unref (G_OBJECT (notebook_menu));

  /*  release gimp_dockbook_tab_button_press()'s reference  */
  g_object_unref (G_OBJECT (dockbook));
}

static void
gimp_dockbook_menu_detacher (GtkWidget *widget,
			     GtkMenu   *menu)
{
  GtkNotebook *notebook;

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

  gtk_notebook_set_current_page (GTK_NOTEBOOK (dockbook), page_num);

  if (bevent->button == 3)
    {
      GimpItemFactory *item_factory;
      GtkWidget       *add_widget;

      item_factory = dockbook->dock->factory->item_factory;

      add_widget = gtk_item_factory_get_widget (GTK_ITEM_FACTORY (item_factory),
                                                "/Select Tab");

      /*  do evil things  */
      {
        GtkWidget *notebook_menu;

        notebook_menu = GTK_NOTEBOOK (dockbook)->menu;

        g_object_ref (G_OBJECT (notebook_menu));

        gtk_menu_detach (GTK_MENU (notebook_menu));

        GTK_NOTEBOOK (dockbook)->menu = notebook_menu;

        gtk_menu_item_set_submenu (GTK_MENU_ITEM (add_widget), notebook_menu);

        g_object_unref (G_OBJECT (notebook_menu));
      }

      /*  an item factory callback may destroy the dockbook, so reference
       *  if for gimp_dockbook_menu_end()
       */
      g_object_ref (G_OBJECT (dockbook));

      gimp_item_factory_popup_with_data (item_factory,
                                         dockbook,
                                         (GtkDestroyNotify) gimp_dockbook_menu_end);
    }

  return TRUE;
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

  g_object_set_data_full (G_OBJECT (dockable), "gimp-dock-drag-widget",
                          window,
                          (GDestroyNotify) gtk_widget_destroy);

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

  drag_widget = g_object_get_data (G_OBJECT (dockable),
				   "gimp-dock-drag-widget");

  /*  finding the drag_widget means the drop was not successful, so
   *  pop up a new dock and move the dockable there
   */
  if (drag_widget)
    {
      GtkWidget *dock;
      GtkWidget *dockbook;
      gboolean   auto_follow_active;
      gboolean   show_image_menu;

      g_object_set_data (G_OBJECT (dockable), "gimp-dock-drag-widget", NULL);

      dock = gimp_dialog_factory_dock_new (dockable->dockbook->dock->factory);

      auto_follow_active =
	GIMP_IMAGE_DOCK (dockable->dockbook->dock)->auto_follow_active;
      show_image_menu =
	GIMP_IMAGE_DOCK (dockable->dockbook->dock)->show_image_menu;

      gimp_image_dock_set_auto_follow_active (GIMP_IMAGE_DOCK (dock),
					      auto_follow_active);
      gimp_image_dock_set_show_image_menu (GIMP_IMAGE_DOCK (dock),
					   show_image_menu);

      gtk_window_set_position (GTK_WINDOW (dock), GTK_WIN_POS_MOUSE);

      dockbook = gimp_dockbook_new ();

      gimp_dock_add_book (GIMP_DOCK (dock), GIMP_DOCKBOOK (dockbook), 0);

      g_object_ref (G_OBJECT (dockable));

      gimp_dockbook_remove (dockable->dockbook, dockable);
      gimp_dockbook_add (GIMP_DOCKBOOK (dockbook), dockable, 0);

      g_object_unref (G_OBJECT (dockable));

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

  dest_dockable = g_object_get_data (G_OBJECT (widget),
				     "gimp-dockable");

  source = gtk_drag_get_source_widget (context);

  if (dest_dockable && source)
    {
      GimpDockable *src_dockable;

      src_dockable = g_object_get_data (G_OBJECT (source),
					"gimp-dockable");

      if (src_dockable)
	{
	  gint dest_index;

	  dest_index =
	    gtk_notebook_page_num (GTK_NOTEBOOK (dest_dockable->dockbook),
				   GTK_WIDGET (dest_dockable));

	  g_object_set_data (G_OBJECT (src_dockable),
			     "gimp-dock-drag-widget", NULL);

	  if (src_dockable->dockbook != dest_dockable->dockbook)
	    {
	      g_object_ref (G_OBJECT (src_dockable));

	      gimp_dockbook_remove (src_dockable->dockbook, src_dockable);
	      gimp_dockbook_add (dest_dockable->dockbook, src_dockable,
				 dest_index);

	      g_object_unref (G_OBJECT (src_dockable));

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
