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
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include "appenv.h"
#include "drawable.h"
#include "draw_core.h"
#include "gimage_mask.h"
#include "gimprc.h"
#include "ink.h"
#include "paint_options.h"
#include "tool_options_ui.h"
#include "tools.h"
#include "undo.h"
#include "blob.h"
#include "gdisplay.h"

#include "libgimp/gimpintl.h"

#include "tile.h"			/* ick. */

#ifndef M_PI
#define M_PI    3.14159265358979323846
#endif /* M_PI */

#define SUBSAMPLE 8

#define DIST_SMOOTHER_BUFFER 10
#define TIME_SMOOTHER_BUFFER 10

/*  the Ink structures  */

typedef Blob *(*BlobFunc) (double, double, double, double, double, double);

typedef struct _InkTool InkTool;
struct _InkTool
{
  DrawCore * core;         /*  Core select object             */
  
  Blob     * last_blob;	   /*  blob for last cursor position  */

  int        x1, y1;       /*  image space coordinate         */
  int        x2, y2;       /*  image space coords             */

  /* circular distance history buffer */
  gdouble    dt_buffer[DIST_SMOOTHER_BUFFER];
  gint       dt_index;

  /* circular timing history buffer */
  guint32    ts_buffer[TIME_SMOOTHER_BUFFER];
  gint       ts_index;

  gdouble    last_time;    /*  previous time of a motion event      */
  gdouble    lastx, lasty; /*  previous position of a motion event  */

  gboolean   init_velocity;
};

typedef struct _BrushWidget BrushWidget;
struct _BrushWidget
{
  GtkWidget   *widget;
  gboolean     state;
};

typedef struct _InkOptions InkOptions;
struct _InkOptions
{
  PaintOptions  paint_options;

  double        size;
  double        size_d;
  GtkObject    *size_w;

  double        sensitivity;
  double        sensitivity_d;
  GtkObject    *sensitivity_w;

  double        vel_sensitivity;
  double        vel_sensitivity_d;
  GtkObject    *vel_sensitivity_w;

  double        tilt_sensitivity;
  double        tilt_sensitivity_d;
  GtkObject    *tilt_sensitivity_w;

  double        tilt_angle;
  double        tilt_angle_d;
  GtkObject    *tilt_angle_w;

  BlobFunc      function;
  BlobFunc      function_d;
  GtkWidget    *function_w[3];  /* 3 radio buttons */

  double        aspect;
  double        aspect_d;
  double        angle;
  double        angle_d;
  BrushWidget  *brush_w;
};


/* the ink tool options  */
static InkOptions *ink_options = NULL;

/* local variables */

/*  undo blocks variables  */
static TileManager *  undo_tiles = NULL;

/* Tiles used to render the stroke at 1 byte/pp */
static TileManager *  canvas_tiles = NULL;

/* Flat buffer that is used to used to render the dirty region
 * for composition onto the destination drawable
 */
static TempBuf *  canvas_buf = NULL;


/*  local function prototypes  */

static void   ink_button_press   (Tool *, GdkEventButton *, gpointer);
static void   ink_button_release (Tool *, GdkEventButton *, gpointer);
static void   ink_motion         (Tool *, GdkEventMotion *, gpointer);
static void   ink_cursor_update  (Tool *, GdkEventMotion *, gpointer);
static void   ink_control        (Tool *, ToolAction,       gpointer);

static void    time_smoother_add    (InkTool* ink_tool, guint32 value);
static gdouble time_smoother_result (InkTool* ink_tool);
static void    time_smoother_init   (InkTool* ink_tool, guint32 initval);
static void    dist_smoother_add    (InkTool* ink_tool, gdouble value);
static gdouble dist_smoother_result (InkTool* ink_tool);
static void    dist_smoother_init   (InkTool* ink_tool, gdouble initval);

static void ink_init   (InkTool      *ink_tool, 
			GimpDrawable *drawable, 
			double        x, 
			double        y);
