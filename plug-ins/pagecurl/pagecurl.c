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

#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

#include "libgimp/stdplugins-intl.h"

#include "curl0.xpm"
#include "curl1.xpm"
#include "curl2.xpm"
#include "curl3.xpm"
#include "curl4.xpm"
#include "curl5.xpm"
#include "curl6.xpm"
#include "curl7.xpm"

#define PLUG_IN_NAME    "plug_in_pagecurl"
#define PLUG_IN_VERSION "May 1998, 0.9"
#define HELP_ID         "plug-in-pagecurl"
#define NGRADSAMPLES    256


/***** Types *****/

typedef struct
{
  gint    do_curl_shade;
  gint    do_curl_gradient;
  gint    do_curl_warp;  /* Not yet supported... */

  gdouble do_curl_opacity;
  gint    do_shade_under;

  gint    do_upper_left;
  gint    do_upper_right;
  gint    do_lower_left;
  gint    do_lower_right;

  gint    do_vertical;
  gint    do_horizontal;
} CurlParams;

/***** Prototypes *****/

static void query                (void);
static void run                  (const gchar      *name,
				  gint              nparams,
				  const GimpParam  *param,
				  gint             *nreturn_vals,
				  GimpParam       **return_vals);
static void set_default_params   (void);

static void dialog_toggle_update (GtkWidget         *widget,
				  gint32             value);
static void dialog_scale_update  (GtkAdjustment     *adjustment,
				  gdouble           *value);

static gint do_dialog            (void);

static void     init_calculation    (gint32 drawable_id);
static void     do_curl_effect      (gint32 drawable_id);
static void     clear_curled_region (gint32 drawable_id);
static void     page_curl           (gint32 drawable_id);
static guchar * get_samples         (gint32 drawable_id);


/***** Variables *****/

GimpPlugInInfo PLUG_IN_INFO =
{
  NULL,  /* init_proc  */
  NULL,  /* quit_proc  */
  query, /* query_proc */
  run    /* run_proc   */
};

static CurlParams curl;

/* Image parameters */

static gint32        image_id;
static GimpDrawable *curl_layer;

static gchar **curl_pixmaps[] =
{
  curl0_xpm,
  curl1_xpm,
  curl2_xpm,
  curl3_xpm,
  curl4_xpm,
  curl5_xpm,
  curl6_xpm,
  curl7_xpm
};

static GtkWidget *curl_pixmap_widget;

static gint   sel_x1, sel_y1, sel_x2, sel_y2;
static gint   true_sel_width, true_sel_height;
static gint   sel_width, sel_height;
static gint   drawable_position;
static gint32 curl_layer_ID;

/* Center and radius of circle */

static GimpVector2 center;
static double      radius;

/* Useful points to keep around */

static GimpVector2 left_tangent;
static GimpVector2 right_tangent;

/* Slopes --- these are *not* in the usual geometric sense! */

static gdouble diagl_slope;
static gdouble diagr_slope;
static gdouble diagb_slope;
static gdouble diagm_slope;

/* User-configured parameters */

static guchar fore_color[3];
static guchar back_color[3];


/***** Functions *****/

MAIN ()

