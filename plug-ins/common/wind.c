/*
 * wind 1.1.0 - a plug-in for the GIMP
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
 * Version: 1.0.0
 *
 * Version: 1.1.0
 * May 2000 tim copperfield [timecop@japan.co.jp]
 *
 * Added dynamic preview.
 *
 */

#include "config.h"

#include <stdio.h>
#include <stdlib.h>

#include <gtk/gtk.h>

#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

#include "libgimp/stdplugins-intl.h"


#define PLUG_IN_NAME "wind"

#define COMPARE_WIDTH   3

#define SCALE_WIDTH   200
#define MIN_THRESHOLD   0
#define MAX_THRESHOLD  50
#define MIN_STRENGTH    1
#define MAX_STRENGTH   50
#define PREVIEW_SIZE  128

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

static void dialog_box       (GDrawable   *drawable);
static void ok_callback      (GtkWidget   *widget, 
			      gpointer     data);
static void radio_callback   (GtkWidget   *widget, 
			      gpointer     data);

static gint render_effect    (GDrawable   *drawable, 
			      gboolean     preview_mode);
static void render_wind      (GDrawable   *drawable, 
			      gint         threshold, 
			      gint         strength,
			      direction_t  direction, 
			      edge_t       edge, 
			      gboolean     preview_mode);
static void render_blast     (GDrawable   *drawable, 
			      gint         threshold, 
			      gint         strength,
			      direction_t  direction, 
			      edge_t       edge, 
			      gboolean     preview_mode);
static gint render_blast_row (guchar      *buffer, 
			      gint         bytes, 
			      gint         lpi,
			      gint         threshold,
			      gint         strength, 
			      edge_t       edge);
static void render_wind_row  (guchar      *sb, 
			      gint         bytes, 
			      gint         lpi, 
			      gint         threshold,
			      gint         strength, 
			      edge_t       edge);


static void get_derivative     (guchar  *pixel_R1, 
				guchar  *pixel_R2,
				edge_t   edge, 
				gint    *derivative_R,
				gint    *derivative_G, 
				gint    *derivative_B);
static gint threshold_exceeded (guchar  *pixel_R1, 
				guchar  *pixel_R2,
				edge_t   edge, 
				gint     threshold);
static void reverse_buffer     (guchar  *buffer, 
				gint     length, 
				gint     bytes);

static void       fill_preview   (GtkWidget *preview_widget, 
				  GDrawable *drawable);
static GtkWidget *preview_widget (GDrawable *drawable);


GPlugInInfo PLUG_IN_INFO =
{
  NULL,	 /* init_proc  */
  NULL,	 /* quit_proc  */
  query, /* query_proc */
  run	 /* run_proc   */
};


/*********************
  Globals
  *******************/

/* This is needed to communicate the result from the dialog
   box button's callback function*/
static gint dialog_result = -1;

