/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimptoolhandlegrid.c
 * Copyright (C) 2017 Michael Natterer <mitch@gimp.org>
 *
 * Based on GimpHandleTransformTool
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

#include "libgimpbase/gimpbase.h"
#include "libgimpmath/gimpmath.h"

#include "display-types.h"

#include "core/gimp-transform-utils.h"
#include "core/gimp-utils.h"

#include "widgets/gimpwidgets-utils.h"

#include "gimpcanvashandle.h"
#include "gimpdisplayshell.h"
#include "gimptoolhandlegrid.h"

#include "gimp-intl.h"


enum
{
  PROP_0,
  PROP_HANDLE_MODE,
  PROP_N_HANDLES,
  PROP_ORIG_X1,
  PROP_ORIG_Y1,
  PROP_ORIG_X2,
  PROP_ORIG_Y2,
  PROP_ORIG_X3,
  PROP_ORIG_Y3,
  PROP_ORIG_X4,
  PROP_ORIG_Y4,
  PROP_TRANS_X1,
  PROP_TRANS_Y1,
  PROP_TRANS_X2,
  PROP_TRANS_Y2,
  PROP_TRANS_X3,
  PROP_TRANS_Y3,
  PROP_TRANS_X4,
  PROP_TRANS_Y4
};


struct _GimpToolHandleGridPrivate
{
  GimpTransformHandleMode  handle_mode; /* enum to be renamed */

  gint     n_handles;
  gdouble  orig_x[4];
  gdouble  orig_y[4];
  gdouble  trans_x[4];
  gdouble  trans_y[4];

  gint     handle;
  gdouble  last_x;
  gdouble  last_y;

  gdouble  mouse_x;
  gdouble  mouse_y;

  GimpCanvasItem *handles[5];
};


/*  local function prototypes  */

static void   gimp_tool_handle_grid_constructed    (GObject               *object);
static void   gimp_tool_handle_grid_set_property   (GObject               *object,
                                                    guint                  property_id,
                                                    const GValue          *value,
                                                    GParamSpec            *pspec);
static void   gimp_tool_handle_grid_get_property   (GObject               *object,
                                                    guint                  property_id,
                                                    GValue                *value,
                                                    GParamSpec            *pspec);

static void   gimp_tool_handle_grid_changed        (GimpToolWidget        *widget);
static gint   gimp_tool_handle_grid_button_press   (GimpToolWidget        *widget,
                                                    const GimpCoords      *coords,
                                                    guint32                time,
                                                    GdkModifierType        state,
                                                    GimpButtonPressType    press_type);
static void   gimp_tool_handle_grid_button_release (GimpToolWidget        *widget,
                                                    const GimpCoords      *coords,
                                                    guint32                time,
                                                    GdkModifierType        state,
                                                    GimpButtonReleaseType  release_type);
static void   gimp_tool_handle_grid_motion         (GimpToolWidget        *widget,
                                                    const GimpCoords      *coords,
                                                    guint32                time,
                                                    GdkModifierType        state);
static void     gimp_tool_handle_grid_hover        (GimpToolWidget        *widget,
                                                    const GimpCoords      *coords,
                                                    GdkModifierType        state,
                                                    gboolean               proximity);
static gboolean gimp_tool_handle_grid_get_cursor   (GimpToolWidget        *widget,
                                                    const GimpCoords      *coords,
                                                    GdkModifierType        state,
                                                    GimpCursorType        *cursor,
                                                    GimpToolCursorType    *tool_cursor,
                                                    GimpCursorModifier    *cursor_modifier);

static void     gimp_tool_handle_grid_update_hilight (GimpToolHandleGrid  *grid);
static void     gimp_tool_handle_grid_update_matrix  (GimpToolHandleGrid  *grid);

static gboolean is_handle_position_valid           (GimpToolHandleGrid    *grid,
                                                    gint                   handle);
static void     handle_micro_move                  (GimpToolHandleGrid    *grid,
                                                    gint                   handle);

static inline gdouble calc_angle               (gdouble ax,
                                                gdouble ay,
                                                gdouble bx,
                                                gdouble by);
static inline gdouble calc_len                 (gdouble a,
                                                gdouble b);
static inline gdouble calc_lineintersect_ratio (gdouble p1x,
                                                gdouble p1y,
                                                gdouble p2x,
                                                gdouble p2y,
                                                gdouble q1x,
                                                gdouble q1y,
                                                gdouble q2x,
                                                gdouble q2y);


G_DEFINE_TYPE (GimpToolHandleGrid, gimp_tool_handle_grid,
               GIMP_TYPE_TOOL_TRANSFORM_GRID)

