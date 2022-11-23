/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
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

#include "libligmamath/ligmamath.h"
#include "libligmawidgets/ligmawidgets.h"

#include "tools-types.h"

#include "core/ligmachannel-select.h"
#include "core/ligmachannel.h"
#include "core/ligmaimage.h"
#include "core/ligmalayer-floating-selection.h"
#include "core/ligmapickable.h"

#include "widgets/ligmahelp-ids.h"
#include "widgets/ligmawidgets-utils.h"

#include "display/ligmadisplay.h"
#include "display/ligmadisplayshell.h"
#include "display/ligmatoolrectangle.h"

#include "ligmaeditselectiontool.h"
#include "ligmarectangleoptions.h"
#include "ligmarectangleselecttool.h"
#include "ligmarectangleselectoptions.h"
#include "ligmatoolcontrol.h"

#include "ligma-intl.h"


struct _LigmaRectangleSelectToolPrivate
{
  LigmaChannelOps  operation;    /* remember for use when modifying   */
  gboolean        use_saved_op; /* use operation or get from options */

  gdouble         press_x;
  gdouble         press_y;

  LigmaToolWidget *widget;
  LigmaToolWidget *grab_widget;
  GList          *bindings;
};


static void     ligma_rectangle_select_tool_control        (LigmaTool              *tool,
                                                           LigmaToolAction         action,
                                                           LigmaDisplay           *display);
static void     ligma_rectangle_select_tool_button_press   (LigmaTool              *tool,
                                                           const LigmaCoords      *coords,
                                                           guint32                time,
                                                           GdkModifierType        state,
                                                           LigmaButtonPressType    press_type,
                                                           LigmaDisplay           *display);
static void     ligma_rectangle_select_tool_button_release (LigmaTool              *tool,
                                                           const LigmaCoords      *coords,
                                                           guint32                time,
                                                           GdkModifierType        state,
                                                           LigmaButtonReleaseType  release_type,
                                                           LigmaDisplay           *display);
static void     ligma_rectangle_select_tool_motion         (LigmaTool              *tool,
                                                           const LigmaCoords      *coords,
                                                           guint32                time,
                                                           GdkModifierType        state,
                                                           LigmaDisplay           *display);
static gboolean ligma_rectangle_select_tool_key_press      (LigmaTool              *tool,
                                                           GdkEventKey           *kevent,
                                                           LigmaDisplay           *display);
static void     ligma_rectangle_select_tool_oper_update    (LigmaTool              *tool,
                                                           const LigmaCoords      *coords,
                                                           GdkModifierType        state,
                                                           gboolean               proximity,
                                                           LigmaDisplay           *display);
static void     ligma_rectangle_select_tool_cursor_update  (LigmaTool              *tool,
                                                           const LigmaCoords      *coords,
                                                           GdkModifierType        state,
                                                           LigmaDisplay           *display);
static void     ligma_rectangle_select_tool_options_notify (LigmaTool              *tool,
                                                           LigmaToolOptions       *options,
                                                           const GParamSpec      *pspec);

static gboolean ligma_rectangle_select_tool_select         (LigmaRectangleSelectTool *rect_tool,
                                                           gint                   x,
                                                           gint                   y,
                                                           gint                   w,
                                                           gint                   h);
static void     ligma_rectangle_select_tool_real_select    (LigmaRectangleSelectTool *rect_tool,
                                                           LigmaChannelOps         operation,
                                                           gint                   x,
                                                           gint                   y,
                                                           gint                   w,
                                                           gint                   h);

static void     ligma_rectangle_select_tool_rectangle_response
                                                          (LigmaToolWidget          *widget,
                                                           gint                     response_id,
                                                           LigmaRectangleSelectTool *rect_tool);
static void     ligma_rectangle_select_tool_rectangle_change_complete
                                                          (LigmaToolWidget          *widget,
                                                           LigmaRectangleSelectTool *rect_tool);

static void     ligma_rectangle_select_tool_start          (LigmaRectangleSelectTool *rect_tool,
                                                           LigmaDisplay             *display);
static void     ligma_rectangle_select_tool_commit         (LigmaRectangleSelectTool *rect_tool);
static void     ligma_rectangle_select_tool_halt           (LigmaRectangleSelectTool *rect_tool);

