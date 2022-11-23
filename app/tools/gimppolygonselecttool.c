/* LIGMA - The GNU Image Manipulation Program
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

#include "libligmawidgets/ligmawidgets.h"

#include "tools-types.h"

#include "core/ligmachannel.h"
#include "core/ligmachannel-select.h"
#include "core/ligmaimage.h"
#include "core/ligmalayer-floating-selection.h"

#include "widgets/ligmahelp-ids.h"
#include "widgets/ligmawidgets-utils.h"

#include "display/ligmadisplay.h"
#include "display/ligmatoolpolygon.h"

#include "ligmapolygonselecttool.h"
#include "ligmaselectionoptions.h"
#include "ligmatoolcontrol.h"


struct _LigmaPolygonSelectToolPrivate
{
  LigmaToolWidget *widget;
  LigmaToolWidget *grab_widget;

  gboolean        pending_response;
  gint            pending_response_id;
};


/*  local function prototypes  */

static void       ligma_polygon_select_tool_finalize                (GObject               *object);

static void       ligma_polygon_select_tool_control                 (LigmaTool              *tool,
                                                                    LigmaToolAction         action,
                                                                    LigmaDisplay           *display);
static void       ligma_polygon_select_tool_button_press            (LigmaTool              *tool,
                                                                    const LigmaCoords      *coords,
                                                                    guint32                time,
                                                                    GdkModifierType        state,
                                                                    LigmaButtonPressType    press_type,
                                                                    LigmaDisplay           *display);
static void       ligma_polygon_select_tool_button_release          (LigmaTool              *tool,
                                                                    const LigmaCoords      *coords,
                                                                    guint32                time,
                                                                    GdkModifierType        state,
                                                                    LigmaButtonReleaseType  release_type,
                                                                    LigmaDisplay           *display);
static void       ligma_polygon_select_tool_motion                  (LigmaTool              *tool,
                                                                    const LigmaCoords      *coords,
                                                                    guint32                time,
                                                                    GdkModifierType        state,
                                                                    LigmaDisplay           *display);
static gboolean   ligma_polygon_select_tool_key_press               (LigmaTool              *tool,
                                                                    GdkEventKey           *kevent,
                                                                    LigmaDisplay           *display);
static void       ligma_polygon_select_tool_modifier_key            (LigmaTool              *tool,
                                                                    GdkModifierType        key,
                                                                    gboolean               press,
                                                                    GdkModifierType        state,
                                                                    LigmaDisplay           *display);
static void       ligma_polygon_select_tool_oper_update             (LigmaTool              *tool,
                                                                    const LigmaCoords      *coords,
                                                                    GdkModifierType        state,
                                                                    gboolean               proximity,
                                                                    LigmaDisplay           *display);
static void       ligma_polygon_select_tool_cursor_update           (LigmaTool              *tool,
                                                                    const LigmaCoords      *coords,
                                                                    GdkModifierType        state,
                                                                    LigmaDisplay           *display);

static void       ligma_polygon_select_tool_real_confirm            (LigmaPolygonSelectTool *poly_sel,
                                                                    LigmaDisplay           *display);

static void       ligma_polygon_select_tool_polygon_change_complete (LigmaToolWidget        *polygon,
                                                                    LigmaPolygonSelectTool *poly_sel);
static void       ligma_polygon_select_tool_polygon_response        (LigmaToolWidget        *polygon,
                                                                    gint                   response_id,
                                                                    LigmaPolygonSelectTool *poly_sel);

static void       ligma_polygon_select_tool_start                   (LigmaPolygonSelectTool *poly_sel,
                                                                    LigmaDisplay           *display);


G_DEFINE_TYPE_WITH_PRIVATE (LigmaPolygonSelectTool, ligma_polygon_select_tool,
                            LIGMA_TYPE_SELECTION_TOOL)

#define parent_class ligma_polygon_select_tool_parent_class


/*  private functions  */

static void
ligma_polygon_select_tool_class_init (LigmaPolygonSelectToolClass *klass)
{
  GObjectClass  *object_class = G_OBJECT_CLASS (klass);
  LigmaToolClass *tool_class   = LIGMA_TOOL_CLASS (klass);

  object_class->finalize     = ligma_polygon_select_tool_finalize;

  tool_class->control        = ligma_polygon_select_tool_control;
  tool_class->button_press   = ligma_polygon_select_tool_button_press;
  tool_class->button_release = ligma_polygon_select_tool_button_release;
  tool_class->motion         = ligma_polygon_select_tool_motion;
  tool_class->key_press      = ligma_polygon_select_tool_key_press;
  tool_class->modifier_key   = ligma_polygon_select_tool_modifier_key;
  tool_class->oper_update    = ligma_polygon_select_tool_oper_update;
  tool_class->cursor_update  = ligma_polygon_select_tool_cursor_update;

  klass->change_complete     = NULL;
  klass->confirm             = ligma_polygon_select_tool_real_confirm;
}

