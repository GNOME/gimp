/* GTK - The GIMP Toolkit
 * Copyright (C) 1995-1997 Peter Mattis, Spencer Kimball and Josh MacDonald
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the Free
 * Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

/*
  Modified 97-06-26	Eiichi Takamori <taka@ma1.seikyou.ne.jp>
  GtkMultiOptionMenu, taken from GtkOptionMenu, can work with
  hierarchal menus.
 */

#include "gtk/gtkmenu.h"
#include "gtk/gtkmenuitem.h"
#include "gtkmultioptionmenu.h"
#include "gtk/gtksignal.h"


#define CHILD_LEFT_SPACING        5
#define CHILD_RIGHT_SPACING       1
#define CHILD_TOP_SPACING         1
#define CHILD_BOTTOM_SPACING      1
#define MULTI_OPTION_INDICATOR_WIDTH    12
#define MULTI_OPTION_INDICATOR_HEIGHT   8
#define MULTI_OPTION_INDICATOR_SPACING  2


static void gtk_multi_option_menu_class_init      (GtkMultiOptionMenuClass *klass);
static void gtk_multi_option_menu_init            (GtkMultiOptionMenu      *multi_option_menu);
static void gtk_multi_option_menu_destroy         (GtkObject          *object);
static void gtk_multi_option_menu_size_request    (GtkWidget          *widget,
						   GtkRequisition     *requisition);
static void gtk_multi_option_menu_size_allocate   (GtkWidget          *widget,
						   GtkAllocation      *allocation);
static void gtk_multi_option_menu_paint           (GtkWidget          *widget,
						   GdkRectangle       *area);
static void gtk_multi_option_menu_draw            (GtkWidget          *widget,
						   GdkRectangle       *area);
static gint gtk_multi_option_menu_expose          (GtkWidget          *widget,
						   GdkEventExpose     *event);
static gint gtk_multi_option_menu_button_press    (GtkWidget          *widget,
						   GdkEventButton     *event);
static void gtk_multi_option_menu_deactivate      (GtkMenuShell       *menu_shell,
						   GtkMultiOptionMenu      *multi_option_menu);
static void gtk_multi_option_menu_update_contents (GtkMultiOptionMenu      *multi_option_menu);
static void gtk_multi_option_menu_remove_contents (GtkMultiOptionMenu      *multi_option_menu);
static void gtk_multi_option_menu_calc_size       (GtkMultiOptionMenu      *multi_option_menu);
static void gtk_multi_option_menu_calc_size_recursive (GtkMultiOptionMenu *multi_option_menu, GtkWidget *menu);
static void gtk_multi_option_menu_position        (GtkMenu            *menu,
						   gint               *x,
						   gint               *y,
						   gpointer            user_data);


static GtkButtonClass *parent_class = NULL;


guint
gtk_multi_option_menu_get_type ()
{
  static guint multi_option_menu_type = 0;

  if (!multi_option_menu_type)
    {
      GtkTypeInfo multi_option_menu_info =
      {
	"GtkMultiOptionMenu",
	sizeof (GtkMultiOptionMenu),
	sizeof (GtkMultiOptionMenuClass),
	(GtkClassInitFunc) gtk_multi_option_menu_class_init,
	(GtkObjectInitFunc) gtk_multi_option_menu_init,
	(GtkArgFunc) NULL,
      };

      multi_option_menu_type = gtk_type_unique (gtk_button_get_type (), &multi_option_menu_info);
    }

  return multi_option_menu_type;
}

static void
gtk_multi_option_menu_class_init (GtkMultiOptionMenuClass *class)
{
  GtkObjectClass *object_class;
  GtkWidgetClass *widget_class;
  GtkButtonClass *button_class;

  object_class = (GtkObjectClass*) class;
  widget_class = (GtkWidgetClass*) class;
  button_class = (GtkButtonClass*) class;

  parent_class = gtk_type_class (gtk_button_get_type ());

  object_class->destroy = gtk_multi_option_menu_destroy;

  widget_class->draw = gtk_multi_option_menu_draw;
  widget_class->draw_focus = NULL;
  widget_class->size_request = gtk_multi_option_menu_size_request;
  widget_class->size_allocate = gtk_multi_option_menu_size_allocate;
  widget_class->expose_event = gtk_multi_option_menu_expose;
  widget_class->button_press_event = gtk_multi_option_menu_button_press;
}

