/* Warp  --- image filter plug-in for The Gimp image manipulation program
 * Copyright (C) 1997 John P. Beale
 * Much of the 'warp' code take from the Displace plug-in by 1996 Stephen Robert Norris
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
 * rotated a user-specified amount first. This displacement operation happens iteratively,
 * generating a new displaced image from each prior image. I use a second
 * image with one layer which I create for the purpose of a ping-pong between
 * image buffers. When finished, the created image buffer is destroyed and the
 * original is updated with the final image.
 * -------------------------------------------------------------------
 *
 * Revision History:
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


#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <unistd.h>
#include <signal.h>
#include <time.h>                  /* time(NULL) for random # seed */
#include "gtk/gtk.h"
#include "libgimp/gimp.h"
#include "libgimp/gimpui.h"


/* Some useful macros */

#define ENTRY_WIDTH 75
#define TILE_CACHE_SIZE 48

#define WRAP   0
#define SMEAR  1
#define BLACK  2
#define COLOR  3

typedef struct {
  gdouble amount;
  gdouble angle;
  gdouble iter;
  gdouble dither;
  gint    warp_map;
  gint    wrap_type;
  gint    mag_map;
  gint    mag_use;
} WarpVals;

typedef struct {
  GtkWidget *amount;
  GtkWidget *angle;
  GtkWidget *iter;
  GtkWidget *dither;
  GtkWidget *menu_map;
  GtkWidget *mag_map;
  GtkWidget *mag_use;

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

static void      diff             (GDrawable *drawable, 
				   gint32 *image_id,
				   gint32 *xl_id, 
				   gint32 *yl_id    );

static void      diff_prepare_row (GPixelRgn  *pixel_rgn,
				   guchar     *data,
				   int         x,
				   int         y,
				   int         w);

static void      warp_one         (GDrawable *draw, 
				   GDrawable *map_x, 
				   GDrawable *map_y,
				   GDrawable *mag_draw,
				   gint first_time      );

static void      warp        (GDrawable *drawable,
				  GDrawable **map_x_p,
				  GDrawable **map_y_p,
				  gint32 *xyimage_id  );
 
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

static gint      warp_map_constrain    (gint32     image_id,
					    gint32     drawable_id,
					    gpointer   data);
static void      warp_map_callback     (gint32     id,
					    gpointer   data);
static void      warp_map_mag_callback (gint32     id,
					    gpointer   data);
static void      warp_close_callback   (GtkWidget *widget,
					    gpointer   data);
static void      warp_ok_callback      (GtkWidget *widget,
					    gpointer   data);
static void      warp_toggle_update    (GtkWidget *widget,
					    gpointer   data);
static void      warp_entry_callback   (GtkWidget *widget,
					    gpointer   data);
static gdouble   warp_map_give_value   (guchar* ptr,
					    gint    alpha,
					    gint    bytes);
static gdouble   warp_map_mag_give_value (guchar *pt, 
					  gint alpha, 
					  gint bytes);

/***** Local vars *****/

GPlugInInfo PLUG_IN_INFO =
{
  NULL,    /* init_proc */
  NULL,    /* quit_proc */
  query,   /* query_proc */
  run,     /* run_proc */
};

static WarpVals dvals =
{
  10.0,    /* amount */
  90.0,    /* angle */
  5.0,        /* iterations */
  0.0,        /* dither */
  -1,      /* warp_map */
  WRAP,    /* wrap_type */
  -1,      /* mag_map */
  FALSE     /* mag_use */
};

static WarpInterface dint =
{
  NULL,   /*  amount  */
  NULL,   /*  angle  */
  NULL,   /*  iter   */
  NULL,   /*  dither   */
  NULL,   /*  menu_map  */
  FALSE,  /*  run  */
  NULL,   /*  mag_map  */
  FALSE   /*  mag_use  */
};

gint    progress = 0;   /* progress indicator bar */
guint tile_width, tile_height;  /* size of an image tile */

guchar color_pixel[4] = {0, 0, 0, 255};  /* current selected foreground color */


/***** Functions *****/

MAIN ()

static void
query ()
{
  static GParamDef args[] =
  {
    { PARAM_INT32, "run_mode", "Interactive, non-interactive" },
    { PARAM_IMAGE, "image", "Input image (unused)" },
    { PARAM_DRAWABLE, "drawable", "Input drawable" },
    { PARAM_FLOAT, "amount", "Pixel displacement multiplier" },
    { PARAM_FLOAT, "angle", "Angle of gradient vector rotation" },
    { PARAM_FLOAT, "iter", "Iteration count" },
    { PARAM_FLOAT, "dither", "Random dither amount" },
    { PARAM_DRAWABLE, "warp_map", "Displacement control map" },
    { PARAM_INT32, "wrap_type", "Edge behavior: { WRAP (0), SMEAR (1), BLACK (2), COLOR (3) }" },
    { PARAM_DRAWABLE, "mag_map", "Magnitude control map" },
    { PARAM_INT32, "mag_option", "Use magnitude map: { FALSE (0), TRUE (1) }" }
  };
  static GParamDef *return_vals = NULL;
  static gint nargs = sizeof (args) / sizeof (args[0]);
  static gint nreturn_vals = 0;

  gimp_install_procedure ("plug_in_warp",
			  "Twist or smear an image",
			  "Smears an image along vector paths calculated as the gradient of a separate control matrix. The effect can look like brushstrokes of acrylic or watercolor paint, in some cases.",
			  "John P. Beale",
			  "John P. Beale",
			  "1997",
			  "<Image>/Filters/Artistic/Warp",
			  "RGB*, GRAY*",
			  PROC_PLUG_IN,
			  nargs, nreturn_vals,
			  args, return_vals);
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
  gint32 xyimage_id;

  GRunModeType run_mode;
  GStatusType status = STATUS_SUCCESS;

  run_mode = param[0].data.d_int32;

  tile_width = gimp_tile_width();    /* initialize some globals */
  tile_height = gimp_tile_height();

  /* get currently selected foreground pixel color */
  gimp_palette_get_foreground (&color_pixel[0], &color_pixel[1], &color_pixel[2]);



  /*  Get the specified drawable  */
  drawable = gimp_drawable_get (param[2].data.d_drawable);

  *nreturn_vals = 1;
  *return_vals = values;

  values[0].type = PARAM_STATUS;
  values[0].data.d_status = status;

  switch (run_mode)
    {
    case RUN_INTERACTIVE:
      /*  Possibly retrieve data  */
      gimp_get_data ("plug_in_warp", &dvals);

      /*  First acquire information with a dialog  */
      if (! warp_dialog (drawable))
	return;
      break;

    case RUN_NONINTERACTIVE:
      /*  Make sure all the arguments are there!  */
      if (nparams != 11)
	status = STATUS_CALLING_ERROR;
      if (status == STATUS_SUCCESS)
	{
	  dvals.amount = param[3].data.d_float;
	  dvals.angle = param[4].data.d_float;
	  dvals.iter = param[5].data.d_float;
	  dvals.dither = param[6].data.d_float;
	  dvals.warp_map = param[7].data.d_int32;
	  dvals.wrap_type = param[8].data.d_int32;
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
         warp (drawable, &map_x, &map_y, &xyimage_id);

         if (run_mode != RUN_NONINTERACTIVE)
	   gimp_displays_flush ();

      /*  Store data  */
      if (run_mode == RUN_INTERACTIVE)
	gimp_set_data ("plug_in_warp", &dvals, sizeof (WarpVals));
    }

  values[0].data.d_status = status;

  /*  gimp_drawable_detach (drawable); */
  gimp_drawable_detach (map_x);
  gimp_drawable_detach (map_y);

  gimp_image_delete(xyimage_id); 

}

static int
warp_dialog (GDrawable *drawable)
{
  GtkWidget *dlg;
  GtkWidget *label;
  GtkWidget *button;
  GtkWidget *toggle;
  GtkWidget *toggle_hbox;
  GtkWidget *frame;
  GtkWidget *table;
  GtkWidget *entry;
  GtkWidget *option_menu;
  GtkWidget *option_menu_mag;
  GtkWidget *menu;
  GtkWidget *magmenu;
  GSList *group = NULL;
  GSList *groupmag = NULL;
  gchar **argv;
  gchar buffer[32];
  gint argc;
  gint use_wrap = (dvals.wrap_type == WRAP);
  gint use_smear = (dvals.wrap_type == SMEAR);
  gint use_black = (dvals.wrap_type == BLACK);
  gint use_color = (dvals.wrap_type == COLOR);
  gint mag_use_yes = (dvals.mag_use == TRUE);
  gint mag_use_no = (dvals.mag_use == FALSE);

  argc = 1;
  argv = g_new (gchar *, 1);
  argv[0] = g_strdup ("Warp");

  gtk_init (&argc, &argv);

  dlg = gtk_dialog_new ();
  gtk_window_set_title (GTK_WINDOW (dlg), "Warp");
  gtk_window_position (GTK_WINDOW (dlg), GTK_WIN_POS_MOUSE);
  gtk_signal_connect (GTK_OBJECT (dlg), "destroy",
		      (GtkSignalFunc) warp_close_callback,
		      NULL);

  /*  Action area  */
  button = gtk_button_new_with_label ("OK");
  GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT);
  gtk_signal_connect (GTK_OBJECT (button), "clicked",
                      (GtkSignalFunc) warp_ok_callback,
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

  /*  The main table  */
  frame = gtk_frame_new ("Main Options");
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_ETCHED_IN);
  gtk_container_border_width (GTK_CONTAINER (frame), 10);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dlg)->vbox), frame, TRUE, TRUE, 0);

      /* table params: rows, columns */
  table = gtk_table_new (4, 3, FALSE);
  gtk_container_border_width (GTK_CONTAINER (table), 5);
  gtk_container_add (GTK_CONTAINER (frame), table);

  gtk_table_set_row_spacing (GTK_TABLE (table), 0, 5);
  gtk_table_set_row_spacing (GTK_TABLE (table), 1, 5);
  gtk_table_set_col_spacing (GTK_TABLE (table), 0, 5);
  gtk_table_set_col_spacing (GTK_TABLE (table), 1, 5);

  /*  on_x, on_y  */
  label = gtk_label_new ("Step Size");
  gtk_table_attach (GTK_TABLE (table), label, 0, 1, 0, 1, GTK_FILL, GTK_FILL, 0, 0);
  gtk_widget_show (label);

  label = gtk_label_new ("Rotation Angle");
  gtk_table_attach (GTK_TABLE (table), label, 0, 1, 1, 2, GTK_FILL, GTK_FILL, 0, 0);
  gtk_widget_show (label);

  label = gtk_label_new ("Iterations");
  gtk_table_attach (GTK_TABLE (table), label, 0, 1, 2, 3, GTK_FILL, GTK_FILL, 0, 0);
  gtk_widget_show (label);

  /*  amount, angle, iter */
  dint.amount = entry = gtk_entry_new ();
  gtk_table_attach (GTK_TABLE (table), entry, 1, 2, 0, 1, GTK_FILL, GTK_FILL, 0, 0);
  gtk_widget_set_usize (entry, ENTRY_WIDTH, 0);
  sprintf (buffer, "%f", dvals.amount);
  gtk_entry_set_text (GTK_ENTRY (entry), buffer);
  gtk_signal_connect (GTK_OBJECT (entry), "changed",
		      (GtkSignalFunc) warp_entry_callback,
		      &dvals.amount);
  gtk_widget_show (entry);

  dint.angle = entry = gtk_entry_new ();
  gtk_table_attach (GTK_TABLE (table), entry, 1, 2, 1, 2, GTK_FILL, GTK_FILL, 0, 0);
  gtk_widget_set_usize (entry, ENTRY_WIDTH, 0);
  sprintf (buffer, "%f", dvals.angle);
  gtk_entry_set_text (GTK_ENTRY (entry), buffer);
  gtk_signal_connect (GTK_OBJECT (entry), "changed",
		      (GtkSignalFunc) warp_entry_callback,
		      &dvals.angle);
  gtk_widget_show (entry);

  dint.iter = entry = gtk_entry_new ();
  gtk_table_attach (GTK_TABLE (table), entry, 1, 2, 2, 3, GTK_FILL, GTK_FILL, 0, 0);
  gtk_widget_set_usize (entry, ENTRY_WIDTH, 0);
  sprintf (buffer, "%f", dvals.iter);
  gtk_entry_set_text (GTK_ENTRY (entry), buffer);
  gtk_signal_connect (GTK_OBJECT (entry), "changed",
		      (GtkSignalFunc) warp_entry_callback,
		      &dvals.iter);
  gtk_widget_show (entry);


  /*  Displacement map menu  */
  label = gtk_label_new ("Displacement Map:");
  gtk_table_attach (GTK_TABLE (table), label, 2, 3, 0, 1, GTK_FILL, GTK_FILL, 0, 0);
  gtk_widget_show (label);

  dint.menu_map = option_menu = gtk_option_menu_new ();
  gtk_table_attach (GTK_TABLE (table), option_menu, 2, 3, 1, 2,
		    GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL, 0, 0);
  menu = gimp_drawable_menu_new (warp_map_constrain, warp_map_callback,
				 drawable, dvals.warp_map);
  gtk_option_menu_set_menu (GTK_OPTION_MENU (option_menu), menu);
  gtk_widget_show (option_menu);


  /* ================================================================================== */

  /*  Displacement Type  */
  toggle_hbox = gtk_hbox_new (FALSE, 10);
  gtk_container_border_width (GTK_CONTAINER (toggle_hbox), 5);
  gtk_table_attach (GTK_TABLE (table), toggle_hbox, 0, 3, 3, 4, GTK_FILL, GTK_FILL, 0, 0);

  label = gtk_label_new ("On Edges: ");
  gtk_box_pack_start (GTK_BOX (toggle_hbox), label, FALSE, FALSE, 0);
  gtk_widget_show (label);

  toggle = gtk_radio_button_new_with_label (group, "Wrap");
  group = gtk_radio_button_group (GTK_RADIO_BUTTON (toggle));
  gtk_box_pack_start (GTK_BOX (toggle_hbox), toggle, FALSE, FALSE, 0);
  gtk_signal_connect (GTK_OBJECT (toggle), "toggled",
		      (GtkSignalFunc) warp_toggle_update,
		      &use_wrap);
  gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (toggle), use_wrap);
  gtk_widget_show (toggle);

  toggle = gtk_radio_button_new_with_label (group, "Smear");
  group = gtk_radio_button_group (GTK_RADIO_BUTTON (toggle));
  gtk_box_pack_start (GTK_BOX (toggle_hbox), toggle, FALSE, FALSE, 0);
  gtk_signal_connect (GTK_OBJECT (toggle), "toggled",
		      (GtkSignalFunc) warp_toggle_update,
		      &use_smear);
  gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (toggle), use_smear);
  gtk_widget_show (toggle);

  toggle = gtk_radio_button_new_with_label (group, "Black");
  group = gtk_radio_button_group (GTK_RADIO_BUTTON (toggle));
  gtk_box_pack_start (GTK_BOX (toggle_hbox), toggle, FALSE, FALSE, 0);
  gtk_signal_connect (GTK_OBJECT (toggle), "toggled",
		      (GtkSignalFunc) warp_toggle_update,
		      &use_black);
  gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (toggle), use_black);
  gtk_widget_show (toggle);

  toggle = gtk_radio_button_new_with_label (group, "FG Color");
  group = gtk_radio_button_group (GTK_RADIO_BUTTON (toggle));
  gtk_box_pack_start (GTK_BOX (toggle_hbox), toggle, FALSE, FALSE, 0);
  gtk_signal_connect (GTK_OBJECT (toggle), "toggled",
		      (GtkSignalFunc) warp_toggle_update,
		      &use_color);
  gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (toggle), use_color);
  gtk_widget_show (toggle);

  gtk_widget_show (toggle_hbox);

  gtk_widget_show (table);
  gtk_widget_show (frame);

  /* -------------------------------------------------------------------- */


  /*  The secondary table  */
  frame = gtk_frame_new ("Secondary Options");
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_ETCHED_IN);
  gtk_container_border_width (GTK_CONTAINER (frame), 10);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dlg)->vbox), frame, TRUE, TRUE, 0);

      /* table params: rows, columns */
  table = gtk_table_new (2, 3, FALSE);
  gtk_container_border_width (GTK_CONTAINER (table), 5);
  gtk_container_add (GTK_CONTAINER (frame), table);

  gtk_table_set_row_spacings (GTK_TABLE (table), 5);
  gtk_table_set_col_spacings (GTK_TABLE (table), 5);


  label = gtk_label_new ("Dither Size");
  gtk_table_attach (GTK_TABLE (table), label, 0, 1, 0, 1, GTK_FILL, GTK_FILL, 0, 0);
  gtk_widget_show (label);

  dint.dither = entry = gtk_entry_new ();
  gtk_table_attach (GTK_TABLE (table), entry, 1, 2, 0, 1, GTK_FILL, GTK_FILL, 0, 0);
  gtk_widget_set_usize (entry, ENTRY_WIDTH, 0);
  sprintf (buffer, "%f", dvals.dither);
  gtk_entry_set_text (GTK_ENTRY (entry), buffer);
  gtk_signal_connect (GTK_OBJECT (entry), "changed",
		      (GtkSignalFunc) warp_entry_callback,
		      &dvals.dither);
  gtk_widget_show (entry);


  /*  Magnitude map menu  */
  label = gtk_label_new ("Magnitude Map:");
  gtk_table_attach (GTK_TABLE (table), label, 2, 3, 0, 1, GTK_FILL, GTK_FILL, 0, 0);
  gtk_widget_show (label);

  dint.mag_map = option_menu_mag = gtk_option_menu_new ();
  gtk_table_attach (GTK_TABLE (table), option_menu_mag, 2, 3, 1, 2,
		    GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL, 0, 0);
  magmenu = gimp_drawable_menu_new (warp_map_constrain, warp_map_mag_callback,
				 drawable, dvals.mag_map);
  gtk_option_menu_set_menu (GTK_OPTION_MENU (option_menu_mag), magmenu);
  gtk_widget_show (option_menu_mag);


  /* ----------------------------------------------------------------------- */
  /*  Magnitude Useage  */
  toggle_hbox = gtk_hbox_new (FALSE, 10);
  gtk_container_border_width (GTK_CONTAINER (toggle_hbox), 5);
  gtk_table_attach (GTK_TABLE (table), toggle_hbox, 0, 2, 1, 2, GTK_FILL, GTK_FILL, 0, 0);

  label = gtk_label_new ("Use Mag Map: ");
  gtk_box_pack_start (GTK_BOX (toggle_hbox), label, FALSE, FALSE, 0);
  gtk_widget_show (label);

  toggle = gtk_radio_button_new_with_label (groupmag, "Yes");
  groupmag = gtk_radio_button_group (GTK_RADIO_BUTTON (toggle));
  gtk_box_pack_start (GTK_BOX (toggle_hbox), toggle, FALSE, FALSE, 0);
  gtk_signal_connect (GTK_OBJECT (toggle), "toggled",
		      (GtkSignalFunc) warp_toggle_update,
		      &mag_use_yes);
  gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (toggle), mag_use_yes);
  gtk_widget_show (toggle);

  toggle = gtk_radio_button_new_with_label (groupmag, "No");
  groupmag = gtk_radio_button_group (GTK_RADIO_BUTTON (toggle));
  gtk_box_pack_start (GTK_BOX (toggle_hbox), toggle, FALSE, FALSE, 0);
  gtk_signal_connect (GTK_OBJECT (toggle), "toggled",
		      (GtkSignalFunc) warp_toggle_update,
		      &mag_use_no);
  gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (toggle), mag_use_no);
  gtk_widget_show (toggle);

  gtk_widget_show (toggle_hbox);



  gtk_widget_show (table);
  gtk_widget_show (frame);
  /* --------------------------------------------------------------- */

  gtk_widget_show (dlg);

  gtk_main ();
  gdk_flush ();

  /*  determine wrap type  */
  if (use_wrap)
    dvals.wrap_type = WRAP;
  else if (use_smear)
    dvals.wrap_type = SMEAR;
  else if (use_black)
    dvals.wrap_type = BLACK;
  else if (use_color)
    dvals.wrap_type = COLOR;

  /* determine whether to use magnitude multiplier map */
  if (mag_use_yes)
    dvals.mag_use = TRUE;
  else if (mag_use_no)
    dvals.mag_use = FALSE;

  return dint.run;
}

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

  if (y == 0)
    gimp_pixel_rgn_get_row (pixel_rgn, data, x, (y + 1), w);
  else if (y == pixel_rgn->h)
    gimp_pixel_rgn_get_row (pixel_rgn, data, x, (y - 1), w);
  else
    gimp_pixel_rgn_get_row (pixel_rgn, data, x, y, w);

  /*  Fill in edge pixels  */
  for (b = 0; b < pixel_rgn->bpp; b++)
    {
      data[-pixel_rgn->bpp + b] = data[b];
      data[w * pixel_rgn->bpp + b] = data[(w - 1) * pixel_rgn->bpp + b];
    }
}

