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
#include "libgimpwidgets/gimpwidgets.h"

#include "tools-types.h"

#include "config/gimpguiconfig.h"

#include "gegl/gimp-gegl-config.h"

#include "core/gimp.h"
#include "core/gimpdrawable.h"
#include "core/gimperror.h"
#include "core/gimpimage.h"
#include "core/gimpimage-pick-color.h"
#include "core/gimpimagemap.h"
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
#include "display/gimptoolgui.h"

#include "gimpcoloroptions.h"
#include "gimpimagemaptool.h"
#include "gimpimagemaptool-settings.h"
#include "gimptoolcontrol.h"
#include "tool_manager.h"

#include "gimp-intl.h"


/*  local function prototypes  */

static void      gimp_image_map_tool_class_init     (GimpImageMapToolClass *klass);
static void      gimp_image_map_tool_base_init      (GimpImageMapToolClass *klass);

static void      gimp_image_map_tool_init           (GimpImageMapTool *im_tool);

static void      gimp_image_map_tool_constructed    (GObject          *object);
static void      gimp_image_map_tool_finalize       (GObject          *object);

static gboolean  gimp_image_map_tool_initialize     (GimpTool         *tool,
                                                     GimpDisplay      *display,
                                                     GError          **error);
static void      gimp_image_map_tool_control        (GimpTool         *tool,
                                                     GimpToolAction    action,
                                                     GimpDisplay      *display);
static gboolean  gimp_image_map_tool_key_press      (GimpTool         *tool,
                                                     GdkEventKey      *kevent,
                                                     GimpDisplay      *display);
static void      gimp_image_map_tool_options_notify (GimpTool         *tool,
                                                     GimpToolOptions  *options,
                                                     const GParamSpec *pspec);

static gboolean  gimp_image_map_tool_pick_color     (GimpColorTool    *color_tool,
                                                     gint              x,
                                                     gint              y,
                                                     const Babl      **sample_format,
                                                     gpointer          pixel,
                                                     GimpRGB          *color);
static void      gimp_image_map_tool_color_picked   (GimpColorTool    *color_tool,
                                                     GimpColorPickState pick_state,
                                                     gdouble           x,
                                                     gdouble           y,
                                                     const Babl       *sample_format,
                                                     gpointer          pixel,
                                                     const GimpRGB    *color);

static void      gimp_image_map_tool_real_reset     (GimpImageMapTool *im_tool);

static void      gimp_image_map_tool_halt           (GimpImageMapTool *im_tool);
static void      gimp_image_map_tool_commit         (GimpImageMapTool *im_tool);

static void      gimp_image_map_tool_map            (GimpImageMapTool *im_tool);
static void      gimp_image_map_tool_dialog         (GimpImageMapTool *im_tool);
static void      gimp_image_map_tool_dialog_unmap   (GtkWidget        *dialog,
                                                     GimpImageMapTool *im_tool);
static void      gimp_image_map_tool_reset          (GimpImageMapTool *im_tool);
static void      gimp_image_map_tool_create_map     (GimpImageMapTool *im_tool);

static void      gimp_image_map_tool_flush          (GimpImageMap     *image_map,
                                                     GimpImageMapTool *im_tool);
static void      gimp_image_map_tool_config_notify  (GObject          *object,
                                                     const GParamSpec *pspec,
                                                     GimpImageMapTool *im_tool);

static void      gimp_image_map_tool_response       (GimpToolGui      *gui,
                                                     gint              response_id,
                                                     GimpImageMapTool *im_tool);


static GimpColorToolClass *parent_class = NULL;


GType
gimp_image_map_tool_get_type (void)
{
  static GType type = 0;

  if (! type)
    {
      const GTypeInfo info =
      {
        sizeof (GimpImageMapToolClass),
        (GBaseInitFunc) gimp_image_map_tool_base_init,
        (GBaseFinalizeFunc) NULL,
        (GClassInitFunc) gimp_image_map_tool_class_init,
        NULL,           /* class_finalize */
        NULL,           /* class_data */
        sizeof (GimpImageMapTool),
        0,              /* n_preallocs */
        (GInstanceInitFunc) gimp_image_map_tool_init,
      };

      type = g_type_register_static (GIMP_TYPE_COLOR_TOOL,
                                     "GimpImageMapTool",
                                     &info, 0);
    }

  return type;
}


