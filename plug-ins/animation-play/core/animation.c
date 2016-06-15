/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * animation.c
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

#include "config.h"

#include <string.h>

#include <libgimp/gimp.h>
#include <libgimp/stdplugins-intl.h>

#include "animation-utils.h"

#include "animation.h"
#include "animationanimatic.h"

enum
{
  LOADING_START,
  LOADING,
  LOADED,
  START,
  STOP,
  FRAMERATE_CHANGED,
  PLAYBACK_RANGE,
  RENDER,
  LOW_FRAMERATE,
  PROXY,
  LAST_SIGNAL
};

enum
{
  PROP_0,
  PROP_IMAGE
};

typedef struct _AnimationPrivate AnimationPrivate;

struct _AnimationPrivate
{
  gint32       image_id;
  gchar       *xml;

  gdouble      framerate;

  guint        playback_timer;

  /* State of the currently loaded animation. */
  gint         position;
  /* We want to allow animator to set any number as first frame,
   * for frame export capability. */
  guint        start_pos;
  /* Playback can be a subset of frames. */
  guint        playback_start;
  guint        playback_stop;

  /* Proxy settings generates a reload. */
  gdouble      proxy_ratio;

  gboolean     loaded;

  gint64       playback_start_time;
  gint64       frames_played;
};


#define ANIMATION_GET_PRIVATE(animation) \
        G_TYPE_INSTANCE_GET_PRIVATE (animation, \
                                     ANIMATION_TYPE_ANIMATION, \
                                     AnimationPrivate)

static void       animation_set_property           (GObject      *object,
                                                    guint         property_id,
                                                    const GValue *value,
                                                    GParamSpec   *pspec);
static void       animation_get_property           (GObject      *object,
                                                    guint         property_id,
                                                    GValue       *value,
                                                    GParamSpec   *pspec);

/* Base implementation of virtual methods. */
static gint       animation_real_get_start_position (Animation *animation);
static gboolean   animation_real_same               (Animation *animation,
                                                     gint       prev_pos,
                                                     gint       next_pos);

/* Timer callback for playback. */
static gboolean   animation_advance_frame_callback  (Animation *animation);

G_DEFINE_TYPE (Animation, animation, G_TYPE_OBJECT)

#define parent_class animation_parent_class

static guint animation_signals[LAST_SIGNAL] = { 0 };

