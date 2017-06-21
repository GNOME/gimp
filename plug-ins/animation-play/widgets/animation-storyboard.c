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

#include "gdk/gdkkeysyms.h"

#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

#include "libgimp/stdplugins-intl.h"

#include "animation-utils.h"

#include "core/animation-animatic.h"
#include "core/animation-playback.h"

#include "animation-storyboard.h"

/* Properties. */
enum
{
  PROP_0,
  PROP_ANIMATION
};

struct _AnimationStoryboardPrivate
{
  AnimationAnimatic  *animation;
  AnimationPlayback  *playback;

  gint                current_panel;

  GList              *panel_buttons;
  GList              *disposal_buttons;
  GList              *comments;

  gint                dragged_panel;
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
                                                        AnimationStoryboard *view);
static void animation_storyboard_stopped               (AnimationPlayback   *playback,
                                                        AnimationStoryboard *view);
static void animation_storyboard_rendered              (AnimationPlayback   *playback,
                                                        gint                 frame_number,
                                                        GeglBuffer          *buffer,
                                                        gboolean             must_draw_null,
                                                        AnimationStoryboard *view);

static void animation_storyboard_duration_spin_changed (GtkSpinButton       *spinbutton,
                                                        AnimationStoryboard *storyboard);

static gboolean animation_storyboard_comment_keypress  (GtkWidget           *entry,
                                                        GdkEventKey         *event,
                                                        AnimationStoryboard *view);
static void animation_storyboard_comment_changed       (GtkTextBuffer       *text_buffer,
                                                        AnimationAnimatic   *animatic);
static void animation_storyboard_disposal_toggled      (GtkToggleButton     *button,
                                                        AnimationAnimatic   *animatic);
static void animation_storyboard_button_clicked        (GtkWidget           *widget,
                                                        AnimationStoryboard *storyboard);

/* Drag and drop */
static void animation_storyboard_panel_drag_begin      (GtkWidget           *widget,
                                                        GdkDragContext      *drag_context,
                                                        AnimationStoryboard *storyboard);
static void animation_storyboard_panel_drag_end        (GtkWidget           *widget,
                                                        GdkDragContext      *drag_context,
                                                        AnimationStoryboard *storyboard);
static gboolean animation_storyboard_panel_drag_drop   (GtkWidget           *widget,
                                                        GdkDragContext      *drag_context,
                                                        gint                 x,
                                                        gint                 y,
                                                        guint                time,
                                                        AnimationStoryboard *storyboard);

/* Utils */
static void animation_storyboard_jump                  (AnimationStoryboard *view,
                                                        gint                 panel);
static void animation_storyboard_move                  (AnimationStoryboard *storyboard,
                                                        gint                 from_panel,
                                                        gint                 to_panel);

G_DEFINE_TYPE (AnimationStoryboard, animation_storyboard, GTK_TYPE_SCROLLED_WINDOW)

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
  view->priv->dragged_panel = -1;
}

/**** Public Functions ****/

/**
 * animation_storyboard_new:
 * @animation: the #AnimationAnimatic for this storyboard.
 *
 * Creates a new layer view tied to @animation, ready to be displayed.
 */
GtkWidget *
animation_storyboard_new (AnimationAnimatic *animation,
                          AnimationPlayback *playback)
{
  GtkWidget           *layer_view;
  AnimationStoryboard *storyboard;

  layer_view = g_object_new (ANIMATION_TYPE_STORYBOARD,
                             "animation", animation,
                             NULL);
  storyboard = ANIMATION_STORYBOARD (layer_view);
  storyboard->priv->playback = playback;

  return layer_view;
}

/**** Private Functions ****/