static void ink_finish (InkTool      *ink_tool, 
			GimpDrawable *drawable, 
			int           tool_id);
static void ink_cleanup (void);

static void ink_type_update               (GtkWidget      *radio_button,
                                           BlobFunc        function);
static GdkPixmap *blob_pixmap (GdkColormap *colormap,
			       GdkVisual   *visual,
			       BlobFunc     function);
static void paint_blob (GdkDrawable  *drawable, 
			GdkGC        *gc,
			Blob         *blob);

/*  Rendering functions  */
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

/*  Brush pseudo-widget callbacks  */
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


/*  functions  */

static void 
ink_type_update (GtkWidget      *radio_button,
		 BlobFunc        function)
{
  InkOptions *options = ink_options;

  if (GTK_TOGGLE_BUTTON (radio_button)->active)
    options->function = function;

  gtk_widget_queue_draw (options->brush_w->widget);
}

static void
ink_options_reset (void)
{
  InkOptions *options = ink_options;

  paint_options_reset ((PaintOptions *) options);

  gtk_adjustment_set_value (GTK_ADJUSTMENT (options->size_w),
			    options->size_d);
  gtk_adjustment_set_value (GTK_ADJUSTMENT (options->sensitivity_w),
			    options->sensitivity_d);
  gtk_adjustment_set_value (GTK_ADJUSTMENT (options->tilt_sensitivity_w),
			    options->tilt_sensitivity_d);
  gtk_adjustment_set_value (GTK_ADJUSTMENT (options->vel_sensitivity_w),
			    options->vel_sensitivity_d);
  gtk_adjustment_set_value (GTK_ADJUSTMENT (options->tilt_angle_w),
			    options->tilt_angle_d);
  gtk_toggle_button_set_active (((options->function_d == blob_ellipse) ?
				 GTK_TOGGLE_BUTTON (options->function_w[0]) :
				 ((options->function_d == blob_square) ?
				  GTK_TOGGLE_BUTTON (options->function_w[1]) :
				  GTK_TOGGLE_BUTTON (options->function_w[2]))),
				TRUE);
  options->aspect = options->aspect_d;
  options->angle  = options->angle_d;
  gtk_widget_queue_draw (options->brush_w->widget);
}

