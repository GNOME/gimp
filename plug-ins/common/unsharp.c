/*
 * Copyright (C) 1999 Winston Chang
 *                    <winstonc@cs.wisc.edu>
 *                    <winston@stdout.org>
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

#include "config.h"

#include <stdlib.h>

#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

#include "libgimp/stdplugins-intl.h"


#define PLUG_IN_PROC    "plug-in-unsharp-mask"
#define PLUG_IN_BINARY  "unsharp"

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

static void      blur_line           (const gdouble  *ctable,
                                      const gdouble  *cmatrix,
                                      const gint      cmatrix_length,
                                      const guchar   *src,
                                      guchar         *dest,
                                      const gint      len,
                                      const gint      bytes);
static gint      gen_convolve_matrix (gdouble         std_dev,
                                      gdouble       **cmatrix);
static gdouble * gen_lookup_table    (const gdouble  *cmatrix,
                                      gint            cmatrix_length);
static void      unsharp_region      (GimpPixelRgn   *srcPTR,
                                      GimpPixelRgn   *dstPTR,
                                      gint            bytes,
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
static void      preview_update      (GimpPreview    *preview);


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
      { GIMP_PDB_INT32,    "run-mode",  "Interactive, non-interactive" },
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
                          "1999",
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

/* This function is written as if it is blurring a column at a time,
 * even though it can operate on rows, too.  There is no difference
 * in the processing of the lines, at least to the blur_line function.
 */