static void
animation_storyboard_constructed (GObject *object)
{
  AnimationStoryboard *view = ANIMATION_STORYBOARD (object);
  GtkWidget           *layout;

  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (view),
                                  GTK_POLICY_AUTOMATIC,
                                  GTK_POLICY_AUTOMATIC);

  layout = gtk_table_new (1, 1, FALSE);
  gtk_scrolled_window_add_with_viewport (GTK_SCROLLED_WINDOW (view),
                                         layout);
  g_signal_connect (view->priv->animation, "loaded",
                    (GCallback) animation_storyboard_load,
                    view);
  gtk_widget_show (layout);
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

  g_signal_handlers_disconnect_by_func (view->priv->playback,
                                        (GCallback) animation_storyboard_rendered,
                                        view);
  g_signal_handlers_disconnect_by_func (view->priv->playback,
                                        (GCallback) animation_storyboard_stopped,
                                        view);
  g_object_unref (view->priv->animation);
  if (view->priv->panel_buttons)
    {
      g_list_free (view->priv->panel_buttons);
    }
  if (view->priv->disposal_buttons)
    {
      g_list_free (view->priv->disposal_buttons);
    }
  if (view->priv->comments)
    {
      g_list_free (view->priv->comments);
    }

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static const GtkTargetEntry target_table[] = {
      { "application/x-gimp-animation-panel",
        GTK_TARGET_SAME_APP,
        ANIMATION_DND_TYPE_PANEL }
};

/* animation_storyboard_load:
 * @view: the #AnimationStoryboard.
 *
 * Recursively fills @store with the #GimpLayers data of the #GimpImage
 * tied to @view.
 */
