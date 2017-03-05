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

#include "core/gimp-utils.h"
#include "core/gimpchannel-select.h"
#include "core/gimpchannel.h"
#include "core/gimpimage.h"
#include "core/gimpimage-undo.h"
#include "core/gimplayer-floating-selection.h"
#include "core/gimppickable.h"
#include "core/gimpundostack.h"

#include "widgets/gimpdialogfactory.h"
#include "widgets/gimphelp-ids.h"
#include "widgets/gimpviewabledialog.h"
#include "widgets/gimpwidgets-utils.h"

#include "display/gimpcanvasgroup.h"
#include "display/gimpdisplay.h"
#include "display/gimpdisplayshell.h"
#include "display/gimpdisplayshell-transform.h"
#include "display/gimpdisplayshell-appearance.h"

#include "gimpeditselectiontool.h"
#include "gimpselectiontool.h"
#include "gimpselectionoptions.h"
#include "gimprectangletool.h"
#include "gimprectangleoptions.h"
#include "gimprectangleselecttool.h"
#include "gimprectangleselectoptions.h"
#include "gimptoolcontrol.h"

#include "gimp-intl.h"


typedef struct GimpRectangleSelectToolPrivate
{
  GimpChannelOps     operation;            /* remember for use when modifying   */
  gboolean           use_saved_op;         /* use operation or get from options */
  gboolean           saved_show_selection; /* used to remember existing value   */
  GimpUndo          *undo;
  GimpUndo          *redo;

  gboolean           round_corners;
  gdouble            corner_radius;

  gdouble            press_x;
  gdouble            press_y;
} GimpRectangleSelectToolPrivate;


#define GIMP_RECTANGLE_SELECT_TOOL_GET_PRIVATE(obj) \
  ((GimpRectangleSelectToolPrivate *) ((GimpRectangleSelectTool *) (obj))->priv)


static void     gimp_rectangle_select_tool_rectangle_tool_iface_init (GimpRectangleToolInterface *iface);

static void     gimp_rectangle_select_tool_constructed    (GObject               *object);

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
static void     gimp_rectangle_select_tool_active_modifier_key
                                                          (GimpTool              *tool,
                                                           GdkModifierType        key,
                                                           gboolean               press,
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
static void     gimp_rectangle_select_tool_draw           (GimpDrawTool          *draw_tool);
static gboolean gimp_rectangle_select_tool_select         (GimpRectangleTool     *rect_tool,
                                                           gint                   x,
                                                           gint                   y,
                                                           gint                   w,
                                                           gint                   h);
static gboolean gimp_rectangle_select_tool_execute        (GimpRectangleTool     *rect_tool,
                                                           gint                   x,
                                                           gint                   y,
                                                           gint                   w,
                                                           gint                   h);
static void     gimp_rectangle_select_tool_cancel         (GimpRectangleTool     *rect_tool);
static gboolean gimp_rectangle_select_tool_rectangle_change_complete
                                                          (GimpRectangleTool     *rect_tool);
static void     gimp_rectangle_select_tool_real_select    (GimpRectangleSelectTool *rect_sel_tool,
                                                           GimpChannelOps         operation,
                                                           gint                   x,
                                                           gint                   y,
                                                           gint                   w,
                                                           gint                   h);
static GimpChannelOps
                gimp_rectangle_select_tool_get_operation  (GimpRectangleSelectTool    *rect_sel_tool);
static void     gimp_rectangle_select_tool_update_option_defaults
                                                          (GimpRectangleSelectTool    *rect_sel_tool,
                                                           gboolean                    ignore_pending);

static void     gimp_rectangle_select_tool_round_corners_notify
                                                          (GimpRectangleSelectOptions *options,
                                                           GParamSpec                 *pspec,
                                                           GimpRectangleSelectTool    *rect_sel_tool);


G_DEFINE_TYPE_WITH_CODE (GimpRectangleSelectTool, gimp_rectangle_select_tool,
                         GIMP_TYPE_SELECTION_TOOL,
                         G_IMPLEMENT_INTERFACE (GIMP_TYPE_RECTANGLE_TOOL,
                                                gimp_rectangle_select_tool_rectangle_tool_iface_init))

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
  GObjectClass      *object_class    = G_OBJECT_CLASS (klass);
  GimpToolClass     *tool_class      = GIMP_TOOL_CLASS (klass);
  GimpDrawToolClass *draw_tool_class = GIMP_DRAW_TOOL_CLASS (klass);

  g_type_class_add_private (klass, sizeof (GimpRectangleSelectToolPrivate));

  object_class->constructed       = gimp_rectangle_select_tool_constructed;
  object_class->set_property      = gimp_rectangle_tool_set_property;
  object_class->get_property      = gimp_rectangle_tool_get_property;

  tool_class->control             = gimp_rectangle_select_tool_control;
  tool_class->button_press        = gimp_rectangle_select_tool_button_press;
  tool_class->button_release      = gimp_rectangle_select_tool_button_release;
  tool_class->motion              = gimp_rectangle_tool_motion;
  tool_class->key_press           = gimp_rectangle_select_tool_key_press;
  tool_class->active_modifier_key = gimp_rectangle_select_tool_active_modifier_key;
  tool_class->oper_update         = gimp_rectangle_select_tool_oper_update;
  tool_class->cursor_update       = gimp_rectangle_select_tool_cursor_update;

  draw_tool_class->draw           = gimp_rectangle_select_tool_draw;

  klass->select                   = gimp_rectangle_select_tool_real_select;

  gimp_rectangle_tool_install_properties (object_class);
}

