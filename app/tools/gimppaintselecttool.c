/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * LigmaPaintSelectTool
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


#include "libligmamath/ligmamath.h"
#include "libligmabase/ligmabase.h"
#include "libligmacolor/ligmacolor.h"
#include "libligmawidgets/ligmawidgets.h"

#include "tools-types.h"

#include "config/ligmaguiconfig.h"

#include "gegl/ligma-gegl-loops.h"
#include "gegl/ligma-gegl-mask.h"
#include "gegl/ligma-gegl-utils.h"

#include "core/ligma.h"
#include "core/ligmachannel-select.h"
#include "core/ligmaerror.h"
#include "core/ligmaimage.h"
#include "core/ligmalayer.h"
#include "core/ligmalayermask.h"
#include "core/ligmaprogress.h"
#include "core/ligmascanconvert.h"

#include "widgets/ligmahelp-ids.h"
#include "widgets/ligmawidgets-utils.h"

#include "display/ligmadisplay.h"
#include "display/ligmadisplayshell.h"
#include "display/ligmadisplayshell-scroll.h"
#include "display/ligmatoolgui.h"

#include "core/ligmatooloptions.h"
#include "ligmapaintselecttool.h"
#include "ligmapaintselectoptions.h"
#include "ligmatoolcontrol.h"

#include "ligma-intl.h"

#include "config/ligmaguiconfig.h" /* playground */


static void      ligma_paint_select_tool_control            (LigmaTool           *tool,
                                                            LigmaToolAction      action,
                                                            LigmaDisplay        *display);
static void      ligma_paint_select_tool_button_press       (LigmaTool           *tool,
                                                            const LigmaCoords   *coords,
                                                            guint32             time,
                                                            GdkModifierType     state,
                                                            LigmaButtonPressType press_type,
                                                            LigmaDisplay          *display);
static void      ligma_paint_select_tool_button_release     (LigmaTool             *tool,
                                                            const LigmaCoords     *coords,
                                                            guint32               time,
                                                            GdkModifierType       state,
                                                            LigmaButtonReleaseType release_type,
                                                            LigmaDisplay          *display);
static void      ligma_paint_select_tool_motion             (LigmaTool             *tool,
                                                            const LigmaCoords     *coords,
                                                            guint32               time,
                                                            GdkModifierType       state,
                                                            LigmaDisplay          *display);
static gboolean  ligma_paint_select_tool_key_press          (LigmaTool             *tool,
                                                            GdkEventKey          *kevent,
                                                            LigmaDisplay          *display);
static void      ligma_paint_select_tool_modifier_key       (LigmaTool             *tool,
                                                            GdkModifierType       key,
                                                            gboolean              press,
                                                            GdkModifierType       state,
                                                            LigmaDisplay          *display);
static void      ligma_paint_select_tool_oper_update        (LigmaTool             *tool,
                                                            const LigmaCoords     *coords,
                                                            GdkModifierType       state,
                                                            gboolean              proximity,
                                                            LigmaDisplay          *display);
static void      ligma_paint_select_tool_options_notify     (LigmaTool             *tool,
                                                            LigmaToolOptions      *options,
                                                            const GParamSpec     *pspec);
static void      ligma_paint_select_tool_cursor_update      (LigmaTool             *tool,
                                                            const LigmaCoords     *coords,
                                                            GdkModifierType       state,
                                                            LigmaDisplay          *display);
static void      ligma_paint_select_tool_draw               (LigmaDrawTool         *draw_tool);

static void      ligma_paint_select_tool_halt               (LigmaPaintSelectTool  *ps_tool);

static void      ligma_paint_select_tool_update_image_mask  (LigmaPaintSelectTool  *ps_tool,
                                                            GeglBuffer           *buffer,
                                                            gint                  offset_x,
                                                            gint                  offset_y,
                                                            LigmaPaintSelectMode   mode);
static void      ligma_paint_select_tool_init_buffers       (LigmaPaintSelectTool  *ps_tool,
                                                            LigmaImage            *image,
                                                            LigmaDrawable         *drawable);
static gboolean  ligma_paint_select_tool_can_paint          (LigmaPaintSelectTool  *ps_tool,
                                                            LigmaDisplay          *display,
                                                            gboolean              show_message);
static gboolean  ligma_paint_select_tool_start              (LigmaPaintSelectTool  *ps_tool,
                                                            LigmaDisplay          *display);
