/*
 * GIMP - The GNU Image Manipulation Program
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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

/*
 * hot.c - Scan an image for pixels with RGB values that will give
 *      "unsafe" values of chrominance signal or composite signal
 *      amplitude when encoded into an NTSC or PAL color signal.
 *      (This happens for certain high-intensity high-saturation colors
 *      that are rare in real scenes, but can easily be present
 *      in synthetic images.)
 *
 *      Such pixels can be flagged so the user may then choose other
 *      colors.  Or, the offending pixels can be made "safe"
 *      in a manner that preserves hue.
 *
 *      There are two reasonable ways to make a pixel "safe":
 *      We can reduce its intensity (luminance) while leaving
 *      hue and saturation the same.  Or, we can reduce saturation
 *      while leaving hue and luminance the same.  A #define selects
 *      which strategy to use.
 *
 * Note to the user: You must add your own read_pixel() and write_pixel()
 *      routines.  You may have to modify pix_decode() and pix_encode().
 *      MAXPIX, WID, and HGT are likely to need modification.
 */

/*
 * Originally written as "ikNTSC.c" by Alan Wm Paeth,
 *      University of Waterloo, August, 1985
 * Updated by Dave Martindale, Imax Systems Corp., December 1990
 */

/*
 * Compile time options:
 *
 *
 * CHROMA_LIM is the limit (in IRE units) of the overall
 *      chrominance amplitude; it should be 50 or perhaps
 *      very slightly higher.
 *
 * COMPOS_LIM is the maximum amplitude (in IRE units) allowed for
 *      the composite signal.  A value of 100 is the maximum
 *      monochrome white, and is always safe.  120 is the absolute
 *      limit for NTSC broadcasting, since the transmitter's carrier
 *      goes to zero with 120 IRE input signal.  Generally, 110
 *      is a good compromise - it allows somewhat brighter colors
 *      than 100, while staying safely away from the hard limit.
 */

#include "config.h"

#include <string.h>

#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

#include "libgimp/stdplugins-intl.h"


#define PLUG_IN_PROC   "plug-in-hot"
#define PLUG_IN_BINARY "hot"
#define PLUG_IN_ROLE   "gimp-hot"


typedef enum
{
  ACT_LREDUX,
  ACT_SREDUX,
  ACT_FLAG
} hotAction;

typedef enum
{
  MODE_NTSC,
  MODE_PAL
} hotModes;

#define CHROMA_LIM      50.0            /* chroma amplitude limit */
#define COMPOS_LIM      110.0           /* max IRE amplitude */

/*
 * RGB to YIQ encoding matrix.
 */

struct
{
  gdouble pedestal;
  gdouble gamma;
  gdouble code[3][3];
}
static mode_vals[2] =
{
  {
    7.5,
    2.2,
    {
      { 0.2989,  0.5866,  0.1144 },
      { 0.5959, -0.2741, -0.3218 },
      { 0.2113, -0.5227,  0.3113 }
    }
  },
  {
    0.0,
    2.8,
    {
      { 0.2989,  0.5866,  0.1144 },
      { -0.1473, -0.2891,  0.4364 },
      { 0.6149, -0.5145, -0.1004 }
    }
  }
};


#define SCALE   8192            /* scale factor: do floats with int math */
#define MAXPIX   255            /* white value */


typedef struct _Hot      Hot;
typedef struct _HotClass HotClass;

struct _Hot
{
  GimpPlugIn parent_instance;
};

struct _HotClass
{
  GimpPlugInClass parent_class;
};


#define HOT_TYPE  (hot_get_type ())
#define HOT(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), HOT_TYPE, Hot))

GType                   hot_get_type         (void) G_GNUC_CONST;

static GList          * hot_query_procedures (GimpPlugIn           *plug_in);
static GimpProcedure  * hot_create_procedure (GimpPlugIn           *plug_in,
                                              const gchar          *name);

static GimpValueArray * hot_run              (GimpProcedure        *procedure,
                                              GimpRunMode           run_mode,
                                              GimpImage            *image,
                                              gint                  n_drawables,
                                              GimpDrawable        **drawables,
                                              GimpProcedureConfig  *config,
                                              gpointer              run_data);

static gboolean         pluginCore           (GimpImage            *image,
                                              GimpDrawable         *drawable,
                                              GObject              *config);
static gboolean         plugin_dialog        (GimpProcedure        *procedure,
                                              GObject              *config);
