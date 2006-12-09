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

#ifndef __GIMP_MOVE_TOOL_H__
#define __GIMP_MOVE_TOOL_H__


#include "gimpdrawtool.h"


#define GIMP_TYPE_MOVE_TOOL            (gimp_move_tool_get_type ())
#define GIMP_MOVE_TOOL(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_MOVE_TOOL, GimpMoveTool))
#define GIMP_MOVE_TOOL_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_MOVE_TOOL, GimpMoveToolClass))
#define GIMP_IS_MOVE_TOOL(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_MOVE_TOOL))
#define GIMP_IS_MOVE_TOOL_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_MOVE_TOOL))
#define GIMP_MOVE_TOOL_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_MOVE_TOOL, GimpMoveToolClass))

#define GIMP_MOVE_TOOL_GET_OPTIONS(t)  (GIMP_MOVE_OPTIONS (gimp_tool_get_options (GIMP_TOOL (t))))


typedef struct _GimpMoveTool      GimpMoveTool;
typedef struct _GimpMoveToolClass GimpMoveToolClass;

struct _GimpMoveTool
{
  GimpDrawTool         parent_instance;

  GimpLayer           *floating_layer;
  GimpGuide           *guide;

  gboolean             moving_guide;
  gint                 guide_position;
  GimpOrientationType  guide_orientation;

  GimpTransformType    saved_type;

  GimpLayer           *old_active_layer;
  GimpVectors         *old_active_vectors;
};

struct _GimpMoveToolClass
{
  GimpDrawToolClass parent_class;
};


void    gimp_move_tool_register (GimpToolRegisterCallback  callback,
                                 gpointer                  data);

GType   gimp_move_tool_get_type (void) G_GNUC_CONST;


void    gimp_move_tool_start_hguide (GimpTool    *tool,
                                     GimpDisplay *display);
void    gimp_move_tool_start_vguide (GimpTool    *tool,
                                     GimpDisplay *display);


#endif  /*  __GIMP_MOVE_TOOL_H__  */
