/* Color Enhance 0.10 --- image filter plug-in for The Gimp image
 * manipulation program
 *
 * Copyright (C) 1999 Martin Weber
 * Copyright (C) 1996 Federico Mena Quintero
 *
 * You can contact me at martweb@gmx.net
 * You can contact the original The Gimp authors at gimp@xcf.berkeley.edu
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

/* This simple plug-in does an automatic Saturation stretch.  For each
   channel in the image, it finds the minimum and maximum values... it
   uses those values to stretch the individual histograms to the full
   range.  For some images it may do just what you want; for others
   it may be total crap :) This version operates in HSV space
   and preserves hue. Most code is taken from autostretch_hsv */


#include "config.h"

#include <stdlib.h>
#include <stdio.h>

#include <libgimp/gimp.h>

#include "libgimp/stdplugins-intl.h"

/* Declare local functions.
 */
static void      query  (void);
static void      run    (char      *name,
			 gint        nparams,
			 GParam    *param,
			 gint       *nreturn_vals,
			 GParam   **return_vals);

static void      Color_Enhance (GDrawable * drawable);
static void      indexed_Color_Enhance (gint32 image_ID);

GPlugInInfo PLUG_IN_INFO =
{
  NULL,    /* init_proc */
  NULL,    /* quit_proc */
  query,   /* query_proc */
  run,     /* run_proc */
};


MAIN ()

static void
query (void)
{
  static GParamDef args[] =
  {
    { PARAM_INT32, "run_mode", "Interactive, non-interactive" },
    { PARAM_IMAGE, "image", "Input image" },
    { PARAM_DRAWABLE, "drawable", "Input drawable" },
  };
  static GParamDef *return_vals = NULL;
  static gint nargs = sizeof (args) / sizeof (args[0]);
  static gint nreturn_vals = 0;

  INIT_I18N();

  gimp_install_procedure ("plug_in_color_enhance",
			  "Automatically stretch the saturation of the specified drawable to cover all possible ranges.",
			  "This simple plug-in does an automatic saturation stretch.  For each channel in the image, it finds the minimum and maximum values... it uses those values to stretch the individual histograms to the full range.  For some images it may do just what you want; for others it may be total crap :).  This version differs from Contrast Autostretch in that it works in HSV space, and preserves hue.",
			  "Martin Weber",
		 	  "Martin Weber", 
		  	  "1997", 	 	 
			  N_("<Image>/Image/Colors/Auto/Color Enhance"),
	   	  	  "RGB*, INDEXED*",
 			  PROC_PLUG_IN,
	 	  	  nargs, nreturn_vals,
 			  args,
			  return_vals);
}

static void
run (char    *name,
     gint      nparams,
     GParam  *param,
     gint     *nreturn_vals,
     GParam **return_vals)
{
  static GParam values[1];
  GDrawable *drawable;
  GRunModeType run_mode;
  GStatusType status = STATUS_SUCCESS;

  gint32 image_ID;

  INIT_I18N();

  run_mode = param[0].data.d_int32;

  /*  Get the specified drawable  */
  drawable = gimp_drawable_get (param[2].data.d_drawable);
  image_ID = param[1].data.d_image;

  /*  Make sure that the drawable is gray or RGB color  */
  if (gimp_drawable_is_rgb (drawable->id) || gimp_drawable_is_gray (drawable->id))
    {
      gimp_progress_init (_("Color Enhance..."));
      gimp_tile_cache_ntiles (2 * (drawable->width / gimp_tile_width () + 1));
      Color_Enhance (drawable);

      if (run_mode != RUN_NONINTERACTIVE)
	gimp_displays_flush ();
    }
  else if (gimp_drawable_is_indexed (drawable->id))
    {
      indexed_Color_Enhance (image_ID);

      if (run_mode != RUN_NONINTERACTIVE)
	gimp_displays_flush ();
    }
  else
    {
      /* gimp_message ("Color_Enhance: cannot operate on indexed color images"); */
      status = STATUS_EXECUTION_ERROR;
    }

  *nreturn_vals = 1;
  *return_vals = values;

  values[0].type = PARAM_STATUS;
  values[0].data.d_status = status;

  gimp_drawable_detach (drawable);
}


static void
indexed_Color_Enhance(gint32 image_ID)  /* a.d.m. */
{
  guchar *cmap;
  gint ncols,i;

  gdouble vhi = 0.0, vlo = 1.0;

  cmap = gimp_image_get_cmap (image_ID, &ncols);

  if (cmap==NULL)
    {
      printf("Color_Enhance: cmap was NULL!  Quitting...\n");
      gimp_quit();
    }

  for (i=0;i<ncols;i++)
    {
      gdouble h, s, v;
      gint c, m, y;
      gint k;
      guchar map[3];
      
      c = 255 - cmap[i*3+0];
      m = 255 - cmap[i*3+1];
      y = 255 - cmap[i*3+2];
      k = c;
      if (m < k) k = m;
      if (y < k) k = y;
      map[0] = c - k;
      map[1] = m - k;
      map[2] = y - k;
      gimp_rgb_to_hsv4(map, &h, &s, &v);
      if (v > vhi) vhi = v;
      if (v < vlo) vlo = v;
    }

  for (i=0;i<ncols;i++)
    {
      gdouble h, s, v;
      gint c, m, y;
      gint k;
      guchar map[3];
      
      c = 255 - cmap[i*3+0];
      m = 255 - cmap[i*3+1];
      y = 255 - cmap[i*3+2];
      k = c;
      if (m < k) k = m;
      if (y < k) k = y;
      map[0] = c - k;
      map[1] = m - k;
      map[2] = y - k;
      gimp_rgb_to_hsv4(map, &h, &s, &v);
      if (vhi!=vlo)
	v = (v-vlo) / (vhi-vlo);
      gimp_hsv_to_rgb4(map, h, s, v);
      c = map[0];
      m = map[1];
      y = map[2];
      c += k;
      if (c > 255) c = 255;
      m += k;
      if (m > 255) m = 255;
      y += k;
      if (y > 255) y = 255;
      cmap[i*3+0] = 255 - c;
      cmap[i*3+1] = 255 - m;
      cmap[i*3+2] = 255 - y;
    }

  gimp_image_set_cmap (image_ID, cmap, ncols);
}


