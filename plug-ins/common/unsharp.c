/* $Id$
 *
 * unsharp.c 0.8 -- This is a plug-in for the GIMP 1.0
 *  http://www.steppe.com/~winston/gimp/unsharp.html
 *
 * Copyright (C) 1999 Winston Chang
 *                    <wchang3@students.wisc.edu>
 *                    <winston@steppe.com>
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
 */


#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <stdio.h>

#include "libgimp/gimp.h"
#include "gtk/gtk.h"

#include "dialog_f.h"
#include "dialog_i.h"

#define PLUG_IN_VERSION "0.8"


/* to show both pretty unoptimized code and ugly optimized code blocks
   There's really no reason to define this, unless you want to see how
   much pointer aritmetic can speed things up.  I find that it is about
   45% faster with the optimized code. */
//#define READABLE_CODE

/* uncomment this line to get a rough feel of how long the
   plug-in takes to run */
//#define TIMER
#ifdef TIMER
	#include <sys/time.h>
	#include <unistd.h>
	static void timerstart();
	static void timerstop();
	static struct timeval time_start,time_stop;
#endif

typedef struct {
	gdouble radius;
	gdouble amount;
	gint threshold;
} UnsharpMaskParams;

typedef struct {
	gint run;
} UnsharpMaskInterface;

/* local function prototypes */
static inline void blur_line(gdouble* ctable, gdouble* cmatrix, gint cmatrix_length,
                             guchar* cur_col, guchar* dest_col,
                             gint y, glong bytes);
static int gen_convolve_matrix(double std_dev, double** cmatrix);
static gdouble* gen_lookup_table(gdouble* cmatrix, gint cmatrix_length);
static inline gint round2int(gdouble i);
static void unsharp_region (GPixelRgn srcPTR, GPixelRgn dstPTR,
                   gint width, gint height, gint bytes,
									 gdouble radius, gdouble amount,
                   gint x1, gint x2, gint y1, gint y2);



static void query(void);
static void run (gchar   *name,
                 gint     nparams,
                 GParam  *param,
                 gint    *nreturn_vals,
                 GParam **return_vals);
static void unsharp_mask(GDrawable *drawable, gint radius, gdouble amount);


static void unsharp_cancel_callback (GtkWidget *widget, gpointer data);
static void unsharp_ok_callback (GtkWidget *widget, gpointer data);
static gint unsharp_mask_dialog();
/* preview shit -- not finished yet */
#undef PREVIEW
#ifdef PREVIEW
static void preview_scroll_callback(void);
static void preview_init(void);
static void preview_exit(void);
static void preview_update(void);
static void dialog_create_ivalue(char     *title, /* I - Label for control */
                     GtkTable *table, /* I - Table container to use */
                     int      row,    /* I - Row # for container */
                     gint     *value, /* I - Value holder */
                     int      left,   /* I - Minimum value for slider */
                     int      right); /* I - Maximum value for slider */


static GtkWidget* preview;
static int preview_width;    /* Width of preview widget */
static int preview_height;   /* Height of preview widget */
static int preview_x1;       /* Upper-left X of preview */
static int preview_y1;       /* Upper-left Y of preview */
static int preview_x2;       /* Lower-right X of preview */
static int preview_y2;       /* Lower-right Y of preview */

static int sel_width;    /* Selection width */
static int sel_height;   /* Selection height */
		
static GtkObject *hscroll_data;    /* Horizontal scrollbar data */
static GtkObject *vscroll_data;    /* Vertical scrollbar data */
#endif

static gint run_filter = FALSE;
		


/* create a few globals, set default values */
static UnsharpMaskParams unsharp_params =
{
	5.0, /* default radius = 5 */
	0.5, /* default amount = .5 */
	0    /* default threshold = 0 */
};

//static UnsharpMaskInterface umint = { FALSE };

/* Setting PLUG_IN_INFO */
GPlugInInfo PLUG_IN_INFO = {
	NULL,    /* init_proc */
	NULL,    /* quit_proc */
	query,   /* query_proc */
	run,     /* run_proc */
};


MAIN()


