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

#include "animation.h"
#include "animation-camera.h"

#include "animation-celanimation.h"

typedef struct _AnimationCelAnimationPrivate AnimationCelAnimationPrivate;

typedef struct
{
  gchar *title;
  /* The list of list of layers (identified by their tattoos). */
  GList *frames;
}
Track;

typedef struct
{
  gint   tattoo;
  gint   offset_x;
  gint   offset_y;
}
CompLayer;

typedef struct
{
  GeglBuffer *buffer;

  /* The list of layers (identified by their tattoos) composited into
   * this buffer allows to easily compare caches so that to not
   * duplicate them.*/
  CompLayer  *composition;
  gint        n_sources;

  gint        refs;
}
Cache;

struct _AnimationCelAnimationPrivate
{
  /* The number of frames. */
  gint             duration;

  /* Frames are cached as GEGL buffers. */
  GList           *cache;

  /* Panel comments. */
  GList           *comments;

  /* List of tracks/levels.
   * The background is a special-named track, always present
   * and first.
   * There is always at least 1 additional track. */
  GList           *tracks;

  /* The globale camera. */
  AnimationCamera *camera;
};

typedef enum
{
  START_STATE,
  ANIMATION_STATE,
  PLAYBACK_STATE,
  SEQUENCE_STATE,
  FRAME_STATE,
  LAYER_STATE,
  COMMENTS_STATE,
  COMMENT_STATE,
  END_STATE
} AnimationParseState;

typedef struct
{
  Animation           *animation;
  AnimationParseState  state;

  Track               *track;
  gint                 frame_position;
  gint                 frame_duration;
} ParseStatus;

#define GET_PRIVATE(animation) \
        G_TYPE_INSTANCE_GET_PRIVATE (animation, \
                                     ANIMATION_TYPE_CEL_ANIMATION, \
                                     AnimationCelAnimationPrivate)

static void         animation_cel_animation_finalize       (GObject           *object);

/* Virtual methods */

static gint         animation_cel_animation_get_duration   (Animation         *animation);

static GeglBuffer * animation_cel_animation_get_frame      (Animation         *animation,
                                                            gint               pos);

static gboolean     animation_cel_animation_same           (Animation         *animation,
                                                            gint               previous_pos,
                                                            gint               next_pos);

static void         animation_cel_animation_purge_cache    (Animation         *animation);

static void         animation_cel_animation_reset_defaults (Animation         *animation);
static gchar      * animation_cel_animation_serialize      (Animation         *animation,
                                                            const gchar       *playback_xml);
static gboolean     animation_cel_animation_deserialize    (Animation         *animation,
                                                            const gchar       *xml,
                                                            GError           **error);

/* XML parsing */

static void      animation_cel_animation_start_element     (GMarkupParseContext *context,
                                                            const gchar         *element_name,
                                                            const gchar        **attribute_names,
                                                            const gchar        **attribute_values,
                                                            gpointer             user_data,
                                                            GError             **error);
static void      animation_cel_animation_end_element       (GMarkupParseContext *context,
                                                            const gchar         *element_name,
                                                            gpointer             user_data,
                                                            GError             **error);

static void      animation_cel_animation_text              (GMarkupParseContext  *context,
                                                            const gchar          *text,
                                                            gsize                 text_len,
                                                            gpointer              user_data,
                                                            GError              **error);

/* Signal handling */

static void      on_camera_offsets_changed                 (AnimationCamera        *camera,
                                                            gint                    position,
                                                            gint                    duration,
                                                            AnimationCelAnimation  *animation);
/* Utils */

static void         animation_cel_animation_cache          (AnimationCelAnimation  *animation,
                                                            gint                    position);
static void         animation_cel_animation_cleanup        (AnimationCelAnimation  *animation);
static gboolean     animation_cel_animation_cache_cmp      (Cache                   *cache1,
                                                            Cache                   *cache2);
static void         animation_cel_animation_clean_cache    (Cache                   *cache);
static void         animation_cel_animation_clean_track    (Track                   *track);

G_DEFINE_TYPE (AnimationCelAnimation, animation_cel_animation, ANIMATION_TYPE_ANIMATION)

#define parent_class animation_cel_animation_parent_class

