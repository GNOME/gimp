/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * Major improvement to support polygonal segments
 * Copyright (C) 2008 Martin Nordholts
 *
 * This program is polygon software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Polygon Software Foundation; either version 3 of the License, or
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

#include "libgimpwidgets/gimpwidgets.h"

#include "tools-types.h"

#include "core/gimpchannel.h"
#include "core/gimpchannel-select.h"
#include "core/gimpimage.h"
#include "core/gimplayer-floating-selection.h"

#include "widgets/gimphelp-ids.h"
#include "widgets/gimpwidgets-utils.h"

#include "display/gimpdisplay.h"
#include "display/gimptoolpolygon.h"

#include "gimppolygonselecttool.h"
#include "gimpselectionoptions.h"
#include "gimptoolcontrol.h"


struct _GimpPolygonSelectToolPrivate
{
  GimpToolWidget *widget;
  GimpToolWidget *grab_widget;

  gboolean        pending_response;
  gint            pending_response_id;
};


/*  local function prototypes  */

static void       gimp_polygon_select_tool_finalize                (GObject               *object);

static void       gimp_polygon_select_tool_control                 (GimpTool              *tool,
                                                                    GimpToolAction         action,
                                                                    GimpDisplay           *display);
static void       gimp_polygon_select_tool_button_press            (GimpTool              *tool,
                                                                    const GimpCoords      *coords,
                                                                    guint32                time,
                                                                    GdkModifierType        state,
                                                                    GimpButtonPressType    press_type,
                                                                    GimpDisplay           *display);
static void       gimp_polygon_select_tool_button_release          (GimpTool              *tool,
                                                                    const GimpCoords      *coords,
                                                                    guint32                time,
                                                                    GdkModifierType        state,
                                                                    GimpButtonReleaseType  release_type,
                                                                    GimpDisplay           *display);
static void       gimp_polygon_select_tool_motion                  (GimpTool              *tool,
                                                                    const GimpCoords      *coords,
                                                                    guint32                time,
                                                                    GdkModifierType        state,
                                                                    GimpDisplay           *display);
static gboolean   gimp_polygon_select_tool_key_press               (GimpTool              *tool,
                                                                    GdkEventKey           *kevent,
                                                                    GimpDisplay           *display);
static void       gimp_polygon_select_tool_modifier_key            (GimpTool              *tool,
                                                                    GdkModifierType        key,
                                                                    gboolean               press,
                                                                    GdkModifierType        state,
                                                                    GimpDisplay           *display);
static void       gimp_polygon_select_tool_oper_update             (GimpTool              *tool,
                                                                    const GimpCoords      *coords,
                                                                    GdkModifierType        state,
                                                                    gboolean               proximity,
                                                                    GimpDisplay           *display);
static void       gimp_polygon_select_tool_cursor_update           (GimpTool              *tool,
                                                                    const GimpCoords      *coords,
                                                                    GdkModifierType        state,
                                                                    GimpDisplay           *display);

static void       gimp_polygon_select_tool_real_confirm            (GimpPolygonSelectTool *poly_sel,
                                                                    GimpDisplay           *display);

static void       gimp_polygon_select_tool_polygon_change_complete (GimpToolWidget        *polygon,
                                                                    GimpPolygonSelectTool *poly_sel);
static void       gimp_polygon_select_tool_polygon_response        (GimpToolWidget        *polygon,
                                                                    gint                   response_id,
                                                                    GimpPolygonSelectTool *poly_sel);

static void       gimp_polygon_select_tool_start                   (GimpPolygonSelectTool *poly_sel,
                                                                    GimpDisplay           *display);


G_DEFINE_TYPE_WITH_PRIVATE (GimpPolygonSelectTool, gimp_polygon_select_tool,
                            GIMP_TYPE_SELECTION_TOOL)

#define parent_class gimp_polygon_select_tool_parent_class


/*  private functions  */

static void
gimp_polygon_select_tool_class_init (GimpPolygonSelectToolClass *klass)
{
  GObjectClass  *object_class = G_OBJECT_CLASS (klass);
  GimpToolClass *tool_class   = GIMP_TOOL_CLASS (klass);

  object_class->finalize     = gimp_polygon_select_tool_finalize;

  tool_class->control        = gimp_polygon_select_tool_control;
  tool_class->button_press   = gimp_polygon_select_tool_button_press;
  tool_class->button_release = gimp_polygon_select_tool_button_release;
  tool_class->motion         = gimp_polygon_select_tool_motion;
  tool_class->key_press      = gimp_polygon_select_tool_key_press;
  tool_class->modifier_key   = gimp_polygon_select_tool_modifier_key;
  tool_class->oper_update    = gimp_polygon_select_tool_oper_update;
  tool_class->cursor_update  = gimp_polygon_select_tool_cursor_update;

  klass->change_complete     = NULL;
  klass->confirm             = gimp_polygon_select_tool_real_confirm;
}

