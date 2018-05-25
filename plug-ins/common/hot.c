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
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
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


typedef struct
{
  gint32 image;
  gint32 drawable;
  gint32 mode;
  gint32 action;
  gint32 new_layerp;
} piArgs;

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
} static mode[2] = {
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

static gint     tab[3][3][MAXPIX+1]; /* multiply lookup table */
static gdouble  chroma_lim;          /* chroma limit */
static gdouble  compos_lim;          /* composite amplitude limit */
static glong    ichroma_lim2;        /* chroma limit squared (scaled integer) */
static gint     icompos_lim;         /* composite amplitude limit (scaled integer) */

static void       query         (void);
static void       run           (const gchar      *name,
                                 gint              nparam,
                                 const GimpParam  *param,
                                 gint             *nretvals,
                                 GimpParam       **retvals);

static gboolean   pluginCore    (piArgs           *argp);
static gboolean   plugin_dialog (piArgs           *argp);
static gboolean   hotp          (guint8            r,
                                 guint8            g,
                                 guint8            b);
static void       build_tab     (gint              m);

/*
 * gc: apply the gamma correction specified for this video standard.
 * inv_gc: inverse function of gc.
 *
 * These are generally just a call to pow(), but be careful!
 * Future standards may use more complex functions.
 * (e.g. SMPTE 240M's "electro-optic transfer characteristic").
 */
#define gc(x,m)     pow(x, 1.0 / mode[m].gamma)
#define inv_gc(x,m) pow(x,       mode[m].gamma)

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

const GimpPlugInInfo PLUG_IN_INFO =
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
    { GIMP_PDB_INT32,    "run-mode",  "The run mode { RUN-INTERACTIVE (0), RUN-NONINTERACTIVE (1) }" },
    { GIMP_PDB_IMAGE,    "image",     "The Image" },
    { GIMP_PDB_DRAWABLE, "drawable",  "The Drawable" },
    { GIMP_PDB_INT32,    "mode",      "Mode { NTSC (0), PAL (1) }" },
    { GIMP_PDB_INT32,    "action",    "The action to perform" },
    { GIMP_PDB_INT32,    "new-layer", "Create a new layer { TRUE, FALSE }" }
  };

  gimp_install_procedure (PLUG_IN_PROC,
                          N_("Find and fix pixels that may be unsafely bright"),
                          "hot scans an image for pixels that will give unsave "
                          "values of chrominance or composite signale "
                          "amplitude when encoded into an NTSC or PAL signal.  "
                          "Three actions can be performed on these ``hot'' "
                          "pixels. (0) reduce luminance, (1) reduce "
                          "saturation, or (2) Blacken.",
                          "Eric L. Hernes, Alan Wm Paeth",
                          "Eric L. Hernes",
                          "1997",
                          N_("_Hot..."),
                          "RGB",
                          GIMP_PLUGIN,
                          G_N_ELEMENTS (args), 0,
                          args, NULL);

  gimp_plugin_menu_register (PLUG_IN_PROC, "<Image>/Colors/Modify");
}

static void
run (const gchar      *name,
     gint              nparam,
     const GimpParam  *param,
     gint             *nretvals,
     GimpParam       **retvals)
{
  static GimpParam rvals[1];
  piArgs           args;

  *nretvals = 1;
  *retvals  = rvals;

  INIT_I18N ();

  memset (&args, 0, sizeof (args));
  args.mode = -1;

  gimp_get_data (PLUG_IN_PROC, &args);

  args.image    = param[1].data.d_image;
  args.drawable = param[2].data.d_drawable;

  rvals[0].type          = GIMP_PDB_STATUS;
  rvals[0].data.d_status = GIMP_PDB_SUCCESS;

  switch (param[0].data.d_int32)
    {
    case GIMP_RUN_INTERACTIVE:
      /* XXX: add code here for interactive running */
      if (args.mode == -1)
        {
          args.mode       = MODE_NTSC;
          args.action     = ACT_LREDUX;
          args.new_layerp = 1;
        }

      if (plugin_dialog (&args))
        {
          if (! pluginCore (&args))
            {
              rvals[0].data.d_status = GIMP_PDB_EXECUTION_ERROR;
            }
        }
      else
        {
          rvals[0].data.d_status = GIMP_PDB_CANCEL;
        }

      gimp_set_data (PLUG_IN_PROC, &args, sizeof (args));
    break;

    case GIMP_RUN_NONINTERACTIVE:
      /* XXX: add code here for non-interactive running */
      if (nparam != 6)
        {
          rvals[0].data.d_status = GIMP_PDB_CALLING_ERROR;
          break;
        }
      args.mode       = param[3].data.d_int32;
      args.action     = param[4].data.d_int32;
      args.new_layerp = param[5].data.d_int32;

      if (! pluginCore (&args))
        {
          rvals[0].data.d_status = GIMP_PDB_EXECUTION_ERROR;
          break;
        }
    break;

    case GIMP_RUN_WITH_LAST_VALS:
      /* XXX: add code here for last-values running */
      if (! pluginCore (&args))
        {
          rvals[0].data.d_status = GIMP_PDB_EXECUTION_ERROR;
        }
    break;
  }
}