static void
blur_line (const gdouble *ctable,
           const gdouble *cmatrix,
           const gint     cmatrix_length,
           const guchar  *src,
           guchar        *dest,
           const gint     len,
           const gint     bytes)
{
  const gdouble *cmatrix_p;
  const gdouble *ctable_p;
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

          for (i = 0; i < bytes; i++)
            {
              gdouble sum = 0;

              src_p1 = src_p++;

              for (j = 0; j < len; j++)
                {
                  if (j + cmatrix_middle - row >= 0 &&
                      j + cmatrix_middle - row < cmatrix_length)
                    sum += *src_p1 * cmatrix[j];

                  src_p1 += bytes;
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

          for (i = 0; i < bytes; i++)
            {
              gdouble sum = 0;

              src_p1 = src_p++;

              for (j = cmatrix_middle - row; j < cmatrix_length; j++)
                {
                  sum += *src_p1 * cmatrix[j];
                  src_p1 += bytes;
                }

              *dest++ = (guchar) ROUND (sum / scale);
            }
        }

      /* go through each pixel in each col */
      for (; row < len - cmatrix_middle; row++)
        {
          src_p = src + (row - cmatrix_middle) * bytes;

          for (i = 0; i < bytes; i++)
            {
              gdouble sum = 0;

              cmatrix_p = cmatrix;
              src_p1 = src_p;
              ctable_p = ctable;

              for (j = 0; j < cmatrix_length; j++)
                {
                  sum += cmatrix[j] * *src_p1;
                  src_p1 += bytes;
                  ctable_p += 256;
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

          src_p = src + (row - cmatrix_middle) * bytes;

          for (i = 0; i < bytes; i++)
            {
              gdouble sum = 0;

              src_p1 = src_p++;

              for (j = 0; j < len - row + cmatrix_middle; j++)
                {
                  sum += *src_p1 * cmatrix[j];
                  src_p1 += bytes;
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
  gint         x1, y1, x2, y2;

  /* initialize pixel regions */
  gimp_pixel_rgn_init (&srcPR, drawable,
                       0, 0, drawable->width, drawable->height, FALSE, FALSE);
  gimp_pixel_rgn_init (&destPR, drawable,
                       0, 0, drawable->width, drawable->height, TRUE, TRUE);

  /* Get the input */
  gimp_drawable_mask_bounds (drawable->drawable_id, &x1, &y1, &x2, &y2);

  unsharp_region (&srcPR, &destPR, drawable->bpp,
                  radius, amount,
                  x1, x2, y1, y2,
                  TRUE);

  gimp_drawable_flush (drawable);
  gimp_drawable_merge_shadow (drawable->drawable_id, TRUE);
  gimp_drawable_update (drawable->drawable_id, x1, y1, x2 - x1, y2 - y1);
}

/* Perform an unsharp mask on the region, given a source region, dest.
 * region, width and height of the regions, and corner coordinates of
 * a subregion to act upon.  Everything outside the subregion is unaffected.
 */
static void
unsharp_region (GimpPixelRgn *srcPR,
                GimpPixelRgn *destPR,
                gint          bytes,
                gdouble       radius,
                gdouble       amount,
                gint          x1,
                gint          x2,
                gint          y1,
                gint          y2,
                gboolean      show_progress)
{
  guchar  *src;
  guchar  *dest;
  gint     width   = x2 - x1;
  gint     height  = y2 - y1;
  gdouble *cmatrix = NULL;
  gint     cmatrix_length;
  gdouble *ctable;
  gint     row, col;
  gint     threshold = unsharp_params.threshold;

  if (show_progress)
    gimp_progress_init (_("Blurring"));

  /* generate convolution matrix
     and make sure it's smaller than each dimension */
  cmatrix_length = gen_convolve_matrix (radius, &cmatrix);

  /* generate lookup table */
  ctable = gen_lookup_table (cmatrix, cmatrix_length);

  /* allocate buffers */
  src  = g_new (guchar, MAX (width, height) * bytes);
  dest = g_new (guchar, MAX (width, height) * bytes);

  /* blur the rows */
  for (row = 0; row < height; row++)
    {
      gimp_pixel_rgn_get_row (srcPR, src, x1, y1 + row, width);
      blur_line (ctable, cmatrix, cmatrix_length, src, dest, width, bytes);
      gimp_pixel_rgn_set_row (destPR, dest, x1, y1 + row, width);

      if (show_progress && row % 8 == 0)
        gimp_progress_update ((gdouble) row / (3 * height));
    }

  /* blur the cols */
  for (col = 0; col < width; col++)
    {
      gimp_pixel_rgn_get_col (destPR, src, x1 + col, y1, height);
      blur_line (ctable, cmatrix, cmatrix_length, src, dest, height, bytes);
      gimp_pixel_rgn_set_col (destPR, dest, x1 + col, y1, height);

      if (show_progress && col % 8 == 0)
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
          for (v = 0; v < bytes; v++)
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

      if (show_progress && row % 8 == 0)
        gimp_progress_update ((gdouble) row / (3 * height) + 0.67);

      gimp_pixel_rgn_set_row (destPR, dest, x1, y1 + row, width);
    }

  if (show_progress)
    gimp_progress_update (1.0);

  g_free (dest);
  g_free (src);
  g_free (ctable);
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
   * inlcuding the center pixel.  Of course, that's only in one direction,
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
          if (base_x + 0.02 * j <= radius)
            sum += exp (- SQR (base_x + 0.02 * j) / (2 * SQR (std_dev)));
        }

      cmatrix[i] = sum / 50;
    }

  /* mirror the thing to the bottom half */
  for (i = 0; i <= matrix_length / 2; i++)
    cmatrix[i] = cmatrix[matrix_length - 1 - i];

  /* find center val -- calculate an odd number of quanta to make it symmetric,
   * even if the center point is weighted slightly higher than others. */
  sum = 0;
  for (j = 0; j <= 50; j++)
    sum += exp (- SQR (0.5 + 0.02 * j) / (2 * SQR (std_dev)));

  cmatrix[matrix_length / 2] = sum / 51;

  /* normalize the distribution by scaling the total sum to one */
  sum = 0;
  for (i = 0; i < matrix_length; i++)
    sum += cmatrix[i];

  for (i = 0; i < matrix_length; i++)
    cmatrix[i] = cmatrix[i] / sum;

  return matrix_length;
}

/* ----------------------- gen_lookup_table ----------------------- */
/* generates a lookup table for every possible product of 0-255 and
   each value in the convolution matrix.  The returned array is
   indexed first by matrix position, then by input multiplicand (?)
   value.
*/
static gdouble *
gen_lookup_table (const gdouble *cmatrix,
                  gint           cmatrix_length)
{
  gdouble       *lookup_table   = g_new (gdouble, cmatrix_length * 256);
  gdouble       *lookup_table_p = lookup_table;
  const gdouble *cmatrix_p      = cmatrix;
  gint           i, j;

  for (i = 0; i < cmatrix_length; i++)
    {
      for (j = 0; j < 256; j++)
        *(lookup_table_p++) = *cmatrix_p * (gdouble) j;

      cmatrix_p++;
    }

  return lookup_table;
}

static gboolean
unsharp_mask_dialog (GimpDrawable *drawable)
{
  GtkWidget *dialog;
  GtkWidget *main_vbox;
  GtkWidget *preview;
  GtkWidget *table;
  GtkObject *adj;
  gboolean   run;

  gimp_ui_init (PLUG_IN_BINARY, TRUE);

  dialog = gimp_dialog_new (_("Unsharp Mask"), PLUG_IN_BINARY,
                            NULL, 0,
                            gimp_standard_help_func, PLUG_IN_PROC,

                            GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                            GTK_STOCK_OK,     GTK_RESPONSE_OK,

                            NULL);

  gtk_dialog_set_alternative_button_order (GTK_DIALOG (dialog),
                                           GTK_RESPONSE_OK,
                                           GTK_RESPONSE_CANCEL,
                                           -1);

  gimp_window_set_transient (GTK_WINDOW (dialog));

  main_vbox = gtk_vbox_new (FALSE, 12);
  gtk_container_set_border_width (GTK_CONTAINER (main_vbox), 12);
  gtk_container_add (GTK_CONTAINER (GTK_DIALOG (dialog)->vbox), main_vbox);
  gtk_widget_show (main_vbox);

  preview = gimp_drawable_preview_new (drawable, NULL);
  gtk_box_pack_start (GTK_BOX (main_vbox), preview, TRUE, TRUE, 0);
  gtk_widget_show (preview);

  g_signal_connect (preview, "invalidated",
                    G_CALLBACK (preview_update),
                    NULL);

  table = gtk_table_new (3, 3, FALSE);
  gtk_table_set_col_spacings (GTK_TABLE (table), 6);
  gtk_table_set_row_spacings (GTK_TABLE (table), 6);
  gtk_box_pack_start (GTK_BOX (main_vbox), table, FALSE, FALSE, 0);
  gtk_widget_show (table);

  adj = gimp_scale_entry_new (GTK_TABLE (table), 0, 0,
                              _("_Radius:"), SCALE_WIDTH, ENTRY_WIDTH,
                              unsharp_params.radius, 0.1, 120.0, 0.1, 1.0, 1,
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
preview_update (GimpPreview *preview)
{
  GimpDrawable *drawable;
  gint          x1, x2;
  gint          y1, y2;
  gint          x, y;
  gint          width, height;
  gint          border;
  GimpPixelRgn  srcPR;
  GimpPixelRgn  destPR;

  drawable =
    gimp_drawable_preview_get_drawable (GIMP_DRAWABLE_PREVIEW (preview));

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
