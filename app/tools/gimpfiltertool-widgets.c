/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ligmafiltertool-widgets.c
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

#include <string.h>

#include <gegl.h>
#include <gtk/gtk.h>

#include "libligmamath/ligmamath.h"

#include "tools-types.h"

#include "core/ligmacontainer.h"
#include "core/ligmaitem.h"

#include "display/ligmadisplay.h"
#include "display/ligmatoolfocus.h"
#include "display/ligmatoolgyroscope.h"
#include "display/ligmatoolline.h"
#include "display/ligmatooltransformgrid.h"
#include "display/ligmatoolwidgetgroup.h"

#include "ligmafilteroptions.h"
#include "ligmafiltertool.h"
#include "ligmafiltertool-widgets.h"


typedef struct _Controller Controller;

struct _Controller
{
  LigmaFilterTool     *filter_tool;
  LigmaControllerType  controller_type;
  LigmaToolWidget     *widget;
  GCallback           creator_callback;
  gpointer            creator_data;
};


/*  local function prototypes  */

static Controller * ligma_filter_tool_controller_new          (void);
static void         ligma_filter_tool_controller_free         (Controller                 *controller);

static void         ligma_filter_tool_set_line                (Controller                 *controller,
                                                              GeglRectangle              *area,
                                                              gdouble                     x1,
                                                              gdouble                     y1,
                                                              gdouble                     x2,
                                                              gdouble                     y2);
static void         ligma_filter_tool_line_changed            (LigmaToolWidget             *widget,
                                                              Controller                 *controller);

static void         ligma_filter_tool_set_slider_line         (Controller                 *controller,
                                                              GeglRectangle              *area,
                                                              gdouble                     x1,
                                                              gdouble                     y1,
                                                              gdouble                     x2,
                                                              gdouble                     y2,
                                                              const LigmaControllerSlider *sliders,
                                                              gint                        n_sliders);
static void         ligma_filter_tool_slider_line_changed     (LigmaToolWidget             *widget,
                                                              Controller                 *controller);

static void         ligma_filter_tool_set_transform_grid      (Controller                 *controller,
                                                              GeglRectangle              *area,
                                                              const LigmaMatrix3          *transform);
static void         ligma_filter_tool_transform_grid_changed  (LigmaToolWidget             *widget,
                                                              Controller                 *controller);

static void         ligma_filter_tool_set_transform_grids     (Controller                 *controller,
                                                              GeglRectangle              *area,
                                                              const LigmaMatrix3          *transforms,
                                                              gint                        n_transforms);
static void         ligma_filter_tool_transform_grids_changed (LigmaToolWidget             *widget,
                                                              Controller                 *controller);

static void         ligma_filter_tool_set_gyroscope           (Controller                 *controller,
                                                              GeglRectangle              *area,
                                                              gdouble                     yaw,
                                                              gdouble                     pitch,
                                                              gdouble                     roll,
                                                              gdouble                     zoom,
                                                              gboolean                    invert);
static void         ligma_filter_tool_gyroscope_changed       (LigmaToolWidget             *widget,
                                                              Controller                 *controller);

static void         ligma_filter_tool_set_focus               (Controller                 *controller,
                                                              GeglRectangle              *area,
                                                              LigmaLimitType               type,
                                                              gdouble                     x,
                                                              gdouble                     y,
                                                              gdouble                     radius,
                                                              gdouble                     aspect_ratio,
                                                              gdouble                     angle,
                                                              gdouble                     inner_limit,
                                                              gdouble                     midpoint);
static void         ligma_filter_tool_focus_changed           (LigmaToolWidget             *widget,
                                                              Controller                 *controller);


/*  public functions  */

