/*
 * This is a plug-in for the GIMP.
 *
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 * Copyright (C) 1996 Torsten Martinsen
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
 * $Id$
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "gtk/gtk.h"
#include "libgimp/gimp.h"

#define ENTRY_WIDTH     30
#define SCALE_WIDTH     125
#define HISTSIZE	256

#define MODE_RGB        0
#define MODE_INTEN      1

#define INTENSITY(p)    ((unsigned int) (p[0]*77+p[1]*150+p[2]*29) >> 8)

typedef struct {
  gdouble mask_size;
  gint mode;
} OilifyVals;

typedef struct {
  gint run;
} OilifyInterface;

/* Declare local functions.
 */
static void      query  (void);
static void      run    (char      *name,
                         int        nparams,
                         GParam    *param,
                         int       *nreturn_vals,
                         GParam   **return_vals);
static void      oilify_rgb (GDrawable * drawable);
static void      oilify_intensity (GDrawable * drawable);

static gint      oilify_dialog ();

static void      oilify_close_callback  (GtkWidget *widget,
                                         gpointer   data);
static void      oilify_ok_callback     (GtkWidget *widget,
                                         gpointer   data);
static void      oilify_scale_update    (GtkAdjustment *adjustment,
                                         double        *scale_val);
static void      oilify_entry_update    (GtkWidget *widget,
                                         gdouble *value);
static void      dialog_create_value    (char *title,
                                         GtkTable *table,
                                         int row,
                                         gdouble *value,
                                         double left,
                                         double right);

static void      oilify_toggle_update   (GtkWidget *widget,
                                         gpointer   data);

GPlugInInfo PLUG_IN_INFO =
{
  NULL,    /* init_proc */
  NULL,    /* quit_proc */
  query,   /* query_proc */
  run,     /* run_proc */
};

static OilifyVals ovals =
{
  7.0,     /* mask size  */
  0        /* mode */
};

static OilifyInterface oint =
{
  FALSE     /*  run  */
};


MAIN ()

static void
query ()
{
  static GParamDef args[] =
  {
    { PARAM_INT32, "run_mode", "Interactive, non-interactive" },
    { PARAM_IMAGE, "image", "Input image (unused)" },
    { PARAM_DRAWABLE, "drawable", "Input drawable" },
    { PARAM_INT32, "mask_size", "Oil paint mask size" },
    { PARAM_INT32, "mode", "Algorithm {RGB (0), INTENSITY (1)}" }
  };
  static GParamDef *return_vals = NULL;
  static int nargs = sizeof (args) / sizeof (args[0]);
  static int nreturn_vals = 0;

  gimp_install_procedure ("plug_in_oilify",
			  "Modify the specified drawable to resemble an oil painting",
			  "This function performs the well-known oil-paint effect on the specified drawable.  The size of the input mask is specified by 'mask_size'.",
			  "Torsten Martinsen",
			  "Torsten Martinsen",
			  "1996",
			  "<Image>/Filters/Artistic/Oilify",
			  "RGB*, GRAY*",
			  PROC_PLUG_IN,
			  nargs, nreturn_vals,
			  args, return_vals);
}

