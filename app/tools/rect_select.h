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

#ifndef __RECT_SELECT_H__
#define __RECT_SELECT_H__


/*  rect select action functions  */
void   rect_select_button_press    (Tool           *tool,
				    GdkEventButton *bevent,
				    gpointer        gdisp_ptr);
void   rect_select_button_release  (Tool           *tool,
				    GdkEventButton *bevent,
				    gpointer        gdisp_ptr);
void   rect_select_motion          (Tool           *tool,
				    GdkEventMotion *mevent,
				    gpointer        gdisp_ptr);
void   rect_select_modifier_update (Tool           *tool,
				    GdkEventKey    *kevent,
				    gpointer        gdisp_ptr);
void   rect_select_cursor_update   (Tool           *tool,
				    GdkEventMotion *mevent,
				    gpointer        gdisp_ptr);
void   rect_select_oper_update     (Tool           *tool,
				    GdkEventMotion *mevent,
				    gpointer        gdisp_ptr);
void   rect_select_control         (Tool           *tool,
				    ToolAction      tool_action,
				    gpointer        gdisp_ptr);

/*  rect select functions  */
void   rect_select_draw            (Tool           *tool);
void   rect_select                 (GimpImage      *gimage,
				    gint            x,
				    gint            y,
				    gint            w,
				    gint            g,
				    SelectOps       op,
				    gboolean        feather,
				    gdouble         feather_radius);

Tool * tools_new_rect_select       (void);
void   tools_free_rect_select      (Tool           *tool);


#endif  /*  __RECT_SELECT_H__  */