static InkOptions *
ink_options_new (void)
{
  InkOptions *options;

  GtkWidget *table;
  GtkWidget *vbox;
  GtkWidget *abox;
  GtkWidget *label;
  GtkWidget *radio_button;
  GtkWidget *pixmap_widget;
  GtkWidget *slider;
  GtkWidget *frame;
  GtkWidget *darea;
  GdkPixmap *pixmap;

  /*  the new ink tool options structure  */
  options = (InkOptions *) g_malloc (sizeof (InkOptions));
  paint_options_init ((PaintOptions *) options,
		      INK,
		      ink_options_reset);
  options->size             = options->size_d             = 4.4;
  options->sensitivity      = options->sensitivity_d      = 1.0;
  options->vel_sensitivity  = options->vel_sensitivity_d  = 0.8;
  options->tilt_sensitivity = options->tilt_sensitivity_d = 0.4;
  options->tilt_angle       = options->tilt_angle_d       = 0.0;
  options->function         = options->function_d         = blob_ellipse;
  options->aspect           = options->aspect_d           = 1.0;
  options->angle            = options->angle_d            = 0.0;

  /*  the main table  */
  table = gtk_table_new (9, 2, FALSE);
  gtk_table_set_col_spacing (GTK_TABLE (table), 0, 4);
  gtk_table_set_row_spacing (GTK_TABLE (table), 0, 2);
  gtk_table_set_row_spacing (GTK_TABLE (table), 1, 2);
  gtk_table_set_row_spacing (GTK_TABLE (table), 3, 2);
  gtk_table_set_row_spacing (GTK_TABLE (table), 5, 2);
  gtk_table_set_row_spacing (GTK_TABLE (table), 7, 4);
  gtk_box_pack_start (GTK_BOX (((ToolOptions *) options)->main_vbox), table,
		      FALSE, FALSE, 0);

  /*  size slider  */
  label = gtk_label_new (_("Size:"));
  gtk_misc_set_alignment (GTK_MISC (label), 1.0, 1.0);
  gtk_table_attach (GTK_TABLE (table), label, 0, 1, 0, 1,
		    GTK_SHRINK | GTK_FILL, GTK_SHRINK | GTK_FILL, 0, 0);
  gtk_widget_show (label);
  
  options->size_w =
    gtk_adjustment_new (options->size_d, 0.0, 20.0, 1.0, 5.0, 0.0);
  slider = gtk_hscale_new (GTK_ADJUSTMENT (options->size_w));
  gtk_scale_set_value_pos (GTK_SCALE (slider), GTK_POS_TOP);
  gtk_table_attach_defaults (GTK_TABLE (table), slider, 1, 2, 0, 1);
  gtk_range_set_update_policy (GTK_RANGE (slider), GTK_UPDATE_DELAYED);
  gtk_signal_connect (GTK_OBJECT (options->size_w), "value_changed",
		      (GtkSignalFunc) tool_options_double_adjustment_update,
		      &options->size);
  gtk_widget_show (slider);

  /* sens slider */
  label = gtk_label_new (_("Sensitivity:"));
  gtk_misc_set_alignment (GTK_MISC (label), 1.0, 1.0);
  gtk_table_attach (GTK_TABLE (table), label, 0, 1, 1, 2,
		    GTK_SHRINK | GTK_FILL, GTK_SHRINK | GTK_FILL, 0, 0);
  gtk_widget_show (label);
  
  options->sensitivity_w =
    gtk_adjustment_new (options->sensitivity_d, 0.0, 1.0, 0.01, 0.1, 0.0);
  slider = gtk_hscale_new (GTK_ADJUSTMENT (options->sensitivity_w));
  gtk_scale_set_value_pos (GTK_SCALE (slider), GTK_POS_TOP);
  gtk_table_attach_defaults (GTK_TABLE (table), slider, 1, 2, 1, 2);
  gtk_range_set_update_policy (GTK_RANGE (slider), GTK_UPDATE_DELAYED);
  gtk_signal_connect (GTK_OBJECT (options->sensitivity_w), "value_changed",
		      (GtkSignalFunc) tool_options_double_adjustment_update,
		      &options->sensitivity);
  gtk_widget_show (slider);
  
  /* tilt sens slider */
  label = gtk_label_new (_("Tilt"));
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 1.0);
  gtk_table_attach (GTK_TABLE (table), label, 0, 1, 2, 3,
		    GTK_SHRINK | GTK_FILL, GTK_SHRINK, 0, 0);
  gtk_widget_show (label);

  label = gtk_label_new (_("Sensitivity:"));
  gtk_misc_set_alignment (GTK_MISC (label), 1.0, 1.0);
  gtk_table_attach (GTK_TABLE (table), label, 0, 1, 3, 4,
		    GTK_SHRINK | GTK_FILL, GTK_SHRINK, 0, 0);
  gtk_widget_show (label);

  abox = gtk_alignment_new (0.5, 1.0, 1.0, 0.0);
  gtk_table_attach (GTK_TABLE (table), abox, 1, 2, 2, 4,
		    GTK_EXPAND | GTK_FILL, GTK_SHRINK | GTK_FILL, 0, 0);
  gtk_widget_show (abox);

  options->tilt_sensitivity_w =
    gtk_adjustment_new (options->tilt_sensitivity_d, 0.0, 1.0, 0.01, 0.1, 0.0);
  slider = gtk_hscale_new (GTK_ADJUSTMENT (options->tilt_sensitivity_w));
  gtk_scale_set_value_pos (GTK_SCALE (slider), GTK_POS_TOP);
  gtk_container_add (GTK_CONTAINER (abox), slider);
  gtk_range_set_update_policy (GTK_RANGE (slider), GTK_UPDATE_DELAYED);
  gtk_signal_connect (GTK_OBJECT (options->tilt_sensitivity_w), "value_changed",
		      (GtkSignalFunc) tool_options_double_adjustment_update,
		      &options->tilt_sensitivity);
  gtk_widget_show (slider);


  /* velocity sens slider */
  label = gtk_label_new (_("Speed"));
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 1.0);
  gtk_table_attach (GTK_TABLE (table), label, 0, 1, 4, 5,
		    GTK_SHRINK | GTK_FILL, GTK_SHRINK, 0, 0);
  gtk_widget_show (label);

  label = gtk_label_new (_("Sensitivity:"));
  gtk_misc_set_alignment (GTK_MISC (label), 1.0, 1.0);
  gtk_table_attach (GTK_TABLE (table), label, 0, 1, 5, 6,
		    GTK_SHRINK | GTK_FILL, GTK_SHRINK, 0, 0);
  gtk_widget_show (label);

  abox = gtk_alignment_new (0.5, 1.0, 1.0, 0.0);
  gtk_table_attach (GTK_TABLE (table), abox, 1, 2, 4, 6,
		    GTK_EXPAND | GTK_FILL, GTK_SHRINK | GTK_FILL, 0, 0);
  gtk_widget_show (abox);

  options->vel_sensitivity_w =
    gtk_adjustment_new (options->vel_sensitivity_d, 0.0, 1.0, 0.01, 0.1, 0.0);
  slider = gtk_hscale_new (GTK_ADJUSTMENT (options->vel_sensitivity_w));
  gtk_scale_set_value_pos (GTK_SCALE (slider), GTK_POS_TOP);
  gtk_container_add (GTK_CONTAINER (abox), slider);
  gtk_range_set_update_policy (GTK_RANGE (slider), GTK_UPDATE_DELAYED);
  gtk_signal_connect (GTK_OBJECT (options->vel_sensitivity_w), "value_changed",
		      (GtkSignalFunc) tool_options_double_adjustment_update,
		      &options->vel_sensitivity);
  gtk_widget_show (slider);


  /* angle adjust slider */
  label = gtk_label_new (_("Angle"));
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 1.0);
  gtk_table_attach (GTK_TABLE (table), label, 0, 1, 6, 7,
		    GTK_SHRINK | GTK_FILL, GTK_SHRINK, 0, 0);
  gtk_widget_show (label);

  label = gtk_label_new (_("Adjust:"));
  gtk_misc_set_alignment (GTK_MISC (label), 1.0, 1.0);
  gtk_table_attach (GTK_TABLE (table), label, 0, 1, 7, 8,
		    GTK_SHRINK | GTK_FILL, GTK_SHRINK, 0, 0);
  gtk_widget_show (label);

  abox = gtk_alignment_new (0.5, 1.0, 1.0, 0.0);
  gtk_table_attach (GTK_TABLE (table), abox, 1, 2, 6, 8,
		    GTK_EXPAND | GTK_FILL, GTK_SHRINK | GTK_FILL, 0, 0);
  gtk_widget_show (abox);

  options->tilt_angle_w =
    gtk_adjustment_new (options->tilt_angle_d, -90.0, 90.0, 1, 10.0, 0.0);
  slider = gtk_hscale_new (GTK_ADJUSTMENT (options->tilt_angle_w));
  gtk_scale_set_value_pos (GTK_SCALE (slider), GTK_POS_TOP);
  gtk_container_add (GTK_CONTAINER (abox), slider);
  gtk_range_set_update_policy (GTK_RANGE (slider), GTK_UPDATE_DELAYED);
  gtk_signal_connect (GTK_OBJECT (options->tilt_angle_w), "value_changed",
		      (GtkSignalFunc) tool_options_double_adjustment_update,
		      &options->tilt_angle);

  /* Brush type radiobuttons */

  frame = gtk_frame_new (_("Type"));
  gtk_table_attach (GTK_TABLE (table), frame, 0, 1, 8, 9,
		    GTK_SHRINK | GTK_FILL, GTK_SHRINK | GTK_FILL, 0, 0);
  gtk_widget_show (frame);

  vbox = gtk_vbox_new (FALSE, 2);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), 2);
  gtk_container_add (GTK_CONTAINER (frame), vbox);
  gtk_widget_show (vbox);

  pixmap = blob_pixmap (gtk_widget_get_colormap (vbox),
			gtk_widget_get_visual (vbox),
			blob_ellipse);

  pixmap_widget = gtk_pixmap_new (pixmap, NULL);
  gdk_pixmap_unref (pixmap);

  radio_button = gtk_radio_button_new (NULL);
  gtk_signal_connect (GTK_OBJECT (radio_button), "toggled",
		      GTK_SIGNAL_FUNC (ink_type_update), 
		      (gpointer)blob_ellipse);

  gtk_container_add (GTK_CONTAINER (radio_button), pixmap_widget);
  gtk_box_pack_start (GTK_BOX (vbox), radio_button, FALSE, FALSE, 0);

  options->function_w[0] = radio_button;

  pixmap = blob_pixmap (gtk_widget_get_colormap (vbox),
			gtk_widget_get_visual (vbox),
			blob_square);

  pixmap_widget = gtk_pixmap_new (pixmap, NULL);
  gdk_pixmap_unref (pixmap);

  radio_button =
    gtk_radio_button_new_from_widget (GTK_RADIO_BUTTON (radio_button));
  gtk_signal_connect (GTK_OBJECT (radio_button), "toggled",
		      GTK_SIGNAL_FUNC (ink_type_update), 
		      (gpointer)blob_square);
  
  gtk_container_add (GTK_CONTAINER (radio_button), pixmap_widget);
  gtk_box_pack_start (GTK_BOX (vbox), radio_button, FALSE, FALSE, 0);

  options->function_w[1] = radio_button;

  pixmap = blob_pixmap (gtk_widget_get_colormap (vbox),
			gtk_widget_get_visual (vbox),
			blob_diamond);

  pixmap_widget = gtk_pixmap_new (pixmap, NULL);
  gdk_pixmap_unref (pixmap);

  radio_button =
    gtk_radio_button_new_from_widget (GTK_RADIO_BUTTON (radio_button));
  gtk_signal_connect (GTK_OBJECT (radio_button), "toggled",
		      GTK_SIGNAL_FUNC (ink_type_update), 
		      (gpointer)blob_diamond);

  gtk_container_add (GTK_CONTAINER (radio_button), pixmap_widget);
  gtk_box_pack_start (GTK_BOX (vbox), radio_button, FALSE, FALSE, 0);

  options->function_w[2] = radio_button;

  /* Brush shape widget */
  frame = gtk_frame_new (_("Shape"));
  gtk_table_attach_defaults (GTK_TABLE (table), frame, 1, 2, 8, 9);
  gtk_widget_show (frame);

  vbox = gtk_vbox_new (FALSE, 2);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), 2);
  gtk_container_add (GTK_CONTAINER (frame), vbox);
  gtk_widget_show (vbox);

  options->brush_w = g_new (BrushWidget, 1);
  options->brush_w->state = FALSE;

  frame = gtk_aspect_frame_new (NULL, 0.5, 0.5, 1.0, FALSE);
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_IN);
  gtk_box_pack_start (GTK_BOX (vbox), frame, TRUE, TRUE, 0);

  darea = gtk_drawing_area_new();
  options->brush_w->widget = darea;

  gtk_drawing_area_size (GTK_DRAWING_AREA (darea), 60, 60);
  gtk_container_add (GTK_CONTAINER (frame), darea);

  gtk_widget_set_events (darea, 
			 GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK
			 | GDK_POINTER_MOTION_MASK | GDK_EXPOSURE_MASK);
  gtk_signal_connect (GTK_OBJECT (darea), "button_press_event",
		      GTK_SIGNAL_FUNC (brush_widget_button_press),
		      options->brush_w);
  gtk_signal_connect (GTK_OBJECT (darea), "button_release_event",
		      GTK_SIGNAL_FUNC (brush_widget_button_release),
		      options->brush_w);
  gtk_signal_connect (GTK_OBJECT (darea), "motion_notify_event",
		      GTK_SIGNAL_FUNC (brush_widget_motion_notify),
		      options->brush_w);
  gtk_signal_connect (GTK_OBJECT (darea), "expose_event",
		      GTK_SIGNAL_FUNC (brush_widget_expose), 
		      options->brush_w);
  gtk_signal_connect (GTK_OBJECT (darea), "realize",
		      GTK_SIGNAL_FUNC (brush_widget_realize),
		      options->brush_w);

  gtk_widget_show_all (table);

  return options;
}


