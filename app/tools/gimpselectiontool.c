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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <gegl.h>
#include <gtk/gtk.h>

#include "libgimpwidgets/gimpwidgets.h"

#include "tools-types.h"

#include "core/gimpchannel.h"
#include "core/gimperror.h"
#include "core/gimpimage.h"
#include "core/gimpimage-pick-item.h"
#include "core/gimpimage-undo.h"
#include "core/gimppickable.h"
#include "core/gimpundostack.h"

#include "display/gimpdisplay.h"
#include "display/gimpdisplayshell-appearance.h"

#include "widgets/gimpwidgets-utils.h"

#include "gimpeditselectiontool.h"
#include "gimpselectiontool.h"
#include "gimpselectionoptions.h"
#include "gimptoolcontrol.h"
#include "gimptools-utils.h"

#include "gimp-intl.h"


static void       gimp_selection_tool_control             (GimpTool           *tool,
                                                           GimpToolAction      action,
                                                           GimpDisplay        *display);
static void       gimp_selection_tool_modifier_key        (GimpTool           *tool,
                                                           GdkModifierType     key,
                                                           gboolean            press,
                                                           GdkModifierType     state,
                                                           GimpDisplay        *display);
static void       gimp_selection_tool_oper_update         (GimpTool           *tool,
                                                           const GimpCoords   *coords,
                                                           GdkModifierType     state,
                                                           gboolean            proximity,
                                                           GimpDisplay        *display);
static void       gimp_selection_tool_cursor_update       (GimpTool           *tool,
                                                           const GimpCoords   *coords,
                                                           GdkModifierType     state,
                                                           GimpDisplay        *display);

static gboolean   gimp_selection_tool_real_have_selection (GimpSelectionTool  *sel_tool,
                                                           GimpDisplay        *display);

static void       gimp_selection_tool_commit              (GimpSelectionTool  *sel_tool);
static void       gimp_selection_tool_halt                (GimpSelectionTool  *sel_tool,
                                                           GimpDisplay        *display);

static gboolean   gimp_selection_tool_check               (GimpSelectionTool  *sel_tool,
                                                           GimpDisplay        *display,
                                                           GError            **error);

static gboolean   gimp_selection_tool_have_selection      (GimpSelectionTool  *sel_tool,
                                                           GimpDisplay        *display);


G_DEFINE_TYPE (GimpSelectionTool, gimp_selection_tool, GIMP_TYPE_DRAW_TOOL)

#define parent_class gimp_selection_tool_parent_class


static void
gimp_selection_tool_class_init (GimpSelectionToolClass *klass)
{
  GimpToolClass *tool_class = GIMP_TOOL_CLASS (klass);

  tool_class->control        = gimp_selection_tool_control;
  tool_class->modifier_key   = gimp_selection_tool_modifier_key;
  tool_class->key_press      = gimp_edit_selection_tool_key_press;
  tool_class->oper_update    = gimp_selection_tool_oper_update;
  tool_class->cursor_update  = gimp_selection_tool_cursor_update;
  tool_class->is_destructive = FALSE;

  klass->have_selection      = gimp_selection_tool_real_have_selection;
}

static void
gimp_selection_tool_init (GimpSelectionTool *selection_tool)
{
  selection_tool->function             = SELECTION_SELECT;
  selection_tool->saved_operation      = GIMP_CHANNEL_OP_REPLACE;

  selection_tool->saved_show_selection = FALSE;
  selection_tool->undo                 = NULL;
  selection_tool->redo                 = NULL;
  selection_tool->idle_id              = 0;

  selection_tool->allow_move           = TRUE;
}

static void
gimp_selection_tool_control (GimpTool       *tool,
                             GimpToolAction  action,
                             GimpDisplay    *display)
{
  GimpSelectionTool *selection_tool = GIMP_SELECTION_TOOL (tool);

  switch (action)
    {
    case GIMP_TOOL_ACTION_PAUSE:
    case GIMP_TOOL_ACTION_RESUME:
      break;

    case GIMP_TOOL_ACTION_HALT:
      gimp_selection_tool_halt (selection_tool, display);
      break;

    case GIMP_TOOL_ACTION_COMMIT:
      gimp_selection_tool_commit (selection_tool);
      break;
    }

  GIMP_TOOL_CLASS (parent_class)->control (tool, action, display);
}

