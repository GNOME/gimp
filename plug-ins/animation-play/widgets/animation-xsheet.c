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
#include <libgimp/gimpui.h>

#include "animation-utils.h"

#include "core/animation.h"
#include "core/animation-camera.h"
#include "core/animation-playback.h"
#include "core/animation-celanimation.h"

#include "animation-keyframe-view.h"
#include "animation-layer-view.h"
#include "animation-menus.h"

#include "animation-xsheet.h"

#include "libgimp/stdplugins-intl.h"

enum
{
  PROP_0,
  PROP_ANIMATION,
  PROP_LAYER_VIEW,
  PROP_KEYFRAME_VIEW,
  PROP_SUITE_CYCLE,
  PROP_SUITE_FPI
};

struct _AnimationXSheetPrivate
{
  AnimationCelAnimation *animation;
  AnimationPlayback     *playback;

  GtkWidget             *layer_view;
  GtkWidget             *keyframe_view;

  GtkWidget             *track_layout;

  GtkWidget             *active_pos_button;
  GList                 *position_buttons;

  GtkWidget             *suite_box;
  GtkWidget             *suite_button;
  gint                   suite_position;
  gint                   suite_level;
  gint                   suite_cycle;
  gint                   suite_fpi;

  GList                 *effect_buttons;

  GList                 *cels;
  gint                   selected_track;
  GQueue                *selected_frames;

  GList                 *comment_fields;
  GList                 *second_separators;
  GtkWidget             *comments_title;

  GList                 *track_buttons;
  GList                 *add_buttons;
  GList                 *titles;
};


/* Data types used to traverse layer trees when creating animation
 * cycles.
 */
typedef struct
{
  GTree *suite;
  gint   count;
}
MergeData;

typedef struct
{
  /* Stable parameters. */
  AnimationXSheet *xsheet;
  gint             level;
  gint             suite_length;

  /* Updated by the caller. */
  gint             loop_idx;

  /* Updated by the traversal function. */
  gint             index;
  gboolean         end_of_animation;
}
CreateData;

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
static void     animation_xsheet_add_headers   (AnimationXSheet   *xsheet,
                                                gint               level);
static void     animation_xsheet_add_track     (AnimationXSheet   *xsheet,
                                                gint               level);
static void     animation_xsheet_remove_track  (AnimationXSheet   *xsheet,
                                                gint               level);
static void     animation_xsheet_add_frames    (AnimationXSheet   *xsheet,
                                                gint               position,
                                                gint               n_frames);
static void     animation_xsheet_move_tracks   (AnimationXSheet   *xsheet,
                                                gint               level);
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
/* Callbacks on camera. */
static void     on_camera_keyframe_set         (AnimationCamera   *camera,
                                                gint               position,
                                                AnimationXSheet   *xsheet);
static void     on_camera_keyframe_deleted     (AnimationCamera   *camera,
                                                gint               position,
                                                AnimationXSheet   *xsheet);

/* Callbacks on playback. */
static void     on_playback_stopped            (AnimationPlayback *playback,
                                                AnimationXSheet   *xsheet);
static void     on_playback_rendered           (AnimationPlayback *animation,
                                                gint               frame_number,
                                                GeglBuffer        *buffer,
                                                gboolean           must_draw_null,
                                                AnimationXSheet   *xsheet);

/* Callbacks on layer view. */
static void     on_layer_selection             (AnimationLayerView *view,
                                                GList              *layers,
                                                AnimationXSheet    *xsheet);

/* UI Signals */
static void     animation_xsheet_suite_do            (GtkWidget       *button,
                                                      AnimationXSheet *xsheet);
static void     animation_xsheet_suite_cancelled     (GtkWidget       *button,
                                                      AnimationXSheet *xsheet);
static gboolean animation_xsheet_effect_clicked      (GtkWidget       *button,
                                                      GdkEvent        *event,
                                                      AnimationXSheet *xsheet);
static gboolean animation_xsheet_frame_clicked       (GtkWidget       *button,
                                                      GdkEvent        *event,
                                                      AnimationXSheet *xsheet);
static gboolean animation_xsheet_cel_clicked         (GtkWidget       *button,
                                                      GdkEventButton  *event,
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
                                                      gboolean         recursively,
                                                      gint             stop_position);
static void     animation_xsheet_jump                (AnimationXSheet *xsheet,
                                                      gint             position);
static void     animation_xsheet_attach_cel          (AnimationXSheet *xsheet,
                                                      GtkWidget       *cel,
                                                      gint             track,
                                                      gint             pos);
static void     animation_xsheet_extract_layer_suite (AnimationXSheet  *xsheet,
                                                      GTree           **suite,
                                                      gboolean          same_numbering,
                                                      const gchar      *prefix,
                                                      const gchar      *suffix,
                                                      const gint       *lower_key);
static gboolean create_suite                         (gint             *key,
                                                      GList            *layers,
                                                      CreateData       *data);
static gboolean merge_suites                         (gint             *key,
                                                      GList            *layers,
                                                      MergeData        *data);
static gint     compare_keys                         (gint             *key1,
                                                      gint             *key2);

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
  /**
   * AnimationXSheet:layer-view:
   *
   * The associated #AnimationLayerView.
   */
  g_object_class_install_property (object_class, PROP_LAYER_VIEW,
                                   g_param_spec_object ("layer-view",
                                                        NULL, NULL,
                                                        ANIMATION_TYPE_LAYER_VIEW,
                                                        G_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT_ONLY));
  /**
   * AnimationXSheet:keyframe-view:
   *
   * The associated #AnimationLayerView.
   */
  g_object_class_install_property (object_class, PROP_KEYFRAME_VIEW,
                                   g_param_spec_object ("keyframe-view",
                                                        NULL, NULL,
                                                        ANIMATION_TYPE_KEYFRAME_VIEW,
                                                        G_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT_ONLY));
  /**
   * AnimationXSheet:suite-cycle:
   *
   * The configured cycle repeat. 0 means indefinitely.
   */
  g_object_class_install_property (object_class, PROP_SUITE_CYCLE,
                                   g_param_spec_int ("suite-cycle", NULL,
                                                     NULL,
                                                     0, 100, 0,
                                                     GIMP_PARAM_READWRITE));
  /**
   * AnimationXSheet:suite-frames-per-image:
   *
   * The number of frames each image will be displayed.
   */
  g_object_class_install_property (object_class, PROP_SUITE_FPI,
                                   g_param_spec_int ("suite-frames-per-image", NULL,
                                                     NULL,
                                                     0, 10, 0,
                                                     GIMP_PARAM_READWRITE));

  g_type_class_add_private (klass, sizeof (AnimationXSheetPrivate));
}

