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

#include "config.h"

#include <stdlib.h>
#include <string.h>

#include <gtk/gtk.h>

#include "libgimpcolor/gimpcolor.h"
#include "libgimpmath/gimpmath.h"

#include "apptypes.h"

#include "brush_scale.h"
#include "cursorutil.h"
#include "devices.h"
#include "drawable.h"
#include "gdisplay.h"
#include "gimage_mask.h"
#include "gimpbrushpipe.h"
#include "gimpcontext.h"
#include "gimpgradient.h"
#include "gimpimage.h"
#include "gimprc.h"
#include "paint_funcs.h"
#include "pixel_region.h"
#include "selection.h"
#include "temp_buf.h"
#include "tile.h"
#include "tile_manager.h"
#include "undo.h"

#include "gimpdrawtool.h"
#include "gimppainttool.h"

#include "libgimp/gimpintl.h"

#include "gimppainttool_kernels.h"


/*  target size  */
#define  TARGET_HEIGHT  15
#define  TARGET_WIDTH   15

#define  EPSILON  0.00001

#define  STATUSBAR_SIZE 128


enum
{
  PAINT,
  LAST_SIGNAL
};


/*  local function prototypes  */
static void   gimp_paint_tool_class_init      (GimpPaintToolClass  *klass);
static void   gimp_paint_tool_init            (GimpPaintTool       *paint_tool);
static void   gimp_paint_tool_destroy         (GtkObject           *object);

static void   gimp_paint_tool_control         (GimpTool	           *tool,
					       ToolAction           action,
					       GDisplay            *gdisp);
static void   gimp_paint_tool_button_press    (GimpTool            *tool,
					       GdkEventButton      *bevent,
					       GDisplay            *gdisp);
static void   gimp_paint_tool_button_release  (GimpTool            *tool,
					       GdkEventButton      *bevent,
					       GDisplay            *gdisp);
static void   gimp_paint_tool_motion          (GimpTool            *tool,
					       GdkEventMotion      *mevent,
					       GDisplay            *gdisp);
static void   gimp_paint_tool_cursor_update   (GimpTool            *tool,
					       GdkEventMotion      *mevent,
					       GDisplay            *gdisp);

static void   gimp_paint_tool_draw            (GimpDrawTool        *draw_tool);


static void      gimp_paint_tool_sample_color         (GimpDrawable *drawable,
						       gint          x,
						       gint          y,
						       gint          state);
static void      gimp_paint_tool_calculate_brush_size (MaskBuf *mask,
						       gdouble  scale,
						       gint    *width,
						       gint    *height);
static MaskBuf * gimp_paint_tool_subsample_mask       (MaskBuf *mask,
						       gdouble  x,
						       gdouble  y);
static MaskBuf * gimp_paint_tool_pressurize_mask      (MaskBuf *brush_mask,
						       gdouble  x,
						       gdouble  y,
						       gdouble  pressure);
static MaskBuf * gimp_paint_tool_solidify_mask        (MaskBuf *brush_mask);
static MaskBuf * gimp_paint_tool_scale_mask           (MaskBuf *brush_mask,
						       gdouble  scale);
static MaskBuf * gimp_paint_tool_scale_pixmap         (MaskBuf *brush_mask,
						       gdouble  scale);

static MaskBuf * gimp_paint_tool_get_brush_mask       (GimpPaintTool            *gimp_paint_tool,
						       BrushApplicationMode  brush_hardness,
						       gdouble               scale);
static void      gimp_paint_tool_paste                (GimpPaintTool	       *gimp_paint_tool,
						       MaskBuf	       *brush_mask,
						       GimpDrawable	       *drawable,
						       gint		        brush_opacity,
						       gint		        image_opacity,
						       LayerModeEffects      paint_mode,
						       PaintApplicationMode  mode);
static void      gimp_paint_tool_replace              (GimpPaintTool            *gimp_paint_tool,
						       MaskBuf              *brush_mask,
						       GimpDrawable	       *drawable,
						       gint		        brush_opacity,
						       gint                  image_opacity,
						       PaintApplicationMode  mode);

static void      brush_to_canvas_tiles           (GimpPaintTool    *gimp_paint_tool,
						  MaskBuf      *brush_mask,
						  gint          brush_opacity);
static void      brush_to_canvas_buf             (GimpPaintTool    *gimp_paint_tool,
						  MaskBuf      *brush_mask,
						  gint          brush_opacity);
static void      canvas_tiles_to_canvas_buf      (GimpPaintTool    *gimp_paint_tool);

static void      set_undo_tiles                  (GimpDrawable *drawable,
						  gint          x,
						  gint          y,
						  gint          w,
						  gint          h);
static void      set_canvas_tiles                (gint          x,
						  gint          y,
						  gint          w,
						  gint          h);
static void      gimp_paint_tool_invalidate_cache     (GimpBrush    *brush,
						       gpointer      data);


/*  paint buffers utility functions  */
static void        free_paint_buffers            (void);

/*  brush pipe utility functions  */
static void        paint_line_pixmap_mask  (GimpImage            *dest,
					    GimpDrawable         *drawable,
					    TempBuf              *pixmap_mask,
					    TempBuf              *brush_mask,
					    guchar               *d,
					    gint                  x,
					    gint                  y,
					    gint                  bytes,
					    gint                  width,
					    BrushApplicationMode  mode);

/***********************************************************************/


/*  undo blocks variables  */
static TileManager  *undo_tiles   = NULL;
static TileManager  *canvas_tiles = NULL;


/***********************************************************************/


/*  paint buffers variables  */
static TempBuf  *orig_buf   = NULL;
static TempBuf  *canvas_buf = NULL;


/*  brush buffers  */
static MaskBuf  *pressure_brush;
static MaskBuf  *solid_brush;
static MaskBuf  *scale_brush     = NULL;
static MaskBuf  *scale_pixmap    = NULL;
static MaskBuf  *kernel_brushes[SUBSAMPLE + 1][SUBSAMPLE + 1];

static MaskBuf  *last_brush_mask = NULL;
static gboolean  cache_invalid   = FALSE;


static guint gimp_paint_tool_signals[LAST_SIGNAL] = { 0 };

static GimpDrawToolClass *parent_class = NULL;


GtkType
gimp_paint_tool_get_type (void)
{
  static GtkType tool_type = 0;

  if (! tool_type)
    {
      GtkTypeInfo tool_info =
      {
        "GimpPaintTool",
        sizeof (GimpPaintTool),
        sizeof (GimpPaintToolClass),
        (GtkClassInitFunc) gimp_paint_tool_class_init,
        (GtkObjectInitFunc) gimp_paint_tool_init,
        /* reserved_1 */ NULL,
        /* reserved_2 */ NULL,
        NULL
      };

      tool_type = gtk_type_unique (GIMP_TYPE_DRAW_TOOL, &tool_info);
    }

  return tool_type;
}

static void
gimp_paint_tool_class_init (GimpPaintToolClass *klass)
{
  GtkObjectClass    *object_class;
  GimpToolClass     *tool_class;
  GimpDrawToolClass *draw_tool_class;

  object_class    = (GtkObjectClass *) klass;
  tool_class      = (GimpToolClass *) klass;
  draw_tool_class = (GimpDrawToolClass *) klass;

  parent_class = gtk_type_class (GIMP_TYPE_DRAW_TOOL);

  gimp_paint_tool_signals[PAINT] =
    gtk_signal_new ("paint",
    		    GTK_RUN_FIRST,
    		    object_class->type,
    		    GTK_SIGNAL_OFFSET (GimpPaintToolClass,
    		    		       paint),
		    gtk_marshal_NONE__POINTER_INT,
		    GTK_TYPE_NONE, 2,
		    GTK_TYPE_POINTER,
		    GTK_TYPE_INT);

  gtk_object_class_add_signals (object_class, gimp_paint_tool_signals,
				LAST_SIGNAL);

  object_class->destroy      = gimp_paint_tool_destroy;

  tool_class->button_press   = gimp_paint_tool_button_press;
  tool_class->button_release = gimp_paint_tool_button_release;
  tool_class->motion         = gimp_paint_tool_motion;
  tool_class->cursor_update  = gimp_paint_tool_cursor_update;
  tool_class->control        = gimp_paint_tool_control;

  draw_tool_class->draw      = gimp_paint_tool_draw;

  klass->paint               = NULL;
}

static void
gimp_paint_tool_init (GimpPaintTool *tool)
{
  tool->pick_colors = FALSE;
  tool->flags       = 0;
  tool->context_id  = 0;
}

static void
gimp_paint_tool_destroy (GtkObject *object)
{
  GimpPaintTool *paint_tool;

  paint_tool = GIMP_PAINT_TOOL (object);

  /*  Cleanup memory  */
  gimp_paint_tool_cleanup ();

  if (GTK_OBJECT_CLASS (parent_class)->destroy)
    GTK_OBJECT_CLASS (parent_class)->destroy (object);
}