static gboolean         hotp                 (guint8                r,
                                              guint8                g,
                                              guint8                b);
static void             build_tab            (gint                  m);

/*
 * gc: apply the gamma correction specified for this video standard.
 * inv_gc: inverse function of gc.
 *
 * These are generally just a call to pow(), but be careful!
 * Future standards may use more complex functions.
 * (e.g. SMPTE 240M's "electro-optic transfer characteristic").
 */
#define gc(x,m)     pow(x, 1.0 / mode_vals[m].gamma)
#define inv_gc(x,m) pow(x,       mode_vals[m].gamma)

/*
 * pix_decode: decode an integer pixel value into a floating-point
 *      intensity in the range [0, 1].
 *
 * pix_encode: encode a floating-point intensity into an integer
 *      pixel value.
 *
 * The code given here assumes simple linear encoding; you must change
 * these routines if you use a different pixel encoding technique.
 */
#define pix_decode(v)  ((double)v / (double)MAXPIX)
#define pix_encode(v)  ((int)(v * (double)MAXPIX + 0.5))


G_DEFINE_TYPE (Hot, hot, GIMP_TYPE_PLUG_IN)

GIMP_MAIN (HOT_TYPE)
DEFINE_STD_SET_I18N


static gint     tab[3][3][MAXPIX+1]; /* multiply lookup table */
static gdouble  chroma_lim;          /* chroma limit */
static gdouble  compos_lim;          /* composite amplitude limit */
static glong    ichroma_lim2;        /* chroma limit squared (scaled integer) */
static gint     icompos_lim;         /* composite amplitude limit (scaled integer) */


static void
hot_class_init (HotClass *klass)
{
  GimpPlugInClass *plug_in_class = GIMP_PLUG_IN_CLASS (klass);

  plug_in_class->query_procedures = hot_query_procedures;
  plug_in_class->create_procedure = hot_create_procedure;
  plug_in_class->set_i18n         = STD_SET_I18N;
}

static void
hot_init (Hot *hot)
{
}

static GList *
hot_query_procedures (GimpPlugIn *plug_in)
{
  return g_list_append (NULL, g_strdup (PLUG_IN_PROC));
}

static GimpProcedure *
hot_create_procedure (GimpPlugIn  *plug_in,
                               const gchar *name)
{
  GimpProcedure *procedure = NULL;

  if (! strcmp (name, PLUG_IN_PROC))
    {
      procedure = gimp_image_procedure_new (plug_in, name,
                                            GIMP_PDB_PROC_TYPE_PLUGIN,
                                            hot_run, NULL, NULL);

      gimp_procedure_set_image_types (procedure, "RGB");
      gimp_procedure_set_sensitivity_mask (procedure,
                                           GIMP_PROCEDURE_SENSITIVE_DRAWABLE);

      gimp_procedure_set_menu_label (procedure, _("_Hot..."));
      gimp_procedure_add_menu_path (procedure, "<Image>/Colors/[Modify]");

      gimp_procedure_set_documentation (procedure,
                                        _("Find and fix pixels that may "
                                          "be unsafely bright"),
                                        "hot scans an image for pixels that "
                                        "will give unsave values of "
                                        "chrominance or composite signal "
                                        "amplitude when encoded into an NTSC "
                                        "or PAL signal. Three actions can be "
                                        "performed on these 'hot' pixels. "
                                        "(0) reduce luminance, "
                                        "(1) reduce saturation, or (2) Blacken.",
                                        name);
      gimp_procedure_set_attribution (procedure,
                                      "Eric L. Hernes, Alan Wm Paeth",
                                      "Eric L. Hernes",
                                      "1997");

      gimp_procedure_add_choice_argument (procedure, "mode",
                                          _("_Mode"),
                                          _("Signal mode"),
                                          gimp_choice_new_with_values ("ntsc", MODE_NTSC, _("NTSC"), NULL,
                                                                       "pal",  MODE_PAL,  _("PAL"),   NULL,
                                                                       NULL),
                                          "ntsc",
                                          G_PARAM_READWRITE);

      gimp_procedure_add_choice_argument (procedure, "action",
                                          _("_Action"),
                                          _("Action"),
                                          gimp_choice_new_with_values ("reduce-luminance",   ACT_LREDUX, _("Reduce Luminance"),  NULL,
                                                                       "reduce-saturation",  ACT_SREDUX, _("Reduce Saturation"), NULL,
                                                                       "blacken",            ACT_FLAG,   _("Blacken"),           NULL,
                                                                       NULL),
                                          "reduce-luminance",
                                          G_PARAM_READWRITE);

      gimp_procedure_add_boolean_argument (procedure, "new-layer",
                                           _("Create _new layer"),
                                           _("Create a new layer"),
                                           TRUE,
                                           G_PARAM_READWRITE);
    }

  return procedure;
}

