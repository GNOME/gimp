/* GIMP - The GNU Image Manipulation Program
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
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef __GIMP_OPERATION_TOOL_H__
#define __GIMP_OPERATION_TOOL_H__


#include "gimpimagemaptool.h"


#define GIMP_TYPE_OPERATION_TOOL            (gimp_operation_tool_get_type ())
#define GIMP_OPERATION_TOOL(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_OPERATION_TOOL, GimpOperationTool))
#define GIMP_OPERATION_TOOL_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_OPERATION_TOOL, GimpOperationToolClass))
#define GIMP_IS_OPERATION_TOOL(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_OPERATION_TOOL))
#define GIMP_IS_OPERATION_TOOL_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_OPERATION_TOOL))
#define GIMP_OPERATION_TOOL_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_OPERATION_TOOL, GimpOperationToolClass))


typedef struct _GimpOperationTool      GimpOperationTool;
typedef struct _GimpOperationToolClass GimpOperationToolClass;

struct _GimpOperationTool
{
  GimpImageMapTool  parent_instance;

  gchar            *operation;
  gchar            *undo_desc;
  gchar            *icon_name;
  GimpObject       *config;
  GeglNode         *aux_input;

  /* dialog */
  GtkWidget        *options_box;
  GtkWidget        *options_gui;
  GtkWidget        *aux_input_box;
  GtkWidget        *aux_input_button;
};

struct _GimpOperationToolClass
{
  GimpImageMapToolClass  parent_class;
};


void    gimp_operation_tool_register      (GimpToolRegisterCallback  callback,
                                           gpointer                  data);

GType   gimp_operation_tool_get_type      (void) G_GNUC_CONST;

void    gimp_operation_tool_set_operation (GimpOperationTool        *tool,
                                           const gchar              *operation,
                                           const gchar              *undo_desc,
                                           const gchar              *icon_name);


#endif  /*  __GIMP_OPERATION_TOOL_H__  */
