/* LIC 0.14 -- image filter plug-in for GIMP
 * Copyright (C) 1996 Tom Bech
 *
 * E-mail: tomb@gimp.org
 * You can contact the original GIMP authors at gimp@xcf.berkeley.edu
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
 *
 * In other words, you can't sue me for whatever happens while using this ;)
 *
 * Changes (post 0.10):
 * -> 0.11: Fixed a bug in the convolution kernels (Tom).
 * -> 0.12: Added Quartic's bilinear interpolation stuff (Tom).
 * -> 0.13 Changed some UI stuff causing trouble with the 0.60 release, added
 *         the (GIMP) tags and changed random() calls to rand() (Tom)
 * -> 0.14 Ported to 0.99.11 (Tom)
 *
 * This plug-in implements the Line Integral Convolution (LIC) as described in
 * Cabral et al. "Imaging vector fields using line integral convolution" in the
 * Proceedings of ACM SIGGRAPH 93. Publ. by ACM, New York, NY, USA. p. 263-270.
 * (See http://www8.cs.umu.se/kurser/TDBD13/VT00/extra/p263-cabral.pdf)
 *
 * Some of the code is based on code by Steinar Haugen (thanks!), the Perlin
 * noise function is practically ripped as is :)
 */

#include "config.h"

#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

#include "libgimp/stdplugins-intl.h"


/************/
/* Typedefs */
/************/

#define numx    40              /* Pseudo-random vector grid size */
#define numy    40

#define PLUG_IN_PROC   "plug-in-lic"
#define PLUG_IN_BINARY "van-gogh-lic"
#define PLUG_IN_ROLE   "gimp-van-gogh-lic"

typedef enum
{
  LIC_HUE,
  LIC_SATURATION,
  LIC_BRIGHTNESS
} LICEffectChannel;


/*****************************/
/* Global variables and such */
/*****************************/

static gdouble G[numx][numy][2];

typedef struct
{
  gdouble  filtlen;
  gdouble  noisemag;
  gdouble  intsteps;
  gdouble  minv;
  gdouble  maxv;
  gint     effect_channel;
  gint     effect_operator;
  gint     effect_convolve;
  gint32   effect_image_id;
} LicValues;

static LicValues licvals;

static gdouble l      = 10.0;
static gdouble dx     =  2.0;
static gdouble dy     =  2.0;
static gdouble minv   = -2.5;
static gdouble maxv   =  2.5;
static gdouble isteps = 20.0;

static gboolean source_drw_has_alpha = FALSE;

static gint    effect_width, effect_height;
static gint    border_x, border_y, border_w, border_h;

static GtkWidget *dialog;

/************************/
/* Convenience routines */
/************************/

static void
peek (GimpPixelRgn *src_rgn,
      gint          x,
      gint          y,
      GimpRGB      *color)
{
  static guchar data[4] = { 0, };

  gimp_pixel_rgn_get_pixel (src_rgn, data, x, y);
  gimp_rgba_set_uchar (color, data[0], data[1], data[2], data[3]);
}

static void
poke (GimpPixelRgn *dest_rgn,
      gint          x,
      gint          y,
      GimpRGB      *color)
{
  static guchar data[4];

  gimp_rgba_get_uchar (color, &data[0], &data[1], &data[2], &data[3]);
  gimp_pixel_rgn_set_pixel (dest_rgn, data, x, y);
}

static gint
peekmap (const guchar *image,
         gint          x,
         gint          y)
{
  while (x < 0)
    x += effect_width;
  x %= effect_width;

  while (y < 0)
    y += effect_height;
  y %= effect_height;

  return (gint) image[x + effect_width * y];
}

/*************/
/* Main part */
/*************/

/***************************************************/
/* Compute the derivative in the x and y direction */
/* We use these convolution kernels:               */
/*     |1 0 -1|     |  1   2   1|                  */
/* DX: |2 0 -2| DY: |  0   0   0|                  */
/*     |1 0 -1|     | -1  -2  -1|                  */
/* (It's a varation of the Sobel kernels, really)  */
/***************************************************/

