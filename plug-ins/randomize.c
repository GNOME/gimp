/****************************************************************************
 * This is a plugin for the GIMP v 0.99.8 or later.
 *
 * Copyright (C) 1997 Miles O'Neal
 * Blur code Copyright (C) 1995 Spencer Kimball and Peter Mattis
 * GUI based on GTK code from:
 *    plasma (Copyright (C) 1996 Stephen Norris),
 *    oilify (Copyright (C) 1996 Torsten Martinsen),
 *    ripple (Copyright (C) 1997 Brian Degenhardt) and
 *    whirl  (Copyright (C) 1997 Federico Mena Quintero).
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
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 ****************************************************************************/

/****************************************************************************
 * Randomize:
 *
 * randomize version 0.4c (17 May 1997, MEO)
 * history
 *     0.5 - 20 May 1997 MEO
 *         added seed initialization choices (current time or user value)
 *         added randomization type to progress label
 *         added RNDM_VERSION macro so version changes made in one place
 *         speed optimizations:
 *             - changed randomize_prepare_row to #define
 *             - moved updates back outside main loop
 *             - less frequent progress updates
 *         minor intialization string and comment cleanup
 *     0.4c - 17 May 1997 MEO
 *         minor comment cleanup
 *     0.4b - 17 May 1997 MEO
 *         minor comment cleanup
 *     0.4a - 16 May 1997 MEO
 *         added, corrected & cleaned up comments
 *         removed unused variables, code
 *         cleaned up wrong names
 *         added version to popups
 *     0.4 - 13 May 1997 MEO
 *         added SLUR function
 *     0.3 - 12 May 1997 MEO
 *         added HURL function
 *         moved from Blurs menu to Distorts menu.
 *     0.2 - 11 May 1997 MEO
 *         converted percentage control from text to scale
 *         standardized tab stops, style
 *     0.1 - 10 May 1997 MEO
 *         initial release, with BLUR and PICK
 *
 * Please send any patches or suggestions to the author: meo@rru.com .
 * 
 * This plug-in adds a user-defined amount of randomization to an
 * image.  Variations include:
 * 
 *  - blurring
 *  - hurling (spewing random colors)
 *  - picking a nearby pixel at random
 *  - slurring (a crude form of melting)
 * 
 * In any case, for each pixel in the selection or image,
 * whether to change the pixel is decided by picking a
 * random number, weighted by the user's "randomization" percentage.
 * If the random number is in range, the pixel is modified.  For
 * blurring, an average is determined from the current and adjacent
 * pixels. *(Except for the random factor, the blur code came
 * straight from the original S&P blur plug-in.)*  Picking
 * one selects the new pixel value at random from the current and
 * adjacent pixels.  Hurling assigns a random value to the pixel.
 * Slurring sort of melts downwards; if a pixel is to be slurred,
 * there is an 80% chance the pixel above be used; otherwise, one
 * of the pixels adjacent to the one above is used (even odds as
 * to which it will be).
 * 
 * Picking, hurling and slurring work with any image type.  Blurring
 * works only with RGB and grayscale images.  If randomize is
 * run against an indexed image, "blur" is not presented as an
 * option.
 * 
 * This plug-in's effectiveness varies a lot with the type
 * and clarity of the image being "randomized".
 * 
 * Hurling more than 75% or so onto an existing image will
 * make the image nearly unrecognizable.  By 90% hurl, most
 * images are indistinguishable from random noise.
 * 
 * The repeat count is especially useful with slurring.
 * 
 * TODO List
 * 
 *  - Add a real melt function.
 ****************************************************************************/

#include <stdlib.h>
#include <time.h>
#include "libgimp/gimp.h"
#include "gtk/gtk.h"

/*
 *  Set this to 0 for faster processing, to 1 if for some
 *  reason you want all functions to be subroutines
 */
#define SUBS_NOT_DEFINES 0
/*
 *  progress meter update frequency
 */
#define PROG_UPDATE_TIME ((row % 10) == 0)

#define RNDM_VERSION "Randomize 0.5"

