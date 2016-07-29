/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * animation.c
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

#include "config.h"

#include <libgimp/gimp.h>
#include <libgimp/stdplugins-intl.h>

#include "animation-utils.h"
#include "animation-celanimation.h"

typedef struct _AnimationCelAnimationPrivate AnimationCelAnimationPrivate;

typedef struct
{
  gchar *title;
  /* The list of layers (identified by their tattoos). */
  GList *frames;
}
Track;

typedef struct
{
  GeglBuffer *buffer;

  /* The list of layers (identified by their tattoos) composited into
   * this buffer allows to easily compare caches so that to not
   * duplicate them.*/
  gint *composition;
  gint  n_sources;

  gint refs;
}
Cache;

struct _AnimationCelAnimationPrivate
{
  /* The number of frames. */
  gint   duration;

  /* Frames are cached as GEGL buffers. */
  GList *cache;

  /* Panel comments. */
  GList *comments;

  /* List of tracks/levels.
   * The background is a special-named track, always present
   * and first.
   * There is always at least 1 additional track. */
  GList *tracks;
};

#define GET_PRIVATE(animation) \
        G_TYPE_INSTANCE_GET_PRIVATE (animation, \
                                     ANIMATION_TYPE_CEL_ANIMATION, \
                                     AnimationCelAnimationPrivate)

static void         animation_cel_animation_finalize   (GObject           *object);

/* Virtual methods */

static gint         animation_cel_animation_get_length  (Animation         *animation);

static void         animation_cel_animation_load        (Animation         *animation);
static GeglBuffer * animation_cel_animation_get_frame   (Animation         *animation,
                                                         gint               pos);
static gboolean     animation_cel_animation_same        (Animation         *animation,
                                                         gint               previous_pos,
                                                         gint               next_pos);

/* Utils */

static void         animation_cel_animation_cleanup   (AnimationCelAnimation  *animation);
static void         animation_cel_animation_cache      (AnimationCelAnimation *animation,
                                                        gint                   position);
static gboolean     animation_cel_animation_cache_cmp (Cache *cache1,
                                                       Cache *cache2);

G_DEFINE_TYPE (AnimationCelAnimation, animation_cel_animation, ANIMATION_TYPE_ANIMATION)

#define parent_class animation_cel_animation_parent_class

static void
animation_cel_animation_class_init (AnimationCelAnimationClass *klass)
{
  GObjectClass   *object_class = G_OBJECT_CLASS (klass);
  AnimationClass *anim_class   = ANIMATION_CLASS (klass);

  object_class->finalize = animation_cel_animation_finalize;

  anim_class->get_length = animation_cel_animation_get_length;
  anim_class->load       = animation_cel_animation_load;
  anim_class->get_frame  = animation_cel_animation_get_frame;
  anim_class->same       = animation_cel_animation_same;

  g_type_class_add_private (klass, sizeof (AnimationCelAnimationPrivate));
}

static void
animation_cel_animation_init (AnimationCelAnimation *animation)
{
  animation->priv = G_TYPE_INSTANCE_GET_PRIVATE (animation,
                                                 ANIMATION_TYPE_CEL_ANIMATION,
                                                 AnimationCelAnimationPrivate);
}