static gint
gradx (const guchar *image,
       gint          x,
       gint          y)
{
  gint val = 0;

  val = val +     peekmap (image, x-1, y-1);
  val = val -     peekmap (image, x+1, y-1);

  val = val + 2 * peekmap (image, x-1, y);
  val = val - 2 * peekmap (image, x+1, y);

  val = val +     peekmap (image, x-1, y+1);
  val = val -     peekmap (image, x+1, y+1);

  return val;
}

static gint
grady (const guchar *image,
       gint          x,
       gint          y)
{
  gint val = 0;

  val = val +     peekmap (image, x-1, y-1);
  val = val + 2 * peekmap (image, x,   y-1);
  val = val +     peekmap (image, x+1, y-1);

  val = val -     peekmap (image, x-1, y+1);
  val = val - 2 * peekmap (image, x,   y+1);
  val = val -     peekmap (image, x+1, y+1);

  return val;
}

/************************************/
/* A nice 2nd order cubic spline :) */
/************************************/

static gdouble
cubic (gdouble t)
{
  gdouble at = fabs (t);

  return (at < 1.0) ? at * at * (2.0 * at - 3.0) + 1.0 : 0.0;
}

static gdouble
omega (gdouble u,
       gdouble v,
       gint    i,
       gint    j)
{
  while (i < 0)
    i += numx;

  while (j < 0)
    j += numy;

  i %= numx;
  j %= numy;

  return cubic (u) * cubic (v) * (G[i][j][0]*u + G[i][j][1]*v);
}

/*************************************************************/
/* The noise function (2D variant of Perlins noise function) */
/*************************************************************/

static gdouble
noise (gdouble x,
       gdouble y)
{
  gint i, sti = (gint) floor (x / dx);
  gint j, stj = (gint) floor (y / dy);

  gdouble sum = 0.0;

  /* Calculate the gdouble sum */
  /* ======================== */

  for (i = sti; i <= sti + 1; i++)
    for (j = stj; j <= stj + 1; j++)
      sum += omega ((x - (gdouble) i * dx) / dx,
                    (y - (gdouble) j * dy) / dy,
                    i, j);

  return sum;
}

/*************************************************/
/* Generates pseudo-random vectors with length 1 */
/*************************************************/

static void
generatevectors (void)
{
  gdouble alpha;
  gint i, j;
  GRand *gr;

  gr = g_rand_new();

  for (i = 0; i < numx; i++)
    {
      for (j = 0; j < numy; j++)
        {
          alpha = g_rand_double_range (gr, 0, 2) * G_PI;
          G[i][j][0] = cos (alpha);
          G[i][j][1] = sin (alpha);
        }
    }

  g_rand_free (gr);
}

/* A simple triangle filter */
/* ======================== */

static gdouble
filter (gdouble u)
{
  gdouble f = 1.0 - fabs (u) / l;

  return (f < 0.0) ? 0.0 : f;
}

/******************************************************/
/* Compute the Line Integral Convolution (LIC) at x,y */
/******************************************************/

static gdouble
lic_noise (gint    x,
           gint    y,
           gdouble vx,
           gdouble vy)
{
  gdouble i = 0.0;
  gdouble f1 = 0.0, f2 = 0.0;
  gdouble u, step = 2.0 * l / isteps;
  gdouble xx = (gdouble) x, yy = (gdouble) y;
  gdouble c, s;

  /* Get vector at x,y */
  /* ================= */

  c = vx;
  s = vy;

  /* Calculate integral numerically */
  /* ============================== */

  f1 = filter (-l) * noise (xx + l * c , yy + l * s);

  for (u = -l + step; u <= l; u += step)
    {
      f2 = filter (u) * noise ( xx - u * c , yy - u * s);
      i += (f1 + f2) * 0.5 * step;
      f1 = f2;
    }

  i = (i - minv) / (maxv - minv);

  i = CLAMP (i, 0.0, 1.0);

  i = (i / 2.0) + 0.5;

  return i;
}

