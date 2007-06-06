/* GIMP - The GNU Image Manipulation Program
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

/* Neon filter for GIMP for BIPS
 *  -Spencer Kimball
 *
 * This filter works in a manner similar to the "edge"
 *  plug-in, but uses the first derivative of the gaussian
 *  operator to achieve resolution independence. The IIR
 *  method of calculating the effect is utilized to keep
 *  the processing time constant between large and small
 *  standard deviations.
 */

#include "config.h"

#include <string.h>

#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

#include "libgimp/stdplugins-intl.h"


#define PLUG_IN_PROC   "plug-in-neon"
#define PLUG_IN_BINARY "neon"


typedef struct
{
  gdouble radius;
  gdouble amount;
} NeonVals;


/*
 * Function prototypes.
 */

static void      query  (void);
static void      run    (const gchar      *name,
                         gint              nparams,
                         const GimpParam  *param,
                         gint             *nreturn_vals,
                         GimpParam       **return_vals);

static void      neon                (GimpDrawable *drawable,
                                      gdouble       radius,
                                      gdouble       amount,
                                      GimpPreview  *preview);

static gboolean  neon_dialog         (GimpDrawable *drawable);
static void      neon_preview_update (GimpPreview  *preview);

/*
 * Gaussian operator helper functions
 */
static void      find_constants      (gdouble  n_p[],
                                      gdouble  n_m[],
                                      gdouble  d_p[],
                                      gdouble  d_m[],
                                      gdouble  bd_p[],
                                      gdouble  bd_m[],
                                      gdouble  std_dev);
static void      transfer_pixels     (gdouble *src1,
                                      gdouble *src2,
                                      guchar  *dest,
                                      gint     bytes,
                                      gint     width);
static void      combine_to_gradient (guchar  *dest,
                                      guchar  *src2,
                                      gint     bytes,
                                      gint     width,
                                      gdouble  amount);


/***** Local vars *****/

const GimpPlugInInfo PLUG_IN_INFO =
{
  NULL,  /* init  */
  NULL,  /* quit  */
  query, /* query */
  run,   /* run   */
};

static NeonVals evals =
{
  5.0,   /* radius */
  0.0,   /* amount */
};


/***** Functions *****/

MAIN ()

static void
query (void)
{
  static const GimpParamDef args[] =
  {
    { GIMP_PDB_INT32,    "run-mode", "Interactive, non-interactive"          },
    { GIMP_PDB_IMAGE,    "image",    "Input image (unused)"                  },
    { GIMP_PDB_DRAWABLE, "drawable", "Input drawable"                        },
    { GIMP_PDB_FLOAT,    "radius",   "Radius of neon effect (in pixels)"     },
    { GIMP_PDB_FLOAT,    "amount",   "Effect enhancement variable (0.0 - 1.0)" },
  };

  gchar *help_string =
    "This filter works in a manner similar to the edge"
    "plug-in, but uses the first derivative of the gaussian"
    "operator to achieve resolution independence. The IIR"
    "method of calculating the effect is utilized to keep"
    "the processing time constant between large and small"
    "standard deviations.";

  gimp_install_procedure (PLUG_IN_PROC,
                          N_("Simulate the glowing boundary of a neon light"),
                          help_string,
                          "Spencer Kimball",
                          "Bit Specialists, Inc.",
                          "2002",
                          N_("_Neon..."),
                          "RGB*, GRAY*",
                          GIMP_PLUGIN,
                          G_N_ELEMENTS (args), 0,
                          args, NULL);

  gimp_plugin_menu_register (PLUG_IN_PROC, "<Image>/Filters/Edge-Detect");
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
  GimpDrawable      *drawable;
  GimpPDBStatusType  status = GIMP_PDB_SUCCESS;

  run_mode = param[0].data.d_int32;

  /*  Get the specified drawable  */
  drawable = gimp_drawable_get (param[2].data.d_drawable);

  /*  set the tile cache size so that the gaussian blur works well  */
  gimp_tile_cache_ntiles (2 * (MAX (drawable->ntile_rows, drawable->ntile_cols)));

  *nreturn_vals = 1;
  *return_vals  = values;

  values[0].type          = GIMP_PDB_STATUS;
  values[0].data.d_status = status;

  INIT_I18N();

  switch (run_mode)
    {
    case GIMP_RUN_INTERACTIVE:
      /*  Possibly retrieve data  */
      gimp_get_data (PLUG_IN_PROC, &evals);

      /*  First acquire information with a dialog  */
      if (! neon_dialog (drawable))
        return;
      break;

    case GIMP_RUN_NONINTERACTIVE:
      /*  Make sure all the arguments are there!  */
      if (nparams != 5)
        status = GIMP_PDB_CALLING_ERROR;
      if (status == GIMP_PDB_SUCCESS)
        {
          evals.radius = param[3].data.d_float;
          evals.amount = param[4].data.d_float;
        }
      break;

    case GIMP_RUN_WITH_LAST_VALS:
      /*  Possibly retrieve data  */
      gimp_get_data (PLUG_IN_PROC, &evals);
      break;

    default:
      break;
    }

  /* make sure the drawable exist and is not indexed */
  if (gimp_drawable_is_rgb (drawable->drawable_id) ||
      gimp_drawable_is_gray (drawable->drawable_id))
    {
      gimp_progress_init (_("Neon"));

      /*  run the neon effect  */
      neon (drawable, evals.radius, evals.amount, NULL);

      if (run_mode != GIMP_RUN_NONINTERACTIVE)
        gimp_displays_flush ();

      /*  Store data  */
      if (run_mode == GIMP_RUN_INTERACTIVE)
        gimp_set_data (PLUG_IN_PROC, &evals, sizeof (NeonVals));
    }
  else
    {
      g_message (_("Cannot operate on indexed color images."));
      status = GIMP_PDB_EXECUTION_ERROR;
    }

  values[0].data.d_status = status;

  gimp_drawable_detach (drawable);
}

