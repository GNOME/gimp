/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * Kaleidoscope 
 * Copyright (C) 1999 Kelly Martin
 * Derived from WhirlPinch
 * Copyright (C) 1997 Federico Mena Quintero
 * federico@nuclecu.unam.mx
 * Copyright (C) 1997 Scott Goehring
 * scott@poverty.bloomington.in.us
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


/* Version 0.01
 *
 * First version.
 */

#include <signal.h>
#include <stdlib.h>
#include <stdio.h>

#include "gtk/gtk.h"
#include "libgimp/gimp.h"

#define PLUG_IN_NAME    "plug_in_kaleidoscope"
#define PLUG_IN_VERSION "0.01 (1999/09/30)"

/***** Magic numbers *****/

#define PREVIEW_SIZE 128
#define SCALE_WIDTH  200
#define ENTRY_WIDTH  60

#define CHECK_SIZE  8
#define CHECK_DARK  ((int) (1.0 / 3.0 * 255))
#define CHECK_LIGHT ((int) (2.0 / 3.0 * 255))


/***** Types *****/

typedef struct {
  gdouble angle1;
  gdouble angle2;
  gint    nsegs;
  gdouble cen_x;
  gdouble cen_y;
  gdouble off_x;
  gdouble off_y;
} kaleidoscope_vals_t;

typedef struct {
  GtkWidget *preview;
  guchar    *check_row_0;
  guchar    *check_row_1;
  guchar    *image;
  guchar    *dimage;

  gint run;
} kaleidoscope_interface_t;

typedef struct {
  gint       col, row;
  gint       img_width, img_height, img_bpp, img_has_alpha;
  gint       tile_width, tile_height;
  guchar     bg_color[4];
  GDrawable *drawable;
  GTile     *tile;
} pixel_fetcher_t;


/***** Prototypes *****/

static void query(void);
static void run(char    *name,
		int      nparams,
		GParam  *param,
		int     *nreturn_vals,
		GParam **return_vals);

static void   kaleidoscope(void);
static int    calc_undistorted_coords(double wx, double wy,
				      double angle1, double angle2, int nsegs,
				      double cen_x, double cen_y,
				      double off_x, double off_y,
				      double *x, double *y);
static guchar bilinear(double x, double y, guchar *values);

static pixel_fetcher_t *pixel_fetcher_new(GDrawable *drawable);
static void             pixel_fetcher_set_bg_color(pixel_fetcher_t *pf, guchar r, guchar g, guchar b, guchar a);
static void             pixel_fetcher_get_pixel(pixel_fetcher_t *pf, int x, int y, guchar *pixel);
static void             pixel_fetcher_destroy(pixel_fetcher_t *pf);

static void build_preview_source_image(void);

static gint kaleidoscope_dialog(void);
static void dialog_update_preview(void);
static void dialog_create_value(char *title, GtkTable *table, int row, gdouble *value,
				double left, double right, double step);
static void dialog_scale_update(GtkAdjustment *adjustment, gdouble *value);
static void dialog_entry_update(GtkWidget *widget, gdouble *value);
static void dialog_create_int_value(char *title, GtkTable *table, int row, gint *value,
				    gint left, gint right, gint step);
static void dialog_int_scale_update(GtkAdjustment *adjustment, gint *value);
static void dialog_int_entry_update(GtkWidget *widget, gint *value);
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

static kaleidoscope_vals_t wpvals = {
  0.0,  /* angle1 */
  0.0,  /* angle2 */
  6,    /* nsegs */
  0.0,  /* x center */
  0.0,  /* y center */
  0.0,  /* x offset */
  0.0   /* y offset */
}; /* wpvals */

static kaleidoscope_interface_t wpint = {
  NULL,  /* preview */
  NULL,  /* check_row_0 */
  NULL,  /* check_row_1 */
  NULL,  /* image */
  NULL,  /* dimage */
  FALSE  /* run */
}; /* wpint */

static GDrawable *drawable;

static gint img_width, img_height, img_bpp, img_has_alpha;
static gint sel_x1, sel_y1, sel_x2, sel_y2;
static gint sel_width, sel_height;
static gint preview_width, preview_height;

static double scale_x, scale_y;
static double radius;


/***** Functions *****/

/*****/

MAIN()


     /*****/

     static void
