/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * animation-camera.c
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

#include "config.h"

#include <libgimp/gimp.h>
#include "libgimp/stdplugins-intl.h"

#include "animation.h"

#include "animation-camera.h"

enum
{
  PROP_0,
  PROP_ANIMATION,
};

enum
{
  CAMERA_CHANGED,
  KEYFRAME_SET,
  KEYFRAME_DELETED,
  LAST_SIGNAL
};

typedef struct
{
  gint x;
  gint y;
}
Offset;

struct _AnimationCameraPrivate
{
  Animation *animation;

  /* Panning and tilting. */
  GList     *offsets;
  GList     *zoom;

  /* Preview */
  Offset    *preview_offset;
  gint       preview_position;
  gdouble    preview_scale;

  gboolean   block_signals;
};

static void   animation_camera_finalize             (GObject         *object);
static void   animation_camera_set_property         (GObject         *object,
                                                     guint            property_id,
                                                     const GValue    *value,
                                                     GParamSpec      *pspec);
static void   animation_camera_get_property         (GObject         *object,
                                                     guint            property_id,
                                                     GValue          *value,
                                                     GParamSpec      *pspec);

static void   animation_camera_emit_camera_changed  (AnimationCamera *camera,
                                                     gint             position);
static void   animation_camera_get_real             (AnimationCamera *camera,
                                                     gint             position,
                                                     gint            *x_offset,
                                                     gint            *y_offset,
                                                     gdouble         *scale);

G_DEFINE_TYPE (AnimationCamera, animation_camera, G_TYPE_OBJECT)

#define parent_class animation_camera_parent_class

static guint signals[LAST_SIGNAL] = { 0 };

static void
animation_camera_class_init (AnimationCameraClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->finalize     = animation_camera_finalize;
  object_class->get_property = animation_camera_get_property;
  object_class->set_property = animation_camera_set_property;

  /**
   * AnimationCamera::camera-changed:
   * @camera: the #AnimationCamera.
   * @position:
   * @duration:
   *
   * The ::camera-changed signal will be emitted when camera offsets,
   * zoom, or other characteristics were updated between
   * [@position; @position + @duration[.
   */
  signals[CAMERA_CHANGED] =
    g_signal_new ("camera-changed",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (AnimationCameraClass, camera_changed),
                  NULL, NULL,
                  NULL,
                  G_TYPE_NONE,
                  2,
                  G_TYPE_INT,
                  G_TYPE_INT);

  /**
   * AnimationCamera::keyframe-set:
   * @camera: the #AnimationCamera.
   * @position:
   *
   * The ::keyframe-set signal will be emitted when a keyframe is
   * created or modified at @position.
   */
  signals[KEYFRAME_SET] =
    g_signal_new ("keyframe-set",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (AnimationCameraClass, keyframe_set),
                  NULL, NULL,
                  NULL,
                  G_TYPE_NONE,
                  1,
                  G_TYPE_INT);

  /**
   * AnimationCamera::keyframe-deleted:
   * @camera: the #AnimationCamera.
   * @position:
   *
   * The ::keyframe-set signal will be emitted when a keyframe is
   * deleted at @position.
   */
  signals[KEYFRAME_DELETED] =
    g_signal_new ("keyframe-deleted",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (AnimationCameraClass, keyframe_deleted),
                  NULL, NULL,
                  NULL,
                  G_TYPE_NONE,
                  1,
                  G_TYPE_INT);

  g_object_class_install_property (object_class, PROP_ANIMATION,
                                   g_param_spec_object ("animation",
                                                        NULL, NULL,
                                                        ANIMATION_TYPE_ANIMATION,
                                                        G_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT_ONLY));

  g_type_class_add_private (klass, sizeof (AnimationCameraPrivate));
}

static void
animation_camera_init (AnimationCamera *view)
{
  view->priv = G_TYPE_INSTANCE_GET_PRIVATE (view,
                                            ANIMATION_TYPE_CAMERA,
                                            AnimationCameraPrivate);
  view->priv->preview_position = -1;
}

/************ Public Functions ****************/

/**
 * animation_camera_new:
 *
 * Creates a new camera.
 */
AnimationCamera *
animation_camera_new (Animation *animation)
{
  AnimationCamera *camera;

  camera = g_object_new (ANIMATION_TYPE_CAMERA,
                         "animation", animation,
                         NULL);

  return camera;
}

gboolean
animation_camera_has_offset_keyframe (AnimationCamera *camera,
                                      gint             position)
{
  g_return_val_if_fail (position >= 0 &&
                        position < animation_get_duration (camera->priv->animation),
                        FALSE);

  return g_list_nth_data (camera->priv->offsets, position) != NULL;
}

