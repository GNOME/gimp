/* The GIMP -- an image manipulation program * Copyright (C) 1995 Spencer
 * Kimball and Peter Mattis * * This program is free software; you can
 * redistribute it and/or modify * it under the terms of the GNU General
 * Public License as published by * the Free Software Foundation; either
 * version 2 of the License, or * (at your option) any later version. * *
 * This program is distributed in the hope that it will be useful, * but
 * WITHOUT ANY WARRANTY; without even the implied warranty of *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the * GNU
 * General Public License for more details. * * You should have received a
 * copy of the GNU General Public License * along with this program; if not,
 * write to the Free Software * Foundation, Inc., 675 Mass Ave, Cambridge, MA
 * 02139, USA. */
/* Holes 0.5 */
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include "libgimp/gimp.h"
#include "gtk/gtk.h"

/* Declare local functions. */
static void query(void);
static void run(char *name,
	       	int nparams,
	       	GParam * param,
	       	int *nreturn_vals,
	       	GParam ** return_vals);

static void doit(GDrawable * drawable);

/* UI funcs */
#define ENTRY_WIDTH 50
#define SCALE_WIDTH 150

static void UI_int_entryscale_new ( GtkTable *table, gint x, gint y,
			 guchar *caption, gint *intvar,
			 gint min, gint max, gint constraint);
static void UI_paired_entry_destroy_callback (GtkWidget *widget,
				    gpointer data);
static void UI_paired_int_scale_update (GtkAdjustment *adjustment,
			      gpointer      data);
static void UI_paired_int_entry_update (GtkWidget *widget,
			      gpointer   data);
static void UI_float_entryscale_new ( GtkTable *table, gint x, gint y,
			 guchar *caption, gfloat *var,
			 gfloat min, gfloat max, gint constraint);
static void UI_paired_float_scale_update (GtkAdjustment *adjustment,
			      gpointer      data);
static void UI_paired_float_entry_update (GtkWidget *widget,
			      gpointer   data);
static void UI_close_callback (GtkWidget *widget,
			 gpointer   data);
static void UI_ok_callback (GtkWidget *widget,
		      gpointer   data);
static gint dialog();





GPlugInInfo PLUG_IN_INFO =
{
	NULL,													/* init_proc */
	NULL,													/* quit_proc */
	query,												/* query_proc */
	run,													/* run_proc */
};


gint bytes;
gint sx1, sy1, sx2, sy2;
guchar *shape_data= NULL, *tmp= NULL;
gint curve[256];

typedef struct {
  gint run;
} HolesInterface;

typedef struct {
  GtkObject     *adjustment;
  GtkWidget     *entry;
  gint          constraint;
} EntryScaleData;

static HolesInterface pint =
{
  FALSE     /* run */
};

typedef struct {
  float density;
  gint shape, size, flag;
} holes_parameters;

/* possible shapes */
#define SQUARE_SHAPE 0
#define CIRCLE_SHAPE 1
#define DIAMOND_SHAPE 2
static holes_parameters params = { -3.1, CIRCLE_SHAPE, 8, TRUE };


MAIN()

static void query()
{
	static GParamDef args[] =
	{
		{PARAM_INT32, "run_mode", "Interactive, non-interactive"},
		{PARAM_IMAGE, "image", "Input image (unused)"},
		{PARAM_DRAWABLE, "drawable", "Input drawable"},
		{PARAM_FLOAT, "density", "Density (actually the log of the density)"},
		{PARAM_INT32, "shape", "shape (0= square, 1= round, 2= diamond)"},
		{PARAM_INT32, "size", "size (in pixels)"},
		{PARAM_INT32, "flag", "Clear it if you want to make holes in your image, or set it if you want to keep the painted (I mean opaque) regions."},
	};
	static GParamDef *return_vals = NULL;
	static int nargs = sizeof(args) / sizeof(args[0]);
	static int nreturn_vals = 0;

	gimp_install_procedure("plug_in_holes",
				"make bucks on a image.",
				"makes holes in the alpha channel of an image, with a density depending on the actual transparency of this image. (so the image must have an alpha channel...)",
				"Xavier Bouchoux",
				"Xavier Bouchoux",
				"1997",
				"<Image>/Filters/Holes",
				"RGBA, GRAYA, INDEXEDA",
				PROC_PLUG_IN,
				nargs, nreturn_vals,
				args, return_vals
	);
}

