/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ligmatoolwidgetgroup.c
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

#include "core/ligmacontainer.h"
#include "core/ligmalist.h"

#include "ligmacanvasgroup.h"
#include "ligmadisplayshell.h"
#include "ligmatoolwidgetgroup.h"


struct _LigmaToolWidgetGroupPrivate
{
  LigmaContainer  *children;

  LigmaToolWidget *focus_widget;
  LigmaToolWidget *hover_widget;

  gboolean        auto_raise;
};


/*  local function prototypes  */

static void             ligma_tool_widget_group_finalize            (GObject               *object);

static void             ligma_tool_widget_group_focus_changed       (LigmaToolWidget        *widget);
static gint             ligma_tool_widget_group_button_press        (LigmaToolWidget        *widget,
                                                                    const LigmaCoords      *coords,
                                                                    guint32                time,
                                                                    GdkModifierType        state,
                                                                    LigmaButtonPressType    press_type);
static void             ligma_tool_widget_group_button_release      (LigmaToolWidget        *widget,
                                                                    const LigmaCoords      *coords,
                                                                    guint32                time,
                                                                    GdkModifierType        state,
                                                                    LigmaButtonReleaseType  release_type);
static void             ligma_tool_widget_group_motion              (LigmaToolWidget        *widget,
                                                                    const LigmaCoords      *coords,
                                                                    guint32                time,
                                                                    GdkModifierType        state);
static LigmaHit          ligma_tool_widget_group_hit                 (LigmaToolWidget        *widget,
                                                                    const LigmaCoords      *coords,
                                                                    GdkModifierType        state,
                                                                    gboolean               proximity);
static void             ligma_tool_widget_group_hover               (LigmaToolWidget        *widget,
                                                                    const LigmaCoords      *coords,
                                                                    GdkModifierType        state,
                                                                    gboolean               proximity);
static void             ligma_tool_widget_group_leave_notify        (LigmaToolWidget        *widget);
static gboolean         ligma_tool_widget_group_key_press           (LigmaToolWidget        *widget,
                                                                    GdkEventKey           *kevent);
static gboolean         ligma_tool_widget_group_key_release         (LigmaToolWidget        *widget,
                                                                    GdkEventKey           *kevent);
static void             ligma_tool_widget_group_motion_modifier     (LigmaToolWidget        *widget,
                                                                    GdkModifierType        key,
                                                                    gboolean               press,
                                                                    GdkModifierType        state);
static void             ligma_tool_widget_group_hover_modifier      (LigmaToolWidget        *widget,
                                                                    GdkModifierType        key,
                                                                    gboolean               press,
                                                                    GdkModifierType        state);
static gboolean         ligma_tool_widget_group_get_cursor          (LigmaToolWidget        *widget,
                                                                    const LigmaCoords      *coords,
                                                                    GdkModifierType        state,
                                                                    LigmaCursorType        *cursor,
                                                                    LigmaToolCursorType    *tool_cursor,
                                                                    LigmaCursorModifier    *modifier);

static void             ligma_tool_widget_group_children_add        (LigmaContainer         *container,
                                                                    LigmaToolWidget        *child,
                                                                    LigmaToolWidgetGroup   *group);
static void             ligma_tool_widget_group_children_remove     (LigmaContainer         *container,
                                                                    LigmaToolWidget        *child,
                                                                    LigmaToolWidgetGroup   *group);
static void             ligma_tool_widget_group_children_reorder    (LigmaContainer         *container,
                                                                    LigmaToolWidget        *child,
                                                                    gint                   new_index,
                                                                    LigmaToolWidgetGroup   *group);

static void             ligma_tool_widget_group_child_changed       (LigmaToolWidget        *child,
                                                                    LigmaToolWidgetGroup   *group);
static void             ligma_tool_widget_group_child_response      (LigmaToolWidget        *child,
                                                                    gint                   response_id,
                                                                    LigmaToolWidgetGroup   *group);
static void             ligma_tool_widget_group_child_snap_offsets  (LigmaToolWidget        *child,
                                                                    gint                   offset_x,
                                                                    gint                   offset_y,
                                                                    gint                   width,
                                                                    gint                   height,
                                                                    LigmaToolWidgetGroup   *group);
static void             ligma_tool_widget_group_child_status        (LigmaToolWidget        *child,
                                                                    const gchar           *status,
                                                                    LigmaToolWidgetGroup   *group);
