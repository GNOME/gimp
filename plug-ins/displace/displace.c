/* Displace --- image filter plug-in for The Gimp image manipulation program
 * Copyright (C) 1996 Stephen Robert Norris
 * Much of the code taken from the pinch plug-in by 1996 Federico Mena Quintero
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
 * You can contact me at srn@flibble.cs.su.oz.au.
 * Please send me any patches or enhancements to this code.
 * You can contact the original The Gimp authors at gimp@xcf.berkeley.edu
 *
 * Extensive modifications to the dialog box, parameters, and some
 * legibility stuff in displace() by Federico Mena Quintero ---
 * federico@nuclecu.unam.mx.  If there are any bugs in these
 * changes, they are my fault and not Stephen's.
 *
 * JTL: May 29th 1997
 * Added (part of) the patch from Eiichi Takamori -- the part which removes the border artefacts
 * (http://ha1.seikyou.ne.jp/home/taka/gimp/displace/displace.html)
 * Added ability to use transparency as the identity transformation
 * (Full transparency is treated as if it was grey 0.5)
 * and the possibility to use RGB/RGBA pictures where the intensity of the pixel is taken into account
 *
 */


/* Version 1.12. */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <unistd.h>
#include <signal.h>
#include "gtk/gtk.h"
#include "libgimp/gimp.h"
#include "libgimp/gimpui.h"


/* Some useful macros */

#define ENTRY_WIDTH 75
#define TILE_CACHE_SIZE 48

#define WRAP   0
#define SMEAR  1
#define BLACK  2

typedef struct {
  gdouble amount_x;
  gdouble amount_y;
  gint    do_x;
  gint    do_y;
  gint    displace_map_x;
  gint    displace_map_y;
  gint    displace_type;
} DisplaceVals;

typedef struct {
  GtkWidget *amount_x;
  GtkWidget *amount_y;
  GtkWidget *menu_x;
  GtkWidget *menu_y;

  gint run;
} DisplaceInterface;

/*
 * Function prototypes.
 */

static void      query  (void);
static void      run    (gchar    *name,
			 gint      nparams,
			 GParam   *param,
			 gint     *nreturn_vals,
			 GParam  **return_vals);

