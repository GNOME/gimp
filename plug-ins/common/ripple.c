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


/* Declare local functions.
 */
static void    query  (void);
static void    run    (const gchar      *name,
		       gint              nparams,
		       const GimpParam  *param,
		       gint             *nreturn_vals,
		       GimpParam       **return_vals);

static void    ripple              (GimpDrawable *drawable);

static gint    ripple_dialog       (void);

static gdouble displace_amount     (gint      location);
static void    average_two_pixels  (guchar   *dest,
                                    guchar    pixels[4][4],
                                    gdouble   x,
                                    gint      bpp,
                                    gboolean  has_alpha);
static void    average_four_pixels (guchar   *dest,
                                    guchar    pixels[4][4],
                                    gdouble   x,
                                    gint      bpp,
                                    gboolean  has_alpha);

/***** Local vars *****/

GimpPlugInInfo PLUG_IN_INFO =
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

static GimpRunMode run_mode;

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
    { GIMP_PDB_INT32, "period", "period; number of pixels for one wave to complete" },
    { GIMP_PDB_INT32, "amplitude", "amplitude; maximum displacement of wave" },
    { GIMP_PDB_INT32, "orientation", "orientation; 0 = Horizontal, 1 = Vertical" },
    { GIMP_PDB_INT32, "edges", "edges; 0 = smear, 1 =  wrap, 2 = black" },
    { GIMP_PDB_INT32, "waveform", "0 = sawtooth, 1 = sine wave" },
    { GIMP_PDB_INT32, "antialias", "antialias; True or False" },
    { GIMP_PDB_INT32, "tile", "tile; if this is true, the image will retain it's tilability" }
  };

  gimp_install_procedure ("plug_in_ripple",
			  "Ripple the contents of the specified drawable",
			  "Ripples the pixels of the specified drawable. Each row or column will be displaced a certain number of pixels coinciding with the given wave form",
			  "Brian Degenhardt <bdegenha@ucsd.edu>",
			  "Brian Degenhardt",
			  "1997",
			  N_("<Image>/Filters/Distorts/_Ripple..."),
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
  GimpDrawable      *drawable;
  GimpPDBStatusType  status = GIMP_PDB_SUCCESS;

  run_mode = param[0].data.d_int32;

  INIT_I18N ();

  /*  Get the specified drawable  */
  drawable = gimp_drawable_get (param[2].data.d_drawable);

  *nreturn_vals = 1;
  *return_vals  = values;

  values[0].type          = GIMP_PDB_STATUS;
  values[0].data.d_status = status;

  switch (run_mode)
    {
    case GIMP_RUN_INTERACTIVE:
      /*  Possibly retrieve data  */
      gimp_get_data ("plug_in_ripple", &rvals);

      /*  First acquire information with a dialog  */
      if (! ripple_dialog ())
	return;
      break;

    case GIMP_RUN_NONINTERACTIVE:
      /*  Make sure all the arguments are there!  */
      if (nparams != 10)
	{
	  status = GIMP_PDB_CALLING_ERROR;
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
	    status = GIMP_PDB_CALLING_ERROR;
	}
      break;

    case GIMP_RUN_WITH_LAST_VALS:
      /*  Possibly retrieve data  */
      gimp_get_data ("plug_in_ripple", &rvals);
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
	  gimp_progress_init (_("Rippling..."));

	  /*  set the tile cache size  */
	  gimp_tile_cache_ntiles (TILE_CACHE_SIZE);

	  /*  run the ripple effect  */
	  ripple (drawable);

	  if (run_mode != GIMP_RUN_NONINTERACTIVE)
	    gimp_displays_flush ();

	  /*  Store data  */
	  if (run_mode == GIMP_RUN_INTERACTIVE)
	    gimp_set_data ("plug_in_ripple", &rvals, sizeof (RippleValues));
	}
      else
	{
	  /* gimp_message ("ripple: cannot operate on indexed color images"); */
	  status = GIMP_PDB_EXECUTION_ERROR;
	}
    }

  values[0].data.d_status = status;

  gimp_drawable_detach (drawable);
}

