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
#include "core/gimpimage-mask.h"
#include "core/gimplayer.h"
#include "core/gimplayer-floating-sel.h"
#include "core/gimptoolinfo.h"

#include "display/gimpdisplay.h"
#include "display/gimpdisplay-foreach.h"
#include "display/gimpdisplayshell.h"

#include "gimpeditselectiontool.h"
#include "gimpmovetool.h"
#include "tool_options.h"

#include "undo.h"

#include "libgimp/gimpintl.h"


typedef struct _MoveOptions MoveOptions;

struct _MoveOptions
{
  GimpToolOptions  tool_options;

  gboolean         move_current;
  gboolean         move_current_d;
  GtkWidget       *move_current_w[2];

  gboolean         move_mask;
  gboolean         move_mask_d;
  GtkWidget       *move_mask_w[2];
};


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
static void   gimp_move_tool_cursor_update  (GimpTool          *tool,
                                             GimpCoords        *coords,
                                             GdkModifierType    state,
                                             GimpDisplay       *gdisp);

static void   gimp_move_tool_draw           (GimpDrawTool      *draw_tool);

static void   gimp_move_tool_start_guide    (GimpTool          *tool,
                                             GimpDisplay       *gdisp,
                                             OrientationType    orientation);

static GimpToolOptions * move_options_new   (GimpToolInfo      *tool_info);
static void              move_options_reset (GimpToolOptions   *tool_options);


static GimpDrawToolClass *parent_class = NULL;


