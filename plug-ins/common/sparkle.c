/* Sparkle --- image filter plug-in for The Gimp image manipulation program
 * Copyright (C) 1996 by John Beale;  ported to Gimp by Michael J. Hammel;
 * It has been optimized a little, bugfixed and modified by Martin Weber
 * for additional functionality.
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
 *
 * You can contact Michael at mjhammel@csn.net
 * You can contact Martin at martin.weber@usa.net
 * Note: set tabstops to 3 to make this more readable.
 */

/*
 * Sparkle 1.26 - simulate pixel bloom and diffraction effects
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "config.h"
#include "gtk/gtk.h"
#include "libgimp/gimp.h"
#include "libgimp/gimpcolorspace.h"
#include "libgimp/stdplugins-intl.h"

#define SCALE_WIDTH 175
#define MAX_CHANNELS 4
#define PSV 2  /* point spread value */
#define EPSILON 0.001
#define SQR(a) ((a) * (a))

#define  NATURAL  0
#define  FOREGROUND 1
#define  BACKGROUND 2

typedef struct
{
  gdouble lum_threshold;
  gdouble flare_inten;
  gdouble spike_len;
  gdouble spike_pts;
  gdouble spike_angle;
  gdouble density;
  gdouble opacity;
  gdouble random_hue;
  gdouble random_saturation;
  gint preserve_luminosity;
  gint invers;
  gint border;
  gint colortype;
} SparkleVals;

typedef struct
{
  gint       run;
} SparkleInterface;

/* Declare local functions.
 */

static void      query  (void);
static void      run    (char      *name,
			 int        nparams,
			 GParam    *param,
			 int       *nreturn_vals,
			 GParam   **return_vals);
static void      sparkle_toggle_update (GtkWidget  *widget,
				        gpointer   data);
static gint      sparkle_dialog        (void);
static gint      compute_luminosity    (guchar *   pixel,
					gint       gray,
					gint       has_alpha);
static gint      compute_lum_threshold (GDrawable * drawable,
					gdouble    percentile);
static void      sparkle               (GDrawable * drawable,
					gint       threshold);
static void      fspike                (GPixelRgn * dest_rgn,
					gint       gray,
					gint       x1,
					gint       y1,
					gint       x2,
					gint       y2,
					gdouble    xr,
					gdouble    yr,
					gint       tile_width,
					gint       tile_height,
					gdouble    inten,
					gdouble    length,
					gdouble    angle);
static GTile*    rpnt                  (GDrawable * drawable,
					GTile *    tile,
					gint       x1,
					gint       y1,
					gint       x2,
					gint       y2,
					gdouble    xr,
					gdouble    yr,
					gint       tile_width,
					gint       tile_height,
					gint *     row,
					gint *     col,
					gint       bytes,
					gdouble    inten,
					guchar color[MAX_CHANNELS]);
static void      sparkle_close_callback(GtkWidget *widget,
					gpointer   data);
static void      sparkle_ok_callback   (GtkWidget *widget,
					gpointer   data);
static void      sparkle_scale_update  (GtkAdjustment *adjustment,
					double        *scale_val);
static void      set_tooltip (GtkTooltips *tooltips,
                                        GtkWidget *widget,
                                        const char *desc);

GPlugInInfo PLUG_IN_INFO =
{
  NULL,    /* init_proc */
  NULL,    /* quit_proc */
  query,   /* query_proc */
  run,     /* run_proc */
};

static SparkleVals svals =
{
  0.001,	/* luminosity threshold */
  0.5,	/* flare intensity */
  20.0,	/* spike length */
  4.0,	/* spike points */
  15.0,	/* spike angle */
  1.0,	/* spike density */
  1.0,	/* opacity */
  0.0,	/* random hue */
  0.0,	/* random saturation */
  FALSE,	/* preserve_luminosity */
  FALSE,	/* invers */
  FALSE,	/* border */
  NATURAL	/* colortype */
};

static SparkleInterface sint =
{
  FALSE          /* run */
};

static gint num_sparkles;


MAIN ()

static void
query ()
{
  static GParamDef args[] =
  {
    { PARAM_INT32, "run_mode", "Interactive, non-interactive" },
    { PARAM_IMAGE, "image", "Input image (unused)" },
    { PARAM_DRAWABLE, "drawable", "Input drawable" },
    { PARAM_FLOAT, "lum_threshold", "Luminosity threshold (0.0 - 1.0)" },
    { PARAM_FLOAT, "flare_inten", "Flare intensity (0.0 - 1.0)" },
    { PARAM_INT32, "spike_len", "Spike length (in pixels)" },
    { PARAM_INT32, "spike_pts", "# of spike points" },
    { PARAM_INT32, "spike_angle", "Spike angle (0-360 degrees, -1: random)" },
    { PARAM_FLOAT, "density", "Spike density (0.0 - 1.0)" },
    { PARAM_FLOAT, "opacity", "Opacity (0.0 - 1.0)" },
    { PARAM_FLOAT, "random_hue", "Random hue (0.0 - 1.0)" },
    { PARAM_FLOAT, "random_saturation", "Random saturation (0.0 - 1.0)" },
    { PARAM_INT32, "preserve_luminosity", "Preserve luminosity (TRUE/FALSE)" },
    { PARAM_INT32, "invers", "Invers (TRUE/FALSE)" },
    { PARAM_INT32, "border", "Add border (TRUE/FALSE)" },
    { PARAM_INT32, "colortype", "Color of sparkles: { NATURAL (0), FOREGROUND (1), BACKGROUND (2) }" }
  };
  static GParamDef *return_vals = NULL;
  static int nargs = sizeof (args) / sizeof (args[0]);
  static int nreturn_vals = 0;

  INIT_I18N();

  gimp_install_procedure ("plug_in_sparkle",
             _("Simulates pixel bloom and diffraction effects"),
			  "No help yet",
                    "John Beale, & (ported to GIMP v0.54) Michael J. Hammel & ted to GIMP v1.0) Spencer Kimball",
			  "John Beale",
			  "Version 1.26, December 1998",
			  N_("<Image>/Filters/Light Effects/Sparkle..."),
			  "RGB*, GRAY*",
			  PROC_PLUG_IN,
			  nargs, nreturn_vals,
			  args, return_vals);
}