static void
gtk_multi_option_menu_init (GtkMultiOptionMenu *multi_option_menu)
{
  GTK_WIDGET_UNSET_FLAGS (multi_option_menu, GTK_CAN_FOCUS);

  multi_option_menu->menu = NULL;
  multi_option_menu->menu_item = NULL;
  multi_option_menu->width = 0;
  multi_option_menu->height = 0;
}

GtkWidget*
gtk_multi_option_menu_new ()
{
  return GTK_WIDGET (gtk_type_new (gtk_multi_option_menu_get_type ()));
}

GtkWidget*
gtk_multi_option_menu_get_menu (GtkMultiOptionMenu *multi_option_menu)
{
  g_return_val_if_fail (multi_option_menu != NULL, NULL);
  g_return_val_if_fail (GTK_IS_MULTI_OPTION_MENU (multi_option_menu), NULL);

  return multi_option_menu->menu;
}

void
gtk_multi_option_menu_set_menu (GtkMultiOptionMenu *multi_option_menu,
				GtkWidget     *menu)
{
  g_return_if_fail (multi_option_menu != NULL);
  g_return_if_fail (GTK_IS_MULTI_OPTION_MENU (multi_option_menu));
  g_return_if_fail (menu != NULL);
  g_return_if_fail (GTK_IS_MENU (menu));

  gtk_multi_option_menu_remove_menu (multi_option_menu);

  multi_option_menu->menu = menu;
  gtk_object_ref (GTK_OBJECT (multi_option_menu->menu));

  gtk_multi_option_menu_calc_size (multi_option_menu);

  gtk_signal_connect (GTK_OBJECT (multi_option_menu->menu), "deactivate",
		      (GtkSignalFunc) gtk_multi_option_menu_deactivate,
		      multi_option_menu);

  if (GTK_WIDGET (multi_option_menu)->parent)
    gtk_widget_queue_resize (GTK_WIDGET (multi_option_menu)->parent);

  gtk_multi_option_menu_update_contents (multi_option_menu);
}

void
gtk_multi_option_menu_remove_menu (GtkMultiOptionMenu *multi_option_menu)
{
  g_return_if_fail (multi_option_menu != NULL);
  g_return_if_fail (GTK_IS_MULTI_OPTION_MENU (multi_option_menu));

  if (multi_option_menu->menu)
    {
      gtk_multi_option_menu_remove_contents (multi_option_menu);
      gtk_signal_disconnect_by_data (GTK_OBJECT (multi_option_menu->menu),
				     multi_option_menu);

      gtk_object_unref (GTK_OBJECT (multi_option_menu->menu));
      multi_option_menu->menu = NULL;
    }
}

void
gtk_multi_option_menu_set_history (GtkMultiOptionMenu *multi_option_menu,
				   gint           index)
{
  GtkWidget *menu_item;

  g_return_if_fail (multi_option_menu != NULL);
  g_return_if_fail (GTK_IS_MULTI_OPTION_MENU (multi_option_menu));

  if (multi_option_menu->menu)
    {
      gtk_menu_set_active (GTK_MENU (multi_option_menu->menu), index);
      menu_item = gtk_menu_get_active (GTK_MENU (multi_option_menu->menu));

      if (menu_item != multi_option_menu->menu_item)
	{
	  gtk_multi_option_menu_remove_contents (multi_option_menu);
	  gtk_multi_option_menu_update_contents (multi_option_menu);
	}
    }
}


