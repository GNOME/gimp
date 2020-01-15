/* Sparkle --- image filter plug-in for GIMP
 * Copyright (C) 1996 by John Beale;  ported to Gimp by Michael J. Hammel;
 *
 * It has been optimized a little, bugfixed and modified by Martin Weber
 * for additional functionality.  Also bugfixed by Seth Burgess (9/17/03)
 * to take rowstrides into account when selections are present (bug #50911).
 * Attempted reformatting.
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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 *
 * You can contact Michael at mjhammel@csn.net
 * You can contact Martin at martweb@gmx.net
 * You can contact Seth at sjburges@gimp.org
 */

/*
 * Sparkle 1.27 - simulate pixel bloom and diffraction effects
 */

#include "config.h"

#include <string.h>

#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

#include "libgimp/stdplugins-intl.h"


#define PLUG_IN_PROC   "plug-in-sparkle"
#define PLUG_IN_BINARY "sparkle"
#define PLUG_IN_ROLE   "gimp-sparkle"

#define SCALE_WIDTH    175
#define ENTRY_WIDTH      7
#define MAX_CHANNELS     4
#define PSV              2  /* point spread value */

#define NATURAL          0
#define FOREGROUND       1
#define BACKGROUND       2


typedef struct
{
  gdouble   lum_threshold;
  gdouble   flare_inten;
  gdouble   spike_len;
  gdouble   spike_pts;
  gdouble   spike_angle;
  gdouble   density;
  gdouble   transparency;
  gdouble   random_hue;
  gdouble   random_saturation;
  gboolean  preserve_luminosity;
  gboolean  inverse;
  gboolean  border;
  gint      colortype;
} SparkleVals;


typedef struct _Sparkle      Sparkle;
typedef struct _SparkleClass SparkleClass;

struct _Sparkle
{
  GimpPlugIn parent_instance;
};

struct _SparkleClass
{
  GimpPlugInClass parent_class;
};


#define SPARKLE_TYPE  (sparkle_get_type ())
#define SPARKLE (obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), SPARKLE_TYPE, Sparkle))

GType                   sparkle_get_type         (void) G_GNUC_CONST;

static GList          * sparkle_query_procedures (GimpPlugIn           *plug_in);
static GimpProcedure  * sparkle_create_procedure (GimpPlugIn           *plug_in,
                                                  const gchar          *name);

static GimpValueArray * sparkle_run              (GimpProcedure        *procedure,
                                                  GimpRunMode           run_mode,
                                                  GimpImage            *image,
                                                  GimpDrawable         *drawable,
                                                  const GimpValueArray *args,
                                                  gpointer              run_data);

static gboolean         sparkle_dialog           (GimpDrawable         *drawable);

static gint             compute_luminosity       (const guchar         *pixel,
                                                  gboolean              gray,
                                                  gboolean              has_alpha);
static gint             compute_lum_threshold    (GimpDrawable         *drawable,
                                                  gdouble               percentile);
static void             sparkle                  (GimpDrawable         *drawable,
                                                  GimpPreview          *preview);
static void             sparkle_preview          (GimpDrawable         *drawable,
                                                  GimpPreview          *preview);
static void             fspike                   (GeglBuffer           *src_buffer,
                                                  GeglBuffer           *dest_buffer,
                                                  const Babl           *format,
                                                  gint                  bytes,
                                                  gint                  x1,
                                                  gint                  y1,
                                                  gint                  x2,
                                                  gint                  y2,
                                                  gint                  xr,
                                                  gint                  yr,
                                                  gdouble               inten,
                                                  gdouble               length,
                                                  gdouble               angle,
                                                  GRand                *gr,
                                                  guchar               *dest_buf);
static void             rpnt                     (GeglBuffer           *dest_buffer,
                                                  const Babl           *format,
                                                  gint                  x1,
                                                  gint                  y1,
                                                  gint                  x2,
                                                  gint                  y2,
                                                  gdouble               xr,
                                                  gdouble               yr,
                                                  gint                  bytes,
                                                  gdouble               inten,
                                                  guchar                color[MAX_CHANNELS],
                                                  guchar               *dest_buf);


G_DEFINE_TYPE (Sparkle, sparkle, GIMP_TYPE_PLUG_IN)

GIMP_MAIN (SPARKLE_TYPE)


static SparkleVals svals =
{
  0.001,   /* luminosity threshold */
  0.5,     /* flare intensity      */
  20.0,    /* spike length         */
  4.0,     /* spike points         */
  15.0,    /* spike angle          */
  1.0,     /* spike density        */
  0.0,     /* transparency         */
  0.0,     /* random hue           */
  0.0,     /* random saturation    */
  FALSE,   /* preserve_luminosity  */
  FALSE,   /* inverse              */
  FALSE,   /* border               */
  NATURAL  /* colortype            */
};

static gint num_sparkles;


static void
sparkle_class_init (SparkleClass *klass)
{
  GimpPlugInClass *plug_in_class = GIMP_PLUG_IN_CLASS (klass);

  plug_in_class->query_procedures = sparkle_query_procedures;
  plug_in_class->create_procedure = sparkle_create_procedure;
}

