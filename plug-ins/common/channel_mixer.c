/****************************************************************************
 * This is a plugin for the GIMP v 2.0 or later.
 *
 * Copyright (C) 2002 Martin Guldahl <mguldahl@xmission.com>
 * Based on GTK code from:
 *    homomorphic (Copyright (C) 2001 Valter Marcus Hilden)
 *    rand-noted  (Copyright (C) 1998 Miles O'Neal)
 *    nlfilt      (Copyright (C) 1997 Eric L. Hernes)
 *    pagecurl    (Copyright (C) 1996 Federico Mena Quintero)
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
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 ****************************************************************************/

#include "config.h"

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <gtk/gtk.h>

#ifdef G_OS_WIN32
#include <libgimpbase/gimpwin32-io.h>
#endif

#include <libgimpmath/gimpmath.h>
#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

#include "libgimp/stdplugins-intl.h"


#define PLUG_IN_NAME        "plug_in_colors_channel_mixer"
#define PLUG_IN_VERSION     "Channel Mixer 0.8"
#define HELP_ID             "plug-in-colors-channel-mixer"
#define PROGRESS_UPDATE_NUM 20
#define CM_LINE_SIZE        1024

typedef enum
{
  CM_RED_CHANNEL,
  CM_GREEN_CHANNEL,
  CM_BLUE_CHANNEL
} CmModeType;

typedef struct
{
  gdouble red_gain;
  gdouble green_gain;
  gdouble blue_gain;
} CmChannelType;

typedef struct
{
  CmChannelType  red;
  CmChannelType  green;
  CmChannelType  blue;
  CmChannelType  black;

  gboolean       monochrome_flag;
  gboolean       preserve_luminosity_flag;

  CmModeType     output_channel;
  gboolean       preview;

  GtkAdjustment *red_data;
  GtkAdjustment *green_data;
  GtkAdjustment *blue_data;

  GtkWidget     *combo;

  CmModeType     old_output_channel;

  GtkWidget     *monochrome_toggle;
  GtkWidget     *preserve_luminosity_toggle;
} CmParamsType;


static GtkWidget *preview;

static void     query (void);
static void     run   (const gchar      *name,
                       gint              nparams,
                       const GimpParam  *param,
                       gint             *nreturn_vals,
                       GimpParam       **return_vals);

static void     channel_mixer (GimpDrawable *drawable);
static gboolean cm_dialog     (GimpDrawable *drawable);

static void cm_red_scale_callback           (GtkAdjustment    *adjustment,
                                             CmParamsType     *mix);
static void cm_green_scale_callback         (GtkAdjustment    *adjustment,
                                             CmParamsType     *mix);
static void cm_blue_scale_callback          (GtkAdjustment    *adjustment,
                                             CmParamsType     *mix);
static void cm_monochrome_callback          (GtkWidget        *widget,
                                             CmParamsType     *mix);
static void cm_preserve_luminosity_callback (GtkWidget        *widget,
                                             CmParamsType     *mix);
static void cm_load_file_callback           (GtkWidget        *widget,
                                             CmParamsType     *mix);
static void cm_load_file_response_callback  (GtkWidget        *dialog,
                                             gint              response_id,
                                             CmParamsType     *mix);
static void cm_save_file_callback           (GtkWidget        *widget,
                                             CmParamsType     *mix);
static void cm_save_file_response_callback  (GtkWidget        *dialog,
                                             gint              response_id,
                                             CmParamsType     *mix);
static void cm_combo_callback               (GtkWidget        *widget,
                                             CmParamsType     *mix);

static gboolean cm_force_overwrite (const gchar *filename,
                                    GtkWidget   *parent);

static gdouble cm_calculate_norm (CmParamsType  *mix,
                                  CmChannelType *ch);

static inline guchar cm_mix_pixel (CmChannelType *ch,
                                   guchar         r,
                                   guchar         g,
                                   guchar         b,
                                   gdouble        norm);

static void cm_preview       (CmParamsType *mix,
                              GimpPreview  *preview);
static void cm_set_adjusters (CmParamsType *mix);

static void cm_save_file (CmParamsType *mix,
                          FILE         *fp);


GimpPlugInInfo PLUG_IN_INFO =
{
  NULL,  /* init_proc  */
  NULL,  /* quit_proc  */
  query, /* query_proc */
  run    /* run_proc   */
};

static CmParamsType mix =
{
  { 1.0, 0.0, 0.0 },
  { 0.0, 1.0, 0.0 },
  { 0.0, 0.0, 1.0 },
  { 1.0, 0.0, 0.0 },
  FALSE,
  FALSE,
  CM_RED_CHANNEL,
  TRUE
};

MAIN ()

