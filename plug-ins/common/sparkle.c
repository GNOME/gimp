/* Sparkle --- image filter plug-in for The Gimp image manipulation program
 * Copyright (C) 1996 by John Beale;  ported to Gimp by Michael J. Hammel
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
 *
 * You can contact Michael at mjhammel@csn.net
 * Note: set tabstops to 3 to make this more readable.
 */

/*
 * Sparkle -  simulate pixel bloom and diffraction effects
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "gtk/gtk.h"
#include "libgimp/gimp.h"

#define SCALE_WIDTH 125
#define TILE_CACHE_SIZE 16
#define MAX_CHANNELS 4
#define PSV 2  /* point spread value */
#define EPSILON 0.001
#define SQR(a) ((a) * (a))

typedef struct
{
  gdouble lum_threshold;
  gdouble flare_inten;
  gdouble spike_len;
  gdouble spike_pts;
  gdouble spike_angle;
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
					gint *     row,
					gint *     col,
					gint       bytes,
					gdouble *  inten);
static void      rgb_to_hsl            (gdouble    r,
					gdouble    g,
					gdouble    b,
					gdouble *  h,
					gdouble *  s,
					gdouble *  l);
static void      hsl_to_rgb            (gdouble    h,
					gdouble    sl,
					gdouble    l,
					gdouble *  r,
					gdouble *  g,
					gdouble *  b);

static void      sparkle_close_callback  (GtkWidget *widget,
					  gpointer   data);
static void      sparkle_ok_callback     (GtkWidget *widget,
					  gpointer   data);
