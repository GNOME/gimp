/*
 * This is a plugin for the GIMP.
 *
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 * Copyright (C) 1996 Torsten Martinsen
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
 * $Id$
 */

/*
 * This filter adds random noise to an image.
 * The amount of noise can be set individually for each RGB channel.
 * This filter does not operate on indexed images.
 *
 * May 2000 tim copperfield [timecop@japan.co.jp]
 * Added dynamic preview.
 *
 * alt@gimp.org. Fixed previews so they handle alpha channels correctly.
 */

#include "config.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <gtk/gtk.h>

#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

#include "libgimp/stdplugins-intl.h"

#include "gimpoldpreview.h"

#define SCALE_WIDTH      125
#define TILE_CACHE_SIZE  16

typedef struct
{
  gint    independent;
  gdouble noise[4];     /*  per channel  */
} NoisifyVals;

typedef struct
{
  gint       channels;
  GtkObject *channel_adj[4];
} NoisifyInterface;

/* Declare local functions.
 */
static void       query  (void);
static void       run    (const gchar      *name,
                          gint              nparams,
                          const GimpParam  *param,
                          gint             *nreturn_vals,
                          GimpParam       **return_vals);

static void       noisify (GimpDrawable    *drawable,
                           gboolean         preview_mode);
static gdouble    gauss   (GRand *gr);

static gint       noisify_dialog                   (GimpDrawable *drawable,
                                                    gint           channels);
static void       noisify_double_adjustment_update (GtkAdjustment *adjustment,
                                                    gpointer       data);

GimpPlugInInfo PLUG_IN_INFO =
{
  NULL,  /* init_proc */
  NULL,  /* quit_proc */
  query, /* query_proc */
  run,   /* run_proc */
};

static NoisifyVals nvals =
{
  TRUE,
  { 0.20, 0.20, 0.20, 0.20 }
};

static NoisifyInterface noise_int =
{
  0,
  { NULL, NULL, NULL, NULL }
};

static GimpRunMode     run_mode;
static GimpOldPreview *preview;

MAIN ()

static void
query (void)
{
  static GimpParamDef args[] =
  {
    { GIMP_PDB_INT32,    "run_mode",    "Interactive, non-interactive" },
    { GIMP_PDB_IMAGE,    "image",       "Input image (unused)" },
    { GIMP_PDB_DRAWABLE, "drawable",    "Input drawable" },
    { GIMP_PDB_INT32,    "independent", "Noise in channels independent" },
    { GIMP_PDB_FLOAT,    "noise_1",     "Noise in the first channel (red, gray)" },
    { GIMP_PDB_FLOAT,    "noise_2",     "Noise in the second channel (green, gray_alpha)" },
    { GIMP_PDB_FLOAT,    "noise_3",     "Noise in the third channel (blue)" },
    { GIMP_PDB_FLOAT,    "noise_4",     "Noise in the fourth channel (alpha)" }
  };

  gimp_install_procedure ("plug_in_noisify",
                          "Adds random noise to a drawable's channels",
                          "More here later",
                          "Torsten Martinsen",
                          "Torsten Martinsen",
                          "May 2000",
                          N_("_Noisify..."),
                          "RGB*, GRAY*",
                          GIMP_PLUGIN,
                          G_N_ELEMENTS (args), 0,
                          args, NULL);

  gimp_plugin_menu_register ("plug_in_noisify",
                             N_("<Image>/Filters/Noise"));
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
  GimpPDBStatusType  status = GIMP_PDB_SUCCESS;

  run_mode = param[0].data.d_int32;

  INIT_I18N ();

  *nreturn_vals = 1;
  *return_vals  = values;

  values[0].type          = GIMP_PDB_STATUS;
  values[0].data.d_status = status;

  /*  Get the specified drawable  */
  drawable = gimp_drawable_get (param[2].data.d_drawable);

  switch (run_mode)
    {
    case GIMP_RUN_INTERACTIVE:
      /*  Possibly retrieve data  */
      gimp_get_data ("plug_in_noisify", &nvals);

      /*  First acquire information with a dialog  */
      if (! noisify_dialog (drawable, drawable->bpp))
        {
          gimp_drawable_detach (drawable);
          return;
        }
      break;

    case GIMP_RUN_NONINTERACTIVE:
      /*  Make sure all the arguments are there!  */
      if (nparams != 8)
        {
          status = GIMP_PDB_CALLING_ERROR;
        }
      else
        {
          nvals.independent = param[3].data.d_int32 ? TRUE : FALSE;
          nvals.noise[0]    = param[4].data.d_float;
          nvals.noise[1]    = param[5].data.d_float;
          nvals.noise[2]    = param[6].data.d_float;
          nvals.noise[3]    = param[7].data.d_float;
        }
      break;

    case GIMP_RUN_WITH_LAST_VALS:
      /*  Possibly retrieve data  */
      gimp_get_data ("plug_in_noisify", &nvals);
      break;

    default:
      break;
    }

  /*  Make sure that the drawable is gray or RGB color  */
  if (gimp_drawable_is_rgb (drawable->drawable_id) ||
      gimp_drawable_is_gray (drawable->drawable_id))
    {
      gimp_progress_init (_("Adding Noise..."));
      gimp_tile_cache_ntiles (TILE_CACHE_SIZE);

      /*  compute the luminosity which exceeds the luminosity threshold  */
      noisify (drawable, FALSE);

      if (run_mode != GIMP_RUN_NONINTERACTIVE)
        gimp_displays_flush ();

      /*  Store data  */
      if (run_mode == GIMP_RUN_INTERACTIVE) {
        gimp_set_data ("plug_in_noisify", &nvals, sizeof (NoisifyVals));
      }
    }
  else
    {
      /* gimp_message ("blur: cannot operate on indexed color images"); ??? BLUR ??? */
      status = GIMP_PDB_EXECUTION_ERROR;
    }

  values[0].data.d_status = status;

  gimp_drawable_detach (drawable);
}

