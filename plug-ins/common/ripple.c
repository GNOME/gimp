/* Ripple --- image filter plug-in for The Gimp image manipulation program
 * Copyright (C) 1997 Brian Degenhardt
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
#define SCALE_WIDTH     200
#define TILE_CACHE_SIZE  16

#define HORIZONTAL 0
#define VERTICAL   1

#define SMEAR 0
#define WRAP  1
#define BLACK 2

#define SAWTOOTH 0
#define SINE     1

typedef struct
{
  gint period;
  gint amplitude;
  gint orientation;
  gint edges;
  gint waveform;
  gint antialias;
  gint tile;
} RippleValues;

typedef struct
{
  gint run;
} RippleInterface;


/* Declare local functions.
 */
static void    query  (void);
static void    run    (gchar    *name,
		       gint      nparams,
		       GParam   *param,
		       gint     *nreturn_vals,
		       GParam  **return_vals);

static void    ripple             (GDrawable *drawable);

static gint    ripple_dialog      (void);
static void    ripple_ok_callback (GtkWidget *widget,
				   gpointer   data);

static GTile * ripple_pixel (GDrawable *drawable,
			     GTile     *tile,
			     gint        x1,
			     gint        y1,
			     gint        x2,
			     gint        y2,
			     gint        x,
			     gint        y,
			     gint       *row,
			     gint       *col,
			     guchar     *pixel);


static gdouble displace_amount (gint     location);
static guchar  averagetwo      (gdouble  location,
				guchar  *v);
static guchar  averagefour     (gdouble  location,
				guchar  *v);

/***** Local vars *****/

GPlugInInfo PLUG_IN_INFO =
{
  NULL,  /* init_proc  */
  NULL,  /* quit_proc  */
  query, /* query_proc */
  run,   /* run_proc   */
};

static RippleValues rvals =
{
  20,         /* period      */
  5,          /* amplitude   */
  HORIZONTAL, /* orientation */
  WRAP,       /* edges       */
  SINE,       /* waveform    */
  TRUE,       /* antialias   */
  TRUE        /* tile        */
};

static RippleInterface rpint =
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
    { PARAM_INT32, "period", "period; number of pixels for one wave to complete" },
    { PARAM_INT32, "amplitude", "amplitude; maximum displacement of wave" },
    { PARAM_INT32, "orientation", "orientation; 0 = Horizontal, 1 = Vertical" },
    { PARAM_INT32, "edges", "edges; 0 = smear, 1 =  wrap, 2 = black" },
    { PARAM_INT32, "waveform", "0 = sawtooth, 1 = sine wave" },
    { PARAM_INT32, "antialias", "antialias; True or False" },
    { PARAM_INT32, "tile", "tile; if this is true, the image will retain it's tilability" }
  };
  static gint nargs = sizeof (args) / sizeof (args[0]);

  gimp_install_procedure ("plug_in_ripple",
			  "Ripple the contents of the specified drawable",
			  "Ripples the pixels of the specified drawable. Each row or column will be displaced a certain number of pixels coinciding with the given wave form",
			  "Brian Degenhardt <bdegenha@ucsd.edu>",
			  "Brian Degenhardt",
			  "1997",
			  N_("<Image>/Filters/Distorts/Ripple..."),
			  "RGB*, GRAY*",
			  PROC_PLUG_IN,
			  nargs, 0,
			  args, NULL);
}

