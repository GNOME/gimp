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

#ifndef __GIMP_SOURCE_TOOL_H__
#define __GIMP_SOURCE_TOOL_H__


#include "gimpbrushtool.h"


#define GIMP_TYPE_SOURCE_TOOL            (gimp_source_tool_get_type ())
#define GIMP_SOURCE_TOOL(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_SOURCE_TOOL, GimpSourceTool))
#define GIMP_SOURCE_TOOL_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_SOURCE_TOOL, GimpSourceToolClass))
#define GIMP_IS_SOURCE_TOOL(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_SOURCE_TOOL))
#define GIMP_IS_SOURCE_TOOL_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_SOURCE_TOOL))
#define GIMP_SOURCE_TOOL_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_SOURCE_TOOL, GimpSourceToolClass))

#define GIMP_SOURCE_TOOL_GET_OPTIONS(t)  (GIMP_SOURCE_OPTIONS (gimp_tool_get_options (GIMP_TOOL (t))))


typedef struct _GimpSourceTool      GimpSourceTool;
typedef struct _GimpSourceToolClass GimpSourceToolClass;

struct _GimpSourceTool
{
  GimpBrushTool        parent_instance;

  GimpDisplay         *src_display;
  gint                 src_x;
  gint                 src_y;

  gboolean             show_source_outline;
  GimpCursorPrecision  saved_precision;

  GimpCanvasItem      *src_handle;
  GimpCanvasItem      *src_outline;

  const gchar         *status_paint;
  const gchar         *status_set_source;
  const gchar         *status_set_source_ctrl;
};

struct _GimpSourceToolClass
{
  GimpBrushToolClass  parent_class;
};


GType   gimp_source_tool_get_type (void) G_GNUC_CONST;


#endif  /*  __GIMP_SOURCE_TOOL_H__  */
