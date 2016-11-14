/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * animation-xsheet.c
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

#include "gdk/gdkkeysyms.h"
#include <gtk/gtk.h>

#include <libgimp/gimp.h>

#include "animation-utils.h"

#include "core/animation.h"
#include "core/animation-playback.h"
#include "core/animation-celanimation.h"

#include "animation-layer-view.h"

#include "animation-xsheet.h"

#include "libgimp/stdplugins-intl.h"

enum
{
  PROP_0,
  PROP_ANIMATION,
  PROP_LAYER_VIEW
};

struct _AnimationXSheetPrivate
{
  AnimationCelAnimation *animation;
  AnimationPlayback     *playback;

  GtkWidget             *layer_view;

  GtkWidget             *track_layout;

  GtkWidget             *active_pos_button;
  GList                 *position_buttons;

  GList                 *cels;
  gint                   selected_track;
  GQueue                *selected_frames;

  GList                 *comment_fields;
  GList                 *second_separators;
};

static void    animation_xsheet_constructed  (GObject      *object);
static void    animation_xsheet_set_property (GObject      *object,
                                              guint         property_id,
                                              const GValue *value,
                                              GParamSpec   *pspec);
static void    animation_xsheet_get_property (GObject      *object,
                                              guint         property_id,
                                              GValue       *value,
                                              GParamSpec   *pspec);
static void    animation_xsheet_finalize     (GObject      *object);

/* Construction methods */
static void     animation_xsheet_add_frames    (AnimationXSheet   *xsheet,
                                                gint               position,
                                                gint               n_frames);
static void     animation_xsheet_remove_frames (AnimationXSheet   *xsheet,
                                                gint               position,
                                                gint               n_frames);
static void     animation_xsheet_reset_layout  (AnimationXSheet   *xsheet);

/* Callbacks on animation. */
static void     on_animation_loaded            (Animation         *animation,
                                                AnimationXSheet   *xsheet);
static void     on_animation_duration_changed  (Animation         *animation,
                                                gint               duration,
                                                AnimationXSheet   *xsheet);
/* Callbacks on playback. */
static void     on_animation_stopped           (AnimationPlayback *playback,
                                                AnimationXSheet   *xsheet);
static void     on_animation_rendered          (AnimationPlayback *animation,
                                                gint               frame_number,
                                                GeglBuffer        *buffer,
                                                gboolean           must_draw_null,
                                                AnimationXSheet   *xsheet);

/* Callbacks on layer view. */
static void     on_layer_selection             (AnimationLayerView *view,
                                                GList              *layers,
                                                AnimationXSheet    *xsheet);

/* UI Signals */
static gboolean animation_xsheet_frame_clicked       (GtkWidget       *button,
                                                      GdkEvent        *event,
                                                      AnimationXSheet *xsheet);
static gboolean animation_xsheet_cel_clicked         (GtkWidget       *button,
                                                      GdkEvent        *event,
                                                      AnimationXSheet *xsheet);
static void     animation_xsheet_track_title_updated (GtkEntryBuffer  *buffer,
                                                      GParamSpec      *param_spec,
                                                      AnimationXSheet *xsheet);
static void     animation_xsheet_comment_changed     (GtkTextBuffer   *text_buffer,
                                                      AnimationXSheet *xsheet);
static gboolean animation_xsheet_comment_keypress    (GtkWidget       *entry,
                                                      GdkEventKey     *event,
                                                      AnimationXSheet *xsheet);

static void     on_track_add_clicked                 (GtkToolButton   *button,
                                                      AnimationXSheet *xsheet);
static void     on_track_delete_clicked              (GtkToolButton   *button,
                                                      AnimationXSheet *xsheet);
static void     on_track_left_clicked                (GtkToolButton   *toolbutton,
                                                      AnimationXSheet *xsheet);
static void     on_track_right_clicked               (GtkToolButton   *toolbutton,
                                                      AnimationXSheet *xsheet);
/* Utils */
static void     animation_xsheet_rename_cel          (AnimationXSheet *xsheet,
                                                      GtkWidget       *cel,
                                                      gboolean         recursively);
static void     animation_xsheet_jump                (AnimationXSheet *xsheet,
                                                      gint             position);

G_DEFINE_TYPE (AnimationXSheet, animation_xsheet, GTK_TYPE_SCROLLED_WINDOW)

#define parent_class animation_xsheet_parent_class

static void
animation_xsheet_class_init (AnimationXSheetClass *klass)
{
  GObjectClass   *object_class     = G_OBJECT_CLASS (klass);

  object_class->constructed        = animation_xsheet_constructed;
  object_class->get_property       = animation_xsheet_get_property;
  object_class->set_property       = animation_xsheet_set_property;
  object_class->finalize           = animation_xsheet_finalize;

  /**
   * AnimationXSheet:animation:
   *
   * The associated #Animation.
   */
  g_object_class_install_property (object_class, PROP_ANIMATION,
                                   g_param_spec_object ("animation",
                                                        NULL, NULL,
                                                        ANIMATION_TYPE_CEL_ANIMATION,
                                                        G_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT_ONLY));
  g_object_class_install_property (object_class, PROP_LAYER_VIEW,
                                   g_param_spec_object ("layer-view",
                                                        NULL, NULL,
                                                        ANIMATION_TYPE_LAYER_VIEW,
                                                        G_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT_ONLY));

  g_type_class_add_private (klass, sizeof (AnimationXSheetPrivate));
}

