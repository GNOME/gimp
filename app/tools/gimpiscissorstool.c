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

/* This tool is based on a paper from SIGGRAPH '95
 * thanks to Professor D. Forsyth for prompting us to implement this tool
 */

#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "appenv.h"
#include "draw_core.h"
#include "channel_pvt.h"
#include "drawable.h"
#include "errors.h"
#include "gdisplay.h"
#include "gimage_mask.h"
#include "interface.h"
#include "iscissors.h"
#include "edit_selection.h"
#include "paint_funcs.h"
#include "rect_select.h"
#include "selection_options.h"
#include "temp_buf.h"
#include "tools.h"
#include "bezier_selectP.h"

#include "libgimp/gimpintl.h"

#ifndef M_PI
#define M_PI    3.14159265358979323846
#endif /* M_PI */
#ifndef M_PI_4
#define M_PI_4  0.78539816339744830962
#endif /* M_PI_4 */

/*  the intelligent scissors structures  */

typedef struct _Kink Kink;
struct _Kink
{
  int             x, y;         /*  coordinates  */
  int             is_a_kink;    /*  is this a kink?  */
  double          normal[2];    /*  normal vector to kink  */
  double          kinkiness;    /*  kinkiness measure  */
};

typedef struct _Point Point;
struct _Point
{
  int             x, y;         /*  coordinates  */
  int             dir;          /*  direction  */
  int             kink;         /*  is it a kink?  */
  int             stable;       /*  is the point in a stable locale?  */
  double          dx, dy;       /*  moving coordinates  */
  double          normal[2];    /*  normal vector to kink  */
};

typedef struct _Iscissors Iscissors;
struct _Iscissors
{
  DrawCore *      core;         /*  Core select object               */
  int             x, y;         /*  upper left hand coordinate       */
  int             ix, iy;       /*  initial coordinates              */
  int             nx, ny;       /*  new coordinates                  */
  int             state;        /*  state of iscissors               */
  int             num_segs;     /*  number of points in the polygon  */
  int             num_pts;      /*  number of kinks in list          */
  int             num_kinks;    /*  number of kinks in list          */
  Channel *       mask;         /*  selection mask                   */
  Kink *          kinks;        /*  kinks in the object outline      */
  TempBuf *       edge_buf;     /*  edge map buffer                  */
};

typedef struct _IScissorsOptions IScissorsOptions;
struct _IScissorsOptions
{
  SelectionOptions  selection_options;

  double            resolution;
  double            resolution_d;
  GtkObject        *resolution_w;

  double            threshold;
  double            threshold_d;
  GtkObject        *threshold_w;

  double            elasticity;
  double            elasticity_d;
  GtkObject        *elasticity_w;
};

typedef double BezierMatrix[4][4];
typedef double CRMatrix[4][4];


/*  the intelligent scissors tool options  */
static IScissorsOptions *iscissors_options = NULL;


/**********************************************/
/*  Intelligent scissors selection apparatus  */

#define  IMAGE_COORDS      1
#define  AA_IMAGE_COORDS   2
#define  SCREEN_COORDS     3

#define  SUPERSAMPLE       3
#define  SUPERSAMPLE2      9

#define  FREE_SELECT_MODE  0
#define  BOUNDARY_MODE     1

#define  POINT_WIDTH       8
#define  POINT_HALFWIDTH   4

#define  DEFAULT_MAX_INC   1024
#define  EDGE_WIDTH        1
#define  LOCALIZE_RADIUS   24
#define  BLOCK_WIDTH       64
#define  BLOCK_HEIGHT      64
#define  CONV_WIDTH        BLOCK_WIDTH
#define  CONV_HEIGHT       BLOCK_HEIGHT

#define  HORIZONTAL        0
#define  VERTICAL          1

#define  SUBDIVIDE         1000
#define  EDGE_STRENGTH     255
#define  EPSILON           0.00001

/*  functional defines  */
#define  SQR(x)            ((x) * (x))
#define  BILINEAR(jk,j1k,jk1,j1k1,dx,dy) \
     ((1-dy) * ((1-dx)*jk + dx*j1k) + \
      dy  * ((1-dx)*jk1 + dx*j1k1))


/*  static variables  */
static Tool* last_tool;

/*  The global array of XSegments for drawing the polygon...  */
static GdkSegment *segs = NULL;
static int         max_segs = 0;
static Point *     pts = NULL;
static int         max_pts = 0;

/*  boundary resolution variables  */
static double      kink_thres = 0.33;   /* between 0.0 -> 1.0 */
static double      std_dev    = 1.0;    /* in pixels */
static int         miss_thres = 4;      /* in intensity */

/*  edge map blocks variables  */
static TempBuf **  edge_map_blocks = NULL;
static int         horz_blocks;
static int         vert_blocks;

/*  convolution and basis matrixes  */
static unsigned char  conv1 [CONV_WIDTH * CONV_HEIGHT * MAX_CHANNELS];
static unsigned char  conv2 [CONV_WIDTH * CONV_HEIGHT * MAX_CHANNELS];
static unsigned char  grad  [(CONV_WIDTH + 2) * (CONV_HEIGHT + 2)];

static CRMatrix CR_basis =
{
  { -0.5,  1.5, -1.5,  0.5 },
  {  1.0, -2.5,  2.0, -0.5 },
  { -0.5,  0.0,  0.5,  0.0 },
  {  0.0,  1.0,  0.0,  0.0 },
};

static CRMatrix CR_bezier_basis =
{
  {  0.0,  1.0,  0.0,  0.0 },
  { -0.16667, 1.0, 0.16667, 0.0 },
  {  0.0,  0.16667, 1.0, -0.16667 },
  {  0.0,  0.0,  1.0,  0.0 },
};


/*  Local function prototypes  */

static void   selection_to_bezier	(GtkWidget* , gpointer);

static void   iscissors_button_press    (Tool *, GdkEventButton *, gpointer);
static void   iscissors_button_release  (Tool *, GdkEventButton *, gpointer);
static void   iscissors_motion          (Tool *, GdkEventMotion *, gpointer);
static void   iscissors_control         (Tool *, ToolAction,       gpointer);
static void   iscissors_reset           (Iscissors *);
static void   iscissors_draw            (Tool *);
static void   iscissors_draw_CR         (GDisplay *, Iscissors *,
					 Point *, int *, int);
static void   CR_compose                (CRMatrix, CRMatrix, CRMatrix);
static int    add_segment               (int *, int, int);
static int    add_point                 (int *, int, int, int, double *);

/*  boundary localization routines  */
static void   normalize                 (double *);
static double dotprod                   (double *, double *);
static Kink * get_kink                  (Kink *, int, int);
static int    find_next_kink            (Kink *, int, int);
static double find_distance             (Kink *, int, int);
static int    go_distance               (Kink *, int, double, double *, double *);
static int    travel_length             (Kink *, int, int, int, int);
static int    find_edge_xy              (TempBuf *, int, double, double, double *);
static void   find_boundary             (Tool *);
static void   shape_of_boundary         (Tool *);
static void   process_kinks             (Tool *);
static void   initial_boundary          (Tool *);
static void   edge_map_from_boundary    (Tool *);
static void   orient_boundary           (Tool *);
static void   reset_boundary            (Tool *);
static int    localize_boundary         (Tool *);
static void   post_process_boundary     (Tool *);
static void   bezierify_boundary        (Tool *);

/*  edge map buffer utility functions  */
static TempBuf *  calculate_edge_map    (GImage *, int, int, int, int);
static void   construct_edge_map        (Tool *, TempBuf *);

/*  edge map blocks utility functions  */
static void   set_edge_map_blocks       (void *, int, int, int, int);
static void   allocate_edge_map_blocks  (int, int, int, int);
static void   free_edge_map_blocks      (void);

/*  gaussian & 1st derivative  */
static void   gaussian_deriv            (PixelRegion *, PixelRegion *, int, double);
static void   make_curve                (int *, int *, double, int);
static void   make_curve_d              (int *, int *, double, int);

/*  Catmull-Rom boundary conversion  */
static void   CR_convert                (Iscissors * , GDisplay *, int);
static void   CR_convert_points         (GdkPoint *, int);
static void   CR_convert_line           (GSList **, int, int, int, int);
static GSList * CR_insert_in_list       (GSList *, int);


/*  functions  */

static void
selection_scale_update (GtkAdjustment *adjustment,
			double        *scale_val)
{
  *scale_val = adjustment->value;
}

static void   
selection_to_bezier(GtkWidget *w,
		    gpointer   none)
{
   Iscissors *iscissors;

   if(last_tool)
     {
       iscissors = (Iscissors *) last_tool->private;
       last_tool->state = INACTIVE;
       bezierify_boundary (last_tool);
     }
}

static void
iscissors_options_reset (void)
{
  IScissorsOptions *options = iscissors_options;

  selection_options_reset ((SelectionOptions *) options);

  gtk_adjustment_set_value (GTK_ADJUSTMENT (options->resolution_w),
			    options->resolution_d);
  gtk_adjustment_set_value (GTK_ADJUSTMENT (options->threshold_w),
			    options->threshold_d);
  gtk_adjustment_set_value (GTK_ADJUSTMENT (options->elasticity_w),
			    options -> elasticity_d);
}

