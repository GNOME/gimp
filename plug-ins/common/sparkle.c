
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
 * You can contact Martin at martweb@gmx.net
 * Note: set tabstops to 3 to make this more readable.
 */

/*
 * Sparkle 1.26 - simulate pixel bloom and diffraction effects
 */

#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <gtk/gtk.h>

#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

#include "libgimp/stdplugins-intl.h"

#define SCALE_WIDTH  175
#define MAX_CHANNELS   4
#define PSV            2  /* point spread value */
#define EPSILON        0.001

#define  NATURAL    0
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
static void      run    (gchar   *name,
			 gint     nparams,
			 GParam  *param,
			 gint    *nreturn_vals,
			 GParam **return_vals);

static gint      sparkle_dialog        (void);
static void      sparkle_ok_callback   (GtkWidget *widget,
					gpointer   data);

static gint      compute_luminosity    (guchar *   pixel,
					gint       gray,
					gint       has_alpha);
static gint      compute_lum_threshold (GDrawable * drawable,
					gdouble    percentile);
static void      sparkle               (GDrawable * drawable,
					gint       threshold);
static void      fspike                (GPixelRgn * src_rgn,
					GPixelRgn * dest_rgn,
					gint       gray,
					gint       x1,
					gint       y1,
					gint       x2,
					gint       y2,
					gint       xr,
					gint       yr,
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

GPlugInInfo PLUG_IN_INFO =
{
  NULL,  /* init_proc  */
  NULL,  /* quit_proc  */
  query, /* query_proc */
  run,   /* run_proc   */
};

static SparkleVals svals =
{
  0.001,  /* luminosity threshold */
  0.5,	  /* flare intensity */
  20.0,	  /* spike length */
  4.0,    /* spike points */
  15.0,	  /* spike angle */
  1.0,    /* spike density */
  0.0,    /* opacity */
  0.0,    /* random hue */
  0.0,    /* random saturation */
  FALSE,  /* preserve_luminosity */
  FALSE,  /* invers */
  FALSE,  /* border */
  NATURAL /* colortype */
};

static SparkleInterface sint =
{
  FALSE          /* run */
};

static gint num_sparkles;


MAIN ()

static void
query (void)
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
  static gint nargs = sizeof (args) / sizeof (args[0]);

  INIT_I18N();

  gimp_install_procedure ("plug_in_sparkle",
			  "Simulates pixel bloom and diffraction effects",
			  "No help yet",
			  "John Beale, & (ported to GIMP v0.54) Michael J. Hammel & ted to GIMP v1.0) Spencer Kimball",
			  "John Beale",
			  "Version 1.26, December 1998",
			  N_("<Image>/Filters/Light Effects/Sparkle..."),
			  "RGB*, GRAY*",
			  PROC_PLUG_IN,
			  nargs, 0,
			  args, NULL);
}

