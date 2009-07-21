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

#include "config.h"

#include <string.h>

#include "libgimp/gimp.h"
#include "libgimp/gimpui.h"

#include "libgimp/stdplugins-intl.h"


#define PLUG_IN_PROC        "plug-in-retinex"
#define PLUG_IN_BINARY      "contrast-retinex"
#define MAX_RETINEX_SCALES    8
#define MIN_GAUSSIAN_SCALE   16
#define MAX_GAUSSIAN_SCALE  250
#define SCALE_WIDTH         150
#define ENTRY_WIDTH           4


typedef struct
{
  gint     scale;
  gint     nscales;
  gint     scales_mode;
  gfloat   cvar;
} RetinexParams;

typedef enum
{
  filter_uniform,
  filter_low,
  filter_high
} FilterMode;

/*
  Definit comment sont repartis les
  differents filtres en fonction de
  l'echelle (~= ecart type de la gaussienne)
 */
#define RETINEX_UNIFORM 0
#define RETINEX_LOW     1
#define RETINEX_HIGH    2

static gfloat RetinexScales[MAX_RETINEX_SCALES];

typedef struct
{
  gint    N;
  gfloat  sigma;
  gdouble B;
  gdouble b[4];
} gauss3_coefs;


/*
 * Declare local functions.
 */
static void     query                       (void);
static void     run                         (const gchar      *name,
                                             gint              nparams,
                                             const GimpParam  *param,
                                             gint             *nreturn_vals,
                                             GimpParam       **return_vals);

/* Gimp */
static gboolean retinex_dialog              (GimpDrawable *drawable);
static void     retinex                     (GimpDrawable *drawable,
                                             GimpPreview  *preview);

static void     retinex_scales_distribution (gfloat       *scales,
                                             gint          nscales,
                                             gint          mode,
                                             gint          s);

static void     compute_mean_var            (gfloat       *src,
                                             gfloat       *mean,
                                             gfloat       *var,
                                             gint          size,
                                             gint          bytes);
/*
 * Gauss
 */
static void     compute_coefs3              (gauss3_coefs *c,
                                             gfloat        sigma);

static void     gausssmooth                 (gfloat       *in,
                                             gfloat       *out,
                                             gint          size,
                                             gint          rowtride,
                                             gauss3_coefs *c);

/*
 * MSRCR = MultiScale Retinex with Color Restoration
 */
static void     MSRCR                       (guchar       *src,
                                             gint          width,
                                             gint          height,
                                             gint          bytes,
                                             gboolean      preview_mode);


/*
 * Private variables.
 */
static RetinexParams rvals =
{
  240,             /* Scale */
  3,               /* Scales */
  RETINEX_UNIFORM, /* Echelles reparties uniformement */
  1.2              /* A voir */
};

