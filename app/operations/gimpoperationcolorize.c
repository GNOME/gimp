/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpoperationcolorize.c
 * Copyright (C) 2007 Michael Natterer <mitch@gimp.org>
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

#include <cairo.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gegl.h>

#include "libgimpcolor/gimpcolor.h"
#include "libgimpcolor/gimpcolor-private.h"
#include "libgimpconfig/gimpconfig.h"

#include "operations-types.h"

#include "gimpoperationcolorize.h"

#include "gimp-intl.h"


enum
{
  PROP_0,
  PROP_HUE,
  PROP_SATURATION,
  PROP_LIGHTNESS,
  PROP_COLOR
};


static void     gimp_operation_colorize_get_property (GObject             *object,
                                                      guint                property_id,
                                                      GValue              *value,
                                                      GParamSpec          *pspec);
static void     gimp_operation_colorize_set_property (GObject             *object,
                                                      guint                property_id,
                                                      const GValue        *value,
                                                      GParamSpec          *pspec);

static void     gimp_operation_colorize_prepare      (GeglOperation       *operation);

static gboolean gimp_operation_colorize_process      (GeglOperation       *operation,
                                                      void                *in_buf,
                                                      void                *out_buf,
                                                      glong                samples,
                                                      const GeglRectangle *roi,
                                                      gint                 level);

static void     gimp_hsl_to_non_linear_rgb           (const gfloat        *hsl,
                                                      gfloat              *rgb);


G_DEFINE_TYPE (GimpOperationColorize, gimp_operation_colorize,
               GIMP_TYPE_OPERATION_POINT_FILTER)

#define parent_class gimp_operation_colorize_parent_class


static void
gimp_operation_colorize_class_init (GimpOperationColorizeClass *klass)
{
  GObjectClass                  *object_class    = G_OBJECT_CLASS (klass);
  GeglOperationClass            *operation_class = GEGL_OPERATION_CLASS (klass);
  GeglOperationPointFilterClass *point_class     = GEGL_OPERATION_POINT_FILTER_CLASS (klass);
  GeglColor                     *color;
  gfloat                         hsl[3]          = { 0.5f, 0.5f, 0.5f };

  object_class->set_property   = gimp_operation_colorize_set_property;
  object_class->get_property   = gimp_operation_colorize_get_property;

  operation_class->prepare     = gimp_operation_colorize_prepare;

  gegl_operation_class_set_keys (operation_class,
                                 "name",        "gimp:colorize",
                                 "categories",  "color",
                                 "description", _("Colorize the image"),
                                 NULL);

  point_class->process = gimp_operation_colorize_process;

  GIMP_CONFIG_PROP_DOUBLE (object_class, PROP_HUE,
                           "hue",
                           _("Hue"),
                           _("Hue"),
                           0.0, 1.0, 0.5, 0);

  GIMP_CONFIG_PROP_DOUBLE (object_class, PROP_SATURATION,
                           "saturation",
                           _("Saturation"),
                           _("Saturation"),
                           0.0, 1.0, 0.5, 0);

  GIMP_CONFIG_PROP_DOUBLE (object_class, PROP_LIGHTNESS,
                           "lightness",
                           _("Lightness"),
                           _("Lightness"),
                           -1.0, 1.0, 0.0, 0);

  color = gegl_color_new (NULL);
  gegl_color_set_pixel (color, babl_format ("HSL float"), hsl);

  g_object_class_install_property (object_class, PROP_COLOR,
                                   gimp_param_spec_color ("color",
                                                          _("Color"),
                                                          _("Color"),
                                                          FALSE, color,
                                                          G_PARAM_READWRITE));
  g_object_unref (color);
}

static void
gimp_operation_colorize_init (GimpOperationColorize *self)
{
  self->hue        = 0.5;
  self->saturation = 0.5;
  self->lightness  = 0.5;
  self->hsl_format = NULL;
}