static void
run (gchar   *name,
     gint     nparams,
     GParam  *param,
     gint    *nreturn_vals,
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
	{
	  status = STATUS_CALLING_ERROR;
	}
      else
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

	  if (svals.lum_threshold < 0.0 || svals.lum_threshold > 1.0)
	    status = STATUS_CALLING_ERROR;
	  else if (svals.flare_inten < 0.0 || svals.flare_inten > 1.0)
	    status = STATUS_CALLING_ERROR;
	  else if (svals.spike_len < 0)
	    status = STATUS_CALLING_ERROR;
	  else if (svals.spike_pts < 0)
	    status = STATUS_CALLING_ERROR;
	  else if (svals.spike_angle < -1 || svals.spike_angle > 360)
	    status = STATUS_CALLING_ERROR;
	  else if (svals.density < 0.0 || svals.density > 1.0)
	    status = STATUS_CALLING_ERROR;
	  else if (svals.opacity < 0.0 || svals.opacity > 1.0)
	    status = STATUS_CALLING_ERROR;
	  else if (svals.random_hue < 0.0 || svals.random_hue > 1.0)
	    status = STATUS_CALLING_ERROR;
	  else if (svals.random_saturation < 0.0 ||
		   svals.random_saturation > 1.0)
	    status = STATUS_CALLING_ERROR;
	  else if (svals.colortype < NATURAL || svals.colortype > BACKGROUND)
	    status = STATUS_CALLING_ERROR;
	}
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
      gimp_progress_init (_("Sparkling..."));
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
sparkle_dialog (void)
{
  GtkWidget *dlg;
  GtkWidget *main_vbox;
  GtkWidget *vbox;
  GtkWidget *hbox;
  GtkWidget *frame;
  GtkWidget *table;
  GtkWidget *toggle;
  GtkWidget *sep;
  GtkWidget *r1, *r2, *r3;
  GtkObject *scale_data;
  gchar **argv;
  gint    argc;

  argc    = 1;
  argv    = g_new (gchar *, 1);
  argv[0] = g_strdup ("sparkle");

  gtk_init (&argc, &argv);
  gtk_rc_parse (gimp_gtkrc ());

  dlg = gimp_dialog_new (_("Sparkle"), "sparkle",
			 gimp_plugin_help_func, "filters/sparkle.html",
			 GTK_WIN_POS_MOUSE,
			 FALSE, TRUE, FALSE,

			 _("OK"), sparkle_ok_callback,
			 NULL, NULL, NULL, TRUE, FALSE,
			 _("Cancel"), gtk_widget_destroy,
			 NULL, 1, NULL, FALSE, TRUE,

			 NULL);

  gtk_signal_connect (GTK_OBJECT (dlg), "destroy",
		      GTK_SIGNAL_FUNC (gtk_main_quit),
		      NULL);

  gimp_help_init ();

  /*  parameter settings  */
  frame = gtk_frame_new (_("Parameter Settings"));
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_ETCHED_IN);
  gtk_container_border_width (GTK_CONTAINER (frame), 6);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dlg)->vbox), frame, TRUE, TRUE, 0);
  gtk_widget_show (frame);

  main_vbox = gtk_vbox_new (FALSE, 4);
  gtk_container_set_border_width (GTK_CONTAINER (main_vbox), 4);
  gtk_container_add (GTK_CONTAINER (frame), main_vbox);
  gtk_widget_show (main_vbox);

  table = gtk_table_new (9, 3, FALSE);
  gtk_table_set_col_spacings (GTK_TABLE (table), 4);
  gtk_table_set_row_spacings (GTK_TABLE (table), 2);
  gtk_box_pack_start (GTK_BOX (main_vbox), table, FALSE, FALSE, 0);
  gtk_widget_show (table);

  scale_data =
    gimp_scale_entry_new (GTK_TABLE (table), 0, 0,
			  _("Luminosity Threshold:"), SCALE_WIDTH, 0,
			  svals.lum_threshold, 0.0, 0.1, 0.001, 0.01, 3,
			  TRUE, 0, 0,
			  _("Adjust the Luminosity Threshold"), NULL);
  gtk_signal_connect (GTK_OBJECT (scale_data), "value_changed",
		      GTK_SIGNAL_FUNC (gimp_double_adjustment_update),
		      &svals.lum_threshold);

  scale_data =
    gimp_scale_entry_new (GTK_TABLE (table), 0, 1,
			  _("Flare Intensity:"), SCALE_WIDTH, 0,
			  svals.flare_inten, 0.0, 1.0, 0.01, 0.1, 2,
			  TRUE, 0, 0,
			  _("Adjust the Flare Intensity"), NULL);
  gtk_signal_connect (GTK_OBJECT (scale_data), "value_changed",
		      GTK_SIGNAL_FUNC (gimp_double_adjustment_update),
		      &svals.flare_inten);

  scale_data =
    gimp_scale_entry_new (GTK_TABLE (table), 0, 2,
			  _("Spike Length:"), SCALE_WIDTH, 0,
			  svals.spike_len, 1, 100, 1, 10, 0,
			  TRUE, 0, 0,
			  _("Adjust the Spike Length"), NULL);
  gtk_signal_connect (GTK_OBJECT (scale_data), "value_changed",
		      GTK_SIGNAL_FUNC (gimp_double_adjustment_update),
		      &svals.spike_len);

  scale_data =
    gimp_scale_entry_new (GTK_TABLE (table), 0, 3,
			  _("Spike Points:"), SCALE_WIDTH, 0,
			  svals.spike_pts, 0, 16, 1, 4, 0,
			  TRUE, 0, 0,
			  _("Adjust the Number of Spikes"), NULL);
  gtk_signal_connect (GTK_OBJECT (scale_data), "value_changed",
		      GTK_SIGNAL_FUNC (gimp_double_adjustment_update),
		      &svals.spike_pts);

  scale_data =
    gimp_scale_entry_new (GTK_TABLE (table), 0, 4,
			  _("Spike Angle (-1: Random):"), SCALE_WIDTH, 0,
			  svals.spike_angle, -1, 360, 1, 15, 0,
			  TRUE, 0, 0,
			  _("Adjust the Spike Angle "
			    "(-1 means a Random Angle is choosen)"), NULL);
  gtk_signal_connect (GTK_OBJECT (scale_data), "value_changed",
		      GTK_SIGNAL_FUNC (gimp_double_adjustment_update),
		      &svals.spike_angle);

  scale_data =
    gimp_scale_entry_new (GTK_TABLE (table), 0, 5,
			  _("Spike Density:"), SCALE_WIDTH, 0,
			  svals.density, 0.0, 1.0, 0.01, 0.1, 2,
			  TRUE, 0, 0,
			  _("Adjust the Spike Density"), NULL);
  gtk_signal_connect (GTK_OBJECT (scale_data), "value_changed",
		      GTK_SIGNAL_FUNC (gimp_double_adjustment_update),
		      &svals.density);

  scale_data =
    gimp_scale_entry_new (GTK_TABLE (table), 0, 6,
			  _("Opacity:"), SCALE_WIDTH, 0,
			  svals.opacity, 0.0, 1.0, 0.01, 0.1, 2,
			  TRUE, 0, 0,
			  _("Adjust the Opacity of the Spikes"), NULL);
  gtk_signal_connect (GTK_OBJECT (scale_data), "value_changed",
		      GTK_SIGNAL_FUNC (gimp_double_adjustment_update),
		      &svals.opacity);

  scale_data =
    gimp_scale_entry_new (GTK_TABLE (table), 0, 7,
			  _("Random Hue:"), SCALE_WIDTH, 0,
			  svals.random_hue, 0.0, 1.0, 0.01, 0.1, 2,
			  TRUE, 0, 0,
			  _("Adjust the Value how much the Hue should "
			    "be changed randomly"), NULL);
  gtk_signal_connect (GTK_OBJECT (scale_data), "value_changed",
		      GTK_SIGNAL_FUNC (gimp_double_adjustment_update),
		      &svals.random_hue);

  scale_data =
    gimp_scale_entry_new (GTK_TABLE (table), 0, 8,
			  _("Random Saturation:"), SCALE_WIDTH, 0,
			  svals.random_saturation, 0.0, 1.0, 0.01, 0.1, 2,
			  TRUE, 0, 0,
			  _("Adjust the Value how much the Saturation should "
			    "be changed randomly"), NULL);
  gtk_signal_connect (GTK_OBJECT (scale_data), "value_changed",
		      GTK_SIGNAL_FUNC (gimp_double_adjustment_update),
		      &svals.random_saturation);

  sep = gtk_hseparator_new ();
  gtk_box_pack_start (GTK_BOX (main_vbox), sep, FALSE, FALSE, 0);
  gtk_widget_show (sep);

  hbox = gtk_hbox_new (FALSE, 4);
  gtk_box_pack_start (GTK_BOX (main_vbox), hbox, FALSE, FALSE, 0);
  gtk_widget_show (hbox);

  vbox = gtk_vbox_new (FALSE, 1);
  gtk_box_pack_start (GTK_BOX (hbox), vbox, TRUE, TRUE, 0);
  gtk_widget_show (vbox);

  toggle = gtk_check_button_new_with_label (_("Preserve Luminosity"));
  gtk_box_pack_start (GTK_BOX (vbox), toggle, FALSE, FALSE, 0);
  gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (toggle),
			       svals.preserve_luminosity);
  gtk_signal_connect (GTK_OBJECT (toggle), "toggled",
		      GTK_SIGNAL_FUNC (gimp_toggle_button_update),
		      &svals.preserve_luminosity);
  gtk_widget_show (toggle);
  gimp_help_set_help_data (toggle, _("Should the Luminosity be preserved?"),
			   NULL);

  toggle = gtk_check_button_new_with_label (_("Inverse"));
  gtk_box_pack_start (GTK_BOX (vbox), toggle, FALSE, FALSE, 0);
  gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (toggle), svals.invers);
  gtk_signal_connect (GTK_OBJECT (toggle), "toggled",
		      GTK_SIGNAL_FUNC (gimp_toggle_button_update),
		      &svals.invers);
  gtk_widget_show (toggle);
  gimp_help_set_help_data (toggle, _("Should an Inverse Effect be done?"), NULL);

  toggle = gtk_check_button_new_with_label (_("Add Border"));
  gtk_box_pack_start (GTK_BOX (vbox), toggle, FALSE, FALSE, 0);
  gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (toggle), svals.border);
  gtk_signal_connect (GTK_OBJECT (toggle), "toggled",
		      GTK_SIGNAL_FUNC (gimp_toggle_button_update),
		      &svals.border);
  gtk_widget_show (toggle);
  gimp_help_set_help_data (toggle,
			   _("Draw a Border of Spikes around the Image"),
			   NULL);

  sep = gtk_vseparator_new ();
  gtk_box_pack_start (GTK_BOX (hbox), sep, FALSE, FALSE, 0);
  gtk_widget_show (sep);

  /*  colortype  */
  vbox =
    gimp_radio_group_new2 (FALSE, NULL,
			   gimp_radio_button_update,
			   &svals.colortype, (gpointer) svals.colortype,

			   _("Natural Color"),    (gpointer) NATURAL, &r1,
			   _("Foreground Color"), (gpointer) FOREGROUND, &r2,
			   _("Background Color"), (gpointer) BACKGROUND, &r3,

			   NULL);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), 0);
  gtk_box_pack_start (GTK_BOX (hbox), vbox, TRUE, TRUE, 0);
  gtk_widget_show (vbox);

  gimp_help_set_help_data (r1, _("Use the Color of the Image"), NULL);
  gimp_help_set_help_data (r2, _("Use the Foreground Color"), NULL);
  gimp_help_set_help_data (r3, _("Use the Background Color"), NULL);

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

  memset (values, 0, sizeof (gint) * 256);

  gimp_drawable_mask_bounds (drawable->id, &x1, &y1, &x2, &y2);
  gray = gimp_drawable_is_gray (drawable->id);
  has_alpha = gimp_drawable_has_alpha (drawable->id);

  gimp_pixel_rgn_init (&src_rgn, drawable,
		       x1, y1, (x2 - x1), (y2 - y1), FALSE, FALSE);

  for (pr = gimp_pixel_rgns_register (1, &src_rgn);
       pr != NULL;
       pr = gimp_pixel_rgns_process (pr))
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
      if ((gdouble) sum > percentile * (gdouble) total)
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

  gimp_pixel_rgn_init (&src_rgn, drawable,
		       x1, y1, (x2 - x1), (y2 - y1), FALSE, FALSE);
  gimp_pixel_rgn_init (&dest_rgn, drawable,
		       x1, y1, (x2 - x1), (y2 - y1), TRUE, TRUE);

  for (pr = gimp_pixel_rgns_register (2, &src_rgn, &dest_rgn);
       pr != NULL;
       pr = gimp_pixel_rgns_process (pr))
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
  gimp_pixel_rgn_init (&src_rgn, drawable,
		       x1, y1, (x2 - x1), (y2 - y1), FALSE, FALSE);
  gimp_pixel_rgn_init (&dest_rgn, drawable,
		       x1, y1, (x2 - x1), (y2 - y1), TRUE, TRUE);

  for (pr = gimp_pixel_rgns_register (2, &src_rgn, &dest_rgn);
       pr != NULL;
       pr = gimp_pixel_rgns_process (pr))
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
		nfrac = fabs ((gdouble) (lum + 1 - threshold) /
			      (gdouble) (256 - threshold));
		length = (gdouble) svals.spike_len * (gdouble) pow (nfrac, 0.8);
		inten = svals.flare_inten * nfrac /* pow (nfrac, 1.0) */;

		/* fspike im x,y intens rlength angle */
		if (svals.spike_pts > 0)
		  {
		    /* major spikes */
		    if (svals.spike_angle == -1)
		   	spike_angle = 360.0 * rand () / G_MAXRAND;
		    else
			spike_angle = svals.spike_angle;
		    if (rand() <= G_MAXRAND * svals.density)
		      {
			fspike (&src_rgn, &dest_rgn, gray, x1, y1, x2, y2,
			    x + src_rgn.x, y + src_rgn.y,
			    tile_width, tile_height,
			    inten, length, spike_angle);
		        /* minor spikes */
			fspike (&src_rgn, &dest_rgn, gray, x1, y1, x2, y2,
			    x + src_rgn.x, y + src_rgn.y,
			    tile_width, tile_height,
			    inten * 0.7, length * 0.7,
			    ((gdouble) spike_angle + 180.0 / svals.spike_pts));
		      }
		  }
		cur_progress ++;
		if ((cur_progress % 5) == 0)
		  gimp_progress_update ((double) cur_progress /
					(double) max_progress);
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
		  new *= (1.0 - val * (1.0 - svals.opacity));
	      else
		{
		  new -= val * color[b] * (1.0 - svals.opacity);
		  if (new < 0.0)
		      new = 0.0;

		}
	    }
	  new *= 1.0 - val * svals.opacity;
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
fspike (GPixelRgn *src_rgn,
	GPixelRgn *dest_rgn,
	gint       gray,
	gint       x1,
	gint       y1,
	gint       x2,
	gint       y2,
	gint       xr,
	gint       yr,
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
      gimp_pixel_rgn_get_pixel (dest_rgn, pixel, xr, yr);

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
	  r = 255 - color[0];
	  g = 255 - color[1];
	  b = 255 - color[2];             
	  gimp_rgb_to_hsv (&r, &g, &b);  
	  r+= (svals.random_hue * ((gdouble) rand() / (gdouble) RAND_MAX - 0.5))*255;
	  if (r >= 255)
	    r -= 255;
	  else if (r < 0) 
	    r += 255;
	  b+= (svals.random_saturation * (2.0 * (gdouble) rand() /
					  (gdouble) RAND_MAX - 1.0))*255;
	  if (b > 255) b = 255;
	  gimp_hsv_to_rgb (&r, &g, &b);
	  color[0] = 255 - r;
	  color[1] = 255 - g;
	  color[2] = 255 - b;
	}

      dx = 0.2 * cos (theta * G_PI / 180.0);
      dy = 0.2 * sin (theta * G_PI / 180.0);
      xrt = (gdouble) xr; /* (gdouble) is needed because some */
      yrt = (gdouble) yr; /* compilers optimize too much otherwise */
      rpos = 0.2;

      do
	{
	  sfac = inten * exp (-pow (rpos / length, efac));
	  ok = FALSE;

        in = 0.2 * sfac;
        if (in > 0.01)
            ok = TRUE;

	  tile = rpnt (dest_rgn->drawable, tile, x1, y1, x2, y2, xrt, yrt, tile_width, tile_height, &row, &col, bytes, in, color);
	  tile = rpnt (dest_rgn->drawable, tile, x1, y1, x2, y2, xrt + 1.0, yrt, tile_width, tile_height, &row, &col, bytes, in, color);
	  tile = rpnt (dest_rgn->drawable, tile, x1, y1, x2, y2, xrt + 1.0, yrt + 1.0, tile_width, tile_height, &row, &col, bytes, in, color);
	  tile = rpnt (dest_rgn->drawable, tile, x1, y1, x2, y2, xrt, yrt + 1.0, tile_width, tile_height, &row, &col, bytes, in, color);

	  xrt += dx;
	  yrt += dy;
	  rpos += 0.2;
	} while (ok);

      theta += 360.0 / svals.spike_pts;
    }

  if (tile)
    gimp_tile_unref (tile, TRUE);
}

static void
sparkle_ok_callback (GtkWidget *widget,
		     gpointer   data)
{
  sint.run = TRUE;

  gtk_widget_destroy (GTK_WIDGET (data));
}
