/* LIBLIGMA - The LIGMA Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * This library is free software: you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library.  If not, see
 * <https://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <babl/babl.h>
#include <glib-object.h>

#include "libligmamath/ligmamath.h"

#include "ligmacolortypes.h"
#include "ligmargb.h"


/**
 * SECTION: ligmargb
 * @title: LigmaRGB
 * @short_description: Definitions and Functions relating to RGB colors.
 *
 * Definitions and Functions relating to RGB colors.
 **/


/*
 * LIGMA_TYPE_RGB
 */

static LigmaRGB * ligma_rgb_copy (const LigmaRGB *rgb);


G_DEFINE_BOXED_TYPE (LigmaRGB, ligma_rgb, ligma_rgb_copy, g_free)

void
ligma_value_get_rgb (const GValue *value,
                    LigmaRGB      *rgb)
{
  g_return_if_fail (LIGMA_VALUE_HOLDS_RGB (value));
  g_return_if_fail (rgb != NULL);

  if (value->data[0].v_pointer)
    *rgb = *((LigmaRGB *) value->data[0].v_pointer);
  else
    ligma_rgba_set (rgb, 0.0, 0.0, 0.0, 1.0);
}

void
ligma_value_set_rgb (GValue        *value,
                    const LigmaRGB *rgb)
{
  g_return_if_fail (LIGMA_VALUE_HOLDS_RGB (value));
  g_return_if_fail (rgb != NULL);

  g_value_set_boxed (value, rgb);
}

static LigmaRGB *
ligma_rgb_copy (const LigmaRGB *rgb)
{
  return g_memdup2 (rgb, sizeof (LigmaRGB));
}


/*  RGB functions  */

/**
 * ligma_rgb_set:
 * @rgb:   a #LigmaRGB struct
 * @red:   the red component
 * @green: the green component
 * @blue:  the blue component
 *
 * Sets the red, green and blue components of @rgb and leaves the
 * alpha component unchanged. The color values should be between 0.0
 * and 1.0 but there is no check to enforce this and the values are
 * set exactly as they are passed in.
 **/
void
ligma_rgb_set (LigmaRGB *rgb,
              gdouble  r,
              gdouble  g,
              gdouble  b)
{
  g_return_if_fail (rgb != NULL);

  rgb->r = r;
  rgb->g = g;
  rgb->b = b;
}

/**
 * ligma_rgb_set_alpha:
 * @rgb:   a #LigmaRGB struct
 * @alpha: the alpha component
 *
 * Sets the alpha component of @rgb and leaves the RGB components unchanged.
 **/
void
ligma_rgb_set_alpha (LigmaRGB *rgb,
                    gdouble  a)
{
  g_return_if_fail (rgb != NULL);

  rgb->a = a;
}

/**
 * ligma_rgb_set_pixel:
 * @rgb:    a #LigmaRGB struct
 * @format: a Babl format
 * @pixel:  pointer to the source pixel
 *
 * Sets the red, green and blue components of @rgb from the color
 * stored in @pixel. The pixel format of @pixel is determined by
 * @format.
 *
 * Since: 2.10
 **/
void
ligma_rgb_set_pixel (LigmaRGB       *rgb,
                    const Babl    *format,
                    gconstpointer  pixel)
{
  g_return_if_fail (rgb != NULL);
  g_return_if_fail (format != NULL);
  g_return_if_fail (pixel != NULL);

  babl_process (babl_fish (format,
                           babl_format ("R'G'B' double")),
                pixel, rgb, 1);
}

/**
 * ligma_rgb_get_pixel:
 * @rgb:    a #LigmaRGB struct
 * @format: a Babl format
 * @pixel:  (out caller-allocates): pointer to the destination pixel
 *
 * Writes the red, green, blue and alpha components of @rgb to the
 * color stored in @pixel. The pixel format of @pixel is determined by
 * @format.
 *
 * Since: 2.10
 **/