static void query () {

	static GParamDef args[] = {
		{ PARAM_INT32, "run_mode", "Interactive, non-interactive" },
		{ PARAM_IMAGE, "image", "(unused)" },
		{ PARAM_DRAWABLE, "drawable", "Drawable to draw on" },
		{ PARAM_FLOAT, "radius", "Radius of gaussian blur (in pixels > 1.0)" },
		{ PARAM_FLOAT, "amount", "Strength of effect." }
	};
	static gint nargs = sizeof(args) / sizeof(args[0]);
	/* for return vals */
	static GParamDef *return_vals = NULL;
	static int nreturn_vals = 0;
	
	/* Install a procedure in the procedure database. */
	gimp_install_procedure ("plug_in_unsharp_mask",
	                        "An unsharp mask filter",
	                        "Long description / help",
	                        "Winston Chang <wchang3@students.wisc.edu>",
	                        "Winston Chang",
	                        "1999",
	                        "<Image>/Filters/Enhance/Unsharp Mask",
	                        "GRAY*, RGB*",
	                        PROC_PLUG_IN,
	                        nargs, nreturn_vals,
	                        args, return_vals);
}


/* this is the actual function */
static void run(char *name, int nparams, GParam *param, int *nreturn_vals,
                GParam **return_vals) {
	
	static GParam values[1];
	GDrawable *drawable;
	GRunModeType run_mode;
	GStatusType status = STATUS_SUCCESS;
	
#ifdef TIMER
	timerstart();
#endif
	
	run_mode = param[0].data.d_int32;
	
	values[0].type = PARAM_STATUS;
	values[0].data.d_status = status;
	*return_vals = values;
	*nreturn_vals = 1;
	
	switch (run_mode) {
		case RUN_INTERACTIVE:
			gimp_get_data ("plug_in_unsharp_mask", &unsharp_params);
			if (! unsharp_mask_dialog ()) return;
			break;

		case RUN_NONINTERACTIVE:
			if (nparams != 4) status = STATUS_CALLING_ERROR;
			/* get the parameters */
			else if (status == STATUS_SUCCESS) {
				unsharp_params.radius = param[3].data.d_float;
				unsharp_params.amount = param[4].data.d_float; 
			}
			/* make sure there are legal values */
			if (status == STATUS_SUCCESS && 
			         ( (unsharp_params.radius < 0) || (unsharp_params.amount<0) ) )
				status = STATUS_CALLING_ERROR;
			break;

		case RUN_WITH_LAST_VALS:
			gimp_get_data ("plug_in_unsharp_mask", &unsharp_params);
			break;

		default:
			break;
	}

	if (status == STATUS_SUCCESS) {
		drawable = gimp_drawable_get (param[2].data.d_drawable);
		gimp_tile_cache_ntiles(2 * (drawable->width / gimp_tile_width() + 1));

		/* here we go */
		unsharp_mask(drawable, unsharp_params.radius, unsharp_params.amount);
	
		//	values[0].data.d_status = status;
		gimp_displays_flush ();
		
		/* set data for next use of filter */
		gimp_set_data ("plug_in_unsharp_mask", &unsharp_params,
		                sizeof (UnsharpMaskParams));
		
		/*fprintf(stderr, "%f %f\n", unsharp_params.radius, unsharp_params.amount);*/
		
		gimp_drawable_detach(drawable);
		values[0].data.d_status = status;

	}
#ifdef TIMER
	timerstop();
#endif
}



/* -------------------------- Unsharp Mask ------------------------- */
static void unsharp_mask(GDrawable *drawable, gint radius, gdouble amount) {
	GPixelRgn srcPR, destPR;
	glong width, height;
	glong bytes;
	gint x1, y1, x2, y2;
	gdouble* cmatrix = NULL;
	gint cmatrix_length;
	gdouble* ctable;
	
	/* Get the input */
	gimp_drawable_mask_bounds(drawable->id, &x1, &y1, &x2, &y2);
	gimp_progress_init("Blurring...");
	
	/* generate convolution matrix */
	cmatrix_length = gen_convolve_matrix(radius, &cmatrix);
	/* generate lookup table */
	ctable = gen_lookup_table(cmatrix, cmatrix_length);


	width = drawable->width;
	height = drawable->height;
	bytes = drawable->bpp;

	/* initialize pixel regions */
	gimp_pixel_rgn_init (&srcPR, drawable, 0, 0, width, height, FALSE, FALSE);
	gimp_pixel_rgn_init (&destPR, drawable, 0, 0, width, height, TRUE, TRUE);


	unsharp_region (srcPR, destPR, width, height, bytes, radius, amount,
	                x1, x2, y1, y2);


	gimp_drawable_flush(drawable);
	gimp_drawable_merge_shadow(drawable->id, TRUE);
	gimp_drawable_update(drawable->id, x1, y1, (x2-x1), (y2-y1));
}



