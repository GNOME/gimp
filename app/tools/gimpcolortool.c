/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995-2001 Spencer Kimball, Peter Mattis, and others
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

#include "display/gimpdisplay.h"
#include "display/gimpdisplayshell.h"
#include "display/gimpdisplayshell-appearance.h"
#include "display/gimpdisplayshell-draw.h"
#include "display/gimpdisplayshell-selection.h"
#include "display/gimpdisplayshell-transform.h"

#include "gimpcoloroptions.h"
#include "gimpcolortool.h"
#include "gimptoolcontrol.h"

#include "gimp-intl.h"


enum
{
  PICKED,
  LAST_SIGNAL
};


/*  local function prototypes  */

static void   gimp_color_tool_finalize       (GObject               *object);

static void   gimp_color_tool_control        (GimpTool              *tool,
                                              GimpToolAction         action,
                                              GimpDisplay           *display);
static void   gimp_color_tool_button_press   (GimpTool              *tool,
                                              GimpCoords            *coords,
                                              guint32                time,
                                              GdkModifierType        state,
                                              GimpDisplay           *display);
static void   gimp_color_tool_button_release (GimpTool              *tool,
                                              GimpCoords            *coords,
                                              guint32                time,
                                              GdkModifierType        state,
                                              GimpButtonReleaseType  release_type,
                                              GimpDisplay           *display);
static void   gimp_color_tool_motion         (GimpTool              *tool,
                                              GimpCoords            *coords,
                                              guint32                time,
                                              GdkModifierType        state,
                                              GimpDisplay           *display);
static void   gimp_color_tool_oper_update    (GimpTool              *tool,
                                              GimpCoords            *coords,
                                              GdkModifierType        state,
                                              gboolean               proximity,
                                              GimpDisplay           *display);
static void   gimp_color_tool_cursor_update  (GimpTool              *tool,
                                              GimpCoords            *coords,
                                              GdkModifierType        state,
                                              GimpDisplay           *display);

static void   gimp_color_tool_draw           (GimpDrawTool          *draw_tool);

static gboolean   gimp_color_tool_real_pick  (GimpColorTool         *color_tool,
                                              gint                   x,
                                              gint                   y,
                                              GimpImageType         *sample_type,
                                              GimpRGB               *color,
                                              gint                  *color_index);
static void   gimp_color_tool_pick           (GimpColorTool         *tool,
                                              GimpColorPickState     pick_state,
                                              gint                   x,
                                              gint                   y);
static void   gimp_color_tool_real_picked    (GimpColorTool         *color_tool,
                                              GimpColorPickState     pick_state,
                                              GimpImageType          sample_type,
                                              GimpRGB               *color,
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
                  gimp_marshal_VOID__ENUM_ENUM_BOXED_INT,
                  G_TYPE_NONE, 4,
                  GIMP_TYPE_COLOR_PICK_STATE,
                  GIMP_TYPE_IMAGE_TYPE,
                  GIMP_TYPE_RGB | G_SIGNAL_TYPE_STATIC_SCOPE,
                  G_TYPE_INT);

  object_class->finalize     = gimp_color_tool_finalize;

  tool_class->control        = gimp_color_tool_control;
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

  gimp_tool_control_set_action_value_2 (tool->control,
                                        "tools/tools-color-average-radius-set");

  color_tool->enabled             = FALSE;
  color_tool->center_x            = 0;
  color_tool->center_y            = 0;
  color_tool->pick_mode           = GIMP_COLOR_PICK_MODE_NONE;

  color_tool->options             = NULL;

  color_tool->sample_point        = NULL;
  color_tool->moving_sample_point = FALSE;
  color_tool->sample_point_x      = -1;
  color_tool->sample_point_y      = -1;
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
gimp_color_tool_control (GimpTool       *tool,
                         GimpToolAction  action,
                         GimpDisplay    *display)
{
  GimpColorTool    *color_tool = GIMP_COLOR_TOOL (tool);
  GimpDisplayShell *shell      = GIMP_DISPLAY_SHELL (display->shell);

  switch (action)
    {
    case GIMP_TOOL_ACTION_PAUSE:
      break;

    case GIMP_TOOL_ACTION_RESUME:
      if (color_tool->sample_point &&
          gimp_display_shell_get_show_sample_points (shell))
        gimp_display_shell_draw_sample_point (shell,
                                              color_tool->sample_point, TRUE);
      break;

    case GIMP_TOOL_ACTION_HALT:
      if (color_tool->sample_point &&
          gimp_display_shell_get_show_sample_points (shell))
        gimp_display_shell_draw_sample_point (shell,
                                              color_tool->sample_point, FALSE);
      break;
    }

  GIMP_TOOL_CLASS (parent_class)->control (tool, action, display);
}