static void
run (char    *name,
     int      nparams,
     GParam  *param,
     int     *nreturn_vals,
     GParam **return_vals)
{
  static GParam values[1];
  GDrawable *drawable;
  GRunModeType run_mode;
  GStatusType status = STATUS_SUCCESS;
  gint threshold, x1, y1, x2, y2;

  run_mode = param[0].data.d_int32;

  *nreturn_vals = 1;
  *return_vals = values;

  values[0].type = PARAM_STATUS;
  values[0].data.d_status = status;

  switch (run_mode)
    {
    case RUN_INTERACTIVE:
      INIT_I18N_UI();
      /*  Possibly retrieve data  */
      gimp_get_data ("plug_in_sparkle", &svals);

      /*  First acquire information with a dialog  */
      if (! sparkle_dialog ())
	return;
      break;

    case RUN_NONINTERACTIVE:
      INIT_I18N();
      /*  Make sure all the arguments are there!  */
      if (nparams != 16)
	status = STATUS_CALLING_ERROR;
      if (status == STATUS_SUCCESS)
	{
	  svals.lum_threshold = param[3].data.d_float;
	  svals.flare_inten = param[4].data.d_float;
	  svals.spike_len = param[5].data.d_int32;
	  svals.spike_pts = param[6].data.d_int32;
	  svals.spike_angle = param[7].data.d_int32;
	  svals.density = param[8].data.d_float;
	  svals.opacity = param[9].data.d_float;
	  svals.random_hue = param[10].data.d_float;
	  svals.random_saturation = param[11].data.d_float;
	  svals.preserve_luminosity = (param[12].data.d_int32) ? TRUE : FALSE;
	  svals.invers = (param[13].data.d_int32) ? TRUE : FALSE;
	  svals.border = (param[14].data.d_int32) ? TRUE : FALSE;
	  svals.colortype = param[15].data.d_int32;
	}
      if (status == STATUS_SUCCESS &&
	  (svals.lum_threshold < 0.0 || svals.lum_threshold > 1.0))
	status = STATUS_CALLING_ERROR;
      if (status == STATUS_SUCCESS &&
	  (svals.flare_inten < 0.0 || svals.flare_inten > 1.0))
	status = STATUS_CALLING_ERROR;
      if (status == STATUS_SUCCESS &&
	  (svals.spike_len < 0))
	status = STATUS_CALLING_ERROR;
      if (status == STATUS_SUCCESS &&
	  (svals.spike_pts < 0))
	status = STATUS_CALLING_ERROR;
      if (status == STATUS_SUCCESS &&
	  (svals.spike_angle < -1 || svals.spike_angle > 360))
	status = STATUS_CALLING_ERROR;
      if (status == STATUS_SUCCESS &&
	  (svals.density < 0.0 || svals.density > 1.0))
	status = STATUS_CALLING_ERROR;
      if (status == STATUS_SUCCESS &&
	  (svals.opacity < 0.0 || svals.opacity > 1.0))
	status = STATUS_CALLING_ERROR;
      if (status == STATUS_SUCCESS &&
	  (svals.random_hue < 0.0 || svals.random_hue > 1.0))
	status = STATUS_CALLING_ERROR;
      if (status == STATUS_SUCCESS &&
	  (svals.random_saturation < 0.0 || svals.random_saturation > 1.0))
	status = STATUS_CALLING_ERROR;
      if (status == STATUS_SUCCESS &&
	  (svals.colortype < NATURAL || svals.colortype > BACKGROUND))
	status = STATUS_CALLING_ERROR;

      break;

    case RUN_WITH_LAST_VALS:
      INIT_I18N();
      /*  Possibly retrieve data  */
      gimp_get_data ("plug_in_sparkle", &svals);
      break;

    default:
      break;
    }

  /*  Get the specified drawable  */
  drawable = gimp_drawable_get (param[2].data.d_drawable);

  /*  Make sure that the drawable is gray or RGB color  */
  if (gimp_drawable_is_rgb (drawable->id) || gimp_drawable_is_gray (drawable->id))
    {
      gimp_progress_init ( _("Sparkling..."));
      gimp_tile_cache_ntiles (2 * (drawable->width / gimp_tile_width () + 1));

      if (svals.border == FALSE)
          /*  compute the luminosity which exceeds the luminosity threshold  */
          threshold = compute_lum_threshold (drawable, svals.lum_threshold);
      else
        {
          gimp_drawable_mask_bounds (drawable->id, &x1, &y1, &x2, &y2);
          num_sparkles = 2 * (x2 - x1 + y2 - y1);
          threshold = 255;
         }
          
      sparkle (drawable, threshold);

      if (run_mode != RUN_NONINTERACTIVE)
	gimp_displays_flush ();

      /*  Store mvals data  */
      if (run_mode == RUN_INTERACTIVE)
	gimp_set_data ("plug_in_sparkle", &svals, sizeof (SparkleVals));
    }
  else
    {
      /* gimp_message ("sparkle: cannot operate on indexed color images"); */
      status = STATUS_EXECUTION_ERROR;
    }

  values[0].data.d_status = status;

  gimp_drawable_detach (drawable);
}

