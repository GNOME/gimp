/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * animation-renderer.c
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

#include "config.h"

#include <libgimp/gimp.h>
#include <libgimp/stdplugins-intl.h>

#include "animation.h"
#include "animation-playback.h"
#include "animation-renderer.h"

enum
{
  CACHE_UPDATED,
  LAST_SIGNAL
};
enum
{
  PROP_0,
  PROP_PLAYBACK
};

struct _AnimationRendererPrivate
{
  AnimationPlayback  *playback;

  /* Frames are cached as GEGL buffers. */
  GeglBuffer        **cache;
  gchar             **hashes;
  GHashTable         *cache_table;
  gint                cache_size;
};

static void     animation_renderer_finalize     (GObject           *object);
static void     animation_renderer_set_property (GObject           *object,
                                                 guint              property_id,
                                                 const GValue      *value,
                                                 GParamSpec        *pspec);
static void     animation_renderer_get_property (GObject           *object,
                                                 guint              property_id,
                                                 GValue            *value,
                                                 GParamSpec        *pspec);

static void     on_frames_changed               (Animation         *animation,
                                                 gint               position,
                                                 gint               length,
                                                 AnimationRenderer *renderer);
static void     on_duration_changed             (Animation         *animation,
                                                 gint               duration,
                                                 AnimationRenderer *renderer);

G_DEFINE_TYPE (AnimationRenderer, animation_renderer, G_TYPE_OBJECT)

#define parent_class animation_renderer_parent_class

static guint signals[LAST_SIGNAL] = { 0 };

static void
animation_renderer_class_init (AnimationRendererClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  /**
   * Animation::cache-updated:
   * @animation: the animation.
   * @position: the frame position whose cache was updated.
   *
   * The ::cache-updated signal will be emitted when the contents
   * of frame at @position changes.
   */
  signals[CACHE_UPDATED] =
    g_signal_new ("cache-updated",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (AnimationRendererClass, cache_updated),
                  NULL, NULL,
                  NULL,
                  G_TYPE_NONE,
                  1,
                  G_TYPE_INT);

  object_class->finalize     = animation_renderer_finalize;
  object_class->set_property = animation_renderer_set_property;
  object_class->get_property = animation_renderer_get_property;

  /**
   * AnimationRenderer:animation:
   *
   * The associated #Animation.
   */
  g_object_class_install_property (object_class, PROP_PLAYBACK,
                                   g_param_spec_object ("playback",
                                                        NULL, NULL,
                                                        ANIMATION_TYPE_PLAYBACK,
                                                        G_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT_ONLY));

  g_type_class_add_private (klass, sizeof (AnimationRendererPrivate));
}

static void
animation_renderer_init (AnimationRenderer *renderer)
{
  renderer->priv = G_TYPE_INSTANCE_GET_PRIVATE (renderer,
                                                ANIMATION_TYPE_RENDERER,
                                                AnimationRendererPrivate);
  renderer->priv->cache_table = g_hash_table_new_full (g_str_hash, g_str_equal,
                                                       (GDestroyNotify) g_free,
                                                       (GDestroyNotify) g_weak_ref_clear);
}

