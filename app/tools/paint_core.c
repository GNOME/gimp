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
#include <stdlib.h>
#include <stdio.h>     /* temporary for debuggin */
#include <string.h>
#include <math.h>
#include "appenv.h"
#include "gimpbrushlist.h"
#include "channels_dialog.h"
#include "drawable.h"
#include "errors.h"
#include "gdisplay.h"
#include "gimage_mask.h"
#include "gimprc.h"
#include "gradient.h"  /* for grad_get_color_at() */
#include "layers_dialog.h"
#include "paint_funcs.h"
#include "paint_core.h"
#include "palette.h"
#include "selection.h"
#include "tools.h"
#include "undo.h"
#include "cursorutil.h"

#include "libgimp/gimpintl.h"

#include "tile.h"			/* ick. */

/*  target size  */
#define  TARGET_HEIGHT     15
#define  TARGET_WIDTH      15

#define    SQR(x) ((x) * (x))
#define    EPSILON  0.00001

/*  global variables--for use in the various paint tools  */
PaintCore  non_gui_paint_core;

/*  local function prototypes  */
static MaskBuf * paint_core_subsample_mask  (MaskBuf *, double, double);
static MaskBuf * paint_core_pressurize_mask  (MaskBuf *, double, double, double);
static MaskBuf * paint_core_solidify_mask   (MaskBuf *);
static MaskBuf * paint_core_get_brush_mask  (PaintCore *, int);
static void      paint_core_paste           (PaintCore *, MaskBuf *, GimpDrawable *, int, int, int, int);
static void      paint_core_replace         (PaintCore *, MaskBuf *, GimpDrawable *, int, int, int);
static void      paint_to_canvas_tiles      (PaintCore *, MaskBuf *, int);
static void      paint_to_canvas_buf        (PaintCore *, MaskBuf *, int);
static void      set_undo_tiles             (GimpDrawable *, int, int, int, int);
static void      set_canvas_tiles           (int, int, int, int);
static int paint_core_invalidate_cache(GimpBrush *brush, gpointer *blah);

/***********************************************************************/


/*  undo blocks variables  */
static TileManager *  undo_tiles = NULL;
static TileManager *  canvas_tiles = NULL;


/***********************************************************************/


/*  paint buffers variables  */
static TempBuf *  orig_buf = NULL;
static TempBuf *  canvas_buf = NULL;


/*  brush buffers  */
static MaskBuf *  pressure_brush;
static MaskBuf *  solid_brush;
static MaskBuf *  kernel_brushes[5][5];


/*  paint buffers utility functions  */
static void        free_paint_buffers     (void);


/***********************************************************************/


#define KERNEL_WIDTH   3
#define KERNEL_HEIGHT  3

/*  Brush pixel subsampling kernels  */
static const int subsample[5][5][9] = {
	{
		{ 64, 64, 0, 64, 64, 0, 0, 0, 0, },
		{ 32, 96, 0, 32, 96, 0, 0, 0, 0, },
		{ 0, 128, 0, 0, 128, 0, 0, 0, 0, },
		{ 0, 96, 32, 0, 96, 32, 0, 0, 0, },
		{ 0, 64, 64, 0, 64, 64, 0, 0, 0, },
	},
	{
		{ 32, 32, 0, 96, 96, 0, 0, 0, 0, },
		{ 16, 48, 0, 48, 144, 0, 0, 0, 0, },
		{ 0, 64, 0, 0, 192, 0, 0, 0, 0, },
		{ 0, 48, 16, 0, 144, 48, 0, 0, 0, },
		{ 0, 32, 32, 0, 96, 96, 0, 0, 0, },
	},
	{
		{ 0, 0, 0, 128, 128, 0, 0, 0, 0, },
		{ 0, 0, 0, 64, 192, 0, 0, 0, 0, },
		{ 0, 0, 0, 0, 256, 0, 0, 0, 0, },
		{ 0, 0, 0, 0, 192, 64, 0, 0, 0, },
		{ 0, 0, 0, 0, 128, 128, 0, 0, 0, },
	},
	{
		{ 0, 0, 0, 96, 96, 0, 32, 32, 0, },
		{ 0, 0, 0, 48, 144, 0, 16, 48, 0, },
		{ 0, 0, 0, 0, 192, 0, 0, 64, 0, },
		{ 0, 0, 0, 0, 144, 48, 0, 48, 16, },
		{ 0, 0, 0, 0, 96, 96, 0, 32, 32, },
	},
	{
		{ 0, 0, 0, 64, 64, 0, 64, 64, 0, },
		{ 0, 0, 0, 32, 96, 0, 32, 96, 0, },
		{ 0, 0, 0, 0, 128, 0, 0, 128, 0, },
		{ 0, 0, 0, 0, 96, 32, 0, 96, 32, },
		{ 0, 0, 0, 0, 64, 64, 0, 64, 64, },
	},
};

static void
paint_core_sample_color (GimpDrawable *drawable, int x, int y, int state)
{
  unsigned char *color;
  if ((color = gimp_drawable_get_color_at(drawable, x, y)))
    {
      if ((state & GDK_CONTROL_MASK))
	palette_set_foreground (color[RED_PIX], color[GREEN_PIX],
				color [BLUE_PIX]);
      else
	palette_set_background (color[RED_PIX], color[GREEN_PIX],
				color [BLUE_PIX]);
      g_free(color);
    }
}


