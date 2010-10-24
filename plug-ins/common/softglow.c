/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
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

/* Softglow filter for GIMP for BIPS
 *  -Spencer Kimball
 *
 * This filter screens a desaturated, sigmoidally transferred
 *  and gaussian blurred version of the drawable over itself
 *  to create a "softglow" photographic effect.
 */

#include "config.h"

#include <string.h>

#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

#include "libgimp/stdplugins-intl.h"


/* Some useful macros */

#define PLUG_IN_PROC    "plug-in-softglow"
#define PLUG_IN_BINARY  "softglow"
#define PLUG_IN_ROLE    "gimp-softglow"
#define TILE_CACHE_SIZE 48
#define SIGMOIDAL_BASE   2
#define SIGMOIDAL_RANGE 20

#define INT_MULT(a,b,t)  ((t) = (a) * (b) + 0x80, ((((t) >> 8) + (t)) >> 8))

typedef struct
{
  gdouble glow_radius;
  gdouble brightness;
  gdouble sharpness;
} SoftglowVals;


/*
 * Function prototypes.
 */

static void      query             (void);
static void      run               (const gchar      *name,
                                    gint              nparams,
                                    const GimpParam  *param,
                                    gint             *nreturn_vals,
                                    GimpParam       **return_vals);

static void      softglow          (GimpDrawable     *drawable,
                                    GimpPreview      *preview);
static gboolean  softglow_dialog   (GimpDrawable     *drawable);

/*
 * Gaussian blur helper functions
 */
static void      find_constants    (gdouble  n_p[],
                                    gdouble  n_m[],
                                    gdouble  d_p[],
                                    gdouble  d_m[],
                                    gdouble  bd_p[],
                                    gdouble  bd_m[],
                                    gdouble  std_dev);
static void      transfer_pixels   (gdouble *src1,
                                    gdouble *src2,
                                    guchar  *dest,
                                    gint     jump,
                                    gint     width);

/***** Local vars *****/

const GimpPlugInInfo PLUG_IN_INFO =
{
  NULL,  /* init  */
  NULL,  /* quit  */
  query, /* query */
  run,   /* run   */
};

static SoftglowVals svals =
{
  10.0, /* glow_radius */
  0.75, /* brightness */
  0.85,  /* sharpness */
};


/***** Functions *****/

MAIN ()

static void
query (void)
{
  static const GimpParamDef args[] =
  {
    { GIMP_PDB_INT32,    "run-mode",    "The run mode { RUN-INTERACTIVE (0), RUN-NONINTERACTIVE (1) }"   },
    { GIMP_PDB_IMAGE,    "image",       "Input image (unused)"           },
    { GIMP_PDB_DRAWABLE, "drawable",    "Input drawable"                 },
    { GIMP_PDB_FLOAT,    "glow-radius", "Glow radius (radius in pixels)" },
    { GIMP_PDB_FLOAT,    "brightness",  "Glow brightness (0.0 - 1.0)"    },
    { GIMP_PDB_FLOAT,    "sharpness",   "Glow sharpness (0.0 - 1.0)"     }
  };

  gchar *help_string =
    "Gives an image a softglow effect by intensifying the highlights in the "
    "image. This is done by screening a modified version of the drawable "
    "with itself. The modified version is desaturated and then a sigmoidal "
    "transfer function is applied to force the distribution of intensities "
    "into very small and very large only. This desaturated version is then "
    "blurred to give it a fuzzy 'vaseline-on-the-lens' effect. The glow "
    "radius parameter controls the sharpness of the glow effect. The "
    "brightness parameter controls the degree of intensification applied "
    "to image highlights. The sharpness parameter controls how defined or "
    "alternatively, diffuse, the glow effect should be.";

  gimp_install_procedure (PLUG_IN_PROC,
                          N_("Simulate glow by making highlights intense and fuzzy"),
                          help_string,
                          "Spencer Kimball",
                          "Bit Specialists, Inc.",
                          "2001",
                          N_("_Softglow..."),
                          "RGB*, GRAY*",
                          GIMP_PLUGIN,
                          G_N_ELEMENTS (args), 0,
                          args, NULL);

  gimp_plugin_menu_register (PLUG_IN_PROC, "<Image>/Filters/Artistic");
}