static gint
sparkle_dialog ()
{
  GtkWidget *dlg;
  GtkWidget *label;
  GtkWidget *hbbox;
  GtkWidget *button;
  GtkWidget *scale;
  GtkWidget *frame;
  GtkWidget *table;
  GtkWidget *toggle;
  GtkObject *scale_data;
  GSList    *group = NULL;
  gchar **argv;
  gint argc;
  gint use_natural = (svals.colortype == NATURAL);
  gint use_foreground = (svals.colortype == FOREGROUND);
  gint use_background = (svals.colortype == BACKGROUND);
  GtkTooltips *tips;
  GdkColor tips_fg, tips_bg;	

  argc = 1;
  argv = g_new (gchar *, 1);
  argv[0] = g_strdup ("sparkle");

  gtk_init (&argc, &argv);
  gtk_rc_parse (gimp_gtkrc ());


  dlg = gtk_dialog_new ();
  gtk_window_set_title (GTK_WINDOW (dlg), _("Sparkle"));
  gtk_window_position (GTK_WINDOW (dlg), GTK_WIN_POS_MOUSE);
  gtk_signal_connect (GTK_OBJECT (dlg), "destroy",
		      (GtkSignalFunc) sparkle_close_callback,
		      NULL);

  /* use black as foreground: */
  tips = gtk_tooltips_new ();
  tips_fg.red   = 0;
  tips_fg.green = 0;
  tips_fg.blue  = 0;
  /* postit yellow (khaki) as background: */
  gdk_color_alloc (gtk_widget_get_colormap (dlg), &tips_fg);
  tips_bg.red   = 61669;
  tips_bg.green = 59113;
  tips_bg.blue  = 35979;
  gdk_color_alloc (gtk_widget_get_colormap (dlg), &tips_bg);
  gtk_tooltips_set_colors (tips,&tips_bg,&tips_fg);

  /*  Action area  */
  gtk_container_set_border_width (GTK_CONTAINER (GTK_DIALOG (dlg)->action_area), 2);
  gtk_box_set_homogeneous (GTK_BOX (GTK_DIALOG (dlg)->action_area), FALSE);
  hbbox = gtk_hbutton_box_new ();
  gtk_button_box_set_spacing (GTK_BUTTON_BOX (hbbox), 4);
  gtk_box_pack_end (GTK_BOX (GTK_DIALOG (dlg)->action_area), hbbox, FALSE, FALSE, 0);
  gtk_widget_show (hbbox);
 
  button = gtk_button_new_with_label ( _("OK"));
  GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT);
  gtk_signal_connect (GTK_OBJECT (button), "clicked",
		      (GtkSignalFunc) sparkle_ok_callback,
		      dlg);
  gtk_box_pack_start (GTK_BOX (hbbox), button, FALSE, FALSE, 0);
  gtk_widget_grab_default (button);
  gtk_widget_show (button);
  set_tooltip(tips,button, _("Accept settings and apply filter on image"));

  button = gtk_button_new_with_label ( _("Cancel"));
  GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT);
  gtk_signal_connect_object (GTK_OBJECT (button), "clicked",
			     (GtkSignalFunc) gtk_widget_destroy,
			     GTK_OBJECT (dlg));
  gtk_box_pack_start (GTK_BOX (hbbox), button, FALSE, FALSE, 0);
  gtk_widget_show (button);
  set_tooltip(tips,button, _("Reject any changes and close plug-in"));

  /*  parameter settings  */
  frame = gtk_frame_new ( _("Parameter Settings"));
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_ETCHED_IN);
  gtk_container_border_width (GTK_CONTAINER (frame), 10);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dlg)->vbox), frame, TRUE, TRUE, 0);
  table = gtk_table_new (15, 2, FALSE);
  gtk_container_border_width (GTK_CONTAINER (table), 10);
  gtk_container_add (GTK_CONTAINER (frame), table);


  label = gtk_label_new ( _("Luminosity Threshold"));
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gtk_table_attach (GTK_TABLE (table), label, 0, 1, 0, 1, GTK_FILL, 0, 5, 5);
  scale_data = gtk_adjustment_new (svals.lum_threshold, 0.0, 0.1, 0.001, 0.001, 0.0);
  scale = gtk_hscale_new (GTK_ADJUSTMENT (scale_data));
  gtk_widget_set_usize (scale, SCALE_WIDTH, 0);
  gtk_table_attach (GTK_TABLE (table), scale, 1, 2, 0, 1, GTK_FILL, 0, 0, 0);
  gtk_scale_set_value_pos (GTK_SCALE (scale), GTK_POS_TOP);
  gtk_scale_set_digits (GTK_SCALE (scale), 3);
  gtk_range_set_update_policy (GTK_RANGE (scale), GTK_UPDATE_DELAYED);
  gtk_signal_connect (GTK_OBJECT (scale_data), "value_changed",
		      (GtkSignalFunc) sparkle_scale_update,
		      &svals.lum_threshold);
  gtk_widget_show (label);
  gtk_widget_show (scale);
  set_tooltip(tips,scale, _("Adjust the Luminosity Threshold"));


  label = gtk_label_new ( _("Flare Intensity"));
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gtk_table_attach (GTK_TABLE (table), label, 0, 1, 1, 2, GTK_FILL, 0, 5, 5);
  scale_data = gtk_adjustment_new (svals.flare_inten, 0.0, 1.0, 0.01, 0.01, 0.0);
  scale = gtk_hscale_new (GTK_ADJUSTMENT (scale_data));
  gtk_widget_set_usize (scale, SCALE_WIDTH, 0);
  gtk_table_attach (GTK_TABLE (table), scale, 1, 2, 1, 2, GTK_FILL, 0, 0, 0);
  gtk_scale_set_value_pos (GTK_SCALE (scale), GTK_POS_TOP);
  gtk_scale_set_digits (GTK_SCALE (scale), 2);
  gtk_range_set_update_policy (GTK_RANGE (scale), GTK_UPDATE_DELAYED);
  gtk_signal_connect (GTK_OBJECT (scale_data), "value_changed",
		      (GtkSignalFunc) sparkle_scale_update,
		      &svals.flare_inten);
  gtk_widget_show (label);
  gtk_widget_show (scale);
  set_tooltip(tips,scale, _("Adjust the Flare Intensity"));

  label = gtk_label_new ( _("Spike Length"));
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gtk_table_attach (GTK_TABLE (table), label, 0, 1, 2, 3, GTK_FILL, 0, 5, 5);
  scale_data = gtk_adjustment_new (svals.spike_len, 1, 100, 1, 1, 0);
  scale = gtk_hscale_new (GTK_ADJUSTMENT (scale_data));
  gtk_widget_set_usize (scale, SCALE_WIDTH, 0);
  gtk_table_attach (GTK_TABLE (table), scale, 1, 2, 2, 3, GTK_FILL, 0, 0, 0);
  gtk_scale_set_value_pos (GTK_SCALE (scale), GTK_POS_TOP);
  gtk_scale_set_digits (GTK_SCALE (scale), 0);
  gtk_range_set_update_policy (GTK_RANGE (scale), GTK_UPDATE_DELAYED);
  gtk_signal_connect (GTK_OBJECT (scale_data), "value_changed",
		      (GtkSignalFunc) sparkle_scale_update,
		      &svals.spike_len);
  gtk_widget_show (label);
  gtk_widget_show (scale);
  set_tooltip(tips,scale, _("Adjust the Spike Length"));

  label = gtk_label_new ( _("Spike Points"));
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gtk_table_attach (GTK_TABLE (table), label, 0, 1, 3, 4, GTK_FILL, 0, 5, 5);
  scale_data = gtk_adjustment_new (svals.spike_pts, 0, 16, 1, 1, 0);
  scale = gtk_hscale_new (GTK_ADJUSTMENT (scale_data));
  gtk_widget_set_usize (scale, SCALE_WIDTH, 0);
  gtk_table_attach (GTK_TABLE (table), scale, 1, 2, 3, 4, GTK_FILL, 0, 0, 0);
  gtk_scale_set_value_pos (GTK_SCALE (scale), GTK_POS_TOP);
  gtk_scale_set_digits (GTK_SCALE (scale), 0);
  gtk_range_set_update_policy (GTK_RANGE (scale), GTK_UPDATE_DELAYED);
  gtk_signal_connect (GTK_OBJECT (scale_data), "value_changed",
		      (GtkSignalFunc) sparkle_scale_update,
		      &svals.spike_pts);
  gtk_widget_show (label);
  gtk_widget_show (scale);
  set_tooltip(tips,scale, _("Adjust the Number of Spike Points"));

  label = gtk_label_new ( _("Spike Angle (-1: Random)"));
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gtk_table_attach (GTK_TABLE (table), label, 0, 1, 4, 5, GTK_FILL, 0, 5, 5);
  scale_data = gtk_adjustment_new (svals.spike_angle, -1, 360, 5, 5, 0);
  scale = gtk_hscale_new (GTK_ADJUSTMENT (scale_data));
  gtk_widget_set_usize (scale, SCALE_WIDTH, 0);
  gtk_table_attach (GTK_TABLE (table), scale, 1, 2, 4, 5, GTK_FILL, 0, 0, 0);
  gtk_scale_set_value_pos (GTK_SCALE (scale), GTK_POS_TOP);
  gtk_scale_set_digits (GTK_SCALE (scale), 0);
  gtk_range_set_update_policy (GTK_RANGE (scale), GTK_UPDATE_DELAYED);
  gtk_signal_connect (GTK_OBJECT (scale_data), "value_changed",
		      (GtkSignalFunc) sparkle_scale_update,
		      &svals.spike_angle);
  gtk_widget_show (label);
  gtk_widget_show (scale);
  set_tooltip(tips,scale, _("Adjust the Spike Angle (-1 means a Random Angle is choosen)"));

  label = gtk_label_new ( _("Spike Density"));
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gtk_table_attach (GTK_TABLE (table), label, 0, 1, 5, 6, GTK_FILL, 0, 5, 5);
  scale_data = gtk_adjustment_new (svals.density, 0.0, 1.0, 0.01, 0.01, 0.0);
  scale = gtk_hscale_new (GTK_ADJUSTMENT (scale_data));
  gtk_widget_set_usize (scale, SCALE_WIDTH, 0);
  gtk_table_attach (GTK_TABLE (table), scale, 1, 2, 5, 6, GTK_FILL, 0, 0, 0);
  gtk_scale_set_value_pos (GTK_SCALE (scale), GTK_POS_TOP);
  gtk_scale_set_digits (GTK_SCALE (scale), 2);
  gtk_range_set_update_policy (GTK_RANGE (scale), GTK_UPDATE_DELAYED);
  gtk_signal_connect (GTK_OBJECT (scale_data), "value_changed",
		      (GtkSignalFunc) sparkle_scale_update,
		      &svals.density);
  gtk_widget_show (label);
  gtk_widget_show (scale);
  set_tooltip(tips,scale, _("Adjust the Spike Density"));

  label = gtk_label_new ( _("Opacity"));
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gtk_table_attach (GTK_TABLE (table), label, 0, 1, 6, 7, GTK_FILL, 0, 5, 5);
  scale_data = gtk_adjustment_new (svals.opacity, 0.0, 1.0, 0.01, 0.01, 0.0);
  scale = gtk_hscale_new (GTK_ADJUSTMENT (scale_data));
  gtk_widget_set_usize (scale, SCALE_WIDTH, 0);
  gtk_table_attach (GTK_TABLE (table), scale, 1, 2, 6, 7, GTK_FILL, 0, 0, 0);
  gtk_scale_set_value_pos (GTK_SCALE (scale), GTK_POS_TOP);
  gtk_scale_set_digits (GTK_SCALE (scale), 2);
  gtk_range_set_update_policy (GTK_RANGE (scale), GTK_UPDATE_DELAYED);
  gtk_signal_connect (GTK_OBJECT (scale_data), "value_changed",
		      (GtkSignalFunc) sparkle_scale_update,
		      &svals.opacity);
  gtk_widget_show (label);
  gtk_widget_show (scale);
  set_tooltip(tips,scale, _("Adjust the Opacity of the Spikes"));

  label = gtk_label_new ( _("Random Hue"));
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gtk_table_attach (GTK_TABLE (table), label, 0, 1, 7, 8, GTK_FILL, 0, 5, 5);
  scale_data = gtk_adjustment_new (svals.random_hue, 0.0, 1.0, 0.01, 0.01, 0.0);
  scale = gtk_hscale_new (GTK_ADJUSTMENT (scale_data));
  gtk_widget_set_usize (scale, SCALE_WIDTH, 0);
  gtk_table_attach (GTK_TABLE (table), scale, 1, 2, 7, 8, GTK_FILL, 0, 0, 0);
  gtk_scale_set_value_pos (GTK_SCALE (scale), GTK_POS_TOP);
  gtk_scale_set_digits (GTK_SCALE (scale), 2);
  gtk_range_set_update_policy (GTK_RANGE (scale), GTK_UPDATE_DELAYED);
  gtk_signal_connect (GTK_OBJECT (scale_data), "value_changed",
		      (GtkSignalFunc) sparkle_scale_update,
		      &svals.random_hue);
  gtk_widget_show (label);
  gtk_widget_show (scale);
  set_tooltip(tips,scale, _("Adjust the Value how much the Hue should be changed randomly"));

  label = gtk_label_new ( _("Random Saturation"));
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gtk_table_attach (GTK_TABLE (table), label, 0, 1, 8, 9, GTK_FILL, 0, 5, 5);
  scale_data = gtk_adjustment_new (svals.random_saturation, 0.0, 1.0, 0.01, 0.01, 0.0);
  scale = gtk_hscale_new (GTK_ADJUSTMENT (scale_data));
  gtk_widget_set_usize (scale, SCALE_WIDTH, 0);
  gtk_table_attach (GTK_TABLE (table), scale, 1, 2, 8, 9, GTK_FILL, 0, 0, 0);
  gtk_scale_set_value_pos (GTK_SCALE (scale), GTK_POS_TOP);
  gtk_scale_set_digits (GTK_SCALE (scale), 2);
  gtk_range_set_update_policy (GTK_RANGE (scale), GTK_UPDATE_DELAYED);
  gtk_signal_connect (GTK_OBJECT (scale_data), "value_changed",
		      (GtkSignalFunc) sparkle_scale_update,
		      &svals.random_saturation);
  gtk_widget_show (label);
  gtk_widget_show (scale);
  set_tooltip(tips,scale, _("Adjust the Value how much the Saturation should be changed randomly"));

  label = gtk_label_new ( _("Preserve Luminosity"));
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gtk_table_attach (GTK_TABLE (table), label, 0, 1, 9, 10, GTK_FILL, 0, 5, 5);
  gtk_widget_show(label);
  toggle = gtk_check_button_new ();

  gtk_table_attach (GTK_TABLE (table), toggle, 1, 2, 9, 10, GTK_FILL, 0, 0, 0);
  gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (toggle), svals.preserve_luminosity);
  gtk_signal_connect (GTK_OBJECT (toggle), "toggled",
		      (GtkSignalFunc) sparkle_toggle_update,
		      &svals.preserve_luminosity);
  gtk_widget_show (toggle);
  set_tooltip(tips,toggle, _("Should the Luminosity be preserved?"));

  label = gtk_label_new ( _("Invers"));
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gtk_table_attach (GTK_TABLE (table), label, 0, 1, 10, 11, GTK_FILL, 0, 5, 5);
  gtk_widget_show(label);
  toggle = gtk_check_button_new ();

  gtk_table_attach (GTK_TABLE (table), toggle, 1, 2, 10, 11, GTK_FILL, 0, 0, 0);
  gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (toggle), svals.invers);
  gtk_signal_connect (GTK_OBJECT (toggle), "toggled",
		      (GtkSignalFunc) sparkle_toggle_update,
		      &svals.invers);
  gtk_widget_show (toggle);
  set_tooltip(tips,toggle, _("Should an Inverse Effect be done?"));

  label = gtk_label_new ( _("Add Border"));
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gtk_table_attach (GTK_TABLE (table), label, 0, 1, 11, 12, GTK_FILL, 0, 5, 5);
  gtk_widget_show(label);
  toggle = gtk_check_button_new ();
  gtk_table_attach (GTK_TABLE (table), toggle, 1, 2, 11, 12, GTK_FILL, 0, 0, 0);
  gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (toggle), svals.border);
  gtk_signal_connect (GTK_OBJECT (toggle), "toggled",
		      (GtkSignalFunc) sparkle_toggle_update,
		      &svals.border);
  gtk_widget_show (toggle);
  set_tooltip(tips,toggle, _("Draw a Border of Spikes around the Image"));

 /*  colortype  */
  label = gtk_label_new ( _("Natural Color"));
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gtk_table_attach (GTK_TABLE (table), label, 0, 1, 12, 13, GTK_FILL, 0, 5, 5);
  gtk_widget_show(label);

  toggle = gtk_radio_button_new (group);
  gtk_table_attach (GTK_TABLE (table), toggle, 1, 2, 12, 13, GTK_FILL, 0, 0, 0);
  group = gtk_radio_button_group (GTK_RADIO_BUTTON (toggle));
  gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (toggle), use_natural);
  gtk_signal_connect (GTK_OBJECT (toggle), "toggled",
		      (GtkSignalFunc) sparkle_toggle_update,
		      &use_natural);
  gtk_widget_show (toggle);
  set_tooltip(tips,toggle, _("Use the Color of the Image"));

  label = gtk_label_new ( _("Foreground Color"));
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gtk_table_attach (GTK_TABLE (table), label, 0, 1, 13, 14, GTK_FILL, 0, 5, 5);
  gtk_widget_show(label);

  toggle = gtk_radio_button_new (group);
  gtk_table_attach (GTK_TABLE (table), toggle, 1, 2, 13, 14, GTK_FILL, 0, 0, 0);
  group = gtk_radio_button_group (GTK_RADIO_BUTTON (toggle));
  gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (toggle), use_foreground);
  gtk_signal_connect (GTK_OBJECT (toggle), "toggled",
		      (GtkSignalFunc) sparkle_toggle_update,
		      &use_foreground);
  gtk_widget_show (toggle);
  set_tooltip(tips,toggle, _("Use the Foreground Color"));

  label = gtk_label_new ( _("Background Color"));
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gtk_table_attach (GTK_TABLE (table), label, 0, 1, 14, 15, GTK_FILL, 0, 5, 5);
  gtk_widget_show(label);

  toggle = gtk_radio_button_new (group);
  gtk_table_attach (GTK_TABLE (table), toggle, 1, 2, 14, 15, GTK_FILL, 0, 0, 0);
  group = gtk_radio_button_group (GTK_RADIO_BUTTON (toggle));
  gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (toggle), use_background);
  gtk_signal_connect (GTK_OBJECT (toggle), "toggled",
		      (GtkSignalFunc) sparkle_toggle_update,
		      &use_background);
  gtk_widget_show (toggle);
  set_tooltip(tips,toggle, _("Use the Background Color"));


  gtk_widget_show (frame);
  gtk_widget_show (table);
  gtk_widget_show (dlg);

  gtk_main ();
  gdk_flush ();

  /*  determine colortype  */
  if (use_natural)
    svals.colortype = NATURAL;
  else if (use_foreground)
    svals.colortype = FOREGROUND;
  else if (use_background)
    svals.colortype = BACKGROUND;

  return sint.run;

}