/* perform an unsharp mask on the region, given a source region, dest.
   region, width and height of the regions, and corner coordinates of
   a subregion to act upon.  Everything outside the subregion is unaffected.
  */
static void unsharp_region (GPixelRgn srcPR, GPixelRgn destPR,
                   gint width, gint height, gint bytes,
                   gdouble radius, gdouble amount,
                   gint x1, gint x2, gint y1, gint y2) {
	guchar* cur_col;
	guchar* dest_col;
	guchar* cur_row;
	guchar* dest_row;
	gint x;
	gint y;
	gdouble* cmatrix = NULL;
	gint cmatrix_length;
	gdouble* ctable;

	gint row, col;  //these are counters for loops

	/* these are used for the merging step */
	gint threshold;
	gint diff;
	gint value;
	gint u,v;

	/* find height and width of subregion to act on */
	x = x2-x1;
	y = y2-y1;


	/* generate convolution matrix and make sure it's smaller than each dimension */
	cmatrix_length = gen_convolve_matrix(radius, &cmatrix);
	/* generate lookup table */
	ctable = gen_lookup_table(cmatrix, cmatrix_length);



	/*  allocate row buffers  */
	cur_row = (guchar *) g_malloc (x * bytes);
	dest_row = (guchar *) g_malloc (x * bytes);

	/* find height and width of subregion to act on */
	x = x2-x1;
	y = y2-y1;

	/* blank out a region of the destination memory area, I think */
	for (row = 0; row < y; row++) {
		gimp_pixel_rgn_get_row(&destPR, dest_row, x1, y1+row, (x2-x1));
		memset(dest_row, 0, x*bytes);
		gimp_pixel_rgn_set_row(&destPR, dest_row, x1, y1+row, (x2-x1));
	}


	/* blur the rows */
	for (row = 0; row < y; row++) {
		gimp_pixel_rgn_get_row(&srcPR, cur_row, x1, y1+row, x);
		gimp_pixel_rgn_get_row(&destPR, dest_row, x1, y1+row, x);
		blur_line(ctable, cmatrix, cmatrix_length, cur_row, dest_row, x, bytes);
		gimp_pixel_rgn_set_row(&destPR, dest_row, x1, y1+row, x);

		if (row%5 == 0) gimp_progress_update((gdouble)row/(3*y));
	}


	/* allocate column buffers */
	cur_col = (guchar *) g_malloc (y * bytes);
	dest_col = (guchar *) g_malloc (y * bytes);

	/* blur the cols */
	for (col = 0; col < x; col++) {
		gimp_pixel_rgn_get_col(&destPR, cur_col, x1+col, y1, y);
		gimp_pixel_rgn_get_col(&destPR, dest_col, x1+col, y1, y);
		blur_line(ctable, cmatrix, cmatrix_length, cur_col, dest_col, y, bytes);
		gimp_pixel_rgn_set_col(&destPR, dest_col, x1+col, y1, y);

		if (col%5 == 0) gimp_progress_update((gdouble)col/(3*x) + 0.33);
	}

	gimp_progress_init("Merging...");

	/* find integer value of threshold */
	threshold = unsharp_params.threshold;
	
	/* merge the source and destination (which currently contains
	   the blurred version) images */
	for (row = 0; row < y; row++) {
		value = 0;
		/* get source row */
		gimp_pixel_rgn_get_row(&srcPR, cur_row, x1, y1+row, x);
		/* get dest row */
		gimp_pixel_rgn_get_row(&destPR, dest_row, x1, y1+row, x);
		/* combine the two */
		for (u=0; u<x; u++) {
			for (v=0; v<bytes; v++) {
				diff = (cur_row[u*bytes+v] - dest_row[u*bytes+v]);
				/* do tresholding */
				if ( abs(2*diff) < threshold )
					diff = 0;

				value = cur_row[u*bytes+v] + amount * diff;
				
				if (value < 0) dest_row[u*bytes+v] =0;
				else if (value > 255) dest_row[u*bytes+v] = 255;
				else  dest_row[u*bytes+v] = value;
			}
		}
		/* update progress bar every five rows */
		if (row%5 == 0) gimp_progress_update((gdouble)row/(3*y) + 0.67);
		gimp_pixel_rgn_set_row(&destPR, dest_row, x1, y1+row, x);
	}

	/* free the memory we took */
	g_free(cur_row);
	g_free(dest_row);
	g_free(cur_col);
	g_free(dest_col);

}

