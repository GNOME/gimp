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

#ifndef  __LIGMA_MYBRUSH_TOOL_H__
#define  __LIGMA_MYBRUSH_TOOL_H__


#include "ligmapainttool.h"


#define LIGMA_TYPE_MYBRUSH_TOOL            (ligma_mybrush_tool_get_type ())
#define LIGMA_MYBRUSH_TOOL(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), LIGMA_TYPE_MYBRUSH_TOOL, LigmaMybrushTool))
#define LIGMA_MYBRUSH_TOOL_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), LIGMA_TYPE_MYBRUSH_TOOL, LigmaMybrushToolClass))
#define LIGMA_IS_MYBRUSH_TOOL(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LIGMA_TYPE_MYBRUSH_TOOL))
#define LIGMA_IS_MYBRUSH_TOOL_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), LIGMA_TYPE_MYBRUSH_TOOL))
#define LIGMA_MYBRUSH_TOOL_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), LIGMA_TYPE_MYBRUSH_TOOL, LigmaMybrushToolClass))

#define LIGMA_MYBRUSH_TOOL_GET_OPTIONS(t)  (LIGMA_MYBRUSH_OPTIONS (ligma_tool_get_options (LIGMA_TOOL (t))))


typedef struct _LigmaMybrushTool      LigmaMybrushTool;
typedef struct _LigmaMybrushToolClass LigmaMybrushToolClass;

struct _LigmaMybrushTool
{
  LigmaPaintTool parent_instance;
};

struct _LigmaMybrushToolClass
{
  LigmaPaintToolClass parent_class;
};


void    ligma_mybrush_tool_register (LigmaToolRegisterCallback  callback,
                                    gpointer                  data);

GType   ligma_mybrush_tool_get_type (void) G_GNUC_CONST;

LigmaCanvasItem * ligma_mybrush_tool_create_cursor (LigmaPaintTool *paint_tool,
                                                  LigmaDisplay   *display,
                                                  gdouble        x,
                                                  gdouble        y,
                                                  gdouble        radius);

#endif  /*  __LIGMA_MYBRUSH_TOOL_H__  */
