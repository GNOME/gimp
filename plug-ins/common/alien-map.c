/**********************************************************************
 *  AlienMap2 (Co-)sine color transformation plug-in (Version 1.01)
 *  Martin Weber (martweb@gmx.net)
 **********************************************************************
 *  Most code taken from AlienMap by Daniel Cotting
 *  This is not a replacement for AlienMap!
 **********************************************************************
 */

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

#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

#include "libgimp/stdplugins-intl.h"

#define PLUG_IN_PROC   "plug-in-alienmap2"
#define PLUG_IN_BINARY "alien-map"
#define SCALE_WIDTH    200
#define ENTRY_WIDTH      6

/***** Color model *****/

typedef enum
{
  RGB_MODEL = 0,
  HSL_MODEL = 1
} ColorModel;

/***** Types *****/
typedef struct
{
  gdouble    redfrequency;
  gdouble    redangle;
  gdouble    greenfrequency;
  gdouble    greenangle;
  gdouble    bluefrequency;
  gdouble    blueangle;
  ColorModel colormodel;
  gboolean   redmode;
  gboolean   greenmode;
  gboolean   bluemode;
} alienmap2_vals_t;

/* Declare local functions. */

static void      query  (void);
static void      run    (const gchar      *name,
                         gint              nparams,
                         const GimpParam  *param,
                         gint             *nreturn_vals,
                         GimpParam       **return_vals);

static void      alienmap2                (GimpDrawable  *drawable);
static void      transform                (guchar        *r,
                                           guchar        *g,
                                           guchar        *b);
static gint      alienmap2_dialog         (void);
static void      dialog_update_preview    (GimpDrawable  *drawable,
                                           GimpPreview   *preview);
static void      dialog_scale_update      (GtkAdjustment *adjustment,
                                           gdouble       *value);
static void      alienmap2_toggle_update  (GtkWidget     *widget,
                                           gpointer       data);
static void      alienmap2_radio_update   (GtkWidget     *widget,
                                           gpointer       data);

static void      alienmap2_set_labels     (void);
static void      alienmap2_set_sensitive  (void);
static void      alienmap2_get_label_size (void);


/***** Variables *****/

static GtkWidget *preview;

const GimpPlugInInfo PLUG_IN_INFO =
{
  NULL,  /* init_proc  */
  NULL,  /* quit_proc  */
  query, /* query_proc */
  run    /* run_proc   */
};

static alienmap2_vals_t wvals =
{
  1.0,
  0.0,
  1.0,
  0.0,
  1.0,
  0.0,
  RGB_MODEL,
  TRUE,
  TRUE,
  TRUE
};

static GimpDrawable *drawable         = NULL;

static GtkWidget    *toggle_modify_rh = NULL;
static GtkWidget    *toggle_modify_gs = NULL;
static GtkWidget    *toggle_modify_bl = NULL;

static GtkWidget    *label_freq_rh    = NULL;
static GtkWidget    *label_freq_gs    = NULL;
static GtkWidget    *label_freq_bl    = NULL;
static GtkWidget    *label_phase_rh   = NULL;
static GtkWidget    *label_phase_gs   = NULL;
static GtkWidget    *label_phase_bl   = NULL;

static GtkObject    *entry_freq_rh    = NULL;
static GtkObject    *entry_freq_gs    = NULL;
static GtkObject    *entry_freq_bl    = NULL;
static GtkObject    *entry_phase_rh   = NULL;
static GtkObject    *entry_phase_gs   = NULL;
static GtkObject    *entry_phase_bl   = NULL;


static const gchar *ctext[][2] =
{
  { N_("_Modify red channel"),   N_("_Modify hue channel")        },
  { N_("Mo_dify green channel"), N_("Mo_dify saturation channel") },
  { N_("Mod_ify blue channel"),  N_("Mod_ify luminosity channel") }
};

