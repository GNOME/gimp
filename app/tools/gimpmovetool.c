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

#include <stdlib.h>

#include <gtk/gtk.h>

#include "libgimpwidgets/gimpwidgets.h"

#include "tools-types.h"

#include "core/gimpimage.h"
#include "core/gimpimage-guides.h"
#include "core/gimplayer.h"
#include "core/gimplayermask.h"
#include "core/gimplayer-floating-sel.h"
#include "core/gimptoolinfo.h"

#include "display/gimpdisplay.h"
#include "display/gimpdisplay-foreach.h"
#include "display/gimpdisplayshell.h"
#include "display/gimpdisplayshell-appearance.h"
#include "display/gimpdisplayshell-draw.h"
#include "display/gimpdisplayshell-transform.h"

#include "widgets/gimphelp-ids.h"
#include "widgets/gimpwidgets-utils.h"

#include "gimpeditselectiontool.h"
#include "gimpmoveoptions.h"
#include "gimpmovetool.h"
#include "gimptoolcontrol.h"

#include "gimp-intl.h"


/*  local function prototypes  */

static void   gimp_move_tool_class_init     (GimpMoveToolClass *klass);
static void   gimp_move_tool_init           (GimpMoveTool      *move_tool);

static void   gimp_move_tool_control        (GimpTool          *tool,
                                             GimpToolAction     action,
                                             GimpDisplay       *gdisp);
static void   gimp_move_tool_button_press   (GimpTool          *tool,
                                             GimpCoords        *coords,
                                             guint32            time,
                                             GdkModifierType    state,
                                             GimpDisplay       *gdisp);
static void   gimp_move_tool_button_release (GimpTool          *tool,
                                             GimpCoords        *coords,
                                             guint32            time,
                                             GdkModifierType    state,
                                             GimpDisplay       *gdisp);
static void   gimp_move_tool_motion         (GimpTool          *tool,
                                             GimpCoords        *coords,
                                             guint32            time,
                                             GdkModifierType    state,
                                             GimpDisplay       *gdisp);
static void   gimp_move_tool_modifier_key   (GimpTool          *tool,
                                             GdkModifierType    key,
                                             gboolean           press,
                                             GdkModifierType    state,
                                             GimpDisplay       *gdisp);
static void   gimp_move_tool_oper_update    (GimpTool          *tool,
                                             GimpCoords        *coords,
                                             GdkModifierType    state,
                                             GimpDisplay       *gdisp);
static void   gimp_move_tool_cursor_update  (GimpTool          *tool,
                                             GimpCoords        *coords,
                                             GdkModifierType    state,
                                             GimpDisplay       *gdisp);

static void   gimp_move_tool_draw           (GimpDrawTool      *draw_tool);

static void   gimp_move_tool_start_guide    (GimpTool            *tool,
                                             GimpDisplay         *gdisp,
                                             GimpOrientationType  orientation);


static GimpDrawToolClass *parent_class = NULL;


void
gimp_move_tool_register (GimpToolRegisterCallback  callback,
                         gpointer                  data)
{
  (* callback) (GIMP_TYPE_MOVE_TOOL,
                GIMP_TYPE_MOVE_OPTIONS,
                gimp_move_options_gui,
                0,
                "gimp-move-tool",
                _("Move"),
                _("Move layers & selections"),
                N_("/Tools/Transform Tools/_Move"), "M",
                NULL, GIMP_HELP_TOOL_MOVE,
                GIMP_STOCK_TOOL_MOVE,
                data);
}

GType
gimp_move_tool_get_type (void)
{
  static GType tool_type = 0;

  if (! tool_type)
    {
      static const GTypeInfo tool_info =
      {
        sizeof (GimpMoveToolClass),
	(GBaseInitFunc) NULL,
	(GBaseFinalizeFunc) NULL,
	(GClassInitFunc) gimp_move_tool_class_init,
	NULL,           /* class_finalize */
	NULL,           /* class_data     */
	sizeof (GimpMoveTool),
	0,              /* n_preallocs    */
	(GInstanceInitFunc) gimp_move_tool_init,
      };

      tool_type = g_type_register_static (GIMP_TYPE_DRAW_TOOL,
					  "GimpMoveTool",
                                          &tool_info, 0);
    }

  return tool_type;
}