/**********************************************/
/*              Neon main                     */
/**********************************************/

static void
neon (GimpDrawable *drawable,
      gdouble       radius,
      gdouble       amount,
      GimpPreview  *preview)
{
  GimpPixelRgn  src_rgn, dest_rgn;
  gint          width, height;
  gint          bytes, bpp;
  gboolean      has_alpha;
  guchar       *dest;
  guchar       *src, *src2, *sp_p, *sp_m;
  gdouble       n_p[5], n_m[5];
  gdouble       d_p[5], d_m[5];
  gdouble       bd_p[5], bd_m[5];
  gdouble      *val_p, *val_m, *vp, *vm;
  gint          x1, y1, x2, y2;
  gint          i, j;
  gint          row, col, b;
  gint          terms;
  gint          progress = 0, max_progress = 1;
  gint          initial_p[4];
  gint          initial_m[4];
  gdouble       std_dev;
  guchar       *preview_buffer1 = NULL;
  guchar       *preview_buffer2 = NULL;

  if (preview)
    {
      gimp_preview_get_position (preview, &x1, &y1);
      gimp_preview_get_size (preview, &width, &height);
      x2 = x1 + width;
      y2 = y1 + height;
    }
  else
    {
      gimp_drawable_mask_bounds (drawable->drawable_id, &x1, &y1, &x2, &y2);
      width  = (x2 - x1);
      height = (y2 - y1);
    }

  if (radius < 1.0)
    return;

  bytes     = drawable->bpp;
  bpp       = bytes;
  has_alpha = gimp_drawable_has_alpha(drawable->drawable_id);
  if (has_alpha)
    bpp--;

  val_p = g_new (gdouble, MAX (width, height) * bytes);
  val_m = g_new (gdouble, MAX (width, height) * bytes);

  src  = g_new (guchar, MAX (width, height) * bytes);
  src2 = g_new (guchar, MAX (width, height) * bytes);
  dest = g_new (guchar, MAX (width, height) * bytes);

  gimp_pixel_rgn_init (&src_rgn, drawable,
                       0, 0, drawable->width, drawable->height, FALSE, FALSE);

  if (preview)
    {
      preview_buffer1 = g_new (guchar, width * height * bytes);
      preview_buffer2 = g_new (guchar, width * height * bytes);
    }
  else
    {
      gimp_pixel_rgn_init (&dest_rgn, drawable,
                           0, 0, drawable->width, drawable->height, TRUE, TRUE);

      progress = 0;
      max_progress = (radius < 1.0 ) ? 0 : width * height * radius * 2;
    }

  /*  First the vertical pass  */
  radius  = fabs (radius) + 1.0;
  std_dev = sqrt (-(radius * radius) / (2 * log (1.0 / 255.0)));

  /*  derive the constants for calculating the gaussian from the std dev  */
  find_constants (n_p, n_m, d_p, d_m, bd_p, bd_m, std_dev);

  for (col = 0; col < width; col++)
    {
      memset (val_p, 0, height * bytes * sizeof (gdouble));
      memset (val_m, 0, height * bytes * sizeof (gdouble));

      gimp_pixel_rgn_get_col (&src_rgn, src, col + x1, y1, (y2 - y1));

      sp_p = src;
      sp_m = src + (height - 1) * bytes;
      vp = val_p;
      vm = val_m + (height - 1) * bytes;

      /*  Set up the first vals  */
      for (i = 0; i < bytes; i++)
        {
          initial_p[i] = sp_p[i];
          initial_m[i] = sp_m[i];
        }

      for (row = 0; row < height; row++)
        {
          gdouble *vpptr, *vmptr;

          terms = (row < 4) ? row : 4;

          for (b = 0; b < bpp; b++)
            {
              vpptr = vp + b; vmptr = vm + b;

              for (i = 0; i <= terms; i++)
                {
                  *vpptr += n_p[i] * sp_p[(-i * bytes) + b] -
                            d_p[i] * vp[(-i * bytes) + b];
                  *vmptr += n_m[i] * sp_m[(i * bytes) + b] -
                            d_m[i] * vm[(i * bytes) + b];
                }

              for (j = i; j <= 4; j++)
                {
                  *vpptr += (n_p[j] - bd_p[j]) * initial_p[b];
                  *vmptr += (n_m[j] - bd_m[j]) * initial_m[b];
                }
            }
          if (has_alpha)
            {
              vp[bpp] = sp_p[bpp];
              vm[bpp] = sp_m[bpp];
            }

          sp_p += bytes;
          sp_m -= bytes;
          vp   += bytes;
          vm   -= bytes;
        }

      transfer_pixels (val_p, val_m, dest, bytes, height);

      if (preview)
        {
          for (row = 0 ; row < height ; row++)
            memcpy (preview_buffer1 + (row * width + col) * bytes,
                    dest + bytes * row,
                    bytes);
        }
      else
        {
          gimp_pixel_rgn_set_col (&dest_rgn, dest, col + x1, y1, (y2 - y1));

          progress += height * radius;

          if ((col % 20) == 0)
            gimp_progress_update ((double) progress / (double) max_progress);
        }
    }

  /*  Now the horizontal pass  */
  gimp_pixel_rgn_init (&src_rgn, drawable,
                       0, 0, drawable->width, drawable->height, FALSE, FALSE);

  for (row = 0; row < height; row++)
    {
      memset (val_p, 0, width * bytes * sizeof (gdouble));
      memset (val_m, 0, width * bytes * sizeof (gdouble));

      gimp_pixel_rgn_get_row (&src_rgn, src, x1, row + y1, (x2 - x1));
      if (preview)
        {
          memcpy (src2,
                  preview_buffer1 + row * width * bytes,
                  width * bytes);
        }
      else
        {
          gimp_pixel_rgn_get_row (&dest_rgn, src2, x1, row + y1, (x2 - x1));
        }

      sp_p = src;
      sp_m = src + (width - 1) * bytes;
      vp = val_p;
      vm = val_m + (width - 1) * bytes;

      /*  Set up the first vals  */
      for (i = 0; i < bytes; i++)
        {
          initial_p[i] = sp_p[i];
          initial_m[i] = sp_m[i];
        }

      for (col = 0; col < width; col++)
        {
          gdouble *vpptr, *vmptr;

          terms = (col < 4) ? col : 4;

          for (b = 0; b < bpp; b++)
            {
              vpptr = vp + b; vmptr = vm + b;

              for (i = 0; i <= terms; i++)
                {
                  *vpptr += n_p[i] * sp_p[(-i * bytes) + b] -
                            d_p[i] * vp[(-i * bytes) + b];
                  *vmptr += n_m[i] * sp_m[(i * bytes) + b] -
                            d_m[i] * vm[(i * bytes) + b];
                }

              for (j = i; j <= 4; j++)
                {
                  *vpptr += (n_p[j] - bd_p[j]) * initial_p[b];
                  *vmptr += (n_m[j] - bd_m[j]) * initial_m[b];
                }
            }
          if (has_alpha)
            {
              vp[bpp] = sp_p[bpp];
              vm[bpp] = sp_m[bpp];
            }

          sp_p += bytes;
          sp_m -= bytes;
          vp   += bytes;
          vm   -= bytes;
        }

      transfer_pixels (val_p, val_m, dest, bytes, width);

      combine_to_gradient (dest, src2, bytes, width, amount);

      if (preview)
        {
          memcpy (preview_buffer2 + row * width * bytes,
                  dest,
                  width * bytes);
        }
      else
        {
          gimp_pixel_rgn_set_row (&dest_rgn, dest, x1, row + y1, (x2 - x1));

          progress += width * radius;
          if ((row % 20) == 0)
            gimp_progress_update ((double) progress / (double) max_progress);
        }
    }

  if (preview)
    {
      gimp_preview_draw_buffer (preview, preview_buffer2, width * bytes);
      g_free (preview_buffer1);
      g_free (preview_buffer2);
    }
  else
    {
      /*  now, merge horizontal and vertical into a magnitude  */
      gimp_pixel_rgn_init (&src_rgn, drawable,
                           0, 0, drawable->width, drawable->height,
                           FALSE, TRUE);

      /*  merge the shadow, update the drawable  */
      gimp_drawable_flush (drawable);
      gimp_drawable_merge_shadow (drawable->drawable_id, TRUE);
      gimp_drawable_update (drawable->drawable_id,
                            x1, y1, (x2 - x1), (y2 - y1));
    }
  /*  free up buffers  */
  g_free (val_p);
  g_free (val_m);
  g_free (src);
  g_free (dest);
}

