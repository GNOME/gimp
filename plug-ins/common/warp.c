/* Warp  --- image filter plug-in for The Gimp image manipulation program
 * Copyright (C) 1997 John P. Beale
 * Much of the 'warp' is from the Displace plug-in: 1996 Stephen Robert Norris
 * Much of the 'displace' code taken in turn  from the pinch plug-in 
 *   which is by 1996 Federico Mena Quintero
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
 * You can contact me (the warp author) at beale@best.com
 * Please send me any patches or enhancements to this code.
 * You can contact the original The Gimp authors at gimp@xcf.berkeley.edu
 *
 * --------------------------------------------------------------------
 * Warp Program structure: after running the user interface and setting the
 * parameters, warp generates a brand-new image (later to be deleted 
 * before the user ever sees it) which contains two grayscale layers,
 * representing the X and Y gradients of the "control" image. For this
 * purpose, all channels of the control image are summed for a scalar
 * value at each pixel coordinate for the gradient operation.
 *
 * The X,Y components of the calculated gradient are then used to displace pixels
 * from the source image into the destination image. The displacement vector is
 * rotated a user-specified amount first. This displacement operation happens
 * iteratively, generating a new displaced image from each prior image. 
 * -------------------------------------------------------------------
 *
 * Revision History:
 * Version 0.37  12/19/98 Fixed Tooltips and freeing memory
 * Version 0.36  11/9/97  Changed XY vector layers  back to own image
 *               fixed 'undo' problem (hopefully)
 *
 * Version 0.35  11/3/97  Added vector-map, mag-map, grad-map to
 *               diff vector instead of separate operation
 *               further futzing with drawable updates
 *               starting adding tooltips
 *
 * Version 0.34  10/30/97   'Fixed' drawable update problem
 *               Added 16-bit resolution to differential map
 *	         Added substep increments for finer control
 *
 * Version 0.33  10/26/97   Added 'angle increment' to user interface
 *
 * Version 0.32  10/25/97   Added magnitude control map (secondary control)
 *               Changed undo behavior to be one undo-step per warp call.
 *
 * Version 0.31  10/25/97   Fixed src/dest pixregions so program works
 *               with multiple-layer images. Still don't know
 *               exactly what I did to fix it :-/  Also, added 'color' option for
 *               border pixels to use the current selected foreground color.
 *
 * Version 0.3   10/20/97  Initial release for Gimp 0.99.xx
 */

#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <time.h>                  /* time(NULL) for random # seed */

#include <gtk/gtk.h>

#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

#include "libgimp/stdplugins-intl.h"

/* Some useful macros */

#define ENTRY_WIDTH     75
#define TILE_CACHE_SIZE 30  /* was 48. There is a cache flush problem in GIMP preventing sequential updates */
#define MIN_ARGS         6  /* minimum number of arguments required */

enum
{
  WRAP,
  SMEAR,
  BLACK,
  COLOR
};

typedef struct
{
  gdouble amount;
  gint    warp_map;
  gint    iter;
  gdouble dither;
  gdouble angle;
  gint    wrap_type;
  gint    mag_map;
  gint    mag_use;
  gint    substeps;
  gint    grad_map;
  gdouble grad_scale;
  gint    vector_map;
  gdouble vector_scale;
  gdouble vector_angle;
} WarpVals;

typedef struct
{
  gint run;
} WarpInterface;

/*
 * Function prototypes.
 */

static void      query  (void);
static void      run    (gchar    *name,
			 gint      nparams,
			 GParam   *param,
			 gint     *nreturn_vals,
			 GParam  **return_vals);

static void      blur16           (GDrawable *drawable);

static void      diff             (GDrawable *drawable, 
				   gint32 *xl_id, 
				   gint32 *yl_id);

static void      diff_prepare_row (GPixelRgn  *pixel_rgn,
				   guchar     *data,
				   int         x,
				   int         y,
				   int         w);

static void      warp_one         (GDrawable *draw, 
				   GDrawable *new,
				   GDrawable *map_x, 
				   GDrawable *map_y,
				   GDrawable *mag_draw,
				   gint first_time,
				   gint step);

static void      warp        (GDrawable  *drawable,
			      GDrawable **map_x_p,
			      GDrawable **map_y_p);
 
static gint      warp_dialog (GDrawable *drawable);
static GTile *   warp_pixel  (GDrawable * drawable,
			      GTile *     tile,
			      gint        width,
			      gint        height,
			      gint        x1,
			      gint        y1,
			      gint        x2,
			      gint        y2,
			      gint        x,
			      gint        y,
			      gint *      row,
			      gint *      col,
			      guchar *    pixel);

static guchar    bilinear        (gdouble    x,
				  gdouble    y,
				  guchar *   v);

static gint      bilinear16      (gdouble    x,
				  gdouble    y,
				  gint *   v);

static gint      warp_map_constrain       (gint32     image_id,
					   gint32     drawable_id,
					   gpointer   data);
static void      warp_map_callback        (gint32     id,
					   gpointer   data);
static void      warp_map_mag_callback    (gint32     id,
					   gpointer   data);
static void      warp_map_grad_callback   (gint32     id,
					   gpointer   data);
static void      warp_map_vector_callback (gint32     id,
					   gpointer   data);
static void      warp_ok_callback         (GtkWidget *widget,
					   gpointer   data);

static gdouble   warp_map_mag_give_value  (guchar    *pt, 
					   gint       alpha, 
					   gint       bytes);

/* -------------------------------------------------------------------------- */
/*   Variables global over entire plug-in scope                               */
/* -------------------------------------------------------------------------- */

GPlugInInfo PLUG_IN_INFO =
{
  NULL,  /* init_proc  */
  NULL,  /* quit_proc  */
  query, /* query_proc */
  run,   /* run_proc   */
};

static WarpVals dvals =
{
  10.0,   /* amount       */
  -1,     /* warp_map     */
  5,      /* iterations   */
  0.0,    /* dither       */
  90.0,   /* angle        */
  WRAP,   /* wrap_type    */
  -1,     /* mag_map      */
  FALSE,  /* mag_use      */
  1,      /* substeps     */
  -1,     /* grad_map     */
  0.0,    /* grad_scale   */
  -1,     /* vector_map   */
  0.0,    /* vector_scale */
  0.0     /* vector_angle */
};

static WarpInterface dint =
{
  FALSE,  /* run */
};

/* -------------------------------------------------------------------------- */

/* static gint         display_diff_map = TRUE;   show 16-bit diff. vectormap */
static gint         progress = 0;              /* progress indicator bar      */
static guint        tile_width, tile_height;   /* size of an image tile       */
static GRunModeType run_mode;                  /* interactive, non-, etc.     */
static guchar       color_pixel[4] = {0, 0, 0, 255};  /* current fg color     */


/* -------------------------------------------------------------------------- */

/***** Functions *****/

MAIN ()

static void
query (void)
{
  static GParamDef args[] =
  {
    { PARAM_INT32, "run_mode", "Interactive, non-interactive" },
    { PARAM_IMAGE, "image", "Input image (unused)" },
    { PARAM_DRAWABLE, "drawable", "Input drawable" },
    { PARAM_FLOAT, "amount", "Pixel displacement multiplier" },
    { PARAM_DRAWABLE, "warp_map", "Displacement control map" },
    { PARAM_INT32, "iter", "Iteration count (last required argument)" },
    { PARAM_FLOAT, "dither", "Random dither amount (first optional argument)" },
    { PARAM_FLOAT, "angle", "Angle of gradient vector rotation" },
    { PARAM_INT32, "wrap_type", "Edge behavior: { WRAP (0), SMEAR (1), BLACK (2), COLOR (3) }" },
    { PARAM_DRAWABLE, "mag_map", "Magnitude control map" },
    { PARAM_INT32, "mag_use", "Use magnitude map: { FALSE (0), TRUE (1) }" },
    { PARAM_INT32, "substeps", "Substeps between image updates" },
    { PARAM_INT32, "grad_map", "Gradient control map" },
    { PARAM_FLOAT, "grad_scale", "Scaling factor for gradient map (0=don't use)" },
    { PARAM_INT32, "vector_map", "Fixed vector control map" },
    { PARAM_FLOAT, "vector_scale", "Scaling factor for fixed vector map (0=don't use)" },
    { PARAM_FLOAT, "vector_angle", "Angle for fixed vector map" },
  };
  static gint nargs = sizeof (args) / sizeof (args[0]);

  INIT_I18N();

  gimp_install_procedure ("plug_in_warp",
			  "Twist or smear an image. (only first six arguments are required)",
			  "Smears an image along vector paths calculated as the gradient of a separate control matrix. The effect can look like brushstrokes of acrylic or watercolor paint, in some cases.",
			  "John P. Beale",
			  "John P. Beale",
			  "1997",
			  N_("<Image>/Filters/Map/Warp..."),
			  "RGB*, GRAY*",
			  PROC_PLUG_IN,
			  nargs, 0,
			  args, NULL);
}

