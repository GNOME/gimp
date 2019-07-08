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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "config.h"
#include <string.h>

#include <libgimp/gimp.h>

#include "libgimp/stdplugins-intl.h"


/* Some useful macros */
#define GRADMAP_PROC    "plug-in-gradmap"
#define PALETTEMAP_PROC "plug-in-palettemap"
#define PLUG_IN_BINARY  "gradient-map"
#define PLUG_IN_ROLE    "gimp-gradient-map"
#define NSAMPLES         2048

typedef enum
  {
    GRADIENT_MODE = 1,
    PALETTE_MODE
  } MapMode;

static void     query                 (void);
static void     run                   (const gchar      *name,
                                       gint              nparams,
                                       const GimpParam  *param,
                                       gint             *nreturn_vals,
                                       GimpParam       **return_vals);
static void     map                   (GeglBuffer       *buffer,
                                       GeglBuffer       *shadow_buffer,
                                       gint32            drawable_id,
                                       MapMode           mode);
static gdouble * get_samples_gradient (gint32            drawable_id);
static gdouble * get_samples_palette  (gint32            drawable_id);


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
  GimpPDBStatusType  status = GIMP_PDB_SUCCESS;
  GimpRunMode        run_mode;
  gint32             drawable_id;
  GeglBuffer        *shadow_buffer;
  GeglBuffer        *buffer;

  run_mode    = param[0].data.d_int32;
  drawable_id = param[2].data.d_drawable;

  INIT_I18N ();
  gegl_init (NULL, NULL);

  *nreturn_vals = 1;
  *return_vals  = values;

  values[0].type          = GIMP_PDB_STATUS;
  values[0].data.d_status = status;

  /*  Get the specified drawable  */
  shadow_buffer = gimp_drawable_get_shadow_buffer (drawable_id);
  buffer        = gimp_drawable_get_buffer (drawable_id);

  /*  Make sure that the drawable is gray or RGB color  */
  if (gimp_drawable_is_rgb  (drawable_id) ||
      gimp_drawable_is_gray (drawable_id))
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
            map (buffer, shadow_buffer, drawable_id, mode);
        }
    }
  else
    {
      status = GIMP_PDB_EXECUTION_ERROR;
    }

  g_object_unref (buffer);
  g_object_unref (shadow_buffer);

  gimp_drawable_merge_shadow (drawable_id, TRUE);

  gimp_drawable_update (drawable_id, 0, 0,
                        gimp_drawable_width (drawable_id),
                        gimp_drawable_height (drawable_id));

  values[0].data.d_status = status;

  if (run_mode != GIMP_RUN_NONINTERACTIVE)
    gimp_displays_flush ();
}

