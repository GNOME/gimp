/* Color Enhance 0.10 --- image filter plug-in for The Gimp image
 * manipulation program
 *
 * Copyright (C) 1999 Martin Weber
 * Copyright (C) 1996 Federico Mena Quintero
 *
 * You can contact me at martin.weber@usa.net
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


#include <stdlib.h>
#include <stdio.h>
#include "libgimp/gimp.h"

/* Declare local functions.
 */
static void      query  (void);
static void      run    (char      *name,
			 int        nparams,
			 GParam    *param,
			 int       *nreturn_vals,
			 GParam   **return_vals);

static void      Color_Enhance (GDrawable * drawable);
static void      indexed_Color_Enhance (gint32 image_ID);

static void calc_rgb_to_hsv(gint r, gint g, gint b, double *h, double
  *s, double *v);
static void calc_hsv_to_rgb(gint *r, gint *g, gint *b,
  double h, double s, double v);

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
  static int nargs = sizeof (args) / sizeof (args[0]);
  static int nreturn_vals = 0;

  gimp_install_procedure ("plug_in_Color_Enhance",
			  "Automatically stretch the saturation of the specified drawable to cover all possible ranges.", 	 	   	 
			  "This simple plug-in does an automatic saturation stretch.  For each channel in the image, it finds the minimum and maximum values... it uses those values to stretch the individual histograms to the full range.  For some images it may do just what you want; for others it may be total crap :).  This version differs from Contrast Autostretch in that it works in HSV space, and preserves hue.",
			  "Martin Weber",
		 	  "Martin Weber", 
		  	  "1997", 	 	 
			  "<Image>/Image/Colors/Color Enhance", 	
	   	  	  "RGB*, INDEXED*",
 			  PROC_PLUG_IN,
	 	  	  nargs, nreturn_vals,
 			  args,
			  return_vals); }

static void
run (char    *name,
     int      nparams,
     GParam  *param,
     int     *nreturn_vals,
     GParam **return_vals)
{
  static GParam values[1];
  GDrawable *drawable;
  GRunModeType run_mode;
  GStatusType status = STATUS_SUCCESS;

  gint32 image_ID;

  run_mode = param[0].data.d_int32;

  /*  Get the specified drawable  */
  drawable = gimp_drawable_get (param[2].data.d_drawable);
  image_ID = param[1].data.d_image;

  /*  Make sure that the drawable is gray or RGB color  */
  if (gimp_drawable_color (drawable->id) || gimp_drawable_gray (drawable->id))
    {
      gimp_progress_init ("Color Enhance...");
      gimp_tile_cache_ntiles (2 * (drawable->width / gimp_tile_width () + 1));
      Color_Enhance (drawable);

      if (run_mode != RUN_NONINTERACTIVE)
	gimp_displays_flush ();
    }
  else if (gimp_drawable_indexed (drawable->id))
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

  double vhi = 0.0, vlo = 1.0;

  cmap = gimp_image_get_cmap (image_ID, &ncols);

  if (cmap==NULL)
    {
      printf("Color_Enhance: cmap was NULL!  Quitting...\n");
      gimp_quit();
    }

  for (i=0;i<ncols;i++)
    {
      double h, s, v;
      double c, m, y;
      double k;
      c = 255 - cmap[i*3+0];
      m = 255 - cmap[i*3+1];
      y = 255 - cmap[i*3+2];
      k = c;
      if (m < k) k = m;
      if (y < k) k = y;
      calc_rgb_to_hsv(c - k, m - k, y - k, &h, &s, &v);
      if (v > vhi) vhi = v;
      if (v < vlo) vlo = v;
    }

  for (i=0;i<ncols;i++)
    {
      double h, s, v;
      gint c, m, y;
      double k;
      c = 255 - cmap[i*3+0];
      m = 255 - cmap[i*3+1];
      y = 255 - cmap[i*3+2];
      k = c;
      if (m < k) k = m;
      if (y < k) k = y;
      calc_rgb_to_hsv(c - k, m - k, y - k, &h, &s, &v);
      if (vhi!=vlo)
	v = (v-vlo) / (vhi-vlo);
      calc_hsv_to_rgb(&c, &m, &y, h, s, v);
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
  double  vhi = 0.0, vlo = 1.0;
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
		  double h, z, v;
		  double c, m, y;
		  double k;
		  c = 255 - s[0];
		  m = 255 - s[1];
		  y = 255 - s[2];
		  k = c;
		  if (m < k) k = m;
		  if (y < k) k = y;
		  calc_rgb_to_hsv(c - k, m - k, y - k, &h, &z, &v);
		  if (v > vhi) vhi = v;
		  if (v < vlo) vlo = v;
		}

	      s += src_rgn.bpp;
	    }

	  src += src_rgn.rowstride;
	}

      /* Update progress */
      progress += src_rgn.w * src_rgn.h;

      gimp_progress_update ((double) progress / (double) max_progress);
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
	      double h, z, v;
	      gint c, m, y;
	      double k;
	      c = 255 - s[0];
	      m = 255 - s[1];
	      y = 255 - s[2];
	      k = c;
	      if (m < k) k = m;
	      if (y < k) k = y;
	      calc_rgb_to_hsv(c - k, m - k, y - k, &h, &z, &v);
	      if (vhi!=vlo)
		v = (v-vlo) / (vhi-vlo);
	      calc_hsv_to_rgb(&c, &m , &y, h, z, v);
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

      gimp_progress_update ((double) progress / (double) max_progress);
    }

  /*  update the region  */
  gimp_drawable_flush (drawable);
  gimp_drawable_merge_shadow (drawable->id, TRUE);
  gimp_drawable_update (drawable->id, x1, y1, (x2 - x1), (y2 - y1));
}