void
paint_core_button_press (tool, bevent, gdisp_ptr)
     Tool *tool;
     GdkEventButton *bevent;
     gpointer gdisp_ptr;
{
  PaintCore * paint_core;
  GDisplay * gdisp;
  int draw_line = 0;
  double x, y;
  GimpDrawable *drawable;

  gdisp = (GDisplay *) gdisp_ptr;
  paint_core = (PaintCore *) tool->private;

  gdisplay_untransform_coords_f (gdisp, (double) bevent->x, (double) bevent->y, &x, &y, TRUE);
  drawable = gimage_active_drawable (gdisp->gimage);

  if (! paint_core_init (paint_core, drawable, x, y))
    return;

  paint_core->curpressure = bevent->pressure;
  paint_core->curxtilt = bevent->xtilt;
  paint_core->curytilt = bevent->ytilt;
  paint_core->state = bevent->state;

  /*  if this is a new image, reinit the core vals  */
  if (gdisp_ptr != tool->gdisp_ptr ||
      ! (bevent->state & GDK_SHIFT_MASK))
    {
      /*  initialize some values  */
      paint_core->startx = paint_core->lastx = paint_core->curx = x;
      paint_core->starty = paint_core->lasty = paint_core->cury = y;
      paint_core->startpressure = paint_core->lastpressure = paint_core->curpressure;
      paint_core->startytilt = paint_core->lastytilt = paint_core->curytilt;
      paint_core->startxtilt = paint_core->lastxtilt = paint_core->curxtilt;
    }
  /*  If shift is down and this is not the first paint
   *  stroke, then draw a line from the last coords to the pointer
   */
  else if (bevent->state & GDK_SHIFT_MASK)
    {
      draw_line = 1;
      paint_core->startx = paint_core->lastx;
      paint_core->starty = paint_core->lasty;
      paint_core->startpressure = paint_core->lastpressure;
      paint_core->startxtilt = paint_core->lastxtilt;
      paint_core->startytilt = paint_core->lastytilt;
    }

  tool->state = ACTIVE;
  tool->gdisp_ptr = gdisp_ptr;
  tool->paused_count = 0;

  /*  pause the current selection and grab the pointer  */
  gdisplays_selection_visibility (gdisp->gimage, SelectionPause);

  /* add motion memory if perfectmouse is set */
  if (perfectmouse != 0)
    gdk_pointer_grab (gdisp->canvas->window, FALSE,
		      GDK_BUTTON1_MOTION_MASK | GDK_BUTTON_RELEASE_MASK,
		      NULL, NULL, bevent->time);
  else
    gdk_pointer_grab (gdisp->canvas->window, FALSE,
		      GDK_POINTER_MOTION_HINT_MASK | GDK_BUTTON1_MOTION_MASK |
		      GDK_BUTTON_RELEASE_MASK,
		      NULL, NULL, bevent->time);
  
  /*  Let the specific painting function initialize itself  */
  (* paint_core->paint_func) (paint_core, drawable, INIT_PAINT);

  if (paint_core->pick_colors
      && (bevent->state & (GDK_CONTROL_MASK | GDK_MOD1_MASK)))
  {
    paint_core_sample_color (drawable, x, y, bevent->state);
    paint_core->pick_state = TRUE;
    return;
  }
  else
    paint_core->pick_state = FALSE;

  /*  Paint to the image  */
  if (draw_line)
    {
      paint_core_interpolate (paint_core, drawable);
      paint_core->lastx = paint_core->curx;
      paint_core->lasty = paint_core->cury;
      paint_core->lastpressure = paint_core->curpressure;
      paint_core->lastxtilt = paint_core->curxtilt;
      paint_core->lastytilt = paint_core->curytilt;
      ;
    }
  else
    (* paint_core->paint_func) (paint_core, drawable, MOTION_PAINT);

  gdisplay_flush_now (gdisp);
}

void
paint_core_button_release (tool, bevent, gdisp_ptr)
     Tool *tool;
     GdkEventButton *bevent;
     gpointer gdisp_ptr;
{
  GDisplay * gdisp;
  GImage * gimage;
  PaintCore * paint_core;

  gdisp = (GDisplay *) gdisp_ptr;
  gimage = gdisp->gimage;
  paint_core = (PaintCore *) tool->private;

  /*  resume the current selection and ungrab the pointer  */
  gdisplays_selection_visibility (gdisp->gimage, SelectionResume);

  gdk_pointer_ungrab (bevent->time);
  gdk_flush ();

  /*  Let the specific painting function finish up  */
  (* paint_core->paint_func) (paint_core, gimage_active_drawable (gdisp->gimage), FINISH_PAINT);

  /*  Set tool state to inactive -- no longer painting */
  draw_core_stop (paint_core->core, tool);
  tool->state = INACTIVE;

  paint_core->pick_state = FALSE;

  paint_core_finish (paint_core, gimage_active_drawable (gdisp->gimage), tool->ID);
  gdisplays_flush ();
}

void
paint_core_motion (tool, mevent, gdisp_ptr)
     Tool *tool;
     GdkEventMotion *mevent;
     gpointer gdisp_ptr;
{
  GDisplay * gdisp;
  PaintCore * paint_core;

  gdisp = (GDisplay *) gdisp_ptr;
  paint_core = (PaintCore *) tool->private;

  gdisplay_untransform_coords_f (gdisp, (double) mevent->x, (double) mevent->y,
				 &paint_core->curx, &paint_core->cury, TRUE);

  if (paint_core->pick_state)
  {
    paint_core_sample_color (gimage_active_drawable (gdisp->gimage),
			     paint_core->curx, paint_core->cury, mevent->state);
    return;
  }

  paint_core->curpressure = mevent->pressure;
  paint_core->curxtilt = mevent->xtilt;
  paint_core->curytilt = mevent->ytilt;
  paint_core->state = mevent->state;

  paint_core_interpolate (paint_core, gimage_active_drawable (gdisp->gimage));

  gdisplay_flush_now (gdisp);

  paint_core->lastx = paint_core->curx;
  paint_core->lasty = paint_core->cury;
  paint_core->lastpressure = paint_core->curpressure;
  paint_core->lastxtilt = paint_core->curxtilt;
  paint_core->lastytilt = paint_core->curytilt;
}

