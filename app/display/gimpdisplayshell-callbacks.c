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
#include "gdk/gdkkeysyms.h"
#include "appenv.h"
#include "bucket_fill.h"
#include "colormaps.h"
#include "cursorutil.h"
#include "devices.h"
#include "disp_callbacks.h"
#include "gdisplay.h"
#include "general.h"
#include "gimpcontext.h"
#include "gimprc.h"
#include "info_window.h"
#include "interface.h"
#include "layer_select.h"
#include "move.h"
#include "patterns.h"
#include "scale.h"
#include "scroll.h"
#include "tools.h"
#include "undo.h"
#include "gimage.h"
#include "dialog_handler.h"

#include "libgimp/gimpintl.h"

/* Function declarations */

static void gdisplay_check_device_cursor (GDisplay *gdisp);

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
      gdisplay_flush_displays_only (gdisp);
    }
}

static void
gdisplay_check_device_cursor (GDisplay *gdisp)
{
  GList *tmp_list;

  /* gdk_input_list_devices returns an internal list, so we shouldn't
     free it afterwards */
  tmp_list = gdk_input_list_devices();

  while (tmp_list)
    {
      GdkDeviceInfo *info = (GdkDeviceInfo *)tmp_list->data;

      if (info->deviceid == current_device)
	{
	  gdisp->draw_cursor = !info->has_cursor;
	  break;
	}
      
      tmp_list = tmp_list->next;
    }
}

static int
key_to_state (int key)
{
  switch (key)
  {
   case GDK_Alt_L: case GDK_Alt_R:
     return GDK_MOD1_MASK;
   case GDK_Shift_L: case GDK_Shift_R:
     return GDK_SHIFT_MASK;
   case GDK_Control_L: case GDK_Control_R:
     return GDK_CONTROL_MASK;
   default:
     return 0;
  }
}