/*  the brush widget functions  */

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
  Blob *b;

  b = ink_options->function (xc,yc,
			     radius*cos(ink_options->angle),
			     radius*sin(ink_options->angle),
			     -(radius/ink_options->aspect)*sin(ink_options->angle),
			     (radius/ink_options->aspect)*cos(ink_options->angle));

  paint_blob (w->window, w->style->fg_gc[w->state],b);
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

/*
 * Return a black-on white pixmap in the given colormap and
 * visual that represents the BlobFunc 'function'
 */
static GdkPixmap *
blob_pixmap (GdkColormap *colormap,
	     GdkVisual *visual,
	     BlobFunc function)
{
  GdkPixmap *pixmap;
  GdkGC *black_gc, *white_gc;
  GdkColor tmp_color;
  Blob *blob;

  pixmap = gdk_pixmap_new (NULL, 22, 21, visual->depth);

  black_gc = gdk_gc_new (pixmap);
  gdk_color_black (colormap, &tmp_color);
  gdk_gc_set_foreground (black_gc, &tmp_color);

  white_gc = gdk_gc_new (pixmap);
  gdk_color_white (colormap, &tmp_color);
  gdk_gc_set_foreground (white_gc, &tmp_color);

  gdk_draw_rectangle (pixmap, white_gc, TRUE, 0, 0, 21, 20);
  gdk_draw_rectangle (pixmap, black_gc, FALSE, 0, 0, 21, 20);
  blob = (*function) (10, 10, 8, 0, 0, 8);
  paint_blob (pixmap, black_gc, blob);

  gdk_gc_unref (white_gc);
  gdk_gc_unref (black_gc);

  return pixmap;
}