static gboolean
pluginCore (piArgs *argp)
{
  GimpDrawable *drw, *ndrw = NULL;
  GimpPixelRgn  srcPr, dstPr;
  gboolean      success = TRUE;
  gint          nl      = 0;
  gint          y, i;
  gint          Y, I, Q;
  gint          width, height, bpp;
  gint          sel_x1, sel_x2, sel_y1, sel_y2;
  gint          prog_interval;
  guchar       *src, *s, *dst, *d;
  guchar        r, prev_r=0, new_r=0;
  guchar        g, prev_g=0, new_g=0;
  guchar        b, prev_b=0, new_b=0;
  gdouble       fy, fc, t, scale;
  gdouble       pr, pg, pb;
  gdouble       py;

  drw = gimp_drawable_get (argp->drawable);

  width  = drw->width;
  height = drw->height;
  bpp    = drw->bpp;

  if (argp->new_layerp)
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
                  mode_names[argp->mode],
                  action_names[argp->action]);

      nl = gimp_layer_new (argp->image, name, width, height,
                           GIMP_RGBA_IMAGE,
                           100,
                           gimp_image_get_default_new_layer_mode (argp->image));
      ndrw = gimp_drawable_get (nl);
      gimp_drawable_fill (nl, GIMP_FILL_TRANSPARENT);
      gimp_image_insert_layer (argp->image, nl, -1, 0);
    }

  if (! gimp_drawable_mask_intersect (drw->drawable_id,
                                      &sel_x1, &sel_y1, &width, &height))
    return success;

  sel_x2 = sel_x1 + width;
  sel_y2 = sel_y1 + height;

  src = g_new (guchar, width * height * bpp);
  dst = g_new (guchar, width * height * 4);
  gimp_pixel_rgn_init (&srcPr, drw, sel_x1, sel_y1, width, height,
                       FALSE, FALSE);

  if (argp->new_layerp)
    {
      gimp_pixel_rgn_init (&dstPr, ndrw, sel_x1, sel_y1, width, height,
                           FALSE, FALSE);
    }
  else
    {
      gimp_pixel_rgn_init (&dstPr, drw, sel_x1, sel_y1, width, height,
                           TRUE, TRUE);
    }

  gimp_pixel_rgn_get_rect (&srcPr, src, sel_x1, sel_y1, width, height);

  s = src;
  d = dst;

  build_tab (argp->mode);

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
              if (argp->action == ACT_FLAG)
                {
                  for (i = 0; i < 3; i++)
                    *d++ = 0;
                  s += 3;
                  if (bpp == 4)
                    *d++ = *s++;
                  else if (argp->new_layerp)
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
                      if (bpp == 4)
                        *d++ = *s++;
                      else if (argp->new_layerp)
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
                      if (argp->action == ACT_LREDUX)
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
                          scale = pow (scale, mode[argp->mode].gamma);

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

                          pr = gc (pr, argp->mode);
                          pg = gc (pg, argp->mode);
                          pb = gc (pb, argp->mode);

                          py = pr * mode[argp->mode].code[0][0] +
                               pg * mode[argp->mode].code[0][1] +
                               pb * mode[argp->mode].code[0][2];

                          r = pix_encode (inv_gc (py + scale * (pr - py),
                                                  argp->mode));
                          g = pix_encode (inv_gc (py + scale * (pg - py),
                                                  argp->mode));
                          b = pix_encode (inv_gc (py + scale * (pb - py),
                                                  argp->mode));
                        }

                      *d++ = new_r = r;
                      *d++ = new_g = g;
                      *d++ = new_b = b;

                      s += 3;

                      if (bpp == 4)
                        *d++ = *s++;
                      else if (argp->new_layerp)
                        *d++ = 255;
                    }
                }
            }
          else
            {
              if (!argp->new_layerp)
                {
                  for (i = 0; i < bpp; i++)
                    *d++ = *s++;
                }
              else
                {
                  s += bpp;
                  d += 4;
                }
            }
        }
    }
  gimp_progress_update (1.0);

  gimp_pixel_rgn_set_rect (&dstPr, dst, sel_x1, sel_y1, width, height);

  g_free (src);
  g_free (dst);

  if (argp->new_layerp)
    {
      gimp_drawable_flush (ndrw);
      gimp_drawable_update (nl, sel_x1, sel_y1, width, height);
    }
  else
    {
      gimp_drawable_flush (drw);
      gimp_drawable_merge_shadow (drw->drawable_id, TRUE);
      gimp_drawable_update (drw->drawable_id, sel_x1, sel_y1, width, height);
    }

  gimp_displays_flush ();

  return success;
}

