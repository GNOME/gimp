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

#include "gtk/gtk.h"
#include "libgimp/gimp.h"


/* Some useful macros */
#define SCALE_WIDTH 200
#define TILE_CACHE_SIZE 16
#define ENTRY_WIDTH 35

#define HORIZONTAL 0
#define VERTICAL 1

#define SMEAR 0
#define WRAP 1
#define BLACK 2

#define SAWTOOTH 0
#define SINE 1

typedef struct {
    gint period;
    gint amplitude;
    gint orientation;
    gint edges;
    gint waveform;
    gint antialias;
    gint tile;
} RippleValues;

typedef struct {
  gint run;
} RippleInterface;


/* Declare local functions.
 */
static void      query  (void);
static void      run    (gchar    *name,
			 gint      nparams,
			 GParam   *param,
			 gint     *nreturn_vals,
			 GParam  **return_vals);
static void      ripple  (GDrawable * drawable);

static gint      ripple_dialog (void);
static GTile *   ripple_pixel  (GDrawable * drawable,
			       GTile *     tile,
			       gint        x1,
			       gint        y1,
			       gint        x2,
			       gint        y2,
			       gint        x,
			       gint        y,
			       gint *      row,
			       gint *      col,
			       guchar *    pixel);

static void      ripple_close_callback  (GtkWidget *widget,
					gpointer   data);
static void      ripple_ok_callback     (GtkWidget *widget,
					gpointer   data);

static void      ripple_toggle_update    (GtkWidget *widget,
					    gpointer   data);

static void      ripple_ientry_callback   (GtkWidget     *widget,
					     gpointer       data);

static void      ripple_iscale_callback   (GtkAdjustment *adjustment,
					     gpointer       data);

static gdouble displace_amount (gint location);

static guchar    averagetwo (gdouble location, guchar * v);

static guchar    averagefour  (gdouble location,  guchar *v);



/***** Local vars *****/

GPlugInInfo PLUG_IN_INFO =
{
  NULL,    /* init_proc */
  NULL,    /* quit_proc */
  query,   /* query_proc */
  run,     /* run_proc */
};

static RippleValues rvals =
{
  20,   /* period  */
  5,    /* amplitude */
  HORIZONTAL, /* orientation */
  WRAP, /* edges */
  SINE, /* waveform */
  TRUE, /* antialias */
  TRUE  /* tile */
};

static RippleInterface rpint =
{
  FALSE   /*  run  */
};

/***** Functions *****/

MAIN ()