gboolean
animation_camera_has_zoom_keyframe (AnimationCamera *camera,
                                    gint             position)
{
  g_return_val_if_fail (position >= 0 &&
                        position < animation_get_duration (camera->priv->animation),
                        FALSE);

  return g_list_nth_data (camera->priv->zoom, position) != NULL;
}

void
animation_camera_set_offsets (AnimationCamera *camera,
                              gint             position,
                              gint             x,
                              gint             y)
{
  GList  *iter;
  Offset *offset;

  g_return_if_fail (position >= 0 &&
                    position < animation_get_duration (camera->priv->animation));

  iter = g_list_nth (camera->priv->offsets, position);

  if (! iter)
    {
      gint length = g_list_length (camera->priv->offsets);
      gint i;

      for (i = length; i < position; i++)
        {
          camera->priv->offsets = g_list_append (camera->priv->offsets, NULL);
        }
      offset = g_new (Offset, 1);
      camera->priv->offsets = g_list_append (camera->priv->offsets, offset);
    }
  else
    {
      if (! iter->data)
        {
          iter->data = g_new (Offset, 1);
        }
      offset = iter->data;
    }

  offset->x = x;
  offset->y = y;

  if (! camera->priv->block_signals)
    {
      g_signal_emit (camera, signals[KEYFRAME_SET], 0, position);
      animation_camera_emit_camera_changed (camera, position);
    }
}

void
animation_camera_zoom (AnimationCamera *camera,
                       gint             position,
                       gdouble          scale)
{
  GList   *iter;
  gdouble *zoom;

  g_return_if_fail (position >= 0 &&
                    position < animation_get_duration (camera->priv->animation));

  iter = g_list_nth (camera->priv->zoom, position);

  if (! iter)
    {
      gint length = g_list_length (camera->priv->zoom);
      gint i;

      for (i = length; i < position; i++)
        {
          camera->priv->zoom = g_list_append (camera->priv->zoom, NULL);
        }
      zoom = g_new (gdouble, 1);
      camera->priv->zoom = g_list_append (camera->priv->zoom, zoom);
    }
  else
    {
      if (! iter->data)
        {
          iter->data = g_new (gdouble, 1);
        }
      zoom = iter->data;
    }
  *zoom = scale;

  if (! camera->priv->block_signals)
    {
      g_signal_emit (camera, signals[KEYFRAME_SET], 0, position);
      animation_camera_emit_camera_changed (camera, position);
    }
}

void
animation_camera_delete_offset_keyframe (AnimationCamera *camera,
                                         gint             position)
{
  GList *iter;

  g_return_if_fail (position >= 0 &&
                    position < animation_get_duration (camera->priv->animation));

  iter = g_list_nth (camera->priv->offsets, position);
  if (iter && iter->data)
    {
      g_free (iter->data);
      iter->data = NULL;

      g_signal_emit (camera, signals[KEYFRAME_DELETED], 0, position);
      animation_camera_emit_camera_changed (camera, position);
    }
}

void
animation_camera_delete_zoom_keyframe (AnimationCamera *camera,
                                       gint             position)
{
  GList *iter;

  g_return_if_fail (position >= 0 &&
                    position < animation_get_duration (camera->priv->animation));

  iter = g_list_nth (camera->priv->zoom, position);
  if (iter && iter->data)
    {
      g_free (iter->data);
      iter->data = NULL;

      g_signal_emit (camera, signals[KEYFRAME_DELETED], 0, position);
      animation_camera_emit_camera_changed (camera, position);
    }
}

void
animation_camera_preview_keyframe (AnimationCamera *camera,
                                   gint             position,
                                   gint             x,
                                   gint             y,
                                   gdouble          scale)
{
  g_return_if_fail (position >= 0 &&
                    position < animation_get_duration (camera->priv->animation));

  if (! camera->priv->preview_offset)
    camera->priv->preview_offset = g_new (Offset, 1);

  camera->priv->preview_offset->x = x;
  camera->priv->preview_offset->y = y;
  camera->priv->preview_scale     = scale;
  camera->priv->preview_position  = position;

  g_signal_emit (camera, signals[CAMERA_CHANGED], 0,
                 position, 1);
}