static void
diff (GDrawable *drawable, 
         gint32 *image_id,
         gint32 *xl_id, 
         gint32 *yl_id    )
{
  GDrawable *draw_xd, *draw_yd;  /* vector disp. drawables */
  gint32 new_image_id;           /* new image holding X and Y diff. arrays */
  gint32 xlayer_id, ylayer_id;   /* individual X and Y layer ID numbers */
  GPixelRgn srcPR, destxPR, destyPR;
  gint width, height;
  gint src_bytes;
  gint dest_bytes;
  guchar *destx, *dx, *desty, *dy;  /* pointers to rows of X and Y diff. data */
  guchar *prev_row, *pr;
  guchar *cur_row, *cr;
  guchar *next_row, *nr;
  guchar *tmp;
  gint row, col, offb, off, bytes;  /* relating to indexing into pixel row arrays */
  gint x1, y1, x2, y2;
  gint dvalx, dvaly;                  /* differential value at particular pixel */
  gint has_alpha, ind;
  
    
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


 /* -- Create an image with two layers: X and Y Displacement vectors -- */

  new_image_id = gimp_image_new(width, height, GRAY);
 /* gimp_image_set_filename (new_image_id, "XY_Vectors"); */

  xlayer_id = gimp_layer_new(new_image_id, "X_Vectors",
			     width, height,
			     GRAY_IMAGE, 100.0, NORMAL_MODE);
  gimp_image_add_layer (new_image_id, xlayer_id, 0);

  draw_xd = gimp_drawable_get (xlayer_id);

  ylayer_id = gimp_layer_new(new_image_id, "Y_Vectors",
			     width, height,
			     GRAY_IMAGE, 100.0, NORMAL_MODE);
  gimp_image_add_layer (new_image_id, ylayer_id, 0);
  draw_yd = gimp_drawable_get (ylayer_id);

  dest_bytes = draw_xd->bpp;                /* bytes per pixel in destination drawable(s) */
  
  /*  allocate row buffers for source & dest. data  */
  /* P.S. Hey, what happens if malloc returns NULL, hmmm..? */

  prev_row = (guchar *) malloc ((x2 - x1 + 2) * src_bytes);
  cur_row = (guchar *) malloc ((x2 - x1 + 2) * src_bytes);
  next_row = (guchar *) malloc ((x2 - x1 + 2) * src_bytes);
  destx = (guchar *) malloc ((x2 - x1) * src_bytes);
  desty = (guchar *) malloc ((x2 - x1) * src_bytes);

  /*  initialize the pixel regions  */
  gimp_pixel_rgn_init (&srcPR, drawable, 0, 0, width, height, FALSE, FALSE);
  gimp_pixel_rgn_init (&destxPR, draw_xd, 0, 0, width, height, TRUE, FALSE);
  gimp_pixel_rgn_init (&destyPR, draw_yd, 0, 0, width, height, TRUE, FALSE);

  pr = prev_row + src_bytes;
  cr = cur_row + src_bytes;
  nr = next_row + src_bytes;

  diff_prepare_row (&srcPR, pr, x1, y1 - 1, (x2 - x1));
  diff_prepare_row (&srcPR, cr, x1, y1, (x2 - x1));

  /*  loop through the rows, applying the convolution  */
  for (row = y1; row < y2; row++)
    {
      /*  prepare the next row  */
      diff_prepare_row (&srcPR, nr, x1, row + 1, (x2 - x1));

      dx = destx;
      dy = desty;
      ind = 0;
      for (col = 0; col < (x2 - x1); col++) /* over columns of pixels */
	  {
	     dvalx = 0;
	     dvaly = 0;
	     offb = col*src_bytes;    /* base of byte pointer offset */
	     for (bytes=0; bytes < src_bytes; bytes++) /* add all channels together */
	       {
 		  off = offb+bytes;                 /* offset into row arrays */
		  dvalx += ((gint) -pr[off - src_bytes]   + (gint) pr[off + src_bytes] +
			  (gint) -2*cr[off - src_bytes] + (gint) 2*cr[off + src_bytes] +
			  (gint) -nr[off - src_bytes]   + (gint) nr[off + src_bytes]);

 		  dvaly += ((gint) -pr[off - src_bytes] - (gint)2*pr[off] - (gint) pr[off + src_bytes] +
			  (gint) nr[off - src_bytes] + (gint)2*nr[off] + (gint) nr[off + src_bytes]);
	       }
	     dvalx /= (3*dest_bytes);   /* take average, then reduce */
	     dvalx += 127;         /* take zero point to be 127, since this is one byte */

             if (dvalx<0) dvalx=0;
	     if (dvalx>255) dvalx=255;
	     *dx = (guchar) dvalx;
	     dx += dest_bytes;       /* move data pointer on to next destination pixel */

	     dvaly /= (2*dest_bytes);   /* take average, then reduce */
	     dvaly += 127;         /* take zero point to be 127, since this is one byte */
	     
	     if (dvaly<0) dvaly=0;
	     if (dvaly>255) dvaly=255;
	     *dy = (guchar) dvaly;
	     dy += dest_bytes;
	  }
      /*  store the dest  */
      gimp_pixel_rgn_set_row (&destxPR, destx, x1, row, (x2 - x1));
      gimp_pixel_rgn_set_row (&destyPR, desty, x1, row, (x2 - x1));

      /*  shuffle the row pointers  */
      tmp = pr;
      pr = cr;
      cr = nr;
      nr = tmp;  

      if ((row % 5) == 0)
	gimp_progress_update ((double) row / (double) (y2 - y1));
    }

  /*  update the region  */
  gimp_drawable_flush (draw_xd);
  gimp_drawable_flush (draw_yd);

  gimp_drawable_update (draw_xd->id, x1, y1, (x2 - x1), (y2 - y1));
  gimp_drawable_update (draw_yd->id, x1, y1, (x2 - x1), (y2 - y1));
 
  /*  gimp_display_new(new_image_id); */  

  free (prev_row);  /* row buffers allocated at top of fn. */
  free (cur_row);
  free (next_row);
  free (destx);
  free (desty);

  *xl_id = xlayer_id;  /* pass back the X and Y layer ID numbers */
  *yl_id = ylayer_id;
  *image_id = new_image_id;      /* return image id for eventual destruction */

} /* end diff() */