static void
gimp_move_tool_class_init (GimpMoveToolClass *klass)
{
  GimpToolClass     *tool_class;
  GimpDrawToolClass *draw_tool_class;

  tool_class      = GIMP_TOOL_CLASS (klass);
  draw_tool_class = GIMP_DRAW_TOOL_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);

  tool_class->control        = gimp_move_tool_control;
  tool_class->button_press   = gimp_move_tool_button_press;
  tool_class->button_release = gimp_move_tool_button_release;
  tool_class->motion         = gimp_move_tool_motion;
  tool_class->arrow_key      = gimp_edit_selection_tool_arrow_key;
  tool_class->modifier_key   = gimp_move_tool_modifier_key;
  tool_class->oper_update    = gimp_move_tool_oper_update;
  tool_class->cursor_update  = gimp_move_tool_cursor_update;

  draw_tool_class->draw      = gimp_move_tool_draw;
}

static void
gimp_move_tool_init (GimpMoveTool *move_tool)
{
  GimpTool *tool = GIMP_TOOL (move_tool);

  move_tool->layer             = NULL;
  move_tool->guide             = NULL;

  move_tool->moving_guide      = FALSE;
  move_tool->guide_position    = -1;
  move_tool->guide_orientation = GIMP_ORIENTATION_UNKNOWN;

  move_tool->saved_type        = GIMP_TRANSFORM_TYPE_LAYER;

  gimp_tool_control_set_snap_to             (tool->control, FALSE);
  gimp_tool_control_set_handles_empty_image (tool->control, TRUE);
  gimp_tool_control_set_tool_cursor         (tool->control,
                                             GIMP_MOVE_TOOL_CURSOR);
}

static void
gimp_move_tool_control (GimpTool       *tool,
                        GimpToolAction  action,
                        GimpDisplay    *gdisp)
{
  GimpDisplayShell *shell;
  GimpMoveTool     *move_tool;

  shell     = GIMP_DISPLAY_SHELL (gdisp->shell);
  move_tool = GIMP_MOVE_TOOL (tool);

  switch (action)
    {
    case PAUSE:
      break;

    case RESUME:
      if (move_tool->guide &&
          gimp_display_shell_get_show_guides (GIMP_DISPLAY_SHELL (shell)))
	gimp_display_shell_draw_guide (GIMP_DISPLAY_SHELL (shell),
                                       move_tool->guide, TRUE);
      break;

    case HALT:
      if (move_tool->guide &&
          gimp_display_shell_get_show_guides (GIMP_DISPLAY_SHELL (shell)))
        gimp_display_shell_draw_guide (GIMP_DISPLAY_SHELL (shell),
                                       move_tool->guide, FALSE);
      break;

    default:
      break;
    }

  GIMP_TOOL_CLASS (parent_class)->control (tool, action, gdisp);
}

