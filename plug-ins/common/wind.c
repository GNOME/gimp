/*
 * wind - a plug-in for the GIMP
 *
 * Copyright (C) Nigel Wetten
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
 * Contact info: nigel@cs.nwu.edu
 *
 * Version: 1.0.0
 */

#include <stdio.h>
#include <stdlib.h>

#include <gtk/gtk.h>

#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>


#define PLUG_IN_NAME "wind"

#define COMPARE_WIDTH   3

#define SCALE_WIDTH   200
#define MIN_THRESHOLD   0
#define MAX_THRESHOLD  50
#define MIN_STRENGTH    1
#define MAX_STRENGTH   50

#define NEGATIVE_STRENGTH_TEXT "\n   Wind Strength must be greater than 0.   \n"
#define THRESHOLD_TEXT "Higher values restrict the effect to fewer areas of the image"
#define STRENGTH_TEXT "Higher values increase the magnitude of the effect"
#define WIND_TEXT "A fine grained algorithm"
#define BLAST_TEXT "A coarse grained algorithm"
#define LEFT_TEXT "Makes the wind come from the left"
#define RIGHT_TEXT "Makes the wind come from the right"
#define LEADING_TEXT "The effect is applied at the leading edge of objects"
#define TRAILING_TEXT "The effect is applied at the trailing edge of objects"
#define BOTH_TEXT "The effect is applied at both edges of objects"

typedef enum
{
  LEFT,
  RIGHT
} direction_t;

typedef enum
{
  RENDER_WIND,
  RENDER_BLAST
} algorithm_t;

typedef enum
{
  BOTH,
  LEADING,
  TRAILING
} edge_t;


static void query (void);
static void run   (gchar   *name,
		   gint     nparams,
		   GParam  *param,
		   gint    *nreturn_vals,
		   GParam **return_vals);

static void dialog_box (void);
static gint render_effect (GDrawable *drawable);
static void render_wind      (GDrawable *drawable, gint threshold, gint strength,
			      direction_t direction, edge_t edge);
static void render_blast     (GDrawable *drawable, gint threshold, gint strength,
			      direction_t direction, edge_t edge);
static gint render_blast_row (guchar *buffer, gint bytes, gint lpi,
			      gint threshold,
			      gint strength, edge_t edge);
static void render_wind_row  (guchar *sb, gint bytes, gint lpi, gint threshold,
			      gint strength, edge_t edge);

static void msg_ok_callback                 (GtkWidget *widget, gpointer data);
static void ok_callback                     (GtkWidget *widget, gpointer data);

static void get_derivative     (guchar *pixel_R1, guchar *pixel_R2,
				edge_t edge, gint *derivative_R,
				gint *derivative_G, gint *derivative_B);
static gint threshold_exceeded (guchar *pixel_R1, guchar *pixel_R2,
				edge_t edge, gint threshold);
static void reverse_buffer     (guchar *buffer, gint length, gint bytes);

static void modal_message_box (gchar *text);

GPlugInInfo PLUG_IN_INFO =
{
  NULL,	  /* init_proc */
  NULL,	  /* quit_proc */
  query,  /* query_proc */
  run	  /* run_proc */
};


/*********************
  Globals
  *******************/

/* This is needed to communicate the result from the dialog
   box button's callback function*/
gint dialog_result = -1;

struct config_tag
{
  gint        threshold;     /* derivative comparison for edge detection */
  direction_t direction;     /* of wind, LEFT or RIGHT */
  gint        strength;	     /* how many pixels to bleed */
  algorithm_t alg;           /* which algorithm */
  edge_t      edge;          /* controls abs, negation of derivative */
};

typedef struct config_tag config_t;
config_t config =
{
  10,          /* threshold for derivative edge detection */
  LEFT,        /* bleed to the right */
  10,          /* how many pixels to bleed */
  RENDER_WIND, /* default algorithm */
  LEADING      /* abs(derivative); */
};



MAIN()

