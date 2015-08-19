/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * animation.c
 * Copyright (C) 2015 Jehan <jehan@gimp.org>
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

#include "animation.h"

enum
{
  LOADING_START,
  LOADING,
  LOADED,
  DISPOSAL_CHANGED,
  PROXY_CHANGED,
  FRAMERATE_CHANGED,
  PLAYBACK_RANGE,
  RENDER,
  LOW_FRAMERATE,
  LAST_SIGNAL
};

enum
{
  PROP_0,
  PROP_IMAGE
};

typedef struct
{
  gint32  drawable_id;
  GList  *indexes;
  GList  *updated_indexes;

  guint  duration;
}
Frame;

typedef struct _AnimationPrivate AnimationPrivate;

struct _AnimationPrivate
{
  gint32       image_id;

  Frame      **frames;
  guint        preview_width;
  guint        preview_height;
  gdouble      framerate;
  guint        playback_timer;

  /* State of the currently loaded animation. */
  gint32       num_frames;
  gint         current_frame;
  /* We want to allow animator to set any number as first frame,
   * for frame export capability. */
  guint        first_frame;

  /* Playback can be a subset of frames. */
  guint        playback_start;
  guint        playback_stop;

  /* These settings generate a reload. */
  DisposeType  disposal;
  gdouble      proxy_ratio;

  /* Frames are associated to an unused image (used as backend for GEGL buffers). */
  gboolean     loaded;
  gint32       frames_image_id;
  /* We keep track of the frames in a separate structure to free drawable
   * memory. We can't use easily priv->frames because some of the
   * drawables may be used in more than 1 frame. */
  GList       *previous_frames;
};


#define ANIMATION_GET_PRIVATE(animation) \
        G_TYPE_INSTANCE_GET_PRIVATE (animation, \
                                     ANIMATION_TYPE_ANIMATION, \
                                     AnimationPrivate)

static void         animation_set_property       (GObject      *object,
                                                  guint         property_id,
                                                  const GValue *value,
                                                  GParamSpec   *pspec);
static void         animation_get_property       (GObject      *object,
                                                  guint         property_id,
                                                  GValue       *value,
                                                  GParamSpec   *pspec);
static void         animation_finalize           (GObject      *object);

static void         cleanup_cache                (Animation    *animation);
static void         animation_init_frame_numbers (Animation    *animation,
                                                  gboolean      disposal_changed);
static void         rec_set_total_frames         (Animation    *animation,
                                                  const gint32  layer,
                                                  gint         *min,
                                                  gint         *max);
static void         rec_init_frames              (Animation    *animation,
                                                  gint32        frames_image_id,
                                                  gint          image_width,
                                                  gint          image_height,
                                                  gint32        layer,
                                                  GList        *previous_frames);
static gint         parse_ms_tag                 (const gchar  *str);
static DisposeType  parse_disposal_tag           (Animation    *animation,
                                                  const gchar  *str);

static gboolean     is_ms_tag                    (const gchar  *str,
                                                  gint         *duration,
                                                  gint         *taglength);
static gboolean     is_disposal_tag              (const gchar  *str,
                                                  DisposeType  *disposal,
                                                  gint         *taglength);
static gint         animation_time_to_next       (Animation    *animation,
                                                  gboolean      reset);
static gboolean     advance_frame_callback       (Animation    *animation);
static gboolean     animation_must_redraw        (Animation    *animation,
                                                  gint          previous_frame,
                                                  gint          next_frame);


G_DEFINE_TYPE (Animation, animation, G_TYPE_OBJECT)

#define parent_class animation_parent_class

static GRegex *nospace_reg;
static GRegex *layers_reg;
static GRegex *all_reg;

static guint   animation_signals[LAST_SIGNAL] = { 0 };

