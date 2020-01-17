/* GIMP - The GNU Image Manipulation Program
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
 * For the core side of this, please see app/core/gimpdrawablefilter.c.
 */

#include "config.h"

#include <gegl.h>
#include <gegl-plugin.h>
#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>

#include "libgimpbase/gimpbase.h"
#include "libgimpconfig/gimpconfig.h"
#include "libgimpmath/gimpmath.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "tools-types.h"

#include "config/gimpguiconfig.h"

#include "operations/gimp-operation-config.h"

#include "gegl/gimp-gegl-utils.h"

#include "core/gimp.h"
#include "core/gimpchannel.h"
#include "core/gimpdrawable.h"
#include "core/gimpdrawablefilter.h"
#include "core/gimperror.h"
#include "core/gimpguide.h"
#include "core/gimpimage.h"
#include "core/gimpimage-guides.h"
#include "core/gimpimage-pick-color.h"
#include "core/gimplayer.h"
#include "core/gimplist.h"
#include "core/gimppickable.h"
#include "core/gimpprogress.h"
#include "core/gimpprojection.h"
#include "core/gimpsettings.h"
#include "core/gimptoolinfo.h"

#include "widgets/gimpsettingsbox.h"
#include "widgets/gimpwidgets-utils.h"

#include "display/gimpdisplay.h"
#include "display/gimpdisplayshell.h"
#include "display/gimpdisplayshell-appearance.h"
#include "display/gimpdisplayshell-transform.h"
#include "display/gimptoolgui.h"
#include "display/gimptoolwidget.h"

#include "gimpfilteroptions.h"
#include "gimpfiltertool.h"
#include "gimpfiltertool-settings.h"
#include "gimpfiltertool-widgets.h"
#include "gimpguidetool.h"
#include "gimptoolcontrol.h"
#include "gimptools-utils.h"
#include "tool_manager.h"

#include "gimp-intl.h"


/*  local function prototypes  */

static void      gimp_filter_tool_finalize       (GObject             *object);

static gboolean  gimp_filter_tool_initialize     (GimpTool            *tool,
                                                  GimpDisplay         *display,
                                                  GError             **error);
static void      gimp_filter_tool_control        (GimpTool            *tool,
                                                  GimpToolAction       action,
                                                  GimpDisplay         *display);
static void      gimp_filter_tool_button_press   (GimpTool            *tool,
                                                  const GimpCoords    *coords,
                                                  guint32              time,
                                                  GdkModifierType      state,
                                                  GimpButtonPressType  press_type,
                                                  GimpDisplay         *display);
static void      gimp_filter_tool_button_release (GimpTool            *tool,
                                                  const GimpCoords    *coords,
                                                  guint32              time,
                                                  GdkModifierType      state,
                                                  GimpButtonReleaseType release_type,
                                                  GimpDisplay         *display);
static void      gimp_filter_tool_motion         (GimpTool            *tool,
                                                  const GimpCoords    *coords,
                                                  guint32              time,
                                                  GdkModifierType      state,
                                                  GimpDisplay         *display);
static gboolean  gimp_filter_tool_key_press      (GimpTool            *tool,
                                                  GdkEventKey         *kevent,
                                                  GimpDisplay         *display);
static void      gimp_filter_tool_oper_update    (GimpTool            *tool,
                                                  const GimpCoords    *coords,
                                                  GdkModifierType      state,
                                                  gboolean             proximity,
                                                  GimpDisplay         *display);
static void      gimp_filter_tool_cursor_update  (GimpTool            *tool,
                                                  const GimpCoords    *coords,
                                                  GdkModifierType      state,
                                                  GimpDisplay         *display);
static void      gimp_filter_tool_options_notify (GimpTool            *tool,
                                                  GimpToolOptions     *options,
                                                  const GParamSpec    *pspec);

static gboolean  gimp_filter_tool_can_pick_color (GimpColorTool       *color_tool,
                                                  const GimpCoords    *coords,
                                                  GimpDisplay         *display);
static gboolean  gimp_filter_tool_pick_color     (GimpColorTool       *color_tool,
                                                  const GimpCoords    *coords,
                                                  GimpDisplay         *display,
                                                  const Babl         **sample_format,
                                                  gpointer             pixel,
                                                  GimpRGB             *color);
static void      gimp_filter_tool_color_picked   (GimpColorTool       *color_tool,
                                                  const GimpCoords    *coords,
                                                  GimpDisplay         *display,
                                                  GimpColorPickState   pick_state,
                                                  const Babl          *sample_format,
                                                  gpointer             pixel,
                                                  const GimpRGB       *color);

static void      gimp_filter_tool_real_reset     (GimpFilterTool      *filter_tool);
static void      gimp_filter_tool_real_set_config(GimpFilterTool      *filter_tool,
                                                  GimpConfig          *config);
static void      gimp_filter_tool_real_config_notify
                                                 (GimpFilterTool      *filter_tool,
                                                  GimpConfig          *config,
                                                  const GParamSpec    *pspec);

static void      gimp_filter_tool_halt           (GimpFilterTool      *filter_tool);
static void      gimp_filter_tool_commit         (GimpFilterTool      *filter_tool);

static void      gimp_filter_tool_dialog         (GimpFilterTool      *filter_tool);
static void      gimp_filter_tool_reset          (GimpFilterTool      *filter_tool);

static void      gimp_filter_tool_create_filter  (GimpFilterTool      *filter_tool);

static void      gimp_filter_tool_update_dialog  (GimpFilterTool      *filter_tool);

static void      gimp_filter_tool_region_changed (GimpFilterTool      *filter_tool);

static void      gimp_filter_tool_flush          (GimpDrawableFilter  *filter,
                                                  GimpFilterTool      *filter_tool);
static void      gimp_filter_tool_config_notify  (GObject             *object,
                                                  const GParamSpec    *pspec,
                                                  GimpFilterTool      *filter_tool);
static void      gimp_filter_tool_unset_setting  (GObject             *object,
                                                  const GParamSpec    *pspec,
                                                  GimpFilterTool      *filter_tool);
static void      gimp_filter_tool_lock_position_changed
                                                 (GimpDrawable        *drawable,
                                                  GimpFilterTool      *filter_tool);
static void      gimp_filter_tool_mask_changed   (GimpImage           *image,
                                                  GimpFilterTool      *filter_tool);

static void      gimp_filter_tool_add_guide      (GimpFilterTool      *filter_tool);
static void      gimp_filter_tool_remove_guide   (GimpFilterTool      *filter_tool);
static void      gimp_filter_tool_move_guide     (GimpFilterTool      *filter_tool);
static void      gimp_filter_tool_guide_removed  (GimpGuide           *guide,
                                                  GimpFilterTool      *filter_tool);
static void      gimp_filter_tool_guide_moved    (GimpGuide           *guide,
                                                  const GParamSpec    *pspec,
                                                  GimpFilterTool      *filter_tool);

static void      gimp_filter_tool_response       (GimpToolGui         *gui,
                                                  gint                 response_id,
                                                  GimpFilterTool      *filter_tool);

static void      gimp_filter_tool_update_filter  (GimpFilterTool      *filter_tool);

static void    gimp_filter_tool_set_has_settings (GimpFilterTool      *filter_tool,
                                                  gboolean             has_settings);


G_DEFINE_TYPE (GimpFilterTool, gimp_filter_tool, GIMP_TYPE_COLOR_TOOL)

#define parent_class gimp_filter_tool_parent_class