static void
run (const gchar      *name,
     gint              nparams,
     const GimpParam  *param,
     gint             *nreturn_vals,
     GimpParam       **return_vals)
{
  static GimpParam   values[2];
  GimpRunMode        run_mode;
  GimpDrawable      *drawable;
  GimpPDBStatusType  status = GIMP_PDB_SUCCESS;

  run_mode = param[0].data.d_int32;

  /*  Get the specified drawable  */
  drawable = gimp_drawable_get (param[2].data.d_drawable);

  /*  set the tile cache size  */
  gimp_tile_cache_ntiles (TILE_CACHE_SIZE);

  *nreturn_vals = 1;
  *return_vals  = values;

  values[0].type          = GIMP_PDB_STATUS;
  values[0].data.d_status = status;

  INIT_I18N();

  switch (run_mode)
    {
    case GIMP_RUN_INTERACTIVE:
      /*  Possibly retrieve data  */
      gimp_get_data (PLUG_IN_PROC, &svals);

      /*  First acquire information with a dialog  */
      if (! softglow_dialog (drawable))
        return;
      break;

    case GIMP_RUN_NONINTERACTIVE:
      svals.glow_radius = param[3].data.d_float;
      svals.brightness  = param[4].data.d_float;
      svals.sharpness   = param[5].data.d_float;
      break;

    case GIMP_RUN_WITH_LAST_VALS:
      /*  Possibly retrieve data  */
      gimp_get_data (PLUG_IN_PROC, &svals);
      break;

    default:
      break;
    }

  if (status == GIMP_PDB_SUCCESS)
    {
      /*  Make sure that the drawable is RGB or GRAY color  */
      if (gimp_drawable_is_rgb (drawable->drawable_id) ||
          gimp_drawable_is_gray (drawable->drawable_id))
        {
          gimp_progress_init ("Softglow");

          softglow (drawable, NULL);

          if (run_mode != GIMP_RUN_NONINTERACTIVE)
            gimp_displays_flush ();

          /*  Store data  */
          if (run_mode == GIMP_RUN_INTERACTIVE)
            gimp_set_data (PLUG_IN_PROC, &svals, sizeof (SoftglowVals));
        }
      else
        {
          status        = GIMP_PDB_EXECUTION_ERROR;
          *nreturn_vals = 2;
          values[1].type          = GIMP_PDB_STRING;
          values[1].data.d_string = _("Cannot operate on indexed color images.");
        }
    }

  values[0].data.d_status = status;

  gimp_drawable_detach (drawable);
}

