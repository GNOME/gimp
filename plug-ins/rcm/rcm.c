/*
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * This is a plug-in for the GIMP.
 *
 * Colormap-Rotation plug-in. Exchanges two color ranges.
 *
 * Copyright (C) 1999 Sven Anders (anderss@fmi.uni-passau.de)
 *                    Based on code from Pavel Grinfeld (pavel@ml.com)
 *
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

/*-----------------------------------------------------------------------------------
 * Change log:
 * 
 * Version 2.0, 04 April 1999.
 *  Nearly complete rewrite, made plug-in stable.
 *  (Works with GIMP 1.1 and GTK+ 1.2)
 *
 * Version 1.0, 27 March 1997.
 *  Initial (unstable) release by Pavel Grinfeld
 *
 *-----------------------------------------------------------------------------------*/

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include <gtk/gtk.h>
#include <libgimp/gimp.h>
#include <libgimp/gimpintl.h>

#include "rcm.h"
#include "rcm_misc.h"
#include "rcm_dialog.h"
#include "rcm_callback.h"

/*-----------------------------------------------------------------------------------*/
/* Forward declarations */
/*-----------------------------------------------------------------------------------*/

void query(void);
void run(char *name, int nparams, GParam *param, int *nreturn_vals, GParam **return_vals);

/*-----------------------------------------------------------------------------------*/
/* Global variables */
/*-----------------------------------------------------------------------------------*/

RcmParams Current =
{
  SELECTION,           /* SELECTION ONLY */
  TRUE,                /* REAL TIME */
  FALSE,               /* SUCCESS */
  RADIANS_OVER_PI,     /* START IN RADIANS OVER PI */
  GRAY_TO
};

/*-----------------------------------------------------------------------------------*/
/* Local variables */
/*-----------------------------------------------------------------------------------*/

GPlugInInfo PLUG_IN_INFO =
{
  NULL,    /* init_proc */
  NULL,    /* quit_proc */
  query,   /* query_proc */
  run,     /* run_proc */
};

/*-----------------------------------------------------------------------------------*/
/* Dummy function */
/*-----------------------------------------------------------------------------------*/

MAIN()

/*-----------------------------------------------------------------------------------*/
/* Query plug-in */
/*-----------------------------------------------------------------------------------*/

void query(void)
{
  GParamDef args[] =
  {
    { PARAM_INT32, "run_mode", "Interactive, non-interactive" },
    { PARAM_IMAGE, "image", "Input image (used for indexed images)" },
    { PARAM_DRAWABLE, "drawable", "Input drawable" },
  };

  GParamDef *return_vals = NULL;
  int nargs = sizeof (args) / sizeof (args[0]);
  int nreturn_vals = 0;

  gimp_install_procedure ("plug-in-rotate-colormap",
			  "Colormap rotation as in xv",
			  "Exchanges two color ranges. "\
			  "Based on code from Pavel Grinfeld (pavel@ml.com). "\
			  "This version written by Sven Anders (anderss@fmi.uni-passau.de).",
			  "Sven Anders (anderss@fmi.uni-passau.de) and Pavel Grinfeld (pavel@ml.com)",
			  "Sven Anders (anderss@fmi.uni-passau.de)",
			  "04th April 1999",
			  "<Image>/Image/Colors/Colormap Rotation...",
			  "RGB*",
			  PROC_PLUG_IN,
			  nargs, nreturn_vals,
			  args, return_vals);
}

/*-----------------------------------------------------------------------------------*/
/* Rotate colormap of a single row */
/*-----------------------------------------------------------------------------------*/

