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

  gint        n_handles;
  GimpVector2 orig[4];
  GimpVector2 trans[4];

  gint     handle;
  gdouble  last_x;
  gdouble  last_y;

  gboolean hover;
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
static GimpHit  gimp_tool_handle_grid_hit          (GimpToolWidget        *widget,
                                                    const GimpCoords      *coords,
                                                    GdkModifierType        state,
                                                    gboolean               proximity);
static void     gimp_tool_handle_grid_hover        (GimpToolWidget        *widget,
                                                    const GimpCoords      *coords,
                                                    GdkModifierType        state,
                                                    gboolean               proximity);
static void     gimp_tool_handle_grid_leave_notify (GimpToolWidget        *widget);
static gboolean gimp_tool_handle_grid_get_cursor   (GimpToolWidget        *widget,
                                                    const GimpCoords      *coords,
                                                    GdkModifierType        state,
                                                    GimpCursorType        *cursor,
                                                    GimpToolCursorType    *tool_cursor,
                                                    GimpCursorModifier    *modifier);

static gint     gimp_tool_handle_grid_get_handle     (GimpToolHandleGrid  *grid,
                                                      const GimpCoords    *coords);
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


G_DEFINE_TYPE_WITH_PRIVATE (GimpToolHandleGrid, gimp_tool_handle_grid,
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
  widget_class->hit             = gimp_tool_handle_grid_hit;
  widget_class->hover           = gimp_tool_handle_grid_hover;
  widget_class->leave_notify    = gimp_tool_handle_grid_leave_notify;
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
}

