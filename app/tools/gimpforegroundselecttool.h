/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef __GIMP_FOREGROUND_SELECT_TOOL_H__
#define __GIMP_FOREGROUND_SELECT_TOOL_H__


#include "gimpfreeselecttool.h"


typedef enum
{
  MATTING_STATE_FREE_SELECT = 0,
  MATTING_STATE_PAINT_TRIMAP,
  MATTING_STATE_PREVIEW_MASK,
} MattingState;


#define GIMP_TYPE_FOREGROUND_SELECT_TOOL            (gimp_foreground_select_tool_get_type ())
#define GIMP_FOREGROUND_SELECT_TOOL(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_FOREGROUND_SELECT_TOOL, GimpForegroundSelectTool))
#define GIMP_FOREGROUND_SELECT_TOOL_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_FOREGROUND_SELECT_TOOL, GimpForegroundSelectToolClass))
#define GIMP_IS_FOREGROUND_SELECT_TOOL(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_FOREGROUND_SELECT_TOOL))
#define GIMP_IS_FOREGROUND_SELECT_TOOL_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_FOREGROUND_SELECT_TOOL))
#define GIMP_FOREGROUND_SELECT_TOOL_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_FOREGROUND_SELECT_TOOL, GimpForegroundSelectToolClass))

#define GIMP_FOREGROUND_SELECT_TOOL_GET_OPTIONS(t)  (GIMP_FOREGROUND_SELECT_OPTIONS (gimp_tool_get_options (GIMP_TOOL (t))))


typedef struct _GimpForegroundSelectTool      GimpForegroundSelectTool;
typedef struct _GimpForegroundSelectToolClass GimpForegroundSelectToolClass;

struct _GimpForegroundSelectTool
{
  GimpFreeSelectTool  parent_instance;

  MattingState        state;

  GimpCoords          last_coords;
  GArray             *stroke;
  GeglBuffer         *trimap;
  GeglBuffer         *mask;

  GList              *undo_stack;
  GList              *redo_stack;

  GimpToolGui        *gui;
  GtkWidget          *preview_toggle;
};

struct _GimpForegroundSelectToolClass
{
  GimpFreeSelectToolClass  parent_class;
};


void    gimp_foreground_select_tool_register (GimpToolRegisterCallback  callback,
                                              gpointer                  data);

GType   gimp_foreground_select_tool_get_type (void) G_GNUC_CONST;


#endif  /*  __GIMP_FOREGROUND_SELECT_TOOL_H__  */
