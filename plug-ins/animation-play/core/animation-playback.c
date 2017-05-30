/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * animation-playback.c
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

#include <string.h>

#include <libgimp/gimp.h>
#include "libgimp/stdplugins-intl.h"

#include "animation.h"
#include "animation-playback.h"
#include "animation-renderer.h"

enum
{
  START,
  STOP,
  RANGE,
  RENDER,
  LOW_FRAMERATE,
  PROXY_CHANGED,
  LAST_SIGNAL
};

enum
{
  PROP_0,
  PROP_ANIMATION
};

struct _AnimationPlaybackPrivate
{
  Animation   *animation;
  GObject     *renderer;

  /* State of the currently loaded playback. */
  gint         position;
  /* Playback can be a subset of frames. */
  gint         start;
  gint         stop;
  gboolean     stop_at_end;

  gdouble      proxy_ratio;

  guint        timer;
  gint64       start_time;
  gint64       frames_played;
};

typedef struct
{
  AnimationPlayback *playback;

  gint               level;
} ParseStatus;

static void       animation_playback_finalize               (GObject      *object);
static void       animation_playback_set_property           (GObject           *object,
                                                             guint              property_id,
                                                             const GValue      *value,
                                                             GParamSpec        *pspec);
static void       animation_playback_get_property           (GObject           *object,
                                                             guint              property_id,
                                                             GValue            *value,
                                                             GParamSpec        *pspec);

static void       on_duration_changed                       (Animation         *animation,
                                                             gint               duration,
                                                             AnimationPlayback *playback);
static void       on_cache_updated                          (AnimationRenderer *renderer,
                                                             gint               position,
                                                             AnimationPlayback *playback);

/* Timer callback for playback. */
static gboolean   animation_playback_advance_frame_callback (AnimationPlayback *playback);

/* XML parsing */
static void       animation_playback_start_element          (GMarkupParseContext  *context,
                                                             const gchar          *element_name,
                                                             const gchar         **attribute_names,
                                                             const gchar         **attribute_values,
                                                             gpointer              user_data,
                                                             GError              **error);
static void       animation_playback_end_element            (GMarkupParseContext  *context,
                                                             const gchar          *element_name,
                                                             gpointer              user_data,
                                                             GError              **error);


G_DEFINE_TYPE (AnimationPlayback, animation_playback, G_TYPE_OBJECT)

#define parent_class animation_playback_parent_class

static guint animation_playback_signals[LAST_SIGNAL] = { 0 };

