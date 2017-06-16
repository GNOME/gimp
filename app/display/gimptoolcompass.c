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
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
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
#include "gimptoolcompass.h"

#include "gimp-intl.h"


#define GIMP_TOOL_HANDLE_SIZE_CROSS     15 /* FIXME */


#define ARC_RADIUS 30

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
  PROP_N_POINTS,
  PROP_X1,
  PROP_Y1,
  PROP_X2,
  PROP_Y2,
  PROP_X3,
  PROP_Y3
};

enum
{
  CREATE_GUIDES,
  LAST_SIGNAL
};

struct _GimpToolCompassPrivate
{
  gint             n_points;
  gint             x[3];
  gint             y[3];

  gdouble          angle1;
  gdouble          angle2;

  CompassFunction  function;
  gdouble          mouse_x;
  gdouble          mouse_y;
  gint             last_x;
  gint             last_y;
  gint             point;

  GimpCanvasItem  *line1;
  GimpCanvasItem  *line2;
  GimpCanvasItem  *angle;
  GimpCanvasItem  *angle_line;
  GimpCanvasItem  *handles[3];
};


/*  local function prototypes  */

static void     gimp_tool_compass_constructed     (GObject               *object);
static void     gimp_tool_compass_set_property    (GObject               *object,
                                                   guint                  property_id,
                                                   const GValue          *value,
                                                   GParamSpec            *pspec);
static void     gimp_tool_compass_get_property    (GObject               *object,
                                                   guint                  property_id,
                                                   GValue                *value,
                                                   GParamSpec            *pspec);

static void     gimp_tool_compass_changed         (GimpToolWidget        *widget);
static gint     gimp_tool_compass_button_press    (GimpToolWidget        *widget,
                                                   const GimpCoords      *coords,
                                                   guint32                time,
                                                   GdkModifierType        state,
                                                   GimpButtonPressType    press_type);
static void     gimp_tool_compass_button_release  (GimpToolWidget        *widget,
                                                   const GimpCoords      *coords,
                                                   guint32                time,
                                                   GdkModifierType        state,
                                                   GimpButtonReleaseType  release_type);
static void     gimp_tool_compass_motion          (GimpToolWidget        *widget,
                                                   const GimpCoords      *coords,
                                                   guint32                time,
                                                   GdkModifierType        state);
static void     gimp_tool_compass_hover           (GimpToolWidget        *widget,
                                                   const GimpCoords      *coords,
                                                   GdkModifierType        state,
                                                   gboolean               proximity);
static void     gimp_tool_compass_motion_modifier (GimpToolWidget        *widget,
                                                   GdkModifierType        key,
                                                   gboolean               press,
                                                   GdkModifierType        state);
static gboolean gimp_tool_compass_get_cursor      (GimpToolWidget        *widget,
                                                   const GimpCoords      *coords,
                                                   GdkModifierType        state,
                                                   GimpCursorType        *cursor,
                                                   GimpToolCursorType    *tool_cursor,
                                                   GimpCursorModifier    *cursor_modifier);

static void     gimp_tool_compass_update_hilight  (GimpToolCompass       *compass);


G_DEFINE_TYPE (GimpToolCompass, gimp_tool_compass, GIMP_TYPE_TOOL_WIDGET)

#define parent_class gimp_tool_compass_parent_class

static guint compass_signals[LAST_SIGNAL] = { 0 };


static void
gimp_tool_compass_class_init (GimpToolCompassClass *klass)
{
  GObjectClass        *object_class = G_OBJECT_CLASS (klass);
  GimpToolWidgetClass *widget_class = GIMP_TOOL_WIDGET_CLASS (klass);

  object_class->constructed     = gimp_tool_compass_constructed;
  object_class->set_property    = gimp_tool_compass_set_property;
  object_class->get_property    = gimp_tool_compass_get_property;

  widget_class->changed         = gimp_tool_compass_changed;
  widget_class->button_press    = gimp_tool_compass_button_press;
  widget_class->button_release  = gimp_tool_compass_button_release;
  widget_class->motion          = gimp_tool_compass_motion;
  widget_class->hover           = gimp_tool_compass_hover;
  widget_class->motion_modifier = gimp_tool_compass_motion_modifier;
  widget_class->get_cursor      = gimp_tool_compass_get_cursor;

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

  g_type_class_add_private (klass, sizeof (GimpToolCompassPrivate));
}

static void
gimp_tool_compass_init (GimpToolCompass *compass)
{
  compass->private = G_TYPE_INSTANCE_GET_PRIVATE (compass,
                                                  GIMP_TYPE_TOOL_COMPASS,
                                                  GimpToolCompassPrivate);

  compass->private->point = -1;
}

