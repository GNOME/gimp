/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * GimpPaintSelectTool
 * Copyright (C) 2020  Thomas Manni <thomas.manni@free.fr>
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
#include <math.h>

#include <gegl.h>
#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>


#include "libgimpmath/gimpmath.h"
#include "libgimpbase/gimpbase.h"
#include "libgimpcolor/gimpcolor.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "tools-types.h"

#include "config/gimpguiconfig.h"

#include "gegl/gimp-gegl-loops.h"
#include "gegl/gimp-gegl-mask.h"
#include "gegl/gimp-gegl-utils.h"

#include "core/gimp.h"
#include "core/gimpchannel-select.h"
#include "core/gimperror.h"
#include "core/gimpimage.h"
#include "core/gimplayer.h"
#include "core/gimplayermask.h"
#include "core/gimpprogress.h"
#include "core/gimpscanconvert.h"

#include "widgets/gimphelp-ids.h"
#include "widgets/gimpwidgets-utils.h"

#include "display/gimpdisplay.h"
#include "display/gimpdisplayshell.h"
#include "display/gimpdisplayshell-scroll.h"
#include "display/gimptoolgui.h"

#include "core/gimptooloptions.h"
#include "gimppaintselecttool.h"
#include "gimppaintselectoptions.h"
#include "gimptoolcontrol.h"

#include "gimp-intl.h"

#include "config/gimpguiconfig.h" /* playground */


static void      gimp_paint_select_tool_control            (GimpTool           *tool,
                                                            GimpToolAction      action,
                                                            GimpDisplay        *display);
static void      gimp_paint_select_tool_button_press       (GimpTool           *tool,
                                                            const GimpCoords   *coords,
                                                            guint32             time,
                                                            GdkModifierType     state,
                                                            GimpButtonPressType press_type,
                                                            GimpDisplay          *display);
static void      gimp_paint_select_tool_button_release     (GimpTool             *tool,
                                                            const GimpCoords     *coords,
                                                            guint32               time,
                                                            GdkModifierType       state,
                                                            GimpButtonReleaseType release_type,
                                                            GimpDisplay          *display);
static void      gimp_paint_select_tool_motion             (GimpTool             *tool,
                                                            const GimpCoords     *coords,
                                                            guint32               time,
                                                            GdkModifierType       state,
                                                            GimpDisplay          *display);
static gboolean  gimp_paint_select_tool_key_press          (GimpTool             *tool,
                                                            GdkEventKey          *kevent,
                                                            GimpDisplay          *display);
static void      gimp_paint_select_tool_modifier_key       (GimpTool             *tool,
                                                            GdkModifierType       key,
                                                            gboolean              press,
                                                            GdkModifierType       state,
                                                            GimpDisplay          *display);
static void      gimp_paint_select_tool_oper_update        (GimpTool             *tool,
                                                            const GimpCoords     *coords,
                                                            GdkModifierType       state,
                                                            gboolean              proximity,
                                                            GimpDisplay          *display);
static void      gimp_paint_select_tool_options_notify     (GimpTool             *tool,
                                                            GimpToolOptions      *options,
                                                            const GParamSpec     *pspec);
static void      gimp_paint_select_tool_cursor_update      (GimpTool             *tool,
                                                            const GimpCoords     *coords,
                                                            GdkModifierType       state,
                                                            GimpDisplay          *display);
static void      gimp_paint_select_tool_draw               (GimpDrawTool         *draw_tool);

static void      gimp_paint_select_tool_halt               (GimpPaintSelectTool  *ps_tool);

static void      gimp_paint_select_tool_update_image_mask  (GimpPaintSelectTool  *ps_tool,
                                                            GeglBuffer           *buffer,
                                                            gint                  offset_x,
                                                            gint                  offset_y,
                                                            GimpPaintSelectMode   mode);
static void      gimp_paint_select_tool_init_buffers       (GimpPaintSelectTool  *ps_tool,
                                                            GimpImage            *image,
                                                            GimpDrawable         *drawable);
static gboolean  gimp_paint_select_tool_can_paint          (GimpPaintSelectTool  *ps_tool,
                                                            GimpDisplay          *display,
                                                            gboolean              show_message);
static gboolean  gimp_paint_select_tool_start              (GimpPaintSelectTool  *ps_tool,
                                                            GimpDisplay          *display);
static void      gimp_paint_select_tool_init_scribble      (GimpPaintSelectTool  *ps_tool);

