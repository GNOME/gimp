/* threshold_alpha.c -- This is a plug-in for the GIMP (1.0's API)
 * Author: Shuji Narazaki <narazaki@InetQ.or.jp>
 * Time-stamp: <2000-01-09 13:25:30 yasuhiro>
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

#include <gtk/gtk.h>

#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

#include "libgimp/stdplugins-intl.h"


#define	PLUG_IN_NAME        "plug_in_threshold_alpha"
#define SHORT_NAME          "threshold_alpha"
#define PROGRESS_UPDATE_NUM 100
#define SCALE_WIDTH         120

static void query (void);
static void run   (gchar   *name,
		   gint     nparams,
		   GParam  *param,
		   gint    *nreturn_vals,
		   GParam **return_vals);

static GStatusType threshold_alpha (gint32 drawable_id);

static gint threshold_alpha_dialog      (void);
static void threshold_alpha_ok_callback (GtkWidget *widget,
					 gpointer   data);

GPlugInInfo PLUG_IN_INFO =
{
  NULL,  /* init_proc  */
  NULL,  /* quit_proc  */
  query, /* query_proc */
  run,   /* run_proc   */
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

static Interface INTERFACE =
{
  FALSE
};

MAIN ()

static void
query (void)
{
  static GParamDef args [] =
  {
    { PARAM_INT32, "run_mode", "Interactive, non-interactive"},
    { PARAM_IMAGE, "image", "Input image (not used)"},
    { PARAM_DRAWABLE, "drawable", "Input drawable" },
    { PARAM_INT32, "threshold", "Threshold" }
  };
  static gint nargs = sizeof (args) / sizeof (args[0]);

  gimp_install_procedure (PLUG_IN_NAME,
			  "",
			  "",
			  "Shuji Narazaki (narazaki@InetQ.or.jp)",
			  "Shuji Narazaki",
			  "1997",
			  N_("<Image>/Image/Alpha/Threshold Alpha..."),
			  "RGBA,GRAYA",
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
  GStatusType   status = STATUS_SUCCESS;
  GRunModeType  run_mode;
  gint          drawable_id;
  
  run_mode = param[0].data.d_int32;
  drawable_id = param[2].data.d_int32;

  if (run_mode != RUN_INTERACTIVE)
    {
      INIT_I18N();
    }
  else
    {
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
	  g_message (_("The layer preserves transparency."));
	  return;
	}
      if (!gimp_drawable_is_rgb (drawable_id) &&
	  !gimp_drawable_is_gray (drawable_id))
	{
	  g_message (_("RGBA/GRAYA drawable is not selected."));
	  return;
	}
      gimp_get_data (PLUG_IN_NAME, &VALS);
      if (! threshold_alpha_dialog ())
	return;
      break;

    case RUN_NONINTERACTIVE:
      if (nparams != 4)
	{
	  status = STATUS_CALLING_ERROR;
	}
      else
	{
	  VALS.threshold = param[3].data.d_int32;
	} 
      break;

    case RUN_WITH_LAST_VALS:
      gimp_get_data (PLUG_IN_NAME, &VALS);
      break;
    }
  
  if (status == STATUS_SUCCESS)
    {
      status = threshold_alpha (drawable_id);

      if (run_mode != RUN_NONINTERACTIVE)
	gimp_displays_flush ();
      if (run_mode == RUN_INTERACTIVE && status == STATUS_SUCCESS)
	gimp_set_data (PLUG_IN_NAME, &VALS, sizeof (ValueType));
    }

  values[0].type = PARAM_STATUS;
  values[0].data.d_status = status;
}

static GStatusType
threshold_alpha (gint32 drawable_id)
{
  GDrawable *drawable;
  GPixelRgn  src_rgn, dest_rgn;
  guchar    *src, *dest;
  gpointer   pr;
  gint       x, y, x1, x2, y1, y2;
  gint       gap, total, processed = 0;
  
  drawable = gimp_drawable_get (drawable_id);
  if (! gimp_drawable_has_alpha (drawable_id))
    return STATUS_EXECUTION_ERROR;

  if (gimp_drawable_is_rgb (drawable_id))
    gap = 3;
  else
    gap = 1;

  gimp_drawable_mask_bounds (drawable_id, &x1, &y1, &x2, &y2);
  total = (x2 - x1) * (y2 - y1);

  gimp_tile_cache_ntiles (2 * (drawable->width / gimp_tile_width () + 1));
  gimp_pixel_rgn_init (&src_rgn, drawable,
		       x1, y1, (x2 - x1), (y2 - y1), FALSE, FALSE);
  gimp_pixel_rgn_init (&dest_rgn, drawable,
		       x1, y1, (x2 - x1), (y2 - y1), TRUE, TRUE);

  pr = gimp_pixel_rgns_register (2, &src_rgn, &dest_rgn);
  gimp_progress_init (_("Threshold Alpha: Coloring Transparency..."));
  for (; pr != NULL; pr = gimp_pixel_rgns_process (pr))
    {
      gint offset, index;

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
		gimp_progress_update ((double) processed /(double) total); 
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
static gint
threshold_alpha_dialog (void)
{
  GtkWidget *dlg;
  GtkWidget *frame;
  GtkWidget *table;
  GtkObject *adj;

  gimp_ui_init ("threshold_alpha", FALSE);

  dlg = gimp_dialog_new (_("Threshold Alpha"), "threshold_alpha",
			 gimp_standard_help_func, "filters/threshold_alpha.html",
			 GTK_WIN_POS_MOUSE,
			 FALSE, TRUE, FALSE,

			 _("OK"), threshold_alpha_ok_callback,
			 NULL, NULL, NULL, TRUE, FALSE,
			 _("Cancel"), gtk_widget_destroy,
			 NULL, 1, NULL, FALSE, TRUE,

			 NULL);

  gtk_signal_connect (GTK_OBJECT (dlg), "destroy",
		      GTK_SIGNAL_FUNC (gtk_main_quit),
		      NULL);

  frame = gtk_frame_new (_("Parameter Settings"));
  gtk_container_set_border_width (GTK_CONTAINER (frame), 6);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dlg)->vbox), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  table = gtk_table_new (1 ,3, FALSE);
  gtk_table_set_col_spacings (GTK_TABLE (table), 4);
  gtk_container_set_border_width (GTK_CONTAINER (table), 4);
  gtk_container_add (GTK_CONTAINER (frame), table);
  gtk_widget_show (table);

  adj = gimp_scale_entry_new (GTK_TABLE (table), 0, 0,
			      _("Threshold:"), SCALE_WIDTH, 0,
			      VALS.threshold, 0, 255, 1, 8, 0,
			      TRUE, 0, 0,
			      NULL, NULL);
  gtk_signal_connect (GTK_OBJECT (adj), "value_changed",
		      GTK_SIGNAL_FUNC (gimp_int_adjustment_update),
		      &VALS.threshold);

  gtk_widget_show (dlg);

  gtk_main ();
  gdk_flush ();

  return INTERFACE.run;
}

static void
threshold_alpha_ok_callback (GtkWidget *widget,
			     gpointer   data)
{
  INTERFACE.run = TRUE;

  gtk_widget_destroy (GTK_WIDGET (data));
}
