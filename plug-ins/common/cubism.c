/* Cubism --- image filter plug-in for The Gimp image manipulation program
 * Copyright (C) 1996 Spencer Kimball, Tracy Scott
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
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 * You can contact me at quartic@polloux.fciencias.unam.mx
 * You can contact the original The Gimp authors at gimp@xcf.berkeley.edu
 */


#include <math.h>
#include <stdlib.h>
#include <string.h>
#include "gtk/gtk.h"
#include "libgimp/gimp.h"

/* Some useful macros */
#define SQR(a) ((a) * (a))

#ifndef M_PI
#define M_PI    3.14159265358979323846
#endif /* M_PI */

#define SCALE_WIDTH 125
#define BLACK 0
#define BG    1
#define SUPERSAMPLE 3
#define MAX_POINTS 4
#define MIN_ANGLE -36000
#define MAX_ANGLE 36000
#define RANDOMNESS 5

typedef struct
{
  gdouble x, y;
} Vertex;

typedef struct
{
  gint npts;
  Vertex pts[MAX_POINTS];
} Polygon;

typedef struct
{
  gdouble tile_size;
  gdouble tile_saturation;
  gint    bg_color;
} CubismVals;

typedef struct
{
  gint run;
} CubismInterface;

/* Declare local functions.
 */
static void      query  (void);
static void      run    (char       *name,
			 int         nparams,
			 GParam     *param,
			 int        *nreturn_vals,
			 GParam    **return_vals);
static void      cubism (GDrawable  *drawable);

static gint      cubism_dialog        (void);
static void      render_cubism        (GDrawable * drawable);
static void      fill_poly_color      (Polygon *  poly,
				       GDrawable * drawable,
				       guchar *   col);
static void      convert_segment      (gint       x1,
				       gint       y1,
				       gint       x2,
				       gint       y2,
				       gint       offset,
				       gint *     min,
				       gint *     max);
static void      randomize_indices    (gint       count,
				       gint *     indices);
static gdouble   fp_rand              (gdouble    val);
static gint      int_rand             (gint       val);
static gdouble   calc_alpha_blend     (gdouble *  vec,
				       gdouble    dist,
				       gdouble    x,
				       gdouble    y);
static void      polygon_add_point    (Polygon *  poly,
				       gdouble    x,
				       gdouble    y);
static void      polygon_translate    (Polygon *  poly,
				       gdouble    tx,
				       gdouble    ty);
static void      polygon_rotate       (Polygon *  poly,
				       gdouble    theta);
static gint      polygon_extents      (Polygon *  poly,
				       gdouble *  min_x,
				       gdouble *  min_y,
				       gdouble *  max_x,
				       gdouble *  max_y);
static void      polygon_reset        (Polygon *  poly);

static void      cubism_close_callback  (GtkWidget *widget,
					 gpointer   data);
static void      cubism_ok_callback     (GtkWidget *widget,
					 gpointer   data);
static void      cubism_toggle_update   (GtkWidget *widget,
					 gpointer   data);
static void      cubism_scale_update    (GtkAdjustment *adjustment,
					 double        *scale_val);

/*
 *  Local variables
 */

static guchar bg_col[4];
static CubismVals cvals =
{
  10.0,        /* tile_size */
  2.5,         /* tile_saturation */
  BLACK        /* bg_color */
};

static CubismInterface cint =
{
  FALSE         /* run */
};

GPlugInInfo PLUG_IN_INFO =
{
  NULL,    /* init_proc */
  NULL,    /* quit_proc */
  query,   /* query_proc */
  run,     /* run_proc */
};


/*
 *  Functions
 */

MAIN ()

