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
#include "appenv.h"
#include "drawable.h"
#include "draw_core.h"
#include "gimage_mask.h"
#include "gimpbrushlist.h"
#include "gimprc.h"
#include "ink.h"
#include "tools.h"
#include "undo.h"
#include "blob.h"
#include "gdisplay.h"

#include "libgimp/gimpintl.h"

#include "tile.h"			/* ick. */

#include <math.h>
#include <stdlib.h>
#include <string.h>

#define SUBSAMPLE 8

/*  the Ink structures  */

typedef struct _InkTool InkTool;
struct _InkTool
{
  DrawCore *      core;         /*  Core select object          */
  
  Blob     *      last_blob;	/*  blob for last cursor position */

  int             x1, y1;       /*  image space coordinate      */
  int             x2, y2;       /*  image space coords          */

};

typedef struct _InkOptions InkOptions;
struct _InkOptions
{
  double       size;
  double       aspect;
  double       angle;
  double       sensitivity;
};

typedef struct _BrushWidget BrushWidget;
struct _BrushWidget
{
  gboolean     state;
};

/*  undo blocks variables  */
static TileManager *  undo_tiles = NULL;
static TileManager *  canvas_tiles = NULL;

/*  paint buffer */
static TempBuf *  canvas_buf = NULL;

/*  local variables  */
static InkOptions *ink_options = NULL;

/*  local function prototypes  */
static void   ink_button_press            (Tool *, GdkEventButton *, gpointer);
static void   ink_button_release          (Tool *, GdkEventButton *, gpointer);
static void   ink_motion                  (Tool *, GdkEventMotion *, gpointer);
static void   ink_cursor_update           (Tool *, GdkEventMotion *, gpointer);
static void   ink_control                 (Tool *, int, gpointer);

static InkOptions *create_ink_options   (void);
static Argument *ink_invoker              (Argument *);

static void ink_init   (InkTool      *ink_tool, 
			GimpDrawable *drawable, 
			double        x, 
			double        y);
static void ink_finish (InkTool      *ink_tool, 
			GimpDrawable *drawable, 
			int           tool_id);
static void ink_cleanup (void);

static void ink_scale_update              (GtkAdjustment  *adjustment, 
                                           gdouble        *value);
/* Rendering functions */

static void ink_set_paint_area  (InkTool      *ink_tool, 
				 GimpDrawable *drawable, 
				 Blob         *blob);
static void ink_paste           (InkTool      *ink_tool, 
				 GimpDrawable *drawable,
				 Blob         *blob);

static void ink_to_canvas_tiles (InkTool      *ink_tool,
				 Blob         *blob,
				 guchar       *color);

static void ink_set_undo_tiles  (GimpDrawable *drawable,
				 int           x, 
				 int           y,
				 int           w, 
				 int           h);
static void ink_set_canvas_tiles(int           x, 
				 int           y,
				 int           w, 
				 int           h);

/* Brush pseudo-widget callbacks */
static void brush_widget_active_rect      (BrushWidget    *brush_widget,
					   GtkWidget      *w, 
					   GdkRectangle   *rect);
static void brush_widget_realize          (GtkWidget      *w);
static void brush_widget_expose           (GtkWidget      *w, 
					   GdkEventExpose *event,
					   BrushWidget    *brush_widget);
static void brush_widget_button_press     (GtkWidget      *w, 
					   GdkEventButton *event,
					   BrushWidget    *brush_widget);
static void brush_widget_button_release   (GtkWidget      *w,  
					   GdkEventButton *event,
					   BrushWidget    *brush_widget);
static void brush_widget_motion_notify    (GtkWidget      *w, 
					   GdkEventMotion *event,
					   BrushWidget    *brush_widget);