static void
animation_cel_animation_class_init (AnimationCelAnimationClass *klass)
{
  GObjectClass   *object_class = G_OBJECT_CLASS (klass);
  AnimationClass *anim_class   = ANIMATION_CLASS (klass);

  object_class->finalize     = animation_cel_animation_finalize;

  anim_class->get_duration   = animation_cel_animation_get_duration;
  anim_class->get_frame      = animation_cel_animation_get_frame;
  anim_class->same           = animation_cel_animation_same;

  anim_class->purge_cache    = animation_cel_animation_purge_cache;

  anim_class->reset_defaults = animation_cel_animation_reset_defaults;
  anim_class->serialize      = animation_cel_animation_serialize;
  anim_class->deserialize    = animation_cel_animation_deserialize;

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
animation_cel_animation_set_layers (AnimationCelAnimation *animation,
                                    gint                   level,
                                    gint                   position,
                                    const GList           *new_layers)
{
  Track *track;

  track = g_list_nth_data (animation->priv->tracks, level);

  if (track)
    {
      GList *layers = g_list_nth (track->frames, position);

      if (! layers)
        {
          gint frames_length = g_list_length (track->frames);
          gint i;

          track->frames = g_list_reverse (track->frames);
          for (i = frames_length; i < position + 1; i++)
            {
              track->frames = g_list_prepend (track->frames, NULL);
              layers = track->frames;
            }
          track->frames = g_list_reverse (track->frames);
        }
      /* Clean out previous layer list. */
      g_list_free (layers->data);
      if (new_layers)
        {
          layers->data = g_list_copy ((GList *) new_layers);
        }
      else
        {
          layers->data = NULL;
        }
      animation_cel_animation_cache (animation, position);
      g_signal_emit_by_name (animation, "cache-invalidated", position, 1);
    }
}

const GList *
animation_cel_animation_get_layers (AnimationCelAnimation *animation,
                                    gint                   level,
                                    gint                   position)
{
  GList *layers = NULL;
  Track *track;

  track = g_list_nth_data (animation->priv->tracks, level);

  if (track)
    {
      layers = g_list_nth_data (track->frames, position);
    }

  return layers;
}

void
animation_cel_animation_set_comment (AnimationCelAnimation *animation,
                                     gint                   position,
                                     const gchar           *comment)
{
  GList *item;

  g_return_if_fail (position >= 0 &&
                    position < animation->priv->duration);

  item = g_list_nth (animation->priv->comments, position);
  if (item && item->data)
    {
      g_free (item->data);
    }
  else if (! item)
    {
      gint length = g_list_length (animation->priv->comments);
      gint i;

      animation->priv->comments = g_list_reverse (animation->priv->comments);
      for (i = length; i < position + 1; i++)
        {
          animation->priv->comments = g_list_prepend (animation->priv->comments, NULL);
          item = animation->priv->comments;
        }
      animation->priv->comments = g_list_reverse (animation->priv->comments);
    }

  item->data = g_strdup (comment);
}

const gchar *
animation_cel_animation_get_comment (AnimationCelAnimation *animation,
                                     gint                   position)
{
  g_return_val_if_fail (position >= 0 &&
                        position < animation->priv->duration,
                        0);

  return g_list_nth_data (animation->priv->comments, position);
}

void
animation_cel_animation_set_duration (AnimationCelAnimation *animation,
                                      gint                   duration)
{
  if (duration < animation->priv->duration)
    {
      GList *iter;

      /* Free memory. */
      iter = g_list_nth (animation->priv->cache, duration);
      if (iter && iter->prev)
        {
          iter->prev->next = NULL;
          iter->prev = NULL;
        }
      g_list_free_full (iter, (GDestroyNotify) animation_cel_animation_clean_cache);

      iter = g_list_nth (animation->priv->tracks, duration);
      if (iter && iter->prev)
        {
          iter->prev->next = NULL;
          iter->prev = NULL;
        }
      g_list_free_full (iter, (GDestroyNotify) animation_cel_animation_clean_track);

      iter = g_list_nth (animation->priv->comments, duration);
      if (iter && iter->prev)
        {
          iter->prev->next = NULL;
          iter->prev = NULL;
        }
      g_list_free_full (iter, (GDestroyNotify) g_free);

      for (iter = animation->priv->tracks; iter; iter = iter->next)
        {
          Track *track = iter->data;
          GList *iter2;

          iter2 = g_list_nth (track->frames, duration);
          if (iter2 && iter2->prev)
            {
              iter2->prev->next = NULL;
              iter2->prev = NULL;
            }
          g_list_free_full (iter2, (GDestroyNotify) g_list_free);
        }
    }

  if (duration != animation->priv->duration)
    {
      animation->priv->duration = duration;
      g_signal_emit_by_name (animation, "duration-changed",
                             duration);
    }
}

GObject *
animation_cel_animation_get_main_camera (AnimationCelAnimation *animation)
{
  return G_OBJECT (animation->priv->camera);
}

gint
animation_cel_animation_get_levels (AnimationCelAnimation *animation)
{
  return g_list_length (animation->priv->tracks);
}

gint
animation_cel_animation_level_up (AnimationCelAnimation *animation,
                                  gint                   level)
{
  gint tracks_n = g_list_length (animation->priv->tracks);

  g_return_val_if_fail (level >= 0 && level < tracks_n, level);

  if (level < tracks_n - 1)
    {
      GList *item = g_list_nth (animation->priv->tracks, level);
      GList *prev = item->prev;
      GList *next = item->next;
      Track *track;
      GList *iter;
      gint   i;

      if (prev)
        prev->next = next;
      else
        animation->priv->tracks = next;
      next->prev = prev;
      item->prev = next;
      item->next = next->next;
      next->next = item;

      level++;

      track = item->data;
      iter  = track->frames;
      for (i = 0; iter; iter = iter->next, i++)
        {
          g_signal_emit_by_name (animation, "loading",
                                 (gdouble) i / ((gdouble) animation->priv->duration - 0.999));

          if (iter->data)
            {
              /* Only cache if the track had contents for this frame. */
              animation_cel_animation_cache (animation, i);
            }
        }
      g_signal_emit_by_name (animation, "cache-invalidated",
                             0, g_list_length (track->frames));
      g_signal_emit_by_name (animation, "loaded");
    }

  return level;
}

gint
animation_cel_animation_level_down (AnimationCelAnimation *animation,
                                    gint                   level)
{
  gint tracks_n = g_list_length (animation->priv->tracks);

  g_return_val_if_fail (level >= 0 && level < tracks_n, level);

  if (level > 0)
    {
      GList *item = g_list_nth (animation->priv->tracks, level);
      GList *prev = item->prev;
      GList *next = item->next;
      Track *track;
      GList *iter;
      gint   i;

      if (! prev->prev)
        animation->priv->tracks = item;
      if (next)
        next->prev = prev;
      prev->next = next;
      item->next = prev;
      item->prev = prev->prev;
      prev->prev = item;

      level--;

      track = item->data;
      iter  = track->frames;
      for (i = 0; iter; iter = iter->next, i++)
        {
          g_signal_emit_by_name (animation, "loading",
                                 (gdouble) i / ((gdouble) animation->priv->duration - 0.999));

          if (iter->data)
            {
              /* Only cache if the track had contents for this frame. */
              animation_cel_animation_cache (animation, i);
            }
        }
      g_signal_emit_by_name (animation, "cache-invalidated",
                             0, g_list_length (track->frames));
      g_signal_emit_by_name (animation, "loaded");
    }

  return level;
}

gboolean
animation_cel_animation_level_delete (AnimationCelAnimation *animation,
                                      gint                   level)
{
  gint   tracks_n = g_list_length (animation->priv->tracks);
  GList *item;
  GList *iter;
  Track *track;
  gint   i;

  g_return_val_if_fail (level >= 0 && level < tracks_n, FALSE);

  /* Do not remove when there is only a single level. */
  if (tracks_n > 1)
    {
      item = g_list_nth (animation->priv->tracks, level);
      track = item->data;
      animation->priv->tracks = g_list_delete_link (animation->priv->tracks, item);

      iter = track->frames;
      for (i = 0; iter; iter = iter->next, i++)
        {
          g_signal_emit_by_name (animation, "loading",
                                 (gdouble) i / ((gdouble) animation->priv->duration - 0.999));

          if (iter->data)
            {
              /* Only cache if the track had contents for this frame. */
              animation_cel_animation_cache (animation, i);
            }
        }
      g_signal_emit_by_name (animation, "cache-invalidated",
                             0, g_list_length (track->frames));
      g_signal_emit_by_name (animation, "loaded");
      animation_cel_animation_clean_track (track);

      return TRUE;
    }
  return FALSE;
}

gboolean
animation_cel_animation_level_add (AnimationCelAnimation *animation,
                                   gint                   level)
{
  Track *track;
  gint   tracks_n = g_list_length (animation->priv->tracks);

  g_return_val_if_fail (level >= 0 && level <= tracks_n, FALSE);

  track = g_new0 (Track, 1);
  track->title = g_strdup (_("Name me"));
  animation->priv->tracks = g_list_insert (animation->priv->tracks,
                                           track, level);

  return TRUE;
}

const gchar *
animation_cel_animation_get_track_title (AnimationCelAnimation *animation,
                                         gint                   level)
{
  gchar *title = NULL;
  GList *track;

  track = g_list_nth (animation->priv->tracks, level);

  if (track)
    {
      title = ((Track *) track->data)->title;
    }

  return title;
}

void
animation_cel_animation_set_track_title (AnimationCelAnimation *animation,
                                         gint                   level,
                                         const gchar           *title)
{
  GList *track;

  track = g_list_nth (animation->priv->tracks, level);

  if (track)
    {
      g_free (((Track *) track->data)->title);
      ((Track *) track->data)->title = g_strdup (title);
    }
}

gboolean
animation_cel_animation_cel_delete (AnimationCelAnimation *animation,
                                    gint                   level,
                                    gint                   position)
{
  Track *track;

  track = g_list_nth_data (animation->priv->tracks, level);

  if (track)
    {
      GList *cel = g_list_nth (track->frames, position);

      if (cel)
        {
          GList *iter;
          gint   i;

          g_list_free (cel->data);
          iter = cel->next;
          track->frames = g_list_delete_link (track->frames, cel);

          for (i = position; iter; iter = iter->next, i++)
            {
              g_signal_emit_by_name (animation, "loading",
                                     (gdouble) i / ((gdouble) animation->priv->duration - 0.999));

              animation_cel_animation_cache (animation, i);
            }
          if (i > position)
            g_signal_emit_by_name (animation, "cache-invalidated",
                                   position, i - position);
          g_signal_emit_by_name (animation, "loaded");

          return TRUE;
        }
    }
  return FALSE;
}

gboolean
animation_cel_animation_cel_add (AnimationCelAnimation *animation,
                                 gint                   level,
                                 gint                   position,
                                 gboolean               dup_previous)
{
  Track *track;

  track = g_list_nth_data (animation->priv->tracks, level);

  if (track)
    {
      GList *cel;
      GList *contents = NULL;
      gint   i = position;

      if (dup_previous && position > 0)
        {
          GList *prev_cell;

          i++;
          prev_cell = g_list_nth (track->frames, position - 1);

          if (prev_cell)
            contents = g_list_copy (prev_cell->data);
        }
      track->frames = g_list_insert (track->frames, contents, position);

      if (g_list_length (track->frames) > animation->priv->duration &&
          g_list_last (track->frames)->data)
        animation_cel_animation_set_duration (animation,
                                              g_list_length (track->frames));
      cel = g_list_nth (track->frames, i);
      if (cel)
        {
          for (; cel; cel = cel->next, i++)
            {
              g_signal_emit_by_name (animation, "loading",
                                     (gdouble) i / ((gdouble) animation->priv->duration - 0.999));

              animation_cel_animation_cache (animation, i);
            }
          if (i > position)
            g_signal_emit_by_name (animation, "cache-invalidated",
                                   position, i - position);
          g_signal_emit_by_name (animation, "loaded");

          return TRUE;
        }
    }
  return FALSE;
}

/**** Virtual methods ****/

static gint
animation_cel_animation_get_duration (Animation *animation)
{
  return ANIMATION_CEL_ANIMATION (animation)->priv->duration;
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
                           pos);
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

  g_return_val_if_fail (pos1 >= 0                            &&
                        pos1 < cel_animation->priv->duration &&
                        pos2 >= 0                            &&
                        pos2 < cel_animation->priv->duration,
                        FALSE);

  cache1 = g_list_nth_data (cel_animation->priv->cache, pos1);
  cache2 = g_list_nth_data (cel_animation->priv->cache, pos2);

  return animation_cel_animation_cache_cmp (cache1, cache2);
}

