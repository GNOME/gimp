/* Spread --- image filter plug-in for The Gimp image manipulation program
 * Copyright (C) 1997 Brian Degenhardt and Federico Mena Quintero
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
 * Please direct all comments, questions, bug reports  etc to Brian Degenhardt
 * bdegenha@ucsd.edu
 *
 * You can contact Federico Mena Quintero at quartic@polloux.fciencias.unam.mx
 * You can contact the original The Gimp authors at gimp@xcf.berkeley.edu
 */
#include "config.h"

#include <time.h>
#include <stdio.h>
#include <stdlib.h>

#include <gtk/gtk.h>

#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

#include "libgimp/stdplugins-intl.h"


#define SCALE_WIDTH     200
#define TILE_CACHE_SIZE  16


typedef struct
{
  gdouble spread_amount_x;
  gdouble spread_amount_y;
} SpreadValues;


/* Declare local functions.
 */
static void      query         (void);
static void      run           (const gchar      *name,
                                gint              nparams,
                                const GimpParam  *param,
                                gint             *nreturn_vals,
                                GimpParam       **return_vals);

static void      spread        (GimpDrawable     *drawable);

static gint      spread_dialog (gint32            image_ID,
                                GimpDrawable     *drawable);


/***** Local vars *****/

GimpPlugInInfo PLUG_IN_INFO =
{
  NULL,  /* init_proc  */
  NULL,  /* quit_proc  */
  query, /* query_proc */
  run,   /* run_proc   */
};

static SpreadValues spvals =
{
  5,  /*  horizontal spread amount  */
  5   /*  vertical spread amount    */
};


/***** Functions *****/

MAIN ()

static void
query (void)
{
  static GimpParamDef args[] =
  {
    { GIMP_PDB_INT32, "run_mode", "Interactive, non-interactive" },
    { GIMP_PDB_IMAGE, "image", "Input image (unused)" },
    { GIMP_PDB_DRAWABLE, "drawable", "Input drawable" },
    { GIMP_PDB_FLOAT, "spread_amount_x", "Horizontal spread amount (0 <= spread_amount_x <= 200)" },
    { GIMP_PDB_FLOAT, "spread_amount_y", "Vertical spread amount (0 <= spread_amount_y <= 200)" }
  };

  gimp_install_procedure ("plug_in_spread",
			  "Spread the contents of the specified drawable",
			  "Spreads the pixels of the specified drawable.  "
			  "Pixels are randomly moved to another location whose "
			  "distance varies from the original by the horizontal "
			  "and vertical spread amounts ",
			  "Spencer Kimball and Peter Mattis, ported by Brian "
			  "Degenhardt and Federico Mena Quintero",
			  "Federico Mena Quintero and Brian Degenhardt",
			  "1997",
			  N_("<Image>/Filters/Noise/Sp_read..."),
			  "RGB*, GRAY*",
			  GIMP_PLUGIN,
			  G_N_ELEMENTS (args), 0,
			  args, NULL);
}

