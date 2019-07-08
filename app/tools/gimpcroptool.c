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

#include "config.h"

#include <gegl.h>
#include <gtk/gtk.h>

#include "libgimpwidgets/gimpwidgets.h"

#include "tools-types.h"

#include "core/gimp.h"
#include "core/gimpimage.h"
#include "core/gimpimage-crop.h"
#include "core/gimpitem.h"
#include "core/gimptoolinfo.h"

#include "widgets/gimphelp-ids.h"

#include "display/gimpdisplay.h"
#include "display/gimpdisplayshell.h"
#include "display/gimptoolrectangle.h"

#include "gimpcropoptions.h"
#include "gimpcroptool.h"
#include "gimprectangleoptions.h"
#include "gimptoolcontrol.h"
#include "gimptools-utils.h"

#include "gimp-intl.h"


static void      gimp_crop_tool_constructed                (GObject              *object);
static void      gimp_crop_tool_dispose                    (GObject              *object);

static void      gimp_crop_tool_control                    (GimpTool             *tool,
                                                            GimpToolAction        action,
                                                            GimpDisplay          *display);
static void      gimp_crop_tool_button_press               (GimpTool             *tool,
                                                            const GimpCoords     *coords,
                                                            guint32               time,
                                                            GdkModifierType       state,
                                                            GimpButtonPressType   press_type,
                                                            GimpDisplay          *display);
static void      gimp_crop_tool_button_release             (GimpTool             *tool,
                                                            const GimpCoords     *coords,
                                                            guint32               time,
                                                            GdkModifierType       state,
                                                            GimpButtonReleaseType release_type,
                                                            GimpDisplay          *display);
static void     gimp_crop_tool_motion                      (GimpTool             *tool,
                                                            const GimpCoords     *coords,
                                                            guint32               time,
                                                            GdkModifierType       state,
                                                            GimpDisplay          *display);
static void      gimp_crop_tool_options_notify             (GimpTool             *tool,
                                                            GimpToolOptions      *options,
                                                            const GParamSpec     *pspec);

static void      gimp_crop_tool_rectangle_changed          (GimpToolWidget       *rectangle,
                                                            GimpCropTool         *crop_tool);
static void      gimp_crop_tool_rectangle_response         (GimpToolWidget       *rectangle,
                                                            gint                  response_id,
                                                            GimpCropTool         *crop_tool);
static void      gimp_crop_tool_rectangle_change_complete  (GimpToolRectangle    *rectangle,
                                                            GimpCropTool         *crop_tool);

static void      gimp_crop_tool_start                      (GimpCropTool         *crop_tool,
                                                            GimpDisplay          *display);
static void      gimp_crop_tool_commit                     (GimpCropTool         *crop_tool);
static void      gimp_crop_tool_halt                       (GimpCropTool         *crop_tool);

static void      gimp_crop_tool_update_option_defaults     (GimpCropTool         *crop_tool,
                                                            gboolean              ignore_pending);
static GimpRectangleConstraint
                 gimp_crop_tool_get_constraint             (GimpCropTool         *crop_tool);

static void      gimp_crop_tool_image_changed              (GimpCropTool         *crop_tool,
                                                            GimpImage            *image,
                                                            GimpContext          *context);
static void      gimp_crop_tool_image_size_changed         (GimpCropTool         *crop_tool);
static void      gimp_crop_tool_image_active_layer_changed (GimpCropTool         *crop_tool);
static void      gimp_crop_tool_layer_size_changed         (GimpCropTool         *crop_tool);

static void      gimp_crop_tool_auto_shrink                (GimpCropTool         *crop_tool);


G_DEFINE_TYPE (GimpCropTool, gimp_crop_tool, GIMP_TYPE_DRAW_TOOL)

#define parent_class gimp_crop_tool_parent_class


/*  public functions  */

