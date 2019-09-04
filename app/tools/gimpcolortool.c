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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <gegl.h>
#include <gtk/gtk.h>

#include "libgimpcolor/gimpcolor.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "tools-types.h"

#include "config/gimpdisplayconfig.h"

#include "core/gimp.h"
#include "core/gimpdata.h"
#include "core/gimpimage.h"
#include "core/gimpimage-pick-color.h"
#include "core/gimpimage-pick-item.h"
#include "core/gimpimage-sample-points.h"
#include "core/gimpmarshal.h"
#include "core/gimpsamplepoint.h"

#include "widgets/gimpcolormapeditor.h"
#include "widgets/gimpdialogfactory.h"
#include "widgets/gimpdockable.h"
#include "widgets/gimpdockcontainer.h"
#include "widgets/gimppaletteeditor.h"
#include "widgets/gimpwidgets-utils.h"
#include "widgets/gimpwindowstrategy.h"

#include "display/gimpcanvasitem.h"
#include "display/gimpdisplay.h"
#include "display/gimpdisplayshell.h"
#include "display/gimpdisplayshell-appearance.h"

#include "gimpcoloroptions.h"
#include "gimpcolortool.h"
#include "gimpsamplepointtool.h"
#include "gimptoolcontrol.h"

#include "gimp-intl.h"


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

static gboolean
              gimp_color_tool_real_can_pick  (GimpColorTool         *color_tool,
                                              const GimpCoords      *coords,
                                              GimpDisplay           *display);
static gboolean   gimp_color_tool_real_pick  (GimpColorTool         *color_tool,
                                              const GimpCoords      *coords,
                                              GimpDisplay           *display,
                                              const Babl           **sample_format,
                                              gpointer               pixel,
                                              GimpRGB               *color);
static void   gimp_color_tool_real_picked    (GimpColorTool         *color_tool,
                                              const GimpCoords      *coords,
                                              GimpDisplay           *display,
                                              GimpColorPickState     pick_state,
                                              const Babl            *sample_format,
                                              gpointer               pixel,
                                              const GimpRGB         *color);

static gboolean   gimp_color_tool_can_pick   (GimpColorTool         *tool,
                                              const GimpCoords      *coords,
                                              GimpDisplay           *display);
static void   gimp_color_tool_pick           (GimpColorTool         *tool,
                                              const GimpCoords      *coords,
                                              GimpDisplay           *display,
                                              GimpColorPickState     pick_state);


G_DEFINE_TYPE (GimpColorTool, gimp_color_tool, GIMP_TYPE_DRAW_TOOL)

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
                  gimp_marshal_VOID__POINTER_OBJECT_ENUM_POINTER_POINTER_BOXED,
                  G_TYPE_NONE, 6,
                  G_TYPE_POINTER,
                  GIMP_TYPE_DISPLAY,
                  GIMP_TYPE_COLOR_PICK_STATE,
                  G_TYPE_POINTER,
                  G_TYPE_POINTER,
                  GIMP_TYPE_RGB | G_SIGNAL_TYPE_STATIC_SCOPE);

  object_class->finalize     = gimp_color_tool_finalize;

  tool_class->button_press   = gimp_color_tool_button_press;
  tool_class->button_release = gimp_color_tool_button_release;
  tool_class->motion         = gimp_color_tool_motion;
  tool_class->oper_update    = gimp_color_tool_oper_update;
  tool_class->cursor_update  = gimp_color_tool_cursor_update;

  draw_class->draw           = gimp_color_tool_draw;

  klass->can_pick            = gimp_color_tool_real_can_pick;
  klass->pick                = gimp_color_tool_real_pick;
  klass->picked              = gimp_color_tool_real_picked;
}

static void
gimp_color_tool_init (GimpColorTool *color_tool)
{
  GimpTool *tool = GIMP_TOOL (color_tool);

  gimp_tool_control_set_action_size (tool->control,
                                     "tools/tools-color-average-radius-set");
}

