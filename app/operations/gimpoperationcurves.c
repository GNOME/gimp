/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpoperationcurves.c
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
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <cairo.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gegl.h>

#include "libgimpmath/gimpmath.h"

#include "operations-types.h"

#include "core/gimpcurve.h"
#include "core/gimpcurve-map.h"

#include "gimpcurvesconfig.h"
#include "gimpoperationcurves.h"


static gboolean gimp_operation_curves_process (GeglOperation       *operation,
                                               void                *in_buf,
                                               void                *out_buf,
                                               glong                samples,
                                               const GeglRectangle *roi,
                                               gint                 level);

static void gimp_operation_curves_prepare (GeglOperation *operation);

G_DEFINE_TYPE (GimpOperationCurves, gimp_operation_curves,
               GIMP_TYPE_OPERATION_POINT_FILTER)

#define parent_class gimp_operation_curves_parent_class


static void
gimp_operation_curves_class_init (GimpOperationCurvesClass *klass)
{
  GObjectClass                  *object_class    = G_OBJECT_CLASS (klass);
  GeglOperationClass            *operation_class = GEGL_OPERATION_CLASS (klass);
  GeglOperationPointFilterClass *point_class     = GEGL_OPERATION_POINT_FILTER_CLASS (klass);

  object_class->set_property   = gimp_operation_point_filter_set_property;
  object_class->get_property   = gimp_operation_point_filter_get_property;

  gegl_operation_class_set_keys (operation_class,
                                 "name",        "gimp:curves",
                                 "categories",  "color",
                                 "description", "GIMP Curves operation",
                                 NULL);

  point_class->process = gimp_operation_curves_process;
  operation_class->prepare = gimp_operation_curves_prepare;

  g_object_class_install_property (object_class,
                                   GIMP_OPERATION_POINT_FILTER_PROP_CONFIG,
                                   g_param_spec_object ("config",
                                                        "Config",
                                                        "The config object",
                                                        GIMP_TYPE_CURVES_CONFIG,
                                                        G_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT));
}

static void
gimp_operation_curves_prepare (GeglOperation *operation)
{
  const Babl *format = babl_format ("RGBA float");

  gegl_operation_set_format (operation, "input",  format);
  gegl_operation_set_format (operation, "output", format);
}

static void
gimp_operation_curves_init (GimpOperationCurves *self)
{
}

static gboolean
gimp_operation_curves_process (GeglOperation       *operation,
                               void                *in_buf,
                               void                *out_buf,
                               glong                samples,
                               const GeglRectangle *roi,
                               gint                 level)
{
  GimpOperationPointFilter *point  = GIMP_OPERATION_POINT_FILTER (operation);
  GimpCurvesConfig         *config = GIMP_CURVES_CONFIG (point->config);
  gfloat                   *src    = in_buf;
  gfloat                   *dest   = out_buf;

  if (! config)
    return FALSE;

  gimp_curve_map_pixels (config->curve[0],
                         config->curve[1],
                         config->curve[2],
                         config->curve[3],
                         config->curve[4], src, dest, samples);

  return TRUE;
}