static void
gimp_rectangle_select_tool_rectangle_tool_iface_init (GimpRectangleToolInterface *iface)
{
  iface->execute                   = gimp_rectangle_select_tool_execute;
  iface->cancel                    = gimp_rectangle_select_tool_cancel;
  iface->rectangle_change_complete = gimp_rectangle_select_tool_rectangle_change_complete;
}

static void
gimp_rectangle_select_tool_init (GimpRectangleSelectTool *rect_sel_tool)
{
  GimpTool                       *tool = GIMP_TOOL (rect_sel_tool);
  GimpRectangleSelectToolPrivate *priv;

  gimp_rectangle_tool_init (GIMP_RECTANGLE_TOOL (rect_sel_tool));

  rect_sel_tool->priv = G_TYPE_INSTANCE_GET_PRIVATE (rect_sel_tool,
                                                     GIMP_TYPE_RECTANGLE_SELECT_TOOL,
                                                     GimpRectangleSelectToolPrivate);

  priv = GIMP_RECTANGLE_SELECT_TOOL_GET_PRIVATE (rect_sel_tool);

  gimp_tool_control_set_wants_click (tool->control, TRUE);
  gimp_tool_control_set_precision   (tool->control,
                                     GIMP_CURSOR_PRECISION_PIXEL_BORDER);
  gimp_tool_control_set_tool_cursor (tool->control,
                                     GIMP_TOOL_CURSOR_RECT_SELECT);
  gimp_tool_control_set_preserve    (tool->control, FALSE);
  gimp_tool_control_set_dirty_mask  (tool->control,
                                     GIMP_DIRTY_IMAGE_SIZE |
                                     GIMP_DIRTY_SELECTION);

  priv->undo    = NULL;
  priv->redo    = NULL;

  priv->press_x = 0.0;
  priv->press_y = 0.0;
}

