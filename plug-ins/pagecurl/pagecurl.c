/* Page Curl 0.9 --- image filter plug-in for The Gimp
 * Copyright (C) 1996 Federico Mena Quintero
 * Ported to Gimp 1.0 1998 by Simon Budig <Simon.Budig@unix-ag.org>
 *
 * You can contact me at quartic@polloux.fciencias.unam.mx
 * You can contact the original The Gimp authors at gimp@xcf.berkeley.edu
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
 */

/* TODO for v0.5 - in 0.9 still to do...
 * As of version 0.5 alpha, the only thing that is not yet implemented
 * is the "Warp curl" option.  Everything else seems to be working
 * just fine.  Please email me if you find any bugs.  I know that the
 * calculation code is horrible, but you don't want to tweak it anyway ;)
 */

/*
 * Ported to the 0.99.x architecture by Simon Budig, Simon.Budig@unix-ag.org
 *  *** Why does gimp_drawable_add_alpha cause the plugin to produce an
 *      ** WARNING **: received tile info did not match computed tile info
 *      ** WARNING **: expected tile ack and received: 0
 */

/*
 * Version History
 * 0.5: (1996) Version for Gimp 0.54 by Federico Mena Quintero
 * 0.6: (Feb '98) First Version for Gimp 0.99.x, very buggy.
 * 0.8: (Mar '98) First "stable" version
 * 0.9: (May '98)
 *      - Added support for Gradients. It is now possible to map
 *        a gradient to the back of the curl.
 *      - This implies a changed PDB-Interface: New "mode" parameter.
 *      - Pagecurl now returns the ID of the new layer.
 *      - Exchanged the meaning of FG/BG Color, because mostly the FG
 *        color is darker.
 */
#include "config.h"

#include <stdio.h>
#include <stdlib.h>

#include <gtk/gtk.h>
#include <gdk/gdk.h>
#include "curl0.xpm"
#include "curl1.xpm"
#include "curl2.xpm"
#include "curl3.xpm"
#include "curl4.xpm"
#include "curl5.xpm"
#include "curl6.xpm"
#include "curl7.xpm"
#include <libgimp/gimp.h>

#define PLUG_IN_NAME    "plug_in_pagecurl"
#define PLUG_IN_VERSION "May 1998, 0.9"
#define NGRADSAMPLES 256

/***** Macros *****/

/* Convert color to Gray ala Gimp... */
#define INTENSITY(r,g,b) (r * 0.30 + g * 0.59 + b * 0.11 + 0.001)


/***** Types *****/

typedef struct {
   double x, y;
} vector_t;

typedef struct {
   gint do_curl_shade;
   gint do_curl_gradient;
   gint do_curl_warp;  /* Not yet supported... */

   double do_curl_opacity;
   gint do_shade_under;

   gint do_upper_left;
   gint do_upper_right;
   gint do_lower_left;
   gint do_lower_right;

   gint do_vertical;
   gint do_horizontal;
} CurlParams;

/***** Prototypes *****/

static void set_default_params (void);

static void dialog_close_callback (GtkWidget *, gpointer);
static void dialog_ok_callback (GtkWidget *, gpointer);
static void dialog_cancel_callback (GtkWidget *, gpointer);
static void dialog_toggle_update (GtkWidget *, gint);
static void dialog_scale_update (GtkAdjustment *, double *);

static void query (void);
static void run (gchar * name,
		 gint nparams,
		 GParam * param,
		 gint * nreturn_vals,
		 GParam ** return_vals);

static int do_dialog (void);

static void v_set (vector_t * v, double x, double y);
static void v_add (vector_t * v, vector_t a, vector_t b);
static void v_sub (vector_t * v, vector_t a, vector_t b);
static double v_mag (vector_t v);
static double v_dot (vector_t a, vector_t b);

static void init_calculation ();

static int left_of_diagl (double x, double y);
static int right_of_diagr (double x, double y);
static int below_diagb (double x, double y);
static int right_of_diagm (double x, double y);
static int inside_circle (double x, double y);

static void do_curl_effect (void);
static void clear_curled_region (void);
static void page_curl (void);
static guchar *get_samples (GDrawable *drawable);

/***** Variables *****/

GPlugInInfo PLUG_IN_INFO = {
   NULL,			/* init_proc */
   NULL,			/* quit_proc */
   query,			/* query_proc */
   run				/* run_proc */
};				/* PLUG_IN_INFO */

static CurlParams curl;

/* Image parameters */

gint32 image_id;
GDrawable *curl_layer;
GDrawable *drawable;
GDrawable *layer_mask;

typedef GdkPixmap *GdkPmP;
GdkPmP gdk_curl_pixmaps[8];

typedef GdkBitmap *GdkBmP;
GdkBmP gdk_curl_masks[8];

GtkWidget *curl_pixmap_widget;

gint sel_x1, sel_y1, sel_x2, sel_y2;
gint true_sel_width, true_sel_height;
gint sel_width, sel_height;
gint drawable_position;
static gint curl_run = FALSE;
gint32 curl_layer_ID;

/* Center and radius of circle */

vector_t center;
double radius;

