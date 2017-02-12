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


typedef enum
{
  TRANSFORM_CREATING,
  TRANSFORM_HANDLE_NONE,
  TRANSFORM_HANDLE_NW_P, /* perspective handles */
  TRANSFORM_HANDLE_NE_P,
  TRANSFORM_HANDLE_SW_P,
  TRANSFORM_HANDLE_SE_P,
  TRANSFORM_HANDLE_NW, /* north west */
  TRANSFORM_HANDLE_NE, /* north east */
  TRANSFORM_HANDLE_SW, /* south west */
  TRANSFORM_HANDLE_SE, /* south east */
  TRANSFORM_HANDLE_N,  /* north      */
  TRANSFORM_HANDLE_S,  /* south      */
  TRANSFORM_HANDLE_E,  /* east       */
  TRANSFORM_HANDLE_W,  /* west       */
  TRANSFORM_HANDLE_CENTER, /* for moving */
  TRANSFORM_HANDLE_PIVOT,  /* pivot for rotation and scaling */
  TRANSFORM_HANDLE_N_S,  /* shearing handles */
  TRANSFORM_HANDLE_S_S,
  TRANSFORM_HANDLE_E_S,
  TRANSFORM_HANDLE_W_S,
  TRANSFORM_HANDLE_ROTATION, /* rotation handle */

  TRANSFORM_HANDLE_NUM /* keep this last so *handles[] is the right size */
} TransformAction;


/* This is not the number of items in the enum above, but the max size
 * of the enums at the top of each transformation tool, stored in
 * trans_info and related
 */
#define TRANS_INFO_SIZE 17

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

  gdouble         curx;               /*  current x coord                    */
  gdouble         cury;               /*  current y coord                    */

  gdouble         lastx;              /*  last x coord                       */
  gdouble         lasty;              /*  last y coord                       */

  gdouble         previousx;          /*  previous x coord                   */
  gdouble         previousy;          /*  previous y coord                   */

  gdouble         mousex;             /*  x coord where mouse was clicked    */
  gdouble         mousey;             /*  y coord where mouse was clicked    */

  gint            x1, y1;             /*  upper left hand coordinate         */
  gint            x2, y2;             /*  lower right hand coords            */
  gdouble         cx, cy;             /*  center point (for moving)          */
  gdouble         px, py;             /*  pivot point (for rotation/scaling) */
  gdouble         aspect;             /*  original aspect ratio              */

  gdouble         tx1, ty1;           /*  transformed handle coords          */
  gdouble         tx2, ty2;
  gdouble         tx3, ty3;
  gdouble         tx4, ty4;
  gdouble         tcx, tcy;
  gdouble         tpx, tpy;

  GimpMatrix3     transform;          /*  transformation matrix              */
  TransInfo       trans_info;         /*  transformation info                */
  TransInfo      *old_trans_info;     /*  for resetting everything           */
  TransInfo      *prev_trans_info;    /*  the current finished state         */
  GList          *undo_list;          /*  list of all states,
                                          head is current == prev_trans_info,
                                          tail is original == old_trans_info */
  GList          *redo_list;          /*  list of all undone states,
                                          NULL when nothing undone */

  TransformAction function;           /*  current tool activity              */

  gboolean        use_grid;           /*  does the tool use the grid         */
  gboolean        use_corner_handles; /*  uses the corner handles            */
  gboolean        use_side_handles;   /*  use handles at midpoints of edges  */
  gboolean        use_center_handle;  /*  uses the center handle             */
  gboolean        use_pivot_handle;   /*  use the pivot point handle         */

  gboolean        does_perspective;   /*  does the tool do non-affine
                                          transformations                    */

  GimpCanvasItem *handles[TRANSFORM_HANDLE_NUM];

  const gchar    *progress_text;

  GimpToolGui    *gui;
};

struct _GimpTransformToolClass
{
  GimpDrawToolClass  parent_class;

  /*  virtual functions  */
  void            (* dialog)        (GimpTransformTool *tool);
  void            (* dialog_update) (GimpTransformTool *tool);
  void            (* prepare)       (GimpTransformTool *tool);
  void            (* motion)        (GimpTransformTool *tool);
  void            (* recalc_matrix) (GimpTransformTool *tool);
  gchar         * (* get_undo_desc) (GimpTransformTool *tool);
  TransformAction (* pick_function) (GimpTransformTool *tool,
                                     const GimpCoords  *coords,
                                     GdkModifierType    state,
                                     GimpDisplay       *display);
  void            (* cursor_update) (GimpTransformTool  *tr_tool,
                                     GimpCursorType     *cursor,
                                     GimpCursorModifier *modifier);
  void            (* draw_gui)      (GimpTransformTool *tool,
                                     gint               handle_w,
                                     gint               handle_h);
  GeglBuffer    * (* transform)     (GimpTransformTool *tool,
                                     GimpItem          *item,
                                     GeglBuffer        *orig_buffer,
                                     gint               orig_offset_x,
                                     gint               orig_offset_y,
                                     GimpColorProfile **buffer_profile,
                                     gint              *new_offset_x,
                                     gint              *new_offset_y);

  const gchar *ok_button_label;
};


GType   gimp_transform_tool_get_type           (void) G_GNUC_CONST;

void    gimp_transform_tool_recalc_matrix      (GimpTransformTool *tr_tool);
void    gimp_transform_tool_push_internal_undo (GimpTransformTool *tr_tool);


#endif  /*  __GIMP_TRANSFORM_TOOL_H__  */
