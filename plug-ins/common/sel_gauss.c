/* Selective gaussian blur filter for the GIMP, version 0.1
 * Adapted from the original gaussian blur filter by Spencer Kimball and
 * Peter Mattis.
 *
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 * Copyright (C) 1999 Thom van Os <thom@vanos.com>
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
 *
 * Changelog:
 *
 * v0.1 	990202, TVO
 * 	First release
 *
 * To do:
 *	- support for horizontal or vertical only blur
 *	- use memory more efficiently, smaller regions at a time
 *	- integrating with other convolution matrix based filters ?
 *	- create more selective and adaptive filters
 *	- threading
 *	- optimization
 *
 */
#include "config.h"

#include <stdio.h>
#include <stdlib.h>

#include <gtk/gtk.h>

#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

#include "libgimp/stdplugins-intl.h"


typedef struct
{
  gdouble radius;
  gint    maxdelta;
} BlurValues;

typedef struct
{
  gint run;
} BlurInterface;


/* Declare local functions.
 */
static void   query  (void);
static void   run    (gchar     *name,
		      gint       nparams,
		      GimpParam     *param,
		      gint      *nreturn_vals,
		      GimpParam   **return_vals);

static void   sel_gauss (GimpDrawable *drawable,
			 gdouble    radius,
			 gint       maxdelta);

static gint   sel_gauss_dialog      (void);
static void   sel_gauss_ok_callback (GtkWidget *widget,
				     gpointer   data);

GimpPlugInInfo PLUG_IN_INFO =
{
  NULL,  /* init_proc  */
  NULL,  /* quit_proc  */
  query, /* query_proc */
  run,   /* run_proc   */
};

static BlurValues bvals =
{
  5.0, /* radius   */
  50   /* maxdelta */
};

static BlurInterface bint =
{
  FALSE  /* run */
};

MAIN ()

