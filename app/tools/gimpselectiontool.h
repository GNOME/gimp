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

#ifndef __GIMP_SELECTION_TOOL_H__
#define __GIMP_SELECTION_TOOL_H__


#include "gimpdrawtool.h"


#define GIMP_TYPE_SELECTION_TOOL            (gimp_selection_tool_get_type ())
#define GIMP_SELECTION_TOOL(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_SELECTION_TOOL, GimpSelectionTool))
#define GIMP_SELECTION_TOOL_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_SELECTION_TOOL, GimpSelectionToolClass))
#define GIMP_IS_SELECTION_TOOL(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_SELECTION_TOOL))
#define GIMP_IS_SELECTION_TOOL_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_SELECTION_TOOL))
#define GIMP_SELECTION_TOOL_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_SELECTION_TOOL, GimpSelectionToolClass))

#define GIMP_SELECTION_TOOL_GET_OPTIONS(t)  (GIMP_SELECTION_OPTIONS (gimp_tool_get_options (GIMP_TOOL (t))))


typedef struct _GimpSelectionTool      GimpSelectionTool;
typedef struct _GimpSelectionToolClass GimpSelectionToolClass;

struct _GimpSelectionTool
{
  GimpDrawTool  parent_instance;

  SelectOps     op;       /*  selection operation (SELECTION_ADD etc.)  */
  SelectOps     saved_op; /*  saved tool options state                  */

  gboolean      allow_move;
};

struct _GimpSelectionToolClass
{
  GimpDrawToolClass  parent_class;
};


GType      gimp_selection_tool_get_type   (void) G_GNUC_CONST;

/*  protected function  */
gboolean   gimp_selection_tool_start_edit (GimpSelectionTool *sel_tool,
                                           GimpCoords        *coords);


#endif  /* __GIMP_SELECTION_TOOL_H__ */
