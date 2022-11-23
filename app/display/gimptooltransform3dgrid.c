/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ligmatool3dtransformgrid.c
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

#include "libligmabase/ligmabase.h"
#include "libligmamath/ligmamath.h"

#include "display-types.h"

#include "widgets/ligmawidgets-utils.h"

#include "core/ligma-transform-3d-utils.h"
#include "core/ligma-utils.h"

#include "ligmadisplayshell.h"
#include "ligmadisplayshell-transform.h"
#include "ligmatooltransform3dgrid.h"

#include "ligma-intl.h"


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

struct _LigmaToolTransform3DGridPrivate
{
  LigmaTransform3DMode mode;
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

  LigmaTransformHandle handle;

  gdouble             orig_x;
  gdouble             orig_y;
  gdouble             orig_offset_x;
  gdouble             orig_offset_y;
  gdouble             orig_offset_z;
  LigmaMatrix3         orig_transform;

  gdouble             last_x;
  gdouble             last_y;

  Axis                constrained_axis;
};


/*  local function prototypes  */

static void       ligma_tool_transform_3d_grid_constructed            (GObject                 *object);
static void       ligma_tool_transform_3d_grid_set_property           (GObject                 *object,
                                                                      guint                    property_id,
                                                                      const GValue            *value,
                                                                      GParamSpec              *pspec);
static void       ligma_tool_transform_3d_grid_get_property           (GObject                 *object,
                                                                      guint                    property_id,
                                                                      GValue                  *value,
                                                                      GParamSpec              *pspec);

static gint       ligma_tool_transform_3d_grid_button_press           (LigmaToolWidget          *widget,
                                                                      const LigmaCoords        *coords,
                                                                      guint32                  time,
                                                                      GdkModifierType          state,
                                                                      LigmaButtonPressType      press_type);
static void       ligma_tool_transform_3d_grid_motion                 (LigmaToolWidget          *widget,
                                                                      const LigmaCoords        *coords,
                                                                      guint32                  time,
                                                                      GdkModifierType          state);
static void       ligma_tool_transform_3d_grid_hover                  (LigmaToolWidget          *widget,
                                                                      const LigmaCoords        *coords,
                                                                      GdkModifierType          state,
                                                                      gboolean                 proximity);
static void       ligma_tool_transform_3d_grid_hover_modifier         (LigmaToolWidget          *widget,
                                                                      GdkModifierType          key,
                                                                      gboolean                 press,
                                                                      GdkModifierType          state);
static gboolean   ligma_tool_transform_3d_grid_get_cursor              (LigmaToolWidget          *widget,
                                                                      const LigmaCoords        *coords,
                                                                      GdkModifierType          state,
                                                                      LigmaCursorType          *cursor,
                                                                      LigmaToolCursorType      *tool_cursor,
                                                                      LigmaCursorModifier      *modifier);

static void       ligma_tool_transform_3d_grid_update_mode            (LigmaToolTransform3DGrid *grid);
static void       ligma_tool_transform_3d_grid_reset_motion           (LigmaToolTransform3DGrid *grid);
static gboolean   ligma_tool_transform_3d_grid_constrain              (LigmaToolTransform3DGrid *grid,
                                                                      gdouble                  x,
                                                                      gdouble                  y,
                                                                      gdouble                  ox,
                                                                      gdouble                  oy,
                                                                      gdouble                 *tx,
                                                                      gdouble                 *ty);

static gboolean   ligma_tool_transform_3d_grid_motion_vanishing_point (LigmaToolTransform3DGrid *grid,
                                                                      gdouble                  x,
                                                                      gdouble                  y);
static gboolean   ligma_tool_transform_3d_grid_motion_move            (LigmaToolTransform3DGrid *grid,
                                                                      gdouble                  x,
                                                                      gdouble                  y);
static gboolean   ligma_tool_transform_3d_grid_motion_rotate          (LigmaToolTransform3DGrid *grid,
                                                                      gdouble                  x,
                                                                      gdouble                  y);


G_DEFINE_TYPE_WITH_PRIVATE (LigmaToolTransform3DGrid, ligma_tool_transform_3d_grid,
                            LIGMA_TYPE_TOOL_TRANSFORM_GRID)

