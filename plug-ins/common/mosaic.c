/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
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

/*  Mosaic is a filter which transforms an image into
 *  what appears to be a mosaic, composed of small primitives,
 *  each of constant color and of an approximate size.
 *        Copyright (C) 1996  Spencer Kimball
 *        Speedups by Elliot Lee
 */
#include "config.h"

#include <stdlib.h>
#include <string.h>
#include "gtk/gtk.h"
#include "config.h"
#include "libgimp/stdplugins-intl.h"
#include "libgimp/gimp.h"

/*  The mosaic logo  */
#include "mosaic_logo.h"

#ifndef RAND_MAX
#define RAND_MAX 2147483647
#endif /* RAND_MAX */

#define  SCALE_WIDTH     150

#define  HORIZONTAL      0
#define  VERTICAL        1
#define  OPAQUE          255
#define  SUPERSAMPLE     3
#define  MAG_THRESHOLD   7.5
#define  COUNT_THRESHOLD 0.1
#define  MAX_POINTS      12

#define  SQUARES  0
#define  HEXAGONS 1
#define  OCTAGONS 2

#define  SMOOTH   0
#define  ROUGH    1

#define  BW       0
#define  FG_BG    1

typedef struct
{
  gdouble x, y;
} Vertex;

typedef struct
{
  gint npts;
  Vertex pts[MAX_POINTS];
} Polygon;

typedef struct
{
  gdouble base_x, base_y;
  gdouble norm_x, norm_y;
  gdouble light;
} SpecVec;

typedef struct
{
  gdouble tile_size;
  gdouble tile_height;
  gdouble tile_spacing;
  gdouble tile_neatness;
  gdouble light_dir;
  gdouble color_variation;
  gint    antialiasing;
  gint    color_averaging;
  gint    tile_type;
  gint    tile_surface;
  gint    grout_color;
} MosaicVals;

typedef struct
{
  gint       run;
} MosaicInterface;

/* Declare local functions.
 */
static void      query  (void);
static void      run    (char      *name,
			 int        nparams,
			 GParam    *param,
			 int       *nreturn_vals,
			 GParam   **return_vals);
static void      mosaic (GDrawable *drawable);

/*  user interface functions  */
static gint      mosaic_dialog     (void);

/*  gradient finding machinery  */
static void      find_gradients    (GDrawable *drawable,
				    gdouble   std_dev);
static void      find_max_gradient (GPixelRgn *src_rgn,
				    GPixelRgn *dest_rgn);

/*  gaussian & 1st derivative  */
static void      gaussian_deriv    (GPixelRgn *src_rgn,
				    GPixelRgn *dest_rgn,
				    gint      direction,
				    gdouble   std_dev,
				    gint *    prog,
				    gint      max_prog,
				    gint      ith_prog);
static void      make_curve        (gint *    curve,
				    gint *    sum,
				    gdouble   std_dev,
				    gint      length);
static void      make_curve_d      (gint *    curve,
				    gint *    sum,
				    gdouble   std_dev,
				    gint      length);

/*  grid creation and localization machinery  */
static gdouble   fp_rand              (gdouble val);
static void      grid_create_squares  (gint x1,
				       gint y1,
				       gint x2,
				       gint y2);
static void      grid_create_hexagons (gint x1,
				       gint y1,
				       gint x2,
				       gint y2);
static void      grid_create_octagons (gint x1,
				       gint y1,
				       gint x2,
				       gint y2);
static void      grid_localize        (gint x1,
				       gint y1,
				       gint x2,
				       gint y2);
static void      grid_render          (GDrawable * drawable);
static void      split_poly           (Polygon  * poly,
				       GDrawable * drawable,
				       guchar   * col,
				       gdouble *  dir,
				       gdouble    color_vary);
static void      clip_poly            (gdouble *  vec,
				       gdouble *  pt,
				       Polygon *  poly,
				       Polygon *  new_poly);
static void      clip_point           (gdouble *  dir,
				       gdouble *  pt,
				       gdouble    x1,
				       gdouble    y1,
				       gdouble    x2,
				       gdouble    y2,
				       Polygon *  poly);
static void      process_poly         (Polygon *  poly,
				       gint       allow_split,
				       GDrawable * drawable,
				       guchar *   col,
				       gint       vary);
static void      render_poly          (Polygon *  poly,
				       GDrawable * drawable,
				       guchar *   col,
				       gdouble    vary);
static void      find_poly_dir        (Polygon *  poly,
				       guchar *   m_gr,
				       guchar *   h_gr,
				       guchar *   v_gr,
				       gdouble *  dir,
				       gdouble *  loc,
				       gint       x1,
				       gint       y1,
				       gint       x2,
				       gint       y2);
static void      find_poly_color      (Polygon *  poly,
				       GDrawable * drawable,
				       guchar *   col,
				       double     vary);
static void      scale_poly           (Polygon *  poly,
				       gdouble    cx,
				       gdouble    cy,
				       gdouble    scale);
static void      fill_poly_color      (Polygon *  poly,
				       GDrawable * drawable,
				       guchar *   col);
static void      fill_poly_image      (Polygon *  poly,
				       GDrawable * drawable,
				       gdouble    vary);
static void      calc_spec_vec        (SpecVec *  vec,
				       gint       xs,
				       gint       ys,
				       gint       xe,
				       gint       ye);
static gdouble   calc_spec_contrib    (SpecVec *  vec,
				       gint       n,
				       gdouble    x,
				       gdouble    y);
static void      convert_segment      (gint       x1,
				       gint       y1,
				       gint       x2,
				       gint       y2,
				       gint       offset,
				       gint *     min,
				       gint *     max);
static void      polygon_add_point    (Polygon *  poly,
				       gdouble    x,
				       gdouble    y);
static gint      polygon_find_center  (Polygon *  poly,
				       gdouble *  x,
				       gdouble *  y);
static void      polygon_translate    (Polygon *  poly,
				       gdouble    tx,
				       gdouble    ty);
static void      polygon_scale        (Polygon *  poly,
				       gdouble    scale);
static gint      polygon_extents      (Polygon *  poly,
				       gdouble *  min_x,
				       gdouble *  min_y,
				       gdouble *  max_x,
				       gdouble *  max_y);
static void      polygon_reset        (Polygon *  poly);

static void      mosaic_close_callback  (GtkWidget *widget,
					 gpointer   data);
static void      mosaic_ok_callback     (GtkWidget *widget,
					 gpointer   data);
static void      mosaic_toggle_update   (GtkWidget *widget,
					 gpointer   data);
static void      mosaic_scale_update    (GtkAdjustment *adjustment,
					 double        *scale_val);
/*
 *  Some static variables
 */
static gdouble std_dev = 1.0;
static gdouble light_x;
static gdouble light_y;
static gdouble scale;
static guchar *h_grad;
static guchar *v_grad;
static guchar *m_grad;
static Vertex *grid;
static gint    grid_rows;
static gint    grid_cols;
static gint    grid_row_pad;
static gint    grid_col_pad;
static gint    grid_multiple;
static gint    grid_rowstride;
static guchar  back[4];
static guchar  fore[4];
static SpecVec vecs[MAX_POINTS];


static MosaicVals mvals =
{
  15.0,        /* tile_size */
  4.0,         /* tile_height */
  1.0,         /* tile_spacing */
  0.65,        /* tile_neatness */
  135,         /* light_dir */
  0.2,         /* color_variation */
  1,           /* antialiasing */
  1,           /* color_averaging  */
  HEXAGONS,    /* tile_type */
  SMOOTH,      /* tile_surface */
  BW           /* grout_color */
};

static MosaicInterface mint =
{
  FALSE,        /* run */
};

GPlugInInfo PLUG_IN_INFO =
{
  NULL,    /* init_proc */
  NULL,    /* quit_proc */
  query,   /* query_proc */
  run,     /* run_proc */
};


MAIN ()

static void
query ()
{
  static GParamDef args[] =
  {
    { PARAM_INT32, "run_mode", "Interactive, non-interactive" },
    { PARAM_IMAGE, "image", "Input image" },
    { PARAM_DRAWABLE, "drawable", "Input drawable" },
    { PARAM_FLOAT, "tile_size", "Average diameter of each tile (in pixels)" },
    { PARAM_FLOAT, "tile_height", "Apparent height of each tile (in pixels)" },
    { PARAM_FLOAT, "tile_spacing", "Inter-tile spacing (in pixels)" },
    { PARAM_FLOAT, "tile_neatness", "Deviation from perfectly formed tiles (0.0 - 1.0)" },
    { PARAM_FLOAT, "light_dir", "Direction of light-source (in degrees)" },
    { PARAM_FLOAT, "color_variation", "Magnitude of random color variations (0.0 - 1.0)" },
    { PARAM_INT32, "antialiasing", "Enables smoother tile output at the cost of speed" },
    { PARAM_INT32, "color_averaging", "Tile color based on average of subsumed pixels" },
    { PARAM_INT32, "tile_type", "Tile geometry: { SQUARES (0), HEXAGONS (1), OCTAGONS (2) }" },
    { PARAM_INT32, "tile_surface", "Surface characteristics: { SMOOTH (0), ROUGH (1) }" },
    { PARAM_INT32, "grout_color", "Grout color (black/white or fore/background): { BW (0), FG_BG (1) }" }
  };
  static GParamDef *return_vals = NULL;

  static int nargs = sizeof (args) / sizeof (args[0]);
  static int nreturn_vals = 0;

  INIT_I18N();

  gimp_install_procedure ("plug_in_mosaic",
			  _("Convert the input drawable into a collection of tiles"),
			  _("Help not yet written for this plug-in"),
			  "Spencer Kimball",
			  "Spencer Kimball & Peter Mattis",
			  "1996",
			  N_("<Image>/Filters/Render/Pattern/Mosaic..."),
			  "RGB*, GRAY*",
			  PROC_PLUG_IN,
			  nargs, nreturn_vals,
			  args, return_vals);
}

