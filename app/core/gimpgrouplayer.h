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

#pragma once

#include "gimplayer.h"


#define GIMP_TYPE_GROUP_LAYER (gimp_group_layer_get_type ())
G_DECLARE_DERIVABLE_TYPE (GimpGroupLayer,
                          gimp_group_layer,
                          GIMP, GROUP_LAYER,
                          GimpLayer)


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
