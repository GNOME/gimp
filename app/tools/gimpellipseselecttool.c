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
#include "core/gimpchannel-select.h"
#include "core/gimpimage.h"
#include "core/gimptoolinfo.h"

#include "widgets/gimphelp-ids.h"

#include "display/gimpdisplay.h"

#include "gimpellipseselecttool.h"
#include "gimpselectionoptions.h"
#include "gimptoolcontrol.h"

#include "gimp-intl.h"


static void   gimp_ellipse_select_tool_class_init (GimpEllipseSelectToolClass *klass);
static void   gimp_ellipse_select_tool_init       (GimpEllipseSelectTool      *ellipse_select);

static void   gimp_ellipse_select_tool_draw        (GimpDrawTool       *draw_tool);

static void   gimp_ellipse_select_tool_rect_select (GimpRectSelectTool *rect_tool,
                                                    gint                x,
                                                    gint                y,
                                                    gint                w,
                                                    gint                h);


static GimpRectSelectToolClass *parent_class = NULL;


/*  public functions  */

void
gimp_ellipse_select_tool_register (GimpToolRegisterCallback  callback,
                                   gpointer                  data)
{
  (* callback) (GIMP_TYPE_ELLIPSE_SELECT_TOOL,
                GIMP_TYPE_SELECTION_OPTIONS,
                gimp_selection_options_gui,
                0,
                "gimp-ellipse-select-tool",
                _("Ellipse Select"),
                _("Select elliptical regions"),
                N_("_Ellipse Select"), "E",
                NULL, GIMP_HELP_TOOL_ELLIPSE_SELECT,
                GIMP_STOCK_TOOL_ELLIPSE_SELECT,
                data);
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
  GimpTool *tool = GIMP_TOOL (ellipse_select);

  gimp_tool_control_set_tool_cursor (tool->control,
                                     GIMP_TOOL_CURSOR_ELLIPSE_SELECT);
}

static void
gimp_ellipse_select_tool_draw (GimpDrawTool *draw_tool)
{
  GimpRectSelectTool *rect_sel = GIMP_RECT_SELECT_TOOL (draw_tool);

  gimp_draw_tool_draw_arc (draw_tool,
                           FALSE,
                           rect_sel->x, rect_sel->y,
                           rect_sel->w, rect_sel->h,
                           0, 23040,
                           FALSE);
}

static void
gimp_ellipse_select_tool_rect_select (GimpRectSelectTool *rect_tool,
                                      gint                x,
                                      gint                y,
                                      gint                w,
                                      gint                h)
{
  GimpTool             *tool;
  GimpSelectionTool    *sel_tool;
  GimpSelectionOptions *options;

  tool     = GIMP_TOOL (rect_tool);
  sel_tool = GIMP_SELECTION_TOOL (rect_tool);
  options  = GIMP_SELECTION_OPTIONS (tool->tool_info->tool_options);

  gimp_channel_select_ellipse (gimp_image_get_mask (tool->gdisp->gimage),
                               x, y, w, h,
                               sel_tool->op,
                               options->antialias,
                               options->feather,
                               options->feather_radius,
                               options->feather_radius);
}