static void
query ()
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
    { PARAM_INT32, "tile", "tile; if this is true, the image will retain it's tilability" },
  };
  static GParamDef *return_vals = NULL;
  static gint nargs = sizeof (args) / sizeof (args[0]);
  static gint nreturn_vals = 0;

  gimp_install_procedure ("plug_in_ripple",
			  "Ripple the contents of the specified drawable",
			  "Ripples the pixels of the specified drawable. Each row or colum will be displaced a certain number of pixels coinciding with the given wave form",
			  "Brian Degenhardt <bdegenha@ucsd.edu>",
			  "Brian Degenhardt",
			  "1997",
			  "<Image>/Filters/Distorts/Ripple",
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
      /*  Possibly retrieve data  */
      gimp_get_data ("plug_in_ripple", &rvals);

      /*  First acquire information with a dialog  */
      if (! ripple_dialog ())
	return;
      break;

    case RUN_NONINTERACTIVE:
      /*  Make sure all the arguments are there!  */
      if (nparams != 10)
	status = STATUS_CALLING_ERROR;
      if (status == STATUS_SUCCESS)
	{
	  rvals.period = param[3].data.d_int32;
	  rvals.amplitude = param[4].data.d_int32;
          rvals.orientation = (param[5].data.d_int32) ? VERTICAL : HORIZONTAL;
          rvals.edges = (param[6].data.d_int32);
          rvals.waveform = param[7].data.d_int32;
          rvals.antialias = (param[8].data.d_int32) ? TRUE : FALSE;
          rvals.tile = (param[9].data.d_int32) ? TRUE : FALSE;
        }
      if (status == STATUS_SUCCESS &&
	  (rvals.edges < SMEAR || rvals.edges > BLACK))
          status = STATUS_CALLING_ERROR;
      break;

    case RUN_WITH_LAST_VALS:
      /*  Possibly retrieve data  */
      gimp_get_data ("plug_in_ripple", &rvals);
      break;

    default:
      break;
    }

  if (status == STATUS_SUCCESS)
    {
      /*  Make sure that the drawable is gray or RGB color  */
      if (gimp_drawable_color (drawable->id) || gimp_drawable_is_gray (drawable->id))
	{
	  gimp_progress_init ("Rippling...");

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

/*****/

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
      rvals.period = width/(width/rvals.period)*(rvals.orientation==HORIZONTAL) + height/(height/rvals.period)*(rvals.orientation==VERTICAL);
  }

  progress     = 0;
  max_progress = (x2 - x1) * (y2 - y1);

/* Ripple the image.  It's a pretty simple algorithm.  If horizontal
     is selected, then every row is displaced a number of pixels that
     follows the pattern of the waveform selected.  The effect is
     just reproduced with columns if vertical is selected.
*/

  gimp_pixel_rgn_init (&dest_rgn, drawable, x1, y1, (x2 - x1), (y2 - y1), TRUE, TRUE);
  for (pr = gimp_pixel_rgns_register (1, &dest_rgn); pr != NULL; pr = gimp_pixel_rgns_process (pr))
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
                          tile = ripple_pixel (drawable, tile, x1, y1, x2, y2, x, yi, &row, &col, pixel[0]);

                          for (k = 0; k < bytes; k++)
                              *otherdest++ = pixel[0][k];
                      }
                      else if (needy < 0 && needy > -1)
                      {
                          tile = ripple_pixel (drawable, tile, x1, y1, x2, y2, x, 0, &row, &col, pixel[0]);

                          for (k = 0; k < bytes; k++)
                              *otherdest++ = pixel[0][k];
                      }

                      else if (yi == height - 2 || yi == 0)
                      {
                          tile = ripple_pixel (drawable, tile, x1, y1, x2, y2, x, yi, &row, &col, pixel[0]);
                          tile = ripple_pixel (drawable, tile, x1, y1, x2, y2, x, yi + 1, &row, &col, pixel[1]);

                          for (k = 0; k < bytes; k++)
                          {
                              values[0] = pixel[0][k];
                              values[1] = pixel[1][k];
                              val = averagetwo(needy, values);

                              *otherdest++ = val;
                          } /* for */
                      }
                      else
                      {
                          tile = ripple_pixel (drawable, tile, x1, y1, x2, y2, x, yi, &row, &col, pixel[0]);
                          tile = ripple_pixel (drawable, tile, x1, y1, x2, y2, x, yi + 1, &row, &col, pixel[1]);
                          tile = ripple_pixel (drawable, tile, x1, y1, x2, y2, x, yi - 1, &row, &col, pixel[2]);
                          tile = ripple_pixel (drawable, tile, x1, y1, x2, y2, x, yi + 2, &row, &col, pixel[3]);

                          for (k = 0; k < bytes; k++)
                          {
                              values[0] = pixel[0][k];
                              values[1] = pixel[1][k];
                              values[2] = pixel[2][k];
                              values[3] = pixel[3][k];

                              val = averagefour(needy, values);

                              *otherdest++ = val;
                          } /* for */
                      } /* else */
                  } /* antialias */

                  else
                  {
                      tile = ripple_pixel (drawable, tile, x1, y1, x2, y2, x, yi, &row, &col, pixel[0]);

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
                  xi = floor(needx);

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
                          tile = ripple_pixel (drawable, tile, x1, y1, x2, y2, xi, y, &row, &col, pixel[0]);

                          for (k = 0; k < bytes; k++)
                              *dest++ = pixel[0][k];
                      }
                      else if (floor(needx) ==  -1)
                      {
                          tile = ripple_pixel (drawable, tile, x1, y1, x2, y2, 0, y, &row, &col, pixel[0]);

                          for (k = 0; k < bytes; k++)
                              *dest++ = pixel[0][k];
                      }

                      else if (xi == width - 2 || xi == 0)
                      {
                          tile = ripple_pixel (drawable, tile, x1, y1, x2, y2, xi, y, &row, &col, pixel[0]);
                          tile = ripple_pixel (drawable, tile, x1, y1, x2, y2, xi + 1, y, &row, &col, pixel[1]);

                          for (k = 0; k < bytes; k++)
                          {
                              values[0] = pixel[0][k];
                              values[1] = pixel[1][k];
                              val = averagetwo(needx, values);

                              *dest++ = val;
                          } /* for */
                      }
                      else
                      {
                          tile = ripple_pixel (drawable, tile, x1, y1, x2, y2, xi, y, &row, &col, pixel[0]);
                          tile = ripple_pixel (drawable, tile, x1, y1, x2, y2, xi + 1, y, &row, &col, pixel[1]);
                          tile = ripple_pixel (drawable, tile, x1, y1, x2, y2, xi - 1 , y, &row, &col, pixel[2]);
                          tile = ripple_pixel (drawable, tile, x1, y1, x2, y2, xi + 2, y, &row, &col, pixel[3]);

                          for (k = 0; k < bytes; k++)
                          {
                              values[0] = pixel[0][k];
                              values[1] = pixel[1][k];
                              values[2] = pixel[2][k];
                              values[3] = pixel[3][k];

                              val = averagefour(needx, values);

                              *dest++ = val;
                          } /* for */
                      } /* else */
                  } /* antialias */

                  else
                  {
                      tile = ripple_pixel (drawable, tile, x1, y1, x2, y2, xi, y, &row, &col, pixel[0]);

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
ripple_dialog ()
{
    GtkWidget *dlg;
    GtkWidget *label;
    GtkWidget *button;
    GtkWidget *toggle;
    GtkWidget *scale;
    GtkWidget *hbox;
    GtkWidget *entry;
    GtkWidget *toggle_vbox;
    GtkWidget *main_vbox;
    GtkWidget *frame;
    GtkWidget *table;
    GtkObject *scale_data;
    GSList *orientation_group = NULL;
    GSList *edges_group = NULL;
    GSList *waveform_group = NULL;
    guchar *color_cube;
    gchar **argv;
    gchar buffer[32];
    gint argc;
    gint do_horizontal = (rvals.orientation == HORIZONTAL);
    gint do_vertical = (rvals.orientation == VERTICAL);

    gint do_smear = (rvals.edges == SMEAR);
    gint do_wrap = (rvals.edges == WRAP);
    gint do_black = (rvals.edges == BLACK);

    gint do_sawtooth = (rvals.waveform == SAWTOOTH);
    gint do_sine = (rvals.waveform == SINE);

    argc = 1;
    argv = g_new (gchar *, 1);
    argv[0] = g_strdup ("ripple");

    gtk_init (&argc, &argv);
    gtk_rc_parse (gimp_gtkrc ());

    gtk_preview_set_gamma (gimp_gamma ());
    gtk_preview_set_install_cmap (gimp_install_cmap ());
    color_cube = gimp_color_cube ();
    gtk_preview_set_color_cube (color_cube[0], color_cube[1],
                                color_cube[2], color_cube[3]);

    gtk_widget_set_default_visual (gtk_preview_get_visual ());
    gtk_widget_set_default_colormap (gtk_preview_get_cmap ());

    dlg = gtk_dialog_new ();
    gtk_window_set_title (GTK_WINDOW (dlg), "Ripple");
    gtk_window_position (GTK_WINDOW (dlg), GTK_WIN_POS_MOUSE);
    gtk_signal_connect (GTK_OBJECT (dlg), "destroy",
                        (GtkSignalFunc) ripple_close_callback,
                        NULL);

        /*  Action area  */
    button = gtk_button_new_with_label ("OK");
    GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT);
    gtk_signal_connect (GTK_OBJECT (button), "clicked",
                        (GtkSignalFunc) ripple_ok_callback,
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

        /*  The main vbox  */
    main_vbox = gtk_vbox_new (FALSE, 5);
    gtk_container_border_width (GTK_CONTAINER (main_vbox), 10);
    gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dlg)->vbox), main_vbox, TRUE, TRUE, 0);

        /*  The hbox for first row of options  */
    hbox = gtk_hbox_new (FALSE, 5);
    gtk_box_pack_start (GTK_BOX (main_vbox), hbox, TRUE, TRUE, 0);

        /* The table to hold the four frames of options */

    table = gtk_table_new (2, 2, FALSE);
    gtk_container_border_width (GTK_CONTAINER (table), 10);
    gtk_box_pack_start (GTK_BOX (hbox), table, TRUE, TRUE, 0);


        /* Options section */
        /*  the vertical box and its toggle buttons  */
    frame = gtk_frame_new ("Options");
    gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_ETCHED_IN);
    gtk_table_attach (GTK_TABLE (table), frame, 0, 1, 0, 1,
                      GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL, 5, 5);
    toggle_vbox = gtk_vbox_new (FALSE, 5);
    gtk_container_border_width (GTK_CONTAINER (toggle_vbox), 5);
    gtk_container_add (GTK_CONTAINER (frame), toggle_vbox);

    toggle = gtk_check_button_new_with_label ("Antialiasing");
    gtk_box_pack_start (GTK_BOX (toggle_vbox), toggle, FALSE, FALSE, 0);
    gtk_signal_connect (GTK_OBJECT (toggle), "toggled",
                        (GtkSignalFunc) ripple_toggle_update,
                        &rvals.antialias);
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (toggle), rvals.antialias);
    gtk_widget_show (toggle);

    toggle = gtk_check_button_new_with_label ("Retain Tilability");
    gtk_box_pack_start (GTK_BOX (toggle_vbox), toggle, FALSE, FALSE, 0);
    gtk_signal_connect (GTK_OBJECT (toggle), "toggled",
                        (GtkSignalFunc) ripple_toggle_update,
                        &rvals.tile);
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (toggle), (rvals.tile));
    gtk_widget_show (toggle);

    gtk_widget_show (toggle_vbox);
    gtk_widget_show (frame);