static LigmaChannelOps
                ligma_rectangle_select_tool_get_operation  (LigmaRectangleSelectTool *rect_tool);
static void     ligma_rectangle_select_tool_update_option_defaults
                                                          (LigmaRectangleSelectTool *rect_tool,
                                                           gboolean                 ignore_pending);
static void     ligma_rectangle_select_tool_update         (LigmaRectangleSelectTool *rect_tool);
static void     ligma_rectangle_select_tool_auto_shrink    (LigmaRectangleSelectTool *rect_tool);


G_DEFINE_TYPE_WITH_PRIVATE (LigmaRectangleSelectTool, ligma_rectangle_select_tool,
                            LIGMA_TYPE_SELECTION_TOOL)

#define parent_class ligma_rectangle_select_tool_parent_class


void
ligma_rectangle_select_tool_register (LigmaToolRegisterCallback  callback,
                                     gpointer                  data)
{
  (* callback) (LIGMA_TYPE_RECTANGLE_SELECT_TOOL,
                LIGMA_TYPE_RECTANGLE_SELECT_OPTIONS,
                ligma_rectangle_select_options_gui,
                0,
                "ligma-rect-select-tool",
                _("Rectangle Select"),
                _("Rectangle Select Tool: Select a rectangular region"),
                N_("_Rectangle Select"), "R",
                NULL, LIGMA_HELP_TOOL_RECT_SELECT,
                LIGMA_ICON_TOOL_RECT_SELECT,
                data);
}

static void
ligma_rectangle_select_tool_class_init (LigmaRectangleSelectToolClass *klass)
{
  LigmaToolClass *tool_class = LIGMA_TOOL_CLASS (klass);

  tool_class->control        = ligma_rectangle_select_tool_control;
  tool_class->button_press   = ligma_rectangle_select_tool_button_press;
  tool_class->button_release = ligma_rectangle_select_tool_button_release;
  tool_class->motion         = ligma_rectangle_select_tool_motion;
  tool_class->key_press      = ligma_rectangle_select_tool_key_press;
  tool_class->oper_update    = ligma_rectangle_select_tool_oper_update;
  tool_class->cursor_update  = ligma_rectangle_select_tool_cursor_update;
  tool_class->options_notify = ligma_rectangle_select_tool_options_notify;

  klass->select              = ligma_rectangle_select_tool_real_select;
}

static void
ligma_rectangle_select_tool_init (LigmaRectangleSelectTool *rect_tool)
{
  LigmaTool *tool = LIGMA_TOOL (rect_tool);

  rect_tool->private =
    ligma_rectangle_select_tool_get_instance_private (rect_tool);

  ligma_tool_control_set_wants_click      (tool->control, TRUE);
  ligma_tool_control_set_active_modifiers (tool->control,
                                          LIGMA_TOOL_ACTIVE_MODIFIERS_SEPARATE);
  ligma_tool_control_set_precision        (tool->control,
                                          LIGMA_CURSOR_PRECISION_PIXEL_BORDER);
  ligma_tool_control_set_tool_cursor      (tool->control,
                                          LIGMA_TOOL_CURSOR_RECT_SELECT);
  ligma_tool_control_set_preserve         (tool->control, FALSE);
  ligma_tool_control_set_dirty_mask       (tool->control,
                                          LIGMA_DIRTY_IMAGE_SIZE |
                                          LIGMA_DIRTY_SELECTION);
  ligma_tool_control_set_dirty_action     (tool->control,
                                          LIGMA_TOOL_ACTION_COMMIT);
}

static void
ligma_rectangle_select_tool_control (LigmaTool       *tool,
                                    LigmaToolAction  action,
                                    LigmaDisplay    *display)
{
  LigmaRectangleSelectTool *rect_tool = LIGMA_RECTANGLE_SELECT_TOOL (tool);

  switch (action)
    {
    case LIGMA_TOOL_ACTION_PAUSE:
    case LIGMA_TOOL_ACTION_RESUME:
      break;

    case LIGMA_TOOL_ACTION_HALT:
      ligma_rectangle_select_tool_halt (rect_tool);
      break;

    case LIGMA_TOOL_ACTION_COMMIT:
      ligma_rectangle_select_tool_commit (rect_tool);
      break;
    }

  LIGMA_TOOL_CLASS (parent_class)->control (tool, action, display);
}

