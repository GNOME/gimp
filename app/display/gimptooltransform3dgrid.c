/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimptool3dtransformgrid.c
 * Copyright (C) 2019 Ell
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

#include "widgets/gimpwidgets-utils.h"

#include "core/gimp-transform-3d-utils.h"
#include "core/gimp-utils.h"

#include "gimpdisplayshell.h"
#include "gimpdisplayshell-transform.h"
#include "gimptooltransform3dgrid.h"

#include "gimp-intl.h"


#define CONSTRAINT_MIN_DIST   8.0
#define PIXELS_PER_REVOLUTION 1000


enum
{
  PROP_0,
  PROP_MODE,
  PROP_UNIFIED,
  PROP_CONSTRAIN_AXIS,
  PROP_Z_AXIS,
  PROP_LOCAL_FRAME,
  PROP_CAMERA_X,
  PROP_CAMERA_Y,
  PROP_CAMERA_Z,
  PROP_OFFSET_X,
  PROP_OFFSET_Y,
  PROP_OFFSET_Z,
  PROP_ROTATION_ORDER,
  PROP_ANGLE_X,
  PROP_ANGLE_Y,
  PROP_ANGLE_Z,
  PROP_PIVOT_3D_X,
  PROP_PIVOT_3D_Y,
  PROP_PIVOT_3D_Z
};

typedef enum
{
  AXIS_NONE,
  AXIS_X,
  AXIS_Y
} Axis;

struct _GimpToolTransform3DGridPrivate
{
  GimpTransform3DMode mode;
  gboolean            unified;

  gboolean            constrain_axis;
  gboolean            z_axis;
  gboolean            local_frame;

  gdouble             camera_x;
  gdouble             camera_y;
  gdouble             camera_z;

  gdouble             offset_x;
  gdouble             offset_y;
  gdouble             offset_z;

  gint                rotation_order;
  gdouble             angle_x;
  gdouble             angle_y;
  gdouble             angle_z;

  gdouble             pivot_x;
  gdouble             pivot_y;
  gdouble             pivot_z;

  GimpTransformHandle handle;

  gdouble             orig_x;
  gdouble             orig_y;
  gdouble             orig_offset_x;
  gdouble             orig_offset_y;
  gdouble             orig_offset_z;
  GimpMatrix3         orig_transform;

  gdouble             last_x;
  gdouble             last_y;

  Axis                constrained_axis;
};


/*  local function prototypes  */

static void       gimp_tool_transform_3d_grid_constructed            (GObject                 *object);
static void       gimp_tool_transform_3d_grid_set_property           (GObject                 *object,
                                                                      guint                    property_id,
                                                                      const GValue            *value,
                                                                      GParamSpec              *pspec);
static void       gimp_tool_transform_3d_grid_get_property           (GObject                 *object,
                                                                      guint                    property_id,
                                                                      GValue                  *value,
                                                                      GParamSpec              *pspec);

static gint       gimp_tool_transform_3d_grid_button_press           (GimpToolWidget          *widget,
                                                                      const GimpCoords        *coords,
                                                                      guint32                  time,
                                                                      GdkModifierType          state,
                                                                      GimpButtonPressType      press_type);
static void       gimp_tool_transform_3d_grid_motion                 (GimpToolWidget          *widget,
                                                                      const GimpCoords        *coords,
                                                                      guint32                  time,
                                                                      GdkModifierType          state);
static void       gimp_tool_transform_3d_grid_hover                  (GimpToolWidget          *widget,
                                                                      const GimpCoords        *coords,
                                                                      GdkModifierType          state,
                                                                      gboolean                 proximity);
static void       gimp_tool_transform_3d_grid_hover_modifier         (GimpToolWidget          *widget,
                                                                      GdkModifierType          key,
                                                                      gboolean                 press,
                                                                      GdkModifierType          state);
static gboolean   gimp_tool_transform_3d_grid_get_cursor              (GimpToolWidget          *widget,
                                                                      const GimpCoords        *coords,
                                                                      GdkModifierType          state,
                                                                      GimpCursorType          *cursor,
                                                                      GimpToolCursorType      *tool_cursor,
                                                                      GimpCursorModifier      *modifier);

static void       gimp_tool_transform_3d_grid_update_mode            (GimpToolTransform3DGrid *grid);
static void       gimp_tool_transform_3d_grid_reset_motion           (GimpToolTransform3DGrid *grid);
static gboolean   gimp_tool_transform_3d_grid_constrain              (GimpToolTransform3DGrid *grid,
                                                                      gdouble                  x,
                                                                      gdouble                  y,
                                                                      gdouble                  ox,
                                                                      gdouble                  oy,
                                                                      gdouble                 *tx,
                                                                      gdouble                 *ty);

