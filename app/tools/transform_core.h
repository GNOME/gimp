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
#ifndef __TRANSFORM_CORE_H__
#define __TRANSFORM_CORE_H__

#include "draw_core.h"
#include "gimpprogress.h"
#include "info_dialog.h"

#include "libgimp/gimpmatrix.h"

/* possible transform functions */
typedef enum
{
  CREATING,
  HANDLE_1,
  HANDLE_2,
  HANDLE_3,
  HANDLE_4,
  HANDLE_CENTER
} TransformAction;

/* the different states that the transformation function can be called with */
typedef enum
{
  INIT,
  MOTION,
  RECALC,
  FINISH
} TransformState;

/* buffer sizes for scaling information strings (for the info dialog) */
#define MAX_INFO_BUF   40
#define TRAN_INFO_SIZE  8

enum BoundingBox
{
  X0, Y0, X1, Y1, X2, Y2, X3, Y3
};

typedef gdouble TranInfo[TRAN_INFO_SIZE];

typedef TileManager * (* TransformFunc) (Tool *, void *, TransformState);


typedef struct _TransformCore TransformCore;

struct _TransformCore
{
  DrawCore       *core;         /*  Core select object          */

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

  TransformFunc   trans_func;   /*  transformation function     */

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


/*  Special undo type  */
typedef struct _TransformUndo TransformUndo;

struct _TransformUndo
{
  gint         tool_ID;
  gint         tool_type;
  TranInfo     trans_info;
  TileManager *original;
  void        *path_undo;
};


/*  make this variable available to all  */
extern InfoDialog * transform_info;

/*  transform tool action functions  */
void          transform_core_button_press   (Tool *, GdkEventButton *, gpointer);
void          transform_core_button_release (Tool *, GdkEventButton *, gpointer);
void          transform_core_motion         (Tool *, GdkEventMotion *, gpointer);
void          transform_core_cursor_update  (Tool *, GdkEventMotion *, gpointer);
void          transform_core_control        (Tool *, ToolAction,       gpointer);

/*  transform tool functions  */
Tool        * transform_core_new                    (ToolType  tool_type,
						     gboolean  interactive);
void          transform_core_free                   (Tool     *tool);
void          transform_core_draw                   (Tool     *tool);
void          transform_core_no_draw                (Tool     *tool);
void          transform_core_transform_bounding_box (Tool     *tool);
void          transform_core_reset                  (Tool     *tool,
						     void     *gdisp_ptr);
void	      transform_core_grid_density_changed   (void);
void	      transform_core_showpath_changed       (gint      type);

/*  transform functions  */
TileManager * transform_core_do    (GImage          *gimage,
				    GimpDrawable    *drawable,
				    TileManager     *float_tiles,
				    gboolean         interpolation,
				    GimpMatrix3      matrix,
				    progress_func_t  progress_callback,
				    gpointer         progress_data);
TileManager * transform_core_cut   (GImage          *gimage,
				    GimpDrawable    *drawable,
				    gboolean        *new_layer);
gboolean      transform_core_paste (GImage          *gimage,
				    GimpDrawable    *drawable,
				    TileManager     *tiles,
				    gboolean         new_layer);

#endif  /*  __TRANSFORM_CORE_H__  */
