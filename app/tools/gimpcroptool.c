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

#include "gimp-intl.h"


static void      gimp_crop_tool_constructed               (GObject              *object);

static void      gimp_crop_tool_control                   (GimpTool             *tool,
                                                           GimpToolAction        action,
                                                           GimpDisplay          *display);
static void      gimp_crop_tool_button_press              (GimpTool             *tool,
                                                           const GimpCoords     *coords,
                                                           guint32               time,
                                                           GdkModifierType       state,
                                                           GimpButtonPressType   press_type,
                                                           GimpDisplay          *display);
static void      gimp_crop_tool_button_release            (GimpTool             *tool,
                                                           const GimpCoords     *coords,
                                                           guint32               time,
                                                           GdkModifierType       state,
                                                           GimpButtonReleaseType release_type,
                                                           GimpDisplay          *display);
static void     gimp_crop_tool_motion                     (GimpTool             *tool,
                                                           const GimpCoords     *coords,
                                                           guint32               time,
                                                           GdkModifierType       state,
                                                           GimpDisplay          *display);
static void      gimp_crop_tool_active_modifier_key       (GimpTool             *tool,
                                                           GdkModifierType       key,
                                                           gboolean              press,
                                                           GdkModifierType       state,
                                                           GimpDisplay          *display);
static void      gimp_crop_tool_cursor_update             (GimpTool             *tool,
                                                           const GimpCoords     *coords,
                                                           GdkModifierType       state,
                                                           GimpDisplay          *display);
static void      gimp_crop_tool_options_notify            (GimpTool             *tool,
                                                           GimpToolOptions      *options,
                                                           const GParamSpec     *pspec);

static void      gimp_crop_tool_rectangle_changed         (GimpToolWidget       *rectangle,
                                                           GimpCropTool         *crop_tool);
static void      gimp_crop_tool_rectangle_response        (GimpToolWidget       *rectangle,
                                                           gint                  response_id,
                                                           GimpCropTool         *crop_tool);
static void      gimp_crop_tool_rectangle_status          (GimpToolWidget       *rectangle,
                                                           const gchar          *status,
                                                           GimpCropTool         *crop_tool);
static void      gimp_crop_tool_rectangle_status_coords   (GimpToolWidget       *rectangle,
                                                           const gchar          *title,
                                                           gdouble               x,
                                                           const gchar          *separator,
                                                           gdouble               y,
                                                           const gchar          *help,
                                                           GimpCropTool         *crop_tool);
static void      gimp_crop_tool_rectangle_snap_offsets    (GimpToolRectangle    *rectangle,
                                                           gint                  x,
                                                           gint                  y,
                                                           gint                  width,
                                                           gint                  height,
                                                           GimpCropTool         *crop_tool);
static void      gimp_crop_tool_rectangle_change_complete (GimpToolRectangle    *rectangle,
                                                           GimpCropTool         *crop_tool);

static void      gimp_crop_tool_commit                    (GimpCropTool         *crop_tool);
static void      gimp_crop_tool_halt                      (GimpCropTool         *crop_tool);

static void      gimp_crop_tool_update_option_defaults    (GimpCropTool         *crop_tool,
                                                           gboolean              ignore_pending);
static GimpRectangleConstraint
                 gimp_crop_tool_get_constraint            (GimpCropTool         *crop_tool);

static void      gimp_crop_tool_image_changed             (GimpCropTool         *crop_tool,
                                                           GimpImage            *image,
                                                           GimpContext          *context);
static void      gimp_crop_tool_image_size_changed        (GimpCropTool         *crop_tool);

static void      gimp_crop_tool_auto_shrink               (GimpCropTool         *crop_tool);


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

  object_class->constructed       = gimp_crop_tool_constructed;

  tool_class->control             = gimp_crop_tool_control;
  tool_class->button_press        = gimp_crop_tool_button_press;
  tool_class->button_release      = gimp_crop_tool_button_release;
  tool_class->motion              = gimp_crop_tool_motion;
  tool_class->active_modifier_key = gimp_crop_tool_active_modifier_key;
  tool_class->cursor_update       = gimp_crop_tool_cursor_update;
  tool_class->options_notify      = gimp_crop_tool_options_notify;
}

