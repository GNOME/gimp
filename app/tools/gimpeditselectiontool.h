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

#ifndef __EDIT_SELECTION_H__
#define __EDIT_SELECTION_H__


#include "tools.h"


typedef enum
{
  EDIT_MASK_TRANSLATE,
  EDIT_MASK_TO_LAYER_TRANSLATE,
  EDIT_LAYER_TRANSLATE,
  EDIT_FLOATING_SEL_TRANSLATE
} EditType;


void   init_edit_selection           (Tool           *tool,
				      gpointer        gdisp_ptr,
				      GdkEventButton *bevent,
				      EditType        edit_type);

void   edit_selection_button_release (Tool           *tool, 
				      GdkEventButton *bevent, 
				      gpointer        gdisp_ptr);
void   edit_selection_motion         (Tool           *tool, 
				      GdkEventMotion *mevent, 
				      gpointer        gdisp_ptr);
void   edit_selection_control        (Tool           *tool, 
				      ToolAction      action,    
				      gpointer        gdisp_ptr);
void   edit_selection_cursor_update  (Tool           *tool, 
				      GdkEventMotion *mevent, 
				      gpointer        gdisp_ptr);
void   edit_selection_draw           (Tool           *tool);
void   edit_sel_arrow_keys_func      (Tool           *tool,
				      GdkEventKey    *kevent,
				      gpointer        gdisp_ptr);


#endif  /*  __EDIT_SELECTION_H__  */
