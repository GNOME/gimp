/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimptoolwidgetgroup.c
 * Copyright (C) 2018 Ell
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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <gegl.h>
#include <gtk/gtk.h>

#include "display-types.h"

#include "core/gimpcontainer.h"
#include "core/gimplist.h"

#include "gimpcanvasgroup.h"
#include "gimpdisplayshell.h"
#include "gimptoolwidgetgroup.h"


struct _GimpToolWidgetGroupPrivate
{
  GimpContainer  *children;

  GimpToolWidget *focus_widget;
  GimpToolWidget *hover_widget;

  gboolean        auto_raise;
};


/*  local function prototypes  */

static void             gimp_tool_widget_group_finalize            (GObject               *object);

static void             gimp_tool_widget_group_focus_changed       (GimpToolWidget        *widget);
static gint             gimp_tool_widget_group_button_press        (GimpToolWidget        *widget,
                                                                    const GimpCoords      *coords,
                                                                    guint32                time,
                                                                    GdkModifierType        state,
                                                                    GimpButtonPressType    press_type);
static void             gimp_tool_widget_group_button_release      (GimpToolWidget        *widget,
                                                                    const GimpCoords      *coords,
                                                                    guint32                time,
                                                                    GdkModifierType        state,
                                                                    GimpButtonReleaseType  release_type);
static void             gimp_tool_widget_group_motion              (GimpToolWidget        *widget,
                                                                    const GimpCoords      *coords,
                                                                    guint32                time,
                                                                    GdkModifierType        state);
static GimpHit          gimp_tool_widget_group_hit                 (GimpToolWidget        *widget,
                                                                    const GimpCoords      *coords,
                                                                    GdkModifierType        state,
                                                                    gboolean               proximity);
static void             gimp_tool_widget_group_hover               (GimpToolWidget        *widget,
                                                                    const GimpCoords      *coords,
                                                                    GdkModifierType        state,
                                                                    gboolean               proximity);
static void             gimp_tool_widget_group_leave_notify        (GimpToolWidget        *widget);
static gboolean         gimp_tool_widget_group_key_press           (GimpToolWidget        *widget,
                                                                    GdkEventKey           *kevent);
static gboolean         gimp_tool_widget_group_key_release         (GimpToolWidget        *widget,
                                                                    GdkEventKey           *kevent);
static void             gimp_tool_widget_group_motion_modifier     (GimpToolWidget        *widget,
                                                                    GdkModifierType        key,
                                                                    gboolean               press,
                                                                    GdkModifierType        state);
static void             gimp_tool_widget_group_hover_modifier      (GimpToolWidget        *widget,
                                                                    GdkModifierType        key,
                                                                    gboolean               press,
                                                                    GdkModifierType        state);
static gboolean         gimp_tool_widget_group_get_cursor          (GimpToolWidget        *widget,
                                                                    const GimpCoords      *coords,
                                                                    GdkModifierType        state,
                                                                    GimpCursorType        *cursor,
                                                                    GimpToolCursorType    *tool_cursor,
                                                                    GimpCursorModifier    *modifier);

static void             gimp_tool_widget_group_children_add        (GimpContainer         *container,
                                                                    GimpToolWidget        *child,
                                                                    GimpToolWidgetGroup   *group);
static void             gimp_tool_widget_group_children_remove     (GimpContainer         *container,
                                                                    GimpToolWidget        *child,
                                                                    GimpToolWidgetGroup   *group);
static void             gimp_tool_widget_group_children_reorder    (GimpContainer         *container,
                                                                    GimpToolWidget        *child,
                                                                    gint                   new_index,
                                                                    GimpToolWidgetGroup   *group);

static void             gimp_tool_widget_group_child_changed       (GimpToolWidget        *child,
                                                                    GimpToolWidgetGroup   *group);
static void             gimp_tool_widget_group_child_response      (GimpToolWidget        *child,
                                                                    gint                   response_id,
                                                                    GimpToolWidgetGroup   *group);
static void             gimp_tool_widget_group_child_snap_offsets  (GimpToolWidget        *child,
                                                                    gint                   offset_x,
                                                                    gint                   offset_y,
                                                                    gint                   width,
                                                                    gint                   height,
                                                                    GimpToolWidgetGroup   *group);