static IScissorsOptions *
iscissors_options_new (void)
{
  IScissorsOptions *options;

  GtkWidget *vbox;
  GtkWidget *abox;
  GtkWidget *table;
  GtkWidget *label;
  GtkWidget *scale;
  GtkWidget *convert_button;

  /*  the new intelligent scissors tool options structure  */
  options = (IScissorsOptions *) g_malloc (sizeof (IScissorsOptions));
  selection_options_init ((SelectionOptions *) options,
			  ISCISSORS,
			  iscissors_options_reset);
  options->resolution     = options->resolution_d     = 40.0;
  options->threshold      = options->threshold_d      = 15.0;
  options->elasticity     = options->elasticity_d     = 0.30;

  /*  the main vbox  */
  vbox = ((ToolOptions *) options)->main_vbox;

  /*  the resolution scale  */
  table = gtk_table_new (5, 2, FALSE);
  gtk_table_set_col_spacing (GTK_TABLE (table), 0, 6);
  gtk_table_set_row_spacing (GTK_TABLE (table), 1, 2);
  gtk_table_set_row_spacing (GTK_TABLE (table), 3, 1);
  gtk_box_pack_start (GTK_BOX (vbox), table, FALSE, FALSE, 0);

  label = gtk_label_new (_("Curve"));
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 1.0);
  gtk_table_attach (GTK_TABLE (table), label, 0, 1, 0, 1,
		    GTK_SHRINK | GTK_FILL, GTK_SHRINK, 0, 0);
  gtk_widget_show (label);

  label = gtk_label_new (_("Resolution:"));
  gtk_misc_set_alignment (GTK_MISC (label), 1.0, 1.0);
  gtk_table_attach (GTK_TABLE (table), label, 0, 1, 1, 2,
		    GTK_SHRINK | GTK_FILL, GTK_SHRINK, 0, 0);
  gtk_widget_show (label);

  abox = gtk_alignment_new (0.5, 1.0, 1.0, 0.0);
  gtk_table_attach (GTK_TABLE (table), abox, 1, 2, 0, 2,
		    GTK_EXPAND | GTK_FILL, GTK_SHRINK | GTK_FILL, 0, 0);
  gtk_widget_show (abox);

  options->resolution_w =
    gtk_adjustment_new (options->resolution_d, 1.0, 200.0, 1.0, 1.0, 0.0);
  scale = gtk_hscale_new (GTK_ADJUSTMENT (options->resolution_w));
  gtk_container_add (GTK_CONTAINER (abox), scale);
  gtk_scale_set_value_pos (GTK_SCALE (scale), GTK_POS_TOP);
  gtk_range_set_update_policy (GTK_RANGE (scale), GTK_UPDATE_DELAYED);
  gtk_signal_connect (GTK_OBJECT (options->resolution_w), "value_changed",
		      (GtkSignalFunc) selection_scale_update,
		      &options->resolution);
  gtk_widget_show (scale);

  /*  the threshold scale  */
  label = gtk_label_new (_("Edge Detect "));
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 1.0);
  gtk_table_attach (GTK_TABLE (table), label, 0, 1, 2, 3,
		    GTK_SHRINK | GTK_FILL, GTK_SHRINK, 0, 0);
  gtk_widget_show (label);

  label = gtk_label_new (_("Threshold:"));
  gtk_misc_set_alignment (GTK_MISC (label), 1.0, 1.0);
  gtk_table_attach (GTK_TABLE (table), label, 0, 1, 3, 4,
		    GTK_SHRINK | GTK_FILL, GTK_SHRINK, 0, 0);
  gtk_widget_show (label);

  abox = gtk_alignment_new (0.5, 1.0, 1.0, 0.0);
  gtk_table_attach (GTK_TABLE (table), abox, 1, 2, 2, 4,
		    GTK_EXPAND | GTK_FILL, GTK_SHRINK | GTK_FILL, 0, 0);
  gtk_widget_show (abox);

  options->threshold_w =
    gtk_adjustment_new (options->threshold_d, 1.0, 255.0, 1.0, 1.0, 0.0);
  scale = gtk_hscale_new (GTK_ADJUSTMENT (options->threshold_w));
  gtk_container_add (GTK_CONTAINER (abox), scale);
  gtk_scale_set_value_pos (GTK_SCALE (scale), GTK_POS_TOP);
  gtk_range_set_update_policy (GTK_RANGE (scale), GTK_UPDATE_DELAYED);
  gtk_signal_connect (GTK_OBJECT (options->threshold_w), "value_changed",
		      (GtkSignalFunc) selection_scale_update,
		      &options->threshold);
  gtk_widget_show (scale);

  /*  the elasticity scale  */
  label = gtk_label_new (_("Elasticity:"));
  gtk_misc_set_alignment (GTK_MISC (label), 1.0, 1.0);
  gtk_table_attach (GTK_TABLE (table), label, 0, 1, 4, 5,
		    GTK_SHRINK | GTK_FILL, GTK_SHRINK | GTK_FILL, 0, 0);
  gtk_widget_show (label);

  options->elasticity_w =
    gtk_adjustment_new (options->elasticity_d, 0.0, 1.0, 0.05, 0.05, 0.0);
  scale = gtk_hscale_new (GTK_ADJUSTMENT (options->elasticity_w));
  gtk_table_attach_defaults (GTK_TABLE (table), scale, 1, 2, 4, 5);
  gtk_scale_set_value_pos (GTK_SCALE (scale), GTK_POS_TOP);
  gtk_range_set_update_policy (GTK_RANGE (scale), GTK_UPDATE_DELAYED);
  gtk_signal_connect (GTK_OBJECT (options->elasticity_w), "value_changed",
		      (GtkSignalFunc) selection_scale_update,
		      &options -> elasticity);
  gtk_widget_show (scale);

  gtk_widget_show (table);

  /*  the convert to bezier button  */
  convert_button = gtk_button_new_with_label (_("Convert to Bezier Curve"));
  gtk_box_pack_start (GTK_BOX (vbox), convert_button, FALSE, FALSE, 1);
  gtk_signal_connect(GTK_OBJECT (convert_button) , "clicked",
			(GtkSignalFunc) selection_to_bezier,
			NULL);
  gtk_widget_show (convert_button);

  return options;
}


Tool *
tools_new_iscissors ()
{
  Tool * tool;
  Iscissors * private;

  /*  The tool options  */
  if (! iscissors_options)
    {
      iscissors_options = iscissors_options_new ();
      tools_register (ISCISSORS, (ToolOptions *) iscissors_options);
    }

  tool = (Tool *) g_malloc (sizeof (Tool));
  private = (Iscissors *) g_malloc (sizeof (Iscissors));

  private->core = draw_core_new (iscissors_draw);
  private->edge_buf = NULL;
  private->kinks = NULL;
  private->mask = NULL;

  tool->type = ISCISSORS;
  tool->state = INACTIVE;
  tool->scroll_lock = 0;  /*  Allow scrolling  */
  tool->auto_snap_to = FALSE;
  tool->private = (void *) private;

  tool->preserve = TRUE;
  tool->gdisp_ptr = NULL;
  tool->drawable = NULL;

  tool->button_press_func = iscissors_button_press;
  tool->button_release_func = iscissors_button_release;
  tool->motion_func = iscissors_motion;
  tool->arrow_keys_func = standard_arrow_keys_func;
  tool->modifier_key_func = standard_modifier_key_func;
  tool->cursor_update_func = rect_select_cursor_update;
  tool->control_func = iscissors_control;
  
  last_tool = tool;
  
  iscissors_reset (private);
  
  return tool;
}

void
tools_free_iscissors (Tool *tool)
{
  Iscissors * iscissors;

  iscissors = (Iscissors *) tool->private;

  if (tool->state == ACTIVE)
    draw_core_stop (iscissors->core, tool);
  draw_core_free (iscissors->core);

  iscissors_reset (iscissors);

  g_free (iscissors);
}


/*  Local functions  */

static void
iscissors_button_press (Tool           *tool,
			GdkEventButton *bevent,
			gpointer        gdisp_ptr)
{
  GDisplay *gdisp;
  GimpDrawable *drawable;
  Iscissors *iscissors;
  int replace, op;
  int x, y;

  last_tool = tool;
  gdisp = (GDisplay *) gdisp_ptr;
  iscissors = (Iscissors *) tool->private;
  drawable = gimage_active_drawable (gdisp->gimage);

  gdisplay_untransform_coords (gdisp, bevent->x, bevent->y,
			       &iscissors->x, &iscissors->y, FALSE, TRUE); 

  /*  If the tool was being used in another image...reset it  */

  if (tool->state == ACTIVE && gdisp_ptr != tool->gdisp_ptr)
    {
      draw_core_stop (iscissors->core, tool);
      iscissors_reset (iscissors);
    }

  switch (iscissors->state)
    {
    case FREE_SELECT_MODE:
      tool->state = ACTIVE;
      last_tool = NULL;
      tool->gdisp_ptr = gdisp_ptr;

      gdk_pointer_grab (gdisp->canvas->window, FALSE,
			(GDK_POINTER_MOTION_HINT_MASK |
			 GDK_BUTTON1_MOTION_MASK |
			 GDK_BUTTON_RELEASE_MASK),
			NULL, NULL, bevent->time);

      if (bevent->state & GDK_MOD1_MASK)
	{
	  init_edit_selection (tool, gdisp_ptr, bevent, MaskTranslate);
	  return;
	}
      else if (!(bevent->state & GDK_SHIFT_MASK) && !(bevent->state & GDK_CONTROL_MASK))
	if (! (layer_is_floating_sel (gimage_get_active_layer (gdisp->gimage))) &&
	    gdisplay_mask_value (gdisp, bevent->x, bevent->y) > HALF_WAY)
	  {
	    /*  Have to blank out the edge blocks since they might change  */
	    iscissors_reset (iscissors);

	    init_edit_selection (tool, gdisp_ptr, bevent, MaskToLayerTranslate);
	    return;
	  }

      /*  If the edge map blocks haven't been allocated, do so now  */
      if (!edge_map_blocks)
	allocate_edge_map_blocks (BLOCK_WIDTH, BLOCK_HEIGHT,
				  drawable_width(drawable),
				  drawable_height(drawable));
				  
      iscissors->num_segs = 0;
	x = bevent->x;
	y = bevent->y;
	
      add_segment (&(iscissors->num_segs), x, y);

      draw_core_start (iscissors->core,
		       gdisp->canvas->window,
		       tool);
      break;
    case BOUNDARY_MODE:
      if (/*channel_value (iscissors->mask, iscissors->x, iscissors->y)*/ TRUE)
	{
	  replace = 0;

	  if ((bevent->state & GDK_SHIFT_MASK) && !(bevent->state & GDK_CONTROL_MASK))
	    op = ADD;
	  else if ((bevent->state & GDK_CONTROL_MASK) && !(bevent->state & GDK_SHIFT_MASK))
	    op = SUB;
	  else if ((bevent->state & GDK_CONTROL_MASK) && (bevent->state & GDK_SHIFT_MASK))
	    op = INTERSECT;
	  else
	    {
	      op = ADD;
	      replace = 1;
	    }

	  tool->state = INACTIVE;

	  /*  If we're antialiased, then recompute the
	   *  mask...
	   */
	  if (((SelectionOptions *) iscissors_options)->antialias)
	    CR_convert (iscissors, tool->gdisp_ptr, TRUE);

	  draw_core_stop (iscissors->core, tool);

	  if (replace)
	    gimage_mask_clear (gdisp->gimage);
	  else
	    gimage_mask_undo (gdisp->gimage);

	  if (((SelectionOptions *) iscissors_options)->feather)
	    channel_feather (iscissors->mask,
			     gimage_get_mask (gdisp->gimage),
			     ((SelectionOptions *) iscissors_options)->feather_radius,
			     ((SelectionOptions *) iscissors_options)->feather_radius,
			     op, 0, 0);
	  else
	    channel_combine_mask (gimage_get_mask (gdisp->gimage),
				  iscissors->mask, op, 0, 0);

	  iscissors_reset (iscissors);

	  gdisplays_flush ();
	}

      break;
    }
}