static InkOptions *
create_ink_options ()
{
  GtkWidget *vbox;
  GtkWidget *hbox;
  GtkWidget *label;
  GtkWidget *slider;
  GtkWidget *aspect_frame;
  GtkWidget *darea;
  GtkAdjustment *adj;

  BrushWidget *brush_widget;
  InkOptions *options;

  /*  the new options structure  */
  options = (InkOptions *) g_malloc (sizeof (InkOptions));

  options->size = 3.0;
  options->sensitivity = 1.0;
  options->aspect = 1.0;
  options->angle = 0.0;

  /*  the main vbox  */
  vbox = gtk_vbox_new (FALSE, 1);

  /*  the main label  */
  label = gtk_label_new (_("Ink Options"));
  gtk_box_pack_start (GTK_BOX (vbox), label, FALSE, FALSE, 0);
  gtk_widget_show (label);

  /* size slider */
  hbox = gtk_hbox_new (FALSE, 2);
  gtk_box_pack_start (GTK_BOX(vbox), hbox, FALSE, FALSE, 0);
  label = gtk_label_new (_("Size:"));
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 2);
  
  adj = GTK_ADJUSTMENT (gtk_adjustment_new (3.0, 0.0, 20.0, 1.0, 5.0, 0.0));
  slider = gtk_hscale_new (adj);
  gtk_box_pack_start (GTK_BOX (hbox), slider, TRUE, TRUE, 0);
  gtk_scale_set_value_pos (GTK_SCALE (slider), GTK_POS_TOP);
  
  gtk_range_set_update_policy (GTK_RANGE (slider), GTK_UPDATE_DELAYED);
  gtk_signal_connect (GTK_OBJECT (adj), "value_changed",
		      (GtkSignalFunc) ink_scale_update,
		      &options->size);
  
  /* sens slider */
  hbox = gtk_hbox_new (FALSE, 2);
  gtk_box_pack_start (GTK_BOX(vbox), hbox, FALSE, FALSE, 0);
  label = gtk_label_new (_("Sensitivity:"));
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 2);
  
  adj = GTK_ADJUSTMENT (gtk_adjustment_new (1.0, 0.0, 1.0, 0.01, 0.1, 0.0));
  slider = gtk_hscale_new (adj);
  gtk_box_pack_start (GTK_BOX (hbox), slider, TRUE, TRUE, 0);
  gtk_scale_set_value_pos (GTK_SCALE (slider), GTK_POS_TOP);
  
  gtk_range_set_update_policy (GTK_RANGE (slider), GTK_UPDATE_DELAYED);
  gtk_signal_connect (GTK_OBJECT (adj), "value_changed",
		      (GtkSignalFunc) ink_scale_update,
		      &options->sensitivity);

  /* Brush shape widget */

  brush_widget = g_new (BrushWidget, 1);
  brush_widget->state = FALSE;
  
  hbox = gtk_hbox_new (FALSE, 2);
  gtk_box_pack_start (GTK_BOX(vbox), hbox, FALSE, FALSE, 5);
  label = gtk_label_new (_("Shape:"));
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
  
  aspect_frame = gtk_aspect_frame_new (NULL, 0.5, 0.5, 1.0, FALSE);
  gtk_frame_set_shadow_type (GTK_FRAME(aspect_frame), GTK_SHADOW_IN);
  gtk_box_pack_start (GTK_BOX (hbox), aspect_frame, TRUE, TRUE, 2);
  
  darea = gtk_drawing_area_new();
  gtk_drawing_area_size (GTK_DRAWING_AREA(darea), 60, 60);
  gtk_container_add (GTK_CONTAINER(aspect_frame), darea);
  
  gtk_widget_set_events (darea, 
			 GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK
			 | GDK_POINTER_MOTION_MASK | GDK_EXPOSURE_MASK);
  gtk_signal_connect (GTK_OBJECT (darea), "button_press_event",
		      GTK_SIGNAL_FUNC (brush_widget_button_press),
		      brush_widget);
  gtk_signal_connect (GTK_OBJECT (darea), "button_release_event",
		      GTK_SIGNAL_FUNC (brush_widget_button_release),
		      brush_widget);
  gtk_signal_connect (GTK_OBJECT (darea), "motion_notify_event",
		      GTK_SIGNAL_FUNC (brush_widget_motion_notify),
		      brush_widget);
  gtk_signal_connect (GTK_OBJECT (darea), "expose_event",
		      GTK_SIGNAL_FUNC (brush_widget_expose), 
		      brush_widget);
  gtk_signal_connect (GTK_OBJECT (darea), "realize",
		      GTK_SIGNAL_FUNC (brush_widget_realize),
		      brush_widget);
  gtk_widget_show_all (vbox);
  gtk_widget_hide (vbox);
  
  /*  Register this selection options widget with the main tools options dialog  */
  tools_register_options (INK, vbox);

  return options;
}