static void
animation_storyboard_load (Animation           *animation,
                           AnimationStoryboard *view)
{
  AnimationAnimatic *animatic = ANIMATION_ANIMATIC (animation);
  GtkWidget         *layout;
  gint              *orig_layers;
  gint              *layers;
  gint32             orig_image_id;
  gint32             image_id;
  gint               n_images;
  gint               i;

  orig_image_id = animation_get_image_id (animation);
  image_id = gimp_image_duplicate (orig_image_id);
  gimp_image_undo_disable (image_id);

  /* The actual layout is the grand-child. */
  layout = gtk_bin_get_child (GTK_BIN (view));
  layout = gtk_bin_get_child (GTK_BIN (layout));

  /* Cleaning previous loads. */
  gtk_container_foreach (GTK_CONTAINER (layout),
                         (GtkCallback) gtk_widget_destroy,
                         NULL);
  if (view->priv->panel_buttons)
    {
      g_list_free (view->priv->panel_buttons);
      view->priv->panel_buttons = NULL;
    }
  if (view->priv->disposal_buttons)
    {
      g_list_free (view->priv->disposal_buttons);
      view->priv->disposal_buttons = NULL;
    }
  if (view->priv->comments)
    {
      g_list_free (view->priv->comments);
      view->priv->comments = NULL;
    }

  /* Setting new values. */
  orig_layers = gimp_image_get_layers (orig_image_id,
                                       &n_images);
  layers = gimp_image_get_layers (image_id,
                                  &n_images);

  gtk_table_resize (GTK_TABLE (layout),
                    5 * n_images,
                    9);

  for (i = 0; i < n_images; i++)
    {
      GdkPixbuf *thumbnail;
      GtkWidget *panel_button;
      GtkWidget *image;
      GtkWidget *comment;
      GtkWidget *duration;
      GtkWidget *disposal;
      gchar     *image_name;
      gint       panel_num = n_images - i - 1;

      panel_button = gtk_button_new ();
      gtk_button_set_alignment (GTK_BUTTON (panel_button),
                                0.5, 0.5);
      gtk_button_set_relief (GTK_BUTTON (panel_button),
                             GTK_RELIEF_NONE);
      gtk_table_attach (GTK_TABLE (layout),
                        panel_button, 0, 4,
                        5 * panel_num,
                        5 * (panel_num + 1),
                        GTK_EXPAND | GTK_FILL,
                        GTK_FILL,
                        1, 1);
      g_object_set_data (G_OBJECT (panel_button), "layer-tattoo",
                         GINT_TO_POINTER (gimp_item_get_tattoo (orig_layers[i])));
      g_object_set_data (G_OBJECT (panel_button), "panel-num",
                         GINT_TO_POINTER (panel_num));
      g_signal_connect (panel_button, "clicked",
                        G_CALLBACK (animation_storyboard_button_clicked),
                        view);

      view->priv->panel_buttons = g_list_prepend (view->priv->panel_buttons,
                                                  panel_button);
      gtk_widget_show (panel_button);

      gimp_layer_resize_to_image_size (layers[i]);
      thumbnail = gimp_drawable_get_thumbnail (layers[i], 250, 250,
                                               GIMP_PIXBUF_SMALL_CHECKS);
      image = gtk_image_new_from_pixbuf (thumbnail);

      /* Make this button a drag source. */
      gtk_drag_source_set (panel_button, GDK_BUTTON1_MASK,
                           target_table, G_N_ELEMENTS (target_table),
                           GDK_ACTION_MOVE);
      gtk_drag_source_set_icon_pixbuf (panel_button, thumbnail);

      g_object_unref (thumbnail);

      gtk_drag_dest_set (panel_button, GTK_DEST_DEFAULT_ALL,
                         target_table, G_N_ELEMENTS (target_table),
                         GDK_ACTION_MOVE);

      g_signal_connect (panel_button, "drag-begin",
                        G_CALLBACK (animation_storyboard_panel_drag_begin),
                        view);
      g_signal_connect (panel_button, "drag-end",
                        G_CALLBACK (animation_storyboard_panel_drag_end),
                        view);
      g_signal_connect (panel_button, "drag-drop",
                        G_CALLBACK (animation_storyboard_panel_drag_drop),
                        view);

      /* Let's align top-right, in case the storyboard gets resized
       * and the image grows (the thumbnail right now stays as fixed size). */
      gtk_misc_set_alignment (GTK_MISC (image), 1.0, 0.0);

      gtk_container_add (GTK_CONTAINER (panel_button), image);
      gtk_widget_show (image);

      comment = gtk_text_view_new ();
      g_object_set_data (G_OBJECT (comment), "panel-num",
                         GINT_TO_POINTER (panel_num));
      gtk_table_attach (GTK_TABLE (layout),
                        comment, 5, 9,
                        5 * panel_num,
                        5 * (panel_num + 1),
                        GTK_EXPAND | GTK_FILL,
                        GTK_FILL,
                        0, 1);
      view->priv->comments = g_list_prepend (view->priv->comments,
                                             comment);
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
          g_signal_connect (comment, "key-press-event",
                            G_CALLBACK (animation_storyboard_comment_keypress),
                            view);
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
                                 animation_animatic_get_panel_duration (animatic,
                                                                        panel_num));
      gtk_entry_set_width_chars (GTK_ENTRY (duration), 2);
      /* Allowing non-numeric text to type "ms" or "s". */
      gtk_spin_button_set_numeric (GTK_SPIN_BUTTON (duration), FALSE);

      gtk_table_attach (GTK_TABLE (layout),
                        duration, 4, 5,
                        5 * panel_num + 1,
                        5 * panel_num + 2,
                        0, /* Do not expand nor fill, nor shrink. */
                        0, /* Do not expand nor fill, nor shrink. */
                        0, 1);
      g_object_set_data (G_OBJECT (duration), "panel-num",
                         GINT_TO_POINTER (panel_num));
      g_signal_connect (duration, "value-changed",
                        (GCallback) animation_storyboard_duration_spin_changed,
                        view);
      gtk_widget_show (duration);

      disposal = gtk_toggle_button_new ();
      gtk_button_set_relief (GTK_BUTTON (disposal), GTK_RELIEF_NONE);
      g_object_set_data (G_OBJECT (disposal), "panel-num",
                         GINT_TO_POINTER (panel_num));
      image = gtk_image_new_from_icon_name (GIMP_ICON_TRANSPARENCY,
                                            GTK_ICON_SIZE_MENU);
      gtk_container_add (GTK_CONTAINER (disposal), image);
      gtk_widget_show (image);
      gtk_table_attach (GTK_TABLE (layout),
                        disposal, 4, 5,
                        5 * panel_num + 2,
                        5 * panel_num + 3,
                        GTK_EXPAND, GTK_EXPAND,
                        0, 1);
      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (disposal),
                                    animation_animatic_get_combine (animatic,
                                                                    panel_num));
      g_signal_connect (disposal, "toggled",
                        G_CALLBACK (animation_storyboard_disposal_toggled),
                        animation);
      view->priv->disposal_buttons = g_list_prepend (view->priv->disposal_buttons,
                                                    disposal);
      gtk_widget_show (disposal);
    }
  g_signal_connect (view->priv->playback, "render",
                    (GCallback) animation_storyboard_rendered,
                    view);
  g_signal_connect (view->priv->playback, "stop",
                    (GCallback) animation_storyboard_stopped,
                    view);
  g_free (layers);
  g_free (orig_layers);
  gimp_image_delete (image_id);
}

