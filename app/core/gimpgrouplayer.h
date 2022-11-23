/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * LigmaGroupLayer
 * Copyright (C) 2009  Michael Natterer <mitch@ligma.org>
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

#ifndef __LIGMA_GROUP_LAYER_H__
#define __LIGMA_GROUP_LAYER_H__


#include "core/ligmalayer.h"


#define LIGMA_TYPE_GROUP_LAYER            (ligma_group_layer_get_type ())
#define LIGMA_GROUP_LAYER(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), LIGMA_TYPE_GROUP_LAYER, LigmaGroupLayer))
#define LIGMA_GROUP_LAYER_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), LIGMA_TYPE_GROUP_LAYER, LigmaGroupLayerClass))
#define LIGMA_IS_GROUP_LAYER(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LIGMA_TYPE_GROUP_LAYER))
#define LIGMA_IS_GROUP_LAYER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), LIGMA_TYPE_GROUP_LAYER))
#define LIGMA_GROUP_LAYER_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), LIGMA_TYPE_GROUP_LAYER, LigmaGroupLayerClass))


typedef struct _LigmaGroupLayerClass LigmaGroupLayerClass;

struct _LigmaGroupLayer
{
  LigmaLayer  parent_instance;
};

struct _LigmaGroupLayerClass
{
  LigmaLayerClass  parent_class;
};


GType            ligma_group_layer_get_type            (void) G_GNUC_CONST;

LigmaLayer      * ligma_group_layer_new                 (LigmaImage           *image);

LigmaProjection * ligma_group_layer_get_projection      (LigmaGroupLayer      *group);

void             ligma_group_layer_suspend_resize      (LigmaGroupLayer      *group,
                                                       gboolean             push_undo);
void             ligma_group_layer_resume_resize       (LigmaGroupLayer      *group,
                                                       gboolean             push_undo);

void             ligma_group_layer_suspend_mask        (LigmaGroupLayer      *group,
                                                       gboolean             push_undo);
void             ligma_group_layer_resume_mask         (LigmaGroupLayer      *group,
                                                       gboolean             push_undo);


void             _ligma_group_layer_set_suspended_mask (LigmaGroupLayer      *group,
                                                       GeglBuffer          *buffer,
                                                       const GeglRectangle *bounds);
GeglBuffer     * _ligma_group_layer_get_suspended_mask (LigmaGroupLayer      *group,
                                                       GeglRectangle       *bounds);

void             _ligma_group_layer_start_transform    (LigmaGroupLayer      *group,
                                                       gboolean             push_undo);
void             _ligma_group_layer_end_transform      (LigmaGroupLayer      *group,
                                                       gboolean             push_undo);


#endif /* __LIGMA_GROUP_LAYER_H__ */
