/*
 * This is a plugin for the GIMP.
 *
 * Copyright (C) 1996 Stephen Norris
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
 */

/*
 * This plug-in produces plasma fractal images. The algorithm is losely
 * based on a description of the fractint algorithm, but completely
 * re-implemented because the fractint code was too ugly to read :)
 *
 * Please send any patches or suggestions to me: srn@flibble.cs.su.oz.au.
 */

/*
 * TODO:
 *	- The progress bar sucks.
 *	- It writes some pixels more than once.
 */

/* Version 1.01 */

/*
 * Ported to GIMP Plug-in API 1.0
 *    by Eiichi Takamori <taka@ma1.seikyou.ne.jp>
 *
 * $Id$
 *
 * A few functions names and their order are changed :)
 * Plasma implementation almost hasn't been changed.
 *
 * Feel free to correct my WRONG English, or to modify Plug-in Path,
 * and so on. ;-)
 */


#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <time.h>  /* For random seeding */
#include "config.h"
#include "gtk/gtk.h"
#include "libgimp/gimp.h"
#include "libgimp/gimpui.h"
#include "libgimp/stdplugins-intl.h"

/* Some useful macros */

#define ENTRY_WIDTH 75
#define SCALE_WIDTH 128
#define TILE_CACHE_SIZE 32

typedef struct {
    gint seed;
    gdouble turbulence;
     /* Interface only */
    gboolean timeseed;
} PlasmaValues;

typedef struct {
    gint run;
} PlasmaInterface;

/*
 * Function prototypes.
 */

static void	query	(void);
static void	run	(gchar   *name,
			 gint    nparams,
			 GParam  *param,
			 gint    *nreturn_vals,
			 GParam  **return_vals);

static gint	plasma_dialog (void);
static void     plasma_close_callback  (GtkWidget *widget,
					  gpointer   data);
static void     plasma_ok_callback     (GtkWidget *widget,
					  gpointer   data);
static void     plasma_entry_callback  (GtkWidget *widget,
					  gpointer   data);
static void     plasma_scale_update    (GtkAdjustment *adjustment,
					  gpointer   data);
static void     toggle_callback (GtkWidget *widget, gboolean *data);
static void	plasma	(GDrawable *drawable);
static void     random_rgb (guchar *d);
static void     add_random (guchar *d, gint amnt);
static void     init_plasma (GDrawable *drawable);
static void     provide_tile (GDrawable *drawable, gint col, gint row );
static void     end_plasma (GDrawable *drawable);
static void     get_pixel (GDrawable *drawable, gint x, gint y, guchar *pixel );
static void     put_pixel (GDrawable *drawable, gint x, gint y, guchar *pixel );
static gint     do_plasma (GDrawable *drawable, gint x1, gint y1,
			   gint x2, gint y2, gint depth, gint scale_depth );

/***** Local vars *****/

GPlugInInfo PLUG_IN_INFO =
{
  NULL,    /* init_proc */
  NULL,    /* quit_proc */
  query,   /* query_proc */
  run,     /* run_proc */
};

static PlasmaValues pvals =
{
    0,     /* seed */
    1.0,   /* turbulence */
    TRUE   /* Time seed? */
};

static PlasmaInterface pint =
{
  FALSE     /* run */
};

/***** Functions *****/

MAIN ()

static void
query()
{
  static GParamDef args[]=
    {
      { PARAM_INT32, "run_mode", "Interactive, non-interactive" },
      { PARAM_IMAGE, "image", "Input image (unused)" },
      { PARAM_DRAWABLE, "drawable", "Input drawable" },
      { PARAM_INT32, "seed", "Random seed" },
      { PARAM_FLOAT, "turbulence", "Turbulence of plasma" }
   };
  static GParamDef *return_vals = NULL;
  static gint nargs = sizeof (args) / sizeof (args[0]);
  static gint nreturn_vals = 0;

  INIT_I18N();

  gimp_install_procedure ("plug_in_plasma",
			  _("Create a plasma cloud like image to the specified drawable"),
			  "More help",
			  "Stephen Norris & (ported to 1.0 by) Eiichi Takamori",
			  "Stephen Norris",
			  "1995",
			  N_("<Image>/Filters/Render/Clouds/Plasma..."),
			  "RGB*, GRAY*",
			  PROC_PLUG_IN,
			  nargs, nreturn_vals,
			  args, return_vals);
}