static void
animation_cel_animation_purge_cache (Animation *animation)
{
  gint duration;
  gint i;

  duration = animation_get_duration (animation);
  for (i = 0; i < duration; i++)
    {
      animation_cel_animation_cache (ANIMATION_CEL_ANIMATION (animation),
                                     i);
      g_signal_emit_by_name (animation, "loading",
                             (gdouble) i / ((gdouble) duration - 0.999));
    }
  g_signal_emit_by_name (animation, "loaded");
}

static void
animation_cel_animation_reset_defaults (Animation *animation)
{
  AnimationCelAnimationPrivate *priv;
  Track                        *track;
  gint32                        image_id;
  gint32                        layer;
  gint                          i;

  priv = ANIMATION_CEL_ANIMATION (animation)->priv;
  animation_cel_animation_cleanup (ANIMATION_CEL_ANIMATION (animation));

  /* Purely arbitrary value. User will anyway change it to suit one's needs. */
  priv->duration = 240;

  /* There are at least 2 tracks. Second one is freely-named. */
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
          GList *layers = NULL;

          layers = g_list_prepend (layers,
                                   GINT_TO_POINTER (tattoo));
          track->frames = g_list_prepend (track->frames,
                                          layers);
        }
    }

  priv->camera = animation_camera_new (animation);
  g_signal_connect (priv->camera, "offsets-changed",
                    G_CALLBACK (on_camera_offsets_changed),
                    animation);
}

