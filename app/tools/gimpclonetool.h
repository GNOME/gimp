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

#ifndef __GIMP_CLONE_TOOL_H__
#define __GIMP_CLONE_TOOL_H__


#include "gimppainttool.h"


typedef enum
{
  IMAGE_CLONE,
  PATTERN_CLONE
} CloneType;


#define GIMP_TYPE_CLONE_TOOL            (gimp_clone_tool_get_type ())
#define GIMP_CLONE_TOOL(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_CLONE_TOOL, GimpCloneTool))
#define GIMP_CLONE_TOOL_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_CLONE_TOOL, GimpCloneToolClass))
#define GIMP_IS_CLONE_TOOL(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_CLONE_TOOL))
#define GIMP_IS_CLONE_TOOL_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_CLONE_TOOL))
#define GIMP_CLONE_TOOL_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_CLONE_TOOL, GimpCloneToolClass))


typedef struct _GimpCloneTool      GimpCloneTool;
typedef struct _GimpCloneToolClass GimpCloneToolClass;

struct _GimpCloneTool
{
  GimpPaintTool parent_instance;
};

struct _GimpCloneToolClass
{
  GimpPaintToolClass parent_class;
};


void    gimp_clone_tool_register (Gimp                     *gimp,
                                  GimpToolRegisterCallback  callback);

GType   gimp_clone_tool_get_type (void);


/* FIXME: Old style functions in need of a replacement. The only 
 * time these are used is to stroke paths or fill selections
 * They should be somewhere else.
 */

gboolean   clone_non_gui         (GimpDrawable *drawable,
				  GimpDrawable *src_drawable,
				  CloneType     clone_type,
				  gdouble       src_x,
				  gdouble       src_y,
				  gint          num_strokes,
				  gdouble      *stroke_array);
gboolean   clone_non_gui_default (GimpDrawable *drawable,
				  gint          num_strokes,
				  gdouble      *stroke_array);


#endif  /*  __GIMP_CLONE_TOOL_H__  */