/* this function is written as if it is blurring a column at a time,
   even though it can operate on rows, too.  There is no difference
   in the processing of the lines, at least to the blur_line function. */
static inline void blur_line(gdouble* ctable, gdouble* cmatrix,
                             gint cmatrix_length,
                             guchar* cur_col, guchar* dest_col,
                             gint y, glong bytes) {

 
#ifdef READABLE_CODE
/* ------------- semi-readable code ------------------- */
	gdouble scale;
	gdouble sum;
	gint i,j;
	gint row;


	// this is to take care cases in which the matrix can go over
	// both edges at once.  It's not efficient, but this can only
	// happen in small pictures anyway.
	if (cmatrix_length > y) {
	
		for (row = 0; row < y ; row++) {
			scale=0;
			// find the scale factor
			for (j = 0; j < y ; j++) {
				// if the index is in bounds, add it to the scale counter
				if ((j + cmatrix_length/2 - row >= 0) &&
				    (j + cmatrix_length/2 - row < cmatrix_length))
					scale += cmatrix[j + cmatrix_length/2 - row];
			}
			for (i = 0; i<bytes; i++) {
				sum = 0;
				for (j = 0; j < y; j++) {
					if ( (j >= row - cmatrix_length/2) && (j <= row + cmatrix_length/2) )
						sum += cur_col[j*bytes + i] * cmatrix[j];
				}
 				dest_col[row*bytes + i] = (guchar)round2int(sum / scale);
			}
		}
	}
	
	else {  // when the cmatrix is smaller than row length

		// for the edge condition, we only use available info, and scale to one
		for (row = 0; row < cmatrix_length/2; row++) {
			// find scale factor
			scale=0;
			for (j = cmatrix_length/2 - row; j<cmatrix_length; j++)
				scale += cmatrix[j];

			for (i = 0; i<bytes; i++) {
				sum = 0;
				for (j = cmatrix_length/2 - row; j<cmatrix_length; j++) {
					sum += cur_col[(row + j-cmatrix_length/2)*bytes + i] * cmatrix[j];
				}
				dest_col[row*bytes + i] = (guchar)round2int(sum / scale);
			}
		}
		// go through each pixel in each col
		for ( ; row < y-cmatrix_length/2; row++) {
			for (i = 0; i<bytes; i++) {
				sum = 0;
				for (j = 0; j<cmatrix_length; j++) {
					sum += cur_col[(row + j-cmatrix_length/2)*bytes + i] * cmatrix[j];
				}
				dest_col[row*bytes + i] = (guchar)round2int(sum);
			}
		}
		// for the edge condition , we only use available info, and scale to one
		for ( ; row < y; row++) {
			// find scale factor
			scale=0;
			for (j = 0; j< y-row + cmatrix_length/2; j++)
				scale += cmatrix[j];
				
			for (i = 0; i<bytes; i++) {
				sum = 0;
				for (j = 0; j<y-row + cmatrix_length/2; j++) {
					sum += cur_col[(row + j-cmatrix_length/2)*bytes + i] * cmatrix[j];
				}
				dest_col[row*bytes + i] = (guchar)round2int(sum / scale);
			}
		}
	}
#endif
#ifndef READABLE_CODE
	/* --------------- optimized, unreadable code -------------------*/

	gdouble scale;
	gdouble sum;
	gint i=0, j=0;
	gint row;
	gint cmatrix_middle = cmatrix_length/2;

	gdouble *cmatrix_p;
	guchar  *cur_col_p;
	guchar  *cur_col_p1;
	guchar  *dest_col_p;
	gdouble *ctable_p;


	// this first block is the same as the non-optimized version --
	// it is only used for very small pictures, so speed isn't a
	// big concern.
	if (cmatrix_length > y) {
	
		for (row = 0; row < y ; row++) {
			scale=0;
			// find the scale factor
			for (j = 0; j < y ; j++) {
				// if the index is in bounds, add it to the scale counter
				if ((j + cmatrix_length/2 - row >= 0) &&
				    (j + cmatrix_length/2 - row < cmatrix_length))
					scale += cmatrix[j + cmatrix_length/2 - row];
			}
			for (i = 0; i<bytes; i++) {
				sum = 0;
				for (j = 0; j < y; j++) {
					if ( (j >= row - cmatrix_length/2) && (j <= row + cmatrix_length/2) )
						sum += cur_col[j*bytes + i] * cmatrix[j];
				}
 				dest_col[row*bytes + i] = (guchar)round2int(sum / scale);
			}
		}
	}

	else {
		// for the edge condition, we only use available info and scale to one
		for (row = 0; row < cmatrix_middle; row++) {
			// find scale factor
			scale=0;
			for (j = cmatrix_middle - row; j<cmatrix_length; j++)
				scale += cmatrix[j];
			for (i = 0; i<bytes; i++) {
				sum = 0;
				for (j = cmatrix_middle - row; j<cmatrix_length; j++) {
					sum += cur_col[(row + j-cmatrix_middle)*bytes + i] * cmatrix[j];
				}
				dest_col[row*bytes + i] = (guchar)round2int(sum / scale);
			}
		}
		// go through each pixel in each col
		dest_col_p = dest_col + row*bytes;
		for ( ; row < y-cmatrix_middle; row++) {
			cur_col_p = (row - cmatrix_middle) * bytes + cur_col;
			for (i = 0; i<bytes; i++) {
				sum = 0;
				cmatrix_p = cmatrix;
				cur_col_p1 = cur_col_p;
				ctable_p = ctable;
				for (j = cmatrix_length; j>0; j--) {
					sum += *(ctable_p + *cur_col_p1);
					cur_col_p1 += bytes;
					ctable_p += 256;
				}
				cur_col_p++;
				*(dest_col_p++) = round2int(sum);
			}
		}
	
		// for the edge condition , we only use available info, and scale to one
		for ( ; row < y; row++) {
			// find scale factor
			scale=0;
			for (j = 0; j< y-row + cmatrix_middle; j++)
				scale += cmatrix[j];
			for (i = 0; i<bytes; i++) {
				sum = 0;
				for (j = 0; j<y-row + cmatrix_middle; j++) {
					sum += cur_col[(row + j-cmatrix_middle)*bytes + i] * cmatrix[j];
				}
				dest_col[row*bytes + i] = (guchar)round2int(sum / scale);
			}
		}
	}
#endif

}

