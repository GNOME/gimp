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

/* IWarp  a plug-in for the GIMP
   Version 0.1 
   
   IWarp is a gimp plug-in for interactive image warping. To apply the 
   selected deformation to the image, press the left mouse button and 
   move the mouse pointer in the preview image.
   
   Copyright (C) 1997 Norbert Schmitz
   nobert.schmitz@student.uni-tuebingen.de

   Most of the gimp and gtk specific code is taken from other plug-ins 

   v0.11a
    animation of non-alpha layers (background) creates now layers with 
    alpha channel. (thanks to Adrian Likins for reporting this bug)

  v0.12 
    fixes a very bad bug. 
     (thanks to Arthur Hagen for reporting it)
    
*/

#include "config.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include <gtk/gtk.h>

#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

#include "libgimp/stdplugins-intl.h"


#define MAX_PREVIEW_WIDTH      256
#define MAX_PREVIEW_HEIGHT     256
#define MAX_DEFORM_AREA_RADIUS 100

#define SCALE_WIDTH    100
#define MAX_NUM_FRAMES 100

typedef enum
{
  GROW,
  SHRINK,
  MOVE,
  REMOVE,
  SWIRL_CCW,
  SWIRL_CW
} DeformMode;

typedef struct
{
  gint run;
} iwarp_interface;

typedef struct
{
  gint        deform_area_radius;
  gdouble     deform_amount;
  DeformMode  deform_mode;
  gboolean    do_bilinear;
  gboolean    do_supersample;
  gdouble     supersample_threshold;
  gint        max_supersample_depth;
} iwarp_vals_t; 


/* Declare local functions.
 */
static void      query  (void);
static void      run    (gchar     *name,
			 gint       nparams,
			 GParam    *param,
			 gint      *nreturn_vals,
			 GParam   **return_vals);

static void      iwarp                   (void);
static void      iwarp_frame             (void);

static gboolean  iwarp_dialog            (void);
static void      iwarp_animate_dialog    (GtkWidget *dlg,
					  GtkWidget *notebook);

static void      iwarp_settings_dialog   (GtkWidget *dlg,
					  GtkWidget *notebook);

static void      iwarp_ok_callback       (GtkWidget *widget,
					  gpointer   data);
static void      iwarp_reset_callback    (GtkWidget *widget,
					  gpointer   data);

static gint      iwarp_motion_callback   (GtkWidget *widget,
					  GdkEvent  *event);

static void      iwarp_update_preview    (gint       x0,
					  gint       y0,
					  gint       x1,
					  gint       y1);

static void      iwarp_get_pixel         (gint       x,
					  gint       y,
					  guchar    *pixel);

static void      iwarp_get_deform_vector (gdouble    x,
					  gdouble    y, 
					  gdouble   *xv,
					  gdouble   *yv);

static void      iwarp_get_point         (gdouble    x,
					  gdouble    y,
					  guchar    *color);

static gint      iwarp_supersample_test  (GimpVector2 *v0,
					  GimpVector2 *v1, 
					  GimpVector2 *v2,
					  GimpVector2 *v3);

static void      iwarp_getsample         (GimpVector2  v0,
					  GimpVector2  v1,
					  GimpVector2  v2,
					  GimpVector2  v3, 
					  gdouble      x,
					  gdouble      y,
					  gint        *sample,
					  gint        *cc,
					  gint         depth,
					  gdouble      scale);

static void      iwarp_supersample       (gint       sxl,
					  gint       syl,
					  gint       sxr,
					  gint       syr,
					  guchar    *dest_data,
					  gint       stride,
					  gint      *progress,
					  gint       max_progress);

static guchar    iwarp_transparent_color (gint       x,
					  gint       y);

static void      iwarp_cpy_images        (void);

static void      iwarp_preview_get_pixel (gint       x,
					  gint       y,
					  guchar   **color);

static void      iwarp_preview_get_point (gdouble    x,
					  gdouble    y,
					  guchar    *color);

static void      iwarp_deform            (gint       x,
					  gint       y,
					  gdouble    vx,
					  gdouble    vy);

static void      iwarp_move              (gint       x,
					  gint       y,
					  gint       xx,
					  gint       yy);


GPlugInInfo PLUG_IN_INFO =
{
  NULL,  /* init_proc  */
  NULL,  /* quit_proc  */
  query, /* query_proc */
  run,   /* run_proc   */
};

static iwarp_interface wint = 
{
  FALSE
};

static iwarp_vals_t iwarp_vals = 
{
  20,
  0.3,
  MOVE,
  TRUE,
  FALSE,
  2.0,
  2
}; 

static GDrawable   *drawable = NULL;
static GDrawable   *destdrawable = NULL;
static GtkWidget   *preview = NULL;
static guchar      *srcimage = NULL;
static guchar      *dstimage = NULL;
static gint         preview_width, preview_height;
static gint         sel_width, sel_height;
static gint         image_bpp;
static gint         preserve_trans;
static GimpVector2 *deform_vectors = NULL;
static GimpVector2 *deform_area_vectors = NULL;
static gint         lastx, lasty;
static gdouble      filter[MAX_DEFORM_AREA_RADIUS];
static gboolean     do_animate = FALSE;
static gboolean     do_animate_reverse = FALSE;
static gboolean     do_animate_ping_pong = FALSE;
static gdouble      supersample_threshold_2;
static gint         xl, yl, xh, yh;
static gint         tile_width, tile_height;
static GTile       *tile = NULL;
static gdouble      pre2img, img2pre;
static gint         preview_bpp;
static gdouble      animate_deform_value = 1.0;
static gint32       imageID;
static gint         animate_num_frames = 2;
static gint         frame_number;
static gint         layer_alpha;


MAIN ()

