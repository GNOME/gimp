/*
 * This is a plugin for the GIMP.
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

/*
 * This filter adds random noise to an image.
 * The amount of noise can be set individually for each RGB channel.
 * This filter does not operate on indexed images.
 *
 * May 2000 tim copperfield [timecop@japan.co.jp]
 * Added dynamic preview.
 *
 * alt@gimp.org. Fixed previews so they handle alpha channels correctly.
 */

#include "config.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include <gtk/gtk.h>

#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

#include "libgimp/stdplugins-intl.h"


#define SCALE_WIDTH      125
#define TILE_CACHE_SIZE  16
#define PREVIEW_SIZE     128 

typedef struct
{
  gint    independent;
  gdouble noise[4];     /*  per channel  */
} NoisifyVals;

typedef struct
{
  gint       channels;
  GtkObject *channel_adj[4];
  gint       run;
} NoisifyInterface;

/* Declare local functions.
 */
static void       query  (void);
static void       run    (gchar     *name,
			  gint       nparams,
			  GimpParam    *param,
			  gint      *nreturn_vals,
			  GimpParam   **return_vals);

static void       noisify (GimpDrawable *drawable, gboolean preview_mode);
static gdouble    gauss   (void);

static void       fill_preview   (GtkWidget *preview_widget, 
				  GimpDrawable *drawable);
static GtkWidget *preview_widget (GimpDrawable *drawable);

static gint       noisify_dialog                   (GimpDrawable *drawable, 
						    gint           channels);
static void       noisify_ok_callback              (GtkWidget     *widget,
						    gpointer       data);
static void       noisify_double_adjustment_update (GtkAdjustment *adjustment,
						    gpointer       data);

GimpPlugInInfo PLUG_IN_INFO =
{
  NULL,  /* init_proc */
  NULL,  /* quit_proc */
  query, /* query_proc */
  run,   /* run_proc */
};

static NoisifyVals nvals =
{
  TRUE,
  { 0.20, 0.20, 0.20, 0.20 }
};

static NoisifyInterface noise_int =
{
  0,
  { NULL, NULL, NULL, NULL },
  FALSE     /* run */
};

static GtkWidget *preview;
static guchar    *preview_cache;
static gint       preview_cache_rowstride;
static gint       preview_cache_bpp;


MAIN ()

static void
query (void)
{
  static GimpParamDef args[] =
  {
    { GIMP_PDB_INT32, "run_mode", "Interactive, non-interactive" },
    { GIMP_PDB_IMAGE, "image", "Input image (unused)" },
    { GIMP_PDB_DRAWABLE, "drawable", "Input drawable" },
    { GIMP_PDB_INT32, "independent", "Noise in channels independent" },
    { GIMP_PDB_FLOAT, "noise_1", "Noise in the first channel (red, gray)" },
    { GIMP_PDB_FLOAT, "noise_2", "Noise in the second channel (green, gray_alpha)" },
    { GIMP_PDB_FLOAT, "noise_3", "Noise in the third channel (blue)" },
    { GIMP_PDB_FLOAT, "noise_4", "Noise in the fourth channel (alpha)" }
  };
  static gint nargs = sizeof (args) / sizeof (args[0]);

  gimp_install_procedure ("plug_in_noisify",
			  "Adds random noise to a drawable's channels",
			  "More here later",
			  "Torsten Martinsen",
			  "Torsten Martinsen",
			  "May 2000",
			  N_("<Image>/Filters/Noise/Noisify..."),
			  "RGB*, GRAY*",
			  GIMP_PLUGIN,
			  nargs, 0,
			  args, NULL);
}