static void
sparkle_init (Sparkle *sparkle)
{
}

static GList *
sparkle_query_procedures (GimpPlugIn *plug_in)
{
  return g_list_append (NULL, g_strdup (PLUG_IN_PROC));
}

static GimpProcedure *
sparkle_create_procedure (GimpPlugIn  *plug_in,
                          const gchar *name)
{
  GimpProcedure *procedure = NULL;

  if (! strcmp (name, PLUG_IN_PROC))
    {
      procedure = gimp_image_procedure_new (plug_in, name,
                                            GIMP_PDB_PROC_TYPE_PLUGIN,
                                            sparkle_run, NULL, NULL);

      gimp_procedure_set_image_types (procedure, "RGB*, GRAY*");

      gimp_procedure_set_menu_label (procedure, N_("_Sparkle..."));
      gimp_procedure_add_menu_path (procedure,
                                    "<Image>/Filters/Light and Shadow/Light");

      gimp_procedure_set_documentation (procedure,
                                        N_("Turn bright spots into "
                                           "starry sparkles"),
                                        "Uses a percentage based luminoisty "
                                        "threhsold to find candidate pixels "
                                        "for adding some sparkles (spikes).",
                                        name);
      gimp_procedure_set_attribution (procedure,
                                      "John Beale, & (ported to GIMP v0.54) "
                                      "Michael J. Hammel & ted to GIMP v1.0) "
                                      "& Seth Burgess & Spencer Kimball",
                                      "John Beale",
                                      "Version 1.27, September 2003");

      GIMP_PROC_ARG_DOUBLE (procedure, "lum-threshold",
                            "Lum threshold",
                            "Luminosity threshold",
                            0.0, 1.0, 0.001,
                            G_PARAM_READWRITE);

      GIMP_PROC_ARG_DOUBLE (procedure, "flare-inten",
                            "Flare inten",
                            "Flare intensity",
                            0.0, 1.0, 0.5,
                            G_PARAM_READWRITE);

      GIMP_PROC_ARG_INT (procedure, "spike-len",
                         "Spike len",
                         "Spike length (in pixels)",
                         1, 1000, 20,
                         G_PARAM_READWRITE);

      GIMP_PROC_ARG_INT (procedure, "spike-points",
                         "Spike points",
                         "# of spike points",
                         1, 1000, 4,
                         G_PARAM_READWRITE);

      GIMP_PROC_ARG_INT (procedure, "spike-angle",
                         "Spike angle",
                         "Spike angle (-1: random)",
                         -1, 360, 15,
                         G_PARAM_READWRITE);

      GIMP_PROC_ARG_DOUBLE (procedure, "density",
                            "Density",
                            "Spike density",
                            0.0, 1.0, 1.0,
                            G_PARAM_READWRITE);

      GIMP_PROC_ARG_DOUBLE (procedure, "transparency",
                            "Transparency",
                            "Transparency",
                            0.0, 1.0, 0.0,
                            G_PARAM_READWRITE);

      GIMP_PROC_ARG_DOUBLE (procedure, "random-hue",
                            "Random hue",
                            "Random hue",
                            0.0, 1.0, 0.0,
                            G_PARAM_READWRITE);

      GIMP_PROC_ARG_DOUBLE (procedure, "random-saturation",
                            "Random saturation",
                            "Random saturation",
                            0.0, 1.0, 0.0,
                            G_PARAM_READWRITE);

      GIMP_PROC_ARG_BOOLEAN (procedure, "preserve-luminosity",
                             "Preserve luminosity",
                             "Preserve luminosity",
                             FALSE,
                             G_PARAM_READWRITE);

      GIMP_PROC_ARG_BOOLEAN (procedure, "inverse",
                             "Inverse",
                             "Inverse",
                             FALSE,
                             G_PARAM_READWRITE);

      GIMP_PROC_ARG_BOOLEAN (procedure, "border",
                             "Border",
                             "Add border",
                             FALSE,
                             G_PARAM_READWRITE);

      GIMP_PROC_ARG_INT (procedure, "color-type",
                         "Color type",
                         "Color of sparkles: { NATURAL (0), "
                         "FOREGROUND (1), BACKGROUND (2) }",
                         0, 2, NATURAL,
                         G_PARAM_READWRITE);
    }

  return procedure;
}

