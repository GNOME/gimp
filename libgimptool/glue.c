/* libgimptool/glue.c
 * Copyright (C) 2002 Tor Lillqvist <tml@iki.fi>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 * 
 * This library is distributed in the hope that it will be useful, 
 * but WITHOUT ANY WARRANTY; without even the implied warranty of 
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU  
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

/*
 * Used in the libgimptool shared library for dynamic linking at
 * run-time against the main executable libgimptool is used by (either
 * gimp itself or tool-safe-mode, as far as I understand).
 *
 * Idea based upon libgimpwidgets/libgimp-glue.c.
 *
 * gimp_object_get_type
 * gimp_tool_control_get_cursor
 * gimp_tool_control_get_cursor_modifier
 * gimp_tool_control_get_toggle_cursor
 * gimp_tool_control_get_toggle_cursor_modifier
 * gimp_tool_control_get_toggle_tool_cursor
 * gimp_tool_control_get_tool_cursor
 * gimp_tool_control_get_type
 * gimp_tool_control_halt
 * gimp_tool_control_is_paused
 * gimp_tool_control_is_toggled
 * gimp_tool_control_pause
 * gimp_tool_control_resume
 * gimp_tool_real_button_press
 * gimp_tool_real_button_release
 * gimp_tool_set_cursor
 */

#include "config.h"

#include <glib.h>

#ifdef G_OS_WIN32 /* Bypass whole file otherwise */

#include <gtk/gtk.h>

#include "libgimpwidgets/gimpwidgets.h"
#include "libgimpproxy/gimpproxytypes.h"
#include "gimptooltypes.h"

#include <windows.h>

typedef int voidish;

static FARPROC 
dynamic_resolve (const gchar* name)
{
  FARPROC fn = NULL;

  fn = GetProcAddress (GetModuleHandle (NULL), name); 

  if (!fn)
    g_error ("GetProcAddress of %s failed!", name);

  return fn;
}

#define ENTRY(type, name, parlist, arglist)			\
type 								\
name parlist							\
{								\
  typedef type (*p_##name) parlist;				\
  static gboolean beenhere = FALSE;				\
  static p_##name fn;						\
  if (!beenhere)						\
    beenhere = TRUE, fn = (p_##name) dynamic_resolve (#name);	\
  return (fn) arglist;					 	\
}

ENTRY (GType, gimp_object_get_type, (void), ());
ENTRY (GdkCursorType, gimp_tool_control_get_cursor, (GimpToolControl *control), (control));
ENTRY (GimpCursorModifier, gimp_tool_control_get_cursor_modifier, (GimpToolControl *control), (control));
ENTRY (GdkCursorType, gimp_tool_control_get_toggle_cursor, (GimpToolControl *control), (control));
ENTRY (GimpCursorModifier, gimp_tool_control_get_toggle_cursor_modifier, (GimpToolControl *control), (control));
ENTRY (GimpToolCursorType, gimp_tool_control_get_toggle_tool_cursor, (GimpToolControl *control), (control));
ENTRY (GimpToolCursorType, gimp_tool_control_get_tool_cursor, (GimpToolControl *control), (control));
ENTRY (GType, gimp_tool_control_get_type, (void), ());
ENTRY (voidish, gimp_tool_control_halt, (GimpToolControl *control), (control));
ENTRY (gboolean, gimp_tool_control_is_paused, (GimpToolControl *control), (control));
ENTRY (gboolean, gimp_tool_control_is_toggled, (GimpToolControl *control), (control));
ENTRY (voidish, gimp_tool_control_pause, (GimpToolControl *control), (control));
ENTRY (voidish, gimp_tool_control_resume, (GimpToolControl *control), (control));
ENTRY (voidish, gimp_tool_real_button_press,	\
       (GimpTool        *tool,			\
	GimpCoords      *coords,		\
	guint32          time,			\
	GdkModifierType  state,			\
	GimpDisplay     *gdisp),		\
       (tool, coords, time, state, gdisp));
ENTRY (voidish, gimp_tool_real_button_release,	\
       (GimpTool        *tool,			\
	GimpCoords      *coords,		\
	guint32          time,			\
	GdkModifierType  state,			\
	GimpDisplay     *gdisp),		\
       (tool, coords, time, state, gdisp));
ENTRY (voidish, gimp_tool_set_cursor,			\
       (GimpTool           *tool,			\
	GimpDisplay        *gdisp,			\
	GdkCursorType       cursor,			\
	GimpToolCursorType  tool_cursor,		\
	GimpCursorModifier  modifier),			\
       (tool, gdisp, cursor, tool_cursor, modifier));

#endif /* G_OS_WIN32 */