static void
gimp_paint_tool_control (GimpTool   *tool,
			 ToolAction  action,
			 GDisplay   *gdisp)
{
  GimpPaintTool *paint_tool;
  GimpDrawable  *drawable;

  paint_tool = GIMP_PAINT_TOOL (tool);
  drawable   = gimp_image_active_drawable (gdisp->gimage);

  switch (action)
    {
    case PAUSE:
      break;

    case RESUME:
      break;

    case HALT:
      gimp_paint_tool_paint (paint_tool, drawable, FINISH_PAINT);
      gimp_paint_tool_cleanup ();
      break;

    default:
      break;
    }

  if (GIMP_TOOL_CLASS (parent_class)->control)
    GIMP_TOOL_CLASS (parent_class)->control (tool, action, gdisp);
}

static void
gimp_paint_tool_button_press (GimpTool       *tool,
			      GdkEventButton *bevent,
			      GDisplay       *gdisp)
{
  GimpPaintTool *paint_tool;
  GimpBrush    	*current_brush;
  gboolean       draw_line;
  gdouble        x, y;
  GimpDrawable  *drawable;

  g_return_if_fail (gdisp != NULL);
  g_return_if_fail (tool != NULL);

  paint_tool = GIMP_PAINT_TOOL (tool);

  gdisplay_untransform_coords_f (gdisp, (double) bevent->x, (double) bevent->y,
				 &x, &y, TRUE);
  drawable = gimp_image_active_drawable (gdisp->gimage);

  if (! gimp_paint_tool_start (paint_tool, drawable, x, y))
    return;

  draw_line = FALSE;

  paint_tool->curpressure = bevent->pressure;
  paint_tool->curxtilt = bevent->xtilt;
  paint_tool->curytilt = bevent->ytilt;
#ifdef GTK_HAVE_SIX_VALUATORS
  paint_tool->curwheel = bevent->wheel;
#endif /* GTK_HAVE_SIX_VALUATORS */
  paint_tool->state = bevent->state;

  if (gdisp != tool->gdisp ||
      paint_tool->context_id < 1)
    {
      /* initialize the statusbar display */
      paint_tool->context_id =
	gtk_statusbar_get_context_id (GTK_STATUSBAR (gdisp->statusbar), "paint");
    }

  /*  if this is a new image, reinit the core vals  */
  if ((gdisp != tool->gdisp) || ! (bevent->state & GDK_SHIFT_MASK))
    {
      /*  initialize some values  */
      paint_tool->startx        = paint_tool->lastx        = paint_tool->curx = x;
      paint_tool->starty        = paint_tool->lasty        = paint_tool->cury = y;
      paint_tool->startpressure = paint_tool->lastpressure = paint_tool->curpressure;
      paint_tool->startytilt    = paint_tool->lastytilt    = paint_tool->curytilt;
      paint_tool->startxtilt    = paint_tool->lastxtilt    = paint_tool->curxtilt;
#ifdef GTK_HAVE_SIX_VALUATORS
      paint_tool->startwheel    = paint_tool->lastwheel    = paint_tool->curwheel;
#endif /* GTK_HAVE_SIX_VALUATORS */
    }

  /*  If shift is down and this is not the first paint
   *  stroke, then draw a line from the last coords to the pointer
   */
  else if (bevent->state & GDK_SHIFT_MASK)
    {
      draw_line = TRUE;
      paint_tool->startx        = paint_tool->lastx;
      paint_tool->starty        = paint_tool->lasty;
      paint_tool->startpressure = paint_tool->lastpressure;
      paint_tool->startxtilt    = paint_tool->lastxtilt;
      paint_tool->startytilt    = paint_tool->lastytilt;
#ifdef GTK_HAVE_SIX_VALUATORS
      paint_tool->startwheel    = paint_tool->lastwheel;
#endif /* GTK_HAVE_SIX_VALUATORS */

      /* Restrict to multiples of 15 degrees if ctrl is pressed */
      if (bevent->state & GDK_CONTROL_MASK)
	{
	  gint tangens2[6] = {  34, 106, 196, 334, 618, 1944 };
	  gint cosinus[7]  = { 256, 247, 222, 181, 128, 66, 0 };
	  gint dx, dy, i, radius, frac;

	  dx = paint_tool->curx - paint_tool->lastx;
	  dy = paint_tool->cury - paint_tool->lasty;

	  if (dy)
	    {
	      radius = sqrt (SQR (dx) + SQR (dy));
	      frac = abs ((dx << 8) / dy);
	      for (i = 0; i < 6; i++)
		{
		  if (frac < tangens2[i])
		    break;
		}
	      dx = dx > 0 ? (cosinus[6-i] * radius) >> 8 : - ((cosinus[6-i] * radius) >> 8);
	      dy = dy > 0 ? (cosinus[i] * radius)   >> 8 : - ((cosinus[i] * radius)   >> 8);
	    }
	  paint_tool->curx = paint_tool->lastx + dx;
	  paint_tool->cury = paint_tool->lasty + dy;
	}
    }

  tool->state        = ACTIVE;
  tool->gdisp        = gdisp;
  tool->paused_count = 0;

  /*  pause the current selection and grab the pointer  */
  gdisplays_selection_visibility (gdisp->gimage, SelectionPause);

  /* add motion memory if perfectmouse is set */
  if (perfectmouse != 0)
    gdk_pointer_grab (gdisp->canvas->window, FALSE,
		      GDK_BUTTON1_MOTION_MASK |
		      GDK_BUTTON_RELEASE_MASK,
		      NULL, NULL, bevent->time);
  else
    gdk_pointer_grab (gdisp->canvas->window, FALSE,
		      GDK_POINTER_MOTION_HINT_MASK |
		      GDK_BUTTON1_MOTION_MASK |
		      GDK_BUTTON_RELEASE_MASK,
		      NULL, NULL, bevent->time);

  /*  Let the specific painting function initialize itself  */
  gimp_paint_tool_paint (paint_tool, drawable, INIT_PAINT);

  if (paint_tool->pick_colors
      && !(bevent->state & GDK_SHIFT_MASK)
      && (bevent->state & (GDK_CONTROL_MASK | GDK_MOD1_MASK)))
    {
      gimp_paint_tool_sample_color (drawable, x, y, bevent->state);
      paint_tool->pick_state = TRUE;
      return;
    }
  else
    paint_tool->pick_state = FALSE;

  /*  store the current brush pointer  */
  current_brush = paint_tool->brush;

  /*  Paint to the image  */
  if (draw_line)
    {
      gimp_draw_tool_pause (GIMP_DRAW_TOOL(paint_tool));
      gimp_paint_tool_interpolate (paint_tool, drawable);

      paint_tool->lastx        = paint_tool->curx;
      paint_tool->lasty        = paint_tool->cury;
      paint_tool->lastpressure = paint_tool->curpressure;
      paint_tool->lastxtilt    = paint_tool->curxtilt;
      paint_tool->lastytilt    = paint_tool->curytilt;
#ifdef GTK_HAVE_SIX_VALUATORS
      paint_tool->lastwheel    = paint_tool->curwheel;
#endif /* GTK_HAVE_SIX_VALUATORS */
    }
  else
    {
      /* If we current point == last point, check if the brush
       * wants to be painted in that case. (Direction dependent
       * pixmap brush pipes don't, as they don't know which
       * pixmap to select.)
       */
      if (paint_tool->lastx != paint_tool->curx
	  || paint_tool->lasty != paint_tool->cury
	  || (* GIMP_BRUSH_CLASS (GTK_OBJECT (paint_tool->brush)
				  ->klass)->want_null_motion) (paint_tool))
	{
	  if (paint_tool->flags & TOOL_CAN_HANDLE_CHANGING_BRUSH)
	    paint_tool->brush =
	      (* GIMP_BRUSH_CLASS (GTK_OBJECT (paint_tool->brush)
				   ->klass)->select_brush) (paint_tool);

	  gimp_paint_tool_paint (paint_tool, drawable, MOTION_PAINT);
	}
    }

  if (paint_tool->flags & TOOL_TRACES_ON_WINDOW)
    gimp_paint_tool_paint (paint_tool, drawable, PRETRACE_PAINT);

  gdisplay_flush_now (gdisp);

  if (paint_tool->flags & TOOL_TRACES_ON_WINDOW)
    gimp_paint_tool_paint (paint_tool, drawable, POSTTRACE_PAINT);

  /*  restore the current brush pointer  */
  paint_tool->brush = current_brush;
}

static void
gimp_paint_tool_button_release (GimpTool       *tool,
				GdkEventButton *bevent,
				GDisplay       *gdisp)
{
  GimpImage     *gimage;
  GimpPaintTool *paint_tool;

  gimage     = gdisp->gimage;
  paint_tool = GIMP_PAINT_TOOL (tool);

  /*  resume the current selection and ungrab the pointer  */
  gdisplays_selection_visibility (gdisp->gimage, SelectionResume);

  gdk_pointer_ungrab (bevent->time);
  gdk_flush ();

  /*  Let the specific painting function finish up  */
  gimp_paint_tool_paint (paint_tool,
			 gimp_image_active_drawable (gdisp->gimage),
			 FINISH_PAINT);

  /*  Set tool state to inactive -- no longer painting */
  gimp_draw_tool_stop (GIMP_DRAW_TOOL (paint_tool));
  tool->state = INACTIVE;

  paint_tool->pick_state = FALSE;

  gimp_paint_tool_finish (paint_tool,
			  gimp_image_active_drawable (gdisp->gimage));

  gdisplays_flush ();
}

