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

#include "libgimpwidgets/gimpwidgets.h"

#include "tools-types.h"

#include "core/gimpdrawable-transform.h"
#include "core/gimpimage.h"
#include "core/gimpitem-linked.h"
#include "core/gimplayer.h"
#include "core/gimplayermask.h"
#include "core/gimppickable.h"

#include "widgets/gimphelp-ids.h"

#include "display/gimpdisplay.h"

#include "gimpflipoptions.h"
#include "gimpfliptool.h"
#include "gimptoolcontrol.h"

#include "gimp-intl.h"


/*  local function prototypes  */

static void          gimp_flip_tool_modifier_key  (GimpTool          *tool,
                                                   GdkModifierType    key,
                                                   gboolean           press,
                                                   GdkModifierType    state,
                                                   GimpDisplay       *display);
static void          gimp_flip_tool_cursor_update (GimpTool          *tool,
                                                   GimpCoords        *coords,
                                                   GdkModifierType    state,
                                                   GimpDisplay       *display);

static TileManager * gimp_flip_tool_transform     (GimpTransformTool *tool,
                                                   GimpItem          *item,
                                                   gboolean           mask_empty,
                                                   GimpDisplay       *display);


G_DEFINE_TYPE (GimpFlipTool, gimp_flip_tool, GIMP_TYPE_TRANSFORM_TOOL)

#define parent_class gimp_flip_tool_parent_class


void
gimp_flip_tool_register (GimpToolRegisterCallback  callback,
                         gpointer                  data)
{
  (* callback) (GIMP_TYPE_FLIP_TOOL,
                GIMP_TYPE_FLIP_OPTIONS,
                gimp_flip_options_gui,
                0,
                "gimp-flip-tool",
                _("Flip"),
                _("Flip Tool: "
                  "Reverse the layer, selection or path horizontally or vertically"),
                N_("_Flip"), "<shift>F",
                NULL, GIMP_HELP_TOOL_FLIP,
                GIMP_STOCK_TOOL_FLIP,
                data);
}

static void
gimp_flip_tool_class_init (GimpFlipToolClass *klass)
{
  GimpToolClass          *tool_class  = GIMP_TOOL_CLASS (klass);
  GimpTransformToolClass *trans_class = GIMP_TRANSFORM_TOOL_CLASS (klass);

  tool_class->modifier_key  = gimp_flip_tool_modifier_key;
  tool_class->cursor_update = gimp_flip_tool_cursor_update;

  trans_class->transform    = gimp_flip_tool_transform;
}

static void
gimp_flip_tool_init (GimpFlipTool *flip_tool)
{
  GimpTool *tool = GIMP_TOOL (flip_tool);
  GimpTransformTool *tr_tool = GIMP_TRANSFORM_TOOL (flip_tool);

  gimp_tool_control_set_snap_to            (tool->control, FALSE);
  gimp_tool_control_set_cursor             (tool->control, GIMP_CURSOR_MOUSE);
  gimp_tool_control_set_toggle_cursor      (tool->control, GIMP_CURSOR_MOUSE);
  gimp_tool_control_set_tool_cursor        (tool->control,
                                            GIMP_TOOL_CURSOR_FLIP_HORIZONTAL);
  gimp_tool_control_set_toggle_tool_cursor (tool->control,
                                            GIMP_TOOL_CURSOR_FLIP_VERTICAL);
  tr_tool->undo_desc = Q_("command|Flip");
}

static void
gimp_flip_tool_modifier_key (GimpTool        *tool,
                             GdkModifierType  key,
                             gboolean         press,
                             GdkModifierType  state,
                             GimpDisplay     *display)
{
  GimpFlipOptions *options = GIMP_FLIP_TOOL_GET_OPTIONS (tool);

  if (key == GDK_CONTROL_MASK)
    {
      switch (options->flip_type)
        {
        case GIMP_ORIENTATION_HORIZONTAL:
          g_object_set (options,
                        "flip-type", GIMP_ORIENTATION_VERTICAL,
                        NULL);
          break;

        case GIMP_ORIENTATION_VERTICAL:
          g_object_set (options,
                        "flip-type", GIMP_ORIENTATION_HORIZONTAL,
                        NULL);
          break;

        default:
          break;
        }
    }
}

static void
gimp_flip_tool_cursor_update (GimpTool        *tool,
                              GimpCoords      *coords,
                              GdkModifierType  state,
                              GimpDisplay     *display)
{
  GimpFlipOptions    *options  = GIMP_FLIP_TOOL_GET_OPTIONS (tool);
  GimpCursorModifier  modifier = GIMP_CURSOR_MODIFIER_BAD;

  if (gimp_image_coords_in_active_pickable (display->image, coords,
                                            FALSE, TRUE))
    {
      modifier = GIMP_CURSOR_MODIFIER_NONE;
    }

  gimp_tool_control_set_cursor_modifier        (tool->control, modifier);
  gimp_tool_control_set_toggle_cursor_modifier (tool->control, modifier);

  gimp_tool_control_set_toggled (tool->control,
                                 options->flip_type ==
                                 GIMP_ORIENTATION_VERTICAL);

  GIMP_TOOL_CLASS (parent_class)->cursor_update (tool, coords, state, display);
}

static TileManager *
gimp_flip_tool_transform (GimpTransformTool *trans_tool,
                          GimpItem          *active_item,
                          gboolean           mask_empty,
                          GimpDisplay       *display)
{
  GimpFlipOptions      *options    = GIMP_FLIP_TOOL_GET_OPTIONS (trans_tool);
  GimpTransformOptions *tr_options = GIMP_TRANSFORM_OPTIONS (options);
  GimpContext          *context    = GIMP_CONTEXT (options);
  gdouble               axis       = 0.0;
  TileManager          *ret        = NULL;

  switch (options->flip_type)
    {
    case GIMP_ORIENTATION_HORIZONTAL:
      axis = ((gdouble) trans_tool->x1 +
              (gdouble) (trans_tool->x2 - trans_tool->x1) / 2.0);
      break;

    case GIMP_ORIENTATION_VERTICAL:
      axis = ((gdouble) trans_tool->y1 +
              (gdouble) (trans_tool->y2 - trans_tool->y1) / 2.0);
      break;

    default:
      break;
    }

  if (gimp_item_get_linked (active_item))
    gimp_item_linked_flip (active_item, context, options->flip_type, axis,
                           FALSE);

  if (GIMP_IS_LAYER (active_item) &&
      gimp_layer_get_mask (GIMP_LAYER (active_item)) &&
      mask_empty)
    {
      GimpLayerMask *mask = gimp_layer_get_mask (GIMP_LAYER (active_item));

      gimp_item_flip (GIMP_ITEM (mask), context,
                      options->flip_type, axis, FALSE);
    }

  switch (tr_options->type)
    {
    case GIMP_TRANSFORM_TYPE_LAYER:
    case GIMP_TRANSFORM_TYPE_SELECTION:
      if (trans_tool->original)
        ret = gimp_drawable_transform_tiles_flip (GIMP_DRAWABLE (active_item),
                                                  context,
                                                  trans_tool->original,
                                                  options->flip_type, axis,
                                                  FALSE);
      break;

    case GIMP_TRANSFORM_TYPE_PATH:
      gimp_item_flip (active_item, context, options->flip_type, axis, FALSE);
      break;
    }

  return ret;
}
