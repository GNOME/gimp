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
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include "appenv.h"
#include "asupsample.h"
#include "blend.h"
#include "brush_select.h"
#include "buildmenu.h"
#include "draw_core.h"
#include "drawable.h"
#include "errors.h"
#include "fuzzy_select.h"
#include "gdisplay.h"
#include "gimage_mask.h"
#include "gradient.h"
#include "interface.h"
#include "paint_funcs.h"
#include "palette.h"
#include "selection.h"
#include "tools.h"
#include "undo.h"

/*  target size  */
#define  TARGET_HEIGHT     15
#define  TARGET_WIDTH      15

#define  SQR(x) ((x) * (x))

#ifndef M_PI
#define M_PI    3.14159265358979323846
#endif /* M_PI */

/*  the Blend structures  */

typedef enum
{
  Linear,
  BiLinear,
  Radial,
  Square,
  ConicalSymmetric,
  ConicalAsymmetric,
  ShapeburstAngular,
  ShapeburstSpherical,
  ShapeburstDimpled
} GradientType;

typedef enum
{
  FG_BG_RGB_MODE,
  FG_BG_HSV_MODE,
  FG_TRANS_MODE,
  CUSTOM_MODE
} BlendMode;

typedef enum
{
  REPEAT_NONE,
  REPEAT_SAWTOOTH,
  REPEAT_TRIANGULAR
} RepeatMode;

typedef double (*RepeatFunc)(double);

typedef struct _BlendTool BlendTool;
struct _BlendTool
{
  DrawCore *   core;       /*  Core select object          */

  int          startx;     /*  starting x coord            */
  int          starty;     /*  starting y coord            */

  int          endx;       /*  ending x coord              */
  int          endy;       /*  ending y coord              */
};

typedef struct _BlendOptions BlendOptions;
struct _BlendOptions
{
  double       opacity;
  double       offset;
  BlendMode    blend_mode;
  int          paint_mode;
  GradientType gradient_type;
  RepeatMode   repeat;
  GtkWidget   *repeat_mode_menu;
  int          supersample;
  GtkWidget   *frame;
  int          max_depth;
  double       threshold;
};

typedef struct {
  double       offset;
  double       sx, sy;
  BlendMode    blend_mode;
  GradientType gradient_type;
  color_t      fg, bg;
  double       dist;
  double       vec[2];
  RepeatFunc   repeat_func;
} RenderBlendData;

typedef struct {
  PixelRegion   *PR;
  unsigned char *row_data;
  int            bytes;
  int            width;
} PutPixelData;

/*  local function prototypes  */
static void   blend_scale_update        (GtkAdjustment *, double *);
static void   gradient_type_callback    (GtkWidget *, gpointer);
static void   blend_mode_callback       (GtkWidget *, gpointer);
static void   paint_mode_callback       (GtkWidget *, gpointer);
static void   repeat_type_callback      (GtkWidget *, gpointer);
static void   supersample_toggle_update (GtkWidget *, gpointer);
static void   max_depth_scale_update    (GtkAdjustment *, int *);
static void   threshold_scale_update    (GtkAdjustment *, double *);

static void   blend_button_press            (Tool *, GdkEventButton *, gpointer);
static void   blend_button_release          (Tool *, GdkEventButton *, gpointer);
static void   blend_motion                  (Tool *, GdkEventMotion *, gpointer);
static void   blend_cursor_update           (Tool *, GdkEventMotion *, gpointer);
static void   blend_control                 (Tool *, int, gpointer);

static void   blend                         (GImage *gimage, GimpDrawable *drawable,
					     BlendMode blend_mode, int paint_mode,
					     GradientType gradient_type,
					     double opacity, double offset,
					     RepeatMode repeat,
					     int supersample, int max_depth, double threshold,
					     double startx, double starty,
					     double endx, double endy);

static double gradient_calc_conical_sym_factor           (double dist, double *axis, double offset,
							  double x, double y);
static double gradient_calc_conical_asym_factor          (double dist, double *axis, double offset,
							  double x, double y);
static double gradient_calc_square_factor                (double dist, double offset,
							  double x, double y);
static double gradient_calc_radial_factor   	         (double dist, double offset,
							  double x, double y);
static double gradient_calc_linear_factor   	         (double dist, double *vec,
							  double x, double y);
static double gradient_calc_bilinear_factor 	         (double dist, double *vec, double offset,
							  double x, double y);
static double gradient_calc_shapeburst_angular_factor    (double x, double y);
static double gradient_calc_shapeburst_spherical_factor  (double x, double y);
static double gradient_calc_shapeburst_dimpled_factor    (double x, double y);

static double gradient_repeat_none(double val);
static double gradient_repeat_sawtooth(double val);
static double gradient_repeat_triangular(double val);

static void   gradient_precalc_shapeburst   (GImage *gimage, GimpDrawable *drawable, PixelRegion *PR, double dist);

static void   gradient_render_pixel(double x, double y, color_t *color, void *render_data);
static void   gradient_put_pixel(int x, int y, color_t color, void *put_pixel_data);

static void   gradient_fill_region          (GImage *gimage, GimpDrawable *drawable, PixelRegion *PR,
					     int width, int height,
					     BlendMode blend_mode, GradientType gradient_type,
					     double offset, RepeatMode repeat,
					     int supersample, int max_depth, double threshold,
					     double sx, double sy, double ex, double ey);

static void calc_rgb_to_hsv(double *r, double *g, double *b);
static void calc_hsv_to_rgb(double *h, double *s, double *v);

static BlendOptions *create_blend_options   (void);
static Argument *blend_invoker              (Argument *);

/*  variables for the shapeburst algs  */
static PixelRegion distR =
{
  NULL,  /* data */
  NULL,  /* tiles */
  0,     /* rowstride */
  0, 0,  /* w, h */
  0, 0,  /* x, y */
  4,     /* bytes */
  0      /* process count */
};

/*  the blend option menu items -- the blend modes  */
static MenuItem blend_option_items[] =
{
  { "FG to BG (RGB)", 0, 0, blend_mode_callback, (gpointer) FG_BG_RGB_MODE, NULL, NULL },
  { "FG to BG (HSV)", 0, 0, blend_mode_callback, (gpointer) FG_BG_HSV_MODE, NULL, NULL },
  { "FG to Transparent", 0, 0, blend_mode_callback, (gpointer) FG_TRANS_MODE, NULL, NULL },
  { "Custom (from editor)", 0, 0, blend_mode_callback, (gpointer) CUSTOM_MODE, NULL, NULL },
  { NULL, 0, 0, NULL, NULL, NULL, NULL }
};

/*  the gradient option menu items -- the gradient modes  */
static MenuItem gradient_option_items[] =
{
  { "Linear", 0, 0, gradient_type_callback, (gpointer) Linear, NULL, NULL },
  { "Bi-Linear", 0, 0, gradient_type_callback, (gpointer) BiLinear, NULL, NULL },
  { "Radial", 0, 0, gradient_type_callback, (gpointer) Radial, NULL, NULL },
  { "Square", 0, 0, gradient_type_callback, (gpointer) Square, NULL, NULL },
  { "Conical (symmetric)", 0, 0, gradient_type_callback, (gpointer) ConicalSymmetric, NULL, NULL },
  { "Conical (asymmetric)", 0, 0, gradient_type_callback, (gpointer) ConicalAsymmetric, NULL, NULL },
  { "Shapeburst (angular)", 0, 0, gradient_type_callback, (gpointer) ShapeburstAngular, NULL, NULL },
  { "Shapeburst (spherical)", 0, 0, gradient_type_callback, (gpointer) ShapeburstSpherical, NULL, NULL },
  { "Shapeburst (dimpled)", 0, 0, gradient_type_callback, (gpointer) ShapeburstDimpled, NULL, NULL },
  { NULL, 0, 0, NULL, NULL, NULL, NULL }
};