/* Useful points to keep around */

vector_t left_tangent;
vector_t right_tangent;

/* Slopes --- these are *not* in the usual geometric sense! */

double diagl_slope;
double diagr_slope;
double diagb_slope;
double diagm_slope;

/* User-configured parameters */

guchar fore_color[3];
guchar back_color[3];


/***** Functions *****/

/****/
MAIN ()
/****/

/*************************/
/* Some Vector-functions */

static void v_set (vector_t * v, double x, double y) {
   v->x = x;
   v->y = y;
}				/* v_set */

static void v_add (vector_t * v, vector_t a, vector_t b) {
   v->x = a.x + b.x;
   v->y = a.y + b.y;
}				/* v_add */

static void v_sub (vector_t * v, vector_t a, vector_t b) {
   v->x = a.x - b.x;
   v->y = a.y - b.y;
}				/* v_sub */

static double v_mag (vector_t v) {
   return sqrt (v.x * v.x + v.y * v.y);
}				/* v_mag */

static double v_dot (vector_t a, vector_t b) {
   return a.x * b.x + a.y * b.y;
}				/* v_dot */


/*****************************************************
 * Functions to locate the current point in the curl.
 *  The following functions assume an curl in the
 *  lower right corner.
 *  diagb crosses the two tangential points from the
 *  circle with diagl and diagr.
 *
 *   +------------------------------------------------+
 *   |                                          __-- /|
 *   |                                      __--   _/ |
 *   |                                  __--      /   |
 *   |                              __--        _/    |
 *   |                          __--           /      |
 *   |                      __--____         _/       |
 *   |                  __----~~    \      _/         |
 *   |              __--             |   _/           |
 *   |    diagl __--               _/| _/diagr        |
 *   |      __--            diagm_/  |/               |
 *   |  __--                    /___/                 |
 *   +-------------------------+----------------------+
 */

static int left_of_diagl (double x, double y) {
   return (x < (sel_width + (y - sel_height) * diagl_slope));
}				/* left_of_diagl */

static int right_of_diagr (double x, double y) {
   return (x > (sel_width + (y - sel_height) * diagr_slope));
}				/* right_of_diagr */

static int below_diagb (double x, double y) {
   return (y < (right_tangent.y + (x - right_tangent.x) * diagb_slope));
}				/* below_diagb */

static int right_of_diagm (double x, double y) {
   return (x > (sel_width + (y - sel_height) * diagm_slope));
}				/* right_of_diagm */

static int inside_circle (double x, double y) {
   double dx, dy;
   dx = x - center.x;
   dy = y - center.y;
   return (sqrt (dx * dx + dy * dy) <= radius);
}				/* inside_circle */


/*****/

static void query (void) {
   static GParamDef args[] = {
      {PARAM_INT32, "run_mode", "Interactive (0), non-interactive (1)"},
      {PARAM_IMAGE, "image", "Input image"},
      {PARAM_DRAWABLE, "drawable", "Input drawable"},
      {PARAM_INT32, "mode", "Pagecurl-mode: Use FG- and BG-Color (0), Use current gradient (1)"},
      {PARAM_INT32, "edge", "Edge to curl (1-4, clockwise, starting in the lower right edge)"},
      {PARAM_INT32, "type", "vertical (0), horizontal (1)"},
      {PARAM_INT32, "shade", "Shade the region under the curl (1) or not (0)"},
   };				/* args */

   static GParamDef return_vals[] = {
      {PARAM_LAYER, "Curl layer", "The new layer with the curl." }
   }; 
   static int nargs = sizeof (args) / sizeof (args[0]);
   static int nreturn_vals = sizeof (return_vals) / sizeof (return_vals[0]);

   gimp_install_procedure (PLUG_IN_NAME,
			   "Pagecurl effect",
			   "This plug-in creates an pagecurl-effect.",
			   "Federico Mena Quintero and Simon Budig",
			   "Federico Mena Quintero and Simon Budig",
			   PLUG_IN_VERSION,
			   "<Image>/Filters/Distorts/Pagecurl",
			   "RGBA*, GRAYA*",
			   PROC_PLUG_IN,
			   nargs,
			   nreturn_vals,
			   args,
			   return_vals);
}				/* query */

