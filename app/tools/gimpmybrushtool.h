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

#include "gimppainttool.h"


#define GIMP_TYPE_MYBRUSH_TOOL (gimp_mybrush_tool_get_type ())
G_DECLARE_DERIVABLE_TYPE (GimpMybrushTool,
                          gimp_mybrush_tool,
                          GIMP, MYBRUSH_TOOL,
                          GimpPaintTool)

#define GIMP_MYBRUSH_TOOL_GET_OPTIONS(t) (GIMP_MYBRUSH_OPTIONS (gimp_tool_get_options (GIMP_TOOL (t))))


struct _GimpMybrushToolClass
{
  GimpPaintToolClass parent_class;
};


void   gimp_mybrush_tool_register (GimpToolRegisterCallback  callback,
                                   gpointer                  data);

GimpCanvasItem * gimp_mybrush_tool_create_cursor (GimpPaintTool *paint_tool,
                                                  GimpDisplay   *display,
                                                  gdouble        x,
                                                  gdouble        y,
                                                  gdouble        radius);