static void
gimp_selection_tool_modifier_key (GimpTool        *tool,
                                  GdkModifierType  key,
                                  gboolean         press,
                                  GdkModifierType  state,
                                  GimpDisplay     *display)
{
  GimpSelectionTool    *selection_tool = GIMP_SELECTION_TOOL (tool);
  GimpSelectionOptions *options        = GIMP_SELECTION_TOOL_GET_OPTIONS (tool);
  GdkModifierType       extend_mask;
  GdkModifierType       modify_mask;

  extend_mask = gimp_get_extend_selection_mask ();
  modify_mask = gimp_get_modify_selection_mask ();

  if (key == extend_mask ||
      key == modify_mask ||
      key == GDK_MOD1_MASK)
    {
      GimpChannelOps button_op = options->operation;

      state &= extend_mask |
               modify_mask |
               GDK_MOD1_MASK;

      if (press)
        {
          if (key == state ||
              /* GimpPolygonSelectTool may mask-out part of the state, which
               * can cause the wrong mode to be restored on release if we don't
               * init saved_operation here.
               *
               * see issue #4992.
               */
              ! state)
            {
              /*  first modifier pressed  */

              selection_tool->saved_operation = options->operation;
            }
        }
      else
        {
          if (! state)
            {
              /*  last modifier released  */

              button_op = selection_tool->saved_operation;
            }
        }

      if (state & GDK_MOD1_MASK)
        {
          /*  if alt is down, pretend that neither
           *  shift nor control are down
           */
          button_op = selection_tool->saved_operation;
        }
      else if (state & (extend_mask |
                        modify_mask))
        {
          /*  else get the operation from the modifier state, but only
           *  if there is actually a modifier pressed, so we don't
           *  override the "last modifier released" assignment above
           */
          button_op = gimp_modifiers_to_channel_op (state);
        }

      if (button_op != options->operation)
        {
          g_object_set (options, "operation", button_op, NULL);
        }
    }
}

