/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpoperationscalarmultiply.c
 * Copyright (C) 2014 Michael Natterer <mitch@gimp.org>
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

#include <gegl.h>

#include "operations-types.h"

#include "gimpoperationscalarmultiply.h"


enum
{
  PROP_0,
  PROP_N_COMPONENTS,
  PROP_FACTOR
};


static void       gimp_operation_scalar_multiply_get_property (GObject             *object,
                                                               guint                property_id,
                                                               GValue              *value,
                                                               GParamSpec          *pspec);
static void       gimp_operation_scalar_multiply_set_property (GObject             *object,
                                                               guint                property_id,
                                                               const GValue        *value,
                                                               GParamSpec          *pspec);

static void       gimp_operation_scalar_multiply_prepare      (GeglOperation       *operation);
static gboolean   gimp_operation_scalar_multiply_process      (GeglOperation       *operation,
                                                               void                *in_buf,
                                                               void                *out_buf,
                                                               glong                samples,
                                                               const GeglRectangle *roi,
                                                               gint                 level);


G_DEFINE_TYPE (GimpOperationScalarMultiply, gimp_operation_scalar_multiply,
               GEGL_TYPE_OPERATION_POINT_FILTER)

#define parent_class gimp_operation_scalar_multiply_parent_class


static void
gimp_operation_scalar_multiply_class_init (GimpOperationScalarMultiplyClass *klass)
{
  GObjectClass                  *object_class    = G_OBJECT_CLASS (klass);
  GeglOperationClass            *operation_class = GEGL_OPERATION_CLASS (klass);
  GeglOperationPointFilterClass *point_class     = GEGL_OPERATION_POINT_FILTER_CLASS (klass);

  object_class->set_property = gimp_operation_scalar_multiply_set_property;
  object_class->get_property = gimp_operation_scalar_multiply_get_property;

  gegl_operation_class_set_keys (operation_class,
                                 "name",        "gimp:scalar-multiply",
                                 "categories",  "gimp",
                                 "description", "Multiply all floats in a buffer by a factor",
                                 NULL);

  operation_class->prepare = gimp_operation_scalar_multiply_prepare;

  point_class->process     = gimp_operation_scalar_multiply_process;

  g_object_class_install_property (object_class, PROP_N_COMPONENTS,
                                   g_param_spec_int ("n-components",
                                                     "N Components",
                                                     "Number of components in the input/output vectors",
                                                     1, 16, 2,
                                                     G_PARAM_READWRITE |
                                                     G_PARAM_CONSTRUCT));

  g_object_class_install_property (object_class, PROP_FACTOR,
                                   g_param_spec_double ("factor",
                                                        "Factor",
                                                        "The scalar factor",
                                                        G_MINFLOAT, G_MAXFLOAT,
                                                        1.0,
                                                        G_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT));
}

static void
gimp_operation_scalar_multiply_init (GimpOperationScalarMultiply *self)
{
}

static void
gimp_operation_scalar_multiply_get_property (GObject    *object,
                                             guint       property_id,
                                             GValue     *value,
                                             GParamSpec *pspec)
{
  GimpOperationScalarMultiply *self = GIMP_OPERATION_SCALAR_MULTIPLY (object);

  switch (property_id)
    {
    case PROP_N_COMPONENTS:
      g_value_set_int (value, self->n_components);
      break;

    case PROP_FACTOR:
      g_value_set_double (value, self->factor);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_operation_scalar_multiply_set_property (GObject      *object,
                                             guint         property_id,
                                             const GValue *value,
                                             GParamSpec   *pspec)
{
  GimpOperationScalarMultiply *self = GIMP_OPERATION_SCALAR_MULTIPLY (object);

  switch (property_id)
    {
    case PROP_N_COMPONENTS:
      self->n_components = g_value_get_int (value);
      break;

    case PROP_FACTOR:
      self->factor = g_value_get_double (value);
      break;

   default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_operation_scalar_multiply_prepare (GeglOperation *operation)
{
  GimpOperationScalarMultiply *self = GIMP_OPERATION_SCALAR_MULTIPLY (operation);
  const Babl                  *format;

  format = babl_format_n (babl_type ("float"), self->n_components);

  gegl_operation_set_format (operation, "input",  format);
  gegl_operation_set_format (operation, "output", format);
}

static gboolean
gimp_operation_scalar_multiply_process (GeglOperation       *operation,
                                        void                *in_buf,
                                        void                *out_buf,
                                        glong                samples,
                                        const GeglRectangle *roi,
                                        gint                 level)
{
  GimpOperationScalarMultiply *self = GIMP_OPERATION_SCALAR_MULTIPLY (operation);
  gfloat                      *src  = in_buf;
  gfloat                      *dest = out_buf;
  glong                        n_samples;

  n_samples = samples * self->n_components;

  while (n_samples--)
    {
      *dest = *src * self->factor;

      src++;
      dest++;
    }

  return TRUE;
}
