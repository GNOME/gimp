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
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

/* This file contains a base class for tools that implement on canvas
 * preview for non destructive editing. The processing of the pixels can
 * be done either by a gegl op or by a C function (apply_func).
 *
 * For the core side of this, please see /app/core/gimpimagemap.c.
 */

#include "config.h"

#include <string.h>

#include <gegl.h>
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
#include "core/gimpdrawable.h"
#include "core/gimpdrawablefilter.h"
#include "core/gimperror.h"
#include "core/gimpguide.h"
#include "core/gimpimage.h"
#include "core/gimpimage-guides.h"
#include "core/gimpimage-pick-color.h"
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

#include "gimpfilteroptions.h"
#include "gimpfiltertool.h"
#include "gimpfiltertool-settings.h"
#include "gimpguidetool.h"
#include "gimptoolcontrol.h"
#include "tool_manager.h"

#include "gimp-intl.h"


/*  local function prototypes  */

static void      gimp_filter_tool_constructed    (GObject             *object);
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

static gboolean  gimp_filter_tool_pick_color     (GimpColorTool       *color_tool,
                                                  gint                 x,
                                                  gint                 y,
                                                  const Babl         **sample_format,
                                                  gpointer             pixel,
                                                  GimpRGB             *color);
static void      gimp_filter_tool_color_picked   (GimpColorTool       *color_tool,
                                                  GimpColorPickState   pick_state,
                                                  gdouble              x,
                                                  gdouble              y,
                                                  const Babl          *sample_format,
                                                  gpointer             pixel,
                                                  const GimpRGB       *color);

static void      gimp_filter_tool_real_reset     (GimpFilterTool      *filter_tool);

static void      gimp_filter_tool_halt           (GimpFilterTool      *filter_tool);
static void      gimp_filter_tool_commit         (GimpFilterTool      *filter_tool);

static void      gimp_filter_tool_dialog         (GimpFilterTool      *filter_tool);
static void      gimp_filter_tool_dialog_unmap   (GtkWidget           *dialog,
                                                  GimpFilterTool      *filter_tool);
static void      gimp_filter_tool_reset          (GimpFilterTool      *filter_tool);
static void      gimp_filter_tool_create_map     (GimpFilterTool      *filter_tool);

static void      gimp_filter_tool_flush          (GimpDrawableFilter  *filter,
                                                  GimpFilterTool      *filter_tool);