static const gchar *etext[][2] =
{
  { N_("Red _frequency:"),       N_("Hue _frequency:")            },
  { N_("Green fr_equency:"),     N_("Saturation fr_equency:")     },
  { N_("Blue freq_uency:"),      N_("Luminosity freq_uency:")     },

  { N_("Red _phaseshift:"),      N_("Hue _phaseshift:")           },
  { N_("Green ph_aseshift:"),    N_("Saturation ph_aseshift:")    },
  { N_("Blue pha_seshift:"),     N_("Luminosity pha_seshift:")    },
};
static gint elabel_maxwidth = 0;


/***** Functions *****/

MAIN ()

static void
query (void)
{
  static const GimpParamDef args[] =
  {
    { GIMP_PDB_INT32,    "run-mode",       "The run mode { RUN-INTERACTIVE (0), RUN-NONINTERACTIVE (1) }" },
    { GIMP_PDB_IMAGE,    "image",          "Input image" },
    { GIMP_PDB_DRAWABLE, "drawable",       "Input drawable" },
    { GIMP_PDB_FLOAT,    "redfrequency",   "Red/hue component frequency factor" },
    { GIMP_PDB_FLOAT,    "redangle",       "Red/hue component angle factor (0-360)" },
    { GIMP_PDB_FLOAT,    "greenfrequency", "Green/saturation component frequency factor" },
    { GIMP_PDB_FLOAT,    "greenangle",     "Green/saturation component angle factor (0-360)" },
    { GIMP_PDB_FLOAT,    "bluefrequency",  "Blue/luminance component frequency factor" },
    { GIMP_PDB_FLOAT,    "blueangle",      "Blue/luminance component angle factor (0-360)" },
    { GIMP_PDB_INT8,     "colormodel",     "Color model { RGB-MODEL (0), HSL-MODEL (1) }" },
    { GIMP_PDB_INT8,     "redmode",        "Red/hue application mode { TRUE, FALSE }" },
    { GIMP_PDB_INT8,     "greenmode",      "Green/saturation application mode { TRUE, FALSE }" },
    { GIMP_PDB_INT8,     "bluemode",       "Blue/luminance application mode { TRUE, FALSE }" },
  };

  gimp_install_procedure (PLUG_IN_PROC,
                          N_("Alter colors in various psychedelic ways"),
                          "No help yet. Just try it and you'll see!",
                          "Martin Weber (martweb@gmx.net)",
                          "Martin Weber (martweb@gmx.net",
                          "24th April 1998",
                          N_("_Alien Map..."),
                          "RGB*",
                          GIMP_PLUGIN,
                          G_N_ELEMENTS (args), 0,
                          args, NULL);

  gimp_plugin_menu_register (PLUG_IN_PROC, "<Image>/Colors/Map");
}

static void
transform (guchar *r,
           guchar *g,
           guchar *b)
{
  switch (wvals.colormodel)
    {
    case HSL_MODEL:
      {
        GimpHSL hsl;
        GimpRGB rgb;

        gimp_rgb_set_uchar (&rgb, *r, *g, *b);
        gimp_rgb_to_hsl (&rgb, &hsl);

        if (wvals.redmode)
          hsl.h = 0.5 * (1.0 + sin (((2 * hsl.h - 1.0) * wvals.redfrequency +
                                     wvals.redangle / 180.0) * G_PI));

        if (wvals.greenmode)
          hsl.s = 0.5 * (1.0 + sin (((2 * hsl.s - 1.0) * wvals.greenfrequency +
                                     wvals.greenangle / 180.0) * G_PI));

        if (wvals.bluemode)
          hsl.l = 0.5 * (1.0 + sin (((2 * hsl.l - 1.0) * wvals.bluefrequency +
                                     wvals.blueangle / 180.0) * G_PI));

        gimp_hsl_to_rgb (&hsl, &rgb);
        gimp_rgb_get_uchar (&rgb, r, g, b);
      }
      break;

    case RGB_MODEL:
      if (wvals.redmode)
        *r = ROUND (127.5 * (1.0 +
                             sin (((*r / 127.5 - 1.0) * wvals.redfrequency +
                                   wvals.redangle / 180.0) * G_PI)));

      if (wvals.greenmode)
        *g = ROUND (127.5 * (1.0 +
                             sin (((*g / 127.5 - 1.0) * wvals.greenfrequency +
                                   wvals.greenangle / 180.0) * G_PI)));

      if (wvals.bluemode)
        *b = ROUND (127.5 * (1.0 +
                             sin (((*b / 127.5 - 1.0) * wvals.bluefrequency +
                                   wvals.blueangle / 180.0) * G_PI)));
      break;
    }
}

