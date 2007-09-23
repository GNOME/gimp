/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include "config.h"

#include <gtk/gtk.h>

#include "libgimpwidgets/gimpwidgets.h"

#include "tools-types.h"

#include "base/boundary.h"

#include "core/gimpchannel.h"
#include "core/gimpchannel-select.h"
#include "core/gimplayer-floating-sel.h"
#include "core/gimpimage.h"
#include "core/gimpimage-undo.h"
#include "core/gimppickable.h"
#include "core/gimp-utils.h"
#include "core/gimpundostack.h"

#include "widgets/gimpdialogfactory.h"
#include "widgets/gimphelp-ids.h"
#include "widgets/gimpviewabledialog.h"
#include "widgets/gimpwidgets-utils.h"

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


static void     gimp_rect_select_tool_rectangle_tool_iface_init (GimpRectangleToolInterface *iface);

static GObject *gimp_rect_select_tool_constructor         (GType              type,
                                                           guint              n_params,
                                                           GObjectConstructParam *params);
static void     gimp_rect_select_tool_control             (GimpTool          *tool,
                                                           GimpToolAction     action,
                                                           GimpDisplay       *display);
static void     gimp_rect_select_tool_button_press        (GimpTool          *tool,
                                                           GimpCoords        *coords,
                                                           guint32            time,
                                                           GdkModifierType    state,
                                                           GimpDisplay       *display);
static void     gimp_rect_select_tool_button_release      (GimpTool          *tool,
                                                           GimpCoords        *coords,
                                                           guint32            time,
                                                           GdkModifierType    state,
                                                           GimpButtonReleaseType release_type,
                                                           GimpDisplay       *display);
static void     gimp_rect_select_tool_active_modifier_key (GimpTool          *tool,
                                                           GdkModifierType    key,
                                                           gboolean           press,
                                                           GdkModifierType    state,
                                                           GimpDisplay       *display);
static gboolean gimp_rect_select_tool_key_press           (GimpTool          *tool,
                                                           GdkEventKey       *kevent,
                                                           GimpDisplay       *display);
static void     gimp_rect_select_tool_oper_update         (GimpTool          *tool,
                                                           GimpCoords        *coords,
                                                           GdkModifierType    state,
                                                           gboolean           proximity,
                                                           GimpDisplay       *display);
static void     gimp_rect_select_tool_cursor_update       (GimpTool          *tool,
                                                           GimpCoords        *coords,
                                                           GdkModifierType    state,
                                                           GimpDisplay       *display);
static void     gimp_rect_select_tool_draw                (GimpDrawTool      *draw_tool);
static gboolean gimp_rect_select_tool_select              (GimpRectangleTool *rect_tool,
                                                           gint               x,
                                                           gint               y,
                                                           gint               w,
                                                           gint               h);
static gboolean gimp_rect_select_tool_execute             (GimpRectangleTool *rect_tool,
                                                           gint               x,
                                                           gint               y,
                                                           gint               w,
                                                           gint               h);
static void     gimp_rect_select_tool_cancel              (GimpRectangleTool *rect_tool);
static gboolean gimp_rect_select_tool_rectangle_changed   (GimpRectangleTool *rect_tool);
static void     gimp_rect_select_tool_real_select         (GimpRectSelectTool *rect_select,
                                                           GimpChannelOps      operation,
                                                           gint                x,
                                                           gint                y,
                                                           gint                w,
                                                           gint                h);
static gboolean gimp_rect_select_tool_selection_visible   (GimpRectSelectTool*rect_select_tool);
static void     gimp_rect_select_tool_update_option_defaults
                                                          (GimpRectSelectTool *rect_select_tool,
                                                           gboolean            ignore_pending);

static void    gimp_rect_select_tool_round_corners_notify (GimpRectSelectOptions *options,
                                                           GParamSpec            *pspec,
                                                           GimpRectSelectTool    *rect_sel);


G_DEFINE_TYPE_WITH_CODE (GimpRectSelectTool, gimp_rect_select_tool,
                         GIMP_TYPE_SELECTION_TOOL,
                         G_IMPLEMENT_INTERFACE (GIMP_TYPE_RECTANGLE_TOOL,
                                                gimp_rect_select_tool_rectangle_tool_iface_init))