static void
gimp_crop_tool_init (GimpCropTool *crop_tool)
{
  GimpTool *tool = GIMP_TOOL (crop_tool);

  gimp_tool_control_set_wants_click (tool->control, TRUE);
  gimp_tool_control_set_precision   (tool->control,
                                     GIMP_CURSOR_PRECISION_PIXEL_BORDER);
  gimp_tool_control_set_tool_cursor (tool->control, GIMP_TOOL_CURSOR_CROP);
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
    gimp_tool_control (tool, GIMP_TOOL_ACTION_COMMIT, tool->display);

  if (! tool->display)
    {
      static const gchar *properties[] =
      {
        "highlight",
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

      GimpDisplayShell *shell   = gimp_display_get_shell (display);
      GimpCropOptions  *options = GIMP_CROP_TOOL_GET_OPTIONS (crop_tool);
      GimpToolWidget   *widget;
      gint              i;

      tool->display = display;

      crop_tool->rectangle = widget = gimp_tool_rectangle_new (shell);

      gimp_draw_tool_set_widget (GIMP_DRAW_TOOL (tool), widget);

      for (i = 0; i < G_N_ELEMENTS (properties); i++)
        g_object_bind_property (G_OBJECT (options), properties[i],
                                G_OBJECT (widget),  properties[i],
                                G_BINDING_SYNC_CREATE |
                                G_BINDING_BIDIRECTIONAL);

      g_signal_connect (widget, "changed",
                        G_CALLBACK (gimp_crop_tool_rectangle_changed),
                        crop_tool);
      g_signal_connect (widget, "response",
                        G_CALLBACK (gimp_crop_tool_rectangle_response),
                        crop_tool);
      g_signal_connect (widget, "status",
                        G_CALLBACK (gimp_crop_tool_rectangle_status),
                        crop_tool);
      g_signal_connect (widget, "status-coords",
                        G_CALLBACK (gimp_crop_tool_rectangle_status_coords),
                        crop_tool);
      g_signal_connect (widget, "snap-offsets",
                        G_CALLBACK (gimp_crop_tool_rectangle_snap_offsets),
                        crop_tool);
      g_signal_connect (widget, "change-complete",
                        G_CALLBACK (gimp_crop_tool_rectangle_change_complete),
                        crop_tool);

      gimp_rectangle_options_connect (GIMP_RECTANGLE_OPTIONS (options),
                                      gimp_display_get_image (shell->display),
                                      G_CALLBACK (gimp_crop_tool_auto_shrink),
                                      crop_tool);

      gimp_tool_rectangle_set_constraint (GIMP_TOOL_RECTANGLE (widget),
                                          gimp_crop_tool_get_constraint (crop_tool));

      gimp_crop_tool_update_option_defaults (crop_tool, FALSE);

      gimp_tool_widget_hover (crop_tool->rectangle, coords, state, TRUE);

      /* HACK: force CREATING on a newly created rectangle; otherwise,
       * the above binding of properties would cause the rectangle to
       * start with the size from tool options.
       */
      gimp_tool_rectangle_set_function (GIMP_TOOL_RECTANGLE (crop_tool->rectangle),
                                        GIMP_TOOL_RECTANGLE_CREATING);

      gimp_draw_tool_start (GIMP_DRAW_TOOL (tool), display);
    }

  if (gimp_tool_widget_button_press (crop_tool->rectangle, coords, time, state,
                                     press_type))
    {
      crop_tool->grab_widget = crop_tool->rectangle;
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

  gimp_tool_push_status (tool, display, _("Click or press Enter to crop"));

  if (crop_tool->grab_widget)
    {
      gimp_tool_widget_button_release (crop_tool->grab_widget,
                                       coords, time, state, release_type);
      crop_tool->grab_widget = NULL;
    }
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
gimp_crop_tool_active_modifier_key (GimpTool        *tool,
                                    GdkModifierType  key,
                                    gboolean         press,
                                    GdkModifierType  state,
                                    GimpDisplay     *display)
{
  GimpCropTool *crop_tool = GIMP_CROP_TOOL (tool);

  if (crop_tool->rectangle)
    {
      gimp_tool_widget_motion_modifier (crop_tool->rectangle,
                                        key, press, state);
    }
}

static void
gimp_crop_tool_cursor_update (GimpTool         *tool,
                              const GimpCoords *coords,
                              GdkModifierType   state,
                              GimpDisplay      *display)
{
  GimpCropTool       *crop_tool = GIMP_CROP_TOOL (tool);
  GimpCursorType      cursor    = GIMP_CURSOR_CROSSHAIR_SMALL;
  GimpCursorModifier  modifier  = GIMP_CURSOR_MODIFIER_NONE;

  if (crop_tool->rectangle && display == tool->display)
    {
      gimp_tool_widget_get_cursor (crop_tool->rectangle, coords, state,
                                   &cursor, NULL, &modifier);
    }

  gimp_tool_control_set_cursor          (tool->control, cursor);
  gimp_tool_control_set_cursor_modifier (tool->control, modifier);

  GIMP_TOOL_CLASS (parent_class)->cursor_update (tool, coords, state, display);
}

static void
gimp_crop_tool_options_notify (GimpTool         *tool,
                               GimpToolOptions  *options,
                               const GParamSpec *pspec)
{
  GimpCropTool *crop_tool = GIMP_CROP_TOOL (tool);

  if (crop_tool->rectangle)
    {
      if (! strcmp (pspec->name, "layer-only") ||
          ! strcmp (pspec->name, "allow-growing"))
        {
          gimp_tool_rectangle_set_constraint (GIMP_TOOL_RECTANGLE (crop_tool->rectangle),
                                              gimp_crop_tool_get_constraint (crop_tool));
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
gimp_crop_tool_rectangle_status (GimpToolWidget *rectangle,
                                 const gchar    *status,
                                 GimpCropTool   *crop_tool)
{
  GimpTool *tool = GIMP_TOOL (crop_tool);

  if (status)
    {
      gimp_tool_replace_status (tool, tool->display, "%s", status);
    }
  else
    {
      gimp_tool_pop_status (tool, tool->display);
    }
}

static void
gimp_crop_tool_rectangle_status_coords (GimpToolWidget *rectangle,
                                        const gchar    *title,
                                        gdouble         x,
                                        const gchar    *separator,
                                        gdouble         y,
                                        const gchar    *help,
                                        GimpCropTool   *crop_tool)
{
  GimpTool *tool = GIMP_TOOL (crop_tool);

  gimp_tool_pop_status (tool, tool->display);
  gimp_tool_push_status_coords (tool, tool->display,
                                gimp_tool_control_get_precision (tool->control),
                                title, x, separator, y, help);
}

static void
gimp_crop_tool_rectangle_snap_offsets (GimpToolRectangle *rectangle,
                                       gint               offset_x,
                                       gint               offset_y,
                                       gint               width,
                                       gint               height,
                                       GimpCropTool      *crop_tool)
{
  GimpTool *tool = GIMP_TOOL (crop_tool);

  gimp_tool_control_set_snap_offsets (tool->control,
                                      offset_x, offset_y,
                                      width, height);
}

static void
gimp_crop_tool_rectangle_change_complete (GimpToolRectangle *rectangle,
                                          GimpCropTool      *crop_tool)
{
  gimp_crop_tool_update_option_defaults (crop_tool, FALSE);
}

static void
gimp_crop_tool_commit (GimpCropTool *crop_tool)
{
  GimpTool        *tool    = GIMP_TOOL (crop_tool);
  GimpCropOptions *options = GIMP_CROP_TOOL_GET_OPTIONS (tool);
  GimpImage       *image   = gimp_display_get_image (tool->display);
  gdouble          x, y;
  gdouble          x2, y2;
  gint             w, h;

  gimp_tool_rectangle_get_public_rect (GIMP_TOOL_RECTANGLE (crop_tool->rectangle),
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

  gimp_crop_tool_halt (crop_tool);
}

static void
gimp_crop_tool_halt (GimpCropTool *crop_tool)
{
  GimpTool         *tool    = GIMP_TOOL (crop_tool);
  GimpCropOptions  *options = GIMP_CROP_TOOL_GET_OPTIONS (crop_tool);

  if (tool->display)
    {
      GimpDisplayShell *shell = gimp_display_get_shell (tool->display);

      gimp_display_shell_set_highlight (shell, NULL);

      gimp_rectangle_options_disconnect (GIMP_RECTANGLE_OPTIONS (options),
                                         G_CALLBACK (gimp_crop_tool_auto_shrink),
                                         crop_tool);
    }

  if (gimp_draw_tool_is_active (GIMP_DRAW_TOOL (tool)))
    gimp_draw_tool_stop (GIMP_DRAW_TOOL (tool));

  g_clear_object (&crop_tool->rectangle);

  tool->display  = NULL;
  tool->drawable = NULL;
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
  GimpToolRectangle    *rectangle = GIMP_TOOL_RECTANGLE (crop_tool->rectangle);
  GimpRectangleOptions *options;

  options = GIMP_RECTANGLE_OPTIONS (GIMP_TOOL_GET_OPTIONS (tool));

  if (! rectangle)
    return;

  if (tool->display && ! ignore_pending)
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

      gimp_tool_rectangle_constraint_size_set (rectangle,
                                               G_OBJECT (options),
                                               "default-aspect-numerator",
                                               "default-aspect-denominator");

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
    }

  crop_tool->current_image = image;

  if (crop_tool->current_image)
    {
      g_signal_connect_object (crop_tool->current_image, "size-changed",
                               G_CALLBACK (gimp_crop_tool_image_size_changed),
                               crop_tool,
                               G_CONNECT_SWAPPED);
    }

  gimp_crop_tool_update_option_defaults (GIMP_CROP_TOOL (crop_tool), FALSE);
}

static void
gimp_crop_tool_image_size_changed (GimpCropTool *crop_tool)
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

  gimp_tool_rectangle_auto_shrink (GIMP_TOOL_RECTANGLE (crop_tool->rectangle),
                                   shrink_merged);
}