static void
ligma_rectangle_select_tool_button_press (LigmaTool            *tool,
                                         const LigmaCoords    *coords,
                                         guint32              time,
                                         GdkModifierType      state,
                                         LigmaButtonPressType  press_type,
                                         LigmaDisplay         *display)
{
  LigmaRectangleSelectTool        *rect_tool = LIGMA_RECTANGLE_SELECT_TOOL (tool);
  LigmaRectangleSelectToolPrivate *private   = rect_tool->private;
  LigmaRectangleFunction           function;

  if (tool->display && display != tool->display)
    ligma_tool_control (tool, LIGMA_TOOL_ACTION_COMMIT, tool->display);

  if (ligma_selection_tool_start_edit (LIGMA_SELECTION_TOOL (tool),
                                      display, coords))
    {
      /* In some cases we want to finish the rectangle select tool
       * and hand over responsibility to the selection tool
       */

      gboolean zero_rect = FALSE;

      if (private->widget)
        {
          gdouble x1, y1, x2, y2;

          ligma_tool_rectangle_get_public_rect (LIGMA_TOOL_RECTANGLE (private->widget),
                                               &x1, &y1, &x2, &y2);
          if (x1 == x2 && y1 == y2)
            zero_rect = TRUE;
        }

      /* Don't commit a zero-size rectangle, it would look like a
       * click to commit() and that could anchor the floating
       * selection or do other evil things. Instead, simply cancel a
       * zero-size rectangle. See bug #796073.
       */
      if (zero_rect)
        ligma_tool_control (tool, LIGMA_TOOL_ACTION_HALT, display);
      else
        ligma_tool_control (tool, LIGMA_TOOL_ACTION_COMMIT, display);

      ligma_rectangle_select_tool_update_option_defaults (rect_tool, TRUE);
      return;
    }

  if (! tool->display)
    {
      ligma_rectangle_select_tool_start (rect_tool, display);

      ligma_tool_widget_hover (private->widget, coords, state, TRUE);

      /* HACK: force CREATING on a newly created rectangle; otherwise,
       * the above binding of properties would cause the rectangle to
       * start with the size from tool options.
       */
      ligma_tool_rectangle_set_function (LIGMA_TOOL_RECTANGLE (private->widget),
                                        LIGMA_TOOL_RECTANGLE_CREATING);
    }

  /* if the shift or ctrl keys are down, we don't want to adjust, we
   * want to create a new rectangle, regardless of pointer loc
   */
  if (state & (ligma_get_extend_selection_mask () |
               ligma_get_modify_selection_mask ()))
    {
      ligma_tool_rectangle_set_function (LIGMA_TOOL_RECTANGLE (private->widget),
                                        LIGMA_TOOL_RECTANGLE_CREATING);
    }

  if (ligma_tool_widget_button_press (private->widget, coords, time, state,
                                     press_type))
    {
      private->grab_widget = private->widget;
    }

  private->press_x = coords->x;
  private->press_y = coords->y;

  /* if we have an existing rectangle in the current display, then
   * we have already "executed", and need to undo at this point,
   * unless the user has done something in the meantime
   */
  function =
    ligma_tool_rectangle_get_function (LIGMA_TOOL_RECTANGLE (private->widget));

  if (function == LIGMA_TOOL_RECTANGLE_CREATING)
    private->use_saved_op = FALSE;

  ligma_selection_tool_start_change (
    LIGMA_SELECTION_TOOL (tool),
    function == LIGMA_TOOL_RECTANGLE_CREATING,
    ligma_rectangle_select_tool_get_operation (rect_tool));

  ligma_tool_control_activate (tool->control);
}