static void
run (const gchar      *name,
     gint              nparams,
     const GimpParam  *param,
     gint             *nreturn_vals,
     GimpParam       **return_vals)
{
  static GimpParam  values[1];
  GimpPDBStatusType status = GIMP_PDB_SUCCESS;
  GimpRunMode       run_mode;

  INIT_I18N ();

  run_mode = param[0].data.d_int32;

  values[0].type = GIMP_PDB_STATUS;
  values[0].data.d_status = status;

  *nreturn_vals = 1;
  *return_vals = values;

  /*  Get the specified drawable  */
  drawable = gimp_drawable_get (param[2].data.d_drawable);

  /* See how we will run */
  switch (run_mode)
    {
    case GIMP_RUN_INTERACTIVE:
      /* Possibly retrieve data */
      gimp_get_data (PLUG_IN_PROC, &wvals);

      /* Get information from the dialog */
      if (!alienmap2_dialog ())
        return;

      break;

    case GIMP_RUN_NONINTERACTIVE:
      /* Make sure all the arguments are present */
      if (nparams != 13)
        status = GIMP_PDB_CALLING_ERROR;

      if (status == GIMP_PDB_SUCCESS)
        {
          wvals.redfrequency   = param[3].data.d_float;
          wvals.redangle       = param[4].data.d_float;
          wvals.greenfrequency = param[5].data.d_float;
          wvals.greenangle     = param[6].data.d_float;
          wvals.bluefrequency  = param[7].data.d_float;
          wvals.blueangle      = param[8].data.d_float;
          wvals.colormodel     = param[9].data.d_int8;
          wvals.redmode        = param[10].data.d_int8 ? TRUE : FALSE;
          wvals.greenmode      = param[11].data.d_int8 ? TRUE : FALSE;
          wvals.bluemode       = param[12].data.d_int8 ? TRUE : FALSE;
        }

      break;

    case GIMP_RUN_WITH_LAST_VALS:
      /* Possibly retrieve data */
      gimp_get_data (PLUG_IN_PROC, &wvals);
      break;

    default:
      break;
    }

  if (status == GIMP_PDB_SUCCESS)
    {
      /*  Make sure that the drawable is RGB or RGBA  */
      if (gimp_drawable_is_rgb (drawable->drawable_id))
        {
          gimp_progress_init (_("Alien Map: Transforming"));

          /* Set the tile cache size */
          gimp_tile_cache_ntiles (2 * (drawable->width /
                                       gimp_tile_width () + 1));

          /* Run! */

          alienmap2 (drawable);
          if (run_mode != GIMP_RUN_NONINTERACTIVE)
            gimp_displays_flush ();

          /* Store data */
          if (run_mode == GIMP_RUN_INTERACTIVE)
            gimp_set_data (PLUG_IN_PROC,
                           &wvals, sizeof (alienmap2_vals_t));
        }
      else
        {
          /* gimp_message("This filter only applies on RGB_MODEL-images"); */
          status = GIMP_PDB_EXECUTION_ERROR;
        }
    }

  values[0].data.d_status = status;

  gimp_drawable_detach (drawable);
}

static void
alienmap2_func (const guchar *src,
                guchar       *dest,
                gint          bpp,
                gpointer      data)
{
  guchar v1, v2, v3;

  v1 = src[0];
  v2 = src[1];
  v3 = src[2];

  transform (&v1, &v2, &v3);

  dest[0] = v1;
  dest[1] = v2;
  dest[2] = v3;

  if (bpp == 4)
    dest[3] = src[3];
}

