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
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <gegl.h>

#include "libgimpcolor/gimpcolor.h"
#include "libgimpmath/gimpmath.h"

#include "gimp-gegl-types.h"

#include "core/gimpcage.h"

#include "gimpoperationcage.h"


static gboolean gimp_operation_cage_process (GeglOperation       *operation,
                                               void                *in_buf,
                                               void                *out_buf,
                                               const GeglRectangle *roi);

G_DEFINE_TYPE (GimpOperationCage, gimp_operation_cage,
               GEGL_TYPE_OPERATION_FILTER)

#define parent_class gimp_operation_cage_parent_class


static void
gimp_operation_cage_class_init (GimpOperationCageClass *klass)
{
  GeglOperationClass              *operation_class;
  GeglOperationFilterClass        *filter_class;

  operation_class = GEGL_OPERATION_CLASS (klass);
  filter_class = GEGL_OPERATION_FILTER_CLASS (klass);

  operation_class->name        = "gimp:cage";
  operation_class->description = "GIMP cage transform";

  filter_class->process         = gimp_operation_cage_process;
}

static void
gimp_operation_cage_init (GimpOperationCage *self)
{
}

static gboolean
gimp_operation_cage_process (GeglOperation       *operation,
                               void                *in_buf,
                               void                *out_buf,
                               const GeglRectangle *roi)
{
  gfloat *in    = in_buf;
  gfloat *out   = out_buf;

  /*while (samples--)
    {
      out[RED]   = in[RED];
      out[GREEN] = in[GREEN];
      out[BLUE]  = in[BLUE];
      out[ALPHA] = in[ALPHA];

      in    += 4;
      out   += 4;
    }*/

  return TRUE;
}