static void
animation_class_init (AnimationClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS (klass);

    /* Loading in progress. The parameter is load percentage. */
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
    animation_signals[LOADING_START] =
      g_signal_new ("loading-start",
                    G_TYPE_FROM_CLASS (klass),
                    G_SIGNAL_RUN_FIRST,
                    0,
                    NULL, NULL,
                    NULL,
                    G_TYPE_NONE,
                    0);
    /* Loading ended. Parameters are:
     * - first_frame
     * - length
     * - playback_start
     * - playback_stop
     * - width
     * - height
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
    animation_signals[DISPOSAL_CHANGED] =
      g_signal_new ("disposal-changed",
                    G_TYPE_FROM_CLASS (klass),
                    G_SIGNAL_RUN_FIRST,
                    0,
                    NULL, NULL,
                    NULL,
                    G_TYPE_NONE,
                    1,
                    G_TYPE_INT);
    animation_signals[PROXY_CHANGED] =
      g_signal_new ("proxy-changed",
                    G_TYPE_FROM_CLASS (klass),
                    G_SIGNAL_RUN_FIRST,
                    0,
                    NULL, NULL,
                    NULL,
                    G_TYPE_NONE,
                    1,
                    G_TYPE_DOUBLE);
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
    animation_signals[PLAYBACK_RANGE] =
      g_signal_new ("playback-range",
                    G_TYPE_FROM_CLASS (klass),
                    G_SIGNAL_RUN_FIRST,
                    0,
                    NULL, NULL,
                    NULL,
                    G_TYPE_NONE,
                    2,
                    G_TYPE_INT,
                    G_TYPE_INT);
    animation_signals[RENDER] =
      g_signal_new ("render",
                    G_TYPE_FROM_CLASS (klass),
                    G_SIGNAL_RUN_FIRST,
                    0,
                    NULL, NULL,
                    NULL,
                    G_TYPE_NONE,
                    2,
                    G_TYPE_INT,
                    GEGL_TYPE_BUFFER);
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

    object_class->finalize     = animation_finalize;
    object_class->get_property = animation_get_property;
    object_class->set_property = animation_set_property;

    /**
     * Animation:image:
     *
     * The attached image id.
     */
    g_object_class_install_property (object_class, PROP_IMAGE,
                                     g_param_spec_int ("image", NULL, NULL,
                                                       0, G_MAXINT, 0,
                                                       G_PARAM_READWRITE |
                                                       G_PARAM_CONSTRUCT));

    /* Init the global variable: frame parsing. */
    nospace_reg = g_regex_new("[ \t]*", G_REGEX_OPTIMIZE, 0, NULL);
    layers_reg  = g_regex_new("\\[(([0-9]+(-[0-9]+)?)(,[0-9]+(-[0-9]+)?)*)\\]", G_REGEX_OPTIMIZE, 0, NULL);
    all_reg     = g_regex_new("\\[\\*\\]", G_REGEX_OPTIMIZE, 0, NULL);

    g_type_class_add_private (klass, sizeof (AnimationPrivate));
}

static void
animation_init (Animation *animation)
{
  AnimationPrivate *priv = ANIMATION_GET_PRIVATE (animation);

  /* Acceptable settings for the default. */
  priv->disposal    = DISPOSE_COMBINE;
  priv->proxy_ratio = 1.0;
  priv->framerate   = 24.0; /* fps */
}

/************ Public Functions ****************/

Animation *
animation_new (gint32 image_id)
{
  Animation *animation;

  animation = g_object_new (ANIMATION_TYPE_ANIMATION,
                            "image", image_id,
                            NULL);

  return animation;
}

