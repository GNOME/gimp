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

#include <gtk/gtk.h>

#include <libgimp/gimp.h>

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
  GtkWidget             *layer_view;

  GtkWidget             *track_layout;

  GList                 *cels;
  gint                   selected_track;
  GQueue                *selected_frames;
};

static void animation_xsheet_constructed  (GObject      *object);
static void animation_xsheet_set_property (GObject      *object,
                                           guint         property_id,
                                           const GValue *value,
                                           GParamSpec   *pspec);
static void animation_xsheet_get_property (GObject      *object,
                                           guint         property_id,
                                           GValue       *value,
                                           GParamSpec   *pspec);
static void animation_xsheet_finalize     (GObject      *object);

/* Construction methods */
static void animation_xsheet_reset_layout (AnimationXSheet *xsheet);

/* Callbacks on animation. */
static void on_animation_loaded           (Animation       *animation,
                                           gint             first_frame,
                                           gint             num_frames,
                                           gint             playback_start,
                                           gint             playback_stop,
                                           gint             preview_width,
                                           gint             preview_height,
                                           AnimationXSheet *xsheet);

/* UI Signals */
static gboolean animation_xsheet_cel_clicked         (GtkWidget       *button,
                                                      GdkEvent        *event,
                                                      AnimationXSheet *xsheet);
static void     animation_xsheet_track_title_updated (GtkEntryBuffer  *buffer,
                                                      GParamSpec      *param_spec,
                                                      AnimationXSheet *xsheet);

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
                      GtkWidget             *layer_view)
{
  GtkWidget *xsheet;

  xsheet = g_object_new (ANIMATION_TYPE_XSHEET,
                         "animation", animation,
                         "layer-view", layer_view,
                         NULL);

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
  xsheet->priv->track_layout = gtk_table_new (1, 1, TRUE);
  
  gtk_scrolled_window_add_with_viewport (GTK_SCROLLED_WINDOW (xsheet),
                                         xsheet->priv->track_layout);
  gtk_table_set_row_spacings (GTK_TABLE (xsheet->priv->track_layout), 0);
  gtk_table_set_col_spacings (GTK_TABLE (xsheet->priv->track_layout), 0);

  gtk_widget_show (xsheet->priv->track_layout);

  /* Reload everything when we reload the animation. */
  g_signal_connect_after (xsheet->priv->animation, "loaded",
                          G_CALLBACK (on_animation_loaded), xsheet);
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

  if (xsheet->priv->animation)
    g_object_unref (xsheet->priv->animation);
  if (xsheet->priv->layer_view)
    g_object_unref (xsheet->priv->layer_view);
  g_queue_free (xsheet->priv->selected_frames);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
animation_xsheet_reset_layout (AnimationXSheet *xsheet)
{
  GtkWidget *frame;
  GtkWidget *label;
  GtkWidget *comment_field;
  GList     *iter;
  gint       n_tracks;
  gint       n_frames;
  gint       i;
  gint       j;

  gtk_container_foreach (GTK_CONTAINER (xsheet->priv->track_layout),
                         (GtkCallback) gtk_widget_destroy,
                         NULL);
  g_list_free (xsheet->priv->cels);
  xsheet->priv->cels = NULL;
  xsheet->priv->selected_track = -1;
  g_queue_clear (xsheet->priv->selected_frames);
  /*
   * | Frame | Background | Track 1 | Track 2 | ... | Comments (x5) |
   * | 1     |            |         |         |     |               |
   * | 2     |            |         |         |     |               |
   * | ...   |            |         |         |     |               |
   * | n     |            |         |         |     |               |
   */

  n_tracks = animation_cel_animation_get_levels (xsheet->priv->animation);
  n_frames = animation_get_length (ANIMATION (xsheet->priv->animation));

  /* The cels structure is a matrix of every cel widget. */
  for (j = 0; j < n_tracks; j++)
    {
      xsheet->priv->cels = g_list_prepend (xsheet->priv->cels, NULL);
    }

  /* Add 4 columns for track names and 1 row for frame numbers. */
  gtk_table_resize (GTK_TABLE (xsheet->priv->track_layout),
                    (guint) (n_frames + 1),
                    (guint) (n_tracks + 6));
  for (i = 0; i < n_frames; i++)
    {
      gchar     *num_str;

      frame = gtk_frame_new (NULL);
      gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_ETCHED_OUT);
      gtk_table_attach (GTK_TABLE (xsheet->priv->track_layout),
                        frame, 0, 1, i + 1, i + 2,
                        GTK_FILL, GTK_FILL, 0, 0);

      num_str = g_strdup_printf ("%d", i + 1);
      label = gtk_label_new (num_str);
      gtk_container_add (GTK_CONTAINER (frame), label);

      gtk_widget_show (label);
      gtk_widget_show (frame);

      for (j = 0; j < n_tracks; j++)
        {
          GtkWidget *cel;
          GList     *track_cels = g_list_nth (xsheet->priv->cels, j);

          frame = gtk_frame_new (NULL);
          gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_IN);
          gtk_table_attach (GTK_TABLE (xsheet->priv->track_layout),
                            frame, j + 1, j + 2, i + 1, i + 2,
                            GTK_FILL, GTK_FILL, 0, 0);

          cel = gtk_toggle_button_new ();
          track_cels->data = g_list_prepend (track_cels->data, cel);
          g_object_set_data (G_OBJECT (cel), "track-num",
                             GINT_TO_POINTER (j));
          g_object_set_data (G_OBJECT (cel), "frame-position",
                             GINT_TO_POINTER (i));
          g_signal_connect (cel, "button-release-event",
                            G_CALLBACK (animation_xsheet_cel_clicked),
                            xsheet);
          gtk_button_set_relief (GTK_BUTTON (cel), GTK_RELIEF_NONE);
          gtk_button_set_focus_on_click (GTK_BUTTON (cel), FALSE);
          gtk_container_add (GTK_CONTAINER (frame), cel);
          gtk_widget_show (cel);
          gtk_widget_show (frame);
        }
      /* Comments */
      frame = gtk_frame_new (NULL);
      gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_IN);
      gtk_table_attach (GTK_TABLE (xsheet->priv->track_layout),
                        frame, n_tracks + 1, n_tracks + 6, i + 1, i + 2,
                        GTK_FILL, GTK_FILL, 0, 0);
      comment_field = gtk_text_view_new ();
      gtk_container_add (GTK_CONTAINER (frame), comment_field);
      gtk_widget_show (comment_field);
      gtk_widget_show (frame);
    }
  for (iter = xsheet->priv->cels; iter; iter = iter->next)
    {
      iter->data = g_list_reverse (iter->data);
    }

  /* Titles. */
  for (j = 0; j < n_tracks; j++)
    {
      const gchar    *title;
      GtkEntryBuffer *entry_buffer;

      title = animation_cel_animation_get_track_title (xsheet->priv->animation, j);

      frame = gtk_frame_new (NULL);
      gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_ETCHED_OUT);
      gtk_table_attach (GTK_TABLE (xsheet->priv->track_layout),
                        frame, j + 1, j + 2, 0, 1,
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
    }
  frame = gtk_frame_new (NULL);
  label = gtk_label_new (_("Comments"));
  gtk_container_add (GTK_CONTAINER (frame), label);
  gtk_widget_show (label);
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_ETCHED_OUT);
  gtk_table_attach (GTK_TABLE (xsheet->priv->track_layout),
                    frame, n_tracks + 1, n_tracks + 6, 0, 1,
                    GTK_FILL, GTK_FILL, 0, 0);

  gtk_widget_show (frame);

  gtk_table_set_row_spacings (GTK_TABLE (xsheet->priv->track_layout), 0);
  gtk_table_set_col_spacings (GTK_TABLE (xsheet->priv->track_layout), 0);
}

