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
};


static void     gimp_operation_equalize_finalize     (GObject              *object);

static void     gimp_operation_equalize_prepare      (GeglOperation        *operation);
static gboolean gimp_operation_equalize_op_process   (GeglOperation        *operation,
                                                      GeglOperationContext *context,
                                                      const gchar          *output_pad,
                                                      const GeglRectangle  *roi,
                                                      gint                  level);

static gboolean gimp_operation_equalize_process      (GeglOperation        *operation,
                                                      void                 *in_buf,
                                                      void                 *out_buf,
                                                      glong                 samples,
                                                      const GeglRectangle  *roi,
                                                      gint                  level);


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

  operation_class->prepare     = gimp_operation_equalize_prepare;
  operation_class->process     = gimp_operation_equalize_op_process;

  gegl_operation_class_set_keys (operation_class,
                                 "name",        "gimp:equalize",
                                 /* Adding to "hidden" category because
                                  * it cannot be used through the API
                                  * right now (missing GimpHistogram
                                  * type in libgimp), and even less as
                                  * NDE since it won't update the
                                  * histogram on changes.
                                  */
                                 "categories",  "color:hidden",
                                 "description", "GIMP Equalize operation",
                                 NULL);

  point_class->process = gimp_operation_equalize_process;
}

static void
gimp_operation_equalize_init (GimpOperationEqualize *self)
{
  GimpOperationPointFilter *pt = GIMP_OPERATION_POINT_FILTER (self);

  /* Let's have equalize work in non-linear space (see #14486), just
   * like in GIMP 2.10.
   * I do wonder if really what matters is that we work in a
   * kinda-perceptual space, then maybe we actually want to work in
   * GIMP_TRC_PERCEPTUAL.
   * And are there cases where we want to equalize across actual light?
   * In this case, maybe we should add a "trc" property to this
   * operation would allow people to use it however they want.
   */
  pt->trc = GIMP_TRC_NON_LINEAR;

  self->values    = NULL;
  self->n_bins    = 0;
  self->histogram = gimp_histogram_new (pt->trc);
  g_mutex_init (&self->mutex);
}

static void
gimp_operation_equalize_finalize (GObject *object)
{
  GimpOperationEqualize *self = GIMP_OPERATION_EQUALIZE (object);

  g_clear_pointer (&self->values, g_free);
  g_clear_object (&self->histogram);
  g_mutex_clear (&self->mutex);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static inline float
gimp_operation_equalize_map (GimpOperationEqualize *self,
                             gint                   component,
                             gfloat                 value)
{
  gint index;

  if (isnan (value))
    value = 0;

  index = component * self->n_bins + \
            (gint) (CLAMP (value * (self->n_bins - 1), 0.0, self->n_bins - 1));

  return self->values[index];
}

static void
gimp_operation_equalize_prepare (GeglOperation *operation)
{
  GimpOperationEqualize *eq = GIMP_OPERATION_EQUALIZE (operation);

  g_clear_pointer (&eq->values, g_free);
  return GEGL_OPERATION_CLASS (parent_class)->prepare (operation);
}

static gboolean
gimp_operation_equalize_op_process (GeglOperation       *operation,
                                    GeglOperationContext *context,
                                    const gchar          *output_pad,
                                    const GeglRectangle  *roi,
                                    gint                  level)
{
  GimpOperationEqualize *eq = GIMP_OPERATION_EQUALIZE (operation);

  g_mutex_lock (&eq->mutex);
  if (eq->values == NULL)
    {
      GimpHistogram       *histogram = eq->histogram;
      GeglBuffer          *input     = GEGL_BUFFER (gegl_operation_context_get_object (context, "input"));
      const GeglRectangle *extents   = gegl_buffer_get_extent (input);
      gdouble              pixels;
      gint                 n_bins;
      gint                 max;
      gint                 k;

      gimp_histogram_calculate (histogram, input, extents, NULL, NULL);

      n_bins = gimp_histogram_n_bins (histogram);

      eq->values = g_new (gdouble, 3 * n_bins);
      eq->n_bins = n_bins;

      pixels = gimp_histogram_get_count (histogram,
                                         GIMP_HISTOGRAM_VALUE, 0, n_bins - 1);

      if (gimp_histogram_n_components (histogram) == 1 ||
          gimp_histogram_n_components (histogram) == 2)
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

              histi = gimp_histogram_get_component (histogram, k, i);

              sum += histi;

              eq->values[k * n_bins + i] = sum / pixels;

              if (max == 1)
                {
                  eq->values[n_bins + i] = eq->values[i];
                  eq->values[2 * n_bins + i] = eq->values[i];
                }
            }
        }
    }
  g_mutex_unlock (&eq->mutex);

  return GEGL_OPERATION_CLASS (parent_class)->process (operation, context, output_pad, roi, level);
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
