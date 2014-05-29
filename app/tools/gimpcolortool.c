/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995-2001 Spencer Kimball, Peter Mattis, and others
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

#include <gegl.h>
#include <gtk/gtk.h>

#include "libgimpmath/gimpmath.h"
#include "libgimpcolor/gimpcolor.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "tools-types.h"

#include "config/gimpdisplayconfig.h"

#include "core/gimp.h"
#include "core/gimpdata.h"
#include "core/gimpimage.h"
#include "core/gimpimage-pick-color.h"
#include "core/gimpimage-sample-points.h"
#include "core/gimpitem.h"
#include "core/gimpmarshal.h"
#include "core/gimpsamplepoint.h"

#include "widgets/gimpcolormapeditor.h"
#include "widgets/gimpdialogfactory.h"
#include "widgets/gimpdockable.h"
#include "widgets/gimppaletteeditor.h"
#include "widgets/gimpsessioninfo.h"
#include "widgets/gimpwidgets-utils.h"
#include "widgets/gimpwindowstrategy.h"

#include "display/gimpcanvasitem.h"
#include "display/gimpdisplay.h"
#include "display/gimpdisplayshell.h"
#include "display/gimpdisplayshell-appearance.h"
#include "display/gimpdisplayshell-selection.h"
#include "display/gimpdisplayshell-transform.h"

#include "gimpcoloroptions.h"
#include "gimpcolortool.h"
#include "gimptoolcontrol.h"

#include "gimp-intl.h"


#define SAMPLE_POINT_POSITION_INVALID G_MININT


enum
{
  PICKED,
  LAST_SIGNAL
};


/*  local function prototypes  */

static void   gimp_color_tool_finalize       (GObject               *object);

static void   gimp_color_tool_button_press   (GimpTool              *tool,
                                              const GimpCoords      *coords,
                                              guint32                time,
                                              GdkModifierType        state,
                                              GimpButtonPressType    press_type,
                                              GimpDisplay           *display);
static void   gimp_color_tool_button_release (GimpTool              *tool,
                                              const GimpCoords      *coords,
                                              guint32                time,
                                              GdkModifierType        state,
                                              GimpButtonReleaseType  release_type,
                                              GimpDisplay           *display);
static void   gimp_color_tool_motion         (GimpTool              *tool,
                                              const GimpCoords      *coords,
                                              guint32                time,
                                              GdkModifierType        state,
                                              GimpDisplay           *display);
static void   gimp_color_tool_oper_update    (GimpTool              *tool,
                                              const GimpCoords      *coords,
                                              GdkModifierType        state,
                                              gboolean               proximity,
                                              GimpDisplay           *display);
static void   gimp_color_tool_cursor_update  (GimpTool              *tool,
                                              const GimpCoords      *coords,
                                              GdkModifierType        state,
                                              GimpDisplay           *display);

static void   gimp_color_tool_draw           (GimpDrawTool          *draw_tool);

static gboolean   gimp_color_tool_real_pick  (GimpColorTool         *color_tool,
                                              gint                   x,
                                              gint                   y,
                                              const Babl           **sample_format,
                                              GimpRGB               *color,
                                              gint                  *color_index);
static void   gimp_color_tool_pick           (GimpColorTool         *tool,
                                              GimpColorPickState     pick_state,
                                              gint                   x,
                                              gint                   y);
static void   gimp_color_tool_real_picked    (GimpColorTool         *color_tool,
                                              GimpColorPickState     pick_state,
                                              gdouble                x,
                                              gdouble                y,
                                              const Babl            *sample_format,
                                              const GimpRGB         *color,
                                              gint                   color_index);


G_DEFINE_TYPE (GimpColorTool, gimp_color_tool, GIMP_TYPE_DRAW_TOOL);

#define parent_class gimp_color_tool_parent_class

static guint gimp_color_tool_signals[LAST_SIGNAL] = { 0 };


