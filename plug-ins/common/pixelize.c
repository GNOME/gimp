/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * Pixelize plug-in (ported to GIMP v1.0)
 * Copyright (C) 1997 Eiichi Takamori <taka@ma1.seikyou.ne.jp>
 * original pixelize.c for GIMP 0.54 by Tracy Scott
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

/*
 * version 1.04
 * This version requires GIMP v0.99.10 or above.
 *
 * This plug-in "pixelizes" the image.
 *
 *	Eiichi Takamori <taka@ma1.seikyou.ne.jp>
 *	http://ha1.seikyou.ne.jp/home/taka/gimp/
 *
 * Changes from version 1.03 to version 1.04:
 * - Added gtk_rc_parse
 * - Added entry with scale
 * - Fixed bug that large pixelwidth >=64 sometimes caused core dump
 *
 * Changes from gimp-0.99.9/plug-ins/pixelize.c to version 1.03:
 * - Fixed comments and help strings
 * - Fixed `RGB, GRAY' to `RGB*, GRAY*'
 * - Fixed procedure db name `pixelize' to `plug_in_pixelize'
 *
 * Differences from Tracy Scott's original `pixelize' plug-in:
 *
 * - Algorithm is modified to work around with the tile management.
 *   The way of pixelizing is switched by the value of pixelwidth.  If
 *   pixelwidth is greater than (or equal to) tile width, then this
 *   plug-in makes GPixelRgn with that width and proceeds. Otherwise,
 *   it makes the region named `PixelArea', whose size is smaller than
 *   tile width and is multiply of pixel width, and acts onto it.
 */

/* pixelize filter written for the GIMP
 *  -Tracy Scott
 *
 * This filter acts as a low pass filter on the color components of
 * the provided region
 */


#include <stdio.h>
#include <stdlib.h>
#include "gtk/gtk.h"
#include "libgimp/gimp.h"
#include "libgimp/gimpui.h"

#ifdef RCSID
static char rcsid[] = "$Id$";
#endif


/* Some useful macros */

#define TILE_CACHE_SIZE 16
#define ENTSCALE_INT_SCALE_WIDTH 125
#define ENTSCALE_INT_ENTRY_WIDTH 40

typedef struct {
  gint pixelwidth;
} PixelizeValues;

typedef struct {
  gint run;
} PixelizeInterface;

typedef struct {
  gint x, y, w, h;
  gint width;
  guchar *data;
} PixelArea;

typedef void (*EntscaleIntCallbackFunc) (gint value, gpointer data);

typedef struct {
  GtkObject     *adjustment;
  GtkWidget     *entry;
  gint          constraint;
  EntscaleIntCallbackFunc	callback;
  gpointer	call_data;
} EntscaleIntData;

/* Declare local functions.
 */
static void	query	(void);
static void	run	(gchar	 *name,
			 gint	 nparams,
			 GParam	 *param,
			 gint	 *nreturn_vals,
			 GParam	 **return_vals);

static gint	pixelize_dialog (void);
static void	pixelize_close_callback	 (GtkWidget *widget,
					  gpointer   data);
static void	pixelize_ok_callback	 (GtkWidget *widget,
					  gpointer   data);

static void	pixelize	(GDrawable *drawable);
static void	pixelize_large	(GDrawable *drawable, gint pixelwidth);
static void	pixelize_small	(GDrawable *drawable, gint pixelwidth, gint tile_width);
static void	pixelize_sub ( gint pixelwidth, gint bpp );

void   entscale_int_new ( GtkWidget *table, gint x, gint y,
			  gchar *caption, gint *intvar, 
			  gint min, gint max, gint constraint,
			  EntscaleIntCallbackFunc callback,
			  gpointer data );
static void   entscale_int_destroy_callback (GtkWidget *widget,
					     gpointer data);
static void   entscale_int_scale_update (GtkAdjustment *adjustment,
					 gpointer      data);
static void   entscale_int_entry_update (GtkWidget *widget,
					 gpointer   data);



