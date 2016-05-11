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
  PROP_NORMALIZE,
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

  g_object_class_install_property (object_class, PROP_NORMALIZE,
                                   g_param_spec_boolean ("normalize",
                                                         "Normalize",
                                                         "Normalize",
                                                         FALSE,
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
    case PROP_NORMALIZE:
      g_value_set_boolean (value, self->normalize);
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
    case PROP_NORMALIZE:
      self->normalize = g_value_get_boolean (value);
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
  gfloat      max_dist = 0.0;
  gfloat     *distbuf;
  gint        x, y;

  distbuf = g_new0 (gfloat, (roi->width + 1) * 2);

  for (y = 0; y < roi->height; y++)
    {
      gfloat *distbuf_cur;
      gfloat *distbuf_prev;
      gfloat  src = 0.0;

      /* toggling distance buffers for the current and previous row.
       * with one spare zero element on the left side */
      if (y % 2)
        {
          distbuf_prev = distbuf + 1;
          distbuf_cur  = distbuf + 1 + roi->width + 1;
        }
      else
        {
          distbuf_prev = distbuf + 1 + roi->width + 1;
          distbuf_cur  = distbuf + 1;
        }

      /*  clear the current rows distbuffer */
      memset (distbuf_cur, 0, sizeof (gfloat) * roi->width);

      for (x = 0; x < roi->width; x++)
        {
          gfloat dist_nw = MIN (distbuf_cur[x-1], distbuf_prev[x]);
          gfloat dist_se = MIN ((roi->width - x - 1), (roi->height - y - 1));
          gfloat dist    = MIN (dist_se, dist_nw);
          gfloat frac    = 1.0;
          gint   k;

#define EPSILON 0.0001

          /*  This loop used to start at "k = (dist) ? (dist - 1) : 0"
           *  and this comment suggested it might have to be changed to
           *  "k = 0", but "k = 0" increases processing time significantly.
           *
           *  When porting this to float, i noticed that starting at
           *  "dist - 2" gets rid of a lot of 8-bit artifacts, while starting
           *  at "dist - 3" or smaller would introduce different artifacts.
           *
           *  Note that I didn't really understand the entire algorithm,
           *  I just "blindly" ported it to float :) --mitch
           */

          /* the idea here is to check the south-eastern "thick" diagonal
           * along the already established accumulated minimum distance.
           *
           * it is easy to understand why it is sufficient to check
           * the triangle to this diagonal (k=0), but in fact we can
           * omit that, since this check has already been incorporated
           * in the accumulated minimum distance of the previous pixels.
           *
           * Not sure however if this is implemented properly.
           *      -- simon
           */

          for (k = MAX (dist - 2, 0); k <= dist; k++)
            {
              gint x1  = x;
              gint y1  = y + k;

              while (y1 >= y)
                {
                  /* FIXME: this should be much faster, it converts to
                   * 32 bit rgba intermediately, bah...
                   */
                  gegl_buffer_sample (input, x1 + roi->x, y1 + roi->y,
                                      NULL, &src, input_format,
                                      GEGL_SAMPLER_NEAREST, GEGL_ABYSS_NONE);

                  if (src < EPSILON)
                    {
                      dist = k;
                      break;
                    }

                  frac = MIN (frac, src);

                  x1++;
                  y1--;
                }
            }

          if (src > EPSILON)
            {
              /*  If dist_se != dist_nw use the previous frac
               *   if it is less than the one found
               */
              if (dist_se != dist)
                {
                  gfloat prev_frac = dist_nw - dist;

                  if (ABS (prev_frac - 1.0) < EPSILON)
                    prev_frac = 0.0;

                  frac = MIN (frac, prev_frac);
                }

              dist += 1.0;
            }

          distbuf_cur[x] = dist + frac;

          max_dist = MAX (max_dist, distbuf_cur[x]);
        }

      /*  set the dist row  */
      gegl_buffer_set (output,
                       GEGL_RECTANGLE (roi->x, roi->y + y,
                                       roi->width, 1),
                       0, output_format, distbuf_cur,
                       GEGL_AUTO_ROWSTRIDE);

      gegl_operation_progress (operation, (gdouble) y / roi->height, "");
    }

  g_free (distbuf);

  if (GIMP_OPERATION_SHAPEBURST (operation)->normalize && max_dist > 0.0)
    {
      GeglBufferIterator *iter;

      iter = gegl_buffer_iterator_new (output, NULL, 0, NULL,
                                       GEGL_ACCESS_READWRITE, GEGL_ABYSS_NONE);

      while (gegl_buffer_iterator_next (iter))
        {
          gint    count = iter->length;
          gfloat *data  = iter->data[0];

          while (count--)
            *data++ /= max_dist;
        }
    }

  gegl_operation_progress (operation, 1.0, "");

  return TRUE;
}