static void
gimp_color_tool_button_press (GimpTool        *tool,
                              GimpCoords      *coords,
                              guint32          time,
                              GdkModifierType  state,
                              GimpDisplay     *display)
{
  GimpColorTool    *color_tool = GIMP_COLOR_TOOL (tool);
  GimpDisplayShell *shell      = GIMP_DISPLAY_SHELL (display->shell);

  /*  Chain up to activate the tool  */
  GIMP_TOOL_CLASS (parent_class)->button_press (tool, coords, time, state,
                                                display);

  if (! color_tool->enabled)
    return;

  if (color_tool->sample_point)
    {
      color_tool->moving_sample_point = TRUE;
      color_tool->sample_point_x      = color_tool->sample_point->x;
      color_tool->sample_point_y      = color_tool->sample_point->y;

      gimp_display_shell_selection_control (shell, GIMP_SELECTION_PAUSE);

      gimp_draw_tool_start (GIMP_DRAW_TOOL (tool), display);

      gimp_tool_push_status_coords (tool, display,
                                    _("Move Sample Point: "),
                                    color_tool->sample_point_x,
                                    ", ",
                                    color_tool->sample_point_y,
                                    NULL);
    }
  else
    {
      gint off_x, off_y;

      /*  Keep the coordinates of the target  */
      gimp_item_offsets (GIMP_ITEM (tool->drawable), &off_x, &off_y);

      color_tool->center_x = coords->x - off_x;
      color_tool->center_y = coords->y - off_y;

      gimp_draw_tool_start (GIMP_DRAW_TOOL (tool), display);

      gimp_color_tool_pick (color_tool, GIMP_COLOR_PICK_STATE_NEW,
                            coords->x, coords->y);
    }
}

static void
gimp_color_tool_button_release (GimpTool              *tool,
                                GimpCoords            *coords,
                                guint32                time,
                                GdkModifierType        state,
                                GimpButtonReleaseType  release_type,
                                GimpDisplay           *display)
{
  GimpColorTool    *color_tool = GIMP_COLOR_TOOL (tool);
  GimpDisplayShell *shell      = GIMP_DISPLAY_SHELL (display->shell);

  /*  Chain up to halt the tool  */
  GIMP_TOOL_CLASS (parent_class)->button_release (tool, coords, time, state,
                                                  release_type, display);

  if (! color_tool->enabled)
    return;

  if (color_tool->moving_sample_point)
    {
      gint x, y, width, height;

      gimp_tool_pop_status (tool, display);

      gimp_draw_tool_stop (GIMP_DRAW_TOOL (tool));

      if (release_type == GIMP_BUTTON_RELEASE_CANCEL)
        {
          color_tool->moving_sample_point = FALSE;
          color_tool->sample_point_x      = -1;
          color_tool->sample_point_y      = -1;

          gimp_display_shell_selection_control (shell, GIMP_SELECTION_RESUME);
          return;
        }

      gimp_display_shell_untransform_viewport (shell, &x, &y, &width, &height);

      if ((color_tool->sample_point_x <  x              ||
           color_tool->sample_point_x > (x + width - 1) ||
           color_tool->sample_point_y < y               ||
           color_tool->sample_point_y > (y + height - 1)))
        {
          if (color_tool->sample_point)
            {
              gimp_image_remove_sample_point (display->image,
                                              color_tool->sample_point, TRUE);
              color_tool->sample_point = NULL;
            }
        }
      else
        {
          if (color_tool->sample_point)
            {
              gimp_image_move_sample_point (display->image,
                                            color_tool->sample_point,
                                            color_tool->sample_point_x,
                                            color_tool->sample_point_y,
                                            TRUE);
            }
          else
            {
              color_tool->sample_point =
                gimp_image_add_sample_point_at_pos (display->image,
                                                    color_tool->sample_point_x,
                                                    color_tool->sample_point_y,
                                                    TRUE);
            }
        }

      gimp_display_shell_selection_control (shell, GIMP_SELECTION_RESUME);
      gimp_image_flush (display->image);

      if (color_tool->sample_point)
        gimp_display_shell_draw_sample_point (shell, color_tool->sample_point,
                                              TRUE);

      color_tool->moving_sample_point = FALSE;
      color_tool->sample_point_x      = -1;
      color_tool->sample_point_y      = -1;
    }
  else
    {
      gimp_draw_tool_stop (GIMP_DRAW_TOOL (tool));
    }
}