/***** Local vars *****/

GPlugInInfo PLUG_IN_INFO =
{
  NULL,	   /* init_proc */
  NULL,	   /* quit_proc */
  query,   /* query_proc */
  run	   /* run_proc */
};

static PixelizeValues pvals =
{
  10
};

static PixelizeInterface pint =
{
  FALSE	    /* run */
};

static PixelArea area;

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
      { PARAM_INT32, "pixelwidth", "Pixel width	 (the decrease in resolution)" }
   };
  static GParamDef *return_vals = NULL;
  static gint nargs = sizeof (args) / sizeof (args[0]);
  static gint nreturn_vals = 0;

  gimp_install_procedure ("plug_in_pixelize",
			  "Pixelize the contents of the specified drawable",
			  "Pixelize the contents of the specified drawable with speficied pixelizing width.",
			  "Spencer Kimball & Peter Mattis, Tracy Scott, (ported to 1.0 by) Eiichi Takamori",
			  "Spencer Kimball & Peter Mattis, Tracy Scott",
			  "1995",
			  "<Image>/Filters/Image/Pixelize",
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
      /*  Possibly retrieve data  */
      gimp_get_data ("plug_in_pixelize", &pvals);

      /*  First acquire information with a dialog  */
      if (! pixelize_dialog ())
	{
	  gimp_drawable_detach (drawable);
	  return;
	}
      break;

    case RUN_NONINTERACTIVE:
      /*  Make sure all the arguments are there!  */
      if (nparams != 4)
	status = STATUS_CALLING_ERROR;
      if (status == STATUS_SUCCESS)
	{
	  pvals.pixelwidth = (gdouble) param[3].data.d_int32;
	}
      if ((status == STATUS_SUCCESS) &&
	  pvals.pixelwidth <= 0 )
	status = STATUS_CALLING_ERROR;
      break;

    case RUN_WITH_LAST_VALS:
      /*  Possibly retrieve data  */
      gimp_get_data ("plug_in_pixelize", &pvals);
      break;

    default:
      break;
    }

  if (status == STATUS_SUCCESS)
    {
      /*  Make sure that the drawable is gray or RGB color  */
      if (gimp_drawable_color (drawable->id) || gimp_drawable_gray (drawable->id))
	{
	  gimp_progress_init ("Pixelizing...");

	  /*  set the tile cache size  */
	  gimp_tile_cache_ntiles (TILE_CACHE_SIZE);

	  /*  run the pixelize effect  */
	  pixelize (drawable);

	  if (run_mode != RUN_NONINTERACTIVE)
	    gimp_displays_flush ();

	  /*  Store data  */
	  if (run_mode == RUN_INTERACTIVE)
	    gimp_set_data ("plug_in_pixelize", &pvals, sizeof (PixelizeValues));
	}
      else
	{
	  /* gimp_message ("pixelize: cannot operate on indexed color images"); */
	  status = STATUS_EXECUTION_ERROR;
	}
    }

  values[0].data.d_status = status;

  gimp_drawable_detach (drawable);
}


static gint
pixelize_dialog ()
{
  GtkWidget *dlg;
  GtkWidget *frame;
  GtkWidget *table;
  GtkWidget *button;
  gchar **argv;
  gint	argc;

  argc = 1;
  argv = g_new (gchar *, 1);
  argv[0] = g_strdup ("pixelize");

  gtk_init (&argc, &argv);
  gtk_rc_parse (gimp_gtkrc ());


  dlg = gtk_dialog_new ();
  gtk_window_set_title (GTK_WINDOW (dlg), "Pixelize");
  gtk_window_position (GTK_WINDOW (dlg), GTK_WIN_POS_MOUSE);
  gtk_signal_connect (GTK_OBJECT (dlg), "destroy",
		      (GtkSignalFunc) pixelize_close_callback,
		      NULL);

  /*  Action area  */
  button = gtk_button_new_with_label ("OK");
  GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT);
  gtk_signal_connect (GTK_OBJECT (button), "clicked",
		      (GtkSignalFunc) pixelize_ok_callback,
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

  /*  parameter settings  */
  frame = gtk_frame_new ("Parameter Settings");
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_ETCHED_IN);
  gtk_container_border_width (GTK_CONTAINER (frame), 10);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dlg)->vbox), frame, TRUE, TRUE, 0);

  table = gtk_table_new (3, 2, FALSE);
  gtk_container_border_width (GTK_CONTAINER (table), 10);
  gtk_container_add (GTK_CONTAINER (frame), table);

  entscale_int_new (table, 0, 0, "Pixel Width:", &pvals.pixelwidth,
		    1, 64, FALSE,
		    NULL, NULL);

  gtk_widget_show (frame);
  gtk_widget_show (table);
  gtk_widget_show (dlg);

  gtk_main ();
  gdk_flush ();

  return pint.run;
}

