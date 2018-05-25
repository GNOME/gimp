/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * Major improvement to support polygonal segments
 * Copyright (C) 2008 Martin Nordholts
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

#include <gegl.h>
#include <gtk/gtk.h>

#include "libgimpwidgets/gimpwidgets.h"

#include "tools-types.h"

#include "core/gimpchannel.h"
#include "core/gimpchannel-select.h"
#include "core/gimpimage.h"
#include "core/gimplayer-floating-selection.h"

#include "widgets/gimphelp-ids.h"

#include "display/gimpdisplay.h"
#include "display/gimptoolpolygon.h"

#include "gimpfreeselecttool.h"
#include "gimpselectionoptions.h"
#include "gimptoolcontrol.h"

#include "gimp-intl.h"


struct _GimpFreeSelectToolPrivate
{
  /* The selection operation active when the tool was started */
  GimpChannelOps  operation_at_start;

  GimpToolWidget *widget;
  GimpToolWidget *grab_widget;
};


static void     gimp_free_select_tool_finalize            (GObject               *object);
static void     gimp_free_select_tool_control             (GimpTool              *tool,
                                                           GimpToolAction         action,
                                                           GimpDisplay           *display);
static void     gimp_free_select_tool_oper_update         (GimpTool              *tool,
                                                           const GimpCoords      *coords,
                                                           GdkModifierType        state,
                                                           gboolean               proximity,
                                                           GimpDisplay           *display);
static void     gimp_free_select_tool_cursor_update       (GimpTool              *tool,
                                                           const GimpCoords      *coords,
                                                           GdkModifierType        state,
                                                           GimpDisplay           *display);
static void     gimp_free_select_tool_button_press        (GimpTool              *tool,
                                                           const GimpCoords      *coords,
                                                           guint32                time,
                                                           GdkModifierType        state,
                                                           GimpButtonPressType    press_type,
                                                           GimpDisplay           *display);
static void     gimp_free_select_tool_button_release      (GimpTool              *tool,
                                                           const GimpCoords      *coords,
                                                           guint32                time,
                                                           GdkModifierType        state,
                                                           GimpButtonReleaseType  release_type,
                                                           GimpDisplay           *display);
static void     gimp_free_select_tool_motion              (GimpTool              *tool,
                                                           const GimpCoords      *coords,
                                                           guint32                time,
                                                           GdkModifierType        state,
                                                           GimpDisplay           *display);
static gboolean gimp_free_select_tool_key_press           (GimpTool              *tool,
                                                           GdkEventKey           *kevent,
                                                           GimpDisplay           *display);
static void     gimp_free_select_tool_modifier_key        (GimpTool              *tool,
                                                           GdkModifierType        key,
                                                           gboolean               press,
                                                           GdkModifierType        state,
                                                           GimpDisplay           *display);
static void     gimp_free_select_tool_active_modifier_key (GimpTool              *tool,
                                                           GdkModifierType        key,
                                                           gboolean               press,
                                                           GdkModifierType        state,
                                                           GimpDisplay           *display);

static void     gimp_free_select_tool_real_select         (GimpFreeSelectTool    *fst,
                                                           GimpDisplay           *display,
                                                           const GimpVector2     *points,
                                                           gint                   n_points);

static void     gimp_free_select_tool_polygon_changed     (GimpToolWidget        *polygon,
                                                           GimpFreeSelectTool    *fst);
static void     gimp_free_select_tool_polygon_response    (GimpToolWidget        *polygon,
                                                           gint                   response_id,
                                                           GimpFreeSelectTool    *fst);


G_DEFINE_TYPE (GimpFreeSelectTool, gimp_free_select_tool,
               GIMP_TYPE_SELECTION_TOOL);

#define parent_class gimp_free_select_tool_parent_class


void
gimp_free_select_tool_register (GimpToolRegisterCallback  callback,
                                gpointer                  data)
{
  (* callback) (GIMP_TYPE_FREE_SELECT_TOOL,
                GIMP_TYPE_SELECTION_OPTIONS,
                gimp_selection_options_gui,
                0,
                "gimp-free-select-tool",
                _("Free Select"),
                _("Free Select Tool: Select a hand-drawn region with free "
                  "and polygonal segments"),
                N_("_Free Select"), "F",
                NULL, GIMP_HELP_TOOL_FREE_SELECT,
                GIMP_ICON_TOOL_FREE_SELECT,
                data);
}

