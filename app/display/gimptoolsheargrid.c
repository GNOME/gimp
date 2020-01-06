/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimptoolsheargrid.c
 * Copyright (C) 2017 Michael Natterer <mitch@gimp.org>
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

#include "libgimpbase/gimpbase.h"
#include "libgimpmath/gimpmath.h"

#include "display-types.h"

#include "core/gimp-transform-utils.h"
#include "core/gimp-utils.h"

#include "gimpdisplayshell.h"
#include "gimptoolsheargrid.h"

#include "gimp-intl.h"


enum
{
  PROP_0,
  PROP_ORIENTATION,
  PROP_SHEAR_X,
  PROP_SHEAR_Y
};


struct _GimpToolShearGridPrivate
{
  GimpOrientationType orientation;
  gdouble             shear_x;
  gdouble             shear_y;

  gdouble             last_x;
  gdouble             last_y;
};


/*  local function prototypes  */

static void     gimp_tool_shear_grid_set_property (GObject             *object,
                                                   guint                property_id,
                                                   const GValue        *value,
                                                   GParamSpec          *pspec);
static void     gimp_tool_shear_grid_get_property (GObject             *object,
                                                   guint                property_id,
                                                   GValue              *value,
                                                   GParamSpec          *pspec);

static gint     gimp_tool_shear_grid_button_press (GimpToolWidget      *widget,
                                                   const GimpCoords    *coords,
                                                   guint32              time,
                                                   GdkModifierType      state,
                                                   GimpButtonPressType  press_type);
static void     gimp_tool_shear_grid_motion        (GimpToolWidget      *widget,
                                                    const GimpCoords    *coords,
                                                    guint32              time,
                                                    GdkModifierType      state);


G_DEFINE_TYPE_WITH_PRIVATE (GimpToolShearGrid, gimp_tool_shear_grid,
                            GIMP_TYPE_TOOL_TRANSFORM_GRID)

#define parent_class gimp_tool_shear_grid_parent_class


static void
gimp_tool_shear_grid_class_init (GimpToolShearGridClass *klass)
{
  GObjectClass        *object_class = G_OBJECT_CLASS (klass);
  GimpToolWidgetClass *widget_class = GIMP_TOOL_WIDGET_CLASS (klass);

  object_class->set_property    = gimp_tool_shear_grid_set_property;
  object_class->get_property    = gimp_tool_shear_grid_get_property;

  widget_class->button_press    = gimp_tool_shear_grid_button_press;
  widget_class->motion          = gimp_tool_shear_grid_motion;

  g_object_class_install_property (object_class, PROP_ORIENTATION,
                                   g_param_spec_enum ("orientation",
                                                      NULL, NULL,
                                                      GIMP_TYPE_ORIENTATION_TYPE,
                                                      GIMP_ORIENTATION_UNKNOWN,
                                                      GIMP_PARAM_READWRITE |
                                                      G_PARAM_CONSTRUCT));

  g_object_class_install_property (object_class, PROP_SHEAR_X,
                                   g_param_spec_double ("shear-x",
                                                        NULL, NULL,
                                                        -GIMP_MAX_IMAGE_SIZE,
                                                        GIMP_MAX_IMAGE_SIZE,
                                                        0.0,
                                                        GIMP_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT));

  g_object_class_install_property (object_class, PROP_SHEAR_Y,
                                   g_param_spec_double ("shear-y",
                                                        NULL, NULL,
                                                        -GIMP_MAX_IMAGE_SIZE,
                                                        GIMP_MAX_IMAGE_SIZE,
                                                        0.0,
                                                        GIMP_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT));
}

static void
gimp_tool_shear_grid_init (GimpToolShearGrid *grid)
{
  grid->private = gimp_tool_shear_grid_get_instance_private (grid);

  g_object_set (grid,
                "inside-function",         GIMP_TRANSFORM_FUNCTION_SHEAR,
                "outside-function",        GIMP_TRANSFORM_FUNCTION_SHEAR,
                "use-corner-handles",      FALSE,
                "use-perspective-handles", FALSE,
                "use-side-handles",        FALSE,
                "use-shear-handles",       FALSE,
                "use-center-handle",       FALSE,
                "use-pivot-handle",        FALSE,
                NULL);
}