static void
gimp_color_tool_motion (GimpTool        *tool,
                        GimpCoords      *coords,
                        guint32          time,
                        GdkModifierType  state,
                        GimpDisplay     *display)
{
  GimpColorTool    *color_tool = GIMP_COLOR_TOOL (tool);
  GimpDisplayShell *shell      = GIMP_DISPLAY_SHELL (display->shell);

  if (! color_tool->enabled)
    return;

  if (color_tool->moving_sample_point)
    {
      gint      tx, ty;
      gboolean  delete_point = FALSE;

      gimp_draw_tool_pause (GIMP_DRAW_TOOL (tool));

      gimp_display_shell_transform_xy (shell,
                                       coords->x, coords->y,
                                       &tx, &ty,
                                       FALSE);

      if (tx < 0 || tx > shell->disp_width ||
          ty < 0 || ty > shell->disp_height)
        {
          color_tool->sample_point_x = -1;
          color_tool->sample_point_y = -1;

          delete_point = TRUE;
        }
      else
        {
          gint x, y, width, height;

          color_tool->sample_point_x = floor (coords->x);
          color_tool->sample_point_y = floor (coords->y);

          gimp_display_shell_untransform_viewport (shell, &x, &y,
                                                   &width, &height);

          if ((color_tool->sample_point_x <  x              ||
               color_tool->sample_point_x > (x + width - 1) ||
               color_tool->sample_point_y < y               ||
               color_tool->sample_point_y > (y + height - 1)))
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
      gint off_x, off_y;

      gimp_draw_tool_pause (GIMP_DRAW_TOOL (tool));

      gimp_item_offsets (GIMP_ITEM (tool->drawable), &off_x, &off_y);

      color_tool->center_x = coords->x - off_x;
      color_tool->center_y = coords->y - off_y;

      gimp_draw_tool_resume (GIMP_DRAW_TOOL (tool));

      gimp_color_tool_pick (color_tool, GIMP_COLOR_PICK_STATE_UPDATE,
                            coords->x, coords->y);
    }
}

static void
gimp_color_tool_oper_update (GimpTool        *tool,
                             GimpCoords      *coords,
                             GdkModifierType  state,
                             gboolean         proximity,
                             GimpDisplay     *display)
{
  GimpColorTool    *color_tool   = GIMP_COLOR_TOOL (tool);
  GimpDisplayShell *shell        = GIMP_DISPLAY_SHELL (display->shell);
  GimpSamplePoint  *sample_point = NULL;

  if (color_tool->enabled &&
      gimp_display_shell_get_show_sample_points (shell) && proximity)
    {
      gint snap_distance;

      snap_distance =
        GIMP_DISPLAY_CONFIG (display->image->gimp->config)->snap_distance;

      sample_point =
        gimp_image_find_sample_point (display->image,
                                      coords->x, coords->y,
                                      FUNSCALEX (shell, snap_distance),
                                      FUNSCALEY (shell, snap_distance));
    }

  if (color_tool->sample_point && color_tool->sample_point != sample_point)
    gimp_image_update_sample_point (shell->display->image,
                                    color_tool->sample_point);

  color_tool->sample_point = sample_point;

  if (color_tool->sample_point)
    gimp_display_shell_draw_sample_point (shell, color_tool->sample_point,
                                          TRUE);
}

static void
gimp_color_tool_cursor_update (GimpTool        *tool,
                               GimpCoords      *coords,
                               GdkModifierType  state,
                               GimpDisplay     *display)
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

          if (gimp_image_coords_in_active_pickable (display->image, coords,
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
      if (color_tool->moving_sample_point)
        {
          if (color_tool->sample_point_x != -1 &&
              color_tool->sample_point_y != -1)
            {
              gimp_draw_tool_draw_line (draw_tool,
                                        0, color_tool->sample_point_y + 0.5,
                                        draw_tool->display->image->width,
                                        color_tool->sample_point_y + 0.5,
                                        FALSE);
              gimp_draw_tool_draw_line (draw_tool,
                                        color_tool->sample_point_x + 0.5, 0,
                                        color_tool->sample_point_x + 0.5,
                                        draw_tool->display->image->height,
                                        FALSE);
            }
        }
      else
        {
          if (color_tool->options->sample_average)
            {
              gdouble radius = color_tool->options->average_radius;

              gimp_draw_tool_draw_rectangle (draw_tool,
                                             FALSE,
                                             color_tool->center_x - radius,
                                             color_tool->center_y - radius,
                                             2 * radius + 1,
                                             2 * radius + 1,
                                             TRUE);
            }
        }
    }

  GIMP_DRAW_TOOL_CLASS (parent_class)->draw (draw_tool);
}

static gboolean
gimp_color_tool_real_pick (GimpColorTool *color_tool,
                           gint           x,
                           gint           y,
                           GimpImageType *sample_type,
                           GimpRGB       *color,
                           gint          *color_index)
{
  GimpTool *tool = GIMP_TOOL (color_tool);

  g_return_val_if_fail (tool->display != NULL, FALSE);
  g_return_val_if_fail (tool->drawable != NULL, FALSE);

  return gimp_image_pick_color (tool->display->image, tool->drawable, x, y,
                                color_tool->options->sample_merged,
                                color_tool->options->sample_average,
                                color_tool->options->average_radius,
                                sample_type,
                                color,
                                color_index);
}

static void
gimp_color_tool_real_picked (GimpColorTool      *color_tool,
                             GimpColorPickState  pick_state,
                             GimpImageType       sample_type,
                             GimpRGB            *color,
                             gint                color_index)
{
  GimpTool          *tool = GIMP_TOOL (color_tool);
  GimpContext       *context;
  GimpDialogFactory *dialog_factory;

  /*  use this tool's own options here (NOT color_tool->options)  */
  context = GIMP_CONTEXT (gimp_tool_get_options (tool));

  dialog_factory = gimp_dialog_factory_from_name ("dock");

  if (color_tool->pick_mode == GIMP_COLOR_PICK_MODE_FOREGROUND ||
      color_tool->pick_mode == GIMP_COLOR_PICK_MODE_BACKGROUND)
    {
      GimpSessionInfo *info;

      if (GIMP_IMAGE_TYPE_IS_INDEXED (sample_type))
        {
          info = gimp_dialog_factory_find_session_info (dialog_factory,
                                                        "gimp-indexed-palette");
          if (info && info->widget)
            {
              GimpColormapEditor *editor;

              editor = GIMP_COLORMAP_EDITOR (gtk_bin_get_child (GTK_BIN (info->widget)));

              gimp_colormap_editor_set_index (editor, color_index, NULL);
            }
        }

      if (TRUE)
        {
          info = gimp_dialog_factory_find_session_info (dialog_factory,
                                                        "gimp-palette-editor");
          if (info && info->widget)
            {
              GimpPaletteEditor *editor;
              gint               index;

              editor = GIMP_PALETTE_EDITOR (gtk_bin_get_child (GTK_BIN (info->widget)));

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
        GdkScreen *screen;
        GtkWidget *dockable;

        screen = gtk_widget_get_screen (tool->display->shell);
        dockable = gimp_dialog_factory_dialog_raise (dialog_factory, screen,
                                                     "gimp-palette-editor",
                                                     -1);
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
  GimpImageType       sample_type;
  GimpRGB             color;
  gint                color_index;

  klass = GIMP_COLOR_TOOL_GET_CLASS (tool);

  if (klass->pick &&
      klass->pick (tool, x, y, &sample_type, &color, &color_index))
    {
      g_signal_emit (tool, gimp_color_tool_signals[PICKED], 0,
                     pick_state, sample_type, &color, color_index);
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

  gimp_display_shell_selection_control (GIMP_DISPLAY_SHELL (display->shell),
                                        GIMP_SELECTION_PAUSE);

  tool->display = display;
  gimp_tool_control_activate (tool->control);

  if (color_tool->sample_point)
    gimp_display_shell_draw_sample_point (GIMP_DISPLAY_SHELL (display->shell),
                                          color_tool->sample_point, FALSE);

  color_tool->sample_point        = NULL;
  color_tool->moving_sample_point = TRUE;
  color_tool->sample_point_x      = -1;
  color_tool->sample_point_y      = -1;

  gimp_tool_set_cursor (tool, display,
                        GIMP_CURSOR_MOUSE,
                        GIMP_TOOL_CURSOR_COLOR_PICKER,
                        GIMP_CURSOR_MODIFIER_MOVE);

  gimp_draw_tool_start (GIMP_DRAW_TOOL (tool), display);
}
