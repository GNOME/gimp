/* scatter_hsv.c -- This is a plug-in for the GIMP (1.0's API)
 * Author: Shuji Narazaki <narazaki@InetQ.or.jp>
 * Time-stamp: <2000-01-08 02:49:39 yasuhiro>
 * Version: 0.42
 *
 * Copyright (C) 1997 Shuji Narazaki <narazaki@InetQ.or.jp>
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

#include "config.h"
#include "gtk/gtk.h"
#include "libgimp/gimp.h"
#include "libgimp/gimpcolorspace.h"
#include "libgimp/stdplugins-intl.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>		/* for seed of random number */

#ifndef RAND_MAX
#define RAND_MAX 2147483647
#endif /* RAND_MAX */

#define	PLUG_IN_NAME	"plug_in_scatter_hsv"
#define SHORT_NAME	"scatter_hsv"
#define	MAIN_FUNCTION	scatter_hsv
#define INTERFACE	scatter_hsv_interface
#define	DIALOG		scatter_hsv_dialog
#define ERROR_DIALOG	scatter_hsv_error_dialog
#define VALS		scatter_hsv_vals
#define OK_CALLBACK	scatter_hsv_ok_callback

static void	query	(void);
static void	run	(char	*name,
			 int	nparams,
			 GParam	*param,
			 int	*nreturn_vals,
			 GParam **return_vals);
static GStatusType	MAIN_FUNCTION (gint32 drawable_id);
void scatter_hsv_scatter (guchar *r, guchar *g, guchar *b);
static int randomize_value (int	now,
			    int min,
			    int max,
			    int mod_p,
			    int	rand_max);

static gint	DIALOG ();
static void	ERROR_DIALOG (gint gtk_was_not_initialized, guchar *message);
static void	OK_CALLBACK (GtkWidget *widget, gpointer   data);
static gint	preview_event_handler (GtkWidget *widget, GdkEvent *event);
static void	scatter_hsv_preview_update ();
/* gtkWrapper functions */ 
#define PROGRESS_UPDATE_NUM	100
#define PREVIEW_WIDTH	128
gint	preview_width = PREVIEW_WIDTH;
#define PREVIEW_HEIGHT	128
gint	preview_height = PREVIEW_HEIGHT;
#define ENTRY_WIDTH	100
#define SCALE_WIDTH	100
static void
gtkW_close_callback (GtkWidget *widget, gpointer   data);
static GtkWidget *
gtkW_dialog_new (char *name,
		 GtkSignalFunc ok_callback,
		 GtkSignalFunc close_callback);
static GtkWidget *
gtkW_error_dialog_new (char * name);
static void
gtkW_table_add_scale_entry (GtkWidget	*table,
			    gchar	*name,
			    gint	x,
			    gint	y,
			    GtkSignalFunc	scale_update,
			    GtkSignalFunc	entry_update,
			    gint	*value,
			    gdouble	min,
			    gdouble	max,
			    gdouble	step,
			    gchar	*buffer);

GtkWidget *gtkW_check_button_new (GtkWidget	*parent,
				  gchar	*name,
				  GtkSignalFunc update,
				  gint	*value);
GtkWidget *gtkW_frame_new (GtkWidget *parent, gchar *name);
GtkWidget *gtkW_table_new (GtkWidget *parent, gint col, gint row);
GtkWidget *gtkW_hbox_new (GtkWidget *parent);
GtkWidget *gtkW_vbox_new (GtkWidget *parent);
static void scatter_hsv_iscale_update (GtkAdjustment *adjustment,
				       gpointer       data);
static void scatter_hsv_ientry_update (GtkWidget *widget,
				gpointer   data);


GPlugInInfo PLUG_IN_INFO =
{
  NULL,				/* init_proc  */
  NULL,				/* quit_proc */
  query,			/* query_proc */
  run,				/* run_proc */
};