static void
brush_widget_active_rect (BrushWidget *brush_widget, GtkWidget *w,
			  GdkRectangle *rect)
{
  int x,y;
  int r;

  r = MIN(w->allocation.width,w->allocation.height)/2;

  x = w->allocation.width/2 + 0.85*r*ink_options->aspect/10. *
    cos (ink_options->angle);
  y = w->allocation.height/2 + 0.85*r*ink_options->aspect/10. *
    sin (ink_options->angle);

  rect->x = x-5;
  rect->y = y-5;
  rect->width = 10;
  rect->height = 10;
}

static void
brush_widget_realize (GtkWidget *w)
{
  gtk_style_set_background (w->style, w->window, GTK_STATE_ACTIVE);
}

static void
brush_widget_draw_brush (BrushWidget *brush_widget, GtkWidget *w,
			 double xc, double yc, double radius)
{
  int i;
  Blob *b;

  b = blob_ellipse(xc,yc,
		   radius*cos(ink_options->angle),
		   radius*sin(ink_options->angle),
		   -(radius/ink_options->aspect)*sin(ink_options->angle),
		   (radius/ink_options->aspect)*cos(ink_options->angle));

  for (i=0;i<b->height;i++)
    {
      if (b->data[i].left <= b->data[i].right)
	gdk_draw_line (w->window,
		       w->style->fg_gc[w->state],
		       b->data[i].left,i+b->y,
		       b->data[i].right+1,i+b->y);
    }
  
  g_free (b);
}

static void
brush_widget_expose (GtkWidget *w, GdkEventExpose *event,
		     BrushWidget *brush_widget)
{
  GdkRectangle rect;
  int r0;
  
  r0 = MIN(w->allocation.width,w->allocation.height)/2;

  if (r0 < 2)
    return;

  gdk_window_clear_area(w->window,
			0,0,w->allocation.width,
			w->allocation.height);
  brush_widget_draw_brush (brush_widget, w,
			   w->allocation.width/2, w->allocation.height/2,
			   0.9*r0);

  brush_widget_active_rect (brush_widget, w, &rect);
  gdk_draw_rectangle (w->window, w->style->bg_gc[GTK_STATE_NORMAL],
		      TRUE,	/* filled */
		      rect.x, rect.y, 
		      rect.width, rect.height);
  gtk_draw_shadow (w->style, w->window, w->state, GTK_SHADOW_OUT,
		   rect.x, rect.y,
		   rect.width, rect.height);
}

static void
brush_widget_button_press (GtkWidget *w, GdkEventButton *event,
			   BrushWidget *brush_widget)
{
  GdkRectangle rect;

  brush_widget_active_rect (brush_widget, w, &rect);
  
  if ((event->x >= rect.x) && (event->x-rect.x < rect.width) &&
      (event->y >= rect.y) && (event->y-rect.y < rect.height))
    {
      brush_widget->state = TRUE;
    }
}

static void
brush_widget_button_release (GtkWidget *w, GdkEventButton *event,
			    BrushWidget *brush_widget)
{
  brush_widget->state = FALSE;
}

static void
brush_widget_motion_notify (GtkWidget *w, GdkEventMotion *event,
			    BrushWidget *brush_widget)
{
  int x;
  int y;
  int r0;
  int rsquare;

  if (brush_widget->state)
    {
      x = event->x - w->allocation.width/2;
      y = event->y - w->allocation.height/2;
      rsquare = x*x + y*y;

      if (rsquare != 0)
	{
	  ink_options->angle = atan2(y,x);

	  r0 = MIN(w->allocation.width,w->allocation.height)/2;
	  ink_options->aspect =
	    10. * sqrt((double)rsquare/(r0*r0)) / 0.85;

	  if (ink_options->aspect < 1.0)
	    ink_options->aspect = 1.0;
	  if (ink_options->aspect > 10.0)
	    ink_options->aspect = 10.0;

	  gtk_widget_draw(w, NULL);
	}
    }
}

