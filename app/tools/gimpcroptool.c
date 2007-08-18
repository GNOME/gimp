/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
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

#include "libgimpwidgets/gimpwidgets.h"

#include "tools-types.h"

#include "core/gimp.h"
#include "core/gimpimage.h"
#include "core/gimpimage-crop.h"
#include "core/gimpitem.h"
#include "core/gimptoolinfo.h"

#include "widgets/gimphelp-ids.h"

#include "display/gimpdisplay.h"

#include "gimprectangleoptions.h"
#include "gimprectangletool.h"
#include "gimpcropoptions.h"
#include "gimpcroptool.h"
#include "gimptoolcontrol.h"

#include "gimp-intl.h"


static void   gimp_crop_tool_rectangle_tool_iface_init (GimpRectangleToolInterface *iface);

static GObject * gimp_crop_tool_constructor      (GType                  type,
                                                  guint                  n_params,
                                                  GObjectConstructParam *params);
static void   gimp_crop_tool_control             (GimpTool              *tool,
                                                  GimpToolAction         action,
                                                  GimpDisplay           *display);
static void   gimp_crop_tool_button_press        (GimpTool              *tool,
                                                  GimpCoords            *coords,
                                                  guint32                time,
                                                  GdkModifierType        state,
                                                  GimpDisplay           *display);
static void   gimp_crop_tool_button_release      (GimpTool              *tool,
                                                  GimpCoords            *coords,
                                                  guint32                time,
                                                  GdkModifierType        state,
                                                  GimpButtonReleaseType  release_type,
                                                  GimpDisplay           *display);
static void   gimp_crop_tool_active_modifier_key (GimpTool              *tool,
                                                  GdkModifierType        key,
                                                  gboolean               press,
                                                  GdkModifierType        state,
                                                  GimpDisplay           *display);
static void   gimp_crop_tool_cursor_update       (GimpTool              *tool,
                                                  GimpCoords            *coords,
                                                  GdkModifierType        state,
                                                  GimpDisplay           *display);

static gboolean   gimp_crop_tool_execute         (GimpRectangleTool     *rectangle,
                                                  gint                   x,
                                                  gint                   y,
                                                  gint                   w,
                                                  gint                   h);

static void   gimp_crop_tool_update_default_fixed_ratio_options
                                                 (GimpCropTool          *crop_tool,
                                                  gboolean               ignore_pending);

static void   gimp_crop_tool_notify_layer_only   (GimpCropOptions       *options,
                                                  GParamSpec            *pspec,
                                                  GimpTool              *tool);

static void   gimp_crop_tool_image_changed       (GimpContext           *gimp_context,
                                                  GimpImage             *image,
                                                  GimpCropTool          *crop_tool);


G_DEFINE_TYPE_WITH_CODE (GimpCropTool, gimp_crop_tool, GIMP_TYPE_DRAW_TOOL,
                         G_IMPLEMENT_INTERFACE (GIMP_TYPE_RECTANGLE_TOOL,
                                                gimp_crop_tool_rectangle_tool_iface_init));

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
                GIMP_STOCK_TOOL_CROP,
                data);
}

static void
gimp_crop_tool_class_init (GimpCropToolClass *klass)
{
  GObjectClass      *object_class    = G_OBJECT_CLASS (klass);
  GimpToolClass     *tool_class      = GIMP_TOOL_CLASS (klass);
  GimpDrawToolClass *draw_tool_class = GIMP_DRAW_TOOL_CLASS (klass);

  object_class->constructor       = gimp_crop_tool_constructor;
  object_class->set_property      = gimp_rectangle_tool_set_property;
  object_class->get_property      = gimp_rectangle_tool_get_property;

  gimp_rectangle_tool_install_properties (object_class);

  tool_class->control             = gimp_crop_tool_control;
  tool_class->button_press        = gimp_crop_tool_button_press;
  tool_class->button_release      = gimp_crop_tool_button_release;
  tool_class->motion              = gimp_rectangle_tool_motion;
  tool_class->key_press           = gimp_rectangle_tool_key_press;
  tool_class->active_modifier_key = gimp_crop_tool_active_modifier_key;
  tool_class->oper_update         = gimp_rectangle_tool_oper_update;
  tool_class->cursor_update       = gimp_crop_tool_cursor_update;

  draw_tool_class->draw           = gimp_rectangle_tool_draw;
}

static void
gimp_crop_tool_rectangle_tool_iface_init (GimpRectangleToolInterface *iface)
{
  iface->execute = gimp_crop_tool_execute;
}

