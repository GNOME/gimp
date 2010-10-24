/* Ripple --- image filter plug-in for GIMP
 * Copyright (C) 1997 Brian Degenhardt
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
 * Please direct all comments, questions, bug reports  etc to Brian Degenhardt
 * bdegenha@ucsd.edu
 *
 * You can contact the original GIMP authors at gimp@xcf.berkeley.edu
 */

#include "config.h"

#include <string.h>

#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

#include "libgimp/stdplugins-intl.h"


/* Some useful macros */
#define PLUG_IN_PROC    "plug-in-ripple"
#define PLUG_IN_BINARY  "ripple"
#define PLUG_IN_ROLE    "gimp-ripple"

#define SCALE_WIDTH     200
#define TILE_CACHE_SIZE  16

#define SMEAR 0
#define WRAP  1
#define BLANK 2

#define SAWTOOTH 0
#define SINE     1

typedef struct
{
  gint                 period;
  gint                 amplitude;
  GimpOrientationType  orientation;
  gint                 edges;
  gint                 waveform;
  gboolean             antialias;
  gboolean             tile;
  gint                 phase_shift;
} RippleValues;


/* Declare local functions.
 */
static void      query              (void);
static void      run                (const gchar      *name,
                                     gint              nparams,
                                     const GimpParam  *param,
                                     gint             *nreturn_vals,
                                     GimpParam       **return_vals);

static void      ripple             (GimpDrawable     *drawable,
                                     GimpPreview      *preview);

static gboolean  ripple_dialog      (GimpDrawable     *drawable);

static gdouble   displace_amount    (gint              location);
static void      average_two_pixels (guchar           *dest,
                                     guchar            pixels[4][4],
                                     gdouble           x,
                                     gint              bpp,
                                     gboolean          has_alpha);

/***** Local vars *****/

const GimpPlugInInfo PLUG_IN_INFO =
{
  NULL,  /* init_proc  */
  NULL,  /* quit_proc  */
  query, /* query_proc */
  run,   /* run_proc   */
};

static RippleValues rvals =
{
  20,                          /* period      */
  5,                           /* amplitude   */
  GIMP_ORIENTATION_HORIZONTAL, /* orientation */
  WRAP,                        /* edges       */
  SINE,                        /* waveform    */
  TRUE,                        /* antialias   */
  FALSE,                       /* tile        */
  0                            /* phase shift */
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
    { GIMP_PDB_INT32,    "period",      "Period: number of pixels for one wave to complete" },
    { GIMP_PDB_INT32,    "amplitude",   "Amplitude: maximum displacement of wave" },
    { GIMP_PDB_INT32,    "orientation", "Orientation { ORIENTATION-HORIZONTAL (0), ORIENTATION-VERTICAL (1) }" },
    { GIMP_PDB_INT32,    "edges",       "Edges { SMEAR (0), WRAP (1), BLANK (2) }" },
    { GIMP_PDB_INT32,    "waveform",    "Waveform { SAWTOOTH (0), SINE (1) }" },
    { GIMP_PDB_INT32,    "antialias",   "Antialias { TRUE, FALSE }" },
    { GIMP_PDB_INT32,    "tile",        "Tileable { TRUE, FALSE }" }
  };

  gimp_install_procedure (PLUG_IN_PROC,
                          N_("Displace pixels in a ripple pattern"),
                          "Ripples the pixels of the specified drawable. "
                          "Each row or column will be displaced a certain "
                          "number of pixels coinciding with the given wave form",
                          "Brian Degenhardt <bdegenha@ucsd.edu>",
                          "Brian Degenhardt",
                          "1997",
                          N_("_Ripple..."),
                          "RGB*, GRAY*",
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
  GimpDrawable      *drawable;
  GimpRunMode        run_mode;
  GimpPDBStatusType  status = GIMP_PDB_SUCCESS;

  run_mode = param[0].data.d_int32;

  INIT_I18N ();

  /*  Get the specified drawable  */
  drawable = gimp_drawable_get (param[2].data.d_drawable);

  /*  set the tile cache size  */
  gimp_tile_cache_ntiles (TILE_CACHE_SIZE);

  *nreturn_vals = 1;
  *return_vals  = values;

  values[0].type          = GIMP_PDB_STATUS;
  values[0].data.d_status = status;

  switch (run_mode)
    {
    case GIMP_RUN_INTERACTIVE:
      /*  Possibly retrieve data  */
      gimp_get_data (PLUG_IN_PROC, &rvals);

      /*  First acquire information with a dialog  */
      if (! ripple_dialog (drawable))
        return;
      break;

    case GIMP_RUN_NONINTERACTIVE:
      /*  Make sure all the arguments are there!  */
      if (nparams != 10)
        {
          status = GIMP_PDB_CALLING_ERROR;
        }
      else
        {
          rvals.period      = param[3].data.d_int32;
          rvals.amplitude   = param[4].data.d_int32;
          rvals.orientation = (param[5].data.d_int32) ? GIMP_ORIENTATION_VERTICAL : GIMP_ORIENTATION_HORIZONTAL;
          rvals.edges       = param[6].data.d_int32;
          rvals.waveform    = param[7].data.d_int32;
          rvals.antialias   = (param[8].data.d_int32) ? TRUE : FALSE;
          rvals.tile        = (param[9].data.d_int32) ? TRUE : FALSE;

          if (rvals.period < 1)
            {
              gimp_message ("Ripple: period must be at least 1.\n");
              status = GIMP_PDB_CALLING_ERROR;
            }

          if (rvals.edges < SMEAR || rvals.edges > BLANK)
            status = GIMP_PDB_CALLING_ERROR;
        }
      break;

    case GIMP_RUN_WITH_LAST_VALS:
      /*  Possibly retrieve data  */
      gimp_get_data (PLUG_IN_PROC, &rvals);
      break;

    default:
      break;
    }

  if (status == GIMP_PDB_SUCCESS)
    {
      /*  Make sure that the drawable is gray or RGB color  */
      if (gimp_drawable_is_rgb (drawable->drawable_id) ||
          gimp_drawable_is_gray (drawable->drawable_id))
        {
          gimp_progress_init (_("Rippling"));

          /*  run the ripple effect  */
          ripple (drawable, NULL);

          gimp_progress_update (1.0);

          if (run_mode != GIMP_RUN_NONINTERACTIVE)
            gimp_displays_flush ();

          /*  Store data  */
          if (run_mode == GIMP_RUN_INTERACTIVE)
            gimp_set_data (PLUG_IN_PROC, &rvals, sizeof (RippleValues));
        }
      else
        {
          /* gimp_message ("ripple: cannot operate on indexed color images"); */
          status = GIMP_PDB_EXECUTION_ERROR;
        }
    }

  values[0].data.d_status = status;

  gimp_drawable_detach (drawable);
}

