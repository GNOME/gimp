/*
 * Copyright (C) 1999 Winston Chang
 *                    <winstonc@cs.wisc.edu>
 *                    <winston@stdout.org>
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
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <stdlib.h>

#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

#include "libgimp/stdplugins-intl.h"


#define PLUG_IN_PROC    "plug-in-unsharp-mask"
#define PLUG_IN_BINARY  "unsharp-mask"
#define PLUG_IN_ROLE    "gimp-unsharp-mask"

#define SCALE_WIDTH   120
#define ENTRY_WIDTH     5

/* Uncomment this line to get a rough estimate of how long the plug-in
 * takes to run.
 */

/*  #define TIMER  */


typedef struct
{
  gdouble  radius;
  gdouble  amount;
  gint     threshold;
} UnsharpMaskParams;

typedef struct
{
  gboolean  run;
} UnsharpMaskInterface;

/* local function prototypes */
static void      query (void);
static void      run   (const gchar      *name,
                        gint              nparams,
                        const GimpParam  *param,
                        gint             *nreturn_vals,
                        GimpParam       **return_vals);

static void      gaussian_blur_line  (const gdouble  *cmatrix,
                                      const gint      cmatrix_length,
                                      const guchar   *src,
                                      guchar         *dest,
                                      const gint      len,
                                      const gint      bpp);
static void      box_blur_line       (const gint      box_width,
                                      const gint      even_offset,
                                      const guchar   *src,
                                      guchar         *dest,
                                      const gint      len,
                                      const gint      bpp);
static gint      gen_convolve_matrix (gdouble         std_dev,
                                      gdouble       **cmatrix);
static void      unsharp_region      (GimpPixelRgn   *srcPTR,
                                      GimpPixelRgn   *dstPTR,
                                      gint            bpp,
                                      gdouble         radius,
                                      gdouble         amount,
                                      gint            x1,
                                      gint            x2,
                                      gint            y1,
                                      gint            y2,
                                      gboolean        show_progress);

static void      unsharp_mask        (GimpDrawable   *drawable,
                                      gdouble         radius,
                                      gdouble         amount);

static gboolean  unsharp_mask_dialog (GimpDrawable   *drawable);
static void      preview_update      (GimpPreview    *preview,
                                      GimpDrawable   *drawable);


/* create a few globals, set default values */
static UnsharpMaskParams unsharp_params =
{
  5.0, /* default radius    */
  0.5, /* default amount    */
  0    /* default threshold */
};

/* Setting PLUG_IN_INFO */
const GimpPlugInInfo PLUG_IN_INFO =
  {
    NULL,  /* init_proc  */
    NULL,  /* quit_proc  */
    query, /* query_proc */
    run,   /* run_proc   */
  };


MAIN ()

static void
query (void)
{
  static const GimpParamDef args[] =
    {
      { GIMP_PDB_INT32,    "run-mode",  "The run mode { RUN-INTERACTIVE (0), RUN-NONINTERACTIVE (1) }" },
      { GIMP_PDB_IMAGE,    "image",     "(unused)" },
      { GIMP_PDB_DRAWABLE, "drawable",  "Drawable to draw on" },
      { GIMP_PDB_FLOAT,    "radius",    "Radius of gaussian blur (in pixels > 1.0)" },
      { GIMP_PDB_FLOAT,    "amount",    "Strength of effect" },
      { GIMP_PDB_INT32,    "threshold", "Threshold (0-255)" }
    };

  gimp_install_procedure (PLUG_IN_PROC,
                          N_("The most widely useful method for sharpening an image"),
                          "The unsharp mask is a sharpening filter that works "
                          "by comparing using the difference of the image and "
                          "a blurred version of the image.  It is commonly "
                          "used on photographic images, and is provides a much "
                          "more pleasing result than the standard sharpen "
                          "filter.",
                          "Winston Chang <winstonc@cs.wisc.edu>",
                          "Winston Chang",
                          "1999-2009",
                          N_("_Unsharp Mask..."),
                          "GRAY*, RGB*",
                          GIMP_PLUGIN,
                          G_N_ELEMENTS (args), 0,
                          args, NULL);

  gimp_plugin_menu_register (PLUG_IN_PROC, "<Image>/Filters/Enhance");
}