/*  Orientation toggle box  */
    frame = gtk_frame_new ("Orientation");
    gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_ETCHED_IN);
    gtk_table_attach (GTK_TABLE (table), frame, 1, 2, 0, 1,
                      GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL, 5, 5);
    toggle_vbox = gtk_vbox_new (FALSE, 5);
    gtk_container_border_width (GTK_CONTAINER (toggle_vbox), 5);
    gtk_container_add (GTK_CONTAINER (frame), toggle_vbox);

    toggle = gtk_radio_button_new_with_label (orientation_group, "Horizontal");
    orientation_group = gtk_radio_button_group (GTK_RADIO_BUTTON (toggle));
    gtk_box_pack_start (GTK_BOX (toggle_vbox), toggle, FALSE, FALSE, 0);
    gtk_signal_connect (GTK_OBJECT (toggle), "toggled",
                        (GtkSignalFunc) ripple_toggle_update,
                        &do_horizontal);
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (toggle), do_horizontal);
    gtk_widget_show (toggle);

    toggle = gtk_radio_button_new_with_label (orientation_group, "Vertical");
    orientation_group = gtk_radio_button_group (GTK_RADIO_BUTTON (toggle));
    gtk_box_pack_start (GTK_BOX (toggle_vbox), toggle, FALSE, FALSE, 0);
    gtk_signal_connect (GTK_OBJECT (toggle), "toggled",
                        (GtkSignalFunc) ripple_toggle_update,
                        &do_vertical);
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (toggle), do_vertical);
    gtk_widget_show (toggle);

    gtk_widget_show (toggle_vbox);
    gtk_widget_show (frame);
    gtk_widget_show (hbox);


        /*  The hbox for the second row of options  */
    hbox = gtk_hbox_new (FALSE, 5);
    gtk_box_pack_start (GTK_BOX (main_vbox), hbox, TRUE, TRUE, 0);