static void
animation_storyboard_stopped (AnimationPlayback   *playback,
                              AnimationStoryboard *view)
{
  gint position;
  gint panel;

  position = animation_playback_get_position (playback);
  panel = animation_animatic_get_panel (view->priv->animation,
                                        position);
  animation_storyboard_jump (view, panel);
}

static void
animation_storyboard_rendered (AnimationPlayback   *playback,
                               gint                 frame_number,
                               GeglBuffer          *buffer,
                               gboolean             must_draw_null,
                               AnimationStoryboard *view)
{
  gint panel;

  panel = animation_animatic_get_panel (view->priv->animation,
                                        frame_number);
  animation_storyboard_jump (view, panel);
}

static void
animation_storyboard_duration_spin_changed (GtkSpinButton       *spinbutton,
                                            AnimationStoryboard *storyboard)
{
  gpointer panel_num;
  gint     duration;
  gint     panel_position;
  gint     position;

  panel_num = g_object_get_data (G_OBJECT (spinbutton), "panel-num");
  duration = gtk_spin_button_get_value_as_int (spinbutton);

  position = animation_playback_get_position (storyboard->priv->playback);
  panel_position = animation_animatic_get_position (storyboard->priv->animation,
                                                    GPOINTER_TO_INT (panel_num));
  if (position >= panel_position)
    {
      gint cur_duration;

      cur_duration = animation_animatic_get_panel_duration (storyboard->priv->animation,
                                                            GPOINTER_TO_INT (panel_num));
      position += duration - cur_duration;
    }

  animation_animatic_set_panel_duration (storyboard->priv->animation,
                                         GPOINTER_TO_INT (panel_num),
                                         duration);
  animation_playback_jump (storyboard->priv->playback, position);
}