static GimpValueArray *
hot_run (GimpProcedure        *procedure,
         GimpRunMode           run_mode,
         GimpImage            *image,
         gint                  n_drawables,
         GimpDrawable        **drawables,
         GimpProcedureConfig  *config,
         gpointer              run_data)
{
  GimpDrawable *drawable;

  gegl_init (NULL, NULL);

  if (n_drawables != 1)
    {
      GError *error = NULL;

      g_set_error (&error, GIMP_PLUG_IN_ERROR, 0,
                   _("Procedure '%s' only works with one drawable."),
                   gimp_procedure_get_name (procedure));

      return gimp_procedure_new_return_values (procedure,
                                               GIMP_PDB_CALLING_ERROR,
                                               error);
    }
  else
    {
      drawable = drawables[0];
    }

  if (run_mode == GIMP_RUN_INTERACTIVE && ! plugin_dialog (procedure, G_OBJECT (config)))
    return gimp_procedure_new_return_values (procedure,
                                             GIMP_PDB_CANCEL,
                                             NULL);

  if (! pluginCore (image, drawable, G_OBJECT (config)))
    return gimp_procedure_new_return_values (procedure,
                                             GIMP_PDB_EXECUTION_ERROR,
                                             NULL);

  if (run_mode != GIMP_RUN_NONINTERACTIVE)
    gimp_displays_flush ();

  return gimp_procedure_new_return_values (procedure, GIMP_PDB_SUCCESS, NULL);
}

