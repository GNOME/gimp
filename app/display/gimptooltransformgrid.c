/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ligmatooltransformgrid.c
 * Copyright (C) 2017 Michael Natterer <mitch@ligma.org>
 *
 * Based on LigmaUnifiedTransformTool
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

#include "libligmabase/ligmabase.h"
#include "libligmamath/ligmamath.h"

#include "display-types.h"

#include "core/ligma-transform-utils.h"
#include "core/ligma-utils.h"

#include "widgets/ligmawidgets-utils.h"

#include "ligmacanvashandle.h"
#include "ligmacanvastransformguides.h"
#include "ligmadisplayshell.h"
#include "ligmatooltransformgrid.h"

#include "ligma-intl.h"


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


struct _LigmaToolTransformGridPrivate
{
  LigmaMatrix3            transform;
  gdouble                x1, y1;
  gdouble                x2, y2;
  gdouble                pivot_x;
  gdouble                pivot_y;
  LigmaGuidesType         guide_type;
  gint                   n_guides;
  gboolean               clip_guides;
  gboolean               show_guides;
  LigmaTransformFunction  inside_function;
  LigmaTransformFunction  outside_function;
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

  LigmaTransformHandle    handle;        /*  current tool activity              */

  LigmaCanvasItem        *guides;
  LigmaCanvasItem        *handles[LIGMA_N_TRANSFORM_HANDLES];
  LigmaCanvasItem        *center_items[2];
  LigmaCanvasItem        *pivot_items[2];
};


/*  local function prototypes  */

static void     ligma_tool_transform_grid_constructed    (GObject               *object);
static void     ligma_tool_transform_grid_set_property   (GObject               *object,
                                                         guint                  property_id,
                                                         const GValue          *value,
                                                         GParamSpec            *pspec);
static void     ligma_tool_transform_grid_get_property   (GObject               *object,
                                                         guint                  property_id,
                                                         GValue                *value,
                                                         GParamSpec            *pspec);

static void     ligma_tool_transform_grid_changed        (LigmaToolWidget        *widget);
static gint     ligma_tool_transform_grid_button_press   (LigmaToolWidget        *widget,
                                                         const LigmaCoords      *coords,
                                                         guint32                time,
                                                         GdkModifierType        state,
                                                         LigmaButtonPressType    press_type);
static void     ligma_tool_transform_grid_button_release (LigmaToolWidget        *widget,
                                                         const LigmaCoords      *coords,
                                                         guint32                time,
                                                         GdkModifierType        state,
                                                         LigmaButtonReleaseType  release_type);
static void     ligma_tool_transform_grid_motion         (LigmaToolWidget        *widget,
                                                         const LigmaCoords      *coords,
                                                         guint32                time,
                                                         GdkModifierType        state);
static LigmaHit  ligma_tool_transform_grid_hit            (LigmaToolWidget        *widget,
                                                         const LigmaCoords      *coords,
                                                         GdkModifierType        state,
                                                         gboolean               proximity);
static void     ligma_tool_transform_grid_hover          (LigmaToolWidget        *widget,
                                                         const LigmaCoords      *coords,
                                                         GdkModifierType        state,
                                                         gboolean               proximity);
static void     ligma_tool_transform_grid_leave_notify   (LigmaToolWidget        *widget);
static void     ligma_tool_transform_grid_hover_modifier (LigmaToolWidget        *widget,
                                                         GdkModifierType        key,
                                                         gboolean               press,
                                                         GdkModifierType        state);
static gboolean ligma_tool_transform_grid_get_cursor     (LigmaToolWidget        *widget,
                                                         const LigmaCoords      *coords,
                                                         GdkModifierType        state,
                                                         LigmaCursorType        *cursor,
                                                         LigmaToolCursorType    *tool_cursor,
                                                         LigmaCursorModifier    *modifier);

static LigmaTransformHandle
                ligma_tool_transform_grid_get_handle_for_coords
                                                        (LigmaToolTransformGrid *grid,
                                                         const LigmaCoords      *coords);
static void     ligma_tool_transform_grid_update_hilight (LigmaToolTransformGrid *grid);
static void     ligma_tool_transform_grid_update_box     (LigmaToolTransformGrid *grid);
static void     ligma_tool_transform_grid_update_matrix  (LigmaToolTransformGrid *grid);
static void     ligma_tool_transform_grid_calc_handles   (LigmaToolTransformGrid *grid,
                                                         gint                  *handle_w,
                                                         gint                  *handle_h);


G_DEFINE_TYPE_WITH_PRIVATE (LigmaToolTransformGrid, ligma_tool_transform_grid,
                            LIGMA_TYPE_TOOL_WIDGET)

#define parent_class ligma_tool_transform_grid_parent_class