static void      gimp_filter_tool_config_notify  (GObject             *object,
                                                  const GParamSpec    *pspec,
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


G_DEFINE_TYPE (GimpFilterTool, gimp_filter_tool, GIMP_TYPE_COLOR_TOOL)

#define parent_class gimp_filter_tool_parent_class


static void
gimp_filter_tool_class_init (GimpFilterToolClass *klass)
{
  GObjectClass       *object_class     = G_OBJECT_CLASS (klass);
  GimpToolClass      *tool_class       = GIMP_TOOL_CLASS (klass);
  GimpColorToolClass *color_tool_class = GIMP_COLOR_TOOL_CLASS (klass);

  object_class->constructed  = gimp_filter_tool_constructed;
  object_class->finalize     = gimp_filter_tool_finalize;

  tool_class->initialize     = gimp_filter_tool_initialize;
  tool_class->control        = gimp_filter_tool_control;
  tool_class->button_press   = gimp_filter_tool_button_press;
  tool_class->key_press      = gimp_filter_tool_key_press;
  tool_class->oper_update    = gimp_filter_tool_oper_update;
  tool_class->cursor_update  = gimp_filter_tool_cursor_update;
  tool_class->options_notify = gimp_filter_tool_options_notify;

  color_tool_class->pick     = gimp_filter_tool_pick_color;
  color_tool_class->picked   = gimp_filter_tool_color_picked;

  klass->settings_name       = NULL;
  klass->import_dialog_title = NULL;
  klass->export_dialog_title = NULL;

  klass->get_operation       = NULL;
  klass->dialog              = NULL;
  klass->reset               = gimp_filter_tool_real_reset;
  klass->get_settings_ui     = gimp_filter_tool_real_get_settings_ui;
  klass->settings_import     = gimp_filter_tool_real_settings_import;
  klass->settings_export     = gimp_filter_tool_real_settings_export;
}

static void
gimp_filter_tool_init (GimpFilterTool *filter_tool)
{
  GimpTool *tool = GIMP_TOOL (filter_tool);

  gimp_tool_control_set_scroll_lock (tool->control, TRUE);
  gimp_tool_control_set_preserve    (tool->control, FALSE);
  gimp_tool_control_set_dirty_mask  (tool->control,
                                     GIMP_DIRTY_IMAGE           |
                                     GIMP_DIRTY_IMAGE_STRUCTURE |
                                     GIMP_DIRTY_DRAWABLE        |
                                     GIMP_DIRTY_ACTIVE_DRAWABLE);
}

static void
gimp_filter_tool_constructed (GObject *object)
{
  GimpFilterTool *filter_tool = GIMP_FILTER_TOOL (object);

  G_OBJECT_CLASS (parent_class)->constructed (object);

  gimp_filter_tool_get_operation (filter_tool);
}

static void
gimp_filter_tool_finalize (GObject *object)
{
  GimpFilterTool *filter_tool = GIMP_FILTER_TOOL (object);

  if (filter_tool->operation)
    {
      g_object_unref (filter_tool->operation);
      filter_tool->operation = NULL;
    }

  if (filter_tool->config)
    {
      g_object_unref (filter_tool->config);
      filter_tool->config = NULL;
    }

  if (filter_tool->default_config)
    {
      g_object_unref (filter_tool->default_config);
      filter_tool->default_config = NULL;
    }

  if (filter_tool->title)
    {
      g_free (filter_tool->title);
      filter_tool->title = NULL;
    }

  if (filter_tool->description)
    {
      g_free (filter_tool->description);
      filter_tool->description = NULL;
    }

  if (filter_tool->undo_desc)
    {
      g_free (filter_tool->undo_desc);
      filter_tool->undo_desc = NULL;
    }

  if (filter_tool->icon_name)
    {
      g_free (filter_tool->icon_name);
      filter_tool->icon_name = NULL;
    }

  if (filter_tool->help_id)
    {
      g_free (filter_tool->help_id);
      filter_tool->help_id = NULL;
    }

  if (filter_tool->gui)
    {
      g_object_unref (filter_tool->gui);
      filter_tool->gui          = NULL;
      filter_tool->settings_box = NULL;
    }

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
      return FALSE;
    }

  if (! gimp_item_is_visible (GIMP_ITEM (drawable)))
    {
      g_set_error_literal (error, GIMP_ERROR, GIMP_FAILED,
                           _("The active layer is not visible."));
      return FALSE;
    }

  if (filter_tool->active_picker)
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (filter_tool->active_picker),
                                  FALSE);

  /*  set display so the dialog can be hidden on display destruction  */
  tool->display = display;

  if (filter_tool->config)
    gimp_config_reset (GIMP_CONFIG (filter_tool->config));

  if (! filter_tool->gui)
    {
      GimpFilterToolClass *klass = GIMP_FILTER_TOOL_GET_CLASS (filter_tool);
      GtkWidget           *vbox;
      GtkWidget           *hbox;
      GtkWidget           *toggle;
      GtkWidget           *expander;
      GtkWidget           *frame;
      GtkWidget           *vbox2;
      GtkWidget           *combo;
      gchar               *operation_name;

      /*  disabled for at least GIMP 2.8  */
      filter_tool->overlay = FALSE;

      filter_tool->gui =
        gimp_tool_gui_new (tool_info,
                           filter_tool->title,
                           filter_tool->description,
                           filter_tool->icon_name,
                           filter_tool->help_id,
                           gtk_widget_get_screen (GTK_WIDGET (shell)),
                           gimp_widget_get_monitor (GTK_WIDGET (shell)),
                           filter_tool->overlay,

                           GIMP_STOCK_RESET, RESPONSE_RESET,
                           GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                           GTK_STOCK_OK,     GTK_RESPONSE_OK,

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

      if (filter_tool->config && klass->settings_name)
        {
          GType          type = G_TYPE_FROM_INSTANCE (filter_tool->config);
          GimpContainer *settings;
          GFile         *settings_file;
          GFile         *default_folder;
          GtkWidget     *settings_ui;

          settings = gimp_operation_config_get_container (type);
          if (! gimp_list_get_sort_func (GIMP_LIST (settings)))
            gimp_list_set_sort_func (GIMP_LIST (settings),
                                     (GCompareFunc) gimp_settings_compare);

          settings_file = gimp_tool_info_get_options_file (tool_info,
                                                           ".settings");
          default_folder = gimp_directory_file (klass->settings_name, NULL);

          settings_ui = klass->get_settings_ui (filter_tool,
                                                settings,
                                                settings_file,
                                                klass->import_dialog_title,
                                                klass->export_dialog_title,
                                                filter_tool->help_id,
                                                default_folder,
                                                &filter_tool->settings_box);

          g_object_unref (default_folder);
          g_object_unref (settings_file);

          gtk_box_pack_start (GTK_BOX (vbox), settings_ui, FALSE, FALSE, 0);
          gtk_widget_show (settings_ui);
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

      /*  The Color Options expander  */
      expander = gtk_expander_new (_("Advanced Color Options"));
      gtk_box_pack_end (GTK_BOX (vbox), expander, FALSE, FALSE, 0);
      gtk_widget_show (expander);

      frame = gimp_frame_new (NULL);
      gtk_container_add (GTK_CONTAINER (expander), frame);
      gtk_widget_show (frame);

      vbox2 = gtk_box_new (GTK_ORIENTATION_VERTICAL, 2);
      gtk_container_add (GTK_CONTAINER (frame), vbox2);
      gtk_widget_show (vbox2);

      /*  The color managed combo  */
      combo = gimp_prop_boolean_combo_box_new
        (G_OBJECT (tool_info->tool_options), "color-managed",
         _("Apply filter in color managed space (slow)"),
         _("Apply filter to the layer's raw pixels"));
      gtk_box_pack_start (GTK_BOX (vbox2), combo, FALSE, FALSE, 0);
      gtk_widget_show (combo);

      /*  The gamma hack toggle  */
      toggle = gimp_prop_check_button_new (G_OBJECT (tool_info->tool_options),
                                           "gamma-hack", NULL);
      gtk_box_pack_start (GTK_BOX (vbox2), toggle, FALSE, FALSE, 0);
      gtk_widget_show (toggle);

      /*  The area combo  */
      gegl_node_get (filter_tool->operation,
                     "operation", &operation_name,
                     NULL);

      filter_tool->region_combo =
        gimp_prop_enum_combo_box_new (G_OBJECT (tool_info->tool_options),
                                      "region",
                                      0, 0);
      gtk_box_pack_end (GTK_BOX (vbox), filter_tool->region_combo,
                        FALSE, FALSE, 0);

      if (operation_name &&
          gegl_operation_get_key (operation_name, "position-dependent"))
        {
          gtk_widget_show (filter_tool->region_combo);
        }

      g_free (operation_name);

      /*  Fill in subclass widgets  */
      gimp_filter_tool_dialog (filter_tool);
    }
  else
    {
      gimp_tool_gui_set_title (filter_tool->gui, filter_tool->title);
      gimp_tool_gui_set_description (filter_tool->gui, filter_tool->description);
      gimp_tool_gui_set_icon_name (filter_tool->gui, filter_tool->icon_name);
      gimp_tool_gui_set_help_id (filter_tool->gui, filter_tool->help_id);
    }

  gimp_tool_gui_set_shell (filter_tool->gui, shell);
  gimp_tool_gui_set_viewable (filter_tool->gui, GIMP_VIEWABLE (drawable));

  filter_tool->drawable = drawable;
  gimp_tool_gui_show (filter_tool->gui);

  gimp_filter_tool_create_map (filter_tool);

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

  if (! gimp_filter_tool_on_guide (filter_tool, coords, display))
    {
      GIMP_TOOL_CLASS (parent_class)->button_press (tool, coords, time, state,
                                                    press_type, display);
    }
  else
    {
      GimpFilterOptions *options = GIMP_FILTER_TOOL_GET_OPTIONS (tool);

      if (state & gimp_get_extend_selection_mask ())
        {
          GimpAlignmentType alignment;

          /* switch side */
          switch (options->preview_alignment)
            {
            case GIMP_ALIGN_LEFT:   alignment = GIMP_ALIGN_RIGHT;  break;
            case GIMP_ALIGN_RIGHT:  alignment = GIMP_ALIGN_LEFT;   break;
            case GIMP_ALIGN_TOP:    alignment = GIMP_ALIGN_BOTTOM; break;
            case GIMP_ALIGN_BOTTOM: alignment = GIMP_ALIGN_TOP;    break;
            default:
              g_return_if_reached ();
            }

          g_object_set (options, "preview-alignment", alignment, NULL);
        }
      else if (state & gimp_get_toggle_behavior_mask ())
        {
          GimpItem          *item = GIMP_ITEM (filter_tool->drawable);
          GimpAlignmentType  alignment;
          gdouble            position;

          /* switch orientation */
          switch (options->preview_alignment)
            {
            case GIMP_ALIGN_LEFT:   alignment = GIMP_ALIGN_TOP;    break;
            case GIMP_ALIGN_RIGHT:  alignment = GIMP_ALIGN_BOTTOM; break;
            case GIMP_ALIGN_TOP:    alignment = GIMP_ALIGN_LEFT;   break;
            case GIMP_ALIGN_BOTTOM: alignment = GIMP_ALIGN_RIGHT;  break;
            default:
              g_return_if_reached ();
            }

          if (alignment == GIMP_ALIGN_LEFT ||
              alignment == GIMP_ALIGN_RIGHT)
            {
              position = ((coords->x - gimp_item_get_offset_x (item)) /
                          gimp_item_get_width (item));
            }
          else
            {
              position = ((coords->y - gimp_item_get_offset_y (item)) /
                          gimp_item_get_height (item));
            }

          g_object_set (options,
                        "preview-alignment", alignment,
                        "preview-position",  CLAMP (position, 0.0, 1.0),
                        NULL);
        }
      else
        {
          gimp_guide_tool_start_edit (tool, display,
                                      filter_tool->percent_guide);
        }
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

  return FALSE;
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

  if (! gimp_filter_tool_on_guide (filter_tool, coords, display))
    {
      GIMP_TOOL_CLASS (parent_class)->oper_update (tool, coords, state,
                                                   proximity, display);
    }
  else
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
}

static void
gimp_filter_tool_cursor_update (GimpTool         *tool,
                                const GimpCoords *coords,
                                GdkModifierType   state,
                                GimpDisplay      *display)
{
  GimpFilterTool *filter_tool = GIMP_FILTER_TOOL (tool);

  if (! gimp_filter_tool_on_guide (filter_tool, coords, display))
    {
      GIMP_TOOL_CLASS (parent_class)->cursor_update (tool, coords, state,
                                                     display);
    }
  else
    {
      gimp_tool_set_cursor (tool, display,
                            GIMP_CURSOR_MOUSE,
                            GIMP_TOOL_CURSOR_HAND,
                            GIMP_CURSOR_MODIFIER_MOVE);
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
          GimpItem         *item  = GIMP_ITEM (filter_tool->drawable);
          gint              x, y, width, height;

          gimp_display_shell_untransform_viewport (shell,
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

      gimp_drawable_filter_set_preview (filter_tool->filter,
                                        filter_options->preview_split,
                                        filter_options->preview_alignment,
                                        filter_options->preview_position);

      if (filter_options->preview_split)
        gimp_filter_tool_add_guide (filter_tool);
      else
        gimp_filter_tool_remove_guide (filter_tool);
    }
  else if (! strcmp (pspec->name, "preview-alignment") &&
           filter_tool->filter)
    {
      gimp_drawable_filter_set_preview (filter_tool->filter,
                                        filter_options->preview_split,
                                        filter_options->preview_alignment,
                                        filter_options->preview_position);

      if (filter_options->preview_split)
        gimp_filter_tool_move_guide (filter_tool);
    }
  else if (! strcmp (pspec->name, "preview-position") &&
           filter_tool->filter)
    {
      gimp_drawable_filter_set_preview (filter_tool->filter,
                                        filter_options->preview_split,
                                        filter_options->preview_alignment,
                                        filter_options->preview_position);

      if (filter_options->preview_split)
        gimp_filter_tool_move_guide (filter_tool);
    }
  else if (! strcmp (pspec->name, "region") &&
           filter_tool->filter)
    {
      gimp_drawable_filter_set_region (filter_tool->filter,
                                       filter_options->region);
    }
  else if (! strcmp (pspec->name, "color-managed") &&
           filter_tool->filter)
    {
      gimp_drawable_filter_set_color_managed (filter_tool->filter,
                                              filter_options->color_managed);
    }
  else if (! strcmp (pspec->name, "gamma-hack") &&
           filter_tool->filter)
    {
      gimp_drawable_filter_set_gamma_hack (filter_tool->filter,
                                           filter_options->gamma_hack);
    }
}

static gboolean
gimp_filter_tool_pick_color (GimpColorTool  *color_tool,
                             gint            x,
                             gint            y,
                             const Babl    **sample_format,
                             gpointer        pixel,
                             GimpRGB        *color)
{
  GimpFilterTool *filter_tool = GIMP_FILTER_TOOL (color_tool);
  gint            off_x, off_y;

  gimp_item_get_offset (GIMP_ITEM (filter_tool->drawable), &off_x, &off_y);

  *sample_format = gimp_drawable_get_format (filter_tool->drawable);

  return gimp_pickable_pick_color (GIMP_PICKABLE (filter_tool->drawable),
                                   x - off_x,
                                   y - off_y,
                                   color_tool->options->sample_average,
                                   color_tool->options->average_radius,
                                   pixel, color);
}

static void
gimp_filter_tool_color_picked (GimpColorTool      *color_tool,
                               GimpColorPickState  pick_state,
                               gdouble             x,
                               gdouble             y,
                               const Babl         *sample_format,
                               gpointer            pixel,
                               const GimpRGB      *color)
{
  GimpFilterTool *filter_tool = GIMP_FILTER_TOOL (color_tool);
  gpointer        identifier;

  g_return_if_fail (GTK_IS_WIDGET (filter_tool->active_picker));

  identifier = g_object_get_data (G_OBJECT (filter_tool->active_picker),
                                  "picker-identifier");

  GIMP_FILTER_TOOL_GET_CLASS (filter_tool)->color_picked (filter_tool,
                                                         identifier,
                                                         x, y,
                                                         sample_format,
                                                         color);
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
gimp_filter_tool_halt (GimpFilterTool *filter_tool)
{
  GimpTool *tool = GIMP_TOOL (filter_tool);

  if (filter_tool->gui)
    gimp_tool_gui_hide (filter_tool->gui);

  if (filter_tool->filter)
    {
      gimp_drawable_filter_abort (filter_tool->filter);
      g_object_unref (filter_tool->filter);
      filter_tool->filter = NULL;

      gimp_filter_tool_remove_guide (filter_tool);
    }

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
      g_object_unref (filter_tool->filter);
      filter_tool->filter = NULL;

      gimp_tool_control_pop_preserve (tool->control);

      gimp_filter_tool_remove_guide (filter_tool);

      gimp_image_flush (gimp_display_get_image (tool->display));

      if (filter_tool->config && filter_tool->settings_box)
        {
          GimpGuiConfig *config = GIMP_GUI_CONFIG (tool->tool_info->gimp->config);

          gimp_settings_box_add_current (GIMP_SETTINGS_BOX (filter_tool->settings_box),
                                         config->filter_tool_max_recent);
        }
    }

  tool->display  = NULL;
  tool->drawable = NULL;
}

static void
gimp_filter_tool_dialog (GimpFilterTool *filter_tool)
{
  GIMP_FILTER_TOOL_GET_CLASS (filter_tool)->dialog (filter_tool);

  g_signal_connect (gimp_tool_gui_get_dialog (filter_tool->gui), "unmap",
                    G_CALLBACK (gimp_filter_tool_dialog_unmap),
                    filter_tool);
}

static void
gimp_filter_tool_dialog_unmap (GtkWidget      *dialog,
                               GimpFilterTool *filter_tool)
{
  if (filter_tool->active_picker)
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (filter_tool->active_picker),
                                  FALSE);
}

static void
gimp_filter_tool_reset (GimpFilterTool *filter_tool)
{
  if (filter_tool->config)
    g_object_freeze_notify (filter_tool->config);

  GIMP_FILTER_TOOL_GET_CLASS (filter_tool)->reset (filter_tool);

  if (filter_tool->config)
    g_object_thaw_notify (filter_tool->config);
}

static void
gimp_filter_tool_create_map (GimpFilterTool *filter_tool)
{
  GimpFilterOptions *options = GIMP_FILTER_TOOL_GET_OPTIONS (filter_tool);

  if (filter_tool->filter)
    {
      gimp_drawable_filter_abort (filter_tool->filter);
      g_object_unref (filter_tool->filter);
    }

  g_assert (filter_tool->operation);

  filter_tool->filter = gimp_drawable_filter_new (filter_tool->drawable,
                                                  filter_tool->undo_desc,
                                                  filter_tool->operation,
                                                  filter_tool->icon_name);

  gimp_drawable_filter_set_region (filter_tool->filter, options->region);

  g_signal_connect (filter_tool->filter, "flush",
                    G_CALLBACK (gimp_filter_tool_flush),
                    filter_tool);

  gimp_gegl_progress_connect (filter_tool->operation,
                              GIMP_PROGRESS (filter_tool),
                              filter_tool->undo_desc);

  if (options->preview)
    gimp_drawable_filter_apply (filter_tool->filter, NULL);
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
  GimpFilterOptions *options = GIMP_FILTER_TOOL_GET_OPTIONS (filter_tool);

  if (filter_tool->filter && options->preview)
    gimp_drawable_filter_apply (filter_tool->filter, NULL);
}

static void
gimp_filter_tool_add_guide (GimpFilterTool *filter_tool)
{
  GimpFilterOptions   *options = GIMP_FILTER_TOOL_GET_OPTIONS (filter_tool);
  GimpItem            *item;
  GimpImage           *image;
  GimpOrientationType  orientation;
  gint                 position;

  if (filter_tool->percent_guide)
    return;

  item = GIMP_ITEM (filter_tool->drawable);
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

  filter_tool->percent_guide =
    gimp_guide_custom_new (orientation,
                           image->gimp->next_guide_ID++,
                           GIMP_GUIDE_STYLE_SPLIT_VIEW);

  gimp_image_add_guide (image, filter_tool->percent_guide, position);

  g_signal_connect (filter_tool->percent_guide, "removed",
                    G_CALLBACK (gimp_filter_tool_guide_removed),
                    filter_tool);
  g_signal_connect (filter_tool->percent_guide, "notify::position",
                    G_CALLBACK (gimp_filter_tool_guide_moved),
                    filter_tool);
}

static void
gimp_filter_tool_remove_guide (GimpFilterTool *filter_tool)
{
  GimpImage *image;

  if (! filter_tool->percent_guide)
    return;

  image = gimp_item_get_image (GIMP_ITEM (filter_tool->drawable));

  gimp_image_remove_guide (image, filter_tool->percent_guide, FALSE);
}

static void
gimp_filter_tool_move_guide (GimpFilterTool *filter_tool)
{
  GimpFilterOptions   *options = GIMP_FILTER_TOOL_GET_OPTIONS (filter_tool);
  GimpItem            *item;
  GimpOrientationType  orientation;
  gint                 position;

  if (! filter_tool->percent_guide)
    return;

  item = GIMP_ITEM (filter_tool->drawable);

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

  if (orientation != gimp_guide_get_orientation (filter_tool->percent_guide) ||
      position    != gimp_guide_get_position (filter_tool->percent_guide))
    {
      gimp_guide_set_orientation (filter_tool->percent_guide, orientation);
      gimp_image_move_guide (gimp_item_get_image (item),
                             filter_tool->percent_guide, position, FALSE);
    }
}

static void
gimp_filter_tool_guide_removed (GimpGuide      *guide,
                                GimpFilterTool *filter_tool)
{
  GimpFilterOptions *options = GIMP_FILTER_TOOL_GET_OPTIONS (filter_tool);

  g_signal_handlers_disconnect_by_func (G_OBJECT (filter_tool->percent_guide),
                                        gimp_filter_tool_guide_removed,
                                        filter_tool);
  g_signal_handlers_disconnect_by_func (G_OBJECT (filter_tool->percent_guide),
                                        gimp_filter_tool_guide_moved,
                                        filter_tool);

  g_object_unref (filter_tool->percent_guide);
  filter_tool->percent_guide = NULL;

  g_object_set (options,
                "preview-split", FALSE,
                NULL);
}

static void
gimp_filter_tool_guide_moved (GimpGuide        *guide,
                              const GParamSpec *pspec,
                              GimpFilterTool   *filter_tool)
{
  GimpFilterOptions *options = GIMP_FILTER_TOOL_GET_OPTIONS (filter_tool);
  GimpItem          *item    = GIMP_ITEM (filter_tool->drawable);
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


/*  public functions  */

void
gimp_filter_tool_get_operation (GimpFilterTool *filter_tool)
{
  GimpFilterToolClass *klass;
  GimpToolInfo        *tool_info;
  gchar               *operation_name;

  g_return_if_fail (GIMP_IS_FILTER_TOOL (filter_tool));

  klass = GIMP_FILTER_TOOL_GET_CLASS (filter_tool);

  tool_info = GIMP_TOOL (filter_tool)->tool_info;

  if (filter_tool->filter)
    {
      gimp_drawable_filter_abort (filter_tool->filter);
      g_object_unref (filter_tool->filter);
      filter_tool->filter = NULL;
    }

  if (filter_tool->operation)
    {
      g_object_unref (filter_tool->operation);
      filter_tool->operation = NULL;
    }

  if (filter_tool->config)
    {
      g_signal_handlers_disconnect_by_func (filter_tool->config,
                                            gimp_filter_tool_config_notify,
                                            filter_tool);

      g_object_unref (filter_tool->config);
      filter_tool->config = NULL;
    }

  if (filter_tool->default_config)
    {
      g_object_unref (filter_tool->default_config);
      filter_tool->default_config = NULL;
    }

  if (filter_tool->title)
    {
      g_free (filter_tool->title);
      filter_tool->title = NULL;
    }

  if (filter_tool->description)
    {
      g_free (filter_tool->description);
      filter_tool->description = NULL;
    }

  if (filter_tool->undo_desc)
    {
      g_free (filter_tool->undo_desc);
      filter_tool->undo_desc = NULL;
    }

  if (filter_tool->icon_name)
    {
      g_free (filter_tool->icon_name);
      filter_tool->icon_name = NULL;
    }

  if (filter_tool->help_id)
    {
      g_free (filter_tool->help_id);
      filter_tool->help_id = NULL;
    }

  operation_name = klass->get_operation (filter_tool,
                                         &filter_tool->title,
                                         &filter_tool->description,
                                         &filter_tool->undo_desc,
                                         &filter_tool->icon_name,
                                         &filter_tool->help_id);

  if (! operation_name)
    operation_name = g_strdup ("gegl:nop");

  if (! filter_tool->title)
    filter_tool->title = g_strdup (tool_info->blurb);

  if (! filter_tool->description)
    filter_tool->description = g_strdup (filter_tool->title);

  if (! filter_tool->undo_desc)
    filter_tool->undo_desc = g_strdup (tool_info->blurb);

  if (! filter_tool->icon_name)
    filter_tool->icon_name =
      g_strdup (gimp_viewable_get_icon_name (GIMP_VIEWABLE (tool_info)));

  if (! filter_tool->help_id)
    filter_tool->help_id = g_strdup (tool_info->help_id);

  filter_tool->operation = gegl_node_new_child (NULL,
                                                "operation", operation_name,
                                                NULL);
  filter_tool->config =
    G_OBJECT (gimp_operation_config_new (operation_name,
                                         filter_tool->icon_name,
                                         GIMP_TYPE_SETTINGS));

  gimp_operation_config_sync_node (GIMP_OBJECT (filter_tool->config),
                                   filter_tool->operation);
  gimp_operation_config_connect_node (GIMP_OBJECT (filter_tool->config),
                                      filter_tool->operation);

  if (filter_tool->gui)
    {
      gimp_tool_gui_set_title       (filter_tool->gui, filter_tool->title);
      gimp_tool_gui_set_description (filter_tool->gui, filter_tool->description);
      gimp_tool_gui_set_icon_name   (filter_tool->gui, filter_tool->icon_name);
      gimp_tool_gui_set_help_id     (filter_tool->gui, filter_tool->help_id);
    }

  if (gegl_operation_get_key (operation_name, "position-dependent"))
    {
      if (filter_tool->gui)
        gtk_widget_show (filter_tool->region_combo);
    }
  else
    {
      if (filter_tool->gui)
        gtk_widget_hide (filter_tool->region_combo);

      g_object_set (GIMP_FILTER_TOOL_GET_OPTIONS (filter_tool),
                    "region", GIMP_FILTER_REGION_SELECTION,
                    NULL);
    }

  g_free (operation_name);

  g_object_set (GIMP_FILTER_TOOL_GET_OPTIONS (filter_tool),
                "preview-split",    FALSE,
                "preview-position", 0.5,
                NULL);

  if (filter_tool->config)
    g_signal_connect_object (filter_tool->config, "notify",
                             G_CALLBACK (gimp_filter_tool_config_notify),
                             G_OBJECT (filter_tool), 0);

  if (GIMP_TOOL (filter_tool)->drawable)
    gimp_filter_tool_create_map (filter_tool);
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

  GIMP_FILTER_TOOL (new_tool)->default_config = g_object_ref (config);

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
      filter_tool->percent_guide &&
      gimp_display_shell_get_show_guides (shell))
    {
      const gint          snap_distance = display->config->snap_distance;
      GimpOrientationType orientation;
      gint                position;

      orientation = gimp_guide_get_orientation (filter_tool->percent_guide);
      position    = gimp_guide_get_position (filter_tool->percent_guide);

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

static void
gimp_filter_tool_color_picker_toggled (GtkWidget      *widget,
                                       GimpFilterTool *filter_tool)
{
  if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (widget)))
    {
      if (filter_tool->active_picker == widget)
        return;

      if (filter_tool->active_picker)
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (filter_tool->active_picker),
                                      FALSE);

      filter_tool->active_picker = widget;

      gimp_color_tool_enable (GIMP_COLOR_TOOL (filter_tool),
                              GIMP_COLOR_TOOL_GET_OPTIONS (filter_tool));
    }
  else if (filter_tool->active_picker == widget)
    {
      filter_tool->active_picker = NULL;
      gimp_color_tool_disable (GIMP_COLOR_TOOL (filter_tool));
    }
}

GtkWidget *
gimp_filter_tool_add_color_picker (GimpFilterTool *filter_tool,
                                   gpointer        identifier,
                                   const gchar    *icon_name,
                                   const gchar    *tooltip)
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

  g_object_set_data (G_OBJECT (button), "picker-identifier", identifier);

  g_signal_connect (button, "toggled",
                    G_CALLBACK (gimp_filter_tool_color_picker_toggled),
                    filter_tool);

  return button;
}