typedef struct
{
  GimpPixelFetcher *pft;
  gint              width;
  gint              height;
  gboolean          has_alpha;
} RippleParam_t;

static void
ripple_vertical (gint      x,
                 gint      y,
                 guchar   *dest,
                 gint      bpp,
                 gpointer  data)
{
  RippleParam_t    *param  = data;
  GimpPixelFetcher *pft    = param->pft;
  const gint        height = param->height;
  guchar            pixel[2][4];
  gdouble           needy;
  gint              yi, yi_a;

  needy = y + displace_amount (x);
  yi = floor (needy);
  yi_a = yi + 1;

  if (rvals.edges == WRAP)
    {
      /* Tile the image. */

      needy = fmod (needy, height);

      if (needy < 0.0)
        needy += height;

      yi = yi % height;

      if (yi < 0)
        yi += height;

      yi_a = yi_a % height;
    }
  else if (rvals.edges == SMEAR)
    {
      /* Smear out the edges of the image by repeating pixels. */

      needy = CLAMP (needy, 0, height - 1);
      yi    = CLAMP (yi,    0, height - 1);
      yi_a  = CLAMP (yi_a,  0, height - 1);
    }

  if (rvals.antialias)
    {
      if (yi >= 0 && yi < height)
        gimp_pixel_fetcher_get_pixel (pft, x, yi  , pixel[0]);
      else
        memset (pixel[0], 0, 4);

      if (yi_a >= 0 && yi_a < height)
        gimp_pixel_fetcher_get_pixel (pft, x, yi_a, pixel[1]);
      else
        memset (pixel[1], 0, 4);

      average_two_pixels (dest, pixel, needy - yi, bpp, param->has_alpha);
    }
  else
    {
      if (yi >= 0 && yi < height)
        gimp_pixel_fetcher_get_pixel (pft, x, yi, dest);
      else
        memset (dest, 0, bpp);
    }
}

