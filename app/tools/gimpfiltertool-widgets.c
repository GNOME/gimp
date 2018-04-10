/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpfiltertool-widgets.c
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

#include "libgimpmath/gimpmath.h"

#include "tools-types.h"

#include "core/gimpitem.h"

#include "display/gimpdisplay.h"
#include "display/gimptoolgyroscope.h"
#include "display/gimptoolline.h"
#include "display/gimptooltransformgrid.h"

#include "gimpfilteroptions.h"
#include "gimpfiltertool.h"
#include "gimpfiltertool-widgets.h"


typedef struct _Controller Controller;

struct _Controller
{
  GimpFilterTool     *filter_tool;
  GimpControllerType  controller_type;
  GimpToolWidget     *widget;
  GCallback           creator_callback;
  gpointer            creator_data;
};


/*  local function prototypes  */

static void   gimp_filter_tool_set_line               (Controller                 *controller,
                                                       GeglRectangle              *area,
                                                       gdouble                     x1,
                                                       gdouble                     y1,
                                                       gdouble                     x2,
                                                       gdouble                     y2);
static void   gimp_filter_tool_line_changed           (GimpToolWidget             *widget,
                                                       Controller                 *controller);

static void   gimp_filter_tool_set_slider_line        (Controller                 *controller,
                                                       GeglRectangle              *area,
                                                       gdouble                     x1,
                                                       gdouble                     y1,
                                                       gdouble                     x2,
                                                       gdouble                     y2,
                                                       const GimpControllerSlider *sliders,
                                                       gint                        n_sliders);
static void   gimp_filter_tool_slider_line_changed    (GimpToolWidget             *widget,
                                                       Controller                 *controller);

static void   gimp_filter_tool_set_transform_grid     (Controller                 *controller,
                                                       GeglRectangle              *area,
                                                       const GimpMatrix3          *matrix);
static void   gimp_filter_tool_transform_grid_changed (GimpToolWidget             *widget,
                                                       Controller                 *controller);

static void   gimp_filter_tool_set_gyroscope          (Controller                 *controller,
                                                       GeglRectangle              *area,
                                                       gdouble                     yaw,
                                                       gdouble                     pitch,
                                                       gdouble                     roll,
                                                       gdouble                     zoom,
                                                       gboolean                    invert);
static void   gimp_filter_tool_gyroscope_changed      (GimpToolWidget             *widget,
                                                       Controller                 *controller);


/*  public functions  */