static void
run (gchar   *name,
     gint     nparams,
     GimpParam  *param,
     gint    *nreturn_vals,
     GimpParam **return_vals)
{
  static GimpParam values[1];
  GimpDrawable *drawable;
  GimpRunModeType run_mode;
  GimpPDBStatusType status = GIMP_PDB_SUCCESS;

  run_mode = param[0].data.d_int32;

  *nreturn_vals = 1;
  *return_vals = values;

  values[0].type = GIMP_PDB_STATUS;
  values[0].data.d_status = status;

  /*  Get the specified drawable  */
  drawable = gimp_drawable_get (param[2].data.d_drawable);

  switch (run_mode)
    {
    case GIMP_RUN_INTERACTIVE:
      INIT_I18N_UI();
      /*  Possibly retrieve data  */
      gimp_get_data ("plug_in_noisify", &nvals);

      /*  First acquire information with a dialog  */
      if (! noisify_dialog (drawable, drawable->bpp))
	{
	  gimp_drawable_detach (drawable);
	  return;
	}
      break;

    case GIMP_RUN_NONINTERACTIVE:
      INIT_I18N();
      /*  Make sure all the arguments are there!  */
      if (nparams != 8)
	{
	  status = GIMP_PDB_CALLING_ERROR;
	}
      else
	{
	  nvals.independent = param[3].data.d_int32 ? TRUE : FALSE;
	  nvals.noise[0]    = param[4].data.d_float;
	  nvals.noise[1]    = param[5].data.d_float;
	  nvals.noise[2]    = param[6].data.d_float;
	  nvals.noise[3]    = param[7].data.d_float;
	}
      break;

    case GIMP_RUN_WITH_LAST_VALS:
      INIT_I18N();
      /*  Possibly retrieve data  */
      gimp_get_data ("plug_in_noisify", &nvals);
      break;

    default:
      break;
    }

  /*  Make sure that the drawable is gray or RGB color  */
  if (gimp_drawable_is_rgb (drawable->id) ||
      gimp_drawable_is_gray (drawable->id))
    {
      gimp_progress_init (_("Adding Noise..."));
      gimp_tile_cache_ntiles (TILE_CACHE_SIZE);

      /*  seed the random number generator  */
      srand (time (NULL));

      /*  compute the luminosity which exceeds the luminosity threshold  */
      noisify (drawable, FALSE);

      if (run_mode != GIMP_RUN_NONINTERACTIVE)
	gimp_displays_flush ();

      /*  Store data  */
      if (run_mode == GIMP_RUN_INTERACTIVE) {
	gimp_set_data ("plug_in_noisify", &nvals, sizeof (NoisifyVals));
	g_free(preview_cache);
      }
    }
  else
    {
      /* gimp_message ("blur: cannot operate on indexed color images"); ??? BLUR ??? */
      status = GIMP_PDB_EXECUTION_ERROR;
    }

  values[0].data.d_status = status;

  gimp_drawable_detach (drawable);
}

