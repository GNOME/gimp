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

#include <stdlib.h>

#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>

#include "display-types.h"

#include "core/gimp.h"
#include "core/gimpcontext.h"
#include "core/gimpimage.h"
#include "core/gimplayer.h"
#include "core/gimptoolinfo.h"

#include "tools/gimpfuzzyselecttool.h"
#include "tools/gimpmovetool.h"
#include "tools/tool_manager.h"

#include "widgets/gimpcursor.h"
#include "widgets/gimpdialogfactory.h"
#include "widgets/gimpwidgets-utils.h"

#include "gui/dialogs.h"

#include "gimpdisplay.h"
#include "gimpdisplayshell.h"
#include "gimpdisplayshell-callbacks.h"
#include "gimpdisplayshell-layer-select.h"
#include "gimpdisplayshell-scale.h"
#include "gimpdisplayshell-scroll.h"
#include "gimpdisplayshell-selection.h"

#include "devices.h"
#include "gimprc.h"

#include "libgimp/gimpintl.h"


static void
gimp_display_shell_redraw (GimpDisplayShell *shell,
                           gint              x,
                           gint              y,
                           gint              w,
                           gint              h)
{
  glong x1, y1, x2, y2;    /*  coordinates of rectangle corners  */

  x1 = x;
  y1 = y;
  x2 = (x + w);
  y2 = (y + h);

  x1 = CLAMP (x1, 0, shell->disp_width);
  y1 = CLAMP (y1, 0, shell->disp_height);
  x2 = CLAMP (x2, 0, shell->disp_width);
  y2 = CLAMP (y2, 0, shell->disp_height);

  if ((x2 - x1) && (y2 - y1))
    {
      gimp_display_shell_add_expose_area (shell,
                                          x1, y1,
                                          (x2 - x1), (y2 - y1));
      gimp_display_shell_flush (shell);
    }
}

static void
gimp_display_shell_check_device_cursor (GimpDisplayShell *shell)
{
  GList *list;

  /*  gdk_devices_list() returns an internal list, so we shouldn't
   *  free it afterwards
   */
  for (list = gdk_devices_list (); list; list = g_list_next (list))
    {
      GdkDevice *device = (GdkDevice *) list->data;

      if (device == current_device)
	{
	  shell->draw_cursor = ! device->has_cursor;
	  break;
	}
    }
}

static GdkModifierType
gimp_display_shell_key_to_state (gint key)
{
  switch (key)
    {
    case GDK_Alt_L:
    case GDK_Alt_R:
      return GDK_MOD1_MASK;
    case GDK_Shift_L:
    case GDK_Shift_R:
      return GDK_SHIFT_MASK;
    case GDK_Control_L:
    case GDK_Control_R:
      return GDK_CONTROL_MASK;
    default:
      return 0;
    }
}

static void
gimp_display_shell_vscrollbar_update (GtkAdjustment    *adjustment,
                                      GimpDisplayShell *shell)
{
  gimp_display_shell_scroll (shell, 0, (adjustment->value - shell->offset_y));
}

static void
gimp_display_shell_hscrollbar_update (GtkAdjustment    *adjustment,
                                      GimpDisplayShell *shell)
{
  gimp_display_shell_scroll (shell, (adjustment->value - shell->offset_x), 0);
}

gboolean
gimp_display_shell_events (GtkWidget        *widget,
                           GdkEvent         *event,
                           GimpDisplayShell *shell)
{
  switch (event->type)
    {
    case GDK_KEY_PRESS:
    case GDK_BUTTON_PRESS:
    case GDK_SCROLL:
      /*  Setting the context's display automatically sets the image, too  */
      gimp_context_set_display (gimp_get_user_context (shell->gdisp->gimage->gimp),
				shell->gdisp);

      break;
    default:
      break;
    }

  return FALSE;
}

