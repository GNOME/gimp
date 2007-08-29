/* scatter_hsv.c -- This is a plug-in for GIMP (1.0's API)
 * Author: Shuji Narazaki <narazaki@InetQ.or.jp>
 * Time-stamp: <2000-01-08 02:49:39 yasuhiro>
 * Version: 0.42
 *
 * Copyright (C) 1997 Shuji Narazaki <narazaki@InetQ.or.jp>
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


#define HSV_NOISE_PROC   "plug-in-hsv-noise"
#define SCATTER_HSV_PROC "plug-in-scatter-hsv"
#define PLUG_IN_BINARY   "scatter_hsv"
#define SCALE_WIDTH      100
#define ENTRY_WIDTH        3


static void     query               (void);
static void     run                 (const gchar      *name,
                                     gint              nparams,
                                     const GimpParam  *param,
                                     gint             *nreturn_vals,
                                     GimpParam       **return_vals);

static void     scatter_hsv         (GimpDrawable     *drawable);
static gboolean scatter_hsv_dialog  (GimpDrawable     *drawable);
static void     scatter_hsv_preview (GimpPreview      *preview);

static void     scatter_hsv_scatter (guchar           *r,
                                     guchar           *g,
                                     guchar           *b);

static gint     randomize_value     (gint              now,
                                     gint              min,
                                     gint              max,
                                     gboolean          wraps_around,
                                     gint              rand_max);


const GimpPlugInInfo PLUG_IN_INFO =
{
  NULL,  /* init_proc  */
  NULL,  /* quit_proc  */
  query, /* query_proc */
  run,   /* run_proc   */
};

typedef struct
{
  gint     holdness;
  gint     hue_distance;
  gint     saturation_distance;
  gint     value_distance;
  gboolean preview;
} ValueType;

static ValueType VALS =
{
  2,
  3,
  10,
  10,
  TRUE
};

MAIN ()