static void
query (void)
{
  static GParamDef args[] =
  {
    { PARAM_INT32, "run_mode", "Interactive, non-interactive" },
    { PARAM_IMAGE, "image", "Input image (unused)" },
    { PARAM_DRAWABLE, "drawable", "Input drawable" }
  };
  static gint nargs = sizeof (args) / sizeof (args[0]);

  gimp_install_procedure ("plug_in_iwarp",
			  "Interactive warping of the specified drawable",
			  "Interactive warping of the specified drawable",
			  "Norbert Schmitz",
			  "Norbert Schmitz",
			  "1997",
			  N_("<Image>/Filters/Distorts/IWarp..."),
			  "RGB*, GRAY*",
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
  GRunModeType run_mode;
  GStatusType status = STATUS_SUCCESS;

  run_mode = param[0].data.d_int32;

  /*  Get the specified drawable  */
  destdrawable = drawable = gimp_drawable_get (param[2].data.d_drawable);
  imageID = param[1].data.d_int32;

  /*  Make sure that the drawable is gray or RGB color  */
  if (gimp_drawable_is_rgb (drawable->id) ||
      gimp_drawable_is_gray (drawable->id))
    {
      switch (run_mode)
	{
        case RUN_INTERACTIVE:
          INIT_I18N_UI();
          gimp_get_data ("plug_in_iwarp", &iwarp_vals);
          gimp_tile_cache_ntiles (2 * (drawable->width + gimp_tile_width ()-1) /
				  gimp_tile_width ());
          if (iwarp_dialog())
	    iwarp();
          gimp_set_data ("plug_in_iwarp", &iwarp_vals, sizeof (iwarp_vals_t));
 	  gimp_displays_flush ();
	  break;

        case RUN_NONINTERACTIVE:
	  status = STATUS_CALLING_ERROR;
        break;

        case RUN_WITH_LAST_VALS:
	  status = STATUS_CALLING_ERROR;
        break;

        default:
	  break;
	}
    }
  else
    {
      status = STATUS_EXECUTION_ERROR;
    }

  *nreturn_vals = 1;
  *return_vals = values;

  values[0].type = PARAM_STATUS;
  values[0].data.d_status = status;

  gimp_drawable_detach (drawable);

  g_free (srcimage);
  g_free (dstimage);  
  g_free (deform_vectors);
  g_free (deform_area_vectors);
}

static void
iwarp_get_pixel (gint    x,
		 gint    y,
		 guchar *pixel)
{
 static gint  old_col = -1;
 static gint  old_row = -1;
 guchar      *data;
 gint         col, row;
 gint         i;
 
 if (x >= xl && x < xh && y  >= yl && y < yh)
   {
     col = x / tile_width;
     row = y / tile_height;
     if (col != old_col || row != old_row)
       {
	 gimp_tile_unref (tile, FALSE);
	 tile = gimp_drawable_get_tile (drawable, FALSE, row, col);
	 gimp_tile_ref (tile);
	 old_col = col;
	 old_row = row;
       }
     data =
       tile->data +
       (tile->ewidth * (y % tile_height) +
	x % tile_width) * image_bpp;
     for (i = 0; i < image_bpp; i++)
       *pixel++ = *data++;
   }
 else
   {
     pixel[0] = pixel[1] = pixel[2] = pixel[3] = 0;
   }
}

static void
iwarp_get_deform_vector (gdouble  x,
			 gdouble  y,
			 gdouble *xv,
			 gdouble *yv)
{
  gint    i, xi, yi;
  gdouble dx, dy, my0, my1, mx0, mx1;

  if (x >= 0 && x < (preview_width - 1) && y >= 0  && y < (preview_height - 1))
    { 
      xi = (gint) x;
      yi = (gint) y;
      dx = x-xi;
      dy = y-yi;
      i = (yi * preview_width + xi);
      mx0 =
	deform_vectors[i].x +
	(deform_vectors[i+1].x -
	 deform_vectors[i].x) * dx;
      mx1 =
	deform_vectors[i+preview_width].x +
	(deform_vectors[i+preview_width+1].x -
	 deform_vectors[i+preview_width].x) * dx;
      my0 =
	deform_vectors[i].y +
	dx * (deform_vectors[i+1].y -
	      deform_vectors[i].y);
      my1 =
	deform_vectors[i+preview_width].y +
	dx * (deform_vectors[i+preview_width+1].y -
	      deform_vectors[i+preview_width].y);
      *xv = mx0 + dy * (mx1 - mx0);
      *yv = my0 + dy * (my1 - my0); 
    }
  else
    {
      *xv = *yv = 0.0; 
    }
}

static void
iwarp_get_point (gdouble  x,
		 gdouble  y,
		 guchar  *color)
{
  gdouble dx, dy, m0, m1;
  guchar  p0[4], p1[4], p2[4], p3[4]; 
  gint    xi, yi, i;
  
  xi = (gint) x;
  yi = (gint) y;
  dx = x - xi;
  dy = y - yi; 
  iwarp_get_pixel (xi, yi, p0);
  iwarp_get_pixel (xi + 1, yi, p1);
  iwarp_get_pixel (xi, yi + 1, p2);
  iwarp_get_pixel (xi + 1, yi + 1, p3); 	         
  for (i = 0; i < image_bpp; i++)
    {
      m0 = p0[i] + dx * (p1[i] - p0[i]);
      m1 = p2[i] + dx * (p3[i] - p2[i]);
      color[i] = (guchar) (m0 + dy * (m1 - m0));
    }
} 

static gboolean
iwarp_supersample_test (GimpVector2 *v0,
			GimpVector2 *v1,
			GimpVector2 *v2,
			GimpVector2 *v3)
{
  gdouble dx, dy;
 
  dx = 1.0+v1->x - v0->x;
  dy = v1->y - v0->y;
  if (dx * dx + dy * dy > supersample_threshold_2)
    return TRUE;

  dx = 1.0+v2->x - v3->x;
  dy = v2->y - v3->y;
  if (dx*dx+dy*dy > supersample_threshold_2) 
    return TRUE;

  dx = v2->x - v0->x;
  dy = 1.0+v2->y - v0->y;
  if (dx*dx+dy*dy > supersample_threshold_2) 
    return TRUE;

  dx = v3->x - v1->x;
  dy = 1.0+v3->y - v1->y;
  if (dx*dx+dy*dy > supersample_threshold_2) 
    return TRUE; 

  return FALSE;
}

static void
iwarp_getsample (GimpVector2  v0,
		 GimpVector2  v1,
		 GimpVector2  v2,
		 GimpVector2  v3, 
		 gdouble      x,
		 gdouble      y,
		 gint        *sample,
		 gint        *cc,
		 gint         depth,
		 gdouble      scale)
{
  gint        i;
  gdouble     xv, yv;
  GimpVector2 v01, v13, v23, v02, vm;
  guchar      c[4];

  if ((depth >= iwarp_vals.max_supersample_depth) ||
      (!iwarp_supersample_test (&v0, &v1, &v2, &v3)))
    {
      iwarp_get_deform_vector (img2pre * (x - xl),
			       img2pre * (y - yl),
			       &xv, &yv);
      xv *= animate_deform_value;
      yv *= animate_deform_value;
      iwarp_get_point (pre2img * xv + x, pre2img * yv + y, c);
      for (i = 0;  i < image_bpp; i++)
	sample[i] += c[i];
      (*cc)++;
    }
 else
   {
     scale *= 0.5;
     iwarp_get_deform_vector (img2pre * (x - xl),
			      img2pre * (y - yl),
			      &xv, &yv);
     xv *= animate_deform_value;
     yv *= animate_deform_value;
     iwarp_get_point (pre2img * xv + x, pre2img * yv + y, c);
     for (i = 0;  i < image_bpp; i++)
       sample[i] += c[i];
     (*cc)++;
     vm.x = xv;
     vm.y = yv;

     iwarp_get_deform_vector (img2pre * (x - xl),
			      img2pre * (y - yl - scale),
			      &xv, &yv);
     xv *= animate_deform_value;
     yv *= animate_deform_value;
     v01.x = xv;
     v01.y = yv;
  
     iwarp_get_deform_vector (img2pre * (x - xl + scale),
			      img2pre * (y - yl),
			      &xv, &yv);
     xv *= animate_deform_value;
     yv *= animate_deform_value;
     v13.x = xv;
     v13.y = yv;
 
     iwarp_get_deform_vector (img2pre * (x - xl),
			      img2pre * (y - yl + scale),
			      &xv, &yv);
     xv *= animate_deform_value;
     yv *= animate_deform_value;
     v23.x = xv;
     v23.y = yv;

     iwarp_get_deform_vector (img2pre * (x - xl - scale),
			      img2pre * (y - yl),
			      &xv, &yv);
     xv *= animate_deform_value;
     yv *= animate_deform_value;
     v02.x = xv;
     v02.y = yv;
 
     iwarp_getsample (v0, v01, vm, v02,
		      x-scale, y-scale,
		      sample, cc, depth + 1,
		      scale);
     iwarp_getsample (v01, v1, v13, vm,
		      x + scale, y - scale,
		      sample, cc, depth + 1,
		      scale);
     iwarp_getsample (v02, vm, v23, v2,
		      x - scale, y + scale,
		      sample, cc, depth + 1,
		      scale);
     iwarp_getsample (vm, v13, v3, v23,
		      x + scale, y + scale,
		      sample, cc, depth + 1,
		      scale);
   }
}

static void
iwarp_supersample (gint    sxl,
		   gint    syl,
		   gint    sxr,
		   gint    syr,
		   guchar *dest_data,
		   gint    stride,
		   gint   *progress,
		   gint    max_progress)
{
  gint         i, wx, wy, col, row, cc;
  GimpVector2 *srow, *srow_old, *vh;
  gdouble      xv, yv;
  gint         color[4];
  guchar      *dest;
 
  wx = sxr - sxl + 1;
  wy = syr - syl + 1;
  srow     = g_new (GimpVector2, sxr - sxl + 1);
  srow_old = g_new (GimpVector2, sxr - sxl + 1);

  for (i = sxl; i < (sxr + 1); i++)
    {
      iwarp_get_deform_vector (img2pre * (-0.5 + i - xl),
			       img2pre * (-0.5 + syl - yl),
			       &xv, &yv);
      xv *= animate_deform_value;
      yv *= animate_deform_value;
      srow_old[i-sxl].x = xv;
      srow_old[i-sxl].y = yv;
    }
 
  for (col = syl; col < syr; col++)
    {
      iwarp_get_deform_vector (img2pre * (-0.5 + sxl - xl),
			       img2pre * (0.5 + col - yl),
			       &xv, &yv);
      xv *= animate_deform_value;
      yv *= animate_deform_value;
      srow[0].x = xv;
      srow[0].y = yv; 
      for (row = sxl; row <sxr; row++)
	{
	  iwarp_get_deform_vector (img2pre * (0.5 + row - xl),
				   img2pre * (0.5 + col - yl),
				   &xv, &yv);
	  xv *= animate_deform_value;
	  yv *= animate_deform_value;
	  srow[row-sxl+1].x = xv;
	  srow[row-sxl+1].y = yv;
	  cc = 0;
	  color[0] = color[1] = color[2] = color[3] = 0;
	  iwarp_getsample (srow_old[row-sxl], srow_old[row-sxl+1],
			   srow[row-sxl], srow[row-sxl+1],
			   row, col, color, &cc, 0, 1.0);
	  if (layer_alpha)
	    dest = dest_data + (col-syl) * (stride) + (row-sxl) * (image_bpp+1);
	  else 
	    dest = dest_data + (col-syl) * stride + (row-sxl) * image_bpp;
	  for (i = 0; i < image_bpp; i++)
	    *dest++ = color[i] / cc;
	  if (layer_alpha)
	    *dest++ = 255; 
	  (*progress)++;
	}

      gimp_progress_update ((gdouble) (*progress) / max_progress);
      vh = srow_old;
      srow_old = srow;
      srow = vh;   
    }

  g_free (srow);
  g_free (srow_old);
}

static void
iwarp_frame (void)
{
  GPixelRgn  dest_rgn;
  gpointer   pr;
  guchar    *dest_row, *dest;
  gint       row, col;
  gint       i;
  gint       progress, max_progress;
  guchar     color[4];
  gdouble    xv, yv;

  progress = 0;
  max_progress = (yh-yl)*(xh-xl);

  gimp_pixel_rgn_init (&dest_rgn, destdrawable,
		       xl, yl, xh-xl, yh-yl, TRUE, TRUE);

  if (!do_animate)
    gimp_progress_init (_("Warping..."));

  for (pr = gimp_pixel_rgns_register (1, &dest_rgn);
       pr != NULL;
       pr = gimp_pixel_rgns_process (pr))
    {
      dest_row = dest_rgn.data;
      if (!iwarp_vals.do_supersample)
	{
	  for (row = dest_rgn.y; row < (dest_rgn.y + dest_rgn.h); row++)
	    {
	      dest = dest_row;
	      for (col = dest_rgn.x; col < (dest_rgn.x + dest_rgn.w); col++)
		{
		  progress++;
		  iwarp_get_deform_vector (img2pre * (col -xl),
					   img2pre * (row -yl),
					   &xv ,&yv);
		  xv *= animate_deform_value;
		  yv *= animate_deform_value;
		  if (fabs(xv) > 0.0 || fabs(yv) > 0.0)
		    {
		      iwarp_get_point (pre2img * xv + col,
				       pre2img *yv + row,
				       color);
		      for (i = 0; i < image_bpp; i++)
			*dest++ = color[i];
		      if (layer_alpha)
			*dest++ = 255;
		    }
		  else
		    {
		      iwarp_get_pixel (col, row, color);
		      for (i = 0; i < image_bpp; i++)
			*dest++ = color[i];
		      if (layer_alpha)
			*dest++ = 255;
		    }
		}
	      dest_row += dest_rgn.rowstride;
	      gimp_progress_update ((gdouble) (progress) / max_progress);
	    }
	}
      else
	{
	  supersample_threshold_2 =
	    iwarp_vals.supersample_threshold * iwarp_vals.supersample_threshold; 
	  iwarp_supersample (dest_rgn.x, dest_rgn.y,
			     dest_rgn.x + dest_rgn.w, dest_rgn.y + dest_rgn.h,
			     dest_rgn.data,
			     dest_rgn.rowstride,
			     &progress, max_progress);
	} 
    }

  gimp_drawable_flush (destdrawable);
  gimp_drawable_merge_shadow (destdrawable->id, TRUE);
  gimp_drawable_update (destdrawable->id, xl, yl, (xh - xl), (yh - yl));
}

static void
iwarp (void)
{
  gint     i;
  gint32   layerID;
  gint32  *animlayers;
  gchar   *st;
  gdouble  delta;

  if (animate_num_frames > 1 && do_animate)
    {
      animlayers = g_new (gint32, animate_num_frames);
      if (do_animate_reverse)
	{
	  animate_deform_value = 1.0;
	  delta = -1.0 / (animate_num_frames - 1);
	}
      else
	{
	  animate_deform_value = 0.0;
	  delta = 1.0 / (animate_num_frames - 1);
	}
      layerID = gimp_image_get_active_layer (imageID);
      if (image_bpp == 1 || image_bpp == 3)
	layer_alpha = TRUE;
      else
	layer_alpha = FALSE;
      frame_number = 0;
      for (i = 0; i < animate_num_frames; i++)
	{
	  st = g_strdup_printf (_("Frame %d"), i);
	  animlayers[i] = gimp_layer_copy (layerID);
	  gimp_layer_set_name (animlayers[i], st);
	  g_free (st);
	  destdrawable = gimp_drawable_get (animlayers[i]);
	  st = g_strdup_printf (_("Warping Frame Nr %d ..."), frame_number);
	  gimp_progress_init (st);
	  g_free (st);
	  if (animate_deform_value >0.0)
	    iwarp_frame ();
	  gimp_image_add_layer (imageID, animlayers[i], 0);
	  animate_deform_value = animate_deform_value + delta;
	  frame_number++;
	}
      if (do_animate_ping_pong)
	{
	  st = g_strdup_printf (_("Warping Frame Nr %d ..."), frame_number);
	  gimp_progress_init (_("Ping Pong"));
	  g_free (st);
	  for (i = 0; i < animate_num_frames; i++)
	    {
	      gimp_progress_update ((gdouble) i / (animate_num_frames - 1));
	      layerID = gimp_layer_copy (animlayers[animate_num_frames-i-1]); 
	      st = g_strdup_printf (_("Frame %d"), i + animate_num_frames);
	      gimp_layer_set_name (layerID, st);
	      g_free (st);
	      gimp_image_add_layer (imageID, layerID, 0); 
	    }
	}
      g_free (animlayers);
    } 
  else
    {
      animate_deform_value = 1.0;
      iwarp_frame ();
    }
  if (tile != NULL)
    {
      gimp_tile_unref (tile, FALSE);
      tile = NULL;
    }
}

static guchar
iwarp_transparent_color (gint x,
			 gint y)
{
  if ((y % (GIMP_CHECK_SIZE * 4)) > (GIMP_CHECK_SIZE * 2))
    {
      if ((x % (GIMP_CHECK_SIZE * 4)) > (GIMP_CHECK_SIZE * 2))
	return GIMP_CHECK_DARK * 255;
      else
	return GIMP_CHECK_LIGHT * 255;
    }
  else
    {
      if ((x % (GIMP_CHECK_SIZE * 4)) < (GIMP_CHECK_SIZE * 2))
	return GIMP_CHECK_DARK * 255;
      else
	return GIMP_CHECK_LIGHT * 255;
    }
}

static void
iwarp_cpy_images (void)
{
  gint     i, j, k, p;
  gdouble  alpha;
  guchar  *srccolor, *dstcolor;
 
  if (image_bpp == 1 || image_bpp ==3) 
    {
      memcpy (dstimage, srcimage, preview_width * preview_height * preview_bpp);
    }
  else
    {
      for (i = 0; i< preview_width; i++) 
	for (j = 0; j< preview_height; j++)
	  {
	    p = (j * preview_width + i);
	    srccolor = srcimage + p * image_bpp;
	    alpha = (gdouble) srccolor[image_bpp-1] / 255;
	    dstcolor = dstimage + p * preview_bpp;
	    for (k = 0; k < preview_bpp; k++)
	      {
		*dstcolor++ = (guchar)
		  (alpha *srccolor[k]+(1.0-alpha) *
		   iwarp_transparent_color (i,j));
	      }
	  }  
    }
}

static void
iwarp_init (void)
{
  gint       y, x, xi, i;
  GPixelRgn  srcrgn;
  guchar    *pts;
  guchar    *linebuffer = NULL;
  gdouble    dx, dy;
 
  gimp_drawable_mask_bounds (drawable->id, &xl, &yl, &xh, &yh);
  sel_width = xh-xl;
  sel_height = yh-yl;
  image_bpp = gimp_drawable_bpp (drawable->id);
  if (gimp_drawable_is_layer (drawable->id))
    preserve_trans = (gimp_layer_get_preserve_transparency (drawable->id));
  else
    preserve_trans = FALSE;
  if (image_bpp < 3)
    preview_bpp = 1;
  else
    preview_bpp = 3;
  dx = (gdouble) sel_width / MAX_PREVIEW_WIDTH;
  dy = (gdouble) sel_height / MAX_PREVIEW_HEIGHT;
  if (dx >dy)
    pre2img = dx;
  else
    pre2img = dy;
  if (dx <=1.0 && dy <= 1.0)
    pre2img = 1.0;  
  img2pre = 1.0 / pre2img;
  preview_width = (gint) (sel_width / pre2img);
  preview_height = (gint) (sel_height / pre2img);
  tile_width = gimp_tile_width ();
  tile_height = gimp_tile_height ();

  srcimage = g_new (guchar, preview_width * preview_height * image_bpp);
  dstimage = g_new (guchar, preview_width * preview_height * preview_bpp);
  deform_vectors = g_new (GimpVector2, preview_width * preview_height);
  deform_area_vectors = g_new (GimpVector2,
			       (MAX_DEFORM_AREA_RADIUS * 2 + 1) *
			       (MAX_DEFORM_AREA_RADIUS * 2 + 1));
  linebuffer = g_new (guchar, sel_width * image_bpp);

  for (i = 0; i < preview_width * preview_height; i++) 
    deform_vectors[i].x = deform_vectors[i].y = 0.0;

  gimp_pixel_rgn_init (&srcrgn, drawable,
		       xl, yl, sel_width, sel_height, FALSE, FALSE);

  for (y = 0; y < preview_height; y++)
    {
      gimp_pixel_rgn_get_row (&srcrgn, linebuffer,
			      xl, (gint) (pre2img * y) + yl, sel_width);
      for (x = 0; x < preview_width; x++)
	{
	  pts = srcimage + (y * preview_width +x) * image_bpp;
	  xi = (gint) (pre2img * x);
	  for (i = 0; i < image_bpp; i++)
	    {
	      *pts++ = linebuffer[xi*image_bpp+i];
	    }
	}
    }
  iwarp_cpy_images ();
  for (i = 0; i < MAX_DEFORM_AREA_RADIUS; i++)
    {
      filter[i] = 
	pow ((cos (sqrt((gdouble) i / MAX_DEFORM_AREA_RADIUS) * G_PI) + 1) *
	     0.5, 0.7); /*0.7*/
    }
  g_free (linebuffer);
}

static void
iwarp_animate_dialog (GtkWidget *dlg,
		      GtkWidget *notebook)
{ 
  GtkWidget *vbox;
  GtkWidget *frame;
  GtkWidget *table;
  GtkWidget *button;
  GtkObject *scale_data;

  vbox = gtk_vbox_new (FALSE, 4);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), 6);

  button = gtk_check_button_new_with_label (_("Animate"));
  gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button), do_animate);
  gtk_signal_connect (GTK_OBJECT (button), "toggled",
                      GTK_SIGNAL_FUNC (gimp_toggle_button_update),
                      &do_animate);
  gtk_widget_show (button);

  frame = gtk_frame_new (NULL);
  gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  gtk_object_set_data (GTK_OBJECT (button), "set_sensitive", frame);
  gtk_widget_set_sensitive (frame, do_animate);

  table = gtk_table_new (3, 3, FALSE);
  gtk_container_set_border_width (GTK_CONTAINER (table), 4);
  gtk_table_set_row_spacings (GTK_TABLE (table), 2);
  gtk_table_set_col_spacings (GTK_TABLE (table), 4);
  gtk_container_add (GTK_CONTAINER (frame), table);
  gtk_widget_show (table);

  scale_data = gimp_scale_entry_new (GTK_TABLE (table), 0, 0,
				     _("Number of Frames:"), SCALE_WIDTH, 0,
				     animate_num_frames,
				     2, MAX_NUM_FRAMES, 1.0, 10.0, 1,
				     TRUE, 0, 0,
				     NULL, NULL);
  gtk_signal_connect (GTK_OBJECT (scale_data), "value_changed",
		      GTK_SIGNAL_FUNC (gimp_int_adjustment_update),
		      &animate_num_frames);

  button = gtk_check_button_new_with_label (_("Reverse"));
  gtk_table_attach (GTK_TABLE (table), button, 0, 3, 1, 2,
		    GTK_FILL | GTK_EXPAND, GTK_FILL | GTK_EXPAND, 0, 0); 
  gtk_signal_connect (GTK_OBJECT (button), "clicked",
                      GTK_SIGNAL_FUNC (gimp_toggle_button_update),
                      &do_animate_reverse);
  gtk_widget_show (button);
 
  button = gtk_check_button_new_with_label (_("Ping Pong"));
  gtk_table_attach (GTK_TABLE (table), button, 0, 3, 2, 3,
		    GTK_FILL | GTK_EXPAND, GTK_FILL | GTK_EXPAND, 0, 0); 
  gtk_signal_connect (GTK_OBJECT (button), "clicked",
                      GTK_SIGNAL_FUNC (gimp_toggle_button_update),
                      &do_animate_ping_pong);
  gtk_widget_show (button);

  gtk_widget_show (vbox);

  gtk_notebook_append_page (GTK_NOTEBOOK (notebook),
			    vbox,
			    gtk_label_new (_("Animate")));
}