typedef struct
{				/* gint, gdouble, and so on */
  gint	holdness;
  gint	hue_distance;
  gint	saturation_distance;
  gint	value_distance;
} ValueType;

static ValueType VALS = 
{
  2, 3, 10, 10
};

typedef struct 
{
  gint run;
} Interface;

static Interface INTERFACE = { FALSE };
gint	drawable_id;

GtkWidget	*preview;
gint	preview_start_x = 0;
gint	preview_end_x = 0;
gint	preview_start_y = 0;
gint	preview_end_y = 0;
guchar	*preview_buffer = NULL;
gint	preview_offset_x = 0;
gint	preview_offset_y = 0;
gint	preview_dragging = FALSE;
gint	preview_drag_start_x = 0;
gint	preview_drag_start_y = 0;

MAIN ()

static void
query ()
{
  static GParamDef args [] =
  {
    { PARAM_INT32, "run_mode", "Interactive, non-interactive"},
    { PARAM_IMAGE, "image", "Input image (not used)"},
    { PARAM_DRAWABLE, "drawable", "Input drawable"},
    { PARAM_INT32, "holdness", "convolution strength"},
    { PARAM_INT32, "hue_distance", "distribution distance on hue axis [0,255]"},
    { PARAM_INT32, "saturation_distance", "distribution distance on saturation axis [0,255]"},
    { PARAM_INT32, "value_distance", "distribution distance on value axis [0,255]"}
  };
  static GParamDef *return_vals = NULL;
  static int nargs = sizeof (args) / sizeof (args[0]);
  static int nreturn_vals = 0;

  INIT_I18N();
  
  gimp_install_procedure (PLUG_IN_NAME,
			  _("Scattering pixel values in HSV space"),
			  _("Scattering pixel values in HSV space"),
			  "Shuji Narazaki (narazaki@InetQ.or.jp)",
			  "Shuji Narazaki",
			  "1997",
              N_("<Image>/Filters/Noise/Scatter HSV..."),
			  "RGB*",
			  PROC_PLUG_IN,
			  nargs, nreturn_vals,
			  args, return_vals);
}

static void
run (char	*name,
     int	nparams,
     GParam	*param,
     int	*nreturn_vals,
     GParam	**return_vals)
{
  static GParam	 values[1];
  GStatusType	status = STATUS_EXECUTION_ERROR;
  GRunModeType	run_mode;
  
  run_mode = param[0].data.d_int32;
  drawable_id = param[2].data.d_int32;

  *nreturn_vals = 1;
  *return_vals = values;
  
  values[0].type = PARAM_STATUS;
  values[0].data.d_status = status;

  switch ( run_mode )
    {
    case RUN_INTERACTIVE:
      INIT_I18N_UI();
      gimp_get_data (PLUG_IN_NAME, &VALS);
      if (!gimp_drawable_is_rgb(drawable_id))
	{
	  scatter_hsv_error_dialog (1, (guchar *)"RGB drawable is not selected.");
	  return;
	}
      if (! DIALOG ())
	return;
      break;
    case RUN_NONINTERACTIVE:
      INIT_I18N();
      VALS.holdness = param[3].data.d_int32;
      VALS.hue_distance = param[4].data.d_int32;
      VALS.saturation_distance = param[5].data.d_int32;
      VALS.value_distance = param[6].data.d_int32;
      break;
    case RUN_WITH_LAST_VALS:
      INIT_I18N();
      gimp_get_data (PLUG_IN_NAME, &VALS);
      break;
    }
  
  status = MAIN_FUNCTION (drawable_id);

  if (run_mode != RUN_NONINTERACTIVE)
    gimp_displays_flush();
  if (run_mode == RUN_INTERACTIVE && status == STATUS_SUCCESS )
    gimp_set_data (PLUG_IN_NAME, &VALS, sizeof (ValueType));

  values[0].type = PARAM_STATUS;
  values[0].data.d_status = status;
}