static gchar *
animation_cel_animation_serialize (Animation   *animation,
                                   const gchar *playback_xml)
{
  AnimationCelAnimationPrivate *priv;
  gchar                        *xml;
  gchar                        *xml2;
  gchar                        *tmp;
  GList                        *iter;
  gint                          i;

  priv = ANIMATION_CEL_ANIMATION (animation)->priv;

  xml = g_strdup_printf ("<animation type=\"cels\" framerate=\"%f\" "
                          " duration=\"%d\" width=\"\" height=\"\">%s",
                          animation_get_framerate (animation),
                          priv->duration, playback_xml);

  for (iter = priv->tracks; iter; iter = iter->next)
    {
      Track *track = iter->data;
      GList *iter2;
      gint   pos;
      gint   duration;

      xml2 = g_markup_printf_escaped ("<sequence name=\"%s\">",
                                      track->title);
      tmp = xml;
      xml = g_strconcat (xml, xml2, NULL);
      g_free (tmp);
      g_free (xml2);

      pos = 1;
      duration = 0;
      for (iter2 = track->frames; iter2; iter2 = iter2->next)
        {
          GList *layers = iter2->data;

          if (layers)
            {
              gboolean next_identical = FALSE;

              duration++;

              if (iter2->next && iter2->next->data &&
                  g_list_length (layers) == g_list_length (iter2->next->data))
                {
                  GList *layer1 = layers;
                  GList *layer2 = iter2->next->data;

                  next_identical = TRUE;
                  for (; layer1; layer1 = layer1->next, layer2 = layer2->next)
                    {
                      if (layer1->data != layer2->data)
                        {
                          next_identical = FALSE;
                          break;
                        }
                    }
                }
              if (! next_identical)
                {
                  /* Open tag. */
                  xml2 = g_markup_printf_escaped ("<frame position=\"%d\""
                                                  " duration=\"%d\">",
                                                  pos - duration, duration);
                  tmp = xml;
                  xml = g_strconcat (xml, xml2, NULL);
                  g_free (tmp);
                  g_free (xml2);

                  for (; layers; layers = layers->next)
                    {
                      gint tattoo = GPOINTER_TO_INT (layers->data);

                      xml2 = g_markup_printf_escaped ("<layer id=\"%d\"/>",
                                                      tattoo);
                      tmp = xml;
                      xml = g_strconcat (xml, xml2, NULL);
                      g_free (tmp);
                      g_free (xml2);
                    }

                  /* End tag. */
                  xml2 = g_markup_printf_escaped ("</frame>");
                  tmp = xml;
                  xml = g_strconcat (xml, xml2, NULL);
                  g_free (tmp);
                  g_free (xml2);

                  duration = 0;
                }
            }
          pos++;
        }

      tmp = xml;
      xml = g_strconcat (xml, "</sequence>", NULL);
      g_free (tmp);
    }

  tmp = xml;
  xml = g_strconcat (xml, "<comments title=\"\">", NULL);
  g_free (tmp);

  /* New loop for comments. */
  for (iter = priv->comments, i = 0; iter; iter = iter->next, i++)
    {
      if (iter->data && strlen (iter->data) > 0)
        {
          gchar *comment = iter->data;

          xml2 = g_markup_printf_escaped ("<comment frame-position=\"%d\">%s</comment>",
                                          i, comment);
          tmp = xml;
          xml = g_strconcat (xml, xml2, NULL);
          g_free (tmp);
          g_free (xml2);
        }
    }
  tmp = xml;
  xml = g_strconcat (xml, "</comments></animation>", NULL);
  g_free (tmp);

  return xml;
}

