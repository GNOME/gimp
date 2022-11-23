/* LIGMA - The GNU Image Manipulation Program
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

/* This file contains a base class for tools that implement on canvas
 * preview for non destructive editing. The processing of the pixels can
 * be done either by a gegl op or by a C function (apply_func).
 *
 * For the core side of this, please see app/core/ligmadrawablefilter.c.
 */

#include "config.h"

#include <gegl.h>
#include <gegl-plugin.h>
#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>

#include "libligmabase/ligmabase.h"
#include "libligmaconfig/ligmaconfig.h"
#include "libligmamath/ligmamath.h"
#include "libligmawidgets/ligmawidgets.h"

#include "tools-types.h"

#include "config/ligmaguiconfig.h"

#include "operations/ligma-operation-config.h"
#include "operations/ligmaoperationsettings.h"

#include "gegl/ligma-gegl-utils.h"

#include "core/ligma.h"
#include "core/ligmachannel.h"
#include "core/ligmadrawable.h"
#include "core/ligmadrawablefilter.h"
#include "core/ligmaerror.h"
#include "core/ligmaguide.h"
#include "core/ligmaimage.h"
#include "core/ligmaimage-guides.h"
#include "core/ligmaimage-pick-color.h"
#include "core/ligmalayer.h"
#include "core/ligmalist.h"
#include "core/ligmapickable.h"
#include "core/ligmaprogress.h"
#include "core/ligmaprojection.h"
#include "core/ligmasettings.h"
#include "core/ligmatoolinfo.h"

#include "widgets/ligmalayermodebox.h"
#include "widgets/ligmapropwidgets.h"
#include "widgets/ligmasettingsbox.h"
#include "widgets/ligmawidgets-utils.h"

#include "display/ligmadisplay.h"
#include "display/ligmadisplayshell.h"
#include "display/ligmadisplayshell-appearance.h"
#include "display/ligmadisplayshell-transform.h"
#include "display/ligmatoolgui.h"
#include "display/ligmatoolwidget.h"

#include "ligmafilteroptions.h"
#include "ligmafiltertool.h"
#include "ligmafiltertool-settings.h"
#include "ligmafiltertool-widgets.h"
#include "ligmaguidetool.h"
#include "ligmatoolcontrol.h"
#include "ligmatools-utils.h"
#include "tool_manager.h"

#include "ligma-intl.h"


/*  local function prototypes  */

static void      ligma_filter_tool_finalize       (GObject             *object);

static gboolean  ligma_filter_tool_initialize     (LigmaTool            *tool,
                                                  LigmaDisplay         *display,
                                                  GError             **error);
static void      ligma_filter_tool_control        (LigmaTool            *tool,
                                                  LigmaToolAction       action,
                                                  LigmaDisplay         *display);
static void      ligma_filter_tool_button_press   (LigmaTool            *tool,
                                                  const LigmaCoords    *coords,
                                                  guint32              time,
                                                  GdkModifierType      state,
                                                  LigmaButtonPressType  press_type,
                                                  LigmaDisplay         *display);
static void      ligma_filter_tool_button_release (LigmaTool            *tool,
                                                  const LigmaCoords    *coords,
                                                  guint32              time,
                                                  GdkModifierType      state,
                                                  LigmaButtonReleaseType release_type,
                                                  LigmaDisplay         *display);
static void      ligma_filter_tool_motion         (LigmaTool            *tool,
                                                  const LigmaCoords    *coords,
                                                  guint32              time,
                                                  GdkModifierType      state,
                                                  LigmaDisplay         *display);
static gboolean  ligma_filter_tool_key_press      (LigmaTool            *tool,
                                                  GdkEventKey         *kevent,
                                                  LigmaDisplay         *display);
static void      ligma_filter_tool_oper_update    (LigmaTool            *tool,
                                                  const LigmaCoords    *coords,
                                                  GdkModifierType      state,
                                                  gboolean             proximity,
                                                  LigmaDisplay         *display);
static void      ligma_filter_tool_cursor_update  (LigmaTool            *tool,
                                                  const LigmaCoords    *coords,
                                                  GdkModifierType      state,
                                                  LigmaDisplay         *display);
static void      ligma_filter_tool_options_notify (LigmaTool            *tool,
                                                  LigmaToolOptions     *options,
                                                  const GParamSpec    *pspec);

static gboolean  ligma_filter_tool_can_pick_color (LigmaColorTool       *color_tool,
                                                  const LigmaCoords    *coords,
                                                  LigmaDisplay         *display);
static gboolean  ligma_filter_tool_pick_color     (LigmaColorTool       *color_tool,
                                                  const LigmaCoords    *coords,
                                                  LigmaDisplay         *display,
                                                  const Babl         **sample_format,
                                                  gpointer             pixel,
                                                  LigmaRGB             *color);
static void      ligma_filter_tool_color_picked   (LigmaColorTool       *color_tool,
                                                  const LigmaCoords    *coords,
                                                  LigmaDisplay         *display,
                                                  LigmaColorPickState   pick_state,
                                                  const Babl          *sample_format,
                                                  gpointer             pixel,
                                                  const LigmaRGB       *color);

static void      ligma_filter_tool_real_reset     (LigmaFilterTool      *filter_tool);
static void      ligma_filter_tool_real_set_config(LigmaFilterTool      *filter_tool,
                                                  LigmaConfig          *config);
static void      ligma_filter_tool_real_config_notify
                                                 (LigmaFilterTool      *filter_tool,
                                                  LigmaConfig          *config,
                                                  const GParamSpec    *pspec);

static void      ligma_filter_tool_halt           (LigmaFilterTool      *filter_tool);
static void      ligma_filter_tool_commit         (LigmaFilterTool      *filter_tool);

static void      ligma_filter_tool_dialog         (LigmaFilterTool      *filter_tool);
static void      ligma_filter_tool_reset          (LigmaFilterTool      *filter_tool);

static void      ligma_filter_tool_create_filter  (LigmaFilterTool      *filter_tool);

static void      ligma_filter_tool_update_dialog  (LigmaFilterTool      *filter_tool);
static void      ligma_filter_tool_update_dialog_operation_settings
                                                 (LigmaFilterTool      *filter_tool);


static void      ligma_filter_tool_region_changed (LigmaFilterTool      *filter_tool);

static void      ligma_filter_tool_flush          (LigmaDrawableFilter  *filter,
                                                  LigmaFilterTool      *filter_tool);
static void      ligma_filter_tool_config_notify  (GObject             *object,
                                                  const GParamSpec    *pspec,
                                                  LigmaFilterTool      *filter_tool);
static void      ligma_filter_tool_unset_setting  (GObject             *object,
                                                  const GParamSpec    *pspec,
                                                  LigmaFilterTool      *filter_tool);
static void      ligma_filter_tool_lock_position_changed
                                                 (LigmaDrawable        *drawable,
                                                  LigmaFilterTool      *filter_tool);
static void      ligma_filter_tool_mask_changed   (LigmaImage           *image,
                                                  LigmaFilterTool      *filter_tool);

static void      ligma_filter_tool_add_guide      (LigmaFilterTool      *filter_tool);
static void      ligma_filter_tool_remove_guide   (LigmaFilterTool      *filter_tool);
static void      ligma_filter_tool_move_guide     (LigmaFilterTool      *filter_tool);
static void      ligma_filter_tool_guide_removed  (LigmaGuide           *guide,
                                                  LigmaFilterTool      *filter_tool);
static void      ligma_filter_tool_guide_moved    (LigmaGuide           *guide,
                                                  const GParamSpec    *pspec,
                                                  LigmaFilterTool      *filter_tool);

static void      ligma_filter_tool_response       (LigmaToolGui         *gui,
                                                  gint                 response_id,
                                                  LigmaFilterTool      *filter_tool);

static void      ligma_filter_tool_update_filter  (LigmaFilterTool      *filter_tool);

static void    ligma_filter_tool_set_has_settings (LigmaFilterTool      *filter_tool,
                                                  gboolean             has_settings);


G_DEFINE_TYPE (LigmaFilterTool, ligma_filter_tool, LIGMA_TYPE_COLOR_TOOL)

#define parent_class ligma_filter_tool_parent_class


static void
ligma_filter_tool_class_init (LigmaFilterToolClass *klass)
{
  GObjectClass       *object_class     = G_OBJECT_CLASS (klass);
  LigmaToolClass      *tool_class       = LIGMA_TOOL_CLASS (klass);
  LigmaColorToolClass *color_tool_class = LIGMA_COLOR_TOOL_CLASS (klass);

  object_class->finalize     = ligma_filter_tool_finalize;

  tool_class->initialize     = ligma_filter_tool_initialize;
  tool_class->control        = ligma_filter_tool_control;
  tool_class->button_press   = ligma_filter_tool_button_press;
  tool_class->button_release = ligma_filter_tool_button_release;
  tool_class->motion         = ligma_filter_tool_motion;
  tool_class->key_press      = ligma_filter_tool_key_press;
  tool_class->oper_update    = ligma_filter_tool_oper_update;
  tool_class->cursor_update  = ligma_filter_tool_cursor_update;
  tool_class->options_notify = ligma_filter_tool_options_notify;

  color_tool_class->can_pick = ligma_filter_tool_can_pick_color;
  color_tool_class->pick     = ligma_filter_tool_pick_color;
  color_tool_class->picked   = ligma_filter_tool_color_picked;

  klass->get_operation       = NULL;
  klass->dialog              = NULL;
  klass->reset               = ligma_filter_tool_real_reset;
  klass->set_config          = ligma_filter_tool_real_set_config;
  klass->config_notify       = ligma_filter_tool_real_config_notify;
  klass->settings_import     = ligma_filter_tool_real_settings_import;
  klass->settings_export     = ligma_filter_tool_real_settings_export;
}