static void
query (void)
{
  static GimpParamDef args[] =
  {
    { GIMP_PDB_INT32, "run_mode", "Interactive, non-interactive" },
    { GIMP_PDB_IMAGE, "image", "Input image (unused)" },
    { GIMP_PDB_DRAWABLE, "drawable", "Input drawable" },
    { GIMP_PDB_FLOAT, "radius", "Radius of gaussian blur (in pixels > 1.0)" },
    { GIMP_PDB_INT32, "maxdelta", "Maximum delta" }
  };
  static gint nargs = sizeof (args) / sizeof (args[0]);

  gimp_install_procedure ("plug_in_sel_gauss",
			  "Applies a selective gaussian blur to the specified drawable.",
			  "This filter functions similar to the regular "
			  "gaussian blur filter except that neighbouring "
			  "pixels that differ more than the given maxdelta "
			  "parameter will not be blended with. This way with "
			  "the correct parameters, an image can be smoothed "
			  "out without losing details. However, this filter "
			  "can be rather slow.",
			  "Thom van Os",
			  "Thom van Os",
			  "1999",
			  N_("<Image>/Filters/Blur/Selective Gaussian Blur..."),
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
  static GimpParam	values[1];
  GimpRunModeType	run_mode;
  GimpPDBStatusType	status = GIMP_PDB_SUCCESS;
  GimpDrawable	*drawable;
  gdouble	radius;

  run_mode = param[0].data.d_int32;

  *nreturn_vals = 1;
  *return_vals = values;

  INIT_I18N_UI(); 

  values[0].type = GIMP_PDB_STATUS;
  values[0].data.d_status = status;

  switch (run_mode)
    {
    case GIMP_RUN_INTERACTIVE:
      /* Possibly retrieve data */
      gimp_get_data ("plug_in_sel_gauss", &bvals);

      /* First acquire information with a dialog */
      if (! sel_gauss_dialog ())
	return;
      break;

    case GIMP_RUN_NONINTERACTIVE:
      /* Make sure all the arguments are there! */
      if (nparams != 7)
	status = GIMP_PDB_CALLING_ERROR;
      if (status == GIMP_PDB_SUCCESS)
	{
	  bvals.radius   = param[3].data.d_float;
	  bvals.maxdelta = CLAMP (param[4].data.d_int32, 0, 255);
	}
      if (status == GIMP_PDB_SUCCESS && (bvals.radius < 1.0))
	status = GIMP_PDB_CALLING_ERROR;
      break;

    case GIMP_RUN_WITH_LAST_VALS:
      /* Possibly retrieve data */
      gimp_get_data ("plug_in_sel_gauss", &bvals);
      break;

    default:
      break;
    }

  if (status != GIMP_PDB_SUCCESS)
    {
      values[0].data.d_status = status;
      return;
    }

  /* Get the specified drawable */
  drawable = gimp_drawable_get (param[2].data.d_drawable);

  /* Make sure that the drawable is gray or RGB color */
  if (gimp_drawable_is_rgb (drawable->id) ||
      gimp_drawable_is_gray (drawable->id))
    {
      gimp_progress_init (_("Selective Gaussian Blur"));

      radius = fabs (bvals.radius) + 1.0;

      /* run the gaussian blur */
      sel_gauss (drawable, radius, bvals.maxdelta);

      /* Store data */
      if (run_mode == GIMP_RUN_INTERACTIVE)
	gimp_set_data ("plug_in_sel_gauss",
		       &bvals, sizeof (BlurValues));

      if (run_mode != GIMP_RUN_NONINTERACTIVE)
	gimp_displays_flush ();
    }
  else
    {
      gimp_message (_("sel_gauss: Cannot operate on indexed color images"));
      status = GIMP_PDB_EXECUTION_ERROR;
    }

  gimp_drawable_detach (drawable);
  values[0].data.d_status = status;
}

static gint
sel_gauss_dialog (void)
{
  GtkWidget *dlg;
  GtkWidget *frame;
  GtkWidget *table;
  GtkWidget *spinbutton;
  GtkObject *adj;

  gimp_ui_init ("sel_gauss", FALSE);

  dlg = gimp_dialog_new (_("Selective Gaussian Blur"), "sel_gauss",
			 gimp_standard_help_func, "filters/sel_gauss.html",
			 GTK_WIN_POS_MOUSE,
			 FALSE, TRUE, FALSE,

			 _("OK"), sel_gauss_ok_callback,
			 NULL, NULL, NULL, TRUE, FALSE,
			 _("Cancel"), gtk_widget_destroy,
			 NULL, 1, NULL, FALSE, TRUE,

			 NULL);

  gtk_signal_connect (GTK_OBJECT (dlg), "destroy",
		      GTK_SIGNAL_FUNC (gtk_main_quit),
		      NULL);

  /* parameter settings */
  frame = gtk_frame_new (_("Parameter Settings"));
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_ETCHED_IN);
  gtk_container_set_border_width (GTK_CONTAINER (frame), 6);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dlg)->vbox), frame,
		      TRUE, TRUE, 0);

  table = gtk_table_new (2, 3, FALSE);
  gtk_table_set_col_spacings (GTK_TABLE (table), 4);
  gtk_table_set_row_spacings (GTK_TABLE (table), 2);
  gtk_container_set_border_width (GTK_CONTAINER (table), 4);
  gtk_container_add (GTK_CONTAINER (frame), table);

  spinbutton = gimp_spin_button_new (&adj,
				     bvals.radius, 1.0, G_MAXINT, 1.0, 5.0,
				     0, 1, 2);
  gimp_table_attach_aligned (GTK_TABLE (table), 0, 0,
			     _("Blur Radius:"), 1.0, 0.5,
			     spinbutton, 1, TRUE);
  gtk_signal_connect (GTK_OBJECT (adj), "value_changed",
		      GTK_SIGNAL_FUNC (gimp_double_adjustment_update),
		      &bvals.radius);

  adj = gimp_scale_entry_new (GTK_TABLE (table), 0, 1,
			      _("Max. Delta:"), 128, 0,
			      bvals.maxdelta, 0, 255, 1, 8, 0,
			      TRUE, 0, 0,
			      FALSE, FALSE);
  gtk_signal_connect (GTK_OBJECT (adj), "value_changed",
		      GTK_SIGNAL_FUNC (gimp_int_adjustment_update),
		      &bvals.maxdelta);

  gtk_widget_show (table);
  gtk_widget_show (frame);
  gtk_widget_show (dlg);

  gtk_main ();
  gdk_flush ();

  return bint.run;
}

static void
sel_gauss_ok_callback (GtkWidget *widget,
		       gpointer   data)
{
  bint.run = TRUE;

  gtk_widget_destroy (GTK_WIDGET (data));
}