static void             ligma_tool_widget_group_child_status_coords (LigmaToolWidget        *child,
                                                                    const gchar           *title,
                                                                    gdouble                x,
                                                                    const gchar           *separator,
                                                                    gdouble                y,
                                                                    const gchar           *help,
                                                                    LigmaToolWidgetGroup   *group);
static void             ligma_tool_widget_group_child_message       (LigmaToolWidget        *child,
                                                                    const gchar           *message,
                                                                    LigmaToolWidgetGroup   *group);
static void             ligma_tool_widget_group_child_focus_changed (LigmaToolWidget        *child,
                                                                    LigmaToolWidgetGroup   *group);

static LigmaToolWidget * ligma_tool_widget_group_get_hover_widget    (LigmaToolWidgetGroup   *group,
                                                                    const LigmaCoords      *coords,
                                                                    GdkModifierType        state,
                                                                    gboolean               proximity,
                                                                    LigmaHit               *hit);


G_DEFINE_TYPE_WITH_PRIVATE (LigmaToolWidgetGroup, ligma_tool_widget_group,
                            LIGMA_TYPE_TOOL_WIDGET)

#define parent_class ligma_tool_widget_group_parent_class


/*  priv functions  */


static void
ligma_tool_widget_group_class_init (LigmaToolWidgetGroupClass *klass)
{
  GObjectClass        *object_class = G_OBJECT_CLASS (klass);
  LigmaToolWidgetClass *widget_class = LIGMA_TOOL_WIDGET_CLASS (klass);

  object_class->finalize        = ligma_tool_widget_group_finalize;

  widget_class->focus_changed   = ligma_tool_widget_group_focus_changed;
  widget_class->button_press    = ligma_tool_widget_group_button_press;
  widget_class->button_release  = ligma_tool_widget_group_button_release;
  widget_class->motion          = ligma_tool_widget_group_motion;
  widget_class->hit             = ligma_tool_widget_group_hit;
  widget_class->hover           = ligma_tool_widget_group_hover;
  widget_class->leave_notify    = ligma_tool_widget_group_leave_notify;
  widget_class->key_press       = ligma_tool_widget_group_key_press;
  widget_class->key_release     = ligma_tool_widget_group_key_release;
  widget_class->motion_modifier = ligma_tool_widget_group_motion_modifier;
  widget_class->hover_modifier  = ligma_tool_widget_group_hover_modifier;
  widget_class->get_cursor      = ligma_tool_widget_group_get_cursor;
}

static void
ligma_tool_widget_group_init (LigmaToolWidgetGroup *group)
{
  LigmaToolWidgetGroupPrivate *priv;

  priv = group->priv = ligma_tool_widget_group_get_instance_private (group);

  priv->children = g_object_new (LIGMA_TYPE_LIST,
                                 "children-type", LIGMA_TYPE_TOOL_WIDGET,
                                 "append",        TRUE,
                                 NULL);

  g_signal_connect (priv->children, "add",
                    G_CALLBACK (ligma_tool_widget_group_children_add),
                    group);
  g_signal_connect (priv->children, "remove",
                    G_CALLBACK (ligma_tool_widget_group_children_remove),
                    group);
  g_signal_connect (priv->children, "reorder",
                    G_CALLBACK (ligma_tool_widget_group_children_reorder),
                    group);

  ligma_container_add_handler (priv->children, "changed",
                              G_CALLBACK (ligma_tool_widget_group_child_changed),
                              group);
  ligma_container_add_handler (priv->children, "response",
                              G_CALLBACK (ligma_tool_widget_group_child_response),
                              group);
  ligma_container_add_handler (priv->children, "snap-offsets",
                              G_CALLBACK (ligma_tool_widget_group_child_snap_offsets),
                              group);
  ligma_container_add_handler (priv->children, "status",
                              G_CALLBACK (ligma_tool_widget_group_child_status),
                              group);
  ligma_container_add_handler (priv->children, "status-coords",
                              G_CALLBACK (ligma_tool_widget_group_child_status_coords),
                              group);
  ligma_container_add_handler (priv->children, "message",
                              G_CALLBACK (ligma_tool_widget_group_child_message),
                              group);
  ligma_container_add_handler (priv->children, "focus-changed",
                              G_CALLBACK (ligma_tool_widget_group_child_focus_changed),
                              group);
}

