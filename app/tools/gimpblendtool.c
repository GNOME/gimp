/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * Major improvements for interactivity
 * Copyright (C) 2014 Michael Henning <drawoc@darkrefraction.com>
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

#include <stdlib.h>

#include <gegl.h>
#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>

#include "libgimpmath/gimpmath.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "tools-types.h"

#include "core/gimp-utils.h"
#include "core/gimpdrawable.h"
#include "core/gimpdrawable-blend.h"
#include "core/gimperror.h"
#include "core/gimpgradient.h"
#include "core/gimpimage.h"
#include "core/gimpimagemap.h"
#include "core/gimpprogress.h"
#include "core/gimpprojection.h"

#include "gegl/gimp-gegl-config-proxy.h"

#include "widgets/gimphelp-ids.h"
#include "widgets/gimpwidgets-utils.h"

#include "display/gimpcanvashandle.h"
#include "display/gimpcanvasline.h"
#include "display/gimpdisplay.h"

#include "gimpblendoptions.h"
#include "gimpblendtool.h"
#include "gimptoolcontrol.h"

#include "gimp-intl.h"

#define HANDLE_CROSS_DIAMETER 18
#define HANDLE_DIAMETER 40

#define POINT_GRAB_THRESHOLD_SQ (SQR (HANDLE_DIAMETER / 2))
#define FULL_HANDLE_THRESHOLD_SQ (POINT_GRAB_THRESHOLD_SQ * 9)

/*  local function prototypes  */

static gboolean gimp_blend_tool_initialize        (GimpTool              *tool,
                                                   GimpDisplay           *display,
                                                   GError               **error);
static void   gimp_blend_tool_dispose             (GObject               *object);
static void   gimp_blend_tool_control             (GimpTool              *tool,
                                                   GimpToolAction         action,
                                                   GimpDisplay           *display);
static void   gimp_blend_tool_oper_update         (GimpTool              *tool,
                                                   const GimpCoords      *coords,
                                                   GdkModifierType        state,
                                                   gboolean               proximity,
                                                   GimpDisplay           *display);
static void   gimp_blend_tool_button_press        (GimpTool              *tool,
                                                   const GimpCoords      *coords,
                                                   guint32                time,
                                                   GdkModifierType        state,
                                                   GimpButtonPressType    press_type,
                                                   GimpDisplay           *display);
static void   gimp_blend_tool_button_release      (GimpTool              *tool,
                                                   const GimpCoords      *coords,
                                                   guint32                time,
                                                   GdkModifierType        state,
                                                   GimpButtonReleaseType  release_type,
                                                   GimpDisplay           *display);
static void   gimp_blend_tool_motion              (GimpTool              *tool,
                                                   const GimpCoords      *coords,
                                                   guint32                time,
                                                   GdkModifierType        state,
                                                   GimpDisplay           *display);
static void   gimp_blend_tool_point_motion        (GimpBlendTool         *blend_tool,
                                                   gboolean               constrain_angle);
static gboolean gimp_blend_tool_key_press         (GimpTool              *tool,
                                                   GdkEventKey           *kevent,
                                                   GimpDisplay           *display);
static void   gimp_blend_tool_active_modifier_key (GimpTool              *tool,
                                                   GdkModifierType        key,
                                                   gboolean               press,
                                                   GdkModifierType        state,
                                                   GimpDisplay           *display);
static void   gimp_blend_tool_cursor_update       (GimpTool              *tool,
                                                   const GimpCoords      *coords,
                                                   GdkModifierType        state,
                                                   GimpDisplay           *display);

static void   gimp_blend_tool_draw                (GimpDrawTool          *draw_tool);

static GimpBlendToolPoint gimp_blend_tool_get_point_under_cursor (GimpBlendTool *blend_tool);

static void   gimp_blend_tool_start_preview       (GimpBlendTool         *bt,
                                                   GimpDisplay           *display);
static void   gimp_blend_tool_halt_preview        (GimpBlendTool         *bt);
static void   gimp_blend_tool_commit              (GimpBlendTool         *bt);

static void   gimp_blend_tool_push_status         (GimpBlendTool         *blend_tool,
                                                   GdkModifierType        state,
                                                   GimpDisplay           *display);

static void   gimp_blend_tool_create_graph        (GimpBlendTool         *blend_tool);
static void   gimp_blend_tool_update_preview_coords (GimpBlendTool       *blend_tool);
static void   gimp_blend_tool_gradient_dirty      (GimpBlendTool         *blend_tool);
static void   gimp_blend_tool_set_gradient        (GimpBlendTool         *blend_tool,
                                                   GimpGradient          *gradient);