static void
ripple_horizontal (gint      x,
                   gint      y,
                   guchar   *dest,
                   gint      bpp,
                   gpointer  data)
{
  RippleParam_t    *param = data;
  GimpPixelFetcher *pft   = param->pft;
  const gint        width = param->width;
  guchar            pixel[2][4];
  gdouble           needx;
  gint              xi, xi_a;

  needx = x + displace_amount (y);
  xi = floor (needx);
  xi_a = xi + 1;

  if (rvals.edges == WRAP)
    {
      /* Tile the image. */

      needx = fmod (needx, width);

      while (needx < 0.0)
        needx += width;

      xi = (xi % width);

      while (xi < 0)
        xi += width;

      xi_a = (xi+1) % width;
    }
  else if (rvals.edges == SMEAR)
    {
      /* Smear out the edges of the image by repeating pixels. */

      needx = CLAMP (needx, 0, width - 1);
      xi    = CLAMP (xi,    0, width - 1);
      xi_a  = CLAMP (xi_a,  0, width - 1);
    }

  if (rvals.antialias)
    {
      if (xi >= 0 && xi < width)
        gimp_pixel_fetcher_get_pixel (pft, xi,   y, pixel[0]);
      else
        memset (pixel[0], 0, 4);

      if (xi_a >= 0 && xi_a < width)
        gimp_pixel_fetcher_get_pixel (pft, xi_a, y, pixel[1]);
      else
        memset (pixel[1], 0, 4);

      average_two_pixels (dest, pixel, needx - xi, bpp, param->has_alpha);
    }

  else
    {
      if (xi >= 0 && xi < width)
        gimp_pixel_fetcher_get_pixel (pft, xi, y, dest);
      else
        memset (dest, 0, bpp);
    }
}

static void
ripple (GimpDrawable *drawable,
        GimpPreview  *preview)
{
  RippleParam_t param;
  gint          edges;
  gint          period;

  param.pft       = gimp_pixel_fetcher_new (drawable, FALSE);
  param.has_alpha = gimp_drawable_has_alpha (drawable->drawable_id);
  param.width     = drawable->width;
  param.height    = drawable->height;

  edges  = rvals.edges;
  period = rvals.period;

  if (rvals.tile)
    {
      rvals.edges = WRAP;
      rvals.period = (param.width / (param.width / rvals.period) *
                      (rvals.orientation == GIMP_ORIENTATION_HORIZONTAL) +
                      param.height / (param.height / rvals.period) *
                      (rvals.orientation == GIMP_ORIENTATION_VERTICAL));
    }

  if (preview)
    {
      guchar *buffer, *d;
      gint    bpp = gimp_drawable_bpp (drawable->drawable_id);
      gint    width, height;
      gint    x, y;
      gint    x1, y1;

      gimp_preview_get_position (preview, &x1, &y1);
      gimp_preview_get_size (preview, &width, &height);

      d = buffer = g_new (guchar, width * height * bpp);

      for (y = 0; y < height ; y++)
        for (x = 0; x < width ; x++)
          {
            if (rvals.orientation == GIMP_ORIENTATION_VERTICAL)
              ripple_vertical (x1 + x, y1 + y, d, bpp, &param);
            else
              ripple_horizontal (x1 + x, y1 + y, d, bpp, &param);

            d += bpp;
          }

      gimp_preview_draw_buffer (preview, buffer, width * bpp);
      g_free (buffer);
    }
  else
    {
      GimpRgnIterator *iter;

      iter = gimp_rgn_iterator_new (drawable, 0);

      gimp_rgn_iterator_dest (iter,
                              rvals.orientation == GIMP_ORIENTATION_VERTICAL ?
                              ripple_vertical :
                              ripple_horizontal,
                              &param);

      gimp_rgn_iterator_free (iter);
    }

  rvals.edges  = edges;
  rvals.period = period;

  gimp_pixel_fetcher_destroy (param.pft);
}

