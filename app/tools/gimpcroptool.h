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

#ifndef  __GIMP_CROP_TOOL_H__
#define  __GIMP_CROP_TOOL_H__


#include "gimpdrawtool.h"


#define GIMP_TYPE_CROP_TOOL            (gimp_crop_tool_get_type ())
#define GIMP_CROP_TOOL(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_CROP_TOOL, GimpCropTool))
#define GIMP_CROP_TOOL_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_CROP_TOOL, GimpCropToolClass))
#define GIMP_IS_CROP_TOOL(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_CROP_TOOL))
#define GIMP_IS_CROP_TOOL_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_CROP_TOOL))
#define GIMP_CROP_TOOL_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_CROP_TOOL, GimpCropToolClass))

#define GIMP_CROP_TOOL_GET_OPTIONS(t)  (GIMP_CROP_OPTIONS (gimp_tool_get_options (GIMP_TOOL (t))))


typedef struct _GimpCropTool      GimpCropTool;
typedef struct _GimpCropToolClass GimpCropToolClass;

struct _GimpCropTool
{
  GimpDrawTool    parent_instance;

  GimpImage      *current_image;
  GList          *current_layers;

  GimpToolWidget *widget;
  GimpToolWidget *grab_widget;
  GList          *bindings;
};

struct _GimpCropToolClass
{
  GimpDrawToolClass parent_class;
};


void    gimp_crop_tool_register (GimpToolRegisterCallback  callback,
                                 gpointer                  data);

GType   gimp_crop_tool_get_type (void) G_GNUC_CONST;


#endif  /*  __GIMP_CROP_TOOL_H__  */
