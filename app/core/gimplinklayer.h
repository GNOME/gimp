/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * GimpLinkLayer
 * Copyright (C) 2019 Jehan
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


#define GIMP_TYPE_LINK_LAYER            (gimp_link_layer_get_type ())
#define GIMP_LINK_LAYER(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_LINK_LAYER, GimpLinkLayer))
#define GIMP_LINK_LAYER_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_LINK_LAYER, GimpLinkLayerClass))
#define GIMP_IS_LINK_LAYER(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_LINK_LAYER))
#define GIMP_IS_LINK_LAYER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_LINK_LAYER))
#define GIMP_LINK_LAYER_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_LINK_LAYER, GimpLinkLayerClass))


typedef struct _GimpLinkLayerClass   GimpLinkLayerClass;
typedef struct _GimpLinkLayerPrivate GimpLinkLayerPrivate;

struct _GimpLinkLayer
{
  GimpLayer             layer;

  GimpLinkLayerPrivate *p;
};

struct _GimpLinkLayerClass
{
  GimpLayerClass  parent_class;
};


GType       gimp_link_layer_get_type             (void) G_GNUC_CONST;

GimpLayer * gimp_link_layer_new                  (GimpImage             *image,
                                                  GimpLink              *link);

GimpLink  * gimp_link_layer_get_link             (GimpLinkLayer         *layer);
gboolean    gimp_link_layer_set_link             (GimpLinkLayer         *layer,
                                                  GimpLink              *link,
                                                  gboolean               push_undo);
gboolean    gimp_link_layer_set_link_with_matrix (GimpLinkLayer         *layer,
                                                  GimpLink              *link,
                                                  GimpMatrix3           *matrix,
                                                  GimpInterpolationType  interpolation_type,
                                                  gint                   offset_x,
                                                  gint                   offset_y,
                                                  gboolean               push_undo);

gboolean    gimp_link_layer_is_monitored         (GimpLinkLayer         *layer);

gboolean    gimp_link_layer_get_transform        (GimpLinkLayer         *layer,
                                                  GimpMatrix3           *matrix,
                                                  gint                  *offset_x,
                                                  gint                  *offset_y,
                                                  GimpInterpolationType *interpolation);
gboolean    gimp_link_layer_set_transform        (GimpLinkLayer         *layer,
                                                  GimpMatrix3           *matrix,
                                                  GimpInterpolationType  interpolation_type,
                                                  gboolean               push_undo);

gboolean    gimp_item_is_link_layer              (GimpItem              *item);

/* Only to be used for XCF loading/saving. */

guint32     gimp_link_layer_get_xcf_flags        (GimpLinkLayer         *layer);
void        gimp_link_layer_set_xcf_flags        (GimpLinkLayer         *layer,
                                                  guint32                flags);