static void
iscissors_button_release (Tool           *tool,
			  GdkEventButton *bevent,
			  gpointer        gdisp_ptr)
{
  Iscissors *iscissors;
  GDisplay *gdisp;

  gdisp = (GDisplay *) gdisp_ptr;
  iscissors = (Iscissors *) tool->private;

  /*return;*/
  
  last_tool = tool;
  
  gdk_pointer_ungrab (bevent->time);
  gdk_flush ();

  draw_core_stop (iscissors->core, tool);

  /*  First take care of the case where the user "cancels" the action  */
  if (! (bevent->state & GDK_BUTTON3_MASK))
    {
      /*  Progress to the next stage of intelligent selection  */
      switch (iscissors->state)
	{
	case FREE_SELECT_MODE:
	  /*  Add one additional segment  */
	  add_segment (&(iscissors->num_segs), segs[0].x1, segs[0].y1);

	  if (iscissors->num_segs >= 3)
	    {
	      /*  Find the boundary  */
	      find_boundary (tool);

	      /*  Set the new state  */
	      iscissors->state = BOUNDARY_MODE;

	      /*  Start the draw core up again  */
	      draw_core_resume (iscissors->core, tool);

	      return;
	    }
	  break;
	case BOUNDARY_MODE:
	  iscissors->state = FREE_SELECT_MODE;
	  break;
	}
    }

  tool->state = INACTIVE;
}

static void
iscissors_motion (Tool           *tool,
		  GdkEventMotion *mevent,
		  gpointer        gdisp_ptr)
{
  Iscissors *iscissors;
  GDisplay *gdisp;
  int x, y;
  
  if (tool->state != ACTIVE)
    return;

  gdisp = (GDisplay *) gdisp_ptr;
  iscissors = (Iscissors *) tool->private;

  
  switch (iscissors->state)
    {
    case FREE_SELECT_MODE:
    	x = mevent->x;
	y = mevent->y;
      if (add_segment (&(iscissors->num_segs), x, y))
	
	gdk_draw_segments (iscissors->core->win, iscissors->core->gc,
			   segs + (iscissors->num_segs - 1), 1);
      break;
    case BOUNDARY_MODE:
      break;
    }
}

static void
iscissors_draw (Tool *tool)
{
  GDisplay *gdisp;
  Iscissors *iscissors;
  int indices[4];
  int i;

  gdisp = (GDisplay *) tool->gdisp_ptr;
  iscissors = (Iscissors *) tool->private;

  switch (iscissors->state)
    {
    case FREE_SELECT_MODE:
      gdk_draw_segments (iscissors->core->win, iscissors->core->gc,
			 segs, iscissors->num_segs);
      break;
    case BOUNDARY_MODE:
      for (i = 0; i < iscissors->num_pts; i ++)
	{
	  indices[0] = (i < 3) ? (iscissors->num_pts + i - 3) : (i - 3);
	  indices[1] = (i < 2) ? (iscissors->num_pts + i - 2) : (i - 2);
	  indices[2] = (i < 1) ? (iscissors->num_pts + i - 1) : (i - 1);
	  indices[3] = i;
	  iscissors_draw_CR (gdisp, iscissors, pts, indices, SCREEN_COORDS);
	}
      break;
    }
}

static void
iscissors_draw_CR (GDisplay  *gdisp,
		   Iscissors *iscissors,
		   Point     *pts,
		   int       *indices,
		   int        draw_type)
{
#define ROUND(x)  ((int) ((x) + 0.5))

  static GdkPoint gdk_points[256];
  static int npoints = 256;

  CRMatrix geometry;
  CRMatrix tmp1, tmp2;
  CRMatrix deltas;
  double x, dx, dx2, dx3;
  double y, dy, dy2, dy3;
  double d, d2, d3;
  int lastx, lasty;
  int newx, newy;
  /* int tx, ty; */
  int index;
  int i;

  GimpDrawable *drawable;
  drawable = gimage_active_drawable (gdisp->gimage);

  /* construct the geometry matrix from the segment */
  /* assumes that a valid segment containing 4 points is passed in */

  for (i = 0; i < 4; i++)
    {
      switch (draw_type)
	{
	case IMAGE_COORDS:
	  geometry[i][0] = pts[indices[i]].dx;
	  geometry[i][1] = pts[indices[i]].dy;
	  break;
	case AA_IMAGE_COORDS:
	  geometry[i][0] = pts[indices[i]].dx * SUPERSAMPLE;
	  geometry[i][1] = pts[indices[i]].dy * SUPERSAMPLE;
	  break;
	case SCREEN_COORDS:
	  gdisplay_transform_coords_f(gdisp, (int) pts[indices[i]].dx, 
			  (int) pts[indices[i]].dy, &x, &y, TRUE);
	  geometry[i][0] = x;
	  geometry[i][1] = y;
	  /*g_print("%f %f\n", x, y);*/
	  break;
	}
      geometry[i][2] = 0;
      geometry[i][3] = 0;
    }

  /* subdivide the curve n times */
  /* n can be adjusted to give a finer or coarser curve */

  d = 1.0 / SUBDIVIDE;
  d2 = d * d;
  d3 = d * d * d;

  /* construct a temporary matrix for determining the forward differencing deltas */

  tmp2[0][0] = 0;     tmp2[0][1] = 0;     tmp2[0][2] = 0;    tmp2[0][3] = 1;
  tmp2[1][0] = d3;    tmp2[1][1] = d2;    tmp2[1][2] = d;    tmp2[1][3] = 0;
  tmp2[2][0] = 6*d3;  tmp2[2][1] = 2*d2;  tmp2[2][2] = 0;    tmp2[2][3] = 0;
  tmp2[3][0] = 6*d3;  tmp2[3][1] = 0;     tmp2[3][2] = 0;    tmp2[3][3] = 0;

  /* compose the basis and geometry matrices */
  CR_compose (CR_basis, geometry, tmp1);

  /* compose the above results to get the deltas matrix */
  CR_compose (tmp2, tmp1, deltas);

  /* extract the x deltas */
  x = deltas[0][0];
  dx = deltas[1][0];
  dx2 = deltas[2][0];
  dx3 = deltas[3][0];

  /* extract the y deltas */
  y = deltas[0][1];
  dy = deltas[1][1];
  dy2 = deltas[2][1];
  dy3 = deltas[3][1];

  lastx = x;
  lasty = y;

  gdk_points[0].x = lastx;
  gdk_points[0].y = lasty;
  index = 1;

  /* loop over the curve */
  for (i = 0; i < SUBDIVIDE; i++)
    {
      /* increment the x values */
      x += dx;
      dx += dx2;
      dx2 += dx3;

      /* increment the y values */
      y += dy;
      dy += dy2;
      dy2 += dy3;

      newx = ROUND(x);
      newy = ROUND(y);

      /* if this point is different than the last one...then draw it */
      if ((lastx != newx) || (lasty != newy))
	{
	  /* add the point to the point buffer */
		      	  
	  /*gdisplay_transform_coords (gdisp, newx, newy, &tx, &ty,1 );*/
	  /*drawable_offsets(drawable, &tx, &ty);
	  tx += newx;
	  ty += newy;*/
	  
	  gdk_points[index].x = newx;
	  gdk_points[index].y = newy;


	  index++;
	  /* if the point buffer is full put it to the screen and zero it out */
	  if (index >= npoints)
	    {
	      switch (draw_type)
		{
		case IMAGE_COORDS: case AA_IMAGE_COORDS:
		  CR_convert_points (gdk_points, index);
		  break;
		case SCREEN_COORDS:
		  gdk_draw_points (iscissors->core->win, iscissors->core->gc,
				   gdk_points, index);
		  break;
		}
	      index = 0;
	    }
	}

      lastx = newx;
      lasty = newy;
    }

  /* if there are points in the buffer, then put them on the screen */
  if (index)
    switch (draw_type)
      {
      case IMAGE_COORDS: case AA_IMAGE_COORDS:
	CR_convert_points (gdk_points, index);
	break;
      case SCREEN_COORDS:
	gdk_draw_points (iscissors->core->win, iscissors->core->gc,
			 gdk_points, index);
	break;
      }
}

static void
iscissors_control (Tool       *tool,
		   ToolAction  action,
		   gpointer    gdisp_ptr)
{
  Iscissors * iscissors;

  iscissors = (Iscissors *) tool->private;

  switch (action)
    {
    case PAUSE:
      draw_core_pause (iscissors->core, tool);
      break;

    case RESUME:
      draw_core_resume (iscissors->core, tool);
      break;

    case HALT:
      draw_core_stop (iscissors->core, tool);
      iscissors_reset (iscissors);
      break;

    default:
      break;
    }
}

static void
iscissors_reset (Iscissors *iscissors)
{
  /*  Reset the edge map blocks structure  */
  free_edge_map_blocks ();

  /*  free edge buffer  */
  if (iscissors->edge_buf)
    temp_buf_free (iscissors->edge_buf);

  /*  free mask  */
  if (iscissors->mask)
    channel_delete (iscissors->mask);

  /*  Free kinks  */
  if (iscissors->kinks)
    g_free (iscissors->kinks);

  iscissors->state = FREE_SELECT_MODE;
  iscissors->mask = NULL;
  iscissors->edge_buf = NULL;
  iscissors->kinks = NULL;
  iscissors->num_segs = 0;
  iscissors->num_pts = 0;
  iscissors->num_kinks = 0;
}

static int
add_segment (int *num_segs,
	     int  x,
	     int  y)
{
  static int first = 1;

  if (*num_segs >= max_segs)
    {
      max_segs += DEFAULT_MAX_INC;

      segs = (GdkSegment *) g_realloc ((void *) segs, sizeof (GdkSegment) * max_segs);

      if (!segs)
	fatal_error ("Unable to reallocate segment array in iscissors.");
    }

  if (*num_segs)
    {
      segs[*num_segs].x1 = segs[*num_segs - 1].x2;
      segs[*num_segs].y1 = segs[*num_segs - 1].y2;
    }
  else if (first)
    {
      segs[0].x1 = x;
      segs[0].y1 = y;
    }

  segs[*num_segs].x2 = x;
  segs[*num_segs].y2 = y;

  if (! *num_segs && first)
    first = 0;
  else
    {
      (*num_segs)++;
      first = 1;
    }

  return 1;
}