static void
alienmap2 (GimpDrawable *drawable)
{
  gimp_rgn_iterate2 (drawable, 0 /* unused */, alienmap2_func, NULL);
}

static gint
alienmap2_dialog (void)
{
  GtkWidget *dialog;
  GtkWidget *main_vbox;
  GtkWidget *top_table;
  GtkWidget *frame;
  GtkWidget *toggle;
  GtkWidget *hbox;
  GtkWidget *vbox;
  GtkWidget *table;
  GtkObject *adj;
  gboolean   run;

  gimp_ui_init (PLUG_IN_BINARY, TRUE);

  dialog = gimp_dialog_new (_("Alien Map"), PLUG_IN_BINARY,
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
                            G_CALLBACK (dialog_update_preview),
                            drawable);

  top_table = gtk_table_new (2, 2, FALSE);
  gtk_table_set_col_spacings (GTK_TABLE (top_table), 12);
  gtk_table_set_row_spacings (GTK_TABLE (top_table), 12);
  gtk_box_pack_start (GTK_BOX (main_vbox), top_table, FALSE, FALSE, 0);
  gtk_widget_show (top_table);

  /* Controls */
  table = gtk_table_new (6, 3, FALSE);
  gtk_table_set_col_spacings (GTK_TABLE (table), 6);
  gtk_table_set_row_spacings (GTK_TABLE (table), 4);
  gtk_table_attach (GTK_TABLE (top_table), table, 0, 2, 1, 2,
                    GTK_EXPAND | GTK_FILL, 0, 0, 0);
  gtk_widget_show (table);

  entry_freq_rh = adj =
    gimp_scale_entry_new (GTK_TABLE (table), 0, 0,
                          NULL, SCALE_WIDTH, ENTRY_WIDTH,
                          wvals.redfrequency, 0, 20.0, 0.1, 1, 2,
                          TRUE, 0, 0,
                          _("Number of cycles covering full value range"),
                          NULL);
  label_freq_rh = GIMP_SCALE_ENTRY_LABEL (adj);
  g_signal_connect (adj, "value-changed",
                    G_CALLBACK (dialog_scale_update),
                    &wvals.redfrequency);

  entry_phase_rh = adj =
    gimp_scale_entry_new (GTK_TABLE (table), 0, 1,
                          NULL, SCALE_WIDTH, ENTRY_WIDTH,
                          wvals.redangle, 0, 360.0, 1, 15, 2,
                          TRUE, 0, 0,
                          _("Phase angle, range 0-360"),
                          NULL);
  label_phase_rh = GIMP_SCALE_ENTRY_LABEL (adj);
  g_signal_connect (adj, "value-changed",
                    G_CALLBACK (dialog_scale_update),
                    &wvals.redangle);

  entry_freq_gs = adj =
    gimp_scale_entry_new (GTK_TABLE (table), 0, 2,
                          NULL, SCALE_WIDTH, ENTRY_WIDTH,
                          wvals.greenfrequency, 0, 20.0, 0.1, 1, 2,
                          TRUE, 0, 0,
                          _("Number of cycles covering full value range"),
                          NULL);
  label_freq_gs = GIMP_SCALE_ENTRY_LABEL (adj);
  g_signal_connect (adj, "value-changed",
                    G_CALLBACK (dialog_scale_update),
                    &wvals.greenfrequency);

  entry_phase_gs = adj =
    gimp_scale_entry_new (GTK_TABLE (table), 0, 3,
                          NULL, SCALE_WIDTH, ENTRY_WIDTH,
                          wvals.redangle, 0, 360.0, 1, 15, 2,
                          TRUE, 0, 0,
                          _("Phase angle, range 0-360"),
                          NULL);
  label_phase_gs = GIMP_SCALE_ENTRY_LABEL (adj);
  g_signal_connect (adj, "value-changed",
                    G_CALLBACK (dialog_scale_update),
                    &wvals.greenangle);

  entry_freq_bl = adj =
    gimp_scale_entry_new (GTK_TABLE (table), 0, 4,
                          NULL, SCALE_WIDTH, ENTRY_WIDTH,
                          wvals.bluefrequency, 0, 20.0, 0.1, 1, 2,
                          TRUE, 0, 0,
                          _("Number of cycles covering full value range"),
                          NULL);
  label_freq_bl = GIMP_SCALE_ENTRY_LABEL (adj);
  g_signal_connect (adj, "value-changed",
                    G_CALLBACK (dialog_scale_update),
                    &wvals.bluefrequency);

  entry_phase_bl = adj =
    gimp_scale_entry_new (GTK_TABLE (table), 0, 5,
                          NULL, SCALE_WIDTH, ENTRY_WIDTH,
                          wvals.blueangle, 0, 360.0, 1, 15, 2,
                          TRUE, 0, 0,
                          _("Phase angle, range 0-360"),
                          NULL);
  label_phase_bl = GIMP_SCALE_ENTRY_LABEL (adj);
  g_signal_connect (adj, "value-changed",
                    G_CALLBACK (dialog_scale_update),
                    &wvals.blueangle);

  /*  Mode toggle box  */
  hbox = gtk_hbox_new (FALSE, 6);
  gtk_table_attach (GTK_TABLE (top_table), hbox, 1, 2, 0, 1,
                    GTK_FILL | GTK_EXPAND, GTK_FILL | GTK_EXPAND, 0, 0);
  gtk_widget_show (hbox);

  frame =
    gimp_int_radio_group_new (TRUE, _("Mode"),
                              G_CALLBACK (alienmap2_radio_update),
                              &wvals.colormodel, wvals.colormodel,

                              _("_RGB color model"), RGB_MODEL, NULL,
                              _("_HSL color model"), HSL_MODEL, NULL,

                              NULL);

  gtk_box_pack_start (GTK_BOX (hbox), frame, FALSE, FALSE, 0);

  vbox = gtk_vbox_new (FALSE, 6);
  gtk_box_pack_start (GTK_BOX (hbox), vbox, FALSE, FALSE, 0);
  gtk_widget_show (vbox);

  toggle_modify_rh = toggle = gtk_check_button_new_with_mnemonic (NULL);
  gtk_box_pack_start (GTK_BOX (vbox), toggle, FALSE, FALSE, 0);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (toggle), wvals.redmode);
  gtk_widget_show (toggle);

  g_signal_connect (toggle, "toggled",
                    G_CALLBACK (alienmap2_toggle_update),
                    &wvals.redmode);

  toggle_modify_gs = toggle = gtk_check_button_new_with_mnemonic (NULL);
  gtk_box_pack_start (GTK_BOX (vbox), toggle, FALSE, FALSE, 0);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (toggle), wvals.greenmode);
  gtk_widget_show (toggle);

  g_signal_connect (toggle, "toggled",
                    G_CALLBACK (alienmap2_toggle_update),
                    &wvals.greenmode);

  toggle_modify_bl = toggle = gtk_check_button_new_with_mnemonic (NULL);
  gtk_box_pack_start (GTK_BOX (vbox), toggle, FALSE, FALSE, 0);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (toggle), wvals.bluemode);
  gtk_widget_show (toggle);

  g_signal_connect (toggle, "toggled",
                    G_CALLBACK (alienmap2_toggle_update),
                    &wvals.bluemode);

  gtk_widget_show (frame);
  gtk_widget_show (dialog);

  alienmap2_get_label_size ();
  alienmap2_set_labels ();
  alienmap2_set_sensitive ();

  run = (gimp_dialog_run (GIMP_DIALOG (dialog)) == GTK_RESPONSE_OK);

  gtk_widget_destroy (dialog);

  return run;
}