static gboolean
ripple_dialog (GimpDrawable *drawable)
{
  GtkWidget     *dialog;
  GtkWidget     *main_vbox;
  GtkWidget     *preview;
  GtkWidget     *toggle;
  GtkWidget     *toggle_vbox;
  GtkWidget     *frame;
  GtkWidget     *table;
  GtkAdjustment *scale_data;
  GtkWidget     *horizontal;
  GtkWidget     *vertical;
  GtkWidget     *wrap;
  GtkWidget     *smear;
  GtkWidget     *blank;
  GtkWidget     *sawtooth;
  GtkWidget     *sine;
  gboolean       run;

  gimp_ui_init (PLUG_IN_BINARY, TRUE);

  dialog = gimp_dialog_new (_("Ripple"), PLUG_IN_ROLE,
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

  /*  The main vbox  */
  main_vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 12);
  gtk_container_set_border_width (GTK_CONTAINER (main_vbox), 12);
  gtk_box_pack_start (GTK_BOX (gtk_dialog_get_content_area (GTK_DIALOG (dialog))),
                      main_vbox, TRUE, TRUE, 0);
  gtk_widget_show (main_vbox);

  preview = gimp_drawable_preview_new_from_drawable_id (drawable->drawable_id);
  gtk_box_pack_start (GTK_BOX (main_vbox), preview, TRUE, TRUE, 0);
  gtk_widget_show (preview);

  g_signal_connect_swapped (preview, "invalidated",
                            G_CALLBACK (ripple),
                            drawable);

  /* The table to hold the four frames of options */
  table = gtk_table_new (2, 2, FALSE);
  gtk_table_set_col_spacings (GTK_TABLE (table), 12);
  gtk_table_set_row_spacings (GTK_TABLE (table), 12);
  gtk_box_pack_start (GTK_BOX (main_vbox), table, FALSE, FALSE, 0);

  /*  Options section  */
  frame = gimp_frame_new ( _("Options"));
  gtk_table_attach (GTK_TABLE (table), frame, 0, 1, 0, 1,
                    GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL, 0, 0);
  gtk_widget_show (frame);

  toggle_vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 6);
  gtk_container_add (GTK_CONTAINER (frame), toggle_vbox);
  gtk_widget_show (toggle_vbox);

  toggle = gtk_check_button_new_with_mnemonic (_("_Antialiasing"));
  gtk_box_pack_start (GTK_BOX (toggle_vbox), toggle, FALSE, FALSE, 0);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (toggle), rvals.antialias);
  gtk_widget_show (toggle);

  g_signal_connect (toggle, "toggled",
                    G_CALLBACK (gimp_toggle_button_update),
                    &rvals.antialias);
  g_signal_connect_swapped (toggle, "toggled",
                            G_CALLBACK (gimp_preview_invalidate),
                            preview);

  toggle = gtk_check_button_new_with_mnemonic ( _("_Retain tilability"));
  gtk_box_pack_start (GTK_BOX (toggle_vbox), toggle, FALSE, FALSE, 0);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (toggle), (rvals.tile));
  gtk_widget_show (toggle);

  g_signal_connect (toggle, "toggled",
                    G_CALLBACK (gimp_toggle_button_update),
                    &rvals.tile);
  g_signal_connect_swapped (toggle, "toggled",
                            G_CALLBACK (gimp_preview_invalidate),
                            preview);

  /*  Orientation toggle box  */
  frame = gimp_int_radio_group_new (TRUE, _("Orientation"),
                                    G_CALLBACK (gimp_radio_button_update),
                                    &rvals.orientation, rvals.orientation,

                                    _("_Horizontal"), GIMP_ORIENTATION_HORIZONTAL,
                                    &horizontal,

                                    _("_Vertical"),   GIMP_ORIENTATION_VERTICAL,
                                    &vertical,

                                    NULL);

  gtk_table_attach (GTK_TABLE (table), frame, 1, 2, 0, 1,
                    GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL, 0, 0);
  gtk_widget_show (frame);

  g_signal_connect_swapped (horizontal, "toggled",
                            G_CALLBACK (gimp_preview_invalidate),
                            preview);
  g_signal_connect_swapped (vertical, "toggled",
                            G_CALLBACK (gimp_preview_invalidate),
                            preview);

  /*  Edges toggle box  */
  frame = gimp_int_radio_group_new (TRUE, _("Edges"),
                                    G_CALLBACK (gimp_radio_button_update),
                                    &rvals.edges, rvals.edges,

                                    _("_Wrap"),  WRAP,  &wrap,
                                    _("_Smear"), SMEAR, &smear,
                                    _("_Blank"), BLANK, &blank,

                                    NULL);

  gtk_table_attach (GTK_TABLE (table), frame, 0, 1, 1, 2,
                    GTK_FILL | GTK_EXPAND, GTK_FILL | GTK_EXPAND, 0, 0);
  gtk_widget_show (frame);

  g_object_bind_property (toggle, "active",
                          frame,  "sensitive",
                          G_BINDING_SYNC_CREATE | G_BINDING_INVERT_BOOLEAN);

  g_signal_connect_swapped (wrap, "toggled",
                            G_CALLBACK (gimp_preview_invalidate),
                            preview);
  g_signal_connect_swapped (smear, "toggled",
                            G_CALLBACK (gimp_preview_invalidate),
                            preview);
  g_signal_connect_swapped (blank, "toggled",
                            G_CALLBACK (gimp_preview_invalidate),
                            preview);

  /*  Wave toggle box  */
  frame = gimp_int_radio_group_new (TRUE, _("Wave Type"),
                                    G_CALLBACK (gimp_radio_button_update),
                                    &rvals.waveform, rvals.waveform,

                                    _("Saw_tooth"), SAWTOOTH, &sawtooth,
                                    _("S_ine"),     SINE,     &sine,

                                    NULL);
  gtk_table_attach (GTK_TABLE (table), frame, 1, 2, 1, 2,
                    GTK_FILL | GTK_EXPAND, GTK_FILL | GTK_EXPAND, 0, 0);
  gtk_widget_show (frame);

  g_signal_connect_swapped (sawtooth, "toggled",
                            G_CALLBACK (gimp_preview_invalidate),
                            preview);
  g_signal_connect_swapped (sine, "toggled",
                            G_CALLBACK (gimp_preview_invalidate),
                            preview);

  gtk_widget_show (table);

  table = gtk_table_new (3, 3, FALSE);
  gtk_table_set_col_spacings (GTK_TABLE (table), 6);
  gtk_table_set_row_spacings (GTK_TABLE (table), 6);
  gtk_box_pack_start (GTK_BOX (main_vbox), table, FALSE, FALSE, 0);

  /*  Period  */
  scale_data = gimp_scale_entry_new (GTK_TABLE (table), 0, 0,
                                     _("_Period:"), SCALE_WIDTH, 0,
                                     rvals.period, 1, 200, 1, 10, 0,
                                     TRUE, 0, 0,
                                     NULL, NULL);
  g_signal_connect (scale_data, "value-changed",
                    G_CALLBACK (gimp_int_adjustment_update),
                    &rvals.period);
  g_signal_connect_swapped (scale_data, "value-changed",
                            G_CALLBACK (gimp_preview_invalidate),
                            preview);

  /*  Amplitude  */
  scale_data = gimp_scale_entry_new (GTK_TABLE (table), 0, 1,
                                     _("A_mplitude:"), SCALE_WIDTH, 0,
                                     rvals.amplitude, 0, 200, 1, 10, 0,
                                     TRUE, 0, 0,
                                     NULL, NULL);
  g_signal_connect (scale_data, "value-changed",
                    G_CALLBACK (gimp_int_adjustment_update),
                    &rvals.amplitude);
  g_signal_connect_swapped (scale_data, "value-changed",
                            G_CALLBACK (gimp_preview_invalidate),
                            preview);

  /*  Phase Shift  */
  scale_data = gimp_scale_entry_new (GTK_TABLE (table), 0, 2,
                                     _("Phase _shift:"), SCALE_WIDTH, 0,
                                     rvals.phase_shift, 0, 360, 1, 15, 0,
                                     TRUE, 0, 0,
                                     NULL, NULL);
  g_signal_connect (scale_data, "value-changed",
                    G_CALLBACK (gimp_int_adjustment_update),
                    &rvals.phase_shift);
  g_signal_connect_swapped (scale_data, "value-changed",
                            G_CALLBACK (gimp_preview_invalidate),
                            preview);

  gtk_widget_show (frame);
  gtk_widget_show (table);
  gtk_widget_show (dialog);

  run = (gimp_dialog_run (GIMP_DIALOG (dialog)) == GTK_RESPONSE_OK);

  gtk_widget_destroy (dialog);

  return run;
}