static void run (gchar * name,
		 gint nparams,
		 GParam * param,
		 gint * nreturn_vals,
		 GParam ** return_vals) {
   static GParam values[2];
   GRunModeType run_mode;
   GStatusType status = STATUS_SUCCESS;

   run_mode = param[0].data.d_int32;

   set_default_params ();

   /*  Possibly retrieve data  */
   gimp_get_data (PLUG_IN_NAME, &curl);

   *nreturn_vals = 2;
   *return_vals = values;

   values[0].type = PARAM_STATUS;
   values[0].data.d_status = status;
   values[1].type = PARAM_LAYER;
   values[1].data.d_layer = -1;

   /*  Get the specified drawable  */
   drawable = gimp_drawable_get (param[2].data.d_drawable);
   image_id = param[1].data.d_image;

   if ((gimp_drawable_color (drawable->id)
	|| gimp_drawable_gray (drawable->id))
       && gimp_drawable_has_alpha (drawable->id)) {

      switch (run_mode) {
      case RUN_INTERACTIVE:
	 /*  First acquire information with a dialog  */
	 if (!do_dialog ())
	    return;
	 break;

      case RUN_NONINTERACTIVE:
	 /*  Make sure all the arguments are there!  */
	 if (nparams != 7)
	    status = STATUS_CALLING_ERROR;
	 if (status == STATUS_SUCCESS) {
	    curl.do_curl_shade = (param[3].data.d_int32 == 0) ? 1 : 0;
	    curl.do_curl_gradient = 1 - curl.do_curl_shade;
	    switch (param[4].data.d_int32) {
	    case 1:
	       curl.do_upper_left = 0;
	       curl.do_upper_right = 0;
	       curl.do_lower_left = 0;
	       curl.do_lower_right = 1;
	       break;
	    case 2:
	       curl.do_upper_left = 0;
	       curl.do_upper_right = 0;
	       curl.do_lower_left = 1;
	       curl.do_lower_right = 0;
	       break;
	    case 3:
	       curl.do_upper_left = 1;
	       curl.do_upper_right = 0;
	       curl.do_lower_left = 0;
	       curl.do_lower_right = 0;
	       break;
	    case 4:
	       curl.do_upper_left = 0;
	       curl.do_upper_right = 1;
	       curl.do_lower_left = 0;
	       curl.do_lower_right = 0;
	       break;
	    default:
	       break;
	    }
	    curl.do_vertical = (param[5].data.d_int32) ? 0 : 1;
	    curl.do_horizontal = 1 - curl.do_vertical;
	    curl.do_shade_under = (param[6].data.d_int32) ? 1 : 0;
	 }
	 break;

      case RUN_WITH_LAST_VALS:
	 break;

      default:
	 break;
      }

      if (status == STATUS_SUCCESS) {
         page_curl ();
         values[1].data.d_layer = curl_layer_ID;
         if (run_mode != RUN_NONINTERACTIVE)
            gimp_displays_flush ();
         if (run_mode == RUN_INTERACTIVE)
            gimp_set_data (PLUG_IN_NAME, &curl, sizeof (CurlParams));
      }
   } else
      /* Sorry - no indexed/noalpha images */
      status = STATUS_EXECUTION_ERROR;

   values[0].data.d_status = status;
}				/* run */


/*****/

static void set_default_params (void) {
   curl.do_curl_shade = 1;
   curl.do_curl_gradient = 0;
   curl.do_curl_warp = 0;  /* Not yet supported... */

   curl.do_curl_opacity = 1.0;
   curl.do_shade_under = 1;

   curl.do_upper_left = 0;
   curl.do_upper_right = 0;
   curl.do_lower_left = 0;
   curl.do_lower_right = 1;

   curl.do_vertical = 1;
   curl.do_horizontal = 0;
}				/* set_default_params */



/********************************************************************/
/* dialog callbacks                                                 */
/********************************************************************/

static void dialog_close_callback (GtkWidget * widget, gpointer data) {
   gtk_main_quit ();
}

static void dialog_ok_callback (GtkWidget * widget, gpointer data) {
   curl_run = TRUE;
   gtk_widget_destroy (GTK_WIDGET (data));
}

static void dialog_cancel_callback (GtkWidget * widget, gpointer data) {
   gtk_widget_destroy (GTK_WIDGET (data));
}

static void dialog_scale_update (GtkAdjustment * adjustment, double *value) {
   *value = ((double) adjustment->value) / 100.0;
}

static void dialog_toggle_update (GtkWidget * widget, gint32 value) {
   gint pixmapindex = 0;

   switch (value) {
   case 0:
      curl.do_upper_left = (GTK_TOGGLE_BUTTON (widget)->active) ? 1 : 0;
      break;
   case 1:
      curl.do_upper_right = (GTK_TOGGLE_BUTTON (widget)->active) ? 1 : 0;
      break;
   case 2:
      curl.do_lower_left = (GTK_TOGGLE_BUTTON (widget)->active) ? 1 : 0;
      break;
   case 3:
      curl.do_lower_right = (GTK_TOGGLE_BUTTON (widget)->active) ? 1 : 0;
      break;
   case 5:
      curl.do_horizontal = (GTK_TOGGLE_BUTTON (widget)->active) ? 1 : 0;
      break;
   case 6:
      curl.do_vertical = (GTK_TOGGLE_BUTTON (widget)->active) ? 1 : 0;
      break;
   case 8:
      curl.do_shade_under = (GTK_TOGGLE_BUTTON (widget)->active) ? 1 : 0;
      return;
      break;
   case 9:
      curl.do_curl_gradient = (GTK_TOGGLE_BUTTON (widget)->active) ? 1 : 0;
      curl.do_curl_shade = (GTK_TOGGLE_BUTTON (widget)->active) ? 0 : 1;
      return;
      break;
   default:
      break;
   }

   if (curl.do_upper_left + curl.do_upper_right + curl.do_lower_left + curl.do_lower_right == 1 &&
       curl.do_horizontal + curl.do_vertical == 1) {
      pixmapindex = curl.do_lower_left + curl.do_upper_right * 2 +
	 curl.do_upper_left * 3 + curl.do_horizontal * 4;
      if (pixmapindex < 0 || pixmapindex > 7)
	 pixmapindex = 0;
      gtk_pixmap_set (GTK_PIXMAP (curl_pixmap_widget),
		      gdk_curl_pixmaps[pixmapindex],
		      gdk_curl_masks[pixmapindex]);
   }
}