static GimpValueArray *
sparkle_run (GimpProcedure        *procedure,
             GimpRunMode           run_mode,
             GimpImage            *image,
             GimpDrawable         *drawable,
             const GimpValueArray *args,
             gpointer              run_data)
{
  gint x, y, w, h;

  INIT_I18N ();
  gegl_init (NULL, NULL);

  if (! gimp_drawable_mask_intersect (drawable, &x, &y, &w, &h))
    {
      g_message (_("Region selected for filter is empty"));

      return gimp_procedure_new_return_values (procedure,
                                               GIMP_PDB_SUCCESS,
                                               NULL);
    }

  switch (run_mode)
    {
    case GIMP_RUN_INTERACTIVE:
      gimp_get_data (PLUG_IN_PROC, &svals);

      if (! sparkle_dialog (drawable))
        {
          return gimp_procedure_new_return_values (procedure,
                                                   GIMP_PDB_CANCEL,
                                                   NULL);
        }
      break;

    case GIMP_RUN_NONINTERACTIVE:
      svals.lum_threshold       = GIMP_VALUES_GET_DOUBLE  (args, 0);
      svals.flare_inten         = GIMP_VALUES_GET_DOUBLE  (args, 1);
      svals.spike_len           = GIMP_VALUES_GET_INT     (args, 2);
      svals.spike_pts           = GIMP_VALUES_GET_INT     (args, 3);
      svals.spike_angle         = GIMP_VALUES_GET_INT     (args, 4);
      svals.density             = GIMP_VALUES_GET_DOUBLE  (args, 5);
      svals.transparency        = GIMP_VALUES_GET_DOUBLE  (args, 6);
      svals.random_hue          = GIMP_VALUES_GET_DOUBLE  (args, 7);
      svals.random_saturation   = GIMP_VALUES_GET_DOUBLE  (args, 8);
      svals.preserve_luminosity = GIMP_VALUES_GET_BOOLEAN (args, 9);
      svals.inverse             = GIMP_VALUES_GET_BOOLEAN (args, 10);
      svals.border              = GIMP_VALUES_GET_BOOLEAN (args, 11);
      svals.colortype           = GIMP_VALUES_GET_INT     (args, 12);
      break;

    case GIMP_RUN_WITH_LAST_VALS:
      gimp_get_data (PLUG_IN_PROC, &svals);
      break;
    }

  if (gimp_drawable_is_rgb (drawable) ||
      gimp_drawable_is_gray (drawable))
    {
      gimp_progress_init (_("Sparkling"));

      sparkle (drawable, NULL);

      if (run_mode != GIMP_RUN_NONINTERACTIVE)
        gimp_displays_flush ();

      if (run_mode == GIMP_RUN_INTERACTIVE)
        gimp_set_data (PLUG_IN_PROC, &svals, sizeof (SparkleVals));
    }
  else
    {
      return gimp_procedure_new_return_values (procedure,
                                               GIMP_PDB_EXECUTION_ERROR,
                                               NULL);
    }

  return gimp_procedure_new_return_values (procedure, GIMP_PDB_SUCCESS, NULL);
}