static void
gimp_selection_tool_oper_update (GimpTool         *tool,
                                 const GimpCoords *coords,
                                 GdkModifierType   state,
                                 gboolean          proximity,
                                 GimpDisplay      *display)
{
  GimpSelectionTool    *selection_tool = GIMP_SELECTION_TOOL (tool);
  GimpSelectionOptions *options        = GIMP_SELECTION_TOOL_GET_OPTIONS (tool);
  GimpImage            *image;
  GList                *drawables;
  GimpLayer            *layer;
  GimpLayer            *floating_sel;
  GdkModifierType       extend_mask;
  GdkModifierType       modify_mask;
  gboolean              have_selection;
  gboolean              move_layer        = FALSE;
  gboolean              move_floating_sel = FALSE;

  image        = gimp_display_get_image (display);
  drawables    = gimp_image_get_selected_drawables (image);
  layer        = gimp_image_pick_layer (image, coords->x, coords->y, NULL);
  floating_sel = gimp_image_get_floating_selection (image);

  extend_mask = gimp_get_extend_selection_mask ();
  modify_mask = gimp_get_modify_selection_mask ();

  have_selection = gimp_selection_tool_have_selection (selection_tool, display);

  if (drawables)
    {
      if (floating_sel)
        {
          if (layer == floating_sel)
            move_floating_sel = TRUE;
        }
      else if (have_selection)
        {
          GList *iter;

          for (iter = drawables; iter; iter = iter->next)
            {
              if (gimp_item_mask_intersect (GIMP_ITEM (iter->data),
                                            NULL, NULL, NULL, NULL))
                {
                  move_layer = TRUE;
                  break;
                }
            }
        }

      g_list_free (drawables);
    }

  selection_tool->function = SELECTION_SELECT;

  if (selection_tool->allow_move &&
      (state & GDK_MOD1_MASK) && (state & modify_mask) && move_layer)
    {
      /* move the selection */
      selection_tool->function = SELECTION_MOVE;
    }
  else if (selection_tool->allow_move &&
           (state & GDK_MOD1_MASK) && (state & extend_mask) && move_layer)
    {
      /* move a copy of the selection */
      selection_tool->function = SELECTION_MOVE_COPY;
    }
  else if (selection_tool->allow_move &&
           (state & GDK_MOD1_MASK) && have_selection)
    {
      /* move the selection mask */
      selection_tool->function = SELECTION_MOVE_MASK;
    }
  else if (selection_tool->allow_move &&
           ! (state & (extend_mask | modify_mask)) &&
           move_floating_sel)
    {
      /* move the selection */
      selection_tool->function = SELECTION_MOVE;
    }
  else if ((state & modify_mask) || (state & extend_mask))
    {
      /* select */
      selection_tool->function = SELECTION_SELECT;
    }
  else if (floating_sel)
    {
      /* anchor the selection */
      selection_tool->function = SELECTION_ANCHOR;
    }

  gimp_tool_pop_status (tool, display);

  if (proximity)
    {
      const gchar     *status      = NULL;
      gboolean         free_status = FALSE;
      GdkModifierType  modifiers   = (extend_mask | modify_mask);

      if (have_selection)
        modifiers |= GDK_MOD1_MASK;

      switch (selection_tool->function)
        {
        case SELECTION_SELECT:
          switch (options->operation)
            {
            case GIMP_CHANNEL_OP_REPLACE:
              if (have_selection)
                {
                  status = gimp_suggest_modifiers (_("Click-Drag to replace the "
                                                     "current selection"),
                                                   modifiers & ~state,
                                                   NULL, NULL, NULL);
                  free_status = TRUE;
                }
              else
                {
                  status = _("Click-Drag to create a new selection");
                }
              break;

            case GIMP_CHANNEL_OP_ADD:
              status = gimp_suggest_modifiers (_("Click-Drag to add to the "
                                                 "current selection"),
                                               modifiers
                                               & ~(state | extend_mask),
                                               NULL, NULL, NULL);
              free_status = TRUE;
              break;

            case GIMP_CHANNEL_OP_SUBTRACT:
              status = gimp_suggest_modifiers (_("Click-Drag to subtract from the "
                                                 "current selection"),
                                               modifiers
                                               & ~(state | modify_mask),
                                               NULL, NULL, NULL);
              free_status = TRUE;
              break;

            case GIMP_CHANNEL_OP_INTERSECT:
              status = gimp_suggest_modifiers (_("Click-Drag to intersect with "
                                                 "the current selection"),
                                               modifiers & ~state,
                                               NULL, NULL, NULL);
              free_status = TRUE;
              break;
            }
          break;

        case SELECTION_MOVE_MASK:
          status = gimp_suggest_modifiers (_("Click-Drag to move the "
                                             "selection mask"),
                                           modifiers & ~state,
                                           NULL, NULL, NULL);
          free_status = TRUE;
          break;

        case SELECTION_MOVE:
          status = _("Click-Drag to move the selected pixels");
          break;

        case SELECTION_MOVE_COPY:
          status = _("Click-Drag to move a copy of the selected pixels");
          break;

        case SELECTION_ANCHOR:
          status = _("Click to anchor the floating selection");
          break;

        default:
          g_return_if_reached ();
        }

      if (status)
        gimp_tool_push_status (tool, display, "%s", status);

      if (free_status)
        g_free ((gchar *) status);
    }
}