static void run(char *name, int n_params, GParam * param, int *nreturn_vals,
								GParam ** return_vals)
{
	static GParam values[1];
	GDrawable *drawable;
	GRunModeType run_mode;
	GStatusType status = STATUS_SUCCESS;

	*nreturn_vals = 1;
	*return_vals = values;

	run_mode = param[0].data.d_int32;

	if (run_mode == RUN_NONINTERACTIVE) {
		if (n_params != 8) {
			status = STATUS_CALLING_ERROR;
		} else {
			params.density = param[3].data.d_float;
			params.shape = param[4].data.d_int32;
			params.size = param[5].data.d_int32;
			params.flag = param[6].data.d_int32;
		}
	} else {
		/*  Possibly retrieve data  */
		gimp_get_data("plug_in_holes", &params);

		if ((run_mode == RUN_INTERACTIVE) ) {
			/* Oh boy. We get to do a dialog box, because we can't really expect the
			 * user to set us up with the right values using gdb.
			 */
		  if (!dialog()) {
		    /* The dialog was closed, or something similarly evil happened. */
		    status = STATUS_EXECUTION_ERROR;
		  }
		}
	}

	if (status == STATUS_SUCCESS) {
		/*  Get the specified drawable  */
		drawable = gimp_drawable_get(param[2].data.d_drawable);

		/*  Make sure that the drawable is gray or RGB color  */
		if (gimp_drawable_has_alpha (drawable->id) && (gimp_drawable_color(drawable->id) || gimp_drawable_gray(drawable->id) ||gimp_drawable_indexed (drawable->id))) {
			gimp_progress_init("holes");
			gimp_tile_cache_ntiles(4);

			srand(time(NULL));
			doit(drawable);

			if (run_mode != RUN_NONINTERACTIVE)
			        gimp_displays_flush();

			if (run_mode == RUN_INTERACTIVE)
				gimp_set_data("plug_in_holes", &params, sizeof(params));
		} else {
			status = STATUS_EXECUTION_ERROR;
		}
	}

	values[0].type = PARAM_STATUS;
	values[0].data.d_status = status;

	gimp_drawable_detach(drawable);
}


static void make_curve (gdouble  sigma)
{
  gdouble sigma2;
  gint i;

  sigma2 = 2 * sigma * sigma;

  if (params.flag)
    for (i = 0; i < 256; i++)
      {
	curve[i] = (gint) (exp (- (i/22.0 * i/22.0) / sigma2) * RAND_MAX);
      }
  else
    for (i = 0; i < 256; i++)
      {
	curve[255-i] = (gint) (exp (- (i/22.0 * i/22.0) / sigma2) * RAND_MAX);
      }

}


/* Prepare the shape data: makes a buffer with the shape calculated with
 * the goood size.
 */
static int prepare_shape()
{
  int i,j, center;

  shape_data = malloc(params.size*params.size);
  if (shape_data == NULL)
    return 0;

  switch (params.shape) {
  case SQUARE_SHAPE:
    for (i=0;i<params.size; i++)
      for (j=0; j<params.size; j++)
	shape_data[i*params.size+j]=255;
    break;
  case CIRCLE_SHAPE:
     center= params.size/2;
     for (i=0;i<params.size; i++)
      for (j=0; j<params.size; j++)
	if (((i-center)*(i-center)+(j-center)*(j-center))<center*center)
	  shape_data[i*params.size+j]=255;
	else
	  shape_data[i*params.size+j]=0;
    break;
  case DIAMOND_SHAPE:
     center= params.size/2;
     for (i=0; i<params.size; i++)
       for (j=0; j<params.size; j++)
         if ((abs(i-center)+abs(j-center))<center)
   	   shape_data[i*params.size+j]=255;
   	 else
   	   shape_data[i*params.size+j]=0;
     break;
  default:
    return 0;
  }
  return 1;
}