static gboolean
animation_cel_animation_deserialize (Animation    *animation,
                                     const gchar  *xml,
                                     GError      **error)
{
  const GMarkupParser markup_parser =
    {
      animation_cel_animation_start_element,
      animation_cel_animation_end_element,
      animation_cel_animation_text,
      NULL,  /*  passthrough  */
      NULL   /*  error        */
    };
  GMarkupParseContext *context;
  ParseStatus          status = { 0, };

  g_return_val_if_fail (xml != NULL && *error == NULL, FALSE);

  /* Parse XML to update. */
  status.state = START_STATE;
  status.animation = animation;

  context = g_markup_parse_context_new (&markup_parser,
                                        0, &status, NULL);
  g_markup_parse_context_parse (context, xml, strlen (xml), error);
  if (*error == NULL)
    {
      AnimationCelAnimation *cel_animation;

      g_markup_parse_context_end_parse (context, error);

      cel_animation = ANIMATION_CEL_ANIMATION (animation);
      /* Reverse track order. */
      cel_animation->priv->tracks = g_list_reverse (cel_animation->priv->tracks);

      /* TODO: just testing right now. I will have to add actual
       * (de)serialization, otherwise there is no persistency of
       * camera works.
       */
      cel_animation->priv->camera = animation_camera_new (animation);
      g_signal_connect (cel_animation->priv->camera, "offsets-changed",
                        G_CALLBACK (on_camera_offsets_changed),
                        animation);
    }
  g_markup_parse_context_free (context);

  return (*error == NULL);
}

