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
#define HORIZONTAL 0
#define VERTICAL 1
#define ENTRY_WIDTH 35

typedef struct
{
  gint shift_amount;
  gint orientation;
} ShiftValues;

typedef struct
{
  gint run;
} ShiftInterface;


/* Declare local functions.
 */
static void    query  (void);
static void    run    (gchar    *name,
		       gint      nparams,
		       GParam   *param,
		       gint     *nreturn_vals,
		       GParam  **return_vals);

static void    shift  (GDrawable *drawable);

static gint    shift_dialog      (void);
static void    shift_ok_callback (GtkWidget *widget,
				  gpointer   data);

static GTile * shift_pixel (GDrawable *drawable,
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

static ShiftValues shvals =
{
  5,          /* shift amount */
  HORIZONTAL  /* orientation  */
};

static ShiftInterface shint =
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
    { PARAM_INT32, "shift_amount", "shift amount (0 <= shift_amount_x <= 200)" },
    { PARAM_INT32, "orientation", "vertical, horizontal orientation" },
  };
  static gint nargs = sizeof (args) / sizeof (args[0]);

  INIT_I18N();

  gimp_install_procedure ("plug_in_shift",
			  "Shift the contents of the specified drawable",
			  "Shifts the pixels of the specified drawable. Each row will be displaced a random value of pixels.",
			  "Spencer Kimball and Peter Mattis, ported by Brian Degenhardt and Federico Mena Quintero",
			  "Brian Degenhardt",
			  "1997",
			  N_("<Image>/Filters/Distorts/Shift..."),
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
      gimp_get_data ("plug_in_shift", &shvals);

      /*  First acquire information with a dialog  */
      if (! shift_dialog ())
	return;
      break;

    case RUN_NONINTERACTIVE:
      INIT_I18N();
      /*  Make sure all the arguments are there!  */
      if (nparams != 5)
	{
	  status = STATUS_CALLING_ERROR;
	}
      else
	{
	  shvals.shift_amount = param[3].data.d_int32;
          shvals.orientation = (param[4].data.d_int32) ? HORIZONTAL : VERTICAL;

	  if (shvals.shift_amount < 0 || shvals.shift_amount > 200)
	    status = STATUS_CALLING_ERROR;
	}
      break;

    case RUN_WITH_LAST_VALS:
      INIT_I18N();
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
	  gimp_progress_init ( _("Shifting..."));

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
shift_dialog (void)
{
  GtkWidget *dlg;
  GtkWidget *frame;
  GtkWidget *radio_vbox;
  GtkWidget *sep;
  GtkWidget *table;
  GtkObject *amount_data;
  gchar **argv;
  gint    argc;

  argc    = 1;
  argv    = g_new (gchar *, 1);
  argv[0] = g_strdup ("shift");

  gtk_init (&argc, &argv);
  gtk_rc_parse (gimp_gtkrc ());

  dlg = gimp_dialog_new (_("Shift"), "shift",
			 gimp_plugin_help_func, "filters/shift.html",
			 GTK_WIN_POS_MOUSE,
			 FALSE, TRUE, FALSE,

			 _("OK"), shift_ok_callback,
			 NULL, NULL, NULL, TRUE, FALSE,
			 _("Cancel"), gtk_widget_destroy,
			 NULL, 1, NULL, FALSE, TRUE,

			 NULL);

  gtk_signal_connect (GTK_OBJECT (dlg), "destroy",
		      GTK_SIGNAL_FUNC (gtk_main_quit),
		      NULL);

  /*  parameter settings  */
  frame = 
    gimp_radio_group_new2 (TRUE, _("Parameter Settings"),
			   gimp_radio_button_update,
			   &shvals.orientation, (gpointer) shvals.orientation,

			   _("Shift Horizontally"), (gpointer) HORIZONTAL, NULL,
			   _("Shift Vertically"),   (gpointer) VERTICAL, NULL,

			   NULL);
  gtk_container_set_border_width (GTK_CONTAINER (frame), 6);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dlg)->vbox), frame, TRUE, TRUE, 0);

  radio_vbox = GTK_BIN (frame)->child;
  gtk_container_set_border_width (GTK_CONTAINER (radio_vbox), 4);

  sep = gtk_hseparator_new ();
  gtk_box_pack_start (GTK_BOX (radio_vbox), sep, FALSE, FALSE, 3);
  gtk_widget_show (sep);

  table = gtk_table_new (1, 3, FALSE);
  gtk_table_set_col_spacings (GTK_TABLE (table), 4);
  gtk_box_pack_start (GTK_BOX (radio_vbox), table, FALSE, FALSE, 0);
  gtk_widget_show (table);

  amount_data = gimp_scale_entry_new (GTK_TABLE (table), 0, 0,
				      _("Shift Amount:"), SCALE_WIDTH, 0,
				      shvals.shift_amount, 0, 200, 1, 10, 0,
				      TRUE, 0, 0,
				      NULL, NULL);
  gtk_signal_connect (GTK_OBJECT (amount_data), "value_changed",
		      GTK_SIGNAL_FUNC (gimp_int_adjustment_update),
		      &shvals.shift_amount);

  gtk_widget_show (frame);

  gtk_widget_show (dlg);

  gtk_main ();
  gdk_flush ();

  return shint.run;
}

static void
shift_ok_callback (GtkWidget *widget,
		   gpointer   data)
{
  shint.run = TRUE;

  gtk_widget_destroy (GTK_WIDGET (data));
}

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
