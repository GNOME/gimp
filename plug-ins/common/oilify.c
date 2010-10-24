/*
 * This is a plug-in for GIMP.
 *
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 * Copyright (C) 1996 Torsten Martinsen
 * Copyright (C) 2007 Daniel Richard G.
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

#include <string.h>

#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>
#include <libgimpmath/gimpmath.h>

#include "libgimp/stdplugins-intl.h"


#define PLUG_IN_PROC          "plug-in-oilify"
#define PLUG_IN_ENHANCED_PROC "plug-in-oilify-enhanced"
#define PLUG_IN_BINARY        "oilify"
#define PLUG_IN_ROLE          "gimp-oilify"

#define SCALE_WIDTH    125
#define HISTSIZE       256

#define MODE_RGB         0
#define MODE_INTEN       1


typedef struct
{
  gdouble  mask_size;
  gboolean use_mask_size_map;
  gint     mask_size_map;
  gdouble  exponent;
  gboolean use_exponent_map;
  gint     exponent_map;
  gint     mode;
} OilifyVals;


/* Declare local functions.
 */
static void      query          (void);
static void      run            (const gchar      *name,
                                 gint              nparams,
                                 const GimpParam  *param,
                                 gint             *nreturn_vals,
                                 GimpParam       **return_vals);

static void      oilify         (GimpDrawable     *drawable,
                                 GimpPreview      *preview);

static gboolean  oilify_dialog  (GimpDrawable     *drawable);


const GimpPlugInInfo PLUG_IN_INFO =
{
  NULL,  /* init_proc  */
  NULL,  /* quit_proc  */
  query, /* query_proc */
  run,   /* run_proc   */
};

static OilifyVals ovals =
{
  8.0,        /* mask size          */
  FALSE,      /* use mask-size map? */
  -1,         /* mask-size map      */
  8.0,        /* exponent           */
  FALSE,      /* use exponent map?  */
  -1,         /* exponent map       */
  MODE_INTEN  /* mode               */
};


MAIN ()

static void
query (void)
{
  static const GimpParamDef args[] =
  {
    { GIMP_PDB_INT32,    "run-mode",  "The run mode { RUN-INTERACTIVE (0), RUN-NONINTERACTIVE (1) }"       },
    { GIMP_PDB_IMAGE,    "image",     "Input image (unused)"                 },
    { GIMP_PDB_DRAWABLE, "drawable",  "Input drawable"                       },
    { GIMP_PDB_INT32,    "mask-size", "Oil paint mask size"                  },
    { GIMP_PDB_INT32,    "mode",      "Algorithm { RGB (0), INTENSITY (1) }" }
  };

  static const GimpParamDef args_enhanced[] =
  {
    { GIMP_PDB_INT32,    "run-mode",      "The run mode { RUN-INTERACTIVE (0), RUN-NONINTERACTIVE (1) }"       },
    { GIMP_PDB_IMAGE,    "image",         "Input image (unused)"             },
    { GIMP_PDB_DRAWABLE, "drawable",      "Input drawable"                   },
    { GIMP_PDB_INT32,    "mode",          "Algorithm { RGB (0), INTENSITY (1) }" },
    { GIMP_PDB_INT32,    "mask-size",     "Oil paint mask size"              },
    { GIMP_PDB_DRAWABLE, "mask-size-map", "Mask size control map"            },
    { GIMP_PDB_INT32,    "exponent",      "Oil paint exponent"               },
    { GIMP_PDB_DRAWABLE, "exponent-map",  "Exponent control map"             }
  };

  gimp_install_procedure (PLUG_IN_PROC,
                          N_("Smear colors to simulate an oil painting"),
                          "This function performs the well-known oil-paint "
                          "effect on the specified drawable.",
                          "Torsten Martinsen",
                          "Torsten Martinsen",
                          "1996",
                          N_("Oili_fy..."),
                          "RGB*, GRAY*",
                          GIMP_PLUGIN,
                          G_N_ELEMENTS (args), 0,
                          args, NULL);

  gimp_plugin_menu_register (PLUG_IN_PROC, "<Image>/Filters/Artistic");

  gimp_install_procedure (PLUG_IN_ENHANCED_PROC,
                          N_("Smear colors to simulate an oil painting"),
                          "This function performs the well-known oil-paint "
                          "effect on the specified drawable.",
                          "Torsten Martinsen, Daniel Richard G.",
                          "Torsten Martinsen, Daniel Richard G.",
                          "2007",
                          NULL,
                          "RGB*, GRAY*",
                          GIMP_PLUGIN,
                          G_N_ELEMENTS (args_enhanced), 0,
                          args_enhanced, NULL);
}

