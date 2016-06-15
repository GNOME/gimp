/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * animation-layer_view.c
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
#include <libgimp/gimpui.h>

#include "libgimp/stdplugins-intl.h"

#include "core/animationanimatic.h"

#include "animation-storyboard.h"

/* Properties. */
enum
{
  PROP_0,
  PROP_ANIMATION
};

struct _AnimationStoryboardPrivate
{
  AnimationAnimatic *animation;
};

/* GObject handlers */
static void animation_storyboard_constructed           (GObject             *object);
static void animation_storyboard_set_property          (GObject             *object,
                                                        guint                property_id,
                                                        const GValue        *value,
                                                        GParamSpec          *pspec);
static void animation_storyboard_get_property          (GObject             *object,
                                                        guint                property_id,
                                                        GValue              *value,
                                                        GParamSpec          *pspec);
static void animation_storyboard_finalize              (GObject             *object);

/* Callbacks on animation */
static void animation_storyboard_load                  (Animation           *animation,
                                                        G_GNUC_UNUSED gint   first_frame,
                                                        G_GNUC_UNUSED gint   num_frames,
                                                        G_GNUC_UNUSED gint   playback_start,
                                                        G_GNUC_UNUSED gint   playback_stop,
                                                        G_GNUC_UNUSED gint   preview_width,
                                                        G_GNUC_UNUSED gint   preview_height,
                                                        AnimationStoryboard *view);

static void animation_storyboard_duration_spin_changed (GtkSpinButton       *spinbutton,
                                                        AnimationAnimatic   *animation);

static void animation_storyboard_comment_changed       (GtkTextBuffer       *text_buffer,
                                                        AnimationAnimatic   *animatic);

G_DEFINE_TYPE (AnimationStoryboard, animation_storyboard, GTK_TYPE_TABLE)

#define parent_class animation_storyboard_parent_class

static void
animation_storyboard_class_init (AnimationStoryboardClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->constructed  = animation_storyboard_constructed;
  object_class->finalize     = animation_storyboard_finalize;
  object_class->get_property = animation_storyboard_get_property;
  object_class->set_property = animation_storyboard_set_property;

  /**
   * AnimationStoryboard:animation:
   *
   * The associated #AnimationAnimatic.
   */
  g_object_class_install_property (object_class, PROP_ANIMATION,
                                   g_param_spec_object ("animation",
                                                        NULL, NULL,
                                                        ANIMATION_TYPE_ANIMATIC,
                                                        G_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT_ONLY));

  g_type_class_add_private (klass, sizeof (AnimationStoryboardPrivate));
}

static void
animation_storyboard_init (AnimationStoryboard *view)
{
  view->priv = G_TYPE_INSTANCE_GET_PRIVATE (view,
                                            ANIMATION_TYPE_STORYBOARD,
                                            AnimationStoryboardPrivate);

  gtk_table_set_homogeneous (GTK_TABLE (view), TRUE);
}

/**** Public Functions ****/

/**
 * animation_storyboard_new:
 * @animation: the #AnimationAnimatic for this storyboard.
 *
 * Creates a new layer view tied to @animation, ready to be displayed.
 */
GtkWidget *
animation_storyboard_new (AnimationAnimatic *animation)
{
  GtkWidget *layer_view;

  layer_view = g_object_new (ANIMATION_TYPE_STORYBOARD,
                             "animation", animation,
                             NULL);
  return layer_view;
}

/**** Private Functions ****/

static void
animation_storyboard_constructed (GObject *object)
{
  AnimationStoryboard *view = ANIMATION_STORYBOARD (object);

  g_signal_connect (view->priv->animation, "loaded",
                    (GCallback) animation_storyboard_load,
                    view);
}

