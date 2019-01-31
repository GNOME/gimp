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

#ifndef __GIMP_PERSPECTIVE_CLONE_TOOL_H__
#define __GIMP_PERSPECTIVE_CLONE_TOOL_H__


#include "gimpbrushtool.h"
#include "gimptransformtool.h"  /* for TransInfo */


#define GIMP_TYPE_PERSPECTIVE_CLONE_TOOL            (gimp_perspective_clone_tool_get_type ())
#define GIMP_PERSPECTIVE_CLONE_TOOL(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_PERSPECTIVE_CLONE_TOOL, GimpPerspectiveCloneTool))
#define GIMP_PERSPECTIVE_CLONE_TOOL_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_PERSPECTIVE_CLONE_TOOL, GimpPerspectiveCloneToolClass))
#define GIMP_IS_PERSPECTIVE_CLONE_TOOL(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_PERSPECTIVE_CLONE_TOOL))
#define GIMP_IS_PERSPECTIVE_CLONE_TOOL_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_PERSPECTIVE_CLONE_TOOL))
#define GIMP_PERSPECTIVE_CLONE_TOOL_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_PERSPECTIVE_CLONE_TOOL, GimpPerspectiveCloneToolClass))

#define GIMP_PERSPECTIVE_CLONE_TOOL_GET_OPTIONS(t)  (GIMP_PERSPECTIVE_CLONE_OPTIONS (gimp_tool_get_options (GIMP_TOOL (t))))


typedef struct _GimpPerspectiveCloneTool      GimpPerspectiveCloneTool;
typedef struct _GimpPerspectiveCloneToolClass GimpPerspectiveCloneToolClass;

struct _GimpPerspectiveCloneTool
{
  GimpBrushTool        parent_instance;

  GimpDisplay         *src_display;
  gint                 src_x;
  gint                 src_y;

  GimpMatrix3          transform;      /*  transformation matrix            */
  TransInfo            trans_info;     /*  transformation info              */
  TransInfo            old_trans_info; /*  for cancelling a drag operation  */

  gint                 x1, y1;         /*  upper left hand coordinate       */
  gint                 x2, y2;         /*  lower right hand coords          */

  GimpCursorPrecision  saved_precision;

  GimpToolWidget      *widget;
  GimpToolWidget      *grab_widget;
};

struct _GimpPerspectiveCloneToolClass
{
  GimpBrushToolClass  parent_class;
};


void    gimp_perspective_clone_tool_register (GimpToolRegisterCallback  callback,
                                              gpointer                  data);

GType   gimp_perspective_clone_tool_get_type (void) G_GNUC_CONST;


#endif  /*  __GIMP_PERSPECTIVE_CLONE_TOOL_H__  */