/*  Pixelize interface functions  */

static void
pixelize_close_callback (GtkWidget *widget,
			 gpointer   data)
{
  gtk_main_quit ();
}

static void
pixelize_ok_callback (GtkWidget *widget,
		      gpointer	 data)
{
  pint.run = TRUE;
  gtk_widget_destroy (GTK_WIDGET (data));
}

/*
  Pixelize Effect
 */

static void
pixelize (GDrawable *drawable)
{
  gint tile_width;
  gint pixelwidth;

  tile_width = gimp_tile_width();
  pixelwidth = pvals.pixelwidth;

  if (pixelwidth < 0)
    pixelwidth = - pixelwidth;
  if (pixelwidth < 1)
    pixelwidth = 1;

  if( pixelwidth >= tile_width )
    pixelize_large( drawable, pixelwidth );
  else
    pixelize_small( drawable, pixelwidth, tile_width );
}

/*
  This function operates on the image when pixelwidth >= tile_width.
  It simply sets the size of GPixelRgn as pixelwidth and proceeds.
 */
static void
pixelize_large (GDrawable *drawable, gint pixelwidth )
{
  GPixelRgn src_rgn, dest_rgn;
  guchar *src_row, *dest_row;
  guchar *src, *dest;
  gulong *average;
  gint row, col, b, bpp;
  gint x, y, x_step, y_step;
  gulong count;
  gint x1, y1, x2, y2;
  gint progress, max_progress;
  gpointer pr;

  gimp_drawable_mask_bounds (drawable->id, &x1, &y1, &x2, &y2);

  bpp = gimp_drawable_bpp( drawable->id );
  average = g_new( gulong, bpp );

  /* Initialize progress */
  progress = 0;
  max_progress = 2 * (x2 - x1) * (y2 - y1);

  for( y = y1; y < y2; y += pixelwidth - ( y % pixelwidth ) )
    {
      for( x = x1; x < x2; x += pixelwidth - ( x % pixelwidth ) )
	{
	  x_step = pixelwidth - ( x % pixelwidth );
	  y_step = pixelwidth - ( y % pixelwidth );
	  x_step = MIN( x_step, x2-x );
	  y_step = MIN( y_step, y2-y );

	  gimp_pixel_rgn_init (&src_rgn, drawable, x, y, x_step, y_step, FALSE, FALSE);
	  for( b = 0; b < bpp;	b++ )
	    average[b] = 0;
	  count = 0;

	  for (pr = gimp_pixel_rgns_register (1, &src_rgn); pr != NULL; pr = gimp_pixel_rgns_process (pr))
	    {
	      src_row = src_rgn.data;
	      for (row = 0; row < src_rgn.h; row++)
		{
		  src = src_row;
		  for (col = 0; col < src_rgn.w; col++)
		    {
		      for( b = 0; b < bpp; b++ )
			average[b] += src[b];
		      src += src_rgn.bpp;
		      count += 1;
		    }
		  src_row += src_rgn.rowstride;
		}
	      /* Update progress */
	      progress += src_rgn.w * src_rgn.h;
	      gimp_progress_update ((double) progress / (double) max_progress);
	    }

	  if ( count > 0 )
	    {
	      for ( b = 0; b < bpp; b++ )
		average[b] = (guchar) ( average[b] / count );
	    }

	  gimp_pixel_rgn_init (&dest_rgn, drawable, x, y, x_step, y_step, TRUE, TRUE);
	  for (pr = gimp_pixel_rgns_register (1, &dest_rgn); pr != NULL; pr = gimp_pixel_rgns_process (pr))
	    {
	      dest_row = dest_rgn.data;
	      for (row = 0; row < dest_rgn.h; row++)
		{
		  dest = dest_row;
		  for (col = 0; col < dest_rgn.w; col++)
		    {
		      for( b = 0; b < bpp; b++ )
			dest[b] = average[b];
		      dest += dest_rgn.bpp;
		      count += 1;
		    }
		  dest_row += dest_rgn.rowstride;
		}
	      /* Update progress */
	      progress += dest_rgn.w * dest_rgn.h;
	      gimp_progress_update ((double) progress / (double) max_progress);
	    }
	}
    }

  g_free( average );

  /*  update the blurred region	 */
  gimp_drawable_flush (drawable);
  gimp_drawable_merge_shadow (drawable->id, TRUE);
  gimp_drawable_update (drawable->id, x1, y1, (x2 - x1), (y2 - y1));
}


