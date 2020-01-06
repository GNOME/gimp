/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimptooltransformgrid.c
 * Copyright (C) 2017 Michael Natterer <mitch@gimp.org>
 *
 * Based on GimpUnifiedTransformTool
 * Copyright (C) 2011 Mikael Magnusson <mikachu@src.gnome.org>
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
#include "gimpcanvastransformguides.h"
#include "gimpdisplayshell.h"
#include "gimptooltransformgrid.h"

#include "gimp-intl.h"


#define MIN_HANDLE_SIZE 6


enum
{
  PROP_0,
  PROP_TRANSFORM,
  PROP_X1,
  PROP_Y1,
  PROP_X2,
  PROP_Y2,
  PROP_PIVOT_X,
  PROP_PIVOT_Y,
  PROP_GUIDE_TYPE,
  PROP_N_GUIDES,
  PROP_CLIP_GUIDES,
  PROP_SHOW_GUIDES,
  PROP_INSIDE_FUNCTION,
  PROP_OUTSIDE_FUNCTION,
  PROP_USE_CORNER_HANDLES,
  PROP_USE_PERSPECTIVE_HANDLES,
  PROP_USE_SIDE_HANDLES,
  PROP_USE_SHEAR_HANDLES,
  PROP_USE_CENTER_HANDLE,
  PROP_USE_PIVOT_HANDLE,
  PROP_DYNAMIC_HANDLE_SIZE,
  PROP_CONSTRAIN_MOVE,
  PROP_CONSTRAIN_SCALE,
  PROP_CONSTRAIN_ROTATE,
  PROP_CONSTRAIN_SHEAR,
  PROP_CONSTRAIN_PERSPECTIVE,
  PROP_FROMPIVOT_SCALE,
  PROP_FROMPIVOT_SHEAR,
  PROP_FROMPIVOT_PERSPECTIVE,
  PROP_CORNERSNAP,
  PROP_FIXEDPIVOT
};


struct _GimpToolTransformGridPrivate
{
  GimpMatrix3            transform;
  gdouble                x1, y1;
  gdouble                x2, y2;
  gdouble                pivot_x;
  gdouble                pivot_y;
  GimpGuidesType         guide_type;
  gint                   n_guides;
  gboolean               clip_guides;
  gboolean               show_guides;
  GimpTransformFunction  inside_function;
  GimpTransformFunction  outside_function;
  gboolean               use_corner_handles;
  gboolean               use_perspective_handles;
  gboolean               use_side_handles;
  gboolean               use_shear_handles;
  gboolean               use_center_handle;
  gboolean               use_pivot_handle;
  gboolean               dynamic_handle_size;
  gboolean               constrain_move;
  gboolean               constrain_scale;
  gboolean               constrain_rotate;
  gboolean               constrain_shear;
  gboolean               constrain_perspective;
  gboolean               frompivot_scale;
  gboolean               frompivot_shear;
  gboolean               frompivot_perspective;
  gboolean               cornersnap;
  gboolean               fixedpivot;

  gdouble                curx;         /*  current x coord                    */
  gdouble                cury;         /*  current y coord                    */

  gdouble                button_down;  /*  is the mouse button pressed        */
  gdouble                mousex;       /*  x coord where mouse was clicked    */
  gdouble                mousey;       /*  y coord where mouse was clicked    */

  gdouble                cx, cy;       /*  center point (for moving)          */

  /*  transformed handle coords */
  gdouble                tx1, ty1;
  gdouble                tx2, ty2;
  gdouble                tx3, ty3;
  gdouble                tx4, ty4;
  gdouble                tcx, tcy;
  gdouble                tpx, tpy;

  /*  previous transformed handle coords */
  gdouble                prev_tx1, prev_ty1;
  gdouble                prev_tx2, prev_ty2;
  gdouble                prev_tx3, prev_ty3;
  gdouble                prev_tx4, prev_ty4;
  gdouble                prev_tcx, prev_tcy;
  gdouble                prev_tpx, prev_tpy;

  GimpTransformHandle    handle;        /*  current tool activity              */

  GimpCanvasItem        *guides;
  GimpCanvasItem        *handles[GIMP_N_TRANSFORM_HANDLES];
  GimpCanvasItem        *center_items[2];
  GimpCanvasItem        *pivot_items[2];
};


/*  local function prototypes  */

static void     gimp_tool_transform_grid_constructed    (GObject               *object);
static void     gimp_tool_transform_grid_set_property   (GObject               *object,
                                                         guint                  property_id,
                                                         const GValue          *value,
                                                         GParamSpec            *pspec);
static void     gimp_tool_transform_grid_get_property   (GObject               *object,
                                                         guint                  property_id,
                                                         GValue                *value,
                                                         GParamSpec            *pspec);

static void     gimp_tool_transform_grid_changed        (GimpToolWidget        *widget);
static gint     gimp_tool_transform_grid_button_press   (GimpToolWidget        *widget,
                                                         const GimpCoords      *coords,
                                                         guint32                time,
                                                         GdkModifierType        state,
                                                         GimpButtonPressType    press_type);
static void     gimp_tool_transform_grid_button_release (GimpToolWidget        *widget,
                                                         const GimpCoords      *coords,
                                                         guint32                time,
                                                         GdkModifierType        state,
                                                         GimpButtonReleaseType  release_type);
static void     gimp_tool_transform_grid_motion         (GimpToolWidget        *widget,
                                                         const GimpCoords      *coords,
                                                         guint32                time,
                                                         GdkModifierType        state);
static GimpHit  gimp_tool_transform_grid_hit            (GimpToolWidget        *widget,
                                                         const GimpCoords      *coords,
                                                         GdkModifierType        state,
                                                         gboolean               proximity);
static void     gimp_tool_transform_grid_hover          (GimpToolWidget        *widget,
                                                         const GimpCoords      *coords,
                                                         GdkModifierType        state,
                                                         gboolean               proximity);
static void     gimp_tool_transform_grid_leave_notify   (GimpToolWidget        *widget);
static void     gimp_tool_transform_grid_hover_modifier (GimpToolWidget        *widget,
                                                         GdkModifierType        key,
                                                         gboolean               press,
                                                         GdkModifierType        state);
static gboolean gimp_tool_transform_grid_get_cursor     (GimpToolWidget        *widget,
                                                         const GimpCoords      *coords,
                                                         GdkModifierType        state,
                                                         GimpCursorType        *cursor,
                                                         GimpToolCursorType    *tool_cursor,
                                                         GimpCursorModifier    *modifier);

static GimpTransformHandle
                gimp_tool_transform_grid_get_handle_for_coords
                                                        (GimpToolTransformGrid *grid,
                                                         const GimpCoords      *coords);
static void     gimp_tool_transform_grid_update_hilight (GimpToolTransformGrid *grid);
static void     gimp_tool_transform_grid_update_box     (GimpToolTransformGrid *grid);
static void     gimp_tool_transform_grid_update_matrix  (GimpToolTransformGrid *grid);
static void     gimp_tool_transform_grid_calc_handles   (GimpToolTransformGrid *grid,
                                                         gint                  *handle_w,
                                                         gint                  *handle_h);


G_DEFINE_TYPE_WITH_PRIVATE (GimpToolTransformGrid, gimp_tool_transform_grid,
                            GIMP_TYPE_TOOL_WIDGET)

#define parent_class gimp_tool_transform_grid_parent_class


