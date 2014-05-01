/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpoperationshapeburst.c
 * Copyright (C) 2012 Michael Natterer <mitch@gimp.org>
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
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <cairo.h>
#include <gegl.h>
#include <gdk-pixbuf/gdk-pixbuf.h>

#include "libgimpcolor/gimpcolor.h"
#include "libgimpmath/gimpmath.h"

#include "operations-types.h"

#include "gimpoperationshapeburst.h"


enum
{
  PROP_0,
  PROP_MAX_ITERATIONS,
  PROP_PROGRESS
};


static void     gimp_operation_shapeburst_get_property (GObject      *object,
                                                        guint         property_id,
                                                        GValue       *value,
                                                        GParamSpec   *pspec);
static void     gimp_operation_shapeburst_set_property (GObject      *object,
                                                        guint         property_id,
                                                        const GValue *value,
                                                        GParamSpec   *pspec);

static GeglRectangle
gimp_operation_shapeburst_get_required_for_output (GeglOperation       *self,
                                                   const gchar         *input_pad,
                                                   const GeglRectangle *roi);
static GeglRectangle
      gimp_operation_shapeburst_get_cached_region (GeglOperation       *self,
                                                   const GeglRectangle *roi);
static void     gimp_operation_shapeburst_prepare (GeglOperation       *operation);
static gboolean gimp_operation_shapeburst_process (GeglOperation       *operation,
                                                   GeglBuffer          *input,
                                                   GeglBuffer          *output,
                                                   const GeglRectangle *roi,
                                                   gint                 level);


G_DEFINE_TYPE (GimpOperationShapeburst, gimp_operation_shapeburst,
               GEGL_TYPE_OPERATION_FILTER)

#define parent_class gimp_operation_shapeburst_parent_class


static void
gimp_operation_shapeburst_class_init (GimpOperationShapeburstClass *klass)
{
  GObjectClass             *object_class    = G_OBJECT_CLASS (klass);
  GeglOperationClass       *operation_class = GEGL_OPERATION_CLASS (klass);
  GeglOperationFilterClass *filter_class    = GEGL_OPERATION_FILTER_CLASS (klass);

  object_class->set_property   = gimp_operation_shapeburst_set_property;
  object_class->get_property   = gimp_operation_shapeburst_get_property;

  gegl_operation_class_set_keys (operation_class,
                                 "name",        "gimp:shapeburst",
                                 "categories",  "gimp",
                                 "description", "GIMP Shapeburst operation",
                                 NULL);

  operation_class->prepare                 = gimp_operation_shapeburst_prepare;
  operation_class->get_required_for_output = gimp_operation_shapeburst_get_required_for_output;
  operation_class->get_cached_region       = gimp_operation_shapeburst_get_cached_region;

  filter_class->process                    = gimp_operation_shapeburst_process;

  g_object_class_install_property (object_class, PROP_MAX_ITERATIONS,
                                   g_param_spec_double ("max-iterations",
                                                        "Max Iterations",
                                                        "Max Iterations",
                                                        0.0, G_MAXFLOAT, 0.0,
                                                        G_PARAM_READWRITE));

  g_object_class_install_property (object_class, PROP_PROGRESS,
                                   g_param_spec_double ("progress",
                                                        "Progress",
                                                        "Progress indicator, and a bad hack",
                                                        0.0, 1.0, 0.0,
                                                        G_PARAM_READWRITE));
}

static void
gimp_operation_shapeburst_init (GimpOperationShapeburst *self)
{
}