static void
gimp_tool_handle_grid_init (GimpToolHandleGrid *grid)
{
  grid->private = gimp_tool_handle_grid_get_instance_private (grid);
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
      private->orig[0].x = g_value_get_double (value);
      break;
    case PROP_ORIG_Y1:
      private->orig[0].y = g_value_get_double (value);
      break;
    case PROP_ORIG_X2:
      private->orig[1].x = g_value_get_double (value);
      break;
    case PROP_ORIG_Y2:
      private->orig[1].y = g_value_get_double (value);
      break;
    case PROP_ORIG_X3:
      private->orig[2].x = g_value_get_double (value);
      break;
    case PROP_ORIG_Y3:
      private->orig[2].y = g_value_get_double (value);
      break;
    case PROP_ORIG_X4:
      private->orig[3].x = g_value_get_double (value);
      break;
    case PROP_ORIG_Y4:
      private->orig[3].y = g_value_get_double (value);
      break;

    case PROP_TRANS_X1:
      private->trans[0].x = g_value_get_double (value);
      break;
    case PROP_TRANS_Y1:
      private->trans[0].y = g_value_get_double (value);
      break;
    case PROP_TRANS_X2:
      private->trans[1].x = g_value_get_double (value);
      break;
    case PROP_TRANS_Y2:
      private->trans[1].y = g_value_get_double (value);
      break;
    case PROP_TRANS_X3:
      private->trans[2].x = g_value_get_double (value);
      break;
    case PROP_TRANS_Y3:
      private->trans[2].y = g_value_get_double (value);
      break;
    case PROP_TRANS_X4:
      private->trans[3].x = g_value_get_double (value);
      break;
    case PROP_TRANS_Y4:
      private->trans[3].y = g_value_get_double (value);
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
      g_value_set_double (value, private->orig[0].x);
      break;
    case PROP_ORIG_Y1:
      g_value_set_double (value, private->orig[0].y);
      break;
    case PROP_ORIG_X2:
      g_value_set_double (value, private->orig[1].x);
      break;
    case PROP_ORIG_Y2:
      g_value_set_double (value, private->orig[1].y);
      break;
    case PROP_ORIG_X3:
      g_value_set_double (value, private->orig[2].x);
      break;
    case PROP_ORIG_Y3:
      g_value_set_double (value, private->orig[2].y);
      break;
    case PROP_ORIG_X4:
      g_value_set_double (value, private->orig[3].x);
      break;
    case PROP_ORIG_Y4:
      g_value_set_double (value, private->orig[3].y);
      break;

    case PROP_TRANS_X1:
      g_value_set_double (value, private->trans[0].x);
      break;
    case PROP_TRANS_Y1:
      g_value_set_double (value, private->trans[0].y);
      break;
    case PROP_TRANS_X2:
      g_value_set_double (value, private->trans[1].x);
      break;
    case PROP_TRANS_Y2:
      g_value_set_double (value, private->trans[1].y);
      break;
    case PROP_TRANS_X3:
      g_value_set_double (value, private->trans[2].x);
      break;
    case PROP_TRANS_Y3:
      g_value_set_double (value, private->trans[2].y);
      break;
    case PROP_TRANS_X4:
      g_value_set_double (value, private->trans[3].x);
      break;
    case PROP_TRANS_Y4:
      g_value_set_double (value, private->trans[3].y);
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
                                       private->trans[i].x,
                                       private->trans[i].y);
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

          private->trans[active_handle].x = coords->x;
          private->trans[active_handle].y = coords->y;
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
                                        private->trans[active_handle].x,
                                        private->trans[active_handle].y,
                                        &private->orig[active_handle].x,
                                        &private->orig[active_handle].y);

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

          GimpVector2 temp  = private->trans[active_handle];
          GimpVector2 tempo = private->orig[active_handle];
          gint        i;

          n_handles--;
          private->n_handles--;

          for (i = active_handle; i < n_handles; i++)
            {
              private->trans[i] = private->trans[i + 1];
              private->orig[i]  = private->orig[i + 1];
            }

          private->trans[n_handles] = temp;
          private->orig[n_handles]  = tempo;

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
                                    private->trans[active_handle].x,
                                    private->trans[active_handle].y,
                                    &private->orig[active_handle].x,
                                    &private->orig[active_handle].y);

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
          private->trans[active_handle].x += diff_x;
          private->trans[active_handle].y += diff_y;

          /* check for valid position and calculating of OX0...OY3 is
           * done on button release hopefully this makes the code run
           * faster Moving could be even faster if there was caching
           * for the image preview
           */
          gimp_canvas_handle_set_position (private->handles[active_handle + 1],
                                           private->trans[active_handle].x,
                                           private->trans[active_handle].y);
        }
      else if (private->handle_mode == GIMP_HANDLE_MODE_ADD_TRANSFORM)
        {
          gdouble angle, angle_sin, angle_cos, scale;
          GimpVector2 fixed_handles[3];
          GimpVector2 oldpos[4];
          GimpVector2 newpos[4];
          gint    i, j;

          for (i = 0, j = 0; i < 4; i++)
            {
              /* Find all visible handles that are not being moved */
              if (i < n_handles && i != active_handle)
                {
                  fixed_handles[j] = private->trans[i];
                  j++;
                }

              newpos[i] = oldpos[i] = private->trans[i];
            }

          newpos[active_handle].x = oldpos[active_handle].x + diff_x;
          newpos[active_handle].y = oldpos[active_handle].y + diff_y;

          switch (n_handles)
            {
            case 1:
              /* move */
              for (i = 0; i < 4; i++)
                {
                  newpos[i].x = oldpos[i].x + diff_x;
                  newpos[i].y = oldpos[i].y + diff_y;
                }
              break;

            case 2:
              /* rotate and keep-aspect-scale */
              scale =
                calc_len (newpos[active_handle].x - fixed_handles[0].x,
                          newpos[active_handle].y - fixed_handles[0].y) /
                calc_len (oldpos[active_handle].x - fixed_handles[0].x,
                          oldpos[active_handle].y - fixed_handles[0].y);

              angle = calc_angle (oldpos[active_handle].x - fixed_handles[0].x,
                                  oldpos[active_handle].y - fixed_handles[0].y,
                                  newpos[active_handle].x - fixed_handles[0].x,
                                  newpos[active_handle].y - fixed_handles[0].y);

              angle_sin = sin (angle);
              angle_cos = cos (angle);

              for (i = 2; i < 4; i++)
                {
                  newpos[i].x =
                    fixed_handles[0].x +
                    scale * (angle_cos * (oldpos[i].x - fixed_handles[0].x) +
                             angle_sin * (oldpos[i].y - fixed_handles[0].y));

                  newpos[i].y =
                    fixed_handles[0].y +
                    scale * (-angle_sin * (oldpos[i].x - fixed_handles[0].x) +
                              angle_cos * (oldpos[i].y - fixed_handles[0].y));
                }
              break;

            case 3:
              /* shear and non-aspect-scale */
              scale = calc_lineintersect_ratio (oldpos[3].x,
                                                oldpos[3].y,
                                                oldpos[active_handle].x,
                                                oldpos[active_handle].y,
                                                fixed_handles[0].x,
                                                fixed_handles[0].y,
                                                fixed_handles[1].x,
                                                fixed_handles[1].y);

              newpos[3].x = oldpos[3].x + scale * diff_x;
              newpos[3].y = oldpos[3].y + scale * diff_y;
              break;
            }

          for (i = 0; i < 4; i++)
            {
              private->trans[i] = newpos[i];
            }

          gimp_tool_handle_grid_update_matrix (grid);
        }
    }

  private->last_x = coords->x;
  private->last_y = coords->y;
}