static void
getpixel (GimpPixelRgn *src_rgn,
          GimpRGB      *p,
          gdouble       u,
          gdouble       v)
{
  register gint x1, y1, x2, y2;
  gint width, height;
  static GimpRGB pp[4];

  width = src_rgn->w;
  height = src_rgn->h;

  x1 = (gint)u;
  y1 = (gint)v;

  if (x1 < 0)
    x1 = width - (-x1 % width);
  else
    x1 = x1 % width;

  if (y1 < 0)
    y1 = height - (-y1 % height);
  else
    y1 = y1 % height;

  x2 = (x1 + 1) % width;
  y2 = (y1 + 1) % height;

  peek (src_rgn, x1, y1, &pp[0]);
  peek (src_rgn, x2, y1, &pp[1]);
  peek (src_rgn, x1, y2, &pp[2]);
  peek (src_rgn, x2, y2, &pp[3]);

  if (source_drw_has_alpha)
    *p = gimp_bilinear_rgba (u, v, pp);
  else
    *p = gimp_bilinear_rgb (u, v, pp);
}

static void
lic_image (GimpPixelRgn *src_rgn,
           gint          x,
           gint          y,
           gdouble       vx,
           gdouble       vy,
           GimpRGB      *color)
{
  gdouble u, step = 2.0 * l / isteps;
  gdouble xx = (gdouble) x, yy = (gdouble) y;
  gdouble c, s;
  GimpRGB col = { 0, 0, 0, 0 };
  GimpRGB col1, col2, col3;

  /* Get vector at x,y */
  /* ================= */

  c = vx;
  s = vy;

  /* Calculate integral numerically */
  /* ============================== */

  getpixel (src_rgn, &col1, xx + l * c, yy + l * s);
  if (source_drw_has_alpha)
    gimp_rgba_multiply (&col1, filter (-l));
  else
    gimp_rgb_multiply (&col1, filter (-l));

  for (u = -l + step; u <= l; u += step)
    {
      getpixel (src_rgn, &col2, xx - u * c, yy - u * s);
      if (source_drw_has_alpha)
        {
          gimp_rgba_multiply (&col2, filter (u));

          col3 = col1;
          gimp_rgba_add (&col3, &col2);
          gimp_rgba_multiply (&col3, 0.5 * step);
          gimp_rgba_add (&col, &col3);
        }
      else
        {
          gimp_rgb_multiply (&col2, filter (u));

          col3 = col1;
          gimp_rgb_add (&col3, &col2);
          gimp_rgb_multiply (&col3, 0.5 * step);
          gimp_rgb_add (&col, &col3);
        }
      col1 = col2;
    }
  if (source_drw_has_alpha)
    gimp_rgba_multiply (&col, 1.0 / l);
  else
    gimp_rgb_multiply (&col, 1.0 / l);
  gimp_rgb_clamp (&col);

  *color = col;
}

static guchar*
rgb_to_hsl (GimpDrawable     *drawable,
            LICEffectChannel  effect_channel)
{
  guchar       *themap, data[4];
  gint          x, y;
  GimpRGB       color;
  GimpHSL       color_hsl;
  gdouble       val = 0.0;
  glong         maxc, index = 0;
  GimpPixelRgn  region;
  GRand        *gr;

  gr = g_rand_new ();

  maxc = drawable->width * drawable->height;

  gimp_pixel_rgn_init (&region, drawable, border_x, border_y,
                       border_w, border_h, FALSE, FALSE);

  themap = g_new (guchar, maxc);

  for (y = 0; y < region.h; y++)
    {
      for (x = 0; x < region.w; x++)
        {
          data[3] = 255;

          gimp_pixel_rgn_get_pixel (&region, data, x, y);
          gimp_rgba_set_uchar (&color, data[0], data[1], data[2], data[3]);
          gimp_rgb_to_hsl (&color, &color_hsl);

          switch (effect_channel)
            {
            case LIC_HUE:
              val = color_hsl.h * 255;
              break;
            case LIC_SATURATION:
              val = color_hsl.s * 255;
              break;
            case LIC_BRIGHTNESS:
              val = color_hsl.l * 255;
              break;
            }

          /* add some random to avoid unstructured areas. */
          val += g_rand_double_range (gr, -1.0, 1.0);

          themap[index++] = (guchar) CLAMP0255 (RINT (val));
        }
    }

  g_rand_free (gr);

  return themap;
}