/*********************************
 *
 *  LOCAL DATA
 *
 ********************************/

#define RNDM_BLUR 1
#define RNDM_PICK 2
#define RNDM_HURL 3
#define RNDM_SLUR 4

#define SEED_TIME 10
#define SEED_USER 11

#define ENTRY_WIDTH 75
#define SCALE_WIDTH 100

typedef struct {
    gint rndm_type;       /* type of randomization to apply */
    gdouble rndm_pct;     /* likelihood of randomization (as %age) */
    gint seed_type;       /* seed init. type - current time or user value */
    gint rndm_seed;       /* seed value for rand() function */
    gdouble rndm_rcount;  /* repeat count */
} RandomizeVals;

static RandomizeVals pivals = {
    RNDM_BLUR,
    50.0,
    SEED_TIME,
    0,
    1.0,
};

typedef struct {
    gint run;
} RandomizeInterface;

static RandomizeInterface rndm_int = {
    FALSE     /*  have we run? */
};

static gint is_indexed_drawable = FALSE;

/*********************************
 *
 *  LOCAL FUNCTIONS
 *
 ********************************/

static void query(void);
static void run(
    char *name,
    int nparams,
    GParam *param,
    int *nreturn_vals,
    GParam **return_vals
);

GPlugInInfo PLUG_IN_INFO = {
    NULL,    /* init_proc */
    NULL,    /* quit_proc */
    query,   /* query_proc */
    run,     /* run_proc */
};

static void randomize(GDrawable *drawable);
#if (SUBS_NOT_DEFINES == 1)
static void randomize_prepare_row(
    GPixelRgn *pixel_rgn,
    guchar *data,
    int x,
    int y,
    int w
);
#endif

static gint randomize_dialog();

static void randomize_close_callback(
    GtkWidget *widget,
    gpointer data
);
static void randomize_ok_callback(
    GtkWidget *widget,
    gpointer data
);
static void randomize_cancel_callback(
    GtkWidget *widget,
    gpointer data
);
static void randomize_scale_update(
    GtkAdjustment *adjustment,
    double *scale_val
);

static void randomize_toggle_update(
    GtkWidget *widget,
    gpointer data
);

static void randomize_text_update(
    GtkWidget *widget,
    gpointer data
);


MAIN()

/*********************************
 *
 *  query() - query_proc
 *
 *      called by the GIMP to learn about this plug-in
 *
 ********************************/

static void
query()
{
    static GParamDef args[] = {
	{ PARAM_INT32, "run_mode", "Interactive, non-interactive" },
	{ PARAM_IMAGE, "image", "Input image (unused)" },
	{ PARAM_DRAWABLE, "drawable", "Input drawable" },
	{ PARAM_INT32, "rndm_type", "Randomization type" },
	{ PARAM_FLOAT, "rndm_pct", "Randomization percentage" },
	{ PARAM_FLOAT, "rndm_rcount", "Repeat count" },
    };
    static GParamDef *return_vals = NULL;
    static int nargs = sizeof(args) / sizeof (args[0]);
    static int nreturn_vals = 0;

    gimp_install_procedure("plug_in_randomize",
	"Add a random factor to the image, by blurring, picking a nearby pixel, slurring (similar to melting), or just hurling on it.",
	"This function randomly ``blurs'' the specified drawable, using either a 3x3 blur, picking a nearby pixel, slurring (cheezy melting), or hurling (spewing colors).  The type and percentage are user selectable.  Blurring is not supported for indexed images.",
	"Miles O'Neal",
	"Miles O'Neal, Spencer Kimball, Peter Mattis, Torsten Martinsen, Brian Degenhardt, Federico Mena Quintero",
	"1995-1997",
	"<Image>/Filters/Distorts/Randomize",
	"RGB*, GRAY*, INDEXED*",
	PROC_PLUG_IN,
	nargs, nreturn_vals,
	args, return_vals);
}

/*
 *  If it's indexed and the current action is BLUR,
 *  use something else.  PICK seems the likeliest
 *  candidate, although in the spirit of randomity
 *  we could pick one at random!
 */