static void
calc_rgb_to_hsv(gint r, gint g, gint b, double *hue, double *sat, double
*val) {
  double red, green, blue;
  double h, s, v;
  double min, max;
  double delta;

  red   = (double)r / 255.0;
  green = (double)g / 255.0;
  blue  = (double)b / 255.0;

  h = 0.0; /* Shut up -Wall */

  if (red > green)
    {
      if (red > blue)
	max = red;
      else
	max = blue;

      if (green < blue)
	min = green;
      else
	min = blue;
    }
  else
    {
      if (green > blue)
	max = green;
      else
	max = blue;

      if (red < blue)
	min = red;
      else
	min = blue;
    }

  v = max;

  if (max != 0.0)
    s = (max - min) / max;
  else
    s = 0.0;

  if (s == 0.0)
    h = 0.0;
  else
    {
      delta = max - min;

      if (red == max)
	h = (green - blue) / delta;
      else if (green == max)
	h = 2 + (blue - red) / delta;
      else if (blue == max)
	h = 4 + (red - green) / delta;

      h /= 6.0;

      if (h < 0.0)
	h += 1.0;
      else if (h > 1.0)
	h -= 1.0;
    }

  *hue = h;
  *sat = s;
  *val = v;
}

static void
calc_hsv_to_rgb(gint *r, gint *g, gint *b, double h, double s, double
v) {
  double hue, saturation, value;
  double f, p, q, t;

  if (s == 0.0)
    {
      h = v;
      s = v;
      v = v; /* heh */
    }
  else
    {
      hue        = h * 6.0;
      saturation = s;
      value      = v;

      if (hue == 6.0)
	hue = 0.0;

      f = hue - (int) hue;
      p = value * (1.0 - saturation);
      q = value * (1.0 - saturation * f);
      t = value * (1.0 - saturation * (1.0 - f));

      switch ((int) hue)
	{
	case 0:
	  h = value;
	  s = t;
	  v = p;
	  break;

	case 1:
	  h = q;
	  s = value;
	  v = p;
	  break;

	case 2:
	  h = p;
	  s = value;
	  v = t;
	  break;

	case 3:
	  h = p;
	  s = q;
	  v = value;
	  break;

	case 4:
	  h = t;
	  s = p;
	  v = value;
	  break;

	case 5:
	  h = value;
	  s = p;
	  v = q;
	  break;
	}
    }

  *r = h*255;
  *g = s*255;
  *b = v*255;
  
}