static void
run (gchar  *name,
     gint    nparams,
     GParam  *param,
     gint   *nreturn_vals,
     GParam **return_vals)
{
  static GParam values[1];
  GDrawable *drawable;
  GDrawable *map_x = NULL;   /* satisfy compiler complaints */
  GDrawable *map_y = NULL;
  gint32 image_ID;           /* image id of drawable */

  GStatusType status = STATUS_SUCCESS;
  gint pcnt;                 /* parameter counter for scanning input params. */

  run_mode = param[0].data.d_int32;

  tile_width = gimp_tile_width();    /* initialize some globals */
  tile_height = gimp_tile_height();

  /* get currently selected foreground pixel color */
  gimp_palette_get_foreground (&color_pixel[0],
			       &color_pixel[1],
			       &color_pixel[2]);

  /*  Get the specified drawable  */
  drawable = gimp_drawable_get (param[2].data.d_drawable);

  *nreturn_vals = 1;
  *return_vals  = values;
  values[0].type          = PARAM_STATUS;
  values[0].data.d_status = status;

  switch (run_mode)
    {
    case RUN_INTERACTIVE:
      INIT_I18N_UI();
      /*  Possibly retrieve data  */
      gimp_get_data ("plug_in_warp", &dvals);

      /*  First acquire information with a dialog  */
      if (! warp_dialog (drawable))
	return;
      break;

    case RUN_NONINTERACTIVE:
      INIT_I18N();
      /*  Make sure minimum args
       *  (mode, image, draw, amount, warp_map, iter) are there 
       */
      if (nparams < MIN_ARGS)
	{
	  status = STATUS_CALLING_ERROR;
	}
      else
	{
	  pcnt = MIN_ARGS;                          /* parameter counter */
	  dvals.amount   = param[3].data.d_float;
	  dvals.warp_map = param[4].data.d_int32;
	  dvals.iter     = param[5].data.d_int32;
	  if (nparams > pcnt++) dvals.dither       = param[6].data.d_float;
	  if (nparams > pcnt++) dvals.angle        = param[7].data.d_float;
	  if (nparams > pcnt++) dvals.wrap_type    = param[8].data.d_int32;
	  if (nparams > pcnt++) dvals.mag_map      = param[9].data.d_int32;
	  if (nparams > pcnt++) dvals.mag_use      = param[10].data.d_int32;
	  if (nparams > pcnt++) dvals.substeps     = param[11].data.d_int32;
	  if (nparams > pcnt++) dvals.grad_map     = param[12].data.d_int32;
	  if (nparams > pcnt++) dvals.grad_scale   = param[13].data.d_float;
	  if (nparams > pcnt++) dvals.vector_map   = param[14].data.d_int32;
	  if (nparams > pcnt++) dvals.vector_scale = param[15].data.d_float;
	  if (nparams > pcnt++) dvals.vector_angle = param[16].data.d_float;
	}
      break;

    case RUN_WITH_LAST_VALS:
      /*  Possibly retrieve data  */
      gimp_get_data ("plug_in_warp", &dvals);
      break;

    default:
      break;
    }

  if (status == STATUS_SUCCESS)
    {
      /*  set the tile cache size  */
      gimp_tile_cache_ntiles (TILE_CACHE_SIZE);

      /*  run the warp effect  */
      warp (drawable, &map_x, &map_y);

      /*  Store data  */
      if (run_mode == RUN_INTERACTIVE)
	gimp_set_data ("plug_in_warp", &dvals, sizeof (WarpVals));
    }

  values[0].data.d_status = status;

  image_ID = gimp_layer_get_image_id(map_x->id);
  gimp_image_delete(image_ID);
  gimp_displays_flush();

  /*
  if (display_diff_map == FALSE) {
    gimp_layer_delete(map_x->id);
    gimp_layer_delete(map_y->id);
  } else {
    image_ID = gimp_layer_get_image_id(drawable->id);
    gimp_image_undo_disable(image_ID);
    gimp_image_undo_enable(image_ID);
  }
  */

  gimp_drawable_detach (map_x);
  gimp_drawable_detach (map_y);

  if (run_mode != RUN_NONINTERACTIVE)
    gimp_displays_flush ();
}