static int
add_point (int    *num_pts,
	   int     kink,
	   int     x,
	   int     y,
	   double *normal)
{
  if (*num_pts >= max_pts)
    {
      max_pts += DEFAULT_MAX_INC;

      pts = (Point *) g_realloc ((void *) pts, sizeof (Point) * max_pts);

      if (!pts)
	fatal_error ("Unable to reallocate points array in iscissors.");
    }

  pts[*num_pts].x = x;
  pts[*num_pts].y = y;
  pts[*num_pts].kink = kink;
  pts[*num_pts].normal[0] = normal[0];
  pts[*num_pts].normal[1] = normal[1];

  (*num_pts)++;

  return 1;
}

static void
CR_compose (CRMatrix a,
	    CRMatrix b,
	    CRMatrix ab)
{
  int i, j;

  for (i = 0; i < 4; i++)
    {
      for (j = 0; j < 4; j++)
        {
          ab[i][j] = (a[i][0] * b[0][j] +
                      a[i][1] * b[1][j] +
                      a[i][2] * b[2][j] +
                      a[i][3] * b[3][j]);
        }
    }
}

static void
normalize (double *vec)
{
  double length;

  length = sqrt (SQR (vec[0]) + SQR (vec[1]));
  if (length)
    {
      vec[0] /= length;
      vec[1] /= length;
    }
}

static double
dotprod (double *vec1,
	 double *vec2)
{
  double val;

  val = vec1[0] * vec2[0] + vec1[1] * vec2[1];

  return val;
}

static Kink *
get_kink (Kink *kinks,
	  int   index,
	  int   num_kinks)
{
  if (index >= 0 && index < num_kinks)
    return kinks + index;
  else if (index < 0)
    {
      while (index < 0)
	index += num_kinks;
      return kinks + index;
    }
  else
    {
      while (index >= num_kinks)
	index -= num_kinks;
      return kinks + index;
    }

  /* I don't think it ever gets to this point -- Rockwalrus */
  return NULL;
}

static int
find_next_kink (Kink *kinks,
		int   num_kinks,
		int   this)
{
  if (this >= num_kinks)
    return 0;

  do {
    this++;
  } while (! kinks[this].is_a_kink);

  return this;
}

static double
find_distance (Kink *kinks,
	       int   this,
	       int   next)
{
  double dist = 0.0;
  double dx, dy;

  while (this != next)
    {
      dx = kinks[this].x - kinks[this + 1].x;
      dy = kinks[this].y - kinks[this + 1].y;

      dist += sqrt (SQR (dx) + SQR (dy));

      this ++;
    }

  return dist;
}

static int
go_distance (Kink   *kinks,
	     int     this,
	     double  dist,
	     double *x,
	     double *y)
{
  double dx, dy;
  double length = 0.0;
  double t = 2.0;

  dx = dy = 0.0;

  if (dist == 0.0)
    {
      *x = kinks[this].x;
      *y = kinks[this].y;
      return 1;
    }

  while (dist > 0.0)
    {
      dx = kinks[this + 1].x - kinks[this].x;
      dy = kinks[this + 1].y - kinks[this].y;
      length = sqrt (SQR (dx) + SQR (dy));
      dist -= length;

      if (dist > 0.0)
	this++;
    }

  t = (length + dist) / length;
  *x = kinks[this].x + t * dx;
  *y = kinks[this].y + t * dy;

  return this;
}

static int
travel_length (Kink *kinks,
	       int   num_kinks,
	       int   start,
	       int   dir,
	       int   dist)
{
  double dx, dy;
  Kink * k1, * k2;
  double distance = dist;
  int length = 0;

  while (distance > 0)
    {
      k1 = get_kink (kinks, start, num_kinks);
      k2 = get_kink (kinks, start+dir, num_kinks);
      dx = k2->x - k1->x;
      dy = k2->y - k1->y;
      distance -= sqrt (SQR (dx) + SQR (dy));

      start += dir;
      length += dir;
    }

  /*  backup one step and return value  */
  return length;
}

static int
find_edge_xy (TempBuf *edge_buf,
	      int      dir,
	      double   x,
	      double   y,
	      double  *edge)
{
  double dx, dy;
  int ix, iy;
  int xx, yy;
  int rowstride, bytes;
  int d11, d12, d21, d22;
  unsigned char * data;
  int b;
  int threshold = (int) iscissors_options->threshold;

  bytes = edge_buf->bytes;
  rowstride = bytes * edge_buf->width;

  x -= edge_buf->x;
  y -= edge_buf->y;

  ix = (int) (x + 2) - 2;
  iy = (int) (y + 2) - 2;

  /*  If we're scouting out of bounds, return 0  */
  if (ix < 0 || ix >= edge_buf->width ||
      iy < 0 || iy >= edge_buf->height)
    {
      edge[0] = EDGE_STRENGTH;
      return 1;
    }

  if (dir > 0)
    {
      dx = x - ix;
      dy = y - iy;
    }
  else
    {
      dx = 1 - (x - ix);
      dy = 1 - (y - iy);
    }

  data = temp_buf_data (edge_buf) + iy * rowstride + ix * bytes;

  for (b = 0; b < bytes; b++)
    {
      if (dir > 0)
	{
	  xx = ((ix + 1) >= edge_buf->width) ? 0 : bytes;
	  yy = ((iy + 1) >= edge_buf->height) ? 0 : rowstride;
	}
      else
	{
	  xx = ((ix - 1) < 0) ? 0 : -bytes;
	  yy = ((iy - 1) < 0) ? 0 : -rowstride;
	}

      d11 = (data[0] > threshold) ? data[0] : 0;
      d12 = (data[xx] > threshold) ? data[xx] : 0;
      d21 = (data[yy] > threshold) ? data[yy] : 0;
      d22 = (data[xx + yy] > threshold) ? data[xx + yy] : 0;

      edge[b] = BILINEAR (d11, d12, d21, d22, dx, dy);
      data++;
    }

  if (edge[0] > 0.0)
    return 1;
  else
    return 0;
}

static void
find_boundary (Tool *tool)
{
  
  /*  Find directional changes  */
  shape_of_boundary (tool);

  /*  Process the kinks  */
  process_kinks (tool);

  /*  Determine the initial boundary  */
  initial_boundary (tool);

  /*  Get the edge map from the boundary extents  */
  edge_map_from_boundary (tool);

  /*  Orient the boundary based on edge detection  */
  orient_boundary (tool);

  /*  Setup the points array for localization  */
  reset_boundary (tool);

  /*  Localize the boundary based on edge detection
   *  and inter-segment elasticity
   */
  while (localize_boundary (tool)) ;

  /*  Post process the points array to fit non-edge-seeking
   *  boundary points into the scheme of things
   */
  post_process_boundary (tool);

  /*  convert the boundary into a mask  */
  CR_convert ((Iscissors *) tool->private,
	      (GDisplay *) tool->gdisp_ptr, FALSE);
}

static void
shape_of_boundary (Tool *tool)
{
  Iscissors * iscissors;
  GDisplay * gdisp;
  Kink * kinks, * k;
  double vec1[2], vec2[2], vec[2];
  double std_dev;
  double weight;
  int left, right;
  int i, j;
  int resolution = (int) iscissors_options->resolution;
  /* int x, y; */

  /*  This function determines the kinkiness at each point in the
   *  original free-hand curve by finding the dotproduct between
   *  the two vectors formed at each point from that point to its
   *  immediate neighbors.  A smoothing function is applied to
   *  determine the vectors to ameliorate the otherwise excessive
   *  jitter associated with original selection.
   */

  gdisp = (GDisplay *) tool->gdisp_ptr;
  iscissors = (Iscissors *) tool->private;

  iscissors->num_kinks = iscissors->num_segs;
  if (iscissors->kinks)
    g_free (iscissors->kinks);
  kinks = iscissors->kinks = (Kink *) g_malloc (sizeof (Kink) *
					       (iscissors->num_kinks + 1));

  for (i = 0,j=0; i < iscissors->num_kinks; i++)
    {
	/* untransform coords */
     gdisplay_untransform_coords (gdisp, segs[i].x1, segs[i].y1,
		       &kinks[j].x, &kinks[j].y, FALSE, TRUE);
	    
/*	   
      kinks[j].x = segs[i].x1;
      kinks[j].y = segs[i].y1;
*/
      
	if(j) {
	   if((kinks[j].x != kinks[j-1].x) || (kinks[j].y != kinks[j-1].y))
		++j;
	} else j++;
    }

  iscissors->num_kinks = j;

  for (i = 0; i < iscissors->num_kinks; i++)
    {
      left = travel_length (kinks, iscissors->num_kinks, i, -1, resolution);
      right = travel_length (kinks, iscissors->num_kinks, i, 1, resolution);

      std_dev = sqrt (-(SQR (left)) / (2 * log (EPSILON)));
      if (fabs (std_dev) < EPSILON) std_dev = 1.0;

      vec1[0] = 0.0;
      vec1[1] = 0.0;
      for (j = left; j < 0; j++)
	{
	  k = get_kink (kinks, i+j, iscissors->num_kinks);
	  vec[0] = k->x - kinks[i].x;
	  vec[1] = k->y - kinks[i].y;
	  normalize (vec);

	  weight = exp (-SQR(j+1) / (2 * SQR (std_dev)));
	  vec1[0] += weight * vec[0];
	  vec1[1] += weight * vec[1];
	}
      normalize (vec1);

      std_dev = sqrt (-(SQR (right)) / (2 * log (EPSILON)));
      if (fabs (std_dev) < EPSILON) std_dev = 1.0;

      vec2[0] = 0.0;
      vec2[1] = 0.0;
      for (j = 1; j <= right; j++)
	{
	  k = get_kink (kinks, i+j, iscissors->num_kinks);
	  vec[0] = k->x - kinks[i].x;
	  vec[1] = k->y - kinks[i].y;
	  normalize (vec);

	  weight = exp (-SQR(j-1) / (2 * SQR (std_dev)));
	  vec2[0] += weight * vec[0];
	  vec2[1] += weight * vec[1];
	}
      normalize (vec2);

      /*  determine the kinkiness based on the two vectors  */
      kinks[i].kinkiness = (M_PI - acos (dotprod (vec1, vec2)))/ M_PI;

      kinks[i].normal[0] = (vec1[0] + vec2[0]) / 2.0;
      kinks[i].normal[1] = (vec1[1] + vec2[1]) / 2.0;

      /*  if the average vector is zero length...  */
      if (kinks[i].normal[0] < EPSILON && kinks[i].normal[1] < EPSILON)
	{
	  /*  normal = 90 degree rotation of vec1  */
	  kinks[i].normal[0] = -vec1[1];
	  kinks[i].normal[1] = vec1[0];
	}
      normalize (kinks[i].normal);
    }
}

