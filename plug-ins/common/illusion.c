/*
 * illusion.c  -- This is a plug-in for the GIMP 1.0
 *                                                                              
 * Copyright (C) 1997  Hirotsuna Mizuno
 *                     s1041150@u-aizu.ac.jp
 *
 * Preview and new mode added May 2000 by tim copperfield
 * 		       timecop@japan.co.jp
 * 		       http://www.ne.jp/asahi/linux/timecop
 *                                                                              
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 *                                                                              
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *                                                                              
 * You should have received a copy of the GNU General Public License along with
 * this program; if not, write to the Free Software Foundation, Inc.,
 * 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 */

#include "config.h"

#include <stdio.h>
#include <stdlib.h>

#include <gtk/gtk.h>

#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

#include "libgimp/stdplugins-intl.h"


#define PLUG_IN_NAME    "plug_in_illusion"
#define PLUG_IN_VERSION "v0.8 (May 14 2000)"

#define PREVIEW_SIZE    128 

/******************************************************************************/

static void      query  (void);
static void      run    (gchar   *name,
			 gint     nparam,
			 GimpParam  *param,
			 gint    *nreturn_vals,
			 GimpParam **return_vals);

static void      filter                  (GimpDrawable *drawable);
static void      filter_preview          (void);
static void      fill_preview_with_thumb (GtkWidget *preview_widget, 
					  gint32     drawable_ID);
static gboolean  dialog                  (GimpDrawable *drawable);

/******************************************************************************/

typedef struct
{
  gint32 division;
  gint   type1;
  gint   type2;
} IllValues;

/******************************************************************************/

GimpPlugInInfo PLUG_IN_INFO =
{
  NULL,  /* init_proc  */
  NULL,  /* quit_proc  */
  query, /* query_proc */
  run    /* run_proc   */
};

static IllValues parameters =
{
  8, /* division */
  1, /* type 1 */
  0  /* type 2 */
};


static GtkWidget *preview;
static guchar    *preview_cache;
static gint       preview_cache_rowstride;
static gint       preview_cache_bpp;

/******************************************************************************/

MAIN ()

/******************************************************************************/

static void
query (void)
{
  static GimpParamDef args[] =
  {
    { GIMP_PDB_INT32,    "run_mode",  "interactive / non-interactive" },
    { GIMP_PDB_IMAGE,    "image",     "input image" },
    { GIMP_PDB_DRAWABLE, "drawable",  "input drawable" },
    { GIMP_PDB_INT32,    "division",  "the number of divisions" }
  };
  static gint nargs = sizeof (args) / sizeof (args[0]);;

  gimp_install_procedure (PLUG_IN_NAME,
			  "produce illusion",
			  "produce illusion",
			  "Hirotsuna Mizuno <s1041150@u-aizu.ac.jp>",
			  "Hirotsuna Mizuno",
			  PLUG_IN_VERSION,
			  N_("<Image>/Filters/Map/Illusion..."),
			  "RGB*, GRAY*",
			  GIMP_PLUGIN,
			  nargs, 0,
			  args, NULL);
}

/******************************************************************************/

static void
run (gchar   *name,
     gint     nparams,
     GimpParam  *params,
     gint    *nreturn_vals,
     GimpParam **return_vals)
{
  GimpDrawable     *drawable;
  GimpRunModeType   run_mode;
  static GimpParam  returnv[1];
  GimpPDBStatusType    status = GIMP_PDB_SUCCESS;

  run_mode = params[0].data.d_int32;
  drawable = gimp_drawable_get (params[2].data.d_drawable);
  *nreturn_vals = 1;
  *return_vals  = returnv;

  /* get the drawable info */

  /* switch the run mode */
  switch (run_mode)
    {
    case GIMP_RUN_INTERACTIVE:
      INIT_I18N_UI();
      gimp_get_data (PLUG_IN_NAME, &parameters);
      if (! dialog(drawable))
	return;
      gimp_set_data (PLUG_IN_NAME, &parameters, sizeof (IllValues));
      g_free(preview_cache);
      break;

    case GIMP_RUN_NONINTERACTIVE:
      if (nparams != 6)
	{
	  status = GIMP_PDB_CALLING_ERROR;
	}
      else
	{
	  parameters.division = params[3].data.d_int32;
          parameters.type1 = params[4].data.d_int32;
	  parameters.type2 = params[5].data.d_int32;
	}
      break;

    case GIMP_RUN_WITH_LAST_VALS:
      gimp_get_data (PLUG_IN_NAME, &parameters);
      break;
    }

  if (status == GIMP_PDB_SUCCESS)
    {
      if (gimp_drawable_is_rgb (drawable->id) || 
	  gimp_drawable_is_gray (drawable->id))
	{
	  gimp_tile_cache_ntiles (2 * (drawable->width / gimp_tile_width() + 1));
  	  gimp_progress_init (_("Illusion..."));
	  filter (drawable);
	  if (run_mode != GIMP_RUN_NONINTERACTIVE)
	    gimp_displays_flush ();
	}
      else
	{
	  status = GIMP_PDB_EXECUTION_ERROR;
	}
    }

  returnv[0].type          = GIMP_PDB_STATUS;
  returnv[0].data.d_status = status;

  gimp_drawable_detach (drawable);
}

