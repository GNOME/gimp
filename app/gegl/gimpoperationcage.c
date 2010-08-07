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
#include "gimpcageconfig.h"


static void           gimp_operation_cage_finalize                  (GObject              *object);
static void           gimp_operation_cage_get_property              (GObject              *object,
                                                                     guint                 property_id,
                                                                     GValue               *value,
                                                                     GParamSpec           *pspec);
static void           gimp_operation_cage_set_property              (GObject              *object,
                                                                     guint                 property_id,
                                                                     const GValue         *value,
                                                                     GParamSpec           *pspec);
static void           gimp_operation_cage_prepare                   (GeglOperation       *operation);
static gboolean       gimp_operation_cage_process                   (GeglOperation       *operation,
                                                                     GeglBuffer          *in_buf,
                                                                     GeglBuffer          *aux_buf,
                                                                     GeglBuffer          *out_buf,
                                                                     const GeglRectangle *roi);
GeglRectangle         gimp_operation_cage_get_cached_region         (GeglOperation        *operation,
                                                                     const GeglRectangle *roi);
GeglRectangle         gimp_operation_cage_get_required_for_output   (GeglOperation        *operation,
                                                                     const gchar         *input_pad,
                                                                     const GeglRectangle *roi);
                                        
G_DEFINE_TYPE (GimpOperationCage, gimp_operation_cage,
               GEGL_TYPE_OPERATION_COMPOSER)

#define parent_class gimp_operation_cage_parent_class


static void
gimp_operation_cage_class_init (GimpOperationCageClass *klass)
{
  GObjectClass                    *object_class     = G_OBJECT_CLASS (klass);
  GeglOperationClass              *operation_class  = GEGL_OPERATION_CLASS (klass);
  GeglOperationComposerClass      *composer_class   = GEGL_OPERATION_COMPOSER_CLASS (klass);

  object_class->get_property          = gimp_operation_cage_get_property;
  object_class->set_property          = gimp_operation_cage_set_property;
  object_class->finalize              = gimp_operation_cage_finalize;

  /* FIXME: wrong categories and name, to appears in the gegl tool */
  operation_class->name         = "gegl:cage";
  operation_class->categories   = "color";
  operation_class->description  = "GIMP cage transform";

  operation_class->prepare      = gimp_operation_cage_prepare;
  
  operation_class->get_required_for_output = gimp_operation_cage_get_required_for_output;
  operation_class->get_cached_region = gimp_operation_cage_get_cached_region;
  operation_class->no_cache     = FALSE;
  
  composer_class->process         = gimp_operation_cage_process;
  
  g_object_class_install_property (object_class,
                                 GIMP_OPERATION_CAGE_PROP_CONFIG,
                                 g_param_spec_object ("config", NULL, NULL,
                                                      GIMP_TYPE_CAGE_CONFIG,
                                                      G_PARAM_READWRITE |
                                                      G_PARAM_CONSTRUCT));
}

static void
gimp_operation_cage_init (GimpOperationCage *self)
{

}

static void
gimp_operation_cage_finalize  (GObject  *object)
{
  GimpOperationCage *self = GIMP_OPERATION_CAGE (object);

  if (self->config)
    {
      g_object_unref (self->config);
      self->config = NULL;
    }

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
gimp_operation_cage_get_property  (GObject      *object,
                                   guint         property_id,
                                   GValue       *value,
                                   GParamSpec   *pspec)
{
  GimpOperationCage   *self   = GIMP_OPERATION_CAGE (object);

  switch (property_id)
    {
    case GIMP_OPERATION_CAGE_PROP_CONFIG:
      g_value_set_object (value, self->config);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_operation_cage_set_property  (GObject        *object,
                                   guint           property_id,
                                   const GValue   *value,
                                   GParamSpec     *pspec)
{
  GimpOperationCage   *self   = GIMP_OPERATION_CAGE (object);

  switch (property_id)
    {
    case GIMP_OPERATION_CAGE_PROP_CONFIG:
      if (self->config)
        g_object_unref (self->config);
      self->config = g_value_dup_object (value);
      break;

   default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
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
                             GeglBuffer          *aux_buf,
                             GeglBuffer          *out_buf,
                             const GeglRectangle *roi)
{
  GimpOperationCage   *oc     = GIMP_OPERATION_CAGE (operation);
  GimpCageConfig      *config = GIMP_CAGE_CONFIG (oc->config);
  
  Babl *format_io = babl_format ("RGBA float");
  Babl *format_coef = babl_format_n (babl_type ("float"), 2 * config->cage_vertice_number);
  
  gint in_index, coef_index;
  gint i;
  
  GeglRectangle rect, bb_cage;
  GeglBufferIterator *it;
  
  rect.height = 1;
  rect.width = 1;
  
  bb_cage = gimp_cage_config_get_bounding_box (config);
  
  it = gegl_buffer_iterator_new (in_buf, &bb_cage, format_io, GEGL_BUFFER_READ);
  in_index = 0;
  
  coef_index = gegl_buffer_iterator_add (it, aux_buf, &bb_cage, format_coef, GEGL_BUFFER_READ);
  
  /* pre-copy the input buffer to the out buffer */
  //gegl_buffer_copy (aux_buf, roi, out_buf, roi);
  
  /* iterate on GeglBuffer */
  while (gegl_buffer_iterator_next (it))
  {
    /* iterate inside the roi */
    gint        n_pixels = it->length;
    gint        x = it->roi->x; /* initial x                   */
    gint        y = it->roi->y; /*           and y coordinates */
    gint        cvn = config->cage_vertice_number;
    
    gfloat      *source = it->data[in_index];
    gfloat      *coef = it->data[coef_index];
    
    while(n_pixels--)
    {
      /* computing of the final position of the source pixel */
      gdouble pos_x, pos_y;
      
      pos_x = 0;
      pos_y = 0;
      
      for(i = 0; i < cvn; i++)
      {
        pos_x += coef[i] * config->cage_vertices_d[i].x;
        pos_y += coef[i] * config->cage_vertices_d[i].y;
      }
      
      for(i = 0; i < cvn; i++)
      {
        pos_x += coef[i + cvn] * config->scaling_factor[i] * gimp_cage_config_get_edge_normal (config, i).x;
        pos_y += coef[i + cvn] * config->scaling_factor[i] * gimp_cage_config_get_edge_normal (config, i).y;
      }
      
      rect.x = (gint) rint(pos_x);
      rect.y = (gint) rint(pos_y);
      
      /*if (sqrt ((pos_x-x) * (pos_x-x) + (pos_y-y) * (pos_y-y)) < 20)
      {
        if (rect.x != x || rect.y != y)
        {
          printf("x: %d    y: %d\n", rect.x, rect.y);
        }
      }*/
      
      /* copy the source pixel in the out buffer */
      gegl_buffer_set(out_buf,
                      &rect,
                      format_io,
                      source,
                      GEGL_AUTO_ROWSTRIDE);
                      
      source += 4;
      coef += 2 * cvn;
      
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

GeglRectangle
gimp_operation_cage_get_cached_region  (GeglOperation        *operation,
                                        const GeglRectangle *roi)
{
  GeglRectangle result = *gegl_operation_source_get_bounding_box (operation, "input");
  
  return result;
}

GeglRectangle
gimp_operation_cage_get_required_for_output (GeglOperation        *operation,
                                              const gchar         *input_pad,
                                              const GeglRectangle *roi)
{
  GeglRectangle result = *gegl_operation_source_get_bounding_box (operation, "input");
  
  return result;
}