query(void)
{
  static GParamDef args[] = {
    { PARAM_INT32,    "run_mode",  "Interactive, non-interactive" },
    { PARAM_IMAGE,    "image",     "Input image" },
    { PARAM_DRAWABLE, "drawable",  "Input drawable" },
    { PARAM_FLOAT,    "angle1",    "Angle of leading edge of viewing slice" },
    { PARAM_FLOAT,    "angle2",    "Rollback angle" },
    { PARAM_INT32,    "nsegs",     "Number of segments" }
  }; /* args */

  static GParamDef *return_vals  = NULL;
  static int        nargs        = sizeof(args) / sizeof(args[0]);
  static int        nreturn_vals = 0;

  gimp_install_procedure(PLUG_IN_NAME,
			 "Simulate looking at an image thru a kaleidoscope",
			 "Simulate looking at an image thru a kaleidoscope",
			 "Kelly Martin",
			 "Kelly Martin",
			 PLUG_IN_VERSION,
			 "<Image>/Filters/Distorts/Kaleidoscope",
			 "RGB*, GRAY*",
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

  GRunModeType run_mode;
  GStatusType  status;
  double       xhsiz, yhsiz;
  int          pwidth, pheight;

  status   = STATUS_SUCCESS;
  run_mode = param[0].data.d_int32;

  values[0].type          = PARAM_STATUS;
  values[0].data.d_status = status;

  *nreturn_vals = 1;
  *return_vals  = values;

  /* Get the active drawable info */

  drawable = gimp_drawable_get(param[2].data.d_drawable);

  img_width     = gimp_drawable_width(drawable->id);
  img_height    = gimp_drawable_height(drawable->id);
  img_bpp       = gimp_drawable_bpp(drawable->id);
  img_has_alpha = gimp_drawable_has_alpha(drawable->id);

  gimp_drawable_mask_bounds(drawable->id, &sel_x1, &sel_y1, &sel_x2, &sel_y2);

  /* Calculate scaling parameters */

  sel_width  = sel_x2 - sel_x1;
  sel_height = sel_y2 - sel_y1;

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

  radius = MAX(xhsiz, yhsiz);

  /* Calculate preview size */

  if (sel_width > sel_height) {
    pwidth  = MIN(sel_width, PREVIEW_SIZE);
    pheight = sel_height * pwidth / sel_width;
  } else {
    pheight = MIN(sel_height, PREVIEW_SIZE);
    pwidth  = sel_width * pheight / sel_height;
  } /* else */

  preview_width  = MAX(pwidth, 2); /* Min size is 2 */
  preview_height = MAX(pheight, 2);

  /* See how we will run */

  switch (run_mode) {
  case RUN_INTERACTIVE:
    /* Possibly retrieve data */

    gimp_get_data(PLUG_IN_NAME, &wpvals);

    /* Get information from the dialog */

    if (!kaleidoscope_dialog())
      return;

    break;

  case RUN_NONINTERACTIVE:
    /* Make sure all the arguments are present */

    if (nparams != 10)
      status = STATUS_CALLING_ERROR;

    if (status == STATUS_SUCCESS) {
      wpvals.angle1 = param[3].data.d_float;
      wpvals.angle2 = param[4].data.d_float;
      wpvals.nsegs  = param[5].data.d_int32;
      wpvals.cen_x  = param[6].data.d_float;
      wpvals.cen_y  = param[7].data.d_float;
      wpvals.off_x  = param[8].data.d_float;
      wpvals.off_y  = param[9].data.d_float;
    } /* if */

    break;

  case RUN_WITH_LAST_VALS:
    /* Possibly retrieve data */

    gimp_get_data(PLUG_IN_NAME, &wpvals);
    break;

  default:
    break;
  } /* switch */

  /* Distort the image */

  if ((status == STATUS_SUCCESS) &&
      (gimp_drawable_color(drawable->id) ||
       gimp_drawable_gray(drawable->id))) {
    /* Set the tile cache size */

    gimp_tile_cache_ntiles(2 * (drawable->width + gimp_tile_width() - 1) / gimp_tile_width());

    /* Run! */

    kaleidoscope();

    /* If run mode is interactive, flush displays */

    if (run_mode != RUN_NONINTERACTIVE)
      gimp_displays_flush();

    /* Store data */

    if (run_mode == RUN_INTERACTIVE)
      gimp_set_data(PLUG_IN_NAME, &wpvals, sizeof(kaleidoscope_vals_t));
  } else if (status == STATUS_SUCCESS)
    status = STATUS_EXECUTION_ERROR;

  values[0].data.d_status = status;

  gimp_drawable_detach(drawable);
} /* run */


/*****/

static void
kaleidoscope(void)
{
  GPixelRgn        dest_rgn;
  gint             progress, max_progress;
  guchar          *top_row, *bot_row;
  guchar          *top_p, *bot_p;
  gint             row, col;
  guchar           pixel[4][4];
  guchar           values[4];
  double           cx, cy;
  double	   angle1, angle2;
  int              ix, iy;
  int              i;
  guchar           bg_color[4];
  pixel_fetcher_t *pft;
#if 0
  printf("Waiting... (pid %d)\n", getpid());
  kill(getpid(), SIGSTOP);
#endif
  /* Initialize rows */

  top_row = g_malloc(img_bpp * sel_width);
  bot_row = g_malloc(img_bpp * sel_width);

  /* Initialize pixel region */

  gimp_pixel_rgn_init(&dest_rgn, drawable, sel_x1, sel_y1, sel_width, sel_height, TRUE, TRUE);

  pft = pixel_fetcher_new(drawable);

  gimp_palette_get_background(&bg_color[0], &bg_color[1], &bg_color[2]);
  pixel_fetcher_set_bg_color(pft,
			     bg_color[0],
			     bg_color[1],
			     bg_color[2],
			     (img_has_alpha ? 0 : 255));

  progress     = 0;
  max_progress = sel_width * sel_height;

  gimp_progress_init("Kaleidoscoping...");

  angle1   = wpvals.angle1 * G_PI / 180;
  angle2   = wpvals.angle2 * G_PI / 180;

  for (row = sel_y1; row < (sel_y1 + sel_y2); row++) {
    top_p = top_row;
    bot_p = bot_row + img_bpp * (sel_width - 1);

    for (col = sel_x1; col < sel_x2; col++) {
      if (calc_undistorted_coords(col, row, angle1, angle2, 
				  wpvals.nsegs,
				  wpvals.cen_x, wpvals.cen_y,
				  wpvals.off_x, wpvals.off_y,
				  &cx, &cy)) {
	/* We are inside the distortion area */

	/* Top */

	if (cx >= 0.0)
	  ix = (int) cx;
	else
	  ix = -((int) -cx + 1);

	if (cy >= 0.0)
	  iy = (int) cy;
	else
	  iy = -((int) -cy + 1);

	pixel_fetcher_get_pixel(pft, ix,     iy,     pixel[0]);
	pixel_fetcher_get_pixel(pft, ix + 1, iy,     pixel[1]);
	pixel_fetcher_get_pixel(pft, ix,     iy + 1, pixel[2]);
	pixel_fetcher_get_pixel(pft, ix + 1, iy + 1, pixel[3]);

	for (i = 0; i < img_bpp; i++) {
	  values[0] = pixel[0][i];
	  values[1] = pixel[1][i];
	  values[2] = pixel[2][i];
	  values[3] = pixel[3][i];

	  *top_p++ = bilinear(cx, cy, values);
	} /* for */

      } else {
	/* We are outside the distortion area; just copy the source pixels */

	/* Top */

	pixel_fetcher_get_pixel(pft, col, row, pixel[0]);

	for (i = 0; i < img_bpp; i++)
	  *top_p++ = pixel[0][i];

      } /* else */
    } /* for */

    /* Paint rows to image */

    gimp_pixel_rgn_set_row(&dest_rgn, top_row, sel_x1, row, sel_width);

    /* Update progress */

    progress += sel_width;
    gimp_progress_update((double) progress / max_progress);
  } /* for */

  pixel_fetcher_destroy(pft);

  g_free(top_row);

  gimp_drawable_flush(drawable);
  gimp_drawable_merge_shadow(drawable->id, TRUE);
  gimp_drawable_update(drawable->id, sel_x1, sel_y1, sel_width, sel_height);
} /* kaleidoscope */


/*****/

static int
calc_undistorted_coords(double wx, double wy,
			double angle1, double angle2, int nsegs,
			double cen_x, double cen_y,
			double off_x, double off_y,
			double *x, double *y)
{
  double dx, dy;
  double r, ang;

  double awidth = G_PI/nsegs;
  double mult;

  dx = wx - cen_x;
  dy = wy - cen_y;

  r = sqrt(dx*dx+dy*dy);
  if (r == 0.0) {
    *x = wx + off_x;
    *y = wy + off_y;
    return TRUE;
  }

  ang = atan2(dy,dx) - angle1 - angle2;
  while (ang<0.0) ang = ang + 2*G_PI;

  mult = ceil(ang/awidth) - 1;
  ang = ang - mult*awidth;
  if (((int) mult) % 2 == 1) ang = awidth - ang;
  ang = ang + angle1;

  *x = cen_x + r*cos(ang) + off_x;
  *y = cen_y + r*sin(ang) + off_y;

  return TRUE;
} /* calc_undistorted_coords */


/*****/

static guchar
bilinear(double x, double y, guchar *values)
{
  double m0, m1;

  x = fmod(x, 1.0);
  y = fmod(y, 1.0);

  if (x < 0.0)
    x += 1.0;

  if (y < 0.0)
    y += 1.0;

  m0 = (double) values[0] + x * ((double) values[1] - values[0]);
  m1 = (double) values[2] + x * ((double) values[3] - values[2]);

  return (guchar) (m0 + y * (m1 - m0));
} /* bilinear */


/*****/

static pixel_fetcher_t *
pixel_fetcher_new(GDrawable *drawable)
{
  pixel_fetcher_t *pf;

  pf = g_malloc(sizeof(pixel_fetcher_t));

  pf->col           = -1;
  pf->row           = -1;
  pf->img_width     = gimp_drawable_width(drawable->id);
  pf->img_height    = gimp_drawable_height(drawable->id);
  pf->img_bpp       = gimp_drawable_bpp(drawable->id);
  pf->img_has_alpha = gimp_drawable_has_alpha(drawable->id);
  pf->tile_width    = gimp_tile_width();
  pf->tile_height   = gimp_tile_height();
  pf->bg_color[0]   = 0;
  pf->bg_color[1]   = 0;
  pf->bg_color[2]   = 0;
  pf->bg_color[3]   = 0;

  pf->drawable    = drawable;
  pf->tile        = NULL;

  return pf;
} /* pixel_fetcher_new */


/*****/

static void
pixel_fetcher_set_bg_color(pixel_fetcher_t *pf, guchar r, guchar g, guchar b, guchar a)
{
  pf->bg_color[0] = r;
  pf->bg_color[1] = g;
  pf->bg_color[2] = b;

  if (pf->img_has_alpha)
    pf->bg_color[pf->img_bpp - 1] = a;
} /* pixel_fetcher_set_bg_color */


/*****/

static void
pixel_fetcher_get_pixel(pixel_fetcher_t *pf, int x, int y, guchar *pixel)
{
  gint    col, row;
  gint    coloff, rowoff;
  guchar *p;
  int     i;

  if ((x < sel_x1) || (x >= sel_x2) ||
      (y < sel_y1) || (y >= sel_y2)) {
    for (i = 0; i < pf->img_bpp; i++)
      pixel[i] = pf->bg_color[i];

    return;
  } /* if */

  col    = x / pf->tile_width;
  coloff = x % pf->tile_width;
  row    = y / pf->tile_height;
  rowoff = y % pf->tile_height;

  if ((col != pf->col) ||
      (row != pf->row) ||
      (pf->tile == NULL)) {
    if (pf->tile != NULL)
      gimp_tile_unref(pf->tile, FALSE);

    pf->tile = gimp_drawable_get_tile(pf->drawable, FALSE, row, col);
    gimp_tile_ref(pf->tile);

    pf->col = col;
    pf->row = row;
  } /* if */

  p = pf->tile->data + pf->img_bpp * (pf->tile->ewidth * rowoff + coloff);

  for (i = pf->img_bpp; i; i--)
    *pixel++ = *p++;
} /* pixel_fetcher_get_pixel */


/*****/

static void
pixel_fetcher_destroy(pixel_fetcher_t *pf)
{
  if (pf->tile != NULL)
    gimp_tile_unref(pf->tile, FALSE);

  g_free(pf);
} /* pixel_fetcher_destroy */


/*****/

static void
build_preview_source_image(void)
{
  double           left, right, bottom, top;
  double           px, py;
  double           dx, dy;
  int              x, y;
  guchar          *p;
  guchar           pixel[4];
  pixel_fetcher_t *pf;

  wpint.check_row_0 = g_malloc(preview_width * sizeof(guchar));
  wpint.check_row_1 = g_malloc(preview_width * sizeof(guchar));
  wpint.image       = g_malloc(preview_width * preview_height * 4 * sizeof(guchar));
  wpint.dimage      = g_malloc(preview_width * preview_height * 3 * sizeof(guchar));

  left   = sel_x1;
  right  = sel_x2 - 1;
  bottom = sel_y2 - 1;
  top    = sel_y1;

  dx = (right - left) / (preview_width - 1);
  dy = (bottom - top) / (preview_height - 1);

  py = top;

  pf = pixel_fetcher_new(drawable);

  p = wpint.image;

  for (y = 0; y < preview_height; y++) {
    px = left;

    for (x = 0; x < preview_width; x++) {
      /* Checks */

      if ((x / CHECK_SIZE) & 1) {
	wpint.check_row_0[x] = CHECK_DARK;
	wpint.check_row_1[x] = CHECK_LIGHT;
      } else {
	wpint.check_row_0[x] = CHECK_LIGHT;
	wpint.check_row_1[x] = CHECK_DARK;
      } /* else */

      /* Thumbnail image */

      pixel_fetcher_get_pixel(pf, (int) px, (int) py, pixel);

      if (img_bpp < 3) {
	if (img_has_alpha)
	  pixel[3] = pixel[1];
	else
	  pixel[3] = 255;

	pixel[1] = pixel[0];
	pixel[2] = pixel[0];
      } else
	if (!img_has_alpha)
	  pixel[3] = 255;

      *p++ = pixel[0];
      *p++ = pixel[1];
      *p++ = pixel[2];
      *p++ = pixel[3];

      px += dx;
    } /* for */

    py += dy;
  } /* for */

  pixel_fetcher_destroy(pf);
} /* build_preview_source_image */


/*****/

static gint
kaleidoscope_dialog(void)
{
  GtkWidget  *dialog;
  GtkWidget  *top_table;
  GtkWidget  *frame;
  GtkWidget  *table;
  GtkWidget  *button;
  gint        argc;
  gchar     **argv;
  guchar     *color_cube;
#if 0
  printf("Waiting... (pid %d)\n", getpid());
  kill(getpid(), SIGSTOP);
#endif
  argc    = 1;
  argv    = g_new(gchar *, 1);
  argv[0] = g_strdup("kaleidoscope");

  gtk_init(&argc, &argv);
  gtk_rc_parse (gimp_gtkrc ());

  gdk_set_use_xshm(gimp_use_xshm());

  gtk_preview_set_gamma(gimp_gamma());
  gtk_preview_set_install_cmap(gimp_install_cmap());
  color_cube = gimp_color_cube();
  gtk_preview_set_color_cube(color_cube[0], color_cube[1], color_cube[2], color_cube[3]);

  gtk_widget_set_default_visual(gtk_preview_get_visual());
  gtk_widget_set_default_colormap(gtk_preview_get_cmap());

  build_preview_source_image();

  dialog = gtk_dialog_new();
  gtk_window_set_title(GTK_WINDOW(dialog), "Kaleidoscope");
  gtk_window_position(GTK_WINDOW(dialog), GTK_WIN_POS_MOUSE);
  gtk_container_border_width(GTK_CONTAINER(dialog), 0);
  gtk_signal_connect(GTK_OBJECT(dialog), "destroy",
		     (GtkSignalFunc) dialog_close_callback,
		     NULL);

  top_table = gtk_table_new(2, 3, FALSE);
  gtk_container_border_width(GTK_CONTAINER(top_table), 6);
  gtk_table_set_row_spacings(GTK_TABLE(top_table), 4);
  gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->vbox), top_table, FALSE, FALSE, 0);
  gtk_widget_show(top_table);

  /* Preview */

  frame = gtk_frame_new(NULL);
  gtk_frame_set_shadow_type(GTK_FRAME(frame), GTK_SHADOW_IN);
  gtk_table_attach(GTK_TABLE(top_table), frame, 1, 2, 0, 1, 0, 0, 0, 0);
  gtk_widget_show(frame);

  wpint.preview = gtk_preview_new(GTK_PREVIEW_COLOR);
  gtk_preview_size(GTK_PREVIEW(wpint.preview), preview_width, preview_height);
  gtk_container_add(GTK_CONTAINER(frame), wpint.preview);
  gtk_widget_show(wpint.preview);

  /* Controls */

  table = gtk_table_new(7, 3, FALSE);
  gtk_container_border_width(GTK_CONTAINER(table), 0);
  gtk_table_attach(GTK_TABLE(top_table), table, 0, 3, 1, 2, GTK_EXPAND | GTK_FILL, 0, 0, 0);
  gtk_widget_show(table);

  dialog_create_value("Angle 1", GTK_TABLE(table), 0, &wpvals.angle1, -360.0, 360.0, 1.0);
	
  dialog_create_value("Angle 2", GTK_TABLE(table), 1, &wpvals.angle2, -360.0, 360.0, 1.0);
	
  dialog_create_int_value("Number of segments", GTK_TABLE(table), 2, &wpvals.nsegs, 1, 16, 1);

  dialog_create_value("X center", GTK_TABLE(table), 3, &wpvals.cen_x, sel_x1, sel_x2, 1.0);
	
  dialog_create_value("Y center", GTK_TABLE(table), 4, &wpvals.cen_y, sel_y1, sel_y2, 1.0);

  dialog_create_value("X offset", GTK_TABLE(table), 5, &wpvals.off_x, -img_width, img_width, 1.0);
	
  dialog_create_value("Y offset", GTK_TABLE(table), 6, &wpvals.off_y, -img_height, img_height, 1.0);
	
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
  dialog_update_preview();

  gtk_main();
  gdk_flush();

  g_free(wpint.check_row_0);
  g_free(wpint.check_row_1);
  g_free(wpint.image);
  g_free(wpint.dimage);

  return wpint.run;
} /* kaleidoscope_dialog */


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
  guchar *p_ul, *i, *p;
  guchar *check_ul;
  int     check;
  guchar  outside[4];
  double  angle1, angle2;

  gimp_palette_get_background(&outside[0], &outside[1], &outside[2]);
  outside[3] = (img_has_alpha ? 0 : 255);

  if (img_bpp < 3) {
    outside[1] = outside[0];
    outside[2] = outside[0];
  } /* if */

  left   = sel_x1;
  right  = sel_x2 - 1;
  bottom = sel_y2 - 1;
  top    = sel_y1;

  dx = (right - left) / (preview_width - 1);
  dy = (bottom - top) / (preview_height - 1);

  scale_x = (double) preview_width / (right - left + 1);
  scale_y = (double) preview_height / (bottom - top + 1);

  angle1   = wpvals.angle1 * G_PI / 180;
  angle2   = wpvals.angle2 * G_PI / 180;

  py = top;

  p_ul = wpint.dimage;

  for (y = 0; y < preview_height; y++) {
    px = left;

    if ((y / CHECK_SIZE) & 1)
      check_ul = wpint.check_row_0;
    else
      check_ul = wpint.check_row_1;

    for (x = 0; x < preview_width; x++) {
      calc_undistorted_coords(px, py, angle1, angle2, wpvals.nsegs, 
			      wpvals.cen_x, wpvals.cen_y,
			      wpvals.off_x, wpvals.off_y,
			      &cx, &cy);

      cx = (cx - left) * scale_x;
      cy = (cy - top) * scale_y;

      ix = (int) (cx + 0.5);
      iy = (int) (cy + 0.5);

      check = check_ul[x];

      if ((ix >= 0) && (ix < preview_width) &&
	  (iy >= 0) && (iy < preview_height))
	i = wpint.image + 4 * (preview_width * iy + ix);
      else
	i = outside;

      p_ul[0] = check + ((i[0] - check) * i[3]) / 255;
      p_ul[1] = check + ((i[1] - check) * i[3]) / 255;
      p_ul[2] = check + ((i[2] - check) * i[3]) / 255;

      p_ul += 3;

      px += dx;
    } /* for */

    py += dy;
  } /* for */

  p = wpint.dimage;

  for (y = 0; y < preview_height; y++) {
    gtk_preview_draw_row(GTK_PREVIEW(wpint.preview), p, 0, y, preview_width);

    p += preview_width * 3;
  } /* for */

  gtk_widget_draw(wpint.preview, NULL);
  gdk_flush();
} /* dialog_update_preview */


