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

#include <string.h>

#include <gegl.h>
#include <gtk/gtk.h>

#include "libligmawidgets/ligmawidgets.h"

#include "tools-types.h"

#include "config/ligmadisplayconfig.h"

#include "core/ligmadrawable-transform.h"
#include "core/ligmaguide.h"
#include "core/ligmaimage.h"
#include "core/ligmaimage-flip.h"
#include "core/ligmaimage-item-list.h"
#include "core/ligmaimage-pick-item.h"
#include "core/ligmalayer.h"
#include "core/ligmalayermask.h"
#include "core/ligmapickable.h"
#include "core/ligmaprogress.h"

#include "widgets/ligmahelp-ids.h"
#include "widgets/ligmawidgets-utils.h"

#include "display/ligmacanvasitem.h"
#include "display/ligmadisplay.h"
#include "display/ligmadisplayshell.h"
#include "display/ligmadisplayshell-appearance.h"

#include "ligmaflipoptions.h"
#include "ligmafliptool.h"
#include "ligmatoolcontrol.h"

#include "ligma-intl.h"


/*  local function prototypes  */

static void         ligma_flip_tool_button_press  (LigmaTool             *tool,
                                                  const LigmaCoords     *coords,
                                                  guint32               time,
                                                  GdkModifierType       state,
                                                  LigmaButtonPressType   press_type,
                                                  LigmaDisplay          *display);
static void         ligma_flip_tool_modifier_key  (LigmaTool             *tool,
                                                  GdkModifierType       key,
                                                  gboolean              press,
                                                  GdkModifierType       state,
                                                  LigmaDisplay          *display);
static void         ligma_flip_tool_oper_update   (LigmaTool             *tool,
                                                  const LigmaCoords     *coords,
                                                  GdkModifierType       state,
                                                  gboolean              proximity,
                                                  LigmaDisplay          *display);
static void         ligma_flip_tool_cursor_update (LigmaTool             *tool,
                                                  const LigmaCoords     *coords,
                                                  GdkModifierType       state,
                                                  LigmaDisplay          *display);

static void         ligma_flip_tool_draw          (LigmaDrawTool         *draw_tool);

static gchar      * ligma_flip_tool_get_undo_desc (LigmaTransformTool    *tr_tool);
static GeglBuffer * ligma_flip_tool_transform     (LigmaTransformTool    *tr_tool,
                                                  GList                *objects,
                                                  GeglBuffer           *orig_buffer,
                                                  gint                  orig_offset_x,
                                                  gint                  orig_offset_y,
                                                  LigmaColorProfile    **buffer_profile,
                                                  gint                 *new_offset_x,
                                                  gint                 *new_offset_y);

static LigmaOrientationType   ligma_flip_tool_get_flip_type  (LigmaFlipTool      *flip);


G_DEFINE_TYPE (LigmaFlipTool, ligma_flip_tool, LIGMA_TYPE_TRANSFORM_TOOL)

#define parent_class ligma_flip_tool_parent_class


void
ligma_flip_tool_register (LigmaToolRegisterCallback  callback,
                         gpointer                  data)
{
  (* callback) (LIGMA_TYPE_FLIP_TOOL,
                LIGMA_TYPE_FLIP_OPTIONS,
                ligma_flip_options_gui,
                LIGMA_CONTEXT_PROP_MASK_BACKGROUND,
                "ligma-flip-tool",
                _("Flip"),
                _("Flip Tool: "
                  "Reverse the layer, selection or path horizontally or vertically"),
                N_("_Flip"), "<shift>F",
                NULL, LIGMA_HELP_TOOL_FLIP,
                LIGMA_ICON_TOOL_FLIP,
                data);
}

