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

#include "libgimpmath/gimpmath.h"
#include "libgimpcolor/gimpcolor.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "tools-types.h"

#include "config/gimpcoreconfig.h"

#include "core/gimp.h"
#include "core/gimpdrawable.h"
#include "core/gimpimage.h"
#include "core/gimptoolinfo.h"

#include "widgets/gimpcolorframe.h"
#include "widgets/gimphelp-ids.h"
#include "widgets/gimpwidgets-utils.h"

#include "display/gimpdisplay.h"
#include "display/gimptoolgui.h"

#include "gimpcolorpickeroptions.h"
#include "gimpcolorpickertool.h"
#include "gimptoolcontrol.h"

#include "gimp-intl.h"


/*  local function prototypes  */

static void   gimp_color_picker_tool_constructed   (GObject             *object);
static void   gimp_color_picker_tool_dispose       (GObject             *object);

static void   gimp_color_picker_tool_control       (GimpTool            *tool,
                                                    GimpToolAction       action,
                                                    GimpDisplay         *display);
static void   gimp_color_picker_tool_modifier_key  (GimpTool            *tool,
                                                    GdkModifierType      key,
                                                    gboolean             press,
                                                    GdkModifierType      state,
                                                    GimpDisplay         *display);
static void   gimp_color_picker_tool_oper_update   (GimpTool            *tool,
                                                    const GimpCoords    *coords,
                                                    GdkModifierType      state,
                                                    gboolean             proximity,
                                                    GimpDisplay         *display);

static void   gimp_color_picker_tool_picked        (GimpColorTool       *color_tool,
                                                    const GimpCoords    *coords,
                                                    GimpDisplay         *display,
                                                    GimpColorPickState   pick_state,
                                                    const Babl          *sample_format,
                                                    gpointer             pixel,
                                                    GeglColor           *color);

static void   gimp_color_picker_tool_info_create   (GimpColorPickerTool *picker_tool,
                                                    GimpDisplay         *display);
static void   gimp_color_picker_tool_info_response (GimpToolGui         *gui,
                                                    gint                 response_id,
                                                    GimpColorPickerTool *picker_tool);
static void   gimp_color_picker_tool_info_update   (GimpColorPickerTool *picker_tool,
                                                    GimpDisplay         *display,
                                                    gboolean             sample_average,
                                                    const Babl          *sample_format,
                                                    gpointer             pixel,
                                                    GeglColor           *color,
                                                    gint                 x,
                                                    gint                 y);


G_DEFINE_TYPE (GimpColorPickerTool, gimp_color_picker_tool,
               GIMP_TYPE_COLOR_TOOL)

#define parent_class gimp_color_picker_tool_parent_class


void
gimp_color_picker_tool_register (GimpToolRegisterCallback  callback,
                                 gpointer                  data)
{
  (* callback) (GIMP_TYPE_COLOR_PICKER_TOOL,
                GIMP_TYPE_COLOR_PICKER_OPTIONS,
                gimp_color_picker_options_gui,
                GIMP_CONTEXT_PROP_MASK_FOREGROUND |
                GIMP_CONTEXT_PROP_MASK_BACKGROUND,
                "gimp-color-picker-tool",
                _("Color Picker"),
                _("Color Picker Tool: Set colors from image pixels"),
                N_("C_olor Picker"), "O",
                NULL, GIMP_HELP_TOOL_COLOR_PICKER,
                GIMP_ICON_TOOL_COLOR_PICKER,
                data);
}

static void
gimp_color_picker_tool_class_init (GimpColorPickerToolClass *klass)
{
  GObjectClass       *object_class     = G_OBJECT_CLASS (klass);
  GimpToolClass      *tool_class       = GIMP_TOOL_CLASS (klass);
  GimpColorToolClass *color_tool_class = GIMP_COLOR_TOOL_CLASS (klass);

  object_class->constructed  = gimp_color_picker_tool_constructed;
  object_class->dispose      = gimp_color_picker_tool_dispose;

  tool_class->control        = gimp_color_picker_tool_control;
  tool_class->modifier_key   = gimp_color_picker_tool_modifier_key;
  tool_class->oper_update    = gimp_color_picker_tool_oper_update;
  tool_class->is_destructive = FALSE;

  color_tool_class->picked   = gimp_color_picker_tool_picked;
}