static void
gimp_paint_tool_motion (GimpTool       *tool,
			GdkEventMotion *mevent,
			GDisplay       *gdisp)
{
  GimpPaintTool *paint_tool;

  paint_tool = GIMP_PAINT_TOOL (tool);

  gdisplay_untransform_coords_f (gdisp, (double) mevent->x, (double) mevent->y,
				 &paint_tool->curx, &paint_tool->cury, TRUE);

  if (paint_tool->pick_state)
    {
      gimp_paint_tool_sample_color (gimp_image_active_drawable (gdisp->gimage),
				    paint_tool->curx, paint_tool->cury,
				    mevent->state);
      return;
    }

  paint_tool->curpressure = mevent->pressure;
  paint_tool->curxtilt    = mevent->xtilt;
  paint_tool->curytilt    = mevent->ytilt;
#ifdef GTK_HAVE_SIX_VALUATORS
  paint_tool->curwheel    = mevent->wheel;
#endif /* GTK_HAVE_SIX_VALUATORS */
  paint_tool->state       = mevent->state;

  gimp_paint_tool_interpolate (paint_tool,
			  gimp_image_active_drawable (gdisp->gimage));

  if (paint_tool->flags & TOOL_TRACES_ON_WINDOW)
    gimp_paint_tool_paint(paint_tool,
				gimp_image_active_drawable (gdisp->gimage),
				PRETRACE_PAINT);

  gdisplay_flush_now (gdisp);

  if (paint_tool->flags & TOOL_TRACES_ON_WINDOW)
    gimp_paint_tool_paint(paint_tool,
				gimp_image_active_drawable (gdisp->gimage),
				POSTTRACE_PAINT);

  paint_tool->lastx        = paint_tool->curx;
  paint_tool->lasty        = paint_tool->cury;
  paint_tool->lastpressure = paint_tool->curpressure;
  paint_tool->lastxtilt    = paint_tool->curxtilt;
  paint_tool->lastytilt    = paint_tool->curytilt;
#ifdef GTK_HAVE_SIX_VALUATORS
  paint_tool->lastwheel    = paint_tool->curwheel;
#endif /* GTK_HAVE_SIX_VALUATORS */
}


/* FIXME: this belongs in the individual tools */
static void
gimp_paint_tool_cursor_update (GimpTool       *tool,
			       GdkEventMotion *mevent,
			       GDisplay       *gdisp)
{
  GimpLayer     *layer;
  GimpPaintTool *paint_tool;
  GimpDrawTool  *draw_tool;
  gint           x, y;
  gchar          status_str[STATUSBAR_SIZE];

  GdkCursorType      ctype     = GIMP_MOUSE_CURSOR;
  GimpCursorModifier cmodifier = GIMP_CURSOR_MODIFIER_NONE;
  gboolean           ctoggle   = FALSE;

  paint_tool = GIMP_PAINT_TOOL (tool);
  draw_tool  = GIMP_DRAW_TOOL (tool);

  /*  undraw the current tool  */
  gimp_draw_tool_pause (draw_tool);

  if (gdisp != tool->gdisp || paint_tool->context_id < 1)
    {
      /* initialize the statusbar display */
      paint_tool->context_id =
	gtk_statusbar_get_context_id (GTK_STATUSBAR (gdisp->statusbar), "paint");
    }

  if (paint_tool->context_id)
    gtk_statusbar_pop (GTK_STATUSBAR (gdisp->statusbar), paint_tool->context_id);

#warning this doesnt belong here
  /* Set toggle cursors for various paint tools */
  if (tool->toggled)
    {
      GtkType type;

      type = GTK_OBJECT (tool)->klass->type;

      if (type == GIMP_TYPE_ERASER_TOOL)
	{
	  ctype     = GIMP_MOUSE_CURSOR;
	  cmodifier = GIMP_CURSOR_MODIFIER_MINUS;
	}
      else if (type == GIMP_TYPE_CONVOLVE_TOOL)
	{
	  ctype     = GIMP_MOUSE_CURSOR;
	  cmodifier = GIMP_CURSOR_MODIFIER_MINUS;
	}
      else if (type == GIMP_TYPE_DODGEBURN_TOOL)
	{
	  ctype   = GIMP_MOUSE_CURSOR;
	  ctoggle = TRUE;
	}
      else
	{
	  ctype = GIMP_MOUSE_CURSOR;
	}
    }

  if ((layer = gimp_image_get_active_layer (gdisp->gimage)))
    {
      /* If shift is down and this is not the first paint stroke, draw a line */
      if (gdisp == tool->gdisp && (mevent->state & GDK_SHIFT_MASK))
	{
	  gdouble dx, dy, d;

	  /*  Get the current coordinates */
	  gdisplay_untransform_coords_f (gdisp,
					 (double) mevent->x,
					 (double) mevent->y,
					 &paint_tool->curx,
					 &paint_tool->cury, TRUE);

	  dx = paint_tool->curx - paint_tool->lastx;
	  dy = paint_tool->cury - paint_tool->lasty;

	  /* Restrict to multiples of 15 degrees if ctrl is pressed */
	  if (mevent->state & GDK_CONTROL_MASK)
	    {
	      gint idx = dx;
	      gint idy = dy;
	      gint tangens2[6] = {  34, 106, 196, 334, 618, 1944 };
	      gint cosinus[7]  = { 256, 247, 222, 181, 128,   66, 0 };
	      gint i, radius, frac;

	      if (idy)
		{
		  radius = sqrt (SQR (idx) + SQR (idy));
		  frac = abs ((idx << 8) / idy);
		  for (i = 0; i < 6; i++)
		    {
		      if (frac < tangens2[i])
			break;
		    }
		  dx = idx > 0 ? (cosinus[6-i] * radius) >> 8 : - ((cosinus[6-i] * radius) >> 8);
		  dy = idy > 0 ? (cosinus[i] * radius) >> 8 : - ((cosinus[i] * radius) >> 8);
		}

	      paint_tool->curx = paint_tool->lastx + dx;
	      paint_tool->cury = paint_tool->lasty + dy;
	    }

	  /*  show distance in statusbar  */
	  if (gdisp->dot_for_dot)
	    {
	      d = sqrt (SQR (dx) + SQR (dy));
	      g_snprintf (status_str, STATUSBAR_SIZE, "%.1f %s", d, _("pixels"));
	    }
	  else
	    {
	      gchar *format_str =
		g_strdup_printf ("%%.%df %s",
				 gimp_unit_get_digits (gdisp->gimage->unit),
				 gimp_unit_get_symbol (gdisp->gimage->unit));
	      d = (gimp_unit_get_factor (gdisp->gimage->unit) *
		   sqrt (SQR (dx / gdisp->gimage->xresolution) +
			 SQR (dy / gdisp->gimage->yresolution)));

	      g_snprintf (status_str, STATUSBAR_SIZE, format_str, d);
	      g_free (format_str);
	    }

	  gtk_statusbar_push (GTK_STATUSBAR (gdisp->statusbar),
			      paint_tool->context_id, status_str);

	  if (draw_tool->gc == NULL)
	    {
	      gimp_draw_tool_start (draw_tool, gdisp->canvas->window);
	    }
	  else
	    {
	      /* is this a bad hack ? */
	      draw_tool->paused_count = 0;
	      gimp_draw_tool_resume (draw_tool);
	    }
	}
      /* If Ctrl or Mod1 is pressed, pick colors */
      else if (paint_tool->pick_colors &&
	       !(mevent->state & GDK_SHIFT_MASK) &&
	       (mevent->state & (GDK_CONTROL_MASK | GDK_MOD1_MASK)))
        {
	  ctype = GIMP_COLOR_PICKER_CURSOR;
	}
      /* Normal operation -- no modifier pressed or first stroke */
      else
	{
	  gint off_x, off_y;

	  gimp_drawable_offsets (GIMP_DRAWABLE (layer), &off_x, &off_y);
	  gdisplay_untransform_coords (gdisp,
				       (double) mevent->x, (double) mevent->y,
				       &x, &y, TRUE, FALSE);

	  if (x >= off_x && y >= off_y &&
	      x < (off_x + gimp_drawable_width (GIMP_DRAWABLE (layer))) &&
	      y < (off_y + gimp_drawable_height (GIMP_DRAWABLE (layer))))
	    {
	      /*  One more test--is there a selected region?
	       *  if so, is cursor inside?
	       */
	      if (gimage_mask_is_empty (gdisp->gimage))
		ctype = GIMP_MOUSE_CURSOR;
	      else if (gimage_mask_value (gdisp->gimage, x, y))
		ctype = GIMP_MOUSE_CURSOR;
	    }
	}

      gdisplay_install_tool_cursor (gdisp,
				    ctype,
				    ctype == GIMP_COLOR_PICKER_CURSOR ?
				    GIMP_COLOR_PICKER_TOOL_CURSOR :
				    ctoggle ?
				    tool->toggle_cursor : tool->tool_cursor,
				    cmodifier);
    }
}

