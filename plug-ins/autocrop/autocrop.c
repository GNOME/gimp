/*
 * Autocrop plug-in version 1.00
 * by Tim Newsome <drz@froody.bloke.com>
 * thanks to quartic for finding a nasty bug for me
 */

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
static int colors_equal(guchar *col1, gchar *col2, int bytes);
static int guess_bgcolor(GPixelRgn *pr, int width, int height, int bytes,
		guchar *color);

static void doit(GDrawable *drawable, gint32);

GPlugInInfo PLUG_IN_INFO =
{
	NULL,													/* init_proc */
	NULL,													/* quit_proc */
	query,												/* query_proc */
	run,													/* run_proc */
};

gint bytes;
gint sx1, sy1, sx2, sy2;
int run_flag = 0;

MAIN()

static void query()
{
	static GParamDef args[] =
	{
		{PARAM_INT32, "run_mode", "Interactive, non-interactive"},
		{PARAM_IMAGE, "image", "Input image"},
		{PARAM_DRAWABLE, "drawable", "Input drawable"},
	};
	static GParamDef *return_vals = NULL;
	static int nargs = sizeof(args) / sizeof(args[0]);
	static int nreturn_vals = 0;

	gimp_install_procedure("plug_in_autocrop",
			       "Automagically crops a picture.",
			       "",
			       "Tim Newsome",
			       "Tim Newsome",
			       "1997",
			       "<Image>/Image/Transforms/Autocrop",
			       "RGB*, GRAY*, INDEXED*",
			       PROC_PLUG_IN,
			       nargs, nreturn_vals,
			       args, return_vals);
}

static void run(char *name, int n_params, GParam * param, int *nreturn_vals,
								GParam ** return_vals)
{
	static GParam values[1];
	GDrawable *drawable;
	GRunModeType run_mode;
	GStatusType status = STATUS_SUCCESS;
	gint32 image_id;

	*nreturn_vals = 1;
	*return_vals = values;

	run_mode = param[0].data.d_int32;

	if (run_mode == RUN_NONINTERACTIVE) {
		if (n_params != 3) {
			status = STATUS_CALLING_ERROR;
		}
	}

	if (status == STATUS_SUCCESS) {
		/*  Get the specified drawable  */
		drawable = gimp_drawable_get(param[2].data.d_drawable);
		image_id = param[1].data.d_image;

		/*  Make sure that the drawable is gray or RGB color  */
		if (gimp_drawable_color(drawable->id) ||
		    gimp_drawable_gray(drawable->id) ||
		    gimp_drawable_indexed(drawable->id)) {
			gimp_progress_init("Cropping...");
			gimp_tile_cache_ntiles(2 * (drawable->width / gimp_tile_width() + 1));

			srand(time(NULL));
			doit(drawable, image_id);

			if (run_mode != RUN_NONINTERACTIVE)
			        gimp_displays_flush();

			/* if (run_mode == RUN_INTERACTIVE)
				gimp_set_data("plug_in_autocrop", &my_config, sizeof(my_config)); */
		} else {
			status = STATUS_EXECUTION_ERROR;
		}
		/*gimp_drawable_detach(drawable);*/
	}

	values[0].type = PARAM_STATUS;
	values[0].data.d_status = status;
}