static void
gimp_color_tool_class_init (GimpColorToolClass *klass)
{
  GObjectClass      *object_class = G_OBJECT_CLASS (klass);
  GimpToolClass     *tool_class   = GIMP_TOOL_CLASS (klass);
  GimpDrawToolClass *draw_class   = GIMP_DRAW_TOOL_CLASS (klass);

  gimp_color_tool_signals[PICKED] =
    g_signal_new ("picked",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (GimpColorToolClass, picked),
                  NULL, NULL,
                  gimp_marshal_VOID__ENUM_DOUBLE_DOUBLE_POINTER_BOXED_INT,
                  G_TYPE_NONE, 6,
                  GIMP_TYPE_COLOR_PICK_STATE,
                  G_TYPE_DOUBLE,
                  G_TYPE_DOUBLE,
                  G_TYPE_POINTER,
                  GIMP_TYPE_RGB | G_SIGNAL_TYPE_STATIC_SCOPE,
                  G_TYPE_INT);

  object_class->finalize     = gimp_color_tool_finalize;

  tool_class->button_press   = gimp_color_tool_button_press;
  tool_class->button_release = gimp_color_tool_button_release;
  tool_class->motion         = gimp_color_tool_motion;
  tool_class->oper_update    = gimp_color_tool_oper_update;
  tool_class->cursor_update  = gimp_color_tool_cursor_update;

  draw_class->draw           = gimp_color_tool_draw;

  klass->pick                = gimp_color_tool_real_pick;
  klass->picked              = gimp_color_tool_real_picked;
}

static void
gimp_color_tool_init (GimpColorTool *color_tool)
{
  GimpTool *tool = GIMP_TOOL (color_tool);

  gimp_tool_control_set_action_size (tool->control,
                                     "tools/tools-color-average-radius-set");

  color_tool->enabled             = FALSE;
  color_tool->center_x            = 0;
  color_tool->center_y            = 0;
  color_tool->pick_mode           = GIMP_COLOR_PICK_MODE_NONE;

  color_tool->options             = NULL;

  color_tool->sample_point        = NULL;
  color_tool->moving_sample_point = FALSE;
  color_tool->sample_point_x      = SAMPLE_POINT_POSITION_INVALID;
  color_tool->sample_point_y      = SAMPLE_POINT_POSITION_INVALID;
}

