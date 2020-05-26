/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimptoolcompass.c
 * Copyright (C) 2017 Michael Natterer <mitch@gimp.org>
 *
 * Measure tool
 * Copyright (C) 1999-2003 Sven Neumann <sven@gimp.org>
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

#include "core/gimp-utils.h"
#include "core/gimpimage.h"
#include "core/gimpmarshal.h"

#include "widgets/gimpwidgets-utils.h"

#include "gimpcanvashandle.h"
#include "gimpcanvasline.h"
#include "gimpdisplay.h"
#include "gimpdisplayshell.h"
#include "gimpdisplayshell-appearance.h"
#include "gimpdisplayshell-transform.h"
#include "gimpdisplayshell-utils.h"
#include "gimptoolcompass.h"

#include "gimp-intl.h"


#define ARC_RADIUS 30
#define ARC_GAP    (ARC_RADIUS / 2)
#define EPSILON    1e-6


/*  possible measure functions  */
typedef enum
{
  CREATING,
  ADDING,
  MOVING,
  MOVING_ALL,
  GUIDING,
  FINISHED
} CompassFunction;

enum
{
  PROP_0,
  PROP_ORIENTATION,
  PROP_N_POINTS,
  PROP_X1,
  PROP_Y1,
  PROP_X2,
  PROP_Y2,
  PROP_X3,
  PROP_Y3,
  PROP_PIXEL_ANGLE,
  PROP_UNIT_ANGLE,
  PROP_EFFECTIVE_ORIENTATION
};

enum
{
  CREATE_GUIDES,
  LAST_SIGNAL
};

struct _GimpToolCompassPrivate
{
  GimpCompassOrientation  orientation;
  gint                    n_points;
  gint                    x[3];
  gint                    y[3];

  GimpVector2             radius1;
  GimpVector2             radius2;
  gdouble                 display_angle;
  gdouble                 pixel_angle;
  gdouble                 unit_angle;
  GimpCompassOrientation  effective_orientation;

  CompassFunction         function;
  gdouble                 mouse_x;
  gdouble                 mouse_y;
  gint                    last_x;
  gint                    last_y;
  gint                    point;

  GimpCanvasItem         *line1;
  GimpCanvasItem         *line2;
  GimpCanvasItem         *arc;
  GimpCanvasItem         *arc_line;
  GimpCanvasItem         *handles[3];
};


/*  local function prototypes  */

static void     gimp_tool_compass_constructed     (GObject                *object);
static void     gimp_tool_compass_set_property    (GObject                *object,
                                                   guint                   property_id,
                                                   const GValue           *value,
                                                   GParamSpec             *pspec);
static void     gimp_tool_compass_get_property    (GObject                *object,
                                                   guint                   property_id,
                                                   GValue                 *value,
                                                   GParamSpec             *pspec);

static void     gimp_tool_compass_changed         (GimpToolWidget         *widget);
static gint     gimp_tool_compass_button_press    (GimpToolWidget         *widget,
                                                   const GimpCoords       *coords,
                                                   guint32                 time,
                                                   GdkModifierType         state,
                                                   GimpButtonPressType     press_type);
static void     gimp_tool_compass_button_release  (GimpToolWidget         *widget,
                                                   const GimpCoords       *coords,
                                                   guint32                 time,
                                                   GdkModifierType         state,
                                                   GimpButtonReleaseType   release_type);
static void     gimp_tool_compass_motion          (GimpToolWidget         *widget,
                                                   const GimpCoords       *coords,
                                                   guint32                 time,
                                                   GdkModifierType         state);
static GimpHit  gimp_tool_compass_hit             (GimpToolWidget         *widget,
                                                   const GimpCoords       *coords,
                                                   GdkModifierType         state,
                                                   gboolean                proximity);
static void     gimp_tool_compass_hover           (GimpToolWidget         *widget,
                                                   const GimpCoords       *coords,
                                                   GdkModifierType         state,
                                                   gboolean                proximity);
static void     gimp_tool_compass_leave_notify    (GimpToolWidget         *widget);
static void     gimp_tool_compass_motion_modifier (GimpToolWidget         *widget,
                                                   GdkModifierType         key,
                                                   gboolean                press,
                                                   GdkModifierType         state);
static gboolean gimp_tool_compass_get_cursor      (GimpToolWidget         *widget,
                                                   const GimpCoords       *coords,
                                                   GdkModifierType         state,
                                                   GimpCursorType         *cursor,
                                                   GimpToolCursorType     *tool_cursor,
                                                   GimpCursorModifier     *modifier);

static gint     gimp_tool_compass_get_point       (GimpToolCompass        *compass,
                                                   const GimpCoords       *coords);
