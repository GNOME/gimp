/* LIGMA - The GNU Image Manipulation Program
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

#include "libligmawidgets/ligmawidgets.h"

#include "tools-types.h"

#include "core/ligmachannel-select.h"
#include "core/ligmaimage.h"

#include "widgets/ligmahelp-ids.h"

#include "display/ligmadisplay.h"

#include "ligmaellipseselecttool.h"
#include "ligmarectangleselectoptions.h"
#include "ligmatoolcontrol.h"

#include "ligma-intl.h"


static void   ligma_ellipse_select_tool_select (LigmaRectangleSelectTool *rect_tool,
                                               LigmaChannelOps           operation,
                                               gint                     x,
                                               gint                     y,
                                               gint                     w,
                                               gint                     h);


G_DEFINE_TYPE (LigmaEllipseSelectTool, ligma_ellipse_select_tool,
               LIGMA_TYPE_RECTANGLE_SELECT_TOOL)

#define parent_class ligma_ellipse_select_tool_parent_class


void
ligma_ellipse_select_tool_register (LigmaToolRegisterCallback  callback,
                                   gpointer                  data)
{
  (* callback) (LIGMA_TYPE_ELLIPSE_SELECT_TOOL,
                LIGMA_TYPE_RECTANGLE_SELECT_OPTIONS,
                ligma_rectangle_select_options_gui,
                0,
                "ligma-ellipse-select-tool",
                _("Ellipse Select"),
                _("Ellipse Select Tool: Select an elliptical region"),
                N_("_Ellipse Select"), "E",
                NULL, LIGMA_HELP_TOOL_ELLIPSE_SELECT,
                LIGMA_ICON_TOOL_ELLIPSE_SELECT,
                data);
}

static void
ligma_ellipse_select_tool_class_init (LigmaEllipseSelectToolClass *klass)
{
  LigmaRectangleSelectToolClass *rect_tool_class;

  rect_tool_class = LIGMA_RECTANGLE_SELECT_TOOL_CLASS (klass);

  rect_tool_class->select       = ligma_ellipse_select_tool_select;
  rect_tool_class->draw_ellipse = TRUE;
}

static void
ligma_ellipse_select_tool_init (LigmaEllipseSelectTool *ellipse_select)
{
  LigmaTool *tool = LIGMA_TOOL (ellipse_select);

  ligma_tool_control_set_tool_cursor (tool->control,
                                     LIGMA_TOOL_CURSOR_ELLIPSE_SELECT);
}

static void
ligma_ellipse_select_tool_select (LigmaRectangleSelectTool *rect_tool,
                                 LigmaChannelOps           operation,
                                 gint                     x,
                                 gint                     y,
                                 gint                     w,
                                 gint                     h)
{
  LigmaTool             *tool    = LIGMA_TOOL (rect_tool);
  LigmaSelectionOptions *options = LIGMA_SELECTION_TOOL_GET_OPTIONS (rect_tool);
  LigmaImage            *image   = ligma_display_get_image (tool->display);

  ligma_channel_select_ellipse (ligma_image_get_mask (image),
                               x, y, w, h,
                               operation,
                               options->antialias,
                               options->feather,
                               options->feather_radius,
                               options->feather_radius,
                               TRUE);
}