void
gimp_display_shell_canvas_realize (GtkWidget        *canvas,
                                   GimpDisplayShell *shell)
{
  GimpDisplay *gdisp;

  gdisp = shell->gdisp;

  gtk_widget_grab_focus (shell->canvas);

  gdk_window_set_back_pixmap (shell->canvas->window, NULL, FALSE);

  gimp_display_shell_resize_cursor_label (shell);
  gimp_display_shell_update_title (shell);

  /*  create the selection object  */
  shell->select = gimp_display_shell_selection_create (canvas->window,
                                                       shell,
                                                       gdisp->gimage->height,
                                                       gdisp->gimage->width, 
                                                       gimprc.marching_speed);

  shell->disp_width  = canvas->allocation.width;
  shell->disp_height = canvas->allocation.height;

  /*  create GC for scrolling  */
  shell->scroll_gc = gdk_gc_new (shell->canvas->window);
  gdk_gc_set_exposures (shell->scroll_gc, TRUE);

  /*  set up the scrollbar observers  */
  g_signal_connect (G_OBJECT (shell->hsbdata), "value_changed",
                    G_CALLBACK (gimp_display_shell_hscrollbar_update),
                    shell);
  g_signal_connect (G_OBJECT (shell->vsbdata), "value_changed",
                    G_CALLBACK (gimp_display_shell_vscrollbar_update),
                    shell);

  /*  setup scale properly  */
  gimp_display_shell_scale_setup (shell);

  /*  set the initial cursor  */
  gimp_display_shell_install_tool_cursor (shell,
                                          GDK_TOP_LEFT_ARROW,
                                          GIMP_TOOL_CURSOR_NONE,
                                          GIMP_CURSOR_MODIFIER_NONE);
}

static void
gimp_display_shell_get_device_state (GimpDisplayShell *shell,
                                     GdkDevice        *device,
                                     GdkModifierType  *state)
{
  gdk_device_get_state (device, shell->canvas->window, NULL, state);
}

static void
gimp_display_shell_get_device_coords (GimpDisplayShell *shell,
                                      GdkDevice        *device,
                                      GimpCoords       *coords)
{
  gdouble axes[GDK_AXIS_LAST];

  gdk_device_get_state (device, shell->canvas->window, axes, NULL);

  gdk_device_get_axis (device, axes, GDK_AXIS_X,        &coords->x);
  gdk_device_get_axis (device, axes, GDK_AXIS_Y,        &coords->y);
  gdk_device_get_axis (device, axes, GDK_AXIS_PRESSURE, &coords->pressure);
  gdk_device_get_axis (device, axes, GDK_AXIS_XTILT,    &coords->xtilt);
  gdk_device_get_axis (device, axes, GDK_AXIS_YTILT,    &coords->ytilt);
  gdk_device_get_axis (device, axes, GDK_AXIS_WHEEL,    &coords->wheel);
}

static gboolean
gimp_display_shell_get_coords (GimpDisplayShell *shell,
                               GdkEvent         *event,
                               GdkDevice        *device,
                               GimpCoords       *coords)
{
  /*  initialize extended axes to something meaningful because each of
   *  the following *_get_axis() calls may return FALSE and leave the
   *  passed gdouble location untouched
   */
  coords->pressure = 1.0;
  coords->xtilt    = 0.5;
  coords->ytilt    = 0.5;
  coords->wheel    = 0.5;

  if (gdk_event_get_axis (event, GDK_AXIS_X, &coords->x))
    {
      gdk_event_get_axis (event, GDK_AXIS_Y,        &coords->y);
      gdk_event_get_axis (event, GDK_AXIS_PRESSURE, &coords->pressure);
      gdk_event_get_axis (event, GDK_AXIS_XTILT,    &coords->xtilt);
      gdk_event_get_axis (event, GDK_AXIS_YTILT,    &coords->ytilt);
      gdk_event_get_axis (event, GDK_AXIS_WHEEL,    &coords->wheel);

      return TRUE;
    }

  gimp_display_shell_get_device_coords (shell, device, coords);

  return FALSE;
}

