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
#include <string.h>

#include <gtk/gtk.h>

#include "libgimpwidgets/gimpwidgets.h"

#include "libgimpmath/gimpmath.h"

#include "tools-types.h"

#include "core/gimpchannel.h"
#include "core/gimpimage.h"
#include "core/gimpimage-mask.h"
#include "core/gimpimage-mask-select.h"
#include "core/gimptoolinfo.h"

#include "display/gimpdisplay.h"
#include "display/gimpdisplay-foreach.h"
#include "display/gimpdisplayshell.h"

#include "gimpeditselectiontool.h"
#include "gimpfreeselecttool.h"
#include "selection_options.h"

#include "floating_sel.h"

#include "libgimp/gimpintl.h"


#define DEFAULT_MAX_INC 1024


static void   gimp_free_select_tool_class_init (GimpFreeSelectToolClass *klass);
static void   gimp_free_select_tool_init       (GimpFreeSelectTool      *free_select);
static void   gimp_free_select_tool_finalize       (GObject         *object);

static void   gimp_free_select_tool_button_press   (GimpTool        *tool,
                                                    GimpCoords      *coords,
                                                    guint32          time,
                                                    GdkModifierType  state,
                                                    GimpDisplay     *gdisp);
static void   gimp_free_select_tool_button_release (GimpTool        *tool,
                                                    GimpCoords      *coords,
                                                    guint32          time,
                                                    GdkModifierType  state,
                                                    GimpDisplay     *gdisp);
static void   gimp_free_select_tool_motion         (GimpTool        *tool,
                                                    GimpCoords      *coords,
                                                    guint32          time,
                                                    GdkModifierType  state,
                                                    GimpDisplay     *gdisp);

static void   gimp_free_select_tool_draw           (GimpDrawTool    *draw_tool);

static void   gimp_free_select_tool_add_point      (GimpFreeSelectTool *free_sel,
                                                    gdouble             x,
                                                    gdouble             y);


static GimpSelectionToolClass *parent_class = NULL;


/*  public functions  */

void
gimp_free_select_tool_register (Gimp                     *gimp,
                                GimpToolRegisterCallback  callback)
{
  (* callback) (gimp,
                GIMP_TYPE_FREE_SELECT_TOOL,
                selection_options_new,
                FALSE,
                "gimp:free_select_tool",
                _("Free Select"),
                _("Select hand-drawn regions"),
                _("/Tools/Selection Tools/Free Select"), "F",
                NULL, "tools/free_select.html",
                GIMP_STOCK_TOOL_FREE_SELECT);
}

GType
gimp_free_select_tool_get_type (void)
{
  static GType tool_type = 0;

  if (! tool_type)
    {
      static const GTypeInfo tool_info =
      {
        sizeof (GimpFreeSelectToolClass),
	(GBaseInitFunc) NULL,
	(GBaseFinalizeFunc) NULL,
	(GClassInitFunc) gimp_free_select_tool_class_init,
	NULL,           /* class_finalize */
	NULL,           /* class_data     */
	sizeof (GimpFreeSelectTool),
	0,              /* n_preallocs    */
	(GInstanceInitFunc) gimp_free_select_tool_init,
      };

      tool_type = g_type_register_static (GIMP_TYPE_SELECTION_TOOL,
					  "GimpFreeSelectTool", 
                                          &tool_info, 0);
    }

  return tool_type;
}

/*  private functions  */

static void
gimp_free_select_tool_class_init (GimpFreeSelectToolClass *klass)
{
  GObjectClass      *object_class;
  GimpToolClass     *tool_class;
  GimpDrawToolClass *draw_tool_class;

  object_class    = G_OBJECT_CLASS (klass);
  tool_class      = GIMP_TOOL_CLASS (klass);
  draw_tool_class = GIMP_DRAW_TOOL_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);

  object_class->finalize     = gimp_free_select_tool_finalize;

  tool_class->button_press   = gimp_free_select_tool_button_press;
  tool_class->button_release = gimp_free_select_tool_button_release;
  tool_class->motion         = gimp_free_select_tool_motion;

  draw_tool_class->draw      = gimp_free_select_tool_draw;
}

static void
gimp_free_select_tool_init (GimpFreeSelectTool *free_select)
{
  GimpTool          *tool;
  GimpSelectionTool *select_tool;

  tool        = GIMP_TOOL (free_select);
  select_tool = GIMP_SELECTION_TOOL (free_select);

  tool->tool_cursor = GIMP_FREE_SELECT_TOOL_CURSOR;
  tool->scroll_lock = TRUE;   /*  Do not allow scrolling  */

  free_select->points     = NULL;
  free_select->num_points = 0;
  free_select->max_segs   = 0;
}