static void
run (const gchar      *name,
     gint              nparams,
     const GimpParam  *param,
     gint             *nreturn_vals,
     GimpParam       **return_vals)
{
  static GimpParam   values[1];
  GimpPDBStatusType  status = GIMP_PDB_SUCCESS;
  GimpDrawable      *drawable;
  GimpRunMode        run_mode;
#ifdef TIMER
  GTimer            *timer = g_timer_new ();
#endif

  run_mode = param[0].data.d_int32;

  *return_vals  = values;
  *nreturn_vals = 1;

  values[0].type          = GIMP_PDB_STATUS;
  values[0].data.d_status = status;

  INIT_I18N ();

  /*
   * Get drawable information...
   */
  drawable = gimp_drawable_get (param[2].data.d_drawable);
  gimp_tile_cache_ntiles (2 * MAX (drawable->width  / gimp_tile_width () + 1 ,
                                   drawable->height / gimp_tile_height () + 1));

  switch (run_mode)
    {
    case GIMP_RUN_INTERACTIVE:
      gimp_get_data (PLUG_IN_PROC, &unsharp_params);
      /* Reset default values show preview unmodified */

      /* initialize pixel regions and buffer */
      if (! unsharp_mask_dialog (drawable))
        return;

      break;

    case GIMP_RUN_NONINTERACTIVE:
      if (nparams != 6)
        {
          status = GIMP_PDB_CALLING_ERROR;
        }
      else
        {
          unsharp_params.radius = param[3].data.d_float;
          unsharp_params.amount = param[4].data.d_float;
          unsharp_params.threshold = param[5].data.d_int32;

          /* make sure there are legal values */
          if ((unsharp_params.radius < 0.0) ||
              (unsharp_params.amount < 0.0))
            status = GIMP_PDB_CALLING_ERROR;
        }
      break;

    case GIMP_RUN_WITH_LAST_VALS:
      gimp_get_data (PLUG_IN_PROC, &unsharp_params);
      break;

    default:
      break;
    }

  if (status == GIMP_PDB_SUCCESS)
    {
      drawable = gimp_drawable_get (param[2].data.d_drawable);

      /* here we go */
      unsharp_mask (drawable, unsharp_params.radius, unsharp_params.amount);

      gimp_displays_flush ();

      /* set data for next use of filter */
      if (run_mode == GIMP_RUN_INTERACTIVE)
        gimp_set_data (PLUG_IN_PROC,
                       &unsharp_params, sizeof (UnsharpMaskParams));

      gimp_drawable_detach(drawable);
      values[0].data.d_status = status;
    }

#ifdef TIMER
  g_printerr ("%f seconds\n", g_timer_elapsed (timer, NULL));
  g_timer_destroy (timer);
#endif
}

/* This function is written as if it is blurring a row of pixels,
 * even though it can operate on colums, too.  There is no difference
 * in the processing of the lines, at least to the blur_line function.
 */
