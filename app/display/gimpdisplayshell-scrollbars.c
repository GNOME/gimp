/* LIGMA - The GNU Image Manipulation Program
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

#include <math.h>

#include <gegl.h>
#include <gtk/gtk.h>

#include "display-types.h"

#include "core/ligmaimage.h"

#include "ligmadisplay.h"
#include "ligmadisplayshell.h"
#include "ligmadisplayshell-scale.h"
#include "ligmadisplayshell-scrollbars.h"


#define MINIMUM_STEP_AMOUNT 1.0


/**
 * ligma_display_shell_scrollbars_update:
 * @shell:
 *
 **/
void
ligma_display_shell_scrollbars_update (LigmaDisplayShell *shell)
{
  g_return_if_fail (LIGMA_IS_DISPLAY_SHELL (shell));

  if (! shell->display)
    return;

  /* Horizontal scrollbar */

  g_object_freeze_notify (G_OBJECT (shell->hsbdata));

  /* Update upper and lower value before we set the new value */
  ligma_display_shell_scrollbars_setup_horizontal (shell, shell->offset_x);

  g_object_set (shell->hsbdata,
                "value",          (gdouble) shell->offset_x,
                "page-size",      (gdouble) shell->disp_width,
                "page-increment", (gdouble) shell->disp_width / 2,
                NULL);

  g_object_thaw_notify (G_OBJECT (shell->hsbdata)); /* emits "changed" */


  /* Vertcal scrollbar */

  g_object_freeze_notify (G_OBJECT (shell->vsbdata));

  /* Update upper and lower value before we set the new value */
  ligma_display_shell_scrollbars_setup_vertical (shell, shell->offset_y);

  g_object_set (shell->vsbdata,
                "value",          (gdouble) shell->offset_y,
                "page-size",      (gdouble) shell->disp_height,
                "page-increment", (gdouble) shell->disp_height / 2,
                NULL);

  g_object_thaw_notify (G_OBJECT (shell->vsbdata)); /* emits "changed" */
}

/**
 * ligma_display_shell_scrollbars_setup_horizontal:
 * @shell:
 * @value:
 *
 * Setup the limits of the horizontal scrollbar
 **/
void
ligma_display_shell_scrollbars_setup_horizontal (LigmaDisplayShell *shell,
                                                gdouble           value)
{
  gint    bounds_x;
  gint    bounds_width;
  gint    bounding_box_x;
  gint    bounding_box_width;
  gint    x1;
  gint    x2;
  gdouble lower;
  gdouble upper;
  gdouble scale_x;

  g_return_if_fail (LIGMA_IS_DISPLAY_SHELL (shell));

  if (! shell->display || ! ligma_display_get_image (shell->display))
    return;

  ligma_display_shell_scale_get_image_bounds (shell,
                                             &bounds_x,     NULL,
                                             &bounds_width, NULL);

  if (! ligma_display_shell_get_infinite_canvas (shell))
    {
      bounding_box_x     = bounds_x;
      bounding_box_width = bounds_width;
    }
  else
    {
      ligma_display_shell_scale_get_image_bounding_box (
        shell,
        &bounding_box_x,     NULL,
        &bounding_box_width, NULL);
    }

  x1 = bounding_box_x;
  x2 = bounding_box_x + bounding_box_width;

  x1 = MIN (x1, bounds_x + bounds_width / 2 - shell->disp_width       / 2);
  x2 = MAX (x2, bounds_x + bounds_width / 2 + (shell->disp_width + 1) / 2);

  lower = MIN (value,                     x1);
  upper = MAX (value + shell->disp_width, x2);

  ligma_display_shell_get_rotated_scale (shell, &scale_x, NULL);

  g_object_set (shell->hsbdata,
                "lower",          lower,
                "upper",          upper,
                "step-increment", (gdouble) MAX (scale_x, MINIMUM_STEP_AMOUNT),
                NULL);
}

/**
 * ligma_display_shell_scrollbars_setup_vertical:
 * @shell:
 * @value:
 *
 * Setup the limits of the vertical scrollbar
 **/
void
ligma_display_shell_scrollbars_setup_vertical (LigmaDisplayShell *shell,
                                              gdouble           value)
{
  gint    bounds_y;
  gint    bounds_height;
  gint    bounding_box_y;
  gint    bounding_box_height;
  gint    y1;
  gint    y2;
  gdouble lower;
  gdouble upper;
  gdouble scale_y;

  g_return_if_fail (LIGMA_IS_DISPLAY_SHELL (shell));

  if (! shell->display || ! ligma_display_get_image (shell->display))
    return;

  ligma_display_shell_scale_get_image_bounds (shell,
                                             NULL, &bounds_y,
                                             NULL, &bounds_height);

  if (! ligma_display_shell_get_infinite_canvas (shell))
    {
      bounding_box_y      = bounds_y;
      bounding_box_height = bounds_height;
    }
  else
    {
      ligma_display_shell_scale_get_image_bounding_box (
        shell,
        NULL, &bounding_box_y,
        NULL, &bounding_box_height);
    }

  y1 = bounding_box_y;
  y2 = bounding_box_y + bounding_box_height;

  y1 = MIN (y1, bounds_y + bounds_height / 2 - shell->disp_height       / 2);
  y2 = MAX (y2, bounds_y + bounds_height / 2 + (shell->disp_height + 1) / 2);

  lower = MIN (value,                      y1);
  upper = MAX (value + shell->disp_height, y2);

  ligma_display_shell_get_rotated_scale (shell, NULL, &scale_y);

  g_object_set (shell->vsbdata,
                "lower",          lower,
                "upper",          upper,
                "step-increment", (gdouble) MAX (scale_y, MINIMUM_STEP_AMOUNT),
                NULL);
}

/**
 * ligma_display_shell_scrollbars_update_steppers:
 * @shell:
 * @min_offset_x:
 * @max_offset_x:
 * @min_offset_y:
 * @max_offset_y:
 *
 * Sets the scrollbars' stepper sensitivity which is set differently
 * from its adjustment limits because we support overscrolling.
 **/
void
ligma_display_shell_scrollbars_update_steppers (LigmaDisplayShell *shell,
                                               gint              min_offset_x,
                                               gint              max_offset_x,
                                               gint              min_offset_y,
                                               gint              max_offset_y)
{
  g_return_if_fail (LIGMA_IS_DISPLAY_SHELL (shell));

  gtk_range_set_lower_stepper_sensitivity (GTK_RANGE (shell->hsb),
                                           min_offset_x < shell->offset_x ?
                                           GTK_SENSITIVITY_ON :
                                           GTK_SENSITIVITY_OFF);

  gtk_range_set_upper_stepper_sensitivity (GTK_RANGE (shell->hsb),
                                           max_offset_x > shell->offset_x ?
                                           GTK_SENSITIVITY_ON :
                                           GTK_SENSITIVITY_OFF);

  gtk_range_set_lower_stepper_sensitivity (GTK_RANGE (shell->vsb),
                                           min_offset_y < shell->offset_y ?
                                           GTK_SENSITIVITY_ON :
                                           GTK_SENSITIVITY_OFF);

  gtk_range_set_upper_stepper_sensitivity (GTK_RANGE (shell->vsb),
                                           max_offset_y > shell->offset_y ?
                                           GTK_SENSITIVITY_ON :
                                           GTK_SENSITIVITY_OFF);
}
