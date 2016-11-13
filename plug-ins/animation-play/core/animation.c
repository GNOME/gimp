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
#include "animation-animatic.h"
#include "animation-celanimation.h"

/* Settings we cache assuming they may be the user's
 * favorite, like a framerate.
 * These will be used only for image without stored animation. */
typedef struct
{
  gdouble framerate;
}
CachedSettings;

enum
{
  LOADING,
  LOADED,
  CACHE_INVALIDATED,
  DURATION_CHANGED,
  FRAMERATE_CHANGED,
  PROXY_CHANGED,
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
  gint32    image_id;
  gchar    *xml;

  gdouble   framerate;

  /* Proxy settings generates a reload. */
  gdouble   proxy_ratio;

  gboolean  loaded;
};


#define ANIMATION_GET_PRIVATE(animation) \
        G_TYPE_INSTANCE_GET_PRIVATE (animation, \
                                     ANIMATION_TYPE_ANIMATION, \
                                     AnimationPrivate)

static void       animation_finalize               (GObject      *object);
static void       animation_set_property           (GObject      *object,
                                                    guint         property_id,
                                                    const GValue *value,
                                                    GParamSpec   *pspec);
static void       animation_get_property           (GObject      *object,
                                                    guint         property_id,
                                                    GValue       *value,
                                                    GParamSpec   *pspec);

/* Base implementation of virtual methods. */
static gboolean   animation_real_same              (Animation *animation,
                                                    gint       prev_pos,
                                                    gint       next_pos);

G_DEFINE_TYPE (Animation, animation, G_TYPE_OBJECT)

#define parent_class animation_parent_class

static guint animation_signals[LAST_SIGNAL] = { 0 };