static void
iwarp_settings_dialog (GtkWidget *dlg,
		       GtkWidget *notebook)
{
  GtkWidget *vbox;
  GtkWidget *vbox2;
  GtkWidget *vbox3;
  GtkWidget *hbox;
  GtkWidget *frame;
  GtkWidget *button;
  GtkWidget *table;
  GtkObject *scale_data;
  GtkWidget *widget[3];
  gint       i;

  vbox = gtk_vbox_new (FALSE, 4);
  gtk_container_border_width (GTK_CONTAINER (vbox), 6);

  table = gtk_table_new (2, 3, FALSE);
  gtk_table_set_row_spacings (GTK_TABLE (table), 2);
  gtk_table_set_col_spacings (GTK_TABLE (table), 4);
  gtk_box_pack_start (GTK_BOX (vbox), table, FALSE, FALSE, 0);
  gtk_widget_show (table);

  scale_data = gimp_scale_entry_new (GTK_TABLE (table), 0, 0,
				     _("Deform Radius:"), SCALE_WIDTH, 0,
				     iwarp_vals.deform_area_radius,
				     5.0, MAX_DEFORM_AREA_RADIUS, 1.0, 10.0, 0,
				     TRUE, 0, 0,
				     NULL, NULL);
  gtk_signal_connect (GTK_OBJECT (scale_data), "value_changed",
		      GTK_SIGNAL_FUNC (gimp_int_adjustment_update),
		      &iwarp_vals.deform_area_radius);
 
  scale_data = gimp_scale_entry_new (GTK_TABLE (table), 0, 1,
				     _("Deform Amount:"), SCALE_WIDTH, 0,
				     iwarp_vals.deform_amount,
				     0.0, 1.0, 0.01, 0.1, 2,
				     TRUE, 0, 0,
				     NULL, NULL);
  gtk_signal_connect (GTK_OBJECT (scale_data), "value_changed",
		      GTK_SIGNAL_FUNC (gimp_double_adjustment_update),
		      &iwarp_vals.deform_amount);

  frame = gtk_frame_new (_("Deform Mode"));
  gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  hbox = gtk_hbox_new (TRUE, 4);
  gtk_container_set_border_width (GTK_CONTAINER (hbox), 2);
  gtk_container_add (GTK_CONTAINER (frame), hbox);
  gtk_widget_show (hbox);

  vbox2 = gimp_radio_group_new2 (FALSE, NULL,
				 gimp_radio_button_update,
				 &iwarp_vals.deform_mode,
				 (gpointer) iwarp_vals.deform_mode,

				 _("Move"),      (gpointer) MOVE, NULL,
				 _("Grow"),      (gpointer) GROW, NULL,
				 _("Swirl CCW"), (gpointer) SWIRL_CCW, NULL,
				 _("Remove"),    (gpointer) REMOVE, &widget[0],
				 _("Shrink"),    (gpointer) SHRINK, &widget[1],
				 _("Swirl CW"),  (gpointer) SWIRL_CW, &widget[2],

				 NULL);
  gtk_container_add (GTK_CONTAINER (hbox), vbox2);
  gtk_widget_show (vbox2);

  vbox3 = gtk_vbox_new (FALSE, 1);
  gtk_container_add (GTK_CONTAINER (hbox), vbox3);
  gtk_widget_show (vbox3);

  for (i = 0; i < 3; i++)
    {
      gtk_object_ref (GTK_OBJECT (widget[i]));
      gtk_widget_hide (widget[i]);
      gtk_container_remove (GTK_CONTAINER (vbox2), widget[i]);
      gtk_box_pack_start (GTK_BOX (vbox3), widget[i],
                          FALSE, FALSE, 0);
      gtk_widget_show (widget[i]);
      gtk_object_unref (GTK_OBJECT (widget[i]));
    }

  button = gtk_check_button_new_with_label (_("Bilinear"));
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button),
				iwarp_vals.do_bilinear);
  gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);
  gtk_signal_connect (GTK_OBJECT (button), "toggled",
                      GTK_SIGNAL_FUNC (gimp_toggle_button_update),
                      &iwarp_vals.do_bilinear);
  gtk_widget_show(button);

  frame = gtk_frame_new (NULL);
  gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  vbox2 = gtk_vbox_new (FALSE, 4);
  gtk_container_set_border_width (GTK_CONTAINER (vbox2), 4);
  gtk_container_add (GTK_CONTAINER (frame), vbox2);
  gtk_widget_show (vbox2);

  button = gtk_check_button_new_with_label (_("Adaptive Supersample"));
  gtk_box_pack_start (GTK_BOX (vbox2), button, FALSE, FALSE, 0);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button),
				iwarp_vals.do_supersample);
  gtk_signal_connect (GTK_OBJECT (button), "toggled",
                      GTK_SIGNAL_FUNC (gimp_toggle_button_update),
                      &iwarp_vals.do_supersample);
  gtk_widget_show (button);

  table = gtk_table_new (2, 3, FALSE);
  gtk_table_set_row_spacings (GTK_TABLE (table), 2);
  gtk_table_set_col_spacings (GTK_TABLE (table), 4);
  gtk_box_pack_start (GTK_BOX (vbox2), table, FALSE, FALSE, 0);
  gtk_widget_show (table);

  gtk_object_set_data (GTK_OBJECT (button), "set_sensitive", table);
  gtk_widget_set_sensitive (table, iwarp_vals.do_supersample);

  scale_data = gimp_scale_entry_new (GTK_TABLE (table), 0, 0,
				     _("Max Depth:"), SCALE_WIDTH, 0,
				     iwarp_vals.max_supersample_depth,
				     1.0, 5.0, 1.1, 1.0, 0,
				     TRUE, 0, 0,
				     NULL, NULL);
  gtk_signal_connect (GTK_OBJECT (scale_data), "value_changed",
		      GTK_SIGNAL_FUNC (gimp_int_adjustment_update),
		      &iwarp_vals.max_supersample_depth);

  scale_data = gimp_scale_entry_new (GTK_TABLE (table), 0, 1,
				     _("Threshold:"), SCALE_WIDTH, 0,
				     iwarp_vals.supersample_threshold,
				     1.0, 10.0, 0.01, 0.1, 2,
				     TRUE, 0, 0,
				     NULL, NULL);
  gtk_signal_connect (GTK_OBJECT (scale_data), "value_changed",
		      GTK_SIGNAL_FUNC (gimp_double_adjustment_update),
		      &iwarp_vals.supersample_threshold);

  gtk_widget_show (vbox);

  gtk_notebook_append_page (GTK_NOTEBOOK (notebook),
			    vbox,
			    gtk_label_new (_("Settings")));
}