static void
ligma_rectangle_select_tool_button_release (LigmaTool              *tool,
                                           const LigmaCoords      *coords,
                                           guint32                time,
                                           GdkModifierType        state,
                                           LigmaButtonReleaseType  release_type,
                                           LigmaDisplay           *display)
{
  LigmaRectangleSelectTool        *rect_tool = LIGMA_RECTANGLE_SELECT_TOOL (tool);
  LigmaRectangleSelectToolPrivate *priv      = rect_tool->private;

  ligma_tool_control_halt (tool->control);

  ligma_selection_tool_end_change (LIGMA_SELECTION_TOOL (tool),
                                  /* if the user has not moved the mouse,
                                   * cancel the change
                                   */
                                  release_type == LIGMA_BUTTON_RELEASE_CLICK ||
                                  release_type == LIGMA_BUTTON_RELEASE_CANCEL);

  ligma_tool_pop_status (tool, display);

  if (priv->grab_widget)
    {
      ligma_tool_widget_button_release (priv->grab_widget,
                                       coords, time, state, release_type);
      priv->grab_widget = NULL;
    }
}

static void
ligma_rectangle_select_tool_motion (LigmaTool         *tool,
                                   const LigmaCoords *coords,
                                   guint32           time,
                                   GdkModifierType   state,
                                   LigmaDisplay      *display)
{
  LigmaRectangleSelectTool        *rect_tool = LIGMA_RECTANGLE_SELECT_TOOL (tool);
  LigmaRectangleSelectToolPrivate *priv      = rect_tool->private;

  if (priv->grab_widget)
    {
      ligma_tool_widget_motion (priv->grab_widget, coords, time, state);
    }
}

static gboolean
ligma_rectangle_select_tool_key_press (LigmaTool    *tool,
                                      GdkEventKey *kevent,
                                      LigmaDisplay *display)
{
  LigmaRectangleSelectTool        *rect_tool = LIGMA_RECTANGLE_SELECT_TOOL (tool);
  LigmaRectangleSelectToolPrivate *priv      = rect_tool->private;

  if (priv->widget && display == tool->display)
    {
      if (ligma_tool_widget_key_press (priv->widget, kevent))
        return TRUE;
    }

  return ligma_edit_selection_tool_key_press (tool, kevent, display);
}

static void
ligma_rectangle_select_tool_oper_update (LigmaTool         *tool,
                                        const LigmaCoords *coords,
                                        GdkModifierType   state,
                                        gboolean          proximity,
                                        LigmaDisplay      *display)
{
  LigmaRectangleSelectTool        *rect_tool = LIGMA_RECTANGLE_SELECT_TOOL (tool);
  LigmaRectangleSelectToolPrivate *priv      = rect_tool->private;

  if (priv->widget && display == tool->display)
    {
      ligma_tool_widget_hover (priv->widget, coords, state, proximity);
    }

  LIGMA_TOOL_CLASS (parent_class)->oper_update (tool, coords, state, proximity,
                                               display);
}

static void
ligma_rectangle_select_tool_cursor_update (LigmaTool         *tool,
                                          const LigmaCoords *coords,
                                          GdkModifierType   state,
                                          LigmaDisplay      *display)
{
  LigmaRectangleSelectTool        *rect_tool = LIGMA_RECTANGLE_SELECT_TOOL (tool);
  LigmaRectangleSelectToolPrivate *private   = rect_tool->private;
  LigmaCursorType                  cursor    = LIGMA_CURSOR_CROSSHAIR_SMALL;
  LigmaCursorModifier              modifier  = LIGMA_CURSOR_MODIFIER_NONE;

  if (private->widget && display == tool->display)
    {
      ligma_tool_widget_get_cursor (private->widget, coords, state,
                                   &cursor, NULL, &modifier);
    }

  ligma_tool_control_set_cursor          (tool->control, cursor);
  ligma_tool_control_set_cursor_modifier (tool->control, modifier);

  /* override the previous if shift or ctrl are down */
  if (state & (ligma_get_extend_selection_mask () |
               ligma_get_modify_selection_mask ()))
    {
      ligma_tool_control_set_cursor (tool->control,
                                    LIGMA_CURSOR_CROSSHAIR_SMALL);
    }

  LIGMA_TOOL_CLASS (parent_class)->cursor_update (tool, coords, state, display);
}

