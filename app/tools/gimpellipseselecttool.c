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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
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

#include "gimpellipseselecttool.h"
#include "gimprectangleselectoptions.h"
#include "gimptoolcontrol.h"

#include "gimp-intl.h"


static void   gimp_ellipse_select_tool_select (GimpRectangleSelectTool *rect_tool,
                                               GimpChannelOps           operation,
                                               gint                     x,
                                               gint                     y,
                                               gint                     w,
                                               gint                     h);


G_DEFINE_TYPE (GimpEllipseSelectTool, gimp_ellipse_select_tool,
               GIMP_TYPE_RECTANGLE_SELECT_TOOL)

#define parent_class gimp_ellipse_select_tool_parent_class


void
gimp_ellipse_select_tool_register (GimpToolRegisterCallback  callback,
                                   gpointer                  data)
{
  (* callback) (GIMP_TYPE_ELLIPSE_SELECT_TOOL,
                GIMP_TYPE_RECTANGLE_SELECT_OPTIONS,
                gimp_rectangle_select_options_gui,
                0,
                "gimp-ellipse-select-tool",
                _("Ellipse Select"),
                _("Ellipse Select Tool: Select an elliptical region"),
                N_("_Ellipse Select"), "E",
                NULL, GIMP_HELP_TOOL_ELLIPSE_SELECT,
                GIMP_ICON_TOOL_ELLIPSE_SELECT,
                data);
}

static void
gimp_ellipse_select_tool_class_init (GimpEllipseSelectToolClass *klass)
{
  GimpRectangleSelectToolClass *rect_tool_class;

  rect_tool_class = GIMP_RECTANGLE_SELECT_TOOL_CLASS (klass);

  rect_tool_class->select       = gimp_ellipse_select_tool_select;
  rect_tool_class->draw_ellipse = TRUE;
}

static void
gimp_ellipse_select_tool_init (GimpEllipseSelectTool *ellipse_select)
{
  GimpTool *tool = GIMP_TOOL (ellipse_select);

  gimp_tool_control_set_tool_cursor (tool->control,
                                     GIMP_TOOL_CURSOR_ELLIPSE_SELECT);
}

static void
gimp_ellipse_select_tool_select (GimpRectangleSelectTool *rect_tool,
                                 GimpChannelOps           operation,
                                 gint                     x,
                                 gint                     y,
                                 gint                     w,
                                 gint                     h)
{
  GimpTool             *tool    = GIMP_TOOL (rect_tool);
  GimpSelectionOptions *options = GIMP_SELECTION_TOOL_GET_OPTIONS (rect_tool);
  GimpImage            *image   = gimp_display_get_image (tool->display);

  gimp_channel_select_ellipse (gimp_image_get_mask (image),
                               x, y, w, h,
                               operation,
                               options->antialias,
                               options->feather,
                               options->feather_radius,
                               options->feather_radius,
                               TRUE);
}