/*  blend options  */
static BlendOptions *blend_options = NULL;

/* repeat menu items */

static MenuItem repeat_option_items[] =
{
  { "None", 0, 0, repeat_type_callback, (gpointer) REPEAT_NONE, NULL, NULL },
  { "Sawtooth wave", 0, 0, repeat_type_callback, (gpointer) REPEAT_SAWTOOTH, NULL, NULL },
  { "Triangular wave", 0, 0, repeat_type_callback, (gpointer) REPEAT_TRIANGULAR, NULL, NULL },
  { NULL, 0, 0, NULL, NULL, NULL, NULL }
};


static void
blend_scale_update (GtkAdjustment *adjustment,
		    double        *scale_val)
{
  *scale_val = adjustment->value;
}

static void
gradient_type_callback (GtkWidget *w,
			gpointer   client_data)
{
  blend_options->gradient_type = (GradientType) client_data;
  gtk_widget_set_sensitive (blend_options->repeat_mode_menu, 
			    (blend_options->gradient_type < 6));
}

static void
blend_mode_callback (GtkWidget *w,
		     gpointer   client_data)
{
  blend_options->blend_mode = (BlendMode) client_data;
}

static void
paint_mode_callback (GtkWidget *w,
		     gpointer   client_data)
{
  blend_options->paint_mode = (long) client_data;
}

static void
repeat_type_callback(GtkWidget *widget,
		     gpointer   client_data)
{
  blend_options->repeat = (RepeatMode) client_data;
}

static void
supersample_toggle_update(GtkWidget *widget,
			  gpointer   client_data)
{
  if (GTK_TOGGLE_BUTTON(widget)->active)
    {
      blend_options->supersample = TRUE;

      gtk_widget_set_sensitive(blend_options->frame, TRUE);
    }
  else
    {
      blend_options->supersample = FALSE;

      gtk_widget_set_sensitive(blend_options->frame, FALSE);
    }
}

static void
max_depth_scale_update(GtkAdjustment *adjustment,
		       int           *scale_val)
{
  *scale_val = (int) adjustment->value;
}

static void
threshold_scale_update(GtkAdjustment *adjustment,
		       double        *scale_val)
{
  *scale_val = adjustment->value;
}