static void
ligma_flip_tool_class_init (LigmaFlipToolClass *klass)
{
  LigmaToolClass          *tool_class      = LIGMA_TOOL_CLASS (klass);
  LigmaDrawToolClass      *draw_tool_class = LIGMA_DRAW_TOOL_CLASS (klass);
  LigmaTransformToolClass *tr_class        = LIGMA_TRANSFORM_TOOL_CLASS (klass);

  tool_class->button_press  = ligma_flip_tool_button_press;
  tool_class->modifier_key  = ligma_flip_tool_modifier_key;
  tool_class->oper_update   = ligma_flip_tool_oper_update;
  tool_class->cursor_update = ligma_flip_tool_cursor_update;

  draw_tool_class->draw     = ligma_flip_tool_draw;

  tr_class->get_undo_desc   = ligma_flip_tool_get_undo_desc;
  tr_class->transform       = ligma_flip_tool_transform;

  tr_class->undo_desc       = C_("undo-type", "Flip");
  tr_class->progress_text   = _("Flipping");
}

static void
ligma_flip_tool_init (LigmaFlipTool *flip_tool)
{
  LigmaTool *tool = LIGMA_TOOL (flip_tool);

  ligma_tool_control_set_snap_to            (tool->control, FALSE);
  ligma_tool_control_set_precision          (tool->control,
                                            LIGMA_CURSOR_PRECISION_PIXEL_CENTER);
  ligma_tool_control_set_cursor             (tool->control, LIGMA_CURSOR_MOUSE);
  ligma_tool_control_set_toggle_cursor      (tool->control, LIGMA_CURSOR_MOUSE);
  ligma_tool_control_set_tool_cursor        (tool->control,
                                            LIGMA_TOOL_CURSOR_FLIP_HORIZONTAL);
  ligma_tool_control_set_toggle_tool_cursor (tool->control,
                                            LIGMA_TOOL_CURSOR_FLIP_VERTICAL);

  flip_tool->guide = NULL;
}

static void
ligma_flip_tool_button_press (LigmaTool            *tool,
                             const LigmaCoords    *coords,
                             guint32              time,
                             GdkModifierType      state,
                             LigmaButtonPressType  press_type,
                             LigmaDisplay         *display)
{
  LigmaTransformTool *tr_tool = LIGMA_TRANSFORM_TOOL (tool);

  tool->display = display;

  ligma_transform_tool_transform (tr_tool, display);

  ligma_tool_control (tool, LIGMA_TOOL_ACTION_HALT, display);
}

static void
ligma_flip_tool_modifier_key (LigmaTool        *tool,
                             GdkModifierType  key,
                             gboolean         press,
                             GdkModifierType  state,
                             LigmaDisplay     *display)
{
  LigmaFlipOptions *options = LIGMA_FLIP_TOOL_GET_OPTIONS (tool);

  if (key == ligma_get_toggle_behavior_mask ())
    {
      switch (options->flip_type)
        {
        case LIGMA_ORIENTATION_HORIZONTAL:
          g_object_set (options,
                        "flip-type", LIGMA_ORIENTATION_VERTICAL,
                        NULL);
          break;

        case LIGMA_ORIENTATION_VERTICAL:
          g_object_set (options,
                        "flip-type", LIGMA_ORIENTATION_HORIZONTAL,
                        NULL);
          break;

        default:
          break;
        }
    }
}