static void
run (char    *name,
     int      nparams,
     GParam  *param,
     int     *nreturn_vals,
     GParam **return_vals)
{
  static GParam values[1];
  GDrawable *drawable;
  GRunModeType run_mode;
  GStatusType status = STATUS_SUCCESS;

  run_mode = param[0].data.d_int32;

  *nreturn_vals = 2;
  *return_vals = values;

  values[0].type = PARAM_STATUS;
  values[0].data.d_status = status;

  switch (run_mode)
    {
    case RUN_INTERACTIVE:
      /*  Possibly retrieve data  */
      gimp_get_data ("plug_in_oilify", &ovals);

      /*  First acquire information with a dialog  */
      if (! oilify_dialog ())
        return;
      break;

    case RUN_NONINTERACTIVE:
      /*  Make sure all the arguments are there!  */
      if (nparams != 5)
        status = STATUS_CALLING_ERROR;
      if (status == STATUS_SUCCESS)
        {
          ovals.mask_size = (gdouble) param[3].data.d_int32;
          ovals.mode = (gint) param[4].data.d_int32;
        }
      if (status == STATUS_SUCCESS &&
          ((ovals.mask_size < 1.0) ||
           ((ovals.mode != MODE_INTEN) && (ovals.mode != MODE_RGB))))
        status = STATUS_CALLING_ERROR;
      break;

    case RUN_WITH_LAST_VALS:
      /*  Possibly retrieve data  */
      gimp_get_data ("plug_in_oilify", &ovals);
      break;

    default:
      break;
    }

  /*  Get the specified drawable  */
  drawable = gimp_drawable_get (param[2].data.d_drawable);

  /*  Make sure that the drawable is gray or RGB color  */
  if ((status == STATUS_SUCCESS) &&
      (gimp_drawable_color (drawable->id) || gimp_drawable_gray (drawable->id)))
    {
      gimp_progress_init ("Oil Painting...");
      gimp_tile_cache_ntiles (2 * (drawable->width / gimp_tile_width () + 1));

      if (gimp_drawable_color (drawable->id) && (ovals.mode == MODE_INTEN))
        oilify_intensity (drawable);
      else
        oilify_rgb (drawable);

      if (run_mode != RUN_NONINTERACTIVE)
        gimp_displays_flush ();

      /*  Store data  */
      if (run_mode == RUN_INTERACTIVE)
        gimp_set_data ("plug_in_oilify", &ovals, sizeof (OilifyVals));
    }
  else
    {
      /* gimp_message ("oilify: cannot operate on indexed color images"); */
      status = STATUS_EXECUTION_ERROR;
    }

  values[0].data.d_status = status;

  gimp_drawable_detach (drawable);
}

/*
 * For each RGB channel, replace the pixel at (x,y) with the
 * value that occurs most often in the n x n chunk centered
 * at (x,y).
 */
static void
oilify_rgb (GDrawable *drawable)
{
  GPixelRgn src_rgn, dest_rgn;
  gint bytes;
  gint width, height;
  guchar *src_row, *src;
  guchar *dest_row, *dest;
  gint x, y, c, b, xx, yy, n;
  gint x1, y1, x2, y2;
  gint x3, y3, x4, y4;
  gint Val[4];
  gint Cnt[4];
  gint Hist[4][HISTSIZE];
  gpointer pr1, pr2;
  gint progress, max_progress;
  gint *tmp1, *tmp2;
  guchar *guc_tmp1;

  /*  get the selection bounds  */
  gimp_drawable_mask_bounds (drawable->id, &x1, &y1, &x2, &y2);
  progress = 0;
  max_progress = (x2 - x1) * (y2 - y1);

  width = drawable->width;
  height = drawable->height;
  bytes = drawable->bpp;

  n = (int) ovals.mask_size / 2;

  gimp_pixel_rgn_init (&dest_rgn, drawable, x1, y1, (x2 - x1), (y2 - y1), TRUE, TRUE);

  for (pr1 = gimp_pixel_rgns_register (1, &dest_rgn); pr1 != NULL; pr1 = gimp_pixel_rgns_process (pr1))
    {
      dest_row = dest_rgn.data;

      for (y = dest_rgn.y; y < (dest_rgn.y + dest_rgn.h); y++)
	{
	  dest = dest_row;
 
	  for (x = dest_rgn.x; x < (dest_rgn.x + dest_rgn.w); x++)
	    {
	      memset(Cnt, 0, sizeof(Cnt));
	      memset(Val, 0, sizeof(Val));
	      memset(Hist, 0, sizeof(Hist));

	      x3 = CLAMP ((x - n), x1, x2);
	      y3 = CLAMP ((y - n), y1, y2);
	      x4 = CLAMP ((x + n + 1), x1, x2);
	      y4 = CLAMP ((y + n + 1), y1, y2);
 
	      gimp_pixel_rgn_init (&src_rgn, drawable, x3, y3, (x4 - x3), (y4 - y3), FALSE, FALSE);
 
	      for (pr2 = gimp_pixel_rgns_register (1, &src_rgn); pr2 != NULL; pr2 = gimp_pixel_rgns_process (pr2))
		{
		  src_row = src_rgn.data;
		  
		  for (yy = 0; yy < src_rgn.h; yy++)
		    {
		      src = src_row;
		      
		      for (xx = 0; xx < src_rgn.w; xx++)
			{
			  for (b = 0,
				 tmp1 = Val,
				 tmp2 = Cnt,
				 guc_tmp1 = src;
			       b < bytes;
			       b++, tmp1++, tmp2++, guc_tmp1++)
			    {
			      if ((c = ++Hist[b][*guc_tmp1]) > *tmp2)
				{
				  *tmp1 = *guc_tmp1;
				  *tmp2 = c;
				}
			    }
			  
			  src += src_rgn.bpp;
			}
		      
		      src_row += src_rgn.rowstride;
		    }
		}
	      
	      for (b = 0, tmp1 = Val; b < bytes; b++)
		*dest++ = *tmp1++;
	    }
	  
	  dest_row += dest_rgn.rowstride;
	}
      
      progress += dest_rgn.w * dest_rgn.h;
      if ((progress % 5) == 0)
	gimp_progress_update ((double) progress / (double) max_progress);
    }
  
  /*  update the oil-painted region  */
  gimp_drawable_flush (drawable);
  gimp_drawable_merge_shadow (drawable->id, TRUE);
  gimp_drawable_update (drawable->id, x1, y1, (x2 - x1), (y2 - y1));
}

