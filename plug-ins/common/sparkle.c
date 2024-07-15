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

#define MAX_CHANNELS     4
#define PSV              2  /* point spread value */

#define NATURAL          0
#define FOREGROUND       1
#define BACKGROUND       2


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
#define SPARKLE(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), SPARKLE_TYPE, Sparkle))

GType                   sparkle_get_type         (void) G_GNUC_CONST;

static GList          * sparkle_query_procedures (GimpPlugIn           *plug_in);
static GimpProcedure  * sparkle_create_procedure (GimpPlugIn           *plug_in,
                                                  const gchar          *name);

static GimpValueArray * sparkle_run              (GimpProcedure        *procedure,
                                                  GimpRunMode           run_mode,
                                                  GimpImage            *image,
                                                  gint                  n_drawables,
                                                  GimpDrawable        **drawables,
                                                  GimpProcedureConfig  *config,
                                                  gpointer              run_data);

static gboolean         sparkle_dialog           (GimpProcedure        *procedure,
                                                  GObject              *config,
                                                  GimpDrawable         *drawable);

static gint             compute_luminosity       (const guchar         *pixel,
                                                  gboolean              gray,
                                                  gboolean              has_alpha,
                                                  gint                  inverse);
static gint             compute_lum_threshold    (GimpDrawable         *drawable,
                                                  gdouble               percentile,
                                                  gint                  inverse);
static void             sparkle                  (GObject              *config,
                                                  GimpDrawable         *drawable,
                                                  GimpPreview          *preview);
static void             sparkle_preview          (GtkWidget            *widget,
                                                  GObject              *config);
