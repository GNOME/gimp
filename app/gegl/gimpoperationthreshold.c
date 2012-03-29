/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpoperationthreshold.c
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
#include <gegl.h>

#include "libgimpcolor/gimpcolor.h"

#include "gimp-gegl-types.h"

#include "gimpoperationthreshold.h"
#include "gimpthresholdconfig.h"


static gboolean gimp_operation_threshold_process (GeglOperation       *operation,
                                                  void                *in_buf,
                                                  void                *out_buf,
                                                  glong                samples,
                                                  const GeglRectangle *roi,
                                                  gint                 level);


G_DEFINE_TYPE (GimpOperationThreshold, gimp_operation_threshold,
               GIMP_TYPE_OPERATION_POINT_FILTER)

#define parent_class gimp_operation_threshold_parent_class


static void
gimp_operation_threshold_class_init (GimpOperationThresholdClass *klass)
{
  GObjectClass                  *object_class    = G_OBJECT_CLASS (klass);
  GeglOperationClass            *operation_class = GEGL_OPERATION_CLASS (klass);
  GeglOperationPointFilterClass *point_class     = GEGL_OPERATION_POINT_FILTER_CLASS (klass);

  object_class->set_property   = gimp_operation_point_filter_set_property;
  object_class->get_property   = gimp_operation_point_filter_get_property;

  gegl_operation_class_set_keys (operation_class,
           "name"       , "gimp:threshold",
           "categories" , "color",
           "description", "GIMP Threshold operation",
           NULL);

  point_class->process         = gimp_operation_threshold_process;

  g_object_class_install_property (object_class,
                                   GIMP_OPERATION_POINT_FILTER_PROP_CONFIG,
                                   g_param_spec_object ("config",
                                                        "Config",
                                                        "The config object",
                                                        GIMP_TYPE_THRESHOLD_CONFIG,
                                                        G_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT));
}

static void
gimp_operation_threshold_init (GimpOperationThreshold *self)
{
}

static gboolean
gimp_operation_threshold_process (GeglOperation       *operation,
                                  void                *in_buf,
                                  void                *out_buf,
                                  glong                samples,
                                  const GeglRectangle *roi,
                                  gint                 level)
{
  GimpOperationPointFilter *point  = GIMP_OPERATION_POINT_FILTER (operation);
  GimpThresholdConfig      *config = GIMP_THRESHOLD_CONFIG (point->config);
  gfloat                   *src    = in_buf;
  gfloat                   *dest   = out_buf;

  if (! config)
    return FALSE;

  while (samples--)
    {
      gfloat value;

      value = MAX (src[RED], src[GREEN]);
      value = MAX (value, src[BLUE]);

      value = (value >= config->low && value <= config->high) ? 1.0 : 0.0;

      dest[RED]   = value;
      dest[GREEN] = value;
      dest[BLUE]  = value;
      dest[ALPHA] = src[ALPHA];

      src  += 4;
      dest += 4;
    }

  return TRUE;
}