/*********/

static int do_dialog (void) {

   /* Missing options: Color-dialogs? / own curl layer ? / transparency
      to original drawable / Warp-curl (unsupported yet) */

   GtkWidget *dialog;
   GtkWidget *orhbox1, *orhbox2, *vbox, *ivbox, *corner_frame, *orient_frame;
   GtkWidget *shade_button, *gradient_button, *button, *label, *scale;
   GtkStyle *style;
   GtkObject *adjustment;
   gint pixmapindex;

   gint argc;
   gchar **argv;

   argc = 1;
   argv = g_new (gchar *, 1);
   argv[0] = g_strdup ("pagecurl");

   gtk_init (&argc, &argv);
   gtk_rc_parse (gimp_gtkrc ());
   gdk_set_use_xshm (gimp_use_xshm ());

   dialog = gtk_dialog_new ();
   gtk_window_set_title (GTK_WINDOW (dialog), "Pagecurl effect");
   gtk_container_border_width (GTK_CONTAINER (dialog), 0);
   gtk_widget_realize (dialog);
   gtk_signal_connect (GTK_OBJECT (dialog), "destroy",
		       (GtkSignalFunc) dialog_close_callback,
		       NULL);

   button = gtk_button_new_with_label ("OK");
   GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT);
   gtk_signal_connect (GTK_OBJECT (button), "clicked",
		       (GtkSignalFunc) dialog_ok_callback,
		       dialog);
   gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog)->action_area), button, TRUE, TRUE, 0);
   gtk_widget_grab_default (button);
   gtk_widget_show (button);

   button = gtk_button_new_with_label ("Cancel");
   GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT);
   gtk_signal_connect (GTK_OBJECT (button), "clicked",
		       (GtkSignalFunc) dialog_cancel_callback,
		       dialog);
   gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog)->action_area), button, TRUE, TRUE, 0);
   gtk_widget_show (button);

   vbox = gtk_vbox_new (FALSE, 5);
   gtk_container_border_width (GTK_CONTAINER (vbox), 5);
   gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog)->vbox),
		       vbox, FALSE, FALSE, 0);

/*****/

   orhbox1 = gtk_hbox_new (FALSE, 5);
   gtk_container_border_width (GTK_CONTAINER (orhbox1), 0);
   gtk_container_add (GTK_CONTAINER (vbox), orhbox1);

   style = gtk_widget_get_style (dialog);
   gdk_curl_pixmaps[0] = gdk_pixmap_create_from_xpm_d (dialog->window,
	     &(gdk_curl_masks[0]), &style->bg[GTK_STATE_NORMAL], curl0_xpm);
   gdk_curl_pixmaps[1] = gdk_pixmap_create_from_xpm_d (dialog->window,
	     &(gdk_curl_masks[1]), &style->bg[GTK_STATE_NORMAL], curl1_xpm);
   gdk_curl_pixmaps[2] = gdk_pixmap_create_from_xpm_d (dialog->window,
	     &(gdk_curl_masks[2]), &style->bg[GTK_STATE_NORMAL], curl2_xpm);
   gdk_curl_pixmaps[3] = gdk_pixmap_create_from_xpm_d (dialog->window,
	     &(gdk_curl_masks[3]), &style->bg[GTK_STATE_NORMAL], curl3_xpm);
   gdk_curl_pixmaps[4] = gdk_pixmap_create_from_xpm_d (dialog->window,
	     &(gdk_curl_masks[4]), &style->bg[GTK_STATE_NORMAL], curl4_xpm);
   gdk_curl_pixmaps[5] = gdk_pixmap_create_from_xpm_d (dialog->window,
	     &(gdk_curl_masks[5]), &style->bg[GTK_STATE_NORMAL], curl5_xpm);
   gdk_curl_pixmaps[6] = gdk_pixmap_create_from_xpm_d (dialog->window,
	     &(gdk_curl_masks[6]), &style->bg[GTK_STATE_NORMAL], curl6_xpm);
   gdk_curl_pixmaps[7] = gdk_pixmap_create_from_xpm_d (dialog->window,
	     &(gdk_curl_masks[7]), &style->bg[GTK_STATE_NORMAL], curl7_xpm);

   pixmapindex = curl.do_lower_left + curl.do_upper_right * 2 +
      curl.do_upper_left * 3 + curl.do_horizontal * 4;
   if (pixmapindex < 0 || pixmapindex > 7)
      pixmapindex = 0;
   curl_pixmap_widget = gtk_pixmap_new (gdk_curl_pixmaps[pixmapindex],
					gdk_curl_masks[pixmapindex]);
   gtk_box_pack_start (GTK_BOX (orhbox1),
		       curl_pixmap_widget, TRUE, TRUE, 0);
   gtk_widget_show (curl_pixmap_widget);

   corner_frame = gtk_frame_new ("Curl location");
   gtk_frame_set_shadow_type (GTK_FRAME (corner_frame), GTK_SHADOW_ETCHED_IN);
   gtk_box_pack_start (GTK_BOX (orhbox1),
		       corner_frame, TRUE, TRUE, 0);

   ivbox = gtk_vbox_new (FALSE, 0);
   gtk_container_border_width (GTK_CONTAINER (ivbox), 5);
   gtk_container_add (GTK_CONTAINER (corner_frame), ivbox);

   {
      int i;
      char *name[4] =
      {"Upper left", "Upper right", "Lower left", "Lower right"};

      button = NULL;
      for (i = 0; i < 4; i++) {
	 button = gtk_radio_button_new_with_label (
		     (button == NULL) ? NULL : gtk_radio_button_group (GTK_RADIO_BUTTON (button)),
		     name[i]);
	 gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button),
	       (i == 0 ? curl.do_upper_left : i == 1 ? curl.do_upper_right :
		i == 2 ? curl.do_lower_left : curl.do_lower_right));

	 gtk_signal_connect (GTK_OBJECT (button), "toggled",
			     (GtkSignalFunc) dialog_toggle_update,
			     (gpointer) i);

	 gtk_box_pack_start (GTK_BOX (ivbox), button, FALSE, FALSE, 0);
	 gtk_widget_show (button);
      }
   }

   gtk_widget_show (ivbox);
   gtk_widget_show (corner_frame);
   gtk_widget_show (orhbox1);