static void      gimp_paint_select_tool_create_graph       (GimpPaintSelectTool  *ps_tool);

static gboolean  gimp_paint_select_tool_paint_scribble     (GimpPaintSelectTool  *ps_tool);

static void      gimp_paint_select_tool_toggle_scribbles_visibility (GimpPaintSelectTool  *ps_tool);

static GeglRectangle gimp_paint_select_tool_get_local_region (GimpDisplay        *display,
                                                              gint                brush_x,
                                                              gint                brush_y,
                                                              gint                drawable_off_x,
                                                              gint                drawable_off_y,
                                                              gint                drawable_width,
                                                              gint                drawable_height);

static gfloat euclidean_distance                           (gint                  x1,
                                                            gint                  y1,
                                                            gint                  x2,
                                                            gint                  y2);


G_DEFINE_TYPE (GimpPaintSelectTool, gimp_paint_select_tool,
               GIMP_TYPE_DRAW_TOOL)

#define parent_class gimp_paint_select_tool_parent_class


void
gimp_paint_select_tool_register (GimpToolRegisterCallback  callback,
                                 gpointer                  data)
{
  if (gegl_has_operation ("gegl:paint-select") &&
      GIMP_GUI_CONFIG (GIMP (data)->config)->playground_paint_select_tool)
    (* callback) (GIMP_TYPE_PAINT_SELECT_TOOL,
                  GIMP_TYPE_PAINT_SELECT_OPTIONS,
                  gimp_paint_select_options_gui,
                  0,
                  "gimp-paint-select-tool",
                  _("Paint Select"),
                  _("Paint Select Tool: Select objects by painting roughly"),
                  N_("P_aint Select"), NULL,
                  NULL, GIMP_HELP_TOOL_FOREGROUND_SELECT,
                  GIMP_ICON_TOOL_PAINT_SELECT,
                  data);
}

static void
gimp_paint_select_tool_class_init (GimpPaintSelectToolClass *klass)
{
  GimpToolClass              *tool_class      = GIMP_TOOL_CLASS (klass);
  GimpDrawToolClass          *draw_tool_class = GIMP_DRAW_TOOL_CLASS (klass);

  tool_class->button_press           = gimp_paint_select_tool_button_press;
  tool_class->button_release         = gimp_paint_select_tool_button_release;
  tool_class->control                = gimp_paint_select_tool_control;
  tool_class->cursor_update          = gimp_paint_select_tool_cursor_update;
  tool_class->key_press              = gimp_paint_select_tool_key_press;
  tool_class->modifier_key           = gimp_paint_select_tool_modifier_key;
  tool_class->motion                 = gimp_paint_select_tool_motion;
  tool_class->oper_update            = gimp_paint_select_tool_oper_update;
  tool_class->options_notify         = gimp_paint_select_tool_options_notify;

  draw_tool_class->draw              = gimp_paint_select_tool_draw;
}

static void
gimp_paint_select_tool_init (GimpPaintSelectTool *ps_tool)
{
  GimpTool *tool = GIMP_TOOL (ps_tool);

  gimp_tool_control_set_motion_mode (tool->control, GIMP_MOTION_MODE_EXACT);
  gimp_tool_control_set_scroll_lock (tool->control, FALSE);
  gimp_tool_control_set_preserve    (tool->control, FALSE);
  gimp_tool_control_set_dirty_mask  (tool->control,
                                     GIMP_DIRTY_IMAGE           |
                                     GIMP_DIRTY_ACTIVE_DRAWABLE);
  gimp_tool_control_set_dirty_action (tool->control,
                                      GIMP_TOOL_ACTION_HALT);
  gimp_tool_control_set_precision   (tool->control,
                                     GIMP_CURSOR_PRECISION_SUBPIXEL);
  gimp_tool_control_set_tool_cursor (tool->control,
                                     GIMP_TOOL_CURSOR_PAINTBRUSH);
  gimp_tool_control_set_cursor_modifier (tool->control,
                                         GIMP_CURSOR_MODIFIER_PLUS);
  gimp_tool_control_set_action_pixel_size (tool->control,
                                           "tools-paint-select-pixel-size-set");
  /* TODO: the size-set action is not implemented. */
  gimp_tool_control_set_action_size       (tool->control,
                                           "tools-paint-select-size-set");
  ps_tool->image_mask  = NULL;
  ps_tool->trimap      = NULL;
  ps_tool->drawable    = NULL;
  ps_tool->scribble    = NULL;
  ps_tool->graph       = NULL;
  ps_tool->ps_node     = NULL;
  ps_tool->render_node = NULL;
}

