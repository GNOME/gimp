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


/* This plugin by thorsten@arch.usyd.edu.au           */
/* Based on S&P's Gauss and Blur filters              */


/* updated 11/04/97:
   don't use rint;
   if gamma-channel: set to white if at least one colour channel is >15 */

/* Update 3/10/97:
   #ifdef Max and Min, 
   save old values
   correct 'cancel' behaviour */

#include "config.h"

#include <stdio.h>
#include <stdlib.h>

#include <gtk/gtk.h>

#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

#include "libgimp/stdplugins-intl.h"


typedef struct
{
  gint horizontal;
  gint vertical;
  gint keep_sign;
} SobelValues;

typedef struct
{
  gint run;
} SobelInterface;


/* Declare local functions.
 */
static void      query  (void);
static void      run    (gchar   *name,
			 gint     nparams,
			 GParam  *param,
			 gint    *nreturn_vals,
			 GParam **return_vals);

static void      sobel  (GDrawable *drawable,
			 gint       horizontal,
			 gint       vertical,
			 gint       keep_sign);

/*
 * Sobel interface
 */
static gint      sobel_dialog (void);

/*
 * Sobel helper functions
 */
static void      sobel_ok_callback     (GtkWidget *widget,
					gpointer   data);

static void      sobel_prepare_row (GPixelRgn  *pixel_rgn,
				    guchar     *data,
				    gint        x,
				    gint        y,
				    gint        w);


GPlugInInfo PLUG_IN_INFO =
{
  NULL,  /* init_proc  */
  NULL,  /* quit_proc  */
  query, /* query_proc */
  run,   /* run_proc   */
};

static SobelValues bvals =
{
  TRUE,  /*  horizontal sobel  */
  TRUE,  /*  vertical sobel    */
  TRUE   /*  keep sign         */
};

static SobelInterface bint =
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
    { PARAM_INT32, "horizontal", "Sobel in horizontal direction" },
    { PARAM_INT32, "vertical", "Sobel in vertical direction" },
    { PARAM_INT32, "keep_sign", "Keep sign of result (one direction only)" }
  };
  static gint nargs = sizeof (args) / sizeof (args[0]);

  gimp_install_procedure ("plug_in_sobel",
			  "Edge Detection with Sobel Operation",
			  "This plugin calculates the gradient with a sobel "
			  "operator. The user can specify which direction to "
			  "use. When both directions are used, the result is "
			  "the RMS of the two gradients; if only one direction "
			  "is used, the result either the absolut value of the "
			  "gradient, or 127 + gradient (if the 'keep sign' "
			  "switch is on). This way, information about the "
			  "direction of the gradient is preserved. Resulting "
			  "images are not autoscaled.",
			  "Thorsten Schnier",
			  "Thorsten Schnier",
			  "1997",
			  N_("<Image>/Filters/Edge-Detect/Sobel..."),
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
      gimp_get_data ("plug_in_sobel", &bvals);

      /*  First acquire information with a dialog  */
      if (! sobel_dialog ())
	return;
      break;

    case RUN_NONINTERACTIVE:
      INIT_I18N();
      /*  Make sure all the arguments are there!  */
      if (nparams != 6)
	{
	  status = STATUS_CALLING_ERROR;
	}
      else
	{
	  bvals.horizontal = (param[4].data.d_int32) ? TRUE : FALSE;
	  bvals.vertical   = (param[5].data.d_int32) ? TRUE : FALSE;
	  bvals.keep_sign  = (param[6].data.d_int32) ? TRUE : FALSE;
	}
      break;

    case RUN_WITH_LAST_VALS:
      INIT_I18N();
      /*  Possibly retrieve data  */
      gimp_get_data ("plug_in_sobel", &bvals);
      break;

    default:
      break;
    }


  /*  Get the specified drawable  */
  drawable = gimp_drawable_get (param[2].data.d_drawable);

  /*  Make sure that the drawable is gray or RGB color  */
  if (gimp_drawable_is_rgb (drawable->id) ||
      gimp_drawable_is_gray (drawable->id))
    {
      gimp_tile_cache_ntiles (2 * (drawable->width / gimp_tile_width () + 1));
      sobel (drawable, bvals.horizontal, bvals.vertical, bvals.keep_sign);

      if (run_mode != RUN_NONINTERACTIVE)
	gimp_displays_flush ();

      
      /*  Store data  */
      if (run_mode == RUN_INTERACTIVE)
	gimp_set_data ("plug_in_sobel", &bvals, sizeof (bvals));    
    }
  else
    {
      /* g_message ("sobel: cannot operate on indexed color images"); */
      status = STATUS_EXECUTION_ERROR;
    }

  gimp_drawable_detach (drawable);

  values[0].data.d_status = status;
}