static void
compute_lic (GimpDrawable *drawable,
             const guchar *scalarfield,
             gboolean      rotate)
{
  gint xcount, ycount;
  GimpRGB color;
  gdouble vx, vy, tmp;
  GimpPixelRgn src_rgn, dest_rgn;

  gimp_pixel_rgn_init (&src_rgn, drawable,
                       border_x, border_y,
                       border_w, border_h, FALSE, FALSE);

  gimp_pixel_rgn_init (&dest_rgn, drawable,
                       border_x, border_y,
                       border_w, border_h, TRUE, TRUE);

  for (ycount = 0; ycount < src_rgn.h; ycount++)
    {
      for (xcount = 0; xcount < src_rgn.w; xcount++)
        {
          /* Get derivative at (x,y) and normalize it */
          /* ============================================================== */

          vx = gradx (scalarfield, xcount, ycount);
          vy = grady (scalarfield, xcount, ycount);

          /* Rotate if needed */
          if (rotate)
            {
              tmp = vy;
              vy = -vx;
              vx = tmp;
            }

          tmp = sqrt (vx * vx + vy * vy);
          if (tmp >= 0.000001)
            {
              tmp = 1.0 / tmp;
              vx *= tmp;
              vy *= tmp;
            }

          /* Convolve with the LIC at (x,y) */
          /* ============================== */

          if (licvals.effect_convolve == 0)
            {
              peek (&src_rgn, xcount, ycount, &color);
              tmp = lic_noise (xcount, ycount, vx, vy);
              if (source_drw_has_alpha)
                gimp_rgba_multiply (&color, tmp);
              else
                gimp_rgb_multiply (&color, tmp);
            }
          else
            {
              lic_image (&src_rgn, xcount, ycount, vx, vy, &color);
            }
          poke (&dest_rgn, xcount, ycount, &color);
        }

      gimp_progress_update ((gfloat) ycount / (gfloat) src_rgn.h);
    }
  gimp_progress_update (1.0);
}

static void
compute_image (GimpDrawable *drawable)
{
  GimpDrawable *effect;
  guchar       *scalarfield = NULL;

  /* Get some useful info on the input drawable */
  /* ========================================== */
  if (! gimp_drawable_mask_intersect (drawable->drawable_id,
                                      &border_x, &border_y, &border_w, &border_h))
    return;

  gimp_progress_init (_("Van Gogh (LIC)"));

  if (licvals.effect_convolve == 0)
    generatevectors ();

  if (licvals.filtlen < 0.1)
    licvals.filtlen = 0.1;

  l = licvals.filtlen;
  dx = dy = licvals.noisemag;
  minv = licvals.minv / 10.0;
  maxv = licvals.maxv / 10.0;
  isteps = licvals.intsteps;

  source_drw_has_alpha = gimp_drawable_has_alpha (drawable->drawable_id);

  effect = gimp_drawable_get (licvals.effect_image_id);

  effect_width = effect->width;
  effect_height = effect->height;

  switch (licvals.effect_channel)
    {
      case 0:
        scalarfield = rgb_to_hsl (effect, LIC_HUE);
        break;
      case 1:
        scalarfield = rgb_to_hsl (effect, LIC_SATURATION);
        break;
      case 2:
        scalarfield = rgb_to_hsl (effect, LIC_BRIGHTNESS);
        break;
    }

  compute_lic (drawable, scalarfield, licvals.effect_operator);

  g_free (scalarfield);

  /* Update image */
  /* ============ */

  gimp_drawable_flush (drawable);
  gimp_drawable_merge_shadow (drawable->drawable_id, TRUE);
  gimp_drawable_update (drawable->drawable_id, border_x, border_y,
                        border_w, border_h);

  gimp_displays_flush ();
}

/**************************/
/* Below is only UI stuff */
/**************************/

static gboolean
effect_image_constrain (gint32    image_id,
                        gint32    drawable_id,
                        gpointer  data)
{
  return gimp_drawable_is_rgb (drawable_id);
}

