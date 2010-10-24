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

/* Photocopy filter for GIMP for BIPS
 *  -Spencer Kimball
 *
 * This filter propagates dark values in an image based on
 *  each pixel's relative darkness to a neighboring average
 *  and sets the remaining pixels to white.
 */

#include "config.h"

#include <string.h>

#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

#include "libgimp/stdplugins-intl.h"


/* Some useful macros */

#define PLUG_IN_PROC    "plug-in-photocopy"
#define PLUG_IN_BINARY  "photocopy"
#define PLUG_IN_ROLE    "gimp-photocopy"
#define TILE_CACHE_SIZE 48
#define GAMMA           1.0
#define EPSILON         2


typedef struct
{
  gdouble  mask_radius;
  gdouble  sharpness;
  gdouble  threshold;
  gdouble  pct_black;
  gdouble  pct_white;
} PhotocopyVals;


/*
 * Function prototypes.
 */

static void      query  (void);
static void      run    (const gchar       *name,
                         gint               nparams,
                         const GimpParam   *param,
                         gint              *nreturn_vals,
                         GimpParam        **return_vals);

static void      photocopy        (GimpDrawable *drawable,
                                   GimpPreview  *preview);
static gboolean  photocopy_dialog (GimpDrawable *drawable);

static gdouble   compute_ramp     (guchar       *dest1,
                                   guchar       *dest2,
                                   gint          length,
                                   gdouble       pct_black,
                                   gint          under_threshold);

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

static PhotocopyVals pvals =
{
  8.0,  /* mask_radius */
  0.8,  /* sharpness */
  0.75, /* threshold */
  0.2,  /* pct_black */
  0.2   /* pct_white */
};


/***** Functions *****/

MAIN ()