static void             gimp_tool_widget_group_child_status        (GimpToolWidget        *child,
                                                                    const gchar           *status,
                                                                    GimpToolWidgetGroup   *group);
static void             gimp_tool_widget_group_child_status_coords (GimpToolWidget        *child,
                                                                    const gchar           *title,
                                                                    gdouble                x,
                                                                    const gchar           *separator,
                                                                    gdouble                y,
                                                                    const gchar           *help,
                                                                    GimpToolWidgetGroup   *group);
static void             gimp_tool_widget_group_child_message       (GimpToolWidget        *child,
                                                                    const gchar           *message,
                                                                    GimpToolWidgetGroup   *group);
static void             gimp_tool_widget_group_child_focus_changed (GimpToolWidget        *child,
                                                                    GimpToolWidgetGroup   *group);

static GimpToolWidget * gimp_tool_widget_group_get_hover_widget    (GimpToolWidgetGroup   *group,
                                                                    const GimpCoords      *coords,
                                                                    GdkModifierType        state,
                                                                    gboolean               proximity,
                                                                    GimpHit               *hit);


G_DEFINE_TYPE_WITH_PRIVATE (GimpToolWidgetGroup, gimp_tool_widget_group,
                            GIMP_TYPE_TOOL_WIDGET)

#define parent_class gimp_tool_widget_group_parent_class


/*  priv functions  */


static void
gimp_tool_widget_group_class_init (GimpToolWidgetGroupClass *klass)
{
  GObjectClass        *object_class = G_OBJECT_CLASS (klass);
  GimpToolWidgetClass *widget_class = GIMP_TOOL_WIDGET_CLASS (klass);

  object_class->finalize        = gimp_tool_widget_group_finalize;

  widget_class->focus_changed   = gimp_tool_widget_group_focus_changed;
  widget_class->button_press    = gimp_tool_widget_group_button_press;
  widget_class->button_release  = gimp_tool_widget_group_button_release;
  widget_class->motion          = gimp_tool_widget_group_motion;
  widget_class->hit             = gimp_tool_widget_group_hit;
  widget_class->hover           = gimp_tool_widget_group_hover;
  widget_class->leave_notify    = gimp_tool_widget_group_leave_notify;
  widget_class->key_press       = gimp_tool_widget_group_key_press;
  widget_class->key_release     = gimp_tool_widget_group_key_release;
  widget_class->motion_modifier = gimp_tool_widget_group_motion_modifier;
  widget_class->hover_modifier  = gimp_tool_widget_group_hover_modifier;
  widget_class->get_cursor      = gimp_tool_widget_group_get_cursor;
}

static void
gimp_tool_widget_group_init (GimpToolWidgetGroup *group)
{
  GimpToolWidgetGroupPrivate *priv;

  priv = group->priv = gimp_tool_widget_group_get_instance_private (group);

  priv->children = g_object_new (GIMP_TYPE_LIST,
                                 "children-type", GIMP_TYPE_TOOL_WIDGET,
                                 "append",        TRUE,
                                 NULL);

  g_signal_connect (priv->children, "add",
                    G_CALLBACK (gimp_tool_widget_group_children_add),
                    group);
  g_signal_connect (priv->children, "remove",
                    G_CALLBACK (gimp_tool_widget_group_children_remove),
                    group);
  g_signal_connect (priv->children, "reorder",
                    G_CALLBACK (gimp_tool_widget_group_children_reorder),
                    group);

  gimp_container_add_handler (priv->children, "changed",
                              G_CALLBACK (gimp_tool_widget_group_child_changed),
                              group);
  gimp_container_add_handler (priv->children, "response",
                              G_CALLBACK (gimp_tool_widget_group_child_response),
                              group);
  gimp_container_add_handler (priv->children, "snap-offsets",
                              G_CALLBACK (gimp_tool_widget_group_child_snap_offsets),
                              group);
  gimp_container_add_handler (priv->children, "status",
                              G_CALLBACK (gimp_tool_widget_group_child_status),
                              group);
  gimp_container_add_handler (priv->children, "status-coords",
                              G_CALLBACK (gimp_tool_widget_group_child_status_coords),
                              group);
  gimp_container_add_handler (priv->children, "message",
                              G_CALLBACK (gimp_tool_widget_group_child_message),
                              group);
  gimp_container_add_handler (priv->children, "focus-changed",
                              G_CALLBACK (gimp_tool_widget_group_child_focus_changed),
                              group);
}