static void
gimp_paint_select_tool_button_press (GimpTool            *tool,
                                     const GimpCoords    *coords,
                                     guint32              time,
                                     GdkModifierType      state,
                                     GimpButtonPressType  press_type,
                                     GimpDisplay         *display)
{
  GimpPaintSelectTool  *ps_tool = GIMP_PAINT_SELECT_TOOL (tool);

  if (tool->display && display != tool->display)
     gimp_tool_control (tool, GIMP_TOOL_ACTION_HALT, tool->display);

  if (! tool->display)
    {
      if (! gimp_paint_select_tool_start (ps_tool, display))
        return;
    }

  g_return_if_fail (g_list_length (tool->drawables) == 1);

  ps_tool->last_pos.x = coords->x;
  ps_tool->last_pos.y = coords->y;

  if (gimp_paint_select_tool_paint_scribble (ps_tool))
    {
      GimpPaintSelectOptions *options = GIMP_PAINT_SELECT_TOOL_GET_OPTIONS (ps_tool);

      gint offset_x = ps_tool->last_pos.x - options->stroke_width / 2;
      gint offset_y = ps_tool->last_pos.y - options->stroke_width / 2;

      gimp_paint_select_tool_update_image_mask (ps_tool,
                                                ps_tool->scribble,
                                                offset_x,
                                                offset_y,
                                                options->mode);
    }

  gimp_tool_control_activate (tool->control);
}

static void
gimp_paint_select_tool_button_release (GimpTool              *tool,
                                       const GimpCoords      *coords,
                                       guint32                time,
                                       GdkModifierType        state,
                                       GimpButtonReleaseType  release_type,
                                       GimpDisplay           *display)
{
  GimpDrawTool  *draw_tool = GIMP_DRAW_TOOL (tool);

  gimp_draw_tool_pause (draw_tool);
  gimp_tool_control_halt (tool->control);
  gimp_draw_tool_resume (draw_tool);
}

static void
gimp_paint_select_tool_control (GimpTool       *tool,
                                GimpToolAction  action,
                                GimpDisplay    *display)
{
  GimpPaintSelectTool *paint_select = GIMP_PAINT_SELECT_TOOL (tool);

  switch (action)
    {
    case GIMP_TOOL_ACTION_PAUSE:
      break;

    case GIMP_TOOL_ACTION_RESUME:
      break;

    case GIMP_TOOL_ACTION_HALT:
      gimp_paint_select_tool_halt (paint_select);
      break;

    case GIMP_TOOL_ACTION_COMMIT:
      break;
    }

  GIMP_TOOL_CLASS (parent_class)->control (tool, action, display);
}

static void
gimp_paint_select_tool_cursor_update (GimpTool         *tool,
                                      const GimpCoords *coords,
                                      GdkModifierType   state,
                                      GimpDisplay      *display)
{
  GimpPaintSelectOptions    *options  = GIMP_PAINT_SELECT_TOOL_GET_OPTIONS (tool);
  GimpCursorModifier  modifier        = GIMP_CURSOR_MODIFIER_NONE;

  if (options->mode == GIMP_PAINT_SELECT_MODE_ADD)
    {
      modifier = GIMP_CURSOR_MODIFIER_PLUS;
    }
  else
    {
      modifier = GIMP_CURSOR_MODIFIER_MINUS;
    }

  gimp_tool_control_set_cursor_modifier (tool->control, modifier);

  GIMP_TOOL_CLASS (parent_class)->cursor_update (tool, coords, state, display);
}

