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


/* the different states that the painting function can be called with  */

typedef enum /*< skip >*/
{
  INIT_PAINT,       /* Setup PaintFunc internals */ 
  MOTION_PAINT,     /* PaintFunc performs motion-related rendering */
  PAUSE_PAINT,      /* Unused. Reserved */
  RESUME_PAINT,     /* Unused. Reserved */
  FINISH_PAINT,     /* Cleanup and/or reset PaintFunc operation */
  PRETRACE_PAINT,   /* PaintFunc performs window tracing activity prior to rendering */
  POSTTRACE_PAINT   /* PaintFunc performs window tracing activity following rendering */
} PaintState;

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

typedef gpointer (* PaintFunc) (PaintCore    *paint_core,
				GimpDrawable *drawable,
				PaintState    paint_state);

struct _PaintCore
{
  DrawCore      * core;          /*  Core select object         */

  gdouble         startx;        /*  starting x coord           */
  gdouble         starty;        /*  starting y coord           */
  gdouble         startpressure; /*  starting pressure          */
  gdouble         startxtilt;    /*  starting xtilt             */
  gdouble         startytilt;    /*  starting ytilt             */
#ifdef GTK_HAVE_SIX_VALUATORS
  gdouble         startwheel;    /*  starting wheel             */ 
#endif /* GTK_HAVE_SIX_VALUATORS */

  gdouble         curx;          /*  current x coord            */
  gdouble         cury;          /*  current y coord            */
  gdouble         curpressure;   /*  current pressure           */
  gdouble         curxtilt;      /*  current xtilt              */
  gdouble         curytilt;      /*  current ytilt              */
#ifdef GTK_HAVE_SIX_VALUATORS
  gdouble         curwheel;      /*  current wheel              */
#endif /* GTK_HAVE_SIX_VALUATORS */

  gdouble         lastx;         /*  last x coord               */
  gdouble         lasty;         /*  last y coord               */
  gdouble         lastpressure;  /*  last pressure              */
  gdouble         lastxtilt;     /*  last xtilt                 */
  gdouble         lastytilt;     /*  last ytilt                 */
#ifdef GTK_HAVE_SIX_VALUATORS
  gdouble         lastwheel;     /*  last wheel                 */ 
#endif /* GTK_HAVE_SIX_VALUATORS */ 

  gint            state;         /*  state of buttons and keys  */

  gdouble         distance;      /*  distance traveled by brush */
  gdouble         pixel_dist;    /*  distance in pixels         */
  gdouble         spacing;       /*  spacing                    */

  gint            x1, y1;        /*  image space coordinate     */
  gint            x2, y2;        /*  image space coords         */

  GimpBrush     * brush;         /*  current brush	        */

  PaintFunc       paint_func;    /*  painting function          */

  gboolean        pick_colors;   /*  pick color if ctrl or alt is pressed  */
  gboolean        pick_state;    /*  was ctrl or alt pressed when clicked? */
  ToolFlags       flags;	 /*  tool flags, see ToolFlags above       */

  guint           context_id;    /*  for the statusbar          */
};

extern PaintCore  non_gui_paint_core;

/*  Special undo type  */
typedef struct _PaintUndo PaintUndo;

struct _PaintUndo
{
  gint     tool_ID;
  gdouble  lastx;
  gdouble  lasty;
  gdouble  lastpressure;
  gdouble  lastxtilt;
  gdouble  lastytilt;
#ifdef GTK_HAVE_SIX_VALUATORS
  gdouble  lastwheel;
#endif /* GTK_HAVE_SIX_VALUATORS */
};

/*  paint tool action functions  */
void          paint_core_button_press    (Tool                *tool,
					  GdkEventButton      *bevent,
					  GDisplay            *gdisp);
void          paint_core_button_release  (Tool                *tool,
					  GdkEventButton      *bevent,
					  GDisplay            *gdisp);
void          paint_core_motion          (Tool                *tool,
					  GdkEventMotion      *mevent,
					  GDisplay            *gdisp);
void          paint_core_cursor_update   (Tool                *tool,
					  GdkEventMotion      *mevent,
					  GDisplay            *gdisp);

void          paint_core_control         (Tool                *tool, 
					  ToolAction           action,
					  GDisplay            *gdisp);

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
