/****************************************************************************
 * This is a plugin for GIMP v 0.99.8 or later.  Documentation is
 * available at http://www.rru.com/~meo/gimp/ .
 *
 * Copyright (C) 1997-98 Miles O'Neal  <meo@rru.com>  http://www.rru.com/~meo/
 * Blur code Copyright (C) 1995 Spencer Kimball and Peter Mattis
 * GUI based on GTK code from:
 *    alienmap (Copyright (C) 1996, 1997 Daniel Cotting)
 *    plasma   (Copyright (C) 1996 Stephen Norris),
 *    oilify   (Copyright (C) 1996 Torsten Martinsen),
 *    ripple   (Copyright (C) 1997 Brian Degenhardt) and
 *    whirl    (Copyright (C) 1997 Federico Mena Quintero).
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 *
 ****************************************************************************/

/****************************************************************************
 * Blur:
 *
 * blur version 2.1 (10 June 2004 WES)
 * history
 *     2.1 - 10 June 2004 WES
 *         removed dialog along with randomization and repeat options
 *     2.0 -  1 May 1998 MEO
 *         based on randomize 1.7
 *
 * Please send any patches or suggestions to the author: meo@rru.com .
 *
 * Blur applies a 3x3 blurring convolution kernel to the specified drawable.
 *
 * For each pixel in the selection or image,
 * whether to change the pixel is decided by picking a
 * random number, weighted by the user's "randomization" percentage.
 * If the random number is in range, the pixel is modified.  For
 * blurring, an average is determined from the current and adjacent
 * pixels. *(Except for the random factor, the blur code came
 * straight from the original S&P blur plug-in.)*
 *
 * This works only with RGB and grayscale images.
 *
 ****************************************************************************/

#include "config.h"

#include <string.h>

#include <libgimp/gimp.h>

#include "libgimp/stdplugins-intl.h"


/*********************************
 *
 *  PLUGIN-SPECIFIC CONSTANTS
 *
 ********************************/

#define PLUG_IN_PROC "plug-in-blur"

/*********************************
 *
 *  LOCAL FUNCTIONS
 *
 ********************************/

static void query (void);
static void run   (const gchar      *name,
                   gint              nparams,
                   const GimpParam  *param,
                   gint             *nreturn_vals,
                   GimpParam       **return_vals);

const GimpPlugInInfo PLUG_IN_INFO =
{
  NULL,  /* init_proc  */
  NULL,  /* quit_proc  */
  query, /* query_proc */
  run,   /* run_proc   */
};

static void         blur             (GimpDrawable *drawable);

static inline void  blur_prepare_row (GimpPixelRgn *pixel_rgn,
                                      guchar       *data,
                                      gint          x,
                                      gint          y,
                                      gint          w);

/************************************ Guts ***********************************/

MAIN ()

/*********************************
 *
 *  query() - query_proc
 *
 *      called by GIMP to learn about this plug-in
 *
 ********************************/

static void
query (void)
{
  static const GimpParamDef args[] =
  {
    { GIMP_PDB_INT32,    "run-mode", "The run mode { RUN-INTERACTIVE (0), RUN-NONINTERACTIVE (1) }" },
    { GIMP_PDB_IMAGE,    "image",    "Input image (unused)"         },
    { GIMP_PDB_DRAWABLE, "drawable", "Input drawable"               },
  };

  gimp_install_procedure (PLUG_IN_PROC,
                          N_("Simple blur, fast but not very strong"),
                          "This plug-in blurs the specified drawable, using "
                          "a 3x3 blur. Indexed images are not supported.",
                          "Miles O'Neal  <meo@rru.com>",
                          "Miles O'Neal, Spencer Kimball, Peter Mattis, "
                          "Torsten Martinsen, Brian Degenhardt, "
                          "Federico Mena Quintero, Stephen Norris, "
                          "Daniel Cotting",
                          "1995-1998",
                          N_("_Blur"),
                          "RGB*, GRAY*",
                          GIMP_PLUGIN,
                          G_N_ELEMENTS (args), 0,
                          args, NULL);
}

