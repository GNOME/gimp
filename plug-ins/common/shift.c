/* Shift --- image filter plug-in for The Gimp image manipulation program
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

#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "gtk/gtk.h"
#include "libgimp/gimp.h"


/* Some useful macros */

#define SCALE_WIDTH 200
#define TILE_CACHE_SIZE 16
#define HORIZONTAL 0
#define VERTICAL 1
#define ENTRY_WIDTH 35

typedef struct {
    gint shift_amount;
    gint orientation;
} ShiftValues;

typedef struct {
  gint run;
} ShiftInterface;


/* Declare local functions.
 */
static void      query  (void);
static void      run    (gchar    *name,
			 gint      nparams,
			 GParam   *param,
			 gint     *nreturn_vals,
			 GParam  **return_vals);
static void      shift  (GDrawable * drawable);

static gint      shift_dialog (void);
static GTile *   shift_pixel  (GDrawable * drawable,
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

static void      shift_close_callback  (GtkWidget *widget,
					gpointer   data);
static void      shift_ok_callback     (GtkWidget *widget,
					gpointer   data);

static void      shift_toggle_update    (GtkWidget *widget,
					    gpointer   data);

static void      shift_ientry_callback   (GtkWidget     *widget,
					     gpointer       data);

static void      shift_iscale_callback   (GtkAdjustment *adjustment,
					     gpointer       data);

/***** Local vars *****/

GPlugInInfo PLUG_IN_INFO =
{
  NULL,    /* init_proc */
  NULL,    /* quit_proc */
  query,   /* query_proc */
  run,     /* run_proc */
};

static ShiftValues shvals =
{
  5,   /* shift amount  */
  HORIZONTAL
};

static ShiftInterface shint =
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
    { PARAM_INT32, "shift_amount", "shift amount (0 <= shift_amount_x <= 200)" },
    { PARAM_INT32, "orientation", "vertical, horizontal orientation" },
  };
  static GParamDef *return_vals = NULL;
  static gint nargs = sizeof (args) / sizeof (args[0]);
  static gint nreturn_vals = 0;

  gimp_install_procedure ("plug_in_shift",
			  "Shift the contents of the specified drawable",
			  "Shifts the pixels of the specified drawable. Each row will be displaced a random value of pixels.",
			  "Spencer Kimball and Peter Mattis, ported by Brian Degenhardt and Federico Mena Quintero",
			  "Brian Degenhardt",
			  "1997",
			  "<Image>/Filters/Distorts/Shift...",
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
      gimp_get_data ("plug_in_shift", &shvals);

      /*  First acquire information with a dialog  */
      if (! shift_dialog ())
	return;
      break;

    case RUN_NONINTERACTIVE:
      /*  Make sure all the arguments are there!  */
      if (nparams != 5)
	status = STATUS_CALLING_ERROR;
      if (status == STATUS_SUCCESS)
	{
	  shvals.shift_amount = param[3].data.d_int32;
          shvals.orientation = (param[4].data.d_int32) ? HORIZONTAL : VERTICAL;
        }
      if ((status == STATUS_SUCCESS) &&
	  (shvals.shift_amount < 0 || shvals.shift_amount > 200))
     	status = STATUS_CALLING_ERROR;
      break;

    case RUN_WITH_LAST_VALS:
      /*  Possibly retrieve data  */
      gimp_get_data ("plug_in_shift", &shvals);
      break;

    default:
      break;
    }

  if (status == STATUS_SUCCESS)
    {
      /*  Make sure that the drawable is gray or RGB color  */
      if (gimp_drawable_is_rgb (drawable->id) || gimp_drawable_is_gray (drawable->id))
	{
	  gimp_progress_init ("Shifting...");

	  /*  set the tile cache size  */
	  gimp_tile_cache_ntiles (TILE_CACHE_SIZE);

	  /*  run the shift effect  */
	  shift (drawable);

	  if (run_mode != RUN_NONINTERACTIVE)
	    gimp_displays_flush ();

	  /*  Store data  */
	  if (run_mode == RUN_INTERACTIVE)
	    gimp_set_data ("plug_in_shift", &shvals, sizeof (ShiftValues));
	}
      else
	{
	  /* gimp_message ("shift: cannot operate on indexed color images"); */
	  status = STATUS_EXECUTION_ERROR;
	}
    }

  values[0].data.d_status = status;

  gimp_drawable_detach (drawable);
}