static gdouble
gimp_tool_compass_get_angle (gint    dx,
                             gint    dy,
                             gdouble xres,
                             gdouble yres)
{
  gdouble angle;

  if (dx)
    angle = gimp_rad_to_deg (atan (((gdouble) (dy) / yres) /
                                   ((gdouble) (dx) / xres)));
  else if (dy)
    angle = dy > 0 ? 270.0 : 90.0;
  else
    angle = 180.0;

  if (dx > 0)
    {
      if (dy > 0)
        angle = 360.0 - angle;
      else
        angle = -angle;
    }
  else
    {
      angle = 180.0 - angle;
    }

  return angle;
}

static void
gimp_tool_compass_update_angles (GimpToolCompass *compass)
{
  GimpToolWidget         *widget  = GIMP_TOOL_WIDGET (compass);
  GimpToolCompassPrivate *private = compass->private;
  GimpDisplayShell       *shell   = gimp_tool_widget_get_shell (widget);
  GimpImage              *image   = gimp_display_get_image (shell->display);
  gint                    ax, ay;
  gint                    bx, by;
  gdouble                 xres;
  gdouble                 yres;

  ax = private->x[1] - private->x[0];
  ay = private->y[1] - private->y[0];

  if (private->n_points == 3)
    {
      bx = private->x[2] - private->x[0];
      by = private->y[2] - private->y[0];
    }
  else
    {
      bx = 0;
      by = 0;
    }

  gimp_image_get_resolution (image, &xres, &yres);

  if (private->n_points != 3)
    bx = ax > 0 ? 1 : -1;

  private->angle1 = gimp_tool_compass_get_angle (ax, ay, xres, yres);
  private->angle2 = gimp_tool_compass_get_angle (bx, by, xres, yres);
}