/*
   This function operates on PixelArea, whose width and height are
   multiply of pixel width, and less than the tile size (to enhance
   its speed).

   If any coordinates of mask boundary is not multiply of pixel width
   ( e.g.  x1 % pixelwidth != 0 ), operates on the region whose width
   or height is the remainder.
 */
static void
pixelize_small ( GDrawable *drawable, gint pixelwidth, gint tile_width )
{
  GPixelRgn src_rgn, dest_rgn;
  gint bpp;
  gint x1, y1, x2, y2;
  gint progress, max_progress;

  gimp_drawable_mask_bounds (drawable->id, &x1, &y1, &x2, &y2);
  gimp_pixel_rgn_init (&src_rgn, drawable, x1, y1, x2-x1, y2-y1, FALSE, FALSE);
  gimp_pixel_rgn_init (&dest_rgn, drawable, x1, y1, x2-x1, y2-y1, TRUE, TRUE);

  /* Initialize progress */
  progress = 0;
  max_progress = (x2 - x1) * (y2 - y1);

  bpp = drawable->bpp;

  area.width = ( tile_width / pixelwidth ) * pixelwidth;
  area.data= g_new( guchar, (glong) bpp * area.width * area.width  );

  for( area.y = y1; area.y < y2;
       area.y += area.width - ( area.y % area.width ) )
    {
      area.h = area.width - ( area.y % area.width );
      area.h = MIN( area.h, y2 - area.y );
      for( area.x = x1; area.x < x2;
	   area.x += area.width - ( area.x % area.width ) )
	{
	  area.w = area.width - ( area.x % area.width );
	  area.w = MIN( area.w, x2 - area.x );

	  gimp_pixel_rgn_get_rect( &src_rgn, area.data, area.x, area.y, area.w, area.h );

	  pixelize_sub( pixelwidth, bpp );

	  gimp_pixel_rgn_set_rect( &dest_rgn, area.data, area.x, area.y, area.w, area.h );

	  /* Update progress */
	  progress += area.w * area.h;
	  gimp_progress_update ((double) progress / (double) max_progress);
	}
    }

  g_free( area.data );

  /*  update the pixelized region  */
  gimp_drawable_flush (drawable);
  gimp_drawable_merge_shadow (drawable->id, TRUE);
  gimp_drawable_update (drawable->id, x1, y1, (x2 - x1), (y2 - y1));
}

/*
  This function acts on one PixelArea.	Since there were so many
  nested FORs in pixelize_small(), I put a few of them here...
  */