static inline gint round2int(gdouble i) {
	return (gint)(i + 0.5);
}


/* generates a 1-D convolution matrix to be used for each pass of 
 * a two-pass gaussian blur.  Returns the length of the matrix.
 */
static gint gen_convolve_matrix(gdouble radius, gdouble** cmatrix_p) {
	gint matrix_length;
	gint matrix_midpoint;
	gdouble* cmatrix;
	gint i,j;
	gdouble std_dev;
	gdouble sum;
	
	/* we want to generate a matrix that goes out a certain radius
	 * from the center, so we have to go out ceil(rad-0.5) pixels,
	 * inlcuding the center pixel.  Of course, that's only in one direction,
	 * so we have to go the same amount in the other direction, but not count
	 * the center pixel again.  So we double the previous result and subtract
	 * one.
   * The radius parameter that is passed to this function is used as
	 * the standard deviation, and the radius of effect is the
	 * standard deviation * 2.  It's a little confusing.
	 */
	radius = fabs(radius) + 1.0;
	
	std_dev = radius;
	radius = std_dev * 2;

	/* go out 'radius' in each direction */
	matrix_length = 2 * ceil(radius-0.5) + 1;
	if (matrix_length <= 0) matrix_length = 1;
	matrix_midpoint = matrix_length/2 + 1;
	*cmatrix_p = (gdouble*)(malloc(matrix_length * sizeof(gdouble)));
	cmatrix = *cmatrix_p;

	/*  Now we fill the matrix by doing a numeric integration approximation
	 * from -2*std_dev to 2*std_dev, sampling 50 points per pixel.
	 * We do the bottom half, mirror it to the top half, then compute the
	 * center point.  Otherwise asymmetric quantization errors will occur.
	 *  The formula to integrate is e^-(x^2/2s^2).
	 */

	/* first we do the top (right) half of matrix */
	for (i=matrix_length/2 + 1; i<matrix_length; i++) {
		double base_x = i - floor(matrix_length/2) - 0.5;
		sum = 0;
		for (j=1; j<=50; j++) {
			if ( base_x+0.02*j <= radius ) 
				sum += exp( -(base_x+0.02*j)*(base_x+0.02*j) / 
				                 (2*std_dev*std_dev) );
		}
		cmatrix[i] = sum/50;
	}

	/* mirror the thing to the bottom half */
	for (i=0; i<=matrix_length/2; i++) {
		cmatrix[i] = cmatrix[matrix_length-1-i];
	}
	
	/* find center val -- calculate an odd number of quanta to make it symmetric,
	 * even if the center point is weighted slightly higher than others. */
	sum = 0;
	for (j=0; j<=50; j++) {
		sum += exp( -(0.5+0.02*j)*(0.5+0.02*j) /
		                 (2*std_dev*std_dev) );
	}
	cmatrix[matrix_length/2] = sum/51;

	
	/* normalize the distribution by scaling the total sum to one */
	sum=0;
	for (i=0; i<matrix_length; i++) sum += cmatrix[i];
	for (i=0; i<matrix_length; i++) cmatrix[i] = cmatrix[i] / sum;

	return matrix_length;
}