void
ligma_rgb_get_pixel (const LigmaRGB *rgb,
                    const Babl    *format,
                    gpointer       pixel)
{
  g_return_if_fail (rgb != NULL);
  g_return_if_fail (format != NULL);
  g_return_if_fail (pixel != NULL);

  babl_process (babl_fish (babl_format ("R'G'B' double"),
                           format),
                rgb, pixel, 1);
}

/**
 * ligma_rgb_set_uchar:
 * @rgb:   a #LigmaRGB struct
 * @red:   the red component
 * @green: the green component
 * @blue:  the blue component
 *
 * Sets the red, green and blue components of @rgb from 8bit values
 * (0 to 255) and leaves the alpha component unchanged.
 **/
void
ligma_rgb_set_uchar (LigmaRGB *rgb,
                    guchar   r,
                    guchar   g,
                    guchar   b)
{
  g_return_if_fail (rgb != NULL);

  rgb->r = (gdouble) r / 255.0;
  rgb->g = (gdouble) g / 255.0;
  rgb->b = (gdouble) b / 255.0;
}

/**
 * ligma_rgb_get_uchar:
 * @rgb: a #LigmaRGB struct
 * @red: (out) (optional): Location for red component, or %NULL
 * @green: (out) (optional): Location for green component, or %NULL
 * @blue: (out) (optional): Location for blue component, or %NULL
 *
 * Writes the red, green, blue and alpha components of @rgb to the
 * color components @red, @green and @blue.
 */
void
ligma_rgb_get_uchar (const LigmaRGB *rgb,
                    guchar        *red,
                    guchar        *green,
                    guchar        *blue)
{
  g_return_if_fail (rgb != NULL);

  if (red)   *red   = ROUND (CLAMP (rgb->r, 0.0, 1.0) * 255.0);
  if (green) *green = ROUND (CLAMP (rgb->g, 0.0, 1.0) * 255.0);
  if (blue)  *blue  = ROUND (CLAMP (rgb->b, 0.0, 1.0) * 255.0);
}

void
ligma_rgb_add (LigmaRGB       *rgb1,
              const LigmaRGB *rgb2)
{
  g_return_if_fail (rgb1 != NULL);
  g_return_if_fail (rgb2 != NULL);

  rgb1->r += rgb2->r;
  rgb1->g += rgb2->g;
  rgb1->b += rgb2->b;
}

void
ligma_rgb_subtract (LigmaRGB       *rgb1,
                   const LigmaRGB *rgb2)
{
  g_return_if_fail (rgb1 != NULL);
  g_return_if_fail (rgb2 != NULL);

  rgb1->r -= rgb2->r;
  rgb1->g -= rgb2->g;
  rgb1->b -= rgb2->b;
}

void
ligma_rgb_multiply (LigmaRGB *rgb,
                   gdouble  factor)
{
  g_return_if_fail (rgb != NULL);

  rgb->r *= factor;
  rgb->g *= factor;
  rgb->b *= factor;
}

gdouble
ligma_rgb_distance (const LigmaRGB *rgb1,
                   const LigmaRGB *rgb2)
{
  g_return_val_if_fail (rgb1 != NULL, 0.0);
  g_return_val_if_fail (rgb2 != NULL, 0.0);

  return (fabs (rgb1->r - rgb2->r) +
          fabs (rgb1->g - rgb2->g) +
          fabs (rgb1->b - rgb2->b));
}

gdouble
ligma_rgb_max (const LigmaRGB *rgb)
{
  g_return_val_if_fail (rgb != NULL, 0.0);

  if (rgb->r > rgb->g)
    return (rgb->r > rgb->b) ? rgb->r : rgb->b;
  else
    return (rgb->g > rgb->b) ? rgb->g : rgb->b;
}

gdouble
ligma_rgb_min (const LigmaRGB *rgb)
{
  g_return_val_if_fail (rgb != NULL, 0.0);

  if (rgb->r < rgb->g)
    return (rgb->r < rgb->b) ? rgb->r : rgb->b;
  else
    return (rgb->g < rgb->b) ? rgb->g : rgb->b;
}

