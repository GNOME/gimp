/*
   smooth palette - derive smooth palette from image
   Copyright (C) 1997  Scott Draves <spot@cs.cmu.edu>

   The GIMP -- an image manipulation program
   Copyright (C) 1995 Spencer Kimball and Peter Mattis

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
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


/* Declare local functions. */
static void query  (void);
static void run    (gchar      *name,
		    gint        nparams,
		    GimpParam  *param,
		    gint       *nreturn_vals,
		    GimpParam **return_vals);

static gboolean   dialog (void);

static gint32 doit   (GimpDrawable *drawable,
		      gint32       *layer_id);

GimpPlugInInfo PLUG_IN_INFO =
{
  NULL,  /* init_proc  */
  NULL,  /* quit_proc  */
  query, /* query_proc */
  run,   /* run_proc   */
};

static gboolean run_flag = FALSE;

MAIN ()

static void
query (void)
{
  static GimpParamDef args[] =
  {
    { GIMP_PDB_INT32, "run_mode", "Interactive, non-interactive" },
    { GIMP_PDB_IMAGE, "image", "Input image (unused)" },
    { GIMP_PDB_DRAWABLE, "drawable", "Input drawable" },
    { GIMP_PDB_INT32, "width", "Width" },
    { GIMP_PDB_INT32, "height", "Height" },
    { GIMP_PDB_INT32, "ntries", "Search Depth" },
    { GIMP_PDB_INT32, "show_image","Show Image?" }
  };

  static GimpParamDef return_vals[] =
  {
    { GIMP_PDB_IMAGE, "new_image", "Output image" },
    { GIMP_PDB_LAYER, "new_layer", "Output layer" }
  };

  gimp_install_procedure ("plug_in_smooth_palette",
			  "derive smooth palette from image",
			  "help!",
			  "Scott Draves",
			  "Scott Draves",
			  "1997",
			  N_("<Image>/Filters/Colors/Smooth Palette..."),
			  "RGB*",
			  GIMP_PLUGIN,
			  G_N_ELEMENTS (args), G_N_ELEMENTS (return_vals),
			  args, return_vals);
}

static struct
{
  gint width;
  gint height;
  gint ntries;
  gint try_size;
  gint show_image;
} config =
{
  256,
  64,
  50,
  10000,
  1
};

static void
run (gchar      *name,
     gint        nparams,
     GimpParam  *param,
     gint       *nreturn_vals,
     GimpParam **return_vals)
{
  static GimpParam   values[3];
  GimpRunMode        run_mode;
  GimpPDBStatusType  status = GIMP_PDB_SUCCESS;
  GimpDrawable      *drawable;

  run_mode = param[0].data.d_int32;

  *nreturn_vals = 3;
  *return_vals  = values;

  values[0].type          = GIMP_PDB_STATUS;
  values[0].data.d_status = status;
  values[1].type          = GIMP_PDB_IMAGE;
  values[2].type          = GIMP_PDB_LAYER;

  switch (run_mode)
    {
    case GIMP_RUN_INTERACTIVE:
      INIT_I18N_UI();
      gimp_get_data ("plug_in_smooth_palette", &config);
      if (! dialog ())
	return;
      break;

    case GIMP_RUN_NONINTERACTIVE:
      INIT_I18N();
      if (nparams != 7)
	{
	  status = GIMP_PDB_CALLING_ERROR;
	}
      else
	{
	  config.width      = param[3].data.d_int32;
	  config.height     = param[4].data.d_int32;
	  config.ntries     = param[5].data.d_int32;
	  config.show_image = param[6].data.d_int32 ? TRUE : FALSE;
	}

      if (status == GIMP_PDB_SUCCESS && 
	  ((config.width <= 0) || (config.height <= 0) || config.ntries <= 0))
	status = GIMP_PDB_CALLING_ERROR;

      break;

    case GIMP_RUN_WITH_LAST_VALS:
      INIT_I18N();
      /*  Possibly retrieve data  */
      gimp_get_data ("plug_in_smooth_palette", &config);
      break;

    default:
      break;
    }

  if (status == GIMP_PDB_SUCCESS)
    {
      drawable = gimp_drawable_get (param[2].data.d_drawable);
      if (gimp_drawable_is_rgb (drawable->drawable_id))
	{
	  gimp_progress_init (_("Deriving smooth palette..."));
	  gimp_tile_cache_ntiles (2 * (drawable->width + 1) /
				  gimp_tile_width ());
	  values[1].data.d_image = doit (drawable, &values[2].data.d_layer);
	  if (run_mode == GIMP_RUN_INTERACTIVE)
	    gimp_set_data ("plug_in_smooth_palette", &config, sizeof (config));
	  if (config.show_image)
	    gimp_display_new (values[1].data.d_image);
	}
      else
	status = GIMP_PDB_EXECUTION_ERROR;
      gimp_drawable_detach (drawable);
  }

  values[0].data.d_status = status;
}