static void
ligma_rectangle_select_tool_options_notify (LigmaTool         *tool,
                                           LigmaToolOptions  *options,
                                           const GParamSpec *pspec)
{
  if (! strcmp (pspec->name, "antialias")      ||
      ! strcmp (pspec->name, "feather")        ||
      ! strcmp (pspec->name, "feather-radius") ||
      ! strcmp (pspec->name, "round-corners")  ||
      ! strcmp (pspec->name, "corner-radius"))
    {
      ligma_rectangle_select_tool_update (LIGMA_RECTANGLE_SELECT_TOOL (tool));
    }

  LIGMA_TOOL_CLASS (parent_class)->options_notify (tool, options, pspec);
}

static gboolean
ligma_rectangle_select_tool_select (LigmaRectangleSelectTool *rect_tool,
                                   gint                     x,
                                   gint                     y,
                                   gint                     w,
                                   gint                     h)
{
  LigmaTool       *tool  = LIGMA_TOOL (rect_tool);
  LigmaImage      *image = ligma_display_get_image (tool->display);
  gboolean        rectangle_exists;
  LigmaChannelOps  operation;

  ligma_tool_pop_status (tool, tool->display);

  rectangle_exists = (x <= ligma_image_get_width  (image) &&
                      y <= ligma_image_get_height (image) &&
                      x + w >= 0                         &&
                      y + h >= 0                         &&
                      w > 0                              &&
                      h > 0);

  operation = ligma_rectangle_select_tool_get_operation (rect_tool);

  /* if rectangle exists, turn it into a selection */
  if (rectangle_exists)
    {
      ligma_selection_tool_start_change (LIGMA_SELECTION_TOOL (rect_tool),
                                        FALSE,
                                        operation);

      LIGMA_RECTANGLE_SELECT_TOOL_GET_CLASS (rect_tool)->select (rect_tool,
                                                                operation,
                                                                x, y, w, h);

      ligma_selection_tool_end_change (LIGMA_SELECTION_TOOL (rect_tool),
                                      FALSE);
    }

  return rectangle_exists;
}

static void
ligma_rectangle_select_tool_real_select (LigmaRectangleSelectTool *rect_tool,
                                        LigmaChannelOps           operation,
                                        gint                     x,
                                        gint                     y,
                                        gint                     w,
                                        gint                     h)
{
  LigmaTool                   *tool    = LIGMA_TOOL (rect_tool);
  LigmaSelectionOptions       *options = LIGMA_SELECTION_TOOL_GET_OPTIONS (tool);
  LigmaRectangleSelectOptions *rect_options;
  LigmaChannel                *channel;

  rect_options = LIGMA_RECTANGLE_SELECT_TOOL_GET_OPTIONS (tool);

  channel = ligma_image_get_mask (ligma_display_get_image (tool->display));

  if (rect_options->round_corners)
    {
      /* To prevent elliptification of the rectangle,
       * we must cap the corner radius.
       */
      gdouble max    = MIN (w / 2.0, h / 2.0);
      gdouble radius = MIN (rect_options->corner_radius, max);

      ligma_channel_select_round_rect (channel,
                                      x, y, w, h,
                                      radius, radius,
                                      operation,
                                      options->antialias,
                                      options->feather,
                                      options->feather_radius,
                                      options->feather_radius,
                                      TRUE);
    }
  else
    {
      ligma_channel_select_rectangle (channel,
                                     x, y, w, h,
                                     operation,
                                     options->feather,
                                     options->feather_radius,
                                     options->feather_radius,
                                     TRUE);
    }
}

