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

#include "animationanimatic.h"

enum
{
  IMAGE_DURATION,
  LAST_SIGNAL
};

typedef struct _AnimationAnimaticPrivate AnimationAnimaticPrivate;

struct _AnimationAnimaticPrivate
{
  gint         preview_width;
  gint         preview_height;

  /* Panel images are associated to an unused image (used as backend
   * for GEGL buffers). */
  gint32       panels;
  /* The number of panels. */
  gint         n_panels;
  /* Layers associated to each panel. For serialization. */
  gint        *tattoos;
  /* The duration of each panel in frames. */
  gint        *durations;
  /* Whether a panel should get blended together with previous panel. */
  gboolean    *combine;
  /* Panel comments. */
  gchar      **comments;
};

#define GET_PRIVATE(animation) \
        G_TYPE_INSTANCE_GET_PRIVATE (animation, \
                                     ANIMATION_TYPE_ANIMATIC, \
                                     AnimationAnimaticPrivate)

static void         animation_animatic_finalize   (GObject           *object);

/* Virtual methods */

static gint         animation_animatic_get_length (Animation         *animation);
static void         animation_animatic_get_size   (Animation         *animation,
                                                   gint              *width,
                                                   gint              *height);

static void         animation_animatic_load       (Animation         *animation,
                                                   gdouble            proxy_ratio);
static GeglBuffer * animation_animatic_get_frame  (Animation         *animation,
                                                   gint               pos);
static gchar      * animation_animatic_serialize  (Animation         *animation);
static gboolean     animation_animatic_same       (Animation         *animation,
                                                   gint               previous_pos,
                                                   gint               next_pos);

/* Utils */

static gint         animation_animatic_get_layer  (AnimationAnimatic *animation,
                                                   gint               pos);

/* Tag handling (from layer names) */

static gint         parse_ms_tag                  (Animation         *animation,
                                                   const gchar       *str);
static gboolean     parse_combine_tag             (const gchar       *str);

static gboolean     is_ms_tag                     (const gchar       *str,
                                                   gint              *duration,
                                                   gint              *taglength);

G_DEFINE_TYPE (AnimationAnimatic, animation_animatic, ANIMATION_TYPE_ANIMATION)

#define parent_class animation_animatic_parent_class

static guint signals[LAST_SIGNAL] = { 0 };

static void
animation_animatic_class_init (AnimationAnimaticClass *klass)
{
  GObjectClass   *object_class = G_OBJECT_CLASS (klass);
  AnimationClass *anim_class   = ANIMATION_CLASS (klass);

  /**
   * AnimationAnimatic::image-duration:
   * @animatic: the #AnimationAnimatic.
   * @layer_id: the #GimpLayer id.
   * @duration: the new duration for @layer_id (in number of panels).
   *
   * The ::image-duration will be emitted when the duration of a layer
   * changes. It can be %0 meaning that this layer should not be shown
   * in the reel.
   */
  signals[IMAGE_DURATION] =
    g_signal_new ("image-duration",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  0,
                  NULL, NULL,
                  NULL,
                  G_TYPE_NONE,
                  2,
                  G_TYPE_INT,
                  G_TYPE_INT);

  object_class->finalize = animation_animatic_finalize;

  anim_class->get_length = animation_animatic_get_length;
  anim_class->get_size   = animation_animatic_get_size;
  anim_class->load       = animation_animatic_load;
  anim_class->get_frame  = animation_animatic_get_frame;
  anim_class->serialize  = animation_animatic_serialize;
  anim_class->same       = animation_animatic_same;

  g_type_class_add_private (klass, sizeof (AnimationAnimaticPrivate));
}

static void
animation_animatic_init (AnimationAnimatic *animation)
{
}