static void
ink_scale_update (GtkAdjustment *adjustment, gdouble *value)
{
  *value = adjustment->value;
}

static Blob *
ink_pen_ellipse (gdouble x_center, gdouble y_center,
		 gdouble pressure, gdouble xtilt, gdouble ytilt)
{
  double size;
  double tsin, tcos;
  double aspect, radmin;
  double x,y;
  
  size = ink_options->size * (1 + ink_options->sensitivity * (2*pressure - 1));
  if (size*SUBSAMPLE < 1) size = 1/SUBSAMPLE;

  /* Add brush angle/aspect to title vectorially */

  x = ink_options->aspect*cos(ink_options->angle) + xtilt*10.0;
  y = ink_options->aspect*sin(ink_options->angle) + ytilt*10.0;
  aspect = sqrt(x*x+y*y);

  if (aspect != 0)
    {
      tcos = x/aspect;
      tsin = y/aspect;
    }
  else
    {
      tsin = sin(ink_options->angle);
      tcos = cos(ink_options->angle);
    }
  
  if (aspect < 1.0) 
    aspect = 1.0;
  else if (aspect > 10.0) 
    aspect = 10.0;

  radmin = SUBSAMPLE * size/aspect;
  if (radmin < 1.0) radmin = 1.0;
  
  return blob_ellipse(x_center * SUBSAMPLE, y_center * SUBSAMPLE,
		      radmin*aspect*tcos, radmin*aspect*tsin,  
		      -radmin*tsin, radmin*tcos);
}

static void
ink_button_press (Tool           *tool,
		  GdkEventButton *bevent,
		  gpointer        gdisp_ptr)
{
  gdouble       x, y;
  GDisplay     *gdisp;
  InkTool      *ink_tool;
  GimpDrawable *drawable;
  Blob         *b;

  gdisp = (GDisplay *) gdisp_ptr;
  ink_tool = (InkTool *) tool->private;

  /*  Keep the coordinates of the target  */
  gdisplay_untransform_coords_f (gdisp, bevent->x, bevent->y,
				 &x, &y, TRUE);
  drawable = gimage_active_drawable (gdisp->gimage);

  ink_init (ink_tool, drawable, x, y);

  tool->state = ACTIVE;
  tool->gdisp_ptr = gdisp_ptr;
  tool->paused_count = 0;

  /*  pause the current selection and grab the pointer  */
  gdisplays_selection_visibility (gdisp->gimage, SelectionPause);

  /* add motion memory if you press mod1 first ^ perfectmouse */
  if (((bevent->state & GDK_MOD1_MASK) != 0) != (perfectmouse != 0))
    gdk_pointer_grab (gdisp->canvas->window, FALSE,
		      GDK_BUTTON1_MOTION_MASK | GDK_BUTTON_RELEASE_MASK,
		      NULL, NULL, bevent->time);
  else
    gdk_pointer_grab (gdisp->canvas->window, FALSE,
		      GDK_POINTER_MOTION_HINT_MASK | GDK_BUTTON1_MOTION_MASK |
		      GDK_BUTTON_RELEASE_MASK,
		      NULL, NULL, bevent->time);
  
  tool->gdisp_ptr = gdisp_ptr;
  tool->state = ACTIVE;

  b = ink_pen_ellipse (x, y, 
		       bevent->pressure, bevent->xtilt, bevent->ytilt);

  ink_paste (ink_tool, drawable, b);
  ink_tool->last_blob = b;

  gdisplay_flush_now (gdisp);
}