gint
gdisplay_shell_events (GtkWidget *widget,
		       GdkEvent  *event,
		       GDisplay  *gdisp)
{
  switch (event->type)
    {
    case GDK_BUTTON_PRESS:
    case GDK_KEY_PRESS:
      /*  Setting the context's display automatically sets the image, too  */
      gimp_context_set_display (gimp_context_get_user (), gdisp);
      break;
    default:
      break;
    }

  return FALSE;
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
  gdouble tx, ty;
  guint state = 0;
  gint return_val = FALSE;
  static gboolean scrolled = FALSE;
  static guint key_signal_id = 0;
  int update_cursor = FALSE;

  tx = ty = 0;

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

      /*  create GC for scrolling  */
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

  /*  Find out what device the event occurred upon  */
  if (devices_check_change (event))
    gdisplay_check_device_cursor (gdisp);

  switch (event->type)
    {
    case GDK_EXPOSE:
      eevent = (GdkEventExpose *) event;
      /*printf(" EXP:%d,%d(%dx%d) ",eevent->area.x, eevent->area.y,
	eevent->area.width, eevent->area.height);fflush(stdout);*/
      redraw (gdisp, eevent->area.x, eevent->area.y,
	      eevent->area.width, eevent->area.height);
      break;

    case GDK_CONFIGURE:
      /*printf(" CNF ");fflush(stdout);*/
      if ((gdisp->disp_width != gdisp->canvas->allocation.width) ||
	  (gdisp->disp_height != gdisp->canvas->allocation.height))
	{
	  gdisp->disp_width = gdisp->canvas->allocation.width;
	  gdisp->disp_height = gdisp->canvas->allocation.height;
	  resize_display (gdisp, 0, FALSE);
	}
      break;

    case GDK_LEAVE_NOTIFY:
      gdisplay_update_cursor (gdisp, 0, 0);
      gtk_label_set_text (GTK_LABEL (gdisp->cursor_label), "");
      info_window_update_RGB(gdisp->window_info_dialog,
				 gdisp,
				 -1,
				 -1);

    case GDK_PROXIMITY_OUT:
      gdisp->proximity = FALSE;
      break;

    case GDK_ENTER_NOTIFY:
      /* Actually, should figure out tx,ty here */
      break;

    case GDK_BUTTON_PRESS:
      bevent = (GdkEventButton *) event;
      state = bevent->state;

      switch (bevent->button)
	{
	case 1:
	  state |= GDK_BUTTON1_MASK;
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
		  update_cursor = TRUE;
		}

	      /* reset the current tool if ... */
	      if ((/* it has no drawable */
		   ! active_tool->drawable ||

		   /* or a drawable different from the current one */
		   (gimage_active_drawable (gdisp->gimage) !=
		    active_tool->drawable)) &&

		  /* and doesn't want to be preserved across drawable changes */
		  ! active_tool->preserve)
		{
		  tools_initialize (active_tool->type, gdisp);
		}

	      /* otherwise set it's drawable if it has none */
	      else if (! active_tool->drawable)
		{
		  active_tool->drawable = gimage_active_drawable (gdisp->gimage);
		}

	      (* active_tool->button_press_func) (active_tool, bevent, gdisp);
	    }
	  break;

	case 2:
	  state |= GDK_BUTTON2_MASK;
	  scrolled = TRUE;
	  gtk_grab_add (canvas);
	  start_grab_and_scroll (gdisp, bevent);
	  break;

	case 3:
	  state |= GDK_BUTTON3_MASK;
	  gtk_menu_popup (GTK_MENU (gdisp->popup),
			  NULL, NULL, NULL, NULL, 3, bevent->time);
	  return_val = TRUE;
	  break;

	  /*  wheelmouse support  */
	case 4:
	  state |= GDK_BUTTON4_MASK;
	  if (state & GDK_SHIFT_MASK)
	    {
	      change_scale (gdisp, ZOOMIN);
	    }
	  else
	    {
	      GtkAdjustment *adj =
		(state & GDK_CONTROL_MASK) ? gdisp->hsbdata : gdisp->vsbdata;
	      gfloat new_value = adj->value - adj->page_increment / 2;
	      new_value =
		CLAMP (new_value, adj->lower, adj->upper - adj->page_size);
	      gtk_adjustment_set_value (adj, new_value);
	    }
	  return_val = TRUE;
	  break;

	case 5:
	  state |= GDK_BUTTON5_MASK;
	  if (state & GDK_SHIFT_MASK)
	    {
	      change_scale (gdisp, ZOOMOUT);
	    }
	  else
	    {
	      GtkAdjustment *adj =
		(state & GDK_CONTROL_MASK) ? gdisp->hsbdata : gdisp->vsbdata;
	      gfloat new_value = adj->value + adj->page_increment / 2;
	      new_value = CLAMP (new_value,
				 adj->lower, adj->upper - adj->page_size);
	      gtk_adjustment_set_value (adj, new_value);
	    }
	  return_val = TRUE;
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
	  state &= ~GDK_BUTTON1_MASK;
	  
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
	    {
	      if (active_tool->state == ACTIVE)
		{
		  if (active_tool->auto_snap_to)
		    {
		      gdisplay_snap_point (gdisp, bevent->x, bevent->y, &tx, &ty);
		      bevent->x = tx;
		      bevent->y = ty;
		      update_cursor = TRUE;
		    }
		  
		  (* active_tool->button_release_func) (active_tool, bevent,
							gdisp);
		}
	    }
	  break;

	case 2:
	  state &= ~GDK_BUTTON2_MASK;
	  scrolled = FALSE;
	  gtk_grab_remove (canvas);
	  end_grab_and_scroll (gdisp, bevent);
	  break;

	case 3:
	  state &= ~GDK_BUTTON3_MASK;
	  break;

	  /*  wheelmouse support  */
	case 4:
	  state &= ~GDK_BUTTON4_MASK;
	  return_val = TRUE;
	  break;

	case 5:
	  state &= ~GDK_BUTTON5_MASK;
	  return_val = TRUE;
	  break;

	default:
	  break;
	}
      break;

    case GDK_MOTION_NOTIFY:
      mevent = (GdkEventMotion *) event;
      state = mevent->state;

     /* Ask for the pointer position, but ignore it except for cursor
      * handling, so motion events sync with the button press/release events */

      if (mevent->is_hint)
	gdk_input_window_get_pointer (canvas->window, current_device, &tx, &ty,
#ifdef GTK_HAVE_SIX_VALUATORS
				      NULL, NULL, NULL, NULL, NULL);
#else /* !GTK_HAVE_SIX_VALUATORS */	
				      NULL, NULL, NULL, NULL);