static void
animation_cel_animation_start_element (GMarkupParseContext  *context,
                                       const gchar          *element_name,
                                       const gchar         **attribute_names,
                                       const gchar         **attribute_values,
                                       gpointer              user_data,
                                       GError              **error)
{
  const gchar                  **names  = attribute_names;
  const gchar                  **values = attribute_values;
  ParseStatus                   *status = (ParseStatus *) user_data;
  AnimationCelAnimation         *animation = ANIMATION_CEL_ANIMATION (status->animation);
  AnimationCelAnimationPrivate  *priv   = GET_PRIVATE (status->animation);

  switch (status->state)
    {
    case START_STATE:
      if (g_strcmp0 (element_name, "animation") != 0)
        {
          g_set_error (error, 0, 0,
                       _("Tag <animation> expected. "
                         "Got \"%s\" instead."),
                       element_name);
          return;
        }
      while (*names && *values)
        {
          if (strcmp (*names, "type") == 0)
            {
              if (! **values || strcmp (*values, "cels") != 0)
                {
                  g_set_error (error, 0, 0,
                               _("Unknown animation type: \"%s\"."),
                               *values);
                  return;
                }
            }
          else if (strcmp (*names, "framerate") == 0 && **values)
            {
              gdouble fps = g_strtod (*values, NULL);
              if (fps >= MAX_FRAMERATE)
                {
                  /* Let's avoid huge frame rates. */
                  fps = MAX_FRAMERATE;
                }
              else if (fps <= 0)
                {
                  /* Null or negative framerates are impossible. */
                  fps = DEFAULT_FRAMERATE;
                }
              animation_set_framerate (status->animation, fps);
            }
          else if (strcmp (*names, "duration") == 0 && **values)
            {
              gint duration = (gint) g_ascii_strtoull (*values, NULL, 10);

              animation_cel_animation_set_duration (animation, duration);
            }

          names++;
          values++;
        }
      status->state = ANIMATION_STATE;
      break;
    case ANIMATION_STATE:
      if (g_strcmp0 (element_name, "sequence") == 0)
        {
          status->track = g_new0 (Track, 1);
          while (*names && *values)
            {
              if (strcmp (*names, "name") == 0)
                {
                  status->track->title = g_strdup (*values);
                }
              names++;
              values++;
            }
          priv->tracks = g_list_prepend (priv->tracks, status->track);
          status->state = SEQUENCE_STATE;
        }
      else if (g_strcmp0 (element_name, "comments") == 0)
        {
          status->state = COMMENTS_STATE;
        }
      else if (g_strcmp0 (element_name, "playback") == 0)
        {
          status->state = PLAYBACK_STATE;
        }
      else
        {
          g_set_error (error, 0, 0,
                       _("Tags <sequence> or <comments> expected. "
                         "Got \"%s\" instead."),
                       element_name);
          return;
        }
      break;
    case PLAYBACK_STATE:
      /* Leave processing to the playback. */
      break;
    case SEQUENCE_STATE:
      if (g_strcmp0 (element_name, "frame") != 0)
        {
          g_set_error (error, 0, 0,
                       _("Tag <frame> expected. "
                         "Got \"%s\" instead."),
                       element_name);
          return;
        }
      status->frame_position = -1;
      status->frame_duration = -1;

      while (*names && *values)
        {
          if (strcmp (*names, "position") == 0 && **values)
            {
              gint position = g_ascii_strtoll (*values, NULL, 10);

              if (position >= 0)
                status->frame_position = position;
            }
          else if (strcmp (*names, "duration") == 0 && **values)
            {
              gint duration = g_ascii_strtoll (*values, NULL, 10);

              if (duration > 0)
                status->frame_duration = duration;
            }

          names++;
          values++;
        }
      if (status->frame_position == -1 ||
          status->frame_duration == -1)
        {
          g_set_error (error, 0, 0,
                       _("Tag <frame> expects the properties: "
                         "position, duration."));
        }
      status->state = FRAME_STATE;
      break;
    case FRAME_STATE:
      if (g_strcmp0 (element_name, "layer") != 0)
        {
          g_set_error (error, 0, 0,
                       _("Tag <layer> expected. "
                         "Got \"%s\" instead."),
                       element_name);
          return;
        }
      while (*names && *values)
        {
          if (strcmp (*names, "id") == 0 && **values)
            {
              GList *iter;
              gint   tattoo = g_ascii_strtoll (*values, NULL, 10);
              gint   track_length;
              gint   i;

              track_length = g_list_length (status->track->frames);

              if (track_length < status->frame_position + status->frame_duration)
                {
                  /* Make sure the list is long enough. */
                  status->track->frames = g_list_reverse (status->track->frames);
                  for (i = track_length; i < status->frame_position + status->frame_duration; i++)
                    {
                      status->track->frames = g_list_prepend (status->track->frames,
                                                              NULL);
                    }
                  status->track->frames = g_list_reverse (status->track->frames);
                }
              iter = status->track->frames;
              for (i = 0; i < status->frame_position + status->frame_duration; i++)
                {
                  if (i >= status->frame_position)
                    {
                      GList *layers = iter->data;

                      layers = g_list_append (layers,
                                              GINT_TO_POINTER (tattoo));
                      iter->data = layers;
                    }
                  iter = iter->next;
                }
            }

          names++;
          values++;
        }
      status->state = LAYER_STATE;
      break;
    case LAYER_STATE:
      /* <layer> should have no child tag. */
      g_set_error (error, 0, 0,
                   _("Unexpected child of <layer>: \"%s\"."),
                   element_name);
      return;
    case COMMENTS_STATE:
      if (g_strcmp0 (element_name, "comment") != 0)
        {
          g_set_error (error, 0, 0,
                       _("Tag <comment> expected. "
                         "Got \"%s\" instead."),
                       element_name);
          return;
        }
      status->frame_position = -1;
      status->frame_duration = -1;

      while (*names && *values)
        {
          if (strcmp (*names, "frame-position") == 0 && **values)
            {
              gint position = (gint) g_ascii_strtoll (*values, NULL, 10);

              status->frame_position = position;
              break;
            }

          names++;
          values++;
        }
      status->state = COMMENT_STATE;
      break;
    case COMMENT_STATE:
      /* <comment> should have no child tag. */
      g_set_error (error, 0, 0,
                   _("Unexpected child of <comment>: <\"%s\">."),
                   element_name);
      return;
    default:
      g_set_error (error, 0, 0,
                   _("Unknown state!"));
      break;
    }
}

