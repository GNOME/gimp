/****************************************************************************
 * This is a plugin for the GIMP v 0.99.8 or later.  Documentation is
 * available at http://www.rru.com/~meo/gimp/ .
 *
 * Copyright (C) 1997-98 Miles O'Neal  <meo@rru.com>  http://www.rru.com/~meo/
 * Blur code Copyright (C) 1995 Spencer Kimball and Peter Mattis
 * GUI based on GTK code from:
 *    alienmap (Copyright (C) 1996, 1997 Daniel Cotting)
 *    plasma   (Copyright (C) 1996 Stephen Norris),
 *    oilify   (Copyright (C) 1996 Torsten Martinsen),
 *    ripple   (Copyright (C) 1997 Brian Degenhardt) and
 *    whirl    (Copyright (C) 1997 Federico Mena Quintero).
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
 ****************************************************************************/

/****************************************************************************
 * Blur:
 *
 * blur version 2.0 (1 May 1998, MEO)
 * history
 *     2.0 -  1 May 1998 MEO
 *         based on randomize 1.7
 *
 * Please send any patches or suggestions to the author: meo@rru.com .
 * 
 * Blur applies a 3x3 blurring convolution kernel to the specified drawable.
 * 
 * For each pixel in the selection or image,
 * whether to change the pixel is decided by picking a
 * random number, weighted by the user's "randomization" percentage.
 * If the random number is in range, the pixel is modified.  For
 * blurring, an average is determined from the current and adjacent
 * pixels. *(Except for the random factor, the blur code came
 * straight from the original S&P blur plug-in.)*
 * 
 * This works only with RGB and grayscale images.
 * 
 ****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "libgimp/gimp.h"
#include "gtk/gtk.h"
#include <plug-ins/gpc/gpc.h>

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

/*********************************
 *
 *  PLUGIN-SPECIFIC CONSTANTS
 *
 ********************************/


/*
 *  progress meter update frequency
 */
#define PROG_UPDATE_TIME ((row % 10) == 0)

#define PLUG_IN_NAME "plug_in_blur"
#define BLUR_VERSION "Blur 2.0"

#define SEED_TIME 10
#define SEED_USER 11

#define ENTRY_WIDTH 75
#define SCALE_WIDTH 100

/*********************************
 *
 *  PLUGIN-SPECIFIC STRUCTURES AND DATA
 *
 ********************************/

typedef struct {
    gdouble blur_pct;     /* likelihood of randomization (as %age) */
    gdouble blur_rcount;  /* repeat count */
    gint seed_type;       /* seed init. type - current time or user value */
    gint blur_seed;       /* seed value for rand() function */
} BlurVals;

static BlurVals pivals = {
    50.0,
    1.0,
    SEED_TIME,
    0,
};

typedef struct {
    gint run;
} BlurInterface;

static BlurInterface blur_int = {
    FALSE     /*  have we run? */
};


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

static void blur(GDrawable *drawable);

static inline void blur_prepare_row(
    GPixelRgn *pixel_rgn,
    guchar *data,
    int x,
    int y,
    int w
);

static gint blur_dialog();

static void blur_ok_callback(
    GtkWidget *widget,
    gpointer data
);