static void
animation_xsheet_init (AnimationXSheet *xsheet)
{
  xsheet->priv = G_TYPE_INSTANCE_GET_PRIVATE (xsheet,
                                              ANIMATION_TYPE_XSHEET,
                                              AnimationXSheetPrivate);
  xsheet->priv->selected_frames = g_queue_new ();
}

/************ Public Functions ****************/

GtkWidget *
animation_xsheet_new (AnimationCelAnimation *animation,
                      AnimationPlayback     *playback,
                      GtkWidget             *layer_view)
{
  GtkWidget *xsheet;

  xsheet = g_object_new (ANIMATION_TYPE_XSHEET,
                         "animation", animation,
                         "layer-view", layer_view,
                         NULL);
  ANIMATION_XSHEET (xsheet)->priv->playback = playback;
  g_signal_connect (ANIMATION_XSHEET (xsheet)->priv->playback,
                    "render", G_CALLBACK (on_animation_rendered),
                    xsheet);
  g_signal_connect (ANIMATION_XSHEET (xsheet)->priv->playback,
                    "stop", G_CALLBACK (on_animation_stopped),
                    xsheet);

  return xsheet;
}

/************ Private Functions ****************/

static void
animation_xsheet_constructed (GObject *object)
{
  AnimationXSheet *xsheet = ANIMATION_XSHEET (object);

  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (xsheet),
                                  GTK_POLICY_ALWAYS, GTK_POLICY_ALWAYS);

  /* We don't know the size yet. */
  xsheet->priv->track_layout = gtk_table_new (1, 1, FALSE);

  gtk_scrolled_window_add_with_viewport (GTK_SCROLLED_WINDOW (xsheet),
                                         xsheet->priv->track_layout);
  gtk_table_set_row_spacings (GTK_TABLE (xsheet->priv->track_layout), 0);
  gtk_table_set_col_spacings (GTK_TABLE (xsheet->priv->track_layout), 0);
  gtk_widget_show (xsheet->priv->track_layout);

  /* Tie the layer view to the xsheet. */
  g_signal_connect (xsheet->priv->layer_view, "layer-selection",
                    G_CALLBACK (on_layer_selection), xsheet);

  /* Reload everything when we reload the animation. */
  g_signal_connect_after (xsheet->priv->animation, "loaded",
                          G_CALLBACK (on_animation_loaded), xsheet);

  g_signal_connect_after (xsheet->priv->animation,
                          "duration-changed",
                          G_CALLBACK (on_animation_duration_changed),
                          xsheet);
}

