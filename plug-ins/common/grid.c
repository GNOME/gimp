/* The GIMP -- an image manipulation program 
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis 
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

/* Original plug-in coded by Tim Newsome. 
 * 
 * Changed to make use of real-life units by Sven Neumann <sven@gimp.org>.
 * 
 * The interface code is heavily commented in the hope that it will
 * help other plug-in developers to adapt their plug-ins to make use
 * of the gimp_size_entry functionality. 
 * 
 * Note: There is a convenience constructor called gimp_coordinetes_new ()
 *       which simplifies the task of setting up a standard X,Y sizeentry. 
 *
 * For more info and bugs see libgimp/gimpsizeentry.h and libgimp/gimpwidgets.h
 *
 * May 2000 tim copperfield [timecop@japan.co.jp]
 * http://www.ne.jp/asahi/linux/timecop
 * Added dynamic preview.  Due to weird implementation of signals from all
 * controls, preview will not auto-update.  But this plugin isn't really
 * crying for real-time updating either.
 *
 */

#include "config.h"

#include <stdlib.h>
#include <stdio.h>

#include <gtk/gtk.h>

#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

#include "libgimp/stdplugins-intl.h"


#define  SPIN_BUTTON_WIDTH   75
#define  COLOR_BUTTON_WIDTH  55
#define  PREVIEW_SIZE        128

/* Declare local functions. */
static void   query  (void);
static void   run    (gchar   *name,
		      gint     nparams,
		      GimpParam  *param,
		      gint    *nreturn_vals,
		      GimpParam **return_vals);

static guchar      best_cmap_match (guchar    *cmap, 
				    gint       ncolors,
				    guchar    *color);
static void        doit            (gint32     image_ID, 
				    GimpDrawable *drawable, 
				    gboolean   preview_mode);
static gint        dialog          (gint32     image_ID, 
				    GimpDrawable *drawable);
static GtkWidget * preview_widget  (GimpDrawable *drawable);
static void        fill_preview    (GtkWidget *preview_widget, 
				    GimpDrawable *drawable);

GimpPlugInInfo PLUG_IN_INFO =
{
  NULL,  /* init_proc  */
  NULL,  /* quit_proc  */
  query, /* query_proc */
  run,   /* run_proc   */
};

gint sx1, sy1, sx2, sy2;
gint run_flag = FALSE;

static GtkWidget *hcolor_button;
static GtkWidget *vcolor_button;
static guchar    *preview_bits;
static GtkWidget *preview;


typedef struct
{
  gint   hwidth;
  gint   hspace;
  gint   hoffset;
  guint8 hcolor[4];
  gint   vwidth;
  gint   vspace;
  gint   voffset;
  guint8 vcolor[4];
  gint   iwidth;
  gint   ispace;
  gint   ioffset;
  guint8 icolor[4];
}
Config;

Config grid_cfg =
{
  1, 16, 8, { 0, 0, 128, 255 },    /* horizontal   */
  1, 16, 8, { 0, 0, 128, 255 },    /* vertical     */
  0,  2, 6, { 0, 0, 255, 255 },    /* intersection */
};


MAIN ()

static 
void query (void)
{
  static GimpParamDef args[] =
  {
    { GIMP_PDB_INT32,    "run_mode", "Interactive, non-interactive" },
    { GIMP_PDB_IMAGE,    "image",    "Input image" },
    { GIMP_PDB_DRAWABLE, "drawable", "Input drawable" },

    { GIMP_PDB_INT32,    "hwidth",   "Horizontal Width   (>= 0)" },
    { GIMP_PDB_INT32,    "hspace",   "Horizontal Spacing (>= 1)" },
    { GIMP_PDB_INT32,    "hoffset",  "Horizontal Offset  (>= 0)" },
    { GIMP_PDB_COLOR,    "hcolor",   "Horizontal Colour" },
    { GIMP_PDB_INT8,     "hopacity", "Horizontal Opacity (0...255)" },

    { GIMP_PDB_INT32,    "vwidth",   "Vertical Width   (>= 0)" },
    { GIMP_PDB_INT32,    "vspace",   "Vertical Spacing (>= 1)" },
    { GIMP_PDB_INT32,    "voffset",  "Vertical Offset  (>= 0)" },
    { GIMP_PDB_COLOR,    "vcolor",   "Vertical Colour" },
    { GIMP_PDB_INT8,     "vopacity", "Vertical Opacity (0...255)" },

    { GIMP_PDB_INT32,    "iwidth",   "Intersection Width   (>= 0)" },
    { GIMP_PDB_INT32,    "ispace",   "Intersection Spacing (>= 0)" },
    { GIMP_PDB_INT32,    "ioffset",  "Intersection Offset  (>= 0)" },
    { GIMP_PDB_COLOR,    "icolor",   "Intersection Colour" },
    { GIMP_PDB_INT8,     "iopacity", "Intersection Opacity (0...255)" }
  };
  static gint nargs = sizeof (args) / sizeof (args[0]);

  gimp_install_procedure ("plug_in_grid",
			  "Draws a grid.",
			  "Draws a grid using the specified colors. "
			  "The grid origin is the upper left corner.",
			  "Tim Newsome",
			  "Tim Newsome, Sven Neumann, Tom Rathborne, TC",
			  "1997 - 2000",
			  N_("<Image>/Filters/Render/Pattern/Grid..."),
			  "RGB*, GRAY*, INDEXED*",
			  GIMP_PLUGIN,
			  nargs, 0,
			  args, NULL);
}