/************************************ Guts ***********************************/

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
    static GParamDef args_ni[] = {
        { PARAM_INT32, "run_mode", "non-interactive" },
        { PARAM_IMAGE, "image", "Input image (unused)" },
        { PARAM_DRAWABLE, "drawable", "Input drawable" },
    };
    static int nargs_ni = sizeof(args_ni) / sizeof (args_ni[0]);

    static GParamDef args[] = {
        { PARAM_INT32, "run_mode", "Interactive, non-interactive" },
        { PARAM_IMAGE, "image", "Input image (unused)" },
        { PARAM_DRAWABLE, "drawable", "Input drawable" },
        { PARAM_FLOAT, "blur_pct", "Randomization percentage (1 - 100)" },
        { PARAM_FLOAT, "blur_rcount", "Repeat count(1 - 100)" },
        { PARAM_INT32, "seed_type", "Seed type (10 = current time, 11 = seed value)" },
        { PARAM_INT32, "blur_seed", "Seed value (used only if seed type is 11)" },
    };
    static int nargs = sizeof(args) / sizeof (args[0]);

    static GParamDef *return_vals = NULL;
    static int nreturn_vals = 0;

    const char *blurb = "Apply a 3x3 blurring convolution kernel to the specified drawable.";
    const char *help = "This plug-in randomly blurs the specified drawable, using a 3x3 blur.  You control the percentage of the pixels that are blurred and the number of times blurring is applied.  Indexed images are not supported.";
    const char *author = "Miles O'Neal  <meo@rru.com>  http://www.rru.com/~meo/";
    const char *copyrights = "Miles O'Neal, Spencer Kimball, Peter Mattis, Torsten Martinsen, Brian Degenhardt, Federico Mena Quintero, Stephen Norris, Daniel Cotting";
    const char *copyright_date = "1995-1998";

    gimp_install_procedure("plug_in_blur_randomize",
        (char *) blurb,
        (char *) help,
        (char *) author,
        (char *) copyrights,
        (char *) copyright_date,
        "<Image>/Filters/Blur/Blur",
        "RGB*, GRAY*",
        PROC_PLUG_IN,
        nargs, nreturn_vals,
        args, return_vals);

    gimp_install_procedure(PLUG_IN_NAME,
        (char *) blurb,
        (char *) help,
        (char *) author,
        (char *) copyrights,
        (char *) copyright_date,
        NULL,
        "RGB*, GRAY*",
        PROC_PLUG_IN,
        nargs_ni, nreturn_vals,
        args_ni, return_vals);
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

    GDrawable *drawable;
    GRunModeType run_mode;
    GStatusType status = STATUS_SUCCESS;        /* assume the best! */
    char prog_label[32];
    static GParam values[1];
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
      gimp_drawable_gray(drawable->id)) {

        switch (run_mode) {
/*
 *  If we're running interactively, pop up the dialog box.
 */
            case RUN_INTERACTIVE:
                gimp_get_data(PLUG_IN_NAME, &pivals);
                if (!blur_dialog())        /* return on Cancel */
                    return;
                break;
/*
 *  If we're not interactive (probably scripting), we
 *  get the parameters from the param[] array, since
 *  we don't use the dialog box.  Make sure all
 *  parameters have legitimate values.
 */
            case RUN_NONINTERACTIVE:
                if ((strcmp(name, "plug_in_blur_randomize") == 0) &&
                  (nparams == 7)) {
                    pivals.blur_pct = (gdouble)param[3].data.d_float;
                    pivals.blur_pct = (gdouble)MIN(100.0, pivals.blur_pct);
                    pivals.blur_pct = (gdouble)MAX(1.0, pivals.blur_pct);
                    pivals.blur_rcount = (gdouble)param[4].data.d_float;
                    pivals.blur_rcount = (gdouble)MIN(100.0,pivals.blur_rcount);
                    pivals.blur_rcount = (gdouble)MAX(1.0, pivals.blur_rcount);
                    pivals.seed_type = (gint) param[5].data.d_int32;
                    pivals.seed_type = (gint) MIN(SEED_USER, param[5].data.d_int32);
                    pivals.seed_type = (gint) MAX(SEED_TIME, param[5].data.d_int32);
                    pivals.blur_seed = (gint) param[6].data.d_int32;
                    status = STATUS_SUCCESS;
                } else if ((strcmp(name, PLUG_IN_NAME) == 0) &&
                  (nparams == 3)) {
                    pivals.blur_pct = (gdouble) 100.0;
                    pivals.blur_rcount = (gdouble) 1.0;
                    pivals.seed_type = SEED_TIME;
                    pivals.blur_seed = 0;
                    status = STATUS_SUCCESS;
                } else {
                    status = STATUS_CALLING_ERROR;
                }
                break;
/*
 *  If we're running with the last set of values, get those values.
 */
            case RUN_WITH_LAST_VALS:
                gimp_get_data(PLUG_IN_NAME, &pivals);
                break;
/*
 *  Hopefully we never get here!
 */
            default:
                break;
        }
        if (status == STATUS_SUCCESS) {
/*
 *  JUST DO IT!
 */
            strcpy(prog_label, BLUR_VERSION);
            gimp_progress_init(prog_label);
            gimp_tile_cache_ntiles(2 * (drawable->width / gimp_tile_width() + 1));
/*
 *  Initialize the rand() function seed
 */
            if (pivals.seed_type == SEED_TIME)
                srand(time(NULL));
            else
                srand(pivals.blur_seed);

            blur(drawable);
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
                gimp_set_data(PLUG_IN_NAME, &pivals, sizeof(BlurVals));
            }
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
 *  blur_prepare_row()
 *
 *  Get a row of pixels.  If the requested row
 *  is off the edge, clone the edge row.
 *
 ********************************/

static inline void
blur_prepare_row(GPixelRgn *pixel_rgn, guchar *data, int x, int y, int w)
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
        data[-(gint)pixel_rgn->bpp + b] = data[b];
        data[w * pixel_rgn->bpp + b] = data[(w - 1) * pixel_rgn->bpp + b];
    }
}