static gboolean
pluginCore (GimpImage    *image,
            GimpDrawable *drawable,
            GObject      *config)
{
  gint        mode;
  gint        action;
  gboolean    new_layer;
  GeglBuffer *src_buffer;
  GeglBuffer *dest_buffer;
  const Babl *src_format;
  const Babl *dest_format;
  gint        src_bpp;
  gint        dest_bpp;
  gboolean    success = TRUE;
  GimpLayer  *nl      = NULL;
  gint        y, i;
  gint        Y, I, Q;
  gint        width, height;
  gint        sel_x1, sel_x2, sel_y1, sel_y2;
  gint        prog_interval;
  guchar     *src, *s, *dst, *d;
  guchar      r, prev_r=0, new_r=0;
  guchar      g, prev_g=0, new_g=0;
  guchar      b, prev_b=0, new_b=0;
  gdouble     fy, fc, t, scale;
  gdouble     pr, pg, pb;
  gdouble     py;

  g_object_get (config,
                "new-layer", &new_layer,
                NULL);
  mode = gimp_procedure_config_get_choice_id (GIMP_PROCEDURE_CONFIG (config),
                                              "mode");
  action = gimp_procedure_config_get_choice_id (GIMP_PROCEDURE_CONFIG (config),
                                                "action");

  width  = gimp_drawable_get_width  (drawable);
  height = gimp_drawable_get_height (drawable);

  if (gimp_drawable_has_alpha (drawable))
    src_format = babl_format ("R'G'B'A u8");
  else
    src_format = babl_format ("R'G'B' u8");

  dest_format = src_format;

  if (new_layer)
    {
      gchar        name[40];
      const gchar *mode_names[] =
      {
        "ntsc",
        "pal",
      };
      const gchar *action_names[] =
      {
        "lum redux",
        "sat redux",
        "flag",
      };

      g_snprintf (name, sizeof (name), "hot mask (%s, %s)",
                  mode_names[mode],
                  action_names[action]);

      nl = gimp_layer_new (image, name, width, height,
                           GIMP_RGBA_IMAGE,
                           100,
                           gimp_image_get_default_new_layer_mode (image));

      gimp_drawable_fill (GIMP_DRAWABLE (nl), GIMP_FILL_TRANSPARENT);
      gimp_image_insert_layer (image, nl, NULL, 0);

      dest_format = babl_format ("R'G'B'A u8");
    }

  if (! gimp_drawable_mask_intersect (drawable,
                                      &sel_x1, &sel_y1, &width, &height))
    return success;

  src_bpp  = babl_format_get_bytes_per_pixel (src_format);
  dest_bpp = babl_format_get_bytes_per_pixel (dest_format);

  sel_x2 = sel_x1 + width;
  sel_y2 = sel_y1 + height;

  src = g_new (guchar, width * height * src_bpp);
  dst = g_new (guchar, width * height * dest_bpp);

  src_buffer = gimp_drawable_get_buffer (drawable);

  if (new_layer)
    {
      dest_buffer = gimp_drawable_get_buffer (GIMP_DRAWABLE (nl));
    }
  else
    {
      dest_buffer = gimp_drawable_get_shadow_buffer (drawable);
    }

  gegl_buffer_get (src_buffer,
                   GEGL_RECTANGLE (sel_x1, sel_y1, width, height), 1.0,
                   src_format, src,
                   GEGL_AUTO_ROWSTRIDE, GEGL_ABYSS_NONE);

  s = src;
  d = dst;

  build_tab (mode);

  gimp_progress_init (_("Hot"));
  prog_interval = height / 10;

  for (y = sel_y1; y < sel_y2; y++)
    {
      gint x;

      if (y % prog_interval == 0)
        gimp_progress_update ((double) y / (double) (sel_y2 - sel_y1));

      for (x = sel_x1; x < sel_x2; x++)
        {
          if (hotp (r = *(s + 0), g = *(s + 1), b = *(s + 2)))
            {
              if (action == ACT_FLAG)
                {
                  for (i = 0; i < 3; i++)
                    *d++ = 0;
                  s += 3;
                  if (src_bpp == 4)
                    *d++ = *s++;
                  else if (new_layer)
                    *d++ = 255;
                }
              else
                {
                  /*
                   * Optimization: cache the last-computed hot pixel.
                   */
                  if (r == prev_r && g == prev_g && b == prev_b)
                    {
                      *d++ = new_r;
                      *d++ = new_g;
                      *d++ = new_b;
                      s += 3;
                      if (src_bpp == 4)
                        *d++ = *s++;
                      else if (new_layer)
                        *d++ = 255;
                    }
                  else
                    {
                      Y = tab[0][0][r] + tab[0][1][g] + tab[0][2][b];
                      I = tab[1][0][r] + tab[1][1][g] + tab[1][2][b];
                      Q = tab[2][0][r] + tab[2][1][g] + tab[2][2][b];

                      prev_r = r;
                      prev_g = g;
                      prev_b = b;
                      /*
                       * Get Y and chroma amplitudes in floating point.
                       *
                       * If your C library doesn't have hypot(), just use
                       * hypot(a,b) = sqrt(a*a, b*b);
                       *
                       * Then extract linear (un-gamma-corrected)
                       * floating-point pixel RGB values.
                       */
                      fy = (double)Y / (double)SCALE;
                      fc = hypot ((double) I / (double) SCALE,
                                  (double) Q / (double) SCALE);

                      pr = (double) pix_decode (r);
                      pg = (double) pix_decode (g);
                      pb = (double) pix_decode (b);

                      /*
                       * Reducing overall pixel intensity by scaling R,
                       * G, and B reduces Y, I, and Q by the same factor.
                       * This changes luminance but not saturation, since
                       * saturation is determined by the chroma/luminance
                       * ratio.
                       *
                       * On the other hand, by linearly interpolating
                       * between the original pixel value and a grey
                       * pixel with the same luminance (R=G=B=Y), we
                       * change saturation without affecting luminance.
                       */
                      if (action == ACT_LREDUX)
                        {
                          /*
                           * Calculate a scale factor that will bring the pixel
                           * within both chroma and composite limits, if we scale
                           * luminance and chroma simultaneously.
                           *
                           * The calculated chrominance reduction applies
                           * to the gamma-corrected RGB values that are
                           * the input to the RGB-to-YIQ operation.
                           * Multiplying the original un-gamma-corrected
                           * pixel values by the scaling factor raised to
                           * the "gamma" power is equivalent, and avoids
                           * calling gc() and inv_gc() three times each.  */
                          scale = chroma_lim / fc;
                          t = compos_lim / (fy + fc);
                          if (t < scale)
                            scale = t;
                          scale = pow (scale, mode_vals[mode].gamma);

                          r = (guint8) pix_encode (scale * pr);
                          g = (guint8) pix_encode (scale * pg);
                          b = (guint8) pix_encode (scale * pb);
                        }
                      else
                        { /* ACT_SREDUX hopefully */
                          /*
                           * Calculate a scale factor that will bring the
                           * pixel within both chroma and composite
                           * limits, if we scale chroma while leaving
                           * luminance unchanged.
                           *
                           * We have to interpolate gamma-corrected RGB
                           * values, so we must convert from linear to
                           * gamma-corrected before interpolation and then
                           * back to linear afterwards.
                           */
                          scale = chroma_lim / fc;
                          t = (compos_lim - fy) / fc;
                          if (t < scale)
                            scale = t;

                          pr = gc (pr, mode);
                          pg = gc (pg, mode);
                          pb = gc (pb, mode);

                          py = pr * mode_vals[mode].code[0][0] +
                               pg * mode_vals[mode].code[0][1] +
                               pb * mode_vals[mode].code[0][2];

                          r = pix_encode (inv_gc (py + scale * (pr - py),
                                                  mode));
                          g = pix_encode (inv_gc (py + scale * (pg - py),
                                                  mode));
                          b = pix_encode (inv_gc (py + scale * (pb - py),
                                                  mode));
                        }

                      *d++ = new_r = r;
                      *d++ = new_g = g;
                      *d++ = new_b = b;

                      s += 3;

                      if (src_bpp == 4)
                        *d++ = *s++;
                      else if (new_layer)
                        *d++ = 255;
                    }
                }
            }
          else
            {
              if (! new_layer)
                {
                  for (i = 0; i < src_bpp; i++)
                    *d++ = *s++;
                }
              else
                {
                  s += src_bpp;
                  d += dest_bpp;
                }
            }
        }
    }

  gegl_buffer_set (dest_buffer,
                   GEGL_RECTANGLE (sel_x1, sel_y1, width, height), 0,
                   dest_format, dst,
                   GEGL_AUTO_ROWSTRIDE);

  gimp_progress_update (1.0);

  g_free (src);
  g_free (dst);

  g_object_unref (src_buffer);
  g_object_unref (dest_buffer);

  if (new_layer)
    {
      gimp_drawable_update (GIMP_DRAWABLE (nl), sel_x1, sel_y1, width, height);
    }
  else
    {
      gimp_drawable_merge_shadow (drawable, TRUE);
      gimp_drawable_update (drawable, sel_x1, sel_y1, width, height);
    }

  gimp_displays_flush ();

  return success;
}

