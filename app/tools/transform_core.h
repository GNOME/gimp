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

#include "info_dialog.h"
#include "draw_core.h"
#include "temp_buf.h"
#include "libgimp/gimpmatrix.h"
#include "gimpprogress.h"

/* possible scaling functions */
#define CREATING        0
#define HANDLE_1        1
#define HANDLE_2        2
#define HANDLE_3        3
#define HANDLE_4        4
#define HANDLE_CENTER	5

/* the different states that the transformation function can be called with  */
#define INIT            0
#define MOTION          1
#define RECALC          2
#define FINISH          3

/* buffer sizes for scaling information strings (for the info dialog) */
#define MAX_INFO_BUF   40
#define TRAN_INFO_SIZE  8

/* control whether the transform tool draws a bounding box */
#define NON_INTERACTIVE 0
#define INTERACTIVE     1


typedef double  TranInfo[TRAN_INFO_SIZE];

typedef void * (* TransformFunc)   (Tool *, void *, int);
typedef struct _transform_core TransformCore;

struct _transform_core
{
  DrawCore *      core;         /*  Core select object          */

  int             startx;       /*  starting x coord            */
  int             starty;       /*  starting y coord            */

  int             curx;         /*  current x coord             */
  int             cury;         /*  current y coord             */

  int             lastx;        /*  last x coord                */
  int             lasty;        /*  last y coord                */

  int             state;        /*  state of buttons and keys   */

  int             x1, y1;       /*  upper left hand coordinate  */
  int             x2, y2;       /*  lower right hand coords     */
  int		  cx, cy;	/*  center point (for rotation) */

  double          tx1, ty1;     /*  transformed coords          */
  double          tx2, ty2;     /*                              */
  double          tx3, ty3;     /*                              */
  double          tx4, ty4;     /*                              */
  double	  tcx, tcy;	/*                              */

  int             sx1, sy1;     /*  transformed screen coords   */
  int             sx2, sy2;     /*  position of four handles    */
  int             sx3, sy3;     /*                              */
  int             sx4, sy4;     /*                              */
  int             scx, scy;     /*  and center for rotation     */

  GimpMatrix      transform;    /*  transformation matrix       */
  TranInfo        trans_info;   /*  transformation info         */

  TileManager *   original;     /*  pointer to original tiles   */

  TransformFunc   trans_func;   /*  transformation function     */

  int             function;     /*  current tool activity       */

  int             interactive;  /*  tool is interactive         */
  int             bpressed;     /* Bug work around make sure we have 
				 * a button pressed before we deal with
				 * motion events. ALT.
				 */
  int		  ngx, ngy;	/*  number of grid lines in original
				    x and y directions  */
  double	  *grid_coords;	/*  x and y coordinates of the grid
				    endpoints (a total of (ngx+ngy)*2
				    coordinate pairs)  */
  double	  *tgrid_coords; /* transformed grid_coords  */
};


/*  Special undo type  */
typedef struct _transform_undo TransformUndo;

struct _transform_undo
{
  int             tool_ID;
  int             tool_type;
  TranInfo        trans_info;
  TileManager *   original;
};


/*  make this variable available to all  */
extern        InfoDialog * transform_info;

/*  transform tool action functions  */
void          transform_core_button_press      (Tool *, GdkEventButton *, gpointer);
void          transform_core_button_release    (Tool *, GdkEventButton *, gpointer);
void          transform_core_motion            (Tool *, GdkEventMotion *, gpointer);
void          transform_core_cursor_update     (Tool *, GdkEventMotion *, gpointer);
void          transform_core_control           (Tool *, int, gpointer);

/*  transform tool functions  */
void          transform_core_draw         (Tool *);
void          transform_core_no_draw      (Tool *);
Tool *        transform_core_new          (int, int);
void          transform_core_free         (Tool *);
void          transform_core_reset        (Tool *, void *);
void	      transform_core_grid_density_changed (void);

/*  transform functions  */
TileManager * transform_core_do           (GImage *, GimpDrawable *, TileManager *, int, GimpMatrix, progress_func_t, gpointer);
TileManager * transform_core_cut          (GImage *, GimpDrawable *, int *);
Layer *       transform_core_paste        (GImage *, GimpDrawable *, TileManager *, int);
void          transform_bounding_box (Tool*);


#endif  /*  __TRANSFORM_CORE_H__  */