static void   gimp_blend_tool_options_notify      (GimpTool              *tool,
                                                   GimpToolOptions       *options,
                                                   const GParamSpec      *pspec);
static gboolean gimp_blend_tool_is_shapeburst     (GimpBlendTool         *blend_tool);

static void   gimp_blend_tool_create_image_map    (GimpBlendTool         *blend_tool,
                                                   GimpDrawable          *drawable);
static void   gimp_blend_tool_image_map_flush     (GimpImageMap          *image_map,
                                                   GimpTool              *tool);



G_DEFINE_TYPE (GimpBlendTool, gimp_blend_tool, GIMP_TYPE_DRAW_TOOL)

#define parent_class gimp_blend_tool_parent_class


void
gimp_blend_tool_register (GimpToolRegisterCallback  callback,
                          gpointer                  data)
{
  (* callback) (GIMP_TYPE_BLEND_TOOL,
                GIMP_TYPE_BLEND_OPTIONS,
                gimp_blend_options_gui,
                GIMP_CONTEXT_FOREGROUND_MASK |
                GIMP_CONTEXT_BACKGROUND_MASK |
                GIMP_CONTEXT_OPACITY_MASK    |
                GIMP_CONTEXT_PAINT_MODE_MASK |
                GIMP_CONTEXT_GRADIENT_MASK,
                "gimp-blend-tool",
                _("Blend"),
                _("Blend Tool: Fill selected area with a color gradient"),
                N_("Blen_d"), "L",
                NULL, GIMP_HELP_TOOL_BLEND,
                GIMP_STOCK_TOOL_BLEND,
                data);
}

static void
gimp_blend_tool_class_init (GimpBlendToolClass *klass)
{
  GObjectClass      *object_class    = G_OBJECT_CLASS (klass);
  GimpToolClass     *tool_class      = GIMP_TOOL_CLASS (klass);
  GimpDrawToolClass *draw_tool_class = GIMP_DRAW_TOOL_CLASS (klass);

  object_class->dispose           = gimp_blend_tool_dispose;

  tool_class->initialize          = gimp_blend_tool_initialize;
  tool_class->control             = gimp_blend_tool_control;
  tool_class->oper_update         = gimp_blend_tool_oper_update;
  tool_class->button_press        = gimp_blend_tool_button_press;
  tool_class->button_release      = gimp_blend_tool_button_release;
  tool_class->motion              = gimp_blend_tool_motion;
  tool_class->key_press           = gimp_blend_tool_key_press;
  tool_class->active_modifier_key = gimp_blend_tool_active_modifier_key;
  tool_class->cursor_update       = gimp_blend_tool_cursor_update;
  tool_class->options_notify      = gimp_blend_tool_options_notify;

  draw_tool_class->draw           = gimp_blend_tool_draw;
}

static void
gimp_blend_tool_init (GimpBlendTool *blend_tool)
{
  GimpTool *tool = GIMP_TOOL (blend_tool);

  gimp_tool_control_set_scroll_lock     (tool->control, TRUE);
  gimp_tool_control_set_precision       (tool->control,
                                         GIMP_CURSOR_PRECISION_SUBPIXEL);
  gimp_tool_control_set_tool_cursor     (tool->control,
                                         GIMP_TOOL_CURSOR_BLEND);
  gimp_tool_control_set_action_opacity  (tool->control,
                                         "context/context-opacity-set");
  gimp_tool_control_set_action_object_1 (tool->control,
                                         "context/context-gradient-select-set");
}

static gboolean
gimp_blend_tool_initialize (GimpTool     *tool,
                            GimpDisplay  *display,
                            GError      **error)
{
  GimpImage        *image    = gimp_display_get_image (display);
  GimpDrawable     *drawable = gimp_image_get_active_drawable (image);
  GimpBlendOptions *options  = GIMP_BLEND_TOOL_GET_OPTIONS (tool);

  if (! GIMP_TOOL_CLASS (parent_class)->initialize (tool, display, error))
    {
      return FALSE;
    }

  if (gimp_viewable_get_children (GIMP_VIEWABLE (drawable)))
    {
      g_set_error_literal (error, GIMP_ERROR, GIMP_FAILED,
			   _("Cannot modify the pixels of layer groups."));
      return FALSE;
    }

  if (gimp_item_is_content_locked (GIMP_ITEM (drawable)))
    {
      g_set_error_literal (error, GIMP_ERROR, GIMP_FAILED,
			   _("The active layer's pixels are locked."));
      return FALSE;
    }

  if (! gimp_item_is_visible (GIMP_ITEM (drawable)))
    {
      g_set_error_literal (error, GIMP_ERROR, GIMP_FAILED,
                           _("The active layer is not visible."));
      return FALSE;
    }

  if (! gimp_context_get_gradient (GIMP_CONTEXT (options)))
    {
      g_set_error_literal (error, GIMP_ERROR, GIMP_FAILED,
                           _("No gradient available for use with this tool."));
      return FALSE;
    }

  return TRUE;
}

