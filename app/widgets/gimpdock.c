/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpdock.c
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


#define GIMP_DOCK_MINIMAL_WIDTH    280
#define GIMP_DOCK_SEPARATOR_HEIGHT 8


static void        gimp_dock_class_init               (GimpDockClass  *klass);
static void        gimp_dock_init                     (GimpDock       *dock);

static GtkWidget * gimp_dock_separator_new            (GimpDock       *dock);

static void        gimp_dock_destroy                  (GtkObject      *object);

static gboolean    gimp_dock_separator_button_press   (GtkWidget      *widget,
						       GdkEventButton *bevent,
						       gpointer        data);
static gboolean    gimp_dock_separator_button_release (GtkWidget      *widget,
						       GdkEventButton *bevent,
						       gpointer        data);

/*
static void        gimp_dock_separator_drag_begin     (GtkWidget      *widget,
			  			       GdkDragContext *context,
						       gpointer        data);
static void        gimp_dock_separator_drag_end       (GtkWidget      *widget,
						       GdkDragContext *context,
						       gpointer        data);
*/

static gboolean    gimp_dock_separator_drag_drop      (GtkWidget      *widget,
						       GdkDragContext *context,
						       gint            x,
						       gint            y,
						       guint           time,
						       gpointer        data);


static GtkWindowClass *parent_class = NULL;

static GtkTargetEntry dialog_target_table[] =
{
  GIMP_TARGET_DIALOG
};
static guint n_dialog_targets = (sizeof (dialog_target_table) /
				 sizeof (dialog_target_table[0]));


GtkType
gimp_dock_get_type (void)
{
  static GtkType dock_type = 0;

  if (! dock_type)
    {
      static const GtkTypeInfo dock_info =
      {
	"GimpDock",
	sizeof (GimpDock),
	sizeof (GimpDockClass),
	(GtkClassInitFunc) gimp_dock_class_init,
	(GtkObjectInitFunc) gimp_dock_init,
	/* reserved_1 */ NULL,
        /* reserved_2 */ NULL,
        (GtkClassInitFunc) NULL,
      };

      dock_type = gtk_type_unique (GTK_TYPE_WINDOW, &dock_info);
    }

  return dock_type;
}

static void
gimp_dock_class_init (GimpDockClass *klass)
{
  GtkObjectClass *object_class;

  object_class = (GtkObjectClass *) klass;

  parent_class = gtk_type_class (GTK_TYPE_WINDOW);

  object_class->destroy = gimp_dock_destroy;
}

static void
gimp_dock_init (GimpDock *dock)
{
  GtkWidget *separator;

  dock->context = NULL;

  gtk_window_set_title (GTK_WINDOW (dock), "Gimp");
  gtk_window_set_wmclass (GTK_WINDOW (dock), "dock", "Gimp");
  gtk_window_set_policy (GTK_WINDOW (dock), FALSE, TRUE, TRUE);
  gimp_dialog_set_icon (GTK_WINDOW (dock));

  gtk_widget_set_usize (GTK_WIDGET (dock), GIMP_DOCK_MINIMAL_WIDTH, -1);
 
  dock->main_vbox = gtk_vbox_new (FALSE, 0);
  gtk_container_add (GTK_CONTAINER (dock), dock->main_vbox);
  gtk_widget_show (dock->main_vbox);

  dock->vbox = gtk_vbox_new (FALSE, 0);
  gtk_container_add (GTK_CONTAINER (dock->main_vbox), dock->vbox);
  gtk_widget_show (dock->vbox);

  separator = gimp_dock_separator_new (dock);
  gtk_box_pack_start (GTK_BOX (dock->vbox), separator, FALSE, FALSE, 0);
  gtk_widget_show (separator);
}

static void
gimp_dock_destroy (GtkObject *object)
{
  GimpDock *dock;

  dock = GIMP_DOCK (object);

  while (dock->dockbooks)
    gimp_dock_remove_book (dock, GIMP_DOCKBOOK (dock->dockbooks->data));

  gtk_object_unref (GTK_OBJECT (dock->context));

  if (GTK_OBJECT_CLASS (parent_class))
    GTK_OBJECT_CLASS (parent_class)->destroy (object);
}