static void      displace        (GDrawable *drawable);
static gint      displace_dialog (GDrawable *drawable);
static GTile *   displace_pixel  (GDrawable * drawable,
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

static gint      displace_map_constrain    (gint32     image_id,
					    gint32     drawable_id,
					    gpointer   data);
static void      displace_map_x_callback   (gint32     id,
					    gpointer   data);
static void      displace_map_y_callback   (gint32     id,
					    gpointer   data);
static void      displace_close_callback   (GtkWidget *widget,
					    gpointer   data);
static void      displace_ok_callback      (GtkWidget *widget,
					    gpointer   data);
static void      displace_toggle_update    (GtkWidget *widget,
					    gpointer   data);
static void      displace_x_toggle_update  (GtkWidget *widget,
					    gpointer   data);
static void      displace_y_toggle_update  (GtkWidget *widget,
					    gpointer   data);
static void      displace_entry_callback   (GtkWidget *widget,
					    gpointer   data);
static gdouble   displace_map_give_value   (guchar* ptr,
					    gint    alpha,
					    gint    bytes);
/***** Local vars *****/

GPlugInInfo PLUG_IN_INFO =
{
  NULL,    /* init_proc */
  NULL,    /* quit_proc */
  query,   /* query_proc */
  run,     /* run_proc */
};

static DisplaceVals dvals =
{
  20.0,    /* amount_x */
  20.0,    /* amount_y */
  TRUE,    /* do_x */
  TRUE,    /* do_y */
  -1,      /* displace_map_x */
  -1,      /* displace_map_y */
  WRAP     /* displace_type */
};

static DisplaceInterface dint =
{
  NULL,   /*  amount_x  */
  NULL,   /*  amount_y  */
  NULL,   /*  menu_x  */
  NULL,   /*  menu_y  */
  FALSE   /*  run  */
};

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
    { PARAM_FLOAT, "amount_x", "Displace multiplier for X direction" },
    { PARAM_FLOAT, "amount_y", "Displace multiplier for Y direction" },
    { PARAM_INT32, "do_x", "Displace in X direction?" },
    { PARAM_INT32, "do_y", "Displace in Y direction?" },
    { PARAM_DRAWABLE, "displace_map_x", "Displacement map for X direction" },
    { PARAM_DRAWABLE, "displace_map_y", "Displacement map for Y direction" },
    { PARAM_INT32, "displace_type", "Edge behavior: { WRAP (0), SMEAR (1), BLACK (2) }" }
  };
  static GParamDef *return_vals = NULL;
  static gint nargs = sizeof (args) / sizeof (args[0]);
  static gint nreturn_vals = 0;

  gimp_install_procedure ("plug_in_displace",
			  "Displace the contents of the specified drawable",
			  "Displaces the contents of the specified drawable by the amounts specified by 'amount_x' and 'amount_y' multiplied by the intensity of corresponding pixels in the 'displace_map' drawables.  Both 'displace_map' drawables must be of type GRAY_IMAGE for this operation to succeed.",
			  "Stephen Robert Norris & (ported to 1.0 by) Spencer Kimball",
			  "Stephen Robert Norris",
			  "1996",
			  "<Image>/Filters/Map/Displace",
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
  GRunModeType run_mode;
  GStatusType status = STATUS_SUCCESS;

  run_mode = param[0].data.d_int32;

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
      gimp_get_data ("plug_in_displace", &dvals);

      /*  First acquire information with a dialog  */
      if (! displace_dialog (drawable))
	return;
      break;

    case RUN_NONINTERACTIVE:
      /*  Make sure all the arguments are there!  */
      if (nparams != 10)
	status = STATUS_CALLING_ERROR;
      if (status == STATUS_SUCCESS)
	{
	  dvals.amount_x = param[3].data.d_float;
	  dvals.amount_y = param[4].data.d_float;
	  dvals.do_x = param[5].data.d_int32;
	  dvals.do_y = param[6].data.d_int32;
	  dvals.displace_map_x = param[7].data.d_int32;
	  dvals.displace_map_y = param[8].data.d_int32;
	  dvals.displace_type = param[9].data.d_int32;
	}
      break;

    case RUN_WITH_LAST_VALS:
      /*  Possibly retrieve data  */
      gimp_get_data ("plug_in_displace", &dvals);
      break;

    default:
      break;
    }

  if (status == STATUS_SUCCESS && (dvals.do_x || dvals.do_y))
    {
      gimp_progress_init ("Displacing...");

      /*  set the tile cache size  */
      gimp_tile_cache_ntiles (TILE_CACHE_SIZE);

      /*  run the displace effect  */
      displace (drawable);

      if (run_mode != RUN_NONINTERACTIVE)
	gimp_displays_flush ();

      /*  Store data  */
      if (run_mode == RUN_INTERACTIVE)
	gimp_set_data ("plug_in_displace", &dvals, sizeof (DisplaceVals));
    }

  values[0].data.d_status = status;

  gimp_drawable_detach (drawable);
}

static int
displace_dialog (GDrawable *drawable)
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
  GtkWidget *menu;
  GSList *group = NULL;
  gchar **argv;
  gchar buffer[32];
  gint argc;
  gint use_wrap = (dvals.displace_type == WRAP);
  gint use_smear = (dvals.displace_type == SMEAR);
  gint use_black = (dvals.displace_type == BLACK);

  argc = 1;
  argv = g_new (gchar *, 1);
  argv[0] = g_strdup ("displace");

#if 0
  printf("displace: pid = %d\n", (int)getpid());
  kill(getpid(), SIGSTOP);