static void      sparkle_scale_update    (GtkAdjustment *adjustment,
					  double        *scale_val);

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
  0.5,		/* flare intensity */
  20.0,		/* spike length */
  6.0,		/* spike points */
  15.0		/* spike angle */
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
    { PARAM_FLOAT, "spike_len", "Spike length (in pixels)" },
    { PARAM_INT32, "spike_pts", "# of spike points" },
    { PARAM_FLOAT, "spike_angle", "Spike angle (0.0-360.0 degrees)" }
  };
  static GParamDef *return_vals = NULL;
  static int nargs = sizeof (args) / sizeof (args[0]);
  static int nreturn_vals = 0;

  gimp_install_procedure ("plug_in_sparkle",
			  "Simulates pixel bloom and diffraction effects",
			  "More here later",
			  "John Beale, & (ported to GIMP v0.54) Michael J. Hammel & (ported to GIMP v1.0) Spencer Kimball",
			  "John Beale",
			  "1996",
			  "<Image>/Filters/Light Effects/Sparkle",
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
  gint threshold;

  run_mode = param[0].data.d_int32;

  *nreturn_vals = 1;
  *return_vals = values;

  values[0].type = PARAM_STATUS;
  values[0].data.d_status = status;

  switch (run_mode)
    {
    case RUN_INTERACTIVE:
      /*  Possibly retrieve data  */
      gimp_get_data ("plug_in_sparkle", &svals);

      /*  First acquire information with a dialog  */
      if (! sparkle_dialog ())
	return;
      break;

    case RUN_NONINTERACTIVE:
      /*  Make sure all the arguments are there!  */
      if (nparams != 8)
	status = STATUS_CALLING_ERROR;
      if (status == STATUS_SUCCESS)
	{
	  svals.lum_threshold = param[3].data.d_float;
	  svals.flare_inten = param[4].data.d_float;
	  svals.spike_len = param[5].data.d_float;
	  svals.spike_pts = param[6].data.d_int32;
	  svals.spike_angle = param[7].data.d_float;
	}
      if (status == STATUS_SUCCESS &&
	  (svals.lum_threshold < 0.0 || svals.lum_threshold > 1.0))
	status = STATUS_CALLING_ERROR;
      if (status == STATUS_SUCCESS &&
	  (svals.flare_inten < 0.0 || svals.flare_inten > 1.0))
	status = STATUS_CALLING_ERROR;
      if (status == STATUS_SUCCESS &&
	  (svals.spike_pts < 0))
	status = STATUS_CALLING_ERROR;
      if (status == STATUS_SUCCESS &&
	  (svals.spike_angle < 0.0 || svals.spike_angle > 360.0))
	status = STATUS_CALLING_ERROR;
      break;

    case RUN_WITH_LAST_VALS:
      /*  Possibly retrieve data  */
      gimp_get_data ("plug_in_sparkle", &svals);
      break;

    default:
      break;
    }

  /*  Get the specified drawable  */
  drawable = gimp_drawable_get (param[2].data.d_drawable);

  /*  Make sure that the drawable is gray or RGB color  */
  if (gimp_drawable_color (drawable->id) || gimp_drawable_gray (drawable->id))
    {
      gimp_progress_init ("Sparkling...");
      gimp_tile_cache_ntiles (TILE_CACHE_SIZE);

      /*  compute the luminosity which exceeds the luminosity threshold  */
      threshold = compute_lum_threshold (drawable, svals.lum_threshold);
      sparkle (drawable, threshold);

      if (run_mode != RUN_NONINTERACTIVE)
	gimp_displays_flush ();

      /*  Store mvals data  */
      if (run_mode == RUN_INTERACTIVE)
	gimp_set_data ("plug_in_sparkle", &svals, sizeof (SparkleVals));
    }
  else
    {
      /* gimp_message ("blur: cannot operate on indexed color images"); */
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
  GtkWidget *button;
  GtkWidget *scale;
  GtkWidget *frame;
  GtkWidget *table;
  GtkObject *scale_data;
  gchar **argv;
  gint argc;

  argc = 1;
  argv = g_new (gchar *, 1);
  argv[0] = g_strdup ("sparkle");

  gtk_init (&argc, &argv);
  gtk_rc_parse (gimp_gtkrc ());

  dlg = gtk_dialog_new ();
  gtk_window_set_title (GTK_WINDOW (dlg), "Sparkle");
  gtk_window_position (GTK_WINDOW (dlg), GTK_WIN_POS_MOUSE);
  gtk_signal_connect (GTK_OBJECT (dlg), "destroy",
		      (GtkSignalFunc) sparkle_close_callback,
		      NULL);

  /*  Action area  */
  button = gtk_button_new_with_label ("OK");
  GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT);
  gtk_signal_connect (GTK_OBJECT (button), "clicked",
                      (GtkSignalFunc) sparkle_ok_callback,
                      dlg);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dlg)->action_area), button, TRUE, TRUE, 0);
  gtk_widget_grab_default (button);
  gtk_widget_show (button);

  button = gtk_button_new_with_label ("Cancel");
  GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT);
  gtk_signal_connect_object (GTK_OBJECT (button), "clicked",
			     (GtkSignalFunc) gtk_widget_destroy,
			     GTK_OBJECT (dlg));
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dlg)->action_area), button, TRUE, TRUE, 0);
  gtk_widget_show (button);

  /*  parameter settings  */
  frame = gtk_frame_new ("Parameter Settings");
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_ETCHED_IN);
  gtk_container_border_width (GTK_CONTAINER (frame), 10);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dlg)->vbox), frame, TRUE, TRUE, 0);
  table = gtk_table_new (5, 2, FALSE);
  gtk_container_border_width (GTK_CONTAINER (table), 10);
  gtk_container_add (GTK_CONTAINER (frame), table);

  label = gtk_label_new ("Luminosity Threshold");
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gtk_table_attach (GTK_TABLE (table), label, 0, 1, 0, 1, GTK_FILL, 0, 5, 0);
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

  label = gtk_label_new ("Flare Intensity");
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gtk_table_attach (GTK_TABLE (table), label, 0, 1, 1, 2, GTK_FILL, 0, 5, 0);
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

  label = gtk_label_new ("Spike Length");
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gtk_table_attach (GTK_TABLE (table), label, 0, 1, 2, 3, GTK_FILL, 0, 5, 0);
  scale_data = gtk_adjustment_new (svals.spike_len, 1.0, 100.0, 1.0, 1.0, 0.0);
  scale = gtk_hscale_new (GTK_ADJUSTMENT (scale_data));
  gtk_widget_set_usize (scale, SCALE_WIDTH, 0);
  gtk_table_attach (GTK_TABLE (table), scale, 1, 2, 2, 3, GTK_FILL, 0, 0, 0);
  gtk_scale_set_value_pos (GTK_SCALE (scale), GTK_POS_TOP);
  gtk_range_set_update_policy (GTK_RANGE (scale), GTK_UPDATE_DELAYED);
  gtk_signal_connect (GTK_OBJECT (scale_data), "value_changed",
		      (GtkSignalFunc) sparkle_scale_update,
		      &svals.spike_len);
  gtk_widget_show (label);
  gtk_widget_show (scale);

  label = gtk_label_new ("Spike Points");
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gtk_table_attach (GTK_TABLE (table), label, 0, 1, 3, 4, GTK_FILL, 0, 5, 0);
  scale_data = gtk_adjustment_new (svals.spike_pts, 0.0, 16.0, 1.0, 1.0, 0.0);
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

  label = gtk_label_new ("Spike Angle");
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gtk_table_attach (GTK_TABLE (table), label, 0, 1, 4, 5, GTK_FILL, 0, 5, 0);
  scale_data = gtk_adjustment_new (svals.spike_angle, 0.0, 360.0, 5.0, 5.0, 0.0);
  scale = gtk_hscale_new (GTK_ADJUSTMENT (scale_data));
  gtk_widget_set_usize (scale, SCALE_WIDTH, 0);
  gtk_table_attach (GTK_TABLE (table), scale, 1, 2, 4, 5, GTK_FILL, 0, 0, 0);
  gtk_scale_set_value_pos (GTK_SCALE (scale), GTK_POS_TOP);
  gtk_range_set_update_policy (GTK_RANGE (scale), GTK_UPDATE_DELAYED);
  gtk_signal_connect (GTK_OBJECT (scale_data), "value_changed",
		      (GtkSignalFunc) sparkle_scale_update,
		      &svals.spike_angle);
  gtk_widget_show (label);
  gtk_widget_show (scale);

  gtk_widget_show (frame);
  gtk_widget_show (table);
  gtk_widget_show (dlg);

  gtk_main ();
  gdk_flush ();

  return sint.run;
}