/*****/

   orient_frame = gtk_frame_new ("Curl orientation");
   gtk_frame_set_shadow_type (GTK_FRAME (orient_frame), GTK_SHADOW_ETCHED_IN);
   gtk_box_pack_start (GTK_BOX (vbox),
		       orient_frame, FALSE, FALSE, 0);

   orhbox2 = gtk_hbox_new (FALSE, 5);
   gtk_container_border_width (GTK_CONTAINER (orhbox2), 5);
   gtk_container_add (GTK_CONTAINER (orient_frame), orhbox2);

   {
      int i;
      char *name[2] =
      {"horizontal", "vertical"};

      button = NULL;
      for (i = 0; i < 2; i++) {
	 button = gtk_radio_button_new_with_label (
		     (button == NULL) ? NULL : gtk_radio_button_group (GTK_RADIO_BUTTON (button)),
		     name[i]);
	 gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button),
			  (i == 0 ? curl.do_horizontal : curl.do_vertical));

	 gtk_signal_connect (GTK_OBJECT (button), "toggled",
			     (GtkSignalFunc) dialog_toggle_update,
			     (gpointer) (i + 5));

	 gtk_box_pack_start (GTK_BOX (orhbox2), button, TRUE, FALSE, 0);
	 gtk_widget_show (button);
      }
   }

   gtk_widget_show (orhbox2);
   gtk_widget_show (orient_frame);

   shade_button = gtk_check_button_new_with_label ("Shade under curl");
   gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (shade_button),
				curl.do_shade_under ? TRUE : FALSE);
   gtk_signal_connect (GTK_OBJECT (shade_button), "toggled",
		       (GtkSignalFunc) dialog_toggle_update, (gpointer) 8);
   gtk_box_pack_start (GTK_BOX (vbox), shade_button, TRUE, FALSE, 0);
   gtk_widget_show (shade_button);

   gradient_button = gtk_check_button_new_with_label ("Use current Gradient\n instead of FG/BG-Color");
   gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (gradient_button),
				curl.do_curl_gradient ? TRUE : FALSE);
   gtk_signal_connect (GTK_OBJECT (gradient_button), "toggled",
		       (GtkSignalFunc) dialog_toggle_update, (gpointer) 9);
   gtk_box_pack_start (GTK_BOX (vbox), gradient_button, TRUE, FALSE, 0);
   gtk_widget_show (gradient_button);


   label = gtk_label_new ("Curl opacity");
   gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
   gtk_box_pack_start (GTK_BOX (vbox), label, TRUE, FALSE, 0);
   gtk_widget_show (label);

   adjustment = gtk_adjustment_new (curl.do_curl_opacity * 100, 0.0, 100.0,
				    1.0, 1.0, 1.0);
   gtk_signal_connect (adjustment, "value_changed",
		       (GtkSignalFunc) dialog_scale_update,
		       &(curl.do_curl_opacity));

   scale = gtk_hscale_new (GTK_ADJUSTMENT (adjustment));
   gtk_widget_set_usize (GTK_WIDGET (scale), 150, 30);
   gtk_range_set_update_policy (GTK_RANGE (scale), GTK_UPDATE_DELAYED);
   gtk_scale_set_digits (GTK_SCALE (scale), 0);
   gtk_scale_set_draw_value (GTK_SCALE (scale), TRUE);
   gtk_box_pack_start (GTK_BOX (vbox), scale, TRUE, FALSE, 0);
   gtk_widget_show (scale);