#endif 

  gtk_init (&argc, &argv);
  gtk_rc_parse(gimp_gtkrc());
  dlg = gtk_dialog_new ();
  gtk_window_set_title (GTK_WINDOW (dlg), "Displace");
  gtk_window_position (GTK_WINDOW (dlg), GTK_WIN_POS_MOUSE);
  gtk_signal_connect (GTK_OBJECT (dlg), "destroy",
		      (GtkSignalFunc) displace_close_callback,
		      NULL);

  /*  Action area  */
  button = gtk_button_new_with_label ("OK");
  GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT);
  gtk_signal_connect (GTK_OBJECT (button), "clicked",
                      (GtkSignalFunc) displace_ok_callback,
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
  frame = gtk_frame_new ("Displace Options");
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_ETCHED_IN);
  gtk_container_border_width (GTK_CONTAINER (frame), 10);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dlg)->vbox), frame, TRUE, TRUE, 0);

  table = gtk_table_new (3, 3, FALSE);
  gtk_container_border_width (GTK_CONTAINER (table), 5);
  gtk_container_add (GTK_CONTAINER (frame), table);

  gtk_table_set_row_spacing (GTK_TABLE (table), 0, 10);
  gtk_table_set_row_spacing (GTK_TABLE (table), 1, 10);
  gtk_table_set_col_spacing (GTK_TABLE (table), 0, 10);
  gtk_table_set_col_spacing (GTK_TABLE (table), 1, 10);

  /*  on_x, on_y  */
  toggle = gtk_check_button_new_with_label ("X Displacement: ");
  gtk_table_attach (GTK_TABLE (table), toggle, 0, 1, 0, 1, GTK_FILL, GTK_FILL, 0, 0);
  gtk_signal_connect (GTK_OBJECT (toggle), "toggled",
		      (GtkSignalFunc) displace_x_toggle_update,
		      &dvals.do_x);
  gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (toggle), dvals.do_x);
  gtk_widget_show (toggle);

  toggle = gtk_check_button_new_with_label ("Y Displacement: ");
  gtk_table_attach (GTK_TABLE (table), toggle, 0, 1, 1, 2, GTK_FILL, GTK_FILL, 0, 0);
  gtk_signal_connect (GTK_OBJECT (toggle), "toggled",
		      (GtkSignalFunc) displace_y_toggle_update,
		      &dvals.do_y);
  gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (toggle), dvals.do_y);
  gtk_widget_show (toggle);

  /*  amount_x, amount_y  */
  dint.amount_x = entry = gtk_entry_new ();
  gtk_table_attach (GTK_TABLE (table), entry, 1, 2, 0, 1, GTK_FILL, GTK_FILL, 0, 0);
  gtk_widget_set_usize (entry, ENTRY_WIDTH, 0);
  sprintf (buffer, "%f", dvals.amount_x);
  gtk_entry_set_text (GTK_ENTRY (entry), buffer);
  gtk_signal_connect (GTK_OBJECT (entry), "changed",
		      (GtkSignalFunc) displace_entry_callback,
		      &dvals.amount_x);
  
  gtk_widget_set_sensitive (dint.amount_x, dvals.do_x);
  gtk_widget_show (entry);

  dint.amount_y = entry = gtk_entry_new ();
  gtk_table_attach (GTK_TABLE (table), entry, 1, 2, 1, 2, GTK_FILL, GTK_FILL, 0, 0);
  gtk_widget_set_usize (entry, ENTRY_WIDTH, 0);
  sprintf (buffer, "%f", dvals.amount_y);
  gtk_entry_set_text (GTK_ENTRY (entry), buffer);
  gtk_signal_connect (GTK_OBJECT (entry), "changed",
		      (GtkSignalFunc) displace_entry_callback,
		      &dvals.amount_y);
  gtk_widget_set_sensitive (dint.amount_y, dvals.do_y);
  gtk_widget_show (entry);

  /*  menu_x, menu_y  */
  dint.menu_x = option_menu = gtk_option_menu_new ();
  gtk_table_attach (GTK_TABLE (table), option_menu, 2, 3, 0, 1,
		    GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL, 0, 0);
  menu = gimp_drawable_menu_new (displace_map_constrain, displace_map_x_callback,
				 drawable, dvals.displace_map_x);
  gtk_option_menu_set_menu (GTK_OPTION_MENU (option_menu), menu);
  gtk_widget_set_sensitive (dint.menu_x, dvals.do_x);
  gtk_widget_show (option_menu);

  dint.menu_y = option_menu = gtk_option_menu_new ();
  gtk_table_attach (GTK_TABLE (table), option_menu, 2, 3, 1, 2,
		    GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL, 0, 0);
  menu = gimp_drawable_menu_new (displace_map_constrain, displace_map_y_callback,
				 drawable, dvals.displace_map_y);
  gtk_option_menu_set_menu (GTK_OPTION_MENU (option_menu), menu);
  gtk_widget_set_sensitive (dint.menu_y, dvals.do_y);
  gtk_widget_show (option_menu);

  /*  Displacement Type  */
  toggle_hbox = gtk_hbox_new (FALSE, 10);
  gtk_container_border_width (GTK_CONTAINER (toggle_hbox), 5);
  gtk_table_attach (GTK_TABLE (table), toggle_hbox, 0, 3, 2, 3, GTK_FILL, GTK_FILL, 0, 0);

  label = gtk_label_new ("On Edges: ");
  gtk_box_pack_start (GTK_BOX (toggle_hbox), label, FALSE, FALSE, 0);
  gtk_widget_show (label);

  toggle = gtk_radio_button_new_with_label (group, "Wrap");
  group = gtk_radio_button_group (GTK_RADIO_BUTTON (toggle));
  gtk_box_pack_start (GTK_BOX (toggle_hbox), toggle, FALSE, FALSE, 0);
  gtk_signal_connect (GTK_OBJECT (toggle), "toggled",
		      (GtkSignalFunc) displace_toggle_update,
		      &use_wrap);
  gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (toggle), use_wrap);
  gtk_widget_show (toggle);

  toggle = gtk_radio_button_new_with_label (group, "Smear");
  group = gtk_radio_button_group (GTK_RADIO_BUTTON (toggle));
  gtk_box_pack_start (GTK_BOX (toggle_hbox), toggle, FALSE, FALSE, 0);
  gtk_signal_connect (GTK_OBJECT (toggle), "toggled",
		      (GtkSignalFunc) displace_toggle_update,
		      &use_smear);
  gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (toggle), use_smear);
  gtk_widget_show (toggle);

  toggle = gtk_radio_button_new_with_label (group, "Black");
  group = gtk_radio_button_group (GTK_RADIO_BUTTON (toggle));
  gtk_box_pack_start (GTK_BOX (toggle_hbox), toggle, FALSE, FALSE, 0);
  gtk_signal_connect (GTK_OBJECT (toggle), "toggled",
		      (GtkSignalFunc) displace_toggle_update,
		      &use_black);
  gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (toggle), use_black);
  gtk_widget_show (toggle);

  gtk_widget_show (toggle_hbox);
  gtk_widget_show (table);
  gtk_widget_show (frame);
  gtk_widget_show (dlg);

  gtk_main ();
  gdk_flush ();

  /*  determine displace type  */
  if (use_wrap)
    dvals.displace_type = WRAP;
  else if (use_smear)
    dvals.displace_type = SMEAR;
  else if (use_black)
    dvals.displace_type = BLACK;

  return dint.run;
}

