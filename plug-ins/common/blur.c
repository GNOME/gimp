/****************************************************************************
 * This is a plugin for the GIMP v 0.99.8 or later.  Documentation is
 * available at http://www.rru.com/~meo/gimp/ .
 *
 * Copyright (C) 1997-98 Miles O'Neal  <meo@rru.com>  http://www.rru.com/~meo/
 * Blur code Copyright (C) 1995 Spencer Kimball and Peter Mattis
 * GUI based on GTK code from:
 *    alienmap (Copyright (C) 1996, 1997 Daniel Cotting)
 *    plasma   (Copyright (C) 1996 Stephen Norris),
 *    oilify   (Copyright (C) 1996 Torsten Martinsen),
 *    ripple   (Copyright (C) 1997 Brian Degenhardt) and
 *    whirl    (Copyright (C) 1997 Federico Mena Quintero).
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
 ****************************************************************************/

/****************************************************************************
 * Blur:
 *
 * blur version 2.0 (1 May 1998, MEO)
 * history
 *     2.0 -  1 May 1998 MEO
 *         based on randomize 1.7
 *
 * Please send any patches or suggestions to the author: meo@rru.com .
 * 
 * Blur applies a 3x3 blurring convolution kernel to the specified drawable.
 * 
 * For each pixel in the selection or image,
 * whether to change the pixel is decided by picking a
 * random number, weighted by the user's "randomization" percentage.
 * If the random number is in range, the pixel is modified.  For
 * blurring, an average is determined from the current and adjacent
 * pixels. *(Except for the random factor, the blur code came
 * straight from the original S&P blur plug-in.)*
 * 
 * This works only with RGB and grayscale images.
 * 
 ****************************************************************************/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <gtk/gtk.h>

#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

#include "libgimp/stdplugins-intl.h"

/*********************************
 *
 *  PLUGIN-SPECIFIC CONSTANTS
 *
 ********************************/

/*
 *  progress meter update frequency
 */
#define PROG_UPDATE_TIME ((row % 10) == 0)

#define PLUG_IN_NAME "plug_in_blur"
#define BLUR_VERSION "Blur 2.0"

#define SEED_TIME 10
#define SEED_USER 11

#define SCALE_WIDTH 100

/*********************************
 *
 *  PLUGIN-SPECIFIC STRUCTURES AND DATA
 *
 ********************************/

typedef struct
{
  gdouble blur_pct;     /* likelihood of randomization (as %age) */
  gdouble blur_rcount;  /* repeat count */
  gint    seed_type;    /* seed init. type - current time or user value */
  gint    blur_seed;    /* seed value for rand() function */
} BlurVals;

static BlurVals pivals =
{
  100.0,
  1.0,
  SEED_TIME,
  0,
};

typedef struct
{
  gint run;
} BlurInterface;

static BlurInterface blur_int =
{
  FALSE     /*  have we run? */
};


/*********************************
 *
 *  LOCAL FUNCTIONS
 *
 ********************************/

static void query (void);
static void run   (gchar   *name,
		   gint     nparams,
		   GimpParam  *param,
		   gint    *nreturn_vals,
		   GimpParam **return_vals);

GimpPlugInInfo PLUG_IN_INFO =
{
  NULL,  /* init_proc  */
  NULL,  /* quit_proc  */
  query, /* query_proc */
  run,   /* run_proc   */
};

static void blur (GimpDrawable *drawable);

static inline void blur_prepare_row (GimpPixelRgn *pixel_rgn,
				     guchar    *data,
				     gint       x,
				     gint       y,
				     gint       w);

static gint blur_dialog      (void);
static void blur_ok_callback (GtkWidget *widget,
			      gpointer   data);

/************************************ Guts ***********************************/

MAIN ()

/*********************************
 *
 *  query() - query_proc
 *
 *      called by the GIMP to learn about this plug-in
 *
 ********************************/

