/*
 * This is a plugin for the GIMP.
 *
 * Copyright (C) 1996 Stephen Norris
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
 */

/*
 * This plug-in produces plasma fractal images. The algorithm is losely
 * based on a description of the fractint algorithm, but completely
 * re-implemented because the fractint code was too ugly to read :)
 *
 * Please send any patches or suggestions to me: srn@flibble.cs.su.oz.au.
 */

/*
 * TODO:
 *	- The progress bar sucks.
 *	- It writes some pixels more than once.
 */

/* Version 1.01 */

/*
 * Ported to GIMP Plug-in API 1.0
 *    by Eiichi Takamori <taka@ma1.seikyou.ne.jp>
 *
 * $Id$
 *
 * A few functions names and their order are changed :)
 * Plasma implementation almost hasn't been changed.
 *
 * Feel free to correct my WRONG English, or to modify Plug-in Path,
 * and so on. ;-)
 *
 * Version 1.02 
 *
 * May 2000
 * tim copperfield [timecop@japan.co.jp]
 * Added dynamic preview mode.
 *
 */

#include "config.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h> /* memcpy */

#ifdef __GNUC__
#warning GTK_DISABLE_DEPRECATED
#endif
#undef GTK_DISABLE_DEPRECATED

#include <gtk/gtk.h>

#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

#include "libgimp/stdplugins-intl.h"


/* Some useful macros */

#define ENTRY_WIDTH      75
#define SCALE_WIDTH     128
#define TILE_CACHE_SIZE  32
#define PREVIEW_SIZE    128

typedef struct
{
  guint32     seed;
  gdouble  turbulence;
} PlasmaValues;

typedef struct
{
  gint run;
} PlasmaInterface;

/*
 * Function prototypes.
 */

static void	query	(void);
static void	run	(gchar      *name,
			 gint        nparams,
			 GimpParam  *param,
			 gint       *nreturn_vals,
			 GimpParam **return_vals);

static GtkWidget *preview_widget         (GimpImageType drawable_type);
static gint	plasma_dialog            (GimpDrawable *drawable,
    					  GimpImageType drawable_type);
static void     plasma_ok_callback       (GtkWidget *widget, 
					  gpointer   data);
static void     plasma_seed_changed_callback (GimpDrawable *drawable,
                                              gpointer      data);

static void	plasma	     (GimpDrawable *drawable, 
			      gboolean   preview_mode);
static void     random_rgb   (GRand *gr, 
                              guchar    *d);
static void     add_random   (GRand *gr, 
                              guchar    *d,
			      gint       amnt);
static GimpPixelFetcher *init_plasma  (GimpDrawable *drawable, 
				       gboolean   preview_mode,
				       GRand *gr);
static void     end_plasma   (GimpDrawable *drawable,
			      GimpPixelFetcher *pft,
                              GRand *gr);
static void     get_pixel    (GimpPixelFetcher *pft,
			      gint       x,
			      gint       y,
			      guchar    *pixel);
static void     put_pixel    (GimpPixelFetcher *pft,
			      gint       x,
			      gint       y,
			      guchar    *pixel);
static gint     do_plasma    (GimpPixelFetcher *pft,
			      gint       x1,
			      gint       y1,
			      gint       x2,
			      gint       y2,
			      gint       depth,
			      gint       scale_depth,
                              GRand     *gr);

/***** Local vars *****/

GimpPlugInInfo PLUG_IN_INFO =
{
  NULL,  /* init_proc  */
  NULL,  /* quit_proc  */
  query, /* query_proc */
  run,   /* run_proc   */
};

static PlasmaValues pvals =
{
  0,     /* seed       */
  1.0,   /* turbulence */
};

static PlasmaInterface pint =
{
  FALSE     /* run */
};

static guchar *work_buffer;
static GtkWidget *preview;

/***** Functions *****/

MAIN ()