/* The displacement is done here. */

static void
displace (GDrawable *drawable)
{
  GDrawable *map_x;
  GDrawable *map_y;
  GPixelRgn dest_rgn;
  GPixelRgn map_x_rgn;
  GPixelRgn map_y_rgn;
  GTile   * tile = NULL;
  gint      row = -1;
  gint      col = -1;
  gpointer  pr;

  gint    width;
  gint    height;
  gint    bytes;
  guchar *destrow, *dest;
  guchar *mxrow, *mx;
  guchar *myrow, *my;
  guchar  pixel[4][4];
  gint    x1, y1, x2, y2;
  gint    x, y;
  gint    progress, max_progress;

  gdouble amnt;
  gdouble needx, needy;
  gint    xi, yi;

  guchar  values[4];
  guchar  val;

  gint k;

  gdouble xm_val, ym_val;
  gint    xm_alpha = 0;
  gint    ym_alpha = 0;
  gint    xm_bytes = 1;
  gint    ym_bytes = 1;

  /* initialize */

  mxrow = NULL;
  myrow = NULL;
  
  /* Get selection area */
  gimp_drawable_mask_bounds (drawable->id, &x1, &y1, &x2, &y2);

  width  = drawable->width;
  height = drawable->height;
  bytes  = drawable->bpp;

  progress     = 0;
  max_progress = (x2 - x1) * (y2 - y1);

  /*
   * The algorithm used here is simple - see
   * http://the-tech.mit.edu/KPT/Tips/KPT7/KPT7.html for a description.
   */

  /* Get the drawables  */
  if (dvals.displace_map_x != -1 && dvals.do_x)
    {
      map_x = gimp_drawable_get (dvals.displace_map_x);
      gimp_pixel_rgn_init (&map_x_rgn, map_x, x1, y1, (x2 - x1), (y2 - y1), FALSE, FALSE);
      if (gimp_drawable_has_alpha(map_x->id))
	xm_alpha = 1;
      xm_bytes = gimp_drawable_bpp(map_x->id);
    }
  else
    map_x = NULL;

  if (dvals.displace_map_y != -1 && dvals.do_y)
    {
      map_y = gimp_drawable_get (dvals.displace_map_y);
      gimp_pixel_rgn_init (&map_y_rgn, map_y, x1, y1, (x2 - x1), (y2 - y1), FALSE, FALSE);
      if (gimp_drawable_has_alpha(map_y->id))
	ym_alpha = 1;
      ym_bytes = gimp_drawable_bpp(map_y->id);
    }
  else
    map_y = NULL;

  gimp_pixel_rgn_init (&dest_rgn, drawable, x1, y1, (x2 - x1), (y2 - y1), TRUE, TRUE);

  /*  Register the pixel regions  */
  if (dvals.do_x && dvals.do_y)
    pr = gimp_pixel_rgns_register (3, &dest_rgn, &map_x_rgn, &map_y_rgn);
  else if (dvals.do_x)
    pr = gimp_pixel_rgns_register (2, &dest_rgn, &map_x_rgn);
  else if (dvals.do_y)
    pr = gimp_pixel_rgns_register (2, &dest_rgn, &map_y_rgn);
  else
    pr = NULL;

  for (pr = pr; pr != NULL; pr = gimp_pixel_rgns_process (pr))
    {
      destrow = dest_rgn.data;
      if (dvals.do_x)
	mxrow = map_x_rgn.data;
      if (dvals.do_y)
	myrow = map_y_rgn.data;
      
      for (y = dest_rgn.y; y < (dest_rgn.y + dest_rgn.h); y++)
	{
	  dest = destrow;
	  mx = mxrow;
	  my = myrow;
	  
	  /*
	   * We could move the displacement image address calculation out of here,
	   * but when we can have different sized displacement and destination
	   * images we'd have to move it back anyway.
	   */
	  
	  for (x = dest_rgn.x; x < (dest_rgn.x + dest_rgn.w); x++)
	    {
	      if (dvals.do_x)
		{
		  xm_val = displace_map_give_value(mx, xm_alpha, xm_bytes); 
		  amnt = dvals.amount_x * (xm_val - 127.5) / 127.5;
		  needx = x + amnt;
		  mx += xm_bytes;
		}
	      else
		needx = x;
	      
	      if (dvals.do_y)
		{
		  ym_val = displace_map_give_value(my, ym_alpha, ym_bytes);
		  amnt = dvals.amount_y * (ym_val - 127.5) / 127.5;
		  needy = y + amnt;
		  my += ym_bytes;
		}
	      else
		needy = y;
	      
	      /* Calculations complete; now copy the proper pixel */
	      
	      if (needx >= 0.0)
		      xi = (int) needx;
	      else
		      xi = -((int) -needx + 1);

	      if (needy >= 0.0)
		      yi = (int) needy;
	      else
		      yi = -((int) -needy + 1);
	      
	      tile = displace_pixel (drawable, tile, width, height, x1, y1, x2, y2, xi, yi, &row, &col, pixel[0]);
	      tile = displace_pixel (drawable, tile, width, height, x1, y1, x2, y2, xi + 1, yi, &row, &col, pixel[1]);
	      tile = displace_pixel (drawable, tile, width, height, x1, y1, x2, y2, xi, yi + 1, &row, &col, pixel[2]);
	      tile = displace_pixel (drawable, tile, width, height, x1, y1, x2, y2, xi + 1, yi + 1, &row, &col, pixel[3]);
	      
	      for (k = 0; k < bytes; k++)
		{
		  values[0] = pixel[0][k];
		  values[1] = pixel[1][k];
		  values[2] = pixel[2][k];
		  values[3] = pixel[3][k];
		  val = bilinear(needx, needy, values);
		  
		  *dest++ = val;
		} /* for */
	    }
	  
	  destrow += dest_rgn.rowstride;
	  
	  if (dvals.do_x)
	    mxrow += map_x_rgn.rowstride;
	  if (dvals.do_y)
	    myrow += map_y_rgn.rowstride;
	}
      
      progress += dest_rgn.w * dest_rgn.h;
      gimp_progress_update ((double) progress / (double) max_progress);
    } /* for */

  if (tile)
    gimp_tile_unref (tile, FALSE);

  /*  detach from the map drawables  */
  if (dvals.do_x)
    gimp_drawable_detach (map_x);
  if (dvals.do_y)
    gimp_drawable_detach (map_y);

  /*  update the region  */
  gimp_drawable_flush (drawable);
  gimp_drawable_merge_shadow (drawable->id, TRUE);
  gimp_drawable_update (drawable->id, x1, y1, (x2 - x1), (y2 - y1));
} /* displace */