static void
run (gchar  *name,
     gint    nparams,
     GParam  *param,
     gint   *nreturn_vals,
     GParam **return_vals)
{
  static GParam values[1];
  GDrawable *drawable;
  GRunModeType run_mode;
  GStatusType status = STATUS_SUCCESS;

  run_mode = param[0].data.d_int32;

  /*  Get the specified drawable  */
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
      gimp_get_data ("plug_in_ripple", &rvals);

      /*  First acquire information with a dialog  */
      if (! ripple_dialog ())
	return;
      break;

    case RUN_NONINTERACTIVE:
      INIT_I18N();
      /*  Make sure all the arguments are there!  */
      if (nparams != 10)
	{
	  status = STATUS_CALLING_ERROR;
	}
      else
	{
	  rvals.period = param[3].data.d_int32;
	  rvals.amplitude = param[4].data.d_int32;
          rvals.orientation = (param[5].data.d_int32) ? VERTICAL : HORIZONTAL;
          rvals.edges = (param[6].data.d_int32);
          rvals.waveform = param[7].data.d_int32;
          rvals.antialias = (param[8].data.d_int32) ? TRUE : FALSE;
          rvals.tile = (param[9].data.d_int32) ? TRUE : FALSE;

	  if (rvals.edges < SMEAR || rvals.edges > BLACK)
	    status = STATUS_CALLING_ERROR;
	}
      break;

    case RUN_WITH_LAST_VALS:
      INIT_I18N();
      /*  Possibly retrieve data  */
      gimp_get_data ("plug_in_ripple", &rvals);
      break;

    default:
      break;
    }

  if (status == STATUS_SUCCESS)
    {
      /*  Make sure that the drawable is gray or RGB color  */
      if (gimp_drawable_is_rgb (drawable->id) ||
	  gimp_drawable_is_gray (drawable->id))
	{
	  gimp_progress_init ( _("Rippling..."));

	  /*  set the tile cache size  */
	  gimp_tile_cache_ntiles (TILE_CACHE_SIZE);

	  /*  run the ripple effect  */
	  ripple (drawable);

	  if (run_mode != RUN_NONINTERACTIVE)
	    gimp_displays_flush ();

	  /*  Store data  */
	  if (run_mode == RUN_INTERACTIVE)
	    gimp_set_data ("plug_in_ripple", &rvals, sizeof (RippleValues));
	}
      else
	{
	  /* gimp_message ("ripple: cannot operate on indexed color images"); */
	  status = STATUS_EXECUTION_ERROR;
	}
    }

  values[0].data.d_status = status;

  gimp_drawable_detach (drawable);
}

