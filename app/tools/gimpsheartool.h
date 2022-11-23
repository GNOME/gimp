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

#ifndef __LIGMA_SHEAR_TOOL_H__
#define __LIGMA_SHEAR_TOOL_H__


#include "ligmatransformgridtool.h"


#define LIGMA_TYPE_SHEAR_TOOL            (ligma_shear_tool_get_type ())
#define LIGMA_SHEAR_TOOL(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), LIGMA_TYPE_SHEAR_TOOL, LigmaShearTool))
#define LIGMA_SHEAR_TOOL_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), LIGMA_TYPE_SHEAR_TOOL, LigmaShearToolClass))
#define LIGMA_IS_SHEAR_TOOL(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LIGMA_TYPE_SHEAR_TOOL))
#define LIGMA_IS_SHEAR_TOOL_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), LIGMA_TYPE_SHEAR_TOOL))
#define LIGMA_SHEAR_TOOL_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), LIGMA_TYPE_SHEAR_TOOL, LigmaShearToolClass))


typedef struct _LigmaShearTool      LigmaShearTool;
typedef struct _LigmaShearToolClass LigmaShearToolClass;

struct _LigmaShearTool
{
  LigmaTransformGridTool  parent_instance;

  GtkAdjustment         *x_adj;
  GtkAdjustment         *y_adj;
};

struct _LigmaShearToolClass
{
  LigmaTransformGridToolClass  parent_class;
};


void    ligma_shear_tool_register (LigmaToolRegisterCallback  callback,
                                  gpointer                  data);

GType   ligma_shear_tool_get_type (void) G_GNUC_CONST;


#endif  /*  __LIGMA_SHEAR_TOOL_H__  */