static void
query (void)
{
  static GimpParamDef args[] =
  {
    { GIMP_PDB_INT32,    "run_mode", "Interactive (0), non-interactive (1)" },
    { GIMP_PDB_IMAGE,    "image",    "Input image"                          },
    { GIMP_PDB_DRAWABLE, "drawable", "Input drawable"                       },
    { GIMP_PDB_INT32,    "mode",
        "Pagecurl-mode: Use FG- and BG-Color (0), Use current gradient (1)" },
    { GIMP_PDB_INT32,    "edge",
        "Edge to curl (1-4, clockwise, starting in the lower right edge)"   },
    { GIMP_PDB_INT32,    "type",     "vertical (0), horizontal (1)"         },
    { GIMP_PDB_INT32,    "shade",
        "Shade the region under the curl (1) or not (0)"                    },
  };

  static GimpParamDef return_vals[] =
  {
    { GIMP_PDB_LAYER, "Curl Layer", "The new layer with the curl." }
  };

  gimp_install_procedure (PLUG_IN_NAME,
			  "Pagecurl effect",
			  "This plug-in creates a pagecurl-effect.",
			  "Federico Mena Quintero and Simon Budig",
			  "Federico Mena Quintero and Simon Budig",
			  PLUG_IN_VERSION,
			  N_("_Pagecurl..."),
			  "RGB*, GRAY*",
			  GIMP_PLUGIN,
			  G_N_ELEMENTS (args),
			  G_N_ELEMENTS (return_vals),
			  args,
			  return_vals);

  gimp_plugin_menu_register (PLUG_IN_NAME,
                             N_("<Image>/Filters/Distorts"));
}

static void
run (const gchar      *name,
     gint              nparams,
     const GimpParam  *param,
     gint             *nreturn_vals,
     GimpParam       **return_vals)
{
  static GimpParam  values[2];
  GimpRunMode       run_mode;
  GimpPDBStatusType status = GIMP_PDB_SUCCESS;
  gint32            drawable_id;

  run_mode = param[0].data.d_int32;

  INIT_I18N ();

  set_default_params ();

  /*  Possibly retrieve data  */
  gimp_get_data (PLUG_IN_NAME, &curl);

  *nreturn_vals = 2;
  *return_vals = values;

  values[0].type = GIMP_PDB_STATUS;
  values[0].data.d_status = status;
  values[1].type = GIMP_PDB_LAYER;
  values[1].data.d_layer = -1;

  /*  Get the specified drawable  */
  drawable_id = param[2].data.d_drawable;
  image_id = param[1].data.d_image;

  if (gimp_drawable_is_rgb (drawable_id)
       || gimp_drawable_is_gray (drawable_id))
    {
      switch (run_mode)
	{
	case GIMP_RUN_INTERACTIVE:
	  /*  First acquire information with a dialog  */
	  if (!do_dialog ())
	    return;
	  break;

	case GIMP_RUN_NONINTERACTIVE:
	  /*  Make sure all the arguments are there!  */
	  if (nparams != 7)
	    status = GIMP_PDB_CALLING_ERROR;
	  if (status == GIMP_PDB_SUCCESS)
	    {
	      curl.do_curl_shade = (param[3].data.d_int32 == 0) ? 1 : 0;
	      curl.do_curl_gradient = 1 - curl.do_curl_shade;
	      switch (param[4].data.d_int32)
		{
		case 1:
		  curl.do_upper_left  = 0;
		  curl.do_upper_right = 0;
		  curl.do_lower_left  = 0;
		  curl.do_lower_right = 1;
		  break;
		case 2:
		  curl.do_upper_left  = 0;
		  curl.do_upper_right = 0;
		  curl.do_lower_left  = 1;
		  curl.do_lower_right = 0;
		  break;
		case 3:
		  curl.do_upper_left  = 1;
		  curl.do_upper_right = 0;
		  curl.do_lower_left  = 0;
		  curl.do_lower_right = 0;
		  break;
		case 4:
		  curl.do_upper_left  = 0;
		  curl.do_upper_right = 1;
		  curl.do_lower_left  = 0;
		  curl.do_lower_right = 0;
		  break;
		default:
		  break;
		}
	      curl.do_vertical    = (param[5].data.d_int32) ? 0 : 1;
	      curl.do_horizontal  = 1 - curl.do_vertical;
	      curl.do_shade_under = (param[6].data.d_int32) ? 1 : 0;
	    }
	  break;

	case GIMP_RUN_WITH_LAST_VALS:
	  break;

	default:
	  break;
	}

      if (status == GIMP_PDB_SUCCESS)
	{
	  page_curl (drawable_id);
	  values[1].data.d_layer = curl_layer_ID;
	  if (run_mode != GIMP_RUN_NONINTERACTIVE)
            gimp_displays_flush ();
	  if (run_mode == GIMP_RUN_INTERACTIVE)
            gimp_set_data (PLUG_IN_NAME, &curl, sizeof (CurlParams));
	}
    }
  else
    /* Sorry - no indexed/noalpha images */
    status = GIMP_PDB_EXECUTION_ERROR;

  values[0].data.d_status = status;
}

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

