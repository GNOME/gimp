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
#include <stdio.h>
#include <math.h>
#include "appenv.h"
#include "asupsample.h"
#include "blend.h"
#include "brush_select.h"
#include "buildmenu.h"
#include "cursorutil.h"
#include "draw_core.h"
#include "drawable.h"
#include "errors.h"
#include "fuzzy_select.h"
#include "gdisplay.h"
#include "gimage_mask.h"
#include "gradient.h"
#include "interface.h"
#include "paint_funcs.h"
#include "paint_options.h"
#include "palette.h"
#include "selection.h"
#include "tool_options_ui.h"
#include "tools.h"
#include "undo.h"

#include "libgimp/gimpintl.h"

#include "tile.h"

/*  target size  */
#define  TARGET_HEIGHT     15
#define  TARGET_WIDTH      15

#define  SQR(x) ((x) * (x))

#ifndef M_PI
#define M_PI    3.14159265358979323846
#endif /* M_PI */

#define STATUSBAR_SIZE 128

/*  the blend structures  */

typedef double (*RepeatFunc)(double);

typedef struct _BlendTool BlendTool;
struct _BlendTool
{
  DrawCore *   core;       /*  Core select object          */

  int          startx;     /*  starting x coord            */
  int          starty;     /*  starting y coord            */

  int          endx;       /*  ending x coord              */
  int          endy;       /*  ending y coord              */
  guint        context_id; /*  for the statusbar           */
};

typedef struct _BlendOptions BlendOptions;
struct _BlendOptions
{
  PaintOptions  paint_options;

  double        offset;
  double        offset_d;
  GtkObject    *offset_w;

  BlendMode     blend_mode;
  BlendMode     blend_mode_d;
  GtkWidget    *blend_mode_w;

  GradientType  gradient_type;
  GradientType  gradient_type_d;
  GtkWidget    *gradient_type_w;

  RepeatMode    repeat;
  RepeatMode    repeat_d;
  GtkWidget    *repeat_w;

  int           supersample;
  int           supersample_d;
  GtkWidget    *supersample_w;

  int           max_depth;
  int           max_depth_d;
  GtkObject    *max_depth_w;

  double        threshold;
  double        threshold_d;
  GtkObject    *threshold_w;
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


/*  the blend tool options  */
static BlendOptions *blend_options = NULL;

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


/*  local function prototypes  */
static void   gradient_type_callback    (GtkWidget *, gpointer);
static void   blend_mode_callback       (GtkWidget *, gpointer);
static void   repeat_type_callback      (GtkWidget *, gpointer);

static void   blend_button_press            (Tool *, GdkEventButton *, gpointer);
static void   blend_button_release          (Tool *, GdkEventButton *, gpointer);
static void   blend_motion                  (Tool *, GdkEventMotion *, gpointer);
static void   blend_cursor_update           (Tool *, GdkEventMotion *, gpointer);
static void   blend_control                 (Tool *, int, gpointer);

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
static double gradient_calc_spiral_factor                (double dist, double *axis, double offset,
							  double x, double y,gint cwise);
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
					     double sx, double sy, double ex, double ey,
					     progress_func_t progress_callback,
					     void *progress_data);

static void calc_rgb_to_hsv(double *r, double *g, double *b);
static void calc_hsv_to_rgb(double *h, double *s, double *v);


/*  functions  */

static void
blend_mode_callback (GtkWidget *w,
		     gpointer   client_data)
{
  blend_options->blend_mode = (BlendMode) client_data;
}

static void
gradient_type_callback (GtkWidget *w,
			gpointer   client_data)
{
  blend_options->gradient_type = (GradientType) client_data;
  gtk_widget_set_sensitive (blend_options->repeat_w, 
			    (blend_options->gradient_type < 6));
}

static void
repeat_type_callback(GtkWidget *widget,
		     gpointer   client_data)
{
  blend_options->repeat = (RepeatMode) client_data;
}