LigmaToolWidget *
ligma_filter_tool_create_widget (LigmaFilterTool     *filter_tool,
                                LigmaControllerType  controller_type,
                                const gchar        *status_title,
                                GCallback           callback,
                                gpointer            callback_data,
                                GCallback          *set_func,
                                gpointer           *set_func_data)
{
  LigmaTool         *tool;
  LigmaDisplayShell *shell;
  Controller       *controller;

  g_return_val_if_fail (LIGMA_IS_FILTER_TOOL (filter_tool), NULL);
  g_return_val_if_fail (filter_tool->config != NULL, NULL);

  tool = LIGMA_TOOL (filter_tool);

  g_return_val_if_fail (tool->display != NULL, NULL);

  shell = ligma_display_get_shell (tool->display);

  controller = ligma_filter_tool_controller_new ();

  controller->filter_tool      = filter_tool;
  controller->controller_type  = controller_type;
  controller->creator_callback = callback;
  controller->creator_data     = callback_data;

  switch (controller_type)
    {
    case LIGMA_CONTROLLER_TYPE_LINE:
      controller->widget = ligma_tool_line_new (shell, 100, 100, 500, 500);

      g_object_set (controller->widget,
                    "status-title", status_title,
                    NULL);

      g_signal_connect (controller->widget, "changed",
                        G_CALLBACK (ligma_filter_tool_line_changed),
                        controller);

      *set_func      = (GCallback) ligma_filter_tool_set_line;
      *set_func_data = controller;
      break;

    case LIGMA_CONTROLLER_TYPE_SLIDER_LINE:
      controller->widget = ligma_tool_line_new (shell, 100, 100, 500, 500);

      g_object_set (controller->widget,
                    "status-title", status_title,
                    NULL);

      g_signal_connect (controller->widget, "changed",
                        G_CALLBACK (ligma_filter_tool_slider_line_changed),
                        controller);

      *set_func      = (GCallback) ligma_filter_tool_set_slider_line;
      *set_func_data = controller;
      break;

    case LIGMA_CONTROLLER_TYPE_TRANSFORM_GRID:
      {
        LigmaMatrix3   transform;
        gint          off_x, off_y;
        GeglRectangle area;
        gdouble       x1, y1;
        gdouble       x2, y2;

        ligma_matrix3_identity (&transform);

        ligma_filter_tool_get_drawable_area (filter_tool, &off_x, &off_y, &area);

        x1 = off_x + area.x;
        y1 = off_y + area.y;
        x2 = x1 + area.width;
        y2 = y1 + area.height;

        controller->widget = ligma_tool_transform_grid_new (shell, &transform,
                                                           x1, y1, x2, y2);

        g_object_set (controller->widget,
                      "pivot-x",                 (x1 + x2) / 2.0,
                      "pivot-y",                 (y1 + y2) / 2.0,
                      "inside-function",         LIGMA_TRANSFORM_FUNCTION_MOVE,
                      "outside-function",        LIGMA_TRANSFORM_FUNCTION_ROTATE,
                      "use-corner-handles",      TRUE,
                      "use-perspective-handles", TRUE,
                      "use-side-handles",        TRUE,
                      "use-shear-handles",       TRUE,
                      "use-pivot-handle",        TRUE,
                      NULL);

        g_signal_connect (controller->widget, "changed",
                          G_CALLBACK (ligma_filter_tool_transform_grid_changed),
                          controller);

        *set_func      = (GCallback) ligma_filter_tool_set_transform_grid;
        *set_func_data = controller;
      }
      break;

    case LIGMA_CONTROLLER_TYPE_TRANSFORM_GRIDS:
      {
        controller->widget = ligma_tool_widget_group_new (shell);

        ligma_tool_widget_group_set_auto_raise (
          LIGMA_TOOL_WIDGET_GROUP (controller->widget), TRUE);

        g_signal_connect (controller->widget, "changed",
                          G_CALLBACK (ligma_filter_tool_transform_grids_changed),
                          controller);

        *set_func      = (GCallback) ligma_filter_tool_set_transform_grids;
        *set_func_data = controller;
      }
      break;

    case LIGMA_CONTROLLER_TYPE_GYROSCOPE:
      {
        GeglRectangle area;
        gint          off_x, off_y;

        ligma_filter_tool_get_drawable_area (filter_tool, &off_x, &off_y, &area);

        controller->widget = ligma_tool_gyroscope_new (shell);

        g_object_set (controller->widget,
                      "speed",   1.0 / MAX (area.width, area.height),
                      "pivot-x", off_x + area.x + area.width  / 2.0,
                      "pivot-y", off_y + area.y + area.height / 2.0,
                      NULL);

        g_signal_connect (controller->widget, "changed",
                          G_CALLBACK (ligma_filter_tool_gyroscope_changed),
                          controller);

        *set_func      = (GCallback) ligma_filter_tool_set_gyroscope;
        *set_func_data = controller;
      }
      break;

    case LIGMA_CONTROLLER_TYPE_FOCUS:
      controller->widget = ligma_tool_focus_new (shell);

      g_signal_connect (controller->widget, "changed",
                        G_CALLBACK (ligma_filter_tool_focus_changed),
                        controller);

      *set_func      = (GCallback) ligma_filter_tool_set_focus;
      *set_func_data = controller;
      break;
    }

  g_object_add_weak_pointer (G_OBJECT (controller->widget),
                            (gpointer) &controller->widget);

  g_object_set_data_full (filter_tool->config,
                          "ligma-filter-tool-controller", controller,
                          (GDestroyNotify) ligma_filter_tool_controller_free);

  return controller->widget;
}