#define parent_class gimp_rect_select_tool_parent_class


void
gimp_rect_select_tool_register (GimpToolRegisterCallback  callback,
                                gpointer                  data)
{
  (* callback) (GIMP_TYPE_RECT_SELECT_TOOL,
                GIMP_TYPE_RECT_SELECT_OPTIONS,
                gimp_rect_select_options_gui,
                0,
                "gimp-rect-select-tool",
                _("Rectangle Select"),
                _("Rectangle Select Tool: Select a rectangular region"),
                N_("_Rectangle Select"), "R",
                NULL, GIMP_HELP_TOOL_RECT_SELECT,
                GIMP_STOCK_TOOL_RECT_SELECT,
                data);
}

static void
gimp_rect_select_tool_class_init (GimpRectSelectToolClass *klass)
{
  GObjectClass      *object_class    = G_OBJECT_CLASS (klass);
  GimpToolClass     *tool_class      = GIMP_TOOL_CLASS (klass);
  GimpDrawToolClass *draw_tool_class = GIMP_DRAW_TOOL_CLASS (klass);

  object_class->constructor       = gimp_rect_select_tool_constructor;
  object_class->set_property      = gimp_rectangle_tool_set_property;
  object_class->get_property      = gimp_rectangle_tool_get_property;

  gimp_rectangle_tool_install_properties (object_class);

  tool_class->control             = gimp_rect_select_tool_control;
  tool_class->button_press        = gimp_rect_select_tool_button_press;
  tool_class->button_release      = gimp_rect_select_tool_button_release;
  tool_class->motion              = gimp_rectangle_tool_motion;
  tool_class->key_press           = gimp_rect_select_tool_key_press;
  tool_class->active_modifier_key = gimp_rect_select_tool_active_modifier_key;
  tool_class->oper_update         = gimp_rect_select_tool_oper_update;
  tool_class->cursor_update       = gimp_rect_select_tool_cursor_update;

  draw_tool_class->draw           = gimp_rect_select_tool_draw;

  klass->select                   = gimp_rect_select_tool_real_select;
}

static void
gimp_rect_select_tool_rectangle_tool_iface_init (GimpRectangleToolInterface *iface)
{
  iface->execute           = gimp_rect_select_tool_execute;
  iface->cancel            = gimp_rect_select_tool_cancel;
  iface->rectangle_changed = gimp_rect_select_tool_rectangle_changed;
}

static void
gimp_rect_select_tool_init (GimpRectSelectTool *rect_select)
{
  GimpTool *tool = GIMP_TOOL (rect_select);

  gimp_tool_control_set_wants_click (tool->control, TRUE);
  gimp_tool_control_set_tool_cursor (tool->control,
                                     GIMP_TOOL_CURSOR_RECT_SELECT);
  gimp_tool_control_set_dirty_mask  (tool->control,
                                     GIMP_DIRTY_IMAGE_SIZE |
                                     GIMP_DIRTY_SELECTION);

  rect_select->undo = NULL;
  rect_select->redo = NULL;
}

static GObject *
gimp_rect_select_tool_constructor (GType                  type,
                                   guint                  n_params,
                                   GObjectConstructParam *params)
{
  GObject               *object;
  GimpRectSelectTool    *rect_sel;
  GimpRectSelectOptions *options;

  object = G_OBJECT_CLASS (parent_class)->constructor (type, n_params, params);

  gimp_rectangle_tool_constructor (object);

  rect_sel = GIMP_RECT_SELECT_TOOL (object);
  options  = GIMP_RECT_SELECT_TOOL_GET_OPTIONS (rect_sel);

  rect_sel->round_corners = options->round_corners;
  rect_sel->corner_radius = options->corner_radius;

  g_signal_connect_object (options, "notify::round-corners",
                           G_CALLBACK (gimp_rect_select_tool_round_corners_notify),
                           object, 0);
  g_signal_connect_object (options, "notify::corner-radius",
                           G_CALLBACK (gimp_rect_select_tool_round_corners_notify),
                           object, 0);

  gimp_rect_select_tool_update_option_defaults (GIMP_RECT_SELECT_TOOL (object),
                                                FALSE);

  return object;
}