static void
animation_cel_animation_end_element (GMarkupParseContext *context,
                                     const gchar         *element_name,
                                     gpointer             user_data,
                                     GError             **error)
{
  ParseStatus *status = (ParseStatus *) user_data;

  switch (status->state)
    {
    case SEQUENCE_STATE:
    case COMMENTS_STATE:
    case PLAYBACK_STATE:
      status->state = ANIMATION_STATE;
      break;
    case FRAME_STATE:
      status->state = SEQUENCE_STATE;
      break;
    case LAYER_STATE:
      status->state = FRAME_STATE;
      break;
    case ANIMATION_STATE:
      status->state = END_STATE;
      break;
    case COMMENT_STATE:
      status->state = COMMENTS_STATE;
      break;
    default: /* START/END_STATE */
      /* invalid XML. I expect the parser to raise an error anyway.*/
      break;
    }
}

static void
animation_cel_animation_text (GMarkupParseContext  *context,
                              const gchar          *text,
                              gsize                 text_len,
                              gpointer              user_data,
                              GError              **error)
{
  ParseStatus *status = (ParseStatus *) user_data;
  AnimationCelAnimation *cel_animation = ANIMATION_CEL_ANIMATION (status->animation);

  switch (status->state)
    {
    case COMMENT_STATE:
      if (status->frame_position == -1)
        /* invalid comment tag. */
        break;
      /* Setting comment to a frame. */
      animation_cel_animation_set_comment (cel_animation,
                                           status->frame_position,
                                           text);
      status->frame_position = -1;
      break;
    default:
      /* Ignoring text everywhere else. */
      break;
    }
}