static void
gimp_blend_tool_dispose (GObject *object)
{
  GimpBlendTool *blend_tool = GIMP_BLEND_TOOL (object);
  gimp_blend_tool_set_gradient (blend_tool, NULL);

  G_OBJECT_CLASS (parent_class)->dispose (object);
}

static void
gimp_blend_tool_control (GimpTool       *tool,
                         GimpToolAction  action,
                         GimpDisplay    *display)
{
  GimpBlendTool *blend_tool = GIMP_BLEND_TOOL (tool);

  switch (action)
    {
    case GIMP_TOOL_ACTION_PAUSE:
    case GIMP_TOOL_ACTION_RESUME:
      break;

    case GIMP_TOOL_ACTION_HALT:
      gimp_blend_tool_halt_preview (blend_tool);
      break;

    case GIMP_TOOL_ACTION_COMMIT:
      gimp_blend_tool_commit (blend_tool);
      break;
    }

  GIMP_TOOL_CLASS (parent_class)->control (tool, action, display);
}

static void
gimp_blend_tool_oper_update (GimpTool         *tool,
                             const GimpCoords *coords,
                             GdkModifierType   state,
                             gboolean          proximity,
                             GimpDisplay      *display)
{
  GimpBlendTool *blend_tool = GIMP_BLEND_TOOL (tool);
  GimpDrawTool  *draw_tool  = GIMP_DRAW_TOOL (tool);

  gimp_draw_tool_pause (draw_tool);
  blend_tool->mouse_x = coords->x;
  blend_tool->mouse_y = coords->y;
  gimp_draw_tool_resume (draw_tool);
}

static void
gimp_blend_tool_button_press (GimpTool            *tool,
                              const GimpCoords    *coords,
                              guint32              time,
                              GdkModifierType      state,
                              GimpButtonPressType  press_type,
                              GimpDisplay         *display)
{
  GimpBlendTool *blend_tool = GIMP_BLEND_TOOL (tool);
  GimpDrawTool  *draw_tool  = GIMP_DRAW_TOOL (tool);

  blend_tool->mouse_x = coords->x;
  blend_tool->mouse_y = coords->y;

  if (tool->display && display != tool->display)
    {
      gimp_tool_pop_status (tool, tool->display);
      gimp_blend_tool_halt_preview (blend_tool);
    }

  gimp_draw_tool_pause (draw_tool);

  blend_tool->grabbed_point = gimp_blend_tool_get_point_under_cursor (blend_tool);

  if (blend_tool->grabbed_point == POINT_NONE)
    {
      if (gimp_draw_tool_is_active (draw_tool))
        {
          gimp_tool_control (tool, GIMP_TOOL_ACTION_COMMIT, display);
          gimp_tool_control (tool, GIMP_TOOL_ACTION_HALT, display);
        }

      if (gimp_blend_tool_is_shapeburst (blend_tool))
        {
          blend_tool->grabbed_point = POINT_FILL_MODE;
        }
      else
        {
          blend_tool->grabbed_point = POINT_INIT_MODE;

          blend_tool->start_x = coords->x;
          blend_tool->start_y = coords->y;
        }
    }

  gimp_blend_tool_point_motion (blend_tool,
                                state & gimp_get_constrain_behavior_mask ());

  tool->display = display;

  gimp_draw_tool_resume (draw_tool);

  if (blend_tool->grabbed_point != POINT_FILL_MODE &&
      blend_tool->grabbed_point != POINT_INIT_MODE)
    {
      gimp_blend_tool_update_preview_coords (blend_tool);
      gimp_image_map_apply (blend_tool->image_map, NULL);
    }

  gimp_tool_control_activate (tool->control);

  gimp_blend_tool_push_status (blend_tool, state, display);
}