#endif /* GTK_HAVE_SIX_VALUATORS */
      else
	{
	  tx = mevent->x;
	  ty = mevent->y;
	}
      update_cursor = TRUE;

      if (!gdisp->proximity)
	{
	  gdisp->proximity = TRUE;
	  gdisplay_check_device_cursor (gdisp);
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
	      if ((mevent->state & GDK_BUTTON1_MASK) && !active_tool->scroll_lock)
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
		  update_cursor = TRUE;
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

	  /* Hide or show all dialogs */
	  if (!kevent->state)
	    dialog_toggle();

	  return_val = TRUE;
	  break;

	  /*  Update the state based on modifiers being pressed  */
	case GDK_Alt_L: case GDK_Alt_R:
	case GDK_Shift_L: case GDK_Shift_R:
	case GDK_Control_L: case GDK_Control_R:
	  state |= key_to_state(kevent->keyval);
	  /* For all modifier keys: call the tools modifier_key_func */
	  if (active_tool && !gimage_is_empty (gdisp->gimage))
	    {
	      gdk_input_window_get_pointer (canvas->window, current_device,
#ifdef GTK_HAVE_SIX_VALUATORS 
					    &tx, &ty, NULL, NULL, NULL, NULL, NULL);
#else /* !GTK_HAVE_SIX_VALUATORS */
					    &tx, &ty, NULL, NULL, NULL, NULL);
#endif /* GTK_HAVE_SIX_VALUATORS */
	      (* active_tool->modifier_key_func) (active_tool, kevent, gdisp);
	      return_val = TRUE;
	    }
	  break;
	}
      break;

    case GDK_KEY_RELEASE:
      kevent = (GdkEventKey *) event;
      state = kevent->state;

      switch (kevent->keyval)
	{
	case GDK_Alt_L: case GDK_Alt_R:
	case GDK_Shift_L: case GDK_Shift_R:
	case GDK_Control_L: case GDK_Control_R:
	  state &= ~key_to_state(kevent->keyval);
	  /* For all modifier keys: call the tools modifier_key_func */
	  if (active_tool && !gimage_is_empty (gdisp->gimage))
	    {
	      gdk_input_window_get_pointer (canvas->window, current_device,
#ifdef GTK_HAVE_SIX_VALUATORS
                                            &tx, &ty, NULL, NULL, NULL, NULL, NULL);
#else /* !GTK_HAVE_SIX_VALUATORS */
                                            &tx, &ty, NULL, NULL, NULL, NULL);
#endif /* GTK_HAVE_SIX_VALUATORS */ 
	      (* active_tool->modifier_key_func) (active_tool, kevent, gdisp);
	      return_val = TRUE;
	    }
	  break;
	}

      return_val = TRUE;
      break;

    default:
      break;
    }

  if (no_cursor_updating == 0)
    {
      if (active_tool && !gimage_is_empty (gdisp->gimage) &&
	  !(state & (GDK_BUTTON1_MASK | GDK_BUTTON2_MASK | GDK_BUTTON3_MASK)))
	{
	  GdkEventMotion me;
	  me.x = tx;  me.y = ty;
	  me.state = state;
	  (* active_tool->cursor_update_func) (active_tool, &me, gdisp);
	}
      else if (gimage_is_empty (gdisp->gimage))
	gdisplay_install_tool_cursor (gdisp, GDK_TOP_LEFT_ARROW);
    }

  if (update_cursor)
    gdisplay_update_cursor (gdisp, tx, ty);

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

      gtk_widget_activate (tool_info[(int) MOVE].tool_widget);
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

      gtk_widget_activate (tool_info[(int) MOVE].tool_widget);
      move_tool_start_vguide (active_tool, gdisp);
      gtk_grab_add (gdisp->canvas);
    }

  return FALSE;
}

gint
gdisplay_origin_button_press (GtkWidget      *widget,
			      GdkEventButton *event,
			      gpointer        data)
{
  GDisplay *gdisp;

  if (event->button == 1)
    {
      gdisp = data;
      gtk_menu_popup (GTK_MENU (gdisp->popup),
		      NULL, NULL, NULL, NULL, 1, event->time);
    }

  /* Stop the signal emission so the button doesn't grab the
   * pointer away from us */
  gtk_signal_emit_stop_by_name (GTK_OBJECT (widget), "button_press_event");

  return FALSE;
}