static void
gimp_paint_tool_draw (GimpDrawTool *draw_tool)
{
  GDisplay      *gdisp;
  GimpPaintTool *paint_tool;
  GimpTool      *tool;
  gint           tx1, ty1, tx2, ty2;

  paint_tool = GIMP_PAINT_TOOL (draw_tool);
  tool       = GIMP_TOOL (draw_tool);

  /* if shift was never used, draw_tool->gc is NULL
     and we don't care about a redraw                       */
  if (draw_tool->gc != NULL)
    {
      gdisp = tool->gdisp;

      gdisplay_transform_coords (gdisp, paint_tool->lastx, paint_tool->lasty,
				 &tx1, &ty1, 1);
      gdisplay_transform_coords (gdisp, paint_tool->curx, paint_tool->cury,
				 &tx2, &ty2, 1);

      /*  Only draw line if it's in the visible area
       *  thus preventing from drawing rubbish
       */

      if (tx2 > 0 && ty2 > 0)
        {
	  gint offx, offy;

	  /* Adjust coords to start drawing from center of pixel if zoom > 1 */
	  offx = (int) SCALEFACTOR_X (gdisp) >> 1;
	  offy = (int) SCALEFACTOR_Y (gdisp) >> 1;
	  tx1 += offx;
	  ty1 += offy;
	  tx2 += offx;
	  ty2 += offy;

	  /*  Draw start target  */
	  gdk_draw_line (gdisp->canvas->window, draw_tool->gc,
			tx1 - (TARGET_WIDTH >> 1), ty1,
			tx1 + (TARGET_WIDTH >> 1), ty1);
	  gdk_draw_line (gdisp->canvas->window, draw_tool->gc,
			tx1, ty1 - (TARGET_HEIGHT >> 1),
			tx1, ty1 + (TARGET_HEIGHT >> 1));

	  /*  Draw end target  */
	  gdk_draw_line (gdisp->canvas->window, draw_tool->gc,
			tx2 - (TARGET_WIDTH >> 1), ty2,
			tx2 + (TARGET_WIDTH >> 1), ty2);
	  gdk_draw_line (gdisp->canvas->window, draw_tool->gc,
			tx2, ty2 - (TARGET_HEIGHT >> 1),
			tx2, ty2 + (TARGET_HEIGHT >> 1));

	  /*  Draw the line between the start and end coords  */
	  gdk_draw_line (gdisp->canvas->window, draw_tool->gc,
			tx1, ty1, tx2, ty2);
	}
    }
  return;
}	

/***********************************************************************/

static void
gimp_paint_tool_sample_color (GimpDrawable *drawable,
			      gint          x,
			      gint          y,
			      gint          state)
{
  GimpRGB  color;
  guchar  *col;

  if( x >= 0 && x < gimp_drawable_width (drawable) &&
      y >= 0 && y < gimp_drawable_height (drawable))
    {
      if ((col = gimp_drawable_get_color_at (drawable, x, y)))
	{
	  gimp_rgba_set_uchar (&color,
			       col[RED_PIX],
			       col[GREEN_PIX],
			       col[BLUE_PIX],
			       255);

	  if ((state & GDK_CONTROL_MASK))
	    gimp_context_set_foreground (gimp_context_get_user (), &color);
	  else
	    gimp_context_set_background (gimp_context_get_user (), &color);

	  g_free (col);
	}
    }
}

void
gimp_paint_tool_paint (GimpPaintTool *tool,
		       GimpDrawable  *drawable,
		       PaintState     state)
{
  gtk_signal_emit (GTK_OBJECT(tool), gimp_paint_tool_signals[PAINT],
		   drawable, state);
}

gboolean
gimp_paint_tool_start (GimpPaintTool *paint_tool,
		       GimpDrawable  *drawable,
		       gdouble        x,
		       gdouble        y)
{
  static GimpBrush *brush = NULL;

  paint_tool->curx = x;
  paint_tool->cury = y;

#warning (FIX non-gui paint tools)
#if 0
  /* Set up some defaults for non-gui use */
  if (paint_tool == &non_gui_paint_tool)
    {
      paint_tool->startpressure = paint_tool->lastpressure = paint_tool->curpressure = 0.5;
      paint_tool->startxtilt = paint_tool->lastxtilt = paint_tool->curxtilt = 0;
      paint_tool->startytilt = paint_tool->lastytilt = paint_tool->curytilt = 0;
#ifdef GTK_HAVE_SIX_VALUATORS
      paint_tool->startwheel = paint_tool->lastwheel = paint_tool->curwheel = 0.5;
#endif /* GTK_HAVE_SIX_VALUATORS */
    }
#endif

  /*  Each buffer is the same size as the maximum bounds of the active brush... */
  if (brush && brush != gimp_context_get_brush (NULL))
    {
      gtk_signal_disconnect_by_func (GTK_OBJECT (brush),
				     GTK_SIGNAL_FUNC (gimp_paint_tool_invalidate_cache),
				     NULL);
      gtk_object_unref (GTK_OBJECT (brush));
    }
  if (!(brush = gimp_context_get_brush (NULL)))
    {
      g_message (_("No brushes available for use with this tool."));
      return FALSE;
    }

  gtk_object_ref (GTK_OBJECT (brush));
  gtk_signal_connect (GTK_OBJECT (brush), "invalidate_preview",
		      GTK_SIGNAL_FUNC (gimp_paint_tool_invalidate_cache),
		      NULL);

  paint_tool->spacing = (double) gimp_brush_get_spacing (brush) / 100.0;

  paint_tool->brush = brush;

  /*  free the block structures  */
  if (undo_tiles)
    tile_manager_destroy (undo_tiles);
  if (canvas_tiles)
    tile_manager_destroy (canvas_tiles);

  /*  Allocate the undo structure  */
  undo_tiles = tile_manager_new (gimp_drawable_width (drawable),
				 gimp_drawable_height (drawable),
				 gimp_drawable_bytes (drawable));

  /*  Allocate the canvas blocks structure  */
  canvas_tiles = tile_manager_new (gimp_drawable_width (drawable),
				   gimp_drawable_height (drawable), 1);

  /*  Get the initial undo extents  */
  paint_tool->x1         = paint_tool->x2 = paint_tool->curx;
  paint_tool->y1         = paint_tool->y2 = paint_tool->cury;
  paint_tool->distance   = 0.0;
  paint_tool->pixel_dist = 0.0;

  return TRUE;
}

void
gimp_paint_tool_interpolate (GimpPaintTool *paint_tool,
			     GimpDrawable  *drawable)
{
  GimpBrush   *current_brush;
  GimpVector2  delta;
  gdouble      dpressure, dxtilt, dytilt;
#ifdef GTK_HAVE_SIX_VALUATORS
  gdouble      dwheel;
#endif /* GTK_HAVE_SIX_VALUATORS */
  /*   double spacing; */
  /*   double lastscale, curscale; */
  gdouble n;
  gdouble left;
  gdouble t;
  gdouble initial;
  gdouble dist;
  gdouble total;
  gdouble pixel_dist;
  gdouble pixel_initial;
  gdouble xd, yd;
  gdouble mag;

  delta.x   = paint_tool->curx        - paint_tool->lastx;
  delta.y   = paint_tool->cury        - paint_tool->lasty;
  dpressure = paint_tool->curpressure - paint_tool->lastpressure;
  dxtilt    = paint_tool->curxtilt    - paint_tool->lastxtilt;
  dytilt    = paint_tool->curytilt    - paint_tool->lastytilt;
#ifdef GTK_HAVE_SIX_VALUATORS
  dwheel    = paint_tool->curwheel    - paint_tool->lastwheel;
#endif /* GTK_HAVE_SIX_VALUATORS */

/* return if there has been no motion */
#ifdef GTK_HAVE_SIX_VALUATORS
  if (!delta.x && !delta.y && !dpressure && !dxtilt && !dytilt && !dwheel)
#else /* !GTK_HAVE_SIX_VALUATORS */
  if (!delta.x && !delta.y && !dpressure && !dxtilt && !dytilt)
#endif /* GTK_HAVE_SIX_VALUATORS */
    return;

  /* calculate the distance traveled in the coordinate space of the brush */
  mag = gimp_vector2_length (&(paint_tool->brush->x_axis));
  xd  = gimp_vector2_inner_product (&delta,
				    &(paint_tool->brush->x_axis)) / (mag*mag);

  mag = gimp_vector2_length (&(paint_tool->brush->y_axis));
  yd  = gimp_vector2_inner_product (&delta,
				    &(paint_tool->brush->y_axis)) / (mag*mag);

  dist    = 0.5 * sqrt (xd*xd + yd*yd);
  total   = dist + paint_tool->distance;
  initial = paint_tool->distance;

  pixel_dist    = gimp_vector2_length (&delta);
  pixel_initial = paint_tool->pixel_dist;

  /*  FIXME: need to adapt the spacing to the size  */
  /*   lastscale = MIN (gimp_paint_tool->lastpressure, 1/256); */
  /*   curscale = MIN (gimp_paint_tool->curpressure,  1/256); */
  /*   spacing = gimp_paint_tool->spacing * sqrt (0.5 * (lastscale + curscale)); */

  while (paint_tool->distance < total)
    {
      n = (gint) (paint_tool->distance / paint_tool->spacing + 1.0 + EPSILON);
      left = n * paint_tool->spacing - paint_tool->distance;

      paint_tool->distance += left;

      if (paint_tool->distance <= (total+EPSILON))
	{
	  t = (paint_tool->distance - initial) / dist;

	  paint_tool->curx        = paint_tool->lastx + delta.x * t;
	  paint_tool->cury        = paint_tool->lasty + delta.y * t;
	  paint_tool->pixel_dist  = pixel_initial + pixel_dist * t;
	  paint_tool->curpressure = paint_tool->lastpressure + dpressure * t;
	  paint_tool->curxtilt    = paint_tool->lastxtilt + dxtilt * t;
	  paint_tool->curytilt    = paint_tool->lastytilt + dytilt * t;
#ifdef GTK_HAVE_SIX_VALUATORS
          paint_tool->curwheel    = paint_tool->lastwheel + dwheel * t;
#endif /* GTK_HAVE_SIX_VALUATORS */

	  /*  save the current brush  */
	  current_brush = paint_tool->brush;

	  if (paint_tool->flags & TOOL_CAN_HANDLE_CHANGING_BRUSH)
	    paint_tool->brush     =
	      (* GIMP_BRUSH_CLASS (GTK_OBJECT (paint_tool->brush)
				   ->klass)->select_brush) (paint_tool);
	  gimp_paint_tool_paint(paint_tool, drawable, MOTION_PAINT);

	  /*  restore the current brush pointer  */
	  paint_tool->brush = current_brush;
	}
    }

  paint_tool->distance    = total;
  paint_tool->pixel_dist  = pixel_initial + pixel_dist;
  paint_tool->curx        = paint_tool->lastx + delta.x;
  paint_tool->cury        = paint_tool->lasty + delta.y;
  paint_tool->curpressure = paint_tool->lastpressure + dpressure;
  paint_tool->curxtilt    = paint_tool->lastxtilt + dxtilt;
  paint_tool->curytilt    = paint_tool->lastytilt + dytilt;
#ifdef GTK_HAVE_SIX_VALUATORS
  paint_tool->curwheel    = paint_tool->lastwheel + dwheel;
#endif /* GTK_HAVE_SIX_VALUATORS */
}