static void
ripple (GDrawable *drawable)
{
  GPixelRgn dest_rgn;
  GTile   * tile = NULL;
  gint      row = -1;
  gint      col = -1;
  gpointer  pr;

  gint    width, height;
  gint    bytes;
  guchar *destline;
  guchar *dest;
  guchar *otherdest;
  guchar  pixel[4][4];
  gint    x1, y1, x2, y2;
  gint    x, y;
  gint    progress, max_progress;
  gdouble needx, needy;

  guchar  values[4];
  guchar  val;

  gint xi, yi;

  gint k;

  /* Get selection area */

  gimp_drawable_mask_bounds (drawable->id, &x1, &y1, &x2, &y2);

  width  = drawable->width;
  height = drawable->height;
  bytes  = drawable->bpp;

  if ( rvals.tile )
    {
      rvals.edges = WRAP;
      rvals.period = (width / (width / rvals.period) *
		      (rvals.orientation == HORIZONTAL) +
		      height / (height / rvals.period) *
		      (rvals.orientation == VERTICAL));
    }

  progress     = 0;
  max_progress = (x2 - x1) * (y2 - y1);

  /* Ripple the image.  It's a pretty simple algorithm.  If horizontal
     is selected, then every row is displaced a number of pixels that
     follows the pattern of the waveform selected.  The effect is
     just reproduced with columns if vertical is selected.
  */

  gimp_pixel_rgn_init (&dest_rgn, drawable,
		       x1, y1, (x2 - x1), (y2 - y1), TRUE, TRUE);
  for (pr = gimp_pixel_rgns_register (1, &dest_rgn);
       pr != NULL;
       pr = gimp_pixel_rgns_process (pr))
    {
      if (rvals.orientation == VERTICAL)
	{
          destline = dest_rgn.data;

          for (x = dest_rgn.x; x < (dest_rgn.x + dest_rgn.w); x++)
	    {
              dest = destline;

              for (y = dest_rgn.y; y < (dest_rgn.y + dest_rgn.h); y++)
		{
                  otherdest = dest;

                  needy = y + displace_amount(x);
                  yi = floor(needy);

		  /* Tile the image. */
                  if (rvals.edges == WRAP)
		    {
                      needy = fmod(needy + height, height);
                      yi = (yi + height) % height;
		    }
		  /* Smear out the edges of the image by repeating pixels. */
                  else if (rvals.edges == SMEAR)
		    {
                      if (yi < 0)
			yi = 0;
                      else if (yi > height - 1)
			yi = height - 1;
		    }

                  if ( rvals.antialias)
		    {
                      if (yi == height - 1)
			{
                          tile = ripple_pixel (drawable, tile,
					       x1, y1, x2, y2,
					       x, yi, &row, &col, pixel[0]);
			  
                          for (k = 0; k < bytes; k++)
			    *otherdest++ = pixel[0][k];
			}
                      else if (needy < 0 && needy > -1)
			{
                          tile = ripple_pixel (drawable, tile,
					       x1, y1, x2, y2,
					       x, 0, &row, &col, pixel[0]);

                          for (k = 0; k < bytes; k++)
			    *otherdest++ = pixel[0][k];
			}

                      else if (yi == height - 2 || yi == 0)
			{
                          tile = ripple_pixel (drawable, tile,
					       x1, y1, x2, y2,
					       x, yi, &row, &col, pixel[0]);
                          tile = ripple_pixel (drawable, tile,
					       x1, y1, x2, y2,
					       x, yi + 1, &row, &col, pixel[1]);

                          for (k = 0; k < bytes; k++)
			    {
                              values[0] = pixel[0][k];
                              values[1] = pixel[1][k];
                              val = averagetwo(needy, values);

                              *otherdest++ = val;
			    }
			}
                      else
			{
                          tile = ripple_pixel (drawable, tile,
					       x1, y1, x2, y2,
					       x, yi, &row, &col, pixel[0]);
                          tile = ripple_pixel (drawable, tile,
					       x1, y1, x2, y2,
					       x, yi + 1, &row, &col, pixel[1]);
                          tile = ripple_pixel (drawable, tile,
					       x1, y1, x2, y2,
					       x, yi - 1, &row, &col, pixel[2]);
                          tile = ripple_pixel (drawable, tile,
					       x1, y1, x2, y2,
					       x, yi + 2, &row, &col, pixel[3]);

                          for (k = 0; k < bytes; k++)
			    {
                              values[0] = pixel[0][k];
                              values[1] = pixel[1][k];
                              values[2] = pixel[2][k];
                              values[3] = pixel[3][k];

                              val = averagefour (needy, values);

                              *otherdest++ = val;
			    }
                      }
		    } /* antialias */
                  else
		    {
                      tile = ripple_pixel (drawable, tile,
					   x1, y1, x2, y2,
					   x, yi, &row, &col, pixel[0]);

                      for (k = 0; k < bytes; k++)
			*otherdest++ = pixel[0][k];
		    }
                  dest += dest_rgn.rowstride;
		} /* for */

              for (k = 0; k < bytes; k++)
		destline++;
	    } /* for */

          progress += dest_rgn.w * dest_rgn.h;
          gimp_progress_update ((double) progress / (double) max_progress);
	}
      else /* HORIZONTAL */
	{
          destline = dest_rgn.data;

          for (y = dest_rgn.y; y < (dest_rgn.y + dest_rgn.h); y++)
	    {
              dest = destline;

              for (x = dest_rgn.x; x < (dest_rgn.x + dest_rgn.w); x++)
		{
                  needx = x + displace_amount(y);
                  xi = floor (needx);

		  /* Tile the image. */
                  if (rvals.edges == WRAP)
		    {
                      needx = fmod((needx + width), width);
                      xi = (xi + width) % width;
		    }
		  /* Smear out the edges of the image by repeating pixels. */
                  else if (rvals.edges == SMEAR)
		    {
                      if (xi < 0)
			xi = 0;
                      else if (xi > width - 1)
			xi = width - 1;
		    }

                  if ( rvals.antialias)
		    {
                      if (xi == width - 1)
			{
                          tile = ripple_pixel (drawable, tile,
					       x1, y1, x2, y2,
					       xi, y, &row, &col, pixel[0]);

                          for (k = 0; k < bytes; k++)
			    *dest++ = pixel[0][k];
			}
                      else if (floor(needx) ==  -1)
			{
                          tile = ripple_pixel (drawable, tile,
					       x1, y1, x2, y2,
					       0, y, &row, &col, pixel[0]);

                          for (k = 0; k < bytes; k++)
			    *dest++ = pixel[0][k];
			}

                      else if (xi == width - 2 || xi == 0)
			{
                          tile = ripple_pixel (drawable, tile,
					       x1, y1, x2, y2,
					       xi, y, &row, &col, pixel[0]);
                          tile = ripple_pixel (drawable, tile,
					       x1, y1, x2, y2,
					       xi + 1, y, &row, &col, pixel[1]);

                          for (k = 0; k < bytes; k++)
			    {
                              values[0] = pixel[0][k];
                              values[1] = pixel[1][k];
                              val = averagetwo (needx, values);

                              *dest++ = val;
			    }
			}
                      else
			{
                          tile = ripple_pixel (drawable, tile,
					       x1, y1, x2, y2,
					       xi, y, &row, &col, pixel[0]);
                          tile = ripple_pixel (drawable, tile,
					       x1, y1, x2, y2,
					       xi + 1, y, &row, &col, pixel[1]);
                          tile = ripple_pixel (drawable, tile,
					       x1, y1, x2, y2,
					       xi - 1 , y, &row, &col, pixel[2]);
                          tile = ripple_pixel (drawable, tile,
					       x1, y1, x2, y2,
					       xi + 2, y, &row, &col, pixel[3]);

                          for (k = 0; k < bytes; k++)
			    {
                              values[0] = pixel[0][k];
                              values[1] = pixel[1][k];
                              values[2] = pixel[2][k];
                              values[3] = pixel[3][k];

                              val = averagefour (needx, values);

                              *dest++ = val;
			    }
			}
		    } /* antialias */

                  else
		    {
                      tile = ripple_pixel (drawable, tile,
					   x1, y1, x2, y2,
					   xi, y, &row, &col, pixel[0]);

                      for (k = 0; k < bytes; k++)
			*dest++ = pixel[0][k];
		    }
		} /* for */

              destline += dest_rgn.rowstride;
	    } /* for */

          progress += dest_rgn.w * dest_rgn.h;
          gimp_progress_update ((double) progress / (double) max_progress);
	}
    }  /* for  */

  if (tile)
    gimp_tile_unref (tile, FALSE);

  /*  update the region  */
  gimp_drawable_flush (drawable);
  gimp_drawable_merge_shadow (drawable->id, TRUE);
  gimp_drawable_update (drawable->id, x1, y1, (x2 - x1), (y2 - y1));
} /* ripple */