#define FIX_INDEX_BLUR() \
if (gimp_drawable_indexed(drawable->id)) { \
    is_indexed_drawable = TRUE; \
    if (pivals.rndm_type == RNDM_BLUR) { \
	pivals.rndm_type = RNDM_PICK; \
    } \
}


/*********************************
 *
 *  run() - main routine
 *
 *  This handles the main interaction with the GIMP itself,
 *  and invokes the routine that actually does the work.
 *
 ********************************/

static void
run(char *name, int nparams, GParam *param, int *nreturn_vals,
    GParam **return_vals)
{

    static GParam values[1];
    GDrawable *drawable;
    GRunModeType run_mode;
    GStatusType status = STATUS_SUCCESS;	/* assume the best! */
    char *rndm_type_str = '\0';
    char prog_label[32];
/*
 *  Get the specified drawable, do standard initialization.
 */
    run_mode = param[0].data.d_int32;
    drawable = gimp_drawable_get(param[2].data.d_drawable);

    values[0].type = PARAM_STATUS;
    values[0].data.d_status = status;
    *nreturn_vals = 1;
    *return_vals = values;
/*
 *  Make sure the drawable type is appropriate.
 */
    if (gimp_drawable_color(drawable->id) ||
      gimp_drawable_gray(drawable->id) ||
      gimp_drawable_indexed(drawable->id)) {

	switch (run_mode) {
/*
 *  If we're running interactively, pop up the dialog box.
 */
	    case RUN_INTERACTIVE:
		FIX_INDEX_BLUR();
		gimp_get_data("plug_in_randomize", &pivals);
		if (!randomize_dialog())	/* return on Cancel */
		    return;
		break;
/*
 *  If we're not interactive (probably scripting), we
 *  get the parameters from the param[] array, since
 *  we don't use the dialog box.  Make sure they all
 *  parameters have legitimate values.
 */
	    case RUN_NONINTERACTIVE:
		FIX_INDEX_BLUR();
		if (nparams != 6) {
		    status = STATUS_CALLING_ERROR;
		}
		if (status == STATUS_SUCCESS) {
		    pivals.rndm_type = (gint) param[3].data.d_int32;
		    pivals.rndm_pct = (gdouble) param[4].data.d_float;
		    pivals.rndm_rcount = (gdouble) param[5].data.d_int32;
		}
		if (status == STATUS_SUCCESS &&
		  ((pivals.rndm_type != RNDM_PICK &&
		    pivals.rndm_type != RNDM_BLUR &&
		    pivals.rndm_type != RNDM_SLUR &&
		    pivals.rndm_type != RNDM_HURL) ||
		  (pivals.rndm_pct < 1.0 || pivals.rndm_pct > 100.0) ||
                  (pivals.rndm_rcount < 1.0 || pivals.rndm_rcount > 100.0))) {
		    status = STATUS_CALLING_ERROR;
		}
		break;
/*
 *  If we're running with the last set of values, get those values.
 */
	    case RUN_WITH_LAST_VALS:
		gimp_get_data("plug_in_randomize", &pivals);
		break;
/*
 *  Hopefully we never get here!
 */
	    default:
		break;
	}
/*
 *  JUST DO IT!
 */
	switch (pivals.rndm_type) {
            case RNDM_BLUR: rndm_type_str = "blur"; break;
            case RNDM_HURL: rndm_type_str = "hurl"; break;
            case RNDM_PICK: rndm_type_str = "pick"; break;
            case RNDM_SLUR: rndm_type_str = "slur"; break;
        }
	sprintf(prog_label, "%s (%s)", RNDM_VERSION, rndm_type_str);
	gimp_progress_init(prog_label);
	gimp_tile_cache_ntiles(2 * (drawable->width / gimp_tile_width() + 1));
/*
 *  Initialie the rand() function seed
 */
	if (pivals.seed_type == SEED_TIME)
	    srand(time(NULL));
	else
	    srand(pivals.rndm_seed);

	randomize(drawable);
/*
 *  If we ran interactively (even repeating) update the display.
 */
        if (run_mode != RUN_NONINTERACTIVE) {
	    gimp_displays_flush();
	}
/*
 *  If we use the dialog popup, set the data for future use.
 */
	if (run_mode == RUN_INTERACTIVE) {
	    gimp_set_data("plug_in_randomize", &pivals, sizeof(RandomizeVals));
	}
    } else {
/*
 *  If we got the wrong drawable type, we need to complain.
 */
	status = STATUS_EXECUTION_ERROR;
    }
/*
 *  DONE!
 *  Set the status where the GIMP can see it, and let go
 *  of the drawable.
 */
    values[0].data.d_status = status;
    gimp_drawable_detach(drawable);
}

