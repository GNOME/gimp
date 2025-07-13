/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpregionselecttool.h
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

#include "gimpselectiontool.h"


#define GIMP_TYPE_REGION_SELECT_TOOL            (gimp_region_select_tool_get_type ())
#define GIMP_REGION_SELECT_TOOL(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_REGION_SELECT_TOOL, GimpRegionSelectTool))
#define GIMP_REGION_SELECT_TOOL_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_REGION_SELECT_TOOL, GimpRegionSelectToolClass))
#define GIMP_IS_REGION_SELECT_TOOL(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_REGION_SELECT_TOOL))
#define GIMP_IS_REGION_SELECT_TOOL_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_REGION_SELECT_TOOL))
#define GIMP_REGION_SELECT_TOOL_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_REGION_SELECT_TOOL, GimpRegionSelectToolClass))

#define GIMP_REGION_SELECT_TOOL_GET_OPTIONS(t)  (GIMP_REGION_SELECT_OPTIONS (gimp_tool_get_options (GIMP_TOOL (t))))


typedef struct _GimpRegionSelectTool      GimpRegionSelectTool;
typedef struct _GimpRegionSelectToolClass GimpRegionSelectToolClass;

struct _GimpRegionSelectTool
{
  GimpSelectionTool  parent_instance;

  gint               x, y;
  gdouble            saved_threshold;

  GeglBuffer        *region_mask;
  GimpBoundSeg      *segs;
  gint               n_segs;
};

struct _GimpRegionSelectToolClass
{
  GimpSelectionToolClass parent_class;

  const gchar * undo_desc;

  GeglBuffer * (* get_mask) (GimpRegionSelectTool *region_tool,
                             GimpDisplay          *display);
};


GType   gimp_region_select_tool_get_type (void) G_GNUC_CONST;