void
paint_core_cursor_update (tool, mevent, gdisp_ptr)
     Tool *tool;
     GdkEventMotion *mevent;
     gpointer gdisp_ptr;
{
  GDisplay *gdisp;
  Layer *layer;
  PaintCore * paint_core;
  GdkCursorType ctype = GDK_TOP_LEFT_ARROW;
  int x, y;

  gdisp = (GDisplay *) gdisp_ptr;
  paint_core = (PaintCore *) tool->private;

  /*  undraw the current tool  */
  draw_core_pause (paint_core->core, tool);

  gdisplay_untransform_coords (gdisp, (double) mevent->x, (double) mevent->y,
			       &x, &y, TRUE, FALSE);
 
  if ((layer = gimage_get_active_layer (gdisp->gimage))) 
    {
      int off_x, off_y;
      drawable_offsets (GIMP_DRAWABLE(layer), &off_x, &off_y);

      if (paint_core->pick_colors
	  && (mevent->state & (GDK_CONTROL_MASK | GDK_MOD1_MASK)))
        {
	  ctype = GIMP_COLOR_PICKER_CURSOR;
	}
      else if (x >= off_x && y >= off_y &&
	       x < (off_x + drawable_width (GIMP_DRAWABLE(layer))) &&
	       y < (off_y + drawable_height (GIMP_DRAWABLE(layer))))
	{
	  /*  One more test--is there a selected region?
	   *  if so, is cursor inside?
	   */
	  if (gimage_mask_is_empty (gdisp->gimage))
	    ctype = GDK_PENCIL;
	  else if (gimage_mask_value (gdisp->gimage, x, y))
	    ctype = GDK_PENCIL;
	}

      /* If shift is down and this is not the first paint stroke, draw a line */
      if (gdisp_ptr == tool->gdisp_ptr && mevent->state & GDK_SHIFT_MASK)
	{
	  /*  Get the current coordinates  */
	  gdisplay_untransform_coords_f (gdisp, (double) mevent->x, (double) mevent->y,
					 &paint_core->curx, &paint_core->cury, TRUE);

	  if (paint_core->core->gc == NULL)
	    draw_core_start (paint_core->core, gdisp->canvas->window, tool);
	  else
	    {
	      /* is this a bad hack ? */
	      paint_core->core->paused_count = 0;
	      draw_core_resume (paint_core->core, tool);
	    }
	}
    }

  gdisplay_install_tool_cursor (gdisp, ctype);
}

void
paint_core_control (tool, action, gdisp_ptr)
     Tool * tool;
     int action;
     gpointer gdisp_ptr;
{
  PaintCore * paint_core;
  GDisplay *gdisp;
  GimpDrawable *drawable;

  gdisp = (GDisplay *) gdisp_ptr;
  paint_core = (PaintCore *) tool->private;
  drawable = gimage_active_drawable (gdisp->gimage);

  switch (action)
    {
    case PAUSE :
      draw_core_pause (paint_core->core, tool);
      break;
    case RESUME :
      draw_core_resume (paint_core->core, tool);
      break;
    case HALT :
      (* paint_core->paint_func) (paint_core, drawable, FINISH_PAINT);
      draw_core_stop (paint_core->core, tool);
      paint_core_cleanup ();
      break;
    }
}

void
paint_core_draw (tool)
     Tool * tool;
{
  GDisplay *gdisp;
  PaintCore * paint_core;
  int tx1, ty1, tx2, ty2;

  paint_core = (PaintCore *) tool->private;

  /* if shift was never used, we don't care about a redraw */
  if (paint_core->core->gc != NULL)
    {
      gdisp = (GDisplay *) tool->gdisp_ptr;

      gdisplay_transform_coords (gdisp, paint_core->lastx, paint_core->lasty,
				 &tx1, &ty1, 1);
      gdisplay_transform_coords (gdisp, paint_core->curx, paint_core->cury,
				 &tx2, &ty2, 1);

      /*  Draw start target  */
      gdk_draw_line (gdisp->canvas->window, paint_core->core->gc,
		     tx1 - (TARGET_WIDTH >> 1), ty1,
		     tx1 + (TARGET_WIDTH >> 1), ty1);
      gdk_draw_line (gdisp->canvas->window, paint_core->core->gc,
		     tx1, ty1 - (TARGET_HEIGHT >> 1),
		     tx1, ty1 + (TARGET_HEIGHT >> 1));
      
      /*  Draw end target  */
      gdk_draw_line (gdisp->canvas->window, paint_core->core->gc,
		     tx2 - (TARGET_WIDTH >> 1), ty2,
		     tx2 + (TARGET_WIDTH >> 1), ty2);
      gdk_draw_line (gdisp->canvas->window, paint_core->core->gc,
		     tx2, ty2 - (TARGET_HEIGHT >> 1),
		     tx2, ty2 + (TARGET_HEIGHT >> 1));
      
      /*  Draw the line between the start and end coords  */
      gdk_draw_line (gdisp->canvas->window, paint_core->core->gc,
		     tx1, ty1, tx2, ty2);
    }
  return;
}	      

Tool *
paint_core_new (type)
     int type;
{
  Tool * tool;
  PaintCore * private;

  tool = (Tool *) g_malloc (sizeof (Tool));
  private = (PaintCore *) g_malloc (sizeof (PaintCore));

  private->core = draw_core_new (paint_core_draw);
  private->pick_colors = FALSE;

  tool->type = type;
  tool->state = INACTIVE;
  tool->scroll_lock = 0;  /*  Allow scrolling  */
  tool->auto_snap_to = TRUE;
  tool->gdisp_ptr = NULL;
  tool->private = (void *) private;
  tool->preserve = TRUE;

  tool->button_press_func = paint_core_button_press;
  tool->button_release_func = paint_core_button_release;
  tool->motion_func = paint_core_motion;
  tool->arrow_keys_func = standard_arrow_keys_func;  
  tool->modifier_key_func = standard_modifier_key_func;
  tool->cursor_update_func = paint_core_cursor_update;
  tool->control_func = paint_core_control;

  return tool;
}

void
paint_core_free (tool)
     Tool * tool;
{
  PaintCore * paint_core;

  paint_core = (PaintCore *) tool->private;

  /*  Make sure the selection core is not visible  */
  if (tool->state == ACTIVE && paint_core->core)
    draw_core_stop (paint_core->core, tool);

  /*  Free the selection core  */
  if (paint_core->core)
    draw_core_free (paint_core->core);

  /*  Cleanup memory  */
  paint_core_cleanup ();

  /*  Free the paint core  */
  g_free (paint_core);
}