static void
map (GeglBuffer   *buffer,
     GeglBuffer   *shadow_buffer,
     gint32        drawable_id,
     MapMode       mode)
{
  GeglBufferIterator *gi;
  gint                nb_color_chan;
  gint                nb_chan;
  gint                nb_chan2;
  gint                nb_chan_samp;
  gint                index_iter;
  gboolean            interpolate;
  gdouble            *samples;
  gboolean            is_rgb;
  gboolean            has_alpha;
  const Babl         *format_shadow;
  const Babl         *format_buffer;

  is_rgb = gimp_drawable_is_rgb (drawable_id);
  has_alpha = gimp_drawable_has_alpha (drawable_id);

  switch (mode)
    {
    case GRADIENT_MODE:
      samples = get_samples_gradient (drawable_id);
      interpolate = TRUE;
      break;
    case PALETTE_MODE:
      samples = get_samples_palette (drawable_id);
      interpolate = FALSE;
      break;
    default:
      g_error ("plug_in_gradmap: invalid mode");
    }

  if (is_rgb)
    {
      nb_color_chan = 3;
      nb_chan_samp = 4;
      if (has_alpha)
        format_shadow = babl_format ("R'G'B'A float");
      else
        format_shadow = babl_format ("R'G'B' float");
    }
  else
    {
      nb_color_chan = 1;
      nb_chan_samp = 2;
      if (has_alpha)
        format_shadow = babl_format ("Y'A float");
      else
        format_shadow = babl_format ("Y' float");
    }


  if (has_alpha)
    {
      nb_chan = nb_color_chan + 1;
      nb_chan2 = 2;
      format_buffer = babl_format ("Y'A float");
    }
  else
    {
      nb_chan = nb_color_chan;
      nb_chan2 = 1;
      format_buffer = babl_format ("Y' float");
    }

  gi = gegl_buffer_iterator_new (shadow_buffer, NULL, 0, format_shadow,
                                 GEGL_ACCESS_WRITE, GEGL_ABYSS_NONE, 2);

  index_iter = gegl_buffer_iterator_add (gi, buffer, NULL,
                                         0, format_buffer,
                                         GEGL_ACCESS_READ, GEGL_ABYSS_NONE);

  while (gegl_buffer_iterator_next (gi))
    {
      guint   k;
      gfloat *data;
      gfloat *data2;

      data  = (gfloat*) gi->items[0].data;
      data2 = (gfloat*) gi->items[index_iter].data;

      if (interpolate)
        {
          for (k = 0; k < gi->length; k++)
            {
              gint      b, ind1, ind2;
              gdouble  *samp1, *samp2;
              gfloat    c1, c2, val;

              val = data2[0] * (NSAMPLES-1);

              ind1 = CLAMP (floor (val), 0, NSAMPLES-1);
              ind2 = CLAMP (ceil  (val), 0, NSAMPLES-1);

              c1 = 1.0 - (val - ind1);
              c2 = 1.0 - c1;

              samp1 = &(samples[ind1 * nb_chan_samp]);
              samp2 = &(samples[ind2 * nb_chan_samp]);

              for (b = 0; b < nb_color_chan; b++)
                data[b] = (samp1[b] * c1 + samp2[b] * c2);

              if (has_alpha)
                {
                  float alpha = (samp1[b] * c1 + samp2[b] * c2);
                  data[b] = alpha * data2[1];
                }

              data += nb_chan;
              data2 += nb_chan2;
            }
        }
      else
        {
          for (k = 0; k < gi->length; k++)
            {
              gint     b, ind;
              gdouble *samp;
              ind = CLAMP (data2[0] * (NSAMPLES-1), 0, NSAMPLES-1);

              samp = &(samples[ind * nb_chan_samp]);

              for (b = 0; b < nb_color_chan; b++)
                data[b] = samp[b];

              if (has_alpha)
                {
                  data[b] = samp[b] * data2[1];
                }

              data += nb_chan;
              data2 += nb_chan2;
            }
        }
    }

  g_free (samples);
}

/*
  Returns 2048 samples of the gradient.
  Each sample is (R'G'B'A float) or (Y'A float), depending on the drawable
 */
static gdouble *
get_samples_gradient (gint32 drawable_id)
{
  gchar   *gradient_name;
  gint     n_d_samples;
  gdouble *d_samples = NULL;

  gradient_name = gimp_context_get_gradient ();

  /* FIXME: "reverse" hardcoded to FALSE. */
  gimp_gradient_get_uniform_samples (gradient_name, NSAMPLES, FALSE,
                                     &n_d_samples, &d_samples);
  g_free (gradient_name);

  if (!gimp_drawable_is_rgb (drawable_id))
    {
      const Babl *format_src = babl_format ("R'G'B'A double");
      const Babl *format_dst = babl_format ("Y'A double");
      const Babl *fish = babl_fish (format_src, format_dst);
      babl_process (fish, d_samples, d_samples, NSAMPLES);
    }

  return d_samples;
}

/*
  Returns 2048 samples of the palette.
  Each sample is (R'G'B'A float) or (Y'A float), depending on the drawable
 */
static gdouble *
get_samples_palette (gint32 drawable_id)
{
  gchar      *palette_name;
  GimpRGB     color_sample;
  gdouble    *d_samples, *d_samp;
  gboolean    is_rgb;
  gdouble     factor;
  gint        pal_entry, num_colors;
  gint        nb_color_chan, nb_chan, i;
  const Babl *format;

  palette_name = gimp_context_get_palette ();
  gimp_palette_get_info (palette_name, &num_colors);

  is_rgb = gimp_drawable_is_rgb (drawable_id);

  factor = ((double) num_colors) / NSAMPLES;
  format = is_rgb ? babl_format ("R'G'B'A double") : babl_format ("Y'A double");
  nb_color_chan = is_rgb ? 3 : 1;
  nb_chan = nb_color_chan + 1;

  d_samples = g_new (gdouble, NSAMPLES * nb_chan);

  for (i = 0; i < NSAMPLES; i++)
    {
      d_samp = &d_samples[i * nb_chan];
      pal_entry = CLAMP ((int)(i * factor), 0, num_colors - 1);

      gimp_palette_entry_get_color (palette_name, pal_entry, &color_sample);
      gimp_rgb_get_pixel (&color_sample,
                          format,
                          d_samp);
    }

  g_free (palette_name);
  return d_samples;
}