static void
query (void)
{
  static GimpParamDef args[] =
  {
    { GIMP_PDB_INT32, "run_mode", "Interactive, non-interactive" },
    { GIMP_PDB_IMAGE, "image", "Input image (unused)" },
    { GIMP_PDB_DRAWABLE, "drawable", "Input drawable" },
    { GIMP_PDB_INT32, "monochrome", "Monochrome (TRUE or FALSE)" },
    { GIMP_PDB_FLOAT, "rr_gain", "Set the red gain for the red channel" },
    { GIMP_PDB_FLOAT, "rg_gain", "Set the green gain for the red channel" },
    { GIMP_PDB_FLOAT, "rb_gain", "Set the blue gain for the red channel" },
    { GIMP_PDB_FLOAT, "gr_gain", "Set the red gain for the green channel" },
    { GIMP_PDB_FLOAT, "gg_gain", "Set the green gain for the green channel" },
    { GIMP_PDB_FLOAT, "gb_gain", "Set the blue gain for the green channel" },
    { GIMP_PDB_FLOAT, "br_gain", "Set the red gain for the blue channel" },
    { GIMP_PDB_FLOAT, "bg_gain", "Set the green gain for the blue channel" },
    { GIMP_PDB_FLOAT, "bb_gain", "Set the blue gain for the blue channel" }
  };

  gimp_install_procedure (PLUG_IN_NAME,
                          "Mix RGB Channels.",
                          "This plug-in mixes the RGB channels.",
                          "Martin Guldahl <mguldahl@xmission.com>",
                          "Martin Guldahl <mguldahl@xmission.com>",
                          "2002",
                          N_("Channel Mi_xer..."),
                          "RGB*",
                          GIMP_PLUGIN,
                          G_N_ELEMENTS (args), 0,
                          args, NULL);

  gimp_plugin_menu_register (PLUG_IN_NAME, "<Image>/Filters/Colors");
}

/*----------------------------------------------------------------------
 *  run() - main routine
 *
 *  This handles the main interaction with the GIMP itself,
 *  and invokes the routine that actually does the work.
 *--------------------------------------------------------------------*/
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

  drawable = gimp_drawable_get (param[2].data.d_drawable);

  if (gimp_drawable_is_rgb (drawable->drawable_id))
    {
      switch (run_mode)
        {
        case GIMP_RUN_INTERACTIVE:
          gimp_get_data (PLUG_IN_NAME, &mix);

          if (! cm_dialog (drawable))
            {
              gimp_drawable_detach (drawable);
              return;
            }

          break;

        case GIMP_RUN_NONINTERACTIVE:
          mix.monochrome_flag = param[3].data.d_int32;

          if (mix.monochrome_flag == 1)
            {
              mix.black.red_gain   = param[4].data.d_float;
              mix.black.green_gain = param[5].data.d_float;
              mix.black.blue_gain  = param[6].data.d_float;
            }
          else
            {
              mix.red.red_gain     = param[4].data.d_float;
              mix.red.green_gain   = param[5].data.d_float;
              mix.red.blue_gain    = param[6].data.d_float;
              mix.green.red_gain   = param[7].data.d_float;
              mix.green.green_gain = param[8].data.d_float;
              mix.green.blue_gain  = param[9].data.d_float;
              mix.blue.red_gain    = param[10].data.d_float;
              mix.blue.green_gain  = param[11].data.d_float;
              mix.blue.blue_gain   = param[12].data.d_float;
            }
          break;

        case GIMP_RUN_WITH_LAST_VALS:
          gimp_get_data (PLUG_IN_NAME, &mix);
          break;

        default:
          break;
        }

      if (status == GIMP_PDB_SUCCESS)
        {
          /* printf("Channel Mixer:: Mode:%d  r %f  g %f  b %f\n ",
                 param[3].data.d_int32, mix.black.red_gain,
                 mix.black.green_gain, mix.black.blue_gain); */

          gimp_progress_init (_(PLUG_IN_VERSION));

          channel_mixer (drawable);

          if (run_mode != GIMP_RUN_NONINTERACTIVE)
            gimp_displays_flush ();

          if (run_mode == GIMP_RUN_INTERACTIVE)
            gimp_set_data (PLUG_IN_NAME, &mix, sizeof (CmParamsType));
        }
    }
  else
    {
      status = GIMP_PDB_EXECUTION_ERROR;
    }

  values[0].data.d_status = status;

  gimp_drawable_detach (drawable);
}

static gdouble
cm_calculate_norm (CmParamsType  *mix,
                   CmChannelType *ch)
{
  gdouble sum;

  sum = ch->red_gain + ch->green_gain + ch->blue_gain;

  if (sum == 0.0 || mix->preserve_luminosity_flag == FALSE)
    return 1.0;

  return fabs (1 / sum);
}

static inline guchar
cm_mix_pixel (CmChannelType *ch,
              guchar         r,
              guchar         g,
              guchar         b,
              gdouble        norm)
{
  gdouble c = ch->red_gain * r + ch->green_gain * g + ch->blue_gain * b;

  c *= norm;

  return (guchar) CLAMP0255 (c);
}

