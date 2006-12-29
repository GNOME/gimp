/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995-2001 Spencer Kimball, Peter Mattis, and others
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

#ifndef __GIMP_TRANSFORM_TOOL_H__
#define __GIMP_TRANSFORM_TOOL_H__


#include "gimpdrawtool.h"


#define TRANS_INFO_SIZE  8


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

  gdouble         startx;          /*  starting x coord                  */
  gdouble         starty;          /*  starting y coord                  */

  gdouble         curx;            /*  current x coord                   */
  gdouble         cury;            /*  current y coord                   */

  gdouble         lastx;           /*  last x coord                      */
  gdouble         lasty;           /*  last y coord                      */

  GdkModifierType state;           /*  state of buttons and keys         */

  gint            x1, y1;          /*  upper left hand coordinate        */
  gint            x2, y2;          /*  lower right hand coords           */
  gdouble         cx, cy;          /*  center point (for rotation)       */
  gdouble         aspect;          /*  original aspect ratio             */

  gdouble         tx1, ty1;        /*  transformed coords                */
  gdouble         tx2, ty2;
  gdouble         tx3, ty3;
  gdouble         tx4, ty4;
  gdouble         tcx, tcy;

  GimpMatrix3     transform;       /*  transformation matrix             */
  TransInfo       trans_info;      /*  transformation info               */

  TransInfo       old_trans_info;  /*  for cancelling a drag operation   */

  TileManager    *original;        /*  pointer to original tiles         */

  TransformAction function;        /*  current tool activity             */

  gboolean        use_grid;        /*  does the tool use the grid        */
  gboolean        use_handles;     /*  uses the corner handles           */
  gboolean        use_center;      /*  uses the center handle            */
  gboolean        use_mid_handles; /*  use handles at midpoints of edges */

  gint            handle_w;        /*  handle width                      */
  gint            handle_h;        /*  handle height                     */

  gint            ngx, ngy;        /*  number of grid lines in original
                                    *  x and y directions
                                    */
  gdouble        *grid_coords;     /*  x and y coordinates of the grid
                                    *  endpoints (a total of (ngx+ngy)*2
                                    *  coordinate pairs)
                                    */
  gdouble        *tgrid_coords;    /*  transformed grid_coords           */

  GimpTransformType       type;
  GimpTransformDirection  direction;

  const gchar    *undo_desc;

  const gchar    *shell_desc;
  const gchar    *progress_text;

  GtkWidget      *dialog;
};

struct _GimpTransformToolClass
{
  GimpDrawToolClass parent_class;

  /*  virtual functions  */
  void          (* dialog)        (GimpTransformTool *tool);
  void          (* dialog_update) (GimpTransformTool *tool);
  void          (* prepare)       (GimpTransformTool *tool,
                                   GimpDisplay       *display);
  void          (* motion)        (GimpTransformTool *tool,
                                   GimpDisplay       *display);
  void          (* recalc)        (GimpTransformTool *tool,
                                   GimpDisplay       *display);
  TileManager * (* transform)     (GimpTransformTool *tool,
                                   GimpItem          *item,
                                   gboolean           mask_empty,
                                   GimpDisplay       *display);
};


GType   gimp_transform_tool_get_type       (void) G_GNUC_CONST;

void    gimp_transform_tool_recalc         (GimpTransformTool *tr_tool,
                                            GimpDisplay       *display);
void    gimp_transform_tool_expose_preview (GimpTransformTool *tr_tool);


#endif  /*  __GIMP_TRANSFORM_TOOL_H__  */