static void
blend_options_reset ()
{
  BlendOptions *options = blend_options;

  paint_options_reset ((PaintOptions *) options);

  options->blend_mode    = options->blend_mode_d;
  options->gradient_type = options->gradient_type_d;
  options->repeat        = options->repeat_d;

  gtk_option_menu_set_history (GTK_OPTION_MENU (blend_options->blend_mode_w), 0);
  gtk_option_menu_set_history (GTK_OPTION_MENU (options->gradient_type_w), 0);
  gtk_option_menu_set_history (GTK_OPTION_MENU (blend_options->repeat_w), 0);

  gtk_adjustment_set_value (GTK_ADJUSTMENT (blend_options->offset_w),
			    blend_options->offset_d);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (blend_options->supersample_w),
				blend_options->supersample_d);
  gtk_adjustment_set_value (GTK_ADJUSTMENT (blend_options->max_depth_w),
			    blend_options->max_depth_d);
  gtk_adjustment_set_value (GTK_ADJUSTMENT (blend_options->threshold_w),
			    blend_options->threshold_d);
}

static BlendOptions *
blend_options_new ()
{
  BlendOptions *options;

  GtkWidget *vbox;
  GtkWidget *abox;
  GtkWidget *label;
  GtkWidget *menu;
  GtkWidget *table;
  GtkWidget *scale;
  GtkWidget *frame;

  static MenuItem blend_option_items[] =
  {
    { N_("FG to BG (RGB)"), 0, 0, blend_mode_callback,
      (gpointer) FG_BG_RGB_MODE, NULL, NULL },
    { N_("FG to BG (HSV)"), 0, 0, blend_mode_callback,
      (gpointer) FG_BG_HSV_MODE, NULL, NULL },
    { N_("FG to Transparent"), 0, 0, blend_mode_callback,
      (gpointer) FG_TRANS_MODE, NULL, NULL },
    { N_("Custom from editor"), 0, 0, blend_mode_callback,
      (gpointer) CUSTOM_MODE, NULL, NULL },
    { NULL, 0, 0, NULL, NULL, NULL, NULL }
  };
  static MenuItem gradient_option_items[] =
  {
    { N_("Linear"), 0, 0, gradient_type_callback,
      (gpointer) LINEAR, NULL, NULL },
    { N_("Bi-Linear"), 0, 0, gradient_type_callback,
      (gpointer) BILINEAR, NULL, NULL },
    { N_("Radial"), 0, 0, gradient_type_callback,
      (gpointer) RADIAL, NULL, NULL },
    { N_("Square"), 0, 0, gradient_type_callback,
      (gpointer) SQUARE, NULL, NULL },
    { N_("Conical (symmetric)"), 0, 0, gradient_type_callback,
      (gpointer) CONICAL_SYMMETRIC, NULL, NULL },
    { N_("Conical (asymmetric)"), 0, 0, gradient_type_callback,
      (gpointer) CONICAL_ASYMMETRIC, NULL, NULL },
    { N_("Shapeburst (angular)"), 0, 0, gradient_type_callback,
      (gpointer) SHAPEBURST_ANGULAR, NULL, NULL },
    { N_("Shapeburst (spherical)"), 0, 0, gradient_type_callback,
      (gpointer) SHAPEBURST_SPHERICAL, NULL, NULL },
    { N_("Shapeburst (dimpled)"), 0, 0, gradient_type_callback,
      (gpointer) SHAPEBURST_DIMPLED, NULL, NULL },
    { N_("Spiral (clockwise)"), 0, 0, gradient_type_callback,
      (gpointer) SPIRAL_CLOCKWISE, NULL, NULL },
    { N_("Spiral (anticlockwise)"), 0, 0, gradient_type_callback,
      (gpointer) SPIRAL_ANTICLOCKWISE, NULL, NULL },
    { NULL, 0, 0, NULL, NULL, NULL, NULL }
  };
  static MenuItem repeat_option_items[] =
  {
    { N_("None"), 0, 0, repeat_type_callback,
      (gpointer) REPEAT_NONE, NULL, NULL },
    { N_("Sawtooth wave"), 0, 0, repeat_type_callback,
      (gpointer) REPEAT_SAWTOOTH, NULL, NULL },
    { N_("Triangular wave"), 0, 0, repeat_type_callback,
      (gpointer) REPEAT_TRIANGULAR, NULL, NULL },
    { NULL, 0, 0, NULL, NULL, NULL, NULL }
  };

  /*  the new blend tool options structure  */
  options = (BlendOptions *) g_malloc (sizeof (BlendOptions));
  paint_options_init ((PaintOptions *) options,
		      BLEND,
		      blend_options_reset);
  options->offset  	 = options->offset_d  	    = 0.0;
  options->blend_mode 	 = options->blend_mode_d    = FG_BG_RGB_MODE;
  options->gradient_type = options->gradient_type_d = LINEAR;
  options->repeat        = options->repeat_d        = REPEAT_NONE;
  options->supersample   = options->supersample_d   = FALSE;
  options->max_depth     = options->max_depth_d     = 3;
  options->threshold     = options->threshold_d     = 0.2;

  /*  the main vbox  */
  vbox = ((ToolOptions *) options)->main_vbox;

  /*  the offset scale  */
  table = gtk_table_new (4, 2, FALSE);
  gtk_table_set_col_spacing (GTK_TABLE (table), 0, 4);
  gtk_table_set_row_spacings (GTK_TABLE (table), 1);
  gtk_table_set_row_spacing (GTK_TABLE (table), 0, 2);
  gtk_box_pack_start (GTK_BOX (vbox), table, TRUE, TRUE, 0);

  label = gtk_label_new (_("Offset:"));
  gtk_misc_set_alignment (GTK_MISC (label), 1.0, 1.0);
  gtk_table_attach (GTK_TABLE (table), label, 0, 1, 0, 1,
		    GTK_SHRINK | GTK_FILL, GTK_SHRINK | GTK_FILL, 0, 0);
  gtk_widget_show (label);

  options->offset_w =
    gtk_adjustment_new (options->offset_d, 0.0, 100.0, 1.0, 1.0, 0.0);
  gtk_signal_connect (GTK_OBJECT (options->offset_w), "value_changed",
		      (GtkSignalFunc) tool_options_double_adjustment_update,
		      &options->offset);
  scale = gtk_hscale_new (GTK_ADJUSTMENT (options->offset_w));
  gtk_scale_set_value_pos (GTK_SCALE (scale), GTK_POS_TOP);
  gtk_range_set_update_policy (GTK_RANGE (scale), GTK_UPDATE_DELAYED);
  gtk_table_attach_defaults (GTK_TABLE (table), scale, 1, 2, 0, 1);
  gtk_widget_show (scale);

  /*  the blend mode menu  */
  label = gtk_label_new (_("Blend:"));
  gtk_misc_set_alignment (GTK_MISC (label), 1.0, 0.5);
  gtk_table_attach (GTK_TABLE (table), label, 0, 1, 1, 2,
		    GTK_SHRINK | GTK_FILL, GTK_SHRINK | GTK_FILL, 0, 2);
  gtk_widget_show (label);

  abox = gtk_alignment_new (0.0, 0.5, 0.0, 0.0);
  gtk_table_attach_defaults (GTK_TABLE (table), abox, 1, 2, 1, 2);
  gtk_widget_show (abox);

  options->blend_mode_w = gtk_option_menu_new ();
  gtk_container_add (GTK_CONTAINER (abox), options->blend_mode_w);
  gtk_widget_show (options->blend_mode_w);

  menu = build_menu (blend_option_items, NULL);
  gtk_option_menu_set_menu (GTK_OPTION_MENU (options->blend_mode_w), menu);

  /*  the gradient type menu  */
  label = gtk_label_new (_("Gradient:"));
  gtk_misc_set_alignment (GTK_MISC (label), 1.0, 0.5);
  gtk_table_attach (GTK_TABLE (table), label, 0, 1, 2, 3,
		    GTK_SHRINK | GTK_FILL, GTK_SHRINK | GTK_FILL, 0, 0);
  gtk_widget_show (label);

  abox = gtk_alignment_new (0.0, 0.5, 0.0, 0.0);
  gtk_table_attach_defaults (GTK_TABLE (table), abox, 1, 2, 2, 3);
  gtk_widget_show (abox);

  options->gradient_type_w = gtk_option_menu_new ();
  gtk_container_add (GTK_CONTAINER (abox), options->gradient_type_w);
  gtk_widget_show (options->gradient_type_w);

  menu = build_menu (gradient_option_items, NULL);
  gtk_option_menu_set_menu (GTK_OPTION_MENU (options->gradient_type_w), menu);

  /*  the repeat option  */
  label = gtk_label_new (_("Repeat:"));
  gtk_misc_set_alignment (GTK_MISC( label), 1.0, 0.5);
  gtk_table_attach (GTK_TABLE (table), label, 0, 1, 3, 4,
		    GTK_SHRINK | GTK_FILL, GTK_SHRINK | GTK_FILL, 0, 0);
  gtk_widget_show (label);

  abox = gtk_alignment_new (0.0, 0.5, 0.0, 0.0);
  gtk_table_attach_defaults (GTK_TABLE (table), abox, 1, 2, 3, 4);
  gtk_widget_show (abox);

  options->repeat_w = gtk_option_menu_new ();
  gtk_container_add (GTK_CONTAINER (abox), options->repeat_w);
  gtk_widget_show (options->repeat_w);

  menu = build_menu (repeat_option_items, NULL);
  gtk_option_menu_set_menu (GTK_OPTION_MENU (options->repeat_w), menu);

  /*  show the table  */
  gtk_widget_show (table);

  /*  frame for supersampling options  */
  frame = gtk_frame_new (NULL);
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_ETCHED_IN);
  gtk_box_pack_start (GTK_BOX (vbox), frame, TRUE, TRUE, 0);
  gtk_widget_show (frame);

  /* vbox for the supersampling stuff */
  vbox = gtk_vbox_new (FALSE, 0);
  gtk_container_add (GTK_CONTAINER (frame), vbox);
  gtk_widget_show (vbox);

  /*  supersampling toggle  */
  options->supersample_w =
    gtk_check_button_new_with_label (_("Adaptive supersampling"));
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (options->supersample_w),
				options->supersample_d);
  gtk_signal_connect (GTK_OBJECT (options->supersample_w), "toggled",
		      (GtkSignalFunc) tool_options_toggle_update,
		      &options->supersample);
  gtk_box_pack_start (GTK_BOX (vbox), options->supersample_w, FALSE, FALSE, 0);
  gtk_widget_show (options->supersample_w);

  /*  table for supersampling options  */
  table = gtk_table_new (2, 2, FALSE);
  gtk_container_set_border_width (GTK_CONTAINER (table), 2);
  gtk_table_set_col_spacing (GTK_TABLE (table), 0, 4);
  gtk_table_set_row_spacings (GTK_TABLE (table), 1);
  gtk_box_pack_start (GTK_BOX (vbox), table, FALSE, FALSE, 0);

  /*  automatically set the sensitive state of the table  */
  gtk_widget_set_sensitive (table, options->supersample_d);
  gtk_object_set_data (GTK_OBJECT (options->supersample_w), "set_sensitive",
		       table);

  /*  max depth scale  */
  label = gtk_label_new (_("Max depth:"));
  gtk_misc_set_alignment (GTK_MISC (label), 1.0, 1.0);
  gtk_table_attach (GTK_TABLE (table), label, 0, 1, 0, 1,
		    GTK_SHRINK | GTK_FILL, GTK_SHRINK | GTK_FILL, 0, 0);
  gtk_widget_show (label);

  options->max_depth_w =
    gtk_adjustment_new (options->max_depth_d, 1.0, 10.0, 1.0, 1.0, 1.0);
  gtk_signal_connect (GTK_OBJECT(options->max_depth_w), "value_changed",
		      (GtkSignalFunc) tool_options_int_adjustment_update,
		      &options->max_depth);
  scale = gtk_hscale_new (GTK_ADJUSTMENT (options->max_depth_w));
  gtk_scale_set_digits (GTK_SCALE (scale), 0);
  gtk_scale_set_value_pos (GTK_SCALE (scale), GTK_POS_TOP);
  gtk_table_attach_defaults (GTK_TABLE (table), scale, 1, 2, 0, 1);
  gtk_widget_show (scale);

  /*  threshold scale  */
  label = gtk_label_new (_("Threshold:"));
  gtk_misc_set_alignment (GTK_MISC (label), 1.0, 1.0);
  gtk_table_attach (GTK_TABLE (table), label, 0, 1, 1, 2,
		    GTK_SHRINK | GTK_FILL, GTK_SHRINK | GTK_FILL, 0, 0);
  gtk_widget_show (label);

  options->threshold_w =
    gtk_adjustment_new (options->threshold_d, 0.0, 4.0, 0.01, 0.01, 0.0);
  gtk_signal_connect (GTK_OBJECT(options->threshold_w), "value_changed",
		      (GtkSignalFunc) tool_options_double_adjustment_update,
		      &options->threshold);
  scale = gtk_hscale_new (GTK_ADJUSTMENT (options->threshold_w));
  gtk_scale_set_digits (GTK_SCALE (scale), 2);
  gtk_scale_set_value_pos (GTK_SCALE (scale), GTK_POS_TOP);
  gtk_table_attach_defaults (GTK_TABLE (table), scale, 1, 2, 1, 2);
  gtk_widget_show (scale);

  /*  show the table  */
  gtk_widget_show (table);

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
      g_message (_("Blend: Invalid for indexed images."));
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

  /* initialize the statusbar display */
  blend_tool->context_id = gtk_statusbar_get_context_id (GTK_STATUSBAR (gdisp->statusbar), "blend");
  gtk_statusbar_push (GTK_STATUSBAR (gdisp->statusbar), blend_tool->context_id, _("Blend: 0, 0"));

  /*  Start drawing the blend tool  */
  draw_core_start (blend_tool->core, gdisp->canvas->window, tool);
}

