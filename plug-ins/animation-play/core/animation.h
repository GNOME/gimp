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

  /* Signals */
  void         (*loading)            (Animation    *animation,
                                      gdouble       ratio);
  void         (*loaded)             (Animation    *animation);

  void         (*cache_invalidated)  (Animation    *animation,
                                      gint          position);
  void         (*duration_changed)   (Animation    *animation,
                                      gint          duration);
  void         (*framerate_changed)  (Animation    *animation,
                                      gdouble       fps);
  void         (*proxy)              (Animation    *animation,
                                      gdouble       ratio);

  /* Defaults to returning FALSE for any different position. */
  gboolean     (*same)               (Animation    *animation,
                                      gint          prev_pos,
                                      gint          next_pos);

  /* These virtual methods must be implemented by any subclass. */
  gint         (*get_duration)       (Animation    *animation);

  GeglBuffer * (*get_frame)          (Animation    *animation,
                                      gint          pos);

  void         (*purge_cache)        (Animation    *animation);

  void         (*reset_defaults)     (Animation    *animation);
  gchar      * (*serialize)          (Animation    *animation,
                                      const gchar  *playback_xml);
  gboolean     (*deserialize)        (Animation    *animation,
                                      const gchar  *xml,
                                      GError      **error);

  void         (*update_paint_view)  (Animation    *animation,
                                      gint          position);
};

GType         animation_get_type (void);

Animation   * animation_new                (gint32       image_id,
                                            gboolean     animatic,
                                            const gchar *xml);

gint32        animation_get_image_id       (Animation   *animation);

void          animation_load               (Animation   *animation);

void          animation_save_to_parasite   (Animation   *animation,
                                            const gchar *playback_xml);

gint          animation_get_duration       (Animation   *animation);

void          animation_set_proxy          (Animation   *animation,
                                            gdouble      ratio);
gdouble       animation_get_proxy          (Animation   *animation);
void          animation_get_size           (Animation   *animation,
                                            gint        *width,
                                            gint        *height);

GeglBuffer  * animation_get_frame          (Animation   *animation,
                                            gint         frame_number);

void          animation_set_framerate      (Animation   *animation,
                                            gdouble      fps);
gdouble       animation_get_framerate      (Animation   *animation);

gboolean      animation_loaded             (Animation   *animation);

void          animation_update_paint_view  (Animation   *animation,
                                            gint         position);

#endif  /*  __ANIMATION_H__  */