static void
gimp_selection_tool_cursor_update (GimpTool         *tool,
                                   const GimpCoords *coords,
                                   GdkModifierType   state,
                                   GimpDisplay      *display)
{
  GimpSelectionTool    *selection_tool = GIMP_SELECTION_TOOL (tool);
  GimpSelectionOptions *options;
  GimpToolCursorType    tool_cursor;
  GimpCursorModifier    modifier;

  options = GIMP_SELECTION_TOOL_GET_OPTIONS (tool);

  tool_cursor = gimp_tool_control_get_tool_cursor (tool->control);
  modifier    = GIMP_CURSOR_MODIFIER_NONE;

  switch (selection_tool->function)
    {
    case SELECTION_SELECT:
      switch (options->operation)
        {
        case GIMP_CHANNEL_OP_REPLACE:
          break;
        case GIMP_CHANNEL_OP_ADD:
          modifier = GIMP_CURSOR_MODIFIER_PLUS;
          break;
        case GIMP_CHANNEL_OP_SUBTRACT:
          modifier = GIMP_CURSOR_MODIFIER_MINUS;
          break;
        case GIMP_CHANNEL_OP_INTERSECT:
          modifier = GIMP_CURSOR_MODIFIER_INTERSECT;
          break;
        }
      break;

    case SELECTION_MOVE_MASK:
      modifier = GIMP_CURSOR_MODIFIER_MOVE;
      break;

    case SELECTION_MOVE:
    case SELECTION_MOVE_COPY:
      tool_cursor = GIMP_TOOL_CURSOR_MOVE;
      break;

    case SELECTION_ANCHOR:
      modifier = GIMP_CURSOR_MODIFIER_ANCHOR;
      break;
    }

  /*  our subclass might have set a BAD modifier, in which case we leave it
   *  there, since it's more important than what we have to say.
   */
  if (gimp_tool_control_get_cursor_modifier (tool->control) ==
      GIMP_CURSOR_MODIFIER_BAD                              ||
      ! gimp_selection_tool_check (selection_tool, display, NULL))
    {
      modifier = GIMP_CURSOR_MODIFIER_BAD;
    }

  gimp_tool_set_cursor (tool, display,
                        gimp_tool_control_get_cursor (tool->control),
                        tool_cursor,
                        modifier);
}

static gboolean
gimp_selection_tool_real_have_selection (GimpSelectionTool *sel_tool,
                                         GimpDisplay       *display)
{
  GimpImage   *image     = gimp_display_get_image (display);
  GimpChannel *selection = gimp_image_get_mask (image);

  return ! gimp_channel_is_empty (selection);
}

static void
gimp_selection_tool_commit (GimpSelectionTool *sel_tool)
{
  /* make sure gimp_selection_tool_halt() doesn't undo the change, if any */
  g_clear_weak_pointer (&sel_tool->undo);
}

static void
gimp_selection_tool_halt (GimpSelectionTool *sel_tool,
                          GimpDisplay       *display)
{
  g_warn_if_fail (sel_tool->change_count == 0);

  if (display)
    {
      GimpTool      *tool       = GIMP_TOOL (sel_tool);
      GimpImage     *image      = gimp_display_get_image (display);
      GimpUndoStack *undo_stack = gimp_image_get_undo_stack (image);
      GimpUndo      *undo       = gimp_undo_stack_peek (undo_stack);

      /* if we have an existing selection in the current display, then
       * we have already "executed", and need to undo at this point,
       * unless the user has done something in the meantime
       */
      if (undo && sel_tool->undo == undo)
        {
          /* prevent this change from halting the tool */
          gimp_tool_control_push_preserve (tool->control, TRUE);

          gimp_image_undo (image);
          gimp_image_flush (image);

          gimp_tool_control_pop_preserve (tool->control);
        }

      /* reset the automatic undo/redo mechanism */
      g_clear_weak_pointer (&sel_tool->undo);
      g_clear_weak_pointer (&sel_tool->redo);
    }
}