static void
query (void)
{
  static GimpParamDef args[]=
  {
    { GIMP_PDB_INT32, "run_mode", "Interactive, non-interactive" },
    { GIMP_PDB_IMAGE, "image", "Input image (unused)" },
    { GIMP_PDB_DRAWABLE, "drawable", "Input drawable" },
    { GIMP_PDB_INT32, "seed", "Random seed" },
    { GIMP_PDB_FLOAT, "turbulence", "Turbulence of plasma" }
  };

  gimp_install_procedure ("plug_in_plasma",
			  "Create a plasma cloud like image to the specified drawable",
			  "More help",
			  "Stephen Norris & (ported to 1.0 by) Eiichi Takamori",
			  "Stephen Norris",
			  "May 2000",
			  /* don't translate '<Image>', it's a special
			   * keyword of the gtk toolkit */
			  N_("<Image>/Filters/Render/Clouds/Plasma..."),
			  "RGB*, GRAY*",
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
  static GimpParam   values[1];
  GimpDrawable      *drawable;
  GimpImageType      drawable_type;
  GimpRunMode        run_mode;
  GimpPDBStatusType  status = GIMP_PDB_SUCCESS;

  run_mode = param[0].data.d_int32;

  *nreturn_vals = 1;
  *return_vals  = values;

  values[0].type          = GIMP_PDB_STATUS;
  values[0].data.d_status = status;

  /*  Get the specified drawable  */
  drawable = gimp_drawable_get (param[2].data.d_drawable);
  drawable_type = gimp_drawable_type (param[2].data.d_drawable);

  switch (run_mode)
    {
    case GIMP_RUN_INTERACTIVE:
      INIT_I18N_UI();
      /*  Possibly retrieve data  */
      gimp_get_data ("plug_in_plasma", &pvals);

      /*  First acquire information with a dialog  */
      if (! plasma_dialog (drawable, drawable_type))
	{
	  gimp_drawable_detach (drawable);
	  return;
	}
      break;

    case GIMP_RUN_NONINTERACTIVE:
      INIT_I18N();
      /*  Make sure all the arguments are there!  */
      if (nparams != 5)
	{
	  status = GIMP_PDB_CALLING_ERROR;
	}
      else
	{
	  pvals.seed = (guint32) param[3].data.d_int32;
	  pvals.turbulence = (gdouble) param[4].data.d_float;

	  if (pvals.turbulence <= 0)
	    status = GIMP_PDB_CALLING_ERROR;
	}
      break;

    case GIMP_RUN_WITH_LAST_VALS:
      INIT_I18N();
      /*  Possibly retrieve data  */
      gimp_get_data ("plug_in_plasma", &pvals);
      break;

    default:
      break;
    }

  if (status == GIMP_PDB_SUCCESS)
    {
      /*  Make sure that the drawable is gray or RGB color  */
      if (gimp_drawable_is_rgb (drawable->drawable_id) ||
	  gimp_drawable_is_gray (drawable->drawable_id))
	{
	  gimp_progress_init (_("Plasma..."));
	  gimp_tile_cache_ntiles (TILE_CACHE_SIZE);

	  plasma (drawable, FALSE);

	  if (run_mode != GIMP_RUN_NONINTERACTIVE)
	    gimp_displays_flush ();

	  /*  Store data  */
	  if (run_mode == GIMP_RUN_INTERACTIVE || 
	      (run_mode == GIMP_RUN_WITH_LAST_VALS))
            gimp_set_data ("plug_in_plasma", &pvals, sizeof (PlasmaValues));
	}
      else
	{
	  /* gimp_message ("plasma: cannot operate on indexed color images"); */
	  status = GIMP_PDB_EXECUTION_ERROR;
	}
    }

  values[0].data.d_status = status;
  gimp_drawable_detach (drawable);
}

static gint
plasma_dialog (GimpDrawable *drawable, GimpImageType drawable_type)
{
  GtkWidget *dlg;
  GtkWidget *main_vbox;
  GtkWidget *abox;
  GtkWidget *frame;
  GtkWidget *label;
  GtkWidget *table;
  GtkWidget *seed;
  GtkObject *adj;

  gimp_ui_init ("plasma", TRUE);
  
  dlg = gimp_dialog_new (_("Plasma"), "plasma",
			 gimp_standard_help_func, "filters/plasma.html",
			 GTK_WIN_POS_MOUSE,
			 FALSE, TRUE, FALSE,

			 GTK_STOCK_CANCEL, gtk_widget_destroy,
			 NULL, 1, NULL, FALSE, TRUE,
			 GTK_STOCK_OK, plasma_ok_callback,
			 NULL, NULL, NULL, TRUE, FALSE,

			 NULL);

  g_signal_connect (G_OBJECT (dlg), "destroy",
                    G_CALLBACK (gtk_main_quit),
                    NULL);

  gimp_help_init ();

  main_vbox = gtk_vbox_new (FALSE, 4);
  gtk_container_set_border_width (GTK_CONTAINER (main_vbox), 6);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dlg)->vbox), main_vbox, TRUE, TRUE, 0);
  gtk_widget_show (main_vbox);

  /* make a nice preview frame */
  frame = gtk_frame_new (_("Preview"));
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

  preview = preview_widget (drawable_type); /* we are here */
  gtk_container_add (GTK_CONTAINER (frame), preview);

  plasma (drawable, TRUE); /* preview image */

  gtk_widget_show (preview);
  
  /*  parameter settings  */
  frame = gtk_frame_new (_("Parameter Settings"));
  gtk_box_pack_start (GTK_BOX (main_vbox), frame, TRUE, TRUE, 0);
  gtk_widget_show (frame);

  table = gtk_table_new (2, 3, FALSE);
  gtk_table_set_col_spacings (GTK_TABLE (table), 4);
  gtk_table_set_row_spacings (GTK_TABLE (table), 2);
  gtk_container_set_border_width (GTK_CONTAINER (table), 4);
  gtk_container_add (GTK_CONTAINER (frame), table);
  gtk_widget_show (table);

  seed = gimp_random_seed_new (&pvals.seed);
  label = gimp_table_attach_aligned (GTK_TABLE (table), 0, 0,
				     _("Random _Seed:"), 1.0, 0.5,
				     seed, 1, TRUE);
  gtk_label_set_mnemonic_widget (GTK_LABEL (label),
				 GIMP_RANDOM_SEED_SPINBUTTON (seed));

  g_signal_connect_swapped (G_OBJECT (GIMP_RANDOM_SEED_SPINBUTTON_ADJ (seed)),
                            "value_changed",
                            G_CALLBACK (plasma_seed_changed_callback),
                            drawable);

  adj = gimp_scale_entry_new (GTK_TABLE (table), 0, 1,
			      _("T_urbulence:"), SCALE_WIDTH, 0,
			      pvals.turbulence,
			      0.1, 7.0, 0.1, 1.0, 1,
			      TRUE, 0, 0,
			      NULL, NULL);
  g_signal_connect (G_OBJECT (adj), "value_changed",
                    G_CALLBACK (gimp_double_adjustment_update),
                    &pvals.turbulence);
  g_signal_connect_swapped (G_OBJECT (adj), "value_changed",
                            G_CALLBACK (plasma_seed_changed_callback),
                            NULL);

  gtk_widget_show (dlg);

  gtk_main ();
  gimp_help_free ();
  gdk_flush ();

  return pint.run;
}

