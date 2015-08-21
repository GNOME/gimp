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

#include "animationlegacy.h"

enum
{
  PROP_0,
  PROP_DISPOSAL
};

typedef struct _AnimationLegacyPrivate AnimationLegacyPrivate;

struct _AnimationLegacyPrivate
{
  /* Accepts 2 disposals: combine and replace. */
  DisposeType  disposal;

  gint         preview_width;
  gint         preview_height;

  /* Frames are associated to an unused GimpImage. */
  gint32       frames;
  gint        *durations;
};

#define GET_PRIVATE(animation) \
        G_TYPE_INSTANCE_GET_PRIVATE (animation, \
                                     ANIMATION_TYPE_ANIMATION_LEGACY, \
                                     AnimationLegacyPrivate)

static void         animation_legacy_set_property (GObject      *object,
                                                   guint         property_id,
                                                   const GValue *value,
                                                   GParamSpec   *pspec);
static void         animation_legacy_get_property (GObject      *object,
                                                   guint         property_id,
                                                   GValue       *value,
                                                   GParamSpec   *pspec);
static void         animation_legacy_finalize     (GObject      *object);

/* Virtual methods */

static DisposeType  animation_legacy_get_disposal (Animation *animation);
static gint         animation_legacy_get_length   (Animation *animation);
static void         animation_legacy_get_size     (Animation *animation,
                                                   gint      *width,
                                                   gint      *height);

static void         animation_legacy_load         (Animation *animation,
                                                   gdouble    proxy_ratio);
static GeglBuffer * animation_legacy_get_frame    (Animation *animation,
                                                   gint       pos);
static gint         animation_legacy_time_to_next (Animation *animation);

/* Tag handling (from layer names) */

static gint         parse_ms_tag                 (const gchar  *str);
static DisposeType  parse_disposal_tag           (Animation    *animation,
                                                  const gchar  *str);

static gboolean     is_ms_tag                    (const gchar  *str,
                                                  gint         *duration,
                                                  gint         *taglength);
static gboolean     is_disposal_tag              (const gchar  *str,
                                                  DisposeType  *disposal,
                                                  gint         *taglength);

G_DEFINE_TYPE (AnimationLegacy, animation_legacy, ANIMATION_TYPE_ANIMATION)

#define parent_class animation_legacy_parent_class

static void
animation_legacy_class_init (AnimationLegacyClass *klass)
{
  GObjectClass   *object_class = G_OBJECT_CLASS (klass);
  AnimationClass *anim_class   = ANIMATION_CLASS (klass);

  object_class->finalize     = animation_legacy_finalize;
  object_class->set_property = animation_legacy_set_property;
  object_class->get_property = animation_legacy_get_property;

  anim_class->get_disposal   = animation_legacy_get_disposal;
  anim_class->get_length     = animation_legacy_get_length;
  anim_class->get_size       = animation_legacy_get_size;
  anim_class->load           = animation_legacy_load;
  anim_class->get_frame      = animation_legacy_get_frame;
  anim_class->time_to_next   = animation_legacy_time_to_next;

  g_object_class_install_property (object_class, PROP_DISPOSAL,
                                   g_param_spec_int ("disposal", NULL, NULL,
                                                     0, DISPOSE_REPLACE,
                                                     DISPOSE_COMBINE,
                                                     G_PARAM_READWRITE |
                                                     G_PARAM_CONSTRUCT_ONLY));

  g_type_class_add_private (klass, sizeof (AnimationLegacyPrivate));
}

static void
animation_legacy_init (AnimationLegacy *animation)
{
}