static void
query (void)
{
  static const GimpParamDef args[] =
  {
    { GIMP_PDB_INT32,    "run-mode",    "The run mode { RUN-INTERACTIVE (0), RUN-NONINTERACTIVE (1) }" },
    { GIMP_PDB_IMAGE,    "image",       "Input image (unused)" },
    { GIMP_PDB_DRAWABLE, "drawable",    "Input drawable" },
    { GIMP_PDB_FLOAT,    "mask-radius", "Photocopy mask radius (radius of pixel neighborhood)" },
    { GIMP_PDB_FLOAT,    "sharpness",   "Sharpness (detail level) (0.0 - 1.0)" },
    { GIMP_PDB_FLOAT,    "pct-black",   "Percentage of darkened pixels to set to black (0.0 - 1.0)" },
    { GIMP_PDB_FLOAT,    "pct-white",   "Percentage of non-darkened pixels left white (0.0 - 1.0)" }
  };

  gchar *help_string =
    "Propagates dark values in an image based on "
    "each pixel's relative darkness to a neighboring average. The idea behind "
    "this filter is to give the look of a photocopied version of the image, "
    "with toner transferred based on the relative darkness of a particular "
    "region. This is achieved by darkening areas of the image which are "
    "measured to be darker than a neighborhood average and setting other "
    "pixels to white. In this way, sufficiently large shifts in intensity "
    "are darkened to black. The rate at which they are darkened to black is "
    "determined by the second pct_black parameter. The mask_radius parameter "
    "controls the size of the pixel neighborhood over which the average "
    "intensity is computed and then compared to each pixel in the neighborhood "
    "to decide whether or not to darken it to black. Large values for "
    "mask_radius result in very thick black areas bordering the regions "
    "of white and much less detail for black areas everywhere including "
    "inside regions of color. Small values result in less toner overall "
    "and more detail everywhere. Small values for the pct_black make the "
    "blend from the white regions to the black border lines smoother and "
    "the toner regions themselves thinner and less noticeable; larger values "
    "achieve the opposite effect.";

  gimp_install_procedure (PLUG_IN_PROC,
                          N_("Simulate color distortion produced by a copy machine"),
                          help_string,
                          "Spencer Kimball",
                          "Bit Specialists, Inc.",
                          "2001",
                          N_("_Photocopy..."),
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
      gimp_get_data (PLUG_IN_PROC, &pvals);

      /*  First acquire information with a dialog  */
      if (! photocopy_dialog (drawable))
        return;
      break;

    case GIMP_RUN_NONINTERACTIVE:
      pvals.mask_radius = param[3].data.d_float;
      pvals.sharpness   = param[4].data.d_float;
      pvals.pct_black   = param[5].data.d_float;
      pvals.pct_white   = param[6].data.d_float;
      break;

    case GIMP_RUN_WITH_LAST_VALS:
      /*  Possibly retrieve data  */
      gimp_get_data (PLUG_IN_PROC, &pvals);
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
          gimp_progress_init ("Photocopy");

          photocopy (drawable, NULL);

          if (run_mode != GIMP_RUN_NONINTERACTIVE)
            gimp_displays_flush ();

          /*  Store data  */
          if (run_mode == GIMP_RUN_INTERACTIVE)
            gimp_set_data (PLUG_IN_PROC, &pvals, sizeof (PhotocopyVals));
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

/*
 * Photocopy algorithm
 * -----------------
 * Mask radius = radius of pixel neighborhood for intensity comparison
 * Threshold   = relative intensity difference which will result in darkening
 * Ramp        = amount of relative intensity difference before total black
 * Blur radius = mask radius / 3.0
 *
 * Algorithm:
 * For each pixel, calculate pixel intensity value to be: avg (blur radius)
 * relative diff = pixel intensity / avg (mask radius)
 * If relative diff < Threshold
 *   intensity mult = (Ramp - MIN (Ramp, (Threshold - relative diff))) / Ramp
 *   pixel intensity *= intensity mult
 * Else
 *   pixel intensity = white
 */
static void
photocopy (GimpDrawable *drawable,
           GimpPreview  *preview)
{
  GimpPixelRgn  src_rgn, dest_rgn;
  GimpPixelRgn *pr;
  gint          x, y, width, height;
  gint          bytes;
  gboolean      has_alpha;
  guchar       *dest1;
  guchar       *dest2;
  guchar       *src1, *sp_p1, *sp_m1;
  guchar       *src2, *sp_p2, *sp_m2;
  gdouble       n_p1[5], n_m1[5];
  gdouble       n_p2[5], n_m2[5];
  gdouble       d_p1[5], d_m1[5];
  gdouble       d_p2[5], d_m2[5];
  gdouble       bd_p1[5], bd_m1[5];
  gdouble       bd_p2[5], bd_m2[5];
  gdouble      *val_p1, *val_m1, *vp1, *vm1;
  gdouble      *val_p2, *val_m2, *vp2, *vm2;
  gint          i, j;
  gint          row, col;
  gint          terms;
  gint          progress, max_progress;
  gint          initial_p1[4];
  gint          initial_p2[4];
  gint          initial_m1[4];
  gint          initial_m2[4];
  gdouble       radius;
  gdouble       val;
  gdouble       std_dev1;
  gdouble       std_dev2;
  gdouble       ramp_down;
  gdouble       ramp_up;

  if (preview)
    {
      gimp_preview_get_position (preview, &x, &y);
      gimp_preview_get_size (preview, &width, &height);
    }
  else
    {
      if (! gimp_drawable_mask_intersect (drawable->drawable_id,
                                          &x, &y, &width, &height))
        return;
    }

  bytes     = drawable->bpp;
  has_alpha = gimp_drawable_has_alpha (drawable->drawable_id);

  val_p1 = g_new (gdouble, MAX (width, height));
  val_p2 = g_new (gdouble, MAX (width, height));
  val_m1 = g_new (gdouble, MAX (width, height));
  val_m2 = g_new (gdouble, MAX (width, height));

  dest1 = g_new0 (guchar, width * height);
  dest2 = g_new0 (guchar, width * height);

  progress = 0;
  max_progress = width * height * 3;

  gimp_pixel_rgn_init (&src_rgn, drawable,
                       x, y, width, height, FALSE, FALSE);

  for (pr = gimp_pixel_rgns_register (1, &src_rgn);
       pr != NULL;
       pr = gimp_pixel_rgns_process (pr))
    {
      guchar *src_ptr  = src_rgn.data;
      guchar *dest_ptr = dest1 + (src_rgn.y - y) * width + (src_rgn.x - x);

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

              /* compute  transfer */
              val = pow (dest_ptr[col], (1.0 / GAMMA));
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
  radius   = MAX (1.0, 10 * (1.0 - pvals.sharpness));
  radius   = fabs (radius) + 1.0;
  std_dev1 = sqrt (-(radius * radius) / (2 * log (1.0 / 255.0)));

  radius   = fabs (pvals.mask_radius) + 1.0;
  std_dev2 = sqrt (-(radius * radius) / (2 * log (1.0 / 255.0)));

  /*  derive the constants for calculating the gaussian from the std dev  */
  find_constants (n_p1, n_m1, d_p1, d_m1, bd_p1, bd_m1, std_dev1);
  find_constants (n_p2, n_m2, d_p2, d_m2, bd_p2, bd_m2, std_dev2);

  /*  First the vertical pass  */
  for (col = 0; col < width; col++)
    {
      memset (val_p1, 0, height * sizeof (gdouble));
      memset (val_p2, 0, height * sizeof (gdouble));
      memset (val_m1, 0, height * sizeof (gdouble));
      memset (val_m2, 0, height * sizeof (gdouble));

      src1  = dest1 + col;
      sp_p1 = src1;
      sp_m1 = src1 + (height - 1) * width;
      vp1   = val_p1;
      vp2   = val_p2;
      vm1   = val_m1 + (height - 1);
      vm2   = val_m2 + (height - 1);

      /*  Set up the first vals  */
      initial_p1[0] = sp_p1[0];
      initial_m1[0] = sp_m1[0];

      for (row = 0; row < height; row++)
        {
          gdouble *vpptr1, *vmptr1;
          gdouble *vpptr2, *vmptr2;

          terms = (row < 4) ? row : 4;

          vpptr1 = vp1; vmptr1 = vm1;
          vpptr2 = vp2; vmptr2 = vm2;

          for (i = 0; i <= terms; i++)
            {
              *vpptr1 += n_p1[i] * sp_p1[-i * width] - d_p1[i] * vp1[-i];
              *vmptr1 += n_m1[i] * sp_m1[i * width] - d_m1[i] * vm1[i];

              *vpptr2 += n_p2[i] * sp_p1[-i * width] - d_p2[i] * vp2[-i];
              *vmptr2 += n_m2[i] * sp_m1[i * width] - d_m2[i] * vm2[i];
            }

          for (j = i; j <= 4; j++)
            {
              *vpptr1 += (n_p1[j] - bd_p1[j]) * initial_p1[0];
              *vmptr1 += (n_m1[j] - bd_m1[j]) * initial_m1[0];

              *vpptr2 += (n_p2[j] - bd_p2[j]) * initial_p1[0];
              *vmptr2 += (n_m2[j] - bd_m2[j]) * initial_m1[0];
            }

          sp_p1 += width;
          sp_m1 -= width;
          vp1   += 1;
          vp2   += 1;
          vm1   -= 1;
          vm2   -= 1;
        }

      transfer_pixels (val_p1, val_m1, dest1 + col, width, height);
      transfer_pixels (val_p2, val_m2, dest2 + col, width, height);

      if (!preview)
        {
          progress += height;
          if ((col % 5) == 0)
            gimp_progress_update ((gdouble) progress / (gdouble) max_progress);
        }
    }

  for (row = 0; row < height; row++)
    {
      memset (val_p1, 0, width * sizeof (gdouble));
      memset (val_p2, 0, width * sizeof (gdouble));
      memset (val_m1, 0, width * sizeof (gdouble));
      memset (val_m2, 0, width * sizeof (gdouble));

      src1 = dest1 + row * width;
      src2 = dest2 + row * width;

      sp_p1 = src1;
      sp_p2 = src2;
      sp_m1 = src1 + width - 1;
      sp_m2 = src2 + width - 1;
      vp1   = val_p1;
      vp2   = val_p2;
      vm1   = val_m1 + width - 1;
      vm2   = val_m2 + width - 1;

      /*  Set up the first vals  */
      initial_p1[0] = sp_p1[0];
      initial_p2[0] = sp_p2[0];
      initial_m1[0] = sp_m1[0];
      initial_m2[0] = sp_m2[0];

      for (col = 0; col < width; col++)
        {
          gdouble *vpptr1, *vmptr1;
          gdouble *vpptr2, *vmptr2;

          terms = (col < 4) ? col : 4;

          vpptr1 = vp1; vmptr1 = vm1;
          vpptr2 = vp2; vmptr2 = vm2;

          for (i = 0; i <= terms; i++)
            {
              *vpptr1 += n_p1[i] * sp_p1[-i] - d_p1[i] * vp1[-i];
              *vmptr1 += n_m1[i] * sp_m1[i] - d_m1[i] * vm1[i];

              *vpptr2 += n_p2[i] * sp_p2[-i] - d_p2[i] * vp2[-i];
              *vmptr2 += n_m2[i] * sp_m2[i] - d_m2[i] * vm2[i];
            }

          for (j = i; j <= 4; j++)
            {
              *vpptr1 += (n_p1[j] - bd_p1[j]) * initial_p1[0];
              *vmptr1 += (n_m1[j] - bd_m1[j]) * initial_m1[0];

              *vpptr2 += (n_p2[j] - bd_p2[j]) * initial_p2[0];
              *vmptr2 += (n_m2[j] - bd_m2[j]) * initial_m2[0];
            }

          sp_p1 ++;
          sp_p2 ++;
          sp_m1 --;
          sp_m2 --;
          vp1 ++;
          vp2 ++;
          vm1 --;
          vm2 --;
        }

      transfer_pixels (val_p1, val_m1, dest1 + row * width, 1, width);
      transfer_pixels (val_p2, val_m2, dest2 + row * width, 1, width);

      if (!preview)
        {
          progress += width;
          if ((row % 5) == 0)
            gimp_progress_update ((gdouble) progress / (gdouble) max_progress);
        }
    }

  /* Compute the ramp value which sets 'pct_black' % of the darkened pixels black */
  ramp_down = compute_ramp (dest1, dest2, width * height, pvals.pct_black, 1);
  ramp_up   = compute_ramp (dest1, dest2, width * height, 1.0 - pvals.pct_white, 0);

  /* Initialize the pixel regions. */
  gimp_pixel_rgn_init (&src_rgn, drawable, x, y, width, height, FALSE, FALSE);
  gimp_pixel_rgn_init (&dest_rgn, drawable, x, y, width, height,
                       (preview == NULL), TRUE);

  pr = gimp_pixel_rgns_register (2, &src_rgn, &dest_rgn);

  while (pr)
    {
      guchar  *src_ptr  = src_rgn.data;
      guchar  *dest_ptr = dest_rgn.data;
      guchar  *blur_ptr = dest1 + (src_rgn.y - y) * width + (src_rgn.x - x);
      guchar  *avg_ptr  = dest2 + (src_rgn.y - y) * width + (src_rgn.x - x);
      gdouble  diff, mult;
      gdouble  lightness = 0.0;

      for (row = 0; row < src_rgn.h; row++)
        {
          for (col = 0; col < src_rgn.w; col++)
            {
              if (avg_ptr[col] > EPSILON)
                {
                  diff = (gdouble) blur_ptr[col] / (gdouble) avg_ptr[col];

                  if (diff < pvals.threshold)
                    {
                      if (ramp_down == 0.0)
                        mult = 0.0;
                      else
                        mult = (ramp_down - MIN (ramp_down,
                                                 (pvals.threshold - diff))) / ramp_down;
                      lightness = CLAMP (blur_ptr[col] * mult, 0, 255);
                    }
                  else
                    {
                      if (ramp_up == 0.0)
                        mult = 1.0;
                      else
                        mult = MIN (ramp_up,
                                    (diff - pvals.threshold)) / ramp_up;

                      lightness = 255 - (1.0 - mult) * (255 - blur_ptr[col]);
                      lightness = CLAMP (lightness, 0, 255);
                    }
                }
              else
                {
                  lightness = 0;
                }

              if (bytes < 3)
                {
                  dest_ptr[col * bytes] = (guchar) lightness;
                  if (has_alpha)
                    dest_ptr[col * bytes + 1] = src_ptr[col * src_rgn.bpp + 1];
                }
              else
                {
                  dest_ptr[col * bytes + 0] = lightness;
                  dest_ptr[col * bytes + 1] = lightness;
                  dest_ptr[col * bytes + 2] = lightness;

                  if (has_alpha)
                    dest_ptr[col * bytes + 3] = src_ptr[col * src_rgn.bpp + 3];
                }
            }

          src_ptr  += src_rgn.rowstride;
          dest_ptr += dest_rgn.rowstride;
          blur_ptr += width;
          avg_ptr  += width;
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

      pr = gimp_pixel_rgns_process (pr);
    }

  if (! preview)
    {
      gimp_progress_update (1.0);
      /*  merge the shadow, update the drawable  */
      gimp_drawable_flush (drawable);
      gimp_drawable_merge_shadow (drawable->drawable_id, TRUE);
      gimp_drawable_update (drawable->drawable_id, x, y, width, height);
    }

  /*  free up buffers  */
  g_free (val_p1);
  g_free (val_p2);
  g_free (val_m1);
  g_free (val_m2);
  g_free (dest1);
  g_free (dest2);
}

static gdouble
compute_ramp (guchar  *dest1,
              guchar  *dest2,
              gint     length,
              gdouble  pct,
              gint     under_threshold)
{
  gint    hist[2000];
  gdouble diff;
  gint    count;
  gint    i;
  gint    sum;

  memset (hist, 0, sizeof (int) * 2000);
  count = 0;

  for (i = 0; i < length; i++)
    {
      if (*dest2 != 0)
        {
          diff = (gdouble) *dest1 / (gdouble) *dest2;

          if (under_threshold)
            {
              if (diff < pvals.threshold)
                {
                  hist[(int) (diff * 1000)] += 1;
                  count += 1;
                }
            }
          else
            {
              if (diff >= pvals.threshold && diff < 2.0)
                {
                  hist[(int) (diff * 1000)] += 1;
                  count += 1;
                }
            }
        }

      dest1++;
      dest2++;
    }

  if (pct == 0.0 || count == 0)
    return (under_threshold ? 1.0 : 0.0);

  sum = 0;
  for (i = 0; i < 2000; i++)
    {
      sum += hist[i];
      if (((gdouble) sum / (gdouble) count) > pct)
        {
          if (under_threshold)
            return (pvals.threshold - (gdouble) i / 1000.0);
          else
            return ((gdouble) i / 1000.0 - pvals.threshold);
        }
    }

  return (under_threshold ? 0.0 : 1.0);
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

  for(i = 0; i < width; i++)
    {
      sum = src1[i] + src2[i];
      if (sum > 255) sum = 255;
      else if(sum < 0) sum = 0;

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

  div = sqrt (2 * G_PI) * std_dev;
  constants [0] = -1.783 / std_dev;
  constants [1] = -1.723 / std_dev;
  constants [2] = 0.6318 / std_dev;
  constants [3] = 1.997  / std_dev;
  constants [4] = 1.6803 / div;
  constants [5] = 3.735 / div;
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
photocopy_dialog (GimpDrawable *drawable)
{
  GtkWidget     *dialog;
  GtkWidget     *main_vbox;
  GtkWidget     *preview;
  GtkWidget     *table;
  GtkAdjustment *scale_data;
  gboolean       run;

  gimp_ui_init (PLUG_IN_BINARY, FALSE);

  dialog = gimp_dialog_new (_("Photocopy"), PLUG_IN_ROLE,
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
                            G_CALLBACK (photocopy),
                            drawable);

  table = gtk_table_new (4, 3, FALSE);
  gtk_table_set_col_spacings (GTK_TABLE (table), 6);
  gtk_table_set_row_spacings (GTK_TABLE (table), 6);
  gtk_box_pack_start (GTK_BOX (main_vbox), table, FALSE, FALSE, 0);
  gtk_widget_show (table);

  /*  Label, scale, entry for pvals.amount  */
  scale_data = gimp_scale_entry_new (GTK_TABLE (table), 0, 0,
                                     _("_Mask radius:"), 100, 5,
                                     pvals.mask_radius, 3.0, 50.0, 1, 5.0, 2,
                                     TRUE, 0, 0,
                                     NULL, NULL);

  g_signal_connect (scale_data, "value-changed",
                    G_CALLBACK (gimp_double_adjustment_update),
                    &pvals.mask_radius);
  g_signal_connect_swapped (scale_data, "value-changed",
                            G_CALLBACK (gimp_preview_invalidate),
                            preview);

  /*  Label, scale, entry for pvals.amount  */
  scale_data = gimp_scale_entry_new (GTK_TABLE (table), 0, 1,
                                     _("_Sharpness:"), 50, 5,
                                     pvals.sharpness, 0.0, 1.0, 0.01, 0.1, 3,
                                     TRUE, 0, 0,
                                     NULL, NULL);

  g_signal_connect (scale_data, "value-changed",
                    G_CALLBACK (gimp_double_adjustment_update),
                    &pvals.sharpness);
  g_signal_connect_swapped (scale_data, "value-changed",
                            G_CALLBACK (gimp_preview_invalidate),
                            preview);

  /*  Label, scale, entry for pvals.amount  */
  scale_data = gimp_scale_entry_new (GTK_TABLE (table), 0, 2,
                                     _("Percent _black:"), 50, 5,
                                     pvals.pct_black, 0.0, 1.0, 0.01, 0.1, 3,
                                     TRUE, 0, 0,
                                     NULL, NULL);

  g_signal_connect (scale_data, "value-changed",
                    G_CALLBACK (gimp_double_adjustment_update),
                    &pvals.pct_black);
  g_signal_connect_swapped (scale_data, "value-changed",
                            G_CALLBACK (gimp_preview_invalidate),
                            preview);

  /*  Label, scale, entry for pvals.amount  */
  scale_data = gimp_scale_entry_new (GTK_TABLE (table), 0, 3,
                                     _("Percent _white:"), 50, 5,
                                     pvals.pct_white, 0.0, 1.0, 0.01, 0.1, 3,
                                     TRUE, 0, 0,
                                     NULL, NULL);

  g_signal_connect (scale_data, "value-changed",
                    G_CALLBACK (gimp_double_adjustment_update),
                    &pvals.pct_white);
  g_signal_connect_swapped (scale_data, "value-changed",
                            G_CALLBACK (gimp_preview_invalidate),
                            preview);

  gtk_widget_show (dialog);

  run = (gimp_dialog_run (GIMP_DIALOG (dialog)) == GTK_RESPONSE_OK);

  gtk_widget_destroy (dialog);

  return run;
}
