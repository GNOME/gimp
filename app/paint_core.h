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

#include "apptypes.h"
#include "draw_core.h"
#include "temp_buf.h"
#include "gimpbrush.h"
#include "gimpdrawableF.h"

/* the different states that the painting function can be called with  */

#define INIT_PAINT       0 /* Setup PaintFunc internals */ 
#define MOTION_PAINT     1 /* PaintFunc performs motion-related rendering */
#define PAUSE_PAINT      2 /* Unused. Reserved */
#define RESUME_PAINT     3 /* Unused. Reserved */
#define FINISH_PAINT     4 /* Cleanup and/or reset PaintFunc operation */
#define PRETRACE_PAINT   5 /* PaintFunc performs window tracing activity prior to rendering */
#define POSTTRACE_PAINT  6 /* PaintFunc performs window tracing activity following rendering */

typedef enum /*< skip >*/
{
  TOOL_CAN_HANDLE_CHANGING_BRUSH = 0x0001, /* Set for tools that don't mind
					    * if the brush changes while
					    * painting.
					    */

  TOOL_TRACES_ON_WINDOW                    /* Set for tools that perform temporary
                                            * rendering directly to the window. These
                                            * require sequencing with gdisplay_flush()
                                            * routines. See clone.c for example.
                                            */
} ToolFlags;

typedef void * (* PaintFunc)   (PaintCore *, GimpDrawable *, int);
struct _paint_core
{
  DrawCore *      core;          /*  Core select object         */

  double          startx;        /*  starting x coord           */
  double          starty;        /*  starting y coord           */
  double          startpressure; /*  starting pressure          */
  double          startxtilt;    /*  starting xtilt             */
  double          startytilt;    /*  starting ytilt             */
#ifdef GTK_HAVE_SIX_VALUATORS
  double          startwheel;    /*  starting wheel             */ 
#endif /* GTK_HAVE_SIX_VALUATORS */

  double          curx;          /*  current x coord            */
  double          cury;          /*  current y coord            */
  double          curpressure;   /*  current pressure           */
  double          curxtilt;      /*  current xtilt              */
  double          curytilt;      /*  current ytilt              */
#ifdef GTK_HAVE_SIX_VALUATORS
  double          curwheel;      /*  current wheel              */
#endif /* GTK_HAVE_SIX_VALUATORS */

  double          lastx;         /*  last x coord               */
  double          lasty;         /*  last y coord               */
  double          lastpressure;  /*  last pressure              */
  double          lastxtilt;     /*  last xtilt                 */
  double          lastytilt;     /*  last ytilt                 */
#ifdef GTK_HAVE_SIX_VALUATORS
  double          lastwheel;     /*  last wheel                 */ 
#endif /* GTK_HAVE_SIX_VALUATORS */ 

  int             state;         /*  state of buttons and keys  */

  double          distance;      /*  distance traveled by brush */
  double          pixel_dist;    /*  distance in pixels         */
  double          spacing;       /*  spacing                    */

  int             x1, y1;        /*  image space coordinate     */
  int             x2, y2;        /*  image space coords         */

  GimpBrush *     brush;         /*  current brush	        */

  PaintFunc       paint_func;    /*  painting function          */

  int             pick_colors;   /*  pick color if ctrl or alt is pressed  */
  int             pick_state;    /*  was ctrl or alt pressed when clicked? */
  ToolFlags       flags;	 /*  tool flags, see ToolFlags above       */

  guint           context_id;    /*  for the statusbar          */
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
#ifdef GTK_HAVE_SIX_VALUATORS
  double         lastwheel;
#endif /* GTK_HAVE_SIX_VALUATORS */
};

/*  paint tool action functions  */
void          paint_core_button_press    (Tool *tool, GdkEventButton *bevent, gpointer gdisp_ptr);
void          paint_core_button_release  (Tool *tool, GdkEventButton *bevent, gpointer gdisp_ptr);
void          paint_core_motion          (Tool *tool, GdkEventMotion *mevent, gpointer gdisp_ptr);
void          paint_core_cursor_update   (Tool *tool, GdkEventMotion *mevent, gpointer gdisp_ptr);

void          paint_core_control         (Tool                *tool, 
					  ToolAction           action, 
					  gpointer             gdisp_ptr);

/*  paint tool functions  */
void          paint_core_no_draw         (Tool                *tool);
Tool *        paint_core_new             (ToolType             type);
void          paint_core_free            (Tool                *tool);
int           paint_core_init            (PaintCore           *paint_core, 
					  GimpDrawable        *drawable, 
					  gdouble              x, 
					  gdouble              y);
void          paint_core_interpolate     (PaintCore           *paint_core, 
					  GimpDrawable        *drawable);
void          paint_core_finish          (PaintCore           *paint_core, 
					  GimpDrawable        *drawable, 
					  gint                 tool_ID);
void          paint_core_cleanup         (void);

void  paint_core_get_color_from_gradient (PaintCore            *paint_core, 
					  gdouble               gradient_length, 
					  gdouble              *r, 
					  gdouble              *g, 
					  gdouble              *b, 
					  gdouble              *a, 
					  GradientPaintMode     mode);

/*  paint tool painting functions  */
TempBuf *     paint_core_get_paint_area  (PaintCore            *paint_core,
					  GimpDrawable         *drawable,
					  gdouble               scale);
TempBuf *     paint_core_get_orig_image  (PaintCore            *paint_core,
					  GimpDrawable         *drawable,
					  gint                  x1, 
					  gint                  y1, 
					  gint                  x2,
					  gint                  y2);
void          paint_core_paste_canvas    (PaintCore            *paint_core,
					  GimpDrawable         *drawable, 
					  gint                  brush_opacity, 
					  gint                  image_opacity,
					  LayerModeEffects      paint_mode,
					  BrushApplicationMode  brush_hardness,
					  gdouble               brush_scale,
					  PaintApplicationMode  mode);
void          paint_core_replace_canvas  (PaintCore            *paint_core,
					  GimpDrawable         *drawable, 
					  gint                  brush_opacity, 
					  gint                  image_opacity,
					  BrushApplicationMode  brush_hardness,
					  gdouble               brush_scale,
					  PaintApplicationMode  mode);
void   paint_core_color_area_with_pixmap (PaintCore            *paint_core,
					  GimpImage            *dest, 
					  GimpDrawable         *drawable,
					  TempBuf              *area, 
					  gdouble               scale, 
					  BrushApplicationMode  mode);

#endif  /*  __PAINT_CORE_H__  */