static void
animation_xsheet_set_property (GObject      *object,
                               guint         property_id,
                               const GValue *value,
                               GParamSpec   *pspec)
{
  AnimationXSheet *xsheet = ANIMATION_XSHEET (object);

  switch (property_id)
    {
    case PROP_ANIMATION:
      xsheet->priv->animation = g_value_dup_object (value);
      break;
    case PROP_LAYER_VIEW:
      xsheet->priv->layer_view = g_value_dup_object (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
animation_xsheet_get_property (GObject      *object,
                               guint         property_id,
                               GValue       *value,
                               GParamSpec   *pspec)
{
  AnimationXSheet *xsheet = ANIMATION_XSHEET (object);

  switch (property_id)
    {
    case PROP_ANIMATION:
      g_value_set_object (value, xsheet->priv->animation);
      break;
    case PROP_LAYER_VIEW:
      g_value_set_object (value, xsheet->priv->layer_view);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
animation_xsheet_finalize (GObject *object)
{
  AnimationXSheet *xsheet = ANIMATION_XSHEET (object);

  g_signal_handlers_disconnect_by_func (ANIMATION_XSHEET (xsheet)->priv->playback,
                                        G_CALLBACK (on_animation_rendered),
                                        xsheet);
  g_signal_handlers_disconnect_by_func (ANIMATION_XSHEET (xsheet)->priv->playback,
                                        G_CALLBACK (on_animation_stopped),
                                        xsheet);
  if (xsheet->priv->animation)
    g_object_unref (xsheet->priv->animation);
  if (xsheet->priv->layer_view)
    g_object_unref (xsheet->priv->layer_view);
  g_queue_free (xsheet->priv->selected_frames);

  g_list_free_full (xsheet->priv->cels, (GDestroyNotify) g_list_free);
  g_list_free (xsheet->priv->comment_fields);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
animation_xsheet_add_frames (AnimationXSheet *xsheet,
                             gint             position,
                             gint             n_frames)
{
  GtkWidget *frame;
  GList     *iter;
  gdouble    framerate;
  gint       duration;
  gint       n_tracks;
  gint       i;
  gint       j;

  duration = animation_get_duration (ANIMATION (xsheet->priv->animation));

  g_return_if_fail (n_frames > 0 && position >= 0 && position <= duration);

  n_tracks = animation_cel_animation_get_levels (xsheet->priv->animation);
  framerate = animation_get_framerate (ANIMATION (xsheet->priv->animation));

  for (j = 0, iter = xsheet->priv->cels; iter; iter = iter->next, j++)
    {
      /* Create new cels. */
      for (i = position; i < position + n_frames; i++)
        {
          GtkWidget *cel;

          frame = gtk_frame_new (NULL);
          gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_IN);
          gtk_table_attach (GTK_TABLE (xsheet->priv->track_layout),
                            frame, j * 9 + 1, j * 9 + 10, i + 2, i + 3,
                            GTK_FILL, GTK_FILL, 0, 0);

          cel = gtk_toggle_button_new ();
          iter->data = g_list_append (iter->data, cel);
          g_object_set_data (G_OBJECT (cel), "track-num",
                             GINT_TO_POINTER (j));
          g_object_set_data (G_OBJECT (cel), "frame-position",
                             GINT_TO_POINTER (i));
          animation_xsheet_rename_cel (xsheet, cel, FALSE);
          g_signal_connect (cel, "button-release-event",
                            G_CALLBACK (animation_xsheet_cel_clicked),
                            xsheet);
          gtk_button_set_relief (GTK_BUTTON (cel), GTK_RELIEF_NONE);
          gtk_button_set_focus_on_click (GTK_BUTTON (cel), FALSE);
          gtk_container_add (GTK_CONTAINER (frame), cel);
          gtk_widget_show (cel);
          gtk_widget_show (frame);
        }
    }

  for (i = position; i < position + n_frames; i++)
    {
      GtkWidget     *label;
      gchar         *num_str;

      GtkWidget     *comment_field;
      const gchar   *comment;
      GtkTextBuffer *text_buffer;

      /* Separator line every second (with round framerate only). */
      if (framerate == (gdouble) (gint) framerate &&
          i != 0                                  &&
          i % ((gint) framerate) == 0)
        {
          GtkWidget *align;
          GtkWidget *separator;

          align = gtk_alignment_new (0.5, 0.0, 1.0, 0.2);
          gtk_table_attach (GTK_TABLE (xsheet->priv->track_layout),
                            align, 0, n_tracks * 9 + 6, i + 2, i + 3,
                            GTK_FILL, GTK_FILL, 0, 0);
          gtk_widget_show (align);

          separator = gtk_hseparator_new ();
          gtk_container_add (GTK_CONTAINER (align), separator);
          gtk_widget_show (separator);

          xsheet->priv->second_separators = g_list_prepend (xsheet->priv->second_separators,
                                                            align);
        }

      /* Create new position buttons. */
      frame = gtk_frame_new (NULL);
      gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_ETCHED_OUT);
      gtk_table_attach (GTK_TABLE (xsheet->priv->track_layout),
                        frame, 0, 1, i + 2, i + 3,
                        GTK_FILL, GTK_FILL, 0, 0);

      num_str = g_strdup_printf ("%d", i + 1);
      label = gtk_toggle_button_new ();
      xsheet->priv->position_buttons = g_list_append (xsheet->priv->position_buttons,
                                                      label);
      gtk_button_set_label (GTK_BUTTON (label), num_str);
      g_free (num_str);
      g_object_set_data (G_OBJECT (label), "frame-position",
                         GINT_TO_POINTER (i));
      gtk_button_set_relief (GTK_BUTTON (label), GTK_RELIEF_NONE);
      gtk_button_set_focus_on_click (GTK_BUTTON (label), FALSE);
      g_signal_connect (label, "button-press-event",
                        G_CALLBACK (animation_xsheet_frame_clicked),
                        xsheet);
      gtk_container_add (GTK_CONTAINER (frame), label);

      gtk_widget_show (label);
      gtk_widget_show (frame);

      /* Create new comment fields. */
      frame = gtk_frame_new (NULL);
      gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_IN);
      gtk_table_attach (GTK_TABLE (xsheet->priv->track_layout),
                        frame, n_tracks * 9 + 1, n_tracks * 9 + 6, i + 2, i + 3,
                        GTK_FILL, GTK_FILL, 0, 0);
      comment_field = gtk_text_view_new ();
      xsheet->priv->comment_fields = g_list_append (xsheet->priv->comment_fields,
                                                    comment_field);
      text_buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (comment_field));
      comment = animation_cel_animation_get_comment (xsheet->priv->animation,
                                                     i);
      if (comment != NULL)
        gtk_text_buffer_insert_at_cursor (text_buffer, comment, -1);

      g_object_set_data (G_OBJECT (comment_field), "frame-position",
                         GINT_TO_POINTER (i));
      g_object_set_data (G_OBJECT (text_buffer), "frame-position",
                         GINT_TO_POINTER (i));
      g_signal_connect (comment_field, "key-press-event",
                        G_CALLBACK (animation_xsheet_comment_keypress),
                        xsheet);
      g_signal_connect (text_buffer, "changed",
                        (GCallback) animation_xsheet_comment_changed,
                        xsheet);
      gtk_container_add (GTK_CONTAINER (frame), comment_field);
      gtk_widget_show (comment_field);
      gtk_widget_show (frame);
    }
}

static void
animation_xsheet_remove_frames (AnimationXSheet   *xsheet,
                                gint               position,
                                gint               n_frames)
{
  GList     *item;
  GList     *iter;
  gdouble    framerate;
  gint       duration;
  gint       i;
  gint       j;

  duration = animation_get_duration (ANIMATION (xsheet->priv->animation));

  g_return_if_fail (n_frames > 0 && position >= 0 && position <= duration);

  framerate = animation_get_framerate (ANIMATION (xsheet->priv->animation));

  /* Make sure this is not a selected frame. */
  for (i = position; i < position + n_frames; i++)
    {
      g_queue_remove (xsheet->priv->selected_frames,
                      GINT_TO_POINTER (i));
    }

  /* Remove the "cel" buttons. */
  for (j = 0, iter = xsheet->priv->cels; iter; iter = iter->next, j++)
    {
      GList *track_cels;
      GList *iter2;

      track_cels = iter->data;
      item = g_list_nth (track_cels, position);

      for (iter2 = item, i = position; iter2 && i < position + n_frames; iter2 = iter2->next)
        {
          gtk_widget_destroy (gtk_widget_get_parent (iter2->data));
        }
      if (item->prev)
        item->prev->next = iter2;
      else
        iter->data = iter2;
      if (iter2)
        {
          iter2->prev->next = NULL;
          iter2->prev = item->prev;
        }
      item->prev = NULL;
      g_list_free (item);
    }

  /* Remove the position button. */
  item = g_list_nth (xsheet->priv->position_buttons,
                     position);
  for (iter = item, i = position; iter && i < position + n_frames; iter = iter->next)
    {
      gtk_widget_destroy (gtk_widget_get_parent (iter->data));
    }
  if (item->prev)
    item->prev->next = iter;
  else
    xsheet->priv->position_buttons = iter;
  if (iter)
    {
      iter->prev->next = NULL;
      iter->prev = item->prev;
    }
  item->prev = NULL;
  g_list_free (item);

  /* Remove the comments field. */
  item = g_list_nth (xsheet->priv->comment_fields,
                     position);
  for (iter = item, i = position; iter && i < position + n_frames; iter = iter->next)
    {
      gtk_widget_destroy (gtk_widget_get_parent (iter->data));
    }
  if (item->prev)
    item->prev->next = iter;
  else
    xsheet->priv->comment_fields = iter;
  if (iter)
    {
      iter->prev->next = NULL;
      iter->prev = item->prev;
    }
  item->prev = NULL;
  g_list_free (item);

  /* Remove useless separators. */
  if (framerate == (gdouble) (gint) framerate)
    {
      gint expected;
      gint n_separators;

      n_separators = g_list_length (xsheet->priv->second_separators);
      expected = duration / (gint) framerate;

      for (i = expected; i < n_separators; i++)
        {
          gtk_widget_destroy (xsheet->priv->second_separators->data);
          xsheet->priv->second_separators = g_list_delete_link (xsheet->priv->second_separators,
                                                                xsheet->priv->second_separators);
        }
    }
}

static void
animation_xsheet_reset_layout (AnimationXSheet *xsheet)
{
  GtkWidget *frame;
  GtkWidget *label;
  gint       n_tracks;
  gint       n_frames;
  gint       j;

  gtk_container_foreach (GTK_CONTAINER (xsheet->priv->track_layout),
                         (GtkCallback) gtk_widget_destroy,
                         NULL);
  g_list_free_full (xsheet->priv->cels, (GDestroyNotify) g_list_free);
  xsheet->priv->cels = NULL;
  g_list_free (xsheet->priv->comment_fields);
  xsheet->priv->comment_fields = NULL;
  g_list_free (xsheet->priv->second_separators);
  xsheet->priv->second_separators = NULL;
  g_list_free (xsheet->priv->position_buttons);
  xsheet->priv->position_buttons = NULL;
  xsheet->priv->active_pos_button = NULL;
  xsheet->priv->selected_track = -1;
  g_queue_clear (xsheet->priv->selected_frames);

  /*
   *         + <             > + <          > + <          > +     +
   * | Frame | Background (x9) | Track 1 (x9) | Track 2 (x9) | ... | Comments (x5) |
   * | 1     |                 |              |              |     |               |
   * | 2     |                 |              |              |     |               |
   * | ...   |                 |              |              |     |               |
   * | n     |                 |              |              |     |               |
   */

  n_tracks = animation_cel_animation_get_levels (xsheet->priv->animation);
  n_frames = animation_get_duration (ANIMATION (xsheet->priv->animation));

  /* The cels structure is a matrix of every cel widget. */
  for (j = 0; j < n_tracks; j++)
    {
      xsheet->priv->cels = g_list_prepend (xsheet->priv->cels, NULL);
    }

  /* Add 4 columns for track names and 1 row for frame numbers. */
  gtk_table_resize (GTK_TABLE (xsheet->priv->track_layout),
                    (guint) (n_frames + 2),
                    (guint) (n_tracks * 9 + 6));
  animation_xsheet_add_frames (xsheet, 0, n_frames);

  /* Titles. */
  for (j = 0; j < n_tracks; j++)
    {
      const gchar    *title;
      GtkEntryBuffer *entry_buffer;
      GtkWidget      *toolbar;
      GtkWidget      *image;
      GtkToolItem    *item;

      title = animation_cel_animation_get_track_title (xsheet->priv->animation, j);

      frame = gtk_frame_new (NULL);
      gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_ETCHED_OUT);
      gtk_table_attach (GTK_TABLE (xsheet->priv->track_layout),
                        frame, j * 9 + 1, j * 9 + 10, 1, 2,
                        GTK_FILL, GTK_FILL, 0, 0);
      label = gtk_entry_new ();
      gtk_entry_set_text (GTK_ENTRY (label), title);
      entry_buffer = gtk_entry_get_buffer (GTK_ENTRY (label));
      g_object_set_data (G_OBJECT (entry_buffer), "track-num",
                         GINT_TO_POINTER (j));
      g_signal_connect (entry_buffer,
                        "notify::text",
                        G_CALLBACK (animation_xsheet_track_title_updated),
                        xsheet);
      gtk_container_add (GTK_CONTAINER (frame), label);
      gtk_widget_show (label);
      gtk_widget_show (frame);

      /* Adding a add-track [+] button. */
      toolbar = gtk_toolbar_new ();
      gtk_toolbar_set_icon_size (GTK_TOOLBAR (toolbar),
                                 GTK_ICON_SIZE_SMALL_TOOLBAR);
      gtk_toolbar_set_style (GTK_TOOLBAR (toolbar),
                             GTK_TOOLBAR_ICONS);
      gtk_toolbar_set_show_arrow (GTK_TOOLBAR (toolbar), FALSE);

      item = gtk_separator_tool_item_new ();
      gtk_separator_tool_item_set_draw (GTK_SEPARATOR_TOOL_ITEM (item),
                                        FALSE);
      gtk_tool_item_set_expand (GTK_TOOL_ITEM (item), TRUE);
      gtk_toolbar_insert (GTK_TOOLBAR (toolbar),
                          GTK_TOOL_ITEM (item), 0);
      gtk_widget_show (GTK_WIDGET (item));

      image = gtk_image_new_from_icon_name ("list-add",
                                            GTK_ICON_SIZE_SMALL_TOOLBAR);
      item = gtk_tool_button_new (image, NULL);
      g_object_set_data (G_OBJECT (item), "track-num",
                         GINT_TO_POINTER (j));
      g_signal_connect (item, "clicked",
                        G_CALLBACK (on_track_add_clicked),
                        xsheet);
      gtk_toolbar_insert (GTK_TOOLBAR (toolbar),
                          GTK_TOOL_ITEM (item), -1);
      gtk_widget_show (image);
      gtk_widget_show (GTK_WIDGET (item));

      item = gtk_separator_tool_item_new ();
      gtk_separator_tool_item_set_draw (GTK_SEPARATOR_TOOL_ITEM (item),
                                        FALSE);
      gtk_tool_item_set_expand (GTK_TOOL_ITEM (item), TRUE);
      gtk_toolbar_insert (GTK_TOOLBAR (toolbar),
                          GTK_TOOL_ITEM (item), -1);
      gtk_widget_show (GTK_WIDGET (item));

      gtk_table_attach (GTK_TABLE (xsheet->priv->track_layout),
                        toolbar, j * 9 + 9, j * 9 + 11, 0, 1,
                        GTK_FILL, GTK_FILL, 0, 0);
      gtk_widget_show (toolbar);

      /* Adding toolbar over each track. */
      toolbar = gtk_toolbar_new ();
      gtk_toolbar_set_icon_size (GTK_TOOLBAR (toolbar),
                                 GTK_ICON_SIZE_SMALL_TOOLBAR);
      gtk_toolbar_set_style (GTK_TOOLBAR (toolbar),
                             GTK_TOOLBAR_ICONS);
      gtk_toolbar_set_show_arrow (GTK_TOOLBAR (toolbar), FALSE);

      image = gtk_image_new_from_icon_name ("gimp-menu-left",
                                            GTK_ICON_SIZE_SMALL_TOOLBAR);
      item = gtk_tool_button_new (image, NULL);
      g_object_set_data (G_OBJECT (item), "track-num",
                         GINT_TO_POINTER (j));
      g_signal_connect (item, "clicked",
                        G_CALLBACK (on_track_left_clicked),
                        xsheet);
      gtk_toolbar_insert (GTK_TOOLBAR (toolbar),
                          GTK_TOOL_ITEM (item), 0);
      gtk_widget_show (image);
      gtk_widget_show (GTK_WIDGET (item));

      item = gtk_separator_tool_item_new ();
      gtk_separator_tool_item_set_draw (GTK_SEPARATOR_TOOL_ITEM (item),
                                        FALSE);
      gtk_tool_item_set_expand (GTK_TOOL_ITEM (item), TRUE);
      gtk_toolbar_insert (GTK_TOOLBAR (toolbar),
                          GTK_TOOL_ITEM (item), -1);
      gtk_widget_show (GTK_WIDGET (item));

      image = gtk_image_new_from_icon_name ("edit-delete",
                                            GTK_ICON_SIZE_SMALL_TOOLBAR);
      item = gtk_tool_button_new (image, NULL);
      g_object_set_data (G_OBJECT (item), "track-num",
                         GINT_TO_POINTER (j));
      g_signal_connect (item, "clicked",
                        G_CALLBACK (on_track_delete_clicked),
                        xsheet);
      gtk_toolbar_insert (GTK_TOOLBAR (toolbar),
                          GTK_TOOL_ITEM (item), -1);
      gtk_widget_show (image);
      gtk_widget_show (GTK_WIDGET (item));

      item = gtk_separator_tool_item_new ();
      gtk_separator_tool_item_set_draw (GTK_SEPARATOR_TOOL_ITEM (item),
                                        FALSE);
      gtk_tool_item_set_expand (GTK_TOOL_ITEM (item), TRUE);
      gtk_toolbar_insert (GTK_TOOLBAR (toolbar),
                          GTK_TOOL_ITEM (item), -1);
      gtk_widget_show (GTK_WIDGET (item));

      image = gtk_image_new_from_icon_name ("gimp-menu-right",
                                            GTK_ICON_SIZE_SMALL_TOOLBAR);
      item = gtk_tool_button_new (image, NULL);
      g_object_set_data (G_OBJECT (item), "track-num",
                         GINT_TO_POINTER (j));
      g_signal_connect (item, "clicked",
                        G_CALLBACK (on_track_right_clicked),
                        xsheet);
      gtk_toolbar_insert (GTK_TOOLBAR (toolbar),
                          GTK_TOOL_ITEM (item), -1);
      gtk_widget_show (image);
      gtk_widget_show (GTK_WIDGET (item));

      gtk_table_attach (GTK_TABLE (xsheet->priv->track_layout),
                        toolbar, j * 9 + 2, j * 9 + 9, 0, 1,
                        GTK_FILL, GTK_FILL, 0, 0);
      gtk_widget_show (toolbar);
    }
  frame = gtk_frame_new (NULL);
  label = gtk_label_new (_("Comments"));
  gtk_container_add (GTK_CONTAINER (frame), label);
  gtk_widget_show (label);
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_ETCHED_OUT);
  gtk_table_attach (GTK_TABLE (xsheet->priv->track_layout),
                    frame, n_tracks * 9 + 1, n_tracks * 9 + 6, 1, 2,
                    GTK_FILL, GTK_FILL, 0, 0);

  gtk_widget_show (frame);

  gtk_table_set_row_spacings (GTK_TABLE (xsheet->priv->track_layout), 0);
  gtk_table_set_col_spacings (GTK_TABLE (xsheet->priv->track_layout), 0);
}