static void
gimp_filter_tool_class_init (GimpFilterToolClass *klass)
{
  GObjectClass       *object_class     = G_OBJECT_CLASS (klass);
  GimpToolClass      *tool_class       = GIMP_TOOL_CLASS (klass);
  GimpColorToolClass *color_tool_class = GIMP_COLOR_TOOL_CLASS (klass);

  object_class->finalize     = gimp_filter_tool_finalize;

  tool_class->initialize     = gimp_filter_tool_initialize;
  tool_class->control        = gimp_filter_tool_control;
  tool_class->button_press   = gimp_filter_tool_button_press;
  tool_class->button_release = gimp_filter_tool_button_release;
  tool_class->motion         = gimp_filter_tool_motion;
  tool_class->key_press      = gimp_filter_tool_key_press;
  tool_class->oper_update    = gimp_filter_tool_oper_update;
  tool_class->cursor_update  = gimp_filter_tool_cursor_update;
  tool_class->options_notify = gimp_filter_tool_options_notify;

  color_tool_class->can_pick = gimp_filter_tool_can_pick_color;
  color_tool_class->pick     = gimp_filter_tool_pick_color;
  color_tool_class->picked   = gimp_filter_tool_color_picked;

  klass->get_operation       = NULL;
  klass->dialog              = NULL;
  klass->reset               = gimp_filter_tool_real_reset;
  klass->set_config          = gimp_filter_tool_real_set_config;
  klass->config_notify       = gimp_filter_tool_real_config_notify;
  klass->settings_import     = gimp_filter_tool_real_settings_import;
  klass->settings_export     = gimp_filter_tool_real_settings_export;
}

static void
gimp_filter_tool_init (GimpFilterTool *filter_tool)
{
  GimpTool *tool = GIMP_TOOL (filter_tool);

  gimp_tool_control_set_scroll_lock      (tool->control, TRUE);
  gimp_tool_control_set_preserve         (tool->control, FALSE);
  gimp_tool_control_set_dirty_mask       (tool->control,
                                          GIMP_DIRTY_IMAGE           |
                                          GIMP_DIRTY_IMAGE_STRUCTURE |
                                          GIMP_DIRTY_DRAWABLE        |
                                          GIMP_DIRTY_ACTIVE_DRAWABLE);
  gimp_tool_control_set_active_modifiers (tool->control,
                                          GIMP_TOOL_ACTIVE_MODIFIERS_SEPARATE);
}

static void
gimp_filter_tool_finalize (GObject *object)
{
  GimpFilterTool *filter_tool = GIMP_FILTER_TOOL (object);

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
gimp_filter_tool_initialize (GimpTool     *tool,
                             GimpDisplay  *display,
                             GError      **error)
{
  GimpFilterTool   *filter_tool = GIMP_FILTER_TOOL (tool);
  GimpToolInfo     *tool_info   = tool->tool_info;
  GimpGuiConfig    *config      = GIMP_GUI_CONFIG (display->gimp->config);
  GimpImage        *image       = gimp_display_get_image (display);
  GimpDrawable     *drawable    = gimp_image_get_active_drawable (image);
  GimpDisplayShell *shell       = gimp_display_get_shell (display);

  if (! drawable)
    return FALSE;

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
      if (error)
        gimp_tools_blink_lock_box (display->gimp, GIMP_ITEM (drawable));
      return FALSE;
    }

  if (! gimp_item_is_visible (GIMP_ITEM (drawable)) &&
      ! config->edit_non_visible)
    {
      g_set_error_literal (error, GIMP_ERROR, GIMP_FAILED,
                           _("The active layer is not visible."));
      return FALSE;
    }

  gimp_filter_tool_get_operation (filter_tool);

  gimp_filter_tool_disable_color_picking (filter_tool);

  tool->display  = display;
  tool->drawable = drawable;

  if (filter_tool->config)
    gimp_config_reset (GIMP_CONFIG (filter_tool->config));

  if (! filter_tool->gui)
    {
      GtkWidget *vbox;
      GtkWidget *hbox;
      GtkWidget *toggle;
      GtkWidget *expander;
      GtkWidget *frame;
      GtkWidget *vbox2;
      GtkWidget *combo;

      /*  disabled for at least GIMP 2.8  */
      filter_tool->overlay = FALSE;

      filter_tool->gui =
        gimp_tool_gui_new (tool_info,
                           gimp_tool_get_label (tool),
                           filter_tool->description,
                           gimp_tool_get_icon_name (tool),
                           gimp_tool_get_help_id (tool),
                           gtk_widget_get_screen (GTK_WIDGET (shell)),
                           gimp_widget_get_monitor (GTK_WIDGET (shell)),
                           filter_tool->overlay,

                           _("_Reset"),  RESPONSE_RESET,
                           _("_Cancel"), GTK_RESPONSE_CANCEL,
                           _("_OK"),     GTK_RESPONSE_OK,

                           NULL);

      gimp_tool_gui_set_default_response (filter_tool->gui, GTK_RESPONSE_OK);

      gimp_tool_gui_set_alternative_button_order (filter_tool->gui,
                                                  RESPONSE_RESET,
                                                  GTK_RESPONSE_OK,
                                                  GTK_RESPONSE_CANCEL,
                                                  -1);

      vbox = gimp_tool_gui_get_vbox (filter_tool->gui);

      g_signal_connect_object (filter_tool->gui, "response",
                               G_CALLBACK (gimp_filter_tool_response),
                               G_OBJECT (filter_tool), 0);

      if (filter_tool->config)
        {
          filter_tool->settings_box =
            gimp_filter_tool_get_settings_box (filter_tool);
          gtk_box_pack_start (GTK_BOX (vbox), filter_tool->settings_box,
                              FALSE, FALSE, 0);

          if (filter_tool->has_settings)
            gtk_widget_show (filter_tool->settings_box);
        }

      /*  The preview and split view toggles  */
      hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 4);
      gtk_box_pack_end (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
      gtk_widget_show (hbox);

      toggle = gimp_prop_check_button_new (G_OBJECT (tool_info->tool_options),
                                           "preview", NULL);
      gtk_box_pack_start (GTK_BOX (hbox), toggle, TRUE, TRUE, 0);
      gtk_widget_show (toggle);

      toggle = gimp_prop_check_button_new (G_OBJECT (tool_info->tool_options),
                                           "preview-split", NULL);
      gtk_box_pack_start (GTK_BOX (hbox), toggle, FALSE, FALSE, 0);
      gtk_widget_show (toggle);

      g_object_bind_property (G_OBJECT (tool_info->tool_options), "preview",
                              toggle,                             "sensitive",
                              G_BINDING_SYNC_CREATE);

      /*  The show-controller toggle  */
      filter_tool->controller_toggle =
        gimp_prop_check_button_new (G_OBJECT (tool_info->tool_options),
                                    "controller", NULL);
      gtk_box_pack_end (GTK_BOX (vbox), filter_tool->controller_toggle,
                        FALSE, FALSE, 0);
      if (filter_tool->widget)
        gtk_widget_show (filter_tool->controller_toggle);

      /*  The Color Options expander  */
      expander = gtk_expander_new (_("Advanced Color Options"));
      gtk_box_pack_end (GTK_BOX (vbox), expander, FALSE, FALSE, 0);

      g_object_bind_property (G_OBJECT (image->gimp->config),
                              "filter-tool-show-color-options",
                              G_OBJECT (expander), "visible",
                              G_BINDING_SYNC_CREATE);

      frame = gimp_frame_new (NULL);
      gtk_container_add (GTK_CONTAINER (expander), frame);
      gtk_widget_show (frame);

      vbox2 = gtk_box_new (GTK_ORIENTATION_VERTICAL, 2);
      gtk_container_add (GTK_CONTAINER (frame), vbox2);
      gtk_widget_show (vbox2);

      /*  The color managed combo  */
      combo = gimp_prop_boolean_combo_box_new
        (G_OBJECT (tool_info->tool_options), "color-managed",
         _("Convert pixels to built-in sRGB to apply filter (slow)"),
         _("Assume pixels are built-in sRGB (ignore actual image color space)"));
      g_object_set (combo, "ellipsize", PANGO_ELLIPSIZE_END, NULL);
      gtk_box_pack_start (GTK_BOX (vbox2), combo, FALSE, FALSE, 0);
      gtk_widget_show (combo);

      /*  The gamma hack toggle  */
      toggle = gimp_prop_check_button_new (G_OBJECT (tool_info->tool_options),
                                           "gamma-hack", NULL);
      gtk_box_pack_start (GTK_BOX (vbox2), toggle, FALSE, FALSE, 0);
      gtk_widget_show (toggle);

      /*  The area combo  */
      filter_tool->region_combo =
        gimp_prop_enum_combo_box_new (G_OBJECT (tool_info->tool_options),
                                      "region",
                                      0, 0);
      gtk_box_pack_end (GTK_BOX (vbox), filter_tool->region_combo,
                        FALSE, FALSE, 0);

      /*  The clipping combo  */
      filter_tool->clip_combo =
        gimp_prop_enum_combo_box_new (G_OBJECT (tool_info->tool_options),
                                      "clip",
                                      GIMP_TRANSFORM_RESIZE_ADJUST,
                                      GIMP_TRANSFORM_RESIZE_CLIP);
      gimp_int_combo_box_set_label (
        GIMP_INT_COMBO_BOX (filter_tool->clip_combo), _("Clipping"));
      gtk_box_pack_end (GTK_BOX (vbox), filter_tool->clip_combo,
                        FALSE, FALSE, 0);

      /*  Fill in subclass widgets  */
      gimp_filter_tool_dialog (filter_tool);
    }
  else
    {
      gimp_tool_gui_set_title       (filter_tool->gui,
                                     gimp_tool_get_label (tool));
      gimp_tool_gui_set_description (filter_tool->gui, filter_tool->description);
      gimp_tool_gui_set_icon_name   (filter_tool->gui,
                                     gimp_tool_get_icon_name (tool));
      gimp_tool_gui_set_help_id     (filter_tool->gui,
                                     gimp_tool_get_help_id (tool));
    }

  gimp_tool_gui_set_shell (filter_tool->gui, shell);
  gimp_tool_gui_set_viewable (filter_tool->gui, GIMP_VIEWABLE (drawable));

  gimp_tool_gui_show (filter_tool->gui);

  g_signal_connect_object (drawable, "lock-position-changed",
                           G_CALLBACK (gimp_filter_tool_lock_position_changed),
                           filter_tool, 0);

  g_signal_connect_object (image, "mask-changed",
                           G_CALLBACK (gimp_filter_tool_mask_changed),
                           filter_tool, 0);

  gimp_filter_tool_mask_changed (image, filter_tool);

  gimp_filter_tool_create_filter (filter_tool);

  return TRUE;
}

