/* max_rgb.c -- This is a plug-in for the GIMP (1.0's API)
 * Author: Shuji Narazaki <narazaki@InetQ.or.jp>
 * Time-stamp: <1997/10/23 23:40:20 narazaki@InetQ.or.jp>
 * Version: 0.35
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
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include "gtk/gtk.h"
#include "libgimp/gimp.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Replace them with the right ones */
#define	PLUG_IN_NAME	"plug_in_max_rgb"
#define SHORT_NAME	"max_rgb"
#define PROGRESS_NAME	"max_rgb: scanning..."
#define MENU_POSITION	"<Image>/Filters/Colors/Max RGB"
#define	MAIN_FUNCTION	max_rgb
/* you need not change the following names */
#define INTERFACE	max_rgb_interface
#define	DIALOG		max_rgb_dialog
#define ERROR_DIALOG	max_rgb_error_dialog
#define VALS		max_rgb_vals
#define OK_CALLBACK	_max_rgbok_callback

static void	query	(void);
static void	run	(char	*name,
			 int	nparams,
			 GParam	*param,
			 int	*nreturn_vals,
			 GParam **return_vals);
static GStatusType	MAIN_FUNCTION (gint32 drawable_id);
static gint	DIALOG ();
static void	ERROR_DIALOG (gint gtk_was_not_initialized, gchar *message);

static void
OK_CALLBACK (GtkWidget *widget, gpointer   data);

/* gtkWrapper functions */ 
#define PROGRESS_UPDATE_NUM	100
#define ENTRY_WIDTH	100
#define SCALE_WIDTH	100
static void
gtkW_close_callback (GtkWidget *widget, gpointer   data);
static void
gtkW_toggle_update (GtkWidget *widget, gpointer   data);
static GtkWidget *
gtkW_dialog_new (char *name,
		 GtkSignalFunc ok_callback,
		 GtkSignalFunc close_callback);
static GtkWidget *
gtkW_error_dialog_new (char * name);
static GSList *
gtkW_vbox_add_radio_button (GtkWidget *vbox,
			    gchar	*name,
			    GSList	*group,
			    GtkSignalFunc	update,
			    gint	*value);
GtkWidget *gtkW_check_button_new (GtkWidget	*parent,
				  gchar	*name,
				  GtkSignalFunc update,
				  gint	*value);
GtkWidget *gtkW_frame_new (GtkWidget *parent, gchar *name);
GtkWidget *gtkW_table_new (GtkWidget *parent, gint col, gint row);
GtkWidget *gtkW_hbox_new (GtkWidget *parent);
GtkWidget *gtkW_vbox_new (GtkWidget *parent);

GPlugInInfo PLUG_IN_INFO =
{
  NULL,				/* init_proc  */
  NULL,				/* quit_proc */
  query,			/* query_proc */
  run,				/* run_proc */
};

typedef struct
{
  gint		max_p;		/* gint, gdouble, and so on */
} ValueType;

static ValueType VALS = 
{
  1
};

typedef struct 
{
  gint run;
} Interface;

static Interface INTERFACE = { FALSE };

gint	hold_max;
gint	hold_min;

MAIN ()