static void
run (const gchar      *name,
     gint              nparams,
     const GimpParam  *param,
     gint             *nreturn_vals,
     GimpParam       **return_vals)
{
  static GimpParam   values[1];
  GimpDrawable      *drawable;
  GimpRunMode        run_mode;
  GimpPDBStatusType  status = GIMP_PDB_SUCCESS;

  INIT_I18N ();

  run_mode = param[0].data.d_int32;

  /*  Get the specified drawable  */
  drawable = gimp_drawable_get (param[2].data.d_drawable);
  gimp_tile_cache_ntiles (2 * drawable->ntile_cols);

  *nreturn_vals = 1;
  *return_vals  = values;

  values[0].type          = GIMP_PDB_STATUS;
  values[0].data.d_status = status;

  switch (run_mode)
    {
    case GIMP_RUN_INTERACTIVE:
      /*  Possibly retrieve data  */
      gimp_get_data (PLUG_IN_PROC, &ovals);

      /*  First acquire information with a dialog  */
      if (! oilify_dialog (drawable))
        return;
      break;

    case GIMP_RUN_NONINTERACTIVE:
      /*  Interpret the arguments per the name used to invoke us  */
      if (! strcmp (name, PLUG_IN_PROC))
        {
          if (nparams != 5)
            {
              status = GIMP_PDB_CALLING_ERROR;
            }
          else
            {
              ovals.mask_size = (gdouble) param[3].data.d_int32;
              ovals.mode = param[4].data.d_int32;
            }
        }
      else if (! strcmp (name, PLUG_IN_ENHANCED_PROC))
        {
          if (nparams < 5 || nparams > 8)
            {
              status = GIMP_PDB_CALLING_ERROR;
            }
          else
            {
              ovals.mode      = param[3].data.d_int32;
              ovals.mask_size = (gdouble) param[4].data.d_int32;

              if (nparams >= 6)
                ovals.mask_size_map = param[5].data.d_int32;

              if (nparams >= 7)
                ovals.exponent      = (gdouble) param[6].data.d_int32;

              if (nparams == 8)
                ovals.exponent_map  = param[7].data.d_int32;

              ovals.use_mask_size_map = ovals.mask_size_map >= 0;
              ovals.use_exponent_map  = ovals.exponent_map >= 0;

              if (ovals.mask_size < 1.0 ||
                  ovals.exponent < 1.0  ||
                  (ovals.mode != MODE_INTEN && ovals.mode != MODE_RGB) ||
                  (ovals.mode == MODE_INTEN &&
                   ! gimp_drawable_is_rgb (drawable->drawable_id)))
                {
                  status = GIMP_PDB_CALLING_ERROR;
                }
            }
        }
      break;

    case GIMP_RUN_WITH_LAST_VALS:
      /*  Possibly retrieve data  */
      gimp_get_data (PLUG_IN_PROC, &ovals);
      break;

    default:
      break;
    }

  /*  Make sure that the drawable is gray or RGB color  */
  if ((status == GIMP_PDB_SUCCESS) &&
      (gimp_drawable_is_rgb (drawable->drawable_id) ||
       gimp_drawable_is_gray (drawable->drawable_id)))
    {
      gimp_progress_init (_("Oil painting"));

      oilify (drawable, NULL);

      if (run_mode != GIMP_RUN_NONINTERACTIVE)
        gimp_displays_flush ();

      /*  Store data  */
      if (run_mode == GIMP_RUN_INTERACTIVE)
        gimp_set_data (PLUG_IN_PROC, &ovals, sizeof (OilifyVals));
    }
  else
    {
      /* gimp_message ("oilify: cannot operate on indexed color images"); */
      status = GIMP_PDB_EXECUTION_ERROR;
    }

  values[0].data.d_status = status;

  gimp_drawable_detach (drawable);
}

