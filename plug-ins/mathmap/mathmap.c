/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * MathMap plug-in --- generate an image by means of a mathematical expression
 * Copyright (C) 1997 Mark Probst
 * schani@unix.cslab.tuwien.ac.at
 *
 * Plug-In structure based on:
 *   Whirl plug-in --- distort an image into a whirlpool
 *   Copyright (C) 1997 Federico Mena Quintero
 *   federico@nuclecu.unam.mx
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
 */


#include <math.h>
#include <stdlib.h>
#include <stdio.h>

#include <gtk/gtk.h>
#include <libgimp/gimp.h>

#include "exprtree.h"
#include "builtins.h"
#include "postfix.h"

/***** Macros *****/

#define MIN(a, b) (((a) < (b)) ? (a) : (b))
#define MAX(a, b) (((a) > (b)) ? (a) : (b))


/***** Magic numbers *****/

#define PREVIEW_SIZE 128
#define SCALE_WIDTH  200
#define ENTRY_WIDTH  60

#define DEFAULT_EXPRESSION      "origValXY(x+sin(y*10)*3,y+sin(x*10)*3)"

#define FLAG_INTERSAMPLING      1
#define FLAG_OVERSAMPLING       2

#define MAX_EXPRESSION_LENGTH   1024

/***** Types *****/

typedef struct {
    gchar expression[MAX_EXPRESSION_LENGTH];
    gint flags;
} mathmap_vals_t;

typedef struct {
	GtkWidget *preview;
	guchar    *image;
	guchar    *wimage;

	gint run;
} mathmap_interface_t;


/***** Prototypes *****/

static void query(void);
static void run(char    *name,
		int      nparams,
		GParam  *param,
		int     *nreturn_vals,
		GParam **return_vals);

static void expression_copy (gchar *dest, gchar *src);

static void mathmap (void);
void   mathmap_get_pixel(int x, int y, guchar *pixel);

static void build_preview_source_image(void);

static gint mathmap_dialog(void);
static void dialog_update_preview(void);
static void dialog_create_value(char *title, GtkTable *table, int row, gdouble *value,
				double left, double right);
static void dialog_scale_update(GtkAdjustment *adjustment, gdouble *value);
static void dialog_entry_update(GtkWidget *widget, gdouble *value);
static void dialog_intersampling_update (GtkWidget *widget, gpointer data);
static void dialog_oversampling_update (GtkWidget *widget, gpointer data);
static void dialog_preview_callback (GtkWidget *widget, gpointer data);
static void dialog_example_callback (GtkWidget *widget, char *expression);
static void dialog_close_callback(GtkWidget *widget, gpointer data);
static void dialog_ok_callback(GtkWidget *widget, gpointer data);
static void dialog_cancel_callback(GtkWidget *widget, gpointer data);

/***** Variables *****/

GPlugInInfo PLUG_IN_INFO = {
	NULL,   /* init_proc */
	NULL,   /* quit_proc */
	query,  /* query_proc */
	run     /* run_proc */
}; /* PLUG_IN_INFO */


static mathmap_vals_t mmvals = {
	DEFAULT_EXPRESSION,      /* expression */
	FLAG_INTERSAMPLING       /* flags */
}; /* mmvals */

static mathmap_interface_t wint = {
	NULL,  /* preview */
	NULL,  /* image */
	NULL,  /* wimage */
	FALSE  /* run */
}; /* wint */

static GDrawable *drawable;

static gint   tile_width, tile_height;
static gint   img_width, img_height, img_bpp;
static gint   sel_x1, sel_y1, sel_x2, sel_y2;
static gint   sel_width, sel_height;
static gint   preview_width, preview_height;
static GTile *the_tile = NULL;

static double cen_x, cen_y;
static double scale_x, scale_y;
static double radius, radius2;

GtkWidget *expressionEntry = 0;

exprtree *theExprtree = 0;
int imageWidth,
    imageHeight,
    wholeImageWidth,
    wholeImageHeight,
    originX,
    originY;
int usesRA = 0;
double currentX,
    currentY,
    currentR,
    currentA,
    imageR,
    imageX,
    imageY,
    middleX,
    middleY;
int intersamplingEnabled,
    oversamplingEnabled;

