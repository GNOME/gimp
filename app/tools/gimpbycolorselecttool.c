/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpbycolorselecttool.c
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

#include "libgimpcolor/gimpcolor.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "tools-types.h"

#include "core/gimpimage.h"
#include "core/gimpimage-contiguous-region.h"
#include "core/gimpitem.h"
#include "core/gimppickable.h"

#include "widgets/gimphelp-ids.h"

#include "display/gimpdisplay.h"

#include "gimpbycolorselecttool.h"
#include "gimpselectionoptions.h"
#include "gimptoolcontrol.h"

#include "gimp-intl.h"


static GimpChannel * gimp_by_color_select_tool_get_mask (GimpRegionSelectTool *region_select,
                                                         GimpDisplay          *display);


G_DEFINE_TYPE (GimpByColorSelectTool, gimp_by_color_select_tool,
               GIMP_TYPE_REGION_SELECT_TOOL)

#define parent_class gimp_by_color_select_tool_parent_class


void
gimp_by_color_select_tool_register (GimpToolRegisterCallback  callback,
                                    gpointer                  data)
{
  (* callback) (GIMP_TYPE_BY_COLOR_SELECT_TOOL,
                GIMP_TYPE_SELECTION_OPTIONS,
                gimp_selection_options_gui,
                0,
                "gimp-by-color-select-tool",
                _("Select by Color"),
                _("Select by Color Tool: Select regions with similar colors"),
                N_("_By Color Select"), "<shift>O",
                NULL, GIMP_HELP_TOOL_BY_COLOR_SELECT,
                GIMP_STOCK_TOOL_BY_COLOR_SELECT,
                data);
}

static void
gimp_by_color_select_tool_class_init (GimpByColorSelectToolClass *klass)
{
  GimpRegionSelectToolClass *region_class;

  region_class = GIMP_REGION_SELECT_TOOL_CLASS (klass);

  region_class->undo_desc = Q_("command|Select by Color");
  region_class->get_mask  = gimp_by_color_select_tool_get_mask;
}

static void
gimp_by_color_select_tool_init (GimpByColorSelectTool *by_color_select)
{
  GimpTool *tool = GIMP_TOOL (by_color_select);

  gimp_tool_control_set_tool_cursor (tool->control, GIMP_TOOL_CURSOR_HAND);
}

static GimpChannel *
gimp_by_color_select_tool_get_mask (GimpRegionSelectTool *region_select,
                                    GimpDisplay          *display)
{
  GimpTool              *tool    = GIMP_TOOL (region_select);
  GimpSelectionOptions  *options = GIMP_SELECTION_TOOL_GET_OPTIONS (tool);
  GimpDrawable          *drawable;
  GimpPickable          *pickable;
  GimpRGB                color;
  gint                   x, y;

  drawable = gimp_image_active_drawable (display->image);

  x = region_select->x;
  y = region_select->y;

  if (! options->sample_merged)
    {
      gint off_x, off_y;

      gimp_item_offsets (GIMP_ITEM (drawable), &off_x, &off_y);

      x -= off_x;
      y -= off_y;

      pickable = GIMP_PICKABLE (drawable);
    }
  else
    {
      pickable = GIMP_PICKABLE (display->image->projection);
    }

  gimp_pickable_flush (pickable);

  if (gimp_pickable_get_color_at (pickable, x, y, &color))
    return gimp_image_contiguous_region_by_color (display->image, drawable,
                                                  options->sample_merged,
                                                  options->antialias,
                                                  options->threshold,
                                                  options->select_transparent,
                                                  options->select_criterion,
                                                  &color);
  else
    return NULL;
}