static GStatusType
MAIN_FUNCTION (gint32	drawable_id)
{
  GDrawable	*drawable;
  GPixelRgn	src_rgn, dest_rgn;
  guchar	*src, *dest;
  gpointer	pr;
  gint		x, y, x1, x2, y1, y2;
  gint		gap, total, processed = 0;
  
  drawable = gimp_drawable_get (drawable_id);
  gap = (gimp_drawable_has_alpha (drawable_id)) ? 1 : 0;
  gimp_drawable_mask_bounds (drawable_id, &x1, &y1, &x2, &y2);
  total = (x2 - x1) * (y2 - y1);

  gimp_tile_cache_ntiles (2 * (drawable->width / gimp_tile_width () + 1));
  gimp_pixel_rgn_init (&src_rgn, drawable, x1, y1, (x2 - x1), (y2 - y1), FALSE, FALSE);
  gimp_pixel_rgn_init (&dest_rgn, drawable, x1, y1, (x2 - x1), (y2 - y1), TRUE, TRUE);

  gimp_progress_init ( _("scatter_hsv: scattering..."));
  srand (time (NULL));
  pr = gimp_pixel_rgns_register (2, &src_rgn, &dest_rgn);
  
  for (; pr != NULL; pr = gimp_pixel_rgns_process (pr))
    {
      int offset;

      for (y = 0; y < src_rgn.h; y++)
	{
	  src = src_rgn.data + y * src_rgn.rowstride;
	  dest = dest_rgn.data + y * dest_rgn.rowstride;
	  offset = 0;

	  for (x = 0; x < src_rgn.w; x++)
	    {
	      guchar	h, s, v;

	      h = *(src + offset);
	      s = *(src + offset + 1);
	      v = *(src + offset + 2);
	      
	      scatter_hsv_scatter (&h, &s, &v);

	      *(dest + offset    ) = (guchar) h;
	      *(dest + offset + 1) = (guchar) s;
	      *(dest + offset + 2) = (guchar) v;

	      offset += 3;
	      if (gap)
		{
		  *(dest + offset) = *(src + offset);
		  offset++;
		}
	      /* the function */
	      if ((++processed % (total / PROGRESS_UPDATE_NUM)) == 0)
		gimp_progress_update ((double)processed /(double) total); 
	    }
	}
  }
  gimp_progress_update (1.0);
  gimp_drawable_flush (drawable);
  gimp_drawable_merge_shadow (drawable->id, TRUE);
  gimp_drawable_update (drawable->id, x1, y1, (x2 - x1), (y2 - y1));
  gimp_drawable_detach (drawable);
  return STATUS_SUCCESS;
}

static int randomize_value (int	now,
			    int min,
			    int max,
			    int mod_p,
			    int	rand_max)
{
  int	flag, new, steps, index;
  double rand_val;
  
  steps = max - min + 1;
  rand_val = ((double) rand()/(double)RAND_MAX);
  for (index = 1; index < VALS.holdness; index++)
    {
      double tmp = ((double) rand()/(double)RAND_MAX);
      if (tmp < rand_val)
	rand_val = tmp;
    }
  
  flag = ((RAND_MAX / 2) < rand()) ? 1 : -1;
  new = now + flag * ((int) (rand_max * rand_val) % steps);
  
  if (new < min)
    {
      if (mod_p == 1)
	new += steps;
      else
	new = min;
    }
  if (max < new)
    {
      if (mod_p == 1)
	new -= steps;
      else
	new = max;
    }
  return new;
}