static void
gimp_tool_widget_group_finalize (GObject *object)
{
  GimpToolWidgetGroup        *group = GIMP_TOOL_WIDGET_GROUP (object);
  GimpToolWidgetGroupPrivate *priv  = group->priv;

  g_clear_object (&priv->children);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static gint
gimp_tool_widget_group_button_press (GimpToolWidget      *widget,
                                     const GimpCoords    *coords,
                                     guint32              time,
                                     GdkModifierType      state,
                                     GimpButtonPressType  press_type)
{
  GimpToolWidgetGroup        *group = GIMP_TOOL_WIDGET_GROUP (widget);
  GimpToolWidgetGroupPrivate *priv  = group->priv;

  gimp_tool_widget_group_hover (widget, coords, state, TRUE);

  if (priv->focus_widget != priv->hover_widget)
    {
      if (priv->hover_widget)
        gimp_tool_widget_set_focus (priv->hover_widget, TRUE);
      else if (priv->focus_widget)
        gimp_tool_widget_set_focus (priv->focus_widget, FALSE);
    }

  if (priv->hover_widget)
    {
      if (priv->auto_raise)
        {
          gimp_container_reorder (priv->children,
                                  GIMP_OBJECT (priv->hover_widget), -1);
        }

      return gimp_tool_widget_button_press (priv->hover_widget,
                                            coords, time, state, press_type);
    }

  return FALSE;
}

static void
gimp_tool_widget_group_focus_changed (GimpToolWidget *widget)
{
  GimpToolWidgetGroup        *group = GIMP_TOOL_WIDGET_GROUP (widget);
  GimpToolWidgetGroupPrivate *priv  = group->priv;

  if (priv->focus_widget)
    {
      GimpToolWidget *focus_widget = priv->focus_widget;

      gimp_tool_widget_set_focus (priv->focus_widget,
                                  gimp_tool_widget_get_focus (widget));

      priv->focus_widget = focus_widget;
    }
}

static void
gimp_tool_widget_group_button_release (GimpToolWidget        *widget,
                                       const GimpCoords      *coords,
                                       guint32                time,
                                       GdkModifierType        state,
                                       GimpButtonReleaseType  release_type)
{
  GimpToolWidgetGroup        *group = GIMP_TOOL_WIDGET_GROUP (widget);
  GimpToolWidgetGroupPrivate *priv  = group->priv;

  if (priv->hover_widget)
    {
      gimp_tool_widget_button_release (priv->hover_widget,
                                       coords, time, state, release_type);
    }
}

static void
gimp_tool_widget_group_motion (GimpToolWidget   *widget,
                        const GimpCoords *coords,
                        guint32           time,
                        GdkModifierType   state)
{
  GimpToolWidgetGroup        *group = GIMP_TOOL_WIDGET_GROUP (widget);
  GimpToolWidgetGroupPrivate *priv = group->priv;

  if (priv->hover_widget)
    gimp_tool_widget_motion (priv->hover_widget, coords, time, state);
}

static GimpHit
gimp_tool_widget_group_hit (GimpToolWidget   *widget,
                            const GimpCoords *coords,
                            GdkModifierType   state,
                            gboolean          proximity)
{
  GimpToolWidgetGroup *group = GIMP_TOOL_WIDGET_GROUP (widget);
  GimpHit              hit;

  gimp_tool_widget_group_get_hover_widget (group,
                                           coords, state, proximity,
                                           &hit);

  return hit;
}

static void
gimp_tool_widget_group_hover (GimpToolWidget   *widget,
                              const GimpCoords *coords,
                              GdkModifierType   state,
                              gboolean          proximity)
{
  GimpToolWidgetGroup        *group = GIMP_TOOL_WIDGET_GROUP (widget);
  GimpToolWidgetGroupPrivate *priv  = group->priv;
  GimpToolWidget             *hover_widget;

  hover_widget =
    gimp_tool_widget_group_get_hover_widget (group,
                                             coords, state, proximity,
                                             NULL);

  if (priv->hover_widget && priv->hover_widget != hover_widget)
    gimp_tool_widget_leave_notify (priv->hover_widget);

  priv->hover_widget = hover_widget;

  if (priv->hover_widget)
    gimp_tool_widget_hover (priv->hover_widget, coords, state, proximity);
}

static void
gimp_tool_widget_group_leave_notify (GimpToolWidget *widget)
{
  GimpToolWidgetGroup        *group = GIMP_TOOL_WIDGET_GROUP (widget);
  GimpToolWidgetGroupPrivate *priv  = group->priv;

  if (priv->hover_widget)
    {
      gimp_tool_widget_leave_notify (priv->hover_widget);

      priv->hover_widget = NULL;
    }

  GIMP_TOOL_WIDGET_CLASS (parent_class)->leave_notify (widget);
}

static gboolean
gimp_tool_widget_group_key_press (GimpToolWidget *widget,
                                  GdkEventKey    *kevent)
{
  GimpToolWidgetGroup        *group = GIMP_TOOL_WIDGET_GROUP (widget);
  GimpToolWidgetGroupPrivate *priv  = group->priv;

  if (priv->focus_widget)
    return gimp_tool_widget_key_press (priv->focus_widget, kevent);

  return GIMP_TOOL_WIDGET_CLASS (parent_class)->key_press (widget, kevent);
}

static gboolean
gimp_tool_widget_group_key_release (GimpToolWidget *widget,
                                    GdkEventKey    *kevent)
{
  GimpToolWidgetGroup        *group = GIMP_TOOL_WIDGET_GROUP (widget);
  GimpToolWidgetGroupPrivate *priv  = group->priv;

  if (priv->focus_widget)
    return gimp_tool_widget_key_release (priv->focus_widget, kevent);

  return FALSE;
}

static void
gimp_tool_widget_group_motion_modifier (GimpToolWidget  *widget,
                                        GdkModifierType  key,
                                        gboolean         press,
                                        GdkModifierType  state)
{
  GimpToolWidgetGroup        *group = GIMP_TOOL_WIDGET_GROUP (widget);
  GimpToolWidgetGroupPrivate *priv  = group->priv;

  if (priv->hover_widget)
    gimp_tool_widget_motion_modifier (priv->hover_widget, key, press, state);
}

static void
gimp_tool_widget_group_hover_modifier (GimpToolWidget  *widget,
                                       GdkModifierType  key,
                                       gboolean         press,
                                       GdkModifierType  state)
{
  GimpToolWidgetGroup        *group = GIMP_TOOL_WIDGET_GROUP (widget);
  GimpToolWidgetGroupPrivate *priv  = group->priv;
  GList                      *iter;

  for (iter = g_queue_peek_head_link (GIMP_LIST (priv->children)->queue);
       iter;
       iter = g_list_next (iter))
    {
      gimp_tool_widget_hover_modifier (iter->data, key, press, state);
    }
}

static gboolean
gimp_tool_widget_group_get_cursor (GimpToolWidget     *widget,
                                   const GimpCoords   *coords,
                                   GdkModifierType     state,
                                   GimpCursorType     *cursor,
                                   GimpToolCursorType *tool_cursor,
                                   GimpCursorModifier *modifier)
{
  GimpToolWidgetGroup        *group = GIMP_TOOL_WIDGET_GROUP (widget);
  GimpToolWidgetGroupPrivate *priv  = group->priv;

  if (priv->hover_widget)
    {
      return gimp_tool_widget_get_cursor (priv->hover_widget,
                                          coords, state,
                                          cursor, tool_cursor, modifier);
    }

  return FALSE;
}

static void
gimp_tool_widget_group_children_add (GimpContainer       *container,
                                     GimpToolWidget      *child,
                                     GimpToolWidgetGroup *group)
{
  GimpToolWidgetGroupPrivate *priv   = group->priv;
  GimpToolWidget             *widget = GIMP_TOOL_WIDGET (group);
  GimpCanvasGroup            *canvas_group;

  canvas_group = GIMP_CANVAS_GROUP (gimp_tool_widget_get_item (widget));

  gimp_canvas_group_add_item (canvas_group,
                              gimp_tool_widget_get_item (child));

  if (gimp_tool_widget_get_focus (child) && priv->focus_widget)
    {
      gimp_tool_widget_set_focus (priv->focus_widget, FALSE);

      priv->focus_widget = NULL;
    }

  if (! priv->focus_widget)
    {
      priv->focus_widget = child;

      gimp_tool_widget_set_focus (child, gimp_tool_widget_get_focus (widget));
    }

  gimp_tool_widget_changed (widget);
}

static void
gimp_tool_widget_group_children_remove (GimpContainer       *container,
                                        GimpToolWidget      *child,
                                        GimpToolWidgetGroup *group)
{
  GimpToolWidgetGroupPrivate *priv   = group->priv;
  GimpToolWidget             *widget = GIMP_TOOL_WIDGET (group);
  GimpCanvasGroup            *canvas_group;

  canvas_group = GIMP_CANVAS_GROUP (gimp_tool_widget_get_item (widget));

  if (priv->focus_widget == child)
    {
      gimp_tool_widget_set_focus (child, FALSE);

      priv->focus_widget = NULL;
    }

  if (priv->hover_widget == child)
    {
      gimp_tool_widget_leave_notify (child);

      priv->hover_widget = NULL;
    }

  if (! priv->focus_widget)
    {
      priv->focus_widget =
        GIMP_TOOL_WIDGET (gimp_container_get_last_child (container));

      if (priv->focus_widget)
        gimp_tool_widget_set_focus (priv->focus_widget, TRUE);
    }

  gimp_canvas_group_remove_item (canvas_group,
                                 gimp_tool_widget_get_item (child));

  gimp_tool_widget_changed (widget);
}

static void
gimp_tool_widget_group_children_reorder (GimpContainer       *container,
                                         GimpToolWidget      *child,
                                         gint                 new_index,
                                         GimpToolWidgetGroup *group)
{
  GimpToolWidget  *widget = GIMP_TOOL_WIDGET (group);
  GimpCanvasGroup *canvas_group;
  GList           *iter;

  canvas_group = GIMP_CANVAS_GROUP (gimp_tool_widget_get_item (widget));

  for (iter = g_queue_peek_head_link (GIMP_LIST (container)->queue);
       iter;
       iter = g_list_next (iter))
    {
      GimpCanvasItem *item = gimp_tool_widget_get_item (iter->data);

      gimp_canvas_group_remove_item (canvas_group, item);
      gimp_canvas_group_add_item    (canvas_group, item);
    }

  gimp_tool_widget_changed (widget);
}

static void
gimp_tool_widget_group_child_changed (GimpToolWidget      *child,
                                      GimpToolWidgetGroup *group)
{
  GimpToolWidget *widget = GIMP_TOOL_WIDGET (group);

  gimp_tool_widget_changed (widget);
}

static void
gimp_tool_widget_group_child_response (GimpToolWidget      *child,
                                       gint                 response_id,
                                       GimpToolWidgetGroup *group)
{
  GimpToolWidgetGroupPrivate *priv   = group->priv;
  GimpToolWidget             *widget = GIMP_TOOL_WIDGET (group);

  if (priv->focus_widget == child)
    gimp_tool_widget_response (widget, response_id);
}

static void
gimp_tool_widget_group_child_snap_offsets (GimpToolWidget      *child,
                                           gint                 offset_x,
                                           gint                 offset_y,
                                           gint                 width,
                                           gint                 height,
                                           GimpToolWidgetGroup *group)
{
  GimpToolWidgetGroupPrivate *priv   = group->priv;
  GimpToolWidget             *widget = GIMP_TOOL_WIDGET (group);

  if (priv->hover_widget == child)
    {
      gimp_tool_widget_set_snap_offsets (widget,
                                         offset_x, offset_y, width, height);
    }
}

static void
gimp_tool_widget_group_child_status (GimpToolWidget      *child,
                                     const gchar         *status,
                                     GimpToolWidgetGroup *group)
{
  GimpToolWidgetGroupPrivate *priv   = group->priv;
  GimpToolWidget             *widget = GIMP_TOOL_WIDGET (group);

  if (priv->hover_widget == child)
    gimp_tool_widget_set_status (widget, status);
}

static void
gimp_tool_widget_group_child_status_coords (GimpToolWidget      *child,
                                            const gchar         *title,
                                            gdouble              x,
                                            const gchar         *separator,
                                            gdouble              y,
                                            const gchar         *help,
                                            GimpToolWidgetGroup *group)
{
  GimpToolWidgetGroupPrivate *priv   = group->priv;
  GimpToolWidget             *widget = GIMP_TOOL_WIDGET (group);

  if (priv->hover_widget == child)
    gimp_tool_widget_set_status_coords (widget, title, x, separator, y, help);
}

static void
gimp_tool_widget_group_child_message (GimpToolWidget      *child,
                                      const gchar         *message,
                                      GimpToolWidgetGroup *group)
{
  GimpToolWidgetGroupPrivate *priv   = group->priv;
  GimpToolWidget             *widget = GIMP_TOOL_WIDGET (group);

  if (priv->focus_widget == child)
    gimp_tool_widget_message_literal (widget, message);
}

static void
gimp_tool_widget_group_child_focus_changed (GimpToolWidget      *child,
                                            GimpToolWidgetGroup *group)
{
  GimpToolWidgetGroupPrivate *priv   = group->priv;
  GimpToolWidget             *widget = GIMP_TOOL_WIDGET (group);

  if (gimp_tool_widget_get_focus (child))
    {
      if (priv->focus_widget && priv->focus_widget != child)
        gimp_tool_widget_set_focus (priv->focus_widget, FALSE);

      priv->focus_widget = child;

      gimp_tool_widget_set_focus (widget, TRUE);
    }
  else
    {
      if (priv->focus_widget == child)
        priv->focus_widget = NULL;
    }
}

static GimpToolWidget *
gimp_tool_widget_group_get_hover_widget (GimpToolWidgetGroup *group,
                                         const GimpCoords    *coords,
                                         GdkModifierType      state,
                                         gboolean             proximity,
                                         GimpHit             *hit)
{
  GimpToolWidgetGroupPrivate *priv           = group->priv;
  GimpToolWidget             *indirect_child = NULL;
  gboolean                    indirect       = FALSE;
  GList                      *iter;

  for (iter = g_queue_peek_tail_link (GIMP_LIST (priv->children)->queue);
       iter;
       iter = g_list_previous (iter))
    {
      GimpToolWidget *child = iter->data;

      switch (gimp_tool_widget_hit (child, coords, state, proximity))
        {
        case GIMP_HIT_DIRECT:
          if (hit) *hit = GIMP_HIT_DIRECT;

          return child;

        case GIMP_HIT_INDIRECT:
          if (! indirect || child == priv->focus_widget)
            indirect_child = child;
          else if (indirect_child != priv->focus_widget)
            indirect_child = NULL;

          indirect = TRUE;

          break;

        case GIMP_HIT_NONE:
          break;
        }
    }

  if (hit) *hit = indirect_child ? GIMP_HIT_INDIRECT : GIMP_HIT_NONE;

  return indirect_child;
}


/*  public functions  */


GimpToolWidget *
gimp_tool_widget_group_new (GimpDisplayShell *shell)
{
  g_return_val_if_fail (GIMP_IS_DISPLAY_SHELL (shell), NULL);

  return g_object_new (GIMP_TYPE_TOOL_WIDGET_GROUP,
                       "shell", shell,
                       NULL);
}

GimpContainer *
gimp_tool_widget_group_get_children (GimpToolWidgetGroup *group)
{
  g_return_val_if_fail (GIMP_IS_TOOL_WIDGET_GROUP (group), NULL);

  return group->priv->children;
}

GimpToolWidget *
gimp_tool_widget_group_get_focus_widget (GimpToolWidgetGroup *group)
{
  g_return_val_if_fail (GIMP_IS_TOOL_WIDGET_GROUP (group), NULL);

  return group->priv->focus_widget;
}

void
gimp_tool_widget_group_set_auto_raise (GimpToolWidgetGroup *group,
                                       gboolean             auto_raise)
{
  g_return_if_fail (GIMP_IS_TOOL_WIDGET_GROUP (group));

  group->priv->auto_raise = auto_raise;
}

gboolean
gimp_tool_widget_group_get_auto_raise (GimpToolWidgetGroup *group)
{
  g_return_val_if_fail (GIMP_IS_TOOL_WIDGET_GROUP (group), FALSE);

  return group->priv->auto_raise;
}