static void
run (gchar   *name,
     gint    nparams,
     GParam  *param,
     gint    *nreturn_vals,
     GParam  **return_vals)
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

  /*  Get the specified drawable  */
  drawable = gimp_drawable_get (param[2].data.d_drawable);

  switch (run_mode)
    {
    case RUN_INTERACTIVE:
      INIT_I18N_UI();
      /*  Possibly retrieve data  */
      gimp_get_data ("plug_in_plasma", &pvals);

      /*  First acquire information with a dialog  */
      if (! plasma_dialog ())
	{
	  gimp_drawable_detach (drawable);
	  return;
	}
      break;

    case RUN_NONINTERACTIVE:
      INIT_I18N();
      /*  Make sure all the arguments are there!  */
      if (nparams != 5)
	status = STATUS_CALLING_ERROR;
      if (status == STATUS_SUCCESS)
	{
	  pvals.seed = (gint) param[3].data.d_int32;
	  pvals.turbulence = (gdouble) param[4].data.d_float;
	}
      if ((status == STATUS_SUCCESS) &&
	  pvals.turbulence <= 0 )
	status = STATUS_CALLING_ERROR;
      break;

    case RUN_WITH_LAST_VALS:
      INIT_I18N();
      /*  Possibly retrieve data  */
      gimp_get_data ("plug_in_plasma", &pvals);
      break;

    default:
      break;
    }

  if (status == STATUS_SUCCESS)
    {
      /*  Make sure that the drawable is gray or RGB color  */
      if (gimp_drawable_is_rgb (drawable->id) || gimp_drawable_is_gray (drawable->id))
	{
	  gimp_progress_init ( _("Plasma..."));
	  gimp_tile_cache_ntiles (TILE_CACHE_SIZE);

	  plasma (drawable);

	  if (run_mode != RUN_NONINTERACTIVE)
	    gimp_displays_flush ();

	  /*  Store data  */
	  if (run_mode == RUN_INTERACTIVE || 
	      (pvals.timeseed && run_mode == RUN_WITH_LAST_VALS))
	    gimp_set_data ("plug_in_plasma", &pvals, sizeof (PlasmaValues));
	}
      else
	{
	  /* gimp_message ("plasma: cannot operate on indexed color images"); */
	  status = STATUS_EXECUTION_ERROR;
	}
    }

  values[0].data.d_status = status;

  gimp_drawable_detach (drawable);
}