static gint
compute_luminosity (guchar *pixel,
		    gint    gray,
		    gint    has_alpha)
{
  gint pixel0, pixel1, pixel2;

  if (svals.invers == FALSE)
    {
      pixel0 = pixel[0];
      pixel1 = pixel[1];
      pixel2 = pixel[2];
    }
  else
    {
      pixel0 = 255 - pixel[0];
      pixel1 = 255 - pixel[1];
      pixel2 = 255 - pixel[2];
    }
  if (gray)
    {
      if (has_alpha)
	return (pixel0 * pixel1) / 255;
      else
	return (pixel0);
    }
  else
    {
      gint min, max;

      min = MIN (pixel0, pixel1);
      min = MIN (min, pixel2);
      max = MAX (pixel0, pixel1);
      max = MAX (max, pixel2);

      if (has_alpha)
	return ((min + max) * pixel[3]) / 510;
      else
	return (min + max) / 2;
    }
}

static gint
compute_lum_threshold (GDrawable *drawable,
		       gdouble    percentile)
{
  GPixelRgn src_rgn;
  gpointer pr;
  guchar *data;
  gint values[256];
  gint total, sum;
  gint size;
  gint gray;
  gint has_alpha;
  gint i;
  gint x1, y1, x2, y2;

  /*  zero out the luminosity values array  */

  memset(values,0,sizeof(gint)*256);

  gimp_drawable_mask_bounds (drawable->id, &x1, &y1, &x2, &y2);
  gray = gimp_drawable_is_gray (drawable->id);
  has_alpha = gimp_drawable_has_alpha (drawable->id);

  gimp_pixel_rgn_init (&src_rgn, drawable, x1, y1, (x2 - x1), (y2 - y1), FALSE, FALSE);

  for (pr = gimp_pixel_rgns_register (1, &src_rgn); pr != NULL; pr = gimp_pixel_rgns_process (pr))
    {
      data = src_rgn.data;
      size = src_rgn.w * src_rgn.h;

      while (size--)
	{
	  values [compute_luminosity (data, gray, has_alpha)]++;
	  data += src_rgn.bpp;
	}
    }

  total = (x2 - x1) * (y2 - y1);
  sum = 0;

  for (i = 255; i >= 0; i--)
    {
      sum += values[i];
      if ((gdouble) sum / (gdouble) total > percentile)
	{
	  num_sparkles = sum;
	  return i;
	}
    }

  return 0;
}