static void
animation_class_init (AnimationClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  /**
   * Animation::loading-start:
   * @animation: the animation loading.
   *
   * The ::loading-start signal is emitted when loading starts.
   * GUI widgets depending on a consistent state of @animation should
   * become unresponsive.
   */
  animation_signals[LOADING_START] =
    g_signal_new ("loading-start",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  0,
                  NULL, NULL,
                  NULL,
                  G_TYPE_NONE,
                  0);
  /**
   * Animation::loading:
   * @animation: the animation loading.
   * @ratio: fraction loaded [0-1].
   *
   * The ::loading signal must be emitted by subclass of Animation.
   * It can be used by a GUI to display a progress bar during loading.
   */
  animation_signals[LOADING] =
    g_signal_new ("loading",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  0,
                  NULL, NULL,
                  NULL,
                  G_TYPE_NONE,
                  1,
                  G_TYPE_DOUBLE);
  /**
   * Animation::loaded:
   * @animation: the animation loading.
   * @start_pos: the first frame.
   * @length: number of frames.
   * @playback_start: the playback start frame.
   * @playback_stop: the playback last frame.
   * @width: display width in pixels.
   * @height: display height in pixels.
   *
   * The ::loaded signal is emitted when @animation is fully loaded.
   * GUI widgets depending on a consistent state of @animation can
   * now become responsive to user interaction.
   */
  animation_signals[LOADED] =
    g_signal_new ("loaded",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  0,
                  NULL, NULL,
                  NULL,
                  G_TYPE_NONE,
                  6,
                  G_TYPE_INT,
                  G_TYPE_INT,
                  G_TYPE_INT,
                  G_TYPE_INT,
                  G_TYPE_INT,
                  G_TYPE_INT);
  /**
   * Animation::render:
   * @animation: the animation.
   * @position: current position to be rendered.
   * @buffer: the #GeglBuffer for the frame at @position.
   * @must_draw_null: meaning of a %NULL @buffer.
   * %TRUE means we have to draw an empty frame.
   * %FALSE means the new frame is same as the current frame.
   *
   * Sends a request for render to the GUI.
   */
  animation_signals[RENDER] =
    g_signal_new ("render",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  0,
                  NULL, NULL,
                  NULL,
                  G_TYPE_NONE,
                  3,
                  G_TYPE_INT,
                  GEGL_TYPE_BUFFER,
                  G_TYPE_BOOLEAN);
  /**
   * Animation::start:
   * @animation: the animation.
   *
   * The @animation starts to play.
   */
  animation_signals[START] =
    g_signal_new ("start",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  0,
                  NULL, NULL,
                  NULL,
                  G_TYPE_NONE,
                  0);
  /**
   * Animation::stops:
   * @animation: the animation.
   *
   * The @animation stops playing.
   */
  animation_signals[STOP] =
    g_signal_new ("stop",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  0,
                  NULL, NULL,
                  NULL,
                  G_TYPE_NONE,
                  0);
  /**
   * Animation::framerate-changed:
   * @animation: the animation.
   * @framerate: the new framerate in frames per second.
   *
   * The ::framerate-changed signal is emitted when framerate has
   * changed.
   */
  animation_signals[FRAMERATE_CHANGED] =
    g_signal_new ("framerate-changed",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  0,
                  NULL, NULL,
                  NULL,
                  G_TYPE_NONE,
                  1,
                  G_TYPE_DOUBLE);
  /**
   * Animation::playback-range:
   * @animation: the animation.
   * @playback_start: the playback start frame.
   * @playback_stop: the playback last frame.
   * @first_frame: the first frame.
   * @length: the full length.
   *
   * The ::playback-range signal is emitted when the playback range is
   * updated.
   */
  animation_signals[PLAYBACK_RANGE] =
    g_signal_new ("playback-range",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  0,
                  NULL, NULL,
                  NULL,
                  G_TYPE_NONE,
                  4,
                  G_TYPE_INT,
                  G_TYPE_INT,
                  G_TYPE_INT,
                  G_TYPE_INT);
  /**
   * Animation::low-framerate:
   * @animation: the animation.
   * @actual_fps: the current playback framerate in fps.
   *
   * The ::low-framerate signal is emitted when the playback framerate
   * is lower than expected. It is also emitted once when the framerate
   * comes back to acceptable rate.
   */
  animation_signals[LOW_FRAMERATE] =
    g_signal_new ("low-framerate-playback",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  0,
                  NULL, NULL,
                  NULL,
                  G_TYPE_NONE,
                  1,
                  G_TYPE_DOUBLE);
  /**
   * Animation::proxy:
   * @animation: the animation.
   * @ratio: the current proxy ratio [0-1.0].
   *
   * The ::proxy signal is emitted to announce a change of proxy size.
   */
  animation_signals[PROXY] =
    g_signal_new ("proxy",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  0,
                  NULL, NULL,
                  NULL,
                  G_TYPE_NONE,
                  1,
                  G_TYPE_DOUBLE);

  object_class->set_property = animation_set_property;
  object_class->get_property = animation_get_property;

  klass->get_start_position  = animation_real_get_start_position;
  klass->same                = animation_real_same;

  /**
   * Animation:image:
   *
   * The attached image id.
   */
  g_object_class_install_property (object_class, PROP_IMAGE,
                                   g_param_spec_int ("image", NULL, NULL,
                                                     0, G_MAXINT, 0,
                                                     G_PARAM_READWRITE |
                                                     G_PARAM_CONSTRUCT_ONLY));

  g_type_class_add_private (klass, sizeof (AnimationPrivate));
}

static void
animation_init (Animation *animation)
{
  AnimationPrivate *priv = ANIMATION_GET_PRIVATE (animation);

  /* Acceptable settings for the default. */
  priv->framerate   = 24.0; /* fps */
  priv->proxy_ratio = 1.0;
}

/************ Public Functions ****************/

