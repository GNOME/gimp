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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <gtk/gtk.h>

#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

#include "libgimp/stdplugins-intl.h"

#define ENTRY_WIDTH 100

typedef struct
{
  gdouble radius;
  gint    horizontal;
  gint    vertical;
} BlurValues;

typedef struct
{
  gint run;
} BlurInterface;


/* Declare local functions.
 */
static void      query  (void);
static void      run    (gchar     *name,
			 gint       nparams,
			 GParam     *param,
			 gint      *nreturn_vals,
			 GParam   **return_vals);

static void      gauss_iir        (GDrawable *drawable,
				   gint       horizontal,
				   gint       vertical,
				   gdouble    std_dev);

/*
 * Gaussian blur interface
 */
static gint      gauss_iir_dialog (void);

/*
 * Gaussian blur helper functions
 */
static void      find_constants   (gdouble n_p[],
				   gdouble n_m[],
				   gdouble d_p[],
				   gdouble d_m[],
				   gdouble bd_p[],
				   gdouble bd_m[],
				   gdouble std_dev);
static void      transfer_pixels  (gdouble * src1,
				   gdouble * src2,
				   guchar *  dest,
				   gint      bytes,
				   gint      width);

static void      gauss_ok_callback     (GtkWidget *widget,
					gpointer   data);
static void      gauss_entry_callback  (GtkWidget *widget,
					gpointer   data);
GPlugInInfo PLUG_IN_INFO =
{
  NULL,  /* init_proc  */
  NULL,  /* quit_proc  */
  query, /* query_proc */
  run,   /* run_proc   */
};

static BlurValues bvals =
{
  5.0,  /*  radius           */
  TRUE, /*  horizontal blur  */
  TRUE  /*  vertical blur    */
};

static BlurInterface bint =
{
  FALSE  /*  run  */
};


MAIN ()