/*****/

   gtk_widget_show (vbox);

   gtk_widget_show (dialog);

   gtk_main ();
   gdk_flush ();

   return curl_run;
}				/* do_dialog */

/*****/

static void init_calculation () {
   double k;
   double alpha, beta;
   double angle;
   vector_t v1, v2;
   gint32 *image_layers, nlayers;


   gimp_layer_add_alpha (drawable->id);

   /* Image parameters */

   /* Determine Position of original Layer in the Layer stack. */

   image_layers = gimp_image_get_layers (image_id, &nlayers);
   drawable_position = 0;
   while (drawable_position < nlayers &&
	  image_layers[drawable_position] != drawable->id)
      drawable_position++;
   if (drawable_position >= nlayers) {
      fprintf (stderr, "Fatal error: drawable not found in layer stack.\n");
      drawable_position = 0;
   }
   /* Get the bounds of the active selection */
   gimp_drawable_mask_bounds (drawable->id, &sel_x1, &sel_y1, &sel_x2, &sel_y2);

   true_sel_width = sel_x2 - sel_x1;
   true_sel_height = sel_y2 - sel_y1;
   if (curl.do_vertical) {
      sel_width = true_sel_width;
      sel_height = true_sel_height;
   } else {
      sel_width = true_sel_height;
      sel_height = true_sel_width;
   }				/* else */


   /* Circle parameters */

   alpha = atan ((double) sel_height / sel_width);
   beta = alpha / 2.0;
   k = sel_width / ((G_PI + alpha) * sin (beta) + cos (beta));
   v_set (&center, k * cos (beta), k * sin (beta));
   radius = center.y;

   /* left_tangent  */

   v_set (&left_tangent, radius * -sin (alpha), radius * cos (alpha));
   v_add (&left_tangent, left_tangent, center);

   /* right_tangent */

   v_sub (&v1, left_tangent, center);
   v_set (&v2, sel_width - center.x, sel_height - center.y);
   angle = -2.0 * acos (v_dot (v1, v2) / (v_mag (v1) * v_mag (v2)));
   v_set (&right_tangent,
	  v1.x * cos (angle) + v1.y * -sin (angle),
	  v1.x * sin (angle) + v1.y * cos (angle));
   v_add (&right_tangent, right_tangent, center);

   /* Slopes */

   diagl_slope = (double) sel_width / sel_height;
   diagr_slope = (sel_width - right_tangent.x) / (sel_height - right_tangent.y);
   diagb_slope = (right_tangent.y - left_tangent.y) / (right_tangent.x - left_tangent.x);
   diagm_slope = (sel_width - center.x) / sel_height;

   /* Colors */

   gimp_palette_get_foreground (&fore_color[0], &fore_color[1], &fore_color[2]);
   gimp_palette_get_background (&back_color[0], &back_color[1], &back_color[2]);

   
}				/* init_calculation */

/*****/