static void
run (gchar   *name, 
     gint     n_params, 
     GimpParam  *param, 
     gint    *nreturn_vals,
     GimpParam **return_vals)
{
  static GimpParam values[1];
  GimpDrawable *drawable;
  gint32 image_ID;
  GimpRunModeType run_mode;
  GimpPDBStatusType status = GIMP_PDB_SUCCESS;

  *nreturn_vals = 1;
  *return_vals = values;

  INIT_I18N_UI(); 

  run_mode = param[0].data.d_int32;
  image_ID = param[1].data.d_int32;
  drawable = gimp_drawable_get (param[2].data.d_drawable);

  if (run_mode == GIMP_RUN_NONINTERACTIVE)
    {
      if (n_params != 18)
	status = GIMP_PDB_CALLING_ERROR;

      if (status == GIMP_PDB_SUCCESS)
	{
	  grid_cfg.hwidth    = MAX (0, param[3].data.d_int32);
	  grid_cfg.hspace    = MAX (1, param[4].data.d_int32);
	  grid_cfg.hoffset   = MAX (0, param[5].data.d_int32);
	  grid_cfg.hcolor[0] = param[6].data.d_color.red;
	  grid_cfg.hcolor[1] = param[6].data.d_color.green;
	  grid_cfg.hcolor[2] = param[6].data.d_color.blue;
	  grid_cfg.hcolor[3] = param[7].data.d_int8;

	  grid_cfg.vwidth    = MAX (0, param[8].data.d_int32);
	  grid_cfg.vspace    = MAX (1, param[9].data.d_int32);
	  grid_cfg.voffset   = MAX (0, param[10].data.d_int32);
	  grid_cfg.vcolor[0] = param[11].data.d_color.red;
	  grid_cfg.vcolor[1] = param[11].data.d_color.green;
	  grid_cfg.vcolor[2] = param[11].data.d_color.blue;
	  grid_cfg.vcolor[3] = param[12].data.d_int8;

	  grid_cfg.iwidth    = MAX (0, param[13].data.d_int32);
	  grid_cfg.ispace    = MAX (0, param[14].data.d_int32);
	  grid_cfg.ioffset   = MAX (0, param[15].data.d_int32);
	  grid_cfg.icolor[0] = param[16].data.d_color.red;
	  grid_cfg.icolor[1] = param[16].data.d_color.green;
	  grid_cfg.icolor[2] = param[16].data.d_color.blue;
	  grid_cfg.icolor[3] = param[17].data.d_int8;
	}
    }
  else
    {
      /*  Possibly retrieve data  */
      gimp_get_data ("plug_in_grid", &grid_cfg);
    }

  if (run_mode == GIMP_RUN_INTERACTIVE)
    {
      if (!dialog (image_ID, drawable))
	{
	  /* The dialog was closed, or something similarly evil happened. */
	  status = GIMP_PDB_EXECUTION_ERROR;
	}
      g_free(preview_bits);
    }

  if (grid_cfg.hspace <= 0 || grid_cfg.vspace <= 0)
    {
      status = GIMP_PDB_EXECUTION_ERROR;
    }

  if (status == GIMP_PDB_SUCCESS)
    {
      gimp_progress_init (_("Drawing Grid..."));
      gimp_tile_cache_ntiles (2 * (drawable->width / gimp_tile_width () + 1));
      
      doit (image_ID, drawable, FALSE);
      
      if (run_mode != GIMP_RUN_NONINTERACTIVE)
	gimp_displays_flush ();
      
      if (run_mode == GIMP_RUN_INTERACTIVE)
	gimp_set_data ("plug_in_grid", &grid_cfg, sizeof (grid_cfg));

      gimp_drawable_detach (drawable);
    }

  values[0].type = GIMP_PDB_STATUS;
  values[0].data.d_status = status;
}


#define MAXDIFF 195076

static guchar
best_cmap_match (guchar *cmap,
		 gint    ncolors,
		 guchar *color)
{
  guchar cmap_index = 0;
  gint max = MAXDIFF;
  gint i, diff, sum;

  for (i = 0; i < ncolors; i++)
    {
      diff = color[0] - *cmap++;
      sum = SQR (diff);
      diff = color[1] - *cmap++;
      sum += SQR (diff);
      diff = color[2] - *cmap++;
      sum += SQR (diff);
      
      if (sum < max)
	{
	  cmap_index = i;
	  max = sum;
	}
    }

  return cmap_index;
}


G_INLINE_FUNC void
pix_composite (guchar   *p1, 
	       guchar    p2[4], 
	       gint      bytes,
	       gboolean  blend,
	       gboolean  alpha)
{
  gint b;

  if (alpha)
    {
      bytes--;
    }
  
  if (blend)
    {
      for (b = 0; b < bytes; b++)
	{
	  *p1 = *p1 * (1.0 - p2[3]/255.0) + p2[b] * p2[3]/255.0;
	  p1++;
	}
    }
  else
    {
      /* blend should only be TRUE for indexed (bytes == 1) */
      *p1++ = *p2;
    }

  if (alpha && *p1 < 255)
    {
      b = *p1 + 255.0 * ((double)p2[3] / (255.0 - *p1));
      *p1 = b > 255 ? 255 : b;
    }
}