static void
pixelize_sub( gint pixelwidth, gint bpp )
{
  glong average[4];		/* bpp <= 4 */
  gint	x, y, w, h;
  guchar *buf_row, *buf;
  gint	row, col;
  gint	rowstride;
  gint	count;
  gint	i;

  rowstride = area.w * bpp;

  for( y = area.y; y < area.y + area.h; y += pixelwidth - ( y % pixelwidth ) )
    {
      h = pixelwidth - ( y % pixelwidth );
      h = MIN( h, area.y + area.h - y );
      for( x = area.x; x < area.x + area.w; x += pixelwidth - ( x % pixelwidth ) )
	{
	  w = pixelwidth - ( x % pixelwidth );
	  w = MIN( w, area.x + area.w - x );

	  for( i = 0; i < bpp; i++ )
	    average[i] = 0;
	  count = 0;

	  /* Read */
	  buf_row = area.data + (y-area.y)*rowstride + (x-area.x)*bpp;

	  for( row = 0; row < h; row++ )
	    {
	      buf = buf_row;
	      for( col = 0; col < w; col++ )
		{
		  for( i = 0; i < bpp; i++ )
		    average[i] += buf[i];
		  count++;
		  buf += bpp;
		}
	      buf_row += rowstride;
	    }

	  /* Average */
	  if ( count > 0 )
	    {
	      for( i = 0; i < bpp; i++ )
		average[i] /= count;
	    }

	  /* Write */
	  buf_row = area.data + (y-area.y)*rowstride + (x-area.x)*bpp;

	  for( row = 0; row < h; row++ )
	    {
	      buf = buf_row;
	      for( col = 0; col < w; col++ )
		{
		  for( i = 0; i < bpp; i++ )
		    buf[i] = average[i];
		  count++;
		  buf += bpp;
		}
	      buf_row += rowstride;
	    }
	}
    }
}

/* ====================================================================== */

/*
  Entry and Scale pair 1.03

  TODO:
  - Do the proper thing when the user changes value in entry,
  so that callback should not be called when value is actually not changed.
  - Update delay
 */

/*
 *  entscale: create new entscale with label. (int)
 *  1 row and 2 cols of table are needed.
 *  Input:
 *    x, y:       starting row and col in table
 *    caption:    label string
 *    intvar:     pointer to variable
 *    min, max:   the boundary of scale
 *    constraint: (bool) true iff the value of *intvar should be constraint
 *                by min and max
 *    callback:	  called when the value is actually changed
 *    call_data:  data for callback func
 */
