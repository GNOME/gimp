/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * animation-renderer.h
 * Copyright (C) 2017 Jehan <jehan@gimp.org>
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

#ifndef __ANIMATION_RENDERER_H__
#define __ANIMATION_RENDERER_H__

#define ANIMATION_TYPE_RENDERER            (animation_renderer_get_type ())
#define ANIMATION_RENDERER(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), ANIMATION_TYPE_RENDERER, AnimationRenderer))
#define ANIMATION_RENDERER_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), ANIMATION_TYPE_RENDERER, AnimationRendererClass))
#define ANIMATION_IS_RENDERER(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), ANIMATION_TYPE_RENDERER))
#define ANIMATION_IS_RENDERER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), ANIMATION_TYPE_RENDERER))
#define ANIMATION_RENDERER_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), ANIMATION_TYPE_RENDERER, AnimationRendererClass))

typedef struct _AnimationRenderer        AnimationRenderer;
typedef struct _AnimationRendererClass   AnimationRendererClass;
typedef struct _AnimationRendererPrivate AnimationRendererPrivate;

struct _AnimationRenderer
{
  GObject                   parent_instance;

  AnimationRendererPrivate *priv;
};

struct _AnimationRendererClass
{
  GObjectClass  parent_class;

  void         (*cache_updated)  (AnimationRenderer *renderer,
                                  gint               position);
};

GType         animation_renderer_get_type (void);


GObject     * animation_renderer_new          (GObject           *playback);
GeglBuffer  * animation_renderer_get_buffer   (AnimationRenderer *renderer,
                                               gint               position);
gboolean      animation_renderer_identical    (AnimationRenderer *renderer,
                                               gint               position1,
                                               gint               position2);

#endif  /*  __ANIMATION_RENDERER_H__  */