static void
gimp_rect_select_tool_control (GimpTool       *tool,
                               GimpToolAction  action,
                               GimpDisplay    *display)
{
  gimp_rectangle_tool_control (tool, action, display);

  GIMP_TOOL_CLASS (parent_class)->control (tool, action, display);
}

static void
gimp_rect_select_tool_draw (GimpDrawTool *draw_tool)
{
  GimpRectSelectTool *rect_sel = GIMP_RECT_SELECT_TOOL (draw_tool);

  if (! gimp_rect_select_tool_selection_visible (rect_sel))
    return;

  gimp_rectangle_tool_draw (draw_tool);

  if (rect_sel->round_corners)
    {
      gint    x1, y1, x2, y2;
      gdouble radius;
      gint    square_size;

      g_object_get (rect_sel,
                    "x1", &x1,
                    "y1", &y1,
                    "x2", &x2,
                    "y2", &y2,
                    NULL);

      radius = MIN (rect_sel->corner_radius,
                    MIN ((x2 - x1) / 2.0, (y2 - y1) / 2.0));

      square_size = (int) (radius * 2);

      gimp_draw_tool_draw_arc (draw_tool, FALSE,
                               x1, y1,
                               square_size, square_size,
                               90 * 64,  90 * 64,
                               FALSE);

      gimp_draw_tool_draw_arc (draw_tool, FALSE,
                               x2 - square_size, y1,
                               square_size, square_size,
                               0,        90 * 64,
                               FALSE);

      gimp_draw_tool_draw_arc (draw_tool, FALSE,
                               x2 - square_size, y2 - square_size,
                               square_size, square_size,
                               270 * 64, 90 * 64,
                               FALSE);

      gimp_draw_tool_draw_arc (draw_tool, FALSE,
                               x1, y2 - square_size,
                               square_size, square_size,
                               180 * 64, 90 * 64,
                               FALSE);
    }
}

static void
gimp_rect_select_tool_button_press (GimpTool        *tool,
                                    GimpCoords      *coords,
                                    guint32          time,
                                    GdkModifierType  state,
                                    GimpDisplay     *display)
{
  GimpRectangleTool     *rectangle   = GIMP_RECTANGLE_TOOL (tool);
  GimpRectSelectTool    *rect_select = GIMP_RECT_SELECT_TOOL (tool);
  GimpDisplayShell      *shell       = GIMP_DISPLAY_SHELL (display->shell);
  GimpRectangleFunction  function;

  if (tool->display && display != tool->display)
    gimp_rectangle_tool_cancel (GIMP_RECTANGLE_TOOL (tool));

  function = gimp_rectangle_tool_get_function (rectangle);

  rect_select->saved_show_selection =
    gimp_display_shell_get_show_selection (shell);

  if (function == RECT_INACTIVE)
    {
      GimpDisplay *old_display = tool->display;
      gboolean     edit_started;

      tool->display = display;
      gimp_tool_control_activate (tool->control);

      edit_started = gimp_selection_tool_start_edit (GIMP_SELECTION_TOOL (tool),
                                                     coords);

      if (gimp_tool_control_is_active (tool->control))
        gimp_tool_control_halt (tool->control);

      tool->display = old_display;

      if (edit_started)
        return;
    }

  /* if the shift or ctrl keys are down, we don't want to adjust, we
   * want to create a new rectangle, regardless of pointer loc */
  if (state & (GDK_SHIFT_MASK | GDK_CONTROL_MASK))
    gimp_rectangle_tool_set_function (rectangle, RECT_CREATING);

  gimp_rectangle_tool_button_press (tool, coords, time, state, display);

  /* if we have an existing rectangle in the current display, then
   * we have already "executed", and need to undo at this point,
   * unless the user has done something in the meantime
   */
  function = gimp_rectangle_tool_get_function (rectangle);

  if (function == RECT_CREATING)
    {
      rect_select->use_saved_op = FALSE;
    }
  else
    {
      GimpSelectionOptions *options = GIMP_SELECTION_TOOL_GET_OPTIONS (tool);
      GimpImage            *image   = tool->display->image;
      GimpUndo             *undo;
      GimpChannelOps        operation;

      if (rect_select->use_saved_op)
        operation = rect_select->operation;
      else
        operation = options->operation;

      undo = gimp_undo_stack_peek (image->undo_stack);

      if (undo && rect_select->undo == undo)
        {
          /* prevent this change from halting the tool */
          gimp_tool_control_set_preserve (tool->control, TRUE);

          gimp_image_undo (image);

          gimp_tool_control_set_preserve (tool->control, FALSE);

          /* we will need to redo if the user cancels or executes */
          rect_select->redo = gimp_undo_stack_peek (image->redo_stack);
        }

      /* if the operation is "Replace", turn off the marching ants,
         because they are confusing */
      if (operation == GIMP_CHANNEL_OP_REPLACE)
        gimp_display_shell_set_show_selection (shell, FALSE);
    }

  rect_select->undo = NULL;
}

