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

#include "libgimpcolor/gimpcolor.h"
#include "libgimpmath/gimpmath.h"

#include "gimp-gegl-types.h"

#include "gimp-gegl-utils.h"
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
    "name"        , "gimp:shapeburst",
    "categories"  , "gimp",
    "description" , "GIMP Shapeburst operation",
    NULL);

  operation_class->prepare                 = gimp_operation_shapeburst_prepare;
  operation_class->get_required_for_output = gimp_operation_shapeburst_get_required_for_output;
  operation_class->get_cached_region       = gimp_operation_shapeburst_get_cached_region;

  filter_class->process        = gimp_operation_shapeburst_process;

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
  gegl_operation_set_format (operation, "input",  babl_format ("Y u8"));
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
  const Babl *input_format   = babl_format ("Y u8");
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
      gint    src = 0;
      gint    j;

      /*  set the current dist row to 0's  */
      memset (distp_cur - 1, 0, sizeof (gfloat) * (length - 1));

      for (j = 0; j < roi->width; j++)
        {
          gfloat float_tmp;
          gfloat min_prev = MIN (distp_cur[j-1], distp_prev[j]);
          gint   min_left = MIN ((roi->width - j - 1), (roi->height - i - 1));
          gint   min      = (gint) MIN (min_left, min_prev);
          gint   fraction = 255;
          gint   k;

          /*  This might need to be changed to 0
              instead of k = (min) ? (min - 1) : 0  */

          for (k = (min) ? (min - 1) : 0; k <= min; k++)
            {
              gint x   = j;
              gint y   = i + k;
              gint end = y - k;

              while (y >= end)
                {
                  guchar src_uchar;

#if 1
                  /* FIXME: this should be much faster, it converts
                   * to 32 bit rgba intermediately, bah...
                   */
                  gegl_buffer_sample (input, x, y, NULL, &src_uchar,
                                      input_format,
                                      GEGL_SAMPLER_NEAREST, GEGL_ABYSS_NONE);
#else
                  gegl_buffer_get (input, GIMP_GEGL_RECT (x, y, 1, 1), 1.0,
                                   input_format, &src_uchar,
                                   GEGL_AUTO_ROWSTRIDE, GEGL_ABYSS_NONE);
#endif

                  src = src_uchar;

                  if (src == 0)
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

          if (src != 0)
            {
              /*  If min_left != min_prev use the previous fraction
               *   if it is less than the one found
               */
              if (min_left != min)
                {
                  gint prev_frac = (int) (255 * (min_prev - min));

                  if (prev_frac == 255)
                    prev_frac = 0;

                  fraction = MIN (fraction, prev_frac);
                }

              min++;
            }

          float_tmp = distp_cur[j] = min + fraction / 256.0;

          if (float_tmp > max_iterations)
            max_iterations = float_tmp;
        }

      /*  set the dist row  */
      gegl_buffer_set (output,
                       GIMP_GEGL_RECT (roi->x, roi->y + i,
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