#define parent_class ligma_tool_transform_3d_grid_parent_class


static void
ligma_tool_transform_3d_grid_class_init (LigmaToolTransform3DGridClass *klass)
{
  GObjectClass        *object_class = G_OBJECT_CLASS (klass);
  LigmaToolWidgetClass *widget_class = LIGMA_TOOL_WIDGET_CLASS (klass);

  object_class->constructed     = ligma_tool_transform_3d_grid_constructed;
  object_class->set_property    = ligma_tool_transform_3d_grid_set_property;
  object_class->get_property    = ligma_tool_transform_3d_grid_get_property;

  widget_class->button_press    = ligma_tool_transform_3d_grid_button_press;
  widget_class->motion          = ligma_tool_transform_3d_grid_motion;
  widget_class->hover           = ligma_tool_transform_3d_grid_hover;
  widget_class->hover_modifier  = ligma_tool_transform_3d_grid_hover_modifier;
  widget_class->get_cursor      = ligma_tool_transform_3d_grid_get_cursor;

  g_object_class_install_property (object_class, PROP_MODE,
                                   g_param_spec_enum ("mode",
                                                      NULL, NULL,
                                                      LIGMA_TYPE_TRANSFORM_3D_MODE,
                                                      LIGMA_TRANSFORM_3D_MODE_CAMERA,
                                                      LIGMA_PARAM_READWRITE |
                                                      G_PARAM_CONSTRUCT));

  g_object_class_install_property (object_class, PROP_UNIFIED,
                                   g_param_spec_boolean ("unified",
                                                         NULL, NULL,
                                                         FALSE,
                                                         LIGMA_PARAM_READWRITE |
                                                         G_PARAM_CONSTRUCT));

  g_object_class_install_property (object_class, PROP_CONSTRAIN_AXIS,
                                   g_param_spec_boolean ("constrain-axis",
                                                         NULL, NULL,
                                                         FALSE,
                                                         LIGMA_PARAM_READWRITE |
                                                         G_PARAM_CONSTRUCT));

  g_object_class_install_property (object_class, PROP_Z_AXIS,
                                   g_param_spec_boolean ("z-axis",
                                                         NULL, NULL,
                                                         FALSE,
                                                         LIGMA_PARAM_READWRITE |
                                                         G_PARAM_CONSTRUCT));

  g_object_class_install_property (object_class, PROP_LOCAL_FRAME,
                                   g_param_spec_boolean ("local-frame",
                                                         NULL, NULL,
                                                         FALSE,
                                                         LIGMA_PARAM_READWRITE |
                                                         G_PARAM_CONSTRUCT));

  g_object_class_install_property (object_class, PROP_CAMERA_X,
                                   g_param_spec_double ("camera-x",
                                                        NULL, NULL,
                                                        -G_MAXDOUBLE,
                                                        G_MAXDOUBLE,
                                                        0.0,
                                                        LIGMA_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT));

  g_object_class_install_property (object_class, PROP_CAMERA_Y,
                                   g_param_spec_double ("camera-y",
                                                        NULL, NULL,
                                                        -G_MAXDOUBLE,
                                                        G_MAXDOUBLE,
                                                        0.0,
                                                        LIGMA_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT));

  g_object_class_install_property (object_class, PROP_CAMERA_Z,
                                   g_param_spec_double ("camera-z",
                                                        NULL, NULL,
                                                        -(1.0 / 0.0),
                                                        1.0 / 0.0,
                                                        0.0,
                                                        LIGMA_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT));

  g_object_class_install_property (object_class, PROP_OFFSET_X,
                                   g_param_spec_double ("offset-x",
                                                        NULL, NULL,
                                                        -G_MAXDOUBLE,
                                                        G_MAXDOUBLE,
                                                        0.0,
                                                        LIGMA_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT));

  g_object_class_install_property (object_class, PROP_OFFSET_Y,
                                   g_param_spec_double ("offset-y",
                                                        NULL, NULL,
                                                        -G_MAXDOUBLE,
                                                        G_MAXDOUBLE,
                                                        0.0,
                                                        LIGMA_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT));

  g_object_class_install_property (object_class, PROP_OFFSET_Z,
                                   g_param_spec_double ("offset-z",
                                                        NULL, NULL,
                                                        -G_MAXDOUBLE,
                                                        G_MAXDOUBLE,
                                                        0.0,
                                                        LIGMA_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT));

  g_object_class_install_property (object_class, PROP_ROTATION_ORDER,
                                   g_param_spec_int ("rotation-order",
                                                     NULL, NULL,
                                                     0, 6, 0,
                                                     LIGMA_PARAM_READWRITE |
                                                     G_PARAM_CONSTRUCT));

  g_object_class_install_property (object_class, PROP_ANGLE_X,
                                   g_param_spec_double ("angle-x",
                                                        NULL, NULL,
                                                        -G_MAXDOUBLE,
                                                        G_MAXDOUBLE,
                                                        0.0,
                                                        LIGMA_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT));

  g_object_class_install_property (object_class, PROP_ANGLE_Y,
                                   g_param_spec_double ("angle-y",
                                                        NULL, NULL,
                                                        -G_MAXDOUBLE,
                                                        G_MAXDOUBLE,
                                                        0.0,
                                                        LIGMA_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT));

  g_object_class_install_property (object_class, PROP_ANGLE_Z,
                                   g_param_spec_double ("angle-z",
                                                        NULL, NULL,
                                                        -G_MAXDOUBLE,
                                                        G_MAXDOUBLE,
                                                        0.0,
                                                        LIGMA_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT));

  g_object_class_install_property (object_class, PROP_PIVOT_3D_X,
                                   g_param_spec_double ("pivot-3d-x",
                                                        NULL, NULL,
                                                        -G_MAXDOUBLE,
                                                        G_MAXDOUBLE,
                                                        0.0,
                                                        LIGMA_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT));

  g_object_class_install_property (object_class, PROP_PIVOT_3D_Y,
                                   g_param_spec_double ("pivot-3d-y",
                                                        NULL, NULL,
                                                        -G_MAXDOUBLE,
                                                        G_MAXDOUBLE,
                                                        0.0,
                                                        LIGMA_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT));

  g_object_class_install_property (object_class, PROP_PIVOT_3D_Z,
                                   g_param_spec_double ("pivot-3d-z",
                                                        NULL, NULL,
                                                        -G_MAXDOUBLE,
                                                        G_MAXDOUBLE,
                                                        0.0,
                                                        LIGMA_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT));
}