static void
box_blur_line (const gint    box_width,   /* Width of the kernel           */
               const gint    even_offset, /* If even width,
                                             offset to left or right       */
               const guchar *src,         /* Pointer to source buffer      */
               guchar       *dest,        /* Pointer to destination buffer */
               const gint    len,         /* Length of buffer, in pixels   */
               const gint    bpp)         /* Bytes per pixel               */
{
  gint  i;
  gint  lead;    /* This marks the leading edge of the kernel              */
  gint  output;  /* This marks the center of the kernel                    */
  gint  trail;   /* This marks the pixel BEHIND the last 1 in the
                    kernel; it's the pixel to remove from the accumulator. */
  gint  ac[bpp]; /* Accumulator for each channel                           */


  /* The algorithm differs for even and odd-sized kernels.
   * With the output at the center,
   * If odd, the kernel might look like this: 0011100
   * If even, the kernel will either be centered on the boundary between
   * the output and its left neighbor, or on the boundary between the
   * output and its right neighbor, depending on even_lr.
   * So it might be 0111100 or 0011110, where output is on the center
   * of these arrays.
   */
  lead = 0;

  if (box_width % 2)
    /* Odd-width kernel */
    {
      output = lead - (box_width - 1) / 2;
      trail  = lead - box_width;
    }
  else
    /* Even-width kernel. */
    {
      /* Right offset */
      if (even_offset == 1)
        {
          output = lead + 1 - box_width / 2;
          trail  = lead - box_width;
        }
      /* Left offset */
      else if (even_offset == -1)
        {
          output = lead - box_width / 2;
          trail  = lead - box_width;
        }
      /* If even_offset isn't 1 or -1, there's some error. */
      else
        g_assert_not_reached ();
    }

  /* Initialize accumulator */
  for (i = 0; i < bpp; i++)
    ac[i] = 0;

  /* As the kernel moves across the image, it has a leading edge and a
   * trailing edge, and the output is in the middle. */
  while (output < len)
    {
      /* The number of pixels that are both in the image and
       * currently covered by the kernel. This is necessary to
       * handle edge cases. */
      guint coverage = (lead < len ? lead : len-1) - (trail >=0 ? trail : -1);

#ifdef READABLE_BOXBLUR_CODE
/* The code here does the same as the code below, but the code below
 * has been optimized by moving the if statements out of the tight for
 * loop, and is harder to understand.
 * Don't use both this code and the code below. */
      for (i = 0; i < bpp; i++)
        {
          /* If the leading edge of the kernel is still on the image,
           * add the value there to the accumulator. */
          if (lead < len)
            ac[i] += src[bpp * lead + i];

          /* If the trailing edge of the kernel is on the image,
           * subtract the value there from the accumulator. */
          if (trail >= 0)
            ac[i] -= src[bpp * trail + i];

          /* Take the averaged value in the accumulator and store
           * that value in the output. The number of pixels currently
           * stored in the accumulator can be less than the nominal
           * width of the kernel because the kernel can go "over the edge"
           * of the image. */
          if (output >= 0)
            dest[bpp * output + i] = (ac[i] + (coverage >> 1)) / coverage;
        }
#endif

      /* If the leading edge of the kernel is still on the image... */
      if (lead < len)
        {
          /* If the trailing edge of the kernel is on the image. (Since
           * the output is in between the lead and trail, it must be on
           * the image. */
          if (trail >= 0)
            for (i = 0; i < bpp; i++)
              {
                ac[i] += src[bpp * lead + i];
                ac[i] -= src[bpp * trail + i];
                dest[bpp * output + i] = (ac[i] + (coverage >> 1)) / coverage;
              }
          /* If the output is on the image, but the trailing edge isn't yet
           * on the image. */
          else if (output >= 0)
            for (i = 0; i < bpp; i++)
              {
                ac[i] += src[bpp * lead + i];
                dest[bpp * output + i] = (ac[i] + (coverage >> 1)) / coverage;
              }
          /* If leading edge is on the image, but the output and trailing
           * edge aren't yet on the image. */
          else
            for (i = 0; i < bpp; i++)
              ac[i] += src[bpp * lead + i];
        }
      /* If the leading edge has gone off the image, but the output and
       * trailing edge are on the image. (The big loop exits when the
       * output goes off the image. */
      else if (trail >= 0)
        {
          for (i = 0; i < bpp; i++)
            {
              ac[i] -= src[bpp * trail + i];
              dest[bpp * output + i] = (ac[i] + (coverage >> 1)) / coverage;
            }
        }
      /* Leading has gone off the image and trailing isn't yet in it
       * (small image) */
      else if (output >= 0)
        {
          for (i = 0; i < bpp; i++)
            dest[bpp * output + i] = (ac[i] + (coverage >> 1)) / coverage;
        }

      lead++;
      output++;
      trail++;
    }
}


/* This function is written as if it is blurring a column at a time,
 * even though it can operate on rows, too.  There is no difference
 * in the processing of the lines, at least to the blur_line function.
 */