static inline gboolean
left_of_diagl (gdouble x,
	       gdouble y)
{
  return (x < (sel_width + (y - sel_height) * diagl_slope));
}

static inline gboolean
right_of_diagr (gdouble x,
		gdouble y)
{
  return (x > (sel_width + (y - sel_height) * diagr_slope));
}

static inline gboolean
below_diagb (gdouble x,
	     gdouble y)
{
  return (y < (right_tangent.y + (x - right_tangent.x) * diagb_slope));
}

static inline gboolean
right_of_diagm (gdouble x,
		gdouble y)
{
  return (x > (sel_width + (y - sel_height) * diagm_slope));
}

static inline gboolean
inside_circle (gdouble x,
	       gdouble y)
{
  x -= center.x;
  y -= center.y;

  return x * x + y * y <= radius * radius;
}

static void
set_default_params (void)
{
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
}

/********************************************************************/
/* dialog callbacks                                                 */
/********************************************************************/

static void
dialog_scale_update (GtkAdjustment *adjustment,
		     gdouble       *value)
{
  *value = ((gdouble) adjustment->value) / 100.0;
}

static void
dialog_toggle_update (GtkWidget *widget,
		      gint32     value)
{
  gint pixmapindex = 0;

  switch (value)
    {
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

  pixmapindex = (curl.do_lower_left + curl.do_upper_right * 2 +
		 curl.do_upper_left * 3 + curl.do_horizontal * 4);

  gimp_pixmap_set (GIMP_PIXMAP (curl_pixmap_widget),
		   curl_pixmaps[pixmapindex]);
}

static gint
do_dialog (void)
{
  /* Missing options: Color-dialogs? / own curl layer ? / transparency
     to original drawable / Warp-curl (unsupported yet) */

  GtkWidget *dialog;
  GtkWidget *hbox;
  GtkWidget *vbox;
  GtkWidget *vbox2;
  GtkWidget *table;
  GtkWidget *frame;
  GtkWidget *shade_button;
  GtkWidget *gradient_button;
  GtkWidget *button;
  GtkWidget *scale;
  GtkObject *adjustment;
  gint       pixmapindex;
  gboolean   run;

  gimp_ui_init ("pagecurl", FALSE);

  dialog = gimp_dialog_new (_("Pagecurl Effect"), "pagecurl",
                            NULL, 0,
			    gimp_standard_help_func, HELP_ID,

			    GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
			    GTK_STOCK_OK,     GTK_RESPONSE_OK,

			    NULL);

   vbox = gtk_vbox_new (FALSE, 4);
   gtk_container_set_border_width (GTK_CONTAINER (vbox), 6);
   gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog)->vbox), vbox, TRUE, TRUE, 0);
   gtk_widget_show (vbox);

   frame = gtk_frame_new (_("Curl Location"));
   gtk_box_pack_start (GTK_BOX (vbox), frame, TRUE, TRUE, 0);

   table = gtk_table_new (3, 3, FALSE);
   gtk_table_set_col_spacings (GTK_TABLE (table), 2);
   gtk_table_set_row_spacings (GTK_TABLE (table), 2);
   gtk_container_set_border_width (GTK_CONTAINER (table), 2);
   gtk_container_add (GTK_CONTAINER (frame), table);

   pixmapindex = (curl.do_lower_left + curl.do_upper_right * 2 +
                  curl.do_upper_left * 3 + curl.do_horizontal * 4);

   if (pixmapindex < 0 || pixmapindex > 7)
     pixmapindex = 0;

   curl_pixmap_widget = gimp_pixmap_new (curl_pixmaps[pixmapindex]);

   gtk_table_attach (GTK_TABLE (table), curl_pixmap_widget, 1, 2, 1, 2,
		     GTK_SHRINK, GTK_SHRINK, 0, 0);
   gtk_widget_show (curl_pixmap_widget);

   {
     gint i;
     gchar *name[] =
     {
       N_("Upper Left"),
       N_("Upper Right"),
       N_("Lower Left"),
       N_("Lower Right")
     };

     button = NULL;
     for (i = 0; i < 4; i++)
       {
	 button = gtk_radio_button_new_with_label
	   ((button == NULL) ?
	    NULL : gtk_radio_button_get_group (GTK_RADIO_BUTTON (button)),
	    gettext(name[i]));
	 gtk_toggle_button_set_active
	   (GTK_TOGGLE_BUTTON (button),
	    (i == 0 ? curl.do_upper_left : i == 1 ? curl.do_upper_right :
	     i == 2 ? curl.do_lower_left : curl.do_lower_right));

	 gtk_table_attach (GTK_TABLE (table), button,
			   (i % 2) ? 2 : 0, (i % 2) ? 3 : 1,
			   (i < 2) ? 0 : 2, (i < 2) ? 1 : 3,
			   GTK_SHRINK, GTK_SHRINK, 0, 0);
	 gtk_widget_show (button);

	 g_signal_connect (button, "toggled",
                           G_CALLBACK (dialog_toggle_update),
                           GINT_TO_POINTER (i));
       }
   }

   gtk_widget_show (table);
   gtk_widget_show (frame);

   frame = gtk_frame_new (_("Curl Orientation"));
   gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, FALSE, 0);

   hbox = gtk_hbox_new (FALSE, 4);
   gtk_container_set_border_width (GTK_CONTAINER (hbox), 2);
   gtk_container_add (GTK_CONTAINER (frame), hbox);

   {
     gint i;
     gchar *name[] =
     {
       N_("Horizontal"),
       N_("Vertical")
     };

     button = NULL;
     for (i = 0; i < 2; i++)
       {
	 button = gtk_radio_button_new_with_label
	   ((button == NULL) ?
	    NULL : gtk_radio_button_get_group (GTK_RADIO_BUTTON (button)),
	    gettext(name[i]));
	 gtk_toggle_button_set_active
	   (GTK_TOGGLE_BUTTON (button),
	    (i == 0 ? curl.do_horizontal : curl.do_vertical));

	 gtk_box_pack_start (GTK_BOX (hbox), button, TRUE, TRUE, 0);
	 gtk_widget_show (button);

	 g_signal_connect (button, "toggled",
                           G_CALLBACK (dialog_toggle_update),
                           GINT_TO_POINTER (i + 5));
       }
   }

   gtk_widget_show (hbox);
   gtk_widget_show (frame);

   shade_button = gtk_check_button_new_with_label (_("Shade under Curl"));
   gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (shade_button),
				 curl.do_shade_under ? TRUE : FALSE);
   gtk_box_pack_start (GTK_BOX (vbox), shade_button, FALSE, FALSE, 0);
   gtk_widget_show (shade_button);

   g_signal_connect (shade_button, "toggled",
                     G_CALLBACK (dialog_toggle_update),
                     GINT_TO_POINTER (8));

   gradient_button =
     gtk_check_button_new_with_label (_("Use Current Gradient\n"
					"instead of FG/BG-Color"));
   gtk_label_set_justify (GTK_LABEL (GTK_BIN (gradient_button)->child),
			  GTK_JUSTIFY_LEFT);
   gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (gradient_button),
				 curl.do_curl_gradient ? TRUE : FALSE);
   gtk_box_pack_start (GTK_BOX (vbox), gradient_button, FALSE, FALSE, 0);
   gtk_widget_show (gradient_button);

   g_signal_connect (gradient_button, "toggled",
                     G_CALLBACK (dialog_toggle_update),
                     GINT_TO_POINTER (9));

   frame = gtk_frame_new (_("Curl Opacity"));
   gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, FALSE, 0);
   gtk_widget_show (frame);

   vbox2 = gtk_vbox_new (FALSE, 0);
   gtk_container_set_border_width (GTK_CONTAINER (vbox2), 2);
   gtk_container_add (GTK_CONTAINER (frame), vbox2);
   gtk_widget_show (vbox2);

   adjustment = gtk_adjustment_new (curl.do_curl_opacity * 100, 0.0, 100.0,
				    1.0, 1.0, 0.0);
   g_signal_connect (adjustment, "value_changed",
                     G_CALLBACK (dialog_scale_update),
                     &(curl.do_curl_opacity));

   scale = gtk_hscale_new (GTK_ADJUSTMENT (adjustment));
   gtk_widget_set_size_request (GTK_WIDGET (scale), 150, 30);
   gtk_range_set_update_policy (GTK_RANGE (scale), GTK_UPDATE_DELAYED);
   gtk_scale_set_digits (GTK_SCALE (scale), 0);
   gtk_scale_set_draw_value (GTK_SCALE (scale), TRUE);
   gtk_box_pack_start (GTK_BOX (vbox2), scale, TRUE, FALSE, 0);
   gtk_widget_show (scale);

   gtk_widget_show (dialog);

   run = (gimp_dialog_run (GIMP_DIALOG (dialog)) == GTK_RESPONSE_OK);

   gtk_widget_destroy (dialog);

   return run;
}