char *examples[][2] = {
    { "wave", "origValXY(x+sin(y*10)*3,y+sin(x*10)*3)" },
    { "square", "origValXY(sign(x)*x*x/50,sign(y)*y*y/50)" },
    { "mosaic", "origValXY(x-x%5,y-y%5)" },
    { "slice", "origValXY(x+5*sign(cos(9*y)),y+5*sign(cos(9*x)))" },
    { "mercator", "origValXY(x*cos(90/Y*y),y)" },
    { "pond", "origValRA(r+sin(r*30)*3,a)" },
    { "enhanced pond", "origValRA(r+(sin(500000/(r+100))*7),a)" },
    { "twirl 90", "origValRA(r,a+(r/R-1)*45)" },
    { "sphere", "origValRA(r*(1-inintv(r/(X*2),-0.5,0.5))+X/90*asin(inintv(r/(X*2),-0.5,0.5)*r/X),a)" },
    { "jitter", "origValRA(r,a+a%8-4)" },
    { "radial mosaic", "origValRA(r-r%5,a-a%5)" },
    { "circular slice", "origValRA(r,a+(r%5)-2)" },
    { "fisheye", "origValRA(r*r/R,a)" },
    { "center shake", "origValXY(x+max(0,cos(x*2))*5*max(0,cos(y*2))*cos(y*10),y+max(0,cos(x*2))*5*max(0,cos(y*2))*cos(x*10))" },
    { "scatter", "origValXY(x+rand(-3,3),y+rand(-3,3))" },
    { "?", "origValRA(r,a+sin(a*10)*20)" },
    { "?", "origValRA(r+r%20,a)" },
    { "sine wave", "grayColor(sin(r*10)*0.5+0.5)" },
    { "grid", "grayColor(if((x%20)*(y%20),1,0))" },
    { 0, 0 }
};


/***** Functions *****/

/*****/

static void
expression_copy (gchar *dest, gchar *src)
{
    strncpy(dest, src, MAX_EXPRESSION_LENGTH);
    dest[MAX_EXPRESSION_LENGTH - 1] = 0;
}


/*****/

MAIN();


/*****/

static void
query(void)
{
    static GParamDef args[] = {
	{ PARAM_INT32,    "run_mode",   "Interactive, non-interactive" },
	{ PARAM_IMAGE,    "image",      "Input image" },
	{ PARAM_DRAWABLE, "drawable",   "Input drawable" },
	{ PARAM_STRING,   "expression", "MathMap expression" },
	{ PARAM_INT32,    "flags",      "1: Intersampling 2: Oversampling" }
    }; /* args */

    static GParamDef *return_vals  = NULL;
    static int        nargs        = sizeof(args) / sizeof(args[0]);
    static int        nreturn_vals = 0;

    gimp_install_procedure("plug_in_mathmap",
			   "Generate an image using a mathematical expression.",
			   "Generates an image by means of a mathematical expression. The expression "
			   "can also refer to the data of an original image. Thus, arbitrary "
			   "distortions can be constructed.",
			   "Mark Probst",
			   "Mark Probst",
			   "October 1997, 0.1",
			   "<Image>/Filters/Distorts/MathMap",
			   "RGB",
			   PROC_PLUG_IN,
			   nargs,
			   nreturn_vals,
			   args,
			   return_vals);
} /* query */


/*****/