static void
gimp_color_picker_tool_init (GimpColorPickerTool *picker_tool)
{
  GimpColorTool *color_tool = GIMP_COLOR_TOOL (picker_tool);

  color_tool->pick_target = GIMP_COLOR_PICK_TARGET_FOREGROUND;
}

static void
gimp_color_picker_tool_constructed (GObject *object)
{
  GimpTool *tool = GIMP_TOOL (object);

  G_OBJECT_CLASS (parent_class)->constructed (object);

  gimp_color_tool_enable (GIMP_COLOR_TOOL (object),
                          GIMP_COLOR_TOOL_GET_OPTIONS (tool));
}

static void
gimp_color_picker_tool_dispose (GObject *object)
{
  GimpColorPickerTool *picker_tool = GIMP_COLOR_PICKER_TOOL (object);

  g_clear_object (&picker_tool->gui);
  picker_tool->color_area   = NULL;
  picker_tool->color_frame1 = NULL;
  picker_tool->color_frame2 = NULL;

  G_OBJECT_CLASS (parent_class)->dispose (object);
}

static void
gimp_color_picker_tool_control (GimpTool       *tool,
                                GimpToolAction  action,
                                GimpDisplay    *display)
{
  GimpColorPickerTool *picker_tool = GIMP_COLOR_PICKER_TOOL (tool);

  switch (action)
    {
    case GIMP_TOOL_ACTION_PAUSE:
    case GIMP_TOOL_ACTION_RESUME:
      break;

    case GIMP_TOOL_ACTION_HALT:
      if (picker_tool->gui)
        gimp_tool_gui_hide (picker_tool->gui);
      break;

    case GIMP_TOOL_ACTION_COMMIT:
      break;
    }

  GIMP_TOOL_CLASS (parent_class)->control (tool, action, display);
}

static void
gimp_color_picker_tool_modifier_key (GimpTool        *tool,
                                     GdkModifierType  key,
                                     gboolean         press,
                                     GdkModifierType  state,
                                     GimpDisplay     *display)
{
  GimpColorPickerOptions *options = GIMP_COLOR_PICKER_TOOL_GET_OPTIONS (tool);

  if (key == gimp_get_extend_selection_mask ())
    {
      g_object_set (options, "use-info-window", ! options->use_info_window,
                    NULL);
    }
  else if (key == gimp_get_toggle_behavior_mask ())
    {
      switch (options->pick_target)
        {
        case GIMP_COLOR_PICK_TARGET_FOREGROUND:
          g_object_set (options,
                        "pick-target", GIMP_COLOR_PICK_TARGET_BACKGROUND,
                        NULL);
          break;

        case GIMP_COLOR_PICK_TARGET_BACKGROUND:
          g_object_set (options,
                        "pick-target", GIMP_COLOR_PICK_TARGET_FOREGROUND,
                        NULL);
          break;

        default:
          break;
        }

    }
}

