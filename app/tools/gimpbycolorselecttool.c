/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ligmabycolorselecttool.c
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

#include "libligmacolor/ligmacolor.h"
#include "libligmawidgets/ligmawidgets.h"

#include "tools-types.h"

#include "core/ligma.h"
#include "core/ligmacontainer.h"
#include "core/ligmaimage.h"
#include "core/ligmaimage-new.h"
#include "core/ligmaitem.h"
#include "core/ligmapickable.h"
#include "core/ligmapickable-contiguous-region.h"

#include "widgets/ligmahelp-ids.h"

#include "display/ligmadisplay.h"

#include "ligmabycolorselecttool.h"
#include "ligmaregionselectoptions.h"
#include "ligmatoolcontrol.h"

#include "ligma-intl.h"


static GeglBuffer * ligma_by_color_select_tool_get_mask (LigmaRegionSelectTool *region_select,
                                                        LigmaDisplay          *display);


G_DEFINE_TYPE (LigmaByColorSelectTool, ligma_by_color_select_tool,
               LIGMA_TYPE_REGION_SELECT_TOOL)

#define parent_class ligma_by_color_select_tool_parent_class


void
ligma_by_color_select_tool_register (LigmaToolRegisterCallback  callback,
                                    gpointer                  data)
{
  (* callback) (LIGMA_TYPE_BY_COLOR_SELECT_TOOL,
                LIGMA_TYPE_REGION_SELECT_OPTIONS,
                ligma_region_select_options_gui,
                0,
                "ligma-by-color-select-tool",
                _("Select by Color"),
                _("Select by Color Tool: Select regions with similar colors"),
                N_("_By Color Select"), "<shift>O",
                NULL, LIGMA_HELP_TOOL_BY_COLOR_SELECT,
                LIGMA_ICON_TOOL_BY_COLOR_SELECT,
                data);
}

static void
ligma_by_color_select_tool_class_init (LigmaByColorSelectToolClass *klass)
{
  LigmaRegionSelectToolClass *region_class;

  region_class = LIGMA_REGION_SELECT_TOOL_CLASS (klass);

  region_class->undo_desc = C_("command", "Select by Color");
  region_class->get_mask  = ligma_by_color_select_tool_get_mask;
}

static void
ligma_by_color_select_tool_init (LigmaByColorSelectTool *by_color_select)
{
  LigmaTool *tool = LIGMA_TOOL (by_color_select);

  ligma_tool_control_set_tool_cursor (tool->control, LIGMA_TOOL_CURSOR_HAND);
}

static GeglBuffer *
ligma_by_color_select_tool_get_mask (LigmaRegionSelectTool *region_select,
                                    LigmaDisplay          *display)
{
  LigmaTool                *tool         = LIGMA_TOOL (region_select);
  LigmaSelectionOptions    *sel_options  = LIGMA_SELECTION_TOOL_GET_OPTIONS (tool);
  LigmaRegionSelectOptions *options      = LIGMA_REGION_SELECT_TOOL_GET_OPTIONS (tool);
  LigmaImage               *image        = ligma_display_get_image (display);
  LigmaImage               *select_image = NULL;
  GList                   *drawables    = ligma_image_get_selected_drawables (image);
  LigmaPickable            *pickable;
  GeglBuffer              *mask         = NULL;
  LigmaRGB                  srgb;
  gint                     x, y;

  x = region_select->x;
  y = region_select->y;

  if (! options->sample_merged)
    {
      if (g_list_length (drawables) == 1)
        {
          gint off_x, off_y;

          ligma_item_get_offset (drawables->data, &off_x, &off_y);

          x -= off_x;
          y -= off_y;

          pickable = LIGMA_PICKABLE (drawables->data);
        }
      else
        {
          select_image = ligma_image_new_from_drawables (image->ligma, drawables, FALSE, FALSE);
          ligma_container_remove (image->ligma->images, LIGMA_OBJECT (select_image));

          pickable = LIGMA_PICKABLE (select_image);
          ligma_pickable_flush (pickable);
        }
    }
  else
    {
      pickable = LIGMA_PICKABLE (image);
    }

  g_list_free (drawables);
  ligma_pickable_flush (pickable);

  if (ligma_pickable_get_color_at (pickable, x, y, &srgb))
    {
      LigmaRGB color;

      ligma_pickable_srgb_to_image_color (pickable, &srgb, &color);

      mask = ligma_pickable_contiguous_region_by_color (pickable,
                                                       sel_options->antialias,
                                                       options->threshold / 255.0,
                                                       options->select_transparent,
                                                       options->select_criterion,
                                                       &color);
    }

  if (select_image)
    g_object_unref (select_image);

  return mask;
}