static gint
plasma_dialog()
{
  GtkWidget *dlg;
  GtkWidget *frame;
  GtkWidget *table;
  GtkWidget *hbbox;
  GtkWidget *button;
  GtkWidget *label;
  GtkWidget *entry;
  GtkWidget *seed_hbox;
  GtkWidget *time_button;
  GtkWidget *scale;
  GtkObject *scale_data;
  gchar **argv;
  gint  argc;
  guchar buffer[32];

  argc = 1;
  argv = g_new (gchar *, 1);
  argv[0] = g_strdup ("plasma");

  gtk_init (&argc, &argv);
  gtk_rc_parse (gimp_gtkrc ());

  dlg = gtk_dialog_new ();
  gtk_window_set_title (GTK_WINDOW (dlg), _("Plasma"));
  gtk_window_position (GTK_WINDOW (dlg), GTK_WIN_POS_MOUSE);
  gtk_signal_connect (GTK_OBJECT (dlg), "destroy",
		      (GtkSignalFunc) plasma_close_callback,
		      NULL);

  /*  Action area  */
  gtk_container_set_border_width (GTK_CONTAINER (GTK_DIALOG (dlg)->action_area), 2);
  gtk_box_set_homogeneous (GTK_BOX (GTK_DIALOG (dlg)->action_area), FALSE);
  hbbox = gtk_hbutton_box_new ();
  gtk_button_box_set_spacing (GTK_BUTTON_BOX (hbbox), 4);
  gtk_box_pack_end (GTK_BOX (GTK_DIALOG (dlg)->action_area), hbbox, FALSE, FALSE, 0);
  gtk_widget_show (hbbox);
 
  button = gtk_button_new_with_label ( _("OK"));
  GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT);
  gtk_signal_connect (GTK_OBJECT (button), "clicked",
		      (GtkSignalFunc) plasma_ok_callback,
		      dlg);
  gtk_box_pack_start (GTK_BOX (hbbox), button, FALSE, FALSE, 0);
  gtk_widget_grab_default (button);
  gtk_widget_show (button);

  button = gtk_button_new_with_label ( _("Cancel"));
  GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT);
  gtk_signal_connect_object (GTK_OBJECT (button), "clicked",
			     (GtkSignalFunc) gtk_widget_destroy,
			     GTK_OBJECT (dlg));
  gtk_box_pack_start (GTK_BOX (hbbox), button, FALSE, FALSE, 0);
  gtk_widget_show (button);

  /*  parameter settings  */
  frame = gtk_frame_new ( _("Plasma Options"));
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_ETCHED_IN);
  gtk_container_border_width (GTK_CONTAINER (frame), 10);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dlg)->vbox), frame, TRUE, TRUE, 0);
  table = gtk_table_new (2, 2, FALSE);
  gtk_container_border_width (GTK_CONTAINER (table), 10);
  gtk_container_add (GTK_CONTAINER (frame), table);

  label = gtk_label_new ( _("Seed"));
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gtk_table_attach (GTK_TABLE (table), label, 0, 1, 0, 1, GTK_FILL, 0, 5, 0 );
  gtk_widget_show (label);

  seed_hbox = gtk_hbox_new(FALSE, 2);
  gtk_table_attach (GTK_TABLE (table), seed_hbox, 1, 2, 0, 1, 
		    GTK_FILL, GTK_FILL, 0, 0 );
  gtk_widget_show (seed_hbox);
  
  entry = gtk_entry_new ();
  gtk_box_pack_start(GTK_BOX(seed_hbox), entry, TRUE, TRUE, 0);
  gtk_widget_set_usize( entry, ENTRY_WIDTH, 0 );
  sprintf( (char *)buffer, "%d", pvals.seed );
  gtk_entry_set_text (GTK_ENTRY (entry), (gchar *)buffer );
  gtk_signal_connect (GTK_OBJECT (entry), "changed",
		      (GtkSignalFunc) plasma_entry_callback,
		      &pvals.seed);
  gtk_widget_show (entry);

  time_button = gtk_toggle_button_new_with_label ( _("Time"));
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(time_button),pvals.timeseed);
  gtk_signal_connect (GTK_OBJECT (time_button), "clicked",
		      (GtkSignalFunc) toggle_callback,
		      &pvals.timeseed);
  gtk_box_pack_end (GTK_BOX (seed_hbox), time_button, FALSE, FALSE, 0);
  gtk_widget_show (time_button);
  gtk_widget_show (seed_hbox);

  label = gtk_label_new ( _("Turbulence"));
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gtk_table_attach (GTK_TABLE (table), label, 0, 1, 1, 2, GTK_FILL, 0, 5, 0);
  gtk_widget_show (label);

  scale_data = gtk_adjustment_new (pvals.turbulence, 0.1, 7.0, 0.1, 0.1, 0.0);
  scale = gtk_hscale_new (GTK_ADJUSTMENT (scale_data));
  gtk_widget_set_usize (scale, SCALE_WIDTH, 0);
  gtk_table_attach (GTK_TABLE (table), scale, 1, 2, 1, 2, GTK_FILL, 0, 0, 0);
  gtk_scale_set_value_pos (GTK_SCALE (scale), GTK_POS_TOP);
  gtk_scale_set_digits (GTK_SCALE (scale), 1);
  gtk_range_set_update_policy (GTK_RANGE (scale), GTK_UPDATE_DELAYED);
  gtk_signal_connect (GTK_OBJECT (scale_data), "value_changed",
		      (GtkSignalFunc) plasma_scale_update,
			  &pvals.turbulence);
  gtk_widget_show (scale);

  gtk_widget_show (frame);
  gtk_widget_show (table);
  gtk_widget_show (dlg);

  gtk_main ();
  gdk_flush ();

  return pint.run;

}


/*  Plasma interface functions  */

static void
plasma_close_callback (GtkWidget *widget,
			 gpointer   data)
{
  gtk_main_quit ();
}

static void
plasma_ok_callback (GtkWidget *widget,
		      gpointer   data)
{
  pint.run = TRUE;
  gtk_widget_destroy (GTK_WIDGET (data));
}

static void
plasma_entry_callback (GtkWidget *widget,
			 gpointer   data)
{
  gint *text_val;

  text_val = (gint *) data;

  *text_val = atoi (gtk_entry_get_text (GTK_ENTRY (widget)));
}


static void
plasma_scale_update (GtkAdjustment *adjustment,
		       gpointer      data)
{
  gdouble *dptr = (gdouble*) data;
  *dptr = adjustment->value;
}

static void 
toggle_callback (GtkWidget *widget, gboolean *data)
{
    *data = GTK_TOGGLE_BUTTON (widget)->active;
}