static void
ligma_filter_tool_reset_transform_grid (LigmaToolWidget *widget,
                                       LigmaFilterTool *filter_tool)
{
  LigmaMatrix3   *transform;
  gint           off_x, off_y;
  GeglRectangle  area;
  gdouble        x1, y1;
  gdouble        x2, y2;
  gdouble        pivot_x, pivot_y;

  g_object_get (widget,
                "transform", &transform,
                NULL);

  ligma_filter_tool_get_drawable_area (filter_tool, &off_x, &off_y, &area);

  x1 = off_x + area.x;
  y1 = off_y + area.y;
  x2 = x1 + area.width;
  y2 = y1 + area.height;

  ligma_matrix3_transform_point (transform,
                                (x1 + x2) / 2.0, (y1 + y2) / 2.0,
                                &pivot_x, &pivot_y);

  g_object_set (widget,
                "pivot-x", pivot_x,
                "pivot-y", pivot_y,
                NULL);

  g_free (transform);
}

void
ligma_filter_tool_reset_widget (LigmaFilterTool *filter_tool,
                               LigmaToolWidget *widget)
{
  Controller *controller;

  g_return_if_fail (LIGMA_IS_FILTER_TOOL (filter_tool));
  g_return_if_fail (LIGMA_IS_TOOL_WIDGET (widget));
  g_return_if_fail (filter_tool->config != NULL);

  controller = g_object_get_data (filter_tool->config,
                                  "ligma-filter-tool-controller");

  g_return_if_fail (controller != NULL);

  switch (controller->controller_type)
    {
    case LIGMA_CONTROLLER_TYPE_TRANSFORM_GRID:
      {
        g_signal_handlers_block_by_func (controller->widget,
                                         ligma_filter_tool_transform_grid_changed,
                                         controller);

        ligma_filter_tool_reset_transform_grid (controller->widget, filter_tool);

        g_signal_handlers_unblock_by_func (controller->widget,
                                           ligma_filter_tool_transform_grid_changed,
                                           controller);
      }
      break;

    case LIGMA_CONTROLLER_TYPE_TRANSFORM_GRIDS:
      {
        g_signal_handlers_block_by_func (controller->widget,
                                         ligma_filter_tool_transform_grids_changed,
                                         controller);

        ligma_container_foreach (
          ligma_tool_widget_group_get_children (
            LIGMA_TOOL_WIDGET_GROUP (controller->widget)),
          (GFunc) ligma_filter_tool_reset_transform_grid,
          filter_tool);

        g_signal_handlers_unblock_by_func (controller->widget,
                                           ligma_filter_tool_transform_grids_changed,
                                           controller);
      }
      break;

    default:
      break;
    }
}


/*  private functions  */

static Controller *
ligma_filter_tool_controller_new (void)
{
  return g_slice_new0 (Controller);
}

static void
ligma_filter_tool_controller_free (Controller *controller)
{
  if (controller->widget)
    {
      g_signal_handlers_disconnect_by_data (controller->widget, controller);

      g_object_remove_weak_pointer (G_OBJECT (controller->widget),
                                    (gpointer) &controller->widget);
    }

  g_slice_free (Controller, controller);
}

