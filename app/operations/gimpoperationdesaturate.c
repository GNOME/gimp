/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ligmaoperationdesaturate.c
 * Copyright (C) 2007 Michael Natterer <mitch@ligma.org>
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

#include "libligmabase/ligmabase.h"
#include "libligmacolor/ligmacolor.h"
#include "libligmaconfig/ligmaconfig.h"

#include "operations-types.h"

#include "ligmaoperationdesaturate.h"

#include "ligma-intl.h"


enum
{
  PROP_0,
  PROP_MODE
};


static void     ligma_operation_desaturate_get_property (GObject             *object,
                                                        guint                property_id,
                                                        GValue              *value,
                                                        GParamSpec          *pspec);
static void     ligma_operation_desaturate_set_property (GObject             *object,
                                                        guint                property_id,
                                                        const GValue        *value,
                                                        GParamSpec          *pspec);

static void     ligma_operation_desaturate_prepare      (GeglOperation       *operation);
static gboolean ligma_operation_desaturate_process      (GeglOperation       *operation,
                                                        void                *in_buf,
                                                        void                *out_buf,
                                                        glong                samples,
                                                        const GeglRectangle *roi,
                                                        gint                 level);


G_DEFINE_TYPE (LigmaOperationDesaturate, ligma_operation_desaturate,
               LIGMA_TYPE_OPERATION_POINT_FILTER)

#define parent_class ligma_operation_desaturate_parent_class


static void
ligma_operation_desaturate_class_init (LigmaOperationDesaturateClass *klass)
{
  GObjectClass                  *object_class    = G_OBJECT_CLASS (klass);
  GeglOperationClass            *operation_class = GEGL_OPERATION_CLASS (klass);
  GeglOperationPointFilterClass *point_class     = GEGL_OPERATION_POINT_FILTER_CLASS (klass);

  object_class->set_property = ligma_operation_desaturate_set_property;
  object_class->get_property = ligma_operation_desaturate_get_property;

  operation_class->prepare   = ligma_operation_desaturate_prepare;

  point_class->process       = ligma_operation_desaturate_process;

  gegl_operation_class_set_keys (operation_class,
                                 "name",        "ligma:desaturate",
                                 "categories",  "color",
                                 "description", _("Turn colors into shades of gray"),
                                 NULL);

  LIGMA_CONFIG_PROP_ENUM (object_class, PROP_MODE,
                         "mode",
                         _("Mode"),
                         _("Choose shade of gray based on"),
                         LIGMA_TYPE_DESATURATE_MODE,
                         LIGMA_DESATURATE_LUMINANCE,
                         LIGMA_PARAM_STATIC_STRINGS);
}

static void
ligma_operation_desaturate_init (LigmaOperationDesaturate *self)
{
}

static void
ligma_operation_desaturate_get_property (GObject    *object,
                                        guint       property_id,
                                        GValue     *value,
                                        GParamSpec *pspec)
{
  LigmaOperationDesaturate *desaturate = LIGMA_OPERATION_DESATURATE (object);

  switch (property_id)
    {
    case PROP_MODE:
      g_value_set_enum (value, desaturate->mode);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
ligma_operation_desaturate_set_property (GObject      *object,
                                        guint         property_id,
                                        const GValue *value,
                                        GParamSpec   *pspec)
{
  LigmaOperationDesaturate *desaturate = LIGMA_OPERATION_DESATURATE (object);

  switch (property_id)
    {
    case PROP_MODE:
      desaturate->mode = g_value_get_enum (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
ligma_operation_desaturate_prepare (GeglOperation *operation)
{
  LigmaOperationDesaturate *desaturate = LIGMA_OPERATION_DESATURATE (operation);
  const Babl              *format = gegl_operation_get_source_format (operation, "input");

  if (desaturate->mode == LIGMA_DESATURATE_LUMINANCE)
    {
      format = babl_format_with_space ("RGBA float", format);
    }
  else
    {
      format = babl_format_with_space ("R'G'B'A float", format);
    }

  gegl_operation_set_format (operation, "input",  format);
  gegl_operation_set_format (operation, "output", format);
}

static gboolean
ligma_operation_desaturate_process (GeglOperation       *operation,
                                   void                *in_buf,
                                   void                *out_buf,
                                   glong                samples,
                                   const GeglRectangle *roi,
                                   gint                 level)
{
  LigmaOperationDesaturate *desaturate = LIGMA_OPERATION_DESATURATE (operation);
  gfloat                  *src        = in_buf;
  gfloat                  *dest       = out_buf;

  switch (desaturate->mode)
    {
    case LIGMA_DESATURATE_LIGHTNESS:
      /* This is the formula for Lightness in the HSL "bi-hexcone"
       * model: https://en.wikipedia.org/wiki/HSL_and_HSV
       */
      while (samples--)
        {
          gfloat min, max, value;

          max = MAX (src[0], src[1]);
          max = MAX (max, src[2]);
          min = MIN (src[0], src[1]);
          min = MIN (min, src[2]);

          value = (max + min) / 2;

          dest[0] = value;
          dest[1] = value;
          dest[2] = value;
          dest[3] = src[3];

          src  += 4;
          dest += 4;
        }
      break;

    case LIGMA_DESATURATE_LUMA:
    case LIGMA_DESATURATE_LUMINANCE:
      {
        const Babl *space = gegl_operation_get_source_space (operation, "input");
        double red_luminance, green_luminance, blue_luminance;
        babl_space_get_rgb_luminance (space, &red_luminance, &green_luminance, &blue_luminance);
        while (samples--)
          {
            gfloat value  = (src[0] * red_luminance)   +
                            (src[1] * green_luminance) +
                            (src[2] * blue_luminance);
            dest[0] = value;
            dest[1] = value;
            dest[2] = value;
            dest[3] = src[3];

            src  += 4;
            dest += 4;
          }
      }
      break;

    case LIGMA_DESATURATE_AVERAGE:
      /* This is the formula for Intensity in the HSI model:
       * https://en.wikipedia.org/wiki/HSL_and_HSV
       */
      while (samples--)
        {
          gfloat value = (src[0] + src[1] + src[2]) / 3;

          dest[0] = value;
          dest[1] = value;
          dest[2] = value;
          dest[3] = src[3];

          src  += 4;
          dest += 4;
        }
      break;

    case LIGMA_DESATURATE_VALUE:
      /* This is the formula for Value in the HSV model:
       * https://en.wikipedia.org/wiki/HSL_and_HSV
       */
      while (samples--)
        {
          gfloat value;

          value = MAX (src[0], src[1]);
          value = MAX (value, src[2]);

          dest[0] = value;
          dest[1] = value;
          dest[2] = value;
          dest[3] = src[3];

          src  += 4;
          dest += 4;
        }
      break;
    }

  return TRUE;
}