void
gimp_crop_tool_register (GimpToolRegisterCallback  callback,
                         gpointer                  data)
{
  (* callback) (GIMP_TYPE_CROP_TOOL,
                GIMP_TYPE_CROP_OPTIONS,
                gimp_crop_options_gui,
                0,
                "gimp-crop-tool",
                _("Crop"),
                _("Crop Tool: Remove edge areas from image or layer"),
                N_("_Crop"), "<shift>C",
                NULL, GIMP_HELP_TOOL_CROP,
                GIMP_ICON_TOOL_CROP,
                data);
}

static void
gimp_crop_tool_class_init (GimpCropToolClass *klass)
{
  GObjectClass  *object_class = G_OBJECT_CLASS (klass);
  GimpToolClass *tool_class   = GIMP_TOOL_CLASS (klass);

  object_class->constructed  = gimp_crop_tool_constructed;
  object_class->dispose      = gimp_crop_tool_dispose;

  tool_class->control        = gimp_crop_tool_control;
  tool_class->button_press   = gimp_crop_tool_button_press;
  tool_class->button_release = gimp_crop_tool_button_release;
  tool_class->motion         = gimp_crop_tool_motion;
  tool_class->options_notify = gimp_crop_tool_options_notify;
}

static void
gimp_crop_tool_init (GimpCropTool *crop_tool)
{
  GimpTool *tool = GIMP_TOOL (crop_tool);

  gimp_tool_control_set_wants_click      (tool->control, TRUE);
  gimp_tool_control_set_active_modifiers (tool->control,
                                          GIMP_TOOL_ACTIVE_MODIFIERS_SEPARATE);
  gimp_tool_control_set_precision        (tool->control,
                                          GIMP_CURSOR_PRECISION_PIXEL_BORDER);
  gimp_tool_control_set_cursor           (tool->control,
                                          GIMP_CURSOR_CROSSHAIR_SMALL);
  gimp_tool_control_set_tool_cursor      (tool->control,
                                          GIMP_TOOL_CURSOR_CROP);

  gimp_draw_tool_set_default_status (GIMP_DRAW_TOOL (tool),
                                     _("Click-Drag to draw a crop rectangle"));
}

static void
gimp_crop_tool_constructed (GObject *object)
{
  GimpCropTool    *crop_tool = GIMP_CROP_TOOL (object);
  GimpContext     *context;
  GimpToolInfo    *tool_info;

  G_OBJECT_CLASS (parent_class)->constructed (object);

  tool_info = GIMP_TOOL (crop_tool)->tool_info;

  context = gimp_get_user_context (tool_info->gimp);

  g_signal_connect_object (context, "image-changed",
                           G_CALLBACK (gimp_crop_tool_image_changed),
                           crop_tool,
                           G_CONNECT_SWAPPED);

  /* Make sure we are connected to "size-changed" for the initial
   * image.
   */
  gimp_crop_tool_image_changed (crop_tool,
                                gimp_context_get_image (context),
                                context);
}

static void
gimp_crop_tool_dispose (GObject *object)
{
  GimpCropTool *crop_tool = GIMP_CROP_TOOL (object);

  /* Clean up current_image and current_layer. */
  gimp_crop_tool_image_changed (crop_tool, NULL, NULL);

  G_OBJECT_CLASS (parent_class)->dispose (object);
}

static void
gimp_crop_tool_control (GimpTool       *tool,
                        GimpToolAction  action,
                        GimpDisplay    *display)
{
  GimpCropTool *crop_tool = GIMP_CROP_TOOL (tool);

  switch (action)
    {
    case GIMP_TOOL_ACTION_PAUSE:
    case GIMP_TOOL_ACTION_RESUME:
      break;

    case GIMP_TOOL_ACTION_HALT:
      gimp_crop_tool_halt (crop_tool);
      break;

    case GIMP_TOOL_ACTION_COMMIT:
      gimp_crop_tool_commit (crop_tool);
      break;
    }

  GIMP_TOOL_CLASS (parent_class)->control (tool, action, display);
}