static void
run(char    *name,
    int      nparams,
    GParam  *param,
    int     *nreturn_vals,
    GParam **return_vals)
{
    static GParam values[1];

    GRunModeType  run_mode;
    GStatusType   status;
    double        xhsiz, yhsiz;
    int           pwidth, pheight;

    status   = STATUS_SUCCESS;
    run_mode = param[0].data.d_int32;

    values[0].type          = PARAM_STATUS;
    values[0].data.d_status = status;

    *nreturn_vals = 1;
    *return_vals  = values;

	/* Get the active drawable info */

    drawable = gimp_drawable_get(param[2].data.d_drawable);

    tile_width  = gimp_tile_width();
    tile_height = gimp_tile_height();

    img_width  = gimp_drawable_width(drawable->id);
    img_height = gimp_drawable_height(drawable->id);
    img_bpp    = gimp_drawable_bpp(drawable->id);

    gimp_drawable_mask_bounds(drawable->id, &sel_x1, &sel_y1, &sel_x2, &sel_y2);

    originX = sel_x1;
    originY = sel_y1;

    sel_width  = sel_x2 - sel_x1;
    sel_height = sel_y2 - sel_y1;

    cen_x = (double) (sel_x2 - 1 + sel_x1) / 2.0;
    cen_y = (double) (sel_y2 - 1 + sel_y1) / 2.0;

    xhsiz = (double) (sel_width - 1) / 2.0;
    yhsiz = (double) (sel_height - 1) / 2.0;

    if (xhsiz < yhsiz) {
	scale_x = yhsiz / xhsiz;
	scale_y = 1.0;
    } else if (xhsiz > yhsiz) {
	scale_x = 1.0;
	scale_y = xhsiz / yhsiz;
    } else {
	scale_x = 1.0;
	scale_y = 1.0;
    } /* else */

    radius  = MAX(xhsiz, yhsiz);
    radius2 = radius * radius;

    /* Calculate preview size */

    if (sel_width > sel_height) {
	pwidth  = MIN(sel_width, PREVIEW_SIZE);
	pheight = sel_height * pwidth / sel_width;
    } else {
	pheight = MIN(sel_height, PREVIEW_SIZE);
	pwidth  = sel_width * pheight / sel_height;
    } /* else */

    preview_width  = MAX(pwidth, 2);  /* Min size is 2 */
    preview_height = MAX(pheight, 2);

    /* See how we will run */

    switch (run_mode) {
	case RUN_INTERACTIVE:
	    /* Possibly retrieve data */

	    gimp_get_data("plug_in_mathmap", &mmvals);

	    /* Get information from the dialog */

	    if (!mathmap_dialog())
		return;

	    break;

	case RUN_NONINTERACTIVE:
	    /* Make sure all the arguments are present */

	    if (nparams != 5)
		status = STATUS_CALLING_ERROR;

	    if (status == STATUS_SUCCESS)
	    {
		expression_copy(mmvals.expression, param[3].data.d_string);
		mmvals.flags = param[4].data.d_int32;
	    }

	    break;

	case RUN_WITH_LAST_VALS:
	    /* Possibly retrieve data */

	    gimp_get_data("plug_in_mathmap", &mmvals);
	    break;

	default:
	    break;
    } /* switch */

    /* Mathmap the image */

    if ((status == STATUS_SUCCESS) &&
	(gimp_drawable_color(drawable->id) ||
	 gimp_drawable_gray(drawable->id))) {

	intersamplingEnabled = mmvals.flags & FLAG_INTERSAMPLING;
	oversamplingEnabled = mmvals.flags & FLAG_OVERSAMPLING;

	/* Set the tile cache size */

	gimp_tile_cache_ntiles((drawable->width + gimp_tile_width() - 1) / gimp_tile_width());

	/* Run! */

	mathmap();

	/* If run mode is interactive, flush displays */

	if (run_mode != RUN_NONINTERACTIVE)
	    gimp_displays_flush();

	/* Store data */

	if (run_mode == RUN_INTERACTIVE)
	    gimp_set_data("plug_in_mathmap", &mmvals, sizeof(mathmap_vals_t));
    } else if (status == STATUS_SUCCESS)
	status = STATUS_EXECUTION_ERROR;

    values[0].data.d_status = status;

    gimp_drawable_detach(drawable);
} /* run */


/*****/

inline void
calc_ra (void)
{
    if (usesRA)
    {
	double x = currentX,
	    y = currentY;

	currentR = sqrt(x * x + y * y);
	if (currentR == 0.0)
	    currentA = 0.0;
	else
	    currentA = acos(x / currentR) * 180 / M_PI;

	if (y < 0)
	    currentA = 360 - currentA;
    }
}