static BlendOptions *
create_blend_options ()
{
  BlendOptions *options;
  GtkWidget *vbox;
  GtkWidget *frame;
  GtkWidget *label;
  GtkWidget *bm_option_menu;
  GtkWidget *bm_menu;
  GtkWidget *pm_option_menu;
  GtkWidget *pm_menu;
  GtkWidget *gt_option_menu;
  GtkWidget *gt_menu;
  GtkWidget *rt_option_menu;
  GtkWidget *rt_menu;
  GtkWidget *opacity_scale;
  GtkWidget *table;
  GtkWidget *offset_scale;
  GtkObject *opacity_scale_data;
  GtkObject *offset_scale_data;
  GtkWidget *button;
  GtkObject *depth_scale_data;
  GtkWidget *depth_scale;
  GtkObject *threshold_scale_data;
  GtkWidget *threshold_scale;

  /*  the new options structure  */
  options = (BlendOptions *) g_malloc (sizeof (BlendOptions));
  options->opacity 	 = 100.0;
  options->offset  	 = 0.0;
  options->blend_mode 	 = FG_BG_RGB_MODE;
  options->paint_mode 	 = NORMAL;
  options->gradient_type = Linear;
  options->repeat        = REPEAT_NONE;
  options->supersample   = FALSE;
  options->max_depth     = 3;
  options->threshold     = 0.2;

  /*  the main vbox  */
  vbox = gtk_vbox_new (FALSE, 2);

  /*  the main label  */
  label = gtk_label_new ("Blend Options");
  gtk_box_pack_start (GTK_BOX (vbox), label, FALSE, FALSE, 0);
  gtk_widget_show (label);

  /*  the table  */
  table = gtk_table_new (6, 2, FALSE);
  gtk_container_border_width (GTK_CONTAINER (table), 2);
  gtk_box_pack_start(GTK_BOX(vbox), table, TRUE, TRUE, 0);

  /*  the opacity scale  */
  label = gtk_label_new ("Opacity:");
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 1.0);
  gtk_table_attach (GTK_TABLE (table), label, 0, 1, 0, 1,
		    GTK_SHRINK | GTK_FILL, GTK_SHRINK | GTK_FILL, 0, 2);
  gtk_widget_show (label);

  opacity_scale_data = gtk_adjustment_new (100.0, 0.0, 100.0, 1.0, 1.0, 0.0);
  opacity_scale = gtk_hscale_new (GTK_ADJUSTMENT (opacity_scale_data));
  gtk_table_attach (GTK_TABLE (table), opacity_scale, 1, 2, 0, 1,
		    GTK_EXPAND | GTK_SHRINK | GTK_FILL, GTK_SHRINK, 4, 2);
  gtk_scale_set_value_pos (GTK_SCALE (opacity_scale), GTK_POS_TOP);
  gtk_range_set_update_policy (GTK_RANGE (opacity_scale), GTK_UPDATE_DELAYED);
  gtk_signal_connect (GTK_OBJECT (opacity_scale_data), "value_changed",
		      (GtkSignalFunc) blend_scale_update,
		      &options->opacity);
  gtk_widget_show (opacity_scale);

  /*  the offset scale  */
  label = gtk_label_new ("Offset:");
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 1.0);
  gtk_table_attach (GTK_TABLE (table), label, 0, 1, 1, 2,
		    GTK_SHRINK | GTK_FILL, GTK_SHRINK | GTK_FILL, 0, 2);
  gtk_widget_show (label);

  offset_scale_data = gtk_adjustment_new (0.0, 0.0, 100.0, 1.0, 1.0, 0.0);
  offset_scale = gtk_hscale_new (GTK_ADJUSTMENT (offset_scale_data));
  gtk_table_attach (GTK_TABLE (table), offset_scale, 1, 2, 1, 2,
		    GTK_EXPAND | GTK_SHRINK | GTK_FILL, GTK_SHRINK, 4, 2);
  gtk_scale_set_value_pos (GTK_SCALE (offset_scale), GTK_POS_TOP);
  gtk_range_set_update_policy (GTK_RANGE (offset_scale), GTK_UPDATE_DELAYED);
  gtk_signal_connect (GTK_OBJECT (offset_scale_data), "value_changed",
		      (GtkSignalFunc) blend_scale_update,
		      &options->offset);
  gtk_widget_show (offset_scale);

  /*  the paint mode menu  */
  label = gtk_label_new ("Mode:");
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gtk_table_attach (GTK_TABLE (table), label, 0, 1, 2, 3,
		    GTK_SHRINK | GTK_FILL, GTK_SHRINK | GTK_FILL, 0, 2);
  pm_menu = create_paint_mode_menu (paint_mode_callback);
  pm_option_menu = gtk_option_menu_new ();
  gtk_table_attach (GTK_TABLE (table), pm_option_menu, 1, 2, 2, 3,
		    GTK_EXPAND | GTK_SHRINK | GTK_FILL, GTK_SHRINK, 4, 2);

  gtk_widget_show (label);
  gtk_widget_show (pm_option_menu);

  /*  the blend mode menu  */
  label = gtk_label_new ("Blend:");
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gtk_table_attach (GTK_TABLE (table), label, 0, 1, 3, 4,
		    GTK_SHRINK | GTK_FILL, GTK_SHRINK | GTK_FILL, 0, 2);
  bm_menu = build_menu (blend_option_items, NULL);
  bm_option_menu = gtk_option_menu_new ();
  gtk_table_attach (GTK_TABLE (table), bm_option_menu, 1, 2, 3, 4,
		    GTK_EXPAND | GTK_SHRINK | GTK_FILL, GTK_SHRINK, 4, 2);

  gtk_widget_show (label);
  gtk_widget_show (bm_option_menu);

  /*  the gradient type menu  */
  label = gtk_label_new ("Gradient:");
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gtk_table_attach (GTK_TABLE (table), label, 0, 1, 4, 5,
		    GTK_SHRINK | GTK_FILL, GTK_SHRINK | GTK_FILL, 0, 2);
  gt_menu = build_menu (gradient_option_items, NULL);
  gt_option_menu = gtk_option_menu_new ();
  gtk_table_attach (GTK_TABLE (table), gt_option_menu, 1, 2, 4, 5,
		    GTK_EXPAND | GTK_SHRINK | GTK_FILL, GTK_SHRINK, 4, 2);

  gtk_widget_show (label);
  gtk_widget_show (gt_option_menu);

  /* the repeat option */

  label = gtk_label_new("Repeat:");
  gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.5);
  gtk_table_attach(GTK_TABLE(table), label, 0, 1, 5, 6,
		   GTK_SHRINK | GTK_FILL, GTK_SHRINK | GTK_FILL, 0, 2);
  rt_menu = build_menu(repeat_option_items, NULL);
  rt_option_menu = gtk_option_menu_new();
  gtk_table_attach(GTK_TABLE(table), rt_option_menu, 1, 2, 5, 6,
		   GTK_EXPAND | GTK_SHRINK | GTK_FILL, GTK_SHRINK, 4, 2);
  gtk_widget_show(label);
  gtk_widget_show(rt_option_menu);

  options->repeat_mode_menu = rt_option_menu;

  /* show the whole table */

  gtk_widget_show (table);

  /* supersampling toggle */

  button = gtk_check_button_new_with_label("Adaptive supersampling");
  gtk_box_pack_start(GTK_BOX(vbox), button, FALSE, FALSE, 0);
  gtk_signal_connect(GTK_OBJECT(button), "toggled",
		     (GtkSignalFunc) supersample_toggle_update,
		     NULL);
  gtk_toggle_button_set_state(GTK_TOGGLE_BUTTON(button), FALSE);
  gtk_widget_show(button);

  /* frame for supersampling options */

  frame = gtk_frame_new(NULL);
  gtk_frame_set_shadow_type(GTK_FRAME(frame), GTK_SHADOW_ETCHED_IN);
  gtk_box_pack_start(GTK_BOX(vbox), frame, TRUE, TRUE, 0);

  /* table for supersamplign options */

  table = gtk_table_new(2, 2, FALSE);
  gtk_container_border_width(GTK_CONTAINER(table), 2);
  gtk_container_add(GTK_CONTAINER(frame), table);

  /* max depth scale */

  label = gtk_label_new("Max depth:");
  gtk_misc_set_alignment(GTK_MISC(label), 0.0, 1.0);
  gtk_table_attach(GTK_TABLE(table), label, 0, 1, 0, 1,
		   GTK_SHRINK | GTK_FILL, GTK_SHRINK | GTK_FILL, 0, 2);
  gtk_widget_show(label);

  depth_scale_data = gtk_adjustment_new(3.0, 1.0, 10.0, 1.0, 1.0, 1.0);
  depth_scale = gtk_hscale_new(GTK_ADJUSTMENT(depth_scale_data));
  gtk_scale_set_digits(GTK_SCALE(depth_scale), 0);
  gtk_table_attach(GTK_TABLE(table), depth_scale, 1, 2, 0, 1,
		   GTK_EXPAND | GTK_SHRINK | GTK_FILL, GTK_SHRINK, 4, 2);
  gtk_scale_set_value_pos(GTK_SCALE(depth_scale), GTK_POS_TOP);
  gtk_signal_connect(GTK_OBJECT(depth_scale_data), "value_changed",
		     (GtkSignalFunc) max_depth_scale_update,
		     &options->max_depth);
  gtk_widget_show(depth_scale);

  /* threshold scale */

  label = gtk_label_new("Threshold:");
  gtk_misc_set_alignment(GTK_MISC(label), 0.0, 1.0);
  gtk_table_attach(GTK_TABLE(table), label, 0, 1, 1, 2,
		   GTK_SHRINK | GTK_FILL, GTK_SHRINK | GTK_FILL, 0, 2);
  gtk_widget_show(label);

  threshold_scale_data = gtk_adjustment_new(0.2, 0.0, 4.0, 0.01, 0.01, 0.0);
  threshold_scale = gtk_hscale_new(GTK_ADJUSTMENT(threshold_scale_data));
  gtk_scale_set_digits(GTK_SCALE(threshold_scale), 2);
  gtk_table_attach(GTK_TABLE(table), threshold_scale, 1, 2, 1, 2,
		   GTK_EXPAND | GTK_SHRINK | GTK_FILL, GTK_SHRINK, 4, 2);
  gtk_scale_set_value_pos(GTK_SCALE(threshold_scale), GTK_POS_TOP);
  gtk_signal_connect(GTK_OBJECT(threshold_scale_data), "value_changed",
		     (GtkSignalFunc) threshold_scale_update,
		     &options->threshold);
  gtk_widget_show(threshold_scale);

  /* show table */

  gtk_widget_show(table);

  /* show frame */

  gtk_widget_show(frame);
  gtk_widget_set_sensitive(frame, FALSE);
  options->frame = frame;

  /*  Register this selection options widget with the main tools options dialog  */
  tools_register_options (BLEND, vbox);

  /*  Post initialization  */
  gtk_option_menu_set_menu (GTK_OPTION_MENU (bm_option_menu), bm_menu);
  gtk_option_menu_set_menu (GTK_OPTION_MENU (gt_option_menu), gt_menu);
  gtk_option_menu_set_menu (GTK_OPTION_MENU (pm_option_menu), pm_menu);
  gtk_option_menu_set_menu (GTK_OPTION_MENU (rt_option_menu), rt_menu);

  return options;
}


static void
blend_button_press (Tool           *tool,
		    GdkEventButton *bevent,
		    gpointer        gdisp_ptr)
{
  GDisplay * gdisp;
  BlendTool * blend_tool;

  gdisp = (GDisplay *) gdisp_ptr;
  blend_tool = (BlendTool *) tool->private;

  switch (drawable_type (gimage_active_drawable (gdisp->gimage)))
    {
    case INDEXED_GIMAGE: case INDEXEDA_GIMAGE:
      message_box ("Blend: Invalid for indexed images.", NULL, NULL);
      return;

      break;
    default:
      break;
    }

  /*  Keep the coordinates of the target  */
  gdisplay_untransform_coords (gdisp, bevent->x, bevent->y,
			       &blend_tool->startx, &blend_tool->starty, FALSE, 1);

  blend_tool->endx = blend_tool->startx;
  blend_tool->endy = blend_tool->starty;

  /*  Make the tool active and set the gdisplay which owns it  */
  gdk_pointer_grab (gdisp->canvas->window, FALSE,
		    GDK_POINTER_MOTION_HINT_MASK | GDK_BUTTON1_MOTION_MASK | GDK_BUTTON_RELEASE_MASK,
		    NULL, NULL, bevent->time);

  tool->gdisp_ptr = gdisp_ptr;
  tool->state = ACTIVE;

  /*  Start drawing the blend tool  */
  draw_core_start (blend_tool->core, gdisp->canvas->window, tool);
}