static void
run (const gchar      *name,
     gint              nparams,
     const GimpParam  *param,
     gint             *nreturn_vals,
     GimpParam       **return_vals)
{
  GimpDrawable      *drawable;
  GimpRunMode        run_mode;
  GimpPDBStatusType  status = GIMP_PDB_SUCCESS;
  static GimpParam   values[1];

  INIT_I18N ();

  values[0].type          = GIMP_PDB_STATUS;
  values[0].data.d_status = status;

  *nreturn_vals = 1;
  *return_vals  = values;

  if (strcmp (name, PLUG_IN_PROC) != 0 || nparams < 3)
    {
      values[0].data.d_status = GIMP_PDB_CALLING_ERROR;
      return;
    }

  run_mode = param[0].data.d_int32;
  drawable = gimp_drawable_get (param[2].data.d_drawable);

  /*
   *  Make sure the drawable type is appropriate.
   */
  if (gimp_drawable_is_rgb (drawable->drawable_id) ||
      gimp_drawable_is_gray (drawable->drawable_id))
    {
      gimp_progress_init (_("Blurring"));
      gimp_tile_cache_ntiles (2 * (drawable->width / gimp_tile_width () + 1));

      blur (drawable);

      if (run_mode != GIMP_RUN_NONINTERACTIVE)
        {
          gimp_displays_flush ();
        }
    }
  else
    {
      status = GIMP_PDB_EXECUTION_ERROR;
    }

  values[0].data.d_status = status;
  gimp_drawable_detach (drawable);
}


/*********************************
 *
 *  blur_prepare_row()
 *
 *  Get a row of pixels.  If the requested row
 *  is off the edge, clone the edge row.
 *
 ********************************/

static inline void
blur_prepare_row (GimpPixelRgn *pixel_rgn,
                  guchar       *data,
                  gint          x,
                  gint          y,
                  gint          w)
{
  gint b;

  y = CLAMP (y, 0, pixel_rgn->h - 1);

  gimp_pixel_rgn_get_row (pixel_rgn, data, x, y, w);

  /*
   *  Fill in edge pixels
   */
  for (b = 0; b < pixel_rgn->bpp; b++)
    data[-(gint)pixel_rgn->bpp + b] = data[b];

  for (b = 0; b < pixel_rgn->bpp; b++)
    data[w * pixel_rgn->bpp + b] = data[(w - 1) * pixel_rgn->bpp + b];
}

/*********************************
 *
 *  blur()
 *
 *  Actually mess with the image.
 *
 ********************************/