static void
gimp_blend_tool_button_release (GimpTool              *tool,
                                const GimpCoords      *coords,
                                guint32                time,
                                GdkModifierType        state,
                                GimpButtonReleaseType  release_type,
                                GimpDisplay           *display)
{
  GimpBlendTool    *blend_tool    = GIMP_BLEND_TOOL (tool);

  gimp_tool_pop_status (tool, display);
  /* XXX: Push a useful status message */

  gimp_tool_control_halt (tool->control);

  /* XXX: handle cancel properly */
  /* if (release_type == GIMP_BUTTON_RELEASE_CANCEL) */

  if (blend_tool->grabbed_point == POINT_INIT_MODE)
    {
      gimp_tool_control (tool, GIMP_TOOL_ACTION_HALT, display);
    }

  if (blend_tool->grabbed_point == POINT_FILL_MODE)
    {
      /* XXX: Temporary, until the handles are working properly for shapebursts */
      gimp_tool_control (tool, GIMP_TOOL_ACTION_COMMIT, display);
      gimp_tool_control (tool, GIMP_TOOL_ACTION_HALT, display);
    }

  blend_tool->grabbed_point = POINT_NONE;
}

static void
gimp_blend_tool_motion (GimpTool         *tool,
                        const GimpCoords *coords,
                        guint32           time,
                        GdkModifierType   state,
                        GimpDisplay      *display)
{
  GimpBlendTool *blend_tool = GIMP_BLEND_TOOL (tool);
  GimpDrawTool  *draw_tool  = GIMP_DRAW_TOOL (tool);
  gdouble last_x;
  gdouble last_y;

  gimp_draw_tool_pause (draw_tool);

  /* Save the mouse coordinates from last call */
  last_x = blend_tool->mouse_x;
  last_y = blend_tool->mouse_y;

  blend_tool->mouse_x = coords->x;
  blend_tool->mouse_y = coords->y;

  if (blend_tool->grabbed_point == POINT_INIT_MODE)
    {
      blend_tool->grabbed_point = POINT_END;
      gimp_blend_tool_start_preview (blend_tool, display);
    }

  /* Move the whole line if alt is pressed */
  if (state & GDK_MOD1_MASK)
    {
      gdouble dx = last_x - coords->x;
      gdouble dy = last_y - coords->y;

      blend_tool->start_x -= dx;
      blend_tool->start_y -= dy;

      blend_tool->end_x -= dx;
      blend_tool->end_y -= dy;
    }
  else
    {
      gimp_blend_tool_point_motion (blend_tool,
                                    state & gimp_get_constrain_behavior_mask ());
    }

  gimp_tool_pop_status (tool, display);
  gimp_blend_tool_push_status (blend_tool, state, display);

  gimp_draw_tool_resume (draw_tool);

  gimp_blend_tool_update_preview_coords (blend_tool);
  gimp_image_map_apply (blend_tool->image_map, NULL);
}

static void
gimp_blend_tool_point_motion (GimpBlendTool *blend_tool,
                              gboolean       constrain_angle)
{
  switch (blend_tool->grabbed_point)
    {
    case POINT_START:
      blend_tool->start_x = blend_tool->mouse_x;
      blend_tool->start_y = blend_tool->mouse_y;

      /* Restrict to multiples of 15 degrees if ctrl is pressed */
      if (constrain_angle)
        {
          gimp_constrain_line (blend_tool->end_x, blend_tool->end_y,
                               &blend_tool->start_x, &blend_tool->start_y,
                               GIMP_CONSTRAIN_LINE_15_DEGREES);
        }
      break;

    case POINT_END:
      blend_tool->end_x = blend_tool->mouse_x;
      blend_tool->end_y = blend_tool->mouse_y;

      if (constrain_angle)
        {
          gimp_constrain_line (blend_tool->start_x, blend_tool->start_y,
                               &blend_tool->end_x, &blend_tool->end_y,
                               GIMP_CONSTRAIN_LINE_15_DEGREES);
        }
      break;

    default:
      break;
    }
}

static gboolean
gimp_blend_tool_key_press (GimpTool    *tool,
                           GdkEventKey *kevent,
                           GimpDisplay *display)
{
  switch (kevent->keyval)
    {
    case GDK_KEY_BackSpace:
      return TRUE;

    case GDK_KEY_Return:
    case GDK_KEY_KP_Enter:
    case GDK_KEY_ISO_Enter:
      gimp_tool_control (tool, GIMP_TOOL_ACTION_COMMIT, display);
      /* fall thru */

    case GDK_KEY_Escape:
      gimp_tool_control (tool, GIMP_TOOL_ACTION_HALT, display);
      return TRUE;

    default:
      break;
    }

  return FALSE;
}