static gboolean
gimp_selection_tool_check (GimpSelectionTool  *sel_tool,
                           GimpDisplay        *display,
                           GError            **error)
{
  GimpSelectionOptions *options   = GIMP_SELECTION_TOOL_GET_OPTIONS (sel_tool);
  GimpImage            *image     = gimp_display_get_image (display);

  switch (sel_tool->function)
    {
    case SELECTION_SELECT:
      switch (options->operation)
        {
        case GIMP_CHANNEL_OP_ADD:
        case GIMP_CHANNEL_OP_REPLACE:
          break;

        case GIMP_CHANNEL_OP_SUBTRACT:
          if (! gimp_item_bounds (GIMP_ITEM (gimp_image_get_mask (image)),
                                  NULL, NULL, NULL, NULL))
            {
              g_set_error (error, GIMP_ERROR, GIMP_FAILED,
                           _("Cannot subtract from an empty selection."));

              return FALSE;
            }
          break;

        case GIMP_CHANNEL_OP_INTERSECT:
          if (! gimp_item_bounds (GIMP_ITEM (gimp_image_get_mask (image)),
                                  NULL, NULL, NULL, NULL))
            {
              g_set_error (error, GIMP_ERROR, GIMP_FAILED,
                           _("Cannot intersect with an empty selection."));

              return FALSE;
            }
          break;
        }
      break;

    case SELECTION_MOVE:
    case SELECTION_MOVE_COPY:
        {
          GList    *drawables   = gimp_image_get_selected_drawables (image);
          GimpItem *locked_item = NULL;
          GList    *iter;

          for (iter = drawables; iter; iter = iter->next)
            {
              if (gimp_viewable_get_children (iter->data))
                {
                  g_set_error (error, GIMP_ERROR, GIMP_FAILED,
                               _("Cannot modify the pixels of layer groups."));

                  g_list_free (drawables);
                  return FALSE;
                }
              else if (gimp_item_is_content_locked (iter->data, &locked_item))
                {
                  g_set_error (error, GIMP_ERROR, GIMP_FAILED,
                               _("A selected item's pixels are locked."));

                  if (error)
                    gimp_tools_blink_lock_box (display->gimp, locked_item);

                  g_list_free (drawables);
                  return FALSE;
                }
            }

          g_list_free (drawables);
        }
      break;

    default:
      break;
    }

  return TRUE;
}

static gboolean
gimp_selection_tool_have_selection (GimpSelectionTool *sel_tool,
                                    GimpDisplay       *display)
{
  return GIMP_SELECTION_TOOL_GET_CLASS (sel_tool)->have_selection (sel_tool,
                                                                   display);
}


/*  public functions  */

gboolean
gimp_selection_tool_start_edit (GimpSelectionTool *sel_tool,
                                GimpDisplay       *display,
                                const GimpCoords  *coords)
{
  GimpTool             *tool;
  GimpSelectionOptions *options;
  GError               *error = NULL;

  g_return_val_if_fail (GIMP_IS_SELECTION_TOOL (sel_tool), FALSE);
  g_return_val_if_fail (GIMP_IS_DISPLAY (display), FALSE);
  g_return_val_if_fail (coords != NULL, FALSE);

  tool    = GIMP_TOOL (sel_tool);
  options = GIMP_SELECTION_TOOL_GET_OPTIONS (sel_tool);

  g_return_val_if_fail (gimp_tool_control_is_active (tool->control) == FALSE,
                        FALSE);

  if (! gimp_selection_tool_check (sel_tool, display, &error))
    {
      gimp_tool_message_literal (tool, display, error->message);

      gimp_tools_show_tool_options (display->gimp);
      gimp_widget_blink (options->mode_box);

      g_clear_error (&error);

      return TRUE;
    }

  switch (sel_tool->function)
    {
    case SELECTION_MOVE_MASK:
      gimp_edit_selection_tool_start (tool, display, coords,
                                      GIMP_TRANSLATE_MODE_MASK, FALSE);
      return TRUE;

    case SELECTION_MOVE:
    case SELECTION_MOVE_COPY:
      {
        GimpTranslateMode edit_mode;

        gimp_tool_control (tool, GIMP_TOOL_ACTION_COMMIT, display);

        if (sel_tool->function == SELECTION_MOVE)
          edit_mode = GIMP_TRANSLATE_MODE_MASK_TO_LAYER;
        else
          edit_mode = GIMP_TRANSLATE_MODE_MASK_COPY_TO_LAYER;

        gimp_edit_selection_tool_start (tool, display, coords,
                                        edit_mode, FALSE);

        return TRUE;
      }

    default:
      break;
    }

  return FALSE;
}

static gboolean
gimp_selection_tool_idle (GimpSelectionTool *sel_tool)
{
  GimpTool         *tool  = GIMP_TOOL (sel_tool);
  GimpDisplayShell *shell = gimp_display_get_shell (tool->display);

  gimp_display_shell_set_show_selection (shell, FALSE);

  sel_tool->idle_id = 0;

  return G_SOURCE_REMOVE;
}