static void
ligma_flip_tool_oper_update (LigmaTool         *tool,
                            const LigmaCoords *coords,
                            GdkModifierType   state,
                            gboolean          proximity,
                            LigmaDisplay      *display)
{
  LigmaFlipTool     *flip      = LIGMA_FLIP_TOOL (tool);
  LigmaDrawTool     *draw_tool = LIGMA_DRAW_TOOL (tool);
  LigmaFlipOptions  *options   = LIGMA_FLIP_TOOL_GET_OPTIONS (tool);
  LigmaDisplayShell *shell     = ligma_display_get_shell (display);
  LigmaImage        *image     = ligma_display_get_image (display);
  LigmaGuide        *guide     = NULL;

  if (ligma_display_shell_get_show_guides (shell) &&
      proximity)
    {
      gint snap_distance = display->config->snap_distance;

      guide = ligma_image_pick_guide (image, coords->x, coords->y,
                                     FUNSCALEX (shell, snap_distance),
                                     FUNSCALEY (shell, snap_distance));
    }

  if (flip->guide != guide ||
      (guide && ! ligma_draw_tool_is_active (draw_tool)))
    {
      ligma_draw_tool_pause (draw_tool);

      if (ligma_draw_tool_is_active (draw_tool) &&
          draw_tool->display != display)
        ligma_draw_tool_stop (draw_tool);

      flip->guide = guide;

      if (! ligma_draw_tool_is_active (draw_tool))
        ligma_draw_tool_start (draw_tool, display);

      ligma_draw_tool_resume (draw_tool);
    }

  gtk_widget_set_sensitive (options->direction_frame, guide == NULL);

  LIGMA_TOOL_CLASS (parent_class)->oper_update (tool,
                                               coords, state, proximity,
                                               display);
}

static void
ligma_flip_tool_cursor_update (LigmaTool         *tool,
                              const LigmaCoords *coords,
                              GdkModifierType   state,
                              LigmaDisplay      *display)
{
  LigmaTransformTool *tr_tool = LIGMA_TRANSFORM_TOOL (tool);
  LigmaFlipTool      *flip    = LIGMA_FLIP_TOOL (tool);
  GList             *selected_objects;

  selected_objects = ligma_transform_tool_check_selected_objects (tr_tool, display, NULL);

  if (! selected_objects)
    {
      ligma_tool_set_cursor (tool, display,
                            ligma_tool_control_get_cursor (tool->control),
                            ligma_tool_control_get_tool_cursor (tool->control),
                            LIGMA_CURSOR_MODIFIER_BAD);
      return;
    }
  g_list_free (selected_objects);

  ligma_tool_control_set_toggled (tool->control,
                                 ligma_flip_tool_get_flip_type (flip) ==
                                 LIGMA_ORIENTATION_VERTICAL);

  LIGMA_TOOL_CLASS (parent_class)->cursor_update (tool, coords, state, display);
}

static void
ligma_flip_tool_draw (LigmaDrawTool *draw_tool)
{
  LigmaFlipTool *flip = LIGMA_FLIP_TOOL (draw_tool);

  if (flip->guide)
    {
      LigmaCanvasItem *item;
      LigmaGuideStyle  style;

      style = ligma_guide_get_style (flip->guide);

      item = ligma_draw_tool_add_guide (draw_tool,
                                       ligma_guide_get_orientation (flip->guide),
                                       ligma_guide_get_position (flip->guide),
                                       style);
      ligma_canvas_item_set_highlight (item, TRUE);
    }
}

static gchar *
ligma_flip_tool_get_undo_desc (LigmaTransformTool *tr_tool)
{
  LigmaFlipTool *flip = LIGMA_FLIP_TOOL (tr_tool);

  switch (ligma_flip_tool_get_flip_type (flip))
    {
    case LIGMA_ORIENTATION_HORIZONTAL:
      return g_strdup (C_("undo-type", "Flip horizontally"));

    case LIGMA_ORIENTATION_VERTICAL:
      return g_strdup (C_("undo-type", "Flip vertically"));

    default:
      /* probably this is not actually reached today, but
       * could be if someone defined FLIP_DIAGONAL, say...
       */
      return LIGMA_TRANSFORM_TOOL_CLASS (parent_class)->get_undo_desc (tr_tool);
    }
}

