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

#ifndef __GIMP_SMUDGE_TOOL_H__
#define __GIMP_SMUDGE_TOOL_H__


#include "gimppainttool.h"


typedef enum
{
  SMUDGE_TYPE_SMUDGE,
  SMUDGE_TYPE_STREAK 
} SmudgeType;

typedef enum
{
  SMUDGE_MODE_HIGHLIGHTS,
  SMUDGE_MODE_MIDTONES,
  SMUDGE_MODE_SHADOWS 
} SmudgeMode;


#define GIMP_TYPE_SMUDGE_TOOL            (gimp_smudge_tool_get_type ())
#define GIMP_SMUDGE_TOOL(obj)            (GTK_CHECK_CAST ((obj), GIMP_TYPE_SMUDGE_TOOL, GimpSmudgeTool))
#define GIMP_IS_SMUDGE_TOOL(obj)         (GTK_CHECK_TYPE ((obj), GIMP_TYPE_SMUDGE_TOOL))
#define GIMP_SMUDGE_TOOL_CLASS(klass)    (GTK_CHECK_CLASS_CAST ((klass), GIMP_TYPE_SMUDGE_TOOL, GimpSmudgeToolClass))
#define GIMP_IS_SMUDGE_TOOL_CLASS(klass) (GTK_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_SMUDGE_TOOL))


typedef struct _GimpSmudgeTool      GimpSmudgeTool;
typedef struct _GimpSmudgeToolClass GimpSmudgeToolClass;

struct _GimpSmudgeTool
{
  GimpPaintTool parent_instance;
};

struct _GimpSmudgeToolClass
{
  GimpPaintToolClass parent_class;
};


void       gimp_smudge_tool_register (void);

GtkType    gimp_smudge_tool_get_type (void);



/* FIXME: this antique code doesn't follow the coding style */
gboolean   gimp_smudge_tool_non_gui          (GimpDrawable *drawable,
					      gdouble       rate,
					      gint          num_strokes,
					      gdouble      *stroke_array);
gboolean   gimp_smudge_tool_non_gui_default  (GimpDrawable *drawable,
					      gint          num_strokes,
					      gdouble      *stroke_array);


#endif  /*  __GIMP_SMUDGE_TOOL_H__  */