static void
ink_button_release (Tool           *tool,
		    GdkEventButton *bevent,
		    gpointer        gdisp_ptr)
{
  GDisplay * gdisp;
  GImage * gimage;
  InkTool * ink_tool;

  gdisp = (GDisplay *) gdisp_ptr;
  gimage = gdisp->gimage;
  ink_tool = (InkTool *) tool->private;

  /*  resume the current selection and ungrab the pointer  */
  gdisplays_selection_visibility (gdisp->gimage, SelectionResume);

  gdk_pointer_ungrab (bevent->time);
  gdk_flush ();

  /*  Set tool state to inactive -- no longer painting */
  tool->state = INACTIVE;

  ink_finish (ink_tool, gimage_active_drawable (gdisp->gimage), tool->ID);
  gdisplays_flush ();
}

static void
ink_motion (Tool           *tool,
	    GdkEventMotion *mevent,
	    gpointer        gdisp_ptr)
{
  GDisplay *gdisp;
  InkTool  *ink_tool;
  GimpDrawable *drawable;
  Blob *b, *blob_union;

  double x, y;

  gdisp = (GDisplay *) gdisp_ptr;
  ink_tool = (InkTool *) tool->private;

  gdisplay_untransform_coords_f (gdisp, mevent->x, mevent->y, &x, &y, TRUE);
  drawable = gimage_active_drawable (gdisp->gimage);

  b = ink_pen_ellipse (x, y, 
		       mevent->pressure, mevent->xtilt, mevent->ytilt);
  blob_union = blob_convex_union (ink_tool->last_blob, b);
  g_free (ink_tool->last_blob);
  ink_tool->last_blob = b;

  ink_paste (ink_tool, drawable, blob_union);  
  g_free (blob_union);
  
  gdisplay_flush_now (gdisp);
}

static void
ink_cursor_update (Tool           *tool,
		   GdkEventMotion *mevent,
		   gpointer        gdisp_ptr)
{
  GDisplay *gdisp;
  Layer *layer;
  GdkCursorType ctype = GDK_TOP_LEFT_ARROW;
  int x, y;

  gdisp = (GDisplay *) gdisp_ptr;

  gdisplay_untransform_coords (gdisp, mevent->x, mevent->y, &x, &y, FALSE, FALSE);
  if ((layer = gimage_get_active_layer (gdisp->gimage))) 
    {
      int off_x, off_y;
      drawable_offsets (GIMP_DRAWABLE(layer), &off_x, &off_y);
    if (x >= off_x && y >= off_y &&
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
    }
  gdisplay_install_tool_cursor (gdisp, ctype);
}

static void
ink_control (Tool     *tool,
	     int       action,
	     gpointer  gdisp_ptr)
{
  GDisplay *gdisp;
  GimpDrawable *drawable;
  InkTool *ink_tool;

  gdisp = (GDisplay *) gdisp_ptr;
  ink_tool = (InkTool *) tool->private;
  drawable = gimage_active_drawable (gdisp->gimage);

  switch (action)
    {
    case PAUSE :
      draw_core_pause (ink_tool->core, tool);
      break;
    case RESUME :
      draw_core_resume (ink_tool->core, tool);
      break;
    case HALT :
      ink_cleanup ();
      break;
    }
}

static void
ink_init (InkTool *ink_tool, GimpDrawable *drawable, 
		 double x, double y)
{
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
  ink_tool->x1 = ink_tool->x2 = x;
  ink_tool->y1 = ink_tool->y2 = y;
}

static void
ink_finish (InkTool *ink_tool, GimpDrawable *drawable, int tool_id)
{
  /*  push an undo  */
  drawable_apply_image (drawable, ink_tool->x1, ink_tool->y1,
			ink_tool->x2, ink_tool->y2, undo_tiles, TRUE);
  undo_tiles = NULL;

  /*  invalidate the drawable--have to do it here, because
   *  it is not done during the actual painting.
   */
  drawable_invalidate_preview (drawable);
}

static void
ink_cleanup (void)
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

  /*  Free the temporary buffer if it exist  */
  if (canvas_buf)
    temp_buf_free (canvas_buf);
  canvas_buf = NULL;
}

/*********************************
 *  Rendering functions          *
 *********************************/

