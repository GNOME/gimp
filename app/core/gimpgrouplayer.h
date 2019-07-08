/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * GimpGroupLayer
 * Copyright (C) 2009  Michael Natterer <mitch@gimp.org>
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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef __GIMP_GROUP_LAYER_H__
#define __GIMP_GROUP_LAYER_H__


#include "core/gimplayer.h"


#define GIMP_TYPE_GROUP_LAYER            (gimp_group_layer_get_type ())
#define GIMP_GROUP_LAYER(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_GROUP_LAYER, GimpGroupLayer))
#define GIMP_GROUP_LAYER_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_GROUP_LAYER, GimpGroupLayerClass))
#define GIMP_IS_GROUP_LAYER(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_GROUP_LAYER))
#define GIMP_IS_GROUP_LAYER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_GROUP_LAYER))
#define GIMP_GROUP_LAYER_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_GROUP_LAYER, GimpGroupLayerClass))


typedef struct _GimpGroupLayerClass GimpGroupLayerClass;

struct _GimpGroupLayer
{
  GimpLayer  parent_instance;
};

struct _GimpGroupLayerClass
{
  GimpLayerClass  parent_class;
};


GType            gimp_group_layer_get_type            (void) G_GNUC_CONST;

GimpLayer      * gimp_group_layer_new                 (GimpImage           *image);

GimpProjection * gimp_group_layer_get_projection      (GimpGroupLayer      *group);

void             gimp_group_layer_suspend_resize      (GimpGroupLayer      *group,
                                                       gboolean             push_undo);
void             gimp_group_layer_resume_resize       (GimpGroupLayer      *group,
                                                       gboolean             push_undo);

void             gimp_group_layer_suspend_mask        (GimpGroupLayer      *group,
                                                       gboolean             push_undo);
void             gimp_group_layer_resume_mask         (GimpGroupLayer      *group,
                                                       gboolean             push_undo);


void             _gimp_group_layer_set_suspended_mask (GimpGroupLayer      *group,
                                                       GeglBuffer          *buffer,
                                                       const GeglRectangle *bounds);
GeglBuffer     * _gimp_group_layer_get_suspended_mask (GimpGroupLayer      *group,
                                                       GeglRectangle       *bounds);

void             _gimp_group_layer_start_transform    (GimpGroupLayer      *group,
                                                       gboolean             push_undo);
void             _gimp_group_layer_end_transform      (GimpGroupLayer      *group,
                                                       gboolean             push_undo);


#endif /* __GIMP_GROUP_LAYER_H__ */
