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

#include "libligmamath/ligmamath.h"
#include "libligmacolor/ligmacolor.h"
#include "libligmawidgets/ligmawidgets.h"

#include "tools-types.h"

#include "config/ligmacoreconfig.h"

#include "core/ligma.h"
#include "core/ligmadrawable.h"
#include "core/ligmaimage.h"
#include "core/ligmatoolinfo.h"

#include "widgets/ligmacolorframe.h"
#include "widgets/ligmahelp-ids.h"
#include "widgets/ligmawidgets-utils.h"

#include "display/ligmadisplay.h"
#include "display/ligmatoolgui.h"

#include "ligmacolorpickeroptions.h"
#include "ligmacolorpickertool.h"
#include "ligmatoolcontrol.h"

#include "ligma-intl.h"


/*  local function prototypes  */

static void   ligma_color_picker_tool_constructed   (GObject             *object);
static void   ligma_color_picker_tool_dispose       (GObject             *object);

static void   ligma_color_picker_tool_control       (LigmaTool            *tool,
                                                    LigmaToolAction       action,
                                                    LigmaDisplay         *display);
static void   ligma_color_picker_tool_modifier_key  (LigmaTool            *tool,
                                                    GdkModifierType      key,
                                                    gboolean             press,
                                                    GdkModifierType      state,
                                                    LigmaDisplay         *display);
static void   ligma_color_picker_tool_oper_update   (LigmaTool            *tool,
                                                    const LigmaCoords    *coords,
                                                    GdkModifierType      state,
                                                    gboolean             proximity,
                                                    LigmaDisplay         *display);

static void   ligma_color_picker_tool_picked        (LigmaColorTool       *color_tool,
                                                    const LigmaCoords    *coords,
                                                    LigmaDisplay         *display,
                                                    LigmaColorPickState   pick_state,
                                                    const Babl          *sample_format,
                                                    gpointer             pixel,
                                                    const LigmaRGB       *color);

static void   ligma_color_picker_tool_info_create   (LigmaColorPickerTool *picker_tool,
                                                    LigmaDisplay         *display);
static void   ligma_color_picker_tool_info_response (LigmaToolGui         *gui,
                                                    gint                 response_id,
                                                    LigmaColorPickerTool *picker_tool);
static void   ligma_color_picker_tool_info_update   (LigmaColorPickerTool *picker_tool,
                                                    LigmaDisplay         *display,
                                                    gboolean             sample_average,
                                                    const Babl          *sample_format,
                                                    gpointer             pixel,
                                                    const LigmaRGB       *color,
                                                    gint                 x,
                                                    gint                 y);


G_DEFINE_TYPE (LigmaColorPickerTool, ligma_color_picker_tool,
               LIGMA_TYPE_COLOR_TOOL)

#define parent_class ligma_color_picker_tool_parent_class


void
ligma_color_picker_tool_register (LigmaToolRegisterCallback  callback,
                                 gpointer                  data)
{
  (* callback) (LIGMA_TYPE_COLOR_PICKER_TOOL,
                LIGMA_TYPE_COLOR_PICKER_OPTIONS,
                ligma_color_picker_options_gui,
                LIGMA_CONTEXT_PROP_MASK_FOREGROUND |
                LIGMA_CONTEXT_PROP_MASK_BACKGROUND,
                "ligma-color-picker-tool",
                _("Color Picker"),
                _("Color Picker Tool: Set colors from image pixels"),
                N_("C_olor Picker"), "O",
                NULL, LIGMA_HELP_TOOL_COLOR_PICKER,
                LIGMA_ICON_TOOL_COLOR_PICKER,
                data);
}

static void
ligma_color_picker_tool_class_init (LigmaColorPickerToolClass *klass)
{
  GObjectClass       *object_class     = G_OBJECT_CLASS (klass);
  LigmaToolClass      *tool_class       = LIGMA_TOOL_CLASS (klass);
  LigmaColorToolClass *color_tool_class = LIGMA_COLOR_TOOL_CLASS (klass);

  object_class->constructed = ligma_color_picker_tool_constructed;
  object_class->dispose     = ligma_color_picker_tool_dispose;

  tool_class->control       = ligma_color_picker_tool_control;
  tool_class->modifier_key  = ligma_color_picker_tool_modifier_key;
  tool_class->oper_update   = ligma_color_picker_tool_oper_update;

  color_tool_class->picked  = ligma_color_picker_tool_picked;
}

