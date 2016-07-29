/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * animation.h
 * Copyright (C) 2015-2016 Jehan <jehan@gimp.org>
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

#ifndef __ANIMATION_H__
#define __ANIMATION_H__

#define ANIMATION_TYPE_ANIMATION            (animation_get_type ())
#define ANIMATION(obj)                      (G_TYPE_CHECK_INSTANCE_CAST ((obj), ANIMATION_TYPE_ANIMATION, Animation))
#define ANIMATION_CLASS(klass)              (G_TYPE_CHECK_CLASS_CAST ((klass), ANIMATION_TYPE_ANIMATION, AnimationClass))
#define ANIMATION_IS_ANIMATION(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), ANIMATION_TYPE_ANIMATION))
#define ANIMATION_IS_ANIMATION_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), ANIMATION_TYPE_ANIMATION))
#define ANIMATION_GET_CLASS(obj)            (G_TYPE_INSTANCE_GET_CLASS ((obj), ANIMATION_TYPE_ANIMATION, AnimationClass))

typedef struct _Animation      Animation;
typedef struct _AnimationClass AnimationClass;

struct _Animation
{
  GObject      parent_instance;
};

struct _AnimationClass
{
  GObjectClass  parent_class;

  /* Defaults to position 1. */
  gint         (*get_start_position) (Animation *animation);

  /* Defaults to returning FALSE for any different position. */
  gboolean     (*same)               (Animation *animation,
                                      gint       prev_pos,
                                      gint       next_pos);

  /* These virtual methods must be implemented by any subclass. */
  void         (*load)          (Animation   *animation);
  void         (*load_xml)      (Animation   *animation,
                                 const gchar *xml);
  gint         (*get_length)    (Animation   *animation);


  GeglBuffer * (*get_frame)     (Animation   *animation,
                                 gint         pos);

  gchar      * (*serialize)     (Animation   *animation);
};

GType         animation_get_type (void);

Animation   * animation_new                (gint32       image_id,
                                            gboolean     animatic,
                                            const gchar *xml);

gint32        animation_get_image_id       (Animation   *animation);

void          animation_load               (Animation   *animation);

void          animation_save_to_parasite   (Animation   *animation);

gint          animation_get_start_position (Animation   *animation);
gint          animation_get_position       (Animation   *animation);
gint          animation_get_length         (Animation   *animation);

void          animation_set_proxy          (Animation   *animation,
                                            gdouble      ratio);
gdouble       animation_get_proxy          (Animation   *animation);
void          animation_get_size           (Animation   *animation,
                                            gint        *width,
                                            gint        *height);

GeglBuffer  * animation_get_frame          (Animation   *animation,
                                            gint         frame_number);


gboolean      animation_is_playing         (Animation   *animation);
void          animation_play               (Animation   *animation);
void          animation_stop               (Animation   *animation);
void          animation_next               (Animation   *animation);
void          animation_prev               (Animation   *animation);
void          animation_jump               (Animation   *animation,
                                            gint         index);

void          animation_set_playback_start (Animation   *animation,
                                            gint         frame_number);
gint          animation_get_playback_start (Animation   *animation);
void          animation_set_playback_stop  (Animation   *animation,
                                            gint         frame_number);
gint          animation_get_playback_stop  (Animation   *animation);

void          animation_set_framerate      (Animation   *animation,
                                            gdouble      fps);
gdouble       animation_get_framerate      (Animation   *animation);

gboolean      animation_loaded             (Animation   *animation);

#endif  /*  __ANIMATION_H__  */