static void
run (char    *name,
     int      nparams,
     GParam  *param,
     int     *nreturn_vals,
     GParam **return_vals)
{
  static GParam values[1];
  GDrawable *active_drawable;
  GRunModeType run_mode;
  GStatusType status = STATUS_SUCCESS;

  run_mode = param[0].data.d_int32;

  values[0].type = PARAM_STATUS;
  values[0].data.d_status = status;

  *nreturn_vals = 1;
  *return_vals = values;

  switch (run_mode)
    {
    case RUN_INTERACTIVE:
      INIT_I18N_UI();
      /*  Possibly retrieve data  */
      gimp_get_data ("plug_in_mosaic", &mvals);

      /*  First acquire information with a dialog  */
      if (! mosaic_dialog ())
	return;
      break;

    case RUN_NONINTERACTIVE:
      INIT_I18N();
      /*  Make sure all the arguments are there!  */
      if (nparams != 14)
	status = STATUS_CALLING_ERROR;
      if (status == STATUS_SUCCESS)
	{
	  mvals.tile_size = param[3].data.d_float;
	  mvals.tile_height = param[4].data.d_float;
	  mvals.tile_spacing = param[5].data.d_float;
	  mvals.tile_neatness = param[6].data.d_float;
	  mvals.light_dir = param[7].data.d_float;
	  mvals.color_variation = param[8].data.d_float;
	  mvals.antialiasing = (param[9].data.d_int32) ? TRUE : FALSE;
	  mvals.color_averaging = (param[10].data.d_int32) ? TRUE : FALSE;
	  mvals.tile_type = param[11].data.d_int32;
	  mvals.tile_surface = param[12].data.d_int32;
	  mvals.grout_color = param[13].data.d_int32;
	}
      if (status == STATUS_SUCCESS &&
	  (mvals.tile_type < SQUARES || mvals.tile_type > OCTAGONS))
	status = STATUS_CALLING_ERROR;
      if (status == STATUS_SUCCESS &&
	  (mvals.tile_surface < SMOOTH || mvals.tile_surface > ROUGH))
	status = STATUS_CALLING_ERROR;
      if (status == STATUS_SUCCESS &&
	  (mvals.grout_color < BW || mvals.grout_color > FG_BG))
	status = STATUS_CALLING_ERROR;
      break;

    case RUN_WITH_LAST_VALS:
      /*  Possibly retrieve data  */
      gimp_get_data ("plug_in_mosaic", &mvals);
      break;

    default:
      break;
    }

  /*  Get the active drawable  */
  active_drawable = gimp_drawable_get (param[2].data.d_drawable);

  /*  Create the mosaic  */
  if ((status == STATUS_SUCCESS) &&
      (gimp_drawable_is_rgb (active_drawable->id) || gimp_drawable_is_gray (active_drawable->id)))
    {
      /*  set the tile cache size so that the gaussian blur works well  */
      gimp_tile_cache_ntiles (2 * (MAX (active_drawable->width, active_drawable->height) /
				   gimp_tile_width () + 1));

      /*  run the effect  */
      mosaic (active_drawable);

      /*  If the run mode is interactive, flush the displays  */
      if (run_mode != RUN_NONINTERACTIVE)
	gimp_displays_flush ();

      /*  Store mvals data  */
      if (run_mode == RUN_INTERACTIVE)
	gimp_set_data ("plug_in_mosaic", &mvals, sizeof (MosaicVals));
    }
  else if (status == STATUS_SUCCESS)
    {
      /* gimp_message ("mosaic: cannot operate on indexed color images"); */
      status = STATUS_EXECUTION_ERROR;
    }

  values[0].data.d_status = status;

  gimp_drawable_detach (active_drawable);
}

static void
mosaic (GDrawable *drawable)
{
  gint x1, y1, x2, y2;
  gint alpha;

  /*  Find the mask bounds  */
  gimp_drawable_mask_bounds (drawable->id, &x1, &y1, &x2, &y2);

  /*  progress bar for gradient finding  */
  gimp_progress_init ( _("Finding Edges..."));

  /*  Find the gradients  */
  find_gradients (drawable, std_dev);

  /*  Create the tile geometry grid  */
  switch (mvals.tile_type)
    {
    case SQUARES:
      grid_create_squares (x1, y1, x2, y2);
      break;
    case HEXAGONS:
      grid_create_hexagons (x1, y1, x2, y2);
      break;
    case OCTAGONS:
      grid_create_octagons (x1, y1, x2, y2);
      break;
    default:
      break;
    }

  /*  Deform the tiles based on image content  */
  grid_localize (x1, y1, x2, y2);

  switch (mvals.grout_color)
    {
    case BW:
      fore[0] = fore[1] = fore[2] = 255;
      back[0] = back[1] = back[2] = 0;
      break;
    case FG_BG:
      gimp_palette_get_foreground (&fore[0], &fore[1], &fore[2]);
      gimp_palette_get_background (&back[0], &back[1], &back[2]);
      break;
    }


  alpha = drawable->bpp - 1;
  if (gimp_drawable_has_alpha (drawable->id))
    {
      fore[alpha] = OPAQUE;
      back[alpha] = OPAQUE;
    }

  light_x = -cos (mvals.light_dir * G_PI / 180.0);
  light_y = sin (mvals.light_dir * G_PI / 180.0);
  scale = (mvals.tile_spacing > mvals.tile_size / 2.0) ?
    0.5 : 1.0 - mvals.tile_spacing / mvals.tile_size;

  /*  Progress bar for rendering tiles  */
  gimp_progress_init ( _("Rendering Tiles..."));

  /*  Render the tiles  */
  grid_render (drawable);

  /*  merge the shadow, update the drawable  */
  gimp_drawable_flush (drawable);
  gimp_drawable_merge_shadow (drawable->id, TRUE);
  gimp_drawable_update (drawable->id, x1, y1, (x2 - x1), (y2 - y1));
}