static void
on_layer_selection (AnimationLayerView *view,
                    GList              *layers,
                    AnimationXSheet    *xsheet)
{
  GQueue   *frames;
  gpointer  position;

  if (g_queue_is_empty (xsheet->priv->selected_frames))
    return;

  frames = g_queue_copy (xsheet->priv->selected_frames);
  while (! g_queue_is_empty (frames))
    {
      GList     *track_cels;
      GtkWidget *button;

      position = g_queue_pop_head (frames);

      track_cels = g_list_nth_data (xsheet->priv->cels,
                                    xsheet->priv->selected_track);
      button = g_list_nth_data (track_cels,
                                GPOINTER_TO_INT (position));

      animation_cel_animation_set_layers (xsheet->priv->animation,
                                          xsheet->priv->selected_track,
                                          GPOINTER_TO_INT (position),
                                          layers);

      animation_xsheet_rename_cel (xsheet, button, TRUE);
    }
  g_queue_free (frames);
}

static void
on_animation_loaded (Animation       *animation,
                     AnimationXSheet *xsheet)
{
  animation_xsheet_reset_layout (xsheet);
}

static void
on_animation_duration_changed (Animation       *animation,
                               gint             duration,
                               AnimationXSheet *xsheet)
{
  gint prev_duration;

  prev_duration = g_list_length (xsheet->priv->comment_fields);

  if (prev_duration < duration)
    {
      animation_xsheet_add_frames (xsheet, prev_duration,
                                   duration - prev_duration);
    }
  else if (duration < prev_duration)
    {
      animation_xsheet_remove_frames (xsheet, duration,
                                      prev_duration - duration);
    }
}

