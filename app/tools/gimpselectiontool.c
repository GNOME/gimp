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

#include "libgimpwidgets/gimpwidgets.h"

#include "tools-types.h"

#include "core/gimpchannel.h"
#include "core/gimpimage.h"
#include "core/gimpimage-pick-item.h"
#include "core/gimppickable.h"

#include "display/gimpdisplay.h"

#include "widgets/gimpwidgets-utils.h"

#include "gimpeditselectiontool.h"
#include "gimpselectiontool.h"
#include "gimpselectionoptions.h"
#include "gimptoolcontrol.h"

#include "gimp-intl.h"


static void   gimp_selection_tool_modifier_key  (GimpTool         *tool,
                                                 GdkModifierType   key,
                                                 gboolean          press,
                                                 GdkModifierType   state,
                                                 GimpDisplay      *display);
static void   gimp_selection_tool_oper_update   (GimpTool         *tool,
                                                 const GimpCoords *coords,
                                                 GdkModifierType   state,
                                                 gboolean          proximity,
                                                 GimpDisplay      *display);
static void   gimp_selection_tool_cursor_update (GimpTool         *tool,
                                                 const GimpCoords *coords,
                                                 GdkModifierType   state,
                                                 GimpDisplay      *display);


G_DEFINE_TYPE (GimpSelectionTool, gimp_selection_tool, GIMP_TYPE_DRAW_TOOL)

#define parent_class gimp_selection_tool_parent_class


static void
gimp_selection_tool_class_init (GimpSelectionToolClass *klass)
{
  GimpToolClass *tool_class = GIMP_TOOL_CLASS (klass);

  tool_class->modifier_key  = gimp_selection_tool_modifier_key;
  tool_class->key_press     = gimp_edit_selection_tool_key_press;
  tool_class->oper_update   = gimp_selection_tool_oper_update;
  tool_class->cursor_update = gimp_selection_tool_cursor_update;
}

static void
gimp_selection_tool_init (GimpSelectionTool *selection_tool)
{
  selection_tool->function        = SELECTION_SELECT;
  selection_tool->saved_operation = GIMP_CHANNEL_OP_REPLACE;

  selection_tool->allow_move      = TRUE;
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

      if (press)
        {
          if (key == (state & (extend_mask |
                               modify_mask |
                               GDK_MOD1_MASK)))
            {
              /*  first modifier pressed  */

              selection_tool->saved_operation = options->operation;
            }
        }
      else
        {
          if (! (state & (extend_mask |
                          modify_mask |
                          GDK_MOD1_MASK)))
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
  GimpChannel          *selection;
  GimpDrawable         *drawable;
  GimpLayer            *layer;
  GimpLayer            *floating_sel;
  GdkModifierType       extend_mask;
  GdkModifierType       modify_mask;
  gboolean              move_layer        = FALSE;
  gboolean              move_floating_sel = FALSE;
  gboolean              selection_empty;

  image        = gimp_display_get_image (display);
  selection    = gimp_image_get_mask (image);
  drawable     = gimp_image_get_active_drawable (image);
  layer        = gimp_image_pick_layer (image, coords->x, coords->y);
  floating_sel = gimp_image_get_floating_selection (image);

  extend_mask = gimp_get_extend_selection_mask ();
  modify_mask = gimp_get_modify_selection_mask ();

  if (drawable)
    {
      if (floating_sel)
        {
          if (layer == floating_sel)
            move_floating_sel = TRUE;
        }
      else if (gimp_item_mask_intersect (GIMP_ITEM (drawable),
                                         NULL, NULL, NULL, NULL))
        {
          move_layer = TRUE;
        }
    }

  selection_empty = gimp_channel_is_empty (selection);

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
           (state & GDK_MOD1_MASK) && ! selection_empty)
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

      if (! selection_empty)
        modifiers |= GDK_MOD1_MASK;

      switch (selection_tool->function)
        {
        case SELECTION_SELECT:
          switch (options->operation)
            {
            case GIMP_CHANNEL_OP_REPLACE:
              if (! selection_empty)
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

  /*  we don't set the bad modifier ourselves, so a subclass has set
   *  it, always leave it there since it's more important than what we
   *  have to say.
   */
  if (gimp_tool_control_get_cursor_modifier (tool->control) ==
      GIMP_CURSOR_MODIFIER_BAD)
    {
      modifier = GIMP_CURSOR_MODIFIER_BAD;
    }

  gimp_tool_set_cursor (tool, display,
                        gimp_tool_control_get_cursor (tool->control),
                        tool_cursor,
                        modifier);
}


/*  public functions  */

gboolean
gimp_selection_tool_start_edit (GimpSelectionTool *sel_tool,
                                GimpDisplay       *display,
                                const GimpCoords  *coords)
{
  GimpTool *tool;

  g_return_val_if_fail (GIMP_IS_SELECTION_TOOL (sel_tool), FALSE);
  g_return_val_if_fail (GIMP_IS_DISPLAY (display), FALSE);
  g_return_val_if_fail (coords != NULL, FALSE);

  tool = GIMP_TOOL (sel_tool);

  g_return_val_if_fail (gimp_tool_control_is_active (tool->control) == FALSE,
                        FALSE);

  switch (sel_tool->function)
    {
    case SELECTION_MOVE_MASK:
      gimp_edit_selection_tool_start (tool, display, coords,
                                      GIMP_TRANSLATE_MODE_MASK, FALSE);
      return TRUE;

    case SELECTION_MOVE:
    case SELECTION_MOVE_COPY:
      {
        GimpImage    *image    = gimp_display_get_image (display);
        GimpDrawable *drawable = gimp_image_get_active_drawable (image);

        if (gimp_viewable_get_children (GIMP_VIEWABLE (drawable)))
          {
            gimp_tool_message_literal (tool, display,
                                       _("Cannot modify the pixels of layer groups."));
          }
        else if (gimp_item_is_content_locked (GIMP_ITEM (drawable)))
          {
            gimp_tool_message_literal (tool, display,
                                       _("The active layer's pixels are locked."));
          }
        else
          {
            GimpTranslateMode edit_mode;

            if (sel_tool->function == SELECTION_MOVE)
              edit_mode = GIMP_TRANSLATE_MODE_MASK_TO_LAYER;
            else
              edit_mode = GIMP_TRANSLATE_MODE_MASK_COPY_TO_LAYER;

            gimp_edit_selection_tool_start (tool, display, coords,
                                            edit_mode, FALSE);
         }

        return TRUE;
      }

    default:
      break;
    }

  return FALSE;
}