static void
ligma_rectangle_select_tool_rectangle_response (LigmaToolWidget          *widget,
                                               gint                     response_id,
                                               LigmaRectangleSelectTool *rect_tool)
{
  LigmaTool *tool = LIGMA_TOOL (rect_tool);

  switch (response_id)
    {
    case LIGMA_TOOL_WIDGET_RESPONSE_CONFIRM:
      {
        gdouble x1, y1, x2, y2;

        ligma_tool_rectangle_get_public_rect (LIGMA_TOOL_RECTANGLE (widget),
                                             &x1, &y1, &x2, &y2);
        if (x1 == x2 && y1 == y2)
          {
            /*  if there are no extents, we got here because of a
             *  click, call commit() directly because we might want to
             *  reconfigure the rectangle and continue, instead of
             *  HALTing it like calling COMMIT would do
             */
            ligma_rectangle_select_tool_commit (rect_tool);

            ligma_tool_rectangle_get_public_rect (LIGMA_TOOL_RECTANGLE (widget),
                                                 &x1, &y1, &x2, &y2);
            if (x1 == x2 && y1 == y2)
              {
                /*  if there still is no rectangle after the
                 *  tool_commit(), the click was outside the selection
                 *  and we HALT to get rid of a zero-size tool widget.
                 */
                ligma_tool_control (tool, LIGMA_TOOL_ACTION_HALT, tool->display);
              }
         }
        else
          {
            ligma_tool_control (tool, LIGMA_TOOL_ACTION_COMMIT, tool->display);
          }
      }
      break;

    case LIGMA_TOOL_WIDGET_RESPONSE_CANCEL:
      ligma_tool_control (tool, LIGMA_TOOL_ACTION_HALT, tool->display);
      break;
    }
}

static void
ligma_rectangle_select_tool_rectangle_change_complete (LigmaToolWidget          *widget,
                                                      LigmaRectangleSelectTool *rect_tool)
{
  ligma_rectangle_select_tool_update (rect_tool);
}

static void
ligma_rectangle_select_tool_start (LigmaRectangleSelectTool *rect_tool,
                                  LigmaDisplay             *display)
{
  static const gchar *properties[] =
  {
    "highlight",
    "highlight-opacity",
    "guide",
    "round-corners",
    "corner-radius",
    "x",
    "y",
    "width",
    "height",
    "fixed-rule-active",
    "fixed-rule",
    "desired-fixed-width",
    "desired-fixed-height",
    "desired-fixed-size-width",
    "desired-fixed-size-height",
    "aspect-numerator",
    "aspect-denominator",
    "fixed-center"
  };

  LigmaTool                       *tool    = LIGMA_TOOL (rect_tool);
  LigmaRectangleSelectToolPrivate *private = rect_tool->private;
  LigmaDisplayShell               *shell   = ligma_display_get_shell (display);
  LigmaRectangleSelectOptions     *options;
  LigmaToolWidget                 *widget;
  gboolean                        draw_ellipse;
  gint                            i;

  options = LIGMA_RECTANGLE_SELECT_TOOL_GET_OPTIONS (rect_tool);

  tool->display = display;

  private->widget = widget = ligma_tool_rectangle_new (shell);

  draw_ellipse = LIGMA_RECTANGLE_SELECT_TOOL_GET_CLASS (rect_tool)->draw_ellipse;

  g_object_set (widget,
                "draw-ellipse", draw_ellipse,
                "status-title", draw_ellipse ? _("Ellipse: ") : _("Rectangle: "),
                NULL);

  ligma_draw_tool_set_widget (LIGMA_DRAW_TOOL (tool), widget);

  for (i = 0; i < G_N_ELEMENTS (properties); i++)
    {
      GBinding *binding =
        g_object_bind_property (G_OBJECT (options), properties[i],
                                G_OBJECT (widget),  properties[i],
                                G_BINDING_SYNC_CREATE |
                                G_BINDING_BIDIRECTIONAL);

      private->bindings = g_list_prepend (private->bindings, binding);
    }

  ligma_rectangle_options_connect (LIGMA_RECTANGLE_OPTIONS (options),
                                  ligma_display_get_image (shell->display),
                                  G_CALLBACK (ligma_rectangle_select_tool_auto_shrink),
                                  rect_tool);

  g_signal_connect (widget, "response",
                    G_CALLBACK (ligma_rectangle_select_tool_rectangle_response),
                    rect_tool);
  g_signal_connect (widget, "change-complete",
                    G_CALLBACK (ligma_rectangle_select_tool_rectangle_change_complete),
                    rect_tool);

  ligma_draw_tool_start (LIGMA_DRAW_TOOL (tool), display);
}

/* This function is called if the user clicks and releases the left
 * button without moving it.  There are the things we might want
 * to do here:
 * 1) If there is an existing rectangle and we are inside it, we
 *    convert it into a selection.
 * 2) If there is an existing rectangle and we are outside it, we
 *    clear it.
 * 3) If there is no rectangle and there is a floating selection,
 *    we anchor it.
 * 4) If there is no rectangle and we are inside the selection, we
 *    create a rectangle from the selection bounds.
 * 5) If there is no rectangle and we are outside the selection,
 *    we clear the selection.
 */
