/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * animation-layer_view.h
 * Copyright (C) 2016 Jehan <jehan@gimp.org>
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

#ifndef __ANIMATION_STORYBOARD_H__
#define __ANIMATION_STORYBOARD_H__

#define ANIMATION_TYPE_STORYBOARD            (animation_storyboard_get_type ())
#define ANIMATION_STORYBOARD(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), ANIMATION_TYPE_STORYBOARD, AnimationStoryboard))
#define ANIMATION_STORYBOARD_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), ANIMATION_TYPE_STORYBOARD, AnimationStoryboardClass))
#define ANIMATION_IS_STORYBOARD(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), ANIMATION_TYPE_STORYBOARD))
#define ANIMATION_IS_STORYBOARD_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), ANIMATION_TYPE_STORYBOARD))
#define ANIMATION_STORYBOARD_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), ANIMATION_TYPE_STORYBOARD, AnimationStoryboardClass))

typedef struct _AnimationStoryboard        AnimationStoryboard;
typedef struct _AnimationStoryboardClass   AnimationStoryboardClass;
typedef struct _AnimationStoryboardPrivate AnimationStoryboardPrivate;

struct _AnimationStoryboard
{
  GtkScrolledWindow           parent_instance;

  AnimationStoryboardPrivate *priv;
};

struct _AnimationStoryboardClass
{
  GtkScrolledWindowClass      parent_class;
};

GType       animation_storyboard_get_type (void) G_GNUC_CONST;

GtkWidget * animation_storyboard_new      (AnimationAnimatic *animation,
                                           AnimationPlayback *playback);

#endif  /*  __ANIMATION_STORYBOARD_H__  */

