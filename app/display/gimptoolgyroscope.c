/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimptoolgyroscope.c
 * Copyright (C) 2018 Ell
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
#include <gdk/gdkkeysyms.h>

#include "libgimpbase/gimpbase.h"
#include "libgimpmath/gimpmath.h"

#include "display-types.h"

#include "widgets/gimpwidgets-utils.h"

#include "gimpdisplayshell.h"
#include "gimpdisplayshell-transform.h"
#include "gimptoolgyroscope.h"

#include "gimp-intl.h"


#define EPSILON    1e-6
#define DEG_TO_RAD (G_PI / 180.0)


typedef enum
{
  MODE_NONE,
  MODE_PAN,
  MODE_ROTATE,
  MODE_ZOOM
} Mode;

typedef enum
{
  CONSTRAINT_NONE,
  CONSTRAINT_UNKNOWN,
  CONSTRAINT_HORIZONTAL,
  CONSTRAINT_VERTICAL
} Constraint;

enum
{
  PROP_0,
  PROP_YAW,
  PROP_PITCH,
  PROP_ROLL,
  PROP_ZOOM,
  PROP_INVERT,
  PROP_SPEED,
  PROP_PIVOT_X,
  PROP_PIVOT_Y
};

struct _GimpToolGyroscopePrivate
{
  gdouble    yaw;
  gdouble    pitch;
  gdouble    roll;
  gdouble    zoom;

  gdouble    orig_yaw;
  gdouble    orig_pitch;
  gdouble    orig_roll;
  gdouble    orig_zoom;

  gboolean   invert;

  gdouble    speed;

  gdouble    pivot_x;
  gdouble    pivot_y;

  Mode       mode;
  Constraint constraint;

  gdouble    last_x;
  gdouble    last_y;

  gdouble    last_angle;
  gdouble    curr_angle;

  gdouble    last_zoom;
};


/*  local function prototypes  */

static void       gimp_tool_gyroscope_set_property    (GObject               *object,
                                                       guint                  property_id,
                                                       const GValue          *value,
                                                       GParamSpec            *pspec);
static void       gimp_tool_gyroscope_get_property    (GObject               *object,
                                                       guint                  property_id,
                                                       GValue                *value,
                                                       GParamSpec            *pspec);

static gint       gimp_tool_gyroscope_button_press    (GimpToolWidget        *widget,
                                                       const GimpCoords      *coords,
                                                       guint32                time,
                                                       GdkModifierType        state,
                                                       GimpButtonPressType    press_type);
static void       gimp_tool_gyroscope_button_release  (GimpToolWidget        *widget,
                                                       const GimpCoords      *coords,
                                                       guint32                time,
                                                       GdkModifierType        state,
                                                       GimpButtonReleaseType  release_type);
static void       gimp_tool_gyroscope_motion          (GimpToolWidget        *widget,
                                                       const GimpCoords      *coords,
                                                       guint32                time,
                                                       GdkModifierType        state);
static GimpHit    gimp_tool_gyroscope_hit             (GimpToolWidget        *widget,
                                                       const GimpCoords      *coords,
                                                       GdkModifierType        state,
                                                       gboolean               proximity);
static void       gimp_tool_gyroscope_hover           (GimpToolWidget        *widget,
                                                       const GimpCoords      *coords,
                                                       GdkModifierType        state,
                                                       gboolean               proximity);
static gboolean   gimp_tool_gyroscope_key_press       (GimpToolWidget        *widget,
                                                       GdkEventKey           *kevent);
static void       gimp_tool_gyroscope_motion_modifier (GimpToolWidget        *widget,
                                                       GdkModifierType        key,
                                                       gboolean               press,
                                                       GdkModifierType        state);
static gboolean   gimp_tool_gyroscope_get_cursor      (GimpToolWidget        *widget,
                                                       const GimpCoords      *coords,
                                                       GdkModifierType        state,
                                                       GimpCursorType        *cursor,
                                                       GimpToolCursorType    *tool_cursor,
                                                       GimpCursorModifier    *modifier);