static void
gimp_filter_tool_control (GimpTool       *tool,
                          GimpToolAction  action,
                          GimpDisplay    *display)
{
  GimpFilterTool *filter_tool = GIMP_FILTER_TOOL (tool);

  switch (action)
    {
    case GIMP_TOOL_ACTION_PAUSE:
    case GIMP_TOOL_ACTION_RESUME:
      break;

    case GIMP_TOOL_ACTION_HALT:
      gimp_filter_tool_halt (filter_tool);
      break;

    case GIMP_TOOL_ACTION_COMMIT:
      gimp_filter_tool_commit (filter_tool);
      break;
    }

  GIMP_TOOL_CLASS (parent_class)->control (tool, action, display);
}

static void
gimp_filter_tool_button_press (GimpTool            *tool,
                               const GimpCoords    *coords,
                               guint32              time,
                               GdkModifierType      state,
                               GimpButtonPressType  press_type,
                               GimpDisplay         *display)
{
  GimpFilterTool *filter_tool = GIMP_FILTER_TOOL (tool);

  if (gimp_filter_tool_on_guide (filter_tool, coords, display))
    {
      GimpFilterOptions *options = GIMP_FILTER_TOOL_GET_OPTIONS (tool);

      if (state & gimp_get_extend_selection_mask ())
        {
          gimp_filter_options_switch_preview_side (options);
        }
      else if (state & gimp_get_toggle_behavior_mask ())
        {
          GimpItem *item = GIMP_ITEM (tool->drawable);
          gdouble   pos_x;
          gdouble   pos_y;

          pos_x = ((coords->x - gimp_item_get_offset_x (item)) /
                   gimp_item_get_width (item));
          pos_y = ((coords->y - gimp_item_get_offset_y (item)) /
                   gimp_item_get_height (item));

          gimp_filter_options_switch_preview_orientation (options,
                                                          pos_x, pos_y);
        }
      else
        {
          gimp_guide_tool_start_edit (tool, display,
                                      filter_tool->preview_guide);
        }
    }
  else if (gimp_color_tool_is_enabled (GIMP_COLOR_TOOL (tool)))
    {
      GIMP_TOOL_CLASS (parent_class)->button_press (tool, coords, time, state,
                                                    press_type, display);
    }
  else if (filter_tool->widget)
    {
      if (gimp_tool_widget_button_press (filter_tool->widget, coords, time,
                                         state, press_type))
        {
          filter_tool->grab_widget = filter_tool->widget;

          gimp_tool_control_activate (tool->control);
        }
    }
}

static void
gimp_filter_tool_button_release (GimpTool              *tool,
                                 const GimpCoords      *coords,
                                 guint32                time,
                                 GdkModifierType        state,
                                 GimpButtonReleaseType  release_type,
                                 GimpDisplay           *display)
{
  GimpFilterTool *filter_tool = GIMP_FILTER_TOOL (tool);

  if (filter_tool->grab_widget)
    {
      gimp_tool_control_halt (tool->control);

      gimp_tool_widget_button_release (filter_tool->grab_widget,
                                       coords, time, state, release_type);
      filter_tool->grab_widget = NULL;
    }
  else
    {
      GIMP_TOOL_CLASS (parent_class)->button_release (tool, coords, time, state,
                                                      release_type, display);
    }
}

static void
gimp_filter_tool_motion (GimpTool         *tool,
                         const GimpCoords *coords,
                         guint32           time,
                         GdkModifierType   state,
                         GimpDisplay      *display)
{
  GimpFilterTool *filter_tool = GIMP_FILTER_TOOL (tool);

  if (filter_tool->grab_widget)
    {
      gimp_tool_widget_motion (filter_tool->grab_widget, coords, time, state);
    }
  else
    {
      GIMP_TOOL_CLASS (parent_class)->motion (tool, coords, time, state,
                                              display);
    }
}

static gboolean
gimp_filter_tool_key_press (GimpTool    *tool,
                            GdkEventKey *kevent,
                            GimpDisplay *display)
{
  GimpFilterTool *filter_tool = GIMP_FILTER_TOOL (tool);

  if (filter_tool->gui && display == tool->display)
    {
      switch (kevent->keyval)
        {
        case GDK_KEY_Return:
        case GDK_KEY_KP_Enter:
        case GDK_KEY_ISO_Enter:
          gimp_filter_tool_response (filter_tool->gui,
                                     GTK_RESPONSE_OK,
                                     filter_tool);
          return TRUE;

        case GDK_KEY_BackSpace:
          gimp_filter_tool_response (filter_tool->gui,
                                     RESPONSE_RESET,
                                     filter_tool);
          return TRUE;

        case GDK_KEY_Escape:
          gimp_filter_tool_response (filter_tool->gui,
                                     GTK_RESPONSE_CANCEL,
                                     filter_tool);
          return TRUE;
        }
    }

  return GIMP_TOOL_CLASS (parent_class)->key_press (tool, kevent, display);
}