static void
gimp_move_tool_button_press (GimpTool        *tool,
                             GimpCoords      *coords,
                             guint32          time,
                             GdkModifierType  state,
                             GimpDisplay     *gdisp)
{
  GimpDisplayShell *shell;
  GimpMoveTool     *move;
  GimpMoveOptions  *options;
  GimpLayer        *layer;
  GimpGuide        *guide;

  shell   = GIMP_DISPLAY_SHELL (gdisp->shell);
  move    = GIMP_MOVE_TOOL (tool);
  options = GIMP_MOVE_OPTIONS (tool->tool_info->tool_options);

  tool->gdisp = gdisp;

  move->layer        = NULL;
  move->guide        = NULL;
  move->moving_guide = FALSE;

  if (options->move_type == GIMP_TRANSFORM_TYPE_PATH)
    {
      if (options->move_current)
        {
          if (gimp_image_get_active_vectors (gdisp->gimage))
            {
              init_edit_selection (tool, gdisp, coords, EDIT_VECTORS_TRANSLATE);
            }
        }
      else
        {
          GimpVectors *vectors;

          if (gimp_draw_tool_on_vectors (GIMP_DRAW_TOOL (tool), gdisp,
                                         coords, 7, 7,
                                         NULL, NULL, NULL, NULL, NULL, &vectors))
            {
              gimp_image_set_active_vectors (gdisp->gimage, vectors);
              init_edit_selection (tool, gdisp, coords, EDIT_VECTORS_TRANSLATE);
            }
        }
    }
  else if (options->move_type == GIMP_TRANSFORM_TYPE_SELECTION)
    {
      if (! gimp_channel_is_empty (gimp_image_get_mask (gdisp->gimage)))
        {
          init_edit_selection (tool, gdisp, coords, EDIT_MASK_TRANSLATE);
        }
    }
  else if (options->move_current)
    {
      GimpDrawable *drawable = gimp_image_active_drawable (gdisp->gimage);

      if (drawable)
        {
          if (GIMP_IS_LAYER_MASK (drawable))
            init_edit_selection (tool, gdisp, coords, EDIT_LAYER_MASK_TRANSLATE);
          else if (GIMP_IS_CHANNEL (drawable))
            init_edit_selection (tool, gdisp, coords, EDIT_CHANNEL_TRANSLATE);
          else
            init_edit_selection (tool, gdisp, coords, EDIT_LAYER_TRANSLATE);
        }
    }
  else
    {
      if (gimp_display_shell_get_show_guides (shell) &&
	  (guide = gimp_image_find_guide (gdisp->gimage, coords->x, coords->y)))
	{
	  move->guide             = guide;
          move->moving_guide      = TRUE;
          move->guide_position    = guide->position;
          move->guide_orientation = guide->orientation;

	  gimp_tool_control_set_scroll_lock (tool->control, TRUE);
	  gimp_tool_control_activate (tool->control);

          gimp_display_shell_selection_visibility (shell, GIMP_SELECTION_PAUSE);

          gimp_draw_tool_start (GIMP_DRAW_TOOL (tool), gdisp);
	}
      else if ((layer = gimp_image_pick_correlate_layer (gdisp->gimage,
                                                         coords->x,
                                                         coords->y)))
	{
	  if (gimp_image_floating_sel (gdisp->gimage) &&
	      ! gimp_layer_is_floating_sel (layer))
	    {
              /*  If there is a floating selection, and this aint it,
               *  use the move tool to anchor it.
               */
	      move->layer = gimp_image_floating_sel (gdisp->gimage);
              gimp_tool_control_activate (tool->control);
	    }
	  else
	    {
              /*  Otherwise, init the edit selection  */
	      gimp_image_set_active_layer (gdisp->gimage, layer);
	      init_edit_selection (tool, gdisp, coords, EDIT_LAYER_TRANSLATE);
	    }
	}
    }
}

