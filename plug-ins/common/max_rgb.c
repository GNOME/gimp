/* max_rgb.c -- This is a plug-in for the GIMP (1.0's API)
 * Author: Shuji Narazaki <narazaki@InetQ.or.jp>
 * Time-stamp: <2000-02-08 16:26:24 yasuhiro>
 * Version: 0.35
 *
 * Copyright (C) 1997 Shuji Narazaki <narazaki@InetQ.or.jp>
 *
 * May 2000 - tim copperfield [timecop@japan.co.jp]
 *
 * Added a preview mode.  After adding preview mode realised just exactly
 * how useless this plugin is :)
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

#include "libgimp/stdplugins-intl.h"


/* Replace them with the right ones */
#define	PLUG_IN_NAME	     "plug_in_max_rgb"
#define SHORT_NAME	     "max_rgb"
#define PROGRESS_UPDATE_NUM  100

static void	query	(void);
static void	run	(gchar      *name,
			 gint        nparams,
			 GimpParam  *param,
			 gint       *nreturn_vals,
			 GimpParam **return_vals);

static GimpPDBStatusType main_function     (GimpDrawable *drawable, 
					    gboolean      preview_mode);

static gint	   dialog         (GimpDrawable *drawable);
static void        ok_callback    (GtkWidget    *widget,
				   gpointer      data);
static void        radio_callback (GtkWidget    *widget, 
				   gpointer      data);


GimpPlugInInfo PLUG_IN_INFO =
{
  NULL,  /* init_proc  */
  NULL,  /* quit_proc  */
  query, /* query_proc */
  run,   /* run_proc   */
};

enum
{
  MIN_CHANNELS = 0,
  MAX_CHANNELS = 1
};

typedef struct
{
  gint max_p;  /* gint, gdouble, and so on */
} ValueType;

typedef struct 
{
  gint run;
} Interface;

static ValueType pvals = 
{
  MAX_CHANNELS
};

static Interface interface =
{
  FALSE
};

static GimpFixMePreview *preview;

MAIN ()

static void
query (void)
{
  static GimpParamDef args [] =
  {
    { GIMP_PDB_INT32,    "run_mode", "Interactive, non-interactive"},
    { GIMP_PDB_IMAGE,    "image",    "Input image (not used)"},
    { GIMP_PDB_DRAWABLE, "drawable", "Input drawable"},
    { GIMP_PDB_INT32,    "max_p",    "1 for maximizing, 0 for minimizing"}
  };

  gimp_install_procedure (PLUG_IN_NAME,
			  "Return an image in which each pixel holds only "
			  "the channel that has the maximum value in three "
			  "(red, green, blue) channels, and other channels "
			  "are zero-cleared",
			  "the help is not yet written for this plug-in since none is needed.",
			  "Shuji Narazaki (narazaki@InetQ.or.jp)",
			  "Shuji Narazaki",
			  "May 2000",
                          N_("<Image>/Filters/Colors/Max RGB..."),
			  "RGB*",
			  GIMP_PLUGIN,
			  G_N_ELEMENTS (args), 0,
			  args, NULL);
}

static void
run (gchar      *name,
     gint        nparams,
     GimpParam  *param,
     gint       *nreturn_vals,
     GimpParam **return_vals)
{
  GimpDrawable      *drawable;
  static GimpParam   values[1];
  GimpPDBStatusType  status = GIMP_PDB_EXECUTION_ERROR;
  GimpRunMode        run_mode;
  
  run_mode = param[0].data.d_int32;
  drawable = gimp_drawable_get (param[2].data.d_drawable);

  *nreturn_vals = 1;
  *return_vals  = values;
  
  values[0].type          = GIMP_PDB_STATUS;
  values[0].data.d_status = status;

  switch (run_mode)
    {
    case GIMP_RUN_INTERACTIVE:
      INIT_I18N_UI();
      gimp_get_data (PLUG_IN_NAME, &pvals);
      /* Since a channel might be selected, we must check wheter RGB or not. */
      if (!gimp_drawable_is_rgb (drawable->drawable_id))
	{
	  g_message (_("Max RGB: Can only operate on RGB drawables."));
	  return;
	}
      if (! dialog (drawable))
	return;
      break;
    case GIMP_RUN_NONINTERACTIVE:
      INIT_I18N();
      /* You must copy the values of parameters to pvals or dialog variables. */
      break;
    case GIMP_RUN_WITH_LAST_VALS:
      INIT_I18N();
      gimp_get_data (PLUG_IN_NAME, &pvals);
      break;
    }
  
  status = main_function (drawable, FALSE);

  if (run_mode != GIMP_RUN_NONINTERACTIVE)
    gimp_displays_flush ();
  if (run_mode == GIMP_RUN_INTERACTIVE && status == GIMP_PDB_SUCCESS)
    gimp_set_data (PLUG_IN_NAME, &pvals, sizeof (ValueType));

  values[0].data.d_status = status;
}