static void
gimp_rect_select_tool_button_release (GimpTool              *tool,
                                      GimpCoords            *coords,
                                      guint32                time,
                                      GdkModifierType        state,
                                      GimpButtonReleaseType  release_type,
                                      GimpDisplay           *display)
{
  GimpRectSelectTool *rect_select = GIMP_RECT_SELECT_TOOL (tool);

  gimp_tool_pop_status (tool, display);
  gimp_display_shell_set_show_selection (GIMP_DISPLAY_SHELL (display->shell),
                                         rect_select->saved_show_selection);

  /*
   * if the user has not moved the mouse, we need to redo the operation
   * that was undone on button press.
   */
  if (release_type == GIMP_BUTTON_RELEASE_CLICK)
    {
      GimpImage *image = tool->display->image;
      GimpUndo  *redo  = gimp_undo_stack_peek (image->redo_stack);

      if (redo && rect_select->redo == redo)
        {
          /* prevent this from halting the tool */
          gimp_tool_control_set_preserve (tool->control, TRUE);

          gimp_image_redo (image);
          rect_select->redo = NULL;

          gimp_tool_control_set_preserve (tool->control, FALSE);
        }
    }

  gimp_rectangle_tool_button_release (tool, coords, time, state, release_type,
                                      display);

  if (release_type == GIMP_BUTTON_RELEASE_CANCEL)
    {
      if (rect_select->redo)
        {
          /* prevent this from halting the tool */
          gimp_tool_control_set_preserve (tool->control, TRUE);

          gimp_image_redo (tool->display->image);

          gimp_tool_control_set_preserve (tool->control, FALSE);
        }

      rect_select->use_saved_op = TRUE;  /* is this correct? */
    }

  rect_select->redo = NULL;

  gimp_rect_select_tool_update_option_defaults (rect_select,
                                                FALSE);
}

