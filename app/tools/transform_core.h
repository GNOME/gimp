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

struct _Canvas;

#include "info_dialog.h"
#include "draw_core.h"

/* possible scaling functions */
#define CREATING        0
#define HANDLE_1        1
#define HANDLE_2        2
#define HANDLE_3        3
#define HANDLE_4        4

/* the different states that the transformation function can be called with  */
#define INIT            0
#define MOTION          1
#define RECALC          2
#define FINISH          3

/* buffer sizes for scaling information strings (for the info dialog) */
#define MAX_INFO_BUF    12
#define TRAN_INFO_SIZE  8

/* control whether the transform tool draws a bounding box */
#define NON_INTERACTIVE 0
#define INTERACTIVE     1


typedef double  Vector[3];
typedef Vector  Matrix[3];
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

  double          tx1, ty1;     /*  transformed coords          */
  double          tx2, ty2;     /*                              */
  double          tx3, ty3;     /*                              */
  double          tx4, ty4;     /*                              */

  int             sx1, sy1;     /*  transformed screen coords   */
  int             sx2, sy2;     /*  position of four handles    */
  int             sx3, sy3;     /*                              */
  int             sx4, sy4;     /*                              */

  Matrix          transform;    /*  transformation matrix       */
  TranInfo        trans_info;   /*  transformation info         */

  struct _Canvas *original;     /*  pointer to original tiles   */

  TransformFunc   trans_func;   /*  transformation function     */

  int             function;     /*  current tool activity       */

  int             interactive;  /*  tool is interactive         */
  int             bpressed;     /* Bug work around make sure we have 
				 * a button pressed before we deal with
				 * motion events. ALT.
				 */
};


/*  Special undo type  */
typedef struct _transform_undo TransformUndo;

struct _transform_undo
{
  int             tool_ID;
  int             tool_type;
  TranInfo        trans_info;
  int             first;
  struct _Canvas *original;
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

/*  transform functions  */
struct _Canvas * transform_core_do           (GImage *, GimpDrawable *, struct _Canvas *, int, Matrix);
struct _Canvas * transform_core_cut          (GImage *, GimpDrawable *, int *);
Layer *       transform_core_paste        (GImage *, GimpDrawable *, struct _Canvas *, int);

/*  matrix functions  */
void          transform_bounding_box      (Tool *);
void          transform_point             (Matrix, double, double, double *, double *);
void          mult_matrix                 (Matrix, Matrix);
void          identity_matrix             (Matrix);
void          translate_matrix            (Matrix, double, double);
void          scale_matrix                (Matrix, double, double);
void          rotate_matrix               (Matrix, double);
void          xshear_matrix               (Matrix, double);
void          yshear_matrix               (Matrix, double);


#endif  /*  __TRANSFORM_CORE_H__  */