static void
blend_button_release (Tool           *tool,
		      GdkEventButton *bevent,
		      gpointer        gdisp_ptr)
{
  GImage * gimage;
  BlendTool * blend_tool;
  Argument *return_vals;
  int nreturn_vals;

  gimage = ((GDisplay *) gdisp_ptr)->gimage;
  blend_tool = (BlendTool *) tool->private;

  gdk_pointer_ungrab (bevent->time);
  gdk_flush ();
  draw_core_stop (blend_tool->core, tool);
  tool->state = INACTIVE;

  /*  if the 3rd button isn't pressed, fill the selected region  */
  if (! (bevent->state & GDK_BUTTON3_MASK) &&
      ((blend_tool->startx != blend_tool->endx) ||
       (blend_tool->starty != blend_tool->endy)))
    {
      return_vals = procedural_db_run_proc ("gimp_blend",
					    &nreturn_vals,
					    PDB_IMAGE, gimage->ID,
					    PDB_DRAWABLE, drawable_ID (gimage_active_drawable (gimage)),
					    PDB_INT32, (gint32) blend_options->blend_mode,
					    PDB_INT32, (gint32) blend_options->paint_mode,
					    PDB_INT32, (gint32) blend_options->gradient_type,
					    PDB_FLOAT, (gdouble) blend_options->opacity,
					    PDB_FLOAT, (gdouble) blend_options->offset,
					    PDB_INT32, (gint32) blend_options->repeat,
					    PDB_INT32, (gint32) blend_options->supersample,
					    PDB_INT32, (gint32) blend_options->max_depth,
					    PDB_FLOAT, (gdouble) blend_options->threshold,
					    PDB_FLOAT, (gdouble) blend_tool->startx,
					    PDB_FLOAT, (gdouble) blend_tool->starty,
					    PDB_FLOAT, (gdouble) blend_tool->endx,
					    PDB_FLOAT, (gdouble) blend_tool->endy,
					    PDB_END);

      if (return_vals[0].value.pdb_int == PDB_SUCCESS)
	gdisplays_flush ();
      else
	message_box ("Blend operation failed.", NULL, NULL);

      procedural_db_destroy_args (return_vals, nreturn_vals);
    }
}

static void
blend_motion (Tool           *tool,
	      GdkEventMotion *mevent,
	      gpointer        gdisp_ptr)
{
  GDisplay * gdisp;
  BlendTool * blend_tool;

  gdisp = (GDisplay *) gdisp_ptr;
  blend_tool = (BlendTool *) tool->private;

  /*  undraw the current tool  */
  draw_core_pause (blend_tool->core, tool);

  /*  Get the current coordinates  */
  gdisplay_untransform_coords (gdisp, mevent->x, mevent->y,
			       &blend_tool->endx, &blend_tool->endy, FALSE, 1);

  /*  redraw the current tool  */
  draw_core_resume (blend_tool->core, tool);
}

static void
blend_cursor_update (Tool           *tool,
		     GdkEventMotion *mevent,
		     gpointer        gdisp_ptr)
{
  GDisplay * gdisp;

  gdisp = (GDisplay *) gdisp_ptr;

  switch (drawable_type (gimage_active_drawable (gdisp->gimage)))
    {
    case INDEXED_GIMAGE: case INDEXEDA_GIMAGE:
      gdisplay_install_tool_cursor (gdisp, GDK_TOP_LEFT_ARROW);
      break;
    default:
      gdisplay_install_tool_cursor (gdisp, GDK_TCROSS);
      break;
    }
}

static void
blend_draw (Tool *tool)
{
  GDisplay * gdisp;
  BlendTool * blend_tool;
  int tx1, ty1, tx2, ty2;

  gdisp = (GDisplay *) tool->gdisp_ptr;
  blend_tool = (BlendTool *) tool->private;

  gdisplay_transform_coords (gdisp, blend_tool->startx, blend_tool->starty,
			     &tx1, &ty1, 1);
  gdisplay_transform_coords (gdisp, blend_tool->endx, blend_tool->endy,
			     &tx2, &ty2, 1);

  /*  Draw start target  */
  gdk_draw_line (blend_tool->core->win, blend_tool->core->gc,
		 tx1 - (TARGET_WIDTH >> 1), ty1,
		 tx1 + (TARGET_WIDTH >> 1), ty1);
  gdk_draw_line (blend_tool->core->win, blend_tool->core->gc,
		 tx1, ty1 - (TARGET_HEIGHT >> 1),
		 tx1, ty1 + (TARGET_HEIGHT >> 1));

  /*  Draw end target  */
  gdk_draw_line (blend_tool->core->win, blend_tool->core->gc,
		 tx2 - (TARGET_WIDTH >> 1), ty2,
		 tx2 + (TARGET_WIDTH >> 1), ty2);
  gdk_draw_line (blend_tool->core->win, blend_tool->core->gc,
		 tx2, ty2 - (TARGET_HEIGHT >> 1),
		 tx2, ty2 + (TARGET_HEIGHT >> 1));

  /*  Draw the line between the start and end coords  */
  gdk_draw_line (blend_tool->core->win, blend_tool->core->gc,
		 tx1, ty1, tx2, ty2);
}

static void
blend_control (Tool     *tool,
	       int       action,
	       gpointer  gdisp_ptr)
{
  BlendTool * blend_tool;

  blend_tool = (BlendTool *) tool->private;

  switch (action)
    {
    case PAUSE :
      draw_core_pause (blend_tool->core, tool);
      break;
    case RESUME :
      draw_core_resume (blend_tool->core, tool);
      break;
    case HALT :
      draw_core_stop (blend_tool->core, tool);
      break;
    }
}


/*****/

/*  The actual blending procedure  */
static void
blend (GImage       *gimage,
       GimpDrawable *drawable,
       BlendMode     blend_mode,
       int           paint_mode,
       GradientType  gradient_type,
       double        opacity,
       double        offset,
       RepeatMode    repeat,
       int           supersample,
       int           max_depth,
       double        threshold,
       double        startx,
       double        starty,
       double        endx,
       double        endy)
{
  TileManager *buf_tiles;
  PixelRegion bufPR;
  int has_alpha;
  int has_selection;
  int bytes;
  int x1, y1, x2, y2;

  has_selection = drawable_mask_bounds (drawable, &x1, &y1, &x2, &y2);

  has_alpha = drawable_has_alpha (drawable);
  bytes = drawable_bytes (drawable);

  /*  Always create an alpha temp buf (for generality) */
  if (! has_alpha)
    {
      has_alpha = TRUE;
      bytes += 1;
    }

  buf_tiles = tile_manager_new ((x2 - x1), (y2 - y1), bytes);
  pixel_region_init (&bufPR, buf_tiles, 0, 0, (x2 - x1), (y2 - y1), TRUE);

  gradient_fill_region (gimage, drawable,
			&bufPR, (x2 - x1), (y2 - y1),
			blend_mode, gradient_type, offset, repeat,
			supersample, max_depth, threshold,
			(startx - x1), (starty - y1),
			(endx - x1), (endy - y1));

  pixel_region_init (&bufPR, buf_tiles, 0, 0, (x2 - x1), (y2 - y1), FALSE);
  gimage_apply_image (gimage, drawable, &bufPR, TRUE,
		      (opacity * 255) / 100, paint_mode, NULL, x1, y1);

  /*  update the image  */
  drawable_update (drawable, x1, y1, (x2 - x1), (y2 - y1));

  /*  free the temporary buffer  */
  tile_manager_destroy (buf_tiles);
}