static void
gimp_rect_select_tool_active_modifier_key (GimpTool        *tool,
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
gimp_rect_select_tool_key_press (GimpTool    *tool,
                                 GdkEventKey *kevent,
                                 GimpDisplay *display)
{
  return (gimp_rectangle_tool_key_press (tool, kevent, display) ||
          gimp_edit_selection_tool_key_press (tool, kevent, display));
}

static void
gimp_rect_select_tool_oper_update (GimpTool        *tool,
                                   GimpCoords      *coords,
                                   GdkModifierType  state,
                                   gboolean         proximity,
                                   GimpDisplay     *display)
{
  GimpRectangleFunction function;

  gimp_rectangle_tool_oper_update (tool, coords, state, proximity, display);

  function = gimp_rectangle_tool_get_function (GIMP_RECTANGLE_TOOL (tool));

  if (function == RECT_INACTIVE)
    GIMP_SELECTION_TOOL (tool)->allow_move = TRUE;
  else
    GIMP_SELECTION_TOOL (tool)->allow_move = FALSE;

  GIMP_TOOL_CLASS (parent_class)->oper_update (tool, coords, state, proximity,
                                               display);
}

static void
gimp_rect_select_tool_cursor_update (GimpTool        *tool,
                                     GimpCoords      *coords,
                                     GdkModifierType  state,
                                     GimpDisplay     *display)
{
  gimp_rectangle_tool_cursor_update (tool, coords, state, display);

  /* override the previous if shift or ctrl are down */
  if (state & (GDK_SHIFT_MASK | GDK_CONTROL_MASK))
    gimp_tool_control_set_cursor (tool->control, GIMP_CURSOR_CROSSHAIR_SMALL);

  GIMP_TOOL_CLASS (parent_class)->cursor_update (tool, coords, state, display);
}


static gboolean
gimp_rect_select_tool_select (GimpRectangleTool *rectangle,
                              gint               x,
                              gint               y,
                              gint               w,
                              gint               h)
{
  GimpTool             *tool        = GIMP_TOOL (rectangle);
  GimpRectSelectTool   *rect_select = GIMP_RECT_SELECT_TOOL (rectangle);
  GimpSelectionOptions *options     = GIMP_SELECTION_TOOL_GET_OPTIONS (tool);
  GimpImage            *image       = tool->display->image;
  gboolean              rectangle_exists;
  GimpChannelOps        operation;

  gimp_tool_pop_status (tool, tool->display);

  rectangle_exists = (x <= image->width && y <= image->height &&
                      x + w >= 0 && y + h >= 0 &&
                      w > 0 && h > 0);

  if (rect_select->use_saved_op)
    operation = rect_select->operation;
  else
    operation = options->operation;

  /* if rectangle exists, turn it into a selection */
  if (rectangle_exists)
    GIMP_RECT_SELECT_TOOL_GET_CLASS (rect_select)->select (rect_select,
                                                           operation,
                                                           x, y, w, h);

  return rectangle_exists;
}

static void
gimp_rect_select_tool_real_select (GimpRectSelectTool *rect_select,
                                   GimpChannelOps      operation,
                                   gint                x,
                                   gint                y,
                                   gint                w,
                                   gint                h)
{
  GimpTool              *tool    = GIMP_TOOL (rect_select);
  GimpSelectionOptions  *options = GIMP_SELECTION_TOOL_GET_OPTIONS (tool);
  GimpRectSelectOptions *rect_select_options;
  GimpChannel           *channel;

  rect_select_options = GIMP_RECT_SELECT_TOOL_GET_OPTIONS (tool);

  channel = gimp_image_get_mask (tool->display->image);

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

static gboolean
gimp_rect_select_tool_selection_visible (GimpRectSelectTool *rect_select_tool)
{
  GimpTool         *tool  = GIMP_TOOL (rect_select_tool);
  GimpDisplayShell *shell = GIMP_DISPLAY_SHELL (tool->display->shell);

  /* Don't draw the rectangle if `Show selection' is off. */
  if (gimp_tool_control_is_active (tool->control))
    {
      return rect_select_tool->saved_show_selection;
    }
  else
    {
      return gimp_display_shell_get_show_selection (shell);
    }
}

/**
 * gimp_rect_select_tool_update_option_defaults:
 * @crop_tool:
 * @ignore_pending: %TRUE to ignore any pending crop rectangle.
 *
 * Sets the default Fixed: Aspect ratio and Fixed: Size option
 * properties.
 */
static void
gimp_rect_select_tool_update_option_defaults (GimpRectSelectTool *rect_select_tool,
                                              gboolean            ignore_pending)
{
  GimpTool             *tool;
  GimpRectangleTool    *rectangle_tool;
  GimpRectangleOptions *rectangle_options;

  tool              = GIMP_TOOL (rect_select_tool);
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
                                            "default-fixed-size-width",
                                            "default-fixed-size-height");
    }
  else
    {
      /* There is no pending rectangle, set default Fixed: Size to
       * 100x100.
       */

      g_object_set (G_OBJECT (rectangle_options),
                    "default-fixed-size-width",  100.0,
                    "default-fixed-size-height", 100.0,
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
gimp_rect_select_tool_execute (GimpRectangleTool *rectangle,
                               gint               x,
                               gint               y,
                               gint               w,
                               gint               h)
{
  if (w == 0 && h == 0)
    {
      GimpImage   *image     = GIMP_TOOL (rectangle)->display->image;
      GimpChannel *selection = gimp_image_get_mask (image);
      gint         pressx;
      gint         pressy;

      if (gimp_image_floating_sel (image))
        {
          floating_sel_anchor (gimp_image_floating_sel (image));
          gimp_image_flush (image);
          return TRUE;
        }

      gimp_rectangle_tool_get_press_coords (rectangle, &pressx, &pressy);

      /*  if the click was inside the marching ants  */
      if (gimp_pickable_get_opacity_at (GIMP_PICKABLE (selection),
                                        pressx, pressy) > BOUNDARY_HALF_WAY)
        {
          gint x1, y1, x2, y2;

          if (gimp_channel_bounds (selection, &x1, &y1, &x2, &y2))
            {
              g_object_set (rectangle,
                            "x1", x1,
                            "y1", y1,
                            "x2", x2,
                            "y2", y2,
                            NULL);
            }

          gimp_rectangle_tool_set_function (rectangle, RECT_MOVING);

          return FALSE;
        }
      else
        {
          GimpTool *tool = GIMP_TOOL (rectangle);

          /* prevent this change from halting the tool */
          gimp_tool_control_set_preserve (tool->control, TRUE);

          /* otherwise clear the selection */
          gimp_channel_clear (selection, NULL, TRUE);
          gimp_image_flush (image);

          gimp_tool_control_set_preserve (tool->control, FALSE);
        }
    }

  gimp_rect_select_tool_update_option_defaults (GIMP_RECT_SELECT_TOOL (rectangle),
                                                FALSE);

  return TRUE;
}

static void
gimp_rect_select_tool_cancel (GimpRectangleTool *rectangle)
{
  GimpTool           *tool        = GIMP_TOOL (rectangle);
  GimpRectSelectTool *rect_select = GIMP_RECT_SELECT_TOOL (rectangle);

  if (tool->display)
    {
      GimpImage *image = tool->display->image;
      GimpUndo  *undo;

      /* if we have an existing rectangle in the current display, then
       * we have already "executed", and need to undo at this point,
       * unless the user has done something in the meantime
       */
      undo  = gimp_undo_stack_peek (image->undo_stack);

      if (undo && rect_select->undo == undo)
        {
          /* prevent this change from halting the tool */
          gimp_tool_control_set_preserve (tool->control, TRUE);

          gimp_image_undo (image);
          gimp_image_flush (image);

          gimp_tool_control_set_preserve (tool->control, FALSE);
        }
    }

  rect_select->undo = NULL;
  rect_select->redo = NULL;
}

static gboolean
gimp_rect_select_tool_rectangle_changed (GimpRectangleTool *rectangle)
{
  GimpTool *tool = GIMP_TOOL (rectangle);

  /* prevent change in selection from halting the tool */
  gimp_tool_control_set_preserve (tool->control, TRUE);

  if (tool->display && ! gimp_tool_control_is_active (tool->control))
    {
      GimpRectSelectTool *rect_select = GIMP_RECT_SELECT_TOOL (tool);
      GimpImage          *image       = tool->display->image;
      GimpUndo           *undo;
      gint                x1, y1, x2, y2;

      /* if we got here via button release, we have already undone the
       * previous operation.  But if we got here by some other means,
       * we need to undo it now.
       */
      undo = gimp_undo_stack_peek (image->undo_stack);

      if (undo && rect_select->undo == undo)
        {
          gimp_image_undo (image);
          rect_select->undo = NULL;
        }

      g_object_get (rectangle,
                    "x1", &x1,
                    "y1", &y1,
                    "x2", &x2,
                    "y2", &y2,
                    NULL);

      if (gimp_rect_select_tool_select (rectangle, x1, y1, x2 - x1, y2 - y1))
        {
          /* save the undo that we got when executing, but only if
           * we actually selected something
           */
          rect_select->undo = gimp_undo_stack_peek (image->undo_stack);
          rect_select->redo = NULL;
        }

      if (! rect_select->use_saved_op)
        {
          GimpSelectionOptions *options;

          options = GIMP_SELECTION_TOOL_GET_OPTIONS (tool);

          /* remember the operation now in case we modify the rectangle */
          rect_select->operation    = options->operation;
          rect_select->use_saved_op = TRUE;
        }

      gimp_image_flush (image);
    }

  gimp_tool_control_set_preserve (tool->control, FALSE);

  return TRUE;
}

static void
gimp_rect_select_tool_round_corners_notify (GimpRectSelectOptions *options,
                                            GParamSpec            *pspec,
                                            GimpRectSelectTool    *rect_sel)
{
  gimp_draw_tool_pause (GIMP_DRAW_TOOL (rect_sel));

  rect_sel->round_corners = options->round_corners;
  rect_sel->corner_radius = options->corner_radius;

  gimp_draw_tool_resume (GIMP_DRAW_TOOL (rect_sel));
}