static void
run (const gchar      *name,
     gint              nparams,
     const GimpParam  *param,
     gint             *nreturn_vals,
     GimpParam       **return_vals)
{
  static GimpParam   values[1];
  gint32             image_ID;
  GimpDrawable      *drawable;
  GimpRunMode        run_mode;
  GimpPDBStatusType  status = GIMP_PDB_SUCCESS;

  run_mode = param[0].data.d_int32;

  INIT_I18N ();

  /*  Get the specified image and drawable  */
  image_ID = param[1].data.d_image;
  drawable = gimp_drawable_get (param[2].data.d_drawable);

  *nreturn_vals = 1;
  *return_vals  = values;

  values[0].type          = GIMP_PDB_STATUS;
  values[0].data.d_status = status;

  switch (run_mode)
    {
    case GIMP_RUN_INTERACTIVE:
      /*  Possibly retrieve data  */
      gimp_get_data ("plug_in_spread", &spvals);

      /*  First acquire information with a dialog  */
      if (! spread_dialog (image_ID, drawable))
	return;
      break;

    case GIMP_RUN_NONINTERACTIVE:
      /*  Make sure all the arguments are there!  */
      if (nparams != 5)
	{
	  status = GIMP_PDB_CALLING_ERROR;
	}
      else
	{
	  spvals.spread_amount_x= param[3].data.d_float;
          spvals.spread_amount_y = param[4].data.d_float;
        }

      if ((status == GIMP_PDB_SUCCESS) &&
	  (spvals.spread_amount_x < 0 || spvals.spread_amount_x > 200) &&
          (spvals.spread_amount_y < 0 || spvals.spread_amount_y > 200))
	status = GIMP_PDB_CALLING_ERROR;
      break;

    case GIMP_RUN_WITH_LAST_VALS:
      /*  Possibly retrieve data  */
      gimp_get_data ("plug_in_spread", &spvals);
      break;

    default:
      break;
    }

  if (status == GIMP_PDB_SUCCESS)
    {
      /*  Make sure that the drawable is gray or RGB color  */
      if (gimp_drawable_is_rgb (drawable->drawable_id) ||
	  gimp_drawable_is_gray (drawable->drawable_id))
	{
	  gimp_progress_init (_("Spreading..."));

	  /*  set the tile cache size  */
	  gimp_tile_cache_ntiles (TILE_CACHE_SIZE);

	  /*  run the spread effect  */
	  spread (drawable);

	  if (run_mode != GIMP_RUN_NONINTERACTIVE)
	    gimp_displays_flush ();

	  /*  Store data  */
	  if (run_mode == GIMP_RUN_INTERACTIVE)
	    gimp_set_data ("plug_in_spread", &spvals, sizeof (SpreadValues));
	}
      else
	{
	  /* gimp_message ("spread: cannot operate on indexed color images"); */
	  status = GIMP_PDB_EXECUTION_ERROR;
	}
    }

  values[0].data.d_status = status;

  gimp_drawable_detach (drawable);
}

static void
spread (GimpDrawable *drawable)
{
  GimpPixelRgn  dest_rgn;
  gpointer      pr;
  GimpPixelFetcher *pft;

  gint    width, height;
  gint    bytes;
  guchar *destrow;
  guchar *dest;
  gint    x1, y1, x2, y2;
  gint    x, y;
  gint    progress, max_progress;
  gdouble x_amount, y_amount;
  gdouble angle;
  gint xdist, ydist;
  gint xi, yi;

  GRand *gr;

  gr = g_rand_new ();

  pft = gimp_pixel_fetcher_new (drawable);

  gimp_drawable_mask_bounds (drawable->drawable_id, &x1, &y1, &x2, &y2);

  width  = drawable->width;
  height = drawable->height;
  bytes  = drawable->bpp;

  progress     = 0;
  max_progress = (x2 - x1) * (y2 - y1);

  x_amount = spvals.spread_amount_x;
  y_amount = spvals.spread_amount_y;

  /* Spread the image.  This is done by going through every pixel
     in the source image and swapping it with some other random
     pixel.  The random pixel is located within an ellipse that is
     as high as the spread_amount_y parameter and as wide as the
     spread_amount_x parameter.  This is done by randomly selecting
     an angle and then multiplying the sine of the angle to a random
     number whose range is between -spread_amount_x/2 and spread_amount_x/2.
     The y coordinate is found by multiplying the cosine of the angle
     to the random value generated from spread_amount_y.  The reason
     that the spread is done this way is to make the end product more
     random looking.  To see a result of this, compare spreading a
     square with gimp 0.54 to spreading a square with this filter.
     The corners are less sharp with this algorithm.
  */

  /* Spread the image! */

  gimp_pixel_rgn_init (&dest_rgn, drawable,
		       x1, y1, (x2 - x1), (y2 - y1), TRUE, TRUE);
  for (pr = gimp_pixel_rgns_register (1, &dest_rgn);
       pr != NULL;
       pr = gimp_pixel_rgns_process (pr))
    {
      destrow = dest_rgn.data;

      for (y = dest_rgn.y; y < (dest_rgn.y + dest_rgn.h); y++)
	{
	  dest = destrow;

	  for (x = dest_rgn.x; x < (dest_rgn.x + dest_rgn.w); x++)
	    {
              /* get random angle, x distance, and y distance */
              xdist = g_rand_int_range (gr, -(x_amount + 1) / 2, 
					(x_amount + 1) / 2);
              ydist = g_rand_int_range (gr, -(y_amount + 1)/2, 
					(y_amount + 1) / 2);
              angle = g_rand_double_range (gr, -G_PI, G_PI);

              xi = x + floor(sin(angle) * xdist);
              yi = y + floor(cos(angle) * ydist);

              /* Only displace the pixel if it's within the bounds of the 
		 image. */
              if ((xi >= 0) && (xi < width) && (yi >= 0) && (yi < height))
		{
		  gimp_pixel_fetcher_get_pixel (pft, xi, yi, dest);
		}
	      else /* Else just copy it */
		{
		  gimp_pixel_fetcher_get_pixel (pft, x, y, dest);
		}
	      dest += bytes;
            }
	  destrow += dest_rgn.rowstride;
	}
      progress += dest_rgn.w * dest_rgn.h;
      gimp_progress_update ((double) progress / (double) max_progress);
    }

  gimp_pixel_fetcher_destroy (pft);

  /*  update the region  */
  gimp_drawable_flush (drawable);
  gimp_drawable_merge_shadow (drawable->drawable_id, TRUE);
  gimp_drawable_update (drawable->drawable_id, x1, y1, (x2 - x1), (y2 - y1));
}