static gboolean
iwarp_dialog (void)
{
  GtkWidget *dlg;
  GtkWidget *main_hbox;
  GtkWidget *frame;
  GtkWidget *abox;
  GtkWidget *pframe;
  GtkWidget *notebook;

  gimp_ui_init ("iwarp", TRUE);

  iwarp_init ();
 
  dlg = gimp_dialog_new (_("IWarp"), "iwarp",
			 gimp_standard_help_func, "filters/iwarp.html",
			 GTK_WIN_POS_MOUSE,
			 FALSE, TRUE, FALSE,

			 _("OK"), iwarp_ok_callback,
			 NULL, NULL, NULL, TRUE, FALSE,
			 _("Reset"), iwarp_reset_callback,
			 NULL, NULL, NULL, FALSE, FALSE,
			 _("Cancel"), gtk_widget_destroy,
			 NULL, 1, NULL, FALSE, TRUE,

			 NULL);

  gtk_signal_connect (GTK_OBJECT (dlg), "destroy",
		      GTK_SIGNAL_FUNC (gtk_main_quit),
		      NULL);

  main_hbox = gtk_hbox_new (FALSE, 6);
  gtk_container_set_border_width (GTK_CONTAINER (main_hbox), 6);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dlg)->vbox), main_hbox,
		      FALSE, FALSE, 0);

  frame = gtk_frame_new (_("Preview"));
  gtk_box_pack_start (GTK_BOX (main_hbox), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  abox = gtk_alignment_new (0.5, 0.5, 0.0, 0.0);
  gtk_container_set_border_width (GTK_CONTAINER (abox), 4);
  gtk_container_add (GTK_CONTAINER (frame), abox);
  gtk_widget_show (abox);

  pframe = gtk_frame_new (NULL);
  gtk_frame_set_shadow_type (GTK_FRAME (pframe), GTK_SHADOW_IN);
  gtk_container_add (GTK_CONTAINER (abox), pframe);
  gtk_widget_show (pframe); 

  if (preview_bpp == 3)
    preview = gtk_preview_new (GTK_PREVIEW_COLOR);
  else
    preview = gtk_preview_new (GTK_PREVIEW_GRAYSCALE);
  gtk_preview_size (GTK_PREVIEW (preview), preview_width, preview_height);
  iwarp_update_preview (0, 0, preview_width, preview_height);
  gtk_container_add (GTK_CONTAINER (pframe), preview);
  gtk_widget_show (preview);
 
  gtk_widget_set_events (preview,
			 GDK_BUTTON_PRESS_MASK |
			 GDK_BUTTON_RELEASE_MASK |
			 GDK_BUTTON1_MOTION_MASK | 
			 GDK_POINTER_MOTION_HINT_MASK);
  gtk_signal_connect (GTK_OBJECT(preview), "event",
		      GTK_SIGNAL_FUNC (iwarp_motion_callback),
		      NULL);

  notebook = gtk_notebook_new ();
  gtk_notebook_set_tab_pos (GTK_NOTEBOOK (notebook), GTK_POS_TOP);
  gtk_box_pack_start (GTK_BOX (main_hbox), notebook, TRUE, TRUE, 0);

  iwarp_settings_dialog (dlg, notebook);
  iwarp_animate_dialog (dlg, notebook);

  gtk_widget_show (notebook);

  gtk_widget_show (main_hbox);
  gtk_widget_show (dlg);

  gtk_main ();
  gdk_flush ();

  return wint.run;   
}