GimpToolWidget *
gimp_filter_tool_create_widget (GimpFilterTool     *filter_tool,
                                GimpControllerType  controller_type,
                                const gchar        *status_title,
                                GCallback           callback,
                                gpointer            callback_data,
                                GCallback          *set_func,
                                gpointer           *set_func_data)
{
  GimpTool         *tool;
  GimpDisplayShell *shell;
  Controller       *controller;

  g_return_val_if_fail (GIMP_IS_FILTER_TOOL (filter_tool), NULL);

  tool = GIMP_TOOL (filter_tool);

  g_return_val_if_fail (tool->display != NULL, NULL);

  shell = gimp_display_get_shell (tool->display);

  controller = g_new0 (Controller, 1);

  controller->filter_tool      = filter_tool;
  controller->controller_type  = controller_type;
  controller->creator_callback = callback;
  controller->creator_data     = callback_data;

  switch (controller_type)
    {
    case GIMP_CONTROLLER_TYPE_LINE:
      controller->widget = gimp_tool_line_new (shell, 100, 100, 500, 500);

      g_object_set (controller->widget,
                    "status-title", status_title,
                    NULL);

      g_signal_connect (controller->widget, "changed",
                        G_CALLBACK (gimp_filter_tool_line_changed),
                        controller);

      *set_func      = (GCallback) gimp_filter_tool_set_line;
      *set_func_data = controller;
      break;

    case GIMP_CONTROLLER_TYPE_SLIDER_LINE:
      controller->widget = gimp_tool_line_new (shell, 100, 100, 500, 500);

      g_object_set (controller->widget,
                    "status-title", status_title,
                    NULL);

      g_signal_connect (controller->widget, "changed",
                        G_CALLBACK (gimp_filter_tool_slider_line_changed),
                        controller);

      *set_func      = (GCallback) gimp_filter_tool_set_slider_line;
      *set_func_data = controller;
      break;

    case GIMP_CONTROLLER_TYPE_TRANSFORM_GRID:
      {
        GimpMatrix3   transform;
        gint          off_x, off_y;
        GeglRectangle area;
        gdouble       x1, y1;
        gdouble       x2, y2;

        gimp_matrix3_identity (&transform);

        gimp_filter_tool_get_drawable_area (filter_tool, &off_x, &off_y, &area);

        x1 = off_x + area.x;
        y1 = off_y + area.y;
        x2 = x1 + area.width;
        y2 = y1 + area.height;

        controller->widget = gimp_tool_transform_grid_new (shell, &transform,
                                                           x1, y1, x2, y2);

        g_object_set (controller->widget,
                      "pivot-x",                 (x1 + x2) / 2.0,
                      "pivot-y",                 (y1 + y2) / 2.0,
                      "inside-function",         GIMP_TRANSFORM_FUNCTION_MOVE,
                      "outside-function",        GIMP_TRANSFORM_FUNCTION_ROTATE,
                      "use-corner-handles",      TRUE,
                      "use-perspective-handles", TRUE,
                      "use-side-handles",        TRUE,
                      "use-shear-handles",       TRUE,
                      "use-pivot-handle",        TRUE,
                      NULL);

        g_signal_connect (controller->widget, "changed",
                          G_CALLBACK (gimp_filter_tool_transform_grid_changed),
                          controller);

        *set_func      = (GCallback) gimp_filter_tool_set_transform_grid;
        *set_func_data = controller;
      }
      break;

    case GIMP_CONTROLLER_TYPE_GYROSCOPE:
      {
        GeglRectangle area;
        gint          off_x, off_y;

        gimp_filter_tool_get_drawable_area (filter_tool, &off_x, &off_y, &area);

        controller->widget = gimp_tool_gyroscope_new (shell);

        g_object_set (controller->widget,
                      "speed",   1.0 / MAX (area.width, area.height),
                      "pivot-x", off_x + area.x + area.width  / 2.0,
                      "pivot-y", off_y + area.y + area.height / 2.0,
                      NULL);

        g_signal_connect (controller->widget, "changed",
                          G_CALLBACK (gimp_filter_tool_gyroscope_changed),
                          controller);

        *set_func      = (GCallback) gimp_filter_tool_set_gyroscope;
        *set_func_data = controller;
      }
      break;
    }

  g_object_set_data_full (G_OBJECT (controller->widget),
                          "controller", controller,
                          (GDestroyNotify) g_free);

  return controller->widget;
}

void
gimp_filter_tool_reset_widget (GimpFilterTool *filter_tool,
                               GimpToolWidget *widget)
{
  Controller *controller;

  g_return_if_fail (GIMP_IS_FILTER_TOOL (filter_tool));
  g_return_if_fail (GIMP_IS_TOOL_WIDGET (widget));

  controller = g_object_get_data (G_OBJECT (filter_tool->widget),
                                  "controller");

  g_return_if_fail (controller != NULL);

  switch (controller->controller_type)
    {
    case GIMP_CONTROLLER_TYPE_TRANSFORM_GRID:
      {
        GimpMatrix3   *transform;
        gint           off_x, off_y;
        GeglRectangle  area;
        gdouble        x1, y1;
        gdouble        x2, y2;
        gdouble        pivot_x, pivot_y;

        g_object_get (controller->widget,
                      "transform", &transform,
                      NULL);

        gimp_filter_tool_get_drawable_area (filter_tool, &off_x, &off_y, &area);

        x1 = off_x + area.x;
        y1 = off_y + area.y;
        x2 = x1 + area.width;
        y2 = y1 + area.height;

        gimp_matrix3_transform_point (transform,
                                      (x1 + x2) / 2.0, (y1 + y2) / 2.0,
                                      &pivot_x, &pivot_y);

        g_signal_handlers_block_by_func (controller->widget,
                                         gimp_filter_tool_transform_grid_changed,
                                         controller);

        g_object_set (controller->widget,
                      "pivot-x", pivot_x,
                      "pivot-y", pivot_y,
                      NULL);

        g_signal_handlers_unblock_by_func (controller->widget,
                                           gimp_filter_tool_transform_grid_changed,
                                           controller);

        g_free (transform);
      }
      break;

    default:
      break;
    }
}


/*  private functions  */

