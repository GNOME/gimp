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

#include <gtk/gtk.h>
#include "config.h"

#include "gimptoolcontrol.h"

static void gimp_tool_control_class_init (GimpToolControlClass *klass);
static void gimp_tool_control_init (GimpToolControl *tool);

static GimpObjectClass *parent_class = NULL;

GType
gimp_tool_control_get_type (void)
{
  static GType tool_control_type = 0;

  if (! tool_control_type)
    {
      static const GTypeInfo tool_control_info =
      {
        sizeof (GimpToolControlClass),
	(GBaseInitFunc) NULL,
	(GBaseFinalizeFunc) NULL,
	(GClassInitFunc) gimp_tool_control_class_init,
	NULL,           /* class_finalize */
	NULL,           /* class_data     */
	sizeof (GimpToolControl),
	0,              /* n_preallocs    */
	(GInstanceInitFunc) gimp_tool_control_init,
      };

      tool_control_type = g_type_register_static (GIMP_TYPE_OBJECT,
		 			          "GimpToolControl", 
                                                 &tool_control_info, 0);
    }

  return tool_control_type;
}

static void
gimp_tool_control_class_init (GimpToolControlClass *klass)
{
  parent_class = g_type_class_peek_parent (klass);
}

static void
gimp_tool_control_init (GimpToolControl *control)
{
  control->state                  = INACTIVE;
  control->paused_count           = 0;

  control->toggled                = FALSE;
}


GimpToolControl *
gimp_tool_control_new  (gboolean           scroll_lock,
	                gboolean           auto_snap_to,
	                gboolean           preserve,
	                gboolean           handle_empty_image,
	                gboolean           perfectmouse,
	                /* are all these necessary? */
			GdkCursorType      cursor,
			GimpToolCursorType tool_cursor,
			GimpCursorModifier cursor_modifier,
		        GdkCursorType      toggle_cursor,
			GimpToolCursorType toggle_tool_cursor,
			GimpCursorModifier toggle_cursor_modifier)
{
  GimpToolControl *control;

  control = GIMP_TOOL_CONTROL (g_object_new (GIMP_TYPE_TOOL_CONTROL, NULL));


  control->scroll_lock            = scroll_lock;
  control->auto_snap_to           = auto_snap_to;
  control->preserve               = preserve;
  control->handle_empty_image     = handle_empty_image;
  control->perfectmouse           = perfectmouse;
  
  control->cursor                 = cursor;
  control->tool_cursor            = tool_cursor;
  control->cursor_modifier        = cursor_modifier;
  control->toggle_cursor          = toggle_cursor;
  control->toggle_tool_cursor     = toggle_tool_cursor;
  control->toggle_cursor_modifier = toggle_cursor_modifier;
  

  return control;
}
			
	       
void
gimp_tool_control_pause            (GimpToolControl   *control)
{
  g_return_if_fail(GIMP_IS_TOOL_CONTROL(control));

  /* control->state = PAUSED ? */
  control->paused_count++;
}

void
gimp_tool_control_resume           (GimpToolControl   *control)
{
  g_return_if_fail(GIMP_IS_TOOL_CONTROL(control));

  control->paused_count--;
}

gboolean
gimp_tool_control_is_paused    (GimpToolControl   *control)
{
  g_return_val_if_fail(GIMP_IS_TOOL_CONTROL(control), FALSE);

  return control->paused_count > 0;
}

void 
gimp_tool_control_activate         (GimpToolControl   *control)
{
  g_return_if_fail(GIMP_IS_TOOL_CONTROL(control));

  control->state = ACTIVE; 
}

void 
gimp_tool_control_halt             (GimpToolControl   *control)
{
  g_return_if_fail(GIMP_IS_TOOL_CONTROL(control));

  control->state = INACTIVE; 
  control->paused_count = 0; 
  
}

gboolean
gimp_tool_control_is_active    (GimpToolControl   *control)
{
  g_return_val_if_fail(GIMP_IS_TOOL_CONTROL(control), FALSE);

  return control->state == ACTIVE;
}

void
gimp_tool_control_set_toggle       (GimpToolControl   *control,
                                    gboolean           toggled) 
{
  g_return_if_fail(GIMP_IS_TOOL_CONTROL(control));

  control->toggled = toggled; 
}



void
gimp_tool_control_set_preserve     (GimpToolControl   *control,
                                    gboolean           preserve) 
{
  g_return_if_fail(GIMP_IS_TOOL_CONTROL(control));

  control->preserve = preserve; 
}

void
gimp_tool_control_set_cursor       (GimpToolControl   *control,
                                    GdkCursorType      cursor) 
{
  g_return_if_fail(GIMP_IS_TOOL_CONTROL(control));

  control->cursor = cursor; 
}

void
gimp_tool_control_set_tool_cursor  (GimpToolControl      *control,
                                    GimpToolCursorType    cursor) 
{
  g_return_if_fail(GIMP_IS_TOOL_CONTROL(control));

  control->tool_cursor = cursor; 
}
void
gimp_tool_control_set_cursor_modifier (GimpToolControl     *control,
                                       GimpCursorModifier   cmodifier) 
{
  g_return_if_fail(GIMP_IS_TOOL_CONTROL(control));

  control->cursor_modifier = cmodifier; 
}




void
gimp_tool_control_set_toggle_cursor       (GimpToolControl   *control,
                                           GdkCursorType      cursor) 
{
  g_return_if_fail(GIMP_IS_TOOL_CONTROL(control));

  control->toggle_cursor = cursor; 
}

void
gimp_tool_control_set_toggle_tool_cursor  (GimpToolControl      *control,
                                           GimpToolCursorType    cursor) 
{
  g_return_if_fail(GIMP_IS_TOOL_CONTROL(control));

  control->toggle_tool_cursor = cursor; 
}

void
gimp_tool_control_set_toggle_cursor_modifier (GimpToolControl     *control,
                                              GimpCursorModifier   cmodifier) 
{
  g_return_if_fail(GIMP_IS_TOOL_CONTROL(control));

  control->toggle_cursor_modifier = cmodifier; 
}


void
gimp_tool_control_set_scroll_lock            (GimpToolControl     *control,
                                              gboolean             scroll_lock)
{
  g_return_if_fail(GIMP_IS_TOOL_CONTROL(control));

  control->scroll_lock = scroll_lock; 
}

