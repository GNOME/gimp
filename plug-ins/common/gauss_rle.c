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
#include <stdio.h>
#include <stdlib.h>
#include "gtk/gtk.h"
#include "libgimp/gimp.h"

#define ENTRY_WIDTH 100

typedef struct
{
  gdouble radius;
  gint horizontal;
  gint vertical;
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
			 GParam    *param,
			 gint      *nreturn_vals,
			 GParam   **return_vals);

static void      gauss_rle         (GDrawable *drawable,
				    gint       horizontal,
				    gint       vertical,
				    gdouble    std_dev);

/*
 * Gaussian blur interface
 */
static gint      gauss_rle_dialog  (void);

/*
 * Gaussian blur helper functions
 */
static gint *    make_curve        (gdouble    sigma,
				    gint *     length);
static void      run_length_encode (guchar *   src,
				    gint *     dest,
				    gint       bytes,
				    gint       width);

static void      gauss_close_callback  (GtkWidget *widget,
					gpointer   data);
static void      gauss_ok_callback     (GtkWidget *widget,
					gpointer   data);
static void      gauss_toggle_update   (GtkWidget *widget,
					gpointer   data);
static void      gauss_entry_callback  (GtkWidget *widget,
					gpointer   data);

GPlugInInfo PLUG_IN_INFO =
{
  NULL,    /* init_proc */
  NULL,    /* quit_proc */
  query,   /* query_proc */
  run,     /* run_proc */
};

static BlurValues bvals =
{
  5.0,  /*  radius  */
  TRUE, /*  horizontal blur  */
  TRUE  /*  vertical blur  */
};

static BlurInterface bint =
{
  FALSE  /*  run  */
};


MAIN ()

static void
query ()
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

  gimp_install_procedure ("plug_in_gauss_rle",
			  "Applies a gaussian blur to the specified drawable.",
			  "Applies a gaussian blur to the drawable, with specified radius of affect.  The standard deviation of the normal distribution used to modify pixel values is calculated based on the supplied radius.  Horizontal and vertical blurring can be independently invoked by specifying only one to run.  The RLE gaussian blurring performs most efficiently on computer-generated images or images with large areas of constant intensity.  Values for radii less than 1.0 are invalid as they will generate spurious results.",
			  "Spencer Kimball & Peter Mattis",
			  "Spencer Kimball & Peter Mattis",
			  "1995-1996",
			  "<Image>/Filters/Blur/Gaussian Blur (RLE)...",
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
      /*  Possibly retrieve data  */
      gimp_get_data ("plug_in_gauss_rle", &bvals);

      /*  First acquire information with a dialog  */
      if (! gauss_rle_dialog ())
	return;
      break;

    case RUN_NONINTERACTIVE:
      /*  Make sure all the arguments are there!  */
      if (nparams != 6)
	status = STATUS_CALLING_ERROR;
      if (status == STATUS_SUCCESS)
	{
	  bvals.radius = param[3].data.d_float;
	  bvals.horizontal = (param[4].data.d_int32) ? TRUE : FALSE;
	  bvals.vertical = (param[5].data.d_int32) ? TRUE : FALSE;
	}
      if (status == STATUS_SUCCESS &&
	  (bvals.radius < 1.0))
	status = STATUS_CALLING_ERROR;
      break;

    case RUN_WITH_LAST_VALS:
      /*  Possibly retrieve data  */
      gimp_get_data ("plug_in_gauss_rle", &bvals);
      break;

    default:
      break;
    }

  if (!(bvals.horizontal || bvals.vertical))
    {
      gimp_message ("gauss_rle: you must specify either horizontal or vertical (or both)");
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
          gimp_progress_init ("RLE Gaussian Blur");

          /*  set the tile cache size so that the gaussian blur works well  */
          gimp_tile_cache_ntiles (2 * (MAX (drawable->width, drawable->height) /
				  gimp_tile_width () + 1));

          radius = fabs (bvals.radius) + 1.0;
          std_dev = sqrt (-(radius * radius) / (2 * log (1.0 / 255.0)));

          /*  run the gaussian blur  */
          gauss_rle (drawable, bvals.horizontal, bvals.vertical, std_dev);

          if (run_mode != RUN_NONINTERACTIVE)
	    gimp_displays_flush ();

          /*  Store data  */
          if (run_mode == RUN_INTERACTIVE)
	    gimp_set_data ("plug_in_gauss_rle", &bvals, sizeof (BlurValues));
        }
      else
        {
          gimp_message ("gauss_rle: cannot operate on indexed color images");
          status = STATUS_EXECUTION_ERROR;
        }

    gimp_drawable_detach (drawable);
  }

  values[0].data.d_status = status;
}