static void
gimp_rectangle_select_tool_constructed (GObject *object)
{
  GimpRectangleSelectTool        *rect_sel_tool;
  GimpRectangleSelectOptions     *options;
  GimpRectangleSelectToolPrivate *priv;

  G_OBJECT_CLASS (parent_class)->constructed (object);

  gimp_rectangle_tool_constructor (object);

  rect_sel_tool = GIMP_RECTANGLE_SELECT_TOOL (object);
  options       = GIMP_RECTANGLE_SELECT_TOOL_GET_OPTIONS (rect_sel_tool);
  priv          = GIMP_RECTANGLE_SELECT_TOOL_GET_PRIVATE (rect_sel_tool);

  priv->round_corners = options->round_corners;
  priv->corner_radius = options->corner_radius;

  g_signal_connect_object (options, "notify::round-corners",
                           G_CALLBACK (gimp_rectangle_select_tool_round_corners_notify),
                           object, 0);
  g_signal_connect_object (options, "notify::corner-radius",
                           G_CALLBACK (gimp_rectangle_select_tool_round_corners_notify),
                           object, 0);

  gimp_rectangle_select_tool_update_option_defaults (rect_sel_tool, FALSE);
}

static void
gimp_rectangle_select_tool_control (GimpTool       *tool,
                                    GimpToolAction  action,
                                    GimpDisplay    *display)
{
  gimp_rectangle_tool_control (tool, action, display);

  GIMP_TOOL_CLASS (parent_class)->control (tool, action, display);
}

static void
gimp_rectangle_select_tool_draw (GimpDrawTool *draw_tool)
{
  GimpRectangleSelectTool        *rect_sel_tool;
  GimpRectangleSelectToolPrivate *priv;
  GimpCanvasGroup                *stroke_group = NULL;

  rect_sel_tool = GIMP_RECTANGLE_SELECT_TOOL (draw_tool);
  priv          = GIMP_RECTANGLE_SELECT_TOOL_GET_PRIVATE (rect_sel_tool);

  if (priv->round_corners)
    {
      GimpCanvasItem *item;
      gint            x1, y1, x2, y2;
      gdouble         radius;
      gint            square_size;

      g_object_get (rect_sel_tool,
                    "x1", &x1,
                    "y1", &y1,
                    "x2", &x2,
                    "y2", &y2,
                    NULL);

      radius = MIN (priv->corner_radius,
                    MIN ((x2 - x1) / 2.0, (y2 - y1) / 2.0));

      square_size = (int) (radius * 2);

      stroke_group =
        GIMP_CANVAS_GROUP (gimp_draw_tool_add_stroke_group (draw_tool));

      item = gimp_draw_tool_add_arc (draw_tool, FALSE,
                                     x1, y1,
                                     square_size, square_size,
                                     G_PI / 2.0, G_PI / 2.0);
      gimp_canvas_group_add_item (stroke_group, item);
      gimp_draw_tool_remove_item (draw_tool, item);

      item = gimp_draw_tool_add_arc (draw_tool, FALSE,
                                     x2 - square_size, y1,
                                     square_size, square_size,
                                     0.0, G_PI / 2.0);
      gimp_canvas_group_add_item (stroke_group, item);
      gimp_draw_tool_remove_item (draw_tool, item);

      item = gimp_draw_tool_add_arc (draw_tool, FALSE,
                                     x2 - square_size, y2 - square_size,
                                     square_size, square_size,
                                     G_PI * 1.5, G_PI / 2.0);
      gimp_canvas_group_add_item (stroke_group, item);
      gimp_draw_tool_remove_item (draw_tool, item);

      item = gimp_draw_tool_add_arc (draw_tool, FALSE,
                                     x1, y2 - square_size,
                                     square_size, square_size,
                                     G_PI, G_PI / 2.0);
      gimp_canvas_group_add_item (stroke_group, item);
      gimp_draw_tool_remove_item (draw_tool, item);
    }

  gimp_rectangle_tool_draw (draw_tool, stroke_group);
}