static void
ligma_filter_tool_set_line (Controller    *controller,
                           GeglRectangle *area,
                           gdouble        x1,
                           gdouble        y1,
                           gdouble        x2,
                           gdouble        y2)
{
  LigmaTool     *tool;
  LigmaDrawable *drawable;

  if (! controller->widget)
    return;

  tool     = LIGMA_TOOL (controller->filter_tool);
  drawable = tool->drawables->data;

  if (drawable)
    {
      gint off_x, off_y;

      ligma_item_get_offset (LIGMA_ITEM (drawable), &off_x, &off_y);

      x1 += off_x + area->x;
      y1 += off_y + area->y;
      x2 += off_x + area->x;
      y2 += off_y + area->y;
    }

  g_signal_handlers_block_by_func (controller->widget,
                                   ligma_filter_tool_line_changed,
                                   controller);

  g_object_set (controller->widget,
                "x1", x1,
                "y1", y1,
                "x2", x2,
                "y2", y2,
                NULL);

  g_signal_handlers_unblock_by_func (controller->widget,
                                     ligma_filter_tool_line_changed,
                                     controller);
}

static void
ligma_filter_tool_line_changed (LigmaToolWidget *widget,
                               Controller     *controller)
{
  LigmaFilterTool             *filter_tool = controller->filter_tool;
  LigmaControllerLineCallback  line_callback;
  gdouble                     x1, y1, x2, y2;
  gint                        off_x, off_y;
  GeglRectangle               area;

  line_callback = (LigmaControllerLineCallback) controller->creator_callback;

  g_object_get (widget,
                "x1", &x1,
                "y1", &y1,
                "x2", &x2,
                "y2", &y2,
                NULL);

  ligma_filter_tool_get_drawable_area (filter_tool, &off_x, &off_y, &area);

  x1 -= off_x + area.x;
  y1 -= off_y + area.y;
  x2 -= off_x + area.x;
  y2 -= off_y + area.y;

  line_callback (controller->creator_data,
                 &area, x1, y1, x2, y2);
}

static void
ligma_filter_tool_set_slider_line (Controller                 *controller,
                                  GeglRectangle              *area,
                                  gdouble                     x1,
                                  gdouble                     y1,
                                  gdouble                     x2,
                                  gdouble                     y2,
                                  const LigmaControllerSlider *sliders,
                                  gint                        n_sliders)
{
  LigmaTool     *tool;
  LigmaDrawable *drawable;

  if (! controller->widget)
    return;

  tool     = LIGMA_TOOL (controller->filter_tool);
  drawable = tool->drawables->data;

  if (drawable)
    {
      gint off_x, off_y;

      ligma_item_get_offset (LIGMA_ITEM (drawable), &off_x, &off_y);

      x1 += off_x + area->x;
      y1 += off_y + area->y;
      x2 += off_x + area->x;
      y2 += off_y + area->y;
    }

  g_signal_handlers_block_by_func (controller->widget,
                                   ligma_filter_tool_slider_line_changed,
                                   controller);

  g_object_set (controller->widget,
                "x1", x1,
                "y1", y1,
                "x2", x2,
                "y2", y2,
                NULL);

  ligma_tool_line_set_sliders (LIGMA_TOOL_LINE (controller->widget),
                              sliders, n_sliders);

  g_signal_handlers_unblock_by_func (controller->widget,
                                     ligma_filter_tool_slider_line_changed,
                                     controller);
}

static void
ligma_filter_tool_slider_line_changed (LigmaToolWidget *widget,
                                      Controller     *controller)
{
  LigmaFilterTool                   *filter_tool = controller->filter_tool;
  LigmaControllerSliderLineCallback  slider_line_callback;
  gdouble                           x1, y1, x2, y2;
  const LigmaControllerSlider       *sliders;
  gint                              n_sliders;
  gint                              off_x, off_y;
  GeglRectangle                     area;

  slider_line_callback =
    (LigmaControllerSliderLineCallback) controller->creator_callback;

  g_object_get (widget,
                "x1", &x1,
                "y1", &y1,
                "x2", &x2,
                "y2", &y2,
                NULL);

  sliders = ligma_tool_line_get_sliders (LIGMA_TOOL_LINE (controller->widget),
                                        &n_sliders);

  ligma_filter_tool_get_drawable_area (filter_tool, &off_x, &off_y, &area);

  x1 -= off_x + area.x;
  y1 -= off_y + area.y;
  x2 -= off_x + area.x;
  y2 -= off_y + area.y;

  slider_line_callback (controller->creator_data,
                        &area, x1, y1, x2, y2, sliders, n_sliders);
}