static gint
sobel_dialog (void)
{
  GtkWidget *dlg;
  GtkWidget *toggle;
  GtkWidget *frame;
  GtkWidget *vbox;

  gimp_ui_init ("sobel", FALSE);

  dlg = gimp_dialog_new (_("Sobel Edge Detection"), "sobel",
			 gimp_plugin_help_func, "filters/sobel.html",
			 GTK_WIN_POS_MOUSE,
			 FALSE, TRUE, FALSE,

			 _("OK"), sobel_ok_callback,
			 NULL, NULL, NULL, TRUE, FALSE,
			 _("Cancel"), gtk_widget_destroy,
			 NULL, 1, NULL, FALSE, TRUE,

			 NULL);

  gtk_signal_connect (GTK_OBJECT (dlg), "destroy",
		      GTK_SIGNAL_FUNC (gtk_main_quit),
		      NULL);

  /*  parameter settings  */
  frame = gtk_frame_new ( _("Parameter Settings"));
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_ETCHED_IN);
  gtk_container_set_border_width (GTK_CONTAINER (frame), 6);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dlg)->vbox), frame, TRUE, TRUE, 0);

  vbox = gtk_vbox_new (FALSE, 1);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), 2);
  gtk_container_add (GTK_CONTAINER (frame), vbox);

  toggle = gtk_check_button_new_with_label (_("Sobel Horizontally"));
  gtk_box_pack_start (GTK_BOX (vbox), toggle, FALSE, FALSE, 0);
  gtk_signal_connect (GTK_OBJECT (toggle), "toggled",
		      GTK_SIGNAL_FUNC (gimp_toggle_button_update),
		      &bvals.horizontal);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (toggle), bvals.horizontal);
  gtk_widget_show (toggle);

  toggle = gtk_check_button_new_with_label (_("Sobel Vertically"));
  gtk_box_pack_start (GTK_BOX (vbox), toggle, FALSE, FALSE, 0);
  gtk_signal_connect (GTK_OBJECT (toggle), "toggled",
		      GTK_SIGNAL_FUNC (gimp_toggle_button_update),
		      &bvals.vertical);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (toggle), bvals.vertical);
  gtk_widget_show (toggle);

  toggle = gtk_check_button_new_with_label (_("Keep Sign of Result (one Direction only)"));
  gtk_box_pack_start (GTK_BOX (vbox), toggle, FALSE, FALSE, 0);
  gtk_signal_connect (GTK_OBJECT (toggle), "toggled",
		      GTK_SIGNAL_FUNC (gimp_toggle_button_update),
		      &bvals.keep_sign);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (toggle), bvals.vertical);
  gtk_widget_show (toggle);

  gtk_widget_show (vbox);
  gtk_widget_show (frame);
  gtk_widget_show (dlg);

  gtk_main ();
  gdk_flush ();

  return bint.run;
}

static void
sobel_prepare_row (GPixelRgn *pixel_rgn,
		   guchar    *data,
		   gint       x,
		   gint       y,
		   gint       w)
{
  gint b;

  if (y == 0)
    gimp_pixel_rgn_get_row (pixel_rgn, data, x, (y + 1), w);
  else if (y == pixel_rgn->h)
    gimp_pixel_rgn_get_row (pixel_rgn, data, x, (y - 1), w);
  else
    gimp_pixel_rgn_get_row (pixel_rgn, data, x, y, w);

  /*  Fill in edge pixels  */
  for (b = 0; b < pixel_rgn->bpp; b++)
    {
      data[-(int)pixel_rgn->bpp + b] = data[b];
      data[w * pixel_rgn->bpp + b] = data[(w - 1) * pixel_rgn->bpp + b];
    }
}

#define SIGN(a) (((a) > 0) ? 1 : -1)
#define RMS(a,b) (sqrt (pow ((a),2) + pow ((b), 2)))