static void
noisify_func (const guchar *src,
              guchar       *dest,
              gint          bpp,
              gpointer      data)
{
  GRand *gr = (GRand*) data;
  gint noise = 0, b;

  if (!nvals.independent)
    noise = (gint) (nvals.noise[0] * gauss (gr) * 127);

  for (b = 0; b < bpp; b++)
    {
      if (nvals.noise[b] > 0.0)
        {
          gint p;

          if (nvals.independent)
            noise = (gint) (nvals.noise[b] * gauss (gr) * 127);

          p = src[b] + noise;
          dest[b] = CLAMP0255 (p);
        }
      else
        {
          dest[b] = src[b];
        }
    }
}

static void
noisify (GimpDrawable *drawable,
         gboolean      preview_mode)
{
  GRand *gr;

  gr = g_rand_new ();

  if (preview_mode)
    gimp_old_preview_update (preview, noisify_func, gr);
  else
    gimp_rgn_iterate2 (drawable, run_mode, noisify_func, gr);

  g_rand_free (gr);
}

static void
noisify_add_channel (GtkWidget *table, gint channel, gchar *name,
                     GimpDrawable *drawable)
{
  GtkObject *adj;

  adj = gimp_scale_entry_new (GTK_TABLE (table), 0, channel + 1,
                              name, SCALE_WIDTH, 0,
                              nvals.noise[channel], 0.0, 1.0, 0.01, 0.1, 2,
                              TRUE, 0, 0,
                              NULL, NULL);

  g_object_set_data (G_OBJECT (adj), "drawable", drawable);

  g_signal_connect (adj, "value_changed",
                    G_CALLBACK (noisify_double_adjustment_update),
                    &nvals.noise[channel]);

  noise_int.channel_adj[channel] = adj;
}