static void
process_kinks (Tool *tool)
{
  Iscissors * iscissors;
  GDisplay * gdisp;
  Kink * kinks, * k_left, * k_right;
  /* int x, y; */
  int i;
  GimpDrawable *drawable;


  gdisp = (GDisplay *) tool->gdisp_ptr;
  drawable = gimage_active_drawable (gdisp->gimage);
  iscissors = (Iscissors *) tool->private;
  kinks = iscissors->kinks;

  for (i = 0; i < iscissors->num_kinks; i++)
    {

      /*FIXME*/
      kinks[i].x = BOUNDS (kinks[i].x, 0, (drawable_width(drawable) - 1));
      kinks[i].y = BOUNDS (kinks[i].y, 0, (drawable_height(drawable) - 1));

      /*  get local maximums  */
      k_left = get_kink (kinks, i-1, iscissors->num_kinks);
      k_right = get_kink (kinks, i+1, iscissors->num_kinks);

      if ((kinks[i].kinkiness > k_left->kinkiness) &&
	  (kinks[i].kinkiness >= k_right->kinkiness) &&
	  (kinks[i].kinkiness > kink_thres))
	kinks[i].is_a_kink = 1;
      else
	kinks[i].is_a_kink = 0;
    }
}

static void
initial_boundary (Tool *tool)
{
  Iscissors * iscissors;
  GDisplay * gdisp;
  Kink * kinks;
  double x, y;
  double dist;
  double res;
  double i_resolution = 1.0 / iscissors_options->resolution;
  int i, n, this, next, k;
  int num_pts = 0;

  gdisp = (GDisplay *) tool->gdisp_ptr;
  iscissors = (Iscissors *) tool->private;
  kinks = iscissors->kinks;

  /*  for a connected boundary, set up the last kink as the same
   *  x & y coordinates as the first
   */
  kinks[iscissors->num_kinks].x = kinks[0].x;
  kinks[iscissors->num_kinks].y = kinks[0].y;
  kinks[iscissors->num_kinks].is_a_kink = 1;

  this = 0;
  while ((next = find_next_kink (kinks, iscissors->num_kinks, this)))
    {
      /*  Find the distance in pixels from the current to
       *  the next kink
       */
      dist = find_distance (kinks, this, next);

      if (dist > 0.0)
	{
	  /*  Find the number of segments that should be created
	   *  to fill the void
	   */
	  n = (int) (dist * i_resolution);
	  res = dist / (double) (n + 1);

	  add_point (&num_pts, 1, kinks[this].x, kinks[this].y, kinks[this].normal);

	  for (i = 1; i <= n; i++)
	    {
	      k = go_distance (kinks, this, res * i, &x, &y);

	      add_point (&num_pts, 0, (int) x, (int) y, kinks[k].normal);
	    }
	}

      this = next;
    }

  iscissors->num_pts = num_pts;
}

static void
edge_map_from_boundary (Tool *tool)
{
  Iscissors * iscissors;
  GDisplay * gdisp;
  unsigned char black[EDGE_WIDTH] = { 0 };
  int x, y, w, h;
  int x1, y1, x2, y2;
  int i;
  GimpDrawable *drawable;

  
  gdisp = (GDisplay *) tool->gdisp_ptr;
  drawable = gimage_active_drawable (gdisp->gimage);
  iscissors = (Iscissors *) tool->private;

  x = y = w = h = x1 = y1 = x2 = y2 = 0;
  
  x1 = drawable_width(drawable);
  y1 = drawable_height(drawable);

  /*  Find the edge map extents  */
  for (i = 0; i < iscissors->num_pts; i++)
    {
      x = BOUNDS (pts[i].x - LOCALIZE_RADIUS, 0,
		  drawable_width(drawable));
      y = BOUNDS (pts[i].y - LOCALIZE_RADIUS, 0,
		  drawable_height(drawable));
      w = BOUNDS (pts[i].x + LOCALIZE_RADIUS, 0,
		  drawable_width(drawable));
      h = BOUNDS (pts[i].y + LOCALIZE_RADIUS, 0,
		  drawable_height(drawable));
		  
      w -= x;
      h -= y;

      set_edge_map_blocks (gdisp->gimage, x, y, w, h);

      if (x < x1)
	x1 = x;
      if (y < y1)
	y1 = y;
      if (x + w > x2)
	x2 = x + w;
      if (y + h > y2)
	y2 = y + h;
    }


  /*  construct the edge map  */
  iscissors->edge_buf = temp_buf_new ((x2 - x1), (y2 - y1),
				      EDGE_WIDTH, x1, y1, black);
  construct_edge_map (tool, iscissors->edge_buf);
}

static void
orient_boundary (Tool *tool)
{
  Iscissors * iscissors;
  GDisplay * gdisp;
  int e1, e2;
  double dx1, dy1, dx2, dy2;
  double edge1[EDGE_WIDTH], edge2[EDGE_WIDTH];
  double max;
  double angle;
  int dir = 0;
  int i, j;
  int max_dir;
  int max_orient;
  int found;

  max_dir = 0;
  max_orient = 0;

  gdisp = (GDisplay *) tool->gdisp_ptr;
  iscissors = (Iscissors *) tool->private;

  for (i = 0; i < iscissors->num_pts; i++)
    {
      /*  Search for the closest edge  */
      j = 0;
      max = 0.0;
      found = 0;

      angle = atan2 (pts[i].normal[1], pts[i].normal[0]);
      dir = ((angle > -3 * M_PI_4) && (angle < M_PI_4)) ? 1 : -1;

      while (j < LOCALIZE_RADIUS && !found)
	{
	  dx1 = pts[i].x + pts[i].normal[0] * j;
	  dy1 = pts[i].y + pts[i].normal[1] * j;
	  e1 = find_edge_xy (iscissors->edge_buf, dir, dx1, dy1, edge1);

	  dx2 = pts[i].x - pts[i].normal[0] * j;
	  dy2 = pts[i].y - pts[i].normal[1] * j;
	  e2 = find_edge_xy (iscissors->edge_buf, -dir, dx2, dy2, edge2);

	  e1 = e2 = 0;
	  if (e1 && e2)
	    {
	      if (edge1[0] > edge2[0])
		pts[i].dir = dir;
	      else
		{
		  pts[i].normal[0] *= -1;
		  pts[i].normal[1] *= -1;
		  pts[i].dir = -dir;
		}
	      found = 1;
	    }
	  else if (e1)
	    {
	      pts[i].dir = dir;
	      found = 1;
	    }
	  else if (e2)
	    {
	      pts[i].dir = -dir;
	      pts[i].normal[0] *= -1;
	      pts[i].normal[1] *= -1;
	      found = 1;
	    }
	  else
	    {
	      if (edge1[0] > max)
		{
		  max = edge1[0];
		  max_orient = 1;
		  max_dir = dir;
		}
	      if (edge2[0] > max)
		{
		  max = edge2[0];
		  max_orient = -1;
		  max_dir = -dir;
		}
	    }

	  j++;
	}

      if (!found && max > miss_thres)
	{
	  pts[i].normal[0] *= max_orient;
	  pts[i].normal[1] *= max_orient;
	  pts[i].dir = max_dir;
	}
      else if (!found)
	{
	  pts[i].normal[0] = 0.0;
	  pts[i].normal[1] = 0.0;
	  pts[i].dir = 0;
	}
    }
}

static void
reset_boundary (Tool *tool)
{
  Iscissors * iscissors;
  GDisplay * gdisp;
  double edge[EDGE_WIDTH];
  int i;

  gdisp = (GDisplay *) tool->gdisp_ptr;
  iscissors = (Iscissors *) tool->private;

  for (i = 0; i < iscissors->num_pts; i++)
    {
      if (pts[i].dir == 0)
	pts[i].stable = 1;
      else if (find_edge_xy (iscissors->edge_buf,
			     pts[i].dir, pts[i].x, pts[i].y, edge))
	pts[i].stable = 1;
      else
	pts[i].stable = 0;
      pts[i].dx = pts[i].x;
      pts[i].dy = pts[i].y;
    }
}

static int
localize_boundary (Tool *tool)
{
  Iscissors * iscissors;
  GDisplay * gdisp;
  double x, y;
  double dx, dy;
  double max;
  double d_l, d_r;
  double edge[EDGE_WIDTH];
  int i, left, right;
  int moved = 0;
  double elasticity = iscissors_options->elasticity + 1.0;

  gdisp = (GDisplay *) tool->gdisp_ptr;
  iscissors = (Iscissors *) tool->private;

  /*  this function lets the boundary crawl in its desired
   *  direction, but within limits set by the elasticity
   *  variable.  The process is incremental, so this function
   *  needs to be repeatedly called until there is no discernable
   *  movement
   */

  for (i = 0; i < iscissors->num_pts; i++)
    {
      if (!pts[i].stable)
	{
	  x = pts[i].dx + pts[i].normal[0];
	  y = pts[i].dy + pts[i].normal[1];

	  left = (i == 0) ? iscissors->num_pts - 1 : i - 1;
	  right = (i == (iscissors->num_pts - 1)) ? 0 : i + 1;

	  dx = x - pts[left].dx;
	  dy = y - pts[left].dy;
	  d_l = sqrt (SQR (dx) + SQR (dy));

	  dx = x - pts[right].dx;
	  dy = y - pts[right].dy;
	  d_r = sqrt (SQR (dx) + SQR (dy));

	  dx = pts[left].dx - pts[right].dx;
	  dy = pts[left].dy - pts[right].dy;
	  max = (sqrt (SQR (dx) + SQR (dy)) / 2.0) * elasticity;

	  /*  If moving the point along it's directional vector
	   *  still satisfies the elasticity constraints (OR)
	   *  the point is a kink (in which case it can violate
	   *  elasticity completely.
	   */
	  if (((d_l < max) && (d_r < max)) || pts[i].kink)
	    {
	      pts[i].dx = x;
	      pts[i].dy = y;
	      if (find_edge_xy (iscissors->edge_buf, pts[i].dir, x, y, edge))
		pts[i].stable = 1;
	      else
		moved++;
	    }
	}
    }

  return moved;
}