static gboolean
gimp_display_shell_get_state (GimpDisplayShell *shell,
                              GdkEvent         *event,
                              GdkDevice        *device,
                              GdkModifierType  *state)
{
  if (gdk_event_get_state (event, state))
    {
      return TRUE;
    }

  gimp_display_shell_get_device_state (shell, device, state);

  return FALSE;
}

gboolean
gimp_display_shell_canvas_events (GtkWidget        *canvas,
                                  GdkEvent         *event,
                                  GimpDisplayShell *shell)
{
  GimpDisplay     *gdisp;
  GimpTool        *active_tool;
  GdkEventExpose  *eevent;
  GdkEventMotion  *mevent;
  GdkEventButton  *bevent;
  GdkEventScroll  *sevent;
  GdkEventKey     *kevent;
  GimpCoords       display_coords;
  GimpCoords       image_coords;
  GdkModifierType  state;
  guint32          time;
  gboolean         return_val    = FALSE;
  gboolean         update_cursor = FALSE;

  static gboolean  scrolling      = FALSE;
  static gint      scroll_start_x = 0;
  static gint      scroll_start_y = 0;
  static guint     key_signal_id  = 0;

  if (! canvas->window)
    {
      g_warning ("gimp_display_shell_canvas_events(): called unrealized");
      return FALSE;
    }

  gdisp = shell->gdisp;

  active_tool = tool_manager_get_active (gdisp->gimage->gimp);

  /*  Find out what device the event occurred upon  */
  if (! gdisp->gimage->gimp->busy && devices_check_change (event))
    gimp_display_shell_check_device_cursor (shell);

  gimp_display_shell_get_coords (shell, event, current_device, &display_coords);
  gimp_display_shell_get_state (shell, event, current_device, &state);
  time = gdk_event_get_time (event);

  image_coords = display_coords;

  /*  GimpCoords passed to tools are ALWAYS in image coordinates  */
  gimp_display_shell_untransform_coords (shell,
                                         &display_coords,
                                         &image_coords);

  switch (event->type)
    {
    case GDK_EXPOSE:
      eevent = (GdkEventExpose *) event;

      gimp_display_shell_redraw (shell,
                                 eevent->area.x, eevent->area.y,
                                 eevent->area.width, eevent->area.height);
      break;

    case GDK_CONFIGURE:
      if ((shell->disp_width  != shell->canvas->allocation.width) ||
	  (shell->disp_height != shell->canvas->allocation.height))
	{
	  shell->disp_width  = shell->canvas->allocation.width;
	  shell->disp_height = shell->canvas->allocation.height;

	  gimp_display_shell_scale_resize (shell, FALSE, FALSE);
	}
      break;

    case GDK_ENTER_NOTIFY:
      if (((GdkEventCrossing *) event)->mode != GDK_CROSSING_NORMAL)
	return TRUE;

      /*  press modifier keys when entering the canvas  */
      if (active_tool && state)
        {
          if (state & GDK_SHIFT_MASK)
            gimp_tool_modifier_key (active_tool,
                                    GDK_SHIFT_MASK, TRUE, state,
                                    gdisp);
          if (state & GDK_CONTROL_MASK)
            gimp_tool_modifier_key (active_tool,
                                    GDK_CONTROL_MASK, TRUE, state,
                                    gdisp);
          if (state & GDK_MOD1_MASK)
            gimp_tool_modifier_key (active_tool,
                                    GDK_MOD1_MASK, TRUE, state,
                                    gdisp);

          gimp_tool_oper_update (active_tool,
                                 &image_coords, state,
                                 gdisp);
        }
      break;

    case GDK_LEAVE_NOTIFY:
      if (((GdkEventCrossing *) event)->mode != GDK_CROSSING_NORMAL)
	return TRUE;

      gimp_display_shell_update_cursor (shell, -1, -1);

      shell->proximity = FALSE;

      /*  release modifier keys when leaving the canvas  */
      if (active_tool && state)
        {
          if (state & GDK_MOD1_MASK)
            gimp_tool_modifier_key (active_tool,
                                    GDK_MOD1_MASK, FALSE, 0,
                                    gdisp);
          if (state & GDK_CONTROL_MASK)
            gimp_tool_modifier_key (active_tool,
                                    GDK_CONTROL_MASK, FALSE, 0,
                                    gdisp);
          if (state & GDK_SHIFT_MASK)
            gimp_tool_modifier_key (active_tool,
                                    GDK_SHIFT_MASK, FALSE, 0,
                                    gdisp);

          gimp_tool_oper_update (active_tool,
                                 &image_coords, 0,
                                 gdisp);
        }
      break;

    case GDK_PROXIMITY_IN:
      break;

    case GDK_PROXIMITY_OUT:
      shell->proximity = FALSE;
      break;

    case GDK_BUTTON_PRESS:
      bevent = (GdkEventButton *) event;

      /*  ignore new mouse events  */
      if (gdisp->gimage->gimp->busy)
	return TRUE;

      switch (bevent->button)
	{
	case 1:
	  state |= GDK_BUTTON1_MASK;

	  gtk_grab_add (canvas);

	  /* This is a hack to prevent other stuff being run in the middle of
	   * a tool operation (like changing image types.... brrrr). We just
	   * block all the keypress event. A better solution is to implement
	   * some sort of locking for images.
	   * Note that this is dependent on specific GTK behavior, and isn't
	   * guaranteed to work in future versions of GTK.
	   * -Yosh
	   */
	  if (key_signal_id == 0)
	    key_signal_id = g_signal_connect (G_OBJECT (canvas),
                                              "key_press_event",
                                              G_CALLBACK (gtk_true),
                                              NULL);

	  /* FIXME!!! This code is ugly */

	  if (active_tool && (! gimp_image_is_empty (gdisp->gimage) ||
                              GIMP_IS_MOVE_TOOL (active_tool) /* EEK */))
	    {
	      if (active_tool->auto_snap_to)
		{
		  gimp_display_shell_snap_point (shell,
                                                 display_coords.x,
                                                 display_coords.y,
                                                 &display_coords.x,
                                                 &display_coords.y);

                  gimp_display_shell_untransform_coords (shell,
                                                         &display_coords,
                                                         &image_coords);

		  update_cursor = TRUE;
		}

	      /*  initialize the current tool if it has no drawable
               */
	      if (! active_tool->drawable)
                {
                  tool_manager_initialize_active (gdisp->gimage->gimp, gdisp);
                }
              else if ((gimp_image_active_drawable (gdisp->gimage) !=
                        active_tool->drawable) &&
                       ! active_tool->preserve)
		{
                  /*  create a new one, deleting the current
                   */
                  gimp_context_tool_changed (gimp_get_user_context (gdisp->gimage->gimp));

                  active_tool = tool_manager_get_active (gdisp->gimage->gimp);
	      
		  tool_manager_initialize_active (gdisp->gimage->gimp, gdisp);
                }

	      gimp_tool_button_press (active_tool,
                                      &image_coords, time, state,
                                      gdisp);
	    } 
	  break;

	case 2:
          state |= GDK_BUTTON2_MASK;

          scrolling = TRUE;

          scroll_start_x = bevent->x + shell->offset_x;
          scroll_start_y = bevent->y + shell->offset_y;

          gtk_grab_add (canvas);

          gimp_display_shell_install_override_cursor (shell, GDK_FLEUR);
	  break;

	case 3:
	  state |= GDK_BUTTON3_MASK;
	  gimp_item_factory_popup_with_data (shell->ifactory, gdisp->gimage);
	  return_val = TRUE;
	  break;

	default:
	  break;
	}
      break;

    case GDK_BUTTON_RELEASE:
      bevent = (GdkEventButton *) event;

      /*  ugly side condition: all operations which set busy cursors are
       *  invoked on BUTTON_RELEASE, thus no new BUTTON_PRESS events are
       *  accepted while Gimp is busy, thus it should be safe to block
       *  BUTTON_RELEASE.  --Mitch
       *
       *  ugly: fuzzy_select sets busy cursors while ACTIVE.
       */
      if (gdisp->gimage->gimp->busy &&
	  ! (GIMP_IS_FUZZY_SELECT_TOOL (active_tool) &&
	     active_tool->state == ACTIVE))
	return TRUE;

      switch (bevent->button)
	{
	case 1:
	  state &= ~GDK_BUTTON1_MASK;

	  /* Lame hack. See above */
	  if (key_signal_id)
	    {
	      g_signal_handler_disconnect (G_OBJECT (canvas), key_signal_id);
	      key_signal_id = 0;
	    }

	  gtk_grab_remove (canvas);

	  if (active_tool && (GIMP_IS_MOVE_TOOL (active_tool) ||
			      ! gimp_image_is_empty (gdisp->gimage)))
	    {
	      if (active_tool->state == ACTIVE)
		{
		  if (active_tool->auto_snap_to)
		    {
		      gimp_display_shell_snap_point (shell,
                                                     display_coords.x,
                                                     display_coords.y,
                                                     &display_coords.x,
                                                     &display_coords.y);

                      gimp_display_shell_untransform_coords (shell,
                                                             &display_coords,
                                                             &image_coords);

		      update_cursor = TRUE;
		    }

		  gimp_tool_button_release (active_tool,
                                            &image_coords, time, state,
                                            gdisp);

                  active_tool = tool_manager_get_active (gdisp->gimage->gimp);

                  gimp_tool_oper_update (active_tool,
                                         &image_coords, state,
                                         gdisp);
		}
	    }
	  break;

	case 2:
	  state &= ~GDK_BUTTON2_MASK;

	  scrolling = FALSE;

          scroll_start_x = 0;
          scroll_start_y = 0;

	  gtk_grab_remove (canvas);

          gimp_display_shell_remove_override_cursor (shell);
	  break;

	case 3:
	  state &= ~GDK_BUTTON3_MASK;
	  break;

	default:
	  break;
	}
      break;

    case GDK_SCROLL:
      sevent = (GdkEventScroll *) event;

      if (state & GDK_SHIFT_MASK)
	{
	  if (sevent->direction == GDK_SCROLL_UP)
	    gimp_display_shell_scale (shell, GIMP_ZOOM_IN);
	  else
	    gimp_display_shell_scale (shell, GIMP_ZOOM_OUT);
	}
      else
	{
	  GtkAdjustment *adj;
	  gdouble        value;

	  if (state & GDK_CONTROL_MASK)
	    adj = shell->hsbdata;
	  else
	    adj = shell->vsbdata;

	  value = adj->value + ((sevent->direction == GDK_SCROLL_UP) ?
				-adj->page_increment / 2 :
				adj->page_increment / 2);
	  value = CLAMP (value, adj->lower, adj->upper - adj->page_size);

	  gtk_adjustment_set_value (adj, value);
	}

      gimp_display_shell_untransform_coords (shell,
                                             &display_coords,
                                             &image_coords);

      if (active_tool)
        {
          gimp_tool_oper_update (active_tool,
                                 &image_coords, state,
                                 gdisp);
        }

      return_val = TRUE;
      break;

    case GDK_MOTION_NOTIFY:
      mevent = (GdkEventMotion *) event;

      /*  for the same reason we block BUTTON_RELEASE,
       *  we block MOTION_NOTIFY.  --Mitch
       *
       *  ugly: fuzzy_select sets busy cursors while ACTIVE.
       */
      if (gdisp->gimage->gimp->busy &&
	  ! (GIMP_IS_FUZZY_SELECT_TOOL (active_tool) &&
	     active_tool->state == ACTIVE))
	return TRUE;

      /* Ask for the pointer position, but ignore it except for cursor
       * handling, so motion events sync with the button press/release events 
       */
      if (mevent->is_hint)
	{
          gimp_display_shell_get_device_coords (shell,
                                                mevent->device,
                                                &display_coords);
	}

      update_cursor = TRUE;

      if (! shell->proximity)
	{
	  shell->proximity = TRUE;
	  gimp_display_shell_check_device_cursor (shell);
	}

      if ((state & GDK_BUTTON1_MASK) &&

          active_tool && (! gimp_image_is_empty (gdisp->gimage) ||
                          GIMP_IS_MOVE_TOOL (active_tool)))
	{
	  if (active_tool->state == ACTIVE)
	    {
	      /*  if the first mouse button is down, check for automatic
	       *  scrolling...
	       */
	      if ((mevent->x < 0                 ||
                   mevent->y < 0                 ||
                   mevent->x > shell->disp_width ||
                   mevent->y > shell->disp_height) &&
                  ! active_tool->scroll_lock)
                {
                  gint off_x, off_y;

                  off_x = off_y = 0;

                  /*  The cases for scrolling  */
                  if (mevent->x < 0)
                    off_x = mevent->x;
                  else if (mevent->x > shell->disp_width)
                    off_x = mevent->x - shell->disp_width;

                  if (mevent->y < 0)
                    off_y = mevent->y;
                  else if (mevent->y > shell->disp_height)
                    off_y = mevent->y - shell->disp_height;

                  if (gimp_display_shell_scroll (shell, off_x, off_y))
                    {
                      GimpCoords device_coords;

                      gimp_display_shell_get_device_coords (shell,
                                                            mevent->device,
                                                            &device_coords);

                      if (device_coords.x == mevent->x &&
                          device_coords.y == mevent->y)
                        {
                          /*  Put this event back on the queue
                           *  so it keeps scrolling
                           */
                          gdk_event_put ((GdkEvent *) mevent);
                        }
                    }
                }

	      if (active_tool->auto_snap_to)
		{
		  gimp_display_shell_snap_point (shell,
                                                 display_coords.x,
                                                 display_coords.y,
                                                 &display_coords.x,
                                                 &display_coords.y);

                  gimp_display_shell_untransform_coords (shell,
                                                         &display_coords,
                                                         &image_coords);

		  update_cursor = TRUE;
		}

	      gimp_tool_motion (active_tool,
                                &image_coords, time, state,
                                gdisp);
	    }
	}
      else if ((state & GDK_BUTTON2_MASK) && scrolling)
	{
          gimp_display_shell_scroll (shell,
                                     (scroll_start_x - mevent->x -
                                      shell->offset_x),
                                     (scroll_start_y - mevent->y -
                                     shell->offset_y));
	}

      if (active_tool && 
	  ! (state & (GDK_BUTTON1_MASK | GDK_BUTTON2_MASK | GDK_BUTTON3_MASK)))
        {
          gimp_tool_oper_update (active_tool,
                                 &image_coords, state,
                                 gdisp);
        }

      break;

    case GDK_KEY_PRESS:
      kevent = (GdkEventKey *) event;

      /*  ignore any key presses  */
      if (gdisp->gimage->gimp->busy)
	return TRUE;

      switch (kevent->keyval)
	{
          GdkModifierType key;

	case GDK_Left: case GDK_Right:
	case GDK_Up: case GDK_Down:
	  if (active_tool && ! gimp_image_is_empty (gdisp->gimage))
            {
              gimp_tool_arrow_key (active_tool,
                                   kevent,
                                   gdisp);
            }

	  return_val = TRUE;
	  break;

	case GDK_Tab:
	  if ((state & GDK_MOD1_MASK) &&
              ! gimp_image_is_empty (gdisp->gimage))
            {
              gimp_display_shell_layer_select_init (gdisp->gimage,
                                                    1, kevent->time);
            }
	  else if ((state & GDK_CONTROL_MASK) &&
                   ! gimp_image_is_empty (gdisp->gimage))
            {
              gimp_display_shell_layer_select_init (gdisp->gimage,
                                                    -1, kevent->time);
            }
	  else if (! state)
            {
              /* Hide or show all dialogs */

              gimp_dialog_factories_toggle (global_dialog_factory,
                                            "gimp:toolbox");
            }

	  return_val = TRUE;
	  break;

	  /*  Update the state based on modifiers being pressed  */
	case GDK_Alt_L: case GDK_Alt_R:
	case GDK_Shift_L: case GDK_Shift_R:
	case GDK_Control_L: case GDK_Control_R:
          key = gimp_display_shell_key_to_state (kevent->keyval);
	  state |= key;

	  /*  For all modifier keys: call the tools modifier_key *and*
           *  oper_update method so tools can choose if they are interested
           *  in the release itself or only in the resulting state
           */
	  if (active_tool && ! gimp_image_is_empty (gdisp->gimage))
	    {
              gimp_tool_modifier_key (active_tool,
                                      key, FALSE, state,
                                      gdisp);

	      return_val = TRUE;
	    }
	  break;
	}

      if (active_tool)
        {
          gimp_tool_oper_update (active_tool,
                                 &image_coords, state,
                                 gdisp);
        }
      break;

    case GDK_KEY_RELEASE:
      kevent = (GdkEventKey *) event;

      /*  ignore any key releases  */
      if (gdisp->gimage->gimp->busy)
	return TRUE;

      switch (kevent->keyval)
	{
          GdkModifierType key;

	  /*  Update the state based on modifiers being pressed  */
	case GDK_Alt_L: case GDK_Alt_R:
	case GDK_Shift_L: case GDK_Shift_R:
	case GDK_Control_L: case GDK_Control_R:
          key = gimp_display_shell_key_to_state (kevent->keyval);
	  state &= ~key;

	  /*  For all modifier keys: call the tools modifier_key *and*
           *  oper_update method so tools can choose if they are interested
           *  in the press itself or only in the resulting state
           */
	  if (active_tool && ! gimp_image_is_empty (gdisp->gimage))
	    {
              gimp_tool_modifier_key (active_tool,
                                      key, TRUE, state,
                                      gdisp);
	    }
	  break;
	}

      if (active_tool)
        {
          gimp_tool_oper_update (active_tool,
                                 &image_coords, state,
                                 gdisp);
        }

      return_val = TRUE;
      break;

    default:
      break;
    }

  /*  if we reached this point in gimp_busy mode, return now  */
  if (gdisp->gimage->gimp->busy)
    return TRUE;

  /*  cursor update support  */

  if (! gimprc.no_cursor_updating)
    {
      active_tool = tool_manager_get_active (gdisp->gimage->gimp);

      if (active_tool)
        {
          if (! gimp_image_is_empty (gdisp->gimage) &&
              ! (state & (GDK_BUTTON1_MASK |
                          GDK_BUTTON2_MASK |
                          GDK_BUTTON3_MASK)))
            {
              gimp_tool_cursor_update (active_tool,
                                       &image_coords, state,
                                       gdisp);
            }
          else if (gimp_image_is_empty (gdisp->gimage))
            {
              gimp_display_shell_install_tool_cursor (shell,
                                                      GIMP_BAD_CURSOR,
                                                      active_tool->tool_cursor,
                                                      GIMP_CURSOR_MODIFIER_NONE);
            }
        }
      else
        {
          gimp_display_shell_install_tool_cursor (shell,
                                                  GIMP_BAD_CURSOR,
                                                  GIMP_TOOL_CURSOR_NONE,
                                                  GIMP_CURSOR_MODIFIER_NONE);
        }
    }

  if (update_cursor)
    {
      gimp_display_shell_update_cursor (shell,
                                        display_coords.x,
                                        display_coords.y);
    }

  return return_val;
}