static void     gimp_tool_compass_update_hilight  (GimpToolCompass        *compass);
static void     gimp_tool_compass_update_angle    (GimpToolCompass        *compass,
                                                   GimpCompassOrientation  orientation,
                                                   gboolean                flip);


G_DEFINE_TYPE_WITH_PRIVATE (GimpToolCompass, gimp_tool_compass,
                            GIMP_TYPE_TOOL_WIDGET)

#define parent_class gimp_tool_compass_parent_class

static guint compass_signals[LAST_SIGNAL] = { 0 };


static void
gimp_tool_compass_class_init (GimpToolCompassClass *klass)
{
  GObjectClass        *object_class = G_OBJECT_CLASS (klass);
  GimpToolWidgetClass *widget_class = GIMP_TOOL_WIDGET_CLASS (klass);

  object_class->constructed      = gimp_tool_compass_constructed;
  object_class->set_property     = gimp_tool_compass_set_property;
  object_class->get_property     = gimp_tool_compass_get_property;

  widget_class->changed          = gimp_tool_compass_changed;
  widget_class->button_press     = gimp_tool_compass_button_press;
  widget_class->button_release   = gimp_tool_compass_button_release;
  widget_class->motion           = gimp_tool_compass_motion;
  widget_class->hit              = gimp_tool_compass_hit;
  widget_class->hover            = gimp_tool_compass_hover;
  widget_class->leave_notify     = gimp_tool_compass_leave_notify;
  widget_class->motion_modifier  = gimp_tool_compass_motion_modifier;
  widget_class->get_cursor       = gimp_tool_compass_get_cursor;
  widget_class->update_on_scale  = TRUE;
  widget_class->update_on_rotate = TRUE;

  compass_signals[CREATE_GUIDES] =
    g_signal_new ("create-guides",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (GimpToolCompassClass, create_guides),
                  NULL, NULL,
                  gimp_marshal_VOID__INT_INT_BOOLEAN_BOOLEAN,
                  G_TYPE_NONE, 4,
                  G_TYPE_INT,
                  G_TYPE_INT,
                  G_TYPE_BOOLEAN,
                  G_TYPE_BOOLEAN);

  g_object_class_install_property (object_class, PROP_ORIENTATION,
                                   g_param_spec_enum ("orientation", NULL, NULL,
                                                      GIMP_TYPE_COMPASS_ORIENTATION,
                                                      GIMP_COMPASS_ORIENTATION_AUTO,
                                                      GIMP_PARAM_READWRITE |
                                                      G_PARAM_CONSTRUCT));

  g_object_class_install_property (object_class, PROP_N_POINTS,
                                   g_param_spec_int ("n-points", NULL, NULL,
                                                     1, 3, 1,
                                                     GIMP_PARAM_READWRITE |
                                                     G_PARAM_CONSTRUCT));

  g_object_class_install_property (object_class, PROP_X1,
                                   g_param_spec_int ("x1", NULL, NULL,
                                                     -GIMP_MAX_IMAGE_SIZE,
                                                     GIMP_MAX_IMAGE_SIZE, 0,
                                                     GIMP_PARAM_READWRITE |
                                                     G_PARAM_CONSTRUCT));

  g_object_class_install_property (object_class, PROP_Y1,
                                   g_param_spec_int ("y1", NULL, NULL,
                                                     -GIMP_MAX_IMAGE_SIZE,
                                                     GIMP_MAX_IMAGE_SIZE, 0,
                                                     GIMP_PARAM_READWRITE |
                                                     G_PARAM_CONSTRUCT));

  g_object_class_install_property (object_class, PROP_X2,
                                   g_param_spec_int ("x2", NULL, NULL,
                                                     -GIMP_MAX_IMAGE_SIZE,
                                                     GIMP_MAX_IMAGE_SIZE, 0,
                                                     GIMP_PARAM_READWRITE |
                                                     G_PARAM_CONSTRUCT));

  g_object_class_install_property (object_class, PROP_Y2,
                                   g_param_spec_int ("y2", NULL, NULL,
                                                     -GIMP_MAX_IMAGE_SIZE,
                                                     GIMP_MAX_IMAGE_SIZE, 0,
                                                     GIMP_PARAM_READWRITE |
                                                     G_PARAM_CONSTRUCT));

  g_object_class_install_property (object_class, PROP_X3,
                                   g_param_spec_int ("x3", NULL, NULL,
                                                     -GIMP_MAX_IMAGE_SIZE,
                                                     GIMP_MAX_IMAGE_SIZE, 0,
                                                     GIMP_PARAM_READWRITE |
                                                     G_PARAM_CONSTRUCT));

  g_object_class_install_property (object_class, PROP_Y3,
                                   g_param_spec_int ("y3", NULL, NULL,
                                                     -GIMP_MAX_IMAGE_SIZE,
                                                     GIMP_MAX_IMAGE_SIZE, 0,
                                                     GIMP_PARAM_READWRITE |
                                                     G_PARAM_CONSTRUCT));

  g_object_class_install_property (object_class, PROP_PIXEL_ANGLE,
                                   g_param_spec_double ("pixel-angle", NULL, NULL,
                                                        -G_PI, G_PI, 0.0,
                                                        GIMP_PARAM_READABLE));

  g_object_class_install_property (object_class, PROP_UNIT_ANGLE,
                                   g_param_spec_double ("unit-angle", NULL, NULL,
                                                        -G_PI, G_PI, 0.0,
                                                        GIMP_PARAM_READABLE));

  g_object_class_install_property (object_class, PROP_EFFECTIVE_ORIENTATION,
                                   g_param_spec_enum ("effective-orientation", NULL, NULL,
                                                      GIMP_TYPE_COMPASS_ORIENTATION,
                                                      GIMP_COMPASS_ORIENTATION_AUTO,
                                                      GIMP_PARAM_READABLE));
}