/**** Signal handling ****/

static void
on_camera_offsets_changed (AnimationCamera       *camera,
                           gint                   position,
                           gint                   duration,
                           AnimationCelAnimation *animation)
{
  gint i;

  for (i = position; i < position + duration; i++)
    animation_cel_animation_cache (animation, i);

  g_signal_emit_by_name (animation, "cache-invalidated",
                         position, duration);
}

/**** Utils ****/

static void
animation_cel_animation_cache (AnimationCelAnimation *animation,
                               gint                   pos)
{
  GeglBuffer *backdrop = NULL;
  GList      *iter;
  Cache      *cache;
  CompLayer  *composition;
  gint        n_sources = 0;
  gint32      image_id;
  gdouble     proxy_ratio;
  gint        preview_width;
  gint        preview_height;
  gint        i;
  gint        main_offset_x;
  gint        main_offset_y;

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
      GList *layers;

      layers = g_list_nth_data (track->frames, pos);

      n_sources += g_list_length (layers);
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

  animation_camera_get (animation->priv->camera,
                        pos, &main_offset_x, &main_offset_y);

  /* Create the new buffer composition. */
  composition = g_new0 (CompLayer, n_sources);
  i = 0;
  for (iter = animation->priv->tracks; iter; iter = iter->next)
    {
      Track *track = iter->data;
      GList *layers;
      GList *layer;

      layers = g_list_nth_data (track->frames, pos);

      for (layer = layers; layer; layer = layer->next)
        {
          gint tattoo;

          tattoo = GPOINTER_TO_INT (layer->data);
          if (tattoo)
            {
              composition[i].tattoo = tattoo;
              composition[i].offset_x = main_offset_x;
              composition[i++].offset_y = main_offset_y;
            }
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
                  if (cache->composition[i].tattoo   != composition[i].tattoo   ||
                      cache->composition[i].offset_x != composition[i].offset_x ||
                      cache->composition[i].offset_y != composition[i].offset_y)
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
                                              cache->composition[i].tattoo);
      if (layer > 0)
        source = gimp_drawable_get_buffer (layer);
      if (layer <= 0 || ! source)
        {
          g_printerr ("Warning: a layer used for frame %d has been deleted.\n",
                      pos);
          continue;
        }
      gimp_drawable_offsets (layer, &layer_offx, &layer_offy);
      intermediate = normal_blend (preview_width, preview_height,
                                   backdrop, 1.0, 0, 0,
                                   source, proxy_ratio,
                                   layer_offx + cache->composition[i].offset_x,
                                   layer_offy + cache->composition[i].offset_y);
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

static void
animation_cel_animation_cleanup (AnimationCelAnimation *animation)
{
  g_list_free_full (animation->priv->cache,
                    (GDestroyNotify) animation_cel_animation_clean_cache);
  animation->priv->cache    = NULL;
  g_list_free_full (animation->priv->comments,
                    (GDestroyNotify) g_free);
  animation->priv->comments = NULL;
  g_list_free_full (animation->priv->tracks,
                    (GDestroyNotify) animation_cel_animation_clean_track);
  animation->priv->tracks   = NULL;

  g_object_unref (animation->priv->camera);
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
          if (cache1->composition[i].tattoo   != cache2->composition[i].tattoo   ||
              cache1->composition[i].offset_x != cache2->composition[i].offset_x ||
              cache1->composition[i].offset_y != cache2->composition[i].offset_y)
            {
              identical = FALSE;
              break;
            }
        }
    }
  return identical;
}

static void
animation_cel_animation_clean_cache (Cache *cache)
{
  if (cache != NULL && --(cache->refs) == 0)
    {
      g_object_unref (cache->buffer);
      g_free (cache->composition);
      g_free (cache);
    }
}

static void
animation_cel_animation_clean_track (Track *track)
{
  g_free (track->title);
  g_list_free_full (track->frames, (GDestroyNotify) g_list_free);
  g_free (track);
}