static void
ligma_filter_tool_init (LigmaFilterTool *filter_tool)
{
  LigmaTool *tool = LIGMA_TOOL (filter_tool);

  ligma_tool_control_set_scroll_lock      (tool->control, TRUE);
  ligma_tool_control_set_preserve         (tool->control, FALSE);
  ligma_tool_control_set_dirty_mask       (tool->control,
                                          LIGMA_DIRTY_IMAGE           |
                                          LIGMA_DIRTY_IMAGE_STRUCTURE |
                                          LIGMA_DIRTY_DRAWABLE        |
                                          LIGMA_DIRTY_ACTIVE_DRAWABLE);
  ligma_tool_control_set_active_modifiers (tool->control,
                                          LIGMA_TOOL_ACTIVE_MODIFIERS_SEPARATE);
}

static void
ligma_filter_tool_finalize (GObject *object)
{
  LigmaFilterTool *filter_tool = LIGMA_FILTER_TOOL (object);

  g_clear_object (&filter_tool->operation);
  g_clear_object (&filter_tool->config);
  g_clear_object (&filter_tool->default_config);
  g_clear_object (&filter_tool->settings);
  g_clear_pointer (&filter_tool->description, g_free);
  g_clear_object (&filter_tool->gui);
  filter_tool->settings_box      = NULL;
  filter_tool->controller_toggle = NULL;
  filter_tool->clip_combo        = NULL;
  filter_tool->region_combo      = NULL;

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

#define RESPONSE_RESET 1

static gboolean
ligma_filter_tool_initialize (LigmaTool     *tool,
                             LigmaDisplay  *display,
                             GError      **error)
{
  LigmaFilterTool   *filter_tool = LIGMA_FILTER_TOOL (tool);
  LigmaToolInfo     *tool_info   = tool->tool_info;
  LigmaGuiConfig    *config      = LIGMA_GUI_CONFIG (display->ligma->config);
  LigmaImage        *image       = ligma_display_get_image (display);
  LigmaDisplayShell *shell       = ligma_display_get_shell (display);
  LigmaItem         *locked_item = NULL;
  GList            *drawables   = ligma_image_get_selected_drawables (image);
  LigmaDrawable     *drawable;

  if (g_list_length (drawables) != 1)
    {
      if (g_list_length (drawables) > 1)
        g_set_error_literal (error, LIGMA_ERROR, LIGMA_FAILED,
                                   _("Cannot modify multiple drawables. Select only one."));
      else
        g_set_error_literal (error, LIGMA_ERROR, LIGMA_FAILED, _("No selected drawables."));

      g_list_free (drawables);
      return FALSE;
    }
  drawable = drawables->data;

  if (ligma_viewable_get_children (LIGMA_VIEWABLE (drawable)))
    {
      g_set_error_literal (error, LIGMA_ERROR, LIGMA_FAILED,
                           _("Cannot modify the pixels of layer groups."));

      g_list_free (drawables);
      return FALSE;
    }

  if (ligma_item_is_content_locked (LIGMA_ITEM (drawable), &locked_item))
    {
      g_set_error_literal (error, LIGMA_ERROR, LIGMA_FAILED,
                           _("A selected item's pixels are locked."));
      if (error)
        ligma_tools_blink_lock_box (display->ligma, locked_item);

      g_list_free (drawables);
      return FALSE;
    }

  if (! ligma_item_is_visible (LIGMA_ITEM (drawable)) &&
      ! config->edit_non_visible)
    {
      g_set_error_literal (error, LIGMA_ERROR, LIGMA_FAILED,
                           _("A selected layer is not visible."));

      g_list_free (drawables);
      return FALSE;
    }

  ligma_filter_tool_get_operation (filter_tool);

  ligma_filter_tool_disable_color_picking (filter_tool);

  tool->display   = display;
  g_list_free (tool->drawables);
  tool->drawables = drawables;

  if (filter_tool->config)
    ligma_config_reset (LIGMA_CONFIG (filter_tool->config));

  if (! filter_tool->gui)
    {
      GtkWidget *vbox;
      GtkWidget *hbox;
      GtkWidget *toggle;

      /*  disabled for at least LIGMA 2.8  */
      filter_tool->overlay = FALSE;

      filter_tool->gui =
        ligma_tool_gui_new (tool_info,
                           ligma_tool_get_label (tool),
                           filter_tool->description,
                           ligma_tool_get_icon_name (tool),
                           ligma_tool_get_help_id (tool),
                           ligma_widget_get_monitor (GTK_WIDGET (shell)),
                           filter_tool->overlay,

                           _("_Reset"),  RESPONSE_RESET,
                           _("_Cancel"), GTK_RESPONSE_CANCEL,
                           _("_OK"),     GTK_RESPONSE_OK,

                           NULL);

      ligma_tool_gui_set_default_response (filter_tool->gui, GTK_RESPONSE_OK);

      ligma_tool_gui_set_alternative_button_order (filter_tool->gui,
                                                  RESPONSE_RESET,
                                                  GTK_RESPONSE_OK,
                                                  GTK_RESPONSE_CANCEL,
                                                  -1);

      vbox = ligma_tool_gui_get_vbox (filter_tool->gui);

      g_signal_connect_object (filter_tool->gui, "response",
                               G_CALLBACK (ligma_filter_tool_response),
                               G_OBJECT (filter_tool), 0);

      if (filter_tool->config)
        {
          filter_tool->settings_box =
            ligma_filter_tool_get_settings_box (filter_tool);
          gtk_box_pack_start (GTK_BOX (vbox), filter_tool->settings_box,
                              FALSE, FALSE, 0);

          if (filter_tool->has_settings)
            gtk_widget_show (filter_tool->settings_box);
        }

      /*  The preview and split view toggles  */
      hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 4);
      gtk_box_pack_end (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
      gtk_widget_show (hbox);

      toggle = ligma_prop_check_button_new (G_OBJECT (tool_info->tool_options),
                                           "preview", NULL);
      gtk_box_pack_start (GTK_BOX (hbox), toggle, TRUE, TRUE, 0);

      toggle = ligma_prop_check_button_new (G_OBJECT (tool_info->tool_options),
                                           "preview-split", NULL);
      gtk_box_pack_start (GTK_BOX (hbox), toggle, FALSE, FALSE, 0);

      g_object_bind_property (G_OBJECT (tool_info->tool_options), "preview",
                              toggle,                             "sensitive",
                              G_BINDING_SYNC_CREATE);

      /*  The show-controller toggle  */
      filter_tool->controller_toggle =
        ligma_prop_check_button_new (G_OBJECT (tool_info->tool_options),
                                    "controller", NULL);
      gtk_box_pack_end (GTK_BOX (vbox), filter_tool->controller_toggle,
                        FALSE, FALSE, 0);
      if (! filter_tool->widget)
        gtk_widget_hide (filter_tool->controller_toggle);

      /*  The operation-settings box  */
      filter_tool->operation_settings_box = gtk_box_new (
        GTK_ORIENTATION_VERTICAL, 2);
      gtk_box_pack_end (GTK_BOX (vbox), filter_tool->operation_settings_box,
                        FALSE, FALSE, 0);
      gtk_widget_show (filter_tool->operation_settings_box);

      ligma_filter_tool_update_dialog_operation_settings (filter_tool);

      /*  Fill in subclass widgets  */
      ligma_filter_tool_dialog (filter_tool);
    }
  else
    {
      ligma_tool_gui_set_title       (filter_tool->gui,
                                     ligma_tool_get_label (tool));
      ligma_tool_gui_set_description (filter_tool->gui, filter_tool->description);
      ligma_tool_gui_set_icon_name   (filter_tool->gui,
                                     ligma_tool_get_icon_name (tool));
      ligma_tool_gui_set_help_id     (filter_tool->gui,
                                     ligma_tool_get_help_id (tool));
    }

  ligma_tool_gui_set_shell (filter_tool->gui, shell);
  ligma_tool_gui_set_viewable (filter_tool->gui, LIGMA_VIEWABLE (drawable));

  ligma_tool_gui_show (filter_tool->gui);

  g_signal_connect_object (drawable, "lock-position-changed",
                           G_CALLBACK (ligma_filter_tool_lock_position_changed),
                           filter_tool, 0);

  g_signal_connect_object (image, "mask-changed",
                           G_CALLBACK (ligma_filter_tool_mask_changed),
                           filter_tool, 0);

  ligma_filter_tool_mask_changed (image, filter_tool);

  ligma_filter_tool_create_filter (filter_tool);

  return TRUE;
}

static void
ligma_filter_tool_control (LigmaTool       *tool,
                          LigmaToolAction  action,
                          LigmaDisplay    *display)
{
  LigmaFilterTool *filter_tool = LIGMA_FILTER_TOOL (tool);

  switch (action)
    {
    case LIGMA_TOOL_ACTION_PAUSE:
    case LIGMA_TOOL_ACTION_RESUME:
      break;

    case LIGMA_TOOL_ACTION_HALT:
      ligma_filter_tool_halt (filter_tool);
      break;

    case LIGMA_TOOL_ACTION_COMMIT:
      ligma_filter_tool_commit (filter_tool);
      break;
    }

  LIGMA_TOOL_CLASS (parent_class)->control (tool, action, display);
}

static void
ligma_filter_tool_button_press (LigmaTool            *tool,
                               const LigmaCoords    *coords,
                               guint32              time,
                               GdkModifierType      state,
                               LigmaButtonPressType  press_type,
                               LigmaDisplay         *display)
{
  LigmaFilterTool *filter_tool = LIGMA_FILTER_TOOL (tool);

  if (ligma_filter_tool_on_guide (filter_tool, coords, display))
    {
      LigmaFilterOptions *options = LIGMA_FILTER_TOOL_GET_OPTIONS (tool);

      if (state & ligma_get_extend_selection_mask ())
        {
          ligma_filter_options_switch_preview_side (options);
        }
      else if (state & ligma_get_toggle_behavior_mask ())
        {
          LigmaItem *item = LIGMA_ITEM (tool->drawables->data);
          gint      pos_x;
          gint      pos_y;

          pos_x = CLAMP (RINT (coords->x) - ligma_item_get_offset_x (item),
                         0, ligma_item_get_width (item));
          pos_y = CLAMP (RINT (coords->y) - ligma_item_get_offset_y (item),
                         0, ligma_item_get_height (item));

          ligma_filter_options_switch_preview_orientation (options,
                                                          pos_x, pos_y);
        }
      else
        {
          ligma_guide_tool_start_edit (tool, display,
                                      filter_tool->preview_guide);
        }
    }
  else if (ligma_color_tool_is_enabled (LIGMA_COLOR_TOOL (tool)))
    {
      LIGMA_TOOL_CLASS (parent_class)->button_press (tool, coords, time, state,
                                                    press_type, display);
    }
  else if (filter_tool->widget)
    {
      if (ligma_tool_widget_button_press (filter_tool->widget, coords, time,
                                         state, press_type))
        {
          filter_tool->grab_widget = filter_tool->widget;

          ligma_tool_control_activate (tool->control);
        }
    }
}

static void
ligma_filter_tool_button_release (LigmaTool              *tool,
                                 const LigmaCoords      *coords,
                                 guint32                time,
                                 GdkModifierType        state,
                                 LigmaButtonReleaseType  release_type,
                                 LigmaDisplay           *display)
{
  LigmaFilterTool *filter_tool = LIGMA_FILTER_TOOL (tool);

  if (filter_tool->grab_widget)
    {
      ligma_tool_control_halt (tool->control);

      ligma_tool_widget_button_release (filter_tool->grab_widget,
                                       coords, time, state, release_type);
      filter_tool->grab_widget = NULL;
    }
  else
    {
      LIGMA_TOOL_CLASS (parent_class)->button_release (tool, coords, time, state,
                                                      release_type, display);
    }
}

static void
ligma_filter_tool_motion (LigmaTool         *tool,
                         const LigmaCoords *coords,
                         guint32           time,
                         GdkModifierType   state,
                         LigmaDisplay      *display)
{
  LigmaFilterTool *filter_tool = LIGMA_FILTER_TOOL (tool);

  if (filter_tool->grab_widget)
    {
      ligma_tool_widget_motion (filter_tool->grab_widget, coords, time, state);
    }
  else
    {
      LIGMA_TOOL_CLASS (parent_class)->motion (tool, coords, time, state,
                                              display);
    }
}

static gboolean
ligma_filter_tool_key_press (LigmaTool    *tool,
                            GdkEventKey *kevent,
                            LigmaDisplay *display)
{
  LigmaFilterTool *filter_tool = LIGMA_FILTER_TOOL (tool);

  if (filter_tool->gui && display == tool->display)
    {
      switch (kevent->keyval)
        {
        case GDK_KEY_Return:
        case GDK_KEY_KP_Enter:
        case GDK_KEY_ISO_Enter:
          ligma_filter_tool_response (filter_tool->gui,
                                     GTK_RESPONSE_OK,
                                     filter_tool);
          return TRUE;

        case GDK_KEY_BackSpace:
          ligma_filter_tool_response (filter_tool->gui,
                                     RESPONSE_RESET,
                                     filter_tool);
          return TRUE;

        case GDK_KEY_Escape:
          ligma_filter_tool_response (filter_tool->gui,
                                     GTK_RESPONSE_CANCEL,
                                     filter_tool);
          return TRUE;
        }
    }

  return LIGMA_TOOL_CLASS (parent_class)->key_press (tool, kevent, display);
}

static void
ligma_filter_tool_oper_update (LigmaTool         *tool,
                              const LigmaCoords *coords,
                              GdkModifierType   state,
                              gboolean          proximity,
                              LigmaDisplay      *display)
{
  LigmaFilterTool *filter_tool = LIGMA_FILTER_TOOL (tool);

  ligma_tool_pop_status (tool, display);

  if (ligma_filter_tool_on_guide (filter_tool, coords, display))
    {
      GdkModifierType  extend_mask = ligma_get_extend_selection_mask ();
      GdkModifierType  toggle_mask = ligma_get_toggle_behavior_mask ();
      gchar           *status      = NULL;

      if (state & extend_mask)
        {
          status = g_strdup (_("Click to switch the original and filtered sides"));
        }
      else if (state & toggle_mask)
        {
          status = g_strdup (_("Click to switch between vertical and horizontal"));
        }
      else
        {
          status = ligma_suggest_modifiers (_("Click to move the split guide"),
                                           (extend_mask | toggle_mask) & ~state,
                                           _("%s: switch original and filtered"),
                                           _("%s: switch horizontal and vertical"),
                                           NULL);
        }

      if (proximity)
        ligma_tool_push_status (tool, display, "%s", status);

      g_free (status);
    }
  else
    {
      LIGMA_TOOL_CLASS (parent_class)->oper_update (tool, coords, state,
                                                   proximity, display);
    }
}

static void
ligma_filter_tool_cursor_update (LigmaTool         *tool,
                                const LigmaCoords *coords,
                                GdkModifierType   state,
                                LigmaDisplay      *display)
{
  LigmaFilterTool *filter_tool = LIGMA_FILTER_TOOL (tool);

  if (ligma_filter_tool_on_guide (filter_tool, coords, display))
    {
      ligma_tool_set_cursor (tool, display,
                            LIGMA_CURSOR_MOUSE,
                            LIGMA_TOOL_CURSOR_HAND,
                            LIGMA_CURSOR_MODIFIER_MOVE);
    }
  else
    {
      LIGMA_TOOL_CLASS (parent_class)->cursor_update (tool, coords, state,
                                                     display);
    }
}

static void
ligma_filter_tool_options_notify (LigmaTool         *tool,
                                 LigmaToolOptions  *options,
                                 const GParamSpec *pspec)
{
  LigmaFilterTool    *filter_tool    = LIGMA_FILTER_TOOL (tool);
  LigmaFilterOptions *filter_options = LIGMA_FILTER_OPTIONS (options);

  if (! strcmp (pspec->name, "preview") &&
      filter_tool->filter)
    {
      ligma_filter_tool_update_filter (filter_tool);

      if (filter_options->preview)
        {
          ligma_drawable_filter_apply (filter_tool->filter, NULL);

          if (filter_options->preview_split)
            ligma_filter_tool_add_guide (filter_tool);
        }
      else
        {
          if (filter_options->preview_split)
            ligma_filter_tool_remove_guide (filter_tool);
        }
    }
  else if (! strcmp (pspec->name, "preview-split") &&
           filter_tool->filter)
    {
      if (filter_options->preview_split)
        {
          LigmaDisplayShell *shell = ligma_display_get_shell (tool->display);
          LigmaItem         *item  = LIGMA_ITEM (tool->drawables->data);
          gint              x, y, width, height;
          gint              position;

          ligma_display_shell_untransform_viewport (shell, TRUE,
                                                   &x, &y, &width, &height);

          if (! ligma_rectangle_intersect (ligma_item_get_offset_x (item),
                                          ligma_item_get_offset_y (item),
                                          ligma_item_get_width  (item),
                                          ligma_item_get_height (item),
                                          x, y, width, height,
                                          &x, &y, &width, &height))
            {
              x      = ligma_item_get_offset_x (item);
              y      = ligma_item_get_offset_y (item);
              width  = ligma_item_get_width    (item);
              height = ligma_item_get_height   (item);
            }

          if (filter_options->preview_split_alignment == LIGMA_ALIGN_LEFT ||
              filter_options->preview_split_alignment == LIGMA_ALIGN_RIGHT)
            {
              position = (x + width  / 2) - ligma_item_get_offset_x (item);
            }
          else
            {
              position = (y + height / 2) - ligma_item_get_offset_y (item);
            }

          g_object_set (
            options,
            "preview-split-position", position,
            NULL);
        }

      ligma_filter_tool_update_filter (filter_tool);

      if (filter_options->preview_split)
        ligma_filter_tool_add_guide (filter_tool);
      else
        ligma_filter_tool_remove_guide (filter_tool);
    }
  else if (! strcmp (pspec->name, "preview-split-alignment") ||
           ! strcmp (pspec->name, "preview-split-position"))
    {
      ligma_filter_tool_update_filter (filter_tool);

      if (filter_options->preview_split)
        ligma_filter_tool_move_guide (filter_tool);
    }
  else if (! strcmp (pspec->name, "controller") &&
           filter_tool->widget)
    {
      ligma_tool_widget_set_visible (filter_tool->widget,
                                    filter_options->controller);
    }
}

static gboolean
ligma_filter_tool_can_pick_color (LigmaColorTool    *color_tool,
                                 const LigmaCoords *coords,
                                 LigmaDisplay      *display)
{
  LigmaTool       *tool        = LIGMA_TOOL (color_tool);
  LigmaFilterTool *filter_tool = LIGMA_FILTER_TOOL (color_tool);
  LigmaImage      *image       = ligma_display_get_image (display);

  if (! ligma_image_equal_selected_drawables (image, tool->drawables))
    return FALSE;

  return filter_tool->pick_abyss ||
         LIGMA_COLOR_TOOL_CLASS (parent_class)->can_pick (color_tool,
                                                         coords, display);
}

static gboolean
ligma_filter_tool_pick_color (LigmaColorTool     *color_tool,
                             const LigmaCoords  *coords,
                             LigmaDisplay       *display,
                             const Babl       **sample_format,
                             gpointer           pixel,
                             LigmaRGB           *color)
{
  LigmaTool       *tool        = LIGMA_TOOL (color_tool);
  LigmaFilterTool *filter_tool = LIGMA_FILTER_TOOL (color_tool);
  gboolean        picked;

  g_return_val_if_fail (g_list_length (tool->drawables) == 1, FALSE);

  picked = LIGMA_COLOR_TOOL_CLASS (parent_class)->pick (color_tool, coords,
                                                       display, sample_format,
                                                       pixel, color);

  if (! picked && filter_tool->pick_abyss)
    {
      color->r = 0.0;
      color->g = 0.0;
      color->b = 0.0;
      color->a = 0.0;

      picked = TRUE;
    }

  return picked;
}

static void
ligma_filter_tool_color_picked (LigmaColorTool      *color_tool,
                               const LigmaCoords   *coords,
                               LigmaDisplay        *display,
                               LigmaColorPickState  pick_state,
                               const Babl         *sample_format,
                               gpointer            pixel,
                               const LigmaRGB      *color)
{
  LigmaFilterTool *filter_tool = LIGMA_FILTER_TOOL (color_tool);

  if (filter_tool->active_picker)
    {
      LigmaPickerCallback callback;
      gpointer           callback_data;

      callback      = g_object_get_data (G_OBJECT (filter_tool->active_picker),
                                         "picker-callback");
      callback_data = g_object_get_data (G_OBJECT (filter_tool->active_picker),
                                         "picker-callback-data");

      if (callback)
        {
          callback (callback_data,
                    filter_tool->pick_identifier,
                    coords->x,
                    coords->y,
                    sample_format, color);

          return;
        }
    }

  LIGMA_FILTER_TOOL_GET_CLASS (filter_tool)->color_picked (filter_tool,
                                                          filter_tool->pick_identifier,
                                                          coords->x,
                                                          coords->y,
                                                          sample_format, color);
}

static void
ligma_filter_tool_real_reset (LigmaFilterTool *filter_tool)
{
  if (filter_tool->config)
    {
      if (filter_tool->default_config)
        {
          ligma_config_copy (LIGMA_CONFIG (filter_tool->default_config),
                            LIGMA_CONFIG (filter_tool->config),
                            0);
        }
      else
        {
          ligma_config_reset (LIGMA_CONFIG (filter_tool->config));
        }
    }
}

static void
ligma_filter_tool_real_set_config (LigmaFilterTool *filter_tool,
                                  LigmaConfig     *config)
{
  LigmaFilterRegion region;

  /* copy the "ligma-region" property first, to avoid incorrectly adjusting the
   * copied operation properties in ligma_operation_tool_region_changed().
   */
  g_object_get (config,
                "ligma-region", &region,
                NULL);
  g_object_set (filter_tool->config,
                "ligma-region", region,
                NULL);

  ligma_config_copy (LIGMA_CONFIG (config),
                    LIGMA_CONFIG (filter_tool->config), 0);

  /*  reset the "time" property, otherwise explicitly storing the
   *  config as setting will also copy the time, and the stored object
   *  will be considered to be among the automatically stored recently
   *  used settings
   */
  g_object_set (filter_tool->config,
                "time", (gint64) 0,
                NULL);
}

static void
ligma_filter_tool_real_config_notify (LigmaFilterTool   *filter_tool,
                                     LigmaConfig       *config,
                                     const GParamSpec *pspec)
{
  LigmaFilterOptions *options = LIGMA_FILTER_TOOL_GET_OPTIONS (filter_tool);

  if (filter_tool->filter)
    {
      /* note that we may be called with a NULL pspec.  see
       * ligma_operation_tool_aux_input_notify().
       */
      if (pspec)
        {
          if (! strcmp (pspec->name, "ligma-clip")    ||
              ! strcmp (pspec->name, "ligma-mode")    ||
              ! strcmp (pspec->name, "ligma-opacity") ||
              ! strcmp (pspec->name, "ligma-gamma-hack"))
            {
              ligma_filter_tool_update_filter (filter_tool);
            }
          else if (! strcmp (pspec->name, "ligma-region"))
            {
              ligma_filter_tool_update_filter (filter_tool);

              ligma_filter_tool_region_changed (filter_tool);
            }
        }

      if (options->preview)
        ligma_drawable_filter_apply (filter_tool->filter, NULL);
    }
}

static void
ligma_filter_tool_halt (LigmaFilterTool *filter_tool)
{
  LigmaTool *tool = LIGMA_TOOL (filter_tool);

  ligma_filter_tool_disable_color_picking (filter_tool);

  if (tool->display)
    {
      LigmaImage *image = ligma_display_get_image (tool->display);
      GList     *iter;

      for (iter = tool->drawables; iter; iter = iter->next)
        g_signal_handlers_disconnect_by_func (iter->data,
                                              ligma_filter_tool_lock_position_changed,
                                              filter_tool);

      g_signal_handlers_disconnect_by_func (image,
                                            ligma_filter_tool_mask_changed,
                                            filter_tool);
    }

  if (filter_tool->gui)
    {
      /* explicitly clear the dialog contents first, since we might be called
       * during the dialog's delete event, in which case the dialog will be
       * externally reffed, and will only die *after* ligma_filter_tool_halt()
       * returns, and, in particular, after filter_tool->config has been
       * cleared.  we want to make sure the gui is destroyed while
       * filter_tool->config is still alive, since the gui's destruction may
       * fire signals whose handlers rely on it.
       */
      ligma_gtk_container_clear (
        GTK_CONTAINER (ligma_filter_tool_dialog_get_vbox (filter_tool)));

      g_clear_object (&filter_tool->gui);
      filter_tool->settings_box      = NULL;
      filter_tool->controller_toggle = NULL;
      filter_tool->clip_combo        = NULL;
      filter_tool->region_combo      = NULL;
    }

  if (filter_tool->filter)
    {
      ligma_drawable_filter_abort (filter_tool->filter);
      g_clear_object (&filter_tool->filter);
      ligma_filter_tool_remove_guide (filter_tool);
    }

  g_clear_object (&filter_tool->operation);

  if (filter_tool->config)
    {
      g_signal_handlers_disconnect_by_func (filter_tool->config,
                                            ligma_filter_tool_config_notify,
                                            filter_tool);
      g_signal_handlers_disconnect_by_func (filter_tool->config,
                                            ligma_filter_tool_unset_setting,
                                            filter_tool);
      g_clear_object (&filter_tool->config);
    }

  g_clear_object (&filter_tool->default_config);
  g_clear_object (&filter_tool->settings);

  if (ligma_draw_tool_is_active (LIGMA_DRAW_TOOL (tool)))
    ligma_draw_tool_stop (LIGMA_DRAW_TOOL (tool));

  ligma_filter_tool_set_widget (filter_tool, NULL);

  tool->display   = NULL;
  g_list_free (tool->drawables);
  tool->drawables = NULL;
}

static void
ligma_filter_tool_commit (LigmaFilterTool *filter_tool)
{
  LigmaTool *tool = LIGMA_TOOL (filter_tool);

  if (filter_tool->gui)
    ligma_tool_gui_hide (filter_tool->gui);

  if (filter_tool->filter)
    {
      LigmaFilterOptions *options = LIGMA_FILTER_TOOL_GET_OPTIONS (tool);

      if (! options->preview)
        ligma_drawable_filter_apply (filter_tool->filter, NULL);

      ligma_tool_control_push_preserve (tool->control, TRUE);

      ligma_drawable_filter_commit (filter_tool->filter,
                                   LIGMA_PROGRESS (tool), TRUE);
      g_clear_object (&filter_tool->filter);

      ligma_tool_control_pop_preserve (tool->control);

      ligma_filter_tool_remove_guide (filter_tool);

      ligma_image_flush (ligma_display_get_image (tool->display));

      if (filter_tool->config && filter_tool->has_settings)
        {
          LigmaGuiConfig *config = LIGMA_GUI_CONFIG (tool->tool_info->ligma->config);

          ligma_settings_box_add_current (LIGMA_SETTINGS_BOX (filter_tool->settings_box),
                                         config->filter_tool_max_recent);
        }
    }
}

static void
ligma_filter_tool_dialog (LigmaFilterTool *filter_tool)
{
  LIGMA_FILTER_TOOL_GET_CLASS (filter_tool)->dialog (filter_tool);
}

static void
ligma_filter_tool_update_dialog_operation_settings (LigmaFilterTool *filter_tool)
{
  LigmaTool          *tool    = LIGMA_TOOL (filter_tool);
  LigmaFilterOptions *options = LIGMA_FILTER_TOOL_GET_OPTIONS (filter_tool);
  LigmaImage         *image   = ligma_display_get_image (tool->display);

  if (filter_tool->operation_settings_box)
    {
      ligma_gtk_container_clear (
        GTK_CONTAINER (filter_tool->operation_settings_box));

      if (filter_tool->config)
        {
          GtkWidget *vbox;
          GtkWidget *expander;
          GtkWidget *frame;
          GtkWidget *vbox2;
          GtkWidget *mode_box;
          GtkWidget *scale;
          GtkWidget *toggle;

          vbox = filter_tool->operation_settings_box;

          /*  The clipping combo  */
          filter_tool->clip_combo =
            ligma_prop_enum_combo_box_new (filter_tool->config,
                                          "ligma-clip",
                                          LIGMA_TRANSFORM_RESIZE_ADJUST,
                                          LIGMA_TRANSFORM_RESIZE_CLIP);
          ligma_int_combo_box_set_label (
            LIGMA_INT_COMBO_BOX (filter_tool->clip_combo), _("Clipping"));
          gtk_box_pack_start (GTK_BOX (vbox), filter_tool->clip_combo,
                              FALSE, FALSE, 0);

          /*  The region combo  */
          filter_tool->region_combo =
            ligma_prop_enum_combo_box_new (filter_tool->config,
                                          "ligma-region",
                                          0, 0);
          gtk_box_pack_start (GTK_BOX (vbox), filter_tool->region_combo,
                              FALSE, FALSE, 0);

          /*  The blending-options expander  */
          expander = gtk_expander_new (_("Blending Options"));
          gtk_box_pack_start (GTK_BOX (vbox), expander,
                              FALSE, FALSE, 0);
          gtk_widget_show (expander);

          g_object_bind_property (options,  "blending-options-expanded",
                                  expander, "expanded",
                                  G_BINDING_SYNC_CREATE |
                                  G_BINDING_BIDIRECTIONAL);

          frame = ligma_frame_new (NULL);
          gtk_container_add (GTK_CONTAINER (expander), frame);
          gtk_widget_show (frame);

          vbox2 = gtk_box_new (GTK_ORIENTATION_VERTICAL, 2);
          gtk_container_add (GTK_CONTAINER (frame), vbox2);
          gtk_widget_show (vbox2);

          /*  The mode box  */
          mode_box = ligma_prop_layer_mode_box_new (
            filter_tool->config, "ligma-mode",
            LIGMA_LAYER_MODE_CONTEXT_FILTER);
          ligma_layer_mode_box_set_label (LIGMA_LAYER_MODE_BOX (mode_box),
                                         _("Mode"));
          gtk_box_pack_start (GTK_BOX (vbox2), mode_box,
                              FALSE, FALSE, 0);
          gtk_widget_show (mode_box);

          /*  The opacity scale  */
          scale = ligma_prop_spin_scale_new (filter_tool->config,
                                            "ligma-opacity",
                                            1.0, 10.0, 1);
          ligma_prop_widget_set_factor (scale, 100.0, 1.0, 10.0, 1);
          gtk_box_pack_start (GTK_BOX (vbox2), scale,
                              FALSE, FALSE, 0);
          gtk_widget_show (scale);

          /*  The Color Options expander  */
          expander = gtk_expander_new (_("Advanced Color Options"));
          gtk_box_pack_start (GTK_BOX (vbox), expander,
                              FALSE, FALSE, 0);

          g_object_bind_property (image->ligma->config,
                                  "filter-tool-show-color-options",
                                  expander, "visible",
                                  G_BINDING_SYNC_CREATE);
          g_object_bind_property (options,  "color-options-expanded",
                                  expander, "expanded",
                                  G_BINDING_SYNC_CREATE |
                                  G_BINDING_BIDIRECTIONAL);

          frame = ligma_frame_new (NULL);
          gtk_container_add (GTK_CONTAINER (expander), frame);
          gtk_widget_show (frame);

          vbox2 = gtk_box_new (GTK_ORIENTATION_VERTICAL, 2);
          gtk_container_add (GTK_CONTAINER (frame), vbox2);
          gtk_widget_show (vbox2);

          /*  The gamma hack toggle  */
          toggle = ligma_prop_check_button_new (filter_tool->config,
                                               "ligma-gamma-hack", NULL);
          gtk_box_pack_start (GTK_BOX (vbox2), toggle,
                              FALSE, FALSE, 0);
          gtk_widget_show (toggle);
        }
    }
}

static void
ligma_filter_tool_reset (LigmaFilterTool *filter_tool)
{
  if (filter_tool->config)
    g_object_freeze_notify (filter_tool->config);

  LIGMA_FILTER_TOOL_GET_CLASS (filter_tool)->reset (filter_tool);

  if (filter_tool->config)
    g_object_thaw_notify (filter_tool->config);

  if (filter_tool->widget)
    ligma_filter_tool_reset_widget (filter_tool, filter_tool->widget);
}

static void
ligma_filter_tool_create_filter (LigmaFilterTool *filter_tool)
{
  LigmaTool          *tool    = LIGMA_TOOL (filter_tool);
  LigmaFilterOptions *options = LIGMA_FILTER_TOOL_GET_OPTIONS (filter_tool);

  if (filter_tool->filter)
    {
      ligma_drawable_filter_abort (filter_tool->filter);
      g_object_unref (filter_tool->filter);
    }

  ligma_assert (filter_tool->operation);
  g_return_if_fail (g_list_length (tool->drawables) == 1);

  filter_tool->filter = ligma_drawable_filter_new (tool->drawables->data,
                                                  ligma_tool_get_undo_desc (tool),
                                                  filter_tool->operation,
                                                  ligma_tool_get_icon_name (tool));

  ligma_filter_tool_update_filter (filter_tool);

  g_signal_connect (filter_tool->filter, "flush",
                    G_CALLBACK (ligma_filter_tool_flush),
                    filter_tool);

  ligma_gegl_progress_connect (filter_tool->operation,
                              LIGMA_PROGRESS (filter_tool),
                              ligma_tool_get_undo_desc (tool));

  if (options->preview)
    ligma_drawable_filter_apply (filter_tool->filter, NULL);
}

static void
ligma_filter_tool_update_dialog (LigmaFilterTool *filter_tool)
{
  LigmaTool *tool = LIGMA_TOOL (filter_tool);

  if (filter_tool->gui)
    {
      LigmaImage   *image = ligma_display_get_image (tool->display);
      LigmaChannel *mask  = ligma_image_get_mask (image);
      const Babl  *format;

      g_return_if_fail (g_list_length (tool->drawables) == 1);

      if (filter_tool->filter)
        format = ligma_drawable_filter_get_format (filter_tool->filter);
      else
        format = ligma_drawable_get_format (tool->drawables->data);

      if (ligma_channel_is_empty (mask))
        {
          gtk_widget_set_visible (
            filter_tool->clip_combo,
            ligma_item_get_clip (LIGMA_ITEM (tool->drawables->data), FALSE) == FALSE &&
            ! ligma_gegl_node_is_point_operation (filter_tool->operation)    &&
            babl_format_has_alpha (format));

          gtk_widget_hide (filter_tool->region_combo);
        }
      else
        {
          gtk_widget_hide (filter_tool->clip_combo);

          gtk_widget_set_visible (
            filter_tool->region_combo,
            ! ligma_gegl_node_is_point_operation (filter_tool->operation) ||
            ligma_gegl_node_has_key (filter_tool->operation,
                                    "position-dependent"));
        }
    }
}

static void
ligma_filter_tool_region_changed (LigmaFilterTool *filter_tool)
{
  if (filter_tool->filter &&
      LIGMA_FILTER_TOOL_GET_CLASS (filter_tool)->region_changed)
    {
      LIGMA_FILTER_TOOL_GET_CLASS (filter_tool)->region_changed (filter_tool);
    }
}

static void
ligma_filter_tool_flush (LigmaDrawableFilter *filter,
                        LigmaFilterTool     *filter_tool)
{
  LigmaTool  *tool  = LIGMA_TOOL (filter_tool);
  LigmaImage *image = ligma_display_get_image (tool->display);

  ligma_projection_flush (ligma_image_get_projection (image));
}

static void
ligma_filter_tool_config_notify (GObject          *object,
                                const GParamSpec *pspec,
                                LigmaFilterTool   *filter_tool)
{
  LIGMA_FILTER_TOOL_GET_CLASS (filter_tool)->config_notify (filter_tool,
                                                           LIGMA_CONFIG (object),
                                                           pspec);
}

static void
ligma_filter_tool_unset_setting (GObject          *object,
                                const GParamSpec *pspec,
                                LigmaFilterTool   *filter_tool)
{
  g_signal_handlers_disconnect_by_func (filter_tool->config,
                                        ligma_filter_tool_unset_setting,
                                        filter_tool);

  ligma_settings_box_unset (LIGMA_SETTINGS_BOX (filter_tool->settings_box));
}

static void
ligma_filter_tool_lock_position_changed (LigmaDrawable   *drawable,
                                        LigmaFilterTool *filter_tool)
{
  ligma_filter_tool_update_dialog (filter_tool);
}

static void
ligma_filter_tool_mask_changed (LigmaImage      *image,
                               LigmaFilterTool *filter_tool)
{
  LigmaOperationSettings *settings;

  settings = LIGMA_OPERATION_SETTINGS (filter_tool->config);

  ligma_filter_tool_update_dialog (filter_tool);

  if (settings && settings->region == LIGMA_FILTER_REGION_SELECTION)
    ligma_filter_tool_region_changed (filter_tool);
}

static void
ligma_filter_tool_add_guide (LigmaFilterTool *filter_tool)
{
  LigmaTool            *tool    = LIGMA_TOOL (filter_tool);
  LigmaFilterOptions   *options = LIGMA_FILTER_TOOL_GET_OPTIONS (filter_tool);
  LigmaItem            *item;
  LigmaImage           *image;
  LigmaOrientationType  orientation;
  gint                 position;

  g_return_if_fail (g_list_length (tool->drawables) == 1);

  if (filter_tool->preview_guide)
    return;

  item  = LIGMA_ITEM (tool->drawables->data);
  image = ligma_item_get_image (item);

  if (options->preview_split_alignment == LIGMA_ALIGN_LEFT ||
      options->preview_split_alignment == LIGMA_ALIGN_RIGHT)
    {
      orientation = LIGMA_ORIENTATION_VERTICAL;
      position    = ligma_item_get_offset_x (item) +
                    options->preview_split_position;
    }
  else
    {
      orientation = LIGMA_ORIENTATION_HORIZONTAL;
      position    = ligma_item_get_offset_y (item) +
                    options->preview_split_position;
    }

  filter_tool->preview_guide =
    ligma_guide_custom_new (orientation,
                           image->ligma->next_guide_id++,
                           LIGMA_GUIDE_STYLE_SPLIT_VIEW);

  ligma_image_add_guide (image, filter_tool->preview_guide, position);

  g_signal_connect (filter_tool->preview_guide, "removed",
                    G_CALLBACK (ligma_filter_tool_guide_removed),
                    filter_tool);
  g_signal_connect (filter_tool->preview_guide, "notify::position",
                    G_CALLBACK (ligma_filter_tool_guide_moved),
                    filter_tool);
}

static void
ligma_filter_tool_remove_guide (LigmaFilterTool *filter_tool)
{
  LigmaTool  *tool = LIGMA_TOOL (filter_tool);
  LigmaImage *image;

  if (! filter_tool->preview_guide)
    return;

  image = ligma_item_get_image (LIGMA_ITEM (tool->drawables->data));

  ligma_image_remove_guide (image, filter_tool->preview_guide, FALSE);
}

static void
ligma_filter_tool_move_guide (LigmaFilterTool *filter_tool)
{
  LigmaTool            *tool    = LIGMA_TOOL (filter_tool);
  LigmaFilterOptions   *options = LIGMA_FILTER_TOOL_GET_OPTIONS (filter_tool);
  LigmaItem            *item;
  LigmaOrientationType  orientation;
  gint                 position;

  g_return_if_fail (g_list_length (tool->drawables) == 1);

  if (! filter_tool->preview_guide)
    return;

  item = LIGMA_ITEM (tool->drawables->data);

  if (options->preview_split_alignment == LIGMA_ALIGN_LEFT ||
      options->preview_split_alignment == LIGMA_ALIGN_RIGHT)
    {
      orientation = LIGMA_ORIENTATION_VERTICAL;
      position    = ligma_item_get_offset_x (item) +
                    options->preview_split_position;
    }
  else
    {
      orientation = LIGMA_ORIENTATION_HORIZONTAL;
      position    = ligma_item_get_offset_x (item) +
                    options->preview_split_position;
    }

  if (orientation != ligma_guide_get_orientation (filter_tool->preview_guide) ||
      position    != ligma_guide_get_position (filter_tool->preview_guide))
    {
      ligma_guide_set_orientation (filter_tool->preview_guide, orientation);
      ligma_image_move_guide (ligma_item_get_image (item),
                             filter_tool->preview_guide, position, FALSE);
    }
}

static void
ligma_filter_tool_guide_removed (LigmaGuide      *guide,
                                LigmaFilterTool *filter_tool)
{
  LigmaFilterOptions *options = LIGMA_FILTER_TOOL_GET_OPTIONS (filter_tool);

  g_signal_handlers_disconnect_by_func (G_OBJECT (filter_tool->preview_guide),
                                        ligma_filter_tool_guide_removed,
                                        filter_tool);
  g_signal_handlers_disconnect_by_func (G_OBJECT (filter_tool->preview_guide),
                                        ligma_filter_tool_guide_moved,
                                        filter_tool);

  g_clear_object (&filter_tool->preview_guide);

  g_object_set (options,
                "preview-split", FALSE,
                NULL);
}

static void
ligma_filter_tool_guide_moved (LigmaGuide        *guide,
                              const GParamSpec *pspec,
                              LigmaFilterTool   *filter_tool)
{
  LigmaTool          *tool    = LIGMA_TOOL (filter_tool);
  LigmaFilterOptions *options = LIGMA_FILTER_TOOL_GET_OPTIONS (filter_tool);
  LigmaItem          *item;
  gint               position;

  g_return_if_fail (g_list_length (tool->drawables) == 1);

  item = LIGMA_ITEM (tool->drawables->data);

  if (options->preview_split_alignment == LIGMA_ALIGN_LEFT ||
      options->preview_split_alignment == LIGMA_ALIGN_RIGHT)
    {
      position = CLAMP (ligma_guide_get_position (guide) -
                        ligma_item_get_offset_x (item),
                        0, ligma_item_get_width (item));
    }
  else
    {
      position = CLAMP (ligma_guide_get_position (guide) -
                        ligma_item_get_offset_y (item),
                        0, ligma_item_get_height (item));
    }

  g_object_set (options,
                "preview-split-position", position,
                NULL);
}

static void
ligma_filter_tool_response (LigmaToolGui    *gui,
                           gint            response_id,
                           LigmaFilterTool *filter_tool)
{
  LigmaTool *tool = LIGMA_TOOL (filter_tool);

  switch (response_id)
    {
    case RESPONSE_RESET:
      ligma_filter_tool_reset (filter_tool);
      break;

    case GTK_RESPONSE_OK:
      ligma_tool_control (tool, LIGMA_TOOL_ACTION_COMMIT, tool->display);
      break;

    default:
      ligma_tool_control (tool, LIGMA_TOOL_ACTION_HALT, tool->display);
      break;
    }
}

static void
ligma_filter_tool_update_filter (LigmaFilterTool *filter_tool)
{
  LigmaFilterOptions     *options  = LIGMA_FILTER_TOOL_GET_OPTIONS (filter_tool);
  LigmaOperationSettings *settings;

  if (! filter_tool->filter)
    return;

  settings = LIGMA_OPERATION_SETTINGS (filter_tool->config);

  ligma_drawable_filter_set_preview       (filter_tool->filter,
                                          options->preview);
  ligma_drawable_filter_set_preview_split (filter_tool->filter,
                                          options->preview_split,
                                          options->preview_split_alignment,
                                          options->preview_split_position);
  ligma_drawable_filter_set_add_alpha     (filter_tool->filter,
                                          ligma_gegl_node_has_key (
                                            filter_tool->operation,
                                            "needs-alpha"));

  ligma_operation_settings_sync_drawable_filter (settings, filter_tool->filter);
}

static void
ligma_filter_tool_set_has_settings (LigmaFilterTool *filter_tool,
                                   gboolean        has_settings)
{
  g_return_if_fail (LIGMA_IS_FILTER_TOOL (filter_tool));

  filter_tool->has_settings = has_settings;

  if (! filter_tool->settings_box)
    return;

  if (filter_tool->has_settings)
    {
      LigmaTool *tool  = LIGMA_TOOL (filter_tool);
      GQuark    quark = g_quark_from_static_string ("settings-folder");
      GType     type  = G_TYPE_FROM_INSTANCE (filter_tool->config);
      GFile    *settings_folder;
      gchar    *import_title;
      gchar    *export_title;

      settings_folder = g_type_get_qdata (type, quark);

      import_title = g_strdup_printf (_("Import '%s' Settings"),
                                      ligma_tool_get_label (tool));
      export_title = g_strdup_printf (_("Export '%s' Settings"),
                                      ligma_tool_get_label (tool));

      g_object_set (filter_tool->settings_box,
                    "visible",        TRUE,
                    "config",         filter_tool->config,
                    "container",      filter_tool->settings,
                    "help-id",        ligma_tool_get_help_id (tool),
                    "import-title",   import_title,
                    "export-title",   export_title,
                    "default-folder", settings_folder,
                    "last-file",      NULL,
                    NULL);

      g_free (import_title);
      g_free (export_title);
    }
  else
    {
      g_object_set (filter_tool->settings_box,
                    "visible",        FALSE,
                    "config",         NULL,
                    "container",      NULL,
                    "help-id",        NULL,
                    "import-title",   NULL,
                    "export-title",   NULL,
                    "default-folder", NULL,
                    "last-file",      NULL,
                    NULL);
    }
}


/*  public functions  */

void
ligma_filter_tool_get_operation (LigmaFilterTool *filter_tool)
{
  LigmaTool             *tool;
  LigmaFilterToolClass  *klass;
  gchar                *operation_name;
  GParamSpec          **pspecs;

  g_return_if_fail (LIGMA_IS_FILTER_TOOL (filter_tool));

  tool  = LIGMA_TOOL (filter_tool);
  klass = LIGMA_FILTER_TOOL_GET_CLASS (filter_tool);

  if (filter_tool->filter)
    {
      ligma_drawable_filter_abort (filter_tool->filter);
      g_clear_object (&filter_tool->filter);

      ligma_filter_tool_remove_guide (filter_tool);
    }

  g_clear_object (&filter_tool->operation);

  if (filter_tool->config)
    {
      g_signal_handlers_disconnect_by_func (filter_tool->config,
                                            ligma_filter_tool_config_notify,
                                            filter_tool);
      g_signal_handlers_disconnect_by_func (filter_tool->config,
                                            ligma_filter_tool_unset_setting,
                                            filter_tool);
      g_clear_object (&filter_tool->config);
    }

  g_clear_object (&filter_tool->default_config);
  g_clear_object (&filter_tool->settings);
  g_clear_pointer (&filter_tool->description, g_free);

  operation_name = klass->get_operation (filter_tool,
                                         &filter_tool->description);

  if (! operation_name)
    operation_name = g_strdup ("gegl:nop");

  if (! filter_tool->description)
    filter_tool->description = g_strdup (ligma_tool_get_label (tool));

  filter_tool->operation = gegl_node_new_child (NULL,
                                                "operation", operation_name,
                                                NULL);

  filter_tool->config =
    g_object_new (ligma_operation_config_get_type (tool->tool_info->ligma,
                                                  operation_name,
                                                  ligma_tool_get_icon_name (tool),
                                                  LIGMA_TYPE_OPERATION_SETTINGS),
                  NULL);

  ligma_operation_config_sync_node (filter_tool->config,
                                   filter_tool->operation);
  ligma_operation_config_connect_node (filter_tool->config,
                                      filter_tool->operation);

  filter_tool->settings =
    ligma_operation_config_get_container (tool->tool_info->ligma,
                                         G_TYPE_FROM_INSTANCE (filter_tool->config),
                                         (GCompareFunc) ligma_settings_compare);
  g_object_ref (filter_tool->settings);

  pspecs =
    ligma_operation_config_list_properties (filter_tool->config,
                                           G_TYPE_FROM_INSTANCE (filter_tool->config),
                                           0, NULL);

  ligma_filter_tool_set_has_settings (filter_tool, (pspecs != NULL));

  g_free (pspecs);

  if (filter_tool->gui)
    {
      ligma_tool_gui_set_title       (filter_tool->gui,
                                     ligma_tool_get_label (tool));
      ligma_tool_gui_set_description (filter_tool->gui, filter_tool->description);
      ligma_tool_gui_set_icon_name   (filter_tool->gui,
                                     ligma_tool_get_icon_name (tool));
      ligma_tool_gui_set_help_id     (filter_tool->gui,
                                     ligma_tool_get_help_id (tool));

      ligma_filter_tool_update_dialog_operation_settings (filter_tool);
    }

  ligma_filter_tool_update_dialog (filter_tool);

  g_free (operation_name);

  g_object_set (LIGMA_FILTER_TOOL_GET_OPTIONS (filter_tool),
                "preview-split", FALSE,
                NULL);

  g_signal_connect_object (filter_tool->config, "notify",
                           G_CALLBACK (ligma_filter_tool_config_notify),
                           G_OBJECT (filter_tool), 0);

  if (tool->drawables)
    ligma_filter_tool_create_filter (filter_tool);
}

void
ligma_filter_tool_set_config (LigmaFilterTool *filter_tool,
                             LigmaConfig     *config)
{
  g_return_if_fail (LIGMA_IS_FILTER_TOOL (filter_tool));
  g_return_if_fail (LIGMA_IS_OPERATION_SETTINGS (config));

  /* if the user didn't change a setting since the last set_config(),
   * this handler is still connected
   */
  g_signal_handlers_disconnect_by_func (filter_tool->config,
                                        ligma_filter_tool_unset_setting,
                                        filter_tool);

  LIGMA_FILTER_TOOL_GET_CLASS (filter_tool)->set_config (filter_tool, config);

  if (filter_tool->widget)
    ligma_filter_tool_reset_widget (filter_tool, filter_tool->widget);

  if (filter_tool->settings_box)
    g_signal_connect_object (filter_tool->config, "notify",
                             G_CALLBACK (ligma_filter_tool_unset_setting),
                             G_OBJECT (filter_tool), 0);
}

void
ligma_filter_tool_edit_as (LigmaFilterTool *filter_tool,
                          const gchar    *new_tool_id,
                          LigmaConfig     *config)
{
  LigmaDisplay  *display;
  LigmaContext  *user_context;
  LigmaToolInfo *tool_info;
  LigmaTool     *new_tool;

  g_return_if_fail (LIGMA_IS_FILTER_TOOL (filter_tool));
  g_return_if_fail (new_tool_id != NULL);
  g_return_if_fail (LIGMA_IS_CONFIG (config));

  display = LIGMA_TOOL (filter_tool)->display;

  user_context = ligma_get_user_context (display->ligma);

  tool_info = (LigmaToolInfo *)
    ligma_container_get_child_by_name (display->ligma->tool_info_list,
                                      new_tool_id);

  ligma_tool_control (LIGMA_TOOL (filter_tool), LIGMA_TOOL_ACTION_HALT, display);
  ligma_context_set_tool (user_context, tool_info);
  tool_manager_initialize_active (display->ligma, display);

  new_tool = tool_manager_get_active (display->ligma);

  LIGMA_FILTER_TOOL (new_tool)->default_config = g_object_ref (G_OBJECT (config));

  ligma_filter_tool_reset (LIGMA_FILTER_TOOL (new_tool));
}

gboolean
ligma_filter_tool_on_guide (LigmaFilterTool   *filter_tool,
                           const LigmaCoords *coords,
                           LigmaDisplay      *display)
{
  LigmaDisplayShell *shell;

  g_return_val_if_fail (LIGMA_IS_FILTER_TOOL (filter_tool), FALSE);
  g_return_val_if_fail (coords != NULL, FALSE);
  g_return_val_if_fail (LIGMA_IS_DISPLAY (display), FALSE);

  shell = ligma_display_get_shell (display);

  if (filter_tool->filter        &&
      filter_tool->preview_guide &&
      ligma_display_shell_get_show_guides (shell))
    {
      const gint          snap_distance = display->config->snap_distance;
      LigmaOrientationType orientation;
      gint                position;

      orientation = ligma_guide_get_orientation (filter_tool->preview_guide);
      position    = ligma_guide_get_position (filter_tool->preview_guide);

      if (orientation == LIGMA_ORIENTATION_HORIZONTAL)
        {
          if (fabs (coords->y - position) <= FUNSCALEY (shell, snap_distance))
            return TRUE;
        }
      else
        {
          if (fabs (coords->x - position) <= FUNSCALEX (shell, snap_distance))
            return TRUE;
        }
    }

  return FALSE;
}

GtkWidget *
ligma_filter_tool_dialog_get_vbox (LigmaFilterTool *filter_tool)
{
  g_return_val_if_fail (LIGMA_IS_FILTER_TOOL (filter_tool), NULL);

  return ligma_tool_gui_get_vbox (filter_tool->gui);
}

void
ligma_filter_tool_enable_color_picking (LigmaFilterTool *filter_tool,
                                       gpointer        identifier,
                                       gboolean        pick_abyss)
{
  g_return_if_fail (LIGMA_IS_FILTER_TOOL (filter_tool));

  ligma_filter_tool_disable_color_picking (filter_tool);

  /* note that ownership over 'identifier' is not transferred, and its
   * lifetime should be managed by the caller.
   */
  filter_tool->pick_identifier = identifier;
  filter_tool->pick_abyss      = pick_abyss;

  ligma_color_tool_enable (LIGMA_COLOR_TOOL (filter_tool),
                          LIGMA_COLOR_TOOL_GET_OPTIONS (filter_tool));
}

void
ligma_filter_tool_disable_color_picking (LigmaFilterTool *filter_tool)
{
  g_return_if_fail (LIGMA_IS_FILTER_TOOL (filter_tool));

  if (filter_tool->active_picker)
    {
      GtkToggleButton *toggle = GTK_TOGGLE_BUTTON (filter_tool->active_picker);

      filter_tool->active_picker = NULL;

      gtk_toggle_button_set_active (toggle, FALSE);
    }

  if (ligma_color_tool_is_enabled (LIGMA_COLOR_TOOL (filter_tool)))
    ligma_color_tool_disable (LIGMA_COLOR_TOOL (filter_tool));
}

static void
ligma_filter_tool_color_picker_toggled (GtkWidget      *widget,
                                       LigmaFilterTool *filter_tool)
{
  if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (widget)))
    {
      gpointer identifier;
      gboolean pick_abyss;

      if (filter_tool->active_picker == widget)
        return;

      identifier =                  g_object_get_data (G_OBJECT (widget),
                                                       "picker-identifier");
      pick_abyss = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (widget),
                                                       "picker-pick-abyss"));

      ligma_filter_tool_enable_color_picking (filter_tool,
                                             identifier, pick_abyss);

      filter_tool->active_picker = widget;
    }
  else if (filter_tool->active_picker == widget)
    {
      ligma_filter_tool_disable_color_picking (filter_tool);
    }
}

