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

#ifndef __LIGMA_FREE_SELECT_TOOL_H__
#define __LIGMA_FREE_SELECT_TOOL_H__


#include "ligmapolygonselecttool.h"


#define LIGMA_TYPE_FREE_SELECT_TOOL            (ligma_free_select_tool_get_type ())
#define LIGMA_FREE_SELECT_TOOL(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), LIGMA_TYPE_FREE_SELECT_TOOL, LigmaFreeSelectTool))
#define LIGMA_FREE_SELECT_TOOL_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), LIGMA_TYPE_FREE_SELECT_TOOL, LigmaFreeSelectToolClass))
#define LIGMA_IS_FREE_SELECT_TOOL(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LIGMA_TYPE_FREE_SELECT_TOOL))
#define LIGMA_IS_FREE_SELECT_TOOL_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), LIGMA_TYPE_FREE_SELECT_TOOL))
#define LIGMA_FREE_SELECT_TOOL_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), LIGMA_TYPE_FREE_SELECT_TOOL, LigmaFreeSelectToolClass))


typedef struct _LigmaFreeSelectTool        LigmaFreeSelectTool;
typedef struct _LigmaFreeSelectToolPrivate LigmaFreeSelectToolPrivate;
typedef struct _LigmaFreeSelectToolClass   LigmaFreeSelectToolClass;

struct _LigmaFreeSelectTool
{
  LigmaPolygonSelectTool      parent_instance;

  LigmaFreeSelectToolPrivate *priv;
};

struct _LigmaFreeSelectToolClass
{
  LigmaPolygonSelectToolClass  parent_class;
};


void    ligma_free_select_tool_register     (LigmaToolRegisterCallback  callback,
                                            gpointer                  data);

GType   ligma_free_select_tool_get_type     (void) G_GNUC_CONST;


#endif  /*  __LIGMA_FREE_SELECT_TOOL_H__  */