static void
gtk_multi_option_menu_destroy (GtkObject *object)
{
  GtkMultiOptionMenu *multi_option_menu;

  g_return_if_fail (object != NULL);
  g_return_if_fail (GTK_IS_MULTI_OPTION_MENU (object));

  multi_option_menu = GTK_MULTI_OPTION_MENU (object);

  gtk_multi_option_menu_remove_contents (multi_option_menu);
  if (multi_option_menu->menu)
    {
      gtk_object_unref (GTK_OBJECT (multi_option_menu->menu));
      gtk_widget_destroy (multi_option_menu->menu);
    }

  if (GTK_OBJECT_CLASS (parent_class)->destroy)
    (* GTK_OBJECT_CLASS (parent_class)->destroy) (object);
}

static void
gtk_multi_option_menu_size_request (GtkWidget      *widget,
				    GtkRequisition *requisition)
{
  GtkMultiOptionMenu *multi_option_menu;
  gint tmp;

  g_return_if_fail (widget != NULL);
  g_return_if_fail (GTK_IS_MULTI_OPTION_MENU (widget));
  g_return_if_fail (requisition != NULL);

  multi_option_menu = GTK_MULTI_OPTION_MENU (widget);

  requisition->width = ((GTK_CONTAINER (widget)->border_width +
			 GTK_WIDGET (widget)->style->klass->xthickness) * 2 +
			multi_option_menu->width +
			MULTI_OPTION_INDICATOR_WIDTH +
			MULTI_OPTION_INDICATOR_SPACING * 5 +
			CHILD_LEFT_SPACING + CHILD_RIGHT_SPACING);
  requisition->height = ((GTK_CONTAINER (widget)->border_width +
			  GTK_WIDGET (widget)->style->klass->ythickness) * 2 +
			 multi_option_menu->height +
			 CHILD_TOP_SPACING + CHILD_BOTTOM_SPACING);

  tmp = (requisition->height - multi_option_menu->height +
	 MULTI_OPTION_INDICATOR_HEIGHT + MULTI_OPTION_INDICATOR_SPACING * 2);
  requisition->height = MAX (requisition->height, tmp);
}

static void
gtk_multi_option_menu_size_allocate (GtkWidget     *widget,
				     GtkAllocation *allocation)
{
  GtkWidget *child;
  GtkAllocation child_allocation;

  g_return_if_fail (widget != NULL);
  g_return_if_fail (GTK_IS_MULTI_OPTION_MENU (widget));
  g_return_if_fail (allocation != NULL);

  widget->allocation = *allocation;
  if (GTK_WIDGET_REALIZED (widget))
    gdk_window_move_resize (widget->window,
			    allocation->x, allocation->y,
			    allocation->width, allocation->height);

  child = GTK_BUTTON (widget)->child;
  if (child && GTK_WIDGET_VISIBLE (child))
    {
      child_allocation.x = (GTK_CONTAINER (widget)->border_width +
			    GTK_WIDGET (widget)->style->klass->xthickness);
      child_allocation.y = (GTK_CONTAINER (widget)->border_width +
			    GTK_WIDGET (widget)->style->klass->ythickness);
      child_allocation.width = (allocation->width - child_allocation.x * 2 -
				MULTI_OPTION_INDICATOR_WIDTH - MULTI_OPTION_INDICATOR_SPACING * 5 -
				CHILD_LEFT_SPACING - CHILD_RIGHT_SPACING);
      child_allocation.height = (allocation->height - child_allocation.y * 2 -
				 CHILD_TOP_SPACING - CHILD_BOTTOM_SPACING);
      child_allocation.x += CHILD_LEFT_SPACING;
      child_allocation.y += CHILD_RIGHT_SPACING;

      gtk_widget_size_allocate (child, &child_allocation);
    }
}