Animation *
animation_new (gint32       image_id,
               const gchar *xml)
{
  Animation        *animation;
  AnimationPrivate *priv;

  animation = g_object_new (ANIMATION_TYPE_ANIMATIC,
                            "image", image_id,
                            NULL);
  priv = ANIMATION_GET_PRIVATE (animation);
  priv->xml = g_strdup (xml);

  return animation;
}

gint32
animation_get_image_id (Animation   *animation)
{
  AnimationPrivate *priv = ANIMATION_GET_PRIVATE (animation);

  return priv->image_id;
}

void
animation_load (Animation *animation,
                gdouble    proxy_ratio)
{
  AnimationPrivate *priv = ANIMATION_GET_PRIVATE (animation);
  GeglBuffer       *buffer;
  gint              width, height;
  gint              length;

  priv->loaded = FALSE;
  g_signal_emit (animation, animation_signals[LOADING_START], 0);

  priv->proxy_ratio = proxy_ratio;
  g_signal_emit (animation, animation_signals[PROXY], 0, proxy_ratio);

  if (priv->xml)
    ANIMATION_GET_CLASS (animation)->load_xml (animation,
                                               priv->xml,
                                               proxy_ratio);
  else
    ANIMATION_GET_CLASS (animation)->load (animation, proxy_ratio);
  /* XML is only used for the first load.
   * Any next loads will use internal data. */
  g_free (priv->xml);
  priv->xml = NULL;

  priv->start_pos = ANIMATION_GET_CLASS (animation)->get_start_position (animation);
  priv->position  = priv->start_pos;
  length    = ANIMATION_GET_CLASS (animation)->get_length (animation);

  /* Default playback is the full range of frames. */
  priv->playback_start = priv->position;
  priv->playback_stop  = priv->position + length - 1;

  priv->loaded = TRUE;

  animation_get_size (animation, &width, &height);
  g_signal_emit (animation, animation_signals[LOADED], 0,
                 1, length,
                 priv->playback_start,
                 priv->playback_stop,
                 width,
                 height);

  /* Once loaded, let's ask render. */
  buffer = animation_get_frame (animation, priv->position);
  g_signal_emit (animation, animation_signals[RENDER], 0,
                 priv->position, buffer, TRUE);

  if (buffer)
    g_object_unref (buffer);
}

gchar *
animation_serialize (Animation   *animation)
{
  return ANIMATION_GET_CLASS (animation)->serialize (animation);
}

gint
animation_get_start_position (Animation *animation)
{
  return ANIMATION_GET_CLASS (animation)->get_start_position (animation);
}

gint
animation_get_position (Animation *animation)
{
  AnimationPrivate *priv = ANIMATION_GET_PRIVATE (animation);

  return priv->position;
}

gint
animation_get_length (Animation *animation)
{
  return ANIMATION_GET_CLASS (animation)->get_length (animation);
}

void
animation_get_size (Animation *animation,
                    gint      *width,
                    gint      *height)
{
  ANIMATION_GET_CLASS (animation)->get_size (animation, width, height);
}

gdouble
animation_get_proxy (Animation   *animation)
{
  AnimationPrivate *priv = ANIMATION_GET_PRIVATE (animation);

  return priv->proxy_ratio;
}

GeglBuffer *
animation_get_frame (Animation *animation,
                     gint       pos)
{
  return ANIMATION_GET_CLASS (animation)->get_frame (animation, pos);
}

gboolean
animation_is_playing (Animation *animation)
{
  AnimationPrivate *priv = ANIMATION_GET_PRIVATE (animation);

  return (priv->playback_timer != 0);
}

void
animation_play (Animation *animation)
{
  AnimationPrivate *priv = ANIMATION_GET_PRIVATE (animation);
  gint              duration;

  duration = (gint) (1000.0 / animation_get_framerate (animation));

  if (priv->playback_timer)
    {
      /* It means we are already playing, so we should not need to play
       * again.
       * Still be liberal and simply remove the timer before creating a
       * new one. */
      g_source_remove (priv->playback_timer);
    }

  priv->playback_start_time = g_get_monotonic_time ();
  priv->frames_played = 1;

  priv->playback_timer = g_timeout_add ((guint) duration,
                                        (GSourceFunc) animation_advance_frame_callback,
                                        animation);
  g_signal_emit (animation, animation_signals[START], 0);
}

