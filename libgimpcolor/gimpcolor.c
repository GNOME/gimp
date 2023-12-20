/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * gimpcolor.c
 * Copyright (C) 2023 Jehan <jehan@gimp.org>
 *
 * This library is free software: you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library.  If not, see
 * <https://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <cairo.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <math.h>

#include <babl/babl.h>
#include <gegl.h>

#include "gimpcolor.h"


/**
 * SECTION: gimpcolor
 * @title: GimpColor
 * @short_description: API to manipulate [class@Gegl.Color] objects.
 *
 * #GimpColor contains a few helper functions to manipulate [class@Gegl.Color]
 * objects more easily.
 **/


static const Babl * gimp_babl_format_get_with_alpha (const Babl *format);
static gfloat       gimp_color_get_CIE2000_distance (GeglColor  *color1,
                                                     GeglColor  *color2);


/**
 * gimp_color_set_alpha:
 * @color: a [class@Gegl.Color]
 * @alpha: new value for the alpha channel.
 *
 * Update the @alpha channel, and any other component if necessary (e.g. in case
 * of premultiplied channels), without changing the format of @color.
 *
 * If @color has no alpha component, this function is a no-op.
 *
 * Since: 3.0
 **/
void
gimp_color_set_alpha (GeglColor *color,
                      gdouble    alpha)
{
  const Babl *format;
  gdouble     red;
  gdouble     green;
  gdouble     blue;
  guint8      pixel[40];

  format = gegl_color_get_format (color);

  gegl_color_get_rgba (color, &red, &green, &blue, NULL);
  gegl_color_set_rgba (color, red, green, blue, alpha);

  /* I could stop at this point, but we want to keep the initial format as much
   * as possible. Since we made a round-trip through linear RGBA float, we need
   * to reset the right format.
   *
   * Also why we do this round trip is because we know we can just change the
   * alpha channel and babl fishes will do the appropriate conversion. I first
   * thought of updating the alpha channel directly by editing the raw data
   * depending on the format, but doing so would break e.g. with premultiplied
   * channels. Babl already has all the internal knowledge so let it do its
   * thing. The only risk is the possible precision loss during conversion.
   * Let's assume that since we use an unbounded 32-bit intermediate value
   * (float), the loss would be acceptable.
   */
  format = gimp_babl_format_get_with_alpha (format);
  gegl_color_get_pixel (color, format, pixel);
  gegl_color_set_pixel (color, format, pixel);
}

/**
 * gimp_color_is_perceptually_identical:
 * @color1: a [class@Gegl.Color]
 * @color2: a [class@Gegl.Color]
 *
 * Determine whether @color1 and @color2 can be considered identical to the
 * human eyes, by computing the distance in a color space as perceptually
 * uniform as possible.
 *
 * This function will also consider any transparency channel, so that if you
 * only want to compare the pure color, you could for instance set both color's
 * alpha channel to 1.0 first (possibly on duplicates of the colors if originals
 * should not be modified), such as:
 *
 * ```C
 * gimp_color_set_alpha (color1, 1.0);
 * gimp_color_set_alpha (color2, 1.0);
 * if (gimp_color_is_perceptually_identical (color1, color2))
 *   {
 *     printf ("Both colors are identical, ignoring their alpha component");
 *   }
 * ```
 *
 * Returns: whether the 2 colors can be considered the same for the human eyes.
 *
 * Since: 3.0
 **/
gboolean
gimp_color_is_perceptually_identical (GeglColor *color1,
                                      GeglColor *color2)
{
  g_return_val_if_fail (GEGL_IS_COLOR (color1), FALSE);
  g_return_val_if_fail (GEGL_IS_COLOR (color2), FALSE);

  /* All CIE deltaE distances were designed with a 1.0 JND (Just Noticeable
   * Difference), though there was some revision to 2.3 for the CIE76 version.
   * I could not find a reliable source about whether such a revision happened
   * for the CIE2000 algorithm. My own tests though seemed to lean towards
   * using ~0.6 for the JND. That's what I'm using for the time being.
   */
  return (gimp_color_get_CIE2000_distance (color1, color2) < 0.6);
}

/**
 * gimp_color_is_out_of_self_gamut:
 * @color: a [class@Gegl.Color]
 *
 * Determine whether @color is out of its own space gamut. This can only
 * happen if the color space is unbounded and any of the color component
 * is out of the `[0; 1]` range.
 * A small error of margin is accepted, so that for instance a component
 * at -0.0000001 is not making the whole color to be considered as
 * out-of-gamut while it may just be computation imprecision.
 *
 * Returns: whether the color is out of its own color space gamut.
 *
 * Since: 3.0
 **/