static gboolean
sparkle_dialog (GimpDrawable *drawable)
{
  GtkWidget     *dialog;
  GtkWidget     *main_vbox;
  GtkWidget     *preview;
  GtkWidget     *vbox;
  GtkWidget     *hbox;
  GtkWidget     *grid;
  GtkWidget     *toggle;
  GtkWidget     *r1, *r2, *r3;
  GtkAdjustment *scale_data;
  gboolean       run;

  gimp_ui_init (PLUG_IN_BINARY);

  dialog = gimp_dialog_new (_("Sparkle"), PLUG_IN_ROLE,
                            NULL, 0,
                            gimp_standard_help_func, PLUG_IN_PROC,

                            _("_Cancel"), GTK_RESPONSE_CANCEL,
                            _("_OK"),     GTK_RESPONSE_OK,

                            NULL);

  gimp_dialog_set_alternative_button_order (GTK_DIALOG (dialog),
                                           GTK_RESPONSE_OK,
                                           GTK_RESPONSE_CANCEL,
                                           -1);

  gimp_window_set_transient (GTK_WINDOW (dialog));

  main_vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 12);
  gtk_container_set_border_width (GTK_CONTAINER (main_vbox), 12);
  gtk_box_pack_start (GTK_BOX (gtk_dialog_get_content_area (GTK_DIALOG (dialog))),
                      main_vbox, TRUE, TRUE, 0);
  gtk_widget_show (main_vbox);

  preview = gimp_drawable_preview_new_from_drawable (drawable);
  gtk_box_pack_start (GTK_BOX (main_vbox), preview, TRUE, TRUE, 0);
  gtk_widget_show (preview);
  g_signal_connect_swapped (preview, "invalidated",
                            G_CALLBACK (sparkle_preview),
                            drawable);

  grid = gtk_grid_new ();
  gtk_grid_set_row_spacing (GTK_GRID (grid), 6);
  gtk_grid_set_column_spacing (GTK_GRID (grid), 6);
  gtk_box_pack_start (GTK_BOX (main_vbox), grid, FALSE, FALSE, 0);
  gtk_widget_show (grid);

  scale_data =
    gimp_scale_entry_new (GTK_GRID (grid), 0, 0,
              _("Luminosity _threshold:"), SCALE_WIDTH, ENTRY_WIDTH,
              svals.lum_threshold, 0.0, 0.1, 0.001, 0.01, 3,
              TRUE, 0, 0,
              _("Adjust the luminosity threshold"), NULL);
  g_signal_connect (scale_data, "value-changed",
                    G_CALLBACK (gimp_double_adjustment_update),
                    &svals.lum_threshold);
  g_signal_connect_swapped (scale_data, "value-changed",
                            G_CALLBACK (gimp_preview_invalidate),
                            preview);

  scale_data =
    gimp_scale_entry_new (GTK_GRID (grid), 0, 1,
              _("F_lare intensity:"), SCALE_WIDTH, ENTRY_WIDTH,
              svals.flare_inten, 0.0, 1.0, 0.01, 0.1, 2,
              TRUE, 0, 0,
              _("Adjust the flare intensity"), NULL);
  g_signal_connect (scale_data, "value-changed",
                    G_CALLBACK (gimp_double_adjustment_update),
                    &svals.flare_inten);
  g_signal_connect_swapped (scale_data, "value-changed",
                            G_CALLBACK (gimp_preview_invalidate),
                            preview);

  scale_data =
    gimp_scale_entry_new (GTK_GRID (grid), 0, 2,
              _("_Spike length:"), SCALE_WIDTH, ENTRY_WIDTH,
              svals.spike_len, 1, 100, 1, 10, 0,
              TRUE, 0, 0,
              _("Adjust the spike length"), NULL);
  g_signal_connect (scale_data, "value-changed",
                    G_CALLBACK (gimp_double_adjustment_update),
                    &svals.spike_len);
  g_signal_connect_swapped (scale_data, "value-changed",
                            G_CALLBACK (gimp_preview_invalidate),
                            preview);

  scale_data =
    gimp_scale_entry_new (GTK_GRID (grid), 0, 3,
              _("Sp_ike points:"), SCALE_WIDTH, ENTRY_WIDTH,
              svals.spike_pts, 0, 16, 1, 4, 0,
              TRUE, 0, 0,
              _("Adjust the number of spikes"), NULL);
  g_signal_connect (scale_data, "value-changed",
                    G_CALLBACK (gimp_double_adjustment_update),
                    &svals.spike_pts);
  g_signal_connect_swapped (scale_data, "value-changed",
                            G_CALLBACK (gimp_preview_invalidate),
                            preview);

  scale_data =
    gimp_scale_entry_new (GTK_GRID (grid), 0, 4,
              _("Spi_ke angle (-1: random):"), SCALE_WIDTH, ENTRY_WIDTH,
              svals.spike_angle, -1, 360, 1, 15, 0,
              TRUE, 0, 0,
              _("Adjust the spike angle "
                "(-1 causes a random angle to be chosen)"), NULL);
  g_signal_connect (scale_data, "value-changed",
                    G_CALLBACK (gimp_double_adjustment_update),
                    &svals.spike_angle);
  g_signal_connect_swapped (scale_data, "value-changed",
                            G_CALLBACK (gimp_preview_invalidate),
                            preview);

  scale_data =
    gimp_scale_entry_new (GTK_GRID (grid), 0, 5,
              _("Spik_e density:"), SCALE_WIDTH, ENTRY_WIDTH,
              svals.density, 0.0, 1.0, 0.01, 0.1, 2,
              TRUE, 0, 0,
              _("Adjust the spike density"), NULL);
  g_signal_connect (scale_data, "value-changed",
                    G_CALLBACK (gimp_double_adjustment_update),
                    &svals.density);
  g_signal_connect_swapped (scale_data, "value-changed",
                            G_CALLBACK (gimp_preview_invalidate),
                            preview);

  scale_data =
    gimp_scale_entry_new (GTK_GRID (grid), 0, 6,
              _("Tr_ansparency:"), SCALE_WIDTH, ENTRY_WIDTH,
              svals.transparency, 0.0, 1.0, 0.01, 0.1, 2,
              TRUE, 0, 0,
              _("Adjust the opacity of the spikes"), NULL);
  g_signal_connect (scale_data, "value-changed",
                    G_CALLBACK (gimp_double_adjustment_update),
                    &svals.transparency);
  g_signal_connect_swapped (scale_data, "value-changed",
                            G_CALLBACK (gimp_preview_invalidate),
                            preview);

  scale_data =
    gimp_scale_entry_new (GTK_GRID (grid), 0, 7,
              _("_Random hue:"), SCALE_WIDTH, ENTRY_WIDTH,
              svals.random_hue, 0.0, 1.0, 0.01, 0.1, 2,
              TRUE, 0, 0,
              _("Adjust how much the hue should be changed randomly"), NULL);
  g_signal_connect (scale_data, "value-changed",
                    G_CALLBACK (gimp_double_adjustment_update),
                    &svals.random_hue);
  g_signal_connect_swapped (scale_data, "value-changed",
                            G_CALLBACK (gimp_preview_invalidate),
                            preview);

  scale_data =
    gimp_scale_entry_new (GTK_GRID (grid), 0, 8,
              _("Rando_m saturation:"), SCALE_WIDTH, ENTRY_WIDTH,
              svals.random_saturation, 0.0, 1.0, 0.01, 0.1, 2,
              TRUE, 0, 0,
              _("Adjust how much the saturation should be changed randomly"),
              NULL);
  g_signal_connect (scale_data, "value-changed",
                    G_CALLBACK (gimp_double_adjustment_update),
                    &svals.random_saturation);
  g_signal_connect_swapped (scale_data, "value-changed",
                            G_CALLBACK (gimp_preview_invalidate),
                            preview);

  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 12);
  gtk_box_pack_start (GTK_BOX (main_vbox), hbox, FALSE, FALSE, 0);
  gtk_widget_show (hbox);

  vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 2);
  gtk_box_pack_start (GTK_BOX (hbox), vbox, TRUE, TRUE, 0);
  gtk_widget_show (vbox);

  toggle = gtk_check_button_new_with_mnemonic (_("_Preserve luminosity"));
  gtk_box_pack_start (GTK_BOX (vbox), toggle, FALSE, FALSE, 0);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (toggle),
                svals.preserve_luminosity);
  gtk_widget_show (toggle);

  gimp_help_set_help_data (toggle,
                           _("Should the luminosity be preserved?"), NULL);

  g_signal_connect (toggle, "toggled",
                    G_CALLBACK (gimp_toggle_button_update),
                    &svals.preserve_luminosity);
  g_signal_connect_swapped (toggle, "toggled",
                            G_CALLBACK (gimp_preview_invalidate),
                            preview);

  toggle = gtk_check_button_new_with_mnemonic (_("In_verse"));
  gtk_box_pack_start (GTK_BOX (vbox), toggle, FALSE, FALSE, 0);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (toggle), svals.inverse);
  gtk_widget_show (toggle);

  gimp_help_set_help_data (toggle,
                           _("Should the effect be inversed?"), NULL);

  g_signal_connect (toggle, "toggled",
                    G_CALLBACK (gimp_toggle_button_update),
                    &svals.inverse);
  g_signal_connect_swapped (toggle, "toggled",
                            G_CALLBACK (gimp_preview_invalidate),
                            preview);

  toggle = gtk_check_button_new_with_mnemonic (_("A_dd border"));
  gtk_box_pack_start (GTK_BOX (vbox), toggle, FALSE, FALSE, 0);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (toggle), svals.border);
  gtk_widget_show (toggle);

  gimp_help_set_help_data (toggle,
                           _("Draw a border of spikes around the image"), NULL);

  g_signal_connect (toggle, "toggled",
                    G_CALLBACK (gimp_toggle_button_update),
                    &svals.border);
  g_signal_connect_swapped (toggle, "toggled",
                            G_CALLBACK (gimp_preview_invalidate),
                            preview);

  /*  colortype  */
  vbox = gimp_int_radio_group_new (FALSE, NULL,
                                   G_CALLBACK (gimp_radio_button_update),
                                   &svals.colortype, NULL, svals.colortype,

                                   _("_Natural color"),    NATURAL,    &r1,
                                   _("_Foreground color"), FOREGROUND, &r2,
                                   _("_Background color"), BACKGROUND, &r3,

                                   NULL);

  gtk_box_pack_start (GTK_BOX (hbox), vbox, TRUE, TRUE, 0);
  gtk_widget_show (vbox);

  gimp_help_set_help_data (r1, _("Use the color of the image"), NULL);
  gimp_help_set_help_data (r2, _("Use the foreground color"), NULL);
  gimp_help_set_help_data (r3, _("Use the background color"), NULL);
  g_signal_connect_swapped (r1, "toggled",
                            G_CALLBACK (gimp_preview_invalidate),
                            preview);
  g_signal_connect_swapped (r2, "toggled",
                            G_CALLBACK (gimp_preview_invalidate),
                            preview);
  g_signal_connect_swapped (r3, "toggled",
                            G_CALLBACK (gimp_preview_invalidate),
                            preview);

  gtk_widget_show (dialog);

  run = (gimp_dialog_run (GIMP_DIALOG (dialog)) == GTK_RESPONSE_OK);

  gtk_widget_destroy (dialog);

  return run;
}