static void
mathmap(void)
{
    GPixelRgn dest_rgn;
    gpointer  pr;
    gint      progress, max_progress;
    guchar   *dest_row;
    guchar   *dest;
    gint      row, col;
    guchar    pixel[4][4];
    guchar    values[4];
    double    cx, cy;
    int       ix, iy;
    int       i;
    double result;
    int red,
	green,
	blue;

    theExprtree = 0;

    yy_scan_string(mmvals.expression);
    yyparse();
    if (theExprtree == 0)
	return;

    make_postfix(theExprtree);

    /* Initialize pixel region */

    gimp_pixel_rgn_init(&dest_rgn, drawable, sel_x1, sel_y1, sel_width, sel_height,
			TRUE, TRUE);

    imageWidth = sel_width;
    imageHeight = sel_height;
    wholeImageWidth = img_width;
    wholeImageHeight = img_height;

    middleX = imageWidth / 2.0;
    middleY = imageHeight / 2.0;

    if (middleX > imageWidth - middleX)
	imageX = middleX;
    else
	imageX = imageWidth - middleX;

    if (middleY > imageHeight - middleY)
	imageY = middleY;
    else
	imageY = imageHeight - middleY;
    
    imageR = sqrt(imageX * imageX + imageY * imageY);

    progress     = 0;
    max_progress = sel_width * sel_height;

    gimp_progress_init("Mathmaping...");

    for (pr = gimp_pixel_rgns_register(1, &dest_rgn);
	 pr != NULL; pr = gimp_pixel_rgns_process(pr))
    {
	if (oversamplingEnabled)
	{
	    unsigned char *line1,
		*line2,
		*line3;

	    dest_row = dest_rgn.data;

	    line1 = (unsigned char*)malloc((sel_width + 1) * 3);
	    line2 = (unsigned char*)malloc(sel_width * 3);
	    line3 = (unsigned char*)malloc((sel_width + 1) * 3);

	    for (col = 0; col <= dest_rgn.w; ++col)
	    {
		currentX = col + dest_rgn.x - sel_x1 - middleX;
		currentY = 0.0 + dest_rgn.y - sel_y1 - middleY;
		calc_ra(); result = eval_postfix();
		double_to_color(result, &red, &green, &blue);

		line1[col * 3 + 0] = red;
		line1[col * 3 + 1] = green;
		line1[col * 3 + 2] = blue;
	    }

	    for (row = 0; row < dest_rgn.h; ++row)
	    {
		dest = dest_row;

		for (col = 0; col < dest_rgn.w; ++col)
		{
		    currentX = col + dest_rgn.x - sel_x1 + 0.5 - middleX;
		    currentY = row + dest_rgn.y - sel_y1 + 0.5 - middleY;
		    calc_ra(); result = eval_postfix();
		    double_to_color(result, &red, &green, &blue);
		    
		    line2[col * 3 + 0] = red;
		    line2[col * 3 + 1] = green;
		    line2[col * 3 + 2] = blue;
		}
		for (col = 0; col <= dest_rgn.w; ++col)
		{
		    currentX = col + dest_rgn.x - sel_x1 - middleX;
		    currentY = row + dest_rgn.y - sel_y1 + 1.0 - middleY;
		    calc_ra(); result = eval_postfix();
		    double_to_color(result, &red, &green, &blue);
		
		    line3[col * 3 + 0] = red;
		    line3[col * 3 + 1] = green;
		    line3[col * 3 + 2] = blue;
		}
	    
		for (col = 0; col < dest_rgn.w; ++col)
		{
		    red = line1[col*3+0]
			+ line1[(col+1)*3+0]
			+ 2*line2[col*3+0]
			+ line3[col*3+0]
			+ line3[(col+1)*3+0];
		    green = line1[col*3+1]
			+ line1[(col+1)*3+1]
			+ 2*line2[col*3+1]
			+ line3[col*3+1]
			+ line3[(col+1)*3+1];
		    blue = line1[col*3+2]
			+ line1[(col+1)*3+2]
			+ 2*line2[col*3+2]
			+ line3[col*3+2]
			+ line3[(col+1)*3+2];
		
		    dest[0] = red / 6;
		    dest[1] = green / 6;
		    dest[2] = blue / 6;
		    dest += img_bpp;
		}

		memcpy(line1, line3, (imageWidth + 1) * 3);

		dest_row += dest_rgn.rowstride;
	    }
	}
	else
	{
	    dest_row = dest_rgn.data;

	    for (row = dest_rgn.y; row < (dest_rgn.y + dest_rgn.h); row++)
	    {
		dest = dest_row;

		for (col = dest_rgn.x; col < (dest_rgn.x + dest_rgn.w); col++)
		{
		    ix = (int) cx;
		    iy = (int) cy;

		    currentX = col - sel_x1 - middleX; currentY = row - sel_y1 - middleY;
		    calc_ra(); result = eval_postfix();
		    double_to_color(result, &red, &green, &blue);
		    
		    dest[0] = red;
		    dest[1] = green;
		    dest[2] = blue;
		    dest += img_bpp;
		}
		
		dest_row += dest_rgn.rowstride;
	    }
	}

	/* Update progress */
	progress += dest_rgn.w * dest_rgn.h;
	gimp_progress_update((double) progress / max_progress);
    }

    if (the_tile != NULL) {
	gimp_tile_unref(the_tile, FALSE);
	the_tile = NULL;
    } /* if */

    gimp_drawable_flush(drawable);
    gimp_drawable_merge_shadow(drawable->id, TRUE);
    gimp_drawable_update(drawable->id, sel_x1, sel_y1, sel_width, sel_height);
} /* mathmap */


/*****/