void scatter_hsv_scatter (guchar *r, guchar *g, guchar *b)
{
  int	h, s, v;
  int	h1, s1, v1;
  int	h2, s2, v2;
  
  h = *r; s = *g; v = *b;
  
  gimp_rgb_to_hsv (&h, &s, &v);

  if (0 < VALS.hue_distance)
    h = randomize_value (h, 0, 255, 1, VALS.hue_distance);
  if ((0 < VALS.saturation_distance))
    s = randomize_value (s, 0, 255, 0, VALS.saturation_distance);
  if ((0 < VALS.value_distance))
    v = randomize_value (v, 0, 255, 0, VALS.value_distance);

  h1 = h; s1 = s; v1 = v;
	      
  gimp_hsv_to_rgb (&h, &s, &v); /* don't believe ! */

  h2 = h; s2 = s; v2 = v;

  gimp_rgb_to_hsv (&h2, &s2, &v2); /* h2 should be h1. But... */
  
  if ((abs (h1 - h2) <= VALS.hue_distance)
      && (abs (s1 - s2) <= VALS.saturation_distance)
      && (abs (v1 - v2) <= VALS.value_distance))
    {
      *r = h;
      *g = s;
      *b = v;
    }
}

/* dialog stuff */
static int
DIALOG ()
{
  GtkWidget	*dlg;
  GtkWidget	*vbox;
  GtkWidget	*frame;
  GtkWidget	*table;
  gchar	**argv;
  gint	argc;
  gint	index = 0;
  static gchar	buffer[4][10];

  argc = 1;
  argv = g_new (gchar *, 1);
  argv[0] = g_strdup (PLUG_IN_NAME);
  gtk_init (&argc, &argv);
  gtk_rc_parse (gimp_gtkrc ());
  
  dlg = gtkW_dialog_new (PLUG_IN_NAME,
			 (GtkSignalFunc) OK_CALLBACK,
			 (GtkSignalFunc) gtkW_close_callback);
  
  vbox = gtkW_vbox_new ((GTK_DIALOG (dlg)->vbox));
  frame = gtkW_frame_new (vbox, _("Parameter Settings"));
  table = gtkW_table_new (frame, 5, 2);

  gtkW_table_add_scale_entry (table, _("Holdness"), 0, index,
			      (GtkSignalFunc) scatter_hsv_iscale_update,
			      (GtkSignalFunc) scatter_hsv_ientry_update,
			      &VALS.holdness,
			      1, 8, 1, buffer[index]);
  index++;
  gtkW_table_add_scale_entry (table, _("Hue"), 0, index,
			      (GtkSignalFunc) scatter_hsv_iscale_update,
			      (GtkSignalFunc) scatter_hsv_ientry_update,
			      &VALS.hue_distance, 
			      0, 255, 1, buffer[index]);
  index++;
  gtkW_table_add_scale_entry (table, _("Saturation"), 0, index,
			      (GtkSignalFunc) scatter_hsv_iscale_update,
			      (GtkSignalFunc) scatter_hsv_ientry_update,
			      &VALS.saturation_distance,
			      0, 255, 1, buffer[index]);
  index++;
  gtkW_table_add_scale_entry (table, _("Value"), 0, index,
			      (GtkSignalFunc) scatter_hsv_iscale_update,
			      (GtkSignalFunc) scatter_hsv_ientry_update,
			      &VALS.value_distance,
			      0, 255,1, buffer[index]);
  gtk_widget_show (table);
  gtk_widget_show (frame);

  frame = gtkW_frame_new (vbox, _("Preview (1:4) - right click to jump"));

  /* preparation for preview */	
  gdk_set_use_xshm (gimp_use_xshm ());
  gtk_preview_set_gamma (gimp_gamma ());
  gtk_preview_set_install_cmap (gimp_install_cmap ());
  {
    guchar *color_cube;
    color_cube = gimp_color_cube ();
    gtk_preview_set_color_cube (color_cube[0], color_cube[1],
				color_cube[2], color_cube[3]);
  }
  gtk_widget_set_default_visual (gtk_preview_get_visual ());
  gtk_widget_set_default_colormap (gtk_preview_get_cmap ());

  preview = gtk_preview_new (GTK_PREVIEW_COLOR);
  {
    gint width = gimp_drawable_width (drawable_id);
    gint height = gimp_drawable_height (drawable_id);

    preview_width = (PREVIEW_WIDTH < width) ? PREVIEW_WIDTH : width;
    preview_height = (PREVIEW_HEIGHT < height) ? PREVIEW_HEIGHT : height;
  }
  gtk_preview_size (GTK_PREVIEW (preview), preview_width * 2, preview_height);
  scatter_hsv_preview_update ();
  gtk_container_add (GTK_CONTAINER (frame), preview);
  gtk_widget_set_events (preview, 
			 GDK_BUTTON_PRESS_MASK |
			 GDK_BUTTON_RELEASE_MASK | 
			 GDK_BUTTON_MOTION_MASK |
			 GDK_POINTER_MOTION_HINT_MASK);
  gtk_signal_connect (GTK_OBJECT (preview), "event",
		      (GtkSignalFunc) preview_event_handler,
		      NULL);
  gtk_widget_show (preview);

  gtk_widget_show (frame);
  gtk_widget_show (vbox);
  gtk_widget_show (dlg);
  
  gtk_main ();
  gdk_flush ();

  return INTERFACE.run;
}