static void
Color_Enhance (GDrawable *drawable)
{
  GPixelRgn src_rgn, dest_rgn;
  guchar *src, *s;
  guchar *dest, *d;
  gdouble  vhi = 0.0, vlo = 1.0;
  gint    progress, max_progress;
  gint    has_alpha, alpha;
  gint    x1, y1, x2, y2;
  gint    x, y;
  gpointer pr;

  /* Get selection area */
  gimp_drawable_mask_bounds (drawable->id, &x1, &y1, &x2, &y2);
  has_alpha = gimp_drawable_has_alpha (drawable->id);
  alpha = (has_alpha) ? drawable->bpp - 1 : drawable->bpp;

  /* Initialize progress */
  progress = 0;
  max_progress = (x2 - x1) * (y2 - y1) * 2;

  gimp_pixel_rgn_init (&src_rgn, drawable, x1, y1, (x2 - x1), (y2 - y1), FALSE, FALSE);

  for (pr = gimp_pixel_rgns_register (1, &src_rgn); pr != NULL; pr = gimp_pixel_rgns_process (pr))
    {
      src = src_rgn.data;

      for (y = 0; y < src_rgn.h; y++)
	{
	  s = src;
	  for (x = 0; x < src_rgn.w; x++)
	    {
	      if (!has_alpha || (has_alpha && s[alpha])) 
		{
		  gdouble h, z, v;
		  gint c, m, y;
		  gint k;
		  guchar map[3];
		  
		  c = 255 - s[0];
		  m = 255 - s[1];
		  y = 255 - s[2];
		  k = c;
		  if (m < k) k = m;
		  if (y < k) k = y;
		  map[0] = c - k;
		  map[1] = m - k;
		  map[2] = y - k;
		  gimp_rgb_to_hsv4(map, &h, &z, &v);
		  if (v > vhi) vhi = v;
		  if (v < vlo) vlo = v;
		}

	      s += src_rgn.bpp;
	    }

	  src += src_rgn.rowstride;
	}

      /* Update progress */
      progress += src_rgn.w * src_rgn.h;

      gimp_progress_update ((gdouble) progress / (gdouble) max_progress);
    }

  /* Now substitute pixel vales */
  gimp_pixel_rgn_init (&src_rgn, drawable, x1, y1, (x2 - x1), (y2 - y1), FALSE, FALSE);
  gimp_pixel_rgn_init (&dest_rgn, drawable, x1, y1, (x2 - x1), (y2 - y1), TRUE, TRUE);

  for (pr = gimp_pixel_rgns_register (2, &src_rgn, &dest_rgn); pr != NULL; pr = gimp_pixel_rgns_process (pr))
    {
      src = src_rgn.data;
      dest = dest_rgn.data;

      for (y = 0; y < src_rgn.h; y++)
	{
	  s = src;
	  d = dest;

	  for (x = 0; x < src_rgn.w; x++)
	    {
	      gdouble h, z, v;
	      gint c, m, y;
	      gint k;
	      guchar map[3];
	      
	      c = 255 - s[0];
	      m = 255 - s[1];
	      y = 255 - s[2];
	      k = c;
	      if (m < k) k = m;
	      if (y < k) k = y;
	      map[0] = c - k;
	      map[1] = m - k;
	      map[2] = y - k;
	      gimp_rgb_to_hsv4(map, &h, &z, &v);
	      if (vhi!=vlo)
		v = (v-vlo) / (vhi-vlo);
	      gimp_hsv_to_rgb4(map, h, z, v);
	      c = map[0];
	      m = map[1];
	      y = map[2];
	      c += k;
	      if (c > 255) c = 255;
	      m += k;
	      if (m > 255) m = 255;
	      y += k;
	      if (y > 255) y = 255;
	      d[0] = 255 - c;
	      d[1] = 255 - m;
	      d[2] = 255 - y;

	      if (has_alpha)
		d[alpha] = s[alpha];

	      s += src_rgn.bpp;
	      d += dest_rgn.bpp;
	    }

	  src += src_rgn.rowstride;
	  dest += dest_rgn.rowstride;

	}

      /* Update progress */
      progress += src_rgn.w * src_rgn.h;

      gimp_progress_update ((gdouble) progress / (gdouble) max_progress);
    }

  /*  update the region  */
  gimp_drawable_flush (drawable);
  gimp_drawable_merge_shadow (drawable->id, TRUE);
  gimp_drawable_update (drawable->id, x1, y1, (x2 - x1), (y2 - y1));
}