void
mathmap_get_pixel(int x, int y, guchar *pixel)
{
    static gint row  = -1;
    static gint col  = -1;

    gint    newcol, newrow;
    gint    newcoloff, newrowoff;
    guchar *p;
    int     i;

    if ((x < 0) || (x >= img_width) || (y < 0) || (y >= img_height)) {
	pixel[0] = 0;
	pixel[1] = 0;
	pixel[2] = 0;
	pixel[3] = 0;

	return;
    } /* if */

    newcol    = x / tile_width; /* The compiler should optimize this */
    newcoloff = x % tile_width;
    newrow    = y / tile_height;
    newrowoff = y % tile_height;

    if ((col != newcol) || (row != newrow) || (the_tile == NULL)) {
	if (the_tile != NULL)
	    gimp_tile_unref(the_tile, FALSE);

	the_tile = gimp_drawable_get_tile(drawable, FALSE, newrow, newcol);
	gimp_tile_ref(the_tile);

	col = newcol;
	row = newrow;
    } /* if */

    p = the_tile->data + the_tile->bpp * (the_tile->ewidth * newrowoff + newcoloff);

    for (i = img_bpp; i; i--)
	*pixel++ = *p++;
}

/*****/

static void
build_preview_source_image(void)
{
    double  left, right, bottom, top;
    double  px, py;
    double  dx, dy;
    int     x, y;
    guchar *p;
    guchar  pixel[4];

    wint.image  = g_malloc(preview_width * preview_height * 3 * sizeof(guchar));
    wint.wimage = g_malloc(preview_width * preview_height * 3 * sizeof(guchar));

    left   = sel_x1;
    right  = sel_x2 - 1;
    bottom = sel_y2 - 1;
    top    = sel_y1;

    dx = (right - left) / (preview_width - 1);
    dy = (bottom - top) / (preview_height - 1);

    py = top;

    p = wint.image;

    for (y = 0; y < preview_height; y++)
    {
	px = left;

	for (x = 0; x < preview_width; x++)
	{
	    mathmap_get_pixel((int) px, (int) py, pixel);

	    if (img_bpp < 3)
	    {
		pixel[1] = pixel[0];
		pixel[2] = pixel[0];
	    } /* if */

	    *p++ = pixel[0];
	    *p++ = pixel[1];
	    *p++ = pixel[2];

	    px += dx;
	} /* for */

	py += dy;
    } /* for */
} /* build_preview_source_image */


/*****/