static void
query (void)
{
  static GParamDef args[] =
  {
    {PARAM_INT32, "run_mode", "Interactive, non-interactive"},
    {PARAM_IMAGE, "image", "Input image (unused)"},
    {PARAM_DRAWABLE, "drawable", "Input drawable"},
    {PARAM_INT32, "threshold", "Controls where blending will be done >= 0"},
    {PARAM_INT32, "direction", "Left or Right: 0 or 1"},
    {PARAM_INT32, "strength", "Controls the extent of the blending > 1"},
    {PARAM_INT32, "alg", "WIND, BLAST"},
    {PARAM_INT32, "edge", "LEADING, TRAILING, or BOTH"}
  };
  static GParamDef *return_vals = NULL;
  static int nargs = sizeof(args) / sizeof(args[0]);
  static int nreturn_vals = 0;

  gimp_install_procedure ("plug_in_wind",
			  "Renders a wind effect.",
			  "Renders a wind effect.",
			  "Nigel Wetten",
			  "Nigel Wetten",
			  "1998",
			  "<Image>/Filters/Distorts/Wind...",
			  "RGB*",
			  PROC_PLUG_IN,
			  nargs, nreturn_vals,
			  args, return_vals);
}

static void
run (gchar   *name,
     gint     nparams,
     GParam  *param,
     gint    *nreturn_vals,
     GParam **return_vals)
{
  static GParam values[1];
  GDrawable *drawable;
  GRunModeType run_mode;
  GStatusType status = STATUS_SUCCESS;

  run_mode = param[0].data.d_int32;
  drawable = gimp_drawable_get(param[2].data.d_drawable);
  gimp_tile_cache_ntiles(2 * (drawable->width / gimp_tile_width() + 1));
  switch (run_mode)
    {
    case RUN_NONINTERACTIVE:
      if (nparams == 8)
	{
	  config.threshold = param[3].data.d_int32;
	  config.direction = param[4].data.d_int32;
	  config.strength = param[5].data.d_int32;
	  config.alg = param[6].data.d_int32;
	  config.edge = param[7].data.d_int32;
	  if (render_effect(drawable) == -1)
	    {
	      status = STATUS_EXECUTION_ERROR;
	    }
	}
      else
	{
	  status = STATUS_CALLING_ERROR;
	}
      break;
      
    case RUN_INTERACTIVE:
      gimp_get_data("plug_in_wind", &config);
      dialog_box();
      if (dialog_result == -1)
	{
	  status = STATUS_EXECUTION_ERROR;
	  break;
	}
      if (render_effect(drawable) == -1)
	{
	  status = STATUS_CALLING_ERROR;
	  break;
	}
      gimp_set_data("plug_in_wind", &config, sizeof(config_t));
      gimp_displays_flush();
      break;
      
    case RUN_WITH_LAST_VALS:
      gimp_get_data("plug_in_wind", &config);
      if (render_effect(drawable) == -1)
	{
	  status = STATUS_EXECUTION_ERROR;
	  gimp_message("An execution error occured.");
	}
      else
	{
	  gimp_displays_flush();
	}
    }
      
  gimp_drawable_detach(drawable);
  
  *nreturn_vals = 1;
  *return_vals = values;
  values[0].type = PARAM_STATUS;
  values[0].data.d_status = status;

  return;
}

static gint
render_effect (GDrawable *drawable)
{
  if (config.alg == RENDER_WIND)
    {
      gimp_progress_init("Rendering Wind...");
      render_wind(drawable, config.threshold, config. strength,
		  config.direction, config.edge);
    }
  else if (config.alg == RENDER_BLAST)
    {
      gimp_progress_init("Rendering Blast...");
      render_blast(drawable, config.threshold, config.strength,
		   config.direction, config.edge);
    }
  return 0;
}

static void
render_blast (GDrawable   *drawable,
	      gint         threshold,
	      gint         strength,
	      direction_t  direction,
	      edge_t       edge)
{
  gint x1, x2, y1, y2;
  gint width = drawable->width;
  gint height = drawable->height;
  gint bytes = drawable->bpp;
  guchar *buffer;
  GPixelRgn src_region, dest_region;
  gint row;
  gint row_stride = width * bytes;
  gint marker = 0;
  gint lpi = row_stride - bytes;
  
  gimp_drawable_mask_bounds(drawable->id, &x1, &y1, &x2, &y2);
  gimp_pixel_rgn_init(&src_region, drawable, 0, 0, width, height, FALSE, FALSE);
  gimp_pixel_rgn_init(&dest_region, drawable, 0, 0, width, height, TRUE, TRUE);

  buffer = (guchar *)g_malloc(row_stride);
  
  for (row = y1; row < y2; row++)
    {
      
      gimp_pixel_rgn_get_row(&src_region, buffer, x1, row, width);
      if (direction == RIGHT)
	{
	  reverse_buffer(buffer, row_stride, bytes);
	}
      marker = render_blast_row(buffer, bytes, lpi, threshold, strength, edge);
      if (direction == RIGHT)
	{
	  reverse_buffer(buffer, row_stride, bytes);
	}
      gimp_pixel_rgn_set_row(&dest_region, buffer, x1, row, width);
      gimp_progress_update((double) row / (double) (y2 - y1));
      if (marker)
	{
	  gint j, limit;
	  
	  limit = 1 + rand() % 2;
	  for (j = 0; (j < limit) && (row < y2); j++)
	    {
	      row++;
	      if (row < y2)
		{
		  gimp_pixel_rgn_get_row(&src_region, buffer, x1, row, width);
		  gimp_pixel_rgn_set_row(&dest_region, buffer, x1, row, width);
		}
	    }
	  marker = 0;
	}
    }
  g_free(buffer);
  
  /*  update the region  */
  gimp_drawable_flush(drawable);
  gimp_drawable_merge_shadow(drawable->id, TRUE);
  gimp_drawable_update(drawable->id, x1, y1, x2 - x1, y2 - y1);

  return;
}

