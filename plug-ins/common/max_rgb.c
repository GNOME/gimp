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

#include <gtk/gtk.h>

#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

#include "libgimp/stdplugins-intl.h"


/* Replace them with the right ones */
#define	PLUG_IN_NAME	     "plug_in_max_rgb"
#define SHORT_NAME	     "max_rgb"
#define PROGRESS_UPDATE_NUM  100
#define PREVIEW_SIZE         128 

static void	query	(void);
static void	run	(gchar  *name,
			 gint    nparams,
			 GParam *param,
			 gint   *nreturn_vals,
			 GParam **return_vals);

static void        fill_preview_with_thumb (GtkWidget *preview_widget, 
					    gint32     drawable_id);
static GtkWidget  *preview_widget          (GDrawable *drawable);

static GStatusType main_function  (GDrawable *drawable, 
				   gboolean   preview_mode);

static gint	   dialog         (GDrawable *drawable);
static void        ok_callback    (GtkWidget *widget,
				   gpointer   data);
static void        radio_callback (GtkWidget *widget, 
				   gpointer   data);


GPlugInInfo PLUG_IN_INFO =
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

static guchar *preview_bits;
static GtkWidget *preview;

MAIN ()

static void
query (void)
{
  static GParamDef args [] =
  {
    { PARAM_INT32,    "run_mode", "Interactive, non-interactive"},
    { PARAM_IMAGE,    "image",    "Input image (not used)"},
    { PARAM_DRAWABLE, "drawable", "Input drawable"},
    { PARAM_INT32,    "max_p",    "1 for maximizing, 0 for minimizing"}
  };
  static gint nargs = sizeof (args) / sizeof (args[0]);

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
  GDrawable    *drawable;
  static GParam	values[1];
  GStatusType	status = STATUS_EXECUTION_ERROR;
  GRunModeType	run_mode;
  
  run_mode = param[0].data.d_int32;
  drawable = gimp_drawable_get (param[2].data.d_drawable);

  *nreturn_vals = 1;
  *return_vals = values;
  
  values[0].type = PARAM_STATUS;
  values[0].data.d_status = status;

  switch (run_mode)
    {
    case RUN_INTERACTIVE:
      INIT_I18N_UI();
      gimp_get_data (PLUG_IN_NAME, &pvals);
      /* Since a channel might be selected, we must check wheter RGB or not. */
      if (!gimp_drawable_is_rgb (drawable->id))
	{
	  g_message (_("Max RGB: Can only operate on RGB drawables."));
	  return;
	}
      if (! dialog (drawable))
	return;
      break;
    case RUN_NONINTERACTIVE:
      INIT_I18N();
      /* You must copy the values of parameters to pvals or dialog variables. */
      break;
    case RUN_WITH_LAST_VALS:
      INIT_I18N();
      gimp_get_data (PLUG_IN_NAME, &pvals);
      break;
    }
  
  status = main_function (drawable, FALSE);

  if (run_mode != RUN_NONINTERACTIVE)
    gimp_displays_flush ();
  if (run_mode == RUN_INTERACTIVE && status == STATUS_SUCCESS)
    gimp_set_data (PLUG_IN_NAME, &pvals, sizeof (ValueType));
    g_free(preview_bits);

  values[0].type = PARAM_STATUS;
  values[0].data.d_status = status;
}

static GStatusType
main_function (GDrawable *drawable, 
	       gboolean   preview_mode)
{
  GPixelRgn  src_rgn, dest_rgn;
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
      x2 = GTK_PREVIEW (preview)->buffer_width;
      y2 = GTK_PREVIEW (preview)->buffer_height;
      gap = 0; /* no alpha on preview */
      bpp = GTK_PREVIEW (preview)->bpp;
    } 
  else 
    {
      gap = (gimp_drawable_has_alpha (drawable->id)) ? 1 : 0;
      gimp_drawable_mask_bounds (drawable->id, &x1, &y1, &x2, &y2);
      gimp_tile_cache_ntiles (2 * (drawable->width / gimp_tile_width () + 1));
      gimp_pixel_rgn_init (&src_rgn, drawable,
			   x1, y1, (x2 - x1), (y2 - y1), FALSE, FALSE);
      gimp_pixel_rgn_init (&dest_rgn, drawable,
			   x1, y1, (x2 - x1), (y2 - y1), TRUE, TRUE);
      pr = gimp_pixel_rgns_register (2, &src_rgn, &dest_rgn);
      gimp_progress_init ( _("Max RGB: Scanning..."));
    }
  
  total = (x2 - x1) * (y2 - y1);

 if (preview_mode) 
   { /* preview mode.  here we go again. see nova.c 
	I just don't want to write a prev_pixel_rgn_process
	and then find out someone else coded a much cooler
	preview widget / functions for GIMP */
     src_data = g_malloc (x2 * y2 * bpp);
     memcpy (src_data, preview_bits, x2 * y2 * bpp);
     dest_data = g_malloc (x2 * y2 * bpp);
     save_dest = dest_data;
     
     for (y = 0; y < y2; y++)
       {
	 src = src_data + y * x2 * bpp;
	 dest = dest_data + y * x2 * bpp;
	 
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
	       *dest++=*src++;
	    }
	}

   memcpy (GTK_PREVIEW (preview)->buffer, save_dest, x2 * y2 * bpp);
   gtk_widget_queue_draw (preview);
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
		 
		 if ((++processed % (total / PROGRESS_UPDATE_NUM)) == 0)
		   gimp_progress_update ((gdouble) processed / (gdouble) total); 
	       }
	   }
       }
     gimp_progress_update (1.0);
     gimp_drawable_flush (drawable);
     gimp_drawable_merge_shadow (drawable->id, TRUE);
     gimp_drawable_update (drawable->id, x1, y1, (x2 - x1), (y2 - y1));
     gimp_drawable_detach (drawable);
   }
 
 return STATUS_SUCCESS;
}
 