static void
ligma_tool_widget_group_finalize (GObject *object)
{
  LigmaToolWidgetGroup        *group = LIGMA_TOOL_WIDGET_GROUP (object);
  LigmaToolWidgetGroupPrivate *priv  = group->priv;

  g_clear_object (&priv->children);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static gint
ligma_tool_widget_group_button_press (LigmaToolWidget      *widget,
                                     const LigmaCoords    *coords,
                                     guint32              time,
                                     GdkModifierType      state,
                                     LigmaButtonPressType  press_type)
{
  LigmaToolWidgetGroup        *group = LIGMA_TOOL_WIDGET_GROUP (widget);
  LigmaToolWidgetGroupPrivate *priv  = group->priv;

  ligma_tool_widget_group_hover (widget, coords, state, TRUE);

  if (priv->focus_widget != priv->hover_widget)
    {
      if (priv->hover_widget)
        ligma_tool_widget_set_focus (priv->hover_widget, TRUE);
      else if (priv->focus_widget)
        ligma_tool_widget_set_focus (priv->focus_widget, FALSE);
    }

  if (priv->hover_widget)
    {
      if (priv->auto_raise)
        {
          ligma_container_reorder (priv->children,
                                  LIGMA_OBJECT (priv->hover_widget), -1);
        }

      return ligma_tool_widget_button_press (priv->hover_widget,
                                            coords, time, state, press_type);
    }

  return FALSE;
}

static void
ligma_tool_widget_group_focus_changed (LigmaToolWidget *widget)
{
  LigmaToolWidgetGroup        *group = LIGMA_TOOL_WIDGET_GROUP (widget);
  LigmaToolWidgetGroupPrivate *priv  = group->priv;

  if (priv->focus_widget)
    {
      LigmaToolWidget *focus_widget = priv->focus_widget;

      ligma_tool_widget_set_focus (priv->focus_widget,
                                  ligma_tool_widget_get_focus (widget));

      priv->focus_widget = focus_widget;
    }
}

static void
ligma_tool_widget_group_button_release (LigmaToolWidget        *widget,
                                       const LigmaCoords      *coords,
                                       guint32                time,
                                       GdkModifierType        state,
                                       LigmaButtonReleaseType  release_type)
{
  LigmaToolWidgetGroup        *group = LIGMA_TOOL_WIDGET_GROUP (widget);
  LigmaToolWidgetGroupPrivate *priv  = group->priv;

  if (priv->hover_widget)
    {
      ligma_tool_widget_button_release (priv->hover_widget,
                                       coords, time, state, release_type);
    }
}

static void
ligma_tool_widget_group_motion (LigmaToolWidget   *widget,
                        const LigmaCoords *coords,
                        guint32           time,
                        GdkModifierType   state)
{
  LigmaToolWidgetGroup        *group = LIGMA_TOOL_WIDGET_GROUP (widget);
  LigmaToolWidgetGroupPrivate *priv = group->priv;

  if (priv->hover_widget)
    ligma_tool_widget_motion (priv->hover_widget, coords, time, state);
}

static LigmaHit
ligma_tool_widget_group_hit (LigmaToolWidget   *widget,
                            const LigmaCoords *coords,
                            GdkModifierType   state,
                            gboolean          proximity)
{
  LigmaToolWidgetGroup *group = LIGMA_TOOL_WIDGET_GROUP (widget);
  LigmaHit              hit;

  ligma_tool_widget_group_get_hover_widget (group,
                                           coords, state, proximity,
                                           &hit);

  return hit;
}

static void
ligma_tool_widget_group_hover (LigmaToolWidget   *widget,
                              const LigmaCoords *coords,
                              GdkModifierType   state,
                              gboolean          proximity)
{
  LigmaToolWidgetGroup        *group = LIGMA_TOOL_WIDGET_GROUP (widget);
  LigmaToolWidgetGroupPrivate *priv  = group->priv;
  LigmaToolWidget             *hover_widget;

  hover_widget =
    ligma_tool_widget_group_get_hover_widget (group,
                                             coords, state, proximity,
                                             NULL);

  if (priv->hover_widget && priv->hover_widget != hover_widget)
    ligma_tool_widget_leave_notify (priv->hover_widget);

  priv->hover_widget = hover_widget;

  if (priv->hover_widget)
    ligma_tool_widget_hover (priv->hover_widget, coords, state, proximity);
}

static void
ligma_tool_widget_group_leave_notify (LigmaToolWidget *widget)
{
  LigmaToolWidgetGroup        *group = LIGMA_TOOL_WIDGET_GROUP (widget);
  LigmaToolWidgetGroupPrivate *priv  = group->priv;

  if (priv->hover_widget)
    {
      ligma_tool_widget_leave_notify (priv->hover_widget);

      priv->hover_widget = NULL;
    }

  LIGMA_TOOL_WIDGET_CLASS (parent_class)->leave_notify (widget);
}

static gboolean
ligma_tool_widget_group_key_press (LigmaToolWidget *widget,
                                  GdkEventKey    *kevent)
{
  LigmaToolWidgetGroup        *group = LIGMA_TOOL_WIDGET_GROUP (widget);
  LigmaToolWidgetGroupPrivate *priv  = group->priv;

  if (priv->focus_widget)
    return ligma_tool_widget_key_press (priv->focus_widget, kevent);

  return LIGMA_TOOL_WIDGET_CLASS (parent_class)->key_press (widget, kevent);
}

static gboolean
ligma_tool_widget_group_key_release (LigmaToolWidget *widget,
                                    GdkEventKey    *kevent)
{
  LigmaToolWidgetGroup        *group = LIGMA_TOOL_WIDGET_GROUP (widget);
  LigmaToolWidgetGroupPrivate *priv  = group->priv;

  if (priv->focus_widget)
    return ligma_tool_widget_key_release (priv->focus_widget, kevent);

  return FALSE;
}

static void
ligma_tool_widget_group_motion_modifier (LigmaToolWidget  *widget,
                                        GdkModifierType  key,
                                        gboolean         press,
                                        GdkModifierType  state)
{
  LigmaToolWidgetGroup        *group = LIGMA_TOOL_WIDGET_GROUP (widget);
  LigmaToolWidgetGroupPrivate *priv  = group->priv;

  if (priv->hover_widget)
    ligma_tool_widget_motion_modifier (priv->hover_widget, key, press, state);
}

static void
ligma_tool_widget_group_hover_modifier (LigmaToolWidget  *widget,
                                       GdkModifierType  key,
                                       gboolean         press,
                                       GdkModifierType  state)
{
  LigmaToolWidgetGroup        *group = LIGMA_TOOL_WIDGET_GROUP (widget);
  LigmaToolWidgetGroupPrivate *priv  = group->priv;
  GList                      *iter;

  for (iter = g_queue_peek_head_link (LIGMA_LIST (priv->children)->queue);
       iter;
       iter = g_list_next (iter))
    {
      ligma_tool_widget_hover_modifier (iter->data, key, press, state);
    }
}

static gboolean
ligma_tool_widget_group_get_cursor (LigmaToolWidget     *widget,
                                   const LigmaCoords   *coords,
                                   GdkModifierType     state,
                                   LigmaCursorType     *cursor,
                                   LigmaToolCursorType *tool_cursor,
                                   LigmaCursorModifier *modifier)
{
  LigmaToolWidgetGroup        *group = LIGMA_TOOL_WIDGET_GROUP (widget);
  LigmaToolWidgetGroupPrivate *priv  = group->priv;

  if (priv->hover_widget)
    {
      return ligma_tool_widget_get_cursor (priv->hover_widget,
                                          coords, state,
                                          cursor, tool_cursor, modifier);
    }

  return FALSE;
}

static void
ligma_tool_widget_group_children_add (LigmaContainer       *container,
                                     LigmaToolWidget      *child,
                                     LigmaToolWidgetGroup *group)
{
  LigmaToolWidgetGroupPrivate *priv   = group->priv;
  LigmaToolWidget             *widget = LIGMA_TOOL_WIDGET (group);
  LigmaCanvasGroup            *canvas_group;

  canvas_group = LIGMA_CANVAS_GROUP (ligma_tool_widget_get_item (widget));

  ligma_canvas_group_add_item (canvas_group,
                              ligma_tool_widget_get_item (child));

  if (ligma_tool_widget_get_focus (child) && priv->focus_widget)
    {
      ligma_tool_widget_set_focus (priv->focus_widget, FALSE);

      priv->focus_widget = NULL;
    }

  if (! priv->focus_widget)
    {
      priv->focus_widget = child;

      ligma_tool_widget_set_focus (child, ligma_tool_widget_get_focus (widget));
    }

  ligma_tool_widget_changed (widget);
}

static void
ligma_tool_widget_group_children_remove (LigmaContainer       *container,
                                        LigmaToolWidget      *child,
                                        LigmaToolWidgetGroup *group)
{
  LigmaToolWidgetGroupPrivate *priv   = group->priv;
  LigmaToolWidget             *widget = LIGMA_TOOL_WIDGET (group);
  LigmaCanvasGroup            *canvas_group;

  canvas_group = LIGMA_CANVAS_GROUP (ligma_tool_widget_get_item (widget));

  if (priv->focus_widget == child)
    {
      ligma_tool_widget_set_focus (child, FALSE);

      priv->focus_widget = NULL;
    }

  if (priv->hover_widget == child)
    {
      ligma_tool_widget_leave_notify (child);

      priv->hover_widget = NULL;
    }

  if (! priv->focus_widget)
    {
      priv->focus_widget =
        LIGMA_TOOL_WIDGET (ligma_container_get_last_child (container));

      if (priv->focus_widget)
        ligma_tool_widget_set_focus (priv->focus_widget, TRUE);
    }

  ligma_canvas_group_remove_item (canvas_group,
                                 ligma_tool_widget_get_item (child));

  ligma_tool_widget_changed (widget);
}

static void
ligma_tool_widget_group_children_reorder (LigmaContainer       *container,
                                         LigmaToolWidget      *child,
                                         gint                 new_index,
                                         LigmaToolWidgetGroup *group)
{
  LigmaToolWidget  *widget = LIGMA_TOOL_WIDGET (group);
  LigmaCanvasGroup *canvas_group;
  GList           *iter;

  canvas_group = LIGMA_CANVAS_GROUP (ligma_tool_widget_get_item (widget));

  for (iter = g_queue_peek_head_link (LIGMA_LIST (container)->queue);
       iter;
       iter = g_list_next (iter))
    {
      LigmaCanvasItem *item = ligma_tool_widget_get_item (iter->data);

      ligma_canvas_group_remove_item (canvas_group, item);
      ligma_canvas_group_add_item    (canvas_group, item);
    }

  ligma_tool_widget_changed (widget);
}

static void
ligma_tool_widget_group_child_changed (LigmaToolWidget      *child,
                                      LigmaToolWidgetGroup *group)
{
  LigmaToolWidget *widget = LIGMA_TOOL_WIDGET (group);

  ligma_tool_widget_changed (widget);
}

static void
ligma_tool_widget_group_child_response (LigmaToolWidget      *child,
                                       gint                 response_id,
                                       LigmaToolWidgetGroup *group)
{
  LigmaToolWidgetGroupPrivate *priv   = group->priv;
  LigmaToolWidget             *widget = LIGMA_TOOL_WIDGET (group);

  if (priv->focus_widget == child)
    ligma_tool_widget_response (widget, response_id);
}

static void
ligma_tool_widget_group_child_snap_offsets (LigmaToolWidget      *child,
                                           gint                 offset_x,
                                           gint                 offset_y,
                                           gint                 width,
                                           gint                 height,
                                           LigmaToolWidgetGroup *group)
{
  LigmaToolWidgetGroupPrivate *priv   = group->priv;
  LigmaToolWidget             *widget = LIGMA_TOOL_WIDGET (group);

  if (priv->hover_widget == child)
    {
      ligma_tool_widget_set_snap_offsets (widget,
                                         offset_x, offset_y, width, height);
    }
}

static void
ligma_tool_widget_group_child_status (LigmaToolWidget      *child,
                                     const gchar         *status,
                                     LigmaToolWidgetGroup *group)
{
  LigmaToolWidgetGroupPrivate *priv   = group->priv;
  LigmaToolWidget             *widget = LIGMA_TOOL_WIDGET (group);

  if (priv->hover_widget == child)
    ligma_tool_widget_set_status (widget, status);
}

static void
ligma_tool_widget_group_child_status_coords (LigmaToolWidget      *child,
                                            const gchar         *title,
                                            gdouble              x,
                                            const gchar         *separator,
                                            gdouble              y,
                                            const gchar         *help,
                                            LigmaToolWidgetGroup *group)
{
  LigmaToolWidgetGroupPrivate *priv   = group->priv;
  LigmaToolWidget             *widget = LIGMA_TOOL_WIDGET (group);

  if (priv->hover_widget == child)
    ligma_tool_widget_set_status_coords (widget, title, x, separator, y, help);
}

static void
ligma_tool_widget_group_child_message (LigmaToolWidget      *child,
                                      const gchar         *message,
                                      LigmaToolWidgetGroup *group)
{
  LigmaToolWidgetGroupPrivate *priv   = group->priv;
  LigmaToolWidget             *widget = LIGMA_TOOL_WIDGET (group);

  if (priv->focus_widget == child)
    ligma_tool_widget_message_literal (widget, message);
}

static void
ligma_tool_widget_group_child_focus_changed (LigmaToolWidget      *child,
                                            LigmaToolWidgetGroup *group)
{
  LigmaToolWidgetGroupPrivate *priv   = group->priv;
  LigmaToolWidget             *widget = LIGMA_TOOL_WIDGET (group);

  if (ligma_tool_widget_get_focus (child))
    {
      if (priv->focus_widget && priv->focus_widget != child)
        ligma_tool_widget_set_focus (priv->focus_widget, FALSE);

      priv->focus_widget = child;

      ligma_tool_widget_set_focus (widget, TRUE);
    }
  else
    {
      if (priv->focus_widget == child)
        priv->focus_widget = NULL;
    }
}

static LigmaToolWidget *
ligma_tool_widget_group_get_hover_widget (LigmaToolWidgetGroup *group,
                                         const LigmaCoords    *coords,
                                         GdkModifierType      state,
                                         gboolean             proximity,
                                         LigmaHit             *hit)
{
  LigmaToolWidgetGroupPrivate *priv           = group->priv;
  LigmaToolWidget             *indirect_child = NULL;
  gboolean                    indirect       = FALSE;
  GList                      *iter;

  for (iter = g_queue_peek_tail_link (LIGMA_LIST (priv->children)->queue);
       iter;
       iter = g_list_previous (iter))
    {
      LigmaToolWidget *child = iter->data;

      switch (ligma_tool_widget_hit (child, coords, state, proximity))
        {
        case LIGMA_HIT_DIRECT:
          if (hit) *hit = LIGMA_HIT_DIRECT;

          return child;

        case LIGMA_HIT_INDIRECT:
          if (! indirect || child == priv->focus_widget)
            indirect_child = child;
          else if (indirect_child != priv->focus_widget)
            indirect_child = NULL;

          indirect = TRUE;

          break;

        case LIGMA_HIT_NONE:
          break;
        }
    }

  if (hit) *hit = indirect_child ? LIGMA_HIT_INDIRECT : LIGMA_HIT_NONE;

  return indirect_child;
}


/*  public functions  */


LigmaToolWidget *
ligma_tool_widget_group_new (LigmaDisplayShell *shell)
{
  g_return_val_if_fail (LIGMA_IS_DISPLAY_SHELL (shell), NULL);

  return g_object_new (LIGMA_TYPE_TOOL_WIDGET_GROUP,
                       "shell", shell,
                       NULL);
}

LigmaContainer *
ligma_tool_widget_group_get_children (LigmaToolWidgetGroup *group)
{
  g_return_val_if_fail (LIGMA_IS_TOOL_WIDGET_GROUP (group), NULL);

  return group->priv->children;
}

LigmaToolWidget *
ligma_tool_widget_group_get_focus_widget (LigmaToolWidgetGroup *group)
{
  g_return_val_if_fail (LIGMA_IS_TOOL_WIDGET_GROUP (group), NULL);

  return group->priv->focus_widget;
}

void
ligma_tool_widget_group_set_auto_raise (LigmaToolWidgetGroup *group,
                                       gboolean             auto_raise)
{
  g_return_if_fail (LIGMA_IS_TOOL_WIDGET_GROUP (group));

  group->priv->auto_raise = auto_raise;
}

gboolean
ligma_tool_widget_group_get_auto_raise (LigmaToolWidgetGroup *group)
{
  g_return_val_if_fail (LIGMA_IS_TOOL_WIDGET_GROUP (group), FALSE);

  return group->priv->auto_raise;
}