static gboolean
animation_storyboard_comment_keypress (GtkWidget           *entry,
                                       GdkEventKey         *event,
                                       AnimationStoryboard *view)
{
  gpointer panel_num;

  panel_num = g_object_get_data (G_OBJECT (entry), "panel-num");

  if (event->keyval == GDK_KEY_Tab    ||
      event->keyval == GDK_KEY_KP_Tab ||
      event->keyval == GDK_KEY_ISO_Left_Tab)
    {
      GtkWidget *comment;

      comment = g_list_nth_data (view->priv->comments,
                                 GPOINTER_TO_INT (panel_num) + 1);
      if (comment)
        {
          /* Grab the next comment widget. */
          gtk_widget_grab_focus (comment);
        }
      else
        {
          /* Loop to the first comment after the last. */
          gtk_widget_grab_focus (view->priv->comments->data);
        }
      return TRUE;
    }
  return FALSE;
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

static void
animation_storyboard_disposal_toggled (GtkToggleButton   *button,
                                       AnimationAnimatic *animatic)
{
  gpointer panel_num;

  panel_num = g_object_get_data (G_OBJECT (button), "panel-num");

  animation_animatic_set_combine (animatic,
                                  GPOINTER_TO_INT (panel_num),
                                  gtk_toggle_button_get_active (button));
}

static void
animation_storyboard_button_clicked (GtkWidget           *widget,
                                     AnimationStoryboard *storyboard)
{
  gpointer panel_num;
  gint     position;

  panel_num = g_object_get_data (G_OBJECT (widget), "panel-num");
  position = animation_animatic_get_position (storyboard->priv->animation,
                                              GPOINTER_TO_INT (panel_num));
  animation_playback_jump (storyboard->priv->playback, position);
}

/**** Drag and drop ****/

static void
animation_storyboard_panel_drag_begin (GtkWidget           *widget,
                                       GdkDragContext      *drag_context,
                                       AnimationStoryboard *storyboard)
{
  gpointer panel_num;

  panel_num = g_object_get_data (G_OBJECT (widget), "panel-num");
  storyboard->priv->dragged_panel = GPOINTER_TO_INT (panel_num);
}

static void
animation_storyboard_panel_drag_end (GtkWidget           *widget,
                                     GdkDragContext      *drag_context,
                                     AnimationStoryboard *storyboard)
{
  storyboard->priv->dragged_panel = -1;
}

static gboolean
animation_storyboard_panel_drag_drop (GtkWidget           *widget,
                                      GdkDragContext      *context,
                                      gint                 x,
                                      gint                 y,
                                      guint                time,
                                      AnimationStoryboard *storyboard)
{
  gpointer       panel_num;
  GtkAllocation  allocation;
  gint           panel_dest;

  g_return_val_if_fail (storyboard->priv->dragged_panel >= 0, FALSE);

  panel_num = g_object_get_data (G_OBJECT (widget), "panel-num");
  gtk_widget_get_allocation (widget, &allocation);
  if (y > allocation.height / 2)
    {
      panel_dest = GPOINTER_TO_INT (panel_num) + 1;
    }
  else
    {
      panel_dest = GPOINTER_TO_INT (panel_num);
    }
  if (storyboard->priv->dragged_panel < panel_dest)
    {
      panel_dest--;
    }
  animation_storyboard_move (storyboard,
                             storyboard->priv->dragged_panel,
                             panel_dest);

  gtk_drag_finish (context, TRUE,
                   gdk_drag_context_get_selected_action (context) == GDK_ACTION_MOVE,
                   time);
  return TRUE;
}

/**** Utils ****/

static void
animation_storyboard_jump (AnimationStoryboard *view,
                           gint                 panel)
{
  /* Don't jump while playing. This is too disturbing. */
  if (! animation_playback_is_playing (view->priv->playback))
    {
      GtkWidget *button;

      if (view->priv->current_panel >= 0)
        {
          button = g_list_nth_data (view->priv->panel_buttons,
                                    view->priv->current_panel);
          gtk_button_set_relief (GTK_BUTTON (button),
                                 GTK_RELIEF_NONE);
        }

      view->priv->current_panel = panel;
      button = g_list_nth_data (view->priv->panel_buttons,
                                view->priv->current_panel);
      gtk_button_set_relief (GTK_BUTTON (button),
                             GTK_RELIEF_NORMAL);
      show_scrolled_child (GTK_SCROLLED_WINDOW (view), button);
    }
}

static void
animation_storyboard_move (AnimationStoryboard *storyboard,
                           gint                 from_panel,
                           gint                 to_panel)
{
  Animation *animation;
  gint32     image_id;

  animation = ANIMATION (storyboard->priv->animation);
  image_id  = animation_get_image_id (animation);
  if (from_panel != to_panel)
    {
      GtkWidget *button;
      gpointer   tattoo;
      gint32     layer;
      gint       new_position;

      button = g_list_nth_data (storyboard->priv->panel_buttons,
                                from_panel);
      tattoo = g_object_get_data (G_OBJECT (button), "layer-tattoo");
      layer = gimp_image_get_layer_by_tattoo (image_id,
                                              GPOINTER_TO_INT (tattoo));

      /* Layers are ordered from top to bottom in GIMP. */
      new_position = g_list_length (storyboard->priv->panel_buttons) - to_panel - 1;
      gimp_image_reorder_item (image_id, layer, 0, new_position);

      /* For now, do a full reload. Ugly but that's a first version.
       * TODO: implement a saner, lighter reordering of the GUI.
       */
      animation_load (animation);
    }
}
