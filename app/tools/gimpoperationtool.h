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

#ifndef __LIGMA_OPERATION_TOOL_H__
#define __LIGMA_OPERATION_TOOL_H__


#include "ligmafiltertool.h"


#define LIGMA_TYPE_OPERATION_TOOL            (ligma_operation_tool_get_type ())
#define LIGMA_OPERATION_TOOL(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), LIGMA_TYPE_OPERATION_TOOL, LigmaOperationTool))
#define LIGMA_OPERATION_TOOL_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), LIGMA_TYPE_OPERATION_TOOL, LigmaOperationToolClass))
#define LIGMA_IS_OPERATION_TOOL(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LIGMA_TYPE_OPERATION_TOOL))
#define LIGMA_IS_OPERATION_TOOL_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), LIGMA_TYPE_OPERATION_TOOL))
#define LIGMA_OPERATION_TOOL_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), LIGMA_TYPE_OPERATION_TOOL, LigmaOperationToolClass))


typedef struct _LigmaOperationTool      LigmaOperationTool;
typedef struct _LigmaOperationToolClass LigmaOperationToolClass;

struct _LigmaOperationTool
{
  LigmaFilterTool  parent_instance;

  gchar          *operation;
  gchar          *description;

  GList          *aux_inputs;

  /* dialog */
  GWeakRef        options_sw_ref;
  GWeakRef        options_box_ref;
  GWeakRef        options_gui_ref;
};

struct _LigmaOperationToolClass
{
  LigmaFilterToolClass  parent_class;
};


void    ligma_operation_tool_register      (LigmaToolRegisterCallback  callback,
                                           gpointer                  data);

GType   ligma_operation_tool_get_type      (void) G_GNUC_CONST;

void    ligma_operation_tool_set_operation (LigmaOperationTool        *op_tool,
                                           const gchar              *operation,
                                           const gchar              *title,
                                           const gchar              *description,
                                           const gchar              *undo_desc,
                                           const gchar              *icon_name,
                                           const gchar              *help_id);


#endif  /*  __LIGMA_OPERATION_TOOL_H__  */