static void
query ()
{
  static GParamDef args[] =
  {
    { PARAM_INT32, "run_mode", "Interactive, non-interactive" },
    { PARAM_IMAGE, "image", "Input image" },
    { PARAM_DRAWABLE, "drawable", "Input drawable" },
    { PARAM_FLOAT, "tile_size", "Average diameter of each tile (in pixels)" },
    { PARAM_FLOAT, "tile_saturation", "Expand tiles by this amount" },
    { PARAM_INT32, "bg_color", "Background color: { BLACK (0), BG (1) }" }
  };
  static GParamDef *return_vals = NULL;

  static int nargs = sizeof (args) / sizeof (args[0]);
  static int nreturn_vals = 0;

  gimp_install_procedure ("plug_in_cubism",
			  "Convert the input drawable into a collection of rotated squares",
			  "Help not yet written for this plug-in",
			  "Spencer Kimball & Tracy Scott",
			  "Spencer Kimball & Tracy Scott",
			  "1996",
			  "<Image>/Filters/Artistic/Cubism",
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
  GDrawable *active_drawable;
  GRunModeType run_mode;
  GStatusType status = STATUS_SUCCESS;

  run_mode = param[0].data.d_int32;

  *nreturn_vals = 1;
  *return_vals = values;

  values[0].type = PARAM_STATUS;
  values[0].data.d_status = status;

  switch (run_mode)
    {
    case RUN_INTERACTIVE:
      /*  Possibly retrieve data  */
      gimp_get_data ("plug_in_cubism", &cvals);

      /*  First acquire information with a dialog  */
      if (! cubism_dialog ())
	return;
      break;

    case RUN_NONINTERACTIVE:
      /*  Make sure all the arguments are there!  */
      if (nparams != 6)
	status = STATUS_CALLING_ERROR;
      if (status == STATUS_SUCCESS)
	{
	  cvals.tile_size = param[3].data.d_float;
	  cvals.tile_saturation = param[4].data.d_float;
	  cvals.bg_color = param[5].data.d_int32;
	}
      if (status == STATUS_SUCCESS &&
	  (cvals.bg_color < BLACK || cvals.bg_color > BG))
	status = STATUS_CALLING_ERROR;
      break;

    case RUN_WITH_LAST_VALS:
      /*  Possibly retrieve data  */
      gimp_get_data ("plug_in_cubism", &cvals);
      break;

    default:
      break;
    }

  /*  get the active drawable  */
  active_drawable = gimp_drawable_get (param[2].data.d_drawable);

  /*  Render the cubism effect  */
  if ((status == STATUS_SUCCESS) &&
      (gimp_drawable_color (active_drawable->id) || gimp_drawable_gray (active_drawable->id)))
    {
      /*  set cache size  */
      gimp_tile_cache_ntiles (SQR (4 * cvals.tile_size * cvals.tile_saturation) / SQR (gimp_tile_width ()));

      cubism (active_drawable);

      /*  If the run mode is interactive, flush the displays  */
      if (run_mode != RUN_NONINTERACTIVE)
	gimp_displays_flush ();

      /*  Store mvals data  */
      if (run_mode == RUN_INTERACTIVE)
	gimp_set_data ("plug_in_cubism", &cvals, sizeof (CubismVals));
    }
  else if (status == STATUS_SUCCESS)
    {
      /* gimp_message ("cubism: cannot operate on indexed color images"); */
      status = STATUS_EXECUTION_ERROR;
    }

  values[0].data.d_status = status;

  gimp_drawable_detach (active_drawable);
}

static void
cubism (GDrawable *drawable)
{
  gint x1, y1, x2, y2;

  /*  find the drawable mask bounds  */
  gimp_drawable_mask_bounds (drawable->id, &x1, &y1, &x2, &y2);

  /*  determine the background color  */
  if (cvals.bg_color == BLACK)
    bg_col[0] = bg_col[1] = bg_col[2] = 0;
  else
    gimp_palette_get_background (&bg_col[0], &bg_col[1], &bg_col[2]);

  if (gimp_drawable_has_alpha (drawable->id))
    bg_col[drawable->bpp - 1] = 0;

  gimp_progress_init ("Cubistic Transformation");

  /*  render the cubism  */
  render_cubism (drawable);

  /*  merge the shadow, update the drawable  */
  gimp_drawable_flush (drawable);
  gimp_drawable_merge_shadow (drawable->id, TRUE);
  gimp_drawable_update (drawable->id, x1, y1, (x2 - x1), (y2 - y1));
}

static gint
cubism_dialog ()
{
  GtkWidget *dlg;
  GtkWidget *label;
  GtkWidget *button;
  GtkWidget *toggle;
  GtkWidget *scale;
  GtkWidget *frame;
  GtkWidget *table;
  GtkObject *scale_data;
  gchar **argv;
  gint argc;

  argc = 1;
  argv = g_new (gchar *, 1);
  argv[0] = g_strdup ("cubism");

  gtk_init (&argc, &argv);
  gtk_rc_parse (gimp_gtkrc ());

  dlg = gtk_dialog_new ();
  gtk_window_set_title (GTK_WINDOW (dlg), "Cubism");
  gtk_window_position (GTK_WINDOW (dlg), GTK_WIN_POS_MOUSE);
  gtk_signal_connect (GTK_OBJECT (dlg), "destroy",
		      (GtkSignalFunc) cubism_close_callback,
		      NULL);

  /*  Action area  */
  button = gtk_button_new_with_label ("OK");
  GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT);
  gtk_signal_connect (GTK_OBJECT (button), "clicked",
                      (GtkSignalFunc) cubism_ok_callback,
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
  table = gtk_table_new (3, 2, FALSE);
  gtk_container_border_width (GTK_CONTAINER (table), 10);
  gtk_container_add (GTK_CONTAINER (frame), table);

  toggle = gtk_check_button_new_with_label ("Use Background Color");
  gtk_table_attach (GTK_TABLE (table), toggle, 0, 2, 0, 1, GTK_FILL, 0, 0, 0);
  gtk_signal_connect (GTK_OBJECT (toggle), "toggled",
		      (GtkSignalFunc) cubism_toggle_update,
		      &cvals.bg_color);
  gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (toggle), (cvals.bg_color == BG));
  gtk_widget_show (toggle);

  label = gtk_label_new ("Tile Size");
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gtk_table_attach (GTK_TABLE (table), label, 0, 1, 1, 2, GTK_FILL, 0, 5, 0);
  scale_data = gtk_adjustment_new (cvals.tile_size, 0.0, 100.0, 1.0, 1.0, 0.0);
  scale = gtk_hscale_new (GTK_ADJUSTMENT (scale_data));
  gtk_widget_set_usize (scale, SCALE_WIDTH, 0);
  gtk_table_attach (GTK_TABLE (table), scale, 1, 2, 1, 2, GTK_FILL, 0, 0, 0);
  gtk_scale_set_value_pos (GTK_SCALE (scale), GTK_POS_TOP);
  gtk_scale_set_digits (GTK_SCALE (scale), 1);
  gtk_range_set_update_policy (GTK_RANGE (scale), GTK_UPDATE_DELAYED);
  gtk_signal_connect (GTK_OBJECT (scale_data), "value_changed",
		      (GtkSignalFunc) cubism_scale_update,
		      &cvals.tile_size);
  gtk_widget_show (label);
  gtk_widget_show (scale);

  label = gtk_label_new ("Tile Saturation");
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gtk_table_attach (GTK_TABLE (table), label, 0, 1, 2, 3, GTK_FILL, 0, 5, 0);
  scale_data = gtk_adjustment_new (cvals.tile_saturation, 0.0, 10.0, 0.1, 0.1, 0.0);
  scale = gtk_hscale_new (GTK_ADJUSTMENT (scale_data));
  gtk_widget_set_usize (scale, SCALE_WIDTH, 0);
  gtk_table_attach (GTK_TABLE (table), scale, 1, 2, 2, 3, GTK_FILL, 0, 0, 0);
  gtk_scale_set_value_pos (GTK_SCALE (scale), GTK_POS_TOP);
  gtk_scale_set_digits (GTK_SCALE (scale), 1);
  gtk_range_set_update_policy (GTK_RANGE (scale), GTK_UPDATE_DELAYED);
  gtk_signal_connect (GTK_OBJECT (scale_data), "value_changed",
		      (GtkSignalFunc) cubism_scale_update,
		      &cvals.tile_saturation);
  gtk_widget_show (label);
  gtk_widget_show (scale);

  gtk_widget_show (frame);
  gtk_widget_show (table);
  gtk_widget_show (dlg);

  gtk_main ();
  gdk_flush ();

  return cint.run;
}