static GeglBuffer *
ligma_flip_tool_transform (LigmaTransformTool *tr_tool,
                          GList             *objects,
                          GeglBuffer        *orig_buffer,
                          gint               orig_offset_x,
                          gint               orig_offset_y,
                          LigmaColorProfile **buffer_profile,
                          gint              *new_offset_x,
                          gint              *new_offset_y)
{
  LigmaFlipTool         *flip        = LIGMA_FLIP_TOOL (tr_tool);
  LigmaFlipOptions      *options     = LIGMA_FLIP_TOOL_GET_OPTIONS (tr_tool);
  LigmaTransformOptions *tr_options  = LIGMA_TRANSFORM_TOOL_GET_OPTIONS (tr_tool);
  LigmaContext          *context     = LIGMA_CONTEXT (options);
  LigmaOrientationType   flip_type   = LIGMA_ORIENTATION_UNKNOWN;
  gdouble               axis        = 0.0;
  gboolean              clip_result = FALSE;
  GeglBuffer           *ret         = NULL;

  flip_type = ligma_flip_tool_get_flip_type (flip);

  if (flip->guide)
    {
      axis = ligma_guide_get_position (flip->guide);
    }
  else
    {
      switch (flip_type)
        {
        case LIGMA_ORIENTATION_HORIZONTAL:
          axis = ((gdouble) tr_tool->x1 +
                  (gdouble) (tr_tool->x2 - tr_tool->x1) / 2.0);
          break;

        case LIGMA_ORIENTATION_VERTICAL:
          axis = ((gdouble) tr_tool->y1 +
                  (gdouble) (tr_tool->y2 - tr_tool->y1) / 2.0);
          break;

        default:
          break;
        }
    }

  switch (tr_options->clip)
    {
    case LIGMA_TRANSFORM_RESIZE_ADJUST:
      clip_result = FALSE;
      break;

    case LIGMA_TRANSFORM_RESIZE_CLIP:
      clip_result = TRUE;
      break;

    default:
      g_return_val_if_reached (NULL);
    }

  if (orig_buffer)
    {
      /*  this happens when transforming a selection cut out of a
       *  normal drawable
       */

      g_return_val_if_fail (LIGMA_IS_DRAWABLE (objects->data), NULL);

      ret = ligma_drawable_transform_buffer_flip (LIGMA_DRAWABLE (objects->data),
                                                 context,
                                                 orig_buffer,
                                                 orig_offset_x,
                                                 orig_offset_y,
                                                 flip_type, axis,
                                                 clip_result,
                                                 buffer_profile,
                                                 new_offset_x,
                                                 new_offset_y);
    }
  else if (g_list_length (objects) == 1 && LIGMA_IS_IMAGE (objects->data))
    {
      /*  this happens for images  */
      LigmaTransformToolClass *tr_class = LIGMA_TRANSFORM_TOOL_GET_CLASS (tr_tool);
      LigmaProgress           *progress;

      progress = ligma_progress_start (LIGMA_PROGRESS (tr_tool), FALSE,
                                      "%s", tr_class->progress_text);

      ligma_image_flip_full (LIGMA_IMAGE (objects->data), context,
                            flip_type, axis, clip_result,
                            progress);

      if (progress)
        ligma_progress_end (progress);
    }
  else
    {
      /*  this happens for entire drawables, paths and layer groups  */
      ligma_image_item_list_flip (ligma_item_get_image (objects->data),
                                 objects, context,
                                 flip_type, axis, clip_result);
    }

  return ret;
}

static LigmaOrientationType
ligma_flip_tool_get_flip_type (LigmaFlipTool *flip)
{
  LigmaFlipOptions *options = LIGMA_FLIP_TOOL_GET_OPTIONS (flip);

  if (flip->guide)
    {
      switch (ligma_guide_get_orientation (flip->guide))
        {
        case LIGMA_ORIENTATION_HORIZONTAL:
          return LIGMA_ORIENTATION_VERTICAL;

        case LIGMA_ORIENTATION_VERTICAL:
          return LIGMA_ORIENTATION_HORIZONTAL;

        default:
          return ligma_guide_get_orientation (flip->guide);
        }
    }
  else
    {
      return options->flip_type;
    }
}