static gboolean
plugin_dialog (GimpProcedure *procedure,
               GObject       *config)
{
  GtkWidget  *dlg;
  GtkWidget  *vbox;
  GtkWidget  *hbox;
  gboolean    run;

  gimp_ui_init (PLUG_IN_BINARY);

  dlg = gimp_procedure_dialog_new (procedure,
                                   GIMP_PROCEDURE_CONFIG (config),
                                   _("Hot"));

  gimp_procedure_dialog_get_widget (GIMP_PROCEDURE_DIALOG (dlg),
                                    "mode", GIMP_TYPE_INT_RADIO_FRAME);
  gimp_procedure_dialog_get_widget (GIMP_PROCEDURE_DIALOG (dlg),
                                    "action", GIMP_TYPE_INT_RADIO_FRAME);

  vbox = gimp_procedure_dialog_fill_box (GIMP_PROCEDURE_DIALOG (dlg),
                                         "hot-left-side",
                                         "mode",
                                         "new-layer",
                                         NULL);
  gtk_box_set_spacing (GTK_BOX (vbox), 12);

  hbox = gimp_procedure_dialog_fill_box (GIMP_PROCEDURE_DIALOG (dlg),
                                         "hot-hbox",
                                         "hot-left-side",
                                         "action",
                                         NULL);
  gtk_box_set_spacing (GTK_BOX (hbox), 12);
  gtk_box_set_homogeneous (GTK_BOX (hbox), TRUE);
  gtk_widget_set_margin_bottom (hbox, 12);
  gtk_orientable_set_orientation (GTK_ORIENTABLE (hbox),
                                  GTK_ORIENTATION_HORIZONTAL);

  gimp_procedure_dialog_fill (GIMP_PROCEDURE_DIALOG (dlg),
                              "hot-hbox",
                              NULL);

  gtk_widget_show (dlg);

  run = gimp_procedure_dialog_run (GIMP_PROCEDURE_DIALOG (dlg));

  gtk_widget_destroy (dlg);

  return run;
}