static double
gradient_calc_conical_sym_factor (double  dist,
				  double *axis,
				  double  offset,
				  double  x,
				  double  y)
{
  double vec[2];
  double r;
  double rat;

  if (dist == 0.0)
    rat = 0.0;
  else
    if ((x != 0) || (y != 0))
      {
	/* Calculate offset from the start in pixels */

	r = sqrt(x * x + y * y);

	vec[0] = x / r;
	vec[1] = y / r;

	rat = axis[0] * vec[0] + axis[1] * vec[1]; /* Dot product */

	if (rat > 1.0)
	  rat = 1.0;
	else if (rat < -1.0)
	  rat = -1.0;

	/* This cool idea is courtesy Josh MacDonald,
	 * Ali Rahimi --- two more XCF losers.  */

	rat = acos(rat) / M_PI;
	rat = pow(rat, (offset / 10) + 1);

	rat = BOUNDS(rat, 0.0, 1.0);
      }
    else
      rat = 0.5;

  return rat;
} /* gradient_calc_conical_sym_factor */


/*****/

static double
gradient_calc_conical_asym_factor (double  dist,
				   double *axis,
				   double  offset,
				   double  x,
				   double  y)
{
  double ang0, ang1;
  double ang;
  double rat;

  if (dist == 0.0)
    rat = 0.0;
  else
    {
      if ((x != 0) || (y != 0))
	{
	  ang0 = atan2(axis[0], axis[1]) + M_PI;
	  ang1 = atan2(x, y) + M_PI;

	  ang = ang1 - ang0;

	  if (ang < 0.0)
	    ang += (2.0 * M_PI);

	  rat = ang / (2.0 * M_PI);
	  rat = pow(rat, (offset / 10) + 1);

	  rat = BOUNDS(rat, 0.0, 1.0);
	}
      else
	rat = 0.5; /* We are on middle point */
    } /* else */

  return rat;
} /* gradient_calc_conical_asym_factor */


/*****/

static double
gradient_calc_square_factor (double dist,
			     double offset,
			     double x,
			     double y)
{
  double r;
  double rat;

  if (dist == 0.0)
    rat = 0.0;
  else
    {
      /* Calculate offset from start as a value in [0, 1] */

      offset = offset / 100.0;

      r   = MAXIMUM(abs(x), abs(y));
      rat = r / dist;

      if (rat < offset)
	rat = 0.0;
      else if (offset == 1)
        rat = (rat>=1) ? 1 : 0;
      else
	rat = (rat - offset) / (1.0 - offset);
    } /* else */

  return rat;
} /* gradient_calc_square_factor */


/*****/

static double
gradient_calc_radial_factor (double dist,
			     double offset,
			     double x,
			     double y)
{
  double r;
  double rat;

  if (dist == 0.0)
    rat = 0.0;
  else
    {
      /* Calculate radial offset from start as a value in [0, 1] */

      offset = offset / 100.0;

      r   = sqrt(SQR(x) + SQR(y));
      rat = r / dist;

      if (rat < offset)
	rat = 0.0;
      else if (offset == 1)
        rat = (rat>=1) ? 1 : 0;
      else
	rat = (rat - offset) / (1.0 - offset);
    } /* else */

  return rat;
} /* gradient_calc_radial_factor */


/*****/

static double
gradient_calc_linear_factor (double  dist,
			     double *vec,
			     double  x,
			     double  y)
{
  double r;
  double rat;

  if (dist == 0.0)
    rat = 0.0;
  else
    {
      r   = vec[0] * x + vec[1] * y;
      rat = r / dist;
    } /* else */

  return rat;
} /* gradient_calc_linear_factor */


/*****/

static double
gradient_calc_bilinear_factor (double  dist,
			       double *vec,
			       double  offset,
			       double  x,
			       double  y)
{
  double r;
  double rat;

  if (dist == 0.0)
    rat = 0.0;
  else
    {
      /* Calculate linear offset from the start line outward */

      offset = offset / 100.0;

      r   = vec[0] * x + vec[1] * y;
      rat = r / dist;

      if (fabs(rat) < offset)
	rat = 0.0;
      else if (offset == 1)
        rat = (rat>=1) ? 1 : 0;
      else
	rat = (fabs(rat) - offset) / (1.0 - offset);
    } /* else */

  return rat;
} /* gradient_calc_bilinear_factor */


static double
gradient_calc_shapeburst_angular_factor (double x,
					 double y)
{
  int ix, iy;
  Tile *tile;
  float value;

  ix = (int) BOUNDS (x, 0, distR.w);
  iy = (int) BOUNDS (y, 0, distR.h);
  tile = tile_manager_get_tile (distR.tiles, ix, iy, 0);
  tile_ref (tile);
  value = 1.0 - *(((float *) tile->data) + ((iy % TILE_HEIGHT) * tile->ewidth + (ix % TILE_WIDTH)));
  tile_unref (tile, FALSE);

  return value;
}


static double
gradient_calc_shapeburst_spherical_factor (double x,
					   double y)
{
  int ix, iy;
  Tile *tile;
  float value;

  ix = (int) BOUNDS (x, 0, distR.w);
  iy = (int) BOUNDS (y, 0, distR.h);
  tile = tile_manager_get_tile (distR.tiles, ix, iy, 0);
  tile_ref (tile);
  value = *(((float *) tile->data) + ((iy % TILE_HEIGHT) * tile->ewidth + (ix % TILE_WIDTH)));
  value = 1.0 - sin (0.5 * M_PI * value);
  tile_unref (tile, FALSE);

  return value;
}


static double
gradient_calc_shapeburst_dimpled_factor (double x,
					 double y)
{
  int ix, iy;
  Tile *tile;
  float value;

  ix = (int) BOUNDS (x, 0, distR.w);
  iy = (int) BOUNDS (y, 0, distR.h);
  tile = tile_manager_get_tile (distR.tiles, ix, iy, 0);
  tile_ref (tile);
  value = *(((float *) tile->data) + ((iy % TILE_HEIGHT) * tile->ewidth + (ix % TILE_WIDTH)));
  value = cos (0.5 * M_PI * value);
  tile_unref (tile, FALSE);

  return value;
}

static double
gradient_repeat_none(double val)
{
  return BOUNDS(val, 0.0, 1.0);
}

static double
gradient_repeat_sawtooth(double val)
{
  if (val >= 0.0)
    return fmod(val, 1.0);
  else
    return 1.0 - fmod(-val, 1.0);
}

static double
gradient_repeat_triangular(double val)
{
  int ival;

  if (val < 0.0)
    val = -val;

  ival = (int) val;

  if (ival & 1)
    return 1.0 - fmod(val, 1.0);
  else
    return fmod(val, 1.0);
}