typedef struct {
  GimpPixelFetcher	*pft;
  gint 			 width;
  gint			 height;
  gboolean 		 has_alpha;
} RippleParam_t;

static void
ripple_vertical (gint x,
		 gint y,
		 guchar *dest,
		 gint bpp,
		 gpointer data)
{
  RippleParam_t *param = (RippleParam_t*) data;
  GimpPixelFetcher *pft;
  guchar   pixel[4][4];
  gdouble  needy;
  gint	   yi, height;

  pft = param->pft;
  height = param->height;

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
      yi = CLAMP (yi, 0, height - 1);
    }
  
  if (rvals.antialias)
    {
      if (yi == height - 1)
	{
	  gimp_pixel_fetcher_get_pixel (pft, x, yi, dest);
	}
      else if (needy < 0 && needy > -1)
	{
	  gimp_pixel_fetcher_get_pixel (pft, x, 0, dest);
	}
      else if (yi == height - 2 || yi == 0)
	{
	  gimp_pixel_fetcher_get_pixel (pft, x, yi, pixel[0]);
	  gimp_pixel_fetcher_get_pixel (pft, x, yi + 1, pixel[1]);
	  
	  average_two_pixels (dest, pixel, needy, bpp, param->has_alpha);
	}
      else
	{
	  gimp_pixel_fetcher_get_pixel (pft, x, yi, pixel[0]);
	  gimp_pixel_fetcher_get_pixel (pft, x, yi + 1, pixel[1]);
	  gimp_pixel_fetcher_get_pixel (pft, x, yi - 1, pixel[2]);
	  gimp_pixel_fetcher_get_pixel (pft, x, yi + 2, pixel[3]);
	  
	  average_four_pixels (dest, pixel, needy, bpp, param->has_alpha);
	}
    } /* antialias */
  else
    {
      gimp_pixel_fetcher_get_pixel (pft, x, yi, dest);
    }
}

static void
ripple_horizontal (gint x,
		 gint y,
		 guchar *dest,
		 gint bpp,
		 gpointer data)
{
  RippleParam_t *param = (RippleParam_t*) data;
  GimpPixelFetcher *pft;
  guchar   pixel[4][4];
  gdouble  needx;
  gint	   xi, width;

  pft = param->pft;
  width = param->width;

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
      xi = CLAMP(xi, 0, width - 1);
    }
  
  if (rvals.antialias)
    {
      if (xi == width - 1)
	{
	  gimp_pixel_fetcher_get_pixel (pft, xi, y, dest);
	}
      else if (floor(needx) ==  -1)
	{
	  gimp_pixel_fetcher_get_pixel (pft, 0, y, dest);
	}
      
      else if (xi == width - 2 || xi == 0)
	{
	  gimp_pixel_fetcher_get_pixel (pft, xi, y, pixel[0]);
	  gimp_pixel_fetcher_get_pixel (pft, xi + 1, y, pixel[1]);
	  
	  average_two_pixels (dest, pixel, needx, bpp, param->has_alpha);
	}
      else
	{
	  gimp_pixel_fetcher_get_pixel (pft, xi, y, pixel[0]);
	  gimp_pixel_fetcher_get_pixel (pft, xi + 1, y, pixel[1]);
	  gimp_pixel_fetcher_get_pixel (pft, xi - 1, y, pixel[2]);
	  gimp_pixel_fetcher_get_pixel (pft, xi + 2, y, pixel[3]);
	  
	  average_four_pixels (dest, pixel, needx, bpp, param->has_alpha);
	}
    } /* antialias */
  
  else
    {
      gimp_pixel_fetcher_get_pixel (pft, xi, y, dest);
    }
}