static void
gimp_color_tool_finalize (GObject *object)
{
  GimpColorTool *color_tool = GIMP_COLOR_TOOL (object);

  g_clear_object (&color_tool->options);

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
  GimpColorTool *color_tool = GIMP_COLOR_TOOL (tool);

  if (color_tool->enabled)
    {
      if (color_tool->sample_point)
        {
          gimp_sample_point_tool_start_edit (tool, display,
                                             color_tool->sample_point);
        }
      else if (gimp_color_tool_can_pick (color_tool, coords, display))
        {
          gimp_color_tool_pick (color_tool, coords, display,
                                GIMP_COLOR_PICK_STATE_START);

          gimp_tool_control_activate (tool->control);
        }
    }
  else
    {
      GIMP_TOOL_CLASS (parent_class)->button_press (tool, coords, time, state,
                                                    press_type, display);
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
  GimpColorTool *color_tool = GIMP_COLOR_TOOL (tool);

  if (color_tool->enabled)
    {
      gimp_tool_control_halt (tool->control);

      if (! color_tool->sample_point &&
          gimp_color_tool_can_pick (color_tool, coords, display))
        {
          gimp_color_tool_pick (color_tool, coords, display,
                                GIMP_COLOR_PICK_STATE_END);
        }
    }
  else
    {
      GIMP_TOOL_CLASS (parent_class)->button_release (tool, coords, time, state,
                                                      release_type, display);
    }
}

static void
gimp_color_tool_motion (GimpTool         *tool,
                        const GimpCoords *coords,
                        guint32           time,
                        GdkModifierType   state,
                        GimpDisplay      *display)
{
  GimpColorTool *color_tool = GIMP_COLOR_TOOL (tool);

  if (color_tool->enabled)
    {
      if (! color_tool->sample_point)
        {
          gimp_draw_tool_pause (GIMP_DRAW_TOOL (tool));

          color_tool->can_pick = gimp_color_tool_can_pick (color_tool,
                                                           coords, display);
          color_tool->center_x = coords->x;
          color_tool->center_y = coords->y;

          if (color_tool->can_pick)
            {
              gimp_color_tool_pick (color_tool, coords, display,
                                    GIMP_COLOR_PICK_STATE_UPDATE);
            }

          gimp_draw_tool_resume (GIMP_DRAW_TOOL (tool));
        }
    }
  else
    {
      GIMP_TOOL_CLASS (parent_class)->motion (tool, coords, time, state,
                                              display);
    }
}

static void
gimp_color_tool_oper_update (GimpTool         *tool,
                             const GimpCoords *coords,
                             GdkModifierType   state,
                             gboolean          proximity,
                             GimpDisplay      *display)
{
  GimpColorTool *color_tool = GIMP_COLOR_TOOL (tool);

  if (color_tool->enabled)
    {
      GimpDrawTool     *draw_tool    = GIMP_DRAW_TOOL (tool);
      GimpDisplayShell *shell        = gimp_display_get_shell (display);
      GimpSamplePoint  *sample_point = NULL;

      gimp_draw_tool_pause (draw_tool);

      if (! draw_tool->widget &&

          gimp_draw_tool_is_active (draw_tool) &&
          (draw_tool->display != display ||
           ! proximity))
        {
          gimp_draw_tool_stop (draw_tool);
        }

      if (gimp_display_shell_get_show_sample_points (shell) &&
          proximity)
        {
          GimpImage *image         = gimp_display_get_image (display);
          gint       snap_distance = display->config->snap_distance;

          sample_point =
            gimp_image_pick_sample_point (image,
                                          coords->x, coords->y,
                                          FUNSCALEX (shell, snap_distance),
                                          FUNSCALEY (shell, snap_distance));
        }

      color_tool->sample_point = sample_point;

      color_tool->can_pick = gimp_color_tool_can_pick (color_tool,
                                                       coords, display);
      color_tool->center_x = coords->x;
      color_tool->center_y = coords->y;

      if (! draw_tool->widget &&

          ! gimp_draw_tool_is_active (draw_tool) &&
          proximity)
        {
          gimp_draw_tool_start (draw_tool, display);
        }

      gimp_draw_tool_resume (draw_tool);
    }
  else
    {
      GIMP_TOOL_CLASS (parent_class)->oper_update (tool, coords, state,
                                                   proximity, display);
    }
}

static void
gimp_color_tool_cursor_update (GimpTool         *tool,
                               const GimpCoords *coords,
                               GdkModifierType   state,
                               GimpDisplay      *display)
{
  GimpColorTool *color_tool = GIMP_COLOR_TOOL (tool);

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

          if (gimp_color_tool_can_pick (color_tool, coords, display))
            {
              switch (color_tool->pick_target)
                {
                case GIMP_COLOR_PICK_TARGET_NONE:
                  modifier = GIMP_CURSOR_MODIFIER_NONE;
                  break;
                case GIMP_COLOR_PICK_TARGET_FOREGROUND:
                  modifier = GIMP_CURSOR_MODIFIER_FOREGROUND;
                  break;
                case GIMP_COLOR_PICK_TARGET_BACKGROUND:
                  modifier = GIMP_CURSOR_MODIFIER_BACKGROUND;
                  break;
                case GIMP_COLOR_PICK_TARGET_PALETTE:
                  modifier = GIMP_CURSOR_MODIFIER_PLUS;
                  break;
                }
            }

          gimp_tool_set_cursor (tool, display,
                                GIMP_CURSOR_COLOR_PICKER,
                                GIMP_TOOL_CURSOR_COLOR_PICKER,
                                modifier);
        }
    }
  else
    {
      GIMP_TOOL_CLASS (parent_class)->cursor_update (tool, coords, state,
                                                     display);
    }
}