GtkWidget *
ligma_filter_tool_add_color_picker (LigmaFilterTool     *filter_tool,
                                   gpointer            identifier,
                                   const gchar        *icon_name,
                                   const gchar        *tooltip,
                                   gboolean            pick_abyss,
                                   LigmaPickerCallback  callback,
                                   gpointer            callback_data)
{
  GtkWidget *button;
  GtkWidget *image;

  g_return_val_if_fail (LIGMA_IS_FILTER_TOOL (filter_tool), NULL);
  g_return_val_if_fail (icon_name != NULL, NULL);

  button = g_object_new (GTK_TYPE_TOGGLE_BUTTON,
                         "draw-indicator", FALSE,
                         NULL);

  image = gtk_image_new_from_icon_name (icon_name, GTK_ICON_SIZE_BUTTON);
  g_object_set (image,
                "margin-start",  2,
                "margin-end",    2,
                "margin-top",    2,
                "margin-bottom", 2,
                NULL);
  gtk_container_add (GTK_CONTAINER (button), image);
  gtk_widget_show (image);

  if (tooltip)
    ligma_help_set_help_data (button, tooltip, NULL);

  g_object_set_data (G_OBJECT (button),
                     "picker-identifier", identifier);
  g_object_set_data (G_OBJECT (button),
                     "picker-pick-abyss", GINT_TO_POINTER (pick_abyss));
  g_object_set_data (G_OBJECT (button),
                     "picker-callback", callback);
  g_object_set_data (G_OBJECT (button),
                     "picker-callback-data", callback_data);

  g_signal_connect (button, "toggled",
                    G_CALLBACK (ligma_filter_tool_color_picker_toggled),
                    filter_tool);

  return button;
}