static int
warp_dialog (GDrawable *drawable)
{
  GtkWidget *dlg;
  GtkWidget *vbox;
  GtkWidget *label;
  GtkWidget *toggle;
  GtkWidget *toggle_hbox;
  GtkWidget *frame;
  GtkWidget *table;
  GtkWidget *otable;
  GtkWidget *spinbutton;
  GtkObject *adj;
  GtkWidget *option_menu;
  GtkWidget *option_menu_mag;
  GtkWidget *option_menu_grad;
  GtkWidget *option_menu_vector;
  GtkWidget *menu;
  GtkWidget *magmenu;
  GtkWidget *gradmenu;
  GtkWidget *vectormenu;

  GSList  *group = NULL;
  gchar  **argv;
  gint     argc;

  argc    = 1;
  argv    = g_new (gchar *, 1);
  argv[0] = g_strdup ("Warp");

  gtk_init (&argc, &argv);
  gtk_rc_parse (gimp_gtkrc ());

  dlg = gimp_dialog_new (_("Warp"), "warp",
			 gimp_plugin_help_func, "filters/warp.html",
			 GTK_WIN_POS_MOUSE,
			 FALSE, TRUE, FALSE,

			 _("OK"), warp_ok_callback,
			 NULL, NULL, NULL, TRUE, FALSE,
			 _("Cancel"), gtk_widget_destroy,
			 NULL, 1, NULL, FALSE, TRUE,

			 NULL);

  gtk_signal_connect (GTK_OBJECT (dlg), "destroy",
		      GTK_SIGNAL_FUNC (gtk_main_quit),
		      NULL);

  gimp_help_init ();

  vbox = gtk_vbox_new (FALSE, 4);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), 6);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dlg)->vbox), vbox, TRUE, TRUE, 0);
  gtk_widget_show (vbox);

  frame = gtk_frame_new (_("Main Options"));
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_ETCHED_IN);
  gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, FALSE, 0);

  table = gtk_table_new (3, 3, FALSE);
  gtk_table_set_row_spacings (GTK_TABLE (table), 2);
  gtk_table_set_col_spacings (GTK_TABLE (table), 4);
  gtk_container_set_border_width (GTK_CONTAINER (table), 4);
  gtk_container_add (GTK_CONTAINER (frame), table);

  /*  amount, iter */
  spinbutton = gimp_spin_button_new (&adj, dvals.amount,
				     -1000, 1000, /* ??? */
				     1, 10, 0, 1, 2);
  gimp_table_attach_aligned (GTK_TABLE (table), 0,
			     _("Step Size:"), 1.0, 0.5,
			     spinbutton, TRUE);
  gtk_signal_connect (GTK_OBJECT (adj), "value_changed",
		      GTK_SIGNAL_FUNC (gimp_double_adjustment_update),
		      &dvals.amount);

  spinbutton = gimp_spin_button_new (&adj, dvals.iter,
				     1, 100, 1, 5, 0, 1, 0);
  gimp_table_attach_aligned (GTK_TABLE (table), 1,
			     _("Iterations:"), 1.0, 0.5,
			     spinbutton, TRUE);
  gtk_signal_connect (GTK_OBJECT (adj), "value_changed",
		      GTK_SIGNAL_FUNC (gimp_double_adjustment_update),
		      &dvals.iter);

  /*  Displacement map menu  */
  label = gtk_label_new (_("Displacement Map:"));
  gtk_table_attach (GTK_TABLE (table), label, 2, 3, 0, 1,
		    GTK_FILL, GTK_FILL, 0, 0);
  gtk_widget_show (label);

  option_menu = gtk_option_menu_new ();
  gtk_table_attach (GTK_TABLE (table), option_menu, 2, 3, 1, 2,
		    GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL, 0, 0);
  menu = gimp_drawable_menu_new (warp_map_constrain, warp_map_callback,
				 drawable, dvals.warp_map);
  gtk_option_menu_set_menu (GTK_OPTION_MENU (option_menu), menu);
  gtk_widget_show (option_menu);

  /* ======================================================================= */

  /*  Displacement Type  */
  label = gtk_label_new (_("On Edges:"));
  gtk_misc_set_alignment (GTK_MISC (label), 1.0, 0.5);
  gtk_table_attach (GTK_TABLE (table), label, 0, 1, 2, 3,
		    GTK_FILL, GTK_FILL, 0, 0);
  gtk_widget_show (label);

  toggle_hbox = gtk_hbox_new (FALSE, 4);
  gtk_table_attach (GTK_TABLE (table), toggle_hbox, 1, 3, 2, 3,
		    GTK_FILL, GTK_FILL, 0, 0);

  toggle = gtk_radio_button_new_with_label (group, _("Wrap"));
  group = gtk_radio_button_group (GTK_RADIO_BUTTON (toggle));
  gtk_box_pack_start (GTK_BOX (toggle_hbox), toggle, FALSE, FALSE, 0);
  gtk_object_set_user_data (GTK_OBJECT (toggle), (gpointer) WRAP);
  gtk_signal_connect (GTK_OBJECT (toggle), "toggled",
		      (GtkSignalFunc) gimp_radio_button_update,
		      &dvals.wrap_type);
  gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (toggle),
			       dvals.wrap_type == WRAP);
  gtk_widget_show (toggle);

  toggle = gtk_radio_button_new_with_label (group, _("Smear"));
  group = gtk_radio_button_group (GTK_RADIO_BUTTON (toggle));
  gtk_box_pack_start (GTK_BOX (toggle_hbox), toggle, FALSE, FALSE, 0);
  gtk_object_set_user_data (GTK_OBJECT (toggle), (gpointer) SMEAR);
  gtk_signal_connect (GTK_OBJECT (toggle), "toggled",
		      (GtkSignalFunc) gimp_radio_button_update,
		      &dvals.wrap_type);
  gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (toggle),
			       dvals.wrap_type == SMEAR);
  gtk_widget_show (toggle);

  toggle = gtk_radio_button_new_with_label (group, _("Black"));
  group = gtk_radio_button_group (GTK_RADIO_BUTTON (toggle));
  gtk_box_pack_start (GTK_BOX (toggle_hbox), toggle, FALSE, FALSE, 0);
  gtk_object_set_user_data (GTK_OBJECT (toggle), (gpointer) BLACK);
  gtk_signal_connect (GTK_OBJECT (toggle), "toggled",
		      (GtkSignalFunc) gimp_radio_button_update,
		      &dvals.wrap_type);
  gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (toggle),
			       dvals.wrap_type == BLACK);
  gtk_widget_show (toggle);

  toggle = gtk_radio_button_new_with_label (group, _("FG Color"));
  group = gtk_radio_button_group (GTK_RADIO_BUTTON (toggle));
  gtk_box_pack_start (GTK_BOX (toggle_hbox), toggle, FALSE, FALSE, 0);
  gtk_object_set_user_data (GTK_OBJECT (toggle), (gpointer) COLOR);
  gtk_signal_connect (GTK_OBJECT (toggle), "toggled",
		      (GtkSignalFunc) gimp_radio_button_update,
		      &dvals.wrap_type);
  gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (toggle),
			       dvals.wrap_type == COLOR);
  gtk_widget_show (toggle);

  gtk_widget_show (toggle_hbox);

  gtk_widget_show (table);
  gtk_widget_show (frame);

  /* -------------------------------------------------------------------- */
  /* ---------    The secondary table         --------------------------  */

  frame = gtk_frame_new (_("Secondary Options"));
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_ETCHED_IN);
  gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, FALSE, 0);

  table = gtk_table_new (3, 3, FALSE);
  gtk_table_set_row_spacings (GTK_TABLE (table), 2);
  gtk_table_set_col_spacings (GTK_TABLE (table), 4);
  gtk_container_set_border_width (GTK_CONTAINER (table), 4);
  gtk_container_add (GTK_CONTAINER (frame), table);

  spinbutton = gimp_spin_button_new (&adj, dvals.dither,
				     0, 100, 1, 10, 0, 1, 2);
  gimp_table_attach_aligned (GTK_TABLE (table), 0,
			     _("Dither Size:"), 1.0, 0.5,
			     spinbutton, TRUE);
  gtk_signal_connect (GTK_OBJECT (adj), "value_changed",
		      GTK_SIGNAL_FUNC (gimp_double_adjustment_update),
		      &dvals.dither);

  spinbutton = gimp_spin_button_new (&adj, dvals.angle,
				     0, 360, 1, 15, 0, 1, 1);
  gimp_table_attach_aligned (GTK_TABLE (table), 1,
			     _("Rotation Angle:"), 1.0, 0.5,
			     spinbutton, TRUE);
  gtk_signal_connect (GTK_OBJECT (adj), "value_changed",
		      GTK_SIGNAL_FUNC (gimp_double_adjustment_update),
		      &dvals.angle);

  spinbutton = gimp_spin_button_new (&adj, dvals.substeps,
				     1, 100, 1, 5, 0, 1, 0);
  gimp_table_attach_aligned (GTK_TABLE (table), 2,
			     _("Substeps:"), 1.0, 0.5,
			     spinbutton, TRUE);
  gtk_signal_connect (GTK_OBJECT (adj), "value_changed",
		      GTK_SIGNAL_FUNC (gimp_int_adjustment_update),
		      &dvals.substeps);

  /*  Magnitude map menu  */
  label = gtk_label_new (_("Magnitude Map:"));
  gtk_table_attach (GTK_TABLE (table), label, 2, 3, 0, 1,
		    GTK_FILL, GTK_FILL, 0, 0);
  gtk_widget_show (label);

  option_menu_mag = gtk_option_menu_new ();
  gtk_table_attach (GTK_TABLE (table), option_menu_mag, 2, 3, 1, 2,
		    GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL, 0, 0);
  magmenu = gimp_drawable_menu_new (warp_map_constrain, warp_map_mag_callback,
				    drawable, dvals.mag_map);
  gtk_option_menu_set_menu (GTK_OPTION_MENU (option_menu_mag), magmenu);
  gtk_widget_show (option_menu_mag);

  /*  Magnitude Usage  */
  toggle_hbox = gtk_hbox_new (FALSE, 4);
  gtk_container_border_width (GTK_CONTAINER (toggle_hbox), 1);
  gtk_table_attach (GTK_TABLE (table), toggle_hbox, 2, 3, 2, 3,
		    GTK_FILL, GTK_FILL, 0, 0);

  toggle = gtk_check_button_new_with_label (_("Use Mag Map"));
  gtk_box_pack_start (GTK_BOX (toggle_hbox), toggle, FALSE, FALSE, 0);
  gtk_signal_connect (GTK_OBJECT (toggle), "toggled",
		      (GtkSignalFunc) gimp_toggle_button_update,
		      &dvals.mag_use);
  gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (toggle), dvals.mag_use);
  gtk_widget_show (toggle);

  gtk_widget_show (toggle_hbox);

  gtk_widget_show (table);
  gtk_widget_show (frame);

  /* -------------------------------------------------------------------- */
  /* ---------    The "other" table         --------------------------  */

  frame = gtk_frame_new (_("Other Options"));
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_ETCHED_IN);
  gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, FALSE, 0);

  otable = gtk_table_new (3, 3, FALSE);
  gtk_table_set_row_spacings (GTK_TABLE (otable), 2);
  gtk_table_set_col_spacings (GTK_TABLE (otable), 4);
  gtk_container_set_border_width (GTK_CONTAINER (otable), 4);
  gtk_container_add (GTK_CONTAINER (frame), otable);

  spinbutton = gimp_spin_button_new (&adj, dvals.grad_scale,
				     -1000, 1000, /* ??? */
				     0.01, 0.1, 0, 1, 3);
  gimp_table_attach_aligned (GTK_TABLE (otable), 0,
			     _("Gradient Scale:"), 1.0, 0.5,
			     spinbutton, TRUE);
  gtk_signal_connect (GTK_OBJECT (adj), "value_changed",
		      GTK_SIGNAL_FUNC (gimp_double_adjustment_update),
		      &dvals.grad_scale);

  /* ---------  Gradient map menu ----------------  */

  option_menu_grad = gtk_option_menu_new ();
  gtk_table_attach (GTK_TABLE (otable), option_menu_grad, 2, 3, 0, 1,
		    GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL, 0, 0);
  gradmenu = gimp_drawable_menu_new (warp_map_constrain, warp_map_grad_callback,
				 drawable, dvals.grad_map);
  gtk_option_menu_set_menu (GTK_OPTION_MENU (option_menu_grad), gradmenu);
  gimp_help_set_help_data (option_menu_grad,
			   _("Gradient map selection menu"), NULL);

  gtk_widget_show (option_menu_grad);

  /* ---------------------------------------------- */

  spinbutton = gimp_spin_button_new (&adj, dvals.vector_scale,
				     -1000, 1000, /* ??? */
				     0.01, 0.1, 0, 1, 3);
  gimp_table_attach_aligned (GTK_TABLE (otable), 1,
			     _("Vector Mag:"), 1.0, 0.5,
			     spinbutton, TRUE);
  gtk_signal_connect (GTK_OBJECT (adj), "value_changed",
		      GTK_SIGNAL_FUNC (gimp_double_adjustment_update),
		      &dvals.vector_scale);

  /* -------------------------------------------------------- */

  spinbutton = gimp_spin_button_new (&adj, dvals.vector_angle,
				     0, 360, 1, 15, 0, 1, 1);
  gimp_table_attach_aligned (GTK_TABLE (otable), 2,
			     _("Angle:"), 1.0, 0.5,
			     spinbutton, TRUE);
  gtk_signal_connect (GTK_OBJECT (adj), "value_changed",
		      GTK_SIGNAL_FUNC (gimp_double_adjustment_update),
		      &dvals.vector_angle);

  /* ---------  Vector map menu ----------------  */
  option_menu_vector = gtk_option_menu_new ();
  gtk_table_attach (GTK_TABLE (otable), option_menu_vector, 2, 3, 1, 2,
		    GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL, 0, 0);
  vectormenu = gimp_drawable_menu_new (warp_map_constrain,
				       warp_map_vector_callback,
				       drawable, dvals.vector_map);
  gtk_option_menu_set_menu (GTK_OPTION_MENU (option_menu_vector), vectormenu);
  gimp_help_set_help_data (option_menu_vector,
			   _("Fixed-direction-vector map selection menu"),
			   NULL);

  gtk_widget_show (option_menu_vector);

  gtk_widget_show (otable);
  gtk_widget_show (frame);

  gtk_widget_show (dlg);

  gtk_main ();
  gimp_help_free ();
  gdk_flush ();

  return dint.run;
}
/* ---------------------------------------------------------------------- */