static void
init_calculation (gint32 drawable_id)
{
  gdouble      k;
  gdouble      alpha, beta;
  gdouble      angle;
  GimpVector2  v1, v2;
  gint32      *image_layers;
  gint32       nlayers;
  GimpRGB      color;

  gimp_layer_add_alpha (drawable_id);

  /* Image parameters */

  /* Determine Position of original Layer in the Layer stack. */

  image_layers = gimp_image_get_layers (image_id, &nlayers);
  drawable_position = 0;
  while (drawable_position < nlayers &&
	 image_layers[drawable_position] != drawable_id)
    drawable_position++;

  /* Get the bounds of the active selection */
  gimp_drawable_mask_bounds (drawable_id,
			     &sel_x1, &sel_y1, &sel_x2, &sel_y2);

  true_sel_width = sel_x2 - sel_x1;
  true_sel_height = sel_y2 - sel_y1;
  if (curl.do_vertical)
    {
      sel_width = true_sel_width;
      sel_height = true_sel_height;
    }
  else
    {
      sel_width = true_sel_height;
      sel_height = true_sel_width;
    }

  /* Circle parameters */

  alpha = atan ((double) sel_height / sel_width);
  beta = alpha / 2.0;
  k = sel_width / ((G_PI + alpha) * sin (beta) + cos (beta));
  gimp_vector2_set (&center, k * cos (beta), k * sin (beta));
  radius = center.y;

  /* left_tangent  */

  gimp_vector2_set (&left_tangent, radius * -sin (alpha), radius * cos (alpha));
  gimp_vector2_add (&left_tangent, &left_tangent, &center);

  /* right_tangent */

  gimp_vector2_sub (&v1, &left_tangent, &center);
  gimp_vector2_set (&v2, sel_width - center.x, sel_height - center.y);
  angle = -2.0 * acos (gimp_vector2_inner_product (&v1, &v2) /
		       (gimp_vector2_length (&v1) * gimp_vector2_length (&v2)));
  gimp_vector2_set (&right_tangent,
		    v1.x * cos (angle) + v1.y * -sin (angle),
		    v1.x * sin (angle) + v1.y * cos (angle));
  gimp_vector2_add (&right_tangent, &right_tangent, &center);

  /* Slopes */

  diagl_slope = (double) sel_width / sel_height;
  diagr_slope = (sel_width - right_tangent.x) / (sel_height - right_tangent.y);
  diagb_slope = (right_tangent.y - left_tangent.y) / (right_tangent.x - left_tangent.x);
  diagm_slope = (sel_width - center.x) / sel_height;

  /* Colors */

  gimp_palette_get_foreground (&color);
  gimp_rgb_get_uchar (&color, &fore_color[0], &fore_color[1], &fore_color[2]);

  gimp_palette_get_background (&color);
  gimp_rgb_get_uchar (&color, &back_color[0], &back_color[1], &back_color[2]);
}