static void
query (void)
{
  static GParamDef args[] =
  {
    { PARAM_INT32, "run_mode", "Interactive, non-interactive" },
    { PARAM_IMAGE, "image", "Input image (unused)" },
    { PARAM_DRAWABLE, "drawable", "Input drawable" },
    { PARAM_FLOAT, "radius", "Radius of gaussian blur (in pixels > 1.0)" },
    { PARAM_INT32, "horizontal", "Blur in horizontal direction" },
    { PARAM_INT32, "vertical", "Blur in vertical direction" },
  };
  static GParamDef *return_vals = NULL;
  static gint nargs = sizeof (args) / sizeof (args[0]);
  static gint nreturn_vals = 0;

  INIT_I18N();
  
  gimp_install_procedure ("plug_in_gauss_iir",
			  _("Applies a gaussian blur to the specified drawable."),
			  _("Applies a gaussian blur to the drawable, with specified radius of affect.  The standard deviation of the normal distribution used to modify pixel values is calculated based on the supplied radius.  Horizontal and vertical blurring can be independently invoked by specifying only one to run.  The IIR gaussian blurring works best for large radius values and for images which are not computer-generated.  Values for radius less than 1.0 are invalid as they will generate spurious results."),
			  "Spencer Kimball & Peter Mattis",
			  "Spencer Kimball & Peter Mattis",
			  "1995-1996",
			  N_("<Image>/Filters/Blur/Gaussian Blur (IIR)..."),
			  "RGB*, GRAY*",
			  PROC_PLUG_IN,
			  nargs, nreturn_vals,
			  args, return_vals);
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
  gdouble radius, std_dev;

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
      gimp_get_data ("plug_in_gauss_iir", &bvals);

      /*  First acquire information with a dialog  */
      if (! gauss_iir_dialog ())
	return;
      break;

    case RUN_NONINTERACTIVE:
      /*  Make sure all the arguments are there!  */
      if (nparams != 6)
	status = STATUS_CALLING_ERROR;
      if (status == STATUS_SUCCESS)
	{
	  bvals.radius     = param[3].data.d_float;
	  bvals.horizontal = (param[4].data.d_int32) ? TRUE : FALSE;
	  bvals.vertical   = (param[5].data.d_int32) ? TRUE : FALSE;
	}
      if (status == STATUS_SUCCESS && (bvals.radius < 1.0))
	status = STATUS_CALLING_ERROR;
      INIT_I18N();
      break;

    case RUN_WITH_LAST_VALS:
      INIT_I18N();
      /*  Possibly retrieve data  */
      gimp_get_data ("plug_in_gauss_iir", &bvals);
      break;

    default:
      break;
    }

  if (!(bvals.horizontal || bvals.vertical))
    {
      gimp_message ( _("gauss_iir: you must specify either horizontal or vertical (or both)"));
      status = STATUS_CALLING_ERROR;
    }

  if (status == STATUS_SUCCESS)
    {
      /*  Get the specified drawable  */
      drawable = gimp_drawable_get (param[2].data.d_drawable);

      /*  Make sure that the drawable is gray or RGB color  */
      if (gimp_drawable_is_rgb (drawable->id) ||
          gimp_drawable_is_gray (drawable->id))
        {
          gimp_progress_init ( _("IIR Gaussian Blur"));

          /*  set the tile cache size so that the gaussian blur works well  */
          gimp_tile_cache_ntiles (2 * (MAX (drawable->width, drawable->height) /
				  gimp_tile_width () + 1));

          radius = fabs (bvals.radius) + 1.0;
          std_dev = sqrt (-(radius * radius) / (2 * log (1.0 / 255.0)));

          /*  run the gaussian blur  */
          gauss_iir (drawable, bvals.horizontal, bvals.vertical, std_dev);

          if (run_mode != RUN_NONINTERACTIVE)
	    gimp_displays_flush ();

          /*  Store data  */
          if (run_mode == RUN_INTERACTIVE)
	    gimp_set_data ("plug_in_gauss_iir", &bvals, sizeof (BlurValues));
        }
      else
        {
          gimp_message ( "gauss_iir: cannot operate on indexed color images");
          status = STATUS_EXECUTION_ERROR;
        }

      gimp_drawable_detach (drawable);
    }

  values[0].data.d_status = status;
}