void
ligma_rgb_clamp (LigmaRGB *rgb)
{
  g_return_if_fail (rgb != NULL);

  rgb->r = CLAMP (rgb->r, 0.0, 1.0);
  rgb->g = CLAMP (rgb->g, 0.0, 1.0);
  rgb->b = CLAMP (rgb->b, 0.0, 1.0);
  rgb->a = CLAMP (rgb->a, 0.0, 1.0);
}

void
ligma_rgb_gamma (LigmaRGB *rgb,
                gdouble  gamma)
{
  gdouble ig;

  g_return_if_fail (rgb != NULL);

  if (gamma != 0.0)
    ig = 1.0 / gamma;
  else
    ig = 0.0;

  rgb->r = pow (rgb->r, ig);
  rgb->g = pow (rgb->g, ig);
  rgb->b = pow (rgb->b, ig);
}

/**
 * ligma_rgb_luminance:
 * @rgb: a #LigmaRGB struct
 *
 * Returns: the luminous intensity of the range from 0.0 to 1.0.
 *
 * Since: 2.4
 **/
gdouble
ligma_rgb_luminance (const LigmaRGB *rgb)
{
  gdouble luminance;

  g_return_val_if_fail (rgb != NULL, 0.0);

  luminance = LIGMA_RGB_LUMINANCE (rgb->r, rgb->g, rgb->b);

  return CLAMP (luminance, 0.0, 1.0);
}

/**
 * ligma_rgb_luminance_uchar:
 * @rgb: a #LigmaRGB struct
 *
 * Returns: the luminous intensity in the range from 0 to 255.
 *
 * Since: 2.4
 **/
guchar
ligma_rgb_luminance_uchar (const LigmaRGB *rgb)
{
  g_return_val_if_fail (rgb != NULL, 0);

  return ROUND (ligma_rgb_luminance (rgb) * 255.0);
}

void
ligma_rgb_composite (LigmaRGB              *color1,
                    const LigmaRGB        *color2,
                    LigmaRGBCompositeMode  mode)
{
  g_return_if_fail (color1 != NULL);
  g_return_if_fail (color2 != NULL);

  switch (mode)
    {
    case LIGMA_RGB_COMPOSITE_NONE:
      break;

    case LIGMA_RGB_COMPOSITE_NORMAL:
      /*  put color2 on top of color1  */
      if (color2->a == 1.0)
        {
          *color1 = *color2;
        }
      else
        {
          gdouble factor = color1->a * (1.0 - color2->a);

          color1->r = color1->r * factor + color2->r * color2->a;
          color1->g = color1->g * factor + color2->g * color2->a;
          color1->b = color1->b * factor + color2->b * color2->a;
          color1->a = factor + color2->a;
        }
      break;

    case LIGMA_RGB_COMPOSITE_BEHIND:
      /*  put color2 below color1  */
      if (color1->a < 1.0)
        {
          gdouble factor = color2->a * (1.0 - color1->a);

          color1->r = color2->r * factor + color1->r * color1->a;
          color1->g = color2->g * factor + color1->g * color1->a;
          color1->b = color2->b * factor + color1->b * color1->a;
          color1->a = factor + color1->a;
        }
      break;
    }
}

/*  RGBA functions  */

/**
 * ligma_rgba_set_pixel:
 * @rgba:   a #LigmaRGB struct
 * @format: a Babl format
 * @pixel:  pointer to the source pixel
 *
 * Sets the red, green, blue and alpha components of @rgba from the
 * color stored in @pixel. The pixel format of @pixel is determined
 * by @format.
 *
 * Since: 2.10
 **/
void
ligma_rgba_set_pixel (LigmaRGB       *rgba,
                     const Babl    *format,
                     gconstpointer  pixel)
{
  g_return_if_fail (rgba != NULL);
  g_return_if_fail (format != NULL);
  g_return_if_fail (pixel != NULL);

  babl_process (babl_fish (format,
                           babl_format ("R'G'B'A double")),
                pixel, rgba, 1);
}

