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

/* Some useful macros */
#define SCALE_WIDTH 200
#define TILE_CACHE_SIZE 16
#define ENTRY_WIDTH 50

typedef struct
{
  gdouble spread_amount_x;
  gdouble spread_amount_y;
} SpreadValues;

typedef struct
{
  GtkWidget *size;
  gint       run;
} SpreadInterface;


/* Declare local functions.
 */
static void      query  (void);
static void      run    (gchar    *name,
			 gint      nparams,
			 GParam   *param,
			 gint     *nreturn_vals,
			 GParam  **return_vals);

static void      spread (GDrawable *drawable);

static gint      spread_dialog          (gint32     image_ID,
					 GDrawable *drawable);
static void      spread_ok_callback     (GtkWidget *widget,
				         gpointer   data);

static GTile *   spread_pixel (GDrawable *drawable,
			       GTile     *tile,
			       gint       x1,
			       gint       y1,
			       gint       x2,
			       gint       y2,
			       gint       x,
			       gint       y,
			       gint      *row,
			       gint      *col,
			       guchar    *pixel);


/***** Local vars *****/

GPlugInInfo PLUG_IN_INFO =
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

static SpreadInterface pint =
{
  FALSE   /*  run  */
};

/***** Functions *****/

MAIN ()

static void
query (void)
{
  static GParamDef args[] =
  {
    { PARAM_INT32, "run_mode", "Interactive, non-interactive" },
    { PARAM_IMAGE, "image", "Input image (unused)" },
    { PARAM_DRAWABLE, "drawable", "Input drawable" },
    { PARAM_FLOAT, "spread_amount_x", "Horizontal spread amount (0 <= spread_amount_x <= 200)" },
    { PARAM_FLOAT, "spread_amount_y", "Vertical spread amount (0 <= spread_amount_y <= 200)" }
  };
  static GParamDef *return_vals = NULL;
  static gint nargs = sizeof (args) / sizeof (args[0]);
  static gint nreturn_vals = 0;

  INIT_I18N();

  gimp_install_procedure ("plug_in_spread",
			  "Spread the contents of the specified drawable",
			  "Spreads the pixels of the specified drawable.  Pixels are randomly moved to another location whose distance varies from the original by the horizontal and vertical spread amounts ",
			  "Spencer Kimball and Peter Mattis, ported by Brian Degenhardt and Federico Mena Quintero",
			  "Federico Mena Quintero and Brian Degenhardt",
			  "1997",
			  N_("<Image>/Filters/Noise/Spread..."),
			  "RGB*, GRAY*",
			  PROC_PLUG_IN,
			  nargs, nreturn_vals,
			  args, return_vals);
}

static void
run (gchar  *name,
     gint    nparams,
     GParam  *param,
     gint   *nreturn_vals,
     GParam **return_vals)
{
  static GParam values[1];
  gint32 image_ID;
  GDrawable *drawable;
  GRunModeType run_mode;
  GStatusType status = STATUS_SUCCESS;

  run_mode = param[0].data.d_int32;

  /*  Get the specified image and drawable  */
  image_ID = param[1].data.d_image;
  drawable = gimp_drawable_get (param[2].data.d_drawable);

  *nreturn_vals = 1;
  *return_vals = values;

  values[0].type = PARAM_STATUS;
  values[0].data.d_status = status;

  switch (run_mode)
    {
    case RUN_INTERACTIVE:
      INIT_I18N_UI();
      /*  Possibly retrieve data  */
      gimp_get_data ("plug_in_spread", &spvals);

      /*  First acquire information with a dialog  */
      if (! spread_dialog (image_ID, drawable))
	return;
      break;

    case RUN_NONINTERACTIVE:
      INIT_I18N();
      /*  Make sure all the arguments are there!  */
      if (nparams != 5)
	status = STATUS_CALLING_ERROR;
      if (status == STATUS_SUCCESS)
	{
	  spvals.spread_amount_x= param[3].data.d_float;
          spvals.spread_amount_y = param[4].data.d_float;
        }
      if ((status == STATUS_SUCCESS) &&
	  (spvals.spread_amount_x < 0 || spvals.spread_amount_x > 200) &&
          (spvals.spread_amount_y < 0 || spvals.spread_amount_y > 200))
	status = STATUS_CALLING_ERROR;
      break;

    case RUN_WITH_LAST_VALS:
      INIT_I18N();
      /*  Possibly retrieve data  */
      gimp_get_data ("plug_in_spread", &spvals);
      break;

    default:
      break;
    }

  if (status == STATUS_SUCCESS)
    {
      /*  Make sure that the drawable is gray or RGB color  */
      if (gimp_drawable_is_rgb (drawable->id) || gimp_drawable_is_gray (drawable->id))
	{
	  gimp_progress_init (_("Spreading..."));

	  /*  set the tile cache size  */
	  gimp_tile_cache_ntiles (TILE_CACHE_SIZE);

	  /*  run the spread effect  */
	  spread (drawable);

	  if (run_mode != RUN_NONINTERACTIVE)
	    gimp_displays_flush ();

	  /*  Store data  */
	  if (run_mode == RUN_INTERACTIVE)
	    gimp_set_data ("plug_in_spread", &spvals, sizeof (SpreadValues));
	}
      else
	{
	  /* gimp_message ("spread: cannot operate on indexed color images"); */
	  status = STATUS_EXECUTION_ERROR;
	}
    }

  values[0].data.d_status = status;

  gimp_drawable_detach (drawable);
}