static gboolean   gimp_tool_transform_3d_grid_motion_vanishing_point (GimpToolTransform3DGrid *grid,
                                                                      gdouble                  x,
                                                                      gdouble                  y);
static gboolean   gimp_tool_transform_3d_grid_motion_move            (GimpToolTransform3DGrid *grid,
                                                                      gdouble                  x,
                                                                      gdouble                  y);
static gboolean   gimp_tool_transform_3d_grid_motion_rotate          (GimpToolTransform3DGrid *grid,
                                                                      gdouble                  x,
                                                                      gdouble                  y);


G_DEFINE_TYPE_WITH_PRIVATE (GimpToolTransform3DGrid, gimp_tool_transform_3d_grid,
                            GIMP_TYPE_TOOL_TRANSFORM_GRID)

#define parent_class gimp_tool_transform_3d_grid_parent_class


static void
gimp_tool_transform_3d_grid_class_init (GimpToolTransform3DGridClass *klass)
{
  GObjectClass        *object_class = G_OBJECT_CLASS (klass);
  GimpToolWidgetClass *widget_class = GIMP_TOOL_WIDGET_CLASS (klass);

  object_class->constructed     = gimp_tool_transform_3d_grid_constructed;
  object_class->set_property    = gimp_tool_transform_3d_grid_set_property;
  object_class->get_property    = gimp_tool_transform_3d_grid_get_property;

  widget_class->button_press    = gimp_tool_transform_3d_grid_button_press;
  widget_class->motion          = gimp_tool_transform_3d_grid_motion;
  widget_class->hover           = gimp_tool_transform_3d_grid_hover;
  widget_class->hover_modifier  = gimp_tool_transform_3d_grid_hover_modifier;
  widget_class->get_cursor      = gimp_tool_transform_3d_grid_get_cursor;

  g_object_class_install_property (object_class, PROP_MODE,
                                   g_param_spec_enum ("mode",
                                                      NULL, NULL,
                                                      GIMP_TYPE_TRANSFORM_3D_MODE,
                                                      GIMP_TRANSFORM_3D_MODE_CAMERA,
                                                      GIMP_PARAM_READWRITE |
                                                      G_PARAM_CONSTRUCT));

  g_object_class_install_property (object_class, PROP_UNIFIED,
                                   g_param_spec_boolean ("unified",
                                                         NULL, NULL,
                                                         FALSE,
                                                         GIMP_PARAM_READWRITE |
                                                         G_PARAM_CONSTRUCT));

  g_object_class_install_property (object_class, PROP_CONSTRAIN_AXIS,
                                   g_param_spec_boolean ("constrain-axis",
                                                         NULL, NULL,
                                                         FALSE,
                                                         GIMP_PARAM_READWRITE |
                                                         G_PARAM_CONSTRUCT));

  g_object_class_install_property (object_class, PROP_Z_AXIS,
                                   g_param_spec_boolean ("z-axis",
                                                         NULL, NULL,
                                                         FALSE,
                                                         GIMP_PARAM_READWRITE |
                                                         G_PARAM_CONSTRUCT));

  g_object_class_install_property (object_class, PROP_LOCAL_FRAME,
                                   g_param_spec_boolean ("local-frame",
                                                         NULL, NULL,
                                                         FALSE,
                                                         GIMP_PARAM_READWRITE |
                                                         G_PARAM_CONSTRUCT));

  g_object_class_install_property (object_class, PROP_CAMERA_X,
                                   g_param_spec_double ("camera-x",
                                                        NULL, NULL,
                                                        -G_MAXDOUBLE,
                                                        G_MAXDOUBLE,
                                                        0.0,
                                                        GIMP_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT));

  g_object_class_install_property (object_class, PROP_CAMERA_Y,
                                   g_param_spec_double ("camera-y",
                                                        NULL, NULL,
                                                        -G_MAXDOUBLE,
                                                        G_MAXDOUBLE,
                                                        0.0,
                                                        GIMP_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT));

  g_object_class_install_property (object_class, PROP_CAMERA_Z,
                                   g_param_spec_double ("camera-z",
                                                        NULL, NULL,
                                                        -(1.0 / 0.0),
                                                        1.0 / 0.0,
                                                        0.0,
                                                        GIMP_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT));

  g_object_class_install_property (object_class, PROP_OFFSET_X,
                                   g_param_spec_double ("offset-x",
                                                        NULL, NULL,
                                                        -G_MAXDOUBLE,
                                                        G_MAXDOUBLE,
                                                        0.0,
                                                        GIMP_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT));

  g_object_class_install_property (object_class, PROP_OFFSET_Y,
                                   g_param_spec_double ("offset-y",
                                                        NULL, NULL,
                                                        -G_MAXDOUBLE,
                                                        G_MAXDOUBLE,
                                                        0.0,
                                                        GIMP_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT));

  g_object_class_install_property (object_class, PROP_OFFSET_Z,
                                   g_param_spec_double ("offset-z",
                                                        NULL, NULL,
                                                        -G_MAXDOUBLE,
                                                        G_MAXDOUBLE,
                                                        0.0,
                                                        GIMP_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT));

  g_object_class_install_property (object_class, PROP_ROTATION_ORDER,
                                   g_param_spec_int ("rotation-order",
                                                     NULL, NULL,
                                                     0, 6, 0,
                                                     GIMP_PARAM_READWRITE |
                                                     G_PARAM_CONSTRUCT));

  g_object_class_install_property (object_class, PROP_ANGLE_X,
                                   g_param_spec_double ("angle-x",
                                                        NULL, NULL,
                                                        -G_MAXDOUBLE,
                                                        G_MAXDOUBLE,
                                                        0.0,
                                                        GIMP_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT));

  g_object_class_install_property (object_class, PROP_ANGLE_Y,
                                   g_param_spec_double ("angle-y",
                                                        NULL, NULL,
                                                        -G_MAXDOUBLE,
                                                        G_MAXDOUBLE,
                                                        0.0,
                                                        GIMP_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT));

  g_object_class_install_property (object_class, PROP_ANGLE_Z,
                                   g_param_spec_double ("angle-z",
                                                        NULL, NULL,
                                                        -G_MAXDOUBLE,
                                                        G_MAXDOUBLE,
                                                        0.0,
                                                        GIMP_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT));

  g_object_class_install_property (object_class, PROP_PIVOT_3D_X,
                                   g_param_spec_double ("pivot-3d-x",
                                                        NULL, NULL,
                                                        -G_MAXDOUBLE,
                                                        G_MAXDOUBLE,
                                                        0.0,
                                                        GIMP_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT));

  g_object_class_install_property (object_class, PROP_PIVOT_3D_Y,
                                   g_param_spec_double ("pivot-3d-y",
                                                        NULL, NULL,
                                                        -G_MAXDOUBLE,
                                                        G_MAXDOUBLE,
                                                        0.0,
                                                        GIMP_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT));

  g_object_class_install_property (object_class, PROP_PIVOT_3D_Z,
                                   g_param_spec_double ("pivot-3d-z",
                                                        NULL, NULL,
                                                        -G_MAXDOUBLE,
                                                        G_MAXDOUBLE,
                                                        0.0,
                                                        GIMP_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT));
}

