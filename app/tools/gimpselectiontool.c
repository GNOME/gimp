/* The GIMP -- an image manipulation program
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
#include "core/gimptoolinfo.h"

#include "display/gimpdisplay.h"

#include "gimpeditselectiontool.h"
#include "gimpselectiontool.h"
#include "gimpselectionoptions.h"
#include "gimptoolcontrol.h"


static void   gimp_selection_tool_class_init (GimpSelectionToolClass *klass);
static void   gimp_selection_tool_init       (GimpSelectionTool      *sel_tool);

static void   gimp_selection_tool_modifier_key  (GimpTool        *tool,
                                                 GdkModifierType  key,
                                                 gboolean         press,
                                                 GdkModifierType  state,
                                                 GimpDisplay     *gdisp);
static void   gimp_selection_tool_oper_update   (GimpTool        *tool,
                                                 GimpCoords      *coords,
                                                 GdkModifierType  state,
                                                 GimpDisplay     *gdisp);
static void   gimp_selection_tool_cursor_update (GimpTool        *tool,
                                                 GimpCoords      *coords,
                                                 GdkModifierType  state,
                                                 GimpDisplay     *gdisp);


static GimpDrawToolClass *parent_class = NULL;


GType
gimp_selection_tool_get_type (void)
{
  static GType tool_type = 0;

  if (! tool_type)
    {
      static const GTypeInfo tool_info =
      {
        sizeof (GimpSelectionToolClass),
        (GBaseInitFunc) NULL,
        (GBaseFinalizeFunc) NULL,
        (GClassInitFunc) gimp_selection_tool_class_init,
        NULL,           /* class_finalize */
        NULL,           /* class_data     */
        sizeof (GimpSelectionTool),
        0,              /* n_preallocs    */
        (GInstanceInitFunc) gimp_selection_tool_init,
      };

      tool_type = g_type_register_static (GIMP_TYPE_DRAW_TOOL,
                                          "GimpSelectionTool",
                                          &tool_info, 0);
    }

  return tool_type;
}

static void
gimp_selection_tool_class_init (GimpSelectionToolClass *klass)
{
  GimpToolClass *tool_class = GIMP_TOOL_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);

  tool_class->modifier_key  = gimp_selection_tool_modifier_key;
  tool_class->key_press     = gimp_edit_selection_tool_key_press;
  tool_class->oper_update   = gimp_selection_tool_oper_update;
  tool_class->cursor_update = gimp_selection_tool_cursor_update;
}

static void
gimp_selection_tool_init (GimpSelectionTool *selection_tool)
{
  selection_tool->op       = SELECTION_REPLACE;
  selection_tool->saved_op = SELECTION_REPLACE;
}