static gboolean
plugin_dialog (piArgs *argp)
{
  GtkWidget *dlg;
  GtkWidget *hbox;
  GtkWidget *vbox;
  GtkWidget *toggle;
  GtkWidget *frame;
  gboolean   run;

  gimp_ui_init (PLUG_IN_BINARY, FALSE);

  dlg = gimp_dialog_new (_("Hot"), PLUG_IN_ROLE,
                         NULL, 0,
                         gimp_standard_help_func, PLUG_IN_PROC,

                         _("_Cancel"), GTK_RESPONSE_CANCEL,
                         _("_OK"),     GTK_RESPONSE_OK,

                         NULL);

  gimp_dialog_set_alternative_button_order (GTK_DIALOG (dlg),
                                           GTK_RESPONSE_OK,
                                           GTK_RESPONSE_CANCEL,
                                           -1);

  gimp_window_set_transient (GTK_WINDOW (dlg));

  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 12);
  gtk_container_set_border_width (GTK_CONTAINER (hbox), 12);
  gtk_box_pack_start (GTK_BOX (gtk_dialog_get_content_area (GTK_DIALOG (dlg))),
                      hbox, TRUE, TRUE, 0);
  gtk_widget_show (hbox);

  vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 12);
  gtk_box_pack_start (GTK_BOX (hbox), vbox, TRUE, TRUE, 0);
  gtk_widget_show (vbox);

  frame = gimp_int_radio_group_new (TRUE, _("Mode"),
                                    G_CALLBACK (gimp_radio_button_update),
                                    &argp->mode, argp->mode,

                                    "N_TSC", MODE_NTSC, NULL,
                                    "_PAL",  MODE_PAL,  NULL,

                                    NULL);

  gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  toggle = gtk_check_button_new_with_mnemonic (_("Create _new layer"));
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (toggle), argp->new_layerp);
  gtk_box_pack_start (GTK_BOX (vbox), toggle, FALSE, FALSE, 0);
  gtk_widget_show (toggle);

  g_signal_connect (toggle, "toggled",
                    G_CALLBACK (gimp_toggle_button_update),
                    &argp->new_layerp);

  frame = gimp_int_radio_group_new (TRUE, _("Action"),
                                    G_CALLBACK (gimp_radio_button_update),
                                    &argp->action, argp->action,

                                    _("Reduce _Luminance"),  ACT_LREDUX, NULL,
                                    _("Reduce _Saturation"), ACT_SREDUX, NULL,
                                    _("_Blacken"),           ACT_FLAG,   NULL,

                                    NULL);

  gtk_box_pack_start (GTK_BOX (hbox), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  gtk_widget_show (dlg);

  run = (gimp_dialog_run (GIMP_DIALOG (dlg)) == GTK_RESPONSE_OK);

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
      f = (double)SCALE * (double)gc((double)pix_decode(pv),m);
      tab[0][0][pv] = (int)(f * mode[m].code[0][0] + 0.5);
      tab[0][1][pv] = (int)(f * mode[m].code[0][1] + 0.5);
      tab[0][2][pv] = (int)(f * mode[m].code[0][2] + 0.5);
      tab[1][0][pv] = (int)(f * mode[m].code[1][0] + 0.5);
      tab[1][1][pv] = (int)(f * mode[m].code[1][1] + 0.5);
      tab[1][2][pv] = (int)(f * mode[m].code[1][2] + 0.5);
      tab[2][0][pv] = (int)(f * mode[m].code[2][0] + 0.5);
      tab[2][1][pv] = (int)(f * mode[m].code[2][1] + 0.5);
      tab[2][2][pv] = (int)(f * mode[m].code[2][2] + 0.5);
    }

  chroma_lim = (double)CHROMA_LIM / (100.0 - mode[m].pedestal);
  compos_lim = ((double)COMPOS_LIM - mode[m].pedestal) /
    (100.0 - mode[m].pedestal);

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
