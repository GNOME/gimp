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

#include "core/gimp.h"
#include "core/gimpdrawable.h"
#include "core/gimperror.h"
#include "core/gimpimage.h"
#include "core/gimpimage-pick-color.h"
#include "core/gimpimagemap.h"
#include "core/gimpimagemapconfig.h"
#include "core/gimplist.h"
#include "core/gimppickable.h"
#include "core/gimpprojection.h"
#include "core/gimptoolinfo.h"

#include "widgets/gimpdialogfactory.h"
#include "widgets/gimpoverlaybox.h"
#include "widgets/gimpoverlaydialog.h"
#include "widgets/gimpsettingsbox.h"
#include "widgets/gimpwidgets-utils.h"

#include "display/gimpdisplay.h"
#include "display/gimpdisplayshell.h"
#include "display/gimpdisplayshell-transform.h"
#include "display/gimptooldialog.h"

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
                                                     GimpImageType    *sample_type,
                                                     GimpRGB          *color,
                                                     gint             *color_index);
static void      gimp_image_map_tool_map            (GimpImageMapTool *im_tool);
static void      gimp_image_map_tool_dialog         (GimpImageMapTool *im_tool);
static void      gimp_image_map_tool_reset          (GimpImageMapTool *im_tool);

static void      gimp_image_map_tool_flush          (GimpImageMap     *image_map,
                                                     GimpImageMapTool *im_tool);

static void      gimp_image_map_tool_response       (GtkWidget        *widget,
                                                     gint              response_id,
                                                     GimpImageMapTool *im_tool);

static void      gimp_image_map_tool_dialog_hide    (GimpImageMapTool *im_tool);
static void      gimp_image_map_tool_dialog_destroy (GimpImageMapTool *im_tool);

static void      gimp_image_map_tool_gegl_notify    (GObject          *config,
                                                     const GParamSpec *pspec,
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

  klass->dialog_desc         = NULL;
  klass->settings_name       = NULL;
  klass->import_dialog_title = NULL;
  klass->export_dialog_title = NULL;

  klass->get_operation       = NULL;
  klass->map                 = NULL;
  klass->dialog              = NULL;
  klass->reset               = NULL;
  klass->settings_import     = gimp_image_map_tool_real_settings_import;
  klass->settings_export     = gimp_image_map_tool_real_settings_export;
}

static void
gimp_image_map_tool_base_init (GimpImageMapToolClass *klass)
{
  klass->recent_settings = gimp_list_new (GIMP_TYPE_IMAGE_MAP_CONFIG, TRUE);
  gimp_list_set_sort_func (GIMP_LIST (klass->recent_settings),
                           (GCompareFunc) gimp_image_map_config_compare);
}

static void
gimp_image_map_tool_init (GimpImageMapTool *image_map_tool)
{
  GimpTool *tool = GIMP_TOOL (image_map_tool);

  gimp_tool_control_set_scroll_lock (tool->control, TRUE);
  gimp_tool_control_set_preserve    (tool->control, FALSE);
  gimp_tool_control_set_dirty_mask  (tool->control,
                                     GIMP_DIRTY_IMAGE           |
                                     GIMP_DIRTY_IMAGE_STRUCTURE |
                                     GIMP_DIRTY_DRAWABLE        |
                                     GIMP_DIRTY_SELECTION       |
                                     GIMP_DIRTY_ACTIVE_DRAWABLE);

  image_map_tool->drawable       = NULL;
  image_map_tool->operation      = NULL;
  image_map_tool->config         = NULL;
  image_map_tool->default_config = NULL;
  image_map_tool->image_map      = NULL;

  image_map_tool->dialog         = NULL;
  image_map_tool->main_vbox      = NULL;
  image_map_tool->settings_box   = NULL;
  image_map_tool->label_group    = NULL;
}

static void
gimp_image_map_tool_constructed (GObject *object)
{
  GimpImageMapTool      *image_map_tool = GIMP_IMAGE_MAP_TOOL (object);
  GimpImageMapToolClass *klass;

  if (G_OBJECT_CLASS (parent_class)->constructed)
    G_OBJECT_CLASS (parent_class)->constructed (object);

  klass = GIMP_IMAGE_MAP_TOOL_GET_CLASS (image_map_tool);

  if (klass->get_operation)
    image_map_tool->operation = klass->get_operation (image_map_tool,
                                                      &image_map_tool->config);
}