void
gimp_paint_tool_finish (GimpPaintTool *paint_tool,
			GimpDrawable  *drawable)
{
  GImage    *gimage;
  PaintUndo *pu;

  if (! (gimage = gimp_drawable_gimage (drawable)))
    return;

  /*  Determine if any part of the image has been altered--
   *  if nothing has, then just return...
   */

  if ((paint_tool->x2 == paint_tool->x1) ||
      (paint_tool->y2 == paint_tool->y1))
    return;

  undo_push_group_start (gimage, PAINT_CORE_UNDO);

  pu = g_new (PaintUndo, 1);
  pu->tool         = paint_tool;
  pu->lastx        = paint_tool->startx;
  pu->lasty        = paint_tool->starty;
  pu->lastpressure = paint_tool->startpressure;
  pu->lastxtilt    = paint_tool->startxtilt;
  pu->lastytilt    = paint_tool->startytilt;
#ifdef GTK_HAVE_SIX_VALUATORS
  pu->lastwheel    = paint_tool->startwheel;
#endif /* GTK_HAVE_SIX_VALUATORS */

  /*  Push a paint undo  */
  undo_push_paint (gimage, pu);

  /*  push an undo  */
  drawable_apply_image (drawable, paint_tool->x1, paint_tool->y1,
			paint_tool->x2, paint_tool->y2, undo_tiles, TRUE);
  undo_tiles = NULL;

  /*  push the group end  */
  undo_push_group_end (gimage);

  /*  invalidate the drawable--have to do it here, because
   *  it is not done during the actual painting.
   */
  gimp_viewable_invalidate_preview (GIMP_VIEWABLE (drawable));
}

void
gimp_paint_tool_cleanup (void)
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

void
gimp_paint_tool_get_color_from_gradient (GimpPaintTool     *paint_tool,
					 gdouble            gradient_length,
					 GimpRGB           *color,
					 GradientPaintMode  mode)
{
  gdouble y;
  gdouble distance;  /* distance in current brush stroke */

  distance = paint_tool->pixel_dist;
  y = ((double) distance / gradient_length);

  /* for the once modes, set y close to 1.0 after the first chunk */
  if ( (mode == ONCE_FORWARD || mode == ONCE_BACKWARDS) && y >= 1.0 )
    y = 0.9999999;

  if ( (((int)y & 1) && mode != LOOP_SAWTOOTH) || mode == ONCE_BACKWARDS )
    y = 1.0 - (y - (int)y);
  else
    y = y - (int)y;

  gimp_gradient_get_color_at (gimp_context_get_gradient (NULL), y, color);
}


/************************/
/*  Painting functions  */
/************************/

TempBuf *
gimp_paint_tool_get_paint_area (GimpPaintTool *paint_tool,
				GimpDrawable  *drawable,
				gdouble        scale)
{
  gint x, y;
  gint x1, y1, x2, y2;
  gint bytes;
  gint dwidth, dheight;
  gint bwidth, bheight;

  bytes = gimp_drawable_has_alpha (drawable) ?
    gimp_drawable_bytes (drawable) : gimp_drawable_bytes (drawable) + 1;

  gimp_paint_tool_calculate_brush_size (paint_tool->brush->mask, scale,
				   &bwidth, &bheight);

  /*  adjust the x and y coordinates to the upper left corner of the brush  */
  x = (gint) floor (paint_tool->curx) - (bwidth  >> 1);
  y = (gint) floor (paint_tool->cury) - (bheight >> 1);

  dwidth  = gimp_drawable_width  (drawable);
  dheight = gimp_drawable_height (drawable);

  x1 = CLAMP (x - 1, 0, dwidth);
  y1 = CLAMP (y - 1, 0, dheight);
  x2 = CLAMP (x + bwidth + 1, 0, dwidth);
  y2 = CLAMP (y + bheight + 1, 0, dheight);

  /*  configure the canvas buffer  */
  if ((x2 - x1) && (y2 - y1))
    canvas_buf = temp_buf_resize (canvas_buf, bytes, x1, y1,
				  (x2 - x1), (y2 - y1));
  else
    return NULL;

  return canvas_buf;
}

TempBuf *
gimp_paint_tool_get_orig_image (GimpPaintTool *paint_tool,
				GimpDrawable  *drawable,
				gint           x1,
				gint           y1,
				gint           x2,
				gint           y2)
{
  PixelRegion  srcPR;
  PixelRegion  destPR;
  Tile        *undo_tile;
  gint         h;
  gint         refd;
  gint         pixelwidth;
  gint         dwidth;
  gint         dheight;
  guchar      *s;
  guchar      *d;
  gpointer     pr;

  orig_buf = temp_buf_resize (orig_buf, gimp_drawable_bytes (drawable),
			      x1, y1, (x2 - x1), (y2 - y1));

  dwidth  = gimp_drawable_width  (drawable);
  dheight = gimp_drawable_height (drawable);

  x1 = CLAMP (x1, 0, dwidth);
  y1 = CLAMP (y1, 0, dheight);
  x2 = CLAMP (x2, 0, dwidth);
  y2 = CLAMP (y2, 0, dheight);

  /*  configure the pixel regions  */
  pixel_region_init (&srcPR, gimp_drawable_data (drawable), x1, y1,
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
gimp_paint_tool_paste_canvas (GimpPaintTool	   *paint_tool,
			      GimpDrawable	   *drawable,
			      gint		    brush_opacity,
			      gint		    image_opacity,
			      LayerModeEffects      paint_mode,
			      BrushApplicationMode  brush_hardness,
			      gdouble               brush_scale,
			      PaintApplicationMode  mode)
{
  MaskBuf *brush_mask;

  /*  get the brush mask  */
  brush_mask = gimp_paint_tool_get_brush_mask (paint_tool,
					       brush_hardness, brush_scale);

  /*  paste the canvas buf  */
  gimp_paint_tool_paste (paint_tool, brush_mask, drawable,
			 brush_opacity, image_opacity, paint_mode, mode);
}

/* Similar to gimp_paint_tool_paste_canvas, but replaces the alpha channel
   rather than using it to composite (i.e. transparent over opaque
   becomes transparent rather than opauqe. */
void
gimp_paint_tool_replace_canvas (GimpPaintTool        *paint_tool,
				GimpDrawable	     *drawable,
				gint                  brush_opacity,
				gint                  image_opacity,
				BrushApplicationMode  brush_hardness,
				gdouble               brush_scale,
				PaintApplicationMode  mode)
{
  MaskBuf *brush_mask;

  /*  get the brush mask  */
  brush_mask =
    gimp_paint_tool_get_brush_mask (paint_tool, brush_hardness, brush_scale);

  /*  paste the canvas buf  */
  gimp_paint_tool_replace (paint_tool, brush_mask, drawable,
			   brush_opacity, image_opacity, mode);
}


static void
gimp_paint_tool_invalidate_cache (GimpBrush *brush,
				  gpointer   data)
{
  /* Make sure we don't cache data for a brush that has changed */
  if (last_brush_mask == brush->mask)
    cache_invalid = TRUE;
}

/************************************************************
 *             LOCAL FUNCTION DEFINITIONS                   *
 ************************************************************/

static void
gimp_paint_tool_calculate_brush_size (MaskBuf *mask,
				      gdouble  scale,
				      gint    *width,
				      gint    *height)
{
  scale = CLAMP (scale, 0.0, 1.0);

  if (current_device == GDK_CORE_POINTER)
    {
      *width  = mask->width;
      *height = mask->height;
    }
  else
    {
      gdouble ratio;

      if (scale < 1 / 256)
	ratio = 1 / 16;
      else
	ratio = sqrt (scale);

      *width  = MAX ((gint) (mask->width  * ratio + 0.5), 1);
      *height = MAX ((gint) (mask->height * ratio + 0.5), 1);
    }
}

static MaskBuf *
gimp_paint_tool_subsample_mask (MaskBuf *mask,
				gdouble  x,
				gdouble  y)
{
  MaskBuf    *dest;
  gdouble     left;
  guchar     *m;
  guchar     *d;
  const gint *k;
  gint        index1;
  gint        index2;
  const gint *kernel;
  gint        new_val;
  gint        i, j;
  gint        r, s;

  x += (x < 0) ? mask->width : 0;
  left = x - floor (x);
  index1 = (gint) (left * (gdouble) (SUBSAMPLE + 1));

  y += (y < 0) ? mask->height : 0;
  left = y - floor (y);
  index2 = (gint) (left * (gdouble) (SUBSAMPLE + 1));

  kernel = subsample[index2][index1];

  if (mask == last_brush_mask && !cache_invalid)
    {
      if (kernel_brushes[index2][index1])
	return kernel_brushes[index2][index1];
    }
  else
    {
      for (i = 0; i <= SUBSAMPLE; i++)
	for (j = 0; j <= SUBSAMPLE; j++)
	  {
	    if (kernel_brushes[i][j])
	      mask_buf_free (kernel_brushes[i][j]);

	    kernel_brushes[i][j] = NULL;
	  }

      last_brush_mask = mask;
      cache_invalid = FALSE;
    }

  dest = kernel_brushes[index2][index1] = mask_buf_new (mask->width  + 2,
							mask->height + 2);

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
		  new_val = *d + ((*m * *k++ + 128) >> 8);
		  *d++ = MIN (new_val, 255);
		}
	    }
	  m++;
	}
    }

  return dest;
}