static void
gimp_crop_tool_button_press (GimpTool            *tool,
                             const GimpCoords    *coords,
                             guint32              time,
                             GdkModifierType      state,
                             GimpButtonPressType  press_type,
                             GimpDisplay         *display)
{
  GimpCropTool *crop_tool = GIMP_CROP_TOOL (tool);

  if (tool->display && display != tool->display)
    gimp_tool_control (tool, GIMP_TOOL_ACTION_HALT, tool->display);

  if (! tool->display)
    {
      gimp_crop_tool_start (crop_tool, display);

      gimp_tool_widget_hover (crop_tool->widget, coords, state, TRUE);

      /* HACK: force CREATING on a newly created rectangle; otherwise,
       * property bindings would cause the rectangle to start with the
       * size from tool options.
       */
      gimp_tool_rectangle_set_function (GIMP_TOOL_RECTANGLE (crop_tool->widget),
                                        GIMP_TOOL_RECTANGLE_CREATING);
    }

  if (gimp_tool_widget_button_press (crop_tool->widget, coords, time, state,
                                     press_type))
    {
      crop_tool->grab_widget = crop_tool->widget;
    }

  gimp_tool_control_activate (tool->control);
}

static void
gimp_crop_tool_button_release (GimpTool              *tool,
                               const GimpCoords      *coords,
                               guint32                time,
                               GdkModifierType        state,
                               GimpButtonReleaseType  release_type,
                               GimpDisplay           *display)
{
  GimpCropTool *crop_tool = GIMP_CROP_TOOL (tool);

  gimp_tool_control_halt (tool->control);

  if (crop_tool->grab_widget)
    {
      gimp_tool_widget_button_release (crop_tool->grab_widget,
                                       coords, time, state, release_type);
      crop_tool->grab_widget = NULL;
    }

  gimp_tool_push_status (tool, display, _("Click or press Enter to crop"));
}

static void
gimp_crop_tool_motion (GimpTool         *tool,
                       const GimpCoords *coords,
                       guint32           time,
                       GdkModifierType   state,
                       GimpDisplay      *display)
{
  GimpCropTool *crop_tool = GIMP_CROP_TOOL (tool);

  if (crop_tool->grab_widget)
    {
      gimp_tool_widget_motion (crop_tool->grab_widget, coords, time, state);
    }
}

static void
gimp_crop_tool_options_notify (GimpTool         *tool,
                               GimpToolOptions  *options,
                               const GParamSpec *pspec)
{
  GimpCropTool *crop_tool = GIMP_CROP_TOOL (tool);

  if (! strcmp (pspec->name, "layer-only") ||
      ! strcmp (pspec->name, "allow-growing"))
    {
      if (crop_tool->widget)
        {
          gimp_tool_rectangle_set_constraint (GIMP_TOOL_RECTANGLE (crop_tool->widget),
                                              gimp_crop_tool_get_constraint (crop_tool));
        }
      else
        {
          gimp_crop_tool_update_option_defaults (crop_tool, FALSE);
        }
    }
}

static void
gimp_crop_tool_rectangle_changed (GimpToolWidget *rectangle,
                                  GimpCropTool   *crop_tool)
{
}

static void
gimp_crop_tool_rectangle_response (GimpToolWidget *rectangle,
                                   gint            response_id,
                                   GimpCropTool   *crop_tool)
{
  GimpTool *tool = GIMP_TOOL (crop_tool);

  switch (response_id)
    {
    case GIMP_TOOL_WIDGET_RESPONSE_CONFIRM:
      gimp_tool_control (tool, GIMP_TOOL_ACTION_COMMIT, tool->display);
      break;

    case GIMP_TOOL_WIDGET_RESPONSE_CANCEL:
      gimp_tool_control (tool, GIMP_TOOL_ACTION_HALT, tool->display);
      break;
    }
}

static void
gimp_crop_tool_rectangle_change_complete (GimpToolRectangle *rectangle,
                                          GimpCropTool      *crop_tool)
{
  gimp_crop_tool_update_option_defaults (crop_tool, FALSE);
}

