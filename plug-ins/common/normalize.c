/* Normalize 1.00 --- image filter plug-in for The GIMP
 *
 * Copyright (C) 1997 Adam D. Moss (adam@foxbox.org)
 * Very largely based on Quartic's "Contrast Autostretch"
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

/* This plugin performs almost the same operation as the 'contrast
 * autostretch' plugin, except that it won't allow the colour channels
 * to normalize independently.  This is actually what most people probably
 * want instead of contrast-autostretch; use c-a only if you wish to remove
 * an undesirable colour-tint from a source image which is supposed to
 * contain pure-white and pure-black.
 */


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

static void      norma (GDrawable * drawable);
static void      indexed_norma (gint32 image_ID);


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

  gimp_install_procedure ("plug_in_normalize",
			  "Normalize the contrast of the specified drawable to cover all possible ranges.",
			  "This plugin performs almost the same operation as the 'contrast autostretch' plugin, except that it won't allow the colour channels to normalize independently.  This is actually what most people probably want instead of contrast-autostretch; use c-a only if you wish to remove an undesirable colour-tint from a source image which is supposed to contain pure-white and pure-black.",
			  "Adam D. Moss, Federico Mena Quintero",
			  "Adam D. Moss, Federico Mena Quintero",
			  "1997",
			  "<Image>/Image/Colors/Normalize",
			  "RGB*, GRAY*, INDEXED*",
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
      gimp_progress_init ("Normalizing...");
      gimp_tile_cache_ntiles (2 * (drawable->width / gimp_tile_width () + 1));
      norma (drawable);

      if (run_mode != RUN_NONINTERACTIVE)
	gimp_displays_flush ();
    }
  else if (gimp_drawable_indexed (drawable->id))
    {
      indexed_norma (image_ID);

      /* GIMP doesn't implicitly update an image whose cmap has
	 changed - it probably should. */

      gimp_drawable_update (drawable->id, 0, 0,
			    gimp_drawable_width(drawable->id),
			    gimp_drawable_height(drawable->id));
      gimp_displays_flush ();
    }
  else
    {
      /* gimp_message ("normalize: cannot operate on indexed color images"); */
      status = STATUS_EXECUTION_ERROR;
    }

  *nreturn_vals = 1;
  *return_vals = values;

  values[0].type = PARAM_STATUS;
  values[0].data.d_status = status;

  gimp_drawable_detach (drawable);
}


static void
indexed_norma(gint32 image_ID)  /* a.d.m. */
{
  guchar *cmap;
  gint ncols,i;
  gint hi=0,lo=255;

  cmap = gimp_image_get_cmap (image_ID, &ncols);

  if (cmap==NULL)
    {
      printf("normalize: cmap was NULL!  Quitting...\n");
      gimp_quit();
    }

  for (i=0;i<ncols;i++)
    {
      if (cmap[i*3 +0] > hi) hi=cmap[i*3 +0];
      if (cmap[i*3 +1] > hi) hi=cmap[i*3 +1];
      if (cmap[i*3 +2] > hi) hi=cmap[i*3 +2];
      if (cmap[i*3 +0] < lo) lo=cmap[i*3 +0];
      if (cmap[i*3 +1] < lo) lo=cmap[i*3 +1];
      if (cmap[i*3 +2] < lo) lo=cmap[i*3 +2];
    }

  if (hi!=lo)
    for (i=0;i<ncols;i++)
      {
	cmap[i*3 +0] = (255 * (cmap[i*3 +0] - lo)) / (hi-lo);
	cmap[i*3 +1] = (255 * (cmap[i*3 +1] - lo)) / (hi-lo);
	cmap[i*3 +2] = (255 * (cmap[i*3 +2] - lo)) / (hi-lo);
      }

  gimp_image_set_cmap (image_ID, cmap, ncols);
}


static void
norma (GDrawable *drawable)
{
  GPixelRgn src_rgn, dest_rgn;
  guchar *src, *s;
  guchar *dest, *d;
  guchar  min, max;
  guchar  range;
  guchar  lut[256];
  gint    progress, max_progress;
  gint    has_alpha, alpha;
  gint    x1, y1, x2, y2;
  gint    x, y, b;
  gpointer pr;

  /* Get selection area */
  gimp_drawable_mask_bounds (drawable->id, &x1, &y1, &x2, &y2);
  has_alpha = gimp_drawable_has_alpha (drawable->id);
  alpha = (has_alpha) ? drawable->bpp - 1 : drawable->bpp;

  /* Initialize progress */
  progress = 0;
  max_progress = (x2 - x1) * (y2 - y1) * 2;

  /* Get minimum and maximum values for each channel */
  min = 255;
  max = 0;

  gimp_pixel_rgn_init (&src_rgn, drawable, x1, y1, (x2 - x1), (y2 - y1), FALSE, FALSE);

  for (pr = gimp_pixel_rgns_register (1, &src_rgn); pr != NULL; pr = gimp_pixel_rgns_process (pr))
    {
      src = src_rgn.data;

      for (y = 0; y < src_rgn.h; y++)
	{
	  s = src;
	  for (x = 0; x < src_rgn.w; x++)
	    {
	      for (b = 0; b < alpha; b++)
		{
		  if (!has_alpha || (has_alpha && s[alpha]))
		    {
		      if (s[b] < min)
			min = s[b];
		      if (s[b] > max)
			max = s[b];
		    }
		}

	      s += src_rgn.bpp;
	    }

	  src += src_rgn.rowstride;
	}

      /* Update progress */
      progress += src_rgn.w * src_rgn.h;

      gimp_progress_update ((double) progress / (double) max_progress);
    }

  /* Calculate LUT */

  range = max - min;

  if (range != 0)
    for (x = min; x <= max; x++)
      lut[x] = 255 * (x - min) / range;
  else
    lut[min] = min;


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
	      for (b = 0; b < alpha; b++)
		d[b] = lut[s[b]];

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