/* dialog stuff */
static gint
dialog (GDrawable *drawable)
{
  GtkWidget *dlg;
  GtkWidget *main_vbox;
  GtkWidget *abox;
  GtkWidget *frame;
  GtkWidget *max;
  GtkWidget *min;

  gimp_ui_init ("max_rgb", TRUE);

  dlg = gimp_dialog_new (_("Max RGB"), "max_rgb",
			 gimp_standard_help_func, "filters/max_rgb.html",
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

  main_vbox = gtk_vbox_new (FALSE, 2);
  gtk_container_set_border_width (GTK_CONTAINER (main_vbox), 0);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dlg)->vbox), main_vbox, TRUE, TRUE, 0);
  gtk_widget_show (main_vbox);

  frame = gtk_frame_new (_("Preview"));
  gtk_container_set_border_width (GTK_CONTAINER (frame), 6);
  gtk_box_pack_start (GTK_BOX (main_vbox), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);
  abox = gtk_alignment_new (0.5, 0.5, 0.0, 0.0);
  gtk_container_set_border_width (GTK_CONTAINER (abox), 4);
  gtk_container_add (GTK_CONTAINER (frame), abox);
  gtk_widget_show (abox);
  frame = gtk_frame_new (NULL);
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_IN);
  gtk_container_add (GTK_CONTAINER (abox), frame);
  gtk_widget_show (frame);
  preview = preview_widget (drawable);
  gtk_container_add (GTK_CONTAINER (frame), preview);
  main_function (drawable, TRUE);
  gtk_widget_show (preview);
  
  frame = gimp_radio_group_new2 (TRUE, _("Parameter Settings"),
				 radio_callback,
				 &pvals.max_p, (gpointer) pvals.max_p,

				 _("Hold the Maximal Channels"),
				 (gpointer) MAX_CHANNELS, &max,
				 _("Hold the Minimal Channels"),
				 (gpointer) MIN_CHANNELS, &min,

				 NULL);
  gtk_object_set_data (GTK_OBJECT (max), "drawable", drawable);
  gtk_object_set_data (GTK_OBJECT (min), "drawable", drawable);

  gtk_container_set_border_width (GTK_CONTAINER (frame), 6);
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
  GDrawable *drawable;

  gimp_radio_button_update (widget, data);
  drawable = gtk_object_get_data (GTK_OBJECT (widget), "drawable");
  main_function (drawable, TRUE);
}

static void
ok_callback (GtkWidget *widget,
	     gpointer   data)
{
  interface.run = TRUE;

  gtk_widget_destroy (GTK_WIDGET (data));
}

static GtkWidget *
preview_widget (GDrawable *drawable)
{
  gint       size;
  GtkWidget *preview;

  preview = gtk_preview_new (GTK_PREVIEW_COLOR);
  fill_preview_with_thumb (preview, drawable->id);
  size = (GTK_PREVIEW (preview)->buffer_width) * 
	 (GTK_PREVIEW (preview)->buffer_height) * 
	 (GTK_PREVIEW (preview)->bpp);
  preview_bits = g_malloc (size);
  memcpy (preview_bits, GTK_PREVIEW (preview)->buffer, size);

  return preview;
}

static void
fill_preview_with_thumb (GtkWidget *widget, 
			 gint32     drawable_ID)
{
  guchar  *drawable_data;
  gint     bpp;
  gint     x,y;
  gint     width  = PREVIEW_SIZE;
  gint     height = PREVIEW_SIZE;
  guchar  *src;
  gdouble  r, g, b, a;
  gdouble  c0, c1;
  guchar  *p0, *p1;
  guchar  *even, *odd;

  bpp = 0; /* Only returned */
  
  drawable_data = 
    gimp_drawable_get_thumbnail_data (drawable_ID, &width, &height, &bpp);

  if (width < 1 || height < 1)
    return;

  gtk_preview_size (GTK_PREVIEW (widget), width, height);

  even = g_malloc (width * 3);
  odd  = g_malloc (width * 3);
  src = drawable_data;

  for (y = 0; y < height; y++)
    {
      p0 = even;
      p1 = odd;
      
      for (x = 0; x < width; x++) 
	{
	  if(bpp == 4)
	    {
	      r =  ((gdouble)src[x*4+0])/255.0;
	      g = ((gdouble)src[x*4+1])/255.0;
	      b = ((gdouble)src[x*4+2])/255.0;
	      a = ((gdouble)src[x*4+3])/255.0;
	    }
	  else if(bpp == 3)
	    {
	      r =  ((gdouble)src[x*3+0])/255.0;
	      g = ((gdouble)src[x*3+1])/255.0;
	      b = ((gdouble)src[x*3+2])/255.0;
	      a = 1.0;
	    }
	  else
	    {
	      r = ((gdouble)src[x*bpp+0])/255.0;
	      g = b = r;
	      if(bpp == 2)
		a = ((gdouble)src[x*bpp+1])/255.0;
	      else
		a = 1.0;
	    }
	  
	  if ((x / GIMP_CHECK_SIZE_SM) & 1) 
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
      
      if ((y / GIMP_CHECK_SIZE_SM) & 1)
	gtk_preview_draw_row (GTK_PREVIEW (widget), (guchar *)odd,  0, y, width);
      else
	gtk_preview_draw_row (GTK_PREVIEW (widget), (guchar *)even, 0, y, width);

      src += width * bpp;
    }

  g_free (even);
  g_free (odd);
  g_free (drawable_data);
}