static void
query (void)
{
  static GimpParamDef args_ni[] =
  {
    { GIMP_PDB_INT32, "run_mode", "non-interactive" },
    { GIMP_PDB_IMAGE, "image", "Input image (unused)" },
    { GIMP_PDB_DRAWABLE, "drawable", "Input drawable" }
  };
  static gint nargs_ni = sizeof(args_ni) / sizeof (args_ni[0]);

  static GimpParamDef args[] =
  {
    { GIMP_PDB_INT32, "run_mode", "Interactive, non-interactive" },
    { GIMP_PDB_IMAGE, "image", "Input image (unused)" },
    { GIMP_PDB_DRAWABLE, "drawable", "Input drawable" },
    { GIMP_PDB_FLOAT, "blur_pct", "Randomization percentage (1 - 100)" },
    { GIMP_PDB_FLOAT, "blur_rcount", "Repeat count(1 - 100)" },
    { GIMP_PDB_INT32, "seed_type", "Seed type (10 = current time, 11 = seed value)" },
    { GIMP_PDB_INT32, "blur_seed", "Seed value (used only if seed type is 11)" }
  };
  static gint nargs = sizeof(args) / sizeof (args[0]);

  const gchar *blurb = "Apply a 3x3 blurring convolution kernel to the specified drawable.";
  const gchar *help = "This plug-in randomly blurs the specified drawable, using a 3x3 blur.  You control the percentage of the pixels that are blurred and the number of times blurring is applied.  Indexed images are not supported.";
  const gchar *author = "Miles O'Neal  <meo@rru.com>  http://www.rru.com/~meo/";
  const gchar *copyrights = "Miles O'Neal, Spencer Kimball, Peter Mattis, Torsten Martinsen, Brian Degenhardt, Federico Mena Quintero, Stephen Norris, Daniel Cotting";
  const gchar *copyright_date = "1995-1998";

  gimp_install_procedure ("plug_in_blur_randomize",
			  (gchar *) blurb,
			  (gchar *) help,
			  (gchar *) author,
			  (gchar *) copyrights,
			  (gchar *) copyright_date,
			  N_("<Image>/Filters/Blur/Blur..."),
			  "RGB*, GRAY*",
			  GIMP_PLUGIN,
			  nargs, 0,
			  args, NULL);

  gimp_install_procedure (PLUG_IN_NAME,
			  (gchar *) blurb,
			  (gchar *) help,
			  (gchar *) author,
			  (gchar *) copyrights,
			  (gchar *) copyright_date,
			  NULL,
			  "RGB*, GRAY*",
			  GIMP_PLUGIN,
			  nargs_ni, 0,
			  args_ni, NULL);
}

/*********************************
 *
 *  run() - main routine
 *
 *  This handles the main interaction with the GIMP itself,
 *  and invokes the routine that actually does the work.
 *
 ********************************/