static void
gimp_move_tool_button_release (GimpTool        *tool,
                               GimpCoords      *coords,
                               guint32          time,
                               GdkModifierType  state,
                               GimpDisplay     *gdisp)
{
  GimpMoveTool     *move;
  GimpDisplayShell *shell;

  move  = GIMP_MOVE_TOOL (tool);
  shell = GIMP_DISPLAY_SHELL (gdisp->shell);

  gimp_tool_control_halt (tool->control);

  if (move->moving_guide)
    {
      gboolean delete_guide = FALSE;
      gint     x, y, width, height;

      gimp_tool_control_set_scroll_lock (tool->control, FALSE);
      gimp_draw_tool_stop (GIMP_DRAW_TOOL (tool));

      if (state & GDK_BUTTON3_MASK)
        {
          move->moving_guide      = FALSE;
          move->guide_position    = -1;
          move->guide_orientation = GIMP_ORIENTATION_UNKNOWN;

          gimp_display_shell_selection_visibility (shell,
                                                   GIMP_SELECTION_RESUME);
          return;
        }

      gimp_display_shell_untransform_viewport (shell, &x, &y, &width, &height);

      switch (move->guide_orientation)
	{
	case GIMP_ORIENTATION_HORIZONTAL:
	  if ((move->guide_position < y) ||
              (move->guide_position > (y + height)))
	    delete_guide = TRUE;
	  break;

	case GIMP_ORIENTATION_VERTICAL:
	  if ((move->guide_position < x) ||
              (move->guide_position > (x + width)))
	    delete_guide = TRUE;
	  break;

	default:
	  break;
	}

      if (delete_guide)
	{
          if (move->guide)
            {
              gimp_image_remove_guide (gdisp->gimage, move->guide, TRUE);
              move->guide = NULL;
            }
	}
      else
        {
          if (move->guide)
            {
              gimp_image_move_guide (gdisp->gimage, move->guide,
                                     move->guide_position, TRUE);
            }
          else
            {
              switch (move->guide_orientation)
                {
                case GIMP_ORIENTATION_HORIZONTAL:
                  move->guide = gimp_image_add_hguide (gdisp->gimage,
                                                       move->guide_position,
                                                       TRUE);
                  break;

                case GIMP_ORIENTATION_VERTICAL:
                  move->guide = gimp_image_add_vguide (gdisp->gimage,
                                                       move->guide_position,
                                                       TRUE);
                  break;

                default:
                  g_assert_not_reached ();
                }
            }
        }

      gimp_display_shell_selection_visibility (shell, GIMP_SELECTION_RESUME);
      gimp_image_flush (gdisp->gimage);

      if (move->guide)
	gimp_display_shell_draw_guide (shell, move->guide, TRUE);

      move->moving_guide      = FALSE;
      move->guide_position    = -1;
      move->guide_orientation = GIMP_ORIENTATION_UNKNOWN;
    }
  else
    {
      /*  Take care of the case where the user "cancels" the action  */
      if (! (state & GDK_BUTTON3_MASK))
	{
	  if (move->layer)
	    {
	      floating_sel_anchor (move->layer);
	      gimp_image_flush (gdisp->gimage);
	    }
	}
    }
}

static void
gimp_move_tool_motion (GimpTool        *tool,
                       GimpCoords      *coords,
                       guint32          time,
                       GdkModifierType  state,
                       GimpDisplay     *gdisp)

{
  GimpMoveTool     *move;
  GimpDisplayShell *shell;

  move  = GIMP_MOVE_TOOL (tool);
  shell = GIMP_DISPLAY_SHELL (gdisp->shell);

  if (move->moving_guide)
    {
      gint tx, ty;

      gimp_draw_tool_pause (GIMP_DRAW_TOOL (tool));

      gimp_display_shell_transform_xy (shell,
                                       coords->x, coords->y,
                                       &tx, &ty,
                                       FALSE);

      if (tx < 0 || tx >= shell->disp_width ||
          ty < 0 || ty >= shell->disp_height)
	{
	  move->guide_position = -1;
	}
      else
        {
          if (move->guide_orientation == GIMP_ORIENTATION_HORIZONTAL)
            move->guide_position = ROUND (coords->y);
          else
            move->guide_position = ROUND (coords->x);
        }

      gimp_draw_tool_resume (GIMP_DRAW_TOOL (tool));
    }
}

static void
gimp_move_tool_modifier_key (GimpTool        *tool,
                             GdkModifierType  key,
                             gboolean         press,
			     GdkModifierType  state,
			     GimpDisplay     *gdisp)
{
  GimpMoveTool    *move_tool;
  GimpMoveOptions *options;

  move_tool = GIMP_MOVE_TOOL (tool);
  options   = GIMP_MOVE_OPTIONS (tool->tool_info->tool_options);

  if (key == GDK_SHIFT_MASK)
    {
      g_object_set (options, "move-current", ! options->move_current, NULL);
    }
  else if (key == GDK_MOD1_MASK || key == GDK_CONTROL_MASK)
    {
      GimpTransformType button_type;

      button_type = options->move_type;

      if (press)
        {
          if (key == (state & (GDK_MOD1_MASK | GDK_CONTROL_MASK)))
            {
              /*  first modifier pressed  */

              move_tool->saved_type = options->move_type;
            }
        }
      else
        {
          if (! (state & (GDK_MOD1_MASK | GDK_CONTROL_MASK)))
            {
              /*  last modifier released  */

              button_type = move_tool->saved_type;
            }
        }

      if (state & GDK_MOD1_MASK)
        {
          button_type = GIMP_TRANSFORM_TYPE_SELECTION;
        }
      else if (state & GDK_CONTROL_MASK)
        {
          button_type = GIMP_TRANSFORM_TYPE_PATH;
        }

      if (button_type != options->move_type)
        {
          g_object_set (options, "move-type", button_type, NULL);
        }
    }
}