/*  Edges toggle box  */
    frame = gtk_frame_new ("Edges");
    gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_ETCHED_IN);
    gtk_table_attach (GTK_TABLE (table), frame, 0, 1, 1, 2, GTK_FILL | GTK_EXPAND, GTK_FILL | GTK_EXPAND, 5, 5);
    toggle_vbox = gtk_vbox_new (FALSE, 5);
    gtk_container_border_width (GTK_CONTAINER (toggle_vbox), 5);
    gtk_container_add (GTK_CONTAINER (frame), toggle_vbox);

    toggle = gtk_radio_button_new_with_label (edges_group, "Wrap");
    edges_group = gtk_radio_button_group (GTK_RADIO_BUTTON (toggle));
    gtk_box_pack_start (GTK_BOX (toggle_vbox), toggle, FALSE, FALSE, 0);
    gtk_signal_connect (GTK_OBJECT (toggle), "toggled",
                        (GtkSignalFunc) ripple_toggle_update,
                        &do_wrap);
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (toggle), do_wrap);
    gtk_widget_show (toggle);

    toggle = gtk_radio_button_new_with_label (edges_group, "Smear");
    edges_group = gtk_radio_button_group (GTK_RADIO_BUTTON (toggle));
    gtk_box_pack_start (GTK_BOX (toggle_vbox), toggle, FALSE, FALSE, 0);
    gtk_signal_connect (GTK_OBJECT (toggle), "toggled",
                        (GtkSignalFunc) ripple_toggle_update,
                        &do_smear);
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (toggle), do_smear);
    gtk_widget_show (toggle);

    toggle = gtk_radio_button_new_with_label (edges_group, "Black");
    edges_group = gtk_radio_button_group (GTK_RADIO_BUTTON (toggle));
    gtk_box_pack_start (GTK_BOX (toggle_vbox), toggle, FALSE, FALSE, 0);
    gtk_signal_connect (GTK_OBJECT (toggle), "toggled",
                        (GtkSignalFunc) ripple_toggle_update,
                        &do_black);
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (toggle), do_black);
    gtk_widget_show (toggle);

    gtk_widget_show (toggle_vbox);
    gtk_widget_show (frame);