gint
gimp_free_select_tool_get_n_points (GimpFreeSelectTool *tool)
{
  GimpFreeSelectToolPrivate *private = tool->private;
  const GimpVector2         *points;
  gint                       n_points = 0;

  if (private->widget)
    gimp_tool_polygon_get_points (GIMP_TOOL_POLYGON (private->widget),
                                  &points, &n_points);

  return n_points;
}

static void
gimp_free_select_tool_class_init (GimpFreeSelectToolClass *klass)
{
  GObjectClass  *object_class = G_OBJECT_CLASS (klass);
  GimpToolClass *tool_class   = GIMP_TOOL_CLASS (klass);

  object_class->finalize          = gimp_free_select_tool_finalize;

  tool_class->control             = gimp_free_select_tool_control;
  tool_class->oper_update         = gimp_free_select_tool_oper_update;
  tool_class->cursor_update       = gimp_free_select_tool_cursor_update;
  tool_class->button_press        = gimp_free_select_tool_button_press;
  tool_class->button_release      = gimp_free_select_tool_button_release;
  tool_class->motion              = gimp_free_select_tool_motion;
  tool_class->key_press           = gimp_free_select_tool_key_press;
  tool_class->modifier_key        = gimp_free_select_tool_modifier_key;
  tool_class->active_modifier_key = gimp_free_select_tool_active_modifier_key;

  klass->select                   = gimp_free_select_tool_real_select;

  g_type_class_add_private (klass, sizeof (GimpFreeSelectToolPrivate));
}

static void
gimp_free_select_tool_init (GimpFreeSelectTool *fst)
{
  GimpTool *tool = GIMP_TOOL (fst);

  fst->private = G_TYPE_INSTANCE_GET_PRIVATE (fst,
                                              GIMP_TYPE_FREE_SELECT_TOOL,
                                              GimpFreeSelectToolPrivate);

  gimp_tool_control_set_motion_mode        (tool->control,
                                            GIMP_MOTION_MODE_EXACT);
  gimp_tool_control_set_wants_click        (tool->control, TRUE);
  gimp_tool_control_set_wants_double_click (tool->control, TRUE);
  gimp_tool_control_set_active_modifiers   (tool->control,
                                            GIMP_TOOL_ACTIVE_MODIFIERS_SEPARATE);
  gimp_tool_control_set_precision          (tool->control,
                                            GIMP_CURSOR_PRECISION_SUBPIXEL);
  gimp_tool_control_set_tool_cursor        (tool->control,
                                            GIMP_TOOL_CURSOR_FREE_SELECT);
}