static void
run (gchar   *name, 
     gint     nparams, 
     GimpParam  *param, 
     gint    *nreturn_vals,
     GimpParam **return_vals)
{
  GimpDrawable *drawable;
  GimpRunModeType run_mode;
  GimpPDBStatusType status = GIMP_PDB_SUCCESS;        /* assume the best! */
  gchar prog_label[32];
  static GimpParam values[1];

  INIT_I18N_UI();

  /*
   *  Get the specified drawable, do standard initialization.
   */
  run_mode = param[0].data.d_int32;
  drawable = gimp_drawable_get(param[2].data.d_drawable);

  values[0].type = GIMP_PDB_STATUS;
  values[0].data.d_status = status;
  *nreturn_vals = 1;
  *return_vals = values;
  /*
   *  Make sure the drawable type is appropriate.
   */
  if (gimp_drawable_is_rgb (drawable->id) ||
      gimp_drawable_is_gray (drawable->id))
    {
      switch (run_mode)
	{
	  /*
	   *  If we're running interactively, pop up the dialog box.
	   */
	case GIMP_RUN_INTERACTIVE:
	  gimp_get_data (PLUG_IN_NAME, &pivals);
	  if (!blur_dialog ())        /* return on Cancel */
	    return;
	  break;

	  /*
	   *  If we're not interactive (probably scripting), we
	   *  get the parameters from the param[] array, since
	   *  we don't use the dialog box.  Make sure all
	   *  parameters have legitimate values.
	   */
	case GIMP_RUN_NONINTERACTIVE:
	  if ((strcmp (name, "plug_in_blur_randomize") == 0) &&
	      (nparams == 7))
	    {
	      pivals.blur_pct    = (gdouble) param[3].data.d_float;
	      pivals.blur_pct    = (gdouble) MIN (100.0, pivals.blur_pct);
	      pivals.blur_pct    = (gdouble) MAX (1.0, pivals.blur_pct);
	      pivals.blur_rcount = (gdouble) param[4].data.d_float;
	      pivals.blur_rcount = (gdouble) MIN (100.0,pivals.blur_rcount);
	      pivals.blur_rcount = (gdouble) MAX (1.0, pivals.blur_rcount);
	      pivals.seed_type   = (gint) param[5].data.d_int32;
	      pivals.seed_type   = (gint) MIN (SEED_USER, param[5].data.d_int32);
	      pivals.seed_type   = (gint) MAX (SEED_TIME, param[5].data.d_int32);
	      pivals.blur_seed   = (gint) param[6].data.d_int32;
	    }
	  else if ((strcmp (name, PLUG_IN_NAME) == 0) &&
		   (nparams == 3))
	    {
	      pivals.blur_pct    = (gdouble) 100.0;
	      pivals.blur_rcount = (gdouble) 1.0;
	      pivals.seed_type   = SEED_TIME;
	      pivals.blur_seed   = 0;
	    }
	  else
	    {
	      status = GIMP_PDB_CALLING_ERROR;
	    }
	  break;

	  /*
	   *  If we're running with the last set of values, get those values.
	   */
	case GIMP_RUN_WITH_LAST_VALS:
	  gimp_get_data (PLUG_IN_NAME, &pivals);
	  break;

	  /*
	   *  Hopefully we never get here!
	   */
	default:
	  break;
        }

      if (status == GIMP_PDB_SUCCESS)
	{
	  /*
	   *  JUST DO IT!
	   */
	  strcpy (prog_label, BLUR_VERSION);
	  gimp_progress_init (prog_label);
	  gimp_tile_cache_ntiles (2 * (drawable->width / gimp_tile_width() + 1));
	  /*
	   *  Initialize the rand() function seed
	   */
	  if (pivals.seed_type == SEED_TIME)
	    pivals.blur_seed = time (NULL);

	  srand (pivals.blur_seed);

	  blur (drawable);
	  /*
	   *  If we ran interactively (even repeating) update the display.
	   */
	  if (run_mode != GIMP_RUN_NONINTERACTIVE)
	    {
	      gimp_displays_flush ();
            }
	  /*
	   *  If we use the dialog popup, set the data for future use.
	   */
	  if (run_mode == GIMP_RUN_INTERACTIVE)
	    {
	      gimp_set_data (PLUG_IN_NAME, &pivals, sizeof (BlurVals));
            }
        }
    }
  else
    {
      /*
       *  If we got the wrong drawable type, we need to complain.
       */
      status = GIMP_PDB_EXECUTION_ERROR;
    }
  /*
   *  DONE!
   *  Set the status where the GIMP can see it, and let go
   *  of the drawable.
   */
  values[0].data.d_status = status;
  gimp_drawable_detach (drawable);
}

/*********************************
 *
 *  blur_prepare_row()
 *
 *  Get a row of pixels.  If the requested row
 *  is off the edge, clone the edge row.
 *
 ********************************/

static inline void
blur_prepare_row (GimpPixelRgn *pixel_rgn, 
		  guchar    *data, 
		  int        x, 
		  int        y, 
		  int        w)
{
  int b;

  if (y == 0)
    {
      gimp_pixel_rgn_get_row(pixel_rgn, data, x, (y + 1), w);
    }
  else if (y == pixel_rgn->h)
    {
      gimp_pixel_rgn_get_row(pixel_rgn, data, x, (y - 1), w);
    }
  else
    {
      gimp_pixel_rgn_get_row(pixel_rgn, data, x, y, w);
    }
  /*
   *  Fill in edge pixels
   */
  for (b = 0; b < pixel_rgn->bpp; b++)
    {
      data[-(gint)pixel_rgn->bpp + b] = data[b];
      data[w * pixel_rgn->bpp + b] = data[(w - 1) * pixel_rgn->bpp + b];
    }
}

/*********************************
 *
 *  blur()
 *
 *  Actually mess with the image.
 *
 ********************************/

