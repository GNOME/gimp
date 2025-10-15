/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * Path tool
 * Copyright (C) 2003 Simon Budig  <simon@gimp.org>
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

#include "gimpdrawtool.h"


#define GIMP_TYPE_PATH_TOOL            (gimp_path_tool_get_type ())
#define GIMP_PATH_TOOL(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_PATH_TOOL, GimpPathTool))
#define GIMP_PATH_TOOL_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_PATH_TOOL, GimpPathToolClass))
#define GIMP_IS_PATH_TOOL(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_PATH_TOOL))
#define GIMP_IS_PATH_TOOL_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_PATH_TOOL))
#define GIMP_PATH_TOOL_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_PATH_TOOL, GimpPathToolClass))

#define GIMP_PATH_TOOL_GET_OPTIONS(t)  (GIMP_PATH_OPTIONS (gimp_tool_get_options (GIMP_TOOL (t))))


typedef struct _GimpPathTool      GimpPathTool;
typedef struct _GimpPathToolClass GimpPathToolClass;

struct _GimpPathTool
{
  GimpDrawTool     parent_instance;

  GimpImage       *current_image;
  GimpVectorLayer *current_vector_layer;

  GimpPath        *path;           /*  the current Path data   */
  GimpPathMode     saved_mode;     /*  used by modifier_key()  */

  GimpToolWidget  *widget;
  GimpToolWidget  *grab_widget;

  GtkWidget       *confirm_dialog;
};

struct _GimpPathToolClass
{
  GimpDrawToolClass  parent_class;
};


void    gimp_path_tool_register (GimpToolRegisterCallback  callback,
                                 gpointer                  data);

GType   gimp_path_tool_get_type (void) G_GNUC_CONST;

void    gimp_path_tool_set_path (GimpPathTool             *path_tool,
                                 GimpVectorLayer          *layer,
                                 GimpPath                 *path);