static void
gimp_free_select_tool_finalize (GObject *object)
{
  GimpFreeSelectTool        *fst  = GIMP_FREE_SELECT_TOOL (object);
  GimpFreeSelectToolPrivate *priv = fst->private;

  g_clear_object (&priv->widget);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
gimp_free_select_tool_start (GimpFreeSelectTool *fst,
                             GimpDisplay        *display)
{
  GimpTool                  *tool    = GIMP_TOOL (fst);
  GimpFreeSelectToolPrivate *private = fst->private;
  GimpSelectionOptions      *options = GIMP_SELECTION_TOOL_GET_OPTIONS (tool);
  GimpDisplayShell          *shell   = gimp_display_get_shell (display);

  tool->display = display;

  /* We want to apply the selection operation that was current when
   * the tool was started, so we save this information
   */
  private->operation_at_start = options->operation;

  private->widget = gimp_tool_polygon_new (shell);

  gimp_draw_tool_set_widget (GIMP_DRAW_TOOL (tool), private->widget);

  g_signal_connect (private->widget, "changed",
                    G_CALLBACK (gimp_free_select_tool_polygon_changed),
                    fst);
  g_signal_connect (private->widget, "response",
                    G_CALLBACK (gimp_free_select_tool_polygon_response),
                    fst);

  gimp_draw_tool_start (GIMP_DRAW_TOOL (tool), display);
}

static void
gimp_free_select_tool_halt (GimpFreeSelectTool *fst)
{
  GimpFreeSelectToolPrivate *private = fst->private;

  gimp_draw_tool_set_widget (GIMP_DRAW_TOOL (fst), NULL);
  g_clear_object (&private->widget);
}

static void
gimp_free_select_tool_commit (GimpFreeSelectTool *fst,
                              GimpDisplay        *display)
{
  GimpFreeSelectToolPrivate *private = fst->private;

  if (private->widget)
    {
      const GimpVector2 *points;
      gint               n_points;

      gimp_tool_polygon_get_points (GIMP_TOOL_POLYGON (private->widget),
                                    &points, &n_points);

      if (n_points >= 3)
        {
          GIMP_FREE_SELECT_TOOL_GET_CLASS (fst)->select (fst, display,
                                                         points, n_points);
        }
    }

  gimp_image_flush (gimp_display_get_image (display));
}

static void
gimp_free_select_tool_control (GimpTool       *tool,
                               GimpToolAction  action,
                               GimpDisplay    *display)
{
  GimpFreeSelectTool *fst = GIMP_FREE_SELECT_TOOL (tool);

  switch (action)
    {
    case GIMP_TOOL_ACTION_PAUSE:
    case GIMP_TOOL_ACTION_RESUME:
      break;

    case GIMP_TOOL_ACTION_HALT:
      gimp_free_select_tool_halt (fst);
      break;

    case GIMP_TOOL_ACTION_COMMIT:
      gimp_free_select_tool_commit (fst, display);
      break;
    }

  GIMP_TOOL_CLASS (parent_class)->control (tool, action, display);
}

static void
gimp_free_select_tool_oper_update (GimpTool         *tool,
                                   const GimpCoords *coords,
                                   GdkModifierType   state,
                                   gboolean          proximity,
                                   GimpDisplay      *display)
{
  GimpFreeSelectTool        *fst  = GIMP_FREE_SELECT_TOOL (tool);
  GimpFreeSelectToolPrivate *priv = fst->private;

  if (display != tool->display)
    {
      GIMP_TOOL_CLASS (parent_class)->oper_update (tool, coords, state,
                                                   proximity, display);
      return;
    }

  if (priv->widget)
    {
      gimp_tool_widget_hover (priv->widget, coords, state, proximity);
    }
}

static void
gimp_free_select_tool_cursor_update (GimpTool         *tool,
                                     const GimpCoords *coords,
                                     GdkModifierType   state,
                                     GimpDisplay      *display)
{
  GimpFreeSelectTool        *fst      = GIMP_FREE_SELECT_TOOL (tool);
  GimpFreeSelectToolPrivate *priv     = fst->private;
  GimpCursorModifier         modifier = GIMP_CURSOR_MODIFIER_NONE;

  if (tool->display == NULL)
    {
      GIMP_TOOL_CLASS (parent_class)->cursor_update (tool, coords, state,
                                                     display);
      return;
    }

  if (priv->widget && display == tool->display)
    {
      gimp_tool_widget_get_cursor (priv->widget, coords, state,
                                   NULL, NULL, &modifier);
    }

  gimp_tool_set_cursor (tool, display,
                        gimp_tool_control_get_cursor (tool->control),
                        gimp_tool_control_get_tool_cursor (tool->control),
                        modifier);
}

static void
gimp_free_select_tool_button_press (GimpTool            *tool,
                                    const GimpCoords    *coords,
                                    guint32              time,
                                    GdkModifierType      state,
                                    GimpButtonPressType  press_type,
                                    GimpDisplay         *display)
{
  GimpFreeSelectTool        *fst     = GIMP_FREE_SELECT_TOOL (tool);
  GimpFreeSelectToolPrivate *private = fst->private;

  if (tool->display && tool->display != display)
    gimp_tool_control (tool, GIMP_TOOL_ACTION_COMMIT, tool->display);

  if (! private->widget) /* not tool->display, we have a subclass */
    {
      /* First of all handle delegation to the selection mask edit logic
       * if appropriate.
       */
      if (gimp_selection_tool_start_edit (GIMP_SELECTION_TOOL (fst),
                                          display, coords))
        {
          return;
        }

      gimp_free_select_tool_start (fst, display);

      gimp_tool_widget_hover (private->widget, coords, state, TRUE);
    }

  if (gimp_tool_widget_button_press (private->widget, coords, time, state,
                                     press_type))
    {
      private->grab_widget = private->widget;
    }

  if (press_type == GIMP_BUTTON_PRESS_NORMAL)
    gimp_tool_control_activate (tool->control);
}

static void
gimp_free_select_tool_button_release (GimpTool              *tool,
                                      const GimpCoords      *coords,
                                      guint32                time,
                                      GdkModifierType        state,
                                      GimpButtonReleaseType  release_type,
                                      GimpDisplay           *display)
{
  GimpFreeSelectTool        *fst   = GIMP_FREE_SELECT_TOOL (tool);
  GimpFreeSelectToolPrivate *priv  = fst->private;
  GimpImage                 *image = gimp_display_get_image (display);

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
}

static void
gimp_free_select_tool_motion (GimpTool         *tool,
                              const GimpCoords *coords,
                              guint32           time,
                              GdkModifierType   state,
                              GimpDisplay      *display)
{
  GimpFreeSelectTool        *fst  = GIMP_FREE_SELECT_TOOL (tool);
  GimpFreeSelectToolPrivate *priv = fst->private;

  if (priv->grab_widget)
    {
      gimp_tool_widget_motion (priv->grab_widget, coords, time, state);
    }
}

static gboolean
gimp_free_select_tool_key_press (GimpTool    *tool,
                                 GdkEventKey *kevent,
                                 GimpDisplay *display)
{
  GimpFreeSelectTool        *fst  = GIMP_FREE_SELECT_TOOL (tool);
  GimpFreeSelectToolPrivate *priv = fst->private;

  if (priv->widget && display == tool->display)
    {
      return gimp_tool_widget_key_press (priv->widget, kevent);
    }

  return FALSE;
}

static void
gimp_free_select_tool_modifier_key (GimpTool        *tool,
                                    GdkModifierType  key,
                                    gboolean         press,
                                    GdkModifierType  state,
                                    GimpDisplay     *display)
{
  GimpFreeSelectTool        *fst  = GIMP_FREE_SELECT_TOOL (tool);
  GimpFreeSelectToolPrivate *priv = fst->private;

  if (priv->widget && display == tool->display)
    {
      gimp_tool_widget_hover_modifier (priv->widget, key, press, state);
    }

  GIMP_TOOL_CLASS (parent_class)->modifier_key (tool, key, press, state,
                                                display);
}

static void
gimp_free_select_tool_active_modifier_key (GimpTool        *tool,
                                           GdkModifierType  key,
                                           gboolean         press,
                                           GdkModifierType  state,
                                           GimpDisplay     *display)
{
  GimpFreeSelectTool        *fst  = GIMP_FREE_SELECT_TOOL (tool);
  GimpFreeSelectToolPrivate *priv = fst->private;

  if (priv->widget)
    {
      gimp_tool_widget_motion_modifier (priv->widget, key, press, state);

      GIMP_TOOL_CLASS (parent_class)->active_modifier_key (tool,
                                                           key, press, state,
                                                           display);
    }
}

static void
gimp_free_select_tool_real_select (GimpFreeSelectTool *fst,
                                   GimpDisplay        *display,
                                   const GimpVector2  *points,
                                   gint                n_points)
{
  GimpSelectionOptions      *options = GIMP_SELECTION_TOOL_GET_OPTIONS (fst);
  GimpFreeSelectToolPrivate *priv    = fst->private;
  GimpImage                 *image   = gimp_display_get_image (display);

  gimp_channel_select_polygon (gimp_image_get_mask (image),
                               C_("command", "Free Select"),
                               n_points,
                               points,
                               priv->operation_at_start,
                               options->antialias,
                               options->feather,
                               options->feather_radius,
                               options->feather_radius,
                               TRUE);

  gimp_tool_control (GIMP_TOOL (fst), GIMP_TOOL_ACTION_HALT, display);
}

static void
gimp_free_select_tool_polygon_changed (GimpToolWidget     *polygon,
                                       GimpFreeSelectTool *fst)
{
}

static void
gimp_free_select_tool_polygon_response (GimpToolWidget     *polygon,
                                        gint                response_id,
                                        GimpFreeSelectTool *fst)
{
  GimpTool *tool = GIMP_TOOL (fst);

  switch (response_id)
    {
    case GIMP_TOOL_WIDGET_RESPONSE_CONFIRM:
      /*  don't gimp_tool_control(COMMIT) here because we don't always
       *  want to HALT the tool after committing because we have a
       *  subclass, we do that in the default implementation of
       *  select().
       */
      gimp_free_select_tool_commit (fst, tool->display);
      break;

    case GIMP_TOOL_WIDGET_RESPONSE_CANCEL:
      gimp_tool_control (tool, GIMP_TOOL_ACTION_HALT, tool->display);
      break;
    }
}