/*
 * For each RGB channel, replace the pixel at (x,y) with the
 * value that occurs most often in the n x n chunk centered
 * at (x,y). Histogram is based on intensity.
 */
static void
oilify_intensity (GDrawable *drawable)
{
  GPixelRgn src_rgn, dest_rgn;
  gint bytes;
  gint width, height;
  guchar *src_row, *src;
  guchar *dest_row, *dest;
  gint x, y, c, b, xx, yy, n;
  gint x1, y1, x2, y2;
  gint x3, y3, x4, y4;
  gint Val[4];
  gint Cnt;
  gint Hist[HISTSIZE];
  gpointer pr1, pr2;
  gint progress, max_progress;
  gint *tmp1;
  guchar *guc_tmp1;

  /*  get the selection bounds  */
  gimp_drawable_mask_bounds (drawable->id, &x1, &y1, &x2, &y2);
  progress = 0;
  max_progress = (x2 - x1) * (y2 - y1);
  
  width = drawable->width;
  height = drawable->height;
  bytes = drawable->bpp;
  
  n = (int) ovals.mask_size / 2;
  
  gimp_pixel_rgn_init (&dest_rgn, drawable, x1, y1, (x2 - x1), (y2 - y1), TRUE, TRUE);
  
  for (pr1 = gimp_pixel_rgns_register (1, &dest_rgn); pr1 != NULL; pr1 = gimp_pixel_rgns_process (pr1))
    {
      dest_row = dest_rgn.data;
      
      for (y = dest_rgn.y; y < (dest_rgn.y + dest_rgn.h); y++)
	{
	  dest = dest_row;
	  
	  for (x = dest_rgn.x; x < (dest_rgn.x + dest_rgn.w); x++)
	    {
	      Cnt = 0;
	      memset(Val, 0, sizeof(Val));
	      memset(Hist, 0, sizeof(Hist));

	      x3 = CLAMP ((x - n), x1, x2);
	      y3 = CLAMP ((y - n), y1, y2);
	      x4 = CLAMP ((x + n + 1), x1, x2);
	      y4 = CLAMP ((y + n + 1), y1, y2);
	      
	      gimp_pixel_rgn_init (&src_rgn, drawable, x3, y3, (x4 - x3), (y4 - y3), FALSE, FALSE);
	      
	      for (pr2 = gimp_pixel_rgns_register (1, &src_rgn); pr2 != NULL; pr2 = gimp_pixel_rgns_process (pr2))
		{
		  src_row = src_rgn.data;
		  
		  for (yy = 0; yy < src_rgn.h; yy++)
		    {
		      src = src_row;
		      
		      for (xx = 0; xx < src_rgn.w; xx++)
			{
			  if ((c = ++Hist[INTENSITY(src)]) > Cnt)
			    {
			      Cnt = c;
			      for (b = 0,
				     tmp1 = Val,
				     guc_tmp1 = src;
				   b < bytes;
				   b++, tmp1++, guc_tmp1++)
				*tmp1 = *guc_tmp1;
			    }
			  
			  src += src_rgn.bpp;
			}
		      
		      src_row += src_rgn.rowstride;
		    }
		}
	      
	      for (b = 0, tmp1 = Val; b < bytes; b++)
		*dest++ = *tmp1++;
	    }
	  
	  dest_row += dest_rgn.rowstride;
	}
      
      progress += dest_rgn.w * dest_rgn.h;
      if ((progress % 5) == 0)
	gimp_progress_update ((double) progress / (double) max_progress);
    }
  
  /*  update the oil-painted region  */
  gimp_drawable_flush (drawable);
  gimp_drawable_merge_shadow (drawable->id, TRUE);
  gimp_drawable_update (drawable->id, x1, y1, (x2 - x1), (y2 - y1));
}