static void
ripple (GimpDrawable *drawable)
{
  GimpRgnIterator *iter;
  RippleParam_t param;

  param.pft = gimp_pixel_fetcher_new (drawable);
  param.has_alpha = gimp_drawable_has_alpha (drawable->drawable_id);
  param.width  = drawable->width;
  param.height = drawable->height;

  if ( rvals.tile )
    {
      rvals.edges = WRAP;
      rvals.period = (param.width / (param.width / rvals.period) *
		      (rvals.orientation == HORIZONTAL) +
		      param.height / (param.height / rvals.period) *
		      (rvals.orientation == VERTICAL));
    }

  iter = gimp_rgn_iterator_new (drawable, run_mode);
  gimp_rgn_iterator_dest (iter, (rvals.orientation == VERTICAL)
			  ? ripple_vertical : ripple_horizontal, &param);
  gimp_rgn_iterator_free (iter);

  gimp_pixel_fetcher_destroy (param.pft);
}

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
  gboolean   run;

  gimp_ui_init ("ripple", TRUE);

  dlg = gimp_dialog_new (_("Ripple"), "ripple",
                         NULL, 0,
			 gimp_standard_help_func, "filters/ripple.html",

			 GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
			 GTK_STOCK_OK,     GTK_RESPONSE_OK,

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

  toggle = gtk_check_button_new_with_mnemonic (_("_Antialiasing"));
  gtk_box_pack_start (GTK_BOX (toggle_vbox), toggle, FALSE, FALSE, 0);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (toggle), rvals.antialias);
  gtk_widget_show (toggle);

  g_signal_connect (toggle, "toggled",
                    G_CALLBACK (gimp_toggle_button_update),
                    &rvals.antialias);

  toggle = gtk_check_button_new_with_mnemonic ( _("_Retain Tilability"));
  gtk_box_pack_start (GTK_BOX (toggle_vbox), toggle, FALSE, FALSE, 0);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (toggle), (rvals.tile));
  gtk_widget_show (toggle);

  g_signal_connect (toggle, "toggled",
                    G_CALLBACK (gimp_toggle_button_update),
                    &rvals.tile);

  gtk_widget_show (toggle_vbox);
  gtk_widget_show (frame);

  /*  Orientation toggle box  */
  frame = gimp_radio_group_new2 (TRUE, _("Orientation"),
                                 G_CALLBACK (gimp_radio_button_update),
                                 &rvals.orientation,
                                 GINT_TO_POINTER (rvals.orientation),

                                 _("_Horizontal"),
                                 GINT_TO_POINTER (HORIZONTAL), NULL,

                                 _("_Vertical"),
                                 GINT_TO_POINTER (VERTICAL), NULL,

                                 NULL);

  gtk_table_attach (GTK_TABLE (table), frame, 1, 2, 0, 1,
		    GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL, 0, 0);
  gtk_widget_show (frame);

  /*  Edges toggle box  */
  frame = gimp_radio_group_new2 (TRUE, _("Edges"),
				 G_CALLBACK (gimp_radio_button_update),
				 &rvals.edges, (gpointer) rvals.edges,

				 _("_Wrap"),  (gpointer) WRAP, NULL,
				 _("_Smear"), (gpointer) SMEAR, NULL,
				 _("_Black"), (gpointer) BLACK, NULL,

				 NULL);
  gtk_table_attach (GTK_TABLE (table), frame, 0, 1, 1, 2,
		    GTK_FILL | GTK_EXPAND, GTK_FILL | GTK_EXPAND, 0, 0);
  gtk_widget_show (frame);

  /*  Wave toggle box  */
  frame = gimp_radio_group_new2 (TRUE, _("Wave Type"),
				 G_CALLBACK (gimp_radio_button_update),
				 &rvals.waveform, (gpointer) rvals.waveform,

				 _("Saw_tooth"), (gpointer) SAWTOOTH, NULL,
				 _("S_ine"),     (gpointer) SINE, NULL,

				 NULL);
  gtk_table_attach (GTK_TABLE (table), frame, 1, 2, 1, 2,
		    GTK_FILL | GTK_EXPAND, GTK_FILL | GTK_EXPAND, 0, 0);
  gtk_widget_show (frame);

  gtk_widget_show (table);

  table = gimp_parameter_settings_new (main_vbox, 2, 3);

  /*  Period  */
  scale_data = gimp_scale_entry_new (GTK_TABLE (table), 0, 0,
				     _("_Period:"), SCALE_WIDTH, 0,
				     rvals.period, 0, 200, 1, 10, 0,
				     TRUE, 0, 0,
				     NULL, NULL);
  g_signal_connect (scale_data, "value_changed",
                    G_CALLBACK (gimp_int_adjustment_update),
                    &rvals.period);

  /*  Amplitude  */
  scale_data = gimp_scale_entry_new (GTK_TABLE (table), 0, 1,
				     _("A_mplitude:"), SCALE_WIDTH, 0,
				     rvals.amplitude, 0, 200, 1, 10, 0,
				     TRUE, 0, 0,
				     NULL, NULL);
  g_signal_connect (scale_data, "value_changed",
                    G_CALLBACK (gimp_int_adjustment_update),
                    &rvals.amplitude);

  gtk_widget_show (main_vbox);
  gtk_widget_show (dlg);

  run = (gimp_dialog_run (GIMP_DIALOG (dlg)) == GTK_RESPONSE_OK);

  gtk_widget_destroy (dlg);

  return run;
}