static gint
spread_dialog (gint32        image_ID,
	       GimpDrawable *drawable)
{
  GtkWidget *dlg;
  GtkWidget *frame;
  GtkWidget *size;
  GimpUnit   unit;
  gdouble    xres;
  gdouble    yres;
  gboolean   run;

  gimp_ui_init ("spread", FALSE);

  dlg = gimp_dialog_new (_("Spread"), "spread",
                         NULL, 0,
			 gimp_standard_help_func, "plug-in-spread",

                         GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                         GTK_STOCK_OK,     GTK_RESPONSE_OK,

                         NULL);

  /*  parameter settings  */
  frame = gtk_frame_new (_("Spread Amount"));
  gtk_container_set_border_width (GTK_CONTAINER (frame), 6);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dlg)->vbox), frame, TRUE, TRUE, 0);
  gtk_widget_show (frame);

  /*  Get the image resolution and unit  */
  gimp_image_get_resolution (image_ID, &xres, &yres);
  unit = gimp_image_get_unit (image_ID);

  /* sizeentries */
  size = gimp_coordinates_new (unit, "%a", TRUE, FALSE, -1,
			       GIMP_SIZE_ENTRY_UPDATE_SIZE,

			       spvals.spread_amount_x == spvals.spread_amount_y,
			       FALSE,

			       _("_Horizontal:"), spvals.spread_amount_x, xres,
			       0, MAX (drawable->width, drawable->height),
			       0, 0,

			       _("_Vertical:"), spvals.spread_amount_y, yres,
			       0, MAX (drawable->width, drawable->height),
			       0, 0);
  gtk_container_set_border_width (GTK_CONTAINER (size), 4);
  gtk_container_add (GTK_CONTAINER (frame), size);
  gtk_widget_show (size);

  gtk_widget_show (dlg);

  run = (gimp_dialog_run (GIMP_DIALOG (dlg)) == GTK_RESPONSE_OK);

  if (run)
    {
      spvals.spread_amount_x =
        gimp_size_entry_get_refval (GIMP_SIZE_ENTRY (size), 0);
      spvals.spread_amount_y =
        gimp_size_entry_get_refval (GIMP_SIZE_ENTRY (size), 1);
    }

  gtk_widget_destroy (dlg);

  return run;
}
