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

#include "core/gimpchannel-select.h"
#include "core/gimpimage.h"

#include "widgets/gimphelp-ids.h"

#include "display/gimpdisplay.h"

#include "gimpselectbyshapetool.h"
#include "gimprectangleselectoptions.h"
#include "gimptoolcontrol.h"

#include "gimp-intl.h"


static void   gimp_select_by_shape_tool_draw   (GimpDrawTool            *draw_tool);

static void   gimp_select_by_shape_tool_select (GimpRectangleSelectTool *rect_tool,
                                               GimpChannelOps           operation,
                                               gint                     x,
                                               gint                     y,
                                               gint                     w,
                                               gint                     h);


G_DEFINE_TYPE (GimpSelectByShapeTool, gimp_select_by_shape_tool,
               GIMP_TYPE_RECTANGLE_SELECT_TOOL)

#define parent_class gimp_select_by_shape_tool_parent_class


void
gimp_select_by_shape_tool_register (GimpToolRegisterCallback  callback,
                                   gpointer                  data)
{
  (* callback) (GIMP_TYPE_SELECT_BY_SHAPE_TOOL,
                GIMP_TYPE_RECTANGLE_SELECT_OPTIONS,
                gimp_rectangle_select_options_gui,
                0,
                "gimp-select-by-shape-tool",
                _("Select by Shape"),
                _("Select by Shape Tool: Selects a predefined shape"),
                N_("_Select By Shape"), "<shift>S",
                NULL, GIMP_HELP_TOOL_SELECT_BY_SHAPE,
                GIMP_STOCK_TOOL_SELECT_BY_SHAPE,
                data);
}

static void
gimp_select_by_shape_tool_class_init (GimpSelectByShapeToolClass *klass)
{
  GimpDrawToolClass            *draw_tool_class = GIMP_DRAW_TOOL_CLASS (klass);
  GimpRectangleSelectToolClass *rect_tool_class = GIMP_RECTANGLE_SELECT_TOOL_CLASS (klass);

  draw_tool_class->draw   = gimp_select_by_shape_tool_draw;

  rect_tool_class->select = gimp_select_by_shape_tool_select;
}

static void
gimp_select_by_shape_tool_init (GimpSelectByShapeTool *select_by_shape)
{
  GimpTool *tool = GIMP_TOOL (select_by_shape);

  gimp_tool_control_set_tool_cursor (tool->control,
                                     GIMP_TOOL_CURSOR_ELLIPSE_SELECT);
  //Need to add new Cursors :'(
}

static void
gimp_select_by_shape_tool_draw (GimpDrawTool *draw_tool)
{
  gint x1, y1, x2, y2;

  GIMP_DRAW_TOOL_CLASS (parent_class)->draw (draw_tool);

  g_object_get (draw_tool,
                "x1", &x1,
                "y1", &y1,
                "x2", &x2,
                "y2", &y2,
                NULL);

  gimp_draw_tool_add_arc (draw_tool,
                          FALSE,
                          x1, y1,
                          x2 - x1, y2 - y1,
                          0.0, 2 * G_PI);
}

static void
gimp_select_by_shape_tool_select (GimpRectangleSelectTool *rect_tool,
                                 GimpChannelOps           operation,
                                 gint                     x,
                                 gint                     y,
                                 gint                     w,
                                 gint                     h)
{
  GimpTool                   *tool    = GIMP_TOOL (rect_tool);
  GimpSelectionOptions       *options = GIMP_SELECTION_TOOL_GET_OPTIONS (rect_tool);
  GimpImage                  *image   = gimp_display_get_image (tool->display);
  GimpRectangleSelectOptions *sel_options     = GIMP_RECTANGLE_SELECT_TOOL_GET_OPTIONS (tool);

  if(! sel_options->shape_options)
   {
     gimp_channel_select_ellipse (gimp_image_get_mask (image),
                               x, y, w, h,
                               operation,
                               options->antialias,
                               options->feather,
                               options->feather_radius,
                               options->feather_radius,
                               TRUE);
   }
   else 
   {   
     gimp_channel_select_rectangle (gimp_image_get_mask (image),
                                     x, y, w, h,
                                     operation,
                                     options->feather,
                                     options->feather_radius,
                                     options->feather_radius,
                                     TRUE);
   }  
}