static void
gimp_operation_shapeburst_get_property (GObject    *object,
                                        guint       property_id,
                                        GValue     *value,
                                        GParamSpec *pspec)
{
 GimpOperationShapeburst *self = GIMP_OPERATION_SHAPEBURST (object);

  switch (property_id)
    {
    case PROP_MAX_ITERATIONS:
      g_value_set_double (value, self->max_iterations);
      break;

    case PROP_PROGRESS:
      g_value_set_double (value, self->progress);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_operation_shapeburst_set_property (GObject      *object,
                                        guint         property_id,
                                        const GValue *value,
                                        GParamSpec   *pspec)
{
 GimpOperationShapeburst *self = GIMP_OPERATION_SHAPEBURST (object);

  switch (property_id)
    {
    case PROP_MAX_ITERATIONS:
      self->max_iterations = g_value_get_double (value);
      break;

    case PROP_PROGRESS:
      self->progress = g_value_get_double (value);
      break;

   default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_operation_shapeburst_prepare (GeglOperation *operation)
{
  gegl_operation_set_format (operation, "input",  babl_format ("Y float"));
  gegl_operation_set_format (operation, "output", babl_format ("Y float"));
}

static GeglRectangle
gimp_operation_shapeburst_get_required_for_output (GeglOperation       *self,
                                                   const gchar         *input_pad,
                                                   const GeglRectangle *roi)
{
  return *gegl_operation_source_get_bounding_box (self, "input");
}

static GeglRectangle
gimp_operation_shapeburst_get_cached_region (GeglOperation       *self,
                                             const GeglRectangle *roi)
{
  return *gegl_operation_source_get_bounding_box (self, "input");
}

static gboolean
gimp_operation_shapeburst_process (GeglOperation       *operation,
                                   GeglBuffer          *input,
                                   GeglBuffer          *output,
                                   const GeglRectangle *roi,
                                   gint                 level)
{
  const Babl *input_format   = babl_format ("Y float");
  const Babl *output_format  = babl_format ("Y float");
  gfloat      max_iterations = 0.0;
  gfloat     *distp_cur;
  gfloat     *distp_prev;
  gfloat     *memory;
  gint        length;
  gint        i;

  length = roi->width + 1;
  memory = g_new (gfloat, length * 2);

  distp_prev = memory;
  for (i = 0; i < length; i++)
    distp_prev[i] = 0.0;

  distp_prev += 1;
  distp_cur = distp_prev + length;

  for (i = 0; i < roi->height; i++)
    {
      gfloat *tmp;
      gfloat  src = 0.0;
      gint    j;

      /*  set the current dist row to 0's  */
      memset (distp_cur - 1, 0, sizeof (gfloat) * (length - 1));

      for (j = 0; j < roi->width; j++)
        {
          gfloat float_tmp;
          gfloat min_prev = MIN (distp_cur[j-1], distp_prev[j]);
          gint   min_left = MIN ((roi->width - j - 1), (roi->height - i - 1));
          gint   min      = (gint) MIN (min_left, min_prev);
          gfloat fraction = 1.0;
          gint   k;

#define EPSILON 0.0001

          /*  This loop used to start at "k = (min) ? (min - 1) : 0"
           *  and this comment suggested it might have to be changed to
           *  "k = 0", but "k = 0" increases processing time significantly.
           *
           *  When porting this to float, i noticed that starting at
           *  "min - 2" gets rid of a lot of 8-bit artifacts, while starting
           *  at "min - 3" or smaller would introduce different artifacts.
           *
           *  Note that I didn't really understand the entire algorithm,
           *  I just "blindly" ported it to float :) --mitch
           */
          for (k = MAX (min - 2, 0); k <= min; k++)
            {
              gint x   = j;
              gint y   = i + k;
              gint end = y - k;

              while (y >= end)
                {
#if 1
                  /* FIXME: this should be much faster, it converts
                   * to 32 bit rgba intermediately, bah...
                   */
                  gegl_buffer_sample (input, x, y, NULL, &src,
                                      input_format,
                                      GEGL_SAMPLER_NEAREST, GEGL_ABYSS_NONE);
#else
                  gegl_buffer_get (input, GEGL_RECTANGLE (x, y, 1, 1), 1.0,
                                   input_format, &src,
                                   GEGL_AUTO_ROWSTRIDE, GEGL_ABYSS_NONE);
#endif

                  if (ABS (src) < EPSILON)
                    {
                      min = k;
                      y = -1;
                      break;
                    }

                  if (src < fraction)
                    fraction = src;

                  x++;
                  y--;
                }
            }

          if (src > EPSILON)
            {
              /*  If min_left != min_prev use the previous fraction
               *   if it is less than the one found
               */
              if (min_left != min)
                {
                  gfloat prev_frac = min_prev - min;

                  if (ABS (prev_frac - 1.0) < EPSILON)
                    prev_frac = 0.0;

                  fraction = MIN (fraction, prev_frac);
                }

              min++;
            }

          float_tmp = distp_cur[j] = min + fraction * (1.0 - EPSILON);

          if (float_tmp > max_iterations)
            max_iterations = float_tmp;
        }

      /*  set the dist row  */
      gegl_buffer_set (output,
                       GEGL_RECTANGLE (roi->x, roi->y + i,
                                       roi->width, 1),
                       1.0, output_format, distp_cur,
                       GEGL_AUTO_ROWSTRIDE);

      /*  swap pointers around  */
      tmp = distp_prev;
      distp_prev = distp_cur;
      distp_cur = tmp;

      g_object_set (operation,
                    "progress", (gdouble) i / roi->height,
                    NULL);
    }

  g_free (memory);

  g_object_set (operation,
                "max-iterations", (gdouble) max_iterations,
                NULL);

  return TRUE;
}