static void do_curl_effect (void) {
   gint x, y, color_image;
   gint x1, y1, k;
   guint alpha_pos, progress, max_progress;
   gdouble intensity, alpha, beta;
   vector_t v, dl, dr;
   gdouble dl_mag, dr_mag, angle, factor;
   guchar *pp, *dest, fore_grayval, back_grayval;
   guchar *gradsamp;
   GPixelRgn dest_rgn;
   gpointer pr;
   guchar *grad_samples = NULL;

   color_image = gimp_drawable_color (drawable->id);
   curl_layer =
      gimp_drawable_get (gimp_layer_new (image_id,
					 "Curl layer",
					 true_sel_width,
					 true_sel_height,
				         color_image ? RGBA_IMAGE : GRAYA_IMAGE,
					 100, NORMAL_MODE));
   gimp_image_add_layer (image_id, curl_layer->id, drawable_position);
   curl_layer_ID = curl_layer->id;

   gimp_drawable_offsets (drawable->id, &x1, &y1);
   gimp_layer_set_offsets (curl_layer->id, sel_x1 + x1, sel_y1 + y1);
   gimp_tile_cache_ntiles (2 * (curl_layer->width / gimp_tile_width () + 1));

   /* Clear the newly created layer */
   gimp_pixel_rgn_init (&dest_rgn, curl_layer, 0, 0, true_sel_width, true_sel_height, TRUE, FALSE);
   for (pr = gimp_pixel_rgns_register (1, &dest_rgn); pr != NULL; pr = gimp_pixel_rgns_process (pr)) {
      dest = dest_rgn.data;
      for (y1 = dest_rgn.y; y1 < dest_rgn.y + dest_rgn.h; y1++) {
	 pp = dest;
	 for (x1 = dest_rgn.x; x1 < dest_rgn.x + dest_rgn.w; x1++) {
	    for (k = 0; k < dest_rgn.bpp; k++)
	       pp[k] = 0;
	    pp += dest_rgn.bpp;
	 }
	 dest += dest_rgn.rowstride;
      }
   }

   gimp_drawable_flush (curl_layer);
   gimp_drawable_update (curl_layer->id, 0, 0, curl_layer->width, curl_layer->height);

   gimp_pixel_rgn_init (&dest_rgn, curl_layer, 0, 0, true_sel_width, true_sel_height, TRUE, TRUE);

   /* Init shade_under */
   v_set (&dl, -sel_width, -sel_height);
   dl_mag = v_mag (dl);
   v_set (&dr, -(sel_width - right_tangent.x), -(sel_height - right_tangent.y));
   dr_mag = v_mag (dr);
   alpha = acos (v_dot (dl, dr) / (dl_mag * dr_mag));
   beta=alpha/2;

   /* Init shade_curl */

   fore_grayval = INTENSITY (fore_color[0], fore_color[1], fore_color[2]);
   back_grayval = INTENSITY (back_color[0], back_color[1], back_color[2]);

   /* Gradient Samples */
   if (curl.do_curl_gradient)
      grad_samples = get_samples (curl_layer);

   max_progress = 2 * sel_width * sel_height;
   progress = 0;

   alpha_pos = dest_rgn.bpp - 1;

   /* Main loop */
   for (pr = gimp_pixel_rgns_register (1, &dest_rgn); pr != NULL; pr = gimp_pixel_rgns_process (pr)) {

      dest = dest_rgn.data;

      for (y1 = dest_rgn.y; y1 < dest_rgn.y + dest_rgn.h; y1++) {
	 pp = dest;
	 for (x1 = dest_rgn.x; x1 < dest_rgn.x + dest_rgn.w; x1++) {
	    /* Map coordinates to get the curl correct... */
	    if (curl.do_horizontal) {
	       x = (curl.do_lower_right || curl.do_lower_left) ? y1 : sel_width - 1 - y1;
	       y = (curl.do_upper_left || curl.do_lower_left) ? x1 : sel_height - 1 - x1;
	    } else {
	       x = (curl.do_upper_right || curl.do_lower_right) ? x1 : sel_width - 1 - x1;
	       y = (curl.do_upper_left || curl.do_upper_right) ? y1 : sel_height - 1 - y1;
	    }
	    if (left_of_diagl (x, y)) {		/* uncurled region */
	       for (k = 0; k <= alpha_pos; k++)
		  pp[k] = 0;
	    } else if (right_of_diagr (x, y) || (right_of_diagm (x, y)
			  && below_diagb (x, y) && !inside_circle (x, y))) {
	       /* curled region */
	       for (k = 0; k <= alpha_pos; k++)
		  pp[k] = 0;
	    } else {
	       v.x = -(sel_width - x);
	       v.y = -(sel_height - y);
	       angle = acos (v_dot (v, dl) / (v_mag (v) * dl_mag));

	       if (inside_circle (x, y) || below_diagb (x, y)) {
		  /* Below the curl. */
		  factor = angle / alpha;
		  for (k = 0; k < alpha_pos; k++)
		     pp[k] = 0;
		  pp[alpha_pos] = curl.do_shade_under ? (guchar) ((float) 255 * (float) factor) : 0;

	       } else {
		  /* On the curl */
		  if (curl.do_curl_gradient) {
		     /* Calculate position in Gradient (0 <= intensity <= 1) */
		     intensity = (angle/alpha) + sin(G_PI*2 * angle/alpha)*0.075;
		     /* Check boundaries */
		     intensity = (intensity < 0 ? 0 : (intensity > 1 ? 1 : intensity ));
		     gradsamp = &grad_samples[((guint) (intensity * NGRADSAMPLES)) * dest_rgn.bpp];
		     if (color_image) {
		        pp[0] = gradsamp[0];
		        pp[1] = gradsamp[1];
		        pp[2] = gradsamp[2];
		     } else 
		        pp[0] = gradsamp[0];
		     pp[alpha_pos] = (guchar) ((double) gradsamp[alpha_pos] * (1 - intensity*(1-curl.do_curl_opacity)));
		  } else {
		     intensity = pow (sin (G_PI * angle / alpha), 1.5);
		     if (color_image) {
		        pp[0] = (intensity * back_color[0] + (1 - intensity) * fore_color[0]);
		        pp[1] = (intensity * back_color[1] + (1 - intensity) * fore_color[1]);
		        pp[2] = (intensity * back_color[2] + (1 - intensity) * fore_color[2]);
		     } else
		        pp[0] = (intensity * back_grayval + (1 - intensity) * fore_grayval);
		     pp[alpha_pos] = (guchar) ((double) 255 * (1 - intensity*(1-curl.do_curl_opacity)));
		  }
	       }
	    }
	    pp += dest_rgn.bpp;
	 }			/* for y1 */
	 dest += dest_rgn.rowstride;
      }				/* for x1 */
      progress += dest_rgn.w * dest_rgn.h;
      gimp_progress_update ((double) progress / (double) max_progress);
   }				/* Pixel Regions loop end */

   gimp_drawable_flush (curl_layer);
   gimp_drawable_merge_shadow (curl_layer->id, FALSE);
   gimp_drawable_update (curl_layer->id, 0, 0, curl_layer->width, curl_layer->height);
   gimp_drawable_detach (curl_layer);

   if (grad_samples != NULL) g_free (grad_samples);
}

