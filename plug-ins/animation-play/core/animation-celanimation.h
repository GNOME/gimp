/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * animation.h
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

#ifndef __ANIMATION_CEL_ANIMATION_H__
#define __ANIMATION_CEL_ANIMATION_H__

#include "animation.h"

#define ANIMATION_TYPE_CEL_ANIMATION            (animation_cel_animation_get_type ())
#define ANIMATION_CEL_ANIMATION(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), ANIMATION_TYPE_CEL_ANIMATION, AnimationCelAnimation))
#define ANIMATION_CEL_ANIMATION_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), ANIMATION_TYPE_CEL_ANIMATION, AnimationCelAnimationClass))
#define ANIMATION_IS_CEL_ANIMATION(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), ANIMATION_TYPE_CEL_ANIMATION))
#define ANIMATION_IS_CEL_ANIMATION_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), ANIMATION_TYPE_CEL_ANIMATION))
#define ANIMATION_CEL_ANIMATION_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), ANIMATION_TYPE_CEL_ANIMATION, AnimationCelAnimationClass))

typedef struct _AnimationCelAnimation        AnimationCelAnimation;
typedef struct _AnimationCelAnimationClass   AnimationCelAnimationClass;
typedef struct _AnimationCelAnimationPrivate AnimationCelAnimationPrivate;

struct _AnimationCelAnimation
{
  Animation                     parent_instance;

  AnimationCelAnimationPrivate *priv;
};

struct _AnimationCelAnimationClass
{
  AnimationClass  parent_class;
};

GType         animation_cel_animation_get_type (void);


void          animation_cel_animation_set_layers      (AnimationCelAnimation *animation,
                                                       gint                   level,
                                                       gint                   position,
                                                       GList                 *layers);
GList       * animation_cel_animation_get_layers      (AnimationCelAnimation *animation,
                                                       gint                   level,
                                                       gint                   position);

void          animation_cel_animation_set_comment     (AnimationCelAnimation *animation,
                                                       gint                   position,
                                                       const gchar           *comment);
const gchar * animation_cel_animation_get_comment     (AnimationCelAnimation *animation,
                                                       gint                   position);

void          animation_cel_animation_set_duration    (AnimationCelAnimation *animation,
                                                       gint                   duration);

gint          animation_cel_animation_get_levels      (AnimationCelAnimation *animation);

void          animation_cel_animation_set_track_title (AnimationCelAnimation *animation,
                                                       gint                   level,
                                                       const gchar           *title);
const gchar * animation_cel_animation_get_track_title (AnimationCelAnimation *animation,
                                                       gint                   level);

#endif  /*  __ANIMATION_CEL_ANIMATION_H__  */
