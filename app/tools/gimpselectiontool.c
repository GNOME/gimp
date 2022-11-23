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

#include "libligmawidgets/ligmawidgets.h"

#include "tools-types.h"

#include "core/ligmachannel.h"
#include "core/ligmaerror.h"
#include "core/ligmaimage.h"
#include "core/ligmaimage-pick-item.h"
#include "core/ligmaimage-undo.h"
#include "core/ligmapickable.h"
#include "core/ligmaundostack.h"

#include "display/ligmadisplay.h"
#include "display/ligmadisplayshell-appearance.h"

#include "widgets/ligmawidgets-utils.h"

#include "ligmaeditselectiontool.h"
#include "ligmaselectiontool.h"
#include "ligmaselectionoptions.h"
#include "ligmatoolcontrol.h"
#include "ligmatools-utils.h"

#include "ligma-intl.h"


static void       ligma_selection_tool_control             (LigmaTool           *tool,
                                                           LigmaToolAction      action,
                                                           LigmaDisplay        *display);
static void       ligma_selection_tool_modifier_key        (LigmaTool           *tool,
                                                           GdkModifierType     key,
                                                           gboolean            press,
                                                           GdkModifierType     state,
                                                           LigmaDisplay        *display);
static void       ligma_selection_tool_oper_update         (LigmaTool           *tool,
                                                           const LigmaCoords   *coords,
                                                           GdkModifierType     state,
                                                           gboolean            proximity,
                                                           LigmaDisplay        *display);
static void       ligma_selection_tool_cursor_update       (LigmaTool           *tool,
                                                           const LigmaCoords   *coords,
                                                           GdkModifierType     state,
                                                           LigmaDisplay        *display);

static gboolean   ligma_selection_tool_real_have_selection (LigmaSelectionTool  *sel_tool,
                                                           LigmaDisplay        *display);

static void       ligma_selection_tool_commit              (LigmaSelectionTool  *sel_tool);
static void       ligma_selection_tool_halt                (LigmaSelectionTool  *sel_tool,
                                                           LigmaDisplay        *display);

static gboolean   ligma_selection_tool_check               (LigmaSelectionTool  *sel_tool,
                                                           LigmaDisplay        *display,
                                                           GError            **error);

static gboolean   ligma_selection_tool_have_selection      (LigmaSelectionTool  *sel_tool,
                                                           LigmaDisplay        *display);

static void       ligma_selection_tool_set_undo_ptr        (LigmaUndo          **undo_ptr,
                                                           LigmaUndo           *undo);


G_DEFINE_TYPE (LigmaSelectionTool, ligma_selection_tool, LIGMA_TYPE_DRAW_TOOL)

#define parent_class ligma_selection_tool_parent_class


static void
ligma_selection_tool_class_init (LigmaSelectionToolClass *klass)
{
  LigmaToolClass *tool_class = LIGMA_TOOL_CLASS (klass);

  tool_class->control       = ligma_selection_tool_control;
  tool_class->modifier_key  = ligma_selection_tool_modifier_key;
  tool_class->key_press     = ligma_edit_selection_tool_key_press;
  tool_class->oper_update   = ligma_selection_tool_oper_update;
  tool_class->cursor_update = ligma_selection_tool_cursor_update;

  klass->have_selection     = ligma_selection_tool_real_have_selection;
}

static void
ligma_selection_tool_init (LigmaSelectionTool *selection_tool)
{
  selection_tool->function             = SELECTION_SELECT;
  selection_tool->saved_operation      = LIGMA_CHANNEL_OP_REPLACE;

  selection_tool->saved_show_selection = FALSE;
  selection_tool->undo                 = NULL;
  selection_tool->redo                 = NULL;
  selection_tool->idle_id              = 0;

  selection_tool->allow_move           = TRUE;
}

static void
ligma_selection_tool_control (LigmaTool       *tool,
                             LigmaToolAction  action,
                             LigmaDisplay    *display)
{
  LigmaSelectionTool *selection_tool = LIGMA_SELECTION_TOOL (tool);

  switch (action)
    {
    case LIGMA_TOOL_ACTION_PAUSE:
    case LIGMA_TOOL_ACTION_RESUME:
      break;

    case LIGMA_TOOL_ACTION_HALT:
      ligma_selection_tool_halt (selection_tool, display);
      break;

    case LIGMA_TOOL_ACTION_COMMIT:
      ligma_selection_tool_commit (selection_tool);
      break;
    }

  LIGMA_TOOL_CLASS (parent_class)->control (tool, action, display);
}

