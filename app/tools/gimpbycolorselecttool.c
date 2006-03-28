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

#include <gtk/gtk.h>

#include "libgimpcolor/gimpcolor.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "tools-types.h"

#include "core/gimpchannel-select.h"
#include "core/gimpdrawable.h"
#include "core/gimpimage.h"
#include "core/gimppickable.h"
#include "core/gimptoolinfo.h"

#include "widgets/gimphelp-ids.h"

#include "display/gimpdisplay.h"

#include "gimpbycolorselecttool.h"
#include "gimpselectionoptions.h"
#include "gimptoolcontrol.h"

#include "gimp-intl.h"


static void   gimp_by_color_select_tool_button_press   (GimpTool        *tool,
                                                        GimpCoords      *coords,
                                                        guint32          time,
                                                        GdkModifierType  state,
                                                        GimpDisplay     *display);
static void   gimp_by_color_select_tool_button_release (GimpTool        *tool,
                                                        GimpCoords      *coords,
                                                        guint32          time,
                                                        GdkModifierType  state,
                                                        GimpDisplay     *display);
static void   gimp_by_color_select_tool_oper_update    (GimpTool        *tool,
                                                        GimpCoords      *coords,
                                                        GdkModifierType  state,
                                                        gboolean         proximity,
                                                        GimpDisplay     *display);
static void   gimp_by_color_select_tool_cursor_update  (GimpTool        *tool,
                                                        GimpCoords      *coords,
                                                        GdkModifierType  state,
                                                        GimpDisplay     *display);


G_DEFINE_TYPE (GimpByColorSelectTool, gimp_by_color_select_tool,
               GIMP_TYPE_SELECTION_TOOL);

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
                _("Select regions by color"),
                N_("_By Color Select"), "<shift>O",
                NULL, GIMP_HELP_TOOL_BY_COLOR_SELECT,
                GIMP_STOCK_TOOL_BY_COLOR_SELECT,
                data);
}

static void
gimp_by_color_select_tool_class_init (GimpByColorSelectToolClass *klass)
{
  GimpToolClass *tool_class = GIMP_TOOL_CLASS (klass);

  tool_class->button_press   = gimp_by_color_select_tool_button_press;
  tool_class->button_release = gimp_by_color_select_tool_button_release;
  tool_class->oper_update    = gimp_by_color_select_tool_oper_update;
  tool_class->cursor_update  = gimp_by_color_select_tool_cursor_update;
}

static void
gimp_by_color_select_tool_init (GimpByColorSelectTool *by_color_select)
{
  by_color_select->x = 0;
  by_color_select->y = 0;
}

static void
gimp_by_color_select_tool_button_press (GimpTool        *tool,
                                        GimpCoords      *coords,
                                        guint32          time,
                                        GdkModifierType  state,
                                        GimpDisplay     *display)
{
  GimpByColorSelectTool *by_color_sel = GIMP_BY_COLOR_SELECT_TOOL (tool);
  GimpSelectionOptions  *options;

  options = GIMP_SELECTION_OPTIONS (tool->tool_info->tool_options);

  tool->drawable = gimp_image_active_drawable (display->image);

  gimp_tool_control_activate (tool->control);
  tool->display = display;

  by_color_sel->x = coords->x;
  by_color_sel->y = coords->y;

  if (! options->sample_merged)
    {
      gint off_x, off_y;

      gimp_item_offsets (GIMP_ITEM (gimp_image_active_drawable (display->image)),
                         &off_x, &off_y);

      by_color_sel->x -= off_x;
      by_color_sel->y -= off_y;
    }
}

static void
gimp_by_color_select_tool_button_release (GimpTool        *tool,
                                          GimpCoords      *coords,
                                          guint32          time,
                                          GdkModifierType  state,
                                          GimpDisplay     *display)
{
  GimpByColorSelectTool *by_color_sel = GIMP_BY_COLOR_SELECT_TOOL (tool);
  GimpSelectionTool     *sel_tool     = GIMP_SELECTION_TOOL (tool);
  GimpSelectionOptions  *options;
  GimpDrawable          *drawable;

  options  = GIMP_SELECTION_OPTIONS (tool->tool_info->tool_options);
  drawable = gimp_image_active_drawable (display->image);

  gimp_tool_control_halt (tool->control);

  /*  First take care of the case where the user "cancels" the action  */
  if (state & GDK_BUTTON3_MASK)
    return;

  if (by_color_sel->x >= 0 &&
      by_color_sel->y >= 0 &&
      by_color_sel->x < gimp_item_width  (GIMP_ITEM (drawable)) &&
      by_color_sel->y < gimp_item_height (GIMP_ITEM (drawable)))
    {
      GimpPickable *pickable;
      guchar       *col;

      if (options->sample_merged)
        pickable = GIMP_PICKABLE (display->image->projection);
      else
        pickable = GIMP_PICKABLE (drawable);

      gimp_pickable_flush (pickable);

      col = gimp_pickable_get_color_at (pickable,
                                        by_color_sel->x,
                                        by_color_sel->y);

      if (col)
        {
          GimpRGB color;

          gimp_rgba_set_uchar (&color, col[0], col[1], col[2], col[3]);
          g_free (col);

          gimp_channel_select_by_color (gimp_image_get_mask (display->image),
                                        drawable,
                                        options->sample_merged,
                                        &color,
                                        options->threshold,
                                        options->select_transparent,
                                        sel_tool->op,
                                        options->antialias,
                                        options->feather,
                                        options->feather_radius,
                                        options->feather_radius);
          gimp_image_flush (display->image);
        }
    }
}

static void
gimp_by_color_select_tool_oper_update (GimpTool        *tool,
                                       GimpCoords      *coords,
                                       GdkModifierType  state,
                                       gboolean         proximity,
                                       GimpDisplay     *display)
{
  GimpSelectionTool    *sel_tool = GIMP_SELECTION_TOOL (tool);
  GimpSelectionOptions *options;

  if (gimp_tool_control_is_active (tool->control))
    return;

  options = GIMP_SELECTION_OPTIONS (tool->tool_info->tool_options);

  if ((state & GDK_CONTROL_MASK) && (state & GDK_SHIFT_MASK))
    {
      sel_tool->op = SELECTION_INTERSECT; /* intersect with selection */
    }
  else if (state & GDK_SHIFT_MASK)
    {
      sel_tool->op = SELECTION_ADD;   /* add to the selection */
    }
  else if (state & GDK_CONTROL_MASK)
    {
      sel_tool->op = SELECTION_SUBTRACT;   /* subtract from the selection */
    }
  else
    {
      sel_tool->op = options->operation;
    }
}

static void
gimp_by_color_select_tool_cursor_update (GimpTool        *tool,
                                         GimpCoords      *coords,
                                         GdkModifierType  state,
                                         GimpDisplay     *display)
{
  GimpSelectionOptions *options;
  GimpLayer            *layer;

  options = GIMP_SELECTION_OPTIONS (tool->tool_info->tool_options);

  layer = gimp_image_pick_correlate_layer (display->image, coords->x, coords->y);

  if (! options->sample_merged &&
      layer && layer != display->image->active_layer)
    {
      gimp_tool_control_set_cursor (tool->control, GIMP_CURSOR_BAD);
    }
  else
    {
      gimp_tool_control_set_cursor (tool->control, GIMP_CURSOR_MOUSE);
    }

  GIMP_TOOL_CLASS (parent_class)->cursor_update (tool, coords, state, display);
}