static void
render_cubism (GDrawable *drawable)
{
  GPixelRgn src_rgn;
  gdouble img_area, tile_area;
  gdouble x, y;
  gdouble width, height;
  gdouble theta;
  gint ix, iy;
  gint rows, cols;
  gint i, j, count;
  gint num_tiles;
  gint x1, y1, x2, y2;
  Polygon poly;
  guchar col[4];
  guchar *dest;
  gint bytes;
  gint has_alpha;
  gint *random_indices;
  gpointer pr;

  has_alpha = gimp_drawable_has_alpha (drawable->id);
  bytes = drawable->bpp;
  gimp_drawable_mask_bounds (drawable->id, &x1, &y1, &x2, &y2);
  img_area = (x2 - x1) * (y2 - y1);
  tile_area = SQR (cvals.tile_size);

  cols = ((x2 - x1) + cvals.tile_size - 1) / cvals.tile_size;
  rows = ((y2 - y1) + cvals.tile_size - 1) / cvals.tile_size;

  /*  Fill the image with the background color  */
  gimp_pixel_rgn_init (&src_rgn, drawable, x1, y1, (x2 - x1), (y2 - y1), TRUE, TRUE);
  for (pr = gimp_pixel_rgns_register (1, &src_rgn); pr != NULL; pr = gimp_pixel_rgns_process (pr))
    {
      count = src_rgn.w * src_rgn.h;
      dest = src_rgn.data;

      while (count--)
	for (i = 0; i < bytes; i++)
	  *dest++ = bg_col[i];
    }

  num_tiles = (rows + 1) * (cols + 1);
  random_indices = (gint *) malloc (sizeof (gint) * num_tiles);
  for (i = 0; i < num_tiles; i++)
    random_indices[i] = i;

  randomize_indices (num_tiles, random_indices);

  count = 0;
  gimp_pixel_rgn_init (&src_rgn, drawable, x1, y1, (x2 - x1), (y2 - y1), FALSE, FALSE);

  while (count < num_tiles)
    {
      i = random_indices[count] / (cols + 1);
      j = random_indices[count] % (cols + 1);
      x = j * cvals.tile_size + (cvals.tile_size / 4.0) - fp_rand (cvals.tile_size/2.0) + x1;
      y = i * cvals.tile_size + (cvals.tile_size / 4.0) - fp_rand (cvals.tile_size/2.0) + y1;
      width = (cvals.tile_size + fp_rand (cvals.tile_size / 4.0) - cvals.tile_size / 8.0) *
	cvals.tile_saturation;
      height = (cvals.tile_size + fp_rand (cvals.tile_size / 4.0) - cvals.tile_size / 8.0) *
	cvals.tile_saturation;
      theta = fp_rand (2 * M_PI);
      polygon_reset (&poly);
      polygon_add_point (&poly, -width / 2.0, -height / 2.0);
      polygon_add_point (&poly, width / 2.0, -height / 2.0);
      polygon_add_point (&poly, width / 2.0, height / 2.0);
      polygon_add_point (&poly, -width / 2.0, height / 2.0);
      polygon_rotate (&poly, theta);
      polygon_translate (&poly, x, y);

      /*  bounds check on x, y  */
      ix = (int) x;
      iy = (int) y;
      if (ix < x1)
	ix = x1;
      if (ix >= x2)
	ix = x2 - 1;
      if (iy < y1)
	iy = y1;
      if (iy >= y2)
	iy = y2 - 1;

      gimp_pixel_rgn_get_pixel (&src_rgn, col, ix, iy);

      if (! has_alpha || (has_alpha && col[bytes - 1] != 0))
	fill_poly_color (&poly, drawable, col);

      count++;
      if ((count % 5) == 0)
	gimp_progress_update ((double) count / (double) num_tiles);
    }

  gimp_progress_update (1.0);
  free (random_indices);
}