static void
gimp_filter_tool_set_line (Controller    *controller,
                           GeglRectangle *area,
                           gdouble        x1,
                           gdouble        y1,
                           gdouble        x2,
                           gdouble        y2)
{
  GimpTool     *tool     = GIMP_TOOL (controller->filter_tool);
  GimpDrawable *drawable = tool->drawable;

  if (drawable)
    {
      gint off_x, off_y;

      gimp_item_get_offset (GIMP_ITEM (drawable), &off_x, &off_y);

      x1 += off_x + area->x;
      y1 += off_y + area->y;
      x2 += off_x + area->x;
      y2 += off_y + area->y;
    }

  g_signal_handlers_block_by_func (controller->widget,
                                   gimp_filter_tool_line_changed,
                                   controller);

  g_object_set (controller->widget,
                "x1", x1,
                "y1", y1,
                "x2", x2,
                "y2", y2,
                NULL);

  g_signal_handlers_unblock_by_func (controller->widget,
                                     gimp_filter_tool_line_changed,
                                     controller);
}

static void
gimp_filter_tool_line_changed (GimpToolWidget *widget,
                               Controller     *controller)
{
  GimpFilterTool             *filter_tool = controller->filter_tool;
  GimpControllerLineCallback  line_callback;
  gdouble                     x1, y1, x2, y2;
  gint                        off_x, off_y;
  GeglRectangle               area;

  line_callback = (GimpControllerLineCallback) controller->creator_callback;

  g_object_get (widget,
                "x1", &x1,
                "y1", &y1,
                "x2", &x2,
                "y2", &y2,
                NULL);

  gimp_filter_tool_get_drawable_area (filter_tool, &off_x, &off_y, &area);

  x1 -= off_x + area.x;
  y1 -= off_y + area.y;
  x2 -= off_x + area.x;
  y2 -= off_y + area.y;

  line_callback (controller->creator_data,
                 &area, x1, y1, x2, y2);
}

static void
gimp_filter_tool_set_slider_line (Controller                 *controller,
                                  GeglRectangle              *area,
                                  gdouble                     x1,
                                  gdouble                     y1,
                                  gdouble                     x2,
                                  gdouble                     y2,
                                  const GimpControllerSlider *sliders,
                                  gint                        n_sliders)
{
  GimpTool     *tool     = GIMP_TOOL (controller->filter_tool);
  GimpDrawable *drawable = tool->drawable;

  if (drawable)
    {
      gint off_x, off_y;

      gimp_item_get_offset (GIMP_ITEM (drawable), &off_x, &off_y);

      x1 += off_x + area->x;
      y1 += off_y + area->y;
      x2 += off_x + area->x;
      y2 += off_y + area->y;
    }

  g_signal_handlers_block_by_func (controller->widget,
                                   gimp_filter_tool_slider_line_changed,
                                   controller);

  g_object_set (controller->widget,
                "x1", x1,
                "y1", y1,
                "x2", x2,
                "y2", y2,
                NULL);

  gimp_tool_line_set_sliders (GIMP_TOOL_LINE (controller->widget),
                              sliders, n_sliders);

  g_signal_handlers_unblock_by_func (controller->widget,
                                     gimp_filter_tool_slider_line_changed,
                                     controller);
}

static void
gimp_filter_tool_slider_line_changed (GimpToolWidget *widget,
                                      Controller     *controller)
{
  GimpFilterTool                   *filter_tool = controller->filter_tool;
  GimpControllerSliderLineCallback  slider_line_callback;
  gdouble                           x1, y1, x2, y2;
  const GimpControllerSlider       *sliders;
  gint                              n_sliders;
  gint                              off_x, off_y;
  GeglRectangle                     area;

  slider_line_callback =
    (GimpControllerSliderLineCallback) controller->creator_callback;

  g_object_get (widget,
                "x1", &x1,
                "y1", &y1,
                "x2", &x2,
                "y2", &y2,
                NULL);

  sliders = gimp_tool_line_get_sliders (GIMP_TOOL_LINE (controller->widget),
                                        &n_sliders);

  gimp_filter_tool_get_drawable_area (filter_tool, &off_x, &off_y, &area);

  x1 -= off_x + area.x;
  y1 -= off_y + area.y;
  x2 -= off_x + area.x;
  y2 -= off_y + area.y;

  slider_line_callback (controller->creator_data,
                        &area, x1, y1, x2, y2, sliders, n_sliders);
}