static void
animation_storyboard_set_property (GObject      *object,
                                        guint         property_id,
                                        const GValue *value,
                                        GParamSpec   *pspec)
{
  AnimationStoryboard *view = ANIMATION_STORYBOARD (object);

  switch (property_id)
    {
    case PROP_ANIMATION:
      view->priv->animation = g_value_dup_object (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
animation_storyboard_get_property (GObject    *object,
                                        guint       property_id,
                                        GValue     *value,
                                        GParamSpec *pspec)
{
  AnimationStoryboard *view = ANIMATION_STORYBOARD (object);

  switch (property_id)
    {
    case PROP_ANIMATION:
      g_value_set_object (value, view->priv->animation);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
animation_storyboard_finalize (GObject *object)
{
  AnimationStoryboard *view = ANIMATION_STORYBOARD (object);

  g_object_unref (view->priv->animation);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

/* animation_storyboard_load:
 * @view: the #AnimationStoryboard.
 *
 * Recursively fills @store with the #GimpLayers data of the #GimpImage
 * tied to @view.
 */
static void
animation_storyboard_load (Animation           *animation,
                           gint                 first_frame,
                           gint                 num_frames,
                           gint                 playback_start,
                           gint                 playback_stop,
                           gint                 preview_width,
                           gint                 preview_height,
                           AnimationStoryboard *view)
{
  gint   *layers;
  gint32  image_id;
  gint    n_images;
  gint    i;

  image_id = animation_get_image_id (animation);

  /* Cleaning previous loads. */
  gtk_container_foreach (GTK_CONTAINER (view),
                         (GtkCallback) gtk_widget_destroy,
                         NULL);

  /* Setting new values. */
  layers = gimp_image_get_layers (image_id,
                                  &n_images);

  gtk_table_resize (GTK_TABLE (view),
                    5 * n_images,
                    10);
  for (i = 0; i < n_images; i++)
    {
      GdkPixbuf *thumbnail;
      GtkWidget *image;
      GtkWidget *comment;
      GtkWidget *duration;
      GtkWidget *disposal;
      gchar     *image_name;
      gint       panel_num = n_images - i;

      thumbnail = gimp_drawable_get_thumbnail (layers[i], 250, 250,
                                               GIMP_PIXBUF_SMALL_CHECKS);
      image = gtk_image_new_from_pixbuf (thumbnail);
      g_object_unref (thumbnail);

      /* Let's align top-right, in case the storyboard gets resized
       * and the image grows (the thumbnail right now stays as fixed size). */
      gtk_misc_set_alignment (GTK_MISC (image), 1.0, 0.0);

      gtk_table_attach (GTK_TABLE (view),
                        image, 0, 4,
                        5 * n_images - 5 * i - 5,
                        5 * n_images - 5 * i,
                        GTK_EXPAND | GTK_FILL,
                        GTK_EXPAND | GTK_FILL,
                        1, 1);
      gtk_widget_show (image);

      comment = gtk_text_view_new ();
      gtk_table_attach (GTK_TABLE (view),
                        comment, 5, 10,
                        5 * n_images - 5 * i - 5,
                        5 * n_images - 5 * i,
                        GTK_EXPAND | GTK_FILL,
                        GTK_EXPAND | GTK_FILL,
                        0, 1);
      gtk_widget_show (comment);

      image_name = gimp_item_get_name (layers[i]);
      if (image_name)
        {
          GtkTextBuffer *buffer;
          const gchar   *comment_contents;

          /* Layer name is shown as a tooltip. */
          gtk_widget_set_tooltip_text (image, image_name);

          buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (comment));

          comment_contents = animation_animatic_get_comment (view->priv->animation,
                                                             panel_num);
          if (comment_contents != NULL)
            gtk_text_buffer_insert_at_cursor (buffer, comment_contents, -1);

          g_object_set_data (G_OBJECT (buffer), "panel-num",
                             GINT_TO_POINTER (panel_num));
          g_signal_connect (buffer, "changed",
                            (GCallback) animation_storyboard_comment_changed,
                            animation);

          g_free (image_name);
        }

      duration = gtk_spin_button_new_with_range (0.0, G_MAXINT, 1.0);
      gtk_spin_button_set_digits (GTK_SPIN_BUTTON (duration), 0);
      gtk_spin_button_set_increments (GTK_SPIN_BUTTON (duration), 1.0, 10.0);
      gtk_spin_button_set_snap_to_ticks (GTK_SPIN_BUTTON (duration), TRUE);
      gtk_spin_button_set_value (GTK_SPIN_BUTTON (duration),
                                 animation_animatic_get_duration (ANIMATION_ANIMATIC (animation),
                                                                  panel_num));
      gtk_entry_set_width_chars (GTK_ENTRY (duration), 2);
      /* Allowing non-numeric text to type "ms" or "s". */
      gtk_spin_button_set_numeric (GTK_SPIN_BUTTON (duration), FALSE);

      gtk_table_attach (GTK_TABLE (view),
                        duration, 4, 5,
                        5 * n_images - 5 * i - 5,
                        5 * n_images - 5 * i - 4,
                        0, /* Do not expand nor fill, nor shrink. */
                        0, /* Do not expand nor fill, nor shrink. */
                        0, 1);
      g_object_set_data (G_OBJECT (duration), "panel-num",
                         GINT_TO_POINTER (panel_num));
      g_signal_connect (duration, "value-changed",
                        (GCallback) animation_storyboard_duration_spin_changed,
                        animation);
      gtk_widget_show (duration);

      disposal = gtk_toggle_button_new ();
      image = gtk_image_new_from_icon_name (GIMP_STOCK_TRANSPARENCY,
                                            GTK_ICON_SIZE_MENU);
      gtk_container_add (GTK_CONTAINER (disposal), image);
      gtk_widget_show (image);
      gtk_table_attach (GTK_TABLE (view),
                        disposal, 4, 5,
                        5 * n_images - 5 * i - 3,
                        5 * n_images - 5 * i - 2,
                        GTK_EXPAND, GTK_EXPAND,
                        0, 1);
      gtk_widget_show (disposal);
    }
}

static void
animation_storyboard_duration_spin_changed (GtkSpinButton     *spinbutton,
                                            AnimationAnimatic *animation)
{
  gpointer panel_num;
  gint     duration;

  panel_num = g_object_get_data (G_OBJECT (spinbutton), "panel-num");
  duration = gtk_spin_button_get_value_as_int (spinbutton);

  animation_animatic_set_duration (animation,
                                   GPOINTER_TO_INT (panel_num),
                                   duration);
}

static void
animation_storyboard_comment_changed (GtkTextBuffer     *text_buffer,
                                      AnimationAnimatic *animatic)
{
  gchar       *text;
  GtkTextIter  start;
  GtkTextIter  end;
  gpointer     panel_num;

  panel_num = g_object_get_data (G_OBJECT (text_buffer), "panel-num");

  gtk_text_buffer_get_bounds (text_buffer, &start, &end);
  text = gtk_text_buffer_get_text (text_buffer, &start, &end, FALSE);
  animation_animatic_set_comment (animatic,
                                  GPOINTER_TO_INT (panel_num),
                                  text);
  g_free (text);
}