void
gimp_move_tool_register (GimpToolRegisterCallback  callback,
                         gpointer                  data)
{
  (* callback) (GIMP_TYPE_MOVE_TOOL,
                move_options_new,
                FALSE,
                "gimp-move-tool",
                _("Move Tool"),
                _("Move layers & selections"),
                N_("/Tools/Transform Tools/Move"), "M",
                NULL, "tools/move.html",
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
  tool_class->cursor_update  = gimp_move_tool_cursor_update;

  draw_tool_class->draw      = gimp_move_tool_draw;
}

static void
gimp_move_tool_init (GimpMoveTool *move_tool)
{
  GimpTool *tool;

  tool = GIMP_TOOL (move_tool);

  move_tool->layer = NULL;
  move_tool->guide = NULL;
  move_tool->disp  = NULL;

  gimp_tool_control_set_snap_to             (tool->control, FALSE);
  gimp_tool_control_set_handles_empty_image (tool->control, TRUE);
  gimp_tool_control_set_tool_cursor         (tool->control, GIMP_MOVE_TOOL_CURSOR);

}

static void
gimp_move_tool_control (GimpTool       *tool,
                        GimpToolAction  action,
                        GimpDisplay    *gdisp)
{
  GimpMoveTool *move_tool;

  move_tool = GIMP_MOVE_TOOL (tool);

  switch (action)
    {
    case PAUSE:
      break;

    case RESUME:
      if (move_tool->guide)
	gimp_display_shell_draw_guide (GIMP_DISPLAY_SHELL (gdisp->shell),
                                       move_tool->guide, TRUE);
      break;

    case HALT:
      if (move_tool->guide)
        gimp_display_shell_draw_guide (GIMP_DISPLAY_SHELL (gdisp->shell),
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
  GimpMoveTool *move;
  MoveOptions  *options;
  GimpLayer    *layer;
  GimpGuide    *guide;

  move = GIMP_MOVE_TOOL (tool);

  options = (MoveOptions *) tool->tool_info->tool_options;

  tool->gdisp = gdisp;

  move->layer = NULL;
  move->guide = NULL;
  move->disp  = NULL;

  if (options->move_mask && ! gimp_image_mask_is_empty (gdisp->gimage))
    {
      init_edit_selection (tool, gdisp, coords, EDIT_MASK_TRANSLATE);
      gimp_tool_control_activate (tool->control);
    }
  else if (options->move_current)
    {
      init_edit_selection (tool, gdisp, coords, EDIT_LAYER_TRANSLATE);
      gimp_tool_control_activate (tool->control);
    }
  else
    {
      if (gdisp->draw_guides &&
	  (guide = gimp_image_find_guide (gdisp->gimage, coords->x, coords->y)))
	{
	  undo_push_image_guide (gdisp->gimage, guide);

	  gimp_image_update_guide (gdisp->gimage, guide);
	  gimp_image_remove_guide (gdisp->gimage, guide);
	  gimp_display_flush (gdisp);
	  gimp_image_add_guide (gdisp->gimage, guide);

	  move->guide = guide;
	  move->disp  = gdisp;

	  gimp_tool_control_set_scroll_lock (tool->control, TRUE);
	  gimp_tool_control_activate (tool->control);

          gimp_draw_tool_start (GIMP_DRAW_TOOL (tool), gdisp);
	}
      else if ((layer = gimp_image_pick_correlate_layer (gdisp->gimage,
                                                         coords->x,
                                                         coords->y)))
	{
	  /*  If there is a floating selection, and this aint it,
	   *  use the move tool
	   */
	  if (gimp_image_floating_sel (gdisp->gimage) &&
	      ! gimp_layer_is_floating_sel (layer))
	    {
	      move->layer = gimp_image_floating_sel (gdisp->gimage);
	    }
	  /*  Otherwise, init the edit selection  */
	  else
	    {
	      gimp_image_set_active_layer (gdisp->gimage, layer);

              if (state & GDK_SHIFT_MASK)
                gimp_layer_set_linked (layer, ! gimp_layer_get_linked (layer));

	      init_edit_selection (tool, gdisp, coords, EDIT_LAYER_TRANSLATE);
	    }

	  gimp_tool_control_activate (tool->control);
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
  GimpMoveTool *move;
  gboolean      delete_guide;
  gint          x1, y1;
  gint          x2, y2;

  move = GIMP_MOVE_TOOL (tool);

  gimp_tool_control_halt (tool->control);    /* sets paused_count to 0 -- is this ok? */

  if (move->guide)
    {
      GimpDisplayShell *shell;

      shell = GIMP_DISPLAY_SHELL (tool->gdisp->shell);

      gimp_tool_control_set_scroll_lock (tool->control, FALSE);

      delete_guide = FALSE;
      gdisplay_untransform_coords (gdisp,
                                   0, 0,
                                   &x1, &y1,
                                   FALSE, FALSE);
      gdisplay_untransform_coords (gdisp,
                                   shell->disp_width, shell->disp_height,
				   &x2, &y2,
                                   FALSE, FALSE);

      if (x1 < 0) x1 = 0;
      if (y1 < 0) y1 = 0;
      if (x2 > gdisp->gimage->width)  x2 = gdisp->gimage->width;
      if (y2 > gdisp->gimage->height) y2 = gdisp->gimage->height;

      switch (move->guide->orientation)
	{
	case ORIENTATION_HORIZONTAL:
	  if ((move->guide->position < y1) || (move->guide->position > y2))
	    delete_guide = TRUE;
	  break;

	case ORIENTATION_VERTICAL:
	  if ((move->guide->position < x1) || (move->guide->position > x2))
	    delete_guide = TRUE;
	  break;

	default:
	  break;
	}

      gimp_image_update_guide (gdisp->gimage, move->guide);

      gimp_draw_tool_stop (GIMP_DRAW_TOOL (tool));

      if (delete_guide)
	{
	  gimp_image_delete_guide (gdisp->gimage, move->guide);
	  move->guide = NULL;
	  move->disp  = NULL;
	}

      gimp_display_shell_selection_visibility (shell, GIMP_SELECTION_RESUME);
      gimp_image_flush (gdisp->gimage);

      if (move->guide)
	gimp_display_shell_draw_guide (shell, move->guide, TRUE);
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
  GimpMoveTool *move;

  move = GIMP_MOVE_TOOL (tool);

  if (move->guide)
    {
      GimpDisplayShell *shell;
      gint              tx, ty;

      shell = GIMP_DISPLAY_SHELL (gdisp->shell);

      gimp_draw_tool_pause (GIMP_DRAW_TOOL (tool));

      gdisplay_transform_coords (gdisp,
                                 coords->x, coords->y,
                                 &tx, &ty,
                                 FALSE);

      if (tx < 0 ||
          ty < 0 ||
          tx >= shell->disp_width ||
          ty >= shell->disp_height)
	{
	  move->guide->position = -1;
	}
      else
        {
          if (move->guide->orientation == ORIENTATION_HORIZONTAL)
            move->guide->position = ROUND (coords->y);
          else
            move->guide->position = ROUND (coords->x);
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
  MoveOptions *options;

  options = (MoveOptions *) tool->tool_info->tool_options;

  if (key == GDK_CONTROL_MASK)
    {
      gimp_radio_group_set_active (GTK_RADIO_BUTTON (options->move_current_w[0]),
                                   GINT_TO_POINTER (! options->move_current));
    }
  else if (key == GDK_MOD1_MASK)
    {
      gimp_radio_group_set_active (GTK_RADIO_BUTTON (options->move_mask_w[0]),
                                   GINT_TO_POINTER (! options->move_mask));
    }
}

static void
gimp_move_tool_cursor_update (GimpTool        *tool,
                              GimpCoords      *coords,
                              GdkModifierType  state,
                              GimpDisplay     *gdisp)
{
  GimpMoveTool *move;
  MoveOptions  *options;
  GimpGuide    *guide;
  GimpLayer    *layer;

  move = GIMP_MOVE_TOOL (tool);

  options = (MoveOptions *) tool->tool_info->tool_options;

  if (options->move_mask && ! gimp_image_mask_is_empty (gdisp->gimage))
    {
      gimp_tool_set_cursor (tool, gdisp,
                            GIMP_MOUSE_CURSOR,
                            GIMP_RECT_SELECT_TOOL_CURSOR,
                            GIMP_CURSOR_MODIFIER_MOVE);
    }
  else if (options->move_current)
    {
      gimp_tool_set_cursor (tool, gdisp,
                            GIMP_MOUSE_CURSOR,
                            GIMP_MOVE_TOOL_CURSOR,
                            GIMP_CURSOR_MODIFIER_NONE);
    }
  else
    {
      if (gdisp->draw_guides &&
	  (guide = gimp_image_find_guide (gdisp->gimage, coords->x, coords->y)))
	{
	  tool->gdisp = gdisp;

	  gimp_tool_set_cursor (tool, gdisp,
                                GDK_HAND2,
                                GIMP_TOOL_CURSOR_NONE,
                                GIMP_CURSOR_MODIFIER_HAND);

	  if (!gimp_tool_control_is_active (tool->control))
	    {
	      if (move->guide)
		{
                  GimpDisplay *old_guide_gdisp;

		  old_guide_gdisp = gdisplays_check_valid (move->disp,
                                                           move->disp->gimage);

		  if (old_guide_gdisp)
                    {
                      GimpDisplayShell *shell;

                      shell = GIMP_DISPLAY_SHELL (old_guide_gdisp->shell);

                      gimp_display_shell_draw_guide (shell, move->guide, FALSE);
                    }
		}

	      gimp_display_shell_draw_guide (GIMP_DISPLAY_SHELL (gdisp->shell),
                                             guide, TRUE);

	      move->guide = guide;
	      move->disp  = gdisp;
	    }
	}
      else if ((layer = gimp_image_pick_correlate_layer (gdisp->gimage,
                                                         coords->x, coords->y)))
	{
	  /*  if there is a floating selection, and this aint it...  */
	  if (gimp_image_floating_sel (gdisp->gimage) &&
	      ! gimp_layer_is_floating_sel (layer))
	    {
	      gimp_tool_set_cursor (tool, gdisp,
                                    GIMP_MOUSE_CURSOR,
                                    GIMP_RECT_SELECT_TOOL_CURSOR,
                                    GIMP_CURSOR_MODIFIER_ANCHOR);
	    }
	  else if (layer == gimp_image_get_active_layer (gdisp->gimage))
	    {
	      gimp_tool_set_cursor (tool, gdisp,
                                    GIMP_MOUSE_CURSOR,
                                    GIMP_MOVE_TOOL_CURSOR,
                                    GIMP_CURSOR_MODIFIER_NONE);
	    }
	  else
	    {
	      gimp_tool_set_cursor (tool, gdisp,
                                    GDK_HAND2,
                                    GIMP_TOOL_CURSOR_NONE,
                                    GIMP_CURSOR_MODIFIER_HAND);
	    }
	}
      else
	{
	  gimp_tool_set_cursor (tool, gdisp,
                                GIMP_BAD_CURSOR,
                                GIMP_MOVE_TOOL_CURSOR,
                                GIMP_CURSOR_MODIFIER_NONE);
	}
    }
}

static void
gimp_move_tool_draw (GimpDrawTool *draw_tool)
{
  GimpMoveTool *move;
  GimpTool     *tool;
  GimpGuide    *guide;

  move = GIMP_MOVE_TOOL (draw_tool);
  tool = GIMP_TOOL (draw_tool);

  guide = move->guide;

  if (guide && guide->position != -1)
    {
      switch (guide->orientation)
        {
        case ORIENTATION_HORIZONTAL:
          gimp_draw_tool_draw_line (draw_tool,
                                    0, guide->position,
                                    tool->gdisp->gimage->width, guide->position,
                                    FALSE);
          break;

        case ORIENTATION_VERTICAL:
          gimp_draw_tool_draw_line (draw_tool,
                                    guide->position, 0,
                                    guide->position, tool->gdisp->gimage->height,
                                    FALSE);
          break;

        default:
          break;
        }          
    }
}

void
gimp_move_tool_start_hguide (GimpTool    *tool,
			     GimpDisplay *gdisp)
{
  gimp_move_tool_start_guide (tool, gdisp, HORIZONTAL);
}

void
gimp_move_tool_start_vguide (GimpTool    *tool,
			     GimpDisplay *gdisp)
{
  gimp_move_tool_start_guide (tool, gdisp, VERTICAL);
}

static void
gimp_move_tool_start_guide (GimpTool        *tool,
                            GimpDisplay     *gdisp,
                            OrientationType  orientation)
{
  GimpMoveTool *move;

  g_return_if_fail (GIMP_IS_MOVE_TOOL (tool));
  g_return_if_fail (GIMP_IS_DISPLAY (gdisp));

  move = GIMP_MOVE_TOOL (tool);

  gimp_display_shell_selection_visibility (GIMP_DISPLAY_SHELL (gdisp->shell),
                                           GIMP_SELECTION_PAUSE);

  tool->gdisp       = gdisp;
  gimp_tool_control_activate (tool->control);
  gimp_tool_control_set_scroll_lock (tool->control, TRUE);

  if (move->guide && move->disp && move->disp->gimage)
    gimp_display_shell_draw_guide (GIMP_DISPLAY_SHELL (move->disp->shell),
                                   move->guide, FALSE);

  switch (orientation)
    {
    case HORIZONTAL:
      move->guide = gimp_image_add_hguide (gdisp->gimage);
      break;

    case VERTICAL:
      move->guide = gimp_image_add_vguide (gdisp->gimage);
      break;

    default:
      g_assert_not_reached ();
    }

  move->disp = gdisp;

  undo_push_image_guide (gdisp->gimage, move->guide);

  gimp_tool_set_cursor (tool, gdisp,
                        GDK_HAND2,
                        GIMP_TOOL_CURSOR_NONE,
                        GIMP_CURSOR_MODIFIER_HAND);

  gimp_draw_tool_start (GIMP_DRAW_TOOL (tool), gdisp);
}


/*  tool options stuff  */

static GimpToolOptions *
move_options_new (GimpToolInfo *tool_info)
{
  MoveOptions *options;
  GtkWidget   *vbox;
  GtkWidget   *frame;
 
  options = g_new0 (MoveOptions, 1);

  tool_options_init ((GimpToolOptions *) options, tool_info);

  ((GimpToolOptions *) options)->reset_func = move_options_reset;

  options->move_current = options->move_current_d = FALSE;
  options->move_mask    = options->move_mask_d    = FALSE;

  /*  the main vbox  */
  vbox = options->tool_options.main_vbox;

  /*  tool toggle  */
  frame = gimp_radio_group_new2 (TRUE, _("Tool Toggle (<Ctrl>)"),
                                 G_CALLBACK (gimp_radio_button_update),
                                 &options->move_current,
                                 GINT_TO_POINTER (options->move_current),

                                 _("Pick a Layer to Move"),
                                 GINT_TO_POINTER (FALSE),
                                 &options->move_current_w[0],

                                 _("Move Current Layer"),
                                 GINT_TO_POINTER (TRUE),
                                 &options->move_current_w[1],

                                 NULL);

  gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  /*  move mask  */
  frame = gimp_radio_group_new2 (TRUE, _("Move Mode (<Alt>)"),
                                 G_CALLBACK (gimp_radio_button_update),
                                 &options->move_mask,
                                 GINT_TO_POINTER (options->move_mask),

                                 _("Move Pixels"),
                                 GINT_TO_POINTER (FALSE),
                                 &options->move_mask_w[0],

                                 _("Move Selection Outline"),
                                 GINT_TO_POINTER (TRUE),
                                 &options->move_mask_w[1],

                                 NULL);

  gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  return (GimpToolOptions *) options;
}

static void
move_options_reset (GimpToolOptions *tool_options)
{
  MoveOptions *options;

  options = (MoveOptions *) tool_options;

  gimp_radio_group_set_active (GTK_RADIO_BUTTON (options->move_current_w[0]),
                               GINT_TO_POINTER (options->move_current_d));

  gimp_radio_group_set_active (GTK_RADIO_BUTTON (options->move_mask_w[0]),
                               GINT_TO_POINTER (options->move_mask_d));
}
