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

#ifndef __GIMP_MOVE_TOOL_H__
#define __GIMP_MOVE_TOOL_H__


#include "gimptool.h"


#define GIMP_TYPE_MOVE_TOOL            (gimp_move_tool_get_type ())
#define GIMP_MOVE_TOOL(obj)            (GTK_CHECK_CAST ((obj), GIMP_TYPE_MOVE_TOOL, GimpMoveTool))
#define GIMP_IS_MOVE_TOOL(obj)         (GTK_CHECK_TYPE ((obj), GIMP_TYPE_MOVE_TOOL))
#define GIMP_MOVE_TOOL_CLASS(klass)    (GTK_CHECK_CLASS_CAST ((klass), GIMP_TYPE_MOVE_TOOL, GimpMoveToolClass))
#define GIMP_IS_MOVE_TOOL_CLASS(klass) (GTK_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_MOVE_TOOL))


typedef struct _GimpMoveTool      GimpMoveTool;
typedef struct _GimpMoveToolClass GimpMoveToolClass;

struct _GimpMoveTool
{
  GimpTool   parent_instance;

  GimpLayer *layer;
  Guide     *guide;
  GDisplay  *disp;
};

struct _GimpMoveToolClass
{
  GimpToolClass parent_class;
};


GtkType    gimp_move_tool_get_type     (void);

void       gimp_move_tool_register     (void);

void       gimp_move_tool_start_hguide (GimpTool *tool,
					GDisplay *gdisp);
void       gimp_move_tool_start_vguide (GimpTool *tool,
					GDisplay *gdisp);


#endif  /*  __GIMP_MOVE_TOOL_H__  */