static void
fill_poly_color (Polygon   *poly,
		 GDrawable *drawable,
		 guchar    *col)
{
  GPixelRgn src_rgn;
  gdouble dmin_x, dmin_y;
  gdouble dmax_x, dmax_y;
  gint xs, ys;
  gint xe, ye;
  gint min_x, min_y;
  gint max_x, max_y;
  gint size_x, size_y;
  gint * max_scanlines;
  gint * min_scanlines;
  gint * vals;
  gint val;
  gint alpha;
  gint bytes;
  guchar buf[4];
  gint b, i, j, k, x, y;
  gdouble sx, sy;
  gdouble ex, ey;
  gdouble xx, yy;
  gdouble vec[2];
  gdouble dist;
  gint supersample;
  gint supersample2;
  gint x1, y1, x2, y2;

  sx = poly->pts[0].x;
  sy = poly->pts[0].y;
  ex = poly->pts[1].x;
  ey = poly->pts[1].y;

  dist = sqrt (SQR (ex - sx) + SQR (ey - sy));
  if (dist > 0.0)
    {
      vec[0] = (ex - sx) / dist;
      vec[1] = (ey - sy) / dist;
    }

  supersample = SUPERSAMPLE;
  supersample2 = SQR (supersample);

  gimp_drawable_mask_bounds (drawable->id, &x1, &y1, &x2, &y2);
  bytes = drawable->bpp;

  polygon_extents (poly, &dmin_x, &dmin_y, &dmax_x, &dmax_y);
  min_x = (gint) dmin_x;
  min_y = (gint) dmin_y;
  max_x = (gint) dmax_x;
  max_y = (gint) dmax_y;

  size_y = (max_y - min_y) * supersample;
  size_x = (max_x - min_x) * supersample;

  min_scanlines = (gint *) malloc (sizeof (gint) * size_y);
  max_scanlines = (gint *) malloc (sizeof (gint) * size_y);
  for (i = 0; i < size_y; i++)
    {
      min_scanlines[i] = max_x * supersample;
      max_scanlines[i] = min_x * supersample;
    }

  for (i = 0; i < poly->npts; i++)
    {
      xs = (gint) ((i) ? poly->pts[i-1].x : poly->pts[poly->npts-1].x);
      ys = (gint) ((i) ? poly->pts[i-1].y : poly->pts[poly->npts-1].y);
      xe = (gint) poly->pts[i].x;
      ye = (gint) poly->pts[i].y;

      xs *= supersample;
      ys *= supersample;
      xe *= supersample;
      ye *= supersample;

      convert_segment (xs, ys, xe, ye, min_y * supersample,
		       min_scanlines, max_scanlines);
    }

  gimp_pixel_rgn_init (&src_rgn, drawable, 0, 0,
		       drawable->width, drawable->height,
		       TRUE, TRUE);

  vals = (gint *) malloc (sizeof (gint) * size_x);
  for (i = 0; i < size_y; i++)
    {
      if (! (i % supersample))
	memset (vals, 0, sizeof (gint) * size_x);

      yy = (gdouble) i / (gdouble) supersample + min_y;

      for (j = min_scanlines[i]; j < max_scanlines[i]; j++)
	{
	  x = j - min_x * supersample;
	  vals[x] += 255;
	}

      if (! ((i + 1) % supersample))
	{
	  y = (i / supersample) + min_y;

	  if (y >= y1 && y < y2)
	    {
	      for (j = 0; j < size_x; j += supersample)
		{
		  x = (j / supersample) + min_x;

		  if (x >= x1 && x < x2)
		    {
		      val = 0;
		      for (k = 0; k < supersample; k++)
			val += vals[j + k];
		      val /= supersample2;

		      if (val > 0)
			{
			  xx = (gdouble) j / (gdouble) supersample + min_x;
			  alpha = (gint) (val * calc_alpha_blend (vec, dist, xx - sx, yy - sy));

			  gimp_pixel_rgn_get_pixel (&src_rgn, buf, x, y);

			  for (b = 0; b < bytes; b++)
			    buf[b] = ((col[b] * alpha) + (buf[b] * (255 - alpha))) / 255;

			  gimp_pixel_rgn_set_pixel (&src_rgn, buf, x, y);
			}
		    }
		}
	    }
	}
    }

  free (vals);
  free (min_scanlines);
  free (max_scanlines);
}