static gboolean
gimp_paint_select_tool_can_paint (GimpPaintSelectTool  *ps_tool,
                                  GimpDisplay          *display,
                                  gboolean              show_message)
{
  GimpTool        *tool      = GIMP_TOOL (ps_tool);
  GimpGuiConfig   *config    = GIMP_GUI_CONFIG (display->gimp->config);
  GimpImage       *image     = gimp_display_get_image (display);
  GList           *drawables = gimp_image_get_selected_drawables (image);
  GimpDrawable    *drawable;

  if (g_list_length (drawables) != 1)
    {
      if (show_message)
        {
          if (g_list_length (drawables) > 1)
            gimp_tool_message_literal (tool, display,
                                       _("Cannot paint select on multiple layers. Select only one layer."));
          else
            gimp_tool_message_literal (tool, display,
                                       _("No active drawables."));
        }

      g_list_free (drawables);

      return FALSE;
    }

  drawable = drawables->data;
  g_list_free (drawables);

  if (gimp_viewable_get_children (GIMP_VIEWABLE (drawable)))
    {
      if (show_message)
        {
          gimp_tool_message_literal (tool, display,
                                     _("Cannot paint select on layer groups."));
        }

      return FALSE;
    }

  if (! gimp_item_is_visible (GIMP_ITEM (drawable)) &&
      ! config->edit_non_visible)
    {
      if (show_message)
        {
          gimp_tool_message_literal (tool, display,
                                     _("The active layer is not visible."));
        }

      return FALSE;
    }

  return TRUE;
}

static gboolean
gimp_paint_select_tool_start (GimpPaintSelectTool  *ps_tool,
                              GimpDisplay          *display)
{
  GimpTool      *tool       = GIMP_TOOL (ps_tool);
  GimpImage     *image      = gimp_display_get_image (display);
  GimpDrawable  *drawable;

  if (! gimp_paint_select_tool_can_paint (ps_tool, display, TRUE))
    return FALSE;

  tool->display   = display;
  g_list_free (tool->drawables);
  tool->drawables = gimp_image_get_selected_drawables (image);

  drawable = tool->drawables->data;

  gimp_paint_select_tool_init_buffers (ps_tool, image, drawable);
  gimp_paint_select_tool_create_graph (ps_tool);

  if (! gimp_draw_tool_is_active (GIMP_DRAW_TOOL (ps_tool)))
    gimp_draw_tool_start (GIMP_DRAW_TOOL (ps_tool), display);

  return TRUE;
}

static gboolean
gimp_paint_select_tool_key_press (GimpTool    *tool,
                                  GdkEventKey *kevent,
                                  GimpDisplay *display)
{
  return GIMP_TOOL_CLASS (parent_class)->key_press (tool, kevent, display);
}

/* Based on gimp_selection_tool_modifier_key () */
static void
gimp_paint_select_tool_modifier_key (GimpTool        *tool,
                                     GdkModifierType  key,
                                     gboolean         press,
                                     GdkModifierType  state,
                                     GimpDisplay     *display)
{
  GimpPaintSelectTool    *ps_tool = GIMP_PAINT_SELECT_TOOL (tool);
  GimpPaintSelectOptions *options = GIMP_PAINT_SELECT_TOOL_GET_OPTIONS (ps_tool);
  GdkModifierType         extend_mask;
  GdkModifierType         modify_mask;

  extend_mask = gimp_get_extend_selection_mask ();
  modify_mask = gimp_get_modify_selection_mask ();

    if (key == extend_mask ||
        key == modify_mask ||
        key == GDK_MOD1_MASK)
      {
        GimpPaintSelectMode button_mode = options->mode;

        state &= extend_mask | modify_mask | GDK_MOD1_MASK;

        if (press)
          {
            if (key == state || ! state)
              ps_tool->saved_mode = options->mode;
          }
        else
          {
            if (! state)
              button_mode = ps_tool->saved_mode;
          }

        if (state & GDK_MOD1_MASK)
          {
            button_mode = ps_tool->saved_mode;
          }
        else if (state & (extend_mask | modify_mask))
          {
            GimpChannelOps op = gimp_modifiers_to_channel_op (state);

            switch (op)
              {
              case GIMP_CHANNEL_OP_ADD:
                button_mode = GIMP_PAINT_SELECT_MODE_ADD;
                break;

              case GIMP_CHANNEL_OP_SUBTRACT:
                button_mode = GIMP_PAINT_SELECT_MODE_SUBTRACT;
                break;

              default:
                break;
              }
          }

        if (button_mode != options->mode)
          g_object_set (options, "mode", button_mode, NULL);
      }

}