static void
transfer_pixels (gdouble *src1,
                 gdouble *src2,
                 guchar  *dest,
                 gint     bytes,
                 gint     width)
{
  gint    b;
  gint    bend = bytes * width;
  gdouble sum;

  for (b = 0; b < bend; b++)
    {
      sum = *src1++ + *src2++;

      if (sum > 255)
        sum = 255;
      else if (sum < 0)
        sum = 0;

      *dest++ = (guchar) sum;
    }
}

static void
combine_to_gradient (guchar *dest,
                     guchar *src2,
                     gint    bytes,
                     gint    width,
                     gdouble amount)
{
  gint    b;
  gint    bend = bytes * width;
  gdouble h, v;
  gdouble sum;
  gdouble scale = (1.0 + 9.0 * amount);

  for (b = 0; b < bend; b++)
    {
      /* scale result */
      h = *src2++;
      v = *dest;

      sum = sqrt (h*h + v*v) * scale;

      if (sum > 255)
        sum = 255;
      else if (sum < 0)
        sum = 0;

      *dest++ = (guchar) sum;
    }
}

static void
find_constants (gdouble n_p[],
                gdouble n_m[],
                gdouble d_p[],
                gdouble d_m[],
                gdouble bd_p[],
                gdouble bd_m[],
                gdouble std_dev)
{
  gdouble a0, a1, b0, b1, c0, c1, w0, w1;
  gdouble w0n, w1n, cos0, cos1, sin0, sin1, b0n, b1n;
  gdouble div;

  /* coefficients for Gaussian 1st derivative filter */
  a0 =  0.6472;
  a1 =  4.531;
  b0 =  1.527;
  b1 =  1.516;
  c0 = -0.6494;
  c1 = -0.9557;
  w0 =  0.6719;
  w1 =  2.072;

  /* coefficients for Gaussian filter */
  /*
  a0 = 1.68;
  a1 = 3.735;
  b0 = 1.783;
  b1 = 1.723;
  c0 = -0.6803;
  c1 = -0.2598;
  w0 = 0.6318;
  w1 = 1.997;
  */

  /* coefficients for filter */
  w0n  = w0 / std_dev;
  w1n  = w1 / std_dev;
  cos0 = cos (w0n);
  cos1 = cos (w1n);
  sin0 = sin (w0n);
  sin1 = sin (w1n);
  b0n  = b0 / std_dev;
  b1n  = b1 / std_dev;

  div = sqrt (2 * G_PI) * std_dev;
  /*
  a0 = a0 / div;
  a1 = a1 / div;
  c0 = c0 / div;
  c1 = c1 / div;
  */

  n_p[4] = 0.0;
  n_p[3] = exp (-b1n - 2 * b0n) * (c1 * sin1 - cos1 * c0) + exp (-b0n - 2 * b1n) * (a1 * sin0 - cos0 * a0);
  n_p[2] = 2 * exp (-b0n - b1n) * ((a0 + c0) * cos1 * cos0 - cos1 * a1 * sin0 - cos0 * c1 * sin1) + c0 * exp (-2 * b0n) + a0 * exp (-2 * b1n);
  n_p[1] = exp (-b1n) * (c1 * sin1 - (c0 + 2 * a0) * cos1) + exp (-b0n) * (a1 * sin0 - (2 * c0 + a0) * cos0);
  n_p[0] = a0 + c0;

  d_p[4] = exp (-2 * b0n - 2 * b1n);
  d_p[3] = -2 * cos0 * exp (-b0n - 2 * b1n) - 2 * cos1 * exp (-b1n - 2 * b0n);
  d_p[2] = 4 * cos1 * cos0 * exp (-b0n - b1n) + exp (-2 * b1n) + exp (-2 * b0n);
  d_p[1] = -2 * exp (-b1n) * cos1 - 2 * exp (-b0n) * cos0;
  d_p[0] = 0.0;

  /* For first derivative */
  n_m[4] = d_p[4] * n_p[0] - n_p[4];
  n_m[3] = d_p[3] * n_p[0] - n_p[3];
  n_m[2] = d_p[2] * n_p[0] - n_p[2];
  n_m[1] = d_p[1] * n_p[0] - n_p[1];
  n_m[0] = 0.0;

  /* For gaussian operator */
  /*
  n_m[4] = n_p[4] - d_p[4] * n_p[0];
  n_m[3] = n_p[3] - d_p[3] * n_p[0];
  n_m[2] = n_p[2] - d_p[2] * n_p[0];
  n_m[1] = n_p[1] - d_p[1] * n_p[0];
  n_m[0] = 0.0;
  */

  d_m[4] = d_p[4];
  d_m[3] = d_p[3];
  d_m[2] = d_p[2];
  d_m[1] = d_p[1];
  d_m[0] = d_p[0];

  {
    gint    i;
    gdouble sum_n_p, sum_n_m, sum_d;
    gdouble a, b;

    sum_n_p = 0.0;
    sum_n_m = 0.0;
    sum_d   = 0.0;

    for (i = 0; i <= 4; i++)
      {
        sum_n_p += n_p[i];
        sum_n_m += n_m[i];
        sum_d   += d_p[i];
      }

    a = sum_n_p / (1 + sum_d);
    b = sum_n_m / (1 + sum_d);

    for (i = 0; i <= 4; i++)
      {
        bd_p[i] = d_p[i] * a;
        bd_m[i] = d_m[i] * b;
      }
  }
}