static void
gimp_filter_tool_set_transform_grid (Controller        *controller,
                                     GeglRectangle     *area,
                                     const GimpMatrix3 *transform)
{
  GimpTool     *tool     = GIMP_TOOL (controller->filter_tool);
  GimpDrawable *drawable = tool->drawable;
  gdouble       x1       = area->x;
  gdouble       y1       = area->y;
  gdouble       x2       = area->x + area->width;
  gdouble       y2       = area->y + area->height;
  GimpMatrix3   matrix;

  if (drawable)
    {
      gint off_x, off_y;

      gimp_item_get_offset (GIMP_ITEM (drawable), &off_x, &off_y);

      x1 += off_x;
      y1 += off_y;
      x2 += off_x;
      y2 += off_y;
    }

  gimp_matrix3_identity (&matrix);
  gimp_matrix3_translate (&matrix, -x1, -y1);
  gimp_matrix3_mult (transform, &matrix);
  gimp_matrix3_translate (&matrix, +x1, +y1);

  g_signal_handlers_block_by_func (controller->widget,
                                   gimp_filter_tool_transform_grid_changed,
                                   controller);

  g_object_set (controller->widget,
                "transform", &matrix,
                "x1",        x1,
                "y1",        y1,
                "x2",        x2,
                "y2",        y2,
                NULL);

  g_signal_handlers_unblock_by_func (controller->widget,
                                     gimp_filter_tool_transform_grid_changed,
                                     controller);
}

static void
gimp_filter_tool_transform_grid_changed (GimpToolWidget *widget,
                                         Controller     *controller)
{
  GimpFilterTool                      *filter_tool = controller->filter_tool;
  GimpControllerTransformGridCallback  transform_grid_callback;
  gint                                 off_x, off_y;
  GeglRectangle                        area;
  GimpMatrix3                         *transform;
  GimpMatrix3                          matrix;

  transform_grid_callback =
    (GimpControllerTransformGridCallback) controller->creator_callback;

  g_object_get (widget,
                "transform", &transform,
                NULL);

  gimp_filter_tool_get_drawable_area (filter_tool, &off_x, &off_y, &area);

  gimp_matrix3_identity (&matrix);
  gimp_matrix3_translate (&matrix, +(off_x + area.x), +(off_y + area.y));
  gimp_matrix3_mult (transform, &matrix);
  gimp_matrix3_translate (&matrix, -(off_x + area.x), -(off_y + area.y));

  transform_grid_callback (controller->creator_data,
                           &area, &matrix);

  g_free (transform);
}

static void
gimp_filter_tool_set_gyroscope (Controller    *controller,
                                GeglRectangle *area,
                                gdouble        yaw,
                                gdouble        pitch,
                                gdouble        roll,
                                gdouble        zoom,
                                gboolean       invert)
{
  g_signal_handlers_block_by_func (controller->widget,
                                   gimp_filter_tool_gyroscope_changed,
                                   controller);

  g_object_set (controller->widget,
                "yaw",    yaw,
                "pitch",  pitch,
                "roll",   roll,
                "zoom",   zoom,
                "invert", invert,
                NULL);

  g_signal_handlers_unblock_by_func (controller->widget,
                                     gimp_filter_tool_gyroscope_changed,
                                     controller);
}

static void   gimp_filter_tool_gyroscope_changed (GimpToolWidget *widget,
                                                  Controller     *controller)
{
  GimpFilterTool                  *filter_tool = controller->filter_tool;
  GimpControllerGyroscopeCallback  gyroscope_callback;
  gint                             off_x, off_y;
  GeglRectangle                    area;
  gdouble                          yaw;
  gdouble                          pitch;
  gdouble                          roll;
  gdouble                          zoom;
  gboolean                         invert;

  gyroscope_callback =
    (GimpControllerGyroscopeCallback) controller->creator_callback;

  gimp_filter_tool_get_drawable_area (filter_tool, &off_x, &off_y, &area);

  g_object_get (widget,
                "yaw",    &yaw,
                "pitch",  &pitch,
                "roll",   &roll,
                "zoom",   &zoom,
                "invert", &invert,
                NULL);

  gyroscope_callback (controller->creator_data,
                      &area, yaw, pitch, roll, zoom, invert);
}
