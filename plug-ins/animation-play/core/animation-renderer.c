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

  GAsyncQueue        *queue;

  GAsyncQueue        *ack_queue;
  guint               idle_id;

  /* Frames are cached as GEGL buffers. */
  GMutex              lock;
  GeglBuffer        **cache;
  gchar             **hashes;
  GHashTable         *cache_table;
  gint                cache_size;

  GThread            *queue_thread;
};

static void     animation_renderer_finalize      (GObject           *object);
static void     animation_renderer_set_property  (GObject           *object,
                                                  guint              property_id,
                                                  const GValue      *value,
                                                  GParamSpec        *pspec);
static void     animation_renderer_get_property  (GObject           *object,
                                                  guint              property_id,
                                                  GValue            *value,
                                                  GParamSpec        *pspec);

static gint     animation_renderer_sort_frame    (gconstpointer      f1,
                                                  gconstpointer      f2,
                                                  gpointer           data);

static gpointer animation_renderer_process_queue (AnimationRenderer *renderer);
static gboolean animation_renderer_idle_update   (AnimationRenderer *renderer);

static void     on_frames_changed                (Animation         *animation,
                                                  gint               position,
                                                  gint               length,
                                                  AnimationRenderer *renderer);
static void     on_duration_changed              (Animation         *animation,
                                                  gint               duration,
                                                  AnimationRenderer *renderer);
static void     on_animation_loaded              (Animation         *animation,
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
  g_mutex_init (&(renderer->priv->lock));
  renderer->priv->cache_table = g_hash_table_new_full (g_str_hash, g_str_equal,
                                                       (GDestroyNotify) g_free,
                                                       (GDestroyNotify) g_weak_ref_clear);
  renderer->priv->queue = g_async_queue_new_full ((GDestroyNotify) g_weak_ref_clear);
  renderer->priv->ack_queue = g_async_queue_new_full ((GDestroyNotify) g_weak_ref_clear);
}

static void
animation_renderer_finalize (GObject *object)
{
  AnimationRenderer *renderer = ANIMATION_RENDERER (object);
  Animation         *animation;
  gint               i;

  animation = animation_playback_get_animation (renderer->priv->playback);

  /* Stop the thread. */
  g_async_queue_push_front (renderer->priv->queue,
                            GINT_TO_POINTER (- 1));
  g_thread_join (renderer->priv->queue_thread);
  g_thread_unref (renderer->priv->queue_thread);

  /* Clean remaining data. */
  if (renderer->priv->idle_id)
    g_source_remove (renderer->priv->idle_id);
  g_mutex_lock (&renderer->priv->lock);
  for (i = 0; i < animation_get_duration (animation); i++)
    {
      if (renderer->priv->cache[i])
        g_object_unref (renderer->priv->cache[i]);
      if (renderer->priv->hashes[i])
        g_free (renderer->priv->hashes[i]);
    }
  g_free (renderer->priv->cache);
  g_free (renderer->priv->hashes);

  g_async_queue_unref (renderer->priv->queue);
  g_async_queue_unref (renderer->priv->ack_queue);
  g_hash_table_destroy (renderer->priv->cache_table);
  g_mutex_unlock (&renderer->priv->lock);
  g_mutex_clear (&renderer->priv->lock);

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

static gint
animation_renderer_sort_frame (gconstpointer f1,
                               gconstpointer f2,
                               gpointer      data)
{
  gint first_frame = GPOINTER_TO_INT (data);
  gint frame1      = GPOINTER_TO_INT (f1);
  gint frame2      = GPOINTER_TO_INT (f2);
  gint invert;

  if (frame1 == frame2)
    {
      return 0;
    }
  else
    {
      invert = ((frame1 >= first_frame && frame2 >= first_frame) ||
                (frame1 < first_frame && frame2 < first_frame)) ? 1 : -1;
      if (frame1 < frame2)
        return invert * -1;
      else
        return invert;
    }
}

static gpointer
animation_renderer_process_queue (AnimationRenderer *renderer)
{
  while (TRUE)
    {
      Animation  *animation;
      GeglBuffer *buffer  = NULL;
      GWeakRef   *ref     = NULL;
      gchar      *hash    = NULL;
      gchar      *hash_cp = NULL;
      gint        frame;

      frame = GPOINTER_TO_INT (g_async_queue_pop (renderer->priv->queue)) - 1;

      /* End flag. */
      if (frame < 0)
        g_thread_exit (NULL);

      /* It is possible to have position bigger than the animation duration if the
       * request was sent before a duration change. When this happens, just ignore
       * the request silently and go to the next one. */
      g_mutex_lock (&renderer->priv->lock);
      if (frame >= renderer->priv->cache_size)
        {
          g_mutex_unlock (&renderer->priv->lock);
          continue;
        }
      g_mutex_unlock (&renderer->priv->lock);

      animation = animation_playback_get_animation (renderer->priv->playback);
      hash = ANIMATION_GET_CLASS (animation)->get_frame_hash (animation,
                                                              frame);
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
              g_mutex_lock (&renderer->priv->lock);
              g_hash_table_insert (renderer->priv->cache_table,
                                   hash, ref);
              g_mutex_unlock (&renderer->priv->lock);
            }

          if (! buffer)
            {
              buffer = ANIMATION_GET_CLASS (animation)->create_frame (animation,
                                                                      G_OBJECT (renderer),
                                                                      frame);
              g_weak_ref_set (ref, buffer);
            }
        }

      g_mutex_lock (&renderer->priv->lock);
      if (renderer->priv->cache[frame])
        g_object_unref (renderer->priv->cache[frame]);
      if (renderer->priv->hashes[frame])
        g_free (renderer->priv->hashes[frame]);
      renderer->priv->cache[frame]  = buffer;
      renderer->priv->hashes[frame] = hash_cp;
      g_mutex_unlock (&renderer->priv->lock);

      /* Tell the main thread which buffers were updated, so that the
       * GUI reflects the change. */
      g_async_queue_remove (renderer->priv->ack_queue,
                            GINT_TO_POINTER (frame + 1));
      g_async_queue_push (renderer->priv->ack_queue,
                          GINT_TO_POINTER (frame + 1));

      /* Relinquish CPU regularly. */
      g_thread_yield ();
    }
  return NULL;
}

