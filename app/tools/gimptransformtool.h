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

enum BoundingBox
{
  X0, Y0, X1, Y1, X2, Y2, X3, Y3
};

typedef gdouble TranInfo[TRAN_INFO_SIZE];


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

  gint            startx;       /*  starting x coord            */
  gint            starty;       /*  starting y coord            */

  gint            curx;         /*  current x coord             */
  gint            cury;         /*  current y coord             */

  gint            lastx;        /*  last x coord                */
  gint            lasty;        /*  last y coord                */

  gint            state;        /*  state of buttons and keys   */

  gint            x1, y1;       /*  upper left hand coordinate  */
  gint            x2, y2;       /*  lower right hand coords     */
  gint		  cx, cy;	/*  center point (for rotation) */

  gdouble         tx1, ty1;     /*  transformed coords          */
  gdouble         tx2, ty2;     /*                              */
  gdouble         tx3, ty3;     /*                              */
  gdouble         tx4, ty4;     /*                              */
  gdouble	  tcx, tcy;	/*                              */

  gint            sx1, sy1;     /*  transformed screen coords   */
  gint            sx2, sy2;     /*  position of four handles    */
  gint            sx3, sy3;     /*                              */
  gint            sx4, sy4;     /*                              */
  gint            scx, scy;     /*  and center for rotation     */

  GimpMatrix3     transform;    /*  transformation matrix       */
  TranInfo        trans_info;   /*  transformation info         */

  TileManager    *original;     /*  pointer to original tiles   */

  TransformAction function;     /*  current tool activity       */

  gboolean        interactive;  /*  tool is interactive         */
  gboolean        bpressed;     /*  Bug work around make sure we have 
				 *  a button pressed before we deal with
				 *  motion events. ALT.
				 */
  gint		  ngx, ngy;	/*  number of grid lines in original
				 *  x and y directions
				 */
  gdouble	 *grid_coords;	/*  x and y coordinates of the grid
				 *  endpoints (a total of (ngx+ngy)*2
				 *  coordinate pairs)
				 */
  gdouble	 *tgrid_coords; /*  transformed grid_coords     */
};

struct _GimpTransformToolClass
{
  GimpDrawToolClass parent_class;

  TileManager * (* transform) (GimpTransformTool    *tool,
		               GimpDisplay          *gdisp,
		               TransformState        state);

};

/*  Special undo type  */
typedef struct _TransformUndo TransformUndo;

struct _TransformUndo
{
  gint         tool_ID;
  GType        tool_type;

  TranInfo     trans_info;
  TileManager *original;
  gpointer     path_undo;
};

/*  transform directions  */
#define TRANSFORM_TRADITIONAL 0
#define TRANSFORM_CORRECTIVE  1


/*  make this variable available to all  */
/*  Do we still need to do this?  -- rock */
extern InfoDialog * transform_info;

/*  transform tool functions  */
/* make PDB compile: ToolType doesn't exist any more 
Tool        * gimp_transform_tool_new                    (GimpTransformToolType        tool_type,
						     gboolean        interactive);
*/

GType         gimp_transform_tool_get_type               (void);

void          gimp_transform_tool_transform_bounding_box (GimpTransformTool    *tool);
void          gimp_transform_tool_reset                  (GimpTransformTool    *tool,
                                                          GimpDisplay          *gdisp);
void	      gimp_transform_tool_grid_density_changed   (void);
void	      gimp_transform_tool_showpath_changed       (gint                  type);
TileManager * gimp_transform_tool_transform              (GimpTransformTool    *tool,
		                                          GimpDisplay          *gdisp,
		                                          TransformState        state);
/*  transform functions  */
/* FIXME this function needs to be renamed */
TileManager * gimp_transform_tool_do                     (GimpImage        *gimage,
							  GimpDrawable     *drawable,
							  TileManager      *float_tiles,
							  gboolean          interpolation,
							  GimpMatrix3       matrix,
							  GimpProgressFunc  progress_callback,
							  gpointer          progress_data);
TileManager * gimp_transform_tool_cut                    (GimpImage        *gimage,
							  GimpDrawable     *drawable,
							  gboolean         *new_layer);
gboolean      gimp_transform_tool_paste                  (GimpImage        *gimage,
							  GimpDrawable     *drawable,
							  TileManager      *tiles,
							  gboolean          new_layer);

gboolean      gimp_transform_tool_smoothing              (void);
gboolean      gimp_transform_tool_showpath               (void);
gboolean      gimp_transform_tool_clip	                 (void);
gint	      gimp_transform_tool_direction              (void);
gint	      gimp_transform_tool_grid_size              (void);
gboolean      gimp_transform_tool_show_grid              (void);


#endif  /*  __GIMP_TRANSFORM_TOOL_H__  */