static void      ligma_paint_select_tool_init_scribble      (LigmaPaintSelectTool  *ps_tool);

static void      ligma_paint_select_tool_create_graph       (LigmaPaintSelectTool  *ps_tool);

static gboolean  ligma_paint_select_tool_paint_scribble     (LigmaPaintSelectTool  *ps_tool);

static void      ligma_paint_select_tool_toggle_scribbles_visibility (LigmaPaintSelectTool  *ps_tool);

static GeglRectangle ligma_paint_select_tool_get_local_region (LigmaDisplay        *display,
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


G_DEFINE_TYPE (LigmaPaintSelectTool, ligma_paint_select_tool,
               LIGMA_TYPE_DRAW_TOOL)

#define parent_class ligma_paint_select_tool_parent_class


void
ligma_paint_select_tool_register (LigmaToolRegisterCallback  callback,
                                 gpointer                  data)
{
  if (gegl_has_operation ("gegl:paint-select") &&
      LIGMA_GUI_CONFIG (LIGMA (data)->config)->playground_paint_select_tool)
    (* callback) (LIGMA_TYPE_PAINT_SELECT_TOOL,
                  LIGMA_TYPE_PAINT_SELECT_OPTIONS,
                  ligma_paint_select_options_gui,
                  0,
                  "ligma-paint-select-tool",
                  _("Paint Select"),
                  _("Paint Select Tool: Select objects by painting roughly"),
                  N_("P_aint Select"), NULL,
                  NULL, LIGMA_HELP_TOOL_FOREGROUND_SELECT,
                  LIGMA_ICON_TOOL_PAINT_SELECT,
                  data);
}

static void
ligma_paint_select_tool_class_init (LigmaPaintSelectToolClass *klass)
{
  LigmaToolClass              *tool_class      = LIGMA_TOOL_CLASS (klass);
  LigmaDrawToolClass          *draw_tool_class = LIGMA_DRAW_TOOL_CLASS (klass);

  tool_class->button_press           = ligma_paint_select_tool_button_press;
  tool_class->button_release         = ligma_paint_select_tool_button_release;
  tool_class->control                = ligma_paint_select_tool_control;
  tool_class->cursor_update          = ligma_paint_select_tool_cursor_update;
  tool_class->key_press              = ligma_paint_select_tool_key_press;
  tool_class->modifier_key           = ligma_paint_select_tool_modifier_key;
  tool_class->motion                 = ligma_paint_select_tool_motion;
  tool_class->oper_update            = ligma_paint_select_tool_oper_update;
  tool_class->options_notify         = ligma_paint_select_tool_options_notify;

  draw_tool_class->draw              = ligma_paint_select_tool_draw;
}

static void
ligma_paint_select_tool_init (LigmaPaintSelectTool *ps_tool)
{
  LigmaTool *tool = LIGMA_TOOL (ps_tool);

  ligma_tool_control_set_motion_mode (tool->control, LIGMA_MOTION_MODE_EXACT);
  ligma_tool_control_set_scroll_lock (tool->control, FALSE);
  ligma_tool_control_set_preserve    (tool->control, FALSE);
  ligma_tool_control_set_dirty_mask  (tool->control,
                                     LIGMA_DIRTY_IMAGE           |
                                     LIGMA_DIRTY_ACTIVE_DRAWABLE);
  ligma_tool_control_set_dirty_action (tool->control,
                                      LIGMA_TOOL_ACTION_HALT);
  ligma_tool_control_set_precision   (tool->control,
                                     LIGMA_CURSOR_PRECISION_SUBPIXEL);
  ligma_tool_control_set_tool_cursor (tool->control,
                                     LIGMA_TOOL_CURSOR_PAINTBRUSH);
  ligma_tool_control_set_cursor_modifier (tool->control,
                                         LIGMA_CURSOR_MODIFIER_PLUS);
  ligma_tool_control_set_action_pixel_size (tool->control,
                                           "tools/tools-paint-select-pixel-size-set");
  /* TODO: the size-set action is not implemented. */
  ligma_tool_control_set_action_size       (tool->control,
                                           "tools/tools-paint-select-size-set");
  ps_tool->image_mask  = NULL;
  ps_tool->trimap      = NULL;
  ps_tool->drawable    = NULL;
  ps_tool->scribble    = NULL;
  ps_tool->graph       = NULL;
  ps_tool->ps_node     = NULL;
  ps_tool->render_node = NULL;
}

static void
ligma_paint_select_tool_button_press (LigmaTool            *tool,
                                     const LigmaCoords    *coords,
                                     guint32              time,
                                     GdkModifierType      state,
                                     LigmaButtonPressType  press_type,
                                     LigmaDisplay         *display)
{
  LigmaPaintSelectTool  *ps_tool = LIGMA_PAINT_SELECT_TOOL (tool);

  if (tool->display && display != tool->display)
     ligma_tool_control (tool, LIGMA_TOOL_ACTION_HALT, tool->display);

  if (! tool->display)
    {
      if (! ligma_paint_select_tool_start (ps_tool, display))
        return;
    }

  g_return_if_fail (g_list_length (tool->drawables) == 1);

  ps_tool->last_pos.x = coords->x;
  ps_tool->last_pos.y = coords->y;

  if (ligma_paint_select_tool_paint_scribble (ps_tool))
    {
      LigmaPaintSelectOptions *options = LIGMA_PAINT_SELECT_TOOL_GET_OPTIONS (ps_tool);

      gint offset_x = ps_tool->last_pos.x - options->stroke_width / 2;
      gint offset_y = ps_tool->last_pos.y - options->stroke_width / 2;

      ligma_paint_select_tool_update_image_mask (ps_tool,
                                                ps_tool->scribble,
                                                offset_x,
                                                offset_y,
                                                options->mode);
    }

  ligma_tool_control_activate (tool->control);
}

static void
ligma_paint_select_tool_button_release (LigmaTool              *tool,
                                       const LigmaCoords      *coords,
                                       guint32                time,
                                       GdkModifierType        state,
                                       LigmaButtonReleaseType  release_type,
                                       LigmaDisplay           *display)
{
  LigmaDrawTool  *draw_tool = LIGMA_DRAW_TOOL (tool);

  ligma_draw_tool_pause (draw_tool);
  ligma_tool_control_halt (tool->control);
  ligma_draw_tool_resume (draw_tool);
}

static void
ligma_paint_select_tool_control (LigmaTool       *tool,
                                LigmaToolAction  action,
                                LigmaDisplay    *display)
{
  LigmaPaintSelectTool *paint_select = LIGMA_PAINT_SELECT_TOOL (tool);

  switch (action)
    {
    case LIGMA_TOOL_ACTION_PAUSE:
      break;

    case LIGMA_TOOL_ACTION_RESUME:
      break;

    case LIGMA_TOOL_ACTION_HALT:
      ligma_paint_select_tool_halt (paint_select);
      break;

    case LIGMA_TOOL_ACTION_COMMIT:
      break;
    }

  LIGMA_TOOL_CLASS (parent_class)->control (tool, action, display);
}

static void
ligma_paint_select_tool_cursor_update (LigmaTool         *tool,
                                      const LigmaCoords *coords,
                                      GdkModifierType   state,
                                      LigmaDisplay      *display)
{
  LigmaPaintSelectOptions    *options  = LIGMA_PAINT_SELECT_TOOL_GET_OPTIONS (tool);
  LigmaCursorModifier  modifier        = LIGMA_CURSOR_MODIFIER_NONE;

  if (options->mode == LIGMA_PAINT_SELECT_MODE_ADD)
    {
      modifier = LIGMA_CURSOR_MODIFIER_PLUS;
    }
  else
    {
      modifier = LIGMA_CURSOR_MODIFIER_MINUS;
    }

  ligma_tool_control_set_cursor_modifier (tool->control, modifier);

  LIGMA_TOOL_CLASS (parent_class)->cursor_update (tool, coords, state, display);
}

static gboolean
ligma_paint_select_tool_can_paint (LigmaPaintSelectTool  *ps_tool,
                                  LigmaDisplay          *display,
                                  gboolean              show_message)
{
  LigmaTool        *tool      = LIGMA_TOOL (ps_tool);
  LigmaGuiConfig   *config    = LIGMA_GUI_CONFIG (display->ligma->config);
  LigmaImage       *image     = ligma_display_get_image (display);
  GList           *drawables = ligma_image_get_selected_drawables (image);
  LigmaDrawable    *drawable;

  if (g_list_length (drawables) != 1)
    {
      if (show_message)
        {
          if (g_list_length (drawables) > 1)
            ligma_tool_message_literal (tool, display,
                                       _("Cannot paint select on multiple layers. Select only one layer."));
          else
            ligma_tool_message_literal (tool, display,
                                       _("No active drawables."));
        }

      g_list_free (drawables);

      return FALSE;
    }

  drawable = drawables->data;
  g_list_free (drawables);

  if (ligma_viewable_get_children (LIGMA_VIEWABLE (drawable)))
    {
      if (show_message)
        {
          ligma_tool_message_literal (tool, display,
                                     _("Cannot paint select on layer groups."));
        }

      return FALSE;
    }

  if (! ligma_item_is_visible (LIGMA_ITEM (drawable)) &&
      ! config->edit_non_visible)
    {
      if (show_message)
        {
          ligma_tool_message_literal (tool, display,
                                     _("The active layer is not visible."));
        }

      return FALSE;
    }

  return TRUE;
}

static gboolean
ligma_paint_select_tool_start (LigmaPaintSelectTool  *ps_tool,
                              LigmaDisplay          *display)
{
  LigmaTool      *tool       = LIGMA_TOOL (ps_tool);
  LigmaImage     *image      = ligma_display_get_image (display);
  LigmaDrawable  *drawable;

  if (! ligma_paint_select_tool_can_paint (ps_tool, display, TRUE))
    return FALSE;

  tool->display   = display;
  g_list_free (tool->drawables);
  tool->drawables = ligma_image_get_selected_drawables (image);

  drawable = tool->drawables->data;

  ligma_paint_select_tool_init_buffers (ps_tool, image, drawable);
  ligma_paint_select_tool_create_graph (ps_tool);

  if (! ligma_draw_tool_is_active (LIGMA_DRAW_TOOL (ps_tool)))
    ligma_draw_tool_start (LIGMA_DRAW_TOOL (ps_tool), display);

  return TRUE;
}

static gboolean
ligma_paint_select_tool_key_press (LigmaTool    *tool,
                                  GdkEventKey *kevent,
                                  LigmaDisplay *display)
{
  return LIGMA_TOOL_CLASS (parent_class)->key_press (tool, kevent, display);
}

static void
ligma_paint_select_tool_modifier_key (LigmaTool        *tool,
                                          GdkModifierType  key,
                                          gboolean         press,
                                          GdkModifierType  state,
                                          LigmaDisplay     *display)
{
}

static void
ligma_paint_select_tool_motion (LigmaTool         *tool,
                               const LigmaCoords *coords,
                               guint32           time,
                               GdkModifierType   state,
                               LigmaDisplay      *display)
{
  LigmaPaintSelectTool *ps_tool = LIGMA_PAINT_SELECT_TOOL (tool);
  LigmaDrawTool        *draw_tool = LIGMA_DRAW_TOOL (tool);

  static guint32 last_time = 0;

  LIGMA_TOOL_CLASS (parent_class)->motion (tool, coords, time, state, display);

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
          ligma_draw_tool_pause (draw_tool);
          ligma_tool_control_halt (tool->control);
          ps_tool->last_pos.x = coords->x;
          ps_tool->last_pos.y = coords->y;

          if (ligma_paint_select_tool_paint_scribble (ps_tool))
            {
              LigmaPaintSelectOptions *options = LIGMA_PAINT_SELECT_TOOL_GET_OPTIONS (ps_tool);
              GeglRectangle  local_region;
              GeglBuffer  *result;
              GTimer      *timer;

              local_region = ligma_paint_select_tool_get_local_region (display,
                                                       coords->x, coords->y,
                                                       ps_tool->drawable_off_x,
                                                       ps_tool->drawable_off_y,
                                                       ps_tool->drawable_width,
                                                       ps_tool->drawable_height);

              if (options->mode == LIGMA_PAINT_SELECT_MODE_ADD)
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

              ligma_paint_select_tool_update_image_mask (ps_tool,
                                                        result,
                                                        ps_tool->drawable_off_x,
                                                        ps_tool->drawable_off_y,
                                                        options->mode);
              g_object_unref (result);
            }

          ligma_tool_control_activate (tool->control);
          ligma_draw_tool_resume (draw_tool);
        }
    }
}

