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

#ifndef __ANIMATION_LEGACY_H__
#define __ANIMATION_LEGACY_H__

#include "animation.h"

#define ANIMATION_TYPE_ANIMATION_LEGACY            (animation_legacy_get_type ())
#define ANIMATION_LEGACY(obj)                      (G_TYPE_CHECK_INSTANCE_CAST ((obj), ANIMATION_TYPE_ANIMATION_LEGACY, Animation))
#define ANIMATION_LEGACY_CLASS(klass)              (G_TYPE_CHECK_CLASS_CAST ((klass), ANIMATION_TYPE_ANIMATION_LEGACY, AnimationClass))
#define ANIMATION_IS_ANIMATION_LEGACY(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), ANIMATION_TYPE_ANIMATION_LEGACY))
#define ANIMATION_IS_ANIMATION_LEGACY_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), ANIMATION_TYPE_ANIMATION_LEGACY))
#define ANIMATION_LEGACY_GET_CLASS(obj)            (G_TYPE_INSTANCE_GET_CLASS ((obj), ANIMATION_TYPE_ANIMATION_LEGACY, AnimationClass))

typedef struct _AnimationLegacy      AnimationLegacy;
typedef struct _AnimationLegacyClass AnimationLegacyClass;

struct _AnimationLegacy
{
  Animation       parent_instance;
};

struct _AnimationLegacyClass
{
  AnimationClass  parent_class;
};

GType         animation_legacy_get_type (void);

#endif  /*  __ANIMATION_LEGACY_H__  */