/*
 * Puts the shape (=the buck) in the alpha layer of the destination image,
 * at position x,y
 */
static void make_hole(GPixelRgn * region, int x, int y)
{
	int j, k, startx=0, starty=0;
	int rx1, rx2, ry1, ry2;
	int new;

	rx1 = x - params.size / 2;
	rx2 = rx1 + params.size;
	ry1 = y - params.size  / 2;
	ry2 = ry1 + params.size;

	/* clipping */
	if (rx1 < sx1) {
	  startx=sx1-rx1;
	  rx1= sx1;
	}
	if (ry1 < sy1) {
	  starty=sy1-ry1;
	  ry1= sy1;
	}
	rx2 = rx2 > sx2 ? sx2 : rx2;
	ry2 = ry2 > sy2 ? sy2 : ry2;

 	gimp_pixel_rgn_get_rect(region, tmp, rx1, ry1, rx2 - rx1, ry2 - ry1);
	for (k = 0; k < (ry2 - ry1); k++) {
		for (j = 0; j < (rx2 - rx1); j++) {
		  new= tmp[((rx2-rx1)*k+j) * bytes + (bytes-1)]-
		               shape_data[params.size*(k+starty)+j+startx];
		  tmp[((rx2-rx1)*k+j) * bytes + (bytes-1)]= (new<=0? 0 : new);
		}
	}
	gimp_pixel_rgn_set_rect(region, tmp, rx1, ry1, rx2 - rx1, ry2 - ry1);
}

static void make_hole_inv(GPixelRgn * region, int x, int y)
{
	int j, k, startx=0, starty=0;
	int rx1, rx2, ry1, ry2;
	int new;

	rx1 = x - params.size / 2;
	rx2 = rx1 + params.size;
	ry1 = y - params.size  / 2;
	ry2 = ry1 + params.size;

	/* clipping */
	if (rx1 < sx1) {
	  startx=sx1-rx1;
	  rx1= sx1;
	}
	if (ry1 < sy1) {
	  starty=sy1-ry1;
	  ry1= sy1;
	}
	rx2 = rx2 > sx2 ? sx2 : rx2;
	ry2 = ry2 > sy2 ? sy2 : ry2;

 	gimp_pixel_rgn_get_rect(region, tmp, rx1, ry1, rx2 - rx1, ry2 - ry1);
	for (k = 0; k < (ry2 - ry1); k++) {
		for (j = 0; j < (rx2 - rx1); j++) {
		  new= tmp[((rx2-rx1)*k+j) * bytes + (bytes-1)]+
		               shape_data[params.size*(k+starty)+j+startx];
		  tmp[((rx2-rx1)*k+j) * bytes + (bytes-1)]= (new>255? 255 : new);
		}
	}
	gimp_pixel_rgn_set_rect(region, tmp, rx1, ry1, rx2 - rx1, ry2 - ry1);
}