static gint
compute_luminosity (const guchar *pixel,
                    gboolean      gray,
                    gboolean      has_alpha)
{
  gint pixel0, pixel1, pixel2;

  if (svals.inverse)
    {
      pixel0 = 255 - pixel[0];
      pixel1 = 255 - pixel[1];
      pixel2 = 255 - pixel[2];
    }
  else
    {
      pixel0 = pixel[0];
      pixel1 = pixel[1];
      pixel2 = pixel[2];
    }

  if (gray)
    {
      if (has_alpha)
        return (pixel0 * pixel1) / 255;
      else
        return (pixel0);
    }
  else
    {
      gint min, max;

      min = MIN (pixel0, pixel1);
      min = MIN (min, pixel2);
      max = MAX (pixel0, pixel1);
      max = MAX (max, pixel2);

      if (has_alpha)
        return ((min + max) * pixel[3]) / 510;
      else
        return (min + max) / 2;
    }
}

static gint
compute_lum_threshold (GimpDrawable *drawable,
                       gdouble percentile)
{
  GeglBuffer         *src_buffer;
  GeglBufferIterator *iter;
  const Babl         *format;
  gint                bpp;
  gint                values[256];
  gint                total, sum;
  gboolean            gray;
  gboolean            has_alpha;
  gint                i;
  gint                x1, y1;
  gint                width, height;

  /*  zero out the luminosity values array  */
  memset (values, 0, sizeof (gint) * 256);

  if (! gimp_drawable_mask_intersect (drawable,
                                      &x1, &y1, &width, &height))
    return 0;

  gray = gimp_drawable_is_gray (drawable);
  has_alpha = gimp_drawable_has_alpha (drawable);

  if (gray)
    {
      if (has_alpha)
        format = babl_format ("Y'A u8");
      else
        format = babl_format ("Y' u8");
    }
  else
    {
      if (has_alpha)
        format = babl_format ("R'G'B'A u8");
      else
        format = babl_format ("R'G'B' u8");
    }

  bpp = babl_format_get_bytes_per_pixel (format);

  src_buffer = gimp_drawable_get_buffer (drawable);

  iter = gegl_buffer_iterator_new (src_buffer,
                                   GEGL_RECTANGLE (x1, y1, width, height), 0,
                                   format,
                                   GEGL_ACCESS_READ, GEGL_ABYSS_NONE, 1);

  while (gegl_buffer_iterator_next (iter))
    {
      const guchar *src    = iter->items[0].data;
      gint          length = iter->length;

      while (length--)
        {
          values [compute_luminosity (src, gray, has_alpha)]++;
          src += bpp;
        }
    }

  g_object_unref (src_buffer);

  total = width * height;
  sum = 0;

  for (i = 255; i >= 0; i--)
    {
      sum += values[i];
      if ((gdouble) sum > percentile * (gdouble) total)
        {
           num_sparkles = sum;
           return i;
        }
    }

  return 0;
}