static void
spread (GDrawable *drawable)
{
  GPixelRgn dest_rgn;
  GTile   * tile = NULL;
  gint      row = -1;
  gint      col = -1;
  gpointer  pr;

  gint    width, height;
  gint    bytes;
  guchar *destrow;
  guchar *dest;
  guchar  pixel[4][4];
  gint    x1, y1, x2, y2;
  gint    x, y;
  gint    progress, max_progress;

  gdouble x_amount, y_amount;
  gdouble angle;

  gint xdist, ydist;
  gint xi, yi;

  gint k;
  gint x_mod_value, x_sub_value;
  gint y_mod_value, y_sub_value;
  gint angle_mod_value, angle_sub_value;

  /* Get selection area */

  gimp_drawable_mask_bounds (drawable->id, &x1, &y1, &x2, &y2);

  width  = drawable->width;
  height = drawable->height;
  bytes  = drawable->bpp;

  progress     = 0;
  max_progress = (x2 - x1) * (y2 - y1);

  x_amount = spvals.spread_amount_x;
  y_amount = spvals.spread_amount_y;

  /* Initialize random stuff */
  srand (time (NULL));
  angle_mod_value = G_PI*2;
  angle_sub_value = angle_mod_value / 2;
  x_mod_value = x_amount + 1;
  x_sub_value = x_mod_value / 2;
  y_mod_value = y_amount + 1;
  y_sub_value = y_mod_value / 2;

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
              xdist = (rand () % x_mod_value) - x_sub_value;
              ydist = (rand () % y_mod_value) - y_sub_value;
              angle = (rand () % angle_mod_value) - angle_sub_value;

              xi = x + floor(sin(angle)*xdist);
              yi = y + floor(cos(angle)*ydist);

              /* Only displace the pixel if it's within the bounds of the image. */
              if ((xi >= 0) && (xi < width) && (yi >= 0) && (yi < height))
                  tile = spread_pixel (drawable, tile, x1, y1, x2, y2, xi, yi, &row, &col, pixel[0]);
	      else
              {
              /* Else just copy it */
                  tile = spread_pixel (drawable, tile, x1, y1, x2, y2, x, y, &row, &col, pixel[0]);
              }

              for (k = 0; k < bytes; k++)
                  *dest++ = pixel[0][k];
            } /* for */

	  destrow += dest_rgn.rowstride;;
	} /* for */

      progress += dest_rgn.w * dest_rgn.h;
      gimp_progress_update ((double) progress / (double) max_progress);
    }  /* for  */

  if (tile)
    gimp_tile_unref (tile, FALSE);

  /*  update the region  */
  gimp_drawable_flush (drawable);
  gimp_drawable_merge_shadow (drawable->id, TRUE);
  gimp_drawable_update (drawable->id, x1, y1, (x2 - x1), (y2 - y1));
} /* spread */