void rcm_row(const guchar *src_row, guchar *dest_row,
	     gint row, gint row_width, gint bytes)
{
  gint col, bytenum, skip;
  hsv H,S,V,R,G,B;
  
  for (col=0; col < row_width; col++)
  {
    skip = 0;
      
    R = (float)src_row[col*bytes + 0]/255.0;
    G = (float)src_row[col*bytes + 1]/255.0;
    B = (float)src_row[col*bytes + 2]/255.0;
      
    rgb_to_hsv(R,G,B, &H,&S,&V);
      
    if (rcm_is_gray(S))
    {
      if (Current.Gray_to_from == GRAY_FROM)
      {
	if (rcm_angle_inside_slice(Current.Gray->hue,Current.From->angle) <= 1)
	{
	  H = Current.Gray->hue / TP;
	  S = Current.Gray->satur;
	}
	else 
	{
	  skip = 1;
	}
      }
      else
      {
	skip = 1;
	hsv_to_rgb(Current.Gray->hue/TP, Current.Gray->satur, V, &R, &G, &B);
      }
    }
      
    if (!skip)
    {
      H = rcm_linear(rcm_left_end(Current.From->angle),
		     rcm_right_end(Current.From->angle),
		     rcm_left_end(Current.To->angle),
		     rcm_right_end(Current.To->angle),
		     H*TP);    

      H = angle_mod_2PI(H) / TP;
      hsv_to_rgb(H,S,V, &R,&G,&B);
    }
      
    dest_row[col*bytes +0] = R * 255;
    dest_row[col*bytes +1] = G * 255;
    dest_row[col*bytes +2] = B * 255;
      
    if (bytes > 3)
    {
      for (bytenum=3; bytenum<bytes; bytenum++)
	dest_row[col*bytes+bytenum] = src_row[col*bytes+bytenum];
    }
  }
}

/*-----------------------------------------------------------------------------------*/
/* Rotate colormap row by row ... */
/*-----------------------------------------------------------------------------------*/

void rcm(GDrawable *drawable)
{
  GPixelRgn srcPR, destPR;
  gint width, height;
  gint bytes;
  guchar *src_row, *dest_row;
  gint row;
  gint x1, y1, x2, y2;
  
  gimp_drawable_mask_bounds (drawable->id, &x1, &y1, &x2, &y2);

  width = drawable->width;
  height = drawable->height;
  bytes = drawable->bpp;

  src_row = (guchar *) malloc ((x2 - x1) * bytes);
  dest_row = (guchar *) malloc ((x2 - x1) * bytes);

  gimp_pixel_rgn_init(&srcPR, drawable, 0, 0, width, height, FALSE, FALSE);
  gimp_pixel_rgn_init(&destPR, drawable, 0, 0, width, height, TRUE, TRUE);

  for (row=y1; row < y2; row++)
  {
    gimp_pixel_rgn_get_row(&srcPR, src_row, x1, row, (x2 - x1));

    rcm_row(src_row, dest_row, row, (x2 - x1), bytes);
      
    gimp_pixel_rgn_set_row(&destPR, dest_row, x1, row, (x2 - x1));
      
    if ((row % 10) == 0)
      gimp_progress_update((double) row / (double) (y2 - y1));
  }

  /*  update the processed region  */

  gimp_drawable_flush(drawable);
  gimp_drawable_merge_shadow(drawable->id, TRUE);
  gimp_drawable_update(drawable->id, x1, y1, (x2 - x1), (y2 - y1));
  
  free (src_row);
  free (dest_row);
}

/*-----------------------------------------------------------------------------------*/
/* STANDARD RUN */
/*-----------------------------------------------------------------------------------*/

void run(char *name, int nparams, GParam *param, int *nreturn_vals, GParam **return_vals)
{
  GParam values[1];
  GStatusType status = STATUS_SUCCESS;
  
  *nreturn_vals = 1;
  *return_vals = values;

  values[0].type = PARAM_STATUS;
  values[0].data.d_status = status;

  Current.drawable = gimp_drawable_get(param[2].data.d_drawable);
  Current.mask = gimp_drawable_get(gimp_image_get_selection(param[1].data.d_image));

  /* works not on INDEXED images */     

  if (gimp_drawable_is_indexed (Current.drawable->id) ||
      gimp_drawable_is_gray (Current.drawable->id) )
  {
    status = STATUS_EXECUTION_ERROR;
  }
  else
  {
    /* call dialog and rotate the colormap */

    if (gimp_drawable_is_rgb(Current.drawable->id) && rcm_dialog())
    {
      gimp_progress_init(_("Rotating the colormap..."));

      gimp_tile_cache_ntiles(2 * (Current.drawable->width / gimp_tile_width() + 1));
      rcm(Current.drawable);
      gimp_displays_flush();
    }
    else
      status = STATUS_EXECUTION_ERROR;
  }
  
  values[0].data.d_status = status;
  if (status == STATUS_SUCCESS)
    gimp_drawable_detach(Current.drawable);
}