static gint
gauss_iir_dialog (void)
{
  GtkWidget *dlg;
  GtkWidget *label;
  GtkWidget *spinbutton;
  GtkWidget *adj;
  GtkWidget *toggle;
  GtkWidget *frame;
  GtkWidget *vbox;
  GtkWidget *hbox;
  gchar **argv;
  gint    argc;

  argc    = 1;
  argv    = g_new (gchar *, 1);
  argv[0] = g_strdup ("gauss_iir");

  gtk_init (&argc, &argv);
  gtk_rc_parse (gimp_gtkrc ());

  dlg = gimp_dialog_new (_("IIR Gaussian Blur"), "gauss_iir",
			 gimp_plugin_help_func, "filters/gauss_iir.html",
			 GTK_WIN_POS_MOUSE,
			 FALSE, TRUE, FALSE,

			 _("OK"), gauss_ok_callback,
			 NULL, NULL, NULL, TRUE, FALSE,
			 _("Cancel"), gtk_widget_destroy,
			 NULL, 1, NULL, FALSE, TRUE,

			 NULL);

  gtk_signal_connect (GTK_OBJECT (dlg), "destroy",
		      GTK_SIGNAL_FUNC (gtk_main_quit),
		      NULL);

  /*  parameter settings  */
  frame = gtk_frame_new (_("Parameter Settings"));
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_ETCHED_IN);
  gtk_container_set_border_width (GTK_CONTAINER (frame), 6);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dlg)->vbox), frame, TRUE, TRUE, 0);

  vbox = gtk_vbox_new (FALSE, 2);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), 4);
  gtk_container_add (GTK_CONTAINER (frame), vbox);

  toggle = gtk_check_button_new_with_label (_("Blur Horizontally"));
  gtk_box_pack_start (GTK_BOX (vbox), toggle, FALSE, FALSE, 0);
  gtk_signal_connect (GTK_OBJECT (toggle), "toggled",
		      (GtkSignalFunc) gimp_toggle_button_update,
		      &bvals.horizontal);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (toggle), bvals.horizontal);
  gtk_widget_show (toggle);

  toggle = gtk_check_button_new_with_label (_("Blur Vertically"));
  gtk_box_pack_start (GTK_BOX (vbox), toggle, FALSE, FALSE, 0);
  gtk_signal_connect (GTK_OBJECT (toggle), "toggled",
		      (GtkSignalFunc) gimp_toggle_button_update,
		      &bvals.vertical);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (toggle), bvals.vertical);
  gtk_widget_show (toggle);

  hbox = gtk_hbox_new (FALSE, 4);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, TRUE, TRUE, 0);

  label = gtk_label_new (_("Blur Radius:"));
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
  gtk_widget_show (label);

  spinbutton = gimp_spin_button_new (&adj,
				     bvals.radius, 1.0, G_MAXDOUBLE, 1.0, 5.0,
				     0, 1, 2);
  gtk_box_pack_start (GTK_BOX (hbox), spinbutton, TRUE, TRUE, 0);
  gtk_signal_connect (GTK_OBJECT (adj), "value_changed",
		      GTK_SIGNAL_FUNC (gimp_double_adjustment_update),
		      &bvals.radius);
  gtk_widget_show (spinbutton);

  gtk_widget_show (hbox);
  gtk_widget_show (vbox);
  gtk_widget_show (frame);
  gtk_widget_show (dlg);

  gtk_main ();
  gdk_flush ();

  return bint.run;
}

/* Convert from separated to premultiplied alpha, on a single scan line. */
static void
multiply_alpha (guchar *buf, gint width, gint bytes)
{
  gint i, j;
  gdouble alpha;

  for (i = 0; i < width * bytes; i += bytes)
    {
      alpha = buf[i + bytes - 1] * (1.0 / 255.0);
      for (j = 0; j < bytes - 1; j++)
	buf[i + j] *= alpha;
    }
}

/* Convert from premultiplied to separated alpha, on a single scan
   line. */
static void
separate_alpha (guchar *buf, gint width, gint bytes)
{
  gint i, j;
  guchar alpha;
  gdouble recip_alpha;
  gint new_val;

  for (i = 0; i < width * bytes; i += bytes)
    {
      alpha = buf[i + bytes - 1];
      if (alpha != 0 && alpha != 255)
	{
	  recip_alpha = 255.0 / alpha;
	  for (j = 0; j < bytes - 1; j++)
	    {
	      new_val = buf[i + j] * recip_alpha;
	      buf[i + j] = MIN (255, new_val);
	    }
	}
    }
}

