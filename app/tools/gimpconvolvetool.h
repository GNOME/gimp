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

#include "gimpbrushtool.h"


#define GIMP_TYPE_CONVOLVE_TOOL            (gimp_convolve_tool_get_type ())
#define GIMP_CONVOLVE_TOOL(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_CONVOLVE_TOOL, GimpConvolveTool))
#define GIMP_CONVOLVE_TOOL_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_CONVOLVE_TOOL, GimpConvolveToolClass))
#define GIMP_IS_CONVOLVE_TOOL(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_CONVOLVE_TOOL))
#define GIMP_IS_CONVOLVE_TOOL_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_CONVOLVE_TOOL))
#define GIMP_CONVOLVE_TOOL_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_CONVOLVE_TOOL, GimpConvolveToolClass))

#define GIMP_CONVOLVE_TOOL_GET_OPTIONS(t)  (GIMP_CONVOLVE_OPTIONS (gimp_tool_get_options (GIMP_TOOL (t))))


typedef struct _GimpConvolveTool      GimpConvolveTool;
typedef struct _GimpConvolveToolClass GimpConvolveToolClass;

struct _GimpConvolveTool
{
  GimpBrushTool parent_instance;

  gboolean      toggled;
};

struct _GimpConvolveToolClass
{
  GimpBrushToolClass parent_class;
};


void    gimp_convolve_tool_register (GimpToolRegisterCallback  callback,
                                     gpointer                  data);

GType   gimp_convolve_tool_get_type (void) G_GNUC_CONST;