static void
gimp_image_map_tool_class_init (GimpImageMapToolClass *klass)
{
  GObjectClass       *object_class     = G_OBJECT_CLASS (klass);
  GimpToolClass      *tool_class       = GIMP_TOOL_CLASS (klass);
  GimpColorToolClass *color_tool_class = GIMP_COLOR_TOOL_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);

  object_class->constructed  = gimp_image_map_tool_constructed;
  object_class->finalize     = gimp_image_map_tool_finalize;

  tool_class->initialize     = gimp_image_map_tool_initialize;
  tool_class->control        = gimp_image_map_tool_control;
  tool_class->key_press      = gimp_image_map_tool_key_press;
  tool_class->options_notify = gimp_image_map_tool_options_notify;

  color_tool_class->pick     = gimp_image_map_tool_pick_color;
  color_tool_class->picked   = gimp_image_map_tool_color_picked;

  klass->settings_name       = NULL;
  klass->import_dialog_title = NULL;
  klass->export_dialog_title = NULL;

  klass->get_operation       = NULL;
  klass->dialog              = NULL;
  klass->reset               = gimp_image_map_tool_real_reset;
  klass->get_settings_ui     = gimp_image_map_tool_real_get_settings_ui;
  klass->settings_import     = gimp_image_map_tool_real_settings_import;
  klass->settings_export     = gimp_image_map_tool_real_settings_export;
}

static void
gimp_image_map_tool_base_init (GimpImageMapToolClass *klass)
{
}

static void
gimp_image_map_tool_init (GimpImageMapTool *im_tool)
{
  GimpTool *tool = GIMP_TOOL (im_tool);

  gimp_tool_control_set_scroll_lock (tool->control, TRUE);
  gimp_tool_control_set_preserve    (tool->control, FALSE);
  gimp_tool_control_set_dirty_mask  (tool->control,
                                     GIMP_DIRTY_IMAGE           |
                                     GIMP_DIRTY_IMAGE_STRUCTURE |
                                     GIMP_DIRTY_DRAWABLE        |
                                     GIMP_DIRTY_SELECTION       |
                                     GIMP_DIRTY_ACTIVE_DRAWABLE);
}

static void
gimp_image_map_tool_constructed (GObject *object)
{
  GimpImageMapTool *im_tool = GIMP_IMAGE_MAP_TOOL (object);

  G_OBJECT_CLASS (parent_class)->constructed (object);

  gimp_image_map_tool_get_operation (im_tool);
}