static void
on_animation_stopped (AnimationPlayback   *playback,
                      AnimationXSheet     *xsheet)
{
  animation_xsheet_jump (xsheet,
                         animation_playback_get_position (playback));
}

static void
on_animation_rendered (AnimationPlayback *playback,
                       gint               frame_number,
                       GeglBuffer        *buffer,
                       gboolean           must_draw_null,
                       AnimationXSheet   *xsheet)
{
  animation_xsheet_jump (xsheet, frame_number);
}

static gboolean
animation_xsheet_frame_clicked (GtkWidget       *button,
                                GdkEvent        *event,
                                AnimationXSheet *xsheet)
{
  gpointer position;

  position = g_object_get_data (G_OBJECT (button), "frame-position");

  animation_playback_jump (xsheet->priv->playback,
                           GPOINTER_TO_INT (position));
  if (xsheet->priv->active_pos_button)
    {
      GtkToggleButton *active_button;

      active_button = GTK_TOGGLE_BUTTON (xsheet->priv->active_pos_button);
      gtk_toggle_button_set_active (active_button, FALSE);
    }
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button),
                                TRUE);
  xsheet->priv->active_pos_button = button;

  /* All handled here. */
  return TRUE;
}

static gboolean
animation_xsheet_cel_clicked (GtkWidget       *button,
                              GdkEvent        *event,
                              AnimationXSheet *xsheet)
{
  gpointer track_num;
  gpointer position;
  gboolean toggled;
  gboolean shift;
  gboolean ctrl;

  track_num = g_object_get_data (G_OBJECT (button), "track-num");
  position  = g_object_get_data (G_OBJECT (button), "frame-position");

  toggled = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (button));
  shift   = ((GdkEventButton *) event)->state & GDK_SHIFT_MASK;
  ctrl    = ((GdkEventButton *) event)->state & GDK_CONTROL_MASK;

  if ((! shift && ! ctrl)                                         ||
      (xsheet->priv->selected_track >= 0 &&
       xsheet->priv->selected_track != GPOINTER_TO_INT (track_num)))
    {
      /* Changing track and normal click resets selection. */
      GList *track_iter;
      GList *frame_iter;

      for (track_iter = xsheet->priv->cels; track_iter; track_iter = track_iter->next)
        {
          for (frame_iter = track_iter->data; frame_iter; frame_iter = frame_iter->next)
            {
              gtk_toggle_button_set_active (frame_iter->data, FALSE);
            }
        }
      g_queue_clear (xsheet->priv->selected_frames);
      xsheet->priv->selected_track = -1;
    }

  if (ctrl)
    {
      /* Ctrl toggle the clicked selection. */
      if (toggled)
        {
          g_queue_remove (xsheet->priv->selected_frames, position);
          if (g_queue_is_empty (xsheet->priv->selected_frames))
            {
              xsheet->priv->selected_track = -1;
            }
        }
      else
        {
          g_queue_push_head (xsheet->priv->selected_frames, position);
          xsheet->priv->selected_track = GPOINTER_TO_INT (track_num);
        }
      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button),
                                    ! toggled);
    }
  else if (shift)
    {
      /* Shift selects all from the first selection, and unselects
       * anything else. */
      GList    *track_iter;
      GList    *frame_iter;
      gint      from = GPOINTER_TO_INT (position);
      gint      to   = from;
      gint      direction;
      gint      min;
      gint      max;
      gint      i;

      xsheet->priv->selected_track = GPOINTER_TO_INT (track_num);
      for (track_iter = xsheet->priv->cels; track_iter; track_iter = track_iter->next)
        {
          for (frame_iter = track_iter->data; frame_iter; frame_iter = frame_iter->next)
            {
              gtk_toggle_button_set_active (frame_iter->data, FALSE);
            }
        }
      if (! g_queue_is_empty (xsheet->priv->selected_frames))
        {
          from = GPOINTER_TO_INT (g_queue_pop_tail (xsheet->priv->selected_frames));
          g_queue_clear (xsheet->priv->selected_frames);
        }
      direction = from > to? -1 : 1;
      for (i = from; i != to + direction; i = i + direction)
        {
          g_queue_push_head (xsheet->priv->selected_frames,
                             GINT_TO_POINTER (i));
        }
      min = MIN (to, from);
      max = MAX (to, from);
      track_iter = g_list_nth (xsheet->priv->cels, xsheet->priv->selected_track);
      frame_iter = track_iter->data;
      for (i = 0; frame_iter && i <= max; frame_iter = frame_iter->next, i++)
        {
          if (i >= min)
            {
              gtk_toggle_button_set_active (frame_iter->data, TRUE);
            }
        }
    }
  else /* ! shift && ! ctrl */
    {
      xsheet->priv->selected_track = GPOINTER_TO_INT (track_num);
      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button), TRUE);
      g_queue_push_head (xsheet->priv->selected_frames, position);
    }

  /* Finally update the layer view. */
  if (g_queue_is_empty (xsheet->priv->selected_frames))
    {
      animation_layer_view_select (ANIMATION_LAYER_VIEW (xsheet->priv->layer_view),
                                   NULL);
    }
  else
    {
      GList *layers;

      /* When several frames are selected, show layers of the first selected. */
      position = g_queue_peek_tail (xsheet->priv->selected_frames);

      layers = animation_cel_animation_get_layers (xsheet->priv->animation,
                                                   GPOINTER_TO_INT (track_num),
                                                   GPOINTER_TO_INT (position));
      animation_layer_view_select (ANIMATION_LAYER_VIEW (xsheet->priv->layer_view),
                                   layers);
    }

  /* All handled here. */
  return TRUE;
}

