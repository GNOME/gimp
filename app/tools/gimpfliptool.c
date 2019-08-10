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

#include <string.h>

#include <gegl.h>
#include <gtk/gtk.h>

#include "libgimpwidgets/gimpwidgets.h"

#include "tools-types.h"

#include "config/gimpdisplayconfig.h"

#include "core/gimpdrawable-transform.h"
#include "core/gimpguide.h"
#include "core/gimpimage.h"
#include "core/gimpimage-flip.h"
#include "core/gimpimage-pick-item.h"
#include "core/gimpitem-linked.h"
#include "core/gimplayer.h"
#include "core/gimplayermask.h"
#include "core/gimppickable.h"
#include "core/gimpprogress.h"

#include "widgets/gimphelp-ids.h"
#include "widgets/gimpwidgets-utils.h"

#include "display/gimpcanvasitem.h"
#include "display/gimpdisplay.h"
#include "display/gimpdisplayshell.h"
#include "display/gimpdisplayshell-appearance.h"

#include "gimpflipoptions.h"
#include "gimpfliptool.h"
#include "gimptoolcontrol.h"

#include "gimp-intl.h"


/*  local function prototypes  */

static void         gimp_flip_tool_button_press  (GimpTool             *tool,
                                                  const GimpCoords     *coords,
                                                  guint32               time,
                                                  GdkModifierType       state,
                                                  GimpButtonPressType   press_type,
                                                  GimpDisplay          *display);
static void         gimp_flip_tool_modifier_key  (GimpTool             *tool,
                                                  GdkModifierType       key,
                                                  gboolean              press,
                                                  GdkModifierType       state,
                                                  GimpDisplay          *display);
static void         gimp_flip_tool_oper_update   (GimpTool             *tool,
                                                  const GimpCoords     *coords,
                                                  GdkModifierType       state,
                                                  gboolean              proximity,
                                                  GimpDisplay          *display);
static void         gimp_flip_tool_cursor_update (GimpTool             *tool,
                                                  const GimpCoords     *coords,
                                                  GdkModifierType       state,
                                                  GimpDisplay          *display);

static void         gimp_flip_tool_draw          (GimpDrawTool         *draw_tool);

static gchar      * gimp_flip_tool_get_undo_desc (GimpTransformTool    *tr_tool);
static GeglBuffer * gimp_flip_tool_transform     (GimpTransformTool    *tr_tool,
                                                  GimpObject           *object,
                                                  GeglBuffer           *orig_buffer,
                                                  gint                  orig_offset_x,
                                                  gint                  orig_offset_y,
                                                  GimpColorProfile    **buffer_profile,
                                                  gint                 *new_offset_x,
                                                  gint                 *new_offset_y);

static GimpOrientationType   gimp_flip_tool_get_flip_type  (GimpFlipTool      *flip);


G_DEFINE_TYPE (GimpFlipTool, gimp_flip_tool, GIMP_TYPE_TRANSFORM_TOOL)

#define parent_class gimp_flip_tool_parent_class


void
gimp_flip_tool_register (GimpToolRegisterCallback  callback,
                         gpointer                  data)
{
  (* callback) (GIMP_TYPE_FLIP_TOOL,
                GIMP_TYPE_FLIP_OPTIONS,
                gimp_flip_options_gui,
                GIMP_CONTEXT_PROP_MASK_BACKGROUND,
                "gimp-flip-tool",
                _("Flip"),
                _("Flip Tool: "
                  "Reverse the layer, selection or path horizontally or vertically"),
                N_("_Flip"), "<shift>F",
                NULL, GIMP_HELP_TOOL_FLIP,
                GIMP_ICON_TOOL_FLIP,
                data);
}

static void
gimp_flip_tool_class_init (GimpFlipToolClass *klass)
{
  GimpToolClass          *tool_class      = GIMP_TOOL_CLASS (klass);
  GimpDrawToolClass      *draw_tool_class = GIMP_DRAW_TOOL_CLASS (klass);
  GimpTransformToolClass *tr_class        = GIMP_TRANSFORM_TOOL_CLASS (klass);

  tool_class->button_press  = gimp_flip_tool_button_press;
  tool_class->modifier_key  = gimp_flip_tool_modifier_key;
  tool_class->oper_update   = gimp_flip_tool_oper_update;
  tool_class->cursor_update = gimp_flip_tool_cursor_update;

  draw_tool_class->draw     = gimp_flip_tool_draw;

  tr_class->get_undo_desc   = gimp_flip_tool_get_undo_desc;
  tr_class->transform       = gimp_flip_tool_transform;

  tr_class->undo_desc       = C_("undo-type", "Flip");
  tr_class->progress_text   = _("Flipping");
}