static void
dialog_update_preview (GimpDrawable *drawable,
                       GimpPreview  *preview)
{
  guchar *dest, *src;
  gint    width, height, bpp;
  gint    i;

  src = gimp_zoom_preview_get_source (GIMP_ZOOM_PREVIEW (preview),
                                      &width, &height, &bpp);
  dest = g_new (guchar, width * height * bpp);

  for (i = 0 ; i < width * height ; i++)
    alienmap2_func (src + i * bpp, dest + i * bpp, bpp, NULL);

  gimp_preview_draw_buffer (preview, dest, width * bpp);

  g_free (src);
  g_free (dest);
}

static void
dialog_scale_update (GtkAdjustment *adjustment,
                     gdouble       *value)
{
  gimp_double_adjustment_update (adjustment, value);

  gimp_preview_invalidate (GIMP_PREVIEW (preview));
}

static void
alienmap2_toggle_update (GtkWidget *widget,
                         gpointer   data)
{
  gimp_toggle_button_update (widget, data);

  alienmap2_set_sensitive ();

  gimp_preview_invalidate (GIMP_PREVIEW (preview));
}

static void
alienmap2_radio_update (GtkWidget *widget,
                        gpointer   data)
{
  gimp_radio_button_update (widget, data);

  alienmap2_set_labels ();

  gimp_preview_invalidate (GIMP_PREVIEW (preview));
}