static void
gimp_crop_tool_start (GimpCropTool *crop_tool,
                      GimpDisplay  *display)
{
  static const gchar *properties[] =
  {
    "highlight",
    "highlight-opacity",
    "guide",
    "x",
    "y",
    "width",
    "height",
    "fixed-rule-active",
    "fixed-rule",
    "desired-fixed-width",
    "desired-fixed-height",
    "desired-fixed-size-width",
    "desired-fixed-size-height",
    "aspect-numerator",
    "aspect-denominator",
    "fixed-center"
  };

  GimpTool         *tool    = GIMP_TOOL (crop_tool);
  GimpDisplayShell *shell   = gimp_display_get_shell (display);
  GimpCropOptions  *options = GIMP_CROP_TOOL_GET_OPTIONS (crop_tool);
  GimpToolWidget   *widget;
  gint              i;

  tool->display = display;

  crop_tool->widget = widget = gimp_tool_rectangle_new (shell);

  g_object_set (widget,
                "status-title", _("Crop to: "),
                NULL);

  gimp_draw_tool_set_widget (GIMP_DRAW_TOOL (tool), widget);

  for (i = 0; i < G_N_ELEMENTS (properties); i++)
    {
      GBinding *binding =
        g_object_bind_property (G_OBJECT (options), properties[i],
                                G_OBJECT (widget),  properties[i],
                                G_BINDING_SYNC_CREATE |
                                G_BINDING_BIDIRECTIONAL);

      crop_tool->bindings = g_list_prepend (crop_tool->bindings, binding);
    }

  gimp_rectangle_options_connect (GIMP_RECTANGLE_OPTIONS (options),
                                  gimp_display_get_image (shell->display),
                                  G_CALLBACK (gimp_crop_tool_auto_shrink),
                                  crop_tool);

  gimp_tool_rectangle_set_constraint (GIMP_TOOL_RECTANGLE (widget),
                                      gimp_crop_tool_get_constraint (crop_tool));

  g_signal_connect (widget, "changed",
                    G_CALLBACK (gimp_crop_tool_rectangle_changed),
                    crop_tool);
  g_signal_connect (widget, "response",
                    G_CALLBACK (gimp_crop_tool_rectangle_response),
                    crop_tool);
  g_signal_connect (widget, "change-complete",
                    G_CALLBACK (gimp_crop_tool_rectangle_change_complete),
                    crop_tool);

  gimp_draw_tool_start (GIMP_DRAW_TOOL (tool), display);
}

static void
gimp_crop_tool_commit (GimpCropTool *crop_tool)
{
  GimpTool *tool = GIMP_TOOL (crop_tool);

  if (tool->display)
    {
      GimpCropOptions *options = GIMP_CROP_TOOL_GET_OPTIONS (tool);
      GimpImage       *image   = gimp_display_get_image (tool->display);
      gdouble          x, y;
      gdouble          x2, y2;
      gint             w, h;

      gimp_tool_rectangle_get_public_rect (GIMP_TOOL_RECTANGLE (crop_tool->widget),
                                           &x, &y, &x2, &y2);
      w = x2 - x;
      h = y2 - y;

      gimp_tool_pop_status (tool, tool->display);

      /* if rectangle exists, crop it */
      if (w > 0 && h > 0)
        {
          if (options->layer_only)
            {
              GimpLayer *layer = gimp_image_get_active_layer (image);
              gint       off_x, off_y;

              if (! layer)
                {
                  gimp_tool_message_literal (tool, tool->display,
                                             _("There is no active layer to crop."));
                  return;
                }

              if (gimp_item_is_content_locked (GIMP_ITEM (layer)))
                {
                  gimp_tool_message_literal (tool, tool->display,
                                             _("The active layer's pixels are locked."));
                  gimp_tools_blink_lock_box (tool->display->gimp,
                                             GIMP_ITEM (layer));
                  return;
                }

              gimp_item_get_offset (GIMP_ITEM (layer), &off_x, &off_y);

              off_x -= x;
              off_y -= y;

              gimp_item_resize (GIMP_ITEM (layer),
                                GIMP_CONTEXT (options), options->fill_type,
                                w, h, off_x, off_y);
            }
          else
            {
              gimp_image_crop (image,
                               GIMP_CONTEXT (options), GIMP_FILL_TRANSPARENT,
                               x, y, w, h, TRUE);
            }

          gimp_image_flush (image);
        }
    }
}