static GtkWidget *
gimp_dock_separator_new (GimpDock *dock)
{
  GtkWidget *event_box;
  GtkWidget *frame;

  event_box = gtk_event_box_new ();
  gtk_widget_set_usize (event_box, -1, GIMP_DOCK_SEPARATOR_HEIGHT);

  frame = gtk_frame_new (NULL);
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_OUT);
  gtk_container_add (GTK_CONTAINER (event_box), frame);
  gtk_widget_show (frame);

  gtk_drag_dest_set (GTK_WIDGET (event_box),
                     GTK_DEST_DEFAULT_ALL,
                     dialog_target_table, n_dialog_targets,
                     GDK_ACTION_MOVE);
  gtk_signal_connect (GTK_OBJECT (event_box), "drag_drop",
		      GTK_SIGNAL_FUNC (gimp_dock_separator_drag_drop),
		      dock);

  gtk_signal_connect (GTK_OBJECT (event_box), "button_press_event",
		      GTK_SIGNAL_FUNC (gimp_dock_separator_button_press),
		      dock);
  gtk_signal_connect (GTK_OBJECT (event_box), "button_release_event",
		      GTK_SIGNAL_FUNC (gimp_dock_separator_button_release),
		      dock);

  return event_box;
}

void
gimp_dock_add (GimpDock     *dock,
	       GimpDockable *dockable,
	       gint          section,
	       gint          position)
{
  GimpDockbook *dockbook;

  g_return_if_fail (dock != NULL);
  g_return_if_fail (GIMP_IS_DOCK (dock));

  g_return_if_fail (dockable != NULL);
  g_return_if_fail (GIMP_IS_DOCKABLE (dockable));

  g_return_if_fail (dockable->dockbook == NULL);

  dockbook = GIMP_DOCKBOOK (dock->dockbooks->data);

  gimp_dockbook_add (dockbook, dockable, position);
}

void
gimp_dock_remove (GimpDock     *dock,
		  GimpDockable *dockable)
{
  g_return_if_fail (dock != NULL);
  g_return_if_fail (GIMP_IS_DOCK (dock));

  g_return_if_fail (dockable != NULL);
  g_return_if_fail (GIMP_IS_DOCKABLE (dockable));

  g_return_if_fail (dockable->dockbook != NULL);
  g_return_if_fail (dockable->dockbook->dock != NULL);
  g_return_if_fail (dockable->dockbook->dock == dock);

  gimp_dockbook_remove (dockable->dockbook, dockable);
}

void
gimp_dock_add_book (GimpDock     *dock,
		    GimpDockbook *dockbook,
		    gint          index)
{
  GtkWidget *separator;
  gint       length;

  g_return_if_fail (dock != NULL);
  g_return_if_fail (GIMP_IS_DOCK (dock));

  g_return_if_fail (dockbook != NULL);
  g_return_if_fail (GIMP_IS_DOCKBOOK (dockbook));

  g_return_if_fail (dockbook->dock == NULL);

  length = g_list_length (dock->dockbooks);

  if (index >= length || index < 0)
    index = length;

  dockbook->dock = dock;
  dock->dockbooks = g_list_insert (dock->dockbooks, dockbook, index);

  index *= 2;

  gtk_box_pack_start (GTK_BOX (dock->vbox), GTK_WIDGET (dockbook),
		      TRUE, TRUE, 0);
  gtk_box_reorder_child (GTK_BOX (dock->vbox), GTK_WIDGET (dockbook), index);
  gtk_widget_show (GTK_WIDGET (dockbook));

  separator = gimp_dock_separator_new (dock);
  gtk_box_pack_start (GTK_BOX (dock->vbox), separator, FALSE, FALSE, 0);
  gtk_box_reorder_child (GTK_BOX (dock->vbox), separator, index);
  gtk_widget_show (separator);
}

void
gimp_dock_remove_book (GimpDock     *dock,
		       GimpDockbook *dockbook)
{
  GList *children;
  gint   length;
  gint   index;
  gint   book_index;

  g_return_if_fail (dock != NULL);
  g_return_if_fail (GIMP_IS_DOCK (dock));

  g_return_if_fail (dockbook != NULL);
  g_return_if_fail (GIMP_IS_DOCKBOOK (dockbook));

  g_return_if_fail (dockbook->dock == dock);

  length = g_list_length (dock->dockbooks);
  index  = g_list_index (dock->dockbooks, dockbook);

  dockbook->dock = NULL;
  dock->dockbooks = g_list_remove (dock->dockbooks, dockbook);

  children   = gtk_container_children (GTK_CONTAINER (dock->vbox));
  book_index = g_list_index (children, dockbook);

  if (length != 1)
    {
      GtkWidget *separator;

      if (index == length -1)
	{
	  separator = g_list_nth_data (children, book_index - 1);
	}
      else
	{
	  separator = g_list_nth_data (children, book_index + 1);
	}

      gtk_container_remove (GTK_CONTAINER (dock->vbox), separator);
    }

  gtk_container_remove (GTK_CONTAINER (dock->vbox), GTK_WIDGET (dockbook));

  if (length == 1)
    {
      gtk_widget_destroy (GTK_WIDGET (dock));
      return;
    }
}