static gint
mosaic_dialog ()
{
  GtkWidget *dlg;
  GtkWidget *label;
  GtkWidget *hbbox;
  GtkWidget *button;
  GtkWidget *toggle;
  GtkWidget *scale;
  GtkWidget *vbox;
  GtkWidget *hbox;
  GtkWidget *logo_box;
  GtkWidget *toggle_vbox;
  GtkWidget *main_hbox;
  GtkWidget *frame;
  GtkWidget *preview;
  GtkWidget *table;
  GtkObject *scale_data;
  GSList *group = NULL;
  guchar *color_cube;
  gchar **argv;
  gint argc;
  gint use_squares = (mvals.tile_type == SQUARES);
  gint use_hexagons = (mvals.tile_type == HEXAGONS);
  gint use_octagons = (mvals.tile_type == OCTAGONS);
  gint y;

  argc = 1;
  argv = g_new (gchar *, 1);
  argv[0] = g_strdup ("mosaic");

  gtk_init (&argc, &argv);
  gtk_rc_parse (gimp_gtkrc ());

  gdk_set_use_xshm (gimp_use_xshm ());
  gtk_preview_set_gamma (gimp_gamma ());
  gtk_preview_set_install_cmap (gimp_install_cmap ());
  color_cube = gimp_color_cube ();
  gtk_preview_set_color_cube (color_cube[0], color_cube[1],
			      color_cube[2], color_cube[3]);

  gtk_widget_set_default_visual (gtk_preview_get_visual ());
  gtk_widget_set_default_colormap (gtk_preview_get_cmap ());

  dlg = gtk_dialog_new ();
  gtk_window_set_title (GTK_WINDOW (dlg), _("Mosaic"));
  gtk_window_position (GTK_WINDOW (dlg), GTK_WIN_POS_MOUSE);
  gtk_signal_connect (GTK_OBJECT (dlg), "destroy",
		      (GtkSignalFunc) mosaic_close_callback,
		      NULL);

  /*  Action area  */
  gtk_container_set_border_width (GTK_CONTAINER (GTK_DIALOG (dlg)->action_area), 2);
  gtk_box_set_homogeneous (GTK_BOX (GTK_DIALOG (dlg)->action_area), FALSE);
  hbbox = gtk_hbutton_box_new ();
  gtk_button_box_set_spacing (GTK_BUTTON_BOX (hbbox), 4);
  gtk_box_pack_end (GTK_BOX (GTK_DIALOG (dlg)->action_area), hbbox, FALSE, FALSE, 0);
  gtk_widget_show (hbbox);
 
  button = gtk_button_new_with_label (_("OK"));
  GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT);
  gtk_signal_connect (GTK_OBJECT (button), "clicked",
		      (GtkSignalFunc) mosaic_ok_callback,
		      dlg);
  gtk_box_pack_start (GTK_BOX (hbbox), button, FALSE, FALSE, 0);
  gtk_widget_grab_default (button);
  gtk_widget_show (button);

  button = gtk_button_new_with_label (_("Cancel"));
  GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT);
  gtk_signal_connect_object (GTK_OBJECT (button), "clicked",
			     (GtkSignalFunc) gtk_widget_destroy,
			     GTK_OBJECT (dlg));
  gtk_box_pack_start (GTK_BOX (hbbox), button, FALSE, FALSE, 0);
  gtk_widget_show (button);

  /*  The main hbox -- splits the scripts and the info vbox  */
  main_hbox = gtk_hbox_new (FALSE, 5);
  gtk_container_border_width (GTK_CONTAINER (main_hbox), 10);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dlg)->vbox), main_hbox, TRUE, TRUE, 0);

  /*  The vbox for first column of options  */
  vbox = gtk_vbox_new (FALSE, 5);
  gtk_box_pack_start (GTK_BOX (main_hbox), vbox, TRUE, TRUE, 0);

  /*  The logo frame & drawing area  */
  hbox = gtk_hbox_new (FALSE, 5);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, TRUE, 0);

  logo_box = gtk_vbox_new (FALSE, 0);
  gtk_box_pack_start (GTK_BOX (hbox), logo_box, FALSE, FALSE, 0);

  frame = gtk_frame_new (NULL);
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_IN);
  gtk_box_pack_start (GTK_BOX (logo_box), frame, FALSE, FALSE, 0);

  preview = gtk_preview_new (GTK_PREVIEW_COLOR);
  gtk_preview_size (GTK_PREVIEW (preview), logo_width, logo_height);

  for (y = 0; y < logo_height; y++)
    gtk_preview_draw_row (GTK_PREVIEW (preview),
			  logo_data + y * logo_width * logo_bpp,
			  0, y, logo_width);

  gtk_container_add (GTK_CONTAINER (frame), preview);
  gtk_widget_show (preview);
  gtk_widget_show (frame);
  gtk_widget_show (logo_box);

  /*  the vertical box and its toggle buttons  */
  frame = gtk_frame_new ("Options");
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_ETCHED_IN);
  gtk_box_pack_start (GTK_BOX (hbox), frame, FALSE, TRUE, 0);
  toggle_vbox = gtk_vbox_new (FALSE, 5);
  gtk_container_border_width (GTK_CONTAINER (toggle_vbox), 5);
  gtk_container_add (GTK_CONTAINER (frame), toggle_vbox);

  toggle = gtk_check_button_new_with_label ( _("Antialiasing"));
  gtk_box_pack_start (GTK_BOX (toggle_vbox), toggle, FALSE, FALSE, 0);
  gtk_signal_connect (GTK_OBJECT (toggle), "toggled",
		      (GtkSignalFunc) mosaic_toggle_update,
		      &mvals.antialiasing);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (toggle), mvals.antialiasing);
  gtk_widget_show (toggle);

  toggle = gtk_check_button_new_with_label ( _("Color Averaging"));
  gtk_box_pack_start (GTK_BOX (toggle_vbox), toggle, FALSE, FALSE, 0);
  gtk_signal_connect (GTK_OBJECT (toggle), "toggled",
		      (GtkSignalFunc) mosaic_toggle_update,
		      &mvals.color_averaging);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (toggle), mvals.color_averaging);
  gtk_widget_show (toggle);

  toggle = gtk_check_button_new_with_label ( _("Pitted Surfaces"));
  gtk_box_pack_start (GTK_BOX (toggle_vbox), toggle, FALSE, FALSE, 0);
  gtk_signal_connect (GTK_OBJECT (toggle), "toggled",
		      (GtkSignalFunc) mosaic_toggle_update,
		      &mvals.tile_surface);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (toggle), (mvals.tile_surface == ROUGH));
  gtk_widget_show (toggle);

  toggle = gtk_check_button_new_with_label ( _("FG/BG Lighting"));
  gtk_box_pack_start (GTK_BOX (toggle_vbox), toggle, FALSE, FALSE, 0);
  gtk_signal_connect (GTK_OBJECT (toggle), "toggled",
		      (GtkSignalFunc) mosaic_toggle_update,
		      &mvals.grout_color);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (toggle), (mvals.grout_color == FG_BG));
  gtk_widget_show (toggle);

  gtk_widget_show (toggle_vbox);
  gtk_widget_show (frame);
  gtk_widget_show (hbox);

  /*  tiling primitive  */
  frame = gtk_frame_new ( _("Tiling Primitives"));
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_ETCHED_IN);
  gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, TRUE, 0);
  toggle_vbox = gtk_vbox_new (FALSE, 5);
  gtk_container_border_width (GTK_CONTAINER (toggle_vbox), 5);
  gtk_container_add (GTK_CONTAINER (frame), toggle_vbox);

  toggle = gtk_radio_button_new_with_label (group, _("Squares"));
  group = gtk_radio_button_group (GTK_RADIO_BUTTON (toggle));
  gtk_box_pack_start (GTK_BOX (toggle_vbox), toggle, FALSE, FALSE, 0);
  gtk_signal_connect (GTK_OBJECT (toggle), "toggled",
		      (GtkSignalFunc) mosaic_toggle_update,
		      &use_squares);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (toggle), use_squares);
  gtk_widget_show (toggle);

  toggle = gtk_radio_button_new_with_label (group, _("Hexagons"));
  group = gtk_radio_button_group (GTK_RADIO_BUTTON (toggle));
  gtk_box_pack_start (GTK_BOX (toggle_vbox), toggle, FALSE, FALSE, 0);
  gtk_signal_connect (GTK_OBJECT (toggle), "toggled",
		      (GtkSignalFunc) mosaic_toggle_update,
		      &use_hexagons);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (toggle), use_hexagons);
  gtk_widget_show (toggle);

  toggle = gtk_radio_button_new_with_label (group, _("Octagons & Squares"));
  group = gtk_radio_button_group (GTK_RADIO_BUTTON (toggle));
  gtk_box_pack_start (GTK_BOX (toggle_vbox), toggle, FALSE, FALSE, 0);
  gtk_signal_connect (GTK_OBJECT (toggle), "toggled",
		      (GtkSignalFunc) mosaic_toggle_update,
		      &use_octagons);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (toggle), use_octagons);
  gtk_widget_show (toggle);

  gtk_widget_show (toggle_vbox);
  gtk_widget_show (frame);
  gtk_widget_show (vbox);

  /*  parameter settings  */
  frame = gtk_frame_new ( _("Parameter Settings"));
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_ETCHED_IN);
  gtk_box_pack_start (GTK_BOX (main_hbox), frame, TRUE, TRUE, 0);
  table = gtk_table_new (6, 2, FALSE);
  gtk_container_border_width (GTK_CONTAINER (table), 5);
  gtk_container_add (GTK_CONTAINER (frame), table);

  label = gtk_label_new ( _("Tile Size"));
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gtk_table_attach (GTK_TABLE (table), label, 0, 1, 0, 1, GTK_FILL, 0, 5, 0);
  scale_data = gtk_adjustment_new (mvals.tile_size, 5.0, 100.0, 1.0, 1.0, 0.0);
  scale = gtk_hscale_new (GTK_ADJUSTMENT (scale_data));
  gtk_widget_set_usize (scale, SCALE_WIDTH, 0);
  gtk_table_attach (GTK_TABLE (table), scale, 1, 2, 0, 1, GTK_FILL, 0, 0, 0);
  gtk_scale_set_value_pos (GTK_SCALE (scale), GTK_POS_TOP);
  gtk_range_set_update_policy (GTK_RANGE (scale), GTK_UPDATE_DELAYED);
  gtk_signal_connect (GTK_OBJECT (scale_data), "value_changed",
		      (GtkSignalFunc) mosaic_scale_update,
		      &mvals.tile_size);
  gtk_widget_show (label);
  gtk_widget_show (scale);

  label = gtk_label_new ( _("Tile Height"));
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gtk_table_attach (GTK_TABLE (table), label, 0, 1, 1, 2, GTK_FILL, 0, 5, 0);
  scale_data = gtk_adjustment_new (mvals.tile_height, 1.0, 50.0, 1.0, 1.0, 0.0);
  scale = gtk_hscale_new (GTK_ADJUSTMENT (scale_data));
  gtk_widget_set_usize (scale, SCALE_WIDTH, 0);
  gtk_table_attach (GTK_TABLE (table), scale, 1, 2, 1, 2, GTK_FILL, 0, 0, 0);
  gtk_scale_set_value_pos (GTK_SCALE (scale), GTK_POS_TOP);
  gtk_range_set_update_policy (GTK_RANGE (scale), GTK_UPDATE_DELAYED);
  gtk_signal_connect (GTK_OBJECT (scale_data), "value_changed",
		      (GtkSignalFunc) mosaic_scale_update,
		      &mvals.tile_height);
  gtk_widget_show (label);
  gtk_widget_show (scale);

  label = gtk_label_new ( _("Tile Spacing"));
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gtk_table_attach (GTK_TABLE (table), label, 0, 1, 2, 3, GTK_FILL, 0, 5, 0);
  scale_data = gtk_adjustment_new (mvals.tile_spacing, 1.0, 50.0, 1.0, 1.0, 0.0);
  scale = gtk_hscale_new (GTK_ADJUSTMENT (scale_data));
  gtk_widget_set_usize (scale, SCALE_WIDTH, 0);
  gtk_table_attach (GTK_TABLE (table), scale, 1, 2, 2, 3, GTK_FILL, 0, 0, 0);
  gtk_scale_set_value_pos (GTK_SCALE (scale), GTK_POS_TOP);
  gtk_range_set_update_policy (GTK_RANGE (scale), GTK_UPDATE_DELAYED);
  gtk_signal_connect (GTK_OBJECT (scale_data), "value_changed",
		      (GtkSignalFunc) mosaic_scale_update,
		      &mvals.tile_spacing);
  gtk_widget_show (label);
  gtk_widget_show (scale);

  label = gtk_label_new ( _("Tile Neatness"));
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gtk_table_attach (GTK_TABLE (table), label, 0, 1, 3, 4, GTK_FILL, 0, 5, 0);
  scale_data = gtk_adjustment_new (mvals.tile_neatness, 0.0, 1.0, 0.01, 0.01, 0.0);
  scale = gtk_hscale_new (GTK_ADJUSTMENT (scale_data));
  gtk_widget_set_usize (scale, SCALE_WIDTH, 0);
  gtk_table_attach (GTK_TABLE (table), scale, 1, 2, 3, 4, GTK_FILL, 0, 0, 0);
  gtk_scale_set_value_pos (GTK_SCALE (scale), GTK_POS_TOP);
  gtk_scale_set_digits (GTK_SCALE (scale), 2);
  gtk_range_set_update_policy (GTK_RANGE (scale), GTK_UPDATE_DELAYED);
  gtk_signal_connect (GTK_OBJECT (scale_data), "value_changed",
		      (GtkSignalFunc) mosaic_scale_update,
		      &mvals.tile_neatness);
  gtk_widget_show (label);
  gtk_widget_show (scale);

  label = gtk_label_new ( _("Light Direction"));
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gtk_table_attach (GTK_TABLE (table), label, 0, 1, 4, 5, GTK_FILL, 0, 5, 0);
  scale_data = gtk_adjustment_new (mvals.light_dir, 0.0, 360.0, 5.0, 5.0, 0.0);
  scale = gtk_hscale_new (GTK_ADJUSTMENT (scale_data));
  gtk_widget_set_usize (scale, SCALE_WIDTH, 0);
  gtk_table_attach (GTK_TABLE (table), scale, 1, 2, 4, 5, GTK_FILL, 0, 0, 0);
  gtk_scale_set_value_pos (GTK_SCALE (scale), GTK_POS_TOP);
  gtk_range_set_update_policy (GTK_RANGE (scale), GTK_UPDATE_DELAYED);
  gtk_signal_connect (GTK_OBJECT (scale_data), "value_changed",
		      (GtkSignalFunc) mosaic_scale_update,
		      &mvals.light_dir);
  gtk_widget_show (label);
  gtk_widget_show (scale);

  label = gtk_label_new ( _("Color Variation"));
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gtk_table_attach (GTK_TABLE (table), label, 0, 1, 5, 6, GTK_FILL, 0, 5, 0);
  scale_data = gtk_adjustment_new (mvals.color_variation, 0.0, 1.0, 0.01, 0.01, 0.0);
  scale = gtk_hscale_new (GTK_ADJUSTMENT (scale_data));
  gtk_widget_set_usize (scale, SCALE_WIDTH, 0);
  gtk_table_attach (GTK_TABLE (table), scale, 1, 2, 5, 6, GTK_FILL, 0, 0, 0);
  gtk_scale_set_value_pos (GTK_SCALE (scale), GTK_POS_TOP);
  gtk_scale_set_digits (GTK_SCALE (scale), 2);
  gtk_range_set_update_policy (GTK_RANGE (scale), GTK_UPDATE_DELAYED);
  gtk_signal_connect (GTK_OBJECT (scale_data), "value_changed",
		      (GtkSignalFunc) mosaic_scale_update,
		      &mvals.color_variation);
  gtk_widget_show (label);
  gtk_widget_show (scale);

  gtk_widget_show (frame);
  gtk_widget_show (table);
  gtk_widget_show (main_hbox);
  gtk_widget_show (dlg);

  gtk_main ();
  gdk_flush ();

  /*  determine tile type  */
  if (use_squares)
    mvals.tile_type = SQUARES;
  else if (use_hexagons)
    mvals.tile_type = HEXAGONS;
  else if (use_octagons)
    mvals.tile_type = OCTAGONS;

  return mint.run;
}


/*
 *  Gradient finding machinery
 */

