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

#include "core/gimpchannel.h"
#include "core/gimpimage.h"
#include "core/gimpimage-mask.h"
#include "core/gimpimage-mask-select.h"
#include "core/gimptoolinfo.h"

#include "display/gimpdisplay.h"

#include "gimpellipseselecttool.h"
#include "selection_options.h"
#include "tool_options.h"
#include "tool_manager.h"

#include "app_procs.h"

#include "libgimp/gimpintl.h"


static void   gimp_ellipse_select_tool_class_init (GimpEllipseSelectToolClass *klass);
static void   gimp_ellipse_select_tool_init       (GimpEllipseSelectTool      *ellipse_select);

static void   gimp_ellipse_select_tool_draw        (GimpDrawTool       *draw_tool);

static void   gimp_ellipse_select_tool_rect_select (GimpRectSelectTool *rect_tool,
                                                    gint                x,
                                                    gint                y,
                                                    gint                w,
                                                    gint                h);


static GimpRectSelectToolClass *parent_class = NULL;

static SelectionOptions *ellipse_options = NULL;


/*  public functions  */

void
gimp_ellipse_select_tool_register (Gimp *gimp)
{
  tool_manager_register_tool (gimp,
			      GIMP_TYPE_ELLIPSE_SELECT_TOOL,
			      FALSE,
                              "gimp:ellipse_select_tool",
                              _("Ellipse Select"),
                              _("Select elliptical regions"),
                              _("/Tools/Selection Tools/Ellipse Select"), "E",
                              NULL, "tools/ellipse_select.html",
                              GIMP_STOCK_TOOL_ELLIPSE_SELECT);
}

GType
gimp_ellipse_select_tool_get_type (void)
{
  static GType tool_type = 0;

  if (! tool_type)
    {
      static const GTypeInfo tool_info =
      {
        sizeof (GimpEllipseSelectToolClass),
	(GBaseInitFunc) NULL,
	(GBaseFinalizeFunc) NULL,
	(GClassInitFunc) gimp_ellipse_select_tool_class_init,
	NULL,           /* class_finalize */
	NULL,           /* class_data     */
	sizeof (GimpEllipseSelectTool),
	0,              /* n_preallocs    */
	(GInstanceInitFunc) gimp_ellipse_select_tool_init,
      };

      tool_type = g_type_register_static (GIMP_TYPE_RECT_SELECT_TOOL,
					  "GimpEllipseSelectTool", 
                                          &tool_info, 0);
    }

  return tool_type;
}


/*  private functions  */

static void
gimp_ellipse_select_tool_class_init (GimpEllipseSelectToolClass *klass)
{
  GimpDrawToolClass       *draw_tool_class;
  GimpRectSelectToolClass *rect_tool_class;

  draw_tool_class = GIMP_DRAW_TOOL_CLASS (klass);
  rect_tool_class = GIMP_RECT_SELECT_TOOL_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);

  draw_tool_class->draw        = gimp_ellipse_select_tool_draw;

  rect_tool_class->rect_select = gimp_ellipse_select_tool_rect_select;
}

static void
gimp_ellipse_select_tool_init (GimpEllipseSelectTool *ellipse_select)
{
  GimpTool          *tool;
  GimpSelectionTool *select_tool;

  tool        = GIMP_TOOL (ellipse_select);
  select_tool = GIMP_SELECTION_TOOL (ellipse_select);

  if (! ellipse_options)
    {
      ellipse_options = selection_options_new (GIMP_TYPE_ELLIPSE_SELECT_TOOL,
					       selection_options_reset);

      tool_manager_register_tool_options (GIMP_TYPE_ELLIPSE_SELECT_TOOL,
                                          (GimpToolOptions *) ellipse_options);
    }

  tool->tool_cursor = GIMP_ELLIPSE_SELECT_TOOL_CURSOR;
  tool->preserve    = FALSE;  /*  Don't preserve on drawable change  */
}

static void
gimp_ellipse_select_tool_draw (GimpDrawTool *draw_tool)
{
  GimpTool           *tool;
  GimpRectSelectTool *rect_sel;
  gint                x1, y1;
  gint                x2, y2;

  tool     = GIMP_TOOL (draw_tool);
  rect_sel = GIMP_RECT_SELECT_TOOL (draw_tool);

  x1 = MIN (rect_sel->x, rect_sel->x + rect_sel->w);
  y1 = MIN (rect_sel->y, rect_sel->y + rect_sel->h);
  x2 = MAX (rect_sel->x, rect_sel->x + rect_sel->w);
  y2 = MAX (rect_sel->y, rect_sel->y + rect_sel->h);

  gdisplay_transform_coords (tool->gdisp, x1, y1, &x1, &y1, 0);
  gdisplay_transform_coords (tool->gdisp, x2, y2, &x2, &y2, 0);

  gdk_draw_arc (draw_tool->win,
		draw_tool->gc,
                0,
		x1, y1, (x2 - x1), (y2 - y1),
                0, 23040);
}

static void
gimp_ellipse_select_tool_rect_select (GimpRectSelectTool *rect_tool,
                                      gint                x,
                                      gint                y,
                                      gint                w,
                                      gint                h)
{
  GimpTool          *tool;
  GimpSelectionTool *sel_tool;
  SelectionOptions  *sel_options;

  tool     = GIMP_TOOL (rect_tool);
  sel_tool = GIMP_SELECTION_TOOL (rect_tool);

  sel_options = (SelectionOptions *)
    tool_manager_get_info_by_tool (the_gimp, tool)->tool_options;

  gimp_image_mask_select_ellipse (tool->gdisp->gimage,
                                  x, y, w, h,
                                  sel_tool->op,
                                  sel_options->antialias,
                                  sel_options->feather,
                                  sel_options->feather_radius,
                                  sel_options->feather_radius);
}