static void
ligma_filter_tool_set_transform_grid (Controller        *controller,
                                     GeglRectangle     *area,
                                     const LigmaMatrix3 *transform)
{
  LigmaTool     *tool;
  LigmaDrawable *drawable;
  gdouble       x1 = area->x;
  gdouble       y1 = area->y;
  gdouble       x2 = area->x + area->width;
  gdouble       y2 = area->y + area->height;
  LigmaMatrix3   matrix;

  if (! controller->widget)
    return;

  tool     = LIGMA_TOOL (controller->filter_tool);
  drawable = tool->drawables->data;

  if (drawable)
    {
      gint off_x, off_y;

      ligma_item_get_offset (LIGMA_ITEM (drawable), &off_x, &off_y);

      x1 += off_x;
      y1 += off_y;
      x2 += off_x;
      y2 += off_y;
    }

  ligma_matrix3_identity (&matrix);
  ligma_matrix3_translate (&matrix, -x1, -y1);
  ligma_matrix3_mult (transform, &matrix);
  ligma_matrix3_translate (&matrix, +x1, +y1);

  g_signal_handlers_block_by_func (controller->widget,
                                   ligma_filter_tool_transform_grid_changed,
                                   controller);

  g_object_set (controller->widget,
                "transform", &matrix,
                "x1",        x1,
                "y1",        y1,
                "x2",        x2,
                "y2",        y2,
                NULL);

  g_signal_handlers_unblock_by_func (controller->widget,
                                     ligma_filter_tool_transform_grid_changed,
                                     controller);
}

static void
ligma_filter_tool_transform_grid_changed (LigmaToolWidget *widget,
                                         Controller     *controller)
{
  LigmaFilterTool                      *filter_tool = controller->filter_tool;
  LigmaControllerTransformGridCallback  transform_grid_callback;
  gint                                 off_x, off_y;
  GeglRectangle                        area;
  LigmaMatrix3                         *transform;
  LigmaMatrix3                          matrix;

  transform_grid_callback =
    (LigmaControllerTransformGridCallback) controller->creator_callback;

  g_object_get (widget,
                "transform", &transform,
                NULL);

  ligma_filter_tool_get_drawable_area (filter_tool, &off_x, &off_y, &area);

  ligma_matrix3_identity (&matrix);
  ligma_matrix3_translate (&matrix, +(off_x + area.x), +(off_y + area.y));
  ligma_matrix3_mult (transform, &matrix);
  ligma_matrix3_translate (&matrix, -(off_x + area.x), -(off_y + area.y));

  transform_grid_callback (controller->creator_data,
                           &area, &matrix);

  g_free (transform);
}