static void
channel_mixer (GimpDrawable *drawable)
{
  GimpPixelRgn  src_rgn, dest_rgn;
  guchar       *src, *dest;
  gpointer      pr;
  gint          x, y;
  gint          total, processed = 0;
  gboolean      has_alpha;
  gdouble       red_norm, green_norm, blue_norm, black_norm;
  gint          x1, y1, x2, y2;
  gint          width, height;

  gimp_drawable_mask_bounds (drawable->drawable_id,
                             &x1, &y1, &x2, &y2);

  width  = x2 - x1;
  height = y2 - y1;

  has_alpha = gimp_drawable_has_alpha (drawable->drawable_id);

  total = width * height;

  gimp_tile_cache_ntiles (2 * (drawable->width / gimp_tile_width () + 1));

  gimp_pixel_rgn_init (&src_rgn, drawable,
                       x1, y1, width, height, FALSE, FALSE);
  gimp_pixel_rgn_init (&dest_rgn, drawable,
                       x1, y1, width, height, TRUE, TRUE);

  red_norm   = cm_calculate_norm (&mix, &mix.red);
  green_norm = cm_calculate_norm (&mix, &mix.green);
  blue_norm  = cm_calculate_norm (&mix, &mix.blue);
  black_norm = cm_calculate_norm (&mix, &mix.black);

  for (pr = gimp_pixel_rgns_register (2, &src_rgn, &dest_rgn);
       pr != NULL; pr = gimp_pixel_rgns_process (pr))
    {
      gint offset;

      for (y = 0; y < src_rgn.h; y++)
        {
          src  = src_rgn.data  + y * src_rgn.rowstride;
          dest = dest_rgn.data + y * dest_rgn.rowstride;

          offset = 0;

          for (x = 0; x < src_rgn.w; x++)
            {
              guchar r, g, b;

              r = src[offset + 0];
              g = src[offset + 1];
              b = src[offset + 2];

              if (mix.monochrome_flag == TRUE)
                {
                  dest[offset + 0] =
                  dest[offset + 1] =
                  dest[offset + 2] =
                    cm_mix_pixel (&mix.black, r, g, b, black_norm);
                }
              else
                {
                  dest[offset + 0] =
                    cm_mix_pixel (&mix.red, r, g, b, red_norm);
                  dest[offset + 1] =
                    cm_mix_pixel (&mix.green, r, g, b, green_norm);
                  dest[offset + 2] =
                    cm_mix_pixel (&mix.blue, r, g, b, blue_norm);
                }

              offset += 3;

              if (has_alpha)
                {
                  dest[offset] = src[offset];
                  offset++;
                }

              /* update progress */
              if ((++processed % (total / PROGRESS_UPDATE_NUM + 1)) == 0)
                gimp_progress_update ((gdouble) processed / (gdouble) total);
            }
        }
    }

  gimp_progress_update (1.0);

  gimp_drawable_flush (drawable);
  gimp_drawable_merge_shadow (drawable->drawable_id, TRUE);
  gimp_drawable_update (drawable->drawable_id,
                        x1, y1, width, height);
}