/******************************************************************************/

static void
filter (GimpDrawable *drawable)
{
  GimpPixelRgn srcPR, destPR;
  guchar  **pixels;
  guchar  **destpixels;
  
  gint    image_width;
  gint    image_height;
  gint    image_bpp;
  gint    image_has_alpha;
  gint    x1;
  gint    y1;
  gint    x2;
  gint    y2;
  gint    select_width;
  gint    select_height;
  gdouble center_x;
  gdouble center_y;

  gint      x, y, b;
  gint      xx = 0;
  gint      yy = 0;
  gdouble   scale, radius, cx, cy, angle, offset;

  image_width     = gimp_drawable_width (drawable->id);
  image_height    = gimp_drawable_height (drawable->id);
  image_bpp       = gimp_drawable_bpp (drawable->id);
  image_has_alpha = gimp_drawable_has_alpha (drawable->id);
  gimp_drawable_mask_bounds (drawable->id,&x1, &y1, &x2, &y2);
  select_width    = x2 - x1;
  select_height   = y2 - y1;
  center_x        = x1 + (gdouble)select_width / 2;
  center_y        = y1 + (gdouble)select_height / 2;
  
  gimp_pixel_rgn_init (&srcPR, drawable,
		       0, 0, image_width, image_height, FALSE, FALSE);
  gimp_pixel_rgn_init (&destPR, drawable,
		       0, 0, image_width, image_height, TRUE, TRUE);

  pixels     = g_new (guchar *, image_height);
  destpixels = g_new (guchar *, image_height);

  for (y = 0; y < image_height; y++)
    {
      pixels[y]     = g_new (guchar, image_width * image_bpp);
      destpixels[y] = g_new (guchar, image_width * image_bpp);
      gimp_pixel_rgn_get_row (&srcPR, pixels[y], 0, y, image_width);
    }

  scale = sqrt (select_width * select_width + select_height * select_height) / 2;
  offset = (gint) (scale / 2);

  for (y = y1; y < y2; y++)
    {
      cy = ((gdouble)y - center_y) / scale; 
      for (x = x1; x < x2; x++)
	{
	  cx = ((gdouble)x - center_x) / scale;
	  angle = floor (atan2 (cy, cx) * parameters.division / G_PI_2)
	    * G_PI_2 / parameters.division + (G_PI / parameters.division);
	  radius = sqrt ((gdouble) (cx * cx + cy * cy));

	  if (parameters.type1) 
	    {
	      xx = x - offset * cos (angle);
	      yy = y - offset * sin (angle);
	    }

	  if (parameters.type2) 
	    {
	      xx = x - offset * sin (angle);
	      yy = y - offset * cos (angle);
	    }

	  if (xx < 0) 
	    xx = 0;
	  else if (image_width <= xx) 
	    xx = image_width - 1;

	  if (yy < 0) 
	    yy = 0;
	  else if (image_height <= yy) 
	    yy = image_height - 1;

	  for (b = 0; b < image_bpp; b++)
	    destpixels[y][x*image_bpp+b] =
	      (1-radius) * pixels[y][x * image_bpp + b] 
	      + radius * pixels[yy][xx * image_bpp + b];
	}
      gimp_pixel_rgn_set_row (&destPR, destpixels[y], 0, y, image_width);
      gimp_progress_update ((double) (y - y1) / (double) select_height);
    }

  gimp_drawable_flush (drawable);
  gimp_drawable_merge_shadow (drawable->id, TRUE);
  gimp_drawable_update (drawable->id,
			x1, y1, select_width, select_height);

  for (y = y1; y < y2; y++) g_free (pixels[y-y1]);
  g_free (pixels);
  for (y = y1; y < y2; y++) g_free (destpixels[y-y1]);
  g_free (destpixels);
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
filter_preview (void)
{
  guchar  **pixels;
  guchar  **destpixels;
  guchar  *even, *odd;
  
  gint      image_width;
  gint      image_height;
  gint      image_bpp;
  gdouble   center_x;
  gdouble   center_y;

  gint      x, y, b;
  gint      xx = 0;
  gint      yy = 0;
  gdouble   scale, radius, cx, cy, angle, offset;

  image_width  = GTK_PREVIEW (preview)->buffer_width;
  image_height = GTK_PREVIEW (preview)->buffer_height;
  image_bpp    = preview_cache_bpp;
  center_x     = (gdouble)image_width  / 2;
  center_y     = (gdouble)image_height / 2;
    
  pixels     = g_new (guchar *, image_height);
  destpixels = g_new (guchar *, image_height);

  even = g_malloc (image_width * 3);
  odd  = g_malloc (image_width * 3);

  for (y = 0; y < image_height; y++)
    {
      pixels[y]     = g_new (guchar, preview_cache_rowstride);
      destpixels[y] = g_new (guchar, preview_cache_rowstride);
      memcpy (pixels[y], 
	      preview_cache + preview_cache_rowstride * y, 
	      preview_cache_rowstride);
    }

  scale  = sqrt (image_width * image_width + image_height * image_height) / 2;
  offset = (gint) (scale / 2);

  for (y = 0; y < image_height; y++)
    {
      cy = ((gdouble)y - center_y) / scale; 
      for (x = 0; x < image_width; x++)
	{
	  cx = ((gdouble)x - center_x) / scale;
	  angle = floor (atan2 (cy, cx) * parameters.division / G_PI_2)
	    * G_PI_2 / parameters.division + (G_PI / parameters.division);
	  radius = sqrt ((gdouble) (cx * cx + cy * cy));

	  if (parameters.type1) 
	    {
	      xx = x - offset * cos (angle);
	      yy = y - offset * sin (angle);
	    }

	  if (parameters.type2) 
	    {
	      xx = x - offset * sin (angle);
	      yy = y - offset * cos (angle);
	    }

	  if (xx < 0) 
	    xx = 0;
	  else if (image_width <= xx) 
	    xx = image_width - 1;

	  if (yy < 0) 
	    yy = 0;
	  else if (image_height <= yy) 
	    yy = image_height - 1;

	  for (b = 0; b < image_bpp; b++)
	    destpixels[y][x*image_bpp+b] =
	      (1-radius) * pixels[y][x * image_bpp + b] 
	      + radius * pixels[yy][xx * image_bpp + b];
	}

      preview_do_row(y,
		     image_width,
		     even,
		     odd,
		     destpixels[y]);
    }

  for (y = 0; y < image_height; y++) 
    g_free (pixels[y]);
  g_free (pixels);

  for (y = 0; y < image_height; y++) 
    g_free (destpixels[y]);
  g_free (destpixels);

  g_free (even);
  g_free (odd);

  gtk_widget_queue_draw (preview);
}

static GtkWidget *
preview_widget (GimpDrawable *drawable)
{
  preview = gtk_preview_new (GTK_PREVIEW_COLOR);
  fill_preview_with_thumb (preview, drawable->id);

  return preview;
}

static void
fill_preview_with_thumb (GtkWidget *widget, 
			 gint32     drawable_ID)
{
  guchar  *drawable_data;
  gint     bpp;
  gint     y;
  gint     width  = PREVIEW_SIZE;
  gint     height = PREVIEW_SIZE;
  guchar  *src;
  guchar  *even, *odd;

  bpp = 0; /* Only returned */
  
  drawable_data = 
    gimp_drawable_get_thumbnail_data (drawable_ID, &width, &height, &bpp);

  if (width < 1 || height < 1)
    return;

  preview_cache = drawable_data;
  preview_cache_rowstride = width * bpp;
  preview_cache_bpp = bpp;

  gtk_preview_size (GTK_PREVIEW (widget), width, height);

  even = g_malloc (width * 3);
  odd  = g_malloc (width * 3);
  src = drawable_data;

  for (y = 0; y < height; y++)
    {
      preview_do_row(y,width,even,odd,preview_cache + (y*width*bpp));
    }

  g_free (even);
  g_free (odd);
}

/******************************************************************************/

static gboolean dialog_status = FALSE;

static void
dialog_ok_handler (GtkWidget *widget,
		   gpointer   data)
{
  dialog_status = TRUE;

  gtk_widget_destroy (GTK_WIDGET (data));
}

/******************************************************************************/

static gboolean
dialog (GimpDrawable *mangle)
{
  GtkWidget *dlg;  
  GtkWidget *main_vbox;
  GtkWidget *abox;
  GtkWidget *frame;
  GtkWidget *table;
  GtkWidget *spinbutton;
  GtkObject *adj;
  GtkWidget *radio;
  GSList    *group = NULL;

  gimp_ui_init ("illusion", TRUE);

  dlg = gimp_dialog_new (_("Illusion"), "illusion",
			 gimp_standard_help_func, "filters/illusion.html",
			 GTK_WIN_POS_MOUSE,
			 FALSE, TRUE, FALSE,
			 
			 _("OK"), dialog_ok_handler,
			 NULL, NULL, NULL, TRUE, FALSE,
			 _("Cancel"), gtk_widget_destroy,
			 NULL, 1, NULL, FALSE, TRUE,
			 
			 NULL);

  gtk_signal_connect (GTK_OBJECT (dlg), "destroy",
		      GTK_SIGNAL_FUNC (gtk_main_quit),
		      NULL);

  /* Initialize Tooltips */
  gimp_help_init ();
  
  main_vbox = gtk_vbox_new (FALSE, 3);
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
  preview = preview_widget (mangle);
  gtk_container_add (GTK_CONTAINER (frame), preview);
  filter_preview();
  gtk_widget_show (preview);
  
  frame = gtk_frame_new (_("Parameter Settings"));
  gtk_container_set_border_width (GTK_CONTAINER (frame), 6);
  gtk_box_pack_start (GTK_BOX (main_vbox), frame, FALSE, FALSE, 0 );
  gtk_widget_show (frame);

  table = gtk_table_new (3, 2, FALSE);
  gtk_container_set_border_width (GTK_CONTAINER (table), 4);
  gtk_table_set_row_spacings (GTK_TABLE (table), 2);
  gtk_table_set_col_spacings (GTK_TABLE (table), 4);
  gtk_container_add (GTK_CONTAINER (frame), table);
  gtk_widget_show (table);

  spinbutton = gimp_spin_button_new (&adj, parameters.division,
				     -32, 64, 1, 10, 0, 1, 0);
  gimp_table_attach_aligned (GTK_TABLE (table), 0, 0,
			     _("Division:"), 1.0, 0.5,
			     spinbutton, 1, TRUE);
  gtk_signal_connect (GTK_OBJECT (adj), "value_changed",
		      GTK_SIGNAL_FUNC (gimp_int_adjustment_update),
		      &parameters.division);
  gtk_signal_connect (GTK_OBJECT (adj), "value_changed",
		      GTK_SIGNAL_FUNC (filter_preview),
		      NULL);

  radio = gtk_radio_button_new_with_label (group, "Mode 1");
  group = gtk_radio_button_group (GTK_RADIO_BUTTON (radio));
  gtk_signal_connect (GTK_OBJECT (radio), "toggled",
		      GTK_SIGNAL_FUNC (gimp_toggle_button_update),
		      &parameters.type1);
  gtk_signal_connect (GTK_OBJECT (radio), "toggled",
		      GTK_SIGNAL_FUNC (filter_preview),
		      NULL);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (radio), parameters.type1);
  gtk_table_attach (GTK_TABLE (table), radio, 0, 2, 1, 2, GTK_FILL, GTK_FILL, 0, 0);
  gtk_widget_show (radio);

  radio = gtk_radio_button_new_with_label (group, "Mode 2");
  group = gtk_radio_button_group (GTK_RADIO_BUTTON (radio));
  gtk_signal_connect (GTK_OBJECT (radio), "toggled",
		      GTK_SIGNAL_FUNC (gimp_toggle_button_update),
		      &parameters.type2);
  gtk_signal_connect (GTK_OBJECT (radio), "toggled",
		      GTK_SIGNAL_FUNC (filter_preview),
		      NULL);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (radio), parameters.type2);
  gtk_table_attach (GTK_TABLE (table), radio, 0, 2, 2, 3, GTK_FILL, GTK_FILL, 0, 0);
  gtk_widget_show (radio);

  gtk_widget_show (dlg);

  gtk_main ();
  gdk_flush ();

  return dialog_status;
}