void
animation_stop (Animation   *animation)
{
  AnimationPrivate *priv = ANIMATION_GET_PRIVATE (animation);

  if (priv->playback_timer)
    {
      /* Stop playing by removing any playback timer. */
      g_source_remove (priv->playback_timer);
      priv->playback_timer = 0;
      g_signal_emit (animation, animation_signals[STOP], 0);
    }
}

void
animation_next (Animation *animation)
{
  AnimationPrivate *priv         = ANIMATION_GET_PRIVATE (animation);
  GeglBuffer       *buffer       = NULL;
  gint              previous_pos = priv->position;
  gboolean          identical;

  priv->position = animation_get_playback_start (animation) +
                   ((priv->position - animation_get_playback_start (animation) + 1) %
                    (animation_get_playback_stop (animation) - animation_get_playback_start (animation) + 1));

  identical = ANIMATION_GET_CLASS (animation)->same (animation,
                                                     previous_pos,
                                                     priv->position);
  if (! identical)
    {
      buffer = animation_get_frame (animation, priv->position);
    }
  g_signal_emit (animation, animation_signals[RENDER], 0,
                 priv->position, buffer, ! identical);
  if (buffer != NULL)
    {
      g_object_unref (buffer);
    }
}

void
animation_prev (Animation *animation)
{
  AnimationPrivate *priv     = ANIMATION_GET_PRIVATE (animation);
  GeglBuffer       *buffer   = NULL;
  gint              prev_pos = priv->position;
  gboolean          identical;

  if (priv->position == animation_get_playback_start (animation))
    {
      priv->position = animation_get_playback_stop (animation);
    }
  else
    {
      --priv->position;
    }

  identical = ANIMATION_GET_CLASS (animation)->same (animation,
                                                     prev_pos,
                                                     priv->position);
  if (! identical)
    {
      buffer = animation_get_frame (animation, priv->position);
    }
  g_signal_emit (animation, animation_signals[RENDER], 0,
                 priv->position, buffer, ! identical);
  if (buffer)
    g_object_unref (buffer);
}

void
animation_jump (Animation *animation,
                gint       index)
{
  AnimationPrivate *priv           = ANIMATION_GET_PRIVATE (animation);
  GeglBuffer       *buffer         = NULL;
  gint              prev_pos = priv->position;
  gboolean          identical;

  if (index < priv->playback_start ||
      index > priv->playback_stop)
    return;
  else
    priv->position = index;

  identical = ANIMATION_GET_CLASS (animation)->same (animation,
                                                     prev_pos,
                                                     priv->position);
  if (! identical)
    {
      buffer = animation_get_frame (animation, priv->position);
    }
  g_signal_emit (animation, animation_signals[RENDER], 0,
                 priv->position, buffer, ! identical);
  if (buffer)
    g_object_unref (buffer);
}

void
animation_set_playback_start (Animation *animation,
                              gint       frame_number)
{
  AnimationPrivate *priv = ANIMATION_GET_PRIVATE (animation);
  gint              length;

  length = animation_get_length (animation);

  if (frame_number < priv->start_pos ||
      frame_number >= priv->start_pos + length)
    {
      priv->playback_start = priv->start_pos;
    }
  else
    {
      priv->playback_start = frame_number;
    }
  if (priv->playback_stop < priv->playback_start)
    {
      priv->playback_stop = priv->start_pos + length - 1;
    }

  g_signal_emit (animation, animation_signals[PLAYBACK_RANGE], 0,
                 priv->playback_start, priv->playback_stop,
                 priv->start_pos, length);

  if (priv->position < priv->playback_start ||
      priv->position > priv->playback_stop)
    {
      animation_jump (animation, priv->playback_start);
    }
}

gint
animation_get_playback_start (Animation *animation)
{
  AnimationPrivate *priv = ANIMATION_GET_PRIVATE (animation);

  return priv->playback_start;
}