static void doit(GDrawable * drawable)
{
	GPixelRgn srcPR, destPR;
	gint width, height;
	int x, y;
	int col, row, b;
	guchar *src_row, *dest_row;
	guchar *src, *dest;
	gint progress, max_progress;
	gpointer pr;
	guchar val_alpha;

	/* Get the input area. This is the bounding box of the selection in
	 *  the image (or the entire image if there is no selection). Only
	 *  operating on the input area is simply an optimization. It doesn't
	 *  need to be done for correct operation. (It simply makes it go
	 *  faster, since fewer pixels need to be operated on).
	 */
	gimp_drawable_mask_bounds(drawable->id, &sx1, &sy1, &sx2, &sy2);

	/* Get the size of the input image. (This will/must be the same
	 *  as the size of the output image.
	 */
	width = drawable->width;
	height = drawable->height;
	bytes = drawable->bpp;

	/* initialize buffers and data */
	tmp = (guchar *) malloc(params.size * params.size * bytes);
	if (tmp == NULL) {
		return;
	}
	if (!prepare_shape())
	  return;
	make_curve(exp(-params.density));

	progress = 0;
	max_progress = (sx2 - sx1) * (sy2 - sy1) * 2;



	/*  initialize the pixel regions  */
	gimp_pixel_rgn_init(&srcPR, drawable, sx1, sy1, sx2-sx1, sy2-sy1, FALSE, FALSE);
	gimp_pixel_rgn_init(&destPR, drawable, sx1, sy1, sx2-sx1, sy2-sy1, TRUE, TRUE);

	/* First off, copy the old one to the new one. */
	if (params.flag)
	  val_alpha=0;
	else
	  val_alpha=255;

	for (pr = gimp_pixel_rgns_register (2, &srcPR, &destPR);
	     pr != NULL; pr = gimp_pixel_rgns_process (pr)) {
	  src_row = srcPR.data;
	  dest_row = destPR.data;
	  for ( row = 0; row < srcPR.h; row++) {
	    src = src_row;
	    dest = dest_row;
	    for ( col = 0; col < srcPR.w; col++) {
	      for (b=0; b< bytes-1; b++)
		*dest++ = *src++;
	      src++;       /* set alpha to opaque */
	      *dest++ = val_alpha;
	    }
	    src_row += srcPR.rowstride;
	    dest_row += destPR.rowstride;
	  }
	  progress += srcPR.w * srcPR.h;
	  gimp_progress_update ((double) progress / (double) max_progress);
	}

	/* Do the effect */
	gimp_pixel_rgn_init(&srcPR, drawable, sx1, sy1, sx2-sx1, sy2-sy1, FALSE, FALSE);
	if (params.flag) {
	  for (pr = gimp_pixel_rgns_register (1, &srcPR);
	       pr != NULL; pr = gimp_pixel_rgns_process (pr)) {
	    src_row = srcPR.data;
	    for ( row = 0, y = srcPR.y; row < srcPR.h; row++, y++) {
	      src = src_row;
	      for ( col = 0, x = srcPR.x; col < srcPR.w; col++, x++) {
		if (curve[src[bytes-1]]<rand()) {
		  make_hole_inv(&destPR, x, y);
		}
		src+= bytes;
	      }
	      src_row += srcPR.rowstride;
	    }
	    progress += srcPR.w * srcPR.h;
	    gimp_progress_update ((double) progress / (double) max_progress);
	  }
	} else {
	  for (pr = gimp_pixel_rgns_register (1, &srcPR);
	       pr != NULL; pr = gimp_pixel_rgns_process (pr)) {
	    src_row = srcPR.data;
	    for ( row = 0, y = srcPR.y; row < srcPR.h; row++, y++) {
	      src = src_row;
	      for ( col = 0, x = srcPR.x; col < srcPR.w; col++, x++) {
		if (curve[src[bytes-1]]<rand()) {
		  make_hole(&destPR, x, y);
		}
		src+= bytes;
	      }
	      src_row += srcPR.rowstride;
	    }
	    progress += srcPR.w * srcPR.h;
	    gimp_progress_update ((double) progress / (double) max_progress);
	  }
	}
	/* free the mem */
	free(tmp);
	free(shape_data);


	/*  update the region  */
	gimp_drawable_flush(drawable);
	gimp_drawable_merge_shadow(drawable->id, TRUE);
	gimp_drawable_update(drawable->id, sx1, sy1, sx2 - sx1, sy2 - sy1);
}

