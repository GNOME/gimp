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
#include <stdlib.h>
#include "gdk/gdkkeysyms.h"
#include "appenv.h"
#include "colormaps.h"
#include "cursorutil.h"
#include "disp_callbacks.h"
#include "gdisplay.h"
#include "general.h"
#include "gimprc.h"
#include "interface.h"
#include "layer_select.h"
#include "move.h"
#include "scale.h"
#include "scroll.h"
#include "tools.h"
#include "gimage.h"


#define HORIZONTAL  1
#define VERTICAL    2

/* force a busy cursor */
#define BUSY_OFF   0
#define BUSY_UP    1
#define BUSY_ON    2
#define BUSY_DOWN  3

static guint gdisp_busy = BUSY_OFF;


static void
redraw (GDisplay *gdisp,
	int       x,
	int       y,
	int       w,
	int       h)
{
  long x1, y1, x2, y2;    /*  coordinate of rectangle corners  */

  x1 = x;
  y1 = y;
  x2 = (x+w);
  y2 = (y+h);

  x1 = BOUNDS (x1, 0, gdisp->disp_width);
  y1 = BOUNDS (y1, 0, gdisp->disp_height);
  x2 = BOUNDS (x2, 0, gdisp->disp_width);
  y2 = BOUNDS (y2, 0, gdisp->disp_height);

  if ((x2 - x1) && (y2 - y1))
    {
      gdisplay_expose_area (gdisp, x1, y1, (x2 - x1), (y2 - y1));
      gdisplays_flush ();
    }
}