static void
sparkle (GDrawable *drawable,
	 gint       threshold)
{
  GPixelRgn src_rgn, dest_rgn;
  guchar *src, *dest;
  gdouble nfrac, length, inten, spike_angle;
  gint cur_progress, max_progress;
  gint x1, y1, x2, y2;
  gint size, lum, x, y, b;
  gint gray;
  gint has_alpha, alpha;
  gpointer pr;
  guchar *tmp1;
  gint tile_width, tile_height;

  gimp_drawable_mask_bounds (drawable->id, &x1, &y1, &x2, &y2);
  gray = gimp_drawable_is_gray (drawable->id);
  has_alpha = gimp_drawable_has_alpha (drawable->id);
  alpha = (has_alpha) ? drawable->bpp - 1 : drawable->bpp;
  tile_width  = gimp_tile_width();
  tile_height = gimp_tile_height();

  /* initialize the progress dialog */
  cur_progress = 0;
  max_progress = num_sparkles;

  gimp_pixel_rgn_init (&src_rgn, drawable, x1, y1, (x2 - x1), (y2 - y1), FALSE, FALSE);
  gimp_pixel_rgn_init (&dest_rgn, drawable, x1, y1, (x2 - x1), (y2 - y1), TRUE, TRUE);

  for (pr = gimp_pixel_rgns_register (2, &src_rgn, &dest_rgn); pr != NULL; pr = gimp_pixel_rgns_process (pr))
    {
      src = src_rgn.data;
      dest = dest_rgn.data;

      size = src_rgn.w * src_rgn.h;

      while (size --)
        {
          if(has_alpha && src[alpha] == 0)
             {
               memset(dest, 0, alpha * sizeof(guchar));
               dest += alpha;
             }
          else
            {
              for (b = 0, tmp1 = src; b < alpha; b++)
                {
                  *dest++ = *tmp1++;
                }
            }
          if (has_alpha)
            *dest++ = src[alpha];

          src += src_rgn.bpp;
        }
    }

  /* add effects to new image based on intensity of old pixels */
  gimp_pixel_rgn_init (&src_rgn, drawable, x1, y1, (x2 - x1), (y2 - y1), FALSE, FALSE);
  gimp_pixel_rgn_init (&dest_rgn, drawable, x1, y1, (x2 - x1), (y2 - y1), TRUE, TRUE);

  for (pr = gimp_pixel_rgns_register (2, &src_rgn, &dest_rgn); pr != NULL; pr = gimp_pixel_rgns_process (pr))
    {
      src = src_rgn.data;

      for (y = 0; y < src_rgn.h; y++)
	for (x = 0; x < src_rgn.w; x++)
	  {
	    if (svals.border)
	      {
	        if (x + src_rgn.x == 0 || y + src_rgn.y == 0
		   || x + src_rgn.x == drawable->width - 1
		   || y + src_rgn.y == drawable->height - 1)
		   lum = 255;
		else
		   lum = 0;
	      }
	    else
	    	lum = compute_luminosity (src, gray, has_alpha);
	    if (lum >= threshold)
	      {
		nfrac = fabs ((gdouble) (lum + 1 - threshold) / (gdouble) (256 - threshold));
		length = (gdouble) svals.spike_len * (gdouble) pow (nfrac, 0.8);
		inten = svals.flare_inten * pow (nfrac, 1.0);

		/* fspike im x,y intens rlength angle */
		if (svals.spike_pts > 0)
		  {
		    /* major spikes */
		    if (svals.spike_angle == -1)
		   	spike_angle = 360.0 * rand () / RAND_MAX;
		    else
			spike_angle = svals.spike_angle;
		    if (rand() <= RAND_MAX * svals.density)
		      {
		        fspike (&dest_rgn, gray, x1, y1, x2, y2,
			    x + src_rgn.x, y + src_rgn.y,
			    tile_width, tile_height,
			    inten, length, spike_angle);
		        /* minor spikes */
		        fspike (&dest_rgn, gray, x1, y1, x2, y2,
			    x + src_rgn.x, y + src_rgn.y,
			    tile_width, tile_height,
			    inten * 0.7, length * 0.7,
			    ((gdouble) spike_angle + 180.0 / svals.spike_pts));
		      }
		  }
		cur_progress ++;
		if ((cur_progress % 5) == 0)
		  gimp_progress_update ((double) cur_progress / (double) max_progress);
	      }
	    src += src_rgn.bpp;
	  }
    }

  gimp_progress_update (1.0);

  /*  update the blurred region  */
  gimp_drawable_flush (drawable);
  gimp_drawable_merge_shadow (drawable->id, TRUE);
  gimp_drawable_update (drawable->id, x1, y1, (x2 - x1), (y2 - y1));
}