struct config_tag
{
  gint        threshold;     /* derivative comparison for edge detection */
  direction_t direction;     /* of wind, LEFT or RIGHT */
  gint        strength;	     /* how many pixels to bleed */
  algorithm_t alg;           /* which algorithm */
  edge_t      edge;          /* controls abs, ne+ static guchar *preview_bits;
+ static GtkWidget *preview;
gation of derivative */
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

static guchar    *preview_bits;
static GtkWidget *preview;


MAIN ()

static void
query (void)
{
  static GParamDef args[] =
  {
    { PARAM_INT32, "run_mode", "Interactive, non-interactive" },
    { PARAM_IMAGE, "image", "Input image (unused)" },
    { PARAM_DRAWABLE, "drawable", "Input drawable" },
    { PARAM_INT32, "threshold", "Controls where blending will be done >= 0" },
    { PARAM_INT32, "direction", "Left or Right: 0 or 1" },
    { PARAM_INT32, "strength", "Controls the extent of the blending > 1" },
    { PARAM_INT32, "alg", "WIND, BLAST" },
    { PARAM_INT32, "edge", "LEADING, TRAILING, or BOTH" }
  };
  static gint nargs = sizeof (args) / sizeof (args[0]);

  gimp_install_procedure ("plug_in_wind",
			  "Renders a wind effect.",
			  "Renders a wind effect.",
			  "Nigel Wetten",
			  "Nigel Wetten",
			  "May 2000",
			  N_("<Image>/Filters/Distorts/Wind..."),
			  "RGB*",
			  PROC_PLUG_IN,
			  nargs, 0,
			  args, NULL);
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
      INIT_I18N();
      if (nparams != 8)
	{
	  status = STATUS_CALLING_ERROR;
	}
      else
	{
	  config.threshold = param[3].data.d_int32;
	  config.direction = param[4].data.d_int32;
	  config.strength = param[5].data.d_int32;
	  config.alg = param[6].data.d_int32;
	  config.edge = param[7].data.d_int32;

	  if (render_effect(drawable, 0) == -1)
	    status = STATUS_EXECUTION_ERROR;
	}
      break;
      
    case RUN_INTERACTIVE:
      INIT_I18N_UI();
      gimp_get_data("plug_in_wind", &config);
      dialog_box(drawable);
      if (dialog_result == -1)
	{
	  status = STATUS_EXECUTION_ERROR;
	  break;
	}
      if (render_effect(drawable, 0) == -1)
	{
	  status = STATUS_CALLING_ERROR;
	  break;
	}
      g_free(preview_bits);
      gimp_set_data("plug_in_wind", &config, sizeof(config_t));
      gimp_displays_flush();
      break;
      
    case RUN_WITH_LAST_VALS:
      INIT_I18N();
      gimp_get_data("plug_in_wind", &config);
      if (render_effect (drawable, FALSE) == -1)
	{
	  status = STATUS_EXECUTION_ERROR;
	  gimp_message("An execution error occured.");
	}
      else
	{
	  gimp_displays_flush ();
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
render_effect (GDrawable *drawable, 
	       gboolean   preview_mode)
{
  if (config.alg == RENDER_WIND)
    {
      render_wind (drawable, config.threshold, config.strength,
		   config.direction, config.edge, preview_mode);
    }
  else if (config.alg == RENDER_BLAST)
    {
      render_blast (drawable, config.threshold, config.strength,
		    config.direction, config.edge, preview_mode);
    }
  return 0;
}

static void
render_blast (GDrawable   *drawable,
	      gint         threshold,
	      gint         strength,
	      direction_t  direction,
	      edge_t       edge,
	      gboolean     preview_mode)
{
  gint x1, x2, y1, y2;
  gint width;
  gint height;
  gint bytes = drawable->bpp;
  guchar *buffer;
  GPixelRgn src_region, dest_region;
  gint row;
  gint row_stride;
  gint marker = 0;
  gint lpi;
  
  if (preview_mode) 
    {
      width  = GTK_PREVIEW (preview)->buffer_width;
      height = GTK_PREVIEW (preview)->buffer_height;
      bytes  = GTK_PREVIEW (preview)->bpp;

      x1 = y1 = 0;
      x2 = width;
      y2 = height;
    } 
  else 
    {
      gimp_progress_init( _("Rendering Blast..."));
      gimp_drawable_mask_bounds (drawable->id, &x1, &y1, &x2, &y2);
      width = x2 - x1;
      height = y2 - y1;
      gimp_pixel_rgn_init (&src_region,  drawable, x1, y1, width, height, FALSE, FALSE);
      gimp_pixel_rgn_init (&dest_region, drawable, x1, y1, width, height, TRUE, TRUE);
  }

  row_stride = width * bytes;
  lpi = row_stride - bytes;

  buffer = (guchar *) g_malloc(row_stride);
  
  for (row = y1; row < y2; row++)
    {
      if (preview_mode)
        memcpy (buffer, preview_bits + (row * bytes * width), width * bytes);
      else
        gimp_pixel_rgn_get_row (&src_region, buffer, x1, row, width);

      if (direction == RIGHT)
	{
	  reverse_buffer (buffer, row_stride, bytes);
	}

      marker = render_blast_row (buffer, bytes, lpi, threshold, strength, edge);

      if (direction == RIGHT)
	{
	  reverse_buffer (buffer, row_stride, bytes);
	}

      if (preview_mode) 
	{
	  memcpy (GTK_PREVIEW (preview)->buffer + (width * bytes * row), buffer, width * bytes);
	} 
      else 
	{
	  gimp_pixel_rgn_set_row (&dest_region, buffer, x1, row, width);
	  gimp_progress_update ((double) (row - y1)/ (double) (height));
	}

      if (marker)
	{
	  gint j, limit;
	  
	  limit = 1 + rand () % 2;
	  for (j = 0; (j < limit) && (row < y2); j++)
	    {
	      row++;
	      if (row < y2)
		{
                  if (preview_mode) 
		    {
		      memcpy (buffer, preview_bits + (row * bytes * width), width * bytes);
		      memcpy (GTK_PREVIEW (preview)->buffer + (width * bytes * row),
			      buffer, width * bytes);
		    } 
		  else 
		    {
		      gimp_pixel_rgn_get_row (&src_region, buffer, x1, row, width);
		      gimp_pixel_rgn_set_row (&dest_region, buffer, x1, row, width);
		    }
		}
	    }
	  marker = 0;
	}
    }

  g_free(buffer);
  
  /*  update the region  */
  if (preview_mode) 
    {
      gtk_widget_queue_draw (preview);
    }
  else 
    {
      gimp_drawable_flush (drawable);
      gimp_drawable_merge_shadow (drawable->id, TRUE);
      gimp_drawable_update (drawable->id, x1, y1, x2 - x1, y2 - y1);
    }

  return;
}

static void
render_wind (GDrawable   *drawable,
	     gint         threshold,
	     gint         strength,
	     direction_t  direction,
	     edge_t       edge,
	     gboolean     preview_mode)
{
  GPixelRgn src_region, dest_region;
  gint width;
  gint height;
  gint bytes;
  gint row_stride;
  gint comp_stride;
  gint row;
  guchar *sb;
  gint lpi;
  gint x1, y1, x2, y2;

  if (preview_mode) 
    {
      width  = GTK_PREVIEW (preview)->buffer_width;
      height = GTK_PREVIEW (preview)->buffer_height;
      bytes  = GTK_PREVIEW (preview)->bpp;

      x1 = y1 = 0;
      x2 = width;
      y2 = height;
    } 
  else 
    {
      gimp_progress_init( _("Rendering Wind..."));
      gimp_drawable_mask_bounds (drawable->id, &x1, &y1, &x2, &y2);
      bytes = drawable->bpp;
      width = x2 - x1;
      height = y2 - y1;
      gimp_pixel_rgn_init (&src_region, drawable, x1, y1, width, height, FALSE, FALSE);
      gimp_pixel_rgn_init (&dest_region, drawable, x1, y1, width, height, TRUE, TRUE);
    }
  
  row_stride = width * bytes;
  comp_stride = bytes * COMPARE_WIDTH;
  lpi = row_stride - comp_stride;

  sb = g_malloc (row_stride);
  
  for (row = y1; row < y2; row++)
    {
      if (preview_mode) 
	memcpy (sb, preview_bits + (row * bytes * width), width * bytes);
      else 
	gimp_pixel_rgn_get_row (&src_region, sb, x1, row, width);

      if (direction == RIGHT)
	reverse_buffer (sb, row_stride, bytes);
      
      render_wind_row (sb, bytes, lpi, threshold, strength, edge);

      if (direction == RIGHT)
	reverse_buffer(sb, row_stride, bytes);

      if (preview_mode) 
	{
	  memcpy (GTK_PREVIEW (preview)->buffer + (width * bytes * row), sb, width * bytes);
	} 
      else 
	{
	  gimp_pixel_rgn_set_row (&dest_region, sb, x1, row, width);
	  gimp_progress_update ((double) (row - y1)/ (double) (height));
	}
    }

  g_free(sb);

  /*  update the region  */
  if (preview_mode) 
    {
      gtk_widget_queue_draw (preview);
    } 
  else 
    {
      gimp_drawable_flush (drawable);
      gimp_drawable_merge_shadow (drawable->id, TRUE);
      gimp_drawable_update (drawable->id, x1, y1, x2 - x1, y2 - y1);
    }

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
	    + (gint) (bleed_length_max * rand() / (G_MAXRAND + 1.0));

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
ok_callback (GtkWidget *widget,
	     gpointer   data)
{
  /* we have to stop the dialog from being closed with strength < 1 */
  /* since we use spinbuttons this should never happen ...          */
  if (config.strength < 1)
    {
      g_message (_("Wind Strength must be greater than 0."));
    }
  else
    {
      dialog_result = 1;
      gtk_widget_destroy (GTK_WIDGET (data));
    }
}

static void
radio_callback (GtkWidget *widget, 
		gpointer   data)
{
  GDrawable *drawable;

  gimp_radio_button_update (widget, data);
  drawable = gtk_object_get_data (GTK_OBJECT (widget), "drawable");
  render_effect (drawable, TRUE);
}

static void
dialog_box (GDrawable *drawable)
{
  GtkWidget *main_vbox;
  GtkWidget *vbox;
  GtkWidget *abox;
  GtkWidget *table;
  GtkObject *adj;
  GtkWidget *frame;
  GtkWidget *dlg;
  GtkWidget *style1;
  GtkWidget *style2;
  GtkWidget *dir1;
  GtkWidget *dir2;
  GtkWidget *edge1;
  GtkWidget *edge2;
  GtkWidget *edge3;

  gimp_ui_init ("wind", FALSE);

  dlg = gimp_dialog_new ( _("Wind"), "wind",
			 gimp_standard_help_func, "filters/wind.html",
			 GTK_WIN_POS_MOUSE,
			 FALSE, TRUE, FALSE,

			 _("OK"), ok_callback,
			 NULL, NULL, NULL, TRUE, FALSE,
			 _("Cancel"), gtk_widget_destroy,
			 NULL, 1, NULL, FALSE, TRUE,

			 NULL);

  gtk_signal_connect (GTK_OBJECT (dlg), "destroy",
		      GTK_SIGNAL_FUNC (gtk_main_quit),
		      NULL);

  /* init tooltips */
  gimp_help_init ();
  
  vbox = gtk_vbox_new (FALSE, 2);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), 0);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dlg)->vbox), vbox, TRUE, TRUE, 0);
  gtk_widget_show (vbox);

  frame = gtk_frame_new (_("Preview"));
  gtk_container_set_border_width (GTK_CONTAINER (frame), 6);
  gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);
  abox = gtk_alignment_new (0.5, 0.5, 0.0, 0.0);
  gtk_container_set_border_width (GTK_CONTAINER (abox), 4);
  gtk_container_add (GTK_CONTAINER (frame), abox);
  gtk_widget_show (abox);
  frame = gtk_frame_new (NULL);
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_IN);
  gtk_container_add (GTK_CONTAINER (abox), frame);
  gtk_widget_show (frame);
  preview = preview_widget (drawable); /* we are here */
  gtk_container_add (GTK_CONTAINER (frame), preview);
  render_effect (drawable, TRUE); /* render preview image */
  gtk_widget_show (preview);

  frame = gtk_frame_new (_("Parameter Settings"));
  gtk_container_set_border_width (GTK_CONTAINER (frame), 6);
  gtk_box_pack_start (GTK_BOX (vbox), frame, TRUE, TRUE, 0);
  gtk_widget_show (frame);
 
  main_vbox = gtk_vbox_new (FALSE, 4);
  gtk_container_set_border_width (GTK_CONTAINER (main_vbox), 6);
  gtk_container_add (GTK_CONTAINER (frame), main_vbox);
  gtk_widget_show (main_vbox);

  /*****************************************************
    outer frame and table
  ***************************************************/

  table = gtk_table_new (1, 3, FALSE);
  gtk_table_set_col_spacings (GTK_TABLE (table), 4);
  gtk_table_set_row_spacings (GTK_TABLE (table), 2);
  gtk_box_pack_start (GTK_BOX (main_vbox), table, FALSE, FALSE, 0);

  /*********************************************************
    radio buttons for choosing wind rendering algorithm
    ******************************************************/

  frame = gimp_radio_group_new2 (TRUE, _("Style"),
				 radio_callback,
				 &config.alg, (gpointer) config.alg,
				 _("Wind"),  (gpointer) RENDER_WIND,  &style1,
				 _("Blast"), (gpointer) RENDER_BLAST, &style2,

				 NULL);
  gtk_object_set_data (GTK_OBJECT (style1), "drawable", drawable);
  gtk_object_set_data (GTK_OBJECT (style2), "drawable", drawable);

  gtk_table_attach (GTK_TABLE (table), frame, 0, 1, 0, 1,
		    GTK_FILL | GTK_EXPAND, GTK_FILL | GTK_EXPAND, 0, 0);
  gtk_widget_show (frame);

  /******************************************************
    radio buttons for choosing LEFT or RIGHT
    **************************************************/

  frame = gimp_radio_group_new2 (TRUE, _("Direction"),
				 radio_callback,
				 &config.direction, (gpointer) config.direction,
				 _("Left"),  (gpointer) LEFT,  &dir1,
				 _("Right"), (gpointer) RIGHT, &dir2,
				 NULL);
  gtk_object_set_data (GTK_OBJECT (dir1), "drawable", drawable);
  gtk_object_set_data (GTK_OBJECT (dir2), "drawable", drawable);

  gtk_table_attach (GTK_TABLE (table), frame, 1, 2, 0, 1,
		    GTK_FILL | GTK_EXPAND, GTK_FILL | GTK_EXPAND, 0, 0);
  gtk_widget_show (frame);
  
  /*****************************************************
    radio buttons for choosing BOTH, LEADING, TRAILING
    ***************************************************/

  frame = gimp_radio_group_new2 (TRUE, _("Edge Affected"),
				 radio_callback,
				 &config.edge, (gpointer) config.edge,

				 _("Leading"),  (gpointer) LEADING,  &edge1,
				 _("Trailing"), (gpointer) TRAILING, &edge2,
				 _("Both"),     (gpointer) BOTH,     &edge3,

				 NULL);
  gtk_object_set_data (GTK_OBJECT (edge1), "drawable", drawable);
  gtk_object_set_data (GTK_OBJECT (edge2), "drawable", drawable);
  gtk_object_set_data (GTK_OBJECT (edge3), "drawable", drawable);

  gtk_table_attach (GTK_TABLE (table), frame, 2, 3, 0, 1,
		    GTK_FILL | GTK_EXPAND, GTK_FILL | GTK_EXPAND, 0, 0);
  gtk_widget_show (frame);

  gtk_widget_show (table);

  /****************************************************
   table for sliders
   ****************************************************/
  table = gtk_table_new (2, 3, FALSE);
  gtk_table_set_col_spacings (GTK_TABLE (table), 4);
  gtk_table_set_row_spacings (GTK_TABLE (table), 2);
  gtk_box_pack_start (GTK_BOX (main_vbox), table, FALSE, FALSE, 0);
 
  /*****************************************************
    slider and entry for threshold
    ***************************************************/

  adj = gimp_scale_entry_new (GTK_TABLE (table), 0, 0,
			      _("Threshold:"), SCALE_WIDTH, 0,
			      config.threshold,
			      MIN_THRESHOLD, MAX_THRESHOLD, 1.0, 10, 0,
			      TRUE, 0, 0,
			      _("Higher values restrict the effect to fewer areas of the image"), NULL);
  gtk_signal_connect (GTK_OBJECT (adj), "value_changed",
		      GTK_SIGNAL_FUNC (gimp_int_adjustment_update),
		      &config.threshold);
  gtk_signal_connect_object (GTK_OBJECT (adj), "value_changed",
			     GTK_SIGNAL_FUNC (render_effect),
			     (gpointer)drawable);

  /*****************************************************
    slider and entry for strength of wind
    ****************************************************/

  adj = gimp_scale_entry_new (GTK_TABLE (table), 0, 1,
			      _("Strength:"), SCALE_WIDTH, 0,
			      config.strength,
			      MIN_STRENGTH, MAX_STRENGTH, 1.0, 10.0, 0,
			      TRUE, 0, 0,
			      _("Higher values increase the magnitude of the effect"), NULL);

  gtk_signal_connect (GTK_OBJECT (adj), "value_changed",
		      GTK_SIGNAL_FUNC (gimp_int_adjustment_update),
		      &config.strength);
  gtk_signal_connect_object (GTK_OBJECT (adj), "value_changed",
			     GTK_SIGNAL_FUNC (render_effect),
			     (gpointer)drawable);

  gtk_widget_show (table);

  gtk_widget_show (dlg);

  gtk_main ();
  gdk_flush ();
}


