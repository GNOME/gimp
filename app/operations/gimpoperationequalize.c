/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpoperationequalize.c
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
#include <gegl.h>
#include <gdk-pixbuf/gdk-pixbuf.h>

#include "libgimpcolor/gimpcolor.h"
#include "libgimpmath/gimpmath.h"

#include "operations-types.h"

#include "core/gimphistogram.h"

#include "gimpoperationequalize.h"


enum
{
  PROP_0,
  PROP_HISTOGRAM
};


static void     gimp_operation_equalize_finalize     (GObject             *object);
static void     gimp_operation_equalize_get_property (GObject             *object,
                                                      guint                property_id,
                                                      GValue              *value,
                                                      GParamSpec          *pspec);
static void     gimp_operation_equalize_set_property (GObject             *object,
                                                      guint                property_id,
                                                      const GValue        *value,
                                                      GParamSpec          *pspec);

static gboolean gimp_operation_equalize_process      (GeglOperation       *operation,
                                                      void                *in_buf,
                                                      void                *out_buf,
                                                      glong                samples,
                                                      const GeglRectangle *roi,
                                                      gint                 level);


G_DEFINE_TYPE (GimpOperationEqualize, gimp_operation_equalize,
               GIMP_TYPE_OPERATION_POINT_FILTER)

#define parent_class gimp_operation_equalize_parent_class


static void
gimp_operation_equalize_class_init (GimpOperationEqualizeClass *klass)
{
  GObjectClass                  *object_class    = G_OBJECT_CLASS (klass);
  GeglOperationClass            *operation_class = GEGL_OPERATION_CLASS (klass);
  GeglOperationPointFilterClass *point_class     = GEGL_OPERATION_POINT_FILTER_CLASS (klass);

  object_class->finalize       = gimp_operation_equalize_finalize;
  object_class->set_property   = gimp_operation_equalize_set_property;
  object_class->get_property   = gimp_operation_equalize_get_property;

  gegl_operation_class_set_keys (operation_class,
                                 "name",        "gimp:equalize",
                                 "categories",  "color",
                                 "description", "GIMP Equalize operation",
                                 NULL);

  point_class->process = gimp_operation_equalize_process;

  g_object_class_install_property (object_class, PROP_HISTOGRAM,
                                   g_param_spec_object ("histogram",
                                                        "Histogram",
                                                        "The histogram",
                                                        GIMP_TYPE_HISTOGRAM,
                                                        G_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT_ONLY));
}

static void
gimp_operation_equalize_init (GimpOperationEqualize *self)
{
  self->values = NULL;
  self->n_bins = 0;
}

static void
gimp_operation_equalize_finalize (GObject *object)
{
  GimpOperationEqualize *self = GIMP_OPERATION_EQUALIZE (object);

  g_clear_pointer (&self->values, g_free);
  g_clear_object (&self->histogram);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
gimp_operation_equalize_get_property (GObject    *object,
                                      guint       property_id,
                                      GValue     *value,
                                      GParamSpec *pspec)
{
  GimpOperationEqualize *self = GIMP_OPERATION_EQUALIZE (object);

  switch (property_id)
    {
    case PROP_HISTOGRAM:
      g_value_set_pointer (value, self->histogram);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_operation_equalize_set_property (GObject      *object,
                                      guint         property_id,
                                      const GValue *value,
                                      GParamSpec   *pspec)
{
  GimpOperationEqualize *self = GIMP_OPERATION_EQUALIZE (object);

  switch (property_id)
    {
    case PROP_HISTOGRAM:
      if (self->histogram)
        g_object_unref (self->histogram);

      self->histogram = g_value_dup_object (value);

      if (self->histogram)
        {
          gdouble pixels;
          gint    n_bins;
          gint    max;
          gint    k;

          n_bins = gimp_histogram_n_bins (self->histogram);

          if ((self->values != NULL) && (self->n_bins != n_bins))
            {
              g_free (self->values);
              self->values = NULL;
            }

          if (self->values == NULL)
            {
              self->values = g_new (gdouble, 3 * n_bins);
            }

          self->n_bins = n_bins;

          pixels = gimp_histogram_get_count (self->histogram,
                                             GIMP_HISTOGRAM_VALUE, 0, n_bins - 1);

          if (gimp_histogram_n_components (self->histogram) == 1 ||
              gimp_histogram_n_components (self->histogram) == 2)
            max = 1;
          else
            max = 3;

          for (k = 0; k < 3; k++)
            {
              gdouble sum = 0;
              gint    i;

             for (i = 0; i < n_bins; i++)
                {
                  gdouble histi;

                  histi = gimp_histogram_get_component (self->histogram, k, i);

                  sum += histi;

                  self->values[k * n_bins + i] = sum / pixels;

                  if (max == 1)
                    {
                      self->values[n_bins + i] = self->values[i];
                      self->values[2 * n_bins + i] = self->values[i];
                    }
                }
           }
        }
      break;

   default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static inline float
gimp_operation_equalize_map (GimpOperationEqualize *self,
                             gint                   component,
                             gfloat                 value)
{
  gint index;
  index = component * self->n_bins + \
            (gint) (CLAMP (value * (self->n_bins - 1), 0.0, self->n_bins - 1));

  return self->values[index];
}

static gboolean
gimp_operation_equalize_process (GeglOperation       *operation,
                                 void                *in_buf,
                                 void                *out_buf,
                                 glong                samples,
                                 const GeglRectangle *roi,
                                 gint                 level)
{
  GimpOperationEqualize *self = GIMP_OPERATION_EQUALIZE (operation);
  gfloat                *src  = in_buf;
  gfloat                *dest = out_buf;

  while (samples--)
    {
      dest[RED]   = gimp_operation_equalize_map (self, RED,   src[RED]);
      dest[GREEN] = gimp_operation_equalize_map (self, GREEN, src[GREEN]);
      dest[BLUE]  = gimp_operation_equalize_map (self, BLUE,  src[BLUE]);
      dest[ALPHA] = src[ALPHA];

      src  += 4;
      dest += 4;
    }

  return TRUE;
}