gboolean
gimp_display_shell_hruler_button_press (GtkWidget        *widget,
                                        GdkEventButton   *event,
                                        GimpDisplayShell *shell)
{
  GimpDisplay *gdisp;

  gdisp = shell->gdisp;

  if (gdisp->gimage->gimp->busy)
    return TRUE;

  if (event->button == 1)
    {
      GimpToolInfo *tool_info;
      GimpTool     *active_tool;

      tool_info = tool_manager_get_info_by_type (gdisp->gimage->gimp,
						 GIMP_TYPE_MOVE_TOOL);

      if (tool_info)
	{
	  gimp_context_set_tool (gimp_get_user_context (gdisp->gimage->gimp),
				 tool_info);

	  active_tool = tool_manager_get_active (gdisp->gimage->gimp);

	  if (active_tool)
	    {
	      gimp_move_tool_start_hguide (active_tool, gdisp);
	      gtk_grab_add (shell->canvas);
	    }
	}
    }

  return FALSE;
}

gboolean
gimp_display_shell_vruler_button_press (GtkWidget        *widget,
                                        GdkEventButton   *event,
                                        GimpDisplayShell *shell)
{
  GimpDisplay *gdisp;

  gdisp = shell->gdisp;

  if (gdisp->gimage->gimp->busy)
    return TRUE;

  if (event->button == 1)
    {
      GimpToolInfo *tool_info;
      GimpTool     *active_tool;

      tool_info = tool_manager_get_info_by_type (gdisp->gimage->gimp,
						 GIMP_TYPE_MOVE_TOOL);

      if (tool_info)
	{
	  gimp_context_set_tool (gimp_get_user_context (gdisp->gimage->gimp),
				 tool_info);

	  active_tool = tool_manager_get_active (gdisp->gimage->gimp);

	  if (active_tool)
	    {
	      gimp_move_tool_start_vguide (active_tool, gdisp);
	      gtk_grab_add (shell->canvas);
	    }
	}
    }

  return FALSE;
}