static gint
gauss_rle_dialog ()
{
  GtkWidget *dlg;
  GtkWidget *label;
  GtkWidget *entry;
  GtkWidget *button;
  GtkWidget *hbbox;
  GtkWidget *toggle;
  GtkWidget *frame;
  GtkWidget *vbox;
  GtkWidget *hbox;
  gchar buffer[12];
  gchar **argv;
  gint argc;

  argc = 1;
  argv = g_new (gchar *, 1);
  argv[0] = g_strdup ("gauss");

  gtk_init (&argc, &argv);
  gtk_rc_parse (gimp_gtkrc ());

  dlg = gtk_dialog_new ();
  gtk_window_set_title (GTK_WINDOW (dlg), "RLE Gaussian Blur");
  gtk_window_position (GTK_WINDOW (dlg), GTK_WIN_POS_MOUSE);
  gtk_signal_connect (GTK_OBJECT (dlg), "destroy",
		      (GtkSignalFunc) gauss_close_callback,
		      NULL);

  /*  Action area  */
  gtk_container_set_border_width (GTK_CONTAINER (GTK_DIALOG (dlg)->action_area), 2);
  gtk_box_set_homogeneous (GTK_BOX (GTK_DIALOG (dlg)->action_area), FALSE);
  hbbox = gtk_hbutton_box_new ();
  gtk_button_box_set_spacing (GTK_BUTTON_BOX (hbbox), 4);
  gtk_box_pack_end (GTK_BOX (GTK_DIALOG (dlg)->action_area), hbbox, FALSE, FALSE, 0);
  gtk_widget_show (hbbox);
 
  button = gtk_button_new_with_label ("OK");
  GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT);
  gtk_signal_connect (GTK_OBJECT (button), "clicked",
		      (GtkSignalFunc) gauss_ok_callback,
		      dlg);
  gtk_box_pack_start (GTK_BOX (hbbox), button, FALSE, FALSE, 0);
  gtk_widget_grab_default (button);
  gtk_widget_show (button);

  button = gtk_button_new_with_label ("Cancel");
  GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT);
  gtk_signal_connect_object (GTK_OBJECT (button), "clicked",
			     (GtkSignalFunc) gtk_widget_destroy,
			     GTK_OBJECT (dlg));
  gtk_box_pack_start (GTK_BOX (hbbox), button, FALSE, FALSE, 0);
  gtk_widget_show (button);

  /*  parameter settings  */
  frame = gtk_frame_new ("Parameter Settings");
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_ETCHED_IN);
  gtk_container_border_width (GTK_CONTAINER (frame), 10);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dlg)->vbox), frame, TRUE, TRUE, 0);
  vbox = gtk_vbox_new (FALSE, 5);
  gtk_container_border_width (GTK_CONTAINER (vbox), 10);
  gtk_container_add (GTK_CONTAINER (frame), vbox);

  toggle = gtk_check_button_new_with_label ("Blur Horizontally");
  gtk_box_pack_start (GTK_BOX (vbox), toggle, TRUE, TRUE, 0);
  gtk_signal_connect (GTK_OBJECT (toggle), "toggled",
		      (GtkSignalFunc) gauss_toggle_update,
		      &bvals.horizontal);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (toggle), bvals.horizontal);
  gtk_widget_show (toggle);

  toggle = gtk_check_button_new_with_label ("Blur Vertically");
  gtk_box_pack_start (GTK_BOX (vbox), toggle, TRUE, TRUE, 0);
  gtk_signal_connect (GTK_OBJECT (toggle), "toggled",
		      (GtkSignalFunc) gauss_toggle_update,
		      &bvals.vertical);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (toggle), bvals.vertical);
  gtk_widget_show (toggle);

  hbox = gtk_hbox_new (FALSE, 5);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, TRUE, TRUE, 0);

  label = gtk_label_new ("Blur Radius: ");
  gtk_box_pack_start (GTK_BOX (hbox), label, TRUE, FALSE, 0);
  gtk_widget_show (label);

  entry = gtk_entry_new ();
  gtk_box_pack_start (GTK_BOX (hbox), entry, TRUE, TRUE, 0);
  gtk_widget_set_usize (entry, ENTRY_WIDTH, 0);
  sprintf (buffer, "%f", bvals.radius);
  gtk_entry_set_text (GTK_ENTRY (entry), buffer);
  gtk_signal_connect (GTK_OBJECT (entry), "changed",
		      (GtkSignalFunc) gauss_entry_callback,
		      NULL);
  gtk_widget_show (entry);

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
gauss_rle (GDrawable *drawable,
	   gint       horz,
	   gint       vert,
	   gdouble    std_dev)
{
  GPixelRgn src_rgn, dest_rgn;
  gint width, height;
  gint bytes;
  gint has_alpha;
  guchar *dest, *dp;
  guchar *src, *sp;
  gint *buf, *bb;
  gint pixels;
  gint total;
  gint x1, y1, x2, y2;
  gint i, row, col, b;
  gint start, end;
  gint progress, max_progress;
  gint *curve;
  gint *sum;
  gint val;
  gint length;
  gint initial_p, initial_m;

  curve = make_curve (std_dev, &length);
  sum = malloc (sizeof (gint) * (2 * length + 1));

  sum[0] = 0;

  for (i = 1; i <= length*2; i++)
    sum[i] = curve[i-length-1] + sum[i-1];
  sum += length;

  gimp_drawable_mask_bounds (drawable->id, &x1, &y1, &x2, &y2);

  width = (x2 - x1);
  height = (y2 - y1);
  bytes = drawable->bpp;
  has_alpha = gimp_drawable_has_alpha(drawable->id);

  buf = (gint *) malloc (sizeof (gint) * MAX (width, height) * 2);

  total = sum[length] - sum[-length];

  /*  allocate buffers for source and destination pixels  */
  src = (guchar *) malloc (MAX (width, height) * bytes);
  dest = (guchar *) malloc (MAX (width, height) * bytes);

  gimp_pixel_rgn_init (&src_rgn, drawable, 0, 0, drawable->width, drawable->height, FALSE, FALSE);
  gimp_pixel_rgn_init (&dest_rgn, drawable, 0, 0, drawable->width, drawable->height, TRUE, TRUE);

  progress = 0;
  max_progress = (horz) ? width * height : 0;
  max_progress += (vert) ? width * height : 0;

  if (vert)
    {
      for (col = 0; col < width; col++)
	{
	  gimp_pixel_rgn_get_col (&src_rgn, src, col + x1, y1, (y2 - y1));

	  if (has_alpha)
	    multiply_alpha (src, height, bytes);

	  sp = src;
	  dp = dest;

	  for (b = 0; b < bytes; b++)
	    {
	      initial_p = sp[b];
	      initial_m = sp[(height-1) * bytes + b];

	      /*  Determine a run-length encoded version of the row  */
	      run_length_encode (sp + b, buf, bytes, height);

	      for (row = 0; row < height; row++)
		{
		  start = (row < length) ? -row : -length;
		  end = (height <= (row + length)) ? (height - row - 1) : length;

		  val = 0;
		  i = start;
		  bb = buf + (row + i) * 2;

		  if (start != -length)
		    val += initial_p * (sum[start] - sum[-length]);

		  while (i < end)
		    {
		      pixels = bb[0];
		      i += pixels;
		      if (i > end)
			i = end;
		      val += bb[1] * (sum[i] - sum[start]);
		      bb += (pixels * 2);
		      start = i;
		    }

		  if (end != length)
		    val += initial_m * (sum[length] - sum[end]);

		  dp[row * bytes + b] = val / total;
		}
	    }

	  if (has_alpha && !horz)
	    separate_alpha (dest, height, bytes);

	  gimp_pixel_rgn_set_col (&dest_rgn, dest, col + x1, y1, (y2 - y1));
	  progress += height;
	  if ((col % 5) == 0)
	    gimp_progress_update ((double) progress / (double) max_progress);
	}

      /*  prepare for the horizontal pass  */
      gimp_pixel_rgn_init (&src_rgn, drawable, 0, 0, drawable->width, drawable->height, FALSE, TRUE);
    }

  if (horz)
    {
      for (row = 0; row < height; row++)
	{
	  gimp_pixel_rgn_get_row (&src_rgn, src, x1, row + y1, (x2 - x1));

	  if (has_alpha && !vert)
	    multiply_alpha (src, height, bytes);

	  sp = src;
	  dp = dest;

	  for (b = 0; b < bytes; b++)
	    {
	      initial_p = sp[b];
	      initial_m = sp[(width-1) * bytes + b];

	      /*  Determine a run-length encoded version of the row  */
	      run_length_encode (sp + b, buf, bytes, width);

	      for (col = 0; col < width; col++)
		{
		  start = (col < length) ? -col : -length;
		  end = (width <= (col + length)) ? (width - col - 1) : length;

		  val = 0;
		  i = start;
		  bb = buf + (col + i) * 2;

		  if (start != -length)
		    val += initial_p * (sum[start] - sum[-length]);

		  while (i < end)
		    {
		      pixels = bb[0];
		      i += pixels;
		      if (i > end)
			i = end;
		      val += bb[1] * (sum[i] - sum[start]);
		      bb += (pixels * 2);
		      start = i;
		    }

		  if (end != length)
		    val += initial_m * (sum[length] - sum[end]);

		  dp[col * bytes + b] = val / total;
		}
	    }

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

  /*  free buffers  */
  free (buf);
  free (src);
  free (dest);
}

/*
 * The equations: g(r) = exp (- r^2 / (2 * sigma^2))
 *                   r = sqrt (x^2 + y ^2)
 */

static gint *
make_curve (gdouble  sigma,
	    gint    *length)
{
  gint *curve;
  gdouble sigma2;
  gdouble l;
  gint temp;
  gint i, n;

  sigma2 = 2 * sigma * sigma;
  l = sqrt (-sigma2 * log (1.0 / 255.0));

  n = ceil (l) * 2;
  if ((n % 2) == 0)
    n += 1;

  curve = malloc (sizeof (gint) * n);

  *length = n / 2;
  curve += *length;
  curve[0] = 255;

  for (i = 1; i <= *length; i++)
    {
      temp = (gint) (exp (- (i * i) / sigma2) * 255);
      curve[-i] = temp;
      curve[i] = temp;
    }

  return curve;
}

static void
run_length_encode (guchar *src,
		   gint   *dest,
		   gint    bytes,
		   gint    width)
{
  gint start;
  gint i;
  gint j;
  guchar last;

  last = *src;
  src += bytes;
  start = 0;

  for (i = 1; i < width; i++)
    {
      if (*src != last)
	{
	  for (j = start; j < i; j++)
	    {
	      *dest++ = (i - j);
	      *dest++ = last;
	    }
	  start = i;
	  last = *src;
	}
      src += bytes;
    }

  for (j = start; j < i; j++)
    {
      *dest++ = (i - j);
      *dest++ = last;
    }
}

/*  Gauss interface functions  */

static void
gauss_close_callback (GtkWidget *widget,
		      gpointer   data)
{
  gtk_main_quit ();
}

static void
gauss_ok_callback (GtkWidget *widget,
		   gpointer   data)
{
  bint.run = TRUE;
  gtk_widget_destroy (GTK_WIDGET (data));
}

static void
gauss_toggle_update (GtkWidget *widget,
		       gpointer   data)
{
  int *toggle_val;

  toggle_val = (int *) data;

  if (GTK_TOGGLE_BUTTON (widget)->active)
    *toggle_val = TRUE;
  else
    *toggle_val = FALSE;
}

static void
gauss_entry_callback (GtkWidget *widget,
		      gpointer   data)
{
  bvals.radius = atof (gtk_entry_get_text (GTK_ENTRY (widget)));
  if (bvals.radius < 1.0)
    bvals.radius = 1.0;
}