static void
plasma_ok_callback (GtkWidget *widget,
		    gpointer   data)
{
  pint.run = TRUE;

  gtk_widget_destroy (GTK_WIDGET (data));
}

static void
plasma_seed_changed_callback (GimpDrawable *drawable,
                              gpointer      data)
{
  plasma(drawable, TRUE);
}

#define AVE(n, v1, v2) n[0] = ((gint)v1[0] + (gint)v2[0]) / 2;\
	n[1] = ((gint)v1[1] + (gint)v2[1]) / 2;\
	n[2] = ((gint)v1[2] + (gint)v2[2]) / 2;

/*
 * Some glabals to save passing too many paramaters that don't change.
 */

static gint	ix1, iy1, ix2, iy2;	/* Selected image size. */
static gint	bpp, has_alpha, alpha;
static gdouble	turbulence;
static glong	max_progress, progress;

/*
 * The setup function.
 */

static void
plasma (GimpDrawable *drawable, 
	gboolean    preview_mode)
{
  GimpPixelFetcher *pft;
  gint  depth;
  GRand *gr;
  
  gr = g_rand_new ();
  
  pft = init_plasma (drawable, preview_mode, gr);

  /*
   * This first time only puts in the seed pixels - one in each
   * corner, and one in the center of each edge, plus one in the
   * center of the image.
   */

  do_plasma (pft, ix1, iy1, ix2 - 1, iy2 - 1, -1, 0, gr);

  /*
   * Now we recurse through the images, going further each time.
   */
  depth = 1;
  while (!do_plasma (pft, ix1, iy1, ix2 - 1, iy2 - 1, depth, 0, gr))
    {
      depth ++;
    }
  end_plasma (drawable, pft, gr);
}