static void
softglow (GimpDrawable *drawable,
          GimpPreview  *preview)
{
  GimpPixelRgn  src_rgn, dest_rgn;
  GimpPixelRgn *pr;
  gint          width, height;
  gint          bytes;
  gboolean      has_alpha;
  guchar       *dest;
  guchar       *src, *sp_p, *sp_m;
  gdouble       n_p[5], n_m[5];
  gdouble       d_p[5], d_m[5];
  gdouble       bd_p[5], bd_m[5];
  gdouble      *val_p, *val_m, *vp, *vm;
  gint          x1, y1;
  gint          i, j;
  gint          row, col, b;
  gint          terms;
  gint          progress, max_progress;
  gint          initial_p[4];
  gint          initial_m[4];
  gint          tmp;
  gdouble       radius;
  gdouble       std_dev;
  gdouble       val;

  if (preview)
    {
      gimp_preview_get_position (preview, &x1, &y1);
      gimp_preview_get_size (preview, &width, &height);
    }
  else if (! gimp_drawable_mask_intersect (drawable->drawable_id,
                                           &x1, &y1, &width, &height))
    return;

  bytes     = drawable->bpp;
  has_alpha = gimp_drawable_has_alpha (drawable->drawable_id);

  val_p = g_new (gdouble, MAX (width, height));
  val_m = g_new (gdouble, MAX (width, height));

  dest = g_new0 (guchar, width * height);

  progress = 0;
  max_progress = width * height * 3;

  /* Initialize the pixel regions. */
  gimp_pixel_rgn_init (&src_rgn, drawable, x1, y1, width, height, FALSE, FALSE);

  for (pr = gimp_pixel_rgns_register (1, &src_rgn);
       pr != NULL;
       pr = gimp_pixel_rgns_process (pr))
    {
      guchar *src_ptr  = src_rgn.data;
      guchar *dest_ptr = dest + (src_rgn.y - y1) * width + (src_rgn.x - x1);

      for (row = 0; row < src_rgn.h; row++)
        {
          for (col = 0; col < src_rgn.w; col++)
            {
              /* desaturate */
              if (bytes > 2)
                dest_ptr[col] = (guchar) gimp_rgb_to_l_int (src_ptr[col * bytes + 0],
                                                            src_ptr[col * bytes + 1],
                                                            src_ptr[col * bytes + 2]);
              else
                dest_ptr[col] = (guchar) src_ptr[col * bytes];

              /* compute sigmoidal transfer */
              val = dest_ptr[col] / 255.0;
              val = 255.0 / (1 + exp (-(SIGMOIDAL_BASE + (svals.sharpness * SIGMOIDAL_RANGE)) * (val - 0.5)));
              val = val * svals.brightness;
              dest_ptr[col] = (guchar) CLAMP (val, 0, 255);
            }

          src_ptr  += src_rgn.rowstride;
          dest_ptr += width;
        }

      if (!preview)
        {
          progress += src_rgn.w * src_rgn.h;
          gimp_progress_update ((gdouble) progress / (gdouble) max_progress);
        }
    }

  /*  Calculate the standard deviations  */
  radius  = fabs (svals.glow_radius) + 1.0;
  std_dev = sqrt (-(radius * radius) / (2 * log (1.0 / 255.0)));

  /*  derive the constants for calculating the gaussian from the std dev  */
  find_constants (n_p, n_m, d_p, d_m, bd_p, bd_m, std_dev);

  /*  First the vertical pass  */
  for (col = 0; col < width; col++)
    {
      memset (val_p, 0, height * sizeof (gdouble));
      memset (val_m, 0, height * sizeof (gdouble));

      src  = dest + col;
      sp_p = src;
      sp_m = src + width * (height - 1);
      vp   = val_p;
      vm   = val_m + (height - 1);

      /*  Set up the first vals  */
      initial_p[0] = sp_p[0];
      initial_m[0] = sp_m[0];

      for (row = 0; row < height; row++)
        {
          gdouble *vpptr, *vmptr;

          terms = (row < 4) ? row : 4;

          vpptr = vp; vmptr = vm;
          for (i = 0; i <= terms; i++)
            {
              *vpptr += n_p[i] * sp_p[-i * width] - d_p[i] * vp[-i];
              *vmptr += n_m[i] * sp_m[i * width] - d_m[i] * vm[i];
            }
          for (j = i; j <= 4; j++)
            {
              *vpptr += (n_p[j] - bd_p[j]) * initial_p[0];
              *vmptr += (n_m[j] - bd_m[j]) * initial_m[0];
            }

          sp_p += width;
          sp_m -= width;
          vp ++;
          vm --;
        }

      transfer_pixels (val_p, val_m, dest + col, width, height);

      if (!preview)
        {
          progress += height;
          if ((col % 5) == 0)
            gimp_progress_update ((gdouble) progress / (gdouble) max_progress);
        }
    }

  for (row = 0; row < height; row++)
    {
      memset (val_p, 0, width * sizeof (gdouble));
      memset (val_m, 0, width * sizeof (gdouble));

      src = dest + row * width;

      sp_p = src;
      sp_m = src + width - 1;
      vp = val_p;
      vm = val_m + width - 1;

      /*  Set up the first vals  */
      initial_p[0] = sp_p[0];
      initial_m[0] = sp_m[0];

      for (col = 0; col < width; col++)
        {
          gdouble *vpptr, *vmptr;

          terms = (col < 4) ? col : 4;

          vpptr = vp; vmptr = vm;

          for (i = 0; i <= terms; i++)
            {
              *vpptr += n_p[i] * sp_p[-i] - d_p[i] * vp[-i];
              *vmptr += n_m[i] * sp_m[i] - d_m[i] * vm[i];
            }

          for (j = i; j <= 4; j++)
            {
              *vpptr += (n_p[j] - bd_p[j]) * initial_p[0];
              *vmptr += (n_m[j] - bd_m[j]) * initial_m[0];
            }

          sp_p ++;
          sp_m --;
          vp ++;
          vm --;
        }

      transfer_pixels (val_p, val_m, dest + row * width, 1, width);

      if (!preview)
        {
          progress += width;
          if ((row % 5) == 0)
            gimp_progress_update ((gdouble) progress / (gdouble) max_progress);
        }
    }

  /* Initialize the pixel regions. */
  gimp_pixel_rgn_init (&src_rgn, drawable, x1, y1, width, height, FALSE, FALSE);
  gimp_pixel_rgn_init (&dest_rgn, drawable,
                       x1, y1, width, height, (preview == NULL), TRUE);

  for (pr = gimp_pixel_rgns_register (2, &src_rgn, &dest_rgn);
       pr != NULL;
       pr = gimp_pixel_rgns_process (pr))
    {
      guchar *src_ptr  = src_rgn.data;
      guchar *dest_ptr = dest_rgn.data;
      guchar *blur_ptr = dest + (src_rgn.y - y1) * width + (src_rgn.x - x1);

      for (row = 0; row < src_rgn.h; row++)
        {
          for (col = 0; col < src_rgn.w; col++)
            {
              /* screen op */
              for (b = 0; b < (has_alpha ? (bytes - 1) : bytes); b++)
                dest_ptr[col * bytes + b] =
                  255 - INT_MULT((255 - src_ptr[col * bytes + b]),
                                 (255 - blur_ptr[col]), tmp);
              if (has_alpha)
                dest_ptr[col * bytes + b] = src_ptr[col * bytes + b];
            }

          src_ptr  += src_rgn.rowstride;
          dest_ptr += dest_rgn.rowstride;
          blur_ptr += width;
        }

      if (preview)
        {
          gimp_drawable_preview_draw_region (GIMP_DRAWABLE_PREVIEW (preview),
                                             &dest_rgn);
        }
      else
        {
          progress += src_rgn.w * src_rgn.h;
          gimp_progress_update ((gdouble) progress / (gdouble) max_progress);
        }
    }

  if (! preview)
    {
      gimp_progress_update (1.0);
      /*  merge the shadow, update the drawable  */
      gimp_drawable_flush (drawable);
      gimp_drawable_merge_shadow (drawable->drawable_id, TRUE);
      gimp_drawable_update (drawable->drawable_id,
                            x1, y1, width, height);
    }

  /*  free up buffers  */
  g_free (val_p);
  g_free (val_m);
  g_free (dest);
}