static void
gimp_selection_tool_modifier_key (GimpTool        *tool,
                                  GdkModifierType  key,
                                  gboolean         press,
                                  GdkModifierType  state,
                                  GimpDisplay     *gdisp)
{
  GimpSelectionTool    *selection_tool = GIMP_SELECTION_TOOL (tool);
  GimpSelectionOptions *options;

  options = GIMP_SELECTION_OPTIONS (tool->tool_info->tool_options);

  if (key == GDK_SHIFT_MASK || key == GDK_CONTROL_MASK)
    {
      SelectOps button_op = options->operation;

      if (press)
        {
          if (key == (state & (GDK_SHIFT_MASK | GDK_CONTROL_MASK)))
            {
              /*  first modifier pressed  */

              selection_tool->saved_op = options->operation;
            }
        }
      else
        {
          if (! (state & (GDK_SHIFT_MASK | GDK_CONTROL_MASK)))
            {
              /*  last modifier released  */

              button_op = selection_tool->saved_op;
            }
        }

      if ((state & GDK_CONTROL_MASK) && (state & GDK_SHIFT_MASK))
        {
          button_op = SELECTION_INTERSECT;
        }
      else if (state & GDK_SHIFT_MASK)
        {
          button_op = SELECTION_ADD;
        }
      else if (state & GDK_CONTROL_MASK)
        {
          button_op = SELECTION_SUBTRACT;
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
                                 GimpDisplay     *gdisp)
{
  GimpSelectionTool    *selection_tool = GIMP_SELECTION_TOOL (tool);
  GimpSelectionOptions *options;
  GimpChannel          *selection;
  GimpLayer            *layer;
  GimpLayer            *floating_sel;
  gboolean              move_layer        = FALSE;
  gboolean              move_floating_sel = FALSE;

  options = GIMP_SELECTION_OPTIONS (tool->tool_info->tool_options);

  selection    = gimp_image_get_mask (gdisp->gimage);
  layer        = gimp_image_pick_correlate_layer (gdisp->gimage,
                                                  coords->x, coords->y);
  floating_sel = gimp_image_floating_sel (gdisp->gimage);

  if (layer)
    {
      if (floating_sel)
        {
          if (layer == floating_sel)
            move_floating_sel = TRUE;
        }
      else if (gimp_channel_value (selection, coords->x, coords->y))
        {
          move_layer = TRUE;
        }
    }

  if ((state & GDK_MOD1_MASK) && (state & GDK_CONTROL_MASK) && move_layer)
    {
      selection_tool->op = SELECTION_MOVE_COPY; /* move a copy of the selection */
    }
  else if ((state & GDK_MOD1_MASK) && ! gimp_channel_is_empty (selection))
    {
      selection_tool->op = SELECTION_MOVE_MASK; /* move the selection mask */
    }
  else if (! (state & (GDK_SHIFT_MASK | GDK_CONTROL_MASK)) &&
           (move_layer || move_floating_sel))
    {
      selection_tool->op = SELECTION_MOVE;      /* move the selection */
    }
  else if ((state & GDK_CONTROL_MASK) && (state & GDK_SHIFT_MASK))
    {
      selection_tool->op = SELECTION_INTERSECT; /* intersect with selection */
    }
  else if (state & GDK_SHIFT_MASK)
    {
      selection_tool->op = SELECTION_ADD;       /* add to the selection */
    }
  else if (state & GDK_CONTROL_MASK)
    {
      selection_tool->op = SELECTION_SUBTRACT;  /* subtract from the selection */
    }
  else if (floating_sel)
    {
      selection_tool->op = SELECTION_ANCHOR;    /* anchor the selection */
    }
  else
    {
      selection_tool->op = options->operation;
    }
}

static void
gimp_selection_tool_cursor_update (GimpTool        *tool,
                                   GimpCoords      *coords,
                                   GdkModifierType  state,
                                   GimpDisplay     *gdisp)
{
  GimpSelectionTool  *selection_tool = GIMP_SELECTION_TOOL (tool);
  GimpToolCursorType  tool_cursor;
  GimpCursorModifier  cmodifier;

  tool_cursor = gimp_tool_control_get_tool_cursor (tool->control);
  cmodifier   = GIMP_CURSOR_MODIFIER_NONE;

  switch (selection_tool->op)
    {
    case SELECTION_ADD:
      cmodifier = GIMP_CURSOR_MODIFIER_PLUS;
      break;
    case SELECTION_SUBTRACT:
      cmodifier = GIMP_CURSOR_MODIFIER_MINUS;
      break;
    case SELECTION_INTERSECT:
      cmodifier = GIMP_CURSOR_MODIFIER_INTERSECT;
      break;
    case SELECTION_REPLACE:
      break;
    case SELECTION_MOVE_MASK:
      cmodifier = GIMP_CURSOR_MODIFIER_MOVE;
      break;
    case SELECTION_MOVE:
    case SELECTION_MOVE_COPY:
      tool_cursor = GIMP_TOOL_CURSOR_MOVE;
      break;
    case SELECTION_ANCHOR:
      cmodifier = GIMP_CURSOR_MODIFIER_ANCHOR;
      break;
    }

  gimp_tool_set_cursor (tool, gdisp,
                        GIMP_CURSOR_MOUSE, tool_cursor, cmodifier);
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

  g_return_val_if_fail (GIMP_IS_DISPLAY (tool->gdisp), FALSE);
  g_return_val_if_fail (gimp_tool_control_is_active (tool->control), FALSE);

  switch (sel_tool->op)
    {
    case SELECTION_MOVE_MASK:
      gimp_edit_selection_tool_start (tool, tool->gdisp, coords,
                                      GIMP_TRANSLATE_MODE_MASK, FALSE);
      return TRUE;

    case SELECTION_MOVE:
      gimp_edit_selection_tool_start (tool, tool->gdisp, coords,
                                      GIMP_TRANSLATE_MODE_MASK_TO_LAYER, FALSE);
      return TRUE;

    case SELECTION_MOVE_COPY:
      gimp_edit_selection_tool_start (tool, tool->gdisp, coords,
                                      GIMP_TRANSLATE_MODE_MASK_COPY_TO_LAYER,
                                      FALSE);
      return TRUE;

    default:
      break;
    }

  return FALSE;
}