/*
 * Helper function to read a sample from a mask-size/exponent map
 */
static inline gfloat
get_map_value (const guchar *src,
               gint          bpp)
{
  gfloat value;

  if (bpp >= 3)
    value = GIMP_RGB_LUMINANCE (src[0], src[1], src[2]);
  else
    value = *src;

  if (value < 1.0)
    value = 1.0;

  /*  value should be in [0,1]  */
  value /= 255.0;

  return value;
}

/*
 * This is a special-case form of the powf() function, limited to integer
 * exponents. It calculates e.g. x^13 as (x^8)*(x^4)*(x^1).
 *
 * x can be anything, y must be in [0,255]
 */
static inline gfloat
fast_powf (gfloat x, gint y)
{
  gfloat value;
  gfloat x_pow[8];
  guint  y_uint = (guint) y;
  guint  bitmask;
  gint   i;

  if (y_uint & 0x01)
    value = x;
  else
    value = 1.0;

  x_pow[0] = x;

  for (bitmask = 0x02, i = 1;
       bitmask <= y_uint;
       bitmask <<= 1, i++)
    {
      /*  x_pow[i] == x_pow[i-1]^2 == x^(2^i)  */

      x_pow[i] = SQR (x_pow[i - 1]);

      if (y_uint & bitmask)
        value *= x_pow[i];
    }

  return value;
}

/*
 * For each i in [0, HISTSIZE), hist[i] is the number of occurrences of the
 * value i. Return a value in [0, HISTSIZE) weighted heavily toward the
 * most frequent values in the histogram.
 *
 * Assuming that hist_max is the maximum number of occurrences for any
 * one value in the histogram, the weight given to each value i is
 *
 *         weight = (hist[i] / hist_max)^exponent
 *
 * (i.e. the normalized histogram frequency raised to some power)
 */
static inline guchar
weighted_average_value (gint hist[HISTSIZE], gfloat exponent)
{
  gint   i;
  gint   hist_max = 1;
  gint   exponent_int = 0;
  gfloat sum = 0.0;
  gfloat div = 1.0e-6;
  gint   value;

  for (i = 0; i < HISTSIZE; i++)
    hist_max = MAX (hist_max, hist[i]);

  if ((exponent - floor (exponent)) < 0.001 && exponent <= 255.0)
    exponent_int = (gint) exponent;

  for (i = 0; i < HISTSIZE; i++)
    {
      gfloat ratio = (gfloat) hist[i] / (gfloat) hist_max;
      gfloat weight;

      if (exponent_int)
        weight = fast_powf (ratio, exponent_int);
      else
        weight = pow (ratio, exponent);

      sum += weight * (gfloat) i;
      div += weight;
    }

  value = (gint) (sum / div);

  return (guchar) CLAMP0255 (value);
}

/*
 * For each i in [0, HISTSIZE), hist[i] is the number of occurrences of
 * pixels with intensity i. hist_rgb[][i] is the average color of those
 * pixels with intensity i, but with each channel multiplied by hist[i].
 * Write to dest a pixel whose color is a weighted average of all the
 * colors in hist_rgb[][], biased heavily toward those with the most
 * frequently-occurring intensities (as noted in hist[]).
 *
 * The weight formula is the same as in weighted_average_value().
 */
static inline void
weighted_average_color (gint    hist[HISTSIZE],
                        gint    hist_rgb[4][HISTSIZE],
                        gfloat  exponent,
                        guchar *dest,
                        gint    bpp)
{
  gint   i, b;
  gint   hist_max = 1;
  gint   exponent_int = 0;
  gfloat div = 1.0e-6;
  gfloat color[4] = { 0.0, 0.0, 0.0, 0.0 };

  for (i = 0; i < HISTSIZE; i++)
    hist_max = MAX (hist_max, hist[i]);

  if ((exponent - floor (exponent)) < 0.001 && exponent <= 255.0)
    exponent_int = (gint) exponent;

  for (i = 0; i < HISTSIZE; i++)
    {
      gfloat ratio = (gfloat) hist[i] / (gfloat) hist_max;
      gfloat weight;

      if (exponent_int)
        weight = fast_powf (ratio, exponent_int);
      else
        weight = pow (ratio, exponent);

      if (hist[i] > 0)
        for (b = 0; b < bpp; b++)
          color[b] += weight * (gfloat) hist_rgb[b][i] / (gfloat) hist[i];

      div += weight;
    }

  for (b = 0; b < bpp; b++)
    {
      gint c = (gint) (color[b] / div);

      dest[b] = (guchar) CLAMP0255 (c);
    }
}