/* ----------------------- gen_lookup_table ----------------------- */
/* generates a lookup table for every possible product of 0-255 and
   each value in the convolution matrix.  The returned array is
   indexed first by matrix position, then by input multiplicand (?)
   value.
*/
static gdouble* gen_lookup_table(gdouble* cmatrix, gint cmatrix_length) {
	int i, j;
	gdouble* lookup_table = malloc(cmatrix_length * 256 * sizeof(gdouble));

#ifdef READABLE_CODE
	for (i=0; i<cmatrix_length; i++) {
		for (j=0; j<256; j++) {
			lookup_table[i*256 + j] = cmatrix[i] * (gdouble)j;
		}
	}
#endif

#ifndef READABLE_CODE
	gdouble* lookup_table_p = lookup_table;
	gdouble* cmatrix_p      = cmatrix;
	for (i=0; i<cmatrix_length; i++) {
		for (j=0; j<256; j++) {
			*(lookup_table_p++) = *cmatrix_p * (gdouble)j;
		}
		cmatrix_p++;
	}
#endif

	return lookup_table;
}




/* ------------------------ unsharp_mask_dialog ----------------------- */
static gint unsharp_mask_dialog() {

	GtkWidget *window;
	GtkWidget *table;
	GtkWidget *button;


	gint    argc = 1;
	gchar** argv = g_new(gchar*, 1);
	argv[0]      = g_strdup("unsharp mask");

	/* initialize */
	gtk_init(&argc, &argv);

	gtk_rc_parse(gimp_gtkrc());
	gdk_set_use_xshm(gimp_use_xshm());
	gtk_preview_set_gamma(gimp_gamma());
	gtk_preview_set_install_cmap(gimp_install_cmap());
	
	/* create a new window */
	window = gtk_dialog_new();
	gtk_window_set_title(GTK_WINDOW(window), "Unsharp Mask " PLUG_IN_VERSION);
	/* I have no idea what the following two lines do. 
	   I took them from sharpen.c */
	gtk_window_set_wmclass(GTK_WINDOW(window), "unsharp mask", "Gimp");
	gtk_window_position(GTK_WINDOW(window), GTK_WIN_POS_MOUSE);
	 

	gtk_signal_connect (GTK_OBJECT (window), "delete_event",
	                    GTK_SIGNAL_FUNC (unsharp_cancel_callback), NULL);
	
	gtk_signal_connect (GTK_OBJECT (window), "destroy",
	                    GTK_SIGNAL_FUNC (unsharp_cancel_callback), NULL);

	

	table = gtk_table_new(3, 3, FALSE);  //Make a 3x3 table in mainbox
	gtk_box_pack_start(GTK_BOX( GTK_DIALOG(window)->vbox), table,
	                   FALSE, FALSE, 0);
	
	

	/* create each of the inputs */
	dialog_create_value_f("Radius:", GTK_TABLE(table), 1, &unsharp_params.radius,
	                      0.1, 1, 1.0, 25.0);
	dialog_create_value_f("Amount:", GTK_TABLE(table), 2, &unsharp_params.amount,
	                      0.01, 2, 0.01, 5.0);
	dialog_create_value_i("Threshold:", GTK_TABLE(table), 3, &unsharp_params.threshold,
	                      1, 0, 255);

	
	/* show table */
	gtk_widget_show(table);


	/* OK and Cancel buttons */
	gtk_container_border_width(GTK_CONTAINER(GTK_DIALOG(window)->action_area),2);

		// Make OK button
		button = gtk_button_new_with_label("OK");
		GTK_WIDGET_SET_FLAGS(button, GTK_CAN_DEFAULT);
		gtk_box_pack_start (GTK_BOX (GTK_DIALOG (window)->action_area), button,
		                    TRUE, TRUE, 0);
		gtk_signal_connect( GTK_OBJECT(button), "clicked",
		                    GTK_SIGNAL_FUNC(unsharp_ok_callback), window);
		gtk_widget_grab_default(button);
		gtk_widget_show(button);

		// Make Cancel button
		button = gtk_button_new_with_label("Cancel");
		GTK_WIDGET_SET_FLAGS(button, GTK_CAN_DEFAULT);
		
		gtk_box_pack_start (GTK_BOX (GTK_DIALOG (window)->action_area), button,
		                    TRUE, TRUE, 0);
		gtk_signal_connect ( GTK_OBJECT(button), "clicked",
		                    GTK_SIGNAL_FUNC(unsharp_cancel_callback), NULL);
		gtk_widget_show(button);

	
	
	/* show the window */
	gtk_widget_show (window);


	gtk_main ();
	gdk_flush();

	/* return ok or cancel */
	return(run_filter);


}