static gboolean
gimp_dock_separator_button_press (GtkWidget      *widget,
				  GdkEventButton *bevent,
				  gpointer        data)
{
  if (bevent->type == GDK_BUTTON_PRESS)
    {
      if (bevent->button == 1)
        {
          gtk_grab_add (widget);
        }
    }

  return TRUE;
}

static gboolean
gimp_dock_separator_button_release (GtkWidget      *widget,
				    GdkEventButton *bevent,
				    gpointer        data)
{
  if (bevent->button == 1)
    {
      gtk_grab_remove (widget);
    }

  return TRUE;
}

/*
static void
gimp_dock_tab_drag_begin (GtkWidget      *widget,
			  GdkDragContext *context,
			  gpointer        data)
{
  GimpDockable *dockable;
  GtkWidget    *window;
  GtkWidget    *frame;
  GtkWidget    *label;

  dockable = GIMP_DOCKABLE (data);

  window = gtk_window_new (GTK_WINDOW_POPUP);

  frame = gtk_frame_new (NULL);
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_OUT);
  gtk_container_add (GTK_CONTAINER (window), frame);
  gtk_widget_show (frame);

  label = gtk_label_new (dockable->name);
  gtk_misc_set_padding (GTK_MISC (label), 10, 5);
  gtk_container_add (GTK_CONTAINER (frame), label);
  gtk_widget_show (label);

  gtk_widget_show (window);

  gtk_object_set_data_full (GTK_OBJECT (dockable), "gimp-dock-drag-widget",
			    window,
			    (GtkDestroyNotify) gtk_widget_destroy);

  gtk_drag_set_icon_widget (context, window,
			    -8, -8);
}

static void
gimp_dock_tab_drag_end (GtkWidget      *widget,
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

      gtk_object_set_data (GTK_OBJECT (dockable), "gimp-dock-drag-widget", NULL);

      dock = gimp_dock_new ();

      gtk_window_set_position (GTK_WINDOW (dock), GTK_WIN_POS_MOUSE);

      gtk_object_ref (GTK_OBJECT (dockable));

      gimp_dock_remove (dockable->dock, dockable);
      gimp_dock_add (GIMP_DOCK (dock), dockable, -1, -1);

      gtk_object_unref (GTK_OBJECT (dockable));

      gtk_widget_show (dock);
    }
}
*/

static gboolean
gimp_dock_separator_drag_drop (GtkWidget      *widget,
			       GdkDragContext *context,
			       gint            x,
			       gint            y,
			       guint           time,
			       gpointer        data)
{
  GimpDock  *dock;
  GtkWidget *source;

  dock = GIMP_DOCK (data);

  source = gtk_drag_get_source_widget (context);

  if (source)
    {
      GimpDockable *src_dockable;

      src_dockable = (GimpDockable *) gtk_object_get_data (GTK_OBJECT (source),
							   "gimp-dockable");

      if (src_dockable)
	{
	  GtkWidget *dockbook;
	  GList     *children;
	  gint       index;

	  gtk_object_set_data (GTK_OBJECT (src_dockable),
			       "gimp-dock-drag-widget", NULL);

	  children = gtk_container_children (GTK_CONTAINER (widget->parent));
	  index    = g_list_index (children, widget) / 2;

	  dockbook = gimp_dockbook_new ();

	  gimp_dock_add_book (dock, GIMP_DOCKBOOK (dockbook), index);

	  gtk_object_ref (GTK_OBJECT (src_dockable));

	  gimp_dockbook_remove (src_dockable->dockbook, src_dockable);
	  gimp_dockbook_add (GIMP_DOCKBOOK (dockbook), src_dockable, -1);

	  gtk_object_unref (GTK_OBJECT (src_dockable));

	  return TRUE;
	}
    }

  return FALSE;
}