static void
blur (GimpDrawable *drawable)
{
  GimpPixelRgn  srcPR, destPR;
  gint          width, height;
  gint          bytes;
  guchar       *dest, *d;
  guchar       *prev_row, *pr;
  guchar       *cur_row, *cr;
  guchar       *next_row, *nr;
  guchar       *tmp;
  gint          row, col;
  gint          x1, y1, x2, y2;
  gint          ind;
  gboolean      has_alpha;

  if (! gimp_drawable_mask_intersect (drawable->drawable_id,
                                      &x1, &y1, &width, &height))
    return;

  x2 = x1 + width;
  y2 = y1 + height;

  /*
   *  Get the size of the input image. (This will/must be the same
   *  as the size of the output image.  Also get alpha info.
   */
  width = drawable->width;
  height = drawable->height;
  bytes = drawable->bpp;
  has_alpha = gimp_drawable_has_alpha (drawable->drawable_id);
  /*
   *  allocate row buffers
   */
  prev_row = g_new (guchar, (x2 - x1 + 2) * bytes);
  cur_row  = g_new (guchar, (x2 - x1 + 2) * bytes);
  next_row = g_new (guchar, (x2 - x1 + 2) * bytes);
  dest     = g_new (guchar, (x2 - x1) * bytes);

  /*
   *  initialize the pixel regions
   */
  gimp_pixel_rgn_init (&srcPR, drawable, 0, 0, width, height, FALSE, FALSE);
  gimp_pixel_rgn_init (&destPR, drawable, 0, 0, width, height, TRUE, TRUE);

  pr = prev_row + bytes;
  cr = cur_row + bytes;
  nr = next_row + bytes;

  /*
   *  prepare the first row and previous row
   */
  blur_prepare_row (&srcPR, pr, x1, y1 - 1, (x2 - x1));
  blur_prepare_row (&srcPR, cr, x1, y1, (x2 - x1));

  /*
   *  loop through the rows, applying the selected convolution
   */
  for (row = y1; row < y2; row++)
    {
      /*  prepare the next row  */
      blur_prepare_row (&srcPR, nr, x1, row + 1, (x2 - x1));

      d = dest;
      ind = 0;
      for (col = 0; col < (x2 - x1) * bytes; col++)
        {
          ind++;
          if (ind == bytes || !has_alpha)
            {
              /*
               *  If no alpha channel,
               *   or if there is one and this is it...
               */
              *d++ = ((gint) pr[col - bytes] + (gint) pr[col] +
                      (gint) pr[col + bytes] +
                      (gint) cr[col - bytes] + (gint) cr[col] +
                      (gint) cr[col + bytes] +
                      (gint) nr[col - bytes] + (gint) nr[col] +
                      (gint) nr[col + bytes] + 4) / 9;
              ind = 0;
            }
          else
            {
              /*
               *  otherwise we have an alpha channel,
               *   but this is a color channel
               */
              *d++ = ROUND(
                           ((gdouble) (pr[col - bytes] * pr[col - ind])
                            + (gdouble) (pr[col] * pr[col + bytes - ind])
                            + (gdouble) (pr[col + bytes] * pr[col + 2*bytes - ind])
                            + (gdouble) (cr[col - bytes] * cr[col - ind])
                            + (gdouble) (cr[col] * cr[col + bytes - ind])
                            + (gdouble) (cr[col + bytes] * cr[col + 2*bytes - ind])
                            + (gdouble) (nr[col - bytes] * nr[col - ind])
                            + (gdouble) (nr[col] * nr[col + bytes - ind])
                            + (gdouble) (nr[col + bytes] * nr[col + 2*bytes - ind]))
                           / ((gdouble) pr[col - ind]
                              + (gdouble) pr[col + bytes - ind]
                              + (gdouble) pr[col + 2*bytes - ind]
                              + (gdouble) cr[col - ind]
                              + (gdouble) cr[col + bytes - ind]
                              + (gdouble) cr[col + 2*bytes - ind]
                              + (gdouble) nr[col - ind]
                              + (gdouble) nr[col + bytes - ind]
                              + (gdouble) nr[col + 2*bytes - ind]));
            }
        }

      /*
       *  Save the modified row, shuffle the row pointers, and every
       *  so often, update the progress meter.
       */
      gimp_pixel_rgn_set_row (&destPR, dest, x1, row, (x2 - x1));

      tmp = pr;
      pr = cr;
      cr = nr;
      nr = tmp;

      if ((row % 32) == 0)
        gimp_progress_update ((gdouble) row / (gdouble) (y2 - y1));
    }

  gimp_progress_update (1.0);

  /*
   *  update the blurred region
   */
  gimp_drawable_flush (drawable);
  gimp_drawable_merge_shadow (drawable->drawable_id, TRUE);
  gimp_drawable_update (drawable->drawable_id, x1, y1, (x2 - x1), (y2 - y1));
  /*
   *  clean up after ourselves.
   */
  g_free (prev_row);
  g_free (cur_row);
  g_free (next_row);
  g_free (dest);
}