static void
do_curl_effect (gint32 drawable_id)
{
  gint          x, y, color_image;
  gint          x1, y1, k;
  guint         alpha_pos, progress, max_progress;
  gdouble       intensity, alpha, beta;
  GimpVector2   v, dl, dr;
  gdouble       dl_mag, dr_mag, angle, factor;
  guchar       *pp, *dest, fore_grayval, back_grayval;
  guchar       *gradsamp;
  GimpPixelRgn  dest_rgn;
  gpointer      pr;
  guchar       *grad_samples = NULL;

  color_image = gimp_drawable_is_rgb (drawable_id);
  curl_layer =
    gimp_drawable_get (gimp_layer_new (image_id,
				       _("Curl Layer"),
				       true_sel_width,
				       true_sel_height,
				       color_image ? GIMP_RGBA_IMAGE : GIMP_GRAYA_IMAGE,
				       100, GIMP_NORMAL_MODE));
  gimp_image_add_layer (image_id, curl_layer->drawable_id, drawable_position);
  gimp_drawable_fill (curl_layer->drawable_id, GIMP_TRANSPARENT_FILL);
  curl_layer_ID = curl_layer->drawable_id;

  gimp_drawable_offsets (drawable_id, &x1, &y1);
  gimp_layer_set_offsets (curl_layer->drawable_id, sel_x1 + x1, sel_y1 + y1);
  gimp_tile_cache_ntiles (2 * (curl_layer->width / gimp_tile_width () + 1));

  gimp_pixel_rgn_init (&dest_rgn, curl_layer,
		       0, 0, true_sel_width, true_sel_height, TRUE, TRUE);

  /* Init shade_under */
  gimp_vector2_set (&dl, -sel_width, -sel_height);
  dl_mag = gimp_vector2_length (&dl);
  gimp_vector2_set (&dr,
		    -(sel_width - right_tangent.x),
		    -(sel_height - right_tangent.y));
  dr_mag = gimp_vector2_length (&dr);
  alpha = acos (gimp_vector2_inner_product (&dl, &dr) / (dl_mag * dr_mag));
  beta = alpha / 2;

  /* Init shade_curl */

  fore_grayval = GIMP_RGB_INTENSITY (fore_color[0],
                                     fore_color[1],
                                     fore_color[2]) + 0.5;
  back_grayval = GIMP_RGB_INTENSITY (back_color[0],
                                     back_color[1],
                                     back_color[2]) + 0.5;

  /* Gradient Samples */
  if (curl.do_curl_gradient)
    grad_samples = get_samples (curl_layer->drawable_id);

  max_progress = 2 * sel_width * sel_height;
  progress = 0;

  alpha_pos = dest_rgn.bpp - 1;

  /* Main loop */
  for (pr = gimp_pixel_rgns_register (1, &dest_rgn);
       pr != NULL;
       pr = gimp_pixel_rgns_process (pr))
    {
      dest = dest_rgn.data;

      for (y1 = dest_rgn.y; y1 < dest_rgn.y + dest_rgn.h; y1++)
	{
	  pp = dest;
	  for (x1 = dest_rgn.x; x1 < dest_rgn.x + dest_rgn.w; x1++)
	    {
	      /* Map coordinates to get the curl correct... */
	      if (curl.do_horizontal)
		{
		  x = ((curl.do_lower_right || curl.do_lower_left) ?
                       y1 : sel_width - 1 - y1);
		  y = ((curl.do_upper_left  || curl.do_lower_left) ?
                       x1 : sel_height - 1 - x1);
		}
	      else
		{
		  x = ((curl.do_upper_right || curl.do_lower_right) ?
                       x1 : sel_width - 1 - x1);
		  y = ((curl.do_upper_left  || curl.do_upper_right) ?
                       y1 : sel_height - 1 - y1);
		}
	      if (left_of_diagl (x, y))
		{ /* uncurled region */
		  for (k = 0; k <= alpha_pos; k++)
		    pp[k] = 0;
		}
	      else if (right_of_diagr (x, y) ||
		       (right_of_diagm (x, y) &&
			below_diagb (x, y) &&
			!inside_circle (x, y)))
		{
		  /* curled region */
		  for (k = 0; k <= alpha_pos; k++)
		    pp[k] = 0;
		}
	      else
		{
		  v.x = -(sel_width - x);
		  v.y = -(sel_height - y);
		  angle = acos (gimp_vector2_inner_product (&v, &dl) /
				(gimp_vector2_length (&v) * dl_mag));

		  if (inside_circle (x, y) || below_diagb (x, y))
		    {
		      /* Below the curl. */
		      factor = angle / alpha;
		      for (k = 0; k < alpha_pos; k++)
			pp[k] = 0;
		      pp[alpha_pos] = (curl.do_shade_under ?
                                       (guchar) ((float) 255 * (float) factor) :
                                       0);
		    }
		  else
		    {
		      /* On the curl */
		      if (curl.do_curl_gradient)
			{
			  /* Calculate position in Gradient */
                          intensity =
                            (angle/alpha) + sin (G_PI*2 * angle/alpha) * 0.075;
			  /* Check boundaries */
			  intensity = (intensity < 0 ?
                                       0 : (intensity > 1 ? 1 : intensity ));
			  gradsamp = &grad_samples[((guint) (intensity * NGRADSAMPLES)) * dest_rgn.bpp];
			  if (color_image)
			    {
			      pp[0] = gradsamp[0];
			      pp[1] = gradsamp[1];
			      pp[2] = gradsamp[2];
			    }
			  else
			    pp[0] = gradsamp[0];

			  pp[alpha_pos] = (guchar) ((double) gradsamp[alpha_pos] * (1.0 - intensity * (1.0 - curl.do_curl_opacity)));
			}
		      else
			{
			  intensity = pow (sin (G_PI * angle / alpha), 1.5);
			  if (color_image)
			    {
			      pp[0] = (intensity * back_color[0] + (1.0 - intensity) * fore_color[0]);
			      pp[1] = (intensity * back_color[1] + (1.0 - intensity) * fore_color[1]);
			      pp[2] = (intensity * back_color[2] + (1.0 - intensity) * fore_color[2]);
			    }
			  else
			    pp[0] = (intensity * back_grayval + (1 - intensity) * fore_grayval);

			  pp[alpha_pos] = (guchar) ((double) 255.99 * (1.0 - intensity * (1.0 - curl.do_curl_opacity)));
			}
		    }
		}
	      pp += dest_rgn.bpp;
	    }
	  dest += dest_rgn.rowstride;
	}
      progress += dest_rgn.w * dest_rgn.h;
      gimp_progress_update ((double) progress / (double) max_progress);
    }

  gimp_drawable_flush (curl_layer);
  gimp_drawable_merge_shadow (curl_layer->drawable_id, FALSE);
  gimp_drawable_update (curl_layer->drawable_id,
			0, 0, curl_layer->width, curl_layer->height);
  gimp_drawable_detach (curl_layer);

  g_free (grad_samples);
}