static void
gtk_multi_option_menu_paint (GtkWidget    *widget,
			     GdkRectangle *area)
{
  GdkRectangle restrict_area;
  GdkRectangle new_area;

  g_return_if_fail (widget != NULL);
  g_return_if_fail (GTK_IS_MULTI_OPTION_MENU (widget));
  g_return_if_fail (area != NULL);

  if (GTK_WIDGET_DRAWABLE (widget))
    {
      restrict_area.x = GTK_CONTAINER (widget)->border_width;
      restrict_area.y = GTK_CONTAINER (widget)->border_width;
      restrict_area.width = widget->allocation.width - restrict_area.x * 2;
      restrict_area.height = widget->allocation.height - restrict_area.y * 2;

      if (gdk_rectangle_intersect (area, &restrict_area, &new_area))
	{
	  gtk_style_set_background (widget->style, widget->window, GTK_WIDGET_STATE (widget));
	  gdk_window_clear_area (widget->window,
				 new_area.x, new_area.y,
				 new_area.width, new_area.height);

	  gtk_draw_shadow (widget->style, widget->window,
			   GTK_WIDGET_STATE (widget), GTK_SHADOW_OUT,
			   restrict_area.x, restrict_area.y,
			   restrict_area.width, restrict_area.height);

	  gtk_draw_shadow (widget->style, widget->window,
			   GTK_WIDGET_STATE (widget), GTK_SHADOW_OUT,
			   restrict_area.x + restrict_area.width - restrict_area.x -
			   MULTI_OPTION_INDICATOR_WIDTH - MULTI_OPTION_INDICATOR_SPACING * 4,
			   restrict_area.y + (restrict_area.height - MULTI_OPTION_INDICATOR_HEIGHT) / 2,
			   MULTI_OPTION_INDICATOR_WIDTH, MULTI_OPTION_INDICATOR_HEIGHT);
	}
    }
}

static void
gtk_multi_option_menu_draw (GtkWidget    *widget,
			    GdkRectangle *area)
{
  GtkWidget *child;
  GdkRectangle child_area;

  g_return_if_fail (widget != NULL);
  g_return_if_fail (GTK_IS_MULTI_OPTION_MENU (widget));
  g_return_if_fail (area != NULL);

  if (GTK_WIDGET_DRAWABLE (widget))
    {
      gtk_multi_option_menu_paint (widget, area);

      child = GTK_BUTTON (widget)->child;
      if (child && gtk_widget_intersect (child, area, &child_area))
	gtk_widget_draw (child, &child_area);
    }
}

static gint
gtk_multi_option_menu_expose (GtkWidget      *widget,
			      GdkEventExpose *event)
{
  GtkWidget *child;
  GdkEventExpose child_event;
  gint remove_child;

  g_return_val_if_fail (widget != NULL, FALSE);
  g_return_val_if_fail (GTK_IS_MULTI_OPTION_MENU (widget), FALSE);
  g_return_val_if_fail (event != NULL, FALSE);

  if (GTK_WIDGET_DRAWABLE (widget))
    {
      gtk_multi_option_menu_paint (widget, &event->area);

      remove_child = FALSE;
      child = GTK_BUTTON (widget)->child;

      if (!child)
	{
	  if (!GTK_MULTI_OPTION_MENU (widget)->menu)
	    return FALSE;
	  gtk_multi_option_menu_update_contents (GTK_MULTI_OPTION_MENU (widget));
	  child = GTK_BUTTON (widget)->child;
	  remove_child = TRUE;
	}

      child_event = *event;

      if (GTK_WIDGET_NO_WINDOW (child) &&
	  gtk_widget_intersect (child, &event->area, &child_event.area))
	gtk_widget_event (child, (GdkEvent*) &child_event);

      if (remove_child)
	gtk_multi_option_menu_remove_contents (GTK_MULTI_OPTION_MENU (widget));
    }

  return FALSE;
}