/*********************************
 *
 *  randomize_prepare_row()
 *
 *  Get a row of pixels.  If the requested row
 *  is off the edge, clone the edge row.
 *
 ********************************/

#if (SUBS_NOT_DEFINES == 1)
static void
randomize_prepare_row(GPixelRgn *pixel_rgn, guchar *data, int x, int y, int w)
{
    int b;

    if (y == 0) {
      gimp_pixel_rgn_get_row(pixel_rgn, data, x, (y + 1), w);
    } else if (y == pixel_rgn->h) {
      gimp_pixel_rgn_get_row(pixel_rgn, data, x, (y - 1), w);
    } else {
      gimp_pixel_rgn_get_row(pixel_rgn, data, x, y, w);
    }
/*
 *  Fill in edge pixels
 */
    for (b = 0; b < pixel_rgn->bpp; b++) {
	data[-pixel_rgn->bpp + b] = data[b];
	data[w * pixel_rgn->bpp + b] = data[(w - 1) * pixel_rgn->bpp + b];
    }
}
#else
#define randomize_prepare_row(pixel_rgn, data, x, y, w) \
{ \
    int b; \
 \
    if (y == 0) { \
      gimp_pixel_rgn_get_row(pixel_rgn, data, x, (y + 1), w); \
    } else if (y == (pixel_rgn)->h) { \
      gimp_pixel_rgn_get_row(pixel_rgn, data, x, (y - 1), w); \
    } else { \
      gimp_pixel_rgn_get_row(pixel_rgn, data, x, y, w); \
    } \
/* \
 *  Fill in edge pixels \
 */ \
    for (b = 0; b < (pixel_rgn)->bpp; b++) { \
	data[-(pixel_rgn)->bpp + b] = data[b]; \
	data[w * (pixel_rgn)->bpp + b] = data[(w - 1) * (pixel_rgn)->bpp + b]; \
    } \
}
#endif

/*********************************
 *
 *  randomize()
 *
 *  Actually mess with the image.
 *
 ********************************/