static void
gimp_rectangle_select_tool_button_press (GimpTool            *tool,
                                         const GimpCoords    *coords,
                                         guint32              time,
                                         GdkModifierType      state,
                                         GimpButtonPressType  press_type,
                                         GimpDisplay         *display)
{
  GimpRectangleTool              *rectangle;
  GimpRectangleSelectTool        *rect_sel_tool;
  GimpDisplayShell               *shell;
  GimpRectangleSelectToolPrivate *priv;
  GimpRectangleFunction           function;

  rectangle     = GIMP_RECTANGLE_TOOL (tool);
  rect_sel_tool = GIMP_RECTANGLE_SELECT_TOOL (tool);
  shell         = gimp_display_get_shell (display);
  priv          = GIMP_RECTANGLE_SELECT_TOOL_GET_PRIVATE (rect_sel_tool);

  if (tool->display && display != tool->display)
    gimp_tool_control (tool, GIMP_TOOL_ACTION_COMMIT, tool->display);

  if (gimp_selection_tool_start_edit (GIMP_SELECTION_TOOL (tool),
                                      display, coords))
    {
      /* In some cases we want to finish the rectangle select tool
       * and hand over responsibility to the selection tool
       */
      gimp_rectangle_tool_control (tool, GIMP_TOOL_ACTION_COMMIT, display);
      gimp_rectangle_select_tool_update_option_defaults (rect_sel_tool,
                                                         TRUE);
      return;
    }

  gimp_tool_control_activate (tool->control);

  priv->saved_show_selection = gimp_display_shell_get_show_selection (shell);

  /* if the shift or ctrl keys are down, we don't want to adjust, we
   * want to create a new rectangle, regardless of pointer loc */
  if (state & (gimp_get_extend_selection_mask () |
               gimp_get_modify_selection_mask ()))
    {
      gimp_rectangle_tool_set_function (rectangle,
                                        GIMP_RECTANGLE_TOOL_CREATING);
    }

  gimp_rectangle_tool_button_press (tool, coords, time, state, display);

  priv->press_x = coords->x;
  priv->press_y = coords->y;

  /* if we have an existing rectangle in the current display, then
   * we have already "executed", and need to undo at this point,
   * unless the user has done something in the meantime
   */
  function = gimp_rectangle_tool_get_function (rectangle);

  if (function == GIMP_RECTANGLE_TOOL_CREATING)
    {
      priv->use_saved_op = FALSE;
    }
  else
    {
      GimpImage      *image      = gimp_display_get_image (tool->display);
      GimpUndoStack  *undo_stack = gimp_image_get_undo_stack (image);
      GimpUndoStack  *redo_stack = gimp_image_get_redo_stack (image);
      GimpUndo       *undo;
      GimpChannelOps  operation;

      undo = gimp_undo_stack_peek (undo_stack);

      if (undo && priv->undo == undo)
        {
          /* prevent this change from halting the tool */
          gimp_tool_control_push_preserve (tool->control, TRUE);

          gimp_image_undo (image);

          gimp_tool_control_pop_preserve (tool->control);

          /* we will need to redo if the user cancels or executes */
          priv->redo = gimp_undo_stack_peek (redo_stack);
        }

      /* if the operation is "Replace", turn off the marching ants,
         because they are confusing */
      operation = gimp_rectangle_select_tool_get_operation (rect_sel_tool);

      if (operation == GIMP_CHANNEL_OP_REPLACE)
        gimp_display_shell_set_show_selection (shell, FALSE);
    }

  priv->undo = NULL;
}