static void
gimp_image_map_tool_finalize (GObject *object)
{
  GimpImageMapTool *im_tool = GIMP_IMAGE_MAP_TOOL (object);

  if (im_tool->operation)
    {
      g_object_unref (im_tool->operation);
      im_tool->operation = NULL;
    }

  if (im_tool->config)
    {
      g_object_unref (im_tool->config);
      im_tool->config = NULL;
    }

  if (im_tool->default_config)
    {
      g_object_unref (im_tool->default_config);
      im_tool->default_config = NULL;
    }

  if (im_tool->title)
    {
      g_free (im_tool->title);
      im_tool->title = NULL;
    }

  if (im_tool->description)
    {
      g_free (im_tool->description);
      im_tool->description = NULL;
    }

  if (im_tool->undo_desc)
    {
      g_free (im_tool->undo_desc);
      im_tool->undo_desc = NULL;
    }

  if (im_tool->icon_name)
    {
      g_free (im_tool->icon_name);
      im_tool->icon_name = NULL;
    }

  if (im_tool->help_id)
    {
      g_free (im_tool->help_id);
      im_tool->help_id = NULL;
    }

  if (im_tool->gui)
    {
      g_object_unref (im_tool->gui);
      im_tool->gui          = NULL;
      im_tool->settings_box = NULL;
    }

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
gamma_hack (GtkToggleButton  *button,
            GimpImageMapTool *im_tool)
{
  if (im_tool->image_map)
    {
      gimp_image_map_set_gamma_hack (im_tool->image_map,
                                     gtk_toggle_button_get_active (button));
      gimp_image_map_tool_preview (im_tool);
    }
}

#define RESPONSE_RESET 1

static gboolean
gimp_image_map_tool_initialize (GimpTool     *tool,
                                GimpDisplay  *display,
                                GError      **error)
{
  GimpImageMapTool *im_tool   = GIMP_IMAGE_MAP_TOOL (tool);
  GimpToolInfo     *tool_info = tool->tool_info;
  GimpImage        *image     = gimp_display_get_image (display);
  GimpDrawable     *drawable  = gimp_image_get_active_drawable (image);
  GimpDisplayShell *shell     = gimp_display_get_shell (display);

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

  if (im_tool->active_picker)
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (im_tool->active_picker),
                                  FALSE);

  /*  set display so the dialog can be hidden on display destruction  */
  tool->display = display;

  if (im_tool->config)
    gimp_config_reset (GIMP_CONFIG (im_tool->config));

  if (! im_tool->gui)
    {
      GimpImageMapToolClass *klass = GIMP_IMAGE_MAP_TOOL_GET_CLASS (im_tool);
      GtkWidget             *vbox;
      GtkWidget             *toggle;
      gchar                 *operation_name;

      /*  disabled for at least GIMP 2.8  */
      im_tool->overlay = FALSE;

      im_tool->gui =
        gimp_tool_gui_new (tool_info,
                           im_tool->title,
                           im_tool->description,
                           im_tool->icon_name,
                           im_tool->help_id,
                           gtk_widget_get_screen (GTK_WIDGET (shell)),
                           gimp_widget_get_monitor (GTK_WIDGET (shell)),
                           im_tool->overlay,

                           GIMP_STOCK_RESET, RESPONSE_RESET,
                           GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                           GTK_STOCK_OK,     GTK_RESPONSE_OK,

                           NULL);

      gimp_tool_gui_set_default_response (im_tool->gui, GTK_RESPONSE_OK);

      gimp_tool_gui_set_alternative_button_order (im_tool->gui,
                                                  RESPONSE_RESET,
                                                  GTK_RESPONSE_OK,
                                                  GTK_RESPONSE_CANCEL,
                                                  -1);

      vbox = gimp_tool_gui_get_vbox (im_tool->gui);

      g_signal_connect_object (im_tool->gui, "response",
                               G_CALLBACK (gimp_image_map_tool_response),
                               G_OBJECT (im_tool), 0);

      if (im_tool->config && klass->settings_name)
        {
          GType          type = G_TYPE_FROM_INSTANCE (im_tool->config);
          GimpContainer *settings;
          GFile         *settings_file;
          GFile         *default_folder;
          GtkWidget     *settings_ui;

          settings = gimp_gegl_config_get_container (type);
          if (! gimp_list_get_sort_func (GIMP_LIST (settings)))
            gimp_list_set_sort_func (GIMP_LIST (settings),
                                     (GCompareFunc) gimp_settings_compare);

          settings_file = gimp_tool_info_get_options_file (tool_info,
                                                           ".settings");
          default_folder = gimp_directory_file (klass->settings_name, NULL);

          settings_ui = klass->get_settings_ui (im_tool,
                                                settings,
                                                settings_file,
                                                klass->import_dialog_title,
                                                klass->export_dialog_title,
                                                im_tool->help_id,
                                                default_folder,
                                                &im_tool->settings_box);

          g_object_unref (default_folder);
          g_object_unref (settings_file);

          gtk_box_pack_start (GTK_BOX (vbox), settings_ui, FALSE, FALSE, 0);
          gtk_widget_show (settings_ui);
        }

      /*  The gamma hack toggle  */
      toggle = gtk_check_button_new_with_label ("Gamma hack (temp hack, please ignore)");
      gtk_box_pack_end (GTK_BOX (vbox), toggle, FALSE, FALSE, 0);
      gtk_widget_show (toggle);

      g_signal_connect (toggle, "toggled",
                        G_CALLBACK (gamma_hack),
                        im_tool);

      /*  The preview toggle  */
      toggle = gimp_prop_check_button_new (G_OBJECT (tool_info->tool_options),
                                           "preview", NULL);
      gtk_box_pack_end (GTK_BOX (vbox), toggle, FALSE, FALSE, 0);
      gtk_widget_show (toggle);

      /*  The area combo  */
      gegl_node_get (im_tool->operation,
                     "operation", &operation_name,
                     NULL);

      im_tool->region_combo =
        gimp_prop_enum_combo_box_new (G_OBJECT (tool_info->tool_options),
                                      "region",
                                      0, 0);
      gtk_box_pack_end (GTK_BOX (vbox), im_tool->region_combo,
                        FALSE, FALSE, 0);

      if (operation_name &&
          gegl_operation_get_key (operation_name, "position-dependent"))
        {
          gtk_widget_show (im_tool->region_combo);
        }

      g_free (operation_name);

      /*  Fill in subclass widgets  */
      gimp_image_map_tool_dialog (im_tool);
    }
  else
    {
      gimp_tool_gui_set_title (im_tool->gui, im_tool->title);
      gimp_tool_gui_set_description (im_tool->gui, im_tool->description);
      gimp_tool_gui_set_icon_name (im_tool->gui, im_tool->icon_name);
      gimp_tool_gui_set_help_id (im_tool->gui, im_tool->help_id);
    }

  gimp_tool_gui_set_shell (im_tool->gui, shell);
  gimp_tool_gui_set_viewable (im_tool->gui, GIMP_VIEWABLE (drawable));

  gimp_tool_gui_show (im_tool->gui);

  im_tool->drawable = drawable;

  gimp_image_map_tool_create_map (im_tool);
  gimp_image_map_tool_preview (im_tool);

  return TRUE;
}