static void
query ()
{
  static GParamDef args [] =
  {
    { PARAM_INT32, "run_mode", "Interactive, non-interactive"},
    { PARAM_IMAGE, "image", "Input image (not used)"},
    { PARAM_DRAWABLE, "drawable", "Input drawable"},
    { PARAM_INT32, "max_p", "1 for maximizing, 0 for minimizing"}
  };
  static GParamDef *return_vals = NULL;
  static int nargs = sizeof (args) / sizeof (args[0]);
  static int nreturn_vals = 0;
  
  gimp_install_procedure (PLUG_IN_NAME,
			  "Return an image in which each pixel holds only the channel that has the maximum value in three (red, green, blue) channels, and other channels are zero-cleared",
			  "the help is not yet written for this plug-in",
			  "Shuji Narazaki (narazaki@InetQ.or.jp)",
			  "Shuji Narazaki",
			  "1997",
			  MENU_POSITION,
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
  gint		drawable_id;
  
  run_mode = param[0].data.d_int32;
  drawable_id = param[2].data.d_int32;

  *nreturn_vals = 1;
  *return_vals = values;
  
  values[0].type = PARAM_STATUS;
  values[0].data.d_status = status;

  switch (run_mode)
    {
    case RUN_INTERACTIVE:
      gimp_get_data (PLUG_IN_NAME, &VALS);
      hold_max = VALS.max_p;
      hold_min = VALS.max_p ? 0 : 1;
      /* Since a channel might be selected, we must check wheter RGB or not. */
      if (!gimp_drawable_color(drawable_id))
	{
	  ERROR_DIALOG (1, "RGB drawable is not selected.");
	  return;
	}
      if (! DIALOG ())
	return;
      break;
    case RUN_NONINTERACTIVE:
      /* You must copy the values of parameters to VALS or dialog variables. */
      break;
    case RUN_WITH_LAST_VALS:
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
MAIN_FUNCTION (gint32 drawable_id)
{
  GDrawable	*drawable;
  GPixelRgn	src_rgn, dest_rgn;
  guchar	*src, *dest;
  gpointer	pr;
  gint		x, y, x1, x2, y1, y2;
  gint		gap, total, processed = 0;
  gint		init_value, flag;

  init_value = (VALS.max_p > 0) ? 0 : 255;
  flag = (0 < VALS.max_p) ? 1 : -1;

  drawable = gimp_drawable_get (drawable_id);
  gap = (gimp_drawable_has_alpha (drawable_id)) ? 1 : 0;
  gimp_drawable_mask_bounds (drawable_id, &x1, &y1, &x2, &y2);
  total = (x2 - x1) * (y2 - y1);

  gimp_tile_cache_ntiles (2 * (drawable->width / gimp_tile_width () + 1));
  gimp_pixel_rgn_init (&src_rgn, drawable, x1, y1, (x2 - x1), (y2 - y1), FALSE, FALSE);
  gimp_pixel_rgn_init (&dest_rgn, drawable, x1, y1, (x2 - x1), (y2 - y1), TRUE, TRUE);

  pr = gimp_pixel_rgns_register (2, &src_rgn, &dest_rgn);
  gimp_progress_init (PROGRESS_NAME);
  for (; pr != NULL; pr = gimp_pixel_rgns_process (pr))
    {
      for (y = 0; y < src_rgn.h; y++)
	{
	  src = src_rgn.data + y * src_rgn.rowstride;
	  dest = dest_rgn.data + y * dest_rgn.rowstride;

	  for (x = 0; x < src_rgn.w; x++)
	    {
	      gint	ch, max_ch = 0;
	      guchar	max, tmp_value;
	      
	      max = init_value;
	      for (ch = 0; ch < 3; ch++)
		if (flag * max <= flag * (tmp_value = (*src++)))
		  if (max == tmp_value)
		    max_ch += 1 << ch;
		  else
		    {
		      max_ch = 1 << ch; /* clear memories of old channels */
		      max = tmp_value;
		    }
	      for ( ch = 0; ch < 3; ch++)
		*dest++ = (guchar)(((max_ch & (1 << ch)) > 0) ? max : 0);
	      if (gap) *dest++=*src++;
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

/* dialog stuff */
static int
DIALOG ()
{
  GtkWidget	*dlg;
  GtkWidget	*hbox;
  GtkWidget	*vbox;
  GtkWidget	*frame;
  GSList	*group = NULL;
  gchar	**argv;
  gint	argc;

  argc = 1;
  argv = g_new (gchar *, 1);
  argv[0] = g_strdup (PLUG_IN_NAME);
  gtk_init (&argc, &argv);
  gtk_rc_parse (gimp_gtkrc ());
  
  dlg = gtkW_dialog_new (PLUG_IN_NAME,
			 (GtkSignalFunc) OK_CALLBACK,
			 (GtkSignalFunc) gtkW_close_callback);
  
  hbox = gtkW_hbox_new ((GTK_DIALOG (dlg)->vbox));
  frame = gtkW_frame_new (hbox, "Parameter Settings");
  /*
  table = gtkW_table_new (frame, 2, 2);
  gtkW_table_add_toggle (table, "Hold the maximal channel", 0, 2, 1,
			 (GtkSignalFunc) gtkW_toggle_update, &VALS.max_p);
  gtk_widget_show (table);
  */
  vbox = gtkW_vbox_new (frame);
  group = gtkW_vbox_add_radio_button (vbox, "Hold the maximal channels", group,
				      (GtkSignalFunc) gtkW_toggle_update,
				      &hold_max);
  group = gtkW_vbox_add_radio_button (vbox, "Hold the minimal channels", group,
				      (GtkSignalFunc) gtkW_toggle_update,
				      &hold_min);

  gtk_main ();
  gdk_flush ();

  return INTERFACE.run;
}

static void
ERROR_DIALOG (gint gtk_was_not_initialized, gchar *message)
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

  label = gtk_label_new (message);
  gtk_table_attach (GTK_TABLE (table), label, 0, 1, 0, 1, GTK_FILL|GTK_EXPAND,
		    0, 0, 0);

  gtk_widget_show (label);
  gtk_widget_show (table);
  gtk_widget_show (dlg);
  
  gtk_main ();
  gdk_flush ();
}

static void
OK_CALLBACK (GtkWidget *widget,
	      gpointer   data)
{
  VALS.max_p = hold_max;
  INTERFACE.run = TRUE;
  gtk_widget_destroy (GTK_WIDGET (data));
}

/* VFtext interface functions  */

static void
gtkW_close_callback (GtkWidget *widget,
		     gpointer   data)
{
  gtk_main_quit ();
}

static void
gtkW_toggle_update (GtkWidget *widget,
		    gpointer   data)
{
  int *toggle_val;

  toggle_val = (int *) data;

  if (GTK_TOGGLE_BUTTON (widget)->active)
    *toggle_val = TRUE;
  else
    *toggle_val = FALSE;
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
  button = gtk_button_new_with_label ("OK");
  GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT);
  gtk_signal_connect (GTK_OBJECT (button), "clicked",
		      (GtkSignalFunc) ok_callback, dlg);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dlg)->action_area), button,
		      TRUE, TRUE, 0);
  gtk_widget_grab_default (button);
  gtk_widget_show (button);

  button = gtk_button_new_with_label ("Cancel");
  GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT);
  gtk_signal_connect_object (GTK_OBJECT (button), "clicked",
			     (GtkSignalFunc) gtk_widget_destroy,
			     GTK_OBJECT(dlg));
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dlg)->action_area), button,
		      TRUE, TRUE, 0);
  gtk_widget_show (button);
  gtk_widget_show (dlg);

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
  button = gtk_button_new_with_label ("OK");
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
  gtk_widget_show (hbox);

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
  gtk_widget_show (vbox);

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
  gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (toggle), *value);
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

static GSList *
gtkW_vbox_add_radio_button (GtkWidget *vbox,
			    gchar	*name,
			    GSList	*group,
			    GtkSignalFunc	update,
			    gint	*value)
{
  GtkWidget *toggle;
  
  toggle = gtk_radio_button_new_with_label(group, name);
  group = gtk_radio_button_group (GTK_RADIO_BUTTON (toggle));
  gtk_box_pack_start (GTK_BOX (vbox), toggle, FALSE, FALSE, 0);
  gtk_signal_connect (GTK_OBJECT (toggle), "toggled",
		      (GtkSignalFunc) update, value);
  gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (toggle), *value);
  gtk_widget_show (toggle);
  return group;
}
/* end of max_rgb.c */
