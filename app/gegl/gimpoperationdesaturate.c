/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpoperationdesaturate.c
 * Copyright (C) 2007 Michael Natterer <mitch@gimp.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include "config.h"

#include <gegl.h>

#include "libgimpcolor/gimpcolor.h"

#include "gegl-types.h"

#include "gimpoperationdesaturate.h"


enum
{
  PROP_0,
  PROP_MODE
};


static void     gimp_operation_desaturate_get_property (GObject       *object,
                                                        guint          property_id,
                                                        GValue        *value,
                                                        GParamSpec    *pspec);
static void     gimp_operation_desaturate_set_property (GObject       *object,
                                                        guint          property_id,
                                                        const GValue  *value,
                                                        GParamSpec    *pspec);

static gboolean gimp_operation_desaturate_process      (GeglOperation *operation,
                                                        void          *in_buf,
                                                        void          *out_buf,
                                                        glong          samples);


G_DEFINE_TYPE (GimpOperationDesaturate, gimp_operation_desaturate,
               GEGL_TYPE_OPERATION_POINT_FILTER)

#define parent_class gimp_operation_desaturate_parent_class


static void
gimp_operation_desaturate_class_init (GimpOperationDesaturateClass *klass)
{
  GObjectClass                  *object_class    = G_OBJECT_CLASS (klass);
  GeglOperationClass            *operation_class = GEGL_OPERATION_CLASS (klass);
  GeglOperationPointFilterClass *point_class     = GEGL_OPERATION_POINT_FILTER_CLASS (klass);

  object_class->set_property = gimp_operation_desaturate_set_property;
  object_class->get_property = gimp_operation_desaturate_get_property;

  point_class->process       = gimp_operation_desaturate_process;

  gegl_operation_class_set_name (operation_class, "gimp-desaturate");

  g_object_class_install_property (object_class,
                                   PROP_MODE,
                                   g_param_spec_enum ("mode",
                                                      "Mode",
                                                      "The desaturate mode",
                                                      GIMP_TYPE_DESATURATE_MODE,
                                                      GIMP_DESATURATE_LIGHTNESS,
                                                      G_PARAM_READWRITE |
                                                      G_PARAM_CONSTRUCT));
}

static void
gimp_operation_desaturate_init (GimpOperationDesaturate *self)
{
}

static void
gimp_operation_desaturate_get_property (GObject    *object,
                                        guint       property_id,
                                        GValue     *value,
                                        GParamSpec *pspec)
{
  GimpOperationDesaturate *self = GIMP_OPERATION_DESATURATE (object);

  switch (property_id)
    {
    case PROP_MODE:
      g_value_set_enum (value, self->mode);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_operation_desaturate_set_property (GObject      *object,
                                        guint         property_id,
                                        const GValue *value,
                                        GParamSpec   *pspec)
{
  GimpOperationDesaturate *self = GIMP_OPERATION_DESATURATE (object);

  switch (property_id)
    {
    case PROP_MODE:
      self->mode = g_value_get_enum (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static gboolean
gimp_operation_desaturate_process (GeglOperation *operation,
                                   void          *in_buf,
                                   void          *out_buf,
                                   glong          samples)
{
  GimpOperationDesaturate *self = GIMP_OPERATION_DESATURATE (operation);
  gfloat                  *src  = in_buf;
  gfloat                  *dest = out_buf;

  while (samples--)
    {
      gfloat value = 0.0;

      switch (self->mode)
        {
        case GIMP_DESATURATE_LIGHTNESS:
          {
            gfloat min, max;

#ifdef __GNUC__
#warning FIXME: cant use FOO_PIX but have no constants from babl???
#endif

            max = MAX (src[RED_PIX], src[GREEN_PIX]);
            max = MAX (max, src[BLUE_PIX]);
            min = MIN (src[RED_PIX], src[GREEN_PIX]);
            min = MIN (min, src[BLUE_PIX]);

            value = (max + min) / 2;
          }
          break;

        case GIMP_DESATURATE_LUMINOSITY:
          value = GIMP_RGB_LUMINANCE (src[RED_PIX],
                                      src[GREEN_PIX],
                                      src[BLUE_PIX]);
          break;

        case GIMP_DESATURATE_AVERAGE:
          value = (src[RED_PIX] + src[GREEN_PIX] + src[BLUE_PIX]) / 3;
          break;
        }

      dest[RED_PIX]   = value;
      dest[GREEN_PIX] = value;
      dest[BLUE_PIX]  = value;
      dest[ALPHA_PIX] = src[ALPHA_PIX];

      src  += 4;
      dest += 4;
    }

  return TRUE;
}
