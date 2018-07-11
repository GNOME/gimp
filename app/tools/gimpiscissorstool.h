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

#ifndef __GIMP_ISCISSORS_TOOL_H__
#define __GIMP_ISCISSORS_TOOL_H__


#include "gimpselectiontool.h"


/*  The possible states...  */
typedef enum
{
  NO_ACTION,
  SEED_PLACEMENT,
  SEED_ADJUSTMENT,
  WAITING
} IscissorsState;

/*  For oper_update & cursor_update  */
typedef enum
{
  ISCISSORS_OP_NONE,
  ISCISSORS_OP_SELECT,
  ISCISSORS_OP_MOVE_POINT,
  ISCISSORS_OP_ADD_POINT,
  ISCISSORS_OP_REMOVE_POINT,
  ISCISSORS_OP_CONNECT,
  ISCISSORS_OP_IMPOSSIBLE
} IscissorsOps;

typedef struct _ISegment ISegment;
typedef struct _ICurve   ICurve;


#define GIMP_TYPE_ISCISSORS_TOOL            (gimp_iscissors_tool_get_type ())
#define GIMP_ISCISSORS_TOOL(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_ISCISSORS_TOOL, GimpIscissorsTool))
#define GIMP_ISCISSORS_TOOL_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_ISCISSORS_TOOL, GimpIscissorsToolClass))
#define GIMP_IS_ISCISSORS_TOOL(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_ISCISSORS_TOOL))
#define GIMP_IS_ISCISSORS_TOOL_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_ISCISSORS_TOOL))
#define GIMP_ISCISSORS_TOOL_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_ISCISSORS_TOOL, GimpIscissorsToolClass))

#define GIMP_ISCISSORS_TOOL_GET_OPTIONS(t)  (GIMP_ISCISSORS_OPTIONS (gimp_tool_get_options (GIMP_TOOL (t))))


typedef struct _GimpIscissorsTool      GimpIscissorsTool;
typedef struct _GimpIscissorsToolClass GimpIscissorsToolClass;

struct _GimpIscissorsTool
{
  GimpSelectionTool  parent_instance;

  IscissorsOps    op;

  gint            x, y;         /*  mouse coordinates                       */

  ISegment       *segment1;     /*  1st segment connected to current point  */
  ISegment       *segment2;     /*  2nd segment connected to current point  */

  ICurve         *curve;        /*  the curve                               */

  GList          *undo_stack;   /*  stack of ICurves for undo               */
  GList          *redo_stack;   /*  stack of ICurves for redo               */

  IscissorsState  state;        /*  state of iscissors                      */

  GeglBuffer     *gradient_map; /*  lazily filled gradient map              */
  GimpChannel    *mask;         /*  selection mask                          */
};

struct _GimpIscissorsToolClass
{
  GimpSelectionToolClass parent_class;
};


void    gimp_iscissors_tool_register (GimpToolRegisterCallback  callback,
                                      gpointer                  data);

GType   gimp_iscissors_tool_get_type (void) G_GNUC_CONST;


#endif  /*  __GIMP_ISCISSORS_TOOL_H__  */
