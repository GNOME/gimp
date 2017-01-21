/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpoperationlchhue.c
 * Copyright (C) 2015 Elle Stone <ellestone@ninedegreesbelow.com>
 *                    Massimo Valentini <mvalentini@src.gnome.org>
 *                    Thomas Manni <thomas.manni@free.fr>
 *               2017 Øyvind Kolås <pippin@gimp.org>
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

#include "gimpoperationlchhue.h"
#include "gimpblendcomposite.h"

G_DEFINE_TYPE (GimpOperationLchHue, gimp_operation_lch_hue,
               GIMP_TYPE_OPERATION_POINT_LAYER_MODE)

#define parent_class gimp_operation_lch_hue_parent_class


static void
gimp_operation_lch_hue_class_init (GimpOperationLchHueClass *klass)
{
  GeglOperationClass               *operation_class;
  GeglOperationPointComposer3Class *point_class;

  operation_class = GEGL_OPERATION_CLASS (klass);
  point_class     = GEGL_OPERATION_POINT_COMPOSER3_CLASS (klass);

  operation_class->want_in_place = FALSE;

  gegl_operation_class_set_keys (operation_class,
                                 "name",        "gimp:lch-hue",
                                 "description", "GIMP LCH hue mode operation",
                                 NULL);

  point_class->process = gimp_operation_lch_hue_process;
}

static void
gimp_operation_lch_hue_init (GimpOperationLchHue *self)
{
}

gboolean
gimp_operation_lch_hue_process (GeglOperation       *op,
                                void                *in,
                                void                *layer,
                                void                *mask,
                                void                *out,
                                glong                samples,
                                const GeglRectangle *roi,
                                gint                 level)
{
  gimp_composite_blend (op, in, layer, mask, out, samples,
                        blendfun_lch_hue);
  return TRUE;
}