static void
gimp_rectangle_select_tool_button_release (GimpTool              *tool,
                                           const GimpCoords      *coords,
                                           guint32                time,
                                           GdkModifierType        state,
                                           GimpButtonReleaseType  release_type,
                                           GimpDisplay           *display)
{
  GimpRectangleSelectTool        *rect_sel_tool;
  GimpRectangleSelectToolPrivate *priv;
  GimpImage                      *image;

  rect_sel_tool = GIMP_RECTANGLE_SELECT_TOOL (tool);
  priv          = GIMP_RECTANGLE_SELECT_TOOL_GET_PRIVATE (rect_sel_tool);

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

  gimp_rectangle_tool_button_release (tool, coords, time, state, release_type,
                                      display);

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
gimp_rectangle_select_tool_active_modifier_key (GimpTool        *tool,
                                                GdkModifierType  key,
                                                gboolean         press,
                                                GdkModifierType  state,
                                                GimpDisplay     *display)
{
  GIMP_TOOL_CLASS (parent_class)->active_modifier_key (tool, key, press, state,
                                                       display);

  gimp_rectangle_tool_active_modifier_key (tool, key, press, state, display);
}

static gboolean
gimp_rectangle_select_tool_key_press (GimpTool    *tool,
                                      GdkEventKey *kevent,
                                      GimpDisplay *display)
{
  return (gimp_rectangle_tool_key_press (tool, kevent, display) ||
          gimp_edit_selection_tool_key_press (tool, kevent, display));
}

static void
gimp_rectangle_select_tool_oper_update (GimpTool         *tool,
                                        const GimpCoords *coords,
                                        GdkModifierType   state,
                                        gboolean          proximity,
                                        GimpDisplay      *display)
{
  gimp_rectangle_tool_oper_update (tool, coords, state, proximity, display);

  GIMP_TOOL_CLASS (parent_class)->oper_update (tool, coords, state, proximity,
                                               display);
}

static void
gimp_rectangle_select_tool_cursor_update (GimpTool         *tool,
                                          const GimpCoords *coords,
                                          GdkModifierType   state,
                                          GimpDisplay      *display)
{
  gimp_rectangle_tool_cursor_update (tool, coords, state, display);

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
gimp_rectangle_select_tool_select (GimpRectangleTool *rectangle,
                                   gint               x,
                                   gint               y,
                                   gint               w,
                                   gint               h)
{
  GimpTool                *tool;
  GimpRectangleSelectTool *rect_sel_tool;
  GimpImage               *image;
  gboolean                 rectangle_exists;
  GimpChannelOps           operation;

  tool          = GIMP_TOOL (rectangle);
  rect_sel_tool = GIMP_RECTANGLE_SELECT_TOOL (rectangle);

  image         = gimp_display_get_image (tool->display);

  gimp_tool_pop_status (tool, tool->display);

  rectangle_exists = (x <= gimp_image_get_width  (image) &&
                      y <= gimp_image_get_height (image) &&
                      x + w >= 0                         &&
                      y + h >= 0                         &&
                      w > 0                              &&
                      h > 0);

  operation = gimp_rectangle_select_tool_get_operation (rect_sel_tool);

  /* if rectangle exists, turn it into a selection */
  if (rectangle_exists)
    GIMP_RECTANGLE_SELECT_TOOL_GET_CLASS (rect_sel_tool)->select (rect_sel_tool,
                                                                  operation,
                                                                  x, y, w, h);

  return rectangle_exists;
}

static void
gimp_rectangle_select_tool_real_select (GimpRectangleSelectTool *rect_sel_tool,
                                        GimpChannelOps           operation,
                                        gint                     x,
                                        gint                     y,
                                        gint                     w,
                                        gint                     h)
{
  GimpTool                   *tool    = GIMP_TOOL (rect_sel_tool);
  GimpSelectionOptions       *options = GIMP_SELECTION_TOOL_GET_OPTIONS (tool);
  GimpRectangleSelectOptions *rect_select_options;
  GimpChannel                *channel;

  rect_select_options = GIMP_RECTANGLE_SELECT_TOOL_GET_OPTIONS (tool);

  channel = gimp_image_get_mask (gimp_display_get_image (tool->display));

  if (rect_select_options->round_corners)
    {
      /* To prevent elliptification of the rectangle,
       * we must cap the corner radius.
       */
      gdouble max    = MIN (w / 2.0, h / 2.0);
      gdouble radius = MIN (rect_select_options->corner_radius, max);

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

static GimpChannelOps
gimp_rectangle_select_tool_get_operation (GimpRectangleSelectTool *rect_sel_tool)
{
  GimpRectangleSelectToolPrivate *priv;
  GimpSelectionOptions           *options;

  priv    = GIMP_RECTANGLE_SELECT_TOOL_GET_PRIVATE (rect_sel_tool);
  options = GIMP_SELECTION_TOOL_GET_OPTIONS (rect_sel_tool);

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
gimp_rectangle_select_tool_update_option_defaults (GimpRectangleSelectTool *rect_sel_tool,
                                                   gboolean                 ignore_pending)
{
  GimpTool             *tool;
  GimpRectangleTool    *rectangle_tool;
  GimpRectangleOptions *rectangle_options;

  tool              = GIMP_TOOL (rect_sel_tool);
  rectangle_tool    = GIMP_RECTANGLE_TOOL (tool);
  rectangle_options = GIMP_RECTANGLE_TOOL_GET_OPTIONS (rectangle_tool);

  if (tool->display != NULL && !ignore_pending)
    {
      /* There is a pending rectangle and we should not ignore it, so
       * set default Fixed: Size to the same as the current pending
       * rectangle width/height.
       */

      gimp_rectangle_tool_pending_size_set (rectangle_tool,
                                            G_OBJECT (rectangle_options),
                                            "default-aspect-numerator",
                                            "default-aspect-denominator");

      g_object_set (G_OBJECT (rectangle_options),
                    "use-string-current", TRUE,
                    NULL);
    }
  else
    {
      g_object_set (G_OBJECT (rectangle_options),
                    "default-aspect-numerator",   1.0,
                    "default-aspect-denominator", 1.0,
                    NULL);

      g_object_set (G_OBJECT (rectangle_options),
                    "use-string-current", FALSE,
                    NULL);
    }
}

/*
 * This function is called if the user clicks and releases the left
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
static gboolean
gimp_rectangle_select_tool_execute (GimpRectangleTool *rectangle,
                                    gint               x,
                                    gint               y,
                                    gint               w,
                                    gint               h)
{
  GimpTool                       *tool = GIMP_TOOL (rectangle);
  GimpRectangleSelectTool        *rect_sel_tool;
  GimpRectangleSelectToolPrivate *priv;

  rect_sel_tool = GIMP_RECTANGLE_SELECT_TOOL (rectangle);
  priv          = GIMP_RECTANGLE_SELECT_TOOL_GET_PRIVATE (rect_sel_tool);

  if (w == 0 && h == 0 && tool->display != NULL)
    {
      GimpImage   *image     = gimp_display_get_image (tool->display);
      GimpChannel *selection = gimp_image_get_mask (image);
      gint         pressx;
      gint         pressy;

      if (gimp_image_get_floating_selection (image))
        {
          floating_sel_anchor (gimp_image_get_floating_selection (image));
          gimp_image_flush (image);
          return TRUE;
        }

      pressx = ROUND (priv->press_x);
      pressy = ROUND (priv->press_y);

      /*  if the click was inside the marching ants  */
      if (gimp_pickable_get_opacity_at (GIMP_PICKABLE (selection),
                                        pressx, pressy) > 0.5)
        {
          gint x, y, w, h;

          if (gimp_item_bounds (GIMP_ITEM (selection), &x, &y, &w, &h))
            {
              g_object_set (rectangle,
                            "x1", x,
                            "y1", y,
                            "x2", x + w,
                            "y2", y + h,
                            NULL);
            }

          gimp_rectangle_tool_set_function (rectangle,
                                            GIMP_RECTANGLE_TOOL_MOVING);
          gimp_rectangle_select_tool_update_option_defaults (rect_sel_tool,
                                                             FALSE);

          return FALSE;
        }
      else
        {
          GimpTool       *tool = GIMP_TOOL (rectangle);
          GimpChannelOps  operation;

          /* prevent this change from halting the tool */
          gimp_tool_control_push_preserve (tool->control, TRUE);

          /* We can conceptually think of a click outside of the
           * selection as adding a 0px selection. Behave intuitivly
           * for the current selection mode
           */
          operation = gimp_rectangle_select_tool_get_operation (rect_sel_tool);
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

  gimp_rectangle_select_tool_update_option_defaults (rect_sel_tool, FALSE);

  /* Reset the automatic undo/redo mechanism */
  priv->undo = NULL;
  priv->redo = NULL;

  return TRUE;
}

static void
gimp_rectangle_select_tool_cancel (GimpRectangleTool *rectangle)
{
  GimpTool                       *tool;
  GimpRectangleSelectTool        *rect_sel_tool;
  GimpRectangleSelectToolPrivate *priv;

  tool          = GIMP_TOOL (rectangle);
  rect_sel_tool = GIMP_RECTANGLE_SELECT_TOOL (rectangle);
  priv          = GIMP_RECTANGLE_SELECT_TOOL_GET_PRIVATE (rect_sel_tool);

  if (tool->display)
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
    }

  gimp_rectangle_select_tool_update_option_defaults (rect_sel_tool, TRUE);

  priv->undo = NULL;
  priv->redo = NULL;
}

static gboolean
gimp_rectangle_select_tool_rectangle_change_complete (GimpRectangleTool *rectangle)
{
  GimpTool                       *tool;
  GimpRectangleSelectTool        *rect_sel_tool;
  GimpRectangleSelectToolPrivate *priv;

  tool          = GIMP_TOOL (rectangle);
  rect_sel_tool = GIMP_RECTANGLE_SELECT_TOOL (tool);
  priv          = GIMP_RECTANGLE_SELECT_TOOL_GET_PRIVATE (rect_sel_tool);

  /* prevent change in selection from halting the tool */
  gimp_tool_control_push_preserve (tool->control, TRUE);

  if (tool->display && ! gimp_tool_control_is_active (tool->control))
    {
      GimpImage     *image      = gimp_display_get_image (tool->display);
      GimpUndoStack *undo_stack = gimp_image_get_undo_stack (image);
      GimpUndo      *undo       = gimp_undo_stack_peek (undo_stack);
      gint           x1, y1, x2, y2;

      /* if we got here via button release, we have already undone the
       * previous operation.  But if we got here by some other means,
       * we need to undo it now.
       */
      if (undo && priv->undo == undo)
        {
          gimp_image_undo (image);
          priv->undo = NULL;
        }

      g_object_get (rectangle,
                    "x1", &x1,
                    "y1", &y1,
                    "x2", &x2,
                    "y2", &y2,
                    NULL);

      if (gimp_rectangle_select_tool_select (rectangle, x1, y1, x2 - x1, y2 - y1))
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

  gimp_rectangle_select_tool_update_option_defaults (rect_sel_tool, FALSE);

  return TRUE;
}

static void
gimp_rectangle_select_tool_round_corners_notify (GimpRectangleSelectOptions *options,
                                                 GParamSpec                 *pspec,
                                                 GimpRectangleSelectTool    *rect_sel_tool)
{
  GimpDrawTool                   *draw_tool = GIMP_DRAW_TOOL (rect_sel_tool);
  GimpRectangleTool              *rect_tool = GIMP_RECTANGLE_TOOL (rect_sel_tool);
  GimpRectangleSelectToolPrivate *priv;

  priv = GIMP_RECTANGLE_SELECT_TOOL_GET_PRIVATE (rect_sel_tool);

  gimp_draw_tool_pause (draw_tool);

  priv->round_corners = options->round_corners;
  priv->corner_radius = options->corner_radius;

  gimp_rectangle_select_tool_rectangle_change_complete (rect_tool);

  gimp_draw_tool_resume (draw_tool);
}
