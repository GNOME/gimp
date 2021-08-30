/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * GimpVectorLayer
 * Copyright (C) 2006 Hendrik Boom
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
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

#ifndef __GIMP_VECTOR_LAYER_H__
#define __GIMP_VECTOR_LAYER_H__


#include "core/gimplayer.h"


#define GIMP_TYPE_VECTOR_LAYER            (gimp_vector_layer_get_type ())
#define GIMP_VECTOR_LAYER(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_VECTOR_LAYER, GimpVectorLayer))
#define GIMP_VECTOR_LAYER_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_VECTOR_LAYER, GimpVectorLayerClass))
#define GIMP_IS_VECTOR_LAYER(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_VECTOR_LAYER))
#define GIMP_IS_VECTOR_LAYER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_VECTOR_LAYER))
#define GIMP_VECTOR_LAYER_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_VECTOR_LAYER, GimpVectorLayerClass))


typedef struct _GimpVectorLayerClass GimpVectorLayerClass;

struct _GimpVectorLayer
{
  GimpLayer               parent_instance;

  GimpVectorLayerOptions *options;
  const gchar            *parasite;
};

struct _GimpVectorLayerClass
{
  GimpLayerClass  parent_class;
};


GType             gimp_vector_layer_get_type    (void) G_GNUC_CONST;

GimpVectorLayer * gimp_vector_layer_new         (GimpImage       *image,
                                                 GimpVectors     *vectors,
                                                 GimpContext     *context);
void              gimp_vector_layer_refresh     (GimpVectorLayer *layer);
void              gimp_vector_layer_discard     (GimpVectorLayer *layer);

gboolean          gimp_item_is_vector_layer     (GimpItem        *item);


#endif /* __GIMP_VECTOR_LAYER_H__ */