void
animation_load (Animation   *animation,
                DisposeType  disposal,
                gdouble      proxy_ratio)
{
  AnimationPrivate *priv = ANIMATION_GET_PRIVATE (animation);

  gint             *layers;
  GeglBuffer       *buffer;
  gint              num_layers;
  gint32            new_frame, previous_frame, new_layer;
  gint              duration = 0;
  gint              image_width;
  gint              image_height;

  /* In case we were playing. */
  animation_stop (animation);

  priv->loaded = FALSE;

  if (! gimp_image_is_valid (priv->image_id))
    {
      gimp_message (_("Invalid image. Did you close it?"));
      return;
    }

  /* Block most UI during frame initialization. */
  g_signal_emit (animation, animation_signals[LOADING_START],
                 0, NULL);

  /* Cleanup before re-generation. */
  cleanup_cache (animation);

  if (disposal != DISPOSE_KEEP)
    {
      priv->disposal = disposal;
    }

  animation_init_frame_numbers (animation, disposal != DISPOSE_KEEP);
  layers = gimp_image_get_layers (priv->image_id, &num_layers);

  if (priv->num_frames <= 0)
    {
      goto animation_loaded;
    }

  priv->frames = g_try_malloc0_n (priv->num_frames, sizeof (Frame*));
  if (! priv->frames)
    {
      gimp_message (_("Memory could not be allocated to the frame container."));
      goto animation_loaded;
    }

  /* We default at full preview size. */
  image_width = gimp_image_width (priv->image_id);
  image_height = gimp_image_height (priv->image_id);

  priv->preview_width = image_width;
  priv->preview_height = image_height;

  if (proxy_ratio != PROXY_KEEP)
    {
      priv->proxy_ratio = proxy_ratio;
    }
  priv->preview_width  *= priv->proxy_ratio;
  priv->preview_height *= priv->proxy_ratio;

  /* We only use RGB images for display because indexed images would somehow
     render terrible colors. Layers from other types will be automatically
     converted. */
  priv->frames_image_id = gimp_image_new (priv->preview_width, priv->preview_height, GIMP_RGB);

  /* Save processing time and memory by not saving history and merged frames. */
  gimp_image_undo_disable (priv->frames_image_id);

  if (priv->disposal == DISPOSE_TAGS)
    {
      gint        i;

      for (i = 0; i < num_layers; i++)
        {
          g_signal_emit (animation, animation_signals[LOADING],
                         0, (gdouble) i / ((gdouble) num_layers - 0.999), NULL);
          rec_init_frames (animation,
                           priv->frames_image_id,
                           image_width, image_height,
                           layers[num_layers - (i + 1)],
                           priv->previous_frames);
        }

      for (i = 0; i < priv->num_frames; i++)
        {
          /* If for some reason a frame is absent, use the previous one.
           * We are ensured that there is at least a "first" frame for this. */
          if (! priv->frames[i])
            {
              priv->frames[i] = priv->frames[i - 1];
              priv->frames[i]->indexes = g_list_append (priv->frames[i]->indexes, GINT_TO_POINTER (i));
            }

          /* A zero duration only means we use the global duration, whatever it is at the time. */
          priv->frames[i]->duration = 0;
        }
    }
  else
    {
      gint   layer_offx;
      gint   layer_offy;
      gchar *layer_name;
      gint   i;

      for (i = 0; i < priv->num_frames; i++)
        {
          DisposeType tmp_disposal = priv->disposal;

          g_signal_emit (animation, animation_signals[LOADING],
                         0, (gdouble) i / ((gdouble) num_layers - 0.999), NULL);

          priv->frames[i] = g_new (Frame, 1);
          priv->frames[i]->indexes = NULL;
          priv->frames[i]->indexes = g_list_append (priv->frames[i]->indexes, GINT_TO_POINTER (i));
          priv->frames[i]->updated_indexes = NULL;

          priv->previous_frames = g_list_append (priv->previous_frames, priv->frames[i]);

          layer_name = gimp_item_get_name (layers[num_layers - (i + 1)]);
          if (layer_name)
            {
              duration = parse_ms_tag (layer_name);
              tmp_disposal = parse_disposal_tag (animation, layer_name);
              g_free (layer_name);
            }

          if (i > 0 && tmp_disposal != DISPOSE_REPLACE)
            {
              previous_frame = gimp_layer_copy (priv->frames[i - 1]->drawable_id);

              gimp_image_insert_layer (priv->frames_image_id, previous_frame, 0, 0);
              gimp_item_set_visible (previous_frame, TRUE);
            }

          new_layer = gimp_layer_new_from_drawable (layers[num_layers - (i + 1)], priv->frames_image_id);

          gimp_image_insert_layer (priv->frames_image_id, new_layer, 0, 0);
          gimp_item_set_visible (new_layer, TRUE);
          gimp_layer_scale (new_layer, (gimp_drawable_width (layers[num_layers - (i + 1)]) * (gint) priv->preview_width) / image_width,
                            (gimp_drawable_height (layers[num_layers - (i + 1)]) * (gint) priv->preview_height) / image_height, FALSE);
          gimp_drawable_offsets (layers[num_layers - (i + 1)], &layer_offx, &layer_offy);
          gimp_layer_set_offsets (new_layer, (layer_offx * (gint) priv->preview_width) / image_width,
                                  (layer_offy * (gint) priv->preview_height) / image_height);
          gimp_layer_resize_to_image_size (new_layer);

          if (gimp_item_is_group (new_layer))
            {
              gint    num_children;
              gint32 *children;
              gint    j;

              /* I want to make all layers in the group visible, so that when I'll make
               * the group visible too at render time, it will display everything in it. */
              children = gimp_item_get_children (new_layer, &num_children);
              for (j = 0; j < num_children; j++)
                gimp_item_set_visible (children[j], TRUE);
            }

          new_frame = gimp_image_merge_visible_layers (priv->frames_image_id, GIMP_CLIP_TO_IMAGE);
          priv->frames[i]->drawable_id = new_frame;
          gimp_item_set_visible (new_frame, FALSE);

          if (duration <= 0)
            duration = 0;
          priv->frames[i]->duration = (guint) duration;
        }
    }

animation_loaded:
  priv->loaded = TRUE;
  g_signal_emit (animation, animation_signals[DISPOSAL_CHANGED], 0,
                 priv->disposal);
  g_signal_emit (animation, animation_signals[PROXY_CHANGED], 0,
                 priv->proxy_ratio);
  g_signal_emit (animation, animation_signals[LOADED], 0,
                 priv->first_frame,
                 priv->num_frames,
                 animation_get_playback_start (animation),
                 animation_get_playback_stop (animation),
                 priv->preview_width,
                 priv->preview_height,
                 NULL);
  buffer = animation_get_frame (animation, priv->current_frame);
  g_signal_emit (animation, animation_signals[RENDER], 0,
                 priv->current_frame, buffer);
  g_free (layers);
}

void
animation_play (Animation   *animation)
{
  AnimationPrivate *priv = ANIMATION_GET_PRIVATE (animation);
  gdouble fps = animation_get_framerate (animation);
  guint duration = animation_time_to_next (animation, TRUE);

  if (priv->playback_timer)
    {
      /* It means we are already playing, so we should not need to play
       * again.
       * Still be liberal and simply remove the timer before creating a
       * new one. */
      g_source_remove (priv->playback_timer);
    }

  if (duration <= 0)
    duration = (guint) (1000.0 / fps);

  priv->playback_timer = g_timeout_add (duration, (GSourceFunc) advance_frame_callback, animation);
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
    }
}