static void             fspike                   (GObject              *config,
                                                  GeglBuffer           *src_buffer,
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
static void             rpnt                     (GObject              *config,
                                                  GeglBuffer           *dest_buffer,
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
DEFINE_STD_SET_I18N


static gint num_sparkles;


static void
sparkle_class_init (SparkleClass *klass)
{
  GimpPlugInClass *plug_in_class = GIMP_PLUG_IN_CLASS (klass);

  plug_in_class->query_procedures = sparkle_query_procedures;
  plug_in_class->create_procedure = sparkle_create_procedure;
  plug_in_class->set_i18n         = STD_SET_I18N;
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
      gimp_procedure_set_sensitivity_mask (procedure,
                                           GIMP_PROCEDURE_SENSITIVE_DRAWABLE);

      gimp_procedure_set_menu_label (procedure, _("_Sparkle..."));
      gimp_procedure_add_menu_path (procedure,
                                    "<Image>/Filters/Light and Shadow/[Light]");

      gimp_procedure_set_documentation (procedure,
                                        _("Turn bright spots into "
                                          "starry sparkles"),
                                        _("Uses a percentage based luminosity "
                                          "threshold to find candidate pixels "
                                          "for adding some sparkles (spikes)."),
                                        name);
      gimp_procedure_set_attribution (procedure,
                                      "John Beale, & (ported to GIMP v0.54) "
                                      "Michael J. Hammel & ted to GIMP v1.0) "
                                      "& Seth Burgess & Spencer Kimball",
                                      "John Beale",
                                      "Version 1.27, September 2003");

      gimp_procedure_add_double_argument (procedure, "lum-threshold",
                                          _("Lu_minosity threshold"),
                                          _("Adjust the luminosity threshold"),
                                          0.0, 0.1, 0.01,
                                          G_PARAM_READWRITE);

      gimp_procedure_add_double_argument (procedure, "flare-inten",
                                          _("_Flare intensity"),
                                          _("Adjust the flare intensity"),
                                          0.0, 1.0, 0.5,
                                          G_PARAM_READWRITE);

      gimp_procedure_add_int_argument (procedure, "spike-len",
                                       _("Spi_ke length"),
                                       _("Adjust the spike length (in pixels)"),
                                       1, 100, 20,
                                       G_PARAM_READWRITE);

      gimp_procedure_add_int_argument (procedure, "spike-points",
                                       _("Spike _points"),
                                       _("Adjust the number of spikes"),
                                       1, 16, 4,
                                       G_PARAM_READWRITE);

      gimp_procedure_add_int_argument (procedure, "spike-angle",
                                       _("Spike angle (-_1: random)"),
                                       _("Adjust the spike angle "
                                         "(-1 causes a random angle to be chosen)"),
                                       -1, 360, 15,
                                       G_PARAM_READWRITE);

      gimp_procedure_add_double_argument (procedure, "density",
                                          _("Spike _density"),
                                          _("Adjust the spike density"),
                                          0.0, 1.0, 1.0,
                                          G_PARAM_READWRITE);

      gimp_procedure_add_double_argument (procedure, "transparency",
                                          _("_Transparency"),
                                          _("Adjust the opacity of the spikes"),
                                          0.0, 1.0, 0.0,
                                          G_PARAM_READWRITE);

      gimp_procedure_add_double_argument (procedure, "random-hue",
                                          _("Random _hue"),
                                          _("Adjust how much the hue should be "
                                            "changed randomly"),
                                          0.0, 1.0, 0.0,
                                          G_PARAM_READWRITE);

      gimp_procedure_add_double_argument (procedure, "random-saturation",
                                          _("R_andom saturation"),
                                          _("Adjust how much the saturation should be "
                                            "changed randomly"),
                                          0.0, 1.0, 0.0,
                                          G_PARAM_READWRITE);

      gimp_procedure_add_boolean_argument (procedure, "preserve-luminosity",
                                           _("Preserve l_uminosity"),
                                           _("Should the luminosity be preserved?"),
                                           FALSE,
                                           G_PARAM_READWRITE);

      gimp_procedure_add_boolean_argument (procedure, "inverse",
                                           _("In_verse"),
                                           _("Should the effect be inversed?"),
                                           FALSE,
                                           G_PARAM_READWRITE);

      gimp_procedure_add_boolean_argument (procedure, "border",
                                           _("Add _border"),
                                           _("Draw a border of spikes around the image"),
                                           FALSE,
                                           G_PARAM_READWRITE);

      gimp_procedure_add_choice_argument (procedure, "color-type",
                                          _("_Color type"),
                                          _("Color of sparkles"),
                                          gimp_choice_new_with_values ("natural-color",    NATURAL,    _("Natural color"),    NULL,
                                                                       "foreground-color", FOREGROUND, _("Foreground color"), NULL,
                                                                       "background-color", BACKGROUND, _("Background color"), NULL,
                                                                       NULL),
                                          "natural-color",
                                          G_PARAM_READWRITE);
    }

  return procedure;
}

static GimpValueArray *
sparkle_run (GimpProcedure        *procedure,
             GimpRunMode           run_mode,
             GimpImage            *image,
             gint                  n_drawables,
             GimpDrawable        **drawables,
             GimpProcedureConfig  *config,
             gpointer              run_data)
{
  GimpDrawable *drawable;
  gint          x, y, w, h;

  gegl_init (NULL, NULL);

  if (n_drawables != 1)
    {
      GError *error = NULL;

      g_set_error (&error, GIMP_PLUG_IN_ERROR, 0,
                   _("Procedure '%s' only works with one drawable."),
                   PLUG_IN_PROC);

      return gimp_procedure_new_return_values (procedure,
                                               GIMP_PDB_CALLING_ERROR,
                                               error);
    }
  else
    {
      drawable = drawables[0];
    }

  if (! gimp_drawable_mask_intersect (drawable, &x, &y, &w, &h))
    {
      g_message (_("Region selected for filter is empty"));

      return gimp_procedure_new_return_values (procedure,
                                               GIMP_PDB_SUCCESS,
                                               NULL);
    }

  if (run_mode == GIMP_RUN_INTERACTIVE && ! sparkle_dialog (procedure, G_OBJECT (config), drawable))
    return gimp_procedure_new_return_values (procedure,
                                             GIMP_PDB_CANCEL,
                                             NULL);

  if (gimp_drawable_is_rgb (drawable) ||
      gimp_drawable_is_gray (drawable))
    {
      gimp_progress_init (_("Sparkling"));

      sparkle (G_OBJECT (config), drawable, NULL);

      if (run_mode != GIMP_RUN_NONINTERACTIVE)
        gimp_displays_flush ();
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
sparkle_dialog (GimpProcedure *procedure,
                GObject       *config,
                GimpDrawable  *drawable)
{
  GtkWidget *dialog;
  GtkWidget *preview;
  GtkWidget *hbox;
  GtkWidget *scale;
  gboolean   run;

  gimp_ui_init (PLUG_IN_BINARY);

  dialog = gimp_procedure_dialog_new (procedure,
                                      GIMP_PROCEDURE_CONFIG (config),
                                      _("Sparkle"));

  gtk_widget_set_size_request (dialog, 430, -1);
  gtk_container_set_border_width (
    GTK_CONTAINER (gtk_dialog_get_content_area (GTK_DIALOG (dialog))), 12);

  gimp_procedure_dialog_get_scale_entry (GIMP_PROCEDURE_DIALOG (dialog),
                                         "lum-threshold", 1.0);

  gimp_procedure_dialog_get_scale_entry (GIMP_PROCEDURE_DIALOG (dialog),
                                         "flare-inten", 1.0);

  gimp_procedure_dialog_get_scale_entry (GIMP_PROCEDURE_DIALOG (dialog),
                                         "spike-len", 1.0);

  gimp_procedure_dialog_get_scale_entry (GIMP_PROCEDURE_DIALOG (dialog),
                                         "spike-points", 1.0);

  gimp_procedure_dialog_get_scale_entry (GIMP_PROCEDURE_DIALOG (dialog),
                                         "spike-angle", 1.0);

  gimp_procedure_dialog_get_scale_entry (GIMP_PROCEDURE_DIALOG (dialog),
                                         "density", 1.0);

  gimp_procedure_dialog_get_scale_entry (GIMP_PROCEDURE_DIALOG (dialog),
                                         "density", 1.0);;

  gimp_procedure_dialog_get_scale_entry (GIMP_PROCEDURE_DIALOG (dialog),
                                         "transparency", 1.0);

  gimp_procedure_dialog_get_scale_entry (GIMP_PROCEDURE_DIALOG (dialog),
                                         "random-hue", 1.0);

  scale = gimp_procedure_dialog_get_scale_entry (GIMP_PROCEDURE_DIALOG (dialog),
                                                 "random-saturation", 1.0);
  gtk_widget_set_margin_bottom (scale, 12);

  gimp_procedure_dialog_fill_box (GIMP_PROCEDURE_DIALOG (dialog),
                                  "sparkle-bool-vbox",
                                  "preserve-luminosity", "inverse",
                                  "border", NULL);

  gimp_procedure_dialog_get_label (GIMP_PROCEDURE_DIALOG (dialog),
                                   "sparkle-bool-label",
                                   _("Additional Options"),
                                   FALSE, FALSE);
  gimp_procedure_dialog_fill_frame (GIMP_PROCEDURE_DIALOG (dialog),
                                    "sparkle-bool-frame", "sparkle-bool-label",
                                    FALSE, "sparkle-bool-vbox");

  /*  colortype  */
  gimp_procedure_dialog_get_widget (GIMP_PROCEDURE_DIALOG (dialog),
                                    "color-type", GIMP_TYPE_INT_RADIO_FRAME);

  hbox = gimp_procedure_dialog_fill_box (GIMP_PROCEDURE_DIALOG (dialog),
                                        "sparkle-row", "sparkle-bool-frame",
                                        "color-type", NULL);
  gtk_orientable_set_orientation (GTK_ORIENTABLE (hbox),
                                  GTK_ORIENTATION_HORIZONTAL);
  gtk_box_set_spacing (GTK_BOX (hbox), 12);
  gtk_box_set_homogeneous (GTK_BOX (hbox), TRUE);
  gtk_widget_set_margin_bottom (hbox, 12);

  preview = gimp_procedure_dialog_get_drawable_preview (GIMP_PROCEDURE_DIALOG (dialog),
                                                        "preview", drawable);
  g_object_set_data (config, "drawable", drawable);

  g_signal_connect (preview, "invalidated",
                    G_CALLBACK (sparkle_preview),
                    config);

  g_signal_connect_swapped (config, "notify",
                            G_CALLBACK (gimp_preview_invalidate),
                            preview);

  gimp_procedure_dialog_fill (GIMP_PROCEDURE_DIALOG (dialog),
                              "preview",
                              "lum-threshold", "flare-inten", "spike-len",
                              "spike-points", "spike-angle", "density",
                              "transparency", "random-hue",
                              "random-saturation", "sparkle-row",
                              NULL);

  gtk_widget_show (dialog);

  run = gimp_procedure_dialog_run (GIMP_PROCEDURE_DIALOG (dialog));

  gtk_widget_destroy (dialog);

  return run;
}

static gint
compute_luminosity (const guchar *pixel,
                    gboolean      gray,
                    gboolean      has_alpha,
                    gint          inverse)
{
  gint pixel0, pixel1, pixel2;

  if (inverse)
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
                       gdouble       percentile,
                       gint          inverse)
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
          values [compute_luminosity (src, gray, has_alpha, inverse)]++;
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
sparkle (GObject      *config,
         GimpDrawable *drawable,
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
  gdouble             lum_threshold;
  gdouble             flare_inten;
  gint                spike_len;
  gint                spike_pts;
  gint                spike_angle_config;
  gdouble             density;
  gint                inverse;
  gint                border;

  g_object_get (config,
                "lum-threshold", &lum_threshold,
                "flare-inten",   &flare_inten,
                "spike-len",     &spike_len,
                "spike-points",  &spike_pts,
                "spike-angle",   &spike_angle_config,
                "density",       &density,
                "inverse",       &inverse,
                "border",        &border,
                NULL);

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

  d_width  = gimp_drawable_get_width  (drawable);
  d_height = gimp_drawable_get_height (drawable);

  gr = g_rand_new ();

  if (border)
    {
      num_sparkles = 2 * (width + height);
      threshold = 255;
    }
  else
    {
      /*  compute the luminosity which exceeds the luminosity threshold  */
      threshold = compute_lum_threshold (drawable, lum_threshold, inverse);
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
              if (border)
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
                  lum = compute_luminosity (s, gray, has_alpha, inverse);
                }

              if (lum >= threshold)
                {
                  nfrac = fabs ((gdouble) (lum + 1 - threshold) /
                                (gdouble) (256 - threshold));
                  length = ((gdouble) spike_len *
                            (gdouble) pow (nfrac, 0.8));
                  inten = flare_inten * nfrac;

                  /* fspike im x,y intens rlength angle */
                  if (spike_pts > 0)
                    {
                      /* major spikes */
                      if (spike_angle_config == -1)
                        spike_angle = g_rand_double_range (gr, 0, 360.0);
                      else
                        spike_angle = spike_angle_config;

                      if (g_rand_double (gr) <= density)
                        {
                          fspike (config, src_buffer, dest_buffer, format,
                                  bytes, x1, y1, x2, y2,
                                  x + roi.x, y + roi.y,
                                  inten, length, spike_angle, gr, dest_buf);

                          /* minor spikes */
                          fspike (config, src_buffer, dest_buffer, format,
                                  bytes, x1, y1, x2, y2,
                                  x + roi.x, y + roi.y,
                                  inten * 0.7, length * 0.7,
                                  ((gdouble) spike_angle + 180.0 / spike_pts),
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
sparkle_preview (GtkWidget *widget,
                 GObject   *config)
{
  GimpPreview  *preview  = GIMP_PREVIEW (widget);
  GimpDrawable *drawable = g_object_get_data (config, "drawable");

  sparkle (config, drawable, preview);
}

static inline void
rpnt (GObject      *config,
      GeglBuffer   *dest_buffer,
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
  gdouble  transparency;
  gint     preserve_luminosity;
  gint     inverse;
  gint     border;

  g_object_get (config,
                "transparency",        &transparency,
                "preserve-luminosity", &preserve_luminosity,
                "inverse",             &inverse,
                "border",              &border,
                NULL);

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
          if (inverse)
            new = 255 - pixel[b];
          else
            new = pixel[b];

          if (preserve_luminosity)
            {
              if (new < color[b])
                {
                  new *= (1.0 - val * (1.0 - transparency));
                }
              else
                {
                  new -= val * color[b] * (1.0 - transparency);
                  if (new < 0.0)
                    new = 0.0;
                }
            }

          new *= 1.0 - val * transparency;
          new += val * color[b];

          if (new > 255)
            new = 255;

          if (inverse)
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
fspike (GObject      *config,
        GeglBuffer   *src_buffer,
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
  GeglColor     *gegl_color;
  guchar         pixel[MAX_CHANNELS];
  guchar         chosen_color[MAX_CHANNELS];
  guchar         color[MAX_CHANNELS];
  gint           spike_pts;
  gdouble        random_hue;
  gdouble        random_saturation;
  gint           inverse;
  gint           colortype;

  g_object_get (config,
                "spike-points",      &spike_pts,
                "random-hue",        &random_hue,
                "random-saturation", &random_saturation,
                "inverse",           &inverse,
                NULL);
  colortype = gimp_procedure_config_get_choice_id (GIMP_PROCEDURE_CONFIG (config),
                                                   "color-type");

  theta = angle;

  switch (colortype)
    {
    case NATURAL:
      break;

    case FOREGROUND:
      gegl_color = gimp_context_get_foreground ();
      gegl_color_get_pixel (gegl_color, babl_format_with_space ("R'G'B' u8", NULL), chosen_color);
      g_clear_object (&gegl_color);
      break;

    case BACKGROUND:
      gegl_color = gimp_context_get_background ();
      gegl_color_get_pixel (gegl_color, babl_format_with_space ("R'G'B' u8", NULL), chosen_color);
      g_clear_object (&gegl_color);
      break;
    }

  gegl_color = gegl_color_new ("black");
  /* draw the major spikes */
  for (i = 0; i < spike_pts; i++)
    {
      gegl_buffer_sample (dest_buffer, xr, yr, NULL, pixel, format,
                          GEGL_SAMPLER_NEAREST, GEGL_ABYSS_NONE);

      if (colortype == NATURAL)
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

      if (inverse)
        {
          color[0] = 255 - color[0];
          color[1] = 255 - color[1];
          color[2] = 255 - color[2];
        }

      if (random_hue > 0.0 || random_saturation > 0.0)
        {
          gfloat rgb[3];
          gfloat hsv[3];

          for (gint j = 0; j < 3; j++)
            rgb[j] = (gdouble) (255 - color[j]) / 255.0;

          gegl_color_set_pixel (gegl_color, babl_format ("R'G'B' float"), rgb);
          gegl_color_set_pixel (gegl_color, babl_format ("HSV float"), hsv);

          hsv[0] += random_hue * g_rand_double_range (gr, -0.5, 0.5);

          if (hsv[0] >= 1.0)
            hsv[0] -= 1.0;
          else if (hsv[0] < 0.0)
            hsv[0] += 1.0;

          hsv[2] += (random_saturation *
                    g_rand_double_range (gr, -1.0, 1.0));

          hsv[2] = CLAMP (hsv[2], 0.0, 1.0);

          gegl_color_set_pixel (gegl_color, babl_format ("HSV float"), hsv);
          gegl_color_get_pixel (gegl_color, babl_format ("R'G'B' float"), rgb);

          for (gint j = 0; j < 3; j++)
            color[j] = 255 - ROUND (rgb[j] * 255.0);
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

          rpnt (config, dest_buffer, format, x1, y1, x2, y2,
                xrt, yrt,
                bytes, in, color, dest_buf);

          rpnt (config, dest_buffer, format, x1, y1, x2, y2,
                xrt + 1.0, yrt,
                bytes, in, color, dest_buf);

          rpnt (config, dest_buffer, format, x1, y1, x2, y2,
                xrt + 1.0, yrt + 1.0,
                bytes, in, color, dest_buf);

          rpnt (config, dest_buffer, format, x1, y1, x2, y2,
                xrt, yrt + 1.0,
                bytes, in, color, dest_buf);

          xrt += dx;
          yrt += dy;
          rpos += 0.2;

        } while (ok);

      theta += 360.0 / spike_pts;
    }
  g_object_unref (gegl_color);
}