static void
ligma_rectangle_select_tool_commit (LigmaRectangleSelectTool *rect_tool)
{
  LigmaTool                       *tool = LIGMA_TOOL (rect_tool);
  LigmaRectangleSelectToolPrivate *priv = rect_tool->private;

  if (priv->widget)
    {
      gdouble x1, y1, x2, y2;
      gint    w, h;

      ligma_tool_rectangle_get_public_rect (LIGMA_TOOL_RECTANGLE (priv->widget),
                                           &x1, &y1, &x2, &y2);
      w = x2 - x1;
      h = y2 - y1;

      if (w == 0 && h == 0)
        {
          LigmaImage   *image     = ligma_display_get_image (tool->display);
          LigmaChannel *selection = ligma_image_get_mask (image);
          gint         press_x;
          gint         press_y;

          if (ligma_image_get_floating_selection (image))
            {
              floating_sel_anchor (ligma_image_get_floating_selection (image));
              ligma_image_flush (image);

              return;
            }

          press_x = ROUND (priv->press_x);
          press_y = ROUND (priv->press_y);

          /*  if the click was inside the marching ants  */
          if (ligma_pickable_get_opacity_at (LIGMA_PICKABLE (selection),
                                            press_x, press_y) > 0.5)
            {
              gint x, y, w, h;

              if (ligma_item_bounds (LIGMA_ITEM (selection), &x, &y, &w, &h))
                {
                  g_object_set (priv->widget,
                                "x1", (gdouble) x,
                                "y1", (gdouble) y,
                                "x2", (gdouble) (x + w),
                                "y2", (gdouble) (y + h),
                                NULL);
                }

              ligma_rectangle_select_tool_update (rect_tool);
            }
          else
            {
              LigmaChannelOps  operation;

              /* prevent this change from halting the tool */
              ligma_tool_control_push_preserve (tool->control, TRUE);

              /* We can conceptually think of a click outside of the
               * selection as adding a 0px selection. Behave intuitively
               * for the current selection mode
               */
              operation = ligma_rectangle_select_tool_get_operation (rect_tool);

              switch (operation)
                {
                case LIGMA_CHANNEL_OP_REPLACE:
                case LIGMA_CHANNEL_OP_INTERSECT:
                  ligma_channel_clear (selection, NULL, TRUE);
                  ligma_image_flush (image);
                  break;

                case LIGMA_CHANNEL_OP_ADD:
                case LIGMA_CHANNEL_OP_SUBTRACT:
                default:
                  /* Do nothing */
                  break;
                }

              ligma_tool_control_pop_preserve (tool->control);
            }
        }

      ligma_rectangle_select_tool_update_option_defaults (rect_tool, FALSE);
    }
}

static void
ligma_rectangle_select_tool_halt (LigmaRectangleSelectTool *rect_tool)
{
  LigmaTool                       *tool = LIGMA_TOOL (rect_tool);
  LigmaRectangleSelectToolPrivate *priv = rect_tool->private;
  LigmaRectangleSelectOptions     *options;

  options = LIGMA_RECTANGLE_SELECT_TOOL_GET_OPTIONS (rect_tool);

  if (tool->display)
    {
      LigmaDisplayShell *shell = ligma_display_get_shell (tool->display);

      ligma_display_shell_set_highlight (shell, NULL, 0.0);

      ligma_rectangle_options_disconnect (LIGMA_RECTANGLE_OPTIONS (options),
                                         G_CALLBACK (ligma_rectangle_select_tool_auto_shrink),
                                         rect_tool);
    }

  if (ligma_draw_tool_is_active (LIGMA_DRAW_TOOL (tool)))
    ligma_draw_tool_stop (LIGMA_DRAW_TOOL (tool));

  /*  disconnect bindings manually so they are really gone *now*, we
   *  might be in the middle of a signal emission that keeps the
   *  widget and its bindings alive.
   */
  g_list_free_full (priv->bindings, (GDestroyNotify) g_object_unref);
  priv->bindings = NULL;

  ligma_draw_tool_set_widget (LIGMA_DRAW_TOOL (tool), NULL);
  g_clear_object (&priv->widget);

  tool->display = NULL;

  ligma_rectangle_select_tool_update_option_defaults (rect_tool, TRUE);
}

