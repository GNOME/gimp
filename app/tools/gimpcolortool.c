/* LIGMA - The GNU Image Manipulation Program
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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <gegl.h>
#include <gtk/gtk.h>

#include "libligmacolor/ligmacolor.h"
#include "libligmawidgets/ligmawidgets.h"

#include "tools-types.h"

#include "config/ligmadisplayconfig.h"

#include "core/ligma.h"
#include "core/ligmadata.h"
#include "core/ligmaimage.h"
#include "core/ligmaimage-pick-color.h"
#include "core/ligmaimage-pick-item.h"
#include "core/ligmaimage-sample-points.h"
#include "core/ligmamarshal.h"
#include "core/ligmasamplepoint.h"

#include "widgets/ligmacolormapeditor.h"
#include "widgets/ligmadialogfactory.h"
#include "widgets/ligmadockable.h"
#include "widgets/ligmadockcontainer.h"
#include "widgets/ligmapaletteeditor.h"
#include "widgets/ligmawidgets-utils.h"
#include "widgets/ligmawindowstrategy.h"

#include "display/ligmacanvasitem.h"
#include "display/ligmadisplay.h"
#include "display/ligmadisplayshell.h"
#include "display/ligmadisplayshell-appearance.h"

#include "ligmacoloroptions.h"
#include "ligmacolortool.h"
#include "ligmasamplepointtool.h"
#include "ligmatoolcontrol.h"

#include "ligma-intl.h"


enum
{
  PICKED,
  LAST_SIGNAL
};


/*  local function prototypes  */

static void   ligma_color_tool_finalize       (GObject               *object);

static void   ligma_color_tool_button_press   (LigmaTool              *tool,
                                              const LigmaCoords      *coords,
                                              guint32                time,
                                              GdkModifierType        state,
                                              LigmaButtonPressType    press_type,
                                              LigmaDisplay           *display);
static void   ligma_color_tool_button_release (LigmaTool              *tool,
                                              const LigmaCoords      *coords,
                                              guint32                time,
                                              GdkModifierType        state,
                                              LigmaButtonReleaseType  release_type,
                                              LigmaDisplay           *display);
static void   ligma_color_tool_motion         (LigmaTool              *tool,
                                              const LigmaCoords      *coords,
                                              guint32                time,
                                              GdkModifierType        state,
                                              LigmaDisplay           *display);
static void   ligma_color_tool_oper_update    (LigmaTool              *tool,
                                              const LigmaCoords      *coords,
                                              GdkModifierType        state,
                                              gboolean               proximity,
                                              LigmaDisplay           *display);
static void   ligma_color_tool_cursor_update  (LigmaTool              *tool,
                                              const LigmaCoords      *coords,
                                              GdkModifierType        state,
                                              LigmaDisplay           *display);

static void   ligma_color_tool_draw           (LigmaDrawTool          *draw_tool);

static gboolean
              ligma_color_tool_real_can_pick  (LigmaColorTool         *color_tool,
                                              const LigmaCoords      *coords,
                                              LigmaDisplay           *display);
static gboolean   ligma_color_tool_real_pick  (LigmaColorTool         *color_tool,
                                              const LigmaCoords      *coords,
                                              LigmaDisplay           *display,
                                              const Babl           **sample_format,
                                              gpointer               pixel,
                                              LigmaRGB               *color);
static void   ligma_color_tool_real_picked    (LigmaColorTool         *color_tool,
                                              const LigmaCoords      *coords,
                                              LigmaDisplay           *display,
                                              LigmaColorPickState     pick_state,
                                              const Babl            *sample_format,
                                              gpointer               pixel,
                                              const LigmaRGB         *color);

static gboolean   ligma_color_tool_can_pick   (LigmaColorTool         *tool,
                                              const LigmaCoords      *coords,
                                              LigmaDisplay           *display);
static void   ligma_color_tool_pick           (LigmaColorTool         *tool,
                                              const LigmaCoords      *coords,
                                              LigmaDisplay           *display,
                                              LigmaColorPickState     pick_state);


G_DEFINE_TYPE (LigmaColorTool, ligma_color_tool, LIGMA_TYPE_DRAW_TOOL)

#define parent_class ligma_color_tool_parent_class

static guint ligma_color_tool_signals[LAST_SIGNAL] = { 0 };