static void
gimp_crop_tool_halt (GimpCropTool *crop_tool)
{
  GimpTool         *tool    = GIMP_TOOL (crop_tool);
  GimpCropOptions  *options = GIMP_CROP_TOOL_GET_OPTIONS (crop_tool);

  if (tool->display)
    {
      GimpDisplayShell *shell = gimp_display_get_shell (tool->display);

      gimp_display_shell_set_highlight (shell, NULL, 0.0);

      gimp_rectangle_options_disconnect (GIMP_RECTANGLE_OPTIONS (options),
                                         G_CALLBACK (gimp_crop_tool_auto_shrink),
                                         crop_tool);
    }

  if (gimp_draw_tool_is_active (GIMP_DRAW_TOOL (tool)))
    gimp_draw_tool_stop (GIMP_DRAW_TOOL (tool));

  /*  disconnect bindings manually so they are really gone *now*, we
   *  might be in the middle of a signal emission that keeps the
   *  widget and its bindings alive.
   */
  g_list_free_full (crop_tool->bindings, (GDestroyNotify) g_object_unref);
  crop_tool->bindings = NULL;

  gimp_draw_tool_set_widget (GIMP_DRAW_TOOL (tool), NULL);
  g_clear_object (&crop_tool->widget);

  tool->display  = NULL;
  tool->drawable = NULL;

  gimp_crop_tool_update_option_defaults (crop_tool, TRUE);
}

/**
 * gimp_crop_tool_update_option_defaults:
 * @crop_tool:
 * @ignore_pending: %TRUE to ignore any pending crop rectangle.
 *
 * Sets the default Fixed: Aspect ratio and Fixed: Size option
 * properties.
 */
static void
gimp_crop_tool_update_option_defaults (GimpCropTool *crop_tool,
                                       gboolean      ignore_pending)
{
  GimpTool             *tool      = GIMP_TOOL (crop_tool);
  GimpToolRectangle    *rectangle = GIMP_TOOL_RECTANGLE (crop_tool->widget);
  GimpRectangleOptions *options;

  options = GIMP_RECTANGLE_OPTIONS (GIMP_TOOL_GET_OPTIONS (tool));

  if (rectangle && ! ignore_pending)
    {
      /* There is a pending rectangle and we should not ignore it, so
       * set default Fixed: Aspect ratio to the same as the current
       * pending rectangle width/height.
       */

      gimp_tool_rectangle_pending_size_set (rectangle,
                                            G_OBJECT (options),
                                            "default-aspect-numerator",
                                            "default-aspect-denominator");

      g_object_set (G_OBJECT (options),
                    "use-string-current", TRUE,
                    NULL);
    }
  else
    {
      /* There is no pending rectangle, set default Fixed: Aspect
       * ratio to that of the current image/layer.
       */

      if (! rectangle)
        {
          /* ugly hack: if we don't have a widget, construct a temporary one
           * so that we can use it to call
           * gimp_tool_rectangle_constraint_size_set().
           */

          GimpContext *context = gimp_get_user_context (tool->tool_info->gimp);
          GimpDisplay *display = gimp_context_get_display (context);

          if (display)
            {
              GimpDisplayShell *shell = gimp_display_get_shell (display);

              rectangle = GIMP_TOOL_RECTANGLE (gimp_tool_rectangle_new (shell));

              gimp_tool_rectangle_set_constraint (
                rectangle, gimp_crop_tool_get_constraint (crop_tool));
            }
        }

      if (rectangle)
        {
          gimp_tool_rectangle_constraint_size_set (rectangle,
                                                   G_OBJECT (options),
                                                   "default-aspect-numerator",
                                                   "default-aspect-denominator");

          if (! crop_tool->widget)
            g_object_unref (rectangle);
        }

      g_object_set (G_OBJECT (options),
                    "use-string-current", FALSE,
                    NULL);
    }
}