static GimpPlugInInfo PLUG_IN_INFO =
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
    { GIMP_PDB_INT32,    "run-mode",    "The run mode { RUN-INTERACTIVE (0), RUN-NONINTERACTIVE (1) }"        },
    { GIMP_PDB_IMAGE,    "image",       "Input image (unused)"                },
    { GIMP_PDB_DRAWABLE, "drawable",    "Input drawable"                      },
    { GIMP_PDB_INT32,    "scale",       "Biggest scale value"                 },
    { GIMP_PDB_INT32,    "nscales",     "Number of scales"                    },
    { GIMP_PDB_INT32,    "scales-mode", "Retinex distribution through scales" },
    { GIMP_PDB_FLOAT,    "cvar",        "Variance value"                      }
  };

  gimp_install_procedure (PLUG_IN_PROC,
                          N_("Enhance contrast using the Retinex method"),
                          "The Retinex Image Enhancement Algorithm is an "
                          "automatic image enhancement method that enhances "
                          "a digital image in terms of dynamic range "
                          "compression, color independence from the spectral "
                          "distribution of the scene illuminant, and "
                          "color/lightness rendition.",
                          "Fabien Pelisson",
                          "Fabien Pelisson",
                          "2003",
                          N_("Retine_x..."),
                          "RGB*",
                          GIMP_PLUGIN,
                          G_N_ELEMENTS (args), 0,
                          args, NULL);

  gimp_plugin_menu_register (PLUG_IN_PROC, "<Image>/Colors/Modify");
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
  gint               x, y, width, height;

  run_mode = param[0].data.d_int32;

  INIT_I18N ();

  *nreturn_vals = 1;
  *return_vals  = values;

  values[0].type          = GIMP_PDB_STATUS;
  values[0].data.d_status = status;

  drawable = gimp_drawable_get (param[2].data.d_drawable);

  if (! gimp_drawable_mask_intersect (drawable->drawable_id,
                                      &x, &y, &width, &height) ||
      width  < MIN_GAUSSIAN_SCALE ||
      height < MIN_GAUSSIAN_SCALE)
    {
      status = GIMP_PDB_EXECUTION_ERROR;
      gimp_drawable_detach (drawable);
      values[0].data.d_status = status;
      return;
    }

  gimp_tile_cache_ntiles (2 * (drawable->width / gimp_tile_width () + 1));

  switch (run_mode)
    {
    case GIMP_RUN_INTERACTIVE:
      /*  Possibly retrieve data  */
      gimp_get_data (PLUG_IN_PROC, &rvals);

      /*  First acquire information with a dialog  */
      if (! retinex_dialog (drawable))
        return;
      break;

    case GIMP_RUN_NONINTERACTIVE:
      /*  Make sure all the arguments are there!  */
      if (nparams != 7)
        {
          status = GIMP_PDB_CALLING_ERROR;
        }
      else
        {
          rvals.scale        = (param[3].data.d_int32);
          rvals.nscales      = (param[4].data.d_int32);
          rvals.scales_mode  = (param[5].data.d_int32);
          rvals.cvar         = (param[6].data.d_float);
        }
      break;

    case GIMP_RUN_WITH_LAST_VALS:
      gimp_get_data (PLUG_IN_PROC, &rvals);
      break;

    default:
      break;
    }

  if ((status == GIMP_PDB_SUCCESS) &&
      (gimp_drawable_is_rgb (drawable->drawable_id)))
    {
      gimp_progress_init (_("Retinex"));

      retinex (drawable, NULL);

      if (run_mode != GIMP_RUN_NONINTERACTIVE)
        gimp_displays_flush ();

      /*  Store data  */
      if (run_mode == GIMP_RUN_INTERACTIVE)
        gimp_set_data (PLUG_IN_PROC, &rvals, sizeof (RetinexParams));
    }
  else
    {
      status = GIMP_PDB_EXECUTION_ERROR;
    }

  gimp_drawable_detach (drawable);

  values[0].data.d_status = status;
}


