/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * animation-keyframe_view.c
 * Copyright (C) 2016-2017 Jehan <jehan@gimp.org>
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

#include "core/animation.h"
#include "core/animation-camera.h"
#include "core/animation-celanimation.h"

#include "animation-keyframe-view.h"

struct _AnimationKeyFrameViewPrivate
{
  AnimationCamera *camera;
  gint             position;

  GtkWidget       *offset_entry;
};

/* GObject handlers */
static void animation_keyframe_view_constructed  (GObject               *object);

static void on_offset_entry_changed              (GimpSizeEntry         *entry,
                                                  AnimationKeyFrameView *view);
static void on_offsets_changed                   (AnimationCamera       *camera,
                                                  gint                   position,
                                                  gint                   duration,
                                                  AnimationKeyFrameView *view);

G_DEFINE_TYPE (AnimationKeyFrameView, animation_keyframe_view, GTK_TYPE_NOTEBOOK)

#define parent_class animation_keyframe_view_parent_class

static void
animation_keyframe_view_class_init (AnimationKeyFrameViewClass *klass)
{
  GObjectClass   *object_class = G_OBJECT_CLASS (klass);

  object_class->constructed  = animation_keyframe_view_constructed;

  g_type_class_add_private (klass, sizeof (AnimationKeyFrameViewPrivate));
}

static void
animation_keyframe_view_init (AnimationKeyFrameView *view)
{
  view->priv = G_TYPE_INSTANCE_GET_PRIVATE (view,
                                            ANIMATION_TYPE_KEYFRAME_VIEW,
                                            AnimationKeyFrameViewPrivate);
}

/************ Public Functions ****************/

/**
 * animation_keyframe_view_new:
 *
 * Creates a new layer view. You should not show it with
 * gtk_widget_show() but with animation_keyframe_view_show() instead.
 */
GtkWidget *
animation_keyframe_view_new ()
{
  GtkWidget *view;

  view = g_object_new (ANIMATION_TYPE_KEYFRAME_VIEW,
                       NULL);

  return view;
}

/**
 * animation_keyframe_view_show:
 * @view: the #AnimationKeyFrameView.
 * @animation: the #Animation.
 * @position:
 *
 * Show the @view widget, displaying the keyframes set on
 * @animation at @position.
 */
void
animation_keyframe_view_show (AnimationKeyFrameView *view,
                              AnimationCelAnimation *animation,
                              gint                   position)
{
  AnimationCamera *camera;
  gint32           image_id;
  gint             width;
  gint             height;
  gdouble          xres;
  gdouble          yres;
  gint             x_offset;
  gint             y_offset;

  camera = ANIMATION_CAMERA (animation_cel_animation_get_main_camera (animation));

  view->priv->camera   = camera;
  view->priv->position = position;

  image_id = animation_get_image_id (ANIMATION (animation));
  gimp_image_get_resolution (image_id, &xres, &yres);

  animation_get_size (ANIMATION (animation), &width, &height);
  gimp_size_entry_set_refval_boundaries (GIMP_SIZE_ENTRY (view->priv->offset_entry),
                                         0, (gdouble) -width, (gdouble) width);
  gimp_size_entry_set_refval_boundaries (GIMP_SIZE_ENTRY (view->priv->offset_entry),
                                         1, (gdouble) -height, (gdouble) height);
  gimp_size_entry_set_size (GIMP_SIZE_ENTRY (view->priv->offset_entry),
                            0, 0.0, (gdouble) width);
  gimp_size_entry_set_size (GIMP_SIZE_ENTRY (view->priv->offset_entry),
                            1, 0.0, (gdouble) height);
  gimp_size_entry_set_resolution (GIMP_SIZE_ENTRY (view->priv->offset_entry),
                            0, xres, TRUE);
  gimp_size_entry_set_resolution (GIMP_SIZE_ENTRY (view->priv->offset_entry),
                            1, yres, TRUE);

  g_signal_handlers_disconnect_by_func (view->priv->offset_entry,
                                        G_CALLBACK (on_offset_entry_changed),
                                        view);
  g_signal_handlers_disconnect_by_func (view->priv->camera,
                                        G_CALLBACK (on_offsets_changed),
                                        view);
  animation_camera_get (camera, position, &x_offset, &y_offset);
  gimp_size_entry_set_value (GIMP_SIZE_ENTRY (view->priv->offset_entry),
                             0, (gdouble) x_offset);
  gimp_size_entry_set_value (GIMP_SIZE_ENTRY (view->priv->offset_entry),
                             1, (gdouble) y_offset);
  g_signal_connect (view->priv->offset_entry, "value-changed",
                    G_CALLBACK (on_offset_entry_changed),
                    view);
  g_signal_connect (camera, "offsets-changed",
                    G_CALLBACK (on_offsets_changed),
                    view);
  gtk_widget_show (GTK_WIDGET (view));
}