gboolean
gimp_color_is_out_of_self_gamut (GeglColor *color)
{
  const Babl *format;
  const Babl *space;
  const Babl *ctype;
  gboolean    oog = FALSE;

  format = gegl_color_get_format (color);
  space  = babl_format_get_space (format);
  /* XXX assuming that all components have the same type. */
  ctype  = babl_format_get_type (format, 0);

  if (ctype == babl_type ("half")  ||
      ctype == babl_type ("float") ||
      ctype == babl_type ("double"))
  {
      /* Only unbounded colors can be out-of-gamut. */
      const Babl *model;

      model = babl_format_get_model (format);

#define CHANNEL_EPSILON 1e-3
        if (model == babl_model ("R'G'B'")  ||
            model == babl_model ("R~G~B~")  ||
            model == babl_model ("RGB")     ||
            model == babl_model ("R'G'B'A") ||
            model == babl_model ("R~G~B~A") ||
            model == babl_model ("RGBA"))
        {
            gdouble rgb[3];

            gegl_color_get_pixel (color, babl_format_with_space ("RGB double", space), rgb);

            oog = ((rgb[0] < 0.0 && -rgb[0] > CHANNEL_EPSILON)      ||
                   (rgb[0] > 1.0 && rgb[0] - 1.0 > CHANNEL_EPSILON) ||
                   (rgb[1] < 0.0 && -rgb[1] > CHANNEL_EPSILON)      ||
                   (rgb[1] > 1.0 && rgb[1] - 1.0 > CHANNEL_EPSILON) ||
                   (rgb[2] < 0.0 && -rgb[2] > CHANNEL_EPSILON)      ||
                   (rgb[2] > 1.0 && rgb[2] - 1.0 > CHANNEL_EPSILON));
        }
        else if (model == babl_model ("Y'")  ||
                 model == babl_model ("Y~")  ||
                 model == babl_model ("Y")   ||
                 model == babl_model ("Y'A") ||
                 model == babl_model ("Y~A") ||
                 model == babl_model ("YA"))
        {
            gdouble gray[1];

            gegl_color_get_pixel (color, babl_format_with_space ("Y double", space), gray);
            oog = ((gray[0] < 0.0 && -gray[0] > CHANNEL_EPSILON)      ||
                   (gray[0] > 1.0 && gray[0] - 1.0 > CHANNEL_EPSILON));
        }
        else if (model == babl_model ("CMYK")  ||
                 model == babl_model ("CMYKA") ||
                 model == babl_model ("cmyk")  ||
                 model == babl_model ("cmykA"))
        {
            gdouble cmyk[4];

            gegl_color_get_pixel (color, babl_format_with_space ("CMYK double", space), cmyk);
            oog = ((cmyk[0] < 0.0 && -cmyk[0] > CHANNEL_EPSILON)      ||
                   (cmyk[0] > 1.0 && cmyk[0] - 1.0 > CHANNEL_EPSILON) ||
                   (cmyk[1] < 0.0 && -cmyk[1] > CHANNEL_EPSILON)      ||
                   (cmyk[1] > 1.0 && cmyk[1] - 1.0 > CHANNEL_EPSILON) ||
                   (cmyk[2] < 0.0 && -cmyk[2] > CHANNEL_EPSILON)      ||
                   (cmyk[2] > 1.0 && cmyk[2] - 1.0 > CHANNEL_EPSILON) ||
                   (cmyk[3] < 0.0 && -cmyk[3] > CHANNEL_EPSILON)      ||
                   (cmyk[3] > 1.0 && cmyk[3] - 1.0 > CHANNEL_EPSILON));
        }
#undef CHANNEL_EPSILON
    }

  return oog;
}

/**
 * gimp_color_is_out_of_gamut:
 * @color: a [class@Gegl.Color]
 * @space: a color space to convert @color to.
 *
 * Determine whether @color is out of its @space gamut.
 * A small error of margin is accepted, so that for instance a component
 * at -0.0000001 is not making the whole color to be considered as
 * out-of-gamut while it may just be computation imprecision.
 *
 * Returns: whether the color is out of @space gamut.
 *
 * Since: 3.0
 **/