void
animation_set_playback_stop (Animation *animation,
                             gint       frame_number)
{
  AnimationPrivate *priv = ANIMATION_GET_PRIVATE (animation);
  gint              length;

  length = animation_get_length (animation);

  if (frame_number < priv->start_pos ||
      frame_number >= priv->start_pos + length)
    {
      priv->playback_stop = priv->start_pos + length - 1;
    }
  else
    {
      priv->playback_stop = frame_number;
    }
  if (priv->playback_stop < priv->playback_start)
    {
      priv->playback_start = priv->start_pos;
    }
  g_signal_emit (animation, animation_signals[PLAYBACK_RANGE], 0,
                 priv->playback_start, priv->playback_stop,
                 priv->start_pos, length);

  if (priv->position < priv->playback_start ||
      priv->position > priv->playback_stop)
    {
      animation_jump (animation, priv->playback_start);
    }
}

gint
animation_get_playback_stop (Animation *animation)
{
  AnimationPrivate *priv = ANIMATION_GET_PRIVATE (animation);

  return priv->playback_stop;
}

void
animation_set_framerate (Animation *animation,
                         gdouble    fps)
{
  AnimationPrivate *priv = ANIMATION_GET_PRIVATE (animation);

  g_return_if_fail (fps > 0.0);

  priv->framerate = fps;

  g_signal_emit (animation, animation_signals[FRAMERATE_CHANGED], 0,
                 fps);
}

gdouble
animation_get_framerate (Animation *animation)
{
  AnimationPrivate *priv = ANIMATION_GET_PRIVATE (animation);

  return priv->framerate;
}

gboolean
animation_loaded (Animation *animation)
{
  AnimationPrivate *priv = ANIMATION_GET_PRIVATE (animation);

  return priv->loaded;
}

/************ Private Functions ****************/

static void
animation_set_property (GObject      *object,
                        guint         property_id,
                        const GValue *value,
                        GParamSpec   *pspec)
{
  Animation        *animation = ANIMATION (object);
  AnimationPrivate *priv = ANIMATION_GET_PRIVATE (animation);

  switch (property_id)
    {
    case PROP_IMAGE:
      priv->image_id = g_value_get_int (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
animation_get_property (GObject    *object,
                        guint       property_id,
                        GValue     *value,
                        GParamSpec *pspec)
{
  Animation        *animation = ANIMATION (object);
  AnimationPrivate *priv = ANIMATION_GET_PRIVATE (animation);

  switch (property_id)
    {
    case PROP_IMAGE:
      g_value_set_int (value, priv->image_id);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static
gint animation_real_get_start_position (Animation *animation)
{
  /* By default, the first frame is numbered 1. */
  return 1;
}

static gboolean
animation_real_same (Animation *animation,
                     gint       previous_pos,
                     gint       next_pos)
{
  /* By default all frames are supposed different. */
  return (previous_pos == next_pos);
}

static gboolean
animation_advance_frame_callback (Animation *animation)
{
  AnimationPrivate *priv = ANIMATION_GET_PRIVATE (animation);
  gint64            duration;
  gint64            duration_since_start;
  static gboolean   prev_low_framerate = FALSE;

  animation_next (animation);
  duration = (gint) (1000.0 / animation_get_framerate (animation));

  duration_since_start = (g_get_monotonic_time () - priv->playback_start_time) / 1000;
  duration = duration - (duration_since_start - priv->frames_played * duration);

  if (duration < 1)
    {
      if (prev_low_framerate)
        {
          /* Let's only warn the user for several subsequent slow frames. */
          gdouble real_framerate = (gdouble) priv->frames_played * 1000.0 / duration_since_start;
          if (real_framerate < priv->framerate)
            g_signal_emit (animation, animation_signals[LOW_FRAMERATE], 0,
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
          g_signal_emit (animation, animation_signals[LOW_FRAMERATE], 0,
                         animation_get_framerate (animation));
        }
      prev_low_framerate = FALSE;
    }
  priv->frames_played++;

  priv->playback_timer = g_timeout_add ((guint) duration,
                                        (GSourceFunc) animation_advance_frame_callback,
                                        (Animation *) animation);

  return G_SOURCE_REMOVE;
}