static void
gaussian_blur_line (const gdouble *cmatrix,
                    const gint     cmatrix_length,
                    const guchar  *src,
                    guchar        *dest,
                    const gint     len,
                    const gint     bpp)
{
  const guchar  *src_p;
  const guchar  *src_p1;
  const gint     cmatrix_middle = cmatrix_length / 2;
  gint           row;
  gint           i, j;

  /* This first block is the same as the optimized version --
   * it is only used for very small pictures, so speed isn't a
   * big concern.
   */
  if (cmatrix_length > len)
    {
      for (row = 0; row < len; row++)
        {
          /* find the scale factor */
          gdouble scale = 0;

          for (j = 0; j < len; j++)
            {
              /* if the index is in bounds, add it to the scale counter */
              if (j + cmatrix_middle - row >= 0 &&
                  j + cmatrix_middle - row < cmatrix_length)
                scale += cmatrix[j];
            }

          src_p = src;

          for (i = 0; i < bpp; i++)
            {
              gdouble sum = 0;

              src_p1 = src_p++;

              for (j = 0; j < len; j++)
                {
                  if (j + cmatrix_middle - row >= 0 &&
                      j + cmatrix_middle - row < cmatrix_length)
                    sum += *src_p1 * cmatrix[j];

                  src_p1 += bpp;
                }

              *dest++ = (guchar) ROUND (sum / scale);
            }
        }
    }
  else
    {
      /* for the edge condition, we only use available info and scale to one */
      for (row = 0; row < cmatrix_middle; row++)
        {
          /* find scale factor */
          gdouble scale = 0;

          for (j = cmatrix_middle - row; j < cmatrix_length; j++)
            scale += cmatrix[j];

          src_p = src;

          for (i = 0; i < bpp; i++)
            {
              gdouble sum = 0;

              src_p1 = src_p++;

              for (j = cmatrix_middle - row; j < cmatrix_length; j++)
                {
                  sum += *src_p1 * cmatrix[j];
                  src_p1 += bpp;
                }

              *dest++ = (guchar) ROUND (sum / scale);
            }
        }

      /* go through each pixel in each col */
      for (; row < len - cmatrix_middle; row++)
        {
          src_p = src + (row - cmatrix_middle) * bpp;

          for (i = 0; i < bpp; i++)
            {
              gdouble sum = 0;

              src_p1 = src_p;

              for (j = 0; j < cmatrix_length; j++)
                {
                  sum += cmatrix[j] * *src_p1;
                  src_p1 += bpp;
                }

              src_p++;
              *dest++ = (guchar) ROUND (sum);
            }
        }

      /* for the edge condition, we only use available info and scale to one */
      for (; row < len; row++)
        {
          /* find scale factor */
          gdouble scale = 0;

          for (j = 0; j < len - row + cmatrix_middle; j++)
            scale += cmatrix[j];

          src_p = src + (row - cmatrix_middle) * bpp;

          for (i = 0; i < bpp; i++)
            {
              gdouble sum = 0;

              src_p1 = src_p++;

              for (j = 0; j < len - row + cmatrix_middle; j++)
                {
                  sum += *src_p1 * cmatrix[j];
                  src_p1 += bpp;
                }

              *dest++ = (guchar) ROUND (sum / scale);
            }
        }
    }
}

static void
unsharp_mask (GimpDrawable *drawable,
              gdouble       radius,
              gdouble       amount)
{
  GimpPixelRgn srcPR, destPR;
  gint         x1, y1, width, height;

  /* initialize pixel regions */
  gimp_pixel_rgn_init (&srcPR, drawable,
                       0, 0, drawable->width, drawable->height, FALSE, FALSE);
  gimp_pixel_rgn_init (&destPR, drawable,
                       0, 0, drawable->width, drawable->height, TRUE, TRUE);

  /* Get the input */
  if (! gimp_drawable_mask_intersect (drawable->drawable_id,
                                      &x1, &y1, &width, &height))
    return;

  unsharp_region (&srcPR, &destPR, drawable->bpp,
                  radius, amount,
                  x1, x1 + width, y1, y1 + height,
                  TRUE);

  gimp_drawable_flush (drawable);
  gimp_drawable_merge_shadow (drawable->drawable_id, TRUE);
  gimp_drawable_update (drawable->drawable_id, x1, y1, width, height);
}

/* Perform an unsharp mask on the region, given a source region, dest.
 * region, width and height of the regions, and corner coordinates of
 * a subregion to act upon.  Everything outside the subregion is unaffected.
 */