static void
sobel (GDrawable *drawable,
       gint       do_horizontal,
       gint       do_vertical,
       gint       keep_sign)
{
  GPixelRgn srcPR, destPR;
  gint    width, height;
  gint    bytes;
  gint    gradient, hor_gradient, ver_gradient;
  guchar *dest, *d;
  guchar *prev_row, *pr;
  guchar *cur_row, *cr;
  guchar *next_row, *nr;
  guchar *tmp;
  gint    row, col;
  gint    x1, y1, x2, y2;
  gint    alpha;
  gint    counter;

  /* Get the input area. This is the bounding box of the selection in
   *  the image (or the entire image if there is no selection). Only
   *  operating on the input area is simply an optimization. It doesn't
   *  need to be done for correct operation. (It simply makes it go
   *  faster, since fewer pixels need to be operated on).
   */

  gimp_drawable_mask_bounds (drawable->id, &x1, &y1, &x2, &y2);
  gimp_progress_init (_("Sobel Edge Detecting..."));

  /* Get the size of the input image. (This will/must be the same
   *  as the size of the output image.
   */
  width  = drawable->width;
  height = drawable->height;
  bytes  = drawable->bpp;
  alpha  = gimp_drawable_has_alpha (drawable -> id);

  /*  allocate row buffers  */
  prev_row = g_new (guchar, (x2 - x1 + 2) * bytes);
  cur_row  = g_new (guchar, (x2 - x1 + 2) * bytes);
  next_row = g_new (guchar, (x2 - x1 + 2) * bytes);
  dest     = g_new (guchar, (x2 - x1) * bytes);

  /*  initialize the pixel regions  */
  gimp_pixel_rgn_init (&srcPR, drawable, 0, 0, width, height, FALSE, FALSE);
  gimp_pixel_rgn_init (&destPR, drawable, 0, 0, width, height, TRUE, TRUE);

  pr = prev_row + bytes;
  cr = cur_row  + bytes;
  nr = next_row + bytes;

  sobel_prepare_row (&srcPR, pr, x1, y1 - 1, (x2 - x1));
  sobel_prepare_row (&srcPR, cr, x1, y1, (x2 - x1));
  counter =0;
  /*  loop through the rows, applying the sobel convolution  */
  for (row = y1; row < y2; row++)
    {
      /*  prepare the next row  */
      sobel_prepare_row (&srcPR, nr, x1, row + 1, (x2 - x1));

      d = dest;
      for (col = 0; col < (x2 - x1) * bytes; col++)
	{
	  hor_gradient = (do_horizontal ?
			  ((pr[col - bytes] +  2 * pr[col] + pr[col + bytes]) -
			   (nr[col - bytes] + 2 * nr[col] + nr[col + bytes]))
			  : 0);
	  ver_gradient = (do_vertical ?
			  ((pr[col - bytes] + 2 * cr[col - bytes] + nr[col - bytes]) -
			   (pr[col + bytes] + 2 * cr[col + bytes] + nr[col + bytes]))
			  : 0);
	  gradient = (do_vertical && do_horizontal) ?
	    (ROUND (RMS (hor_gradient, ver_gradient)) / 5.66) /* always >0 */
	    : (keep_sign ? (127 + (ROUND ((hor_gradient + ver_gradient) / 8.0)))
	       : (ROUND (abs (hor_gradient + ver_gradient) / 4.0)));

	  if (alpha && (((col + 1) % bytes) == 0))
	    { /* the alpha channel */
	      *d++ = (counter == 0) ? 0 : 255;
	      counter = 0;
	    }
	  else
	    {
	      *d++ = gradient;
	      if (gradient > 10) counter ++;
	    }
	}
      /*  store the dest  */
      gimp_pixel_rgn_set_row (&destPR, dest, x1, row, (x2 - x1));

      /*  shuffle the row pointers  */
      tmp = pr;
      pr = cr;
      cr = nr;
      nr = tmp;

      if ((row % 5) == 0)
	gimp_progress_update ((double) row / (double) (y2 - y1));
    }

  /*  update the sobeled region  */
  gimp_drawable_flush (drawable);
  gimp_drawable_merge_shadow (drawable->id, TRUE);
  gimp_drawable_update (drawable->id, x1, y1, (x2 - x1), (y2 - y1));

  g_free (prev_row);
  g_free (cur_row);
  g_free (next_row);
  g_free (dest);
}

/*  Sobel interface functions  */

static void
sobel_ok_callback (GtkWidget *widget,
		   gpointer   data)
{
  bint.run = TRUE;

  gtk_widget_destroy (GTK_WIDGET (data));
}