static void
animation_xsheet_track_title_updated (GtkEntryBuffer  *buffer,
                                      GParamSpec      *param_spec,
                                      AnimationXSheet *xsheet)
{
  const gchar *title;
  GList       *iter;
  gpointer     track_num;

  track_num = g_object_get_data (G_OBJECT (buffer), "track-num");
  title = gtk_entry_buffer_get_text (buffer);

  animation_cel_animation_set_track_title (xsheet->priv->animation,
                                           GPOINTER_TO_INT (track_num),
                                           title);

  iter = g_list_nth_data (xsheet->priv->cels,
                          GPOINTER_TO_INT (track_num));
  for (; iter; iter = iter->next)
    {
      GtkWidget *cel;

      cel = iter->data;
      animation_xsheet_rename_cel (xsheet, cel, FALSE);
    }
}

static void
animation_xsheet_comment_changed (GtkTextBuffer   *text_buffer,
                                  AnimationXSheet *xsheet)
{
  gchar       *text;
  GtkTextIter  start;
  GtkTextIter  end;
  gpointer     position;

  position = g_object_get_data (G_OBJECT (text_buffer), "frame-position");

  gtk_text_buffer_get_bounds (text_buffer, &start, &end);
  text = gtk_text_buffer_get_text (text_buffer, &start, &end, FALSE);
  animation_cel_animation_set_comment (xsheet->priv->animation,
                                       GPOINTER_TO_INT (position),
                                       text);
  g_free (text);
}