/* The Warp displacement is done here. */

static void      
warp        (GDrawable *orig_draw,
		  GDrawable **map_x_p,
		  GDrawable **map_y_p,
		  gint32 *xyimage_id_p )
{
  GDrawable *disp_map;    /* Displacement map, ie, control array */
  GDrawable *mag_draw;    /* Magnitude multiplier factor map */

  GDrawable *map_x = NULL;
  GDrawable *map_y = NULL;

  gchar string[80];    /* string to hold title of progress bar window */

  gint    first_time = TRUE;
  gint    width;
  gint    height;
  gint    bytes;

  gint    x1, y1, x2, y2;

  gint32 xdlayer = -1;
  gint32 ydlayer = -1;
  gint32 xyimage_id = -1;

  gint warp_iter;           /* index var. over all "warp" Displacement iterations */


  disp_map = gimp_drawable_get(dvals.warp_map);
  mag_draw = gimp_drawable_get(dvals.mag_map);

  /* calculate new X,Y Displacement image maps */

  gimp_progress_init ("Finding gradient...");

  diff(disp_map, &xyimage_id, &xdlayer, &ydlayer);              /* generate x,y differential images (arrays) */
 
  /* Get selection area */
   gimp_drawable_mask_bounds (orig_draw->id, &x1, &y1, &x2, &y2);

   width  = orig_draw->width;
   height = orig_draw->height;
   bytes  = orig_draw->bpp;

   map_x = gimp_drawable_get(xdlayer);
   map_y = gimp_drawable_get(ydlayer);

   for (warp_iter = 0; warp_iter < dvals.iter; warp_iter++)
   {
     sprintf(string,"Step %d...",warp_iter+1);
     gimp_progress_init (string);
     progress     = 0;
     warp_one(orig_draw, map_x, map_y, mag_draw, first_time);

     first_time = FALSE;

   } /*  end for (warp_iter) */

  *xyimage_id_p = xyimage_id;
  *map_x_p = map_x;
  *map_y_p = map_y;

} /* Warp */