static void
gimp_move_tool_oper_update (GimpTool        *tool,
                            GimpCoords      *coords,
                            GdkModifierType  state,
                            GimpDisplay     *gdisp)
{
  GimpDisplayShell *shell;
  GimpMoveTool     *move;
  GimpMoveOptions  *options;
  GimpGuide        *guide = NULL;

  shell   = GIMP_DISPLAY_SHELL (gdisp->shell);
  move    = GIMP_MOVE_TOOL (tool);
  options = GIMP_MOVE_OPTIONS (tool->tool_info->tool_options);

  if (options->move_type == GIMP_TRANSFORM_TYPE_LAYER &&
      ! options->move_current                         &&
      gimp_display_shell_get_show_guides (shell)      &&
      shell->proximity)
    {
      guide = gimp_image_find_guide (gdisp->gimage, coords->x, coords->y);
    }

  if (move->guide && move->guide != guide)
    {
      gimp_display_shell_draw_guide (shell, move->guide, FALSE);
      move->guide = NULL;
    }

  if (guide)
    {
      gimp_display_shell_draw_guide (shell, guide, TRUE);
      move->guide = guide;
    }
}

static void
gimp_move_tool_cursor_update (GimpTool        *tool,
                              GimpCoords      *coords,
                              GdkModifierType  state,
                              GimpDisplay     *gdisp)
{
  GimpDisplayShell *shell;
  GimpMoveTool     *move;
  GimpMoveOptions  *options;

  GdkCursorType      cursor      = GIMP_BAD_CURSOR;
  GimpToolCursorType tool_cursor = GIMP_MOVE_TOOL_CURSOR;
  GimpCursorModifier modifier    = GIMP_CURSOR_MODIFIER_NONE;

  shell   = GIMP_DISPLAY_SHELL (gdisp->shell);
  move    = GIMP_MOVE_TOOL (tool);
  options = GIMP_MOVE_OPTIONS (tool->tool_info->tool_options);

  if (options->move_type == GIMP_TRANSFORM_TYPE_PATH)
    {
      tool_cursor = GIMP_BEZIER_SELECT_TOOL_CURSOR;
      modifier    = GIMP_CURSOR_MODIFIER_MOVE;

      if (options->move_current)
        {
          if (gimp_image_get_active_vectors (gdisp->gimage))
            cursor = GIMP_MOUSE_CURSOR;
        }
      else
        {
          if (gimp_draw_tool_on_vectors (GIMP_DRAW_TOOL (tool), gdisp,
                                         coords, 7, 7,
                                         NULL, NULL, NULL, NULL, NULL, NULL))
            {
              cursor      = GDK_HAND2;
              tool_cursor = GIMP_HAND_TOOL_CURSOR;
            }
        }
    }
  else if (options->move_type == GIMP_TRANSFORM_TYPE_SELECTION)
    {
      tool_cursor = GIMP_RECT_SELECT_TOOL_CURSOR;
      modifier    = GIMP_CURSOR_MODIFIER_MOVE;

      if (! gimp_channel_is_empty (gimp_image_get_mask (gdisp->gimage)))
        cursor = GIMP_MOUSE_CURSOR;
    }
  else if (options->move_current)
    {
      if (gimp_image_active_drawable (gdisp->gimage))
        cursor = GIMP_MOUSE_CURSOR;
    }
  else
    {
      GimpGuide *guide;
      GimpLayer *layer;

      if (gimp_display_shell_get_show_guides (shell) &&
          (guide = gimp_image_find_guide (gdisp->gimage, coords->x, coords->y)))
        {
          cursor      = GDK_HAND2;
          tool_cursor = GIMP_HAND_TOOL_CURSOR;
          modifier    = GIMP_CURSOR_MODIFIER_MOVE;
	}
      else if ((layer = gimp_image_pick_correlate_layer (gdisp->gimage,
                                                         coords->x, coords->y)))
	{
	  /*  if there is a floating selection, and this aint it...  */
	  if (gimp_image_floating_sel (gdisp->gimage) &&
	      ! gimp_layer_is_floating_sel (layer))
	    {
              cursor      = GIMP_MOUSE_CURSOR;
              tool_cursor = GIMP_RECT_SELECT_TOOL_CURSOR;
              modifier    = GIMP_CURSOR_MODIFIER_ANCHOR;
	    }
	  else if (layer == gimp_image_get_active_layer (gdisp->gimage))
	    {
              cursor = GIMP_MOUSE_CURSOR;
	    }
	  else
	    {
              cursor      = GDK_HAND2;
              tool_cursor = GIMP_HAND_TOOL_CURSOR;
              modifier    = GIMP_CURSOR_MODIFIER_MOVE;
	    }
	}
    }

  gimp_tool_control_set_cursor          (tool->control, cursor);
  gimp_tool_control_set_tool_cursor     (tool->control, tool_cursor);
  gimp_tool_control_set_cursor_modifier (tool->control, modifier);

  GIMP_TOOL_CLASS (parent_class)->cursor_update (tool, coords, state, gdisp);
}

