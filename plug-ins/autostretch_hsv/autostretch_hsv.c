/* Autostretch HSV 0.10 --- image filter plug-in for The Gimp image
 * manipulation program
 *
 * Copyright (C) 1997 Scott Goehring
 * Copyright (C) 1996 Federico Mena Quintero
 *
 * You can contact me at scott@poverty.bloomington.in.us
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
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

/* This simple plug-in does an automatic contrast stretch.  For each
   channel in the image, it finds the minimum and maximum values... it
   uses those values to stretch the individual histograms to the full
   contrast range.  For some images it may do just what you want; for
   others it may be total crap :) This version operates in HSV space
   and preserves hue, unlike the original Contrast Autostretch. */


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

static void      autostretch_hsv (GDrawable * drawable);
static void      indexed_autostretch_hsv (gint32 image_ID);

static void calc_rgb_to_hsv(guchar *rgb, double *h, double *s, double *v);
static void calc_hsv_to_rgb(guchar *rgb, double h, double s, double v);

GPlugInInfo PLUG_IN_INFO =
{
  NULL,    /* init_proc */
  NULL,    /* quit_proc */
  query,   /* query_proc */
  run,     /* run_proc */
};


MAIN ()

static void
query ()
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

  gimp_install_procedure ("plug_in_autostretch_hsv",
			  "Automatically stretch the contrast of the specified drawable to cover all possible ranges.",
			  "This simple plug-in does an automatic contrast stretch.  For each channel in the image, it finds the minimum and maximum values... it uses those values to stretch the individual histograms to the full contrast range.  For some images it may do just what you want; for others it may be total crap :).  This version differs from Contrast Autostretch in that it works in HSV space, and preserves hue.",
			  "Scott Goehring and Federico Mena Quintero",
			  "Scott Goehring and Federico Mena Quintero",
			  "1997",
			  "<Image>/Image/Colors/Auto-Stretch HSV",
			  "RGB*, INDEXED*",
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
      gimp_progress_init ("Auto-Stretching Contrast...");
      gimp_tile_cache_ntiles (2 * (drawable->width / gimp_tile_width () + 1));
      autostretch_hsv (drawable);

      if (run_mode != RUN_NONINTERACTIVE)
	gimp_displays_flush ();
    }
  else if (gimp_drawable_indexed (drawable->id))
    {
      indexed_autostretch_hsv (image_ID);

      /* GIMP doesn't implicitly update an image whose cmap has
	 changed - it probably should. */

      gimp_drawable_update (drawable->id, 0, 0,
			    gimp_drawable_width(drawable->id),
			    gimp_drawable_height(drawable->id));
      gimp_displays_flush ();
    }
  else
    {
      /* gimp_message ("autostretch_hsv: cannot operate on indexed color images"); */
      status = STATUS_EXECUTION_ERROR;
    }

  *nreturn_vals = 1;
  *return_vals = values;

  values[0].type = PARAM_STATUS;
  values[0].data.d_status = status;

  gimp_drawable_detach (drawable);
}


static void
indexed_autostretch_hsv(gint32 image_ID)  /* a.d.m. */
{
  guchar *cmap;
  gint ncols,i;

  double shi = 0.0, vhi = 0.0, slo = 1.0, vlo = 1.0;

  cmap = gimp_image_get_cmap (image_ID, &ncols);

  if (cmap==NULL)
    {
      printf("autostretch_hsv: cmap was NULL!  Quitting...\n");
      gimp_quit();
    }

  for (i=0;i<ncols;i++)
    {
      double h, s, v;
      calc_rgb_to_hsv(&cmap[i*3], &h, &s, &v);
      if (s > shi) shi = s;
      if (s < slo) slo = s;
      if (v > vhi) vhi = v;
      if (v < vlo) vlo = v;
    }

  for (i=0;i<ncols;i++)
    {
      double h, s, v;
      calc_rgb_to_hsv(&cmap[i*3], &h, &s, &v);
      if (shi!=slo)
	s = (s-slo) / (shi-slo);
      if (vhi!=vlo)
	v = (v-vlo) / (vhi-vlo);
      calc_hsv_to_rgb(&cmap[i*3], h, s, v);
    }

  gimp_image_set_cmap (image_ID, cmap, ncols);
}


static void
autostretch_hsv (GDrawable *drawable)
{
  GPixelRgn src_rgn, dest_rgn;
  guchar *src, *s;
  guchar *dest, *d;
  double  shi = 0.0, slo = 1.0, vhi = 0.0, vlo = 1.0;
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
		  calc_rgb_to_hsv(s, &h, &z, &v);
		  if (z > shi) shi = z;
		  if (z < slo) slo = z;
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
	      calc_rgb_to_hsv(s, &h, &z, &v);
	      if (shi!=slo)
		z = (z-slo) / (shi-slo);
	      if (vhi!=vlo)
		v = (v-vlo) / (vhi-vlo);
	      calc_hsv_to_rgb(d, h, z, v);

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
calc_rgb_to_hsv(guchar *rgb, double *hue, double *sat, double *val)
{
  double red, green, blue;
  double h, s, v;
  double min, max;
  double delta;

  red   = rgb[0] / 255.0;
  green = rgb[1] / 255.0;
  blue  = rgb[2] / 255.0;

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
calc_hsv_to_rgb(guchar *rgb, double h, double s, double v)
{
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

  rgb[0] = h*255;
  rgb[1] = s*255;
  rgb[2] = v*255;
  
}