static void
gimp_blend_tool_active_modifier_key (GimpTool        *tool,
                                     GdkModifierType  key,
                                     gboolean         press,
                                     GdkModifierType  state,
                                     GimpDisplay     *display)
{
  GimpBlendTool *blend_tool = GIMP_BLEND_TOOL (tool);
  GimpDrawTool  *draw_tool  = GIMP_DRAW_TOOL (tool);

  if (key == gimp_get_constrain_behavior_mask ())
    {
      gimp_draw_tool_pause (draw_tool);
      gimp_blend_tool_point_motion (blend_tool, press);
      gimp_draw_tool_resume (draw_tool);

      gimp_tool_pop_status (tool, display);
      gimp_blend_tool_push_status (blend_tool, state, display);

      gimp_blend_tool_update_preview_coords (blend_tool);
      gimp_image_map_apply (blend_tool->image_map, NULL);
    }
  else if (key == GDK_MOD1_MASK)
    {
      gimp_tool_pop_status (tool, display);
      gimp_blend_tool_push_status (blend_tool, state, display);
    }
}

static void
gimp_blend_tool_cursor_update (GimpTool         *tool,
                               const GimpCoords *coords,
                               GdkModifierType   state,
                               GimpDisplay      *display)
{
  GimpBlendTool      *blend_tool = GIMP_BLEND_TOOL (tool);
  GimpImage          *image      = gimp_display_get_image (display);
  GimpDrawable       *drawable   = gimp_image_get_active_drawable (image);
  GimpCursorModifier  modifier   = GIMP_CURSOR_MODIFIER_NONE;

  blend_tool->mouse_x = coords->x;
  blend_tool->mouse_y = coords->y;

  if (gimp_viewable_get_children (GIMP_VIEWABLE (drawable)) ||
      gimp_item_is_content_locked (GIMP_ITEM (drawable))    ||
      ! gimp_item_is_visible (GIMP_ITEM (drawable)))
    {
      modifier = GIMP_CURSOR_MODIFIER_BAD;
    }
  else if (gimp_blend_tool_is_shapeburst (blend_tool))
    {
      modifier = GIMP_CURSOR_MODIFIER_PLUS;
    }
  else if (gimp_blend_tool_get_point_under_cursor (blend_tool))
    {
      modifier = GIMP_CURSOR_MODIFIER_MOVE;
    }

  gimp_tool_control_set_cursor_modifier (tool->control, modifier);

  GIMP_TOOL_CLASS (parent_class)->cursor_update (tool, coords, state, display);
}

static void
gimp_blend_tool_draw (GimpDrawTool *draw_tool)
{
  GimpBlendTool      *blend_tool = GIMP_BLEND_TOOL (draw_tool);
  GimpCanvasItem     *start_handle_cross, *end_handle_cross;
  GimpBlendToolPoint  hilight_point;
  gboolean            start_visible, end_visible;

  gimp_draw_tool_add_line (draw_tool,
                           blend_tool->start_x,
                           blend_tool->start_y,
                           blend_tool->end_x,
                           blend_tool->end_y);

  start_handle_cross =
    gimp_draw_tool_add_handle (draw_tool,
                               GIMP_HANDLE_CROSS,
                               blend_tool->start_x,
                               blend_tool->start_y,
                               HANDLE_CROSS_DIAMETER,
                               HANDLE_CROSS_DIAMETER,
                               GIMP_HANDLE_ANCHOR_CENTER);

  end_handle_cross =
    gimp_draw_tool_add_handle (draw_tool,
                               GIMP_HANDLE_CROSS,
                               blend_tool->end_x,
                               blend_tool->end_y,
                               HANDLE_CROSS_DIAMETER,
                               HANDLE_CROSS_DIAMETER,
                               GIMP_HANDLE_ANCHOR_CENTER);

  /* Calculate handle visibility */
  if (blend_tool->grabbed_point)
    {
      start_visible = FALSE;
      end_visible = FALSE;
    }
  else
    {
      gdouble dist;
      dist = gimp_draw_tool_calc_distance_square (draw_tool,
                                                  draw_tool->display,
                                                  blend_tool->mouse_x,
                                                  blend_tool->mouse_y,
                                                  blend_tool->start_x,
                                                  blend_tool->start_y);

      start_visible = dist < FULL_HANDLE_THRESHOLD_SQ;

      dist = gimp_draw_tool_calc_distance_square (draw_tool,
                                                  draw_tool->display,
                                                  blend_tool->mouse_x,
                                                  blend_tool->mouse_y,
                                                  blend_tool->end_x,
                                                  blend_tool->end_y);

      end_visible = dist < FULL_HANDLE_THRESHOLD_SQ;
    }

  /* Update hilights */
  if (blend_tool->grabbed_point)
    hilight_point = blend_tool->grabbed_point;
  else
    hilight_point = gimp_blend_tool_get_point_under_cursor (blend_tool);

  if (start_visible)
    {
      GimpCanvasItem *start_handle_circle;

      start_handle_circle =
        gimp_draw_tool_add_handle (draw_tool,
                                   GIMP_HANDLE_CIRCLE,
                                   blend_tool->start_x,
                                   blend_tool->start_y,
                                   HANDLE_DIAMETER,
                                   HANDLE_DIAMETER,
                                   GIMP_HANDLE_ANCHOR_CENTER);

      gimp_canvas_item_set_highlight (start_handle_circle,
                                      hilight_point == POINT_START);
    }

  if (end_visible)
    {
      GimpCanvasItem *end_handle_circle;

      end_handle_circle =
        gimp_draw_tool_add_handle (draw_tool,
                                   GIMP_HANDLE_CIRCLE,
                                   blend_tool->end_x,
                                   blend_tool->end_y,
                                   HANDLE_DIAMETER,
                                   HANDLE_DIAMETER,
                                   GIMP_HANDLE_ANCHOR_CENTER);

      gimp_canvas_item_set_highlight (end_handle_circle,
                                      hilight_point == POINT_END);
    }

  gimp_canvas_item_set_highlight (start_handle_cross,
                                  hilight_point == POINT_START);
  gimp_canvas_item_set_highlight (end_handle_cross,
                                  hilight_point == POINT_END);
}