/*
 * Draw a blob onto a drawable with the specified graphics context
 */
static void
paint_blob (GdkDrawable *drawable, GdkGC *gc,
	    Blob *blob)
{
  int i;

  for (i=0;i<blob->height;i++)
    {
      if (blob->data[i].left <= blob->data[i].right)
	gdk_draw_line (drawable, gc,
		       blob->data[i].left,i+blob->y,
		       blob->data[i].right+1,i+blob->y);
    }
}


static Blob *
ink_pen_ellipse (gdouble x_center, gdouble y_center,
		 gdouble pressure, gdouble xtilt, gdouble ytilt,
		 gdouble velocity)
{
  double size;
  double tsin, tcos;
  double aspect, radmin;
  double x,y;
  double tscale;
  double tscale_c;
  double tscale_s;
  
  /* Adjust the size depending on pressure. */

  size = ink_options->size * (1.0 + ink_options->sensitivity *
			      (2.0 * pressure - 1.0) );

  /* Adjust the size further depending on pointer velocity
     and velocity-sensitivity.  These 'magic constants' are
     'feels natural' tigert-approved. --ADM */

  if (velocity < 3.0)
    velocity = 3.0;

#ifdef VERBOSE
  g_print("%f (%f) -> ", (float)size, (float)velocity);
#endif  

  size = ink_options->vel_sensitivity *
    ((4.5 * size) / (1.0 + ink_options->vel_sensitivity * (2.0*(velocity))))
    + (1.0 - ink_options->vel_sensitivity) * size;

#ifdef VERBOSE
  g_print("%f\n", (float)size);
#endif

  /* Clamp resulting size to sane limits */

  if (size > ink_options->size * (1.0 + ink_options->sensitivity))
    size = ink_options->size * (1.0 + ink_options->sensitivity);

  if (size*SUBSAMPLE < 1.0) size = 1.0/SUBSAMPLE;

  /* Add brush angle/aspect to tilt vectorially */

  /* I'm not happy with the way the brush widget info is combined with
     tilt info from the brush. My personal feeling is that representing
     both as affine transforms would make the most sense. -RLL */

  tscale = ink_options->tilt_sensitivity * 10.0;
  tscale_c = tscale * cos (ink_options->tilt_angle * M_PI / 180);
  tscale_s = tscale * sin (ink_options->tilt_angle * M_PI / 180);
  x = ink_options->aspect*cos(ink_options->angle) +
    xtilt * tscale_c - ytilt * tscale_s;
  y = ink_options->aspect*sin(ink_options->angle) +
    ytilt * tscale_c + xtilt * tscale_s;
#ifdef VERBOSE
  g_print ("angle %g aspect %g; %g %g; %g %g\n",
	   ink_options->angle, ink_options->aspect, tscale_c, tscale_s, x, y);
#endif
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
  
  return ink_options->function (x_center * SUBSAMPLE, y_center * SUBSAMPLE,
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
		       bevent->pressure, bevent->xtilt, bevent->ytilt, 10.0);

  ink_paste (ink_tool, drawable, b);
  ink_tool->last_blob = b;

  time_smoother_init (ink_tool, bevent->time);
  ink_tool->last_time = bevent->time;
  dist_smoother_init (ink_tool, 0.0);
  ink_tool->init_velocity = TRUE;
  ink_tool->lastx = x;
  ink_tool->lasty = y;

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

  /*  free the last blob  */
  g_free (ink_tool->last_blob);
  ink_tool->last_blob = NULL;

  ink_finish (ink_tool, gimage_active_drawable (gdisp->gimage), tool->ID);
  gdisplays_flush ();
}


