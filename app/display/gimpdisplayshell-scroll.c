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


gboolean
gimp_display_shell_scroll (GimpDisplayShell *shell,
                           gint              x_offset,
                           gint              y_offset)
{
  gint      old_x, old_y;
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

      /*  reset the old values so that the tool can accurately redraw  */
      shell->offset_x = old_x;
      shell->offset_y = old_y;

      tool_manager_control_active (shell->gdisp->gimage->gimp, PAUSE,
                                   shell->gdisp);

      gdk_window_scroll (shell->canvas->window, -x_offset, -y_offset);

      /*  set the offsets back to the new values  */
      shell->offset_x += x_offset;
      shell->offset_y += y_offset;

      tool_manager_control_active (shell->gdisp->gimage->gimp, RESUME,
                                   shell->gdisp);

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

      gimp_display_shell_scrolled (shell);

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
