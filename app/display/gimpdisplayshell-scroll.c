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

#include "tools/gimptool.h"
#include "tools/tool_manager.h"

#include "gimpdisplay.h"
#include "gimpdisplay-foreach.h"
#include "gimpdisplay-scale.h"
#include "gimpdisplay-scroll.h"

#include "gimprc.h"
#include "nav_window.h"


gboolean
gimp_display_scroll (GimpDisplay *gdisp,
                     gint         x_offset,
                     gint         y_offset)
{
  gint      old_x, old_y;
  gint      src_x, src_y;
  gint      dest_x, dest_y;
  GdkEvent *event;

  g_return_val_if_fail (GIMP_IS_DISPLAY (gdisp), FALSE);

  old_x = gdisp->offset_x;
  old_y = gdisp->offset_y;

  gdisp->offset_x += x_offset;
  gdisp->offset_y += y_offset;

  gimp_display_scroll_clamp_offsets (gdisp);

  /*  the actual changes in offset  */
  x_offset = (gdisp->offset_x - old_x);
  y_offset = (gdisp->offset_y - old_y);

  if (x_offset || y_offset)
    {
      /* FIXME: I'm sure this is useless if all other places are correct --mitch
       */
      gimp_display_scale_setup (gdisp);

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

void
gimp_display_scroll_clamp_offsets (GimpDisplay *gdisp)
{
  gint sx, sy;

  g_return_if_fail (GIMP_IS_DISPLAY (gdisp));

  sx = SCALEX (gdisp, gdisp->gimage->width);
  sy = SCALEY (gdisp, gdisp->gimage->height);

  gdisp->offset_x = CLAMP (gdisp->offset_x, 0,
			   LOWPASS (sx - gdisp->disp_width));

  gdisp->offset_y = CLAMP (gdisp->offset_y, 0,
			   LOWPASS (sy - gdisp->disp_height));
}

