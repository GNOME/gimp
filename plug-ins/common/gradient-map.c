/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * Gradient Map plug-in
 * Copyright (C) 1997 Eiichi Takamori <taka@ma1.seikyou.ne.jp>
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
#include <string.h>

#include <libgimp/gimp.h>

#include "libgimp/stdplugins-intl.h"


/* Some useful macros */

#define GRADMAP_PROC    "plug-in-gradmap"
#define PALETTEMAP_PROC "plug-in-palettemap"
#define PLUG_IN_BINARY  "gradient-map"
#define NSAMPLES        256
#define LUMINOSITY(X)   (GIMP_RGB_LUMINANCE (X[0], X[1], X[2]) + 0.5)

typedef enum
  {
    GRADIENT_MODE = 1,
    PALETTE_MODE
  } MapMode;

/* Declare a local function.
 */
static void     query       (void);
static void     run         (const gchar      *name,
                             gint              nparams,
                             const GimpParam  *param,
                             gint             *nreturn_vals,
                             GimpParam       **return_vals);

static void     map                  (GimpDrawable     *drawable,
                                      MapMode           mode);
static guchar * get_samples_gradient (GimpDrawable     *drawable);
static guchar * get_samples_palette  (GimpDrawable     *drawable);
static void     map_func             (const guchar     *src,
                                      guchar           *dest,
                                      gint              bpp,
                                      gpointer          data);


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
  static const GimpParamDef args[]=
  {
    { GIMP_PDB_INT32,    "run-mode", "The run mode { RUN-INTERACTIVE (0), RUN-NONINTERACTIVE (1) }" },
    { GIMP_PDB_IMAGE,    "image",    "Input image (unused)" },
    { GIMP_PDB_DRAWABLE, "drawable", "Input drawable"       }
  };

  gimp_install_procedure (GRADMAP_PROC,
                          N_("Recolor the image using colors from the active gradient"),
                          "This plug-in maps the contents of the specified "
                          "drawable with active gradient. It calculates "
                          "luminosity of each pixel and replaces the pixel "
                          "by the sample of active gradient at the position "
                          "proportional to that luminosity. Complete black "
                          "pixel becomes the leftmost color of the gradient, "
                          "and complete white becomes the rightmost. Works on "
                          "both Grayscale and RGB image with/without alpha "
                          "channel.",
                          "Eiichi Takamori",
                          "Eiichi Takamori",
                          "1997",
                          N_("_Gradient Map"),
                          "RGB*, GRAY*",
                          GIMP_PLUGIN,
                          G_N_ELEMENTS (args), 0,
                          args, NULL);

  gimp_plugin_menu_register (GRADMAP_PROC, "<Image>/Colors/Map");

  gimp_install_procedure (PALETTEMAP_PROC,
                          N_("Recolor the image using colors from the active palette"),
                          "This plug-in maps the contents of the specified "
                          "drawable with the active palette. It calculates "
                          "luminosity of each pixel and replaces the pixel "
                          "by the palette sample  at the corresponding "
                          "index. A complete black "
                          "pixel becomes the lowest palette entry, "
                          "and complete white becomes the highest. Works on "
                          "both Grayscale and RGB image with/without alpha "
                          "channel.",
                          "Bill Skaggs",
                          "Bill Skaggs",
                          "2004",
                          N_("_Palette Map"),
                          "RGB*, GRAY*",
                          GIMP_PLUGIN,
                          G_N_ELEMENTS (args), 0,
                          args, NULL);

  gimp_plugin_menu_register (PALETTEMAP_PROC, "<Image>/Colors/Map");
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
  GimpRunMode        run_mode;

  run_mode = param[0].data.d_int32;

  INIT_I18N();

  *nreturn_vals = 1;
  *return_vals  = values;

  values[0].type          = GIMP_PDB_STATUS;
  values[0].data.d_status = status;

  /*  Get the specified drawable  */
  drawable = gimp_drawable_get (param[2].data.d_drawable);

  /*  Make sure that the drawable is gray or RGB color  */
  if (gimp_drawable_is_rgb (drawable->drawable_id) ||
      gimp_drawable_is_gray (drawable->drawable_id))
    {
      MapMode mode = 0;

      if ( !strcmp (name, GRADMAP_PROC))
        {
          mode = GRADIENT_MODE;
          gimp_progress_init (_("Gradient Map"));
        }
      else if ( !strcmp (name, PALETTEMAP_PROC))
        {
          mode = PALETTE_MODE;
          gimp_progress_init (_("Palette Map"));
        }
      else
        {
          status = GIMP_PDB_CALLING_ERROR;
        }

      if (status == GIMP_PDB_SUCCESS)
        {
          if (mode)
            map (drawable, mode);

          if (run_mode != GIMP_RUN_NONINTERACTIVE)
            gimp_displays_flush ();
        }
    }
  else
    {
      status = GIMP_PDB_EXECUTION_ERROR;
    }

  values[0].data.d_status = status;

  gimp_drawable_detach (drawable);
}