static void
ligma_color_tool_class_init (LigmaColorToolClass *klass)
{
  GObjectClass      *object_class = G_OBJECT_CLASS (klass);
  LigmaToolClass     *tool_class   = LIGMA_TOOL_CLASS (klass);
  LigmaDrawToolClass *draw_class   = LIGMA_DRAW_TOOL_CLASS (klass);

  ligma_color_tool_signals[PICKED] =
    g_signal_new ("picked",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (LigmaColorToolClass, picked),
                  NULL, NULL,
                  ligma_marshal_VOID__POINTER_OBJECT_ENUM_POINTER_POINTER_BOXED,
                  G_TYPE_NONE, 6,
                  G_TYPE_POINTER,
                  LIGMA_TYPE_DISPLAY,
                  LIGMA_TYPE_COLOR_PICK_STATE,
                  G_TYPE_POINTER,
                  G_TYPE_POINTER,
                  LIGMA_TYPE_RGB | G_SIGNAL_TYPE_STATIC_SCOPE);

  object_class->finalize     = ligma_color_tool_finalize;

  tool_class->button_press   = ligma_color_tool_button_press;
  tool_class->button_release = ligma_color_tool_button_release;
  tool_class->motion         = ligma_color_tool_motion;
  tool_class->oper_update    = ligma_color_tool_oper_update;
  tool_class->cursor_update  = ligma_color_tool_cursor_update;

  draw_class->draw           = ligma_color_tool_draw;

  klass->can_pick            = ligma_color_tool_real_can_pick;
  klass->pick                = ligma_color_tool_real_pick;
  klass->picked              = ligma_color_tool_real_picked;
}

static void
ligma_color_tool_init (LigmaColorTool *color_tool)
{
  LigmaTool *tool = LIGMA_TOOL (color_tool);

  ligma_tool_control_set_action_size (tool->control,
                                     "tools/tools-color-average-radius-set");
}