static void
gimp_polygon_select_tool_init (GimpPolygonSelectTool *poly_sel)
{
  GimpTool          *tool     = GIMP_TOOL (poly_sel);
  GimpSelectionTool *sel_tool = GIMP_SELECTION_TOOL (tool);

  poly_sel->priv = gimp_polygon_select_tool_get_instance_private (poly_sel);

  gimp_tool_control_set_motion_mode        (tool->control,
                                            GIMP_MOTION_MODE_EXACT);
  gimp_tool_control_set_wants_click        (tool->control, TRUE);
  gimp_tool_control_set_wants_double_click (tool->control, TRUE);
  gimp_tool_control_set_active_modifiers   (tool->control,
                                            GIMP_TOOL_ACTIVE_MODIFIERS_SEPARATE);
  gimp_tool_control_set_precision          (tool->control,
                                            GIMP_CURSOR_PRECISION_SUBPIXEL);

  sel_tool->allow_move = FALSE;
}

static void
gimp_polygon_select_tool_finalize (GObject *object)
{
  GimpPolygonSelectTool        *poly_sel = GIMP_POLYGON_SELECT_TOOL (object);
  GimpPolygonSelectToolPrivate *priv     = poly_sel->priv;

  g_clear_object (&priv->widget);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
gimp_polygon_select_tool_control (GimpTool       *tool,
                                  GimpToolAction  action,
                                  GimpDisplay    *display)
{
  GimpPolygonSelectTool *poly_sel = GIMP_POLYGON_SELECT_TOOL (tool);

  switch (action)
    {
    case GIMP_TOOL_ACTION_PAUSE:
    case GIMP_TOOL_ACTION_RESUME:
      break;

    case GIMP_TOOL_ACTION_HALT:
      gimp_polygon_select_tool_halt (poly_sel);
      break;

    case GIMP_TOOL_ACTION_COMMIT:
      break;
    }

  GIMP_TOOL_CLASS (parent_class)->control (tool, action, display);
}

static void
gimp_polygon_select_tool_button_press (GimpTool            *tool,
                                       const GimpCoords    *coords,
                                       guint32              time,
                                       GdkModifierType      state,
                                       GimpButtonPressType  press_type,
                                       GimpDisplay         *display)
{
  GimpPolygonSelectTool        *poly_sel = GIMP_POLYGON_SELECT_TOOL (tool);
  GimpPolygonSelectToolPrivate *priv     = poly_sel->priv;

  if (tool->display && tool->display != display)
    gimp_tool_control (tool, GIMP_TOOL_ACTION_COMMIT, tool->display);

  if (! priv->widget) /* not tool->display, we have a subclass */
    {
      /* First of all handle delegation to the selection mask edit logic
       * if appropriate.
       */
      if (gimp_selection_tool_start_edit (GIMP_SELECTION_TOOL (poly_sel),
                                          display, coords))
        {
          return;
        }

      gimp_polygon_select_tool_start (poly_sel, display);

      gimp_tool_widget_hover (priv->widget, coords, state, TRUE);
    }

  if (gimp_tool_widget_button_press (priv->widget, coords, time, state,
                                     press_type))
    {
      priv->grab_widget = priv->widget;
    }

  if (press_type == GIMP_BUTTON_PRESS_NORMAL)
    gimp_tool_control_activate (tool->control);
}

static void
gimp_polygon_select_tool_button_release (GimpTool              *tool,
                                         const GimpCoords      *coords,
                                         guint32                time,
                                         GdkModifierType        state,
                                         GimpButtonReleaseType  release_type,
                                         GimpDisplay           *display)
{
  GimpPolygonSelectTool        *poly_sel = GIMP_POLYGON_SELECT_TOOL (tool);
  GimpPolygonSelectToolPrivate *priv     = poly_sel->priv;
  GimpImage                    *image    = gimp_display_get_image (display);

  gimp_tool_control_halt (tool->control);

  switch (release_type)
    {
    case GIMP_BUTTON_RELEASE_CLICK:
    case GIMP_BUTTON_RELEASE_NO_MOTION:
      /*  If there is a floating selection, anchor it  */
      if (gimp_image_get_floating_selection (image))
        {
          floating_sel_anchor (gimp_image_get_floating_selection (image));

          gimp_tool_control (tool, GIMP_TOOL_ACTION_HALT, display);

          return;
        }

      /* fallthru */

    default:
      if (priv->grab_widget)
        {
          gimp_tool_widget_button_release (priv->grab_widget,
                                           coords, time, state, release_type);
          priv->grab_widget = NULL;
        }
    }

  if (priv->pending_response)
    {
      gimp_polygon_select_tool_polygon_response (priv->widget,
                                                 priv->pending_response_id,
                                                 poly_sel);

      priv->pending_response = FALSE;
    }
}

static void
gimp_polygon_select_tool_motion (GimpTool         *tool,
                                 const GimpCoords *coords,
                                 guint32           time,
                                 GdkModifierType   state,
                                 GimpDisplay      *display)
{
  GimpPolygonSelectTool        *poly_sel = GIMP_POLYGON_SELECT_TOOL (tool);
  GimpPolygonSelectToolPrivate *priv     = poly_sel->priv;

  if (priv->grab_widget)
    {
      gimp_tool_widget_motion (priv->grab_widget, coords, time, state);
    }
}

static gboolean
gimp_polygon_select_tool_key_press (GimpTool    *tool,
                                    GdkEventKey *kevent,
                                    GimpDisplay *display)
{
  GimpPolygonSelectTool        *poly_sel = GIMP_POLYGON_SELECT_TOOL (tool);
  GimpPolygonSelectToolPrivate *priv     = poly_sel->priv;

  if (priv->widget && display == tool->display)
    {
      return gimp_tool_widget_key_press (priv->widget, kevent);
    }

  return FALSE;
}

static void
gimp_polygon_select_tool_modifier_key (GimpTool        *tool,
                                       GdkModifierType  key,
                                       gboolean         press,
                                       GdkModifierType  state,
                                       GimpDisplay     *display)
{
  GimpPolygonSelectTool        *poly_sel = GIMP_POLYGON_SELECT_TOOL (tool);
  GimpPolygonSelectToolPrivate *priv     = poly_sel->priv;

  if (priv->widget && display == tool->display)
    {
      gimp_tool_widget_hover_modifier (priv->widget, key, press, state);

      /* let GimpSelectTool handle alt+<mod> */
      if (! (state & GDK_MOD1_MASK))
        {
          /* otherwise, shift/ctrl are handled by the widget */
          state &= ~(gimp_get_extend_selection_mask () |
                     gimp_get_modify_selection_mask ());
        }
    }

  GIMP_TOOL_CLASS (parent_class)->modifier_key (tool, key, press, state,
                                                display);
}

static void
gimp_polygon_select_tool_oper_update (GimpTool         *tool,
                                      const GimpCoords *coords,
                                      GdkModifierType   state,
                                      gboolean          proximity,
                                      GimpDisplay      *display)
{
  GimpPolygonSelectTool        *poly_sel = GIMP_POLYGON_SELECT_TOOL (tool);
  GimpPolygonSelectToolPrivate *priv     = poly_sel->priv;

  if (priv->widget && display == tool->display)
    {
      gimp_tool_widget_hover (priv->widget, coords, state, proximity);

      /* let GimpSelectTool handle alt+<mod> */
      if (! (state & GDK_MOD1_MASK))
        {
          /* otherwise, shift/ctrl are handled by the widget */
          state &= ~(gimp_get_extend_selection_mask () |
                     gimp_get_modify_selection_mask ());
        }
    }

  GIMP_TOOL_CLASS (parent_class)->oper_update (tool, coords, state, proximity,
                                               display);
}

static void
gimp_polygon_select_tool_cursor_update (GimpTool         *tool,
                                        const GimpCoords *coords,
                                        GdkModifierType   state,
                                        GimpDisplay      *display)
{
  GimpPolygonSelectTool        *poly_sel = GIMP_POLYGON_SELECT_TOOL (tool);
  GimpPolygonSelectToolPrivate *priv     = poly_sel->priv;
  GimpCursorModifier            modifier = GIMP_CURSOR_MODIFIER_NONE;

  if (tool->display)
    {
      if (priv->widget && display == tool->display)
        {
          gimp_tool_widget_get_cursor (priv->widget, coords, state,
                                       NULL, NULL, &modifier);

          /* let GimpSelectTool handle alt+<mod> */
          if (! (state & GDK_MOD1_MASK))
            {
              /* otherwise, shift/ctrl are handled by the widget */
              state &= ~(gimp_get_extend_selection_mask () |
                         gimp_get_modify_selection_mask ());
            }
        }

      gimp_tool_set_cursor (tool, display,
                            gimp_tool_control_get_cursor (tool->control),
                            gimp_tool_control_get_tool_cursor (tool->control),
                            modifier);
    }

  GIMP_TOOL_CLASS (parent_class)->cursor_update (tool, coords, state,
                                                 display);
}

static void
gimp_polygon_select_tool_real_confirm (GimpPolygonSelectTool *poly_sel,
                                       GimpDisplay           *display)
{
  gimp_tool_control (GIMP_TOOL (poly_sel), GIMP_TOOL_ACTION_COMMIT, display);
}

static void
gimp_polygon_select_tool_polygon_change_complete (GimpToolWidget        *polygon,
                                                  GimpPolygonSelectTool *poly_sel)
{
  if (GIMP_POLYGON_SELECT_TOOL_GET_CLASS (poly_sel)->change_complete)
    {
      GIMP_POLYGON_SELECT_TOOL_GET_CLASS (poly_sel)->change_complete (
        poly_sel, GIMP_TOOL (poly_sel)->display);
    }
}

static void
gimp_polygon_select_tool_polygon_response (GimpToolWidget        *polygon,
                                           gint                   response_id,
                                           GimpPolygonSelectTool *poly_sel)
{
  GimpTool                     *tool = GIMP_TOOL (poly_sel);
  GimpPolygonSelectToolPrivate *priv = poly_sel->priv;

  /* if we're in the middle of a click, defer the response to the
   * button_release() event
   */
  if (gimp_tool_control_is_active (tool->control))
    {
      priv->pending_response    = TRUE;
      priv->pending_response_id = response_id;

      return;
    }

  switch (response_id)
    {
    case GIMP_TOOL_WIDGET_RESPONSE_CONFIRM:
      /*  don't gimp_tool_control(COMMIT) here because we don't always
       *  want to HALT the tool after committing because we have a
       *  subclass, we do that in the default implementation of
       *  confirm().
       */
      if (GIMP_POLYGON_SELECT_TOOL_GET_CLASS (poly_sel)->confirm)
        {
          GIMP_POLYGON_SELECT_TOOL_GET_CLASS (poly_sel)->confirm (
            poly_sel, tool->display);
        }
      break;

    case GIMP_TOOL_WIDGET_RESPONSE_CANCEL:
      gimp_tool_control (tool, GIMP_TOOL_ACTION_HALT, tool->display);
      break;
    }
}

static void
gimp_polygon_select_tool_start (GimpPolygonSelectTool *poly_sel,
                                GimpDisplay           *display)
{
  GimpTool                     *tool  = GIMP_TOOL (poly_sel);
  GimpPolygonSelectToolPrivate *priv  = poly_sel->priv;
  GimpDisplayShell             *shell = gimp_display_get_shell (display);

  tool->display = display;

  priv->widget = gimp_tool_polygon_new (shell);

  gimp_draw_tool_set_widget (GIMP_DRAW_TOOL (tool), priv->widget);

  g_signal_connect (priv->widget, "change-complete",
                    G_CALLBACK (gimp_polygon_select_tool_polygon_change_complete),
                    poly_sel);
  g_signal_connect (priv->widget, "response",
                    G_CALLBACK (gimp_polygon_select_tool_polygon_response),
                    poly_sel);

  gimp_draw_tool_start (GIMP_DRAW_TOOL (tool), display);
}


/*  public functions  */

gboolean
gimp_polygon_select_tool_is_closed (GimpPolygonSelectTool *poly_sel)
{
  GimpPolygonSelectToolPrivate *priv;

  g_return_val_if_fail (GIMP_IS_POLYGON_SELECT_TOOL (poly_sel), FALSE);

  priv = poly_sel->priv;

  if (priv->widget)
    return gimp_tool_polygon_is_closed (GIMP_TOOL_POLYGON (priv->widget));

  return FALSE;
}

void
gimp_polygon_select_tool_get_points (GimpPolygonSelectTool  *poly_sel,
                                     const GimpVector2     **points,
                                     gint                   *n_points)
{
  GimpPolygonSelectToolPrivate *priv;

  g_return_if_fail (GIMP_IS_POLYGON_SELECT_TOOL (poly_sel));

  priv = poly_sel->priv;

  if (priv->widget)
    {
      gimp_tool_polygon_get_points (GIMP_TOOL_POLYGON (priv->widget),
                                    points, n_points);
    }
  else
    {
      if (points)   *points   = NULL;
      if (n_points) *n_points = 0;
    }
}


/*  protected functions  */

gboolean
gimp_polygon_select_tool_is_grabbed (GimpPolygonSelectTool *poly_sel)
{
  GimpPolygonSelectToolPrivate *priv;

  g_return_val_if_fail (GIMP_IS_POLYGON_SELECT_TOOL (poly_sel), FALSE);

  priv = poly_sel->priv;

  return priv->grab_widget != NULL;
}

void
gimp_polygon_select_tool_halt (GimpPolygonSelectTool *poly_sel)
{
  GimpPolygonSelectToolPrivate *priv;

  g_return_if_fail (GIMP_IS_POLYGON_SELECT_TOOL (poly_sel));

  priv = poly_sel->priv;

  gimp_draw_tool_set_widget (GIMP_DRAW_TOOL (poly_sel), NULL);
  g_clear_object (&priv->widget);
}