/*  Edges toggle box  */
    frame = gtk_frame_new ("Wave Type");
    gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_ETCHED_IN);
    gtk_table_attach (GTK_TABLE (table), frame, 1, 2, 1, 2, GTK_FILL | GTK_EXPAND, GTK_FILL | GTK_EXPAND, 5, 5);
    toggle_vbox = gtk_vbox_new (FALSE, 5);
    gtk_container_border_width (GTK_CONTAINER (toggle_vbox), 5);
    gtk_container_add (GTK_CONTAINER (frame), toggle_vbox);

    toggle = gtk_radio_button_new_with_label (waveform_group, "Sawtooth");
    waveform_group = gtk_radio_button_group (GTK_RADIO_BUTTON (toggle));
    gtk_box_pack_start (GTK_BOX (toggle_vbox), toggle, FALSE, FALSE, 0);
    gtk_signal_connect (GTK_OBJECT (toggle), "toggled",
                        (GtkSignalFunc) ripple_toggle_update,
                        &do_sawtooth);
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (toggle), do_sawtooth);
    gtk_widget_show (toggle);

    toggle = gtk_radio_button_new_with_label (waveform_group, "Sine");
    waveform_group = gtk_radio_button_group (GTK_RADIO_BUTTON (toggle));
    gtk_box_pack_start (GTK_BOX (toggle_vbox), toggle, FALSE, FALSE, 0);
    gtk_signal_connect (GTK_OBJECT (toggle), "toggled",
                        (GtkSignalFunc) ripple_toggle_update,
                        &do_sine);
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (toggle), do_sine);
    gtk_widget_show (toggle);

    gtk_widget_show (toggle_vbox);
    gtk_widget_show (frame);
    gtk_widget_show (table);
    gtk_widget_show (hbox);


  /*  parameter settings  */
  frame = gtk_frame_new ("Parameter Settings");
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_ETCHED_IN);
  gtk_container_border_width (GTK_CONTAINER (frame), 10);
  gtk_box_pack_start (GTK_BOX (main_vbox), frame, TRUE, TRUE, 0);
  table = gtk_table_new (2, 2, FALSE);
  gtk_container_border_width (GTK_CONTAINER (table), 10);
  gtk_container_add (GTK_CONTAINER (frame), table);




