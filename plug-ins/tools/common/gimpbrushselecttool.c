/* The GIMP -- an image manipulation program
 * Copyright (C) 1995-2002 Spencer Kimball, Peter Mattis, and others
 *
 * Brush selection tool by Nathan Summers
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

#include <gmodule.h>
#include <gtk/gtk.h>

#include "libgimpwidgets/gimpwidgets.h"

#include "libgimpmath/gimpmath.h"

#include "tools-types.h"

#include "core/gimpchannel.h"
#include "core/gimpimage.h"
#include "core/gimpimage-mask.h"
#include "core/gimpimage-mask-select.h"
#include "core/gimplayer-floating-sel.h"
#include "core/gimptoolinfo.h"

#include "display/gimpdisplay.h"
#include "display/gimpdisplay-foreach.h"

#include "gimpeditselectiontool.h"
#include "gimpbrushselecttool.h"
#include "selection_options.h"

#include "libgimp/gimpintl.h"


#define DEFAULT_MAX_INC 1024


static void   gimp_brush_select_tool_class_init (GimpBrushSelectToolClass *klass);
static void   gimp_brush_select_tool_init       (GimpBrushSelectTool      *brush_select);
static void   gimp_brush_select_tool_finalize       (GObject         *object);

static void   gimp_brush_select_tool_button_press   (GimpTool        *tool,
                                                    GimpCoords      *coords,
                                                    guint32          time,
                                                    GdkModifierType  state,
                                                    GimpDisplay     *gdisp);
static void   gimp_brush_select_tool_button_release (GimpTool        *tool,
                                                    GimpCoords      *coords,
                                                    guint32          time,
                                                    GdkModifierType  state,
                                                    GimpDisplay     *gdisp);
static void   gimp_brush_select_tool_motion         (GimpTool        *tool,
                                                    GimpCoords      *coords,
                                                    guint32          time,
                                                    GdkModifierType  state,
                                                    GimpDisplay     *gdisp);

static void   gimp_brush_select_tool_draw           (GimpDrawTool    *draw_tool);

static void   gimp_brush_select_tool_add_point      (GimpBrushSelectTool *free_sel,
                                                    gdouble             x,
                                                    gdouble             y);


static GimpSelectionToolClass *parent_class = NULL;
GType gimp_type_brush_select_tool;


/*  public functions  */

G_MODULE_EXPORT gboolean
gimp_tool_module_register_type (GTypeModule *module)
{
  g_message("registering brush selection tool type.\n");

  gimp_brush_select_tool_get_type(module);
  return 1;
}

G_MODULE_EXPORT void
gimp_tool_module_register_tool (Gimp                     *gimp,
                                GimpToolRegisterCallback  callback)
{
  g_message("registering brush selection tool info.\n");
  
  (* callback) (gimp,
                GIMP_TYPE_BRUSH_SELECT_TOOL,
                selection_options_new,
                FALSE,
                "gimp-brush-select-tool-module",
                _("Brush Select"),
                _("Select hand-painted regions"),
                _("/Tools/Selection Tools/Brush Select"), "B",
                NULL, "tools/brush_select.html",
                /* danger will robinson! */
                GIMP_STOCK_TOOL_FREE_SELECT);
}

GType
gimp_brush_select_tool_get_type (GTypeModule *module)
{
  static const GTypeInfo tool_info =
  {
    sizeof (GimpBrushSelectToolClass),
    (GBaseInitFunc) NULL,
    (GBaseFinalizeFunc) NULL,
    (GClassInitFunc) gimp_brush_select_tool_class_init,
    NULL,           /* class_finalize */
    NULL,           /* class_data     */
    sizeof (GimpBrushSelectTool),
    0,              /* n_preallocs    */
    (GInstanceInitFunc) gimp_brush_select_tool_init,
  };

  gimp_type_brush_select_tool = g_type_module_register_type (module,
 	 		 	           		     GIMP_TYPE_SELECTION_TOOL,
	                                    		     "GimpBrushSelectTool", 
                                           		     &tool_info, 0);

  return gimp_type_brush_select_tool;
}

/*  private functions  */