static void
gimp_tool_compass_init (GimpToolCompass *compass)
{
  compass->private = gimp_tool_compass_get_instance_private (compass);

  compass->private->point = -1;
}

static void
gimp_tool_compass_constructed (GObject *object)
{
  GimpToolCompass        *compass = GIMP_TOOL_COMPASS (object);
  GimpToolWidget         *widget  = GIMP_TOOL_WIDGET (object);
  GimpToolCompassPrivate *private = compass->private;
  GimpCanvasGroup        *stroke_group;
  gint                    i;

  G_OBJECT_CLASS (parent_class)->constructed (object);

  stroke_group = gimp_tool_widget_add_stroke_group (widget);

  gimp_tool_widget_push_group (widget, stroke_group);

  private->line1 = gimp_tool_widget_add_line (widget,
                                              private->x[0],
                                              private->y[0],
                                              private->x[1],
                                              private->y[1]);

  private->line2 = gimp_tool_widget_add_line (widget,
                                              private->x[0],
                                              private->y[0],
                                              private->x[2],
                                              private->y[2]);

  private->arc = gimp_tool_widget_add_handle (widget,
                                              GIMP_HANDLE_CIRCLE,
                                              private->x[0],
                                              private->y[0],
                                              ARC_RADIUS * 2 + 1,
                                              ARC_RADIUS * 2 + 1,
                                              GIMP_HANDLE_ANCHOR_CENTER);

  private->arc_line = gimp_tool_widget_add_line (widget,
                                                 private->x[0],
                                                 private->y[0],
                                                 private->x[0] + 10,
                                                 private->y[0]);

  gimp_tool_widget_pop_group (widget);

  for (i = 0; i < 3; i++)
    {
      private->handles[i] =
        gimp_tool_widget_add_handle (widget,
                                     i == 0 ?
                                     GIMP_HANDLE_CIRCLE : GIMP_HANDLE_CROSS,
                                     private->x[i],
                                     private->y[i],
                                     GIMP_CANVAS_HANDLE_SIZE_CROSS,
                                     GIMP_CANVAS_HANDLE_SIZE_CROSS,
                                     GIMP_HANDLE_ANCHOR_CENTER);
    }

  gimp_tool_compass_changed (widget);
}