static void
gimp_free_select_tool_finalize (GObject *object)
{
  GimpFreeSelectTool *free_sel;

  free_sel = GIMP_FREE_SELECT_TOOL (object);

  if (free_sel->points)
    {
      g_free (free_sel->points);
      free_sel->points = NULL;
    }

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
gimp_free_select_tool_button_press (GimpTool        *tool,
                                    GimpCoords      *coords,
                                    guint32          time,
                                    GdkModifierType  state,
                                    GimpDisplay     *gdisp)
{
  GimpFreeSelectTool *free_sel;
  GimpDisplayShell   *shell;

  free_sel = GIMP_FREE_SELECT_TOOL (tool);

  shell = GIMP_DISPLAY_SHELL (gdisp->shell);

  tool->state = ACTIVE;
  tool->gdisp = gdisp;

  gdk_pointer_grab (shell->canvas->window, FALSE,
		    GDK_POINTER_MOTION_HINT_MASK |
		    GDK_BUTTON1_MOTION_MASK |
		    GDK_BUTTON_RELEASE_MASK,
		    NULL, NULL, time);

  switch (GIMP_SELECTION_TOOL (tool)->op)
    {
    case SELECTION_MOVE_MASK:
      init_edit_selection (tool, gdisp, coords, EDIT_MASK_TRANSLATE);
      return;
    case SELECTION_MOVE:
      init_edit_selection (tool, gdisp, coords, EDIT_MASK_TO_LAYER_TRANSLATE);
      return;
    default:
      break;
    }

  free_sel->num_points = 0;

  gimp_free_select_tool_add_point (free_sel, coords->x, coords->y);

  gimp_draw_tool_start (GIMP_DRAW_TOOL (tool), gdisp);
}

static void
gimp_free_select_tool_button_release (GimpTool        *tool,
                                      GimpCoords      *coords,
                                      guint32          time,
                                      GdkModifierType  state,
                                      GimpDisplay     *gdisp)
{
  GimpFreeSelectTool *free_sel;
  SelectionOptions   *sel_options;

  free_sel = GIMP_FREE_SELECT_TOOL (tool);

  sel_options = (SelectionOptions *) tool->tool_info->tool_options;

  gdk_pointer_ungrab (time);
  gdk_flush ();

  gimp_draw_tool_stop (GIMP_DRAW_TOOL (tool));

  tool->state = INACTIVE;

  /*  First take care of the case where the user "cancels" the action  */
  if (! (state & GDK_BUTTON3_MASK))
    {
      if (free_sel->num_points == 1)
	{
	  /*  If there is a floating selection, anchor it  */
	  if (gimp_image_floating_sel (gdisp->gimage))
	    floating_sel_anchor (gimp_image_floating_sel (gdisp->gimage));
	  /*  Otherwise, clear the selection mask  */
	  else
	    gimp_image_mask_clear (gdisp->gimage);

	  gdisplays_flush ();
	  return;
	}

      gimp_image_mask_select_polygon (gdisp->gimage,
                                      free_sel->num_points,
                                      free_sel->points,
                                      GIMP_SELECTION_TOOL (tool)->op,
                                      sel_options->antialias,
                                      sel_options->feather,
                                      sel_options->feather_radius,
                                      sel_options->feather_radius);

      gdisplays_flush ();
    }
}

static void
gimp_free_select_tool_motion (GimpTool        *tool,
                              GimpCoords      *coords,
                              guint32          time,
                              GdkModifierType  state,
                              GimpDisplay     *gdisp)
{
  GimpFreeSelectTool *free_sel;
  GimpSelectionTool  *sel_tool;

  free_sel  = GIMP_FREE_SELECT_TOOL (tool);
  sel_tool  = GIMP_SELECTION_TOOL (tool);

  if (tool->state != ACTIVE)
    return;

  if (sel_tool->op == SELECTION_ANCHOR)
    {
      sel_tool->op = SELECTION_REPLACE;

      gimp_tool_cursor_update (tool, coords, state, gdisp);
    }

  gimp_free_select_tool_add_point (free_sel, coords->x, coords->y);

  gimp_draw_tool_draw_line (GIMP_DRAW_TOOL (tool),
                            free_sel->points[free_sel->num_points - 2].x,
                            free_sel->points[free_sel->num_points - 2].y,
                            free_sel->points[free_sel->num_points - 1].x,
                            free_sel->points[free_sel->num_points - 1].y,
                            FALSE);
}

static void
gimp_free_select_tool_draw (GimpDrawTool *draw_tool)
{
  GimpFreeSelectTool *free_sel;
  gint                i;

  free_sel = GIMP_FREE_SELECT_TOOL (draw_tool);

  for (i = 1; i < free_sel->num_points; i++)
    {
      gimp_draw_tool_draw_line (draw_tool,
                                free_sel->points[i - 1].x,
                                free_sel->points[i - 1].y,
                                free_sel->points[i].x,
                                free_sel->points[i].y,
                                FALSE);
    }
}

static void
gimp_free_select_tool_add_point (GimpFreeSelectTool *free_sel,
                                 gdouble             x,
                                 gdouble             y)
{
  if (free_sel->num_points >= free_sel->max_segs)
    {
      free_sel->max_segs += DEFAULT_MAX_INC;

      free_sel->points = g_realloc (free_sel->points,
                                    sizeof (GimpVector2) * free_sel->max_segs);
    }

  free_sel->points[free_sel->num_points].x = x;
  free_sel->points[free_sel->num_points].y = y;

  free_sel->num_points++;
}