static void
doit (gint32     image_ID,
      GimpDrawable *drawable,
      gboolean   preview_mode)
{
  GimpPixelRgn srcPR, destPR;
  gint width, height, bytes;
  gint x_offset, y_offset;
  guchar *dest;
  gint x, y;
  gboolean alpha;
  gboolean blend;
  guchar hcolor[4];
  guchar vcolor[4];
  guchar icolor[4];
  guchar *cmap;
  gint ncolors;
  
  if (preview_mode) 
    {
      memcpy (hcolor, grid_cfg.hcolor, 4);
      memcpy (vcolor, grid_cfg.vcolor, 4);
      memcpy (icolor, grid_cfg.icolor, 4);
      blend = TRUE;
    } 
  else 
    {
      switch (gimp_image_base_type (image_ID))
	{
	case GIMP_RGB:
	  memcpy (hcolor, grid_cfg.hcolor, 4);
	  memcpy (vcolor, grid_cfg.vcolor, 4);
	  memcpy (icolor, grid_cfg.icolor, 4);
	  blend = TRUE;
	  break;

	case GIMP_GRAY:
	  hcolor[0] = INTENSITY (grid_cfg.hcolor[0], grid_cfg.hcolor[1], grid_cfg.hcolor[2]);
	  hcolor[3] = grid_cfg.hcolor[3];
	  vcolor[0] = INTENSITY (grid_cfg.vcolor[0], grid_cfg.vcolor[1], grid_cfg.vcolor[2]);
	  vcolor[3] = grid_cfg.vcolor[3];
	  icolor[0] = INTENSITY (grid_cfg.icolor[0], grid_cfg.icolor[1], grid_cfg.icolor[2]);
	  icolor[3] = grid_cfg.icolor[3];
	  blend = TRUE;
	  break;

	case GIMP_INDEXED:
	  cmap = gimp_image_get_cmap (image_ID, &ncolors);
	  hcolor[0] = best_cmap_match (cmap, ncolors, grid_cfg.hcolor);
	  hcolor[3] = grid_cfg.hcolor[3];
	  vcolor[0] = best_cmap_match (cmap, ncolors, grid_cfg.vcolor);
	  vcolor[3] = grid_cfg.vcolor[3];
	  icolor[0] = best_cmap_match (cmap, ncolors, grid_cfg.icolor);
	  icolor[3] = grid_cfg.icolor[3];
	  g_free (cmap);
	  blend = FALSE;
	  break;

	default:
	  g_assert_not_reached ();
	  blend = FALSE;
	}
    }
  
  if (preview_mode) 
    { 
      sx1 = sy1 = 0;
      sx2 = GTK_PREVIEW (preview)->buffer_width;
      sy2 = GTK_PREVIEW (preview)->buffer_height;
      width  = sx2;
      height = sy2;
      alpha  = 0;
      bytes  = GTK_PREVIEW (preview)->bpp;
    } 
  else 
    {
      /* Get the input area. This is the bounding box of the selection in
       *  the image (or the entire image if there is no selection). 
       */
      gimp_drawable_mask_bounds (drawable->id, &sx1, &sy1, &sx2, &sy2);
      width  = gimp_drawable_width (drawable->id);
      height = gimp_drawable_height (drawable->id);
      alpha  = gimp_drawable_has_alpha (drawable->id);
      bytes  = drawable->bpp;
      
      /*  initialize the pixel regions  */
      gimp_pixel_rgn_init (&srcPR, drawable, 0, 0, width, height, FALSE, FALSE);
      gimp_pixel_rgn_init (&destPR, drawable, 0, 0, width, height, TRUE, TRUE);
    }

  dest = g_malloc (width * bytes);
  for (y = sy1; y < sy2; y++)
    {
      if (preview_mode) 
	memcpy (dest, 
		preview_bits + (GTK_PREVIEW (preview)->rowstride * y),
		GTK_PREVIEW (preview)->rowstride);
      else 
	gimp_pixel_rgn_get_row (&srcPR, dest, sx1, y, (sx2 - sx1));

      y_offset = y - grid_cfg.voffset;
      while (y_offset < 0)
	y_offset += grid_cfg.vspace;

      if ((y_offset + (grid_cfg.vwidth / 2)) % grid_cfg.vspace < grid_cfg.vwidth)
	{
	  for (x = sx1; x < sx2; x++)
	    {
	      pix_composite (&dest[(x-sx1) * bytes], hcolor, bytes, blend, alpha);
	    }
	}

      if ((y_offset + (grid_cfg.iwidth / 2)) % grid_cfg.vspace < grid_cfg.iwidth)
        {
	  for (x = sx1; x < sx2; x++)
	    {
	      x_offset = grid_cfg.hspace + x - grid_cfg.hoffset;
	      while (x_offset < 0)
		x_offset += grid_cfg.hspace;

              if ((x_offset % grid_cfg.hspace >= grid_cfg.ispace
		   && 
		   x_offset % grid_cfg.hspace < grid_cfg.ioffset) 
		  || 
		  (grid_cfg.hspace - (x_offset % grid_cfg.hspace) >= grid_cfg.ispace
		   && 
		   grid_cfg.hspace - (x_offset % grid_cfg.hspace) < grid_cfg.ioffset))
		{
		  pix_composite (&dest[(x-sx1) * bytes], icolor, bytes, blend, alpha);
                }
	    }
        }

      for (x = sx1; x < sx2; x++)
        {
	  x_offset = grid_cfg.hspace + x - grid_cfg.hoffset;
	  while (x_offset < 0)
	    x_offset += grid_cfg.hspace;

          if ((x_offset + (grid_cfg.hwidth / 2)) % grid_cfg.hspace < grid_cfg.hwidth)
            {
	      pix_composite (&dest[(x-sx1) * bytes], vcolor, bytes, blend, alpha);
            }

          if ((x_offset + (grid_cfg.iwidth / 2)) % grid_cfg.hspace < grid_cfg.iwidth
	      && 
	      ((y_offset % grid_cfg.vspace >= grid_cfg.ispace
		&& 
		y_offset % grid_cfg.vspace < grid_cfg.ioffset)
	       || 
	       (grid_cfg.vspace - (y_offset % grid_cfg.vspace) >= grid_cfg.ispace
		&& 
		grid_cfg.vspace - (y_offset % grid_cfg.vspace) < grid_cfg.ioffset)))
            {
	      pix_composite (&dest[(x-sx1) * bytes], icolor, bytes, blend, alpha);
            }
        }
      if (preview_mode) 
	{
	  memcpy (GTK_PREVIEW (preview)->buffer + (GTK_PREVIEW (preview)->rowstride * y),
		  dest,
		  GTK_PREVIEW (preview)->rowstride);
	} 
      else 
	{
	  gimp_pixel_rgn_set_row (&destPR, dest, sx1, y, (sx2-sx1) );
	  gimp_progress_update ((double) y / (double) (sy2 - sy1));
	}
    } 
  g_free (dest);

  /*  update the timred region  */
  if (preview_mode) 
    {
      gtk_widget_queue_draw (preview);
    } 
  else 
    {
      gimp_drawable_flush (drawable);
      gimp_drawable_merge_shadow (drawable->id, TRUE);
      gimp_drawable_update (drawable->id, sx1, sy1, sx2 - sx1, sy2 - sy1);
    }
}