void
entscale_int_new ( GtkWidget *table, gint x, gint y,
		   gchar *caption, gint *intvar,
		   gint min, gint max, gint constraint,
		   EntscaleIntCallbackFunc callback,
		   gpointer call_data)
{
  EntscaleIntData *userdata;
  GtkWidget *hbox;
  GtkWidget *label;
  GtkWidget *entry;
  GtkWidget *scale;
  GtkObject *adjustment;
  gchar    buffer[256];
  gint	    constraint_val;

  userdata = g_new ( EntscaleIntData, 1 );

  label = gtk_label_new (caption);
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);

  /*
    If the first arg of gtk_adjustment_new() isn't between min and
    max, it is automatically corrected by gtk later with
    "value_changed" signal. I don't like this, since I want to leave
    *intvar untouched when `constraint' is false.
    The lines below might look oppositely, but this is OK.
   */
  userdata->constraint = constraint;
  if( constraint )
    constraint_val = *intvar;
  else
    constraint_val = ( *intvar < min ? min : *intvar > max ? max : *intvar );

  userdata->adjustment = adjustment = 
    gtk_adjustment_new ( constraint_val, min, max, 1.0, 1.0, 0.0);
  scale = gtk_hscale_new ( GTK_ADJUSTMENT(adjustment) );
  gtk_widget_set_usize (scale, ENTSCALE_INT_SCALE_WIDTH, 0);
  gtk_scale_set_draw_value (GTK_SCALE (scale), FALSE);

  userdata->entry = entry = gtk_entry_new ();
  gtk_widget_set_usize (entry, ENTSCALE_INT_ENTRY_WIDTH, 0);
  sprintf( buffer, "%d", *intvar );
  gtk_entry_set_text( GTK_ENTRY (entry), buffer );

  userdata->callback = callback;
  userdata->call_data = call_data;

  /* userdata is done */
  gtk_object_set_user_data (GTK_OBJECT(adjustment), userdata);
  gtk_object_set_user_data (GTK_OBJECT(entry), userdata);

  /* now ready for signals */
  gtk_signal_connect (GTK_OBJECT (entry), "changed",
		      (GtkSignalFunc) entscale_int_entry_update,
		      intvar);
  gtk_signal_connect (GTK_OBJECT (adjustment), "value_changed",
		      (GtkSignalFunc) entscale_int_scale_update,
		      intvar);
  gtk_signal_connect (GTK_OBJECT (entry), "destroy",
		      (GtkSignalFunc) entscale_int_destroy_callback,
		      userdata );

  /* start packing */
  hbox = gtk_hbox_new (FALSE, 5);
  gtk_box_pack_start (GTK_BOX (hbox), scale, TRUE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (hbox), entry, FALSE, TRUE, 0);

  gtk_table_attach (GTK_TABLE (table), label, x, x+1, y, y+1,
		    GTK_FILL, GTK_FILL, 0, 0);
  gtk_table_attach (GTK_TABLE (table), hbox, x+1, x+2, y, y+1,
		    GTK_EXPAND | GTK_FILL, GTK_FILL, 0, 0);

  gtk_widget_show (label);
  gtk_widget_show (entry);
  gtk_widget_show (scale);
  gtk_widget_show (hbox);
}  


/* when destroyed, userdata is destroyed too */
static void
entscale_int_destroy_callback (GtkWidget *widget,
			       gpointer data)
{
  EntscaleIntData *userdata;

  userdata = data;
  g_free ( userdata );
}

static void
entscale_int_scale_update (GtkAdjustment *adjustment,
			   gpointer      data)
{
  EntscaleIntData *userdata;
  GtkEntry	*entry;
  gchar		buffer[256];
  gint		*intvar = data;
  gint		new_val;

  userdata = gtk_object_get_user_data (GTK_OBJECT (adjustment));

  new_val = (gint) adjustment->value;

  *intvar = new_val;

  entry = GTK_ENTRY( userdata->entry );
  sprintf (buffer, "%d", (int) new_val );
  
  /* avoid infinite loop (scale, entry, scale, entry ...) */
  gtk_signal_handler_block_by_data ( GTK_OBJECT(entry), data );
  gtk_entry_set_text ( entry, buffer);
  gtk_signal_handler_unblock_by_data ( GTK_OBJECT(entry), data );

  if (userdata->callback)
    (*userdata->callback) (*intvar, userdata->call_data);
}

static void
entscale_int_entry_update (GtkWidget *widget,
			   gpointer   data)
{
  EntscaleIntData *userdata;
  GtkAdjustment	*adjustment;
  int		new_val, constraint_val;
  int		*intvar = data;

  userdata = gtk_object_get_user_data (GTK_OBJECT (widget));
  adjustment = GTK_ADJUSTMENT( userdata->adjustment );

  new_val = atoi (gtk_entry_get_text (GTK_ENTRY (widget)));
  constraint_val = new_val;
  if ( constraint_val < adjustment->lower )
    constraint_val = adjustment->lower;
  if ( constraint_val > adjustment->upper )
    constraint_val = adjustment->upper;

  if ( userdata->constraint )
    *intvar = constraint_val;
  else
    *intvar = new_val;

  adjustment->value = constraint_val;
  gtk_signal_handler_block_by_data ( GTK_OBJECT(adjustment), data );
  gtk_signal_emit_by_name ( GTK_OBJECT(adjustment), "value_changed");
  gtk_signal_handler_unblock_by_data ( GTK_OBJECT(adjustment), data );
  
  if (userdata->callback)
    (*userdata->callback) (*intvar, userdata->call_data);
}
