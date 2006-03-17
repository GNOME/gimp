/* Selective gaussian blur filter for the GIMP, version 0.1
 * Adapted from the original gaussian blur filter by Spencer Kimball and
 * Peter Mattis.
 *
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 * Copyright (C) 1999 Thom van Os <thom@vanos.com>
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
 *
 * To do:
 *      - support for horizontal or vertical only blur
 *      - use memory more efficiently, smaller regions at a time
 *      - integrating with other convolution matrix based filters ?
 *      - create more selective and adaptive filters
 *      - threading
 *      - optimization
 */

#include "config.h"

#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

#include "libgimp/stdplugins-intl.h"


#define PLUG_IN_PROC   "plug-in-sel-gauss"
#define PLUG_IN_BINARY "sel_gauss"


typedef struct
{
  gdouble  radius;
  gint     maxdelta;
  gboolean update_preview;
} BlurValues;


/* Declare local functions.
 */
static void      query            (void);
static void      run              (const gchar      *name,
                                   gint              nparams,
                                   const GimpParam  *param,
                                   gint             *nreturn_vals,
                                   GimpParam       **return_vals);

static void      sel_gauss        (GimpDrawable     *drawable,
                                   gdouble           radius,
                                   gint              maxdelta);
static gboolean  sel_gauss_dialog (GimpDrawable     *drawable);
static void      preview_update   (GimpPreview      *preview);


GimpPlugInInfo PLUG_IN_INFO =
{
  NULL,  /* init_proc  */
  NULL,  /* quit_proc  */
  query, /* query_proc */
  run,   /* run_proc   */
};

static BlurValues bvals =
{
  5.0, /* radius   */
  50,  /* maxdelta */
  TRUE /* update_preview */
};

MAIN ()

static void
query (void)
{
  static GimpParamDef args[] =
  {
    { GIMP_PDB_INT32,    "run-mode",  "Interactive, non-interactive" },
    { GIMP_PDB_IMAGE,    "image",     "Input image (unused)"         },
    { GIMP_PDB_DRAWABLE, "drawable",  "Input drawable"               },
    { GIMP_PDB_FLOAT,    "radius",    "Radius of gaussian blur (in pixels, > 0.0)" },
    { GIMP_PDB_INT32,    "max-delta", "Maximum delta"                }
  };

  gimp_install_procedure (PLUG_IN_PROC,
                          N_("Blur neighboring pixels, but only in low-contrast areas"),
                          "This filter functions similar to the regular "
                          "gaussian blur filter except that neighbouring "
                          "pixels that differ more than the given maxdelta "
                          "parameter will not be blended with. This way with "
                          "the correct parameters, an image can be smoothed "
                          "out without losing details. However, this filter "
                          "can be rather slow.",
                          "Thom van Os",
                          "Thom van Os",
                          "1999",
                          N_("_Selective Gaussian Blur..."),
                          "RGB*, GRAY*",
                          GIMP_PLUGIN,
                          G_N_ELEMENTS (args), 0,
                          args, NULL);

  gimp_plugin_menu_register (PLUG_IN_PROC, "<Image>/Filters/Blur");
}

static void
run (const gchar      *name,
     gint              nparams,
     const GimpParam  *param,
     gint             *nreturn_vals,
     GimpParam       **return_vals)
{
  static GimpParam   values[1];
  GimpRunMode        run_mode;
  GimpPDBStatusType  status = GIMP_PDB_SUCCESS;
  GimpDrawable      *drawable;
  gdouble            radius;

  run_mode = param[0].data.d_int32;

  *nreturn_vals = 1;
  *return_vals  = values;

  INIT_I18N ();

  values[0].type          = GIMP_PDB_STATUS;
  values[0].data.d_status = status;

  /* Get the specified drawable */
  drawable = gimp_drawable_get (param[2].data.d_drawable);
  gimp_tile_cache_ntiles (2 * drawable->ntile_cols);

  switch (run_mode)
    {
    case GIMP_RUN_INTERACTIVE:
      /* Possibly retrieve data */
      gimp_get_data (PLUG_IN_PROC, &bvals);

      /* First acquire information with a dialog */
      if (! sel_gauss_dialog (drawable))
        return;
      break;

    case GIMP_RUN_NONINTERACTIVE:
      /* Make sure all the arguments are there! */
      if (nparams != 5)
        status = GIMP_PDB_CALLING_ERROR;
      if (status == GIMP_PDB_SUCCESS)
        {
          bvals.radius   = param[3].data.d_float;
          bvals.maxdelta = CLAMP (param[4].data.d_int32, 0, 255);

          if (bvals.radius <= 0.0)
            status = GIMP_PDB_CALLING_ERROR;
        }
      break;

    case GIMP_RUN_WITH_LAST_VALS:
      /* Possibly retrieve data */
      gimp_get_data (PLUG_IN_PROC, &bvals);
      break;

    default:
      break;
    }

  if (status != GIMP_PDB_SUCCESS)
    {
      values[0].data.d_status = status;
      return;
    }

  /* Make sure that the drawable is gray or RGB color */
  if (gimp_drawable_is_rgb (drawable->drawable_id) ||
      gimp_drawable_is_gray (drawable->drawable_id))
    {
      gimp_progress_init (_("Selective Gaussian Blur"));

      radius = fabs (bvals.radius) + 1.0;

      /* run the gaussian blur */
      sel_gauss (drawable, radius, bvals.maxdelta);

      /* Store data */
      if (run_mode == GIMP_RUN_INTERACTIVE)
        gimp_set_data (PLUG_IN_PROC, &bvals, sizeof (BlurValues));

      if (run_mode != GIMP_RUN_NONINTERACTIVE)
        gimp_displays_flush ();
    }
  else
    {
      gimp_message (_("Cannot operate on indexed color images."));
      status = GIMP_PDB_EXECUTION_ERROR;
    }

  gimp_drawable_detach (drawable);
  values[0].data.d_status = status;
}