/*
 *  Gaussian blur helper functions
 */

static void
transfer_pixels (gdouble *src1,
                 gdouble *src2,
                 guchar  *dest,
                 gint     jump,
                 gint     width)
{
  gint    i;
  gdouble sum;

  for (i = 0; i < width; i++)
    {
      sum = src1[i] + src2[i];

      sum = CLAMP0255 (sum);

      *dest = (guchar) sum;
      dest += jump;
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
  gint    i;
  gdouble constants [8];
  gdouble div;

  /*  The constants used in the implemenation of a casual sequence
   *  using a 4th order approximation of the gaussian operator
   */

  div = sqrt(2 * G_PI) * std_dev;

  constants [0] = -1.783  / std_dev;
  constants [1] = -1.723  / std_dev;
  constants [2] =  0.6318 / std_dev;
  constants [3] =  1.997  / std_dev;
  constants [4] =  1.6803 / div;
  constants [5] =  3.735  / div;
  constants [6] = -0.6803 / div;
  constants [7] = -0.2598 / div;

  n_p [0] = constants[4] + constants[6];
  n_p [1] = exp (constants[1]) *
    (constants[7] * sin (constants[3]) -
     (constants[6] + 2 * constants[4]) * cos (constants[3])) +
       exp (constants[0]) *
         (constants[5] * sin (constants[2]) -
          (2 * constants[6] + constants[4]) * cos (constants[2]));
  n_p [2] = 2 * exp (constants[0] + constants[1]) *
    ((constants[4] + constants[6]) * cos (constants[3]) * cos (constants[2]) -
     constants[5] * cos (constants[3]) * sin (constants[2]) -
     constants[7] * cos (constants[2]) * sin (constants[3])) +
       constants[6] * exp (2 * constants[0]) +
         constants[4] * exp (2 * constants[1]);
  n_p [3] = exp (constants[1] + 2 * constants[0]) *
    (constants[7] * sin (constants[3]) - constants[6] * cos (constants[3])) +
      exp (constants[0] + 2 * constants[1]) *
        (constants[5] * sin (constants[2]) - constants[4] * cos (constants[2]));
  n_p [4] = 0.0;

  d_p [0] = 0.0;
  d_p [1] = -2 * exp (constants[1]) * cos (constants[3]) -
    2 * exp (constants[0]) * cos (constants[2]);
  d_p [2] = 4 * cos (constants[3]) * cos (constants[2]) * exp (constants[0] + constants[1]) +
    exp (2 * constants[1]) + exp (2 * constants[0]);
  d_p [3] = -2 * cos (constants[2]) * exp (constants[0] + 2 * constants[1]) -
    2 * cos (constants[3]) * exp (constants[1] + 2 * constants[0]);
  d_p [4] = exp (2 * constants[0] + 2 * constants[1]);

#ifndef ORIGINAL_READABLE_CODE
  memcpy(d_m, d_p, 5 * sizeof(gdouble));
#else
  for (i = 0; i <= 4; i++)
    d_m [i] = d_p [i];
#endif

  n_m[0] = 0.0;
  for (i = 1; i <= 4; i++)
    n_m [i] = n_p[i] - d_p[i] * n_p[0];

  {
    gdouble sum_n_p, sum_n_m, sum_d;
    gdouble a, b;

    sum_n_p = 0.0;
    sum_n_m = 0.0;
    sum_d   = 0.0;

    for (i = 0; i <= 4; i++)
      {
        sum_n_p += n_p[i];
        sum_n_m += n_m[i];
        sum_d += d_p[i];
      }

#ifndef ORIGINAL_READABLE_CODE
    sum_d++;
    a = sum_n_p / sum_d;
    b = sum_n_m / sum_d;
#else
    a = sum_n_p / (1 + sum_d);
    b = sum_n_m / (1 + sum_d);
#endif

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
softglow_dialog (GimpDrawable *drawable)
{
  GtkWidget     *dialog;
  GtkWidget     *main_vbox;
  GtkWidget     *preview;
  GtkWidget     *table;
  GtkAdjustment *scale_data;
  gboolean       run;

  gimp_ui_init (PLUG_IN_BINARY, FALSE);

  dialog = gimp_dialog_new (_("Softglow"), PLUG_IN_ROLE,
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

  g_signal_connect_swapped (preview, "invalidated",
                            G_CALLBACK (softglow),
                            drawable);

  table = gtk_table_new (3, 3, FALSE);
  gtk_table_set_col_spacings (GTK_TABLE (table), 6);
  gtk_table_set_row_spacings (GTK_TABLE (table), 6);
  gtk_box_pack_start (GTK_BOX (main_vbox), table, FALSE, FALSE, 0);
  gtk_widget_show (table);

  /*  Label, scale, entry for svals.amount  */
  scale_data = gimp_scale_entry_new (GTK_TABLE (table), 0, 0,
                                     _("_Glow radius:"), 100, 5,
                                     svals.glow_radius, 1.0, 50.0, 1, 5.0, 2,
                                     TRUE, 0, 0,
                                     NULL, NULL);

  g_signal_connect (scale_data, "value-changed",
                    G_CALLBACK (gimp_double_adjustment_update),
                    &svals.glow_radius);
  g_signal_connect_swapped (scale_data, "value-changed",
                            G_CALLBACK (gimp_preview_invalidate),
                            preview);

  /*  Label, scale, entry for svals.amount  */
  scale_data = gimp_scale_entry_new (GTK_TABLE (table), 0, 1,
                                     _("_Brightness:"), 100, 5,
                                     svals.brightness, 0.0, 1.0, 0.01, 0.1, 2,
                                     TRUE, 0, 0,
                                     NULL, NULL);

  g_signal_connect (scale_data, "value-changed",
                    G_CALLBACK (gimp_double_adjustment_update),
                    &svals.brightness);
  g_signal_connect_swapped (scale_data, "value-changed",
                            G_CALLBACK (gimp_preview_invalidate),
                            preview);

  /*  Label, scale, entry for svals.amount  */
  scale_data = gimp_scale_entry_new (GTK_TABLE (table), 0, 2,
                                     _("_Sharpness:"), 100, 5,
                                     svals.sharpness, 0.0, 1.0, 0.01, 0.1, 2,
                                     TRUE, 0, 0,
                                     NULL, NULL);

  g_signal_connect (scale_data, "value-changed",
                    G_CALLBACK (gimp_double_adjustment_update),
                    &svals.sharpness);
  g_signal_connect_swapped (scale_data, "value-changed",
                            G_CALLBACK (gimp_preview_invalidate),
                            preview);

  gtk_widget_show (dialog);

  run = (gimp_dialog_run (GIMP_DIALOG (dialog)) == GTK_RESPONSE_OK);

  gtk_widget_destroy (dialog);

  return run;
}
