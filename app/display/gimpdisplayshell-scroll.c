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

#include "tools/tools-types.h"

#include "tools/tool_manager.h"

#include "gimpdisplay.h"
#include "gimpdisplay-foreach.h"
#include "gimpdisplayshell.h"
#include "gimpdisplayshell-scale.h"
#include "gimpdisplayshell-scroll.h"

#include "gimprc.h"


gboolean
gimp_display_shell_scroll (GimpDisplayShell *shell,
                           gint              x_offset,
                           gint              y_offset)
{
  gint      old_x, old_y;
  gint      src_x, src_y;
  gint      dest_x, dest_y;
  GdkEvent *event;

  g_return_val_if_fail (GIMP_IS_DISPLAY_SHELL (shell), FALSE);

  old_x = shell->offset_x;
  old_y = shell->offset_y;

  shell->offset_x += x_offset;
  shell->offset_y += y_offset;

  gimp_display_shell_scroll_clamp_offsets (shell);

  /*  the actual changes in offset  */
  x_offset = (shell->offset_x - old_x);
  y_offset = (shell->offset_y - old_y);

  if (x_offset || y_offset)
    {
      /* FIXME: I'm sure this is useless if all other places are correct --mitch
       */
      gimp_display_shell_scale_setup (shell);

      src_x  = (x_offset < 0) ? 0 : x_offset;
      src_y  = (y_offset < 0) ? 0 : y_offset;
      dest_x = (x_offset < 0) ? -x_offset : 0;
      dest_y = (y_offset < 0) ? -y_offset : 0;

      /*  reset the old values so that the tool can accurately redraw  */
      shell->offset_x = old_x;
      shell->offset_y = old_y;

      /*  freeze the active tool  */
      tool_manager_control_active (shell->gdisp->gimage->gimp, PAUSE,
                                   shell->gdisp);

      /*  set the offsets back to the new values  */
      shell->offset_x += x_offset;
      shell->offset_y += y_offset;

      gdk_draw_drawable (shell->canvas->window,
			 shell->render_gc,
			 shell->canvas->window,
			 src_x, src_y,
			 dest_x, dest_y,
			 (shell->disp_width  - abs (x_offset)),
			 (shell->disp_height - abs (y_offset)));

      /*  re-enable the active tool  */
      tool_manager_control_active (shell->gdisp->gimage->gimp, RESUME,
                                   shell->gdisp);

      /*  scale the image into the exposed regions  */
      if (x_offset)
	{
	  src_x = (x_offset < 0) ? 0 : shell->disp_width - x_offset;
	  src_y = 0;

	  gimp_display_shell_add_expose_area (shell,
                                              src_x, src_y,
                                              abs (x_offset),
                                              shell->disp_height);
	}

      if (y_offset)
	{
	  src_x = 0;
	  src_y = (y_offset < 0) ? 0 : shell->disp_height - y_offset;

	  gimp_display_shell_add_expose_area (shell,
                                              src_x, src_y,
                                              shell->disp_width,
                                              abs (y_offset));
	}

      if (x_offset || y_offset)
        {
          gimp_display_flush (shell->gdisp);
        }

      gimp_display_shell_scrolled (shell);

      /* Make sure graphics expose events are processed before scrolling
       * again
       */
      while ((event = gdk_event_get_graphics_expose (shell->canvas->window)))
	{
          gdk_window_invalidate_rect (shell->canvas->window,
                                      &event->expose.area,
                                      FALSE);

          if (event->expose.count == 0)
            {
              gdk_event_free (event);
              break;
            }

	  gdk_event_free (event);
	}

      gdk_window_process_updates (shell->canvas->window, FALSE);

      return TRUE;
    }

  return FALSE;
}

void
gimp_display_shell_scroll_clamp_offsets (GimpDisplayShell *shell)
{
  gint sx, sy;

  g_return_if_fail (GIMP_IS_DISPLAY_SHELL (shell));

  sx = SCALEX (shell, shell->gdisp->gimage->width);
  sy = SCALEY (shell, shell->gdisp->gimage->height);

  shell->offset_x = CLAMP (shell->offset_x, 0,
			   MAX (sx - shell->disp_width, 0));

  shell->offset_y = CLAMP (shell->offset_y, 0,
			   MAX (sy - shell->disp_height, 0));
}