/*****/
static void
gradient_precalc_shapeburst (GImage      *gimage,
			     GimpDrawable *drawable,
			     PixelRegion *PR,
			     double       dist)
{
  Channel *mask;
  PixelRegion tempR;
  float max_iteration;
  float *distp;
  int size;
  void * pr;
  unsigned char white[1] = { OPAQUE_OPACITY };

  /*  allocate the distance map  */
  if (distR.tiles)
    tile_manager_destroy (distR.tiles);
  distR.tiles = tile_manager_new (PR->w, PR->h, sizeof (float));

  /*  allocate the selection mask copy  */
  tempR.tiles = tile_manager_new (PR->w, PR->h, 1);
  pixel_region_init (&tempR, tempR.tiles, 0, 0, PR->w, PR->h, TRUE);

  /*  If the gimage mask is not empty, use it as the shape burst source  */
  if (! gimage_mask_is_empty (gimage))
    {
      PixelRegion maskR;
      int x1, y1, x2, y2;
      int offx, offy;

      drawable_mask_bounds (drawable, &x1, &y1, &x2, &y2);
      drawable_offsets (drawable, &offx, &offy);

      /*  the selection mask  */
      mask = gimage_get_mask (gimage);
      pixel_region_init (&maskR, drawable_data (GIMP_DRAWABLE(mask)), 
			 x1 + offx, y1 + offy, (x2 - x1), (y2 - y1), FALSE);

      /*  copy the mask to the temp mask  */
      copy_region (&maskR, &tempR);
    }
  /*  otherwise...  */
  else
    {
      /*  If the intended drawable has an alpha channel, use that  */
      if (drawable_has_alpha (drawable))
	{
	  PixelRegion drawableR;

	  pixel_region_init (&drawableR, drawable_data (drawable), PR->x, PR->y, PR->w, PR->h, FALSE);

	  extract_alpha_region (&drawableR, NULL, &tempR);
	}
      /*  Otherwise, just fill the shapeburst to white  */
      else
	color_region (&tempR, white);
    }

  pixel_region_init (&tempR, tempR.tiles, 0, 0, PR->w, PR->h, TRUE);
  pixel_region_init (&distR, distR.tiles, 0, 0, PR->w, PR->h, TRUE);
  max_iteration = shapeburst_region (&tempR, &distR);

  /*  normalize the shapeburst with the max iteration  */
  if (max_iteration > 0)
    {
      pixel_region_init (&distR, distR.tiles, 0, 0, PR->w, PR->h, TRUE);

      for (pr = pixel_regions_register (1, &distR); pr != NULL; pr = pixel_regions_process (pr))
	{
	  distp = (float *) distR.data;
	  size = distR.w * distR.h;

	  while (size--)
	    *distp++ /= max_iteration;
	}

      pixel_region_init (&distR, distR.tiles, 0, 0, PR->w, PR->h, FALSE);
    }

  tile_manager_destroy (tempR.tiles);
}


static void
gradient_render_pixel(double x, double y, color_t *color, void *render_data)
{
  RenderBlendData *rbd;
  double           factor;

  rbd = render_data;

  /* Calculate blending factor */

  switch (rbd->gradient_type)
    {
    case Radial:
      factor = gradient_calc_radial_factor(rbd->dist, rbd->offset,
					   x - rbd->sx, y - rbd->sy);
      break;

    case ConicalSymmetric:
      factor = gradient_calc_conical_sym_factor(rbd->dist, rbd->vec, rbd->offset,
						x - rbd->sx, y - rbd->sy);
      break;

    case ConicalAsymmetric:
      factor = gradient_calc_conical_asym_factor(rbd->dist, rbd->vec, rbd->offset,
						 x - rbd->sx, y - rbd->sy);
      break;

    case Square:
      factor = gradient_calc_square_factor(rbd->dist, rbd->offset,
					   x - rbd->sx, y - rbd->sy);
      break;

    case Linear:
      factor = gradient_calc_linear_factor(rbd->dist, rbd->vec,
					   x - rbd->sx, y - rbd->sy);
      break;

    case BiLinear:
      factor = gradient_calc_bilinear_factor(rbd->dist, rbd->vec, rbd->offset,
					     x - rbd->sx, y - rbd->sy);
      break;

    case ShapeburstAngular:
      factor = gradient_calc_shapeburst_angular_factor(x, y);
      break;

    case ShapeburstSpherical:
      factor = gradient_calc_shapeburst_spherical_factor(x, y);
      break;

    case ShapeburstDimpled:
      factor = gradient_calc_shapeburst_dimpled_factor(x, y);
      break;

    default:
      fatal_error("gradient_render_pixel(): unknown gradient type %d",
		  (int) rbd->gradient_type);
      return;
    }

  /* Adjust for repeat */

  factor = (*rbd->repeat_func)(factor);

  /* Blend the colors */

  if (rbd->blend_mode == CUSTOM_MODE)
    grad_get_color_at(factor, &color->r, &color->g, &color->b, &color->a);
  else
    {
      /* Blend values */

      color->r = rbd->fg.r + (rbd->bg.r - rbd->fg.r) * factor;
      color->g = rbd->fg.g + (rbd->bg.g - rbd->fg.g) * factor;
      color->b = rbd->fg.b + (rbd->bg.b - rbd->fg.b) * factor;
      color->a = rbd->fg.a + (rbd->bg.a - rbd->fg.a) * factor;

      if (rbd->blend_mode == FG_BG_HSV_MODE)
	calc_hsv_to_rgb(&color->r, &color->g, &color->b);
    }
}


static void
gradient_put_pixel(int x, int y, color_t color, void *put_pixel_data)
{
  PutPixelData  *ppd;
  unsigned char *data;

  ppd = put_pixel_data;

  /* Paint */

  data = ppd->row_data + ppd->bytes * x;

  if (ppd->bytes >= 3)
    {
      *data++ = color.r * 255.0;
      *data++ = color.g * 255.0;
      *data++ = color.b * 255.0;
      *data++ = color.a * 255.0;
    }
  else
    {
      /* Convert to grayscale */

      *data++ = 255.0 * (0.30 * color.r +
			 0.59 * color.g +
			 0.11 * color.b);
      *data++ = color.a * 255.0;
    }

  /* Paint whole row if we are on the rightmost pixel */

  if (x == (ppd->width - 1))
    pixel_region_set_row(ppd->PR, 0, y, ppd->width, ppd->row_data);
}