static void
average_two_pixels (guchar   *dest,
                    guchar    pixels[4][4],
                    gdouble   x,
                    gint      bpp,
                    gboolean  has_alpha)
{
  gint b;

  x = fmod (x, 1.0);

  if (has_alpha)
    {
      gdouble xa0 = pixels[0][bpp-1] * (1.0 - x);
      gdouble xa1 = pixels[1][bpp-1] * x;
      gdouble alpha;

      alpha = xa0 + xa1;

      if (alpha)
        for (b = 0; b < bpp-1; b++)
          dest[b] = (xa0 * pixels[0][b] + xa1 * pixels[1][b]) / alpha;

      dest[bpp-1] = alpha;
    }
  else
    {
      for (b = 0; b < bpp; b++)
        dest[b] = (1.0 - x) * pixels[0][b] + x * pixels[1][b];
    }
}

static void
average_four_pixels (guchar   *dest,
                     guchar    pixels[4][4],
                     gdouble   x,
                     gint      bpp,
                     gboolean  has_alpha)
{
  gint b;

  x = fmod (x, 1.0);

  if (has_alpha)
    {
      gdouble xa0 = pixels[0][bpp-1] * (1.0 - x) / 2;
      gdouble xa1 = pixels[1][bpp-1] * x / 2;
      gdouble xa2 = pixels[2][bpp-1] * (1.0 - x) / 2;
      gdouble xa3 = pixels[3][bpp-1] * x / 2;
      gdouble alpha;

      alpha = xa0 + xa1 + xa2 + xa3;

      if (alpha)
        for (b = 0; b < bpp-1; b++)
          dest[b] = (xa0 * pixels[0][b] +
                     xa1 * pixels[1][b] +
                     xa2 * pixels[2][b] +
                     xa3 * pixels[3][b]) / alpha;

      dest[bpp-1] = alpha;
    }
  else
    {
      for (b = 0; b < bpp; b++)
        dest[b] = ((1.0 - x) * (pixels[0][b] + pixels[2][b]) / 2 +
                   x         * (pixels[1][b] + pixels[3][b]) / 2);
    }
}

static gdouble
displace_amount (gint location)
{
  switch (rvals.waveform)
    {
    case SINE:
      return (rvals.amplitude *
              sin (location * (2 * G_PI) / (gdouble) rvals.period));
    case SAWTOOTH:
      return floor (rvals.amplitude *
		    (fabs ((((location % rvals.period) /
			     (gdouble) rvals.period) * 4) - 2) - 1));
    }

  return 0.0;
}