static void
ERROR_DIALOG (gint gtk_was_not_initialized, guchar *message)
{
  GtkWidget *dlg;
  GtkWidget *table;
  GtkWidget *label;
  gchar	**argv;
  gint	argc;

  if (gtk_was_not_initialized)
    {
      argc = 1;
      argv = g_new (gchar *, 1);
      argv[0] = g_strdup (PLUG_IN_NAME);
      gtk_init (&argc, &argv);
      gtk_rc_parse (gimp_gtkrc ());
    }
  
  dlg = gtkW_error_dialog_new (PLUG_IN_NAME);
  
  table = gtk_table_new (1,1, FALSE);
  gtk_container_border_width (GTK_CONTAINER (table), 10);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dlg)->vbox), table, TRUE, TRUE, 0);

  label = gtk_label_new ((char *)message);
  gtk_table_attach (GTK_TABLE (table), label, 0, 1, 0, 1, GTK_FILL|GTK_EXPAND,
		    0, 0, 0);

  gtk_widget_show (label);
  gtk_widget_show (table);
  gtk_widget_show (dlg);
  
  gtk_main ();
  gdk_flush ();
}

static gint
preview_event_handler (GtkWidget *widget, GdkEvent *event)
{
  gint            x, y;
  gint            dx, dy;
  GdkEventButton *bevent;
	
  gtk_widget_get_pointer (widget, &x, &y);

  bevent = (GdkEventButton *) event;

  switch (event->type) 
    {
    case GDK_BUTTON_PRESS:
      if (x < preview_width)
	{
	  if (bevent->button == 3)
	    {
	      preview_offset_x = - x;
	      preview_offset_y = - y;
	      scatter_hsv_preview_update ();
	    }
	  else
	    {
	      preview_dragging = TRUE;
	      preview_drag_start_x = x;
	      preview_drag_start_y = y;
	      gtk_grab_add (widget);
	    }
	}
      break;
    case GDK_BUTTON_RELEASE:
      if (preview_dragging)
	{
	  gtk_grab_remove (widget);
	  preview_dragging = FALSE;
	  scatter_hsv_preview_update ();
	}
      break;
    case GDK_MOTION_NOTIFY:
      if (preview_dragging)
	{
	  dx = x - preview_drag_start_x;
	  dy = y - preview_drag_start_y;

	  preview_drag_start_x = x;
	  preview_drag_start_y = y;

	  if ((dx == 0) && (dy == 0))
	    break;

	  preview_offset_x = MAX (preview_offset_x - dx, 0);
	  preview_offset_y = MAX (preview_offset_y - dy, 0);
	  scatter_hsv_preview_update ();
	}
      break; 
    default:
      break;
    }
  return FALSE;
}

