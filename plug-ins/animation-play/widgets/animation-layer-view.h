/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * animation-layer_view.h
 * Copyright (C) 2015 Jehan <jehan@gimp.org>
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

#ifndef __ANIMATION_LAYER_VIEW_H__
#define __ANIMATION_LAYER_VIEW_H__

#define ANIMATION_TYPE_LAYER_VIEW            (animation_layer_view_get_type ())
#define ANIMATION_LAYER_VIEW(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), ANIMATION_TYPE_LAYER_VIEW, AnimationLayerView))
#define ANIMATION_LAYER_VIEW_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), ANIMATION_TYPE_LAYER_VIEW, AnimationLayerViewClass))
#define ANIMATION_IS_LAYER_VIEW(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), ANIMATION_TYPE_LAYER_VIEW))
#define ANIMATION_IS_LAYER_VIEW_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), ANIMATION_TYPE_LAYER_VIEW))
#define ANIMATION_LAYER_VIEW_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), ANIMATION_TYPE_LAYER_VIEW, AnimationLayerViewClass))

typedef struct _AnimationLayerView        AnimationLayerView;
typedef struct _AnimationLayerViewClass   AnimationLayerViewClass;
typedef struct _AnimationLayerViewPrivate AnimationLayerViewPrivate;

struct _AnimationLayerView
{
  GtkVBox                    parent_instance;

  AnimationLayerViewPrivate *priv;
};

struct _AnimationLayerViewClass
{
  GtkVBoxClass  parent_class;
};

GType       animation_layer_view_get_type (void) G_GNUC_CONST;

GtkWidget * animation_layer_view_new      (gint32              image_id);

void        animation_layer_view_refresh  (AnimationLayerView *view);

void        animation_layer_view_filter   (AnimationLayerView *view,
                                           const gchar        *filter);
void        animation_layer_view_select   (AnimationLayerView *view,
                                           const GList        *layers);
#endif  /*  __ANIMATION_LAYER_VIEW_H__  */

