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

#include "animation-animatic.h"
#include "animation-renderer.h"

typedef enum
{
  START_STATE,
  ANIMATION_STATE,
  PLAYBACK_STATE,
  SEQUENCE_STATE,
  PANEL_STATE,
  LAYER_STATE,
  END_SEQUENCE_STATE,
  COMMENTS_STATE,
  COMMENT_STATE,
  END_STATE
} AnimationParseState;

typedef struct
{
  Animation           *animation;
  AnimationParseState  state;

  gint                 panel;
  gint                 duration;

  gint                 xml_level;
} ParseStatus;

enum
{
  PANEL_DURATION,
  LAST_SIGNAL
};

typedef struct _AnimationAnimaticPrivate AnimationAnimaticPrivate;

struct _AnimationAnimaticPrivate
{
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

static void         animation_animatic_finalize       (GObject           *object);

/* Virtual methods */

static gint         animation_animatic_get_duration   (Animation         *animation);

static gchar      * animation_animatic_get_frame_hash (Animation         *animation,
                                                       gint               position);
static GeglBuffer * animation_animatic_create_frame   (Animation         *animation,
                                                       GObject           *renderer,
                                                       gint               position,
                                                       gdouble            proxy_ratio);

static void         animation_animatic_reset_defaults (Animation         *animation);
static gchar      * animation_animatic_serialize      (Animation         *animation,
                                                       const gchar       *playback_xml);
static gboolean     animation_animatic_deserialize    (Animation         *animation,
                                                       const gchar       *xml,
                                                       GError           **error);

/* XML parsing */

static void      animation_animatic_start_element (GMarkupParseContext  *context,
                                                   const gchar          *element_name,
                                                   const gchar         **attribute_names,
                                                   const gchar         **attribute_values,
                                                   gpointer              user_data,
                                                   GError              **error);
static void      animation_animatic_end_element   (GMarkupParseContext  *context,
                                                   const gchar          *element_name,
                                                   gpointer              user_data,
                                                   GError              **error);

static void      animation_animatic_text          (GMarkupParseContext  *context,
                                                   const gchar          *text,
                                                   gsize                 text_len,
                                                   gpointer              user_data,
                                                   GError              **error);

/* Tag handling (from layer names) */

static gint      parse_ms_tag                     (Animation            *animation,
                                                   const gchar          *str);
static gboolean  parse_combine_tag                (const gchar          *str);

static gboolean  is_ms_tag                        (const gchar          *str,
                                                   gint                 *duration,
                                                   gint                 *taglength);

G_DEFINE_TYPE (AnimationAnimatic, animation_animatic, ANIMATION_TYPE_ANIMATION)

#define parent_class animation_animatic_parent_class

static guint signals[LAST_SIGNAL] = { 0 };

static void
animation_animatic_class_init (AnimationAnimaticClass *klass)
{
  GObjectClass   *object_class = G_OBJECT_CLASS (klass);
  AnimationClass *anim_class   = ANIMATION_CLASS (klass);

  /**
   * AnimationAnimatic::panel-duration:
   * @animatic: the #AnimationAnimatic.
   * @panel: the panel number (first panel is 0).
   * @duration: the new duration for @panel (in number of frames).
   *
   * The ::panel-duration will be emitted when the duration of a layer
   * changes. It can be %0 meaning that this layer should not be shown
   * in the reel.
   */
  signals[PANEL_DURATION] =
    g_signal_new ("panel-duration",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  0,
                  NULL, NULL,
                  NULL,
                  G_TYPE_NONE,
                  2,
                  G_TYPE_INT,
                  G_TYPE_INT);

  object_class->finalize     = animation_animatic_finalize;

  anim_class->get_duration   = animation_animatic_get_duration;
  anim_class->get_frame_hash = animation_animatic_get_frame_hash;
  anim_class->create_frame   = animation_animatic_create_frame;

  anim_class->reset_defaults = animation_animatic_reset_defaults;
  anim_class->serialize      = animation_animatic_serialize;
  anim_class->deserialize    = animation_animatic_deserialize;

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
  gint                      i;

  if (priv->tattoos)
    g_free (priv->tattoos);
  if (priv->durations)
    g_free (priv->durations);
  if (priv->combine)
    g_free (priv->combine);
  if (priv->comments)
    {
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
animation_animatic_set_panel_duration (AnimationAnimatic *animatic,
                                       gint               panel_num,
                                       gint               panel_duration)
{
  AnimationAnimaticPrivate *priv           = GET_PRIVATE (animatic);
  Animation                *animation      = ANIMATION (animatic);
  gint                      duration;

  g_return_if_fail (panel_duration >= 0  &&
                    panel_num >= 0 &&
                    panel_num < priv->n_panels);

  priv->durations[panel_num] = panel_duration;
  duration = animation_get_duration (animation);

  g_signal_emit (animatic, signals[PANEL_DURATION], 0,
                 panel_num, panel_duration);
  g_signal_emit_by_name (animation, "duration-changed",
                         duration);
}

gint
animation_animatic_get_panel_duration (AnimationAnimatic *animatic,
                                       gint               panel_num)
{
  AnimationAnimaticPrivate *priv = GET_PRIVATE (animatic);

  g_return_val_if_fail (panel_num >= 0 &&
                        panel_num < priv->n_panels,
                        0);

  return priv->durations[panel_num];
}

void
animation_animatic_set_comment (AnimationAnimatic *animatic,
                                gint               panel_num,
                                const gchar       *comment)
{
  AnimationAnimaticPrivate *priv = GET_PRIVATE (animatic);

  g_return_if_fail (panel_num >= 0 &&
                    panel_num < priv->n_panels);

  if (priv->comments[panel_num])
    g_free (priv->comments[panel_num]);

  priv->comments[panel_num] = g_strdup (comment);
}

const gchar *
animation_animatic_get_comment (AnimationAnimatic *animatic,
                                gint               panel_num)
{
  AnimationAnimaticPrivate *priv = GET_PRIVATE (animatic);

  g_return_val_if_fail (panel_num >= 0 &&
                        panel_num < priv->n_panels,
                        0);

  return priv->comments[panel_num];
}

void
animation_animatic_set_combine (AnimationAnimatic *animatic,
                                gint               panel_num,
                                gboolean           combine)
{
  AnimationAnimaticPrivate *priv = GET_PRIVATE (animatic);

  g_return_if_fail (panel_num >= 0 &&
                    panel_num < priv->n_panels);

  if (priv->combine[panel_num] != combine)
    {
      gint i;

      priv->combine[panel_num] = combine;

      if (animation_animatic_get_panel_duration (animatic, panel_num) > 0)
        g_signal_emit_by_name (animatic, "frames-changed",
                               animation_animatic_get_position (animatic,
                                                                panel_num),
                               animation_animatic_get_panel_duration (animatic,
                                                                      panel_num));

      /* If next panel is in "combine" mode, it must also be re-cached.
       * And so on, recursively. */
      for (i = panel_num + 1; i < priv->n_panels; i++)
        {
          if (priv->combine[i])
            {
              g_signal_emit_by_name (animatic, "frames-changed",
                                     animation_animatic_get_position (animatic, i),
                                     animation_animatic_get_panel_duration (animatic, i));
            }
          else
            {
              break;
            }
        }
    }
}

const gboolean
animation_animatic_get_combine (AnimationAnimatic *animatic,
                                gint               panel_num)
{
  AnimationAnimaticPrivate *priv = GET_PRIVATE (animatic);

  g_return_val_if_fail (panel_num >= 0 &&
                        panel_num < priv->n_panels,
                        FALSE);

  return priv->combine[panel_num];
}

gint
animation_animatic_get_panel (AnimationAnimatic *animation,
                              gint               pos)
{
  AnimationAnimaticPrivate *priv = GET_PRIVATE (animation);
  gint                      count = 0;
  gint                      i     = -1;

  if (pos >= 0       &&
      pos < animation_animatic_get_duration (ANIMATION (animation)))
    {
      for (i = 0; i < priv->n_panels; i++)
        {
          count += priv->durations[i];
          if (count > pos)
            break;
        }
    }

  if (i != -1 && i < priv->n_panels)
    return i;

  return -1;
}

gint
animation_animatic_get_position (AnimationAnimatic *animation,
                                 gint               panel)
{
  AnimationAnimaticPrivate *priv = GET_PRIVATE (animation);
  gint                      pos  = 0;
  gint                      i;

  g_return_val_if_fail (panel < priv->n_panels,
                        priv->n_panels - 1);

  for (i = 0; i < panel; i++)
    {
      pos += priv->durations[i];
    }

  return pos;
}

/**** Virtual methods ****/

static gint
animation_animatic_get_duration (Animation *animation)
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

static gchar *
animation_animatic_get_frame_hash (Animation *animation,
                                   gint       position)
{
  AnimationAnimaticPrivate *priv = GET_PRIVATE (animation);
  gchar                    *hash;
  gint                      panel;

  panel = animation_animatic_get_panel (ANIMATION_ANIMATIC (animation),
                                        position);
  hash = g_strdup_printf ("%d;", priv->tattoos[panel]);
  while (panel > 0 && priv->combine[panel--])
    {
      gchar *tmp = hash;
      hash = g_strdup_printf ("%s%d;", hash, priv->tattoos[panel]);
      g_free (tmp);
    }
  return hash;
}

static GeglBuffer *
animation_animatic_create_frame (Animation *animation,
                                 GObject   *renderer,
                                 gint       position,
                                 gdouble    proxy_ratio)
{
  AnimationAnimaticPrivate *priv = GET_PRIVATE (animation);
  GeglBuffer               *buffer;
  GeglBuffer               *buffer2;
  GeglBuffer               *backdrop = NULL;
  gint                      layer_offx;
  gint                      layer_offy;
  gint                      preview_width;
  gint                      preview_height;
  gint32                    image_id;
  gint32                    layer;
  gint                      panel;

  panel = animation_animatic_get_panel (ANIMATION_ANIMATIC (animation),
                                        position);
  image_id = animation_get_image_id (animation);
  layer = gimp_image_get_layer_by_tattoo (image_id,
                                          priv->tattoos[panel]);
  if (! layer)
    {
      g_printerr ("Warning: buffer creation of panel %d failed; "
                  "the associated layer must have been deleted.\n",
                  panel);
      return NULL;
    }

  buffer2 = gimp_drawable_get_buffer (layer);
  animation_get_size (animation, &preview_width, &preview_height);
  preview_height *= proxy_ratio;
  preview_width  *= proxy_ratio;
  gimp_drawable_offsets (layer,
                         &layer_offx, &layer_offy);

  if (panel > 0 && priv->combine[panel])
    {
      backdrop = animation_renderer_get_buffer (ANIMATION_RENDERER (renderer),
                                                panel - 1);
    }

  buffer = normal_blend (preview_width, preview_height,
                         backdrop, 1.0, 0, 0,
                         buffer2, proxy_ratio,
                         layer_offx, layer_offy);
  g_object_unref (buffer2);
  if (backdrop)
    g_object_unref (backdrop);

  return buffer;
}

static void
animation_animatic_reset_defaults (Animation *animation)
{
  AnimationAnimaticPrivate *priv = GET_PRIVATE (animation);
  gint32                   *layers;
  gint32                    image_id;
  gint                      i;

  /* Cleaning. */
  g_free (priv->tattoos);
  g_free (priv->durations);
  g_free (priv->combine);
  for (i = 0; i < priv->n_panels; i++)
    {
      g_free (priv->comments[i]);
    }
  g_free (priv->comments);

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

  for (i = 0; i < priv->n_panels; i++)
    {
      gchar    *layer_name;
      gint      duration;
      gboolean  combine;

      layer_name = gimp_item_get_name (layers[priv->n_panels - (i + 1)]);

      duration = parse_ms_tag (animation, layer_name);
      combine  = parse_combine_tag (layer_name);

      /* Frame duration. */
      priv->tattoos[i]   = gimp_item_get_tattoo (layers[priv->n_panels - (i + 1)]);
      priv->durations[i] = duration;
      priv->combine[i]   = combine;
      /* Layer names are used as default comments. */
      priv->comments[i]  = layer_name;
    }
  g_free (layers);
}

static gchar *
animation_animatic_serialize (Animation   *animation,
                              const gchar *playback_xml)
{
  AnimationAnimaticPrivate *priv = GET_PRIVATE (animation);
  gchar                    *text;
  gchar                    *tmp;
  gint                      width;
  gint                      height;
  gint                      i;

  animation_get_size (animation, &width, &height);
  text = g_strdup_printf ("<animation type=\"animatic\" framerate=\"%f\" "
                          " duration=\"%d\" width=\"%d\" height=\"%d\">"
                          "%s<sequence>",
                          animation_get_framerate (animation),
                          priv->n_panels, width, height,
                          playback_xml);
  for (i = 0; i < priv->n_panels; i++)
    {
      gchar  *panel;

      if (priv->combine[i])
        panel = g_markup_printf_escaped ("<panel duration=\"%d\" "
                                         "blend-mode=\"normal\">"
                                         "<layer id=\"%d\"/></panel>",
                                         priv->durations[i],
                                         priv->tattoos[i]);
      else
        panel = g_markup_printf_escaped ("<panel duration=\"%d\">"
                                         "<layer id=\"%d\"/></panel>",
                                         priv->durations[i],
                                         priv->tattoos[i]);

      tmp = text;
      text = g_strconcat (text, panel, NULL);
      g_free (tmp);
      g_free (panel);
    }
  tmp = text;
  text = g_strconcat (text, "</sequence><comments>", NULL);
  g_free (tmp);

  /* New loop for comments. */
  for (i = 0; i < priv->n_panels; i++)
    {
      if (priv->comments[i])
        {
          gchar *comment;

          /* Comments are for a given panel, not for a frame position. */
          comment = g_markup_printf_escaped ("<comment panel=\"%d\">%s</comment>",
                                             i,
                                             priv->comments[i]);
          tmp = text;
          text = g_strconcat (text, comment, NULL);
          g_free (tmp);
          g_free (comment);
        }
    }
  tmp = text;
  text = g_strconcat (text, "</comments></animation>", NULL);
  g_free (tmp);

  return text;
}

static gboolean
animation_animatic_deserialize (Animation    *animation,
                                const gchar  *xml,
                                GError      **error)
{
  const GMarkupParser markup_parser =
    {
      animation_animatic_start_element,
      animation_animatic_end_element,
      animation_animatic_text,
      NULL,  /*  passthrough  */
      NULL   /*  error        */
    };
  GMarkupParseContext *context;
  ParseStatus          status = { 0, };

  g_return_val_if_fail (xml != NULL && *error == NULL, FALSE);

  /* Init with a default load. */
  animation_animatic_reset_defaults (animation);

  /* Parse XML to update. */
  status.state = START_STATE;
  status.animation = animation;
  status.xml_level = 0;

  context = g_markup_parse_context_new (&markup_parser,
                                        0, &status, NULL);
  g_markup_parse_context_parse (context, xml, strlen (xml), error);
  if (*error == NULL)
    g_markup_parse_context_end_parse (context, error);
  g_markup_parse_context_free (context);

  return (*error == NULL);
}

static void
animation_animatic_start_element (GMarkupParseContext *context,
                                  const gchar         *element_name,
                                  const gchar        **attribute_names,
                                  const gchar        **attribute_values,
                                  gpointer             user_data,
                                  GError             **error)
{
  const gchar              **names  = attribute_names;
  const gchar              **values = attribute_values;
  ParseStatus               *status = (ParseStatus *) user_data;
  AnimationAnimaticPrivate  *priv   = GET_PRIVATE (status->animation);
  gboolean                   combine;

  status->xml_level++;
  switch (status->state)
    {
    case START_STATE:
      if (g_strcmp0 (element_name, "animation") != 0)
        {
          g_set_error (error, 0, 0,
                       _("Unknown animation tag: \"%s\"."),
                       element_name);
          return;
        }
      while (*names && *values)
        {
          if (strcmp (*names, "type") == 0)
            {
              if (! **values || strcmp (*values, "animatic") != 0)
                {
                  g_set_error (error, 0, 0,
                               _("Unknown animation type: \"%s\"."),
                               *values);
                  return;
                }
            }
          else if (strcmp (*names, "width") == 0 && **values)
            {
              gint width;
              gint height;

              animation_get_size (status->animation, &width, &height);
              width = (gint) g_ascii_strtoull (*values, NULL, 10);
              animation_set_size (status->animation, width, height);
            }
          else if (strcmp (*names, "height") == 0 && **values)
            {
              gint width;
              gint height;

              animation_get_size (status->animation, &width, &height);
              height = (gint) g_ascii_strtoull (*values, NULL, 10);
              animation_set_size (status->animation, width, height);
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

          names++;
          values++;
        }
      status->state = ANIMATION_STATE;
      break;
    case ANIMATION_STATE:
      if (g_strcmp0 (element_name, "sequence") == 0)
        {
          status->state = SEQUENCE_STATE;
          status->panel = -1;
        }
      else if (g_strcmp0 (element_name, "playback") == 0)
        {
          status->state = PLAYBACK_STATE;
        }
      else
        {
          g_set_error (error, 0, 0,
                       _("Unknown sequence tag: \"%s\"."),
                       element_name);
          return;
        }
      break;
    case SEQUENCE_STATE:
      if (g_strcmp0 (element_name, "panel") != 0)
        {
          g_set_error (error, 0, 0,
                       _("Unknown panel tag: \"%s\"."),
                       element_name);
          return;
        }
      status->panel++;
      combine = FALSE;
      while (*names && *values)
        {
          if (strcmp (*names, "duration") == 0 && **values)
            {
              gint duration = g_ascii_strtoll (*values, NULL, 10);

              if (duration > 0)
                priv->durations[status->panel] = duration;
            }
          else if (strcmp (*names, "blend-mode") == 0 && **values &&
                   strcmp (*values, "normal") == 0)
            {
              /* Only the "normal" blend mode is supported currently. */
              combine = TRUE;
            }

          names++;
          values++;
        }
      if (priv->combine[status->panel] != combine)
        priv->combine[status->panel] = combine;
      status->state = PANEL_STATE;
      break;
    case PLAYBACK_STATE:
      /* Leave processing to the playback. */
      break;
    case PANEL_STATE:
      if (g_strcmp0 (element_name, "layer") != 0)
        {
          g_set_error (error, 0, 0,
                       _("Unknown layer tag: \"%s\"."),
                       element_name);
          return;
        }
      status->state = LAYER_STATE;
      break;
    case LAYER_STATE:
      /* <layer> should have no child tag. */
      g_set_error (error, 0, 0,
                   _("Unknown layer tag: \"%s\"."),
                   element_name);
      return;
    case END_SEQUENCE_STATE:
      if (g_strcmp0 (element_name, "comments") != 0)
        {
          g_set_error (error, 0, 0,
                       _("Unknown comments tag: \"%s\"."),
                       element_name);
          return;
        }
      status->state = COMMENTS_STATE;
      break;
    case COMMENTS_STATE:
      if (g_strcmp0 (element_name, "comment") != 0)
        {
          g_set_error (error, 0, 0,
                       _("Unknown comment tag: \"%s\"."),
                       element_name);
          return;
        }
      status->panel = -1;
      while (*names && *values)
        {
          if (strcmp (*names, "panel") == 0 && **values)
            {
              gint panel = (gint) g_ascii_strtoll (*values, NULL, 10);

              status->panel = panel;
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
                   _("Unknown layer tag: \"%s\"."),
                   element_name);
      return;
    default:
      g_set_error (error, 0, 0,
                   _("Unknown state!"));
      break;
    }
}

static void
animation_animatic_end_element (GMarkupParseContext *context,
                                const gchar         *element_name,
                                gpointer             user_data,
                                GError             **error)
{
  ParseStatus *status = (ParseStatus *) user_data;

  status->xml_level--;

  switch (status->state)
    {
    case SEQUENCE_STATE:
    case COMMENTS_STATE:
      status->state = END_SEQUENCE_STATE;
      break;
    case PANEL_STATE:
      status->state = SEQUENCE_STATE;
      break;
    case LAYER_STATE:
      status->state = PANEL_STATE;
      break;
    case END_SEQUENCE_STATE:
    case ANIMATION_STATE:
      status->state = END_STATE;
      break;
    case PLAYBACK_STATE:
      status->state = ANIMATION_STATE;
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
animation_animatic_text (GMarkupParseContext  *context,
                         const gchar          *text,
                         gsize                 text_len,
                         gpointer              user_data,
                         GError              **error)
{
  ParseStatus *status = (ParseStatus *) user_data;
  AnimationAnimatic *animatic = ANIMATION_ANIMATIC (status->animation);

  switch (status->state)
    {
    case COMMENT_STATE:
      if (status->panel == -1)
        /* invalid comment tag. */
        break;
      /* Setting comment to a panel. */
      animation_animatic_set_comment (animatic,
                                      status->panel,
                                      text);
      status->panel = -1;
      break;
    default:
      /* Ignoring text everywhere else. */
      break;
    }
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