static void
sparkle (GimpDrawable *drawable,
         GimpPreview  *preview)
{
  GeglBuffer         *src_buffer;
  GeglBuffer         *dest_buffer;
  GeglBufferIterator *iter;
  const Babl         *format;
  gint                d_width, d_height;
  gdouble             nfrac, length, inten, spike_angle;
  gint                cur_progress, max_progress;
  gint                x1, y1, x2, y2;
  gint                width, height;
  gint                threshold;
  gint                lum, x, y, b;
  gboolean            gray, has_alpha;
  gint                alpha;
  gint                bytes;
  GRand              *gr;
  guchar             *dest_buf = NULL;

  gray = gimp_drawable_is_gray (drawable);
  has_alpha = gimp_drawable_has_alpha (drawable);

  if (gray)
    {
      if (has_alpha)
        format = babl_format ("Y'A u8");
      else
        format = babl_format ("Y' u8");
    }
  else
    {
      if (has_alpha)
        format = babl_format ("R'G'B'A u8");
      else
        format = babl_format ("R'G'B' u8");
    }

  bytes = babl_format_get_bytes_per_pixel (format);

  alpha = (has_alpha) ? bytes - 1 : bytes;

  if (preview)
    {
      gimp_preview_get_position (preview, &x1, &y1);
      gimp_preview_get_size (preview, &width, &height);

      x2 = x1 + width;
      y2 = y1 + height;
      dest_buf = g_new0 (guchar, width * height * bytes);
    }
  else
    {
      if (! gimp_drawable_mask_intersect (drawable,
                                          &x1, &y1, &width, &height))
        return;

      x2 = x1 + width;
      y2 = y1 + height;
    }

  if (width < 1 || height < 1)
    return;

  d_width  = gimp_drawable_width  (drawable);
  d_height = gimp_drawable_height (drawable);

  gr = g_rand_new ();

  if (svals.border)
    {
      num_sparkles = 2 * (width + height);
      threshold = 255;
    }
  else
    {
      /*  compute the luminosity which exceeds the luminosity threshold  */
      threshold = compute_lum_threshold (drawable, svals.lum_threshold);
    }

  /* initialize the progress dialog */
  cur_progress = 0;
  max_progress = num_sparkles;

  /* copy what is already there */
  src_buffer  = gimp_drawable_get_buffer (drawable);
  dest_buffer = gimp_drawable_get_shadow_buffer (drawable);

  iter = gegl_buffer_iterator_new (src_buffer,
                                   GEGL_RECTANGLE (x1, y1, width, height), 0,
                                   format,
                                   GEGL_ACCESS_READ, GEGL_ABYSS_NONE, 2);

  gegl_buffer_iterator_add (iter, dest_buffer,
                            GEGL_RECTANGLE (x1, y1, width, height), 0,
                            format,
                            GEGL_ACCESS_WRITE, GEGL_ABYSS_NONE);

  while (gegl_buffer_iterator_next (iter))
    {
      GeglRectangle  roi = iter->items[0].roi;
      const guchar  *src,  *s;
      guchar        *dest, *d;

      src = iter->items[0].data;
      if (preview)
        dest = dest_buf + (((roi.y - y1) * width) + (roi.x - x1)) * bytes;
      else
        dest = iter->items[1].data;

      for (y = 0; y < roi.height; y++)
        {
          s = src;
          d = dest;

          for (x = 0; x < roi.width; x++)
            {
              if (has_alpha && s[alpha] == 0)
                {
                  memset (d, 0, alpha);
                }
               else
                {
                  for (b = 0; b < alpha; b++)
                    d[b] = s[b];
                }

              if (has_alpha)
                d[alpha] = s[alpha];

              s += bytes;
              d += bytes;
            }

          src += roi.width * bytes;
          if (preview)
            dest += width * bytes;
          else
            dest += roi.width * bytes;
        }
    }

  /* add effects to new image based on intensity of old pixels */

  iter = gegl_buffer_iterator_new (src_buffer,
                                   GEGL_RECTANGLE (x1, y1, width, height), 0,
                                   format,
                                   GEGL_ACCESS_READ, GEGL_ABYSS_NONE, 2);

  gegl_buffer_iterator_add (iter, dest_buffer,
                            GEGL_RECTANGLE (x1, y1, width, height), 0,
                            format,
                            GEGL_ACCESS_WRITE, GEGL_ABYSS_NONE);

  while (gegl_buffer_iterator_next (iter))
    {
      GeglRectangle  roi = iter->items[0].roi;
      const guchar  *src, *s;

      src = iter->items[0].data;

      for (y = 0; y < roi.height; y++)
        {
          s = src;

          for (x = 0; x < roi.width; x++)
            {
              if (svals.border)
                {
                  if (x + roi.x == 0 ||
                      y + roi.y == 0 ||
                      x + roi.x == d_width  - 1 ||
                      y + roi.y == d_height - 1)
                    {
                      lum = 255;
                    }
                  else
                    {
                      lum = 0;
                    }
                }
              else
                {
                  lum = compute_luminosity (s, gray, has_alpha);
                }

              if (lum >= threshold)
                {
                  nfrac = fabs ((gdouble) (lum + 1 - threshold) /
                                (gdouble) (256 - threshold));
                  length = ((gdouble) svals.spike_len *
                            (gdouble) pow (nfrac, 0.8));
                  inten = svals.flare_inten * nfrac;

                  /* fspike im x,y intens rlength angle */
                  if (svals.spike_pts > 0)
                    {
                      /* major spikes */
                      if (svals.spike_angle == -1)
                        spike_angle = g_rand_double_range (gr, 0, 360.0);
                      else
                        spike_angle = svals.spike_angle;

                      if (g_rand_double (gr) <= svals.density)
                        {
                          fspike (src_buffer, dest_buffer, format, bytes,
                                  x1, y1, x2, y2,
                                  x + roi.x, y + roi.y,
                                  inten, length, spike_angle, gr, dest_buf);

                          /* minor spikes */
                          fspike (src_buffer, dest_buffer, format, bytes,
                                  x1, y1, x2, y2,
                                  x + roi.x, y + roi.y,
                                  inten * 0.7, length * 0.7,
                                  ((gdouble)spike_angle+180.0/svals.spike_pts),
                                  gr, dest_buf);
                        }
                    }
                  if (!preview)
                    {
                      cur_progress ++;

                      if ((cur_progress % 5) == 0)
                        gimp_progress_update ((double) cur_progress /
                                              (double) max_progress);
                    }
                }

              s += bytes;
            }

          src += roi.width * bytes;
        }
    }

  g_object_unref (src_buffer);
  g_object_unref (dest_buffer);

  if (preview)
    {
      gimp_preview_draw_buffer (preview, dest_buf, width * bytes);
      g_free (dest_buf);
    }
  else
    {
      gimp_progress_update (1.0);

      gimp_drawable_merge_shadow (drawable, TRUE);
      gimp_drawable_update (drawable, x1, y1, width, height);
    }

  g_rand_free (gr);
}