int
paint_core_init (paint_core, drawable, x, y)
     PaintCore *paint_core;
     GimpDrawable *drawable;
     double x, y;
{
  static GimpBrushP brush = 0;
  
  paint_core->curx = x;
  paint_core->cury = y;

  /* Set up some defaults for non-gui use */
  paint_core->startpressure = paint_core->lastpressure = paint_core->curpressure = 0.5;
  paint_core->startxtilt = paint_core->lastxtilt = paint_core->curxtilt = 0;
  paint_core->startytilt = paint_core->lastytilt = paint_core->curytilt = 0;

  /*  Each buffer is the same size as the maximum bounds of the active brush... */
  if (brush && brush != get_active_brush ())
  {
    gtk_signal_disconnect_by_func(GTK_OBJECT(brush),
				  GTK_SIGNAL_FUNC(paint_core_invalidate_cache),
				  NULL);
    gtk_object_unref(GTK_OBJECT(brush));
  }
  if (!(brush = get_active_brush ()))
    {
      g_message (_("No brushes available for use with this tool."));
      return FALSE;
    }
  gtk_object_ref(GTK_OBJECT(brush));
  gtk_signal_connect(GTK_OBJECT (brush), "dirty",
		     GTK_SIGNAL_FUNC(paint_core_invalidate_cache),
		     NULL);

  paint_core->spacing = (double) gimp_brush_get_spacing (brush) / 100.0;
/*  paint_core->spacing =
    (double) MAXIMUM (brush->mask->width, brush->mask->height) *
    ((double) gimp_brush_get_spacing (brush) / 100.0);
  if (paint_core->spacing < 1.0)
    paint_core->spacing = 1.0; */
  paint_core->brush = brush;

  /*  free the block structures  */
  if (undo_tiles)
    tile_manager_destroy (undo_tiles);
  if (canvas_tiles)
    tile_manager_destroy (canvas_tiles);

  /*  Allocate the undo structure  */
  undo_tiles = tile_manager_new (drawable_width (drawable),
				 drawable_height (drawable),
				 drawable_bytes (drawable));

  /*  Allocate the canvas blocks structure  */
  canvas_tiles = tile_manager_new (drawable_width (drawable),
				   drawable_height (drawable), 1);

  /*  Get the initial undo extents  */
  paint_core->x1 = paint_core->x2 = paint_core->curx;
  paint_core->y1 = paint_core->y2 = paint_core->cury;
  paint_core->distance = 0.0;

  return TRUE;
}

void
paint_core_get_color_from_gradient (PaintCore *paint_core, 
				    double gradient_length, 
				    double *r, double *g, double *b, double *a, 
				    int mode)
{
  double y;
  double distance;    /* distance in current brush stroke */
  double position;    /* position in the gradient to ge the color from */
  double temp_opacity;  /* so i can blank out stuff */

  distance = paint_core->distance;
  y = ((double) distance / gradient_length);
  temp_opacity = 1.0; 
 

  /* if were past the first chunk... */
  if ((y/gradient_length) > 1.0)
    {
      /* if this is an "odd" chunk..." */
      if ((int)(y/gradient_length) & 1)
	{
	  /* draw it "normally" */
	  y = y - (gradient_length*(int)(y/gradient_length));
	}
      /* if this is an "even" chunk... */
      else 
	{
	  /* draw it "backwards"  */
	  switch (mode)
	    {
	    case LOOP_SAWTOOTH:
	      y = y - (gradient_length*(int)(y/gradient_length));
	      break;
	    case LOOP_TRIANGLE:
	      y = gradient_length - fmod(y,gradient_length);
	      break;
	    }
	}
      if(mode == ONCE_FORWARD || mode == ONCE_BACKWARDS)
	{
	  printf("got here \n");
	  /* for the once modes, set alpha to 0.0 */
	  temp_opacity = 0.0;
	}

    }
  /* if this is the first chunk... */
  else
    {
      /* draw it backwards */
      switch (mode)
	{
	case ONCE_FORWARD:
	case ONCE_END_COLOR:
	case LOOP_TRIANGLE:
  	  y = gradient_length - y;
	  break;
	default:
	  /* all the other modes go here ;-> */
	  break;
	}
      /* if it doesnt need to be reveresed, let y be y ;-> */
    }
  /* stolen from the fade effect in paintbrush */

  /* model this on a gaussian curve */
  /* position = exp (- y * y * 0.5); */
  position = y/gradient_length;
  grad_get_color_at(position,r,g,b,a);
  /* set opacity to zero if this isnt a repeater call */
  *a = (temp_opacity * *a);

}
  

void
paint_core_interpolate (paint_core, drawable)
     PaintCore *paint_core;
     GimpDrawable *drawable;
{
  int n;
  vector2d delta;
  double dpressure, dxtilt, dytilt;
  double left;
  double t;
  double initial;
  double dist;
  double total;
  double xd, yd;
  double mag;
  delta.x = paint_core->curx - paint_core->lastx;
  delta.y = paint_core->cury - paint_core->lasty;
  dpressure = paint_core->curpressure - paint_core->lastpressure;
  dxtilt = paint_core->curxtilt - paint_core->lastxtilt;
  dytilt = paint_core->curytilt - paint_core->lastytilt;

  if (!delta.x && !delta.y && !dpressure && !dxtilt && !dytilt)
    return;
  mag = vector2d_magnitude (&(paint_core->brush->x_axis));
  xd = vector2d_dot_product(&delta, &(paint_core->brush->x_axis)) / (mag*mag);

  mag = vector2d_magnitude (&(paint_core->brush->y_axis));
  yd = vector2d_dot_product(&delta, &(paint_core->brush->y_axis)) /
    SQR(vector2d_magnitude(&(paint_core->brush->y_axis)));

  dist = .5 * sqrt (xd*xd + yd*yd);

  total = dist + paint_core->distance;
  initial = paint_core->distance;

  while (paint_core->distance < total)
    {
      n = (int) (paint_core->distance / paint_core->spacing + 1.0 + EPSILON);
      left = n * paint_core->spacing - paint_core->distance;
      paint_core->distance += left;

      if (paint_core->distance <= total)
	{
	  t = (paint_core->distance - initial) / dist;

	  paint_core->curx = paint_core->lastx + delta.x * t;
	  paint_core->cury = paint_core->lasty + delta.y * t;
	  paint_core->curpressure = paint_core->lastpressure + dpressure * t;
	  paint_core->curxtilt = paint_core->lastxtilt + dxtilt * t;
	  paint_core->curytilt = paint_core->lastytilt + dytilt * t;
	  (* paint_core->paint_func) (paint_core, drawable, MOTION_PAINT);
	}
    }

  paint_core->distance = total;
  paint_core->curx = paint_core->lastx + delta.x;
  paint_core->cury = paint_core->lasty + delta.y;
  paint_core->curpressure = paint_core->lastpressure + dpressure;
  paint_core->curxtilt = paint_core->lastxtilt + dxtilt;
  paint_core->curytilt = paint_core->lastytilt + dytilt;
}

