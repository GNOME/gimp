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

#ifndef __GIMP_CONVOLVE_TOOL_H__
#define __GIMP_CONVOLVE_TOOL_H__


#include "gimppainttool.h"


typedef enum
{
  BLUR_CONVOLVE,
  SHARPEN_CONVOLVE,
  CUSTOM_CONVOLVE
} ConvolveType;


#define GIMP_TYPE_CONVOLVE_TOOL            (gimp_convolve_tool_get_type ())
#define GIMP_CONVOLVE_TOOL(obj)            (GTK_CHECK_CAST ((obj), GIMP_TYPE_CONVOLVE_TOOL, GimpConvolveTool))
#define GIMP_IS_CONVOLVE_TOOL(obj)         (GTK_CHECK_TYPE ((obj), GIMP_TYPE_CONVOLVE_TOOL))
#define GIMP_CONVOLVE_TOOL_CLASS(klass)    (GTK_CHECK_CLASS_CAST ((klass), GIMP_TYPE_CONVOLVE_TOOL, GimpConvolveToolClass))
#define GIMP_IS_CONVOLVE_TOOL_CLASS(klass) (GTK_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_CONVOLVE_TOOL))


typedef struct _GimpConvolveTool      GimpConvolveTool;
typedef struct _GimpConvolveToolClass GimpConvolveToolClass;

struct _GimpConvolveTool
{
  GimpPaintTool parent_instance;
};

struct _GimpConvolveToolClass
{
  GimpPaintToolClass parent_class;
};


void       gimp_convolve_tool_register (Gimp *gimp);

GtkType    gimp_convolve_tool_get_type (void);


/* FIXME: These need to disappear */
gboolean   convolve_non_gui         (GimpDrawable *drawable,
				     gdouble       rate,
				     ConvolveType  type,
				     gint          num_strokes,
				     gdouble      *stroke_array);
gboolean   convolve_non_gui_default (GimpDrawable *drawable,
				     gint          num_strokes,
				     gdouble      *stroke_array);


#endif  /*  __GIMP_CONVOLVE_TOOL_H__  */