static gboolean
create_main_dialog (void)
{
  GtkWidget     *vbox;
  GtkWidget     *hbox;
  GtkWidget     *frame;
  GtkWidget     *table;
  GtkWidget     *combo;
  GtkAdjustment *scale_data;
  gint           row;
  gboolean       run;

  gimp_ui_init (PLUG_IN_BINARY, TRUE);

  dialog = gimp_dialog_new (_("Van Gogh (LIC)"), PLUG_IN_ROLE,
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

  vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 12);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), 12);
  gtk_box_pack_start (GTK_BOX (gtk_dialog_get_content_area (GTK_DIALOG (dialog))),
                      vbox, TRUE, TRUE, 0);
  gtk_widget_show (vbox);

  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 12);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
  gtk_widget_show (hbox);

  frame = gimp_int_radio_group_new (TRUE, _("Effect Channel"),
                                    G_CALLBACK (gimp_radio_button_update),
                                    &licvals.effect_channel,
                                    licvals.effect_channel,

                                    _("_Hue"),        0, NULL,
                                    _("_Saturation"), 1, NULL,
                                    _("_Brightness"), 2, NULL,

                                    NULL);
  gtk_box_pack_start (GTK_BOX (hbox), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  frame = gimp_int_radio_group_new (TRUE, _("Effect Operator"),
                                    G_CALLBACK (gimp_radio_button_update),
                                    &licvals.effect_operator,
                                    licvals.effect_operator,

                                    _("_Derivative"), 0, NULL,
                                    _("_Gradient"),   1, NULL,

                                    NULL);
  gtk_box_pack_start (GTK_BOX (hbox), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  frame = gimp_int_radio_group_new (TRUE, _("Convolve"),
                                    G_CALLBACK (gimp_radio_button_update),
                                    &licvals.effect_convolve,
                                    licvals.effect_convolve,

                                    _("_With white noise"),  0, NULL,
                                    _("W_ith source image"), 1, NULL,

                                    NULL);
  gtk_box_pack_start (GTK_BOX (hbox), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  /* Effect image menu */
  table = gtk_table_new (1, 2, FALSE);
  gtk_table_set_col_spacings (GTK_TABLE (table), 6);
  gtk_box_pack_start (GTK_BOX (vbox), table, FALSE, FALSE, 0);
  gtk_widget_show (table);

  combo = gimp_drawable_combo_box_new (effect_image_constrain, NULL);
  gimp_int_combo_box_connect (GIMP_INT_COMBO_BOX (combo),
                              licvals.effect_image_id,
                              G_CALLBACK (gimp_int_combo_box_get_active),
                              &licvals.effect_image_id);

  gimp_table_attach_aligned (GTK_TABLE (table), 0, 0,
                             _("_Effect image:"), 0.0, 0.5, combo, 2, TRUE);

  table = gtk_table_new (5, 3, FALSE);
  gtk_table_set_col_spacings (GTK_TABLE (table), 6);
  gtk_table_set_row_spacings (GTK_TABLE (table), 6);
  gtk_box_pack_start (GTK_BOX (vbox), table, FALSE, FALSE, 0);
  gtk_widget_show (table);

  row = 0;

  scale_data = gimp_scale_entry_new (GTK_TABLE (table), 0, row++,
                                     _("_Filter length:"), 0, 6,
                                     licvals.filtlen, 0.1, 64, 1.0, 8.0, 1,
                                     TRUE, 0, 0,
                                     NULL, NULL);
  g_signal_connect (scale_data, "value-changed",
                    G_CALLBACK (gimp_double_adjustment_update),
                    &licvals.filtlen);

  scale_data = gimp_scale_entry_new (GTK_TABLE (table), 0, row++,
                                     _("_Noise magnitude:"), 0, 6,
                                     licvals.noisemag, 1, 5, 0.1, 1.0, 1,
                                     TRUE, 0, 0,
                                     NULL, NULL);
  g_signal_connect (scale_data, "value-changed",
                    G_CALLBACK (gimp_double_adjustment_update),
                    &licvals.noisemag);

  scale_data = gimp_scale_entry_new (GTK_TABLE (table), 0, row++,
                                     _("In_tegration steps:"), 0, 6,
                                     licvals.intsteps, 1, 40, 1.0, 5.0, 1,
                                     TRUE, 0, 0,
                                     NULL, NULL);
  g_signal_connect (scale_data, "value-changed",
                    G_CALLBACK (gimp_double_adjustment_update),
                    &licvals.intsteps);

  scale_data = gimp_scale_entry_new (GTK_TABLE (table), 0, row++,
                                     _("_Minimum value:"), 0, 6,
                                     licvals.minv, -100, 0, 1, 10, 1,
                                     TRUE, 0, 0,
                                     NULL, NULL);
  g_signal_connect (scale_data, "value-changed",
                    G_CALLBACK (gimp_double_adjustment_update),
                    &licvals.minv);

  scale_data = gimp_scale_entry_new (GTK_TABLE (table), 0, row++,
                                     _("M_aximum value:"), 0, 6,
                                     licvals.maxv, 0, 100, 1, 10, 1,
                                     TRUE, 0, 0,
                                     NULL, NULL);
  g_signal_connect (scale_data, "value-changed",
                    G_CALLBACK (gimp_double_adjustment_update),
                    &licvals.maxv);

  gtk_widget_show (dialog);

  run = (gimp_dialog_run (GIMP_DIALOG (dialog)) == GTK_RESPONSE_OK);

  gtk_widget_destroy (dialog);

  return run;
}

/*************************************/
/* Set parameters to standard values */
/*************************************/

static void
set_default_settings (void)
{
  licvals.filtlen          = 5;
  licvals.noisemag         = 2;
  licvals.intsteps         = 25;
  licvals.minv             = -25;
  licvals.maxv             = 25;
  licvals.effect_channel   = 2;
  licvals.effect_operator  = 1;
  licvals.effect_convolve  = 1;
  licvals.effect_image_id  = 0;
}

static void
query (void)
{
  static const GimpParamDef args[] =
  {
    { GIMP_PDB_INT32,    "run-mode", "The run mode { RUN-INTERACTIVE (0) }"    },
    { GIMP_PDB_IMAGE,    "image",    "Input image"    },
    { GIMP_PDB_DRAWABLE, "drawable", "Input drawable" }
  };

  gimp_install_procedure (PLUG_IN_PROC,
                          N_("Special effects that nobody understands"),
                          "No help yet",
                          "Tom Bech & Federico Mena Quintero",
                          "Tom Bech & Federico Mena Quintero",
                          "Version 0.14, September 24 1997",
                          N_("_Van Gogh (LIC)..."),
                          "RGB*",
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
  static GimpParam   values[1];
  GimpDrawable      *drawable;
  GimpRunMode        run_mode;
  GimpPDBStatusType  status = GIMP_PDB_SUCCESS;

  run_mode = param[0].data.d_int32;

  INIT_I18N ();

  *nreturn_vals = 1;
  *return_vals  = values;

  values[0].type          = GIMP_PDB_STATUS;
  values[0].data.d_status = status;

  /* Set default values */
  /* ================== */

  set_default_settings ();

  /* Possibly retrieve data */
  /* ====================== */

  gimp_get_data (PLUG_IN_PROC, &licvals);

  /* Get the specified drawable */
  /* ========================== */

  drawable = gimp_drawable_get (param[2].data.d_drawable);

  if (status == GIMP_PDB_SUCCESS)
    {
      /* Make sure that the drawable is RGBA or RGB color */
      /* ================================================ */

      if (gimp_drawable_is_rgb (drawable->drawable_id))
        {
          /* Set the tile cache size */
          /* ======================= */

          gimp_tile_cache_ntiles (2*(drawable->width / gimp_tile_width () + 1));

          switch (run_mode)
            {
              case GIMP_RUN_INTERACTIVE:
                if (create_main_dialog ())
                  compute_image (drawable);

                gimp_set_data (PLUG_IN_PROC, &licvals, sizeof (LicValues));
              break;
              case GIMP_RUN_WITH_LAST_VALS:
                compute_image (drawable);
                break;
              default:
                break;
            }
        }
      else
        status = GIMP_PDB_EXECUTION_ERROR;
    }

  values[0].data.d_status = status;
  gimp_drawable_detach (drawable);
}

const GimpPlugInInfo PLUG_IN_INFO =
{
  NULL,  /* init_proc  */
  NULL,  /* quit_proc  */
  query, /* query_proc */
  run,   /* run_proc   */
};

MAIN ()