static gint
mathmap_dialog(void)
{
    GtkWidget  *dialog;
    GtkWidget  *top_table;
    GtkWidget  *frame;
    GtkWidget  *table;
    GtkWidget  *button;
    GtkWidget  *label;
    GtkWidget  *entry;
    GtkWidget  *toggle;
    GtkWidget  *arrow;
    GtkWidget  *alignment;
    GtkWidget  *menubox;
    GtkWidget  *menubar;
    GtkWidget  *menubaritem;
    GtkWidget  *menu;
    GtkWidget  *item;
    int i;
    gint        argc;
    gchar     **argv;
    guchar     *color_cube;
    /*
      printf("Waiting... (pid %d)\n", getpid());
      kill(getpid(), SIGSTOP);
    */
    argc    = 1;
    argv    = g_new(gchar *, 1);
    argv[0] = g_strdup("mathmap");

    gtk_init(&argc, &argv);

    gtk_preview_set_gamma(gimp_gamma());
    gtk_preview_set_install_cmap(gimp_install_cmap());
    color_cube = gimp_color_cube();
    gtk_preview_set_color_cube(color_cube[0], color_cube[1], color_cube[2], color_cube[3]);

    gtk_widget_set_default_visual(gtk_preview_get_visual());
    gtk_widget_set_default_colormap(gtk_preview_get_cmap());

    fprintf(stderr, "here we are5!\n");

    build_preview_source_image();

    dialog = gtk_dialog_new();
    gtk_window_set_title(GTK_WINDOW(dialog), "MathMap");
    gtk_window_position(GTK_WINDOW(dialog), GTK_WIN_POS_MOUSE);
    gtk_container_border_width(GTK_CONTAINER(dialog), 0);
    gtk_signal_connect(GTK_OBJECT(dialog), "destroy",
		       (GtkSignalFunc) dialog_close_callback,
		       NULL);

    top_table = gtk_table_new(6, 3, FALSE);
    gtk_container_border_width(GTK_CONTAINER(top_table), 6);
    gtk_table_set_row_spacings(GTK_TABLE(top_table), 4);
    gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->vbox), top_table, FALSE, FALSE, 0);
    gtk_widget_show(top_table);

	/* Preview */

    frame = gtk_frame_new(NULL);
    gtk_frame_set_shadow_type(GTK_FRAME(frame), GTK_SHADOW_IN);
    gtk_table_attach(GTK_TABLE(top_table), frame, 1, 2, 0, 1, 0, 0, 0, 0);
    gtk_widget_show(frame);

    wint.preview = gtk_preview_new(GTK_PREVIEW_COLOR);
    gtk_preview_size(GTK_PREVIEW(wint.preview), preview_width, preview_height);
    gtk_container_add(GTK_CONTAINER(frame), wint.preview);
    gtk_widget_show(wint.preview);

    button = gtk_button_new_with_label("Preview");
    gtk_signal_connect(GTK_OBJECT(button), "clicked", (GtkSignalFunc)dialog_preview_callback, 0);
    gtk_table_attach(GTK_TABLE(top_table), button, 1, 2, 1, 2, GTK_FILL, GTK_FILL, 0, 0);
    gtk_widget_show(button);

    alignment = gtk_alignment_new(0, 0, 0, 0);
    toggle = gtk_check_button_new_with_label("Intersampling");
    gtk_container_add(GTK_CONTAINER(alignment), toggle);
    gtk_toggle_button_set_state(GTK_TOGGLE_BUTTON(toggle), mmvals.flags & FLAG_INTERSAMPLING);
    gtk_table_attach(GTK_TABLE(top_table), alignment, 0, 3, 2, 3, GTK_FILL, 0, 0, 0);
    gtk_signal_connect(GTK_OBJECT(toggle), "toggled",
		       (GtkSignalFunc)dialog_intersampling_update, 0);
    gtk_widget_show(toggle);
    gtk_widget_show(alignment);

    alignment = gtk_alignment_new(0, 0, 0, 0);
    toggle = gtk_check_button_new_with_label("Oversampling");
    gtk_container_add(GTK_CONTAINER(alignment), toggle);
    gtk_toggle_button_set_state(GTK_TOGGLE_BUTTON(toggle), mmvals.flags & FLAG_OVERSAMPLING);
    gtk_table_attach(GTK_TABLE(top_table), alignment, 0, 3, 3, 4, GTK_FILL, 0, 0, 0);
    gtk_signal_connect(GTK_OBJECT(toggle), "toggled",
		       (GtkSignalFunc)dialog_oversampling_update, 0);
    gtk_widget_show(toggle);
    gtk_widget_show(alignment);

    /* Menu */

    menu = gtk_menu_new();

    for (i = 0; examples[i][0] != 0; ++i)
    {
	item = gtk_menu_item_new_with_label(examples[i][0]);
	gtk_menu_append(GTK_MENU(menu), item);
	gtk_signal_connect(GTK_OBJECT(item), "activate", (GtkSignalFunc)dialog_example_callback, examples[i][1]);
	gtk_widget_show(item);
    }

    alignment = gtk_alignment_new(0, 0, 0, 0);
    menubar = gtk_menu_bar_new();
    gtk_container_add(GTK_CONTAINER(alignment), menubar);
    menubaritem = gtk_menu_item_new();

    menubox = gtk_hbox_new(FALSE, 2);
    label = gtk_label_new("Examples");
    arrow = gtk_arrow_new(GTK_ARROW_DOWN, GTK_SHADOW_OUT);
    gtk_box_pack_start(GTK_BOX(menubox), arrow, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(menubox), label, FALSE, FALSE, 4);

    gtk_container_add(GTK_CONTAINER(menubaritem), menubox);
    gtk_container_add(GTK_CONTAINER(menubar), menubaritem);
    gtk_menu_item_set_submenu(GTK_MENU_ITEM(menubaritem), menu);

    gtk_table_attach(GTK_TABLE(top_table), alignment, 0, 3, 4, 5, GTK_FILL, 0, 0, 0);

    gtk_widget_show(arrow);
    gtk_widget_show(label);
    gtk_widget_show(menubox);
    gtk_widget_show(menubaritem);
    gtk_widget_show(menubar);
    gtk_widget_show(alignment);

	/* Expression */

    table = gtk_table_new(1, 2, FALSE);
    gtk_container_border_width(GTK_CONTAINER(table), 0);
    gtk_table_attach(GTK_TABLE(top_table), table, 0, 3, 5, 6, GTK_EXPAND | GTK_FILL, 0, 0, 0);
    gtk_widget_show(table);

    label = gtk_label_new("Expression");
    gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.5);
    gtk_table_attach(GTK_TABLE(table), label, 0, 1, 0, 1, GTK_FILL, GTK_FILL, 4, 0);
    gtk_widget_show(label);

    expressionEntry = gtk_entry_new();
    /*
	gtk_object_set_user_data(GTK_OBJECT(entry), scale_data);
	gtk_object_set_user_data(scale_data, entry);
	*/
    gtk_widget_set_usize(expressionEntry, ENTRY_WIDTH, 0);
    gtk_entry_set_text(GTK_ENTRY(expressionEntry), mmvals.expression);
    gtk_signal_connect(GTK_OBJECT(expressionEntry), "changed",
		       (GtkSignalFunc) dialog_entry_update,
		       0);
    gtk_table_attach(GTK_TABLE(table), expressionEntry, 1, 2, 0, 1,
		     GTK_EXPAND | GTK_FILL, GTK_FILL, 4, 0);
    gtk_widget_show(expressionEntry);
	
	/* Buttons */

    gtk_container_border_width(GTK_CONTAINER(GTK_DIALOG(dialog)->action_area), 6);

    button = gtk_button_new_with_label("OK");
    GTK_WIDGET_SET_FLAGS(button, GTK_CAN_DEFAULT);
    gtk_signal_connect(GTK_OBJECT(button), "clicked",
		       (GtkSignalFunc) dialog_ok_callback,
		       dialog);
    gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->action_area), button, TRUE, TRUE, 0);
    gtk_widget_grab_default(button);
    gtk_widget_show(button);

    button = gtk_button_new_with_label("Cancel");
    GTK_WIDGET_SET_FLAGS(button, GTK_CAN_DEFAULT);
    gtk_signal_connect(GTK_OBJECT(button), "clicked",
		       (GtkSignalFunc) dialog_cancel_callback,
		       dialog);
    gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->action_area), button, TRUE, TRUE, 0);
    gtk_widget_show(button);

    /* Done */

    gtk_widget_show(dialog);
    /*	dialog_update_preview(); */

    gtk_main();
    gdk_flush();

    if (the_tile != NULL) {
	gimp_tile_unref(the_tile, FALSE);
	the_tile = NULL;
    } /* if */

    g_free(wint.image);
    g_free(wint.wimage);

    return wint.run;
} /* mathmap_dialog */