static GimpPDBStatusType
main_function (GimpDrawable *drawable, 
	       gboolean      preview_mode)
{
  GimpPixelRgn  src_rgn, dest_rgn;
  guchar    *src, *dest, *save_dest, *src_data, *dest_data;
  gpointer   pr = NULL;
  gint       x, y, x1, x2, y1, y2;
  gint       gap, total, processed = 0;
  gint       init_value, flag;
  gint       bpp = 3;

  init_value = (pvals.max_p > 0) ? 0 : 255;
  flag = (0 < pvals.max_p) ? 1 : -1;

  if (preview_mode) 
    {
      x1 = y1 = 0;
      x2 = preview->width;
      y2 = preview->height;
      gap = 0; /* no alpha on preview */
      bpp = preview->bpp;
    } 
  else 
    {
      gap = (gimp_drawable_has_alpha (drawable->drawable_id)) ? 1 : 0;
      gimp_drawable_mask_bounds (drawable->drawable_id, &x1, &y1, &x2, &y2);
      gimp_tile_cache_ntiles (2 * (drawable->width / gimp_tile_width () + 1));
      gimp_pixel_rgn_init (&src_rgn, drawable,
			   x1, y1, (x2 - x1), (y2 - y1), FALSE, FALSE);
      gimp_pixel_rgn_init (&dest_rgn, drawable,
			   x1, y1, (x2 - x1), (y2 - y1), TRUE, TRUE);
      pr = gimp_pixel_rgns_register (2, &src_rgn, &dest_rgn);
      gimp_progress_init ( _("Max RGB: Scanning..."));
    }
  
  total = (x2 - x1) * (y2 - y1);
  if (total < 1)
    return GIMP_PDB_EXECUTION_ERROR;

  if (preview_mode) 
    {
      src_data = g_malloc (preview->rowstride * y2);
      memcpy (src_data, preview->cache, preview->rowstride * y2);
      dest_data = g_malloc (preview->rowstride * y2);
      save_dest = dest_data;
      
      for (y = 0; y < y2; y++)
	{
	  src  = src_data  + y * preview->rowstride;
	  dest = dest_data + y * preview->rowstride;
	  
	  for (x = 0; x < x2; x++)
	    {
	      gint   ch, max_ch = 0;
	      guchar max, tmp_value;
	      
	      max = init_value;
	      for (ch = 0; ch < 3; ch++)
		if (flag * max <= flag * (tmp_value = (*src++)))
		  {
		    if (max == tmp_value)
		      {
			max_ch += 1 << ch;
		      }
		    else
		      {
			max_ch = 1 << ch; /* clear memories of old channels */
			max = tmp_value;
		      }
		  }
	      
	      for ( ch = 0; ch < 3; ch++)
		*dest++ = (guchar)(((max_ch & (1 << ch)) > 0) ? max : 0);
	      
	      if (gap) 
		*dest++ = *src++;
	    }
	}
      
      memcpy (preview->buffer, save_dest, preview->rowstride * y2);
      gtk_widget_queue_draw (preview->widget);
    } 
  else 
    { /* normal mode */
      for (; pr != NULL; pr = gimp_pixel_rgns_process (pr))
	{
	  for (y = 0; y < src_rgn.h; y++)
	    {
	      src = src_rgn.data + y * src_rgn.rowstride;
	      dest = dest_rgn.data + y * dest_rgn.rowstride;
	      
	      for (x = 0; x < src_rgn.w; x++)
		{
		  gint   ch, max_ch = 0;
		  guchar max, tmp_value;
		  
		  max = init_value;
		  for (ch = 0; ch < 3; ch++)
		    if (flag * max <= flag * (tmp_value = (*src++)))
		      {
			if (max == tmp_value)
			  {
			    max_ch += 1 << ch;
			  }
			else
			  {
			    max_ch = 1 << ch; /* clear memories of old channels */
			    max = tmp_value;
			  }
			
		      }
		  for ( ch = 0; ch < 3; ch++)
		    *dest++ = (guchar)(((max_ch & (1 << ch)) > 0) ? max : 0);
		  
		  if (gap) 
		    *dest++=*src++;
		  
		  if ((++processed % (total / PROGRESS_UPDATE_NUM + 1)) == 0)
		    gimp_progress_update ((gdouble) processed / (gdouble) total); 
		}
	    }
	}
      gimp_progress_update (1.0);
      gimp_drawable_flush (drawable);
      gimp_drawable_merge_shadow (drawable->drawable_id, TRUE);
      gimp_drawable_update (drawable->drawable_id, x1, y1, (x2 - x1), (y2 - y1));
      gimp_drawable_detach (drawable);
    }
  
  return GIMP_PDB_SUCCESS;
}
 