static void
scatter_hsv_preview_update ()
{
  GDrawable	*drawable;
  GPixelRgn	src_rgn;
  gint	scale;
  gint	x, y, dx, dy;
  gint	bound_start_x, bound_start_y, bound_end_x, bound_end_y;
  gint	src_has_alpha = FALSE;
  gint	src_is_gray = FALSE;
  gint	src_bpp, src_bpl;
  guchar	data[3];
  gdouble	shift_rate;
    
  drawable = gimp_drawable_get (drawable_id);
  gimp_drawable_mask_bounds (drawable_id,
			     &bound_start_x, &bound_start_y,
			     &bound_end_x, &bound_end_y);
  src_has_alpha  = gimp_drawable_has_alpha (drawable_id);
  src_is_gray =  gimp_drawable_is_gray (drawable_id);
  src_bpp = (src_is_gray ? 1 : 3) + (src_has_alpha ? 1 : 0);
  src_bpl = preview_width * src_bpp;

  if (! preview_buffer)
    preview_buffer
      = (guchar *) g_malloc (src_bpl * preview_height * sizeof (guchar));

  if (preview_offset_x < 0)
    preview_offset_x = (bound_end_x - bound_start_x) * (- preview_offset_x) /  preview_width;
  if (preview_offset_y < 0)
    preview_offset_y = (bound_end_y - bound_start_y) * (- preview_offset_y) /  preview_height;
  preview_start_x = CLAMP (bound_start_x + preview_offset_x,
			   bound_start_x, MAX (bound_end_x - preview_width, 0));
  preview_start_y = CLAMP (bound_start_y + preview_offset_y,
			   bound_start_y, MAX (bound_end_y - preview_height, 0));
  if (preview_start_x == bound_start_x)
    preview_offset_x = 0;
  if (preview_start_y == bound_start_y)
    preview_offset_y =0;

  gimp_pixel_rgn_init (&src_rgn, drawable, preview_start_x, preview_start_y,
		       preview_width, preview_height,
		       FALSE, FALSE);

  /* Since it's small, get whole data before processing. */
  gimp_pixel_rgn_get_rect (&src_rgn, preview_buffer,
			   preview_start_x, preview_start_y,
			   preview_width, preview_height);

  scale = 4;
  shift_rate = (gdouble) (scale - 1) / ( 2 * scale);
  for (y = 0; y < preview_height/4; y++)
    {
      for (x = 0; x < preview_width/4; x++)
	{
	  gint pos;
	  gint	i;

	  pos = (gint)(y + preview_height * shift_rate) * src_bpl
	        + (gint)(x + preview_width * shift_rate) * src_bpp;

	  for (i = 0; i < src_bpp; i++)
	    data[i] = preview_buffer[pos + i];

	  scatter_hsv_scatter (data+0, data+1, data+2);
	  for (dy = 0; dy < scale; dy++)
	    for (dx = 0; dx < scale; dx++)
	      gtk_preview_draw_row (GTK_PREVIEW (preview), data,
				    preview_width + x * scale + dx,
				    y * scale + dy, 1);
	}
    }
  for (y = 0; y < preview_height; y ++)
    for (x = 0; x < preview_width; x++)
      {
	gint	i;

	for (i = 0; i < src_bpp; i++)
	  data[i] = preview_buffer[y * src_bpl + x * src_bpp + i];

	scatter_hsv_scatter (data+0, data+1, data+2);
	gtk_preview_draw_row (GTK_PREVIEW (preview), data, x, y, 1);
      }
  gtk_widget_draw (preview, NULL);
  gdk_flush ();
}

static void
OK_CALLBACK (GtkWidget *widget,
	      gpointer   data)
{
  INTERFACE.run = TRUE;
  gtk_widget_destroy (GTK_WIDGET (data));
}

static void
gtkW_close_callback (GtkWidget *widget,
		     gpointer   data)
{
  gtk_main_quit ();
}