#define AVE(n, v1, v2) n[0] = ((gint)v1[0] + (gint)v2[0]) / 2;\
	n[1] = ((gint)v1[1] + (gint)v2[1]) / 2;\
	n[2] = ((gint)v1[2] + (gint)v2[2]) / 2;

/*
 * Some glabals to save passing too many paramaters that don't change.
 */

gint	ix1, iy1, ix2, iy2;	/* Selected image size. */
GTile   *tile=NULL;
gint    tile_row, tile_col;
gint    tile_width, tile_height;
gint    tile_dirty;
gint	bpp, has_alpha, alpha;
gdouble	turbulence;
glong	max_progress, progress;

/*
 * The setup function.
 */

static void
plasma(GDrawable *drawable)
{
    gint  depth;

    init_plasma(drawable);

    /*
     * This first time only puts in the seed pixels - one in each
     * corner, and one in the center of each edge, plus one in the
     * center of the image.
     */

    do_plasma( drawable, ix1, iy1, ix2 - 1, iy2 - 1, -1, 0);

    /*
     * Now we recurse through the images, going further each time.
     */
    depth = 1;
    while (!do_plasma( drawable, ix1, iy1, ix2 - 1, iy2 - 1, depth, 0)){
	depth ++;
    }
    end_plasma( drawable );
}

static void
init_plasma( GDrawable *drawable )
{
     if (pvals.timeseed)
	  pvals.seed = time(NULL);

    srand( pvals.seed );
    turbulence = pvals.turbulence;

    gimp_drawable_mask_bounds(drawable->id, &ix1, &iy1, &ix2, &iy2);

    max_progress = (ix2 - ix1) * (iy2 - iy1);
    progress = 0;

    tile_width = gimp_tile_width ();
    tile_height = gimp_tile_height ();

    tile = NULL;
    tile_row = 0; tile_col = 0;

    bpp = drawable->bpp;
    has_alpha = gimp_drawable_has_alpha( drawable->id );
    if( has_alpha )
      alpha = bpp-1;
    else
      alpha = bpp;
}

static void
provide_tile( GDrawable *drawable, gint col, gint row )
{
    if ( col != tile_col || row != tile_row || !tile )
      {
	  if( tile )
	    gimp_tile_unref( tile, tile_dirty );
	  tile_col = col;
	  tile_row = row;
	  tile = gimp_drawable_get_tile( drawable, TRUE, tile_row, tile_col );
	  tile_dirty = FALSE;
	  gimp_tile_ref( tile );
      }
}

static void
end_plasma( GDrawable *drawable )
{
    if( tile )
      gimp_tile_unref( tile, tile_dirty );
    tile=NULL;

    gimp_drawable_flush (drawable);
    gimp_drawable_merge_shadow (drawable->id, TRUE);
    gimp_drawable_update (drawable->id, ix1, iy1, (ix2 - ix1), (iy2 - iy1));
}

static void
get_pixel( GDrawable *drawable, gint x, gint y, guchar *pixel )
{
    gint   row, col;
    gint   offx, offy, i;
    guchar  *ptr;

    if (x < ix1)       x = ix1;
    if (x > ix2 - 1)   x = ix2 - 1;
    if (y < iy1)       y = iy1;
    if (y > iy2 - 1)   y = iy2 - 1;

    col = x / tile_width;
    row = y / tile_height;
    offx = x % tile_width;
    offy = y % tile_height;

    provide_tile( drawable, col, row );

    ptr = tile->data + ( offy * tile->ewidth + offx ) * bpp;
    for( i = 0; i < alpha; i++ )
      pixel[i] = ptr[i];
}

static void
put_pixel( GDrawable *drawable, gint x, gint y, guchar *pixel )
{
    gint   row, col;
    gint   offx, offy, i;
    guchar  *ptr;

    if (x < ix1)       x = ix1;
    if (x > ix2 - 1)   x = ix2 - 1;
    if (y < iy1)       y = iy1;
    if (y > iy2 - 1)   y = iy2 - 1;

    col = x / tile_width;
    row = y / tile_height;
    offx = x % tile_width;
    offy = y % tile_height;

    provide_tile( drawable, col, row );

    ptr = tile->data + ( offy * tile->ewidth + offx ) * bpp;
    for( i = 0; i < alpha; i++ )
      ptr[i] = pixel[i];
    if( has_alpha )
      ptr[alpha] = 255;

    tile_dirty = TRUE;

    progress++;
}