/* dialog stuff */
static gint
dialog (GimpDrawable *drawable)
{
  GtkWidget *dlg;
  GtkWidget *main_vbox;
  GtkWidget *frame;
  GtkWidget *max;
  GtkWidget *min;

  gimp_ui_init ("max_rgb", TRUE);

  dlg = gimp_dialog_new (_("Max RGB"), "max_rgb",
			 gimp_standard_help_func, "filters/max_rgb.html",
			 GTK_WIN_POS_MOUSE,
			 FALSE, TRUE, FALSE,

			 GTK_STOCK_CANCEL, gtk_widget_destroy,
			 NULL, 1, NULL, FALSE, TRUE,

			 GTK_STOCK_OK, ok_callback,
			 NULL, NULL, NULL, TRUE, FALSE,

			 NULL);

  g_signal_connect (dlg, "destroy",
                    G_CALLBACK (gtk_main_quit),
                    NULL);

  main_vbox = gtk_vbox_new (FALSE, 4);
  gtk_container_set_border_width (GTK_CONTAINER (main_vbox), 6);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dlg)->vbox), main_vbox,
                      TRUE, TRUE, 0);
  gtk_widget_show (main_vbox);

  preview = gimp_fixme_preview_new (drawable, TRUE);
  gtk_box_pack_start (GTK_BOX (main_vbox), preview->frame, FALSE, FALSE, 0);
  main_function (drawable, TRUE);
  gtk_widget_show (preview->widget);
  
  frame = gimp_radio_group_new2 (TRUE, _("Parameter Settings"),
				 G_CALLBACK (radio_callback),
				 &pvals.max_p,
                                 GINT_TO_POINTER (pvals.max_p),

				 _("_Hold the Maximal Channels"),
				 GINT_TO_POINTER (MAX_CHANNELS), &max,

				 _("Ho_ld the Minimal Channels"),
				 GINT_TO_POINTER (MIN_CHANNELS), &min,

				 NULL);

  g_object_set_data (G_OBJECT (max), "drawable", drawable);
  g_object_set_data (G_OBJECT (min), "drawable", drawable);

  gtk_container_add (GTK_CONTAINER (main_vbox), frame);
  gtk_widget_show (frame);

  gtk_widget_show (dlg);

  gtk_main ();
  gdk_flush ();

  return interface.run;
}

static void
radio_callback (GtkWidget *widget, 
		gpointer  data)
{
  gimp_radio_button_update (widget, data);

  if (GTK_TOGGLE_BUTTON (widget)->active)
    {
      GimpDrawable *drawable;
      drawable = g_object_get_data (G_OBJECT (widget), "drawable");
      main_function (drawable, TRUE);
    }
}

static void
ok_callback (GtkWidget *widget,
	     gpointer   data)
{
  interface.run = TRUE;

  gtk_widget_destroy (GTK_WIDGET (data));
}