/***************************************************
 * GUI stuff
 */

static void
ok_callback (GtkWidget *widget, 
	     gpointer   data)
{
  GtkWidget *entry;

  run_flag = TRUE;

  entry = gtk_object_get_data (GTK_OBJECT (data), "width");
  grid_cfg.hwidth = (int)(gimp_size_entry_get_refval (GIMP_SIZE_ENTRY (entry), 0) + 0.5);
  grid_cfg.vwidth = (int)(gimp_size_entry_get_refval (GIMP_SIZE_ENTRY (entry), 1) + 0.5);
  grid_cfg.iwidth = (int)(gimp_size_entry_get_refval (GIMP_SIZE_ENTRY (entry), 2) + 0.5);
  
  entry = gtk_object_get_data (GTK_OBJECT (data), "space");
  grid_cfg.hspace = (int)(gimp_size_entry_get_refval (GIMP_SIZE_ENTRY (entry), 0) + 0.5);
  grid_cfg.vspace = (int)(gimp_size_entry_get_refval (GIMP_SIZE_ENTRY (entry), 1) + 0.5);
  grid_cfg.ispace = (int)(gimp_size_entry_get_refval (GIMP_SIZE_ENTRY (entry), 2) + 0.5);
 
  entry = gtk_object_get_data (GTK_OBJECT (data), "offset");
  grid_cfg.hoffset = (int)(gimp_size_entry_get_refval (GIMP_SIZE_ENTRY (entry), 0) + 0.5);
  grid_cfg.voffset = (int)(gimp_size_entry_get_refval (GIMP_SIZE_ENTRY (entry), 1) + 0.5);
  grid_cfg.ioffset = (int)(gimp_size_entry_get_refval (GIMP_SIZE_ENTRY (entry), 2) + 0.5);

  gtk_widget_destroy (GTK_WIDGET (data));
}


static void
entry_callback (GtkWidget *widget, 
		gpointer   data)
{
  static gdouble x = -1.0;
  static gdouble y = -1.0;
  gdouble new_x;
  gdouble new_y;

  new_x = gimp_size_entry_get_refval (GIMP_SIZE_ENTRY (widget), 0);
  new_y = gimp_size_entry_get_refval (GIMP_SIZE_ENTRY (widget), 1);

  if (gimp_chain_button_get_active (GIMP_CHAIN_BUTTON (data)))
    {
      if (new_x != x)
	{
	  y = new_y = x = new_x;
	  gimp_size_entry_set_refval (GIMP_SIZE_ENTRY (widget), 1, y);
	}
      if (new_y != y)
	{
	  x = new_x = y = new_y;
	  gimp_size_entry_set_refval (GIMP_SIZE_ENTRY (widget), 0, x);
	}
    }
  else
    {
      if (new_x != x)
	x = new_x;
      if (new_y != y)
	y = new_y;
    }     
}


static void
color_callback (GtkWidget *widget, 
		gpointer   data)
{
  gint i;

  if (gimp_chain_button_get_active (GIMP_CHAIN_BUTTON (data)))
    {
      if (widget == hcolor_button)
	{
	  for (i = 0; i < 4; i++)
	    grid_cfg.vcolor[i] = grid_cfg.hcolor[i];
	  gimp_color_button_update (GIMP_COLOR_BUTTON (vcolor_button));
	}
      else
	{
	  for (i = 0; i < 4; i++)
	    grid_cfg.hcolor[i] = grid_cfg.vcolor[i];
	  gimp_color_button_update (GIMP_COLOR_BUTTON (hcolor_button));
	}	
    }
}