static void
gimp_filter_tool_oper_update (GimpTool         *tool,
                              const GimpCoords *coords,
                              GdkModifierType   state,
                              gboolean          proximity,
                              GimpDisplay      *display)
{
  GimpFilterTool *filter_tool = GIMP_FILTER_TOOL (tool);

  gimp_tool_pop_status (tool, display);

  if (gimp_filter_tool_on_guide (filter_tool, coords, display))
    {
      GdkModifierType  extend_mask = gimp_get_extend_selection_mask ();
      GdkModifierType  toggle_mask = gimp_get_toggle_behavior_mask ();
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
          status = gimp_suggest_modifiers (_("Click to move the split guide"),
                                           (extend_mask | toggle_mask) & ~state,
                                           _("%s: switch original and filtered"),
                                           _("%s: switch horizontal and vertical"),
                                           NULL);
        }

      if (proximity)
        gimp_tool_push_status (tool, display, "%s", status);

      g_free (status);
    }
  else
    {
      GIMP_TOOL_CLASS (parent_class)->oper_update (tool, coords, state,
                                                   proximity, display);
    }
}

static void
gimp_filter_tool_cursor_update (GimpTool         *tool,
                                const GimpCoords *coords,
                                GdkModifierType   state,
                                GimpDisplay      *display)
{
  GimpFilterTool *filter_tool = GIMP_FILTER_TOOL (tool);

  if (gimp_filter_tool_on_guide (filter_tool, coords, display))
    {
      gimp_tool_set_cursor (tool, display,
                            GIMP_CURSOR_MOUSE,
                            GIMP_TOOL_CURSOR_HAND,
                            GIMP_CURSOR_MODIFIER_MOVE);
    }
  else
    {
      GIMP_TOOL_CLASS (parent_class)->cursor_update (tool, coords, state,
                                                     display);
    }
}

static void
gimp_filter_tool_options_notify (GimpTool         *tool,
                                 GimpToolOptions  *options,
                                 const GParamSpec *pspec)
{
  GimpFilterTool    *filter_tool    = GIMP_FILTER_TOOL (tool);
  GimpFilterOptions *filter_options = GIMP_FILTER_OPTIONS (options);

  if (! strcmp (pspec->name, "preview") &&
      filter_tool->filter)
    {
      if (filter_options->preview)
        {
          gimp_drawable_filter_apply (filter_tool->filter, NULL);

          if (filter_options->preview_split)
            gimp_filter_tool_add_guide (filter_tool);
        }
      else
        {
          gimp_drawable_filter_abort (filter_tool->filter);

          if (filter_options->preview_split)
            gimp_filter_tool_remove_guide (filter_tool);
        }
    }
  else if (! strcmp (pspec->name, "preview-split") &&
           filter_tool->filter)
    {
      if (filter_options->preview_split)
        {
          GimpDisplayShell *shell = gimp_display_get_shell (tool->display);
          GimpItem         *item  = GIMP_ITEM (tool->drawable);
          gint              x, y, width, height;

          gimp_display_shell_untransform_viewport (shell, TRUE,
                                                   &x, &y, &width, &height);

          if (gimp_rectangle_intersect (gimp_item_get_offset_x (item),
                                        gimp_item_get_offset_y (item),
                                        gimp_item_get_width  (item),
                                        gimp_item_get_height (item),
                                        x, y, width, height,
                                        &x, &y, &width, &height))
            {
              gdouble position;

              if (filter_options->preview_alignment == GIMP_ALIGN_LEFT ||
                  filter_options->preview_alignment == GIMP_ALIGN_RIGHT)
                {
                  position = ((gdouble) ((x + width / 2) -
                                         gimp_item_get_offset_x (item)) /
                              (gdouble) gimp_item_get_width (item));
                }
              else
                {
                  position = ((gdouble) ((y + height / 2) -
                                         gimp_item_get_offset_y (item)) /
                              (gdouble) gimp_item_get_height (item));
                }

              g_object_set (options,
                            "preview-position", CLAMP (position, 0.0, 1.0),
                            NULL);
            }
        }

      gimp_filter_tool_update_filter (filter_tool);

      if (filter_options->preview_split)
        gimp_filter_tool_add_guide (filter_tool);
      else
        gimp_filter_tool_remove_guide (filter_tool);
    }
  else if (! strcmp (pspec->name, "preview-alignment") ||
           ! strcmp (pspec->name, "preview-position"))
    {
      gimp_filter_tool_update_filter (filter_tool);

      if (filter_options->preview_split)
        gimp_filter_tool_move_guide (filter_tool);
    }
  else if (! strcmp (pspec->name, "clip")          ||
           ! strcmp (pspec->name, "region")        ||
           ! strcmp (pspec->name, "color-managed") ||
           ! strcmp (pspec->name, "gamma-hack"))
    {
      gimp_filter_tool_update_filter (filter_tool);
    }
  else if (! strcmp (pspec->name, "controller") &&
           filter_tool->widget)
    {
      gimp_tool_widget_set_visible (filter_tool->widget,
                                    filter_options->controller);
    }
}

static gboolean
gimp_filter_tool_can_pick_color (GimpColorTool    *color_tool,
                                 const GimpCoords *coords,
                                 GimpDisplay      *display)
{
  GimpTool       *tool        = GIMP_TOOL (color_tool);
  GimpFilterTool *filter_tool = GIMP_FILTER_TOOL (color_tool);
  GimpImage      *image       = gimp_display_get_image (display);

  if (gimp_image_get_active_drawable (image) != tool->drawable)
    return FALSE;

  return filter_tool->pick_abyss ||
         GIMP_COLOR_TOOL_CLASS (parent_class)->can_pick (color_tool,
                                                         coords, display);
}