static void
blur16 (GDrawable *drawable)
{
  /*  blur a 2-or-more byte-per-pixel drawable,
   *  1st 2 bytes interpreted as a 16-bit height field.
   */
  GPixelRgn srcPR, destPR;
  gint width, height;
  gint src_bytes;
  gint dest_bytes;
  gint dest_bytes_inc;
  gint offb, off1;

  guchar *dest, *d;  /* pointers to rows of X and Y diff. data */
  guchar *prev_row, *pr;
  guchar *cur_row, *cr;
  guchar *next_row, *nr;
  guchar *tmp;
  gint row, col;  /* relating to indexing into pixel row arrays */
  gint x1, y1, x2, y2;
  gdouble pval;          /* average pixel value of pixel & neighbors */

  /* --------------------------------------- */

  gimp_drawable_mask_bounds (drawable->id, &x1, &y1, &x2, &y2);

  width = drawable->width;     /* size of input drawable*/
  height = drawable->height;
  src_bytes = drawable->bpp;   /* bytes per pixel in SOURCE drawable, must be 2 or more  */
  dest_bytes = drawable->bpp;   /* bytes per pixel in SOURCE drawable, >= 2  */
  dest_bytes_inc = dest_bytes - 2;  /* this is most likely zero, but I guess it's more conservative... */

  /*  allocate row buffers for source & dest. data  */

  prev_row = (guchar *) malloc ((x2 - x1 + 2) * src_bytes);
  cur_row = (guchar *) malloc ((x2 - x1 + 2) * src_bytes);
  next_row = (guchar *) malloc ((x2 - x1 + 2) * src_bytes);
  dest = (guchar *) malloc ((x2 - x1) * src_bytes);

  /* initialize the pixel regions (read from source, write into dest)  */
  gimp_pixel_rgn_init (&srcPR, drawable, 0, 0, width, height, FALSE, FALSE);
  gimp_pixel_rgn_init (&destPR, drawable, 0, 0, width, height, TRUE, TRUE);

  pr = prev_row + src_bytes;   /* row arrays are prepared for indexing to -1 (!) */
  cr = cur_row + src_bytes;
  nr = next_row + src_bytes;

  diff_prepare_row (&srcPR, pr, x1, y1, (x2 - x1));
  diff_prepare_row (&srcPR, cr, x1, y1+1, (x2 - x1));

  /*  loop through the rows, applying the smoothing function  */
  for (row = y1; row < y2; row++)
    {
      /*  prepare the next row  */
      diff_prepare_row (&srcPR, nr, x1, row + 1, (x2 - x1));

      d = dest;
      for (col = 0; col < (x2 - x1); col++) /* over columns of pixels */
	  {
	     offb = col*src_bytes;    /* base of byte pointer offset */
 	     off1 = offb+1;                 /* offset into row arrays */

		  pval = (   256.0*pr[offb - src_bytes] + pr[off1 - src_bytes] +
		             256.0*pr[offb] + pr[off1] +
			     256.0*pr[offb + src_bytes] + pr[off1 + src_bytes] +
			     256.0*cr[offb - src_bytes] + cr[off1 - src_bytes] +
			     256.0*cr[offb]  + cr[off1] +
			     256.0*cr[offb + src_bytes] + cr[off1 + src_bytes] +
			     256.0*nr[offb - src_bytes] + nr[off1 - src_bytes] +
			     256.0*nr[offb] + nr[off1] +
			     256.0*nr[offb + src_bytes]) + nr[off1 + src_bytes];

	     pval /= 9.0;  /* take the average */
	     *d++ = (guchar) (((gint)pval)>>8);   /* high-order byte */
             *d++ = (guchar) (((gint)pval)%256);  /* low-order byte */
	     d += dest_bytes_inc;       /* move data pointer on to next destination pixel */
	     
	  }
      /*  store the dest  */
      gimp_pixel_rgn_set_row (&destPR, dest, x1, row, (x2 - x1));

      /*  shuffle the row pointers  */
      tmp = pr;
      pr = cr;
      cr = nr;
      nr = tmp;  

      if ((row % 5) == 0)
	gimp_progress_update ((double) row / (double) (y2 - y1));
    }

  /*  update the region  */
  gimp_drawable_flush (drawable);
  gimp_drawable_merge_shadow (drawable->id, TRUE);
  gimp_drawable_update (drawable->id, x1, y1, (x2 - x1), (y2 - y1));

  free (prev_row);  /* row buffers allocated at top of fn. */
  free (cur_row);
  free (next_row);
  free (dest);

} /* end blur16() */


/* ====================================================================== */
/* Get one row of pixels from the PixelRegion and put them in 'data'      */

static void
diff_prepare_row (GPixelRgn *pixel_rgn,
		  guchar    *data,
		  int        x,
		  int        y,
		  int        w)
{
  int b;

  if (y == 0)    /* wrap around */
    gimp_pixel_rgn_get_row (pixel_rgn, data, x, (pixel_rgn->h - 1), w);
  else if (y == pixel_rgn->h)
    gimp_pixel_rgn_get_row (pixel_rgn, data, x, 1, w);
  else
    gimp_pixel_rgn_get_row (pixel_rgn, data, x, y, w);

  /*  Fill in edge pixels  */
  for (b = 0; b < pixel_rgn->bpp; b++)
    {
      data[-pixel_rgn->bpp + b] = data[b];
      data[w * pixel_rgn->bpp + b] = data[(w - 1) * pixel_rgn->bpp + b];
    }
}

/* -------------------------------------------------------------------------- */
/*  'diff' combines the input drawables to prepare the two                    */
/*  16-bit (X,Y) vector displacement maps                                     */
/* -------------------------------------------------------------------------- */