static GimpBlendToolPoint
gimp_blend_tool_get_point_under_cursor (GimpBlendTool *blend_tool)
{
  GimpDrawTool *draw_tool = GIMP_DRAW_TOOL (blend_tool);
  gdouble       dist;

  if (!draw_tool->display)
    return POINT_NONE;

  /* Check the points in the reverse order of drawing */

  /* Check end point */
  dist = gimp_draw_tool_calc_distance_square (draw_tool,
                                              draw_tool->display,
                                              blend_tool->mouse_x,
                                              blend_tool->mouse_y,
                                              blend_tool->end_x,
                                              blend_tool->end_y);
  if (dist < POINT_GRAB_THRESHOLD_SQ)
    return POINT_END;

  /* Check start point */
  dist = gimp_draw_tool_calc_distance_square (draw_tool,
                                              draw_tool->display,
                                              blend_tool->mouse_x,
                                              blend_tool->mouse_y,
                                              blend_tool->start_x,
                                              blend_tool->start_y);
  if (dist < POINT_GRAB_THRESHOLD_SQ)
    return POINT_START;

  /* No point found */
  return POINT_NONE;
}

static void
gimp_blend_tool_start_preview (GimpBlendTool *blend_tool,
                               GimpDisplay   *display)
{
  GimpTool         *tool     = GIMP_TOOL (blend_tool);
  GimpImage        *image    = gimp_display_get_image (display);
  GimpDrawable     *drawable = gimp_image_get_active_drawable (image);
  GimpBlendOptions *options  = GIMP_BLEND_TOOL_GET_OPTIONS (blend_tool);
  GimpContext      *context  = GIMP_CONTEXT (options);

  tool->display  = display;
  tool->drawable = drawable;

  if (blend_tool->grabbed_point != POINT_FILL_MODE)
    {
      gimp_blend_tool_create_image_map (blend_tool, drawable);

      /* Initially sync all of the properties */
      gimp_gegl_config_proxy_sync (GIMP_OBJECT (options), blend_tool->render_node);

      /* Connect signal handlers for the gradient */
      gimp_blend_tool_set_gradient (blend_tool, context->gradient);
    }

  if (! gimp_draw_tool_is_active (GIMP_DRAW_TOOL (blend_tool)))
    gimp_draw_tool_start (GIMP_DRAW_TOOL (blend_tool), display);
}

static void
gimp_blend_tool_halt_preview (GimpBlendTool *blend_tool)
{
  GimpTool *tool = GIMP_TOOL (blend_tool);

  if (blend_tool->graph)
    {
      g_object_unref (blend_tool->graph);
      blend_tool->graph       = NULL;
      blend_tool->render_node = NULL;
    }

  if (blend_tool->image_map)
    {
      gimp_image_map_abort (blend_tool->image_map);
      g_object_unref (blend_tool->image_map);
      blend_tool->image_map = NULL;

      gimp_image_flush (gimp_display_get_image (tool->display));
    }

  tool->display  = NULL;
  tool->drawable = NULL;

  if (gimp_draw_tool_is_active (GIMP_DRAW_TOOL (blend_tool)))
    gimp_draw_tool_stop (GIMP_DRAW_TOOL (blend_tool));
}