static void
render_wind (GDrawable   *drawable,
	     gint         threshold,
	     gint         strength,
	     direction_t  direction,
	     edge_t       edge)
{
  GPixelRgn src_region, dest_region;
  gint width = drawable->width;
  gint height = drawable->height;
  gint bytes = drawable->bpp;
  gint row_stride = width * bytes;
  gint comp_stride = bytes * COMPARE_WIDTH;
  gint row;
  guchar *sb;
  gint lpi = row_stride - comp_stride;
  gint x1, y1, x2, y2;

  gimp_drawable_mask_bounds(drawable->id, &x1, &y1, &x2, &y2);
  gimp_pixel_rgn_init(&src_region, drawable, 0, 0, width, height,
		      FALSE, FALSE);
  gimp_pixel_rgn_init(&dest_region, drawable, 0, 0, width, height, TRUE, TRUE);
  sb = g_malloc(row_stride);
  
  for (row = y1; row < y2; row++)
    {
      gimp_pixel_rgn_get_row(&src_region, sb, x1, row, width);
      if (direction == RIGHT)
	{
	  reverse_buffer(sb, row_stride, bytes);
	}
      render_wind_row(sb, bytes, lpi, threshold, strength, edge);
      if (direction == RIGHT)
	{
	  reverse_buffer(sb, row_stride, bytes);
	}
      gimp_pixel_rgn_set_row(&dest_region, sb, x1, row, width);
      gimp_progress_update((gdouble) row / (gdouble) (y2 - y1));
    }
  g_free(sb);
  gimp_drawable_flush(drawable);
  gimp_drawable_merge_shadow(drawable->id, TRUE);
  gimp_drawable_update(drawable->id, x1, y1, x2 - x1, y2 - y1);
  return;
}

static gint
render_blast_row (guchar *buffer,
		  gint    bytes,
		  gint    lpi,
		  gint    threshold,
		  gint    strength,
		  edge_t  edge)
{
  gint Ri, Gi, Bi;
  gint sbi, lbi;
  gint bleed_length;
  gint i, j;
  gint weight, random_factor;
  gint skip = 0;

  for (j = 0; j < lpi; j += bytes)
    {
      Ri = j; Gi = j + 1; Bi = j + 2;
	  
      if (threshold_exceeded(buffer+Ri, buffer+Ri+bytes, edge, threshold))
	{
	  /* we have found an edge, do bleeding */
	  sbi = Ri;
	      
	  weight = rand() % 10;
	  if (weight > 5)
	    {
	      random_factor = 2;
	    }
	  else if (weight > 3)
	    {
	      random_factor = 3;
	    }
	  else
	    {
	      random_factor = 4;
	    }
	  bleed_length = 0;
	  switch (rand() % random_factor)
	    {
	    case 3:
	      bleed_length += strength;
	      /* fall through to add up multiples of strength */
	    case 2:
	      bleed_length += strength;
	      /* fall through */
	    case 1:
	      bleed_length += strength;
	      /* fall through */
	    case 0:
	      bleed_length += strength;
	      /* fall through */
	    }

	  lbi = sbi + bytes * bleed_length;
	  if (lbi > lpi)
	    {
	      lbi = lpi;
	    }

	  for (i = sbi; i < lbi; i += bytes)
	    {
	      buffer[i] = buffer[Ri];
	      buffer[i+1] = buffer[Gi];
	      buffer[i+2] = buffer[Bi];
	    }
	  j = lbi - bytes;
	  if ((rand() % 10) > 7)
	    {
	      skip = 1;
	    }
	}
    }
  return skip;
}