static void
animation_playback_class_init (AnimationPlaybackClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  /**
   * AnimationPlayback::start:
   * @playback: the #AnimationPlayback.
   *
   * The @playback starts to play.
   */
  animation_playback_signals[START] =
    g_signal_new ("start",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (AnimationPlaybackClass, start),
                  NULL, NULL,
                  g_cclosure_marshal_VOID__VOID,
                  G_TYPE_NONE,
                  0);
  /**
   * AnimationPlayback::stop:
   * @playback: the #AnimationPlayback.
   *
   * The @playback stops playing.
   */
  animation_playback_signals[STOP] =
    g_signal_new ("stop",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (AnimationPlaybackClass, stop),
                  NULL, NULL,
                  g_cclosure_marshal_VOID__VOID,
                  G_TYPE_NONE,
                  0);
  /**
   * AnimationPlayback::range:
   * @playback: the #AnimationPlayback.
   * @start: the playback start frame.
   * @stop: the playback last frame.
   *
   * The ::range signal is emitted when the playback range is
   * updated.
   */
  animation_playback_signals[RANGE] =
    g_signal_new ("range",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (AnimationPlaybackClass, range),
                  NULL, NULL,
                  NULL,
                  G_TYPE_NONE,
                  2,
                  G_TYPE_INT,
                  G_TYPE_INT);
  /**
   * AnimationPlayback::render:
   * @playback: the #AnimationPlayback.
   * @position: current position to be rendered.
   * @buffer: the #GeglBuffer for the frame at @position.
   * @must_draw_null: meaning of a %NULL @buffer.
   * %TRUE means we have to draw an empty frame.
   * %FALSE means the new frame is same as the current frame.
   *
   * Sends a request for render to the GUI.
   */
  animation_playback_signals[RENDER] =
    g_signal_new ("render",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (AnimationPlaybackClass, render),
                  NULL, NULL,
                  NULL,
                  G_TYPE_NONE,
                  3,
                  G_TYPE_INT,
                  GEGL_TYPE_BUFFER,
                  G_TYPE_BOOLEAN);
  /**
   * AnimationPlayback::low-framerate:
   * @playback: the #AnimationPlayback.
   * @actual_fps: the current playback framerate in fps.
   *
   * The ::low-framerate signal is emitted when the playback framerate
   * is lower than expected. It is also emitted once when the framerate
   * comes back to acceptable rate.
   */
  animation_playback_signals[LOW_FRAMERATE] =
    g_signal_new ("low-framerate",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (AnimationPlaybackClass, low_framerate),
                  NULL, NULL,
                  g_cclosure_marshal_VOID__DOUBLE,
                  G_TYPE_NONE,
                  1,
                  G_TYPE_DOUBLE);

  /**
   * AnimationPlayback::proxy:
   * @playback: the #AnimationPlayback.
   * @ratio: the current proxy ratio [0-1.0].
   *
   * The ::proxy signal is emitted to announce a change of proxy size.
   */
  animation_playback_signals[PROXY_CHANGED] =
    g_signal_new ("proxy-changed",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (AnimationPlaybackClass, proxy_changed),
                  NULL, NULL,
                  g_cclosure_marshal_VOID__DOUBLE,
                  G_TYPE_NONE,
                  1,
                  G_TYPE_DOUBLE);

  object_class->finalize     = animation_playback_finalize;
  object_class->set_property = animation_playback_set_property;
  object_class->get_property = animation_playback_get_property;

  /**
   * AnimationPlayback:animation:
   *
   * The associated #Animation.
   */
  g_object_class_install_property (object_class, PROP_ANIMATION,
                                   g_param_spec_object ("animation",
                                                        NULL, NULL,
                                                        ANIMATION_TYPE_ANIMATION,
                                                        G_PARAM_READWRITE));

  g_type_class_add_private (klass, sizeof (AnimationPlaybackPrivate));
}

static void
animation_playback_init (AnimationPlayback *playback)
{
  playback->priv = G_TYPE_INSTANCE_GET_PRIVATE (playback,
                                                ANIMATION_TYPE_PLAYBACK,
                                                AnimationPlaybackPrivate);
  playback->priv->proxy_ratio = 1.0;
}

/************ Public Functions ****************/

AnimationPlayback *
animation_playback_new (void)
{
  AnimationPlayback *playback;

  playback = g_object_new (ANIMATION_TYPE_PLAYBACK,
                           NULL);

  return playback;
}

gchar *
animation_playback_serialize (AnimationPlayback *playback)
{
  gchar *xml;
  gchar  proxy[6];

  /* Make sure to have a locale-independent string. Also no need to have
   * useless precision. This should give 3 digits after the decimal point.
   * More than enough.
   */
  g_ascii_dtostr ((gchar*) &proxy, 6, playback->priv->proxy_ratio);
  xml = g_strdup_printf ("<playback position=\"%d\" "
                         "start=\"%d\" stop=\"%d\" proxy=\"%s\"/>",
                         playback->priv->position,
                         playback->priv->start,
                         playback->priv->stop,
                         proxy);
  return xml;
}

