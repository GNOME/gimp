/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <gegl.h>
#include <gtk/gtk.h>

#include "display-types.h"

#include "widgets/gimpdeviceinfo.h"
#include "widgets/gimpdeviceinfo-coords.h"

#include "gimpdisplay.h"
#include "gimpdisplayshell.h"
#include "gimpdisplayshell-autoscroll.h"
#include "gimpdisplayshell-scroll.h"
#include "gimpdisplayshell-transform.h"

#include "tools/tools-types.h"
#include "tools/tool_manager.h"
#include "tools/gimptool.h"
#include "tools/gimptoolcontrol.h"


#define AUTOSCROLL_DT  20
#define AUTOSCROLL_DX  0.1


typedef struct
{
  GdkEventMotion  *mevent;
  GimpDeviceInfo  *device;
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

  info = g_slice_new0 (ScrollInfo);

  info->mevent     = mevent;
  info->device     = gimp_device_info_get_by_device (mevent->device);
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

  g_slice_free (ScrollInfo, info);
  shell->scroll_info = NULL;
}


/*  private functions  */

static gboolean
gimp_display_shell_autoscroll_timeout (gpointer data)
{
  GimpDisplayShell *shell = GIMP_DISPLAY_SHELL (data);
  ScrollInfo       *info  = shell->scroll_info;
  GimpCoords        device_coords;
  GimpCoords        image_coords;
  gint              dx = 0;
  gint              dy = 0;

  gimp_device_info_get_device_coords (info->device,
                                      gtk_widget_get_window (shell->canvas),
                                      &device_coords);

  if (device_coords.x < 0)
    dx = device_coords.x;
  else if (device_coords.x > shell->disp_width)
    dx = device_coords.x - shell->disp_width;

  if (device_coords.y < 0)
    dy = device_coords.y;
  else if (device_coords.y > shell->disp_height)
    dy = device_coords.y - shell->disp_height;

  if (dx || dy)
    {
      GimpDisplay *display         = shell->display;
      GimpTool    *active_tool     = tool_manager_get_active (display->gimp);
      gint         scroll_amount_x = AUTOSCROLL_DX * dx;
      gint         scroll_amount_y = AUTOSCROLL_DX * dy;

      info->time += AUTOSCROLL_DT;

      gimp_display_shell_scroll_unoverscrollify (shell,
                                                 scroll_amount_x,
                                                 scroll_amount_y,
                                                 &scroll_amount_x,
                                                 &scroll_amount_y);

      gimp_display_shell_scroll (shell,
                                 scroll_amount_x,
                                 scroll_amount_y);

      gimp_display_shell_untransform_coords (shell,
                                             &device_coords,
                                             &image_coords);

      if (gimp_tool_control_get_snap_to (active_tool->control))
        {
          gint x, y, width, height;

          gimp_tool_control_get_snap_offsets (active_tool->control,
                                              &x, &y, &width, &height);

          gimp_display_shell_snap_coords (shell,
                                          &image_coords,
                                          x, y, width, height);
        }

      tool_manager_motion_active (display->gimp,
                                  &image_coords,
                                  info->time, info->state,
                                  display);

      return TRUE;
    }
  else
    {
      g_slice_free (ScrollInfo, info);
      shell->scroll_info = NULL;

      return FALSE;
    }
}