static void
ligma_color_picker_tool_init (LigmaColorPickerTool *picker_tool)
{
  LigmaColorTool *color_tool = LIGMA_COLOR_TOOL (picker_tool);

  color_tool->pick_target = LIGMA_COLOR_PICK_TARGET_FOREGROUND;
}

static void
ligma_color_picker_tool_constructed (GObject *object)
{
  LigmaTool *tool = LIGMA_TOOL (object);

  G_OBJECT_CLASS (parent_class)->constructed (object);

  ligma_color_tool_enable (LIGMA_COLOR_TOOL (object),
                          LIGMA_COLOR_TOOL_GET_OPTIONS (tool));
}

static void
ligma_color_picker_tool_dispose (GObject *object)
{
  LigmaColorPickerTool *picker_tool = LIGMA_COLOR_PICKER_TOOL (object);

  g_clear_object (&picker_tool->gui);
  picker_tool->color_area   = NULL;
  picker_tool->color_frame1 = NULL;
  picker_tool->color_frame2 = NULL;

  G_OBJECT_CLASS (parent_class)->dispose (object);
}

static void
ligma_color_picker_tool_control (LigmaTool       *tool,
                                LigmaToolAction  action,
                                LigmaDisplay    *display)
{
  LigmaColorPickerTool *picker_tool = LIGMA_COLOR_PICKER_TOOL (tool);

  switch (action)
    {
    case LIGMA_TOOL_ACTION_PAUSE:
    case LIGMA_TOOL_ACTION_RESUME:
      break;

    case LIGMA_TOOL_ACTION_HALT:
      if (picker_tool->gui)
        ligma_tool_gui_hide (picker_tool->gui);
      break;

    case LIGMA_TOOL_ACTION_COMMIT:
      break;
    }

  LIGMA_TOOL_CLASS (parent_class)->control (tool, action, display);
}

static void
ligma_color_picker_tool_modifier_key (LigmaTool        *tool,
                                     GdkModifierType  key,
                                     gboolean         press,
                                     GdkModifierType  state,
                                     LigmaDisplay     *display)
{
  LigmaColorPickerOptions *options = LIGMA_COLOR_PICKER_TOOL_GET_OPTIONS (tool);

  if (key == ligma_get_extend_selection_mask ())
    {
      g_object_set (options, "use-info-window", ! options->use_info_window,
                    NULL);
    }
  else if (key == ligma_get_toggle_behavior_mask ())
    {
      switch (options->pick_target)
        {
        case LIGMA_COLOR_PICK_TARGET_FOREGROUND:
          g_object_set (options,
                        "pick-target", LIGMA_COLOR_PICK_TARGET_BACKGROUND,
                        NULL);
          break;

        case LIGMA_COLOR_PICK_TARGET_BACKGROUND:
          g_object_set (options,
                        "pick-target", LIGMA_COLOR_PICK_TARGET_FOREGROUND,
                        NULL);
          break;

        default:
          break;
        }

    }
}