static void
gimp_tool_compass_set_property (GObject      *object,
                                guint         property_id,
                                const GValue *value,
                                GParamSpec   *pspec)
{
  GimpToolCompass        *compass = GIMP_TOOL_COMPASS (object);
  GimpToolCompassPrivate *private = compass->private;

  switch (property_id)
    {
    case PROP_ORIENTATION:
      private->orientation = g_value_get_enum (value);
      break;
    case PROP_N_POINTS:
      private->n_points = g_value_get_int (value);
      break;
    case PROP_X1:
      private->x[0] = g_value_get_int (value);
      break;
    case PROP_Y1:
      private->y[0] = g_value_get_int (value);
      break;
    case PROP_X2:
      private->x[1] = g_value_get_int (value);
      break;
    case PROP_Y2:
      private->y[1] = g_value_get_int (value);
      break;
    case PROP_X3:
      private->x[2] = g_value_get_int (value);
      break;
    case PROP_Y3:
      private->y[2] = g_value_get_int (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_tool_compass_get_property (GObject    *object,
                                guint       property_id,
                                GValue     *value,
                                GParamSpec *pspec)
{
  GimpToolCompass        *compass = GIMP_TOOL_COMPASS (object);
  GimpToolCompassPrivate *private = compass->private;

  switch (property_id)
    {
    case PROP_ORIENTATION:
      g_value_set_enum (value, private->orientation);
      break;
    case PROP_N_POINTS:
      g_value_set_int (value, private->n_points);
      break;
    case PROP_X1:
      g_value_set_int (value, private->x[0]);
      break;
    case PROP_Y1:
      g_value_set_int (value, private->y[0]);
      break;
    case PROP_X2:
      g_value_set_int (value, private->x[1]);
      break;
    case PROP_Y2:
      g_value_set_int (value, private->y[1]);
      break;
    case PROP_X3:
      g_value_set_int (value, private->x[2]);
      break;
    case PROP_Y3:
      g_value_set_int (value, private->y[2]);
      break;
    case PROP_PIXEL_ANGLE:
      g_value_set_double (value, private->pixel_angle);
      break;
    case PROP_UNIT_ANGLE:
      g_value_set_double (value, private->unit_angle);
      break;
    case PROP_EFFECTIVE_ORIENTATION:
      g_value_set_enum (value, private->effective_orientation);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_tool_compass_changed (GimpToolWidget *widget)
{
  GimpToolCompass        *compass       = GIMP_TOOL_COMPASS (widget);
  GimpToolCompassPrivate *private       = compass->private;
  GimpDisplayShell       *shell         = gimp_tool_widget_get_shell (widget);
  gdouble                 angle1;
  gdouble                 angle2;
  gint                    draw_arc      = 0;
  gboolean                draw_arc_line = FALSE;
  gdouble                 arc_line_display_length;
  gdouble                 arc_line_length;

  gimp_tool_compass_update_angle (compass, private->orientation, FALSE);

  angle1 = -atan2 (private->radius1.y * shell->scale_y,
                   private->radius1.x * shell->scale_x);
  angle2 = -private->display_angle;

  gimp_canvas_line_set (private->line1,
                        private->x[0],
                        private->y[0],
                        private->x[1],
                        private->y[1]);
  gimp_canvas_item_set_visible (private->line1, private->n_points > 1);
  if (private->n_points > 1 &&
      gimp_canvas_item_transform_distance (private->line1,
                                           private->x[0],
                                           private->y[0],
                                           private->x[1],
                                           private->y[1]) > ARC_RADIUS)
    {
      draw_arc++;
    }


  arc_line_display_length = ARC_RADIUS                           +
                            (GIMP_CANVAS_HANDLE_SIZE_CROSS >> 1) +
                            ARC_GAP;
  arc_line_length         = arc_line_display_length              /
                            hypot (private->radius2.x * shell->scale_x,
                                  private->radius2.y * shell->scale_y);

  if (private->n_points > 2)
    {
      gdouble length = gimp_canvas_item_transform_distance (private->line2,
                                                            private->x[0],
                                                            private->y[0],
                                                            private->x[2],
                                                            private->y[2]);

      if (length > ARC_RADIUS)
        {
          draw_arc++;
          draw_arc_line = TRUE;

          if (length > arc_line_display_length)
            {
              gimp_canvas_line_set (
                private->line2,
                private->x[0] + private->radius2.x * arc_line_length,
                private->y[0] + private->radius2.y * arc_line_length,
                private->x[2],
                private->y[2]);
              gimp_canvas_item_set_visible (private->line2, TRUE);
            }
          else
            {
              gimp_canvas_item_set_visible (private->line2, FALSE);
            }
        }
      else
        {
          gimp_canvas_line_set (private->line2,
                                private->x[0],
                                private->y[0],
                                private->x[2],
                                private->y[2]);
          gimp_canvas_item_set_visible (private->line2, TRUE);
        }
    }
  else
    {
      gimp_canvas_item_set_visible (private->line2, FALSE);
    }

  gimp_canvas_handle_set_position (private->arc,
                                   private->x[0], private->y[0]);
  gimp_canvas_handle_set_angles (private->arc, angle1, angle2);
  gimp_canvas_item_set_visible (private->arc,
                                private->n_points > 1             &&
                                draw_arc == private->n_points - 1 &&
                                fabs (angle2) > EPSILON);

  arc_line_length = (ARC_RADIUS + (GIMP_CANVAS_HANDLE_SIZE_CROSS >> 1)) /
                    hypot (private->radius2.x * shell->scale_x,
                           private->radius2.y * shell->scale_y);

  gimp_canvas_line_set (private->arc_line,
                        private->x[0],
                        private->y[0],
                        private->x[0] + private->radius2.x * arc_line_length,
                        private->y[0] + private->radius2.y * arc_line_length);
  gimp_canvas_item_set_visible (private->arc_line,
                                (private->n_points == 2 || draw_arc_line) &&
                                fabs (angle2) > EPSILON);

  gimp_canvas_handle_set_position (private->handles[0],
                                   private->x[0], private->y[0]);
  gimp_canvas_item_set_visible (private->handles[0],
                                private->n_points > 0);

  gimp_canvas_handle_set_position (private->handles[1],
                                   private->x[1], private->y[1]);
  gimp_canvas_item_set_visible (private->handles[1],
                                private->n_points > 1);

  gimp_canvas_handle_set_position (private->handles[2],
                                   private->x[2], private->y[2]);
  gimp_canvas_item_set_visible (private->handles[2],
                                private->n_points > 2);

  gimp_tool_compass_update_hilight (compass);
}

gint
gimp_tool_compass_button_press (GimpToolWidget      *widget,
                                const GimpCoords    *coords,
                                guint32              time,
                                GdkModifierType      state,
                                GimpButtonPressType  press_type)
{
  GimpToolCompass        *compass = GIMP_TOOL_COMPASS (widget);
  GimpToolCompassPrivate *private = compass->private;

  private->function = CREATING;

  private->mouse_x = coords->x;
  private->mouse_y = coords->y;

  /*  if the cursor is in one of the handles, the new function will be
   *  moving or adding a new point or guide
   */
  if (private->point != -1)
    {
      GdkModifierType extend_mask = gimp_get_extend_selection_mask ();
      GdkModifierType toggle_mask = gimp_get_toggle_behavior_mask ();

      if (state & (toggle_mask | GDK_MOD1_MASK))
        {
          gboolean create_hguide = (state & toggle_mask);
          gboolean create_vguide = (state & GDK_MOD1_MASK);

          g_signal_emit (compass, compass_signals[CREATE_GUIDES], 0,
                         private->x[private->point],
                         private->y[private->point],
                         create_hguide,
                         create_vguide);

          private->function = GUIDING;
        }
      else
        {
          if (private->n_points == 1 || (state & extend_mask))
            private->function = ADDING;
          else
            private->function = MOVING;
        }
    }

  /*  adding to the middle point makes no sense  */
  if (private->point    == 0      &&
      private->function == ADDING &&
      private->n_points == 3)
    {
      private->function = MOVING;
    }

  /*  if the function is still CREATING, we are outside the handles  */
  if (private->function == CREATING)
    {
      if (private->n_points > 1 && (state & GDK_MOD1_MASK))
        {
          private->function = MOVING_ALL;

          private->last_x = coords->x;
          private->last_y = coords->y;
        }
    }

  if (private->function == CREATING)
    {
      /*  set the first point and go into ADDING mode  */
      g_object_set (compass,
                    "n-points", 1,
                    "x1",       (gint) (coords->x + 0.5),
                    "y1",       (gint) (coords->y + 0.5),
                    "x2",       0,
                    "y2",       0,
                    "x3",       0,
                    "y3",       0,
                    NULL);

      private->point    = 0;
      private->function = ADDING;
    }

  return 1;
}

void
gimp_tool_compass_button_release (GimpToolWidget        *widget,
                                  const GimpCoords      *coords,
                                  guint32                time,
                                  GdkModifierType        state,
                                  GimpButtonReleaseType  release_type)
{
  GimpToolCompass        *compass = GIMP_TOOL_COMPASS (widget);
  GimpToolCompassPrivate *private = compass->private;

  private->function = FINISHED;
}

void
gimp_tool_compass_motion (GimpToolWidget   *widget,
                          const GimpCoords *coords,
                          guint32           time,
                          GdkModifierType   state)
{
  GimpToolCompass        *compass  = GIMP_TOOL_COMPASS (widget);
  GimpToolCompassPrivate *private  = compass->private;
  gint                    new_n_points;
  gint                    new_x[3];
  gint                    new_y[3];
  gint                    dx, dy;
  gint                    tmp;

  private->mouse_x = coords->x;
  private->mouse_y = coords->y;

  /*  A few comments here, because this routine looks quite weird at first ...
   *
   *  The goal is to keep point 0, called the start point, to be
   *  always the one in the middle or, if there are only two points,
   *  the one that is fixed.  The angle is then always measured at
   *  this point.
   */

  new_n_points = private->n_points;
  new_x[0]     = private->x[0];
  new_y[0]     = private->y[0];
  new_x[1]     = private->x[1];
  new_y[1]     = private->y[1];
  new_x[2]     = private->x[2];
  new_y[2]     = private->y[2];

  switch (private->function)
    {
    case ADDING:
      switch (private->point)
        {
        case 0:
          /*  we are adding to the start point  */
          break;

        case 1:
          /*  we are adding to the end point, make it the new start point  */
          new_x[0] = private->x[1];
          new_y[0] = private->y[1];

          new_x[1] = private->x[0];
          new_y[1] = private->y[0];
          break;

        case 2:
          /*  we are adding to the third point, make it the new start point  */
          new_x[1] = private->x[0];
          new_y[1] = private->y[0];
          new_x[0] = private->x[2];
          new_y[0] = private->y[2];
          break;

        default:
          break;
        }

      new_n_points = MIN (new_n_points + 1, 3);

      private->point    = new_n_points - 1;
      private->function = MOVING;
      /*  don't break here!  */

    case MOVING:
      /*  if we are moving the start point and only have two, make it
       *  the end point
       */
      if (new_n_points == 2 && private->point == 0)
        {
          tmp = new_x[0];
          new_x[0] = new_x[1];
          new_x[1] = tmp;

          tmp = new_y[0];
          new_y[0] = new_y[1];
          new_y[1] = tmp;

          private->point = 1;
        }

      new_x[private->point] = ROUND (coords->x);
      new_y[private->point] = ROUND (coords->y);

      if (state & gimp_get_constrain_behavior_mask ())
        {
          gdouble  x = new_x[private->point];
          gdouble  y = new_y[private->point];

          gimp_display_shell_constrain_line (gimp_tool_widget_get_shell (widget),
                                             new_x[0], new_y[0],
                                             &x, &y,
                                             GIMP_CONSTRAIN_LINE_15_DEGREES);

          new_x[private->point] = ROUND (x);
          new_y[private->point] = ROUND (y);
        }

      g_object_set (compass,
                    "n-points", new_n_points,
                    "x1",       new_x[0],
                    "y1",       new_y[0],
                    "x2",       new_x[1],
                    "y2",       new_y[1],
                    "x3",       new_x[2],
                    "y3",       new_y[2],
                    NULL);
      break;

    case MOVING_ALL:
      dx = ROUND (coords->x) - private->last_x;
      dy = ROUND (coords->y) - private->last_y;

      g_object_set (compass,
                    "x1", new_x[0] + dx,
                    "y1", new_y[0] + dy,
                    "x2", new_x[1] + dx,
                    "y2", new_y[1] + dy,
                    "x3", new_x[2] + dx,
                    "y3", new_y[2] + dy,
                    NULL);

      private->last_x = ROUND (coords->x);
      private->last_y = ROUND (coords->y);
      break;

    default:
      break;
    }
}

GimpHit
gimp_tool_compass_hit (GimpToolWidget   *widget,
                       const GimpCoords *coords,
                       GdkModifierType   state,
                       gboolean          proximity)
{
  GimpToolCompass *compass = GIMP_TOOL_COMPASS (widget);

  if (gimp_tool_compass_get_point (compass, coords) >= 0)
    return GIMP_HIT_DIRECT;
  else
    return GIMP_HIT_INDIRECT;
}

void
gimp_tool_compass_hover (GimpToolWidget   *widget,
                         const GimpCoords *coords,
                         GdkModifierType   state,
                         gboolean          proximity)
{
  GimpToolCompass        *compass = GIMP_TOOL_COMPASS (widget);
  GimpToolCompassPrivate *private = compass->private;
  gint                    point;

  private->mouse_x = coords->x;
  private->mouse_y = coords->y;

  point = gimp_tool_compass_get_point (compass, coords);

  if (point >= 0)
    {
      GdkModifierType  extend_mask = gimp_get_extend_selection_mask ();
      GdkModifierType  toggle_mask = gimp_get_toggle_behavior_mask ();
      gchar           *status;

      if (state & toggle_mask)
        {
          if (state & GDK_MOD1_MASK)
            {
              status = gimp_suggest_modifiers (_("Click to place "
                                                 "vertical and "
                                                 "horizontal guides"),
                                               0,
                                               NULL, NULL, NULL);
            }
          else
            {
              status = gimp_suggest_modifiers (_("Click to place a "
                                                 "horizontal guide"),
                                               GDK_MOD1_MASK & ~state,
                                               NULL, NULL, NULL);
            }
        }
      else if (state & GDK_MOD1_MASK)
        {
          status = gimp_suggest_modifiers (_("Click to place a "
                                             "vertical guide"),
                                           toggle_mask & ~state,
                                           NULL, NULL, NULL);
        }
      else if ((state & extend_mask) &&
               ! ((point == 0) && (private->n_points == 3)))
        {
          status = gimp_suggest_modifiers (_("Click-Drag to add a "
                                             "new point"),
                                           (toggle_mask |
                                            GDK_MOD1_MASK) & ~state,
                                           NULL, NULL, NULL);
        }
      else
        {
          if ((point == 0) && (private->n_points == 3))
            state |= extend_mask;

          status = gimp_suggest_modifiers (_("Click-Drag to move this "
                                             "point"),
                                           (extend_mask |
                                            toggle_mask |
                                            GDK_MOD1_MASK) & ~state,
                                           NULL, NULL, NULL);
        }

      gimp_tool_widget_set_status (widget, status);

      g_free (status);
    }
  else
    {
      if ((private->n_points > 1) && (state & GDK_MOD1_MASK))
        {
          gimp_tool_widget_set_status (widget,
                                       _("Click-Drag to move all points"));
        }
      else
        {
          gimp_tool_widget_set_status (widget, NULL);
        }
    }

  if (point != private->point)
    {
      private->point = point;

      gimp_tool_compass_update_hilight (compass);
    }
}

void
gimp_tool_compass_leave_notify (GimpToolWidget *widget)
{
  GimpToolCompass        *compass = GIMP_TOOL_COMPASS (widget);
  GimpToolCompassPrivate *private = compass->private;

  if (private->point != -1)
    {
      private->point = -1;

      gimp_tool_compass_update_hilight (compass);
    }

  GIMP_TOOL_WIDGET_CLASS (parent_class)->leave_notify (widget);
}

static void
gimp_tool_compass_motion_modifier (GimpToolWidget  *widget,
                                   GdkModifierType  key,
                                   gboolean         press,
                                   GdkModifierType  state)
{
  GimpToolCompass        *compass = GIMP_TOOL_COMPASS (widget);
  GimpToolCompassPrivate *private = compass->private;

  if (key == gimp_get_constrain_behavior_mask () &&
      private->function == MOVING)
    {
      gint    new_x[3];
      gint    new_y[3];
      gdouble x = private->mouse_x;
      gdouble y = private->mouse_y;

      new_x[0] = private->x[0];
      new_y[0] = private->y[0];
      new_x[1] = private->x[1];
      new_y[1] = private->y[1];
      new_x[2] = private->x[2];
      new_y[2] = private->y[2];

      if (press)
        {
          gimp_display_shell_constrain_line (gimp_tool_widget_get_shell (widget),
                                             private->x[0], private->y[0],
                                             &x, &y,
                                             GIMP_CONSTRAIN_LINE_15_DEGREES);
        }

      new_x[private->point] = ROUND (x);
      new_y[private->point] = ROUND (y);

      g_object_set (compass,
                    "x1", new_x[0],
                    "y1", new_y[0],
                    "x2", new_x[1],
                    "y2", new_y[1],
                    "x3", new_x[2],
                    "y3", new_y[2],
                    NULL);
    }
}

static gboolean
gimp_tool_compass_get_cursor (GimpToolWidget     *widget,
                              const GimpCoords   *coords,
                              GdkModifierType     state,
                              GimpCursorType     *cursor,
                              GimpToolCursorType *tool_cursor,
                              GimpCursorModifier *modifier)
{
  GimpToolCompass        *compass = GIMP_TOOL_COMPASS (widget);
  GimpToolCompassPrivate *private = compass->private;

  if (private->point != -1)
    {
      GdkModifierType extend_mask = gimp_get_extend_selection_mask ();
      GdkModifierType toggle_mask = gimp_get_toggle_behavior_mask ();

      if (state & toggle_mask)
        {
          if (state & GDK_MOD1_MASK)
            {
              *cursor = GIMP_CURSOR_CORNER_BOTTOM_RIGHT;
              return TRUE;
            }
          else
            {
              *cursor = GIMP_CURSOR_SIDE_BOTTOM;
              return TRUE;
            }
        }
      else if (state & GDK_MOD1_MASK)
        {
          *cursor = GIMP_CURSOR_SIDE_RIGHT;
          return TRUE;
        }
      else if ((state & extend_mask) &&
               ! ((private->point == 0) &&
                  (private->n_points == 3)))
        {
          *modifier = GIMP_CURSOR_MODIFIER_PLUS;
          return TRUE;
        }
      else
        {
          *modifier = GIMP_CURSOR_MODIFIER_MOVE;
          return TRUE;
        }
    }
  else
    {
      if ((private->n_points > 1) && (state & GDK_MOD1_MASK))
        {
          *modifier = GIMP_CURSOR_MODIFIER_MOVE;
          return TRUE;
        }
    }

  return FALSE;
}

static gint
gimp_tool_compass_get_point (GimpToolCompass  *compass,
                             const GimpCoords *coords)
{
  GimpToolCompassPrivate *private = compass->private;
  gint                    i;

  for (i = 0; i < private->n_points; i++)
    {
      if (gimp_canvas_item_hit (private->handles[i],
                                coords->x, coords->y))
        {
          return i;
        }
    }

  return -1;
}

static void
gimp_tool_compass_update_hilight (GimpToolCompass *compass)
{
  GimpToolCompassPrivate *private = compass->private;
  gint                    i;

  for (i = 0; i < private->n_points; i++)
    {
      if (private->handles[i])
        {
          gimp_canvas_item_set_highlight (private->handles[i],
                                          private->point == i);
        }
    }
}

static void
gimp_tool_compass_update_angle (GimpToolCompass        *compass,
                                GimpCompassOrientation  orientation,
                                gboolean                flip)
{
  GimpToolWidget         *widget  = GIMP_TOOL_WIDGET (compass);
  GimpToolCompassPrivate *private = compass->private;
  GimpDisplayShell       *shell   = gimp_tool_widget_get_shell (widget);
  GimpImage              *image   = gimp_display_get_image (shell->display);
  GimpVector2             radius1;
  GimpVector2             radius2;
  gdouble                 pixel_angle;
  gdouble                 unit_angle;
  gdouble                 xres;
  gdouble                 yres;

  gimp_image_get_resolution (image, &xres, &yres);

  private->radius1.x = private->x[1] - private->x[0];
  private->radius1.y = private->y[1] - private->y[0];

  if (private->n_points == 3)
    {
      orientation = GIMP_COMPASS_ORIENTATION_AUTO;

      private->radius2.x = private->x[2] - private->x[0];
      private->radius2.y = private->y[2] - private->y[0];
    }
  else
    {
      gdouble angle = -shell->rotate_angle * G_PI / 180.0;

      if (orientation == GIMP_COMPASS_ORIENTATION_VERTICAL)
        angle -= G_PI / 2.0;

      if (flip)
        angle += G_PI;

      if (shell->flip_horizontally)
        angle = G_PI - angle;
      if (shell->flip_vertically)
        angle = -angle;

      private->radius2.x = cos (angle);
      private->radius2.y = sin (angle);

      if (! shell->dot_for_dot)
        {
          private->radius2.x *= xres;
          private->radius2.y *= yres;

          gimp_vector2_normalize (&private->radius2);
        }
    }

  radius1 = private->radius1;
  radius2 = private->radius2;

  pixel_angle = atan2 (gimp_vector2_cross_product (&radius1, &radius2).x,
                       gimp_vector2_inner_product (&radius1, &radius2));

  radius1.x /= xres;
  radius1.y /= yres;

  radius2.x /= xres;
  radius2.y /= yres;

  unit_angle = atan2 (gimp_vector2_cross_product (&radius1, &radius2).x,
                      gimp_vector2_inner_product (&radius1, &radius2));

  if (shell->dot_for_dot)
    private->display_angle = pixel_angle;
  else
    private->display_angle = unit_angle;

  if (private->n_points == 2)
    {
      if (! flip && fabs (private->display_angle) > G_PI / 2.0 + EPSILON)
        {
          gimp_tool_compass_update_angle (compass, orientation, TRUE);

          return;
        }
      else if (orientation == GIMP_COMPASS_ORIENTATION_AUTO)
        {
          if (fabs (private->display_angle) <= G_PI / 4.0 + EPSILON)
            {
              orientation = GIMP_COMPASS_ORIENTATION_HORIZONTAL;
            }
          else
            {
              gimp_tool_compass_update_angle (compass,
                                              GIMP_COMPASS_ORIENTATION_VERTICAL,
                                              FALSE);

              return;
            }
        }
    }

  if (fabs (pixel_angle - private->pixel_angle) > EPSILON)
    {
      private->pixel_angle = pixel_angle;

      g_object_notify (G_OBJECT (compass), "pixel-angle");
    }

  if (fabs (unit_angle - private->unit_angle) > EPSILON)
    {
      private->unit_angle = unit_angle;

      g_object_notify (G_OBJECT (compass), "unit-angle");
    }

  if (orientation != private->effective_orientation)
    {
      private->effective_orientation = orientation;

      g_object_notify (G_OBJECT (compass), "effective-orientation");
    }
}


/*  public functions  */

GimpToolWidget *
gimp_tool_compass_new (GimpDisplayShell       *shell,
                       GimpCompassOrientation  orientation,
                       gint                    n_points,
                       gint                    x1,
                       gint                    y1,
                       gint                    x2,
                       gint                    y2,
                       gint                    x3,
                       gint                    y3)
{
  g_return_val_if_fail (GIMP_IS_DISPLAY_SHELL (shell), NULL);

  return g_object_new (GIMP_TYPE_TOOL_COMPASS,
                       "shell",       shell,
                       "orientation", orientation,
                       "n-points",    n_points,
                       "x1",          x1,
                       "y1",          y1,
                       "x2",          x2,
                       "y2",          y2,
                       NULL);
}