/************************************************/

static void clear_curled_region () {
   GPixelRgn src_rgn, dest_rgn;
   gpointer pr;
   gint x, y;
   guint x1, y1, i;
   guchar *dest, *src, *pp, *sp;
   guint alpha_pos, progress, max_progress;

   max_progress = 2 * sel_width * sel_height;
   progress = max_progress / 2;

   gimp_tile_cache_ntiles (2 * (drawable->width / gimp_tile_width () + 1));
   gimp_pixel_rgn_init (&src_rgn, drawable, sel_x1, sel_y1, true_sel_width, true_sel_height, FALSE, FALSE);
   gimp_pixel_rgn_init (&dest_rgn, drawable, sel_x1, sel_y1, true_sel_width, true_sel_height, TRUE, TRUE);
   alpha_pos = dest_rgn.bpp - 1;
   if (dest_rgn.bpp != src_rgn.bpp)
      fprintf (stderr, "WARNING: src_rgn.bpp != dest_rgn.bpp!\n");
   for (pr = gimp_pixel_rgns_register (2, &dest_rgn, &src_rgn); pr != NULL; pr = gimp_pixel_rgns_process (pr)) {
      dest = dest_rgn.data;
      src = src_rgn.data;
      for (y1 = dest_rgn.y; y1 < dest_rgn.y + dest_rgn.h; y1++) {
	 sp = src;
	 pp = dest;
	 for (x1 = dest_rgn.x; x1 < dest_rgn.x + dest_rgn.w; x1++) {
	    /* Map coordinates to get the curl correct... */
	    if (curl.do_horizontal) {
	       x = (curl.do_lower_right || curl.do_lower_left) ? y1 - sel_y1 : sel_width - 1 - (y1 - sel_y1);
	       y = (curl.do_upper_left || curl.do_lower_left) ? x1 - sel_x1 : sel_height - 1 - (x1 - sel_x1);
	    } else {
	       x = (curl.do_upper_right || curl.do_lower_right) ? x1 - sel_x1 : sel_width - 1 - (x1 - sel_x1);
	       y = (curl.do_upper_left || curl.do_upper_right) ? y1 - sel_y1 : sel_height - 1 - (y1 - sel_y1);
	    }
	    for (i = 0; i < alpha_pos; i++)
	       pp[i] = sp[i];
	    if (right_of_diagr (x, y) || (right_of_diagm (x, y) && below_diagb (x, y) && !inside_circle (x, y))) {
	       /* Right of the curl */
	       pp[alpha_pos] = 0;
	    } else {
	       pp[alpha_pos] = sp[alpha_pos];
	    }
	    pp += dest_rgn.bpp;
	    sp += src_rgn.bpp;
	 }
	 src += src_rgn.rowstride;
	 dest += dest_rgn.rowstride;
      }
      progress += dest_rgn.w * dest_rgn.h;
      gimp_progress_update ((double) progress / (double) max_progress);
   }
   gimp_drawable_flush (drawable);
   gimp_drawable_merge_shadow (drawable->id, TRUE);
   gimp_drawable_update (drawable->id, sel_x1, sel_y1, true_sel_width, true_sel_height);
   gimp_drawable_detach (drawable);
}

/*****/

static void page_curl () {
   int nreturn_vals;
   gimp_run_procedure ("gimp_undo_push_group_start", &nreturn_vals,
		       PARAM_IMAGE, image_id,
		       PARAM_END);

   gimp_progress_init ("Page Curl");
   init_calculation ();
   do_curl_effect ();
   clear_curled_region ();
   gimp_run_procedure ("gimp_undo_push_group_end", &nreturn_vals,
		       PARAM_IMAGE, image_id,
		       PARAM_END);
}				/* page_curl */


/*
  Returns NGRADSAMPLES samples of active gradient.
  Each sample has (gimp_drawable_bpp (drawable->id)) bytes.
  "ripped" from gradmap.c.
 */
static guchar *get_samples (GDrawable *drawable) {
  gdouble       *f_samples, *f_samp;    /* float samples */
  guchar        *b_samples, *b_samp;    /* byte samples */
  gint          bpp, color, has_alpha, alpha;
  gint          i, j;

  f_samples = gimp_gradients_sample_uniform (NGRADSAMPLES);

  bpp       = gimp_drawable_bpp (drawable->id);
  color     = gimp_drawable_color (drawable->id);
  has_alpha = gimp_drawable_has_alpha (drawable->id);
  alpha     = (has_alpha ? bpp - 1 : bpp);

  b_samples = g_new (guchar, NGRADSAMPLES * bpp);

  for (i = 0; i < NGRADSAMPLES; i++)
    {
      b_samp = &b_samples[i * bpp];
      f_samp = &f_samples[i * 4];
      if (color)
        for (j = 0; j < 3; j++)
          b_samp[j] = f_samp[j] * 255;
      else
        b_samp[0] = INTENSITY (f_samp[0], f_samp[1], f_samp[2]) * 255;

      if (has_alpha)
        b_samp[alpha] = f_samp[3] * 255;
    }

  g_free (f_samples);
  return b_samples;
}

