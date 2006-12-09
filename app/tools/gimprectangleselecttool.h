/* GIMP - The GNU Image Manipulation Program
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

#ifndef  __GIMP_RECT_SELECT_TOOL_H__
#define  __GIMP_RECT_SELECT_TOOL_H__


#include "gimpselectiontool.h"


#define GIMP_TYPE_RECT_SELECT_TOOL            (gimp_rect_select_tool_get_type ())
#define GIMP_RECT_SELECT_TOOL(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_RECT_SELECT_TOOL, GimpRectSelectTool))
#define GIMP_RECT_SELECT_TOOL_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_RECT_SELECT_TOOL, GimpRectSelectToolClass))
#define GIMP_IS_RECT_SELECT_TOOL(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_RECT_SELECT_TOOL))
#define GIMP_IS_RECT_SELECT_TOOL_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_RECT_SELECT_TOOL))
#define GIMP_RECT_SELECT_TOOL_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_RECT_SELECT_TOOL, GimpRectSelectToolClass))

#define GIMP_RECT_SELECT_TOOL_GET_OPTIONS(t)  (GIMP_RECT_SELECT_OPTIONS (gimp_tool_get_options (GIMP_TOOL (t))))


typedef struct _GimpRectSelectTool      GimpRectSelectTool;
typedef struct _GimpRectSelectToolClass GimpRectSelectToolClass;

struct _GimpRectSelectTool
{
  GimpSelectionTool  parent_instance;

  GimpChannelOps     operation;            /* remember for use when modifying   */
  gboolean           use_saved_op;         /* use operation or get from options */
  gboolean           saved_show_selection; /* used to remember existing value   */
  GimpUndo          *undo;
  GimpUndo          *redo;

  gboolean           round_corners;
  gdouble            corner_radius;
};

struct _GimpRectSelectToolClass
{
  GimpSelectionToolClass parent_class;

  void (* select) (GimpRectSelectTool *rect_select,
                   GimpChannelOps      operation,
                   gint                x,
                   gint                y,
                   gint                w,
                   gint                h);
};


void    gimp_rect_select_tool_register (GimpToolRegisterCallback  callback,
                                        gpointer                  data);

GType   gimp_rect_select_tool_get_type (void) G_GNUC_CONST;


#endif  /*  __GIMP_RECT_SELECT_TOOL_H__  */
