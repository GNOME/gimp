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

#include "display-types.h"

#include "core/gimpimage.h"

#include "widgets/gimpcursor.h"

#include "tools/gimptool.h"
#include "tools/tool_manager.h"

#include "gimpdisplay.h"
#include "gimpdisplay-scale.h"
#include "gimpdisplay-scroll.h"

#include "nav_window.h"


/*  This is the delay before dithering begins
 *  for example, after an operation such as scrolling
 */
#define DITHER_DELAY 250  /*  milliseconds  */

/*  STATIC variables  */
/*  These are the values of the initial pointer grab   */
static gint startx, starty;

void
start_grab_and_scroll (GimpDisplay    *gdisp,
		       GdkEventButton *bevent)
{
  GdkCursor *cursor;

  startx = bevent->x + gdisp->offset_x;
  starty = bevent->y + gdisp->offset_y;

  cursor = gimp_cursor_new (GDK_FLEUR,
			    GIMP_TOOL_CURSOR_NONE,
			    GIMP_CURSOR_MODIFIER_NONE);
  gdk_window_set_cursor (gdisp->canvas->window, cursor);
  gdk_cursor_unref (cursor);
}


void
end_grab_and_scroll (GimpDisplay    *gdisp,
		     GdkEventButton *bevent)
{
  gdisplay_real_install_tool_cursor (gdisp,
				     gdisp->current_cursor,
				     gdisp->tool_cursor,
				     GIMP_CURSOR_MODIFIER_NONE,
				     TRUE);
}


void
grab_and_scroll (GimpDisplay    *gdisp,
		 GdkEventMotion *mevent)
{
  if (mevent && mevent->window != gdisp->canvas->window)
    return;

  scroll_display (gdisp,
		  startx - mevent->x - gdisp->offset_x,
		  starty - mevent->y - gdisp->offset_y);
}


void
scroll_to_pointer_position (GimpDisplay    *gdisp,
			    GdkEventMotion *mevent)
{
  gdouble child_x, child_y;
  gint    off_x, off_y;

  off_x = off_y = 0;

  /*  The cases for scrolling  */
  if (mevent->x < 0)
    off_x = mevent->x;
  else if (mevent->x > gdisp->disp_width)
    off_x = mevent->x - gdisp->disp_width;

  if (mevent->y < 0)
    off_y = mevent->y;
  else if (mevent->y > gdisp->disp_height)
    off_y = mevent->y - gdisp->disp_height;

  if (scroll_display (gdisp, off_x, off_y))
    {
#ifdef __GNUC__
#warning FIXME: replace gdk_input_window_get_pointer()
#endif
#if 0
      gdk_input_window_get_pointer (gdisp->canvas->window, mevent->deviceid,
				    &child_x, &child_y, 
				    NULL, NULL, NULL, NULL);

      if (child_x == mevent->x && child_y == mevent->y)
	/*  Put this event back on the queue -- so it keeps scrolling */
	gdk_event_put ((GdkEvent *) mevent);
#endif
    }
}

gboolean
scroll_display (GimpDisplay *gdisp,
		gint      x_offset,
		gint      y_offset)
{
  gint      old_x, old_y;
  gint      src_x, src_y;
  gint      dest_x, dest_y;
  GdkEvent *event;

  old_x = gdisp->offset_x;
  old_y = gdisp->offset_y;

  gdisp->offset_x += x_offset;
  gdisp->offset_y += y_offset;

  bounds_checking (gdisp);

  /*  the actual changes in offset  */
  x_offset = (gdisp->offset_x - old_x);
  y_offset = (gdisp->offset_y - old_y);

  if (x_offset || y_offset)
    {
      setup_scale (gdisp);

      src_x = (x_offset < 0) ? 0 : x_offset;
      src_y = (y_offset < 0) ? 0 : y_offset;
      dest_x = (x_offset < 0) ? -x_offset : 0;
      dest_y = (y_offset < 0) ? -y_offset : 0;

      /*  reset the old values so that the tool can accurately redraw  */
      gdisp->offset_x = old_x;
      gdisp->offset_y = old_y;

      /*  freeze the active tool  */
      tool_manager_control_active (gdisp->gimage->gimp, PAUSE, gdisp);

      /*  set the offsets back to the new values  */
      gdisp->offset_x += x_offset;
      gdisp->offset_y += y_offset;

      gdk_draw_drawable (gdisp->canvas->window,
			 gdisp->scroll_gc,
			 gdisp->canvas->window,
			 src_x, src_y,
			 dest_x, dest_y,
			 (gdisp->disp_width - abs (x_offset)),
			 (gdisp->disp_height - abs (y_offset)));

      /*  re-enable the active tool  */
      tool_manager_control_active (gdisp->gimage->gimp, RESUME, gdisp);

      /*  scale the image into the exposed regions  */
      if (x_offset)
	{
	  src_x = (x_offset < 0) ? 0 : gdisp->disp_width - x_offset;
	  src_y = 0;
	  gdisplay_expose_area (gdisp,
				src_x, src_y,
				abs (x_offset), gdisp->disp_height);
	}
      if (y_offset)
	{
	  src_x = 0;
	  src_y = (y_offset < 0) ? 0 : gdisp->disp_height - y_offset;
	  gdisplay_expose_area (gdisp,
				src_x, src_y,
				gdisp->disp_width, abs (y_offset));
	}

      if (x_offset || y_offset)
	gdisplays_flush ();

      nav_dialog_update_window_marker (gdisp->window_nav_dialog); 

      if (gdisp->nav_popup)
	nav_dialog_update_window_marker (gdisp->nav_popup);

      /* Make sure graphics expose events are processed before scrolling
       * again
       */
      while ((event = gdk_event_get_graphics_expose (gdisp->canvas->window)))
	{
	  gtk_widget_event (gdisp->canvas, event);

	  if (event->expose.count == 0)
	    {
	      gdk_event_free (event);
	      break;
	    }

	  gdk_event_free (event);
	}

      return TRUE;
    }

  return FALSE;
}