static void
animation_renderer_finalize (GObject *object)
{
  AnimationRenderer *renderer = ANIMATION_RENDERER (object);
  Animation         *animation;
  gint               i;

  animation = animation_playback_get_animation (renderer->priv->playback);

  for (i = 0; i < animation_get_duration (animation); i++)
    {
      if (renderer->priv->cache[i])
        g_object_unref (renderer->priv->cache[i]);
      if (renderer->priv->hashes[i])
        g_free (renderer->priv->hashes[i]);
    }
  g_free (renderer->priv->cache);
  g_free (renderer->priv->hashes);
  g_hash_table_destroy (renderer->priv->cache_table);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
animation_renderer_set_property (GObject      *object,
                                 guint         property_id,
                                 const GValue *value,
                                 GParamSpec   *pspec)
{
  AnimationRenderer *renderer = ANIMATION_RENDERER (object);

  switch (property_id)
    {
    case PROP_PLAYBACK:
      renderer->priv->playback = g_value_get_object (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
animation_renderer_get_property (GObject      *object,
                                 guint         property_id,
                                 GValue       *value,
                                 GParamSpec   *pspec)
{
  AnimationRenderer *renderer = ANIMATION_RENDERER (object);

  switch (property_id)
    {
    case PROP_PLAYBACK:
      g_value_set_object (value, renderer->priv->playback);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
on_frames_changed (Animation         *animation,
                   gint               position,
                   gint               length,
                   AnimationRenderer *renderer)
{
  gint i;

  for (i = position; i < position + length; i++)
    {
      GeglBuffer *buffer  = NULL;
      GWeakRef   *ref     = NULL;
      gchar      *hash    = NULL;
      gchar      *hash_cp = NULL;

      hash = ANIMATION_GET_CLASS (animation)->get_frame_hash (animation, i);
      if (hash)
        {
          ref = g_hash_table_lookup (renderer->priv->cache_table, hash);
          hash_cp = g_strdup (hash);
          if (ref)
            {
              /* Acquire and add a new reference to the buffer. */
              buffer = g_weak_ref_get (ref);
            }
          else
            {
              ref = g_new (GWeakRef, 1);
              g_weak_ref_init (ref, NULL);
              g_hash_table_insert (renderer->priv->cache_table,
                                   hash, ref);
            }

          if (! buffer)
            {
              buffer = ANIMATION_GET_CLASS (animation)->create_frame (animation,
                                                                      G_OBJECT (renderer),
                                                                      i);
              g_weak_ref_set (ref, buffer);
            }
        }

      if (renderer->priv->cache[i])
        g_object_unref (renderer->priv->cache[i]);
      if (renderer->priv->hashes[i])
        g_free (renderer->priv->hashes[i]);
      renderer->priv->cache[i]  = buffer;
      renderer->priv->hashes[i] = hash_cp;

      g_signal_emit_by_name (renderer, "cache-updated", i);
    }
}

static void
on_duration_changed (Animation         *animation,
                     gint               duration,
                     AnimationRenderer *renderer)
{
  gint i;

  if (duration < renderer->priv->cache_size)
    {
      for (i = duration; i < renderer->priv->cache_size; i++)
        {
          if (renderer->priv->cache[i])
            g_object_unref (renderer->priv->cache[i]);
          if (renderer->priv->hashes[i])
            g_free (renderer->priv->hashes[i]);
        }
      renderer->priv->cache = g_renew (GeglBuffer*,
                                       renderer->priv->cache,
                                       duration);
      renderer->priv->hashes = g_renew (gchar*,
                                        renderer->priv->hashes,
                                        duration);
    }
  else if (duration > renderer->priv->cache_size)
    {
      renderer->priv->cache = g_renew (GeglBuffer*,
                                       renderer->priv->cache,
                                       duration);
      renderer->priv->hashes = g_renew (gchar*,
                                        renderer->priv->hashes,
                                        duration);
      for (i = renderer->priv->cache_size; i < duration; i++)
        {
            renderer->priv->cache[i]  = NULL;
            renderer->priv->hashes[i] = NULL;
        }
    }
  renderer->priv->cache_size = duration;
}

/**** Public Functions ****/

/**
 * animation_renderer_new:
 * @playback: the #AnimationPlayback.
 *
 * Returns: a new #AnimationRenderer. This renderer as well as all its methods
 * should only be visible by the attached @playback.
 **/
GObject *
animation_renderer_new (GObject *playback)
{
  GObject           *object;
  AnimationRenderer *renderer;
  Animation         *animation;

  object = g_object_new (ANIMATION_TYPE_RENDERER,
                         "playback", playback,
                         NULL);
  renderer = ANIMATION_RENDERER (object);

  animation = animation_playback_get_animation (renderer->priv->playback);
  renderer->priv->cache_size = animation_get_duration (animation);
  renderer->priv->cache      = g_new0 (GeglBuffer*,
                                       renderer->priv->cache_size);
  renderer->priv->hashes     = g_new0 (gchar*,
                                       renderer->priv->cache_size);
  g_signal_connect (animation, "frames-changed",
                    G_CALLBACK (on_frames_changed), renderer);
  g_signal_connect (animation, "duration-changed",
                    G_CALLBACK (on_duration_changed), renderer);

  return object;
}

/**
 * animation_renderer_get_buffer:
 * @renderer: the #AnimationRenderer.
 * @position:
 *
 * Returns: the #GeglBuffer cached for the frame at @position, with an
 * additional reference, so that it will stay valid even if the frame is
 * updated in-between. Therefore call g_object_unref() after usage.
 * As all other renderer function, it should only be visible by the playback,
 * or by frame-rendering code in #Animation subclasses themselves, since the
 * renderer passes itself as argument when it requests a new frame buffer.
 * Other pieces of code, in particular the GUI, should only call
 * animation_playback_get_buffer().
 **/
GeglBuffer *
animation_renderer_get_buffer (AnimationRenderer *renderer,
                               gint               position)
{
  GeglBuffer *frame;

  frame = renderer->priv->cache[position];
  if (frame)
    frame = g_object_ref (frame);

  return frame;
}

gboolean
animation_renderer_identical (AnimationRenderer *renderer,
                              gint               position1,
                              gint               position2)
{
  return (g_strcmp0 (renderer->priv->hashes[position1],
                     renderer->priv->hashes[position2]) == 0);
}