static void
animation_class_init (AnimationClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  /**
   * Animation::loading:
   * @animation: the animation loading.
   * @ratio: fraction loaded [0-1].
   *
   * The ::loading signal must be emitted by a subclass of Animation.
   * It can be used by a GUI to display a progress bar during loading.
   * GUI widgets depending on a consistent state of @animation should
   * become unresponsive.
   */
  animation_signals[LOADING] =
    g_signal_new ("loading",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (AnimationClass, loading),
                  NULL, NULL,
                  g_cclosure_marshal_VOID__DOUBLE,
                  G_TYPE_NONE,
                  1,
                  G_TYPE_DOUBLE);
  /**
   * Animation::loaded:
   * @animation: the animation loading.
   * @duration: number of frames.
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
                  G_STRUCT_OFFSET (AnimationClass, loaded),
                  NULL, NULL,
                  g_cclosure_marshal_VOID__VOID,
                  G_TYPE_NONE,
                  0);
  /**
   * Animation::cache-invalidated:
   * @animation: the animation.
   * @position: the first frame position whose cache is invalid.
   * @length: the number of invalidated frames from @position.
   *
   * The ::cache-invalidated signal must be emitted when the contents
   * of one or more successive frames change.
   */
  animation_signals[CACHE_INVALIDATED] =
    g_signal_new ("cache-invalidated",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (AnimationClass, cache_invalidated),
                  NULL, NULL,
                  NULL,
                  G_TYPE_NONE,
                  2,
                  G_TYPE_INT,
                  G_TYPE_INT);
  /**
   * Animation::duration:
   * @animation: the animation.
   * @duration: the new duration of @animation in number of frames.
   *
   * The ::playback-range signal must be emitted when the duration of
   * @animation changes.
   */
  animation_signals[DURATION_CHANGED] =
    g_signal_new ("duration-changed",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (AnimationClass, duration_changed),
                  NULL, NULL,
                  g_cclosure_marshal_VOID__INT,
                  G_TYPE_NONE,
                  1,
                  G_TYPE_INT);
  /**
   * Animation::framerate-changed:
   * @animation: the animation.
   * @framerate: the new framerate of @animation in frames per second.
   *
   * The ::playback-range signal is emitted when the framerate of
   * @animation changes.
   */
  animation_signals[FRAMERATE_CHANGED] =
    g_signal_new ("framerate-changed",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (AnimationClass, framerate_changed),
                  NULL, NULL,
                  g_cclosure_marshal_VOID__DOUBLE,
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
  animation_signals[PROXY_CHANGED] =
    g_signal_new ("proxy-changed",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (AnimationClass, proxy),
                  NULL, NULL,
                  g_cclosure_marshal_VOID__DOUBLE,
                  G_TYPE_NONE,
                  1,
                  G_TYPE_DOUBLE);

  object_class->finalize     = animation_finalize;
  object_class->set_property = animation_set_property;
  object_class->get_property = animation_get_property;

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
  CachedSettings    settings;

  /* Acceptable default settings. */
  settings.framerate = 24.0;

  /* If we saved any settings globally, use the one from the last run. */
  gimp_get_data (PLUG_IN_PROC, &settings);

  /* Acceptable settings for the default. */
  priv->framerate   = settings.framerate;
  priv->proxy_ratio = 1.0;
}

/************ Public Functions ****************/

Animation *
animation_new (gint32       image_id,
               gboolean     animatic,
               const gchar *xml)
{
  Animation        *animation;
  AnimationPrivate *priv;

  animation = g_object_new (animatic? ANIMATION_TYPE_ANIMATIC :
                                      ANIMATION_TYPE_CEL_ANIMATION,
                            "image", image_id,
                            NULL);
  priv = ANIMATION_GET_PRIVATE (animation);
  priv->xml = g_strdup (xml);

  return animation;
}

gint32
animation_get_image_id (Animation *animation)
{
  AnimationPrivate *priv = ANIMATION_GET_PRIVATE (animation);

  return priv->image_id;
}

void
animation_load (Animation *animation)
{
  AnimationPrivate *priv = ANIMATION_GET_PRIVATE (animation);

  priv->loaded = FALSE;
  g_signal_emit (animation, animation_signals[LOADING], 0, 0.0);

  if (priv->xml)
    ANIMATION_GET_CLASS (animation)->load_xml (animation,
                                               priv->xml);
  else
    ANIMATION_GET_CLASS (animation)->load (animation);

  /* XML is only used for the first load.
   * Any next loads will use internal data. */
  g_free (priv->xml);
  priv->xml = NULL;

  priv->loaded = TRUE;

  /* XXX */
  /*g_signal_emit (animation, animation_signals[LOADED], 0);*/
}

void
animation_save_to_parasite (Animation *animation)
{
  AnimationPrivate *priv = ANIMATION_GET_PRIVATE (animation);
  GimpParasite     *old_parasite;
  const gchar      *parasite_name;
  const gchar      *selected;
  gchar            *xml;
  gboolean          undo_step_started = FALSE;
  CachedSettings    settings;

  /* First saving in cache as default in the same session. */
  settings.framerate = animation_get_framerate (animation);
  gimp_set_data (PLUG_IN_PROC, &settings, sizeof (&settings));

  /* Then as a parasite for the specific image. */
  xml = ANIMATION_GET_CLASS (animation)->serialize (animation);

  if (ANIMATION_IS_ANIMATIC (animation))
    {
      selected = "animatic";
      parasite_name = PLUG_IN_PROC "/animatic";
    }
  else /* ANIMATION_IS_CEL_ANIMATION */
    {
      selected = "cel-animation";
      parasite_name = PLUG_IN_PROC "/cel-animation";
    }
  /* If there was already parasites and they were all the same as the
   * current state, do not resave them.
   * This prevents setting the image in a dirty state while it stayed
   * the same. */
  old_parasite = gimp_image_get_parasite (priv->image_id, parasite_name);
  if (xml && (! old_parasite ||
              g_strcmp0 ((gchar *) gimp_parasite_data (old_parasite), xml)))
    {
      GimpParasite *parasite;

      if (! undo_step_started)
        {
          gimp_image_undo_group_start (priv->image_id);
          undo_step_started = TRUE;
        }
      parasite = gimp_parasite_new (parasite_name,
                                    GIMP_PARASITE_PERSISTENT | GIMP_PARASITE_UNDOABLE,
                                    strlen (xml) + 1, xml);
      gimp_image_attach_parasite (priv->image_id, parasite);
      gimp_parasite_free (parasite);
    }
  gimp_parasite_free (old_parasite);

  old_parasite = gimp_image_get_parasite (priv->image_id,
                                          PLUG_IN_PROC "/selected");
  if (! old_parasite ||
      g_strcmp0 ((gchar *) gimp_parasite_data (old_parasite), selected))
    {
      GimpParasite *parasite;

      if (! undo_step_started)
        {
          gimp_image_undo_group_start (priv->image_id);
          undo_step_started = TRUE;
        }
      parasite = gimp_parasite_new (PLUG_IN_PROC "/selected",
                                    GIMP_PARASITE_PERSISTENT | GIMP_PARASITE_UNDOABLE,
                                    strlen (selected) + 1, selected);
      gimp_image_attach_parasite (priv->image_id, parasite);
      gimp_parasite_free (parasite);
    }
  gimp_parasite_free (old_parasite);

  if (undo_step_started)
    {
      gimp_image_undo_group_end (priv->image_id);
    }
}

gint
animation_get_duration (Animation *animation)
{
  return ANIMATION_GET_CLASS (animation)->get_duration (animation);
}

void
animation_set_proxy (Animation *animation,
                     gdouble    ratio)
{
  AnimationPrivate *priv = ANIMATION_GET_PRIVATE (animation);

  g_return_if_fail (ratio > 0.0 && ratio <= 1.0);

  if (priv->proxy_ratio != ratio)
    {
      priv->proxy_ratio = ratio;
      g_signal_emit (animation, animation_signals[PROXY_CHANGED], 0, ratio);

      /* A proxy change implies a reload. */
      animation_load (animation);
    }
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
  gint              image_width;
  gint              image_height;

  image_width  = gimp_image_width (priv->image_id);
  image_height = gimp_image_height (priv->image_id);

  /* Full preview size. */
  *width  = image_width;
  *height = image_height;

  /* Apply proxy ratio. */
  *width  *= priv->proxy_ratio;
  *height *= priv->proxy_ratio;
}

GeglBuffer *
animation_get_frame (Animation *animation,
                     gint       pos)
{
  return ANIMATION_GET_CLASS (animation)->get_frame (animation, pos);
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
animation_finalize (GObject *object)
{
  Animation        *animation = ANIMATION (object);
  AnimationPrivate *priv = ANIMATION_GET_PRIVATE (animation);

  if (priv->xml)
    g_free (priv->xml);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

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

static gboolean
animation_real_same (Animation *animation,
                     gint       previous_pos,
                     gint       next_pos)
{
  /* By default all frames are supposed different. */
  return (previous_pos == next_pos);
}
