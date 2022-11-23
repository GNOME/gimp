/* LIGMA - The GNU Image Manipulation Program
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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef __LIGMA_FOREGROUND_SELECT_TOOL_H__
#define __LIGMA_FOREGROUND_SELECT_TOOL_H__


#include "ligmapolygonselecttool.h"


typedef enum
{
  MATTING_STATE_FREE_SELECT = 0,
  MATTING_STATE_PAINT_TRIMAP,
  MATTING_STATE_PREVIEW_MASK,
} MattingState;


#define LIGMA_TYPE_FOREGROUND_SELECT_TOOL            (ligma_foreground_select_tool_get_type ())
#define LIGMA_FOREGROUND_SELECT_TOOL(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), LIGMA_TYPE_FOREGROUND_SELECT_TOOL, LigmaForegroundSelectTool))
#define LIGMA_FOREGROUND_SELECT_TOOL_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), LIGMA_TYPE_FOREGROUND_SELECT_TOOL, LigmaForegroundSelectToolClass))
#define LIGMA_IS_FOREGROUND_SELECT_TOOL(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LIGMA_TYPE_FOREGROUND_SELECT_TOOL))
#define LIGMA_IS_FOREGROUND_SELECT_TOOL_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), LIGMA_TYPE_FOREGROUND_SELECT_TOOL))
#define LIGMA_FOREGROUND_SELECT_TOOL_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), LIGMA_TYPE_FOREGROUND_SELECT_TOOL, LigmaForegroundSelectToolClass))

#define LIGMA_FOREGROUND_SELECT_TOOL_GET_OPTIONS(t)  (LIGMA_FOREGROUND_SELECT_OPTIONS (ligma_tool_get_options (LIGMA_TOOL (t))))


typedef struct _LigmaForegroundSelectTool      LigmaForegroundSelectTool;
typedef struct _LigmaForegroundSelectToolClass LigmaForegroundSelectToolClass;

struct _LigmaForegroundSelectTool
{
  LigmaPolygonSelectTool  parent_instance;

  MattingState           state;

  LigmaCoords             last_coords;
  GArray                *stroke;
  GeglBuffer            *trimap;
  GeglBuffer            *mask;

  GList                 *undo_stack;
  GList                 *redo_stack;

  LigmaToolGui           *gui;
  GtkWidget             *preview_toggle;

  LigmaCanvasItem        *grayscale_preview;
};

struct _LigmaForegroundSelectToolClass
{
  LigmaPolygonSelectToolClass  parent_class;
};


void    ligma_foreground_select_tool_register (LigmaToolRegisterCallback  callback,
                                              gpointer                  data);

GType   ligma_foreground_select_tool_get_type (void) G_GNUC_CONST;


#endif  /*  __LIGMA_FOREGROUND_SELECT_TOOL_H__  */