static void
diff (GDrawable *drawable, 
      gint32    *xl_id, 
      gint32    *yl_id)
{
  GDrawable *draw_xd, *draw_yd;  /* vector disp. drawables */
  GDrawable *mdraw, *vdraw, *gdraw;
  gint32 image_id;           /* image holding X and Y diff. arrays */
  gint32 new_image_id;       /* image holding X and Y diff. layers */
  gint32 layer_active;       /* currently active layer */
  gint32 xlayer_id, ylayer_id;   /* individual X and Y layer ID numbers */
  GPixelRgn srcPR, destxPR, destyPR;
  GPixelRgn vecPR, magPR, gradPR;
  gint width, height;
  gint src_bytes;
  gint mbytes=0;
  gint vbytes=0;
  gint gbytes=0;   /* bytes-per-pixel of various source drawables */
  gint dest_bytes;
  gint dest_bytes_inc;
  gint do_gradmap = FALSE;          /* whether to add in gradient of gradmap to final diff. map */
  gint do_vecmap = FALSE;          /* whether to add in a fixed vector scaled by the vector map */
  gint do_magmap = FALSE;          /* whether to multiply result by the magnitude map */

  guchar *destx, *dx, *desty, *dy;  /* pointers to rows of X and Y diff. data */
  guchar *tmp;
  guchar *prev_row, *pr;
  guchar *cur_row, *cr;
  guchar *next_row, *nr;
  guchar *prev_row_g, *prg=NULL;          /* pointers to gradient map data */
  guchar *cur_row_g, *crg=NULL;
  guchar *next_row_g, *nrg=NULL;
  guchar *cur_row_v, *crv=NULL;      /* pointers to vector map data */
  guchar *cur_row_m, *crm=NULL;      /* pointers to magnitude map data */
  gint row, col, offb, off, bytes;   /* relating to indexing into pixel row arrays */
  gint x1, y1, x2, y2;
  gint dvalx, dvaly;                  /* differential value at particular pixel */
  gdouble tx, ty;                     /* temporary x,y differential value increments from gradmap, etc. */
  gdouble rdx, rdy;                     /* x,y differential values: real #s */
  gdouble rscalefac;                  /* scaling factor for x,y differential of 'curl' map */
  gdouble gscalefac;                  /* scaling factor for x,y differential of 'gradient' map */
  gdouble r, theta, dtheta;           /* rectangular<-> spherical coordinate transform for vector rotation */
  gdouble scale_vec_x, scale_vec_y;   /* fixed vector X,Y component scaling factors */
  gint has_alpha, ind;
  
  /* ----------------------------------------------------------------------- */

  if (dvals.grad_scale != 0.0)
      do_gradmap = TRUE;    /* add in gradient of gradmap if scale != 0.000 */
  if (dvals.vector_scale != 0.0)   /* add in gradient of vectormap if scale != 0.000 */
      do_vecmap = TRUE;
  do_magmap = (dvals.mag_use == TRUE);   /* multiply by magnitude map if so requested */

  /* Get the input area. This is the bounding box of the selection in
   *  the image (or the entire image if there is no selection). Only
   *  operating on the input area is simply an optimization. It doesn't
   *  need to be done for correct operation. (It simply makes it go
   *  faster, since fewer pixels need to be operated on).
   */
  gimp_drawable_mask_bounds (drawable->id, &x1, &y1, &x2, &y2);

  /* Get the size of the input image. (This will/must be the same
   *  as the size of the output image.
   */
  width = drawable->width;
  height = drawable->height;
  src_bytes = drawable->bpp;                /* bytes per pixel in SOURCE drawable */
  has_alpha = gimp_drawable_has_alpha(drawable->id);

  /* -- Add two layers: X and Y Displacement vectors -- */
  /* -- I'm using a RGB  drawable and using the first two bytes for a 
        16-bit pixel value. This is either clever, or a kluge, 
        depending on your point of view.  */

  image_id = gimp_layer_get_image_id(drawable->id);
  layer_active = gimp_image_get_active_layer(image_id);

  new_image_id = gimp_image_new(width, height, RGB); /* create new image for X,Y diff */

  xlayer_id = gimp_layer_new(new_image_id, "Warp_X_Vectors",
			     width, height,
			     RGB_IMAGE, 100.0, NORMAL_MODE);

  ylayer_id = gimp_layer_new(new_image_id, "Warp_Y_Vectors",
			     width, height,
			     RGB_IMAGE, 100.0, NORMAL_MODE);

  draw_yd = gimp_drawable_get (ylayer_id);
  draw_xd = gimp_drawable_get (xlayer_id);
 
    gimp_image_add_layer (new_image_id, xlayer_id, 1);
    gimp_image_add_layer (new_image_id, ylayer_id, 1);
    gimp_drawable_fill(xlayer_id, BG_IMAGE_FILL);
    gimp_drawable_fill(ylayer_id, BG_IMAGE_FILL);
    gimp_image_set_active_layer(image_id, layer_active);

  dest_bytes = draw_xd->bpp;                /* bytes per pixel in destination drawable(s) */
  /* for a GRAYA drawable, I would expect this to be two bytes; any more would be excess */
  dest_bytes_inc = dest_bytes - 2;


  /*  allocate row buffers for source & dest. data  */

  prev_row = g_new (guchar, (x2 - x1 + 2) * src_bytes);
  cur_row  = g_new (guchar, (x2 - x1 + 2) * src_bytes);
  next_row = g_new (guchar, (x2 - x1 + 2) * src_bytes);

  prev_row_g = g_new (guchar, (x2 - x1 + 2) * src_bytes);
  cur_row_g  = g_new (guchar, (x2 - x1 + 2) * src_bytes);
  next_row_g = g_new (guchar, (x2 - x1 + 2) * src_bytes);

  cur_row_v = g_new (guchar, (x2 - x1 + 2) * src_bytes);  /* vector map */
  cur_row_m = g_new (guchar, (x2 - x1 + 2) * src_bytes);  /* magnitude map */

  destx = g_new (guchar, (x2 - x1) * dest_bytes);
  desty = g_new (guchar, (x2 - x1) * dest_bytes);

  if ((desty==NULL) || (destx==NULL) || (cur_row_m==NULL) || (cur_row_v==NULL)
    || (next_row_g==NULL) || (cur_row_g==NULL) || (prev_row_g==NULL)
    || (next_row==NULL) || (cur_row==NULL) || (prev_row==NULL)) {
   fprintf(stderr, "Warp diff: error allocating memory.\n");
   exit(1);
  }

  /*  initialize the source and destination pixel regions  */
  gimp_pixel_rgn_init (&srcPR, drawable, 0, 0, width, height, FALSE, FALSE);  /* 'curl' vector-rotation input */
  gimp_pixel_rgn_init (&destxPR, draw_xd, 0, 0, width, height, TRUE, FALSE);  /* destination: X diff output */
  gimp_pixel_rgn_init (&destyPR, draw_yd, 0, 0, width, height, TRUE, FALSE);  /* Y diff output */

  pr = prev_row + src_bytes;
  cr = cur_row + src_bytes;
  nr = next_row + src_bytes;

  diff_prepare_row (&srcPR, pr, x1, y1, (x2 - x1));
  diff_prepare_row (&srcPR, cr, x1, y1+1, (x2 - x1));

 /* fixed-vector (x,y) component scale factors */
  scale_vec_x = dvals.vector_scale*cos((90-dvals.vector_angle)*M_PI/180.0)*256.0/10;
  scale_vec_y = dvals.vector_scale*sin((90-dvals.vector_angle)*M_PI/180.0)*256.0/10;

  if (do_vecmap) {
    /*    fprintf(stderr,"%f %f  x,y vector components.\n",scale_vec_x,scale_vec_y); */

    vdraw = gimp_drawable_get(dvals.vector_map);
    vbytes = vdraw->bpp;                /* bytes per pixel in SOURCE drawable */
    gimp_pixel_rgn_init (&vecPR, vdraw, 0, 0, width, height, FALSE, FALSE);          /* fixed-vector scale-map */
    crv = cur_row_v + vbytes;
    diff_prepare_row (&vecPR, crv, x1, y1, (x2 - x1));
  }
  if (do_gradmap) {
    gdraw = gimp_drawable_get(dvals.grad_map);
    gbytes = gdraw->bpp;
    gimp_pixel_rgn_init (&gradPR, gdraw, 0, 0, width, height, FALSE, FALSE);          /* fixed-vector scale-map */
    prg = prev_row_g + gbytes;
    crg = cur_row_g + gbytes;
    nrg = next_row_g + gbytes;
    diff_prepare_row (&gradPR, prg, x1, y1 - 1, (x2 - x1));
    diff_prepare_row (&gradPR, crg, x1, y1, (x2 - x1));
  }
  if (do_magmap) {
    mdraw = gimp_drawable_get(dvals.mag_map);
    mbytes = mdraw->bpp;
    gimp_pixel_rgn_init (&magPR, mdraw, 0, 0, width, height, FALSE, FALSE);          /* fixed-vector scale-map */
    crm = cur_row_m + mbytes;
    diff_prepare_row (&magPR, crm, x1, y1, (x2 - x1));
  }

  dtheta = dvals.angle * M_PI / 180.0;
  rscalefac = 256.0 / (3*src_bytes);         /* note that '3' is rather arbitrary here. */
  gscalefac = dvals.grad_scale* 256.0 / (3*gbytes);            /* scale factor for gradient map components */

  /*  loop through the rows, applying the differential convolution  */
  for (row = y1; row < y2; row++)
    {
      /*  prepare the next row  */
      diff_prepare_row (&srcPR, nr, x1, row + 1, (x2 - x1));
  
      if (do_magmap)
        diff_prepare_row (&magPR, crm, x1, row + 1, (x2 - x1));
      if (do_vecmap)
        diff_prepare_row (&vecPR, crv, x1, row + 1, (x2 - x1));
      if (do_gradmap)
        diff_prepare_row (&gradPR, crg, x1, row + 1, (x2 - x1));

      dx = destx;
      dy = desty;
      ind = 0;

      for (col = 0; col < (x2 - x1); col++) /* over columns of pixels */
	  {
	     rdx = 0.0;
	     rdy = 0.0;
	     ty = 0.0;
	     tx = 0.0;

	     offb = col*src_bytes;    /* base of byte pointer offset */
	     for (bytes=0; bytes < src_bytes; bytes++) /* add all channels together */
	       {
 		  off = offb+bytes;                 /* offset into row arrays */
		  rdx += ((gint) -pr[off - src_bytes]   + (gint) pr[off + src_bytes] +
			  (gint) -2*cr[off - src_bytes] + (gint) 2*cr[off + src_bytes] +
			  (gint) -nr[off - src_bytes]   + (gint) nr[off + src_bytes]);

 		  rdy += ((gint) -pr[off - src_bytes] - (gint)2*pr[off] - (gint) pr[off + src_bytes] +
			  (gint) nr[off - src_bytes] + (gint)2*nr[off] + (gint) nr[off + src_bytes]);
	       }

	     rdx *= rscalefac;   /* take average, then reduce. Assume max. rdx now 65535 */
	     rdy *= rscalefac;   /* take average, then reduce */     

	     theta = atan2(rdy,rdx);          /* convert to polar, then back to rectang. coords */
	     r = sqrt(rdy*rdy + rdx*rdx);
	     theta += dtheta;              /* rotate gradient vector by this angle (radians) */
	     rdx = r * cos(theta);
	     rdy = r * sin(theta);

             if (do_gradmap) {
	       
	       offb = col*gbytes;     /* base of byte pointer offset into pixel values (R,G,B,Alpha, etc.) */
  	       for (bytes=0; bytes < src_bytes; bytes++) /* add all channels together */
	        {
 		  off = offb+bytes;                 /* offset into row arrays */
		  tx += ((gint) -prg[off - gbytes]   + (gint) prg[off + gbytes] +
			  (gint) -2*crg[off - gbytes] + (gint) 2*crg[off + gbytes] +
			  (gint) -nrg[off - gbytes]   + (gint) nrg[off + gbytes]);

 		  ty += ((gint) -prg[off - gbytes] - (gint)2*prg[off] - (gint) prg[off + gbytes] +
			  (gint) nrg[off - gbytes] + (gint)2*nrg[off] + (gint) nrg[off + gbytes]);
	        }
	       tx *= gscalefac;
	       ty *= gscalefac;

	       rdx += tx;         /* add gradient component in to the other one */
	       rdy += ty;

             } /* if (do_gradmap) */
	     

	     if (do_vecmap) {  /* add in fixed vector scaled by  vec. map data */
	       tx = (gdouble) crv[col*vbytes];       /* use first byte only */
	       rdx += scale_vec_x * tx;
	       rdy += scale_vec_y * tx;

	     } /* if (do_vecmap) */

	     if (do_magmap) {  /* multiply result by mag. map data */
	       tx = (gdouble) crm[col*mbytes];
	       rdx = (rdx * tx)/(255.0);
	       rdy = (rdy * tx)/(255.0);

	     } /* if do_magmap */


	     dvalx = rdx + (2<<14);         /* take zero point to be 2^15, since this is two bytes */
	     dvaly = rdy + (2<<14);

             if (dvalx<0) dvalx=0;
	     if (dvalx>65535) dvalx=65535;
	     *dx++ = (guchar) (dvalx >> 8);    /* store high order byte in value channel */
             *dx++ = (guchar) (dvalx % 256);   /* store low order byte in alpha channel */
	     dx += dest_bytes_inc;       /* move data pointer on to next destination pixel */
	     
	     if (dvaly<0) dvaly=0;
	     if (dvaly>65535) dvaly=65535;
	     *dy++ = (guchar) (dvaly >> 8);
             *dy++ = (guchar) (dvaly % 256);
	     dy += dest_bytes_inc;

	  } /* ------------------------------- for (col...) ----------------  */

      /*  store the dest  */
      gimp_pixel_rgn_set_row (&destxPR, destx, x1, row, (x2 - x1));
      gimp_pixel_rgn_set_row (&destyPR, desty, x1, row, (x2 - x1));

      /*  swap around the pointers to row buffers  */
      tmp = pr;
      pr = cr;
      cr = nr;
      nr = tmp;

      if (do_gradmap) {
        tmp = prg;
        prg = crg;
        crg = nrg;
        nrg = tmp;
      }

      if ((row % 5) == 0)
	gimp_progress_update ((double) row / (double) (y2 - y1));

    } /* for (row..) */

  /*  update the region  */
  gimp_drawable_flush (draw_xd);
  gimp_drawable_flush (draw_yd);

  gimp_drawable_update (draw_xd->id, x1, y1, (x2 - x1), (y2 - y1));
  gimp_drawable_update (draw_yd->id, x1, y1, (x2 - x1), (y2 - y1));

  /*
  if (display_diff_map) {
    gimp_display_new(new_image_id);
  }
  */

  gimp_displays_flush();  /* make sure layer is visible */

  gimp_progress_init ( _("Smoothing X gradient..."));
  blur16(draw_xd); 
  gimp_progress_init ( _("Smoothing Y gradient..."));
  blur16(draw_yd);

  g_free (prev_row);  /* row buffers allocated at top of fn. */
  g_free (cur_row);
  g_free (next_row);
  g_free (prev_row_g);  /* row buffers allocated at top of fn. */
  g_free (cur_row_g);
  g_free (next_row_g);
  g_free (cur_row_v);
  g_free (cur_row_m);

  g_free (destx);
  g_free (desty);

  *xl_id = xlayer_id;  /* pass back the X and Y layer ID numbers */
  *yl_id = ylayer_id;

} /* end diff() */