static void
gimp_tool_transform_3d_grid_init (GimpToolTransform3DGrid *grid)
{
  grid->priv = gimp_tool_transform_3d_grid_get_instance_private (grid);
}

static void
gimp_tool_transform_3d_grid_constructed (GObject *object)
{
  G_OBJECT_CLASS (parent_class)->constructed (object);

  g_object_set (object,
                "clip-guides",         TRUE,
                "dynamic-handle-size", FALSE,
                NULL);
}

static void
gimp_tool_transform_3d_grid_set_property (GObject      *object,
                                          guint         property_id,
                                          const GValue *value,
                                          GParamSpec   *pspec)
{
  GimpToolTransform3DGrid        *grid = GIMP_TOOL_TRANSFORM_3D_GRID (object);
  GimpToolTransform3DGridPrivate *priv = grid->priv;

  switch (property_id)
    {
    case PROP_MODE:
      priv->mode = g_value_get_enum (value);
      gimp_tool_transform_3d_grid_update_mode (grid);
      break;

    case PROP_UNIFIED:
      priv->unified = g_value_get_boolean (value);
      gimp_tool_transform_3d_grid_update_mode (grid);
      break;

    case PROP_CONSTRAIN_AXIS:
      priv->constrain_axis = g_value_get_boolean (value);
      gimp_tool_transform_3d_grid_reset_motion (grid);
      break;
    case PROP_Z_AXIS:
      priv->z_axis = g_value_get_boolean (value);
      gimp_tool_transform_3d_grid_reset_motion (grid);
      break;
    case PROP_LOCAL_FRAME:
      priv->local_frame = g_value_get_boolean (value);
      gimp_tool_transform_3d_grid_reset_motion (grid);
      break;

    case PROP_CAMERA_X:
      priv->camera_x = g_value_get_double (value);
      g_object_set (grid,
                    "pivot-x", priv->camera_x,
                    NULL);
      break;
    case PROP_CAMERA_Y:
      priv->camera_y = g_value_get_double (value);
      g_object_set (grid,
                    "pivot-y", priv->camera_y,
                    NULL);
      break;
    case PROP_CAMERA_Z:
      priv->camera_z = g_value_get_double (value);
      break;

    case PROP_OFFSET_X:
      priv->offset_x = g_value_get_double (value);
      break;
    case PROP_OFFSET_Y:
      priv->offset_y = g_value_get_double (value);
      break;
    case PROP_OFFSET_Z:
      priv->offset_z = g_value_get_double (value);
      break;

    case PROP_ROTATION_ORDER:
      priv->rotation_order = g_value_get_int (value);
      break;
    case PROP_ANGLE_X:
      priv->angle_x = g_value_get_double (value);
      break;
    case PROP_ANGLE_Y:
      priv->angle_y = g_value_get_double (value);
      break;
    case PROP_ANGLE_Z:
      priv->angle_z = g_value_get_double (value);
      break;

    case PROP_PIVOT_3D_X:
      priv->pivot_x = g_value_get_double (value);
      break;
    case PROP_PIVOT_3D_Y:
      priv->pivot_y = g_value_get_double (value);
      break;
    case PROP_PIVOT_3D_Z:
      priv->pivot_z = g_value_get_double (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_tool_transform_3d_grid_get_property (GObject    *object,
                                          guint       property_id,
                                          GValue     *value,
                                          GParamSpec *pspec)
{
  GimpToolTransform3DGrid        *grid = GIMP_TOOL_TRANSFORM_3D_GRID (object);
  GimpToolTransform3DGridPrivate *priv = grid->priv;

  switch (property_id)
    {
    case PROP_MODE:
      g_value_set_enum (value, priv->mode);
      break;
    case PROP_UNIFIED:
      g_value_set_boolean (value, priv->unified);
      break;

    case PROP_CONSTRAIN_AXIS:
      g_value_set_boolean (value, priv->constrain_axis);
      break;
    case PROP_Z_AXIS:
      g_value_set_boolean (value, priv->z_axis);
      break;
    case PROP_LOCAL_FRAME:
      g_value_set_boolean (value, priv->local_frame);
      break;

    case PROP_CAMERA_X:
      g_value_set_double (value, priv->camera_x);
      break;
    case PROP_CAMERA_Y:
      g_value_set_double (value, priv->camera_y);
      break;
    case PROP_CAMERA_Z:
      g_value_set_double (value, priv->camera_z);
      break;

    case PROP_OFFSET_X:
      g_value_set_double (value, priv->offset_x);
      break;
    case PROP_OFFSET_Y:
      g_value_set_double (value, priv->offset_y);
      break;
    case PROP_OFFSET_Z:
      g_value_set_double (value, priv->offset_z);
      break;

    case PROP_ROTATION_ORDER:
      g_value_set_int (value, priv->rotation_order);
      break;
    case PROP_ANGLE_X:
      g_value_set_double (value, priv->angle_x);
      break;
    case PROP_ANGLE_Y:
      g_value_set_double (value, priv->angle_y);
      break;
    case PROP_ANGLE_Z:
      g_value_set_double (value, priv->angle_z);
      break;

    case PROP_PIVOT_3D_X:
      g_value_set_double (value, priv->pivot_x);
      break;
    case PROP_PIVOT_3D_Y:
      g_value_set_double (value, priv->pivot_y);
      break;
    case PROP_PIVOT_3D_Z:
      g_value_set_double (value, priv->pivot_z);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static gint
gimp_tool_transform_3d_grid_button_press (GimpToolWidget      *widget,
                                          const GimpCoords    *coords,
                                          guint32              time,
                                          GdkModifierType      state,
                                          GimpButtonPressType  press_type)
{
  GimpToolTransform3DGrid        *grid = GIMP_TOOL_TRANSFORM_3D_GRID (widget);
  GimpToolTransform3DGridPrivate *priv = grid->priv;

  priv->handle = GIMP_TOOL_WIDGET_CLASS (parent_class)->button_press (
    widget, coords, time, state, press_type);

  priv->orig_x           = coords->x;
  priv->orig_y           = coords->y;
  priv->orig_offset_x    = priv->offset_x;
  priv->orig_offset_y    = priv->offset_y;
  priv->orig_offset_z    = priv->offset_z;
  priv->last_x           = coords->x;
  priv->last_y           = coords->y;

  gimp_tool_transform_3d_grid_reset_motion (grid);

  return priv->handle;
}

void
gimp_tool_transform_3d_grid_motion (GimpToolWidget   *widget,
                                    const GimpCoords *coords,
                                    guint32           time,
                                    GdkModifierType   state)
{
  GimpToolTransform3DGrid        *grid = GIMP_TOOL_TRANSFORM_3D_GRID (widget);
  GimpToolTransform3DGridPrivate *priv = grid->priv;
  GimpMatrix3                     transform;
  gboolean                        update = TRUE;

  switch (priv->handle)
    {
    case GIMP_TRANSFORM_HANDLE_PIVOT:
      update = gimp_tool_transform_3d_grid_motion_vanishing_point (
        grid, coords->x, coords->y);
      break;

    case GIMP_TRANSFORM_HANDLE_CENTER:
      update = gimp_tool_transform_3d_grid_motion_move (
        grid, coords->x, coords->y);
      break;

    case GIMP_TRANSFORM_HANDLE_ROTATION:
      update = gimp_tool_transform_3d_grid_motion_rotate (
        grid, coords->x, coords->y);
    break;

    default:
      g_return_if_reached ();
    }

  if (update)
    {
      gimp_transform_3d_matrix (&transform,

                                priv->camera_x,
                                priv->camera_y,
                                priv->camera_z,

                                priv->offset_x,
                                priv->offset_y,
                                priv->offset_z,

                                priv->rotation_order,
                                priv->angle_x,
                                priv->angle_y,
                                priv->angle_z,

                                priv->pivot_x,
                                priv->pivot_y,
                                priv->pivot_z);

      g_object_set (widget,
                    "transform", &transform,
                    NULL);
    }
}

static void
gimp_tool_transform_3d_grid_hover (GimpToolWidget   *widget,
                                   const GimpCoords *coords,
                                   GdkModifierType   state,
                                   gboolean          proximity)
{
  GIMP_TOOL_WIDGET_CLASS (parent_class)->hover (widget,
                                                coords, state, proximity);

  if (proximity &&
      gimp_tool_transform_grid_get_handle (GIMP_TOOL_TRANSFORM_GRID (widget)) ==
      GIMP_TRANSFORM_HANDLE_PIVOT)
    {
      gimp_tool_widget_set_status (widget,
                                   _("Click-Drag to move the vanishing point"));
    }
}

static void
gimp_tool_transform_3d_grid_hover_modifier (GimpToolWidget  *widget,
                                            GdkModifierType  key,
                                            gboolean         press,
                                            GdkModifierType  state)
{
  GimpToolTransform3DGrid        *grid = GIMP_TOOL_TRANSFORM_3D_GRID (widget);
  GimpToolTransform3DGridPrivate *priv = grid->priv;

  GIMP_TOOL_WIDGET_CLASS (parent_class)->hover_modifier (widget,
                                                         key, press, state);

  priv->local_frame = (state & gimp_get_extend_selection_mask ()) != 0;
}

static gboolean
gimp_tool_transform_3d_grid_get_cursor (GimpToolWidget     *widget,
                                        const GimpCoords   *coords,
                                        GdkModifierType     state,
                                        GimpCursorType     *cursor,
                                        GimpToolCursorType *tool_cursor,
                                        GimpCursorModifier *modifier)
{
  if (! GIMP_TOOL_WIDGET_CLASS (parent_class)->get_cursor (widget,
                                                           coords,
                                                           state,
                                                           cursor,
                                                           tool_cursor,
                                                           modifier))
    {
      return FALSE;
    }

  if (gimp_tool_transform_grid_get_handle (GIMP_TOOL_TRANSFORM_GRID (widget)) ==
      GIMP_TRANSFORM_HANDLE_PIVOT)
    {
      *tool_cursor = GIMP_TOOL_CURSOR_TRANSFORM_3D_CAMERA;
    }

  return TRUE;
}

static void
gimp_tool_transform_3d_grid_update_mode (GimpToolTransform3DGrid *grid)
{
  GimpToolTransform3DGridPrivate *priv = grid->priv;

  if (priv->unified)
    {
      g_object_set (grid,
                    "inside-function",  GIMP_TRANSFORM_FUNCTION_MOVE,
                    "outside-function", GIMP_TRANSFORM_FUNCTION_ROTATE,
                    "use-pivot-handle", TRUE,
                    NULL);
    }
  else
    {
      switch (priv->mode)
        {
        case GIMP_TRANSFORM_3D_MODE_CAMERA:
          g_object_set (grid,
                        "inside-function",  GIMP_TRANSFORM_FUNCTION_NONE,
                        "outside-function", GIMP_TRANSFORM_FUNCTION_NONE,
                        "use-pivot-handle", TRUE,
                        NULL);
          break;

        case GIMP_TRANSFORM_3D_MODE_MOVE:
          g_object_set (grid,
                        "inside-function",  GIMP_TRANSFORM_FUNCTION_MOVE,
                        "outside-function", GIMP_TRANSFORM_FUNCTION_MOVE,
                        "use-pivot-handle", FALSE,
                        NULL);
          break;

        case GIMP_TRANSFORM_3D_MODE_ROTATE:
          g_object_set (grid,
                        "inside-function",  GIMP_TRANSFORM_FUNCTION_ROTATE,
                        "outside-function", GIMP_TRANSFORM_FUNCTION_ROTATE,
                        "use-pivot-handle", FALSE,
                        NULL);
          break;
        }
    }
}

static void
gimp_tool_transform_3d_grid_reset_motion (GimpToolTransform3DGrid *grid)
{
  GimpToolTransform3DGridPrivate *priv = grid->priv;
  GimpMatrix3                    *transform;

  priv->constrained_axis = AXIS_NONE;

  g_object_get (grid,
                "transform", &transform,
                NULL);

  priv->orig_transform = *transform;

  g_free (transform);
}

static gboolean
gimp_tool_transform_3d_grid_constrain (GimpToolTransform3DGrid *grid,
                                       gdouble                  x,
                                       gdouble                  y,
                                       gdouble                  ox,
                                       gdouble                  oy,
                                       gdouble                 *tx,
                                       gdouble                 *ty)
{
  GimpToolTransform3DGridPrivate *priv = grid->priv;

  if (! priv->constrain_axis)
    return TRUE;

  if (priv->constrained_axis == AXIS_NONE)
    {
      GimpDisplayShell *shell;
      gdouble           x1, y1;
      gdouble           x2, y2;

      shell = gimp_tool_widget_get_shell (GIMP_TOOL_WIDGET (grid));

      gimp_display_shell_transform_xy_f (shell,
                                         priv->last_x, priv->last_y,
                                         &x1,          &y1);
      gimp_display_shell_transform_xy_f (shell,
                                         x,            y,
                                         &x2,          &y2);

      if (hypot (x2 - x1, y2 - y1) < CONSTRAINT_MIN_DIST)
        return FALSE;

      if (fabs (*tx - ox) >= fabs (*ty - oy))
        priv->constrained_axis = AXIS_X;
      else
        priv->constrained_axis = AXIS_Y;
    }

  if (priv->constrained_axis == AXIS_X)
    *ty = oy;
  else
    *tx = ox;

  return TRUE;
}

static gboolean
gimp_tool_transform_3d_grid_motion_vanishing_point (GimpToolTransform3DGrid *grid,
                                                    gdouble                  x,
                                                    gdouble                  y)
{
  GimpToolTransform3DGridPrivate *priv = grid->priv;
  GimpCoords                      c    = {};
  gdouble                         pivot_x;
  gdouble                         pivot_y;

  if (! gimp_tool_transform_3d_grid_constrain (grid,
                                               x,            y,
                                               priv->last_x, priv->last_y,
                                               &x,           &y))
    {
      return FALSE;
    }

  c.x = x;
  c.y = y;

  GIMP_TOOL_WIDGET_CLASS (parent_class)->motion (GIMP_TOOL_WIDGET (grid),
                                                 &c, 0, 0);

  g_object_get (grid,
                "pivot-x", &pivot_x,
                "pivot-y", &pivot_y,
                NULL);

  g_object_set (grid,
                "camera-x", pivot_x,
                "camera-y", pivot_y,
                NULL);

  priv->last_x = c.x;
  priv->last_y = c.y;

  return TRUE;
}

static gboolean
gimp_tool_transform_3d_grid_motion_move (GimpToolTransform3DGrid *grid,
                                         gdouble                  x,
                                         gdouble                  y)
{
  GimpToolTransform3DGridPrivate *priv = grid->priv;
  GimpMatrix4                     matrix;

  if (! priv->z_axis)
    {
      gdouble x1, y1, z1, w1;
      gdouble x2, y2, z2, w2;

      if (! priv->local_frame)
        {
          gimp_matrix4_identity (&matrix);
        }
      else
        {
          GimpMatrix3 transform_inv = priv->orig_transform;;

          gimp_matrix3_invert (&transform_inv);

          gimp_transform_3d_matrix3_to_matrix4 (&transform_inv, &matrix, 2);
        }

      w1 = gimp_matrix4_transform_point (&matrix,
                                         priv->last_x, priv->last_y, 0.0,
                                         &x1,          &y1,          &z1);
      w2 = gimp_matrix4_transform_point (&matrix,
                                         x,            y,            0.0,
                                         &x2,          &y2,          &z2);

      if (w1 <= 0.0)
        return FALSE;

      if (! gimp_tool_transform_3d_grid_constrain (grid,
                                                   x,   y,
                                                   x1,  y1,
                                                   &x2, &y2))
        {
          return FALSE;
        }

      if (priv->local_frame)
        {
          gimp_matrix4_identity (&matrix);

          gimp_transform_3d_matrix4_rotate_euler (&matrix,
                                                  priv->rotation_order,
                                                  priv->angle_x,
                                                  priv->angle_y,
                                                  priv->angle_z,
                                                  0.0, 0.0, 0.0);

          gimp_matrix4_transform_point (&matrix,
                                        x1,  y1,  z1,
                                        &x1, &y1, &z1);
          gimp_matrix4_transform_point (&matrix,
                                        x2,  y2,  z2,
                                        &x2, &y2, &z2);
        }

      if (w2 > 0.0)
        {
          g_object_set (grid,
                        "offset-x", priv->offset_x + (x2 - x1),
                        "offset-y", priv->offset_y + (y2 - y1),
                        "offset-z", priv->offset_z + (z2 - z1),
                        NULL);

          priv->last_x = x;
          priv->last_y = y;
        }
      else
        {
          g_object_set (grid,
                        "offset-x", priv->orig_offset_x,
                        "offset-y", priv->orig_offset_y,
                        "offset-z", priv->orig_offset_z,
                        NULL);

          priv->last_x = priv->orig_x;
          priv->last_y = priv->orig_y;
        }
    }
  else
    {
      GimpVector3 axis;
      gdouble     amount;

      if (! priv->local_frame)
        {
          axis.x = 0.0;
          axis.y = 0.0;
          axis.z = 1.0;
        }
      else
        {
          gimp_matrix4_identity (&matrix);

          gimp_transform_3d_matrix4_rotate_euler (&matrix,
                                                  priv->rotation_order,
                                                  priv->angle_x,
                                                  priv->angle_y,
                                                  priv->angle_z,
                                                  0.0, 0.0, 0.0);

          axis.x = matrix.coeff[0][2];
          axis.y = matrix.coeff[1][2];
          axis.z = matrix.coeff[2][2];

          if (axis.x < 0.0)
            gimp_vector3_neg (&axis);
        }

      amount = x - priv->last_x;

      g_object_set (grid,
                    "offset-x", priv->offset_x + axis.x * amount,
                    "offset-y", priv->offset_y + axis.y * amount,
                    "offset-z", priv->offset_z + axis.z * amount,
                    NULL);

      priv->last_x = x;
      priv->last_y = y;
    }

  return TRUE;
}

static gboolean
gimp_tool_transform_3d_grid_motion_rotate (GimpToolTransform3DGrid *grid,
                                           gdouble                  x,
                                           gdouble                  y)
{
  GimpToolTransform3DGridPrivate *priv = grid->priv;
  GimpDisplayShell               *shell;
  GimpMatrix4                     matrix;
  GimpMatrix2                     basis_inv;
  GimpVector3                     omega;
  gdouble                         z_sign;
  gboolean                        local_frame;

  shell = gimp_tool_widget_get_shell (GIMP_TOOL_WIDGET (grid));

  local_frame = priv->local_frame && (priv->constrain_axis || priv->z_axis);

  if (! local_frame)
    {
      gimp_matrix2_identity (&basis_inv);
      z_sign = 1.0;
    }
  else
    {
      {
        GimpVector3 o, n, c;

        gimp_matrix4_identity (&matrix);

        gimp_transform_3d_matrix4_rotate_euler (&matrix,
                                                priv->rotation_order,
                                                priv->angle_x,
                                                priv->angle_y,
                                                priv->angle_z,
                                                priv->pivot_x,
                                                priv->pivot_y,
                                                priv->pivot_z);

        gimp_transform_3d_matrix4_translate (&matrix,
                                             priv->offset_x,
                                             priv->offset_y,
                                             priv->offset_z);

        gimp_matrix4_transform_point (&matrix,
                                      0.0,  0.0,  0.0,
                                      &o.x, &o.y, &o.z);
        gimp_matrix4_transform_point (&matrix,
                                      0.0,  0.0,  1.0,
                                      &n.x, &n.y, &n.z);

        c.x = priv->camera_x;
        c.y = priv->camera_y;
        c.z = priv->camera_z;

        gimp_vector3_sub (&n, &n, &o);
        gimp_vector3_sub (&c, &c, &o);

        z_sign = gimp_vector3_inner_product (&c, &n) <= 0.0 ? +1.0 : -1.0;
      }

      {
        GimpVector2 o, u, v;

        gimp_matrix3_transform_point (&priv->orig_transform,
                                      priv->pivot_x, priv->pivot_y,
                                      &o.x, &o.y);
        gimp_matrix3_transform_point (&priv->orig_transform,
                                      priv->pivot_x + 1.0, priv->pivot_y,
                                      &u.x, &u.y);
        gimp_matrix3_transform_point (&priv->orig_transform,
                                      priv->pivot_x, priv->pivot_y + 1.0,
                                      &v.x, &v.y);

        gimp_vector2_sub (&u, &u, &o);
        gimp_vector2_sub (&v, &v, &o);

        gimp_vector2_normalize (&u);
        gimp_vector2_normalize (&v);

        basis_inv.coeff[0][0] = u.x;
        basis_inv.coeff[1][0] = u.y;
        basis_inv.coeff[0][1] = v.x;
        basis_inv.coeff[1][1] = v.y;

        gimp_matrix2_invert (&basis_inv);
      }
    }

  if (! priv->z_axis)
    {
      GimpVector2 scale;
      gdouble     norm;

      gimp_matrix2_transform_point (&basis_inv,
                                    -(y - priv->last_y),
                                    x - priv->last_x,
                                    &omega.x, &omega.y);

      omega.z = 0.0;

      if (! gimp_tool_transform_3d_grid_constrain (grid,
                                                   x,        y,
                                                   0.0,      0.0,
                                                   &omega.x, &omega.y))
        {
          return FALSE;
        }

      norm = gimp_vector3_length (&omega);

      if (norm > 0.0)
        {
          scale.x = shell->scale_x * omega.y / norm;
          scale.y = shell->scale_y * omega.x / norm;

          gimp_vector3_mul (&omega, gimp_vector2_length (&scale));
          gimp_vector3_mul (&omega, 2.0 * G_PI / PIXELS_PER_REVOLUTION);
        }
    }
  else
    {
      GimpVector2 o;
      GimpVector2 v1 = {priv->last_x,  priv->last_y};
      GimpVector2 v2 = {x,             y};

      g_warn_if_fail (priv->pivot_z == 0.0);

      gimp_matrix3_transform_point (&priv->orig_transform,
                                    priv->pivot_x, priv->pivot_y,
                                    &o.x,          &o.y);

      gimp_vector2_sub (&v1, &v1, &o);
      gimp_vector2_sub (&v2, &v2, &o);

      gimp_vector2_normalize (&v1);
      gimp_vector2_normalize (&v2);

      omega.x = 0.0;
      omega.y = 0.0;
      omega.z = atan2 (gimp_vector2_cross_product (&v1, &v2).y,
                       gimp_vector2_inner_product (&v1, &v2));

      omega.z *= z_sign;
    }

  gimp_matrix4_identity (&matrix);

  if (local_frame)
    gimp_transform_3d_matrix4_rotate (&matrix, &omega);

  gimp_transform_3d_matrix4_rotate_euler (&matrix,
                                          priv->rotation_order,
                                          priv->angle_x,
                                          priv->angle_y,
                                          priv->angle_z,
                                          0.0, 0.0, 0.0);

  if (! local_frame)
    gimp_transform_3d_matrix4_rotate (&matrix, &omega);

  gimp_transform_3d_matrix4_rotate_euler_decompose (&matrix,
                                                    priv->rotation_order,
                                                    &priv->angle_x,
                                                    &priv->angle_y,
                                                    &priv->angle_z);

  priv->last_x = x;
  priv->last_y = y;

  g_object_set (grid,
                "angle-x", priv->angle_x,
                "angle-y", priv->angle_y,
                "angle-z", priv->angle_z,
                NULL);

  return TRUE;
}


/*  public functions  */

GimpToolWidget *
gimp_tool_transform_3d_grid_new (GimpDisplayShell *shell,
                                 gdouble           x1,
                                 gdouble           y1,
                                 gdouble           x2,
                                 gdouble           y2,
                                 gdouble           camera_x,
                                 gdouble           camera_y,
                                 gdouble           camera_z)
{
  g_return_val_if_fail (GIMP_IS_DISPLAY_SHELL (shell), NULL);

  return g_object_new (GIMP_TYPE_TOOL_TRANSFORM_3D_GRID,
                       "shell",      shell,
                       "x1",         x1,
                       "y1",         y1,
                       "x2",         x2,
                       "y2",         y2,
                       "camera-x",   camera_x,
                       "camera-y",   camera_y,
                       "camera-z",   camera_z,
                       "pivot-3d-x", (x1 + x2) / 2.0,
                       "pivot-3d-y", (y1 + y2) / 2.0,
                       "pivot-3d-z", 0.0,
                       NULL);
}