static inline GTile *
rpnt (GDrawable *drawable,
      GTile     *tile,
      gint       x1,
      gint       y1,
      gint       x2,
      gint       y2,
      gdouble    xr,
      gdouble    yr,
      gint       tile_width,
      gint       tile_height,
      gint      *row,
      gint      *col,
      gint       bytes,
      gdouble    inten,
      guchar     color[MAX_CHANNELS])
{
  gint x, y, b;
  gdouble dx, dy, rs, val;
  guchar *pixel;
  gdouble new;
  gint    newcol, newrow;
  gint    newcoloff, newrowoff;

  x = (int) (xr);	/* integer coord. to upper left of real point */
  y = (int) (yr);

  if (x >= x1 && y >= y1 && x < x2 && y < y2)
    {
      newcol    = x / tile_width;
      newcoloff = x % tile_width;
      newrow    = y / tile_height;
      newrowoff = y % tile_height;
      
      if ((newcol != *col) || (newrow != *row))
	{
	  *col = newcol;
	  *row = newrow;
	  if (tile)
	    gimp_tile_unref (tile, TRUE);
	  tile = gimp_drawable_get_tile (drawable, TRUE, *row, *col);
	  gimp_tile_ref (tile);
	}

      pixel = tile->data + tile->bpp * (tile->ewidth * newrowoff + newcoloff);
      dx = xr - x; dy = yr - y;
      rs = dx * dx + dy * dy;
      val = inten * exp (-rs / PSV);

      for (b = 0; b < bytes; b++)
	{
	  if (svals.invers == FALSE)
	      new = pixel[b];
	  else
	      new = 255 - pixel[b];

          if (svals.preserve_luminosity==TRUE)
            {
              if (new < color[b])
                  new *= (1.0 - val * svals.opacity);
              else
                {
                  new -= val * color[b] * svals.opacity;
                  if (new < 0.0)
                      new = 0.0;
                  
                }
            }
          new *= 1.0 - val * (1.0 - svals.opacity);
          new += val * color[b];

          if (new > 255) new = 255;

	  if (svals.invers != FALSE)
	      pixel[b] = 255 - new;
	  else
	      pixel[b] = new;
	}
    }

  return tile;
}