static void
convert_segment (gint  x1,
		 gint  y1,
		 gint  x2,
		 gint  y2,
		 gint  offset,
		 gint *min,
		 gint *max)
{
  gint ydiff, y, tmp;
  gdouble xinc, xstart;

  if (y1 > y2)
    { tmp = y2; y2 = y1; y1 = tmp;
      tmp = x2; x2 = x1; x1 = tmp; }
  ydiff = (y2 - y1);

  if ( ydiff )
    {
      xinc = (gdouble) (x2 - x1) / (gdouble) ydiff;
      xstart = x1 + 0.5 * xinc;
      for (y = y1 ; y < y2; y++)
	{
	  if (xstart < min[y - offset])
	    min[y-offset] = xstart;
	  if (xstart > max[y - offset])
	    max[y-offset] = xstart;

	  xstart += xinc;
	}
    }
}

static gdouble
calc_alpha_blend (gdouble *vec,
		  gdouble  dist,
		  gdouble  x,
		  gdouble  y)
{
  gdouble r;

  if (!dist)
    return 1.0;
  else
    {
      r = (vec[1] * y + vec[0] * x) / dist;
      if (r < 0.2)
	r = 0.2;
      else if (r > 1.0)
	r = 1.0;
    }
  return r;
}

static void
randomize_indices (gint  count,
		   gint *indices)
{
  gint i;
  gint index1, index2;
  gint tmp;

  for (i = 0; i < count * RANDOMNESS; i++)
    {
      index1 = int_rand (count);
      index2 = int_rand (count);
      tmp = indices[index1];
      indices[index1] = indices[index2];
      indices[index2] = tmp;
    }
}

