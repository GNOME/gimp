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

#include "tools-types.h"

#include "gimpimagemaptool.h"


static GimpToolClass *parent_class = NULL;


/*  local function prototypes  */

static void   gimp_image_map_tool_class_init (GimpImageMapToolClass *klass);
static void   gimp_image_map_tool_init       (GimpImageMapTool      *image_map_tool);


/*  functions  */

GType
gimp_image_map_tool_get_type (void)
{
  static GType tool_type = 0;

  if (! tool_type)
    {
      static const GTypeInfo tool_info =
      {
        sizeof (GimpImageMapToolClass),
	(GBaseInitFunc) NULL,
	(GBaseFinalizeFunc) NULL,
	(GClassInitFunc) gimp_image_map_tool_class_init,
	NULL,           /* class_finalize */
	NULL,           /* class_data     */
	sizeof (GimpImageMapTool),
	0,              /* n_preallocs    */
	(GInstanceInitFunc) gimp_image_map_tool_init,
      };

      tool_type = g_type_register_static (GIMP_TYPE_TOOL,
					  "GimpImageMapTool", 
                                          &tool_info, 0);
    }

  return tool_type;
}

static void
gimp_image_map_tool_class_init (GimpImageMapToolClass *klass)
{
  parent_class = g_type_class_peek_parent (klass);
}

static void
gimp_image_map_tool_init (GimpImageMapTool *image_map_tool)
{
  GimpTool *tool;

  tool = GIMP_TOOL (image_map_tool);

  tool->control = gimp_tool_control_new  (TRUE,                       /* scroll_lock */
                                          TRUE,                       /* auto_snap_to */
                                          FALSE,                      /* preserve */
                                          FALSE,                      /* handle_empty_image */
                                          FALSE,                      /* perfectmouse */
                                          GIMP_MOUSE_CURSOR,          /* cursor */
                                          GIMP_TOOL_CURSOR_NONE,      /* tool_cursor */
                                          GIMP_CURSOR_MODIFIER_NONE,  /* cursor_modifier */
                                          GIMP_MOUSE_CURSOR,          /* toggle_cursor */
                                          GIMP_TOOL_CURSOR_NONE,      /* toggle_tool_cursor */
                                          GIMP_CURSOR_MODIFIER_NONE   /* toggle_cursor_modifier */);
}