void
animation_playback_set_animation (AnimationPlayback *playback,
                                  Animation         *animation,
                                  const gchar       *xml)
{
  g_object_set (playback,
                "animation", animation,
                NULL);

  if (xml)
    {
      /* Reset to last known playback status. */
      const GMarkupParser  markup_parser =
        {
          animation_playback_start_element,
          animation_playback_end_element,
          NULL,  /*  text         */
          NULL,  /*  passthrough  */
          NULL   /*  error        */
        };
      GMarkupParseContext *context;
      ParseStatus          status = { 0, };
      GError              *error = NULL;

      status.playback = playback;
      status.level    = 0;
      context = g_markup_parse_context_new (&markup_parser,
                                            0, &status, NULL);
      g_markup_parse_context_parse (context, xml, strlen (xml), &error);
      if (error == NULL)
        g_markup_parse_context_end_parse (context, &error);
      g_markup_parse_context_free (context);
      if (error)
        g_warning ("Error parsing XML: %s", error->message);
      g_clear_error (&error);
    }
}

Animation *
animation_playback_get_animation (AnimationPlayback *playback)
{
  return playback->priv->animation;
}

gint
animation_playback_get_position (AnimationPlayback *playback)
{
  return playback->priv->position;
}

GeglBuffer *
animation_playback_get_buffer (AnimationPlayback *playback,
                               gint               position)
{
  AnimationRenderer *renderer;

  renderer = ANIMATION_RENDERER (playback->priv->renderer);
  return animation_renderer_get_buffer (renderer, position);
}

gboolean
animation_playback_is_playing (AnimationPlayback *playback)
{
  return (playback->priv->timer != 0);
}

void
animation_playback_play (AnimationPlayback *playback)
{
  gint duration;

  duration = (gint) (1000.0 /
                     animation_get_framerate (playback->priv->animation));

  if (playback->priv->timer)
    {
      /* It means we are already playing, so we should not need to play
       * again.
       * Still be liberal and simply remove the timer before creating a
       * new one. */
      g_source_remove (playback->priv->timer);
      g_signal_emit (playback, animation_playback_signals[STOP], 0);
    }

  playback->priv->start_time = g_get_monotonic_time ();
  playback->priv->frames_played = 1;

  playback->priv->timer = g_timeout_add ((guint) duration,
                                         (GSourceFunc) animation_playback_advance_frame_callback,
                                         playback);
  g_signal_emit (playback, animation_playback_signals[START], 0);
}

void
animation_playback_stop (AnimationPlayback *playback)
{
  if (playback->priv->timer)
    {
      /* Stop playing by removing any playback timer. */
      g_source_remove (playback->priv->timer);
      playback->priv->timer = 0;
      g_signal_emit (playback, animation_playback_signals[STOP], 0);
    }
}

void
animation_playback_next (AnimationPlayback *playback)
{
  AnimationRenderer *renderer;
  GeglBuffer        *buffer       = NULL;
  gint               previous_pos = playback->priv->position;
  gboolean           identical;

  if (! playback->priv->animation)
    return;

  playback->priv->position = playback->priv->start +
                             ((playback->priv->position - playback->priv->start + 1) %
                              (playback->priv->stop - playback->priv->start + 1));

  renderer = ANIMATION_RENDERER (playback->priv->renderer);
  identical = animation_renderer_identical (renderer,
                                            previous_pos,
                                            playback->priv->position);
  if (! identical)
    {
      buffer = animation_renderer_get_buffer (renderer,
                                              playback->priv->position);
    }
  g_signal_emit (playback, animation_playback_signals[RENDER], 0,
                 playback->priv->position, buffer, ! identical);
  if (buffer != NULL)
    {
      g_object_unref (buffer);
    }
}

