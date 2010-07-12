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
#include "gimp-gegl-types.h"
#include <gegl-buffer-iterator.h>

#include "libgimpcolor/gimpcolor.h"
#include "libgimpmath/gimpmath.h"


#include "core/gimpcage.h"

#include "gimpoperationcage.h"


static gboolean gimp_operation_cage_process (GeglOperation       *operation,
                                             GeglBuffer          *in_buf,
                                             GeglBuffer          *out_buf,
                                             const GeglRectangle *roi);


G_DEFINE_TYPE (GimpOperationCage, gimp_operation_cage,
               GEGL_TYPE_OPERATION_FILTER)

//#define parent_class gimp_operation_cage_parent_class


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
                             GeglBuffer          *in_buf,
                             GeglBuffer          *out_buf,
                             const GeglRectangle *roi)
{
  GeglBufferIterator *i;
  Babl *format = babl_format ("RaGaBaA float");
  
  i = gegl_buffer_iterator_new (out_buf, roi, format, GEGL_BUFFER_WRITE);
  
  //iterate on GeglBuffer
  while (gegl_buffer_iterator_next (i))
  {
    //iterate inside the roi
    gint        n_pixels = i->length;
    gint        x = i->roi->x; /* initial x                   */
    gint        y = i->roi->y; /*           and y coordinates */
    
    while(n_pixels--)
    {
      gfloat *buf = i->data[0];
      buf[0] = 0.5;
      buf[1] = 0.5;
      buf[2] = 0.5;
      buf[3] = 0.5;
      
      /* update x and y coordinates */
      x++;
      if (x >= (i->roi->x + i->roi->width))
      {
        x = i->roi->x;
        y++;
      }
      
    }
  }
  return TRUE;
}