static void
gimp_image_map_tool_control (GimpTool       *tool,
                             GimpToolAction  action,
                             GimpDisplay    *display)
{
  GimpImageMapTool *im_tool = GIMP_IMAGE_MAP_TOOL (tool);

  switch (action)
    {
    case GIMP_TOOL_ACTION_PAUSE:
    case GIMP_TOOL_ACTION_RESUME:
      break;

    case GIMP_TOOL_ACTION_HALT:
      gimp_image_map_tool_halt (im_tool);
      break;

    case GIMP_TOOL_ACTION_COMMIT:
      gimp_image_map_tool_commit (im_tool);
      break;
    }

  GIMP_TOOL_CLASS (parent_class)->control (tool, action, display);
}

static gboolean
gimp_image_map_tool_key_press (GimpTool    *tool,
                               GdkEventKey *kevent,
                               GimpDisplay *display)
{
  GimpImageMapTool *im_tool = GIMP_IMAGE_MAP_TOOL (tool);

  if (im_tool->gui && display == tool->display)
    {
      switch (kevent->keyval)
        {
        case GDK_KEY_Return:
        case GDK_KEY_KP_Enter:
        case GDK_KEY_ISO_Enter:
          gimp_image_map_tool_response (im_tool->gui,
                                        GTK_RESPONSE_OK,
                                        im_tool);
          return TRUE;

        case GDK_KEY_BackSpace:
          gimp_image_map_tool_response (im_tool->gui,
                                        RESPONSE_RESET,
                                        im_tool);
          return TRUE;

        case GDK_KEY_Escape:
          gimp_image_map_tool_response (im_tool->gui,
                                        GTK_RESPONSE_CANCEL,
                                        im_tool);
          return TRUE;
        }
    }

  return FALSE;
}