static void
find_gradients (GDrawable *drawable,
		gdouble    std_dev)
{
  GPixelRgn src_rgn;
  GPixelRgn dest_rgn;
  gint bytes;
  gint width, height;
  gint i, j;
  guchar *gr, * dh, * dv;
  gint hmax, vmax;
  gint row, rows;
  gint ith_row;
  gint x1, y1, x2, y2;

  /*  find the mask bounds  */
  gimp_drawable_mask_bounds (drawable->id, &x1, &y1, &x2, &y2);
  width = (x2 - x1);
  height = (y2 - y1);
  bytes = drawable->bpp;

  /*  allocate the gradient maps  */
  h_grad = (guchar *) malloc (width * height);
  v_grad = (guchar *) malloc (width * height);
  m_grad = (guchar *) malloc (width * height);

  /*  Calculate total number of rows to be processed  */
  rows = width * 2 + height * 2;
  ith_row = rows / 256;
  if (!ith_row) ith_row = 1;
  row = 0;

  /*  Get the horizontal derivative  */
  gimp_pixel_rgn_init (&src_rgn, drawable, x1, y1, width, height, FALSE, FALSE);
  gimp_pixel_rgn_init (&dest_rgn, drawable, x1, y1, width, height, TRUE, TRUE);
  gaussian_deriv (&src_rgn, &dest_rgn, HORIZONTAL, std_dev, &row, rows, ith_row);

  gimp_pixel_rgn_init (&src_rgn, drawable, x1, y1, width, height, FALSE, TRUE);
  dest_rgn.x = dest_rgn.y = 0;
  dest_rgn.w = width;
  dest_rgn.h = height;
  dest_rgn.bpp = 1;
  dest_rgn.rowstride = width;
  dest_rgn.data = h_grad;
  find_max_gradient (&src_rgn, &dest_rgn);

  /*  Get the vertical derivative  */
  gimp_pixel_rgn_init (&src_rgn, drawable, x1, y1, width, height, FALSE, FALSE);
  gimp_pixel_rgn_init (&dest_rgn, drawable, x1, y1, width, height, TRUE, TRUE);
  gaussian_deriv (&src_rgn, &dest_rgn, VERTICAL, std_dev, &row, rows, ith_row);

  gimp_pixel_rgn_init (&src_rgn, drawable, x1, y1, width, height, FALSE, TRUE);
  dest_rgn.x = dest_rgn.y = 0;
  dest_rgn.w = width;
  dest_rgn.h = height;
  dest_rgn.bpp = 1;
  dest_rgn.rowstride = width;
  dest_rgn.data = v_grad;
  find_max_gradient (&src_rgn, &dest_rgn);

  gimp_progress_update (1.0);

  /*  fill in the gradient map  */
  gr = m_grad;
  dh = h_grad;
  dv = v_grad;

  for (i = 0; i < height; i++)
    {
      for (j = 0; j < width; j++, dh++, dv++, gr++)
	{
	  /*  Find the gradient  */
	  if (!j || !i || (j == width - 1) || (i == height - 1))
	    *gr = MAG_THRESHOLD;
	  else {
	    hmax = *dh - 128;
	    vmax = *dv - 128;
	    
	    *gr = (guchar)sqrt (SQR (hmax) + SQR (hmax));
	  }

	}
    }
}


static void
find_max_gradient (GPixelRgn *src_rgn,
		   GPixelRgn *dest_rgn)
{
  guchar *s, *d, *s_iter, *s_end;
  gpointer pr;
  gint i, j;
  gint val;
  gint max;

  /*  Find the maximum value amongst intensity channels  */
  pr = gimp_pixel_rgns_register (2, src_rgn, dest_rgn);
  while (pr)
    {
      s = src_rgn->data;
      d = dest_rgn->data;

      for (i = 0; i < src_rgn->h; i++)
	{
	  for (j = 0; j < src_rgn->w; j++)
	    {
	      max = 0;
#ifndef SLOW_CODE
#define ABSVAL(x) ((x) >= 0 ? (x) : -(x))

	      for(s_iter = s, s_end = s + src_rgn->bpp;
		  s_iter < s_end; s_iter++) {
		val = *s;
		if(ABSVAL(val) > ABSVAL(max))
		  max = val;
	      }
	      *d++ = max;
#else
	      for (b = 0; b < src_rgn->bpp; b++)
		{
		  val = (gint) s[b] - 128;
		  if (abs (val) > abs (max))
		    max = val;
		}
	      *d++ = (max + 128);
#endif
	      s += src_rgn->bpp;
	    }

	  s += (src_rgn->rowstride - src_rgn->w * src_rgn->bpp);
	  d += (dest_rgn->rowstride - dest_rgn->w);
	}

      pr = gimp_pixel_rgns_process (pr);
    }
}


/*********************************************/
/*   Functions for gaussian convolutions     */
/*********************************************/


static void
gaussian_deriv (GPixelRgn *src_rgn,
		GPixelRgn *dest_rgn,
		gint       type,
		gdouble    std_dev,
		gint      *prog,
		gint       max_prog,
		gint       ith_prog)
{
  guchar *dest, *dp;
  guchar *src, *sp, *s;
  guchar *data;
  gint *buf, *b;
  gint chan;
  gint i, row, col;
  gint start, end;
  gint curve_array [9];
  gint sum_array [9];
  gint * curve;
  gint * sum;
  gint bytes;
  gint val;
  gint total;
  gint length;
  gint initial_p[4], initial_m[4];
  gint x1, y1, x2, y2;

  /*  get the mask bounds  */
  gimp_drawable_mask_bounds (src_rgn->drawable->id, &x1, &y1, &x2, &y2);
  bytes = src_rgn->bpp;

  /*  allocate buffers for get/set pixel region rows/cols  */
  length = MAX (src_rgn->w, src_rgn->h) * bytes;
  data = (guchar *) malloc (length * 2);
  src = data;
  dest = data + length;

#ifdef UNOPTIMIZED_CODE
  length = 3;    /*  static for speed  */
#else
  /* badhack :) */
# define length 3
#endif

  /*  initialize  */
  curve = curve_array + length;
  sum = sum_array + length;
  buf = (gint *) malloc (sizeof (gint) * MAX ((x2 - x1), (y2 - y1)) * bytes);

  if (type == VERTICAL)
    {
      make_curve_d (curve, sum, std_dev, length);
      total = sum[0] * -2;
    }
  else
    {
      make_curve (curve, sum, std_dev, length);
      total = sum[length] + curve[length];
    }

  for (col = x1; col < x2; col++)
    {
      gimp_pixel_rgn_get_col (src_rgn, src, col, y1, (y2 - y1));

      sp = src;
      dp = dest;
      b = buf;

      for (chan = 0; chan < bytes; chan++)
	{
	  initial_p[chan] = sp[chan];
	  initial_m[chan] = sp[(y2 - y1 - 1) * bytes + chan];
	}

      for (row = y1; row < y2; row++)
	{
	  start = ((row - y1) < length) ? (y1 - row) : -length;
	  end = ((y2 - row - 1) < length) ? (y2 - row - 1) : length;

	  for (chan = 0; chan < bytes; chan++)
	    {
	      s = sp + (start * bytes) + chan;
	      val = 0;
	      i = start;

	      if (start != -length)
		val += initial_p[chan] * (sum[start] - sum[-length]);

	      while (i <= end)
		{
		  val += *s * curve[i++];
		  s += bytes;
		}

	      if (end != length)
		val += initial_m[chan] * (sum[length] + curve[length] - sum[end+1]);

	      *b++ = val / total;
	    }

	  sp += bytes;
	}

      b = buf;
      if (type == VERTICAL)
	for (row = y1; row < y2; row++)
	  {
	    for (chan = 0; chan < bytes; chan++)
	      {
		b[chan] = b[chan] + 128;
		if (b[chan] > 255)
		  dp[chan] = 255;
		else if (b[chan] < 0)
		  dp[chan] = 0;
		else
		  dp[chan] = b[chan];
	      }
	    b += bytes;
	    dp += bytes;
	  }
      else
	for (row = y1; row < y2; row++)
	  {
	    for (chan = 0; chan < bytes; chan++)
	      {
		if (b[chan] > 255)
		  dp[chan] = 255;
		else if (b[chan] < 0)
		  dp[chan] = 0;
		else
		  dp[chan] = b[chan];
	      }
	    b += bytes;
	    dp += bytes;
	  }

      gimp_pixel_rgn_set_col (dest_rgn, dest, col, y1, (y2 - y1));

      if (! ((*prog)++ % ith_prog))
	gimp_progress_update ((double) *prog / (double) max_prog);
    }

  if (type == HORIZONTAL)
    {
      make_curve_d (curve, sum, std_dev, length);
      total = sum[0] * -2;
    }
  else
    {
      make_curve (curve, sum, std_dev, length);
      total = sum[length] + curve[length];
    }

  for (row = y1; row < y2; row++)
    {
      gimp_pixel_rgn_get_row (dest_rgn, src, x1, row, (x2 - x1));

      sp = src;
      dp = dest;
      b = buf;

      for (chan = 0; chan < bytes; chan++)
	{
	  initial_p[chan] = sp[chan];
	  initial_m[chan] = sp[(x2 - x1 - 1) * bytes + chan];
	}

      for (col = x1; col < x2; col++)
	{
	  start = ((col - x1) < length) ? (x1 - col) : -length;
	  end = ((x2 - col - 1) < length) ? (x2 - col - 1) : length;

	  for (chan = 0; chan < bytes; chan++)
	    {
	      s = sp + (start * bytes) + chan;
	      val = 0;
	      i = start;

	      if (start != -length)
		val += initial_p[chan] * (sum[start] - sum[-length]);

	      while (i <= end)
		{
		  val += *s * curve[i++];
		  s += bytes;
		}

	      if (end != length)
		val += initial_m[chan] * (sum[length] + curve[length] - sum[end+1]);

	      *b++ = val / total;
	    }

	  sp += bytes;
	}

      b = buf;
      if (type == HORIZONTAL)
	for (col = x1; col < x2; col++)
	  {
	    for (chan = 0; chan < bytes; chan++)
	      {
		b[chan] = b[chan] + 128;
		if (b[chan] > 255)
		  dp[chan] = 255;
		else if (b[chan] < 0)
		  dp[chan] = 0;
		else
		  dp[chan] = b[chan];
	      }
	    b += bytes;
	    dp += bytes;
	  }
      else
	for (col = x1; col < x2; col++)
	  {
	    for (chan = 0; chan < bytes; chan++)
	      {
		if (b[chan] > 255)
		  dp[chan] = 255;
		else if (b[chan] < 0)
		  dp[chan] = 0;
		else
		  dp[chan] = b[chan];
	      }
	    b += bytes;
	    dp += bytes;
	  }

      gimp_pixel_rgn_set_row (dest_rgn, dest, x1, row, (x2 - x1));

      if (! ((*prog)++ % ith_prog))
	gimp_progress_update ((double) *prog / (double) max_prog);
    }

  free (buf);
  free (data);
#ifndef UNOPTIMIZED_CODE
  /* end bad hack */
#undef length
#endif
}

/*
 * The equations: g(r) = exp (- r^2 / (2 * sigma^2))
 *                   r = sqrt (x^2 + y ^2)
 */

