/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * animation-keyframe_view.h
 * Copyright (C) 2016-2017 Jehan <jehan@gimp.org>
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

#ifndef __ANIMATION_KEYFRAME_VIEW_H__
#define __ANIMATION_KEYFRAME_VIEW_H__

#define ANIMATION_TYPE_KEYFRAME_VIEW            (animation_keyframe_view_get_type ())
#define ANIMATION_KEYFRAME_VIEW(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), ANIMATION_TYPE_KEYFRAME_VIEW, AnimationKeyFrameView))
#define ANIMATION_KEYFRAME_VIEW_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), ANIMATION_TYPE_KEYFRAME_VIEW, AnimationKeyFrameViewClass))
#define ANIMATION_IS_KEYFRAME_VIEW(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), ANIMATION_TYPE_KEYFRAME_VIEW))
#define ANIMATION_IS_KEYFRAME_VIEW_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), ANIMATION_TYPE_KEYFRAME_VIEW))
#define ANIMATION_KEYFRAME_VIEW_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), ANIMATION_TYPE_KEYFRAME_VIEW, AnimationKeyFrameViewClass))

typedef struct _AnimationKeyFrameView        AnimationKeyFrameView;
typedef struct _AnimationKeyFrameViewClass   AnimationKeyFrameViewClass;
typedef struct _AnimationKeyFrameViewPrivate AnimationKeyFrameViewPrivate;

struct _AnimationKeyFrameView
{
  GtkNotebook                   parent_instance;

  AnimationKeyFrameViewPrivate *priv;
};

struct _AnimationKeyFrameViewClass
{
  GtkNotebookClass              parent_class;
};

GType       animation_keyframe_view_get_type (void) G_GNUC_CONST;

GtkWidget * animation_keyframe_view_new      (void);

void        animation_keyframe_view_show     (AnimationKeyFrameView *view,
                                              AnimationCelAnimation *animation,
                                              gint                   position);
void        animation_keyframe_view_hide     (AnimationKeyFrameView *view);

#endif  /*  __ANIMATION_KEYFRAME_VIEW_H__  */