static void 
iwarp_update_preview (gint x0,
		      gint y0,
		      gint x1,
		      gint y1)
{
  gint i;
  GdkRectangle rect;
 
  if (x0 < 0)
    x0 = 0;
  if (y0 < 0)
    y0 = 0;
  if (x1 >= preview_width)
    x1 = preview_width;
  if (y1 >= preview_height)
    y1 = preview_height;
  for (i = y0; i < y1; i++)
    gtk_preview_draw_row (GTK_PREVIEW (preview),
			  dstimage + (i * preview_width + x0) * preview_bpp,
			  x0, i,x1-x0);
  rect.x = x0;
  rect.y = y0;
  rect.width = x1-x0;
  rect.height = y1-y0;  
  gtk_widget_draw (preview, &rect);
  gdk_flush ();
}

static void
iwarp_preview_get_pixel (gint     x,
			 gint     y,
			 guchar **color)
{
  static guchar black[4] = { 0, 0, 0, 0 };

  if (x < 0 || x >= preview_width || y<0 || y >= preview_height)
    { 
      *color = black;
      return;
    }
  *color = srcimage + (y * preview_width + x) * image_bpp; 
} 

static void
iwarp_preview_get_point (gdouble  x,
			 gdouble  y,
			 guchar  *color)
{
  gint     xi, yi, j;
  gdouble  dx, dy, m0, m1;
  guchar  *p0, *p1, *p2, *p3;

  xi = (gint) x;
  yi = (gint) y;
  if (iwarp_vals.do_bilinear)
    {
      dx = x-xi;
      dy = y-yi; 
      iwarp_preview_get_pixel (xi, yi, &p0);
      iwarp_preview_get_pixel (xi + 1, yi, &p1);
      iwarp_preview_get_pixel (xi, yi + 1, &p2);
      iwarp_preview_get_pixel (xi + 1, yi + 1, &p3);
      for (j = 0; j < image_bpp; j++)
	{
	  m0 = p0[j] + dx * (p1[j] - p0[j]);
	  m1 = p2[j] + dx * (p3[j] - p2[j]);
	  color[j] = (guchar) (m0 + dy * (m1 - m0));  
	}
    }
  else
    {
      iwarp_preview_get_pixel (xi, yi, &p0);
      for (j = 0; j < image_bpp; j++)
	color[j] = p0[j];
    }
}

