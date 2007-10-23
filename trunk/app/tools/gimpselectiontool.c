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
#include <gdk/gdkkeysyms.h>

#include "libgimpwidgets/gimpwidgets.h"

#include "tools-types.h"

#include "core/gimpchannel.h"
#include "core/gimpimage.h"
#include "core/gimppickable.h"

#include "display/gimpdisplay.h"

#include "widgets/gimpwidgets-utils.h"

#include "gimpeditselectiontool.h"
#include "gimpselectiontool.h"
#include "gimpselectionoptions.h"
#include "gimptoolcontrol.h"

#include "gimp-intl.h"


static void   gimp_selection_tool_modifier_key  (GimpTool        *tool,
                                                 GdkModifierType  key,
                                                 gboolean         press,
                                                 GdkModifierType  state,
                                                 GimpDisplay     *display);
static void   gimp_selection_tool_oper_update   (GimpTool        *tool,
                                                 GimpCoords      *coords,
                                                 GdkModifierType  state,
                                                 gboolean         proximity,
                                                 GimpDisplay     *display);
static void   gimp_selection_tool_cursor_update (GimpTool        *tool,
                                                 GimpCoords      *coords,
                                                 GdkModifierType  state,
                                                 GimpDisplay     *display);


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
  GimpSelectionOptions *options;

  options = GIMP_SELECTION_TOOL_GET_OPTIONS (tool);

  if (key == GDK_SHIFT_MASK   ||
      key == GDK_CONTROL_MASK ||
      key == GDK_MOD1_MASK)
    {
      GimpChannelOps button_op = options->operation;

      if (press)
        {
          if (key == (state & (GDK_SHIFT_MASK   |
                               GDK_CONTROL_MASK |
                               GDK_MOD1_MASK)))
            {
              /*  first modifier pressed  */

              selection_tool->saved_operation = options->operation;
            }
        }
      else
        {
          if (! (state & (GDK_SHIFT_MASK   |
                          GDK_CONTROL_MASK |
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
      else if ((state & GDK_CONTROL_MASK) && (state & GDK_SHIFT_MASK))
        {
          button_op = GIMP_CHANNEL_OP_INTERSECT;
        }
      else if (state & GDK_SHIFT_MASK)
        {
          button_op = GIMP_CHANNEL_OP_ADD;
        }
      else if (state & GDK_CONTROL_MASK)
        {
          button_op = GIMP_CHANNEL_OP_SUBTRACT;
        }

      if (button_op != options->operation)
        {
          g_object_set (options, "operation", button_op, NULL);
        }
    }
}

static void
gimp_selection_tool_oper_update (GimpTool        *tool,
                                 GimpCoords      *coords,
                                 GdkModifierType  state,
                                 gboolean         proximity,
                                 GimpDisplay     *display)
{
  GimpSelectionTool    *selection_tool = GIMP_SELECTION_TOOL (tool);
  GimpSelectionOptions *options;
  GimpChannel          *selection;
  GimpDrawable         *drawable;
  GimpLayer            *layer;
  GimpLayer            *floating_sel;
  gboolean              move_layer        = FALSE;
  gboolean              move_floating_sel = FALSE;
  gboolean              selection_empty;

  options = GIMP_SELECTION_TOOL_GET_OPTIONS (tool);

  selection    = gimp_image_get_mask (display->image);
  drawable     = gimp_image_get_active_drawable (display->image);
  layer        = gimp_image_pick_correlate_layer (display->image,
                                                  coords->x, coords->y);
  floating_sel = gimp_image_floating_sel (display->image);

  if (drawable)
    {
      if (floating_sel)
        {
          if (layer == floating_sel)
            move_floating_sel = TRUE;
        }
      else if (gimp_drawable_mask_intersect (drawable,
                                             NULL, NULL, NULL, NULL))
        {
          move_layer = TRUE;
        }
    }

  selection_empty = gimp_channel_is_empty (selection);

  selection_tool->function = SELECTION_SELECT;

  if (selection_tool->allow_move &&
      (state & GDK_MOD1_MASK) && (state & GDK_CONTROL_MASK) && move_layer)
    {
      /* move the selection */
      selection_tool->function = SELECTION_MOVE;
    }
  else if (selection_tool->allow_move &&
           (state & GDK_MOD1_MASK) && (state & GDK_SHIFT_MASK) && move_layer)
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
           ! (state & (GDK_SHIFT_MASK | GDK_CONTROL_MASK)) &&
           move_floating_sel)
    {
      /* move the selection */
      selection_tool->function = SELECTION_MOVE;
    }
  else if ((state & GDK_CONTROL_MASK) || (state & GDK_SHIFT_MASK))
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
      gchar           *status      = NULL;
      gboolean         free_status = FALSE;
      GdkModifierType  modifiers   = (GDK_SHIFT_MASK | GDK_CONTROL_MASK);

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
                                               & ~(state | GDK_SHIFT_MASK),
                                               NULL, NULL, NULL);
              free_status = TRUE;
              break;

            case GIMP_CHANNEL_OP_SUBTRACT:
              status = gimp_suggest_modifiers (_("Click-Drag to subtract from the "
                                                 "current selection"),
                                               modifiers
                                               & ~(state | GDK_CONTROL_MASK),
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
        gimp_tool_push_status (tool, display, status);

      if (free_status)
        g_free (status);
    }
}

static void
gimp_selection_tool_cursor_update (GimpTool        *tool,
                                   GimpCoords      *coords,
                                   GdkModifierType  state,
                                   GimpDisplay     *display)
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
                                GimpCoords        *coords)
{
  GimpTool *tool;

  g_return_val_if_fail (GIMP_IS_SELECTION_TOOL (sel_tool), FALSE);
  g_return_val_if_fail (coords != NULL, FALSE);

  tool = GIMP_TOOL (sel_tool);

  g_return_val_if_fail (GIMP_IS_DISPLAY (tool->display), FALSE);
  g_return_val_if_fail (gimp_tool_control_is_active (tool->control), FALSE);

  switch (sel_tool->function)
    {
    case SELECTION_MOVE_MASK:
      gimp_edit_selection_tool_start (tool, tool->display, coords,
                                      GIMP_TRANSLATE_MODE_MASK, FALSE);
      return TRUE;

    case SELECTION_MOVE:
      gimp_edit_selection_tool_start (tool, tool->display, coords,
                                      GIMP_TRANSLATE_MODE_MASK_TO_LAYER, FALSE);
      return TRUE;

    case SELECTION_MOVE_COPY:
      gimp_edit_selection_tool_start (tool, tool->display, coords,
                                      GIMP_TRANSLATE_MODE_MASK_COPY_TO_LAYER,
                                      FALSE);
      return TRUE;

    default:
      break;
    }

  return FALSE;
}