static void
dist_smoother_init (InkTool* ink_tool, gdouble initval)
{
  gint i;

  ink_tool->dt_index = 0;

  for (i=0; i<DIST_SMOOTHER_BUFFER; i++)
    {
      ink_tool->dt_buffer[i] = initval;
    }
}

static gdouble
dist_smoother_result (InkTool* ink_tool)
{
  gint i;
  gdouble result = 0.0;

  for (i=0; i<DIST_SMOOTHER_BUFFER; i++)
    {
      result += ink_tool->dt_buffer[i];
    }

  return (result / (gdouble)DIST_SMOOTHER_BUFFER);
}

static void
dist_smoother_add (InkTool* ink_tool, gdouble value)
{
  ink_tool->dt_buffer[ink_tool->dt_index] = value;

  if ((++ink_tool->dt_index) == DIST_SMOOTHER_BUFFER)
    ink_tool->dt_index = 0;
}


static void
time_smoother_init (InkTool* ink_tool, guint32 initval)
{
  gint i;

  ink_tool->ts_index = 0;

  for (i=0; i<TIME_SMOOTHER_BUFFER; i++)
    {
      ink_tool->ts_buffer[i] = initval;
    }
}

static gdouble
time_smoother_result (InkTool* ink_tool)
{
  gint i;
  guint64 result = 0;

  for (i=0; i<TIME_SMOOTHER_BUFFER; i++)
    {
      result += ink_tool->ts_buffer[i];
    }

#ifdef _MSC_VER
  return (gdouble) (gint64) (result / TIME_SMOOTHER_BUFFER);
#else
  return (result / TIME_SMOOTHER_BUFFER);
#endif
}