static void
warp_one(GDrawable *draw, 
	 GDrawable *map_x, 
	 GDrawable *map_y,
	 GDrawable *mag_draw,
	 gint first_time       )
{
  GPixelRgn src_rgn;
  GPixelRgn dest_rgn;
  GPixelRgn map_x_rgn;
  GPixelRgn map_y_rgn;
  GPixelRgn mag_rgn;
  GTile   * tile = NULL;
  gint      row = -1;
  gint      col = -1;
  gpointer  pr;

  gint    width = -1;
  gint    height = -1;
  gint    dest_bytes=-1;

  guchar *destrow, *dest;
  guchar *srcrow, *src;
  guchar *mxrow=NULL, *mx;  /* NULL ptr. to make gcc's -Wall fn. happy */
  guchar *myrow=NULL, *my;
  guchar *mmagrow=NULL, *mmag=NULL;
  guchar  pixel[4][4];
  gint    x1, y1, x2, y2;
  gint    x, y;
  gint    max_progress;

  gdouble amnt_x, amnt_y;  /* amount of Displacement of each pixel */
  gdouble needx, needy;
  gdouble scalefac;        /* multiplier for vector displacement scaling */
  gint    xi, yi;

  guchar  values[4];
  guchar  val;

  gint k;

  gdouble dx, dy;           /* X and Y Displacement, integer from GRAY map */
  gdouble r, theta, angled; /* Rect <-> Polar coordinate conversion variables */

  gint    xm_alpha = 0;
  gint    ym_alpha = 0;
  gint    mmag_alpha = 0;
  gint    xm_bytes = 1;
  gint    ym_bytes = 1;
  gint    mmag_bytes = 1;


  srand(time(NULL));                   /* seed random # generator */

  /* ================ Outer Loop calculation =================================== */

  /* Get selection area */

   gimp_drawable_mask_bounds (draw->id, &x1, &y1, &x2, &y2);
   width  = draw->width;
   height = draw->height;
   dest_bytes  = draw->bpp;

   max_progress = (x2 - x1) * (y2 - y1);

   gimp_pixel_rgn_init (&map_x_rgn, map_x, x1, y1, (x2 - x1), (y2 - y1), FALSE, FALSE);
   if (gimp_drawable_has_alpha(map_x->id))
	xm_alpha = 1;
   xm_bytes = gimp_drawable_bpp(map_x->id);

   gimp_pixel_rgn_init (&map_y_rgn, map_y, x1, y1, (x2 - x1), (y2 - y1), FALSE, FALSE);
   if (gimp_drawable_has_alpha(map_y->id))
	ym_alpha = 1;
   ym_bytes = gimp_drawable_bpp(map_y->id);


   gimp_pixel_rgn_init (&src_rgn, draw, x1, y1, (x2 - x1), (y2 - y1), FALSE, FALSE);

   /* only update shadow tiles (& push undo-stack) the first time through. Thanks Spencer! */
   if (first_time==TRUE)
     gimp_pixel_rgn_init (&dest_rgn, draw, x1, y1, (x2 - x1), (y2 - y1), TRUE, TRUE);
   else
     gimp_pixel_rgn_init (&dest_rgn, draw, x1, y1, (x2 - x1), (y2 - y1), TRUE, FALSE);

  /*  Register the pixel region  */

   if (dvals.mag_use == TRUE) {
     gimp_pixel_rgn_init (&mag_rgn, mag_draw, x1, y1, (x2 - x1), (y2 - y1), FALSE, FALSE);
     if (gimp_drawable_has_alpha(mag_draw->id))
       mmag_alpha = 1;
     mmag_bytes = gimp_drawable_bpp(mag_draw->id);

     pr = gimp_pixel_rgns_register (5, &dest_rgn, &src_rgn, &map_x_rgn, &map_y_rgn, &mag_rgn);
   } else {
     pr = gimp_pixel_rgns_register (4, &dest_rgn, &src_rgn, &map_x_rgn, &map_y_rgn);
   }

   for (pr = pr; pr != NULL; pr = gimp_pixel_rgns_process (pr))
    {  

      srcrow = src_rgn.data;
      destrow = dest_rgn.data;
      mxrow = map_x_rgn.data;
      myrow = map_y_rgn.data;
      if (dvals.mag_use == TRUE) 
	mmagrow = mag_rgn.data;
      
      angled = dvals.angle * M_PI / 180;
      
      /* loop over destination pixels */
      for (y = dest_rgn.y; y < (dest_rgn.y + dest_rgn.h); y++)
	{
	  src = srcrow;
	  dest = destrow;
	  mx = mxrow;
	  my = myrow;
          if (dvals.mag_use == TRUE) 
	    mmag = mmagrow;
	  
	  /*
	   * We could move the Displacement image address calculation out of here,
	   * but when we can have different sized Displacement and destination
	   * images we'd have to move it back anyway.
	   */
	  
	  for (x = dest_rgn.x; x < (dest_rgn.x + dest_rgn.w); x++)
	    {
              dx = (warp_map_give_value(mx, xm_alpha, xm_bytes)-127.5) / 127.5; 
	      dy = (warp_map_give_value(my, ym_alpha, ym_bytes)-127.5) / 127.5;

              r = sqrt(dx*dx + dy*dy);
              theta = atan2(dy,dx);
              theta += angled;
                 
	      /* here is the displacement vector */
	      amnt_x = dvals.amount * r * cos(theta);
              amnt_y = dvals.amount * r * sin(theta);

	      if (dvals.mag_use == TRUE) {
		scalefac = warp_map_mag_give_value(mmag, mmag_alpha, mmag_bytes)/255.0;
		amnt_x *= scalefac;
		amnt_y *= scalefac;
	      }
	      
	      if (dvals.dither != 0.0) {       /* random dither is +/- dvals.dither pixels */
		amnt_x += dvals.dither*((gdouble)(rand() - (RAND_MAX >> 1)) / (RAND_MAX >> 1));
		amnt_y += dvals.dither*((gdouble)(rand() - (RAND_MAX >> 1)) / (RAND_MAX >> 1));
	      }

	      needx = x + amnt_x;
	      needy = y + amnt_y;

	      mx += xm_bytes;
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

   if (tile)
    gimp_tile_unref (tile, FALSE);

     /*  update the region  */
   gimp_drawable_flush (draw);

   if (first_time==TRUE)
       gimp_drawable_merge_shadow(draw->id, TRUE);
  
   gimp_drawable_update (draw->id, x1, y1, (x2 - x1), (y2 - y1));

     /* if (run_mode != RUN_NONINTERACTIVE) */
   gimp_displays_flush();


} /* warp_one */


static gdouble
warp_map_give_value(guchar *pt, gint alpha, gint bytes)
{
  gdouble ret, val_alpha;
  
  if (bytes >= 3)
    ret =  0.30 * pt[0] + 0.59 * pt[1] + 0.11 * pt[2];
  else
    ret = (gdouble) *pt;
  
  if (alpha)
    {
      val_alpha = pt[bytes - 1];
      ret = ((ret - 127.5) * val_alpha / 255.0) + 127.5;
    };
  
  return (ret);
}

static gdouble
warp_map_mag_give_value(guchar *pt, gint alpha, gint bytes)
{
  gdouble ret, val_alpha;
  
  if (bytes >= 3)
    ret =  0.30 * pt[0] + 0.59 * pt[1] + 0.11 * pt[2];
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
warp_pixel (GDrawable * drawable,
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
		guchar *    pixel)
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
bilinear (gdouble x, gdouble y, guchar *v)
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
warp_map_callback (gint32     id,
			 gpointer   data)
{
  dvals.warp_map = id;
}

static void
warp_map_mag_callback (gint32     id,
			 gpointer   data)
{
  dvals.mag_map = id;
}

static void
warp_close_callback (GtkWidget *widget,
			 gpointer   data)
{
  gtk_main_quit ();
}

static void
warp_ok_callback (GtkWidget *widget,
		      gpointer   data)
{
  dint.run = TRUE;
  gtk_widget_destroy (GTK_WIDGET (data));
}

static void
warp_toggle_update (GtkWidget *widget,
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
warp_entry_callback (GtkWidget *widget,
			 gpointer   data)
{
  double *text_val;

  text_val = (double *) data;

  *text_val = atof (gtk_entry_get_text (GTK_ENTRY (widget)));
}