static void       gimp_tool_gyroscope_update_status   (GimpToolGyroscope      *gyroscope,
                                                       GdkModifierType         state);

static void       gimp_tool_gyroscope_save            (GimpToolGyroscope      *gyroscope);
static void       gimp_tool_gyroscope_restore         (GimpToolGyroscope      *gyroscope);

static void       gimp_tool_gyroscope_rotate          (GimpToolGyroscope      *gyroscope,
                                                       const GimpVector3      *axis);
static void       gimp_tool_gyroscope_rotate_vector   (GimpVector3            *vector,
                                                       const GimpVector3      *axis);


G_DEFINE_TYPE_WITH_PRIVATE (GimpToolGyroscope, gimp_tool_gyroscope,
                            GIMP_TYPE_TOOL_WIDGET)

#define parent_class gimp_tool_gyroscope_parent_class


static void
gimp_tool_gyroscope_class_init (GimpToolGyroscopeClass *klass)
{
  GObjectClass        *object_class = G_OBJECT_CLASS (klass);
  GimpToolWidgetClass *widget_class = GIMP_TOOL_WIDGET_CLASS (klass);

  object_class->set_property    = gimp_tool_gyroscope_set_property;
  object_class->get_property    = gimp_tool_gyroscope_get_property;

  widget_class->button_press    = gimp_tool_gyroscope_button_press;
  widget_class->button_release  = gimp_tool_gyroscope_button_release;
  widget_class->motion          = gimp_tool_gyroscope_motion;
  widget_class->hit             = gimp_tool_gyroscope_hit;
  widget_class->hover           = gimp_tool_gyroscope_hover;
  widget_class->key_press       = gimp_tool_gyroscope_key_press;
  widget_class->motion_modifier = gimp_tool_gyroscope_motion_modifier;
  widget_class->get_cursor      = gimp_tool_gyroscope_get_cursor;

  g_object_class_install_property (object_class, PROP_YAW,
                                   g_param_spec_double ("yaw", NULL, NULL,
                                                        -G_MAXDOUBLE,
                                                        +G_MAXDOUBLE,
                                                        0.0,
                                                        GIMP_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT));

  g_object_class_install_property (object_class, PROP_PITCH,
                                   g_param_spec_double ("pitch", NULL, NULL,
                                                        -G_MAXDOUBLE,
                                                        +G_MAXDOUBLE,
                                                        0.0,
                                                        GIMP_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT));

  g_object_class_install_property (object_class, PROP_ROLL,
                                   g_param_spec_double ("roll", NULL, NULL,
                                                        -G_MAXDOUBLE,
                                                        +G_MAXDOUBLE,
                                                        0.0,
                                                        GIMP_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT));

  g_object_class_install_property (object_class, PROP_ZOOM,
                                   g_param_spec_double ("zoom", NULL, NULL,
                                                        0.0,
                                                        +G_MAXDOUBLE,
                                                        1.0,
                                                        GIMP_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT));

  g_object_class_install_property (object_class, PROP_INVERT,
                                   g_param_spec_boolean ("invert", NULL, NULL,
                                                         FALSE,
                                                         GIMP_PARAM_READWRITE |
                                                         G_PARAM_CONSTRUCT));

  g_object_class_install_property (object_class, PROP_SPEED,
                                   g_param_spec_double ("speed", NULL, NULL,
                                                        -G_MAXDOUBLE,
                                                        +G_MAXDOUBLE,
                                                        1.0,
                                                        GIMP_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT));

  g_object_class_install_property (object_class, PROP_PIVOT_X,
                                   g_param_spec_double ("pivot-x", NULL, NULL,
                                                        -G_MAXDOUBLE,
                                                        +G_MAXDOUBLE,
                                                        0.0,
                                                        GIMP_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT));

  g_object_class_install_property (object_class, PROP_PIVOT_Y,
                                   g_param_spec_double ("pivot-y", NULL, NULL,
                                                        -G_MAXDOUBLE,
                                                        +G_MAXDOUBLE,
                                                        0.0,
                                                        GIMP_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT));
}