static gboolean
animation_xsheet_comment_keypress (GtkWidget       *entry,
                                   GdkEventKey     *event,
                                   AnimationXSheet *xsheet)
{
  gpointer position;

  position = g_object_get_data (G_OBJECT (entry), "frame-position");

  if (event->keyval == GDK_KEY_Tab    ||
      event->keyval == GDK_KEY_KP_Tab ||
      event->keyval == GDK_KEY_ISO_Left_Tab)
    {
      GtkWidget *comment;

      comment = g_list_nth_data (xsheet->priv->comment_fields,
                                 GPOINTER_TO_INT (position) + 1);
      if (comment)
        {
          /* Grab the next comment widget. */
          gtk_widget_grab_focus (comment);
        }
      else
        {
          /* Loop to the first comment after the last. */
          gtk_widget_grab_focus (xsheet->priv->comment_fields->data);
        }
      return TRUE;
    }
  return FALSE;
}

static void
on_track_add_clicked (GtkToolButton   *button,
                      AnimationXSheet *xsheet)
{
  gpointer     track_num;

  track_num = g_object_get_data (G_OBJECT (button), "track-num");
  if (animation_cel_animation_level_add (xsheet->priv->animation,
                                         GPOINTER_TO_INT (track_num) + 1))
    {
      animation_xsheet_reset_layout (xsheet);
    }
}