gboolean
gdisplay_drag_drop (GtkWidget      *widget,
		    GdkDragContext *context,
		    gint            x,
		    gint            y,
		    guint           time,
		    gpointer        data)
{
  GDisplay  *gdisp;
  GtkWidget *src_widget;
  gboolean   return_val = FALSE;

  gdisp = (GDisplay *) data;

  if ((src_widget = gtk_drag_get_source_widget (context)))
    {
      GimpDrawable *drawable       = NULL;
      Layer        *layer          = NULL;
      Channel      *channel        = NULL;
      LayerMask    *layer_mask     = NULL;
      GImage       *component      = NULL;
      GPattern     *pattern        = NULL;
      ChannelType   component_type = -1;

      layer = (Layer *) gtk_object_get_data (GTK_OBJECT (src_widget),
					     "gimp_layer");
      channel = (Channel *) gtk_object_get_data (GTK_OBJECT (src_widget),
						 "gimp_channel");
      layer_mask = (LayerMask *) gtk_object_get_data (GTK_OBJECT (src_widget),
						      "gimp_layer_mask");
      component = (GImage *) gtk_object_get_data (GTK_OBJECT (src_widget),
						  "gimp_component");
      pattern = (GPattern *) gtk_object_get_data (GTK_OBJECT (src_widget),
						  "gimp_pattern");

      if (layer)
	{
	  drawable = GIMP_DRAWABLE (layer);
	}
      else if (channel)
	{
	  drawable = GIMP_DRAWABLE (channel);
	}
      else if (layer_mask)
	{
	  drawable = GIMP_DRAWABLE (layer_mask);
	}
      else if (component)
	{
	  component_type =
	    (ChannelType) gtk_object_get_data (GTK_OBJECT (src_widget),
					       "gimp_component_type");
	}

      /*  FIXME: implement special treatment of channel etc.
       */
      if (drawable)
	{
          GImage      *gimage;
	  Layer       *new_layer;
	  GImage      *dest_gimage;
	  gint         width, height;
	  gint         dest_width, dest_height;
	  gint         off_x, off_y;
	  TileManager *tiles;
	  PixelRegion  srcPR, destPR;
	  guchar       bg[MAX_CHANNELS];
	  gint         bytes, type;

	  gimage = gimp_drawable_gimage (drawable);
          width  = gimp_drawable_width  (drawable);
          height = gimp_drawable_height (drawable);

	  /*  How many bytes in the temp buffer?  */
	  switch (drawable_type (drawable))
	    {
	    case RGB_GIMAGE: case RGBA_GIMAGE:
	      bytes = 4; type = RGB;
	      break;
	    case GRAY_GIMAGE: case GRAYA_GIMAGE:
	      bytes = 2; type = GRAY;
	      break;
	    case INDEXED_GIMAGE: case INDEXEDA_GIMAGE:
	      bytes = 4; type = INDEXED;
	      break;
	    default:
	      bytes = 3; type = RGB;
	      break;
	    }

	  gimage_get_background (gimage, drawable, bg);

	  tiles = tile_manager_new (width, height, bytes);

	  pixel_region_init (&srcPR, drawable_data (drawable),
			     0, 0, width, height, FALSE);
	  pixel_region_init (&destPR, tiles,
			     0, 0, width, height, TRUE);

	  if (type == INDEXED)
	    /*  If the layer is indexed...we need to extract pixels  */
	    extract_from_region (&srcPR, &destPR, NULL,
				 drawable_cmap (drawable), bg, type,
				 drawable_has_alpha (drawable), FALSE);
	  else if (bytes > srcPR.bytes)
	    /*  If the layer doesn't have an alpha channel, add one  */
	    add_alpha_region (&srcPR, &destPR);
	  else
	    /*  Otherwise, do a straight copy  */
	    copy_region (&srcPR, &destPR);

	  dest_gimage = gdisp->gimage;
	  dest_width  = dest_gimage->width;
	  dest_height = dest_gimage->height;

	  new_layer =
	    layer_from_tiles (dest_gimage,
			      GIMP_DRAWABLE (gimage_get_active_layer (dest_gimage)),
			      tiles, _("Pasted Layer"),
			      OPAQUE_OPACITY, NORMAL_MODE);

	  tile_manager_destroy (tiles);

	  if (new_layer)
	    {
	      undo_push_group_start (dest_gimage, EDIT_PASTE_UNDO);

	      gimp_drawable_set_gimage (GIMP_DRAWABLE (new_layer), dest_gimage);

	      off_x = (dest_gimage->width - width) / 2;
	      off_y = (dest_gimage->height - height) / 2;

	      layer_translate (new_layer, off_x, off_y);

	      gimage_add_layer (dest_gimage, new_layer, -1);

	      gdisplays_flush ();

	      return_val = TRUE;

	      undo_push_group_end (dest_gimage);
	    }
	}
      
      if (pattern)
	{
	  GimpImage    *gimage;
	  GimpDrawable *drawable;
	  GimpContext  *fill_context;
	  TileManager  *buf_tiles;
	  PixelRegion   bufPR;
	  gint          x1, x2, y1, y2;
	  gint          bytes;
	  gboolean      has_alpha;

	  gimage = gdisp->gimage;
	  drawable = gimage_active_drawable (gimage);
	  if (drawable)
	    {
	      gimp_add_busy_cursors ();

	      /*  Get the fill parameters  */
	      if (gimp_context_get_current () == gimp_context_get_user () &&
		  ! global_paint_options)
		fill_context = tool_info[BUCKET_FILL].tool_context;
	      else
		fill_context = gimp_context_get_current ();

	      drawable_mask_bounds (drawable, &x1, &y1, &x2, &y2);
	      bytes = drawable_bytes (drawable);
	      has_alpha = drawable_has_alpha (drawable);

	        /*  Fill the region  */
	      buf_tiles = tile_manager_new ((x2 - x1), (y2 - y1), bytes);
	      pixel_region_init (&bufPR, buf_tiles, 0, 0, (x2 - x1), (y2 - y1), TRUE);
	      bucket_fill_region (PATTERN_BUCKET_FILL, &bufPR, NULL,
				  NULL, pattern->mask, x1, y1, has_alpha);
	      
	      /*  Apply it to the image  */
	      pixel_region_init (&bufPR, buf_tiles, 0, 0, (x2 - x1), (y2 - y1), FALSE);
	      gimage_apply_image (gimage, drawable, &bufPR, TRUE,
				  gimp_context_get_opacity (fill_context) * 255,
				  gimp_context_get_paint_mode (fill_context),
				  NULL, x1, y1);
	      tile_manager_destroy (buf_tiles);
	      
	      /*  Update the displays  */
	      drawable_update (drawable, x1, y1, (x2 - x1), (y2 - y1));
	      gdisplays_flush ();
	      
	      gimp_remove_busy_cursors (NULL);
	      
	      return_val = TRUE;
	    }
	}
    }

  gtk_drag_finish (context, return_val, FALSE, time);

  if (return_val)
    gimp_context_set_display (gimp_context_get_user (), gdisp);

  return return_val;
}

