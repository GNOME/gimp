/* GIMP - The GNU Image Manipulation Program
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
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <gegl.h>
#include <gtk/gtk.h>

#include "libgimpmath/gimpmath.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "tools-types.h"

#include "core/gimpchannel-select.h"
#include "core/gimpchannel.h"
#include "core/gimpimage.h"
#include "core/gimpimage-undo.h"
#include "core/gimplayer-floating-selection.h"
#include "core/gimppickable.h"
#include "core/gimpundostack.h"

#include "widgets/gimphelp-ids.h"
#include "widgets/gimpwidgets-utils.h"

#include "display/gimpdisplay.h"
#include "display/gimpdisplayshell.h"
#include "display/gimpdisplayshell-appearance.h"
#include "display/gimptoolrectangle.h"

#include "gimpeditselectiontool.h"
#include "gimprectangleoptions.h"
#include "gimprectangleselecttool.h"
#include "gimprectangleselectoptions.h"
#include "gimptoolcontrol.h"

#include "gimp-intl.h"


struct _GimpRectangleSelectToolPrivate
{
  GimpChannelOps     operation;            /* remember for use when modifying   */
  gboolean           use_saved_op;         /* use operation or get from options */
  gboolean           saved_show_selection; /* used to remember existing value   */
  GimpUndo          *undo;
  GimpUndo          *redo;

  gdouble            press_x;
  gdouble            press_y;

  GimpToolWidget    *widget;
  GimpToolWidget    *grab_widget;
  GList             *bindings;
};


static void     gimp_rectangle_select_tool_control        (GimpTool              *tool,
                                                           GimpToolAction         action,
                                                           GimpDisplay           *display);
static void     gimp_rectangle_select_tool_button_press   (GimpTool              *tool,
                                                           const GimpCoords      *coords,
                                                           guint32                time,
                                                           GdkModifierType        state,
                                                           GimpButtonPressType    press_type,
                                                           GimpDisplay           *display);
static void     gimp_rectangle_select_tool_button_release (GimpTool              *tool,
                                                           const GimpCoords      *coords,
                                                           guint32                time,
                                                           GdkModifierType        state,
                                                           GimpButtonReleaseType  release_type,
                                                           GimpDisplay           *display);
static void     gimp_rectangle_select_tool_motion         (GimpTool              *tool,
                                                           const GimpCoords      *coords,
                                                           guint32                time,
                                                           GdkModifierType        state,
                                                           GimpDisplay           *display);
static gboolean gimp_rectangle_select_tool_key_press      (GimpTool              *tool,
                                                           GdkEventKey           *kevent,
                                                           GimpDisplay           *display);
static void     gimp_rectangle_select_tool_oper_update    (GimpTool              *tool,
                                                           const GimpCoords      *coords,
                                                           GdkModifierType        state,
                                                           gboolean               proximity,
                                                           GimpDisplay           *display);
static void     gimp_rectangle_select_tool_cursor_update  (GimpTool              *tool,
                                                           const GimpCoords      *coords,
                                                           GdkModifierType        state,
                                                           GimpDisplay           *display);

static gboolean gimp_rectangle_select_tool_select         (GimpRectangleSelectTool *rect_tool,
                                                           gint                   x,
                                                           gint                   y,
                                                           gint                   w,
                                                           gint                   h);
static void     gimp_rectangle_select_tool_real_select    (GimpRectangleSelectTool *rect_tool,
                                                           GimpChannelOps         operation,
                                                           gint                   x,
                                                           gint                   y,
                                                           gint                   w,
                                                           gint                   h);

static void     gimp_rectangle_select_tool_rectangle_response
                                                          (GimpToolWidget          *widget,
                                                           gint                     response_id,
                                                           GimpRectangleSelectTool *rect_tool);
static void     gimp_rectangle_select_tool_rectangle_change_complete
                                                          (GimpToolWidget          *widget,
                                                           GimpRectangleSelectTool *rect_tool);

static void     gimp_rectangle_select_tool_start          (GimpRectangleSelectTool *rect_tool,
                                                           GimpDisplay             *display);
