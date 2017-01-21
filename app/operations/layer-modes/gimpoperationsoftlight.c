/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpoperationsoftlightmode.c
 * Copyright (C) 2008 Michael Natterer <mitch@gimp.org>
 *               2012 Ville Sokk <ville.sokk@gmail.com>
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

#include <gegl-plugin.h>

#include "../operations-types.h"

#include "gimpoperationsoftlight.h"
#include "gimpblendcomposite.h"


G_DEFINE_TYPE (GimpOperationSoftlight, gimp_operation_softlight,
               GIMP_TYPE_OPERATION_LAYER_MODE)


static const gchar* reference_xml = "<?xml version='1.0' encoding='UTF-8'?>"
"<gegl>"
"<node operation='gimp:softlight-mode'>"
"  <node operation='gegl:load'>"
"    <params>"
"      <param name='path'>B.png</param>"
"    </params>"
"  </node>"
"</node>"
"<node operation='gegl:load'>"
"  <params>"
"    <param name='path'>A.png</param>"
"  </params>"
"</node>"
"</gegl>";


static void
gimp_operation_softlight_class_init (GimpOperationSoftlightClass *klass)
{
  GeglOperationClass               *operation_class;
  GeglOperationPointComposer3Class *point_class;

  operation_class = GEGL_OPERATION_CLASS (klass);
  point_class     = GEGL_OPERATION_POINT_COMPOSER3_CLASS (klass);

  gegl_operation_class_set_keys (operation_class,
                                 "name",        "gimp:softlight",
                                 "description", "GIMP softlight mode operation",
                                 "reference-image", "soft-light-mode.png",
                                 "reference-composition", reference_xml,
                                 NULL);

  point_class->process = gimp_operation_softlight_process;
}

static void
gimp_operation_softlight_init (GimpOperationSoftlight *self)
{
}

gboolean
gimp_operation_softlight_process (GeglOperation       *op,
                                  void                *in,
                                  void                *layer,
                                  void                *mask,
                                  void                *out,
                                  glong                samples,
                                  const GeglRectangle *roi,
                                  gint                 level)
{
  gimp_composite_blend (op, in, layer, mask, out, samples,
                        blendfun_softlight);
  return TRUE;
}