static void
gimp_display_shell_origin_menu_position (GtkMenu  *menu,
                                         gint     *x,
                                         gint     *y,
                                         gpointer  data)
{
  GtkWidget *origin;
  gint       origin_x;
  gint       origin_y;

  origin = (GtkWidget *) data;

  gdk_window_get_origin (origin->window, &origin_x, &origin_y);

  *x = origin_x + origin->allocation.x + origin->allocation.width - 1;
  *y = origin_y + origin->allocation.y + (origin->allocation.height - 1) / 2;

  if (*x + GTK_WIDGET (menu)->allocation.width > gdk_screen_width ())
    *x -= (GTK_WIDGET (menu)->allocation.width + origin->allocation.width);

  if (*y + GTK_WIDGET (menu)->allocation.height > gdk_screen_height ())
    *y -= (GTK_WIDGET (menu)->allocation.height);
}

gboolean
gimp_display_shell_origin_button_press (GtkWidget        *widget,
                                        GdkEventButton   *event,
                                        GimpDisplayShell *shell)
{
  GimpDisplay *gdisp;

  gdisp = shell->gdisp;

  if (! gdisp->gimage->gimp->busy && event->button == 1)
    {
      gint x, y;

      gimp_display_shell_origin_menu_position (GTK_MENU (shell->ifactory->widget),
                                               &x, &y, widget);

      gtk_item_factory_popup_with_data (shell->ifactory,
					gdisp->gimage, NULL,
					x, y,
					1, event->time);
    }

  /* Return TRUE to stop signal emission so the button doesn't grab the
   * pointer away from us.
   */
  return TRUE;
}