static void
ligma_paint_select_tool_oper_update (LigmaTool         *tool,
                                    const LigmaCoords *coords,
                                    GdkModifierType   state,
                                    gboolean          proximity,
                                    LigmaDisplay      *display)
{
  LigmaPaintSelectTool *ps = LIGMA_PAINT_SELECT_TOOL (tool);
  LigmaDrawTool *draw_tool = LIGMA_DRAW_TOOL (tool);

  if (proximity)
    {
      ligma_draw_tool_pause (draw_tool);

      if (! tool->display || display == tool->display)
        {
          ps->last_pos.x = coords->x;
          ps->last_pos.y = coords->y;
        }

      if (! ligma_draw_tool_is_active (draw_tool))
        ligma_draw_tool_start (draw_tool, display);

      ligma_draw_tool_resume (draw_tool);
    }
  else if (ligma_draw_tool_is_active (draw_tool))
    {
      ligma_draw_tool_stop (draw_tool);
    }
}

static void
ligma_paint_select_tool_draw (LigmaDrawTool *draw_tool)
{
  LigmaPaintSelectTool    *paint_select = LIGMA_PAINT_SELECT_TOOL (draw_tool);
  LigmaPaintSelectOptions *options = LIGMA_PAINT_SELECT_TOOL_GET_OPTIONS (paint_select);
  gint size = options->stroke_width;

  ligma_draw_tool_add_arc (draw_tool,
                          FALSE,
                          paint_select->last_pos.x - (size / 2.0),
                          paint_select->last_pos.y - (size / 2.0),
                          size, size,
                          0.0, (2.0 * G_PI));
}