/**
 * ligma_rgba_get_pixel:
 * @rgba:   a #LigmaRGB struct
 * @format: a Babl format
 * @pixel:  (out caller-allocates): pointer to the destination pixel
 *
 * Writes the red, green, blue and alpha components of @rgba to the
 * color stored in @pixel. The pixel format of @pixel is determined by
 * @format.
 *
 * Since: 2.10
 **/
void
ligma_rgba_get_pixel (const LigmaRGB *rgba,
                     const Babl    *format,
                     gpointer       pixel)
{
  g_return_if_fail (rgba != NULL);
  g_return_if_fail (format != NULL);
  g_return_if_fail (pixel != NULL);

  babl_process (babl_fish (babl_format ("R'G'B'A double"),
                           format),
                rgba, pixel, 1);
}

/**
 * ligma_rgba_set:
 * @rgba:  a #LigmaRGB struct
 * @red:   the red component
 * @green: the green component
 * @blue:  the blue component
 * @alpha: the alpha component
 *
 * Sets the red, green, blue and alpha components of @rgb. The values
 * should be between 0.0 and 1.0 but there is no check to enforce this
 * and the values are set exactly as they are passed in.
 **/
void
ligma_rgba_set (LigmaRGB *rgba,
               gdouble  r,
               gdouble  g,
               gdouble  b,
               gdouble  a)
{
  g_return_if_fail (rgba != NULL);

  rgba->r = r;
  rgba->g = g;
  rgba->b = b;
  rgba->a = a;
}

/**
 * ligma_rgba_set_uchar:
 * @rgba:  a #LigmaRGB struct
 * @red:   the red component
 * @green: the green component
 * @blue:  the blue component
 * @alpha: the alpha component
 *
 * Sets the red, green, blue and alpha components of @rgba from 8bit
 * values (0 to 255).
 **/
void
ligma_rgba_set_uchar (LigmaRGB *rgba,
                     guchar   r,
                     guchar   g,
                     guchar   b,
                     guchar   a)
{
  g_return_if_fail (rgba != NULL);

  rgba->r = (gdouble) r / 255.0;
  rgba->g = (gdouble) g / 255.0;
  rgba->b = (gdouble) b / 255.0;
  rgba->a = (gdouble) a / 255.0;
}

/**
 * ligma_rgba_get_uchar:
 * @rgba:  a #LigmaRGB struct
 * @red: (out) (optional): Location for the red component
 * @green: (out) (optional): Location for the green component
 * @blue: (out) (optional): Location for the blue component
 * @alpha: (out) (optional): Location for the alpha component
 *
 * Gets the 8bit red, green, blue and alpha components of @rgba.
 **/
void
ligma_rgba_get_uchar (const LigmaRGB *rgba,
                     guchar        *r,
                     guchar        *g,
                     guchar        *b,
                     guchar        *a)
{
  g_return_if_fail (rgba != NULL);

  if (r) *r = ROUND (CLAMP (rgba->r, 0.0, 1.0) * 255.0);
  if (g) *g = ROUND (CLAMP (rgba->g, 0.0, 1.0) * 255.0);
  if (b) *b = ROUND (CLAMP (rgba->b, 0.0, 1.0) * 255.0);
  if (a) *a = ROUND (CLAMP (rgba->a, 0.0, 1.0) * 255.0);
}

void
ligma_rgba_add (LigmaRGB       *rgba1,
               const LigmaRGB *rgba2)
{
  g_return_if_fail (rgba1 != NULL);
  g_return_if_fail (rgba2 != NULL);

  rgba1->r += rgba2->r;
  rgba1->g += rgba2->g;
  rgba1->b += rgba2->b;
  rgba1->a += rgba2->a;
}

void
ligma_rgba_subtract (LigmaRGB       *rgba1,
                    const LigmaRGB *rgba2)
{
  g_return_if_fail (rgba1 != NULL);
  g_return_if_fail (rgba2 != NULL);

  rgba1->r -= rgba2->r;
  rgba1->g -= rgba2->g;
  rgba1->b -= rgba2->b;
  rgba1->a -= rgba2->a;
}

