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
#include "libgimp/gimp.h"


#define PLUG_IN_NAME "wind"

#define COMPARE_WIDTH 3

#define ENTRY_WIDTH 40
#define SCALE_WIDTH 200
#define MIN_THRESHOLD 0
#define MAX_THRESHOLD 50
#define MIN_STRENGTH 1
#define MAX_STRENGTH 50

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

typedef enum {LEFT, RIGHT} direction_t;
typedef enum {RENDER_WIND, RENDER_BLAST} algorithm_t;
typedef enum {BOTH, LEADING, TRAILING} edge_t;


static void query(void);
static void run(char *name, int nparams, GParam *param,
		int *nreturn_vals, GParam **return_vals);
static void dialog_box(void);
static gint render_effect(GDrawable *drawable);
static void render_wind(GDrawable *drawable, gint threshold, gint strength,
			direction_t direction, edge_t edge);
static void render_blast(GDrawable *drawable, gint threshold, gint strength,
			 direction_t direction, edge_t edge);
static gint render_blast_row(guchar *buffer, gint bytes, gint lpi, gint threshold,
		       gint strength, edge_t edge);
static void render_wind_row(guchar *sb, gint bytes, gint lpi, gint threshold,
		      gint strength, edge_t edge);
static void msg_ok_callback(GtkWidget *widget, gpointer data);
static void msg_close_callback(GtkWidget *widget, gpointer data);
static void close_callback(GtkWidget *widget, gpointer data);
static void ok_callback(GtkWidget *widget, gpointer data);
static void entry_callback(GtkWidget *widget, gpointer data);
static void radio_button_alg_callback(GtkWidget *widget, gpointer data);
static void radio_button_direction_callback(GtkWidget *widget, gpointer data);
static void get_derivative(guchar *pixel_R1, guchar *pixel_R2,
			   edge_t edge, gint *derivative_R,
			   gint *derivative_G, gint *derivative_B);
static gint threshold_exceeded(guchar *pixel_R1, guchar *pixel_R2,
			       edge_t edge, gint threshold);
static void reverse_buffer(guchar *buffer, gint length, gint bytes);
static void modal_message_box(gchar *text);

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
  gint threshold;            /* derivative comparison for edge detection */
  direction_t direction;     /* of wind, LEFT or RIGHT */
  gint strength;	     /* how many pixels to bleed */
  algorithm_t alg;           /* which algorithm */
  edge_t edge;               /* controls abs, negation of derivative */
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
query(void)
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

  gimp_install_procedure("plug_in_wind",
			 "Renders a wind effect.",
			 "Renders a wind effect.",
			 "Nigel Wetten",
			 "Nigel Wetten",
			 "1998",
			 "<Image>/Filters/Distorts/Wind",
			 "RGB*",
			 PROC_PLUG_IN,
			 nargs, nreturn_vals,
			 args, return_vals);
  return;
}  /* query */

/*****/

static void
run(gchar *name, gint nparams, GParam *param, gint *nreturn_vals,
    GParam **return_vals)
{
  static GParam values[1];
  GDrawable *drawable;
  GRunModeType run_mode;
  GStatusType status = STATUS_SUCCESS;

  run_mode = param[0].data.d_int32;
  drawable = gimp_drawable_get(param[2].data.d_drawable);
  gimp_tile_cache_ntiles(drawable->width / gimp_tile_width() + 1);
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

    }  /* switch */
      
  gimp_drawable_detach(drawable);
  
  *nreturn_vals = 1;
  *return_vals = values;
  values[0].type = PARAM_STATUS;
  values[0].data.d_status = status;

  return;
}  /* run */

/*****/

static gint
render_effect(GDrawable *drawable)
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
}  /* render_effect */

/*****/

static void
render_blast(GDrawable *drawable, gint threshold, gint strength,
	     direction_t direction, edge_t edge)
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

  buffer = malloc(row_stride);
  
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
    }  /* for */
  free(buffer);
  
  /*  update the region  */
  gimp_drawable_flush(drawable);
  gimp_drawable_merge_shadow(drawable->id, TRUE);
  gimp_drawable_update(drawable->id, x1, y1, x2 - x1, y2 - y1);

  return;
}  /* render_blast */

/*****/

static void
render_wind(GDrawable *drawable, gint threshold, gint strength,
	    direction_t direction, edge_t edge)
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
  free(sb);
  gimp_drawable_flush(drawable);
  gimp_drawable_merge_shadow(drawable->id, TRUE);
  gimp_drawable_update(drawable->id, x1, y1, x2 - x1, y2 - y1);
  return;
}  /* render_wind */