static void
sparkle_preview (GimpDrawable *drawable,
                 GimpPreview  *preview)
{
  sparkle (drawable, preview);
}

static inline void
rpnt (GeglBuffer   *dest_buffer,
      const Babl   *format,
      gint          x1,
      gint          y1,
      gint          x2,
      gint          y2,
      gdouble       xr,
      gdouble       yr,
      gint          bytes,
      gdouble       inten,
      guchar        color[MAX_CHANNELS],
      guchar       *dest_buf)
{
  gint     x, y, b;
  gdouble  dx, dy, rs, val;
  guchar  *pixel;
  guchar   pixel_buf[4];
  gdouble  new;

  x = (int) (xr);    /* integer coord. to upper left of real point */
  y = (int) (yr);

  if (x >= x1 && y >= y1 && x < x2 && y < y2)
    {
      if (dest_buf)
        {
          pixel = dest_buf + ((y - y1) * (x2 - x1) + (x - x1)) * bytes;
        }
      else
        {
          gegl_buffer_sample (dest_buffer, x, y, NULL,
                              pixel_buf, format,
                              GEGL_SAMPLER_NEAREST, GEGL_ABYSS_NONE);

          pixel = pixel_buf;
        }

      dx = xr - x; dy = yr - y;
      rs = dx * dx + dy * dy;
      val = inten * exp (-rs / PSV);

      for (b = 0; b < bytes; b++)
        {
          if (svals.inverse)
            new = 255 - pixel[b];
          else
            new = pixel[b];

          if (svals.preserve_luminosity)
            {
              if (new < color[b])
                {
                  new *= (1.0 - val * (1.0 - svals.transparency));
                }
              else
                {
                  new -= val * color[b] * (1.0 - svals.transparency);
                  if (new < 0.0)
                    new = 0.0;
                }
            }

          new *= 1.0 - val * svals.transparency;
          new += val * color[b];

          if (new > 255)
            new = 255;

          if (svals.inverse)
            pixel[b] = 255 - new;
          else
            pixel[b] = new;
        }

      if (! dest_buf)
        gegl_buffer_set (dest_buffer, GEGL_RECTANGLE (x, y, 1, 1), 0,
                         format, pixel_buf,
                         GEGL_AUTO_ROWSTRIDE);
    }
}