static void
unsharp_region (GimpPixelRgn *srcPR,
                GimpPixelRgn *destPR,
                gint          bpp,
                gdouble       radius, /* Radius, AKA standard deviation */
                gdouble       amount,
                gint          x1,
                gint          x2,
                gint          y1,
                gint          y2,
                gboolean      show_progress)
{
  guchar     *src;                /* Temporary copy of source row/col      */
  guchar     *dest;               /* Temporary copy of destination row/col */
  const gint  width   = x2 - x1;
  const gint  height  = y2 - y1;
  gdouble    *cmatrix = NULL;     /* Convolution matrix (for gaussian)     */
  gint        cmatrix_length = 0;
  gint        row, col;           /* Row, column counters                  */
  const gint  threshold = unsharp_params.threshold;
  gboolean    box_blur;           /* If we want to use a three pass box
                                     blur instead of a gaussian blur       */
  gint        box_width = 0;

  if (show_progress)
    gimp_progress_init (_("Blurring"));

  /* If the radius is less than 10, use a true gaussian kernel.  This
   * is slower, but more accurate and allows for finer adjustments.
   * Otherwise use a three-pass box blur; this is much faster but it
   * isn't a perfect approximation, and it only allows radius
   * increments of about 0.42.
   */
  if (radius < 10)
    {
      box_blur = FALSE;
      /* If true gaussian, generate convolution matrix
         and make sure it's smaller than each dimension */
      cmatrix_length = gen_convolve_matrix (radius, &cmatrix);
    }
  else
    {
      box_blur = TRUE;
      /* Three box blurs of this width approximate a gaussian */
      box_width = ROUND (radius * 3 * sqrt (2 * G_PI) / 4);
    }

  /* Allocate buffers temporary copies of a row/column */
  src  = g_new (guchar, MAX (width, height) * bpp);
  dest = g_new (guchar, MAX (width, height) * bpp);

  /* Blur the rows */
  for (row = 0; row < height; row++)
    {
      gimp_pixel_rgn_get_row (srcPR, src, x1, y1 + row, width);

      if (box_blur)
        {
          /* Odd-width box blur: repeat 3 times, centered on output pixel.
           * Swap back and forth between the buffers. */
          if (box_width % 2)
            {
              box_blur_line (box_width, 0, src, dest, width, bpp);
              box_blur_line (box_width, 0, dest, src, width, bpp);
              box_blur_line (box_width, 0, src, dest, width, bpp);
            }
          /* Even-width box blur:
           * This method is suggested by the specification for SVG.
           * One pass with width n, centered between output and right pixel
           * One pass with width n, centered between output and left pixel
           * One pass with width n+1, centered on output pixel
           * Swap back and forth between buffers.
           */
          else
            {
              box_blur_line (box_width,  -1, src, dest, width, bpp);
              box_blur_line (box_width,   1, dest, src, width, bpp);
              box_blur_line (box_width+1, 0, src, dest, width, bpp);
            }
        }
      else
        {
          /* Gaussian blur */
          gaussian_blur_line (cmatrix, cmatrix_length, src, dest, width, bpp);
        }

      gimp_pixel_rgn_set_row (destPR, dest, x1, y1 + row, width);

      if (show_progress && row % 64 == 0)
        gimp_progress_update ((gdouble) row / (3 * height));
    }

  /* Blur the cols. Essentially same as above. */
  for (col = 0; col < width; col++)
    {
      gimp_pixel_rgn_get_col (destPR, src, x1 + col, y1, height);

      if (box_blur)
        {
          /* Odd-width box blur */
          if (box_width % 2)
            {
              box_blur_line (box_width, 0, src, dest, height, bpp);
              box_blur_line (box_width, 0, dest, src, height, bpp);
              box_blur_line (box_width, 0, src, dest, height, bpp);
            }
          /* Even-width box blur */
          else
            {
              box_blur_line (box_width,  -1, src, dest, height, bpp);
              box_blur_line (box_width,   1, dest, src, height, bpp);
              box_blur_line (box_width+1, 0, src, dest, height, bpp);
            }
        }
      else
        {
          /* Gaussian blur */
          gaussian_blur_line (cmatrix, cmatrix_length, src, dest,height, bpp);
        }

      gimp_pixel_rgn_set_col (destPR, dest, x1 + col, y1, height);

      if (show_progress && col % 64 == 0)
        gimp_progress_update ((gdouble) col / (3 * width) + 0.33);
    }

  if (show_progress)
    gimp_progress_set_text (_("Merging"));

  /* merge the source and destination (which currently contains
     the blurred version) images */
  for (row = 0; row < height; row++)
    {
      const guchar *s = src;
      guchar       *d = dest;
      gint          u, v;

      /* get source row */
      gimp_pixel_rgn_get_row (srcPR, src, x1, y1 + row, width);

      /* get dest row */
      gimp_pixel_rgn_get_row (destPR, dest, x1, y1 + row, width);

      /* combine the two */
      for (u = 0; u < width; u++)
        {
          for (v = 0; v < bpp; v++)
            {
              gint value;
              gint diff = *s - *d;

              /* do tresholding */
              if (abs (2 * diff) < threshold)
                diff = 0;

              value = *s++ + amount * diff;
              *d++ = CLAMP (value, 0, 255);
            }
        }

      if (show_progress && row % 64 == 0)
        gimp_progress_update ((gdouble) row / (3 * height) + 0.67);

      gimp_pixel_rgn_set_row (destPR, dest, x1, y1 + row, width);
    }

  if (show_progress)
    gimp_progress_update (1.0);

  g_free (dest);
  g_free (src);
  g_free (cmatrix);
}