static void
animation_xsheet_init (AnimationXSheet *xsheet)
{
  xsheet->priv = G_TYPE_INSTANCE_GET_PRIVATE (xsheet,
                                              ANIMATION_TYPE_XSHEET,
                                              AnimationXSheetPrivate);
  xsheet->priv->selected_frames = g_queue_new ();
  xsheet->priv->suite_fpi       = 2;
  xsheet->priv->suite_cycle     = 1;
}

/************ Public Functions ****************/

GtkWidget *
animation_xsheet_new (AnimationCelAnimation *animation,
                      AnimationPlayback     *playback,
                      GtkWidget             *layer_view,
                      GtkWidget             *keyframe_view)
{
  GtkWidget *xsheet;

  xsheet = g_object_new (ANIMATION_TYPE_XSHEET,
                         "animation", animation,
                         "layer-view", layer_view,
                         "keyframe-view", keyframe_view,
                         NULL);
  ANIMATION_XSHEET (xsheet)->priv->playback = playback;
  g_signal_connect (ANIMATION_XSHEET (xsheet)->priv->playback,
                    "render", G_CALLBACK (on_playback_rendered),
                    xsheet);
  g_signal_connect (ANIMATION_XSHEET (xsheet)->priv->playback,
                    "stop", G_CALLBACK (on_playback_stopped),
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
    case PROP_KEYFRAME_VIEW:
      xsheet->priv->keyframe_view = g_value_dup_object (value);
      break;
    case PROP_SUITE_CYCLE:
      xsheet->priv->suite_cycle = g_value_get_int (value);
      break;
    case PROP_SUITE_FPI:
      xsheet->priv->suite_fpi = g_value_get_int (value);
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
    case PROP_KEYFRAME_VIEW:
      g_value_set_object (value, xsheet->priv->keyframe_view);
      break;
    case PROP_SUITE_CYCLE:
      g_value_set_int (value, xsheet->priv->suite_cycle);
      break;
    case PROP_SUITE_FPI:
      g_value_set_int (value, xsheet->priv->suite_fpi);
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
                                        G_CALLBACK (on_playback_rendered),
                                        xsheet);
  g_signal_handlers_disconnect_by_func (ANIMATION_XSHEET (xsheet)->priv->playback,
                                        G_CALLBACK (on_playback_stopped),
                                        xsheet);
  if (xsheet->priv->animation)
    g_object_unref (xsheet->priv->animation);
  if (xsheet->priv->layer_view)
    g_object_unref (xsheet->priv->layer_view);
  g_queue_free (xsheet->priv->selected_frames);

  g_list_free_full (xsheet->priv->cels, (GDestroyNotify) g_list_free);
  g_list_free (xsheet->priv->comment_fields);
  g_list_free (xsheet->priv->effect_buttons);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
animation_xsheet_add_headers (AnimationXSheet *xsheet,
                              gint             level)
{
  const gchar    *title;
  GtkEntryBuffer *entry_buffer;
  GtkWidget      *frame;
  GtkWidget      *label;
  GtkWidget      *toolbar;
  GtkWidget      *image;
  GtkToolItem    *item;

  title = animation_cel_animation_get_track_title (xsheet->priv->animation,
                                                   level);

  /* Adding a title. */
  frame = gtk_frame_new (NULL);
  xsheet->priv->titles = g_list_insert (xsheet->priv->titles,
                                        frame, level);
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_ETCHED_OUT);
  gtk_table_attach (GTK_TABLE (xsheet->priv->track_layout),
                    frame, level * 9 + 2, level * 9 + 11, 1, 2,
                    GTK_FILL, GTK_FILL, 0, 0);
  label = gtk_entry_new ();
  gtk_entry_set_text (GTK_ENTRY (label), title);
  entry_buffer = gtk_entry_get_buffer (GTK_ENTRY (label));
  g_object_set_data (G_OBJECT (entry_buffer), "track-num",
                     GINT_TO_POINTER (level));
  g_signal_connect (entry_buffer,
                    "notify::text",
                    G_CALLBACK (animation_xsheet_track_title_updated),
                    xsheet);
  gtk_container_add (GTK_CONTAINER (frame), label);
  gtk_widget_show (label);
  gtk_widget_show (frame);

  /* Adding a add-track [+] button. */
  toolbar = gtk_toolbar_new ();
  xsheet->priv->add_buttons = g_list_insert (xsheet->priv->add_buttons,
                                             toolbar, level);
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
                     GINT_TO_POINTER (level));
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
                    toolbar, level * 9 + 10, level * 9 + 12, 0, 1,
                    GTK_FILL, GTK_FILL, 0, 0);
  gtk_widget_show (toolbar);

  /* Adding toolbar over each track. */
  toolbar = gtk_toolbar_new ();
  xsheet->priv->track_buttons = g_list_insert (xsheet->priv->track_buttons,
                                               toolbar, level);
  gtk_toolbar_set_icon_size (GTK_TOOLBAR (toolbar),
                             GTK_ICON_SIZE_SMALL_TOOLBAR);
  gtk_toolbar_set_style (GTK_TOOLBAR (toolbar),
                         GTK_TOOLBAR_ICONS);
  gtk_toolbar_set_show_arrow (GTK_TOOLBAR (toolbar), FALSE);

  image = gtk_image_new_from_icon_name ("gimp-menu-left",
                                        GTK_ICON_SIZE_SMALL_TOOLBAR);
  item = gtk_tool_button_new (image, NULL);
  g_object_set_data (G_OBJECT (item), "track-num",
                     GINT_TO_POINTER (level));
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
                     GINT_TO_POINTER (level));
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
                     GINT_TO_POINTER (level));
  g_signal_connect (item, "clicked",
                    G_CALLBACK (on_track_right_clicked),
                    xsheet);
  gtk_toolbar_insert (GTK_TOOLBAR (toolbar),
                      GTK_TOOL_ITEM (item), -1);
  gtk_widget_show (image);
  gtk_widget_show (GTK_WIDGET (item));

  gtk_table_attach (GTK_TABLE (xsheet->priv->track_layout),
                    toolbar, level * 9 + 3, level * 9 + 10, 0, 1,
                    GTK_FILL, GTK_FILL, 0, 0);
  gtk_widget_show (toolbar);
}

