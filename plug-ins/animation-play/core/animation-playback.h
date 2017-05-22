/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * animation_playback.h
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

#ifndef __ANIMATION_PLAYBACK_H__
#define __ANIMATION_PLAYBACK_H__

#define ANIMATION_TYPE_PLAYBACK            (animation_playback_get_type ())
#define ANIMATION_PLAYBACK(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), ANIMATION_TYPE_PLAYBACK, AnimationPlayback))
#define ANIMATION_PLAYBACK_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), ANIMATION_TYPE_PLAYBACK, AnimationPlaybackClass))
#define ANIMATION_IS_PLAYBACK(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), ANIMATION_TYPE_PLAYBACK))
#define ANIMATION_IS_PLAYBACK_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), ANIMATION_TYPE_PLAYBACK))
#define ANIMATION_PLAYBACK_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), ANIMATION_TYPE_PLAYBACK, AnimationPlaybackClass))

typedef struct _AnimationPlayback        AnimationPlayback;
typedef struct _AnimationPlaybackClass   AnimationPlaybackClass;
typedef struct _AnimationPlaybackPrivate AnimationPlaybackPrivate;

struct _AnimationPlayback
{
  GObject                   parent_instance;

  AnimationPlaybackPrivate *priv;
};

struct _AnimationPlaybackClass
{
  GObjectClass               parent_class;

  /* Signals */
  void       (*start)            (AnimationPlayback *playback);
  void       (*stop)             (AnimationPlayback *playback);
  void       (*range)            (AnimationPlayback *playback,
                                  gint               start,
                                  gint               stop);
  void       (*render)           (AnimationPlayback *playback,
                                  gint               position,
                                  GeglBuffer        *buffer,
                                  gboolean           must_draw_null);
  void       (*low_framerate)    (AnimationPlayback *playback,
                                  gdouble            actual_fps);
};

GType         animation_playback_get_type (void);

AnimationPlayback * animation_playback_new     (void);
gchar       * animation_playback_serialize     (AnimationPlayback   *playback);

void          animation_playback_set_animation (AnimationPlayback   *playback,
                                                Animation           *animation,
                                                const gchar         *xml);
Animation   * animation_playback_get_animation (AnimationPlayback   *playback);

gint          animation_playback_get_position  (AnimationPlayback   *playback);
GeglBuffer  * animation_playback_get_buffer    (AnimationPlayback   *playback,
                                                gint                 position);

gboolean      animation_playback_is_playing    (AnimationPlayback   *playback);
void          animation_playback_play          (AnimationPlayback   *playback);
void          animation_playback_stop          (AnimationPlayback   *playback);
void          animation_playback_next          (AnimationPlayback   *playback);
void          animation_playback_prev          (AnimationPlayback   *playback);
void          animation_playback_jump          (AnimationPlayback   *playback,
                                                gint                 index);

void          animation_playback_set_start     (AnimationPlayback   *playback,
                                                gint                 index);
gint          animation_playback_get_start     (AnimationPlayback   *playback);

void          animation_playback_set_stop      (AnimationPlayback   *playback,
                                                gint                 index);
gint          animation_playback_get_stop      (AnimationPlayback   *playback);

void          animation_playback_export        (AnimationPlayback   *playback);
#endif  /*  __ANIMATION_PLAYBACK_H__  */