static void
gimp_image_map_tool_options_notify (GimpTool         *tool,
                                    GimpToolOptions  *options,
                                    const GParamSpec *pspec)
{
  GimpImageMapTool    *im_tool    = GIMP_IMAGE_MAP_TOOL (tool);
  GimpImageMapOptions *im_options = GIMP_IMAGE_MAP_OPTIONS (options);

  if (! strcmp (pspec->name, "preview") &&
      im_tool->image_map)
    {
      if (im_options->preview)
        {
          gimp_tool_control_push_preserve (tool->control, TRUE);

          gimp_image_map_tool_map (im_tool);

          gimp_tool_control_pop_preserve (tool->control);
        }
      else
        {
          gimp_tool_control_push_preserve (tool->control, TRUE);

          gimp_image_map_abort (im_tool->image_map);

          gimp_tool_control_pop_preserve (tool->control);
        }
    }
  else if (! strcmp (pspec->name, "region") &&
           im_tool->image_map)
    {
      gimp_image_map_set_region (im_tool->image_map, im_options->region);
      gimp_image_map_tool_preview (im_tool);
    }
}

static gboolean
gimp_image_map_tool_pick_color (GimpColorTool  *color_tool,
                                gint            x,
                                gint            y,
                                const Babl    **sample_format,
                                gpointer        pixel,
                                GimpRGB        *color)
{
  GimpImageMapTool *im_tool = GIMP_IMAGE_MAP_TOOL (color_tool);
  gint              off_x, off_y;

  gimp_item_get_offset (GIMP_ITEM (im_tool->drawable), &off_x, &off_y);

  *sample_format = gimp_drawable_get_format (im_tool->drawable);

  return gimp_pickable_pick_color (GIMP_PICKABLE (im_tool->drawable),
                                   x - off_x,
                                   y - off_y,
                                   color_tool->options->sample_average,
                                   color_tool->options->average_radius,
                                   pixel, color);
}

static void
gimp_image_map_tool_color_picked (GimpColorTool      *color_tool,
                                  GimpColorPickState  pick_state,
                                  gdouble             x,
                                  gdouble             y,
                                  const Babl         *sample_format,
                                  gpointer            pixel,
                                  const GimpRGB      *color)
{
  GimpImageMapTool *im_tool = GIMP_IMAGE_MAP_TOOL (color_tool);
  gpointer          identifier;

  g_return_if_fail (GTK_IS_WIDGET (im_tool->active_picker));

  identifier = g_object_get_data (G_OBJECT (im_tool->active_picker),
                                  "picker-identifier");

  GIMP_IMAGE_MAP_TOOL_GET_CLASS (im_tool)->color_picked (im_tool,
                                                         identifier,
                                                         x, y,
                                                         sample_format,
                                                         color);
}

static void
gimp_image_map_tool_real_reset (GimpImageMapTool *im_tool)
{
  if (im_tool->config)
    {
      if (im_tool->default_config)
        {
          gimp_config_copy (GIMP_CONFIG (im_tool->default_config),
                            GIMP_CONFIG (im_tool->config),
                            0);
        }
      else
        {
          gimp_config_reset (GIMP_CONFIG (im_tool->config));
        }
    }
}

static void
gimp_image_map_tool_halt (GimpImageMapTool *im_tool)
{
  GimpTool *tool = GIMP_TOOL (im_tool);

  if (im_tool->gui)
    gimp_tool_gui_hide (im_tool->gui);

  if (im_tool->image_map)
    {
      gimp_tool_control_push_preserve (tool->control, TRUE);

      gimp_image_map_abort (im_tool->image_map);
      g_object_unref (im_tool->image_map);
      im_tool->image_map = NULL;

      gimp_tool_control_pop_preserve (tool->control);
    }

  tool->drawable = NULL;
}

