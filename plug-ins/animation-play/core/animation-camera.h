/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * animation-camera.h
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

#ifndef __ANIMATION_CAMERA_H__
#define __ANIMATION_CAMERA_H__

#define ANIMATION_TYPE_CAMERA            (animation_camera_get_type ())
#define ANIMATION_CAMERA(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), ANIMATION_TYPE_CAMERA, AnimationCamera))
#define ANIMATION_CAMERA_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), ANIMATION_TYPE_CAMERA, AnimationCameraClass))
#define ANIMATION_IS_CAMERA(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), ANIMATION_TYPE_CAMERA))
#define ANIMATION_IS_CAMERA_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), ANIMATION_TYPE_CAMERA))
#define ANIMATION_CAMERA_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), ANIMATION_TYPE_CAMERA, AnimationCameraClass))

typedef struct _AnimationCamera        AnimationCamera;
typedef struct _AnimationCameraClass   AnimationCameraClass;
typedef struct _AnimationCameraPrivate AnimationCameraPrivate;

struct _AnimationCamera
{
  GObject                   parent_instance;

  AnimationCameraPrivate   *priv;
};

struct _AnimationCameraClass
{
  GObjectClass              parent_class;

  /* Signals */
  void         (*offsets_changed)   (AnimationCamera *camera,
                                     gint             position,
                                     gint             duration);
  void         (*keyframe_set)      (AnimationCamera *camera,
                                     gint             position);
  void         (*keyframe_deleted)  (AnimationCamera *camera,
                                     gint             position);
};

GType             animation_camera_get_type         (void) G_GNUC_CONST;

AnimationCamera * animation_camera_new              (Animation       *animation);

gboolean          animation_camera_has_keyframe     (AnimationCamera *camera,
                                                     gint             position);

void              animation_camera_set_keyframe     (AnimationCamera *camera,
                                                     gint             position,
                                                     gint             x,
                                                     gint             y);
void              animation_camera_delete_keyframe  (AnimationCamera *camera,
                                                     gint             position);
void              animation_camera_preview_keyframe (AnimationCamera *camera,
                                                     gint             position,
                                                     gint             x,
                                                     gint             y);
void              animation_camera_apply_preview    (AnimationCamera *camera);
void              animation_camera_reset_preview    (AnimationCamera *camera);

void              animation_camera_get              (AnimationCamera *camera,
                                                     gint             position,
                                                     gint            *x_offset,
                                                     gint            *y_offset);

#endif  /*  __ANIMATION_CAMERA_H__  */