static long
pix_diff (guchar *pal,
	  gint    bpp,
	  gint    i,
	  gint    j)
{
  glong r = 0;
  gint k;

  for (k = 0; k < bpp; k++)
    {
      gint p1 = pal[j * bpp + k];
      gint p2 = pal[i * bpp + k];
      r += (p1 - p2) * (p1 - p2);
    }

  return r;
}

static void
pix_swap (guchar *pal,
	  gint    bpp,
	  gint    i,
	  gint    j)
{
  gint k;

  for (k = 0; k < bpp; k++)
    {
      guchar t = pal[j * bpp + k];
      pal[j * bpp + k] = pal[i * bpp + k];
      pal[i * bpp + k] = t;
    }
}

static gint32
doit (GimpDrawable *drawable,
      gint32    *layer_id)
{
  gint32     new_image_id;
  GimpDrawable *new_layer;
  gint       psize, i, j;
  guchar    *pal;
  gint       bpp = drawable->bpp;
  gint 	     sel_x1, sel_x2, sel_y1, sel_y2;
  gint       width, height;
  GimpPixelRgn  pr;
  GRand *gr;
  
  gr = g_rand_new ();

  new_image_id = gimp_image_new (config.width, config.height, GIMP_RGB);
  *layer_id = gimp_layer_new (new_image_id, _("Background"),
			      config.width, config.height,
			      gimp_drawable_type (drawable->drawable_id),
			      100, GIMP_NORMAL_MODE);
  gimp_image_add_layer (new_image_id, *layer_id, 0);
  new_layer = gimp_drawable_get (*layer_id);

  psize = config.width;

  pal = g_malloc (psize * bpp);

  gimp_drawable_mask_bounds (drawable->drawable_id,
			     &sel_x1, &sel_y1, &sel_x2, &sel_y2);
  width = sel_x2 - sel_x1;
  height = sel_y2 - sel_y1;

  gimp_pixel_rgn_init (&pr, drawable, sel_x1, sel_y1, width, height,
		       FALSE, FALSE);

  /* get initial palette */
  for (i = 0; i < psize; i++)
    {
      gint x = sel_x1 + g_rand_int_range (gr, 0, width);
      gint y = sel_y1 + g_rand_int_range (gr, 0, height);

      gimp_pixel_rgn_get_pixel (&pr, pal + bpp * i, x, y);
    }

  /* reorder */
  if (1)
    {
      guchar  *pal_best;
      guchar  *original;
      gdouble  len_best = 0;
      gint     try;

      pal_best = g_memdup (pal, bpp * psize);
      original = g_memdup (pal, bpp * psize);

      for (try = 0; try < config.ntries; try++)
	{
	  gdouble len;

	  if (!(try%5))
	    gimp_progress_update (try / (double) config.ntries);
	  memcpy (pal, original, bpp * psize);

	  /* scramble */
	  for (i = 1; i < psize; i++)
	    pix_swap (pal, bpp, i, g_rand_int_range (gr, 0, psize));

	  /* measure */
	  len = 0.0;
	  for (i = 1; i < psize; i++)
	    len += pix_diff (pal, bpp, i, i-1);

	  /* improve */
	  for (i = 0; i < config.try_size; i++)
	    {
	      gint  i0 = 1 + g_rand_int_range (gr, 0, psize-2);
	      gint  i1 = 1 + g_rand_int_range (gr, 0, psize-2);
	      glong as_is, swapd;

	      if (1 == (i0 - i1))
		{
		  as_is = (pix_diff (pal, bpp, i1 - 1, i1) +
			   pix_diff (pal, bpp, i0, i0 + 1));
		  swapd = (pix_diff (pal, bpp, i1 - 1, i0) +
			   pix_diff (pal, bpp, i1, i0 + 1));
		}
	      else if (1 == (i1 - i0))
		{
		  as_is = (pix_diff (pal, bpp, i0 - 1, i0) +
			   pix_diff (pal, bpp, i1, i1 + 1));
		  swapd = (pix_diff (pal, bpp, i0 - 1, i1) +
			   pix_diff (pal, bpp, i0, i1 + 1));
		}
	      else
		{
		  as_is = (pix_diff (pal, bpp, i0, i0 + 1) +
			   pix_diff (pal, bpp, i0, i0 - 1) +
			   pix_diff (pal, bpp, i1, i1 + 1) +
			   pix_diff (pal, bpp, i1, i1 - 1));
		  swapd = (pix_diff (pal, bpp, i1, i0 + 1) +
			   pix_diff (pal, bpp, i1, i0 - 1) +
			   pix_diff (pal, bpp, i0, i1 + 1) +
			   pix_diff (pal, bpp, i0, i1 - 1));
		}
	      if (swapd < as_is)
		{
		  pix_swap (pal, bpp, i0, i1);
		  len += swapd - as_is;
		}
	    }
	  /* best? */
	  if (0 == try || len < len_best)
	    {
	      memcpy (pal_best, pal, bpp * psize);
	      len_best = len;
	    }
	}
      memcpy (pal, pal_best, bpp * psize);
      g_free (pal_best);
      g_free (original);
      /* clean */
      for (i = 1; i < 4 * psize; i++)
	{
	  glong as_is, swapd;
	  gint i0 = 1 + g_rand_int_range (gr, 0, psize - 2);
	  gint i1 = i0 + 1;

	  as_is = (pix_diff (pal, bpp, i0 - 1, i0) +
		   pix_diff (pal, bpp, i1, i1 + 1));
	  swapd = (pix_diff (pal, bpp, i0 - 1, i1) +
		   pix_diff (pal, bpp, i0, i1 + 1));
	  if (swapd < as_is)
	    {
	      pix_swap (pal, bpp, i0, i1);
	      len_best += swapd - as_is;
	    }
	}
    }

  /* store smooth palette */
  gimp_pixel_rgn_init (&pr, new_layer, 0, 0,
		       config.width, config.height,
		       TRUE, TRUE);
  for (j = 0; j < config.height; j++)
    for (i = 0; i < config.width; i++)
      gimp_pixel_rgn_set_pixel (&pr, pal + bpp * i, i, j);
  g_free (pal);

  g_rand_free (gr);
  gimp_drawable_flush (new_layer);
  gimp_drawable_merge_shadow (new_layer->drawable_id, TRUE);
  gimp_drawable_update(new_layer->drawable_id, 0, 0,
		       config.width, config.height);

  return new_image_id;
}