static void
render_wind_row (guchar *sb,
		 gint    bytes,
		 gint    lpi,
		 gint    threshold,
		 gint    strength,
		 edge_t  edge)
{
  gint i, j;
  gint bleed_length;
  gint blend_amt_R, blend_amt_G, blend_amt_B;
  gint blend_colour_R, blend_colour_G, blend_colour_B;
  gint target_colour_R, target_colour_G, target_colour_B;
  gdouble bleed_length_max;
  gint bleed_variation;
  gint n;
  gint sbi;  /* starting bleed index */
  gint lbi;	 /* last bleed index */
  gdouble denominator;
  gint comp_stride = bytes * COMPARE_WIDTH;
  
  for (j = 0; j < lpi; j += bytes)
    {
      gint Ri = j;
      gint Gi = j + 1;
      gint Bi = j + 2;

      if (threshold_exceeded(sb+Ri, sb+Ri+comp_stride, edge, threshold))
	{
	  /* we have found an edge, do bleeding */
	  sbi = Ri + comp_stride;
	  blend_colour_R = sb[Ri];
	  blend_colour_G = sb[Gi];
	  blend_colour_B = sb[Bi];
	  target_colour_R = sb[sbi];
	  target_colour_G = sb[sbi+1];
	  target_colour_B = sb[sbi+2];
	  bleed_length_max = strength;

	  if (rand() % 3) /* introduce weighted randomness */
	    {
	      bleed_length_max = strength;
	    }
	  else
	    {
	      bleed_length_max = 4 * strength;
	    }

	  bleed_variation = 1
	    + (gint) (bleed_length_max * rand() / (RAND_MAX + 1.0));

	  lbi = sbi + bleed_variation * bytes;
	  if (lbi > lpi)
	    {
	      lbi = lpi; /* stop overunning the buffer */
	    }

	  bleed_length = bleed_variation;

	  blend_amt_R = target_colour_R - blend_colour_R;
	  blend_amt_G = target_colour_G - blend_colour_G;
	  blend_amt_B = target_colour_B - blend_colour_B;
	  denominator = bleed_length * bleed_length + bleed_length;
	  denominator = 2.0 / denominator;
	  n = bleed_length;
	  for (i = sbi; i < lbi; i += bytes)
	    {

	      /* check against original colour */
	      if (!threshold_exceeded(sb+Ri, sb+i, edge, threshold)
		  && (rand() % 2))
		{
		  break;
		}

	      blend_colour_R += blend_amt_R * n * denominator;
	      blend_colour_G += blend_amt_G * n * denominator;
	      blend_colour_B += blend_amt_B * n * denominator;

	      if (blend_colour_R > 255) blend_colour_R = 255;
	      else if (blend_colour_R < 0) blend_colour_R = 0;
	      if (blend_colour_G > 255) blend_colour_G = 255;
	      else if (blend_colour_G < 0) blend_colour_G = 0;
	      if (blend_colour_B > 255) blend_colour_B = 255;
	      else if (blend_colour_B < 0) blend_colour_B = 0;

	      sb[i] = (blend_colour_R * 2 + sb[i]) / 3;
	      sb[i+1] = (blend_colour_G * 2 + sb[i+1]) / 3;
	      sb[i+2] = (blend_colour_B * 2 + sb[i+2]) / 3;

	      if (threshold_exceeded(sb+i, sb+i+comp_stride, BOTH,
				     threshold))
		{
		  target_colour_R = sb[i+comp_stride];
		  target_colour_G = sb[i+comp_stride+1];
		  target_colour_B = sb[i+comp_stride+2];
		  blend_amt_R = target_colour_R - blend_colour_R;
		  blend_amt_G = target_colour_G - blend_colour_G;
		  blend_amt_B = target_colour_B - blend_colour_B;
		  denominator = n * n + n;
		  denominator = 2.0 / denominator;
		}
	      n--;
	    }
	}
    }
  return;
}

static gint
threshold_exceeded (guchar *pixel_R1,
		    guchar *pixel_R2,
		    edge_t  edge,
		    gint    threshold)
{
  gint derivative_R, derivative_G, derivative_B;
  gint return_value;

  get_derivative(pixel_R1, pixel_R2, edge,
		 &derivative_R, &derivative_G, &derivative_B);

  if(((derivative_R + derivative_G + derivative_B) / 3) > threshold)
    {
      return_value = 1;
    }
  else
    {
      return_value = 0;
    }
  return return_value;
}