static void
gimp_image_map_tool_commit (GimpImageMapTool *im_tool)
{
  GimpTool *tool = GIMP_TOOL (im_tool);

  if (im_tool->gui)
    gimp_tool_gui_hide (im_tool->gui);

  if (im_tool->image_map)
    {
      GimpImageMapOptions *options = GIMP_IMAGE_MAP_TOOL_GET_OPTIONS (tool);

      gimp_tool_control_push_preserve (tool->control, TRUE);

      if (! options->preview)
        gimp_image_map_tool_map (im_tool);

      gimp_image_map_commit (im_tool->image_map, GIMP_PROGRESS (tool), TRUE);
      g_object_unref (im_tool->image_map);
      im_tool->image_map = NULL;

      gimp_tool_control_pop_preserve (tool->control);

      gimp_image_flush (gimp_display_get_image (tool->display));

      if (im_tool->config && im_tool->settings_box)
        {
          GimpGuiConfig *config = GIMP_GUI_CONFIG (tool->tool_info->gimp->config);

          gimp_settings_box_add_current (GIMP_SETTINGS_BOX (im_tool->settings_box),
                                         config->image_map_tool_max_recent);
        }
    }

  tool->display  = NULL;
  tool->drawable = NULL;
}

static void
gimp_image_map_tool_map (GimpImageMapTool *im_tool)
{
  gimp_gegl_config_sync_node (GIMP_OBJECT (im_tool->config),
                              im_tool->operation);

  gimp_image_map_apply (im_tool->image_map, NULL);
}

static void
gimp_image_map_tool_dialog (GimpImageMapTool *im_tool)
{
  GIMP_IMAGE_MAP_TOOL_GET_CLASS (im_tool)->dialog (im_tool);

  g_signal_connect (gimp_tool_gui_get_dialog (im_tool->gui), "unmap",
                    G_CALLBACK (gimp_image_map_tool_dialog_unmap),
                    im_tool);
}

static void
gimp_image_map_tool_dialog_unmap (GtkWidget        *dialog,
                                  GimpImageMapTool *im_tool)
{
  if (im_tool->active_picker)
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (im_tool->active_picker),
                                  FALSE);
}

static void
gimp_image_map_tool_reset (GimpImageMapTool *im_tool)
{
  if (im_tool->config)
    g_object_freeze_notify (im_tool->config);

  GIMP_IMAGE_MAP_TOOL_GET_CLASS (im_tool)->reset (im_tool);

  if (im_tool->config)
    g_object_thaw_notify (im_tool->config);
}

static void
gimp_image_map_tool_create_map (GimpImageMapTool *im_tool)
{
  GimpImageMapOptions *options = GIMP_IMAGE_MAP_TOOL_GET_OPTIONS (im_tool);

  if (im_tool->image_map)
    {
      gimp_image_map_abort (im_tool->image_map);
      g_object_unref (im_tool->image_map);
    }

  g_assert (im_tool->operation);

  im_tool->image_map = gimp_image_map_new (im_tool->drawable,
                                           im_tool->undo_desc,
                                           im_tool->operation,
                                           im_tool->icon_name);

  gimp_image_map_set_region (im_tool->image_map, options->region);

  g_signal_connect (im_tool->image_map, "flush",
                    G_CALLBACK (gimp_image_map_tool_flush),
                    im_tool);
}

static void
gimp_image_map_tool_flush (GimpImageMap     *image_map,
                           GimpImageMapTool *im_tool)
{
  GimpTool  *tool  = GIMP_TOOL (im_tool);
  GimpImage *image = gimp_display_get_image (tool->display);

  gimp_projection_flush (gimp_image_get_projection (image));
}

static void
gimp_image_map_tool_config_notify (GObject          *object,
                                   const GParamSpec *pspec,
                                   GimpImageMapTool *im_tool)
{
  gimp_image_map_tool_preview (im_tool);
}

static void
gimp_image_map_tool_response (GimpToolGui      *gui,
                              gint              response_id,
                              GimpImageMapTool *im_tool)
{
  GimpTool *tool = GIMP_TOOL (im_tool);

  switch (response_id)
    {
    case RESPONSE_RESET:
      gimp_image_map_tool_reset (im_tool);
      gimp_image_map_tool_preview (im_tool);
      break;

    case GTK_RESPONSE_OK:
      gimp_tool_control (tool, GIMP_TOOL_ACTION_COMMIT, tool->display);
      break;

    default:
      gimp_tool_control (tool, GIMP_TOOL_ACTION_HALT, tool->display);
      break;
    }
}