void
paint_core_finish (paint_core, drawable, tool_id)
     PaintCore *paint_core;
     GimpDrawable *drawable;
     int tool_id;
{
  GImage *gimage;
  PaintUndo *pu;

  if (! (gimage = drawable_gimage (drawable)))
    return;

  /*  Determine if any part of the image has been altered--
   *  if nothing has, then just return...
   */

  if ((paint_core->x2 == paint_core->x1) || (paint_core->y2 == paint_core->y1))
    return;

  undo_push_group_start (gimage, PAINT_CORE_UNDO);

  pu = (PaintUndo *) g_malloc (sizeof (PaintUndo));
  pu->tool_ID = tool_id;
  pu->lastx = paint_core->startx;
  pu->lasty = paint_core->starty;
  pu->lastpressure = paint_core->startpressure;
  pu->lastxtilt = paint_core->startxtilt;
  pu->lastytilt = paint_core->startytilt;

  /*  Push a paint undo  */
  undo_push_paint (gimage, pu);

  /*  push an undo  */
  drawable_apply_image (drawable, paint_core->x1, paint_core->y1,
			paint_core->x2, paint_core->y2, undo_tiles, TRUE);
  undo_tiles = NULL;

  /*  push the group end  */
  undo_push_group_end (gimage);

  /*  invalidate the drawable--have to do it here, because
   *  it is not done during the actual painting.
   */
  drawable_invalidate_preview (drawable);
}

void
paint_core_cleanup ()
{
  /*  CLEANUP  */
  /*  If the undo tiles exist, nuke them  */
  if (undo_tiles)
    {
      tile_manager_destroy (undo_tiles);
      undo_tiles = NULL;
    }

  /*  If the canvas blocks exist, nuke them  */
  if (canvas_tiles)
    {
      tile_manager_destroy (canvas_tiles);
      canvas_tiles = NULL;
    }

  /*  Free the temporary buffers if they exist  */
  free_paint_buffers ();
}


/************************/
/*  Painting functions  */
/************************/

TempBuf *
paint_core_get_paint_area (paint_core, drawable)
     PaintCore *paint_core;
     GimpDrawable *drawable;
{
  int x, y;
  int x1, y1, x2, y2;
  int bytes;
  int dwidth, dheight;

  bytes = drawable_has_alpha (drawable) ?
    drawable_bytes (drawable) : drawable_bytes (drawable) + 1;

  /*  adjust the x and y coordinates to the upper left corner of the brush  */
  x = (int) paint_core->curx - (paint_core->brush->mask->width >> 1);
  y = (int) paint_core->cury - (paint_core->brush->mask->height >> 1);

  dwidth = drawable_width (drawable);
  dheight = drawable_height (drawable);

  x1 = BOUNDS (x - 1, 0, dwidth);
  y1 = BOUNDS (y - 1, 0, dheight);
  x2 = BOUNDS (x + paint_core->brush->mask->width + 1,
	       0, dwidth);
  y2 = BOUNDS (y + paint_core->brush->mask->height + 1,
	       0, dheight);

  /*  configure the canvas buffer  */
  if ((x2 - x1) && (y2 - y1))
    canvas_buf = temp_buf_resize (canvas_buf, bytes, x1, y1,
				  (x2 - x1), (y2 - y1));
  else
    return NULL;

  return canvas_buf;
}

TempBuf *
paint_core_get_orig_image (paint_core, drawable, x1, y1, x2, y2)
     PaintCore *paint_core;
     GimpDrawable *drawable;
     int x1, y1;
     int x2, y2;
{
  PixelRegion srcPR, destPR;
  Tile *undo_tile;
  int h;
  int refd;
  int pixelwidth;
  int dwidth, dheight;
  unsigned char * s, * d;
  void * pr;

  orig_buf = temp_buf_resize (orig_buf, drawable_bytes (drawable),
			      x1, y1, (x2 - x1), (y2 - y1));

  dwidth = drawable_width (drawable);
  dheight = drawable_height (drawable);

  x1 = BOUNDS (x1, 0, dwidth);
  y1 = BOUNDS (y1, 0, dheight);
  x2 = BOUNDS (x2, 0, dwidth);
  y2 = BOUNDS (y2, 0, dheight);

  /*  configure the pixel regions  */
  pixel_region_init (&srcPR, drawable_data (drawable), x1, y1,
		     (x2 - x1), (y2 - y1), FALSE);
  destPR.bytes = orig_buf->bytes;
  destPR.x = 0; destPR.y = 0;
  destPR.w = (x2 - x1); destPR.h = (y2 - y1);
  destPR.rowstride = orig_buf->bytes * orig_buf->width;
  destPR.data = temp_buf_data (orig_buf) +
    (y1 - orig_buf->y) * destPR.rowstride + (x1 - orig_buf->x) * destPR.bytes;

  for (pr = pixel_regions_register (2, &srcPR, &destPR);
       pr != NULL;
       pr = pixel_regions_process (pr))
    {
      /*  If the undo tile corresponding to this location is valid, use it  */
      undo_tile = tile_manager_get_tile (undo_tiles, srcPR.x, srcPR.y,
					 FALSE, FALSE);
      if (tile_is_valid (undo_tile) == TRUE)
	{
	  refd = 1;
	  undo_tile = tile_manager_get_tile (undo_tiles, srcPR.x, srcPR.y,
					     TRUE, FALSE);
	  s = (unsigned char*)tile_data_pointer (undo_tile, 0, 0) +
	    srcPR.rowstride * (srcPR.y % TILE_HEIGHT) +
	    srcPR.bytes * (srcPR.x % TILE_WIDTH); /* dubious... */
	}
      else
	{
	  refd = 0;
	  s = srcPR.data;
	}

      d = destPR.data;
      pixelwidth = srcPR.w * srcPR.bytes;
      h = srcPR.h;
      while (h --)
	{
	  memcpy (d, s, pixelwidth);
	  s += srcPR.rowstride;
	  d += destPR.rowstride;
	}

      if (refd)
	tile_release (undo_tile, FALSE);
    }

  return orig_buf;
}

void
paint_core_paste_canvas (paint_core, drawable, brush_opacity, image_opacity, paint_mode,
			 brush_hardness, mode)
     PaintCore *paint_core;
     GimpDrawable *drawable;
     int brush_opacity;
     int image_opacity;
     int paint_mode;
     int brush_hardness;
     int mode;
{
  MaskBuf *brush_mask;

  /*  get the brush mask  */
  brush_mask = paint_core_get_brush_mask (paint_core, brush_hardness);

  /*  paste the canvas buf  */
  paint_core_paste (paint_core, brush_mask, drawable,
		    brush_opacity, image_opacity, paint_mode, mode);
}