static gboolean
gimp_filter_tool_pick_color (GimpColorTool     *color_tool,
                             const GimpCoords  *coords,
                             GimpDisplay       *display,
                             const Babl       **sample_format,
                             gpointer           pixel,
                             GimpRGB           *color)
{
  GimpTool       *tool        = GIMP_TOOL (color_tool);
  GimpFilterTool *filter_tool = GIMP_FILTER_TOOL (color_tool);
  gint            off_x, off_y;
  gboolean        picked;

  gimp_item_get_offset (GIMP_ITEM (tool->drawable), &off_x, &off_y);

  *sample_format = gimp_drawable_get_format (tool->drawable);

  picked = gimp_pickable_pick_color (GIMP_PICKABLE (tool->drawable),
                                     coords->x - off_x,
                                     coords->y - off_y,
                                     color_tool->options->sample_average,
                                     color_tool->options->average_radius,
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
gimp_filter_tool_color_picked (GimpColorTool      *color_tool,
                               const GimpCoords   *coords,
                               GimpDisplay        *display,
                               GimpColorPickState  pick_state,
                               const Babl         *sample_format,
                               gpointer            pixel,
                               const GimpRGB      *color)
{
  GimpFilterTool *filter_tool = GIMP_FILTER_TOOL (color_tool);

  if (filter_tool->active_picker)
    {
      GimpPickerCallback callback;
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

  GIMP_FILTER_TOOL_GET_CLASS (filter_tool)->color_picked (filter_tool,
                                                          filter_tool->pick_identifier,
                                                          coords->x,
                                                          coords->y,
                                                          sample_format, color);
}

static void
gimp_filter_tool_real_reset (GimpFilterTool *filter_tool)
{
  if (filter_tool->config)
    {
      if (filter_tool->default_config)
        {
          gimp_config_copy (GIMP_CONFIG (filter_tool->default_config),
                            GIMP_CONFIG (filter_tool->config),
                            0);
        }
      else
        {
          gimp_config_reset (GIMP_CONFIG (filter_tool->config));
        }
    }
}

static void
gimp_filter_tool_real_set_config (GimpFilterTool *filter_tool,
                                  GimpConfig     *config)
{
  gimp_config_copy (GIMP_CONFIG (config),
                    GIMP_CONFIG (filter_tool->config), 0);

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
gimp_filter_tool_real_config_notify (GimpFilterTool   *filter_tool,
                                     GimpConfig       *config,
                                     const GParamSpec *pspec)
{
  GimpFilterOptions *options = GIMP_FILTER_TOOL_GET_OPTIONS (filter_tool);

  if (filter_tool->filter && options->preview)
    gimp_drawable_filter_apply (filter_tool->filter, NULL);
}

static void
gimp_filter_tool_halt (GimpFilterTool *filter_tool)
{
  GimpTool *tool = GIMP_TOOL (filter_tool);

  gimp_filter_tool_disable_color_picking (filter_tool);

  if (tool->display)
    {
      GimpImage *image = gimp_display_get_image (tool->display);

      g_signal_handlers_disconnect_by_func (tool->drawable,
                                            gimp_filter_tool_lock_position_changed,
                                            filter_tool);

      g_signal_handlers_disconnect_by_func (image,
                                            gimp_filter_tool_mask_changed,
                                            filter_tool);
    }

  if (filter_tool->gui)
    {
      /* explicitly clear the dialog contents first, since we might be called
       * during the dialog's delete event, in which case the dialog will be
       * externally reffed, and will only die *after* gimp_filter_tool_halt()
       * returns, and, in particular, after filter_tool->config has been
       * cleared.  we want to make sure the gui is destroyed while
       * filter_tool->config is still alive, since the gui's destruction may
       * fire signals whose handlers rely on it.
       */
      gimp_gtk_container_clear (
        GTK_CONTAINER (gimp_filter_tool_dialog_get_vbox (filter_tool)));

      g_clear_object (&filter_tool->gui);
      filter_tool->settings_box      = NULL;
      filter_tool->controller_toggle = NULL;
      filter_tool->clip_combo        = NULL;
      filter_tool->region_combo      = NULL;
    }

  if (filter_tool->filter)
    {
      gimp_drawable_filter_abort (filter_tool->filter);
      g_clear_object (&filter_tool->filter);

      gimp_filter_tool_remove_guide (filter_tool);
    }

  g_clear_object (&filter_tool->operation);

  if (filter_tool->config)
    {
      g_signal_handlers_disconnect_by_func (filter_tool->config,
                                            gimp_filter_tool_config_notify,
                                            filter_tool);
      g_signal_handlers_disconnect_by_func (filter_tool->config,
                                            gimp_filter_tool_unset_setting,
                                            filter_tool);
      g_clear_object (&filter_tool->config);
    }

  g_clear_object (&filter_tool->default_config);
  g_clear_object (&filter_tool->settings);

  if (gimp_draw_tool_is_active (GIMP_DRAW_TOOL (tool)))
    gimp_draw_tool_stop (GIMP_DRAW_TOOL (tool));

  gimp_filter_tool_set_widget (filter_tool, NULL);

  tool->display  = NULL;
  tool->drawable = NULL;
}

static void
gimp_filter_tool_commit (GimpFilterTool *filter_tool)
{
  GimpTool *tool = GIMP_TOOL (filter_tool);

  if (filter_tool->gui)
    gimp_tool_gui_hide (filter_tool->gui);

  if (filter_tool->filter)
    {
      GimpFilterOptions *options = GIMP_FILTER_TOOL_GET_OPTIONS (tool);

      if (! options->preview)
        gimp_drawable_filter_apply (filter_tool->filter, NULL);

      gimp_tool_control_push_preserve (tool->control, TRUE);

      gimp_drawable_filter_commit (filter_tool->filter,
                                   GIMP_PROGRESS (tool), TRUE);
      g_clear_object (&filter_tool->filter);

      gimp_tool_control_pop_preserve (tool->control);

      gimp_filter_tool_remove_guide (filter_tool);

      gimp_image_flush (gimp_display_get_image (tool->display));

      if (filter_tool->config && filter_tool->has_settings)
        {
          GimpGuiConfig *config = GIMP_GUI_CONFIG (tool->tool_info->gimp->config);

          gimp_settings_box_add_current (GIMP_SETTINGS_BOX (filter_tool->settings_box),
                                         config->filter_tool_max_recent);
        }
    }
}

static void
gimp_filter_tool_dialog (GimpFilterTool *filter_tool)
{
  GIMP_FILTER_TOOL_GET_CLASS (filter_tool)->dialog (filter_tool);
}

static void
gimp_filter_tool_reset (GimpFilterTool *filter_tool)
{
  if (filter_tool->config)
    g_object_freeze_notify (filter_tool->config);

  GIMP_FILTER_TOOL_GET_CLASS (filter_tool)->reset (filter_tool);

  if (filter_tool->config)
    g_object_thaw_notify (filter_tool->config);

  if (filter_tool->widget)
    gimp_filter_tool_reset_widget (filter_tool, filter_tool->widget);
}

static void
gimp_filter_tool_create_filter (GimpFilterTool *filter_tool)
{
  GimpTool          *tool    = GIMP_TOOL (filter_tool);
  GimpFilterOptions *options = GIMP_FILTER_TOOL_GET_OPTIONS (filter_tool);

  if (filter_tool->filter)
    {
      gimp_drawable_filter_abort (filter_tool->filter);
      g_object_unref (filter_tool->filter);
    }

  gimp_assert (filter_tool->operation);

  filter_tool->filter = gimp_drawable_filter_new (tool->drawable,
                                                  gimp_tool_get_undo_desc (tool),
                                                  filter_tool->operation,
                                                  gimp_tool_get_icon_name (tool));

  gimp_filter_tool_update_filter (filter_tool);

  g_signal_connect (filter_tool->filter, "flush",
                    G_CALLBACK (gimp_filter_tool_flush),
                    filter_tool);

  gimp_gegl_progress_connect (filter_tool->operation,
                              GIMP_PROGRESS (filter_tool),
                              gimp_tool_get_undo_desc (tool));

  if (options->preview)
    gimp_drawable_filter_apply (filter_tool->filter, NULL);
}

static void
gimp_filter_tool_update_dialog (GimpFilterTool *filter_tool)
{
  GimpTool *tool = GIMP_TOOL (filter_tool);

  if (filter_tool->gui)
    {
      GimpImage   *image = gimp_display_get_image (tool->display);
      GimpChannel *mask  = gimp_image_get_mask (image);

      if (gimp_channel_is_empty (mask))
        {
          gtk_widget_set_visible (
            filter_tool->clip_combo,
            gimp_item_get_clip (GIMP_ITEM (tool->drawable), FALSE) == FALSE &&
            ! gimp_gegl_node_is_point_operation (filter_tool->operation)    &&
            gimp_drawable_has_alpha (tool->drawable));

          gtk_widget_hide (filter_tool->region_combo);
        }
      else
        {
          gtk_widget_hide (filter_tool->clip_combo);

          gtk_widget_set_visible (
            filter_tool->region_combo,
            ! gimp_gegl_node_is_point_operation (filter_tool->operation) ||
            gimp_gegl_node_get_key (filter_tool->operation,
                                    "position-dependent"));
        }
    }
}

static void
gimp_filter_tool_region_changed (GimpFilterTool *filter_tool)
{
  if (filter_tool->filter &&
      GIMP_FILTER_TOOL_GET_CLASS (filter_tool)->region_changed)
    {
      GIMP_FILTER_TOOL_GET_CLASS (filter_tool)->region_changed (filter_tool);
    }
}

static void
gimp_filter_tool_flush (GimpDrawableFilter *filter,
                        GimpFilterTool     *filter_tool)
{
  GimpTool  *tool  = GIMP_TOOL (filter_tool);
  GimpImage *image = gimp_display_get_image (tool->display);

  gimp_projection_flush (gimp_image_get_projection (image));
}

static void
gimp_filter_tool_config_notify (GObject          *object,
                                const GParamSpec *pspec,
                                GimpFilterTool   *filter_tool)
{
  GIMP_FILTER_TOOL_GET_CLASS (filter_tool)->config_notify (filter_tool,
                                                           GIMP_CONFIG (object),
                                                           pspec);
}

static void
gimp_filter_tool_unset_setting (GObject          *object,
                                const GParamSpec *pspec,
                                GimpFilterTool   *filter_tool)
{
  g_signal_handlers_disconnect_by_func (filter_tool->config,
                                        gimp_filter_tool_unset_setting,
                                        filter_tool);

  gimp_settings_box_unset (GIMP_SETTINGS_BOX (filter_tool->settings_box));
}

static void
gimp_filter_tool_lock_position_changed (GimpDrawable   *drawable,
                                        GimpFilterTool *filter_tool)
{
  gimp_filter_tool_update_dialog (filter_tool);
}

static void
gimp_filter_tool_mask_changed (GimpImage      *image,
                               GimpFilterTool *filter_tool)
{
  GimpFilterOptions *options = GIMP_FILTER_TOOL_GET_OPTIONS (filter_tool);

  gimp_filter_tool_update_dialog (filter_tool);

  if (options->region == GIMP_FILTER_REGION_SELECTION)
    gimp_filter_tool_region_changed (filter_tool);
}

static void
gimp_filter_tool_add_guide (GimpFilterTool *filter_tool)
{
  GimpTool            *tool    = GIMP_TOOL (filter_tool);
  GimpFilterOptions   *options = GIMP_FILTER_TOOL_GET_OPTIONS (filter_tool);
  GimpItem            *item;
  GimpImage           *image;
  GimpOrientationType  orientation;
  gint                 position;

  if (filter_tool->preview_guide)
    return;

  item  = GIMP_ITEM (tool->drawable);
  image = gimp_item_get_image (item);

  if (options->preview_alignment == GIMP_ALIGN_LEFT ||
      options->preview_alignment == GIMP_ALIGN_RIGHT)
    {
      orientation = GIMP_ORIENTATION_VERTICAL;

      position = (gimp_item_get_offset_x (item) +
                  gimp_item_get_width (item) *
                  options->preview_position);
    }
  else
    {
      orientation = GIMP_ORIENTATION_HORIZONTAL;

      position = (gimp_item_get_offset_y (item) +
                  gimp_item_get_height (item) *
                  options->preview_position);
    }

  filter_tool->preview_guide =
    gimp_guide_custom_new (orientation,
                           image->gimp->next_guide_ID++,
                           GIMP_GUIDE_STYLE_SPLIT_VIEW);

  gimp_image_add_guide (image, filter_tool->preview_guide, position);

  g_signal_connect (filter_tool->preview_guide, "removed",
                    G_CALLBACK (gimp_filter_tool_guide_removed),
                    filter_tool);
  g_signal_connect (filter_tool->preview_guide, "notify::position",
                    G_CALLBACK (gimp_filter_tool_guide_moved),
                    filter_tool);
}

static void
gimp_filter_tool_remove_guide (GimpFilterTool *filter_tool)
{
  GimpTool  *tool = GIMP_TOOL (filter_tool);
  GimpImage *image;

  if (! filter_tool->preview_guide)
    return;

  image = gimp_item_get_image (GIMP_ITEM (tool->drawable));

  gimp_image_remove_guide (image, filter_tool->preview_guide, FALSE);
}

static void
gimp_filter_tool_move_guide (GimpFilterTool *filter_tool)
{
  GimpTool            *tool    = GIMP_TOOL (filter_tool);
  GimpFilterOptions   *options = GIMP_FILTER_TOOL_GET_OPTIONS (filter_tool);
  GimpItem            *item;
  GimpOrientationType  orientation;
  gint                 position;

  if (! filter_tool->preview_guide)
    return;

  item = GIMP_ITEM (tool->drawable);

  if (options->preview_alignment == GIMP_ALIGN_LEFT ||
      options->preview_alignment == GIMP_ALIGN_RIGHT)
    {
      orientation = GIMP_ORIENTATION_VERTICAL;

      position = (gimp_item_get_offset_x (item) +
                  gimp_item_get_width (item) *
                  options->preview_position);
    }
  else
    {
      orientation = GIMP_ORIENTATION_HORIZONTAL;

      position = (gimp_item_get_offset_y (item) +
                  gimp_item_get_height (item) *
                  options->preview_position);
    }

  if (orientation != gimp_guide_get_orientation (filter_tool->preview_guide) ||
      position    != gimp_guide_get_position (filter_tool->preview_guide))
    {
      gimp_guide_set_orientation (filter_tool->preview_guide, orientation);
      gimp_image_move_guide (gimp_item_get_image (item),
                             filter_tool->preview_guide, position, FALSE);
    }
}

static void
gimp_filter_tool_guide_removed (GimpGuide      *guide,
                                GimpFilterTool *filter_tool)
{
  GimpFilterOptions *options = GIMP_FILTER_TOOL_GET_OPTIONS (filter_tool);

  g_signal_handlers_disconnect_by_func (G_OBJECT (filter_tool->preview_guide),
                                        gimp_filter_tool_guide_removed,
                                        filter_tool);
  g_signal_handlers_disconnect_by_func (G_OBJECT (filter_tool->preview_guide),
                                        gimp_filter_tool_guide_moved,
                                        filter_tool);

  g_clear_object (&filter_tool->preview_guide);

  g_object_set (options,
                "preview-split", FALSE,
                NULL);
}

static void
gimp_filter_tool_guide_moved (GimpGuide        *guide,
                              const GParamSpec *pspec,
                              GimpFilterTool   *filter_tool)
{
  GimpTool          *tool    = GIMP_TOOL (filter_tool);
  GimpFilterOptions *options = GIMP_FILTER_TOOL_GET_OPTIONS (filter_tool);
  GimpItem          *item    = GIMP_ITEM (tool->drawable);
  gdouble            position;

  if (options->preview_alignment == GIMP_ALIGN_LEFT ||
      options->preview_alignment == GIMP_ALIGN_RIGHT)
    {
      position = ((gdouble) (gimp_guide_get_position (guide) -
                             gimp_item_get_offset_x (item)) /
                  (gdouble) gimp_item_get_width (item));
    }
  else
    {
      position = ((gdouble) (gimp_guide_get_position (guide) -
                             gimp_item_get_offset_y (item)) /
                  (gdouble) gimp_item_get_height (item));
    }

  g_object_set (options,
                "preview-position", CLAMP (position, 0.0, 1.0),
                NULL);
}

static void
gimp_filter_tool_response (GimpToolGui    *gui,
                           gint            response_id,
                           GimpFilterTool *filter_tool)
{
  GimpTool *tool = GIMP_TOOL (filter_tool);

  switch (response_id)
    {
    case RESPONSE_RESET:
      gimp_filter_tool_reset (filter_tool);
      break;

    case GTK_RESPONSE_OK:
      gimp_tool_control (tool, GIMP_TOOL_ACTION_COMMIT, tool->display);
      break;

    default:
      gimp_tool_control (tool, GIMP_TOOL_ACTION_HALT, tool->display);
      break;
    }
}

static void
gimp_filter_tool_update_filter (GimpFilterTool *filter_tool)
{
  GimpTool          *tool    = GIMP_TOOL (filter_tool);
  GimpFilterOptions *options = GIMP_FILTER_TOOL_GET_OPTIONS (filter_tool);
  gboolean           add_alpha;
  gboolean           clip;

  if (! filter_tool->filter)
    return;

  add_alpha = gimp_drawable_supports_alpha (tool->drawable) &&
              gimp_gegl_node_get_key (filter_tool->operation, "needs-alpha");
  clip      = options->clip == GIMP_TRANSFORM_RESIZE_CLIP ||
              ! (gimp_drawable_has_alpha (tool->drawable) ||
                 add_alpha);

  gimp_drawable_filter_set_clip          (filter_tool->filter,
                                          clip);
  gimp_drawable_filter_set_region        (filter_tool->filter,
                                          options->region);
  gimp_drawable_filter_set_preview       (filter_tool->filter,
                                          options->preview_split,
                                          options->preview_alignment,
                                          options->preview_position);
  gimp_drawable_filter_set_add_alpha     (filter_tool->filter,
                                          add_alpha);
  gimp_drawable_filter_set_color_managed (filter_tool->filter,
                                          options->color_managed);
  gimp_drawable_filter_set_gamma_hack    (filter_tool->filter,
                                          options->gamma_hack);
}

static void
gimp_filter_tool_set_has_settings (GimpFilterTool *filter_tool,
                                   gboolean        has_settings)
{
  g_return_if_fail (GIMP_IS_FILTER_TOOL (filter_tool));

  filter_tool->has_settings = has_settings;

  if (! filter_tool->settings_box)
    return;

  if (filter_tool->has_settings)
    {
      GimpTool *tool  = GIMP_TOOL (filter_tool);
      GQuark    quark = g_quark_from_static_string ("settings-folder");
      GType     type  = G_TYPE_FROM_INSTANCE (filter_tool->config);
      GFile    *settings_folder;
      gchar    *import_title;
      gchar    *export_title;

      settings_folder = g_type_get_qdata (type, quark);

      import_title = g_strdup_printf (_("Import '%s' Settings"),
                                      gimp_tool_get_label (tool));
      export_title = g_strdup_printf (_("Export '%s' Settings"),
                                      gimp_tool_get_label (tool));

      g_object_set (filter_tool->settings_box,
                    "visible",        TRUE,
                    "config",         filter_tool->config,
                    "container",      filter_tool->settings,
                    "help-id",        gimp_tool_get_help_id (tool),
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
gimp_filter_tool_get_operation (GimpFilterTool *filter_tool)
{
  GimpTool             *tool;
  GimpFilterToolClass  *klass;
  gchar                *operation_name;
  GParamSpec          **pspecs;

  g_return_if_fail (GIMP_IS_FILTER_TOOL (filter_tool));

  tool  = GIMP_TOOL (filter_tool);
  klass = GIMP_FILTER_TOOL_GET_CLASS (filter_tool);

  if (filter_tool->filter)
    {
      gimp_drawable_filter_abort (filter_tool->filter);
      g_clear_object (&filter_tool->filter);

      gimp_filter_tool_remove_guide (filter_tool);
    }

  g_clear_object (&filter_tool->operation);

  if (filter_tool->config)
    {
      g_signal_handlers_disconnect_by_func (filter_tool->config,
                                            gimp_filter_tool_config_notify,
                                            filter_tool);
      g_signal_handlers_disconnect_by_func (filter_tool->config,
                                            gimp_filter_tool_unset_setting,
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
    filter_tool->description = g_strdup (gimp_tool_get_label (tool));

  filter_tool->operation = gegl_node_new_child (NULL,
                                                "operation", operation_name,
                                                NULL);

  filter_tool->config =
    g_object_new (gimp_operation_config_get_type (tool->tool_info->gimp,
                                                  operation_name,
                                                  gimp_tool_get_icon_name (tool),
                                                  GIMP_TYPE_SETTINGS),
                  NULL);

  gimp_operation_config_sync_node (filter_tool->config,
                                   filter_tool->operation);
  gimp_operation_config_connect_node (filter_tool->config,
                                      filter_tool->operation);

  filter_tool->settings =
    gimp_operation_config_get_container (tool->tool_info->gimp,
                                         G_TYPE_FROM_INSTANCE (filter_tool->config),
                                         (GCompareFunc) gimp_settings_compare);
  g_object_ref (filter_tool->settings);

  pspecs =
    gimp_operation_config_list_properties (filter_tool->config,
                                           G_TYPE_FROM_INSTANCE (filter_tool->config),
                                           0, NULL);

  gimp_filter_tool_set_has_settings (filter_tool, (pspecs != NULL));

  g_free (pspecs);

  if (filter_tool->gui)
    {
      gimp_tool_gui_set_title       (filter_tool->gui,
                                     gimp_tool_get_label (tool));
      gimp_tool_gui_set_description (filter_tool->gui, filter_tool->description);
      gimp_tool_gui_set_icon_name   (filter_tool->gui,
                                     gimp_tool_get_icon_name (tool));
      gimp_tool_gui_set_help_id     (filter_tool->gui,
                                     gimp_tool_get_help_id (tool));
    }

  if (gimp_gegl_node_is_point_operation (filter_tool->operation) &&
      ! gimp_gegl_node_get_key (filter_tool->operation, "position-dependent"))
    {
      g_object_set (GIMP_FILTER_TOOL_GET_OPTIONS (filter_tool),
                    "region", GIMP_FILTER_REGION_SELECTION,
                    NULL);
    }

  gimp_filter_tool_update_dialog (filter_tool);

  g_free (operation_name);

  g_object_set (GIMP_FILTER_TOOL_GET_OPTIONS (filter_tool),
                "preview-split",    FALSE,
                "preview-position", 0.5,
                NULL);

  g_signal_connect_object (filter_tool->config, "notify",
                           G_CALLBACK (gimp_filter_tool_config_notify),
                           G_OBJECT (filter_tool), 0);

  if (tool->drawable)
    gimp_filter_tool_create_filter (filter_tool);
}

void
gimp_filter_tool_set_config (GimpFilterTool *filter_tool,
                             GimpConfig     *config)
{
  g_return_if_fail (GIMP_IS_FILTER_TOOL (filter_tool));
  g_return_if_fail (GIMP_IS_SETTINGS (config));

  /* if the user didn't change a setting since the last set_config(),
   * this handler is still connected
   */
  g_signal_handlers_disconnect_by_func (filter_tool->config,
                                        gimp_filter_tool_unset_setting,
                                        filter_tool);

  GIMP_FILTER_TOOL_GET_CLASS (filter_tool)->set_config (filter_tool, config);

  if (filter_tool->widget)
    gimp_filter_tool_reset_widget (filter_tool, filter_tool->widget);

  if (filter_tool->settings_box)
    g_signal_connect_object (filter_tool->config, "notify",
                             G_CALLBACK (gimp_filter_tool_unset_setting),
                             G_OBJECT (filter_tool), 0);
}

void
gimp_filter_tool_edit_as (GimpFilterTool *filter_tool,
                          const gchar    *new_tool_id,
                          GimpConfig     *config)
{
  GimpDisplay  *display;
  GimpContext  *user_context;
  GimpToolInfo *tool_info;
  GimpTool     *new_tool;

  g_return_if_fail (GIMP_IS_FILTER_TOOL (filter_tool));
  g_return_if_fail (new_tool_id != NULL);
  g_return_if_fail (GIMP_IS_CONFIG (config));

  display = GIMP_TOOL (filter_tool)->display;

  user_context = gimp_get_user_context (display->gimp);

  tool_info = (GimpToolInfo *)
    gimp_container_get_child_by_name (display->gimp->tool_info_list,
                                      new_tool_id);

  gimp_tool_control (GIMP_TOOL (filter_tool), GIMP_TOOL_ACTION_HALT, display);
  gimp_context_set_tool (user_context, tool_info);
  tool_manager_initialize_active (display->gimp, display);

  new_tool = tool_manager_get_active (display->gimp);

  GIMP_FILTER_TOOL (new_tool)->default_config = g_object_ref (G_OBJECT (config));

  gimp_filter_tool_reset (GIMP_FILTER_TOOL (new_tool));
}

gboolean
gimp_filter_tool_on_guide (GimpFilterTool   *filter_tool,
                           const GimpCoords *coords,
                           GimpDisplay      *display)
{
  GimpDisplayShell *shell;

  g_return_val_if_fail (GIMP_IS_FILTER_TOOL (filter_tool), FALSE);
  g_return_val_if_fail (coords != NULL, FALSE);
  g_return_val_if_fail (GIMP_IS_DISPLAY (display), FALSE);

  shell = gimp_display_get_shell (display);

  if (filter_tool->filter        &&
      filter_tool->preview_guide &&
      gimp_display_shell_get_show_guides (shell))
    {
      const gint          snap_distance = display->config->snap_distance;
      GimpOrientationType orientation;
      gint                position;

      orientation = gimp_guide_get_orientation (filter_tool->preview_guide);
      position    = gimp_guide_get_position (filter_tool->preview_guide);

      if (orientation == GIMP_ORIENTATION_HORIZONTAL)
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
gimp_filter_tool_dialog_get_vbox (GimpFilterTool *filter_tool)
{
  g_return_val_if_fail (GIMP_IS_FILTER_TOOL (filter_tool), NULL);

  return gimp_tool_gui_get_vbox (filter_tool->gui);
}

void
gimp_filter_tool_enable_color_picking (GimpFilterTool *filter_tool,
                                       gpointer        identifier,
                                       gboolean        pick_abyss)
{
  g_return_if_fail (GIMP_IS_FILTER_TOOL (filter_tool));

  gimp_filter_tool_disable_color_picking (filter_tool);

  /* note that ownership over 'identifier' is not transferred, and its
   * lifetime should be managed by the caller.
   */
  filter_tool->pick_identifier = identifier;
  filter_tool->pick_abyss      = pick_abyss;

  gimp_color_tool_enable (GIMP_COLOR_TOOL (filter_tool),
                          GIMP_COLOR_TOOL_GET_OPTIONS (filter_tool));
}

void
gimp_filter_tool_disable_color_picking (GimpFilterTool *filter_tool)
{
  g_return_if_fail (GIMP_IS_FILTER_TOOL (filter_tool));

  if (filter_tool->active_picker)
    {
      GtkToggleButton *toggle = GTK_TOGGLE_BUTTON (filter_tool->active_picker);

      filter_tool->active_picker = NULL;

      gtk_toggle_button_set_active (toggle, FALSE);
    }

  if (gimp_color_tool_is_enabled (GIMP_COLOR_TOOL (filter_tool)))
    gimp_color_tool_disable (GIMP_COLOR_TOOL (filter_tool));
}

static void
gimp_filter_tool_color_picker_toggled (GtkWidget      *widget,
                                       GimpFilterTool *filter_tool)
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

      gimp_filter_tool_enable_color_picking (filter_tool,
                                             identifier, pick_abyss);

      filter_tool->active_picker = widget;
    }
  else if (filter_tool->active_picker == widget)
    {
      gimp_filter_tool_disable_color_picking (filter_tool);
    }
}

GtkWidget *
gimp_filter_tool_add_color_picker (GimpFilterTool     *filter_tool,
                                   gpointer            identifier,
                                   const gchar        *icon_name,
                                   const gchar        *tooltip,
                                   gboolean            pick_abyss,
                                   GimpPickerCallback  callback,
                                   gpointer            callback_data)
{
  GtkWidget *button;
  GtkWidget *image;

  g_return_val_if_fail (GIMP_IS_FILTER_TOOL (filter_tool), NULL);
  g_return_val_if_fail (icon_name != NULL, NULL);

  button = g_object_new (GTK_TYPE_TOGGLE_BUTTON,
                         "draw-indicator", FALSE,
                         NULL);

  image = gtk_image_new_from_icon_name (icon_name, GTK_ICON_SIZE_BUTTON);
  gtk_misc_set_padding (GTK_MISC (image), 2, 2);
  gtk_container_add (GTK_CONTAINER (button), image);
  gtk_widget_show (image);

  if (tooltip)
    gimp_help_set_help_data (button, tooltip, NULL);

  g_object_set_data (G_OBJECT (button),
                     "picker-identifier", identifier);
  g_object_set_data (G_OBJECT (button),
                     "picker-pick-abyss", GINT_TO_POINTER (pick_abyss));
  g_object_set_data (G_OBJECT (button),
                     "picker-callback", callback);
  g_object_set_data (G_OBJECT (button),
                     "picker-callback-data", callback_data);

  g_signal_connect (button, "toggled",
                    G_CALLBACK (gimp_filter_tool_color_picker_toggled),
                    filter_tool);

  return button;
}

GCallback
gimp_filter_tool_add_controller (GimpFilterTool     *filter_tool,
                                 GimpControllerType  controller_type,
                                 const gchar        *status_title,
                                 GCallback           callback,
                                 gpointer            callback_data,
                                 gpointer           *set_func_data)
{
  GimpToolWidget *widget;
  GCallback       set_func;

  g_return_val_if_fail (GIMP_IS_FILTER_TOOL (filter_tool), NULL);
  g_return_val_if_fail (callback != NULL, NULL);
  g_return_val_if_fail (callback_data != NULL, NULL);
  g_return_val_if_fail (set_func_data != NULL, NULL);

  widget = gimp_filter_tool_create_widget (filter_tool,
                                           controller_type,
                                           status_title,
                                           callback,
                                           callback_data,
                                           &set_func,
                                           set_func_data);
  gimp_filter_tool_set_widget (filter_tool, widget);
  g_object_unref (widget);

  return set_func;
}

void
gimp_filter_tool_set_widget (GimpFilterTool *filter_tool,
                             GimpToolWidget *widget)
{
  g_return_if_fail (GIMP_IS_FILTER_TOOL (filter_tool));
  g_return_if_fail (widget == NULL || GIMP_IS_TOOL_WIDGET (widget));

  if (widget == filter_tool->widget)
    return;

  if (filter_tool->widget)
    {
      if (gimp_draw_tool_is_active (GIMP_DRAW_TOOL (filter_tool)))
        gimp_draw_tool_stop (GIMP_DRAW_TOOL (filter_tool));

      g_object_unref (filter_tool->widget);
    }

  filter_tool->widget = widget;
  gimp_draw_tool_set_widget (GIMP_DRAW_TOOL (filter_tool), widget);

  if (filter_tool->widget)
    {
      GimpFilterOptions *options = GIMP_FILTER_TOOL_GET_OPTIONS (filter_tool);

      g_object_ref (filter_tool->widget);

      gimp_tool_widget_set_visible (filter_tool->widget,
                                    options->controller);

      if (GIMP_TOOL (filter_tool)->display)
        gimp_draw_tool_start (GIMP_DRAW_TOOL (filter_tool),
                              GIMP_TOOL (filter_tool)->display);
    }

  if (filter_tool->controller_toggle)
    {
      gtk_widget_set_visible (filter_tool->controller_toggle,
                              filter_tool->widget != NULL);
    }
}

gboolean
gimp_filter_tool_get_drawable_area (GimpFilterTool *filter_tool,
                                    gint           *drawable_offset_x,
                                    gint           *drawable_offset_y,
                                    GeglRectangle  *drawable_area)
{
  GimpTool     *tool;
  GimpDrawable *drawable;

  g_return_val_if_fail (GIMP_IS_FILTER_TOOL (filter_tool), FALSE);
  g_return_val_if_fail (drawable_offset_x != NULL, FALSE);
  g_return_val_if_fail (drawable_offset_y != NULL, FALSE);
  g_return_val_if_fail (drawable_area != NULL, FALSE);

  tool = GIMP_TOOL (filter_tool);

  *drawable_offset_x = 0;
  *drawable_offset_y = 0;

  drawable_area->x      = 0;
  drawable_area->y      = 0;
  drawable_area->width  = 1;
  drawable_area->height = 1;

  drawable = tool->drawable;

  if (drawable)
    {
      GimpFilterOptions *options = GIMP_FILTER_TOOL_GET_OPTIONS (filter_tool);

      gimp_item_get_offset (GIMP_ITEM (drawable),
                            drawable_offset_x, drawable_offset_y);

      switch (options->region)
        {
        case GIMP_FILTER_REGION_SELECTION:
          if (! gimp_item_mask_intersect (GIMP_ITEM (drawable),
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

        case GIMP_FILTER_REGION_DRAWABLE:
          drawable_area->width  = gimp_item_get_width  (GIMP_ITEM (drawable));
          drawable_area->height = gimp_item_get_height (GIMP_ITEM (drawable));
          break;
        }

      return TRUE;
    }

  return FALSE;
}
