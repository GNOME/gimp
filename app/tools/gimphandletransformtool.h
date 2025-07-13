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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#pragma once

#include "gimpgenerictransformtool.h"


#define GIMP_TYPE_HANDLE_TRANSFORM_TOOL            (gimp_handle_transform_tool_get_type ())
#define GIMP_HANDLE_TRANSFORM_TOOL(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_HANDLE_TRANSFORM_TOOL, GimpHandleTransformTool))
#define GIMP_HANDLE_TRANSFORM_TOOL_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_HANDLE_TRANSFORM_TOOL, GimpHandleTransformToolClass))
#define GIMP_IS_HANDLE_TRANSFORM_TOOL(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_HANDLE_TRANSFORM_TOOL))
#define GIMP_IS_HANDLE_TRANSFORM_TOOL_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_HANDLE_TRANSFORM_TOOL))
#define GIMP_HANDLE_TRANSFORM_TOOL_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_HANDLE_TRANSFORM_TOOL, GimpHandleTransformToolClass))

#define GIMP_HANDLE_TRANSFORM_TOOL_GET_OPTIONS(t)  (GIMP_HANDLE_TRANSFORM_OPTIONS (gimp_tool_get_options (GIMP_TOOL (t))))


typedef struct _GimpHandleTransformTool      GimpHandleTransformTool;
typedef struct _GimpHandleTransformToolClass GimpHandleTransformToolClass;

struct _GimpHandleTransformTool
{
  GimpGenericTransformTool  parent_instance;

  GimpTransformHandleMode   saved_handle_mode;
};

struct _GimpHandleTransformToolClass
{
  GimpGenericTransformToolClass  parent_class;
};


void    gimp_handle_transform_tool_register (GimpToolRegisterCallback  callback,
                                             gpointer                  data);

GType   gimp_handle_transform_tool_get_type (void) G_GNUC_CONST;