/* #define FANCY_PRESSURE */

static MaskBuf *
gimp_paint_tool_pressurize_mask (MaskBuf *brush_mask,
				 gdouble  x,
				 gdouble  y,
				 gdouble  pressure)
{
  static MaskBuf *last_brush = NULL;
  static guchar   mapi[256];
  guchar  *source;
  guchar  *dest;
  MaskBuf *subsample_mask;
  gint     i;
#ifdef FANCY_PRESSURE
  static gdouble map[256];
  gdouble        ds, s, c;
#endif

  /* Get the raw subsampled mask */
  subsample_mask = gimp_paint_tool_subsample_mask (brush_mask, x, y);

  /* Special case pressure = 0.5 */
  if ((int)(pressure * 100 + 0.5) == 50)
    return subsample_mask;

  /* Make sure we have the right sized buffer */
  if (brush_mask != last_brush)
    {
      if (pressure_brush)
	mask_buf_free (pressure_brush);

      pressure_brush = mask_buf_new (brush_mask->width  + 2,
				     brush_mask->height + 2);
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

  for (i = 0; i < 256; i++)
    {
      gint tmp = (pressure / 0.5) * i;

      if (tmp > 255)
	mapi[i] = 255;
      else
	mapi[i] = tmp;
    }

#endif /* FANCY_PRESSURE */

  /* Now convert the brush */

  source = mask_buf_data (subsample_mask);
  dest   = mask_buf_data (pressure_brush);

  i = subsample_mask->width * subsample_mask->height;
  while (i--)
    {
      *dest++ = mapi[(*source++)];
    }

  return pressure_brush;
}

static MaskBuf *
gimp_paint_tool_solidify_mask (MaskBuf *brush_mask)
{
  static MaskBuf *last_brush = NULL;

  gint    i;
  gint    j;
  guchar *data;
  guchar *src;

  if (brush_mask == last_brush && !cache_invalid)
    return solid_brush;

  last_brush = brush_mask;

  if (solid_brush)
    mask_buf_free (solid_brush);

  solid_brush = mask_buf_new (brush_mask->width + 2, brush_mask->height + 2);

  /*  get the data and advance one line into it  */
  data = mask_buf_data (solid_brush) + solid_brush->width;
  src  = mask_buf_data (brush_mask);

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
gimp_paint_tool_scale_mask (MaskBuf *brush_mask,
			    gdouble  scale)
{
  static MaskBuf *last_brush  = NULL;
  static gint     last_width  = 0.0;
  static gint     last_height = 0.0;
  gint dest_width;
  gint dest_height;

  scale = CLAMP (scale, 0.0, 1.0);

  if (scale == 0.0)
    return NULL;

  if (scale == 1.0)
    return brush_mask;

  gimp_paint_tool_calculate_brush_size (brush_mask, scale,
				   &dest_width, &dest_height);

  if (brush_mask == last_brush && !cache_invalid &&
      dest_width == last_width && dest_height == last_height)
    return scale_brush;

  if (scale_brush)
    mask_buf_free (scale_brush);

  last_brush  = brush_mask;
  last_width  = dest_width;
  last_height = dest_height;

  scale_brush = brush_scale_mask (brush_mask, dest_width, dest_height);
  cache_invalid = TRUE;

  return scale_brush;
}

static MaskBuf *
gimp_paint_tool_scale_pixmap (MaskBuf *brush_mask,
			      gdouble  scale)
{
  static MaskBuf *last_brush  = NULL;
  static gint     last_width  = 0.0;
  static gint     last_height = 0.0;
  gint dest_width;
  gint dest_height;

  scale = CLAMP (scale, 0.0, 1.0);

  if (scale == 0.0)
    return NULL;

  if (scale == 1.0)
    return brush_mask;

  gimp_paint_tool_calculate_brush_size (brush_mask, scale,
				   &dest_width, &dest_height);

  if (brush_mask == last_brush && !cache_invalid &&
      dest_width == last_width && dest_height == last_height)
    return scale_pixmap;

  if (scale_pixmap)
    mask_buf_free (scale_pixmap);

  last_brush  = brush_mask;
  last_width  = dest_width;
  last_height = dest_height;

  scale_pixmap = brush_scale_pixmap (brush_mask, dest_width, dest_height);
  cache_invalid = TRUE;

  return scale_pixmap;
}

static MaskBuf *
gimp_paint_tool_get_brush_mask (GimpPaintTool	     *paint_tool,
				BrushApplicationMode  brush_hardness,
				gdouble               scale)
{
  MaskBuf *mask;

  if (current_device == GDK_CORE_POINTER)
    mask = paint_tool->brush->mask;
  else
    mask = gimp_paint_tool_scale_mask (paint_tool->brush->mask, scale);

  switch (brush_hardness)
    {
    case SOFT:
      mask = gimp_paint_tool_subsample_mask (mask,
					     paint_tool->curx, paint_tool->cury);
      break;
    case HARD:
      mask = gimp_paint_tool_solidify_mask (mask);
      break;
    case PRESSURE:
      mask = gimp_paint_tool_pressurize_mask (mask,
					      paint_tool->curx, paint_tool->cury,
					      paint_tool->curpressure);
      break;
    default:
      break;
    }

  return mask;
}

static void
gimp_paint_tool_paste (GimpPaintTool        *paint_tool,
		       MaskBuf              *brush_mask,
		       GimpDrawable         *drawable,
		       gint                  brush_opacity,
		       gint                  image_opacity,
		       LayerModeEffects      paint_mode,
		       PaintApplicationMode  mode)
{
  GimpImage   *gimage;
  PixelRegion  srcPR;
  TileManager *alt = NULL;
  gint         offx;
  gint         offy;

  if (! (gimage = gimp_drawable_gimage (drawable)))
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

      brush_to_canvas_tiles (paint_tool, brush_mask, brush_opacity);
      canvas_tiles_to_canvas_buf (paint_tool);
      alt = undo_tiles;
    }
  /*  Otherwise:
   *   combine the canvas buf and the brush mask to the canvas buf
   */
  else
    {
      brush_to_canvas_buf (paint_tool, brush_mask, brush_opacity);
    }

  /*  intialize canvas buf source pixel regions  */
  srcPR.bytes = canvas_buf->bytes;
  srcPR.x = 0; srcPR.y = 0;
  srcPR.w = canvas_buf->width;
  srcPR.h = canvas_buf->height;
  srcPR.rowstride = canvas_buf->width * canvas_buf->bytes;
  srcPR.data = temp_buf_data (canvas_buf);

  /*  apply the paint area to the gimage  */
  gimp_image_apply_image (gimage, drawable, &srcPR,
			  FALSE, image_opacity, paint_mode,
			  alt,  /*  specify an alternative src1  */
			  canvas_buf->x, canvas_buf->y);

  /*  Update the undo extents  */
  paint_tool->x1 = MIN (paint_tool->x1, canvas_buf->x);
  paint_tool->y1 = MIN (paint_tool->y1, canvas_buf->y);
  paint_tool->x2 = MAX (paint_tool->x2, (canvas_buf->x + canvas_buf->width));
  paint_tool->y2 = MAX (paint_tool->y2, (canvas_buf->y + canvas_buf->height));

  /*  Update the gimage--it is important to call gdisplays_update_area
   *  instead of drawable_update because we don't want the drawable
   *  preview to be constantly invalidated
   */
  gimp_drawable_offsets (drawable, &offx, &offy);
  gdisplays_update_area (gimage, canvas_buf->x + offx, canvas_buf->y + offy,
			 canvas_buf->width, canvas_buf->height);
}

/* This works similarly to gimp_paint_tool_paste. However, instead of combining
   the canvas to the paint core drawable using one of the combination
   modes, it uses a "replace" mode (i.e. transparent pixels in the
   canvas erase the paint core drawable).

   When not drawing on alpha-enabled images, it just paints using NORMAL
   mode.
*/
static void
gimp_paint_tool_replace (GimpPaintTool        *paint_tool,
			 MaskBuf              *brush_mask,
			 GimpDrawable         *drawable,
			 gint                  brush_opacity,
			 gint                  image_opacity,
			 PaintApplicationMode  mode)
{
  GimpImage   *gimage;
  PixelRegion  srcPR;
  PixelRegion  maskPR;
  TileManager *alt = NULL;
  gint         offx;
  gint         offy;

  if (! gimp_drawable_has_alpha (drawable))
    {
      gimp_paint_tool_paste (paint_tool, brush_mask, drawable,
			     brush_opacity, image_opacity, NORMAL_MODE,
			     mode);
      return;
    }

  if (! (gimage = gimp_drawable_gimage (drawable)))
    return;

  /*  set undo blocks  */
  set_undo_tiles (drawable,
		  canvas_buf->x, canvas_buf->y,
		  canvas_buf->width, canvas_buf->height);

  if (mode == CONSTANT)
    {
      /*  initialize any invalid canvas tiles  */
      set_canvas_tiles (canvas_buf->x, canvas_buf->y,
			canvas_buf->width, canvas_buf->height);

      /* combine the brush mask and the canvas tiles */
      brush_to_canvas_tiles (paint_tool, brush_mask, brush_opacity);

      /* set the alt source as the unaltered undo_tiles */
      alt = undo_tiles;

      /* initialize the maskPR from the canvas tiles */
      pixel_region_init (&maskPR, canvas_tiles,
			 canvas_buf->x, canvas_buf->y,
			 canvas_buf->width, canvas_buf->height, FALSE);
    }
  else
    {
      /* The mask is just the brush mask */
      maskPR.bytes     = 1;
      maskPR.x         = 0;
      maskPR.y         = 0;
      maskPR.w         = canvas_buf->width;
      maskPR.h         = canvas_buf->height;
      maskPR.rowstride = maskPR.bytes * brush_mask->width;
      maskPR.data      = mask_buf_data (brush_mask);
    }

  /*  intialize canvas buf source pixel regions  */
  srcPR.bytes     = canvas_buf->bytes;
  srcPR.x         = 0;
  srcPR.y         = 0;
  srcPR.w         = canvas_buf->width;
  srcPR.h         = canvas_buf->height;
  srcPR.rowstride = canvas_buf->width * canvas_buf->bytes;
  srcPR.data      = temp_buf_data (canvas_buf);

  /*  apply the paint area to the gimage  */
  gimp_image_replace_image (gimage, drawable, &srcPR,
			    FALSE, image_opacity,
			    &maskPR,
			    canvas_buf->x, canvas_buf->y);

  /*  Update the undo extents  */
  paint_tool->x1 = MIN (paint_tool->x1, canvas_buf->x);
  paint_tool->y1 = MIN (paint_tool->y1, canvas_buf->y);
  paint_tool->x2 = MAX (paint_tool->x2, (canvas_buf->x + canvas_buf->width));
  paint_tool->y2 = MAX (paint_tool->y2, (canvas_buf->y + canvas_buf->height));

  /*  Update the gimage--it is important to call gdisplays_update_area
   *  instead of drawable_update because we don't want the drawable
   *  preview to be constantly invalidated
   */
  gimp_drawable_offsets (drawable, &offx, &offy);
  gdisplays_update_area (gimage, canvas_buf->x + offx, canvas_buf->y + offy,
			 canvas_buf->width, canvas_buf->height);
}

static void
canvas_tiles_to_canvas_buf (GimpPaintTool *gimp_paint_tool)
{
  PixelRegion srcPR;
  PixelRegion maskPR;

  /*  combine the canvas tiles and the canvas buf  */
  srcPR.bytes     = canvas_buf->bytes;
  srcPR.x         = 0;
  srcPR.y         = 0;
  srcPR.w         = canvas_buf->width;
  srcPR.h         = canvas_buf->height;
  srcPR.rowstride = canvas_buf->width * canvas_buf->bytes;
  srcPR.data      = temp_buf_data (canvas_buf);

  pixel_region_init (&maskPR, canvas_tiles,
		     canvas_buf->x, canvas_buf->y,
		     canvas_buf->width, canvas_buf->height, FALSE);

  /*  apply the canvas tiles to the canvas buf  */
  apply_mask_to_region (&srcPR, &maskPR, OPAQUE_OPACITY);
}

static void
brush_to_canvas_tiles (GimpPaintTool *paint_tool,
		       MaskBuf       *brush_mask,
		       gint          brush_opacity)
{
  PixelRegion srcPR;
  PixelRegion maskPR;
  gint        x;
  gint        y;
  gint        xoff;
  gint        yoff;

  /*   combine the brush mask and the canvas tiles  */
  pixel_region_init (&srcPR, canvas_tiles,
		     canvas_buf->x, canvas_buf->y,
		     canvas_buf->width, canvas_buf->height, TRUE);

  x = (gint) floor (paint_tool->curx) - (brush_mask->width  >> 1);
  y = (gint) floor (paint_tool->cury) - (brush_mask->height >> 1);
  xoff = (x < 0) ? -x : 0;
  yoff = (y < 0) ? -y : 0;

  maskPR.bytes     = 1;
  maskPR.x         = 0;
  maskPR.y         = 0;
  maskPR.w         = srcPR.w;
  maskPR.h         = srcPR.h;
  maskPR.rowstride = maskPR.bytes * brush_mask->width;
  maskPR.data      = mask_buf_data (brush_mask) + yoff * maskPR.rowstride + xoff * maskPR.bytes;

  /*  combine the mask to the canvas tiles  */
  combine_mask_and_region (&srcPR, &maskPR, brush_opacity);
}

static void
brush_to_canvas_buf (GimpPaintTool *paint_tool,
		     MaskBuf       *brush_mask,
		     gint           brush_opacity)
{
  PixelRegion srcPR;
  PixelRegion maskPR;
  gint        x;
  gint        y;
  gint        xoff;
  gint        yoff;

  x = (gint) floor (paint_tool->curx) - (brush_mask->width  >> 1);
  y = (gint) floor (paint_tool->cury) - (brush_mask->height >> 1);
  xoff = (x < 0) ? -x : 0;
  yoff = (y < 0) ? -y : 0;

  /*  combine the canvas buf and the brush mask to the canvas buf  */
  srcPR.bytes     = canvas_buf->bytes;
  srcPR.x         = 0;
  srcPR.y         = 0;
  srcPR.w         = canvas_buf->width;
  srcPR.h         = canvas_buf->height;
  srcPR.rowstride = canvas_buf->width * canvas_buf->bytes;
  srcPR.data      = temp_buf_data (canvas_buf);

  maskPR.bytes     = 1;
  maskPR.x         = 0;
  maskPR.y         = 0;
  maskPR.w         = srcPR.w;
  maskPR.h         = srcPR.h;
  maskPR.rowstride = maskPR.bytes * brush_mask->width;
  maskPR.data      = mask_buf_data (brush_mask) + yoff * maskPR.rowstride + xoff * maskPR.bytes;

  /*  apply the mask  */
  apply_mask_to_region (&srcPR, &maskPR, brush_opacity);
}

#if 0
static void
paint_to_canvas_tiles (GimpPaintTool *paint_tool,
		       MaskBuf       *brush_mask,
		       gint           brush_opacity)
{
  PixelRegion srcPR;
  PixelRegion maskPR;
  gint x;
  gint y;
  gint xoff;
  gint yoff;

  /*   combine the brush mask and the canvas tiles  */
  pixel_region_init (&srcPR, canvas_tiles,
		     canvas_buf->x, canvas_buf->y,
		     canvas_buf->width, canvas_buf->height, TRUE);

  x = (gint) floor (paint_tool->curx) - (brush_mask->width  >> 1);
  y = (gint) floor (paint_tool->cury) - (brush_mask->height >> 1);
  xoff = (x < 0) ? -x : 0;
  yoff = (y < 0) ? -y : 0;

  maskPR.bytes     = 1;
  maskPR.x         = 0;
  maskPR.y         = 0;
  maskPR.w         = srcPR.w;
  maskPR.h         = srcPR.h;
  maskPR.rowstride = maskPR.bytes * brush_mask->width;
  maskPR.data      = mask_buf_data (brush_mask) + yoff * maskPR.rowstride + xoff * maskPR.bytes;

  /*  combine the mask and canvas tiles  */
  combine_mask_and_region (&srcPR, &maskPR, brush_opacity);

  /*  combine the canvas tiles and the canvas buf  */
  srcPR.bytes     = canvas_buf->bytes;
  srcPR.x         = 0;
  srcPR.y         = 0;
  srcPR.w         = canvas_buf->width;
  srcPR.h         = canvas_buf->height;
  srcPR.rowstride = canvas_buf->width * canvas_buf->bytes;
  srcPR.data      = temp_buf_data (canvas_buf);

  pixel_region_init (&maskPR, canvas_tiles,
		     canvas_buf->x, canvas_buf->y,
		     canvas_buf->width, canvas_buf->height, FALSE);

  /*  apply the canvas tiles to the canvas buf  */
  apply_mask_to_region (&srcPR, &maskPR, OPAQUE_OPACITY);
}

static void
paint_to_canvas_buf (GimpPaintTool *paint_tool,
		     MaskBuf       *brush_mask,
		     gint           brush_opacity)
{
  PixelRegion srcPR;
  PixelRegion maskPR;
  gint x;
  gint y;
  gint xoff;
  gint yoff;

  x = (gint) floor (paint_tool->curx) - (brush_mask->width  >> 1);
  y = (gint) floor (paint_tool->cury) - (brush_mask->height >> 1);
  xoff = (x < 0) ? -x : 0;
  yoff = (y < 0) ? -y : 0;


  /*  combine the canvas buf and the brush mask to the canvas buf  */
  srcPR.bytes     = canvas_buf->bytes;
  srcPR.x         = 0;
  srcPR.y         = 0;
  srcPR.w         = canvas_buf->width;
  srcPR.h         = canvas_buf->height;
  srcPR.rowstride = canvas_buf->width * canvas_buf->bytes;
  srcPR.data      = temp_buf_data (canvas_buf);

  maskPR.bytes     = 1;
  maskPR.x         = 0;
  maskPR.y         = 0;
  maskPR.w         = srcPR.w;
  maskPR.h         = srcPR.h;
  maskPR.rowstride = maskPR.bytes * brush_mask->width;
  maskPR.data      = mask_buf_data (brush_mask) + yoff * maskPR.rowstride + xoff * maskPR.bytes;

  /*  apply the mask  */
  apply_mask_to_region (&srcPR, &maskPR, brush_opacity);
}
#endif

static void
set_undo_tiles (GimpDrawable *drawable,
		gint          x,
		gint          y,
		gint          w,
		gint          h)
{
  gint  i;
  gint  j;
  Tile *src_tile;
  Tile *dest_tile;

  if (undo_tiles == NULL)
    {
      g_warning ("set_undo_tiles: undo_tiles is null");
      return;
    }

  for (i = y; i < (y + h); i += (TILE_HEIGHT - (i % TILE_HEIGHT)))
    {
      for (j = x; j < (x + w); j += (TILE_WIDTH - (j % TILE_WIDTH)))
	{
	  dest_tile = tile_manager_get_tile (undo_tiles, j, i, FALSE, FALSE);
	  if (tile_is_valid (dest_tile) == FALSE)
	    {
	      src_tile = tile_manager_get_tile (gimp_drawable_data (drawable),
						j, i, TRUE, FALSE);
	      tile_manager_map_tile (undo_tiles, j, i, src_tile);
	      tile_release (src_tile, FALSE);
	    }
	}
    }
}

static void
set_canvas_tiles (gint x,
		  gint y,
		  gint w,
		  gint h)
{
  gint  i;
  gint  j;
  Tile *tile;

  for (i = y; i < (y + h); i += (TILE_HEIGHT - (i % TILE_HEIGHT)))
    {
      for (j = x; j < (x + w); j += (TILE_WIDTH - (j % TILE_WIDTH)))
	{
	  tile = tile_manager_get_tile (canvas_tiles, j, i, FALSE, FALSE);
	  if (tile_is_valid (tile) == FALSE)
	    {
	      tile = tile_manager_get_tile (canvas_tiles, j, i, TRUE, TRUE);
	      memset (tile_data_pointer (tile, 0, 0), 0, tile_size (tile));
	      tile_release (tile, TRUE);
	    }
	}
    }
}


/*****************************************************/
/*  Paint buffers utility functions                  */
/*****************************************************/


static void
free_paint_buffers (void)
{
  if (orig_buf)
    temp_buf_free (orig_buf);
  orig_buf = NULL;

  if (canvas_buf)
    temp_buf_free (canvas_buf);
  canvas_buf = NULL;
}


/**************************************************/
/*  Brush pipe utility functions                  */
/**************************************************/

void
gimp_paint_tool_color_area_with_pixmap (GimpPaintTool        *paint_tool,
					GimpImage            *dest,
					GimpDrawable         *drawable,
					TempBuf              *area,
					gdouble               scale,
					BrushApplicationMode  mode)
{
  PixelRegion destPR;
  void    *pr;
  guchar  *d;
  gint     ulx;
  gint     uly;
  gint     offsetx;
  gint     offsety;
  gint     y;
  TempBuf *pixmap_mask;
  TempBuf *brush_mask;

  g_return_if_fail (GIMP_IS_BRUSH (paint_tool->brush));
  g_return_if_fail (paint_tool->brush->pixmap != NULL);

  /*  scale the brushes  */
  pixmap_mask = gimp_paint_tool_scale_pixmap (paint_tool->brush->pixmap, scale);

  if (mode == SOFT)
    brush_mask = gimp_paint_tool_scale_mask (paint_tool->brush->mask, scale);
  else
    brush_mask = NULL;

  destPR.bytes     = area->bytes;
  destPR.x         = 0;
  destPR.y         = 0;
  destPR.w         = area->width;
  destPR.h         = area->height;
  destPR.rowstride = destPR.bytes * area->width;
  destPR.data      = temp_buf_data (area);
		
  pr = pixel_regions_register (1, &destPR);

  /* Calculate upper left corner of brush as in
   * gimp_paint_tool_get_paint_area.  Ugly to have to do this here, too.
   */

  ulx = (gint) floor (paint_tool->curx) - (pixmap_mask->width  >> 1);
  uly = (gint) floor (paint_tool->cury) - (pixmap_mask->height >> 1);

  offsetx = area->x - ulx;
  offsety = area->y - uly;

  for (; pr != NULL; pr = pixel_regions_process (pr))
    {
      d = destPR.data;
      for (y = 0; y < destPR.h; y++)
	{
	  paint_line_pixmap_mask (dest, drawable, pixmap_mask, brush_mask,
				  d, offsetx, y + offsety,
				  destPR.bytes, destPR.w, mode);
	  d += destPR.rowstride;
	}
    }
}

static void
paint_line_pixmap_mask (GimpImage            *dest,
			GimpDrawable         *drawable,
			TempBuf              *pixmap_mask,
			TempBuf              *brush_mask,
			guchar               *d,
			gint                  x,
			gint                  y,
			gint                  bytes,
			gint                  width,
			BrushApplicationMode  mode)
{
  guchar  *b;
  guchar  *p;
  guchar  *mask;
  gdouble  alpha;
  gdouble  factor = 0.00392156986;  /*  1.0 / 255.0  */
  gint     x_index;
  gint     i,byte_loop;

  /*  Make sure x, y are positive  */
  while (x < 0)
    x += pixmap_mask->width;
  while (y < 0)
    y += pixmap_mask->height;

  /* Point to the approriate scanline */
  b = temp_buf_data (pixmap_mask) +
    (y % pixmap_mask->height) * pixmap_mask->width * pixmap_mask->bytes;
    
  if (mode == SOFT && brush_mask)
    {
      /* ditto, except for the brush mask, so we can pre-multiply the alpha value */
      mask = temp_buf_data (brush_mask) +
	(y % brush_mask->height) * brush_mask->width;
      for (i = 0; i < width; i++)
	{
	  /* attempt to avoid doing this calc twice in the loop */
	  x_index = ((i + x) % pixmap_mask->width);
	  p = b + x_index * pixmap_mask->bytes;
	  d[bytes-1] = mask[x_index];
	  
	  /* multiply alpha into the pixmap data */
	  /* maybe we could do this at tool creation or brush switch time? */
	  /* and compute it for the whole brush at once and cache it?  */
	  alpha = d[bytes-1] * factor;
	  if(alpha)
	    for (byte_loop = 0; byte_loop < bytes - 1; byte_loop++) 
	      d[byte_loop] *= alpha;

	  /* printf("i: %i d->r: %i d->g: %i d->b: %i d->a: %i\n",i,(int)d[0], (int)d[1], (int)d[2], (int)d[3]); */
	  gimp_image_transform_color (dest, drawable, p, d, RGB);
	  d += bytes;
	}
    }
  else
    {
      for (i = 0; i < width; i++)
	{
	  /* attempt to avoid doing this calc twice in the loop */
	  x_index = ((i + x) % pixmap_mask->width);
	  p = b + x_index * pixmap_mask->bytes;
	  d[bytes-1] = 255;
	  
	  /* multiply alpha into the pixmap data */
	  /* maybe we could do this at tool creation or brush switch time? */
	  /* and compute it for the whole brush at once and cache it?  */
	  gimp_image_transform_color (dest, drawable, p, d, RGB);
	  d += bytes;
	}
    }
}