gboolean
gimp_color_is_out_of_gamut (GeglColor  *color,
                            const Babl *space)
{
  gboolean is_out_of_gamut = FALSE;

#define CHANNEL_EPSILON 1e-3
  if (babl_space_is_gray (space))
    {
      gfloat gray[1];

      gegl_color_get_pixel (color,
                            babl_format_with_space ("Y' float", space),
                            gray);
      is_out_of_gamut = ((gray[0] < 0.0 && -gray[0] > CHANNEL_EPSILON)       ||
                         (gray[0] > 1.0 && gray[0] - 1.0 > CHANNEL_EPSILON));

      if (! is_out_of_gamut)
        {
          gdouble rgb[3];

          /* Grayscale colors can be out of gamut if the color is out of the [0;
           * 1] range in the target space and also if they can be converted to
           * RGB with non-equal components.
           */
          gegl_color_get_pixel (color,
                                babl_format_with_space ("R'G'B' double", space),
                                rgb);
          is_out_of_gamut = (ABS (rgb[0] - rgb[0]) > CHANNEL_EPSILON ||
                             ABS (rgb[1] - rgb[1]) > CHANNEL_EPSILON ||
                             ABS (rgb[2] - rgb[2]) > CHANNEL_EPSILON);
        }
    }
  else if (babl_space_is_cmyk (space))
    {
      GeglColor *c = gegl_color_new (NULL);
      gdouble    cmyk[4];

      /* CMYK conversion always produces colors in [0; 1] range. What we want
       * to check is whether the source and converted colors are the same in
       * Lab space.
       */
      gegl_color_get_pixel (color,
                            babl_format_with_space ("CMYK double", space),
                            cmyk);
      gegl_color_set_pixel (c, babl_format_with_space ("CMYK double", space), cmyk);
      is_out_of_gamut = (! gimp_color_is_perceptually_identical (color, c));
      g_object_unref (c);
    }
  else
    {
      gdouble rgb[3];

      gegl_color_get_pixel (color,
                            babl_format_with_space ("R'G'B' double", space),
                            rgb);
      is_out_of_gamut = ((rgb[0] < 0.0 && -rgb[0] > CHANNEL_EPSILON)       ||
                         (rgb[0] > 1.0 && rgb[0] - 1.0 > CHANNEL_EPSILON) ||
                         (rgb[1] < 0.0 && -rgb[1] > CHANNEL_EPSILON)       ||
                         (rgb[1] > 1.0 && rgb[1] - 1.0 > CHANNEL_EPSILON) ||
                         (rgb[2] < 0.0 && -rgb[2] > CHANNEL_EPSILON)       ||
                         (rgb[2] > 1.0 && rgb[2] - 1.0 > CHANNEL_EPSILON));
    }
#undef CHANNEL_EPSILON

  return is_out_of_gamut;
}


/* Private functions. */

static const Babl *
gimp_babl_format_get_with_alpha (const Babl *format)
{
  const Babl  *new_format = NULL;
  const gchar *new_model  = NULL;
  const gchar *model;
  const gchar *type;
  gchar       *name;

  if (babl_format_has_alpha (format))
    return format;

  model = babl_get_name (babl_format_get_model (format));
  /* Assuming we use Babl formats with same type for all components. */
  type  = babl_get_name (babl_format_get_type (format, 0));

  if (babl_format_is_palette (format))
    {
      gchar *alpha_palette = g_strdup (model);

      alpha_palette[0] = '\\';
      babl_new_palette_with_space (alpha_palette, babl_format_get_space (format),
                                   NULL, &new_format);
      g_free (alpha_palette);

      return new_format;
    }

  if (g_strcmp0 (model, "Y") == 0)
    new_model = "YA";
  else if (g_strcmp0 (model, "RGB") == 0)
    new_model = "RGBA";
  else if (g_strcmp0 (model, "Y'") == 0)
    new_model = "Y'A";
  else if (g_strcmp0 (model, "R'G'B'") == 0)
    new_model = "R'G'B'A";
  else if (g_strcmp0 (model, "Y~") == 0)
    new_model = "Y~A";
  else if (g_strcmp0 (model, "R~G~B~") == 0)
    new_model = "R~G~B~A";
  else if (g_strcmp0 (model, "CIE Lab") == 0)
    new_model = "CIE Lab alpha";
  else if (g_strcmp0 (model, "CIE xyY") == 0)
    new_model = "CIE xyY alpha";
  else if (g_strcmp0 (model, "CIE XYZ") == 0)
    new_model = "CIE XYZ alpha";
  else if (g_strcmp0 (model, "CIE Yuv") == 0)
    new_model = "CIE Yuv alpha";
  else if (g_strcmp0 (model, "CIE LCH(ab)") == 0)
    new_model = "CIE LCH(ab) alpha";
  else if (g_strcmp0 (model, "CMYK") == 0)
    new_model = "CMYKA";
  else if (g_strcmp0 (model, "cmyk") == 0)
    new_model = "cmykA";
  else if (g_strcmp0 (model, "HSL") == 0)
    new_model = "HSLA";
  else if (g_strcmp0 (model, "HSV") == 0)
    new_model = "HSVA";
  else if (g_strcmp0 (model, "cairo-RGB24") == 0)
    new_model = "cairo-ARGB32";

  if (new_model == NULL)
    {
      g_warning ("%s: unsupported format \"%s\".", G_STRFUNC, babl_get_name (format));
      return format;
    }

  name = g_strdup_printf ("%s %s", new_model, type);
  new_format = babl_format_with_space (name, format);
  g_free (name);

  return new_format;
}