/*
 * For all x and y as requested, replace the pixel at (x,y)
 * with a weighted average of the most frequently occurring
 * values in a circle of mask_size diameter centered at (x,y).
 */
static void
oilify (GimpDrawable *drawable,
        GimpPreview  *preview)
{
  gboolean      use_inten;
  gboolean      use_msmap = FALSE;
  gboolean      use_emap = FALSE;
  GimpDrawable *mask_size_map_drawable = NULL;
  GimpDrawable *exponent_map_drawable = NULL;
  GimpPixelRgn  mask_size_map_rgn;
  GimpPixelRgn  exponent_map_rgn;
  gint          msmap_bpp = 0;
  gint          emap_bpp = 0;
  GimpPixelRgn  dest_rgn;
  GimpPixelRgn *regions[3];
  gint          n_regions;
  gint          bpp;
  gint         *sqr_lut;
  gint          x1, y1, x2, y2;
  gint          width, height;
  gint          Hist[HISTSIZE];
  gint          Hist_rgb[4][HISTSIZE];
  gpointer      pr;
  gint          progress, max_progress;
  guchar       *src_buf;
  guchar       *src_inten_buf = NULL;
  gint          i;

  use_inten = (ovals.mode == MODE_INTEN);

  /*  Get the selection bounds  */
  if (preview)
    {
      gimp_preview_get_position (preview, &x1, &y1);
      gimp_preview_get_size (preview, &width, &height);

      x2 = x1 + width;
      y2 = y1 + height;
    }
  else
    {
      if (! gimp_drawable_mask_intersect (drawable->drawable_id,
                                          &x1, &y1, &width, &height))
	return;

      x2 = x1 + width;
      y2 = y1 + height;
    }

  progress = 0;
  max_progress = width * height;

  bpp = drawable->bpp;

  /*
   * Look-up-table implementation of the square function, for use in the
   * VERY TIGHT inner loops
   */
  {
    gint lut_size = (gint) ovals.mask_size / 2 + 1;

    sqr_lut = g_new (gint, lut_size);

    for (i = 0; i < lut_size; i++)
      sqr_lut[i] = SQR (i);
  }

  /*  Get the map drawables, if applicable  */

  if (ovals.use_mask_size_map && ovals.mask_size_map >= 0)
    {
      use_msmap = TRUE;

      mask_size_map_drawable = gimp_drawable_get (ovals.mask_size_map);
      gimp_pixel_rgn_init (&mask_size_map_rgn, mask_size_map_drawable,
                           x1, y1, width, height, FALSE, FALSE);

      msmap_bpp = mask_size_map_drawable->bpp;
    }

  if (ovals.use_exponent_map && ovals.exponent_map >= 0)
    {
      use_emap = TRUE;

      exponent_map_drawable = gimp_drawable_get (ovals.exponent_map);
      gimp_pixel_rgn_init (&exponent_map_rgn, exponent_map_drawable,
                           x1, y1, width, height, FALSE, FALSE);

      emap_bpp = exponent_map_drawable->bpp;
    }

  gimp_pixel_rgn_init (&dest_rgn, drawable,
                       x1, y1, width, height, (preview == NULL), TRUE);

  {
    GimpPixelRgn src_rgn;

    gimp_pixel_rgn_init (&src_rgn, drawable,
                         x1, y1, width, height, FALSE, FALSE);
    src_buf = g_new (guchar, width * height * bpp);
    gimp_pixel_rgn_get_rect (&src_rgn, src_buf, x1, y1, width, height);
  }

  /*
   * If we're working in intensity mode, then generate a separate intensity
   * map of the source image. This way, we can avoid calculating the
   * intensity of any given source pixel more than once.
   */
  if (use_inten)
    {
      guchar *src;
      guchar *dest;

      src_inten_buf = g_new (guchar, width * height);

      for (i = 0,
           src = src_buf,
           dest = src_inten_buf
           ;
           i < (width * height)
           ;
           i++,
           src += bpp,
           dest++)
        {
          *dest = (guchar) GIMP_RGB_LUMINANCE (src[0], src[1], src[2]);
        }
    }

  n_regions = 0;
  regions[n_regions++] = &dest_rgn;
  if (use_msmap)
    regions[n_regions++] = &mask_size_map_rgn;
  if (use_emap)
    regions[n_regions++] = &exponent_map_rgn;

  for (pr = gimp_pixel_rgns_register2 (n_regions, regions);
       pr != NULL;
       pr = gimp_pixel_rgns_process (pr))
    {
      gint    y;
      guchar *dest_row;
      guchar *src_msmap_row = NULL;
      guchar *src_emap_row = NULL;

      for (y = dest_rgn.y,
           dest_row = dest_rgn.data,
           src_msmap_row = mask_size_map_rgn.data,  /* valid iff use_msmap */
           src_emap_row = exponent_map_rgn.data     /* valid iff use_emap */
           ;
           y < (gint) (dest_rgn.y + dest_rgn.h)
           ;
           y++,
           dest_row += dest_rgn.rowstride,
           src_msmap_row += mask_size_map_rgn.rowstride,  /* valid iff use_msmap */
           src_emap_row += exponent_map_rgn.rowstride)    /* valid iff use_emap */
        {
          gint    x;
          guchar *dest;
          guchar *src_msmap = NULL;
          guchar *src_emap = NULL;

          for (x = dest_rgn.x,
               dest = dest_row,
               src_msmap = src_msmap_row,  /* valid iff use_msmap */
               src_emap = src_emap_row     /* valid iff use_emap */
               ;
               x < (gint) (dest_rgn.x + dest_rgn.w)
               ;
               x++,
               dest += bpp,
               src_msmap += msmap_bpp,  /* valid iff use_msmap */
               src_emap += emap_bpp)    /* valid iff use_emap */
            {
              gint    radius, radius_squared;
              gfloat  exponent;
              gint    mask_x1, mask_y1;
              gint    mask_x2, mask_y2;
              gint    mask_y;
              gint    src_offset;
              guchar *src_row;
              guchar *src_inten_row = NULL;

              if (use_msmap)
                {
                  gfloat factor = get_map_value (src_msmap, msmap_bpp);

                  radius = ROUND (factor * (0.5 * ovals.mask_size));
                }
              else
                {
                  radius = (gint) ovals.mask_size / 2;
                }

              radius_squared = SQR (radius);

              exponent = ovals.exponent;
              if (use_emap)
                exponent *= get_map_value (src_emap, emap_bpp);

              if (use_inten)
                memset (Hist, 0, sizeof (Hist));

              memset (Hist_rgb, 0, sizeof (Hist_rgb));

              mask_x1 = CLAMP ((x - radius), x1, x2);
              mask_y1 = CLAMP ((y - radius), y1, y2);
              mask_x2 = CLAMP ((x + radius + 1), x1, x2);
              mask_y2 = CLAMP ((y + radius + 1), y1, y2);

              src_offset = (mask_y1 - y1) * width + (mask_x1 - x1);

              for (mask_y = mask_y1,
                   src_row = src_buf + src_offset * bpp,
                   src_inten_row = src_inten_buf + src_offset  /* valid iff use_inten */
                   ;
                   mask_y < mask_y2
                   ;
                   mask_y++,
                   src_row += width * bpp,
                   src_inten_row += width)  /* valid iff use_inten */
                {
                  guchar *src;
                  guchar *src_inten = NULL;
                  gint    dy_squared = sqr_lut[ABS (mask_y - y)];
                  gint    mask_x;

                  for (mask_x = mask_x1,
                       src = src_row,
                       src_inten = src_inten_row  /* valid iff use_inten */
                       ;
                       mask_x < mask_x2
                       ;
                       mask_x++,
                       src += bpp,
                       src_inten++)  /* valid iff use_inten */
                    {
                      gint dx_squared = sqr_lut[ABS (mask_x - x)];
                      gint b;

                      /*  Stay inside a circular mask area  */
                      if ((dx_squared + dy_squared) > radius_squared)
                        continue;

                      if (use_inten)
                        {
                          gint inten = *src_inten;
                          ++Hist[inten];
                          for (b = 0; b < bpp; b++)
                            Hist_rgb[b][inten] += src[b];
                        }
                      else
                        {
                          for (b = 0; b < bpp; b++)
                            ++Hist_rgb[b][src[b]];
                        }

                    } /* for mask_x */
                } /* for mask_y */

              if (use_inten)
                {
                  weighted_average_color (Hist, Hist_rgb, exponent, dest, bpp);
                }
              else
                {
                  gint b;

                  for (b = 0; b < bpp; b++)
                    dest[b] = weighted_average_value (Hist_rgb[b], exponent);
                }

            } /* for x */
        } /* for y */

      if (preview)
        {
          gimp_drawable_preview_draw_region (GIMP_DRAWABLE_PREVIEW (preview),
                                             &dest_rgn);
        }
      else
        {
          progress += dest_rgn.w * dest_rgn.h;
          gimp_progress_update ((gdouble) progress / (gdouble) max_progress);
        }
    } /* for pr */

  /*  Detach from the map drawables  */
  if (mask_size_map_drawable)
    gimp_drawable_detach (mask_size_map_drawable);

  if (exponent_map_drawable)
    gimp_drawable_detach (exponent_map_drawable);

  if (src_inten_buf)
    g_free (src_inten_buf);

  g_free (src_buf);
  g_free (sqr_lut);

  if (!preview)
    {
      gimp_progress_update (1.0);
      /*  Update the oil-painted region  */
      gimp_drawable_flush (drawable);
      gimp_drawable_merge_shadow (drawable->drawable_id, TRUE);
      gimp_drawable_update (drawable->drawable_id, x1, y1, width, height);
    }
}