static void
ligma_filter_tool_set_transform_grids (Controller        *controller,
                                      GeglRectangle     *area,
                                      const LigmaMatrix3 *transforms,
                                      gint               n_transforms)
{
  LigmaTool         *tool;
  LigmaDisplayShell *shell;
  LigmaDrawable     *drawable;
  LigmaContainer    *grids;
  LigmaToolWidget   *grid = NULL;
  gdouble           x1   = area->x;
  gdouble           y1   = area->y;
  gdouble           x2   = area->x + area->width;
  gdouble           y2   = area->y + area->height;
  LigmaMatrix3       matrix;
  gint              i;

  if (! controller->widget)
    return;

  tool     = LIGMA_TOOL (controller->filter_tool);
  shell    = ligma_display_get_shell (tool->display);
  drawable = tool->drawables->data;

  g_signal_handlers_block_by_func (controller->widget,
                                   ligma_filter_tool_transform_grids_changed,
                                   controller);

  if (drawable)
    {
      gint off_x, off_y;

      ligma_item_get_offset (LIGMA_ITEM (drawable), &off_x, &off_y);

      x1 += off_x;
      y1 += off_y;
      x2 += off_x;
      y2 += off_y;
    }

  grids = ligma_tool_widget_group_get_children (
    LIGMA_TOOL_WIDGET_GROUP (controller->widget));

  if (n_transforms > ligma_container_get_n_children (grids))
    {
      ligma_matrix3_identity (&matrix);

      for (i = ligma_container_get_n_children (grids); i < n_transforms; i++)
        {
          gdouble pivot_x;
          gdouble pivot_y;

          grid = ligma_tool_transform_grid_new (shell, &matrix, x1, y1, x2, y2);

          if (i > 0 && ! memcmp (&transforms[i], &transforms[i - 1],
                                 sizeof (LigmaMatrix3)))
            {
              g_object_get (ligma_container_get_last_child (grids),
                            "pivot-x", &pivot_x,
                            "pivot-y", &pivot_y,
                            NULL);
            }
          else
            {
              pivot_x = (x1 + x2) / 2.0;
              pivot_y = (y1 + y2) / 2.0;

              ligma_matrix3_transform_point (&transforms[i],
                                            pivot_x, pivot_y,
                                            &pivot_x, &pivot_y);
            }

          g_object_set (grid,
                        "pivot-x",                 pivot_x,
                        "pivot-y",                 pivot_y,
                        "inside-function",         LIGMA_TRANSFORM_FUNCTION_MOVE,
                        "outside-function",        LIGMA_TRANSFORM_FUNCTION_ROTATE,
                        "use-corner-handles",      TRUE,
                        "use-perspective-handles", TRUE,
                        "use-side-handles",        TRUE,
                        "use-shear-handles",       TRUE,
                        "use-pivot-handle",        TRUE,
                        NULL);

          ligma_container_add (grids, LIGMA_OBJECT (grid));

          g_object_unref (grid);
        }

      ligma_tool_widget_set_focus (grid, TRUE);
    }
  else
    {
      while (ligma_container_get_n_children (grids) > n_transforms)
        ligma_container_remove (grids, ligma_container_get_last_child (grids));
    }

  for (i = 0; i < n_transforms; i++)
    {
      ligma_matrix3_identity (&matrix);
      ligma_matrix3_translate (&matrix, -x1, -y1);
      ligma_matrix3_mult (&transforms[i], &matrix);
      ligma_matrix3_translate (&matrix, +x1, +y1);

      g_object_set (ligma_container_get_child_by_index (grids, i),
                    "transform", &matrix,
                    "x1",        x1,
                    "y1",        y1,
                    "x2",        x2,
                    "y2",        y2,
                    NULL);
    }

  g_signal_handlers_unblock_by_func (controller->widget,
                                     ligma_filter_tool_transform_grids_changed,
                                     controller);
}

static void
ligma_filter_tool_transform_grids_changed (LigmaToolWidget *widget,
                                          Controller     *controller)
{
  LigmaFilterTool                       *filter_tool = controller->filter_tool;
  LigmaControllerTransformGridsCallback  transform_grids_callback;
  LigmaContainer                        *grids;
  gint                                  off_x, off_y;
  GeglRectangle                         area;
  LigmaMatrix3                          *transforms;
  gint                                  n_transforms;
  gint                                  i;

  transform_grids_callback =
    (LigmaControllerTransformGridsCallback) controller->creator_callback;

  grids = ligma_tool_widget_group_get_children (
    LIGMA_TOOL_WIDGET_GROUP (controller->widget));

  ligma_filter_tool_get_drawable_area (filter_tool, &off_x, &off_y, &area);

  n_transforms = ligma_container_get_n_children (grids);
  transforms   = g_new (LigmaMatrix3, n_transforms);

  for (i = 0; i < n_transforms; i++)
    {
      LigmaMatrix3 *transform;

      g_object_get (ligma_container_get_child_by_index (grids, i),
                    "transform", &transform,
                    NULL);

      ligma_matrix3_identity (&transforms[i]);
      ligma_matrix3_translate (&transforms[i],
                              +(off_x + area.x), +(off_y + area.y));
      ligma_matrix3_mult (transform, &transforms[i]);
      ligma_matrix3_translate (&transforms[i],
                              -(off_x + area.x), -(off_y + area.y));

      g_free (transform);
    }

  transform_grids_callback (controller->creator_data,
                            &area, transforms, n_transforms);

  g_free (transforms);
}

