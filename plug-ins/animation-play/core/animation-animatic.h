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

#ifndef __ANIMATION_ANIMATIC_H__
#define __ANIMATION_ANIMATIC_H__

#include "animation.h"

#define ANIMATION_TYPE_ANIMATIC            (animation_animatic_get_type ())
#define ANIMATION_ANIMATIC(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), ANIMATION_TYPE_ANIMATIC, AnimationAnimatic))
#define ANIMATION_ANIMATIC_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), ANIMATION_TYPE_ANIMATIC, AnimationAnimaticClass))
#define ANIMATION_IS_ANIMATIC(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), ANIMATION_TYPE_ANIMATIC))
#define ANIMATION_IS_ANIMATIC_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), ANIMATION_TYPE_ANIMATIC))
#define ANIMATION_ANIMATIC_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), ANIMATION_TYPE_ANIMATIC, AnimationAnimaticClass))

typedef struct _AnimationAnimatic      AnimationAnimatic;
typedef struct _AnimationAnimaticClass AnimationAnimaticClass;

struct _AnimationAnimatic
{
  Animation       parent_instance;
};

struct _AnimationAnimaticClass
{
  AnimationClass  parent_class;
};

GType            animation_animatic_get_type (void);

void             animation_animatic_set_panel_duration (AnimationAnimatic *animatic,
                                                        gint               panel,
                                                        gint               duration);
gint             animation_animatic_get_panel_duration (AnimationAnimatic *animatic,
                                                        gint               panel);

void             animation_animatic_set_comment        (AnimationAnimatic *animatic,
                                                        gint               panel,
                                                        const gchar       *comment);
const gchar    * animation_animatic_get_comment        (AnimationAnimatic *animatic,
                                                        gint               panel);

void             animation_animatic_set_combine        (AnimationAnimatic *animatic,
                                                        gint               panel,
                                                        gboolean           combine);
const gboolean   animation_animatic_get_combine        (AnimationAnimatic *animatic,
                                                        gint               panel);

gint             animation_animatic_get_panel          (AnimationAnimatic *animation,
                                                        gint               position);
gint             animation_animatic_get_position       (AnimationAnimatic *animation,
                                                        gint               panel);

void             animation_animatic_move_panel         (AnimationAnimatic *animatic,
                                                        gint               panel,
                                                        gint               new_panel);
#endif  /*  __ANIMATION_ANIMATIC_H__  */