/************************************************/

static void
clear_curled_region (gint32 drawable_id)
{
  GimpPixelRgn  src_rgn, dest_rgn;
  gpointer      pr;
  gint          x, y;
  guint         x1, y1, i;
  guchar       *dest, *src, *pp, *sp;
  guint         alpha_pos, progress, max_progress;
  GimpDrawable *drawable;

  max_progress = 2 * sel_width * sel_height;
  progress = max_progress / 2;

  drawable = gimp_drawable_get (drawable_id);

  gimp_tile_cache_ntiles (2 * (drawable->width / gimp_tile_width () + 1));
  gimp_pixel_rgn_init (&src_rgn, drawable,
		       sel_x1, sel_y1, true_sel_width, true_sel_height,
		       FALSE, FALSE);
  gimp_pixel_rgn_init (&dest_rgn, drawable,
		       sel_x1, sel_y1, true_sel_width, true_sel_height,
		       TRUE, TRUE);
  alpha_pos = dest_rgn.bpp - 1;

  for (pr = gimp_pixel_rgns_register (2, &dest_rgn, &src_rgn);
       pr != NULL;
       pr = gimp_pixel_rgns_process (pr))
    {
      dest = dest_rgn.data;
      src = src_rgn.data;
      for (y1 = dest_rgn.y; y1 < dest_rgn.y + dest_rgn.h; y1++)
	{
	  sp = src;
	  pp = dest;
	  for (x1 = dest_rgn.x; x1 < dest_rgn.x + dest_rgn.w; x1++)
	    {
	      /* Map coordinates to get the curl correct... */
	      if (curl.do_horizontal)
		{
		  x = (curl.do_lower_right || curl.do_lower_left) ? y1 - sel_y1 : sel_width - 1 - (y1 - sel_y1);
		  y = (curl.do_upper_left || curl.do_lower_left) ? x1 - sel_x1 : sel_height - 1 - (x1 - sel_x1);
		}
	      else
		{
		  x = ((curl.do_upper_right || curl.do_lower_right) ?
                       x1 - sel_x1 : sel_width - 1 - (x1 - sel_x1));
		  y = ((curl.do_upper_left || curl.do_upper_right) ?
                       y1 - sel_y1 : sel_height - 1 - (y1 - sel_y1));
		}
	      for (i = 0; i < alpha_pos; i++)
		pp[i] = sp[i];
	      if (right_of_diagr (x, y) ||
		  (right_of_diagm (x, y) &&
		   below_diagb (x, y) &&
		   !inside_circle (x, y)))
		{
		  /* Right of the curl */
		  pp[alpha_pos] = 0;
		}
	      else
		{
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
  gimp_drawable_merge_shadow (drawable_id, TRUE);
  gimp_drawable_update (drawable_id,
			sel_x1, sel_y1, true_sel_width, true_sel_height);
  gimp_drawable_detach (drawable);
}

static void
page_curl (gint32 drawable_id)
{
  gimp_image_undo_group_start (image_id);

  gimp_progress_init (_("Page Curl..."));

  init_calculation (drawable_id);

  do_curl_effect (drawable_id);
  clear_curled_region (drawable_id);

  gimp_image_undo_group_end (image_id);
}

/*
  Returns NGRADSAMPLES samples of active gradient.
  Each sample has (gimp_drawable_bpp (drawable_id)) bytes.
  "ripped" from gradmap.c.
 */
static guchar *
get_samples (gint32 drawable_id)
 {
  gdouble       *f_samples, *f_samp;    /* float samples */
  guchar        *b_samples, *b_samp;    /* byte samples */
  gint          bpp, color, has_alpha, alpha;
  gint          i, j;

#ifdef __GNUC__
#warning FIXME: "reverse" hardcoded to FALSE.
#endif
  f_samples = gimp_gradients_sample_uniform (NGRADSAMPLES, FALSE);

  bpp       = gimp_drawable_bpp (drawable_id);
  color     = gimp_drawable_is_rgb (drawable_id);
  has_alpha = gimp_drawable_has_alpha (drawable_id);
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
        b_samp[0] = GIMP_RGB_INTENSITY (f_samp[0], f_samp[1], f_samp[2]) * 255;

      if (has_alpha)
        b_samp[alpha] = f_samp[3] * 255;
    }

  g_free (f_samples);

  return b_samples;
}