GCallback
ligma_filter_tool_add_controller (LigmaFilterTool     *filter_tool,
                                 LigmaControllerType  controller_type,
                                 const gchar        *status_title,
                                 GCallback           callback,
                                 gpointer            callback_data,
                                 gpointer           *set_func_data)
{
  LigmaToolWidget *widget;
  GCallback       set_func;

  g_return_val_if_fail (LIGMA_IS_FILTER_TOOL (filter_tool), NULL);
  g_return_val_if_fail (callback != NULL, NULL);
  g_return_val_if_fail (callback_data != NULL, NULL);
  g_return_val_if_fail (set_func_data != NULL, NULL);

  widget = ligma_filter_tool_create_widget (filter_tool,
                                           controller_type,
                                           status_title,
                                           callback,
                                           callback_data,
                                           &set_func,
                                           set_func_data);
  ligma_filter_tool_set_widget (filter_tool, widget);
  g_object_unref (widget);

  return set_func;
}

void
ligma_filter_tool_set_widget (LigmaFilterTool *filter_tool,
                             LigmaToolWidget *widget)
{
  g_return_if_fail (LIGMA_IS_FILTER_TOOL (filter_tool));
  g_return_if_fail (widget == NULL || LIGMA_IS_TOOL_WIDGET (widget));

  if (widget == filter_tool->widget)
    return;

  if (filter_tool->widget)
    {
      if (ligma_draw_tool_is_active (LIGMA_DRAW_TOOL (filter_tool)))
        ligma_draw_tool_stop (LIGMA_DRAW_TOOL (filter_tool));

      g_object_unref (filter_tool->widget);
    }

  filter_tool->widget = widget;
  ligma_draw_tool_set_widget (LIGMA_DRAW_TOOL (filter_tool), widget);

  if (filter_tool->widget)
    {
      LigmaFilterOptions *options = LIGMA_FILTER_TOOL_GET_OPTIONS (filter_tool);

      g_object_ref (filter_tool->widget);

      ligma_tool_widget_set_visible (filter_tool->widget,
                                    options->controller);

      if (LIGMA_TOOL (filter_tool)->display)
        ligma_draw_tool_start (LIGMA_DRAW_TOOL (filter_tool),
                              LIGMA_TOOL (filter_tool)->display);
    }

  if (filter_tool->controller_toggle)
    {
      gtk_widget_set_visible (filter_tool->controller_toggle,
                              filter_tool->widget != NULL);
    }
}