static void
gradient_fill_region (GImage       *gimage,
		      GimpDrawable *drawable,
		      PixelRegion  *PR,
		      int           width,
		      int           height,
		      BlendMode     blend_mode,
		      GradientType  gradient_type,
		      double        offset,
		      RepeatMode    repeat,
		      int           supersample,
		      int           max_depth,
		      double        threshold,
		      double        sx,
		      double        sy,
		      double        ex,
		      double        ey)
{
  RenderBlendData  rbd;
  PutPixelData     ppd;
  unsigned char    r, g, b;
  int              x, y;
  int              endx, endy;
  void            *pr;
  unsigned char   *data;
  color_t          color;

  /* Get foreground and background colors, normalized */

  palette_get_foreground(&r, &g, &b);

  rbd.fg.r = r / 255.0;
  rbd.fg.g = g / 255.0;
  rbd.fg.b = b / 255.0;
  rbd.fg.a = 1.0;  /* Foreground is always opaque */

  palette_get_background(&r, &g, &b);

  rbd.bg.r = r / 255.0;
  rbd.bg.g = g / 255.0;
  rbd.bg.b = b / 255.0;
  rbd.bg.a = 1.0; /* opaque, for now */

  switch (blend_mode)
    {
    case FG_BG_RGB_MODE:
      break;

    case FG_BG_HSV_MODE:
      /* Convert to HSV */

      calc_rgb_to_hsv(&rbd.fg.r, &rbd.fg.g, &rbd.fg.b);
      calc_rgb_to_hsv(&rbd.bg.r, &rbd.bg.g, &rbd.bg.b);

      break;

    case FG_TRANS_MODE:
      /* Color does not change, just the opacity */

      rbd.bg   = rbd.fg;
      rbd.bg.a = 0.0; /* transparent */

      break;

    case CUSTOM_MODE:
      break;

    default:
      fatal_error("gradient_fill_region(): unknown blend mode %d",
		  (int) blend_mode);
      break;
    }

  /* Calculate type-specific parameters */

  switch (gradient_type)
    {
    case Radial:
      rbd.dist = sqrt(SQR(ex - sx) + SQR(ey - sy));
      break;

    case Square:
      rbd.dist = MAXIMUM(fabs(ex - sx), fabs(ey - sy));
      break;

    case ConicalSymmetric:
    case ConicalAsymmetric:
    case Linear:
    case BiLinear:
      rbd.dist = sqrt(SQR(ex - sx) + SQR(ey - sy));

      if (rbd.dist > 0.0)
	{
	  rbd.vec[0] = (ex - sx) / rbd.dist;
	  rbd.vec[1] = (ey - sy) / rbd.dist;
	}

      break;

    case ShapeburstAngular:
    case ShapeburstSpherical:
    case ShapeburstDimpled:
      rbd.dist = sqrt(SQR(ex - sx) + SQR(ey - sy));
      gradient_precalc_shapeburst(gimage, drawable, PR, rbd.dist);
      break;

    default:
      fatal_error("gradient_fill_region(): unknown gradient type %d",
		  (int) gradient_type);
      break;
    }

  /* Set repeat function */

  switch (repeat)
    {
    case REPEAT_NONE:
      rbd.repeat_func = gradient_repeat_none;
      break;

    case REPEAT_SAWTOOTH:
      rbd.repeat_func = gradient_repeat_sawtooth;
      break;

    case REPEAT_TRIANGULAR:
      rbd.repeat_func = gradient_repeat_triangular;
      break;

    default:
      fatal_error("gradient_fill_region(): unknown repeat mode %d",
		  (int) repeat);
      break;
    }

  /* Initialize render data */

  rbd.offset        = offset;
  rbd.sx            = sx;
  rbd.sy            = sy;
  rbd.blend_mode    = blend_mode;
  rbd.gradient_type = gradient_type;

  /* Render the gradient! */

  if (supersample)
    {
      /* Initialize put pixel data */

      ppd.PR       = PR;
      ppd.row_data = g_malloc(width * PR->bytes);
      ppd.bytes    = PR->bytes;
      ppd.width    = width;

      /* Render! */

      adaptive_supersample_area(0, 0, (width - 1), (height - 1),
				max_depth, threshold,
				gradient_render_pixel, &rbd,
				gradient_put_pixel, &ppd,
				NULL, NULL);

      /* Clean up */

      g_free(ppd.row_data);
    }
  else
    {
      for (pr = pixel_regions_register(1, PR); pr != NULL; pr = pixel_regions_process(pr))
	{
	  data = PR->data;
	  endx = PR->x + PR->w;
	  endy = PR->y + PR->h;

	  for (y = PR->y; y < endy; y++)
	    for (x = PR->x; x < endx; x++)
	      {
		gradient_render_pixel(x, y, &color, &rbd);

		if (PR->bytes >= 3)
		  {
		    *data++ = color.r * 255.0;
		    *data++ = color.g * 255.0;
		    *data++ = color.b * 255.0;
		    *data++ = color.a * 255.0;
		  }
		else
		  {
		    /* Convert to grayscale */

		    *data++ = 255.0 * (0.30 * color.r +
				       0.59 * color.g +
				       0.11 * color.b);
		    *data++ = color.a * 255.0;
		  }
	      }
	}
    }
}

static void
calc_rgb_to_hsv(double *r, double *g, double *b)
{
  double red, green, blue;
  double h, s, v;
  double min, max;
  double delta;

  red   = *r;
  green = *g;
  blue  = *b;

  h = 0.0; /* Shut up -Wall */

  if (red > green)
    {
      if (red > blue)
	max = red;
      else
	max = blue;

      if (green < blue)
	min = green;
      else
	min = blue;
    }
  else
    {
      if (green > blue)
	max = green;
      else
	max = blue;

      if (red < blue)
	min = red;
      else
	min = blue;
    }

  v = max;

  if (max != 0.0)
    s = (max - min) / max;
  else
    s = 0.0;

  if (s == 0.0)
    h = 0.0;
  else
    {
      delta = max - min;

      if (red == max)
	h = (green - blue) / delta;
      else if (green == max)
	h = 2 + (blue - red) / delta;
      else if (blue == max)
	h = 4 + (red - green) / delta;

      h /= 6.0;

      if (h < 0.0)
	h += 1.0;
      else if (h > 1.0)
	h -= 1.0;
    }

  *r = h;
  *g = s;
  *b = v;
}

static void
calc_hsv_to_rgb(double *h, double *s, double *v)
{
  double hue, saturation, value;
  double f, p, q, t;

  if (*s == 0.0)
    {
      *h = *v;
      *s = *v;
      *v = *v; /* heh */
    }
  else
    {
      hue        = *h * 6.0;
      saturation = *s;
      value      = *v;

      if (hue == 6.0)
	hue = 0.0;

      f = hue - (int) hue;
      p = value * (1.0 - saturation);
      q = value * (1.0 - saturation * f);
      t = value * (1.0 - saturation * (1.0 - f));

      switch ((int) hue)
	{
	case 0:
	  *h = value;
	  *s = t;
	  *v = p;
	  break;

	case 1:
	  *h = q;
	  *s = value;
	  *v = p;
	  break;

	case 2:
	  *h = p;
	  *s = value;
	  *v = t;
	  break;

	case 3:
	  *h = p;
	  *s = q;
	  *v = value;
	  break;

	case 4:
	  *h = t;
	  *s = p;
	  *v = value;
	  break;

	case 5:
	  *h = value;
	  *s = p;
	  *v = q;
	  break;
	}
    }
}


/****************************/
/*  Global blend functions  */
/****************************/

Tool *
tools_new_blend ()
{
  Tool * tool;
  BlendTool * private;

  if (! blend_options)
    blend_options = create_blend_options ();

  tool = (Tool *) g_malloc (sizeof (Tool));
  private = (BlendTool *) g_malloc (sizeof (BlendTool));

  private->core = draw_core_new (blend_draw);

  tool->type = BLEND;
  tool->state = INACTIVE;
  tool->scroll_lock = 1;  /*  Disallow scrolling  */
  tool->auto_snap_to = TRUE;
  tool->private = private;
  tool->button_press_func = blend_button_press;
  tool->button_release_func = blend_button_release;
  tool->motion_func = blend_motion;
  tool->arrow_keys_func = standard_arrow_keys_func;
  tool->cursor_update_func = blend_cursor_update;
  tool->control_func = blend_control;
  tool->preserve = TRUE;

  return tool;
}

void
tools_free_blend (Tool *tool)
{
  BlendTool * blend_tool;

  blend_tool = (BlendTool *) tool->private;

  if (tool->state == ACTIVE)
    draw_core_stop (blend_tool->core, tool);

  draw_core_free (blend_tool->core);

  /*  free the distance map data if it exists  */
  if (distR.tiles)
    tile_manager_destroy (distR.tiles);
  distR.tiles = NULL;

  g_free (blend_tool);
}


