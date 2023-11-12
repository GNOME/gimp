/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpbycolorselecttool.c
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

#include <string.h>

#include <gegl.h>
#include <gtk/gtk.h>

#include "libgimpcolor/gimpcolor.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "tools-types.h"

#include "core/gimp.h"
#include "core/gimpcontainer.h"
#include "core/gimpimage.h"
#include "core/gimpimage-new.h"
#include "core/gimpitem.h"
#include "core/gimppickable.h"
#include "core/gimppickable-contiguous-region.h"

#include "widgets/gimphelp-ids.h"

#include "display/gimpdisplay.h"

#include "gimpbycolorselecttool.h"
#include "gimpregionselectoptions.h"
#include "gimptoolcontrol.h"

#include "gimp-intl.h"


static GeglBuffer * gimp_by_color_select_tool_get_mask (GimpRegionSelectTool *region_select,
                                                        GimpDisplay          *display);


G_DEFINE_TYPE (GimpByColorSelectTool, gimp_by_color_select_tool,
               GIMP_TYPE_REGION_SELECT_TOOL)

#define parent_class gimp_by_color_select_tool_parent_class


void
gimp_by_color_select_tool_register (GimpToolRegisterCallback  callback,
                                    gpointer                  data)
{
  (* callback) (GIMP_TYPE_BY_COLOR_SELECT_TOOL,
                GIMP_TYPE_REGION_SELECT_OPTIONS,
                gimp_region_select_options_gui,
                0,
                "gimp-by-color-select-tool",
                _("Select by Color"),
                _("Select by Color Tool: Select regions with similar colors"),
                N_("_By Color Select"), "<shift>O",
                NULL, GIMP_HELP_TOOL_BY_COLOR_SELECT,
                GIMP_ICON_TOOL_BY_COLOR_SELECT,
                data);
}

static void
gimp_by_color_select_tool_class_init (GimpByColorSelectToolClass *klass)
{
  GimpRegionSelectToolClass *region_class;

  region_class = GIMP_REGION_SELECT_TOOL_CLASS (klass);

  region_class->undo_desc = C_("command", "Select by Color");
  region_class->get_mask  = gimp_by_color_select_tool_get_mask;
}

static void
gimp_by_color_select_tool_init (GimpByColorSelectTool *by_color_select)
{
  GimpTool *tool = GIMP_TOOL (by_color_select);

  gimp_tool_control_set_tool_cursor (tool->control, GIMP_TOOL_CURSOR_HAND);
}

static GeglBuffer *
gimp_by_color_select_tool_get_mask (GimpRegionSelectTool *region_select,
                                    GimpDisplay          *display)
{
  GimpTool                *tool         = GIMP_TOOL (region_select);
  GimpSelectionOptions    *sel_options  = GIMP_SELECTION_TOOL_GET_OPTIONS (tool);
  GimpRegionSelectOptions *options      = GIMP_REGION_SELECT_TOOL_GET_OPTIONS (tool);
  GimpImage               *image        = gimp_display_get_image (display);
  GimpImage               *select_image = NULL;
  GList                   *drawables    = gimp_image_get_selected_drawables (image);
  GimpPickable            *pickable;
  GeglBuffer              *mask         = NULL;
  GeglColor               *color;
  gint                     x, y;

  x = region_select->x;
  y = region_select->y;

  if (! options->sample_merged)
    {
      if (g_list_length (drawables) == 1)
        {
          gint off_x, off_y;

          gimp_item_get_offset (drawables->data, &off_x, &off_y);

          x -= off_x;
          y -= off_y;

          pickable = GIMP_PICKABLE (drawables->data);
        }
      else
        {
          select_image = gimp_image_new_from_drawables (image->gimp, drawables, FALSE, FALSE);
          gimp_container_remove (image->gimp->images, GIMP_OBJECT (select_image));

          pickable = GIMP_PICKABLE (select_image);
          gimp_pickable_flush (pickable);
        }
    }
  else
    {
      pickable = GIMP_PICKABLE (image);
    }

  g_list_free (drawables);
  gimp_pickable_flush (pickable);

  if ((color = gimp_pickable_get_color_at (pickable, x, y)) != NULL)
    {
      mask = gimp_pickable_contiguous_region_by_color (pickable,
                                                       sel_options->antialias,
                                                       options->threshold / 255.0,
                                                       options->select_transparent,
                                                       options->select_criterion,
                                                       color);
      g_object_unref (color);
    }

  if (select_image)
    g_object_unref (select_image);

  return mask;
}