static void
fspike (GeglBuffer   *src_buffer,
        GeglBuffer   *dest_buffer,
        const Babl   *format,
        gint          bytes,
        gint          x1,
        gint          y1,
        gint          x2,
        gint          y2,
        gint          xr,
        gint          yr,
        gdouble       inten,
        gdouble       length,
        gdouble       angle,
        GRand        *gr,
        guchar       *dest_buf)
{
  const gdouble  efac = 2.0;
  gdouble        xrt, yrt, dx, dy;
  gdouble        rpos;
  gdouble        in;
  gdouble        theta;
  gdouble        sfac;
  gint           i;
  gboolean       ok;
  GimpRGB        gimp_color;
  guchar         pixel[MAX_CHANNELS];
  guchar         chosen_color[MAX_CHANNELS];
  guchar         color[MAX_CHANNELS];

  theta = angle;

  switch (svals.colortype)
    {
    case NATURAL:
      break;

    case FOREGROUND:
      gimp_context_get_foreground (&gimp_color);
      gimp_rgb_get_uchar (&gimp_color, &chosen_color[0], &chosen_color[1],
                          &chosen_color[2]);
      break;

    case BACKGROUND:
      gimp_context_get_background (&gimp_color);
      gimp_rgb_get_uchar (&gimp_color, &chosen_color[0], &chosen_color[1],
                          &chosen_color[2]);
      break;
    }

  /* draw the major spikes */
  for (i = 0; i < svals.spike_pts; i++)
    {
      gegl_buffer_sample (dest_buffer, xr, yr, NULL, pixel, format,
                          GEGL_SAMPLER_NEAREST, GEGL_ABYSS_NONE);

      if (svals.colortype == NATURAL)
        {
          color[0] = pixel[0];
          color[1] = pixel[1];
          color[2] = pixel[2];
        }
      else
        {
          color[0] = chosen_color[0];
          color[1] = chosen_color[1];
          color[2] = chosen_color[2];
        }

      color[3] = pixel[3];

      if (svals.inverse)
        {
          color[0] = 255 - color[0];
          color[1] = 255 - color[1];
          color[2] = 255 - color[2];
        }

      if (svals.random_hue > 0.0 || svals.random_saturation > 0.0)
        {
          GimpRGB rgb;
          GimpHSV hsv;

          rgb.r = (gdouble) (255 - color[0]) / 255.0;
          rgb.g = (gdouble) (255 - color[1]) / 255.0;
          rgb.b = (gdouble) (255 - color[2]) / 255.0;

          gimp_rgb_to_hsv (&rgb, &hsv);

          hsv.h += svals.random_hue * g_rand_double_range (gr, -0.5, 0.5);

          if (hsv.h >= 1.0)
            hsv.h -= 1.0;
          else if (hsv.h < 0.0)
            hsv.h += 1.0;

          hsv.v += (svals.random_saturation *
                    g_rand_double_range (gr, -1.0, 1.0));

          hsv.v = CLAMP (hsv.v, 0.0, 1.0);

          gimp_hsv_to_rgb (&hsv, &rgb);

          color[0] = 255 - ROUND (rgb.r * 255.0);
          color[1] = 255 - ROUND (rgb.g * 255.0);
          color[2] = 255 - ROUND (rgb.b * 255.0);
        }

      dx = 0.2 * cos (theta * G_PI / 180.0);
      dy = 0.2 * sin (theta * G_PI / 180.0);
      xrt = (gdouble) xr; /* (gdouble) is needed because some      */
      yrt = (gdouble) yr; /* compilers optimize too much otherwise */
      rpos = 0.2;

      do
        {
          sfac = inten * exp (-pow (rpos / length, efac));
          ok = FALSE;

          in = 0.2 * sfac;
          if (in > 0.01)
            ok = TRUE;

          rpnt (dest_buffer, format, x1, y1, x2, y2,
                xrt, yrt,
                bytes, in, color, dest_buf);

          rpnt (dest_buffer, format, x1, y1, x2, y2,
                xrt + 1.0, yrt,
                bytes, in, color, dest_buf);

          rpnt (dest_buffer, format, x1, y1, x2, y2,
                xrt + 1.0, yrt + 1.0,
                bytes, in, color, dest_buf);

          rpnt (dest_buffer, format, x1, y1, x2, y2,
                xrt, yrt + 1.0,
                bytes, in, color, dest_buf);

          xrt += dx;
          yrt += dy;
          rpos += 0.2;

        } while (ok);

      theta += 360.0 / svals.spike_pts;
    }
}