static void
animation_xsheet_add_track (AnimationXSheet *xsheet,
                            gint             level)
{
  GList *track;
  gint   duration;
  gint   i;

  duration = animation_get_duration (ANIMATION (xsheet->priv->animation));

  /* Create a new track. */
  xsheet->priv->cels = g_list_insert (xsheet->priv->cels, NULL, level);
  track = g_list_nth (xsheet->priv->cels, level);
  /* Create new cels. */
  for (i = 0; i < duration; i++)
    {
      GtkWidget *cel;

      cel = gtk_toggle_button_new ();
      track->data = g_list_append (track->data, cel);
      animation_xsheet_attach_cel (xsheet, cel, level, i);
      animation_xsheet_rename_cel (xsheet, cel, FALSE, -1);

      g_signal_connect (cel, "button-release-event",
                        G_CALLBACK (animation_xsheet_cel_clicked),
                        xsheet);
      gtk_button_set_relief (GTK_BUTTON (cel), GTK_RELIEF_NONE);
      gtk_button_set_focus_on_click (GTK_BUTTON (cel), FALSE);
      gtk_widget_show (cel);
    }
  /* Create a new title and top buttons. */
  animation_xsheet_add_headers (xsheet, level);
  /* Move higher-level tracks one level up in the layout. */
  animation_xsheet_move_tracks (xsheet, level + 1);
}

static void
animation_xsheet_remove_track (AnimationXSheet *xsheet,
                               gint             level)
{
  GList *track;
  GList *item;
  GList *iter;

  /* Remove all data first. */
  track = g_list_nth (xsheet->priv->cels, level);
  for (iter = track->data; iter; iter = iter->next)
    {
      gtk_widget_destroy (gtk_widget_get_parent (iter->data));
    }
  g_list_free (track->data);
  xsheet->priv->cels = g_list_delete_link (xsheet->priv->cels,
                                           track);

  item = g_list_nth (xsheet->priv->add_buttons, level);
  gtk_widget_destroy (item->data);
  xsheet->priv->add_buttons = g_list_delete_link (xsheet->priv->add_buttons,
                                                  item);
  item = g_list_nth (xsheet->priv->track_buttons, level);
  gtk_widget_destroy (item->data);
  xsheet->priv->track_buttons = g_list_delete_link (xsheet->priv->track_buttons,
                                                    item);
  item = g_list_nth (xsheet->priv->titles, level);
  gtk_widget_destroy (item->data);
  xsheet->priv->titles = g_list_delete_link (xsheet->priv->titles,
                                             item);

  animation_xsheet_move_tracks (xsheet, level);
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

          cel = gtk_toggle_button_new ();
          iter->data = g_list_append (iter->data, cel);
          animation_xsheet_attach_cel (xsheet, cel, j, i);
          animation_xsheet_rename_cel (xsheet, cel, FALSE, -1);

          g_signal_connect (cel, "button-release-event",
                            G_CALLBACK (animation_xsheet_cel_clicked),
                            xsheet);
          gtk_button_set_relief (GTK_BUTTON (cel), GTK_RELIEF_NONE);
          gtk_button_set_focus_on_click (GTK_BUTTON (cel), FALSE);
          gtk_widget_show (cel);
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

      /* Create effect button. */
      frame = gtk_frame_new (NULL);
      gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_ETCHED_OUT);
      gtk_table_attach (GTK_TABLE (xsheet->priv->track_layout),
                        frame, 1, 2, i + 2, i + 3,
                        GTK_FILL, GTK_FILL, 0, 0);

      label = gtk_toggle_button_new ();
      xsheet->priv->effect_buttons = g_list_append (xsheet->priv->effect_buttons,
                                                    label);
      g_object_set_data (G_OBJECT (label), "frame-position",
                         GINT_TO_POINTER (i));
      gtk_button_set_relief (GTK_BUTTON (label), GTK_RELIEF_NONE);
      gtk_button_set_focus_on_click (GTK_BUTTON (label), FALSE);
      g_signal_connect (label, "button-press-event",
                        G_CALLBACK (animation_xsheet_effect_clicked),
                        xsheet);
      g_signal_connect (animation_cel_animation_get_main_camera (xsheet->priv->animation),
                        "keyframe-set",
                        G_CALLBACK (on_camera_keyframe_set),
                        xsheet);
      g_signal_connect (animation_cel_animation_get_main_camera (xsheet->priv->animation),
                        "keyframe-deleted",
                        G_CALLBACK (on_camera_keyframe_deleted),
                        xsheet);
      gtk_container_add (GTK_CONTAINER (frame), label);

      gtk_widget_show (label);
      gtk_widget_show (frame);

      /* Create new comment fields. */
      frame = gtk_frame_new (NULL);
      gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_IN);
      gtk_table_attach (GTK_TABLE (xsheet->priv->track_layout),
                        frame, n_tracks * 9 + 2, n_tracks * 9 + 7, i + 2, i + 3,
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

  /* Remove the effect button. */
  item = g_list_nth (xsheet->priv->effect_buttons, position);
  for (iter = item, i = position; iter && i < position + n_frames; iter = iter->next)
    {
      gtk_widget_destroy (gtk_widget_get_parent (iter->data));
    }
  if (item->prev)
    item->prev->next = iter;
  else
    xsheet->priv->effect_buttons = iter;
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
  GtkWidget *image;
  GtkWidget *label;
  gint       n_tracks;
  gint       n_frames;
  gint       j;

  gtk_container_foreach (GTK_CONTAINER (xsheet->priv->track_layout),
                         (GtkCallback) gtk_widget_destroy,
                         NULL);
  g_list_free_full (xsheet->priv->cels, (GDestroyNotify) g_list_free);
  xsheet->priv->cels = NULL;
  g_list_free (xsheet->priv->effect_buttons);
  xsheet->priv->effect_buttons = NULL;
  g_list_free (xsheet->priv->comment_fields);
  xsheet->priv->comment_fields = NULL;
  g_list_free (xsheet->priv->second_separators);
  xsheet->priv->second_separators = NULL;
  g_list_free (xsheet->priv->position_buttons);
  xsheet->priv->position_buttons = NULL;
  g_list_free (xsheet->priv->track_buttons);
  xsheet->priv->track_buttons = NULL;
  g_list_free (xsheet->priv->add_buttons);
  xsheet->priv->add_buttons = NULL;
  g_list_free (xsheet->priv->titles);
  xsheet->priv->titles = NULL;

  xsheet->priv->active_pos_button = NULL;
  xsheet->priv->selected_track = -1;
  xsheet->priv->comments_title = NULL;
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
                    (guint) (n_tracks * 9 + 7));
  animation_xsheet_add_frames (xsheet, 0, n_frames);

  /* Titles. */
  for (j = 0; j < n_tracks; j++)
    {
      animation_xsheet_add_headers (xsheet, j);
    }

  image = gtk_image_new_from_icon_name ("camera-video",
                                        GTK_ICON_SIZE_SMALL_TOOLBAR);
  gtk_table_attach (GTK_TABLE (xsheet->priv->track_layout), image,
                    1, 2, 1, 2,
                    GTK_FILL, GTK_FILL, 0, 0);
  gtk_widget_show (image);

  frame = gtk_frame_new (NULL);
  label = gtk_label_new (_("Comments"));
  gtk_container_add (GTK_CONTAINER (frame), label);
  gtk_widget_show (label);
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_ETCHED_OUT);
  gtk_table_attach (GTK_TABLE (xsheet->priv->track_layout),
                    frame, n_tracks * 9 + 2, n_tracks * 9 + 7, 1, 2,
                    GTK_FILL, GTK_FILL, 0, 0);
  xsheet->priv->comments_title = frame;
  gtk_widget_show (frame);

  gtk_table_set_row_spacings (GTK_TABLE (xsheet->priv->track_layout), 0);
  gtk_table_set_col_spacings (GTK_TABLE (xsheet->priv->track_layout), 0);
}

