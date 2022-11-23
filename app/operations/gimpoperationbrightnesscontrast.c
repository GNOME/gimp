/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ligmaoperationbrightnesscontrast.c
 * Copyright (C) 2012 Michael Natterer <mitch@ligma.org>
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
#include <gegl.h>
#include <gdk-pixbuf/gdk-pixbuf.h>

#include "libligmacolor/ligmacolor.h"
#include "libligmamath/ligmamath.h"

#include "operations-types.h"

#include "ligmabrightnesscontrastconfig.h"
#include "ligmaoperationbrightnesscontrast.h"

#include "ligma-intl.h"


static gboolean ligma_operation_brightness_contrast_process (GeglOperation       *operation,
                                                            void                *in_buf,
                                                            void                *out_buf,
                                                            glong                samples,
                                                            const GeglRectangle *roi,
                                                            gint                 level);


G_DEFINE_TYPE (LigmaOperationBrightnessContrast, ligma_operation_brightness_contrast,
               LIGMA_TYPE_OPERATION_POINT_FILTER)

#define parent_class ligma_operation_brightness_contrast_parent_class


static void
ligma_operation_brightness_contrast_class_init (LigmaOperationBrightnessContrastClass *klass)
{
  GObjectClass                  *object_class    = G_OBJECT_CLASS (klass);
  GeglOperationClass            *operation_class = GEGL_OPERATION_CLASS (klass);
  GeglOperationPointFilterClass *point_class     = GEGL_OPERATION_POINT_FILTER_CLASS (klass);

  object_class->set_property   = ligma_operation_point_filter_set_property;
  object_class->get_property   = ligma_operation_point_filter_get_property;

  gegl_operation_class_set_keys (operation_class,
                                 "name",        "ligma:brightness-contrast",
                                 "categories",  "color",
                                 "description", _("Adjust brightness and contrast"),
                                 NULL);

  point_class->process         = ligma_operation_brightness_contrast_process;

  g_object_class_install_property (object_class,
                                   LIGMA_OPERATION_POINT_FILTER_PROP_CONFIG,
                                   g_param_spec_object ("config",
                                                        "Config",
                                                        "The config object",
                                                        LIGMA_TYPE_BRIGHTNESS_CONTRAST_CONFIG,
                                                        G_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT));
}

static void
ligma_operation_brightness_contrast_init (LigmaOperationBrightnessContrast *self)
{
}

static inline gfloat
ligma_operation_brightness_contrast_map (gfloat  value,
                                        gdouble brightness,
                                        gdouble slant)
{
  /* apply brightness */
  if (brightness < 0.0)
    value = value * (1.0 + brightness);
  else
    value = value + ((1.0 - value) * brightness);

  value = (value - 0.5) * slant + 0.5;

  return value;
}

static gboolean
ligma_operation_brightness_contrast_process (GeglOperation       *operation,
                                            void                *in_buf,
                                            void                *out_buf,
                                            glong                samples,
                                            const GeglRectangle *roi,
                                            gint                 level)
{
  LigmaOperationPointFilter     *point  = LIGMA_OPERATION_POINT_FILTER (operation);
  LigmaBrightnessContrastConfig *config = LIGMA_BRIGHTNESS_CONTRAST_CONFIG (point->config);
  gfloat                       *src    = in_buf;
  gfloat                       *dest   = out_buf;
  gdouble                       brightness;
  gdouble                       slant;

  if (! config)
    return FALSE;

  brightness = config->brightness / 2.0;
  slant = tan ((config->contrast + 1) * G_PI_4);

  while (samples--)
    {
      dest[RED] = ligma_operation_brightness_contrast_map (src[RED],
                                                          brightness,
                                                          slant);
      dest[GREEN] = ligma_operation_brightness_contrast_map (src[GREEN],
                                                            brightness,
                                                            slant);
      dest[BLUE] = ligma_operation_brightness_contrast_map (src[BLUE],
                                                           brightness,
                                                           slant);
      dest[ALPHA] = src[ALPHA];

      src  += 4;
      dest += 4;
    }

  return TRUE;
}