static void
randomize(GDrawable *drawable)
{
    GPixelRgn srcPR, destPR;
    gint width, height;
    gint bytes;
    guchar *dest, *d;
    guchar *prev_row, *pr;
    guchar *cur_row, *cr;
    guchar *next_row, *nr;
    guchar *tmp;
    gint row, col;
    gint x1, y1, x2, y2;
    gint cnt;

/*
 *  Get the input area. This is the bounding box of the selection in
 *  the image (or the entire image if there is no selection). Only
 *  operating on the input area is simply an optimization. It doesn't
 *  need to be done for correct operation. (It simply makes it go
 *  faster, since fewer pixels need to be operated on).
 */
    gimp_drawable_mask_bounds(drawable->id, &x1, &y1, &x2, &y2);
/*
 *  Get the size of the input image. (This will/must be the same
 *  as the size of the output image.
 */
    width = drawable->width;
    height = drawable->height;
    bytes = drawable->bpp;
/*
 *  allocate row buffers
 */
    prev_row = (guchar *) malloc((x2 - x1 + 2) * bytes);
    cur_row = (guchar *) malloc((x2 - x1 + 2) * bytes);
    next_row = (guchar *) malloc((x2 - x1 + 2) * bytes);
    dest = (guchar *) malloc((x2 - x1) * bytes);

    for (cnt = 1; cnt <= pivals.rndm_rcount; cnt++) {
/*
 *  initialize the pixel regions
 */
	gimp_pixel_rgn_init(&srcPR, drawable, 0, 0, width, height, FALSE, FALSE);
	gimp_pixel_rgn_init(&destPR, drawable, 0, 0, width, height, TRUE, TRUE);

	pr = prev_row + bytes;
	cr = cur_row + bytes;
	nr = next_row + bytes;
/*
 *  prepare the first row and previous row
 */
	randomize_prepare_row(&srcPR, pr, x1, y1 - 1, (x2 - x1));
	randomize_prepare_row(&srcPR, cr, x1, y1, (x2 - x1));
/*
 *  loop through the rows, applying the selected convolution
 */
	for (row = y1; row < y2; row++) {
	    /*  prepare the next row  */
	    randomize_prepare_row(&srcPR, nr, x1, row + 1, (x2 - x1));

	    d = dest;
	    for (col = 0; col < (x2 - x1) * bytes; col++) {
		if (((rand() % 100)) <= (gint) pivals.rndm_pct) {
		    switch (pivals.rndm_type) {
/*
 *  BLUR
 *      Use the average of the neighboring pixels.
 */
			case RNDM_BLUR:
			*d++ = ((gint) pr[col - bytes] + (gint) pr[col] +
			    (gint) pr[col + bytes] +
			    (gint) cr[col - bytes] + (gint) cr[col] +
			    (gint) cr[col + bytes] +
			    (gint) nr[col - bytes] + (gint) nr[col] +
			    (gint) nr[col + bytes]) / 9;
			break;
/*
 *  HURL
 *      Just assign a random value.
 */
			case RNDM_HURL:
			*d++ = rand() % 256;
			break;
/*
 *  PICK
 *      pick at random from a neighboring pixel.
 */
			case RNDM_PICK:
			    switch (rand() % 9) {
				case 0:
				    *d++ = (gint) pr[col - bytes];
				    break;
				case 1:
				    *d++ = (gint) pr[col];
				    break;
				case 2:
				    *d++ = (gint) pr[col + bytes];
				    break;
				case 3:
				    *d++ = (gint) cr[col - bytes];
				    break;
				case 4:
				    *d++ = (gint) cr[col];
				    break;
				case 5:
				    *d++ = (gint) cr[col + bytes];
				    break;
				case 6:
				    *d++ = (gint) nr[col - bytes];
				    break;
				case 7:
				    *d++ = (gint) nr[col];
				    break;
				case 8:
				    *d++ = (gint) nr[col + bytes];
				    break;
			    }
			    break;
/*
 *  SLUR
 *      80% chance it's from directly above,
 *      10% from above left,
 *      10% from above right.
 */
		       case RNDM_SLUR:
			    switch (rand() % 10) {
				case 0:
				    *d++ = (gint) pr[col - bytes];
				    break;
				case 9:
				    *d++ = (gint) pr[col + bytes];
				    break;
				default:
				    *d++ = (gint) pr[col];
				    break;
			    }
			    break;
		    }
/*
 *  Otherwise, this pixel was not selected for randomization,
 *  so use the current value.
 */
		} else {
		    *d++ = (gint) cr[col];
		}
	    }
/*
 *  Save the modified row, shuffle the row pointers, and every
 *  so often, update the progress meter.
 */
	    gimp_pixel_rgn_set_row(&destPR, dest, x1, row, (x2 - x1));

	    tmp = pr;
	    pr = cr;
	    cr = nr;
	    nr = tmp;

	    if (PROG_UPDATE_TIME)
		gimp_progress_update((double) row / (double) (y2 - y1));
	}
    }
    gimp_progress_update((double) 100);
/*
 *  update the randomized region
 */
    gimp_drawable_flush(drawable);
    gimp_drawable_merge_shadow(drawable->id, TRUE);
    gimp_drawable_update(drawable->id, x1, y1, (x2 - x1), (y2 - y1));
/*
 *  clean up after ourselves.
 */
    free(prev_row);
    free(cur_row);
    free(next_row);
    free(dest);
}

