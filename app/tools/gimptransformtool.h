/* The GIMP -- an image manipulation program
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


/* FIXME */
#include "gui/gui-types.h"


/* buffer sizes for scaling information strings (for the info dialog) */
#define MAX_INFO_BUF   40
#define TRAN_INFO_SIZE  8


typedef gdouble TransInfo[TRAN_INFO_SIZE];


#define GIMP_TYPE_TRANSFORM_TOOL            (gimp_transform_tool_get_type ())
#define GIMP_TRANSFORM_TOOL(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_TRANSFORM_TOOL, GimpTransformTool))
#define GIMP_TRANSFORM_TOOL_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_TRANSFORM_TOOL, GimpTransformToolClass))
#define GIMP_IS_TRANSFORM_TOOL(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_TRANSFORM_TOOL))
#define GIMP_IS_TRANSFORM_TOOL_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_TRANSFORM_TOOL))
#define GIMP_TRANSFORM_TOOL_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_TRANSFORM_TOOL, GimpTransformToolClass))


typedef struct _GimpTransformToolClass GimpTransformToolClass;

struct _GimpTransformTool
{
  GimpDrawTool    parent_instance;

  gint            startx;         /*  starting x coord                 */
  gint            starty;         /*  starting y coord                 */

  gint            curx;           /*  current x coord                  */
  gint            cury;           /*  current y coord                  */

  gint            lastx;          /*  last x coord                     */
  gint            lasty;          /*  last y coord                     */

  gint            state;          /*  state of buttons and keys        */

  gint            x1, y1;         /*  upper left hand coordinate       */
  gint            x2, y2;         /*  lower right hand coords          */
  gint		  cx, cy;	  /*  center point (for rotation)      */

  gdouble         tx1, ty1;       /*  transformed coords               */
  gdouble         tx2, ty2;
  gdouble         tx3, ty3;
  gdouble         tx4, ty4;
  gdouble	  tcx, tcy;

  GimpMatrix3     transform;      /*  transformation matrix            */
  TransInfo       trans_info;     /*  transformation info              */

  TransInfo       old_trans_info; /*  for cancelling a drag operation  */

  TileManager    *original;       /*  pointer to original tiles        */

  TransformAction function;       /*  current tool activity            */

  gboolean        use_grid;       /*  does the tool use the grid       */

  gint		  ngx, ngy;	  /*  number of grid lines in original
                                   *  x and y directions
                                   */
  gdouble	 *grid_coords;	  /*  x and y coordinates of the grid
                                   *  endpoints (a total of (ngx+ngy)*2
                                   *  coordinate pairs)
                                   */
  gdouble	 *tgrid_coords;   /*  transformed grid_coords          */

  InfoDialog     *info_dialog;    /*  transform info dialog            */
};

struct _GimpTransformToolClass
{
  GimpDrawToolClass parent_class;

  /*  virtual function  */

  TileManager * (* transform) (GimpTransformTool    *tool,
		               GimpDisplay          *gdisp,
		               TransformState        state);

};

typedef struct _TransformUndo TransformUndo;

struct _TransformUndo
{
  gint         tool_ID;
  GType        tool_type;

  TransInfo    trans_info;
  TileManager *original;
  gpointer     path_undo;
};


GType   gimp_transform_tool_get_type               (void);

TileManager * gimp_transform_tool_transform_tiles  (GimpTransformTool *tr_tool,
                                                    const gchar       *progress_text);
void    gimp_transform_tool_transform_bounding_box (GimpTransformTool *tr_tool);

void    gimp_transform_tool_info_dialog_connect    (GimpTransformTool *tr_tool,
                                                    const gchar       *ok_button);

void	gimp_transform_tool_grid_density_changed   (GimpTransformTool *tr_tool);
void	gimp_transform_tool_show_path_changed      (GimpTransformTool *tr_tool,
                                                    gint               type);


#endif  /*  __GIMP_TRANSFORM_TOOL_H__  */