#ifdef PREVIEW
/* 'preview_init()' - Initialize the preview window... */

static void
preview_init(void)
{
  int	width;		/* Byte width of the image */

 /*
  * Setup for preview filter...
  */

  width = preview_width * img_bpp;

  preview_src   = g_malloc(width * preview_height * sizeof(guchar));
  preview_neg   = g_malloc(width * preview_height * sizeof(guchar));
  preview_dst   = g_malloc(width * preview_height * sizeof(guchar));
  preview_image = g_malloc(preview_width * preview_height * 3 * sizeof(guchar));

  preview_x1 = sel_x1;
  preview_y1 = sel_y1;
  preview_x2 = preview_x1 + preview_width;
  preview_y2 = preview_y1 + preview_height;
}

/*
 * 'preview_scroll_callback()' - Update the preview when a scrollbar is moved.
 */

static void
preview_scroll_callback(void)
{
  preview_x1 = sel_x1 + GTK_ADJUSTMENT(hscroll_data)->value;
  preview_y1 = sel_y1 + GTK_ADJUSTMENT(vscroll_data)->value;
  preview_x2 = preview_x1 + MIN(preview_width, sel_width);
  preview_y2 = preview_y1 + MIN(preview_height, sel_height);

  preview_update();
}


/*
 * 'preview_update()' - Update the preview window.
 */

static void
preview_update(void) {

}
#endif

static void unsharp_cancel_callback (GtkWidget *widget, gpointer data) {
	gtk_main_quit ();
}

static void unsharp_ok_callback (GtkWidget *widget, gpointer data) {
	run_filter = TRUE;
	gtk_widget_destroy (GTK_WIDGET (data));
}

/* delete_event */
gboolean delete_event( GtkWidget *widget, GdkEvent *event, gpointer data ) {

	return FALSE;
}


/* Destroy callback */
void destroy( GtkWidget *widget, gpointer data ) {
	gtk_main_quit();
}


#ifdef TIMER
static void timerstart() {
	gettimeofday(&time_start,NULL);
}
static void timerstop() {
	long sec, usec;
	
	gettimeofday(&time_stop,NULL);
	sec = time_stop.tv_sec  - time_start.tv_sec;
	usec = time_stop.tv_usec - time_start.tv_usec;
	
	if (usec < 0) {
		sec--;
		usec += 1000000;
	}
	
	fprintf(stderr, "%ld.%ld seconds\n", sec, usec);

}
#endif