static void
ligma_selection_tool_modifier_key (LigmaTool        *tool,
                                  GdkModifierType  key,
                                  gboolean         press,
                                  GdkModifierType  state,
                                  LigmaDisplay     *display)
{
  LigmaSelectionTool    *selection_tool = LIGMA_SELECTION_TOOL (tool);
  LigmaSelectionOptions *options        = LIGMA_SELECTION_TOOL_GET_OPTIONS (tool);
  GdkModifierType       extend_mask;
  GdkModifierType       modify_mask;

  extend_mask = ligma_get_extend_selection_mask ();
  modify_mask = ligma_get_modify_selection_mask ();

  if (key == extend_mask ||
      key == modify_mask ||
      key == GDK_MOD1_MASK)
    {
      LigmaChannelOps button_op = options->operation;

      state &= extend_mask |
               modify_mask |
               GDK_MOD1_MASK;

      if (press)
        {
          if (key == state ||
              /* LigmaPolygonSelectTool may mask-out part of the state, which
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
          button_op = ligma_modifiers_to_channel_op (state);
        }

      if (button_op != options->operation)
        {
          g_object_set (options, "operation", button_op, NULL);
        }
    }
}

static void
ligma_selection_tool_oper_update (LigmaTool         *tool,
                                 const LigmaCoords *coords,
                                 GdkModifierType   state,
                                 gboolean          proximity,
                                 LigmaDisplay      *display)
{
  LigmaSelectionTool    *selection_tool = LIGMA_SELECTION_TOOL (tool);
  LigmaSelectionOptions *options        = LIGMA_SELECTION_TOOL_GET_OPTIONS (tool);
  LigmaImage            *image;
  GList                *drawables;
  LigmaLayer            *layer;
  LigmaLayer            *floating_sel;
  GdkModifierType       extend_mask;
  GdkModifierType       modify_mask;
  gboolean              have_selection;
  gboolean              move_layer        = FALSE;
  gboolean              move_floating_sel = FALSE;

  image        = ligma_display_get_image (display);
  drawables    = ligma_image_get_selected_drawables (image);
  layer        = ligma_image_pick_layer (image, coords->x, coords->y, NULL);
  floating_sel = ligma_image_get_floating_selection (image);

  extend_mask = ligma_get_extend_selection_mask ();
  modify_mask = ligma_get_modify_selection_mask ();

  have_selection = ligma_selection_tool_have_selection (selection_tool, display);

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
              if (ligma_item_mask_intersect (LIGMA_ITEM (iter->data),
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

  ligma_tool_pop_status (tool, display);

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
            case LIGMA_CHANNEL_OP_REPLACE:
              if (have_selection)
                {
                  status = ligma_suggest_modifiers (_("Click-Drag to replace the "
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

            case LIGMA_CHANNEL_OP_ADD:
              status = ligma_suggest_modifiers (_("Click-Drag to add to the "
                                                 "current selection"),
                                               modifiers
                                               & ~(state | extend_mask),
                                               NULL, NULL, NULL);
              free_status = TRUE;
              break;

            case LIGMA_CHANNEL_OP_SUBTRACT:
              status = ligma_suggest_modifiers (_("Click-Drag to subtract from the "
                                                 "current selection"),
                                               modifiers
                                               & ~(state | modify_mask),
                                               NULL, NULL, NULL);
              free_status = TRUE;
              break;

            case LIGMA_CHANNEL_OP_INTERSECT:
              status = ligma_suggest_modifiers (_("Click-Drag to intersect with "
                                                 "the current selection"),
                                               modifiers & ~state,
                                               NULL, NULL, NULL);
              free_status = TRUE;
              break;
            }
          break;

        case SELECTION_MOVE_MASK:
          status = ligma_suggest_modifiers (_("Click-Drag to move the "
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
        ligma_tool_push_status (tool, display, "%s", status);

      if (free_status)
        g_free ((gchar *) status);
    }
}

static void
ligma_selection_tool_cursor_update (LigmaTool         *tool,
                                   const LigmaCoords *coords,
                                   GdkModifierType   state,
                                   LigmaDisplay      *display)
{
  LigmaSelectionTool    *selection_tool = LIGMA_SELECTION_TOOL (tool);
  LigmaSelectionOptions *options;
  LigmaToolCursorType    tool_cursor;
  LigmaCursorModifier    modifier;

  options = LIGMA_SELECTION_TOOL_GET_OPTIONS (tool);

  tool_cursor = ligma_tool_control_get_tool_cursor (tool->control);
  modifier    = LIGMA_CURSOR_MODIFIER_NONE;

  switch (selection_tool->function)
    {
    case SELECTION_SELECT:
      switch (options->operation)
        {
        case LIGMA_CHANNEL_OP_REPLACE:
          break;
        case LIGMA_CHANNEL_OP_ADD:
          modifier = LIGMA_CURSOR_MODIFIER_PLUS;
          break;
        case LIGMA_CHANNEL_OP_SUBTRACT:
          modifier = LIGMA_CURSOR_MODIFIER_MINUS;
          break;
        case LIGMA_CHANNEL_OP_INTERSECT:
          modifier = LIGMA_CURSOR_MODIFIER_INTERSECT;
          break;
        }
      break;

    case SELECTION_MOVE_MASK:
      modifier = LIGMA_CURSOR_MODIFIER_MOVE;
      break;

    case SELECTION_MOVE:
    case SELECTION_MOVE_COPY:
      tool_cursor = LIGMA_TOOL_CURSOR_MOVE;
      break;

    case SELECTION_ANCHOR:
      modifier = LIGMA_CURSOR_MODIFIER_ANCHOR;
      break;
    }

  /*  our subclass might have set a BAD modifier, in which case we leave it
   *  there, since it's more important than what we have to say.
   */
  if (ligma_tool_control_get_cursor_modifier (tool->control) ==
      LIGMA_CURSOR_MODIFIER_BAD                              ||
      ! ligma_selection_tool_check (selection_tool, display, NULL))
    {
      modifier = LIGMA_CURSOR_MODIFIER_BAD;
    }

  ligma_tool_set_cursor (tool, display,
                        ligma_tool_control_get_cursor (tool->control),
                        tool_cursor,
                        modifier);
}

static gboolean
ligma_selection_tool_real_have_selection (LigmaSelectionTool *sel_tool,
                                         LigmaDisplay       *display)
{
  LigmaImage   *image     = ligma_display_get_image (display);
  LigmaChannel *selection = ligma_image_get_mask (image);

  return ! ligma_channel_is_empty (selection);
}

static void
ligma_selection_tool_commit (LigmaSelectionTool *sel_tool)
{
  /* make sure ligma_selection_tool_halt() doesn't undo the change, if any */
  ligma_selection_tool_set_undo_ptr (&sel_tool->undo, NULL);
}

static void
ligma_selection_tool_halt (LigmaSelectionTool *sel_tool,
                          LigmaDisplay       *display)
{
  g_warn_if_fail (sel_tool->change_count == 0);

  if (display)
    {
      LigmaTool      *tool       = LIGMA_TOOL (sel_tool);
      LigmaImage     *image      = ligma_display_get_image (display);
      LigmaUndoStack *undo_stack = ligma_image_get_undo_stack (image);
      LigmaUndo      *undo       = ligma_undo_stack_peek (undo_stack);

      /* if we have an existing selection in the current display, then
       * we have already "executed", and need to undo at this point,
       * unless the user has done something in the meantime
       */
      if (undo && sel_tool->undo == undo)
        {
          /* prevent this change from halting the tool */
          ligma_tool_control_push_preserve (tool->control, TRUE);

          ligma_image_undo (image);
          ligma_image_flush (image);

          ligma_tool_control_pop_preserve (tool->control);
        }

      /* reset the automatic undo/redo mechanism */
      ligma_selection_tool_set_undo_ptr (&sel_tool->undo, NULL);
      ligma_selection_tool_set_undo_ptr (&sel_tool->redo, NULL);
    }
}

static gboolean
ligma_selection_tool_check (LigmaSelectionTool  *sel_tool,
                           LigmaDisplay        *display,
                           GError            **error)
{
  LigmaSelectionOptions *options   = LIGMA_SELECTION_TOOL_GET_OPTIONS (sel_tool);
  LigmaImage            *image     = ligma_display_get_image (display);

  switch (sel_tool->function)
    {
    case SELECTION_SELECT:
      switch (options->operation)
        {
        case LIGMA_CHANNEL_OP_ADD:
        case LIGMA_CHANNEL_OP_REPLACE:
          break;

        case LIGMA_CHANNEL_OP_SUBTRACT:
          if (! ligma_item_bounds (LIGMA_ITEM (ligma_image_get_mask (image)),
                                  NULL, NULL, NULL, NULL))
            {
              g_set_error (error, LIGMA_ERROR, LIGMA_FAILED,
                           _("Cannot subtract from an empty selection."));

              return FALSE;
            }
          break;

        case LIGMA_CHANNEL_OP_INTERSECT:
          if (! ligma_item_bounds (LIGMA_ITEM (ligma_image_get_mask (image)),
                                  NULL, NULL, NULL, NULL))
            {
              g_set_error (error, LIGMA_ERROR, LIGMA_FAILED,
                           _("Cannot intersect with an empty selection."));

              return FALSE;
            }
          break;
        }
      break;

    case SELECTION_MOVE:
    case SELECTION_MOVE_COPY:
        {
          GList    *drawables   = ligma_image_get_selected_drawables (image);
          LigmaItem *locked_item = NULL;
          GList    *iter;

          for (iter = drawables; iter; iter = iter->next)
            {
              if (ligma_viewable_get_children (iter->data))
                {
                  g_set_error (error, LIGMA_ERROR, LIGMA_FAILED,
                               _("Cannot modify the pixels of layer groups."));

                  g_list_free (drawables);
                  return FALSE;
                }
              else if (ligma_item_is_content_locked (iter->data, &locked_item))
                {
                  g_set_error (error, LIGMA_ERROR, LIGMA_FAILED,
                               _("A selected item's pixels are locked."));

                  if (error)
                    ligma_tools_blink_lock_box (display->ligma, locked_item);

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
ligma_selection_tool_have_selection (LigmaSelectionTool *sel_tool,
                                    LigmaDisplay       *display)
{
  return LIGMA_SELECTION_TOOL_GET_CLASS (sel_tool)->have_selection (sel_tool,
                                                                   display);
}

static void
ligma_selection_tool_set_undo_ptr (LigmaUndo **undo_ptr,
                                  LigmaUndo  *undo)
{
  if (*undo_ptr)
    {
      g_object_remove_weak_pointer (G_OBJECT (*undo_ptr),
                                    (gpointer *) undo_ptr);
    }

  *undo_ptr = undo;

  if (*undo_ptr)
    {
      g_object_add_weak_pointer (G_OBJECT (*undo_ptr),
                                 (gpointer *) undo_ptr);
    }
}


/*  public functions  */

gboolean
ligma_selection_tool_start_edit (LigmaSelectionTool *sel_tool,
                                LigmaDisplay       *display,
                                const LigmaCoords  *coords)
{
  LigmaTool             *tool;
  LigmaSelectionOptions *options;
  GError               *error = NULL;

  g_return_val_if_fail (LIGMA_IS_SELECTION_TOOL (sel_tool), FALSE);
  g_return_val_if_fail (LIGMA_IS_DISPLAY (display), FALSE);
  g_return_val_if_fail (coords != NULL, FALSE);

  tool    = LIGMA_TOOL (sel_tool);
  options = LIGMA_SELECTION_TOOL_GET_OPTIONS (sel_tool);

  g_return_val_if_fail (ligma_tool_control_is_active (tool->control) == FALSE,
                        FALSE);

  if (! ligma_selection_tool_check (sel_tool, display, &error))
    {
      ligma_tool_message_literal (tool, display, error->message);

      ligma_tools_show_tool_options (display->ligma);
      ligma_widget_blink (options->mode_box);

      g_clear_error (&error);

      return TRUE;
    }

  switch (sel_tool->function)
    {
    case SELECTION_MOVE_MASK:
      ligma_edit_selection_tool_start (tool, display, coords,
                                      LIGMA_TRANSLATE_MODE_MASK, FALSE);
      return TRUE;

    case SELECTION_MOVE:
    case SELECTION_MOVE_COPY:
      {
        LigmaTranslateMode edit_mode;

        ligma_tool_control (tool, LIGMA_TOOL_ACTION_COMMIT, display);

        if (sel_tool->function == SELECTION_MOVE)
          edit_mode = LIGMA_TRANSLATE_MODE_MASK_TO_LAYER;
        else
          edit_mode = LIGMA_TRANSLATE_MODE_MASK_COPY_TO_LAYER;

        ligma_edit_selection_tool_start (tool, display, coords,
                                        edit_mode, FALSE);

        return TRUE;
      }

    default:
      break;
    }

  return FALSE;
}

static gboolean
ligma_selection_tool_idle (LigmaSelectionTool *sel_tool)
{
  LigmaTool         *tool  = LIGMA_TOOL (sel_tool);
  LigmaDisplayShell *shell = ligma_display_get_shell (tool->display);

  ligma_display_shell_set_show_selection (shell, FALSE);

  sel_tool->idle_id = 0;

  return G_SOURCE_REMOVE;
}

void
ligma_selection_tool_start_change (LigmaSelectionTool *sel_tool,
                                  gboolean           create,
                                  LigmaChannelOps     operation)
{
  LigmaTool         *tool;
  LigmaDisplayShell *shell;
  LigmaImage        *image;
  LigmaUndoStack    *undo_stack;

  g_return_if_fail (LIGMA_IS_SELECTION_TOOL (sel_tool));

  tool = LIGMA_TOOL (sel_tool);

  g_return_if_fail (tool->display != NULL);

  if (sel_tool->change_count++ > 0)
    return;

  shell      = ligma_display_get_shell (tool->display);
  image      = ligma_display_get_image (tool->display);
  undo_stack = ligma_image_get_undo_stack (image);

  sel_tool->saved_show_selection =
    ligma_display_shell_get_show_selection (shell);

  if (create)
    {
      ligma_selection_tool_set_undo_ptr (&sel_tool->undo, NULL);
    }
  else
    {
      LigmaUndoStack *redo_stack = ligma_image_get_redo_stack (image);
      LigmaUndo      *undo;

      undo = ligma_undo_stack_peek (undo_stack);

      if (undo && undo == sel_tool->undo)
        {
          /* prevent this change from halting the tool */
          ligma_tool_control_push_preserve (tool->control, TRUE);

          ligma_image_undo (image);

          ligma_tool_control_pop_preserve (tool->control);

          ligma_selection_tool_set_undo_ptr (&sel_tool->undo, NULL);

          /* we will need to redo if the user cancels or executes */
          ligma_selection_tool_set_undo_ptr (
            &sel_tool->redo,
            ligma_undo_stack_peek (redo_stack));
        }

      /* if the operation is "Replace", turn off the marching ants,
       * because they are confusing ...
       */
      if (operation == LIGMA_CHANNEL_OP_REPLACE)
        {
          /* ... however, do this in an idle function, to avoid unnecessarily
           * restarting the selection if we don't visit the main loop between
           * the start_change() and end_change() calls.
           */
          sel_tool->idle_id = g_idle_add_full (
            G_PRIORITY_HIGH_IDLE,
            (GSourceFunc) ligma_selection_tool_idle,
            sel_tool, NULL);
        }
    }

  ligma_selection_tool_set_undo_ptr (
    &sel_tool->undo,
    ligma_undo_stack_peek (undo_stack));
}

void
ligma_selection_tool_end_change (LigmaSelectionTool *sel_tool,
                                gboolean           cancel)
{
  LigmaTool         *tool;
  LigmaDisplayShell *shell;
  LigmaImage        *image;
  LigmaUndoStack    *undo_stack;

  g_return_if_fail (LIGMA_IS_SELECTION_TOOL (sel_tool));
  g_return_if_fail (sel_tool->change_count > 0);

  tool = LIGMA_TOOL (sel_tool);

  g_return_if_fail (tool->display != NULL);

  if (--sel_tool->change_count > 0)
    return;

  shell      = ligma_display_get_shell (tool->display);
  image      = ligma_display_get_image (tool->display);
  undo_stack = ligma_image_get_undo_stack (image);

  if (cancel)
    {
      LigmaUndoStack *redo_stack = ligma_image_get_redo_stack (image);
      LigmaUndo      *redo       = ligma_undo_stack_peek (redo_stack);

      if (redo && redo == sel_tool->redo)
        {
          /* prevent this from halting the tool */
          ligma_tool_control_push_preserve (tool->control, TRUE);

          ligma_image_redo (image);

          ligma_tool_control_pop_preserve (tool->control);

          ligma_selection_tool_set_undo_ptr (
            &sel_tool->undo,
            ligma_undo_stack_peek (undo_stack));
        }
      else
        {
          ligma_selection_tool_set_undo_ptr (&sel_tool->undo, NULL);
        }
    }
  else
    {
      LigmaUndo *undo = ligma_undo_stack_peek (undo_stack);

      /* save the undo that we got when executing, but only if
       * we actually selected something
       */
      if (undo && undo != sel_tool->undo)
        ligma_selection_tool_set_undo_ptr (&sel_tool->undo, undo);
      else
        ligma_selection_tool_set_undo_ptr (&sel_tool->undo, NULL);
    }

  ligma_selection_tool_set_undo_ptr (&sel_tool->redo, NULL);

  if (sel_tool->idle_id)
    {
      g_source_remove (sel_tool->idle_id);
      sel_tool->idle_id = 0;
    }
  else
    {
      ligma_display_shell_set_show_selection (shell,
                                             sel_tool->saved_show_selection);
    }

  ligma_image_flush (image);
}