static void
make_curve (gint    *curve,
	    gint    *sum,
	    gdouble  sigma,
	    gint     length)
{
  gdouble sigma2;
  gint i;

  sigma2 = sigma * sigma;

  curve[0] = 255;
  for (i = 1; i <= length; i++)
    {
      curve[i] = (gint) (exp (- (i * i) / (2 * sigma2)) * 255);
      curve[-i] = curve[i];
    }

  sum[-length] = 0;
  for (i = -length+1; i <= length; i++)
    sum[i] = sum[i-1] + curve[i-1];
}


/*
 * The equations: d_g(r) = -r * exp (- r^2 / (2 * sigma^2)) / sigma^2
 *                   r = sqrt (x^2 + y ^2)
 */

static void
make_curve_d (gint    *curve,
	      gint    *sum,
	      gdouble  sigma,
	      gint     length)
{
  gdouble sigma2;
  gint i;

  sigma2 = sigma * sigma;

  curve[0] = 0;
  for (i = 1; i <= length; i++)
    {
      curve[i] = (gint) ((i * exp (- (i * i) / (2 * sigma2)) / sigma2) * 255);
      curve[-i] = -curve[i];
    }

  sum[-length] = 0;
  sum[0] = 0;
  for (i = 1; i <= length; i++)
    {
      sum[-length + i] = sum[-length + i - 1] + curve[-length + i - 1];
      sum[i] = sum[i - 1] + curve[i - 1];
    }
}


/*********************************************/
/*   Functions for grid manipulation         */
/*********************************************/

static gdouble
fp_rand (gdouble val)
{
  gdouble rand_val;

  rand_val = (gdouble) rand () / (gdouble) (RAND_MAX - 1);
  return rand_val * val;
}


static void
grid_create_squares (gint x1,
		     gint y1,
		     gint x2,
		     gint y2)
{
  gint rows, cols;
  gint width, height;
  gint i, j;
  gint size = (gint) mvals.tile_size;
  Vertex *pt;

  width = x2 - x1;
  height = y2 - y1;
  rows = (height + size - 1) / size;
  cols = (width + size - 1) / size;

  grid = (Vertex *) malloc (sizeof (Vertex) * (cols + 2) * (rows + 2));
  grid += (cols + 2) + 1;

  for (i = -1; i <= rows; i++)
    for (j = -1; j <= cols; j++)
      {
	pt = grid + (i * (cols + 2) + j);

	pt->x = x1 + j * size + size/2;
	pt->y = y1 + i * size + size/2;
      }

  grid_rows = rows;
  grid_cols = cols;
  grid_row_pad = 1;
  grid_col_pad = 1;
  grid_multiple = 1;
  grid_rowstride = cols + 2;
}


static void
grid_create_hexagons (gint x1,
		      gint y1,
		      gint x2,
		      gint y2)
{
  gint rows, cols;
  gint width, height;
  gint i, j;
  gdouble hex_l1, hex_l2, hex_l3;
  gdouble hex_width;
  gdouble hex_height;
  Vertex *pt;

  width = x2 - x1;
  height = y2 - y1;
  hex_l1 = mvals.tile_size / 2.0;
  hex_l2 = hex_l1 * 2.0 / sqrt (3.0);
  hex_l3 = hex_l1 / sqrt (3.0);
  hex_width = 6 * hex_l1 / sqrt (3.0);
  hex_height = mvals.tile_size;
  rows = ((height + hex_height - 1) / hex_height);
  cols = ((width + hex_width * 2 - 1) / hex_width);

  grid = (Vertex *) malloc (sizeof (Vertex) * (cols + 2) * 4 * (rows + 2));
  grid += (cols + 2) * 4 + 4;

  for (i = -1; i <= rows; i++)
    for (j = -1; j <= cols; j++)
      {
	pt = grid + (i * (cols + 2) * 4 + j * 4);

	pt[0].x = x1 + hex_width * j + hex_l3;
	pt[0].y = y1 + hex_height * i;
	pt[1].x = pt[0].x + hex_l2;
	pt[1].y = pt[0].y;
	pt[2].x = pt[1].x + hex_l3;
	pt[2].y = pt[1].y + hex_l1;
	pt[3].x = pt[0].x - hex_l3;
	pt[3].y = pt[0].y + hex_l1;
      }

  grid_rows = rows;
  grid_cols = cols;
  grid_row_pad = 1;
  grid_col_pad = 1;
  grid_multiple = 4;
  grid_rowstride = (cols + 2) * 4;
}


static void
grid_create_octagons (gint x1,
		      gint y1,
		      gint x2,
		      gint y2)
{
  gint rows, cols;
  gint width, height;
  gint i, j;
  gdouble ts, side, leg;
  gdouble oct_size;
  Vertex *pt;

  width = x2 - x1;
  height = y2 - y1;

  ts = mvals.tile_size;
  side = ts / (sqrt (2.0) + 1.0);
  leg = side * sqrt (2.0) * 0.5;
  oct_size = ts + side;

  rows = ((height + oct_size - 1) / oct_size);
  cols = ((width + oct_size * 2 - 1) / oct_size);

  grid = (Vertex *) malloc (sizeof (Vertex) * (cols + 2) * 8 * (rows + 2));
  grid += (cols + 2) * 8 + 8;

  for (i = -1; i < rows + 1; i++)
    for (j = -1; j < cols + 1; j++)
      {
	pt = grid + (i * (cols + 2) * 8 + j * 8);

	pt[0].x = x1 + oct_size * j;
	pt[0].y = y1 + oct_size * i;
	pt[1].x = pt[0].x + side;
	pt[1].y = pt[0].y;
	pt[2].x = pt[0].x + leg + side;
	pt[2].y = pt[0].y + leg;
	pt[3].x = pt[2].x;
	pt[3].y = pt[0].y + leg + side;
	pt[4].x = pt[1].x;
	pt[4].y = pt[0].y + 2 * leg + side;
	pt[5].x = pt[0].x;
	pt[5].y = pt[4].y;
	pt[6].x = pt[0].x - leg;
	pt[6].y = pt[3].y;
	pt[7].x = pt[6].x;
	pt[7].y = pt[2].y;
      }

  grid_rows = rows;
  grid_cols = cols;
  grid_row_pad = 1;
  grid_col_pad = 1;
  grid_multiple = 8;
  grid_rowstride = (cols + 2) * 8;
}


static void
grid_localize (gint x1,
	       gint y1,
	       gint x2,
	       gint y2)
{
  gint width, height;
  gint i, j;
  gint k, l;
  gint x3, y3, x4, y4;
  gint size;
  gint max_x, max_y;
  gint max;
  guchar * data;
  gdouble rand_localize;
  Vertex * pt;

  width = x2 - x1;
  height = y2 - y1;
  size = (gint) mvals.tile_size;
  rand_localize = size * (1.0 - mvals.tile_neatness);

  for (i = -grid_row_pad; i < grid_rows + grid_row_pad; i++)
    for (j = -grid_col_pad * grid_multiple; j < (grid_cols + grid_col_pad) * grid_multiple; j++)
      {
	pt = grid + (i * grid_rowstride + j);

	max_x = pt->x + (gint) (fp_rand (rand_localize) - rand_localize/2.0);
	max_y = pt->y + (gint) (fp_rand (rand_localize) - rand_localize/2.0);

	x3 = pt->x - (gint) (rand_localize / 2.0);
	y3 = pt->y - (gint) (rand_localize / 2.0);
	x4 = x3 + (gint) rand_localize;
	y4 = y3 + (gint) rand_localize;

	if (x3 < x1) x3 = x1;
	else if (x3 >= x2) x3 = (x2 - 1);
	if (y3 < y1) y3 = y1;
	else if (y3 >= y2) y3 = (y2 - 1);
	if (x4 >= x2) x4 = (x2 - 1);
	else if (x4 < x1) x4 = x1;
	if (y4 >= y2) y4 = (y2 - 1);
	else if (y4 < y1) y4 = y1;

	max = *(m_grad + (y3 - y1) * width + (x3 - x1));
	data = m_grad + width * (y3 - y1);

	for (k = y3; k <= y4; k++)
	  {
	    for (l = x3; l <= x4; l++)
	      {
		if (data[l] > max)
		  {
		    max_y = k;
		    max_x = l;
		    max = data[(l - x1)];
		  }
	      }
	    data += width;
	  }

	pt->x = max_x;
	pt->y = max_y;
      }
}