static void
init_matrix (gdouble   radius,
	     gdouble **mat,
	     gint      num)
{
  gint    dx, dy;
  gdouble sd, c1, c2;

  /* This formula isn't really correct, but it'll do */
  sd = radius / 3.329042969;
  c1 = 1.0 / sqrt(2.0 * G_PI * sd);
  c2 = -2.0 * (sd * sd);

  for (dy=0; dy<num; dy++)
    {
      for (dx=dy; dx<num; dx++)
	{
	  mat[dx][dy] = c1 * exp(((dx*dx)+ (dy*dy))/ c2);
	  mat[dy][dx] = mat[dx][dy];
	}
    }
}

static void
matrixmult (guchar   *src,
	    guchar   *dest,
	    gint      width,
	    gint      height,
	    gdouble **mat,
	    gint      numrad,
	    gint      bytes,
	    gint      has_alpha,
	    gint      maxdelta)
{
  gint    i, j, b, nb, x, y;
  gint    six, dix, tmp;
  gdouble sum, fact, d, alpha=1.0;

  nb = bytes - (has_alpha?1:0);

  for (y = 0; y< height; y++)
    {
      for (x = 0; x< width; x++)
	{
	  dix = (bytes*((width*y)+x));
	  if (has_alpha)
	    dest[dix+nb] = src[dix+nb];
	  for (b=0; b<nb; b++)
	    {
	      sum = 0.0;
	      fact = 0.0;
	      for (i= 1-numrad; i<numrad; i++)
		{
		  if (((x+i)< 0) || ((x+i)>= width))
		    continue;
		  for (j= 1-numrad; j<numrad; j++)
		    {
		      if (((y+j)<0)||((y+j)>=height))
			continue;
		      six = (bytes*((width*(y+j))+x+i));
		      if (has_alpha)
			{
			  if (!src[six+nb])
			    continue;
			  alpha = (double)src[six+nb] / 255.0;
			}
		      tmp = src[dix+b] - src[six+b];
		      if ((tmp>maxdelta)||
			  (tmp<-maxdelta))
			continue;
		      d = mat[ABS(i)][ABS(j)];
		      if (has_alpha)
			{
			  d *= alpha;
			}
		      sum += d * src[six+b];
		      fact += d;
		    }
		}
	      if (fact == 0.0)
		dest[dix+b] = src[dix+b];
	      else
		dest[dix+b] = sum/fact;
	    }
	}
      if (!(y % 5))
	gimp_progress_update((double)y / (double)height);
    }
}

static void
sel_gauss (GimpDrawable *drawable,
	   gdouble    radius,
	   gint       maxdelta)
{
  GimpPixelRgn src_rgn, dest_rgn;
  gint      width, height;
  gint      bytes;
  gint      has_alpha;
  guchar   *dest;
  guchar   *src;
  gint      x1, y1, x2, y2;
  gint      i;
  gdouble **mat;
  gint      numrad;

  gimp_drawable_mask_bounds (drawable->id, &x1, &y1, &x2, &y2);

  width  = (x2 - x1);
  height = (y2 - y1);
  bytes  = drawable->bpp;
  has_alpha = gimp_drawable_has_alpha(drawable->id);

  if ((width < 1) || (height < 1) || (bytes < 1))
    return;

  numrad = (gint) (radius + 1.0);
  mat = g_new (gdouble *, numrad);
  for (i = 0; i < numrad; i++)
    mat[i] = g_new (gdouble, numrad);
  init_matrix(radius, mat, numrad);

  src  = g_new (guchar, width * height * bytes);
  dest = g_new (guchar, width * height * bytes);

  gimp_pixel_rgn_init (&src_rgn, drawable, x1, y1, drawable->width,
		       drawable->height, FALSE, FALSE);

  gimp_pixel_rgn_get_rect (&src_rgn, src, x1, y1, width, height);

  matrixmult (src, dest, width, height, mat, numrad,
	      bytes, has_alpha, maxdelta);

  gimp_pixel_rgn_init (&dest_rgn, drawable, x1, y1, width, height,
		       TRUE, TRUE);
  gimp_pixel_rgn_set_rect (&dest_rgn, dest, x1, y1, width, height);

  /*  merge the shadow, update the drawable  */
  gimp_drawable_flush (drawable);
  gimp_drawable_merge_shadow (drawable->id, TRUE);
  gimp_drawable_update (drawable->id, x1, y1, width, height);

  /* free up buffers */
  g_free (src);
  g_free (dest);
  for (i = 0; i < numrad; i++)
    g_free (mat[i]);
  g_free (mat);
}