static LigmaChannelOps
ligma_rectangle_select_tool_get_operation (LigmaRectangleSelectTool *rect_tool)
{
  LigmaRectangleSelectToolPrivate *priv = rect_tool->private;
  LigmaSelectionOptions           *options;

  options = LIGMA_SELECTION_TOOL_GET_OPTIONS (rect_tool);

  if (priv->use_saved_op)
    return priv->operation;
  else
    return options->operation;
}

/**
 * ligma_rectangle_select_tool_update_option_defaults:
 * @crop_tool:
 * @ignore_pending: %TRUE to ignore any pending crop rectangle.
 *
 * Sets the default Fixed: Aspect ratio and Fixed: Size option
 * properties.
 */
static void
ligma_rectangle_select_tool_update_option_defaults (LigmaRectangleSelectTool *rect_tool,
                                                   gboolean                 ignore_pending)
{
  LigmaRectangleSelectToolPrivate *priv = rect_tool->private;
  LigmaTool                       *tool = LIGMA_TOOL (rect_tool);
  LigmaRectangleOptions           *rect_options;

  rect_options = LIGMA_RECTANGLE_OPTIONS (ligma_tool_get_options (tool));

  if (priv->widget && ! ignore_pending)
    {
      /* There is a pending rectangle and we should not ignore it, so
       * set default Fixed: Size to the same as the current pending
       * rectangle width/height.
       */

      ligma_tool_rectangle_pending_size_set (LIGMA_TOOL_RECTANGLE (priv->widget),
                                            G_OBJECT (rect_options),
                                            "default-aspect-numerator",
                                            "default-aspect-denominator");

      g_object_set (G_OBJECT (rect_options),
                    "use-string-current", TRUE,
                    NULL);
    }
  else
    {
      g_object_set (G_OBJECT (rect_options),
                    "default-aspect-numerator",   1.0,
                    "default-aspect-denominator", 1.0,
                    NULL);

      g_object_set (G_OBJECT (rect_options),
                    "use-string-current", FALSE,
                    NULL);
    }
}

static void
ligma_rectangle_select_tool_update (LigmaRectangleSelectTool *rect_tool)
{
  LigmaTool                       *tool = LIGMA_TOOL (rect_tool);
  LigmaRectangleSelectToolPrivate *priv = rect_tool->private;

  /* prevent change in selection from halting the tool */
  ligma_tool_control_push_preserve (tool->control, TRUE);

  if (tool->display && ! ligma_tool_control_is_active (tool->control))
    {
      gdouble x1, y1, x2, y2;

      ligma_tool_rectangle_get_public_rect (LIGMA_TOOL_RECTANGLE (priv->widget),
                                           &x1, &y1, &x2, &y2);

      ligma_rectangle_select_tool_select (rect_tool,
                                         x1, y1, x2 - x1, y2 - y1);

      if (! priv->use_saved_op)
        {
          LigmaSelectionOptions *options;

          options = LIGMA_SELECTION_TOOL_GET_OPTIONS (tool);

          /* remember the operation now in case we modify the rectangle */
          priv->operation    = options->operation;
          priv->use_saved_op = TRUE;
        }
    }

  ligma_tool_control_pop_preserve (tool->control);

  ligma_rectangle_select_tool_update_option_defaults (rect_tool, FALSE);
}

static void
ligma_rectangle_select_tool_auto_shrink (LigmaRectangleSelectTool *rect_tool)
{
  LigmaRectangleSelectToolPrivate *private = rect_tool->private;
  gboolean                        shrink_merged;

  g_object_get (ligma_tool_get_options (LIGMA_TOOL (rect_tool)),
                "shrink-merged", &shrink_merged,
                NULL);

  ligma_tool_rectangle_auto_shrink (LIGMA_TOOL_RECTANGLE (private->widget),
                                   shrink_merged);
}