static void
post_process_boundary (Tool *tool)
{
  Iscissors * iscissors;
  GDisplay * gdisp;
  int i;
  int left, right;

  gdisp = (GDisplay *) tool->gdisp_ptr;
  iscissors = (Iscissors *) tool->private;

  /*  Relocate all points which did not manage to seek an edge
   *  to the average of their edge-seeking neighbors
   *  Also relocate points which failed to reach a stable
   *  edge position.  These cases indicate that the point
   *  somehow slipped through the cracks and is headed towards
   *  lands unknown and most likely undesired.
   */
  for (i = 0; i < iscissors->num_pts; i++)
    {
/*   iff you uncomment this, change it to use the drawable width&height
   pts[i].x = BOUNDS (pts[i].x, 0, (gdisp->gimage->width - 1));
      pts[i].y = BOUNDS (pts[i].y, 0, (gdisp->gimage->height - 1));
      pts[i].dx = BOUNDS (pts[i].dx, 0, (gdisp->gimage->width - 1));
      pts[i].dy = BOUNDS (pts[i].dy, 0, (gdisp->gimage->height - 1));
*/   
      if (pts[i].dir == 0 || pts[i].stable == 0)
	{
	  left = (i == 0) ? iscissors->num_pts - 1 : i - 1;
	  right = (i == (iscissors->num_pts - 1)) ? 0 : i + 1;

	  if (pts[left].stable && pts[right].stable)
	    {
	      pts[i].dx = (pts[left].dx + pts[right].dx) / 2.0;
	      pts[i].dy = (pts[left].dy + pts[right].dy) / 2.0;
	    }
	}
    }

  /*  connect the boundary  */
  pts[iscissors->num_pts].dx = pts[0].dx;
  pts[iscissors->num_pts].dy = pts[0].dy;
}

static void
bezierify_boundary (Tool *tool)
{
  Iscissors * iscissors;
  GDisplay * gdisp;
  CRMatrix geometry;
  CRMatrix bezier_geom;
  BezierPoint * bez_pts;
  BezierPoint * new_pt;
  BezierPoint * last_pt;
  int indices[4];
  int i, j, off_x, off_y;

  gdisp = (GDisplay *) tool->gdisp_ptr;
  iscissors = (Iscissors *) tool->private;
  draw_core_stop (iscissors->core, tool);

  if (iscissors->num_pts < 4)
    {
      g_message (_("Boundary contains < 4 points!  Cannot bezierify."));
      return;
    }

  bez_pts = NULL;
  last_pt = NULL;

  drawable_offsets (GIMP_DRAWABLE (gdisp->gimage->active_layer),
		      &off_x, &off_y);
    
  for (i = 0; i < iscissors->num_pts; i ++)
    {
      indices[0] = (i < 3) ? (iscissors->num_pts + i - 3) : (i - 3);
      indices[1] = (i < 2) ? (iscissors->num_pts + i - 2) : (i - 2);
      indices[2] = (i < 1) ? (iscissors->num_pts + i - 1) : (i - 1);
      indices[3] = i;

      for (j = 0; j < 4; j++)
	{
	  geometry[j][0] = pts[indices[j]].dx + off_x;
	  geometry[j][1] = pts[indices[j]].dy + off_y;

	  geometry[j][2] = 0;
	  geometry[j][3] = 0;
	}

      CR_compose (CR_bezier_basis, geometry, bezier_geom);

      for (j = 0; j < 3; j++)
	{
	  new_pt = (BezierPoint *) g_malloc (sizeof (BezierPoint));
	  if (last_pt) last_pt->next = new_pt;
	  else bez_pts = new_pt;
	  new_pt->type = (j == 0) ? BEZIER_ANCHOR : BEZIER_CONTROL;
	  new_pt->x = bezier_geom[j][0];
	  new_pt->y = bezier_geom[j][1];
	  new_pt->next = NULL;
	  new_pt->prev = last_pt;
	  last_pt = new_pt;
	}
    }

  /*  final anchor point  */
  last_pt->next = bez_pts;
  bez_pts->prev = last_pt;

  /*  Load this curve into the bezier tool  */
  bezier_select_load (gdisp, bez_pts, iscissors->num_pts * 3, 1);
  iscissors->state = FREE_SELECT_MODE;
  last_tool = NULL;
/*  	iscissors_reset (iscissors);
*/

}

static TempBuf *
calculate_edge_map (GImage *gimage,
		    int     x,
		    int     y,
		    int     w,
		    int     h)
{
  TempBuf * edge_map;
  PixelRegion srcPR, destPR;
  int width, height;
  int offx, offy;
  int i, j;
  int x1, y1, x2, y2;
  double gradient;
  double dx, dy;
  int xx, yy;
  unsigned char *gr, * dh, * dv, * cm;
  int hmax, vmax;
  int b;
  double prev, next;
  GimpDrawable *drawable;
  void *pr;

  drawable = gimage_active_drawable (gimage);

  x1 = y1 = x2 = y2 = 0;

  /*  allocate the new edge map  */
  edge_map = temp_buf_new (w, h, EDGE_WIDTH, x, y, NULL);

  /*  calculate the extent of the search make a 1 pixel border */
  x1 = BOUNDS (x, 0, drawable_width(drawable));
  y1 = BOUNDS (y, 0, drawable_height(drawable));
  x2 = BOUNDS (x + w, 0, drawable_width(drawable));
  y2 = BOUNDS (y + h, 0, drawable_height(drawable));

  width = x2 - x1;
  height = y2 - y1;
  offx = (x - x1);
  offy = (y - y1);

  /*  Set the drawable up as the source pixel region  */
  /*srcPR.bytes = drawable_bytes (drawable);
  srcPR.w = width;
  srcPR.h = height;
  srcPR.rowstride = gimage->width * drawable_bytes (drawable);
  srcPR.data = drawable_data (drawable) + y1 * srcPR.rowstride + x1 * srcPR.bytes;*/

  pixel_region_init(&srcPR, drawable_data(drawable), x1, y1, width, height, 1);

  /*  Get the horizontal derivative  */
  destPR.data = conv1 /*+ MAX_CHANNELS * (CONV_WIDTH * offy + offx)*/;
  destPR.rowstride = CONV_WIDTH * MAX_CHANNELS;
  destPR.tiles = NULL;
  destPR.bytes = MAX_CHANNELS;
  destPR.x = x1;
  destPR.y = y1;
  destPR.w = CONV_WIDTH;
  destPR.h = CONV_HEIGHT;
  destPR.dirty = 1;
  
  for (pr =pixel_regions_register (2, &srcPR, &destPR); pr != NULL; pr = pixel_regions_process (pr))
    gaussian_deriv (&srcPR, &destPR, HORIZONTAL, std_dev);

  /*  Get the vertical derivative  */
  destPR.data = conv2 + MAX_CHANNELS * (CONV_WIDTH * offy + offx);

  for (pr =pixel_regions_register (2, &srcPR, &destPR); pr != NULL; pr = pixel_regions_process (pr))
    gaussian_deriv (&srcPR, &destPR, VERTICAL, std_dev);

  /*  fill in the edge map  */

  for (i = 0; i < height; i++)
    {
      gr = grad + (CONV_WIDTH + 2) * (i+1) + 1;
      dh = conv1 + destPR.rowstride * i;
      dv = conv2 + destPR.rowstride * i;

      for (j = 0; j < width; j++)
	{
	  hmax = dh[0] - 128;
	  vmax = dv[0] - 128;
	  for (b = 1; b < drawable_bytes (drawable); b++)
	    {
	      if (abs (dh[b] - 128) > abs (hmax)) 
		hmax = dh[b] - 128;
	      if (abs (dv[b] - 128) > abs (vmax)) 
		vmax = dv[b] - 128;
	    }

	  /*  store the information in the edge map  */
	  dh[0] = hmax + 128;
	  dv[0] = vmax + 128;

	  /*  Find the gradient  */
	  gradient = sqrt (SQR (hmax) + SQR (vmax));

	  /*  Make the edge gradient map extend one pixel further  */
	  if (j == 0)
	    gr[-1] = (unsigned char) gradient;
	  if (j == (width - 1))
	    gr[+1] = (unsigned char) gradient;

	  *gr++ = (unsigned char) gradient;

	  dh += srcPR.bytes;
	  dv += srcPR.bytes;
	}
    }

  /*  Make the edge gradient map extend one row further  */
   memcpy (grad, grad + (CONV_WIDTH+2), (CONV_WIDTH+2));
  memcpy (grad + (CONV_WIDTH+2) * (CONV_HEIGHT+1),
	  grad + (CONV_WIDTH+2) * (CONV_HEIGHT),
	  (CONV_WIDTH+2));

  cm = temp_buf_data (edge_map);

  for (i = 0; i < h; i++)
    {
      gr = grad + (CONV_WIDTH+2)*(i + offy + 1) + (offx + 1);
      dh = conv1 + destPR.rowstride*(i + offy) + srcPR.bytes * offx;
      dv = conv2 + destPR.rowstride*(i + offy) + srcPR.bytes * offx;

      for (j = 0; j < w; j++)
	{
	  dx = (double) (dh[0] - 128) / 128.0;
	  dy = (double) (dv[0] - 128) / 128.0;

	  xx = (dx > 0) ? 1 : -1;
	  yy = (dy > 0) ? (CONV_WIDTH + 2) : -(CONV_WIDTH + 2);

	  dx = fabs (dx);
	  dy = fabs (dy);

	  prev = BILINEAR (gr[0], gr[-xx], gr[-yy], gr[-xx -yy], dx, dy);
	  next = BILINEAR (gr[0], gr[xx], gr[yy], gr[xx + yy], dx, dy);

	  if (gr[0] >= prev && gr[0] >= next)
	    *cm++ = gr[0];
	  else
	    *cm++ = 0;

	  
	  /*cm++ = dh[0];*/
	  /*cm++ = dv[0];*/
	 

	  dh += srcPR.bytes;
	  dv += srcPR.bytes;
	  gr ++;
	}
    }

  return edge_map; 
}