/* Some of this stuff should probably be combined with the 
 * code it was copied from in paint_core.c; but I wanted
 * to learn this stuff, so I've kept it simple.
 *
 * The following only supports CONSTANT mode. Incremental
 * would, I think, interact strangely with the way we
 * do things. But it wouldn't be hard to implement at all.
 */

static void
ink_set_paint_area (InkTool      *ink_tool, 
		    GimpDrawable *drawable, 
		    Blob         *blob)
{
  int x, y, width, height;
  int x1, y1, x2, y2;
  int bytes;
  
  blob_bounds (blob, &x, &y, &width, &height);

  bytes = drawable_has_alpha (drawable) ?
    drawable_bytes (drawable) : drawable_bytes (drawable) + 1;

  x1 = CLAMP (x/SUBSAMPLE - 1,            0, drawable_width (drawable));
  y1 = CLAMP (y/SUBSAMPLE - 1,            0, drawable_height (drawable));
  x2 = CLAMP ((x + width)/SUBSAMPLE + 2,  0, drawable_width (drawable));
  y2 = CLAMP ((y + height)/SUBSAMPLE + 2, 0, drawable_height (drawable));

  /*  configure the canvas buffer  */
  if ((x2 - x1) && (y2 - y1))
    canvas_buf = temp_buf_resize (canvas_buf, bytes, x1, y1,
				  (x2 - x1), (y2 - y1));
}

enum { ROW_START, ROW_STOP };

/* The insertion sort here, for SUBSAMPLE = 8, tends to beat out
 * qsort() by 4x with CFLAGS=-O2, 2x with CFLAGS=-g
 */
static void insert_sort (int *data, int n)
{
  int i, j, k;
  int tmp1, tmp2;

  for (i=2; i<2*n; i+=2)
    {
      tmp1 = data[i];
      tmp2 = data[i+1];
      j = 0;
      while (data[j] < tmp1)
	j += 2;

      for (k=i; k>j; k-=2)
	{
	  data[k] = data[k-2];
	  data[k+1] = data[k-1];
	}

      data[j] = tmp1;
      data[j+1] = tmp2;
    }
}

static void
fill_run (guchar *dest,
	  guchar  alpha,
	  int     w)
{
  if (alpha == 255)
    {
      memset (dest, 255, w);
    }
  else
    {
      while (w--)
	{
	  *dest += ((256 - *dest) * alpha) >> 8;
	  dest++;
	}
    }
}

static void
render_blob_line (Blob *blob, guchar *dest,
		  int x, int y, int width)
{
  int buf[4*SUBSAMPLE];
  int *data = buf;
  int n = 0;
  int i, j;
  int current = 0;		/* number of filled rows at this point
				 * in the scan line */

  int last_x;

  /* Sort start and ends for all lines */
  
  j = y * SUBSAMPLE - blob->y;
  for (i=0; i<SUBSAMPLE; i++)
    {
      if (j >= blob->height)
	break;
      if ((j > 0) && (blob->data[j].left <= blob->data[j].right))
	{
	  data[2*n] = blob->data[j].left;
	  data[2*n+1] = ROW_START;
	  data[2*SUBSAMPLE+2*n] = blob->data[j].right;
	  data[2*SUBSAMPLE+2*n+1] = ROW_STOP;
	  n++;
	}
      j++;
    }

  /*   If we have less than SUBSAMPLE rows, compress */
  if (n < SUBSAMPLE)
    {
      for (i=0; i<2*n; i++)
	data[2*n+i] = data[2*SUBSAMPLE+i];
    }

  /*   Now count start and end separately */
  n *= 2;

  insert_sort (data, n);

  /* Discard portions outside of tile */

  while ((n > 0) && (data[0] < SUBSAMPLE*x))
    {
      if (data[1] == ROW_START)
	current++;
      else
	current--;
      data += 2;
      n--;
    }

  while ((n > 0) && (data[2*(n-1)] >= SUBSAMPLE*(x+width)))
    n--;
  
  /* Render the row */

  last_x = 0;
  for (i=0; i<n;)
    {
      int cur_x = data[2*i] / SUBSAMPLE - x;
      int pixel;

      /* Fill in portion leading up to this pixel */
      if (current && cur_x != last_x)
	fill_run (dest + last_x, (255*current)/SUBSAMPLE, cur_x - last_x);

      /* Compute the value for this pixel */
      pixel = current * SUBSAMPLE; 

      while (i<n)
	{
	  int tmp_x = data[2*i] / SUBSAMPLE;
	  if (tmp_x - x != cur_x)
	    break;

	  if (data[2*i+1] == ROW_START)
	    {
	      current++;
	      pixel += ((tmp_x + 1) * SUBSAMPLE) - data[2*i];
	    }
	  else
	    {
	      current--;
	      pixel -= ((tmp_x + 1) * SUBSAMPLE) - data[2*i];
	    }
	  
	  i++;
	}

      /* Fill in the pixel */
      dest[cur_x] += 
	((256 - dest[cur_x]) * pixel * 255) / (256 * SUBSAMPLE * SUBSAMPLE);

      last_x = cur_x + 1;
    }

  if (current != 0)
    fill_run (dest + last_x, (255*current)/SUBSAMPLE, width - last_x);
}