static gint
ripple_dialog (void)
{
  GtkWidget *dlg;
  GtkWidget *toggle;
  GtkWidget *toggle_vbox;
  GtkWidget *main_vbox;
  GtkWidget *frame;
  GtkWidget *table;
  GtkObject *scale_data;

  gimp_ui_init ("ripple", TRUE);

  dlg = gimp_dialog_new (_("Ripple"), "ripple",
			 gimp_plugin_help_func, "filters/ripple.html",
			 GTK_WIN_POS_MOUSE,
			 FALSE, TRUE, FALSE,

			 _("OK"), ripple_ok_callback,
			 NULL, NULL, NULL, TRUE, FALSE,
			 _("Cancel"), gtk_widget_destroy,
			 NULL, 1, NULL, FALSE, TRUE,

			 NULL);

  gtk_signal_connect (GTK_OBJECT (dlg), "destroy",
		      GTK_SIGNAL_FUNC (gtk_main_quit),
		      NULL);

  /*  The main vbox  */
  main_vbox = gtk_vbox_new (FALSE, 6);
  gtk_container_set_border_width (GTK_CONTAINER (main_vbox), 6);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dlg)->vbox), main_vbox,
		      FALSE, FALSE, 0);

  /* The table to hold the four frames of options */
  table = gtk_table_new (2, 2, FALSE);
  gtk_table_set_col_spacings (GTK_TABLE (table), 6);
  gtk_table_set_row_spacings (GTK_TABLE (table), 6);
  gtk_box_pack_start (GTK_BOX (main_vbox), table, FALSE, FALSE, 0);

  /*  Options section  */
  frame = gtk_frame_new ( _("Options"));
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_ETCHED_IN);
  gtk_table_attach (GTK_TABLE (table), frame, 0, 1, 0, 1,
		    GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL, 0, 0);

  toggle_vbox = gtk_vbox_new (FALSE, 1);
  gtk_container_set_border_width (GTK_CONTAINER (toggle_vbox), 2);
  gtk_container_add (GTK_CONTAINER (frame), toggle_vbox);

  toggle = gtk_check_button_new_with_label (_("Antialiasing"));
  gtk_box_pack_start (GTK_BOX (toggle_vbox), toggle, FALSE, FALSE, 0);
  gtk_signal_connect (GTK_OBJECT (toggle), "toggled",
		      GTK_SIGNAL_FUNC (gimp_toggle_button_update),
		      &rvals.antialias);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (toggle), rvals.antialias);
  gtk_widget_show (toggle);

  toggle = gtk_check_button_new_with_label ( _("Retain Tilability"));
  gtk_box_pack_start (GTK_BOX (toggle_vbox), toggle, FALSE, FALSE, 0);
  gtk_signal_connect (GTK_OBJECT (toggle), "toggled",
		      GTK_SIGNAL_FUNC (gimp_toggle_button_update),
		      &rvals.tile);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (toggle), (rvals.tile));
  gtk_widget_show (toggle);

  gtk_widget_show (toggle_vbox);
  gtk_widget_show (frame);

  /*  Orientation toggle box  */
  frame =
    gimp_radio_group_new2 (TRUE, _("Orientation"),
			   gimp_radio_button_update,
			   &rvals.orientation, (gpointer) rvals.orientation,

			   _("Horizontal"), (gpointer) HORIZONTAL, NULL,
			   _("Vertical"),   (gpointer) VERTICAL, NULL,

			   NULL);
  gtk_table_attach (GTK_TABLE (table), frame, 1, 2, 0, 1,
		    GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL, 0, 0);
  gtk_widget_show (frame);

  /*  Edges toggle box  */
  frame = gimp_radio_group_new2 (TRUE, _("Edges"),
				 gimp_radio_button_update,
				 &rvals.edges, (gpointer) rvals.edges,

				 _("Wrap"),  (gpointer) WRAP, NULL,
				 _("Smear"), (gpointer) SMEAR, NULL,
				 _("Black"), (gpointer) BLACK, NULL,

				 NULL);
  gtk_table_attach (GTK_TABLE (table), frame, 0, 1, 1, 2,
		    GTK_FILL | GTK_EXPAND, GTK_FILL | GTK_EXPAND, 0, 0);
  gtk_widget_show (frame);

  /*  Wave toggle box  */
  frame = gimp_radio_group_new2 (TRUE, _("Wave Type"),
				 gimp_radio_button_update,
				 &rvals.waveform, (gpointer) rvals.waveform,

				 _("Sawtooth"), (gpointer) SAWTOOTH, NULL,
				 _("Sine"),     (gpointer) SINE, NULL,

				 NULL);
  gtk_table_attach (GTK_TABLE (table), frame, 1, 2, 1, 2,
		    GTK_FILL | GTK_EXPAND, GTK_FILL | GTK_EXPAND, 0, 0);
  gtk_widget_show (frame);

  gtk_widget_show (table);

  /*  Parameter Settings  */
  frame = gtk_frame_new ( _("Parameter Settings"));
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_ETCHED_IN);
  gtk_box_pack_start (GTK_BOX (main_vbox), frame, FALSE, FALSE, 0);

  table = gtk_table_new (2, 3, FALSE);
  gtk_table_set_col_spacings (GTK_TABLE (table), 4);
  gtk_table_set_row_spacings (GTK_TABLE (table), 2);
  gtk_container_set_border_width (GTK_CONTAINER (table), 4);
  gtk_container_add (GTK_CONTAINER (frame), table);

  /*  Period  */
  scale_data = gimp_scale_entry_new (GTK_TABLE (table), 0, 0,
				     _("Period:"), SCALE_WIDTH, 0,
				     rvals.period, 0, 200, 1, 10, 0,
				     TRUE, 0, 0,
				     NULL, NULL);
  gtk_signal_connect (GTK_OBJECT (scale_data), "value_changed",
		      GTK_SIGNAL_FUNC (gimp_int_adjustment_update),
		      &rvals.period);

  /*  Amplitude  */
  scale_data = gimp_scale_entry_new (GTK_TABLE (table), 0, 1,
				     _("Amplitude:"), SCALE_WIDTH, 0,
				     rvals.amplitude, 0, 200, 1, 10, 0,
				     TRUE, 0, 0,
				     NULL, NULL);
  gtk_signal_connect (GTK_OBJECT (scale_data), "value_changed",
		      GTK_SIGNAL_FUNC (gimp_int_adjustment_update),
		      &rvals.amplitude);

  gtk_widget_show (frame);
  gtk_widget_show (table);

  gtk_widget_show (main_vbox);
  gtk_widget_show (dlg);

  gtk_main ();
  gdk_flush ();

  return rpint.run;
}