static void
gimp_tool_transform_grid_class_init (GimpToolTransformGridClass *klass)
{
  GObjectClass        *object_class = G_OBJECT_CLASS (klass);
  GimpToolWidgetClass *widget_class = GIMP_TOOL_WIDGET_CLASS (klass);

  object_class->constructed     = gimp_tool_transform_grid_constructed;
  object_class->set_property    = gimp_tool_transform_grid_set_property;
  object_class->get_property    = gimp_tool_transform_grid_get_property;

  widget_class->changed         = gimp_tool_transform_grid_changed;
  widget_class->button_press    = gimp_tool_transform_grid_button_press;
  widget_class->button_release  = gimp_tool_transform_grid_button_release;
  widget_class->motion          = gimp_tool_transform_grid_motion;
  widget_class->hit             = gimp_tool_transform_grid_hit;
  widget_class->hover           = gimp_tool_transform_grid_hover;
  widget_class->leave_notify    = gimp_tool_transform_grid_leave_notify;
  widget_class->hover_modifier  = gimp_tool_transform_grid_hover_modifier;
  widget_class->get_cursor      = gimp_tool_transform_grid_get_cursor;

  g_object_class_install_property (object_class, PROP_TRANSFORM,
                                   gimp_param_spec_matrix3 ("transform",
                                                            NULL, NULL,
                                                            NULL,
                                                            GIMP_PARAM_READWRITE |
                                                            G_PARAM_CONSTRUCT));

  g_object_class_install_property (object_class, PROP_X1,
                                   g_param_spec_double ("x1",
                                                        NULL, NULL,
                                                        -GIMP_MAX_IMAGE_SIZE,
                                                        GIMP_MAX_IMAGE_SIZE,
                                                        0.0,
                                                        GIMP_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT));

  g_object_class_install_property (object_class, PROP_Y1,
                                   g_param_spec_double ("y1",
                                                        NULL, NULL,
                                                        -GIMP_MAX_IMAGE_SIZE,
                                                        GIMP_MAX_IMAGE_SIZE,
                                                        0.0,
                                                        GIMP_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT));

  g_object_class_install_property (object_class, PROP_X2,
                                   g_param_spec_double ("x2",
                                                        NULL, NULL,
                                                        -GIMP_MAX_IMAGE_SIZE,
                                                        GIMP_MAX_IMAGE_SIZE,
                                                        0.0,
                                                        GIMP_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT));

  g_object_class_install_property (object_class, PROP_Y2,
                                   g_param_spec_double ("y2",
                                                        NULL, NULL,
                                                        -GIMP_MAX_IMAGE_SIZE,
                                                        GIMP_MAX_IMAGE_SIZE,
                                                        0.0,
                                                        GIMP_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT));

  g_object_class_install_property (object_class, PROP_PIVOT_X,
                                   g_param_spec_double ("pivot-x",
                                                        NULL, NULL,
                                                        -GIMP_MAX_IMAGE_SIZE,
                                                        GIMP_MAX_IMAGE_SIZE,
                                                        0.0,
                                                        GIMP_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT));

  g_object_class_install_property (object_class, PROP_PIVOT_Y,
                                   g_param_spec_double ("pivot-y",
                                                        NULL, NULL,
                                                        -GIMP_MAX_IMAGE_SIZE,
                                                        GIMP_MAX_IMAGE_SIZE,
                                                        0.0,
                                                        GIMP_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT));

  g_object_class_install_property (object_class, PROP_GUIDE_TYPE,
                                   g_param_spec_enum ("guide-type", NULL, NULL,
                                                      GIMP_TYPE_GUIDES_TYPE,
                                                      GIMP_GUIDES_NONE,
                                                      GIMP_PARAM_READWRITE |
                                                      G_PARAM_CONSTRUCT));

  g_object_class_install_property (object_class, PROP_N_GUIDES,
                                   g_param_spec_int ("n-guides", NULL, NULL,
                                                     1, 128, 4,
                                                     GIMP_PARAM_READWRITE |
                                                     G_PARAM_CONSTRUCT));

  g_object_class_install_property (object_class, PROP_CLIP_GUIDES,
                                   g_param_spec_boolean ("clip-guides", NULL, NULL,
                                                         FALSE,
                                                         GIMP_PARAM_READWRITE |
                                                         G_PARAM_CONSTRUCT));

  g_object_class_install_property (object_class, PROP_SHOW_GUIDES,
                                   g_param_spec_boolean ("show-guides", NULL, NULL,
                                                         TRUE,
                                                         GIMP_PARAM_READWRITE |
                                                         G_PARAM_CONSTRUCT));

  g_object_class_install_property (object_class, PROP_INSIDE_FUNCTION,
                                   g_param_spec_enum ("inside-function",
                                                      NULL, NULL,
                                                      GIMP_TYPE_TRANSFORM_FUNCTION,
                                                      GIMP_TRANSFORM_FUNCTION_MOVE,
                                                      GIMP_PARAM_READWRITE |
                                                      G_PARAM_CONSTRUCT));

  g_object_class_install_property (object_class, PROP_OUTSIDE_FUNCTION,
                                   g_param_spec_enum ("outside-function",
                                                      NULL, NULL,
                                                      GIMP_TYPE_TRANSFORM_FUNCTION,
                                                      GIMP_TRANSFORM_FUNCTION_ROTATE,
                                                      GIMP_PARAM_READWRITE |
                                                      G_PARAM_CONSTRUCT));

  g_object_class_install_property (object_class, PROP_USE_CORNER_HANDLES,
                                   g_param_spec_boolean ("use-corner-handles",
                                                         NULL, NULL,
                                                         FALSE,
                                                         GIMP_PARAM_READWRITE |
                                                         G_PARAM_CONSTRUCT));

  g_object_class_install_property (object_class, PROP_USE_PERSPECTIVE_HANDLES,
                                   g_param_spec_boolean ("use-perspective-handles",
                                                         NULL, NULL,
                                                         FALSE,
                                                         GIMP_PARAM_READWRITE |
                                                         G_PARAM_CONSTRUCT));

  g_object_class_install_property (object_class, PROP_USE_SIDE_HANDLES,
                                   g_param_spec_boolean ("use-side-handles",
                                                         NULL, NULL,
                                                         FALSE,
                                                         GIMP_PARAM_READWRITE |
                                                         G_PARAM_CONSTRUCT));

  g_object_class_install_property (object_class, PROP_USE_SHEAR_HANDLES,
                                   g_param_spec_boolean ("use-shear-handles",
                                                         NULL, NULL,
                                                         FALSE,
                                                         GIMP_PARAM_READWRITE |
                                                         G_PARAM_CONSTRUCT));

  g_object_class_install_property (object_class, PROP_USE_CENTER_HANDLE,
                                   g_param_spec_boolean ("use-center-handle",
                                                         NULL, NULL,
                                                         FALSE,
                                                         GIMP_PARAM_READWRITE |
                                                         G_PARAM_CONSTRUCT));

  g_object_class_install_property (object_class, PROP_USE_PIVOT_HANDLE,
                                   g_param_spec_boolean ("use-pivot-handle",
                                                         NULL, NULL,
                                                         FALSE,
                                                         GIMP_PARAM_READWRITE |
                                                         G_PARAM_CONSTRUCT));

  g_object_class_install_property (object_class, PROP_DYNAMIC_HANDLE_SIZE,
                                   g_param_spec_boolean ("dynamic-handle-size",
                                                         NULL, NULL,
                                                         TRUE,
                                                         GIMP_PARAM_READWRITE |
                                                         G_PARAM_CONSTRUCT));

  g_object_class_install_property (object_class, PROP_CONSTRAIN_MOVE,
                                   g_param_spec_boolean ("constrain-move",
                                                         NULL, NULL,
                                                         FALSE,
                                                         GIMP_PARAM_READWRITE |
                                                         G_PARAM_CONSTRUCT));

  g_object_class_install_property (object_class, PROP_CONSTRAIN_SCALE,
                                   g_param_spec_boolean ("constrain-scale",
                                                         NULL, NULL,
                                                         FALSE,
                                                         GIMP_PARAM_READWRITE |
                                                         G_PARAM_CONSTRUCT));

  g_object_class_install_property (object_class, PROP_CONSTRAIN_ROTATE,
                                   g_param_spec_boolean ("constrain-rotate",
                                                         NULL, NULL,
                                                         FALSE,
                                                         GIMP_PARAM_READWRITE |
                                                         G_PARAM_CONSTRUCT));

  g_object_class_install_property (object_class, PROP_CONSTRAIN_SHEAR,
                                   g_param_spec_boolean ("constrain-shear",
                                                         NULL, NULL,
                                                         FALSE,
                                                         GIMP_PARAM_READWRITE |
                                                         G_PARAM_CONSTRUCT));

  g_object_class_install_property (object_class, PROP_CONSTRAIN_PERSPECTIVE,
                                   g_param_spec_boolean ("constrain-perspective",
                                                         NULL, NULL,
                                                         FALSE,
                                                         GIMP_PARAM_READWRITE |
                                                         G_PARAM_CONSTRUCT));

  g_object_class_install_property (object_class, PROP_FROMPIVOT_SCALE,
                                   g_param_spec_boolean ("frompivot-scale",
                                                         NULL, NULL,
                                                         FALSE,
                                                         GIMP_PARAM_READWRITE |
                                                         G_PARAM_CONSTRUCT));

  g_object_class_install_property (object_class, PROP_FROMPIVOT_SHEAR,
                                   g_param_spec_boolean ("frompivot-shear",
                                                         NULL, NULL,
                                                         FALSE,
                                                         GIMP_PARAM_READWRITE |
                                                         G_PARAM_CONSTRUCT));

  g_object_class_install_property (object_class, PROP_FROMPIVOT_PERSPECTIVE,
                                   g_param_spec_boolean ("frompivot-perspective",
                                                         NULL, NULL,
                                                         FALSE,
                                                         GIMP_PARAM_READWRITE |
                                                         G_PARAM_CONSTRUCT));

  g_object_class_install_property (object_class, PROP_CORNERSNAP,
                                   g_param_spec_boolean ("cornersnap",
                                                         NULL, NULL,
                                                         FALSE,
                                                         GIMP_PARAM_READWRITE |
                                                         G_PARAM_CONSTRUCT));

  g_object_class_install_property (object_class, PROP_FIXEDPIVOT,
                                   g_param_spec_boolean ("fixedpivot",
                                                         NULL, NULL,
                                                         FALSE,
                                                         GIMP_PARAM_READWRITE |
                                                         G_PARAM_CONSTRUCT));
}

static void
gimp_tool_transform_grid_init (GimpToolTransformGrid *grid)
{
  grid->private = gimp_tool_transform_grid_get_instance_private (grid);
}

static void
gimp_tool_transform_grid_constructed (GObject *object)
{
  GimpToolTransformGrid        *grid    = GIMP_TOOL_TRANSFORM_GRID (object);
  GimpToolWidget               *widget  = GIMP_TOOL_WIDGET (object);
  GimpToolTransformGridPrivate *private = grid->private;
  GimpCanvasGroup              *stroke_group;
  gint                          i;

  G_OBJECT_CLASS (parent_class)->constructed (object);

  private->guides = gimp_tool_widget_add_transform_guides (widget,
                                                           &private->transform,
                                                           private->x1,
                                                           private->y1,
                                                           private->x2,
                                                           private->y2,
                                                           private->guide_type,
                                                           private->n_guides,
                                                           private->clip_guides);

  for (i = 0; i < 4; i++)
    {
      /*  draw the scale handles  */
      private->handles[GIMP_TRANSFORM_HANDLE_NW + i] =
        gimp_tool_widget_add_handle (widget,
                                     GIMP_HANDLE_SQUARE,
                                     0, 0, 10, 10,
                                     GIMP_HANDLE_ANCHOR_CENTER);

      /*  draw the perspective handles  */
      private->handles[GIMP_TRANSFORM_HANDLE_NW_P + i] =
        gimp_tool_widget_add_handle (widget,
                                     GIMP_HANDLE_DIAMOND,
                                     0, 0, 10, 10,
                                     GIMP_HANDLE_ANCHOR_CENTER);

      /*  draw the side handles  */
      private->handles[GIMP_TRANSFORM_HANDLE_N + i] =
        gimp_tool_widget_add_handle (widget,
                                     GIMP_HANDLE_SQUARE,
                                     0, 0, 10, 10,
                                     GIMP_HANDLE_ANCHOR_CENTER);

      /*  draw the shear handles  */
      private->handles[GIMP_TRANSFORM_HANDLE_N_S + i] =
        gimp_tool_widget_add_handle (widget,
                                     GIMP_HANDLE_FILLED_DIAMOND,
                                     0, 0, 10, 10,
                                     GIMP_HANDLE_ANCHOR_CENTER);
    }

  /*  draw the rotation center axis handle  */
  stroke_group = gimp_tool_widget_add_stroke_group (widget);

  private->handles[GIMP_TRANSFORM_HANDLE_PIVOT] =
    GIMP_CANVAS_ITEM (stroke_group);

  gimp_tool_widget_push_group (widget, stroke_group);

  private->pivot_items[0] =
    gimp_tool_widget_add_handle (widget,
                                 GIMP_HANDLE_CIRCLE,
                                 0, 0, 10, 10,
                                 GIMP_HANDLE_ANCHOR_CENTER);
  private->pivot_items[1] =
    gimp_tool_widget_add_handle (widget,
                                 GIMP_HANDLE_CROSS,
                                 0, 0, 10, 10,
                                 GIMP_HANDLE_ANCHOR_CENTER);

  gimp_tool_widget_pop_group (widget);

  /*  draw the center handle  */
  stroke_group = gimp_tool_widget_add_stroke_group (widget);

  private->handles[GIMP_TRANSFORM_HANDLE_CENTER] =
    GIMP_CANVAS_ITEM (stroke_group);

  gimp_tool_widget_push_group (widget, stroke_group);

  private->center_items[0] =
    gimp_tool_widget_add_handle (widget,
                                 GIMP_HANDLE_SQUARE,
                                 0, 0, 10, 10,
                                 GIMP_HANDLE_ANCHOR_CENTER);
  private->center_items[1] =
    gimp_tool_widget_add_handle (widget,
                                 GIMP_HANDLE_CROSS,
                                 0, 0, 10, 10,
                                 GIMP_HANDLE_ANCHOR_CENTER);

  gimp_tool_widget_pop_group (widget);

  gimp_tool_transform_grid_changed (widget);
}

