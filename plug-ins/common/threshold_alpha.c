/* threshold_alpha.c -- This is a plug-in for the GIMP (1.0's API)
 * Author: Shuji Narazaki <narazaki@InetQ.or.jp>
 * Time-stamp: <1999-09-05 04:24:02 yasuhiro>
 * Version: 0.13A (the 'A' is for Adam who hacked in greyscale
 *                 support - don't know if there's a more recent official
 *                 version)
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <gtk/gtk.h>

#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>
#include <libgimp/gimpmath.h>

#include "libgimp/stdplugins-intl.h"

/* Replace them with the right ones */
#define	PLUG_IN_NAME	"plug_in_threshold_alpha"
#define SHORT_NAME	"threshold_alpha"
#define	MAIN_FUNCTION	threshold_alpha
/* you need not change the following names */
#define INTERFACE	threshold_alpha_interface
#define	DIALOG		threshold_alpha_dialog
#define ERROR_DIALOG	threshold_alpha_error_dialog
#define VALS		threshold_alpha_vals
#define OK_CALLBACK	threshold_alpha_ok_callback

static void	query	(void);
static void	run	(char	*name,
			 int	nparams,
			 GParam	*param,
			 int	*nreturn_vals,
			 GParam **return_vals);
static GStatusType	MAIN_FUNCTION ();
static gint	DIALOG ();
static void	ERROR_DIALOG (gint gtk_was_not_initialized, gchar *message);
static void     OK_CALLBACK (GtkWidget *widget, gpointer   data);

/* gtkWrapper functions */ 
#define PROGRESS_UPDATE_NUM	100
#define ENTRY_WIDTH	100
#define SCALE_WIDTH	120
/*
static void
gtkW_gint_update (GtkWidget *widget, gpointer   data);
static void
gtkW_scale_update (GtkAdjustment *adjustment, double   *data);
*/
/*
static void
gtkW_toggle_update (GtkWidget *widget, gpointer   data);
*/
static void
gtkW_iscale_update (GtkAdjustment *adjustment,
		    gpointer       data);
static void
gtkW_ientry_update (GtkWidget *widget,
		    gpointer   data);