static void
gimp_color_picker_tool_oper_update (GimpTool         *tool,
                                    const GimpCoords *coords,
                                    GdkModifierType   state,
                                    gboolean          proximity,
                                    GimpDisplay      *display)
{
  GimpColorPickerTool    *picker_tool = GIMP_COLOR_PICKER_TOOL (tool);
  GimpColorPickerOptions *options = GIMP_COLOR_PICKER_TOOL_GET_OPTIONS (tool);

  GIMP_COLOR_TOOL (tool)->pick_target = options->pick_target;

  gimp_tool_pop_status (tool, display);

  if (proximity)
    {
      gchar           *status_help = NULL;
      GdkModifierType  extend_mask = 0;
      GdkModifierType  toggle_mask;

      if (! picker_tool->gui)
        extend_mask = gimp_get_extend_selection_mask ();

      toggle_mask = gimp_get_toggle_behavior_mask ();

      switch (options->pick_target)
        {
        case GIMP_COLOR_PICK_TARGET_NONE:
          status_help = gimp_suggest_modifiers (_("Click in any image to view"
                                                  " its color"),
                                                extend_mask & ~state,
                                                NULL, NULL, NULL);
          break;

        case GIMP_COLOR_PICK_TARGET_FOREGROUND:
          status_help = gimp_suggest_modifiers (_("Click in any image to pick"
                                                  " the foreground color"),
                                                (extend_mask | toggle_mask) &
                                                ~state,
                                                NULL, NULL, NULL);
          break;

        case GIMP_COLOR_PICK_TARGET_BACKGROUND:
          status_help = gimp_suggest_modifiers (_("Click in any image to pick"
                                                  " the background color"),
                                                (extend_mask | toggle_mask) &
                                                ~state,
                                                NULL, NULL, NULL);
          break;

        case GIMP_COLOR_PICK_TARGET_PALETTE:
          status_help = gimp_suggest_modifiers (_("Click in any image to add"
                                                  " the color to the palette"),
                                                extend_mask & ~state,
                                                NULL, NULL, NULL);
          break;
        }

      if (status_help != NULL)
        {
          gimp_tool_push_status (tool, display, "%s", status_help);
          g_free (status_help);
        }
    }

  GIMP_TOOL_CLASS (parent_class)->oper_update (tool, coords, state, proximity,
                                               display);
}

static void
gimp_color_picker_tool_picked (GimpColorTool      *color_tool,
                               const GimpCoords   *coords,
                               GimpDisplay        *display,
                               GimpColorPickState  pick_state,
                               const Babl         *sample_format,
                               gpointer            pixel,
                               GeglColor          *color)
{
  GimpColorPickerTool    *picker_tool = GIMP_COLOR_PICKER_TOOL (color_tool);
  GimpColorPickerOptions *options;

  options = GIMP_COLOR_PICKER_TOOL_GET_OPTIONS (color_tool);

  if (options->use_info_window && ! picker_tool->gui)
    gimp_color_picker_tool_info_create (picker_tool, display);

  if (picker_tool->gui &&
      (options->use_info_window ||
       gimp_tool_gui_get_visible (picker_tool->gui)))
    {
      gimp_color_picker_tool_info_update (picker_tool, display,
                                          GIMP_COLOR_OPTIONS (options)->sample_average,
                                          sample_format, pixel, color,
                                          (gint) floor (coords->x),
                                          (gint) floor (coords->y));
    }

  GIMP_COLOR_TOOL_CLASS (parent_class)->picked (color_tool,
                                                coords, display, pick_state,
                                                sample_format, pixel, color);
}