static void
gimp_tool_compass_constructed (GObject *object)
{
  GimpToolCompass        *compass = GIMP_TOOL_COMPASS (object);
  GimpToolWidget         *widget  = GIMP_TOOL_WIDGET (object);
  GimpToolCompassPrivate *private = compass->private;
  GimpCanvasGroup        *stroke_group;

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

  private->angle = gimp_tool_widget_add_handle (widget,
                                                GIMP_HANDLE_CIRCLE,
                                                private->x[0],
                                                private->y[0],
                                                ARC_RADIUS * 2 + 1,
                                                ARC_RADIUS * 2 + 1,
                                                GIMP_HANDLE_ANCHOR_CENTER);

  private->angle_line = gimp_tool_widget_add_line (widget,
                                                   private->x[0],
                                                   private->y[0],
                                                   private->x[0] + 10,
                                                   private->y[0]);

  gimp_tool_widget_pop_group (widget);

  private->handles[0] = gimp_tool_widget_add_handle (widget,
                                                     GIMP_HANDLE_CIRCLE,
                                                     private->x[0],
                                                     private->y[0],
                                                     GIMP_TOOL_HANDLE_SIZE_CROSS,
                                                     GIMP_TOOL_HANDLE_SIZE_CROSS,
                                                     GIMP_HANDLE_ANCHOR_CENTER);

  private->handles[1] = gimp_tool_widget_add_handle (widget,
                                                     GIMP_HANDLE_CROSS,
                                                     private->x[1],
                                                     private->y[1],
                                                     GIMP_TOOL_HANDLE_SIZE_CROSS,
                                                     GIMP_TOOL_HANDLE_SIZE_CROSS,
                                                     GIMP_HANDLE_ANCHOR_CENTER);

  private->handles[2] = gimp_tool_widget_add_handle (widget,
                                                     GIMP_HANDLE_CROSS,
                                                     private->x[2],
                                                     private->y[2],
                                                     GIMP_TOOL_HANDLE_SIZE_CROSS,
                                                     GIMP_TOOL_HANDLE_SIZE_CROSS,
                                                     GIMP_HANDLE_ANCHOR_CENTER);

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

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_tool_compass_changed (GimpToolWidget *widget)
{
  GimpToolCompass        *compass = GIMP_TOOL_COMPASS (widget);
  GimpToolCompassPrivate *private = compass->private;
  GimpDisplayShell       *shell   = gimp_tool_widget_get_shell (widget);
  gdouble                 angle1;
  gdouble                 angle2;
  gint                    draw_arc = 0;
  gdouble                 target;
  gdouble                 arc_radius;

  gimp_tool_compass_update_angles (compass);

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

  gimp_canvas_line_set (private->line2,
                        private->x[0],
                        private->y[0],
                        private->x[2],
                        private->y[2]);
  gimp_canvas_item_set_visible (private->line2, private->n_points > 2);
  if (private->n_points > 2 &&
      gimp_canvas_item_transform_distance (private->line2,
                                           private->x[0],
                                           private->y[0],
                                           private->x[2],
                                           private->y[2]) > ARC_RADIUS)
    {
      draw_arc++;
    }

  angle1 = private->angle2 / 180.0 * G_PI;
  angle2 = (private->angle1 - private->angle2) / 180.0 * G_PI;

  if (angle2 > G_PI)
    angle2 -= 2.0 * G_PI;

  if (angle2 < -G_PI)
    angle2 += 2.0 * G_PI;

  gimp_canvas_handle_set_position (private->angle,
                                   private->x[0], private->y[0]);
  gimp_canvas_handle_set_angles (private->angle, angle1, angle2);
  gimp_canvas_item_set_visible (private->angle,
                                private->n_points > 1             &&
                                draw_arc == private->n_points - 1 &&
                                angle2 != 0.0);

  target     = FUNSCALEX (shell, (GIMP_TOOL_HANDLE_SIZE_CROSS >> 1));
  arc_radius = FUNSCALEX (shell, ARC_RADIUS);

  gimp_canvas_line_set (private->angle_line,
                        private->x[0],
                        private->y[0],
                        private->x[0] + (private->x[1] >= private->x[0] ?
                                          (arc_radius + target) :
                                         -(arc_radius + target)),
                        private->y[0]);
  gimp_canvas_item_set_visible (private->angle_line,
                                private->n_points == 2 &&
                                angle2 != 0.0);

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

          gimp_constrain_line (new_x[0], new_y[0],
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

void
gimp_tool_compass_hover (GimpToolWidget   *widget,
                         const GimpCoords *coords,
                         GdkModifierType   state,
                         gboolean          proximity)
{
  GimpToolCompass        *compass = GIMP_TOOL_COMPASS (widget);
  GimpToolCompassPrivate *private = compass->private;
  gint                    point   = -1;
  gint                    i;

  private->mouse_x = coords->x;
  private->mouse_y = coords->y;

  for (i = 0; i < private->n_points; i++)
    {
      if (gimp_canvas_item_hit (private->handles[i],
                                coords->x, coords->y))
        {
          GdkModifierType  extend_mask = gimp_get_extend_selection_mask ();
          GdkModifierType  toggle_mask = gimp_get_toggle_behavior_mask ();
          gchar           *status;

          point = i;

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

              gimp_tool_widget_status (widget, status);
              g_free (status);
              break;
            }

          if (state & GDK_MOD1_MASK)
            {
              status = gimp_suggest_modifiers (_("Click to place a "
                                                 "vertical guide"),
                                               toggle_mask & ~state,
                                               NULL, NULL, NULL);
              gimp_tool_widget_status (widget, status);
              g_free (status);
              break;
            }

          if ((state & extend_mask) &&
              ! ((i == 0) && (private->n_points == 3)))
            {
              status = gimp_suggest_modifiers (_("Click-Drag to add a "
                                                 "new point"),
                                               (toggle_mask |
                                                GDK_MOD1_MASK) & ~state,
                                               NULL, NULL, NULL);
            }
          else
            {
              if ((i == 0) && (private->n_points == 3))
                state |= extend_mask;
              status = gimp_suggest_modifiers (_("Click-Drag to move this "
                                                 "point"),
                                               (extend_mask |
                                                toggle_mask |
                                                GDK_MOD1_MASK) & ~state,
                                               NULL, NULL, NULL);
            }

          gimp_tool_widget_status (widget, status);
          g_free (status);
          break;
        }
    }

  if (point == -1)
    {
      if ((private->n_points > 1) && (state & GDK_MOD1_MASK))
        {
          gimp_tool_widget_status (widget, _("Click-Drag to move all points"));
        }
      else
        {
          gimp_tool_widget_status (widget, NULL);
        }
    }

  if (point != private->point)
    {
      private->point = point;

      gimp_tool_compass_update_hilight (compass);
    }
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
        gimp_constrain_line (private->x[0], private->y[0],
                             &x, &y,
                             GIMP_CONSTRAIN_LINE_15_DEGREES);

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
                              GimpCursorModifier *cursor_modifier)
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
          *cursor_modifier = GIMP_CURSOR_MODIFIER_PLUS;
          return TRUE;
        }
      else
        {
          *cursor_modifier = GIMP_CURSOR_MODIFIER_MOVE;
          return TRUE;
        }
    }
  else
    {
      if ((private->n_points > 1) && (state & GDK_MOD1_MASK))
        {
          *cursor_modifier = GIMP_CURSOR_MODIFIER_MOVE;
          return TRUE;
        }
    }

  return FALSE;
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


/*  public functions  */

GimpToolWidget *
gimp_tool_compass_new (GimpDisplayShell *shell,
                       gint              n_points,
                       gint              x1,
                       gint              y1,
                       gint              x2,
                       gint              y2,
                       gint              x3,
                       gint              y3)
{
  g_return_val_if_fail (GIMP_IS_DISPLAY_SHELL (shell), NULL);

  return g_object_new (GIMP_TYPE_TOOL_COMPASS,
                       "shell",    shell,
                       "n-points", n_points,
                       "x1",       x1,
                       "y1",       y1,
                       "x2",       x2,
                       "y2",       y2,
                       NULL);
}