/* -------------------------------------------------------------------------- */
/*            The Warp displacement is done here.                             */
/* -------------------------------------------------------------------------- */

static void      
warp (GDrawable  *orig_draw,
      GDrawable **map_x,
      GDrawable **map_y)
{
  GDrawable *disp_map;    /* Displacement map, ie, control array */
  GDrawable *mag_draw;    /* Magnitude multiplier factor map */

  gchar *string;          /* string to hold title of progress bar window */

  gint    first_time = TRUE;
  gint    width;
  gint    height;
  gint    bytes;
  gint orig_image_id;
  gint image_type;


  gint    x1, y1, x2, y2;

  gint32 xdlayer = -1;
  gint32 ydlayer = -1;

  gint warp_iter;      /* index var. over all "warp" Displacement iterations */


  disp_map = gimp_drawable_get(dvals.warp_map);
  mag_draw = gimp_drawable_get(dvals.mag_map);

  /* calculate new X,Y Displacement image maps */

  gimp_progress_init ( _("Finding XY gradient..."));

  diff(disp_map, &xdlayer, &ydlayer);    /* generate x,y differential images (arrays) */
 
  /* Get selection area */
   gimp_drawable_mask_bounds (orig_draw->id, &x1, &y1, &x2, &y2);

   width  = orig_draw->width;
   height = orig_draw->height;
   bytes  = orig_draw->bpp;
   image_type = gimp_drawable_type(orig_draw->id);

   *map_x = gimp_drawable_get(xdlayer);
   *map_y = gimp_drawable_get(ydlayer);

   orig_image_id = gimp_layer_get_image_id(orig_draw->id);

   /*   gimp_image_lower_layer(orig_image_id, new_layer_id); */ /* hide it! */

   /*   gimp_layer_set_opacity(new_layer_id, 0.0); */

   for (warp_iter = 0; warp_iter < dvals.iter; warp_iter++)
   {
     if (run_mode != RUN_NONINTERACTIVE) {
       string = g_strdup_printf (_("Flow Step %d..."), warp_iter+1);
       gimp_progress_init (string);
       g_free (string);
       progress = 0;
       gimp_progress_update (0);
     }
     warp_one(orig_draw, orig_draw, *map_x, *map_y, mag_draw, first_time, warp_iter);

     gimp_drawable_update (orig_draw->id, x1, y1, (x2 - x1), (y2 - y1));

     if (run_mode != RUN_NONINTERACTIVE)
       gimp_displays_flush();


     first_time = FALSE;

   } /*  end for (warp_iter) */

   /* gimp_image_add_layer (orig_image_id, new_layer_id, 1); */  /* make layer visible in 'layers' dialog */

} /* Warp */