static void
ligma_tool_transform_grid_class_init (LigmaToolTransformGridClass *klass)
{
  GObjectClass        *object_class = G_OBJECT_CLASS (klass);
  LigmaToolWidgetClass *widget_class = LIGMA_TOOL_WIDGET_CLASS (klass);

  object_class->constructed     = ligma_tool_transform_grid_constructed;
  object_class->set_property    = ligma_tool_transform_grid_set_property;
  object_class->get_property    = ligma_tool_transform_grid_get_property;

  widget_class->changed         = ligma_tool_transform_grid_changed;
  widget_class->button_press    = ligma_tool_transform_grid_button_press;
  widget_class->button_release  = ligma_tool_transform_grid_button_release;
  widget_class->motion          = ligma_tool_transform_grid_motion;
  widget_class->hit             = ligma_tool_transform_grid_hit;
  widget_class->hover           = ligma_tool_transform_grid_hover;
  widget_class->leave_notify    = ligma_tool_transform_grid_leave_notify;
  widget_class->hover_modifier  = ligma_tool_transform_grid_hover_modifier;
  widget_class->get_cursor      = ligma_tool_transform_grid_get_cursor;
  widget_class->update_on_scale = TRUE;

  g_object_class_install_property (object_class, PROP_TRANSFORM,
                                   ligma_param_spec_matrix3 ("transform",
                                                            NULL, NULL,
                                                            NULL,
                                                            LIGMA_PARAM_READWRITE |
                                                            G_PARAM_CONSTRUCT));

  g_object_class_install_property (object_class, PROP_X1,
                                   g_param_spec_double ("x1",
                                                        NULL, NULL,
                                                        -LIGMA_MAX_IMAGE_SIZE,
                                                        LIGMA_MAX_IMAGE_SIZE,
                                                        0.0,
                                                        LIGMA_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT));

  g_object_class_install_property (object_class, PROP_Y1,
                                   g_param_spec_double ("y1",
                                                        NULL, NULL,
                                                        -LIGMA_MAX_IMAGE_SIZE,
                                                        LIGMA_MAX_IMAGE_SIZE,
                                                        0.0,
                                                        LIGMA_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT));

  g_object_class_install_property (object_class, PROP_X2,
                                   g_param_spec_double ("x2",
                                                        NULL, NULL,
                                                        -LIGMA_MAX_IMAGE_SIZE,
                                                        LIGMA_MAX_IMAGE_SIZE,
                                                        0.0,
                                                        LIGMA_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT));

  g_object_class_install_property (object_class, PROP_Y2,
                                   g_param_spec_double ("y2",
                                                        NULL, NULL,
                                                        -LIGMA_MAX_IMAGE_SIZE,
                                                        LIGMA_MAX_IMAGE_SIZE,
                                                        0.0,
                                                        LIGMA_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT));

  g_object_class_install_property (object_class, PROP_PIVOT_X,
                                   g_param_spec_double ("pivot-x",
                                                        NULL, NULL,
                                                        -LIGMA_MAX_IMAGE_SIZE,
                                                        LIGMA_MAX_IMAGE_SIZE,
                                                        0.0,
                                                        LIGMA_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT));

  g_object_class_install_property (object_class, PROP_PIVOT_Y,
                                   g_param_spec_double ("pivot-y",
                                                        NULL, NULL,
                                                        -LIGMA_MAX_IMAGE_SIZE,
                                                        LIGMA_MAX_IMAGE_SIZE,
                                                        0.0,
                                                        LIGMA_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT));

  g_object_class_install_property (object_class, PROP_GUIDE_TYPE,
                                   g_param_spec_enum ("guide-type", NULL, NULL,
                                                      LIGMA_TYPE_GUIDES_TYPE,
                                                      LIGMA_GUIDES_NONE,
                                                      LIGMA_PARAM_READWRITE |
                                                      G_PARAM_CONSTRUCT));

  g_object_class_install_property (object_class, PROP_N_GUIDES,
                                   g_param_spec_int ("n-guides", NULL, NULL,
                                                     1, 128, 4,
                                                     LIGMA_PARAM_READWRITE |
                                                     G_PARAM_CONSTRUCT));

  g_object_class_install_property (object_class, PROP_CLIP_GUIDES,
                                   g_param_spec_boolean ("clip-guides", NULL, NULL,
                                                         FALSE,
                                                         LIGMA_PARAM_READWRITE |
                                                         G_PARAM_CONSTRUCT));

  g_object_class_install_property (object_class, PROP_SHOW_GUIDES,
                                   g_param_spec_boolean ("show-guides", NULL, NULL,
                                                         TRUE,
                                                         LIGMA_PARAM_READWRITE |
                                                         G_PARAM_CONSTRUCT));

  g_object_class_install_property (object_class, PROP_INSIDE_FUNCTION,
                                   g_param_spec_enum ("inside-function",
                                                      NULL, NULL,
                                                      LIGMA_TYPE_TRANSFORM_FUNCTION,
                                                      LIGMA_TRANSFORM_FUNCTION_MOVE,
                                                      LIGMA_PARAM_READWRITE |
                                                      G_PARAM_CONSTRUCT));

  g_object_class_install_property (object_class, PROP_OUTSIDE_FUNCTION,
                                   g_param_spec_enum ("outside-function",
                                                      NULL, NULL,
                                                      LIGMA_TYPE_TRANSFORM_FUNCTION,
                                                      LIGMA_TRANSFORM_FUNCTION_ROTATE,
                                                      LIGMA_PARAM_READWRITE |
                                                      G_PARAM_CONSTRUCT));

  g_object_class_install_property (object_class, PROP_USE_CORNER_HANDLES,
                                   g_param_spec_boolean ("use-corner-handles",
                                                         NULL, NULL,
                                                         FALSE,
                                                         LIGMA_PARAM_READWRITE |
                                                         G_PARAM_CONSTRUCT));

  g_object_class_install_property (object_class, PROP_USE_PERSPECTIVE_HANDLES,
                                   g_param_spec_boolean ("use-perspective-handles",
                                                         NULL, NULL,
                                                         FALSE,
                                                         LIGMA_PARAM_READWRITE |
                                                         G_PARAM_CONSTRUCT));

  g_object_class_install_property (object_class, PROP_USE_SIDE_HANDLES,
                                   g_param_spec_boolean ("use-side-handles",
                                                         NULL, NULL,
                                                         FALSE,
                                                         LIGMA_PARAM_READWRITE |
                                                         G_PARAM_CONSTRUCT));

  g_object_class_install_property (object_class, PROP_USE_SHEAR_HANDLES,
                                   g_param_spec_boolean ("use-shear-handles",
                                                         NULL, NULL,
                                                         FALSE,
                                                         LIGMA_PARAM_READWRITE |
                                                         G_PARAM_CONSTRUCT));

  g_object_class_install_property (object_class, PROP_USE_CENTER_HANDLE,
                                   g_param_spec_boolean ("use-center-handle",
                                                         NULL, NULL,
                                                         FALSE,
                                                         LIGMA_PARAM_READWRITE |
                                                         G_PARAM_CONSTRUCT));

  g_object_class_install_property (object_class, PROP_USE_PIVOT_HANDLE,
                                   g_param_spec_boolean ("use-pivot-handle",
                                                         NULL, NULL,
                                                         FALSE,
                                                         LIGMA_PARAM_READWRITE |
                                                         G_PARAM_CONSTRUCT));

  g_object_class_install_property (object_class, PROP_DYNAMIC_HANDLE_SIZE,
                                   g_param_spec_boolean ("dynamic-handle-size",
                                                         NULL, NULL,
                                                         TRUE,
                                                         LIGMA_PARAM_READWRITE |
                                                         G_PARAM_CONSTRUCT));

  g_object_class_install_property (object_class, PROP_CONSTRAIN_MOVE,
                                   g_param_spec_boolean ("constrain-move",
                                                         NULL, NULL,
                                                         FALSE,
                                                         LIGMA_PARAM_READWRITE |
                                                         G_PARAM_CONSTRUCT));

  g_object_class_install_property (object_class, PROP_CONSTRAIN_SCALE,
                                   g_param_spec_boolean ("constrain-scale",
                                                         NULL, NULL,
                                                         FALSE,
                                                         LIGMA_PARAM_READWRITE |
                                                         G_PARAM_CONSTRUCT));

  g_object_class_install_property (object_class, PROP_CONSTRAIN_ROTATE,
                                   g_param_spec_boolean ("constrain-rotate",
                                                         NULL, NULL,
                                                         FALSE,
                                                         LIGMA_PARAM_READWRITE |
                                                         G_PARAM_CONSTRUCT));

  g_object_class_install_property (object_class, PROP_CONSTRAIN_SHEAR,
                                   g_param_spec_boolean ("constrain-shear",
                                                         NULL, NULL,
                                                         FALSE,
                                                         LIGMA_PARAM_READWRITE |
                                                         G_PARAM_CONSTRUCT));

  g_object_class_install_property (object_class, PROP_CONSTRAIN_PERSPECTIVE,
                                   g_param_spec_boolean ("constrain-perspective",
                                                         NULL, NULL,
                                                         FALSE,
                                                         LIGMA_PARAM_READWRITE |
                                                         G_PARAM_CONSTRUCT));

  g_object_class_install_property (object_class, PROP_FROMPIVOT_SCALE,
                                   g_param_spec_boolean ("frompivot-scale",
                                                         NULL, NULL,
                                                         FALSE,
                                                         LIGMA_PARAM_READWRITE |
                                                         G_PARAM_CONSTRUCT));

  g_object_class_install_property (object_class, PROP_FROMPIVOT_SHEAR,
                                   g_param_spec_boolean ("frompivot-shear",
                                                         NULL, NULL,
                                                         FALSE,
                                                         LIGMA_PARAM_READWRITE |
                                                         G_PARAM_CONSTRUCT));

  g_object_class_install_property (object_class, PROP_FROMPIVOT_PERSPECTIVE,
                                   g_param_spec_boolean ("frompivot-perspective",
                                                         NULL, NULL,
                                                         FALSE,
                                                         LIGMA_PARAM_READWRITE |
                                                         G_PARAM_CONSTRUCT));

  g_object_class_install_property (object_class, PROP_CORNERSNAP,
                                   g_param_spec_boolean ("cornersnap",
                                                         NULL, NULL,
                                                         FALSE,
                                                         LIGMA_PARAM_READWRITE |
                                                         G_PARAM_CONSTRUCT));

  g_object_class_install_property (object_class, PROP_FIXEDPIVOT,
                                   g_param_spec_boolean ("fixedpivot",
                                                         NULL, NULL,
                                                         FALSE,
                                                         LIGMA_PARAM_READWRITE |
                                                         G_PARAM_CONSTRUCT));
}

static void
ligma_tool_transform_grid_init (LigmaToolTransformGrid *grid)
{
  grid->private = ligma_tool_transform_grid_get_instance_private (grid);
}