static void
gimp_color_tool_draw (GimpDrawTool *draw_tool)
{
  GimpColorTool *color_tool = GIMP_COLOR_TOOL (draw_tool);

  GIMP_DRAW_TOOL_CLASS (parent_class)->draw (draw_tool);

  if (color_tool->enabled)
    {
      if (color_tool->sample_point)
        {
          GimpImage      *image = gimp_display_get_image (draw_tool->display);
          GimpCanvasItem *item;
          gint            index;
          gint            x;
          gint            y;

          gimp_sample_point_get_position (color_tool->sample_point, &x, &y);

          index = g_list_index (gimp_image_get_sample_points (image),
                                color_tool->sample_point) + 1;

          item = gimp_draw_tool_add_sample_point (draw_tool, x, y, index);
          gimp_canvas_item_set_highlight (item, TRUE);
        }
      else if (color_tool->can_pick && color_tool->options->sample_average)
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
gimp_color_tool_real_can_pick (GimpColorTool    *color_tool,
                               const GimpCoords *coords,
                               GimpDisplay      *display)
{
  GimpDisplayShell *shell = gimp_display_get_shell (display);
  GimpImage        *image = gimp_display_get_image (display);

  return gimp_image_coords_in_active_pickable (image, coords,
                                               shell->show_all,
                                               color_tool->options->sample_merged,
                                               FALSE);
}

static gboolean
gimp_color_tool_real_pick (GimpColorTool     *color_tool,
                           const GimpCoords  *coords,
                           GimpDisplay       *display,
                           const Babl       **sample_format,
                           gpointer           pixel,
                           GimpRGB           *color)
{
  GimpDisplayShell *shell    = gimp_display_get_shell (display);
  GimpImage        *image    = gimp_display_get_image (display);
  GimpDrawable     *drawable = gimp_image_get_active_drawable (image);

  g_return_val_if_fail (drawable != NULL, FALSE);

  return gimp_image_pick_color (image, drawable,
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
gimp_color_tool_real_picked (GimpColorTool      *color_tool,
                             const GimpCoords   *coords,
                             GimpDisplay        *display,
                             GimpColorPickState  pick_state,
                             const Babl         *sample_format,
                             gpointer            pixel,
                             const GimpRGB      *color)
{
  GimpTool          *tool  = GIMP_TOOL (color_tool);
  GimpDisplayShell  *shell = gimp_display_get_shell (display);
  GimpImageWindow   *image_window;
  GimpDialogFactory *dialog_factory;
  GimpContext       *context;

  image_window   = gimp_display_shell_get_window (shell);
  dialog_factory = gimp_dock_container_get_dialog_factory (GIMP_DOCK_CONTAINER (image_window));

  /*  use this tool's own options here (NOT color_tool->options)  */
  context = GIMP_CONTEXT (gimp_tool_get_options (tool));

  if (color_tool->pick_target == GIMP_COLOR_PICK_TARGET_FOREGROUND ||
      color_tool->pick_target == GIMP_COLOR_PICK_TARGET_BACKGROUND)
    {
      GtkWidget *widget;

      widget = gimp_dialog_factory_find_widget (dialog_factory,
                                                "gimp-indexed-palette");
      if (widget)
        {
          GtkWidget *editor = gtk_bin_get_child (GTK_BIN (widget));
          GimpImage *image  = gimp_display_get_image (display);

          if (babl_format_is_palette (sample_format))
            {
              guchar *index  = pixel;

              gimp_colormap_editor_set_index (GIMP_COLORMAP_EDITOR (editor),
                                              *index, NULL);
            }
          else if (gimp_image_get_base_type (image) == GIMP_INDEXED)
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
              gint index = gimp_colormap_editor_get_index (GIMP_COLORMAP_EDITOR (editor),
                                                           color);
              if (index > -1)
                gimp_colormap_editor_set_index (GIMP_COLORMAP_EDITOR (editor),
                                                index, NULL);
            }
        }

      widget = gimp_dialog_factory_find_widget (dialog_factory,
                                                "gimp-palette-editor");
      if (widget)
        {
          GtkWidget *editor = gtk_bin_get_child (GTK_BIN (widget));
          gint       index;

          index = gimp_palette_editor_get_index (GIMP_PALETTE_EDITOR (editor),
                                                 color);
          if (index != -1)
            gimp_palette_editor_set_index (GIMP_PALETTE_EDITOR (editor),
                                           index, NULL);
        }
    }

  switch (color_tool->pick_target)
    {
    case GIMP_COLOR_PICK_TARGET_NONE:
      break;

    case GIMP_COLOR_PICK_TARGET_FOREGROUND:
      gimp_context_set_foreground (context, color);
      break;

    case GIMP_COLOR_PICK_TARGET_BACKGROUND:
      gimp_context_set_background (context, color);
      break;

    case GIMP_COLOR_PICK_TARGET_PALETTE:
      {
        GdkScreen *screen  = gtk_widget_get_screen (GTK_WIDGET (shell));
        gint       monitor = gimp_widget_get_monitor (GTK_WIDGET (shell));
        GtkWidget *dockable;

        dockable =
          gimp_window_strategy_show_dockable_dialog (GIMP_WINDOW_STRATEGY (gimp_get_window_strategy (display->gimp)),
                                                     display->gimp,
                                                     dialog_factory,
                                                     screen,
                                                     monitor,
                                                     "gimp-palette-editor");

        if (dockable)
          {
            GtkWidget *palette_editor;
            GimpData  *data;

            /* don't blink like mad when updating */
            if (pick_state != GIMP_COLOR_PICK_STATE_START)
              gimp_widget_blink_cancel (dockable);

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

static gboolean
gimp_color_tool_can_pick (GimpColorTool    *tool,
                          const GimpCoords *coords,
                          GimpDisplay      *display)
{
  GimpColorToolClass *klass;

  klass = GIMP_COLOR_TOOL_GET_CLASS (tool);

  return klass->can_pick && klass->can_pick (tool, coords, display);
}

static void
gimp_color_tool_pick (GimpColorTool      *tool,
                      const GimpCoords   *coords,
                      GimpDisplay        *display,
                      GimpColorPickState  pick_state)
{
  GimpColorToolClass *klass;
  const Babl         *sample_format;
  guchar              pixel[32];
  GimpRGB             color;

  klass = GIMP_COLOR_TOOL_GET_CLASS (tool);

  if (klass->pick &&
      klass->pick (tool, coords, display, &sample_format, pixel, &color))
    {
      g_signal_emit (tool, gimp_color_tool_signals[PICKED], 0,
                     coords, display, pick_state,
                     sample_format, pixel, &color);
    }
}


/*  public functions  */

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

  g_set_object (&color_tool->options, options);

  /*  color picking doesn't snap, see bug #768058  */
  color_tool->saved_snap_to = gimp_tool_control_get_snap_to (tool->control);
  gimp_tool_control_set_snap_to (tool->control, FALSE);

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

  g_clear_object (&color_tool->options);

  gimp_tool_control_set_snap_to (tool->control, color_tool->saved_snap_to);
  color_tool->saved_snap_to = FALSE;

  color_tool->enabled = FALSE;
}

gboolean
gimp_color_tool_is_enabled (GimpColorTool *color_tool)
{
  return color_tool->enabled;
}