static void
gimp_brush_select_tool_class_init (GimpBrushSelectToolClass *klass)
{
  GObjectClass      *object_class;
  GimpToolClass     *tool_class;
  GimpDrawToolClass *draw_tool_class;

  object_class    = G_OBJECT_CLASS (klass);
  tool_class      = GIMP_TOOL_CLASS (klass);
  draw_tool_class = GIMP_DRAW_TOOL_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);

  object_class->finalize     = gimp_brush_select_tool_finalize;

  tool_class->button_press   = gimp_brush_select_tool_button_press;
  tool_class->button_release = gimp_brush_select_tool_button_release;
  tool_class->motion         = gimp_brush_select_tool_motion;

  draw_tool_class->draw      = gimp_brush_select_tool_draw;
}

static void
gimp_brush_select_tool_init (GimpBrushSelectTool *brush_select)
{
  GimpTool          *tool;
  GimpSelectionTool *select_tool;

  tool        = GIMP_TOOL (brush_select);
  select_tool = GIMP_SELECTION_TOOL (brush_select);

  tool->tool_cursor = GIMP_FREE_SELECT_TOOL_CURSOR;
  tool->scroll_lock = TRUE;   /*  Do not allow scrolling  */

  brush_select->points     = NULL;
  brush_select->num_points = 0;
  brush_select->max_segs   = 0;
}

static void
gimp_brush_select_tool_finalize (GObject *object)
{
  GimpBrushSelectTool *free_sel;

  free_sel = GIMP_BRUSH_SELECT_TOOL (object);

  if (free_sel->points)
    {
      g_free (free_sel->points);
      free_sel->points = NULL;
    }

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
gimp_brush_select_tool_button_press (GimpTool        *tool,
                                    GimpCoords      *coords,
                                    guint32          time,
                                    GdkModifierType  state,
                                    GimpDisplay     *gdisp)
{
  GimpBrushSelectTool *free_sel;

  free_sel = GIMP_BRUSH_SELECT_TOOL (tool);

  tool->state = ACTIVE;
  tool->gdisp = gdisp;

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

  gimp_brush_select_tool_add_point (free_sel, coords->x, coords->y);

  gimp_draw_tool_start (GIMP_DRAW_TOOL (tool), gdisp);
}

static void
gimp_brush_select_tool_button_release (GimpTool        *tool,
                                      GimpCoords      *coords,
                                      guint32          time,
                                      GdkModifierType  state,
                                      GimpDisplay     *gdisp)
{
  GimpBrushSelectTool *free_sel;
  SelectionOptions   *sel_options;

  free_sel = GIMP_BRUSH_SELECT_TOOL (tool);

  sel_options = (SelectionOptions *) tool->tool_info->tool_options;

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
gimp_brush_select_tool_motion (GimpTool        *tool,
                              GimpCoords      *coords,
                              guint32          time,
                              GdkModifierType  state,
                              GimpDisplay     *gdisp)
{
  GimpBrushSelectTool *free_sel;
  GimpSelectionTool  *sel_tool;

  free_sel  = GIMP_BRUSH_SELECT_TOOL (tool);
  sel_tool  = GIMP_SELECTION_TOOL (tool);

  if (tool->state != ACTIVE)
    return;

  if (sel_tool->op == SELECTION_ANCHOR)
    {
      sel_tool->op = SELECTION_REPLACE;

      gimp_tool_cursor_update (tool, coords, state, gdisp);
    }

  gimp_brush_select_tool_add_point (free_sel, coords->x, coords->y);

  gimp_draw_tool_draw_line (GIMP_DRAW_TOOL (tool),
                            free_sel->points[free_sel->num_points - 2].x,
                            free_sel->points[free_sel->num_points - 2].y,
                            free_sel->points[free_sel->num_points - 1].x,
                            free_sel->points[free_sel->num_points - 1].y,
                            FALSE);
}

static void
gimp_brush_select_tool_draw (GimpDrawTool *draw_tool)
{
  GimpBrushSelectTool *free_sel;
  gint                i;

  free_sel = GIMP_BRUSH_SELECT_TOOL (draw_tool);

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
gimp_brush_select_tool_add_point (GimpBrushSelectTool *free_sel,
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
