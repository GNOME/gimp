/* The GIMP -- an image manipulation program
 * Copyright (C) 1995-2002 Spencer Kimball, Peter Mattis and others
 *
 * gimptoolcontrol-displayshell.c: toolcontrol functions exclusively for
 * the DisplayShell
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

#include <gtk/gtk.h>
#include "config.h"

#include "gimptoolcontrol.h"
#include "gimptoolcontrol-displayshell.h"

gboolean
gimp_tool_control_wants_perfect_mouse    (GimpToolControl   *control)
{
  g_return_val_if_fail(GIMP_IS_TOOL_CONTROL(control), FALSE);

  return control->perfectmouse;
}

gboolean
gimp_tool_control_handles_empty_image    (GimpToolControl   *control)
{
  g_return_val_if_fail(GIMP_IS_TOOL_CONTROL(control), FALSE);

  return control->handle_empty_image;
}

gboolean
gimp_tool_control_auto_snap_to           (GimpToolControl   *control)
{
  g_return_val_if_fail(GIMP_IS_TOOL_CONTROL(control), FALSE);

  return control->auto_snap_to;
}

gboolean
gimp_tool_control_preserve               (GimpToolControl   *control)
{
  g_return_val_if_fail(GIMP_IS_TOOL_CONTROL(control), FALSE);

  return control->preserve;
}

gboolean
gimp_tool_control_scroll_lock            (GimpToolControl   *control)
{
  g_return_val_if_fail(GIMP_IS_TOOL_CONTROL(control), FALSE);

  return control->scroll_lock;
}

GimpToolCursorType 
gimp_tool_control_get_tool_cursor  (GimpToolControl   *control)
{
  g_return_val_if_fail(GIMP_IS_TOOL_CONTROL(control), FALSE);

  return control->tool_cursor;
}