static void
gimp_flip_tool_init (GimpFlipTool *flip_tool)
{
  GimpTool *tool = GIMP_TOOL (flip_tool);

  gimp_tool_control_set_snap_to            (tool->control, FALSE);
  gimp_tool_control_set_precision          (tool->control,
                                            GIMP_CURSOR_PRECISION_PIXEL_CENTER);
  gimp_tool_control_set_cursor             (tool->control, GIMP_CURSOR_MOUSE);
  gimp_tool_control_set_toggle_cursor      (tool->control, GIMP_CURSOR_MOUSE);
  gimp_tool_control_set_tool_cursor        (tool->control,
                                            GIMP_TOOL_CURSOR_FLIP_HORIZONTAL);
  gimp_tool_control_set_toggle_tool_cursor (tool->control,
                                            GIMP_TOOL_CURSOR_FLIP_VERTICAL);

  flip_tool->guide = NULL;
}

static void
gimp_flip_tool_button_press (GimpTool            *tool,
                             const GimpCoords    *coords,
                             guint32              time,
                             GdkModifierType      state,
                             GimpButtonPressType  press_type,
                             GimpDisplay         *display)
{
  GimpTransformTool *tr_tool = GIMP_TRANSFORM_TOOL (tool);

  tool->display = display;

  gimp_transform_tool_transform (tr_tool, display);

  gimp_tool_control (tool, GIMP_TOOL_ACTION_HALT, display);
}