static gint
gtk_multi_option_menu_button_press (GtkWidget      *widget,
				    GdkEventButton *event)
{
  GtkMultiOptionMenu *multi_option_menu;

  g_return_val_if_fail (widget != NULL, FALSE);
  g_return_val_if_fail (GTK_IS_MULTI_OPTION_MENU (widget), FALSE);
  g_return_val_if_fail (event != NULL, FALSE);

  if ((event->type == GDK_BUTTON_PRESS) &&
      (event->button == 1))
    {
      multi_option_menu = GTK_MULTI_OPTION_MENU (widget);
      gtk_multi_option_menu_remove_contents (multi_option_menu);
      gtk_menu_popup (GTK_MENU (multi_option_menu->menu), NULL, NULL,
		      gtk_multi_option_menu_position, multi_option_menu,
		      event->button, event->time);
    }

  return FALSE;
}

static void
gtk_multi_option_menu_deactivate (GtkMenuShell  *menu_shell,
				  GtkMultiOptionMenu *multi_option_menu)
{
  g_return_if_fail (menu_shell != NULL);
  g_return_if_fail (multi_option_menu != NULL);
  g_return_if_fail (GTK_IS_MULTI_OPTION_MENU (multi_option_menu));

  gtk_multi_option_menu_update_contents (multi_option_menu);
}

static void
gtk_multi_option_menu_update_contents (GtkMultiOptionMenu *multi_option_menu)
{
  GtkWidget *child;
  GtkWidget *submenu;
  GtkWidget *menu_item;

  g_return_if_fail (multi_option_menu != NULL);
  g_return_if_fail (GTK_IS_MULTI_OPTION_MENU (multi_option_menu));

  if (multi_option_menu->menu)
    {
      gtk_multi_option_menu_remove_contents (multi_option_menu);

      menu_item = gtk_menu_get_active (GTK_MENU (multi_option_menu->menu));
      while (menu_item && GTK_IS_MENU_ITEM (menu_item) && GTK_MENU_ITEM (menu_item)->submenu)
	{
	  submenu = GTK_MENU_ITEM (menu_item)->submenu;
	  menu_item = gtk_menu_get_active (GTK_MENU (submenu));
	}
      multi_option_menu->menu_item = menu_item;
      
      if (multi_option_menu->menu_item)
	{
	  child = GTK_BIN (multi_option_menu->menu_item)->child;
	  if (child)
	    {
	      gtk_container_block_resize (GTK_CONTAINER (multi_option_menu));
	      if (GTK_WIDGET (multi_option_menu)->state != child->state)
		gtk_widget_set_state (child, GTK_WIDGET (multi_option_menu)->state);
	      gtk_widget_reparent (child, GTK_WIDGET (multi_option_menu));
	      gtk_container_unblock_resize (GTK_CONTAINER (multi_option_menu));
	    }

	  gtk_widget_size_allocate (GTK_WIDGET (multi_option_menu),
				    &(GTK_WIDGET (multi_option_menu)->allocation));

	  if (GTK_WIDGET_DRAWABLE (multi_option_menu))
	    gtk_widget_draw (GTK_WIDGET (multi_option_menu), NULL);
	}
    }
}

static void
gtk_multi_option_menu_remove_contents (GtkMultiOptionMenu *multi_option_menu)
{
  g_return_if_fail (multi_option_menu != NULL);
  g_return_if_fail (GTK_IS_MULTI_OPTION_MENU (multi_option_menu));

  if (GTK_BUTTON (multi_option_menu)->child)
    {
      gtk_container_block_resize (GTK_CONTAINER (multi_option_menu));
      if (GTK_WIDGET (multi_option_menu->menu_item)->state != GTK_BUTTON (multi_option_menu)->child->state)
	gtk_widget_set_state (GTK_BUTTON (multi_option_menu)->child,
			      GTK_WIDGET (multi_option_menu->menu_item)->state);
      GTK_WIDGET_UNSET_FLAGS (GTK_BUTTON (multi_option_menu)->child, GTK_MAPPED | GTK_REALIZED);
      gtk_widget_reparent (GTK_BUTTON (multi_option_menu)->child, multi_option_menu->menu_item);
      gtk_container_unblock_resize (GTK_CONTAINER (multi_option_menu));
      multi_option_menu->menu_item = NULL;
    }
}
static void
gtk_multi_option_menu_calc_size (GtkMultiOptionMenu *multi_option_menu)
{
  g_return_if_fail (multi_option_menu != NULL);
  g_return_if_fail (GTK_IS_MULTI_OPTION_MENU (multi_option_menu));

  multi_option_menu->width = 0;
  multi_option_menu->height = 0;

  if (multi_option_menu->menu)
    {
      gtk_multi_option_menu_calc_size_recursive (multi_option_menu, multi_option_menu->menu);
    }
}