static GimpPixelFetcher*
init_plasma (GimpDrawable *drawable,
	     gboolean   preview_mode,
             GRand *gr)
{
  GimpPixelFetcher *pft;

  g_rand_set_seed (gr, pvals.seed);

  turbulence = pvals.turbulence;

  if (preview_mode) 
    {
      ix1 = iy1 = 0;
      ix2 = GTK_PREVIEW (preview)->buffer_width;
      iy2 = GTK_PREVIEW (preview)->buffer_height;
      bpp = GTK_PREVIEW (preview)->bpp;
      alpha = bpp;
      has_alpha = 0;
      work_buffer = g_malloc (GTK_PREVIEW (preview)->rowstride * iy2);
      memcpy (work_buffer, GTK_PREVIEW (preview)->buffer, GTK_PREVIEW (preview)->rowstride * iy2);
      pft = NULL;
    } 
  else 
    {
      gimp_drawable_mask_bounds (drawable->drawable_id, &ix1, &iy1, &ix2, &iy2);
      bpp = drawable->bpp;
      has_alpha = gimp_drawable_has_alpha (drawable->drawable_id);
      alpha = (has_alpha) ? bpp - 1 : bpp;
      pft = gimp_pixel_fetcher_new (drawable);
      gimp_pixel_fetcher_set_shadow (pft, TRUE);
    }

  max_progress = (ix2 - ix1) * (iy2 - iy1);
  progress = 0;

  return pft;
}

static void
end_plasma (GimpDrawable *drawable,
	    GimpPixelFetcher *pft,
            GRand     *gr)
{
  if (pft)
    {
      gimp_pixel_fetcher_destroy (pft);

      gimp_drawable_flush (drawable);
      gimp_drawable_merge_shadow (drawable->drawable_id, TRUE);
      gimp_drawable_update (drawable->drawable_id,
			    ix1, iy1, (ix2 - ix1), (iy2 - iy1));
    } 
  else 
    {
      memcpy (GTK_PREVIEW (preview)->buffer, work_buffer, 
	      GTK_PREVIEW (preview)->rowstride * iy2);
      g_free (work_buffer);
      gtk_widget_queue_draw (preview);
    }
  g_rand_free (gr);
}

static void
get_pixel (GimpPixelFetcher *pft,
	   gint       x,
	   gint       y,
	   guchar    *pixel)
{
  if (pft) 
    {
      gimp_pixel_fetcher_get_pixel (pft, x, y, pixel);
    }
  else 
    {
      x = CLAMP (x, ix1, ix2 - 1);
      y = CLAMP (y, iy1, iy2 - 1);
      memcpy (pixel, 
	      work_buffer + y * GTK_PREVIEW (preview)->rowstride + x * bpp, 
	      bpp);
    }
}

static void
put_pixel (GimpPixelFetcher *pft,
	   gint       x,
	   gint       y,
	   guchar    *pixel)
{
  if (pft)
    {
      gimp_pixel_fetcher_put_pixel (pft, x, y, pixel);
      progress++;
    }
  else
    {
      x = CLAMP (x, ix1, ix2 - 1);
      y = CLAMP (y, iy1, iy2 - 1);
      memcpy (work_buffer + y * GTK_PREVIEW (preview)->rowstride + x * bpp, 
	      pixel, bpp);
    }
}

static void
random_rgb (GRand *gr, 
            guchar *d)
{
  gint i;

  for(i = 0; i < alpha; i++)
    {
      d[i] = g_rand_int_range (gr, 0, 256);
    }
  if (has_alpha)
    d[alpha] = 255;
}

static void
add_random (GRand *gr,
            guchar *d,
	    gint    amnt)
{
  gint i, tmp;

  if (amnt == 0)
    {
      amnt = 1;
    }

  for (i = 0; i < alpha; i++)
    {
      tmp = d[i] + g_rand_int_range(gr, -amnt/2, amnt/2);
      d[i] = CLAMP0255 (tmp);
    }
}