static void
update_preview_callback (GtkWidget *widget, 
			 gpointer   data)
{
  GimpDrawable *drawable;
  GtkWidget *entry;

  drawable = gtk_object_get_data (GTK_OBJECT (widget), "drawable");

  entry = gtk_object_get_data (GTK_OBJECT (widget), "width");
  grid_cfg.hwidth = (int)(gimp_size_entry_get_refval (GIMP_SIZE_ENTRY (entry), 0) + 0.5);
  grid_cfg.vwidth = (int)(gimp_size_entry_get_refval (GIMP_SIZE_ENTRY (entry), 1) + 0.5);
  grid_cfg.iwidth = (int)(gimp_size_entry_get_refval (GIMP_SIZE_ENTRY (entry), 2) + 0.5);

  entry = gtk_object_get_data (GTK_OBJECT (widget), "space");
  grid_cfg.hspace = (int)(gimp_size_entry_get_refval (GIMP_SIZE_ENTRY (entry), 0) + 0.5);
  grid_cfg.vspace = (int)(gimp_size_entry_get_refval (GIMP_SIZE_ENTRY (entry), 1) + 0.5);
  grid_cfg.ispace = (int)(gimp_size_entry_get_refval (GIMP_SIZE_ENTRY (entry), 2) + 0.5);

  entry = gtk_object_get_data (GTK_OBJECT (widget), "offset");
  grid_cfg.hoffset = (int)(gimp_size_entry_get_refval (GIMP_SIZE_ENTRY (entry), 0) + 0.5);
  grid_cfg.voffset = (int)(gimp_size_entry_get_refval (GIMP_SIZE_ENTRY (entry), 1) + 0.5);
  grid_cfg.ioffset = (int)(gimp_size_entry_get_refval (GIMP_SIZE_ENTRY (entry), 2) + 0.5);

  doit (0, drawable, TRUE); /* we can set image_ID = 0 'cause we dont use it */
}