static void
ligma_color_tool_finalize (GObject *object)
{
  LigmaColorTool *color_tool = LIGMA_COLOR_TOOL (object);

  g_clear_object (&color_tool->options);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
ligma_color_tool_button_press (LigmaTool            *tool,
                              const LigmaCoords    *coords,
                              guint32              time,
                              GdkModifierType      state,
                              LigmaButtonPressType  press_type,
                              LigmaDisplay         *display)
{
  LigmaColorTool *color_tool = LIGMA_COLOR_TOOL (tool);

  if (color_tool->enabled)
    {
      if (color_tool->sample_point)
        {
          ligma_sample_point_tool_start_edit (tool, display,
                                             color_tool->sample_point);
        }
      else if (ligma_color_tool_can_pick (color_tool, coords, display))
        {
          ligma_color_tool_pick (color_tool, coords, display,
                                LIGMA_COLOR_PICK_STATE_START);

          ligma_tool_control_activate (tool->control);
        }
    }
  else
    {
      LIGMA_TOOL_CLASS (parent_class)->button_press (tool, coords, time, state,
                                                    press_type, display);
    }
}

static void
ligma_color_tool_button_release (LigmaTool              *tool,
                                const LigmaCoords      *coords,
                                guint32                time,
                                GdkModifierType        state,
                                LigmaButtonReleaseType  release_type,
                                LigmaDisplay           *display)
{
  LigmaColorTool *color_tool = LIGMA_COLOR_TOOL (tool);

  if (color_tool->enabled)
    {
      ligma_tool_control_halt (tool->control);

      if (! color_tool->sample_point &&
          ligma_color_tool_can_pick (color_tool, coords, display))
        {
          ligma_color_tool_pick (color_tool, coords, display,
                                LIGMA_COLOR_PICK_STATE_END);
        }
    }
  else
    {
      LIGMA_TOOL_CLASS (parent_class)->button_release (tool, coords, time, state,
                                                      release_type, display);
    }
}

static void
ligma_color_tool_motion (LigmaTool         *tool,
                        const LigmaCoords *coords,
                        guint32           time,
                        GdkModifierType   state,
                        LigmaDisplay      *display)
{
  LigmaColorTool *color_tool = LIGMA_COLOR_TOOL (tool);

  if (color_tool->enabled)
    {
      if (! color_tool->sample_point)
        {
          ligma_draw_tool_pause (LIGMA_DRAW_TOOL (tool));

          color_tool->can_pick = ligma_color_tool_can_pick (color_tool,
                                                           coords, display);
          color_tool->center_x = coords->x;
          color_tool->center_y = coords->y;

          if (color_tool->can_pick)
            {
              ligma_color_tool_pick (color_tool, coords, display,
                                    LIGMA_COLOR_PICK_STATE_UPDATE);
            }

          ligma_draw_tool_resume (LIGMA_DRAW_TOOL (tool));
        }
    }
  else
    {
      LIGMA_TOOL_CLASS (parent_class)->motion (tool, coords, time, state,
                                              display);
    }
}

static void
ligma_color_tool_oper_update (LigmaTool         *tool,
                             const LigmaCoords *coords,
                             GdkModifierType   state,
                             gboolean          proximity,
                             LigmaDisplay      *display)
{
  LigmaColorTool *color_tool = LIGMA_COLOR_TOOL (tool);

  if (color_tool->enabled)
    {
      LigmaDrawTool     *draw_tool    = LIGMA_DRAW_TOOL (tool);
      LigmaDisplayShell *shell        = ligma_display_get_shell (display);
      LigmaSamplePoint  *sample_point = NULL;

      ligma_draw_tool_pause (draw_tool);

      if (! draw_tool->widget &&

          ligma_draw_tool_is_active (draw_tool) &&
          (draw_tool->display != display ||
           ! proximity))
        {
          ligma_draw_tool_stop (draw_tool);
        }

      if (ligma_display_shell_get_show_sample_points (shell) &&
          proximity)
        {
          LigmaImage *image         = ligma_display_get_image (display);
          gint       snap_distance = display->config->snap_distance;

          sample_point =
            ligma_image_pick_sample_point (image,
                                          coords->x, coords->y,
                                          FUNSCALEX (shell, snap_distance),
                                          FUNSCALEY (shell, snap_distance));
        }

      color_tool->sample_point = sample_point;

      color_tool->can_pick = ligma_color_tool_can_pick (color_tool,
                                                       coords, display);
      color_tool->center_x = coords->x;
      color_tool->center_y = coords->y;

      if (! draw_tool->widget &&

          ! ligma_draw_tool_is_active (draw_tool) &&
          proximity)
        {
          ligma_draw_tool_start (draw_tool, display);
        }

      ligma_draw_tool_resume (draw_tool);
    }
  else
    {
      LIGMA_TOOL_CLASS (parent_class)->oper_update (tool, coords, state,
                                                   proximity, display);
    }
}

static void
ligma_color_tool_cursor_update (LigmaTool         *tool,
                               const LigmaCoords *coords,
                               GdkModifierType   state,
                               LigmaDisplay      *display)
{
  LigmaColorTool *color_tool = LIGMA_COLOR_TOOL (tool);

  if (color_tool->enabled)
    {
      if (color_tool->sample_point)
        {
          ligma_tool_set_cursor (tool, display,
                                LIGMA_CURSOR_MOUSE,
                                LIGMA_TOOL_CURSOR_COLOR_PICKER,
                                LIGMA_CURSOR_MODIFIER_MOVE);
        }
      else
        {
          LigmaCursorModifier modifier = LIGMA_CURSOR_MODIFIER_BAD;

          if (ligma_color_tool_can_pick (color_tool, coords, display))
            {
              switch (color_tool->pick_target)
                {
                case LIGMA_COLOR_PICK_TARGET_NONE:
                  modifier = LIGMA_CURSOR_MODIFIER_NONE;
                  break;
                case LIGMA_COLOR_PICK_TARGET_FOREGROUND:
                  modifier = LIGMA_CURSOR_MODIFIER_FOREGROUND;
                  break;
                case LIGMA_COLOR_PICK_TARGET_BACKGROUND:
                  modifier = LIGMA_CURSOR_MODIFIER_BACKGROUND;
                  break;
                case LIGMA_COLOR_PICK_TARGET_PALETTE:
                  modifier = LIGMA_CURSOR_MODIFIER_PLUS;
                  break;
                }
            }

          ligma_tool_set_cursor (tool, display,
                                LIGMA_CURSOR_COLOR_PICKER,
                                LIGMA_TOOL_CURSOR_COLOR_PICKER,
                                modifier);
        }
    }
  else
    {
      LIGMA_TOOL_CLASS (parent_class)->cursor_update (tool, coords, state,
                                                     display);
    }
}

static void
ligma_color_tool_draw (LigmaDrawTool *draw_tool)
{
  LigmaColorTool *color_tool = LIGMA_COLOR_TOOL (draw_tool);

  LIGMA_DRAW_TOOL_CLASS (parent_class)->draw (draw_tool);

  if (color_tool->enabled)
    {
      if (color_tool->sample_point)
        {
          LigmaImage      *image = ligma_display_get_image (draw_tool->display);
          LigmaCanvasItem *item;
          gint            index;
          gint            x;
          gint            y;

          ligma_sample_point_get_position (color_tool->sample_point, &x, &y);

          index = g_list_index (ligma_image_get_sample_points (image),
                                color_tool->sample_point) + 1;

          item = ligma_draw_tool_add_sample_point (draw_tool, x, y, index);
          ligma_canvas_item_set_highlight (item, TRUE);
        }
      else if (color_tool->can_pick && color_tool->options->sample_average)
        {
          gdouble radius = color_tool->options->average_radius;

          ligma_draw_tool_add_rectangle (draw_tool,
                                        FALSE,
                                        color_tool->center_x - radius,
                                        color_tool->center_y - radius,
                                        2 * radius + 1,
                                        2 * radius + 1);
        }
    }
}

static gboolean
ligma_color_tool_real_can_pick (LigmaColorTool    *color_tool,
                               const LigmaCoords *coords,
                               LigmaDisplay      *display)
{
  LigmaDisplayShell *shell = ligma_display_get_shell (display);
  LigmaImage        *image = ligma_display_get_image (display);

  return ligma_image_coords_in_active_pickable (image, coords,
                                               shell->show_all,
                                               color_tool->options->sample_merged,
                                               FALSE);
}

static gboolean
ligma_color_tool_real_pick (LigmaColorTool     *color_tool,
                           const LigmaCoords  *coords,
                           LigmaDisplay       *display,
                           const Babl       **sample_format,
                           gpointer           pixel,
                           LigmaRGB           *color)
{
  LigmaDisplayShell *shell     = ligma_display_get_shell (display);
  LigmaImage        *image     = ligma_display_get_image (display);
  GList            *drawables = ligma_image_get_selected_drawables (image);

  g_return_val_if_fail (drawables != NULL, FALSE);

  return ligma_image_pick_color (image, drawables,
                                coords->x, coords->y,
                                shell->show_all,
                                color_tool->options->sample_merged,
                                color_tool->options->sample_average,
                                color_tool->options->average_radius,
                                sample_format,
                                pixel,
                                color);
}

static void
ligma_color_tool_real_picked (LigmaColorTool      *color_tool,
                             const LigmaCoords   *coords,
                             LigmaDisplay        *display,
                             LigmaColorPickState  pick_state,
                             const Babl         *sample_format,
                             gpointer            pixel,
                             const LigmaRGB      *color)
{
  LigmaTool          *tool  = LIGMA_TOOL (color_tool);
  LigmaDisplayShell  *shell = ligma_display_get_shell (display);
  LigmaImageWindow   *image_window;
  LigmaDialogFactory *dialog_factory;
  LigmaContext       *context;

  image_window   = ligma_display_shell_get_window (shell);
  dialog_factory = ligma_dock_container_get_dialog_factory (LIGMA_DOCK_CONTAINER (image_window));

  /*  use this tool's own options here (NOT color_tool->options)  */
  context = LIGMA_CONTEXT (ligma_tool_get_options (tool));

  if (color_tool->pick_target == LIGMA_COLOR_PICK_TARGET_FOREGROUND ||
      color_tool->pick_target == LIGMA_COLOR_PICK_TARGET_BACKGROUND)
    {
      GtkWidget *widget;

      widget = ligma_dialog_factory_find_widget (dialog_factory,
                                                "ligma-indexed-palette");
      if (widget)
        {
          GtkWidget *editor = gtk_bin_get_child (GTK_BIN (widget));
          LigmaImage *image  = ligma_display_get_image (display);

          if (babl_format_is_palette (sample_format))
            {
              guchar *index  = pixel;

              ligma_colormap_editor_set_index (LIGMA_COLORMAP_EDITOR (editor),
                                              *index, NULL);
            }
          else if (ligma_image_get_base_type (image) == LIGMA_INDEXED)
            {
              /* When Sample merged is set, we don't have the index
               * information and it is possible to pick colors out of
               * the colormap (with compositing). In such a case, the
               * sample format will not be a palette format even though
               * the image is indexed. Still search if the color exists
               * in the colormap.
               * Note that even if it does, we might still pick the
               * wrong color, since several indexes may contain the same
               * color and we can't know for sure which is the right
               * one.
               */
              gint index = ligma_colormap_editor_get_index (LIGMA_COLORMAP_EDITOR (editor),
                                                           color);
              if (index > -1)
                ligma_colormap_editor_set_index (LIGMA_COLORMAP_EDITOR (editor),
                                                index, NULL);
            }
        }

      widget = ligma_dialog_factory_find_widget (dialog_factory,
                                                "ligma-palette-editor");
      if (widget)
        {
          GtkWidget *editor = gtk_bin_get_child (GTK_BIN (widget));
          gint       index;

          index = ligma_palette_editor_get_index (LIGMA_PALETTE_EDITOR (editor),
                                                 color);
          if (index != -1)
            ligma_palette_editor_set_index (LIGMA_PALETTE_EDITOR (editor),
                                           index, NULL);
        }
    }

  switch (color_tool->pick_target)
    {
    case LIGMA_COLOR_PICK_TARGET_NONE:
      break;

    case LIGMA_COLOR_PICK_TARGET_FOREGROUND:
      ligma_context_set_foreground (context, color);
      break;

    case LIGMA_COLOR_PICK_TARGET_BACKGROUND:
      ligma_context_set_background (context, color);
      break;

    case LIGMA_COLOR_PICK_TARGET_PALETTE:
      {
        GdkMonitor *monitor = ligma_widget_get_monitor (GTK_WIDGET (shell));
        GtkWidget  *dockable;

        dockable =
          ligma_window_strategy_show_dockable_dialog (LIGMA_WINDOW_STRATEGY (ligma_get_window_strategy (display->ligma)),
                                                     display->ligma,
                                                     dialog_factory,
                                                     monitor,
                                                     "ligma-palette-editor");

        if (dockable)
          {
            GtkWidget *palette_editor;
            LigmaData  *data;

            /* don't blink like mad when updating */
            if (pick_state != LIGMA_COLOR_PICK_STATE_START)
              ligma_widget_blink_cancel (dockable);

            palette_editor = gtk_bin_get_child (GTK_BIN (dockable));

            data = ligma_data_editor_get_data (LIGMA_DATA_EDITOR (palette_editor));

            if (! data)
              {
                data = LIGMA_DATA (ligma_context_get_palette (context));

                ligma_data_editor_set_data (LIGMA_DATA_EDITOR (palette_editor),
                                           data);
              }

            ligma_palette_editor_pick_color (LIGMA_PALETTE_EDITOR (palette_editor),
                                            color, pick_state);
          }
      }
      break;
    }
}

static gboolean
ligma_color_tool_can_pick (LigmaColorTool    *tool,
                          const LigmaCoords *coords,
                          LigmaDisplay      *display)
{
  LigmaColorToolClass *klass;

  klass = LIGMA_COLOR_TOOL_GET_CLASS (tool);

  return klass->can_pick && klass->can_pick (tool, coords, display);
}

static void
ligma_color_tool_pick (LigmaColorTool      *tool,
                      const LigmaCoords   *coords,
                      LigmaDisplay        *display,
                      LigmaColorPickState  pick_state)
{
  LigmaColorToolClass *klass;
  const Babl         *sample_format;
  gdouble             pixel[4];
  LigmaRGB             color;

  klass = LIGMA_COLOR_TOOL_GET_CLASS (tool);

  if (klass->pick &&
      klass->pick (tool, coords, display, &sample_format, pixel, &color))
    {
      g_signal_emit (tool, ligma_color_tool_signals[PICKED], 0,
                     coords, display, pick_state,
                     sample_format, pixel, &color);
    }
}


/*  public functions  */

void
ligma_color_tool_enable (LigmaColorTool    *color_tool,
                        LigmaColorOptions *options)
{
  LigmaTool *tool;

  g_return_if_fail (LIGMA_IS_COLOR_TOOL (color_tool));
  g_return_if_fail (LIGMA_IS_COLOR_OPTIONS (options));

  tool = LIGMA_TOOL (color_tool);

  if (ligma_tool_control_is_active (tool->control))
    {
      g_warning ("Trying to enable LigmaColorTool while it is active.");
      return;
    }

  g_set_object (&color_tool->options, options);

  /*  color picking doesn't snap, see bug #768058  */
  color_tool->saved_snap_to = ligma_tool_control_get_snap_to (tool->control);
  ligma_tool_control_set_snap_to (tool->control, FALSE);

  color_tool->enabled = TRUE;
}

void
ligma_color_tool_disable (LigmaColorTool *color_tool)
{
  LigmaTool *tool;

  g_return_if_fail (LIGMA_IS_COLOR_TOOL (color_tool));

  tool = LIGMA_TOOL (color_tool);

  if (ligma_tool_control_is_active (tool->control))
    {
      g_warning ("Trying to disable LigmaColorTool while it is active.");
      return;
    }

  g_clear_object (&color_tool->options);

  ligma_tool_control_set_snap_to (tool->control, color_tool->saved_snap_to);
  color_tool->saved_snap_to = FALSE;

  color_tool->enabled = FALSE;
}

gboolean
ligma_color_tool_is_enabled (LigmaColorTool *color_tool)
{
  return color_tool->enabled;
}
