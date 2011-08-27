/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995-2001 Spencer Kimball, Peter Mattis, and others
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
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef __GIMP_TRANSFORM_TOOL_H__
#define __GIMP_TRANSFORM_TOOL_H__


#include "gimpdrawtool.h"


#define TRANS_INFO_SIZE  8

typedef enum
{
  TRANSFORM_CREATING,
  TRANSFORM_HANDLE_NONE,
  TRANSFORM_HANDLE_NW, /* north west */
  TRANSFORM_HANDLE_NE, /* north east */
  TRANSFORM_HANDLE_SW, /* south west */
  TRANSFORM_HANDLE_SE, /* south east */
  TRANSFORM_HANDLE_N,  /* north      */
  TRANSFORM_HANDLE_S,  /* south      */
  TRANSFORM_HANDLE_E,  /* east       */
  TRANSFORM_HANDLE_W,  /* west       */
  TRANSFORM_HANDLE_CENTER
} TransformAction;

typedef gdouble TransInfo[TRANS_INFO_SIZE];


#define GIMP_TYPE_TRANSFORM_TOOL            (gimp_transform_tool_get_type ())
#define GIMP_TRANSFORM_TOOL(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_TRANSFORM_TOOL, GimpTransformTool))
#define GIMP_TRANSFORM_TOOL_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_TRANSFORM_TOOL, GimpTransformToolClass))
#define GIMP_IS_TRANSFORM_TOOL(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_TRANSFORM_TOOL))
#define GIMP_IS_TRANSFORM_TOOL_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_TRANSFORM_TOOL))
#define GIMP_TRANSFORM_TOOL_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_TRANSFORM_TOOL, GimpTransformToolClass))

#define GIMP_TRANSFORM_TOOL_GET_OPTIONS(t)  (GIMP_TRANSFORM_OPTIONS (gimp_tool_get_options (GIMP_TOOL (t))))


typedef struct _GimpTransformToolClass GimpTransformToolClass;

struct _GimpTransformTool
{
  GimpDrawTool    parent_instance;

  gdouble         curx;            /*  current x coord                   */
  gdouble         cury;            /*  current y coord                   */

  gdouble         lastx;           /*  last x coord                      */
  gdouble         lasty;           /*  last y coord                      */

  gint            x1, y1;          /*  upper left hand coordinate        */
  gint            x2, y2;          /*  lower right hand coords           */
  gdouble         cx, cy;          /*  center point (for rotation)       */
  gdouble         aspect;          /*  original aspect ratio             */

  gdouble         tx1, ty1;        /*  transformed handle coords         */
  gdouble         tx2, ty2;
  gdouble         tx3, ty3;
  gdouble         tx4, ty4;
  gdouble         tcx, tcy;

  GimpMatrix3     transform;       /*  transformation matrix             */
  TransInfo       trans_info;      /*  transformation info               */
  TransInfo       old_trans_info;  /*  for resetting everything          */
  TransInfo       prev_trans_info; /*  for cancelling a drag operation   */

  TransformAction function;        /*  current tool activity             */

  gboolean        use_grid;        /*  does the tool use the grid        */
  gboolean        use_handles;     /*  uses the corner handles           */
  gboolean        use_center;      /*  uses the center handle            */
  gboolean        use_mid_handles; /*  use handles at midpoints of edges */

  GimpCanvasItem *handles[TRANSFORM_HANDLE_CENTER + 1];

  const gchar    *progress_text;

  GtkWidget      *dialog;
};

struct _GimpTransformToolClass
{
  GimpDrawToolClass  parent_class;

  /*  virtual functions  */
  void          (* dialog)        (GimpTransformTool *tool);
  void          (* dialog_update) (GimpTransformTool *tool);
  void          (* prepare)       (GimpTransformTool *tool);
  void          (* motion)        (GimpTransformTool *tool);
  void          (* recalc_matrix) (GimpTransformTool *tool);
  gchar       * (* get_undo_desc) (GimpTransformTool *tool);
  TileManager * (* transform)     (GimpTransformTool *tool,
                                   GimpItem          *item,
                                   TileManager       *orig_tiles,
                                   gint               orig_offset_x,
                                   gint               orig_offset_y,
                                   gint              *new_offset_x,
                                   gint              *new_offset_y);
};


GType   gimp_transform_tool_get_type      (void) G_GNUC_CONST;

void    gimp_transform_tool_recalc_matrix (GimpTransformTool *tr_tool);


#endif  /*  __GIMP_TRANSFORM_TOOL_H__  */