static void
animation_legacy_set_property (GObject      *object,
                               guint         property_id,
                               const GValue *value,
                               GParamSpec   *pspec)
{
  AnimationLegacyPrivate *priv = GET_PRIVATE (object);

  switch (property_id)
    {
    case PROP_DISPOSAL:
      priv->disposal = g_value_get_int (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
animation_legacy_get_property (GObject      *object,
                               guint         property_id,
                               GValue       *value,
                               GParamSpec   *pspec)
{
  AnimationLegacyPrivate *priv = GET_PRIVATE (object);

  switch (property_id)
    {
    case PROP_DISPOSAL:
      g_value_set_int (value, priv->disposal);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
animation_legacy_finalize (GObject *object)
{
  AnimationLegacyPrivate *priv = GET_PRIVATE (object);

  gimp_image_delete (priv->frames);
  g_free (priv->durations);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

/* Virtual methods */

static DisposeType
animation_legacy_get_disposal (Animation *animation)
{
  return GET_PRIVATE (animation)->disposal;
}

static gint
animation_legacy_get_length (Animation *animation)
{
  gint   *layers;
  gint    num_layers;
  gint32  image_id;

  image_id = animation_get_image_id (animation);
  layers   = gimp_image_get_layers (image_id, &num_layers);
  g_free (layers);

  return num_layers;
}

static void
animation_legacy_get_size (Animation *animation,
                           gint      *width,
                           gint      *height)
{
  AnimationLegacyPrivate *priv = GET_PRIVATE (animation);

  *width  = priv->preview_width;
  *height = priv->preview_height;
}

static void
animation_legacy_load (Animation *animation,
                       gdouble    proxy_ratio)
{
  AnimationLegacyPrivate *priv = GET_PRIVATE (animation);
  gint32                 *layers;
  gint32                  image_id;
  gint32                  num_layers;
  gint32                  new_frame;
  gint32                  previous_frame = 0;
  gint                    image_width;
  gint                    image_height;
  gint                    i;

  g_return_if_fail (proxy_ratio > 0.0 && proxy_ratio <= 1.0);

  /* Cleaning. */
  if (gimp_image_is_valid (priv->frames))
    {
      gimp_image_delete (priv->frames);
      g_free (priv->durations);
    }

  image_id = animation_get_image_id (animation);
  layers   = gimp_image_get_layers (image_id, &num_layers);

  priv->durations = g_try_malloc0_n (num_layers, sizeof (gint));
  if (! priv->durations)
    {
      gimp_message (_("Memory could not be allocated to the frame container."));
      gimp_quit ();
      return;
    }

  /* We default at full preview size. */
  image_width = gimp_image_width (image_id);
  image_height = gimp_image_height (image_id);

  priv->preview_width = image_width;
  priv->preview_height = image_height;

  /* Apply proxy ratio. */
  priv->preview_width  *= proxy_ratio;
  priv->preview_height *= proxy_ratio;

  priv->frames = gimp_image_new (priv->preview_width,
                                 priv->preview_width,
                                 GIMP_RGB);

  gimp_image_undo_disable (priv->frames);

  for (i = 0; i < num_layers; i++)
    {
      gchar       *layer_name;
      gint         duration;
      DisposeType  disposal;
      gint         layer_offx;
      gint         layer_offy;

      g_signal_emit_by_name (animation, "loading",
                             (gdouble) i / ((gdouble) num_layers - 0.999));

      layer_name = gimp_item_get_name (layers[num_layers - (i + 1)]);

      duration = parse_ms_tag (layer_name);
      disposal = parse_disposal_tag (animation, layer_name);
      g_free (layer_name);

      /* Frame duration. */
      priv->durations[i] = duration;

      /* Frame disposal. */
      if (i > 0 && disposal == DISPOSE_COMBINE)
        {
          previous_frame = gimp_layer_copy (previous_frame);
          gimp_image_insert_layer (priv->frames, previous_frame, 0, 0);
          gimp_item_set_visible (previous_frame, TRUE);
        }

      new_frame = gimp_layer_new_from_drawable (layers[num_layers - (i + 1)],
                                                priv->frames);
      gimp_image_insert_layer (priv->frames, new_frame, 0, 0);
      gimp_layer_scale (new_frame,
                        (gimp_drawable_width (layers[num_layers - (i + 1)]) * (gint) priv->preview_width) / image_width,
                        (gimp_drawable_height (layers[num_layers - (i + 1)]) * (gint) priv->preview_height) / image_height,
                        FALSE);
      gimp_drawable_offsets (layers[num_layers - (i + 1)], &layer_offx, &layer_offy);
      gimp_layer_set_offsets (new_frame,
                              (layer_offx * (gint) priv->preview_width) / image_width,
                              (layer_offy * (gint) priv->preview_height) / image_height);
      gimp_layer_resize_to_image_size (new_frame);
      gimp_item_set_visible (new_frame, TRUE);
      new_frame = gimp_image_merge_visible_layers (priv->frames, GIMP_CLIP_TO_IMAGE);
      gimp_item_set_visible (new_frame, FALSE);

      previous_frame = new_frame;
    }
  g_free (layers);
}

static GeglBuffer *
animation_legacy_get_frame (Animation *animation,
                            gint       pos)
{
  AnimationLegacyPrivate *priv = GET_PRIVATE (animation);
  GeglBuffer             *buffer = NULL;

  if (priv->frames)
    {
      gint32 *layers;
      gint32  num_layers;

      layers = gimp_image_get_layers (priv->frames, &num_layers);

      if (num_layers > 0 &&
          pos >= 1 && pos <= num_layers)
        {
          buffer = gimp_drawable_get_buffer (layers[num_layers - pos]);
        }

      g_free (layers);
    }

  return buffer;
}

static gint
animation_legacy_time_to_next (Animation *animation)
{
  return GET_PRIVATE (animation)->durations[animation_get_position (animation)];
}

/****** TAG UTILS ******/

static gint
parse_ms_tag (const gchar *str)
{
  if (str != NULL)
    {
      gint length = strlen (str);
      gint i;

      for (i = 0; i < length; i++)
        {
          gint rtn;
          gint dummy;

          if (is_ms_tag (&str[i], &rtn, &dummy))
            return rtn;
        }
    }

  /* -1 means using whatever is the default framerate. */
  return -1;
}

static DisposeType
parse_disposal_tag (Animation   *animation,
                    const gchar *str)
{
  AnimationLegacyPrivate *priv = GET_PRIVATE (animation);

  if (str != NULL)
    {
      gint length = strlen (str);
      gint i;

      for (i = 0; i < length; i++)
        {
          DisposeType rtn;
          gint        dummy;

          if (is_disposal_tag (&str[i], &rtn, &dummy))
            return rtn;
        }
    }

  return priv->disposal;
}

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