/***************************************************
 * GUI stuff
 */
/*===================================================================

         Entry - Scale Pair

====================================================================*/


/***********************************************************************/
/*                                                                     */
/*    Create new entry-scale pair with label. (int)                    */
/*    1 row and 2 cols of table are needed.                            */
/*                                                                     */
/*    `x' and `y' means starting row and col in `table'.               */
/*                                                                     */
/*    `caption' is label string.                                       */
/*                                                                     */
/*    `min', `max' are boundary of scale.                              */
/*                                                                     */
/*    `constraint' means whether value of *intvar should be constraint */
/*    by scale adjustment, e.g. between `min' and `max'.               */
/*                                                                     */
/***********************************************************************/

static void
UI_int_entryscale_new ( GtkTable *table, gint x, gint y,
			 guchar *caption, gint *intvar,
			 gint min, gint max, gint constraint)
{
  GtkWidget *hbox;
  GtkWidget *label;
  GtkWidget *entry;
  GtkWidget *scale;
  GtkObject *adjustment;
  EntryScaleData *userdata;
  guchar    buffer[256];

  label = gtk_label_new (caption);
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);

  adjustment = gtk_adjustment_new ( *intvar, min, max, 1.0, 1.0, 0.0);
  scale = gtk_hscale_new ( GTK_ADJUSTMENT(adjustment) );
  gtk_widget_set_usize (scale, SCALE_WIDTH, 0);
  gtk_scale_set_draw_value (GTK_SCALE (scale), FALSE);

  entry = gtk_entry_new ();
  gtk_widget_set_usize (entry, ENTRY_WIDTH, 0);
  sprintf( buffer, "%d", *intvar );
  gtk_entry_set_text( GTK_ENTRY (entry), buffer );


  userdata = g_new ( EntryScaleData, 1 );
  userdata->entry = entry;
  userdata->adjustment = adjustment;
  userdata->constraint = constraint;
  gtk_object_set_user_data (GTK_OBJECT(entry), userdata);
  gtk_object_set_user_data (GTK_OBJECT(adjustment), userdata);

  gtk_signal_connect (GTK_OBJECT (entry), "changed",
		      (GtkSignalFunc) UI_paired_int_entry_update,
		      intvar);
  gtk_signal_connect ( adjustment, "value_changed",
		      (GtkSignalFunc) UI_paired_int_scale_update,
		      intvar);
  gtk_signal_connect ( GTK_OBJECT( entry ), "destroy",
		      (GtkSignalFunc) UI_paired_entry_destroy_callback,
		      userdata );


  hbox = gtk_hbox_new ( FALSE, 5 );
  gtk_box_pack_start (GTK_BOX (hbox), scale, TRUE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (hbox), entry, FALSE, TRUE, 0);

  gtk_table_attach (GTK_TABLE (table), label, x, x+1, y, y+1,
		    GTK_FILL, GTK_FILL, 0, 0);
  gtk_table_attach (GTK_TABLE (table), hbox, x+1, x+2, y, y+1,
		    GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL, 0, 0);

  gtk_widget_show (label);
  gtk_widget_show (entry);
  gtk_widget_show (scale);
  gtk_widget_show (hbox);
}


/*
   when destroyed, userdata is destroyed too
*/
static void
UI_paired_entry_destroy_callback (GtkWidget *widget,
				    gpointer data)
{
  EntryScaleData *userdata;
  userdata = data;
  g_free ( userdata );
}

/* scale callback (int) */
/* ==================== */

static void
UI_paired_int_scale_update (GtkAdjustment *adjustment,
			      gpointer      data)
{
  EntryScaleData *userdata;
  GtkEntry *entry;
  gchar buffer[256];
  gint *val, new_val;

  val = data;
  new_val = (gint) adjustment->value;

  *val = new_val;

  userdata = gtk_object_get_user_data (GTK_OBJECT (adjustment));
  entry = GTK_ENTRY( userdata->entry );
  sprintf (buffer, "%d", (int) new_val );
  /* avoid infinite loop (scale, entry, scale, entry ...) */
  gtk_signal_handler_block_by_data ( GTK_OBJECT(entry), data );
  gtk_entry_set_text ( entry, buffer);
  gtk_signal_handler_unblock_by_data ( GTK_OBJECT(entry), data );
}