static void
gauss_iir (GDrawable *drawable,
	   gint       horz,
	   gint       vert,
	   gdouble    std_dev)
{
  GPixelRgn src_rgn, dest_rgn;
  gint width, height;
  gint bytes;
  gint has_alpha;
  guchar *dest;
  guchar *src, *sp_p, *sp_m;
  gdouble n_p[5], n_m[5];
  gdouble d_p[5], d_m[5];
  gdouble bd_p[5], bd_m[5];
  gdouble *val_p, *val_m, *vp, *vm;
  gint x1, y1, x2, y2;
  gint i, j;
  gint row, col, b;
  gint terms;
  gint progress, max_progress;
  gint initial_p[4];
  gint initial_m[4];
  guchar *guc_tmp1, *guc_tmp2;
  gint *gi_tmp1, *gi_tmp2;

  gimp_drawable_mask_bounds (drawable->id, &x1, &y1, &x2, &y2);

  width = (x2 - x1);
  height = (y2 - y1);
  bytes = drawable->bpp;
  has_alpha = gimp_drawable_has_alpha(drawable->id);

  val_p = (gdouble *) g_malloc (MAX (width, height) * bytes * sizeof (gdouble));
  val_m = (gdouble *) g_malloc (MAX (width, height) * bytes * sizeof (gdouble));

  src = (guchar *) g_malloc (MAX (width, height) * bytes);
  dest = (guchar *) g_malloc (MAX (width, height) * bytes);

  /*  derive the constants for calculating the gaussian from the std dev  */
  find_constants (n_p, n_m, d_p, d_m, bd_p, bd_m, std_dev);

  progress = 0;
  max_progress = (horz) ? width * height : 0;
  max_progress += (vert) ? width * height : 0;

  gimp_pixel_rgn_init (&src_rgn, drawable, 0, 0, drawable->width, drawable->height, FALSE, FALSE);
  gimp_pixel_rgn_init (&dest_rgn, drawable, 0, 0, drawable->width, drawable->height, TRUE, TRUE);

  if (vert)
    {
      /*  First the vertical pass  */
      for (col = 0; col < width; col++)
	{
	  memset(val_p, 0, height * bytes * sizeof (gdouble));
	  memset(val_m, 0, height * bytes * sizeof (gdouble));

	  gimp_pixel_rgn_get_col (&src_rgn, src, col + x1, y1, (y2 - y1));

	  if (has_alpha)
	    multiply_alpha (src, height, bytes);

	  sp_p = src;
	  sp_m = src + (height - 1) * bytes;
	  vp = val_p;
	  vm = val_m + (height - 1) * bytes;

	  /*  Set up the first vals  */
#ifndef ORIGINAL_READABLE_CODE
	  for(guc_tmp1 = sp_p, guc_tmp2 = sp_m,
		gi_tmp1 = initial_p, gi_tmp2 = initial_m;
	      (guc_tmp1 - sp_p) < bytes;)
	    {
	      *gi_tmp1++ = *guc_tmp1++;
	      *gi_tmp2++ = *guc_tmp2++;
	    }
#else
	  for (i = 0; i < bytes; i++)
	    {
	      initial_p[i] = sp_p[i];
	      initial_m[i] = sp_m[i];
	    }
#endif

	  for (row = 0; row < height; row++)
	    {
	      gdouble *vpptr, *vmptr;
	      terms = (row < 4) ? row : 4;

	      for (b = 0; b < bytes; b++)
		{
		  vpptr = vp + b; vmptr = vm + b;
		  for (i = 0; i <= terms; i++)
		    {
		      *vpptr += n_p[i] * sp_p[(-i * bytes) + b] -
			d_p[i] * vp[(-i * bytes) + b];
		      *vmptr += n_m[i] * sp_m[(i * bytes) + b] -
			d_m[i] * vm[(i * bytes) + b];
		    }
		  for (j = i; j <= 4; j++)
		    {
		      *vpptr += (n_p[j] - bd_p[j]) * initial_p[b];
		      *vmptr += (n_m[j] - bd_m[j]) * initial_m[b];
		    }
		}

	      sp_p += bytes;
	      sp_m -= bytes;
	      vp += bytes;
	      vm -= bytes;
	    }

	  transfer_pixels (val_p, val_m, dest, bytes, height);

	  if (has_alpha && !horz)
	    separate_alpha (dest, height, bytes);

	  gimp_pixel_rgn_set_col (&dest_rgn, dest, col + x1, y1, (y2 - y1));

	  progress += height;
	  if ((col % 5) == 0)
	    gimp_progress_update ((double) progress / (double) max_progress);
	}

    }

  if (horz)
    {
      /*  prepare for the horizontal pass  */
  if (vert)    
       gimp_pixel_rgn_init (&src_rgn, drawable, 0, 0, drawable->width, drawable->height, FALSE, TRUE); 
      /*  Now the horizontal pass  */
      for (row = 0; row < height; row++)
	{
	  memset(val_p, 0, width * bytes * sizeof (gdouble));
	  memset(val_m, 0, width * bytes * sizeof (gdouble));

	  gimp_pixel_rgn_get_row (&src_rgn, src, x1, row + y1, (x2 - x1));

	  if (has_alpha && !vert)
	    multiply_alpha (src, height, bytes);

	  sp_p = src;
	  sp_m = src + (width - 1) * bytes;
	  vp = val_p;
	  vm = val_m + (width - 1) * bytes;

	  /*  Set up the first vals  */
#ifndef ORIGINAL_READABLE_CODE
	  for(guc_tmp1 = sp_p, guc_tmp2 = sp_m,
		gi_tmp1 = initial_p, gi_tmp2 = initial_m;
	      (guc_tmp1 - sp_p) < bytes;)
	    {
	      *gi_tmp1++ = *guc_tmp1++;
	      *gi_tmp2++ = *guc_tmp2++;
	    }
#else
	  for (i = 0; i < bytes; i++)
	    {
	      initial_p[i] = sp_p[i];
	      initial_m[i] = sp_m[i];
	    }
#endif

	  for (col = 0; col < width; col++)
	    {
	      gdouble *vpptr, *vmptr;
	      terms = (col < 4) ? col : 4;

	      for (b = 0; b < bytes; b++)
		{
		  vpptr = vp + b; vmptr = vm + b;
		  for (i = 0; i <= terms; i++)
		    {
		      *vpptr += n_p[i] * sp_p[(-i * bytes) + b] -
			d_p[i] * vp[(-i * bytes) + b];
		      *vmptr += n_m[i] * sp_m[(i * bytes) + b] -
			d_m[i] * vm[(i * bytes) + b];
		    }
		  for (j = i; j <= 4; j++)
		    {
		      *vpptr += (n_p[j] - bd_p[j]) * initial_p[b];
		      *vmptr += (n_m[j] - bd_m[j]) * initial_m[b];
		    }
		}

	      sp_p += bytes;
	      sp_m -= bytes;
	      vp += bytes;
	      vm -= bytes;
	    }

	  transfer_pixels (val_p, val_m, dest, bytes, width);

	  if (has_alpha)
	    separate_alpha (dest, width, bytes);

	  gimp_pixel_rgn_set_row (&dest_rgn, dest, x1, row + y1, (x2 - x1));

	  progress += width;
	  if ((row % 5) == 0)
	    gimp_progress_update ((double) progress / (double) max_progress);
	}
    }

  /*  merge the shadow, update the drawable  */
  gimp_drawable_flush (drawable);
  gimp_drawable_merge_shadow (drawable->id, TRUE);
  gimp_drawable_update (drawable->id, x1, y1, (x2 - x1), (y2 - y1));

  /*  free up buffers  */
  g_free (val_p);
  g_free (val_m);
  g_free (src);
  g_free (dest);
}