/*
static void
gtkW_table_add_toggle (GtkWidget	*table,
		       gchar	*name,
		       gint	x1,
		       gint	x2,
		       gint	y,
		       GtkSignalFunc update,
		       gint	*value);
*/
/*
static GSList *
gtkW_vbox_add_radio_button (GtkWidget *vbox,
			    gchar	*name,
			    GSList	*group,
			    GtkSignalFunc	update,
			    gint	*value);
*/
/*
static void
gtkW_table_add_gint (GtkWidget	*table,
		     gchar	*name,
		     gint	x,
		     gint	y, 
		     GtkSignalFunc	update,
		     gint	*value,
		     gchar	*buffer);
*/
/*
static void
gtkW_table_add_scale (GtkWidget	*table,
		      gchar	*name,
		      gint	x1,
		      gint	y,
		      GtkSignalFunc update,
		      gdouble *value,
		      gdouble min,
		      gdouble max,
		      gdouble step);
*/
static void
gtkW_table_add_iscale_entry (GtkWidget	*table,
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

GPlugInInfo PLUG_IN_INFO =
{
  NULL,				/* init_proc  */
  NULL,				/* quit_proc */
  query,			/* query_proc */
  run,				/* run_proc */
};

typedef struct
{
  gint	threshold;
} ValueType;

static ValueType VALS = 
{
  127
};

typedef struct 
{
  gint run;
} Interface;

static Interface INTERFACE = { FALSE };

MAIN ()

static void
query ()
{
  static GParamDef args [] =
  {
    { PARAM_INT32, "run_mode", "Interactive, non-interactive"},
    { PARAM_IMAGE, "image", "Input image (not used)"},
    { PARAM_DRAWABLE, "drawable", "Input drawable" },
    { PARAM_INT32, "threshold", "Threshold" },
  };
  static GParamDef *return_vals = NULL;
  static int nargs = sizeof (args) / sizeof (args[0]);
  static int nreturn_vals = 0;

  gimp_install_procedure (PLUG_IN_NAME,
			  "",
			  "",
			  "Shuji Narazaki (narazaki@InetQ.or.jp)",
			  "Shuji Narazaki",
			  "1997",
			  N_("<Image>/Image/Alpha/Threshold Alpha..."),
			  "RGBA,GRAYA",
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
  GStatusType	status = STATUS_SUCCESS;
  GRunModeType	run_mode;
  gint		drawable_id;
  
  run_mode = param[0].data.d_int32;
  drawable_id = param[2].data.d_int32;

  if (run_mode != RUN_INTERACTIVE) {
    INIT_I18N();
  } else {
    INIT_I18N_UI();
  }

  *nreturn_vals = 1;
  *return_vals = values;
  
  values[0].type = PARAM_STATUS;
  values[0].data.d_status = status;

  switch (run_mode)
    {
    case RUN_INTERACTIVE:
      /* Since a channel might be selected, we must check wheter RGB or not. */
      if (gimp_layer_get_preserve_transparency (drawable_id))
	{
	  ERROR_DIALOG (1, _("The layer preserves transparency."));
	  return;
	}
      if (!gimp_drawable_is_rgb (drawable_id) &&
	  !gimp_drawable_is_gray (drawable_id))
	{
	  ERROR_DIALOG (1, _("RGBA/GRAYA drawable is not selected."));
	  return;
	}
      gimp_get_data (PLUG_IN_NAME, &VALS);
      if (! DIALOG ())
	return;
      break;
    case RUN_NONINTERACTIVE:
      if (nparams != 4)
	status = STATUS_CALLING_ERROR;
      if (status == STATUS_SUCCESS)
	{
	  VALS.threshold = param[3].data.d_int32;
	} 
      break;
    case RUN_WITH_LAST_VALS:
      gimp_get_data (PLUG_IN_NAME, &VALS);
      break;
    }
  
  if (status == STATUS_SUCCESS)
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
  
  drawable = gimp_drawable_get (drawable_id);
  if (! gimp_drawable_has_alpha (drawable_id)) return STATUS_EXECUTION_ERROR;

  if (gimp_drawable_is_rgb(drawable_id))
    gap = 3;
  else
    gap = 1;

  gimp_drawable_mask_bounds (drawable_id, &x1, &y1, &x2, &y2);
  total = (x2 - x1) * (y2 - y1);

  gimp_tile_cache_ntiles (2 * (drawable->width / gimp_tile_width () + 1));
  gimp_pixel_rgn_init (&src_rgn, drawable, x1, y1, (x2 - x1), (y2 - y1), FALSE, FALSE);
  gimp_pixel_rgn_init (&dest_rgn, drawable, x1, y1, (x2 - x1), (y2 - y1), TRUE, TRUE);

  pr = gimp_pixel_rgns_register (2, &src_rgn, &dest_rgn);
  gimp_progress_init (_("threshold_alpha (0.13):coloring transparency..."));
  for (; pr != NULL; pr = gimp_pixel_rgns_process (pr))
    {
      int	offset, index;
      
      for (y = 0; y < src_rgn.h; y++)
	{
	  src = src_rgn.data + y * src_rgn.rowstride;
	  dest = dest_rgn.data + y * dest_rgn.rowstride;
	  offset = 0;

	  for (x = 0; x < src_rgn.w; x++)
	    {
	      for (index = 0; index < gap; index++)
		*dest++ = *src++;
	      *dest++ = (VALS.threshold < *src++) ? 255 : 0;

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
  GtkWidget	*frame;
  GtkWidget	*table;
  gchar	**argv;
  gint	  argc;

  argc    = 1;
  argv    = g_new (gchar *, 1);
  argv[0] = g_strdup ("threshold_alpha");

  gtk_init (&argc, &argv);
  gtk_rc_parse (gimp_gtkrc ());
  
  dlg = gimp_dialog_new (_("Threshold Alpha"), "threshold_alpha",
			 gimp_plugin_help_func, "filters/threshold_alpha.html",
			 GTK_WIN_POS_MOUSE,
			 FALSE, TRUE, FALSE,

			 _("OK"), OK_CALLBACK,
			 NULL, NULL, NULL, TRUE, FALSE,
			 _("Cancel"), gtk_widget_destroy,
			 NULL, 1, NULL, FALSE, TRUE,

			 NULL);

  gtk_signal_connect (GTK_OBJECT (dlg), "destroy",
		      GTK_SIGNAL_FUNC (gtk_main_quit),
		      NULL);

  frame = gtkW_frame_new (GTK_DIALOG (dlg)->vbox, _("Parameter Settings"));
  table = gtkW_table_new (frame, 1, 2);
  gtkW_table_add_iscale_entry (table, _("Threshold:"), 0, 0,
			       (GtkSignalFunc) gtkW_iscale_update,
			       (GtkSignalFunc) gtkW_ientry_update,
			       &VALS.threshold,
			       0, 255, 1, malloc (10));

  gtk_widget_show (dlg);
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
  
  dlg = gimp_dialog_new (_("Threshold Aplha"), "threshold_alpha",
			 gimp_plugin_help_func, "filters/threshold_alpha.html",
			 GTK_WIN_POS_MOUSE,
			 FALSE, TRUE, FALSE,

			 _("OK"), gtk_main_quit,
			 NULL, NULL, NULL, TRUE, TRUE,

			 NULL);

  table = gtk_table_new (1,1, FALSE);
  gtk_container_set_border_width (GTK_CONTAINER (table), 6);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dlg)->vbox), table, TRUE, TRUE, 0);

  label = gtk_label_new (message);
  gtk_table_attach (GTK_TABLE (table), label, 0, 1, 0, 1, GTK_FILL|GTK_EXPAND,
		    0, 0, 0);
  gtk_widget_show (table);
  gtk_widget_show (label);
  gtk_widget_show (dlg);

  gtk_main ();
  gdk_flush ();
}

static void
OK_CALLBACK (GtkWidget *widget,
	      gpointer   data)
{
  INTERFACE.run = TRUE;
  gtk_widget_destroy (GTK_WIDGET (data));
}

/* VFtext interface functions  */
/*
static void
gtkW_gint_update (GtkWidget *widget,
		  gpointer   data)
{
  *(gint *)data = (gint)atof (gtk_entry_get_text (GTK_ENTRY (widget)));
}
*/

/*
static void
gtkW_scale_update (GtkAdjustment *adjustment,
		   gdouble	 *scale_val)
{
  *scale_val = (gdouble) adjustment->value;
}
*/

/*
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
*/

GtkWidget *
gtkW_table_new (GtkWidget *parent, gint col, gint row)
{
  GtkWidget	*table;
  
  table = gtk_table_new (col,row, FALSE);
  gtk_table_set_col_spacings (GTK_TABLE (table), 4);
  gtk_container_set_border_width (GTK_CONTAINER (table), 4);
  gtk_container_add (GTK_CONTAINER (parent), table);
  gtk_widget_show (table);
  
  return table;
}

GtkWidget *
gtkW_hbox_new (GtkWidget *parent)
{
  GtkWidget *hbox;

  hbox = gtk_hbox_new (FALSE, 4);
  gtk_container_set_border_width (GTK_CONTAINER (hbox), 5);
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
  gtk_container_set_border_width (GTK_CONTAINER (frame), 6);
  gtk_box_pack_start (GTK_BOX(parent), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);
  return frame;
}

/*
static void
gtkW_table_add_toggle (GtkWidget	*table,
		       gchar	*name,
		       gint	x1,
		       gint	x2,
		       gint	y,
		       GtkSignalFunc update,
		       gint	*value)
{
  GtkWidget *toggle;
  
  toggle = gtk_check_button_new_with_label(name);
  gtk_table_attach (GTK_TABLE (table), toggle, x1, x2, y, y+1,
		    GTK_FILL|GTK_EXPAND, 0, 0, 0);
  gtk_signal_connect (GTK_OBJECT (toggle), "toggled",
		      (GtkSignalFunc) update,
		      value);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (toggle), *value);
  gtk_widget_show (toggle);
}
*/

/*
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
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (toggle), *value);
  gtk_widget_show (toggle);
  return group;
}
*/

/*
static void
gtkW_table_add_gint (GtkWidget	*table,
		     gchar	*name,
		     gint	x,
		     gint	y, 
		     GtkSignalFunc	update,
		     gint	*value,
		     gchar	*buffer)
{
  GtkWidget *label, *entry;
  
  label = gtk_label_new (name);
  gtk_misc_set_alignment (GTK_MISC(label), 0.0, 0.5);
  gtk_table_attach (GTK_TABLE(table), label, x, x+1, y, y+1,
		    GTK_FILL|GTK_EXPAND, GTK_FILL, 5, 0);
  gtk_widget_show(label);

  entry = gtk_entry_new();
  gtk_table_attach (GTK_TABLE(table), entry, x+1, x+2, y, y+1,
		    GTK_FILL|GTK_EXPAND, GTK_FILL, 10, 0);
  gtk_widget_set_usize (entry, ENTRY_WIDTH, 0);
  sprintf (buffer, "%d", *(gint *)value);
  gtk_entry_set_text (GTK_ENTRY (entry), buffer);
  gtk_signal_connect (GTK_OBJECT (entry), "changed",
		      (GtkSignalFunc) update, value);
  gtk_widget_show(entry);
}
*/

/*
static void
gtkW_table_add_scale (GtkWidget	*table,
		      gchar	*name,
		      gint	x,
		      gint	y,
		      GtkSignalFunc update,
		      gdouble *value,
		      gdouble min,
		      gdouble max,
		      gdouble step)
{
  GtkObject *scale_data;
  GtkWidget *label, *scale;
  
  label = gtk_label_new (name);
  gtk_misc_set_alignment (GTK_MISC(label), 0.0, 0.5);
  gtk_table_attach (GTK_TABLE(table), label, x, x+1, y, y+1,
		    GTK_FILL|GTK_EXPAND, GTK_FILL, 5, 0);
  gtk_widget_show (label);

  scale_data = gtk_adjustment_new (*value, min, max, step, step, 0.0);

  scale = gtk_hscale_new (GTK_ADJUSTMENT (scale_data));
  gtk_widget_set_usize (scale, SCALE_WIDTH, 0);
  gtk_table_attach (GTK_TABLE (table), scale, x+1, x+2, y, y+1, 
		    GTK_FILL|GTK_EXPAND, GTK_FILL, 10, 5);
  gtk_scale_set_value_pos (GTK_SCALE (scale), GTK_POS_TOP);
  gtk_scale_set_digits (GTK_SCALE (scale), 2);
  gtk_range_set_update_policy (GTK_RANGE (scale), GTK_UPDATE_DELAYED);
  gtk_signal_connect (GTK_OBJECT (scale_data), "value_changed",
		      (GtkSignalFunc) update, value);
  gtk_widget_show (label);
  gtk_widget_show (scale);
}
*/

static void
gtkW_table_add_iscale_entry (GtkWidget	*table,
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
  gtk_misc_set_alignment (GTK_MISC(label), 1.0, 0.5);
  gtk_table_attach (GTK_TABLE(table), label, x, x+1, y, y+1,
		    GTK_FILL|GTK_EXPAND, GTK_FILL, 0, 0);
  gtk_widget_show (label);

  hbox = gtk_hbox_new (FALSE, 4);
  gtk_table_attach (GTK_TABLE (table), hbox, x+1, x+2, y, y+1,
		    GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL, 0, 0);

  adjustment = gtk_adjustment_new (*value, min, max, step, step, 0.0);
  gtk_widget_show (hbox);

  scale = gtk_hscale_new (GTK_ADJUSTMENT (adjustment));
  gtk_widget_set_usize (scale, SCALE_WIDTH, 0);
  gtk_box_pack_start (GTK_BOX (hbox), scale, TRUE, TRUE, 0);
  gtk_scale_set_draw_value (GTK_SCALE (scale), FALSE);
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
gtkW_iscale_update (GtkAdjustment *adjustment,
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
    }
}

static void
gtkW_ientry_update (GtkWidget *widget,
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
	}
    }
}

/* end of threshold_alpha.c */
