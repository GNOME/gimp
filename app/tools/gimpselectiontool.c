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

#include "core/gimpimage.h"
#include "core/gimpimage-mask.h"
#include "core/gimptoolinfo.h"

#include "display/gimpdisplay.h"

#include "gimpdrawtool.h"
#include "gimpeditselectiontool.h"
#include "gimpselectiontool.h"
#include "selection_options.h"


static void   gimp_selection_tool_class_init    (GimpSelectionToolClass *klass);
static void   gimp_selection_tool_init          (GimpSelectionTool      *selection_tool);

static void   gimp_selection_tool_modifier_key    (GimpTool          *tool,
                                                   GdkModifierType    key,
                                                   gboolean           press,
                                                   GdkModifierType    state,
                                                   GimpDisplay       *gdisp);
static void   gimp_selection_tool_oper_update     (GimpTool          *tool,
                                                   GimpCoords        *coords,
                                                   GdkModifierType    state,
                                                   GimpDisplay       *gdisp);
static void   gimp_selection_tool_cursor_update   (GimpTool          *tool,
                                                   GimpCoords        *coords,
                                                   GdkModifierType    state,
                                                   GimpDisplay       *gdisp);


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
  GimpToolClass *tool_class;

  tool_class = GIMP_TOOL_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);

  tool_class->modifier_key  = gimp_selection_tool_modifier_key;
  tool_class->arrow_key     = gimp_edit_selection_tool_arrow_key;
  tool_class->oper_update   = gimp_selection_tool_oper_update;
  tool_class->cursor_update = gimp_selection_tool_cursor_update;
}

static void
gimp_selection_tool_init (GimpSelectionTool *selection_tool)
{
  GimpTool *tool;

  tool = GIMP_TOOL (selection_tool);
  
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
  GimpSelectionTool *selection_tool;
  SelectionOptions  *sel_options;
  SelectOps          button_op;

  selection_tool = GIMP_SELECTION_TOOL (tool);

  sel_options = (SelectionOptions *) tool->tool_info->tool_options;

  button_op = sel_options->op;

  if (key == GDK_SHIFT_MASK || key == GDK_CONTROL_MASK)
    {
      if (press)
        {
          if (key == state) /*  first modifier pressed  */
            {
              selection_tool->saved_op = sel_options->op;
            }
        }
      else
        {
          if (! state) /*  last modifier released  */
            {
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
          button_op = SELECTION_SUB;
        }

      if (button_op != sel_options->op)
        {
          gimp_radio_group_set_active (GTK_RADIO_BUTTON (sel_options->op_w[0]),
                                       GINT_TO_POINTER (button_op));
        }
    }
}

static void
gimp_selection_tool_oper_update (GimpTool        *tool,
                                 GimpCoords      *coords,
                                 GdkModifierType  state,
                                 GimpDisplay     *gdisp)
{
  GimpSelectionTool *selection_tool;
  SelectionOptions  *sel_options;
  GimpLayer         *layer;
  GimpLayer         *floating_sel;

  selection_tool = GIMP_SELECTION_TOOL (tool);

  sel_options = (SelectionOptions *) tool->tool_info->tool_options;

  layer = gimp_image_pick_correlate_layer (gdisp->gimage, coords->x, coords->y);
  floating_sel = gimp_image_floating_sel (gdisp->gimage);

  if ((state & GDK_MOD1_MASK) && ! gimp_image_mask_is_empty (gdisp->gimage))
    {
      selection_tool->op = SELECTION_MOVE_MASK; /* move the selection mask */
    }
  else if (! (state & (GDK_SHIFT_MASK | GDK_CONTROL_MASK)) &&
           layer &&
	   (layer == floating_sel ||
	    (gimp_image_mask_value (gdisp->gimage, coords->x, coords->y) &&
	     floating_sel == NULL)))
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
      selection_tool->op = SELECTION_SUB;       /* subtract from the selection */
    }
  else if (floating_sel)
    {
      selection_tool->op = SELECTION_ANCHOR;    /* anchor the selection */
    }
  else
    {
      selection_tool->op = sel_options->op;
    }
}

static void
gimp_selection_tool_cursor_update (GimpTool        *tool,
                                   GimpCoords      *coords,
                                   GdkModifierType  state,
                                   GimpDisplay     *gdisp)
{
  GimpSelectionTool  *selection_tool;
  GimpToolCursorType  tool_cursor;
  GimpCursorModifier  cmodifier;

  selection_tool = GIMP_SELECTION_TOOL (tool);

  tool_cursor = tool->tool_cursor;
  cmodifier   = GIMP_CURSOR_MODIFIER_NONE;

  switch (selection_tool->op)
    {
    case SELECTION_ADD:
      cmodifier = GIMP_CURSOR_MODIFIER_PLUS;
      break;
    case SELECTION_SUB:
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
      tool_cursor = GIMP_MOVE_TOOL_CURSOR;
      cmodifier   = GIMP_CURSOR_MODIFIER_NONE;
      break;
    case SELECTION_ANCHOR:
      cmodifier = GIMP_CURSOR_MODIFIER_ANCHOR;
      break;
    }

  gimp_tool_set_cursor (tool, gdisp,
                        GIMP_MOUSE_CURSOR,
                        tool_cursor,
                        cmodifier);
}