/*********************************
 *
 *  blur()
 *
 *  Actually mess with the image.
 *
 ********************************/

static void
blur(GDrawable *drawable)
{
    GPixelRgn srcPR, destPR, destPR2, *sp, *dp, *tp;
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
    gint has_alpha, ind;

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
 *  as the size of the output image.  Also get alpha info.
 */
    width = drawable->width;
    height = drawable->height;
    bytes = drawable->bpp;
    has_alpha = gimp_drawable_has_alpha(drawable->id);
/*
 *  allocate row buffers
 */
    prev_row = (guchar *) malloc((x2 - x1 + 2) * bytes);
    cur_row = (guchar *) malloc((x2 - x1 + 2) * bytes);
    next_row = (guchar *) malloc((x2 - x1 + 2) * bytes);
    dest = (guchar *) malloc((x2 - x1) * bytes);

/*
 *  initialize the pixel regions
 */
    gimp_pixel_rgn_init(&srcPR, drawable, 0, 0, width, height, FALSE, FALSE);
    gimp_pixel_rgn_init(&destPR, drawable, 0, 0, width, height, TRUE, TRUE);
    gimp_pixel_rgn_init(&destPR2, drawable, 0, 0, width, height, TRUE, TRUE);
    sp = &srcPR;
    dp = &destPR;
    tp = NULL;

    pr = prev_row + bytes;
    cr = cur_row + bytes;
    nr = next_row + bytes;

    for (cnt = 1; cnt <= pivals.blur_rcount; cnt++) {
/*
 *  prepare the first row and previous row
 */
        blur_prepare_row(sp, pr, x1, y1 - 1, (x2 - x1));
        blur_prepare_row(dp, cr, x1, y1, (x2 - x1));
/*
 *  loop through the rows, applying the selected convolution
 */
        for (row = y1; row < y2; row++) {
            /*  prepare the next row  */
            blur_prepare_row(sp, nr, x1, row + 1, (x2 - x1));

            d = dest;
            ind = 0;
            for (col = 0; col < (x2 - x1) * bytes; col++) {
                if (((rand() % 100)) <= (gint) pivals.blur_pct) {
                    ind++;
                    if (ind==bytes || !(has_alpha)) {
/*
 *  If no alpha channel, or if there is one and this is it...
 */
                        *d++ = ((gint) pr[col - bytes] + (gint) pr[col] +
                                (gint) pr[col + bytes] +
                            (gint) cr[col - bytes] + (gint) cr[col] +
                                (gint) cr[col + bytes] +
                            (gint) nr[col - bytes] + (gint) nr[col] +
                                (gint) nr[col + bytes]) / 9;
                        ind = 0;
                    } else {
/*
 *  otherwise we have an alpha channel, but this is a color channel
 */
                        *d++ = ((gint) (((gdouble) (pr[col - bytes] * pr[col - ind]) 
                             + (gdouble) (pr[col] * pr[col + bytes - ind])
                             + (gdouble) (pr[col + bytes] * pr[col + 2*bytes - ind])
                             + (gdouble) (cr[col - bytes] * cr[col - ind])
                             + (gdouble) (cr[col] * cr[col + bytes - ind])
                             + (gdouble) (cr[col + bytes] * cr[col + 2*bytes - ind])
                             + (gdouble) (nr[col - bytes] * nr[col - ind])
                             + (gdouble) (nr[col] * nr[col + bytes - ind]) 
                             + (gdouble) (nr[col + bytes] * nr[col + 2*bytes - ind]))
                             / ((gdouble) pr[col - ind]
                             + (gdouble) pr[col + bytes - ind]
                             + (gdouble) pr[col + 2*bytes - ind]
                             + (gdouble) cr[col - ind]
                             + (gdouble) cr[col + bytes - ind]
                             + (gdouble) cr[col + 2*bytes - ind]
                             + (gdouble) nr[col - ind]
                             + (gdouble) nr[col + bytes - ind]
                             + (gdouble) nr[col + 2*bytes - ind])));
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
            gimp_pixel_rgn_set_row(dp, dest, x1, row, (x2 - x1));

            tmp = pr;
            pr = cr;
            cr = nr;
            nr = tmp;

            if (PROG_UPDATE_TIME)
                gimp_progress_update((double) row / (double) (y2 - y1));
        }
/*
 *  if we have more cycles to perform, swap the src and dest Pixel Regions
 */
        if (cnt < pivals.blur_rcount) {
            if (tp != NULL) {
                tp = dp;
                dp = sp;
                sp = tp;
            } else {
                tp = &srcPR;
                sp = &destPR;
                dp = &destPR2;
            }
        }
    }
    gimp_progress_update((double) 100);
/*
 *  update the blurred region
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


/*********************************
 *
 *  blur_dialog() - set up the plug-in's dialog box
 *
 ********************************/

static gint
blur_dialog()
{
    GtkWidget *dlg, *entry, *frame,
        *seed_hbox, *seed_vbox, *table;
    GSList *seed_group = NULL;
    gchar **argv;
    gint argc;
    gchar buffer[10];
/*
 *  various initializations
 */
    gint do_time = (pivals.seed_type == SEED_TIME);
    gint do_user = (pivals.seed_type == SEED_USER);

    argc = 1;
    argv = g_new(gchar *, 1);
    argv[0] = g_strdup("blur");

    gtk_init(&argc, &argv);
    gtk_rc_parse(gimp_gtkrc());
/*
 *  Open a new dialog, label it and set up its
 *  destroy callback.
 */
    dlg = gtk_dialog_new();
    gtk_window_set_title(GTK_WINDOW(dlg), BLUR_VERSION);
    gtk_window_position(GTK_WINDOW(dlg), GTK_WIN_POS_MOUSE);
    gtk_signal_connect(GTK_OBJECT(dlg), "destroy",
        (GtkSignalFunc) gpc_close_callback, NULL);
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
    gtk_widget_show(table);
    gpc_setup_tooltips(table);
/*
 *  Action area OK & Cancel buttons
 */
    gpc_add_action_button("OK", (GtkSignalFunc) blur_ok_callback, dlg,
        "Accept settings and apply filter to image");
    gpc_add_action_button("Cancel", (GtkSignalFunc) gpc_cancel_callback, dlg,
        "Close plug-in without making any changes");
/*
 *  Randomization seed initialization controls
 */
    gpc_add_label("Randomization Seed:", table, 0, 1, 1, 2);
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
    gpc_add_radio_button(&seed_group, "Current Time", seed_vbox, &do_time,
        "Seed random number generator from the current time - this guarantees a reasonable randomization");
/*
 *  Box to hold seed user initialization controls
 */
    seed_hbox = gtk_hbox_new(FALSE, 3);
    gtk_container_border_width(GTK_CONTAINER(seed_hbox), 0);
    gtk_box_pack_start(GTK_BOX(seed_vbox), seed_hbox, FALSE, FALSE, 0);
/*
 *  User button
 */
    gpc_add_radio_button(&seed_group, "Other Value", seed_hbox, &do_user,
        "Enable user-entered value for random number generator seed - this allows you to repeat a given \"random\" operation");
/*
 *  Randomization seed number (text)
 */
    entry = gtk_entry_new();
    gtk_widget_set_usize(entry, ENTRY_WIDTH, 0);
    gtk_box_pack_start(GTK_BOX(seed_hbox), entry, FALSE, FALSE, 0);
    sprintf(buffer, "%d", pivals.blur_seed);
    gtk_entry_set_text(GTK_ENTRY(entry), buffer);
    gtk_signal_connect(GTK_OBJECT(entry), "changed",
        (GtkSignalFunc) gpc_text_update, &pivals.blur_seed);
    gtk_widget_show(entry);
    gpc_set_tooltip(entry, "Value for seeding the random number generator");
    gtk_widget_show(seed_hbox);
/*
 *  Randomization percentage label & scale (1 to 100)
 */
    gpc_add_label("Randomization %:", table, 0, 1, 2, 3);
    gpc_add_hscale(table, SCALE_WIDTH,
        1.0, 100.0, &pivals.blur_pct, 1, 2, 2, 3,
        "Percentage of pixels to be filtered");

/*
 *  Repeat count label & scale (1 to 100)
 */
    gpc_add_label("Repeat:", table, 0, 1, 3, 4);
    gpc_add_hscale(table, SCALE_WIDTH,
        1.0, 100.0, &pivals.blur_rcount, 1, 2, 3, 4,
        "Number of times to apply filter");

/*
 *  Display everything.
 */
    gtk_widget_show(frame);
    gtk_widget_show(dlg);

    gtk_main();
    gdk_flush();
/*
 *  Figure out which type of seed initialization to apply.
 */
    if (do_time) {
        pivals.seed_type = SEED_TIME;
    } else {
        pivals.seed_type = SEED_USER;
    }

    return blur_int.run;
}


static void
blur_ok_callback(GtkWidget *widget, gpointer data) {
    blur_int.run = TRUE;
    gtk_widget_destroy(GTK_WIDGET(data));
}