static gboolean
cm_dialog (GimpDrawable *drawable)
{
  GtkWidget *dialog;
  GtkWidget *main_vbox;
  GtkWidget *frame;
  GtkWidget *hbox;
  GtkWidget *button;
  GtkWidget *label;
  GtkWidget *image;
  GtkWidget *table;
  gdouble    red_value, green_value, blue_value;
  gboolean   run;

  gimp_ui_init ("channel_mixer", FALSE);

  /* get values */
  if (mix.monochrome_flag == TRUE)
    {
      red_value   = mix.black.red_gain   * 100;
      green_value = mix.black.green_gain * 100;
      blue_value  = mix.black.blue_gain  * 100;
    }
  else
    {
      switch (mix.output_channel)
        {
        case CM_RED_CHANNEL:
          red_value   = mix.red.red_gain   * 100;
          green_value = mix.red.green_gain * 100;
          blue_value  = mix.red.blue_gain  * 100;
          break;

        case CM_GREEN_CHANNEL:
          red_value   = mix.green.red_gain   * 100;
          green_value = mix.green.green_gain * 100;
          blue_value  = mix.green.blue_gain  * 100;
          break;

        case CM_BLUE_CHANNEL:
          red_value   = mix.blue.red_gain   * 100;
          green_value = mix.blue.green_gain * 100;
          blue_value  = mix.blue.blue_gain  * 100;
          break;

        default:
          g_assert_not_reached ();

          red_value = green_value = blue_value = 0.0;
          break;
        }
    }

  dialog = gimp_dialog_new (_("Channel Mixer"), "mixer",
                            NULL, 0,
                            gimp_standard_help_func, HELP_ID,

                            GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                            GTK_STOCK_OK,     GTK_RESPONSE_OK,

                            NULL);

  main_vbox = gtk_vbox_new (FALSE, 12);
  gtk_container_set_border_width (GTK_CONTAINER (main_vbox), 12);
  gtk_container_add (GTK_CONTAINER (GTK_DIALOG (dialog)->vbox), main_vbox);
  gtk_widget_show (main_vbox);

  preview = gimp_aspect_preview_new (drawable, &mix.preview);
  gtk_box_pack_start_defaults (GTK_BOX (main_vbox), preview);
  gtk_widget_show (preview);
  g_signal_connect_swapped (preview, "invalidated",
                            G_CALLBACK (cm_preview),
                            &mix);

  frame = gimp_frame_new (NULL);
  gtk_box_pack_start (GTK_BOX (main_vbox), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  hbox = gtk_hbox_new (FALSE, 6);
  gtk_frame_set_label_widget (GTK_FRAME (frame), hbox);
  gtk_widget_show (hbox);

  label = gtk_label_new_with_mnemonic (_("O_utput channel:"));

  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
  gtk_widget_show (label);

  mix.combo = gimp_int_combo_box_new (NULL, 0);

  gimp_int_combo_box_append (GIMP_INT_COMBO_BOX (mix.combo),
                             GIMP_INT_STORE_VALUE,    CM_RED_CHANNEL,
                             GIMP_INT_STORE_LABEL,    _("Red"),
                             GIMP_INT_STORE_STOCK_ID, GIMP_STOCK_CHANNEL_RED,
                             -1);
  gimp_int_combo_box_append (GIMP_INT_COMBO_BOX (mix.combo),
                             GIMP_INT_STORE_VALUE,    CM_GREEN_CHANNEL,
                             GIMP_INT_STORE_LABEL,    _("Green"),
                             GIMP_INT_STORE_STOCK_ID, GIMP_STOCK_CHANNEL_GREEN,
                             -1);
  gimp_int_combo_box_append (GIMP_INT_COMBO_BOX (mix.combo),
                             GIMP_INT_STORE_VALUE,    CM_BLUE_CHANNEL,
                             GIMP_INT_STORE_LABEL,    _("Blue"),
                             GIMP_INT_STORE_STOCK_ID, GIMP_STOCK_CHANNEL_BLUE,
                             -1);

  gimp_int_combo_box_set_active (GIMP_INT_COMBO_BOX (mix.combo),
                                 mix.output_channel);

  g_signal_connect (mix.combo, "changed",
                    G_CALLBACK (cm_combo_callback),
                    &mix);

  gtk_box_pack_start (GTK_BOX (hbox), mix.combo, TRUE, TRUE, 0);
  gtk_widget_show (mix.combo);

  if (mix.monochrome_flag)
    gtk_widget_set_sensitive (mix.combo, FALSE);

  gtk_label_set_mnemonic_widget (GTK_LABEL (label), mix.combo);

  /*........................................................... */

  table = gtk_table_new (3, 4, FALSE);
  gtk_table_set_row_spacings (GTK_TABLE (table), 6);
  gtk_table_set_col_spacings (GTK_TABLE (table), 6);
  gtk_container_add (GTK_CONTAINER (frame), table);
  gtk_widget_show (table);

  image = gtk_image_new_from_stock (GIMP_STOCK_CHANNEL_RED,
                                    GTK_ICON_SIZE_BUTTON);
  gtk_table_attach (GTK_TABLE (table), image,
                    0, 1, 0, 1, GTK_FILL, GTK_FILL, 0, 0);
  gtk_widget_show (image);

  mix.red_data =
    GTK_ADJUSTMENT (gimp_scale_entry_new (GTK_TABLE (table), 1, 0,
                                          _("_Red:"), 150, -1,
                                          red_value, -200.0, 200.0,
                                          1.0, 10.0, 1,
                                          TRUE, 0.0, 0.0,
                                          NULL, NULL));

  g_signal_connect (mix.red_data, "value_changed",
                    G_CALLBACK (cm_red_scale_callback),
                    &mix);

  image = gtk_image_new_from_stock (GIMP_STOCK_CHANNEL_GREEN,
                                    GTK_ICON_SIZE_BUTTON);
  gtk_table_attach (GTK_TABLE (table), image,
                    0, 1, 1, 2, GTK_FILL, GTK_FILL, 0, 0);
  gtk_widget_show (image);

  mix.green_data =
    GTK_ADJUSTMENT (gimp_scale_entry_new (GTK_TABLE (table), 1, 1,
                                          _("_Green:"), 150, -1,
                                          green_value, -200.0, 200.0,
                                          1.0, 10.0, 1,
                                          TRUE, 0.0, 0.0,
                                          NULL, NULL));

  g_signal_connect (mix.green_data, "value_changed",
                    G_CALLBACK (cm_green_scale_callback),
                    &mix);


  image = gtk_image_new_from_stock (GIMP_STOCK_CHANNEL_BLUE,
                                    GTK_ICON_SIZE_BUTTON);
  gtk_table_attach (GTK_TABLE (table), image,
                    0, 1, 2, 3, GTK_FILL, GTK_FILL, 0, 0);
  gtk_widget_show (image);

  mix.blue_data =
    GTK_ADJUSTMENT (gimp_scale_entry_new (GTK_TABLE (table), 1, 2,
                                          _("_Blue:"), 150, -1,
                                          blue_value, -200.0, 200.0,
                                          1.0, 10.0, 1,
                                          TRUE, 0.0, 0.0,
                                          NULL, NULL));

  g_signal_connect (mix.blue_data, "value_changed",
                    G_CALLBACK (cm_blue_scale_callback),
                    &mix);

  /*  The monochrome toggle  */
  mix.monochrome_toggle = gtk_check_button_new_with_mnemonic (_("_Monochrome"));
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (mix.monochrome_toggle),
                                mix.monochrome_flag);
  gtk_box_pack_start (GTK_BOX (main_vbox), mix.monochrome_toggle, FALSE, FALSE, 0);
  gtk_widget_show (mix.monochrome_toggle);

  g_signal_connect (mix.monochrome_toggle, "toggled",
                    G_CALLBACK (cm_monochrome_callback),
                    &mix);

  /*  The preserve luminosity toggle  */
  mix.preserve_luminosity_toggle =
    gtk_check_button_new_with_mnemonic (_("Preserve _luminosity"));
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON
                                (mix.preserve_luminosity_toggle),
                                mix.preserve_luminosity_flag);
  gtk_box_pack_start (GTK_BOX (main_vbox), mix.preserve_luminosity_toggle,
                      FALSE, FALSE, 0);
  gtk_widget_show (mix.preserve_luminosity_toggle);

  g_signal_connect (mix.preserve_luminosity_toggle, "toggled",
                    G_CALLBACK (cm_preserve_luminosity_callback),
                    &mix);

  /*........................................................... */
  /*  Horizontal box for file i/o  */
  hbox = gtk_hbox_new (FALSE, 6);
  gtk_box_pack_end (GTK_BOX (main_vbox), hbox, FALSE, FALSE, 0);
  gtk_widget_show (hbox);

  button = gtk_button_new_from_stock (GTK_STOCK_OPEN);
  gtk_container_add (GTK_CONTAINER (hbox), button);
  gtk_widget_show (button);

  g_signal_connect (button, "clicked",
                    G_CALLBACK (cm_load_file_callback),
                    &mix);

  button = gtk_button_new_from_stock (GTK_STOCK_SAVE);
  gtk_container_add (GTK_CONTAINER (hbox), button);
  gtk_widget_show (button);

  g_signal_connect (button, "clicked",
                    G_CALLBACK (cm_save_file_callback),
                    &mix);

  gtk_widget_show (dialog);

  run = (gimp_dialog_run (GIMP_DIALOG (dialog)) == GTK_RESPONSE_OK);

  gtk_widget_destroy (dialog);

  return run;
}