static void
on_animation_loaded (Animation       *animation,
                     gint             first_frame,
                     gint             num_frames,
                     gint             playback_start,
                     gint             playback_stop,
                     gint             preview_width,
                     gint             preview_height,
                     AnimationXSheet *xsheet)
{
  animation_xsheet_reset_layout (xsheet);
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

  if ((! shift && ! ctrl) ||
      (xsheet->priv->selected_track >= 0 &&
       xsheet->priv->selected_track != GPOINTER_TO_INT (track_num)))
    {
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
    }
  else
    {
      g_queue_remove (xsheet->priv->selected_frames, position);
    }
  xsheet->priv->selected_track = GPOINTER_TO_INT (track_num);

  if (! toggled)
    {
      GList *frame_iter;
      gint   prev_selection = GPOINTER_TO_INT (position);
      gint   direction = 1;
      gint   selection_size;
      gint   i;

      frame_iter = g_list_nth (xsheet->priv->cels, GPOINTER_TO_INT (track_num))->data;

      if (shift)
        {
          prev_selection = GPOINTER_TO_INT (g_queue_pop_head (xsheet->priv->selected_frames));
          if (prev_selection > GPOINTER_TO_INT (position))
            {
              direction = -1;
            }
        }

      selection_size = MAX (GPOINTER_TO_INT (position), prev_selection) - MIN (GPOINTER_TO_INT (position), prev_selection) + 1;
      for (i = prev_selection; selection_size; i = i + direction, selection_size--)
        {
          g_queue_push_head (xsheet->priv->selected_frames, GINT_TO_POINTER (i));
        }

      frame_iter = g_list_nth (frame_iter, MIN (GPOINTER_TO_INT (position), prev_selection));
      selection_size = MAX (GPOINTER_TO_INT (position), prev_selection) - MIN (GPOINTER_TO_INT (position), prev_selection) + 1;
      for (; frame_iter && selection_size > 0; frame_iter = frame_iter->next, selection_size--)
        {
          gtk_toggle_button_set_active (frame_iter->data, TRUE);
        }
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
  gpointer     track_num;

  track_num = g_object_get_data (G_OBJECT (buffer), "track-num");
  title = gtk_entry_buffer_get_text (buffer);

  animation_cel_animation_set_track_title (xsheet->priv->animation,
                                           GPOINTER_TO_INT (track_num),
                                           title);
}