static gint
spread_dialog (gint32     image_ID,
	       GDrawable *drawable)
{
  GtkWidget *dlg;
  GtkWidget *frame;
  GtkWidget *size;
  GimpUnit   unit;
  gdouble    xres;
  gdouble    yres;
  gchar **argv;
  gint    argc;

  argc    = 1;
  argv    = g_new (gchar *, 1);
  argv[0] = g_strdup ("spread");

  gtk_init (&argc, &argv);
  gtk_rc_parse (gimp_gtkrc ());

  dlg = gimp_dialog_new (_("Spread"), "spread",
			 gimp_plugin_help_func, "filters/spread.html",
			 GTK_WIN_POS_MOUSE,
			 FALSE, TRUE, FALSE,

			 _("OK"), spread_ok_callback,
			 NULL, NULL, NULL, TRUE, FALSE,
			 _("Cancel"), gtk_widget_destroy,
			 NULL, 1, NULL, FALSE, TRUE,

			 NULL);

  gtk_signal_connect (GTK_OBJECT (dlg), "destroy",
		      GTK_SIGNAL_FUNC (gtk_main_quit),
		      NULL);

  /*  parameter settings  */
  frame = gtk_frame_new (_("Spread Amount"));
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_ETCHED_IN);
  gtk_container_set_border_width (GTK_CONTAINER (frame), 6);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dlg)->vbox), frame, TRUE, TRUE, 0);

  /*  Get the image resolution and unit  */
  gimp_image_get_resolution (image_ID, &xres, &yres);
  unit = gimp_image_get_unit (image_ID);

  /* sizeentries */
  size = gimp_coordinates_new (unit, "%a", TRUE, FALSE, 75, 
			       GIMP_SIZE_ENTRY_UPDATE_SIZE,

			       spvals.spread_amount_x == spvals.spread_amount_y,
			       FALSE,

			       _("Horizontal:"), spvals.spread_amount_x, xres,
			       0, MAX (drawable->width, drawable->height),
			       0, 0,

			       _("Vertical:"), spvals.spread_amount_y, yres,
			       0, MAX (drawable->width, drawable->height),
			       0, 0);
  gtk_container_set_border_width (GTK_CONTAINER (size), 4);
  gtk_container_add (GTK_CONTAINER (frame), size);

  pint.size = size;

  gtk_widget_show (size);
  gtk_widget_show (frame);

  gtk_widget_show (dlg);

  gtk_main ();
  gdk_flush ();

  return pint.run;
}

static void
spread_ok_callback (GtkWidget *widget,
		    gpointer   data)
{
  spvals.spread_amount_x =
    gimp_size_entry_get_refval (GIMP_SIZE_ENTRY (pint.size), 0);
  spvals.spread_amount_y =
    gimp_size_entry_get_refval (GIMP_SIZE_ENTRY (pint.size), 1);

  pint.run = TRUE;

  gtk_widget_destroy (GTK_WIDGET (data));
}

static GTile *
spread_pixel (GDrawable * drawable,
	      GTile *     tile,
	      gint        x1,
	      gint        y1,
	      gint        x2,
	      gint        y2,
	      gint        x,
	      gint        y,
	      gint *      row,
	      gint *      col,
	      guchar *    pixel)
{
  static guchar empty_pixel[4] = {0, 0, 0, 0};
  guchar *data;
  gint b;

  if (x >= x1 && y >= y1 && x < x2 && y < y2)
    {
      if ((x >> 6 != *col) || (y >> 6 != *row))
	{
	  *col = x >> 6;
	  *row = y >> 6;
	  if (tile)
	    gimp_tile_unref (tile, FALSE);
	  tile = gimp_drawable_get_tile (drawable, FALSE, *row, *col);
	  gimp_tile_ref (tile);
	}

      data = tile->data + tile->bpp * (tile->ewidth * (y % 64) + (x % 64));
    }
  else
    data = empty_pixel;

  for (b = 0; b < drawable->bpp; b++)
    pixel[b] = data[b];

  return tile;
}