/* Similar to paint_core_paste_canvas, but replaces the alpha channel
   rather than using it to composite (i.e. transparent over opaque
   becomes transparent rather than opauqe. */
void
paint_core_replace_canvas (paint_core, drawable, brush_opacity, image_opacity,
			   brush_hardness, mode)
     PaintCore *paint_core;
     GimpDrawable *drawable;
     int brush_opacity;
     int image_opacity;
     int brush_hardness;
     int mode;
{
  MaskBuf *brush_mask;

  /*  get the brush mask  */
  brush_mask = paint_core_get_brush_mask (paint_core, brush_hardness);

  /*  paste the canvas buf  */
  paint_core_replace (paint_core, brush_mask, drawable,
		      brush_opacity, image_opacity, mode);
}

static MaskBuf *last_brush_mask = NULL;
static int cache_invalid = 0;

static int paint_core_invalidate_cache(GimpBrush *brush, gpointer *blah)
{
/* Make sure we don't cache data for a brush that has changed */
  if (last_brush_mask == brush->mask)
  {
    cache_invalid = 1;
    return 1;
  }
  return 0;
}

/************************************************************
 *             LOCAL FUNCTION DEFINITIONS                   *
 ************************************************************/

static MaskBuf *
paint_core_subsample_mask (mask, x, y)
     MaskBuf * mask;
     double x, y;
{
  MaskBuf * dest;
  double left;
  unsigned char * m, * d;
  const int * k;
  int index1, index2;
  const int * kernel;
  int new_val;
  int i, j;
  int r, s;

  x += (x < 0) ? mask->width : 0;
  left = x - floor(x) + 0.125;
  index1 = (int) (left * 4);

  y += (y < 0) ? mask->height : 0;
  left = y - floor(y) + 0.125;
  index2 = (int) (left * 4);

  kernel = subsample[index2][index1];

  if ((mask == last_brush_mask) && kernel_brushes[index2][index1] &&
      !cache_invalid)
    return kernel_brushes[index2][index1];
  else if (mask != last_brush_mask || cache_invalid)
    for (i = 0; i < 5; i++)
      for (j = 0; j < 5; j++)
	{
	  if (kernel_brushes[i][j])
	    mask_buf_free (kernel_brushes[i][j]);
	  kernel_brushes[i][j] = NULL;
	}

  last_brush_mask = mask;
  cache_invalid = 0;
  kernel_brushes[index2][index1] = mask_buf_new (mask->width + 2, mask->height + 2);
  dest = kernel_brushes[index2][index1];

  m = mask_buf_data (mask);
  for (i = 0; i < mask->height; i++)
    {
      for (j = 0; j < mask->width; j++)
	{
	  k = kernel;
	  for (r = 0; r < KERNEL_HEIGHT; r++)
	    {
	      d = mask_buf_data (dest) + (i+r) * dest->width + j;
	      s = KERNEL_WIDTH;
	      while (s--)
		{
		  new_val = *d + ((*m * *k++) >> 8);
		  *d++ = MINIMUM (new_val, 255);
		}
	    }
	  m++;
	}
    }

  return dest;
}

/* #define FANCY_PRESSURE */

static MaskBuf *
paint_core_pressurize_mask (brush_mask, x, y, pressure)
     MaskBuf * brush_mask;
     double x, y;
     double pressure;
{
  static MaskBuf *last_brush = NULL;
  static unsigned char mapi[256];
  unsigned char *source;
  unsigned char *dest;
  MaskBuf *subsample_mask;
  int i;
#ifdef FANCY_PRESSURE
  static double map[256];
  double ds,s,c;
#endif

  /* Get the raw subsampled mask */
  subsample_mask = paint_core_subsample_mask(brush_mask, x, y);

  /* Special case pressure = 0.5 */
  if ((int)(pressure*100+0.5) == 50)
    return subsample_mask;

  /* Make sure we have the right sized buffer */
  if (brush_mask != last_brush)
    {
      if (pressure_brush)
	mask_buf_free (pressure_brush);
      pressure_brush = mask_buf_new (brush_mask->width + 2,
				     brush_mask->height +2);
    }

#ifdef FANCY_PRESSURE
  /* Create the pressure profile

     It is: I'(I) = tanh(20*(pressure-0.5)*I) : pressure > 0.5
            I'(I) = 1 - tanh(20*(0.5-pressure)*(1-I)) : pressure < 0.5

     It looks like:

        low pressure      medium pressure     high pressure

             |                   /                 --
             |    		/                 /
            /     	       /                 |
          --                  /	                 |

  */

  ds = (pressure - 0.5)*(20./256.);
  s = 0;
  c = 1.0;

  if (ds > 0)
    {
      for (i=0;i<256;i++)
	{
	  map[i] = s/c;
	  s += c*ds;
	  c += s*ds;
	}
      for (i=0;i<256;i++)
	mapi[i] = (int)(255*map[i]/map[255]);
    }
  else
    {
      ds = -ds;
      for (i=255;i>=0;i--)
	{
	  map[i] = s/c;
	  s += c*ds;
	  c += s*ds;
	}
      for (i=0;i<256;i++)
	mapi[i] = (int)(255*(1-map[i]/map[0]));
    }
#else /* ! FANCY_PRESSURE */

  for (i=0;i<256;i++)
    {
      int tmp = (pressure/0.5)*i;
      if (tmp > 255)
	mapi[i] = 255;
      else
	mapi[i] = tmp;
    }

#endif /* FANCY_PRESSURE */

  /* Now convert the brush */

  source = mask_buf_data (subsample_mask);
  dest = mask_buf_data (pressure_brush);

  i = subsample_mask->width * subsample_mask->height;
  while (i--)
    {
      *dest++ = mapi[(*source++)];
    }

  return pressure_brush;
}