static gint
dialog (gint32     image_ID,
	GimpDrawable *drawable)
{
  GtkWidget *dlg;
  GtkWidget *main_hbox;
  GtkWidget *main_vbox;
  GtkWidget *frame;
  GtkWidget *abox;
 
  GtkWidget *button;
  GtkWidget *hbox;
  GtkWidget *width;
  GtkWidget *space;
  GtkWidget *offset;
  GtkWidget *chain_button;
  GtkWidget *table;
  GtkWidget *align;
  GimpUnit   unit;
  gdouble    xres;
  gdouble    yres;

  gimp_ui_init ("grid", TRUE);

  dlg = gimp_dialog_new (_("Grid"), "grid",
			 gimp_standard_help_func, "filters/grid.html",
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

  /*  Get the image resolution and unit  */
  gimp_image_get_resolution (image_ID, &xres, &yres);
  unit = gimp_image_get_unit (image_ID);

  /* main hbox: [   ][       ] */
  main_hbox = gtk_hbox_new (FALSE, 2);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dlg)->vbox), main_hbox, FALSE, FALSE, 4);
  /* hbox created and packed into the dialog */

  /* make a nice frame */
  frame = gtk_frame_new (_("Preview"));
  gtk_container_set_border_width (GTK_CONTAINER (frame), 4);
  gtk_box_pack_start (GTK_BOX (main_hbox), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  hbox = gtk_vbox_new (FALSE, 2);
  gtk_container_set_border_width (GTK_CONTAINER (hbox), 0);
  gtk_container_add (GTK_CONTAINER (frame), hbox);
  gtk_widget_show (hbox);

  /* row 1 */
  abox = gtk_alignment_new (0.5, 0.5, 0.0, 0.0);
  gtk_container_set_border_width (GTK_CONTAINER (abox), 4);
  gtk_box_pack_start (GTK_BOX (hbox), abox, TRUE, TRUE, 0);
  gtk_widget_show (abox);
  frame = gtk_frame_new (NULL);
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_IN);
  gtk_container_add (GTK_CONTAINER (abox), frame);
  gtk_widget_show (frame);
  preview = preview_widget (drawable); /* we are here */
  gtk_container_add (GTK_CONTAINER (frame), preview);
  doit (image_ID, drawable, TRUE); /* render preview */
  gtk_widget_show (preview);

  /* row 2 */
  abox = gtk_alignment_new (0.5, 0.5, 0.0, 0.0);
  gtk_container_set_border_width (GTK_CONTAINER (abox), 4);
  gtk_container_add (GTK_CONTAINER (hbox), abox);
  gtk_widget_show (abox);
  button = gtk_button_new_with_label (_("Update Preview"));
  gtk_misc_set_padding (GTK_MISC (GTK_BIN (button)->child), 2, 0);
  gtk_object_set_data (GTK_OBJECT (button), "drawable", drawable);
  gtk_signal_connect (GTK_OBJECT (button), "clicked",
		      GTK_SIGNAL_FUNC (update_preview_callback),
		      NULL);
  gtk_container_add (GTK_CONTAINER (abox), button);
  gtk_widget_show (button);
  /* left side of the UI is done */
 
  /* right side */
  frame = gtk_frame_new (_("Parameter Settings"));
  gtk_container_set_border_width (GTK_CONTAINER (frame), 4);
  gtk_box_pack_start (GTK_BOX (main_hbox), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  main_vbox = gtk_vbox_new (FALSE, 4);
  gtk_container_set_border_width (GTK_CONTAINER (main_vbox), 6);
  gtk_container_add (GTK_CONTAINER (frame), main_vbox);
  gtk_widget_show (main_vbox);
  
  /*  The width entries  */
  width = gimp_size_entry_new (3,                            /*  number_of_fields  */ 
			       unit,                         /*  unit              */
			       "%a",                         /*  unit_format       */
			       TRUE,                         /*  menu_show_pixels  */
			       TRUE,                         /*  menu_show_percent */
			       FALSE,                        /*  show_refval       */
			       SPIN_BUTTON_WIDTH,            /*  spinbutton_usize  */
			       GIMP_SIZE_ENTRY_UPDATE_SIZE); /*  update_policy     */
  gtk_object_set_data (GTK_OBJECT (button), "width", width);

  /*  set the unit back to pixels, since most times we will want pixels */
  gimp_size_entry_set_unit (GIMP_SIZE_ENTRY (width), GIMP_UNIT_PIXEL);

  /*  set the resolution to the image resolution  */
  gimp_size_entry_set_resolution (GIMP_SIZE_ENTRY (width), 0, xres, TRUE);
  gimp_size_entry_set_resolution (GIMP_SIZE_ENTRY (width), 1, yres, TRUE);
  gimp_size_entry_set_resolution (GIMP_SIZE_ENTRY (width), 2, xres, TRUE);

  /*  set the size (in pixels) that will be treated as 0% and 100%  */
  gimp_size_entry_set_size (GIMP_SIZE_ENTRY (width), 0, 0.0, (gdouble)(drawable->width));
  gimp_size_entry_set_size (GIMP_SIZE_ENTRY (width), 1, 0.0, (gdouble)(drawable->height));
  gimp_size_entry_set_size (GIMP_SIZE_ENTRY (width), 2, 0.0, (gdouble)(drawable->width));

  /*  set upper and lower limits (in pixels)  */
  gimp_size_entry_set_refval_boundaries (GIMP_SIZE_ENTRY (width), 0, 0.0, 
					 (gdouble)(drawable->width));
  gimp_size_entry_set_refval_boundaries (GIMP_SIZE_ENTRY (width), 1, 0.0, 
					 (gdouble)(drawable->height));
  gimp_size_entry_set_refval_boundaries (GIMP_SIZE_ENTRY (width), 2, 0.0, 
					 (gdouble)(MAX (drawable->width, drawable->height)));
  gtk_table_set_col_spacing (GTK_TABLE (width), 2, 12);
  gtk_table_set_col_spacing (GTK_TABLE (width), 3, 12);

  /*  initialize the values  */
  gimp_size_entry_set_refval (GIMP_SIZE_ENTRY (width), 0, (gdouble)grid_cfg.hwidth);
  gimp_size_entry_set_refval (GIMP_SIZE_ENTRY (width), 1, (gdouble)grid_cfg.vwidth);
  gimp_size_entry_set_refval (GIMP_SIZE_ENTRY (width), 2, (gdouble)grid_cfg.iwidth);

  /*  attach labels  */
  gimp_size_entry_attach_label (GIMP_SIZE_ENTRY (width), _("Horizontal"), 0, 1, 0.0);
  gimp_size_entry_attach_label (GIMP_SIZE_ENTRY (width), _("Vertical"), 0, 2, 0.0);
  gimp_size_entry_attach_label (GIMP_SIZE_ENTRY (width), _("Intersection"), 0, 3, 0.0);
  gimp_size_entry_attach_label (GIMP_SIZE_ENTRY (width), _("Width: "), 1, 0, 0.0); 

  /*  put a chain_button under the size_entries  */
  chain_button = gimp_chain_button_new (GIMP_CHAIN_BOTTOM);
  if (grid_cfg.hwidth == grid_cfg.vwidth)
    gimp_chain_button_set_active (GIMP_CHAIN_BUTTON (chain_button), TRUE);
  gtk_table_attach_defaults (GTK_TABLE (width), chain_button, 1, 3, 2, 3);
  gtk_widget_show (chain_button);

  /*  connect to the 'value_changed' signal because we have to take care 
      of keeping the entries in sync when the chainbutton is active        */
  gtk_signal_connect (GTK_OBJECT (width), "value_changed", 
		      (GtkSignalFunc) entry_callback, chain_button);

  abox = gtk_alignment_new (1.0, 0.5, 0.0, 0.0);
  gtk_container_add (GTK_CONTAINER (abox), width);
  gtk_box_pack_start (GTK_BOX (main_vbox), abox, FALSE, FALSE, 4);
  gtk_widget_show (width);
  gtk_widget_show (abox);

  /*  The spacing entries  */
  space = gimp_size_entry_new (3,                            /*  number_of_fields  */ 
			       unit,                         /*  unit              */
			       "%a",                         /*  unit_format       */
			       TRUE,                         /*  menu_show_pixels  */
			       TRUE,                         /*  menu_show_percent */
			       FALSE,                        /*  show_refval       */
			       SPIN_BUTTON_WIDTH,            /*  spinbutton_usize  */
			       GIMP_SIZE_ENTRY_UPDATE_SIZE); /*  update_policy     */
  gtk_object_set_data (GTK_OBJECT (button), "space", space);
  gimp_size_entry_set_unit (GIMP_SIZE_ENTRY (space), GIMP_UNIT_PIXEL);

  /*  set the resolution to the image resolution  */
  gimp_size_entry_set_resolution (GIMP_SIZE_ENTRY (space), 0, xres, TRUE);
  gimp_size_entry_set_resolution (GIMP_SIZE_ENTRY (space), 1, yres, TRUE);
  gimp_size_entry_set_resolution (GIMP_SIZE_ENTRY (space), 2, xres, TRUE);

  /*  set the size (in pixels) that will be treated as 0% and 100%  */
  gimp_size_entry_set_size (GIMP_SIZE_ENTRY (space), 0, 0.0, (gdouble)(drawable->width));
  gimp_size_entry_set_size (GIMP_SIZE_ENTRY (space), 1, 0.0, (gdouble)(drawable->height));
  gimp_size_entry_set_size (GIMP_SIZE_ENTRY (space), 2, 0.0, (gdouble)(drawable->width));

  /*  set upper and lower limits (in pixels)  */
  gimp_size_entry_set_refval_boundaries (GIMP_SIZE_ENTRY (space), 0, 1.0, 
					 (gdouble)(drawable->width));
  gimp_size_entry_set_refval_boundaries (GIMP_SIZE_ENTRY (space), 1, 1.0, 
					 (gdouble)(drawable->height));
  gimp_size_entry_set_refval_boundaries (GIMP_SIZE_ENTRY (space), 2, 0.0, 
					 (gdouble)(MAX (drawable->width, drawable->height)));
  gtk_table_set_col_spacing (GTK_TABLE (space), 2, 12);
  gtk_table_set_col_spacing (GTK_TABLE (space), 3, 12);

  /*  initialize the values  */
  gimp_size_entry_set_refval (GIMP_SIZE_ENTRY (space), 0, (gdouble)grid_cfg.hspace);
  gimp_size_entry_set_refval (GIMP_SIZE_ENTRY (space), 1, (gdouble)grid_cfg.vspace);
  gimp_size_entry_set_refval (GIMP_SIZE_ENTRY (space), 2, (gdouble)grid_cfg.ispace);
  /*  attach labels  */
  gimp_size_entry_attach_label (GIMP_SIZE_ENTRY (space), _("Spacing: "), 1, 0, 0.0); 

  /*  put a chain_button under the spacing_entries  */
  chain_button = gimp_chain_button_new (GIMP_CHAIN_BOTTOM);
  if (grid_cfg.hspace == grid_cfg.vspace)
    gimp_chain_button_set_active (GIMP_CHAIN_BUTTON (chain_button), TRUE);
  gtk_table_attach_defaults (GTK_TABLE (space), chain_button, 1, 3, 2, 3);
  gtk_widget_show (chain_button);

  /*  connect to the 'value_changed' and "unit_changed" signals because we have to 
      take care of keeping the entries in sync when the chainbutton is active        */
  gtk_signal_connect (GTK_OBJECT (space), "value_changed", 
		      (GtkSignalFunc) entry_callback, chain_button);
  gtk_signal_connect (GTK_OBJECT (space), "unit_changed", 
		      (GtkSignalFunc) entry_callback, chain_button);

  abox = gtk_alignment_new (1.0, 0.5, 0.0, 0.0);
  gtk_container_add (GTK_CONTAINER (abox), space);
  gtk_box_pack_start (GTK_BOX (main_vbox), abox, FALSE, FALSE, 4);
  gtk_widget_show (space);
  gtk_widget_show (abox);

  /*  The offset entries  */
  offset = gimp_size_entry_new (3,                            /*  number_of_fields  */ 
				unit,                         /*  unit              */
				"%a",                         /*  unit_format       */
				TRUE,                         /*  menu_show_pixels  */
				TRUE,                         /*  menu_show_percent */
				FALSE,                        /*  show_refval       */
				SPIN_BUTTON_WIDTH,            /*  spinbutton_usize  */
				GIMP_SIZE_ENTRY_UPDATE_SIZE); /*  update_policy     */
  gtk_object_set_data (GTK_OBJECT (button), "offset", offset);
  gimp_size_entry_set_unit (GIMP_SIZE_ENTRY (offset), GIMP_UNIT_PIXEL);

  /*  set the resolution to the image resolution  */
  gimp_size_entry_set_resolution (GIMP_SIZE_ENTRY (offset), 0, xres, TRUE);
  gimp_size_entry_set_resolution (GIMP_SIZE_ENTRY (offset), 1, yres, TRUE);
  gimp_size_entry_set_resolution (GIMP_SIZE_ENTRY (offset), 2, xres, TRUE);

  /*  set the size (in pixels) that will be treated as 0% and 100%  */
  gimp_size_entry_set_size (GIMP_SIZE_ENTRY (offset), 0, 0.0, (gdouble)(drawable->width));
  gimp_size_entry_set_size (GIMP_SIZE_ENTRY (offset), 1, 0.0, (gdouble)(drawable->height));
  gimp_size_entry_set_size (GIMP_SIZE_ENTRY (offset), 2, 0.0, (gdouble)(drawable->width));

  /*  set upper and lower limits (in pixels)  */
  gimp_size_entry_set_refval_boundaries (GIMP_SIZE_ENTRY (offset), 0, 0.0, 
					 (gdouble)(drawable->width));
  gimp_size_entry_set_refval_boundaries (GIMP_SIZE_ENTRY (offset), 1, 0.0, 
					 (gdouble)(drawable->height));
  gimp_size_entry_set_refval_boundaries (GIMP_SIZE_ENTRY (offset), 2, 0.0, 
					 (gdouble)(MAX (drawable->width, drawable->height)));
  gtk_table_set_col_spacing (GTK_TABLE (offset), 2, 12);
  gtk_table_set_col_spacing (GTK_TABLE (offset), 3, 12);

  /*  initialize the values  */
  gimp_size_entry_set_refval (GIMP_SIZE_ENTRY (offset), 0, (gdouble)grid_cfg.hoffset);
  gimp_size_entry_set_refval (GIMP_SIZE_ENTRY (offset), 1, (gdouble)grid_cfg.voffset);
  gimp_size_entry_set_refval (GIMP_SIZE_ENTRY (offset), 2, (gdouble)grid_cfg.ioffset);

  /*  attach labels  */
  gimp_size_entry_attach_label (GIMP_SIZE_ENTRY (offset), _("Offset: "), 1, 0, 0.0); 

  /*  this is a weird hack: we put a table into the offset table  */
  table = gtk_table_new (3, 3, FALSE);
  gtk_table_attach_defaults (GTK_TABLE (offset), table, 1, 4, 2, 3);
  gtk_table_set_row_spacing (GTK_TABLE (table), 0, 10);
  gtk_table_set_col_spacing (GTK_TABLE (table), 1, 12);

  /*  put a chain_button under the offset_entries  */
  chain_button = gimp_chain_button_new (GIMP_CHAIN_BOTTOM);
  if (grid_cfg.hoffset == grid_cfg.voffset)
    gimp_chain_button_set_active (GIMP_CHAIN_BUTTON (chain_button), TRUE);
  gtk_table_attach_defaults (GTK_TABLE (table), chain_button, 0, 2, 0, 1);
  gtk_widget_show (chain_button);

  /*  connect to the 'value_changed' and "unit_changed" signals because we have to 
      take care of keeping the entries in sync when the chainbutton is active        */
  gtk_signal_connect (GTK_OBJECT (offset), "value_changed", 
		      (GtkSignalFunc) entry_callback, chain_button);
  gtk_signal_connect (GTK_OBJECT (offset), "unit_changed",
		      (GtkSignalFunc) entry_callback, chain_button);

  /*  put a chain_button under the color_buttons  */
  chain_button = gimp_chain_button_new (GIMP_CHAIN_BOTTOM);
  if ((guint32)(*grid_cfg.hcolor) == (guint32)(*grid_cfg.vcolor))
    gimp_chain_button_set_active (GIMP_CHAIN_BUTTON (chain_button), TRUE);
  gtk_table_attach_defaults (GTK_TABLE (table), chain_button, 0, 2, 2, 3);
  gtk_widget_show (chain_button);

  /*  attach color selectors  */
  hcolor_button = gimp_color_button_new (_("Horizontal Color"), COLOR_BUTTON_WIDTH, 16, 
					 grid_cfg.hcolor, 4);
  gtk_signal_connect (GTK_OBJECT (hcolor_button), "color_changed", 
		      (GtkSignalFunc) color_callback, chain_button);
  align = gtk_alignment_new (0.0, 0.5, 0, 0);
  gtk_container_add (GTK_CONTAINER (align),  hcolor_button);
  gtk_table_attach_defaults (GTK_TABLE (table), align, 0, 1, 1, 2);
  gtk_widget_show (hcolor_button);
  gtk_widget_show (align);

  vcolor_button = gimp_color_button_new (_("Vertical Color"), COLOR_BUTTON_WIDTH, 16, 
					 grid_cfg.vcolor, 4);
  gtk_signal_connect (GTK_OBJECT (vcolor_button), "color_changed", 
		      (GtkSignalFunc) color_callback, chain_button);  
  align = gtk_alignment_new (0.0, 0.5, 0, 0);
  gtk_container_add (GTK_CONTAINER (align), vcolor_button);
  gtk_table_attach_defaults (GTK_TABLE (table), align, 1, 2, 1, 2);
  gtk_widget_show (vcolor_button);
  gtk_widget_show (align);

  button = gimp_color_button_new (_("Intersection Color"), COLOR_BUTTON_WIDTH, 16, 
				  grid_cfg.icolor, 4);
  align = gtk_alignment_new (0.0, 0.5, 0, 0);
  gtk_container_add (GTK_CONTAINER (align), button);
  gtk_table_attach_defaults (GTK_TABLE (table), align, 2, 3, 1, 2);
  gtk_widget_show (button);
  gtk_widget_show (align);
  gtk_widget_show (table);

  abox = gtk_alignment_new (1.0, 0.5, 0.0, 0.0);
  gtk_container_add (GTK_CONTAINER (abox), offset);
  gtk_box_pack_start (GTK_BOX (main_vbox), abox, FALSE, FALSE, 4);
  gtk_widget_show (offset);
  gtk_widget_show (abox);

  gtk_widget_show (main_hbox); 
  gtk_widget_show (dlg);

  gtk_object_set_data (GTK_OBJECT (dlg), "width",  width);
  gtk_object_set_data (GTK_OBJECT (dlg), "space",  space);  
  gtk_object_set_data (GTK_OBJECT (dlg), "offset", offset);  

  gtk_main ();
  gdk_flush ();

  return run_flag;
}

static GtkWidget *
preview_widget (GimpDrawable *drawable)
{
  gint       size;
  GtkWidget *preview;

  preview = gtk_preview_new (GTK_PREVIEW_COLOR);
  fill_preview (preview, drawable);
  size = GTK_PREVIEW (preview)->rowstride * GTK_PREVIEW (preview)->buffer_height;
  preview_bits = g_malloc (size);
  memcpy (preview_bits, GTK_PREVIEW (preview)->buffer, size);
  
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