static void
time_smoother_add (InkTool* ink_tool, guint32 value)
{
  ink_tool->ts_buffer[ink_tool->ts_index] = value;

  if ((++ink_tool->ts_index) == TIME_SMOOTHER_BUFFER)
    ink_tool->ts_index = 0;
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
  double pressure;
  double velocity;
  double dist;
  gdouble lasttime, thistime;
  

  gdisp = (GDisplay *) gdisp_ptr;
  ink_tool = (InkTool *) tool->private;

  gdisplay_untransform_coords_f (gdisp, mevent->x, mevent->y, &x, &y, TRUE);
  drawable = gimage_active_drawable (gdisp->gimage);

  lasttime = ink_tool->last_time;

  time_smoother_add (ink_tool, mevent->time);
  thistime = ink_tool->last_time =
    time_smoother_result(ink_tool);

  /* The time resolution on X-based GDK motion events is
     bloody awful, hence the use of the smoothing function.
     Sadly this also means that there is always the chance of
     having an indeterminite velocity since this event and
     the previous several may still appear to issue at the same
     instant. -ADM */

  if (thistime == lasttime)
    thistime = lasttime + 1;

  if (ink_tool->init_velocity)
    {
      dist_smoother_init (ink_tool, dist = sqrt((ink_tool->lastx-x)*(ink_tool->lastx-x) +
						(ink_tool->lasty-y)*(ink_tool->lasty-y)));
      ink_tool->init_velocity = FALSE;
    }
  else
    {
      dist_smoother_add (ink_tool, sqrt((ink_tool->lastx-x)*(ink_tool->lastx-x) +
					(ink_tool->lasty-y)*(ink_tool->lasty-y)));
      dist = dist_smoother_result(ink_tool);
    }

  ink_tool->lastx = x;
  ink_tool->lasty = y;

  pressure = mevent->pressure;
  velocity = 10.0 * sqrt((dist) / (double)(thistime - lasttime));
  
  b = ink_pen_ellipse (x, y, pressure, mevent->xtilt, mevent->ytilt, velocity);
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
ink_control (Tool       *tool,
	     ToolAction  action,
	     gpointer    gdisp_ptr)
{
  InkTool *ink_tool;

  ink_tool = (InkTool *) tool->private;

  switch (action)
    {
    case PAUSE:
      draw_core_pause (ink_tool->core, tool);
      break;

    case RESUME:
      draw_core_resume (ink_tool->core, tool);
      break;

    case HALT:
      ink_cleanup ();
      break;

    default:
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
	  *dest = MAX(*dest, alpha);
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

      dest[cur_x] = MAX(dest[cur_x], (pixel * 255) / (SUBSAMPLE * SUBSAMPLE));

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
		      (int) (gimp_context_get_opacity (NULL) * 255),
		      gimp_context_get_paint_mode (NULL),
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

/* This routine a) updates the representation of the stroke
 * in the canvas tiles, then renders the dirty bit of it
 * into canvas_buf.
 */
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
ink_set_undo_tiles (GimpDrawable *drawable,
		    int x, int y, int w, int h)
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
ink_set_canvas_tiles (int x, int y, int w, int h)
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

/**************************/
/*  Global ink functions  */
/**************************/

void
ink_no_draw (Tool *tool)
{
  return;
}

Tool *
tools_new_ink (void)
{
  Tool * tool;
  InkTool * private;

  /*  The tool options  */
  if (! ink_options)
    {
      ink_options = ink_options_new ();
      tools_register (INK, (ToolOptions *) ink_options);

      /*  press all default buttons  */
      ink_options_reset ();
    }

  tool = tools_new_tool (INK);
  private = g_new (InkTool, 1);

  private->core = draw_core_new (ink_no_draw);
  private->last_blob = NULL;

  tool->private = private;

  tool->button_press_func   = ink_button_press;
  tool->button_release_func = ink_button_release;
  tool->motion_func         = ink_motion;
  tool->cursor_update_func  = ink_cursor_update;
  tool->control_func        = ink_control;

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

  /*  Free the last blob, if any  */
  if (ink_tool->last_blob)
    g_free (ink_tool->last_blob);
  
  /*  Cleanup memory  */
  ink_cleanup ();

  /*  Free the paint core  */
  g_free (ink_tool);
}
