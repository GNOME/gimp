/* GIMP - The GNU Image Manipulation Program
 *
 * gimpoperationcage.c
 * Copyright (C) 2010 Michael Muré <batolettre@gmail.com>
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

#include "gimpoperationcagetransform.h"
#include "gimpcageconfig.h"

#include "gimp-intl.h"

enum
{
  PROP_0,
  PROP_CONFIG,
  PROP_FILL,
};


static void         gimp_operation_cage_transform_finalize                (GObject             *object);
static void         gimp_operation_cage_transform_get_property            (GObject             *object,
                                                                           guint                property_id,
                                                                           GValue              *value,
                                                                           GParamSpec          *pspec);
static void         gimp_operation_cage_transform_set_property            (GObject             *object,
                                                                           guint                property_id,
                                                                           const GValue        *value,
                                                                           GParamSpec          *pspec);
static void         gimp_operation_cage_transform_prepare                 (GeglOperation       *operation);
static gboolean     gimp_operation_cage_transform_process                 (GeglOperation       *operation,
                                                                           GeglBuffer          *in_buf,
                                                                           GeglBuffer          *aux_buf,
                                                                           GeglBuffer          *out_buf,
                                                                           const GeglRectangle *roi,
                                                                           gint                 level);
static void         gimp_operation_cage_transform_interpolate_source_coords_recurs
                                                                          (GimpOperationCageTransform  *oct,
                                                                           GeglBuffer          *out_buf,
                                                                           const GeglRectangle *roi,
                                                                           GimpVector2          p1_s,
                                                                           GimpVector2          p1_d,
                                                                           GimpVector2          p2_s,
                                                                           GimpVector2          p2_d,
                                                                           GimpVector2          p3_s,
                                                                           GimpVector2          p3_d,
                                                                           gint                 recursion_depth,
                                                                           gfloat              *coords);
static GimpVector2  gimp_cage_transform_compute_destination               (GimpCageConfig      *config,
                                                                           gfloat              *coef,
                                                                           GeglSampler         *coef_sampler,
                                                                           GimpVector2          coords);
GeglRectangle       gimp_operation_cage_transform_get_cached_region       (GeglOperation       *operation,
                                                                           const GeglRectangle *roi);
GeglRectangle       gimp_operation_cage_transform_get_required_for_output (GeglOperation       *operation,
                                                                           const gchar         *input_pad,
                                                                           const GeglRectangle *roi);
GeglRectangle       gimp_operation_cage_transform_get_bounding_box        (GeglOperation       *operation);


G_DEFINE_TYPE (GimpOperationCageTransform, gimp_operation_cage_transform,
                      GEGL_TYPE_OPERATION_COMPOSER)

#define parent_class gimp_operation_cage_transform_parent_class


static void
gimp_operation_cage_transform_class_init (GimpOperationCageTransformClass *klass)
{
  GObjectClass               *object_class    = G_OBJECT_CLASS (klass);
  GeglOperationClass         *operation_class = GEGL_OPERATION_CLASS (klass);
  GeglOperationComposerClass *filter_class    = GEGL_OPERATION_COMPOSER_CLASS (klass);

  object_class->get_property               = gimp_operation_cage_transform_get_property;
  object_class->set_property               = gimp_operation_cage_transform_set_property;
  object_class->finalize                   = gimp_operation_cage_transform_finalize;

  gegl_operation_class_set_keys (operation_class,
                                 "name",        "gimp:cage-transform",
                                 "categories",  "transform",
                                 "description", _("Convert a set of coefficient buffer to a coordinate buffer for the GIMP cage tool"),
                                 NULL);

  operation_class->prepare                 = gimp_operation_cage_transform_prepare;

  operation_class->get_required_for_output = gimp_operation_cage_transform_get_required_for_output;
  operation_class->get_cached_region       = gimp_operation_cage_transform_get_cached_region;
  operation_class->get_bounding_box        = gimp_operation_cage_transform_get_bounding_box;
  /* XXX Temporarily disable multi-threading on this operation because
   * it is much faster when single-threaded. See bug 787663.
   */
  operation_class->threaded                = FALSE;

  filter_class->process                    = gimp_operation_cage_transform_process;

  g_object_class_install_property (object_class, PROP_CONFIG,
                                   g_param_spec_object ("config",
                                                        "Config",
                                                        "A GimpCageConfig object, that define the transformation",
                                                        GIMP_TYPE_CAGE_CONFIG,
                                                        G_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT));

  g_object_class_install_property (object_class, PROP_FILL,
                                   g_param_spec_boolean ("fill-plain-color",
                                                         _("Fill with plain color"),
                                                         _("Fill the original position of the cage with a plain color"),
                                                         FALSE,
                                                         G_PARAM_READWRITE));
}