void
animation_camera_apply_preview (AnimationCamera *camera)
{
  if (camera->priv->preview_offset)
    {
      gint    preview_offset_x;
      gint    preview_offset_y;
      gdouble preview_scale;
      gint    real_offset_x;
      gint    real_offset_y;
      gdouble real_scale;
      gint    position;

      animation_camera_get (camera, camera->priv->preview_position,
                            &preview_offset_x, &preview_offset_y,
                            &preview_scale);
      animation_camera_get_real (camera, camera->priv->preview_position,
                                 &real_offset_x, &real_offset_y,
                                 &real_scale);

      g_free (camera->priv->preview_offset);
      camera->priv->preview_offset    = NULL;
      position = camera->priv->preview_position;
      camera->priv->preview_position  = -1;

      /* Do not run the changed signal twice and recompute twice the
       * same frame. Just a little internal trick. */
      camera->priv->block_signals = TRUE;
      if (preview_offset_x != real_offset_x ||
          preview_offset_y != real_offset_y)
        {
          animation_camera_set_offsets (camera, position,
                                        preview_offset_x,
                                        preview_offset_y);
        }
      if (preview_scale != real_scale)
        {
          animation_camera_zoom (camera, position, preview_scale);
        }
      camera->priv->block_signals = FALSE;
      g_signal_emit (camera, signals[KEYFRAME_SET], 0, position);
      animation_camera_emit_camera_changed (camera, position);
    }
}

void
animation_camera_reset_preview (AnimationCamera *camera)
{
  gboolean changed = FALSE;
  gint     position_changed;

  if (camera->priv->preview_offset)
    {
      gint    preview_offset_x;
      gint    preview_offset_y;
      gdouble preview_scale;
      gint    real_offset_x;
      gint    real_offset_y;
      gdouble real_scale;

      animation_camera_get (camera, camera->priv->preview_position,
                            &preview_offset_x, &preview_offset_y,
                            &preview_scale);
      animation_camera_get_real (camera, camera->priv->preview_position,
                                 &real_offset_x, &real_offset_y,
                                 &real_scale);
      changed = (preview_offset_x != real_offset_x ||
                 preview_offset_y != real_offset_y ||
                 preview_scale != real_scale);
      position_changed = camera->priv->preview_position;

      g_free (camera->priv->preview_offset);
      camera->priv->preview_offset    = NULL;
    }

  camera->priv->preview_position  = -1;

  if (changed)
    g_signal_emit (camera, signals[CAMERA_CHANGED], 0,
                   position_changed, 1);
}

void
animation_camera_get (AnimationCamera *camera,
                      gint             position,
                      gint            *x_offset,
                      gint            *y_offset,
                      gdouble         *scale)
{
  if (camera->priv->preview_position == position)
    {
      *x_offset = camera->priv->preview_offset->x;
      *y_offset = camera->priv->preview_offset->y;
      *scale    = camera->priv->preview_scale;
    }
  else
    {
      animation_camera_get_real (camera, position,
                                 x_offset, y_offset,
                                 scale);
    }
}

/************ Private Functions ****************/