static void
render_blob (PixelRegion *dest, Blob *blob)
{
  int i;
  int h;
  unsigned char * s;
  void * pr;

  for (pr = pixel_regions_register (1, dest); 
       pr != NULL; 
       pr = pixel_regions_process (pr))
    {
      h = dest->h;
      s = dest->data;

      for (i=0; i<h; i++)
	{
	  render_blob_line (blob, s,
			    dest->x, dest->y + i, dest->w);
	  s += dest->rowstride;
	}
    }
}

static void
ink_paste (InkTool      *ink_tool, 
	   GimpDrawable *drawable,
	   Blob         *blob)
{
  GImage *gimage;
  PixelRegion srcPR;
  int offx, offy;
  unsigned char col[MAX_CHANNELS];

  if (! (gimage = drawable_gimage (drawable)))
    return;

  /* Get the the buffer */
  ink_set_paint_area (ink_tool, drawable, blob);
 
  /* check to make sure there is actually a canvas to draw on */
  if (!canvas_buf)
    return;

  gimage_get_foreground (gimage, drawable, col);

  /*  set the alpha channel  */
  col[canvas_buf->bytes - 1] = OPAQUE_OPACITY;

  /*  color the pixels  */
  color_pixels (temp_buf_data (canvas_buf), col,
		canvas_buf->width * canvas_buf->height, canvas_buf->bytes);

  /*  set undo blocks  */
  ink_set_undo_tiles (drawable,
		      canvas_buf->x, canvas_buf->y,
		      canvas_buf->width, canvas_buf->height);

  /*  initialize any invalid canvas tiles  */
  ink_set_canvas_tiles (canvas_buf->x, canvas_buf->y,
			canvas_buf->width, canvas_buf->height);

  ink_to_canvas_tiles (ink_tool, blob, col);

  /*  initialize canvas buf source pixel regions  */
  srcPR.bytes = canvas_buf->bytes;
  srcPR.x = 0; srcPR.y = 0;
  srcPR.w = canvas_buf->width;
  srcPR.h = canvas_buf->height;
  srcPR.rowstride = canvas_buf->width * canvas_buf->bytes;
  srcPR.data = temp_buf_data (canvas_buf);

  /*  apply the paint area to the gimage  */
  gimage_apply_image (gimage, drawable, &srcPR,
		      FALSE, 
		      (int) (gimp_brush_get_opacity () * 255),
		      gimp_brush_get_paint_mode(),
		      undo_tiles,  /*  specify an alternative src1  */
		      canvas_buf->x, canvas_buf->y);

  /*  Update the undo extents  */
  ink_tool->x1 = MIN (ink_tool->x1, canvas_buf->x);
  ink_tool->y1 = MIN (ink_tool->y1, canvas_buf->y);
  ink_tool->x2 = MAX (ink_tool->x2, (canvas_buf->x + canvas_buf->width));
  ink_tool->y2 = MAX (ink_tool->y2, (canvas_buf->y + canvas_buf->height));

  /*  Update the gimage--it is important to call gdisplays_update_area
   *  instead of drawable_update because we don't want the drawable
   *  preview to be constantly invalidated
   */
  drawable_offsets (drawable, &offx, &offy);
  gdisplays_update_area (gimage, canvas_buf->x + offx, canvas_buf->y + offy,
			 canvas_buf->width, canvas_buf->height);
}