/*
   entry callback (int)
*/

static void
UI_paired_int_entry_update (GtkWidget *widget,
			      gpointer   data)
{
  EntryScaleData *userdata;
  GtkAdjustment *adjustment;
  int new_val, constraint_val;
  int *val;

  val = data;
  new_val = atoi (gtk_entry_get_text (GTK_ENTRY (widget)));
  *val = new_val;

  userdata = gtk_object_get_user_data (GTK_OBJECT (widget));
  adjustment = GTK_ADJUSTMENT( userdata->adjustment );

  constraint_val = new_val;
  if ( constraint_val < adjustment->lower )
    constraint_val = adjustment->lower;
  if ( constraint_val > adjustment->upper )
    constraint_val = adjustment->upper;

  if ( userdata->constraint )
    *val = constraint_val;
  else
    *val = new_val;

  adjustment->value = constraint_val;
  gtk_signal_handler_block_by_data ( GTK_OBJECT(adjustment), data );
  gtk_signal_emit_by_name ( GTK_OBJECT(adjustment), "value_changed");
  gtk_signal_handler_unblock_by_data ( GTK_OBJECT(adjustment), data );
}



/* The same thing, but with floats... */

static void
UI_float_entryscale_new ( GtkTable *table, gint x, gint y,
			 guchar *caption, gfloat *var,
			 gfloat min, gfloat max, gint constraint)
{
  GtkWidget *hbox;
  GtkWidget *label;
  GtkWidget *entry;
  GtkWidget *scale;
  GtkObject *adjustment;
  EntryScaleData *userdata;
  guchar    buffer[256];

  label = gtk_label_new (caption);
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);

  adjustment = gtk_adjustment_new ( *var, min, max, 1.0, 1.0, 0.0);
  scale = gtk_hscale_new ( GTK_ADJUSTMENT(adjustment) );
  gtk_widget_set_usize (scale, SCALE_WIDTH, 0);
  gtk_scale_set_draw_value (GTK_SCALE (scale), FALSE);

  entry = gtk_entry_new ();
  gtk_widget_set_usize (entry, ENTRY_WIDTH, 0);
  sprintf( buffer, "%1.2f", *var );
  gtk_entry_set_text( GTK_ENTRY (entry), buffer );


  userdata = g_new ( EntryScaleData, 1 );
  userdata->entry = entry;
  userdata->adjustment = adjustment;
  userdata->constraint = constraint;
  gtk_object_set_user_data (GTK_OBJECT(entry), userdata);
  gtk_object_set_user_data (GTK_OBJECT(adjustment), userdata);

  gtk_signal_connect (GTK_OBJECT (entry), "changed",
		      (GtkSignalFunc) UI_paired_float_entry_update,
		      var);
  gtk_signal_connect ( adjustment, "value_changed",
		      (GtkSignalFunc) UI_paired_float_scale_update,
		      var);
  gtk_signal_connect ( GTK_OBJECT( entry ), "destroy",
		      (GtkSignalFunc) UI_paired_entry_destroy_callback,
		      userdata );


  hbox = gtk_hbox_new ( FALSE, 5 );
  gtk_box_pack_start (GTK_BOX (hbox), scale, TRUE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (hbox), entry, FALSE, TRUE, 0);

  gtk_table_attach (GTK_TABLE (table), label, x, x+1, y, y+1,
		    GTK_FILL, GTK_FILL, 0, 0);
  gtk_table_attach (GTK_TABLE (table), hbox, x+1, x+2, y, y+1,
		    GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL, 0, 0);

  gtk_widget_show (label);
  gtk_widget_show (entry);
  gtk_widget_show (scale);
  gtk_widget_show (hbox);
}