void
animation_playback_prev (AnimationPlayback *playback)
{
  AnimationRenderer *renderer;
  GeglBuffer        *buffer    = NULL;
  gint               prev_pos  = playback->priv->position;
  gboolean           identical;

  if (! playback->priv->animation)
    return;

  if (playback->priv->position == playback->priv->start)
    {
      playback->priv->position = animation_playback_get_stop (playback);
    }
  else
    {
      --playback->priv->position;
    }

  renderer = ANIMATION_RENDERER (playback->priv->renderer);
  identical = animation_renderer_identical (renderer,
                                            prev_pos,
                                            playback->priv->position);
  if (! identical)
    {
      buffer = animation_renderer_get_buffer (renderer, playback->priv->position);
    }
  g_signal_emit (playback, animation_playback_signals[RENDER], 0,
                 playback->priv->position, buffer, ! identical);
  if (buffer)
    g_object_unref (buffer);
}

void
animation_playback_jump (AnimationPlayback *playback,
                         gint               index)
{
  AnimationRenderer *renderer;
  GeglBuffer        *buffer    = NULL;
  gint               prev_pos  = playback->priv->position;
  gboolean           identical;

  if (! playback->priv->animation)
    return;

  if (index < playback->priv->start ||
      index > playback->priv->stop)
    return;
  else
    playback->priv->position = index;

  renderer = ANIMATION_RENDERER (playback->priv->renderer);
  identical = animation_renderer_identical (renderer,
                                            prev_pos,
                                            playback->priv->position);
  if (! identical)
    {
      buffer = animation_renderer_get_buffer (renderer, playback->priv->position);
    }
  g_signal_emit (playback, animation_playback_signals[RENDER], 0,
                 playback->priv->position, buffer, ! identical);
  if (buffer)
    g_object_unref (buffer);
}

void
animation_playback_set_start (AnimationPlayback *playback,
                              gint               index)
{
  gint duration;

  if (! playback->priv->animation)
    return;

  duration = animation_get_duration (playback->priv->animation);

  if (index < 0 ||
      index >= duration)
    {
      playback->priv->start = 0;
    }
  else
    {
      playback->priv->start = index;
    }
  if (playback->priv->stop < playback->priv->start)
    {
      playback->priv->stop = duration - 1;
      playback->priv->stop_at_end = TRUE;
    }

  g_signal_emit (playback, animation_playback_signals[RANGE], 0,
                 playback->priv->start, playback->priv->stop);

  if (playback->priv->position < playback->priv->start ||
      playback->priv->position > playback->priv->stop)
    {
      animation_playback_jump (playback, playback->priv->start);
    }
}

gint
animation_playback_get_start (AnimationPlayback *playback)
{
  return playback->priv->start;
}

void
animation_playback_set_stop (AnimationPlayback *playback,
                             gint               index)
{
  gint duration;

  if (! playback->priv->animation)
    return;

  duration = animation_get_duration (playback->priv->animation);

  if (index < 0 ||
      index >= duration)
    {
      playback->priv->stop = duration - 1;
      playback->priv->stop_at_end = TRUE;
    }
  else
    {
      playback->priv->stop = index;

      if (index == duration - 1)
        playback->priv->stop_at_end = TRUE;
    }
  if (playback->priv->stop < playback->priv->start)
    {
      playback->priv->start = 0;
    }

  g_signal_emit (playback, animation_playback_signals[RANGE], 0,
                 playback->priv->start, playback->priv->stop);

  if (playback->priv->position < playback->priv->start ||
      playback->priv->position > playback->priv->stop)
    {
      animation_playback_jump (playback, playback->priv->start);
    }
}

gint
animation_playback_get_stop (AnimationPlayback *playback)
{
  return playback->priv->stop;
}

void
animation_playback_get_size (AnimationPlayback   *playback,
                             gint                *width,
                             gint                *height)
{
  animation_get_size (playback->priv->animation,
                      width, height);

  /* Apply proxy ratio. */
  *width  *= playback->priv->proxy_ratio;
  *height *= playback->priv->proxy_ratio;
}