static void     gimp_rectangle_select_tool_commit         (GimpRectangleSelectTool *rect_tool);
static void     gimp_rectangle_select_tool_halt           (GimpRectangleSelectTool *rect_tool);

static GimpChannelOps
                gimp_rectangle_select_tool_get_operation  (GimpRectangleSelectTool *rect_tool);
static void     gimp_rectangle_select_tool_update_option_defaults
                                                          (GimpRectangleSelectTool *rect_tool,
                                                           gboolean                 ignore_pending);
static void     gimp_rectangle_select_tool_auto_shrink    (GimpRectangleSelectTool *rect_tool);


G_DEFINE_TYPE (GimpRectangleSelectTool, gimp_rectangle_select_tool,
               GIMP_TYPE_SELECTION_TOOL)

#define parent_class gimp_rectangle_select_tool_parent_class


void
gimp_rectangle_select_tool_register (GimpToolRegisterCallback  callback,
                                     gpointer                  data)
{
  (* callback) (GIMP_TYPE_RECTANGLE_SELECT_TOOL,
                GIMP_TYPE_RECTANGLE_SELECT_OPTIONS,
                gimp_rectangle_select_options_gui,
                0,
                "gimp-rect-select-tool",
                _("Rectangle Select"),
                _("Rectangle Select Tool: Select a rectangular region"),
                N_("_Rectangle Select"), "R",
                NULL, GIMP_HELP_TOOL_RECT_SELECT,
                GIMP_ICON_TOOL_RECT_SELECT,
                data);
}

static void
gimp_rectangle_select_tool_class_init (GimpRectangleSelectToolClass *klass)
{
  GimpToolClass *tool_class = GIMP_TOOL_CLASS (klass);

  tool_class->control        = gimp_rectangle_select_tool_control;
  tool_class->button_press   = gimp_rectangle_select_tool_button_press;
  tool_class->button_release = gimp_rectangle_select_tool_button_release;
  tool_class->motion         = gimp_rectangle_select_tool_motion;
  tool_class->key_press      = gimp_rectangle_select_tool_key_press;
  tool_class->oper_update    = gimp_rectangle_select_tool_oper_update;
  tool_class->cursor_update  = gimp_rectangle_select_tool_cursor_update;

  klass->select              = gimp_rectangle_select_tool_real_select;

  g_type_class_add_private (klass, sizeof (GimpRectangleSelectToolPrivate));
}

static void
gimp_rectangle_select_tool_init (GimpRectangleSelectTool *rect_tool)
{
  GimpTool *tool = GIMP_TOOL (rect_tool);

  rect_tool->private =
    G_TYPE_INSTANCE_GET_PRIVATE (rect_tool,
                                 GIMP_TYPE_RECTANGLE_SELECT_TOOL,
                                 GimpRectangleSelectToolPrivate);

  gimp_tool_control_set_wants_click      (tool->control, TRUE);
  gimp_tool_control_set_active_modifiers (tool->control,
                                          GIMP_TOOL_ACTIVE_MODIFIERS_SEPARATE);
  gimp_tool_control_set_precision        (tool->control,
                                          GIMP_CURSOR_PRECISION_PIXEL_BORDER);
  gimp_tool_control_set_tool_cursor      (tool->control,
                                          GIMP_TOOL_CURSOR_RECT_SELECT);
  gimp_tool_control_set_preserve         (tool->control, FALSE);
  gimp_tool_control_set_dirty_mask       (tool->control,
                                          GIMP_DIRTY_IMAGE_SIZE |
                                          GIMP_DIRTY_SELECTION);
}

static void
gimp_rectangle_select_tool_control (GimpTool       *tool,
                                    GimpToolAction  action,
                                    GimpDisplay    *display)
{
  GimpRectangleSelectTool *rect_tool = GIMP_RECTANGLE_SELECT_TOOL (tool);

  switch (action)
    {
    case GIMP_TOOL_ACTION_PAUSE:
    case GIMP_TOOL_ACTION_RESUME:
      break;

    case GIMP_TOOL_ACTION_HALT:
      gimp_rectangle_select_tool_halt (rect_tool);
      break;

    case GIMP_TOOL_ACTION_COMMIT:
      gimp_rectangle_select_tool_commit (rect_tool);
      break;
    }

  GIMP_TOOL_CLASS (parent_class)->control (tool, action, display);
}