static void
on_track_delete_clicked (GtkToolButton   *button,
                         AnimationXSheet *xsheet)
{
  gpointer     track_num;

  track_num = g_object_get_data (G_OBJECT (button), "track-num");
  if (animation_cel_animation_level_delete (xsheet->priv->animation,
                                            GPOINTER_TO_INT (track_num)))
    {
      animation_xsheet_reset_layout (xsheet);
    }
}

static void
on_track_left_clicked (GtkToolButton   *button,
                       AnimationXSheet *xsheet)
{
  gpointer     track_num;

  track_num = g_object_get_data (G_OBJECT (button), "track-num");
  g_queue_clear (xsheet->priv->selected_frames);
  animation_cel_animation_level_down (xsheet->priv->animation,
                                      GPOINTER_TO_INT (track_num));
}

static void
on_track_right_clicked (GtkToolButton   *button,
                        AnimationXSheet *xsheet)
{
  gpointer     track_num;

  track_num = g_object_get_data (G_OBJECT (button), "track-num");
  g_queue_clear (xsheet->priv->selected_frames);
  animation_cel_animation_level_up (xsheet->priv->animation,
                                    GPOINTER_TO_INT (track_num));
}

static void
animation_xsheet_rename_cel (AnimationXSheet *xsheet,
                             GtkWidget       *cel,
                             gboolean         recursively)
{
  gchar    *prev_label = NULL;
  GList    *layers;
  GList    *prev_layers;
  gpointer  track_num;
  gpointer  position;

  if (recursively)
    {
      prev_label = g_strdup (gtk_button_get_label (GTK_BUTTON (cel)));
    }

  track_num = g_object_get_data (G_OBJECT (cel), "track-num");
  position  = g_object_get_data (G_OBJECT (cel), "frame-position");

  layers = animation_cel_animation_get_layers (xsheet->priv->animation,
                                               GPOINTER_TO_INT (track_num),
                                               GPOINTER_TO_INT (position));
  prev_layers = animation_cel_animation_get_layers (xsheet->priv->animation,
                                                    GPOINTER_TO_INT (track_num),
                                                    GPOINTER_TO_INT (position) - 1);

  if (layers && prev_layers && layers->data == prev_layers->data)
    {
      gtk_button_set_label (GTK_BUTTON (cel), "-");
    }
  else if (layers)
    {
      const gchar *track_title;
      gchar       *layer_title;
      gint32       image_id;
      gint32       layer;

      track_title = animation_cel_animation_get_track_title (xsheet->priv->animation,
                                                             GPOINTER_TO_INT (track_num));
      image_id = animation_get_image_id (ANIMATION (xsheet->priv->animation));
      layer = gimp_image_get_layer_by_tattoo (image_id,
                                              GPOINTER_TO_INT (layers->data));
      layer_title = gimp_item_get_name (layer);
      if (g_strcmp0 (layer_title, track_title) == 0)
        {
          /* If the track and the layer have exactly the same name. */
          gtk_button_set_label (GTK_BUTTON (cel), "*");
        }
      else if (g_str_has_prefix (layer_title, track_title))
        {
          gchar *cel_label;

          cel_label = g_strdup_printf ("*%s",
                                       layer_title + strlen (track_title));
          gtk_button_set_label (GTK_BUTTON (cel), cel_label);
          g_free (cel_label);
        }
      else
        {
          gtk_button_set_label (GTK_BUTTON (cel), layer_title);
        }
      g_free (layer_title);
    }
  else
    {
      gtk_button_set_label (GTK_BUTTON (cel), "");
    }

  if (recursively)
    {
      const gchar *new_label;
      gboolean     label_changed;

      new_label = gtk_button_get_label (GTK_BUTTON (cel));
      label_changed = (g_strcmp0 (prev_label, new_label) != 0);
      g_free (prev_label);

      if (label_changed)
        {
          GList     *track_cels;
          GtkWidget *next_cel;

          /* Only update the next cel if the label changed. */
          track_cels = g_list_nth_data (xsheet->priv->cels,
                                        GPOINTER_TO_INT (track_num));
          next_cel = g_list_nth_data (track_cels,
                                      GPOINTER_TO_INT (position) + 1);
          if (next_cel)
            {
              animation_xsheet_rename_cel (xsheet, next_cel, TRUE);
            }
        }
    }
}

static void
animation_xsheet_jump (AnimationXSheet *xsheet,
                       gint             position)
{
  if (! animation_playback_is_playing (xsheet->priv->playback))
    {
      GtkWidget *button;

      button = g_list_nth_data (xsheet->priv->position_buttons,
                                position);
      if (xsheet->priv->active_pos_button)
        {
          GtkToggleButton *active_button;

          active_button = GTK_TOGGLE_BUTTON (xsheet->priv->active_pos_button);
          gtk_toggle_button_set_active (active_button, FALSE);
        }
      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button),
                                    TRUE);
      xsheet->priv->active_pos_button = button;

      show_scrolled_child (GTK_SCROLLED_WINDOW (xsheet), button);
    }
}
