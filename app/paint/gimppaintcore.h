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

#ifndef __GIMP_PAINT_TOOL_H__
#define __GIMP_PAINT_TOOL_H__


#include "tools/gimpdrawtool.h"


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


#define GIMP_TYPE_PAINT_TOOL            (gimp_paint_tool_get_type ())
#define GIMP_PAINT_TOOL(obj)            (GTK_CHECK_CAST ((obj), GIMP_TYPE_PAINT_TOOL, GimpPaintTool))
#define GIMP_IS_PAINT_TOOL(obj)         (GTK_CHECK_TYPE ((obj), GIMP_TYPE_PAINT_TOOL))
#define GIMP_PAINT_TOOL_CLASS(klass)    (GTK_CHECK_CLASS_CAST ((klass), GIMP_TYPE_PAINT_TOOL, GimpPaintToolClass))
#define GIMP_IS_PAINT_TOOL_CLASS(klass) (GTK_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_PAINT_TOOL))


typedef struct _GimpPaintToolClass GimpPaintToolClass;

struct _GimpPaintTool
{
  GimpDrawTool    parent_instance;

  gdouble         startx;        /*  starting x coord           */
  gdouble         starty;        /*  starting y coord           */
  gdouble         startpressure; /*  starting pressure          */
  gdouble         startxtilt;    /*  starting xtilt             */
  gdouble         startytilt;    /*  starting ytilt             */

  gdouble         curx;          /*  current x coord            */
  gdouble         cury;          /*  current y coord            */
  gdouble         curpressure;   /*  current pressure           */
  gdouble         curxtilt;      /*  current xtilt              */
  gdouble         curytilt;      /*  current ytilt              */

  gdouble         lastx;         /*  last x coord               */
  gdouble         lasty;         /*  last y coord               */
  gdouble         lastpressure;  /*  last pressure              */
  gdouble         lastxtilt;     /*  last xtilt                 */
  gdouble         lastytilt;     /*  last ytilt                 */

  gint            state;         /*  state of buttons and keys  */

  gdouble         distance;      /*  distance traveled by brush */
  gdouble         pixel_dist;    /*  distance in pixels         */
  gdouble         spacing;       /*  spacing                    */

  gint            x1, y1;        /*  image space coordinate     */
  gint            x2, y2;        /*  image space coords         */

  GimpBrush     * brush;         /*  current brush	        */

  gboolean        pick_colors;   /*  pick color if ctrl or alt is pressed  */
  gboolean        pick_state;    /*  was ctrl or alt pressed when clicked? */
  ToolFlags       flags;	 /*  tool flags, see ToolFlags above       */

  guint           context_id;    /*  for the statusbar          */
};

struct _GimpPaintToolClass
{
  GimpDrawToolClass parent_class;

  void (* paint) (GimpPaintTool *tool,
		  GimpDrawable 	*drawable,
		  PaintState     paint_state);
};


/*  Special undo type  */
typedef struct _PaintUndo PaintUndo;

struct _PaintUndo
{
  gint     tool_ID;
  GtkType  tool_type;

  gdouble  lastx;
  gdouble  lasty;
  gdouble  lastpressure;
  gdouble  lastxtilt;
  gdouble  lastytilt;
};


GtkType	gimp_paint_tool_get_type (void);

void  gimp_paint_tool_paint           (GimpPaintTool       *tool,
				       GimpDrawable        *drawable,
				       PaintState	    state);

void  gimp_paint_tool_no_draw         (GimpPaintTool       *tool);

int   gimp_paint_tool_start           (GimpPaintTool    *tool,
				       GimpDrawable        *drawable,
				       gdouble              x,
				       gdouble              y);
void  gimp_paint_tool_interpolate     (GimpPaintTool    *tool,
				       GimpDrawable        *drawable);
void  gimp_paint_tool_finish          (GimpPaintTool    *tool,
				       GimpDrawable        *drawable);
void  gimp_paint_tool_cleanup         (void);

void  gimp_paint_tool_get_color_from_gradient (GimpPaintTool         *tool,
					       gdouble               gradient_length,
					       GimpRGB              *color,
					       GradientPaintMode     mode);

/*  paint tool painting functions  */
TempBuf * gimp_paint_tool_get_paint_area  (GimpPaintTool     *tool,
					  GimpDrawable         *drawable,
					  gdouble               scale);
TempBuf * gimp_paint_tool_get_orig_image  (GimpPaintTool     *tool,
					  GimpDrawable         *drawable,
					  gint                  x1,
					  gint                  y1,
					  gint                  x2,
					  gint                  y2);
void  gimp_paint_tool_paste_canvas    (GimpPaintTool     *tool,
				       GimpDrawable         *drawable,
				       gint                  brush_opacity,
				       gint                  image_opacity,
				       LayerModeEffects      paint_mode,
				       BrushApplicationMode  brush_hardness,
				       gdouble               brush_scale,
				       PaintApplicationMode  mode);
void  gimp_paint_tool_replace_canvas  (GimpPaintTool     *tool,
				       GimpDrawable         *drawable,
				       gint                  brush_opacity,
				       gint                  image_opacity,
				       BrushApplicationMode  brush_hardness,
				       gdouble               brush_scale,
				       PaintApplicationMode  mode);
void gimp_paint_tool_color_area_with_pixmap (GimpPaintTool     *tool,
					     GimpImage            *dest, 
					     GimpDrawable         *drawable,
					     TempBuf              *area, 
					     gdouble               scale, 
					     BrushApplicationMode  mode);


#endif  /*  __GIMP_PAINT_TOOL_H__  */