/*****/

static void
dialog_update_preview(void)
{
    double  left, right, bottom, top;
    double  dx, dy;
    double  px, py;
    double  cx, cy;
    int     ix, iy;
    int     x, y;
    double  scale_x, scale_y;
    guchar *p_ul, *p_lr, *i, *p;

    intersamplingEnabled = mmvals.flags & FLAG_INTERSAMPLING;
    oversamplingEnabled = mmvals.flags & FLAG_OVERSAMPLING;

    left   = sel_x1;
    right  = sel_x2 - 1;
    bottom = sel_y2 - 1;
    top    = sel_y1;

    dx = (right - left) / (preview_width - 1);
    dy = (bottom - top) / (preview_height - 1);

    scale_x = (double) (preview_width - 1) / (right - left);
    scale_y = (double) (preview_height - 1) / (bottom - top);

    py = top;

    p_ul = wint.wimage;
    p_lr = wint.wimage + 3 * (preview_width * preview_height - 1);

    theExprtree = 0;

    yy_scan_string(mmvals.expression);
    yyparse();
    if (theExprtree == 0)
	return;
    
    make_postfix(theExprtree);

    imageWidth = sel_width;
    imageHeight = sel_height;
    wholeImageWidth = img_width;
    wholeImageHeight = img_height;

    middleX = imageWidth / 2.0;
    middleY = imageHeight / 2.0;

    if (middleX > imageWidth - middleX)
	imageX = middleX;
    else
	imageX = imageWidth - middleX;

    if (middleY > imageHeight - middleY)
	imageY = middleY;
    else
	imageY = imageHeight - middleY;
    
    imageR = sqrt(imageX * imageX + imageY * imageY);

    for (y = 0; y <= preview_height; y++)
    {
	px = left;

	for (x = 0; x < preview_width; x++)
	{
	    double result;
	    int red,
		green,
		blue;

	    currentX = x * imageWidth / preview_width - middleX;
	    currentY = y * imageHeight / preview_height - middleY;
	    calc_ra(); result = eval_postfix();
	    double_to_color(result, &red, &green, &blue);

	    p_ul[0] = red;
	    p_ul[1] = green;
	    p_ul[2] = blue;

	    p_ul += 3;
	}
    }

    p = wint.wimage;

    for (y = 0; y < preview_height; y++)
    {
	gtk_preview_draw_row(GTK_PREVIEW(wint.preview), p, 0, y, preview_width);

	p += preview_width * 3;
    } /* for */

    gtk_widget_draw(wint.preview, NULL);
    gdk_flush();
} /* dialog_update_preview */