static void
gtk_multi_option_menu_calc_size_recursive (GtkMultiOptionMenu *multi_option_menu, GtkWidget *menu)
{
  GtkWidget *child;
  GList *children;

  g_return_if_fail (multi_option_menu != NULL);
  g_return_if_fail (GTK_IS_MULTI_OPTION_MENU (multi_option_menu));
  g_return_if_fail (menu != NULL);
  g_return_if_fail (GTK_IS_MENU (menu));

  children = GTK_MENU_SHELL (menu)->children;
  while (children)
    {
      child = children->data;
      children = children->next;

      if (GTK_WIDGET_VISIBLE (child))
	{
	  gtk_widget_size_request (child, &child->requisition);
	  
	  multi_option_menu->width = MAX (multi_option_menu->width, child->requisition.width);
	  multi_option_menu->height = MAX (multi_option_menu->height, child->requisition.height);
	  if (GTK_IS_MENU_ITEM (child) && GTK_MENU_ITEM (child)->submenu)
	    gtk_multi_option_menu_calc_size_recursive (multi_option_menu, GTK_MENU_ITEM (child)->submenu);
	}
    }
}


static void
gtk_multi_option_menu_position (GtkMenu  *menu,
				gint     *x,
				gint     *y,
				gpointer  user_data)
{
  GtkMultiOptionMenu *multi_option_menu;
  GtkWidget *active;
  GtkWidget *child;
  GList *children;
  gint shift_menu;
  gint screen_width;
  gint screen_height;
  gint menu_xpos;
  gint menu_ypos;
  gint width;
  gint height;

  g_return_if_fail (user_data != NULL);
  g_return_if_fail (GTK_IS_MULTI_OPTION_MENU (user_data));

  multi_option_menu = GTK_MULTI_OPTION_MENU (user_data);

  width = GTK_WIDGET (menu)->allocation.width;
  height = GTK_WIDGET (menu)->allocation.height;

  active = gtk_menu_get_active (GTK_MENU (multi_option_menu->menu));
  children = GTK_MENU_SHELL (multi_option_menu->menu)->children;
  gdk_window_get_origin (GTK_WIDGET (multi_option_menu)->window, &menu_xpos, &menu_ypos);

  menu_ypos += (GTK_WIDGET (multi_option_menu)->allocation.height - active->requisition.height) / 2 - 2;

  while (children)
    {
      child = children->data;

      if (active == child)
	break;

      menu_ypos -= child->allocation.height;
      children = children->next;
    }

  screen_width = gdk_screen_width ();
  screen_height = gdk_screen_height ();

  shift_menu = FALSE;
  if (menu_ypos < 0)
    {
      menu_ypos = 0;
      shift_menu = TRUE;
    }
  else if ((menu_ypos + height) > screen_height)
    {
      menu_ypos -= ((menu_ypos + height) - screen_height);
      shift_menu = TRUE;
    }

  if (shift_menu)
    {
      if ((menu_xpos + GTK_WIDGET (multi_option_menu)->allocation.width + width) <= screen_width)
	menu_xpos += GTK_WIDGET (multi_option_menu)->allocation.width;
      else
	menu_xpos -= width;
    }

  if (menu_xpos < 0)
    menu_xpos = 0;
  else if ((menu_xpos + width) > screen_width)
    menu_xpos -= ((menu_xpos + width) - screen_width);

  *x = menu_xpos;
  *y = menu_ypos;
}