gboolean
ligma_filter_tool_get_drawable_area (LigmaFilterTool *filter_tool,
                                    gint           *drawable_offset_x,
                                    gint           *drawable_offset_y,
                                    GeglRectangle  *drawable_area)
{
  LigmaTool              *tool;
  LigmaOperationSettings *settings;
  LigmaDrawable          *drawable;

  g_return_val_if_fail (LIGMA_IS_FILTER_TOOL (filter_tool), FALSE);
  g_return_val_if_fail (drawable_offset_x != NULL, FALSE);
  g_return_val_if_fail (drawable_offset_y != NULL, FALSE);
  g_return_val_if_fail (drawable_area != NULL, FALSE);

  tool     = LIGMA_TOOL (filter_tool);
  settings = LIGMA_OPERATION_SETTINGS (filter_tool->config);

  g_return_val_if_fail (g_list_length (tool->drawables) == 1, FALSE);

  *drawable_offset_x = 0;
  *drawable_offset_y = 0;

  drawable_area->x      = 0;
  drawable_area->y      = 0;
  drawable_area->width  = 1;
  drawable_area->height = 1;

  drawable = tool->drawables->data;

  if (drawable && settings)
    {
      ligma_item_get_offset (LIGMA_ITEM (drawable),
                            drawable_offset_x, drawable_offset_y);

      switch (settings->region)
        {
        case LIGMA_FILTER_REGION_SELECTION:
          if (! ligma_item_mask_intersect (LIGMA_ITEM (drawable),
                                          &drawable_area->x,
                                          &drawable_area->y,
                                          &drawable_area->width,
                                          &drawable_area->height))
            {
              drawable_area->x      = 0;
              drawable_area->y      = 0;
              drawable_area->width  = 1;
              drawable_area->height = 1;
            }
          break;

        case LIGMA_FILTER_REGION_DRAWABLE:
          drawable_area->width  = ligma_item_get_width  (LIGMA_ITEM (drawable));
          drawable_area->height = ligma_item_get_height (LIGMA_ITEM (drawable));
          break;
        }

      return TRUE;
    }

  return FALSE;
}