static void
gimp_color_tool_finalize (GObject *object)
{
  GimpColorTool *color_tool = GIMP_COLOR_TOOL (object);

  if (color_tool->options)
    {
      g_object_unref (color_tool->options);
      color_tool->options = NULL;
    }

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
gimp_color_tool_button_press (GimpTool            *tool,
                              const GimpCoords    *coords,
                              guint32              time,
                              GdkModifierType      state,
                              GimpButtonPressType  press_type,
                              GimpDisplay         *display)
{
  GimpColorTool    *color_tool = GIMP_COLOR_TOOL (tool);
  GimpDisplayShell *shell      = gimp_display_get_shell (display);

  /*  Chain up to activate the tool  */
  GIMP_TOOL_CLASS (parent_class)->button_press (tool, coords, time, state,
                                                press_type, display);

  if (! color_tool->enabled)
    return;

  if (color_tool->sample_point)
    {
      color_tool->moving_sample_point = TRUE;
      color_tool->sample_point_x      = color_tool->sample_point->x;
      color_tool->sample_point_y      = color_tool->sample_point->y;

      gimp_tool_control_set_scroll_lock (tool->control, TRUE);

      gimp_display_shell_selection_pause (shell);

      if (! gimp_draw_tool_is_active (GIMP_DRAW_TOOL (tool)))
        gimp_draw_tool_start (GIMP_DRAW_TOOL (tool), display);

      gimp_tool_push_status_coords (tool, display,
                                    gimp_tool_control_get_precision (tool->control),
                                    _("Move Sample Point: "),
                                    color_tool->sample_point_x,
                                    ", ",
                                    color_tool->sample_point_y,
                                    NULL);
    }
  else
    {
      color_tool->center_x = coords->x;
      color_tool->center_y = coords->y;

      gimp_draw_tool_start (GIMP_DRAW_TOOL (tool), display);

      gimp_color_tool_pick (color_tool, GIMP_COLOR_PICK_STATE_NEW,
                            coords->x, coords->y);
    }
}

static void
gimp_color_tool_button_release (GimpTool              *tool,
                                const GimpCoords      *coords,
                                guint32                time,
                                GdkModifierType        state,
                                GimpButtonReleaseType  release_type,
                                GimpDisplay           *display)
{
  GimpColorTool    *color_tool = GIMP_COLOR_TOOL (tool);
  GimpDisplayShell *shell      = gimp_display_get_shell (display);

  /*  Chain up to halt the tool  */
  GIMP_TOOL_CLASS (parent_class)->button_release (tool, coords, time, state,
                                                  release_type, display);

  if (! color_tool->enabled)
    return;

  if (color_tool->moving_sample_point)
    {
      GimpImage *image  = gimp_display_get_image (display);
      gint       width  = gimp_image_get_width  (image);
      gint       height = gimp_image_get_height (image);

      gimp_tool_pop_status (tool, display);

      gimp_tool_control_set_scroll_lock (tool->control, FALSE);
      gimp_draw_tool_stop (GIMP_DRAW_TOOL (tool));

      if (release_type == GIMP_BUTTON_RELEASE_CANCEL)
        {
          color_tool->moving_sample_point = FALSE;
          color_tool->sample_point_x      = SAMPLE_POINT_POSITION_INVALID;
          color_tool->sample_point_y      = SAMPLE_POINT_POSITION_INVALID;

          gimp_display_shell_selection_resume (shell);
          return;
        }

      if (color_tool->sample_point_x == SAMPLE_POINT_POSITION_INVALID ||
          color_tool->sample_point_x <  0                             ||
          color_tool->sample_point_x >= width                         ||
          color_tool->sample_point_y == SAMPLE_POINT_POSITION_INVALID ||
          color_tool->sample_point_y <  0                             ||
          color_tool->sample_point_y >= height)
        {
          if (color_tool->sample_point)
            {
              gimp_image_remove_sample_point (image,
                                              color_tool->sample_point, TRUE);
              color_tool->sample_point = NULL;
            }
        }
      else
        {
          if (color_tool->sample_point)
            {
              gimp_image_move_sample_point (image,
                                            color_tool->sample_point,
                                            color_tool->sample_point_x,
                                            color_tool->sample_point_y,
                                            TRUE);
            }
          else
            {
              color_tool->sample_point =
                gimp_image_add_sample_point_at_pos (image,
                                                    color_tool->sample_point_x,
                                                    color_tool->sample_point_y,
                                                    TRUE);
            }
        }

      gimp_display_shell_selection_resume (shell);
      gimp_image_flush (image);

      color_tool->moving_sample_point = FALSE;
      color_tool->sample_point_x      = SAMPLE_POINT_POSITION_INVALID;
      color_tool->sample_point_y      = SAMPLE_POINT_POSITION_INVALID;

      if (color_tool->sample_point)
        gimp_draw_tool_start (GIMP_DRAW_TOOL (tool), display);
    }
  else
    {
      gimp_draw_tool_stop (GIMP_DRAW_TOOL (tool));
    }
}

static void
gimp_color_tool_motion (GimpTool         *tool,
                        const GimpCoords *coords,
                        guint32           time,
                        GdkModifierType   state,
                        GimpDisplay      *display)
{
  GimpColorTool    *color_tool = GIMP_COLOR_TOOL (tool);
  GimpDisplayShell *shell      = gimp_display_get_shell (display);

  if (! color_tool->enabled)
    return;

  if (color_tool->moving_sample_point)
    {
      gint      tx, ty;
      gboolean  delete_point = FALSE;

      gimp_draw_tool_pause (GIMP_DRAW_TOOL (tool));

      gimp_display_shell_transform_xy (shell,
                                       coords->x, coords->y,
                                       &tx, &ty);

      if (tx < 0 || tx > shell->disp_width ||
          ty < 0 || ty > shell->disp_height)
        {
          color_tool->sample_point_x = SAMPLE_POINT_POSITION_INVALID;
          color_tool->sample_point_y = SAMPLE_POINT_POSITION_INVALID;

          delete_point = TRUE;
        }
      else
        {
          GimpImage *image  = gimp_display_get_image (display);
          gint       width  = gimp_image_get_width  (image);
          gint       height = gimp_image_get_height (image);

          color_tool->sample_point_x = floor (coords->x);
          color_tool->sample_point_y = floor (coords->y);

          if (color_tool->sample_point_x <  0     ||
              color_tool->sample_point_x >= width ||
              color_tool->sample_point_y <  0     ||
              color_tool->sample_point_y >= height)
            {
              delete_point = TRUE;
            }
        }

      gimp_draw_tool_resume (GIMP_DRAW_TOOL (tool));

      gimp_tool_pop_status (tool, display);

      if (delete_point)
        {
          gimp_tool_push_status (tool, display,
                                 color_tool->sample_point ?
                                 _("Remove Sample Point") :
                                 _("Cancel Sample Point"));
        }
      else
        {
          gimp_tool_push_status_coords (tool, display,
                                        gimp_tool_control_get_precision (tool->control),
                                        color_tool->sample_point ?
                                        _("Move Sample Point: ") :
                                        _("Add Sample Point: "),
                                        color_tool->sample_point_x,
                                        ", ",
                                        color_tool->sample_point_y,
                                        NULL);
        }
    }
  else
    {
      gimp_draw_tool_pause (GIMP_DRAW_TOOL (tool));

      color_tool->center_x = coords->x;
      color_tool->center_y = coords->y;

      gimp_draw_tool_resume (GIMP_DRAW_TOOL (tool));

      gimp_color_tool_pick (color_tool, GIMP_COLOR_PICK_STATE_UPDATE,
                            coords->x, coords->y);
    }
}

static void
gimp_color_tool_oper_update (GimpTool         *tool,
                             const GimpCoords *coords,
                             GdkModifierType   state,
                             gboolean          proximity,
                             GimpDisplay      *display)
{
  GimpColorTool    *color_tool   = GIMP_COLOR_TOOL (tool);
  GimpDisplayShell *shell        = gimp_display_get_shell (display);
  GimpImage        *image        = gimp_display_get_image (display);
  GimpSamplePoint  *sample_point = NULL;

  if (color_tool->enabled                               &&
      gimp_display_shell_get_show_sample_points (shell) &&
      proximity)
    {
      gint snap_distance = display->config->snap_distance;

      sample_point =
        gimp_image_find_sample_point (image,
                                      coords->x, coords->y,
                                      FUNSCALEX (shell, snap_distance),
                                      FUNSCALEY (shell, snap_distance));
    }

  if (color_tool->sample_point != sample_point)
    {
      GimpDrawTool *draw_tool = GIMP_DRAW_TOOL (tool);

      gimp_draw_tool_pause (draw_tool);

      if (gimp_draw_tool_is_active (draw_tool) &&
          draw_tool->display != display)
        gimp_draw_tool_stop (draw_tool);

      color_tool->sample_point = sample_point;

      if (! gimp_draw_tool_is_active (draw_tool))
        gimp_draw_tool_start (draw_tool, display);

      gimp_draw_tool_resume (draw_tool);
    }
}

static void
gimp_color_tool_cursor_update (GimpTool         *tool,
                               const GimpCoords *coords,
                               GdkModifierType   state,
                               GimpDisplay      *display)
{
  GimpColorTool *color_tool = GIMP_COLOR_TOOL (tool);
  GimpImage     *image      = gimp_display_get_image (display);

  if (color_tool->enabled)
    {
      if (color_tool->sample_point)
        {
          gimp_tool_set_cursor (tool, display,
                                GIMP_CURSOR_MOUSE,
                                GIMP_TOOL_CURSOR_COLOR_PICKER,
                                GIMP_CURSOR_MODIFIER_MOVE);
        }
      else
        {
          GimpCursorModifier modifier = GIMP_CURSOR_MODIFIER_BAD;

          if (gimp_image_coords_in_active_pickable (image, coords,
                                                    color_tool->options->sample_merged,
                                                    FALSE))
            {
              switch (color_tool->pick_mode)
                {
                case GIMP_COLOR_PICK_MODE_NONE:
                  modifier = GIMP_CURSOR_MODIFIER_NONE;
                  break;
                case GIMP_COLOR_PICK_MODE_FOREGROUND:
                  modifier = GIMP_CURSOR_MODIFIER_FOREGROUND;
                  break;
                case GIMP_COLOR_PICK_MODE_BACKGROUND:
                  modifier = GIMP_CURSOR_MODIFIER_BACKGROUND;
                  break;
                case GIMP_COLOR_PICK_MODE_PALETTE:
                  modifier = GIMP_CURSOR_MODIFIER_PLUS;
                  break;
                }
            }

          gimp_tool_set_cursor (tool, display,
                                GIMP_CURSOR_COLOR_PICKER,
                                GIMP_TOOL_CURSOR_COLOR_PICKER,
                                modifier);
        }

      return;  /*  don't chain up  */
    }

  GIMP_TOOL_CLASS (parent_class)->cursor_update (tool, coords, state, display);
}

static void
gimp_color_tool_draw (GimpDrawTool *draw_tool)
{
  GimpColorTool *color_tool = GIMP_COLOR_TOOL (draw_tool);

  if (color_tool->enabled)
    {
      if (color_tool->sample_point)
        {
          GimpImage      *image = gimp_display_get_image (draw_tool->display);
          GimpCanvasItem *item;
          gint            index;

          index = g_list_index (gimp_image_get_sample_points (image),
                                color_tool->sample_point) + 1;

          item = gimp_draw_tool_add_sample_point (draw_tool,
                                                  color_tool->sample_point->x,
                                                  color_tool->sample_point->y,
                                                  index);
          gimp_canvas_item_set_highlight (item, TRUE);
        }

      if (color_tool->moving_sample_point)
        {
          if (color_tool->sample_point_x != SAMPLE_POINT_POSITION_INVALID &&
              color_tool->sample_point_y != SAMPLE_POINT_POSITION_INVALID)
            {
              gimp_draw_tool_add_crosshair (draw_tool,
                                            color_tool->sample_point_x,
                                            color_tool->sample_point_y);
            }
        }
      else if (color_tool->options->sample_average &&
               gimp_tool_control_is_active (GIMP_TOOL (draw_tool)->control))
        {
          gdouble radius = color_tool->options->average_radius;

          gimp_draw_tool_add_rectangle (draw_tool,
                                        FALSE,
                                        color_tool->center_x - radius,
                                        color_tool->center_y - radius,
                                        2 * radius + 1,
                                        2 * radius + 1);
        }
    }
}

static gboolean
gimp_color_tool_real_pick (GimpColorTool  *color_tool,
                           gint            x,
                           gint            y,
                           const Babl    **sample_format,
                           GimpRGB        *color,
                           gint           *color_index)
{
  GimpTool  *tool  = GIMP_TOOL (color_tool);
  GimpImage *image = gimp_display_get_image (tool->display);

  g_return_val_if_fail (tool->display != NULL, FALSE);
  g_return_val_if_fail (tool->drawable != NULL, FALSE);

  return gimp_image_pick_color (image, tool->drawable, x, y,
                                color_tool->options->sample_merged,
                                color_tool->options->sample_average,
                                color_tool->options->average_radius,
                                sample_format,
                                color,
                                color_index);
}

static void
gimp_color_tool_real_picked (GimpColorTool      *color_tool,
                             GimpColorPickState  pick_state,
                             gdouble             x,
                             gdouble             y,
                             const Babl         *sample_format,
                             const GimpRGB      *color,
                             gint                color_index)
{
  GimpTool          *tool = GIMP_TOOL (color_tool);
  GimpContext       *context;

  /*  use this tool's own options here (NOT color_tool->options)  */
  context = GIMP_CONTEXT (gimp_tool_get_options (tool));

  if (color_tool->pick_mode == GIMP_COLOR_PICK_MODE_FOREGROUND ||
      color_tool->pick_mode == GIMP_COLOR_PICK_MODE_BACKGROUND)
    {
      GtkWidget *widget;

      if (babl_format_is_palette (sample_format))
        {
          widget = gimp_dialog_factory_find_widget (gimp_dialog_factory_get_singleton (),
                                                    "gimp-indexed-palette");
          if (widget)
            {
              GimpColormapEditor *editor;

              editor = GIMP_COLORMAP_EDITOR (gtk_bin_get_child (GTK_BIN (widget)));

              gimp_colormap_editor_set_index (editor, color_index, NULL);
            }
        }

      if (TRUE)
        {
          widget = gimp_dialog_factory_find_widget (gimp_dialog_factory_get_singleton (),
                                                    "gimp-palette-editor");
          if (widget)
            {
              GimpPaletteEditor *editor;
              gint               index;

              editor = GIMP_PALETTE_EDITOR (gtk_bin_get_child (GTK_BIN (widget)));

              index = gimp_palette_editor_get_index (editor, color);
              if (index != -1)
                gimp_palette_editor_set_index (editor, index, NULL);
            }
        }
    }

  switch (color_tool->pick_mode)
    {
    case GIMP_COLOR_PICK_MODE_NONE:
      break;

    case GIMP_COLOR_PICK_MODE_FOREGROUND:
      gimp_context_set_foreground (context, color);
      break;

    case GIMP_COLOR_PICK_MODE_BACKGROUND:
      gimp_context_set_background (context, color);
      break;

    case GIMP_COLOR_PICK_MODE_PALETTE:
      {
        GimpDisplayShell *shell   = gimp_display_get_shell (tool->display);
        GdkScreen        *screen  = gtk_widget_get_screen (GTK_WIDGET (shell));
        gint              monitor = gimp_widget_get_monitor (GTK_WIDGET (shell));
        GtkWidget        *dockable;

        dockable =
          gimp_window_strategy_show_dockable_dialog (GIMP_WINDOW_STRATEGY (gimp_get_window_strategy (tool->display->gimp)),
                                                     tool->display->gimp,
                                                     gimp_dialog_factory_get_singleton (),
                                                     screen,
                                                     monitor,
                                                     "gimp-palette-editor");

        if (dockable)
          {
            GtkWidget *palette_editor;
            GimpData  *data;

            /* don't blink like mad when updating */
            if (pick_state == GIMP_COLOR_PICK_STATE_UPDATE)
              gimp_dockable_blink_cancel (GIMP_DOCKABLE (dockable));

            palette_editor = gtk_bin_get_child (GTK_BIN (dockable));

            data = gimp_data_editor_get_data (GIMP_DATA_EDITOR (palette_editor));

            if (! data)
              {
                data = GIMP_DATA (gimp_context_get_palette (context));

                gimp_data_editor_set_data (GIMP_DATA_EDITOR (palette_editor),
                                           data);
              }

            gimp_palette_editor_pick_color (GIMP_PALETTE_EDITOR (palette_editor),
                                            color, pick_state);
          }
      }
      break;
    }
}

static void
gimp_color_tool_pick (GimpColorTool      *tool,
                      GimpColorPickState  pick_state,
                      gint                x,
                      gint                y)
{
  GimpColorToolClass *klass;
  const Babl         *sample_format;
  GimpRGB             color;
  gint                color_index;

  klass = GIMP_COLOR_TOOL_GET_CLASS (tool);

  if (klass->pick &&
      klass->pick (tool, x, y, &sample_format, &color, &color_index))
    {
      g_signal_emit (tool, gimp_color_tool_signals[PICKED], 0,
                     pick_state,
                     (gdouble) x, (gdouble) y,
                     sample_format, &color, color_index);
    }
}


void
gimp_color_tool_enable (GimpColorTool    *color_tool,
                        GimpColorOptions *options)
{
  GimpTool *tool;

  g_return_if_fail (GIMP_IS_COLOR_TOOL (color_tool));
  g_return_if_fail (GIMP_IS_COLOR_OPTIONS (options));

  tool = GIMP_TOOL (color_tool);

  if (gimp_tool_control_is_active (tool->control))
    {
      g_warning ("Trying to enable GimpColorTool while it is active.");
      return;
    }

  if (color_tool->options)
    g_object_unref (color_tool->options);

  color_tool->options = g_object_ref (options);

  color_tool->enabled = TRUE;
}

void
gimp_color_tool_disable (GimpColorTool *color_tool)
{
  GimpTool *tool;

  g_return_if_fail (GIMP_IS_COLOR_TOOL (color_tool));

  tool = GIMP_TOOL (color_tool);

  if (gimp_tool_control_is_active (tool->control))
    {
      g_warning ("Trying to disable GimpColorTool while it is active.");
      return;
    }

  if (color_tool->options)
    {
      g_object_unref (color_tool->options);
      color_tool->options = NULL;
    }

  color_tool->enabled = FALSE;
}

gboolean
gimp_color_tool_is_enabled (GimpColorTool *color_tool)
{
  return color_tool->enabled;
}

void
gimp_color_tool_start_sample_point (GimpTool    *tool,
                                    GimpDisplay *display)
{
  GimpColorTool *color_tool;

  g_return_if_fail (GIMP_IS_COLOR_TOOL (tool));
  g_return_if_fail (GIMP_IS_DISPLAY (display));

  color_tool = GIMP_COLOR_TOOL (tool);

  gimp_display_shell_selection_pause (gimp_display_get_shell (display));

  tool->display = display;
  gimp_tool_control_activate (tool->control);
  gimp_tool_control_set_scroll_lock (tool->control, TRUE);

  if (gimp_draw_tool_is_active (GIMP_DRAW_TOOL (tool)))
    gimp_draw_tool_stop (GIMP_DRAW_TOOL (tool));

  color_tool->sample_point        = NULL;
  color_tool->moving_sample_point = TRUE;
  color_tool->sample_point_x      = SAMPLE_POINT_POSITION_INVALID;
  color_tool->sample_point_y      = SAMPLE_POINT_POSITION_INVALID;

  gimp_tool_set_cursor (tool, display,
                        GIMP_CURSOR_MOUSE,
                        GIMP_TOOL_CURSOR_COLOR_PICKER,
                        GIMP_CURSOR_MODIFIER_MOVE);

  gimp_draw_tool_start (GIMP_DRAW_TOOL (tool), display);
}