static void
ligma_tool_transform_grid_constructed (GObject *object)
{
  LigmaToolTransformGrid        *grid    = LIGMA_TOOL_TRANSFORM_GRID (object);
  LigmaToolWidget               *widget  = LIGMA_TOOL_WIDGET (object);
  LigmaToolTransformGridPrivate *private = grid->private;
  LigmaCanvasGroup              *stroke_group;
  gint                          i;

  G_OBJECT_CLASS (parent_class)->constructed (object);

  private->guides = ligma_tool_widget_add_transform_guides (widget,
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
      private->handles[LIGMA_TRANSFORM_HANDLE_NW + i] =
        ligma_tool_widget_add_handle (widget,
                                     LIGMA_HANDLE_SQUARE,
                                     0, 0, 10, 10,
                                     LIGMA_HANDLE_ANCHOR_CENTER);

      /*  draw the perspective handles  */
      private->handles[LIGMA_TRANSFORM_HANDLE_NW_P + i] =
        ligma_tool_widget_add_handle (widget,
                                     LIGMA_HANDLE_DIAMOND,
                                     0, 0, 10, 10,
                                     LIGMA_HANDLE_ANCHOR_CENTER);

      /*  draw the side handles  */
      private->handles[LIGMA_TRANSFORM_HANDLE_N + i] =
        ligma_tool_widget_add_handle (widget,
                                     LIGMA_HANDLE_SQUARE,
                                     0, 0, 10, 10,
                                     LIGMA_HANDLE_ANCHOR_CENTER);

      /*  draw the shear handles  */
      private->handles[LIGMA_TRANSFORM_HANDLE_N_S + i] =
        ligma_tool_widget_add_handle (widget,
                                     LIGMA_HANDLE_FILLED_DIAMOND,
                                     0, 0, 10, 10,
                                     LIGMA_HANDLE_ANCHOR_CENTER);
    }

  /*  draw the rotation center axis handle  */
  stroke_group = ligma_tool_widget_add_stroke_group (widget);

  private->handles[LIGMA_TRANSFORM_HANDLE_PIVOT] =
    LIGMA_CANVAS_ITEM (stroke_group);

  ligma_tool_widget_push_group (widget, stroke_group);

  private->pivot_items[0] =
    ligma_tool_widget_add_handle (widget,
                                 LIGMA_HANDLE_CIRCLE,
                                 0, 0, 10, 10,
                                 LIGMA_HANDLE_ANCHOR_CENTER);
  private->pivot_items[1] =
    ligma_tool_widget_add_handle (widget,
                                 LIGMA_HANDLE_CROSS,
                                 0, 0, 10, 10,
                                 LIGMA_HANDLE_ANCHOR_CENTER);

  ligma_tool_widget_pop_group (widget);

  /*  draw the center handle  */
  stroke_group = ligma_tool_widget_add_stroke_group (widget);

  private->handles[LIGMA_TRANSFORM_HANDLE_CENTER] =
    LIGMA_CANVAS_ITEM (stroke_group);

  ligma_tool_widget_push_group (widget, stroke_group);

  private->center_items[0] =
    ligma_tool_widget_add_handle (widget,
                                 LIGMA_HANDLE_SQUARE,
                                 0, 0, 10, 10,
                                 LIGMA_HANDLE_ANCHOR_CENTER);
  private->center_items[1] =
    ligma_tool_widget_add_handle (widget,
                                 LIGMA_HANDLE_CROSS,
                                 0, 0, 10, 10,
                                 LIGMA_HANDLE_ANCHOR_CENTER);

  ligma_tool_widget_pop_group (widget);

  ligma_tool_transform_grid_changed (widget);
}