static void
gimp_operation_colorize_get_property (GObject    *object,
                                      guint       property_id,
                                      GValue     *value,
                                      GParamSpec *pspec)
{
  GimpOperationColorize *self = GIMP_OPERATION_COLORIZE (object);

  switch (property_id)
    {
    case PROP_HUE:
      g_value_set_double (value, self->hue);
      break;

    case PROP_SATURATION:
      g_value_set_double (value, self->saturation);
      break;

    case PROP_LIGHTNESS:
      g_value_set_double (value, self->lightness);
      break;

    case PROP_COLOR:
      {
        GeglColor *color;
        gfloat     hsl[3];

        hsl[0] = self->hue;
        hsl[1] = self->saturation;
        hsl[2] = (self->lightness + 1.0) / 2.0;

        color = gegl_color_new (NULL);
        gegl_color_set_pixel (color, babl_format ("HSL float"), hsl);
        g_value_take_object (value, color);
      }
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_operation_colorize_set_property (GObject      *object,
                                      guint         property_id,
                                      const GValue *value,
                                      GParamSpec   *pspec)
{
  GimpOperationColorize *self = GIMP_OPERATION_COLORIZE (object);

  switch (property_id)
    {
    case PROP_HUE:
      self->hue = g_value_get_double (value);
      g_object_notify (object, "color");
      break;

    case PROP_SATURATION:
      self->saturation = g_value_get_double (value);
      g_object_notify (object, "color");
      break;

    case PROP_LIGHTNESS:
      self->lightness = g_value_get_double (value);
      g_object_notify (object, "color");
      break;

    case PROP_COLOR:
      {
        GeglColor *color;
        float      hsl[3];

        color = g_value_get_object (value);
        gegl_color_get_pixel (color, self->hsl_format, hsl);

        g_object_set (self,
                      "hue",        hsl[0],
                      "saturation", hsl[1],
                      "lightness",  hsl[2] * 2.0 - 1.0,
                      NULL);
      }
      break;

   default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_operation_colorize_prepare (GeglOperation *operation)
{
  GimpOperationColorize *colorize = GIMP_OPERATION_COLORIZE (operation);
  const Babl            *space    = gegl_operation_get_source_space (operation,
                                                                     "input");
  const Babl            *in_format;
  const Babl            *out_format;

  /* GIMP_RGB_LUMINANCE() requires the input to be linear RGB for correctness. */
  in_format  = babl_format_with_space ("RGBA float", space);
  /* Technically it looks like our code is returning non-linear RGB so we should
   * set the output format to "R'G'B'A float". I leave this like this for now as
   * it's the algorithm we used for years.
   */
  out_format = babl_format_with_space ("RGBA float", space);

  gegl_operation_set_format (operation, "input",  in_format);
  gegl_operation_set_format (operation, "output", out_format);

  colorize->hsl_format    = babl_format_with_space ("HSL float", in_format);
  colorize->fish_to_lum   = babl_fish (in_format, "Y float");
  colorize->fish_from_hsl = babl_fish (colorize->hsl_format, out_format);
}

static gboolean
gimp_operation_colorize_process (GeglOperation       *operation,
                                 void                *in_buf,
                                 void                *out_buf,
                                 glong                samples,
                                 const GeglRectangle *roi,
                                 gint                 level)
{
  GimpOperationColorize *colorize = GIMP_OPERATION_COLORIZE (operation);
  gfloat                *src      = in_buf;
  gfloat                *dest     = out_buf;
  gfloat                 hsl[3];

  hsl[0] = colorize->hue;
  hsl[1] = colorize->saturation;

  while (samples--)
    {
      gfloat lum = GIMP_RGB_LUMINANCE (src[RED],
                                       src[GREEN],
                                       src[BLUE]);

      /* TODO: the following would compute luminance correctly whatever the
       * input format but is slower:
      */
      /*babl_process (colorize->fish_to_lum, src, &lum, 1);*/

      if (colorize->lightness > 0.f)
        {
          lum = lum * (1.0f - colorize->lightness);

          lum += 1.0f - (1.0f - colorize->lightness);
        }
      else if (colorize->lightness < 0.f)
        {
          lum = lum * (colorize->lightness + 1.0f);
        }

      hsl[2] = lum;

      gimp_hsl_to_non_linear_rgb (hsl, dest);
      /* TODO: the following would convert correctly from HSL to RGB but it's a
       * lot slower. Also the result is different as of now because
       * gimp_hsl_to_rgb() computes values in non-linear whereas we set our
       * output format to be linear.
       */
      /*babl_process (colorize->fish_from_hsl, hsl, dest, 1);*/

      dest[ALPHA] = src[ALPHA];

      src  += 4;
      dest += 4;
    }

  return TRUE;
}

/* This is a copy of gimp_hsl_value() from libgimpcolor/gimpcolorspace.c */
static inline gdouble
gimp_hsl_value (gdouble n1,
                gdouble n2,
                gdouble hue)
{
  gdouble val;

  if (hue > 6.0)
    hue -= 6.0;
  else if (hue < 0.0)
    hue += 6.0;

  if (hue < 1.0)
    val = n1 + (n2 - n1) * hue;
  else if (hue < 3.0)
    val = n2;
  else if (hue < 4.0)
    val = n1 + (n2 - n1) * (4.0 - hue);
  else
    val = n1;

  return val;
}

/* This is a copy of gimp_hsl_to_rgb() from libgimpcolor/gimpcolorspace.c
 * except that instead of working on former GimpHSL and GimpRGB types, we work
 * on float arrays of size 3 directly.
 */
static void
gimp_hsl_to_non_linear_rgb (const gfloat *hsl,
                            gfloat       *rgb)
{
  g_return_if_fail (hsl != NULL);
  g_return_if_fail (rgb != NULL);

  if (hsl[1] == 0)
    {
      /*  achromatic case  */
      rgb[0] = hsl[2];
      rgb[1] = hsl[2];
      rgb[2] = hsl[2];
    }
  else
    {
      gdouble m1, m2;

      if (hsl[2] <= 0.5)
        m2 = hsl[2] * (1.0 + hsl[1]);
      else
        m2 = hsl[2] + hsl[1] - hsl[2] * hsl[1];

      m1 = 2.0 * hsl[2] - m2;

      rgb[0] = gimp_hsl_value (m1, m2, hsl[0] * 6.0 + 2.0);
      rgb[1] = gimp_hsl_value (m1, m2, hsl[0] * 6.0);
      rgb[2] = gimp_hsl_value (m1, m2, hsl[0] * 6.0 - 2.0);
    }
}