void
animation_playback_set_proxy (AnimationPlayback *playback,
                              gdouble            ratio)
{
  g_return_if_fail (ratio > 0.0 && ratio <= 1.0);

  if (playback->priv->proxy_ratio != ratio)
    {
      playback->priv->proxy_ratio = ratio;
      g_signal_emit (playback,
                     animation_playback_signals[PROXY_CHANGED],
                     0, ratio);
    }
}

gdouble
animation_playback_get_proxy (AnimationPlayback *playback)
{
  return playback->priv->proxy_ratio;
}

/************ Private Functions ****************/

static void
animation_playback_finalize (GObject      *object)
{
  AnimationPlayback *playback = ANIMATION_PLAYBACK (object);

  g_object_unref (playback->priv->renderer);
  if (playback->priv->animation)
    g_object_unref (playback->priv->animation);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
animation_playback_set_property (GObject      *object,
                                 guint         property_id,
                                 const GValue *value,
                                 GParamSpec   *pspec)
{
  AnimationPlayback *playback = ANIMATION_PLAYBACK (object);

  switch (property_id)
    {
    case PROP_ANIMATION:
        {
          Animation *animation;

          if (playback->priv->renderer)
            g_object_unref (playback->priv->renderer);
          if (playback->priv->animation)
            g_object_unref (playback->priv->animation);

          animation = g_value_dup_object (value);
          playback->priv->animation = animation;
          playback->priv->renderer = NULL;

          if (! animation)
            break;

          /* Default playback is the full range of frames. */
          playback->priv->position = 0;
          playback->priv->start = 0;
          playback->priv->stop  = animation_get_duration (animation) - 1;
          playback->priv->stop_at_end = TRUE;

          g_signal_connect (animation, "duration-changed",
                            G_CALLBACK (on_duration_changed), playback);

          playback->priv->renderer = animation_renderer_new (object);
          g_signal_connect (playback->priv->renderer, "cache-updated",
                            G_CALLBACK (on_cache_updated), playback);
        }
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
animation_playback_get_property (GObject    *object,
                                 guint       property_id,
                                 GValue     *value,
                                 GParamSpec *pspec)
{
  AnimationPlayback *playback = ANIMATION_PLAYBACK (object);

  switch (property_id)
    {
    case PROP_ANIMATION:
      g_value_set_object (value, playback->priv->animation);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
on_duration_changed (Animation         *animation,
                     gint               duration,
                     AnimationPlayback *playback)
{
  if (! playback->priv->animation)
    return;

  if (playback->priv->stop >= duration ||
      playback->priv->stop_at_end)
    {
      playback->priv->stop = duration - 1;
      playback->priv->stop_at_end = TRUE;
    }
  if (playback->priv->start >= duration ||
      playback->priv->start > playback->priv->stop)
    {
      playback->priv->start = 0;
    }

  if (playback->priv->position < playback->priv->start ||
      playback->priv->position > playback->priv->stop)
    {
      playback->priv->position = playback->priv->start;
    }
  g_signal_emit (playback, animation_playback_signals[RANGE], 0,
                 playback->priv->start, playback->priv->stop);
}

static void
on_cache_updated (AnimationRenderer *renderer,
                  gint               position,
                  AnimationPlayback *playback)
{
  if (animation_playback_get_position (playback) == position)
    {
      AnimationRenderer *renderer;
      GeglBuffer        *buffer;

      renderer = ANIMATION_RENDERER (playback->priv->renderer);
      buffer = animation_renderer_get_buffer (renderer, position);
      g_signal_emit (playback, animation_playback_signals[RENDER], 0,
                     position, buffer, TRUE);
      if (buffer)
        {
          g_object_unref (buffer);
        }
    }
}

static gboolean
animation_playback_advance_frame_callback (AnimationPlayback *playback)
{
  gdouble         framerate;
  gint64          duration;
  gint64          duration_since_start;
  static gboolean prev_low_framerate = FALSE;

  framerate = animation_get_framerate (playback->priv->animation);

  animation_playback_next (playback);
  duration = (gint) (1000.0 / framerate);

  duration_since_start = (g_get_monotonic_time () - playback->priv->start_time) / 1000;
  duration = duration - (duration_since_start - playback->priv->frames_played * duration);

  if (duration < 1)
    {
      if (prev_low_framerate)
        {
          /* Let's only warn the user for several subsequent slow frames. */
          gdouble real_framerate;

          real_framerate = (gdouble) playback->priv->frames_played * 1000.0 / duration_since_start;

          if (real_framerate < framerate)
            g_signal_emit (playback, animation_playback_signals[LOW_FRAMERATE], 0,
                           real_framerate);
        }
      duration = 1;
      prev_low_framerate = TRUE;
    }
  else
    {
      if (prev_low_framerate)
        {
          /* Let's reset framerate warning. */
          g_signal_emit (playback, animation_playback_signals[LOW_FRAMERATE], 0,
                         framerate);
        }
      prev_low_framerate = FALSE;
    }
  playback->priv->frames_played++;

  playback->priv->timer = g_timeout_add ((guint) duration,
                                        (GSourceFunc) animation_playback_advance_frame_callback,
                                        (AnimationPlayback *) playback);

  return G_SOURCE_REMOVE;
}

static void
animation_playback_start_element (GMarkupParseContext  *context,
                                  const gchar          *element_name,
                                  const gchar         **attribute_names,
                                  const gchar         **attribute_values,
                                  gpointer              user_data,
                                  GError              **error)
{
  const gchar       **names    = attribute_names;
  const gchar       **values   = attribute_values;
  ParseStatus        *status   = (ParseStatus *) user_data;
  AnimationPlayback  *playback = status->playback;

  if (status->level == 1 && g_strcmp0 (element_name, "playback") == 0)
    {
      gint duration;

      duration = animation_get_duration (playback->priv->animation);
      while (*names && *values)
        {
          if (strcmp (*names, "position") == 0 && **values)
            {
              gint position = g_ascii_strtoll (*values, NULL, 10);

              if (position >= duration)
                {
                  g_set_error (error, 0, 0,
                               _("Playback position %d out of range [0, %d]."),
                               position, duration - 1);
                }
              else
                {
                  playback->priv->position = position;
                }
            }
          else if (strcmp (*names, "start") == 0 && **values)
            {
              gint start = g_ascii_strtoll (*values, NULL, 10);

              if (start >= duration)
                {
                  g_set_error (error, 0, 0,
                               _("Playback start %d out of range [0, %d]."),
                               start, duration - 1);
                }
              else
                {
                  playback->priv->start = start;
                }
            }
          else if (strcmp (*names, "stop") == 0 && **values)
            {
              gint stop = g_ascii_strtoll (*values, NULL, 10);

              if (stop >= duration)
                {
                  g_set_error (error, 0, 0,
                               _("Playback stop %d out of range [0, %d]."),
                               stop, duration - 1);
                }
              else
                {
                  playback->priv->stop = stop;
                  playback->priv->stop_at_end = (stop == duration - 1);
                }
            }
          else if (strcmp (*names, "proxy") == 0 && **values)
            {
              gdouble ratio = g_ascii_strtod (*values, NULL);
              animation_playback_set_proxy (playback, ratio);
            }

          names++;
          values++;
        }
      if (playback->priv->stop < playback->priv->start)
        {
          playback->priv->stop = duration - 1;
          playback->priv->stop_at_end = TRUE;
        }

      if (playback->priv->position < playback->priv->start ||
          playback->priv->position > playback->priv->stop)
        {
          playback->priv->position = playback->priv->start;
        }
    }
  status->level++;
}

static void
animation_playback_end_element (GMarkupParseContext  *context,
                                const gchar          *element_name,
                                gpointer              user_data,
                                GError              **error)
{
  ((ParseStatus *) user_data)->level--;
}