/*
 * animation_xsheet_move_tracks:
 * @level: the left-est level from which to refresh the layout.
 *
 * Refresh the layout's widgets from @level and over. This should
 * be used after adding or removing tracks.
 */
static void
animation_xsheet_move_tracks (AnimationXSheet *xsheet,
                              gint             level)
{
  GList *track;
  GList *iter;
  gint   n_tracks;
  gint   i;
  gint   j;

  /* Now move next tracks one level up. */
  track = g_list_nth (xsheet->priv->cels, level);
  for (j = level, iter = track; iter; iter = iter->next, j++)
    {
      GList *iter2;

      for (i = 0, iter2 = iter->data; iter2; iter2 = iter2->next, i++)
        {
          animation_xsheet_attach_cel (xsheet, iter2->data, j, i);
        }
    }

  /* Titles */
  track = g_list_nth (xsheet->priv->titles, level);
  for (j = level, iter = track; iter; iter = iter->next, j++)
    {
      GtkWidget      *frame;
      GtkWidget      *entry;
      GtkEntryBuffer *buffer;

      frame = g_object_ref (iter->data);
      entry = gtk_bin_get_child (GTK_BIN (frame));
      buffer = gtk_entry_get_buffer (GTK_ENTRY (entry));
      g_object_set_data (G_OBJECT (buffer), "track-num",
                         GINT_TO_POINTER (j));

      gtk_container_remove (GTK_CONTAINER (xsheet->priv->track_layout),
                            frame);
      gtk_table_attach (GTK_TABLE (xsheet->priv->track_layout),
                        frame, j * 9 + 2, j * 9 + 11, 1, 2,
                        GTK_FILL, GTK_FILL, 0, 0);
      g_object_unref (frame);
    }

  /* Comments */
  n_tracks = animation_cel_animation_get_levels (xsheet->priv->animation);
  for (i = 0, iter = xsheet->priv->comment_fields; iter; iter = iter->next, i++)
    {
      GtkWidget *comment = iter->data;
      GtkWidget *frame;

      frame = g_object_ref (gtk_widget_get_parent (comment));
      gtk_container_remove (GTK_CONTAINER (xsheet->priv->track_layout),
                            frame);
      gtk_table_attach (GTK_TABLE (xsheet->priv->track_layout),
                        frame, n_tracks * 9 + 2, n_tracks * 9 + 7, i + 2, i + 3,
                        GTK_FILL, GTK_FILL, 0, 0);
      g_object_unref (frame);
    }

  /* Comments title */
  g_object_ref (xsheet->priv->comments_title);
  gtk_container_remove (GTK_CONTAINER (xsheet->priv->track_layout),
                        xsheet->priv->comments_title);
  gtk_table_attach (GTK_TABLE (xsheet->priv->track_layout),
                    xsheet->priv->comments_title,
                    n_tracks * 9 + 2, n_tracks * 9 + 7, 1, 2,
                    GTK_FILL, GTK_FILL, 0, 0);
  g_object_unref (xsheet->priv->comments_title);

  /* Track buttons */
  iter = g_list_nth (xsheet->priv->track_buttons, level);
  for (j = level; iter; iter = iter->next, j++)
    {
      GtkWidget *toolbar;
      GList     *buttons;
      GList     *iter2;

      toolbar = iter->data;
      buttons = gtk_container_get_children (GTK_CONTAINER (toolbar));
      for (iter2 = buttons; iter2; iter2 = iter2->next)
        {
          g_object_set_data (G_OBJECT (iter2->data), "track-num",
                             GINT_TO_POINTER (j));
        }
      g_list_free (buttons);

      g_object_ref (toolbar);
      gtk_container_remove (GTK_CONTAINER (xsheet->priv->track_layout),
                            toolbar);
      gtk_table_attach (GTK_TABLE (xsheet->priv->track_layout),
                        toolbar, j * 9 + 2, j * 9 + 9, 0, 1,
                        GTK_FILL, GTK_FILL, 0, 0);
      g_object_unref (toolbar);
    }

  /* Add buttons */
  iter = g_list_nth (xsheet->priv->add_buttons, level);
  for (j = level; iter; iter = iter->next, j++)
    {
      GtkWidget *toolbar;
      GList     *buttons;
      GList     *iter2;

      toolbar = iter->data;
      buttons = gtk_container_get_children (GTK_CONTAINER (toolbar));
      for (iter2 = buttons; iter2; iter2 = iter2->next)
        {
          g_object_set_data (G_OBJECT (iter2->data), "track-num",
                             GINT_TO_POINTER (j));
        }
      g_list_free (buttons);

      g_object_ref (toolbar);
      gtk_container_remove (GTK_CONTAINER (xsheet->priv->track_layout),
                            toolbar);
      gtk_table_attach (GTK_TABLE (xsheet->priv->track_layout),
                        toolbar, j * 9 + 9, j * 9 + 11, 0, 1,
                        GTK_FILL, GTK_FILL, 0, 0);
      g_object_unref (toolbar);
    }
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
  g_queue_sort (frames, compare_int_from, 0);
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

      animation_xsheet_rename_cel (xsheet, button, TRUE,
                                   GPOINTER_TO_INT (g_queue_peek_head (frames)));
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
on_camera_keyframe_set (AnimationCamera *camera,
                        gint             position,
                        AnimationXSheet *xsheet)
{
  GtkWidget *button;

  button = g_list_nth_data (xsheet->priv->effect_buttons,
                            position);

  if (button)
    {
      GtkWidget *image;

      if (gtk_bin_get_child (GTK_BIN (button)))
        {
          gtk_container_remove (GTK_CONTAINER (button),
                                gtk_bin_get_child (GTK_BIN (button)));
        }
      image = gtk_image_new_from_icon_name ("gtk-ok",
                                            GTK_ICON_SIZE_SMALL_TOOLBAR);
      gtk_container_add (GTK_CONTAINER (button), image);
      gtk_widget_show (image);
    }
}

static void
on_camera_keyframe_deleted (AnimationCamera *camera,
                            gint             position,
                            AnimationXSheet *xsheet)
{
  GtkWidget *button;

  button = g_list_nth_data (xsheet->priv->effect_buttons,
                            position);

  if (button && gtk_bin_get_child (GTK_BIN (button)))
    {
      gtk_container_remove (GTK_CONTAINER (button),
                            gtk_bin_get_child (GTK_BIN (button)));
    }
}

static void
on_playback_stopped (AnimationPlayback *playback,
                     AnimationXSheet   *xsheet)
{
  animation_xsheet_jump (xsheet,
                         animation_playback_get_position (playback));
}

static void
on_playback_rendered (AnimationPlayback *playback,
                      gint               frame_number,
                      GeglBuffer        *buffer,
                      gboolean           must_draw_null,
                      AnimationXSheet   *xsheet)
{
  if (animation_loaded (ANIMATION (xsheet->priv->animation)))
    animation_xsheet_jump (xsheet, frame_number);
}

static void
animation_xsheet_suite_do (GtkWidget       *button,
                           AnimationXSheet *xsheet)
{
  const GList  *selection;
  GTree        *suite = NULL;
  GList        *cels;
  GtkWidget    *cel;
  GRegex       *regex;
  CreateData    data;
  gint32        image_id;
  gint          position;
  gint          level;
  gint          i;

  image_id = animation_get_image_id (ANIMATION (xsheet->priv->animation));

  position = xsheet->priv->suite_position;
  level = xsheet->priv->suite_level;

  selection = animation_cel_animation_get_layers (xsheet->priv->animation,
                                                  level, position);

  if (selection)
    {
      gchar    *prev_key = NULL;
      GList    *iter;
      gboolean  same_numbering = TRUE;

      /* Using current selection as base. */
      regex = g_regex_new ("^(.*[^0-9_ -])[ \t_-]*([0-9]+([- _][0-9]+)*)[ \t]*([^0-9]*)[ \t]*$",
                           0, 0, NULL);

      /* First pass: check if selected layers use same numbering. */
      for (iter = (GList *) selection; iter; iter = iter->next)
        {
          GMatchInfo *match;
          gchar      *layer_title;
          gint32      layer;

          layer = gimp_image_get_layer_by_tattoo (image_id,
                                                  GPOINTER_TO_INT (iter->data));
          layer_title = gimp_item_get_name (layer);

          if (g_regex_match (regex, layer_title, 0, &match))
            {
              gchar **tokens;
              gchar  *num;
              gchar  *key;

              num = g_match_info_fetch (match, 2);
              tokens = g_strsplit_set (num, " -_", -1);
              key = g_strjoinv ("-", tokens);

              g_strfreev (tokens);
              g_free (num);

              if (! prev_key)
                {
                  prev_key = key;
                }
              else if (g_strcmp0 (prev_key, key) != 0)
                {
                  g_free (key);
                  same_numbering = FALSE;
                  break;
                }
              else
                {
                  g_free (key);
                }
            }

          g_match_info_free (match);
          g_free (layer_title);
        }
      g_free (prev_key);

      /* Second pass: search compatible layers. */
      for (i = 0; selection; selection = selection->next, i++)
        {
          GMatchInfo *match;
          gchar      *layer_title;
          gchar      *title_prefix;
          gchar      *title_suffix;
          gint       *lower_key = NULL;
          gint32      layer;

          layer = gimp_image_get_layer_by_tattoo (image_id,
                                                  GPOINTER_TO_INT (selection->data));
          layer_title = gimp_item_get_name (layer);

          if (g_regex_match (regex, layer_title, 0, &match))
            {
              gchar **tokens;
              gchar  *title_key;
              gint    k;

              title_prefix = g_match_info_fetch (match, 1);
              title_key    = g_match_info_fetch (match, 2);
              title_suffix = g_match_info_fetch (match, 4);

              tokens = g_strsplit_set (title_key, " -_", -1);
              lower_key = g_new0 (int, g_strv_length (tokens) + 1);

              for (k = 0; k < g_strv_length (tokens); k++)
                {
                  lower_key[k] = (gint) g_ascii_strtoll (tokens[k], NULL, 10);
                }
              lower_key[g_strv_length (tokens)] = -1;

              g_free (title_key);
              g_strfreev (tokens);
            }
          else
            {
              title_prefix = g_strdup (layer_title);
              title_suffix = g_strdup ("");
            }
          animation_xsheet_extract_layer_suite (xsheet, &suite,
                                                same_numbering,
                                                title_prefix, title_suffix,
                                                lower_key);
          g_free (title_prefix);
          g_free (title_suffix);
          g_free (lower_key);
          g_match_info_free (match);
          g_free (layer_title);
        }
    }
  else
    {
      const gchar *track_title;

      track_title = animation_cel_animation_get_track_title (xsheet->priv->animation,
                                                             level);
      animation_xsheet_extract_layer_suite (xsheet, &suite, FALSE,
                                            track_title, "", NULL);
    }

  /*Create the actual suite of frames. */
  data.xsheet = xsheet;
  data.level = level;
  data.suite_length = g_tree_nnodes (suite) * xsheet->priv->suite_fpi;

  data.end_of_animation = FALSE;
  data.index = 0;

  for (data.loop_idx = 0; xsheet->priv->suite_cycle == 0 || data.loop_idx < xsheet->priv->suite_cycle; data.loop_idx++)
    {
      g_tree_foreach (suite, (GTraverseFunc) create_suite, &data);
      if (data.end_of_animation)
        break;
    }

  /* Cleaning. */
  g_tree_destroy (suite);

  /* Rename the cels. */
  cels = g_list_nth_data (xsheet->priv->cels, level);
  cel = g_list_nth_data (cels, position);

  animation_xsheet_rename_cel (xsheet, cel, TRUE, -1);

  /* Delete the widget. */
  animation_xsheet_suite_cancelled (NULL, xsheet);
}

static void
animation_xsheet_suite_cancelled (GtkWidget       *button,
                                  AnimationXSheet *xsheet)
{
  gtk_widget_destroy (xsheet->priv->suite_box);
  xsheet->priv->suite_box = NULL;
  gtk_widget_show (xsheet->priv->suite_button);
  xsheet->priv->suite_button = NULL;
}

static gboolean
animation_xsheet_effect_clicked (GtkWidget       *button,
                                 GdkEvent        *event,
                                 AnimationXSheet *xsheet)
{
  gpointer position;

  position = g_object_get_data (G_OBJECT (button), "frame-position");

  animation_keyframe_view_show (ANIMATION_KEYFRAME_VIEW (xsheet->priv->keyframe_view),
                                ANIMATION_CEL_ANIMATION (xsheet->priv->animation),
                                GPOINTER_TO_INT (position));

  /* All handled here. */
  return TRUE;
}

static gboolean
animation_xsheet_frame_clicked (GtkWidget       *button,
                                GdkEvent        *event,
                                AnimationXSheet *xsheet)
{
  gpointer position;

  position = g_object_get_data (G_OBJECT (button), "frame-position");

  if (event->type == GDK_BUTTON_PRESS)
    {
      /* Single click: jump to the position. */
      animation_playback_jump (xsheet->priv->playback,
                               GPOINTER_TO_INT (position));
      if (xsheet->priv->active_pos_button)
        {
          GtkToggleButton *active_button;

          active_button = GTK_TOGGLE_BUTTON (xsheet->priv->active_pos_button);
          gtk_toggle_button_set_active (active_button, FALSE);
        }
      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button), TRUE);
      xsheet->priv->active_pos_button = button;
    }
  else if (event->type == GDK_2BUTTON_PRESS)
    {
      /* Double click: update GIMP view. */
      animation_update_paint_view (ANIMATION (xsheet->priv->animation),
                                   GPOINTER_TO_INT (position));
    }

  /* All handled here. */
  return TRUE;
}