static void
ligma_paint_select_tool_options_notify (LigmaTool         *tool,
                                       LigmaToolOptions  *options,
                                       const GParamSpec *pspec)
{
  LigmaPaintSelectTool  *ps_tool = LIGMA_PAINT_SELECT_TOOL (tool);

  if (g_strcmp0 (pspec->name, "stroke-width") == 0)
    {
      /* This triggers a redraw of the tool pointer, especially useful
       * here when we change the pen size with on-canvas interaction.
       */
      ligma_draw_tool_pause (LIGMA_DRAW_TOOL (tool));
      ligma_draw_tool_resume (LIGMA_DRAW_TOOL (tool));
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
      ligma_paint_select_tool_toggle_scribbles_visibility (ps_tool);
    }
}

static void
ligma_paint_select_tool_halt (LigmaPaintSelectTool *ps_tool)
{
  LigmaTool     *tool = LIGMA_TOOL (ps_tool);

  g_clear_object (&ps_tool->trimap);
  g_clear_object (&ps_tool->graph);
  g_clear_object (&ps_tool->scribble);

  ps_tool->drawable = NULL;
  ps_tool->render_node = NULL;
  ps_tool->ps_node = NULL;

  ps_tool->image_mask = NULL;

  if (tool->display)
    {
      ligma_display_shell_set_mask (ligma_display_get_shell (tool->display),
                                   NULL, 0, 0, NULL, FALSE);
      ligma_image_flush (ligma_display_get_image (tool->display));
    }

  tool->display   = NULL;
  g_list_free (tool->drawables);
  tool->drawables = NULL;
}