static void
gimp_crop_tool_init (GimpCropTool *crop_tool)
{
  GimpTool *tool = GIMP_TOOL (crop_tool);

  gimp_tool_control_set_wants_click (tool->control, TRUE);
  gimp_tool_control_set_tool_cursor (tool->control, GIMP_TOOL_CURSOR_CROP);
}

static GObject *
gimp_crop_tool_constructor (GType                  type,
                            guint                  n_params,
                            GObjectConstructParam *params)
{
  GObject         *object;
  GimpCropOptions *options;
  GimpContext     *gimp_context;
  GimpToolInfo    *tool_info;

  object = G_OBJECT_CLASS (parent_class)->constructor (type, n_params, params);

  gimp_rectangle_tool_constructor (object);


  g_object_get (object,
                "tool-info", &tool_info,
                NULL);

  gimp_context = gimp_get_user_context (tool_info->gimp);

  g_signal_connect_object (gimp_context, "image-changed",
                           G_CALLBACK (gimp_crop_tool_image_changed),
                           GIMP_CROP_TOOL (object), 0);


  options = GIMP_CROP_TOOL_GET_OPTIONS (object);

  g_signal_connect_object (options, "notify::layer-only",
                           G_CALLBACK (gimp_crop_tool_notify_layer_only),
                           object, 0);

  gimp_rectangle_tool_set_constraint (GIMP_RECTANGLE_TOOL (object),
                                      options->layer_only ?
                                      GIMP_RECTANGLE_CONSTRAIN_DRAWABLE :
                                      GIMP_RECTANGLE_CONSTRAIN_IMAGE);

  gimp_crop_tool_update_default_fixed_ratio_options (GIMP_CROP_TOOL (object),
                                                     FALSE);

  return object;
}

static void
gimp_crop_tool_control (GimpTool       *tool,
                        GimpToolAction  action,
                        GimpDisplay    *display)
{
  gimp_rectangle_tool_control (tool, action, display);

  GIMP_TOOL_CLASS (parent_class)->control (tool, action, display);
}

static void
gimp_crop_tool_button_press (GimpTool        *tool,
                             GimpCoords      *coords,
                             guint32          time,
                             GdkModifierType  state,
                             GimpDisplay     *display)
{
  if (tool->display && display != tool->display)
    gimp_rectangle_tool_cancel (GIMP_RECTANGLE_TOOL (tool));

  gimp_rectangle_tool_button_press (tool, coords, time, state, display);
}

static void
gimp_crop_tool_button_release (GimpTool              *tool,
                               GimpCoords            *coords,
                               guint32                time,
                               GdkModifierType        state,
                               GimpButtonReleaseType  release_type,
                               GimpDisplay           *display)
{
  gimp_tool_push_status (tool, display, _("Click or press Enter to crop"));

  gimp_crop_tool_update_default_fixed_ratio_options (GIMP_CROP_TOOL (tool),
                                                     FALSE);

  gimp_rectangle_tool_button_release (tool, coords, time, state, release_type,
                                      display);
}

static void
gimp_crop_tool_active_modifier_key (GimpTool        *tool,
                                    GdkModifierType  key,
                                    gboolean         press,
                                    GdkModifierType  state,
                                    GimpDisplay     *display)
{
  GIMP_TOOL_CLASS (parent_class)->active_modifier_key (tool, key, press, state,
                                                       display);

  gimp_rectangle_tool_active_modifier_key (tool, key, press, state, display);
}

static void
gimp_crop_tool_cursor_update (GimpTool        *tool,
                              GimpCoords      *coords,
                              GdkModifierType  state,
                              GimpDisplay     *display)
{
  gimp_rectangle_tool_cursor_update (tool, coords, state, display);

  GIMP_TOOL_CLASS (parent_class)->cursor_update (tool, coords, state, display);
}

