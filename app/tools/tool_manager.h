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


void           tool_manager_init                  (Gimp             *gimp);
void           tool_manager_exit                  (Gimp             *gimp);

void        tool_manager_set_global_paint_options (Gimp             *gimp,
						   gboolean          global);

GimpTool     * tool_manager_get_active            (Gimp             *gimp); 

void           tool_manager_select_tool           (Gimp             *gimp,
						   GimpTool         *tool);

void           tool_manager_push_tool             (Gimp             *gimp,
						   GimpTool         *tool);
void           tool_manager_pop_tool              (Gimp             *gimp);


void           tool_manager_initialize_tool       (Gimp             *gimp,
						   GimpTool         *tool,
						   GimpDisplay      *gdisp);

void           tool_manager_control_active        (Gimp             *gimp,
						   ToolAction        action,
						   GimpDisplay      *gdisp);

const gchar  * tool_manager_active_get_help_data  (Gimp             *gimp);


void           tool_manager_register_tool         (Gimp             *gimp,
						   GType             tool_type,
						   gboolean          tool_context,
						   const gchar      *identifier,
						   const gchar      *blurb,
						   const gchar      *help,
						   const gchar      *menu_path,
						   const gchar      *menu_accel,
						   const gchar      *help_domain,
						   const gchar      *help_data,
						   const gchar      *stock_id);

void           tool_manager_register_tool_options (GType             tool_type,
						   GimpToolOptions  *tool_options);

GimpToolInfo * tool_manager_get_info_by_type      (Gimp             *gimp,
						   GType             tool_type);


void	       tool_manager_help_func             (const gchar      *help_data);


#endif  /*  __TOOL_MANAGER_H__  */