static void
ligma_filter_tool_set_gyroscope (Controller    *controller,
                                GeglRectangle *area,
                                gdouble        yaw,
                                gdouble        pitch,
                                gdouble        roll,
                                gdouble        zoom,
                                gboolean       invert)
{
  if (! controller->widget)
    return;

  g_signal_handlers_block_by_func (controller->widget,
                                   ligma_filter_tool_gyroscope_changed,
                                   controller);

  g_object_set (controller->widget,
                "yaw",    yaw,
                "pitch",  pitch,
                "roll",   roll,
                "zoom",   zoom,
                "invert", invert,
                NULL);

  g_signal_handlers_unblock_by_func (controller->widget,
                                     ligma_filter_tool_gyroscope_changed,
                                     controller);
}

static void
ligma_filter_tool_gyroscope_changed (LigmaToolWidget *widget,
                                    Controller     *controller)
{
  LigmaFilterTool                  *filter_tool = controller->filter_tool;
  LigmaControllerGyroscopeCallback  gyroscope_callback;
  gint                             off_x, off_y;
  GeglRectangle                    area;
  gdouble                          yaw;
  gdouble                          pitch;
  gdouble                          roll;
  gdouble                          zoom;
  gboolean                         invert;

  gyroscope_callback =
    (LigmaControllerGyroscopeCallback) controller->creator_callback;

  ligma_filter_tool_get_drawable_area (filter_tool, &off_x, &off_y, &area);

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

static void
ligma_filter_tool_set_focus (Controller    *controller,
                            GeglRectangle *area,
                            LigmaLimitType  type,
                            gdouble        x,
                            gdouble        y,
                            gdouble        radius,
                            gdouble        aspect_ratio,
                            gdouble        angle,
                            gdouble        inner_limit,
                            gdouble        midpoint)
{
  LigmaTool     *tool;
  LigmaDrawable *drawable;

  if (! controller->widget)
    return;

  tool     = LIGMA_TOOL (controller->filter_tool);
  drawable = tool->drawables->data;

  if (drawable)
    {
      gint off_x, off_y;

      ligma_item_get_offset (LIGMA_ITEM (drawable), &off_x, &off_y);

      x += off_x + area->x;
      y += off_y + area->y;
    }

  g_signal_handlers_block_by_func (controller->widget,
                                   ligma_filter_tool_focus_changed,
                                   controller);

  g_object_set (controller->widget,
                "type",         type,
                "x",            x,
                "y",            y,
                "radius",       radius,
                "aspect-ratio", aspect_ratio,
                "angle",        angle,
                "inner-limit",  inner_limit,
                "midpoint",     midpoint,
                NULL);

  g_signal_handlers_unblock_by_func (controller->widget,
                                     ligma_filter_tool_focus_changed,
                                     controller);
}

static void
ligma_filter_tool_focus_changed (LigmaToolWidget *widget,
                                Controller     *controller)
{
  LigmaFilterTool              *filter_tool = controller->filter_tool;
  LigmaControllerFocusCallback  focus_callback;
  LigmaLimitType                type;
  gdouble                      x,  y;
  gdouble                      radius;
  gdouble                      aspect_ratio;
  gdouble                      angle;
  gdouble                      inner_limit;
  gdouble                      midpoint;
  gint                         off_x, off_y;
  GeglRectangle                area;

  focus_callback = (LigmaControllerFocusCallback) controller->creator_callback;

  g_object_get (widget,
                "type",         &type,
                "x",            &x,
                "y",            &y,
                "radius",       &radius,
                "aspect-ratio", &aspect_ratio,
                "angle",        &angle,
                "inner-limit",  &inner_limit,
                "midpoint",     &midpoint,
                NULL);

  ligma_filter_tool_get_drawable_area (filter_tool, &off_x, &off_y, &area);

  x -= off_x + area.x;
  y -= off_y + area.y;

  focus_callback (controller->creator_data,
                  &area,
                  type,
                  x,
                  y,
                  radius,
                  aspect_ratio,
                  angle,
                  inner_limit,
                  midpoint);
}