void
gimp_selection_tool_start_change (GimpSelectionTool *sel_tool,
                                  gboolean           create,
                                  GimpChannelOps     operation)
{
  GimpTool         *tool;
  GimpDisplayShell *shell;
  GimpImage        *image;
  GimpUndoStack    *undo_stack;

  g_return_if_fail (GIMP_IS_SELECTION_TOOL (sel_tool));

  tool = GIMP_TOOL (sel_tool);

  g_return_if_fail (tool->display != NULL);

  if (sel_tool->change_count++ > 0)
    return;

  shell      = gimp_display_get_shell (tool->display);
  image      = gimp_display_get_image (tool->display);
  undo_stack = gimp_image_get_undo_stack (image);

  sel_tool->saved_show_selection =
    gimp_display_shell_get_show_selection (shell);

  if (create)
    {
      g_clear_weak_pointer (&sel_tool->undo);
    }
  else
    {
      GimpUndoStack *redo_stack = gimp_image_get_redo_stack (image);
      GimpUndo      *undo;

      undo = gimp_undo_stack_peek (undo_stack);

      if (undo && undo == sel_tool->undo)
        {
          /* prevent this change from halting the tool */
          gimp_tool_control_push_preserve (tool->control, TRUE);

          gimp_image_undo (image);

          gimp_tool_control_pop_preserve (tool->control);

          g_clear_weak_pointer (&sel_tool->undo);

          /* we will need to redo if the user cancels or executes */
          g_set_weak_pointer (&sel_tool->redo,
                              gimp_undo_stack_peek (redo_stack));
        }

      /* if the operation is "Replace", turn off the marching ants,
       * because they are confusing ...
       */
      if (operation == GIMP_CHANNEL_OP_REPLACE)
        {
          /* ... however, do this in an idle function, to avoid unnecessarily
           * restarting the selection if we don't visit the main loop between
           * the start_change() and end_change() calls.
           */
          sel_tool->idle_id = g_idle_add_full (
            G_PRIORITY_HIGH_IDLE,
            (GSourceFunc) gimp_selection_tool_idle,
            sel_tool, NULL);
        }
    }

  g_set_weak_pointer (&sel_tool->undo,
                      gimp_undo_stack_peek (undo_stack));
}

void
gimp_selection_tool_end_change (GimpSelectionTool *sel_tool,
                                gboolean           cancel)
{
  GimpTool         *tool;
  GimpDisplayShell *shell;
  GimpImage        *image;
  GimpUndoStack    *undo_stack;

  g_return_if_fail (GIMP_IS_SELECTION_TOOL (sel_tool));
  g_return_if_fail (sel_tool->change_count > 0);

  tool = GIMP_TOOL (sel_tool);

  g_return_if_fail (tool->display != NULL);

  if (--sel_tool->change_count > 0)
    return;

  shell      = gimp_display_get_shell (tool->display);
  image      = gimp_display_get_image (tool->display);
  undo_stack = gimp_image_get_undo_stack (image);

  if (cancel)
    {
      GimpUndoStack *redo_stack = gimp_image_get_redo_stack (image);
      GimpUndo      *redo       = gimp_undo_stack_peek (redo_stack);

      if (redo && redo == sel_tool->redo)
        {
          /* prevent this from halting the tool */
          gimp_tool_control_push_preserve (tool->control, TRUE);

          gimp_image_redo (image);

          gimp_tool_control_pop_preserve (tool->control);

          g_set_weak_pointer (&sel_tool->undo,
                              gimp_undo_stack_peek (undo_stack));
        }
      else
        {
          g_clear_weak_pointer (&sel_tool->undo);
        }
    }
  else
    {
      GimpUndo *undo = gimp_undo_stack_peek (undo_stack);

      /* save the undo that we got when executing, but only if
       * we actually selected something
       */
      if (undo && undo != sel_tool->undo)
        g_set_weak_pointer (&sel_tool->undo, undo);
      else
        g_clear_weak_pointer (&sel_tool->undo);
    }

  g_clear_weak_pointer (&sel_tool->redo);

  if (sel_tool->idle_id)
    {
      g_source_remove (sel_tool->idle_id);
      sel_tool->idle_id = 0;
    }
  else
    {
      gimp_display_shell_set_show_selection (shell,
                                             sel_tool->saved_show_selection);
    }

  gimp_image_flush (image);
}