static void
average_two_pixels (guchar   *dest,
                    guchar    pixels[4][4],
                    gdouble   x,
                    gint      bpp,
                    gboolean  has_alpha)
{
  gint b;

  /* x = x - floor(x); */

  if (has_alpha)
    {
      gdouble xa0 = pixels[0][bpp-1] * (1.0 - x);
      gdouble xa1 = pixels[1][bpp-1] * x;
      gdouble alpha;

      alpha = xa0 + xa1;

      if (alpha)
        for (b = 0; b < bpp-1; b++)
          dest[b] = (xa0 * pixels[0][b] + xa1 * pixels[1][b]) / alpha;

      dest[bpp-1] = alpha;
    }
  else
    {
      for (b = 0; b < bpp; b++)
        dest[b] = (1.0 - x) * pixels[0][b] + x * pixels[1][b];
    }
}

static gdouble
displace_amount (gint location)
{
  const gdouble phi = rvals.phase_shift / 360.0;
  gdouble       lambda;

  switch (rvals.waveform)
    {
    case SINE:
      return (rvals.amplitude *
              sin (2 * G_PI * (location / (gdouble) rvals.period - phi)));

    case SAWTOOTH:
      lambda = location % rvals.period - phi * rvals.period;
      if (lambda < 0)
        lambda += rvals.period;
      return (rvals.amplitude * (fabs (((lambda / rvals.period) * 4) - 2) - 1));
    }

  return 0.0;
}
