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

#include <gtk/gtk.h>

#include "display-types.h"

#include "core/gimpimage.h"

#include "gimpdisplay.h"
#include "gimpdisplayshell.h"
#include "gimpdisplayshell-autoscroll.h"
#include "gimpdisplayshell-coords.h"
#include "gimpdisplayshell-scroll.h"
#include "gimpdisplayshell-transform.h"

#include "tools/tools-types.h"
#include "tools/tool_manager.h"


#define AUTOSCROLL_DT  20
#define AUTOSCROLL_DX  0.1


typedef struct
{
  GdkEventMotion  *mevent;
  GdkDevice       *device;
  guint32          time;
  GdkModifierType  state;
  guint            timeout_id;
} ScrollInfo;


/*  local function prototypes  */

static gboolean gimp_display_shell_autoscroll_timeout (gpointer data);


/*  public functions  */

void
gimp_display_shell_autoscroll_start (GimpDisplayShell *shell,
                                     GdkModifierType   state,
                                     GdkEventMotion   *mevent)
{
  ScrollInfo *info;

  g_return_if_fail (GIMP_IS_DISPLAY_SHELL (shell));

  if (shell->scroll_info)
    return;

  info = g_new0 (ScrollInfo, 1);

  info->mevent     = mevent;
  info->device     = mevent->device;
  info->time       = gdk_event_get_time ((GdkEvent *) mevent);
  info->state      = state;
  info->timeout_id = g_timeout_add (AUTOSCROLL_DT,
                                    gimp_display_shell_autoscroll_timeout,
                                    shell);

  shell->scroll_info = info;
}

void
gimp_display_shell_autoscroll_stop (GimpDisplayShell *shell)
{
  ScrollInfo *info;

  g_return_if_fail (GIMP_IS_DISPLAY_SHELL (shell));

  if (! shell->scroll_info)
    return;

  info = shell->scroll_info;

  if (info->timeout_id)
    {
      g_source_remove (info->timeout_id);
      info->timeout_id = 0;
    }

  g_free (info);
  shell->scroll_info = NULL;
}


/*  private functions  */

static gboolean
gimp_display_shell_autoscroll_timeout (gpointer data)
{
  GimpDisplayShell *shell = GIMP_DISPLAY_SHELL (data);
  GimpDisplay      *gdisp = shell->gdisp;
  ScrollInfo       *info  = shell->scroll_info;
  GimpCoords        device_coords;
  GimpCoords        image_coords;
  gint              off_x = 0;
  gint              off_y = 0;

  info->time += AUTOSCROLL_DT;

  gimp_display_shell_get_device_coords (shell,
                                        info->device, &device_coords);

  if (device_coords.x < 0)
    off_x = device_coords.x;
  else if (device_coords.x > shell->disp_width)
    off_x = device_coords.x - shell->disp_width;

  if (device_coords.y < 0)
    off_y = device_coords.y;
  else if (device_coords.y > shell->disp_height)
    off_y = device_coords.y - shell->disp_height;

  if (off_x == 0 && off_y == 0)
    {
      g_free (info);
      shell->scroll_info = NULL;

      return FALSE;
    }

  gimp_display_shell_scroll (shell,
                             AUTOSCROLL_DX * (gdouble) off_x,
                             AUTOSCROLL_DX * (gdouble) off_y);

  gimp_display_shell_untransform_coords (shell,
                                         &device_coords, &image_coords);

  tool_manager_motion_active (gdisp->gimage->gimp,
                              &image_coords,
                              info->time, info->state,
                              gdisp);

  return TRUE;
}
