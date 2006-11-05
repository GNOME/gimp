/* The GIMP -- an image manipulation program
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

#include "core/gimpimage.h"
#include "core/gimpimage-crop.h"

#include "widgets/gimphelp-ids.h"

#include "display/gimpdisplay.h"

#include "gimprectangleoptions.h"
#include "gimprectangletool.h"
#include "gimpcropoptions.h"
#include "gimpcroptool.h"
#include "gimptoolcontrol.h"

#include "gimp-intl.h"


static void     gimp_crop_tool_rectangle_tool_iface_init (GimpRectangleToolInterface *iface);

static GObject *gimp_crop_tool_constructor         (GType              type,
                                                    guint              n_params,
                                                    GObjectConstructParam *params);
static gboolean gimp_crop_tool_initialize          (GimpTool          *tool,
                                                    GimpDisplay       *display,
                                                    GError           **error);
static void     gimp_crop_tool_control             (GimpTool          *tool,
                                                    GimpToolAction     action,
                                                    GimpDisplay       *display);

static void     gimp_crop_tool_button_press        (GimpTool          *tool,
                                                    GimpCoords        *coords,
                                                    guint32            time,
                                                    GdkModifierType    state,
                                                    GimpDisplay       *display);
static void     gimp_crop_tool_button_release      (GimpTool          *tool,
                                                    GimpCoords        *coords,
                                                    guint32            time,
                                                    GdkModifierType    state,
                                                    GimpDisplay       *display);
static void     gimp_crop_tool_active_modifier_key (GimpTool          *tool,
                                                    GdkModifierType    key,
                                                    gboolean           press,
                                                    GdkModifierType    state,
                                                    GimpDisplay       *display);
static void     gimp_crop_tool_cursor_update       (GimpTool          *tool,
                                                    GimpCoords        *coords,
                                                    GdkModifierType    state,
                                                    GimpDisplay       *display);
static gboolean gimp_crop_tool_execute             (GimpRectangleTool *rectangle,
                                                    gint               x,
                                                    gint               y,
                                                    gint               w,
                                                    gint               h);
static void     gimp_crop_tool_notify_layer_only   (GimpCropOptions   *options,
                                                    GParamSpec        *pspec,
                                                    GimpTool          *tool);


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

  tool_class->initialize          = gimp_crop_tool_initialize;
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
  GimpTool           *tool      = GIMP_TOOL (crop_tool);
  GimpRectangleTool  *rect_tool = GIMP_RECTANGLE_TOOL (crop_tool);

  gimp_tool_control_set_tool_cursor (tool->control, GIMP_TOOL_CURSOR_CROP);
  gimp_rectangle_tool_set_constraint (rect_tool, TRUE);
}

static GObject *
gimp_crop_tool_constructor (GType                  type,
                            guint                  n_params,
                            GObjectConstructParam *params)
{
  GObject *object;

  object = G_OBJECT_CLASS (parent_class)->constructor (type, n_params, params);

  gimp_rectangle_tool_constructor (object);

  return object;
}

static gboolean
gimp_crop_tool_initialize (GimpTool     *tool,
                           GimpDisplay  *display,
                           GError      **error)
{
  GimpRectangleTool *rectangle = GIMP_RECTANGLE_TOOL (tool);
  GObject           *options   = G_OBJECT (gimp_tool_get_options (tool));
  gboolean           layer_only;

  g_object_get (options,
                "layer-only", &layer_only,
                NULL);

  g_signal_connect_object (options, "notify::layer-only",
                           G_CALLBACK (gimp_crop_tool_notify_layer_only),
                           tool, 0);

  if (layer_only)
    gimp_rectangle_tool_set_constraint (rectangle,
                                        GIMP_RECTANGLE_CONSTRAIN_DRAWABLE);
  else
    gimp_rectangle_tool_set_constraint (rectangle,
                                        GIMP_RECTANGLE_CONSTRAIN_IMAGE);

  return gimp_rectangle_tool_initialize (tool, display, error);
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
gimp_crop_tool_button_release (GimpTool        *tool,
                               GimpCoords      *coords,
                               guint32          time,
                               GdkModifierType  state,
                               GimpDisplay     *display)
{
  gimp_tool_push_status (tool, display, _("Click or press enter to crop"));

  gimp_rectangle_tool_button_release (tool, coords, time, state, display);
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
  gimp_tool_control_set_tool_cursor (tool->control, GIMP_TOOL_CURSOR_CROP);

  GIMP_TOOL_CLASS (parent_class)->cursor_update (tool, coords, state, display);
}

static gboolean
gimp_crop_tool_execute (GimpRectangleTool  *rectangle,
                        gint                x,
                        gint                y,
                        gint                w,
                        gint                h)
{
  GimpTool        *tool    = GIMP_TOOL (rectangle);
  GimpCropOptions *options = GIMP_CROP_TOOL_GET_OPTIONS (tool);
  GimpImage       *image;
  gint             max_x, max_y;
  gboolean         rectangle_exists;

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

      return TRUE;
    }

  return TRUE;
}

static void
gimp_crop_tool_notify_layer_only (GimpCropOptions *options,
                                  GParamSpec      *pspec,
                                  GimpTool        *tool)
{
  GimpRectangleTool *rectangle = GIMP_RECTANGLE_TOOL (tool);
  gboolean           layer_only;

  g_object_get (options,
                "layer-only", &layer_only,
                NULL);

  if (layer_only)
    gimp_rectangle_tool_set_constraint (rectangle,
                                        GIMP_RECTANGLE_CONSTRAIN_DRAWABLE);
  else
    gimp_rectangle_tool_set_constraint (rectangle,
                                        GIMP_RECTANGLE_CONSTRAIN_IMAGE);
}