static void
gimp_paint_select_tool_motion (GimpTool         *tool,
                               const GimpCoords *coords,
                               guint32           time,
                               GdkModifierType   state,
                               GimpDisplay      *display)
{
  GimpPaintSelectTool *ps_tool = GIMP_PAINT_SELECT_TOOL (tool);
  GimpDrawTool        *draw_tool = GIMP_DRAW_TOOL (tool);

  static guint32 last_time = 0;

  GIMP_TOOL_CLASS (parent_class)->motion (tool, coords, time, state, display);

  /* don't let the events come in too fast, ignore below a delay of 100 ms */
  if (time - last_time < 100)
    return;

  last_time = time;

  if (state & GDK_BUTTON1_MASK)
    {
      gfloat distance = euclidean_distance (coords->x,
                                            coords->y,
                                            ps_tool->last_pos.x,
                                            ps_tool->last_pos.y);

      if (distance >= 2.f)
        {
          gimp_draw_tool_pause (draw_tool);
          gimp_tool_control_halt (tool->control);
          ps_tool->last_pos.x = coords->x;
          ps_tool->last_pos.y = coords->y;

          if (gimp_paint_select_tool_paint_scribble (ps_tool))
            {
              GimpPaintSelectOptions *options = GIMP_PAINT_SELECT_TOOL_GET_OPTIONS (ps_tool);
              GeglRectangle  local_region;
              GeglBuffer  *result;
              GTimer      *timer;

              local_region = gimp_paint_select_tool_get_local_region (display,
                                                       coords->x, coords->y,
                                                       ps_tool->drawable_off_x,
                                                       ps_tool->drawable_off_y,
                                                       ps_tool->drawable_width,
                                                       ps_tool->drawable_height);

              if (options->mode == GIMP_PAINT_SELECT_MODE_ADD)
                {
                  gegl_node_set (ps_tool->ps_node, "mode", 0, NULL);
                  gegl_node_set (ps_tool->threshold_node, "value", 0.99, NULL);
                }
              else
                {
                  gegl_node_set (ps_tool->ps_node, "mode", 1, NULL);
                  gegl_node_set (ps_tool->threshold_node, "value", 0.01, NULL);
                }

              if (local_region.width < ps_tool->drawable_width ||
                  local_region.height < ps_tool->drawable_height)
                {
                  gegl_node_set (ps_tool->ps_node,
                                 "use_local_region", TRUE,
                                 "region_x", local_region.x,
                                 "region_y", local_region.y,
                                 "region_width", local_region.width,
                                 "region_height", local_region.height,
                                 NULL);
                }
              else
                {
                  gegl_node_set (ps_tool->ps_node,
                                 "use_local_region", FALSE, NULL);
                }

              gegl_node_set (ps_tool->render_node, "buffer", &result, NULL);

              timer = g_timer_new ();;
              gegl_node_process (ps_tool->render_node);
              g_timer_stop (timer);
              g_printerr ("processing graph takes %.3f s\n\n", g_timer_elapsed (timer, NULL));
              g_timer_destroy (timer);

              gimp_paint_select_tool_update_image_mask (ps_tool,
                                                        result,
                                                        ps_tool->drawable_off_x,
                                                        ps_tool->drawable_off_y,
                                                        options->mode);
              g_object_unref (result);
            }

          gimp_tool_control_activate (tool->control);
          gimp_draw_tool_resume (draw_tool);
        }
    }
}

static void
gimp_paint_select_tool_oper_update (GimpTool         *tool,
                                    const GimpCoords *coords,
                                    GdkModifierType   state,
                                    gboolean          proximity,
                                    GimpDisplay      *display)
{
  GimpPaintSelectTool *ps = GIMP_PAINT_SELECT_TOOL (tool);
  GimpDrawTool *draw_tool = GIMP_DRAW_TOOL (tool);

  if (proximity)
    {
      gimp_draw_tool_pause (draw_tool);

      if (! tool->display || display == tool->display)
        {
          ps->last_pos.x = coords->x;
          ps->last_pos.y = coords->y;
        }

      if (! gimp_draw_tool_is_active (draw_tool))
        gimp_draw_tool_start (draw_tool, display);

      gimp_draw_tool_resume (draw_tool);
    }
  else if (gimp_draw_tool_is_active (draw_tool))
    {
      gimp_draw_tool_stop (draw_tool);
    }
}

static void
gimp_paint_select_tool_draw (GimpDrawTool *draw_tool)
{
  GimpPaintSelectTool    *paint_select = GIMP_PAINT_SELECT_TOOL (draw_tool);
  GimpPaintSelectOptions *options = GIMP_PAINT_SELECT_TOOL_GET_OPTIONS (paint_select);
  gint size = options->stroke_width;

  gimp_draw_tool_add_arc (draw_tool,
                          FALSE,
                          paint_select->last_pos.x - (size / 2.0),
                          paint_select->last_pos.y - (size / 2.0),
                          size, size,
                          0.0, (2.0 * G_PI));
}