static gboolean
animation_renderer_idle_update (AnimationRenderer *renderer)
{
  gpointer p;
  gboolean retval;

  while ((p = g_async_queue_try_pop (renderer->priv->ack_queue)))
    {
      gint frame = GPOINTER_TO_INT (p) - 1;
      g_signal_emit_by_name (renderer, "cache-updated", frame);
    }
  /* Make sure the UI gets updated regularly. */
  while (g_main_context_pending (NULL))
    g_main_context_iteration (NULL, FALSE);

  /* If nothing is being rendered (negative queue length, meaning the
   * renderer is waiting), nor is there anything in the ACK queue, just
   * stop the idle source. */
  if (g_async_queue_length (renderer->priv->queue) < 0 &&
      g_async_queue_length (renderer->priv->ack_queue) == 0)
    {
      retval = G_SOURCE_REMOVE;
      renderer->priv->idle_id = 0;
    }
  else
    {
      retval = G_SOURCE_CONTINUE;
    }

  return retval;
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
      /* Remove if already present: don't process twice the same frames.
       * XXX GAsyncQueue does not allow NULL data for no good reason (it relies
       * on GQueue which does allow NULL data. As a trick, I just add 1 so that
       * we can process the frame 0. */
      g_async_queue_remove (renderer->priv->queue, GINT_TO_POINTER (i + 1));
      g_async_queue_push_sorted (renderer->priv->queue,
                                 GINT_TO_POINTER (i + 1),
                                 (GCompareDataFunc) animation_renderer_sort_frame,
                                 /* TODO: right now I am sorting the render
                                  * queue in common order. I will have to test
                                  * sorting it from the current position.
                                  */
                                 0);
      if (renderer->priv->idle_id == 0)
        renderer->priv->idle_id = g_idle_add_full (G_PRIORITY_HIGH_IDLE,
                                                   (GSourceFunc) animation_renderer_idle_update,
                                                   renderer, NULL);
    }
}

static void
on_duration_changed (Animation         *animation,
                     gint               duration,
                     AnimationRenderer *renderer)
{
  gint i;

  g_mutex_lock (&renderer->priv->lock);
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
  g_mutex_unlock (&renderer->priv->lock);
}

static void
on_animation_loaded (Animation         *animation,
                     AnimationRenderer *renderer)
{
  g_signal_handlers_disconnect_by_func (animation,
                                        G_CALLBACK (on_animation_loaded),
                                        renderer);
  renderer->priv->queue_thread = g_thread_new ("gimp-animation-process-queue",
                                               (GThreadFunc) animation_renderer_process_queue,
                                               renderer);
  renderer->priv->idle_id = 0;
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
  g_signal_connect (animation, "loaded",
                    G_CALLBACK (on_animation_loaded), renderer);

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

  g_mutex_lock (&renderer->priv->lock);
  frame = renderer->priv->cache[position];
  if (frame)
    frame = g_object_ref (frame);
  g_mutex_unlock (&renderer->priv->lock);

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