void
gimp_image_map_tool_get_operation (GimpImageMapTool *im_tool)
{
  GimpImageMapToolClass *klass;
  GimpToolInfo          *tool_info;
  gchar                 *operation_name;

  g_return_if_fail (GIMP_IS_IMAGE_MAP_TOOL (im_tool));

  klass = GIMP_IMAGE_MAP_TOOL_GET_CLASS (im_tool);

  tool_info = GIMP_TOOL (im_tool)->tool_info;

  if (im_tool->image_map)
    {
      gimp_image_map_abort (im_tool->image_map);
      g_object_unref (im_tool->image_map);
      im_tool->image_map = NULL;
    }

  if (im_tool->operation)
    {
      g_object_unref (im_tool->operation);
      im_tool->operation = NULL;
    }

  if (im_tool->config)
    {
      g_signal_handlers_disconnect_by_func (im_tool->config,
                                            gimp_image_map_tool_config_notify,
                                            im_tool);

      g_object_unref (im_tool->config);
      im_tool->config = NULL;
    }

  if (im_tool->default_config)
    {
      g_object_unref (im_tool->default_config);
      im_tool->default_config = NULL;
    }

  if (im_tool->title)
    {
      g_free (im_tool->title);
      im_tool->title = NULL;
    }

  if (im_tool->description)
    {
      g_free (im_tool->description);
      im_tool->description = NULL;
    }

  if (im_tool->undo_desc)
    {
      g_free (im_tool->undo_desc);
      im_tool->undo_desc = NULL;
    }

  if (im_tool->icon_name)
    {
      g_free (im_tool->icon_name);
      im_tool->icon_name = NULL;
    }

  if (im_tool->help_id)
    {
      g_free (im_tool->help_id);
      im_tool->help_id = NULL;
    }

  operation_name = klass->get_operation (im_tool,
                                         &im_tool->title,
                                         &im_tool->description,
                                         &im_tool->undo_desc,
                                         &im_tool->icon_name,
                                         &im_tool->help_id);

  if (! operation_name)
    operation_name = g_strdup ("gegl:nop");

  if (! im_tool->title)
    im_tool->title = g_strdup (tool_info->blurb);

  if (! im_tool->description)
    im_tool->description = g_strdup (im_tool->title);

  if (! im_tool->undo_desc)
    im_tool->undo_desc = g_strdup (tool_info->blurb);

  if (! im_tool->icon_name)
    im_tool->icon_name =
      g_strdup (gimp_viewable_get_icon_name (GIMP_VIEWABLE (tool_info)));

  if (! im_tool->help_id)
    im_tool->help_id = g_strdup (tool_info->help_id);

  im_tool->operation = gegl_node_new_child (NULL,
                                            "operation", operation_name,
                                            NULL);
  im_tool->config = G_OBJECT (gimp_gegl_config_new (operation_name,
                                                    im_tool->icon_name,
                                                    GIMP_TYPE_SETTINGS));

  gimp_gegl_config_sync_node (GIMP_OBJECT (im_tool->config),
                              im_tool->operation);

  if (im_tool->gui)
    {
      gimp_tool_gui_set_title       (im_tool->gui, im_tool->title);
      gimp_tool_gui_set_description (im_tool->gui, im_tool->description);
      gimp_tool_gui_set_icon_name   (im_tool->gui, im_tool->icon_name);
      gimp_tool_gui_set_help_id     (im_tool->gui, im_tool->help_id);
    }

  if (gegl_operation_get_key (operation_name, "position-dependent"))
    {
      if (im_tool->gui)
        gtk_widget_show (im_tool->region_combo);
    }
  else
    {
      if (im_tool->gui)
        gtk_widget_hide (im_tool->region_combo);

      g_object_set (GIMP_IMAGE_MAP_TOOL_GET_OPTIONS (im_tool),
                    "region", GIMP_IMAGE_MAP_REGION_SELECTION,
                    NULL);
    }

  g_free (operation_name);

  if (im_tool->config)
    g_signal_connect_object (im_tool->config, "notify",
                             G_CALLBACK (gimp_image_map_tool_config_notify),
                             G_OBJECT (im_tool), 0);

  if (GIMP_TOOL (im_tool)->drawable)
    gimp_image_map_tool_create_map (im_tool);
}

