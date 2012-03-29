/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpoperationdesaturate.c
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

#include "gimpoperationdesaturate.h"
#include "gimpdesaturateconfig.h"


static gboolean  gimp_operation_desaturate_process (GeglOperation       *operation,
                                                    void                *in_buf,
                                                    void                *out_buf,
                                                    glong                samples,
                                                    const GeglRectangle *roi,
                                                    gint                 level);


G_DEFINE_TYPE (GimpOperationDesaturate, gimp_operation_desaturate,
               GIMP_TYPE_OPERATION_POINT_FILTER)

#define parent_class gimp_operation_desaturate_parent_class


static void
gimp_operation_desaturate_class_init (GimpOperationDesaturateClass *klass)
{
  GObjectClass                  *object_class    = G_OBJECT_CLASS (klass);
  GeglOperationClass            *operation_class = GEGL_OPERATION_CLASS (klass);
  GeglOperationPointFilterClass *point_class     = GEGL_OPERATION_POINT_FILTER_CLASS (klass);

  object_class->set_property   = gimp_operation_point_filter_set_property;
  object_class->get_property   = gimp_operation_point_filter_get_property;

  gegl_operation_class_set_keys (operation_class,
           "name"       , "gimp:desaturate",
           "categories" , "color",
           "description", "GIMP Desaturate operation",
           NULL);

  point_class->process         = gimp_operation_desaturate_process;

  g_object_class_install_property (object_class,
                                   GIMP_OPERATION_POINT_FILTER_PROP_CONFIG,
                                   g_param_spec_object ("config",
                                                        "Config",
                                                        "The config object",
                                                        GIMP_TYPE_DESATURATE_CONFIG,
                                                        G_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT));
}

static void
gimp_operation_desaturate_init (GimpOperationDesaturate *self)
{
}

static gboolean
gimp_operation_desaturate_process (GeglOperation       *operation,
                                   void                *in_buf,
                                   void                *out_buf,
                                   glong                samples,
                                   const GeglRectangle *roi,
                                   gint                 level)
{
  GimpOperationPointFilter *point  = GIMP_OPERATION_POINT_FILTER (operation);
  GimpDesaturateConfig     *config = GIMP_DESATURATE_CONFIG (point->config);
  gfloat                   *src    = in_buf;
  gfloat                   *dest   = out_buf;

  if (! config)
    return FALSE;

  switch (config->mode)
    {
    case GIMP_DESATURATE_LIGHTNESS:
      while (samples--)
        {
          gfloat min, max, value;

          max = MAX (src[0], src[1]);
          max = MAX (max, src[2]);
          min = MIN (src[0], src[1]);
          min = MIN (min, src[2]);

          value = (max + min) / 2;

          dest[0] = value;
          dest[1] = value;
          dest[2] = value;
          dest[3] = src[3];

          src  += 4;
          dest += 4;
        }
      break;

    case GIMP_DESATURATE_LUMINOSITY:
      while (samples--)
        {
          gfloat value = GIMP_RGB_LUMINANCE (src[0], src[1], src[2]);

          dest[0] = value;
          dest[1] = value;
          dest[2] = value;
          dest[3] = src[3];

          src  += 4;
          dest += 4;
        }
      break;

    case GIMP_DESATURATE_AVERAGE:
      while (samples--)
        {
          gfloat value = (src[0] + src[1] + src[2]) / 3;

          dest[0] = value;
          dest[1] = value;
          dest[2] = value;
          dest[3] = src[3];

          src  += 4;
          dest += 4;
        }
      break;
    }

  return TRUE;
}