#define parent_class gimp_tool_handle_grid_parent_class


static void
gimp_tool_handle_grid_class_init (GimpToolHandleGridClass *klass)
{
  GObjectClass        *object_class = G_OBJECT_CLASS (klass);
  GimpToolWidgetClass *widget_class = GIMP_TOOL_WIDGET_CLASS (klass);

  object_class->constructed     = gimp_tool_handle_grid_constructed;
  object_class->set_property    = gimp_tool_handle_grid_set_property;
  object_class->get_property    = gimp_tool_handle_grid_get_property;

  widget_class->changed         = gimp_tool_handle_grid_changed;
  widget_class->button_press    = gimp_tool_handle_grid_button_press;
  widget_class->button_release  = gimp_tool_handle_grid_button_release;
  widget_class->motion          = gimp_tool_handle_grid_motion;
  widget_class->hover           = gimp_tool_handle_grid_hover;
  widget_class->get_cursor      = gimp_tool_handle_grid_get_cursor;

  g_object_class_install_property (object_class, PROP_HANDLE_MODE,
                                   g_param_spec_enum ("handle-mode",
                                                      NULL, NULL,
                                                      GIMP_TYPE_TRANSFORM_HANDLE_MODE,
                                                      GIMP_HANDLE_MODE_ADD_TRANSFORM,
                                                      GIMP_PARAM_READWRITE |
                                                      G_PARAM_CONSTRUCT));

  g_object_class_install_property (object_class, PROP_N_HANDLES,
                                   g_param_spec_int ("n-handles",
                                                     NULL, NULL,
                                                     0, 4, 0,
                                                     GIMP_PARAM_READWRITE |
                                                     G_PARAM_CONSTRUCT));

  g_object_class_install_property (object_class, PROP_ORIG_X1,
                                   g_param_spec_double ("orig-x1",
                                                        NULL, NULL,
                                                        -GIMP_MAX_IMAGE_SIZE,
                                                        GIMP_MAX_IMAGE_SIZE,
                                                        0.0,
                                                        GIMP_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT));

  g_object_class_install_property (object_class, PROP_ORIG_Y1,
                                   g_param_spec_double ("orig-y1",
                                                        NULL, NULL,
                                                        -GIMP_MAX_IMAGE_SIZE,
                                                        GIMP_MAX_IMAGE_SIZE,
                                                        0.0,
                                                        GIMP_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT));

  g_object_class_install_property (object_class, PROP_ORIG_X2,
                                   g_param_spec_double ("orig-x2",
                                                        NULL, NULL,
                                                        -GIMP_MAX_IMAGE_SIZE,
                                                        GIMP_MAX_IMAGE_SIZE,
                                                        0.0,
                                                        GIMP_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT));

  g_object_class_install_property (object_class, PROP_ORIG_Y2,
                                   g_param_spec_double ("orig-y2",
                                                        NULL, NULL,
                                                        -GIMP_MAX_IMAGE_SIZE,
                                                        GIMP_MAX_IMAGE_SIZE,
                                                        0.0,
                                                        GIMP_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT));

  g_object_class_install_property (object_class, PROP_ORIG_X3,
                                   g_param_spec_double ("orig-x3",
                                                        NULL, NULL,
                                                        -GIMP_MAX_IMAGE_SIZE,
                                                        GIMP_MAX_IMAGE_SIZE,
                                                        0.0,
                                                        GIMP_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT));

  g_object_class_install_property (object_class, PROP_ORIG_Y3,
                                   g_param_spec_double ("orig-y3",
                                                        NULL, NULL,
                                                        -GIMP_MAX_IMAGE_SIZE,
                                                        GIMP_MAX_IMAGE_SIZE,
                                                        0.0,
                                                        GIMP_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT));

  g_object_class_install_property (object_class, PROP_ORIG_X4,
                                   g_param_spec_double ("orig-x4",
                                                        NULL, NULL,
                                                        -GIMP_MAX_IMAGE_SIZE,
                                                        GIMP_MAX_IMAGE_SIZE,
                                                        0.0,
                                                        GIMP_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT));

  g_object_class_install_property (object_class, PROP_ORIG_Y4,
                                   g_param_spec_double ("orig-y4",
                                                        NULL, NULL,
                                                        -GIMP_MAX_IMAGE_SIZE,
                                                        GIMP_MAX_IMAGE_SIZE,
                                                        0.0,
                                                        GIMP_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT));

  g_object_class_install_property (object_class, PROP_TRANS_X1,
                                   g_param_spec_double ("trans-x1",
                                                        NULL, NULL,
                                                        -GIMP_MAX_IMAGE_SIZE,
                                                        GIMP_MAX_IMAGE_SIZE,
                                                        0.0,
                                                        GIMP_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT));

  g_object_class_install_property (object_class, PROP_TRANS_Y1,
                                   g_param_spec_double ("trans-y1",
                                                        NULL, NULL,
                                                        -GIMP_MAX_IMAGE_SIZE,
                                                        GIMP_MAX_IMAGE_SIZE,
                                                        0.0,
                                                        GIMP_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT));

  g_object_class_install_property (object_class, PROP_TRANS_X2,
                                   g_param_spec_double ("trans-x2",
                                                        NULL, NULL,
                                                        -GIMP_MAX_IMAGE_SIZE,
                                                        GIMP_MAX_IMAGE_SIZE,
                                                        0.0,
                                                        GIMP_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT));

  g_object_class_install_property (object_class, PROP_TRANS_Y2,
                                   g_param_spec_double ("trans-y2",
                                                        NULL, NULL,
                                                        -GIMP_MAX_IMAGE_SIZE,
                                                        GIMP_MAX_IMAGE_SIZE,
                                                        0.0,
                                                        GIMP_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT));

  g_object_class_install_property (object_class, PROP_TRANS_X3,
                                   g_param_spec_double ("trans-x3",
                                                        NULL, NULL,
                                                        -GIMP_MAX_IMAGE_SIZE,
                                                        GIMP_MAX_IMAGE_SIZE,
                                                        0.0,
                                                        GIMP_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT));

  g_object_class_install_property (object_class, PROP_TRANS_Y3,
                                   g_param_spec_double ("trans-y3",
                                                        NULL, NULL,
                                                        -GIMP_MAX_IMAGE_SIZE,
                                                        GIMP_MAX_IMAGE_SIZE,
                                                        0.0,
                                                        GIMP_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT));

  g_object_class_install_property (object_class, PROP_TRANS_X4,
                                   g_param_spec_double ("trans-x4",
                                                        NULL, NULL,
                                                        -GIMP_MAX_IMAGE_SIZE,
                                                        GIMP_MAX_IMAGE_SIZE,
                                                        0.0,
                                                        GIMP_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT));

  g_object_class_install_property (object_class, PROP_TRANS_Y4,
                                   g_param_spec_double ("trans-y4",
                                                        NULL, NULL,
                                                        -GIMP_MAX_IMAGE_SIZE,
                                                        GIMP_MAX_IMAGE_SIZE,
                                                        0.0,
                                                        GIMP_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT));

  g_type_class_add_private (klass, sizeof (GimpToolHandleGridPrivate));
}