static void
gimp_blend_tool_commit (GimpBlendTool *blend_tool)
{
  GimpTool *tool = GIMP_TOOL (blend_tool);

  GimpBlendOptions *options       = GIMP_BLEND_TOOL_GET_OPTIONS (tool);
  GimpPaintOptions *paint_options = GIMP_PAINT_OPTIONS (options);
  GimpContext      *context       = GIMP_CONTEXT (options);
  GimpImage        *image         = gimp_display_get_image (tool->display);
  GimpDrawable     *drawable      = gimp_image_get_active_drawable (image);
  GimpProgress     *progress;
  gint              off_x;
  gint              off_y;

  progress = gimp_progress_start (GIMP_PROGRESS (tool), FALSE,
                                  _("Blending"));

  gimp_item_get_offset (GIMP_ITEM (drawable), &off_x, &off_y);

  gimp_drawable_blend (drawable,
                       context,
                       gimp_context_get_gradient (context),
                       gimp_context_get_paint_mode (context),
                       options->gradient_type,
                       gimp_context_get_opacity (context),
                       options->offset,
                       paint_options->gradient_options->gradient_repeat,
                       paint_options->gradient_options->gradient_reverse,
                       options->supersample,
                       options->supersample_depth,
                       options->supersample_threshold,
                       options->dither,
                       blend_tool->start_x - off_x,
                       blend_tool->start_y - off_y,
                       blend_tool->end_x - off_x,
                       blend_tool->end_y - off_y,
                       progress);

  if (progress)
    gimp_progress_end (progress);

  gimp_image_flush (image);
}

static void
gimp_blend_tool_push_status (GimpBlendTool   *blend_tool,
                             GdkModifierType  state,
                             GimpDisplay     *display)
{
  GimpTool *tool = GIMP_TOOL (blend_tool);
  gchar    *status_help;

  status_help = gimp_suggest_modifiers ("",
                                        (gimp_get_constrain_behavior_mask () |
                                         GDK_MOD1_MASK) &
                                        ~state,
                                        NULL,
                                        _("%s for constrained angles"),
                                        _("%s to move the whole line"));

  gimp_tool_push_status_coords (tool, display,
                                gimp_tool_control_get_precision (tool->control),
                                _("Blend: "),
                                blend_tool->end_x - blend_tool->start_x,
                                ", ",
                                blend_tool->end_y - blend_tool->start_y,
                                status_help);

  g_free (status_help);
}

/* gegl graph stuff */

static void
gimp_blend_tool_create_graph (GimpBlendTool *blend_tool)
{
  GimpBlendOptions *options = GIMP_BLEND_TOOL_GET_OPTIONS (blend_tool);
  GimpContext      *context = GIMP_CONTEXT (options);
  GeglNode *graph, *output, *render;

  /* render_node is not supposed to be recreated */
  g_return_if_fail (blend_tool->graph == NULL);

  graph = gegl_node_new ();

  output = gegl_node_get_output_proxy (graph, "output");


  render = gegl_node_new_child (graph,
                                "operation", "gimp:blend",
                                NULL);

  gegl_node_link (render, output);

  blend_tool->graph       = graph;
  blend_tool->render_node = render;

  gegl_node_set (render,
                 "context", context,
                 NULL);
}

static void
gimp_blend_tool_update_preview_coords (GimpBlendTool *blend_tool)
{
  GimpTool *tool = GIMP_TOOL (blend_tool);
  gint      off_x, off_y;

  gimp_item_get_offset (GIMP_ITEM (tool->drawable), &off_x, &off_y);

  gegl_node_set (blend_tool->render_node,
                 "start_x", blend_tool->start_x - off_x,
                 "start_y", blend_tool->start_y - off_y,
                 "end_x",   blend_tool->end_x - off_x,
                 "end_y",   blend_tool->end_y - off_y,
                 NULL);
}

static void
gimp_blend_tool_gradient_dirty (GimpBlendTool *blend_tool)
{
  if (!blend_tool->image_map)
    return;

  /* Set a property on the node. Otherwise it will cache and refuse to update */
  gegl_node_set (blend_tool->render_node,
                 "gradient", blend_tool->gradient,
                 NULL);

  /* Update the image_map */
  gimp_image_map_apply (blend_tool->image_map, NULL);
}