static void
ligma_tool_transform_3d_grid_init (LigmaToolTransform3DGrid *grid)
{
  grid->priv = ligma_tool_transform_3d_grid_get_instance_private (grid);
}

static void
ligma_tool_transform_3d_grid_constructed (GObject *object)
{
  G_OBJECT_CLASS (parent_class)->constructed (object);

  g_object_set (object,
                "clip-guides",         TRUE,
                "dynamic-handle-size", FALSE,
                NULL);
}

static void
ligma_tool_transform_3d_grid_set_property (GObject      *object,
                                          guint         property_id,
                                          const GValue *value,
                                          GParamSpec   *pspec)
{
  LigmaToolTransform3DGrid        *grid = LIGMA_TOOL_TRANSFORM_3D_GRID (object);
  LigmaToolTransform3DGridPrivate *priv = grid->priv;

  switch (property_id)
    {
    case PROP_MODE:
      priv->mode = g_value_get_enum (value);
      ligma_tool_transform_3d_grid_update_mode (grid);
      break;

    case PROP_UNIFIED:
      priv->unified = g_value_get_boolean (value);
      ligma_tool_transform_3d_grid_update_mode (grid);
      break;

    case PROP_CONSTRAIN_AXIS:
      priv->constrain_axis = g_value_get_boolean (value);
      ligma_tool_transform_3d_grid_reset_motion (grid);
      break;
    case PROP_Z_AXIS:
      priv->z_axis = g_value_get_boolean (value);
      ligma_tool_transform_3d_grid_reset_motion (grid);
      break;
    case PROP_LOCAL_FRAME:
      priv->local_frame = g_value_get_boolean (value);
      ligma_tool_transform_3d_grid_reset_motion (grid);
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
ligma_tool_transform_3d_grid_get_property (GObject    *object,
                                          guint       property_id,
                                          GValue     *value,
                                          GParamSpec *pspec)
{
  LigmaToolTransform3DGrid        *grid = LIGMA_TOOL_TRANSFORM_3D_GRID (object);
  LigmaToolTransform3DGridPrivate *priv = grid->priv;

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
ligma_tool_transform_3d_grid_button_press (LigmaToolWidget      *widget,
                                          const LigmaCoords    *coords,
                                          guint32              time,
                                          GdkModifierType      state,
                                          LigmaButtonPressType  press_type)
{
  LigmaToolTransform3DGrid        *grid = LIGMA_TOOL_TRANSFORM_3D_GRID (widget);
  LigmaToolTransform3DGridPrivate *priv = grid->priv;

  priv->handle = LIGMA_TOOL_WIDGET_CLASS (parent_class)->button_press (
    widget, coords, time, state, press_type);

  priv->orig_x           = coords->x;
  priv->orig_y           = coords->y;
  priv->orig_offset_x    = priv->offset_x;
  priv->orig_offset_y    = priv->offset_y;
  priv->orig_offset_z    = priv->offset_z;
  priv->last_x           = coords->x;
  priv->last_y           = coords->y;

  ligma_tool_transform_3d_grid_reset_motion (grid);

  return priv->handle;
}

void
ligma_tool_transform_3d_grid_motion (LigmaToolWidget   *widget,
                                    const LigmaCoords *coords,
                                    guint32           time,
                                    GdkModifierType   state)
{
  LigmaToolTransform3DGrid        *grid = LIGMA_TOOL_TRANSFORM_3D_GRID (widget);
  LigmaToolTransform3DGridPrivate *priv = grid->priv;
  LigmaMatrix3                     transform;
  gboolean                        update = TRUE;

  switch (priv->handle)
    {
    case LIGMA_TRANSFORM_HANDLE_PIVOT:
      update = ligma_tool_transform_3d_grid_motion_vanishing_point (
        grid, coords->x, coords->y);
      break;

    case LIGMA_TRANSFORM_HANDLE_CENTER:
      update = ligma_tool_transform_3d_grid_motion_move (
        grid, coords->x, coords->y);
      break;

    case LIGMA_TRANSFORM_HANDLE_ROTATION:
      update = ligma_tool_transform_3d_grid_motion_rotate (
        grid, coords->x, coords->y);
    break;

    default:
      g_return_if_reached ();
    }

  if (update)
    {
      ligma_transform_3d_matrix (&transform,

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
ligma_tool_transform_3d_grid_hover (LigmaToolWidget   *widget,
                                   const LigmaCoords *coords,
                                   GdkModifierType   state,
                                   gboolean          proximity)
{
  LIGMA_TOOL_WIDGET_CLASS (parent_class)->hover (widget,
                                                coords, state, proximity);

  if (proximity &&
      ligma_tool_transform_grid_get_handle (LIGMA_TOOL_TRANSFORM_GRID (widget)) ==
      LIGMA_TRANSFORM_HANDLE_PIVOT)
    {
      ligma_tool_widget_set_status (widget,
                                   _("Click-Drag to move the vanishing point"));
    }
}

static void
ligma_tool_transform_3d_grid_hover_modifier (LigmaToolWidget  *widget,
                                            GdkModifierType  key,
                                            gboolean         press,
                                            GdkModifierType  state)
{
  LigmaToolTransform3DGrid        *grid = LIGMA_TOOL_TRANSFORM_3D_GRID (widget);
  LigmaToolTransform3DGridPrivate *priv = grid->priv;

  LIGMA_TOOL_WIDGET_CLASS (parent_class)->hover_modifier (widget,
                                                         key, press, state);

  priv->local_frame = (state & ligma_get_extend_selection_mask ()) != 0;
}

static gboolean
ligma_tool_transform_3d_grid_get_cursor (LigmaToolWidget     *widget,
                                        const LigmaCoords   *coords,
                                        GdkModifierType     state,
                                        LigmaCursorType     *cursor,
                                        LigmaToolCursorType *tool_cursor,
                                        LigmaCursorModifier *modifier)
{
  if (! LIGMA_TOOL_WIDGET_CLASS (parent_class)->get_cursor (widget,
                                                           coords,
                                                           state,
                                                           cursor,
                                                           tool_cursor,
                                                           modifier))
    {
      return FALSE;
    }

  if (ligma_tool_transform_grid_get_handle (LIGMA_TOOL_TRANSFORM_GRID (widget)) ==
      LIGMA_TRANSFORM_HANDLE_PIVOT)
    {
      *tool_cursor = LIGMA_TOOL_CURSOR_TRANSFORM_3D_CAMERA;
    }

  return TRUE;
}

static void
ligma_tool_transform_3d_grid_update_mode (LigmaToolTransform3DGrid *grid)
{
  LigmaToolTransform3DGridPrivate *priv = grid->priv;

  if (priv->unified)
    {
      g_object_set (grid,
                    "inside-function",  LIGMA_TRANSFORM_FUNCTION_MOVE,
                    "outside-function", LIGMA_TRANSFORM_FUNCTION_ROTATE,
                    "use-pivot-handle", TRUE,
                    NULL);
    }
  else
    {
      switch (priv->mode)
        {
        case LIGMA_TRANSFORM_3D_MODE_CAMERA:
          g_object_set (grid,
                        "inside-function",  LIGMA_TRANSFORM_FUNCTION_NONE,
                        "outside-function", LIGMA_TRANSFORM_FUNCTION_NONE,
                        "use-pivot-handle", TRUE,
                        NULL);
          break;

        case LIGMA_TRANSFORM_3D_MODE_MOVE:
          g_object_set (grid,
                        "inside-function",  LIGMA_TRANSFORM_FUNCTION_MOVE,
                        "outside-function", LIGMA_TRANSFORM_FUNCTION_MOVE,
                        "use-pivot-handle", FALSE,
                        NULL);
          break;

        case LIGMA_TRANSFORM_3D_MODE_ROTATE:
          g_object_set (grid,
                        "inside-function",  LIGMA_TRANSFORM_FUNCTION_ROTATE,
                        "outside-function", LIGMA_TRANSFORM_FUNCTION_ROTATE,
                        "use-pivot-handle", FALSE,
                        NULL);
          break;
        }
    }
}

static void
ligma_tool_transform_3d_grid_reset_motion (LigmaToolTransform3DGrid *grid)
{
  LigmaToolTransform3DGridPrivate *priv = grid->priv;
  LigmaMatrix3                    *transform;

  priv->constrained_axis = AXIS_NONE;

  g_object_get (grid,
                "transform", &transform,
                NULL);

  priv->orig_transform = *transform;

  g_free (transform);
}

static gboolean
ligma_tool_transform_3d_grid_constrain (LigmaToolTransform3DGrid *grid,
                                       gdouble                  x,
                                       gdouble                  y,
                                       gdouble                  ox,
                                       gdouble                  oy,
                                       gdouble                 *tx,
                                       gdouble                 *ty)
{
  LigmaToolTransform3DGridPrivate *priv = grid->priv;

  if (! priv->constrain_axis)
    return TRUE;

  if (priv->constrained_axis == AXIS_NONE)
    {
      LigmaDisplayShell *shell;
      gdouble           x1, y1;
      gdouble           x2, y2;

      shell = ligma_tool_widget_get_shell (LIGMA_TOOL_WIDGET (grid));

      ligma_display_shell_transform_xy_f (shell,
                                         priv->last_x, priv->last_y,
                                         &x1,          &y1);
      ligma_display_shell_transform_xy_f (shell,
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
ligma_tool_transform_3d_grid_motion_vanishing_point (LigmaToolTransform3DGrid *grid,
                                                    gdouble                  x,
                                                    gdouble                  y)
{
  LigmaToolTransform3DGridPrivate *priv = grid->priv;
  LigmaCoords                      c    = {};
  gdouble                         pivot_x;
  gdouble                         pivot_y;

  if (! ligma_tool_transform_3d_grid_constrain (grid,
                                               x,            y,
                                               priv->last_x, priv->last_y,
                                               &x,           &y))
    {
      return FALSE;
    }

  c.x = x;
  c.y = y;

  LIGMA_TOOL_WIDGET_CLASS (parent_class)->motion (LIGMA_TOOL_WIDGET (grid),
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
ligma_tool_transform_3d_grid_motion_move (LigmaToolTransform3DGrid *grid,
                                         gdouble                  x,
                                         gdouble                  y)
{
  LigmaToolTransform3DGridPrivate *priv = grid->priv;
  LigmaMatrix4                     matrix;

  if (! priv->z_axis)
    {
      gdouble x1, y1, z1, w1;
      gdouble x2, y2, z2, w2;

      if (! priv->local_frame)
        {
          ligma_matrix4_identity (&matrix);
        }
      else
        {
          LigmaMatrix3 transform_inv = priv->orig_transform;;

          ligma_matrix3_invert (&transform_inv);

          ligma_transform_3d_matrix3_to_matrix4 (&transform_inv, &matrix, 2);
        }

      w1 = ligma_matrix4_transform_point (&matrix,
                                         priv->last_x, priv->last_y, 0.0,
                                         &x1,          &y1,          &z1);
      w2 = ligma_matrix4_transform_point (&matrix,
                                         x,            y,            0.0,
                                         &x2,          &y2,          &z2);

      if (w1 <= 0.0)
        return FALSE;

      if (! ligma_tool_transform_3d_grid_constrain (grid,
                                                   x,   y,
                                                   x1,  y1,
                                                   &x2, &y2))
        {
          return FALSE;
        }

      if (priv->local_frame)
        {
          ligma_matrix4_identity (&matrix);

          ligma_transform_3d_matrix4_rotate_euler (&matrix,
                                                  priv->rotation_order,
                                                  priv->angle_x,
                                                  priv->angle_y,
                                                  priv->angle_z,
                                                  0.0, 0.0, 0.0);

          ligma_matrix4_transform_point (&matrix,
                                        x1,  y1,  z1,
                                        &x1, &y1, &z1);
          ligma_matrix4_transform_point (&matrix,
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
      LigmaVector3 axis;
      gdouble     amount;

      if (! priv->local_frame)
        {
          axis.x = 0.0;
          axis.y = 0.0;
          axis.z = 1.0;
        }
      else
        {
          ligma_matrix4_identity (&matrix);

          ligma_transform_3d_matrix4_rotate_euler (&matrix,
                                                  priv->rotation_order,
                                                  priv->angle_x,
                                                  priv->angle_y,
                                                  priv->angle_z,
                                                  0.0, 0.0, 0.0);

          axis.x = matrix.coeff[0][2];
          axis.y = matrix.coeff[1][2];
          axis.z = matrix.coeff[2][2];

          if (axis.x < 0.0)
            ligma_vector3_neg (&axis);
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
ligma_tool_transform_3d_grid_motion_rotate (LigmaToolTransform3DGrid *grid,
                                           gdouble                  x,
                                           gdouble                  y)
{
  LigmaToolTransform3DGridPrivate *priv = grid->priv;
  LigmaDisplayShell               *shell;
  LigmaMatrix4                     matrix;
  LigmaMatrix2                     basis_inv;
  LigmaVector3                     omega;
  gdouble                         z_sign;
  gboolean                        local_frame;

  shell = ligma_tool_widget_get_shell (LIGMA_TOOL_WIDGET (grid));

  local_frame = priv->local_frame && (priv->constrain_axis || priv->z_axis);

  if (! local_frame)
    {
      ligma_matrix2_identity (&basis_inv);
      z_sign = 1.0;
    }
  else
    {
      {
        LigmaVector3 o, n, c;

        ligma_matrix4_identity (&matrix);

        ligma_transform_3d_matrix4_rotate_euler (&matrix,
                                                priv->rotation_order,
                                                priv->angle_x,
                                                priv->angle_y,
                                                priv->angle_z,
                                                priv->pivot_x,
                                                priv->pivot_y,
                                                priv->pivot_z);

        ligma_transform_3d_matrix4_translate (&matrix,
                                             priv->offset_x,
                                             priv->offset_y,
                                             priv->offset_z);

        ligma_matrix4_transform_point (&matrix,
                                      0.0,  0.0,  0.0,
                                      &o.x, &o.y, &o.z);
        ligma_matrix4_transform_point (&matrix,
                                      0.0,  0.0,  1.0,
                                      &n.x, &n.y, &n.z);

        c.x = priv->camera_x;
        c.y = priv->camera_y;
        c.z = priv->camera_z;

        ligma_vector3_sub (&n, &n, &o);
        ligma_vector3_sub (&c, &c, &o);

        z_sign = ligma_vector3_inner_product (&c, &n) <= 0.0 ? +1.0 : -1.0;
      }

      {
        LigmaVector2 o, u, v;

        ligma_matrix3_transform_point (&priv->orig_transform,
                                      priv->pivot_x, priv->pivot_y,
                                      &o.x, &o.y);
        ligma_matrix3_transform_point (&priv->orig_transform,
                                      priv->pivot_x + 1.0, priv->pivot_y,
                                      &u.x, &u.y);
        ligma_matrix3_transform_point (&priv->orig_transform,
                                      priv->pivot_x, priv->pivot_y + 1.0,
                                      &v.x, &v.y);

        ligma_vector2_sub (&u, &u, &o);
        ligma_vector2_sub (&v, &v, &o);

        ligma_vector2_normalize (&u);
        ligma_vector2_normalize (&v);

        basis_inv.coeff[0][0] = u.x;
        basis_inv.coeff[1][0] = u.y;
        basis_inv.coeff[0][1] = v.x;
        basis_inv.coeff[1][1] = v.y;

        ligma_matrix2_invert (&basis_inv);
      }
    }

  if (! priv->z_axis)
    {
      LigmaVector2 scale;
      gdouble     norm;

      ligma_matrix2_transform_point (&basis_inv,
                                    -(y - priv->last_y),
                                    x - priv->last_x,
                                    &omega.x, &omega.y);

      omega.z = 0.0;

      if (! ligma_tool_transform_3d_grid_constrain (grid,
                                                   x,        y,
                                                   0.0,      0.0,
                                                   &omega.x, &omega.y))
        {
          return FALSE;
        }

      norm = ligma_vector3_length (&omega);

      if (norm > 0.0)
        {
          scale.x = shell->scale_x * omega.y / norm;
          scale.y = shell->scale_y * omega.x / norm;

          ligma_vector3_mul (&omega, ligma_vector2_length (&scale));
          ligma_vector3_mul (&omega, 2.0 * G_PI / PIXELS_PER_REVOLUTION);
        }
    }
  else
    {
      LigmaVector2 o;
      LigmaVector2 v1 = {priv->last_x,  priv->last_y};
      LigmaVector2 v2 = {x,             y};

      g_warn_if_fail (priv->pivot_z == 0.0);

      ligma_matrix3_transform_point (&priv->orig_transform,
                                    priv->pivot_x, priv->pivot_y,
                                    &o.x,          &o.y);

      ligma_vector2_sub (&v1, &v1, &o);
      ligma_vector2_sub (&v2, &v2, &o);

      ligma_vector2_normalize (&v1);
      ligma_vector2_normalize (&v2);

      omega.x = 0.0;
      omega.y = 0.0;
      omega.z = atan2 (ligma_vector2_cross_product (&v1, &v2).y,
                       ligma_vector2_inner_product (&v1, &v2));

      omega.z *= z_sign;
    }

  ligma_matrix4_identity (&matrix);

  if (local_frame)
    ligma_transform_3d_matrix4_rotate (&matrix, &omega);

  ligma_transform_3d_matrix4_rotate_euler (&matrix,
                                          priv->rotation_order,
                                          priv->angle_x,
                                          priv->angle_y,
                                          priv->angle_z,
                                          0.0, 0.0, 0.0);

  if (! local_frame)
    ligma_transform_3d_matrix4_rotate (&matrix, &omega);

  ligma_transform_3d_matrix4_rotate_euler_decompose (&matrix,
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

LigmaToolWidget *
ligma_tool_transform_3d_grid_new (LigmaDisplayShell *shell,
                                 gdouble           x1,
                                 gdouble           y1,
                                 gdouble           x2,
                                 gdouble           y2,
                                 gdouble           camera_x,
                                 gdouble           camera_y,
                                 gdouble           camera_z)
{
  g_return_val_if_fail (LIGMA_IS_DISPLAY_SHELL (shell), NULL);

  return g_object_new (LIGMA_TYPE_TOOL_TRANSFORM_3D_GRID,
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