/**
 * gimp_color_get_CIE2000_distance:
 * @color1: a [class@Gegl.Color]
 * @color2: a [class@Gegl.Color]
 *
 * Compute the CIEDE2000 distance between @color1 and @color2 which tries to
 * measure visual difference in the CIELAB color space while correcting the
 * computation to take into account the space being not perfectly perceptual
 * uniform.
 *
 * This function does not take into account any transparency channel.
 *
 * Returns: the distance computed using the CIEDE2000 algorithm.
 *
 * Since: 3.0
 **/
static gfloat
gimp_color_get_CIE2000_distance (GeglColor *color1,
                                 GeglColor *color2)
{
  gfloat lab1[3];
  gfloat lab2[3];
  gfloat dL;
  gfloat C_prime;
  gfloat dC;
  gfloat dh;
  gfloat dH;
  gfloat h_prime;
  gfloat T;
  gfloat L_50_2;
  gfloat SL;
  gfloat SC;
  gfloat SH;
  gfloat C_prime7;
  gfloat RT;
  gfloat dE00;
  gfloat RC;
  gfloat d0;

  g_return_val_if_fail (GEGL_IS_COLOR (color1), FALSE);
  g_return_val_if_fail (GEGL_IS_COLOR (color2), FALSE);

  gegl_color_get_pixel (color1, babl_format ("CIE LCH(ab) float"), lab1);
  gegl_color_get_pixel (color2, babl_format ("CIE LCH(ab) float"), lab2);

  dL = lab2[0] - lab1[0];
  dC = lab2[1] - lab1[1];
  dh = lab2[2] - lab1[2];
  dH = 2.f * sqrtf (lab1[1] * lab2[1]) * sinf (dh / 2.0f * M_PI / 180.f);

  h_prime = lab1[2] + lab2[2] ;
  if (lab1[1] * lab2[1] != 0.f)
    {
      if (fabsf (dh) <= 180.0f)
        {
          h_prime /= 2.0f;
        }
      else
        {
          if (h_prime < 360.f)
            h_prime = (h_prime + 360.f) / 2.f;
          else
            h_prime = (h_prime - 360.f) / 2.f;
        }
    }
  T = 1.f - 0.17f * cosf ((h_prime - 30.f) * M_PI / 180.f) + 0.24f * cosf (2.f * h_prime * M_PI / 180.f) +
      0.32f * cosf ((3.f * h_prime + 6.f) * M_PI / 180.f) - 0.2f * cosf ((4.f * h_prime - 63.f) * M_PI / 180.f);
  C_prime = (lab1[1] + lab2[1]) / 2.f;
  L_50_2 = (((lab1[0] + lab2[0]) / 2.f) - 50.f);
  L_50_2 *= L_50_2;
  SL = 1.f + 0.015f * L_50_2 / sqrtf (20.f + L_50_2);
  SC = 1.f + 0.045f * C_prime;
  SH = 1.f + 0.015f * C_prime * T;

  C_prime7 = powf (C_prime, 7.f);
  d0 = 30.f * expf (- powf ((h_prime - 275.f) / 25.f, 2.f));
#define CONST_25_POWER_7 6103515625.0f
  RC = 2.f * sqrtf (C_prime7  / (C_prime7 + CONST_25_POWER_7));
#undef CONST_25_POWER_7
  RT = - sinf (2.f * d0 * M_PI / 180.f) * RC;
  dE00 = sqrtf (powf (dL / SL, 2.f) + powf (dC / SC, 2.f) + powf (dH / SH, 2.f) +
                RT * dC * dH / SC / SH);

  return dE00;
}