static void
ink_to_canvas_tiles (InkTool *ink_tool,
		     Blob    *blob,
		     guchar  *color)
{
  PixelRegion srcPR, maskPR;

  /*  draw the blob on the canvas tiles  */
  pixel_region_init (&srcPR, canvas_tiles,
		     canvas_buf->x, canvas_buf->y,
		     canvas_buf->width, canvas_buf->height, TRUE);

  render_blob (&srcPR, blob);

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
ink_set_undo_tiles (drawable, x, y, w, h)
     GimpDrawable *drawable;
     int x, y;
     int w, h;
{
  int i, j;
  Tile *src_tile;
  Tile *dest_tile;

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
ink_set_canvas_tiles (x, y, w, h)
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
	      memset (tile_data_pointer (tile, 0, 0), 
		      0, 
		      tile_size (tile));
	      tile_release (tile, TRUE);
	    }
	}
    }
}

/****************************/
/*  Global ink functions  */
/****************************/

void
ink_no_draw (Tool *tool)
{
  return;
}

Tool *
tools_new_ink ()
{
  Tool * tool;
  InkTool * private;

  if (! ink_options)
    ink_options = create_ink_options ();

  tool = (Tool *) g_malloc (sizeof (Tool));
  private = (InkTool *) g_malloc (sizeof (InkTool));

  private->core = draw_core_new (ink_no_draw);
  private->last_blob = NULL;

  tool->type = INK;
  tool->state = INACTIVE;
  tool->scroll_lock = 0;  /*  Allow scrolling  */
  tool->auto_snap_to = TRUE;
  tool->gdisp_ptr = NULL;
  tool->private = private;
  tool->preserve = TRUE;

  tool->button_press_func = ink_button_press;
  tool->button_release_func = ink_button_release;
  tool->motion_func = ink_motion;
  tool->arrow_keys_func = standard_arrow_keys_func;
  tool->cursor_update_func = ink_cursor_update;
  tool->control_func = ink_control;

  return tool;
}

void
tools_free_ink (Tool *tool)
{
  InkTool * ink_tool;

  ink_tool = (InkTool *) tool->private;

  /*  Make sure the selection core is not visible  */
  if (tool->state == ACTIVE && ink_tool->core)
    draw_core_stop (ink_tool->core, tool);

  /*  Free the selection core  */
  if (ink_tool->core)
    draw_core_free (ink_tool->core);

  /*  Free the last blob, if any */
  if (ink_tool->last_blob)
    g_free (ink_tool->last_blob);
  
  /*  Cleanup memory  */
  ink_cleanup ();

  /*  Free the paint core  */
  g_free (ink_tool);
}


/*  The ink procedure definition  */
ProcArg ink_args[] =
{
  { PDB_IMAGE,
    "image",
    "the image"
  },
  { PDB_DRAWABLE,
    "drawable",
    "the drawable"
  },
  { PDB_INT32,
    "num_strokes",
    "number of stroke control points (count each coordinate as 2 points)"
  },
  { PDB_FLOATARRAY,
    "strokes",
    "array of stroke coordinates: {s1.x, s1.y, s2.x, s2.y, ..., sn.x, sn.y}"
  }
};


ProcRecord ink_proc =
{
  "gimp_ink",
  "Paint in the current brush without sub-pixel sampling",
  "fixme fixme",
  "Spencer Kimball & Peter Mattis",
  "Spencer Kimball & Peter Mattis",
  "1995-1996",
  PDB_INTERNAL,

  /*  Input arguments  */
  4,
  ink_args,

  /*  Output arguments  */
  0,
  NULL,

  /*  Exec method  */
  { { ink_invoker } },
};

static Argument *
ink_invoker (args)
     Argument *args;
{
  /* Fix me */
  return NULL;
}