static gint
do_plasma (GimpPixelFetcher *pft,
	   gint       x1,
	   gint       y1,
	   gint       x2,
	   gint       y2,
	   gint       depth,
	   gint       scale_depth,
           GRand     *gr)
{
  guchar  tl[4], ml[4], bl[4], mt[4], mm[4], mb[4], tr[4], mr[4], br[4];
  guchar  tmp[4];
  gint    ran;
  gint    xm, ym;

  static gint count = 0;

  /* Initial pass through - no averaging. */

  if (depth == -1)
    {
      random_rgb (gr, tl);
      put_pixel (pft, x1, y1, tl);
      random_rgb (gr, tr);
      put_pixel (pft, x2, y1, tr);
      random_rgb (gr, bl);
      put_pixel (pft, x1, y2, bl);
      random_rgb (gr, br);
      put_pixel (pft, x2, y2, br);
      random_rgb (gr, mm);
      put_pixel (pft, (x1 + x2) / 2, (y1 + y2) / 2, mm);
      random_rgb (gr, ml);
      put_pixel (pft, x1, (y1 + y2) / 2, ml);
      random_rgb (gr, mr);
      put_pixel (pft, x2, (y1 + y2) / 2, mr);
      random_rgb (gr, mt);
      put_pixel (pft, (x1 + x2) / 2, y1, mt);
      random_rgb (gr, ml);
      put_pixel (pft, (x1 + x2) / 2, y2, ml);

      return 0;
    }

  /*
   * Some later pass, at the bottom of this pass,
   * with averaging at this depth.
   */
  if (depth == 0)
    {
      gdouble	rnd;
      gint	xave, yave;

      get_pixel (pft, x1, y1, tl);
      get_pixel (pft, x1, y2, bl);
      get_pixel (pft, x2, y1, tr);
      get_pixel (pft, x2, y2, br);

      rnd = (256.0 / (2.0 * (gdouble)scale_depth)) * turbulence;
      ran = rnd;

      xave = (x1 + x2) / 2;
      yave = (y1 + y2) / 2;

      if (xave == x1 && xave == x2 && yave == y1 && yave == y2)
	{
	  return 0;
	}

      if (xave != x1 || xave != x2)
	{
	  /* Left. */
	  AVE (ml, tl, bl);
	  add_random (gr, ml, ran);
	  put_pixel (pft, x1, yave, ml);

	  if (x1 != x2)
	    {
	      /* Right. */
	      AVE (mr, tr, br);
	      add_random (gr, mr, ran);
	      put_pixel (pft, x2, yave, mr);
	    }
	}

      if (yave != y1 || yave != y2)
	{
	  if (x1 != xave || yave != y2)
	    {
	      /* Bottom. */
	      AVE (mb, bl, br);
	      add_random (gr, mb, ran);
	      put_pixel (pft, xave, y2, mb);
	    }

	  if (y1 != y2)
	    {
	      /* Top. */
	      AVE (mt, tl, tr);
	      add_random (gr, mt, ran);
	      put_pixel (pft, xave, y1, mt);
	    }
	}

      if (y1 != y2 || x1 != x2)
	{
	  /* Middle pixel. */
	  AVE (mm, tl, br);
	  AVE (tmp, bl, tr);
	  AVE (mm, mm, tmp);

	  add_random (gr, mm, ran);
	  put_pixel (pft, xave, yave, mm);
	}

      count ++;

      if (!(count % 2000) && pft)
	{
	  gimp_progress_update ((double) progress / (double) max_progress);
	}

      if ((x2 - x1) < 3 && (y2 - y1) < 3)
	{
	  return 1;
	}
      return 0;
    }

  xm = (x1 + x2) >> 1;
  ym = (y1 + y2) >> 1;

  /* Top left. */
  do_plasma (pft, x1, y1, xm, ym, depth - 1, scale_depth + 1, gr);
  /* Bottom left. */
  do_plasma (pft, x1, ym, xm ,y2, depth - 1, scale_depth + 1, gr);
  /* Top right. */
  do_plasma (pft, xm, y1, x2 , ym, depth - 1, scale_depth + 1, gr);
  /* Bottom right. */
  return do_plasma (pft, xm, ym, x2, y2, depth - 1, scale_depth + 1, gr);
}


/* preview library */


static GtkWidget *
preview_widget (GimpImageType drawable_type)
{
  GtkWidget *preview;
  guchar    *buf;
  gint       y;

  if (drawable_type == GIMP_GRAY_IMAGE || 
           drawable_type == GIMP_GRAYA_IMAGE)
  {
    preview = gtk_preview_new (GTK_PREVIEW_GRAYSCALE);
    buf = g_malloc0 (PREVIEW_SIZE);
  }
  else /* Gray & colour are the only possibilities here */
  {
    preview = gtk_preview_new (GTK_PREVIEW_COLOR);
    buf = g_malloc0 (PREVIEW_SIZE * 3);
  }

  gtk_preview_size (GTK_PREVIEW (preview), PREVIEW_SIZE, PREVIEW_SIZE);
  
  for (y = 0; y < PREVIEW_SIZE; y++) 
    gtk_preview_draw_row (GTK_PREVIEW (preview), buf, 0, y, PREVIEW_SIZE);
  
  g_free (buf);
  
  return preview;
}