static void
ligma_color_picker_tool_oper_update (LigmaTool         *tool,
                                    const LigmaCoords *coords,
                                    GdkModifierType   state,
                                    gboolean          proximity,
                                    LigmaDisplay      *display)
{
  LigmaColorPickerTool    *picker_tool = LIGMA_COLOR_PICKER_TOOL (tool);
  LigmaColorPickerOptions *options = LIGMA_COLOR_PICKER_TOOL_GET_OPTIONS (tool);

  LIGMA_COLOR_TOOL (tool)->pick_target = options->pick_target;

  ligma_tool_pop_status (tool, display);

  if (proximity)
    {
      gchar           *status_help = NULL;
      GdkModifierType  extend_mask = 0;
      GdkModifierType  toggle_mask;

      if (! picker_tool->gui)
        extend_mask = ligma_get_extend_selection_mask ();

      toggle_mask = ligma_get_toggle_behavior_mask ();

      switch (options->pick_target)
        {
        case LIGMA_COLOR_PICK_TARGET_NONE:
          status_help = ligma_suggest_modifiers (_("Click in any image to view"
                                                  " its color"),
                                                extend_mask & ~state,
                                                NULL, NULL, NULL);
          break;

        case LIGMA_COLOR_PICK_TARGET_FOREGROUND:
          status_help = ligma_suggest_modifiers (_("Click in any image to pick"
                                                  " the foreground color"),
                                                (extend_mask | toggle_mask) &
                                                ~state,
                                                NULL, NULL, NULL);
          break;

        case LIGMA_COLOR_PICK_TARGET_BACKGROUND:
          status_help = ligma_suggest_modifiers (_("Click in any image to pick"
                                                  " the background color"),
                                                (extend_mask | toggle_mask) &
                                                ~state,
                                                NULL, NULL, NULL);
          break;

        case LIGMA_COLOR_PICK_TARGET_PALETTE:
          status_help = ligma_suggest_modifiers (_("Click in any image to add"
                                                  " the color to the palette"),
                                                extend_mask & ~state,
                                                NULL, NULL, NULL);
          break;
        }

      if (status_help != NULL)
        {
          ligma_tool_push_status (tool, display, "%s", status_help);
          g_free (status_help);
        }
    }

  LIGMA_TOOL_CLASS (parent_class)->oper_update (tool, coords, state, proximity,
                                               display);
}

static void
ligma_color_picker_tool_picked (LigmaColorTool      *color_tool,
                               const LigmaCoords   *coords,
                               LigmaDisplay        *display,
                               LigmaColorPickState  pick_state,
                               const Babl         *sample_format,
                               gpointer            pixel,
                               const LigmaRGB      *color)
{
  LigmaColorPickerTool    *picker_tool = LIGMA_COLOR_PICKER_TOOL (color_tool);
  LigmaColorPickerOptions *options;

  options = LIGMA_COLOR_PICKER_TOOL_GET_OPTIONS (color_tool);

  if (options->use_info_window && ! picker_tool->gui)
    ligma_color_picker_tool_info_create (picker_tool, display);

  if (picker_tool->gui &&
      (options->use_info_window ||
       ligma_tool_gui_get_visible (picker_tool->gui)))
    {
      ligma_color_picker_tool_info_update (picker_tool, display,
                                          LIGMA_COLOR_OPTIONS (options)->sample_average,
                                          sample_format, pixel, color,
                                          (gint) floor (coords->x),
                                          (gint) floor (coords->y));
    }

  LIGMA_COLOR_TOOL_CLASS (parent_class)->picked (color_tool,
                                                coords, display, pick_state,
                                                sample_format, pixel, color);
}