/*********************************
 *
 *  GUI ROUTINES
 *
 ********************************/

#define randomize_add_action_button(label, callback, dialog) \
{ \
    GtkWidget *button; \
\
    button = gtk_button_new_with_label(label); \
    GTK_WIDGET_SET_FLAGS(button, GTK_CAN_DEFAULT); \
    gtk_signal_connect(GTK_OBJECT(button), "clicked", callback, dialog); \
    gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->action_area), \
	button, TRUE, TRUE, 0); \
    gtk_widget_grab_default(button); \
    gtk_widget_show(button); \
}

#define randomize_add_radio_button(group, label, box, callback, value) \
{  \
    GtkWidget *toggle; \
\
    toggle = gtk_radio_button_new_with_label(group, label); \
    group = gtk_radio_button_group(GTK_RADIO_BUTTON(toggle)); \
    gtk_box_pack_start(GTK_BOX(box), toggle, FALSE, FALSE, 0); \
    gtk_signal_connect(GTK_OBJECT(toggle), "toggled", \
        (GtkSignalFunc) callback, value); \
    gtk_toggle_button_set_state(GTK_TOGGLE_BUTTON(toggle), *value); \
    gtk_widget_show(toggle); \
    gtk_widget_show(box); \
}

/*********************************
 *
 *  randomize_dialog() - set up the plug-in's dialog box
 *
 ********************************/

static gint
randomize_dialog()
{
    GtkWidget *dlg, *entry, *frame, *label, *scale,
	*seed_hbox, *seed_vbox, *table, *toggle_hbox;
    GtkObject *scale_data;
    GSList *type_group = NULL;
    GSList *seed_group = NULL;
    gchar **argv;
    gint argc;
    gchar buffer[10];
/*
 *  various initializations
 */
    gint do_blur = (pivals.rndm_type == RNDM_BLUR);
    gint do_pick = (pivals.rndm_type == RNDM_PICK);
    gint do_hurl = (pivals.rndm_type == RNDM_HURL);
    gint do_slur = (pivals.rndm_type == RNDM_SLUR);

    gint do_time = (pivals.seed_type == SEED_TIME);
    gint do_user = (pivals.seed_type == SEED_USER);

    argc = 1;
    argv = g_new(gchar *, 1);
    argv[0] = g_strdup("randomize");

    gtk_init(&argc, &argv);
/*
 *  Open a new dialog, label it and set up its
 *  destroy callback.
 */
    dlg = gtk_dialog_new();
    gtk_window_set_title(GTK_WINDOW(dlg), RNDM_VERSION);
    gtk_window_position(GTK_WINDOW(dlg), GTK_WIN_POS_MOUSE);
    gtk_signal_connect(GTK_OBJECT(dlg), "destroy",
	(GtkSignalFunc) randomize_close_callback, NULL);
/*
 *  Action area OK & Cancel buttons
 */
    randomize_add_action_button("OK",
	(GtkSignalFunc) randomize_ok_callback, dlg);
    randomize_add_action_button("Cancel",
	(GtkSignalFunc) randomize_cancel_callback, dlg);
/*
 *  Parameter settings
 *
 *  First set up the basic containers, label them, etc.
 */
    frame = gtk_frame_new("Parameter Settings");
    gtk_frame_set_shadow_type(GTK_FRAME(frame), GTK_SHADOW_ETCHED_IN);
    gtk_container_border_width(GTK_CONTAINER(frame), 10);
    gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dlg)->vbox), frame, TRUE, TRUE, 0);
    table = gtk_table_new(4, 2, FALSE);
    gtk_container_border_width(GTK_CONTAINER(table), 10);
    gtk_container_add(GTK_CONTAINER(frame), table);
