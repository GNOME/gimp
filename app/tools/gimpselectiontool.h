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

#ifndef __LIGMA_SELECTION_TOOL_H__
#define __LIGMA_SELECTION_TOOL_H__


#include "ligmadrawtool.h"


#define LIGMA_TYPE_SELECTION_TOOL            (ligma_selection_tool_get_type ())
#define LIGMA_SELECTION_TOOL(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), LIGMA_TYPE_SELECTION_TOOL, LigmaSelectionTool))
#define LIGMA_SELECTION_TOOL_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), LIGMA_TYPE_SELECTION_TOOL, LigmaSelectionToolClass))
#define LIGMA_IS_SELECTION_TOOL(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LIGMA_TYPE_SELECTION_TOOL))
#define LIGMA_IS_SELECTION_TOOL_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), LIGMA_TYPE_SELECTION_TOOL))
#define LIGMA_SELECTION_TOOL_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), LIGMA_TYPE_SELECTION_TOOL, LigmaSelectionToolClass))

#define LIGMA_SELECTION_TOOL_GET_OPTIONS(t)  (LIGMA_SELECTION_OPTIONS (ligma_tool_get_options (LIGMA_TOOL (t))))


typedef struct _LigmaSelectionTool      LigmaSelectionTool;
typedef struct _LigmaSelectionToolClass LigmaSelectionToolClass;

struct _LigmaSelectionTool
{
  LigmaDrawTool    parent_instance;

  SelectFunction  function;         /*  selection function        */
  LigmaChannelOps  saved_operation;  /*  saved tool options state  */

  gint            change_count;
  gboolean        saved_show_selection;
  LigmaUndo       *undo;
  LigmaUndo       *redo;
  gint            idle_id;

  gboolean        allow_move;
};

struct _LigmaSelectionToolClass
{
  LigmaDrawToolClass  parent_class;

  /*  virtual functions  */
  gboolean (* have_selection) (LigmaSelectionTool *sel_tool,
                               LigmaDisplay       *display);
};


GType      ligma_selection_tool_get_type     (void) G_GNUC_CONST;

/*  protected function  */
gboolean   ligma_selection_tool_start_edit   (LigmaSelectionTool *sel_tool,
                                             LigmaDisplay       *display,
                                             const LigmaCoords  *coords);

void       ligma_selection_tool_start_change (LigmaSelectionTool *sel_tool,
                                             gboolean           create,
                                             LigmaChannelOps     operation);
void       ligma_selection_tool_end_change   (LigmaSelectionTool *sel_tool,
                                             gboolean           cancel);


#endif  /* __LIGMA_SELECTION_TOOL_H__ */