/*  The blend procedure definition  */
ProcArg blend_args[] =
{
  { PDB_IMAGE,
    "image",
    "The image"
  },
  { PDB_DRAWABLE,
    "drawable",
    "The affected drawable"
  },
  { PDB_INT32,
    "blend_mode",
    "The type of blend: { FG-BG-RGB (0), FG-BG-HSV (1), FG-TRANS (2), CUSTOM (3) }"
  },
  { PDB_INT32,
    "paint_mode",
    "the paint application mode: { NORMAL (0), DISSOLVE (1), BEHIND (2), MULTIPLY (3), SCREEN (4), OVERLAY (5) DIFFERENCE (6), ADDITION (7), SUBTRACT (8), DARKEN-ONLY (9), LIGHTEN-ONLY (10), HUE (11), SATURATION (12), COLOR (13), VALUE (14) }"
  },
  { PDB_INT32,
    "gradient_type",
    "The type of gradient: { LINEAR (0), BILINEAR (1), RADIAL (2), SQUARE (3), CONICAL-SYMMETRIC (4), CONICAL-ASYMMETRIC (5), SHAPEBURST-ANGULAR (6), SHAPEBURST-SPHERICAL (7), SHAPEBURST-DIMPLED (8) }"
  },
  { PDB_FLOAT,
    "opacity",
    "The opacity of the final blend (0 <= opacity <= 100)"
  },
  { PDB_FLOAT,
    "offset",
    "Offset relates to the starting and ending coordinates specified for the blend.  This parameter is mode depndent (0 <= offset)"
  },
  { PDB_INT32,
    "repeat",
    "Repeat mode: { REPEAT-NONE (0), REPEAT-SAWTOOTH (1), REPEAT-TRIANGULAR (2) }"
  },
  { PDB_INT32,
    "supersample",
    "Do adaptive supersampling (true / false)"
  },
  { PDB_INT32,
    "max_depth",
    "Maximum recursion levels for supersampling"
  },
  { PDB_FLOAT,
    "threshold",
    "Supersampling threshold"
  },
  { PDB_FLOAT,
    "x1",
    "The x coordinate of this blend's starting point"
  },
  { PDB_FLOAT,
    "y1",
    "The y coordinate of this blend's starting point"
  },
  { PDB_FLOAT,
    "x2",
    "The x coordinate of this blend's ending point"
  },
  { PDB_FLOAT,
    "y2",
    "The y coordinate of this blend's ending point"
  }
};

ProcRecord blend_proc =
{
  "gimp_blend",
  "Blend between the starting and ending coordinates with the specified blend mode and gradient type.",
  "This tool requires information on the paint application mode, the blend mode, and the gradient type.  It creates the specified variety of blend using the starting and ending coordinates as defined for each gradient type.",
  "Spencer Kimball & Peter Mattis & Federico Mena Quintero",
  "Spencer Kimball & Peter Mattis & Federico Mena Quintero",
  "1995-1996",
  PDB_INTERNAL,

  /*  Input arguments  */
  15,
  blend_args,

  /*  Output arguments  */
  0,
  NULL,

  /*  Exec method  */
  { { blend_invoker } },
};

static Argument *
blend_invoker (Argument *args)
{
  int success = TRUE;
  GImage *gimage;
  GimpDrawable *drawable;
  BlendMode blend_mode;
  int paint_mode;
  GradientType gradient_type;
  double opacity;
  double offset;
  RepeatMode repeat;
  int supersample;
  int max_depth;
  double threshold;
  double x1, y1;
  double x2, y2;
  int int_value;
  double fp_value;

  drawable    = NULL;
  blend_mode  = FG_BG_RGB_MODE;
  paint_mode  = NORMAL_MODE;
  gradient_type = Linear;
  opacity       = 100.0;
  offset        = 0.0;
  repeat        = REPEAT_NONE;
  supersample   = FALSE;
  max_depth     = 0;
  threshold     = 0.0;

  /*  the gimage  */
  if (success)
    {
      int_value = args[0].value.pdb_int;
      if (! (gimage = gimage_get_ID (int_value)))
	success = FALSE;
    }
  /*  the drawable  */
  if (success)
    {
      int_value = args[1].value.pdb_int;
      drawable = drawable_get_ID (int_value);
      if (drawable == NULL || gimage != drawable_gimage (drawable))
	success = FALSE;
    }
  /*  blend mode  */
  if (success)
    {
      int_value = args[2].value.pdb_int;
      switch (int_value)
	{
	case 0: blend_mode = FG_BG_RGB_MODE; break;
	case 1: blend_mode = FG_BG_HSV_MODE; break;
	case 2: blend_mode = FG_TRANS_MODE; break;
	case 3: blend_mode = CUSTOM_MODE; break;
	default: success = FALSE;
	}
    }
  /*  paint mode  */
  if (success)
    {
      int_value = args[3].value.pdb_int;
      if (int_value >= NORMAL_MODE && int_value <= VALUE_MODE)
	paint_mode = int_value;
      else
	success = FALSE;
    }
  /*  gradient type  */
  if (success)
    {
      int_value = args[4].value.pdb_int;
      switch (int_value)
	{
	case 0: gradient_type = Linear; break;
	case 1: gradient_type = BiLinear; break;
	case 2: gradient_type = Radial; break;
	case 3: gradient_type = Square; break;
	case 4: gradient_type = ConicalSymmetric; break;
	case 5: gradient_type = ConicalAsymmetric; break;
	case 6: gradient_type = ShapeburstAngular; break;
	case 7: gradient_type = ShapeburstSpherical; break;
	case 8: gradient_type = ShapeburstDimpled; break;
	default: success = FALSE;
	}
    }
  /*  opacity  */
  if (success)
    {
      fp_value = args[5].value.pdb_float;
      if (fp_value >= 0.0 && fp_value <= 100.0)
	opacity = fp_value;
      else
	success = FALSE;
    }
  /*  offset  */
  if (success)
    {
      fp_value = args[6].value.pdb_float;
      if (fp_value >= 0.0)
	offset = fp_value;
      else
	success = FALSE;
    }
  /* repeat */
  if (success)
    {
      int_value = args[7].value.pdb_int;
      switch (int_value)
	{
	case 0: repeat = REPEAT_NONE; break;
	case 1: repeat = REPEAT_SAWTOOTH; break;
	case 2: repeat = REPEAT_TRIANGULAR; break;
	default: success = FALSE;
	}
    }
  /* supersampling */
  if (success)
    {
      int_value = args[8].value.pdb_int;

      supersample = (int_value ? TRUE : FALSE);
    }
  /* max_depth */
  if (success)
    {
      int_value = args[9].value.pdb_int;

      if (((int_value >= 1) && (int_value <= 9)) || !supersample)
	max_depth = int_value;
      else
	success = FALSE;
    }
  /* threshold */
  if (success)
    {
      fp_value = args[10].value.pdb_float;

      if (((fp_value >= 0.0) && (fp_value <= 4.0)) || !supersample)
	threshold = fp_value;
      else
	success = FALSE;
    }
  /*  x1, y1, x2, y2  */
  if (success)
    {
      x1 = args[11].value.pdb_float;
      y1 = args[12].value.pdb_float;
      x2 = args[13].value.pdb_float;
      y2 = args[14].value.pdb_float;
    }

  /*  call the blend procedure  */
  if (success)
    {
      blend (gimage, drawable, blend_mode, paint_mode, gradient_type,
	     opacity, offset, repeat, supersample, max_depth, threshold, x1, y1, x2, y2);
    }

  return procedural_db_return_args (&blend_proc, success);
}