/*****/

static void
dialog_create_value(char *title, GtkTable *table, int row, gdouble *value,
		    double left, double right)
{
    GtkWidget *label;
    GtkWidget *scale;
    GtkWidget *entry;
    GtkObject *scale_data;
    char       buf[256];

    label = gtk_label_new(title);
    gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.5);
    gtk_table_attach(table, label, 0, 1, row, row + 1, GTK_FILL, GTK_FILL, 4, 0);
    gtk_widget_show(label);

    scale_data = gtk_adjustment_new(*value, left, right,
				    (right - left) / 360.0,
				    (right - left) / 360.0,
				    0.0);

    gtk_signal_connect(GTK_OBJECT(scale_data), "value_changed",
		       (GtkSignalFunc) dialog_scale_update,
		       value);

    scale = gtk_hscale_new(GTK_ADJUSTMENT(scale_data));
    gtk_widget_set_usize(scale, SCALE_WIDTH, 0);
    gtk_table_attach(table, scale, 1, 2, row, row + 1, GTK_EXPAND | GTK_FILL, GTK_FILL, 0, 0);
    gtk_scale_set_draw_value(GTK_SCALE(scale), FALSE);
    gtk_scale_set_digits(GTK_SCALE(scale), 3);
    gtk_range_set_update_policy(GTK_RANGE(scale), GTK_UPDATE_CONTINUOUS);
    gtk_widget_show(scale);

    entry = gtk_entry_new();
    gtk_object_set_user_data(GTK_OBJECT(entry), scale_data);
    gtk_object_set_user_data(scale_data, entry);
    gtk_widget_set_usize(entry, ENTRY_WIDTH, 0);
    sprintf(buf, "%0.2f", *value);
    gtk_entry_set_text(GTK_ENTRY(entry), buf);
    gtk_signal_connect(GTK_OBJECT(entry), "changed",
		       (GtkSignalFunc) dialog_entry_update,
		       value);
    gtk_table_attach(GTK_TABLE(table), entry, 2, 3, row, row + 1, GTK_FILL, GTK_FILL, 4, 0);
    gtk_widget_show(entry);
} /* dialog_create_value */


/*****/

static void
dialog_scale_update(GtkAdjustment *adjustment, gdouble *value)
{
    GtkWidget *entry;
    char       buf[256];

    if (*value != adjustment->value) {
	*value = adjustment->value;

	entry = gtk_object_get_user_data(GTK_OBJECT(adjustment));
	sprintf(buf, "%0.2f", *value);

	gtk_signal_handler_block_by_data(GTK_OBJECT(entry), value);
	gtk_entry_set_text(GTK_ENTRY(entry), buf);
	gtk_signal_handler_unblock_by_data(GTK_OBJECT(entry), value);

	/*		dialog_update_preview(); */
    } /* if */
} /* dialog_scale_update */


/*****/

static void
dialog_entry_update(GtkWidget *widget, gdouble *value)
{
    expression_copy(mmvals.expression, gtk_entry_get_text(GTK_ENTRY(widget)));
} /* dialog_entry_update */

/*****/

static void
dialog_oversampling_update (GtkWidget *widget, gpointer data)
{
    mmvals.flags &= ~FLAG_OVERSAMPLING;

    if (GTK_TOGGLE_BUTTON(widget)->active)
	mmvals.flags |= FLAG_OVERSAMPLING;
}

/*****/

static void
dialog_intersampling_update (GtkWidget *widget, gpointer data)
{
    mmvals.flags &= ~FLAG_INTERSAMPLING;

    if (GTK_TOGGLE_BUTTON(widget)->active)
	mmvals.flags |= FLAG_INTERSAMPLING;
}

/*****/

static void
dialog_example_callback (GtkWidget *widget, char *expression)
{
    gtk_entry_set_text(GTK_ENTRY(expressionEntry), expression);
}

/*****/

static void
dialog_preview_callback (GtkWidget *widget, gpointer data)
{
    dialog_update_preview();
}

/*****/

static void
dialog_close_callback(GtkWidget *widget, gpointer data)
{
    gtk_main_quit();
} /* dialog_close_callback */


/*****/

static void
dialog_ok_callback(GtkWidget *widget, gpointer data)
{
    wint.run = TRUE;
    gtk_widget_destroy(GTK_WIDGET(data));
} /* dialog_ok_callback */


/*****/

static void
dialog_cancel_callback(GtkWidget *widget, gpointer data)
{
    gtk_widget_destroy(GTK_WIDGET(data));
} /* dialog_cancel_callback */