static void
gimp_paint_select_tool_options_notify (GimpTool         *tool,
                                       GimpToolOptions  *options,
                                       const GParamSpec *pspec)
{
  GimpPaintSelectTool  *ps_tool = GIMP_PAINT_SELECT_TOOL (tool);

  if (g_strcmp0 (pspec->name, "stroke-width") == 0)
    {
      /* This triggers a redraw of the tool pointer, especially useful
       * here when we change the pen size with on-canvas interaction.
       */
      gimp_draw_tool_pause (GIMP_DRAW_TOOL (tool));
      gimp_draw_tool_resume (GIMP_DRAW_TOOL (tool));
    }

  if (! tool->display)
    return;

  if (! strcmp (pspec->name, "stroke-width") && ps_tool->scribble)
    {
      g_object_unref (ps_tool->scribble);
      ps_tool->scribble = NULL;
    }
  else if (! strcmp (pspec->name, "show-scribbles"))
    {
      gimp_paint_select_tool_toggle_scribbles_visibility (ps_tool);
    }
}

static void
gimp_paint_select_tool_halt (GimpPaintSelectTool *ps_tool)
{
  GimpTool     *tool = GIMP_TOOL (ps_tool);

  g_clear_object (&ps_tool->trimap);
  g_clear_object (&ps_tool->graph);
  g_clear_object (&ps_tool->scribble);

  ps_tool->drawable = NULL;
  ps_tool->render_node = NULL;
  ps_tool->ps_node = NULL;

  ps_tool->image_mask = NULL;

  if (tool->display)
    {
      gimp_display_shell_set_mask (gimp_display_get_shell (tool->display),
                                   NULL, 0, 0, NULL, FALSE);
      gimp_image_flush (gimp_display_get_image (tool->display));
    }

  tool->display   = NULL;
  g_list_free (tool->drawables);
  tool->drawables = NULL;
}

static void
gimp_paint_select_tool_update_image_mask (GimpPaintSelectTool *ps_tool,
                                          GeglBuffer          *buffer,
                                          gint                 offset_x,
                                          gint                 offset_y,
                                          GimpPaintSelectMode  mode)
{
  GimpTool  *tool = GIMP_TOOL (ps_tool);
  GimpChannelOps op;

  if (tool->display)
    {
      GimpImage *image = gimp_display_get_image (tool->display);

      if (mode == GIMP_PAINT_SELECT_MODE_ADD)
        op = GIMP_CHANNEL_OP_ADD;
      else
        op = GIMP_CHANNEL_OP_SUBTRACT;

      gimp_channel_select_buffer (gimp_image_get_mask (image),
                                  C_("command", "Paint Select"),
                                  buffer,
                                  offset_x,
                                  offset_y,
                                  op,
                                  FALSE,
                                  0,
                                  0);
      gimp_image_flush (image);
    }
}

static void
gimp_paint_select_tool_init_buffers (GimpPaintSelectTool  *ps_tool,
                                     GimpImage            *image,
                                     GimpDrawable         *drawable)
{
  GimpChannel  *channel;
  GeglColor    *grey = gegl_color_new ("#888");

  g_return_if_fail (ps_tool->trimap == NULL);
  g_return_if_fail (ps_tool->drawable == NULL);

  gimp_item_get_offset (GIMP_ITEM (drawable),
                        &ps_tool->drawable_off_x,
                        &ps_tool->drawable_off_y);
  ps_tool->drawable_width  = gimp_item_get_width (GIMP_ITEM (drawable));
  ps_tool->drawable_height = gimp_item_get_height (GIMP_ITEM (drawable));

  ps_tool->drawable = gimp_drawable_get_buffer (drawable);

  channel = gimp_image_get_mask (image);
  ps_tool->image_mask = gimp_drawable_get_buffer (GIMP_DRAWABLE (channel));
  ps_tool->trimap = gegl_buffer_new (gegl_buffer_get_extent (ps_tool->drawable),
                                     babl_format ("Y float"));
  gegl_buffer_set_color (ps_tool->trimap, NULL, grey);

  g_object_unref (grey);
}