/*******************************************************/
/*                    Dialog                           */
/*******************************************************/

static gboolean
neon_dialog (GimpDrawable *drawable)
{
  GtkWidget *dialog;
  GtkWidget *main_vbox;
  GtkWidget *preview;
  GtkWidget *table;
  GtkObject *scale_data;
  gboolean   run;

  gimp_ui_init (PLUG_IN_BINARY, FALSE);

  dialog = gimp_dialog_new (_("Neon Detection"), PLUG_IN_BINARY,
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
                    G_CALLBACK (neon_preview_update),
                    NULL);

  table = gtk_table_new (2, 3, FALSE);
  gtk_table_set_col_spacings (GTK_TABLE (table), 6);
  gtk_table_set_row_spacings (GTK_TABLE (table), 6);
  gtk_box_pack_start (GTK_BOX (main_vbox), table, FALSE, FALSE, 0);
  gtk_widget_show (table);

  /*  Label, scale, entry for evals.radius  */
  scale_data = gimp_scale_entry_new (GTK_TABLE (table), 0, 0,
                                     _("_Radius:"), 100, 8,
                                     evals.radius, 0.0, 64.0, 1, 10, 2,
                                     FALSE, 0.0,
                                     8 * MAX (drawable->width, drawable->height),
                                     NULL, NULL);

  g_signal_connect (scale_data, "value-changed",
                    G_CALLBACK (gimp_double_adjustment_update),
                    &evals.radius);
  g_signal_connect_swapped (scale_data, "value-changed",
                            G_CALLBACK (gimp_preview_invalidate),
                            preview);

  /*  Label, scale, entry for evals.amount  */
  scale_data = gimp_scale_entry_new (GTK_TABLE (table), 0, 1,
                                     _("_Amount:"), 100, 8,
                                     evals.amount, 0.0, 1.0, 0.01, 0.1, 2,
                                     TRUE, 0, 0,
                                     NULL, NULL);

  g_signal_connect (scale_data, "value-changed",
                    G_CALLBACK (gimp_double_adjustment_update),
                    &evals.amount);
  g_signal_connect_swapped (scale_data, "value-changed",
                            G_CALLBACK (gimp_preview_invalidate),
                            preview);

  gtk_widget_show (dialog);

  run = (gimp_dialog_run (GIMP_DIALOG (dialog)) == GTK_RESPONSE_OK);

  gtk_widget_destroy (dialog);

  return run;
}

static void
neon_preview_update (GimpPreview *preview)
{
  neon (GIMP_DRAWABLE_PREVIEW (preview)->drawable,
        evals.radius,
        evals.amount,
        preview);
}