/*
 * Return TRUE iff the specified drawable can be used as a mask-size /
 * exponent map with the source image. The map and the image must have the
 * same dimensions.
 */
static gboolean
oilify_map_constrain (gint32   image_id G_GNUC_UNUSED,
                      gint32   drawable_id,
                      gpointer data)
{
  GimpDrawable *drawable = data;

  return (gimp_drawable_width (drawable_id)  == (gint) drawable->width &&
          gimp_drawable_height (drawable_id) == (gint) drawable->height);
}

static gint
oilify_dialog (GimpDrawable *drawable)
{
  GtkWidget     *dialog;
  GtkWidget     *main_vbox;
  GtkWidget     *preview;
  GtkWidget     *table;
  GtkWidget     *toggle;
  GtkWidget     *combo;
  GtkAdjustment *adj;
  gboolean       can_use_mode_inten;
  gboolean       ret;

  can_use_mode_inten = gimp_drawable_is_rgb (drawable->drawable_id);

  if (! can_use_mode_inten && ovals.mode == MODE_INTEN)
    ovals.mode = MODE_RGB;

  gimp_ui_init (PLUG_IN_BINARY, FALSE);

  dialog = gimp_dialog_new (_("Oilify"), PLUG_IN_ROLE,
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
                            G_CALLBACK (oilify), drawable);

  table = gtk_table_new (7, 3, FALSE);
  gtk_table_set_col_spacings (GTK_TABLE (table), 6);
  gtk_table_set_row_spacings (GTK_TABLE (table), 6);
  gtk_box_pack_start (GTK_BOX (main_vbox), table, FALSE, FALSE, 0);
  gtk_widget_show (table);

  /*
   * Mask-size scale
   */

  adj = gimp_scale_entry_new (GTK_TABLE (table), 0, 0,
                              _("_Mask size:"), SCALE_WIDTH, 0,
                              ovals.mask_size, 3.0, 50.0, 1.0, 5.0, 0,
                              TRUE, 0.0, 0.0,
                              NULL, NULL);
  g_signal_connect (adj, "value-changed",
                    G_CALLBACK (gimp_double_adjustment_update),
                    &ovals.mask_size);
  g_signal_connect_swapped (adj, "value-changed",
                            G_CALLBACK (gimp_preview_invalidate),
                            preview);

  /*
   * Mask-size map check button
   */

  toggle = gtk_check_button_new_with_mnemonic (_("Use m_ask-size map:"));
  gtk_table_attach (GTK_TABLE (table), toggle, 0, 3, 1, 2, GTK_FILL, 0, 0, 0);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (toggle),
                                ovals.use_mask_size_map);
  gtk_widget_show (toggle);

  g_signal_connect (toggle, "toggled",
                    G_CALLBACK (gimp_toggle_button_update),
                    &ovals.use_mask_size_map);
  g_signal_connect_swapped (toggle, "toggled",
                            G_CALLBACK (gimp_preview_invalidate),
                            preview);

  /*
   * Mask-size map combo
   */

  combo = gimp_drawable_combo_box_new (oilify_map_constrain, drawable);
  gimp_int_combo_box_connect (GIMP_INT_COMBO_BOX (combo), ovals.mask_size_map,
                              G_CALLBACK (gimp_int_combo_box_get_active),
                              &ovals.mask_size_map);
  g_signal_connect_swapped (combo, "changed",
                            G_CALLBACK (gimp_preview_invalidate),
                            preview);

  gtk_table_attach (GTK_TABLE (table), combo, 0, 3, 2, 3,
                    GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL, 0, 0);
  gtk_widget_show (combo);

  g_object_bind_property (toggle, "active",
                          combo,  "sensitive",
                          G_BINDING_SYNC_CREATE);

  /*
   * Exponent scale
   */

  adj = gimp_scale_entry_new (GTK_TABLE (table), 0, 3,
                              _("_Exponent:"), SCALE_WIDTH, 0,
                              ovals.exponent, 1.0, 20.0, 1.0, 4.0, 0,
                              TRUE, 0.0, 0.0,
                              NULL, NULL);
  g_signal_connect (adj, "value-changed",
                    G_CALLBACK (gimp_double_adjustment_update),
                    &ovals.exponent);
  g_signal_connect_swapped (adj, "value-changed",
                            G_CALLBACK (gimp_preview_invalidate),
                            preview);

  /*
   * Exponent map check button
   */

  toggle = gtk_check_button_new_with_mnemonic (_("Use e_xponent map:"));
  gtk_table_attach (GTK_TABLE (table), toggle, 0, 3, 4, 5, GTK_FILL, 0, 0, 0);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (toggle),
                                ovals.use_exponent_map);
  gtk_widget_show (toggle);

  g_signal_connect (toggle, "toggled",
                    G_CALLBACK (gimp_toggle_button_update),
                    &ovals.use_exponent_map);
  g_signal_connect_swapped (toggle, "toggled",
                            G_CALLBACK (gimp_preview_invalidate),
                            preview);

  /*
   * Exponent map combo
   */

  combo = gimp_drawable_combo_box_new (oilify_map_constrain, drawable);
  gimp_int_combo_box_connect (GIMP_INT_COMBO_BOX (combo), ovals.exponent_map,
                              G_CALLBACK (gimp_int_combo_box_get_active),
                              &ovals.exponent_map);
  g_signal_connect_swapped (combo, "changed",
                            G_CALLBACK (gimp_preview_invalidate),
                            preview);

  gtk_table_attach (GTK_TABLE (table), combo, 0, 3, 5, 6,
                    GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL, 0, 0);
  gtk_widget_show (combo);

  g_object_bind_property (toggle, "active",
                          combo,  "sensitive",
                          G_BINDING_SYNC_CREATE);

  /*
   * Intensity algorithm check button
   */

  toggle = gtk_check_button_new_with_mnemonic (_("_Use intensity algorithm"));
  gtk_table_attach (GTK_TABLE (table), toggle, 0, 3, 6, 7, GTK_FILL, 0, 0, 0);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (toggle), ovals.mode);
  gtk_widget_set_sensitive (toggle, can_use_mode_inten);
  gtk_widget_show (toggle);

  g_signal_connect (toggle, "toggled",
                    G_CALLBACK (gimp_toggle_button_update),
                    &ovals.mode);
  g_signal_connect_swapped (toggle, "toggled",
                            G_CALLBACK (gimp_preview_invalidate),
                            preview);

  gtk_widget_show (dialog);

  ret = (gimp_dialog_run (GIMP_DIALOG (dialog)) == GTK_RESPONSE_OK);

  gtk_widget_destroy (dialog);

  return ret;
}