static void
ligma_polygon_select_tool_init (LigmaPolygonSelectTool *poly_sel)
{
  LigmaTool          *tool     = LIGMA_TOOL (poly_sel);
  LigmaSelectionTool *sel_tool = LIGMA_SELECTION_TOOL (tool);

  poly_sel->priv = ligma_polygon_select_tool_get_instance_private (poly_sel);

  ligma_tool_control_set_motion_mode        (tool->control,
                                            LIGMA_MOTION_MODE_EXACT);
  ligma_tool_control_set_wants_click        (tool->control, TRUE);
  ligma_tool_control_set_wants_double_click (tool->control, TRUE);
  ligma_tool_control_set_active_modifiers   (tool->control,
                                            LIGMA_TOOL_ACTIVE_MODIFIERS_SEPARATE);
  ligma_tool_control_set_precision          (tool->control,
                                            LIGMA_CURSOR_PRECISION_SUBPIXEL);

  sel_tool->allow_move = FALSE;
}

static void
ligma_polygon_select_tool_finalize (GObject *object)
{
  LigmaPolygonSelectTool        *poly_sel = LIGMA_POLYGON_SELECT_TOOL (object);
  LigmaPolygonSelectToolPrivate *priv     = poly_sel->priv;

  g_clear_object (&priv->widget);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
ligma_polygon_select_tool_control (LigmaTool       *tool,
                                  LigmaToolAction  action,
                                  LigmaDisplay    *display)
{
  LigmaPolygonSelectTool *poly_sel = LIGMA_POLYGON_SELECT_TOOL (tool);

  switch (action)
    {
    case LIGMA_TOOL_ACTION_PAUSE:
    case LIGMA_TOOL_ACTION_RESUME:
      break;

    case LIGMA_TOOL_ACTION_HALT:
      ligma_polygon_select_tool_halt (poly_sel);
      break;

    case LIGMA_TOOL_ACTION_COMMIT:
      break;
    }

  LIGMA_TOOL_CLASS (parent_class)->control (tool, action, display);
}

static void
ligma_polygon_select_tool_button_press (LigmaTool            *tool,
                                       const LigmaCoords    *coords,
                                       guint32              time,
                                       GdkModifierType      state,
                                       LigmaButtonPressType  press_type,
                                       LigmaDisplay         *display)
{
  LigmaPolygonSelectTool        *poly_sel = LIGMA_POLYGON_SELECT_TOOL (tool);
  LigmaPolygonSelectToolPrivate *priv     = poly_sel->priv;

  if (tool->display && tool->display != display)
    ligma_tool_control (tool, LIGMA_TOOL_ACTION_COMMIT, tool->display);

  if (! priv->widget) /* not tool->display, we have a subclass */
    {
      /* First of all handle delegation to the selection mask edit logic
       * if appropriate.
       */
      if (ligma_selection_tool_start_edit (LIGMA_SELECTION_TOOL (poly_sel),
                                          display, coords))
        {
          return;
        }

      ligma_polygon_select_tool_start (poly_sel, display);

      ligma_tool_widget_hover (priv->widget, coords, state, TRUE);
    }

  if (ligma_tool_widget_button_press (priv->widget, coords, time, state,
                                     press_type))
    {
      priv->grab_widget = priv->widget;
    }

  if (press_type == LIGMA_BUTTON_PRESS_NORMAL)
    ligma_tool_control_activate (tool->control);
}

static void
ligma_polygon_select_tool_button_release (LigmaTool              *tool,
                                         const LigmaCoords      *coords,
                                         guint32                time,
                                         GdkModifierType        state,
                                         LigmaButtonReleaseType  release_type,
                                         LigmaDisplay           *display)
{
  LigmaPolygonSelectTool        *poly_sel = LIGMA_POLYGON_SELECT_TOOL (tool);
  LigmaPolygonSelectToolPrivate *priv     = poly_sel->priv;
  LigmaImage                    *image    = ligma_display_get_image (display);

  ligma_tool_control_halt (tool->control);

  switch (release_type)
    {
    case LIGMA_BUTTON_RELEASE_CLICK:
    case LIGMA_BUTTON_RELEASE_NO_MOTION:
      /*  If there is a floating selection, anchor it  */
      if (ligma_image_get_floating_selection (image))
        {
          floating_sel_anchor (ligma_image_get_floating_selection (image));

          ligma_tool_control (tool, LIGMA_TOOL_ACTION_HALT, display);

          return;
        }

      /* fallthru */

    default:
      if (priv->grab_widget)
        {
          ligma_tool_widget_button_release (priv->grab_widget,
                                           coords, time, state, release_type);
          priv->grab_widget = NULL;
        }
    }

  if (priv->pending_response)
    {
      ligma_polygon_select_tool_polygon_response (priv->widget,
                                                 priv->pending_response_id,
                                                 poly_sel);

      priv->pending_response = FALSE;
    }
}

static void
ligma_polygon_select_tool_motion (LigmaTool         *tool,
                                 const LigmaCoords *coords,
                                 guint32           time,
                                 GdkModifierType   state,
                                 LigmaDisplay      *display)
{
  LigmaPolygonSelectTool        *poly_sel = LIGMA_POLYGON_SELECT_TOOL (tool);
  LigmaPolygonSelectToolPrivate *priv     = poly_sel->priv;

  if (priv->grab_widget)
    {
      ligma_tool_widget_motion (priv->grab_widget, coords, time, state);
    }
}

static gboolean
ligma_polygon_select_tool_key_press (LigmaTool    *tool,
                                    GdkEventKey *kevent,
                                    LigmaDisplay *display)
{
  LigmaPolygonSelectTool        *poly_sel = LIGMA_POLYGON_SELECT_TOOL (tool);
  LigmaPolygonSelectToolPrivate *priv     = poly_sel->priv;

  if (priv->widget && display == tool->display)
    {
      return ligma_tool_widget_key_press (priv->widget, kevent);
    }

  return FALSE;
}

static void
ligma_polygon_select_tool_modifier_key (LigmaTool        *tool,
                                       GdkModifierType  key,
                                       gboolean         press,
                                       GdkModifierType  state,
                                       LigmaDisplay     *display)
{
  LigmaPolygonSelectTool        *poly_sel = LIGMA_POLYGON_SELECT_TOOL (tool);
  LigmaPolygonSelectToolPrivate *priv     = poly_sel->priv;

  if (priv->widget && display == tool->display)
    {
      ligma_tool_widget_hover_modifier (priv->widget, key, press, state);

      /* let LigmaSelectTool handle alt+<mod> */
      if (! (state & GDK_MOD1_MASK))
        {
          /* otherwise, shift/ctrl are handled by the widget */
          state &= ~(ligma_get_extend_selection_mask () |
                     ligma_get_modify_selection_mask ());
        }
    }

  LIGMA_TOOL_CLASS (parent_class)->modifier_key (tool, key, press, state,
                                                display);
}

static void
ligma_polygon_select_tool_oper_update (LigmaTool         *tool,
                                      const LigmaCoords *coords,
                                      GdkModifierType   state,
                                      gboolean          proximity,
                                      LigmaDisplay      *display)
{
  LigmaPolygonSelectTool        *poly_sel = LIGMA_POLYGON_SELECT_TOOL (tool);
  LigmaPolygonSelectToolPrivate *priv     = poly_sel->priv;

  if (priv->widget && display == tool->display)
    {
      ligma_tool_widget_hover (priv->widget, coords, state, proximity);

      /* let LigmaSelectTool handle alt+<mod> */
      if (! (state & GDK_MOD1_MASK))
        {
          /* otherwise, shift/ctrl are handled by the widget */
          state &= ~(ligma_get_extend_selection_mask () |
                     ligma_get_modify_selection_mask ());
        }
    }

  LIGMA_TOOL_CLASS (parent_class)->oper_update (tool, coords, state, proximity,
                                               display);
}

static void
ligma_polygon_select_tool_cursor_update (LigmaTool         *tool,
                                        const LigmaCoords *coords,
                                        GdkModifierType   state,
                                        LigmaDisplay      *display)
{
  LigmaPolygonSelectTool        *poly_sel = LIGMA_POLYGON_SELECT_TOOL (tool);
  LigmaPolygonSelectToolPrivate *priv     = poly_sel->priv;
  LigmaCursorModifier            modifier = LIGMA_CURSOR_MODIFIER_NONE;

  if (tool->display)
    {
      if (priv->widget && display == tool->display)
        {
          ligma_tool_widget_get_cursor (priv->widget, coords, state,
                                       NULL, NULL, &modifier);

          /* let LigmaSelectTool handle alt+<mod> */
          if (! (state & GDK_MOD1_MASK))
            {
              /* otherwise, shift/ctrl are handled by the widget */
              state &= ~(ligma_get_extend_selection_mask () |
                         ligma_get_modify_selection_mask ());
            }
        }

      ligma_tool_set_cursor (tool, display,
                            ligma_tool_control_get_cursor (tool->control),
                            ligma_tool_control_get_tool_cursor (tool->control),
                            modifier);
    }

  LIGMA_TOOL_CLASS (parent_class)->cursor_update (tool, coords, state,
                                                 display);
}

static void
ligma_polygon_select_tool_real_confirm (LigmaPolygonSelectTool *poly_sel,
                                       LigmaDisplay           *display)
{
  ligma_tool_control (LIGMA_TOOL (poly_sel), LIGMA_TOOL_ACTION_COMMIT, display);
}

static void
ligma_polygon_select_tool_polygon_change_complete (LigmaToolWidget        *polygon,
                                                  LigmaPolygonSelectTool *poly_sel)
{
  if (LIGMA_POLYGON_SELECT_TOOL_GET_CLASS (poly_sel)->change_complete)
    {
      LIGMA_POLYGON_SELECT_TOOL_GET_CLASS (poly_sel)->change_complete (
        poly_sel, LIGMA_TOOL (poly_sel)->display);
    }
}

static void
ligma_polygon_select_tool_polygon_response (LigmaToolWidget        *polygon,
                                           gint                   response_id,
                                           LigmaPolygonSelectTool *poly_sel)
{
  LigmaTool                     *tool = LIGMA_TOOL (poly_sel);
  LigmaPolygonSelectToolPrivate *priv = poly_sel->priv;

  /* if we're in the middle of a click, defer the response to the
   * button_release() event
   */
  if (ligma_tool_control_is_active (tool->control))
    {
      priv->pending_response    = TRUE;
      priv->pending_response_id = response_id;

      return;
    }

  switch (response_id)
    {
    case LIGMA_TOOL_WIDGET_RESPONSE_CONFIRM:
      /*  don't ligma_tool_control(COMMIT) here because we don't always
       *  want to HALT the tool after committing because we have a
       *  subclass, we do that in the default implementation of
       *  confirm().
       */
      if (LIGMA_POLYGON_SELECT_TOOL_GET_CLASS (poly_sel)->confirm)
        {
          LIGMA_POLYGON_SELECT_TOOL_GET_CLASS (poly_sel)->confirm (
            poly_sel, tool->display);
        }
      break;

    case LIGMA_TOOL_WIDGET_RESPONSE_CANCEL:
      ligma_tool_control (tool, LIGMA_TOOL_ACTION_HALT, tool->display);
      break;
    }
}

static void
ligma_polygon_select_tool_start (LigmaPolygonSelectTool *poly_sel,
                                LigmaDisplay           *display)
{
  LigmaTool                     *tool  = LIGMA_TOOL (poly_sel);
  LigmaPolygonSelectToolPrivate *priv  = poly_sel->priv;
  LigmaDisplayShell             *shell = ligma_display_get_shell (display);

  tool->display = display;

  priv->widget = ligma_tool_polygon_new (shell);

  ligma_draw_tool_set_widget (LIGMA_DRAW_TOOL (tool), priv->widget);

  g_signal_connect (priv->widget, "change-complete",
                    G_CALLBACK (ligma_polygon_select_tool_polygon_change_complete),
                    poly_sel);
  g_signal_connect (priv->widget, "response",
                    G_CALLBACK (ligma_polygon_select_tool_polygon_response),
                    poly_sel);

  ligma_draw_tool_start (LIGMA_DRAW_TOOL (tool), display);
}


/*  public functions  */

gboolean
ligma_polygon_select_tool_is_closed (LigmaPolygonSelectTool *poly_sel)
{
  LigmaPolygonSelectToolPrivate *priv;

  g_return_val_if_fail (LIGMA_IS_POLYGON_SELECT_TOOL (poly_sel), FALSE);

  priv = poly_sel->priv;

  if (priv->widget)
    return ligma_tool_polygon_is_closed (LIGMA_TOOL_POLYGON (priv->widget));

  return FALSE;
}

void
ligma_polygon_select_tool_get_points (LigmaPolygonSelectTool  *poly_sel,
                                     const LigmaVector2     **points,
                                     gint                   *n_points)
{
  LigmaPolygonSelectToolPrivate *priv;

  g_return_if_fail (LIGMA_IS_POLYGON_SELECT_TOOL (poly_sel));

  priv = poly_sel->priv;

  if (priv->widget)
    {
      ligma_tool_polygon_get_points (LIGMA_TOOL_POLYGON (priv->widget),
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
ligma_polygon_select_tool_is_grabbed (LigmaPolygonSelectTool *poly_sel)
{
  LigmaPolygonSelectToolPrivate *priv;

  g_return_val_if_fail (LIGMA_IS_POLYGON_SELECT_TOOL (poly_sel), FALSE);

  priv = poly_sel->priv;

  return priv->grab_widget != NULL;
}

void
ligma_polygon_select_tool_halt (LigmaPolygonSelectTool *poly_sel)
{
  LigmaPolygonSelectToolPrivate *priv;

  g_return_if_fail (LIGMA_IS_POLYGON_SELECT_TOOL (poly_sel));

  priv = poly_sel->priv;

  ligma_draw_tool_set_widget (LIGMA_DRAW_TOOL (poly_sel), NULL);
  g_clear_object (&priv->widget);
}