static void
gimp_tool_transform_grid_set_property (GObject      *object,
                                       guint         property_id,
                                       const GValue *value,
                                       GParamSpec   *pspec)
{
  GimpToolTransformGrid        *grid    = GIMP_TOOL_TRANSFORM_GRID (object);
  GimpToolTransformGridPrivate *private = grid->private;
  gboolean                      box     = FALSE;

  switch (property_id)
    {
    case PROP_TRANSFORM:
      {
        GimpMatrix3 *transform = g_value_get_boxed (value);

        if (transform)
          private->transform = *transform;
        else
          gimp_matrix3_identity (&private->transform);
      }
      break;

    case PROP_X1:
      private->x1 = g_value_get_double (value);
      box = TRUE;
      break;
    case PROP_Y1:
      private->y1 = g_value_get_double (value);
      box = TRUE;
      break;
    case PROP_X2:
      private->x2 = g_value_get_double (value);
      box = TRUE;
      break;
    case PROP_Y2:
      private->y2 = g_value_get_double (value);
      box = TRUE;
      break;

    case PROP_PIVOT_X:
      private->pivot_x = g_value_get_double (value);
      break;
    case PROP_PIVOT_Y:
      private->pivot_y = g_value_get_double (value);
      break;

    case PROP_GUIDE_TYPE:
      private->guide_type = g_value_get_enum (value);
      break;
    case PROP_N_GUIDES:
      private->n_guides = g_value_get_int (value);
      break;
    case PROP_CLIP_GUIDES:
      private->clip_guides = g_value_get_boolean (value);
      break;
    case PROP_SHOW_GUIDES:
      private->show_guides = g_value_get_boolean (value);
      break;

    case PROP_INSIDE_FUNCTION:
      private->inside_function = g_value_get_enum (value);
      break;
    case PROP_OUTSIDE_FUNCTION:
      private->outside_function = g_value_get_enum (value);
      break;

    case PROP_USE_CORNER_HANDLES:
      private->use_corner_handles = g_value_get_boolean (value);
      break;
    case PROP_USE_PERSPECTIVE_HANDLES:
      private->use_perspective_handles = g_value_get_boolean (value);
      break;
    case PROP_USE_SIDE_HANDLES:
      private->use_side_handles = g_value_get_boolean (value);
      break;
    case PROP_USE_SHEAR_HANDLES:
      private->use_shear_handles = g_value_get_boolean (value);
      break;
    case PROP_USE_CENTER_HANDLE:
      private->use_center_handle = g_value_get_boolean (value);
      break;
    case PROP_USE_PIVOT_HANDLE:
      private->use_pivot_handle = g_value_get_boolean (value);
      break;

    case PROP_DYNAMIC_HANDLE_SIZE:
      private->dynamic_handle_size = g_value_get_boolean (value);
      break;

    case PROP_CONSTRAIN_MOVE:
      private->constrain_move = g_value_get_boolean (value);
      break;
    case PROP_CONSTRAIN_SCALE:
      private->constrain_scale = g_value_get_boolean (value);
      break;
    case PROP_CONSTRAIN_ROTATE:
      private->constrain_rotate = g_value_get_boolean (value);
      break;
    case PROP_CONSTRAIN_SHEAR:
      private->constrain_shear = g_value_get_boolean (value);
      break;
    case PROP_CONSTRAIN_PERSPECTIVE:
      private->constrain_perspective = g_value_get_boolean (value);
      break;

    case PROP_FROMPIVOT_SCALE:
      private->frompivot_scale = g_value_get_boolean (value);
      break;
    case PROP_FROMPIVOT_SHEAR:
      private->frompivot_shear = g_value_get_boolean (value);
      break;
    case PROP_FROMPIVOT_PERSPECTIVE:
      private->frompivot_perspective = g_value_get_boolean (value);
      break;

    case PROP_CORNERSNAP:
      private->cornersnap = g_value_get_boolean (value);
      break;
    case PROP_FIXEDPIVOT:
      private->fixedpivot = g_value_get_boolean (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }

  if (box)
    {
      private->cx = (private->x1 + private->x2) / 2.0;
      private->cy = (private->y1 + private->y2) / 2.0;
    }
}

static void
gimp_tool_transform_grid_get_property (GObject    *object,
                                       guint       property_id,
                                       GValue     *value,
                                       GParamSpec *pspec)
{
  GimpToolTransformGrid        *grid    = GIMP_TOOL_TRANSFORM_GRID (object);
  GimpToolTransformGridPrivate *private = grid->private;

  switch (property_id)
    {
    case PROP_TRANSFORM:
      g_value_set_boxed (value, &private->transform);
      break;

    case PROP_X1:
      g_value_set_double (value, private->x1);
      break;
    case PROP_Y1:
      g_value_set_double (value, private->y1);
      break;
    case PROP_X2:
      g_value_set_double (value, private->x2);
      break;
    case PROP_Y2:
      g_value_set_double (value, private->y2);
      break;

    case PROP_PIVOT_X:
      g_value_set_double (value, private->pivot_x);
      break;
    case PROP_PIVOT_Y:
      g_value_set_double (value, private->pivot_y);
      break;

    case PROP_GUIDE_TYPE:
      g_value_set_enum (value, private->guide_type);
      break;
    case PROP_N_GUIDES:
      g_value_set_int (value, private->n_guides);
      break;
    case PROP_CLIP_GUIDES:
      g_value_set_boolean (value, private->clip_guides);
      break;
    case PROP_SHOW_GUIDES:
      g_value_set_boolean (value, private->show_guides);
      break;

    case PROP_INSIDE_FUNCTION:
      g_value_set_enum (value, private->inside_function);
      break;
    case PROP_OUTSIDE_FUNCTION:
      g_value_set_enum (value, private->outside_function);
      break;

    case PROP_USE_CORNER_HANDLES:
      g_value_set_boolean (value, private->use_corner_handles);
      break;
    case PROP_USE_PERSPECTIVE_HANDLES:
      g_value_set_boolean (value, private->use_perspective_handles);
      break;
    case PROP_USE_SIDE_HANDLES:
      g_value_set_boolean (value, private->use_side_handles);
      break;
    case PROP_USE_SHEAR_HANDLES:
      g_value_set_boolean (value, private->use_shear_handles);
      break;
    case PROP_USE_CENTER_HANDLE:
      g_value_set_boolean (value, private->use_center_handle);
      break;
    case PROP_USE_PIVOT_HANDLE:
      g_value_set_boolean (value, private->use_pivot_handle);
      break;

    case PROP_DYNAMIC_HANDLE_SIZE:
      g_value_set_boolean (value, private->dynamic_handle_size);
      break;

    case PROP_CONSTRAIN_MOVE:
      g_value_set_boolean (value, private->constrain_move);
      break;
    case PROP_CONSTRAIN_SCALE:
      g_value_set_boolean (value, private->constrain_scale);
      break;
    case PROP_CONSTRAIN_ROTATE:
      g_value_set_boolean (value, private->constrain_rotate);
      break;
    case PROP_CONSTRAIN_SHEAR:
      g_value_set_boolean (value, private->constrain_shear);
      break;
    case PROP_CONSTRAIN_PERSPECTIVE:
      g_value_set_boolean (value, private->constrain_perspective);
      break;

    case PROP_FROMPIVOT_SCALE:
      g_value_set_boolean (value, private->frompivot_scale);
      break;
    case PROP_FROMPIVOT_SHEAR:
      g_value_set_boolean (value, private->frompivot_shear);
      break;
    case PROP_FROMPIVOT_PERSPECTIVE:
      g_value_set_boolean (value, private->frompivot_perspective);
      break;

    case PROP_CORNERSNAP:
      g_value_set_boolean (value, private->cornersnap);
      break;
    case PROP_FIXEDPIVOT:
      g_value_set_boolean (value, private->fixedpivot);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static gboolean
transform_is_convex (GimpVector2 *pos)
{
  return gimp_transform_polygon_is_convex (pos[0].x, pos[0].y,
                                           pos[1].x, pos[1].y,
                                           pos[2].x, pos[2].y,
                                           pos[3].x, pos[3].y);
}

static gboolean
transform_grid_is_convex (GimpToolTransformGrid *grid)
{
  GimpToolTransformGridPrivate *private = grid->private;

  return gimp_transform_polygon_is_convex (private->tx1, private->ty1,
                                           private->tx2, private->ty2,
                                           private->tx3, private->ty3,
                                           private->tx4, private->ty4);
}

static inline gboolean
vectorisnull (GimpVector2 v)
{
  return ((v.x == 0.0) && (v.y == 0.0));
}

static inline gdouble
dotprod (GimpVector2 a,
         GimpVector2 b)
{
  return a.x * b.x + a.y * b.y;
}

static inline gdouble
norm (GimpVector2 a)
{
  return sqrt (dotprod (a, a));
}

static inline GimpVector2
vectorsubtract (GimpVector2 a,
                GimpVector2 b)
{
  GimpVector2 c;

  c.x = a.x - b.x;
  c.y = a.y - b.y;

  return c;
}

static inline GimpVector2
vectoradd (GimpVector2 a,
           GimpVector2 b)
{
  GimpVector2 c;

  c.x = a.x + b.x;
  c.y = a.y + b.y;

  return c;
}

static inline GimpVector2
scalemult (GimpVector2 a,
           gdouble     b)
{
  GimpVector2 c;

  c.x = a.x * b;
  c.y = a.y * b;

  return c;
}

static inline GimpVector2
vectorproject (GimpVector2 a,
               GimpVector2 b)
{
  return scalemult (b, dotprod (a, b) / dotprod (b, b));
}

/* finds the clockwise angle between the vectors given, 0-2π */
static inline gdouble
calcangle (GimpVector2 a,
           GimpVector2 b)
{
  gdouble angle, angle2;
  gdouble length;

  if (vectorisnull (a) || vectorisnull (b))
    return 0.0;

  length = norm (a) * norm (b);

  angle = acos (SAFE_CLAMP (dotprod (a, b) / length, -1.0, +1.0));
  angle2 = b.y;
  b.y = -b.x;
  b.x = angle2;
  angle2 = acos (SAFE_CLAMP (dotprod (a, b) / length, -1.0, +1.0));

  return ((angle2 > G_PI / 2.0) ? angle : 2.0 * G_PI - angle);
}

static inline GimpVector2
rotate2d (GimpVector2 p,
          gdouble     angle)
{
  GimpVector2 ret;

  ret.x = cos (angle) * p.x-sin (angle) * p.y;
  ret.y = sin (angle) * p.x+cos (angle) * p.y;

  return ret;
}

static inline GimpVector2
lineintersect (GimpVector2 p1, GimpVector2 p2,
               GimpVector2 q1, GimpVector2 q2)
{
  gdouble     denom, u;
  GimpVector2 p;

  denom = (q2.y - q1.y) * (p2.x - p1.x) - (q2.x - q1.x) * (p2.y - p1.y);
  if (denom == 0.0)
    {
      p.x = (p1.x + p2.x + q1.x + q2.x) / 4;
      p.y = (p1.y + p2.y + q1.y + q2.y) / 4;
    }
  else
    {
      u = (q2.x - q1.x) * (p1.y - q1.y) - (q2.y - q1.y) * (p1.x - q1.x);
      u /= denom;

      p.x = p1.x + u * (p2.x - p1.x);
      p.y = p1.y + u * (p2.y - p1.y);
    }

  return p;
}

static inline GimpVector2
get_pivot_delta (GimpToolTransformGrid *grid,
                 GimpVector2           *oldpos,
                 GimpVector2           *newpos,
                 GimpVector2            pivot)
{
  GimpToolTransformGridPrivate *private = grid->private;
  GimpMatrix3                   transform_before;
  GimpMatrix3                   transform_after;
  GimpVector2                   delta;

  gimp_matrix3_identity (&transform_before);
  gimp_matrix3_identity (&transform_after);

  gimp_transform_matrix_perspective (&transform_before,
                                     private->x1,
                                     private->y1,
                                     private->x2 - private->x1,
                                     private->y2 - private->y1,
                                     oldpos[0].x, oldpos[0].y,
                                     oldpos[1].x, oldpos[1].y,
                                     oldpos[2].x, oldpos[2].y,
                                     oldpos[3].x, oldpos[3].y);
  gimp_transform_matrix_perspective (&transform_after,
                                     private->x1,
                                     private->y1,
                                     private->x2 - private->x1,
                                     private->y2 - private->y1,
                                     newpos[0].x, newpos[0].y,
                                     newpos[1].x, newpos[1].y,
                                     newpos[2].x, newpos[2].y,
                                     newpos[3].x, newpos[3].y);
  gimp_matrix3_invert (&transform_before);
  gimp_matrix3_mult (&transform_after, &transform_before);
  gimp_matrix3_transform_point (&transform_before,
                                pivot.x, pivot.y, &delta.x, &delta.y);

  delta = vectorsubtract (delta, pivot);

  return delta;
}

static gboolean
point_is_inside_polygon (gint     n,
                         gdouble *x,
                         gdouble *y,
                         gdouble  px,
                         gdouble  py)
{
  gint     i, j;
  gboolean odd = FALSE;

  for (i = 0, j = n - 1; i < n; j = i++)
    {
      if ((y[i] < py && y[j] >= py) ||
          (y[j] < py && y[i] >= py))
        {
          if (x[i] + (py - y[i]) / (y[j] - y[i]) * (x[j] - x[i]) < px)
            odd = !odd;
        }
    }

  return odd;
}

static gboolean
point_is_inside_polygon_pos (GimpVector2 *pos,
                             GimpVector2  point)
{
  return point_is_inside_polygon (4,
                                  (gdouble[4]){ pos[0].x, pos[1].x,
                                                pos[3].x, pos[2].x },
                                  (gdouble[4]){ pos[0].y, pos[1].y,
                                                pos[3].y, pos[2].y },
                                  point.x, point.y);
}

static void
get_handle_geometry (GimpToolTransformGrid *grid,
                     GimpVector2           *position,
                     gdouble               *angle)
{
  GimpToolTransformGridPrivate *private = grid->private;

  GimpVector2 o[] = { { .x = private->tx1, .y = private->ty1 },
                      { .x = private->tx2, .y = private->ty2 },
                      { .x = private->tx3, .y = private->ty3 },
                      { .x = private->tx4, .y = private->ty4 } };
  GimpVector2 right = { .x = 1.0, .y = 0.0 };
  GimpVector2 up    = { .x = 0.0, .y = 1.0 };

  if (position)
    {
      position[0] = o[0];
      position[1] = o[1];
      position[2] = o[2];
      position[3] = o[3];
    }

  angle[0] = calcangle (vectorsubtract (o[1], o[0]), right);
  angle[1] = calcangle (vectorsubtract (o[3], o[2]), right);
  angle[2] = calcangle (vectorsubtract (o[3], o[1]), up);
  angle[3] = calcangle (vectorsubtract (o[2], o[0]), up);

  angle[4] = (angle[0] + angle[3]) / 2.0;
  angle[5] = (angle[0] + angle[2]) / 2.0;
  angle[6] = (angle[1] + angle[3]) / 2.0;
  angle[7] = (angle[1] + angle[2]) / 2.0;

  angle[8] = (angle[0] + angle[1] + angle[2] + angle[3]) / 4.0;
}

static void
gimp_tool_transform_grid_changed (GimpToolWidget *widget)
{
  GimpToolTransformGrid        *grid    = GIMP_TOOL_TRANSFORM_GRID (widget);
  GimpToolTransformGridPrivate *private = grid->private;
  gdouble                       angle[9];
  GimpVector2                   o[4], t[4];
  gint                          handle_w;
  gint                          handle_h;
  gint                          d, i;

  gimp_tool_transform_grid_update_box (grid);

  gimp_canvas_transform_guides_set (private->guides,
                                    &private->transform,
                                    private->x1,
                                    private->y1,
                                    private->x2,
                                    private->y2,
                                    private->guide_type,
                                    private->n_guides,
                                    private->clip_guides);
  gimp_canvas_item_set_visible (private->guides, private->show_guides);

  get_handle_geometry (grid, o, angle);
  gimp_tool_transform_grid_calc_handles (grid, &handle_w, &handle_h);

  for (i = 0; i < 4; i++)
    {
      GimpCanvasItem *h;
      gdouble         factor;

      /*  the scale handles  */
      factor = 1.0;
      if (private->use_perspective_handles)
        factor = 1.5;

      h = private->handles[GIMP_TRANSFORM_HANDLE_NW + i];
      gimp_canvas_item_set_visible (h, private->use_corner_handles);

      if (private->use_corner_handles)
        {
          gimp_canvas_handle_set_position (h, o[i].x, o[i].y);
          gimp_canvas_handle_set_size (h, handle_w * factor, handle_h * factor);
          gimp_canvas_handle_set_angles (h, angle[i + 4], 0.0);
        }

      /*  the perspective handles  */
      factor = 1.0;
      if (private->use_corner_handles)
        factor = 0.8;

      h = private->handles[GIMP_TRANSFORM_HANDLE_NW_P + i];
      gimp_canvas_item_set_visible (h, private->use_perspective_handles);

      if (private->use_perspective_handles)
        {
          gimp_canvas_handle_set_position (h, o[i].x, o[i].y);
          gimp_canvas_handle_set_size (h, handle_w * factor, handle_h * factor);
          gimp_canvas_handle_set_angles (h, angle[i + 4], 0.0);
        }
    }

  /*  draw the side handles  */
  t[0] = scalemult (vectoradd (o[0], o[1]), 0.5);
  t[1] = scalemult (vectoradd (o[2], o[3]), 0.5);
  t[2] = scalemult (vectoradd (o[1], o[3]), 0.5);
  t[3] = scalemult (vectoradd (o[2], o[0]), 0.5);

  for (i = 0; i < 4; i++)
    {
      GimpCanvasItem *h;

      h = private->handles[GIMP_TRANSFORM_HANDLE_N + i];
      gimp_canvas_item_set_visible (h, private->use_side_handles);

      if (private->use_side_handles)
        {
          gimp_canvas_handle_set_position (h, t[i].x, t[i].y);
          gimp_canvas_handle_set_size (h, handle_w, handle_h);
          gimp_canvas_handle_set_angles (h, angle[i], 0.0);
        }
    }

  /*  draw the shear handles  */
  t[0] = scalemult (vectoradd (           o[0]      , scalemult (o[1], 3.0)),
                    0.25);
  t[1] = scalemult (vectoradd (scalemult (o[2], 3.0),            o[3]      ),
                    0.25);
  t[2] = scalemult (vectoradd (           o[1]      , scalemult (o[3], 3.0)),
                    0.25);
  t[3] = scalemult (vectoradd (scalemult (o[0], 3.0),            o[2]      ),
                    0.25);

  for (i = 0; i < 4; i++)
    {
      GimpCanvasItem *h;

      h = private->handles[GIMP_TRANSFORM_HANDLE_N_S + i];
      gimp_canvas_item_set_visible (h, private->use_shear_handles);

      if (private->use_shear_handles)
        {
          gimp_canvas_handle_set_position (h, t[i].x, t[i].y);
          gimp_canvas_handle_set_size (h, handle_w, handle_h);
          gimp_canvas_handle_set_angles (h, angle[i], 0.0);
        }
    }

  d = MIN (handle_w, handle_h);
  if (private->use_center_handle)
    d *= 2; /* so you can grab it from under the center handle */

  gimp_canvas_item_set_visible (private->handles[GIMP_TRANSFORM_HANDLE_PIVOT],
                                private->use_pivot_handle);

  if (private->use_pivot_handle)
    {
      gimp_canvas_handle_set_position (private->pivot_items[0],
                                       private->tpx, private->tpy);
      gimp_canvas_handle_set_size (private->pivot_items[0], d, d);

      gimp_canvas_handle_set_position (private->pivot_items[1],
                                       private->tpx, private->tpy);
      gimp_canvas_handle_set_size (private->pivot_items[1], d, d);
    }

  d = MIN (handle_w, handle_h);

  gimp_canvas_item_set_visible (private->handles[GIMP_TRANSFORM_HANDLE_CENTER],
                                private->use_center_handle);

  if (private->use_center_handle)
    {
      gimp_canvas_handle_set_position (private->center_items[0],
                                       private->tcx, private->tcy);
      gimp_canvas_handle_set_size (private->center_items[0], d, d);
      gimp_canvas_handle_set_angles (private->center_items[0], angle[8], 0.0);

      gimp_canvas_handle_set_position (private->center_items[1],
                                       private->tcx, private->tcy);
      gimp_canvas_handle_set_size (private->center_items[1], d, d);
      gimp_canvas_handle_set_angles (private->center_items[1], angle[8], 0.0);
    }

  gimp_tool_transform_grid_update_hilight (grid);
}

gint
gimp_tool_transform_grid_button_press (GimpToolWidget      *widget,
                                       const GimpCoords    *coords,
                                       guint32              time,
                                       GdkModifierType      state,
                                       GimpButtonPressType  press_type)
{
  GimpToolTransformGrid        *grid    = GIMP_TOOL_TRANSFORM_GRID (widget);
  GimpToolTransformGridPrivate *private = grid->private;

  private->button_down = TRUE;
  private->mousex      = coords->x;
  private->mousey      = coords->y;

  if (private->handle != GIMP_TRANSFORM_HANDLE_NONE)
    {
      if (private->handles[private->handle])
        {
          GimpCanvasItem *handle;
          gdouble         x, y;

          switch (private->handle)
            {
            case GIMP_TRANSFORM_HANDLE_CENTER:
              handle = private->center_items[0];
              break;

            case GIMP_TRANSFORM_HANDLE_PIVOT:
              handle = private->pivot_items[0];
              break;

             default:
              handle = private->handles[private->handle];
              break;
            }

          gimp_canvas_handle_get_position (handle, &x, &y);

          gimp_tool_widget_set_snap_offsets (widget,
                                             SIGNED_ROUND (x - coords->x),
                                             SIGNED_ROUND (y - coords->y),
                                             0, 0);
        }
      else
        {
          gimp_tool_widget_set_snap_offsets (widget, 0, 0, 0, 0);
        }

      private->prev_tx1 = private->tx1;
      private->prev_ty1 = private->ty1;
      private->prev_tx2 = private->tx2;
      private->prev_ty2 = private->ty2;
      private->prev_tx3 = private->tx3;
      private->prev_ty3 = private->ty3;
      private->prev_tx4 = private->tx4;
      private->prev_ty4 = private->ty4;
      private->prev_tpx = private->tpx;
      private->prev_tpy = private->tpy;
      private->prev_tcx = private->tcx;
      private->prev_tcy = private->tcy;

      return private->handle;
    }

  gimp_tool_widget_set_snap_offsets (widget, 0, 0, 0, 0);

  return 0;
}

void
gimp_tool_transform_grid_button_release (GimpToolWidget        *widget,
                                         const GimpCoords      *coords,
                                         guint32                time,
                                         GdkModifierType        state,
                                         GimpButtonReleaseType  release_type)
{
  GimpToolTransformGrid        *grid    = GIMP_TOOL_TRANSFORM_GRID (widget);
  GimpToolTransformGridPrivate *private = grid->private;

  private->button_down = FALSE;
}

void
gimp_tool_transform_grid_motion (GimpToolWidget   *widget,
                                 const GimpCoords *coords,
                                 guint32           time,
                                 GdkModifierType   state)
{
  GimpToolTransformGrid        *grid    = GIMP_TOOL_TRANSFORM_GRID (widget);
  GimpToolTransformGridPrivate *private = grid->private;
  gdouble                      *x[4], *y[4];
  gdouble                      *newpivot_x, *newpivot_y;

  GimpVector2                  oldpos[5], newpos[4];
  GimpVector2                  cur   = { .x = coords->x,
                                         .y = coords->y };
  GimpVector2                  mouse = { .x = private->mousex,
                                         .y = private->mousey };
  GimpVector2                  d;
  GimpVector2                  pivot;

  gboolean                     fixedpivot = private->fixedpivot;
  GimpTransformHandle          handle     = private->handle;
  gint                         i;

  private->curx = coords->x;
  private->cury = coords->y;

  x[0] = &private->tx1;
  y[0] = &private->ty1;
  x[1] = &private->tx2;
  y[1] = &private->ty2;
  x[2] = &private->tx3;
  y[2] = &private->ty3;
  x[3] = &private->tx4;
  y[3] = &private->ty4;

  newpos[0].x = oldpos[0].x = private->prev_tx1;
  newpos[0].y = oldpos[0].y = private->prev_ty1;
  newpos[1].x = oldpos[1].x = private->prev_tx2;
  newpos[1].y = oldpos[1].y = private->prev_ty2;
  newpos[2].x = oldpos[2].x = private->prev_tx3;
  newpos[2].y = oldpos[2].y = private->prev_ty3;
  newpos[3].x = oldpos[3].x = private->prev_tx4;
  newpos[3].y = oldpos[3].y = private->prev_ty4;

  /* put center point in this array too */
  oldpos[4].x = private->prev_tcx;
  oldpos[4].y = private->prev_tcy;

  d = vectorsubtract (cur, mouse);

  newpivot_x = &private->tpx;
  newpivot_y = &private->tpy;

  if (private->use_pivot_handle)
    {
      pivot.x = private->prev_tpx;
      pivot.y = private->prev_tpy;
    }
  else
    {
      /* when the transform grid doesn't use a pivot handle, use the center
       * point as the pivot instead.
       */
      pivot.x = private->prev_tcx;
      pivot.y = private->prev_tcy;

      fixedpivot = TRUE;
    }

  /* move */
  if (handle == GIMP_TRANSFORM_HANDLE_CENTER)
    {
      if (private->constrain_move)
        {
          /* snap to 45 degree vectors from starting point */
          gdouble angle = 16.0 * calcangle ((GimpVector2) { 1.0, 0.0 },
                                            d) / (2.0 * G_PI);
          gdouble dist  = norm (d) / sqrt (2);

          if (angle < 1.0 || angle >= 15.0)
            d.y = 0;
          else if (angle < 3.0)
            d.y = -(d.x = dist);
          else if (angle < 5.0)
            d.x = 0;
          else if (angle < 7.0)
            d.x = d.y = -dist;
          else if (angle < 9.0)
            d.y = 0;
          else if (angle < 11.0)
            d.x = -(d.y = dist);
          else if (angle < 13.0)
            d.x = 0;
          else if (angle < 15.0)
            d.x = d.y = dist;
        }

      for (i = 0; i < 4; i++)
        newpos[i] = vectoradd (oldpos[i], d);
    }

  /* rotate */
  if (handle == GIMP_TRANSFORM_HANDLE_ROTATION)
    {
      gdouble angle = calcangle (vectorsubtract (cur, pivot),
                                 vectorsubtract (mouse, pivot));

      if (private->constrain_rotate)
        {
          /* round to 15 degree multiple */
          angle /= 2 * G_PI / 24.0;
          angle = round (angle);
          angle *= 2 * G_PI / 24.0;
        }

      for (i = 0; i < 4; i++)
        newpos[i] = vectoradd (pivot,
                               rotate2d (vectorsubtract (oldpos[i], pivot),
                                         angle));

      fixedpivot = TRUE;
    }

  /* move rotation axis */
  if (handle == GIMP_TRANSFORM_HANDLE_PIVOT)
    {
      pivot = vectoradd (pivot, d);

      if (private->cornersnap)
        {
          /* snap to corner points and center */
          gint    closest = 0;
          gdouble closest_dist = G_MAXDOUBLE, dist;

          for (i = 0; i < 5; i++)
            {
              dist = norm (vectorsubtract (pivot, oldpos[i]));
              if (dist < closest_dist)
                {
                  closest_dist = dist;
                  closest = i;
                }
            }

          if (closest_dist *
              gimp_tool_widget_get_shell (widget)->scale_x < 50)
            {
              pivot = oldpos[closest];
            }
        }

      fixedpivot = TRUE;
    }

  /* scaling via corner */
  if (handle == GIMP_TRANSFORM_HANDLE_NW ||
      handle == GIMP_TRANSFORM_HANDLE_NE ||
      handle == GIMP_TRANSFORM_HANDLE_SE ||
      handle == GIMP_TRANSFORM_HANDLE_SW)
    {
      /* Scaling through scale handles means translating one corner point,
       * with all sides at constant angles.
       */

      gint this, left, right, opposite;

      /* 0: northwest, 1: northeast, 2: southwest, 3: southeast */
      if (handle == GIMP_TRANSFORM_HANDLE_NW)
        {
          this = 0; left = 1; right = 2; opposite = 3;
        }
      else if (handle == GIMP_TRANSFORM_HANDLE_NE)
        {
          this = 1; left = 3; right = 0; opposite = 2;
        }
      else if (handle == GIMP_TRANSFORM_HANDLE_SW)
        {
          this = 2; left = 0; right = 3; opposite = 1;
        }
      else if (handle == GIMP_TRANSFORM_HANDLE_SE)
        {
          this = 3; left = 2; right = 1; opposite = 0;
        }
      else
        gimp_assert_not_reached ();

      /* when the keep aspect transformation constraint is enabled,
       * the translation shall only be along the diagonal that runs
       * trough this corner point.
       */
      if (private->constrain_scale)
        {
          /* restrict to movement along the diagonal */
          GimpVector2 diag = vectorsubtract (oldpos[this], oldpos[opposite]);

          d = vectorproject (d, diag);
        }

      /* Move the corner being interacted with */
      /*    rp---------tp
       *   /           /\ <- d, the interaction vector
       *  /           /  tp
       * op----------/
       *
       */
      newpos[this] = vectoradd (oldpos[this], d);

      /* Where the corner to the right and left would go, need these to form
       * lines to intersect with the sides */
      /*    rp----------/
       *   /\          /\
       *  /  nr       /  nt
       * op----------lp
       *              \
       *               nl
       */

      newpos[right] = vectoradd (oldpos[right], d);
      newpos[left]  = vectoradd (oldpos[left], d);

      /* Now we just need to find the intersection of op-rp and nr-nt.
       *    rp----------/
       *   /           /
       *  /  nr==========nt
       * op----------/
       *
       */
      newpos[right] = lineintersect (newpos[right], newpos[this],
                                     oldpos[opposite], oldpos[right]);
      newpos[left]  = lineintersect (newpos[left], newpos[this],
                                     oldpos[opposite], oldpos[left]);
      /*    /-----------/
       *   /           /
       *  rp============nt
       * op----------/
       *
       */

      /*
       *
       *  /--------------/
       * /--------------/
       *
       */

      if (private->frompivot_scale &&
          transform_is_convex (newpos) &&
          transform_is_convex (oldpos))
        {
          /* transform the pivot point before the interaction and
           * after, and move everything by this difference
           */
          //TODO the handle doesn't actually end up where the mouse cursor is
          GimpVector2 delta = get_pivot_delta (grid, oldpos, newpos, pivot);
          for (i = 0; i < 4; i++)
            newpos[i] = vectorsubtract (newpos[i], delta);

          fixedpivot = TRUE;
        }
    }

  /* scaling via sides */
  if (handle == GIMP_TRANSFORM_HANDLE_N ||
      handle == GIMP_TRANSFORM_HANDLE_E ||
      handle == GIMP_TRANSFORM_HANDLE_S ||
      handle == GIMP_TRANSFORM_HANDLE_W)
    {
      gint        this_l, this_r, opp_l, opp_r;
      GimpVector2 side_l, side_r, midline;

      /* 0: northwest, 1: northeast, 2: southwest, 3: southeast */
      if (handle == GIMP_TRANSFORM_HANDLE_N)
        {
          this_l = 1; this_r = 0;
        }
      else if (handle == GIMP_TRANSFORM_HANDLE_E)
        {
          this_l = 3; this_r = 1;
        }
      else if (handle == GIMP_TRANSFORM_HANDLE_S)
        {
          this_l = 2; this_r = 3;
        }
      else if (handle == GIMP_TRANSFORM_HANDLE_W)
        {
          this_l = 0; this_r = 2;
        }
      else
        gimp_assert_not_reached ();

      opp_l = 3 - this_r; opp_r = 3 - this_l;

      side_l = vectorsubtract (oldpos[opp_l], oldpos[this_l]);
      side_r = vectorsubtract (oldpos[opp_r], oldpos[this_r]);
      midline = vectoradd (side_l, side_r);

      /* restrict to movement along the midline */
      d = vectorproject (d, midline);

      if (private->constrain_scale)
        {
          GimpVector2 before, after, effective_pivot = pivot;
          gdouble     distance;

          if (! private->frompivot_scale)
            {
              /* center of the opposite side is pivot */
              effective_pivot = scalemult (vectoradd (oldpos[opp_l],
                                                      oldpos[opp_r]), 0.5);
            }

          /* get the difference between the distance from the pivot to
           * where interaction started and the distance from the pivot
           * to where cursor is now, and scale all corners distance
           * from the pivot with this factor
           */
          before = vectorsubtract (effective_pivot, mouse);
          after = vectorsubtract (effective_pivot, cur);
          after = vectorproject (after, before);

          distance = 0.5 * (after.x / before.x + after.y / before.y);

          for (i = 0; i < 4; i++)
            newpos[i] = vectoradd (effective_pivot,
                                   scalemult (vectorsubtract (oldpos[i],
                                                              effective_pivot),
                                              distance));
        }
      else
        {
          /* just move the side */
          newpos[this_l] = vectoradd (oldpos[this_l], d);
          newpos[this_r] = vectoradd (oldpos[this_r], d);
        }

      if (! private->constrain_scale   &&
          private->frompivot_scale     &&
          transform_is_convex (newpos) &&
          transform_is_convex (oldpos))
        {
          GimpVector2 delta = get_pivot_delta (grid, oldpos, newpos, pivot);
          for (i = 0; i < 4; i++)
            newpos[i] = vectorsubtract (newpos[i], delta);

          fixedpivot = TRUE;
        }
    }

  /* shear */
  if (handle == GIMP_TRANSFORM_HANDLE_N_S ||
      handle == GIMP_TRANSFORM_HANDLE_E_S ||
      handle == GIMP_TRANSFORM_HANDLE_S_S ||
      handle == GIMP_TRANSFORM_HANDLE_W_S)
    {
      gint this_l, this_r;

      /* set up indices for this edge and the opposite edge */
      if (handle == GIMP_TRANSFORM_HANDLE_N_S)
        {
          this_l = 1; this_r = 0;
        }
      else if (handle == GIMP_TRANSFORM_HANDLE_W_S)
        {
          this_l = 0; this_r = 2;
        }
      else if (handle == GIMP_TRANSFORM_HANDLE_S_S)
        {
          this_l = 2; this_r = 3;
        }
      else if (handle == GIMP_TRANSFORM_HANDLE_E_S)
        {
          this_l = 3; this_r = 1;
        }
      else
        gimp_assert_not_reached ();

      if (private->constrain_shear)
        {
          /* restrict to movement along the side */
          GimpVector2 side = vectorsubtract (oldpos[this_r], oldpos[this_l]);

          d = vectorproject (d, side);
        }

      newpos[this_l] = vectoradd (oldpos[this_l], d);
      newpos[this_r] = vectoradd (oldpos[this_r], d);

      if (private->frompivot_shear     &&
          transform_is_convex (newpos) &&
          transform_is_convex (oldpos))
        {
          GimpVector2 delta = get_pivot_delta (grid, oldpos, newpos, pivot);
          for (i = 0; i < 4; i++)
            newpos[i] = vectorsubtract (newpos[i], delta);

          fixedpivot = TRUE;
        }
    }

  /* perspective transform */
  if (handle == GIMP_TRANSFORM_HANDLE_NW_P ||
      handle == GIMP_TRANSFORM_HANDLE_NE_P ||
      handle == GIMP_TRANSFORM_HANDLE_SE_P ||
      handle == GIMP_TRANSFORM_HANDLE_SW_P)
    {
      gint this, left, right, opposite;

      /* 0: northwest, 1: northeast, 2: southwest, 3: southeast */
      if (handle == GIMP_TRANSFORM_HANDLE_NW_P)
        {
          this = 0; left = 1; right = 2; opposite = 3;
        }
      else if (handle == GIMP_TRANSFORM_HANDLE_NE_P)
        {
          this = 1; left = 3; right = 0; opposite = 2;
        }
      else if (handle == GIMP_TRANSFORM_HANDLE_SW_P)
        {
          this = 2; left = 0; right = 3; opposite = 1;
        }
      else if (handle == GIMP_TRANSFORM_HANDLE_SE_P)
        {
          this = 3; left = 2; right = 1; opposite = 0;
        }
      else
        gimp_assert_not_reached ();

      if (private->constrain_perspective)
        {
          /* when the constrain transformation constraint is enabled,
           * the translation shall only be either along the side
           * angles of the two sides that run to this corner point, or
           * along the diagonal that runs trough this corner point.
           */
          GimpVector2 proj[4];
          gdouble     rej[4];

          for (i = 0; i < 4; i++)
            {
              if (i == this)
                continue;

              /* get the vectors along the sides and the diagonal */
              proj[i] = vectorsubtract (oldpos[this], oldpos[i]);

              /* project d on each candidate vector and see which has
               * the shortest rejection
               */
              proj[i] = vectorproject (d, proj[i]);
              rej[i] = norm (vectorsubtract (d, proj[i]));
            }

          if (rej[left] < rej[right] && rej[left] < rej[opposite])
            d = proj[left];
          else if (rej[right] < rej[opposite])
            d = proj[right];
          else
            d = proj[opposite];
        }

      newpos[this] = vectoradd (oldpos[this], d);

      if (private->frompivot_perspective &&
          transform_is_convex (newpos)   &&
          transform_is_convex (oldpos))
        {
          GimpVector2 delta = get_pivot_delta (grid, oldpos, newpos, pivot);

          for (i = 0; i < 4; i++)
            newpos[i] = vectorsubtract (newpos[i], delta);

          fixedpivot = TRUE;
        }
    }

  /* this will have been set to TRUE if an operation used the pivot in
   * addition to being a user option
   */
  if (! fixedpivot                 &&
      transform_is_convex (newpos) &&
      transform_is_convex (oldpos) &&
      point_is_inside_polygon_pos (oldpos, pivot))
    {
      GimpVector2 delta = get_pivot_delta (grid, oldpos, newpos, pivot);
      pivot = vectoradd (pivot, delta);
    }

  /* make sure the new coordinates are valid */
  for (i = 0; i < 4; i++)
    {
      if (! isfinite (newpos[i].x) || ! isfinite (newpos[i].y))
        return;
    }

  if (! isfinite (pivot.x) || ! isfinite (pivot.y))
    return;

  for (i = 0; i < 4; i++)
    {
      *x[i] = newpos[i].x;
      *y[i] = newpos[i].y;
    }

  /* set unconditionally: if options get toggled during operation, we
   * have to move pivot back
   */
  *newpivot_x = pivot.x;
  *newpivot_y = pivot.y;

  gimp_tool_transform_grid_update_matrix (grid);
}

static const gchar *
get_friendly_operation_name (GimpTransformHandle handle)
{
  switch (handle)
    {
    case GIMP_TRANSFORM_HANDLE_NONE:
      return "";
    case GIMP_TRANSFORM_HANDLE_NW_P:
    case GIMP_TRANSFORM_HANDLE_NE_P:
    case GIMP_TRANSFORM_HANDLE_SW_P:
    case GIMP_TRANSFORM_HANDLE_SE_P:
      return _("Click-Drag to change perspective");
    case GIMP_TRANSFORM_HANDLE_NW:
    case GIMP_TRANSFORM_HANDLE_NE:
    case GIMP_TRANSFORM_HANDLE_SW:
    case GIMP_TRANSFORM_HANDLE_SE:
      return _("Click-Drag to scale");
    case GIMP_TRANSFORM_HANDLE_N:
    case GIMP_TRANSFORM_HANDLE_S:
    case GIMP_TRANSFORM_HANDLE_E:
    case GIMP_TRANSFORM_HANDLE_W:
      return _("Click-Drag to scale");
    case GIMP_TRANSFORM_HANDLE_CENTER:
      return _("Click-Drag to move");
    case GIMP_TRANSFORM_HANDLE_PIVOT:
      return _("Click-Drag to move the pivot point");
    case GIMP_TRANSFORM_HANDLE_N_S:
    case GIMP_TRANSFORM_HANDLE_S_S:
    case GIMP_TRANSFORM_HANDLE_E_S:
    case GIMP_TRANSFORM_HANDLE_W_S:
      return _("Click-Drag to shear");
    case GIMP_TRANSFORM_HANDLE_ROTATION:
      return _("Click-Drag to rotate");
    default:
      gimp_assert_not_reached ();
    }
}

static GimpTransformHandle
gimp_tool_transform_get_area_handle (GimpToolTransformGrid *grid,
                                     const GimpCoords      *coords,
                                     GimpTransformFunction  function)
{
  GimpToolTransformGridPrivate *private = grid->private;
  GimpTransformHandle           handle  = GIMP_TRANSFORM_HANDLE_NONE;

  switch (function)
    {
    case GIMP_TRANSFORM_FUNCTION_NONE:
      break;

    case GIMP_TRANSFORM_FUNCTION_MOVE:
      handle = GIMP_TRANSFORM_HANDLE_CENTER;
      break;

    case GIMP_TRANSFORM_FUNCTION_ROTATE:
      handle = GIMP_TRANSFORM_HANDLE_ROTATION;
      break;

    case GIMP_TRANSFORM_FUNCTION_SCALE:
    case GIMP_TRANSFORM_FUNCTION_PERSPECTIVE:
      {
        gdouble closest_dist;
        gdouble dist;

        dist = gimp_canvas_item_transform_distance_square (private->guides,
                                                           coords->x, coords->y,
                                                           private->tx1,
                                                           private->ty1);
        closest_dist = dist;
        if (function == GIMP_TRANSFORM_FUNCTION_PERSPECTIVE)
          handle = GIMP_TRANSFORM_HANDLE_NW_P;
        else
          handle = GIMP_TRANSFORM_HANDLE_NW;

        dist = gimp_canvas_item_transform_distance_square (private->guides,
                                                           coords->x, coords->y,
                                                           private->tx2,
                                                           private->ty2);
        if (dist < closest_dist)
          {
            closest_dist = dist;
            if (function == GIMP_TRANSFORM_FUNCTION_PERSPECTIVE)
              handle = GIMP_TRANSFORM_HANDLE_NE_P;
            else
              handle = GIMP_TRANSFORM_HANDLE_NE;
          }

        dist = gimp_canvas_item_transform_distance_square (private->guides,
                                                           coords->x, coords->y,
                                                           private->tx3,
                                                           private->ty3);
        if (dist < closest_dist)
          {
            closest_dist = dist;
            if (function == GIMP_TRANSFORM_FUNCTION_PERSPECTIVE)
              handle = GIMP_TRANSFORM_HANDLE_SW_P;
            else
              handle = GIMP_TRANSFORM_HANDLE_SW;
          }

        dist = gimp_canvas_item_transform_distance_square (private->guides,
                                                           coords->x, coords->y,
                                                           private->tx4,
                                                           private->ty4);
        if (dist < closest_dist)
          {
            closest_dist = dist;
            if (function == GIMP_TRANSFORM_FUNCTION_PERSPECTIVE)
              handle = GIMP_TRANSFORM_HANDLE_SE_P;
            else
              handle = GIMP_TRANSFORM_HANDLE_SE;
          }
      }
      break;

    case GIMP_TRANSFORM_FUNCTION_SHEAR:
      {
        gdouble handle_x;
        gdouble handle_y;
        gdouble closest_dist;
        gdouble dist;

        gimp_canvas_handle_get_position (private->handles[GIMP_TRANSFORM_HANDLE_N],
                                         &handle_x, &handle_y);
        dist = gimp_canvas_item_transform_distance_square (private->guides,
                                                           coords->x, coords->y,
                                                           handle_x, handle_y);
        closest_dist = dist;
        handle = GIMP_TRANSFORM_HANDLE_N_S;

        gimp_canvas_handle_get_position (private->handles[GIMP_TRANSFORM_HANDLE_W],
                                         &handle_x, &handle_y);
        dist = gimp_canvas_item_transform_distance_square (private->guides,
                                                           coords->x, coords->y,
                                                           handle_x, handle_y);
        if (dist < closest_dist)
          {
            closest_dist = dist;
            handle = GIMP_TRANSFORM_HANDLE_W_S;
          }

        gimp_canvas_handle_get_position (private->handles[GIMP_TRANSFORM_HANDLE_E],
                                         &handle_x, &handle_y);
        dist = gimp_canvas_item_transform_distance_square (private->guides,
                                                           coords->x, coords->y,
                                                           handle_x, handle_y);
        if (dist < closest_dist)
          {
            closest_dist = dist;
            handle = GIMP_TRANSFORM_HANDLE_E_S;
          }

        gimp_canvas_handle_get_position (private->handles[GIMP_TRANSFORM_HANDLE_S],
                                         &handle_x, &handle_y);
        dist = gimp_canvas_item_transform_distance_square (private->guides,
                                                           coords->x, coords->y,
                                                           handle_x, handle_y);
        if (dist < closest_dist)
          {
            closest_dist = dist;
            handle = GIMP_TRANSFORM_HANDLE_S_S;
          }
      }
      break;
    }

  return handle;
}

GimpHit
gimp_tool_transform_grid_hit (GimpToolWidget   *widget,
                              const GimpCoords *coords,
                              GdkModifierType   state,
                              gboolean          proximity)
{
  GimpToolTransformGrid *grid = GIMP_TOOL_TRANSFORM_GRID (widget);
  GimpTransformHandle    handle;

  handle = gimp_tool_transform_grid_get_handle_for_coords (grid, coords);

  if (handle != GIMP_TRANSFORM_HANDLE_NONE)
    return GIMP_HIT_DIRECT;

  return GIMP_HIT_INDIRECT;
}

void
gimp_tool_transform_grid_hover (GimpToolWidget   *widget,
                                const GimpCoords *coords,
                                GdkModifierType   state,
                                gboolean          proximity)
{
  GimpToolTransformGrid        *grid    = GIMP_TOOL_TRANSFORM_GRID (widget);
  GimpToolTransformGridPrivate *private = grid->private;
  GimpTransformHandle           handle;

  handle = gimp_tool_transform_grid_get_handle_for_coords (grid, coords);

  if (handle == GIMP_TRANSFORM_HANDLE_NONE)
    {
      /* points passed in clockwise order */
      if (point_is_inside_polygon (4,
                                   (gdouble[4]){ private->tx1, private->tx2,
                                                 private->tx4, private->tx3 },
                                   (gdouble[4]){ private->ty1, private->ty2,
                                                 private->ty4, private->ty3 },
                                   coords->x, coords->y))
        {
          handle = gimp_tool_transform_get_area_handle (grid, coords,
                                                        private->inside_function);
        }
      else
        {
          handle = gimp_tool_transform_get_area_handle (grid, coords,
                                                        private->outside_function);
        }
    }

  if (handle != GIMP_TRANSFORM_HANDLE_NONE && proximity)
    {
      gimp_tool_widget_set_status (widget,
                                   get_friendly_operation_name (handle));
    }
  else
    {
      gimp_tool_widget_set_status (widget, NULL);
    }

  private->handle = handle;

  gimp_tool_transform_grid_update_hilight (grid);
}

void
gimp_tool_transform_grid_leave_notify (GimpToolWidget *widget)
{
  GimpToolTransformGrid        *grid    = GIMP_TOOL_TRANSFORM_GRID (widget);
  GimpToolTransformGridPrivate *private = grid->private;

  private->handle = GIMP_TRANSFORM_HANDLE_NONE;

  gimp_tool_transform_grid_update_hilight (grid);

  GIMP_TOOL_WIDGET_CLASS (parent_class)->leave_notify (widget);
}

static void
gimp_tool_transform_grid_modifier (GimpToolWidget  *widget,
                                   GdkModifierType  key)
{
  GimpToolTransformGrid        *grid    = GIMP_TOOL_TRANSFORM_GRID (widget);
  GimpToolTransformGridPrivate *private = grid->private;

  if (key == gimp_get_constrain_behavior_mask ())
    {
      g_object_set (widget,
                    "frompivot-scale",       ! private->frompivot_scale,
                    "frompivot-shear",       ! private->frompivot_shear,
                    "frompivot-perspective", ! private->frompivot_perspective,
                    NULL);
    }
  else if (key == gimp_get_extend_selection_mask ())
    {
      g_object_set (widget,
                    "cornersnap",            ! private->cornersnap,
                    "constrain-move",        ! private->constrain_move,
                    "constrain-scale",       ! private->constrain_scale,
                    "constrain-rotate",      ! private->constrain_rotate,
                    "constrain-shear",       ! private->constrain_shear,
                    "constrain-perspective", ! private->constrain_perspective,
                    NULL);
    }
}

static void
gimp_tool_transform_grid_hover_modifier (GimpToolWidget  *widget,
                                         GdkModifierType  key,
                                         gboolean         press,
                                         GdkModifierType  state)
{
  GimpToolTransformGrid        *grid    = GIMP_TOOL_TRANSFORM_GRID (widget);
  GimpToolTransformGridPrivate *private = grid->private;
  GimpCoords                    coords  = { 0.0, };

  gimp_tool_transform_grid_modifier (widget, key);

  if (private->button_down)
    {
      /*  send a non-motion to update the grid with the new constraints  */
      coords.x = private->curx;
      coords.y = private->cury;
      gimp_tool_transform_grid_motion (widget, &coords, 0, state);
    }
}

static gboolean
gimp_tool_transform_grid_get_cursor (GimpToolWidget     *widget,
                                     const GimpCoords   *coords,
                                     GdkModifierType     state,
                                     GimpCursorType     *cursor,
                                     GimpToolCursorType *tool_cursor,
                                     GimpCursorModifier *modifier)
{
  GimpToolTransformGrid        *grid    = GIMP_TOOL_TRANSFORM_GRID (widget);
  GimpToolTransformGridPrivate *private = grid->private;
  gdouble                       angle[9];
  gint                          i;
  GimpCursorType                map[8];
  GimpVector2                   pos[4], this, that;
  gboolean                      flip       = FALSE;
  gboolean                      side       = FALSE;
  gboolean                      set_cursor = TRUE;

  map[0] = GIMP_CURSOR_CORNER_TOP_LEFT;
  map[1] = GIMP_CURSOR_CORNER_TOP;
  map[2] = GIMP_CURSOR_CORNER_TOP_RIGHT;
  map[3] = GIMP_CURSOR_CORNER_RIGHT;
  map[4] = GIMP_CURSOR_CORNER_BOTTOM_RIGHT;
  map[5] = GIMP_CURSOR_CORNER_BOTTOM;
  map[6] = GIMP_CURSOR_CORNER_BOTTOM_LEFT;
  map[7] = GIMP_CURSOR_CORNER_LEFT;

  get_handle_geometry (grid, pos, angle);

  for (i = 0; i < 8; i++)
    angle[i] = round (angle[i] * 180.0 / G_PI / 45.0);

  switch (private->handle)
    {
    case GIMP_TRANSFORM_HANDLE_NW_P:
    case GIMP_TRANSFORM_HANDLE_NW:
      i = (gint) angle[4] + 0;
      this = pos[0];
      that = pos[3];
      break;

    case GIMP_TRANSFORM_HANDLE_NE_P:
    case GIMP_TRANSFORM_HANDLE_NE:
      i = (gint) angle[5] + 2;
      this = pos[1];
      that = pos[2];
      break;

    case GIMP_TRANSFORM_HANDLE_SW_P:
    case GIMP_TRANSFORM_HANDLE_SW:
      i = (gint) angle[6] + 6;
      this = pos[2];
      that = pos[1];
      break;

    case GIMP_TRANSFORM_HANDLE_SE_P:
    case GIMP_TRANSFORM_HANDLE_SE:
      i = (gint) angle[7] + 4;
      this = pos[3];
      that = pos[0];
      break;

    case GIMP_TRANSFORM_HANDLE_N:
    case GIMP_TRANSFORM_HANDLE_N_S:
      i = (gint) angle[0] + 1;
      this = vectoradd (pos[0], pos[1]);
      that = vectoradd (pos[2], pos[3]);
      side = TRUE;
      break;

    case GIMP_TRANSFORM_HANDLE_S:
    case GIMP_TRANSFORM_HANDLE_S_S:
      i = (gint) angle[1] + 5;
      this = vectoradd (pos[2], pos[3]);
      that = vectoradd (pos[0], pos[1]);
      side = TRUE;
      break;

    case GIMP_TRANSFORM_HANDLE_E:
    case GIMP_TRANSFORM_HANDLE_E_S:
      i = (gint) angle[2] + 3;
      this = vectoradd (pos[1], pos[3]);
      that = vectoradd (pos[0], pos[2]);
      side = TRUE;
      break;

    case GIMP_TRANSFORM_HANDLE_W:
    case GIMP_TRANSFORM_HANDLE_W_S:
      i = (gint) angle[3] + 7;
      this = vectoradd (pos[0], pos[2]);
      that = vectoradd (pos[1], pos[3]);
      side = TRUE;
      break;

    default:
      set_cursor = FALSE;
      break;
    }

  if (set_cursor)
    {
      i %= 8;

      switch (map[i])
        {
        case GIMP_CURSOR_CORNER_TOP_LEFT:
          if (this.x + this.y > that.x + that.y)
            flip = TRUE;
          break;
        case GIMP_CURSOR_CORNER_TOP:
          if (this.y > that.y)
            flip = TRUE;
          break;
        case GIMP_CURSOR_CORNER_TOP_RIGHT:
          if (this.x - this.y < that.x - that.y)
            flip = TRUE;
          break;
        case GIMP_CURSOR_CORNER_RIGHT:
          if (this.x < that.x)
            flip = TRUE;
          break;
        case GIMP_CURSOR_CORNER_BOTTOM_RIGHT:
          if (this.x + this.y < that.x + that.y)
            flip = TRUE;
          break;
        case GIMP_CURSOR_CORNER_BOTTOM:
          if (this.y < that.y)
            flip = TRUE;
          break;
        case GIMP_CURSOR_CORNER_BOTTOM_LEFT:
          if (this.x - this.y > that.x - that.y)
            flip = TRUE;
          break;
        case GIMP_CURSOR_CORNER_LEFT:
          if (this.x > that.x)
            flip = TRUE;
          break;
        default:
          gimp_assert_not_reached ();
        }

      if (flip)
        *cursor = map[(i + 4) % 8];
      else
        *cursor = map[i];

      if (side)
        *cursor += 8;
    }

  /* parent class handles *cursor and *modifier for most handles */
  switch (private->handle)
    {
    case GIMP_TRANSFORM_HANDLE_NONE:
      *tool_cursor = GIMP_TOOL_CURSOR_NONE;
      break;

    case GIMP_TRANSFORM_HANDLE_NW_P:
    case GIMP_TRANSFORM_HANDLE_NE_P:
    case GIMP_TRANSFORM_HANDLE_SW_P:
    case GIMP_TRANSFORM_HANDLE_SE_P:
      *tool_cursor = GIMP_TOOL_CURSOR_PERSPECTIVE;
      break;

    case GIMP_TRANSFORM_HANDLE_NW:
    case GIMP_TRANSFORM_HANDLE_NE:
    case GIMP_TRANSFORM_HANDLE_SW:
    case GIMP_TRANSFORM_HANDLE_SE:
    case GIMP_TRANSFORM_HANDLE_N:
    case GIMP_TRANSFORM_HANDLE_S:
    case GIMP_TRANSFORM_HANDLE_E:
    case GIMP_TRANSFORM_HANDLE_W:
      *tool_cursor = GIMP_TOOL_CURSOR_RESIZE;
      break;

    case GIMP_TRANSFORM_HANDLE_CENTER:
      *tool_cursor = GIMP_TOOL_CURSOR_MOVE;
      break;

    case GIMP_TRANSFORM_HANDLE_PIVOT:
      *tool_cursor = GIMP_TOOL_CURSOR_ROTATE;
      *modifier    = GIMP_CURSOR_MODIFIER_MOVE;
      break;

    case GIMP_TRANSFORM_HANDLE_N_S:
    case GIMP_TRANSFORM_HANDLE_S_S:
    case GIMP_TRANSFORM_HANDLE_E_S:
    case GIMP_TRANSFORM_HANDLE_W_S:
      *tool_cursor = GIMP_TOOL_CURSOR_SHEAR;
      break;

    case GIMP_TRANSFORM_HANDLE_ROTATION:
      *tool_cursor = GIMP_TOOL_CURSOR_ROTATE;
      break;

    default:
      g_return_val_if_reached (FALSE);
    }

  return TRUE;
}

static GimpTransformHandle
gimp_tool_transform_grid_get_handle_for_coords (GimpToolTransformGrid *grid,
                                                const GimpCoords      *coords)
{
  GimpToolTransformGridPrivate *private = grid->private;
  GimpTransformHandle           i;

  for (i = GIMP_TRANSFORM_HANDLE_NONE + 1; i < GIMP_N_TRANSFORM_HANDLES; i++)
    {
      if (private->handles[i] &&
          gimp_canvas_item_hit (private->handles[i], coords->x, coords->y))
        {
          return i;
        }
    }

  return GIMP_TRANSFORM_HANDLE_NONE;
}

static void
gimp_tool_transform_grid_update_hilight (GimpToolTransformGrid *grid)
{
  GimpToolTransformGridPrivate *private = grid->private;
  GimpTransformHandle           handle;

  for (handle = GIMP_TRANSFORM_HANDLE_NONE;
       handle < GIMP_N_TRANSFORM_HANDLES;
       handle++)
    {
      if (private->handles[handle])
        {
          gimp_canvas_item_set_highlight (private->handles[handle],
                                          handle == private->handle);
        }
    }
}

static void
gimp_tool_transform_grid_update_box (GimpToolTransformGrid  *grid)
{
  GimpToolTransformGridPrivate *private = grid->private;

  gimp_matrix3_transform_point (&private->transform,
                                private->x1, private->y1,
                                &private->tx1, &private->ty1);
  gimp_matrix3_transform_point (&private->transform,
                                private->x2, private->y1,
                                &private->tx2, &private->ty2);
  gimp_matrix3_transform_point (&private->transform,
                                private->x1, private->y2,
                                &private->tx3, &private->ty3);
  gimp_matrix3_transform_point (&private->transform,
                                private->x2, private->y2,
                                &private->tx4, &private->ty4);

  /* don't transform pivot */
  private->tpx = private->pivot_x;
  private->tpy = private->pivot_y;

  if (transform_grid_is_convex (grid))
    {
      gimp_matrix3_transform_point (&private->transform,
                                    (private->x1 + private->x2) / 2.0,
                                    (private->y1 + private->y2) / 2.0,
                                    &private->tcx, &private->tcy);
    }
  else
    {
      private->tcx = (private->tx1 +
                      private->tx2 +
                      private->tx3 +
                      private->tx4) / 4.0;
      private->tcy = (private->ty1 +
                      private->ty2 +
                      private->ty3 +
                      private->ty4) / 4.0;
    }
}

static void
gimp_tool_transform_grid_update_matrix (GimpToolTransformGrid *grid)
{
  GimpToolTransformGridPrivate *private = grid->private;

  gimp_matrix3_identity (&private->transform);
  gimp_transform_matrix_perspective (&private->transform,
                                     private->x1,
                                     private->y1,
                                     private->x2 - private->x1,
                                     private->y2 - private->y1,
                                     private->tx1,
                                     private->ty1,
                                     private->tx2,
                                     private->ty2,
                                     private->tx3,
                                     private->ty3,
                                     private->tx4,
                                     private->ty4);

  private->pivot_x = private->tpx;
  private->pivot_y = private->tpy;

  g_object_freeze_notify (G_OBJECT (grid));
  g_object_notify (G_OBJECT (grid), "transform");
  g_object_notify (G_OBJECT (grid), "pivot-x");
  g_object_notify (G_OBJECT (grid), "pivot-x");
  g_object_thaw_notify (G_OBJECT (grid));
}

static void
gimp_tool_transform_grid_calc_handles (GimpToolTransformGrid *grid,
                                       gint                  *handle_w,
                                       gint                  *handle_h)
{
  GimpToolTransformGridPrivate *private = grid->private;
  gint                          dx1, dy1;
  gint                          dx2, dy2;
  gint                          dx3, dy3;
  gint                          dx4, dy4;
  gint                          x1, y1;
  gint                          x2, y2;

  if (! private->dynamic_handle_size)
    {
      *handle_w = GIMP_CANVAS_HANDLE_SIZE_LARGE;
      *handle_h = GIMP_CANVAS_HANDLE_SIZE_LARGE;

      return;
    }

  gimp_canvas_item_transform_xy (private->guides,
                                 private->tx1, private->ty1,
                                 &dx1, &dy1);
  gimp_canvas_item_transform_xy (private->guides,
                                 private->tx2, private->ty2,
                                 &dx2, &dy2);
  gimp_canvas_item_transform_xy (private->guides,
                                 private->tx3, private->ty3,
                                 &dx3, &dy3);
  gimp_canvas_item_transform_xy (private->guides,
                                 private->tx4, private->ty4,
                                 &dx4, &dy4);

  x1 = MIN4 (dx1, dx2, dx3, dx4);
  y1 = MIN4 (dy1, dy2, dy3, dy4);
  x2 = MAX4 (dx1, dx2, dx3, dx4);
  y2 = MAX4 (dy1, dy2, dy3, dy4);

  *handle_w = CLAMP ((x2 - x1) / 3,
                     MIN_HANDLE_SIZE, GIMP_CANVAS_HANDLE_SIZE_LARGE);
  *handle_h = CLAMP ((y2 - y1) / 3,
                     MIN_HANDLE_SIZE, GIMP_CANVAS_HANDLE_SIZE_LARGE);
}


/*  public functions  */

GimpToolWidget *
gimp_tool_transform_grid_new (GimpDisplayShell  *shell,
                              const GimpMatrix3 *transform,
                              gdouble            x1,
                              gdouble            y1,
                              gdouble            x2,
                              gdouble            y2)
{
  g_return_val_if_fail (GIMP_IS_DISPLAY_SHELL (shell), NULL);

  return g_object_new (GIMP_TYPE_TOOL_TRANSFORM_GRID,
                       "shell",      shell,
                       "transform",  transform,
                       "x1",         x1,
                       "y1",         y1,
                       "x2",         x2,
                       "y2",         y2,
                       NULL);
}


/*  protected functions  */

GimpTransformHandle
gimp_tool_transform_grid_get_handle (GimpToolTransformGrid *grid)
{
  g_return_val_if_fail (GIMP_IS_TOOL_TRANSFORM_GRID (grid),
                        GIMP_TRANSFORM_HANDLE_NONE);

  return grid->private->handle;
}