/* generates a 1-D convolution matrix to be used for each pass of
 * a two-pass gaussian blur.  Returns the length of the matrix.
 */
static gint
gen_convolve_matrix (gdouble   radius,
                     gdouble **cmatrix_p)
{
  gdouble *cmatrix;
  gdouble  std_dev;
  gdouble  sum;
  gint     matrix_length;
  gint     i, j;

  /* we want to generate a matrix that goes out a certain radius
   * from the center, so we have to go out ceil(rad-0.5) pixels,
   * including the center pixel.  Of course, that's only in one direction,
   * so we have to go the same amount in the other direction, but not count
   * the center pixel again.  So we double the previous result and subtract
   * one.
   * The radius parameter that is passed to this function is used as
   * the standard deviation, and the radius of effect is the
   * standard deviation * 2.  It's a little confusing.
   */
  radius = fabs (radius) + 1.0;

  std_dev = radius;
  radius = std_dev * 2;

  /* go out 'radius' in each direction */
  matrix_length = 2 * ceil (radius - 0.5) + 1;
  if (matrix_length <= 0)
    matrix_length = 1;

  *cmatrix_p = g_new (gdouble, matrix_length);
  cmatrix = *cmatrix_p;

  /*  Now we fill the matrix by doing a numeric integration approximation
   * from -2*std_dev to 2*std_dev, sampling 50 points per pixel.
   * We do the bottom half, mirror it to the top half, then compute the
   * center point.  Otherwise asymmetric quantization errors will occur.
   *  The formula to integrate is e^-(x^2/2s^2).
   */

  /* first we do the top (right) half of matrix */
  for (i = matrix_length / 2 + 1; i < matrix_length; i++)
    {
      gdouble base_x = i - (matrix_length / 2) - 0.5;

      sum = 0;
      for (j = 1; j <= 50; j++)
        {
          gdouble r = base_x + 0.02 * j;

          if (r <= radius)
            sum += exp (- SQR (r) / (2 * SQR (std_dev)));
        }

      cmatrix[i] = sum / 50;
    }

  /* mirror the thing to the bottom half */
  for (i = 0; i <= matrix_length / 2; i++)
    cmatrix[i] = cmatrix[matrix_length - 1 - i];

  /* find center val -- calculate an odd number of quanta to make it
   * symmetric, even if the center point is weighted slightly higher
   * than others.
   */
  sum = 0;
  for (j = 0; j <= 50; j++)
    sum += exp (- SQR (- 0.5 + 0.02 * j) / (2 * SQR (std_dev)));

  cmatrix[matrix_length / 2] = sum / 51;

  /* normalize the distribution by scaling the total sum to one */
  sum = 0;
  for (i = 0; i < matrix_length; i++)
    sum += cmatrix[i];

  for (i = 0; i < matrix_length; i++)
    cmatrix[i] = cmatrix[i] / sum;

  return matrix_length;
}