/* Period */
  label = gtk_label_new ("Period");
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gtk_table_attach (GTK_TABLE (table), label, 0, 1, 0, 1, GTK_FILL | GTK_EXPAND, GTK_FILL, 10, 5);
  gtk_widget_show (label);

  hbox = gtk_hbox_new (FALSE, 5);
  gtk_table_attach (GTK_TABLE (table), hbox, 1, 2, 0, 1,
		    GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL, 0, 0);
  gtk_widget_show (hbox);

  scale_data = gtk_adjustment_new (rvals.period, 0, 200, 1, 1, 0.0);
  gtk_signal_connect (GTK_OBJECT (scale_data), "value_changed",
		      (GtkSignalFunc) ripple_iscale_callback,
		      &rvals.period);

  scale = gtk_hscale_new (GTK_ADJUSTMENT (scale_data));
  gtk_widget_set_usize (scale, SCALE_WIDTH, 0);
  gtk_scale_set_digits (GTK_SCALE (scale), 2);
  gtk_scale_set_draw_value (GTK_SCALE (scale), FALSE);
  gtk_box_pack_start (GTK_BOX (hbox), scale, TRUE, TRUE, 0);
  gtk_widget_show (scale);

  entry = gtk_entry_new ();
  gtk_object_set_user_data (GTK_OBJECT (entry), scale_data);
  gtk_object_set_user_data (scale_data, entry);
  gtk_box_pack_start (GTK_BOX (hbox), entry, FALSE, TRUE, 0);
  gtk_widget_set_usize (entry, ENTRY_WIDTH, 0);
  sprintf (buffer, "%d", rvals.period);
  gtk_entry_set_text (GTK_ENTRY (entry), buffer);
  gtk_signal_connect (GTK_OBJECT (entry), "changed",
		      (GtkSignalFunc) ripple_ientry_callback,
		      &rvals.period);
  gtk_widget_show (entry);