gint
gdisplay_canvas_events (GtkWidget *canvas,
			GdkEvent  *event)
{
  GDisplay *gdisp;
  GdkEventExpose *eevent;
  GdkEventMotion *mevent;
  GdkEventButton *bevent;
  GdkEventKey *kevent;
  gint tx, ty;
  GdkModifierType tmask;
  guint state = 0;
  gint return_val = FALSE;
  static gboolean scrolled = FALSE;
  static guint key_signal_id = 0;

  gdisp = (GDisplay *) gtk_object_get_user_data (GTK_OBJECT (canvas));

  if (!canvas->window) 
    return FALSE;

  /*  If this is the first event...  */
  if (!gdisp->select)
    {
      /*  create the selection object  */
      gdisp->select = selection_create (gdisp->canvas->window, gdisp,
					gdisp->gimage->height,
					gdisp->gimage->width, marching_speed);

      gdisp->disp_width = gdisp->canvas->allocation.width;
      gdisp->disp_height = gdisp->canvas->allocation.height;

      /*  create GC for scrolling */
 
      gdisp->scroll_gc = gdk_gc_new (gdisp->canvas->window);
      gdk_gc_set_exposures (gdisp->scroll_gc, TRUE);

      /*  set up the scrollbar observers  */
      gtk_signal_connect (GTK_OBJECT (gdisp->hsbdata), "value_changed",
			  (GtkSignalFunc) scrollbar_horz_update,
			  gdisp);
      gtk_signal_connect (GTK_OBJECT (gdisp->vsbdata), "value_changed",
			  (GtkSignalFunc) scrollbar_vert_update,
			  gdisp);

      /*  setup scale properly  */
      setup_scale (gdisp);
    }

  /*  get the pointer position  */
  gdk_window_get_pointer (canvas->window, &tx, &ty, &tmask);

  switch (event->type)
    {
    case GDK_EXPOSE:
      eevent = (GdkEventExpose *) event;
      redraw (gdisp, eevent->area.x, eevent->area.y,
	      eevent->area.width, eevent->area.height);
      break;

    case GDK_CONFIGURE:
      if ((gdisp->disp_width != gdisp->canvas->allocation.width) ||
	  (gdisp->disp_height != gdisp->canvas->allocation.height))
	{
	  gdisp->disp_width = gdisp->canvas->allocation.width;
	  gdisp->disp_height = gdisp->canvas->allocation.height;
	  resize_display (gdisp, 0, FALSE);
	}
      break;

    case GDK_BUTTON_PRESS:
      bevent = (GdkEventButton *) event;
      state = bevent->state;

      switch (bevent->button)
	{
	case 1:
	  gtk_grab_add (canvas);

	  /* This is a hack to prevent other stuff being run in the middle of
	     a tool operation (like changing image types.... brrrr). We just
	     block all the keypress event. A better solution is to implement
	     some sort of locking for images.
	     Note that this is dependent on specific GTK behavior, and isn't
	     guaranteed to work in future versions of GTK.
	     -Yosh
	   */
	  if (key_signal_id == 0)
	    key_signal_id = gtk_signal_connect (GTK_OBJECT (canvas),
						"key_press_event",
						GTK_SIGNAL_FUNC (gtk_true),
						NULL);

	  if (active_tool && ((active_tool->type == MOVE) ||
			      !gimage_is_empty (gdisp->gimage)))
	      {
		if (active_tool->auto_snap_to)
		  {
		    gdisplay_snap_point (gdisp, bevent->x, bevent->y, &tx, &ty);
		    bevent->x = tx;
		    bevent->y = ty;
		  }
		
		/* reset the current tool if we're changing gdisplays */
		/*
		if (active_tool->gdisp_ptr) {
		  tool_gdisp = active_tool->gdisp_ptr;
		  if (tool_gdisp->ID != gdisp->ID) {
		    tools_initialize (active_tool->type, gdisp);
		    active_tool->drawable = gimage_active_drawable(gdisp->gimage);
		  }
		} else
		*/
		/* reset the current tool if we're changing drawables */
		  if (active_tool->drawable) {
		    if (((gimage_active_drawable(gdisp->gimage)) !=
			 active_tool->drawable) &&
			!active_tool->preserve)
		      tools_initialize (active_tool->type, gdisp);
		  } else
		    active_tool->drawable = gimage_active_drawable(gdisp->gimage);

                  /* hack hack hack */
                  if (no_cursor_updating == 0)
                    {
                      switch (active_tool->type)
                        {
                        case BEZIER_SELECT:
                          gdisplay_install_tool_cursor (gdisp, GDK_WATCH);
                          break;
                          
                        case FUZZY_SELECT:                          
                          gdisplay_install_tool_cursor (gdisp, GDK_WATCH);
                          gdisp_busy = BUSY_UP;
                          break;

                        default:
                          break;
                        }
                    }
                      
                  (* active_tool->button_press_func) (active_tool, bevent, gdisp);
	      }
	  break;

	case 2:
	  scrolled = TRUE;
	  gtk_grab_add (canvas);
	  start_grab_and_scroll (gdisp, bevent);
	  break;

	case 3:
	  popup_shell = gdisp->shell;
	  gdisplay_set_menu_sensitivity (gdisp);
	  gtk_menu_popup (GTK_MENU (gdisp->popup), NULL, NULL, NULL, NULL, 3, bevent->time);
	  break;

	default:
	  break;
	}
      break;

    case GDK_BUTTON_RELEASE:
      bevent = (GdkEventButton *) event;
      state = bevent->state;

      switch (bevent->button)
	{
	case 1:
	  /* Lame hack. See above */
	  if (key_signal_id)
	    {
	      gtk_signal_disconnect (GTK_OBJECT (canvas), key_signal_id);
	      key_signal_id = 0;
	    }

	  gtk_grab_remove (canvas);
	  gdk_pointer_ungrab (bevent->time);  /* fixes pointer grab bug */
	  if (active_tool && ((active_tool->type == MOVE) ||
			      !gimage_is_empty (gdisp->gimage)))
	    if (active_tool->state == ACTIVE)
	      {
		if (active_tool->auto_snap_to)
		  {
		    gdisplay_snap_point (gdisp, bevent->x, bevent->y, &tx, &ty);
		    bevent->x = tx;
		    bevent->y = ty;
		  }

                /* hack hack hack */
                if (no_cursor_updating == 0)
                  {
                    switch (active_tool->type)
                      {
                      case ROTATE:
                      case SCALE:
                      case SHEAR:
                      case PERSPECTIVE:

                      case RECT_SELECT:
                      case ELLIPSE_SELECT:
                      case FREE_SELECT:
                      case BEZIER_SELECT:
                      case FUZZY_SELECT:

                      case FLIP_HORZ:
                      case FLIP_VERT:
                      case BUCKET_FILL:
                      case BLEND:

                        gdisplay_install_tool_cursor (gdisp, GDK_WATCH);
                        gdisp_busy = BUSY_DOWN;
                        break;
                          
                      default:
                        break;
                      }
                  }
                
                (* active_tool->button_release_func) (active_tool, bevent, gdisp);
	      }
	  break;

	case 2:
	  scrolled = FALSE;
	  gtk_grab_remove (canvas);
	  end_grab_and_scroll (gdisp, bevent);
	  break;

	case 3:
	  break;

	default:
	  break;
	}
      break;

    case GDK_MOTION_NOTIFY:
      mevent = (GdkEventMotion *) event;
      state = mevent->state;

      if (mevent->is_hint)
	{
	  mevent->x = tx;
	  mevent->y = ty;
	  mevent->state = tmask;
	}

      if (active_tool && ((active_tool->type == MOVE) ||
			  !gimage_is_empty (gdisp->gimage)) &&
	  (mevent->state & GDK_BUTTON1_MASK))
	{
	  if (active_tool->state == ACTIVE)
	    {
	      /*  if the first mouse button is down, check for automatic
	       *  scrolling...
	       */
	      if ((mevent->state & Button1Mask) && !active_tool->scroll_lock)
		{
		  if (mevent->x < 0 || mevent->y < 0 ||
		      mevent->x > gdisp->disp_width ||
		      mevent->y > gdisp->disp_height)
		    scroll_to_pointer_position (gdisp, mevent);
		}

	      if (active_tool->auto_snap_to)
		{
		  gdisplay_snap_point (gdisp, mevent->x, mevent->y, &tx, &ty);
		  mevent->x = tx;
		  mevent->y = ty;
		}

	      (* active_tool->motion_func) (active_tool, mevent, gdisp);
	    }
	}
      else if ((mevent->state & GDK_BUTTON2_MASK) && scrolled)
	{
	  grab_and_scroll (gdisp, mevent);
	}
      break;

    case GDK_KEY_PRESS:
      kevent = (GdkEventKey *) event;
      state = kevent->state;

      switch (kevent->keyval)
	{
	case GDK_Left: case GDK_Right:
	case GDK_Up: case GDK_Down:
	  if (active_tool && !gimage_is_empty (gdisp->gimage))
	    (* active_tool->arrow_keys_func) (active_tool, kevent, gdisp);
	  return_val = TRUE;
	  break;

	case GDK_Tab:
	  if (kevent->state & GDK_MOD1_MASK && !gimage_is_empty (gdisp->gimage))
	    layer_select_init (gdisp->gimage, 1, kevent->time);
	  if (kevent->state & GDK_CONTROL_MASK && !gimage_is_empty (gdisp->gimage))
	    layer_select_init (gdisp->gimage, -1, kevent->time);
	  return_val = TRUE;
	  break;

	  /*  Update the state based on modifiers being pressed  */
	case GDK_Alt_L: case GDK_Alt_R:
	  state |= GDK_MOD1_MASK;
	  break;
	case GDK_Shift_L: case GDK_Shift_R:
	  state |= GDK_SHIFT_MASK;
	  break;
	case GDK_Control_L: case GDK_Control_R:
	  state |= GDK_CONTROL_MASK;
	  break;
	}

      /*  We need this here in case of accelerators  */
      gdisplay_set_menu_sensitivity (gdisp);
      break;

    case GDK_KEY_RELEASE:
      kevent = (GdkEventKey *) event;
      state = kevent->state;

      switch (kevent->keyval)
	{
	case GDK_Alt_L: case GDK_Alt_R:
	  state &= ~GDK_MOD1_MASK;
	  break;
	case GDK_Shift_L: case GDK_Shift_R:
	  kevent->state &= ~GDK_SHIFT_MASK;
	  break;
	case GDK_Control_L: case GDK_Control_R:
	  kevent->state &= ~GDK_CONTROL_MASK;
	  break;
	}

      return_val = TRUE;
      break;

    default:
      break;
    }

  if (no_cursor_updating == 0)
    {
      if (gdisp_busy == BUSY_UP)
        {
          gdisplay_install_tool_cursor (gdisp, GDK_WATCH);
          gdisp_busy = BUSY_ON;
        }
      else if (active_tool && !gimage_is_empty (gdisp->gimage) &&
	  !(state & (GDK_BUTTON1_MASK | GDK_BUTTON2_MASK | GDK_BUTTON3_MASK)))
	{
	  GdkEventMotion me;
	  me.x = tx;  me.y = ty;
	  me.state = state;
	  (* active_tool->cursor_update_func) (active_tool, &me, gdisp);
	}
      else if (gdisp_busy == BUSY_DOWN)
        {
          gdisplay_install_tool_cursor (gdisp, GDK_TOP_LEFT_ARROW);
          gdisp_busy = BUSY_OFF;
        }
      else if (gimage_is_empty (gdisp->gimage))
        {
          gdisplay_install_tool_cursor (gdisp, GDK_TOP_LEFT_ARROW);
        }
    }

  return return_val;
}

gint
gdisplay_hruler_button_press (GtkWidget      *widget,
			      GdkEventButton *event,
			      gpointer        data)
{
  GDisplay *gdisp;

  if (event->button == 1)
    {
      gdisp = data;

      gtk_widget_activate (tool_widgets[tool_info[(int) MOVE].toolbar_position]);
      move_tool_start_hguide (active_tool, gdisp);
      gtk_grab_add (gdisp->canvas);
    }

  return FALSE;
}

gint
gdisplay_vruler_button_press (GtkWidget      *widget,
			      GdkEventButton *event,
			      gpointer        data)
{
  GDisplay *gdisp;

  if (event->button == 1)
    {
      gdisp = data;

      gtk_widget_activate (tool_widgets[tool_info[(int) MOVE].toolbar_position]);
      move_tool_start_vguide (active_tool, gdisp);
      gtk_grab_add (gdisp->canvas);
    }

  return FALSE;
}