static void
preview_do_row(gint    row,
	       gint    width,
	       guchar *even,
	       guchar *odd,
	       guchar *src)
{
  gint    x;
  
  guchar *p0 = even;
  guchar *p1 = odd;
  
  gdouble    r, g, b, a;
  gdouble    c0, c1;
  
  for (x = 0; x < width; x++) 
    {
      if (preview_cache_bpp == 4)
	{
	  r = ((gdouble)src[x*4+0]) / 255.0;
	  g = ((gdouble)src[x*4+1]) / 255.0;
	  b = ((gdouble)src[x*4+2]) / 255.0;
	  a = ((gdouble)src[x*4+3]) / 255.0;
	}
      else if (preview_cache_bpp == 3)
	{
	  r = ((gdouble)src[x*3+0]) / 255.0;
	  g = ((gdouble)src[x*3+1]) / 255.0;
	  b = ((gdouble)src[x*3+2]) / 255.0;
	  a = 1.0;
	}
      else
	{
	  r = ((gdouble)src[x*preview_cache_bpp+0]) / 255.0;
	  g = b = r;
	  if (preview_cache_bpp == 2)
		    a = ((gdouble)src[x*preview_cache_bpp+1]) / 255.0;
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
  
  if ((row / GIMP_CHECK_SIZE) & 1)
    {
      gtk_preview_draw_row (GTK_PREVIEW (preview), (guchar *)odd,  0, row, width); 
    }
  else
    {
      gtk_preview_draw_row (GTK_PREVIEW (preview), (guchar *)even, 0, row, width); 
    }
}

static void
noisify (GimpDrawable *drawable, 
	 gboolean   preview_mode)
{
  GimpPixelRgn src_rgn, dest_rgn;
  guchar *src_row, *dest_row;
  guchar *src, *dest, *dest_data;
  gint row, col, b;
  gint x1, y1, x2, y2, p, bpp = 3;
  gint noise;
  gint progress = 0, max_progress = 0;
  gpointer pr;
  gint row_stride = 0;
  guchar *odd = NULL;
  guchar *even = NULL;
  /* initialize */

  noise = 0;

  if (preview_mode) 
    {
      x1 = y1 = 0;
      x2 = GTK_PREVIEW (preview)->buffer_width;
      y2 = GTK_PREVIEW (preview)->buffer_height;
      bpp = preview_cache_bpp;
      row_stride = preview_cache_rowstride;
      even = g_malloc (x2 * 3);
      odd  = g_malloc (x2 * 3);
    } 
  else 
    {
      gimp_drawable_mask_bounds (drawable->id, &x1, &y1, &x2, &y2);
      gimp_pixel_rgn_init (&src_rgn, drawable,
			   x1, y1, (x2 - x1), (y2 - y1), FALSE, FALSE);
      gimp_pixel_rgn_init (&dest_rgn, drawable,
			   x1, y1, (x2 - x1), (y2 - y1), TRUE, TRUE);
      /* Initialize progress */
      progress = 0;
      max_progress = (x2 - x1) * (y2 - y1);
    }

  if (preview_mode) 
    {
      dest_data = g_malloc (row_stride * y2);

      for (row = 0; row < y2; row++)
	{
	  src  = preview_cache + row * row_stride;
	  dest = dest_data + row * row_stride;
	  
	  for (col = 0; col < x2; col++)
	    {
	      if (nvals.independent == FALSE)
		noise = (gint) (nvals.noise[0] * gauss() * 127);

	      for (b = 0; b < bpp; b++)
		{
		  if (nvals.independent == TRUE)
		    noise = (gint) (nvals.noise[b] * gauss() * 127);

		  if (nvals.noise[b] > 0.0)
		    {
		      p = src[b] + noise;
		      if (p < 0)
		        p = 0;
		      else if (p > 255)
		        p = 255;
		      dest[b] = p;
		    }
		  else
		    dest[b] = src[b];

		}
	      src += bpp;
	      dest += bpp;
	    }
	}

      for (row = 0; row < y2; row++)
	{
	  preview_do_row(row,x2,even,odd,dest_data + row * row_stride);
	}

      gtk_widget_queue_draw (preview);

      if(even)
	g_free(even);
      
      if(odd)
	g_free(odd);

    } 
  else 
    {
      for (pr = gimp_pixel_rgns_register (2, &src_rgn, &dest_rgn);
	   pr != NULL;
	   pr = gimp_pixel_rgns_process (pr))
	{
	  src_row = src_rgn.data;
	  dest_row = dest_rgn.data;

	  for (row = 0; row < src_rgn.h; row++)
	    {
	      src = src_row;
	      dest = dest_row;
	      
	      for (col = 0; col < src_rgn.w; col++)
		{
		  if (nvals.independent == FALSE)
		    noise = (gint) (nvals.noise[0] * gauss() * 127);
		  
		  for (b = 0; b < src_rgn.bpp; b++)
		    {
		      if (nvals.independent == TRUE)
			noise = (gint) (nvals.noise[b] * gauss() * 127);
		      
		      if (nvals.noise[b] > 0.0)
			{
			  p = src[b] + noise;
			  if (p < 0)
			    p = 0;
			  else if (p > 255)
			    p = 255;
			  dest[b] = p;
			}
		      else
			dest[b] = src[b];
		      
		    }
		  src += src_rgn.bpp;
		  dest += dest_rgn.bpp;
		}
	      
	      src_row += src_rgn.rowstride;
	      dest_row += dest_rgn.rowstride;
	    }
	  
	  /* Update progress */
	  progress += src_rgn.w * src_rgn.h;
	  gimp_progress_update ((double) progress / (double) max_progress);
	}
 
      /*  update the blurred region  */
      gimp_drawable_flush (drawable);
      gimp_drawable_merge_shadow (drawable->id, TRUE);
      gimp_drawable_update (drawable->id, x1, y1, (x2 - x1), (y2 - y1));

    } /* endif normal mode */
}

static gint
noisify_dialog (GimpDrawable *drawable, 
		gint       channels)
{
  GtkWidget *dlg;
  GtkWidget *main_vbox;
  GtkWidget *abox;
  GtkWidget *toggle;
  GtkWidget *frame;
  GtkWidget *table;
  GtkObject *adj;
  gchar *buffer;
  gint   i;

  gimp_ui_init ("noisify", FALSE);

  dlg = gimp_dialog_new (_("Noisify"), "noisify",
			 gimp_standard_help_func, "filters/noisify.html",
			 GTK_WIN_POS_MOUSE,
			 FALSE, TRUE, FALSE,

			 _("OK"), noisify_ok_callback,
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

  /* preview */
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
  noisify (drawable, TRUE); /* preview noisify */
  gtk_widget_show (preview);
  
  /*  parameter settings  */
  frame = gtk_frame_new (_("Parameter Settings"));
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_ETCHED_IN);
  gtk_container_set_border_width (GTK_CONTAINER (frame), 6);
  gtk_box_pack_start (GTK_BOX (main_vbox), frame, TRUE, TRUE, 0);

  table = gtk_table_new (channels + 1, 3, FALSE);
  gtk_table_set_col_spacings (GTK_TABLE (table), 4);
  gtk_table_set_row_spacings (GTK_TABLE (table), 2);
  gtk_table_set_row_spacing (GTK_TABLE (table), 0, 4);
  gtk_container_set_border_width (GTK_CONTAINER (table), 4);
  gtk_container_add (GTK_CONTAINER (frame), table);

  toggle = gtk_check_button_new_with_label (_("Independent"));
  gtk_table_attach (GTK_TABLE (table), toggle, 0, 3, 0, 1, GTK_FILL, 0, 0, 0);
  gtk_signal_connect (GTK_OBJECT (toggle), "toggled",
		      GTK_SIGNAL_FUNC (gimp_toggle_button_update),
		      &nvals.independent);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (toggle), nvals.independent);
  gtk_widget_show (toggle);

  noise_int.channels = channels;

  if (channels == 1) 
    {
      adj = gimp_scale_entry_new (GTK_TABLE (table), 0, 1,
				  _("Gray:"), SCALE_WIDTH, 0,
				  nvals.noise[0], 0.0, 1.0, 0.01, 0.1, 2,
				  TRUE, 0, 0,
				  NULL, NULL);

      gtk_object_set_data (GTK_OBJECT (adj), "drawable", drawable);
      gtk_signal_connect (GTK_OBJECT (adj), "value_changed",
			  GTK_SIGNAL_FUNC (noisify_double_adjustment_update),
			  &nvals.noise[0]);
      noise_int.channel_adj[0] = adj;
    }
  else if (channels == 2)
    {
      adj = gimp_scale_entry_new (GTK_TABLE (table), 0, 1,
				  _("Gray:"), SCALE_WIDTH, 0,
				  nvals.noise[0], 0.0, 1.0, 0.01, 0.1, 2,
				  TRUE, 0, 0,
				  NULL, NULL);
      gtk_object_set_data (GTK_OBJECT (adj), "drawable", drawable);
      gtk_signal_connect (GTK_OBJECT (adj), "value_changed",
			  GTK_SIGNAL_FUNC (noisify_double_adjustment_update),
			  &nvals.noise[0]);
      noise_int.channel_adj[0] = adj;

      adj = gimp_scale_entry_new (GTK_TABLE (table), 0, 2,
				  _("Alpha:"), SCALE_WIDTH, 0,
				  nvals.noise[1], 0.0, 1.0, 0.01, 0.1, 2,
				  TRUE, 0, 0,
				  NULL, NULL);
      gtk_object_set_data (GTK_OBJECT (adj), "drawable", drawable);
      gtk_signal_connect (GTK_OBJECT (adj), "value_changed",
			  GTK_SIGNAL_FUNC (noisify_double_adjustment_update),
			  &nvals.noise[1]);
      noise_int.channel_adj[1] = adj;
    }
  
  else if (channels == 3)
    {
      adj = gimp_scale_entry_new (GTK_TABLE (table), 0, 1,
				  _("Red:"), SCALE_WIDTH, 0,
				  nvals.noise[0], 0.0, 1.0, 0.01, 0.1, 2,
				  TRUE, 0, 0,
				  NULL, NULL);
      gtk_object_set_data (GTK_OBJECT (adj), "drawable", drawable);
      gtk_signal_connect (GTK_OBJECT (adj), "value_changed",
			  GTK_SIGNAL_FUNC (noisify_double_adjustment_update),
			  &nvals.noise[0]);
      noise_int.channel_adj[0] = adj;

      adj = gimp_scale_entry_new (GTK_TABLE (table), 0, 2,
				  _("Green:"), SCALE_WIDTH, 0,
				  nvals.noise[1], 0.0, 1.0, 0.01, 0.1, 2,
				  TRUE, 0, 0,
				  NULL, NULL);
      gtk_object_set_data (GTK_OBJECT (adj), "drawable", drawable);
      gtk_signal_connect (GTK_OBJECT (adj), "value_changed",
			  GTK_SIGNAL_FUNC (noisify_double_adjustment_update),
			  &nvals.noise[1]);
      noise_int.channel_adj[1] = adj;

      adj = gimp_scale_entry_new (GTK_TABLE (table), 0, 3,
				  _("Blue:"), SCALE_WIDTH, 0,
				  nvals.noise[2], 0.0, 1.0, 0.01, 0.1, 2,
				  TRUE, 0, 0,
				  NULL, NULL);
      gtk_object_set_data (GTK_OBJECT (adj), "drawable", drawable);
      gtk_signal_connect (GTK_OBJECT (adj), "value_changed",
			  GTK_SIGNAL_FUNC (noisify_double_adjustment_update),
			  &nvals.noise[2]);
      noise_int.channel_adj[2] = adj;
    }

  else if (channels == 4)
    {
      adj = gimp_scale_entry_new (GTK_TABLE (table), 0, 1,
				  _("Red:"), SCALE_WIDTH, 0,
				  nvals.noise[0], 0.0, 1.0, 0.01, 0.1, 2,
				  TRUE, 0, 0,
				  NULL, NULL);
      gtk_object_set_data (GTK_OBJECT (adj), "drawable", drawable);
      gtk_signal_connect (GTK_OBJECT (adj), "value_changed",
			  GTK_SIGNAL_FUNC (noisify_double_adjustment_update),
			  &nvals.noise[0]);
      noise_int.channel_adj[0] = adj;

      adj = gimp_scale_entry_new (GTK_TABLE (table), 0, 2,
				  _("Green:"), SCALE_WIDTH, 0,
				  nvals.noise[1], 0.0, 1.0, 0.01, 0.1, 2,
				  TRUE, 0, 0,
				  NULL, NULL);
      gtk_object_set_data (GTK_OBJECT (adj), "drawable", drawable);
      gtk_signal_connect (GTK_OBJECT (adj), "value_changed",
			  GTK_SIGNAL_FUNC (noisify_double_adjustment_update),
			  &nvals.noise[1]);
      noise_int.channel_adj[1] = adj;

      adj = gimp_scale_entry_new (GTK_TABLE (table), 0, 3,
				  _("Blue:"), SCALE_WIDTH, 0,
				  nvals.noise[2], 0.0, 1.0, 0.01, 0.1, 2,
				  TRUE, 0, 0,
				  NULL, NULL);
      gtk_object_set_data (GTK_OBJECT (adj), "drawable", drawable);
      gtk_signal_connect (GTK_OBJECT (adj), "value_changed",
			  GTK_SIGNAL_FUNC (noisify_double_adjustment_update),
			  &nvals.noise[2]);
      noise_int.channel_adj[2] = adj;

      adj = gimp_scale_entry_new (GTK_TABLE (table), 0, 4,
				  _("Alpha:"), SCALE_WIDTH, 0,
				  nvals.noise[3], 0.0, 1.0, 0.01, 0.1, 2,
				  TRUE, 0, 0,
				  NULL, NULL);
      gtk_object_set_data (GTK_OBJECT (adj), "drawable", drawable);
      gtk_signal_connect (GTK_OBJECT (adj), "value_changed",
			  GTK_SIGNAL_FUNC (noisify_double_adjustment_update),
			  &nvals.noise[3]);
      noise_int.channel_adj[3] = adj;
    }
  else
    {
      for (i = 0; i < channels; i++)
	{
	  buffer = g_strdup_printf (_("Channel #%d:"), i);

	  adj = gimp_scale_entry_new (GTK_TABLE (table), 0, i + 1,
				      buffer, SCALE_WIDTH, 0,
				      nvals.noise[i], 0.0, 1.0, 0.01, 0.1, 2,
				      TRUE, 0, 0,
				      NULL, NULL);
          gtk_object_set_data (GTK_OBJECT (adj), "drawable", drawable);
	  gtk_signal_connect (GTK_OBJECT (adj), "value_changed",
			      GTK_SIGNAL_FUNC (noisify_double_adjustment_update),
			      &nvals.noise[i]);
	  noise_int.channel_adj[i] = adj;

	  g_free (buffer);
	}
    }

  gtk_widget_show (frame);
  gtk_widget_show (table);

  gtk_widget_show (dlg);

  gtk_main ();
  gdk_flush ();

  return noise_int.run;
}

/*
 * Return a Gaussian (aka normal) random variable.
 *
 * Adapted from ppmforge.c, which is part of PBMPLUS.
 * The algorithm comes from:
 * 'The Science Of Fractal Images'. Peitgen, H.-O., and Saupe, D. eds.
 * Springer Verlag, New York, 1988.
 */
static gdouble
gauss (void)
{
  gint i;
  gdouble sum = 0.0;

  for (i = 0; i < 4; i++)
    sum += rand () & 0x7FFF;

  return sum * 5.28596089837e-5 - 3.46410161514;
}

static void
noisify_ok_callback (GtkWidget *widget,
		     gpointer   data)
{
  noise_int.run = TRUE;

  gtk_widget_destroy (GTK_WIDGET (data));
}

static void
noisify_double_adjustment_update (GtkAdjustment *adjustment,
				  gpointer       data)
{
  GimpDrawable *drawable;
  
  gimp_double_adjustment_update (adjustment, data);
  drawable = gtk_object_get_data (GTK_OBJECT (adjustment), "drawable");
  noisify (drawable, TRUE);

  if (! nvals.independent)
    {
      gint i;

      for (i = 0; i < noise_int.channels; i++)
	if (adjustment != GTK_ADJUSTMENT (noise_int.channel_adj[i]))
	  gtk_adjustment_set_value (GTK_ADJUSTMENT (noise_int.channel_adj[i]),
				    adjustment->value);
    }
}

static GtkWidget *
preview_widget (GimpDrawable *drawable)
{
  gint       size;

  preview = gtk_preview_new (GTK_PREVIEW_COLOR);
  fill_preview (preview, drawable);
  size = GTK_PREVIEW (preview)->rowstride * GTK_PREVIEW (preview)->buffer_height;
  return preview;
}

static void
fill_preview (GtkWidget *widget, 
	      GimpDrawable *drawable)
{
  GimpPixelRgn  srcPR;
  gint       width;
  gint       height;
  gint       x1, x2, y1, y2;
  gint       bpp;
  gint       y;
  guchar    *src;
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
  preview_cache = g_malloc(width * bpp * height);
  preview_cache_rowstride = width * bpp;
  preview_cache_bpp = bpp;


  for (y = 0; y < height; y++)
    {
      gimp_pixel_rgn_get_row (&srcPR, src, x1, y + y1, width);
      memcpy(preview_cache + (y*width*bpp),src,width*bpp);
    }

  for (y = 0; y < height; y++)
    {
      preview_do_row(y,width,even,odd,preview_cache + (y*width*bpp));
    }

  g_free (even);
  g_free (odd);
  g_free (src);
}
