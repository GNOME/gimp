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
#include "animation-playback.h"

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
  SIZE_CHANGED,
  FRAMES_CHANGED,
  DURATION_CHANGED,
  FRAMERATE_CHANGED,
  LAST_SIGNAL
};

enum
{
  PROP_0,
  PROP_IMAGE,
  PROP_XML
};

typedef struct _AnimationPrivate AnimationPrivate;

struct _AnimationPrivate
{
  gint32    image_id;

  /* Animation size may be different from image size. */
  gint      width;
  gint      height;

  gdouble   framerate;

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
   * Animation::size-changed:
   * @animation: the animation.
   * @width: @animation width.
   * @height: @animation height.
   *
   * The ::size-changed signal will be emitted when @animation display
   * size changes.
   */
  animation_signals[SIZE_CHANGED] =
    g_signal_new ("size-changed",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (AnimationClass, size_changed),
                  NULL, NULL,
                  NULL,
                  G_TYPE_NONE,
                  2,
                  G_TYPE_INT,
                  G_TYPE_INT);
  /**
   * Animation::frames-changed:
   * @animation: the animation.
   * @position: the first frame position whose contents changed.
   * @length: the number of changed frames from @position.
   *
   * The ::frames-changed signal must be emitted when the contents
   * of one or more successive frames change.
   */
  animation_signals[FRAMES_CHANGED] =
    g_signal_new ("frames-changed",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (AnimationClass, frames_changed),
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
   * The ::duration signal must be emitted when the duration of
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
   * The ::framerate-changed signal is emitted when the framerate of
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

  object_class->finalize     = animation_finalize;
  object_class->set_property = animation_set_property;
  object_class->get_property = animation_get_property;

  /**
   * Animation:image:
   *
   * The associated image id.
   */
  g_object_class_install_property (object_class, PROP_IMAGE,
                                   g_param_spec_int ("image", NULL, NULL,
                                                     0, G_MAXINT, 0,
                                                     G_PARAM_READWRITE |
                                                     G_PARAM_CONSTRUCT_ONLY));
  /**
   * Animation:xml:
   *
   * The animation serialized as a XML string.
   */
  g_object_class_install_property (object_class, PROP_XML,
                                   g_param_spec_string ("xml", NULL,
                                                        NULL, NULL,
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
}

/************ Public Functions ****************/

Animation *
animation_new (gint32       image_id,
               gboolean     animatic,
               const gchar *xml)
{
  Animation *animation;

  animation = g_object_new (animatic? ANIMATION_TYPE_ANIMATIC :
                                      ANIMATION_TYPE_CEL_ANIMATION,
                            "image", image_id,
                            "xml", xml,
                            NULL);
  g_signal_emit (animation, animation_signals[FRAMES_CHANGED], 0,
                 0, animation_get_duration (animation));

  return animation;
}

gint32
animation_get_image_id (Animation *animation)
{
  AnimationPrivate *priv = ANIMATION_GET_PRIVATE (animation);

  return priv->image_id;
}

/**
 * animation_load:
 * @animation: the #Animation.
 *
 * Cache the whole animation. This is to be used at the start, or each
 * time you want to reload the image contents.
 **/
void
animation_load (Animation *animation)
{
  AnimationPrivate *priv = ANIMATION_GET_PRIVATE (animation);

  priv->loaded = FALSE;
  g_signal_emit (animation, animation_signals[FRAMES_CHANGED], 0,
                 0, animation_get_duration (animation));
  priv->loaded = TRUE;
  g_signal_emit (animation, animation_signals[LOADED], 0);
}

void
animation_save_to_parasite (Animation   *animation,
                            const gchar *playback_xml)
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
  xml = ANIMATION_GET_CLASS (animation)->serialize (animation,
                                                    playback_xml);

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
  if (xml)
    g_free (xml);

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
animation_set_size (Animation *animation,
                    gint       width,
                    gint       height)
{
  AnimationPrivate *priv = ANIMATION_GET_PRIVATE (animation);

  if (width != priv->width || height != priv->height)
    {
      priv->width  = width;
      priv->height = height;
      g_signal_emit (animation, animation_signals[SIZE_CHANGED], 0,
                     width, height);
    }
}

void
animation_get_size (Animation *animation,
                    gint      *width,
                    gint      *height)
{
  AnimationPrivate *priv = ANIMATION_GET_PRIVATE (animation);

  if (width)
    *width  = priv->width;
  if (height)
    *height = priv->height;
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

void
animation_update_paint_view (Animation *animation,
                             gint       position)
{
  ANIMATION_GET_CLASS (animation)->update_paint_view (animation, position);
  gimp_displays_flush ();
}

/************ Private Functions ****************/

static void
animation_finalize (GObject *object)
{
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
    case PROP_XML:
        {
          const gchar *xml   = g_value_get_string (value);
          GError      *error = NULL;
          gint         width;
          gint         height;

          if (! xml ||
              ! ANIMATION_GET_CLASS (animation)->deserialize (animation,
                                                              xml,
                                                              &error))
            {
              if (error)
                g_warning ("Error parsing XML: %s", error->message);

              /* First time or XML parsing failed: reset to defaults. */
              ANIMATION_GET_CLASS (animation)->reset_defaults (animation);
            }
          g_clear_error (&error);

          animation_get_size (animation, &width, &height);
          if (width <= 0 || height <= 0)
            {
              /* Default display size is the size of the image. */
              animation_set_size (animation,
                                  gimp_image_width (priv->image_id),
                                  gimp_image_height (priv->image_id));
            }
        }
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
    case PROP_XML:
        {
          gchar *xml;

          xml = ANIMATION_GET_CLASS (animation)->serialize (animation,
                                                            NULL);
          g_value_take_string (value, xml);
          g_free (xml);
        }
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}