/*****/

static void
dialog_create_value(char *title, GtkTable *table, int row, gdouble *value,
		    double left, double right, double step)
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
				  step,
				  step,
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
  sprintf(buf, "%0.3f", *value);
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
    sprintf(buf, "%0.3f", *value);

    gtk_signal_handler_block_by_data(GTK_OBJECT(entry), value);
    gtk_entry_set_text(GTK_ENTRY(entry), buf);
    gtk_signal_handler_unblock_by_data(GTK_OBJECT(entry), value);

    dialog_update_preview();
  } /* if */
} /* dialog_scale_update */


/*****/

static void
dialog_entry_update(GtkWidget *widget, gdouble *value)
{
  GtkAdjustment *adjustment;
  gdouble        new_value;

  new_value = atof(gtk_entry_get_text(GTK_ENTRY(widget)));

  if (*value != new_value) {
    adjustment = gtk_object_get_user_data(GTK_OBJECT(widget));

    if ((new_value >= adjustment->lower) &&
	(new_value <= adjustment->upper)) {
      *value            = new_value;
      adjustment->value = new_value;

      gtk_signal_emit_by_name(GTK_OBJECT(adjustment), "value_changed");

      dialog_update_preview();
    } /* if */
  } /* if */
} /* dialog_entry_update */