static void
grid_render (GDrawable *drawable)
{
  GPixelRgn src_rgn;
  gint i, j, k;
  gint x1, y1, x2, y2;
  guchar *data;
  guchar col[4];
  gint bytes;
  gint size, frac_size;
  gint count;
  gint index;
  gint vary;
  Polygon poly;
  gpointer pr;

  gimp_drawable_mask_bounds (drawable->id, &x1, &y1, &x2, &y2);
  bytes = drawable->bpp;

  /*  Fill the image with the background color  */
  gimp_pixel_rgn_init (&src_rgn, drawable, x1, y1, (x2 - x1), (y2 - y1), TRUE, TRUE);
  for (pr = gimp_pixel_rgns_register (1, &src_rgn); pr != NULL; pr = gimp_pixel_rgns_process (pr))
    {
      size = src_rgn.w * src_rgn.h;
      data = src_rgn.data;

      while (size--)
	for (i = 0; i < bytes; i++)
	  *data++ = back[i];
    }

  size = (grid_rows + grid_row_pad) * (grid_cols + grid_col_pad);
  frac_size = (gint) (size * mvals.color_variation);
  count = 0;

  for (i = -grid_row_pad; i < grid_rows; i++)
    for (j = -grid_col_pad; j < grid_cols; j++)
      {
	vary = ((rand () % size) < frac_size) ? 1 : 0;

	index = i * grid_rowstride + j * grid_multiple;

	switch (mvals.tile_type)
	  {
	  case SQUARES:
	    polygon_reset (&poly);
	    polygon_add_point (&poly,
			       grid[index].x,
			       grid[index].y);
	    polygon_add_point (&poly,
			       grid[index + 1].x,
			       grid[index + 1].y);
	    polygon_add_point (&poly,
			       grid[index + grid_rowstride + 1].x,
			       grid[index + grid_rowstride + 1].y);
	    polygon_add_point (&poly,
			       grid[index + grid_rowstride].x,
			       grid[index + grid_rowstride].y);

	    process_poly (&poly, TRUE, drawable, col, vary);
	    break;

	  case HEXAGONS:
	    /*  The main hexagon  */
	    polygon_reset (&poly);
	    polygon_add_point (&poly,
			       grid[index].x,
			       grid[index].y);
	    polygon_add_point (&poly,
			       grid[index + 1].x,
			       grid[index + 1].y);
	    polygon_add_point (&poly,
			       grid[index + 2].x,
			       grid[index + 2].y);
	    polygon_add_point (&poly,
			       grid[index + grid_rowstride + 1].x,
			       grid[index + grid_rowstride + 1].y);
	    polygon_add_point (&poly,
			       grid[index + grid_rowstride].x,
			       grid[index + grid_rowstride].y);
	    polygon_add_point (&poly,
			       grid[index + 3].x,
			       grid[index + 3].y);
	    process_poly (&poly, TRUE, drawable, col, vary);

	    /*  The auxillary hexagon  */
	    polygon_reset (&poly);
	    polygon_add_point (&poly,
			       grid[index + 2].x,
			       grid[index + 2].y);
	    polygon_add_point (&poly,
			       grid[index + grid_multiple * 2 - 1].x,
			       grid[index + grid_multiple * 2 - 1].y);
	    polygon_add_point (&poly,
			       grid[index + grid_rowstride + grid_multiple].x,
			       grid[index + grid_rowstride + grid_multiple].y);
	    polygon_add_point (&poly,
			       grid[index + grid_rowstride + grid_multiple + 3].x,
			       grid[index + grid_rowstride + grid_multiple + 3].y);
	    polygon_add_point (&poly,
			       grid[index + grid_rowstride + 2].x,
			       grid[index + grid_rowstride + 2].y);
	    polygon_add_point (&poly,
			       grid[index + grid_rowstride + 1].x,
			       grid[index + grid_rowstride + 1].y);
	    process_poly (&poly, TRUE, drawable, col, vary);
	    break;

	  case OCTAGONS:
	    /*  The main octagon  */
	    polygon_reset (&poly);
	    for (k = 0; k < 8; k++)
	      polygon_add_point (&poly,
				 grid[index + k].x,
				 grid[index + k].y);
	    process_poly (&poly, TRUE, drawable, col, vary);

	    /*  The auxillary octagon  */
	    polygon_reset (&poly);
	    polygon_add_point (&poly,
			       grid[index + 3].x,
			       grid[index + 3].y);
	    polygon_add_point (&poly,
			       grid[index + grid_multiple * 2 - 2].x,
			       grid[index + grid_multiple * 2 - 2].y);
	    polygon_add_point (&poly,
			       grid[index + grid_multiple * 2 - 3].x,
			       grid[index + grid_multiple * 2 - 3].y);
	    polygon_add_point (&poly,
			       grid[index + grid_rowstride + grid_multiple].x,
			       grid[index + grid_rowstride + grid_multiple].y);
	    polygon_add_point (&poly,
			       grid[index + grid_rowstride + grid_multiple * 2 - 1].x,
			       grid[index + grid_rowstride + grid_multiple * 2 - 1].y);
	    polygon_add_point (&poly,
			       grid[index + grid_rowstride + 2].x,
			       grid[index + grid_rowstride + 2].y);
	    polygon_add_point (&poly,
			       grid[index + grid_rowstride + 1].x,
			       grid[index + grid_rowstride + 1].y);
	    polygon_add_point (&poly,
			       grid[index + 4].x,
			       grid[index + 4].y);
	    process_poly (&poly, TRUE, drawable, col, vary);

	    /*  The main square  */
	    polygon_reset (&poly);
	    polygon_add_point (&poly,
			       grid[index + 2].x,
			       grid[index + 2].y);
	    polygon_add_point (&poly,
			       grid[index + grid_multiple * 2 - 1].x,
			       grid[index + grid_multiple * 2 - 1].y);
	    polygon_add_point (&poly,
			       grid[index + grid_multiple * 2 - 2].x,
			       grid[index + grid_multiple * 2 - 2].y);
	    polygon_add_point (&poly,
			       grid[index + 3].x,
			       grid[index + 3].y);
	    process_poly (&poly, FALSE, drawable, col, vary);

	    /*  The auxillary square  */
	    polygon_reset (&poly);
	    polygon_add_point (&poly,
			       grid[index + 5].x,
			       grid[index + 5].y);
	    polygon_add_point (&poly,
			       grid[index + 4].x,
			       grid[index + 4].y);
	    polygon_add_point (&poly,
			       grid[index + grid_rowstride + 1].x,
			       grid[index + grid_rowstride + 1].y);
	    polygon_add_point (&poly,
			       grid[index + grid_rowstride].x,
			       grid[index + grid_rowstride].y);
	    process_poly (&poly, FALSE, drawable, col, vary);
	    break;
	  }

	gimp_progress_update ((double) count++ / (double) size);
      }

  gimp_progress_update (1.0);
}


static void
process_poly (Polygon   *poly,
	      gint       allow_split,
	      GDrawable *drawable,
	      guchar    *col,
	      gint       vary)
{
  gdouble dir[2];
  gdouble loc[2];
  gdouble cx, cy;
  gdouble magnitude;
  gdouble distance;
  gdouble color_vary;
  gint x1, y1, x2, y2;

  /*  find mask bounds  */
  gimp_drawable_mask_bounds (drawable->id, &x1, &y1, &x2, &y2);

  /*  determine the variation of tile color based on tile number  */
  color_vary = (vary) ? fp_rand (mvals.color_variation) : 0;
  color_vary = (rand () % 2) ? color_vary * 127 : -color_vary * 127;

  /*  Determine direction of edges inside polygon, if any  */
  find_poly_dir (poly, m_grad, h_grad, v_grad, dir, loc, x1, y1, x2, y2);
  magnitude = sqrt (SQR (dir[0] - 128) + SQR (dir[1] - 128));

  /*  Find the center of the polygon  */
  polygon_find_center (poly, &cx, &cy);
  distance = sqrt (SQR (loc[0] - cx) + SQR (loc[1] - cy));

  /*  If the magnitude of direction inside the polygon is greater than
   *  THRESHOLD, split the polygon into two new polygons
   */
  if (magnitude > MAG_THRESHOLD && (2 * distance / mvals.tile_size) < 0.5 && allow_split)
    split_poly (poly, drawable, col, dir, color_vary);
  /*  Otherwise, render the original polygon
   */
  else
    render_poly (poly, drawable, col, color_vary);
}


static void
render_poly (Polygon   *poly,
	     GDrawable *drawable,
	     guchar    *col,
	     gdouble    vary)
{
  gdouble cx, cy;

  polygon_find_center (poly, &cx, &cy);

  if (mvals.color_averaging)
    find_poly_color (poly, drawable, col, vary);

  scale_poly (poly, cx, cy, scale);

  if (mvals.color_averaging)
    fill_poly_color (poly, drawable, col);
  else
    fill_poly_image (poly, drawable, vary);
}


static void
split_poly (Polygon   *poly,
	    GDrawable *drawable,
	    guchar    *col,
	    gdouble   *dir,
	    gdouble    vary)
{
  Polygon new_poly;
  gdouble spacing;
  gdouble cx, cy;
  gdouble magnitude;
  gdouble vec[2];
  gdouble pt[2];

  spacing = mvals.tile_spacing / (2.0 * scale);

  polygon_find_center (poly, &cx, &cy);
  polygon_translate (poly, -cx, -cy);

  magnitude = sqrt (SQR (dir[0] - 128) + SQR (dir[1] - 128));
  vec[0] = -(dir[1] - 128) / magnitude;
  vec[1] = (dir[0] - 128) / magnitude;
  pt[0] = -vec[1] * spacing;
  pt[1] = vec[0] * spacing;

  polygon_reset (&new_poly);
  clip_poly (vec, pt, poly, &new_poly);
  polygon_translate (&new_poly, cx, cy);

  if (new_poly.npts)
    {
      if (mvals.color_averaging)
	find_poly_color (&new_poly, drawable, col, vary);
      scale_poly (&new_poly, cx, cy, scale);
      if (mvals.color_averaging)
	fill_poly_color (&new_poly, drawable, col);
      else
	fill_poly_image (&new_poly, drawable, vary);
    }

  vec[0] = -vec[0];
  vec[1] = -vec[1];
  pt[0] = -pt[0];
  pt[1] = -pt[1];

  polygon_reset (&new_poly);
  clip_poly (vec, pt, poly, &new_poly);
  polygon_translate (&new_poly, cx, cy);

  if (new_poly.npts)
    {
      if (mvals.color_averaging)
	find_poly_color (&new_poly, drawable, col, vary);
      scale_poly (&new_poly, cx, cy, scale);
      if (mvals.color_averaging)
	fill_poly_color (&new_poly, drawable, col);
      else
	fill_poly_image (&new_poly, drawable, vary);
    }
}


static void
clip_poly (double  *dir,
	   double  *pt,
	   Polygon *poly,
	   Polygon *poly_new)
{
  int i;
  double x1, y1, x2, y2;

  for (i = 0; i < poly->npts; i++)
    {
      x1 = (i) ? poly->pts[i-1].x : poly->pts[poly->npts-1].x;
      y1 = (i) ? poly->pts[i-1].y : poly->pts[poly->npts-1].y;
      x2 = poly->pts[i].x;
      y2 = poly->pts[i].y;

      clip_point (dir, pt, x1, y1, x2, y2, poly_new);
    }
}


static void
clip_point (gdouble *dir,
	    gdouble *pt,
	    gdouble  x1,
	    gdouble  y1,
	    gdouble  x2,
	    gdouble  y2,
	    Polygon *poly_new)
{
  gdouble det, m11, m12, m21, m22;
  gdouble side1, side2;
  gdouble t;
  gdouble vec[2];

  x1 -= pt[0]; x2 -= pt[0];
  y1 -= pt[1]; y2 -= pt[1];

  side1 = x1 * -dir[1] + y1 * dir[0];
  side2 = x2 * -dir[1] + y2 * dir[0];

  /*  If both points are to be clipped, ignore  */
  if (side1 < 0.0 && side2 < 0.0)
    return;
  /*  If both points are non-clipped, set point  */
  else if (side1 >= 0.0 && side2 >= 0.0)
    {
      polygon_add_point (poly_new, x2 + pt[0], y2 + pt[1]);
      return;
    }
  /*  Otherwise, there is an intersection...  */
  else
    {
      vec[0] = x1 - x2;
      vec[1] = y1 - y2;
      det = dir[0] * vec[1] - dir[1] * vec[0];

      if (det == 0.0)
	{
	  polygon_add_point (poly_new, x2 + pt[0], y2 + pt[1]);
	  return;
	}

      m11 = vec[1] / det;
      m12 = -vec[0] / det;
      m21 = -dir[1] / det;
      m22 = dir[0] / det;

      t = m11 * x1 + m12 * y1;

      /*  If the first point is clipped, set intersection and point  */
      if (side1 < 0.0 && side2 > 0.0)
	{
	  polygon_add_point (poly_new, dir[0] * t + pt[0], dir[1] * t + pt[1]);
	  polygon_add_point (poly_new, x2 + pt[0], y2 + pt[1]);
	}
      else
	polygon_add_point (poly_new, dir[0] * t + pt[0], dir[1] * t + pt[1]);
    }
}


