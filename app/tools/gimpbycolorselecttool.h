/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ligmabycolorselectool.h
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

#ifndef __LIGMA_BY_COLOR_SELECT_TOOL_H__
#define __LIGMA_BY_COLOR_SELECT_TOOL_H__


#include "ligmaregionselecttool.h"


#define LIGMA_TYPE_BY_COLOR_SELECT_TOOL            (ligma_by_color_select_tool_get_type ())
#define LIGMA_BY_COLOR_SELECT_TOOL(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), LIGMA_TYPE_BY_COLOR_SELECT_TOOL, LigmaByColorSelectTool))
#define LIGMA_BY_COLOR_SELECT_TOOL_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), LIGMA_TYPE_BY_COLOR_SELECT_TOOL, LigmaByColorSelectToolClass))
#define LIGMA_IS_BY_COLOR_SELECT_TOOL(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LIGMA_TYPE_BY_COLOR_SELECT_TOOL))
#define LIGMA_IS_BY_COLOR_SELECT_TOOL_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), LIGMA_TYPE_BY_COLOR_SELECT_TOOL))
#define LIGMA_BY_COLOR_SELECT_TOOL_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), LIGMA_TYPE_BY_COLOR_SELECT_TOOL, LigmaByColorSelectToolClass))


typedef struct _LigmaByColorSelectTool      LigmaByColorSelectTool;
typedef struct _LigmaByColorSelectToolClass LigmaByColorSelectToolClass;

struct _LigmaByColorSelectTool
{
  LigmaRegionSelectTool  parent_instance;
};

struct _LigmaByColorSelectToolClass
{
  LigmaRegionSelectToolClass  parent_class;
};


void    ligma_by_color_select_tool_register (LigmaToolRegisterCallback  callback,
                                            gpointer                  data);

GType   ligma_by_color_select_tool_get_type (void) G_GNUC_CONST;


#endif  /*  __LIGMA_BY_COLOR_SELECT_TOOL_H__  */