static gboolean
gimp_crop_tool_execute (GimpRectangleTool  *rectangle,
                        gint                x,
                        gint                y,
                        gint                w,
                        gint                h)
{
  GimpTool             *tool;
  GimpCropOptions      *options;
  GimpRectangleOptions *rectangle_options;
  GimpImage            *image;
  gint                  max_x, max_y;
  gboolean              rectangle_exists;

  tool              = GIMP_TOOL (rectangle);
  options           = GIMP_CROP_TOOL_GET_OPTIONS (tool);
  rectangle_options = GIMP_RECTANGLE_TOOL_GET_OPTIONS (rectangle);

  gimp_tool_pop_status (tool, tool->display);

  image = tool->display->image;
  max_x = image->width;
  max_y = image->height;

  rectangle_exists = (x <= max_x && y <= max_y &&
                      x + w >= 0 && y + h >= 0 &&
                      w > 0 && h > 0);

  if (x < 0)
    {
      w += x;
      x = 0;
    }

  if (y < 0)
    {
      h += y;
      y = 0;
    }

  if (x + w > max_x)
    w = max_x - x;

  if (y + h > max_y)
    h = max_y - y;

  /* if rectangle exists, crop it */
  if (rectangle_exists)
    {
      gimp_image_crop (image, GIMP_CONTEXT (options),
                       x, y, w + x, h + y,
                       options->layer_only,
                       TRUE);

      gimp_image_flush (image);

      gimp_crop_tool_update_default_fixed_ratio_options (GIMP_CROP_TOOL (tool),
                                                         TRUE);

      return TRUE;
    }

  return TRUE;
}

/**
 * gimp_crop_tool_update_default_fixed_ratio_options:
 * @crop_tool:
 * @ignore_pending: %TRUE to ignore any pending crop rectangle.
 *
 * Sets the default aspect numerator/denominator to that of the current
 * layer/image/pending crop rectangle.
 */
static void gimp_crop_tool_update_default_fixed_ratio_options (GimpCropTool *crop_tool,
                                                               gboolean      ignore_pending)
{
  GimpTool             *tool;
  GimpRectangleTool    *rectangle_tool;
  GimpRectangleOptions *rectangle_options;
  gdouble               default_numerator;
  gdouble               default_denominator;

  tool              = GIMP_TOOL (crop_tool);
  rectangle_tool    = GIMP_RECTANGLE_TOOL (tool);
  rectangle_options = GIMP_RECTANGLE_TOOL_GET_OPTIONS (rectangle_tool);

  if (tool->display != NULL && !ignore_pending)
    {
      gint crop_rectangle_width;
      gint crop_rectangle_height;

      /* There exists a pending crop rectangle, set default aspect ratio to the
       * same aspect ratio of the rectangle.
       */

      gimp_rectangle_tool_get_rectangle_size (rectangle_tool,
                                              &crop_rectangle_width,
                                              &crop_rectangle_height);

      default_numerator   = MAX (crop_rectangle_width,  1.0);
      default_denominator = MAX (crop_rectangle_height, 1.0);
    }
  else
    {
      GimpRectangleConstraint  constraint;
      GimpContext             *gimp_context;
      GimpImage               *image;

      /* There exist no rectangle, set defaults */

      gimp_context = gimp_get_user_context (tool->tool_info->gimp);
      image        = gimp_context_get_image (gimp_context);

      if (image == NULL)
        {
          default_numerator   = 1.0;
          default_denominator = 1.0;
        }
      else
        {
          constraint = gimp_rectangle_tool_get_constraint (rectangle_tool);

          switch (constraint)
            {
            case GIMP_RECTANGLE_CONSTRAIN_DRAWABLE:
              {
                GimpItem *item = GIMP_ITEM (gimp_image_get_active_layer (image));

                if (item == NULL)
                  {
                    default_numerator   = 1.0;
                    default_denominator = 1.0;
                  }
                else
                  {
                    default_numerator   = gimp_item_width (item);
                    default_denominator = gimp_item_height (item);
                  }
              }
              break;

            case GIMP_RECTANGLE_CONSTRAIN_IMAGE:
            default:
              {
                default_numerator   = image->width;
                default_denominator = image->height;
              }
              break;
            }
        }
    }

  g_object_set (rectangle_options,
                "default-aspect-numerator",   default_numerator,
                "default-aspect-denominator", default_denominator,
                NULL);
}

static void
gimp_crop_tool_notify_layer_only (GimpCropOptions *options,
                                  GParamSpec      *pspec,
                                  GimpTool        *tool)
{
  gimp_rectangle_tool_set_constraint (GIMP_RECTANGLE_TOOL (tool),
                                      options->layer_only ?
                                      GIMP_RECTANGLE_CONSTRAIN_DRAWABLE :
                                      GIMP_RECTANGLE_CONSTRAIN_IMAGE);

  gimp_crop_tool_update_default_fixed_ratio_options (GIMP_CROP_TOOL (tool),
                                                     FALSE);
}

static void
gimp_crop_tool_image_changed (GimpContext  *gimp_context,
                              GimpImage    *image,
                              GimpCropTool *crop_tool)
{
  gimp_crop_tool_update_default_fixed_ratio_options (GIMP_CROP_TOOL (crop_tool),
                                                     FALSE);
}