static gdouble
fp_rand (gdouble val)
{
  gdouble rand_val;

  rand_val = (gdouble) rand () / (gdouble) (RAND_MAX - 1);
  return rand_val * val;
}

static gint
int_rand (gint val)
{
  gint rand_val;

  rand_val = rand () % val;
  return rand_val;
}

static void
polygon_add_point (Polygon *poly,
		   gdouble  x,
		   gdouble  y)
{
  if (poly->npts < 12)
    {
      poly->pts[poly->npts].x = x;
      poly->pts[poly->npts].y = y;
      poly->npts++;
    }
  else
    g_print ("Unable to add additional point.\n");
}

static void
polygon_rotate (Polygon *poly,
		gdouble  theta)
{
  gint i;
  gdouble ct, st;
  gdouble ox, oy;

  ct = cos (theta);
  st = sin (theta);

  for (i = 0; i < poly->npts; i++)
    {
      ox = poly->pts[i].x;
      oy = poly->pts[i].y;
      poly->pts[i].x = ct * ox - st * oy;
      poly->pts[i].y = st * ox + ct * oy;
    }
}

static void
polygon_translate (Polygon *poly,
		   gdouble  tx,
		   gdouble  ty)
{
  gint i;

  for (i = 0; i < poly->npts; i++)
    {
      poly->pts[i].x += tx;
      poly->pts[i].y += ty;
    }
}

static gint
polygon_extents (Polygon *poly,
		 gdouble *x1,
		 gdouble *y1,
		 gdouble *x2,
		 gdouble *y2)
{
  gint i;

  if (!poly->npts)
    return 0;

  *x1 = *x2 = poly->pts[0].x;
  *y1 = *y2 = poly->pts[0].y;

  for (i = 1; i < poly->npts; i++)
    {
      if (poly->pts[i].x < *x1)
	*x1 = poly->pts[i].x;
      if (poly->pts[i].x > *x2)
	*x2 = poly->pts[i].x;
      if (poly->pts[i].y < *y1)
	*y1 = poly->pts[i].y;
      if (poly->pts[i].y > *y2)
	*y2 = poly->pts[i].y;
    }

  return 1;
}

static void
polygon_reset (Polygon *poly)
{
  poly->npts = 0;
}

/*  Cubism interface functions  */

static void
cubism_close_callback (GtkWidget *widget,
		       gpointer   data)
{
  gtk_main_quit ();
}

static void
cubism_ok_callback (GtkWidget *widget,
		    gpointer   data)
{
  cint.run = TRUE;
  gtk_widget_destroy (GTK_WIDGET (data));
}

static void
cubism_toggle_update (GtkWidget *widget,
		      gpointer   data)
{
  int *toggle_val;

  toggle_val = (int *) data;

  if (GTK_TOGGLE_BUTTON (widget)->active)
    *toggle_val = TRUE;
  else
    *toggle_val = FALSE;
}

static void
cubism_scale_update (GtkAdjustment *adjustment,
		     double        *scale_val)
{
  *scale_val = adjustment->value;
}