static void
gimp_tool_shear_grid_set_property (GObject      *object,
                                   guint         property_id,
                                   const GValue *value,
                                   GParamSpec   *pspec)
{
  GimpToolShearGrid        *grid    = GIMP_TOOL_SHEAR_GRID (object);
  GimpToolShearGridPrivate *private = grid->private;

  switch (property_id)
    {
    case PROP_ORIENTATION:
      private->orientation = g_value_get_enum (value);
      break;
    case PROP_SHEAR_X:
      private->shear_x = g_value_get_double (value);
      break;
    case PROP_SHEAR_Y:
      private->shear_y = g_value_get_double (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_tool_shear_grid_get_property (GObject    *object,
                                   guint       property_id,
                                   GValue     *value,
                                   GParamSpec *pspec)
{
  GimpToolShearGrid        *grid    = GIMP_TOOL_SHEAR_GRID (object);
  GimpToolShearGridPrivate *private = grid->private;

  switch (property_id)
    {
    case PROP_ORIENTATION:
      g_value_set_enum (value, private->orientation);
      break;
    case PROP_SHEAR_X:
      g_value_set_double (value, private->shear_x);
      break;
    case PROP_SHEAR_Y:
      g_value_set_double (value, private->shear_y);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static gint
gimp_tool_shear_grid_button_press (GimpToolWidget      *widget,
                                   const GimpCoords    *coords,
                                   guint32              time,
                                   GdkModifierType      state,
                                   GimpButtonPressType  press_type)
{
  GimpToolShearGrid        *grid    = GIMP_TOOL_SHEAR_GRID (widget);
  GimpToolShearGridPrivate *private = grid->private;

  private->last_x = coords->x;
  private->last_y = coords->y;

  return 1;
}

void
gimp_tool_shear_grid_motion (GimpToolWidget   *widget,
                             const GimpCoords *coords,
                             guint32           time,
                             GdkModifierType   state)
{
  GimpToolShearGrid        *grid    = GIMP_TOOL_SHEAR_GRID (widget);
  GimpToolShearGridPrivate *private = grid->private;
  gdouble                   diffx   = coords->x - private->last_x;
  gdouble                   diffy   = coords->y - private->last_y;
  gdouble                   amount  = 0.0;
  GimpMatrix3               transform;
  GimpMatrix3              *t;
  gdouble                   x1, y1;
  gdouble                   x2, y2;
  gdouble                   tx1, ty1;
  gdouble                   tx2, ty2;
  gdouble                   tx3, ty3;
  gdouble                   tx4, ty4;
  gdouble                   current_x;
  gdouble                   current_y;

  g_object_get (widget,
                "transform", &t,
                "x1",        &x1,
                "y1",        &y1,
                "x2",        &x2,
                "y2",        &y2,
                NULL);

  gimp_matrix3_transform_point (t, x1, y1, &tx1, &ty1);
  gimp_matrix3_transform_point (t, x2, y1, &tx2, &ty2);
  gimp_matrix3_transform_point (t, x1, y2, &tx3, &ty3);
  gimp_matrix3_transform_point (t, x2, y2, &tx4, &ty4);

  g_free (t);

  current_x = coords->x;
  current_y = coords->y;

  diffx = current_x - private->last_x;
  diffy = current_y - private->last_y;

  /*  If we haven't yet decided on which way to control shearing
   *  decide using the maximum differential
   */
  if (private->orientation == GIMP_ORIENTATION_UNKNOWN)
    {
#define MIN_MOVE 5

      if (ABS (diffx) > MIN_MOVE || ABS (diffy) > MIN_MOVE)
        {
          if (ABS (diffx) > ABS (diffy))
            {
              private->orientation = GIMP_ORIENTATION_HORIZONTAL;
              private->shear_x     = 0.0;
            }
          else
            {
              private->orientation = GIMP_ORIENTATION_VERTICAL;
              private->shear_y     = 0.0;
            }
        }
      /*  set the current coords to the last ones  */
      else
        {
          current_x = private->last_x;
          current_y = private->last_y;
        }
    }

  /*  if the direction is known, keep track of the magnitude  */
  if (private->orientation == GIMP_ORIENTATION_HORIZONTAL)
    {
      if (current_y > (ty1 + ty3) / 2)
        private->shear_x += diffx;
      else
        private->shear_x -= diffx;

      amount = private->shear_x;
    }
  else if (private->orientation == GIMP_ORIENTATION_VERTICAL)
    {
      if (current_x > (tx1 + tx2) / 2)
        private->shear_y += diffy;
      else
        private->shear_y -= diffy;

      amount = private->shear_y;
    }

  gimp_matrix3_identity (&transform);
  gimp_transform_matrix_shear (&transform,
                               x1, y1, x2 - x1, y2 - y1,
                               private->orientation, amount);

  g_object_set (widget,
                "transform",   &transform,
                "orientation", private->orientation,
                "shear-x",     private->shear_x,
                "shear_y",     private->shear_y,
                NULL);

  private->last_x = current_x;
  private->last_y = current_y;
}


/*  public functions  */

GimpToolWidget *
gimp_tool_shear_grid_new (GimpDisplayShell    *shell,
                          gdouble              x1,
                          gdouble              y1,
                          gdouble              x2,
                          gdouble              y2,
                          GimpOrientationType  orientation,
                          gdouble              shear_x,
                          gdouble              shear_y)
{
  GimpMatrix3 transform;
  gdouble     amount;

  g_return_val_if_fail (GIMP_IS_DISPLAY_SHELL (shell), NULL);

  if (orientation == GIMP_ORIENTATION_HORIZONTAL)
    amount = shear_x;
  else
    amount = shear_y;

  gimp_matrix3_identity (&transform);
  gimp_transform_matrix_shear (&transform,
                               x1, y1, x2 - x1, y2 - y1,
                               orientation, amount);

  return g_object_new (GIMP_TYPE_TOOL_SHEAR_GRID,
                       "shell",       shell,
                       "transform",   &transform,
                       "x1",          x1,
                       "y1",          y1,
                       "x2",          x2,
                       "y2",          y2,
                       "orientation", orientation,
                       "shear-x",     shear_x,
                       "shear-y",     shear_y,
                       NULL);
}