static gboolean
animation_xsheet_cel_clicked (GtkWidget       *button,
                              GdkEventButton  *event,
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
  shift   = event->state & GDK_SHIFT_MASK;
  ctrl    = event->state & GDK_CONTROL_MASK;

  /* Left click */
  if (event->button == 1)
    {
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
          const GList *layers;

          /* When several frames are selected, show layers of the first selected. */
          position = g_queue_peek_tail (xsheet->priv->selected_frames);

          layers = animation_cel_animation_get_layers (xsheet->priv->animation,
                                                       GPOINTER_TO_INT (track_num),
                                                       GPOINTER_TO_INT (position));
          animation_layer_view_select (ANIMATION_LAYER_VIEW (xsheet->priv->layer_view),
                                       layers);
        }
    }
  /* Middle click */
  else if (event->button == 2)
    {
      GtkWidget *widget;
      GtkWidget *image;
      gint       level = GPOINTER_TO_INT (track_num);
      gint       pos   = GPOINTER_TO_INT (position);

      if (xsheet->priv->suite_box)
        {
          gtk_widget_show (xsheet->priv->suite_button);
          gtk_widget_destroy (xsheet->priv->suite_box);
        }

      xsheet->priv->suite_box = gtk_table_new (2, 3, FALSE);
      xsheet->priv->suite_position = pos;
      xsheet->priv->suite_level = level;

      g_object_set_data (G_OBJECT (xsheet->priv->suite_box),
                         "frame-position", position);
      g_object_set_data (G_OBJECT (xsheet->priv->suite_box),
                         "track-num", track_num);

      /* Frame per image label */
      widget = gtk_label_new (_("On:"));
      gimp_help_set_help_data (widget,
                               _("Number of frames per image"),
                               NULL);
      gtk_table_attach (GTK_TABLE (xsheet->priv->suite_box),
                        widget, 0, 1, 0, 1,
                        GTK_EXPAND, GTK_SHRINK, 2, 0);
      gtk_widget_show (widget);

      /* Frame per image spin */
      widget = gimp_prop_spin_button_new (G_OBJECT (xsheet),
                                          "suite-frames-per-image",
                                          1.0, 2.0, 0.0);
      gtk_spin_button_set_range (GTK_SPIN_BUTTON (widget),
                                 1.0, 100.0);
      gtk_entry_set_width_chars (GTK_ENTRY (widget), 2);
      gtk_table_attach (GTK_TABLE (xsheet->priv->suite_box),
                        widget, 1, 2, 0, 1,
                        GTK_SHRINK, GTK_SHRINK, 2, 0);
      gtk_widget_show (widget);

      /* Cycle label */
      widget = gtk_label_new (_("Cycle:"));
      gimp_help_set_help_data (widget,
                               _("Number of times the whole cycle repeats"
                                 " (0 means indefinitely)"),
                               NULL);
      gtk_table_attach (GTK_TABLE (xsheet->priv->suite_box),
                        widget, 0, 1, 1, 2,
                        GTK_EXPAND, GTK_SHRINK, 2, 0);
      gtk_widget_show (widget);

      /* Cycle spin */
      widget = gimp_prop_spin_button_new (G_OBJECT (xsheet),
                                          "suite-cycle",
                                          1.0, 5.0, 0.0);
      gtk_entry_set_width_chars (GTK_ENTRY (widget), 2);
      gtk_table_attach (GTK_TABLE (xsheet->priv->suite_box),
                        widget, 1, 2, 1, 2,
                        GTK_SHRINK, GTK_SHRINK, 2, 0);
      gtk_widget_show (widget);

      /* Ok button */
      widget = gtk_button_new ();

      image = gtk_image_new_from_icon_name ("gtk-ok",
                                            GTK_ICON_SIZE_SMALL_TOOLBAR);
      gtk_container_add (GTK_CONTAINER (widget),
                         image);
      gtk_table_attach (GTK_TABLE (xsheet->priv->suite_box),
                        widget, 2, 3, 0, 1,
                        GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL, 2, 0);
      g_signal_connect (widget, "clicked",
                        G_CALLBACK (animation_xsheet_suite_do),
                        xsheet);
      gtk_widget_show (image);
      gtk_widget_show (widget);

      /* Cancel button */
      widget = gtk_button_new ();

      image = gtk_image_new_from_icon_name ("gtk-cancel",
                                            GTK_ICON_SIZE_SMALL_TOOLBAR);
      gtk_container_add (GTK_CONTAINER (widget),
                         image);
      gtk_table_attach (GTK_TABLE (xsheet->priv->suite_box),
                        widget, 2, 3, 1, 2,
                        GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL, 2, 0);
      g_signal_connect (widget, "clicked",
                        G_CALLBACK (animation_xsheet_suite_cancelled),
                        xsheet);
      gtk_widget_show (image);
      gtk_widget_show (widget);

      /* Finalize */
      gtk_table_attach (GTK_TABLE (xsheet->priv->track_layout),
                        xsheet->priv->suite_box,
                        level * 9 + 2, level * 9 + 11,
                        pos + 2, pos + 3,
                        GTK_FILL, GTK_FILL, 0, 0);
      gtk_widget_hide (button);
      xsheet->priv->suite_button = button;
      gtk_widget_show (xsheet->priv->suite_box);
    }
  else if (event->type == GDK_BUTTON_RELEASE && event->button == 3)
    {
      animation_menu_cell (xsheet->priv->animation,
                           event,
                           GPOINTER_TO_INT (position),
                           GPOINTER_TO_INT (track_num));
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
      animation_xsheet_rename_cel (xsheet, cel, FALSE, -1);
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
  gpointer track_num;

  track_num = g_object_get_data (G_OBJECT (button), "track-num");
  g_signal_handlers_disconnect_by_func (xsheet->priv->animation,
                                        G_CALLBACK (on_animation_loaded),
                                        xsheet);
  if (animation_cel_animation_level_add (xsheet->priv->animation,
                                         GPOINTER_TO_INT (track_num) + 1))
    {
      animation_xsheet_add_track (xsheet, GPOINTER_TO_INT (track_num) + 1);
    }
  g_signal_connect_after (xsheet->priv->animation, "loaded",
                          G_CALLBACK (on_animation_loaded), xsheet);
}