static void
gimp_image_map_tool_finalize (GObject *object)
{
  GimpImageMapTool *image_map_tool = GIMP_IMAGE_MAP_TOOL (object);

  if (image_map_tool->operation)
    {
      g_object_unref (image_map_tool->operation);
      image_map_tool->operation = NULL;
    }

  if (image_map_tool->config)
    {
      g_object_unref (image_map_tool->config);
      image_map_tool->config = NULL;
    }

  if (image_map_tool->default_config)
    {
      g_object_unref (image_map_tool->default_config);
      image_map_tool->default_config = NULL;
    }

  if (image_map_tool->dialog)
    gimp_image_map_tool_dialog_destroy (image_map_tool);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

#define RESPONSE_RESET 1

static gboolean
gimp_image_map_tool_initialize (GimpTool     *tool,
                                GimpDisplay  *display,
                                GError      **error)
{
  GimpImageMapTool *image_map_tool = GIMP_IMAGE_MAP_TOOL (tool);
  GimpToolInfo     *tool_info      = tool->tool_info;
  GimpImage        *image          = gimp_display_get_image (display);
  GimpDrawable     *drawable       = gimp_image_get_active_drawable (image);
  GimpDisplayShell *display_shell  = gimp_display_get_shell (display);

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

  /*  set display so the dialog can be hidden on display destruction  */
  tool->display = display;

  if (! image_map_tool->dialog)
    {
      GimpImageMapToolClass *klass;
      GtkWidget             *dialog;
      GtkWidget             *vbox;
      GtkWidget             *toggle;

      klass = GIMP_IMAGE_MAP_TOOL_GET_CLASS (image_map_tool);

      /*  disabled for at least GIMP 2.8  */
      image_map_tool->overlay = FALSE;

      if (image_map_tool->overlay)
        {
          image_map_tool->dialog = dialog =
            gimp_overlay_dialog_new (tool_info,
                                     klass->dialog_desc,

                                     GIMP_STOCK_RESET, RESPONSE_RESET,
                                     GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                                     GTK_STOCK_OK,     GTK_RESPONSE_OK,

                                     NULL);

          gtk_container_set_border_width (GTK_CONTAINER (dialog), 6);

          gimp_overlay_box_add_child (GIMP_OVERLAY_BOX (display_shell->canvas),
                                      dialog, 1.0, 1.0);
          gimp_overlay_box_set_child_angle (GIMP_OVERLAY_BOX (display_shell->canvas),
                                            dialog, 0.0);

          image_map_tool->main_vbox = vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 6);
          gtk_container_add (GTK_CONTAINER (dialog), vbox);
        }
      else
        {
          image_map_tool->dialog = dialog =
            gimp_tool_dialog_new (tool_info,
                                  display_shell,
                                  klass->dialog_desc,

                                  GIMP_STOCK_RESET, RESPONSE_RESET,
                                  GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                                  GTK_STOCK_OK,     GTK_RESPONSE_OK,

                                  NULL);

          gtk_dialog_set_alternative_button_order (GTK_DIALOG (dialog),
                                                   RESPONSE_RESET,
                                                   GTK_RESPONSE_OK,
                                                   GTK_RESPONSE_CANCEL,
                                                   -1);

          image_map_tool->main_vbox = vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 6);
          gtk_container_set_border_width (GTK_CONTAINER (vbox), 6);
          gtk_box_pack_start (GTK_BOX (gtk_dialog_get_content_area (GTK_DIALOG (dialog))),
                              vbox, TRUE, TRUE, 0);
        }

      g_signal_connect_object (dialog, "response",
                               G_CALLBACK (gimp_image_map_tool_response),
                               G_OBJECT (image_map_tool), 0);

      if (klass->settings_name)
        gimp_image_map_tool_add_settings_gui (image_map_tool);

      /*  The preview toggle  */
      toggle = gimp_prop_check_button_new (G_OBJECT (tool_info->tool_options),
                                           "preview",
                                           _("_Preview"));

      gtk_box_pack_end (GTK_BOX (image_map_tool->main_vbox), toggle,
                        FALSE, FALSE, 0);
      gtk_widget_show (toggle);

      /*  Fill in subclass widgets  */
      gimp_image_map_tool_dialog (image_map_tool);

      gtk_widget_show (vbox);

      if (image_map_tool->operation)
        g_signal_connect_object (tool_info->gimp->config, "notify::use-gegl",
                                 G_CALLBACK (gimp_image_map_tool_gegl_notify),
                                 image_map_tool, 0);
    }
  else if (GIMP_IS_OVERLAY_DIALOG (image_map_tool->dialog) &&
           ! gtk_widget_get_parent (image_map_tool->dialog))
    {
      gimp_overlay_box_add_child (GIMP_OVERLAY_BOX (display_shell->canvas),
                                  image_map_tool->dialog, 1.0, 1.0);
      g_object_unref (image_map_tool->dialog);
    }

  if (! image_map_tool->overlay)
    {
      gimp_viewable_dialog_set_viewable (GIMP_VIEWABLE_DIALOG (image_map_tool->dialog),
                                         GIMP_VIEWABLE (drawable),
                                         GIMP_CONTEXT (tool_info->tool_options));
      gimp_tool_dialog_set_shell (GIMP_TOOL_DIALOG (image_map_tool->dialog),
                                  display_shell);
    }

  gtk_widget_show (image_map_tool->dialog);

  image_map_tool->drawable = drawable;

  gimp_image_map_tool_create_map (image_map_tool);

  return TRUE;
}