static void
alienmap2_set_sensitive (void)
{
  gimp_scale_entry_set_sensitive (entry_freq_rh,  wvals.redmode);
  gimp_scale_entry_set_sensitive (entry_phase_rh, wvals.redmode);
  gimp_scale_entry_set_sensitive (entry_freq_gs,  wvals.greenmode);
  gimp_scale_entry_set_sensitive (entry_phase_gs, wvals.greenmode);
  gimp_scale_entry_set_sensitive (entry_freq_bl,  wvals.bluemode);
  gimp_scale_entry_set_sensitive (entry_phase_bl, wvals.bluemode);
}


static void
alienmap2_set_labels (void)
{
  gtk_button_set_label (GTK_BUTTON (toggle_modify_rh),
                        gettext (ctext[0][wvals.colormodel]));
  gtk_button_set_label (GTK_BUTTON (toggle_modify_gs),
                        gettext (ctext[1][wvals.colormodel]));
  gtk_button_set_label (GTK_BUTTON (toggle_modify_bl),
                        gettext (ctext[2][wvals.colormodel]));

  gtk_label_set_text_with_mnemonic (GTK_LABEL (label_freq_rh),
                                    gettext (etext[0][wvals.colormodel]));
  gtk_label_set_text_with_mnemonic (GTK_LABEL (label_freq_gs),
                                    gettext (etext[1][wvals.colormodel]));
  gtk_label_set_text_with_mnemonic (GTK_LABEL (label_freq_bl),
                                    gettext (etext[2][wvals.colormodel]));
  gtk_label_set_text_with_mnemonic (GTK_LABEL (label_phase_rh),
                                    gettext (etext[3][wvals.colormodel]));
  gtk_label_set_text_with_mnemonic (GTK_LABEL (label_phase_gs),
                                    gettext (etext[4][wvals.colormodel]));
  gtk_label_set_text_with_mnemonic (GTK_LABEL (label_phase_bl),
                                    gettext (etext[5][wvals.colormodel]));

  gtk_widget_set_size_request (label_freq_rh, elabel_maxwidth, -1);
}


static void
alienmap2_get_label_size (void)
{
  PangoLayout *layout;
  gint         width;
  gint         i, j;

  elabel_maxwidth = 0;

  for (i = 0; i < 6; i++)
    for (j = 0; j < 2; j++)
      {
        gtk_label_set_text_with_mnemonic (GTK_LABEL (label_freq_rh),
                                          gettext (etext[i][j]));
        layout = gtk_label_get_layout (GTK_LABEL (label_freq_rh));
        pango_layout_get_pixel_size (layout, &width, NULL);

        if (width > elabel_maxwidth)
          elabel_maxwidth = width;
      }
}