/* gtkW is the abbreviation of gtk Wrapper */
static GtkWidget *
gtkW_dialog_new (char * name,
		 GtkSignalFunc ok_callback,
		 GtkSignalFunc close_callback)
{
  GtkWidget *dlg, *button;
  
  dlg = gtk_dialog_new ();
  gtk_window_set_title (GTK_WINDOW (dlg), name);
  gtk_window_position (GTK_WINDOW (dlg), GTK_WIN_POS_MOUSE);
  gtk_signal_connect (GTK_OBJECT (dlg), "destroy",
		      (GtkSignalFunc) gtkW_close_callback, NULL);

  /* Action Area */
  button = gtk_button_new_with_label ( _("OK"));
  GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT);
  gtk_signal_connect (GTK_OBJECT (button), "clicked",
		      (GtkSignalFunc) ok_callback, dlg);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dlg)->action_area), button,
		      TRUE, TRUE, 0);
  gtk_widget_grab_default (button);
  gtk_widget_show (button);

  button = gtk_button_new_with_label ( _("Cancel"));
  GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT);
  gtk_signal_connect_object (GTK_OBJECT (button), "clicked",
			     (GtkSignalFunc) gtk_widget_destroy,
			     GTK_OBJECT(dlg));
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dlg)->action_area), button,
		      TRUE, TRUE, 0);
  gtk_widget_show (button);

  return dlg;
}

static GtkWidget *
gtkW_error_dialog_new (char * name)
{
  GtkWidget *dlg, *button;
  
  dlg = gtk_dialog_new ();
  gtk_window_set_title (GTK_WINDOW (dlg), name);
  gtk_window_position (GTK_WINDOW (dlg), GTK_WIN_POS_MOUSE);
  gtk_signal_connect (GTK_OBJECT (dlg), "destroy",
		      (GtkSignalFunc) gtkW_close_callback, NULL);

  /* Action Area */
  button = gtk_button_new_with_label ( _("OK"));
  GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT);
  gtk_signal_connect (GTK_OBJECT (button), "clicked",
		      (GtkSignalFunc) gtkW_close_callback, dlg);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dlg)->action_area), button,
		      TRUE, TRUE, 0);
  gtk_widget_grab_default (button);
  gtk_widget_show (button);

  return dlg;
}

GtkWidget *
gtkW_table_new (GtkWidget *parent, gint col, gint row)
{
  GtkWidget	*table;
  
  table = gtk_table_new (col,row, FALSE);
  gtk_container_border_width (GTK_CONTAINER (table), 10);
  gtk_container_add (GTK_CONTAINER (parent), table);
  return table;
}

GtkWidget *
gtkW_hbox_new (GtkWidget *parent)
{
  GtkWidget	*hbox;
  
  hbox = gtk_hbox_new (FALSE, 5);
  gtk_container_border_width (GTK_CONTAINER (hbox), 5);
  gtk_box_pack_start (GTK_BOX (parent), hbox, FALSE, TRUE, 0);
  return hbox;
}

GtkWidget *
gtkW_vbox_new (GtkWidget *parent)
{
  GtkWidget *vbox;
  
  vbox = gtk_vbox_new (FALSE, 5);
  gtk_container_border_width (GTK_CONTAINER (vbox), 10);
  /* gtk_box_pack_start (GTK_BOX (parent), vbox, TRUE, TRUE, 0); */
  gtk_container_add (GTK_CONTAINER (parent), vbox);
  return vbox;
}

GtkWidget *
gtkW_check_button_new (GtkWidget	*parent,
		       gchar	*name,
		       GtkSignalFunc update,
		       gint	*value)
{
  GtkWidget *toggle;
  
  toggle = gtk_check_button_new_with_label (name);
  gtk_signal_connect (GTK_OBJECT (toggle), "toggled",
		      (GtkSignalFunc) update,
		      value);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (toggle), *value);
  gtk_container_add (GTK_CONTAINER (parent), toggle);
  gtk_widget_show (toggle);
  return toggle;
}

GtkWidget *
gtkW_frame_new (GtkWidget *parent,
		gchar *name)
{
  GtkWidget *frame;
  
  frame = gtk_frame_new (name);
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_ETCHED_IN);
  gtk_container_border_width (GTK_CONTAINER (frame), 5);
  gtk_box_pack_start (GTK_BOX(parent), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);
  return frame;
}