static void
ligma_paint_select_tool_update_image_mask (LigmaPaintSelectTool *ps_tool,
                                          GeglBuffer          *buffer,
                                          gint                 offset_x,
                                          gint                 offset_y,
                                          LigmaPaintSelectMode  mode)
{
  LigmaTool  *tool = LIGMA_TOOL (ps_tool);
  LigmaChannelOps op;

  if (tool->display)
    {
      LigmaImage *image = ligma_display_get_image (tool->display);

      if (mode == LIGMA_PAINT_SELECT_MODE_ADD)
        op = LIGMA_CHANNEL_OP_ADD;
      else
        op = LIGMA_CHANNEL_OP_SUBTRACT;

      ligma_channel_select_buffer (ligma_image_get_mask (image),
                                  C_("command", "Paint Select"),
                                  buffer,
                                  offset_x,
                                  offset_y,
                                  op,
                                  FALSE,
                                  0,
                                  0);
      ligma_image_flush (image);
    }
}

static void
ligma_paint_select_tool_init_buffers (LigmaPaintSelectTool  *ps_tool,
                                     LigmaImage            *image,
                                     LigmaDrawable         *drawable)
{
  LigmaChannel  *channel;
  GeglColor    *grey = gegl_color_new ("#888");

  g_return_if_fail (ps_tool->trimap == NULL);
  g_return_if_fail (ps_tool->drawable == NULL);

  ligma_item_get_offset (LIGMA_ITEM (drawable),
                        &ps_tool->drawable_off_x,
                        &ps_tool->drawable_off_y);
  ps_tool->drawable_width  = ligma_item_get_width (LIGMA_ITEM (drawable));
  ps_tool->drawable_height = ligma_item_get_height (LIGMA_ITEM (drawable));

  ps_tool->drawable = ligma_drawable_get_buffer (drawable);

  channel = ligma_image_get_mask (image);
  ps_tool->image_mask = ligma_drawable_get_buffer (LIGMA_DRAWABLE (channel));
  ps_tool->trimap = gegl_buffer_new (gegl_buffer_get_extent (ps_tool->drawable),
                                     babl_format ("Y float"));
  gegl_buffer_set_color (ps_tool->trimap, NULL, grey);

  g_object_unref (grey);
}