static void
gimp_flip_tool_modifier_key (GimpTool        *tool,
                             GdkModifierType  key,
                             gboolean         press,
                             GdkModifierType  state,
                             GimpDisplay     *display)
{
  GimpFlipOptions *options = GIMP_FLIP_TOOL_GET_OPTIONS (tool);

  if (key == gimp_get_toggle_behavior_mask ())
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
gimp_flip_tool_oper_update (GimpTool         *tool,
                            const GimpCoords *coords,
                            GdkModifierType   state,
                            gboolean          proximity,
                            GimpDisplay      *display)
{
  GimpFlipTool     *flip      = GIMP_FLIP_TOOL (tool);
  GimpDrawTool     *draw_tool = GIMP_DRAW_TOOL (tool);
  GimpFlipOptions  *options   = GIMP_FLIP_TOOL_GET_OPTIONS (tool);
  GimpDisplayShell *shell     = gimp_display_get_shell (display);
  GimpImage        *image     = gimp_display_get_image (display);
  GimpGuide        *guide     = NULL;

  if (gimp_display_shell_get_show_guides (shell) &&
      proximity)
    {
      gint snap_distance = display->config->snap_distance;

      guide = gimp_image_pick_guide (image, coords->x, coords->y,
                                     FUNSCALEX (shell, snap_distance),
                                     FUNSCALEY (shell, snap_distance));
    }

  if (flip->guide != guide ||
      (guide && ! gimp_draw_tool_is_active (draw_tool)))
    {
      gimp_draw_tool_pause (draw_tool);

      if (gimp_draw_tool_is_active (draw_tool) &&
          draw_tool->display != display)
        gimp_draw_tool_stop (draw_tool);

      flip->guide = guide;

      if (! gimp_draw_tool_is_active (draw_tool))
        gimp_draw_tool_start (draw_tool, display);

      gimp_draw_tool_resume (draw_tool);
    }

  gtk_widget_set_sensitive (options->direction_frame, guide == NULL);

  GIMP_TOOL_CLASS (parent_class)->oper_update (tool,
                                               coords, state, proximity,
                                               display);
}

static void
gimp_flip_tool_cursor_update (GimpTool         *tool,
                              const GimpCoords *coords,
                              GdkModifierType   state,
                              GimpDisplay      *display)
{
  GimpTransformTool *tr_tool = GIMP_TRANSFORM_TOOL (tool);
  GimpFlipTool      *flip    = GIMP_FLIP_TOOL (tool);

  if (! gimp_transform_tool_check_active_object (tr_tool, display, NULL))
    {
      gimp_tool_set_cursor (tool, display,
                            gimp_tool_control_get_cursor (tool->control),
                            gimp_tool_control_get_tool_cursor (tool->control),
                            GIMP_CURSOR_MODIFIER_BAD);
      return;
    }

  gimp_tool_control_set_toggled (tool->control,
                                 gimp_flip_tool_get_flip_type (flip) ==
                                 GIMP_ORIENTATION_VERTICAL);

  GIMP_TOOL_CLASS (parent_class)->cursor_update (tool, coords, state, display);
}

static void
gimp_flip_tool_draw (GimpDrawTool *draw_tool)
{
  GimpFlipTool *flip = GIMP_FLIP_TOOL (draw_tool);

  if (flip->guide)
    {
      GimpCanvasItem *item;
      GimpGuideStyle  style;

      style = gimp_guide_get_style (flip->guide);

      item = gimp_draw_tool_add_guide (draw_tool,
                                       gimp_guide_get_orientation (flip->guide),
                                       gimp_guide_get_position (flip->guide),
                                       style);
      gimp_canvas_item_set_highlight (item, TRUE);
    }
}

static gchar *
gimp_flip_tool_get_undo_desc (GimpTransformTool *tr_tool)
{
  GimpFlipTool *flip = GIMP_FLIP_TOOL (tr_tool);

  switch (gimp_flip_tool_get_flip_type (flip))
    {
    case GIMP_ORIENTATION_HORIZONTAL:
      return g_strdup (C_("undo-type", "Flip horizontally"));

    case GIMP_ORIENTATION_VERTICAL:
      return g_strdup (C_("undo-type", "Flip vertically"));

    default:
      /* probably this is not actually reached today, but
       * could be if someone defined FLIP_DIAGONAL, say...
       */
      return GIMP_TRANSFORM_TOOL_CLASS (parent_class)->get_undo_desc (tr_tool);
    }
}

static GeglBuffer *
gimp_flip_tool_transform (GimpTransformTool *tr_tool,
                          GimpObject        *object,
                          GeglBuffer        *orig_buffer,
                          gint               orig_offset_x,
                          gint               orig_offset_y,
                          GimpColorProfile **buffer_profile,
                          gint              *new_offset_x,
                          gint              *new_offset_y)
{
  GimpFlipTool         *flip        = GIMP_FLIP_TOOL (tr_tool);
  GimpFlipOptions      *options     = GIMP_FLIP_TOOL_GET_OPTIONS (tr_tool);
  GimpTransformOptions *tr_options  = GIMP_TRANSFORM_TOOL_GET_OPTIONS (tr_tool);
  GimpContext          *context     = GIMP_CONTEXT (options);
  GimpOrientationType   flip_type   = GIMP_ORIENTATION_UNKNOWN;
  gdouble               axis        = 0.0;
  gboolean              clip_result = FALSE;
  GeglBuffer           *ret         = NULL;

  flip_type = gimp_flip_tool_get_flip_type (flip);

  if (flip->guide)
    {
      axis = gimp_guide_get_position (flip->guide);
    }
  else
    {
      switch (flip_type)
        {
        case GIMP_ORIENTATION_HORIZONTAL:
          axis = ((gdouble) tr_tool->x1 +
                  (gdouble) (tr_tool->x2 - tr_tool->x1) / 2.0);
          break;

        case GIMP_ORIENTATION_VERTICAL:
          axis = ((gdouble) tr_tool->y1 +
                  (gdouble) (tr_tool->y2 - tr_tool->y1) / 2.0);
          break;

        default:
          break;
        }
    }

  switch (tr_options->clip)
    {
    case GIMP_TRANSFORM_RESIZE_ADJUST:
      clip_result = FALSE;
      break;

    case GIMP_TRANSFORM_RESIZE_CLIP:
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

      g_return_val_if_fail (GIMP_IS_DRAWABLE (object), NULL);

      ret = gimp_drawable_transform_buffer_flip (GIMP_DRAWABLE (object),
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
  else if (GIMP_IS_ITEM (object))
    {
      /*  this happens for entire drawables, paths and layer groups  */

      GimpItem *item = GIMP_ITEM (object);

      if (gimp_item_get_linked (item))
        {
          gimp_item_linked_flip (item, context,
                                 flip_type, axis, clip_result);
        }
      else
        {
          clip_result = gimp_item_get_clip (item, clip_result);

          gimp_item_flip (item, context,
                          flip_type, axis, clip_result);
        }
    }
  else
    {
      /*  this happens for images  */
      GimpTransformToolClass *tr_class = GIMP_TRANSFORM_TOOL_GET_CLASS (tr_tool);
      GimpProgress           *progress;

      g_return_val_if_fail (GIMP_IS_IMAGE (object), NULL);

      progress = gimp_progress_start (GIMP_PROGRESS (tr_tool), FALSE,
                                      "%s", tr_class->progress_text);

      gimp_image_flip_full (GIMP_IMAGE (object), context,
                            flip_type, axis, clip_result,
                            progress);

      if (progress)
        gimp_progress_end (progress);
    }

  return ret;
}

static GimpOrientationType
gimp_flip_tool_get_flip_type (GimpFlipTool *flip)
{
  GimpFlipOptions *options = GIMP_FLIP_TOOL_GET_OPTIONS (flip);

  if (flip->guide)
    {
      switch (gimp_guide_get_orientation (flip->guide))
        {
        case GIMP_ORIENTATION_HORIZONTAL:
          return GIMP_ORIENTATION_VERTICAL;

        case GIMP_ORIENTATION_VERTICAL:
          return GIMP_ORIENTATION_HORIZONTAL;

        default:
          return gimp_guide_get_orientation (flip->guide);
        }
    }
  else
    {
      return options->flip_type;
    }
}
