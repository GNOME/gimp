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

#ifndef __GIMP_PAINTBRUSH_TOOL_H__
#define __GIMP_PAINTBRUSH_TOOL_H__


#include "gimppainttool.h"


#define GIMP_TYPE_PAINTBRUSH_TOOL            (gimp_paintbrush_tool_get_type ())
#define GIMP_PAINTBRUSH_TOOL(obj)            (GTK_CHECK_CAST ((obj), GIMP_TYPE_PAINTBRUSH_TOOL, GimpPaintbrushTool))
#define GIMP_IS_PAINTBRUSH_TOOL(obj)         (GTK_CHECK_TYPE ((obj), GIMP_TYPE_PAINTBRUSH_TOOL))
#define GIMP_PAINTBRUSH_TOOL_CLASS(klass)    (GTK_CHECK_CLASS_CAST ((klass), GIMP_TYPE_PAINTBRUSH_TOOL, GimpPaintbrushToolClass))
#define GIMP_IS_PAINTBRUSH_TOOL_CLASS(klass) (GTK_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_PAINTBRUSH_TOOL))


typedef struct _GimpPaintbrushTool      GimpPaintbrushTool;
typedef struct _GimpPaintbrushToolClass GimpPaintbrushToolClass;

struct _GimpPaintbrushTool
{
  GimpPaintTool parent_instance;
};

struct _GimpPaintbrushToolClass
{
  GimpPaintToolClass parent_class;
};


void       gimp_paintbrush_tool_register        (void);

GtkType    gimp_paintbrush_tool_get_type        (void);

gboolean   gimp_paintbrush_tool_non_gui         (GimpDrawable *drawable,
						 gint          num_srokes,
						 gdouble      *stroke_array,
						 gdouble       fade_out,
						 gint          incremental,
						 gdouble       gradient_length);
gboolean   gimp_paintbrush_tool_non_gui_default (GimpDrawable *drawable,
						 gint          num_strokes,
						 gdouble      *stroke_array);


#endif  /*  __GIMP_PAINTBRUSH_TOOL_H__  */