void
gimp_image_map_tool_preview (GimpImageMapTool *im_tool)
{
  GimpTool            *tool;
  GimpImageMapOptions *options;

  g_return_if_fail (GIMP_IS_IMAGE_MAP_TOOL (im_tool));

  tool    = GIMP_TOOL (im_tool);
  options = GIMP_IMAGE_MAP_TOOL_GET_OPTIONS (tool);

  if (im_tool->image_map && options->preview)
    {
      gimp_tool_control_push_preserve (tool->control, TRUE);

      gimp_image_map_tool_map (im_tool);

      gimp_tool_control_pop_preserve (tool->control);
    }
}

void
gimp_image_map_tool_edit_as (GimpImageMapTool *im_tool,
                             const gchar      *new_tool_id,
                             GimpConfig       *config)
{
  GimpDisplay  *display;
  GimpContext  *user_context;
  GimpToolInfo *tool_info;
  GimpTool     *new_tool;

  g_return_if_fail (GIMP_IS_IMAGE_MAP_TOOL (im_tool));
  g_return_if_fail (new_tool_id != NULL);
  g_return_if_fail (GIMP_IS_CONFIG (config));

  display = GIMP_TOOL (im_tool)->display;

  user_context = gimp_get_user_context (display->gimp);

  tool_info = (GimpToolInfo *)
    gimp_container_get_child_by_name (display->gimp->tool_info_list,
                                      new_tool_id);

  gimp_tool_control (GIMP_TOOL (im_tool), GIMP_TOOL_ACTION_HALT, display);
  gimp_context_set_tool (user_context, tool_info);
  tool_manager_initialize_active (display->gimp, display);

  new_tool = tool_manager_get_active (display->gimp);

  GIMP_IMAGE_MAP_TOOL (new_tool)->default_config = g_object_ref (config);

  gimp_image_map_tool_reset (GIMP_IMAGE_MAP_TOOL (new_tool));
}

GtkWidget *
gimp_image_map_tool_dialog_get_vbox (GimpImageMapTool *im_tool)
{
  g_return_val_if_fail (GIMP_IS_IMAGE_MAP_TOOL (im_tool), NULL);

  return gimp_tool_gui_get_vbox (im_tool->gui);
}

static void
gimp_image_map_tool_color_picker_toggled (GtkWidget        *widget,
                                          GimpImageMapTool *im_tool)
{
  if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (widget)))
    {
      if (im_tool->active_picker == widget)
        return;

      if (im_tool->active_picker)
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (im_tool->active_picker),
                                      FALSE);

      im_tool->active_picker = widget;

      gimp_color_tool_enable (GIMP_COLOR_TOOL (im_tool),
                              GIMP_COLOR_TOOL_GET_OPTIONS (im_tool));
    }
  else if (im_tool->active_picker == widget)
    {
      im_tool->active_picker = NULL;
      gimp_color_tool_disable (GIMP_COLOR_TOOL (im_tool));
    }
}

GtkWidget *
gimp_image_map_tool_add_color_picker (GimpImageMapTool *im_tool,
                                      gpointer          identifier,
                                      const gchar      *icon_name,
                                      const gchar      *tooltip)
{
  GtkWidget *button;
  GtkWidget *image;

  g_return_val_if_fail (GIMP_IS_IMAGE_MAP_TOOL (im_tool), NULL);
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
                    G_CALLBACK (gimp_image_map_tool_color_picker_toggled),
                    im_tool);

  return button;
}