/*
 *  Randomization Type - label & radio buttons
 */
    label = gtk_label_new("Randomization Type:");
    gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.5);
    gtk_table_attach(GTK_TABLE(table), label, 0, 1, 0, 1,
	GTK_FILL | GTK_EXPAND, GTK_FILL, 5, 0);
    gtk_widget_show(label);

    toggle_hbox = gtk_hbox_new(FALSE, 5);
    gtk_container_border_width(GTK_CONTAINER(toggle_hbox), 5);
    gtk_table_attach(GTK_TABLE(table), toggle_hbox, 1, 2, 0, 1,
	GTK_FILL | GTK_EXPAND, GTK_FILL, 5, 0);
/*
 *  Blur button (won't work with indexed - if the drawable is indexed,
 *      don't allow blur as an option)
 */
    if (! is_indexed_drawable) {
	randomize_add_radio_button(type_group, "Blur", toggle_hbox,
	    (GtkSignalFunc) randomize_toggle_update, &do_blur);
    }
/*
 *  Hurl, Pick and Slur buttons
 */
    randomize_add_radio_button(type_group, "Hurl", toggle_hbox,
	(GtkSignalFunc) randomize_toggle_update, &do_hurl);
    randomize_add_radio_button(type_group, "Pick", toggle_hbox,
	(GtkSignalFunc) randomize_toggle_update, &do_pick);
    randomize_add_radio_button(type_group, "Slur", toggle_hbox,
	(GtkSignalFunc) randomize_toggle_update, &do_slur);

/*
 *  Randomization seed initialization controls
 */
    label = gtk_label_new("Seed");
    gtk_misc_set_alignment(GTK_MISC (label), 0.0, 0.5);
    gtk_table_attach(GTK_TABLE(table), label, 0, 1, 1, 2, GTK_FILL, 0, 5, 0);
    gtk_widget_show(label);
/*
 *  Box to hold seed initialization radio buttons
 */
    seed_vbox = gtk_vbox_new(FALSE, 2);
    gtk_container_border_width(GTK_CONTAINER(seed_vbox), 5);
    gtk_table_attach(GTK_TABLE(table), seed_vbox, 1, 2, 1, 2,
	GTK_FILL | GTK_EXPAND, GTK_FILL, 5, 0);
/*
 *  Time button
 */
    randomize_add_radio_button(seed_group, "Current Time", seed_vbox,
	(GtkSignalFunc) randomize_toggle_update, &do_time);
/*
 *  Box to hold seed user initialization controls
 */
    seed_hbox = gtk_hbox_new(FALSE, 3);
    gtk_container_border_width(GTK_CONTAINER(seed_hbox), 0);
    gtk_box_pack_start(GTK_BOX(seed_vbox), seed_hbox, FALSE, FALSE, 0);
/*
 *  User button
 */
    randomize_add_radio_button(seed_group, "User", seed_hbox,
	(GtkSignalFunc) randomize_toggle_update, &do_user);
/*
 *  Randomization seed text
 */
    entry = gtk_entry_new();
    gtk_widget_set_usize(entry, ENTRY_WIDTH, 0);
    gtk_box_pack_start(GTK_BOX(seed_hbox), entry, FALSE, FALSE, 0);
    sprintf(buffer, "%d", pivals.rndm_seed);
    gtk_entry_set_text(GTK_ENTRY(entry), buffer);
    gtk_signal_connect(GTK_OBJECT(entry), "changed",
	(GtkSignalFunc) randomize_text_update, &pivals.rndm_seed);
    gtk_widget_show(entry);
    gtk_widget_show(seed_hbox);