static void
gimp_paint_select_tool_init_scribble (GimpPaintSelectTool  *ps_tool)
{
  GimpPaintSelectOptions *options = GIMP_PAINT_SELECT_TOOL_GET_OPTIONS (ps_tool);

  GimpScanConvert  *scan_convert;
  GimpVector2       points[2];
  gint              size   = options->stroke_width;
  gint              radius = size / 2;
  GeglRectangle     square = {0, 0, size, size};

  if (ps_tool->scribble)
    g_object_unref (ps_tool->scribble);

  ps_tool->scribble = gegl_buffer_linear_new (&square, babl_format ("Y float"));

  points[0].x = points[1].x = radius;
  points[0].y = points[1].y = radius;
  points[1].x += 0.01;
  points[1].y += 0.01;

  scan_convert = gimp_scan_convert_new ();
  gimp_scan_convert_add_polyline (scan_convert, 2, points, FALSE);
  gimp_scan_convert_stroke (scan_convert, size,
                            GIMP_JOIN_ROUND, GIMP_CAP_ROUND, 10.0,
                            0.0, NULL);
  gimp_scan_convert_compose (scan_convert, ps_tool->scribble, 0, 0);
  gimp_scan_convert_free (scan_convert);
}

static gboolean
gimp_paint_select_tool_paint_scribble (GimpPaintSelectTool  *ps_tool)
{
  GimpPaintSelectOptions *options = GIMP_PAINT_SELECT_TOOL_GET_OPTIONS (ps_tool);

  gint  size   = options->stroke_width;
  gint  radius = size / 2;
  GeglRectangle  trimap_area;
  GeglRectangle  mask_area;

  GeglBufferIterator  *iter;
  gfloat scribble_value;
  gboolean overlap = FALSE;

  if (! ps_tool->scribble)
    {
      gimp_paint_select_tool_init_scribble (ps_tool);
    }

  /* add the scribble to the trimap buffer and check the image mask to see if
     an optimization should be triggered.
   */

  if (options->mode == GIMP_PAINT_SELECT_MODE_ADD)
    {
      scribble_value = 1.f;
    }
  else
    {
      scribble_value = 0.f;
    }

  iter = gegl_buffer_iterator_new (ps_tool->scribble, NULL, 0,
                                   babl_format ("Y float"),
                                   GEGL_ACCESS_READ, GEGL_ABYSS_NONE, 3);

  mask_area = *gegl_buffer_get_extent (ps_tool->scribble);
  mask_area.x = ps_tool->last_pos.x - radius;
  mask_area.y = ps_tool->last_pos.y - radius;

  gegl_rectangle_copy (&trimap_area, &mask_area);

  trimap_area.x = mask_area.x - ps_tool->drawable_off_x;
  trimap_area.y = mask_area.y - ps_tool->drawable_off_y;

  gegl_buffer_iterator_add (iter, ps_tool->trimap, &trimap_area, 0,
                            babl_format ("Y float"),
                            GEGL_ACCESS_READWRITE, GEGL_ABYSS_NONE);

  gegl_buffer_iterator_add (iter, ps_tool->image_mask, &mask_area, 0,
                            babl_format ("Y float"),
                            GEGL_ACCESS_READ, GEGL_ABYSS_NONE);

  while (gegl_buffer_iterator_next (iter))
    {
      gfloat  *scribble_pix = iter->items[0].data;
      gfloat  *trimap_pix   = iter->items[1].data;
      gfloat  *mask_pix     = iter->items[2].data;
      gint     n_pixels     = iter->length;

      while (n_pixels--)
        {
          if (*scribble_pix)
            {
              *trimap_pix = scribble_value;

              if (*mask_pix != scribble_value)
                overlap = TRUE;
            }

          scribble_pix++;
          trimap_pix++;
          mask_pix++;
        }
    }

  gimp_paint_select_tool_toggle_scribbles_visibility (ps_tool);

  return overlap;
}

