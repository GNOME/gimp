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

#include "gimpfiltertool.h"


#define GIMP_TYPE_OFFSET_TOOL            (gimp_offset_tool_get_type ())
#define GIMP_OFFSET_TOOL(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_OFFSET_TOOL, GimpOffsetTool))
#define GIMP_OFFSET_TOOL_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_OFFSET_TOOL, GimpOffsetToolClass))
#define GIMP_IS_OFFSET_TOOL(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_OFFSET_TOOL))
#define GIMP_IS_OFFSET_TOOL_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_OFFSET_TOOL))
#define GIMP_OFFSET_TOOL_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_OFFSET_TOOL, GimpOffsetToolClass))


typedef struct _GimpOffsetTool      GimpOffsetTool;
typedef struct _GimpOffsetToolClass GimpOffsetToolClass;

struct _GimpOffsetTool
{
  GimpFilterTool  parent_instance;

  gboolean        dragging;
  gdouble         x;
  gdouble         y;
  gint            offset_x;
  gint            offset_y;

  /* dialog */
  GtkWidget      *offset_se;
  GtkWidget      *transparent_radio;
  GtkWidget      *color_button;
};

struct _GimpOffsetToolClass
{
  GimpFilterToolClass  parent_class;
};


void    gimp_offset_tool_register (GimpToolRegisterCallback callback,
                                   gpointer                 data);

GType   gimp_offset_tool_get_type (void) G_GNUC_CONST;