static void
gimp_move_tool_draw (GimpDrawTool *draw_tool)
{
  GimpMoveTool *move;
  GimpTool     *tool;

  move = GIMP_MOVE_TOOL (draw_tool);
  tool = GIMP_TOOL (draw_tool);

  if (move->moving_guide && move->guide_position != -1)
    {
      switch (move->guide_orientation)
        {
        case GIMP_ORIENTATION_HORIZONTAL:
          gimp_draw_tool_draw_line (draw_tool,
                                    0, move->guide_position,
                                    draw_tool->gdisp->gimage->width,
                                    move->guide_position,
                                    FALSE);
          break;

        case GIMP_ORIENTATION_VERTICAL:
          gimp_draw_tool_draw_line (draw_tool,
                                    move->guide_position, 0,
                                    move->guide_position,
                                    draw_tool->gdisp->gimage->height,
                                    FALSE);
          break;

        default:
          g_assert_not_reached ();
        }
    }
}

void
gimp_move_tool_start_hguide (GimpTool    *tool,
			     GimpDisplay *gdisp)
{
  gimp_move_tool_start_guide (tool, gdisp, GIMP_ORIENTATION_HORIZONTAL);
}

void
gimp_move_tool_start_vguide (GimpTool    *tool,
			     GimpDisplay *gdisp)
{
  gimp_move_tool_start_guide (tool, gdisp, GIMP_ORIENTATION_VERTICAL);
}

static void
gimp_move_tool_start_guide (GimpTool            *tool,
                            GimpDisplay         *gdisp,
                            GimpOrientationType  orientation)
{
  GimpMoveTool *move;

  g_return_if_fail (GIMP_IS_MOVE_TOOL (tool));
  g_return_if_fail (GIMP_IS_DISPLAY (gdisp));

  move = GIMP_MOVE_TOOL (tool);

  gimp_display_shell_selection_visibility (GIMP_DISPLAY_SHELL (gdisp->shell),
                                           GIMP_SELECTION_PAUSE);

  tool->gdisp = gdisp;
  gimp_tool_control_activate (tool->control);
  gimp_tool_control_set_scroll_lock (tool->control, TRUE);

  if (move->guide)
    gimp_display_shell_draw_guide (GIMP_DISPLAY_SHELL (gdisp->shell),
                                   move->guide, FALSE);

  move->guide             = NULL;
  move->moving_guide      = TRUE;
  move->guide_position    = -1;
  move->guide_orientation = orientation;

  gimp_tool_set_cursor (tool, gdisp,
                        GDK_HAND2,
                        GIMP_HAND_TOOL_CURSOR,
                        GIMP_CURSOR_MODIFIER_MOVE);

  gimp_draw_tool_start (GIMP_DRAW_TOOL (tool), gdisp);
}