/*****/

static gint
render_blast_row(guchar *buffer, gint bytes, gint lpi, gint threshold,
		 gint strength, edge_t edge)
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
	}  /* if */
    }  /* for j=0 */
  return skip;
}  /* render_blast_row */

/*****/

static void
render_wind_row(guchar *sb, gint bytes, gint lpi, gint threshold,
		gint strength, edge_t edge)
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
	}  /* if */
    }  /* for j=0 */
  return;
}  /* render_wind_row */

/*****/

static gint
threshold_exceeded(guchar *pixel_R1, guchar *pixel_R2, edge_t edge,
		   gint threshold)
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
}  /* threshold_exceeded */

/*****/

static void
get_derivative(guchar *pixel_R1, guchar *pixel_R2, edge_t edge,
	       gint *derivative_R, gint *derivative_G, gint *derivative_B)
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
}  /* get_derivative */

/*****/

static void
reverse_buffer(guchar *buffer, gint length, gint bytes)
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
}  /* reverse_buffer */

/*****/

/***************************************************
  GUI 
 ***************************************************/

static void
msg_ok_callback(GtkWidget *widget, gpointer data)
{
  gtk_grab_remove(GTK_WIDGET(data));
  gtk_widget_destroy(GTK_WIDGET(data));
  return;
}  /* msg_ok_callback */

/*****/

static void
msg_close_callback(GtkWidget *widget, gpointer data)
{
  return;
}  /* msg_close_callback */

/*****/

static void
modal_message_box(gchar *text)
{
  GtkWidget *message_box;
  GtkWidget *button;
  GtkWidget *label;

  message_box = gtk_dialog_new();
  gtk_window_set_title(GTK_WINDOW(message_box), "Ooops!");
  gtk_window_position(GTK_WINDOW(message_box), GTK_WIN_POS_MOUSE);
  gtk_signal_connect(GTK_OBJECT(message_box), "destroy",
		     (GtkSignalFunc) msg_close_callback, NULL);

  label = gtk_label_new(text);
  gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
  gtk_box_pack_start(GTK_BOX(GTK_DIALOG(message_box)->vbox), label,
		     TRUE, TRUE, 0);
  gtk_widget_show(label);
      
  button = gtk_button_new_with_label("OK");
  GTK_WIDGET_SET_FLAGS(button, GTK_CAN_DEFAULT);
  gtk_signal_connect(GTK_OBJECT(button), "clicked",
		     (GtkSignalFunc) msg_ok_callback, message_box);
  gtk_box_pack_start(GTK_BOX(GTK_DIALOG(message_box)->action_area),
		     button, TRUE, TRUE, 0);
  gtk_widget_grab_default(button);
  gtk_widget_show(button);

  gtk_grab_add(message_box);
  gtk_widget_show(message_box);
  return;
}  /* modal_message_box */

/*****/

static void
close_callback(GtkWidget *widget, gpointer data)
{
  gtk_main_quit();
  return;
}  /* close_callback */

/*****/

static void
ok_callback(GtkWidget *widget, gpointer data)
{
  /* we have to stop the dialog from being closed with strength < 1 */
      
  if (config.strength < 1)
    {
      modal_message_box(NEGATIVE_STRENGTH_TEXT);
    }
  else
    {
      dialog_result = 1;
      gtk_widget_destroy(GTK_WIDGET(data));
    }
  return;
}  /* ok_callback */

/*****/

static void
entry_callback(GtkWidget *widget, gpointer data)
{
  GtkAdjustment *adjustment;
  gint new_value;

  new_value = atoi(gtk_entry_get_text(GTK_ENTRY(widget)));

  if (*(gint *) data != new_value)
    {
      *(gint *) data = new_value;
      adjustment = gtk_object_get_user_data(GTK_OBJECT(widget));
      if ((new_value >= adjustment-> lower)
	  && (new_value <= adjustment->upper))
	{
	  adjustment->value = new_value;
	  gtk_signal_handler_block_by_data(GTK_OBJECT(adjustment), data);
	  gtk_signal_emit_by_name(GTK_OBJECT(adjustment), "value_changed");
	  gtk_signal_handler_unblock_by_data(GTK_OBJECT(adjustment), data);
	}
    }
  return;
}  /* entry_callback */
	    
/*****/