static void
find_poly_dir (Polygon *poly,
	       guchar  *m_gr,
	       guchar  *h_gr,
	       guchar  *v_gr,
	       gdouble *dir,
	       gdouble *loc,
	       gint     x1,
	       gint     y1,
	       gint     x2,
	       gint     y2)
{
  gdouble dmin_x, dmin_y;
  gdouble dmax_x, dmax_y;
  gint xs, ys;
  gint xe, ye;
  gint min_x, min_y;
  gint max_x, max_y;
  gint size_x, size_y;
  gint *max_scanlines;
  gint *min_scanlines;
  guchar *dm, *dv, *dh;
  gint count, total;
  gint rowstride;
  gint i, j;

  rowstride = (x2 - x1);
  count = 0;
  total = 0;
  dir[0] = 0.0;
  dir[1] = 0.0;
  loc[0] = 0.0;
  loc[1] = 0.0;

  polygon_extents (poly, &dmin_x, &dmin_y, &dmax_x, &dmax_y);
  min_x = (int) dmin_x;
  min_y = (int) dmin_y;
  max_x = (int) dmax_x;
  max_y = (int) dmax_y;
  size_y = max_y - min_y;
  size_x = max_x - min_x;

  min_scanlines = (int *) malloc (sizeof (int) * size_y);
  max_scanlines = (int *) malloc (sizeof (int) * size_y);
  for (i = 0; i < size_y; i++)
    {
      min_scanlines[i] = max_x;
      max_scanlines[i] = min_x;
    }

  for (i = 0; i < poly->npts; i++)
    {
      xs = (int) ((i) ? poly->pts[i-1].x : poly->pts[poly->npts-1].x);
      ys = (int) ((i) ? poly->pts[i-1].y : poly->pts[poly->npts-1].y);
      xe = (int) poly->pts[i].x;
      ye = (int) poly->pts[i].y;

      convert_segment (xs, ys, xe, ye, min_y,
		       min_scanlines, max_scanlines);
    }

  for (i = 0; i < size_y; i++)
    {
      if ((i + min_y) >= y1 && (i + min_y) < y2)
	{
	  dm = m_gr + (i + min_y - y1) * rowstride - x1;
	  dh = h_gr + (i + min_y - y1) * rowstride - x1;
	  dv = v_gr + (i + min_y - y1) * rowstride - x1;

	  for (j = min_scanlines[i]; j < max_scanlines[i]; j++)
	    {
	      if (j >= x1 && j < x2)
		{
		  if (dm[j] > MAG_THRESHOLD)
		    {
		      dir[0] += dh[j];
		      dir[1] += dv[j];
		      loc[0] += j;
		      loc[1] += i + min_y;
		      count++;
		    }
		  total++;
		}
	    }
	}
    }

  if (!total)
    return;

  if ((gdouble) count / (gdouble) total > COUNT_THRESHOLD)
    {
      dir[0] /= count;
      dir[1] /= count;
      loc[0] /= count;
      loc[1] /= count;
    }
  else
    {
      dir[0] = 128.0;
      dir[1] = 128.0;
      loc[0] = 0.0;
      loc[1] = 0.0;
    }

  free (min_scanlines);
  free (max_scanlines);
}


static void
find_poly_color (Polygon   *poly,
		 GDrawable *drawable,
		 guchar    *col,
		 gdouble    color_var)
{
  GPixelRgn src_rgn;
  gdouble dmin_x, dmin_y;
  gdouble dmax_x, dmax_y;
  gint xs, ys;
  gint xe, ye;
  gint min_x, min_y;
  gint max_x, max_y;
  gint size_x, size_y;
  gint * max_scanlines;
  gint * min_scanlines;
  gint col_sum[4] = {0, 0, 0, 0};
  gint bytes;
  gint b, count;
  gint i, j, y;
  gint x1, y1, x2, y2;

  count = 0;

  gimp_drawable_mask_bounds (drawable->id, &x1, &y1, &x2, &y2);
  bytes = drawable->bpp;

  polygon_extents (poly, &dmin_x, &dmin_y, &dmax_x, &dmax_y);
  min_x = (gint) dmin_x;
  min_y = (gint) dmin_y;
  max_x = (gint) dmax_x;
  max_y = (gint) dmax_y;

  size_y = max_y - min_y;
  size_x = max_x - min_x;

  min_scanlines = (gint *) malloc (sizeof (gint) * size_y);
  max_scanlines = (gint *) malloc (sizeof (gint) * size_y);
  for (i = 0; i < size_y; i++)
    {
      min_scanlines[i] = max_x;
      max_scanlines[i] = min_x;
    }

  for (i = 0; i < poly->npts; i++)
    {
      xs = (gint) ((i) ? poly->pts[i-1].x : poly->pts[poly->npts-1].x);
      ys = (gint) ((i) ? poly->pts[i-1].y : poly->pts[poly->npts-1].y);
      xe = (gint) poly->pts[i].x;
      ye = (gint) poly->pts[i].y;

      convert_segment (xs, ys, xe, ye, min_y,
		       min_scanlines, max_scanlines);
    }

  gimp_pixel_rgn_init (&src_rgn, drawable, 0, 0,
		       drawable->width, drawable->height,
		       FALSE, FALSE);
  for (i = 0; i < size_y; i++)
    {
      y = i + min_y;
      if (y >= y1 && y < y2)
	{
	  for (j = min_scanlines[i]; j < max_scanlines[i]; j++)
	    {
	      if (j >= x1 && j < x2)
		{
		  gimp_pixel_rgn_get_pixel (&src_rgn, col, j, y);

		  for (b = 0; b < bytes; b++)
		    col_sum[b] += col[b];

		  count++;
		}
	    }
	}
    }

  if (count)
    for (b = 0; b < bytes; b++)
      {
	col_sum[b] = (gint) (col_sum[b] / count + color_var);
	if (col_sum[b] > 255)
	  col[b] = 255;
	else if (col_sum[b] < 0)
	  col[b] = 0;
	else
	  col[b] = col_sum[b];
      }

  free (min_scanlines);
  free (max_scanlines);
}


static void
scale_poly (Polygon *poly,
	    gdouble  tx,
	    gdouble  ty,
	    gdouble  poly_scale)
{
  polygon_translate (poly, -tx, -ty);
  polygon_scale (poly, poly_scale);
  polygon_translate (poly, tx, ty);
}


static void
fill_poly_color (Polygon   *poly,
		 GDrawable *drawable,
		 guchar    *col)
{
  GPixelRgn src_rgn;
  gdouble dmin_x, dmin_y;
  gdouble dmax_x, dmax_y;
  gint xs, ys;
  gint xe, ye;
  gint min_x, min_y;
  gint max_x, max_y;
  gint size_x, size_y;
  gint * max_scanlines, *max_scanlines_iter;
  gint * min_scanlines, *min_scanlines_iter;
  gint * vals;
  gint val;
  gint pixel;
  gint bytes;
  guchar buf[4];
  gint b, i, j, k, x, y;
  gdouble contrib;
  gdouble xx, yy;
  gint supersample;
  gint supersample2;
  gint x1, y1, x2, y2;
  Vertex *pts_tmp;
  const int poly_npts = poly->npts;

  /*  Determine antialiasing  */
  if (mvals.antialiasing)
    {
      supersample = SUPERSAMPLE;
      supersample2 = SQR (supersample);
    }
  else
    {
      supersample = supersample2 = 1;
    }

  gimp_drawable_mask_bounds (drawable->id, &x1, &y1, &x2, &y2);
  bytes = drawable->bpp;

  /* begin loop */
  if(poly_npts) {
    pts_tmp = poly->pts;
    xs = (gint) pts_tmp[poly_npts-1].x;
    ys = (gint) pts_tmp[poly_npts-1].y;
    xe = (gint) pts_tmp->x;
    ye = (gint) pts_tmp->y;
  
    calc_spec_vec (vecs, xs, ys, xe, ye);
  
    for (i = 1; i < poly_npts; i++)
      {
	xs = (gint) (pts_tmp->x);
	ys = (gint) (pts_tmp->y);
	pts_tmp++;
	xe = (gint) pts_tmp->x;
	ye = (gint) pts_tmp->y;
	
	calc_spec_vec (vecs+i, xs, ys, xe, ye);
      }
  }
  /* end loop */

  polygon_extents (poly, &dmin_x, &dmin_y, &dmax_x, &dmax_y);
  min_x = (gint) dmin_x;
  min_y = (gint) dmin_y;
  max_x = (gint) dmax_x;
  max_y = (gint) dmax_y;

  size_y = (max_y - min_y) * supersample;
  size_x = (max_x - min_x) * supersample;

  min_scanlines = min_scanlines_iter = (gint *) malloc (sizeof (gint) * size_y);
  max_scanlines = max_scanlines_iter = (gint *) malloc (sizeof (gint) * size_y);
  for (i = 0; i < size_y; i++)
    {
      min_scanlines[i] = max_x * supersample;
      max_scanlines[i] = min_x * supersample;
    }

  /* begin loop */
  if(poly_npts) {
    pts_tmp = poly->pts;
    xs = (gint) pts_tmp[poly_npts-1].x;
    ys = (gint) pts_tmp[poly_npts-1].y;
    xe = (gint) pts_tmp->x;
    ye = (gint) pts_tmp->y;

    xs *= supersample;
    ys *= supersample;
    xe *= supersample;
    ye *= supersample;

    convert_segment (xs, ys, xe, ye, min_y * supersample,
		     min_scanlines, max_scanlines);

    for (i = 1; i < poly_npts; i++)
      {
	xs = (gint) pts_tmp->x;
	ys = (gint) pts_tmp->y;
	pts_tmp++;
	xe = (gint) pts_tmp->x;
	ye = (gint) pts_tmp->y;

	xs *= supersample;
	ys *= supersample;
	xe *= supersample;
	ye *= supersample;

	convert_segment (xs, ys, xe, ye, min_y * supersample,
			 min_scanlines, max_scanlines);
      }
  }
  /* end loop */

  gimp_pixel_rgn_init (&src_rgn, drawable, 0, 0,
		       drawable->width, drawable->height,
		       TRUE, TRUE);

  vals = (gint *) malloc (sizeof (gint) * size_x);
  for (i = 0; i < size_y; i++, min_scanlines_iter++, max_scanlines_iter++)
    {
      if (! (i % supersample))
	memset (vals, 0, sizeof (gint) * size_x);

      yy = (gdouble) i / (gdouble) supersample + min_y;

      for (j = *min_scanlines_iter; j < *max_scanlines_iter; j++)
	{
	  x = j - min_x * supersample;
	  vals[x] += 255;
	}

      if (! ((i + 1) % supersample))
	{
	  y = (i / supersample) + min_y;

	  if (y >= y1 && y < y2)
	    {
	      for (j = 0; j < size_x; j += supersample)
		{
		  x = (j / supersample) + min_x;

		  if (x >= x1 && x < x2)
		    {
		      val = 0;
		      for (k = 0; k < supersample; k++)
			val += vals[j + k];
		      val /= supersample2;

		      if (val > 0)
			{

			  xx = (gdouble) j / (gdouble) supersample + min_x;
			  contrib = calc_spec_contrib (vecs, poly_npts, xx, yy);

			  for (b = 0; b < bytes; b++)
			    {
			      pixel = col[b] + (gint) (((contrib < 0.0)?(col[b] - back[b]):(fore[b] - col[b])) * contrib);

			      buf[b] = ((pixel * val) + (back[b] * (255 - val))) / 255;
			    }

			  gimp_pixel_rgn_set_pixel (&src_rgn, buf, x, y);
			}
		    }
		}
	    }
	}
    }

  free (vals);
  free (min_scanlines);
  free (max_scanlines);
}