static void
transfer_pixels (gdouble *src1,
		 gdouble *src2,
		 guchar  *dest,
		 gint     bytes,
		 gint     width)
{
  gint b;
  gint bend = bytes * width;
  gdouble sum;

  for(b = 0; b < bend; b++)
    {
      sum = *src1++ + *src2++;
      if (sum > 255) sum = 255;
      else if(sum < 0) sum = 0;
	  
      *dest++ = (guchar) sum;
    }
}

static void
find_constants (gdouble n_p[],
		gdouble n_m[],
		gdouble d_p[],
		gdouble d_m[],
		gdouble bd_p[],
		gdouble bd_m[],
		gdouble std_dev)
{
  gint i;
  gdouble constants [8];
  gdouble div;

  /*  The constants used in the implemenation of a casual sequence
   *  using a 4th order approximation of the gaussian operator
   */

  div = sqrt(2 * G_PI) * std_dev;
  constants [0] = -1.783 / std_dev;
  constants [1] = -1.723 / std_dev;
  constants [2] = 0.6318 / std_dev;
  constants [3] = 1.997  / std_dev;
  constants [4] = 1.6803 / div;
  constants [5] = 3.735 / div;
  constants [6] = -0.6803 / div;
  constants [7] = -0.2598 / div;

  n_p [0] = constants[4] + constants[6];
  n_p [1] = exp (constants[1]) *
    (constants[7] * sin (constants[3]) -
     (constants[6] + 2 * constants[4]) * cos (constants[3])) +
       exp (constants[0]) *
	 (constants[5] * sin (constants[2]) -
	  (2 * constants[6] + constants[4]) * cos (constants[2]));
  n_p [2] = 2 * exp (constants[0] + constants[1]) *
    ((constants[4] + constants[6]) * cos (constants[3]) * cos (constants[2]) -
     constants[5] * cos (constants[3]) * sin (constants[2]) -
     constants[7] * cos (constants[2]) * sin (constants[3])) +
       constants[6] * exp (2 * constants[0]) +
	 constants[4] * exp (2 * constants[1]);
  n_p [3] = exp (constants[1] + 2 * constants[0]) *
    (constants[7] * sin (constants[3]) - constants[6] * cos (constants[3])) +
      exp (constants[0] + 2 * constants[1]) *
	(constants[5] * sin (constants[2]) - constants[4] * cos (constants[2]));
  n_p [4] = 0.0;

  d_p [0] = 0.0;
  d_p [1] = -2 * exp (constants[1]) * cos (constants[3]) -
    2 * exp (constants[0]) * cos (constants[2]);
  d_p [2] = 4 * cos (constants[3]) * cos (constants[2]) * exp (constants[0] + constants[1]) +
    exp (2 * constants[1]) + exp (2 * constants[0]);
  d_p [3] = -2 * cos (constants[2]) * exp (constants[0] + 2 * constants[1]) -
    2 * cos (constants[3]) * exp (constants[1] + 2 * constants[0]);
  d_p [4] = exp (2 * constants[0] + 2 * constants[1]);

#ifndef ORIGINAL_READABLE_CODE
  memcpy(d_m, d_p, 5 * sizeof(gdouble));
#else
  for (i = 0; i <= 4; i++)
    d_m [i] = d_p [i];
#endif

  n_m[0] = 0.0;
  for (i = 1; i <= 4; i++)
    n_m [i] = n_p[i] - d_p[i] * n_p[0];

  {
    gdouble sum_n_p, sum_n_m, sum_d;
    gdouble a, b;

    sum_n_p = 0.0;
    sum_n_m = 0.0;
    sum_d = 0.0;
    for (i = 0; i <= 4; i++)
      {
	sum_n_p += n_p[i];
	sum_n_m += n_m[i];
	sum_d += d_p[i];
      }

#ifndef ORIGINAL_READABLE_CODE
    sum_d++;
    a = sum_n_p / sum_d;
    b = sum_n_m / sum_d;
#else
    a = sum_n_p / (1 + sum_d);
    b = sum_n_m / (1 + sum_d);
#endif

    for (i = 0; i <= 4; i++)
      {
	bd_p[i] = d_p[i] * a;
	bd_m[i] = d_m[i] * b;
      }
  }
}

/*  Gauss interface functions  */

static void
gauss_ok_callback (GtkWidget *widget,
		   gpointer   data)
{
  bint.run = TRUE;

  gtk_widget_destroy (GTK_WIDGET (data));
}

static void
gauss_entry_callback (GtkWidget *widget,
		      gpointer   data)
{
  bvals.radius = atof (gtk_entry_get_text (GTK_ENTRY (widget)));
  if (bvals.radius < 1.0)
    bvals.radius = 1.0;
}
