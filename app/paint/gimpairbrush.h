/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#ifndef __GIMP_AIRBRUSH_TOOL_H__
#define __GIMP_AIRBRUSH_TOOL_H__


#include "gimppainttool.h"


#define GIMP_TYPE_AIRBRUSH_TOOL            (gimp_airbrush_tool_get_type ())
#define GIMP_AIRBRUSH_TOOL(obj)            (GTK_CHECK_CAST ((obj), GIMP_TYPE_AIRBRUSH_TOOL, GimpAirbrushTool))
#define GIMP_IS_AIRBRUSH_TOOL(obj)         (GTK_CHECK_TYPE ((obj), GIMP_TYPE_AIRBRUSH_TOOL))
#define GIMP_AIRBRUSH_TOOL_CLASS(klass)    (GTK_CHECK_CLASS_CAST ((klass), GIMP_TYPE_AIRBRUSH_TOOL, GimpAirbrushToolClass))
#define GIMP_IS_AIRBRUSH_TOOL_CLASS(klass) (GTK_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_AIRBRUSH_TOOL))


typedef struct _GimpAirbrushTool      GimpAirbrushTool;
typedef struct _GimpAirbrushToolClass GimpAirbrushToolClass;

struct _GimpAirbrushTool
{
  GimpPaintTool parent_instance;
};

struct _GimpAirbrushToolClass
{
  GimpPaintToolClass parent_class;
};


void       gimp_airbrush_tool_register (void);

GtkType    gimp_airbrush_tool_get_type (void);

gboolean   airbrush_non_gui            (GimpDrawable *drawable,
					gdouble       pressure,
					gint          num_strokes,
					gdouble      *stroke_array);
gboolean   airbrush_non_gui_default    (GimpDrawable *drawable,
					gint          num_strokes,
					gdouble      *stroke_array);


#endif  /*  __GIMP_AIRBRUSH_TOOL_H__  */