static void
ligma_paint_select_tool_init_scribble (LigmaPaintSelectTool  *ps_tool)
{
  LigmaPaintSelectOptions *options = LIGMA_PAINT_SELECT_TOOL_GET_OPTIONS (ps_tool);

  LigmaScanConvert  *scan_convert;
  LigmaVector2       points[2];
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

  scan_convert = ligma_scan_convert_new ();
  ligma_scan_convert_add_polyline (scan_convert, 2, points, FALSE);
  ligma_scan_convert_stroke (scan_convert, size,
                            LIGMA_JOIN_ROUND, LIGMA_CAP_ROUND, 10.0,
                            0.0, NULL);
  ligma_scan_convert_compose (scan_convert, ps_tool->scribble, 0, 0);
  ligma_scan_convert_free (scan_convert);
}

static gboolean
ligma_paint_select_tool_paint_scribble (LigmaPaintSelectTool  *ps_tool)
{
  LigmaPaintSelectOptions *options = LIGMA_PAINT_SELECT_TOOL_GET_OPTIONS (ps_tool);

  gint  size   = options->stroke_width;
  gint  radius = size / 2;
  GeglRectangle  trimap_area;
  GeglRectangle  mask_area;

  GeglBufferIterator  *iter;
  gfloat scribble_value;
  gboolean overlap = FALSE;

  if (! ps_tool->scribble)
    {
      ligma_paint_select_tool_init_scribble (ps_tool);
    }

  /* add the scribble to the trimap buffer and check the image mask to see if
     an optimization should be triggered.
   */

  if (options->mode == LIGMA_PAINT_SELECT_MODE_ADD)
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

  ligma_paint_select_tool_toggle_scribbles_visibility (ps_tool);

  return overlap;
}

static void
ligma_paint_select_tool_create_graph (LigmaPaintSelectTool  *ps_tool)
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

  gegl_node_connect_to (d, "output", ps_tool->ps_node, "aux");
  gegl_node_connect_to (t, "output", ps_tool->ps_node, "aux2");
}

static void
ligma_paint_select_tool_toggle_scribbles_visibility (LigmaPaintSelectTool  *ps_tool)
{
  LigmaTool  *tool = LIGMA_TOOL (ps_tool);
  LigmaPaintSelectOptions  *options = LIGMA_PAINT_SELECT_TOOL_GET_OPTIONS (tool);

  if (options->show_scribbles)
    {
      const LigmaRGB black = {0.0, 0.0, 0.0, 1.0};
      ligma_display_shell_set_mask (ligma_display_get_shell (tool->display),
                                   ps_tool->trimap,
                                   ps_tool->drawable_off_x,
                                   ps_tool->drawable_off_y,
                                   &black,
                                   TRUE);
    }
  else
    {
      ligma_display_shell_set_mask (ligma_display_get_shell (tool->display),
                                   NULL, 0, 0, NULL, FALSE);
    }
}

static GeglRectangle
ligma_paint_select_tool_get_local_region (LigmaDisplay  *display,
                                         gint          brush_x,
                                         gint          brush_y,
                                         gint          drawable_off_x,
                                         gint          drawable_off_y,
                                         gint          drawable_width,
                                         gint          drawable_height)
{
  LigmaDisplayShell  *shell;
  GeglRectangle      brush_window;
  GeglRectangle      drawable_region;
  GeglRectangle      viewport;
  GeglRectangle      local_region;
  gdouble            x, y, w, h;

  shell = ligma_display_get_shell (display);
  ligma_display_shell_scroll_get_viewport (shell, &x, &y, &w, &h);

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