static gint
oilify_dialog ()
{
  GtkWidget *dlg;
  GtkWidget *button;
  GtkWidget *frame;
  GtkWidget *table;
  GtkWidget *toggle;
  gchar **argv;
  gint argc;

  argc = 1;
  argv = g_new (gchar *, 1);
  argv[0] = g_strdup ("oilify");

  gtk_init (&argc, &argv);
  gtk_rc_parse (gimp_gtkrc ());

  dlg = gtk_dialog_new ();
  gtk_window_set_title (GTK_WINDOW (dlg), "Oilify");
  gtk_window_position (GTK_WINDOW (dlg), GTK_WIN_POS_MOUSE);
  gtk_signal_connect (GTK_OBJECT (dlg), "destroy",
                      (GtkSignalFunc) oilify_close_callback,
                      NULL);

  /*  Action area  */
  button = gtk_button_new_with_label ("OK");
  GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT);
  gtk_signal_connect (GTK_OBJECT (button), "clicked",
                      (GtkSignalFunc) oilify_ok_callback,
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

  /*  parameter settings  */
  frame = gtk_frame_new ("Parameter Settings");
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_ETCHED_IN);
  gtk_container_border_width (GTK_CONTAINER (frame), 10);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dlg)->vbox), frame, TRUE, TRUE, 0);
  table = gtk_table_new (2, 3, FALSE);
  gtk_container_border_width (GTK_CONTAINER (table), 10);
  gtk_container_add (GTK_CONTAINER (frame), table);

  toggle = gtk_check_button_new_with_label ("Use intensity algorithm");
  gtk_table_attach (GTK_TABLE (table), toggle, 0, 2, 0, 1, GTK_FILL, 0, 0, 0);
  gtk_signal_connect (GTK_OBJECT (toggle), "toggled",
                      (GtkSignalFunc) oilify_toggle_update,
                      &ovals.mode);
  gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (toggle), ovals.mode);
  gtk_widget_show (toggle);

  dialog_create_value("Mask Size", GTK_TABLE(table), 1, &ovals.mask_size, 3.0, 50.0);

  gtk_widget_show (frame);
  gtk_widget_show (table);
  gtk_widget_show (dlg);

  gtk_main ();
  gdk_flush ();

  return oint.run;
}


/*  Oilify interface functions  */

static void
oilify_close_callback (GtkWidget *widget,
                       gpointer   data)
{
  gtk_main_quit ();
}

