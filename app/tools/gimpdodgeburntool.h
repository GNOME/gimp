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

#ifndef __LIGMA_DODGE_BURN_TOOL_H__
#define __LIGMA_DODGE_BURN_TOOL_H__


#include "ligmabrushtool.h"


#define LIGMA_TYPE_DODGE_BURN_TOOL            (ligma_dodge_burn_tool_get_type ())
#define LIGMA_DODGE_BURN_TOOL(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), LIGMA_TYPE_DODGE_BURN_TOOL, LigmaDodgeBurnTool))
#define LIGMA_IS_DODGE_BURN_TOOL(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LIGMA_TYPE_DODGE_BURN_TOOL))
#define LIGMA_DODGE_BURN_TOOL_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), LIGMA_TYPE_DODGE_BURN_TOOL, LigmaDodgeBurnToolClass))
#define LIGMA_IS_DODGE_BURN_TOOL_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), LIGMA_TYPE_DODGE_BURN_TOOL))

#define LIGMA_DODGE_BURN_TOOL_GET_OPTIONS(t)  (LIGMA_DODGE_BURN_OPTIONS (ligma_tool_get_options (LIGMA_TOOL (t))))


typedef struct _LigmaDodgeBurnTool      LigmaDodgeBurnTool;
typedef struct _LigmaDodgeBurnToolClass LigmaDodgeBurnToolClass;

struct _LigmaDodgeBurnTool
{
  LigmaBrushTool parent_instance;

  gboolean      toggled;
};

struct _LigmaDodgeBurnToolClass
{
  LigmaBrushToolClass parent_class;
};


void    ligma_dodge_burn_tool_register (LigmaToolRegisterCallback  callback,
                                       gpointer                  data);

GType   ligma_dodge_burn_tool_get_type (void) G_GNUC_CONST;


#endif  /*  __LIGMA_DODGEBURN_TOOL_H__  */