static void 
iwarp_deform (gint    x,
	      int     y,
	      gdouble vx,
	      gdouble vy)
{
  gint    xi, yi, ptr, fptr, x0, x1, y0, y1, radius2, length2;
  gdouble deform_value, xn, yn, nvx=0, nvy=0, emh, em, edge_width, xv, yv, alpha;
  guchar  color[4];
 
  if (x - iwarp_vals.deform_area_radius <0)
    x0 = -x;
  else
    x0 = -iwarp_vals.deform_area_radius;

  if (x + iwarp_vals.deform_area_radius >= preview_width)
    x1 = preview_width - x - 1;
  else
    x1 = iwarp_vals.deform_area_radius;

  if (y - iwarp_vals.deform_area_radius < 0)
    y0 = -y;
  else
    y0 = -iwarp_vals.deform_area_radius;

  if (y + iwarp_vals.deform_area_radius >= preview_height)
    y1 = preview_height-y-1;
  else
    y1 = iwarp_vals.deform_area_radius;

  radius2 = iwarp_vals.deform_area_radius * iwarp_vals.deform_area_radius;

  for (yi = y0; yi <= y1; yi++)
    for (xi = x0; xi <= x1; xi++)
      {
	length2 = (xi * xi + yi * yi) * MAX_DEFORM_AREA_RADIUS / radius2;
	if (length2 < MAX_DEFORM_AREA_RADIUS)
	  {
	    ptr = (y + yi) * preview_width + x + xi;   
	    fptr =
	      (yi + iwarp_vals.deform_area_radius) *
	      (iwarp_vals.deform_area_radius * 2 + 1) +
	      xi +
	      iwarp_vals.deform_area_radius;

	    switch (iwarp_vals.deform_mode)
	      {
	      case GROW:
		deform_value = filter[length2] *  0.1* iwarp_vals.deform_amount;
		nvx = -deform_value * xi;
		nvy = -deform_value * yi;
		break;

	      case SHRINK:
		deform_value = filter[length2] * 0.1* iwarp_vals.deform_amount;
		nvx = deform_value * xi;
		nvy = deform_value * yi;
		break;

	      case SWIRL_CW:
		deform_value = filter[length2] * iwarp_vals.deform_amount * 0.5;
		nvx =  deform_value * yi;
		nvy = -deform_value * xi;
		break;

	      case SWIRL_CCW:
		deform_value = filter[length2] *iwarp_vals.deform_amount * 0.5;
		nvx = -deform_value * yi;
		nvy =  deform_value  * xi;
		break;

	      case MOVE:
		deform_value = filter[length2] * iwarp_vals.deform_amount;
		nvx = deform_value * vx;
		nvy = deform_value * vy;
		break;

	      default:
		break;
	      }

	    if (iwarp_vals.deform_mode == REMOVE)
	      {
		deform_value =
		  1.0 - 0.5 * iwarp_vals.deform_amount * filter[length2];
		deform_area_vectors[fptr].x =
		  deform_value * deform_vectors[ptr].x ;
		deform_area_vectors[fptr].y =
		  deform_value * deform_vectors[ptr].y ;
	      } 
	    else
	      {
		edge_width = 0.2 *  iwarp_vals.deform_area_radius;
		emh = em = 1.0;
		if (x+xi < edge_width)
		  em = (gdouble) (x + xi) / edge_width;
		if (y + yi < edge_width)
		  emh = (gdouble) (y + yi) / edge_width;
		if (emh <em)
		  em = emh;
		if (preview_width - x - xi - 1 < edge_width)
		  emh = (gdouble) (preview_width - x - xi - 1) / edge_width;
		if (emh < em)
		  em = emh;
		if (preview_height-y-yi-1 < edge_width)
		  emh = (gdouble) (preview_height - y - yi - 1) / edge_width;
		if (emh <em)
		  em = emh;

		nvx = nvx * em;
		nvy = nvy * em;

		iwarp_get_deform_vector (nvx + x+ xi, nvy + y + yi, &xv, &yv);
		xv = nvx +xv;
		if (xv +x+xi <0.0)
		  xv = -x - xi;
		else if (xv + x +xi > (preview_width-1))
		  xv = preview_width - x -xi-1;
		yv = nvy +yv;
		if (yv + y + yi < 0.0)
		  yv = -y - yi;
		else if (yv + y + yi > (preview_height-1))
		  yv = preview_height - y -yi - 1;
		deform_area_vectors[fptr].x = xv;
		deform_area_vectors[fptr].y = yv;
	      }

	    xn = deform_area_vectors[fptr].x + x + xi;
	    yn = deform_area_vectors[fptr].y + y + yi;

	    /* gcc - shut up! */
	    alpha = 1;

	    /* Yeah, it is ugly but since color is a pointer into image-data
	     * I must not change color[image_bpp - 1]  ...
	     */
	    if (preserve_trans && (image_bpp == 4 || image_bpp == 2))
	      {
		iwarp_preview_get_point (x + xi, y + yi, color);
		alpha = (gdouble) color[image_bpp - 1] / 255;
	      }

	    iwarp_preview_get_point (xn, yn, color);

	    if (!preserve_trans && (image_bpp == 4 || image_bpp == 2))
	      {
		alpha = (gdouble) color[image_bpp - 1] / 255;
	      }

	    if (preview_bpp == 3)
	      {
		if (image_bpp == 4)
		  {
		    dstimage[ptr*3] =
		      (guchar) (alpha * color[0] +
				(1.0 - alpha) *
				iwarp_transparent_color (x + xi, y + yi));
		    dstimage[ptr*3+1] =
		      (guchar) (alpha * color[1] +
				(1.0 - alpha) *
				iwarp_transparent_color (x + xi, y + yi));
		    dstimage[ptr*3+2] =
		      (guchar) (alpha * color[2] +
				(1.0 - alpha) *
				iwarp_transparent_color (x + xi, y + yi));
		  }
		else
		  {
		    dstimage[ptr*3] = color[0];
		    dstimage[ptr*3+1] = color[1];
		    dstimage[ptr*3+2] = color[2];
		  }
	      }
	    else
	      {
		if (image_bpp == 2)
		  {
		    dstimage[ptr] =
		      (guchar) (alpha * color[0] +
				(1.0 - alpha) *
				iwarp_transparent_color (x + xi, y + yi)) ;
		  }
		else
		  {
		    dstimage[ptr] = color[0];
		  }
	      }
	  }
      }

  for (yi = y0; yi <= y1; yi++)
    for (xi = x0; xi <= x1; xi++)
      {
	length2 = (xi*xi+yi*yi) * MAX_DEFORM_AREA_RADIUS / radius2;
	if (length2 <MAX_DEFORM_AREA_RADIUS)
	  {
	    ptr = (yi +y) * preview_width + xi +x;
	    fptr =
	      (yi+iwarp_vals.deform_area_radius) *
	      (iwarp_vals.deform_area_radius*2+1) +
	      xi +
	      iwarp_vals.deform_area_radius;
	    deform_vectors[ptr].x = deform_area_vectors[fptr].x;
	    deform_vectors[ptr].y = deform_area_vectors[fptr].y;
	  }
      }

  iwarp_update_preview (x + x0, y + y0, x + x1 + 1, y + y1 + 1);   
} 

