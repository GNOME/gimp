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
#ifndef __PAINT_CORE_H__
#define __PAINT_CORE_H__

#include "draw_core.h"
#include "temp_buf.h"
#include "gimpbrush.h"
#include "gimpdrawableF.h"

/* the different states that the painting function can be called with  */
#define INIT_PAINT      0
#define MOTION_PAINT    1
#define PAUSE_PAINT     2
#define RESUME_PAINT    3
#define FINISH_PAINT    4

/* brush application types  */
typedef enum
{
  HARD,     /* pencil */
  SOFT,     /* paintbrush */
  PRESSURE  /* paintbrush with variable pressure */
} BrushApplicationMode;

/* paint application modes  */
typedef enum
{
  CONSTANT,    /*< nick=CONTINUOUS >*/ /* pencil, paintbrush, airbrush, clone */
  INCREMENTAL  /* convolve, smudge */
} PaintApplicationMode;

/* gradient paint modes */
typedef enum {
  ONCE_FORWARD,    /* paint through once, then stop */
  ONCE_BACKWARDS,  /* paint once, then stop, but run the gradient the other way */
  LOOP_SAWTOOTH,   /* keep painting, looping through the grad start->end,start->end /|/|/| */
  LOOP_TRIANGLE,   /* keep paiting, looping though the grad start->end,end->start /\/\/\/  */
  ONCE_END_COLOR,  /* paint once, but keep painting with the end color */
} GradientPaintMode;

typedef struct _paint_core PaintCore;
typedef void * (* PaintFunc)   (PaintCore *, GimpDrawable *, int);
struct _paint_core
{
  DrawCore *      core;         /*  Core select object          */

  double          startx;       /*  starting x coord            */
  double          starty;       /*  starting y coord            */
  double          startpressure;  /* starting pressure          */
  double          startxtilt;    /* starting xtilt              */
  double          startytilt;  /* starting ytilt                */

  double          curx;         /*  current x coord             */
  double          cury;         /*  current y coord             */
  double          curpressure;   /*  current pressure            */
  double          curxtilt;     /*  current xtilt               */
  double          curytilt;     /*  current ytilt               */

  double          lastx;        /*  last x coord                */
  double          lasty;        /*  last y coord                */
  double          lastpressure; /* last pressure               */
  double          lastxtilt;    /* last xtilt                  */
  double          lastytilt;    /* last ytilt                  */

  int             state;        /*  state of buttons and keys   */

  double          distance;     /*  distance traveled by brush  */
  double          spacing;      /*  distance traveled by brush  */

  int             x1, y1;       /*  image space coordinate      */
  int             x2, y2;       /*  image space coords          */

  GimpBrush *     brush;        /*  current brush	        */

  PaintFunc       paint_func;   /*  painting function           */
};

extern PaintCore  non_gui_paint_core;

/*  Special undo type  */
typedef struct _paint_undo PaintUndo;

struct _paint_undo
{
  int             tool_ID;
  double          lastx;
  double          lasty;
  double	  lastpressure;
  double          lastxtilt;
  double          lastytilt;
};

/*  paint tool action functions  */
void          paint_core_button_press      (Tool *, GdkEventButton *, gpointer);
void          paint_core_button_release    (Tool *, GdkEventButton *, gpointer);
void          paint_core_motion            (Tool *, GdkEventMotion *, gpointer);
void          paint_core_cursor_update     (Tool *, GdkEventMotion *, gpointer);
void          paint_core_control           (Tool *, int, gpointer);

/*  paint tool functions  */
void          paint_core_no_draw      (Tool *);
Tool *        paint_core_new          (int);
void          paint_core_free         (Tool *);
int           paint_core_init         (PaintCore *, GimpDrawable *, double, double);
void          paint_core_interpolate  (PaintCore *, GimpDrawable *);
void          paint_core_get_color_from_gradient (PaintCore *, double, double*, double*, double*,double *,int);
void          paint_core_finish       (PaintCore *, GimpDrawable *, int);
void          paint_core_cleanup      (void);

/*  paint tool painting functions  */
TempBuf *     paint_core_get_paint_area    (PaintCore *, GimpDrawable *);
TempBuf *     paint_core_get_orig_image    (PaintCore *, GimpDrawable *, int, int, int, int);
void          paint_core_paste_canvas      (PaintCore *, GimpDrawable *, int, int, int, int, int);
void          paint_core_replace_canvas    (PaintCore *, GimpDrawable *, int, int, int, int);

#endif  /*  __PAINT_CORE_H__  */
