/* The GIMP -- an image manipulation program
 * Copyright (C) 1995-2002 Spencer Kimball, Peter Mattis and others
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

#include "libgimpproxy/gimpproxytypes.h"
#include "libgimptool/gimptooltypes.h"
#include "libgimptool/gimptool.h"

void
gimp_tool_push_status (GimpTool    *tool,
                       const gchar *message)
{
#if 0
  GimpStatusbar *statusbar;

  g_return_if_fail (GIMP_IS_TOOL (tool));
  g_return_if_fail (GIMP_IS_DISPLAY (tool->gdisp));
  g_return_if_fail (message != NULL);

  statusbar =
    GIMP_STATUSBAR (GIMP_DISPLAY_SHELL (tool->gdisp->shell)->statusbar);

  gimp_statusbar_push (statusbar,
                       G_OBJECT_TYPE_NAME (tool),
                       message);
#endif
}

void
gimp_tool_push_status_coords (GimpTool    *tool,
                              const gchar *title,
                              gdouble      x,
                              const gchar *separator,
                              gdouble      y)
{
#if 0
  GimpStatusbar *statusbar;

  g_return_if_fail (GIMP_IS_TOOL (tool));
  g_return_if_fail (GIMP_IS_DISPLAY (tool->gdisp));
  g_return_if_fail (title != NULL);
  g_return_if_fail (separator != NULL);

  statusbar =
    GIMP_STATUSBAR (GIMP_DISPLAY_SHELL (tool->gdisp->shell)->statusbar);

  gimp_statusbar_push_coords (statusbar,
                              G_OBJECT_TYPE_NAME (tool),
                              title, x, separator, y);
#endif                             
}

void
gimp_tool_pop_status (GimpTool *tool)
{
#if 0
  GimpStatusbar *statusbar;

  g_return_if_fail (GIMP_IS_TOOL (tool));
  g_return_if_fail (GIMP_IS_DISPLAY (tool->gdisp));

  statusbar =
    GIMP_STATUSBAR (GIMP_DISPLAY_SHELL (tool->gdisp->shell)->statusbar);

  gimp_statusbar_pop (statusbar,
                      G_OBJECT_TYPE_NAME (tool));

#endif
}

void
gimp_tool_set_cursor (GimpTool           *tool,
                      GimpDisplay        *gdisp,
                      GdkCursorType       cursor,
                      GimpToolCursorType  tool_cursor,
                      GimpCursorModifier  modifier)
{
#if 0
  g_return_if_fail (GIMP_IS_TOOL (tool));
  g_return_if_fail (GIMP_IS_DISPLAY (gdisp));

  gimp_display_shell_set_cursor (GIMP_DISPLAY_SHELL (gdisp->shell),
                                 cursor,
                                 tool_cursor,
                                 modifier);
#endif                                
}

void
gimp_tool_real_button_press (GimpTool        *tool,
                             GimpCoords      *coords,
                             guint32          time,
			     GdkModifierType  state,
			     GimpDisplay     *gdisp)
{
#if 0
  tool->gdisp    = gdisp;
  tool->drawable = gimp_image_active_drawable (gdisp->gimage);

  tool->state = ACTIVE;
#endif
}

void
gimp_tool_real_button_release (GimpTool        *tool,
                               GimpCoords      *coords,
                               guint32          time,
			       GdkModifierType  state,
			       GimpDisplay     *gdisp)
{
#if 0
  tool->state = INACTIVE;
#endif
}