static void
radio_button_alg_callback(GtkWidget *widget, gpointer data)
{
  if (GTK_TOGGLE_BUTTON (widget)->active) /* button is TRUE, i.e. checked */
    {
      config.alg = (algorithm_t) data;
    }
  return;
}  /* radio_button_alg_callback */

/*****/

static void
radio_button_direction_callback(GtkWidget *widget, gpointer data)
{
  if (GTK_TOGGLE_BUTTON (widget)->active)  /* button is checked */
    {
      config.direction = (direction_t) data;
    }
  return;
}  /* radio_button_direction_callback */

/*****/

static void
radio_button_edge_callback(GtkWidget *widget, gpointer data)
{
  if (GTK_TOGGLE_BUTTON (widget)->active) /* button is selected */
    {
      config.edge = (edge_t) data;
    }
  return;
}  /* radio_button_edge_callback */

/*****/

static void
adjustment_callback(GtkAdjustment *adjustment, gpointer data)
{
  GtkWidget *entry;
  gchar buffer[50];

  if (*(gint *)data != adjustment->value)
    {
      *(gint *) data = adjustment->value;
      entry = gtk_object_get_user_data(GTK_OBJECT(adjustment));
      sprintf(buffer, "%d", *(gint *) data);
      gtk_signal_handler_block_by_data(GTK_OBJECT(entry), data);
      gtk_entry_set_text(GTK_ENTRY(entry), buffer);
      gtk_signal_handler_unblock_by_data(GTK_OBJECT(entry), data);
    }
  return;
}  /* adjustment_callback */

/*****/