static void
gimp_tool_handle_grid_init (GimpToolHandleGrid *grid)
{
  grid->private = G_TYPE_INSTANCE_GET_PRIVATE (grid,
                                               GIMP_TYPE_TOOL_HANDLE_GRID,
                                               GimpToolHandleGridPrivate);
}

static void
gimp_tool_handle_grid_constructed (GObject *object)
{
  GimpToolHandleGrid        *grid    = GIMP_TOOL_HANDLE_GRID (object);
  GimpToolWidget            *widget  = GIMP_TOOL_WIDGET (object);
  GimpToolHandleGridPrivate *private = grid->private;
  gint                       i;

  G_OBJECT_CLASS (parent_class)->constructed (object);

  for (i = 0; i < 4; i++)
    {
      private->handles[i + 1] =
        gimp_tool_widget_add_handle (widget,
                                     GIMP_HANDLE_CIRCLE,
                                     0, 0,
                                     GIMP_CANVAS_HANDLE_SIZE_CIRCLE,
                                     GIMP_CANVAS_HANDLE_SIZE_CIRCLE,
                                     GIMP_HANDLE_ANCHOR_CENTER);
    }

  gimp_tool_handle_grid_changed (widget);
}

static void
gimp_tool_handle_grid_set_property (GObject      *object,
                                    guint         property_id,
                                    const GValue *value,
                                    GParamSpec   *pspec)
{
  GimpToolHandleGrid        *grid    = GIMP_TOOL_HANDLE_GRID (object);
  GimpToolHandleGridPrivate *private = grid->private;

  switch (property_id)
    {
    case PROP_HANDLE_MODE:
      private->handle_mode = g_value_get_enum (value);
      break;

    case PROP_N_HANDLES:
      private->n_handles = g_value_get_int (value);
      break;

    case PROP_ORIG_X1:
      private->orig_x[0] = g_value_get_double (value);
      break;
    case PROP_ORIG_Y1:
      private->orig_y[0] = g_value_get_double (value);
      break;
    case PROP_ORIG_X2:
      private->orig_x[1] = g_value_get_double (value);
      break;
    case PROP_ORIG_Y2:
      private->orig_y[1] = g_value_get_double (value);
      break;
    case PROP_ORIG_X3:
      private->orig_x[2] = g_value_get_double (value);
      break;
    case PROP_ORIG_Y3:
      private->orig_y[2] = g_value_get_double (value);
      break;
    case PROP_ORIG_X4:
      private->orig_x[3] = g_value_get_double (value);
      break;
    case PROP_ORIG_Y4:
      private->orig_y[3] = g_value_get_double (value);
      break;

    case PROP_TRANS_X1:
      private->trans_x[0] = g_value_get_double (value);
      break;
    case PROP_TRANS_Y1:
      private->trans_y[0] = g_value_get_double (value);
      break;
    case PROP_TRANS_X2:
      private->trans_x[1] = g_value_get_double (value);
      break;
    case PROP_TRANS_Y2:
      private->trans_y[1] = g_value_get_double (value);
      break;
    case PROP_TRANS_X3:
      private->trans_x[2] = g_value_get_double (value);
      break;
    case PROP_TRANS_Y3:
      private->trans_y[2] = g_value_get_double (value);
      break;
    case PROP_TRANS_X4:
      private->trans_x[3] = g_value_get_double (value);
      break;
    case PROP_TRANS_Y4:
      private->trans_y[3] = g_value_get_double (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_tool_handle_grid_get_property (GObject    *object,
                                    guint       property_id,
                                    GValue     *value,
                                    GParamSpec *pspec)
{
  GimpToolHandleGrid        *grid    = GIMP_TOOL_HANDLE_GRID (object);
  GimpToolHandleGridPrivate *private = grid->private;

  switch (property_id)
    {
    case PROP_N_HANDLES:
      g_value_set_int (value, private->n_handles);
      break;

    case PROP_HANDLE_MODE:
      g_value_set_enum (value, private->handle_mode);
      break;

    case PROP_ORIG_X1:
      g_value_set_double (value, private->orig_x[0]);
      break;
    case PROP_ORIG_Y1:
      g_value_set_double (value, private->orig_y[0]);
      break;
    case PROP_ORIG_X2:
      g_value_set_double (value, private->orig_x[1]);
      break;
    case PROP_ORIG_Y2:
      g_value_set_double (value, private->orig_y[1]);
      break;
    case PROP_ORIG_X3:
      g_value_set_double (value, private->orig_x[2]);
      break;
    case PROP_ORIG_Y3:
      g_value_set_double (value, private->orig_y[2]);
      break;
    case PROP_ORIG_X4:
      g_value_set_double (value, private->orig_x[3]);
      break;
    case PROP_ORIG_Y4:
      g_value_set_double (value, private->orig_y[3]);
      break;

    case PROP_TRANS_X1:
      g_value_set_double (value, private->trans_x[0]);
      break;
    case PROP_TRANS_Y1:
      g_value_set_double (value, private->trans_y[0]);
      break;
    case PROP_TRANS_X2:
      g_value_set_double (value, private->trans_x[1]);
      break;
    case PROP_TRANS_Y2:
      g_value_set_double (value, private->trans_y[1]);
      break;
    case PROP_TRANS_X3:
      g_value_set_double (value, private->trans_x[2]);
      break;
    case PROP_TRANS_Y3:
      g_value_set_double (value, private->trans_y[2]);
      break;
    case PROP_TRANS_X4:
      g_value_set_double (value, private->trans_x[3]);
      break;
    case PROP_TRANS_Y4:
      g_value_set_double (value, private->trans_y[3]);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_tool_handle_grid_changed (GimpToolWidget *widget)
{
  GimpToolHandleGrid        *grid    = GIMP_TOOL_HANDLE_GRID (widget);
  GimpToolHandleGridPrivate *private = grid->private;
  gint                       i;

  GIMP_TOOL_WIDGET_CLASS (parent_class)->changed (widget);

  for (i = 0; i < 4; i++)
    {
      gimp_canvas_handle_set_position (private->handles[i + 1],
                                       private->trans_x[i],
                                       private->trans_y[i]);
      gimp_canvas_item_set_visible (private->handles[i + 1],
                                    i < private->n_handles);
    }

  gimp_tool_handle_grid_update_hilight (grid);
}

static gint
gimp_tool_handle_grid_button_press (GimpToolWidget      *widget,
                                    const GimpCoords    *coords,
                                    guint32              time,
                                    GdkModifierType      state,
                                    GimpButtonPressType  press_type)
{
  GimpToolHandleGrid        *grid          = GIMP_TOOL_HANDLE_GRID (widget);
  GimpToolHandleGridPrivate *private       = grid->private;
  gint                       n_handles     = private->n_handles;
  gint                       active_handle = private->handle - 1;

  switch (private->handle_mode)
    {
    case GIMP_HANDLE_MODE_ADD_TRANSFORM:
      if (n_handles < 4 && active_handle == -1)
        {
          /* add handle */

          GimpMatrix3 *matrix;

          active_handle = n_handles;

          private->trans_x[active_handle] = coords->x;
          private->trans_y[active_handle] = coords->y;
          private->n_handles++;

          if (! is_handle_position_valid (grid, active_handle))
            {
              handle_micro_move (grid, active_handle);
            }

          /* handle was added, calculate new original position */
          g_object_get (grid,
                        "transform", &matrix,
                        NULL);

          gimp_matrix3_invert (matrix);
          gimp_matrix3_transform_point (matrix,
                                        private->trans_x[active_handle],
                                        private->trans_y[active_handle],
                                        &private->orig_x[active_handle],
                                        &private->orig_y[active_handle]);

          g_free (matrix);

          private->handle = active_handle + 1;

          g_object_notify (G_OBJECT (grid), "n-handles");
        }
      break;

    case GIMP_HANDLE_MODE_MOVE:
      /* check for valid position and calculating of OX0...OY3 is
       * done on button release
       */
      break;

    case GIMP_HANDLE_MODE_REMOVE:
      if (n_handles > 0      &&
          active_handle >= 0 &&
          active_handle < 4)
        {
          /* remove handle */

          gdouble tempx  = private->trans_x[active_handle];
          gdouble tempy  = private->trans_y[active_handle];
          gdouble tempox = private->orig_x[active_handle];
          gdouble tempoy = private->orig_y[active_handle];
          gint    i;

          n_handles--;
          private->n_handles--;

          for (i = active_handle; i < n_handles; i++)
            {
              private->trans_x[i] = private->trans_x[i + 1];
              private->trans_y[i] = private->trans_y[i + 1];
              private->orig_x[i]  = private->orig_x[i + 1];
              private->orig_y[i]  = private->orig_y[i + 1];
            }

          private->trans_x[n_handles] = tempx;
          private->trans_y[n_handles] = tempy;
          private->orig_x[n_handles]  = tempox;
          private->orig_y[n_handles]  = tempoy;

          g_object_notify (G_OBJECT (grid), "n-handles");
        }
      break;
    }

  private->last_x = coords->x;
  private->last_y = coords->y;

  return private->handle;
}

static void
gimp_tool_handle_grid_button_release (GimpToolWidget        *widget,
                                      const GimpCoords      *coords,
                                      guint32                time,
                                      GdkModifierType        state,
                                      GimpButtonReleaseType  release_type)
{
  GimpToolHandleGrid        *grid          = GIMP_TOOL_HANDLE_GRID (widget);
  GimpToolHandleGridPrivate *private       = grid->private;
  gint                       active_handle = private->handle - 1;

  if (private->handle_mode == GIMP_HANDLE_MODE_MOVE &&
      active_handle >= 0 &&
      active_handle < 4)
    {
      GimpMatrix3 *matrix;

      if (! is_handle_position_valid (grid, active_handle))
        {
          handle_micro_move (grid, active_handle);
        }

      /* handle was moved, calculate new original position */
      g_object_get (grid,
                    "transform", &matrix,
                    NULL);

      gimp_matrix3_invert (matrix);
      gimp_matrix3_transform_point (matrix,
                                    private->trans_x[active_handle],
                                    private->trans_y[active_handle],
                                    &private->orig_x[active_handle],
                                    &private->orig_y[active_handle]);

      g_free (matrix);

      gimp_tool_handle_grid_update_matrix (grid);
    }
}

void
gimp_tool_handle_grid_motion (GimpToolWidget   *widget,
                              const GimpCoords *coords,
                              guint32           time,
                              GdkModifierType   state)
{
  GimpToolHandleGrid        *grid          = GIMP_TOOL_HANDLE_GRID (widget);
  GimpToolHandleGridPrivate *private       = grid->private;
  gint                       n_handles     = private->n_handles;
  gint                       active_handle = private->handle - 1;
  gdouble                    diff_x        = coords->x - private->last_x;
  gdouble                    diff_y        = coords->y - private->last_y;

  private->mouse_x = coords->x;
  private->mouse_y = coords->y;

  if (active_handle >= 0 && active_handle < 4)
    {
      if (private->handle_mode == GIMP_HANDLE_MODE_MOVE)
        {
          private->trans_x[active_handle] += diff_x;
          private->trans_y[active_handle] += diff_y;

          /* check for valid position and calculating of OX0...OY3 is
           * done on button release hopefully this makes the code run
           * faster Moving could be even faster if there was caching
           * for the image preview
           */
          gimp_canvas_handle_set_position (private->handles[active_handle + 1],
                                           private->trans_x[active_handle],
                                           private->trans_y[active_handle]);
        }
      else if (private->handle_mode == GIMP_HANDLE_MODE_ADD_TRANSFORM)
        {
          gdouble angle, angle_sin, angle_cos, scale;
          gdouble fixed_handles_x[3];
          gdouble fixed_handles_y[3];
          gdouble oldpos_x[4], oldpos_y[4];
          gdouble newpos_x[4], newpos_y[4];
          gint    i, j;

          for (i = 0, j = 0; i < 4; i++)
            {
              /* Find all visible handles that are not being moved */
              if (i < n_handles && i != active_handle)
                {
                  fixed_handles_x[j] = private->trans_x[i];
                  fixed_handles_y[j] = private->trans_y[i];
                  j++;
                }

              newpos_x[i] = oldpos_x[i] = private->trans_x[i];
              newpos_y[i] = oldpos_y[i] = private->trans_y[i];
            }

          newpos_x[active_handle] = oldpos_x[active_handle] + diff_x;
          newpos_y[active_handle] = oldpos_y[active_handle] + diff_y;

          switch (n_handles)
            {
            case 1:
              /* move */
              for (i = 0; i < 4; i++)
                {
                  newpos_x[i] = oldpos_x[i] + diff_x;
                  newpos_y[i] = oldpos_y[i] + diff_y;
                }
              break;

            case 2:
              /* rotate and keep-aspect-scale */
              scale =
                calc_len (newpos_x[active_handle] - fixed_handles_x[0],
                          newpos_y[active_handle] - fixed_handles_y[0]) /
                calc_len (oldpos_x[active_handle] - fixed_handles_x[0],
                          oldpos_y[active_handle] - fixed_handles_y[0]);

              angle = calc_angle (oldpos_x[active_handle] - fixed_handles_x[0],
                                  oldpos_y[active_handle] - fixed_handles_y[0],
                                  newpos_x[active_handle] - fixed_handles_x[0],
                                  newpos_y[active_handle] - fixed_handles_y[0]);

              angle_sin = sin (angle);
              angle_cos = cos (angle);

              for (i = 2; i < 4; i++)
                {
                  newpos_x[i] =
                    fixed_handles_x[0] +
                    scale * (angle_cos * (oldpos_x[i] - fixed_handles_x[0]) +
                             angle_sin * (oldpos_y[i] - fixed_handles_y[0]));

                  newpos_y[i] =
                    fixed_handles_y[0] +
                    scale * (-angle_sin * (oldpos_x[i] - fixed_handles_x[0]) +
                              angle_cos * (oldpos_y[i] - fixed_handles_y[0]));
                }
              break;

            case 3:
              /* shear and non-aspect-scale */
              scale = calc_lineintersect_ratio (oldpos_x[3],
                                                oldpos_y[3],
                                                oldpos_x[active_handle],
                                                oldpos_y[active_handle],
                                                fixed_handles_x[0],
                                                fixed_handles_y[0],
                                                fixed_handles_x[1],
                                                fixed_handles_y[1]);

              newpos_x[3] = oldpos_x[3] + scale * diff_x;
              newpos_y[3] = oldpos_y[3] + scale * diff_y;
              break;
            }

          for (i = 0; i < 4; i++)
            {
              private->trans_x[i] = newpos_x[i];
              private->trans_y[i] = newpos_y[i];
            }

          gimp_tool_handle_grid_update_matrix (grid);
        }
    }

  private->last_x = coords->x;
  private->last_y = coords->y;
}

static void
gimp_tool_handle_grid_hover (GimpToolWidget   *widget,
                             const GimpCoords *coords,
                             GdkModifierType   state,
                             gboolean          proximity)
{
  GimpToolHandleGrid        *grid    = GIMP_TOOL_HANDLE_GRID (widget);
  GimpToolHandleGridPrivate *private = grid->private;
  gchar                     *status  = NULL;
  gint                       i;

  private->mouse_x = coords->x;
  private->mouse_y = coords->y;

  private->handle = 0;

  for (i = 0; i < 4; i++)
    {
      if (private->handles[i + 1] &&
          gimp_canvas_item_hit (private->handles[i + 1],
                                coords->x, coords->y))
        {
          private->handle = i + 1;
          break;
        }
    }

  if (proximity)
    {
      GdkModifierType extend_mask = gimp_get_extend_selection_mask ();
      GdkModifierType toggle_mask = gimp_get_toggle_behavior_mask ();

      switch (private->handle_mode)
        {
        case GIMP_HANDLE_MODE_ADD_TRANSFORM:
          if (private->handle > 0)
            {
              const gchar *s = NULL;

              switch (private->n_handles)
                {
                case 1:
                  s = _("Click-Drag to move");
                  break;
                case 2:
                  s = _("Click-Drag to rotate and scale");
                  break;
                case 3:
                  s = _("Click-Drag to shear and scale");
                  break;
                case 4:
                  s = _("Click-Drag to change perspective");
                  break;
                }

              status = gimp_suggest_modifiers (s,
                                               extend_mask | toggle_mask,
                                               NULL, NULL, NULL);
            }
          else
            {
              if (private->n_handles < 4)
                status = g_strdup (_("Click to add a handle"));
            }
          break;

        case GIMP_HANDLE_MODE_MOVE:
          if (private->handle > 0)
            status = g_strdup (_("Click-Drag to move this handle"));
          break;

        case GIMP_HANDLE_MODE_REMOVE:
          if (private->handle > 0)
            status = g_strdup (_("Click-Drag to remove this handle"));
          break;
        }
    }

  gimp_tool_widget_set_status (widget, status);
  g_free (status);

  gimp_tool_handle_grid_update_hilight (grid);
}

static gboolean
gimp_tool_handle_grid_get_cursor (GimpToolWidget     *widget,
                                  const GimpCoords   *coords,
                                  GdkModifierType     state,
                                  GimpCursorType     *cursor,
                                  GimpToolCursorType *tool_cursor,
                                  GimpCursorModifier *cursor_modifier)
{
  GimpToolHandleGrid        *grid    = GIMP_TOOL_HANDLE_GRID (widget);
  GimpToolHandleGridPrivate *private = grid->private;

  *cursor          = GIMP_CURSOR_CROSSHAIR_SMALL;
  *tool_cursor     = GIMP_TOOL_CURSOR_NONE;
  *cursor_modifier = GIMP_CURSOR_MODIFIER_NONE;

  switch (private->handle_mode)
    {
    case GIMP_HANDLE_MODE_ADD_TRANSFORM:
      if (private->handle > 0)
        {
          switch (private->n_handles)
            {
            case 1:
              *tool_cursor = GIMP_TOOL_CURSOR_MOVE;
              break;
            case 2:
              *tool_cursor = GIMP_TOOL_CURSOR_ROTATE;
              break;
            case 3:
              *tool_cursor = GIMP_TOOL_CURSOR_SHEAR;
              break;
            case 4:
              *tool_cursor = GIMP_TOOL_CURSOR_PERSPECTIVE;
              break;
            }
        }
      else
        {
          if (private->n_handles < 4)
            *cursor_modifier = GIMP_CURSOR_MODIFIER_PLUS;
          else
            *cursor_modifier = GIMP_CURSOR_MODIFIER_BAD;
        }
      break;

    case GIMP_HANDLE_MODE_MOVE:
      if (private->handle > 0)
        *cursor_modifier = GIMP_CURSOR_MODIFIER_MOVE;
      else
        *cursor_modifier = GIMP_CURSOR_MODIFIER_BAD;
      break;

    case GIMP_HANDLE_MODE_REMOVE:
      if (private->handle > 0)
        *cursor_modifier = GIMP_CURSOR_MODIFIER_MINUS;
      else
        *cursor_modifier = GIMP_CURSOR_MODIFIER_BAD;
      break;
    }

  return TRUE;
}

static void
gimp_tool_handle_grid_update_hilight (GimpToolHandleGrid *grid)
{
  GimpToolHandleGridPrivate *private = grid->private;
  gint                       i;

  for (i = 0; i < 4; i++)
    {
      GimpCanvasItem *item = private->handles[i + 1];

      if (item)
        {
          gdouble diameter =
            gimp_canvas_handle_calc_size (item,
                                          private->mouse_x,
                                          private->mouse_y,
                                          GIMP_CANVAS_HANDLE_SIZE_CIRCLE,
                                          2 * GIMP_CANVAS_HANDLE_SIZE_CIRCLE);

          gimp_canvas_handle_set_size (item, diameter, diameter);
          gimp_canvas_item_set_highlight (item, (i + 1) == private->handle);
        }
    }
}

static void
gimp_tool_handle_grid_update_matrix (GimpToolHandleGrid *grid)
{
  GimpToolHandleGridPrivate *private = grid->private;
  GimpMatrix3                transform;

  gimp_matrix3_identity (&transform);
  gimp_transform_matrix_handles (&transform,
                                 private->orig_x[0],
                                 private->orig_y[0],
                                 private->orig_x[1],
                                 private->orig_y[1],
                                 private->orig_x[2],
                                 private->orig_y[2],
                                 private->orig_x[3],
                                 private->orig_y[3],
                                 private->trans_x[0],
                                 private->trans_y[0],
                                 private->trans_x[1],
                                 private->trans_y[1],
                                 private->trans_x[2],
                                 private->trans_y[2],
                                 private->trans_x[3],
                                 private->trans_y[3]);

  g_object_set (grid,
                "transform", &transform,
                NULL);
}

/* check if a handle is not on the connection line of two other handles */
static gboolean
is_handle_position_valid (GimpToolHandleGrid *grid,
                          gint                handle)
{
  GimpToolHandleGridPrivate *private = grid->private;
  gint                       i, j, k;

  for (i = 0; i < 2; i++)
    {
      for (j = i + 1; j < 3; j++)
        {
          for (k = j + 1; i < 4; i++)
            {
              if (handle == i ||
                  handle == j ||
                  handle == k)
                {
                  if ((private->trans_x[i] -
                       private->trans_x[j]) *
                      (private->trans_y[j] -
                       private->trans_y[k]) ==

                      (private->trans_x[j] -
                       private->trans_x[k]) *
                      (private->trans_y[i] -
                       private->trans_y[j]))
                    {
                      return FALSE;
                    }
                }
            }
        }
    }

  return TRUE;
}

/* three handles on a line causes problems.
 * Let's move the new handle around a bit to find a better position */
static void
handle_micro_move (GimpToolHandleGrid *grid,
                   gint                handle)
{
  GimpToolHandleGridPrivate *private = grid->private;
  gdouble                    posx    = private->trans_x[handle];
  gdouble                    posy    = private->trans_y[handle];
  gdouble                    dx, dy;

  for (dx = -0.1; dx < 0.11; dx += 0.1)
    {
      private->trans_x[handle] = posx + dx;

      for (dy = -0.1; dy < 0.11; dy += 0.1)
        {
          private->trans_y[handle] = posy + dy;

          if (is_handle_position_valid (grid, handle))
            {
              return;
            }
        }
    }
}

/* finds the clockwise angle between the vectors given, 0-2π */
static inline gdouble
calc_angle (gdouble ax,
            gdouble ay,
            gdouble bx,
            gdouble by)
{
  gdouble angle;
  gdouble direction;
  gdouble length = sqrt ((ax * ax + ay * ay) * (bx * bx + by * by));

  angle = acos ((ax * bx + ay * by) / length);
  direction = ax * by - ay * bx;

  return ((direction < 0) ? angle : 2 * G_PI - angle);
}

static inline gdouble
calc_len  (gdouble a,
           gdouble b)
{
  return sqrt (a * a + b * b);
}

/* imagine two lines, one through the points p1 and p2, the other one
 * through the points q1 and q2. Find the intersection point r.
 * Calculate (distance p1 to r)/(distance p2 to r)
 */
static inline gdouble
calc_lineintersect_ratio (gdouble p1x, gdouble p1y,
                          gdouble p2x, gdouble p2y,
                          gdouble q1x, gdouble q1y,
                          gdouble q2x, gdouble q2y)
{
  gdouble denom, u;

  denom = (q2y - q1y) * (p2x - p1x) - (q2x - q1x) * (p2y - p1y);
  if (denom == 0.0)
    {
      /* u is infinite, so u/(u-1) is 1 */
      return 1.0;
    }

  u = (q2y - q1y) * (q1x - p1x) - (q1y - p1y) * (q2x - q1x);
  u /= denom;

  return u / (u - 1);
}


/*  public functions  */

GimpToolWidget *
gimp_tool_handle_grid_new (GimpDisplayShell *shell,
                           gdouble           x1,
                           gdouble           y1,
                           gdouble           x2,
                           gdouble           y2)
{
  g_return_val_if_fail (GIMP_IS_DISPLAY_SHELL (shell), NULL);

  return g_object_new (GIMP_TYPE_TOOL_HANDLE_GRID,
                       "shell",     shell,
                       "x1",        x1,
                       "y1",        y1,
                       "x2",        x2,
                       "y2",        y2,
                       NULL);
}