gchar *
animation_get_name (Animation *animation)
{
  AnimationPrivate *priv = ANIMATION_GET_PRIVATE (animation);

  return gimp_image_get_name (priv->image_id);
}

DisposeType
animation_get_disposal (Animation *animation)
{
  AnimationPrivate *priv = ANIMATION_GET_PRIVATE (animation);

  return priv->disposal;
}

gdouble
animation_get_proxy (Animation *animation)
{
  AnimationPrivate *priv = ANIMATION_GET_PRIVATE (animation);

  return priv->proxy_ratio;
}

void
animation_get_size (Animation *animation,
                    gint      *width,
                    gint      *height)
{
  AnimationPrivate *priv = ANIMATION_GET_PRIVATE (animation);

  *width = priv->preview_width;
  *height = priv->preview_height;
}

gint
animation_get_length (Animation *animation)
{
  AnimationPrivate *priv = ANIMATION_GET_PRIVATE (animation);

  return priv->num_frames;
}

gint animation_get_first_frame (Animation *animation)
{
  AnimationPrivate *priv = ANIMATION_GET_PRIVATE (animation);

  return priv->first_frame;
}

gint
animation_get_position (Animation *animation)
{
  AnimationPrivate *priv = ANIMATION_GET_PRIVATE (animation);

  return priv->current_frame;
}