static gboolean
retinex_dialog (GimpDrawable *drawable)
{
  GtkWidget *dialog;
  GtkWidget *main_vbox;
  GtkWidget *preview;
  GtkWidget *table;
  GtkWidget *combo;
  GtkWidget *scale;
  GtkObject *adj;
  gboolean   run;

  gimp_ui_init (PLUG_IN_BINARY, FALSE);

  dialog = gimp_dialog_new (_("Retinex Image Enhancement"), PLUG_IN_BINARY,
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
  gtk_container_add (GTK_CONTAINER (gtk_dialog_get_content_area (GTK_DIALOG (dialog))),
                     main_vbox);
  gtk_widget_show (main_vbox);

  preview = gimp_zoom_preview_new (drawable);
  gtk_box_pack_start (GTK_BOX (main_vbox), preview, TRUE, TRUE, 0);
  gtk_widget_show (preview);

  g_signal_connect_swapped (preview, "invalidated",
                            G_CALLBACK (retinex),
                            drawable);

  table = gtk_table_new (4, 3, FALSE);
  gtk_table_set_col_spacings (GTK_TABLE (table), 6);
  gtk_table_set_row_spacings (GTK_TABLE (table), 6);
  gtk_box_pack_start (GTK_BOX (main_vbox), table, FALSE, FALSE, 0);
  gtk_widget_show (table);

  combo = gimp_int_combo_box_new (_("Uniform"), filter_uniform,
                                  _("Low"),     filter_low,
                                  _("High"),    filter_high,
                                  NULL);

  gimp_int_combo_box_connect (GIMP_INT_COMBO_BOX (combo), rvals.scales_mode,
                              G_CALLBACK (gimp_int_combo_box_get_active),
                              &rvals.scales_mode);
  g_signal_connect_swapped (combo, "changed",
                            G_CALLBACK (gimp_preview_invalidate),
                            preview);

  gimp_table_attach_aligned (GTK_TABLE (table), 0, 0,
                             _("_Level:"), 0.0, 0.5,
                             combo, 2, FALSE);
  gtk_widget_show (combo);

  adj = gimp_scale_entry_new (GTK_TABLE (table), 0, 1,
                              _("_Scale:"), SCALE_WIDTH, ENTRY_WIDTH,
                              rvals.scale,
                              MIN_GAUSSIAN_SCALE, MAX_GAUSSIAN_SCALE, 1, 1, 0,
                              TRUE, 0, 0, NULL, NULL);
  scale = GIMP_SCALE_ENTRY_SCALE (adj);
  gtk_range_set_update_policy (GTK_RANGE (scale), GTK_UPDATE_DISCONTINUOUS);

  g_signal_connect (adj, "value-changed",
                    G_CALLBACK (gimp_int_adjustment_update),
                    &rvals.scale);
  g_signal_connect_swapped (adj, "value-changed",
                            G_CALLBACK (gimp_preview_invalidate),
                            preview);

  adj = gimp_scale_entry_new (GTK_TABLE (table), 0, 2,
                              _("Scale _division:"), SCALE_WIDTH, ENTRY_WIDTH,
                              rvals.nscales,
                              0, MAX_RETINEX_SCALES, 1, 1, 0,
                              TRUE, 0, 0, NULL, NULL);
  scale = GIMP_SCALE_ENTRY_SCALE (adj);
  gtk_range_set_update_policy (GTK_RANGE (scale), GTK_UPDATE_DISCONTINUOUS);

  g_signal_connect (adj, "value-changed",
                    G_CALLBACK (gimp_int_adjustment_update),
                    &rvals.nscales);
  g_signal_connect_swapped (adj, "value-changed",
                            G_CALLBACK (gimp_preview_invalidate),
                            preview);

  adj = gimp_scale_entry_new (GTK_TABLE (table), 0, 3,
                              _("Dy_namic:"), SCALE_WIDTH, ENTRY_WIDTH,
                              rvals.cvar, 0, 4, 0.1, 0.1, 1,
                              TRUE, 0, 0, NULL, NULL);
  scale = GIMP_SCALE_ENTRY_SCALE (adj);
  gtk_range_set_update_policy (GTK_RANGE (scale), GTK_UPDATE_DISCONTINUOUS);

  g_signal_connect (adj, "value-changed",
                    G_CALLBACK (gimp_float_adjustment_update),
                    &rvals.cvar);
  g_signal_connect_swapped (adj, "value-changed",
                            G_CALLBACK (gimp_preview_invalidate),
                            preview);

  gtk_widget_show (dialog);

  run = (gimp_dialog_run (GIMP_DIALOG (dialog)) == GTK_RESPONSE_OK);

  gtk_widget_destroy (dialog);

  return run;
}

/*
 * Applies the algorithm
 */
static void
retinex (GimpDrawable *drawable,
         GimpPreview  *preview)
{
  gint          x, y, width, height;
  gint          size, bytes;
  guchar       *src  = NULL;
  guchar       *psrc = NULL;
  GimpPixelRgn  dst_rgn, src_rgn;

  bytes = drawable->bpp;

  /*
   * Get the size of the current image or its selection.
   */
  if (preview)
    {
      src = gimp_zoom_preview_get_source (GIMP_ZOOM_PREVIEW (preview),
                                          &width, &height, &bytes);
    }
  else
    {
      if (! gimp_drawable_mask_intersect (drawable->drawable_id,
                                          &x, &y, &width, &height))
        return;

      /* Allocate memory */
      size = width * height * bytes;
      src = g_try_malloc (sizeof (guchar) * size);

      if (src == NULL)
        {
          g_warning ("Failed to allocate memory");
          return;
        }

      memset (src, 0, sizeof (guchar) * size);

      /* Fill allocated memory with pixel data */
      gimp_pixel_rgn_init (&src_rgn, drawable,
                           x, y, width, height,
                           FALSE, FALSE);
      gimp_pixel_rgn_get_rect (&src_rgn, src, x, y, width, height);
    }

  /*
    Algorithm for Multi-scale Retinex with color Restoration (MSRCR).
   */
  psrc = src;
  MSRCR (psrc, width, height, bytes, preview != NULL);

  if (preview)
    {
      gimp_preview_draw_buffer (preview, psrc, width * bytes);
    }
  else
    {
      gimp_pixel_rgn_init (&dst_rgn, drawable,
                           x, y, width, height,
                           TRUE, TRUE);
      gimp_pixel_rgn_set_rect (&dst_rgn, psrc, x, y, width, height);

      gimp_progress_update (1.0);

      gimp_drawable_flush (drawable);
      gimp_drawable_merge_shadow (drawable->drawable_id, TRUE);
      gimp_drawable_update (drawable->drawable_id, x, y, width, height);
    }

  g_free (src);
}


/*
 * calculate scale values for desired distribution.
 */
static void
retinex_scales_distribution(gfloat* scales, gint nscales, gint mode, gint s)
{
  if (nscales == 1)
    { /* For one filter we choose the median scale */
      scales[0] = (gint) s / 2;
    }
  else if (nscales == 2)
    { /* For two filters whe choose the median and maximum scale */
      scales[0] = (gint) s / 2;
      scales[1] = (gint) s;
    }
  else
    {
      gfloat size_step = (gfloat) s / (gfloat) nscales;
      gint   i;

      switch(mode)
        {
        case RETINEX_UNIFORM:
          for(i = 0; i < nscales; ++i)
            scales[i] = 2. + (gfloat) i * size_step;
          break;

        case RETINEX_LOW:
          size_step = (gfloat) log(s - 2.0) / (gfloat) nscales;
          for (i = 0; i < nscales; ++i)
            scales[i] = 2. + pow (10, (i * size_step) / log (10));
          break;

        case RETINEX_HIGH:
          size_step = (gfloat) log(s - 2.0) / (gfloat) nscales;
          for (i = 0; i < nscales; ++i)
            scales[i] = s - pow (10, (i * size_step) / log (10));
          break;

        default:
          break;
        }
    }
}

/*
 * Calculate the coefficients for the recursive filter algorithm
 * Fast Computation of gaussian blurring.
 */
static void
compute_coefs3 (gauss3_coefs *c, gfloat sigma)
{
  /*
   * Papers:  "Recursive Implementation of the gaussian filter.",
   *          Ian T. Young , Lucas J. Van Vliet, Signal Processing 44, Elsevier 1995.
   * formula: 11b       computation of q
   *          8c        computation of b0..b1
   *          10        alpha is normalization constant B
   */
  gfloat q, q2, q3;

  q = 0;

  if (sigma >= 2.5)
    {
      q = 0.98711 * sigma - 0.96330;
    }
  else if ((sigma >= 0.5) && (sigma < 2.5))
    {
      q = 3.97156 - 4.14554 * (gfloat) sqrt ((double) 1 - 0.26891 * sigma);
    }
  else
    {
      q = 0.1147705018520355224609375;
    }

  q2 = q * q;
  q3 = q * q2;
  c->b[0] = (1.57825+(2.44413*q)+(1.4281 *q2)+(0.422205*q3));
  c->b[1] = (        (2.44413*q)+(2.85619*q2)+(1.26661 *q3));
  c->b[2] = (                   -((1.4281*q2)+(1.26661 *q3)));
  c->b[3] = (                                 (0.422205*q3));
  c->B = 1.0-((c->b[1]+c->b[2]+c->b[3])/c->b[0]);
  c->sigma = sigma;
  c->N = 3;

/*
  g_printerr ("q %f\n", q);
  g_printerr ("q2 %f\n", q2);
  g_printerr ("q3 %f\n", q3);
  g_printerr ("c->b[0] %f\n", c->b[0]);
  g_printerr ("c->b[1] %f\n", c->b[1]);
  g_printerr ("c->b[2] %f\n", c->b[2]);
  g_printerr ("c->b[3] %f\n", c->b[3]);
  g_printerr ("c->B %f\n", c->B);
  g_printerr ("c->sigma %f\n", c->sigma);
  g_printerr ("c->N %d\n", c->N);
*/
}

static void
gausssmooth (gfloat *in, gfloat *out, gint size, gint rowstride, gauss3_coefs *c)
{
  /*
   * Papers:  "Recursive Implementation of the gaussian filter.",
   *          Ian T. Young , Lucas J. Van Vliet, Signal Processing 44, Elsevier 1995.
   * formula: 9a        forward filter
   *          9b        backward filter
   *          fig7      algorithm
   */
  gint i,n, bufsize;
  gfloat *w1,*w2;

  /* forward pass */
  bufsize = size+3;
  size -= 1;
  w1 = (gfloat *) g_try_malloc (bufsize * sizeof (gfloat));
  w2 = (gfloat *) g_try_malloc (bufsize * sizeof (gfloat));
  w1[0] = in[0];
  w1[1] = in[0];
  w1[2] = in[0];
  for ( i = 0 , n=3; i <= size ; i++, n++)
    {
      w1[n] = (gfloat)(c->B*in[i*rowstride] +
                       ((c->b[1]*w1[n-1] +
                         c->b[2]*w1[n-2] +
                         c->b[3]*w1[n-3] ) / c->b[0]));
    }

  /* backward pass */
  w2[size+1]= w1[size+3];
  w2[size+2]= w1[size+3];
  w2[size+3]= w1[size+3];
  for (i = size, n = i; i >= 0; i--, n--)
    {
      w2[n]= out[i * rowstride] = (gfloat)(c->B*w1[n] +
                                           ((c->b[1]*w2[n+1] +
                                             c->b[2]*w2[n+2] +
                                             c->b[3]*w2[n+3] ) / c->b[0]));
    }

  g_free (w1);
  g_free (w2);
}

/*
 * This function is the heart of the algo.
 * (a)  Filterings at several scales and sumarize the results.
 * (b)  Calculation of the final values.
 */
static void
MSRCR (guchar *src, gint width, gint height, gint bytes, gboolean preview_mode)
{

  gint          scale,row,col;
  gint          i,j;
  gint          size;
  gint          pos;
  gint          channel;
  guchar       *psrc = NULL;            /* backup pointer for src buffer */
  gfloat       *dst  = NULL;            /* float buffer for algorithm */
  gfloat       *pdst = NULL;            /* backup pointer for float buffer */
  gfloat       *in, *out;
  gint          channelsize;            /* Float memory cache for one channel */
  gfloat        weight;
  gauss3_coefs  coef;
  gfloat        mean, var;
  gfloat        mini, range, maxi;
  gfloat        alpha;
  gfloat        gain;
  gfloat        offset;
  gdouble       max_preview = 0.0;

  if (!preview_mode)
    {
      gimp_progress_init (_("Retinex: filtering"));
      max_preview = 3 * rvals.nscales;
    }

  /* Allocate all the memory needed for algorithm*/
  size = width * height * bytes;
  dst = g_try_malloc (size * sizeof (gfloat));
  if (dst == NULL)
    {
      g_warning ("Failed to allocate memory");
      return;
    }
  memset (dst, 0, size * sizeof (gfloat));

  channelsize  = (width * height);
  in  = (gfloat *) g_try_malloc (channelsize * sizeof (gfloat));
  if (in == NULL)
    {
      g_free (dst);
      g_warning ("Failed to allocate memory");
      return; /* do some clever stuff */
    }

  out  = (gfloat *) g_try_malloc (channelsize * sizeof (gfloat));
  if (out == NULL)
    {
      g_free (in);
      g_free (dst);
      g_warning ("Failed to allocate memory");
      return; /* do some clever stuff */
    }


  /*
     Calculate the scales of filtering according to the
     number of filter and their distribution.
   */

  retinex_scales_distribution (RetinexScales,
                               rvals.nscales, rvals.scales_mode, rvals.scale);

  /*
      Filtering according to the various scales.
      Summerize the results of the various filters according to a
      specific weight(here equivalent for all).
  */
  weight = 1./ (gfloat) rvals.nscales;

  /*
    The recursive filtering algorithm needs different coefficients according
    to the selected scale (~ = standard deviation of Gaussian).
   */
  pos = 0;
  for (channel = 0; channel < 3; channel++)
    {
      for (i = 0, pos = channel; i < channelsize ; i++, pos += bytes)
         {
            /* 0-255 => 1-256 */
            in[i] = (gfloat)(src[pos] + 1.0);
         }
      for (scale = 0; scale < rvals.nscales; scale++)
        {
          compute_coefs3 (&coef, RetinexScales[scale]);
          /*
           *  Filtering (smoothing) Gaussian recursive.
           *
           *  Filter rows first
           */
          for (row=0 ;row < height; row++)
            {
              pos =  row * width;
              gausssmooth (in + pos, out + pos, width, 1, &coef);
            }

          memcpy(in,  out, channelsize * sizeof(gfloat));
          memset(out, 0  , channelsize * sizeof(gfloat));

          /*
           *  Filtering (smoothing) Gaussian recursive.
           *
           *  Second columns
           */
          for (col=0; col < width; col++)
            {
              pos = col;
              gausssmooth(in + pos, out + pos, height, width, &coef);
            }

          /*
             Summarize the filtered values.
             In fact one calculates a ratio between the original values and the filtered values.
           */
          for (i = 0, pos = channel; i < channelsize; i++, pos += bytes)
            {
              dst[pos] += weight * (log (src[pos] + 1.) - log (out[i]));
            }

           if (!preview_mode)
             gimp_progress_update ((channel * rvals.nscales + scale) /
                                   max_preview);
        }
    }
  g_free(in);
  g_free(out);

  /*
      Final calculation with original value and cumulated filter values.
      The parameters gain, alpha and offset are constants.
  */
  /* Ci(x,y)=log[a Ii(x,y)]-log[ Ei=1-s Ii(x,y)] */

  alpha  = 128.;
  gain   = 1.;
  offset = 0.;

  for (i = 0; i < size; i += bytes)
    {
      gfloat logl;

      psrc = src+i;
      pdst = dst+i;

      logl = log((gfloat)psrc[0] + (gfloat)psrc[1] + (gfloat)psrc[2] + 3.);

      pdst[0] = gain * ((log(alpha * (psrc[0]+1.)) - logl) * pdst[0]) + offset;
      pdst[1] = gain * ((log(alpha * (psrc[1]+1.)) - logl) * pdst[1]) + offset;
      pdst[2] = gain * ((log(alpha * (psrc[2]+1.)) - logl) * pdst[2]) + offset;
    }

/*  if (!preview_mode)
    gimp_progress_update ((2.0 + (rvals.nscales * 3)) /
                          ((rvals.nscales * 3) + 3));*/

  /*
      Adapt the dynamics of the colors according to the statistics of the first and second order.
      The use of the variance makes it possible to control the degree of saturation of the colors.
  */
  pdst = dst;

  compute_mean_var (pdst, &mean, &var, size, bytes);
  mini = mean - rvals.cvar*var;
  maxi = mean + rvals.cvar*var;
  range = maxi - mini;

  if (!range)
    range = 1.0;

  for (i = 0; i < size; i+= bytes)
    {
      psrc = src + i;
      pdst = dst + i;

      for (j = 0 ; j < 3 ; j++)
        {
          gfloat c = 255 * ( pdst[j] - mini ) / range;

          psrc[j] = (guchar) CLAMP (c, 0, 255);
        }
    }

  g_free (dst);
}

/*
 * Calculate the average and variance in one go.
 */
static void
compute_mean_var (gfloat *src, gfloat *mean, gfloat *var, gint size, gint bytes)
{
  gfloat vsquared;
  gint i,j;
  gfloat *psrc;

  vsquared = 0;
  *mean = 0;
  for (i = 0; i < size; i+= bytes)
    {
       psrc = src+i;
       for (j = 0 ; j < 3 ; j++)
         {
            *mean += psrc[j];
            vsquared += psrc[j] * psrc[j];
         }
    }

  *mean /= (gfloat) size; /* mean */
  vsquared /= (gfloat) size; /* mean (x^2) */
  *var = ( vsquared - (*mean * *mean) );
  *var = sqrt(*var); /* var */
}