static void
gimp_tool_gyroscope_init (GimpToolGyroscope *gyroscope)
{
  gyroscope->private = gimp_tool_gyroscope_get_instance_private (gyroscope);
}

static void
gimp_tool_gyroscope_set_property (GObject      *object,
                                  guint         property_id,
                                  const GValue *value,
                                  GParamSpec   *pspec)
{
  GimpToolGyroscope        *gyroscope = GIMP_TOOL_GYROSCOPE (object);
  GimpToolGyroscopePrivate *private = gyroscope->private;

  switch (property_id)
    {
    case PROP_YAW:
      private->yaw = g_value_get_double (value);
      break;
    case PROP_PITCH:
      private->pitch = g_value_get_double (value);
      break;
    case PROP_ROLL:
      private->roll = g_value_get_double (value);
      break;
    case PROP_ZOOM:
      private->zoom = g_value_get_double (value);
      break;
    case PROP_INVERT:
      private->invert = g_value_get_boolean (value);
      break;
    case PROP_SPEED:
      private->speed = g_value_get_double (value);
      break;
    case PROP_PIVOT_X:
      private->pivot_x = g_value_get_double (value);
      break;
    case PROP_PIVOT_Y:
      private->pivot_y = g_value_get_double (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_tool_gyroscope_get_property (GObject    *object,
                                  guint       property_id,
                                  GValue     *value,
                                  GParamSpec *pspec)
{
  GimpToolGyroscope        *gyroscope = GIMP_TOOL_GYROSCOPE (object);
  GimpToolGyroscopePrivate *private = gyroscope->private;

  switch (property_id)
    {
    case PROP_YAW:
      g_value_set_double (value, private->yaw);
      break;
    case PROP_PITCH:
      g_value_set_double (value, private->pitch);
      break;
    case PROP_ROLL:
      g_value_set_double (value, private->roll);
      break;
    case PROP_ZOOM:
      g_value_set_double (value, private->zoom);
      break;
    case PROP_INVERT:
      g_value_set_boolean (value, private->invert);
      break;
    case PROP_SPEED:
      g_value_set_double (value, private->speed);
      break;
    case PROP_PIVOT_X:
      g_value_set_double (value, private->pivot_x);
      break;
    case PROP_PIVOT_Y:
      g_value_set_double (value, private->pivot_y);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static gint
gimp_tool_gyroscope_button_press (GimpToolWidget      *widget,
                                  const GimpCoords    *coords,
                                  guint32              time,
                                  GdkModifierType      state,
                                  GimpButtonPressType  press_type)
{
  GimpToolGyroscope        *gyroscope = GIMP_TOOL_GYROSCOPE (widget);
  GimpToolGyroscopePrivate *private   = gyroscope->private;

  gimp_tool_gyroscope_save (gyroscope);

  if (state & GDK_MOD1_MASK)
    {
      private->mode = MODE_ZOOM;

      private->last_zoom = private->zoom;
    }
  else if (state & gimp_get_extend_selection_mask ())
    {
      private->mode = MODE_ROTATE;

      private->last_angle = atan2 (coords->y - private->pivot_y,
                                   coords->x - private->pivot_x);
      private->curr_angle = private->last_angle;
    }
  else
    {
      private->mode = MODE_PAN;

      if (state & gimp_get_constrain_behavior_mask ())
        private->constraint = CONSTRAINT_UNKNOWN;
      else
        private->constraint = CONSTRAINT_NONE;
    }

  private->last_x = coords->x;
  private->last_y = coords->y;

  gimp_tool_gyroscope_update_status (gyroscope, state);

  return 1;
}

static void
gimp_tool_gyroscope_button_release (GimpToolWidget        *widget,
                                    const GimpCoords      *coords,
                                    guint32                time,
                                    GdkModifierType        state,
                                    GimpButtonReleaseType  release_type)
{
  GimpToolGyroscope        *gyroscope = GIMP_TOOL_GYROSCOPE (widget);
  GimpToolGyroscopePrivate *private   = gyroscope->private;

  if (release_type == GIMP_BUTTON_RELEASE_CANCEL)
    gimp_tool_gyroscope_restore (gyroscope);

  private->mode = MODE_NONE;

  gimp_tool_gyroscope_update_status (gyroscope, state);
}

static void
gimp_tool_gyroscope_motion (GimpToolWidget   *widget,
                            const GimpCoords *coords,
                            guint32           time,
                            GdkModifierType   state)
{
  GimpToolGyroscope        *gyroscope = GIMP_TOOL_GYROSCOPE (widget);
  GimpToolGyroscopePrivate *private   = gyroscope->private;
  GimpDisplayShell         *shell     = gimp_tool_widget_get_shell (widget);
  GimpVector3               axis      = {};

  switch (private->mode)
    {
      case MODE_PAN:
        {
          gdouble x1     = private->last_x;
          gdouble y1     = private->last_y;
          gdouble x2     = coords->x;
          gdouble y2     = coords->y;
          gdouble factor = 1.0 / private->zoom;

          if (private->constraint != CONSTRAINT_NONE)
            {
              gimp_display_shell_rotate_xy_f (shell, x1, y1, &x1, &y1);
              gimp_display_shell_rotate_xy_f (shell, x2, y2, &x2, &y2);

              if (private->constraint == CONSTRAINT_UNKNOWN)
                {
                  if (fabs (x2 - x1) > fabs (y2 - y1))
                    private->constraint = CONSTRAINT_HORIZONTAL;
                  else if (fabs (y2 - y1) > fabs (x2 - x1))
                    private->constraint = CONSTRAINT_VERTICAL;
                }

              if (private->constraint == CONSTRAINT_HORIZONTAL)
                y2 = y1;
              else if (private->constraint == CONSTRAINT_VERTICAL)
                x2 = x1;

              gimp_display_shell_unrotate_xy_f (shell, x1, y1, &x1, &y1);
              gimp_display_shell_unrotate_xy_f (shell, x2, y2, &x2, &y2);
            }

          if (private->invert)
            factor = 1.0 / factor;

          gimp_vector3_set (&axis, y2 - y1, x2 - x1, 0.0);
          gimp_vector3_mul (&axis, factor * private->speed);
        }
      break;

    case MODE_ROTATE:
      {
        gdouble angle;

        angle = atan2 (coords->y - private->pivot_y,
                       coords->x - private->pivot_x);

        private->curr_angle = angle;

        angle -= private->last_angle;

        if (state & gimp_get_constrain_behavior_mask ())
          angle = RINT (angle / (G_PI / 6.0)) * (G_PI / 6.0);

        gimp_vector3_set (&axis, 0.0, 0.0, angle);

        private->last_angle += angle;
      }
    break;

    case MODE_ZOOM:
      {
        gdouble x1, y1;
        gdouble x2, y2;
        gdouble zoom;

        gimp_display_shell_transform_xy_f (shell,
                                           private->last_x, private->last_y,
                                           &x1,             &y1);
        gimp_display_shell_transform_xy_f (shell,
                                           coords->x,       coords->y,
                                           &x2,             &y2);

        zoom = (y1 - y2) * shell->scale_y / 128.0;

        if (private->invert)
          zoom = -zoom;

        private->last_zoom *= pow (2.0, zoom);

        zoom = log (private->last_zoom / private->zoom) / G_LN2;

        if (state & gimp_get_constrain_behavior_mask ())
          zoom = RINT (zoom * 2.0) / 2.0;

        g_object_set (gyroscope,
                      "zoom", private->zoom * pow (2.0, zoom),
                      NULL);
      }
      break;

    case MODE_NONE:
      g_return_if_reached ();
  }

  private->last_x = coords->x;
  private->last_y = coords->y;

  gimp_tool_gyroscope_rotate (gyroscope, &axis);
}

static GimpHit
gimp_tool_gyroscope_hit (GimpToolWidget   *widget,
                         const GimpCoords *coords,
                         GdkModifierType   state,
                         gboolean          proximity)
{
  return GIMP_HIT_INDIRECT;
}

static void
gimp_tool_gyroscope_hover (GimpToolWidget   *widget,
                           const GimpCoords *coords,
                           GdkModifierType   state,
                           gboolean          proximity)
{
  gimp_tool_gyroscope_update_status (GIMP_TOOL_GYROSCOPE (widget), state);
}

static gboolean
gimp_tool_gyroscope_key_press (GimpToolWidget *widget,
                               GdkEventKey    *kevent)
{
  GimpToolGyroscope        *gyroscope = GIMP_TOOL_GYROSCOPE (widget);
  GimpToolGyroscopePrivate *private   = gyroscope->private;
  GimpDisplayShell         *shell     = gimp_tool_widget_get_shell (widget);
  GimpVector3               axis      = {};
  gboolean                  fast;
  gboolean                  result    = FALSE;

  fast = (kevent->state & gimp_get_constrain_behavior_mask ());

  if (kevent->state & GDK_MOD1_MASK)
    {
      /* zoom */
      gdouble zoom = 0.0;

      switch (kevent->keyval)
        {
        case GDK_KEY_Up:
          zoom   = fast ? +1.0 : +1.0 / 8.0;
          result = TRUE;
          break;

        case GDK_KEY_Down:
          zoom   = fast ? -1.0 : -1.0 / 8.0;
          result = TRUE;
          break;
        }

      if (private->invert)
        zoom = -zoom;

      if (zoom)
        {
          g_object_set (gyroscope,
                        "zoom", private->zoom * pow (2.0, zoom),
                        NULL);
        }
    }
  else if (kevent->state & gimp_get_extend_selection_mask ())
    {
      /* rotate */
      gdouble angle = 0.0;

      switch (kevent->keyval)
        {
        case GDK_KEY_Left:
          angle  = fast ? +15.0 : +1.0;
          result = TRUE;
          break;

        case GDK_KEY_Right:
          angle  = fast ? -15.0 : -1.0;
          result = TRUE;
          break;
        }

      if (shell->flip_horizontally ^ shell->flip_vertically)
        angle = -angle;

      gimp_vector3_set (&axis, 0.0, 0.0, angle * DEG_TO_RAD);
    }
  else
    {
      /* pan */
      gdouble x0     = 0.0;
      gdouble y0     = 0.0;
      gdouble x      = 0.0;
      gdouble y      = 0.0;
      gdouble factor = 1.0 / private->zoom;

      if (private->invert)
        factor = 1.0 / factor;

      switch (kevent->keyval)
        {
        case GDK_KEY_Left:
          x      = fast ? +15.0 : +1.0;
          result = TRUE;
          break;

        case GDK_KEY_Right:
          x      = fast ? -15.0 : -1.0;
          result = TRUE;
          break;

        case GDK_KEY_Up:
          y      = fast ? +15.0 : +1.0;
          result = TRUE;
          break;

        case GDK_KEY_Down:
          y      = fast ? -15.0 : -1.0;
          result = TRUE;
          break;
        }

      gimp_display_shell_unrotate_xy_f (shell, x0, y0, &x0, &y0);
      gimp_display_shell_unrotate_xy_f (shell, x,  y,  &x,  &y);

      gimp_vector3_set (&axis,
                        (y - y0) * DEG_TO_RAD,
                        (x - x0) * DEG_TO_RAD,
                        0.0);
      gimp_vector3_mul (&axis, factor);
    }

  gimp_tool_gyroscope_rotate (gyroscope, &axis);

  return result;
}

static void
gimp_tool_gyroscope_motion_modifier (GimpToolWidget  *widget,
                                     GdkModifierType  key,
                                     gboolean         press,
                                     GdkModifierType  state)
{
  GimpToolGyroscope        *gyroscope = GIMP_TOOL_GYROSCOPE (widget);
  GimpToolGyroscopePrivate *private   = gyroscope->private;

  gimp_tool_gyroscope_update_status (gyroscope, state);

  if (key == gimp_get_constrain_behavior_mask ())
    {
      switch (private->mode)
      {
        case MODE_PAN:
          if (state & gimp_get_constrain_behavior_mask ())
            private->constraint = CONSTRAINT_UNKNOWN;
          else
            private->constraint = CONSTRAINT_NONE;
          break;

        case MODE_ROTATE:
          if (! (state & gimp_get_constrain_behavior_mask ()))
            private->last_angle = private->curr_angle;
          break;

        case MODE_ZOOM:
          if (! (state & gimp_get_constrain_behavior_mask ()))
            private->last_zoom = private->zoom;
          break;

        case MODE_NONE:
          break;
      }
    }
}

static gboolean
gimp_tool_gyroscope_get_cursor (GimpToolWidget     *widget,
                                const GimpCoords   *coords,
                                GdkModifierType     state,
                                GimpCursorType     *cursor,
                                GimpToolCursorType *tool_cursor,
                                GimpCursorModifier *modifier)
{
  if (state & GDK_MOD1_MASK)
    *modifier = GIMP_CURSOR_MODIFIER_ZOOM;
  else if (state & gimp_get_extend_selection_mask ())
    *modifier = GIMP_CURSOR_MODIFIER_ROTATE;
  else
    *modifier = GIMP_CURSOR_MODIFIER_MOVE;

  return TRUE;
}

static void
gimp_tool_gyroscope_update_status (GimpToolGyroscope *gyroscope,
                                   GdkModifierType    state)
{
  GimpToolGyroscopePrivate *private = gyroscope->private;
  gchar                    *status;

  if (private->mode == MODE_ZOOM ||
      (private->mode == MODE_NONE &&
       state & GDK_MOD1_MASK))
    {
      status = gimp_suggest_modifiers (_("Click-Drag to zoom"),
                                       gimp_get_toggle_behavior_mask () &
                                       ~state,
                                       NULL,
                                       _("%s for constrained steps"),
                                       NULL);
    }
  else if (private->mode == MODE_ROTATE ||
           (private->mode == MODE_NONE &&
            state & gimp_get_extend_selection_mask ()))
    {
      status = gimp_suggest_modifiers (_("Click-Drag to rotate"),
                                       gimp_get_toggle_behavior_mask () &
                                       ~state,
                                       NULL,
                                       _("%s for constrained angles"),
                                       NULL);
    }
  else
    {
      status = gimp_suggest_modifiers (_("Click-Drag to pan"),
                                       (((gimp_get_extend_selection_mask () |
                                          GDK_MOD1_MASK)                    *
                                         (private->mode == MODE_NONE))      |
                                        gimp_get_toggle_behavior_mask  ())  &
                                        ~state,
                                       _("%s to rotate"),
                                       _("%s for a constrained axis"),
                                       _("%s to zoom"));
    }

  gimp_tool_widget_set_status (GIMP_TOOL_WIDGET (gyroscope), status);

  g_free (status);
}

static void
gimp_tool_gyroscope_save (GimpToolGyroscope *gyroscope)
{
  GimpToolGyroscopePrivate *private = gyroscope->private;

  private->orig_yaw   = private->yaw;
  private->orig_pitch = private->pitch;
  private->orig_roll  = private->roll;
  private->orig_zoom  = private->zoom;
}

static void
gimp_tool_gyroscope_restore (GimpToolGyroscope *gyroscope)
{
  GimpToolGyroscopePrivate *private = gyroscope->private;

  g_object_set (gyroscope,
                "yaw",   private->orig_yaw,
                "pitch", private->orig_pitch,
                "roll",  private->orig_roll,
                "zoom",  private->orig_zoom,
                NULL);
}

static void
gimp_tool_gyroscope_rotate (GimpToolGyroscope *gyroscope,
                            const GimpVector3 *axis)
{
  GimpToolGyroscopePrivate *private = gyroscope->private;
  GimpVector3               real_axis;
  GimpVector3               basis[2];
  gdouble                   yaw;
  gdouble                   pitch;
  gdouble                   roll;
  gint                      i;

  if (gimp_vector3_length (axis) < EPSILON)
    return;

  real_axis = *axis;

  if (private->invert)
    gimp_vector3_neg (&real_axis);

  for (i = 0; i < 2; i++)
    {
      gimp_vector3_set (&basis[i], i == 0, i == 1, 0.0);

      if (private->invert)
        gimp_tool_gyroscope_rotate_vector (&basis[i], &real_axis);

      gimp_tool_gyroscope_rotate_vector (
        &basis[i], &(GimpVector3) {0.0, private->yaw * DEG_TO_RAD, 0.0});
      gimp_tool_gyroscope_rotate_vector (
        &basis[i], &(GimpVector3) {private->pitch * DEG_TO_RAD, 0.0, 0.0});
      gimp_tool_gyroscope_rotate_vector (
        &basis[i], &(GimpVector3) {0.0, 0.0, private->roll * DEG_TO_RAD});

      if (! private->invert)
        gimp_tool_gyroscope_rotate_vector (&basis[i], &real_axis);
    }

  roll = atan2 (basis[1].x, basis[1].y);

  for (i = 0; i < 2; i++)
    {
      gimp_tool_gyroscope_rotate_vector (
        &basis[i], &(GimpVector3) {0.0, 0.0, -roll});
    }

  pitch = atan2 (-basis[1].z, basis[1].y);

  for (i = 0; i < 1; i++)
    {
      gimp_tool_gyroscope_rotate_vector (
        &basis[i], &(GimpVector3) {-pitch, 0.0, 0.0});
    }

  yaw = atan2 (basis[0].z, basis[0].x);

  g_object_set (gyroscope,
                "yaw",   yaw   / DEG_TO_RAD,
                "pitch", pitch / DEG_TO_RAD,
                "roll",  roll  / DEG_TO_RAD,
                NULL);
}

static void
gimp_tool_gyroscope_rotate_vector (GimpVector3       *vector,
                                   const GimpVector3 *axis)
{
  GimpVector3 normalized_axis;
  GimpVector3 projection;
  GimpVector3 u;
  GimpVector3 v;
  gdouble     angle;

  angle = gimp_vector3_length (axis);

  if (angle < EPSILON)
    return;

  normalized_axis = gimp_vector3_mul_val (*axis, 1.0 / angle);

  projection = gimp_vector3_mul_val (
    normalized_axis,
    gimp_vector3_inner_product (vector, &normalized_axis));

  u = gimp_vector3_sub_val (*vector, projection);
  v = gimp_vector3_cross_product (&u, &normalized_axis);

  gimp_vector3_mul (&u, cos (angle));
  gimp_vector3_mul (&v, sin (angle));

  gimp_vector3_add (vector, &u, &v);
  gimp_vector3_add (vector, vector, &projection);
}


/*  public functions  */


GimpToolWidget *
gimp_tool_gyroscope_new (GimpDisplayShell *shell)
{
  g_return_val_if_fail (GIMP_IS_DISPLAY_SHELL (shell), NULL);

  return g_object_new (GIMP_TYPE_TOOL_GYROSCOPE,
                       "shell", shell,
                       NULL);
}