/*
 *  Randomization percentage label & scale (1 to 100)
 */
    label = gtk_label_new("Randomization %:");
    gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.5);
    gtk_table_attach(GTK_TABLE(table), label, 0, 1, 2, 3,
	GTK_FILL | GTK_EXPAND, GTK_FILL, 5, 0);

    scale_data = gtk_adjustment_new(pivals.rndm_pct,
	1.0, 100.0, 1.0, 1.0, 0.0);
    scale = gtk_hscale_new(GTK_ADJUSTMENT(scale_data));
    gtk_widget_set_usize(scale, SCALE_WIDTH, 0);
    gtk_table_attach(GTK_TABLE(table), scale, 1, 2, 2, 3,
	GTK_FILL | GTK_EXPAND, GTK_FILL, 0, 0);
    gtk_scale_set_value_pos(GTK_SCALE(scale), GTK_POS_TOP);
    gtk_scale_set_digits(GTK_SCALE(scale), 0);
    gtk_range_set_update_policy(GTK_RANGE(scale), GTK_UPDATE_DELAYED);
    gtk_signal_connect(GTK_OBJECT(scale_data), "value_changed",
	(GtkSignalFunc) randomize_scale_update, &pivals.rndm_pct);
    gtk_widget_show(label);
    gtk_widget_show(scale);
/*
 *  Repeat count label & scale (1 to 100)
 */
    label = gtk_label_new("Repeat");
    gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.5);
    gtk_table_attach(GTK_TABLE(table), label, 0, 1, 3, 4,
	GTK_FILL, 0, 5, 0);

    scale_data = gtk_adjustment_new(pivals.rndm_rcount,
	1.0, 100.0, 1.0, 1.0, 0.0);
    scale = gtk_hscale_new(GTK_ADJUSTMENT(scale_data));
    gtk_widget_set_usize(scale, SCALE_WIDTH, 0);
    gtk_table_attach(GTK_TABLE(table), scale, 1, 2, 3, 4,
	GTK_FILL | GTK_EXPAND, GTK_FILL, 0, 0);
    gtk_scale_set_value_pos(GTK_SCALE(scale), GTK_POS_TOP);
    gtk_scale_set_digits(GTK_SCALE(scale), 0);
    gtk_range_set_update_policy(GTK_RANGE(scale), GTK_UPDATE_DELAYED);
    gtk_signal_connect(GTK_OBJECT(scale_data), "value_changed",
	(GtkSignalFunc) randomize_scale_update, &pivals.rndm_rcount);
    gtk_widget_show(label);
    gtk_widget_show(scale);
/*
 *  Display everything.
 */
    gtk_widget_show(frame);
    gtk_widget_show(table);
    gtk_widget_show(dlg);

    gtk_main();
    gdk_flush();
/*
 *  Figure out which type of randomization to apply.
 */
    if (do_blur) {
	pivals.rndm_type = RNDM_BLUR;
    } else if (do_pick) {
	pivals.rndm_type = RNDM_PICK;
    } else if (do_slur) {
	pivals.rndm_type = RNDM_SLUR;
    } else {
	pivals.rndm_type = RNDM_HURL;
    }
/*
 *  Figure out which type of seed initialization to apply.
 */
    if (do_time) {
	pivals.seed_type = SEED_TIME;
    } else {
	pivals.seed_type = SEED_USER;
    }

    return rndm_int.run;
}


/*********************************
 *
 *  GUI accessories (callbacks, etc)
 *
 ********************************/

static void
randomize_close_callback(GtkWidget *widget, gpointer data) {
    gtk_main_quit();
}

static void
randomize_ok_callback(GtkWidget *widget, gpointer data) {
    rndm_int.run = TRUE;
    gtk_widget_destroy(GTK_WIDGET(data));
}

static void
randomize_cancel_callback(GtkWidget *widget, gpointer data) {
    gtk_widget_destroy(GTK_WIDGET(data));
}


static void
randomize_scale_update(GtkAdjustment *adjustment, double *scale_val) {
    *scale_val = adjustment->value;
}

static void
randomize_toggle_update(GtkWidget *widget, gpointer data) {
    int *toggle_val;

    toggle_val = (int *) data;

    if (GTK_TOGGLE_BUTTON(widget)->active)
      *toggle_val = TRUE;
    else
      *toggle_val = FALSE;
}

static void
randomize_text_update(GtkWidget *widget, gpointer data) {
  gint *text_val;

  text_val = (gint *) data;

  *text_val = atoi(gtk_entry_get_text(GTK_ENTRY(widget)));
}