void
animation_keyframe_view_hide (AnimationKeyFrameView *view)
{
  g_signal_handlers_disconnect_by_func (view->priv->offset_entry,
                                        G_CALLBACK (on_offset_entry_changed),
                                        view);
  g_signal_handlers_disconnect_by_func (view->priv->camera,
                                        G_CALLBACK (on_offsets_changed),
                                        view);
}

/************ Private Functions ****************/

static void
animation_keyframe_view_constructed (GObject *object)
{
  AnimationKeyFrameView *view = ANIMATION_KEYFRAME_VIEW (object);
  GtkWidget             *page;
  GtkWidget             *label;

  page = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
  label = gtk_image_new_from_icon_name ("camera-video",
                                        GTK_ICON_SIZE_SMALL_TOOLBAR);
  gtk_notebook_append_page (GTK_NOTEBOOK (view), page,
                            label);

  view->priv->offset_entry = gimp_size_entry_new (2, GIMP_UNIT_PIXEL, NULL,
                                                  TRUE, TRUE, FALSE, 5,
                                                  GIMP_SIZE_ENTRY_UPDATE_SIZE);
  gimp_size_entry_attach_label (GIMP_SIZE_ENTRY (view->priv->offset_entry),
                                _("Horizontal offset:"), 0, 1, 0.0);
  gimp_size_entry_attach_label (GIMP_SIZE_ENTRY (view->priv->offset_entry),
                                _("Vertical offset:"), 0, 2, 0.0);
  gimp_size_entry_set_pixel_digits (GIMP_SIZE_ENTRY (view->priv->offset_entry), 0);
  gtk_box_pack_start (GTK_BOX (page), view->priv->offset_entry, FALSE, FALSE, 0);
  gtk_widget_show (view->priv->offset_entry);

  gtk_widget_show (page);
}

static void
on_offset_entry_changed (GimpSizeEntry         *entry,
                         AnimationKeyFrameView *view)
{
  gdouble x_offset;
  gdouble y_offset;

  x_offset = gimp_size_entry_get_refval (GIMP_SIZE_ENTRY (view->priv->offset_entry), 0);
  y_offset = gimp_size_entry_get_refval (GIMP_SIZE_ENTRY (view->priv->offset_entry), 1);
  animation_camera_set_keyframe (view->priv->camera,
                                 view->priv->position,
                                 x_offset, y_offset);
}

static void
on_offsets_changed (AnimationCamera       *camera,
                    gint                   position,
                    gint                   duration,
                    AnimationKeyFrameView *view)
{
  if (view->priv->position >= position &&
      view->priv->position < position + duration)
    {
      gint x_offset;
      gint y_offset;

      g_signal_handlers_block_by_func (view->priv->offset_entry,
                                       G_CALLBACK (on_offset_entry_changed),
                                       view);
      animation_camera_get (camera, position, &x_offset, &y_offset);
      gimp_size_entry_set_value (GIMP_SIZE_ENTRY (view->priv->offset_entry),
                                 0, (gdouble) x_offset);
      gimp_size_entry_set_value (GIMP_SIZE_ENTRY (view->priv->offset_entry),
                                 1, (gdouble) y_offset);
      g_signal_handlers_unblock_by_func (view->priv->offset_entry,
                                         G_CALLBACK (on_offset_entry_changed),
                                         view);
    }
}