void
ligma_rgba_multiply (LigmaRGB *rgba,
                    gdouble  factor)
{
  g_return_if_fail (rgba != NULL);

  rgba->r *= factor;
  rgba->g *= factor;
  rgba->b *= factor;
  rgba->a *= factor;
}

gdouble
ligma_rgba_distance (const LigmaRGB *rgba1,
                    const LigmaRGB *rgba2)
{
  g_return_val_if_fail (rgba1 != NULL, 0.0);
  g_return_val_if_fail (rgba2 != NULL, 0.0);

  return (fabs (rgba1->r - rgba2->r) +
          fabs (rgba1->g - rgba2->g) +
          fabs (rgba1->b - rgba2->b) +
          fabs (rgba1->a - rgba2->a));
}


/*
 * LIGMA_TYPE_PARAM_RGB
 */

#define LIGMA_PARAM_SPEC_RGB(pspec)    (G_TYPE_CHECK_INSTANCE_CAST ((pspec), LIGMA_TYPE_PARAM_RGB, LigmaParamSpecRGB))

struct _LigmaParamSpecRGB
{
  GParamSpecBoxed  parent_instance;

  gboolean         has_alpha;
  gboolean         validate; /* change this to enable [0.0...1.0] */
  LigmaRGB          default_value;
};

static void       ligma_param_rgb_class_init  (GParamSpecClass *class);
static void       ligma_param_rgb_init        (GParamSpec      *pspec);
static void       ligma_param_rgb_set_default (GParamSpec      *pspec,
                                              GValue          *value);
static gboolean   ligma_param_rgb_validate    (GParamSpec      *pspec,
                                              GValue          *value);
static gint       ligma_param_rgb_values_cmp  (GParamSpec      *pspec,
                                              const GValue    *value1,
                                              const GValue    *value2);

/**
 * ligma_param_rgb_get_type:
 *
 * Reveals the object type
 *
 * Returns: the #GType for a LigmaParamRGB object
 *
 * Since: 2.4
 **/
GType
ligma_param_rgb_get_type (void)
{
  static GType spec_type = 0;

  if (! spec_type)
    {
      const GTypeInfo type_info =
      {
        sizeof (GParamSpecClass),
        NULL, NULL,
        (GClassInitFunc) ligma_param_rgb_class_init,
        NULL, NULL,
        sizeof (LigmaParamSpecRGB),
        0,
        (GInstanceInitFunc) ligma_param_rgb_init
      };

      spec_type = g_type_register_static (G_TYPE_PARAM_BOXED,
                                          "LigmaParamRGB",
                                          &type_info, 0);
    }

  return spec_type;
}

static void
ligma_param_rgb_class_init (GParamSpecClass *class)
{
  class->value_type        = LIGMA_TYPE_RGB;
  class->value_set_default = ligma_param_rgb_set_default;
  class->value_validate    = ligma_param_rgb_validate;
  class->values_cmp        = ligma_param_rgb_values_cmp;
}

static void
ligma_param_rgb_init (GParamSpec *pspec)
{
  LigmaParamSpecRGB *cspec = LIGMA_PARAM_SPEC_RGB (pspec);

  ligma_rgba_set (&cspec->default_value, 0.0, 0.0, 0.0, 1.0);
}

static void
ligma_param_rgb_set_default (GParamSpec *pspec,
                            GValue     *value)
{
  LigmaParamSpecRGB *cspec = LIGMA_PARAM_SPEC_RGB (pspec);

  g_value_set_static_boxed (value, &cspec->default_value);
}

static gboolean
ligma_param_rgb_validate (GParamSpec *pspec,
                         GValue     *value)
{
  LigmaParamSpecRGB *rgb_spec = LIGMA_PARAM_SPEC_RGB (pspec);
  LigmaRGB          *rgb      = value->data[0].v_pointer;

  if (rgb_spec->validate && rgb)
    {
      LigmaRGB oval = *rgb;

      ligma_rgb_clamp (rgb);

      return (oval.r != rgb->r ||
              oval.g != rgb->g ||
              oval.b != rgb->b ||
              (rgb_spec->has_alpha && oval.a != rgb->a));
    }

  return FALSE;
}

