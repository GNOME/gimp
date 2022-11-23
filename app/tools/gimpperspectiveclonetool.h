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

#ifndef __LIGMA_PERSPECTIVE_CLONE_TOOL_H__
#define __LIGMA_PERSPECTIVE_CLONE_TOOL_H__


#include "ligmabrushtool.h"
#include "ligmatransformtool.h"  /* for TransInfo */


#define LIGMA_TYPE_PERSPECTIVE_CLONE_TOOL            (ligma_perspective_clone_tool_get_type ())
#define LIGMA_PERSPECTIVE_CLONE_TOOL(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), LIGMA_TYPE_PERSPECTIVE_CLONE_TOOL, LigmaPerspectiveCloneTool))
#define LIGMA_PERSPECTIVE_CLONE_TOOL_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), LIGMA_TYPE_PERSPECTIVE_CLONE_TOOL, LigmaPerspectiveCloneToolClass))
#define LIGMA_IS_PERSPECTIVE_CLONE_TOOL(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LIGMA_TYPE_PERSPECTIVE_CLONE_TOOL))
#define LIGMA_IS_PERSPECTIVE_CLONE_TOOL_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), LIGMA_TYPE_PERSPECTIVE_CLONE_TOOL))
#define LIGMA_PERSPECTIVE_CLONE_TOOL_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), LIGMA_TYPE_PERSPECTIVE_CLONE_TOOL, LigmaPerspectiveCloneToolClass))

#define LIGMA_PERSPECTIVE_CLONE_TOOL_GET_OPTIONS(t)  (LIGMA_PERSPECTIVE_CLONE_OPTIONS (ligma_tool_get_options (LIGMA_TOOL (t))))


typedef struct _LigmaPerspectiveCloneTool      LigmaPerspectiveCloneTool;
typedef struct _LigmaPerspectiveCloneToolClass LigmaPerspectiveCloneToolClass;

struct _LigmaPerspectiveCloneTool
{
  LigmaBrushTool        parent_instance;

  LigmaDisplay         *src_display;
  gint                 src_x;
  gint                 src_y;

  LigmaMatrix3          transform;      /*  transformation matrix            */
  TransInfo            trans_info;     /*  transformation info              */
  TransInfo            old_trans_info; /*  for cancelling a drag operation  */

  gint                 x1, y1;         /*  upper left hand coordinate       */
  gint                 x2, y2;         /*  lower right hand coords          */

  LigmaCursorPrecision  saved_precision;

  LigmaToolWidget      *widget;
  LigmaToolWidget      *grab_widget;
};

struct _LigmaPerspectiveCloneToolClass
{
  LigmaBrushToolClass  parent_class;
};


void    ligma_perspective_clone_tool_register (LigmaToolRegisterCallback  callback,
                                              gpointer                  data);

GType   ligma_perspective_clone_tool_get_type (void) G_GNUC_CONST;


#endif  /*  __LIGMA_PERSPECTIVE_CLONE_TOOL_H__  */