static void
gtkW_table_add_scale_entry (GtkWidget	*table,
			    gchar	*name,
			    gint	x,
			    gint	y,
			    GtkSignalFunc	scale_update,
			    GtkSignalFunc	entry_update,
			    gint	*value,
			    gdouble	min,
			    gdouble	max,
			    gdouble	step,
			    gchar	*buffer)
{
  GtkObject *adjustment;
  GtkWidget *label, *hbox, *scale, *entry;
  
  label = gtk_label_new (name);
  gtk_misc_set_alignment (GTK_MISC(label), 0.0, 0.5);
  gtk_table_attach (GTK_TABLE(table), label, x, x+1, y, y+1,
		    GTK_FILL|GTK_EXPAND, GTK_FILL, 5, 0);
  gtk_widget_show (label);

  hbox = gtk_hbox_new (FALSE, 5);
  gtk_table_attach (GTK_TABLE (table), hbox, x+1, x+2, y, y+1,
		    GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL, 0, 0);

  adjustment = gtk_adjustment_new (*value, min, max, step, step, 0.0);
  gtk_widget_show (hbox);

  scale = gtk_hscale_new (GTK_ADJUSTMENT (adjustment));
  gtk_widget_set_usize (scale, SCALE_WIDTH, 0);
  gtk_box_pack_start (GTK_BOX (hbox), scale, TRUE, TRUE, 0);
  gtk_scale_set_value_pos (GTK_SCALE (scale), GTK_POS_TOP);
  gtk_scale_set_digits (GTK_SCALE (scale), 0);
  gtk_range_set_update_policy (GTK_RANGE (scale), GTK_UPDATE_DELAYED);
  gtk_signal_connect (GTK_OBJECT (adjustment), "value_changed",
		      (GtkSignalFunc) scale_update, value);

  entry = gtk_entry_new ();
  gtk_object_set_user_data (GTK_OBJECT (entry), adjustment);
  gtk_object_set_user_data (adjustment, entry);
  gtk_box_pack_start (GTK_BOX (hbox), entry, TRUE, TRUE, 0);
  gtk_widget_set_usize (entry, ENTRY_WIDTH/3, 0);
  sprintf (buffer, "%d", *value);
  gtk_entry_set_text (GTK_ENTRY (entry), buffer);
  gtk_signal_connect (GTK_OBJECT (entry), "changed",
		      (GtkSignalFunc) entry_update, value);

  gtk_widget_show (label);
  gtk_widget_show (scale);
  gtk_widget_show (entry);
}

static void
scatter_hsv_iscale_update (GtkAdjustment *adjustment,
			   gpointer       data)
{
  GtkWidget *entry;
  gchar buffer[32];
  int *val;

  val = data;
  if (*val != (int) adjustment->value)
    {
      *val = adjustment->value;
      entry = gtk_object_get_user_data (GTK_OBJECT (adjustment));
      sprintf (buffer, "%d", (int) adjustment->value);
      gtk_entry_set_text (GTK_ENTRY (entry), buffer);
      scatter_hsv_preview_update ();
    }
}

static void
scatter_hsv_ientry_update (GtkWidget *widget,
			   gpointer   data)
{
  GtkAdjustment *adjustment;
  int new_val;
  int *val;

  val = data;
  new_val = atoi (gtk_entry_get_text (GTK_ENTRY (widget)));

  if (*val != new_val)
    {
      adjustment = gtk_object_get_user_data (GTK_OBJECT (widget));

      if ((new_val >= adjustment->lower) &&
	  (new_val <= adjustment->upper))
	{
	  *val = new_val;
	  adjustment->value = new_val;
	  gtk_signal_emit_by_name (GTK_OBJECT (adjustment), "value_changed");
	  scatter_hsv_preview_update ();
	}
    }
}

/* end of scatter_hsv.c */