static GTile *
ripple_pixel (GDrawable *drawable,
	      GTile     *tile,
	      gint       x1,
	      gint       y1,
	      gint       x2,
	      gint       y2,
	      gint       x,
	      gint       y,
	      gint      *row,
	      gint      *col,
	      guchar    *pixel)
{
  static guchar empty_pixel[4] = {0, 0, 0, 0};

  guchar *data;
  gint    b;

  if (x >= x1 && y >= y1 && x < x2 && y < y2)
    {
      if ((x >> 6 != *col) || (y >> 6 != *row))
	{
	  *col = x / 64;
	  *row = y / 64;
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


/*  Ripple interface functions  */

static void
ripple_ok_callback (GtkWidget *widget,
		    gpointer   data)
{
  rpint.run = TRUE;

  gtk_widget_destroy (GTK_WIDGET (data));
}

static guchar
averagetwo (gdouble  location,
	    guchar  *v)
{
  location = fmod(location, 1.0);

  return (guchar) ((1.0 - location) * v[0] + location * v[1]);
}

static guchar
averagefour (gdouble  location,
	     guchar  *v)
{
  location = fmod(location, 1.0);

  return ((1.0 - location) * (v[0] + v[2]) + location * (v[1] + v[3]))/2;
}

static gdouble
displace_amount (gint location)
{
  switch (rvals.waveform)
    {
    case SINE:
      return rvals.amplitude*sin(location*(2*G_PI)/(double)rvals.period);
    case SAWTOOTH:
      return floor (rvals.amplitude *
		    (fabs ((((location%rvals.period) /
			     (double)rvals.period) * 4) - 2) - 1));
    }

  return 0;
}