/*
 * build_tab: Build multiply lookup table.
 *
 * For each possible pixel value, decode value into floating-point
 * intensity.  Then do gamma correction required by the video
 * standard.  Scale the result by our fixed-point scale factor.
 * Then calculate 9 lookup table entries for this pixel value.
 *
 * We also calculate floating-point and scaled integer versions
 * of our limits here.  This prevents evaluating expressions every pixel
 * when the compiler is too stupid to evaluate constant-valued
 * floating-point expressions at compile time.
 *
 * For convenience, the limits are #defined using IRE units.
 * We must convert them here into the units in which YIQ
 * are measured.  The conversion from IRE to internal units
 * depends on the pedestal level in use, since as Y goes from
 * 0 to 1, the signal goes from the pedestal level to 100 IRE.
 * Chroma is always scaled to remain consistent with Y.
 */
static void
build_tab (int m)
{
  double f;
  int pv;

  for (pv = 0; pv <= MAXPIX; pv++)
    {
      f = (double) SCALE * (double) gc ((double) pix_decode (pv), m);
      tab[0][0][pv] = (int) (f * mode_vals[m].code[0][0] + 0.5);
      tab[0][1][pv] = (int) (f * mode_vals[m].code[0][1] + 0.5);
      tab[0][2][pv] = (int) (f * mode_vals[m].code[0][2] + 0.5);
      tab[1][0][pv] = (int) (f * mode_vals[m].code[1][0] + 0.5);
      tab[1][1][pv] = (int) (f * mode_vals[m].code[1][1] + 0.5);
      tab[1][2][pv] = (int) (f * mode_vals[m].code[1][2] + 0.5);
      tab[2][0][pv] = (int) (f * mode_vals[m].code[2][0] + 0.5);
      tab[2][1][pv] = (int) (f * mode_vals[m].code[2][1] + 0.5);
      tab[2][2][pv] = (int) (f * mode_vals[m].code[2][2] + 0.5);
    }

  chroma_lim = (double) CHROMA_LIM / (100.0 - mode_vals[m].pedestal);
  compos_lim = ((double )COMPOS_LIM - mode_vals[m].pedestal) /
    (100.0 - mode_vals[m].pedestal);

  ichroma_lim2 = (int)(chroma_lim * SCALE + 0.5);
  ichroma_lim2 *= ichroma_lim2;
  icompos_lim = (int)(compos_lim * SCALE + 0.5);
}

static gboolean
hotp (guint8 r,
      guint8 g,
      guint8 b)
{
  int   y, i, q;
  long  y2, c2;

  /*
   * Pixel decoding, gamma correction, and matrix multiplication
   * all done by lookup table.
   *
   * "i" and "q" are the two chrominance components;
   * they are I and Q for NTSC.
   * For PAL, "i" is U (scaled B-Y) and "q" is V (scaled R-Y).
   * Since we only care about the length of the chroma vector,
   * not its angle, we don't care which is which.
   */
  y = tab[0][0][r] + tab[0][1][g] + tab[0][2][b];
  i = tab[1][0][r] + tab[1][1][g] + tab[1][2][b];
  q = tab[2][0][r] + tab[2][1][g] + tab[2][2][b];

  /*
   * Check to see if the chrominance vector is too long or the
   * composite waveform amplitude is too large.
   *
   * Chrominance is too large if
   *
   *    sqrt(i^2, q^2)  >  chroma_lim.
   *
   * The composite signal amplitude is too large if
   *
   *    y + sqrt(i^2, q^2)  >  compos_lim.
   *
   * We avoid doing the sqrt by checking
   *
   *    i^2 + q^2  >  chroma_lim^2
   * and
   *    y + sqrt(i^2 + q^2)  >  compos_lim
   *    sqrt(i^2 + q^2)  >  compos_lim - y
   *    i^2 + q^2  >  (compos_lim - y)^2
   *
   */

  c2 = (long)i * i + (long)q * q;
  y2 = (long)icompos_lim - y;
  y2 *= y2;

  if (c2 <= ichroma_lim2 && c2 <= y2)
    {   /* no problems */
      return FALSE;
    }

  return TRUE;
}
