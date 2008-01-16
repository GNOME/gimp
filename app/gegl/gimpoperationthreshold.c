/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpoperationthreshold.c
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

#include "gimpoperationthreshold.h"


enum
{
  PROP_0,
  PROP_LOW,
  PROP_HIGH
};


static void     gimp_operation_threshold_get_property (GObject       *object,
                                                       guint          property_id,
                                                       GValue        *value,
                                                       GParamSpec    *pspec);
static void     gimp_operation_threshold_set_property (GObject       *object,
                                                       guint          property_id,
                                                       const GValue  *value,
                                                       GParamSpec    *pspec);

static gboolean gimp_operation_threshold_process      (GeglOperation *operation,
                                                       void          *in_buf,
                                                       void          *out_buf,
                                                       glong          samples);


G_DEFINE_TYPE (GimpOperationThreshold, gimp_operation_threshold,
               GEGL_TYPE_OPERATION_POINT_FILTER)

#define parent_class gimp_operation_threshold_parent_class


static void
gimp_operation_threshold_class_init (GimpOperationThresholdClass * klass)
{
  GObjectClass                  *object_class    = G_OBJECT_CLASS (klass);
  GeglOperationClass            *operation_class = GEGL_OPERATION_CLASS (klass);
  GeglOperationPointFilterClass *point_class     = GEGL_OPERATION_POINT_FILTER_CLASS (klass);

  object_class->set_property = gimp_operation_threshold_set_property;
  object_class->get_property = gimp_operation_threshold_get_property;

  point_class->process       = gimp_operation_threshold_process;

  gegl_operation_class_set_name (operation_class, "gimp-threshold");

  g_object_class_install_property (object_class, PROP_LOW,
                                   g_param_spec_double ("low",
                                                        "Low",
                                                        "Low threshold",
                                                        0.0, 1.0, 0.5,
                                                        G_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT));

  g_object_class_install_property (object_class, PROP_HIGH,
                                   g_param_spec_double ("high",
                                                        "High",
                                                        "High threshold",
                                                        0.0, 1.0, 1.0,
                                                        G_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT));
}

static void
gimp_operation_threshold_init (GimpOperationThreshold *self)
{
}

static void
gimp_operation_threshold_get_property (GObject    *object,
                                       guint       property_id,
                                       GValue     *value,
                                       GParamSpec *pspec)
{
  GimpOperationThreshold *self = GIMP_OPERATION_THRESHOLD (object);

  switch (property_id)
    {
    case PROP_LOW:
      g_value_set_double (value, self->low);
      break;

    case PROP_HIGH:
      g_value_set_double (value, self->high);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_operation_threshold_set_property (GObject      *object,
                                       guint         property_id,
                                       const GValue *value,
                                       GParamSpec   *pspec)
{
  GimpOperationThreshold *self = GIMP_OPERATION_THRESHOLD (object);

  switch (property_id)
    {
    case PROP_LOW:
      self->low = g_value_get_double (value);
      break;

    case PROP_HIGH:
      self->high = g_value_get_double (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static gboolean
gimp_operation_threshold_process (GeglOperation *operation,
                                  void          *in_buf,
                                  void          *out_buf,
                                  glong          samples)
{
  GimpOperationThreshold *self = GIMP_OPERATION_THRESHOLD (operation);
  gfloat                 *src  = in_buf;
  gfloat                 *dest = out_buf;
  glong                   sample;

  for (sample = 0; sample < samples; sample++)
    {
      gfloat value;

      value = MAX (src[RED_PIX], src[GREEN_PIX]);
      value = MAX (value, src[BLUE_PIX]);

      value = (value >= self->low && value <= self->high) ? 1.0 : 0.0;

      dest[RED_PIX]   = value;
      dest[GREEN_PIX] = value;
      dest[BLUE_PIX]  = value;
      dest[ALPHA_PIX] = src[ALPHA_PIX];

      src  += 4;
      dest += 4;
    }

  return TRUE;
}