static void
random_rgb(guchar *d)
{
    gint i;
    for( i = 0; i < alpha; i++ ) {
	d[i] = rand() % 256;
    }
}

static void
add_random(guchar *d, gint amnt)
{
    gint	i, tmp;

    for (i = 0; i < alpha; i++){
	if (amnt == 0){
	    amnt = 1;
	}
	tmp = amnt/2 - rand() % amnt;

	if ((gint)d[i] + tmp < 0){
	    d[i] = 0;
	} else if ((gint)d[i] + tmp > 255){
	    d[i] = 255;
	} else {
	    d[i] += tmp;
	}
    }
}

static gint
do_plasma( GDrawable *drawable, gint x1, gint y1, gint x2, gint y2,
	  gint depth, gint scale_depth)
{
	guchar		tl[3], ml[3], bl[3], mt[3], mm[3], mb[3], tr[3], mr[3], br[3];
	guchar		tmp[3];
	gint		ran;
	gint		xm, ym;
	static gint	count = 0;

	/* Initial pass through - no averaging. */

	if (depth == -1){
		random_rgb(tl);
		put_pixel( drawable, x1, y1, tl);
		random_rgb(tr);
		put_pixel( drawable, x2, y1, tr);
		random_rgb(bl);
		put_pixel( drawable, x1, y2, bl);
		random_rgb(br);
		put_pixel( drawable, x2, y2, br);
		random_rgb(mm);
		put_pixel( drawable, (x1 + x2) / 2, (y1 + y2) / 2, mm);
		random_rgb(ml);
		put_pixel( drawable, x1, (y1 + y2) / 2, ml);
		random_rgb(mr);
		put_pixel( drawable, x2, (y1 + y2) / 2, mr);
		random_rgb(mt);
		put_pixel( drawable, (x1 + x2) / 2, y1, mt);
		random_rgb(ml);
		put_pixel( drawable, (x1 + x2) / 2, y2, ml);

		return 0;
	}

	/*
	 * Some later pass, at the bottom of this pass,
	 * with averaging at this depth.
	 */
	if (depth == 0){
		gdouble	rnd;
		gint	xave, yave;

		get_pixel( drawable, x1, y1, tl);
		get_pixel( drawable, x1, y2, bl);
		get_pixel( drawable, x2, y1, tr);
		get_pixel( drawable, x2, y2, br);

		rnd = (256.0 / (2.0 * (gdouble)scale_depth)) * turbulence;
		ran = rnd;

		xave = (x1 + x2) / 2;
		yave = (y1 + y2) / 2;

		if (xave == x1 && xave == x2 && yave == y1 && yave == y2){
			return 0;
		}

		if (xave != x1 || xave != x2){
			/* Left. */
			AVE(ml, tl, bl);
			add_random(ml, ran);
			put_pixel( drawable, x1, yave, ml);

			if (x1 != x2){
				/* Right. */
				AVE(mr, tr, br);
				add_random(mr, ran);
				put_pixel( drawable, x2, yave, mr);
			}
		}

		if (yave != y1 || yave != y2){
			if (x1 != xave || yave != y2){
				/* Bottom. */
				AVE(mb, bl, br);
				add_random(mb, ran);
				put_pixel( drawable, xave, y2, mb);
			}

			if (y1 != y2){
				/* Top. */
				AVE(mt, tl, tr);
				add_random(mt, ran);
				put_pixel( drawable, xave, y1, mt);
			}
		}

		if (y1 != y2 || x1 != x2){
			/* Middle pixel. */
			AVE(mm, tl, br);
			AVE(tmp, bl, tr);
			AVE(mm, mm, tmp);

			add_random(mm, ran);
			put_pixel( drawable, xave, yave, mm);
		}

		count ++;

		if (!(count % 2000)){
			gimp_progress_update( (double) progress / (double) max_progress);
		}

		if ((x2 - x1) < 3 && (y2 - y1) < 3){
			return 1;
		}
		return 0;
	}

	xm = (x1 + x2) >> 1;
	ym = (y1 + y2) >> 1;

	/* Top left. */
	do_plasma( drawable, x1, y1, xm, ym, depth - 1, scale_depth + 1);
	/* Bottom left. */
	do_plasma( drawable, x1, ym, xm ,y2, depth - 1, scale_depth + 1);
	/* Top right. */
	do_plasma( drawable, xm, y1, x2 , ym, depth - 1, scale_depth + 1);
	/* Bottom right. */
	return do_plasma( drawable, xm, ym, x2, y2, depth - 1, scale_depth + 1);
}