typedef struct
{
  guchar   *samples;
  gboolean  is_rgb;
  gboolean  has_alpha;
  MapMode   mode;
} MapParam;

static void
map_func (const guchar *src,
          guchar       *dest,
          gint          bpp,
          gpointer      data)
{
  MapParam *param = data;
  gint      lum;
  gint      b;
  guchar   *samp;

  lum = (param->is_rgb) ? LUMINOSITY (src) : src[0];
  samp = &param->samples[lum * bpp];

  if (param->has_alpha)
    {
      for (b = 0; b < bpp - 1; b++)
        dest[b] = samp[b];
      dest[b] = ((guint)samp[b] * (guint)src[b]) / 255;
    }
  else
    {
      for (b = 0; b < bpp; b++)
        dest[b] = samp[b];
    }
}

static void
map (GimpDrawable *drawable,
     MapMode       mode)
{
  MapParam param;

  param.is_rgb = gimp_drawable_is_rgb (drawable->drawable_id);
  param.has_alpha = gimp_drawable_has_alpha (drawable->drawable_id);

  switch (mode)
    {
    case GRADIENT_MODE:
      param.samples = get_samples_gradient (drawable);
      break;
    case PALETTE_MODE:
      param.samples = get_samples_palette (drawable);
      break;
    default:
      g_error ("plug_in_gradmap: invalid mode");
    }

  gimp_rgn_iterate2 (drawable, 0 /* unused */, map_func, &param);
}

/*
  Returns 256 samples of active gradient.
  Each sample has (gimp_drawable_bpp (drawable->drawable_id)) bytes.
 */
static guchar *
get_samples_gradient (GimpDrawable *drawable)
{
  gchar   *gradient_name;
  gint     n_f_samples;
  gdouble *f_samples, *f_samp;  /* float samples */
  guchar  *byte_samples, *b_samp;  /* byte samples */
  gint     bpp, color, has_alpha, alpha;
  gint     i, j;

  gradient_name = gimp_context_get_gradient ();

#ifdef __GNUC__
#warning FIXME: "reverse" hardcoded to FALSE.
#endif
  gimp_gradient_get_uniform_samples (gradient_name, NSAMPLES, FALSE,
                                     &n_f_samples, &f_samples);

  bpp       = gimp_drawable_bpp (drawable->drawable_id);
  color     = gimp_drawable_is_rgb (drawable->drawable_id);
  has_alpha = gimp_drawable_has_alpha (drawable->drawable_id);
  alpha     = (has_alpha ? bpp - 1 : bpp);

  byte_samples = g_new (guchar, NSAMPLES * bpp);

  for (i = 0; i < NSAMPLES; i++)
    {
      b_samp = &byte_samples[i * bpp];
      f_samp = &f_samples[i * 4];
      if (color)
        for (j = 0; j < 3; j++)
          b_samp[j] = f_samp[j] * 255;
      else
        b_samp[0] = LUMINOSITY (f_samp) * 255;

      if (has_alpha)
        b_samp[alpha] = f_samp[3] * 255;
    }

  g_free (f_samples);

  return byte_samples;
}

/*
  Returns 256 samples of the palette.
  Each sample has (gimp_drawable_bpp (drawable->drawable_id)) bytes.
 */
static guchar *
get_samples_palette (GimpDrawable *drawable)
{
  gchar   *palette_name;
  GimpRGB  color_sample;
  guchar  *byte_samples;
  guchar  *b_samp;
  gint     bpp, color, has_alpha, alpha;
  gint     i;
  gint     num_colors;
  gfloat   factor;
  gint     pal_entry;

  palette_name = gimp_context_get_palette ();
  gimp_palette_get_info (palette_name, &num_colors);

  bpp       = gimp_drawable_bpp (drawable->drawable_id);
  color     = gimp_drawable_is_rgb (drawable->drawable_id);
  has_alpha = gimp_drawable_has_alpha (drawable->drawable_id);
  alpha     = (has_alpha ? bpp - 1 : bpp);

  byte_samples = g_new (guchar, NSAMPLES * bpp);
  factor = ( (float) num_colors) / NSAMPLES;

  for (i = 0; i < NSAMPLES; i++)
    {
      b_samp = &byte_samples[i * bpp];

      pal_entry = CLAMP( (int)(i * factor), 0, num_colors);
      gimp_palette_entry_get_color (palette_name, pal_entry, &color_sample);

      if (color)
        gimp_rgb_get_uchar (&color_sample,
                            b_samp, b_samp + 1, b_samp + 2);
      else
        *b_samp = gimp_rgb_luminance_uchar (&color_sample);

      if (has_alpha)
        b_samp[alpha] = 255;

    }

  return byte_samples;
}