static void
ligma_color_picker_tool_info_create (LigmaColorPickerTool *picker_tool,
                                    LigmaDisplay         *display)
{
  Ligma             *ligma      = ligma_display_get_ligma (display);
  LigmaTool         *tool      = LIGMA_TOOL (picker_tool);
  LigmaToolOptions  *options   = LIGMA_TOOL_GET_OPTIONS (tool);
  LigmaContext      *context   = LIGMA_CONTEXT (tool->tool_info->tool_options);
  LigmaDisplayShell *shell     = ligma_display_get_shell (display);
  LigmaImage        *image     = ligma_display_get_image (display);
  GList            *drawables = ligma_image_get_selected_drawables (image);
  GtkWidget        *hbox;
  GtkWidget        *frame;
  LigmaRGB           color;

  picker_tool->gui = ligma_tool_gui_new (tool->tool_info,
                                        NULL,
                                        _("Color Picker Information"),
                                        NULL, NULL,
                                        ligma_widget_get_monitor (GTK_WIDGET (shell)),
                                        TRUE,

                                        _("_Close"), GTK_RESPONSE_CLOSE,

                                        NULL);

  ligma_tool_gui_set_auto_overlay (picker_tool->gui, TRUE);
  ligma_tool_gui_set_focus_on_map (picker_tool->gui, FALSE);
  ligma_tool_gui_set_viewables (picker_tool->gui, drawables);

  g_signal_connect (picker_tool->gui, "response",
                    G_CALLBACK (ligma_color_picker_tool_info_response),
                    picker_tool);

  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 6);
  gtk_box_pack_start (GTK_BOX (ligma_tool_gui_get_vbox (picker_tool->gui)),
                      hbox, FALSE, FALSE, 0);
  gtk_widget_show (hbox);

  picker_tool->color_frame1 = ligma_color_frame_new (ligma);
  ligma_color_frame_set_color_config (LIGMA_COLOR_FRAME (picker_tool->color_frame1),
                                     context->ligma->config->color_management);
  ligma_color_frame_set_has_coords (LIGMA_COLOR_FRAME (picker_tool->color_frame1),
                                   TRUE);
  g_object_bind_property (options,                   "frame1-mode",
                          picker_tool->color_frame1, "mode",
                          G_BINDING_BIDIRECTIONAL |
                          G_BINDING_SYNC_CREATE);
  gtk_box_pack_start (GTK_BOX (hbox), picker_tool->color_frame1,
                      FALSE, FALSE, 0);
  gtk_widget_show (picker_tool->color_frame1);

  picker_tool->color_frame2 = ligma_color_frame_new (ligma);
  ligma_color_frame_set_color_config (LIGMA_COLOR_FRAME (picker_tool->color_frame2),
                                     context->ligma->config->color_management);
  g_object_bind_property (options,                   "frame2-mode",
                          picker_tool->color_frame2, "mode",
                          G_BINDING_BIDIRECTIONAL |
                          G_BINDING_SYNC_CREATE);
  gtk_box_pack_start (GTK_BOX (hbox), picker_tool->color_frame2,
                      FALSE, FALSE, 0);
  gtk_widget_show (picker_tool->color_frame2);

  frame = gtk_frame_new (NULL);
  ligma_widget_set_fully_opaque (frame, TRUE);
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_IN);
  gtk_box_pack_start (GTK_BOX (hbox), frame, TRUE, TRUE, 0);
  gtk_widget_show (frame);

  ligma_rgba_set (&color, 0.0, 0.0, 0.0, 0.0);
  picker_tool->color_area =
    ligma_color_area_new (&color,
                         drawables && ligma_drawable_has_alpha (drawables->data) ?
                         LIGMA_COLOR_AREA_LARGE_CHECKS :
                         LIGMA_COLOR_AREA_FLAT,
                         GDK_BUTTON1_MASK | GDK_BUTTON2_MASK);
  ligma_color_area_set_color_config (LIGMA_COLOR_AREA (picker_tool->color_area),
                                    context->ligma->config->color_management);
  gtk_widget_set_size_request (picker_tool->color_area, 48, -1);
  gtk_drag_dest_unset (picker_tool->color_area);
  gtk_container_add (GTK_CONTAINER (frame), picker_tool->color_area);
  gtk_widget_show (picker_tool->color_area);

  g_list_free (drawables);
}

static void
ligma_color_picker_tool_info_response (LigmaToolGui         *gui,
                                      gint                 response_id,
                                      LigmaColorPickerTool *picker_tool)
{
  LigmaTool *tool = LIGMA_TOOL (picker_tool);

  ligma_tool_control (tool, LIGMA_TOOL_ACTION_HALT, NULL);
}

static void
ligma_color_picker_tool_info_update (LigmaColorPickerTool *picker_tool,
                                    LigmaDisplay         *display,
                                    gboolean             sample_average,
                                    const Babl          *sample_format,
                                    gpointer             pixel,
                                    const LigmaRGB       *color,
                                    gint                 x,
                                    gint                 y)
{
  LigmaTool  *tool      = LIGMA_TOOL (picker_tool);
  LigmaImage *image     = ligma_display_get_image (display);
  GList     *drawables = ligma_image_get_selected_drawables (image);

  tool->display = display;

  ligma_tool_gui_set_shell (picker_tool->gui,
                           ligma_display_get_shell (display));
  ligma_tool_gui_set_viewables (picker_tool->gui, drawables);
  g_list_free (drawables);

  ligma_color_area_set_color (LIGMA_COLOR_AREA (picker_tool->color_area),
                             color);

  ligma_color_frame_set_color (LIGMA_COLOR_FRAME (picker_tool->color_frame1),
                              sample_average, sample_format, pixel, color,
                              x, y);
  ligma_color_frame_set_color (LIGMA_COLOR_FRAME (picker_tool->color_frame2),
                              sample_average, sample_format, pixel, color,
                              x, y);

  ligma_tool_gui_show (picker_tool->gui);
}