static gdouble
displace_map_give_value(guchar *pt, gint alpha, gint bytes)
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


static GTile *
displace_pixel (GDrawable * drawable,
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
  if (dvals.displace_type == WRAP)
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
  else if (dvals.displace_type == SMEAR)
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
      if ((x >> 6 != *col) || (y >> 6 != *row))
	{
	  *col = x / 64;
	  *row = y / 64;
	  if (tile)
	    gimp_tile_unref (tile, FALSE);
	  tile = gimp_drawable_get_tile (drawable, FALSE, *row, *col);
	  gimp_tile_ref (tile);
	}

      data = tile->data + tile->bpp * (tile->ewidth * (y % 64) + (x % 64));
    }
  else
    data = empty_pixel;

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


/*  Displace interface functions  */

static gint
displace_map_constrain (gint32     image_id,
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
displace_map_x_callback (gint32     id,
			 gpointer   data)
{
  dvals.displace_map_x = id;
}

static void
displace_map_y_callback (gint32     id,
			 gpointer   data)
{
  dvals.displace_map_y = id;
}

static void
displace_close_callback (GtkWidget *widget,
			 gpointer   data)
{
  gtk_main_quit ();
}

static void
displace_ok_callback (GtkWidget *widget,
		      gpointer   data)
{
  dint.run = TRUE;
  gtk_widget_destroy (GTK_WIDGET (data));
}

static void
displace_toggle_update (GtkWidget *widget,
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
displace_x_toggle_update (GtkWidget *widget,
			  gpointer   data)
{
  int *toggle_val;

  toggle_val = (int *) data;

  if (GTK_TOGGLE_BUTTON (widget)->active)
    *toggle_val = TRUE;
  else
    *toggle_val = FALSE;

  if (dint.amount_x)
    gtk_widget_set_sensitive (dint.amount_x, *toggle_val);
  if (dint.menu_x)
    gtk_widget_set_sensitive (dint.menu_x, *toggle_val);
}

static void
displace_y_toggle_update (GtkWidget *widget,
			  gpointer   data)
{
  int *toggle_val;

  toggle_val = (int *) data;

  if (GTK_TOGGLE_BUTTON (widget)->active)
    *toggle_val = TRUE;
  else
    *toggle_val = FALSE;

  if (dint.amount_y)
    gtk_widget_set_sensitive (dint.amount_y, *toggle_val);
  if (dint.menu_y)
    gtk_widget_set_sensitive (dint.menu_y, *toggle_val);
}

static void
displace_entry_callback (GtkWidget *widget,
			 gpointer   data)
{
  double *text_val;

  text_val = (double *) data;

  *text_val = atof (gtk_entry_get_text (GTK_ENTRY (widget)));
}