static gboolean
sel_gauss_dialog (GimpDrawable *drawable)
{
  GtkWidget *dialog;
  GtkWidget *main_vbox;
  GtkWidget *preview;
  GtkWidget *table;
  GtkWidget *spinbutton;
  GtkObject *adj;
  gboolean   run;

  gimp_ui_init (PLUG_IN_BINARY, FALSE);

  dialog = gimp_dialog_new (_("Selective Gaussian Blur"), PLUG_IN_BINARY,
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

  preview = gimp_drawable_preview_new (drawable, &bvals.update_preview);
  gtk_box_pack_start (GTK_BOX (main_vbox), preview, TRUE, TRUE, 0);
  gtk_widget_show (preview);

  g_signal_connect (preview, "invalidated",
                    G_CALLBACK (preview_update),
                    NULL);

  table = gtk_table_new (2, 3, FALSE);
  gtk_table_set_col_spacings (GTK_TABLE (table), 6);
  gtk_table_set_row_spacings (GTK_TABLE (table), 6);
  gtk_box_pack_start (GTK_BOX (main_vbox), table, FALSE, FALSE, 0);
  gtk_widget_show (table);

  spinbutton = gimp_spin_button_new (&adj,
                                     bvals.radius, 0.0, G_MAXINT, 1.0, 5.0,
                                     0, 1, 2);
  gimp_table_attach_aligned (GTK_TABLE (table), 0, 0,
                             _("_Blur radius:"), 0.0, 0.5,
                             spinbutton, 1, TRUE);
  g_signal_connect (adj, "value-changed",
                    G_CALLBACK (gimp_double_adjustment_update),
                    &bvals.radius);
  g_signal_connect_swapped (spinbutton, "value-changed",
                            G_CALLBACK (gimp_preview_invalidate),
                            preview);

  adj = gimp_scale_entry_new (GTK_TABLE (table), 0, 1,
                              _("_Max. delta:"), 128, 0,
                              bvals.maxdelta, 0, 255, 1, 8, 0,
                              TRUE, 0, 0,
                              NULL, NULL);
  g_signal_connect (adj, "value-changed",
                    G_CALLBACK (gimp_int_adjustment_update),
                    &bvals.maxdelta);
  g_signal_connect_swapped (adj, "value-changed",
                            G_CALLBACK (gimp_preview_invalidate),
                            preview);

  gtk_widget_show (dialog);

  run = (gimp_dialog_run (GIMP_DIALOG (dialog)) == GTK_RESPONSE_OK);

  gtk_widget_destroy (dialog);

  return run;
}

static void
init_matrix (gdouble   radius,
             gdouble **mat,
             gint      num)
{
  gint    dx, dy;
  gdouble sd, c1, c2;

  /* This formula isn't really correct, but it'll do */
  sd = radius / 3.329042969;
  c1 = 1.0 / sqrt (2.0 * G_PI * sd);
  c2 = -2.0 * (sd * sd);

  for (dy = 0; dy < num; dy++)
    {
      for (dx = dy; dx < num; dx++)
        {
          mat[dx][dy] = c1 * exp ((dx * dx + dy * dy)/ c2);
          mat[dy][dx] = mat[dx][dy];
        }
    }
}

static void
matrixmult (guchar   *src,
            guchar   *dest,
            gint      width,
            gint      height,
            gdouble **mat,
            gint      numrad,
            gint      bytes,
            gboolean  has_alpha,
            gint      maxdelta,
            gboolean  preview_mode)
{
  gint     i, j, b, nb, x, y;
  gint     six, dix, tmp;
  gint     rowstride;
  gdouble  sum, fact, d, alpha = 1.0;
  guchar  *src_b, *src_db;
  gdouble *m;
  gint     offset;

  nb = bytes - (has_alpha ? 1 : 0);
  rowstride = width * bytes;

  for (y = 0; y < height; y++)
    {
      for (x = 0; x < width; x++)
        {
          dix = bytes * (width * y + x);
          if (has_alpha)
            dest[dix + nb] = src[dix + nb];

          for (b = 0; b < nb; b++)
            {
              sum = 0.0;
              fact = 0.0;
              src_db = src + dix + b;

              offset = rowstride * (y - numrad) + bytes * (x - numrad);

              for (i = 1 - numrad; i < numrad; i++)
                {
                  offset += bytes;
                  if (x + i < 0 || x + i >= width)
                    continue;

                  six = offset;
                  m = mat[ABS(i)];

                  src_b = src + six + b;

                  for (j = 1 - numrad; j < numrad; j++)
                    {
                      src_b += rowstride;
                      six += rowstride;

                      if (y + j < 0 || y + j >= height)
                        continue;

                      tmp = *src_db - *src_b;
                      if (tmp > maxdelta || tmp < -maxdelta)
                        continue;

                      d = m[ABS(j)];
                      if (has_alpha)
                        {
                          if (!src[six + nb])
                            continue;
                          alpha = (double) src[six + nb] / 255.0;
                          d *= alpha;
                        }
                      sum += d * *src_b;
                      fact += d;
                    }
                }
              if (fact == 0.0)
                dest[dix + b] = *src_db;
              else
                dest[dix + b] = sum / fact;
            }
        }

      if (!(y % 10) && !preview_mode)
        gimp_progress_update ((double)y / (double)height);
    }
}

static void
sel_gauss (GimpDrawable *drawable,
           gdouble       radius,
           gint          maxdelta)
{
  GimpPixelRgn src_rgn, dest_rgn;
  gint         width, height;
  gint         bytes;
  gboolean     has_alpha;
  guchar      *dest;
  guchar      *src;
  gint         x1, y1, x2, y2;
  gint         i;
  gdouble    **mat;
  gint         numrad;

  gimp_drawable_mask_bounds (drawable->drawable_id, &x1, &y1, &x2, &y2);

  width     = x2 - x1;
  height    = y2 - y1;
  bytes     = drawable->bpp;
  has_alpha = gimp_drawable_has_alpha (drawable->drawable_id);

  numrad = (gint) (radius + 1.0);
  mat = g_new (gdouble *, numrad);
  for (i = 0; i < numrad; i++)
    mat[i] = g_new (gdouble, numrad);
  init_matrix(radius, mat, numrad);

  src  = g_new (guchar, width * height * bytes);
  dest = g_new (guchar, width * height * bytes);

  gimp_pixel_rgn_init (&src_rgn,
                       drawable, x1, y1, width, height, FALSE, FALSE);
  gimp_pixel_rgn_get_rect (&src_rgn, src, x1, y1, width, height);

  matrixmult (src, dest, width, height, mat, numrad,
              bytes, has_alpha, maxdelta, FALSE);

  gimp_pixel_rgn_init (&dest_rgn,
                       drawable, x1, y1, width, height, TRUE, TRUE);
  gimp_pixel_rgn_set_rect (&dest_rgn, dest, x1, y1, width, height);

  /*  merge the shadow, update the drawable  */
  gimp_drawable_flush (drawable);
  gimp_drawable_merge_shadow (drawable->drawable_id, TRUE);
  gimp_drawable_update (drawable->drawable_id, x1, y1, width, height);

  /* free up buffers */
  g_free (src);
  g_free (dest);
  for (i = 0; i < numrad; i++)
    g_free (mat[i]);
  g_free (mat);
}

static void
preview_update (GimpPreview *preview)
{
  GimpDrawable  *drawable;
  glong          bytes;
  gint           x1, y1;
  guchar        *render_buffer;  /* Buffer to hold rendered image */
  gint           width;          /* Width of preview widget */
  gint           height;         /* Height of preview widget */

  GimpPixelRgn   srcPR;           /* Pixel region */
  guchar        *src;
  gboolean       has_alpha;
  gint           numrad, i;
  gdouble      **mat;
  gdouble       radius;

  /* Get drawable info */
  drawable =
    gimp_drawable_preview_get_drawable (GIMP_DRAWABLE_PREVIEW (preview));
  bytes = drawable->bpp;

  /*
   * Setup for filter...
   */
  gimp_preview_get_position (preview, &x1, &y1);
  gimp_preview_get_size (preview, &width, &height);

  /* initialize pixel regions */
  gimp_pixel_rgn_init (&srcPR, drawable,
                       x1, y1, width, height,
                       FALSE, FALSE);
  render_buffer = g_new (guchar, width * height * bytes);

  src = g_new (guchar, width * height * bytes);

  /* render image */
  gimp_pixel_rgn_get_rect (&srcPR, src, x1, y1, width, height);
  has_alpha = gimp_drawable_has_alpha (drawable->drawable_id);

  radius = fabs (bvals.radius) + 1.0;
  numrad = (gint) (radius + 1.0);

  mat = g_new (gdouble *, numrad);
  for (i = 0; i < numrad; i++)
    mat[i] = g_new (gdouble, numrad);

  init_matrix(radius, mat, numrad);

  matrixmult (src, render_buffer,
              width, height,
              mat, numrad,
              bytes, has_alpha, bvals.maxdelta, TRUE);

  for (i = 0; i < numrad; i++)
    g_free (mat[i]);
  g_free (mat);
  g_free (src);

  /*
   * Draw the preview image on the screen...
   */
  gimp_preview_draw_buffer (preview, render_buffer, width * bytes);

  g_free (render_buffer);
}
