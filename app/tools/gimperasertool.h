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

#ifndef __LIGMA_ERASER_TOOL_H__
#define __LIGMA_ERASER_TOOL_H__


#include "ligmabrushtool.h"


#define LIGMA_TYPE_ERASER_TOOL            (ligma_eraser_tool_get_type ())
#define LIGMA_ERASER_TOOL(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), LIGMA_TYPE_ERASER_TOOL, LigmaEraserTool))
#define LIGMA_ERASER_TOOL_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), LIGMA_TYPE_ERASER_TOOL, LigmaEraserToolClass))
#define LIGMA_IS_ERASER_TOOL(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LIGMA_TYPE_ERASER_TOOL))
#define LIGMA_IS_ERASER_TOOL_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), LIGMA_TYPE_ERASER_TOOL))
#define LIGMA_ERASER_TOOL_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), LIGMA_TYPE_ERASER_TOOL, LigmaEraserToolClass))

#define LIGMA_ERASER_TOOL_GET_OPTIONS(t)  (LIGMA_ERASER_OPTIONS (ligma_tool_get_options (LIGMA_TOOL (t))))


typedef struct _LigmaEraserTool      LigmaEraserTool;
typedef struct _LigmaEraserToolClass LigmaEraserToolClass;

struct _LigmaEraserTool
{
  LigmaBrushTool parent_instance;
};

struct _LigmaEraserToolClass
{
  LigmaBrushToolClass parent_class;
};


void    ligma_eraser_tool_register (LigmaToolRegisterCallback  callback,
                                   gpointer                  data);

GType   ligma_eraser_tool_get_type (void) G_GNUC_CONST;


#endif  /*  __LIGMA_ERASER_TOOL_H__  */