static void
cm_red_scale_callback (GtkAdjustment *adjustment,
                       CmParamsType  *mix)
{
  if (mix->monochrome_flag == TRUE)
    mix->black.red_gain = adjustment->value / 100.0;
  else
    switch (mix->output_channel)
      {
      case CM_RED_CHANNEL:
        mix->red.red_gain = adjustment->value / 100.0;
        break;
      case CM_GREEN_CHANNEL:
        mix->green.red_gain = adjustment->value / 100.0;
        break;
      case CM_BLUE_CHANNEL:
        mix->blue.red_gain = adjustment->value / 100.0;
        break;
      }

  gimp_preview_invalidate (GIMP_PREVIEW (preview));
}

static void
cm_green_scale_callback (GtkAdjustment *adjustment,
                         CmParamsType  *mix)
{
  if (mix->monochrome_flag == TRUE)
    mix->black.green_gain = adjustment->value / 100.0;
  else
    switch (mix->output_channel)
      {
      case CM_RED_CHANNEL:
        mix->red.green_gain = adjustment->value / 100.0;
        break;
      case CM_GREEN_CHANNEL:
        mix->green.green_gain = adjustment->value / 100.0;
        break;
      case CM_BLUE_CHANNEL:
        mix->blue.green_gain = adjustment->value / 100.0;
        break;
      }

  gimp_preview_invalidate (GIMP_PREVIEW (preview));
}

static void
cm_blue_scale_callback (GtkAdjustment *adjustment,
                        CmParamsType  *mix)
{
  if (mix->monochrome_flag == TRUE)
    mix->black.blue_gain = adjustment->value / 100.0;
  else
    switch (mix->output_channel)
      {
      case CM_RED_CHANNEL:
        mix->red.blue_gain = adjustment->value / 100.0;
        break;
      case CM_GREEN_CHANNEL:
        mix->green.blue_gain = adjustment->value / 100.0;
        break;
      case CM_BLUE_CHANNEL:
        mix->blue.blue_gain = adjustment->value / 100.0;
        break;
      }

  gimp_preview_invalidate (GIMP_PREVIEW (preview));
}