static void
gimp_operation_cage_transform_init (GimpOperationCageTransform *self)
{
  self->format_coords = babl_format_n(babl_type("float"), 2);
}

static void
gimp_operation_cage_transform_finalize (GObject *object)
{
  GimpOperationCageTransform *self = GIMP_OPERATION_CAGE_TRANSFORM (object);

  g_clear_object (&self->config);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
gimp_operation_cage_transform_get_property (GObject    *object,
                                            guint       property_id,
                                            GValue     *value,
                                            GParamSpec *pspec)
{
  GimpOperationCageTransform *self = GIMP_OPERATION_CAGE_TRANSFORM (object);

  switch (property_id)
    {
    case PROP_CONFIG:
      g_value_set_object (value, self->config);
      break;
    case PROP_FILL:
      g_value_set_boolean (value, self->fill_plain_color);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_operation_cage_transform_set_property (GObject      *object,
                                            guint         property_id,
                                            const GValue *value,
                                            GParamSpec   *pspec)
{
  GimpOperationCageTransform *self = GIMP_OPERATION_CAGE_TRANSFORM (object);

  switch (property_id)
    {
    case PROP_CONFIG:
      if (self->config)
        g_object_unref (self->config);
      self->config = g_value_dup_object (value);
      break;
    case PROP_FILL:
      self->fill_plain_color = g_value_get_boolean (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_operation_cage_transform_prepare (GeglOperation *operation)
{
  GimpOperationCageTransform *oct    = GIMP_OPERATION_CAGE_TRANSFORM (operation);
  GimpCageConfig             *config = GIMP_CAGE_CONFIG (oct->config);

  gegl_operation_set_format (operation, "input",
                             babl_format_n (babl_type ("float"),
                                            2 * gimp_cage_config_get_n_points (config)));
  gegl_operation_set_format (operation, "output",
                             babl_format_n (babl_type ("float"), 2));
}

static gboolean
gimp_operation_cage_transform_process (GeglOperation       *operation,
                                       GeglBuffer          *in_buf,
                                       GeglBuffer          *aux_buf,
                                       GeglBuffer          *out_buf,
                                       const GeglRectangle *roi,
                                       gint                 level)
{
  GimpOperationCageTransform *oct    = GIMP_OPERATION_CAGE_TRANSFORM (operation);
  GimpCageConfig             *config = GIMP_CAGE_CONFIG (oct->config);
  GeglRectangle               cage_bb;
  gfloat                     *coords;
  gfloat                     *coef;
  const Babl                 *format_coef;
  GeglSampler                *coef_sampler;
  GimpVector2                 plain_color;
  GeglBufferIterator         *it;
  gint                        x, y;
  gboolean                    output_set;
  GimpCagePoint              *point;
  guint                       n_cage_vertices;

  /* pre-fill the out buffer with no-displacement coordinate */
  it      = gegl_buffer_iterator_new (out_buf, roi, 0, NULL,
                                      GEGL_ACCESS_WRITE, GEGL_ABYSS_NONE, 1);
  cage_bb = gimp_cage_config_get_bounding_box (config);

  point = &(g_array_index (config->cage_points, GimpCagePoint, 0));
  plain_color.x = (gint) point->src_point.x;
  plain_color.y = (gint) point->src_point.y;

  n_cage_vertices = gimp_cage_config_get_n_points (config);

  while (gegl_buffer_iterator_next (it))
    {
      /* iterate inside the roi */
      gint    n_pixels = it->length;
      gfloat *output   = it->items[0].data;

      x = it->items[0].roi.x; /* initial x         */
      y = it->items[0].roi.y; /* and y coordinates */

      while (n_pixels--)
        {
          output_set = FALSE;
          if (oct->fill_plain_color)
            {
              if (x > cage_bb.x &&
                  y > cage_bb.y &&
                  x < cage_bb.x + cage_bb.width &&
                  y < cage_bb.y + cage_bb.height)
                {
                  if (gimp_cage_config_point_inside (config, x, y))
                    {
                      output[0] = plain_color.x;
                      output[1] = plain_color.y;
                      output_set = TRUE;
                    }
                }
            }
          if (!output_set)
            {
              output[0] = x + 0.5;
              output[1] = y + 0.5;
            }

          output += 2;

          /* update x and y coordinates */
          x++;
          if (x >= (it->items[0].roi.x + it->items[0].roi.width))
            {
              x = it->items[0].roi.x;
              y++;
            }
        }
    }

  if (! aux_buf)
    return TRUE;

  gegl_operation_progress (operation, 0.0, "");

  /* pre-allocate memory outside of the loop */
  coords       = g_slice_alloc (2 * sizeof (gfloat));
  coef         = g_malloc (n_cage_vertices * 2 * sizeof (gfloat));
  format_coef  = babl_format_n (babl_type ("float"), 2 * n_cage_vertices);
  coef_sampler = gegl_buffer_sampler_new (aux_buf,
                                          format_coef, GEGL_SAMPLER_NEAREST);

  /* compute, reverse and interpolate the transformation */
  for (y = cage_bb.y; y < cage_bb.y + cage_bb.height - 1; y++)
    {
      GimpVector2 p1_d, p2_d, p3_d, p4_d;
      GimpVector2 p1_s, p2_s, p3_s, p4_s;

      p1_s.y = y;
      p2_s.y = y+1;
      p3_s.y = y+1;
      p3_s.x = cage_bb.x;
      p4_s.y = y;
      p4_s.x = cage_bb.x;

      p3_d = gimp_cage_transform_compute_destination (config, coef, coef_sampler, p3_s);
      p4_d = gimp_cage_transform_compute_destination (config, coef, coef_sampler, p4_s);

      for (x = cage_bb.x; x < cage_bb.x + cage_bb.width - 1; x++)
        {
          p1_s = p4_s;
          p2_s = p3_s;
          p3_s.x = x+1;
          p4_s.x = x+1;

          p1_d = p4_d;
          p2_d = p3_d;
          p3_d = gimp_cage_transform_compute_destination (config, coef, coef_sampler, p3_s);
          p4_d = gimp_cage_transform_compute_destination (config, coef, coef_sampler, p4_s);

          if (gimp_cage_config_point_inside (config, x, y))
            {
              gimp_operation_cage_transform_interpolate_source_coords_recurs (oct,
                                                                              out_buf,
                                                                              roi,
                                                                              p1_s, p1_d,
                                                                              p2_s, p2_d,
                                                                              p3_s, p3_d,
                                                                              0,
                                                                              coords);

              gimp_operation_cage_transform_interpolate_source_coords_recurs (oct,
                                                                              out_buf,
                                                                              roi,
                                                                              p1_s, p1_d,
                                                                              p3_s, p3_d,
                                                                              p4_s, p4_d,
                                                                              0,
                                                                              coords);
            }
        }

      if ((y - cage_bb.y) % 20 == 0)
        {
          gdouble fraction = ((gdouble) (y - cage_bb.y) /
                              (gdouble) (cage_bb.height));

          /*  0.0 and 1.0 indicate progress start/end, so avoid them  */
          if (fraction > 0.0 && fraction < 1.0)
            {
              gegl_operation_progress (operation, fraction, "");
            }
        }
    }

  g_object_unref (coef_sampler);
  g_free (coef);
  g_slice_free1 (2 * sizeof (gfloat), coords);

  gegl_operation_progress (operation, 1.0, "");

  return TRUE;
}


static void
gimp_operation_cage_transform_interpolate_source_coords_recurs (GimpOperationCageTransform *oct,
                                                                GeglBuffer                 *out_buf,
                                                                const GeglRectangle        *roi,
                                                                GimpVector2                 p1_s,
                                                                GimpVector2                 p1_d,
                                                                GimpVector2                 p2_s,
                                                                GimpVector2                 p2_d,
                                                                GimpVector2                 p3_s,
                                                                GimpVector2                 p3_d,
                                                                gint                        recursion_depth,
                                                                gfloat                     *coords)
{
  gint xmin, xmax, ymin, ymax, x, y;

  /* Stop recursion if all 3 vertices of the triangle are outside the
   * ROI (left/right or above/below).
   */
  if (p1_d.x >= roi->x + roi->width &&
      p2_d.x >= roi->x + roi->width &&
      p3_d.x >= roi->x + roi->width) return;
  if (p1_d.y >= roi->y + roi->height &&
      p2_d.y >= roi->y + roi->height &&
      p3_d.y >= roi->y + roi->height) return;

  if (p1_d.x < roi->x &&
      p2_d.x < roi->x &&
      p3_d.x < roi->x) return;
  if (p1_d.y < roi->y &&
      p2_d.y < roi->y &&
      p3_d.y < roi->y) return;

  xmin = xmax = lrint (p1_d.x);
  ymin = ymax = lrint (p1_d.y);

  x = lrint (p2_d.x);
  xmin = MIN (x, xmin);
  xmax = MAX (x, xmax);

  x = lrint (p3_d.x);
  xmin = MIN (x, xmin);
  xmax = MAX (x, xmax);

  y = lrint (p2_d.y);
  ymin = MIN (y, ymin);
  ymax = MAX (y, ymax);

  y = lrint (p3_d.y);
  ymin = MIN (y, ymin);
  ymax = MAX (y, ymax);

  /* test if there is no more pixel in the triangle */
  if (xmin == xmax || ymin == ymax)
    return;

  /* test if the triangle is implausibly large as manifested by too deep recursion */
  if (recursion_depth > 5)
    return;

  /* test if the triangle is small enough.
   *
   * if yes, we compute the coefficient of the barycenter for the
   * pixel (x,y) and see if a pixel is inside (ie the 3 coef have the
   * same sign).
   */
  if (xmax - xmin == 1 && ymax - ymin == 1)
    {
      gdouble a, b, c, denom, x, y;

      x = (gdouble) xmin + 0.5;
      y = (gdouble) ymin + 0.5;

      denom = (p2_d.x - p1_d.x) * p3_d.y + (p1_d.x - p3_d.x) * p2_d.y + (p3_d.x - p2_d.x) * p1_d.y;
      a = ((p2_d.x - x) * p3_d.y + (x - p3_d.x) * p2_d.y + (p3_d.x - p2_d.x) * y) / denom;
      b = - ((p1_d.x - x) * p3_d.y + (x - p3_d.x) * p1_d.y + (p3_d.x - p1_d.x) * y) / denom;
      c = 1.0 - a - b;

      /* if a pixel is inside, we compute its source coordinate and
       * set it in the output buffer
       */
      if ((a > 0 && b > 0 && c > 0) || (a < 0 && b < 0 && c < 0))
        {
          GeglRectangle rect = { 0, 0, 1, 1 };
          gfloat        coords[2];

          rect.x = xmin;
          rect.y = ymin;

          coords[0] = (a * p1_s.x + b * p2_s.x + c * p3_s.x);
          coords[1] = (a * p1_s.y + b * p2_s.y + c * p3_s.y);

          gegl_buffer_set (out_buf,
                           &rect,
                           0,
                           oct->format_coords,
                           coords,
                           GEGL_AUTO_ROWSTRIDE);
        }

      return;
    }
  else
    {
      /* we cut the triangle in 4 sub-triangle and treat it recursively */
      /*
       *       /\
       *      /__\
       *     /\  /\
       *    /__\/__\
       *
       */

      GimpVector2 pm1_d, pm2_d, pm3_d;
      GimpVector2 pm1_s, pm2_s, pm3_s;
      gint        next_depth = recursion_depth + 1;

      pm1_d.x = (p1_d.x + p2_d.x) / 2.0;
      pm1_d.y = (p1_d.y + p2_d.y) / 2.0;

      pm2_d.x = (p2_d.x + p3_d.x) / 2.0;
      pm2_d.y = (p2_d.y + p3_d.y) / 2.0;

      pm3_d.x = (p3_d.x + p1_d.x) / 2.0;
      pm3_d.y = (p3_d.y + p1_d.y) / 2.0;

      pm1_s.x = (p1_s.x + p2_s.x) / 2.0;
      pm1_s.y = (p1_s.y + p2_s.y) / 2.0;

      pm2_s.x = (p2_s.x + p3_s.x) / 2.0;
      pm2_s.y = (p2_s.y + p3_s.y) / 2.0;

      pm3_s.x = (p3_s.x + p1_s.x) / 2.0;
      pm3_s.y = (p3_s.y + p1_s.y) / 2.0;

      gimp_operation_cage_transform_interpolate_source_coords_recurs (oct,
                                                                      out_buf,
                                                                      roi,
                                                                      p1_s, p1_d,
                                                                      pm1_s, pm1_d,
                                                                      pm3_s, pm3_d,
                                                                      next_depth,
                                                                      coords);

      gimp_operation_cage_transform_interpolate_source_coords_recurs (oct,
                                                                      out_buf,
                                                                      roi,
                                                                      pm1_s, pm1_d,
                                                                      p2_s, p2_d,
                                                                      pm2_s, pm2_d,
                                                                      next_depth,
                                                                      coords);

      gimp_operation_cage_transform_interpolate_source_coords_recurs (oct,
                                                                      out_buf,
                                                                      roi,
                                                                      pm1_s, pm1_d,
                                                                      pm2_s, pm2_d,
                                                                      pm3_s, pm3_d,
                                                                      next_depth,
                                                                      coords);

      gimp_operation_cage_transform_interpolate_source_coords_recurs (oct,
                                                                      out_buf,
                                                                      roi,
                                                                      pm3_s, pm3_d,
                                                                      pm2_s, pm2_d,
                                                                      p3_s, p3_d,
                                                                      next_depth,
                                                                      coords);
    }
}

static GimpVector2
gimp_cage_transform_compute_destination (GimpCageConfig *config,
                                         gfloat         *coef,
                                         GeglSampler    *coef_sampler,
                                         GimpVector2     coords)
{
  GimpVector2    result = {0, 0};
  gint           n_cage_vertices = gimp_cage_config_get_n_points (config);
  gint           i;
  GimpCagePoint *point;

  gegl_sampler_get (coef_sampler,
                    coords.x, coords.y, NULL, coef, GEGL_ABYSS_NONE);

  for (i = 0; i < n_cage_vertices; i++)
    {
      point = &g_array_index (config->cage_points, GimpCagePoint, i);

      result.x += coef[i] * point->dest_point.x;
      result.y += coef[i] * point->dest_point.y;

      result.x += coef[i + n_cage_vertices] * point->edge_scaling_factor * point->edge_normal.x;
      result.y += coef[i + n_cage_vertices] * point->edge_scaling_factor * point->edge_normal.y;
    }

  return result;
}

GeglRectangle
gimp_operation_cage_transform_get_cached_region (GeglOperation       *operation,
                                                 const GeglRectangle *roi)
{
  GeglRectangle result = *gegl_operation_source_get_bounding_box (operation,
                                                                  "input");

  return result;
}

GeglRectangle
gimp_operation_cage_transform_get_required_for_output (GeglOperation       *operation,
                                                       const gchar         *input_pad,
                                                       const GeglRectangle *roi)
{
  GeglRectangle result = *gegl_operation_source_get_bounding_box (operation,
                                                                  "input");

  return result;
}

GeglRectangle
gimp_operation_cage_transform_get_bounding_box (GeglOperation *operation)
{
  GeglRectangle result = *gegl_operation_source_get_bounding_box (operation,
                                                                  "input");

  return result;
}