static void
blend_button_release (Tool           *tool,
		      GdkEventButton *bevent,
		      gpointer        gdisp_ptr)
{
  GDisplay * gdisp;
  GImage * gimage;
  BlendTool * blend_tool;
#ifdef BLEND_UI_CALLS_VIA_PDB
  Argument *return_vals;
  int nreturn_vals;
#else
  gimp_progress *progress;
#endif

  gdisp = (GDisplay *) gdisp_ptr;
  gimage = gdisp->gimage;
  blend_tool = (BlendTool *) tool->private;

  gdk_pointer_ungrab (bevent->time);
  gdk_flush ();

  gtk_statusbar_pop (GTK_STATUSBAR (gdisp->statusbar), blend_tool->context_id);

  draw_core_stop (blend_tool->core, tool);
  tool->state = INACTIVE;

  /*  if the 3rd button isn't pressed, fill the selected region  */
  if (! (bevent->state & GDK_BUTTON3_MASK) &&
      ((blend_tool->startx != blend_tool->endx) ||
       (blend_tool->starty != blend_tool->endy)))
    {
      /* we can't do callbacks easily with the PDB, so this UI/backend
       * separation (though good) is ignored for the moment */
#ifdef BLEND_UI_CALLS_VIA_PDB
      return_vals = procedural_db_run_proc ("gimp_blend",
					    &nreturn_vals,
					    PDB_DRAWABLE, drawable_ID (gimage_active_drawable (gimage)),
					    PDB_INT32, (gint32) blend_options->blend_mode,
					    PDB_INT32, (gint32) PAINT_OPTIONS_GET_PAINT_MODE (blend_options),
					    PDB_INT32, (gint32) blend_options->gradient_type,
					    PDB_FLOAT, (gdouble) PAINT_OPTIONS_GET_OPACITY (blend_options) * 100,
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

      if (return_vals && return_vals[0].value.pdb_int == PDB_SUCCESS)
	gdisplays_flush ();
      else
	g_message (_("Blend operation failed."));

      procedural_db_destroy_args (return_vals, nreturn_vals);

#else /* ! BLEND_UI_CALLS_VIA_PDB */

      progress = progress_start (gdisp, _("Blending..."), FALSE, NULL, NULL);

      blend (gimage,
	     gimage_active_drawable (gimage),
	     blend_options->blend_mode,
	     PAINT_OPTIONS_GET_PAINT_MODE (blend_options),
	     blend_options->gradient_type,
	     PAINT_OPTIONS_GET_OPACITY (blend_options) * 100,
	     blend_options->offset,
	     blend_options->repeat,
	     blend_options->supersample,
	     blend_options->max_depth,
	     blend_options->threshold,
	     blend_tool->startx,
	     blend_tool->starty,
	     blend_tool->endx,
	     blend_tool->endy,
	     progress? progress_update_and_flush : NULL, progress);

      if (progress)
	progress_end (progress);

      gdisplays_flush ();
#endif /* ! BLEND_UI_CALLS_VIA_PDB */
    }
}

static void
blend_motion (Tool           *tool,
	      GdkEventMotion *mevent,
	      gpointer        gdisp_ptr)
{
  GDisplay * gdisp;
  BlendTool * blend_tool;
  gchar vector[STATUSBAR_SIZE];

  gdisp = (GDisplay *) gdisp_ptr;
  blend_tool = (BlendTool *) tool->private;

  /*  undraw the current tool  */
  draw_core_pause (blend_tool->core, tool);

  /*  Get the current coordinates  */
  gdisplay_untransform_coords (gdisp, mevent->x, mevent->y,
			       &blend_tool->endx, &blend_tool->endy, FALSE, 1);

  gtk_statusbar_pop (GTK_STATUSBAR (gdisp->statusbar), blend_tool->context_id);
  if (gdisp->dot_for_dot)
    {
      g_snprintf (vector, STATUSBAR_SIZE, gdisp->cursor_format_str,
		  _("Blend: "),
		  abs(blend_tool->endx - blend_tool->startx),
		  ", ",
		  abs(blend_tool->endy - blend_tool->starty));
    }
  else /* show real world units */
    {
      float unit_factor = gimp_unit_get_factor (gdisp->gimage->unit);

      g_snprintf (vector, STATUSBAR_SIZE, gdisp->cursor_format_str,
		  _("Blend: "),
		  abs(blend_tool->endx - blend_tool->startx) * unit_factor /
		  gdisp->gimage->xresolution,
		  ", ",
		  abs(blend_tool->endy - blend_tool->starty) * unit_factor /
		  gdisp->gimage->yresolution);
    }
  gtk_statusbar_push (GTK_STATUSBAR (gdisp->statusbar), blend_tool->context_id,
		      vector);

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
void
blend (GImage          *gimage,
       GimpDrawable    *drawable,
       BlendMode        blend_mode,
       int              paint_mode,
       GradientType     gradient_type,
       double           opacity,
       double           offset,
       RepeatMode       repeat,
       int              supersample,
       int              max_depth,
       double           threshold,
       double           startx,
       double           starty,
       double           endx,
       double           endy,
       progress_func_t  progress_callback,
       void            *progress_data)
{
  TileManager *buf_tiles;
  PixelRegion bufPR;
  int has_alpha;
  int has_selection;
  int bytes;
  int x1, y1, x2, y2;

  gimp_add_busy_cursors();

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
			(endx - x1), (endy - y1),
			progress_callback, progress_data);

  pixel_region_init (&bufPR, buf_tiles, 0, 0, (x2 - x1), (y2 - y1), FALSE);
  gimage_apply_image (gimage, drawable, &bufPR, TRUE,
		      (opacity * 255) / 100, paint_mode, NULL, x1, y1);

  /*  update the image  */
  drawable_update (drawable, x1, y1, (x2 - x1), (y2 - y1));

  /*  free the temporary buffer  */
  tile_manager_destroy (buf_tiles);

  gimp_remove_busy_cursors(NULL);
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
gradient_calc_spiral_factor (double  dist,
			     double *axis,
			     double  offset,
			     double  x,
			     double  y,
			     gint cwise)
{
  double ang0, ang1;
  double ang, r;
  double rat;

  if (dist == 0.0)
    rat = 0.0;
  else
    {
      if (x != 0.0 || y != 0.0)
	{
	  ang0 = atan2 (axis[0], axis[1]) + M_PI;
	  ang1 = atan2 (x, y) + M_PI;
	  if(!cwise)
	    ang = ang0 - ang1;
	  else
	    ang = ang1 - ang0;

	  if (ang < 0.0)
	    ang += (2.0 * M_PI);

	  r = sqrt (x * x + y * y) / dist;
	  rat = ang / (2.0 * M_PI) + r + offset;
	  rat = fmod (rat, 1.0);
	}
      else
	rat = 0.5 ; /* We are on the middle point */
    }

  return rat;
}

static double
gradient_calc_shapeburst_angular_factor (double x,
					 double y)
{
  int ix, iy;
  Tile *tile;
  float value;

  ix = (int) BOUNDS (x, 0, distR.w);
  iy = (int) BOUNDS (y, 0, distR.h);
  tile = tile_manager_get_tile (distR.tiles, ix, iy, TRUE, FALSE);
  value = 1.0 - *((float *) tile_data_pointer (tile, ix % TILE_WIDTH, iy % TILE_HEIGHT));
  tile_release (tile, FALSE);

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
  tile = tile_manager_get_tile (distR.tiles, ix, iy, TRUE, FALSE);
  value = *((float *) tile_data_pointer (tile, ix % TILE_WIDTH, iy % TILE_HEIGHT));
  value = 1.0 - sin (0.5 * M_PI * value);
  tile_release (tile, FALSE);

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
  tile = tile_manager_get_tile (distR.tiles, ix, iy, TRUE, FALSE);
  value = *((float *) tile_data_pointer (tile, ix % TILE_WIDTH, iy % TILE_HEIGHT));
  value = cos (0.5 * M_PI * value);
  tile_release (tile, FALSE);

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
    case RADIAL:
      factor = gradient_calc_radial_factor(rbd->dist, rbd->offset,
					   x - rbd->sx, y - rbd->sy);
      break;

    case CONICAL_SYMMETRIC:
      factor = gradient_calc_conical_sym_factor(rbd->dist, rbd->vec, rbd->offset,
						x - rbd->sx, y - rbd->sy);
      break;

    case CONICAL_ASYMMETRIC:
      factor = gradient_calc_conical_asym_factor(rbd->dist, rbd->vec, rbd->offset,
						 x - rbd->sx, y - rbd->sy);
      break;

    case SQUARE:
      factor = gradient_calc_square_factor(rbd->dist, rbd->offset,
					   x - rbd->sx, y - rbd->sy);
      break;

    case LINEAR:
      factor = gradient_calc_linear_factor(rbd->dist, rbd->vec,
					   x - rbd->sx, y - rbd->sy);
      break;

    case BILINEAR:
      factor = gradient_calc_bilinear_factor(rbd->dist, rbd->vec, rbd->offset,
					     x - rbd->sx, y - rbd->sy);
      break;

    case SHAPEBURST_ANGULAR:
      factor = gradient_calc_shapeburst_angular_factor(x, y);
      break;

    case SHAPEBURST_SPHERICAL:
      factor = gradient_calc_shapeburst_spherical_factor(x, y);
      break;

    case SHAPEBURST_DIMPLED:
      factor = gradient_calc_shapeburst_dimpled_factor(x, y);
      break;

    case SPIRAL_CLOCKWISE:
      factor = gradient_calc_spiral_factor (rbd->dist, rbd->vec, rbd->offset,
					    x - rbd->sx, y - rbd->sy,TRUE);
      break;

    case SPIRAL_ANTICLOCKWISE:
      factor = gradient_calc_spiral_factor (rbd->dist, rbd->vec, rbd->offset,
					    x - rbd->sx, y - rbd->sy,FALSE);
      break;

    default:
      fatal_error(_("gradient_render_pixel(): unknown gradient type %d"),
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
gradient_fill_region (GImage          *gimage,
		      GimpDrawable    *drawable,
		      PixelRegion     *PR,
		      int              width,
		      int              height,
		      BlendMode        blend_mode,
		      GradientType     gradient_type,
		      double           offset,
		      RepeatMode       repeat,
		      int              supersample,
		      int              max_depth,
		      double           threshold,
		      double           sx,
		      double           sy,
		      double           ex,
		      double           ey,
		      progress_func_t  progress_callback,
		      void            *progress_data)
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
      fatal_error(_("gradient_fill_region(): unknown blend mode %d"),
		  (int) blend_mode);
      break;
    }

  /* Calculate type-specific parameters */

  switch (gradient_type)
    {
    case RADIAL:
      rbd.dist = sqrt(SQR(ex - sx) + SQR(ey - sy));
      break;

    case SQUARE:
      rbd.dist = MAXIMUM(fabs(ex - sx), fabs(ey - sy));
      break;

    case CONICAL_SYMMETRIC:
    case CONICAL_ASYMMETRIC:
    case SPIRAL_CLOCKWISE:
    case SPIRAL_ANTICLOCKWISE:
    case LINEAR:
    case BILINEAR:
      rbd.dist = sqrt(SQR(ex - sx) + SQR(ey - sy));

      if (rbd.dist > 0.0)
	{
	  rbd.vec[0] = (ex - sx) / rbd.dist;
	  rbd.vec[1] = (ey - sy) / rbd.dist;
	}

      break;

    case SHAPEBURST_ANGULAR:
    case SHAPEBURST_SPHERICAL:
    case SHAPEBURST_DIMPLED:
      rbd.dist = sqrt(SQR(ex - sx) + SQR(ey - sy));
      gradient_precalc_shapeburst(gimage, drawable, PR, rbd.dist);
      break;

    default:
      fatal_error(_("gradient_fill_region(): unknown gradient type %d"),
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
      fatal_error(_("gradient_fill_region(): unknown repeat mode %d"),
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
				progress_callback, progress_data);

      /* Clean up */

      g_free(ppd.row_data);
    }
  else
    {
      int max_progress = PR->w * PR->h;
      int progress = 0;

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

	  progress += PR->w * PR->h;
	  if (progress_callback)
	    (*progress_callback) (0, max_progress, progress, progress_data);
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

  /*  The tool options  */
  if (! blend_options)
    {
      blend_options = blend_options_new ();
      tools_register (BLEND, (ToolOptions *) blend_options);
    }

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
  tool->arrow_keys_func = standard_arrow_keys_func;  tool->toggle_key_func = standard_toggle_key_func;
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