static void
construct_edge_map (Tool    *tool,
		    TempBuf *edge_buf)
{
  TempBuf * block;
  int index;
  int x, y;
  int endx, endy;
  int row, col;
  int offx, offy;
  int x2, y2;
  long sboffset;
  long dboffset;
   
  PixelRegion srcPR, destPR;
/*#define ISCISSORS_STILL_DOES_NOT_WORK */
#ifdef ISCISSORS_STILL_DOES_NOT_WORK
   FILE *dump; 
#endif
  /*  init some variables  */
  srcPR.bytes = edge_buf->bytes;
  destPR.rowstride = edge_buf->bytes * edge_buf->width;
  destPR.x = 0 /*edge_buf->x*/;
  destPR.y = 0 /*edge_buf->y*/;
  destPR.h = edge_buf->height;
  destPR.w = edge_buf->width;
  destPR.bytes = edge_buf->bytes;
  srcPR.tiles = destPR.tiles = NULL;

  y = edge_buf->y;
  endx = edge_buf->x + edge_buf->width;
  endy = edge_buf->y + edge_buf->height;

  row = (y / BLOCK_HEIGHT);
  /*  patch the buffer with the saved portions of the image  */
  while (y < endy)
    {
      x = edge_buf->x;
      col = (x / BLOCK_WIDTH);

      /*  calculate y offset into this row of blocks  */
      offy = (y - row * BLOCK_HEIGHT);
		      
      y2 = (row + 1) * BLOCK_HEIGHT;
      if (y2 > endy) y2 = endy;
      srcPR.h = y2 - y;

      while (x < endx)
      {
	index = row * horz_blocks + col;
	block = edge_map_blocks [index];
	/*  If the block exists, patch it into buf  */
	if (block)
	  {
	    srcPR.x = block->x;
	    srcPR.y = block->y;
	    
	    /* calculate x offset into the block  */
	    offx = (x - col * BLOCK_WIDTH);
	    x2 = (col + 1) * BLOCK_WIDTH;
	    if (x2 > endx) x2 = endx;
	    srcPR.w = x2 - x;

	   /* i Am going to special case this thing */

	    /*  Calculate the offsets into source buffer  */
	    srcPR.rowstride = srcPR.bytes * block->width;

	    sboffset = offy;
 	    sboffset *= srcPR.rowstride;
	    sboffset += offx*srcPR.bytes;
	    srcPR.data = temp_buf_data (block) + sboffset;

	    /*  Calculate offset into destination buffer */
	    dboffset = ((edge_buf->y > srcPR.y)?(0):(srcPR.y - edge_buf->y));
	    dboffset *= destPR.rowstride;
	    dboffset += ((edge_buf->x < srcPR.x)?((srcPR.x - edge_buf->x)*destPR.bytes):((edge_buf->x - srcPR.x)*destPR.bytes));

	    destPR.data = temp_buf_data (edge_buf) + dboffset;

	   /* look at this debuggin info.
	   printf("Pixel region dump (Y %d %d) X %d %d\n", y, endy, x, endx);
	   printf("index(%d) X: %d Y: %d ox: %d oy: %d \n", index, x, y, offx, offy);
	   printf("soff: %d  doff: %d\n",sboffset,dboffset);
	   printf("s.x:%d s.y:%d s.w:%d s.h:%d s.rs:%d s.b%d\n",srcPR.x, srcPR.y, srcPR.w, srcPR.h,srcPR.rowstride, srcPR.bytes);
	   printf("d.x:%d d.y:%d d.w:%d d.h:%d d.rs:%d d.b%d\n",destPR.x,destPR.y,destPR.w,destPR.h,destPR.rowstride, destPR.bytes);
	   printf("e.x:%d e.y:%d e.w:%d e.h:%d\n",edge_buf->x,edge_buf->y,edge_buf->width,edge_buf->height);
	   printf("sdata:%d ddata:%d\n",srcPR.data, destPR.data);
	   printf("bdata:%d edata:%d\n", block->data, edge_buf->data);
	   if((dboffset + (srcPR.h*destPR.rowstride)) > (edge_buf->height * edge_buf -> width)) printf ("ERROR\n");
	   */
	   if(!((dboffset + (srcPR.h*destPR.rowstride)) >
		(edge_buf->height * edge_buf -> width)))
	     copy_region (&srcPR, &destPR);
	  }

	col ++;
	x = col * BLOCK_WIDTH;

      }

      row ++;
      y = row * BLOCK_HEIGHT;
    }
    
#ifdef ISCISSORS_STILL_DOES_NOT_WORK
    
  /*  dump the edge buffer for debugging*/

   dump=fopen("dump", "wb"); 
   fprintf(dump, "P5\n%d %d\n255\n", edge_buf->width, edge_buf->height); 
   fwrite(edge_buf->data, edge_buf->width * edge_buf->height, sizeof (guchar), dump); 
   fclose (dump);
#endif
}


/*  edge map blocks utility functions  */

static void
set_edge_map_blocks (void *gimage_ptr,
		     int   x,
		     int   y,
		     int   w,
		     int   h)
{
  GImage * gimage;
  int endx, endy;
  int startx;
  int index;
  int x1, y1;
  int x2, y2;
  int row, col;
  int width, height;
  GimpDrawable *drawable;

  width = height = 0;

  gimage = (GImage *) gimage_ptr;
  drawable = gimage_active_drawable (gimage);
  width = drawable_width(drawable);
  height = drawable_height(drawable);

  startx = x;
  endx = x + w;
  endy = y + h;

  row = y / BLOCK_HEIGHT;
  while (y < endy)
    {
      col = x / BLOCK_WIDTH;
      while (x < endx)
	{
	  index = row * horz_blocks + col;

	  /*  If the block doesn't exist, create and initialize it  */
	  if (! edge_map_blocks [index])
	    {
	      /*  determine memory efficient width and height of block  */
	      x1 = col * BLOCK_WIDTH;
	      x2 = BOUNDS (x1 + BLOCK_WIDTH, 0, width);
	      w = (x2 - x1);
	      y1 = row * BLOCK_HEIGHT;
	      y2 = BOUNDS (y1 + BLOCK_HEIGHT, 0, height);
	      h = (y2 - y1);

	      /*  calculate a edge map for the specified portion of the gimage  */
	      edge_map_blocks [index] = calculate_edge_map (gimage, x1, y1, w, h);
	    }
	  col++;
	  x = col * BLOCK_WIDTH;
	}

      row ++;
      y = row * BLOCK_HEIGHT;
      x = startx;
    }
}

static void
allocate_edge_map_blocks (int block_width,
			  int block_height,
			  int image_width,
			  int image_height)
{
  int num_blocks;
  int i;

  /*  calculate the number of rows and cols in the edge map block grid  */
  horz_blocks = (image_width + block_width - 1) / block_width;
  vert_blocks = (image_height + block_height - 1) / block_height;

  /*  Allocate the array  */
  num_blocks = horz_blocks * vert_blocks;
  edge_map_blocks = (TempBuf **) g_malloc (sizeof (TempBuf *) * num_blocks);

  /*  Initialize the array  */
  for (i = 0; i < num_blocks; i++)
    edge_map_blocks [i] = NULL;
}


static void
free_edge_map_blocks ()
{
  int i;
  int num_blocks;

  if (!edge_map_blocks)
    return;

  num_blocks = vert_blocks * horz_blocks;
  
  for (i = 0; i < num_blocks; i++)
    if (edge_map_blocks [i]) {

/*	printf("tbf: index %d %d   ",i, num_blocks);
	printf("X:%d ",edge_map_blocks[i]->x);
	printf("Y:%d ",edge_map_blocks[i]->y);
	printf("W:%d ",edge_map_blocks[i]->width);
	printf("H:%d ",edge_map_blocks[i]->height);
	printf("data:%d ",edge_map_blocks[i]->data);
	printf("\n");
*/
      temp_buf_free (edge_map_blocks [i]);
   }
  g_free (edge_map_blocks);

  edge_map_blocks = NULL;
}

/*********************************************/
/*   Functions for gaussian convolutions     */
/*********************************************/


static void
gaussian_deriv (PixelRegion *input,
		PixelRegion *output,
		int          type,
		double       std_dev)
{
  long width, height;
  unsigned char *dest, *dp;
  unsigned char *src, *sp, *s;
  int bytes;
  int *buf, *b;
  int chan;
  int i, row, col;
  int start, end;
  int curve_array [9];
  int sum_array [9];
  int * curve;
  int * sum;
  int val;
  int total;
  int length;
  int initial_p[MAX_CHANNELS], initial_m[MAX_CHANNELS];

  length = 3;    /*  static for speed  */
  width = input->w;
  height = input->h;
  bytes = input->bytes;

  /*  initialize  */
  curve = curve_array + length;
  sum = sum_array + length;
  buf = g_malloc (sizeof (int) * MAXIMUM (width, height) * bytes);

  if (type == VERTICAL)
    {
      make_curve_d (curve, sum, std_dev, length);
      total = sum[0] * -2;
    }
  else
    {
      make_curve (curve, sum, std_dev, length);
      total = sum[length] + curve[length];
    }

  src = input->data;
  dest = output->data;

  for (col = 0; col < width; col++)
    {
      sp = src;
      dp = dest;
      b = buf;

      src += bytes;
      dest += bytes;

      for (chan = 0; chan < bytes; chan++)
	{
	  initial_p[chan] = sp[chan];
	  initial_m[chan] = sp[(height-1) * input->rowstride + chan];
	}

      for (row = 0; row < height; row++)
	{
	  start = (row < length) ? -row : -length;
	  end = (height <= (row + length)) ? (height - row - 1) : length;

	  for (chan = 0; chan < bytes; chan++)
	    {
	      s = sp + (start * input->rowstride) + chan ;
	      val = 0;
	      i = start;

	      if (start != -length)
		val += initial_p[chan] * (sum[start] - sum[-length]);

	      while (i <= end)
		{
		  val += *s * curve[i++];
		  s += input->rowstride;
		}

	      if (end != length)
		val += initial_m[chan] * (sum[length] + curve[length] - sum[end+1]);

	      *b++ = val / total;
	    }

	  sp += input->rowstride;
	}

      b = buf;
      if (type == VERTICAL)
	for (row = 0; row < height; row++)
	  {
	    for (chan = 0; chan < bytes; chan++)
	      {
		b[chan] = b[chan] + 128;
		if (b[chan] > 255)
		  dp[chan] = 255;
		else if (b[chan] < 0)
		  dp[chan] = 0;
		else
		  dp[chan] = b[chan];
	      }
	    b += bytes;
	    dp += output->rowstride;
	  }
      else
	for (row = 0; row < height; row++)
	  {
	    for (chan = 0; chan < bytes; chan++)
	      {
		if (b[chan] > 255)
		  dp[chan] = 255;
		else if (b[chan] < 0)
		  dp[chan] = 0;
		else
		  dp[chan] = b[chan];
	      }
	    b += bytes;
	    dp += output->rowstride;
	  }
    }

  if (type == HORIZONTAL)
    {
      make_curve_d (curve, sum, std_dev, length);
      total = sum[0] * -2;
    }
  else
    {
      make_curve (curve, sum, std_dev, length);
      total = sum[length] + curve[length];
    }

  src = output->data;
  dest = output->data;

  for (row = 0; row < height; row++)
    {
      sp = src;
      dp = dest;
      b = buf;

      src += output->rowstride;
      dest += output->rowstride;

      for (chan = 0; chan < bytes; chan++)
	{
	  initial_p[chan] = sp[chan];
	  initial_m[chan] = sp[(width-1) * bytes + chan];
	}

      for (col = 0; col < width; col++)
	{
	  start = (col < length) ? -col : -length;
	  end = (width <= (col + length)) ? (width - col - 1) : length;

	  for (chan = 0; chan < bytes; chan++)
	    {
	      s = sp + (start * bytes) + chan;
	      val = 0;
	      i = start;

	      if (start != -length)
		val += initial_p[chan] * (sum[start] - sum[-length]);

	      while (i <= end)
		{
		  val += *s * curve[i++];
		  s += bytes;
		}

	      if (end != length)
		val += initial_m[chan] * (sum[length] + curve[length] - sum[end+1]);

	      *b++ = val / total;
	    }

	  sp += bytes;
	}

      b = buf;
      if (type == HORIZONTAL)
	for (col = 0; col < width; col++)
	  {
	    for (chan = 0; chan < bytes; chan++)
	      {
		b[chan] = b[chan] + 128;
		if (b[chan] > 255)
		  dp[chan] = 255;
		else if (b[chan] < 0)
		  dp[chan] = 0;
		else
		  dp[chan] = b[chan];
	      }
	    b += bytes;
	    dp += bytes;
	  }
      else
	for (col = 0; col < width; col++)
	  {
	    for (chan = 0; chan < bytes; chan++)
	      {
		if (b[chan] > 255)
		  dp[chan] = 255;
		else if (b[chan] < 0)
		  dp[chan] = 0;
		else
		  dp[chan] = b[chan];
	      }
	    b += bytes;
	    dp += bytes;
	  }
    }

  g_free (buf);
}