/* scale callback (float) */
/* ==================== */

static void
UI_paired_float_scale_update (GtkAdjustment *adjustment,
			      gpointer      data)
{
  EntryScaleData *userdata;
  GtkEntry *entry;
  gchar buffer[256];
  gfloat *val, new_val;

  val = data;
  new_val = (gfloat) adjustment->value;

  *val = new_val;

  userdata = gtk_object_get_user_data (GTK_OBJECT (adjustment));
  entry = GTK_ENTRY( userdata->entry );
  sprintf (buffer, "%1.2f", (gfloat) new_val );
  /* avoid infinite loop (scale, entry, scale, entry ...) */
  gtk_signal_handler_block_by_data ( GTK_OBJECT(entry), data );
  gtk_entry_set_text ( entry, buffer);
  gtk_signal_handler_unblock_by_data ( GTK_OBJECT(entry), data );
}

/*
   entry callback (float)
*/

static void
UI_paired_float_entry_update (GtkWidget *widget,
			      gpointer   data)
{
  EntryScaleData *userdata;
  GtkAdjustment *adjustment;
  gfloat new_val, constraint_val;
  gfloat *val;

  val = data;
  new_val = atof (gtk_entry_get_text (GTK_ENTRY (widget)));
  *val = new_val;

  userdata = gtk_object_get_user_data (GTK_OBJECT (widget));
  adjustment = GTK_ADJUSTMENT( userdata->adjustment );

  constraint_val = new_val;
  if ( constraint_val < adjustment->lower )
    constraint_val = adjustment->lower;
  if ( constraint_val > adjustment->upper )
    constraint_val = adjustment->upper;

  if ( userdata->constraint )
    *val = constraint_val;
  else
    *val = new_val;

  adjustment->value = constraint_val;
  gtk_signal_handler_block_by_data ( GTK_OBJECT(adjustment), data );
  gtk_signal_emit_by_name ( GTK_OBJECT(adjustment), "value_changed");
  gtk_signal_handler_unblock_by_data ( GTK_OBJECT(adjustment), data );
}


static void
UI_toggle_update (GtkWidget *widget,
		       gpointer   data)
{
  int *toggle_val;

  toggle_val = (int *) data;

  if (GTK_TOGGLE_BUTTON (widget)->active)
    *toggle_val = TRUE;
  else
    *toggle_val = FALSE;
}


void shape_radio_callback(GtkWidget *widget, GtkRadioButton *button)
{
  gint id;
  /* Get radio button ID */
  id=(gint)gtk_object_get_data(GTK_OBJECT(button),"Radio_ID");

  if (GTK_TOGGLE_BUTTON(button)->active==TRUE) {
      params.shape= id;
    }
}



/***  Dialog interface ***/

static void
UI_close_callback (GtkWidget *widget,
			 gpointer   data)
{
  gtk_main_quit ();
}

static void
UI_ok_callback (GtkWidget *widget,
		      gpointer   data)
{
  pint.run = TRUE;
  gtk_widget_destroy (GTK_WIDGET (data));
}