/* -------------------------------------------------------------------------- */

static void
warp_one (GDrawable *draw, 
	  GDrawable *new,
	  GDrawable *map_x, 
	  GDrawable *map_y,
	  GDrawable *mag_draw,
	  gint       first_time,
	  gint       step)
{
  GPixelRgn src_rgn;
  GPixelRgn dest_rgn;
  GPixelRgn map_x_rgn;
  GPixelRgn map_y_rgn;
  GPixelRgn mag_rgn;
  GTile   * tile = NULL;
  GTile   * xtile = NULL;
  GTile   * ytile = NULL;
  gint row=-1;
  gint xrow=-1;
  gint yrow=-1;
  gint col=-1;
  gint xcol=-1;
  gint ycol=-1;

  gpointer  pr;

  gint    width = -1;
  gint    height = -1;
  gint    dest_bytes=-1;
  gint    dmap_bytes=-1;

  guchar *destrow, *dest;
  guchar *srcrow, *src;
  guchar *mxrow=NULL, *mx;  /* NULL ptr. to make gcc's -Wall fn. happy */
  guchar *myrow=NULL, *my;

  guchar *mmagrow=NULL, *mmag=NULL;
  guchar  pixel[4][4];
  gint    x1, y1, x2, y2;
  gint    x, y;
  gint    max_progress;

  gdouble needx, needy;
  gdouble xval=0;      /* initialize to quiet compiler grumbles */
  gdouble yval=0;      /* interpolated vector displacement */
  gdouble scalefac;        /* multiplier for vector displacement scaling */
  gdouble dscalefac;       /* multiplier for incremental displacement vectors */
  gint    xi, yi;
  gint    substep;         /* loop variable counting displacement vector substeps */

  guchar  values[4];
  gint    ivalues[4];
  guchar  val;

  gint k;

  gdouble dx, dy;           /* X and Y Displacement, integer from GRAY map */

  gint    xm_alpha = 0;
  gint    ym_alpha = 0;
  gint    mmag_alpha = 0;
  gint    xm_bytes = 1;
  gint    ym_bytes = 1;
  gint    mmag_bytes = 1;


  srand(time(NULL));                   /* seed random # generator */

  /* ================ Outer Loop calculation ================================ */

  /* Get selection area */

   gimp_drawable_mask_bounds (draw->id, &x1, &y1, &x2, &y2);
   width  = draw->width;
   height = draw->height;
   dest_bytes  = draw->bpp;

   dmap_bytes = map_x->bpp;

   max_progress = (x2 - x1) * (y2 - y1);


   /*  --------- Register the (many) pixel regions ----------  */

   gimp_pixel_rgn_init (&src_rgn, draw, x1, y1, (x2 - x1), (y2 - y1), FALSE, FALSE);

   /* only push undo-stack the first time through. Thanks Spencer! */
   if (first_time==TRUE)
     gimp_pixel_rgn_init (&dest_rgn, new, x1, y1, (x2 - x1), (y2 - y1), TRUE, TRUE);
   else
     /*     gimp_pixel_rgn_init (&dest_rgn, new, x1, y1, (x2 - x1), (y2 - y1), TRUE, FALSE); */
     gimp_pixel_rgn_init (&dest_rgn, new, x1, y1, (x2 - x1), (y2 - y1), TRUE, TRUE);


   gimp_pixel_rgn_init (&map_x_rgn, map_x, x1, y1, (x2 - x1), (y2 - y1), FALSE, FALSE);
   if (gimp_drawable_has_alpha(map_x->id))
	xm_alpha = 1;
   xm_bytes = gimp_drawable_bpp(map_x->id);

   gimp_pixel_rgn_init (&map_y_rgn, map_y, x1, y1, (x2 - x1), (y2 - y1), FALSE, FALSE);
   if (gimp_drawable_has_alpha(map_y->id))
	ym_alpha = 1;
   ym_bytes = gimp_drawable_bpp(map_y->id);


   if (dvals.mag_use == TRUE) {
     gimp_pixel_rgn_init (&mag_rgn, mag_draw, x1, y1, (x2 - x1), (y2 - y1), FALSE, FALSE);
     if (gimp_drawable_has_alpha(mag_draw->id))
       mmag_alpha = 1;
     mmag_bytes = gimp_drawable_bpp(mag_draw->id);

     pr = gimp_pixel_rgns_register (5, &src_rgn, &dest_rgn, &map_x_rgn, &map_y_rgn, &mag_rgn);
   } else {
     pr = gimp_pixel_rgns_register (4, &src_rgn, &dest_rgn, &map_x_rgn, &map_y_rgn);
   }

   dscalefac = (dvals.amount) / (256* 127.5 * dvals.substeps);  /* substep displacement vector scale factor */


   for (pr = pr; pr != NULL; pr = gimp_pixel_rgns_process (pr))
    {  

      srcrow = src_rgn.data;
      destrow = dest_rgn.data;
      mxrow = map_x_rgn.data;
      myrow = map_y_rgn.data;
      if (dvals.mag_use == TRUE) 
	mmagrow = mag_rgn.data;
            
      /* loop over destination pixels */
      for (y = dest_rgn.y; y < (dest_rgn.y + dest_rgn.h); y++)
	{
	  src = srcrow;
	  dest = destrow;
	  mx = mxrow;
	  my = myrow;

          if (dvals.mag_use == TRUE) 
	    mmag = mmagrow;
	  
	  for (x = dest_rgn.x; x < (dest_rgn.x + dest_rgn.w); x++)
	    {
	      /* ----- Find displacement vector (amnt_x, amnt_y) ------------ */

              dx = dscalefac * ((256.0*mx[0])+mx[1] -32768);  /* 16-bit values */ 
	      dy = dscalefac * ((256.0*my[0])+my[1] -32768);

	      if (dvals.mag_use == TRUE) {
		scalefac = warp_map_mag_give_value(mmag, mmag_alpha, mmag_bytes)/255.0;
		dx *= scalefac;
		dy *= scalefac;
	      }
	      
	      if (dvals.dither != 0.0) {       /* random dither is +/- dvals.dither pixels */
		dx += dvals.dither*((gdouble)(rand() - (G_MAXRAND >> 1)) / (G_MAXRAND >> 1));
		dy += dvals.dither*((gdouble)(rand() - (G_MAXRAND >> 1)) / (G_MAXRAND >> 1));
	      }
	      
	      if (dvals.substeps != 1) {   /* trace (substeps) iterations of displacement vector */

		for (substep = 1; substep < dvals.substeps; substep++) {

  	          needx = x + dx;   /* In this (substep) loop, (x,y) remain fixed. (dx,dy) vary each step. */
	          needy = y + dy;

	          if (needx >= 0.0)    xi = (int) needx;
		    else	       xi = -((int) -needx + 1);

		  if (needy >= 0.0)    yi = (int) needy;
		    else               yi = -((int) -needy + 1);

		     /* get 4 neighboring DX values from DiffX drawable for linear interpolation */
	          xtile = warp_pixel (map_x, xtile, width, height, x1, y1, x2, y2, xi, yi, &xrow, &xcol, pixel[0]);
	          xtile = warp_pixel (map_x, xtile, width, height, x1, y1, x2, y2, xi + 1, yi, &xrow, &xcol, pixel[1]);
	          xtile = warp_pixel (map_x, xtile, width, height, x1, y1, x2, y2, xi, yi + 1, &xrow, &xcol, pixel[2]);
	          xtile = warp_pixel (map_x, xtile, width, height, x1, y1, x2, y2, xi + 1, yi + 1, &xrow, &xcol, pixel[3]);
	      
	          ivalues[0] = 256*pixel[0][0] + pixel[0][1];
		  ivalues[1] = 256*pixel[1][0] + pixel[1][1];
		  ivalues[2] = 256*pixel[2][0] + pixel[2][1];
		  ivalues[3] = 256*pixel[3][0] + pixel[3][1];
		  xval = bilinear16(needx, needy, ivalues);

		     /* get 4 neighboring DY values from DiffY drawable for linear interpolation */
	          ytile = warp_pixel (map_y, ytile, width, height, x1, y1, x2, y2, xi, yi, &yrow, &ycol, pixel[0]);
	          ytile = warp_pixel (map_y, ytile, width, height, x1, y1, x2, y2, xi + 1, yi, &yrow, &ycol, pixel[1]);
	          ytile = warp_pixel (map_y, ytile, width, height, x1, y1, x2, y2, xi, yi + 1, &yrow, &ycol, pixel[2]);
	          ytile = warp_pixel (map_y, ytile, width, height, x1, y1, x2, y2, xi + 1, yi + 1, &yrow, &ycol, pixel[3]);
	      
	          ivalues[0] = 256*pixel[0][0] + pixel[0][1];
		  ivalues[1] = 256*pixel[1][0] + pixel[1][1];
		  ivalues[2] = 256*pixel[2][0] + pixel[2][1];
		  ivalues[3] = 256*pixel[3][0] + pixel[3][1];
		  yval = bilinear16(needx, needy, ivalues);

		  dx += dscalefac * (xval-32768);      /* move displacement vector to this new value */
		  dy += dscalefac * (yval-32768);

		} /* for (substep) */
	      } /* if (substeps != 0) */


	      /* --------------------------------------------------------- */

	      needx = x + dx;
	      needy = y + dy;

	      mx += xm_bytes;         /* pointers into x,y displacement maps */
	      my += ym_bytes;

	      if (dvals.mag_use == TRUE)
		mmag += mmag_bytes;
	      
	      /* Calculations complete; now copy the proper pixel */
	      
	      if (needx >= 0.0)
		      xi = (int) needx;
	      else
		      xi = -((int) -needx + 1);

	      if (needy >= 0.0)
		      yi = (int) needy;
	      else
		      yi = -((int) -needy + 1);
	      
	      /* get 4 neighboring pixel values from source drawable for linear interpolation */
	      tile = warp_pixel (draw, tile, width, height, x1, y1, x2, y2, xi, yi, &row, &col, pixel[0]);
	      tile = warp_pixel (draw, tile, width, height, x1, y1, x2, y2, xi + 1, yi, &row, &col, pixel[1]);
	      tile = warp_pixel (draw, tile, width, height, x1, y1, x2, y2, xi, yi + 1, &row, &col, pixel[2]);
	      tile = warp_pixel (draw, tile, width, height, x1, y1, x2, y2, xi + 1, yi + 1, &row, &col, pixel[3]);
	      
	      for (k = 0; k < dest_bytes; k++)
		{
		  values[0] = pixel[0][k];
		  values[1] = pixel[1][k];
		  values[2] = pixel[2][k];
		  values[3] = pixel[3][k];
		  val = bilinear(needx, needy, values);
		  
		  *dest++ = val;
		} /* for k */

	    } /* for x */
	  
	  /*	  srcrow += src_rgn.rowstride; */
	  srcrow += src_rgn.rowstride;
	  destrow += dest_rgn.rowstride;
	  mxrow += map_x_rgn.rowstride;
	  myrow += map_y_rgn.rowstride;
          if (dvals.mag_use == TRUE)
	    mmagrow += mag_rgn.rowstride;

	} /* for y */

      progress += (dest_rgn.w * dest_rgn.h);
      gimp_progress_update ((double) progress / (double) max_progress);

  
    } /* for pr */

   if (tile != NULL)
    gimp_tile_unref (tile, FALSE);

   if (xtile != NULL)
    gimp_tile_unref (xtile, FALSE);

   if (ytile != NULL)
    gimp_tile_unref (ytile, FALSE);

     /*  update the region  */
   gimp_drawable_flush (new);

   gimp_drawable_merge_shadow(draw->id, (first_time == TRUE));
  
} /* warp_one */

