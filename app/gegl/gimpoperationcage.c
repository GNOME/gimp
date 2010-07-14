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

#include "gimpoperationcage.h"


static gboolean       gimp_operation_cage_process                   (GeglOperation       *operation,
                                                                     GeglBuffer          *in_buf,
                                                                     GeglBuffer          *out_buf,
                                                                     const GeglRectangle *roi);
static void           gimp_operation_cage_prepare                   (GeglOperation       *operation);


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

  /* FIXME: wrong categories and name, to appears in the gegl tool */
  operation_class->name         = "gegl:cage";
  operation_class->categories   = "color";
  operation_class->description  = "GIMP cage transform";

  operation_class->prepare      = gimp_operation_cage_prepare;
  
  filter_class->process         = gimp_operation_cage_process;
}

static void
gimp_operation_cage_init (GimpOperationCage *self)
{
  //FIXME: for test
  self->cage = g_object_new (GIMP_TYPE_CAGE, NULL);
  self->deformedCage = g_object_new (GIMP_TYPE_CAGE, NULL);
  
  #if 0
  
    #if 1
    gimp_cage_add_cage_point(self->cage, 20, 20);
    gimp_cage_add_cage_point(self->cage, 50, 50);
    gimp_cage_add_cage_point(self->cage, 200, 80);
    gimp_cage_add_cage_point(self->cage, 70, 200);
    gimp_cage_add_cage_point(self->cage, 25, 80);
    #else
    gimp_cage_add_cage_point(self->cage, 25, 80);
    gimp_cage_add_cage_point(self->cage, 70, 200);
    gimp_cage_add_cage_point(self->cage, 200, 80);
    gimp_cage_add_cage_point(self->cage, 50, 50);
    gimp_cage_add_cage_point(self->cage, 20, 20);
    #endif
  
  #else
  
    #if 0
    gimp_cage_add_cage_point(self->cage, 20, 20); /* need reverse */
    gimp_cage_add_cage_point(self->cage, 50, 20);
    gimp_cage_add_cage_point(self->cage, 50, 60);
    gimp_cage_add_cage_point(self->cage, 80, 60);
    gimp_cage_add_cage_point(self->cage, 80, 20);
    gimp_cage_add_cage_point(self->cage, 110, 20);
    gimp_cage_add_cage_point(self->cage, 110, 120);
    gimp_cage_add_cage_point(self->cage, 20, 120);
    #else
    gimp_cage_add_cage_point(self->cage, 40, 240);
    gimp_cage_add_cage_point(self->cage, 220, 240);
    gimp_cage_add_cage_point(self->cage, 220, 40);
    gimp_cage_add_cage_point(self->cage, 160, 40);
    gimp_cage_add_cage_point(self->cage, 160, 120);
    gimp_cage_add_cage_point(self->cage, 100, 120);
    gimp_cage_add_cage_point(self->cage, 100, 40);
    gimp_cage_add_cage_point(self->cage, 40, 40);
    
    gimp_cage_add_cage_point(self->deformedCage, 40, 240);
    gimp_cage_add_cage_point(self->deformedCage, 320, 320);
    gimp_cage_add_cage_point(self->deformedCage, 220, 40);
    gimp_cage_add_cage_point(self->deformedCage, 160, 40);
    gimp_cage_add_cage_point(self->deformedCage, 160, 120);
    gimp_cage_add_cage_point(self->deformedCage, 100, 120);
    gimp_cage_add_cage_point(self->deformedCage, 100, 40);
    gimp_cage_add_cage_point(self->deformedCage, 40, 40);
    #endif
    
  #endif
}

static void
gimp_operation_cage_prepare (GeglOperation  *operation)
{
  gegl_operation_set_format (operation, "input", babl_format ("RGBA float"));
  gegl_operation_set_format (operation, "output", babl_format ("RGBA float"));
}

static gboolean
gimp_operation_cage_process (GeglOperation       *operation,
                             GeglBuffer          *in_buf,
                             GeglBuffer          *out_buf,
                             const GeglRectangle *roi)
{
  GimpOperationCage *op_cage  = GIMP_OPERATION_CAGE (operation);
  GimpCage *cage = op_cage->cage;
  GimpCage *deformedCage = op_cage->deformedCage;
  
  Babl *format_io = babl_format ("RGBA float");
  Babl *format_coef = babl_format_n (babl_type ("float"), op_cage->cage->cage_vertice_number);
  
  gint in, coef_vertices, coef_edges;
  gint i;
  GeglRectangle rect;
  GeglBufferIterator *it;
  
  rect.height = 1;
  rect.width = 1;
  
  cage->extent.height = roi->height;
  cage->extent.width = roi->width;
  cage->extent.x = roi->x;
  cage->extent.y = roi->y;
  gimp_cage_compute_coefficient (cage);
  
  it = gegl_buffer_iterator_new (in_buf, roi, format_io, GEGL_BUFFER_READ);
  in = 0;
  
  coef_vertices = gegl_buffer_iterator_add (it, cage->cage_vertices_coef, roi, format_coef, GEGL_BUFFER_READ);
  coef_edges = gegl_buffer_iterator_add (it, cage->cage_edges_coef, roi, format_coef, GEGL_BUFFER_READ);
  
  /* iterate on GeglBuffer */
  while (gegl_buffer_iterator_next (it))
  {
    /* iterate inside the roi */
    gint        n_pixels = it->length;
    gint        x = it->roi->x; /* initial x                   */
    gint        y = it->roi->y; /*           and y coordinates */
    
    gfloat      *source = it->data[in];
    gfloat      *coef_v = it->data[coef_vertices];
    gfloat      *coef_e = it->data[coef_edges];
    
    while(n_pixels--)
    {
      /* computing of the final position of the source pixel */
      gfloat pos_x, pos_y;
      
      pos_x = 0;
      pos_y = 0;
      
      for(i = 0; i < deformedCage->cage_vertice_number; i++)
      {
        pos_x += coef_v[i] * deformedCage->cage_vertices[i].x;
        pos_y += coef_v[i] * deformedCage->cage_vertices[i].y;
      }
      
      for(i = 0; i < deformedCage->cage_vertice_number; i++)
      {
        pos_x += coef_e[i] * gimp_cage_get_edge_normal (deformedCage, i).x;
        pos_y += coef_e[i] * gimp_cage_get_edge_normal (deformedCage, i).y;
      }
      
      rect.x = pos_x;
      rect.y = pos_y;      
      
      /* copy the source pixel in the out buffer */
      gegl_buffer_set(out_buf,
                      &rect,
                      format_io,
                      source,
                      GEGL_AUTO_ROWSTRIDE);
      
      source += 4;
      coef_v += op_cage->cage->cage_vertice_number;
      coef_e += op_cage->cage->cage_vertice_number;
      
      /* update x and y coordinates */
      x++;
      if (x >= (it->roi->x + it->roi->width))
      {
        x = it->roi->x;
        y++;
      }
      
    }
  }
  return TRUE;
}