static void
cm_preview (CmParamsType *mix,
            GimpPreview  *preview)
{
  guchar       *dst, *src;
  gint          x, y;
  gint          offset, rowsize;
  gdouble       red_norm, green_norm, blue_norm, black_norm;
  gint          width, height, bpp;
  GimpDrawable *drawable = GIMP_ASPECT_PREVIEW (preview)->drawable;

  red_norm   = cm_calculate_norm (mix, &mix->red);
  green_norm = cm_calculate_norm (mix, &mix->green);
  blue_norm  = cm_calculate_norm (mix, &mix->blue);
  black_norm = cm_calculate_norm (mix, &mix->black);

  gimp_preview_get_size (GIMP_PREVIEW (preview), &width, &height);
  bpp = gimp_drawable_bpp (drawable->drawable_id);
  src = gimp_drawable_get_thumbnail_data (drawable->drawable_id,
                                          &width, &height, &bpp);

  rowsize = width * bpp;
  dst = g_new (guchar, rowsize * height);

  offset = 0;
  for (y = 0; y < height; y++)
    {
      for (x = 0; x < width; x++)
        {
          guchar r, g, b;

          r = src[offset + 0];
          g = src[offset + 1];
          b = src[offset + 2];

          if (mix->monochrome_flag == TRUE)
            {
              dst[offset + 0] =
              dst[offset + 1] =
              dst[offset + 2] =
                cm_mix_pixel (&mix->black, r, g, b, black_norm);
            }
          else
            {
              dst[offset + 0] =
                cm_mix_pixel (&mix->red, r, g, b, red_norm);
              dst[offset + 1] =
                cm_mix_pixel (&mix->green, r, g, b, green_norm);
              dst[offset + 2] =
                cm_mix_pixel (&mix->blue, r, g, b, blue_norm);
            }
          if (bpp == 4)
            dst[offset + 3] = src[offset + 3];

          offset += bpp;
        }
    }
  gimp_preview_draw_buffer (GIMP_PREVIEW (preview), dst, bpp * width);

  g_free (dst);
}

static void
cm_monochrome_callback (GtkWidget    *widget,
                        CmParamsType *mix)
{
  if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (widget)))
    {
      mix->old_output_channel = mix->output_channel;
      mix->monochrome_flag = TRUE;
      gtk_widget_set_sensitive (mix->combo, FALSE);
    }
  else
    {
      mix->output_channel = mix->old_output_channel;
      mix->monochrome_flag = FALSE;
      gtk_widget_set_sensitive (mix->combo, TRUE);
    }

  cm_set_adjusters (mix);

  gimp_preview_invalidate (GIMP_PREVIEW (preview));
}

static void
cm_preserve_luminosity_callback (GtkWidget    *widget,
                                 CmParamsType *mix)
{
  if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (widget)))
    mix->preserve_luminosity_flag = TRUE;
  else
    mix->preserve_luminosity_flag = FALSE;

  gimp_preview_invalidate (GIMP_PREVIEW (preview));
}

static gchar *
cm_settings_filename (void)
{
  return g_build_filename (gimp_directory (),
                           "channel-mixer",
                           "settings",
                           NULL);
}

static void
cm_load_file_callback (GtkWidget    *widget,
                       CmParamsType *mix)
{
  static GtkWidget *dialog = NULL;

  if (! dialog)
    {
      GtkWidget *parent = gtk_widget_get_toplevel (widget);
      gchar     *name;

      dialog =
        gtk_file_chooser_dialog_new (_("Load Channel Mixer Settings"),
                                     GTK_WINDOW (parent),
                                     GTK_FILE_CHOOSER_ACTION_OPEN,

                                     GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                                     GTK_STOCK_OPEN,   GTK_RESPONSE_OK,

                                     NULL);

      gtk_dialog_set_default_response (GTK_DIALOG (dialog), GTK_RESPONSE_OK);

      g_signal_connect (dialog, "response",
                        G_CALLBACK (cm_load_file_response_callback),
                        mix);
      g_signal_connect (dialog, "delete_event",
                        G_CALLBACK (gtk_true),
                        NULL);

      name = cm_settings_filename ();
      gtk_file_chooser_set_filename (GTK_FILE_CHOOSER (dialog), name);
      g_free (name);
    }

  gtk_window_present (GTK_WINDOW (dialog));
}