static void
gimp_color_picker_tool_info_create (GimpColorPickerTool *picker_tool,
                                    GimpDisplay         *display)
{
  Gimp             *gimp      = gimp_display_get_gimp (display);
  GimpTool         *tool      = GIMP_TOOL (picker_tool);
  GimpToolOptions  *options   = GIMP_TOOL_GET_OPTIONS (tool);
  GimpContext      *context   = GIMP_CONTEXT (tool->tool_info->tool_options);
  GimpDisplayShell *shell     = gimp_display_get_shell (display);
  GimpImage        *image     = gimp_display_get_image (display);
  GList            *drawables = gimp_image_get_selected_drawables (image);
  GtkWidget        *hbox;
  GtkWidget        *frame;
  GeglColor        *color;

  picker_tool->gui = gimp_tool_gui_new (tool->tool_info,
                                        NULL,
                                        _("Color Picker Information"),
                                        NULL, NULL,
                                        gimp_widget_get_monitor (GTK_WIDGET (shell)),
                                        TRUE,

                                        _("_Close"), GTK_RESPONSE_CLOSE,

                                        NULL);

  gimp_tool_gui_set_auto_overlay (picker_tool->gui, TRUE);
  gimp_tool_gui_set_focus_on_map (picker_tool->gui, FALSE);
  gimp_tool_gui_set_viewables (picker_tool->gui, drawables);

  g_signal_connect (picker_tool->gui, "response",
                    G_CALLBACK (gimp_color_picker_tool_info_response),
                    picker_tool);

  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 6);
  gtk_box_pack_start (GTK_BOX (gimp_tool_gui_get_vbox (picker_tool->gui)),
                      hbox, FALSE, FALSE, 0);
  gtk_widget_show (hbox);

  picker_tool->color_frame1 = gimp_color_frame_new (gimp);
  gimp_color_frame_set_color_config (GIMP_COLOR_FRAME (picker_tool->color_frame1),
                                     context->gimp->config->color_management);
  gimp_color_frame_set_has_coords (GIMP_COLOR_FRAME (picker_tool->color_frame1),
                                   TRUE);
  g_object_bind_property (options,                   "frame1-mode",
                          picker_tool->color_frame1, "mode",
                          G_BINDING_BIDIRECTIONAL |
                          G_BINDING_SYNC_CREATE);
  gtk_box_pack_start (GTK_BOX (hbox), picker_tool->color_frame1,
                      FALSE, FALSE, 0);
  gtk_widget_show (picker_tool->color_frame1);

  picker_tool->color_frame2 = gimp_color_frame_new (gimp);
  gimp_color_frame_set_color_config (GIMP_COLOR_FRAME (picker_tool->color_frame2),
                                     context->gimp->config->color_management);
  g_object_bind_property (options,                   "frame2-mode",
                          picker_tool->color_frame2, "mode",
                          G_BINDING_BIDIRECTIONAL |
                          G_BINDING_SYNC_CREATE);
  gtk_box_pack_start (GTK_BOX (hbox), picker_tool->color_frame2,
                      FALSE, FALSE, 0);
  gtk_widget_show (picker_tool->color_frame2);

  frame = gtk_frame_new (NULL);
  gimp_widget_set_fully_opaque (frame, TRUE);
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_IN);
  gtk_box_pack_start (GTK_BOX (hbox), frame, TRUE, TRUE, 0);
  gtk_widget_show (frame);

  color = gegl_color_new ("transparent");
  picker_tool->color_area =
    gimp_color_area_new (color,
                         drawables && gimp_drawable_has_alpha (drawables->data) ?
                         GIMP_COLOR_AREA_LARGE_CHECKS :
                         GIMP_COLOR_AREA_FLAT,
                         GDK_BUTTON1_MASK | GDK_BUTTON2_MASK);
  gimp_color_area_set_color_config (GIMP_COLOR_AREA (picker_tool->color_area),
                                    context->gimp->config->color_management);
  gtk_widget_set_size_request (picker_tool->color_area, 48, -1);
  gtk_drag_dest_unset (picker_tool->color_area);
  gtk_container_add (GTK_CONTAINER (frame), picker_tool->color_area);
  gtk_widget_show (picker_tool->color_area);

  g_list_free (drawables);
  g_object_unref (color);
}

static void
gimp_color_picker_tool_info_response (GimpToolGui         *gui,
                                      gint                 response_id,
                                      GimpColorPickerTool *picker_tool)
{
  GimpTool *tool = GIMP_TOOL (picker_tool);

  gimp_tool_control (tool, GIMP_TOOL_ACTION_HALT, NULL);
}

static void
gimp_color_picker_tool_info_update (GimpColorPickerTool *picker_tool,
                                    GimpDisplay         *display,
                                    gboolean             sample_average,
                                    const Babl          *sample_format,
                                    gpointer             pixel,
                                    GeglColor           *color,
                                    gint                 x,
                                    gint                 y)
{
  GimpTool  *tool      = GIMP_TOOL (picker_tool);
  GimpImage *image     = gimp_display_get_image (display);
  GList     *drawables = gimp_image_get_selected_drawables (image);

  tool->display = display;

  gimp_tool_gui_set_shell (picker_tool->gui,
                           gimp_display_get_shell (display));
  gimp_tool_gui_set_viewables (picker_tool->gui, drawables);
  g_list_free (drawables);

  gimp_color_area_set_color (GIMP_COLOR_AREA (picker_tool->color_area), color);
  gimp_color_frame_set_color (GIMP_COLOR_FRAME (picker_tool->color_frame1),
                              sample_average, color, x, y);
  gimp_color_frame_set_color (GIMP_COLOR_FRAME (picker_tool->color_frame2),
                              sample_average, color, x, y);

  gimp_tool_gui_show (picker_tool->gui);
}