static gboolean
unsharp_mask_dialog (GimpDrawable *drawable)
{
  GtkWidget     *dialog;
  GtkWidget     *main_vbox;
  GtkWidget     *preview;
  GtkWidget     *table;
  GtkAdjustment *adj;
  gboolean       run;

  gimp_ui_init (PLUG_IN_BINARY, TRUE);

  dialog = gimp_dialog_new (_("Unsharp Mask"), PLUG_IN_ROLE,
                            NULL, 0,
                            gimp_standard_help_func, PLUG_IN_PROC,

                            _("_Cancel"), GTK_RESPONSE_CANCEL,
                            _("_OK"),     GTK_RESPONSE_OK,

                            NULL);

  gtk_dialog_set_alternative_button_order (GTK_DIALOG (dialog),
                                           GTK_RESPONSE_OK,
                                           GTK_RESPONSE_CANCEL,
                                           -1);

  gimp_window_set_transient (GTK_WINDOW (dialog));

  main_vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 12);
  gtk_container_set_border_width (GTK_CONTAINER (main_vbox), 12);
  gtk_box_pack_start (GTK_BOX (gtk_dialog_get_content_area (GTK_DIALOG (dialog))),
                      main_vbox, TRUE, TRUE, 0);
  gtk_widget_show (main_vbox);

  preview = gimp_drawable_preview_new_from_drawable_id (drawable->drawable_id);
  gtk_box_pack_start (GTK_BOX (main_vbox), preview, TRUE, TRUE, 0);
  gtk_widget_show (preview);

  g_signal_connect (preview, "invalidated",
                    G_CALLBACK (preview_update),
                    drawable);

  table = gtk_table_new (3, 3, FALSE);
  gtk_table_set_col_spacings (GTK_TABLE (table), 6);
  gtk_table_set_row_spacings (GTK_TABLE (table), 6);
  gtk_box_pack_start (GTK_BOX (main_vbox), table, FALSE, FALSE, 0);
  gtk_widget_show (table);

  adj = gimp_scale_entry_new (GTK_TABLE (table), 0, 0,
                              _("_Radius:"), SCALE_WIDTH, ENTRY_WIDTH,
                              unsharp_params.radius, 0.1, 500.0, 0.1, 1.0, 1,
                              TRUE, 0, 0,
                              NULL, NULL);

  g_signal_connect (adj, "value-changed",
                    G_CALLBACK (gimp_double_adjustment_update),
                    &unsharp_params.radius);
  g_signal_connect_swapped (adj, "value-changed",
                            G_CALLBACK (gimp_preview_invalidate),
                            preview);

  adj = gimp_scale_entry_new (GTK_TABLE (table), 0, 1,
                              _("_Amount:"), SCALE_WIDTH, ENTRY_WIDTH,
                              unsharp_params.amount, 0.0, 10.0, 0.01, 0.1, 2,
                              TRUE, 0, 0,
                              NULL, NULL);

  g_signal_connect (adj, "value-changed",
                    G_CALLBACK (gimp_double_adjustment_update),
                    &unsharp_params.amount);
  g_signal_connect_swapped (adj, "value-changed",
                            G_CALLBACK (gimp_preview_invalidate),
                            preview);

  adj = gimp_scale_entry_new (GTK_TABLE (table), 0, 2,
                              _("_Threshold:"), SCALE_WIDTH, ENTRY_WIDTH,
                              unsharp_params.threshold,
                              0.0, 255.0, 1.0, 10.0, 0,
                              TRUE, 0, 0,
                              NULL, NULL);

  g_signal_connect (adj, "value-changed",
                    G_CALLBACK (gimp_int_adjustment_update),
                    &unsharp_params.threshold);
  g_signal_connect_swapped (adj, "value-changed",
                            G_CALLBACK (gimp_preview_invalidate),
                            preview);

  gtk_widget_show (dialog);

  run = (gimp_dialog_run (GIMP_DIALOG (dialog)) == GTK_RESPONSE_OK);

  gtk_widget_destroy (dialog);

  return run;
}

static void
preview_update (GimpPreview  *preview,
                GimpDrawable *drawable)
{
  gint          x1, x2;
  gint          y1, y2;
  gint          x, y;
  gint          width, height;
  gint          border;
  GimpPixelRgn  srcPR;
  GimpPixelRgn  destPR;

  gimp_pixel_rgn_init (&srcPR, drawable,
                       0, 0, drawable->width, drawable->height, FALSE, FALSE);
  gimp_pixel_rgn_init (&destPR, drawable,
                       0, 0, drawable->width, drawable->height, TRUE, TRUE);

  gimp_preview_get_position (preview, &x, &y);
  gimp_preview_get_size (preview, &width, &height);

  /* enlarge the region to avoid artefacts at the edges of the preview */
  border = 2.0 * unsharp_params.radius + 0.5;
  x1 = MAX (0, x - border);
  y1 = MAX (0, y - border);
  x2 = MIN (x + width  + border, drawable->width);
  y2 = MIN (y + height + border, drawable->height);

  unsharp_region (&srcPR, &destPR, drawable->bpp,
                  unsharp_params.radius, unsharp_params.amount,
                  x1, x2, y1, y2,
                  FALSE);

  gimp_pixel_rgn_init (&destPR, drawable, x, y, width, height, FALSE, TRUE);
  gimp_drawable_preview_draw_region (GIMP_DRAWABLE_PREVIEW (preview), &destPR);
}