static gint
ligma_param_rgb_values_cmp (GParamSpec   *pspec,
                           const GValue *value1,
                           const GValue *value2)
{
  LigmaRGB *rgb1 = value1->data[0].v_pointer;
  LigmaRGB *rgb2 = value2->data[0].v_pointer;

  /*  try to return at least *something*, it's useless anyway...  */

  if (! rgb1)
    {
      return rgb2 != NULL ? -1 : 0;
    }
  else if (! rgb2)
    {
      return rgb1 != NULL;
    }
  else
    {
      guint32 int1 = 0;
      guint32 int2 = 0;

      if (LIGMA_PARAM_SPEC_RGB (pspec)->has_alpha)
        {
          ligma_rgba_get_uchar (rgb1,
                               ((guchar *) &int1) + 0,
                               ((guchar *) &int1) + 1,
                               ((guchar *) &int1) + 2,
                               ((guchar *) &int1) + 3);
          ligma_rgba_get_uchar (rgb2,
                               ((guchar *) &int2) + 0,
                               ((guchar *) &int2) + 1,
                               ((guchar *) &int2) + 2,
                               ((guchar *) &int2) + 3);
        }
      else
        {
          ligma_rgb_get_uchar (rgb1,
                              ((guchar *) &int1) + 0,
                              ((guchar *) &int1) + 1,
                              ((guchar *) &int1) + 2);
          ligma_rgb_get_uchar (rgb2,
                              ((guchar *) &int2) + 0,
                              ((guchar *) &int2) + 1,
                              ((guchar *) &int2) + 2);
        }

      return int1 - int2;
    }
}

/**
 * ligma_param_spec_rgb:
 * @name:          Canonical name of the param
 * @nick:          Nickname of the param
 * @blurb:         Brief description of param.
 * @has_alpha:     %TRUE if the alpha channel has relevance.
 * @default_value: Value to use if none is assigned.
 * @flags:         a combination of #GParamFlags
 *
 * Creates a param spec to hold an #LigmaRGB value.
 * See g_param_spec_internal() for more information.
 *
 * Returns: (transfer full): a newly allocated #GParamSpec instance
 *
 * Since: 2.4
 **/
GParamSpec *
ligma_param_spec_rgb (const gchar   *name,
                     const gchar   *nick,
                     const gchar   *blurb,
                     gboolean       has_alpha,
                     const LigmaRGB *default_value,
                     GParamFlags    flags)
{
  LigmaParamSpecRGB *cspec;

  cspec = g_param_spec_internal (LIGMA_TYPE_PARAM_RGB,
                                 name, nick, blurb, flags);

  cspec->has_alpha = has_alpha;

  if (default_value)
    cspec->default_value = *default_value;
  else
    ligma_rgba_set (&cspec->default_value, 0.0, 0.0, 0.0, 1.0);

  return G_PARAM_SPEC (cspec);
}

/**
 * ligma_param_spec_rgb_get_default:
 * @pspec:         a #LigmaParamSpecRGB.
 * @default_value: return location for @pspec's default value
 *
 * Returns the @pspec's default color value.
 *
 * Since: 2.10.14
 **/
void
ligma_param_spec_rgb_get_default (GParamSpec *pspec,
                                 LigmaRGB    *default_value)
{
  g_return_if_fail (LIGMA_IS_PARAM_SPEC_RGB (pspec));
  g_return_if_fail (default_value != NULL);

  *default_value = LIGMA_PARAM_SPEC_RGB (pspec)->default_value;
}

/**
 * ligma_param_spec_rgb_has_alpha:
 * @pspec: a #GParamSpec to hold an #LigmaRGB value.
 *
 * Returns: %TRUE if the alpha channel is relevant.
 *
 * Since: 2.4
 **/
gboolean
ligma_param_spec_rgb_has_alpha (GParamSpec *pspec)
{
  g_return_val_if_fail (LIGMA_IS_PARAM_SPEC_RGB (pspec), FALSE);

  return LIGMA_PARAM_SPEC_RGB (pspec)->has_alpha;
}