static void
animation_camera_finalize (GObject *object)
{
  g_list_free_full (ANIMATION_CAMERA (object)->priv->offsets, g_free);
  g_list_free_full (ANIMATION_CAMERA (object)->priv->zoom, g_free);
  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
animation_camera_set_property (GObject      *object,
                               guint         property_id,
                               const GValue *value,
                               GParamSpec   *pspec)
{
  AnimationCamera *camera = ANIMATION_CAMERA (object);

  switch (property_id)
    {
    case PROP_ANIMATION:
      camera->priv->animation = g_value_get_object (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}


static void
animation_camera_get_property (GObject      *object,
                               guint         property_id,
                               GValue       *value,
                               GParamSpec   *pspec)
{
  AnimationCamera *camera = ANIMATION_CAMERA (object);

  switch (property_id)
    {
    case PROP_ANIMATION:
      g_value_set_object (value, camera->priv->animation);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
animation_camera_emit_camera_changed (AnimationCamera *camera,
                                      gint             position)
{
  GList *iter;
  gint   prev_keyframe;
  gint   next_keyframe;
  gint   i;

  if (position > 0)
    {
      i = position - 1;
      iter = g_list_nth (camera->priv->offsets, i);
      for (; iter && ! iter->data; iter = iter->prev, i--)
        ;
      iter = g_list_nth (camera->priv->zoom, i);
      prev_keyframe = i + 1;
      for (; iter && ! iter->data; iter = iter->prev, i--)
        ;
      prev_keyframe = MIN (i + 1, prev_keyframe);
    }
  else
    {
      prev_keyframe = 0;
    }
  if (position < animation_get_duration (camera->priv->animation) - 1)
    {
      i = position + 1;
      iter = g_list_nth (camera->priv->offsets, i);
      for (; iter && ! iter->data; iter = iter->next, i++)
        ;
      if (iter && iter->data)
        {
          next_keyframe = i - 1;
          iter = g_list_nth (camera->priv->zoom, i);
          for (; iter && ! iter->data; iter = iter->next, i++)
            ;
          next_keyframe = MAX (i - 1, next_keyframe);
        }
      else
        {
          next_keyframe = animation_get_duration (camera->priv->animation) - 1;
        }
    }
  else
    {
      next_keyframe = animation_get_duration (camera->priv->animation) - 1;
    }
  g_signal_emit (camera, signals[CAMERA_CHANGED], 0,
                 prev_keyframe, next_keyframe - prev_keyframe + 1);
}

static void
animation_camera_get_real (AnimationCamera *camera,
                           gint             position,
                           gint            *x_offset,
                           gint            *y_offset,
                           gdouble         *scale)
{
  Offset  *position_keyframe;
  gdouble *zoom_keyframe;

  g_return_if_fail (position >= 0 &&
                    position < animation_get_duration (camera->priv->animation));

  position_keyframe = g_list_nth_data (camera->priv->offsets, position);
  if (position_keyframe)
    {
      /* There is a keyframe to this exact position. Use its values. */
      *x_offset = position_keyframe->x;
      *y_offset = position_keyframe->y;
    }
  else
    {
      GList  *iter;
      Offset *prev_keyframe = NULL;
      Offset *next_keyframe = NULL;
      gint    prev_keyframe_pos;
      gint    next_keyframe_pos;
      gint    i;

      /* This position is not a keyframe. */
      if (position > 0)
        {
          i = MIN (position - 1, g_list_length (camera->priv->offsets) - 1);
          iter = g_list_nth (camera->priv->offsets, i);
          for (; iter && ! iter->data; iter = iter->prev, i--)
            ;
          if (iter && iter->data)
            {
              prev_keyframe_pos = i;
              prev_keyframe = iter->data;
            }
        }
      if (position < animation_get_duration (camera->priv->animation) - 1)
        {
          i = position + 1;
          iter = g_list_nth (camera->priv->offsets, i);
          for (; iter && ! iter->data; iter = iter->next, i++)
            ;
          if (iter && iter->data)
            {
              next_keyframe_pos = i;
              next_keyframe = iter->data;
            }
        }

      if (prev_keyframe == NULL && next_keyframe == NULL)
        {
          *x_offset = *y_offset = 0;
        }
      else if (prev_keyframe == NULL)
        {
          *x_offset = next_keyframe->x;
          *y_offset = next_keyframe->y;
        }
      else if (next_keyframe == NULL)
        {
          *x_offset = prev_keyframe->x;
          *y_offset = prev_keyframe->y;
        }
      else
        {
          /* XXX No curve editing or anything like this yet.
           * All keyframing is linear in this first version.
           */
          *x_offset = prev_keyframe->x + (position - prev_keyframe_pos) *
                                         (next_keyframe->x - prev_keyframe->x) /
                                         (next_keyframe_pos - prev_keyframe_pos);
          *y_offset = prev_keyframe->y + (position - prev_keyframe_pos) *
                                         (next_keyframe->y - prev_keyframe->y) /
                                         (next_keyframe_pos - prev_keyframe_pos);
        }
    }

  zoom_keyframe = g_list_nth_data (camera->priv->zoom, position);
  if (zoom_keyframe)
    {
      /* There is a keyframe to this exact position. Use its values. */
      *scale = *zoom_keyframe;
    }
  else
    {
      GList  *iter;
      gdouble *prev_keyframe = NULL;
      gdouble *next_keyframe = NULL;
      gint    prev_keyframe_pos;
      gint    next_keyframe_pos;
      gint    i;

      /* This position is not a keyframe. */
      if (position > 0)
        {
          i = MIN (position - 1, g_list_length (camera->priv->zoom) - 1);
          iter = g_list_nth (camera->priv->zoom, i);
          for (; iter && ! iter->data; iter = iter->prev, i--)
            ;
          if (iter && iter->data)
            {
              prev_keyframe_pos = i;
              prev_keyframe = iter->data;
            }
        }
      if (position < animation_get_duration (camera->priv->animation) - 1)
        {
          i = position + 1;
          iter = g_list_nth (camera->priv->zoom, i);
          for (; iter && ! iter->data; iter = iter->next, i++)
            ;
          if (iter && iter->data)
            {
              next_keyframe_pos = i;
              next_keyframe = iter->data;
            }
        }

      if (prev_keyframe == NULL && next_keyframe == NULL)
        {
          *scale = 1.0;
        }
      else if (prev_keyframe == NULL)
        {
          *scale = *next_keyframe;
        }
      else if (next_keyframe == NULL)
        {
          *scale = *prev_keyframe;
        }
      else
        {
          /* XXX No curve editing or anything like this yet.
           * All keyframing is linear in this first version.
           */
          *scale = *prev_keyframe + (position - prev_keyframe_pos) *
                                    (*next_keyframe - *prev_keyframe) /
                                    (next_keyframe_pos - prev_keyframe_pos);
        }
    }
}