static void doit(GDrawable *drawable, gint32 image_id)
{
	GPixelRgn srcPR;
	gint width, height;
	int x, y, abort;
	gint32 nx, ny, nw, nh;
	guchar *buffer;
	guchar color[4] = {0, 0, 0, 0};
	int nreturn_vals;

	width = drawable->width;
	height = drawable->height;
	bytes = drawable->bpp;

	nx = 0;
	ny = 0;
	nw = width;
	nh = height;

	/*  initialize the pixel regions  */
	gimp_pixel_rgn_init(&srcPR, drawable, 0, 0, width, height, FALSE, FALSE);

	/* First, let's figure out what exactly to crop. */
	buffer = malloc((width > height ? width : height) * bytes);

	guess_bgcolor(&srcPR, width, height, bytes, color);

	/* Check how many of the top lines are uniform. */
	abort = 0;
	for (y = 0; y < height && !abort; y++) {
		gimp_pixel_rgn_get_row(&srcPR, buffer, 0, y, width);
		for (x = 0; x < width && !abort; x++) {
			abort = !colors_equal(color, buffer + x * bytes, bytes);
		}
	}
	if (y == height) {
		free(buffer);
	}
	y--;
	ny = y;
	nh = height - y;

	gimp_progress_update(.25);

	/* Check how many of the bottom lines are uniform. */
	abort = 0;
	for (y = height - 1; y >= 0 && !abort; y--) {
		gimp_pixel_rgn_get_row(&srcPR, buffer, 0, y, width);
		for (x = 0; x < width && !abort; x++) {
			abort = !colors_equal(color, buffer + x * bytes, bytes);
		}
	}
	nh = y - ny + 2;

	gimp_progress_update(.5);

	/* Check how many of the left lines are uniform. */
	abort = 0;
	for (x = 0; x < width && !abort; x++) {
		gimp_pixel_rgn_get_col(&srcPR, buffer, x, ny, nh);
		for (y = 0; y < nh && !abort; y++) {
			abort = !colors_equal(color, buffer + y * bytes, bytes);
		}
	}
	x--;
	nx = x;
	nw = width - x;

	gimp_progress_update(.75);

	/* Check how many of the right lines are uniform. */
	abort = 0;
	for (x = width - 1; x >= 0 && !abort; x--) {
		gimp_pixel_rgn_get_col(&srcPR, buffer, x, ny, nh);
		for (y = 0; y < nh && !abort; y++) {
			abort = !colors_equal(color, buffer + y * bytes, bytes);
		}
	}
	nw = x - nx + 2;

	free(buffer);

	gimp_drawable_detach(drawable);
	if (nw != width || nh != height) {
		gimp_run_procedure("gimp_crop", &nreturn_vals,
				PARAM_IMAGE, image_id,
				PARAM_INT32, nw,
				PARAM_INT32, nh,
				PARAM_INT32, nx,
				PARAM_INT32, ny,
				PARAM_END);
	}

	/*  update the timred region  */
	/*gimp_drawable_merge_shadow(drawable->id, TRUE);*/
	gimp_displays_flush();
}

static int guess_bgcolor(GPixelRgn *pr, int width, int height, int bytes,
		guchar *color) {
	guchar tl[4], tr[4], bl[4], br[4];

	gimp_pixel_rgn_get_pixel(pr, tl, 0, 0);
	gimp_pixel_rgn_get_pixel(pr, tr, width - 1, 0);
	gimp_pixel_rgn_get_pixel(pr, bl, 0, height - 1);
	gimp_pixel_rgn_get_pixel(pr, br, width - 1, height - 1);

	/* Algorithm pinched from pnmcrop.
	 * To guess the background, first see if 3 corners are equal.
	 * Then if two are equal.
	 * Otherwise average the colors.
	 */

	if (colors_equal(tr, bl, bytes) && colors_equal(tr, br, bytes)) {
		memcpy(color, tr, bytes);
		return 3;
	} else if (colors_equal(tl, bl, bytes) && colors_equal(tl, br, bytes)) {
		memcpy(color, tl, bytes);
		return 3;
	} else if (colors_equal(tl, tr, bytes) && colors_equal(tl, br, bytes)) {
		memcpy(color, tl, bytes);
		return 3;
	} else if (colors_equal(tl, tr, bytes) && colors_equal(tl, bl, bytes)) {
		memcpy(color, tl, bytes);
		return 3;

	} else if (colors_equal(tl, tr, bytes) || colors_equal(tl, bl, bytes) ||
			colors_equal(tl, br, bytes)) {
		memcpy(color, tl, bytes);
		return 2;
	} else if (colors_equal(tr, bl, bytes) || colors_equal(tr, bl, bytes)) {
		memcpy(color, tr, bytes);
		return 2;
	} else if (colors_equal(br, bl, bytes)) {
		memcpy(color, br, bytes);
		return 2;
	} else {
		while (bytes--) {
			color[bytes] = (tl[bytes] + tr[bytes] + bl[bytes] + br[bytes]) / 4;
		}
		return 0;
	}
}

static int colors_equal(guchar *col1, gchar *col2, int bytes) {
	int equal = 1;
	int b;

	for (b = 0; b < bytes; b++) {
		if (col1[b] != col2[b]) {
			equal = 0;
			break;
		}
	}

	return equal;
}