static void
gimp_blend_tool_set_gradient (GimpBlendTool *blend_tool,
                              GimpGradient  *gradient)
{
  GimpBlendOptions *options = GIMP_BLEND_TOOL_GET_OPTIONS (blend_tool);
  GimpContext      *context = GIMP_CONTEXT (options);

  if (blend_tool->gradient)
    {
      g_signal_handlers_disconnect_by_func (blend_tool->gradient,
                                            G_CALLBACK (gimp_blend_tool_gradient_dirty),
                                            blend_tool);
      g_signal_handlers_disconnect_by_func (context,
                                            G_CALLBACK (gimp_blend_tool_gradient_dirty),
                                            blend_tool);
      g_object_unref (blend_tool->gradient);
      blend_tool->gradient = NULL;
    }

  if (gradient)
    {
      blend_tool->gradient = g_object_ref (gradient);
      g_signal_connect_swapped (blend_tool->gradient, "dirty",
                                G_CALLBACK (gimp_blend_tool_gradient_dirty),
                                blend_tool);

      if (gimp_gradient_has_fg_bg_segments (blend_tool->gradient))
        {
          g_signal_connect_swapped (context, "background-changed",
                                    G_CALLBACK (gimp_blend_tool_gradient_dirty),
                                    blend_tool);
          g_signal_connect_swapped (context, "foreground-changed",
                                    G_CALLBACK (gimp_blend_tool_gradient_dirty),
                                    blend_tool);
        }

      if (blend_tool->render_node)
        gegl_node_set (blend_tool->render_node,
                       "gradient", blend_tool->gradient,
                       NULL);
    }
}

static void
gimp_blend_tool_options_notify (GimpTool         *tool,
                                GimpToolOptions  *options,
                                const GParamSpec *pspec)
{
  GimpContext   *context    = GIMP_CONTEXT (options);
  GimpBlendTool *blend_tool = GIMP_BLEND_TOOL (tool);

  if (! strcmp (pspec->name, "gradient"))
    {
      gimp_blend_tool_set_gradient (blend_tool, context->gradient);

      if (blend_tool->image_map)
        gimp_image_map_apply (blend_tool->image_map, NULL);
    }
  else if (blend_tool->render_node &&
           gegl_node_find_property (blend_tool->render_node, pspec->name))
    {
      /* Sync any property changes on the config object that match the op */
      GValue value = G_VALUE_INIT;
      g_value_init (&value, pspec->value_type);

      g_object_get_property (G_OBJECT (options), pspec->name, &value);
      gegl_node_set_property (blend_tool->render_node, pspec->name, &value);

      g_value_unset (&value);

      gimp_image_map_apply (blend_tool->image_map, NULL);
    }
  else if (blend_tool->image_map &&
           (! strcmp (pspec->name, "opacity") ||
            ! strcmp (pspec->name, "paint-mode")))
    {
      gimp_image_map_set_mode (blend_tool->image_map,
                               gimp_context_get_opacity (context),
                               gimp_context_get_paint_mode (context));

      gimp_image_map_apply (blend_tool->image_map, NULL);
    }
}

static gboolean
gimp_blend_tool_is_shapeburst (GimpBlendTool *blend_tool)
{
  GimpBlendOptions *options = GIMP_BLEND_TOOL_GET_OPTIONS (blend_tool);

  return options->gradient_type >= GIMP_GRADIENT_SHAPEBURST_ANGULAR &&
         options->gradient_type <= GIMP_GRADIENT_SHAPEBURST_DIMPLED;
}

/* Image map stuff */

static void
gimp_blend_tool_create_image_map (GimpBlendTool *blend_tool,
                                  GimpDrawable  *drawable)
{
  GimpBlendOptions *options = GIMP_BLEND_TOOL_GET_OPTIONS (blend_tool);
  GimpContext      *context = GIMP_CONTEXT (options);

  if (! blend_tool->graph)
    gimp_blend_tool_create_graph (blend_tool);

  blend_tool->image_map = gimp_image_map_new (drawable,
                                              C_("undo-type", "Blend"),
                                              blend_tool->graph,
                                              GIMP_STOCK_TOOL_BLEND);

  gimp_image_map_set_region (blend_tool->image_map,
                             GIMP_IMAGE_MAP_REGION_DRAWABLE);

  g_signal_connect (blend_tool->image_map, "flush",
                    G_CALLBACK (gimp_blend_tool_image_map_flush),
                    blend_tool);

  gimp_image_map_set_mode (blend_tool->image_map,
                           gimp_context_get_opacity (context),
                           gimp_context_get_paint_mode (context));
}

static void
gimp_blend_tool_image_map_flush (GimpImageMap *image_map,
                                 GimpTool     *tool)
{
  GimpImage *image = gimp_display_get_image (tool->display);

  gimp_projection_flush (gimp_image_get_projection (image));
}
