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

#ifndef __TOOL_MANAGER_H__
#define __TOOL_MANAGER_H__

#include "gimpdrawable.h"
#include "tool.h"


/*  Function declarations  */
void        tool_manager_select_tool           (GimpTool       *tool);

void        tool_manager_initialize_tool       (GimpTool       *tool,
						GDisplay       *gdisp);

void        tool_manager_control_active        (ToolAction      action,
						GDisplay       *gdisp);

void        tool_manager_register              (GimpToolClass  *tool_type
						/*, ToolOptions *tool_options*/);


void        tool_manager_init                  (void);

/*  these should to go their own file  */
void        tool_manager_register_tool         (GtkType         tool_type,
						const gchar    *tool_name,
						const gchar    *menu_path,
						const gchar    *menu_accel,
						const gchar    *tool_desc,
						const gchar    *help_domain,
						const gchar    *help_data,
						const gchar   **icon_data);

GimpToolInfo * tool_manager_get_info_by_type   (GtkType         tool_type);


gchar     * tool_manager_get_active_PDB_string (void);
gchar     * tool_manager_active_get_help_data  (void);


/*  Global Data Structures  */
extern GimpTool      *active_tool;
extern GSList  	     *registered_tools;

extern GimpContainer *global_tool_info_list;


#endif  /*  __TOOL_MANAGER_H__  */