static GimpHit
gimp_tool_handle_grid_hit (GimpToolWidget   *widget,
                           const GimpCoords *coords,
                           GdkModifierType   state,
                           gboolean          proximity)
{
  GimpToolHandleGrid        *grid    = GIMP_TOOL_HANDLE_GRID (widget);
  GimpToolHandleGridPrivate *private = grid->private;

  if (proximity)
    {
      gint handle = gimp_tool_handle_grid_get_handle (grid, coords);

      switch (private->handle_mode)
        {
        case GIMP_HANDLE_MODE_ADD_TRANSFORM:
          if (handle > 0)
            return GIMP_HIT_DIRECT;
          else
            return GIMP_HIT_INDIRECT;
          break;

        case GIMP_HANDLE_MODE_MOVE:
        case GIMP_HANDLE_MODE_REMOVE:
          if (private->handle > 0)
            return GIMP_HIT_DIRECT;
          break;
        }
    }

  return GIMP_HIT_NONE;
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

  private->hover   = TRUE;
  private->mouse_x = coords->x;
  private->mouse_y = coords->y;

  private->handle = gimp_tool_handle_grid_get_handle (grid, coords);

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

static void
gimp_tool_handle_grid_leave_notify (GimpToolWidget *widget)
{
  GimpToolHandleGrid        *grid    = GIMP_TOOL_HANDLE_GRID (widget);
  GimpToolHandleGridPrivate *private = grid->private;

  private->hover  = FALSE;
  private->handle = 0;

  gimp_tool_handle_grid_update_hilight (grid);

  GIMP_TOOL_WIDGET_CLASS (parent_class)->leave_notify (widget);
}

static gboolean
gimp_tool_handle_grid_get_cursor (GimpToolWidget     *widget,
                                  const GimpCoords   *coords,
                                  GdkModifierType     state,
                                  GimpCursorType     *cursor,
                                  GimpToolCursorType *tool_cursor,
                                  GimpCursorModifier *modifier)
{
  GimpToolHandleGrid        *grid    = GIMP_TOOL_HANDLE_GRID (widget);
  GimpToolHandleGridPrivate *private = grid->private;

  *cursor      = GIMP_CURSOR_CROSSHAIR_SMALL;
  *tool_cursor = GIMP_TOOL_CURSOR_NONE;
  *modifier    = GIMP_CURSOR_MODIFIER_NONE;

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
            *modifier = GIMP_CURSOR_MODIFIER_PLUS;
          else
            *modifier = GIMP_CURSOR_MODIFIER_BAD;
        }
      break;

    case GIMP_HANDLE_MODE_MOVE:
      if (private->handle > 0)
        *modifier = GIMP_CURSOR_MODIFIER_MOVE;
      else
        *modifier = GIMP_CURSOR_MODIFIER_BAD;
      break;

    case GIMP_HANDLE_MODE_REMOVE:
      if (private->handle > 0)
        *modifier = GIMP_CURSOR_MODIFIER_MINUS;
      else
        *modifier = GIMP_CURSOR_MODIFIER_BAD;
      break;
    }

  return TRUE;
}

static gint
gimp_tool_handle_grid_get_handle (GimpToolHandleGrid *grid,
                                  const GimpCoords   *coords)
{
  GimpToolHandleGridPrivate *private = grid->private;
  gint                       i;

  for (i = 0; i < 4; i++)
    {
      if (private->handles[i + 1] &&
          gimp_canvas_item_hit (private->handles[i + 1],
                                coords->x, coords->y))
        {
          return i + 1;
        }
    }

  return 0;
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
          gdouble diameter = GIMP_CANVAS_HANDLE_SIZE_CIRCLE;

          if (private->hover)
            {
              diameter = gimp_canvas_handle_calc_size (
                item,
                private->mouse_x,
                private->mouse_y,
                GIMP_CANVAS_HANDLE_SIZE_CIRCLE,
                2 * GIMP_CANVAS_HANDLE_SIZE_CIRCLE);
            }

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
  gboolean                   transform_valid;

  gimp_matrix3_identity (&transform);
  transform_valid = gimp_transform_matrix_generic (&transform,
                                                   private->orig,
                                                   private->trans);

  g_object_set (grid,
                "transform",   &transform,
                "show-guides", transform_valid,
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
                  if ((private->trans[i].x -
                       private->trans[j].x) *
                      (private->trans[j].y -
                       private->trans[k].y) ==

                      (private->trans[j].x -
                       private->trans[k].x) *
                      (private->trans[i].y -
                       private->trans[j].y))
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
  gdouble                    posx    = private->trans[handle].x;
  gdouble                    posy    = private->trans[handle].y;
  gdouble                    dx, dy;

  for (dx = -0.1; dx < 0.11; dx += 0.1)
    {
      private->trans[handle].x = posx + dx;

      for (dy = -0.1; dy < 0.11; dy += 0.1)
        {
          private->trans[handle].y = posy + dy;

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
                       "shell",       shell,
                       "x1",          x1,
                       "y1",          y1,
                       "x2",          x2,
                       "y2",          y2,
                       "clip-guides", TRUE,
                       NULL);
}