void
gdisplay_set_color (GtkWidget *widget,
		    guchar     r,
		    guchar     g,
		    guchar     b,
		    gpointer   data)
{
  GimpImage    *gimage;
  GimpDrawable *drawable;
  TileManager  *buf_tiles;
  PixelRegion   bufPR;
  GimpContext  *context;
  gint     x1, x2, y1, y2;
  gint     bytes;
  gboolean has_alpha;
  guchar   col[3];

  gimage = ((GDisplay *) data)->gimage;
  drawable = gimage_active_drawable (gimage);
  if (!drawable)
    return;

  gimp_add_busy_cursors ();

  /*  Get the fill parameters  */
  if (gimp_context_get_current () == gimp_context_get_user () &&
      ! global_paint_options)
    context = tool_info[BUCKET_FILL].tool_context;
  else
    context = gimp_context_get_current ();

  drawable_mask_bounds (drawable, &x1, &y1, &x2, &y2);

  bytes = drawable_bytes (drawable);
  has_alpha = drawable_has_alpha (drawable);

  col[0] = r;
  col[1] = g;
  col[2] = b;

  /*  Fill the region  */
  buf_tiles = tile_manager_new ((x2 - x1), (y2 - y1), bytes);
  pixel_region_init (&bufPR, buf_tiles, 0, 0, (x2 - x1), (y2 - y1), TRUE);
  bucket_fill_region (FG_BUCKET_FILL, &bufPR, NULL,
		      col, NULL, x1, y1, has_alpha);

  /*  Apply it to the image  */
  pixel_region_init (&bufPR, buf_tiles, 0, 0, (x2 - x1), (y2 - y1), FALSE);
  gimage_apply_image (gimage, drawable, &bufPR, TRUE,
		      gimp_context_get_opacity (context) * 255,
		      gimp_context_get_paint_mode (context),
		      NULL, x1, y1);
  tile_manager_destroy (buf_tiles);

  /*  Update the displays  */
  drawable_update (drawable, x1, y1, (x2 - x1), (y2 - y1));
  gdisplays_flush ();

  gimp_remove_busy_cursors (NULL);
}