static void
animation_animatic_finalize (GObject *object)
{
  AnimationAnimaticPrivate *priv = GET_PRIVATE (object);

  gimp_image_delete (priv->panels);
  if (priv->tattoos)
    g_free (priv->tattoos);
  if (priv->durations)
    g_free (priv->durations);
  if (priv->combine)
    g_free (priv->combine);
  if (priv->comments)
    {
      gint i;

      for (i = 0; i < priv->n_panels; i++)
        {
          g_free (priv->comments[i]);
        }
      g_free (priv->comments);
    }

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

/**** Public Functions ****/

void
animation_animatic_set_duration (AnimationAnimatic *animatic,
                                 gint               panel_num,
                                 gint               duration)
{
  AnimationAnimaticPrivate *priv           = GET_PRIVATE (animatic);
  Animation                *animation      = ANIMATION (animatic);
  gint                      prev_length    = animation_get_length (animation);
  gint                      playback_start = animation_get_playback_start (animation);
  gint                      playback_stop  = animation_get_playback_stop (animation);
  gint                      position       = animation_get_position (animation);
  gint                      layer_id;
  gint                      length;

  g_return_if_fail (duration >= 0  &&
                    panel_num > 0 &&
                    panel_num <= priv->n_panels);

  layer_id = animation_animatic_get_layer (animatic, position);

  priv->durations[panel_num - 1] = duration;
  length = animation_get_length (animation);

  if (playback_start > length)
    {
      playback_start = animation_get_start_position (animation);
    }
  if (playback_stop > length ||
      playback_stop == prev_length)
    {
      playback_stop = length;
    }
  g_signal_emit (animatic, signals[IMAGE_DURATION], 0,
                 panel_num, duration);
  g_signal_emit_by_name (animatic, "playback-range",
                         playback_start, playback_stop,
                         animation_get_start_position (animation),
                         animation_get_length (animation));
  if (position > length)
    {
      animation_jump (animation, length);
    }
  else if (layer_id != animation_animatic_get_layer (animatic, position))
    {
      GeglBuffer *buffer;

      buffer = animation_get_frame (animation, position);
      g_signal_emit_by_name (animation, "render",
                             position, buffer, TRUE);
      if (buffer)
        g_object_unref (buffer);
    }
}

gint
animation_animatic_get_duration (AnimationAnimatic *animatic,
                                 gint               panel_num)
{
  AnimationAnimaticPrivate *priv = GET_PRIVATE (animatic);

  g_return_val_if_fail (panel_num > 0 &&
                        panel_num <= priv->n_panels,
                        0);

  return priv->durations[panel_num - 1];
}

void
animation_animatic_set_comment (AnimationAnimatic *animatic,
                                gint               panel_num,
                                const gchar       *comment)
{
  AnimationAnimaticPrivate *priv = GET_PRIVATE (animatic);

  g_return_if_fail (panel_num > 0 &&
                    panel_num <= priv->n_panels);

  if (priv->comments[panel_num - 1])
    g_free (priv->comments[panel_num - 1]);

  priv->comments[panel_num - 1] = g_strdup (comment);
}

const gchar *
animation_animatic_get_comment (AnimationAnimatic *animatic,
                                gint               panel_num)
{
  AnimationAnimaticPrivate *priv = GET_PRIVATE (animatic);

  g_return_val_if_fail (panel_num > 0 &&
                        panel_num <= priv->n_panels,
                        0);
  return priv->comments[panel_num - 1];
}

void
animation_animatic_set_combine (AnimationAnimatic *animatic,
                                gint               panel_num,
                                gboolean           combine)
{
  AnimationAnimaticPrivate *priv = GET_PRIVATE (animatic);

  g_return_if_fail (panel_num > 0 &&
                    panel_num <= priv->n_panels);

  priv->combine[panel_num] = combine;
}

const gboolean
animation_animatic_get_combine (AnimationAnimatic *animatic,
                                gint               panel_num)
{
  AnimationAnimaticPrivate *priv = GET_PRIVATE (animatic);

  g_return_val_if_fail (panel_num > 0 &&
                        panel_num <= priv->n_panels,
                        0);
  return priv->combine[panel_num - 1];
}

/**** Virtual methods ****/

static gint
animation_animatic_get_length (Animation *animation)
{
  AnimationAnimaticPrivate *priv = GET_PRIVATE (animation);
  gint                      count = 0;
  gint                      i ;

  for (i = 0; i < priv->n_panels; i++)
    {
      count += priv->durations[i];
    }

  return count;
}

static void
animation_animatic_get_size (Animation *animation,
                             gint      *width,
                             gint      *height)
{
  AnimationAnimaticPrivate *priv = GET_PRIVATE (animation);

  *width  = priv->preview_width;
  *height = priv->preview_height;
}

static void
animation_animatic_load (Animation *animation,
                         gdouble    proxy_ratio)
{
  AnimationAnimaticPrivate *priv = GET_PRIVATE (animation);
  gint32                   *layers;
  gint32                    image_id;
  gint32                    new_frame;
  gint32                    previous_frame = 0;
  gint                      image_width;
  gint                      image_height;
  gint                      i;

  g_return_if_fail (proxy_ratio > 0.0 && proxy_ratio <= 1.0);

  /* Cleaning. */
  if (gimp_image_is_valid (priv->panels))
    {
      gimp_image_delete (priv->panels);
      g_free (priv->tattoos);
      g_free (priv->durations);
      g_free (priv->combine);

      for (i = 0; i < priv->n_panels; i++)
        {
          g_free (priv->comments[i]);
        }
      g_free (priv->comments);
    }

  image_id = animation_get_image_id (animation);
  layers   = gimp_image_get_layers (image_id, &priv->n_panels);

  priv->tattoos   = g_try_malloc0_n (priv->n_panels, sizeof (gint));
  priv->durations = g_try_malloc0_n (priv->n_panels, sizeof (gint));
  priv->combine   = g_try_malloc0_n (priv->n_panels, sizeof (gboolean));
  priv->comments  = g_try_malloc0_n (priv->n_panels, sizeof (gchar*));
  if (! priv->tattoos || ! priv->durations ||
      ! priv->combine || ! priv->comments)
    {
      gimp_message (_("Memory could not be allocated to the animatic."));
      gimp_quit ();
      return;
    }

  /* We default at full preview size. */
  image_width  = gimp_image_width (image_id);
  image_height = gimp_image_height (image_id);

  priv->preview_width = image_width;
  priv->preview_height = image_height;

  /* Apply proxy ratio. */
  priv->preview_width  *= proxy_ratio;
  priv->preview_height *= proxy_ratio;

  priv->panels = gimp_image_new (priv->preview_width,
                                 priv->preview_width,
                                 GIMP_RGB);

  gimp_image_undo_disable (priv->panels);

  for (i = 0; i < priv->n_panels; i++)
    {
      gchar       *layer_name;
      gint         duration;
      gboolean     combine;
      gint         layer_offx;
      gint         layer_offy;

      g_signal_emit_by_name (animation, "loading",
                             (gdouble) i / ((gdouble) priv->n_panels - 0.999));

      layer_name = gimp_item_get_name (layers[priv->n_panels - (i + 1)]);

      duration = parse_ms_tag (animation, layer_name);
      combine  = parse_combine_tag (layer_name);

      /* Frame duration. */
      priv->tattoos[i]   = gimp_item_get_tattoo (layers[priv->n_panels - (i + 1)]);
      priv->durations[i] = duration;
      priv->combine[i]   = combine;
      /* Layer names are used as default comments. */
      priv->comments[i]  = layer_name;

      /* Frame disposal. */
      if (i > 0 && combine)
        {
          previous_frame = gimp_layer_copy (previous_frame);
          gimp_image_insert_layer (priv->panels, previous_frame, 0, 0);
          gimp_item_set_visible (previous_frame, TRUE);
        }

      new_frame = gimp_layer_new_from_drawable (layers[priv->n_panels - (i + 1)],
                                                priv->panels);
      gimp_image_insert_layer (priv->panels, new_frame, 0, 0);
      gimp_layer_scale (new_frame,
                        (gimp_drawable_width (layers[priv->n_panels - (i + 1)]) * (gint) priv->preview_width) / image_width,
                        (gimp_drawable_height (layers[priv->n_panels - (i + 1)]) * (gint) priv->preview_height) / image_height,
                        FALSE);
      gimp_drawable_offsets (layers[priv->n_panels - (i + 1)], &layer_offx, &layer_offy);
      gimp_layer_set_offsets (new_frame,
                              (layer_offx * (gint) priv->preview_width) / image_width,
                              (layer_offy * (gint) priv->preview_height) / image_height);
      gimp_layer_resize_to_image_size (new_frame);
      gimp_item_set_visible (new_frame, TRUE);
      new_frame = gimp_image_merge_visible_layers (priv->panels, GIMP_CLIP_TO_IMAGE);
      gimp_item_set_visible (new_frame, FALSE);

      previous_frame = new_frame;
    }
  g_free (layers);
}

static GeglBuffer *
animation_animatic_get_frame (Animation *animation,
                              gint       pos)
{
  AnimationAnimaticPrivate *priv = GET_PRIVATE (animation);
  GeglBuffer               *buffer = NULL;

  if (priv->panels)
    {
      gint32 *layers;
      gint32  num_layers;
      gint    count = 0;

      layers = gimp_image_get_layers (priv->panels, &num_layers);

      if (num_layers > 0 &&
          pos >= 1       &&
          pos <= animation_animatic_get_length (animation))
        {
          gint i ;

          for (i = 0; i < num_layers; i++)
            {
              count += priv->durations[i];
              if (count >= pos)
                break;
            }

          buffer = gimp_drawable_get_buffer (layers[num_layers - i - 1]);
        }

      g_free (layers);
    }

  return buffer;
}

static gchar *
animation_animatic_serialize (Animation *animation)
{
  AnimationAnimaticPrivate *priv = GET_PRIVATE (animation);
  gchar                    *text;
  gchar                    *tmp;
  gint                      i;

  text = g_strdup_printf ("<animation framerate=\"%f\" type=\"animatic\"><sequence>",
                          animation_get_framerate (animation));
  for (i = 0; i < priv->n_panels; i++)
    {
      gchar  *panel;
      gchar  *layer_name;
      gint32 *panels;
      gint    n_panels;

      panels = gimp_image_get_layers (priv->panels, &n_panels);
      layer_name = gimp_item_get_name (panels[n_panels - (i + 1)]);

      panel = g_markup_printf_escaped ("<panel title=\"%s\" duration=\"%d\" layer=\"%d\">",
                                       layer_name, priv->durations[i],
                                       priv->tattoos[i]);
      g_free (layer_name);

      tmp = text;
      text = g_strconcat (text, panel, NULL);
      g_free (tmp);
      g_free (panel);

      if (priv->comments[i])
        {
          gchar *comment;

          comment = g_markup_printf_escaped ("<notes title=\"Notes\">%s</notes>",
                                             priv->comments[i]);
          tmp = text;
          text = g_strconcat (text, comment, NULL);
          g_free (tmp);
          g_free (comment);
        }
      tmp = text;
      text = g_strconcat (text, "</panel>", NULL);
      g_free (tmp);
    }
  tmp = text;
  text = g_strconcat (text, "</sequence></animation>", NULL);
  g_free (tmp);

  return text;
}

static gboolean
animation_animatic_same (Animation *animation,
                         gint       previous_pos,
                         gint       next_pos)
{
  AnimationAnimaticPrivate *priv = GET_PRIVATE (animation);
  gint                      count = 0;
  gboolean                  identical = FALSE;
  gint                      i ;

  for (i = 0; i < priv->n_panels; i++)
    {
      count += priv->durations[i];
      if (count >= previous_pos && count >= next_pos)
        {
          identical = TRUE;
          break;
        }
      else if (count >= previous_pos || count >= next_pos)
        {
          identical = FALSE;
          break;
        }
    }

  return identical;
}

/**** Utils ****/

static gint
animation_animatic_get_layer (AnimationAnimatic *animation,
                              gint               pos)
{
  AnimationAnimaticPrivate *priv = GET_PRIVATE (animation);
  gint                      i = -1;

  if (priv->panels)
    {
      gint32 *layers;
      gint32  num_layers;
      gint    count = 0;

      layers = gimp_image_get_layers (priv->panels, &num_layers);

      if (num_layers > 0 &&
          pos >= 1       &&
          pos <= animation_animatic_get_length (ANIMATION (animation)))
        {
          for (i = num_layers - 1; i >= 0; i--)
            {
              count += priv->durations[i];
              if (count >= pos)
                break;
            }
        }

      g_free (layers);
    }

  return i;
}

/**** TAG UTILS ****/

static gint
parse_ms_tag (Animation   *animation,
              const gchar *str)
{
  if (str != NULL)
    {
      gint length = strlen (str);
      gint i;

      for (i = 0; i < length; i++)
        {
          gint duration;
          gint dummy;

          if (is_ms_tag (&str[i], &duration, &dummy))
            {
              gdouble fps = animation_get_framerate (animation);

              /* Get frame duration in frame numbers, not millisecond. */
              duration = (gint) ((fps * (gdouble) duration) / 1000.0);

              return duration;
            }
        }
    }

  /* Default to 6 frames per panel.
   * Storyboard-type animations are rarely detailed. */
  return 6;
}

static gboolean
parse_combine_tag (const gchar *str)
{
  gboolean combine = FALSE;

  if (str != NULL)
    {
      gint length = strlen (str);
      gint i;

      for (i = 0; i < length; i++)
        {
          if (strlen (str) != 9)
            continue;

          if (strncmp (str, "(combine)", 9) == 0)
            {
              combine = TRUE;
              break;
            }
          else if (strncmp (str, "(replace)", 9) == 0)
            {
              combine = FALSE;
              break;
            }
        }
    }

  return combine;
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