static void
get_derivative (guchar *pixel_R1,
		guchar *pixel_R2,
		edge_t  edge,
		gint   *derivative_R,
		gint   *derivative_G,
		gint   *derivative_B)
{
  guchar *pixel_G1 = pixel_R1 + 1;
  guchar *pixel_B1 = pixel_R1 + 2;
  guchar *pixel_G2 = pixel_R2 + 1;
  guchar *pixel_B2 = pixel_R2 + 2;

  *derivative_R = *pixel_R2 - *pixel_R1;
  *derivative_G = *pixel_G2 - *pixel_G1;
  *derivative_B = *pixel_B2 - *pixel_B1;
  
  if (edge == BOTH)
    {
      *derivative_R = abs(*derivative_R);
      *derivative_G = abs(*derivative_G);
      *derivative_B = abs(*derivative_B);
    }
  else if (edge == LEADING)
    {
      *derivative_R = -(*derivative_R);
      *derivative_G = -(*derivative_G);
      *derivative_B = -(*derivative_B);
    }
  else if (edge == TRAILING)
    {
      /* no change needed */
    }
  return;
}

static void
reverse_buffer (guchar *buffer,
		gint    length,
		gint    bytes)
{
  gint i, si;
  gint temp;
  gint midpoint;
  
  midpoint = length / 2;
  for (i = 0; i < midpoint; i += bytes)
    {
      si = length - bytes - i;
      
      temp = buffer[i];
      buffer[i] = buffer[si];
      buffer[si] = (guchar) temp;

      temp = buffer[i+1];
      buffer[i+1] = buffer[si+1];
      buffer[si+1] = (guchar) temp;

      temp = buffer[i+2];
      buffer[i+2] = buffer[si+2];
      buffer[si+2] = (guchar) temp;
    }

  return;
}

/***************************************************
  GUI 
 ***************************************************/

static void
msg_ok_callback (GtkWidget *widget,
		 gpointer   data)
{
  gtk_grab_remove (GTK_WIDGET (data));
  gtk_widget_destroy (GTK_WIDGET (data));

  return;
}

static void
modal_message_box (gchar *text)
{
  GtkWidget *message_box;
  GtkWidget *label;

  message_box = gimp_dialog_new ("Wind", "wind",
				 gimp_plugin_help_func, "filters/wind.html",
				 GTK_WIN_POS_MOUSE,
				 FALSE, TRUE, FALSE,

				 "OK", msg_ok_callback,
				 NULL, NULL, NULL, TRUE, TRUE,

				 NULL);

  label = gtk_label_new (text);
  gtk_misc_set_alignment (GTK_MISC (label), 0, 0);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (message_box)->vbox), label,
		      TRUE, TRUE, 0);
  gtk_widget_show (label);

  gtk_grab_add (message_box);
  gtk_widget_show (message_box);
}

static void
ok_callback (GtkWidget *widget,
	     gpointer   data)
{
  /* we have to stop the dialog from being closed with strength < 1 */

  if (config.strength < 1)
    {
      modal_message_box (NEGATIVE_STRENGTH_TEXT);
    }
  else
    {
      dialog_result = 1;
      gtk_widget_destroy (GTK_WIDGET (data));
    }
}