static void
oilify_ok_callback (GtkWidget *widget,
                    gpointer   data)
{
  oint.run = TRUE;
  gtk_widget_destroy (GTK_WIDGET (data));
}

/*
 * Thanks to Quartic for these.
 */
static void
dialog_create_value(char *title, GtkTable *table, int row, gdouble *value, double left, double right)
{
  GtkWidget *label;
  GtkWidget *scale;
  GtkWidget *entry;
  GtkObject *scale_data;
  char       buf[256];
  
  label = gtk_label_new(title);
  gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.5);
  gtk_table_attach(table, label, 0, 1, row, row + 1, GTK_FILL, GTK_FILL, 4, 0);
  gtk_widget_show(label);
  
  scale_data = gtk_adjustment_new(*value, left, right, 1.0, 5.0, 5.0);
  
  gtk_signal_connect(GTK_OBJECT(scale_data), "value_changed",
		     (GtkSignalFunc) oilify_scale_update,
		     value);
  
  scale = gtk_hscale_new(GTK_ADJUSTMENT(scale_data));
  gtk_widget_set_usize(scale, SCALE_WIDTH, 0);
  gtk_table_attach(table, scale, 1, 2, row, row + 1, GTK_EXPAND | GTK_FILL, GTK_FILL, 0, 0);
  gtk_scale_set_draw_value(GTK_SCALE(scale), FALSE);
  gtk_scale_set_digits(GTK_SCALE(scale), 3);
  gtk_range_set_update_policy(GTK_RANGE(scale), GTK_UPDATE_CONTINUOUS);
  gtk_widget_show(scale);
  
  entry = gtk_entry_new();
  gtk_object_set_user_data(GTK_OBJECT(entry), scale_data);
  gtk_object_set_user_data(scale_data, entry);
  gtk_widget_set_usize(entry, ENTRY_WIDTH, 0);
  sprintf(buf, "%.0f", *value);
  gtk_entry_set_text(GTK_ENTRY(entry), buf);
  gtk_signal_connect(GTK_OBJECT(entry), "changed",
		     (GtkSignalFunc) oilify_entry_update,
		     value);
  gtk_table_attach(GTK_TABLE(table), entry, 2, 3, row, row + 1, GTK_FILL, GTK_FILL, 4, 0);
  gtk_widget_show(entry);
}

static void
oilify_entry_update(GtkWidget *widget, gdouble *value)
{
  GtkAdjustment *adjustment;
  gdouble        new_value;

  new_value = atof(gtk_entry_get_text(GTK_ENTRY(widget)));
  
  if (*value != new_value) {
    adjustment = gtk_object_get_user_data(GTK_OBJECT(widget));

    if ((new_value >= adjustment->lower) &&
	(new_value <= adjustment->upper)) {
      *value            = new_value;
      adjustment->value = new_value;
      
      gtk_signal_emit_by_name(GTK_OBJECT(adjustment), "value_changed");
    } /* if */
  } /* if */
}

static void
oilify_scale_update (GtkAdjustment *adjustment, gdouble *value)
{
  GtkWidget *entry;
  char       buf[256];
  
  if (*value != adjustment->value) {
    adjustment->value = (int) adjustment->value | 1;
    *value = adjustment->value;
    
    entry = gtk_object_get_user_data(GTK_OBJECT(adjustment));
    sprintf(buf, "%.0f", *value);
    
    gtk_signal_handler_block_by_data(GTK_OBJECT(entry), value);
    gtk_entry_set_text(GTK_ENTRY(entry), buf);
    gtk_signal_handler_unblock_by_data(GTK_OBJECT(entry), value);
  } /* if */
}
 
static void
oilify_toggle_update (GtkWidget *widget,
		      gpointer   data)
{
  int *toggle_val;
  
  toggle_val = (int *) data;
  
  if (GTK_TOGGLE_BUTTON (widget)->active)
    *toggle_val = MODE_INTEN;
  else
    *toggle_val = MODE_RGB;
}