static void
iwarp_move (gint x,
	    gint y,
	    gint xx,
	    gint yy)
{
  gdouble l, dx, dy, xf, yf;
  gint    num, i, x0, y0; 
 
  dx = x-xx;
  dy = y-yy;
  l= sqrt (dx * dx + dy * dy);
  num = (gint) (l * 2 / iwarp_vals.deform_area_radius) + 1;
  dx /= num;
  dy /= num;
  xf = xx + dx; yf = yy + dy; 
  for (i=0; i< num; i++)
    {
      x0 = (gint) xf;
      y0 = (gint) yf;
      iwarp_deform (x0, y0, -dx, -dy);
      xf += dx;
      yf += dy;
    }
} 

static void
iwarp_ok_callback (GtkWidget *widget,
		    gpointer   data)
{
  wint.run = TRUE;

  gtk_widget_destroy (GTK_WIDGET (data));
}

static gint
iwarp_motion_callback (GtkWidget *widget,
		       GdkEvent  *event)
{
  GdkEventButton *mb;
  gint x, y;
 
  mb = (GdkEventButton *) event; 
  switch (event->type)
    {
    case GDK_BUTTON_PRESS:
      lastx = mb->x;
      lasty = mb->y;
      break;

    case GDK_BUTTON_RELEASE:
      if (mb->state & GDK_BUTTON1_MASK)
	{
	  x = mb->x;
	  y = mb->y;
	  if (iwarp_vals.deform_mode == MOVE)
	    iwarp_move (x, y, lastx, lasty);
	  else
	    iwarp_deform (x, y, 0.0, 0.0);
	}
      break;
 
   case GDK_MOTION_NOTIFY:
     if (mb->state & GDK_BUTTON1_MASK)
       {
	 x = mb->x;
	 y = mb->y;
	 if (iwarp_vals.deform_mode == MOVE)
	   iwarp_move (x, y, lastx, lasty);
	 else
	   iwarp_deform (x, y, 0.0, 0.0);
	 lastx = x;
	 lasty = y;
	 gtk_widget_get_pointer (widget, NULL, NULL); 
       }
     break;

    default:
      break;
    }

  return FALSE;
}

static void
iwarp_reset_callback (GtkWidget *widget,
		      gpointer   data)
{
  gint i;

  iwarp_cpy_images ();
  for (i = 0; i < preview_width * preview_height; i++)
    deform_vectors[i].x = deform_vectors[i].y = 0.0;

  iwarp_update_preview (0, 0, preview_width, preview_height);   
}