static void
gimp_image_map_tool_control (GimpTool       *tool,
                             GimpToolAction  action,
                             GimpDisplay    *display)
{
  GimpImageMapTool *image_map_tool = GIMP_IMAGE_MAP_TOOL (tool);

  switch (action)
    {
    case GIMP_TOOL_ACTION_PAUSE:
    case GIMP_TOOL_ACTION_RESUME:
      break;

    case GIMP_TOOL_ACTION_HALT:
      gimp_image_map_tool_dialog_hide (image_map_tool);

      if (image_map_tool->image_map)
        {
          GimpImage *image;

          gimp_tool_control_push_preserve (tool->control, TRUE);

          gimp_image_map_abort (image_map_tool->image_map);
          g_object_unref (image_map_tool->image_map);
          image_map_tool->image_map = NULL;

          gimp_tool_control_pop_preserve (tool->control);

          /* don't call gimp_image_flush() here, because the tool
           * might be cancelled from some other place opening an undo
           * group, so flushing the image would update menus and
           * whatnot while that other operation is running, with
           * unforeseeable side effects. Also, flusing the image here
           * is not needed because we didn't change anything in the
           * image. Instead, make sure manually that the display is
           * updated correctly after restoring GimpImageMapTool's
           * temporary editing.
           */
          image = gimp_display_get_image (tool->display);
          gimp_projection_flush_now (gimp_image_get_projection (image));
          gimp_display_flush_now (tool->display);
        }

      tool->drawable = NULL;
      break;
    }

  GIMP_TOOL_CLASS (parent_class)->control (tool, action, display);
}