/* ------------------------------------------------------------------------- */

static gdouble
warp_map_mag_give_value (guchar *pt,
			 gint    alpha,
			 gint    bytes)
{
  gdouble ret, val_alpha;
  
  if (bytes >= 3)
    ret =  (pt[0] + pt[1] + pt[2])/3.0;
  else
    ret = (gdouble) *pt;
  
  if (alpha)
    {
      val_alpha = pt[bytes - 1];
      ret = (ret * val_alpha / 255.0);
    };
  
  return (ret);
}


static GTile *
warp_pixel (GDrawable *drawable,
	    GTile     *tile,
	    gint       width,
	    gint       height,
	    gint       x1,
	    gint       y1,
	    gint       x2,
	    gint       y2,
	    gint       x,
	    gint       y,
	    gint      *row,
	    gint      *col,
	    guchar    *pixel)
{
  static guchar empty_pixel[4] = {0, 0, 0, 0};
  guchar *data;
  gint b;

  /* Tile the image. */
  if (dvals.wrap_type == WRAP)
    {
      if (x < 0)
	x = width - (-x % width);
      else
	x %= width;

      if (y < 0)
	y = height - (-y % height);
      else
	y %= height;
    }
  /* Smear out the edges of the image by repeating pixels. */
  else if (dvals.wrap_type == SMEAR)
    {
      if (x < 0)
	x = 0;
      else if (x > width - 1)
	x = width - 1;

      if (y < 0)
	y = 0;
      else if (y > height - 1)
	y = height - 1;
    }

  if (x >= x1 && y >= y1 && x < x2 && y < y2)
    {
      if ((( (guint) (x / tile_width)) != *col) || (( (guint) (y / tile_height)) != *row))
	{
	  *col = x / tile_width;
	  *row = y / tile_height;
	  if (tile)
	    gimp_tile_unref (tile, FALSE);
	  tile = gimp_drawable_get_tile (drawable, FALSE, *row, *col);
	  gimp_tile_ref (tile);
	}

      data = tile->data + tile->bpp * (tile->ewidth * (y % tile_height) + (x % tile_width));
    }
  else
    {
      if (dvals.wrap_type == BLACK)
        data = empty_pixel;
      else
        data = color_pixel;      /* must have selected COLOR type */
    }

  for (b = 0; b < drawable->bpp; b++)
    pixel[b] = data[b];

  return tile;
}

static guchar
bilinear (gdouble  x,
	  gdouble  y,
	  guchar  *v)
{
  gdouble m0, m1;

  x = fmod(x, 1.0);
  y = fmod(y, 1.0);

  if (x < 0)
    x += 1.0;
  if (y < 0)
    y += 1.0;

  m0 = (gdouble) v[0] + x * ((gdouble) v[1] - v[0]);
  m1 = (gdouble) v[2] + x * ((gdouble) v[3] - v[2]);

  return (guchar) (m0 + y * (m1 - m0));
} /* bilinear */

static gint
bilinear16 (gdouble  x,
	    gdouble  y,
	    gint    *v)
{
  gdouble m0, m1;

  x = fmod(x, 1.0);
  y = fmod(y, 1.0);

  if (x < 0)
    x += 1.0;
  if (y < 0)
    y += 1.0;

  m0 = (gdouble) v[0] + x * ((gdouble) v[1] - v[0]);
  m1 = (gdouble) v[2] + x * ((gdouble) v[3] - v[2]);

  return (gint) (m0 + y * (m1 - m0));
} /* bilinear16 */


/*  Warp interface functions  */

static gint
warp_map_constrain (gint32     image_id,
		    gint32     drawable_id,
		    gpointer   data)
{
  GDrawable *drawable;

  drawable = (GDrawable *) data;

  if (drawable_id == -1)
    return TRUE;

  if (gimp_drawable_width (drawable_id) == drawable->width &&
      gimp_drawable_height (drawable_id) == drawable->height)
    return TRUE;
  else
    return FALSE;
}

static void
warp_map_callback (gint32   id,
		   gpointer data)
{
  dvals.warp_map = id;
}

static void
warp_map_mag_callback (gint32   id,
		       gpointer data)
{
  dvals.mag_map = id;
}

static void
warp_map_grad_callback (gint32   id,
			gpointer data)
{
  dvals.grad_map = id;
}

static void
warp_map_vector_callback (gint32   id,
			  gpointer data)
{
  dvals.vector_map = id;
}

static void
warp_ok_callback (GtkWidget *widget,
		  gpointer   data)
{
  dint.run = TRUE;

  gtk_widget_destroy (GTK_WIDGET (data));
}