static void
dialog_create_int_value(char *title, GtkTable *table, int row, gint *value,
			int left, int right, int step)
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

  scale_data = gtk_adjustment_new(1.0 * (*value), 
				  1.0*left, 
				  1.0*right,
				  1.0*step,
				  1.0*step,
				  0.0);

  gtk_signal_connect(GTK_OBJECT(scale_data), "value_changed",
		     (GtkSignalFunc) dialog_int_scale_update,
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
  sprintf(buf, "%d", *value);
  gtk_entry_set_text(GTK_ENTRY(entry), buf);
  gtk_signal_connect(GTK_OBJECT(entry), "changed",
		     (GtkSignalFunc) dialog_int_entry_update,
		     value);
  gtk_table_attach(GTK_TABLE(table), entry, 2, 3, row, row + 1, GTK_FILL, GTK_FILL, 4, 0);
  gtk_widget_show(entry);
} /* dialog_create_int_value */


/*****/

static void
dialog_int_scale_update(GtkAdjustment *adjustment, gint *value)
{
  GtkWidget *entry;
  char       buf[256];

  if (*value != (int) adjustment->value) {
    *value = (int) adjustment->value;

    entry = gtk_object_get_user_data(GTK_OBJECT(adjustment));
    sprintf(buf, "%d", *value);

    gtk_signal_handler_block_by_data(GTK_OBJECT(entry), value);
    gtk_entry_set_text(GTK_ENTRY(entry), buf);
    gtk_signal_handler_unblock_by_data(GTK_OBJECT(entry), value);

    dialog_update_preview();
  } /* if */
} /* dialog_scale_update */


/*****/

static void
dialog_int_entry_update(GtkWidget *widget, gint *value)
{
  GtkAdjustment *adjustment;
  gint           new_int_value;

  new_int_value = atoi(gtk_entry_get_text(GTK_ENTRY(widget)));

  if (*value != new_int_value) {
    adjustment = gtk_object_get_user_data(GTK_OBJECT(widget));

    if ((new_int_value >= adjustment->lower) &&
	(new_int_value <= adjustment->upper)) {
      *value            = new_int_value;
      adjustment->value = 1.0*new_int_value;

      gtk_signal_emit_by_name(GTK_OBJECT(adjustment), "value_changed");

      dialog_update_preview();
    } /* if */
  } /* if */
} /* dialog_entry_update */


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
  wpint.run = TRUE;
  gtk_widget_destroy(GTK_WIDGET(data));
} /* dialog_ok_callback */


/*****/

static void
dialog_cancel_callback(GtkWidget *widget, gpointer data)
{
  gtk_widget_destroy(GTK_WIDGET(data));
} /* dialog_cancel_callback */