static MaskBuf *
paint_core_solidify_mask (brush_mask)
     MaskBuf * brush_mask;
{
  static MaskBuf *last_brush = NULL;
  int i, j;
  unsigned char * data, * src;

  if (brush_mask == last_brush)
    return solid_brush;

  last_brush = brush_mask;
  if (solid_brush)
    mask_buf_free (solid_brush);
  solid_brush = mask_buf_new (brush_mask->width + 2, brush_mask->height + 2);

  /*  get the data and advance one line into it  */
  data = mask_buf_data (solid_brush) + solid_brush->width;
  src = mask_buf_data (brush_mask);

  for (i = 0; i < brush_mask->height; i++)
    {
      data++;
      for (j = 0; j < brush_mask->width; j++)
	{
	  *data++ = (*src++) ? OPAQUE_OPACITY : TRANSPARENT_OPACITY;
	}
      data++;
    }

  return solid_brush;
}

static MaskBuf *
paint_core_get_brush_mask (paint_core, brush_hardness)
     PaintCore * paint_core;
     int brush_hardness;
{
  MaskBuf * bm;

  switch (brush_hardness)
    {
    case SOFT:
      bm = paint_core_subsample_mask (paint_core->brush->mask, paint_core->curx, paint_core->cury);
      break;
    case HARD:
      bm = paint_core_solidify_mask (paint_core->brush->mask);
      break;
    case PRESSURE:
      bm = paint_core_pressurize_mask (paint_core->brush->mask, paint_core->curx, paint_core->cury, paint_core->curpressure);
      break;
    default:
      bm = NULL;
      break;
    }

  return bm;
}

static void
paint_core_paste (paint_core, brush_mask, drawable, brush_opacity, image_opacity, paint_mode, mode)
     PaintCore *paint_core;
     MaskBuf *brush_mask;
     GimpDrawable *drawable;
     int brush_opacity;
     int image_opacity;
     int paint_mode;
     int mode;
{
  GImage *gimage;
  PixelRegion srcPR;
  TileManager *alt = NULL;
  int offx, offy;

  if (! (gimage = drawable_gimage (drawable)))
    return;

  /*  set undo blocks  */
  set_undo_tiles (drawable,
		  canvas_buf->x, canvas_buf->y,
		  canvas_buf->width, canvas_buf->height);

  /*  If the mode is CONSTANT:
   *   combine the canvas buf, the brush mask to the canvas tiles
   */
  if (mode == CONSTANT)
    {
      /*  initialize any invalid canvas tiles  */
      set_canvas_tiles (canvas_buf->x, canvas_buf->y,
			canvas_buf->width, canvas_buf->height);

      paint_to_canvas_tiles (paint_core, brush_mask, brush_opacity);
      alt = undo_tiles;
    }
  /*  Otherwise:
   *   combine the canvas buf and the brush mask to the canvas buf
   */
  else  /*  mode != CONSTANT  */
    paint_to_canvas_buf (paint_core, brush_mask, brush_opacity);

  /*  intialize canvas buf source pixel regions  */
  srcPR.bytes = canvas_buf->bytes;
  srcPR.x = 0; srcPR.y = 0;
  srcPR.w = canvas_buf->width;
  srcPR.h = canvas_buf->height;
  srcPR.rowstride = canvas_buf->width * canvas_buf->bytes;
  srcPR.data = temp_buf_data (canvas_buf);

  /*  apply the paint area to the gimage  */
  gimage_apply_image (gimage, drawable, &srcPR,
		      FALSE, image_opacity, paint_mode,
		      alt,  /*  specify an alternative src1  */
		      canvas_buf->x, canvas_buf->y);

  /*  Update the undo extents  */
  paint_core->x1 = MINIMUM (paint_core->x1, canvas_buf->x);
  paint_core->y1 = MINIMUM (paint_core->y1, canvas_buf->y);
  paint_core->x2 = MAXIMUM (paint_core->x2, (canvas_buf->x + canvas_buf->width));
  paint_core->y2 = MAXIMUM (paint_core->y2, (canvas_buf->y + canvas_buf->height));

  /*  Update the gimage--it is important to call gdisplays_update_area
   *  instead of drawable_update because we don't want the drawable
   *  preview to be constantly invalidated
   */
  drawable_offsets (drawable, &offx, &offy);
  gdisplays_update_area (gimage, canvas_buf->x + offx, canvas_buf->y + offy,
			 canvas_buf->width, canvas_buf->height);
}

/* This works similarly to paint_core_paste. However, instead of combining
   the canvas to the paint core drawable using one of the combination
   modes, it uses a "replace" mode (i.e. transparent pixels in the 
   canvas erase the paint core drawable).

   When not drawing on alpha-enabled images, it just paints using NORMAL
   mode.
*/
static void
paint_core_replace (paint_core, brush_mask, drawable, brush_opacity, image_opacity, mode)
     PaintCore *paint_core;
     MaskBuf *brush_mask;
     GimpDrawable *drawable;
     int brush_opacity;
     int image_opacity;
     int mode;
{
  GImage *gimage;
  PixelRegion srcPR, maskPR;
  int offx, offy;

  if (!drawable_has_alpha (drawable))
    {
      paint_core_paste (paint_core, brush_mask, drawable,
			brush_opacity, image_opacity, NORMAL_MODE,
			mode);
      return;
    }

  if (mode != INCREMENTAL)
    {
      g_message (_("paint_core_replace only works in INCREMENTAL mode"));
      return;
    }

  if (! (gimage = drawable_gimage (drawable)))
    return;

  /*  set undo blocks  */
  set_undo_tiles (drawable,
		  canvas_buf->x, canvas_buf->y,
		  canvas_buf->width, canvas_buf->height);

  maskPR.bytes = 1;
  maskPR.x = 0; maskPR.y = 0;
  maskPR.w = canvas_buf->width;
  maskPR.h = canvas_buf->height;
  maskPR.rowstride = maskPR.bytes * brush_mask->width;
  maskPR.data = mask_buf_data (brush_mask);
  
  /*  intialize canvas buf source pixel regions  */
  srcPR.bytes = canvas_buf->bytes;
  srcPR.x = 0; srcPR.y = 0;
  srcPR.w = canvas_buf->width;
  srcPR.h = canvas_buf->height;
  srcPR.rowstride = canvas_buf->width * canvas_buf->bytes;
  srcPR.data = temp_buf_data (canvas_buf);

  /*  apply the paint area to the gimage  */
  gimage_replace_image (gimage, drawable, &srcPR,
			FALSE, image_opacity,
			&maskPR,
			canvas_buf->x, canvas_buf->y);

  /*  Update the undo extents  */
  paint_core->x1 = MINIMUM (paint_core->x1, canvas_buf->x);
  paint_core->y1 = MINIMUM (paint_core->y1, canvas_buf->y);
  paint_core->x2 = MAXIMUM (paint_core->x2, (canvas_buf->x + canvas_buf->width));
  paint_core->y2 = MAXIMUM (paint_core->y2, (canvas_buf->y + canvas_buf->height));

  /*  Update the gimage--it is important to call gdisplays_update_area
   *  instead of drawable_update because we don't want the drawable
   *  preview to be constantly invalidated
   */
  drawable_offsets (drawable, &offx, &offy);
  gdisplays_update_area (gimage, canvas_buf->x + offx, canvas_buf->y + offy,
			 canvas_buf->width, canvas_buf->height);
}