static void
fill_poly_image (Polygon   *poly,
		 GDrawable *drawable,
		 gdouble    vary)
{
  GPixelRgn src_rgn, dest_rgn;
  gdouble dmin_x, dmin_y;
  gdouble dmax_x, dmax_y;
  gint xs, ys;
  gint xe, ye;
  gint min_x, min_y;
  gint max_x, max_y;
  gint size_x, size_y;
  gint * max_scanlines;
  gint * min_scanlines;
  gint * vals;
  gint val;
  gint pixel;
  gint bytes;
  guchar buf[4];
  gint b, i, j, k, x, y;
  gdouble contrib;
  gdouble xx, yy;
  gint supersample;
  gint supersample2;
  gint x1, y1, x2, y2;

  /*  Determine antialiasing  */
  if (mvals.antialiasing)
    {
      supersample = SUPERSAMPLE;
      supersample2 = SQR (supersample);
    }
  else
    {
      supersample = supersample2 = 1;
    }

  gimp_drawable_mask_bounds (drawable->id, &x1, &y1, &x2, &y2);
  bytes = drawable->bpp;
  for (i = 0; i < poly->npts; i++)
    {
      xs = (gint) ((i) ? poly->pts[i-1].x : poly->pts[poly->npts-1].x);
      ys = (gint) ((i) ? poly->pts[i-1].y : poly->pts[poly->npts-1].y);
      xe = (gint) poly->pts[i].x;
      ye = (gint) poly->pts[i].y;

      calc_spec_vec (vecs+i, xs, ys, xe, ye);
    }

  polygon_extents (poly, &dmin_x, &dmin_y, &dmax_x, &dmax_y);
  min_x = (gint) dmin_x;
  min_y = (gint) dmin_y;
  max_x = (gint) dmax_x;
  max_y = (gint) dmax_y;

  size_y = (max_y - min_y) * supersample;
  size_x = (max_x - min_x) * supersample;

  min_scanlines = (gint *) malloc (sizeof (gint) * size_y);
  max_scanlines = (gint *) malloc (sizeof (gint) * size_y);
  for (i = 0; i < size_y; i++)
    {
      min_scanlines[i] = max_x * supersample;
      max_scanlines[i] = min_x * supersample;
    }

  for (i = 0; i < poly->npts; i++)
    {
      xs = (gint) ((i) ? poly->pts[i-1].x : poly->pts[poly->npts-1].x);
      ys = (gint) ((i) ? poly->pts[i-1].y : poly->pts[poly->npts-1].y);
      xe = (gint) poly->pts[i].x;
      ye = (gint) poly->pts[i].y;

      xs *= supersample;
      ys *= supersample;
      xe *= supersample;
      ye *= supersample;

      convert_segment (xs, ys, xe, ye, min_y * supersample,
		       min_scanlines, max_scanlines);
    }

  gimp_pixel_rgn_init (&src_rgn, drawable, 0, 0,
		       drawable->width, drawable->height,
		       FALSE, FALSE);
  gimp_pixel_rgn_init (&dest_rgn, drawable, 0, 0,
		       drawable->width, drawable->height,
		       TRUE, TRUE);
  vals = (gint *) malloc (sizeof (gint) * size_x);
  for (i = 0; i < size_y; i++)
    {
      if (! (i % supersample))
	memset (vals, 0, sizeof (gint) * size_x);

      yy = (gdouble) i / (gdouble) supersample + min_y;

      for (j = min_scanlines[i]; j < max_scanlines[i]; j++)
	{
	  x = j - min_x * supersample;
	  vals[x] += 255;
	}

      if (! ((i + 1) % supersample))
	{
	  y = (i / supersample) + min_y;

	  if (y >= y1 && y < y2)
	    {
	      for (j = 0; j < size_x; j += supersample)
		{
		  x = (j / supersample) + min_x;

		  if (x >= x1 && x < x2)
		    {
		      val = 0;
		      for (k = 0; k < supersample; k++)
			val += vals[j + k];
		      val /= supersample2;

		      if (val > 0)
			{
			  xx = (double) j / (double) supersample + min_x;
			  contrib = calc_spec_contrib (vecs, poly->npts, xx, yy);

			  gimp_pixel_rgn_get_pixel (&src_rgn, buf, x, y);

			  for (b = 0; b < bytes; b++)
			    {
			      if (contrib < 0.0)
				pixel = buf[b] + (int) ((buf[b] - back[b]) * contrib);
			      else
				pixel = buf[b] + (int) ((fore[b] - buf[b]) * contrib);

			      /*  factor in per-tile intensity variation  */
			      pixel += vary;
			      if (pixel > 255)
				pixel = 255;
			      if (pixel < 0)
				pixel = 0;

			      buf[b] = ((pixel * val) + (back[b] * (255 - val))) / 255;
			    }

			  gimp_pixel_rgn_set_pixel (&dest_rgn, buf, x, y);
			}
		    }
		}
	    }
	}
    }

  free (vals);
  free (min_scanlines);
  free (max_scanlines);
}


static void
calc_spec_vec (SpecVec *vec,
	       gint     x1,
	       gint     y1,
	       gint     x2,
	       gint     y2)
{
  gdouble r;

  vec->base_x = x1;  vec->base_y = y1;
  r = sqrt (SQR (x2 - x1) + SQR (y2 - y1));
  if (r > 0.0)
    {
      vec->norm_x = -(y2 - y1) / r;
      vec->norm_y = (x2 - x1) / r;
    }
  else
    {
      vec->norm_x = 0;
      vec->norm_y = 0;
    }

  vec->light = vec->norm_x * light_x + vec->norm_y * light_y;
}


static double
calc_spec_contrib (SpecVec *vecs,
		   gint     n,
		   gdouble  x,
		   gdouble  y)
{
  gint i;
  gdouble dist;
  gdouble contrib = 0;
  gdouble x_p, y_p;

  for (i = 0; i < n; i++)
    {
      x_p = x - vecs[i].base_x;
      y_p = y - vecs[i].base_y;

      dist = fabs (x_p * vecs[i].norm_x + y_p * vecs[i].norm_y);

      if (mvals.tile_surface == ROUGH)
	{
	  /*  If the surface is rough, randomly perturb the distance  */
	  dist -= dist * ((gdouble) rand () / (gdouble) RAND_MAX);
	}

      /*  If the distance to an edge is less than the tile_spacing, there
       *  will be no highlight as the tile blends to background here
       */
      if (dist < 1.0)
	contrib += vecs[i].light;
      else if (dist <= mvals.tile_height)
	contrib += vecs[i].light * (1.0 - (dist / mvals.tile_height));
    }

  return contrib / 4.0;
}


static void
convert_segment (gint  x1,
		 gint  y1,
		 gint  x2,
		 gint  y2,
		 gint  offset,
		 gint *min,
		 gint *max)
{
  gint ydiff, y, tmp;
  gdouble xinc, xstart;

  if (y1 > y2)
    { tmp = y2; y2 = y1; y1 = tmp;
      tmp = x2; x2 = x1; x1 = tmp; }
  ydiff = (y2 - y1);

  if ( ydiff )
    {
      xinc = (gdouble) (x2 - x1) / (gdouble) ydiff;
      xstart = x1 + 0.5 * xinc;
      for (y = y1 ; y < y2; y++)
	{
	  if (xstart < min[y - offset])
	    min[y-offset] = xstart;
	  if (xstart > max[y - offset])
	    max[y-offset] = xstart;

	  xstart += xinc;
	}
    }
}


static void
polygon_add_point (Polygon *poly,
		   gdouble  x,
		   gdouble  y)
{
  if (poly->npts < 12)
    {
      poly->pts[poly->npts].x = x;
      poly->pts[poly->npts].y = y;
      poly->npts++;
    }
  else
    g_print ( _("Unable to add additional point.\n"));
}


static int
polygon_find_center (Polygon *poly,
		     gdouble *cx,
		     gdouble *cy)
{
  gint i;

  if (!poly->npts)
    return 0;

  *cx = 0.0;
  *cy = 0.0;

  for (i = 0; i < poly->npts; i++)
    {
      *cx += poly->pts[i].x;
      *cy += poly->pts[i].y;
    }

  *cx /= poly->npts;
  *cy /= poly->npts;

  return 1;
}


static void
polygon_translate (Polygon *poly,
		   gdouble  tx,
		   gdouble  ty)
{
  gint i;

  for (i = 0; i < poly->npts; i++)
    {
      poly->pts[i].x += tx;
      poly->pts[i].y += ty;
    }
}


static void
polygon_scale (Polygon *poly,
	       gdouble  poly_scale)
{
  gint i;

  for (i = 0; i < poly->npts; i++)
    {
      poly->pts[i].x *= poly_scale;
      poly->pts[i].y *= poly_scale;
    }
}

static gint
polygon_extents (Polygon *poly,
		 gdouble *x1,
		 gdouble *y1,
		 gdouble *x2,
		 gdouble *y2)
{
  gint i;

  if (!poly->npts)
    return 0;

  *x1 = *x2 = poly->pts[0].x;
  *y1 = *y2 = poly->pts[0].y;

  for (i = 1; i < poly->npts; i++)
    {
      if (poly->pts[i].x < *x1)
	*x1 = poly->pts[i].x;
      if (poly->pts[i].x > *x2)
	*x2 = poly->pts[i].x;
      if (poly->pts[i].y < *y1)
	*y1 = poly->pts[i].y;
      if (poly->pts[i].y > *y2)
	*y2 = poly->pts[i].y;
    }

  return 1;
}


static void
polygon_reset (Polygon *poly)
{
  poly->npts = 0;
}


/*  Mosaic interface functions  */

static void
mosaic_close_callback (GtkWidget *widget,
		       gpointer   data)
{
  gtk_main_quit ();
}

static void
mosaic_ok_callback (GtkWidget *widget,
		    gpointer   data)
{
  mint.run = TRUE;
  gtk_widget_destroy (GTK_WIDGET (data));
}

static void
mosaic_toggle_update (GtkWidget *widget,
		      gpointer   data)
{
  int *toggle_val;

  toggle_val = (int *) data;

  if (GTK_TOGGLE_BUTTON (widget)->active)
    *toggle_val = TRUE;
  else
    *toggle_val = FALSE;
}

static void
mosaic_scale_update (GtkAdjustment *adjustment,
		     double        *scale_val)
{
  *scale_val = adjustment->value;
}