static void
animation_cel_animation_finalize (GObject *object)
{
  animation_cel_animation_cleanup (ANIMATION_CEL_ANIMATION (object));

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

/**** Public Functions ****/

void
animation_cel_animation_set_comment (AnimationCelAnimation *animation,
                                     gint                   position,
                                     const gchar           *comment)
{
  GList *item;

  g_return_if_fail (position > 0 &&
                    position <= animation->priv->duration);

  item = g_list_nth (animation->priv->comments, position - 1);
  if (item->data)
    {
      g_free (item->data);
    }

  item->data = g_strdup (comment);
}

const gchar *
animation_cel_animation_get_comment (AnimationCelAnimation *animation,
                                     gint                   position)
{
  g_return_val_if_fail (position > 0 &&
                        position <= animation->priv->duration,
                        0);

  return g_list_nth_data (animation->priv->comments, position - 1);
}

/**** Virtual methods ****/

static gint
animation_cel_animation_get_length (Animation *animation)
{
  return ANIMATION_CEL_ANIMATION (animation)->priv->duration;
}

static void
animation_cel_animation_load (Animation *animation)
{
  AnimationCelAnimationPrivate *priv;
  Track                        *track;
  gint32                        image_id;
  gint32                        layer;
  gint                          i;

  priv = ANIMATION_CEL_ANIMATION (animation)->priv;

  /* Cleaning. */
  animation_cel_animation_cleanup (ANIMATION_CEL_ANIMATION (animation));

  /* Purely arbitrary value. User will anyway change it to suit one's needs. */
  priv->duration = 240;

  /* There are at least 2 tracks.
   * Second one is freely-named. */
  track = g_new0 (Track, 1);
  track->title = g_strdup (_("Name me"));
  priv->tracks = g_list_prepend (priv->tracks, track);
  /* The first track is called "Background". */
  track = g_new0 (Track, 1);
  track->title = g_strdup (_("Background"));
  priv->tracks = g_list_prepend (priv->tracks, track);

  /* If there is a layer named "Background", set it to all frames
   * on background track. */
  image_id = animation_get_image_id (animation);
  layer    = gimp_image_get_layer_by_name (image_id, _("Background"));
  if (layer > 0)
    {
      gint tattoo;

      tattoo = gimp_item_get_tattoo (layer);
      for (i = 0; i < priv->duration; i++)
        {
          track->frames = g_list_prepend (track->frames,
                                          GINT_TO_POINTER (tattoo));
        }
    }

  /* Finally cache. */
  for (i = 0; i < priv->duration; i++)
    {
      g_signal_emit_by_name (animation, "loading",
                             (gdouble) i / ((gdouble) priv->duration - 0.999));

      /* Panel image. */
      animation_cel_animation_cache (ANIMATION_CEL_ANIMATION (animation), i);
    }
}

static GeglBuffer *
animation_cel_animation_get_frame (Animation *animation,
                                   gint       pos)
{
  AnimationCelAnimation *cel_animation;
  Cache                 *cache;
  GeglBuffer            *frame = NULL;

  cel_animation = ANIMATION_CEL_ANIMATION (animation);

  cache = g_list_nth_data (cel_animation->priv->cache,
                           pos - 1);
  if (cache)
    {
      frame = g_object_ref (cache->buffer);
    }
  return frame;
}

static gboolean
animation_cel_animation_same (Animation *animation,
                              gint       pos1,
                              gint       pos2)
{
  AnimationCelAnimation *cel_animation;
  Cache                 *cache1;
  Cache                 *cache2;

  cel_animation = ANIMATION_CEL_ANIMATION (animation);

  g_return_val_if_fail (pos1 > 0                              &&
                        pos1 <= cel_animation->priv->duration &&
                        pos2 > 0                              &&
                        pos2 <= cel_animation->priv->duration,
                        FALSE);

  cache1 = g_list_nth_data (cel_animation->priv->cache,
                            pos1);
  cache2 = g_list_nth_data (cel_animation->priv->cache,
                            pos2);

  return animation_cel_animation_cache_cmp (cache1, cache2);
}

/**** Utils ****/

static void
animation_cel_animation_cleanup (AnimationCelAnimation *animation)
{
  AnimationCelAnimationPrivate *priv;
  GList                        *iter;

  priv = ANIMATION_CEL_ANIMATION (animation)->priv;

  if (priv->cache)
    {
      for (iter = priv->cache; iter; iter = iter->next)
        {
          if (iter->data)
            {
              Cache *cache = iter->data;

              if (--(cache->refs) == 0)
                {
                  g_object_unref (cache->buffer);
                  g_free (cache->composition);
                }
            }
        }
      g_list_free (priv->cache);
    }
  if (priv->comments)
    {
      for (iter = priv->comments; iter; iter = iter->next)
        {
          if (iter->data)
            {
              g_free (iter->data);
            }
        }
      g_list_free (priv->comments);
    }
  if (priv->tracks)
    {
      for (iter = priv->tracks; iter; iter = iter->next)
        {
          Track *track = iter->data;

          /* The item's data must always exist. */
          g_free (track->title);
          g_list_free (track->frames);
        }
      g_list_free (priv->tracks);
    }
  priv->cache    = NULL;
  priv->comments = NULL;
  priv->tracks   = NULL;
}

static void
animation_cel_animation_cache (AnimationCelAnimation *animation,
                               gint                   pos)
{
  GeglBuffer *backdrop = NULL;
  GList      *iter;
  Cache      *cache;
  gint       *composition;
  gint        n_sources = 0;
  gint32      image_id;
  gdouble     proxy_ratio;
  gint        preview_width;
  gint        preview_height;
  gint        i;

  /* Clean out current cache. */
  iter = g_list_nth (animation->priv->cache, pos);
  if (iter && iter->data)
    {
      Cache *cache = iter->data;

      if (--(cache->refs) == 0)
        {
          g_free (cache->composition);
          g_object_unref (cache->buffer);
          g_free (cache);
        }
      iter->data = NULL;
    }

  /* Check if new configuration needs caching. */
  for (iter = animation->priv->tracks; iter; iter = iter->next)
    {
      Track *track = iter->data;

      if (GPOINTER_TO_INT (g_list_nth_data (track->frames, pos)))
        {
          n_sources++;
        }
    }
  if (n_sources == 0)
    {
      return;
    }

  /* Make sure the cache list is long enough. */
  if (pos >= g_list_length (animation->priv->cache))
    {
      animation->priv->cache = g_list_reverse (animation->priv->cache);
      for (i = g_list_length (animation->priv->cache); i <= pos; i++)
        {
          animation->priv->cache = g_list_prepend (animation->priv->cache,
                                                   NULL);
        }
      animation->priv->cache = g_list_reverse (animation->priv->cache);
    }

  /* Create the new buffer composition. */
  composition = g_new0 (int, n_sources);
  i = 0;
  for (iter = animation->priv->tracks; iter; iter = iter->next)
    {
      Track *track = iter->data;
      gint   tattoo;

      tattoo = GPOINTER_TO_INT (g_list_nth_data (track->frames, pos));
      if (tattoo)
        {
          composition[i++] = tattoo;
        }
    }

  /* Check if new configuration was not already cached. */
  for (iter = animation->priv->cache; iter; iter = iter->next)
    {
      if (iter->data)
        {
          Cache    *cache = iter->data;
          gboolean  same = FALSE;

          if (n_sources == cache->n_sources)
            {
              same = TRUE;
              for (i = 0; i < n_sources; i++)
                {
                  if (cache->composition[i] != composition[i])
                    {
                      same = FALSE;
                      break;
                    }
                }
              if (same)
                {
                  /* A buffer with the same contents already exists. */
                  g_free (composition);
                  (cache->refs)++;
                  g_list_nth (animation->priv->cache, pos)->data = cache;
                  return;
                }
            }
        }
    }

  /* New configuration. Finally compute the cache. */
  cache = g_new0 (Cache, 1);
  cache->refs        = 1;
  cache->n_sources   = n_sources;
  cache->composition = composition;

  image_id    = animation_get_image_id (ANIMATION (animation));
  proxy_ratio = animation_get_proxy (ANIMATION (animation));
  animation_get_size (ANIMATION (animation),
                      &preview_width, &preview_height);

  for (i = 0; i < n_sources; i++)
    {
      GeglBuffer *source;
      GeglBuffer *intermediate;
      gint32      layer;
      gint        layer_offx;
      gint        layer_offy;

      layer = gimp_image_get_layer_by_tattoo (image_id,
                                              cache->composition[i]);
      source = gimp_drawable_get_buffer (layer);
      gimp_drawable_offsets (layer,
                             &layer_offx, &layer_offy);
      intermediate = normal_blend (preview_width, preview_height,
                                   backdrop, 1.0, 0, 0,
                                   source, proxy_ratio,
                                   layer_offx, layer_offy);
      g_object_unref (source);
      if (backdrop)
        {
          g_object_unref (backdrop);
        }
      backdrop = intermediate;
    }
  cache->buffer = backdrop;

  /* This item exists and has a NULL data. */
  g_list_nth (animation->priv->cache, pos)->data = cache;
}


static gboolean
animation_cel_animation_cache_cmp (Cache *cache1,
                                   Cache *cache2)
{
  gboolean identical = FALSE;

  if (cache1 && cache2 &&
      cache1->n_sources == cache2->n_sources)
    {
      gint i;

      identical = TRUE;
      for (i = 0; i < cache1->n_sources; i++)
        {
          if (cache1->composition[i] != cache2->composition[i])
            {
              identical = FALSE;
              break;
            }
        }
    }
  return identical;
}