static void
cm_load_file_response_callback (GtkWidget    *dialog,
                                gint          response_id,
                                CmParamsType *mix)
{
  FILE *fp;

  if (response_id == GTK_RESPONSE_OK)
    {
      gchar *filename;

      filename = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (dialog));

      fp = fopen (filename, "r");

      if (fp)
        {
          gchar buf[3][CM_LINE_SIZE];

          buf[0][0] = '\0';
          buf[1][0] = '\0';
          buf[2][0] = '\0';

          fgets (buf[0], CM_LINE_SIZE - 1, fp);

          fscanf (fp, "%*s %s", buf[0]);
          if (strcmp (buf[0], "RED") == 0)
            mix->output_channel = CM_RED_CHANNEL;
          else if (strcmp (buf[0], "GREEN") == 0)
            mix->output_channel = CM_GREEN_CHANNEL;
          else if (strcmp (buf[0], "BLUE") == 0)
            mix->output_channel = CM_BLUE_CHANNEL;

          fscanf (fp, "%*s %s", buf[0]); /* preview flag, preserved for compatibility */

          fscanf (fp, "%*s %s", buf[0]);
          if (strcmp (buf[0], "TRUE") == 0)
            mix->monochrome_flag = TRUE;
          else
            mix->monochrome_flag = FALSE;

          gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (mix->monochrome_toggle),
                                        mix->monochrome_flag);

          fscanf (fp, "%*s %s", buf[0]);
          if (strcmp (buf[0], "TRUE") == 0)
            mix->preserve_luminosity_flag = TRUE;
          else
            mix->preserve_luminosity_flag = FALSE;

          gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (mix->preserve_luminosity_toggle),
                                        mix->preserve_luminosity_flag);

          fscanf (fp, "%*s %s %s %s", buf[0], buf[1], buf[2]);
          mix->red.red_gain   = g_ascii_strtod (buf[0], NULL);
          mix->red.green_gain = g_ascii_strtod (buf[1], NULL);
          mix->red.blue_gain  = g_ascii_strtod (buf[2], NULL);

          fscanf (fp, "%*s %s %s %s", buf[0], buf[1], buf[2]);
          mix->green.red_gain   = g_ascii_strtod (buf[0], NULL);
          mix->green.green_gain = g_ascii_strtod (buf[1], NULL);
          mix->green.blue_gain  = g_ascii_strtod (buf[2], NULL);

          fscanf (fp, "%*s %s %s %s", buf[0], buf[1], buf[2]);
          mix->blue.red_gain   = g_ascii_strtod (buf[0], NULL);
          mix->blue.green_gain = g_ascii_strtod (buf[1], NULL);
          mix->blue.blue_gain  = g_ascii_strtod (buf[2], NULL);

          fscanf (fp, "%*s %s %s %s", buf[0], buf[1], buf[2]);
          mix->black.red_gain   = g_ascii_strtod (buf[0], NULL);
          mix->black.green_gain = g_ascii_strtod (buf[1], NULL);
          mix->black.blue_gain  = g_ascii_strtod (buf[2], NULL);

          fclose (fp);

          gimp_int_combo_box_set_active (GIMP_INT_COMBO_BOX (mix->combo),
                                         mix->output_channel);
          cm_set_adjusters (mix);

          gimp_preview_invalidate (GIMP_PREVIEW (preview));
        }
      else
        {
          g_message (_("Could not open '%s' for reading: %s"),
                     gimp_filename_to_utf8 (filename),
                     g_strerror (errno));
        }

      g_free (filename);
    }

  gtk_widget_hide (dialog);
}

static void
cm_save_file_callback (GtkWidget    *widget,
                       CmParamsType *mix)
{
  static GtkWidget *dialog = NULL;

  if (! dialog)
    {
      GtkWidget *parent = gtk_widget_get_toplevel (widget);
      gchar     *name;

      dialog =
        gtk_file_chooser_dialog_new (_("Save Channel Mixer Settings"),
                                     GTK_WINDOW (parent),
                                     GTK_FILE_CHOOSER_ACTION_SAVE,

                                     GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                                     GTK_STOCK_SAVE,   GTK_RESPONSE_OK,

                                     NULL);

      gtk_dialog_set_default_response (GTK_DIALOG (dialog), GTK_RESPONSE_OK);

      g_signal_connect (dialog, "response",
                        G_CALLBACK (cm_save_file_response_callback),
                        mix);
      g_signal_connect (dialog, "delete_event",
                        G_CALLBACK (gtk_true),
                        NULL);

      name = cm_settings_filename ();
      gtk_file_chooser_set_filename (GTK_FILE_CHOOSER (dialog), name);
      g_free (name);
    }

  gtk_window_present (GTK_WINDOW (dialog));
}

static void
cm_save_file_response_callback (GtkWidget    *dialog,
                                gint          response_id,
                                CmParamsType *mix)
{
  gchar *filename;
  FILE  *file = NULL;

  if (response_id != GTK_RESPONSE_OK)
    {
      gtk_widget_hide (dialog);
      return;
    }

  filename = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (dialog));
  if (! filename)
    return;

  if (g_file_test (filename, G_FILE_TEST_EXISTS) &&
      ! cm_force_overwrite (filename, dialog))
    {
      g_free (filename);
      return;
    }

  file = fopen (filename, "w");

  if (! file)
    {
      g_message (_("Could not open '%s' for writing: %s"),
                 gimp_filename_to_utf8 (filename), g_strerror (errno));
      g_free (filename);
      return;
    }

  cm_save_file (mix, file);

  g_message (_("Parameters were saved to '%s'"),
             gimp_filename_to_utf8 (filename));

  gtk_widget_hide (dialog);
}

static gboolean
cm_force_overwrite (const gchar *filename,
                    GtkWidget   *parent)
{
  GtkWidget *dlg;
  GtkWidget *label;
  GtkWidget *hbox;
  gchar     *buffer;
  gboolean   overwrite;

  dlg = gimp_dialog_new (_("Channel Mixer File Operation Warning"),
                         "channel_mixer",
                         parent, GTK_DIALOG_MODAL,
                         gimp_standard_help_func, HELP_ID,

                         GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                         GTK_STOCK_OK,     GTK_RESPONSE_OK,

                         NULL);

  hbox = gtk_hbox_new (FALSE, 0);
  gtk_container_set_border_width (GTK_CONTAINER (hbox), 12);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dlg)->vbox), hbox, FALSE, FALSE, 0);
  gtk_widget_show (hbox);

  buffer = g_strdup_printf (_("File '%s' exists.\n"
                              "Overwrite it?"), filename);
  label = gtk_label_new (buffer);
  g_free (buffer);

  gtk_label_set_justify (GTK_LABEL (label), GTK_JUSTIFY_LEFT);
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
  gtk_widget_show (label);

  gtk_widget_show (dlg);

  overwrite = (gimp_dialog_run (GIMP_DIALOG (dlg)) == GTK_RESPONSE_OK);

  gtk_widget_destroy (dlg);

  return overwrite;
}