static void
gimp_paint_select_tool_create_graph (GimpPaintSelectTool  *ps_tool)
{
  GeglNode  *t;         /* trimap */
  GeglNode  *m;         /* mask   */
  GeglNode  *d;         /* drawable */
  GeglNode  *crop;
  GeglNode  *translate = NULL;

  ps_tool->graph = gegl_node_new ();

  m = gegl_node_new_child (ps_tool->graph,
                           "operation", "gegl:buffer-source",
                           "buffer", ps_tool->image_mask,
                           NULL);

  crop = gegl_node_new_child (ps_tool->graph,
                              "operation", "gegl:crop",
                              "x", (gdouble) ps_tool->drawable_off_x,
                              "y", (gdouble) ps_tool->drawable_off_y,
                              "width", (gdouble) gegl_buffer_get_width (ps_tool->drawable),
                              "height", (gdouble) gegl_buffer_get_height (ps_tool->drawable),
                              NULL);

  ps_tool->threshold_node = gegl_node_new_child (ps_tool->graph,
                                                 "operation", "gegl:threshold",
                                                 NULL);

  if (ps_tool->drawable_off_x || ps_tool->drawable_off_y)
    {
      translate = gegl_node_new_child (ps_tool->graph,
                                       "operation", "gegl:translate",
                                       "x", -1.0 * ps_tool->drawable_off_x,
                                       "y", -1.0 * ps_tool->drawable_off_y,
                                       NULL);
    }

  d = gegl_node_new_child (ps_tool->graph,
                           "operation", "gegl:buffer-source",
                           "buffer", ps_tool->drawable,
                           NULL);


  t = gegl_node_new_child (ps_tool->graph,
                           "operation", "gegl:buffer-source",
                           "buffer", ps_tool->trimap,
                           NULL);

  ps_tool->ps_node = gegl_node_new_child (ps_tool->graph,
                            "operation", "gegl:paint-select",
                            NULL);

  ps_tool->render_node = gegl_node_new_child (ps_tool->graph,
                                            "operation", "gegl:buffer-sink",
                                             NULL);

  gegl_node_link_many (m, ps_tool->threshold_node, crop, NULL);

  if (translate)
    gegl_node_link_many (crop, translate, ps_tool->ps_node, ps_tool->render_node, NULL);
  else
    gegl_node_link_many (crop, ps_tool->ps_node, ps_tool->render_node, NULL);

  gegl_node_connect (d, "output", ps_tool->ps_node, "aux");
  gegl_node_connect (t, "output", ps_tool->ps_node, "aux2");
}

static void
gimp_paint_select_tool_toggle_scribbles_visibility (GimpPaintSelectTool  *ps_tool)
{
  GimpTool  *tool = GIMP_TOOL (ps_tool);
  GimpPaintSelectOptions  *options = GIMP_PAINT_SELECT_TOOL_GET_OPTIONS (tool);

  if (options->show_scribbles)
    {
      GeglColor *black = gegl_color_new ("black");

      gimp_display_shell_set_mask (gimp_display_get_shell (tool->display),
                                   ps_tool->trimap,
                                   ps_tool->drawable_off_x,
                                   ps_tool->drawable_off_y,
                                   black,
                                   TRUE);
      g_object_unref (black);
    }
  else
    {
      gimp_display_shell_set_mask (gimp_display_get_shell (tool->display),
                                   NULL, 0, 0, NULL, FALSE);
    }
}

static GeglRectangle
gimp_paint_select_tool_get_local_region (GimpDisplay  *display,
                                         gint          brush_x,
                                         gint          brush_y,
                                         gint          drawable_off_x,
                                         gint          drawable_off_y,
                                         gint          drawable_width,
                                         gint          drawable_height)
{
  GimpDisplayShell  *shell;
  GeglRectangle      brush_window;
  GeglRectangle      drawable_region;
  GeglRectangle      viewport;
  GeglRectangle      local_region;
  gdouble            x, y, w, h;

  shell = gimp_display_get_shell (display);
  gimp_display_shell_scroll_get_viewport (shell, &x, &y, &w, &h);

  viewport.x      = (gint) x;
  viewport.y      = (gint) y;
  viewport.width  = (gint) w;
  viewport.height = (gint) h;

  brush_window.x      = brush_x - viewport.width / 2;
  brush_window.y      = brush_y - viewport.height / 2;
  brush_window.width  = viewport.width;
  brush_window.height = viewport.height;

  gegl_rectangle_bounding_box (&local_region, &brush_window, &viewport);

  drawable_region.x      = drawable_off_x;
  drawable_region.y      = drawable_off_y;
  drawable_region.width  = drawable_width;
  drawable_region.height = drawable_height;

  gegl_rectangle_intersect (&local_region, &local_region, &drawable_region);

  local_region.x -= drawable_off_x;
  local_region.y -= drawable_off_y;

  g_printerr ("local region: (%d,%d) %d x %d\n",
              local_region.x, local_region.y,
              local_region.width, local_region.height);

  return local_region;
}

static gfloat
euclidean_distance (gint  x1,
                    gint  y1,
                    gint  x2,
                    gint  y2)
{
  return sqrtf ((x1 - x2) * (x1 - x2) + (y1 - y2) * (y1 - y2));
}