/*
 * The equations: g(r) = exp (- r^2 / (2 * sigma^2))
 *                   r = sqrt (x^2 + y ^2)
 */

static void
make_curve (int    *curve,
	    int    *sum,
	    double  sigma,
	    int     length)
{
  double sigma2;
  int i;

  sigma2 = sigma * sigma;

  curve[0] = 255;
  for (i = 1; i <= length; i++)
    {
      curve[i] = (int) (exp (- (i * i) / (2 * sigma2)) * 255);
      curve[-i] = curve[i];
    }

  sum[-length] = 0;
  for (i = -length+1; i <= length; i++)
    sum[i] = sum[i-1] + curve[i-1];
}

/*
 * The equations: d_g(r) = -r * exp (- r^2 / (2 * sigma^2)) / sigma^2
 *                   r = sqrt (x^2 + y ^2)
 */

static void
make_curve_d (int    *curve,
	      int    *sum,
	      double  sigma,
	      int     length)
{
  double sigma2;
  int i;

  sigma2 = sigma * sigma;

  curve[0] = 0;
  for (i = 1; i <= length; i++)
    {
      curve[i] = (int) ((i * exp (- (i * i) / (2 * sigma2)) / sigma2) * 255);
      curve[-i] = -curve[i];
    }

  sum[-length] = 0;
  sum[0] = 0;
  for (i = 1; i <= length; i++)
    {
      sum[-length + i] = sum[-length + i - 1] + curve[-length + i - 1];
      sum[i] = sum[i - 1] + curve[i - 1];
    }
}


/***********************************************/
/*   Functions for Catmull-Rom area conversion */
/***********************************************/

static GSList ** CR_scanlines = NULL;
static int start_convert;
static int width, height;
static int lastx;
static int lasty;

static void
CR_convert (Iscissors *iscissors,
	    GDisplay  *gdisp,
	    int        antialias)
{
  int indices[4];
  GSList *list;
  int draw_type;
  int *vals, val;
  int x, w;
  int i, j;
  int offx, offy;
  PixelRegion maskPR;
  unsigned char *buf, *b;
  GimpDrawable *drawable;
  
  drawable = gimage_active_drawable(gdisp->gimage);
  vals = NULL;

  /* destroy previous region */
  if (iscissors->mask)
    {
      channel_delete (iscissors->mask);
      iscissors->mask = NULL;
    }

  /* get the new mask's maximum extents */
  if (antialias)
    {
      width = gdisp->gimage->width * SUPERSAMPLE;
      height = gdisp->gimage->height * SUPERSAMPLE;
      draw_type = AA_IMAGE_COORDS;
      /* allocate value array  */
      vals = (int *) g_malloc (sizeof (int) * width);
      buf = (unsigned char *) g_malloc  (sizeof(unsigned char *) * width);
    }
  else
    {
      width = gdisp->gimage->width;
      height = gdisp->gimage->height;
      draw_type = IMAGE_COORDS;
      buf = NULL;
      vals = NULL;
    }

  /* create a new mask */
  iscissors->mask = channel_new_mask (gdisp->gimage, gdisp->gimage->width,
				      gdisp->gimage->height);

  /* allocate room for the scanlines */
  CR_scanlines = g_malloc (sizeof (GSList *) * height);

  /* zero out the scanlines */
  for (i = 0; i < height; i++)
    CR_scanlines[i] = NULL;

  /* scan convert the curve */
  start_convert = 1;

  for (i = 0; i < iscissors->num_pts; i ++)
    {
      indices[0] = (i < 3) ? (iscissors->num_pts + i - 3) : (i - 3);
      indices[1] = (i < 2) ? (iscissors->num_pts + i - 2) : (i - 2);
      indices[2] = (i < 1) ? (iscissors->num_pts + i - 1) : (i - 1);
      indices[3] = i;
      iscissors_draw_CR (gdisp, iscissors, pts, indices, draw_type);
    }

  drawable_offsets(drawable, &offx, &offy);
  pixel_region_init (&maskPR, iscissors->mask->drawable.tiles, 0, 0,
		     iscissors->mask->drawable.width,
		     iscissors->mask->drawable.height, TRUE);

  for (i = 0; i < height-(offy*SUPERSAMPLE); i++)
    {
      list = CR_scanlines[i];

      /*  zero the vals array  */
      if (antialias && !(i % SUPERSAMPLE))
	memset (vals, 0, width * sizeof (int));

      while (list)
        {
          x = (long) list->data + offx;
          if ( x < 0 ) x = 0;
          
          list = list->next;
          if (list)
            {
              w = (long) list->data - x;

		  if (w+x > width) w = width - x;
		  
	      if (!antialias)
		 channel_add_segment (iscissors->mask, x, i+offy, w, 255);
	      else
		for (j = 0; j < w; j++)
		  vals[j + x] += 255;

              list = g_slist_next (list);
            }
        }

      if (antialias && !((i+1) % SUPERSAMPLE))
	{
	  b = buf;
	  for (j = 0; j < width; j += SUPERSAMPLE)
	    {
	      val = 0;
	      for (x = 0; x < SUPERSAMPLE; x++)
		val += vals[j + x];

	      *b++ = (unsigned char) (val / SUPERSAMPLE2);
	    }
	  pixel_region_set_row (&maskPR, offx/*0*/, (i / SUPERSAMPLE)+offy, iscissors->mask->drawable.width-offx, buf);
	}

      g_slist_free (CR_scanlines[i]);
      CR_scanlines[i] = NULL;
    }

  if (antialias)
    g_free (vals);

  g_free (CR_scanlines);
  CR_scanlines = NULL;
}

static void
CR_convert_points (GdkPoint *points,
		   int       npoints)
{
  int i;

  if (start_convert)
    start_convert = 0;
  else
    CR_convert_line (CR_scanlines, lastx, lasty, points[0].x, points[0].y);

  for (i = 0; i < (npoints - 1); i++)
    {
      CR_convert_line (CR_scanlines,
			   points[i].x, points[i].y,
			   points[i+1].x, points[i+1].y);
    }

  lastx = points[npoints-1].x;
  lasty = points[npoints-1].y;
}

static void
CR_convert_line (GSList **scanlines,
		 int      x1,
		 int      y1,
		 int      x2,
		 int      y2)
{
  int dx, dy;
  int error, inc;
  int tmp;
  float slope;

  if (y1 == y2)
    return;

  if (y1 > y2)
    {
      tmp = y2; y2 = y1; y1 = tmp;
      tmp = x2; x2 = x1; x1 = tmp;
    }

  if (y1 < 0)
    {
      if (y2 < 0)
	return;

      if (x2 == x1)
	{
	  y1 = 0;
	}
      else
	{
	  slope = (float) (y2 - y1) / (float) (x2 - x1);
	  x1 = x2 + (0 - y2) / slope;
	  y1 = 0;
	}
    }

  if (y2 >= height)
    {
      if (y1 >= height)
	return;

      if (x2 == x1)
	{
	  y2 = height;
	}
      else
	{
	  slope = (float) (y2 - y1) / (float) (x2 - x1);
	  x2 = x1 + (height - y1) / slope;
	  y2 = height;
	}
    }

  if (y1 == y2)
    return;

  dx = x2 - x1;
  dy = y2 - y1;

  scanlines = &scanlines[y1];

  if (((dx < 0) ? -dx : dx) > ((dy < 0) ? -dy : dy))
    {
      if (dx < 0)
        {
          inc = -1;
          dx = -dx;
        }
      else
        {
          inc = 1;
        }

      error = -dx /2;
      while (x1 != x2)
        {
          error += dy;
          if (error > 0)
            {
              error -= dx;
	      *scanlines = CR_insert_in_list (*scanlines, x1);
	      scanlines++;
            }

          x1 += inc;
        }
    }
  else
    {
      error = -dy /2;
      if (dx < 0)
        {
          dx = -dx;
          inc = -1;
        }
      else
        {
          inc = 1;
        }

      while (y1++ < y2)
        {
	  *scanlines = CR_insert_in_list (*scanlines, x1);
	  scanlines++;

          error += dx;
          if (error > 0)
            {
              error -= dy;
              x1 += inc;
            }
        }
    }
}

static GSList *
CR_insert_in_list (GSList *list,
		   int     x)
{
  GSList *orig = list;
  GSList *rest;

  if (!list)
    return g_slist_prepend (list, (gpointer) ((long) x));

  while (list)
    {
      rest = g_slist_next (list);
      if (x < (long) list->data)
        {
          rest = g_slist_prepend (rest, list->data);
          list->next = rest;
          list->data = (gpointer) ((long) x);
          return orig;
        }
      else if (!rest)
        {
          g_slist_append (list, (gpointer) ((long) x));
          return orig;
        }
      list = g_slist_next (list);
    }

  return orig;
}