static gint
compute_luminosity (guchar *pixel,
		    gint    gray,
		    gint    has_alpha)
{
  if (gray)
    {
      if (has_alpha)
	return (pixel[0] * pixel[1]) / 255;
      else
	return pixel[0];
    }
  else
    {
      gint min, max;

      min = MIN (pixel[0], pixel[1]);
      min = MIN (min, pixel[2]);
      max = MAX (pixel[0], pixel[1]);
      max = MAX (max, pixel[2]);

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
  for (i = 0; i < 256; i++)
    values[i] = 0;

  gimp_drawable_mask_bounds (drawable->id, &x1, &y1, &x2, &y2);
  gray = gimp_drawable_gray (drawable->id);
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
  gdouble nfrac, length, inten;
  gint cur_progress, max_progress;
  gint x1, y1, x2, y2;
  gint size, lum, x, y, b;
  gint gray;
  gint has_alpha, alpha;
  gpointer pr;

  gimp_drawable_mask_bounds (drawable->id, &x1, &y1, &x2, &y2);
  gray = gimp_drawable_gray (drawable->id);
  has_alpha = gimp_drawable_has_alpha (drawable->id);
  alpha = (has_alpha) ? drawable->bpp - 1 : drawable->bpp;

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
	  for (b = 0; b < alpha; b++)
	    {
	      if (has_alpha && src[alpha] == 0)
		*dest++ = 0;
	      else
		*dest++ = src[b];
	    }

	  if (has_alpha)
	    *dest++ = src[alpha];

	  src += src_rgn.bpp;
	}
    }

  /* add effects to new image based on intensity of old pixels */
  gimp_pixel_rgn_init (&src_rgn, drawable, x1, y1, (x2 - x1), (y2 - y1), FALSE, FALSE);
  gimp_pixel_rgn_init (&dest_rgn, drawable, x1, y1, (x2 - x1), (y2 - y1), TRUE, TRUE);

  for (pr = gimp_pixel_rgns_register (1, &src_rgn); pr != NULL; pr = gimp_pixel_rgns_process (pr))
    {
      src = src_rgn.data;

      for (y = 0; y < src_rgn.h; y++)
	for (x = 0; x < src_rgn.w; x++)
	  {
	    lum = compute_luminosity (src, gray, has_alpha);
	    if (lum >= threshold)
	      {
		nfrac = fabs ((gdouble) (lum + 1 - threshold) / (gdouble) (256 - threshold));

		length = svals.spike_len * pow (nfrac, 0.8);
		inten = svals.flare_inten * pow (nfrac, 1.0);

		/* fspike im x,y intens rlength angle */
		if (svals.spike_pts > 0)
		  {
		    /* major spikes */
		    fspike (&dest_rgn, gray, x1, y1, x2, y2,
			    x + src_rgn.x, y + src_rgn.y,
			    inten, length, svals.spike_angle);

		    /* minor spikes */
		    fspike (&dest_rgn, gray, x1, y1, x2, y2,
			    x + src_rgn.x, y + src_rgn.y,
			    inten * 0.7, length * 0.7,
			    (svals.spike_angle + 180.0 / svals.spike_pts));
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

static void
fspike (GPixelRgn *dest_rgn,
	gint       gray,
	gint       x1,
	gint       y1,
	gint       x2,
	gint       y2,
	gdouble    xr,
	gdouble    yr,
	gdouble    inten,
	gdouble    length,
	gdouble    angle)
{
  gdouble xrt, yrt, dx, dy;
  gdouble rpos;
  gdouble in[MAX_CHANNELS];
  gdouble val[MAX_CHANNELS];
  gdouble ho, so, vo;
  gdouble theta, efac;
  gdouble sfac;
  GTile *tile = NULL;
  gint row, col;
  gint i;
  gint bytes;
  gint x, y;
  gint ok;
  gint b;
  guchar pixel[MAX_CHANNELS];

  theta = angle;
  efac = 2.0;    /* exponent on intensity falloff with radius */
  bytes = dest_rgn->bpp;
  row = -1;
  col = -1;

  /* draw the major spikes */
  for (i = 0; i < svals.spike_pts; i++)
    {
      x = (int) (xr + 0.5);
      y = (int) (yr + 0.5);

      gimp_pixel_rgn_get_pixel (dest_rgn, pixel, x, y);

      for (b = 0; b < bytes; b++)
	val[b] = (gdouble) pixel[b] / 255.0;

      /*  increase saturation to full for color image  */
      if (! gray)
	{
	  rgb_to_hsl (val[0], val[1], val[2], &ho, &so, &vo);
	  hsl_to_rgb (ho, 1.0, vo, &val[0], &val[1], &val[2]);
	}

      dx = 0.2 * cos (theta * M_PI / 180.0);
      dy = 0.2 * sin (theta * M_PI / 180.0);
      xrt = xr;
      yrt = yr;
      rpos = 0.2;

      do
	{
	  sfac = exp (-pow (rpos / length, efac));
	  sfac = sfac * inten;

	  ok = FALSE;
	  for (b = 0; b < bytes; b++)
	    {
	      in[b] = 0.2 * val[b] * sfac;
	      if (in[b] > 0.01)
		ok = TRUE;
	    }

	  tile = rpnt (dest_rgn->drawable, tile, x1, y1, x2, y2, xrt, yrt, &row, &col, bytes, in);
	  tile = rpnt (dest_rgn->drawable, tile, x1, y1, x2, y2, xrt + 1, yrt, &row, &col, bytes, in);
	  tile = rpnt (dest_rgn->drawable, tile, x1, y1, x2, y2, xrt + 1, yrt + 1, &row, &col, bytes, in);
	  tile = rpnt (dest_rgn->drawable, tile, x1, y1, x2, y2, xrt, yrt + 1, &row, &col, bytes, in);

	  xrt += dx;
	  yrt += dy;
	  rpos += 0.2;
	}
      while (ok);

      theta += 360.0 / svals.spike_pts;
    }

  if (tile)
    gimp_tile_unref (tile, TRUE);
}

static GTile *
rpnt (GDrawable *drawable,
      GTile     *tile,
      gint       x1,
      gint       y1,
      gint       x2,
      gint       y2,
      gdouble    xr,
      gdouble    yr,
      gint      *row,
      gint      *col,
      gint       bytes,
      gdouble   *inten)
{
  gint x, y, b;
  gdouble dx, dy, rs, fac;
  gdouble val;
  guchar *pixel;

  x = (int) (xr);	/* integer coord. to upper left of real point */
  y = (int) (yr);

  if (x >= x1 && y >= y1 && x < x2 && y < y2)
    {
      if ((x >> 6 != *col) || (y >> 6 != *row))
	{
	  *col = x / 64;
	  *row = y / 64;
	  if (tile)
	    gimp_tile_unref (tile, TRUE);
	  tile = gimp_drawable_get_tile (drawable, TRUE, *row, *col);
	  gimp_tile_ref (tile);
	}
      pixel = tile->data + tile->bpp * (tile->ewidth * (y % 64) + (x % 64));

      dx = xr - x; dy = yr - y;
      rs = dx * dx + dy * dy;
      fac = exp (-rs / PSV);

      for (b = 0; b < bytes; b++)
	{
	  val = inten[b] * fac;
	  val += (gdouble) pixel[b] / 255.0;
	  if (val > 1.0) val = 1.0;
	  pixel[b] = (guchar) (val * 255);
	}
    }

  return tile;
}


/*
 * RGB-HSL transforms.
 * Ken Fishkin, Pixar Inc., January 1989.
 */

/*
 * given r,g,b on [0 ... 1],
 * return (h,s,l) on [0 ... 1]
 */

static void
rgb_to_hsl (gdouble  r,
	    gdouble  g,
	    gdouble  b,
	    gdouble *h,
	    gdouble *s,
	    gdouble *l)
{
  gdouble v;
  gdouble m;
  gdouble vm;
  gdouble r2, g2, b2;

  v = MAX(r,g);
  v = MAX(v,b);
  m = MIN(r,g);
  m = MIN(m,b);

  if ((*l = (m + v) / 2.0) <= 0.0)
    return;
  if ((*s = vm = v - m) > 0.0)
    {
      *s /= (*l <= 0.5) ? (v + m ) :
	(2.0 - v - m) ;
    }
  else
    return;

  r2 = (v - r) / vm;
  g2 = (v - g) / vm;
  b2 = (v - b) / vm;

  if (r == v)
    *h = (g == m ? 5.0 + b2 : 1.0 - g2);
  else if (g == v)
    *h = (b == m ? 1.0 + r2 : 3.0 - b2);
  else
    *h = (r == m ? 3.0 + g2 : 5.0 - r2);

  *h /= 6;
}

/*
 * given h,s,l on [0..1],
 * return r,g,b on [0..1]
 */

static void
hsl_to_rgb (gdouble  h,
	    gdouble  sl,
	    gdouble  l,
	    gdouble *r,
	    gdouble *g,
	    gdouble *b)
{
  gdouble v;

  v = (l <= 0.5) ? (l * (1.0 + sl)) : (l + sl - l * sl);
  if (v <= 0)
    {
      *r = *g = *b = 0.0;
    }
  else
    {
      gdouble m;
      gdouble sv;
      gint sextant;
      gdouble fract, vsf, mid1, mid2;

      m = l + l - v;
      sv = (v - m ) / v;
      h *= 6.0;
      sextant = h;
      fract = h - sextant;
      vsf = v * sv * fract;
      mid1 = m + vsf;
      mid2 = v - vsf;
      switch (sextant)
	{
	case 0: *r = v; *g = mid1; *b = m; break;
	case 1: *r = mid2; *g = v; *b = m; break;
	case 2: *r = m; *g = v; *b = mid1; break;
	case 3: *r = m; *g = mid2; *b = v; break;
	case 4: *r = mid1; *g = m; *b = v; break;
	case 5: *r = v; *g = m; *b = mid2; break;
	}
    }
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