static gint
dialog ()
{
  GtkWidget *dlg;
  GtkWidget *frame;
  GtkWidget *table;
  GtkWidget *button;
  GtkWidget *toggle;
  GSList *group = NULL;

  gchar **argv;
  gint  argc;

  argc = 1;
  argv = g_new (gchar *, 1);
  argv[0] = g_strdup ("holes");

  gtk_init (&argc, &argv);
  gtk_rc_parse (gimp_gtkrc ());

  dlg = gtk_dialog_new ();
  gtk_window_set_title (GTK_WINDOW (dlg), "Holes");
  gtk_window_position (GTK_WINDOW (dlg), GTK_WIN_POS_MOUSE);
  gtk_signal_connect (GTK_OBJECT (dlg), "destroy",
		      (GtkSignalFunc) UI_close_callback,
		      NULL);

  /*  Action area  */
  button = gtk_button_new_with_label ("OK");
  GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT);
  gtk_signal_connect (GTK_OBJECT (button), "clicked",
                      (GtkSignalFunc) UI_ok_callback,
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
  table = gtk_table_new (6, 2, FALSE);
  gtk_container_border_width (GTK_CONTAINER (table), 10);
  gtk_container_add (GTK_CONTAINER (frame), table);
  gtk_table_set_row_spacings (GTK_TABLE (table), 3);
  gtk_table_set_col_spacings (GTK_TABLE (table), 10);

  UI_int_entryscale_new( GTK_TABLE (table), 0, 1,
			  "Size:", &params.size,
			  1, 100, TRUE );
  UI_float_entryscale_new( GTK_TABLE (table), 0, 2,
			  "Density:", &params.density,
			  -5.0, 5.0, TRUE );
  toggle = gtk_check_button_new_with_label ("Keep opaque");
  gtk_table_attach (GTK_TABLE (table), toggle, 0, 1, 3, 4,
                    GTK_FILL, GTK_FILL, 0, 0);
  gtk_signal_connect (GTK_OBJECT (toggle), "toggled",
                      (GtkSignalFunc) UI_toggle_update,
                      &params.flag);
  gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (toggle), params.flag);
  gtk_widget_show(toggle);

  toggle = gtk_radio_button_new_with_label (group,"square shaped holes");
  group = gtk_radio_button_group (GTK_RADIO_BUTTON (toggle));
  gtk_table_attach (GTK_TABLE (table), toggle, 1, 2, 3, 4,
                    GTK_FILL, GTK_FILL, 0, 0);
  gtk_signal_connect_object (GTK_OBJECT (toggle), "toggled",
		      (GtkSignalFunc) shape_radio_callback,
		      (gpointer)toggle);
  gtk_object_set_data(GTK_OBJECT(toggle),"Radio_ID",(gpointer)SQUARE_SHAPE);
  gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (toggle), (params.shape == SQUARE_SHAPE));
  gtk_widget_show (toggle);

  toggle = gtk_radio_button_new_with_label (group,"round shaped holes");
  group = gtk_radio_button_group (GTK_RADIO_BUTTON (toggle));
  gtk_table_attach (GTK_TABLE (table), toggle, 1, 2, 4, 5,
                    GTK_FILL, GTK_FILL, 0, 0);
  gtk_signal_connect_object (GTK_OBJECT (toggle), "toggled",
		      (GtkSignalFunc) shape_radio_callback,
		      (gpointer)toggle);
  gtk_object_set_data(GTK_OBJECT(toggle),"Radio_ID",(gpointer)CIRCLE_SHAPE);
  gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (toggle), (params.shape == CIRCLE_SHAPE));
  gtk_widget_show (toggle);

  toggle = gtk_radio_button_new_with_label (group,"diamond shaped holes");
  group = gtk_radio_button_group (GTK_RADIO_BUTTON (toggle));
  gtk_table_attach (GTK_TABLE (table), toggle, 1, 2, 5, 6,
                    GTK_FILL, GTK_FILL, 0, 0);
  gtk_signal_connect_object (GTK_OBJECT (toggle), "toggled",
		      (GtkSignalFunc) shape_radio_callback,
		      (gpointer)toggle);
  gtk_object_set_data(GTK_OBJECT(toggle),"Radio_ID",(gpointer)DIAMOND_SHAPE);
  gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (toggle), (params.shape == DIAMOND_SHAPE));
  gtk_widget_show (toggle);

  gtk_widget_show (frame);
  gtk_widget_show (table);
  gtk_widget_show (dlg);

  gtk_main ();
  gdk_flush ();

  return pint.run;
}