static GtkWidget *
preview_widget (GDrawable *drawable)
{
  gint       size;
  GtkWidget *preview;

  preview = gtk_preview_new (GTK_PREVIEW_COLOR);
  fill_preview (preview, drawable);
  size = (GTK_PREVIEW (preview)->buffer_width) * 
	 (GTK_PREVIEW (preview)->buffer_height) * 
	 (GTK_PREVIEW (preview)->bpp);
  preview_bits = g_malloc (size);
  memcpy (preview_bits, GTK_PREVIEW (preview)->buffer, size);

  return preview;
}

static void
fill_preview (GtkWidget *widget, 
	      GDrawable *drawable)
{
  GPixelRgn  srcPR;
  gint       width;
  gint       height;
  gint       x1, x2, y1, y2;
  gint       bpp;
  gint       x, y;
  guchar    *src;
  gdouble    r, g, b, a;
  gdouble    c0, c1;
  guchar    *p0, *p1;
  guchar    *even, *odd;
  
  gimp_drawable_mask_bounds (drawable->id, &x1, &y1, &x2, &y2);

  if (x2 - x1 > PREVIEW_SIZE)
    x2 = x1 + PREVIEW_SIZE;
  
  if (y2 - y1 > PREVIEW_SIZE)
    y2 = y1 + PREVIEW_SIZE;
  
  width  = x2 - x1;
  height = y2 - y1;
  bpp    = gimp_drawable_bpp (drawable->id);
  
  if (width < 1 || height < 1)
    return;

  gtk_preview_size (GTK_PREVIEW (widget), width, height);

  gimp_pixel_rgn_init (&srcPR, drawable, x1, y1, x2, y2, FALSE, FALSE);

  even = g_malloc (width * 3);
  odd  = g_malloc (width * 3);
  src  = g_malloc (width * bpp);

  for (y = 0; y < height; y++)
    {
      gimp_pixel_rgn_get_row (&srcPR, src, x1, y + y1, width);
      p0 = even;
      p1 = odd;
      
      for (x = 0; x < width; x++) 
	{
	  if (bpp == 4)
	    {
	      r = ((gdouble)src[x*4+0]) / 255.0;
	      g = ((gdouble)src[x*4+1]) / 255.0;
	      b = ((gdouble)src[x*4+2]) / 255.0;
	      a = ((gdouble)src[x*4+3]) / 255.0;
	    }
	  else if (bpp == 3)
	    {
	      r = ((gdouble)src[x*3+0]) / 255.0;
	      g = ((gdouble)src[x*3+1]) / 255.0;
	      b = ((gdouble)src[x*3+2]) / 255.0;
	      a = 1.0;
	    }
	  else
	    {
	      r = ((gdouble)src[x*bpp+0]) / 255.0;
	      g = b = r;
	      if (bpp == 2)
		a = ((gdouble)src[x*bpp+1]) / 255.0;
	      else
		a = 1.0;
	    }
	
	if ((x / GIMP_CHECK_SIZE) & 1) 
	  {
	    c0 = GIMP_CHECK_LIGHT;
	    c1 = GIMP_CHECK_DARK;
	  } 
	else 
	  {
	    c0 = GIMP_CHECK_DARK;
	    c1 = GIMP_CHECK_LIGHT;
	  }
	
	*p0++ = (c0 + (r - c0) * a) * 255.0;
	*p0++ = (c0 + (g - c0) * a) * 255.0;
	*p0++ = (c0 + (b - c0) * a) * 255.0;
	
	*p1++ = (c1 + (r - c1) * a) * 255.0;
	*p1++ = (c1 + (g - c1) * a) * 255.0;
	*p1++ = (c1 + (b - c1) * a) * 255.0;
	
      } /* for */
      
      if ((y / GIMP_CHECK_SIZE) & 1)
	gtk_preview_draw_row (GTK_PREVIEW (widget), (guchar *)odd,  0, y, width);
      else
	gtk_preview_draw_row (GTK_PREVIEW (widget), (guchar *)even, 0, y, width);
    }

  g_free (even);
  g_free (odd);
  g_free (src);
}