static void
cm_save_file (CmParamsType *mix,
              FILE         *fp)
{
  const gchar *str = NULL;
  gchar        buf[3][G_ASCII_DTOSTR_BUF_SIZE];

  switch (mix->output_channel)
    {
    case CM_RED_CHANNEL:
      str = "RED";
      break;
    case CM_GREEN_CHANNEL:
      str = "GREEN";
      break;
    case CM_BLUE_CHANNEL:
      str = "BLUE";
      break;
    default:
      g_assert_not_reached ();
      break;
    }

  fprintf (fp, "# Channel Mixer Configuration File\n");

  fprintf (fp, "CHANNEL: %s\n", str);
  fprintf (fp, "PREVIEW: %s\n", "TRUE"); /* preserved for compatibility */
  fprintf (fp, "MONOCHROME: %s\n",
           mix->monochrome_flag ? "TRUE" : "FALSE");
  fprintf (fp, "PRESERVE_LUMINOSITY: %s\n",
           mix->preserve_luminosity_flag ? "TRUE" : "FALSE");

  fprintf (fp, "RED: %s %s %s\n",
           g_ascii_formatd (buf[0], sizeof (buf[0]), "%5.3f",
                            mix->red.red_gain),
           g_ascii_formatd (buf[1], sizeof (buf[1]), "%5.3f",
                            mix->red.green_gain),
           g_ascii_formatd (buf[2], sizeof (buf[2]), "%5.3f",
                            mix->red.blue_gain));

  fprintf (fp, "GREEN: %s %s %s\n",
           g_ascii_formatd (buf[0], sizeof (buf[0]), "%5.3f",
                            mix->green.red_gain),
           g_ascii_formatd (buf[1], sizeof (buf[1]), "%5.3f",
                            mix->green.green_gain),
           g_ascii_formatd (buf[2], sizeof (buf[2]), "%5.3f",
                            mix->green.blue_gain));

  fprintf (fp, "BLUE: %s %s %s\n",
           g_ascii_formatd (buf[0], sizeof (buf[0]), "%5.3f",
                            mix->blue.red_gain),
           g_ascii_formatd (buf[1], sizeof (buf[1]), "%5.3f",
                            mix->blue.green_gain),
           g_ascii_formatd (buf[2], sizeof (buf[2]), "%5.3f",
                            mix->blue.blue_gain));

  fprintf (fp, "BLACK: %s %s %s\n",
           g_ascii_formatd (buf[0], sizeof (buf[0]), "%5.3f",
                            mix->black.red_gain),
           g_ascii_formatd (buf[1], sizeof (buf[1]), "%5.3f",
                            mix->black.green_gain),
           g_ascii_formatd (buf[2], sizeof (buf[2]), "%5.3f",
                            mix->black.blue_gain));

  fclose (fp);
}

static void
cm_combo_callback (GtkWidget    *widget,
                   CmParamsType *mix)
{
  gimp_int_combo_box_get_active (GIMP_INT_COMBO_BOX (widget),
                                 (gint *) &mix->output_channel);

  cm_set_adjusters (mix);
}

static void
cm_set_adjusters (CmParamsType *mix)
{
  if (mix->monochrome_flag == TRUE)
    {
      gtk_adjustment_set_value (mix->red_data,   mix->black.red_gain   * 100.0);
      gtk_adjustment_set_value (mix->green_data, mix->black.green_gain * 100.0);
      gtk_adjustment_set_value (mix->blue_data,  mix->black.blue_gain  * 100.0);

      return;
    }

  switch (mix->output_channel)
    {
    case CM_RED_CHANNEL:
      gtk_adjustment_set_value (mix->red_data,   mix->red.red_gain   * 100.0);
      gtk_adjustment_set_value (mix->green_data, mix->red.green_gain * 100.0);
      gtk_adjustment_set_value (mix->blue_data,  mix->red.blue_gain  * 100.0);
      break;

    case CM_GREEN_CHANNEL:
      gtk_adjustment_set_value (mix->red_data,   mix->green.red_gain   * 100.0);
      gtk_adjustment_set_value (mix->green_data, mix->green.green_gain * 100.0);
      gtk_adjustment_set_value (mix->blue_data,  mix->green.blue_gain  * 100.0);
      break;

    case CM_BLUE_CHANNEL:
      gtk_adjustment_set_value (mix->red_data,   mix->blue.red_gain   * 100.0);
      gtk_adjustment_set_value (mix->green_data, mix->blue.green_gain * 100.0);
      gtk_adjustment_set_value (mix->blue_data,  mix->blue.blue_gain  * 100.0);
      break;
    }
}