static void
query (void)
{
  static const GimpParamDef args [] =
  {
    { GIMP_PDB_INT32,    "run-mode",            "Interactive, non-interactive" },
    { GIMP_PDB_IMAGE,    "image",               "Input image (not used)" },
    { GIMP_PDB_DRAWABLE, "drawable",            "Input drawable" },
    { GIMP_PDB_INT32,    "holdness",            "convolution strength" },
    { GIMP_PDB_INT32,    "hue-distance",        "scattering of hue angle [0,180]" },
    { GIMP_PDB_INT32,    "saturation-distance", "distribution distance on saturation axis [0,255]" },
    { GIMP_PDB_INT32,    "value-distance",      "distribution distance on value axis [0,255]" }
  };

  gimp_install_procedure (HSV_NOISE_PROC,
                          N_("Randomize hue/saturation/value independently"),
                          "Scattering pixel values in HSV space",
                          "Shuji Narazaki (narazaki@InetQ.or.jp)",
                          "Shuji Narazaki",
                          "1997",
                          N_("HSV Noise..."),
                          "RGB*",
                          GIMP_PLUGIN,
                          G_N_ELEMENTS (args), 0,
                          args, NULL);

  gimp_plugin_menu_register (HSV_NOISE_PROC, "<Image>/Filters/Noise");

  gimp_install_procedure (SCATTER_HSV_PROC,
                          "Scattering pixel values in HSV space",
                          "Scattering pixel values in HSV space",
                          "Shuji Narazaki (narazaki@InetQ.or.jp)",
                          "Shuji Narazaki",
                          "1997",
                          NULL,
                          "RGB*",
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
  static GimpParam   values[1];
  GimpPDBStatusType  status = GIMP_PDB_SUCCESS;
  GimpRunMode        run_mode;
  GimpDrawable      *drawable;

  INIT_I18N ();

  run_mode = param[0].data.d_int32;
  drawable = gimp_drawable_get (param[2].data.d_int32);

  *nreturn_vals = 1;
  *return_vals  = values;

  values[0].type          = GIMP_PDB_STATUS;
  values[0].data.d_status = status;

  switch (run_mode)
    {
    case GIMP_RUN_INTERACTIVE:
      gimp_get_data (HSV_NOISE_PROC, &VALS);
      if (!gimp_drawable_is_rgb (drawable->drawable_id))
        {
          g_message (_("Can only operate on RGB drawables."));
          return;
        }
      if (! scatter_hsv_dialog (drawable))
        return;
      break;

    case GIMP_RUN_NONINTERACTIVE:
      VALS.holdness            = CLAMP (param[3].data.d_int32, 1, 8);
      VALS.hue_distance        = CLAMP (param[4].data.d_int32, 0, 180);
      VALS.saturation_distance = CLAMP (param[5].data.d_int32, 0, 255);
      VALS.value_distance      = CLAMP (param[6].data.d_int32, 0, 255);
      break;

    case GIMP_RUN_WITH_LAST_VALS:
      gimp_get_data (HSV_NOISE_PROC, &VALS);
      break;
    }

  scatter_hsv (drawable);

  if (run_mode != GIMP_RUN_NONINTERACTIVE)
    gimp_displays_flush();
  if (run_mode == GIMP_RUN_INTERACTIVE && status == GIMP_PDB_SUCCESS )
    gimp_set_data (HSV_NOISE_PROC, &VALS, sizeof (ValueType));

  values[0].type = GIMP_PDB_STATUS;
  values[0].data.d_status = status;
}

static void
scatter_hsv_func (const guchar *src,
                  guchar       *dest,
                  gint          bpp,
                  gpointer      data)
{
  guchar h, s, v;

  h = src[0];
  s = src[1];
  v = src[2];

  scatter_hsv_scatter (&h, &s, &v);

  dest[0] = h;
  dest[1] = s;
  dest[2] = v;

  if (bpp == 4)
    dest[3] = src[3];
}

static void
scatter_hsv (GimpDrawable *drawable)
{
  gimp_tile_cache_ntiles (2 * (drawable->width / gimp_tile_width () + 1));

  gimp_progress_init (_("HSV Noise"));

  gimp_rgn_iterate2 (drawable, 0 /* unused */, scatter_hsv_func, NULL);

  gimp_drawable_detach (drawable);
}

static gint
randomize_value (gint     now,
                 gint     min,
                 gint     max,
                 gboolean wraps_around,
                 gint     rand_max)
{
  gint    flag, new, steps, index;
  gdouble rand_val;

  steps = max - min + 1;
  rand_val = g_random_double ();
  for (index = 1; index < VALS.holdness; index++)
    {
      double tmp = g_random_double ();
      if (tmp < rand_val)
        rand_val = tmp;
    }

  if (g_random_double () < 0.5)
    flag = -1;
  else
    flag = 1;

  new = now + flag * ((int) (rand_max * rand_val) % steps);

  if (new < min)
    {
      if (wraps_around)
        new += steps;
      else
        new = min;
    }
  if (max < new)
    {
      if (wraps_around)
        new -= steps;
      else
        new = max;
    }
  return new;
}

static void
scatter_hsv_scatter (guchar *r,
                     guchar *g,
                     guchar *b)
{
  gint h, s, v;
  gint h1, s1, v1;
  gint h2, s2, v2;

  h = *r; s = *g; v = *b;

  gimp_rgb_to_hsv_int (&h, &s, &v);

  if (VALS.hue_distance > 0)
    h = randomize_value (h, 0, 359, TRUE,  VALS.hue_distance);

  if (VALS.saturation_distance > 0)
    s = randomize_value (s, 0, 255, FALSE, VALS.saturation_distance);

  if (VALS.value_distance > 0)
    v = randomize_value (v, 0, 255, FALSE, VALS.value_distance);

  h1 = h; s1 = s; v1 = v;

  gimp_hsv_to_rgb_int (&h, &s, &v); /* don't believe ! */

  h2 = h; s2 = s; v2 = v;

  gimp_rgb_to_hsv_int (&h2, &s2, &v2); /* h2 should be h1. But... */

  if ((abs (h1 - h2) <= VALS.hue_distance)        &&
      (abs (s1 - s2) <= VALS.saturation_distance) &&
      (abs (v1 - v2) <= VALS.value_distance))
    {
      *r = h;
      *g = s;
      *b = v;
    }
}

static void
scatter_hsv_preview (GimpPreview *preview)
{
  GimpDrawable *drawable;
  GimpPixelRgn  src_rgn;
  guchar       *src, *dst;
  gint          i;
  gint          x1, y1;
  gint          width, height;
  gint          bpp;

  drawable =
    gimp_drawable_preview_get_drawable (GIMP_DRAWABLE_PREVIEW (preview));

  gimp_preview_get_position (preview, &x1, &y1);
  gimp_preview_get_size (preview, &width, &height);

  bpp = drawable->bpp;

  src = g_new (guchar, width * height * bpp);
  dst = g_new (guchar, width * height * bpp);

  gimp_pixel_rgn_init (&src_rgn, drawable,
                       x1, y1, width, height,
                       FALSE, FALSE);
  gimp_pixel_rgn_get_rect (&src_rgn, src, x1, y1, width, height);

  for (i = 0; i < width * height; i++)
    scatter_hsv_func (src + i * bpp, dst + i * bpp, bpp, NULL);

  gimp_preview_draw_buffer (preview, dst, width * bpp);

  g_free (src);
  g_free (dst);
}


/* dialog stuff */

static gboolean
scatter_hsv_dialog (GimpDrawable *drawable)
{
  GtkWidget *dialog;
  GtkWidget *main_vbox;
  GtkWidget *preview;
  GtkWidget *table;
  GtkObject *adj;
  gboolean   run;

  gimp_ui_init (PLUG_IN_BINARY, TRUE);

  dialog = gimp_dialog_new (_("Scatter HSV"), PLUG_IN_BINARY,
                            NULL, 0,
                            gimp_standard_help_func, HSV_NOISE_PROC,

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

  preview = gimp_drawable_preview_new (drawable, &VALS.preview);
  gtk_box_pack_start_defaults (GTK_BOX (main_vbox), preview);
  gtk_widget_show (preview);

  g_signal_connect (preview, "invalidated",
                    G_CALLBACK (scatter_hsv_preview),
                    NULL);

  table = gtk_table_new (4, 3, FALSE);
  gtk_table_set_col_spacings (GTK_TABLE (table), 6);
  gtk_table_set_row_spacings (GTK_TABLE (table), 6);
  gtk_box_pack_start (GTK_BOX (main_vbox), table, FALSE, FALSE, 0);
  gtk_widget_show (table);

  adj = gimp_scale_entry_new (GTK_TABLE (table), 0, 0,
                              _("_Holdness:"), SCALE_WIDTH, ENTRY_WIDTH,
                              VALS.holdness, 1, 8, 1, 2, 0,
                              TRUE, 0, 0,
                              NULL, NULL);
  g_signal_connect (adj, "value-changed",
                    G_CALLBACK (gimp_int_adjustment_update),
                    &VALS.holdness);
  g_signal_connect_swapped (adj, "value-changed",
                            G_CALLBACK (gimp_preview_invalidate),
                            preview);

  adj = gimp_scale_entry_new (GTK_TABLE (table), 0, 1,
                              _("H_ue:"), SCALE_WIDTH, ENTRY_WIDTH,
                              VALS.hue_distance, 0, 180, 1, 6, 0,
                              TRUE, 0, 0,
                              NULL, NULL);
  g_signal_connect (adj, "value-changed",
                    G_CALLBACK (gimp_int_adjustment_update),
                    &VALS.hue_distance);
  g_signal_connect_swapped (adj, "value-changed",
                            G_CALLBACK (gimp_preview_invalidate),
                            preview);

  adj = gimp_scale_entry_new (GTK_TABLE (table), 0, 2,
                              _("_Saturation:"), SCALE_WIDTH, ENTRY_WIDTH,
                              VALS.saturation_distance, 0, 255, 1, 8, 0,
                              TRUE, 0, 0,
                              NULL, NULL);
  g_signal_connect (adj, "value-changed",
                    G_CALLBACK (gimp_int_adjustment_update),
                    &VALS.saturation_distance);
  g_signal_connect_swapped (adj, "value-changed",
                            G_CALLBACK (gimp_preview_invalidate),
                            preview);

  adj = gimp_scale_entry_new (GTK_TABLE (table), 0, 3,
                              _("_Value:"), SCALE_WIDTH, ENTRY_WIDTH,
                              VALS.value_distance, 0, 255, 1, 8, 0,
                              TRUE, 0, 0,
                              NULL, NULL);
  g_signal_connect (adj, "value-changed",
                    G_CALLBACK (gimp_int_adjustment_update),
                    &VALS.value_distance);
  g_signal_connect_swapped (adj, "value-changed",
                            G_CALLBACK (gimp_preview_invalidate),
                            preview);

  gtk_widget_show (dialog);

  run = (gimp_dialog_run (GIMP_DIALOG (dialog)) == GTK_RESPONSE_OK);

  gtk_widget_destroy (dialog);

  return run;
}