static gint
noisify_dialog (GimpDrawable *drawable,
                gint       channels)
{
  GtkWidget *dlg;
  GtkWidget *main_vbox;
  GtkWidget *toggle;
  GtkWidget *frame;
  GtkWidget *table;
  gboolean   run;

  gimp_ui_init ("noisify", FALSE);

  dlg = gimp_dialog_new (_("Noisify"), "noisify",
                         NULL, 0,
                         gimp_standard_help_func, "plug-in-noisify",

                         GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                         GTK_STOCK_OK,     GTK_RESPONSE_OK,

                         NULL);

  main_vbox = gtk_vbox_new (FALSE, 2);
  gtk_container_set_border_width (GTK_CONTAINER (main_vbox), 0);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dlg)->vbox), main_vbox, TRUE, TRUE, 0);
  gtk_widget_show (main_vbox);

  /* preview */
  preview = gimp_old_preview_new (NULL, TRUE);
  gtk_box_pack_start (GTK_BOX (main_vbox), preview->frame, FALSE, FALSE, 0);
  gtk_widget_show (preview->widget);
  gimp_old_preview_fill (preview, drawable);
  noisify (drawable, TRUE); /* preview noisify */

  /*  parameter settings  */
  frame = gtk_frame_new (_("Parameter Settings"));
  gtk_container_set_border_width (GTK_CONTAINER (frame), 6);
  gtk_box_pack_start (GTK_BOX (main_vbox), frame, TRUE, TRUE, 0);
  gtk_widget_show (frame);

  table = gtk_table_new (channels + 1, 3, FALSE);
  gtk_table_set_col_spacings (GTK_TABLE (table), 4);
  gtk_table_set_row_spacings (GTK_TABLE (table), 2);
  gtk_table_set_row_spacing (GTK_TABLE (table), 0, 4);
  gtk_container_set_border_width (GTK_CONTAINER (table), 4);
  gtk_container_add (GTK_CONTAINER (frame), table);
  gtk_widget_show (table);

  toggle = gtk_check_button_new_with_mnemonic (_("_Independent"));
  gtk_table_attach (GTK_TABLE (table), toggle, 0, 3, 0, 1, GTK_FILL, 0, 0, 0);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (toggle), nvals.independent);
  gtk_widget_show (toggle);

  g_signal_connect (toggle, "toggled",
                    G_CALLBACK (gimp_toggle_button_update),
                    &nvals.independent);

  noise_int.channels = channels;

  if (channels == 1)
    {
      noisify_add_channel (table, 0, _("_Gray:"), drawable);
    }
  else if (channels == 2)
    {
      noisify_add_channel (table, 0, _("_Gray:"), drawable);
      noisify_add_channel (table, 1, _("_Alpha:"), drawable);
    }
  else if (channels == 3)
    {
      noisify_add_channel (table, 0, _("_Red:"), drawable);
      noisify_add_channel (table, 1, _("_Green:"), drawable);
      noisify_add_channel (table, 2, _("_Blue:"), drawable);
    }

  else if (channels == 4)
    {
      noisify_add_channel (table, 0, _("_Red:"), drawable);
      noisify_add_channel (table, 1, _("_Green:"), drawable);
      noisify_add_channel (table, 2, _("_Blue:"), drawable);
      noisify_add_channel (table, 3, _("_Alpha:"), drawable);
    }
  else
    {
      gchar *buffer;
      gint   i;

      for (i = 0; i < channels; i++)
        {
          buffer = g_strdup_printf (_("Channel #%d:"), i);

          noisify_add_channel (table, i, buffer, drawable);

          g_free (buffer);
        }
    }

  gtk_widget_show (dlg);

  run = (gimp_dialog_run (GIMP_DIALOG (dlg)) == GTK_RESPONSE_OK);

  gtk_widget_destroy (dlg);

  return run;
}

/*
 * Return a Gaussian (aka normal) random variable.
 *
 * Adapted from ppmforge.c, which is part of PBMPLUS.
 * The algorithm comes from:
 * 'The Science Of Fractal Images'. Peitgen, H.-O., and Saupe, D. eds.
 * Springer Verlag, New York, 1988.
 *
 * It would probably be better to use another algorithm, such as that
 * in Knuth
 */
static gdouble
gauss (GRand *gr)
{
  gint i;
  gdouble sum = 0.0;

  for (i = 0; i < 4; i++)
    sum += g_rand_int_range (gr, 0, 0x7FFF);

  return sum * 5.28596089837e-5 - 3.46410161514;
}

static void
noisify_double_adjustment_update (GtkAdjustment *adjustment,
                                  gpointer       data)
{
  GimpDrawable *drawable;

  gimp_double_adjustment_update (adjustment, data);

  drawable = g_object_get_data (G_OBJECT (adjustment), "drawable");

  noisify (drawable, TRUE);

  if (! nvals.independent)
    {
      gint i;

      for (i = 0; i < noise_int.channels; i++)
        if (adjustment != GTK_ADJUSTMENT (noise_int.channel_adj[i]))
          gtk_adjustment_set_value (GTK_ADJUSTMENT (noise_int.channel_adj[i]),
                                    adjustment->value);
    }
}