static void
dialog_box (void)
{
  GtkWidget *main_vbox;
  GtkWidget *table;
  GtkObject *adj;
  GtkWidget *frame;
  GtkWidget *dlg;
  gchar  **argv;
  gint     argc;
 
  argc    = 1;
  argv    = g_new (gchar *, 1);
  argv[0] = g_strdup (PLUG_IN_NAME);

  gtk_init (&argc, &argv);
  gtk_rc_parse (gimp_gtkrc ());

  dlg = gimp_dialog_new ("Wind", "wind",
			 gimp_plugin_help_func, "filters/wind.html",
			 GTK_WIN_POS_MOUSE,
			 FALSE, TRUE, FALSE,

			 "OK", ok_callback,
			 NULL, NULL, NULL, TRUE, FALSE,
			 "Cancel", gtk_widget_destroy,
			 NULL, 1, NULL, FALSE, TRUE,

			 NULL);

  gtk_signal_connect (GTK_OBJECT (dlg), "destroy",
		      GTK_SIGNAL_FUNC (gtk_main_quit),
		      NULL);

  /* init tooltips */
  gimp_help_init ();

  main_vbox = gtk_vbox_new (FALSE, 6);
  gtk_container_add (GTK_CONTAINER (GTK_DIALOG (dlg)->vbox), main_vbox);
  gtk_container_set_border_width (GTK_CONTAINER (main_vbox), 6);
  gtk_widget_show (main_vbox);

  /****************************************************
   frame for sliders
   ****************************************************/
  frame = gtk_frame_new (NULL);
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_ETCHED_IN);
  gtk_box_pack_start (GTK_BOX(main_vbox), frame, FALSE, FALSE, 0);

  table = gtk_table_new (2, 3, FALSE);
  gtk_table_set_col_spacings (GTK_TABLE (table), 4);
  gtk_table_set_row_spacings (GTK_TABLE (table), 2);
  gtk_container_set_border_width (GTK_CONTAINER(table), 4);
  gtk_container_add (GTK_CONTAINER (frame), table);
 
  /*****************************************************
    slider and entry for threshold
    ***************************************************/

  adj = gimp_scale_entry_new (GTK_TABLE (table), 0, 0,
			      "Threshold:", SCALE_WIDTH, 0,
			      config.threshold,
			      MIN_THRESHOLD, MAX_THRESHOLD, 1.0, 10, 0,
			      THRESHOLD_TEXT, NULL);
  gtk_signal_connect (GTK_OBJECT (adj), "value_changed",
		      GTK_SIGNAL_FUNC (gimp_int_adjustment_update),
		      &config.threshold);

  /*****************************************************
    slider and entry for strength of wind
    ****************************************************/

  adj = gimp_scale_entry_new (GTK_TABLE (table), 0, 1,
			      "Strength:", SCALE_WIDTH, 0,
			      config.strength,
			      MIN_STRENGTH, MAX_STRENGTH, 1.0, 10.0, 0,
			      STRENGTH_TEXT, NULL);
  gtk_signal_connect (GTK_OBJECT (adj), "value_changed",
		      GTK_SIGNAL_FUNC (gimp_int_adjustment_update),
		      &config.strength);

  gtk_widget_show (table);
  gtk_widget_show (frame);

  /*****************************************************
    outer frame and table
  ***************************************************/

  table = gtk_table_new (1, 3, FALSE);
  gtk_table_set_col_spacings (GTK_TABLE(table), 4);
  gtk_box_pack_start (GTK_BOX (main_vbox), table, FALSE, FALSE, 0);

  /*********************************************************
    radio buttons for choosing wind rendering algorithm
    ******************************************************/

  frame = gimp_radio_group_new2 (TRUE, "Style",
				 gimp_radio_button_update,
				 &config.alg, (gpointer) config.alg,

				 "Wind",  (gpointer) RENDER_WIND, NULL,
				 "Blast", (gpointer) RENDER_BLAST, NULL,

				 NULL);

  gtk_table_attach (GTK_TABLE (table), frame, 0, 1, 0, 1,
		    GTK_FILL | GTK_EXPAND, GTK_FILL | GTK_EXPAND, 0, 0);
  gtk_widget_show (frame);

  /******************************************************
    radio buttons for choosing LEFT or RIGHT
    **************************************************/

  frame = gimp_radio_group_new2 (TRUE, "Direction",
				 gimp_radio_button_update,
				 &config.direction, (gpointer) config.direction,

				 "Left",  (gpointer) LEFT, NULL,
				 "Right", (gpointer) RIGHT, NULL,

				 NULL);

  gtk_table_attach (GTK_TABLE (table), frame, 1, 2, 0, 1,
		    GTK_FILL | GTK_EXPAND, GTK_FILL | GTK_EXPAND, 0, 0);
  gtk_widget_show (frame);
  
  /*****************************************************
    radio buttons for choosing BOTH, LEADING, TRAILING
    ***************************************************/

  frame = gimp_radio_group_new2 (TRUE, "Edge Affected",
				 gimp_radio_button_update,
				 &config.edge, (gpointer) config.edge,

				 "Leading",  (gpointer) LEADING, NULL,
				 "Trailing", (gpointer) TRAILING, NULL,
				 "Both",     (gpointer) BOTH, NULL,

				 NULL);

  gtk_table_attach (GTK_TABLE (table), frame, 2, 3, 0, 1,
		    GTK_FILL | GTK_EXPAND, GTK_FILL | GTK_EXPAND, 0, 0);
  gtk_widget_show (frame);

  gtk_widget_show (table);
  gtk_widget_show (dlg);

  gtk_main ();
  gdk_flush ();
}