static void
gimp_rectangle_select_tool_button_press (GimpTool            *tool,
                                         const GimpCoords    *coords,
                                         guint32              time,
                                         GdkModifierType      state,
                                         GimpButtonPressType  press_type,
                                         GimpDisplay         *display)
{
  GimpRectangleSelectTool        *rect_tool = GIMP_RECTANGLE_SELECT_TOOL (tool);
  GimpRectangleSelectToolPrivate *private   = rect_tool->private;
  GimpDisplayShell               *shell     = gimp_display_get_shell (display);
  GimpRectangleFunction           function;

  if (tool->display && display != tool->display)
    gimp_tool_control (tool, GIMP_TOOL_ACTION_COMMIT, tool->display);

  if (gimp_selection_tool_start_edit (GIMP_SELECTION_TOOL (tool),
                                      display, coords))
    {
      /* In some cases we want to finish the rectangle select tool
       * and hand over responsibility to the selection tool
       */
      gimp_tool_control (tool, GIMP_TOOL_ACTION_COMMIT, display);
      gimp_rectangle_select_tool_update_option_defaults (rect_tool, TRUE);
      return;
    }

  if (! tool->display)
    {
      gimp_rectangle_select_tool_start (rect_tool, display);

      gimp_tool_widget_hover (private->widget, coords, state, TRUE);

      /* HACK: force CREATING on a newly created rectangle; otherwise,
       * the above binding of properties would cause the rectangle to
       * start with the size from tool options.
       */
      gimp_tool_rectangle_set_function (GIMP_TOOL_RECTANGLE (private->widget),
                                        GIMP_TOOL_RECTANGLE_CREATING);
    }

  private->saved_show_selection = gimp_display_shell_get_show_selection (shell);

  /* if the shift or ctrl keys are down, we don't want to adjust, we
   * want to create a new rectangle, regardless of pointer loc
   */
  if (state & (gimp_get_extend_selection_mask () |
               gimp_get_modify_selection_mask ()))
    {
      gimp_tool_rectangle_set_function (GIMP_TOOL_RECTANGLE (private->widget),
                                        GIMP_TOOL_RECTANGLE_CREATING);
    }

  if (gimp_tool_widget_button_press (private->widget, coords, time, state,
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
    gimp_tool_rectangle_get_function (GIMP_TOOL_RECTANGLE (private->widget));

  if (function == GIMP_TOOL_RECTANGLE_CREATING)
    {
      private->use_saved_op = FALSE;
    }
  else
    {
      GimpImage      *image      = gimp_display_get_image (tool->display);
      GimpUndoStack  *undo_stack = gimp_image_get_undo_stack (image);
      GimpUndoStack  *redo_stack = gimp_image_get_redo_stack (image);
      GimpUndo       *undo;
      GimpChannelOps  operation;

      undo = gimp_undo_stack_peek (undo_stack);

      if (undo && private->undo == undo)
        {
          /* prevent this change from halting the tool */
          gimp_tool_control_push_preserve (tool->control, TRUE);

          gimp_image_undo (image);

          gimp_tool_control_pop_preserve (tool->control);

          /* we will need to redo if the user cancels or executes */
          private->redo = gimp_undo_stack_peek (redo_stack);
        }

      /* if the operation is "Replace", turn off the marching ants,
       * because they are confusing
       */
      operation = gimp_rectangle_select_tool_get_operation (rect_tool);

      if (operation == GIMP_CHANNEL_OP_REPLACE)
        gimp_display_shell_set_show_selection (shell, FALSE);
    }

  private->undo = NULL;

  gimp_tool_control_activate (tool->control);
}

static void
gimp_rectangle_select_tool_button_release (GimpTool              *tool,
                                           const GimpCoords      *coords,
                                           guint32                time,
                                           GdkModifierType        state,
                                           GimpButtonReleaseType  release_type,
                                           GimpDisplay           *display)
{
  GimpRectangleSelectTool        *rect_tool = GIMP_RECTANGLE_SELECT_TOOL (tool);
  GimpRectangleSelectToolPrivate *priv      = rect_tool->private;
  GimpImage                      *image;

  image = gimp_display_get_image (tool->display);

  gimp_tool_control_halt (tool->control);

  gimp_tool_pop_status (tool, display);
  gimp_display_shell_set_show_selection (gimp_display_get_shell (display),
                                         priv->saved_show_selection);

  /*
   * if the user has not moved the mouse, we need to redo the operation
   * that was undone on button press.
   */
  if (release_type == GIMP_BUTTON_RELEASE_CLICK)
    {
      GimpUndoStack *redo_stack = gimp_image_get_redo_stack (image);
      GimpUndo      *redo       = gimp_undo_stack_peek (redo_stack);

      if (redo && priv->redo == redo)
        {
          /* prevent this from halting the tool */
          gimp_tool_control_push_preserve (tool->control, TRUE);

          gimp_image_redo (image);
          priv->redo = NULL;

          gimp_tool_control_pop_preserve (tool->control);
        }
    }

  if (priv->grab_widget)
    {
      gimp_tool_widget_button_release (priv->grab_widget,
                                       coords, time, state, release_type);
      priv->grab_widget = NULL;
    }

  if (release_type == GIMP_BUTTON_RELEASE_CANCEL)
    {
      if (priv->redo)
        {
          /* prevent this from halting the tool */
          gimp_tool_control_push_preserve (tool->control, TRUE);

          gimp_image_redo (image);

          gimp_tool_control_pop_preserve (tool->control);
        }

      priv->use_saved_op = TRUE;  /* is this correct? */
    }

  priv->redo = NULL;
}

static void
gimp_rectangle_select_tool_motion (GimpTool         *tool,
                                   const GimpCoords *coords,
                                   guint32           time,
                                   GdkModifierType   state,
                                   GimpDisplay      *display)
{
  GimpRectangleSelectTool        *rect_tool = GIMP_RECTANGLE_SELECT_TOOL (tool);
  GimpRectangleSelectToolPrivate *priv      = rect_tool->private;

  if (priv->grab_widget)
    {
      gimp_tool_widget_motion (priv->grab_widget, coords, time, state);
    }
}

static gboolean
gimp_rectangle_select_tool_key_press (GimpTool    *tool,
                                      GdkEventKey *kevent,
                                      GimpDisplay *display)
{
  GimpRectangleSelectTool        *rect_tool = GIMP_RECTANGLE_SELECT_TOOL (tool);
  GimpRectangleSelectToolPrivate *priv      = rect_tool->private;

  if (priv->widget && display == tool->display)
    {
      if (gimp_tool_widget_key_press (priv->widget, kevent))
        return TRUE;
    }

  return gimp_edit_selection_tool_key_press (tool, kevent, display);
}

static void
gimp_rectangle_select_tool_oper_update (GimpTool         *tool,
                                        const GimpCoords *coords,
                                        GdkModifierType   state,
                                        gboolean          proximity,
                                        GimpDisplay      *display)
{
  GimpRectangleSelectTool        *rect_tool = GIMP_RECTANGLE_SELECT_TOOL (tool);
  GimpRectangleSelectToolPrivate *priv      = rect_tool->private;

  if (priv->widget && display == tool->display)
    {
      gimp_tool_widget_hover (priv->widget, coords, state, proximity);
    }

  GIMP_TOOL_CLASS (parent_class)->oper_update (tool, coords, state, proximity,
                                               display);
}

static void
gimp_rectangle_select_tool_cursor_update (GimpTool         *tool,
                                          const GimpCoords *coords,
                                          GdkModifierType   state,
                                          GimpDisplay      *display)
{
  GimpRectangleSelectTool        *rect_tool = GIMP_RECTANGLE_SELECT_TOOL (tool);
  GimpRectangleSelectToolPrivate *private   = rect_tool->private;
  GimpCursorType                  cursor    = GIMP_CURSOR_CROSSHAIR_SMALL;
  GimpCursorModifier              modifier  = GIMP_CURSOR_MODIFIER_NONE;

  if (private->widget && display == tool->display)
    {
      gimp_tool_widget_get_cursor (private->widget, coords, state,
                                   &cursor, NULL, &modifier);

      gimp_tool_control_set_cursor          (tool->control, cursor);
      gimp_tool_control_set_cursor_modifier (tool->control, modifier);
    }

  /* override the previous if shift or ctrl are down */
  if (state & (gimp_get_extend_selection_mask () |
               gimp_get_modify_selection_mask ()))
    {
      gimp_tool_control_set_cursor (tool->control,
                                    GIMP_CURSOR_CROSSHAIR_SMALL);
    }

  GIMP_TOOL_CLASS (parent_class)->cursor_update (tool, coords, state, display);
}

static gboolean
gimp_rectangle_select_tool_select (GimpRectangleSelectTool *rect_tool,
                                   gint                     x,
                                   gint                     y,
                                   gint                     w,
                                   gint                     h)
{
  GimpTool       *tool  = GIMP_TOOL (rect_tool);
  GimpImage      *image = gimp_display_get_image (tool->display);
  gboolean        rectangle_exists;
  GimpChannelOps  operation;

  gimp_tool_pop_status (tool, tool->display);

  rectangle_exists = (x <= gimp_image_get_width  (image) &&
                      y <= gimp_image_get_height (image) &&
                      x + w >= 0                         &&
                      y + h >= 0                         &&
                      w > 0                              &&
                      h > 0);

  operation = gimp_rectangle_select_tool_get_operation (rect_tool);

  /* if rectangle exists, turn it into a selection */
  if (rectangle_exists)
    GIMP_RECTANGLE_SELECT_TOOL_GET_CLASS (rect_tool)->select (rect_tool,
                                                              operation,
                                                              x, y, w, h);

  return rectangle_exists;
}

static void
gimp_rectangle_select_tool_real_select (GimpRectangleSelectTool *rect_tool,
                                        GimpChannelOps           operation,
                                        gint                     x,
                                        gint                     y,
                                        gint                     w,
                                        gint                     h)
{
  GimpTool                   *tool    = GIMP_TOOL (rect_tool);
  GimpSelectionOptions       *options = GIMP_SELECTION_TOOL_GET_OPTIONS (tool);
  GimpRectangleSelectOptions *rect_options;
  GimpChannel                *channel;

  rect_options = GIMP_RECTANGLE_SELECT_TOOL_GET_OPTIONS (tool);

  channel = gimp_image_get_mask (gimp_display_get_image (tool->display));

  if (rect_options->round_corners)
    {
      /* To prevent elliptification of the rectangle,
       * we must cap the corner radius.
       */
      gdouble max    = MIN (w / 2.0, h / 2.0);
      gdouble radius = MIN (rect_options->corner_radius, max);

      gimp_channel_select_round_rect (channel,
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
      gimp_channel_select_rectangle (channel,
                                     x, y, w, h,
                                     operation,
                                     options->feather,
                                     options->feather_radius,
                                     options->feather_radius,
                                     TRUE);
    }
}

static void
gimp_rectangle_select_tool_rectangle_response (GimpToolWidget          *widget,
                                               gint                     response_id,
                                               GimpRectangleSelectTool *rect_tool)
{
  GimpTool                       *tool = GIMP_TOOL (rect_tool);
  GimpRectangleSelectToolPrivate *priv = rect_tool->private;

  switch (response_id)
    {
    case GIMP_TOOL_WIDGET_RESPONSE_CONFIRM:
      {
        gdouble x1, y1, x2, y2;

        gimp_tool_rectangle_get_public_rect (GIMP_TOOL_RECTANGLE (widget),
                                             &x1, &y1, &x2, &y2);
        if (x1 == x2 && y1 == y2)
          {
            /*  if there are no extents, we got here because of a
             *  click, call commit() directly because we might want to
             *  reconfigure the rectangle and continue, instead of
             *  HALTing it like calling COMMIT would do
             */
            gimp_rectangle_select_tool_commit (rect_tool);
          }
        else
          {
            gimp_tool_control (tool, GIMP_TOOL_ACTION_COMMIT, tool->display);
          }
      }
      break;

    case GIMP_TOOL_WIDGET_RESPONSE_CANCEL:
      {
        GimpImage     *image      = gimp_display_get_image (tool->display);
        GimpUndoStack *undo_stack = gimp_image_get_undo_stack (image);
        GimpUndo      *undo       = gimp_undo_stack_peek (undo_stack);

        /* if we have an existing rectangle in the current display, then
         * we have already "executed", and need to undo at this point,
         * unless the user has done something in the meantime
         */
        if (undo && priv->undo == undo)
          {
            /* prevent this change from halting the tool */
            gimp_tool_control_push_preserve (tool->control, TRUE);

            gimp_image_undo (image);
            gimp_image_flush (image);

            gimp_tool_control_pop_preserve (tool->control);
          }

        priv->undo = NULL;
        priv->redo = NULL;

        gimp_tool_control (tool, GIMP_TOOL_ACTION_HALT, tool->display);
      }
      break;
    }
}

static void
gimp_rectangle_select_tool_rectangle_change_complete (GimpToolWidget          *widget,
                                                      GimpRectangleSelectTool *rect_tool)
{
  GimpTool                       *tool = GIMP_TOOL (rect_tool);
  GimpRectangleSelectToolPrivate *priv = rect_tool->private;

  /* prevent change in selection from halting the tool */
  gimp_tool_control_push_preserve (tool->control, TRUE);

  if (tool->display && ! gimp_tool_control_is_active (tool->control))
    {
      GimpImage     *image      = gimp_display_get_image (tool->display);
      GimpUndoStack *undo_stack = gimp_image_get_undo_stack (image);
      GimpUndo      *undo       = gimp_undo_stack_peek (undo_stack);
      gdouble        x1, y1, x2, y2;

      /* if we got here via button release, we have already undone the
       * previous operation.  But if we got here by some other means,
       * we need to undo it now.
       */
      if (undo && priv->undo == undo)
        {
          gimp_image_undo (image);
          priv->undo = NULL;
        }

      gimp_tool_rectangle_get_public_rect (GIMP_TOOL_RECTANGLE (widget),
                                           &x1, &y1, &x2, &y2);

      if (gimp_rectangle_select_tool_select (rect_tool,
                                             x1, y1, x2 - x1, y2 - y1))
        {
          /* save the undo that we got when executing, but only if
           * we actually selected something
           */
          priv->undo = gimp_undo_stack_peek (undo_stack);
          priv->redo = NULL;
        }

      if (! priv->use_saved_op)
        {
          GimpSelectionOptions *options;

          options = GIMP_SELECTION_TOOL_GET_OPTIONS (tool);

          /* remember the operation now in case we modify the rectangle */
          priv->operation    = options->operation;
          priv->use_saved_op = TRUE;
        }

      gimp_image_flush (image);
    }

  gimp_tool_control_pop_preserve (tool->control);

  gimp_rectangle_select_tool_update_option_defaults (rect_tool, FALSE);
}

static void
gimp_rectangle_select_tool_start (GimpRectangleSelectTool *rect_tool,
                                  GimpDisplay             *display)
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

  GimpTool                       *tool    = GIMP_TOOL (rect_tool);
  GimpRectangleSelectToolPrivate *private = rect_tool->private;
  GimpDisplayShell               *shell   = gimp_display_get_shell (display);
  GimpRectangleSelectOptions     *options;
  GimpToolWidget                 *widget;
  gboolean                        draw_ellipse;
  gint                            i;

  options = GIMP_RECTANGLE_SELECT_TOOL_GET_OPTIONS (rect_tool);

  tool->display = display;

  private->widget = widget = gimp_tool_rectangle_new (shell);

  draw_ellipse = GIMP_RECTANGLE_SELECT_TOOL_GET_CLASS (rect_tool)->draw_ellipse;

  g_object_set (widget,
                "draw-ellipse", draw_ellipse,
                "status-title", draw_ellipse ? _("Ellipse: ") : _("Rectangle: "),
                NULL);

  gimp_draw_tool_set_widget (GIMP_DRAW_TOOL (tool), widget);

  for (i = 0; i < G_N_ELEMENTS (properties); i++)
    {
      GBinding *binding =
        g_object_bind_property (G_OBJECT (options), properties[i],
                                G_OBJECT (widget),  properties[i],
                                G_BINDING_SYNC_CREATE |
                                G_BINDING_BIDIRECTIONAL);

      private->bindings = g_list_prepend (private->bindings, binding);
    }

  gimp_rectangle_options_connect (GIMP_RECTANGLE_OPTIONS (options),
                                  gimp_display_get_image (shell->display),
                                  G_CALLBACK (gimp_rectangle_select_tool_auto_shrink),
                                  rect_tool);

  gimp_rectangle_select_tool_update_option_defaults (rect_tool, TRUE);

  g_signal_connect (widget, "response",
                    G_CALLBACK (gimp_rectangle_select_tool_rectangle_response),
                    rect_tool);
  g_signal_connect (widget, "change-complete",
                    G_CALLBACK (gimp_rectangle_select_tool_rectangle_change_complete),
                    rect_tool);

  gimp_draw_tool_start (GIMP_DRAW_TOOL (tool), display);
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
gimp_rectangle_select_tool_commit (GimpRectangleSelectTool *rect_tool)
{
  GimpTool                       *tool = GIMP_TOOL (rect_tool);
  GimpRectangleSelectToolPrivate *priv = rect_tool->private;

  if (priv->widget)
    {
      gdouble x1, y1, x2, y2;
      gint    w, h;

      gimp_tool_rectangle_get_public_rect (GIMP_TOOL_RECTANGLE (priv->widget),
                                           &x1, &y1, &x2, &y2);
      w = x2 - x1;
      h = y2 - y1;

      if (w == 0 && h == 0)
        {
          GimpImage   *image     = gimp_display_get_image (tool->display);
          GimpChannel *selection = gimp_image_get_mask (image);
          gint         press_x;
          gint         press_y;

          if (gimp_image_get_floating_selection (image))
            {
              floating_sel_anchor (gimp_image_get_floating_selection (image));
              gimp_image_flush (image);

              return;
            }

          press_x = ROUND (priv->press_x);
          press_y = ROUND (priv->press_y);

          /*  if the click was inside the marching ants  */
          if (gimp_pickable_get_opacity_at (GIMP_PICKABLE (selection),
                                            press_x, press_y) > 0.5)
            {
              gint x, y, w, h;

              if (gimp_item_bounds (GIMP_ITEM (selection), &x, &y, &w, &h))
                {
                  g_object_set (priv->widget,
                                "x1", (gdouble) x,
                                "y1", (gdouble) y,
                                "x2", (gdouble) (x + w),
                                "y2", (gdouble) (y + h),
                                NULL);
                }

              gimp_rectangle_select_tool_update_option_defaults (rect_tool,
                                                                 FALSE);
              return;
            }
          else
            {
              GimpChannelOps  operation;

              /* prevent this change from halting the tool */
              gimp_tool_control_push_preserve (tool->control, TRUE);

              /* We can conceptually think of a click outside of the
               * selection as adding a 0px selection. Behave intuitivly
               * for the current selection mode
               */
              operation = gimp_rectangle_select_tool_get_operation (rect_tool);

              switch (operation)
                {
                case GIMP_CHANNEL_OP_REPLACE:
                case GIMP_CHANNEL_OP_INTERSECT:
                  gimp_channel_clear (selection, NULL, TRUE);
                  gimp_image_flush (image);
                  break;

                case GIMP_CHANNEL_OP_ADD:
                case GIMP_CHANNEL_OP_SUBTRACT:
                default:
                  /* Do nothing */
                  break;
                }

              gimp_tool_control_pop_preserve (tool->control);
            }
        }

      gimp_rectangle_select_tool_update_option_defaults (rect_tool, FALSE);
    }

  /* Reset the automatic undo/redo mechanism */
  priv->undo = NULL;
  priv->redo = NULL;
}

static void
gimp_rectangle_select_tool_halt (GimpRectangleSelectTool *rect_tool)
{
  GimpTool                       *tool = GIMP_TOOL (rect_tool);
  GimpRectangleSelectToolPrivate *priv = rect_tool->private;
  GimpRectangleSelectOptions     *options;

  options = GIMP_RECTANGLE_SELECT_TOOL_GET_OPTIONS (rect_tool);

  if (tool->display)
    {
      GimpDisplayShell *shell = gimp_display_get_shell (tool->display);

      gimp_display_shell_set_highlight (shell, NULL, 0.0);

      gimp_rectangle_options_disconnect (GIMP_RECTANGLE_OPTIONS (options),
                                         G_CALLBACK (gimp_rectangle_select_tool_auto_shrink),
                                         rect_tool);
    }

  priv->undo = NULL;
  priv->redo = NULL;

  if (gimp_draw_tool_is_active (GIMP_DRAW_TOOL (tool)))
    gimp_draw_tool_stop (GIMP_DRAW_TOOL (tool));

  /*  disconnect bindings manually so they are really gone *now*, we
   *  might be in the middle of a signal emission that keeps the
   *  widget and its bindings alive.
   */
  g_list_free_full (priv->bindings, (GDestroyNotify) g_object_unref);
  priv->bindings = NULL;

  gimp_draw_tool_set_widget (GIMP_DRAW_TOOL (tool), NULL);
  g_clear_object (&priv->widget);

  tool->display = NULL;

  gimp_rectangle_select_tool_update_option_defaults (rect_tool, TRUE);
}

static GimpChannelOps
gimp_rectangle_select_tool_get_operation (GimpRectangleSelectTool *rect_tool)
{
  GimpRectangleSelectToolPrivate *priv = rect_tool->private;
  GimpSelectionOptions           *options;

  options = GIMP_SELECTION_TOOL_GET_OPTIONS (rect_tool);

  if (priv->use_saved_op)
    return priv->operation;
  else
    return options->operation;
}

/**
 * gimp_rectangle_select_tool_update_option_defaults:
 * @crop_tool:
 * @ignore_pending: %TRUE to ignore any pending crop rectangle.
 *
 * Sets the default Fixed: Aspect ratio and Fixed: Size option
 * properties.
 */
static void
gimp_rectangle_select_tool_update_option_defaults (GimpRectangleSelectTool *rect_tool,
                                                   gboolean                 ignore_pending)
{
  GimpRectangleSelectToolPrivate *priv = rect_tool->private;
  GimpTool                       *tool = GIMP_TOOL (rect_tool);
  GimpRectangleOptions           *rect_options;

  rect_options = GIMP_RECTANGLE_OPTIONS (gimp_tool_get_options (tool));

  if (priv->widget && ! ignore_pending)
    {
      /* There is a pending rectangle and we should not ignore it, so
       * set default Fixed: Size to the same as the current pending
       * rectangle width/height.
       */

      gimp_tool_rectangle_pending_size_set (GIMP_TOOL_RECTANGLE (priv->widget),
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
gimp_rectangle_select_tool_auto_shrink (GimpRectangleSelectTool *rect_tool)
{
  GimpRectangleSelectToolPrivate *private = rect_tool->private;
  gboolean                        shrink_merged;

  g_object_get (gimp_tool_get_options (GIMP_TOOL (rect_tool)),
                "shrink-merged", &shrink_merged,
                NULL);

  gimp_tool_rectangle_auto_shrink (GIMP_TOOL_RECTANGLE (private->widget),
                                   shrink_merged);
}