static void
blur (GimpDrawable *drawable)
{
  GimpPixelRgn srcPR, destPR, destPR2, *sp, *dp, *tp;
  gint width, height;
  gint bytes;
  guchar *dest, *d;
  guchar *prev_row, *pr;
  guchar *cur_row, *cr;
  guchar *next_row, *nr;
  guchar *tmp;
  gint row, col;
  gint x1, y1, x2, y2;
  gint cnt;
  gint has_alpha, ind;

  /*
   *  Get the input area. This is the bounding box of the selection in
   *  the image (or the entire image if there is no selection). Only
   *  operating on the input area is simply an optimization. It doesn't
   *  need to be done for correct operation. (It simply makes it go
   *  faster, since fewer pixels need to be operated on).
   */
  gimp_drawable_mask_bounds(drawable->id, &x1, &y1, &x2, &y2);
  /*
   *  Get the size of the input image. (This will/must be the same
   *  as the size of the output image.  Also get alpha info.
   */
  width = drawable->width;
  height = drawable->height;
  bytes = drawable->bpp;
  has_alpha = gimp_drawable_has_alpha(drawable->id);
  /*
   *  allocate row buffers
   */
  prev_row = g_new (guchar, (x2 - x1 + 2) * bytes);
  cur_row  = g_new (guchar, (x2 - x1 + 2) * bytes);
  next_row = g_new (guchar, (x2 - x1 + 2) * bytes);
  dest     = g_new (guchar, (x2 - x1) * bytes);

  /*
   *  initialize the pixel regions
   */
  gimp_pixel_rgn_init (&srcPR, drawable, 0, 0, width, height, FALSE, FALSE);
  gimp_pixel_rgn_init (&destPR, drawable, 0, 0, width, height, TRUE, TRUE);
  gimp_pixel_rgn_init (&destPR2, drawable, 0, 0, width, height, TRUE, TRUE);
  sp = &srcPR;
  dp = &destPR;
  tp = NULL;

  pr = prev_row + bytes;
  cr = cur_row + bytes;
  nr = next_row + bytes;

  for (cnt = 1; cnt <= pivals.blur_rcount; cnt++)
    {
      /*
       *  prepare the first row and previous row
       */
      blur_prepare_row(sp, pr, x1, y1 - 1, (x2 - x1));
      blur_prepare_row(dp, cr, x1, y1, (x2 - x1));
      /*
       *  loop through the rows, applying the selected convolution
       */
      for (row = y1; row < y2; row++)
	{
	  /*  prepare the next row  */
	  blur_prepare_row(sp, nr, x1, row + 1, (x2 - x1));

	  d = dest;
	  ind = 0;
	  for (col = 0; col < (x2 - x1) * bytes; col++)
	    {
	      if (((rand() % 100)) <= (gint) pivals.blur_pct)
		{
		  ind++;
		  if (ind==bytes || !(has_alpha))
		    {
		      /*
		       *  If no alpha channel,
		       *   or if there is one and this is it...
		       */
		      *d++ = ((gint) pr[col - bytes] + (gint) pr[col] +
			      (gint) pr[col + bytes] +
			      (gint) cr[col - bytes] + (gint) cr[col] +
			      (gint) cr[col + bytes] +
			      (gint) nr[col - bytes] + (gint) nr[col] +
			      (gint) nr[col + bytes]) / 9;
		      ind = 0;
                    }
		  else
		    {
		      /*
		       *  otherwise we have an alpha channel,
		       *   but this is a color channel
		       */
		      *d++ = ((gint)
			      (((gdouble) (pr[col - bytes] * pr[col - ind]) 
				+ (gdouble) (pr[col] * pr[col + bytes - ind])
				+ (gdouble) (pr[col + bytes] * pr[col + 2*bytes - ind])
				+ (gdouble) (cr[col - bytes] * cr[col - ind])
				+ (gdouble) (cr[col] * cr[col + bytes - ind])
				+ (gdouble) (cr[col + bytes] * cr[col + 2*bytes - ind])
				+ (gdouble) (nr[col - bytes] * nr[col - ind])
				+ (gdouble) (nr[col] * nr[col + bytes - ind]) 
				+ (gdouble) (nr[col + bytes] * nr[col + 2*bytes - ind]))
                             / ((gdouble) pr[col - ind]
				+ (gdouble) pr[col + bytes - ind]
				+ (gdouble) pr[col + 2*bytes - ind]
				+ (gdouble) cr[col - ind]
				+ (gdouble) cr[col + bytes - ind]
				+ (gdouble) cr[col + 2*bytes - ind]
				+ (gdouble) nr[col - ind]
				+ (gdouble) nr[col + bytes - ind]
				+ (gdouble) nr[col + 2*bytes - ind])));
                    }
		  /*
		   *  Otherwise, this pixel was not selected for randomization,
		   *  so use the current value.
		   */
                }
	      else
		{
		  *d++ = (gint) cr[col];
                }
            }
	  /*
	   *  Save the modified row, shuffle the row pointers, and every
	   *  so often, update the progress meter.
	   */
	  gimp_pixel_rgn_set_row(dp, dest, x1, row, (x2 - x1));

	  tmp = pr;
	  pr = cr;
	  cr = nr;
	  nr = tmp;

	  if (PROG_UPDATE_TIME)
	    gimp_progress_update((double) row / (double) (y2 - y1));
        }
      /*
       *  if we have more cycles to perform, swap the src and dest Pixel Regions
       */
      if (cnt < pivals.blur_rcount)
	{
	  if (tp != NULL)
	    {
	      tp = dp;
	      dp = sp;
	      sp = tp;
            }
	  else
	    {
	      tp = &srcPR;
	      sp = &destPR;
	      dp = &destPR2;
            }
        }
    }
  gimp_progress_update ((double) 100);
  /*
   *  update the blurred region
   */
  gimp_drawable_flush(drawable);
  gimp_drawable_merge_shadow(drawable->id, TRUE);
  gimp_drawable_update(drawable->id, x1, y1, (x2 - x1), (y2 - y1));
  /*
   *  clean up after ourselves.
   */
  g_free (prev_row);
  g_free (cur_row);
  g_free (next_row);
  g_free (dest);
}