static void
on_track_delete_clicked (GtkToolButton   *button,
                         AnimationXSheet *xsheet)
{
  gpointer track_num;

  track_num = g_object_get_data (G_OBJECT (button), "track-num");
  g_signal_handlers_disconnect_by_func (xsheet->priv->animation,
                                        G_CALLBACK (on_animation_loaded),
                                        xsheet);
  if (animation_cel_animation_level_delete (xsheet->priv->animation,
                                            GPOINTER_TO_INT (track_num)))
    {
      animation_xsheet_remove_track (xsheet,
                                     GPOINTER_TO_INT (track_num));
    }
  g_signal_connect_after (xsheet->priv->animation, "loaded",
                          G_CALLBACK (on_animation_loaded), xsheet);
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
                             gboolean         recursively,
                             gint             stop_position)
{
  const GList *layers;
  const GList *prev_layers = NULL;
  gpointer     track_num;
  gpointer     position;
  gboolean     same_as_prev = FALSE;
  GtkWidget   *label        = NULL;
  gchar       *markup       = NULL;

  track_num = g_object_get_data (G_OBJECT (cel), "track-num");
  position  = g_object_get_data (G_OBJECT (cel), "frame-position");

  layers = animation_cel_animation_get_layers (xsheet->priv->animation,
                                               GPOINTER_TO_INT (track_num),
                                               GPOINTER_TO_INT (position));
  if (position > 0)
    prev_layers = animation_cel_animation_get_layers (xsheet->priv->animation,
                                                      GPOINTER_TO_INT (track_num),
                                                      GPOINTER_TO_INT (position) - 1);

  /* Remove the previous label, if any. */
  if (gtk_bin_get_child (GTK_BIN (cel)))
    gtk_container_remove (GTK_CONTAINER (cel),
                          gtk_bin_get_child (GTK_BIN (cel)));

  if (layers && prev_layers &&
      g_list_length ((GList *) layers) == g_list_length ((GList *) prev_layers))
    {
      GList *iter1 = (GList *) layers;
      GList *iter2 = (GList *) prev_layers;

      same_as_prev = TRUE;
      for (; iter1; iter1 = iter1->next, iter2 = iter2->next)
        {
          if (iter1->data != iter2->data)
            {
              same_as_prev = FALSE;
              break;
            }
        }
    }
  if (same_as_prev)
    {
      markup = g_strdup_printf ("<b>-</b>");
    }
  else if (layers)
    {
      const gchar *track_title;
      const gchar *ellipsis;
      gchar       *layer_title = NULL;
      gint32       image_id;
      gint32       layer;

      ellipsis = g_list_length ((GList *) layers) > 1 ?
        " <u>\xe2\x80\xa6</u>": "";

      track_title = animation_cel_animation_get_track_title (xsheet->priv->animation,
                                                             GPOINTER_TO_INT (track_num));
      image_id = animation_get_image_id (ANIMATION (xsheet->priv->animation));
      layer = gimp_image_get_layer_by_tattoo (image_id,
                                              GPOINTER_TO_INT (layers->data));
      if (layer <= 0)
        {
          g_printerr ("Warning: removing invalid layer from cell: level %d, frame %d.\n",
                      GPOINTER_TO_INT (track_num),
                      GPOINTER_TO_INT (position));
          /* Update the layer list on this cell. */
          animation_cel_animation_set_layers (xsheet->priv->animation,
                                              GPOINTER_TO_INT (track_num),
                                              GPOINTER_TO_INT (position),
                                              NULL);
          animation_xsheet_rename_cel (xsheet, cel, recursively, stop_position);
          return;
        }
      else
        {
          layer_title = gimp_item_get_name (layer);
        }
      if (g_strcmp0 (layer_title, track_title) == 0)
        {
          /* If the track and the layer have exactly the same name. */
          markup = g_strdup_printf ("<u>*</u>%s", ellipsis);
        }
      else if (g_str_has_prefix (layer_title, track_title))
        {
          /* The layer is prefixed with the track name. */
          gchar *suffix = layer_title + strlen (track_title);

          while (*suffix)
            {
              if (*suffix != ' ' && *suffix != '\t' &&
                  *suffix != '-' && *suffix != '_')
                break;

              suffix++;
            }
          if (strlen (suffix) == 0)
            suffix = layer_title + strlen (track_title);

          markup = g_strdup_printf ("<i>%s</i>%s",
                                    g_markup_escape_text (suffix, -1),
                                    ellipsis);
        }
      else
        {
          /* Completely different names. */
          markup = g_strdup_printf ("%s%s",
                                    g_markup_escape_text (layer_title, -1),
                                    ellipsis);
        }
      g_free (layer_title);
    }

  if (markup)
    {
      label = gtk_label_new (NULL);
      gtk_label_set_markup (GTK_LABEL (label), markup);
      gtk_container_add (GTK_CONTAINER (cel), label);
      gtk_widget_show (label);

      g_free (markup);
    }

  if (recursively &&
      GPOINTER_TO_INT (position) + 1 != stop_position)
    {
      GList     *track_cels;
      GtkWidget *next_cel;

      track_cels = g_list_nth_data (xsheet->priv->cels,
                                    GPOINTER_TO_INT (track_num));
      next_cel = g_list_nth_data (track_cels,
                                  GPOINTER_TO_INT (position) + 1);
      if (next_cel)
        {
          animation_xsheet_rename_cel (xsheet, next_cel, TRUE,
                                       stop_position);
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

static void
animation_xsheet_attach_cel (AnimationXSheet *xsheet,
                             GtkWidget       *cel,
                             gint             track,
                             gint             pos)
{
  GtkWidget *frame;
  gboolean   attached = FALSE;

  g_object_set_data (G_OBJECT (cel), "frame-position",
                     GINT_TO_POINTER (pos));
  g_object_set_data (G_OBJECT (cel), "track-num",
                     GINT_TO_POINTER (track));

  frame = gtk_widget_get_parent (cel);
  if (frame)
    {
      /* Already attached. Detach first. */
      g_object_ref (frame);
      gtk_container_remove (GTK_CONTAINER (xsheet->priv->track_layout),
                            frame);
      attached = TRUE;
    }
  else
    {
      frame = gtk_frame_new (NULL);
      gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_IN);
      gtk_container_add (GTK_CONTAINER (frame), cel);
    }
  gtk_table_attach (GTK_TABLE (xsheet->priv->track_layout),
                    frame, track * 9 + 2, track * 9 + 11,
                    pos + 2, pos + 3,
                    GTK_FILL, GTK_FILL, 0, 0);
  gtk_widget_show (frame);
  if (attached)
    g_object_unref (frame);
}

static void
animation_xsheet_extract_layer_suite (AnimationXSheet  *xsheet,
                                      GTree           **suite,
                                      gboolean          same_numbering,
                                      const gchar      *prefix,
                                      const gchar      *suffix,
                                      const gint       *lower_key)
{
  GRegex *regex;
  GTree  *temp_suite;
  gchar  *esc_prefix;
  gchar  *esc_suffix;
  gchar  *regex_string;
  gint   *layers;
  gint32  image_id;
  gint    num_layers;
  gint    i;

  /* Create the suite if not done yet. */
  if ((*suite) == NULL)
    *suite = g_tree_new_full ((GCompareDataFunc) compare_keys,
                              NULL,
                              (GDestroyNotify) g_free,
                              (GDestroyNotify) g_list_free);
  if (same_numbering)
    temp_suite = *suite;
  else
    /* Sequential numbering. */
    temp_suite = g_tree_new_full ((GCompareDataFunc) compare_keys,
                                  NULL,
                                  (GDestroyNotify) g_free,
                                  (GDestroyNotify) g_list_free);

  /* Escape characters used in regular expressions. */
  esc_prefix = g_regex_escape_string (prefix, -1);
  esc_suffix = g_regex_escape_string (suffix, -1);

  regex_string = g_strdup_printf ("^%s[ \t_-]*([0-9]*(([- _][0-9]+)*)?)[ \t]*%s[ \t]*$",
                                  esc_prefix, esc_suffix);
  regex = g_regex_new (regex_string, 0, 0, NULL);
  g_free (esc_prefix);
  g_free (esc_suffix);
  g_free (regex_string);

  /* Loop through the image layers. */
  image_id = animation_get_image_id (ANIMATION (xsheet->priv->animation));
  layers = gimp_image_get_layers (image_id, &num_layers);
  for (i = 0; i < num_layers; i++)
    {
      GMatchInfo *match;
      gchar      *layer_title;

      layer_title = gimp_item_get_name (layers[i]);
      if (g_regex_match (regex, layer_title, 0, &match))
        {
          gchar **tokens;
          gint   *key;
          gchar  *num;
          gint    k;

          /* Get the key. */
          num    = g_match_info_fetch (match, 1);
          tokens = g_strsplit_set (num, " -_", -1);
          key    = g_new0 (int, g_strv_length (tokens) + 1);

          for (k = 0; k < g_strv_length (tokens); k++)
            {
              key[k] = (gint) g_ascii_strtoll (tokens[k], NULL, 10);
            }
          key[g_strv_length (tokens)] = -1;

          if (! lower_key || compare_keys (key,
                                           (gint *) lower_key) >= 0)
            {
              GList *new_layers;
              gint   tattoo;

              tattoo = gimp_item_get_tattoo (GPOINTER_TO_INT (layers[i]));

              new_layers = g_tree_lookup (temp_suite, key);
              new_layers = g_list_prepend (g_list_copy (new_layers),
                                           GINT_TO_POINTER (tattoo));
              g_tree_replace (temp_suite, key, new_layers);
            }
          else
            {
              g_free (key);
            }
          g_strfreev (tokens);
          g_free (num);
        }

      g_free (layer_title);
      g_match_info_free (match);
    }

  if (! same_numbering)
    {
      /* We ordered the layers first in a separate list, then later
       * we merge, keeping the order. */
      MergeData data;

      data.count = 0;
      data.suite = *suite;

      g_tree_foreach (temp_suite, (GTraverseFunc) merge_suites,
                      &data);
      g_tree_destroy (temp_suite);
    }

  g_regex_unref (regex);
  g_free (layers);
}

static gboolean
create_suite (gint       *key,
              GList      *layers,
              CreateData *data)
{
  AnimationXSheet *xsheet = data->xsheet;
  gint             selected_pos;
  gint             i;

  selected_pos = GPOINTER_TO_INT (g_queue_peek_tail (xsheet->priv->selected_frames));

  for (i = 0; i < xsheet->priv->suite_fpi; i++)
    {
      gint pos;

      pos = xsheet->priv->suite_position              +
            (data->index * xsheet->priv->suite_fpi) +
            i + data->suite_length * data->loop_idx;
      if (pos >= animation_get_duration (ANIMATION (xsheet->priv->animation)))
        {
          data->end_of_animation = TRUE;
          return TRUE;
        }

      animation_cel_animation_set_layers (xsheet->priv->animation,
                                          data->level, pos, layers);
      /* If currently selected position, refresh the layer view. */
      if (xsheet->priv->selected_track == data->level &&
          selected_pos == pos)
        {
          animation_layer_view_select (ANIMATION_LAYER_VIEW (xsheet->priv->layer_view),
                                       layers);
        }
    }

  data->index++;
  return FALSE;
}

static gboolean
merge_suites (gint      *key,
              GList     *layers,
              MergeData *data)
{
  gint  *new_key = g_new0 (int, 2);
  GList *new_layers;

  new_key[0] = data->count;
  new_key[1] = -1;

  new_layers = g_tree_lookup (data->suite, new_key);
  new_layers = g_list_concat (g_list_copy (new_layers),
                              g_list_copy (layers));
  g_tree_replace (data->suite, new_key,
                  new_layers);
  data->count++;

  /* Continue traversal. */
  return FALSE;
}

static gint
compare_keys (gint *key1,
              gint *key2)
{
  gint retval = 0;
  gint i = 0;

  while (TRUE)
    {
      gint num1 = key1[i];
      gint num2 = key2[i];

      if (num1 >= 0 && num2 < 0)
        {
          retval = 1;
          break;
        }
      else if (num2 >= 0 && num1 < 0)
        {
          retval = -1;
          break;
        }
      else if (num2 < 0 && num1 < 0)
        {
          retval = 0;
          break;
        }
      else if (num1 > num2)
        {
          retval = 1;
          break;
        }
      else if (num1 < num2)
        {
          retval = -1;
          break;
        }
      /* In case of equal numbers, continue. */
      i++;
    }

  return retval;
}