void
animation_set_playback_start (Animation *animation,
                              gint       frame_number)
{
  AnimationPrivate *priv = ANIMATION_GET_PRIVATE (animation);

  if (frame_number < priv->first_frame ||
      frame_number >= priv->first_frame + priv->num_frames)
    {
      priv->playback_start = priv->first_frame;
    }
  else
    {
      priv->playback_start = frame_number;
    }
  if (priv->playback_stop < priv->playback_start)
    {
      priv->playback_stop = priv->first_frame + priv->num_frames - 1;
    }

  g_signal_emit (animation, animation_signals[PLAYBACK_RANGE], 0,
                 priv->playback_start, priv->playback_stop);

  if (priv->current_frame < priv->playback_start ||
      priv->current_frame > priv->playback_stop)
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

  if (frame_number < priv->first_frame ||
      frame_number >= priv->first_frame + priv->num_frames)
    {
      priv->playback_stop = priv->first_frame + priv->num_frames - 1;
    }
  else
    {
      priv->playback_stop = frame_number;
    }
  if (priv->playback_stop < priv->playback_start)
    {
      priv->playback_start = priv->first_frame;
    }
  g_signal_emit (animation, animation_signals[PLAYBACK_RANGE], 0,
                 priv->playback_start, priv->playback_stop);

  if (priv->current_frame < priv->playback_start ||
      priv->current_frame > priv->playback_stop)
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

/* @return the GeglBuffer for the current frame.
 * The returned buffer must be unref-ed after use. */
GeglBuffer *
animation_get_frame (Animation *animation,
                     gint       frame_number)
{
  AnimationPrivate *priv = ANIMATION_GET_PRIVATE (animation);
  GeglBuffer       *buffer = NULL;

  if (priv->loaded && priv->num_frames > 0)
    buffer = gimp_drawable_get_buffer (priv->frames[frame_number - priv->first_frame]->drawable_id);

  return buffer;
}

void
animation_next (Animation *animation)
{
  AnimationPrivate *priv           = ANIMATION_GET_PRIVATE (animation);
  GeglBuffer       *buffer         = NULL;
  gint              previous_frame = priv->current_frame;

  priv->current_frame = animation_get_playback_start (animation) +
                        ((priv->current_frame - animation_get_playback_start (animation) + 1) %
                        (animation_get_playback_stop (animation) - animation_get_playback_start (animation) + 1));

  if (animation_must_redraw (animation, 
                             previous_frame,
                             priv->current_frame))
    {
      buffer = animation_get_frame (animation, priv->current_frame);
    }
  g_signal_emit (animation, animation_signals[RENDER], 0,
                 priv->current_frame, buffer);
}

void
animation_prev (Animation *animation)
{
  AnimationPrivate *priv           = ANIMATION_GET_PRIVATE (animation);
  GeglBuffer       *buffer         = NULL;
  gint              previous_frame = priv->current_frame;

  if (priv->current_frame == animation_get_playback_start (animation))
    {
      priv->current_frame = animation_get_playback_stop (animation);
    }
  else
    {
      --priv->current_frame;
    }

  if (animation_must_redraw (animation, 
                             previous_frame,
                             priv->current_frame))
    {
      buffer = animation_get_frame (animation, priv->current_frame);
    }
  g_signal_emit (animation, animation_signals[RENDER], 0,
                 priv->current_frame, buffer);
}

void
animation_jump (Animation *animation,
                gint       index)
{
  AnimationPrivate *priv           = ANIMATION_GET_PRIVATE (animation);
  GeglBuffer       *buffer         = NULL;
  gint              previous_frame = priv->current_frame;

  g_assert (index >= animation_get_playback_start (animation) &&
            index <= animation_get_playback_stop (animation));

  if (index < priv->playback_start ||
      index > priv->playback_stop)
    priv->current_frame = priv->playback_start;
  else
    priv->current_frame = index;

  if (animation_must_redraw (animation, 
                             previous_frame,
                             priv->current_frame))
    {
      buffer = animation_get_frame (animation, priv->current_frame);
    }
  g_signal_emit (animation, animation_signals[RENDER], 0,
                 priv->current_frame, buffer);
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
      priv->image_id       = g_value_get_int (value);
      priv->preview_width  = gimp_image_width (priv->image_id);
      priv->preview_height = gimp_image_height (priv->image_id);

      if (priv->proxy_ratio != 1.0)
        {
          priv->preview_width  *= priv->proxy_ratio;
          priv->preview_height *= priv->proxy_ratio;
        }
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
animation_get_property (GObject      *object,
                        guint         property_id,
                        GValue       *value,
                        GParamSpec   *pspec)
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

static void
animation_finalize (GObject *object)
{
  Animation *animation = ANIMATION (object);

  cleanup_cache (animation);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
cleanup_cache (Animation *animation)
{
  AnimationPrivate *priv = ANIMATION_GET_PRIVATE (animation);

  if (priv->frames)
    {
      GList *idx;

      gimp_image_delete (priv->frames_image_id);
      priv->frames_image_id = 0;

      /* Freeing previous frames only once. */
      for (idx = g_list_first (priv->previous_frames); idx != NULL; idx = g_list_next (idx))
        {
          Frame* frame = (Frame*) idx->data;

          g_list_free (frame->indexes);
          g_list_free (frame->updated_indexes);
          g_free (frame);
        }
      g_list_free (priv->previous_frames);
      priv->previous_frames = NULL;

      g_free (priv->frames);
      priv->frames = NULL;
    }
}

/**
 * Set num_frames, which is not necessarily the number of layers, in
 * particular with the DISPOSE_TAGS disposal.
 * Will set 0 if no layer has frame tags in tags mode, or if there is
 * no layer in combine/replace.
 */
static void
animation_init_frame_numbers (Animation   *animation,
                              gboolean     disposal_changed)
{
  AnimationPrivate *priv = ANIMATION_GET_PRIVATE (animation);
  gint             *layers;
  gint              num_layers;

  layers = gimp_image_get_layers (priv->image_id, &num_layers);

  if (priv->disposal != DISPOSE_TAGS)
    {
      priv->num_frames  = num_layers;
      priv->first_frame = 1;
    }
  else
    {
      gint i;
      gint max = G_MININT;
      gint min = G_MAXINT;

      for (i = 0; i < num_layers; i++)
        rec_set_total_frames (animation, layers[i], &min, &max);

      priv->num_frames  = (max > min)? max + 1 - min : 0;
      priv->first_frame = min;
    }
  if (priv->num_frames == 0)
    {
      priv->first_frame = 0;
      priv->playback_start = priv->playback_stop = 0;
    }

  if (disposal_changed)
    {
      /* Reset the playback range to first and last frame. */
      animation_set_playback_start (animation, priv->first_frame);
      animation_set_playback_stop (animation, priv->first_frame + priv->num_frames - 1);
      priv->current_frame = priv->first_frame;
    }
  else
    {
      /* Keep the same play state, unless it is now invalid. */
      animation_set_playback_start (animation, priv->playback_start);
      animation_set_playback_stop (animation, priv->playback_stop);
      if (priv->current_frame > animation_get_playback_stop (animation) ||
          priv->current_frame < animation_get_playback_start (animation))
        {
          priv->current_frame = priv->first_frame;
        }
    }
}

/**
 * A recursive call which will call itself for layer groups
 * and update the min/max progressively.
 **/
static void
rec_set_total_frames (Animation    *animation,
                      const gint32  layer,
                      gint         *min,
                      gint         *max)
{
  gchar        *layer_name;
  gchar        *nospace_name;
  GMatchInfo   *match_info;
  gint          i;

  if (gimp_item_is_group (layer))
    {
      gint    num_children;
      gint32 *children;

      children = gimp_item_get_children (layer, &num_children);
      for (i = 0; i < num_children; i++)
        rec_set_total_frames (animation, children[i], min, max);

      return;
    }

  layer_name   = gimp_item_get_name (layer);
  nospace_name = g_regex_replace_literal (nospace_reg, layer_name, -1, 0, "", 0, NULL);

  g_regex_match (layers_reg, nospace_name, 0, &match_info);

  while (g_match_info_matches (match_info))
    {
      gchar *tag = g_match_info_fetch (match_info, 1);
      gchar** tokens = g_strsplit(tag, ",", 0);

      for (i = 0; tokens[i] != NULL; i++)
        {
          gchar* hyphen;
          hyphen = g_strrstr(tokens[i], "-");
          if (hyphen != NULL)
            {
              gint32 first = (gint32) g_ascii_strtoll (tokens[i], NULL, 10);
              gint32 second = (gint32) g_ascii_strtoll (&hyphen[1], NULL, 10);
              *max = (second > first && second > *max)? second : *max;
              *min = (second > first && first < *min)? first : *min;
            }
          else
            {
              gint32 num = (gint32) g_ascii_strtoll (tokens[i], NULL, 10);
              *max = (num > *max)? num : *max;
              *min = (num < *min)? num : *min;
            }
        }
      g_strfreev (tokens);
      g_free (tag);
      g_free (layer_name);
      g_match_info_next (match_info, NULL);
    }

  g_free (nospace_name);
  g_match_info_free (match_info);
}

/**
 * Recursive call to generate frames in TAGS mode.
 **/
static void
rec_init_frames (Animation *animation,
                 gint32     frames_image_id,
                 gint       image_width,
                 gint       image_height,
                 gint32     layer,
                 GList     *previous_frames)
{
  AnimationPrivate *priv = ANIMATION_GET_PRIVATE (animation);
  Frame            *empty_frame = NULL;
  gchar            *layer_name;
  gchar            *nospace_name;
  GMatchInfo       *match_info;
  gboolean          preview_quality;
  gint32            new_layer;
  gint              j, k;

  if (gimp_item_is_group (layer))
    {
      gint    num_children;
      gint32 *children;

      children = gimp_item_get_children (layer, &num_children);
      for (j = 0; j < num_children; j++)
        rec_init_frames (animation, frames_image_id,
                         image_width, image_height,
                         children[num_children - j - 1],
                         previous_frames);

      return;
    }

  layer_name = gimp_item_get_name (layer);
  nospace_name = g_regex_replace_literal (nospace_reg, layer_name, -1, 0, "", 0, NULL);
  preview_quality = priv->proxy_ratio != 1.0;

  if (g_regex_match (all_reg, nospace_name, 0, NULL))
    {
      for (j = 0; j < priv->num_frames; j++)
        {
          if (! priv->frames[j])
            {
              if (! empty_frame)
                {
                  empty_frame = g_new (Frame, 1);
                  empty_frame->indexes = NULL;
                  empty_frame->updated_indexes = NULL;
                  empty_frame->drawable_id = 0;

                  previous_frames = g_list_append (previous_frames, empty_frame);
                }

              if (! g_list_find (empty_frame->updated_indexes, GINT_TO_POINTER (j)))
                empty_frame->updated_indexes = g_list_append (empty_frame->updated_indexes, GINT_TO_POINTER (j));

              priv->frames[j] = empty_frame;
            }
          else if (! g_list_find (priv->frames[j]->updated_indexes, GINT_TO_POINTER (j)))
            priv->frames[j]->updated_indexes = g_list_append (priv->frames[j]->updated_indexes, GINT_TO_POINTER (j));
        }
    }
  else
    {
      g_regex_match (layers_reg, nospace_name, 0, &match_info);

      while (g_match_info_matches (match_info))
        {
          gchar *tag = g_match_info_fetch (match_info, 1);
          gchar** tokens = g_strsplit(tag, ",", 0);

          for (j = 0; tokens[j] != NULL; j++)
            {
              gchar* hyphen = g_strrstr(tokens[j], "-");

              if (hyphen != NULL)
                {
                  gint32 first = (gint32) g_ascii_strtoll (tokens[j], NULL, 10);
                  gint32 second = (gint32) g_ascii_strtoll (&hyphen[1], NULL, 10);

                  for (k = first; k <= second; k++)
                    {
                      if (! priv->frames[k - priv->first_frame])
                        {
                          if (! empty_frame)
                            {
                              empty_frame = g_new (Frame, 1);
                              empty_frame->indexes = NULL;
                              empty_frame->updated_indexes = NULL;
                              empty_frame->drawable_id = 0;

                              previous_frames = g_list_append (previous_frames, empty_frame);
                            }

                          if (! g_list_find (priv->frames[k - priv->first_frame]->updated_indexes, GINT_TO_POINTER (k - priv->first_frame)))
                            empty_frame->updated_indexes = g_list_append (empty_frame->updated_indexes, GINT_TO_POINTER (k - priv->first_frame));

                          priv->frames[k - priv->first_frame] = empty_frame;
                        }
                      else if (! g_list_find (priv->frames[k - priv->first_frame]->updated_indexes, GINT_TO_POINTER (k - priv->first_frame)))
                        priv->frames[k - priv->first_frame]->updated_indexes = g_list_append (priv->frames[k - priv->first_frame]->updated_indexes,
                                                                                       GINT_TO_POINTER (k - priv->first_frame));
                    }
                }
              else
                {
                  gint32 num = (gint32) g_ascii_strtoll (tokens[j], NULL, 10);

                  if (! priv->frames[num - priv->first_frame])
                    {
                      if (! empty_frame)
                        {
                          empty_frame = g_new (Frame, 1);
                          empty_frame->indexes = NULL;
                          empty_frame->updated_indexes = NULL;
                          empty_frame->drawable_id = 0;

                          previous_frames = g_list_append (previous_frames, empty_frame);
                        }

                      if (! g_list_find (priv->frames[num - priv->first_frame]->updated_indexes, GINT_TO_POINTER (num - priv->first_frame)))
                        empty_frame->updated_indexes = g_list_append (empty_frame->updated_indexes, GINT_TO_POINTER (num - priv->first_frame));

                      priv->frames[num - priv->first_frame] = empty_frame;
                    }
                  else if (! g_list_find (priv->frames[num - priv->first_frame]->updated_indexes, GINT_TO_POINTER (num - priv->first_frame)))
                    priv->frames[num - priv->first_frame]->updated_indexes = g_list_append (priv->frames[num - priv->first_frame]->updated_indexes,
                                                                                     GINT_TO_POINTER (num - priv->first_frame));
                }
            }
          g_strfreev (tokens);
          g_free (tag);
          g_match_info_next (match_info, NULL);
        }
      g_match_info_free (match_info);
    }

  for (j = 0; j < priv->num_frames; j++)
    {
      /* Check which frame must be updated with the current layer. */
      if (priv->frames[j] && g_list_length (priv->frames[j]->updated_indexes))
        {
          new_layer = gimp_layer_new_from_drawable (layer, frames_image_id);
          gimp_image_insert_layer (frames_image_id, new_layer, 0, 0);

          if (preview_quality)
            {
              gint layer_offx, layer_offy;

              gimp_layer_scale (new_layer,
                                (gimp_drawable_width (layer) * (gint) priv->preview_width) / image_width,
                                (gimp_drawable_height (layer) * (gint) priv->preview_height) / image_height,
                                FALSE);
              gimp_drawable_offsets (layer, &layer_offx, &layer_offy);
              gimp_layer_set_offsets (new_layer, (layer_offx * (gint) priv->preview_width) / image_width,
                                      (layer_offy * (gint) priv->preview_height) / image_height);
            }
          gimp_layer_resize_to_image_size (new_layer);

          if (priv->frames[j]->drawable_id == 0)
            {
              priv->frames[j]->drawable_id = new_layer;
              priv->frames[j]->indexes = priv->frames[j]->updated_indexes;
              priv->frames[j]->updated_indexes = NULL;
            }
          else if (g_list_length (priv->frames[j]->indexes) == g_list_length (priv->frames[j]->updated_indexes))
            {
              gimp_item_set_visible (new_layer, TRUE);
              gimp_item_set_visible (priv->frames[j]->drawable_id, TRUE);

              priv->frames[j]->drawable_id = gimp_image_merge_visible_layers (frames_image_id, GIMP_CLIP_TO_IMAGE);
              g_list_free (priv->frames[j]->updated_indexes);
              priv->frames[j]->updated_indexes = NULL;
            }
          else
            {
              GList    *idx;
              gboolean  move_j = FALSE;
              Frame    *forked_frame = g_new (Frame, 1);
              gint32    forked_drawable_id = gimp_layer_new_from_drawable (priv->frames[j]->drawable_id, frames_image_id);

              /* if part only of the priv->frames are updated, we fork the existing frame. */
              gimp_image_insert_layer (frames_image_id, forked_drawable_id, 0, 1);
              gimp_item_set_visible (new_layer, TRUE);
              gimp_item_set_visible (forked_drawable_id, TRUE);
              forked_drawable_id = gimp_image_merge_visible_layers (frames_image_id, GIMP_CLIP_TO_IMAGE);

              forked_frame->drawable_id = forked_drawable_id;
              forked_frame->indexes = priv->frames[j]->updated_indexes;
              forked_frame->updated_indexes = NULL;
              priv->frames[j]->updated_indexes = NULL;

              for (idx = g_list_first (forked_frame->indexes); idx != NULL; idx = g_list_next (idx))
                {
                  priv->frames[j]->indexes = g_list_remove (priv->frames[j]->indexes, idx->data);
                  if (GPOINTER_TO_INT (idx->data) != j)
                    priv->frames[GPOINTER_TO_INT (idx->data)] = forked_frame;
                  else
                    /* Frame j must also be moved to the forked frame, but only after the loop. */
                    move_j = TRUE;
                }
              if (move_j)
                priv->frames[j] = forked_frame;

              gimp_item_set_visible (forked_drawable_id, FALSE);

              previous_frames = g_list_append (previous_frames, forked_frame);
            }

          gimp_item_set_visible (priv->frames[j]->drawable_id, FALSE);
        }
    }

  g_free (layer_name);
  g_free (nospace_name);
}

static gint
parse_ms_tag (const gchar *str)
{
  gint i;
  gint length = strlen (str);

  for (i = 0; i < length; i++)
    {
      gint rtn;
      gint dummy;

      if (is_ms_tag (&str[i], &rtn, &dummy))
        return rtn;
    }

  return -1;
}

static DisposeType
parse_disposal_tag (Animation* animation,
                    const gchar         *str)
{
  AnimationPrivate *priv = ANIMATION_GET_PRIVATE (animation);
  gint             length = strlen (str);
  gint             i;

  for (i = 0; i < length; i++)
    {
      DisposeType rtn;
      gint        dummy;

      if (is_disposal_tag (&str[i], &rtn, &dummy))
        return rtn;
    }

  return priv->disposal;
}

/******* TAG UTILS **************/

static gboolean
is_ms_tag (const gchar *str,
           gint        *duration,
           gint        *taglength)
{
  gint sum = 0;
  gint offset;
  gint length;

  length = strlen(str);

  if (str[0] != '(')
    return FALSE;

  offset = 1;

  /* eat any spaces between open-parenthesis and number */
  while ((offset < length) && (str[offset] == ' '))
    offset++;

  if ((offset>=length) || (!g_ascii_isdigit (str[offset])))
    return FALSE;

  do
    {
      sum *= 10;
      sum += str[offset] - '0';
      offset++;
    }
  while ((offset<length) && (g_ascii_isdigit (str[offset])));

  if (length - offset <= 2)
    return FALSE;

  /* eat any spaces between number and 'ms' */
  while ((offset < length) && (str[offset] == ' '))
    offset++;

  if (length - offset <= 2                     ||
      g_ascii_toupper (str[offset])     != 'M' ||
      g_ascii_toupper (str[offset + 1]) != 'S')
    return FALSE;

  offset += 2;

  /* eat any spaces between 'ms' and close-parenthesis */
  while ((offset < length) && (str[offset] == ' '))
    offset++;

  if ((length - offset < 1) || (str[offset] != ')'))
    return FALSE;

  offset++;

  *duration = sum;
  *taglength = offset;

  return TRUE;
}

static gboolean
is_disposal_tag (const gchar *str,
                 DisposeType *disposal,
                 gint        *taglength)
{
  if (strlen (str) != 9)
    return FALSE;

  if (strncmp (str, "(combine)", 9) == 0)
    {
      *taglength = 9;
      *disposal = DISPOSE_COMBINE;
      return TRUE;
    }
  else if (strncmp (str, "(replace)", 9) == 0)
    {
      *taglength = 9;
      *disposal = DISPOSE_REPLACE;
      return TRUE;
    }

  return FALSE;
}

static gint
animation_time_to_next (Animation *animation,
                        gboolean   reset)
{
  AnimationPrivate *priv = ANIMATION_GET_PRIVATE (animation);
  static gint64     start_time = -1;
  static gint       frames = 0;
  static gboolean   prev_low_framerate = FALSE;
  gdouble           expected_time_from_start = 0.0;
  gdouble           duration;

  duration = priv->frames[priv->current_frame - priv->first_frame]->duration;
  if (duration <= 0)
    {
      duration = (guint) (1000.0 / priv->framerate);
    }

  if (priv->disposal != DISPOSE_COMBINE && priv->disposal != DISPOSE_REPLACE)
    {
      /* Replace and Combine disposals can have different duration for each
       * frame (by tagging) making it harder to compute any framerate check. */
      if (reset || start_time == -1)
        {
          start_time = g_get_monotonic_time ();
          frames = 0;
        }

      expected_time_from_start = (gdouble) frames * 1000.0 / priv->framerate;
      frames++;

      if (frames > 1)
        duration = duration - (((gdouble) (g_get_monotonic_time () - start_time)) / 1000.0 - expected_time_from_start);

      if (duration < 1.0)
        {
          if (prev_low_framerate)
            {
              /* Let's only warn the user if several subsequent frames slow. */
              gdouble real_framerate = (gdouble) frames * 1000000.0 / (g_get_monotonic_time () - start_time);
              g_signal_emit (animation, animation_signals[LOW_FRAMERATE], 0,
                             real_framerate);
            }
          duration = 1.0;
          prev_low_framerate = TRUE;
        }
      else
        {
          prev_low_framerate = FALSE;
        }

    }

  return duration;
}

static gboolean
advance_frame_callback (Animation *animation)
{
  AnimationPrivate *priv = ANIMATION_GET_PRIVATE (animation);
  guint duration;
  gboolean reset;

  reset = (animation_get_position (animation) == animation_get_playback_stop (animation));
  animation_next (animation);
  duration = animation_time_to_next (animation, reset);

  priv->playback_timer = g_timeout_add (duration,
                                        (GSourceFunc) advance_frame_callback,
                                        (Animation *) animation);

  return G_SOURCE_REMOVE;
}

gboolean
animation_must_redraw (Animation *animation,
                       gint       previous_frame,
                       gint       next_frame)
{
  AnimationPrivate *priv = ANIMATION_GET_PRIVATE (animation);

  return (previous_frame < priv->first_frame ||
          previous_frame >= priv->first_frame + priv->num_frames ||
          (priv->num_frames > 0 &&
          NULL == g_list_find (priv->frames[previous_frame - priv->first_frame]->indexes,
                               GINT_TO_POINTER (next_frame - priv->first_frame))));
}