/*********************************
 *  GUI ROUTINES
 ********************************/

static gint
blur_dialog (void)
{
  GtkWidget *dlg;
  GtkWidget *frame;
  GtkWidget *table;
  GtkWidget *seed_hbox;
  GtkObject *adj;

  gimp_ui_init ("blur", FALSE);

  dlg = gimp_dialog_new (BLUR_VERSION, "blur",
			 gimp_standard_help_func, "filters/blur.html",
			 GTK_WIN_POS_MOUSE,
			 FALSE, TRUE, FALSE,

			 _("OK"), blur_ok_callback,
			 NULL, NULL, NULL, TRUE, FALSE,
			 _("Cancel"), gtk_widget_destroy,
			 NULL, 1, NULL, FALSE, TRUE,

			 NULL);

  gtk_signal_connect (GTK_OBJECT (dlg), "destroy",
		      GTK_SIGNAL_FUNC (gtk_main_quit),
		      NULL);

  gimp_help_init ();

  /*
   *  Parameter settings
   *
   *  First set up the basic containers, label them, etc.
   */
  frame = gtk_frame_new (_("Parameter Settings"));
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_ETCHED_IN);
  gtk_container_set_border_width (GTK_CONTAINER(frame), 6);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG(dlg)->vbox), frame, TRUE, TRUE, 0);

  table = gtk_table_new (3, 3, FALSE);
  gtk_table_set_col_spacings (GTK_TABLE (table), 4);
  gtk_table_set_row_spacings (GTK_TABLE (table), 2);
  gtk_container_set_border_width (GTK_CONTAINER (table), 4);
  gtk_container_add (GTK_CONTAINER (frame), table);
  gtk_widget_show (table);

  /*  Random Seed  */
  seed_hbox = gimp_random_seed_new (&pivals.blur_seed,
				    &pivals.seed_type,
				    SEED_TIME, SEED_USER);
  gimp_table_attach_aligned (GTK_TABLE (table), 0, 0,
                             _("Random Seed:"), 1.0, 0.5,
                             seed_hbox, 1, TRUE);

  /*
   *  Randomization percentage label & scale (1 to 100)
   */
  adj = gimp_scale_entry_new (GTK_TABLE (table), 0, 1,
			      _("Randomization %:"), SCALE_WIDTH, 0,
			      pivals.blur_pct, 1.0, 100.0, 1.0, 10.0, 0,
			      TRUE, 0, 0,
			      _("Percentage of pixels to be filtered"), NULL);
  gtk_signal_connect (GTK_OBJECT (adj), "value_changed",
		      GTK_SIGNAL_FUNC (gimp_double_adjustment_update),
		      &pivals.blur_pct);

  /*
   *  Repeat count label & scale (1 to 100)
   */
  adj = gimp_scale_entry_new (GTK_TABLE (table), 0, 2,
			      _("Repeat:"), SCALE_WIDTH, 0,
			      pivals.blur_rcount, 1.0, 100.0, 1.0, 10.0, 0,
			      TRUE, 0, 0,
			      _("Number of times to apply filter"), NULL);
  gtk_signal_connect (GTK_OBJECT (adj), "value_changed",
		      GTK_SIGNAL_FUNC (gimp_double_adjustment_update),
		      &pivals.blur_rcount);

  gtk_widget_show (frame);

  gtk_widget_show (dlg);

  gtk_main ();
  gimp_help_free ();
  gdk_flush ();

  return blur_int.run;
}

static void
blur_ok_callback (GtkWidget *widget, 
		  gpointer   data) 
{
  blur_int.run = TRUE;

  gtk_widget_destroy (GTK_WIDGET (data));
}