static void
fspike (GPixelRgn *dest_rgn,
	gint       gray,
	gint       x1,
	gint       y1,
	gint       x2,
	gint       y2,
	gdouble    xr,
	gdouble    yr,
	gint       tile_width,
	gint       tile_height,
	gdouble    inten,
	gdouble    length,
	gdouble    angle)
{
  const gdouble efac = 2.0;
  gdouble xrt, yrt, dx, dy;
  gdouble rpos;
  gdouble in;
  gdouble theta;
  gdouble sfac;
  gint r, g, b;
  GTile *tile = NULL;
  gint row, col;
  gint i;
  gint bytes;
  gint x, y;
  gint ok;
  guchar pixel[MAX_CHANNELS];
  guchar color[MAX_CHANNELS];

  theta = angle;
  bytes = dest_rgn->bpp;
  row = -1;
  col = -1;

  /* draw the major spikes */
  for (i = 0; i < svals.spike_pts; i++)
    {
      x = (int) (xr + 0.5);
      y = (int) (yr + 0.5);

      gimp_pixel_rgn_get_pixel (dest_rgn, pixel, x, y);

      switch (svals.colortype)
      {
	case FOREGROUND:
	gimp_palette_get_foreground (&color[0], &color[1], &color[2]);
	break;
	
	case BACKGROUND:
	gimp_palette_get_background (&color[0], &color[1], &color[2]);
	break;
	
	default:
	color[0] = pixel[0];
	color[1] = pixel[1];
	color[2] = pixel[2];
	break;	
      }
      
      if (svals.invers)
	{
	       color[0] = 255 - color[0];
	       color[1] = 255 - color[1];
	       color[2] = 255 - color[2];
        }
      if (svals.random_hue > 0.0 || svals.random_saturation > 0.0)
        {
 	       r = color[0];
 	       g = color[1];
 	       b = color[2];             
               gimp_rgb_to_hls(&r, &g, &b);  
               r+= (svals.random_hue * ((gdouble) rand() / (gdouble) RAND_MAX - 0.5))*255;
               if (r >= 255.0)
		 r -= 255.0;
               else if (r < 0) 
                 r += 255.0;
               b+= (svals.random_saturation * (2 * (gdouble) rand() / (gdouble) RAND_MAX - 1.0))*255;
               if (b > 1.0) b = 1.0;
               gimp_hls_to_rgb(&r, &g, &b);
               color[0] = r;
               color[1] = g;
               color[2] = b;
	}

      dx = 0.2 * cos (theta * G_PI / 180.0);
      dy = 0.2 * sin (theta * G_PI / 180.0);
      xrt = xr;
      yrt = yr;
      rpos = 0.2;

      do
	{
	  sfac = inten * exp (-pow (rpos / length, efac));
	  ok = FALSE;

        in = 0.2 * sfac;
        if (in > 0.01)
            ok = TRUE;

	  tile = rpnt (dest_rgn->drawable, tile, x1, y1, x2, y2, xrt, yrt, tile_width, tile_height, &row, &col, bytes, in, color);
	  tile = rpnt (dest_rgn->drawable, tile, x1, y1, x2, y2, xrt + 1, yrt, tile_width, tile_height, &row, &col, bytes, in, color);
	  tile = rpnt (dest_rgn->drawable, tile, x1, y1, x2, y2, xrt + 1, yrt + 1, tile_width, tile_height, &row, &col, bytes, in, color);
	  tile = rpnt (dest_rgn->drawable, tile, x1, y1, x2, y2, xrt, yrt + 1, tile_width, tile_height, &row, &col, bytes, in, color);

	  xrt += dx;
	  yrt += dy;
	  rpos += 0.2;
	} while (ok);

      theta += 360.0 / svals.spike_pts;
    }

  if (tile)
    gimp_tile_unref (tile, TRUE);
}

/*  Sparkle interface functions  */

static void
sparkle_close_callback (GtkWidget *widget,
			gpointer   data)
{
  gtk_main_quit ();
}

static void
sparkle_ok_callback (GtkWidget *widget,
		     gpointer   data)
{
  sint.run = TRUE;
  gtk_widget_destroy (GTK_WIDGET (data));
}

static void
sparkle_scale_update (GtkAdjustment *adjustment,
		     double        *scale_val)
{
  *scale_val = adjustment->value;
}

static void
sparkle_toggle_update (GtkWidget *widget,
		     gpointer	   data)
{
  int *toggle_val;
  toggle_val = (int *) data;
  if (GTK_TOGGLE_BUTTON (widget)->active)
    *toggle_val = TRUE;
  else
    *toggle_val = FALSE;
}

static void
set_tooltip (GtkTooltips *tooltips, GtkWidget *widget, const char *desc)
{
  if (desc && desc[0])
    gtk_tooltips_set_tip (tooltips, widget, (char *) desc, NULL);
}