static void
ligma_tool_transform_grid_set_property (GObject      *object,
                                       guint         property_id,
                                       const GValue *value,
                                       GParamSpec   *pspec)
{
  LigmaToolTransformGrid        *grid    = LIGMA_TOOL_TRANSFORM_GRID (object);
  LigmaToolTransformGridPrivate *private = grid->private;
  gboolean                      box     = FALSE;

  switch (property_id)
    {
    case PROP_TRANSFORM:
      {
        LigmaMatrix3 *transform = g_value_get_boxed (value);

        if (transform)
          private->transform = *transform;
        else
          ligma_matrix3_identity (&private->transform);
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
ligma_tool_transform_grid_get_property (GObject    *object,
                                       guint       property_id,
                                       GValue     *value,
                                       GParamSpec *pspec)
{
  LigmaToolTransformGrid        *grid    = LIGMA_TOOL_TRANSFORM_GRID (object);
  LigmaToolTransformGridPrivate *private = grid->private;

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
transform_is_convex (LigmaVector2 *pos)
{
  return ligma_transform_polygon_is_convex (pos[0].x, pos[0].y,
                                           pos[1].x, pos[1].y,
                                           pos[2].x, pos[2].y,
                                           pos[3].x, pos[3].y);
}

static gboolean
transform_grid_is_convex (LigmaToolTransformGrid *grid)
{
  LigmaToolTransformGridPrivate *private = grid->private;

  return ligma_transform_polygon_is_convex (private->tx1, private->ty1,
                                           private->tx2, private->ty2,
                                           private->tx3, private->ty3,
                                           private->tx4, private->ty4);
}

static inline gboolean
vectorisnull (LigmaVector2 v)
{
  return ((v.x == 0.0) && (v.y == 0.0));
}

static inline gdouble
dotprod (LigmaVector2 a,
         LigmaVector2 b)
{
  return a.x * b.x + a.y * b.y;
}

static inline gdouble
norm (LigmaVector2 a)
{
  return sqrt (dotprod (a, a));
}

static inline LigmaVector2
vectorsubtract (LigmaVector2 a,
                LigmaVector2 b)
{
  LigmaVector2 c;

  c.x = a.x - b.x;
  c.y = a.y - b.y;

  return c;
}

static inline LigmaVector2
vectoradd (LigmaVector2 a,
           LigmaVector2 b)
{
  LigmaVector2 c;

  c.x = a.x + b.x;
  c.y = a.y + b.y;

  return c;
}

static inline LigmaVector2
scalemult (LigmaVector2 a,
           gdouble     b)
{
  LigmaVector2 c;

  c.x = a.x * b;
  c.y = a.y * b;

  return c;
}

static inline LigmaVector2
vectorproject (LigmaVector2 a,
               LigmaVector2 b)
{
  return scalemult (b, dotprod (a, b) / dotprod (b, b));
}

/* finds the clockwise angle between the vectors given, 0-2π */
static inline gdouble
calcangle (LigmaVector2 a,
           LigmaVector2 b)
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

static inline LigmaVector2
rotate2d (LigmaVector2 p,
          gdouble     angle)
{
  LigmaVector2 ret;

  ret.x = cos (angle) * p.x-sin (angle) * p.y;
  ret.y = sin (angle) * p.x+cos (angle) * p.y;

  return ret;
}

static inline LigmaVector2
lineintersect (LigmaVector2 p1, LigmaVector2 p2,
               LigmaVector2 q1, LigmaVector2 q2)
{
  gdouble     denom, u;
  LigmaVector2 p;

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

static inline LigmaVector2
get_pivot_delta (LigmaToolTransformGrid *grid,
                 LigmaVector2           *oldpos,
                 LigmaVector2           *newpos,
                 LigmaVector2            pivot)
{
  LigmaToolTransformGridPrivate *private = grid->private;
  LigmaMatrix3                   transform_before;
  LigmaMatrix3                   transform_after;
  LigmaVector2                   delta;

  ligma_matrix3_identity (&transform_before);
  ligma_matrix3_identity (&transform_after);

  ligma_transform_matrix_perspective (&transform_before,
                                     private->x1,
                                     private->y1,
                                     private->x2 - private->x1,
                                     private->y2 - private->y1,
                                     oldpos[0].x, oldpos[0].y,
                                     oldpos[1].x, oldpos[1].y,
                                     oldpos[2].x, oldpos[2].y,
                                     oldpos[3].x, oldpos[3].y);
  ligma_transform_matrix_perspective (&transform_after,
                                     private->x1,
                                     private->y1,
                                     private->x2 - private->x1,
                                     private->y2 - private->y1,
                                     newpos[0].x, newpos[0].y,
                                     newpos[1].x, newpos[1].y,
                                     newpos[2].x, newpos[2].y,
                                     newpos[3].x, newpos[3].y);
  ligma_matrix3_invert (&transform_before);
  ligma_matrix3_mult (&transform_after, &transform_before);
  ligma_matrix3_transform_point (&transform_before,
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
point_is_inside_polygon_pos (LigmaVector2 *pos,
                             LigmaVector2  point)
{
  return point_is_inside_polygon (4,
                                  (gdouble[4]){ pos[0].x, pos[1].x,
                                                pos[3].x, pos[2].x },
                                  (gdouble[4]){ pos[0].y, pos[1].y,
                                                pos[3].y, pos[2].y },
                                  point.x, point.y);
}

static void
get_handle_geometry (LigmaToolTransformGrid *grid,
                     LigmaVector2           *position,
                     gdouble               *angle)
{
  LigmaToolTransformGridPrivate *private = grid->private;

  LigmaVector2 o[] = { { .x = private->tx1, .y = private->ty1 },
                      { .x = private->tx2, .y = private->ty2 },
                      { .x = private->tx3, .y = private->ty3 },
                      { .x = private->tx4, .y = private->ty4 } };
  LigmaVector2 right = { .x = 1.0, .y = 0.0 };
  LigmaVector2 up    = { .x = 0.0, .y = 1.0 };

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
ligma_tool_transform_grid_changed (LigmaToolWidget *widget)
{
  LigmaToolTransformGrid        *grid    = LIGMA_TOOL_TRANSFORM_GRID (widget);
  LigmaToolTransformGridPrivate *private = grid->private;
  gdouble                       angle[9];
  LigmaVector2                   o[4], t[4];
  gint                          handle_w;
  gint                          handle_h;
  gint                          d, i;

  ligma_tool_transform_grid_update_box (grid);

  ligma_canvas_transform_guides_set (private->guides,
                                    &private->transform,
                                    private->x1,
                                    private->y1,
                                    private->x2,
                                    private->y2,
                                    private->guide_type,
                                    private->n_guides,
                                    private->clip_guides);
  ligma_canvas_item_set_visible (private->guides, private->show_guides);

  get_handle_geometry (grid, o, angle);
  ligma_tool_transform_grid_calc_handles (grid, &handle_w, &handle_h);

  for (i = 0; i < 4; i++)
    {
      LigmaCanvasItem *h;
      gdouble         factor;

      /*  the scale handles  */
      factor = 1.0;
      if (private->use_perspective_handles)
        factor = 1.5;

      h = private->handles[LIGMA_TRANSFORM_HANDLE_NW + i];
      ligma_canvas_item_set_visible (h, private->use_corner_handles);

      if (private->use_corner_handles)
        {
          ligma_canvas_handle_set_position (h, o[i].x, o[i].y);
          ligma_canvas_handle_set_size (h, handle_w * factor, handle_h * factor);
          ligma_canvas_handle_set_angles (h, angle[i + 4], 0.0);
        }

      /*  the perspective handles  */
      factor = 1.0;
      if (private->use_corner_handles)
        factor = 0.8;

      h = private->handles[LIGMA_TRANSFORM_HANDLE_NW_P + i];
      ligma_canvas_item_set_visible (h, private->use_perspective_handles);

      if (private->use_perspective_handles)
        {
          ligma_canvas_handle_set_position (h, o[i].x, o[i].y);
          ligma_canvas_handle_set_size (h, handle_w * factor, handle_h * factor);
          ligma_canvas_handle_set_angles (h, angle[i + 4], 0.0);
        }
    }

  /*  draw the side handles  */
  t[0] = scalemult (vectoradd (o[0], o[1]), 0.5);
  t[1] = scalemult (vectoradd (o[2], o[3]), 0.5);
  t[2] = scalemult (vectoradd (o[1], o[3]), 0.5);
  t[3] = scalemult (vectoradd (o[2], o[0]), 0.5);

  for (i = 0; i < 4; i++)
    {
      LigmaCanvasItem *h;

      h = private->handles[LIGMA_TRANSFORM_HANDLE_N + i];
      ligma_canvas_item_set_visible (h, private->use_side_handles);

      if (private->use_side_handles)
        {
          ligma_canvas_handle_set_position (h, t[i].x, t[i].y);
          ligma_canvas_handle_set_size (h, handle_w, handle_h);
          ligma_canvas_handle_set_angles (h, angle[i], 0.0);
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
      LigmaCanvasItem *h;

      h = private->handles[LIGMA_TRANSFORM_HANDLE_N_S + i];
      ligma_canvas_item_set_visible (h, private->use_shear_handles);

      if (private->use_shear_handles)
        {
          ligma_canvas_handle_set_position (h, t[i].x, t[i].y);
          ligma_canvas_handle_set_size (h, handle_w, handle_h);
          ligma_canvas_handle_set_angles (h, angle[i], 0.0);
        }
    }

  d = MIN (handle_w, handle_h);
  if (private->use_center_handle)
    d *= 2; /* so you can grab it from under the center handle */

  ligma_canvas_item_set_visible (private->handles[LIGMA_TRANSFORM_HANDLE_PIVOT],
                                private->use_pivot_handle);

  if (private->use_pivot_handle)
    {
      ligma_canvas_handle_set_position (private->pivot_items[0],
                                       private->tpx, private->tpy);
      ligma_canvas_handle_set_size (private->pivot_items[0], d, d);

      ligma_canvas_handle_set_position (private->pivot_items[1],
                                       private->tpx, private->tpy);
      ligma_canvas_handle_set_size (private->pivot_items[1], d, d);
    }

  d = MIN (handle_w, handle_h);

  ligma_canvas_item_set_visible (private->handles[LIGMA_TRANSFORM_HANDLE_CENTER],
                                private->use_center_handle);

  if (private->use_center_handle)
    {
      ligma_canvas_handle_set_position (private->center_items[0],
                                       private->tcx, private->tcy);
      ligma_canvas_handle_set_size (private->center_items[0], d, d);
      ligma_canvas_handle_set_angles (private->center_items[0], angle[8], 0.0);

      ligma_canvas_handle_set_position (private->center_items[1],
                                       private->tcx, private->tcy);
      ligma_canvas_handle_set_size (private->center_items[1], d, d);
      ligma_canvas_handle_set_angles (private->center_items[1], angle[8], 0.0);
    }

  ligma_tool_transform_grid_update_hilight (grid);
}

gint
ligma_tool_transform_grid_button_press (LigmaToolWidget      *widget,
                                       const LigmaCoords    *coords,
                                       guint32              time,
                                       GdkModifierType      state,
                                       LigmaButtonPressType  press_type)
{
  LigmaToolTransformGrid        *grid    = LIGMA_TOOL_TRANSFORM_GRID (widget);
  LigmaToolTransformGridPrivate *private = grid->private;

  private->button_down = TRUE;
  private->mousex      = coords->x;
  private->mousey      = coords->y;

  if (private->handle != LIGMA_TRANSFORM_HANDLE_NONE)
    {
      if (private->handles[private->handle])
        {
          LigmaCanvasItem *handle;
          gdouble         x, y;

          switch (private->handle)
            {
            case LIGMA_TRANSFORM_HANDLE_CENTER:
              handle = private->center_items[0];
              break;

            case LIGMA_TRANSFORM_HANDLE_PIVOT:
              handle = private->pivot_items[0];
              break;

             default:
              handle = private->handles[private->handle];
              break;
            }

          ligma_canvas_handle_get_position (handle, &x, &y);

          ligma_tool_widget_set_snap_offsets (widget,
                                             SIGNED_ROUND (x - coords->x),
                                             SIGNED_ROUND (y - coords->y),
                                             0, 0);
        }
      else
        {
          ligma_tool_widget_set_snap_offsets (widget, 0, 0, 0, 0);
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

  ligma_tool_widget_set_snap_offsets (widget, 0, 0, 0, 0);

  return 0;
}

void
ligma_tool_transform_grid_button_release (LigmaToolWidget        *widget,
                                         const LigmaCoords      *coords,
                                         guint32                time,
                                         GdkModifierType        state,
                                         LigmaButtonReleaseType  release_type)
{
  LigmaToolTransformGrid        *grid    = LIGMA_TOOL_TRANSFORM_GRID (widget);
  LigmaToolTransformGridPrivate *private = grid->private;

  private->button_down = FALSE;
}

void
ligma_tool_transform_grid_motion (LigmaToolWidget   *widget,
                                 const LigmaCoords *coords,
                                 guint32           time,
                                 GdkModifierType   state)
{
  LigmaToolTransformGrid        *grid    = LIGMA_TOOL_TRANSFORM_GRID (widget);
  LigmaToolTransformGridPrivate *private = grid->private;
  gdouble                      *x[4], *y[4];
  gdouble                      *newpivot_x, *newpivot_y;

  LigmaVector2                  oldpos[5], newpos[4];
  LigmaVector2                  cur   = { .x = coords->x,
                                         .y = coords->y };
  LigmaVector2                  mouse = { .x = private->mousex,
                                         .y = private->mousey };
  LigmaVector2                  d;
  LigmaVector2                  pivot;

  gboolean                     fixedpivot = private->fixedpivot;
  LigmaTransformHandle          handle     = private->handle;
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
  if (handle == LIGMA_TRANSFORM_HANDLE_CENTER)
    {
      if (private->constrain_move)
        {
          /* snap to 45 degree vectors from starting point */
          gdouble angle = 16.0 * calcangle ((LigmaVector2) { 1.0, 0.0 },
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
  if (handle == LIGMA_TRANSFORM_HANDLE_ROTATION)
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
  if (handle == LIGMA_TRANSFORM_HANDLE_PIVOT)
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
              ligma_tool_widget_get_shell (widget)->scale_x < 50)
            {
              pivot = oldpos[closest];
            }
        }

      fixedpivot = TRUE;
    }

  /* scaling via corner */
  if (handle == LIGMA_TRANSFORM_HANDLE_NW ||
      handle == LIGMA_TRANSFORM_HANDLE_NE ||
      handle == LIGMA_TRANSFORM_HANDLE_SE ||
      handle == LIGMA_TRANSFORM_HANDLE_SW)
    {
      /* Scaling through scale handles means translating one corner point,
       * with all sides at constant angles.
       */

      gint this, left, right, opposite;

      /* 0: northwest, 1: northeast, 2: southwest, 3: southeast */
      if (handle == LIGMA_TRANSFORM_HANDLE_NW)
        {
          this = 0; left = 1; right = 2; opposite = 3;
        }
      else if (handle == LIGMA_TRANSFORM_HANDLE_NE)
        {
          this = 1; left = 3; right = 0; opposite = 2;
        }
      else if (handle == LIGMA_TRANSFORM_HANDLE_SW)
        {
          this = 2; left = 0; right = 3; opposite = 1;
        }
      else if (handle == LIGMA_TRANSFORM_HANDLE_SE)
        {
          this = 3; left = 2; right = 1; opposite = 0;
        }
      else
        ligma_assert_not_reached ();

      /* when the keep aspect transformation constraint is enabled,
       * the translation shall only be along the diagonal that runs
       * through this corner point.
       */
      if (private->constrain_scale)
        {
          /* restrict to movement along the diagonal */
          LigmaVector2 diag = vectorsubtract (oldpos[this], oldpos[opposite]);

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
          LigmaVector2 delta = get_pivot_delta (grid, oldpos, newpos, pivot);
          for (i = 0; i < 4; i++)
            newpos[i] = vectorsubtract (newpos[i], delta);

          fixedpivot = TRUE;
        }
    }

  /* scaling via sides */
  if (handle == LIGMA_TRANSFORM_HANDLE_N ||
      handle == LIGMA_TRANSFORM_HANDLE_E ||
      handle == LIGMA_TRANSFORM_HANDLE_S ||
      handle == LIGMA_TRANSFORM_HANDLE_W)
    {
      gint        this_l, this_r, opp_l, opp_r;
      LigmaVector2 side_l, side_r, midline;

      /* 0: northwest, 1: northeast, 2: southwest, 3: southeast */
      if (handle == LIGMA_TRANSFORM_HANDLE_N)
        {
          this_l = 1; this_r = 0;
        }
      else if (handle == LIGMA_TRANSFORM_HANDLE_E)
        {
          this_l = 3; this_r = 1;
        }
      else if (handle == LIGMA_TRANSFORM_HANDLE_S)
        {
          this_l = 2; this_r = 3;
        }
      else if (handle == LIGMA_TRANSFORM_HANDLE_W)
        {
          this_l = 0; this_r = 2;
        }
      else
        ligma_assert_not_reached ();

      opp_l = 3 - this_r; opp_r = 3 - this_l;

      side_l = vectorsubtract (oldpos[opp_l], oldpos[this_l]);
      side_r = vectorsubtract (oldpos[opp_r], oldpos[this_r]);
      midline = vectoradd (side_l, side_r);

      /* restrict to movement along the midline */
      d = vectorproject (d, midline);

      if (private->constrain_scale)
        {
          LigmaVector2 before, after, effective_pivot = pivot;
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
          LigmaVector2 delta = get_pivot_delta (grid, oldpos, newpos, pivot);
          for (i = 0; i < 4; i++)
            newpos[i] = vectorsubtract (newpos[i], delta);

          fixedpivot = TRUE;
        }
    }

  /* shear */
  if (handle == LIGMA_TRANSFORM_HANDLE_N_S ||
      handle == LIGMA_TRANSFORM_HANDLE_E_S ||
      handle == LIGMA_TRANSFORM_HANDLE_S_S ||
      handle == LIGMA_TRANSFORM_HANDLE_W_S)
    {
      gint this_l, this_r;

      /* set up indices for this edge and the opposite edge */
      if (handle == LIGMA_TRANSFORM_HANDLE_N_S)
        {
          this_l = 1; this_r = 0;
        }
      else if (handle == LIGMA_TRANSFORM_HANDLE_W_S)
        {
          this_l = 0; this_r = 2;
        }
      else if (handle == LIGMA_TRANSFORM_HANDLE_S_S)
        {
          this_l = 2; this_r = 3;
        }
      else if (handle == LIGMA_TRANSFORM_HANDLE_E_S)
        {
          this_l = 3; this_r = 1;
        }
      else
        ligma_assert_not_reached ();

      if (private->constrain_shear)
        {
          /* restrict to movement along the side */
          LigmaVector2 side = vectorsubtract (oldpos[this_r], oldpos[this_l]);

          d = vectorproject (d, side);
        }

      newpos[this_l] = vectoradd (oldpos[this_l], d);
      newpos[this_r] = vectoradd (oldpos[this_r], d);

      if (private->frompivot_shear     &&
          transform_is_convex (newpos) &&
          transform_is_convex (oldpos))
        {
          LigmaVector2 delta = get_pivot_delta (grid, oldpos, newpos, pivot);
          for (i = 0; i < 4; i++)
            newpos[i] = vectorsubtract (newpos[i], delta);

          fixedpivot = TRUE;
        }
    }

  /* perspective transform */
  if (handle == LIGMA_TRANSFORM_HANDLE_NW_P ||
      handle == LIGMA_TRANSFORM_HANDLE_NE_P ||
      handle == LIGMA_TRANSFORM_HANDLE_SE_P ||
      handle == LIGMA_TRANSFORM_HANDLE_SW_P)
    {
      gint this, left, right, opposite;

      /* 0: northwest, 1: northeast, 2: southwest, 3: southeast */
      if (handle == LIGMA_TRANSFORM_HANDLE_NW_P)
        {
          this = 0; left = 1; right = 2; opposite = 3;
        }
      else if (handle == LIGMA_TRANSFORM_HANDLE_NE_P)
        {
          this = 1; left = 3; right = 0; opposite = 2;
        }
      else if (handle == LIGMA_TRANSFORM_HANDLE_SW_P)
        {
          this = 2; left = 0; right = 3; opposite = 1;
        }
      else if (handle == LIGMA_TRANSFORM_HANDLE_SE_P)
        {
          this = 3; left = 2; right = 1; opposite = 0;
        }
      else
        ligma_assert_not_reached ();

      if (private->constrain_perspective)
        {
          /* when the constrain transformation constraint is enabled,
           * the translation shall only be either along the side
           * angles of the two sides that run to this corner point, or
           * along the diagonal that runs through this corner point.
           */
          LigmaVector2 proj[4];
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
          LigmaVector2 delta = get_pivot_delta (grid, oldpos, newpos, pivot);

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
      LigmaVector2 delta = get_pivot_delta (grid, oldpos, newpos, pivot);
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

  ligma_tool_transform_grid_update_matrix (grid);
}

static const gchar *
get_friendly_operation_name (LigmaTransformHandle handle)
{
  switch (handle)
    {
    case LIGMA_TRANSFORM_HANDLE_NONE:
      return "";
    case LIGMA_TRANSFORM_HANDLE_NW_P:
    case LIGMA_TRANSFORM_HANDLE_NE_P:
    case LIGMA_TRANSFORM_HANDLE_SW_P:
    case LIGMA_TRANSFORM_HANDLE_SE_P:
      return _("Click-Drag to change perspective");
    case LIGMA_TRANSFORM_HANDLE_NW:
    case LIGMA_TRANSFORM_HANDLE_NE:
    case LIGMA_TRANSFORM_HANDLE_SW:
    case LIGMA_TRANSFORM_HANDLE_SE:
      return _("Click-Drag to scale");
    case LIGMA_TRANSFORM_HANDLE_N:
    case LIGMA_TRANSFORM_HANDLE_S:
    case LIGMA_TRANSFORM_HANDLE_E:
    case LIGMA_TRANSFORM_HANDLE_W:
      return _("Click-Drag to scale");
    case LIGMA_TRANSFORM_HANDLE_CENTER:
      return _("Click-Drag to move");
    case LIGMA_TRANSFORM_HANDLE_PIVOT:
      return _("Click-Drag to move the pivot point");
    case LIGMA_TRANSFORM_HANDLE_N_S:
    case LIGMA_TRANSFORM_HANDLE_S_S:
    case LIGMA_TRANSFORM_HANDLE_E_S:
    case LIGMA_TRANSFORM_HANDLE_W_S:
      return _("Click-Drag to shear");
    case LIGMA_TRANSFORM_HANDLE_ROTATION:
      return _("Click-Drag to rotate");
    default:
      ligma_assert_not_reached ();
    }
}

static LigmaTransformHandle
ligma_tool_transform_get_area_handle (LigmaToolTransformGrid *grid,
                                     const LigmaCoords      *coords,
                                     LigmaTransformFunction  function)
{
  LigmaToolTransformGridPrivate *private = grid->private;
  LigmaTransformHandle           handle  = LIGMA_TRANSFORM_HANDLE_NONE;

  switch (function)
    {
    case LIGMA_TRANSFORM_FUNCTION_NONE:
      break;

    case LIGMA_TRANSFORM_FUNCTION_MOVE:
      handle = LIGMA_TRANSFORM_HANDLE_CENTER;
      break;

    case LIGMA_TRANSFORM_FUNCTION_ROTATE:
      handle = LIGMA_TRANSFORM_HANDLE_ROTATION;
      break;

    case LIGMA_TRANSFORM_FUNCTION_SCALE:
    case LIGMA_TRANSFORM_FUNCTION_PERSPECTIVE:
      {
        gdouble closest_dist;
        gdouble dist;

        dist = ligma_canvas_item_transform_distance_square (private->guides,
                                                           coords->x, coords->y,
                                                           private->tx1,
                                                           private->ty1);
        closest_dist = dist;
        if (function == LIGMA_TRANSFORM_FUNCTION_PERSPECTIVE)
          handle = LIGMA_TRANSFORM_HANDLE_NW_P;
        else
          handle = LIGMA_TRANSFORM_HANDLE_NW;

        dist = ligma_canvas_item_transform_distance_square (private->guides,
                                                           coords->x, coords->y,
                                                           private->tx2,
                                                           private->ty2);
        if (dist < closest_dist)
          {
            closest_dist = dist;
            if (function == LIGMA_TRANSFORM_FUNCTION_PERSPECTIVE)
              handle = LIGMA_TRANSFORM_HANDLE_NE_P;
            else
              handle = LIGMA_TRANSFORM_HANDLE_NE;
          }

        dist = ligma_canvas_item_transform_distance_square (private->guides,
                                                           coords->x, coords->y,
                                                           private->tx3,
                                                           private->ty3);
        if (dist < closest_dist)
          {
            closest_dist = dist;
            if (function == LIGMA_TRANSFORM_FUNCTION_PERSPECTIVE)
              handle = LIGMA_TRANSFORM_HANDLE_SW_P;
            else
              handle = LIGMA_TRANSFORM_HANDLE_SW;
          }

        dist = ligma_canvas_item_transform_distance_square (private->guides,
                                                           coords->x, coords->y,
                                                           private->tx4,
                                                           private->ty4);
        if (dist < closest_dist)
          {
            closest_dist = dist;
            if (function == LIGMA_TRANSFORM_FUNCTION_PERSPECTIVE)
              handle = LIGMA_TRANSFORM_HANDLE_SE_P;
            else
              handle = LIGMA_TRANSFORM_HANDLE_SE;
          }
      }
      break;

    case LIGMA_TRANSFORM_FUNCTION_SHEAR:
      {
        gdouble handle_x;
        gdouble handle_y;
        gdouble closest_dist;
        gdouble dist;

        ligma_canvas_handle_get_position (private->handles[LIGMA_TRANSFORM_HANDLE_N],
                                         &handle_x, &handle_y);
        dist = ligma_canvas_item_transform_distance_square (private->guides,
                                                           coords->x, coords->y,
                                                           handle_x, handle_y);
        closest_dist = dist;
        handle = LIGMA_TRANSFORM_HANDLE_N_S;

        ligma_canvas_handle_get_position (private->handles[LIGMA_TRANSFORM_HANDLE_W],
                                         &handle_x, &handle_y);
        dist = ligma_canvas_item_transform_distance_square (private->guides,
                                                           coords->x, coords->y,
                                                           handle_x, handle_y);
        if (dist < closest_dist)
          {
            closest_dist = dist;
            handle = LIGMA_TRANSFORM_HANDLE_W_S;
          }

        ligma_canvas_handle_get_position (private->handles[LIGMA_TRANSFORM_HANDLE_E],
                                         &handle_x, &handle_y);
        dist = ligma_canvas_item_transform_distance_square (private->guides,
                                                           coords->x, coords->y,
                                                           handle_x, handle_y);
        if (dist < closest_dist)
          {
            closest_dist = dist;
            handle = LIGMA_TRANSFORM_HANDLE_E_S;
          }

        ligma_canvas_handle_get_position (private->handles[LIGMA_TRANSFORM_HANDLE_S],
                                         &handle_x, &handle_y);
        dist = ligma_canvas_item_transform_distance_square (private->guides,
                                                           coords->x, coords->y,
                                                           handle_x, handle_y);
        if (dist < closest_dist)
          {
            closest_dist = dist;
            handle = LIGMA_TRANSFORM_HANDLE_S_S;
          }
      }
      break;
    }

  return handle;
}

LigmaHit
ligma_tool_transform_grid_hit (LigmaToolWidget   *widget,
                              const LigmaCoords *coords,
                              GdkModifierType   state,
                              gboolean          proximity)
{
  LigmaToolTransformGrid *grid = LIGMA_TOOL_TRANSFORM_GRID (widget);
  LigmaTransformHandle    handle;

  handle = ligma_tool_transform_grid_get_handle_for_coords (grid, coords);

  if (handle != LIGMA_TRANSFORM_HANDLE_NONE)
    return LIGMA_HIT_DIRECT;

  return LIGMA_HIT_INDIRECT;
}

void
ligma_tool_transform_grid_hover (LigmaToolWidget   *widget,
                                const LigmaCoords *coords,
                                GdkModifierType   state,
                                gboolean          proximity)
{
  LigmaToolTransformGrid        *grid    = LIGMA_TOOL_TRANSFORM_GRID (widget);
  LigmaToolTransformGridPrivate *private = grid->private;
  LigmaTransformHandle           handle;

  handle = ligma_tool_transform_grid_get_handle_for_coords (grid, coords);

  if (handle == LIGMA_TRANSFORM_HANDLE_NONE)
    {
      /* points passed in clockwise order */
      if (point_is_inside_polygon (4,
                                   (gdouble[4]){ private->tx1, private->tx2,
                                                 private->tx4, private->tx3 },
                                   (gdouble[4]){ private->ty1, private->ty2,
                                                 private->ty4, private->ty3 },
                                   coords->x, coords->y))
        {
          handle = ligma_tool_transform_get_area_handle (grid, coords,
                                                        private->inside_function);
        }
      else
        {
          handle = ligma_tool_transform_get_area_handle (grid, coords,
                                                        private->outside_function);
        }
    }

  if (handle != LIGMA_TRANSFORM_HANDLE_NONE && proximity)
    {
      ligma_tool_widget_set_status (widget,
                                   get_friendly_operation_name (handle));
    }
  else
    {
      ligma_tool_widget_set_status (widget, NULL);
    }

  private->handle = handle;

  ligma_tool_transform_grid_update_hilight (grid);
}

void
ligma_tool_transform_grid_leave_notify (LigmaToolWidget *widget)
{
  LigmaToolTransformGrid        *grid    = LIGMA_TOOL_TRANSFORM_GRID (widget);
  LigmaToolTransformGridPrivate *private = grid->private;

  private->handle = LIGMA_TRANSFORM_HANDLE_NONE;

  ligma_tool_transform_grid_update_hilight (grid);

  LIGMA_TOOL_WIDGET_CLASS (parent_class)->leave_notify (widget);
}

static void
ligma_tool_transform_grid_modifier (LigmaToolWidget  *widget,
                                   GdkModifierType  key)
{
  LigmaToolTransformGrid        *grid    = LIGMA_TOOL_TRANSFORM_GRID (widget);
  LigmaToolTransformGridPrivate *private = grid->private;

  if (key == ligma_get_constrain_behavior_mask ())
    {
      g_object_set (widget,
                    "frompivot-scale",       ! private->frompivot_scale,
                    "frompivot-shear",       ! private->frompivot_shear,
                    "frompivot-perspective", ! private->frompivot_perspective,
                    NULL);
    }
  else if (key == ligma_get_extend_selection_mask ())
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
ligma_tool_transform_grid_hover_modifier (LigmaToolWidget  *widget,
                                         GdkModifierType  key,
                                         gboolean         press,
                                         GdkModifierType  state)
{
  LigmaToolTransformGrid        *grid    = LIGMA_TOOL_TRANSFORM_GRID (widget);
  LigmaToolTransformGridPrivate *private = grid->private;
  LigmaCoords                    coords  = { 0.0, };

  ligma_tool_transform_grid_modifier (widget, key);

  if (private->button_down)
    {
      /*  send a non-motion to update the grid with the new constraints  */
      coords.x = private->curx;
      coords.y = private->cury;
      ligma_tool_transform_grid_motion (widget, &coords, 0, state);
    }
}

static gboolean
ligma_tool_transform_grid_get_cursor (LigmaToolWidget     *widget,
                                     const LigmaCoords   *coords,
                                     GdkModifierType     state,
                                     LigmaCursorType     *cursor,
                                     LigmaToolCursorType *tool_cursor,
                                     LigmaCursorModifier *modifier)
{
  LigmaToolTransformGrid        *grid    = LIGMA_TOOL_TRANSFORM_GRID (widget);
  LigmaToolTransformGridPrivate *private = grid->private;
  gdouble                       angle[9];
  gint                          i;
  LigmaCursorType                map[8];
  LigmaVector2                   pos[4], this, that;
  gboolean                      flip       = FALSE;
  gboolean                      side       = FALSE;
  gboolean                      set_cursor = TRUE;

  map[0] = LIGMA_CURSOR_CORNER_TOP_LEFT;
  map[1] = LIGMA_CURSOR_CORNER_TOP;
  map[2] = LIGMA_CURSOR_CORNER_TOP_RIGHT;
  map[3] = LIGMA_CURSOR_CORNER_RIGHT;
  map[4] = LIGMA_CURSOR_CORNER_BOTTOM_RIGHT;
  map[5] = LIGMA_CURSOR_CORNER_BOTTOM;
  map[6] = LIGMA_CURSOR_CORNER_BOTTOM_LEFT;
  map[7] = LIGMA_CURSOR_CORNER_LEFT;

  get_handle_geometry (grid, pos, angle);

  for (i = 0; i < 8; i++)
    angle[i] = round (angle[i] * 180.0 / G_PI / 45.0);

  switch (private->handle)
    {
    case LIGMA_TRANSFORM_HANDLE_NW_P:
    case LIGMA_TRANSFORM_HANDLE_NW:
      i = (gint) angle[4] + 0;
      this = pos[0];
      that = pos[3];
      break;

    case LIGMA_TRANSFORM_HANDLE_NE_P:
    case LIGMA_TRANSFORM_HANDLE_NE:
      i = (gint) angle[5] + 2;
      this = pos[1];
      that = pos[2];
      break;

    case LIGMA_TRANSFORM_HANDLE_SW_P:
    case LIGMA_TRANSFORM_HANDLE_SW:
      i = (gint) angle[6] + 6;
      this = pos[2];
      that = pos[1];
      break;

    case LIGMA_TRANSFORM_HANDLE_SE_P:
    case LIGMA_TRANSFORM_HANDLE_SE:
      i = (gint) angle[7] + 4;
      this = pos[3];
      that = pos[0];
      break;

    case LIGMA_TRANSFORM_HANDLE_N:
    case LIGMA_TRANSFORM_HANDLE_N_S:
      i = (gint) angle[0] + 1;
      this = vectoradd (pos[0], pos[1]);
      that = vectoradd (pos[2], pos[3]);
      side = TRUE;
      break;

    case LIGMA_TRANSFORM_HANDLE_S:
    case LIGMA_TRANSFORM_HANDLE_S_S:
      i = (gint) angle[1] + 5;
      this = vectoradd (pos[2], pos[3]);
      that = vectoradd (pos[0], pos[1]);
      side = TRUE;
      break;

    case LIGMA_TRANSFORM_HANDLE_E:
    case LIGMA_TRANSFORM_HANDLE_E_S:
      i = (gint) angle[2] + 3;
      this = vectoradd (pos[1], pos[3]);
      that = vectoradd (pos[0], pos[2]);
      side = TRUE;
      break;

    case LIGMA_TRANSFORM_HANDLE_W:
    case LIGMA_TRANSFORM_HANDLE_W_S:
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
        case LIGMA_CURSOR_CORNER_TOP_LEFT:
          if (this.x + this.y > that.x + that.y)
            flip = TRUE;
          break;
        case LIGMA_CURSOR_CORNER_TOP:
          if (this.y > that.y)
            flip = TRUE;
          break;
        case LIGMA_CURSOR_CORNER_TOP_RIGHT:
          if (this.x - this.y < that.x - that.y)
            flip = TRUE;
          break;
        case LIGMA_CURSOR_CORNER_RIGHT:
          if (this.x < that.x)
            flip = TRUE;
          break;
        case LIGMA_CURSOR_CORNER_BOTTOM_RIGHT:
          if (this.x + this.y < that.x + that.y)
            flip = TRUE;
          break;
        case LIGMA_CURSOR_CORNER_BOTTOM:
          if (this.y < that.y)
            flip = TRUE;
          break;
        case LIGMA_CURSOR_CORNER_BOTTOM_LEFT:
          if (this.x - this.y > that.x - that.y)
            flip = TRUE;
          break;
        case LIGMA_CURSOR_CORNER_LEFT:
          if (this.x > that.x)
            flip = TRUE;
          break;
        default:
          ligma_assert_not_reached ();
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
    case LIGMA_TRANSFORM_HANDLE_NONE:
      *tool_cursor = LIGMA_TOOL_CURSOR_NONE;
      break;

    case LIGMA_TRANSFORM_HANDLE_NW_P:
    case LIGMA_TRANSFORM_HANDLE_NE_P:
    case LIGMA_TRANSFORM_HANDLE_SW_P:
    case LIGMA_TRANSFORM_HANDLE_SE_P:
      *tool_cursor = LIGMA_TOOL_CURSOR_PERSPECTIVE;
      break;

    case LIGMA_TRANSFORM_HANDLE_NW:
    case LIGMA_TRANSFORM_HANDLE_NE:
    case LIGMA_TRANSFORM_HANDLE_SW:
    case LIGMA_TRANSFORM_HANDLE_SE:
    case LIGMA_TRANSFORM_HANDLE_N:
    case LIGMA_TRANSFORM_HANDLE_S:
    case LIGMA_TRANSFORM_HANDLE_E:
    case LIGMA_TRANSFORM_HANDLE_W:
      *tool_cursor = LIGMA_TOOL_CURSOR_RESIZE;
      break;

    case LIGMA_TRANSFORM_HANDLE_CENTER:
      *tool_cursor = LIGMA_TOOL_CURSOR_MOVE;
      break;

    case LIGMA_TRANSFORM_HANDLE_PIVOT:
      *tool_cursor = LIGMA_TOOL_CURSOR_ROTATE;
      *modifier    = LIGMA_CURSOR_MODIFIER_MOVE;
      break;

    case LIGMA_TRANSFORM_HANDLE_N_S:
    case LIGMA_TRANSFORM_HANDLE_S_S:
    case LIGMA_TRANSFORM_HANDLE_E_S:
    case LIGMA_TRANSFORM_HANDLE_W_S:
      *tool_cursor = LIGMA_TOOL_CURSOR_SHEAR;
      break;

    case LIGMA_TRANSFORM_HANDLE_ROTATION:
      *tool_cursor = LIGMA_TOOL_CURSOR_ROTATE;
      break;

    default:
      g_return_val_if_reached (FALSE);
    }

  return TRUE;
}

static LigmaTransformHandle
ligma_tool_transform_grid_get_handle_for_coords (LigmaToolTransformGrid *grid,
                                                const LigmaCoords      *coords)
{
  LigmaToolTransformGridPrivate *private = grid->private;
  LigmaTransformHandle           i;

  for (i = LIGMA_TRANSFORM_HANDLE_NONE + 1; i < LIGMA_N_TRANSFORM_HANDLES; i++)
    {
      if (private->handles[i] &&
          ligma_canvas_item_hit (private->handles[i], coords->x, coords->y))
        {
          return i;
        }
    }

  return LIGMA_TRANSFORM_HANDLE_NONE;
}

static void
ligma_tool_transform_grid_update_hilight (LigmaToolTransformGrid *grid)
{
  LigmaToolTransformGridPrivate *private = grid->private;
  LigmaTransformHandle           handle;

  for (handle = LIGMA_TRANSFORM_HANDLE_NONE;
       handle < LIGMA_N_TRANSFORM_HANDLES;
       handle++)
    {
      if (private->handles[handle])
        {
          ligma_canvas_item_set_highlight (private->handles[handle],
                                          handle == private->handle);
        }
    }
}

static void
ligma_tool_transform_grid_update_box (LigmaToolTransformGrid  *grid)
{
  LigmaToolTransformGridPrivate *private = grid->private;

  ligma_matrix3_transform_point (&private->transform,
                                private->x1, private->y1,
                                &private->tx1, &private->ty1);
  ligma_matrix3_transform_point (&private->transform,
                                private->x2, private->y1,
                                &private->tx2, &private->ty2);
  ligma_matrix3_transform_point (&private->transform,
                                private->x1, private->y2,
                                &private->tx3, &private->ty3);
  ligma_matrix3_transform_point (&private->transform,
                                private->x2, private->y2,
                                &private->tx4, &private->ty4);

  /* don't transform pivot */
  private->tpx = private->pivot_x;
  private->tpy = private->pivot_y;

  if (transform_grid_is_convex (grid))
    {
      ligma_matrix3_transform_point (&private->transform,
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
ligma_tool_transform_grid_update_matrix (LigmaToolTransformGrid *grid)
{
  LigmaToolTransformGridPrivate *private = grid->private;

  ligma_matrix3_identity (&private->transform);
  ligma_transform_matrix_perspective (&private->transform,
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
ligma_tool_transform_grid_calc_handles (LigmaToolTransformGrid *grid,
                                       gint                  *handle_w,
                                       gint                  *handle_h)
{
  LigmaToolTransformGridPrivate *private = grid->private;
  gint                          dx1, dy1;
  gint                          dx2, dy2;
  gint                          dx3, dy3;
  gint                          dx4, dy4;
  gint                          x1, y1;
  gint                          x2, y2;

  if (! private->dynamic_handle_size)
    {
      *handle_w = LIGMA_CANVAS_HANDLE_SIZE_LARGE;
      *handle_h = LIGMA_CANVAS_HANDLE_SIZE_LARGE;

      return;
    }

  ligma_canvas_item_transform_xy (private->guides,
                                 private->tx1, private->ty1,
                                 &dx1, &dy1);
  ligma_canvas_item_transform_xy (private->guides,
                                 private->tx2, private->ty2,
                                 &dx2, &dy2);
  ligma_canvas_item_transform_xy (private->guides,
                                 private->tx3, private->ty3,
                                 &dx3, &dy3);
  ligma_canvas_item_transform_xy (private->guides,
                                 private->tx4, private->ty4,
                                 &dx4, &dy4);

  x1 = MIN4 (dx1, dx2, dx3, dx4);
  y1 = MIN4 (dy1, dy2, dy3, dy4);
  x2 = MAX4 (dx1, dx2, dx3, dx4);
  y2 = MAX4 (dy1, dy2, dy3, dy4);

  *handle_w = CLAMP ((x2 - x1) / 3,
                     MIN_HANDLE_SIZE, LIGMA_CANVAS_HANDLE_SIZE_LARGE);
  *handle_h = CLAMP ((y2 - y1) / 3,
                     MIN_HANDLE_SIZE, LIGMA_CANVAS_HANDLE_SIZE_LARGE);
}


/*  public functions  */

LigmaToolWidget *
ligma_tool_transform_grid_new (LigmaDisplayShell  *shell,
                              const LigmaMatrix3 *transform,
                              gdouble            x1,
                              gdouble            y1,
                              gdouble            x2,
                              gdouble            y2)
{
  g_return_val_if_fail (LIGMA_IS_DISPLAY_SHELL (shell), NULL);

  return g_object_new (LIGMA_TYPE_TOOL_TRANSFORM_GRID,
                       "shell",      shell,
                       "transform",  transform,
                       "x1",         x1,
                       "y1",         y1,
                       "x2",         x2,
                       "y2",         y2,
                       NULL);
}


/*  protected functions  */

LigmaTransformHandle
ligma_tool_transform_grid_get_handle (LigmaToolTransformGrid *grid)
{
  g_return_val_if_fail (LIGMA_IS_TOOL_TRANSFORM_GRID (grid),
                        LIGMA_TRANSFORM_HANDLE_NONE);

  return grid->private->handle;
}