static gboolean
gimp_image_map_tool_key_press (GimpTool    *tool,
                               GdkEventKey *kevent,
                               GimpDisplay *display)
{
  GimpImageMapTool *image_map_tool = GIMP_IMAGE_MAP_TOOL (tool);

  if (image_map_tool->dialog && display == tool->display)
    {
      switch (kevent->keyval)
        {
        case GDK_KEY_Return:
        case GDK_KEY_KP_Enter:
        case GDK_KEY_ISO_Enter:
          gimp_image_map_tool_response (image_map_tool->dialog,
                                        GTK_RESPONSE_OK,
                                        image_map_tool);
          return TRUE;

        case GDK_KEY_BackSpace:
          gimp_image_map_tool_response (image_map_tool->dialog,
                                        RESPONSE_RESET,
                                        image_map_tool);
          return TRUE;

        case GDK_KEY_Escape:
          gimp_image_map_tool_response (image_map_tool->dialog,
                                        GTK_RESPONSE_CANCEL,
                                        image_map_tool);
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
  GimpImageMapTool *image_map_tool = GIMP_IMAGE_MAP_TOOL (tool);

  if (! strcmp (pspec->name, "preview") &&
      image_map_tool->image_map)
    {
      GimpImageMapOptions *im_options = GIMP_IMAGE_MAP_OPTIONS (options);

      if (im_options->preview)
        {
          gimp_tool_control_push_preserve (tool->control, TRUE);

          gimp_image_map_tool_map (image_map_tool);

          gimp_tool_control_pop_preserve (tool->control);
        }
      else
        {
          gimp_tool_control_push_preserve (tool->control, TRUE);

          gimp_image_map_clear (image_map_tool->image_map);

          gimp_tool_control_pop_preserve (tool->control);

          gimp_image_map_tool_flush (image_map_tool->image_map,
                                     image_map_tool);
        }
    }
}

static gboolean
gimp_image_map_tool_pick_color (GimpColorTool *color_tool,
                                gint           x,
                                gint           y,
                                GimpImageType *sample_type,
                                GimpRGB       *color,
                                gint          *color_index)
{
  GimpImageMapTool *tool = GIMP_IMAGE_MAP_TOOL (color_tool);
  gint              off_x, off_y;

  gimp_item_get_offset (GIMP_ITEM (tool->drawable), &off_x, &off_y);

  *sample_type = gimp_drawable_type (tool->drawable);

  return gimp_pickable_pick_color (GIMP_PICKABLE (tool->image_map),
                                   x - off_x,
                                   y - off_y,
                                   color_tool->options->sample_average,
                                   color_tool->options->average_radius,
                                   color, color_index);
}

static void
gimp_image_map_tool_map (GimpImageMapTool *tool)
{
  GimpDisplayShell *shell = gimp_display_get_shell (GIMP_TOOL (tool)->display);
  GimpItem         *item  = GIMP_ITEM (tool->drawable);
  gint              x, y;
  gint              w, h;
  gint              off_x, off_y;
  GeglRectangle     visible;

  GIMP_IMAGE_MAP_TOOL_GET_CLASS (tool)->map (tool);

  gimp_display_shell_untransform_viewport (shell, &x, &y, &w, &h);

  gimp_item_get_offset (item, &off_x, &off_y);

  gimp_rectangle_intersect (x, y, w, h,
                            off_x,
                            off_y,
                            gimp_item_get_width  (item),
                            gimp_item_get_height (item),
                            &visible.x,
                            &visible.y,
                            &visible.width,
                            &visible.height);

  visible.x -= off_x;
  visible.y -= off_y;

  gimp_image_map_apply (tool->image_map, &visible);
}

static void
gimp_image_map_tool_dialog (GimpImageMapTool *tool)
{
  GIMP_IMAGE_MAP_TOOL_GET_CLASS (tool)->dialog (tool);
}

static void
gimp_image_map_tool_reset (GimpImageMapTool *tool)
{
  if (GIMP_IMAGE_MAP_TOOL_GET_CLASS (tool)->reset)
    {
      GIMP_IMAGE_MAP_TOOL_GET_CLASS (tool)->reset (tool);
    }
  else if (tool->config)
    {
      if (tool->default_config)
        {
          gimp_config_copy (GIMP_CONFIG (tool->default_config),
                            GIMP_CONFIG (tool->config),
                            0);
        }
      else
        {
          gimp_config_reset (GIMP_CONFIG (tool->config));
        }
    }
}

void
gimp_image_map_tool_create_map (GimpImageMapTool *tool)
{
  Gimp     *gimp;
  gboolean  use_gegl;

  g_return_if_fail (GIMP_IS_IMAGE_MAP_TOOL (tool));

  gimp = GIMP_TOOL (tool)->tool_info->gimp;

  if (tool->image_map)
    {
      gimp_image_map_clear (tool->image_map);
      g_object_unref (tool->image_map);
    }

  g_assert (tool->operation || tool->apply_func);

  use_gegl = gimp_use_gegl (gimp) || ! tool->apply_func;

  tool->image_map = gimp_image_map_new (tool->drawable,
                                        GIMP_TOOL (tool)->tool_info->blurb,
                                        use_gegl ? tool->operation : NULL,
                                        tool->apply_func,
                                        tool->apply_data);

  g_signal_connect (tool->image_map, "flush",
                    G_CALLBACK (gimp_image_map_tool_flush),
                    tool);
}

static void
gimp_image_map_tool_flush (GimpImageMap     *image_map,
                           GimpImageMapTool *image_map_tool)
{
  GimpTool  *tool  = GIMP_TOOL (image_map_tool);
  GimpImage *image = gimp_display_get_image (tool->display);

  gimp_projection_flush_now (gimp_image_get_projection (image));
  gimp_display_flush_now (tool->display);
}

static void
gimp_image_map_tool_response (GtkWidget        *widget,
                              gint              response_id,
                              GimpImageMapTool *image_map_tool)
{
  GimpTool *tool = GIMP_TOOL (image_map_tool);

  switch (response_id)
    {
    case RESPONSE_RESET:
      gimp_image_map_tool_reset (image_map_tool);
      gimp_image_map_tool_preview (image_map_tool);
      break;

    case GTK_RESPONSE_OK:
      gimp_image_map_tool_dialog_hide (image_map_tool);

      if (image_map_tool->image_map)
        {
          GimpImageMapOptions *options = GIMP_IMAGE_MAP_TOOL_GET_OPTIONS (tool);

          gimp_tool_control_push_preserve (tool->control, TRUE);

          if (! options->preview)
            gimp_image_map_tool_map (image_map_tool);

          gimp_image_map_commit (image_map_tool->image_map);
          g_object_unref (image_map_tool->image_map);
          image_map_tool->image_map = NULL;

          gimp_tool_control_pop_preserve (tool->control);

          gimp_image_flush (gimp_display_get_image (tool->display));

          if (image_map_tool->config && image_map_tool->settings_box)
            gimp_settings_box_add_current (GIMP_SETTINGS_BOX (image_map_tool->settings_box),
                                           GIMP_GUI_CONFIG (tool->tool_info->gimp->config)->image_map_tool_max_recent);
        }

      tool->display  = NULL;
      tool->drawable = NULL;
      break;

    default:
      gimp_tool_control (tool, GIMP_TOOL_ACTION_HALT, tool->display);
      break;
    }
}

static void
gimp_image_map_tool_dialog_hide (GimpImageMapTool *image_map_tool)
{
  GtkWidget *dialog = image_map_tool->dialog;

  if (GTK_IS_DIALOG (dialog))
    {
      gimp_dialog_factory_hide_dialog (dialog);
    }
  else if (GIMP_IS_OVERLAY_DIALOG (dialog))
    {
      g_object_ref (dialog);
      gtk_container_remove (GTK_CONTAINER (gtk_widget_get_parent (dialog)),
                            dialog);
    }
}

static void
gimp_image_map_tool_dialog_destroy (GimpImageMapTool *image_map_tool)
{
  if (GTK_IS_DIALOG (image_map_tool->dialog) ||
      gtk_widget_get_parent (image_map_tool->dialog))
    gtk_widget_destroy (image_map_tool->dialog);
  else
    g_object_unref (image_map_tool->dialog);

  image_map_tool->dialog       = NULL;
  image_map_tool->main_vbox    = NULL;
  image_map_tool->settings_box = NULL;
  image_map_tool->label_group  = NULL;
}

void
gimp_image_map_tool_preview (GimpImageMapTool *image_map_tool)
{
  GimpTool            *tool;
  GimpImageMapOptions *options;

  g_return_if_fail (GIMP_IS_IMAGE_MAP_TOOL (image_map_tool));

  tool    = GIMP_TOOL (image_map_tool);
  options = GIMP_IMAGE_MAP_TOOL_GET_OPTIONS (tool);

  if (image_map_tool->image_map && options->preview)
    {
      gimp_tool_control_push_preserve (tool->control, TRUE);

      gimp_image_map_tool_map (image_map_tool);

      gimp_tool_control_pop_preserve (tool->control);
    }
}

static void
gimp_image_map_tool_gegl_notify (GObject          *config,
                                 const GParamSpec *pspec,
                                 GimpImageMapTool *im_tool)
{
  if (im_tool->image_map)
    {
      gimp_tool_control_push_preserve (GIMP_TOOL (im_tool)->control, TRUE);

      gimp_image_map_tool_create_map (im_tool);

      gimp_tool_control_pop_preserve (GIMP_TOOL (im_tool)->control);

      gimp_image_map_tool_preview (im_tool);
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
  g_return_if_fail (new_tool_id);
  g_return_if_fail (GIMP_IS_CONFIG (config));

  display = GIMP_TOOL (im_tool)->display;

  user_context = gimp_get_user_context (display->gimp);

  tool_info = (GimpToolInfo *)
    gimp_container_get_child_by_name (display->gimp->tool_info_list,
                                      new_tool_id);

  gimp_context_set_tool (user_context, tool_info);
  tool_manager_initialize_active (display->gimp, display);

  new_tool = tool_manager_get_active (display->gimp);

  GIMP_IMAGE_MAP_TOOL (new_tool)->default_config = g_object_ref (config);

  gimp_image_map_tool_reset (GIMP_IMAGE_MAP_TOOL (new_tool));
}

GtkWidget *
gimp_image_map_tool_dialog_get_vbox (GimpImageMapTool *tool)
{
  g_return_val_if_fail (GIMP_IS_IMAGE_MAP_TOOL (tool), NULL);

  return tool->main_vbox;
}


GtkSizeGroup *
gimp_image_map_tool_dialog_get_label_group (GimpImageMapTool *tool)
{
  g_return_val_if_fail (GIMP_IS_IMAGE_MAP_TOOL (tool), NULL);

  return tool->label_group;
}