/*****/

static void
shift (GDrawable *drawable)
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
  gint    seed;

  gint amount;

  gint xdist, ydist;
  gint xi, yi;

  gint k;
  gint mod_value, sub_value;

  /* Get selection area */

  gimp_drawable_mask_bounds (drawable->id, &x1, &y1, &x2, &y2);

  width  = drawable->width;
  height = drawable->height;
  bytes  = drawable->bpp;

  progress     = 0;
  max_progress = (x2 - x1) * (y2 - y1);

  amount = shvals.shift_amount;

  /* Initialize random stuff */
  mod_value = amount + 1;
  sub_value = mod_value / 2;
  seed = time(NULL);

/* Shift the image.  It's a pretty simple algorithm.  If horizontal
     is selected, then every row is shifted a random number of pixels
     in the range of -shift_amount/2 to shift_amount/2.  The effect is
     just reproduced with columns if vertical is selected.  Vertical
     has been added since 0.54 so that the user doesn't have to rotate
     the image to do a vertical shift.
  */

  gimp_pixel_rgn_init (&dest_rgn, drawable, x1, y1, (x2 - x1), (y2 - y1), TRUE, TRUE);
  for (pr = gimp_pixel_rgns_register (1, &dest_rgn); pr != NULL; pr = gimp_pixel_rgns_process (pr))
    {
        if (shvals.orientation == VERTICAL)
        {

            destline = dest_rgn.data;
            srand(seed+dest_rgn.x);

            for (x = dest_rgn.x; x < (dest_rgn.x + dest_rgn.w); x++)
            {
                dest = destline;
                ydist = (rand() % mod_value) - sub_value;
                for (y = dest_rgn.y; y < (dest_rgn.y + dest_rgn.h); y++)
                {
                    otherdest = dest;

                    yi = (y + ydist + height)%height; /*  add width before % because % isn't a true modulo */

                    tile = shift_pixel (drawable, tile, x1, y1, x2, y2, x, yi, &row, &col, pixel[0]);

                    for (k = 0; k < bytes; k++)
                        *otherdest++ = pixel[0][k];
                    dest += dest_rgn.rowstride;
                } /* for */

                for (k = 0; k < bytes; k++)
                    destline++;
            } /* for */

            progress += dest_rgn.w * dest_rgn.h;
            gimp_progress_update ((double) progress / (double) max_progress);
        }
        else
        {
            destline = dest_rgn.data;
            srand(seed+dest_rgn.y);

            for (y = dest_rgn.y; y < (dest_rgn.y + dest_rgn.h); y++)
            {
                dest = destline;
                xdist = (rand() % mod_value) - sub_value;
                for (x = dest_rgn.x; x < (dest_rgn.x + dest_rgn.w); x++)
                {
                    xi = (x + xdist + width)%width; /*  add width before % because % isn't a true modulo */

                    tile = shift_pixel (drawable, tile, x1, y1, x2, y2, xi, y, &row, &col, pixel[0]);

                    for (k = 0; k < bytes; k++)
                        *dest++ = pixel[0][k];
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
} /* shift */


static gint
shift_dialog ()
{
  GtkWidget *amount_label;
  GtkWidget *amount;
  GtkWidget *dlg;
  GtkWidget *hbbox;
  GtkWidget *button;
  GtkWidget *toggle;
  GtkWidget *frame;
  GtkWidget *vbox;
  GtkWidget *hbox;
  GtkWidget *entry;
  GtkObject *amount_data;
  GSList *group = NULL;
  gchar **argv;
  gchar buffer[32];
  gint argc;
  gint do_horizontal = (shvals.orientation == HORIZONTAL);
  gint do_vertical = (shvals.orientation == VERTICAL);

  argc = 1;
  argv = g_new (gchar *, 1);
  argv[0] = g_strdup ("shift");

  gtk_init (&argc, &argv);
  gtk_rc_parse (gimp_gtkrc ());

  dlg = gtk_dialog_new ();
  gtk_window_set_title (GTK_WINDOW (dlg), "Shift");
  gtk_window_position (GTK_WINDOW (dlg), GTK_WIN_POS_MOUSE);
  gtk_signal_connect (GTK_OBJECT (dlg), "destroy",
		      (GtkSignalFunc) shift_close_callback,
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
		      (GtkSignalFunc) shift_ok_callback,
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

  toggle = gtk_radio_button_new_with_label (group, "Shift Horizontally");
  group = gtk_radio_button_group (GTK_RADIO_BUTTON (toggle));
  gtk_box_pack_start (GTK_BOX (vbox), toggle, TRUE, TRUE, 0);
  gtk_signal_connect (GTK_OBJECT (toggle), "toggled",
		      (GtkSignalFunc) shift_toggle_update,
		      &do_horizontal);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (toggle), do_horizontal);
  gtk_widget_show (toggle);

  toggle = gtk_radio_button_new_with_label (group, "Shift Vertically");
  group = gtk_radio_button_group (GTK_RADIO_BUTTON (toggle));
  gtk_box_pack_start (GTK_BOX (vbox), toggle, TRUE, TRUE, 0);
  gtk_signal_connect (GTK_OBJECT (toggle), "toggled",
		      (GtkSignalFunc) shift_toggle_update,
		      &do_vertical);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (toggle), do_vertical);
  gtk_widget_show (toggle);


  hbox = gtk_hbox_new (FALSE, 5);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, TRUE, TRUE, 0);

  amount_label = gtk_label_new ("Shift Amount:");
  gtk_misc_set_alignment (GTK_MISC (amount_label), 0.0, 0.5);
  gtk_box_pack_start (GTK_BOX (hbox), amount_label, TRUE, TRUE, 0);
  gtk_widget_show (amount_label);

  gtk_widget_show (hbox);

  amount_data = gtk_adjustment_new (shvals.shift_amount, 0, 200, 1, 1, 0.0);
  gtk_signal_connect (GTK_OBJECT (amount_data), "value_changed",
		      (GtkSignalFunc) shift_iscale_callback,
		      &shvals.shift_amount);

  amount = gtk_hscale_new (GTK_ADJUSTMENT (amount_data));
  gtk_widget_set_usize (amount, SCALE_WIDTH, 0);
  gtk_scale_set_digits (GTK_SCALE (amount), 0);
  gtk_scale_set_draw_value (GTK_SCALE (amount), FALSE);
  gtk_box_pack_start (GTK_BOX (hbox), amount, TRUE, TRUE, 0);
  gtk_widget_show (amount);

  entry = gtk_entry_new ();
  gtk_object_set_user_data (GTK_OBJECT (entry), amount_data);
  gtk_object_set_user_data (amount_data, entry);
  gtk_box_pack_start (GTK_BOX (hbox), entry, FALSE, TRUE, 0);
  gtk_widget_set_usize (entry, ENTRY_WIDTH, 0);
  sprintf (buffer, "%d", shvals.shift_amount);
  gtk_entry_set_text (GTK_ENTRY (entry), buffer);
  gtk_signal_connect (GTK_OBJECT (entry), "changed",
		      (GtkSignalFunc) shift_ientry_callback,
		      &shvals.shift_amount);
  gtk_widget_show (entry);

  gtk_widget_show (vbox);
  gtk_widget_show (frame);
  gtk_widget_show (dlg);

  gtk_main ();
  gdk_flush ();

  if (do_horizontal)
      shvals.orientation = HORIZONTAL;
  else
      shvals.orientation = VERTICAL;

  return shint.run;
}

/*****/

static GTile *
shift_pixel (GDrawable * drawable,
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




/*  Shift interface functions  */

static void
shift_close_callback (GtkWidget *widget,
		      gpointer   data)
{
  gtk_main_quit ();
}

static void
shift_ok_callback (GtkWidget *widget,
		   gpointer   data)
{
  shint.run = TRUE;
  gtk_widget_destroy (GTK_WIDGET (data));
}

static void
shift_toggle_update (GtkWidget *widget,
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
shift_ientry_callback (GtkWidget *widget,
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
	  gtk_signal_emit_by_name (GTK_OBJECT (adjustment), "value_change");
	}
    }
}

static void
shift_iscale_callback (GtkAdjustment *adjustment,
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