static void
paint_to_canvas_tiles (paint_core, brush_mask, brush_opacity)
     PaintCore *paint_core;
     MaskBuf *brush_mask;
     int brush_opacity;
{
  PixelRegion srcPR, maskPR;
  int x, y;
  int xoff, yoff;

  /*   combine the brush mask and the canvas tiles  */
  pixel_region_init (&srcPR, canvas_tiles,
		     canvas_buf->x, canvas_buf->y,
		     canvas_buf->width, canvas_buf->height, TRUE);

  x = (int) paint_core->curx - (brush_mask->width >> 1);
  y = (int) paint_core->cury - (brush_mask->height >> 1);
  xoff = (x < 0) ? -x : 0;
  yoff = (y < 0) ? -y : 0;

  maskPR.bytes = 1;
  maskPR.x = 0; maskPR.y = 0;
  maskPR.w = srcPR.w;
  maskPR.h = srcPR.h;
  maskPR.rowstride = maskPR.bytes * brush_mask->width;
  maskPR.data = mask_buf_data (brush_mask) + yoff * maskPR.rowstride + xoff * maskPR.bytes;

  /*  combine the mask and canvas tiles  */
  combine_mask_and_region (&srcPR, &maskPR, brush_opacity);

  /*  combine the canvas tiles and the canvas buf  */
  srcPR.bytes = canvas_buf->bytes;
  srcPR.x = 0; srcPR.y = 0;
  srcPR.w = canvas_buf->width;
  srcPR.h = canvas_buf->height;
  srcPR.rowstride = canvas_buf->width * canvas_buf->bytes;
  srcPR.data = temp_buf_data (canvas_buf);

  pixel_region_init (&maskPR, canvas_tiles,
		     canvas_buf->x, canvas_buf->y,
		     canvas_buf->width, canvas_buf->height, FALSE);

  /*  apply the canvas tiles to the canvas buf  */
  apply_mask_to_region (&srcPR, &maskPR, OPAQUE_OPACITY);
}

static void
paint_to_canvas_buf (paint_core, brush_mask, brush_opacity)
     PaintCore *paint_core;
     MaskBuf *brush_mask;
     int brush_opacity;
{
  PixelRegion srcPR, maskPR;
  int x, y;
  int xoff, yoff;

  x = (int) paint_core->curx - (brush_mask->width >> 1);
  y = (int) paint_core->cury - (brush_mask->height >> 1);
  xoff = (x < 0) ? -x : 0;
  yoff = (y < 0) ? -y : 0;


  /*  combine the canvas buf and the brush mask to the canvas buf  */
  srcPR.bytes = canvas_buf->bytes;
  srcPR.x = 0; srcPR.y = 0;
  srcPR.w = canvas_buf->width;
  srcPR.h = canvas_buf->height;
  srcPR.rowstride = canvas_buf->width * canvas_buf->bytes;
  srcPR.data = temp_buf_data (canvas_buf);

  maskPR.bytes = 1;
  maskPR.x = 0; maskPR.y = 0;
  maskPR.w = srcPR.w;
  maskPR.h = srcPR.h;
  maskPR.rowstride = maskPR.bytes * brush_mask->width;
  maskPR.data = mask_buf_data (brush_mask) + yoff * maskPR.rowstride + xoff * maskPR.bytes;

  /*  apply the mask  */
  apply_mask_to_region (&srcPR, &maskPR, brush_opacity);
}

static void
set_undo_tiles (drawable, x, y, w, h)
     GimpDrawable *drawable;
     int x, y;
     int w, h;
{
  int i, j;
  Tile *src_tile;
  Tile *dest_tile;

  if (undo_tiles == NULL) 
    {
      g_warning (_("set_undo_tiles: undo_tiles is null"));
      return;
    }

  for (i = y; i < (y + h); i += (TILE_HEIGHT - (i % TILE_HEIGHT)))
    {
      for (j = x; j < (x + w); j += (TILE_WIDTH - (j % TILE_WIDTH)))
	{
	  dest_tile = tile_manager_get_tile (undo_tiles, j, i, FALSE, FALSE);
	  if (tile_is_valid (dest_tile) == FALSE)
	    {
	      src_tile = tile_manager_get_tile (drawable_data (drawable), j, i, TRUE, FALSE);
	      tile_manager_map_tile (undo_tiles, j, i, src_tile);
	      tile_release (src_tile, FALSE);
	    }
	}
    }
}

static void
set_canvas_tiles (x, y, w, h)
     int x, y;
     int w, h;
{
  int i, j;
  Tile *tile;

  for (i = y; i < (y + h); i += (TILE_HEIGHT - (i % TILE_HEIGHT)))
    {
      for (j = x; j < (x + w); j += (TILE_WIDTH - (j % TILE_WIDTH)))
	{
	  tile = tile_manager_get_tile (canvas_tiles, j, i, FALSE, FALSE);
	  if (tile_is_valid (tile) == FALSE)
	    {
	      tile = tile_manager_get_tile (canvas_tiles, j, i, TRUE, TRUE);
	      memset (tile_data_pointer (tile, 0, 0), 0, 
		      tile_size (tile));
	      tile_release (tile, TRUE);
	    }
	}
    }
}


/*****************************************************/
/*  Paint buffers utility functions                  */
/*****************************************************/


static void
free_paint_buffers ()
{
  if (orig_buf)
    temp_buf_free (orig_buf);
  orig_buf = NULL;

  if (canvas_buf)
    temp_buf_free (canvas_buf);
  canvas_buf = NULL;
}