static void
ok_callback (GtkWidget *widget,
	     gpointer   data)
{
  run_flag = TRUE;

  gtk_widget_destroy (GTK_WIDGET (data));
}

static gboolean
dialog (void)
{
  GtkWidget *dlg;
  GtkWidget *frame;
  GtkWidget *table;
  GtkWidget *spinbutton;
  GtkObject *adj;

  gimp_ui_init ("smooth_palette", FALSE);

  dlg = gimp_dialog_new (_("Smooth Palette"), "smooth_palette",
			 gimp_standard_help_func, "filters/smooth_palette.html",
			 GTK_WIN_POS_MOUSE,
			 FALSE, TRUE, FALSE,

			 GTK_STOCK_CANCEL, gtk_widget_destroy,
			 NULL, 1, NULL, FALSE, TRUE,

			 GTK_STOCK_OK, ok_callback,
			 NULL, NULL, NULL, TRUE, FALSE,

			 NULL);

  g_signal_connect (G_OBJECT (dlg), "destroy",
                    G_CALLBACK (gtk_main_quit),
                    NULL);

  frame = gtk_frame_new (_("Parameter Settings"));
  gtk_container_set_border_width (GTK_CONTAINER (frame), 6);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dlg)->vbox), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  table = gtk_table_new (3, 2, FALSE);
  gtk_table_set_row_spacings (GTK_TABLE (table), 2);
  gtk_table_set_col_spacings (GTK_TABLE (table), 4);
  gtk_container_set_border_width (GTK_CONTAINER (table), 4);
  gtk_container_add (GTK_CONTAINER (frame), table);
  gtk_widget_show (table);


  spinbutton = gimp_spin_button_new (&adj, config.width,
				     1, GIMP_MAX_IMAGE_SIZE, 1, 10, 0, 1, 0);
  gimp_table_attach_aligned (GTK_TABLE (table), 0, 0,
			     _("_Width:"), 1.0, 0.5,
			     spinbutton, 1, FALSE);
  g_signal_connect (G_OBJECT (adj), "value_changed",
                    G_CALLBACK (gimp_int_adjustment_update),
                    &config.width);

  spinbutton = gimp_spin_button_new (&adj, config.height,
				     1, GIMP_MAX_IMAGE_SIZE, 1, 10, 0, 1, 0);
  gimp_table_attach_aligned (GTK_TABLE (table), 0, 1,
			     _("_Height:"), 1.0, 0.5,
			     spinbutton, 1, FALSE);
  g_signal_connect (G_OBJECT (adj), "value_changed",
                    G_CALLBACK (gimp_int_adjustment_update),
                    &config.height);

  spinbutton = gimp_spin_button_new (&adj, config.ntries,
				     1, 1024, 1, 10, 0, 1, 0);
  gimp_table_attach_aligned (GTK_TABLE (table), 0, 2,
			     _("_Search Depth:"), 1.0, 0.5,
			     spinbutton, 1, FALSE);
  g_signal_connect (G_OBJECT (adj), "value_changed",
                    G_CALLBACK (gimp_int_adjustment_update),
                    &config.ntries);

  gtk_widget_show (dlg);

  gtk_main ();
  gdk_flush ();

  return run_flag;
}