static void
dialog_box(void)
{
  GtkWidget *table;
  GtkWidget *outer_table;
  GtkObject *adjustment;
  GtkWidget *scale;
  GtkWidget *rbutton;
  GSList *list;
  gchar *text_label;
  GtkWidget *frame;
  GtkWidget *outer_frame;
  GtkWidget *dlg;
  GtkWidget *button;
  GtkWidget *label;
  GtkWidget *entry;
  gchar buffer[12];
  gchar **argv;
  gint argc;
  GtkTooltips *tooltips;
  GdkColor tips_fg, tips_bg;

  argc = 1;
  argv = g_new(gchar *, 1);
  argv[0] = g_strdup(PLUG_IN_NAME);

  gtk_init(&argc, &argv);
  gtk_rc_parse(gimp_gtkrc());

  dlg = gtk_dialog_new();
  gtk_window_set_title(GTK_WINDOW(dlg), PLUG_IN_NAME);
  gtk_window_position(GTK_WINDOW(dlg), GTK_WIN_POS_MOUSE);
  gtk_signal_connect(GTK_OBJECT(dlg),
		     "destroy", (GtkSignalFunc) close_callback, NULL);

  /*  Action area  */
  button = gtk_button_new_with_label("OK");
  GTK_WIDGET_SET_FLAGS(button, GTK_CAN_DEFAULT);
  gtk_signal_connect(GTK_OBJECT(button), "clicked",
		     (GtkSignalFunc) ok_callback, dlg);
  gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dlg)->action_area), button,
		     TRUE, TRUE, 0);
  gtk_widget_grab_default(button);
  gtk_widget_show(button);

  button = gtk_button_new_with_label("Cancel");
  GTK_WIDGET_SET_FLAGS(button, GTK_CAN_DEFAULT);
  gtk_signal_connect_object(GTK_OBJECT(button), "clicked",
			    (GtkSignalFunc) gtk_widget_destroy,
			    GTK_OBJECT(dlg));
  gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dlg)->action_area), button,
		     TRUE, TRUE, 0);
  gtk_widget_show(button);

  /* init tooltips */
  tooltips = gtk_tooltips_new();
  tips_fg.red = 0;
  tips_fg.green = 0;
  tips_fg.blue = 0;
  gdk_color_alloc(gtk_widget_get_colormap(dlg), &tips_fg);
  tips_bg.red = 61669;
  tips_bg.green = 59113;
  tips_bg.blue = 35979;
  gdk_color_alloc(gtk_widget_get_colormap(dlg), &tips_bg);
  gtk_tooltips_set_colors(tooltips, &tips_bg, &tips_fg);
  
  /****************************************************
   frame for sliders
   ****************************************************/
  frame = gtk_frame_new(NULL);
  gtk_frame_set_shadow_type(GTK_FRAME(frame), GTK_SHADOW_ETCHED_IN);
  gtk_container_border_width(GTK_CONTAINER(frame), 10);
  gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dlg)->vbox), frame, TRUE, TRUE, 0);

  table = gtk_table_new(3, 2, FALSE);
  gtk_container_border_width(GTK_CONTAINER(table), 10);
  gtk_container_add(GTK_CONTAINER(frame), table);
 
  /*****************************************************
    slider and entry for threshold
    ***************************************************/
  
  label = gtk_label_new("Threshold:");
  gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
  gtk_table_attach(GTK_TABLE(table), label, 0, 1, 0, 1, GTK_FILL, 0, 0, 0);
  gtk_widget_show(label);

  adjustment = gtk_adjustment_new(config.threshold, MIN_THRESHOLD,
				  MAX_THRESHOLD, 1.0, 1.0, 0);
  gtk_signal_connect(GTK_OBJECT(adjustment), "value_changed",
		     (GtkSignalFunc) adjustment_callback,
		     &config.threshold);
  scale = gtk_hscale_new(GTK_ADJUSTMENT(adjustment));
  gtk_widget_set_usize(scale, SCALE_WIDTH, 0);
  gtk_table_attach(GTK_TABLE(table), scale, 1, 2, 0, 1, GTK_FILL, 0, 0, 0);
  gtk_scale_set_draw_value(GTK_SCALE(scale), FALSE);
  gtk_range_set_update_policy(GTK_RANGE(scale), GTK_UPDATE_CONTINUOUS);
  gtk_widget_show(scale);
  gtk_tooltips_set_tip(tooltips, scale, THRESHOLD_TEXT, NULL);
  
  entry = gtk_entry_new();
  gtk_object_set_user_data(GTK_OBJECT(entry), adjustment);
  gtk_object_set_user_data(GTK_OBJECT(adjustment), entry);
  gtk_widget_set_usize(entry, ENTRY_WIDTH, 0);
  sprintf(buffer, "%i", config.threshold);
  gtk_entry_set_text(GTK_ENTRY(entry), buffer);
  gtk_signal_connect(GTK_OBJECT(entry), "changed",
		     GTK_SIGNAL_FUNC(entry_callback),
		     (gpointer) &config.threshold);
  gtk_table_attach(GTK_TABLE(table), entry, 2, 3, 0, 1, GTK_FILL, 0, 0, 0);
  gtk_widget_show(entry);
  gtk_tooltips_set_tip(tooltips, entry, THRESHOLD_TEXT, NULL);
	
  /*****************************************************
    slider and entry for strength of wind
    ****************************************************/

  label = gtk_label_new("Strength:");
  gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
  gtk_table_attach(GTK_TABLE(table), label, 0, 1, 1, 2, GTK_FILL, 0, 0, 0);
  gtk_widget_show(label);

  adjustment = gtk_adjustment_new(config.strength, MIN_STRENGTH,
				  MAX_STRENGTH, 1.0, 1.0, 0);
  gtk_signal_connect(GTK_OBJECT(adjustment), "value_changed",
		     (GtkSignalFunc) adjustment_callback,
		     &config.strength);
  scale = gtk_hscale_new(GTK_ADJUSTMENT(adjustment));
  gtk_widget_set_usize(scale, SCALE_WIDTH, 0);
  gtk_table_attach(GTK_TABLE(table), scale, 1, 2, 1, 2, GTK_FILL, 0, 0, 0);
  gtk_scale_set_draw_value(GTK_SCALE(scale), FALSE);
  gtk_range_set_update_policy(GTK_RANGE(scale), GTK_UPDATE_CONTINUOUS);
  gtk_widget_show(scale);
  gtk_tooltips_set_tip(tooltips, scale, STRENGTH_TEXT, NULL);

  entry = gtk_entry_new();
  gtk_object_set_user_data(GTK_OBJECT(entry), adjustment);
  gtk_object_set_user_data(GTK_OBJECT(adjustment), entry);
  gtk_widget_set_usize(entry, ENTRY_WIDTH, 0);
  sprintf(buffer, "%i", config.strength);
  gtk_entry_set_text(GTK_ENTRY(entry), buffer);
  gtk_signal_connect(GTK_OBJECT(entry), "changed",
		     GTK_SIGNAL_FUNC(entry_callback),
		     (gpointer) &config.strength);
  gtk_table_attach(GTK_TABLE(table), entry, 2, 3, 1, 2, GTK_FILL, 0, 0, 0);
  gtk_widget_show(entry);
  gtk_tooltips_set_tip(tooltips, entry, STRENGTH_TEXT, NULL);

  gtk_widget_show(table);
  gtk_widget_show(frame);

  /*****************************************************
    outer frame and table
  ***************************************************/
  
  outer_frame = gtk_frame_new(NULL);
  gtk_frame_set_shadow_type(GTK_FRAME(outer_frame), GTK_SHADOW_NONE);
  gtk_container_border_width(GTK_CONTAINER(outer_frame), 10);
  gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dlg)->vbox), outer_frame,
		     TRUE, TRUE, 0);

  outer_table = gtk_table_new(3, 3, FALSE);
  gtk_container_border_width(GTK_CONTAINER(outer_table), 0);
  gtk_table_set_col_spacings(GTK_TABLE(outer_table), 10);
  gtk_container_add(GTK_CONTAINER(outer_frame), outer_table);

  /*********************************************************
    radio buttons for choosing wind rendering algorithm
    ******************************************************/

  frame = gtk_frame_new("Style");
  gtk_frame_set_shadow_type(GTK_FRAME(frame), GTK_SHADOW_ETCHED_IN);
  gtk_container_border_width(GTK_CONTAINER(frame), 0);
  gtk_table_attach(GTK_TABLE(outer_table), frame, 0, 1, 0, 3,
		   GTK_FILL | GTK_EXPAND, GTK_FILL | GTK_EXPAND, 0, 0);

  table = gtk_table_new(3, 2, FALSE);
  gtk_container_border_width(GTK_CONTAINER(table), 10);
  gtk_container_add(GTK_CONTAINER(frame), table);
  
  text_label = "Wind";
  rbutton = gtk_radio_button_new_with_label(NULL, text_label);
  list = gtk_radio_button_group((GtkRadioButton *) rbutton);
  gtk_toggle_button_set_active((GtkToggleButton *) rbutton,
			      config.alg == RENDER_WIND ? TRUE : FALSE);
  gtk_signal_connect(GTK_OBJECT(rbutton), "toggled",
		     GTK_SIGNAL_FUNC (radio_button_alg_callback),
		     (gpointer) RENDER_WIND);
  gtk_table_attach(GTK_TABLE(table), rbutton, 0, 1, 0, 1, GTK_FILL, 0, 0, 0);
  gtk_widget_show(rbutton);
  gtk_tooltips_set_tip(tooltips, rbutton, WIND_TEXT, NULL);
	
  text_label = "Blast";
  rbutton = gtk_radio_button_new_with_label(list, text_label);
  list = gtk_radio_button_group((GtkRadioButton *) rbutton);
  gtk_toggle_button_set_active((GtkToggleButton *) rbutton,
			      config.alg == RENDER_BLAST ? TRUE : FALSE);
  gtk_signal_connect(GTK_OBJECT(rbutton), "toggled",
		     GTK_SIGNAL_FUNC (radio_button_alg_callback),
		     (gpointer) RENDER_BLAST);
  gtk_table_attach(GTK_TABLE(table), rbutton, 0, 1, 1, 2, GTK_FILL, 0, 0, 0);
  gtk_widget_show(rbutton);
  gtk_tooltips_set_tip(tooltips, rbutton, BLAST_TEXT, NULL);

  gtk_widget_show(table);
  gtk_widget_show(frame);
  
  /******************************************************
    radio buttons for choosing LEFT or RIGHT
    **************************************************/
  frame = gtk_frame_new("Direction");
  gtk_frame_set_shadow_type(GTK_FRAME(frame), GTK_SHADOW_ETCHED_IN);
  gtk_container_border_width(GTK_CONTAINER(frame), 0);
  gtk_table_attach(GTK_TABLE(outer_table), frame, 1, 2, 0, 3,
		   GTK_FILL | GTK_EXPAND, GTK_FILL | GTK_EXPAND, 0, 0);

  table = gtk_table_new(1, 2, FALSE);
  gtk_container_border_width(GTK_CONTAINER(table), 10);
  gtk_container_add(GTK_CONTAINER(frame), table);
		     
  rbutton = gtk_radio_button_new_with_label(NULL, "Left");
  list = gtk_radio_button_group((GtkRadioButton *) rbutton);
  gtk_toggle_button_set_active((GtkToggleButton *) rbutton,
			      config.direction == LEFT ? TRUE : FALSE);
  gtk_signal_connect(GTK_OBJECT(rbutton), "toggled",
		     GTK_SIGNAL_FUNC(radio_button_direction_callback),
		     (gpointer) LEFT);
  gtk_table_attach(GTK_TABLE(table), rbutton, 0, 1, 0, 1, GTK_FILL, 0, 0, 0);
  gtk_widget_show(rbutton);
  gtk_tooltips_set_tip(tooltips, rbutton, LEFT_TEXT, NULL);

  rbutton = gtk_radio_button_new_with_label(list, "Right");
  gtk_toggle_button_set_active((GtkToggleButton *) rbutton,
			      config.direction == RIGHT ? TRUE : FALSE);
  gtk_signal_connect(GTK_OBJECT(rbutton), "toggled",
		     GTK_SIGNAL_FUNC(radio_button_direction_callback),
		     (gpointer) RIGHT);
  gtk_table_attach(GTK_TABLE(table), rbutton, 0, 1, 1, 2, GTK_FILL, 0, 0, 0);
  gtk_widget_show(rbutton);
  gtk_tooltips_set_tip(tooltips, rbutton, RIGHT_TEXT, NULL);

  gtk_widget_show(table);
  gtk_widget_show(frame);
  
  /*****************************************************
    radio buttons for choosing BOTH, LEADING, TRAILING
    ***************************************************/

  frame = gtk_frame_new("Edge affected");
  gtk_frame_set_shadow_type(GTK_FRAME(frame), GTK_SHADOW_ETCHED_IN);
  gtk_container_border_width(GTK_CONTAINER(frame), 0);
  gtk_table_attach(GTK_TABLE(outer_table), frame, 2, 3, 0, 3,
		   GTK_FILL | GTK_EXPAND, GTK_FILL | GTK_EXPAND, 0, 0);

  table = gtk_table_new(1, 3, FALSE);
  gtk_container_border_width(GTK_CONTAINER(table), 10);
  gtk_container_add(GTK_CONTAINER(frame), table);
  
  rbutton = gtk_radio_button_new_with_label(NULL, "Leading");
  list = gtk_radio_button_group((GtkRadioButton *) rbutton);
  gtk_toggle_button_set_active((GtkToggleButton *) rbutton,
			      config.edge == LEADING ? TRUE : FALSE);
  gtk_signal_connect(GTK_OBJECT(rbutton), "toggled",
		     GTK_SIGNAL_FUNC(radio_button_edge_callback),
		     (gpointer) LEADING);
  gtk_table_attach(GTK_TABLE(table), rbutton, 0, 1, 0, 1, GTK_FILL, 0, 0, 0);
  gtk_widget_show(rbutton);
  gtk_tooltips_set_tip(tooltips, rbutton, LEADING_TEXT, NULL);
  
  rbutton = gtk_radio_button_new_with_label(list, "Trailing");
  list = gtk_radio_button_group((GtkRadioButton *) rbutton);
  gtk_toggle_button_set_active((GtkToggleButton *) rbutton,
			      config.edge == TRAILING ? TRUE : FALSE);
  gtk_signal_connect(GTK_OBJECT(rbutton), "toggled",
		     GTK_SIGNAL_FUNC(radio_button_edge_callback),
		     (gpointer) TRAILING);
  gtk_table_attach(GTK_TABLE(table), rbutton, 0, 1, 1, 2, GTK_FILL, 0, 0, 0);
  gtk_widget_show(rbutton);
  gtk_tooltips_set_tip(tooltips, rbutton, TRAILING_TEXT, NULL);

  rbutton = gtk_radio_button_new_with_label(list, "Both");
  gtk_toggle_button_set_active((GtkToggleButton *) rbutton,
			      config.edge == BOTH ? TRUE : FALSE);
  gtk_signal_connect(GTK_OBJECT(rbutton), "toggled",
		     GTK_SIGNAL_FUNC(radio_button_edge_callback),
		     (gpointer) BOTH);
  gtk_table_attach(GTK_TABLE(table), rbutton, 0, 1, 2, 3, GTK_FILL, 0, 0, 0);
  gtk_widget_show(rbutton);
  gtk_tooltips_set_tip(tooltips, rbutton, BOTH_TEXT, NULL);
  
  gtk_widget_show(table);
  gtk_widget_show(frame);

  gtk_widget_show(outer_table);
  gtk_widget_show(outer_frame);
  gtk_widget_show(dlg);

  gtk_main();
  gdk_flush();

  return;
}

/*****/