static GimpRectangleConstraint
gimp_crop_tool_get_constraint (GimpCropTool *crop_tool)
{
  GimpCropOptions *crop_options = GIMP_CROP_TOOL_GET_OPTIONS (crop_tool);

  if (crop_options->allow_growing)
    {
      return GIMP_RECTANGLE_CONSTRAIN_NONE;
    }
  else
    {
      return crop_options->layer_only ? GIMP_RECTANGLE_CONSTRAIN_DRAWABLE :
                                        GIMP_RECTANGLE_CONSTRAIN_IMAGE;
    }
}

static void
gimp_crop_tool_image_changed (GimpCropTool *crop_tool,
                              GimpImage    *image,
                              GimpContext  *context)
{
  if (crop_tool->current_image)
    {
      g_signal_handlers_disconnect_by_func (crop_tool->current_image,
                                            gimp_crop_tool_image_size_changed,
                                            NULL);
      g_signal_handlers_disconnect_by_func (crop_tool->current_image,
                                            gimp_crop_tool_image_active_layer_changed,
                                            NULL);

      g_object_remove_weak_pointer (G_OBJECT (crop_tool->current_image),
                                    (gpointer) &crop_tool->current_image);
    }

  crop_tool->current_image = image;

  if (crop_tool->current_image)
    {
      g_object_add_weak_pointer (G_OBJECT (crop_tool->current_image),
                                 (gpointer) &crop_tool->current_image);

      g_signal_connect_object (crop_tool->current_image, "size-changed",
                               G_CALLBACK (gimp_crop_tool_image_size_changed),
                               crop_tool,
                               G_CONNECT_SWAPPED);
      g_signal_connect_object (crop_tool->current_image, "active-layer-changed",
                               G_CALLBACK (gimp_crop_tool_image_active_layer_changed),
                               crop_tool,
                               G_CONNECT_SWAPPED);
    }

  /* Make sure we are connected to "size-changed" for the initial
   * layer.
   */
  gimp_crop_tool_image_active_layer_changed (crop_tool);

  gimp_crop_tool_update_option_defaults (GIMP_CROP_TOOL (crop_tool), FALSE);
}

static void
gimp_crop_tool_image_size_changed (GimpCropTool *crop_tool)
{
  gimp_crop_tool_update_option_defaults (crop_tool, FALSE);
}

static void
gimp_crop_tool_image_active_layer_changed (GimpCropTool *crop_tool)
{
  if (crop_tool->current_layer)
    {
      g_signal_handlers_disconnect_by_func (crop_tool->current_layer,
                                            gimp_crop_tool_layer_size_changed,
                                            NULL);

      g_object_remove_weak_pointer (G_OBJECT (crop_tool->current_layer),
                                    (gpointer) &crop_tool->current_layer);
    }

  if (crop_tool->current_image)
    {
      crop_tool->current_layer =
        gimp_image_get_active_layer (crop_tool->current_image);
    }
  else
    {
      crop_tool->current_layer = NULL;
    }

  if (crop_tool->current_layer)
    {
      g_object_add_weak_pointer (G_OBJECT (crop_tool->current_layer),
                                 (gpointer) &crop_tool->current_layer);

      g_signal_connect_object (crop_tool->current_layer, "size-changed",
                               G_CALLBACK (gimp_crop_tool_layer_size_changed),
                               crop_tool,
                               G_CONNECT_SWAPPED);
    }

  gimp_crop_tool_update_option_defaults (crop_tool, FALSE);
}

static void
gimp_crop_tool_layer_size_changed (GimpCropTool *crop_tool)
{
  gimp_crop_tool_update_option_defaults (crop_tool, FALSE);
}

static void
gimp_crop_tool_auto_shrink (GimpCropTool *crop_tool)
{
  gboolean shrink_merged ;

  g_object_get (gimp_tool_get_options (GIMP_TOOL (crop_tool)),
                "shrink-merged", &shrink_merged,
                NULL);

  gimp_tool_rectangle_auto_shrink (GIMP_TOOL_RECTANGLE (crop_tool->widget),
                                   shrink_merged);
}