/* Amplitude */
  label = gtk_label_new ("Amplitude");
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gtk_table_attach (GTK_TABLE (table), label, 0, 1, 1, 2, GTK_FILL | GTK_EXPAND, GTK_FILL, 10, 5);
  gtk_widget_show (label);

  hbox = gtk_hbox_new (FALSE, 5);
  gtk_table_attach (GTK_TABLE (table), hbox, 1, 2, 1, 2,
		    GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL, 0, 0);
  gtk_widget_show (hbox);

  scale_data = gtk_adjustment_new (rvals.amplitude, 0, 200, 1, 1, 0.0);
  gtk_signal_connect (GTK_OBJECT (scale_data), "value_changed",
		      (GtkSignalFunc) ripple_iscale_callback,
		      &rvals.amplitude);

  scale = gtk_hscale_new (GTK_ADJUSTMENT (scale_data));
  gtk_widget_set_usize (scale, SCALE_WIDTH, 0);
  gtk_scale_set_digits (GTK_SCALE (scale), 2);
  gtk_scale_set_draw_value (GTK_SCALE (scale), FALSE);
  gtk_box_pack_start (GTK_BOX (hbox), scale, TRUE, TRUE, 0);
  gtk_widget_show (scale);

  entry = gtk_entry_new ();
  gtk_object_set_user_data (GTK_OBJECT (entry), scale_data);
  gtk_object_set_user_data (scale_data, entry);
  gtk_box_pack_start (GTK_BOX (hbox), entry, FALSE, TRUE, 0);
  gtk_widget_set_usize (entry, ENTRY_WIDTH, 0);
  sprintf (buffer, "%d", rvals.amplitude);
  gtk_entry_set_text (GTK_ENTRY (entry), buffer);
  gtk_signal_connect (GTK_OBJECT (entry), "changed",
		      (GtkSignalFunc) ripple_ientry_callback,
		      &rvals.amplitude);
  gtk_widget_show (entry);

  gtk_widget_show (frame);
  gtk_widget_show (table);
  gtk_widget_show (main_vbox);
  gtk_widget_show (dlg);

  gtk_main();
  gdk_flush();

 /*  determine orientation  */
  if (do_horizontal)
    rvals.orientation = HORIZONTAL;
  else
    rvals.orientation = VERTICAL;

  /*  determine edges  */
  if (do_smear)
    rvals.edges = SMEAR;
  else if (do_wrap)
    rvals.edges = WRAP;
  else if (do_black)
    rvals.edges = BLACK;

  /*  determine wave form  */
  if (do_sawtooth)
    rvals.waveform = SAWTOOTH;
  else if (do_sine)
    rvals.waveform = SINE;

  return rpint.run;
}

/*****/

static GTile *
ripple_pixel (GDrawable * drawable,
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
ripple_close_callback (GtkWidget *widget,
		      gpointer   data)
{
  gtk_main_quit ();
}

static void
ripple_ok_callback (GtkWidget *widget,
		   gpointer   data)
{
  rpint.run = TRUE;
  gtk_widget_destroy (GTK_WIDGET (data));
}

static void
ripple_toggle_update (GtkWidget *widget,
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
ripple_ientry_callback (GtkWidget *widget,
			  gpointer   data)
{
  GtkAdjustment *adjustment;
  int new_val;
  int *val;

  val = data;
  new_val = atoi (gtk_entry_get_text (GTK_ENTRY (widget)));

  if (*val != new_val)
    {
      adjustment = gtk_object_get_user_data (GTK_OBJECT (widget));

      if ((new_val >= adjustment->lower) &&
	  (new_val <= adjustment->upper))
	{
	  *val = new_val;
	  adjustment->value = new_val;
	  gtk_signal_emit_by_name (GTK_OBJECT (adjustment), "value_changed");
	}
    }
}

static void
ripple_iscale_callback (GtkAdjustment *adjustment,
			  gpointer       data)
{
  GtkWidget *entry;
  gchar buffer[32];
  int *val;

  val = data;
  if (*val != (int) adjustment->value)
    {
      *val = adjustment->value;
      entry = gtk_object_get_user_data (GTK_OBJECT (adjustment));
      sprintf (buffer, "%d", (int) adjustment->value);
      gtk_entry_set_text (GTK_ENTRY (entry), buffer);
    }
}

static guchar
averagetwo (gdouble location, guchar *v)
{
  location = fmod(location, 1.0);

  return (guchar) ((1.0 - location) * v[0] + location * v[1]);
} /* averagetwo */

static guchar
averagefour (gdouble location, guchar *v)
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
            return floor(rvals.amplitude*(fabs((((location%rvals.period)/(double)rvals.period)*4)-2)-1));
    }
    return 0;
}
