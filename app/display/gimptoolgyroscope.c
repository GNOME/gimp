/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ligmatoolgyroscope.c
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

#include "libligmabase/ligmabase.h"
#include "libligmamath/ligmamath.h"

#include "display-types.h"

#include "widgets/ligmawidgets-utils.h"

#include "ligmadisplayshell.h"
#include "ligmadisplayshell-transform.h"
#include "ligmatoolgyroscope.h"

#include "ligma-intl.h"


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

struct _LigmaToolGyroscopePrivate
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

static void       ligma_tool_gyroscope_set_property    (GObject               *object,
                                                       guint                  property_id,
                                                       const GValue          *value,
                                                       GParamSpec            *pspec);
static void       ligma_tool_gyroscope_get_property    (GObject               *object,
                                                       guint                  property_id,
                                                       GValue                *value,
                                                       GParamSpec            *pspec);

static gint       ligma_tool_gyroscope_button_press    (LigmaToolWidget        *widget,
                                                       const LigmaCoords      *coords,
                                                       guint32                time,
                                                       GdkModifierType        state,
                                                       LigmaButtonPressType    press_type);
static void       ligma_tool_gyroscope_button_release  (LigmaToolWidget        *widget,
                                                       const LigmaCoords      *coords,
                                                       guint32                time,
                                                       GdkModifierType        state,
                                                       LigmaButtonReleaseType  release_type);
static void       ligma_tool_gyroscope_motion          (LigmaToolWidget        *widget,
                                                       const LigmaCoords      *coords,
                                                       guint32                time,
                                                       GdkModifierType        state);
static LigmaHit    ligma_tool_gyroscope_hit             (LigmaToolWidget        *widget,
                                                       const LigmaCoords      *coords,
                                                       GdkModifierType        state,
                                                       gboolean               proximity);
static void       ligma_tool_gyroscope_hover           (LigmaToolWidget        *widget,
                                                       const LigmaCoords      *coords,
                                                       GdkModifierType        state,
                                                       gboolean               proximity);
static gboolean   ligma_tool_gyroscope_key_press       (LigmaToolWidget        *widget,
                                                       GdkEventKey           *kevent);
static void       ligma_tool_gyroscope_motion_modifier (LigmaToolWidget        *widget,
                                                       GdkModifierType        key,
                                                       gboolean               press,
                                                       GdkModifierType        state);
static gboolean   ligma_tool_gyroscope_get_cursor      (LigmaToolWidget        *widget,
                                                       const LigmaCoords      *coords,
                                                       GdkModifierType        state,
                                                       LigmaCursorType        *cursor,
                                                       LigmaToolCursorType    *tool_cursor,
                                                       LigmaCursorModifier    *modifier);

static void       ligma_tool_gyroscope_update_status   (LigmaToolGyroscope      *gyroscope,
                                                       GdkModifierType         state);

static void       ligma_tool_gyroscope_save            (LigmaToolGyroscope      *gyroscope);
static void       ligma_tool_gyroscope_restore         (LigmaToolGyroscope      *gyroscope);

static void       ligma_tool_gyroscope_rotate          (LigmaToolGyroscope      *gyroscope,
                                                       const LigmaVector3      *axis);
static void       ligma_tool_gyroscope_rotate_vector   (LigmaVector3            *vector,
                                                       const LigmaVector3      *axis);


G_DEFINE_TYPE_WITH_PRIVATE (LigmaToolGyroscope, ligma_tool_gyroscope,
                            LIGMA_TYPE_TOOL_WIDGET)

#define parent_class ligma_tool_gyroscope_parent_class


static void
ligma_tool_gyroscope_class_init (LigmaToolGyroscopeClass *klass)
{
  GObjectClass        *object_class = G_OBJECT_CLASS (klass);
  LigmaToolWidgetClass *widget_class = LIGMA_TOOL_WIDGET_CLASS (klass);

  object_class->set_property    = ligma_tool_gyroscope_set_property;
  object_class->get_property    = ligma_tool_gyroscope_get_property;

  widget_class->button_press    = ligma_tool_gyroscope_button_press;
  widget_class->button_release  = ligma_tool_gyroscope_button_release;
  widget_class->motion          = ligma_tool_gyroscope_motion;
  widget_class->hit             = ligma_tool_gyroscope_hit;
  widget_class->hover           = ligma_tool_gyroscope_hover;
  widget_class->key_press       = ligma_tool_gyroscope_key_press;
  widget_class->motion_modifier = ligma_tool_gyroscope_motion_modifier;
  widget_class->get_cursor      = ligma_tool_gyroscope_get_cursor;

  g_object_class_install_property (object_class, PROP_YAW,
                                   g_param_spec_double ("yaw", NULL, NULL,
                                                        -G_MAXDOUBLE,
                                                        +G_MAXDOUBLE,
                                                        0.0,
                                                        LIGMA_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT));

  g_object_class_install_property (object_class, PROP_PITCH,
                                   g_param_spec_double ("pitch", NULL, NULL,
                                                        -G_MAXDOUBLE,
                                                        +G_MAXDOUBLE,
                                                        0.0,
                                                        LIGMA_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT));

  g_object_class_install_property (object_class, PROP_ROLL,
                                   g_param_spec_double ("roll", NULL, NULL,
                                                        -G_MAXDOUBLE,
                                                        +G_MAXDOUBLE,
                                                        0.0,
                                                        LIGMA_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT));

  g_object_class_install_property (object_class, PROP_ZOOM,
                                   g_param_spec_double ("zoom", NULL, NULL,
                                                        0.0,
                                                        +G_MAXDOUBLE,
                                                        1.0,
                                                        LIGMA_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT));

  g_object_class_install_property (object_class, PROP_INVERT,
                                   g_param_spec_boolean ("invert", NULL, NULL,
                                                         FALSE,
                                                         LIGMA_PARAM_READWRITE |
                                                         G_PARAM_CONSTRUCT));

  g_object_class_install_property (object_class, PROP_SPEED,
                                   g_param_spec_double ("speed", NULL, NULL,
                                                        -G_MAXDOUBLE,
                                                        +G_MAXDOUBLE,
                                                        1.0,
                                                        LIGMA_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT));

  g_object_class_install_property (object_class, PROP_PIVOT_X,
                                   g_param_spec_double ("pivot-x", NULL, NULL,
                                                        -G_MAXDOUBLE,
                                                        +G_MAXDOUBLE,
                                                        0.0,
                                                        LIGMA_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT));

  g_object_class_install_property (object_class, PROP_PIVOT_Y,
                                   g_param_spec_double ("pivot-y", NULL, NULL,
                                                        -G_MAXDOUBLE,
                                                        +G_MAXDOUBLE,
                                                        0.0,
                                                        LIGMA_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT));
}

static void
ligma_tool_gyroscope_init (LigmaToolGyroscope *gyroscope)
{
  gyroscope->private = ligma_tool_gyroscope_get_instance_private (gyroscope);
}

static void
ligma_tool_gyroscope_set_property (GObject      *object,
                                  guint         property_id,
                                  const GValue *value,
                                  GParamSpec   *pspec)
{
  LigmaToolGyroscope        *gyroscope = LIGMA_TOOL_GYROSCOPE (object);
  LigmaToolGyroscopePrivate *private = gyroscope->private;

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
ligma_tool_gyroscope_get_property (GObject    *object,
                                  guint       property_id,
                                  GValue     *value,
                                  GParamSpec *pspec)
{
  LigmaToolGyroscope        *gyroscope = LIGMA_TOOL_GYROSCOPE (object);
  LigmaToolGyroscopePrivate *private = gyroscope->private;

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
ligma_tool_gyroscope_button_press (LigmaToolWidget      *widget,
                                  const LigmaCoords    *coords,
                                  guint32              time,
                                  GdkModifierType      state,
                                  LigmaButtonPressType  press_type)
{
  LigmaToolGyroscope        *gyroscope = LIGMA_TOOL_GYROSCOPE (widget);
  LigmaToolGyroscopePrivate *private   = gyroscope->private;

  ligma_tool_gyroscope_save (gyroscope);

  if (state & GDK_MOD1_MASK)
    {
      private->mode = MODE_ZOOM;

      private->last_zoom = private->zoom;
    }
  else if (state & ligma_get_extend_selection_mask ())
    {
      private->mode = MODE_ROTATE;

      private->last_angle = atan2 (coords->y - private->pivot_y,
                                   coords->x - private->pivot_x);
      private->curr_angle = private->last_angle;
    }
  else
    {
      private->mode = MODE_PAN;

      if (state & ligma_get_constrain_behavior_mask ())
        private->constraint = CONSTRAINT_UNKNOWN;
      else
        private->constraint = CONSTRAINT_NONE;
    }

  private->last_x = coords->x;
  private->last_y = coords->y;

  ligma_tool_gyroscope_update_status (gyroscope, state);

  return 1;
}

static void
ligma_tool_gyroscope_button_release (LigmaToolWidget        *widget,
                                    const LigmaCoords      *coords,
                                    guint32                time,
                                    GdkModifierType        state,
                                    LigmaButtonReleaseType  release_type)
{
  LigmaToolGyroscope        *gyroscope = LIGMA_TOOL_GYROSCOPE (widget);
  LigmaToolGyroscopePrivate *private   = gyroscope->private;

  if (release_type == LIGMA_BUTTON_RELEASE_CANCEL)
    ligma_tool_gyroscope_restore (gyroscope);

  private->mode = MODE_NONE;

  ligma_tool_gyroscope_update_status (gyroscope, state);
}

static void
ligma_tool_gyroscope_motion (LigmaToolWidget   *widget,
                            const LigmaCoords *coords,
                            guint32           time,
                            GdkModifierType   state)
{
  LigmaToolGyroscope        *gyroscope = LIGMA_TOOL_GYROSCOPE (widget);
  LigmaToolGyroscopePrivate *private   = gyroscope->private;
  LigmaDisplayShell         *shell     = ligma_tool_widget_get_shell (widget);
  LigmaVector3               axis      = {};

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
              ligma_display_shell_rotate_xy_f (shell, x1, y1, &x1, &y1);
              ligma_display_shell_rotate_xy_f (shell, x2, y2, &x2, &y2);

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

              ligma_display_shell_unrotate_xy_f (shell, x1, y1, &x1, &y1);
              ligma_display_shell_unrotate_xy_f (shell, x2, y2, &x2, &y2);
            }

          if (private->invert)
            factor = 1.0 / factor;

          ligma_vector3_set (&axis, y2 - y1, x2 - x1, 0.0);
          ligma_vector3_mul (&axis, factor * private->speed);
        }
      break;

    case MODE_ROTATE:
      {
        gdouble angle;

        angle = atan2 (coords->y - private->pivot_y,
                       coords->x - private->pivot_x);

        private->curr_angle = angle;

        angle -= private->last_angle;

        if (state & ligma_get_constrain_behavior_mask ())
          angle = RINT (angle / (G_PI / 6.0)) * (G_PI / 6.0);

        ligma_vector3_set (&axis, 0.0, 0.0, angle);

        private->last_angle += angle;
      }
    break;

    case MODE_ZOOM:
      {
        gdouble x1, y1;
        gdouble x2, y2;
        gdouble zoom;

        ligma_display_shell_transform_xy_f (shell,
                                           private->last_x, private->last_y,
                                           &x1,             &y1);
        ligma_display_shell_transform_xy_f (shell,
                                           coords->x,       coords->y,
                                           &x2,             &y2);

        zoom = (y1 - y2) * shell->scale_y / 128.0;

        if (private->invert)
          zoom = -zoom;

        private->last_zoom *= pow (2.0, zoom);

        zoom = log (private->last_zoom / private->zoom) / G_LN2;

        if (state & ligma_get_constrain_behavior_mask ())
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

  ligma_tool_gyroscope_rotate (gyroscope, &axis);
}

static LigmaHit
ligma_tool_gyroscope_hit (LigmaToolWidget   *widget,
                         const LigmaCoords *coords,
                         GdkModifierType   state,
                         gboolean          proximity)
{
  return LIGMA_HIT_INDIRECT;
}

static void
ligma_tool_gyroscope_hover (LigmaToolWidget   *widget,
                           const LigmaCoords *coords,
                           GdkModifierType   state,
                           gboolean          proximity)
{
  ligma_tool_gyroscope_update_status (LIGMA_TOOL_GYROSCOPE (widget), state);
}

static gboolean
ligma_tool_gyroscope_key_press (LigmaToolWidget *widget,
                               GdkEventKey    *kevent)
{
  LigmaToolGyroscope        *gyroscope = LIGMA_TOOL_GYROSCOPE (widget);
  LigmaToolGyroscopePrivate *private   = gyroscope->private;
  LigmaDisplayShell         *shell     = ligma_tool_widget_get_shell (widget);
  LigmaVector3               axis      = {};
  gboolean                  fast;
  gboolean                  result    = FALSE;

  fast = (kevent->state & ligma_get_constrain_behavior_mask ());

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
  else if (kevent->state & ligma_get_extend_selection_mask ())
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

      ligma_vector3_set (&axis, 0.0, 0.0, angle * DEG_TO_RAD);
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

      ligma_display_shell_unrotate_xy_f (shell, x0, y0, &x0, &y0);
      ligma_display_shell_unrotate_xy_f (shell, x,  y,  &x,  &y);

      ligma_vector3_set (&axis,
                        (y - y0) * DEG_TO_RAD,
                        (x - x0) * DEG_TO_RAD,
                        0.0);
      ligma_vector3_mul (&axis, factor);
    }

  ligma_tool_gyroscope_rotate (gyroscope, &axis);

  return result;
}

static void
ligma_tool_gyroscope_motion_modifier (LigmaToolWidget  *widget,
                                     GdkModifierType  key,
                                     gboolean         press,
                                     GdkModifierType  state)
{
  LigmaToolGyroscope        *gyroscope = LIGMA_TOOL_GYROSCOPE (widget);
  LigmaToolGyroscopePrivate *private   = gyroscope->private;

  ligma_tool_gyroscope_update_status (gyroscope, state);

  if (key == ligma_get_constrain_behavior_mask ())
    {
      switch (private->mode)
      {
        case MODE_PAN:
          if (state & ligma_get_constrain_behavior_mask ())
            private->constraint = CONSTRAINT_UNKNOWN;
          else
            private->constraint = CONSTRAINT_NONE;
          break;

        case MODE_ROTATE:
          if (! (state & ligma_get_constrain_behavior_mask ()))
            private->last_angle = private->curr_angle;
          break;

        case MODE_ZOOM:
          if (! (state & ligma_get_constrain_behavior_mask ()))
            private->last_zoom = private->zoom;
          break;

        case MODE_NONE:
          break;
      }
    }
}

static gboolean
ligma_tool_gyroscope_get_cursor (LigmaToolWidget     *widget,
                                const LigmaCoords   *coords,
                                GdkModifierType     state,
                                LigmaCursorType     *cursor,
                                LigmaToolCursorType *tool_cursor,
                                LigmaCursorModifier *modifier)
{
  if (state & GDK_MOD1_MASK)
    *modifier = LIGMA_CURSOR_MODIFIER_ZOOM;
  else if (state & ligma_get_extend_selection_mask ())
    *modifier = LIGMA_CURSOR_MODIFIER_ROTATE;
  else
    *modifier = LIGMA_CURSOR_MODIFIER_MOVE;

  return TRUE;
}

static void
ligma_tool_gyroscope_update_status (LigmaToolGyroscope *gyroscope,
                                   GdkModifierType    state)
{
  LigmaToolGyroscopePrivate *private = gyroscope->private;
  gchar                    *status;

  if (private->mode == MODE_ZOOM ||
      (private->mode == MODE_NONE &&
       state & GDK_MOD1_MASK))
    {
      status = ligma_suggest_modifiers (_("Click-Drag to zoom"),
                                       ligma_get_toggle_behavior_mask () &
                                       ~state,
                                       NULL,
                                       _("%s for constrained steps"),
                                       NULL);
    }
  else if (private->mode == MODE_ROTATE ||
           (private->mode == MODE_NONE &&
            state & ligma_get_extend_selection_mask ()))
    {
      status = ligma_suggest_modifiers (_("Click-Drag to rotate"),
                                       ligma_get_toggle_behavior_mask () &
                                       ~state,
                                       NULL,
                                       _("%s for constrained angles"),
                                       NULL);
    }
  else
    {
      status = ligma_suggest_modifiers (_("Click-Drag to pan"),
                                       (((ligma_get_extend_selection_mask () |
                                          GDK_MOD1_MASK)                    *
                                         (private->mode == MODE_NONE))      |
                                        ligma_get_toggle_behavior_mask  ())  &
                                        ~state,
                                       _("%s to rotate"),
                                       _("%s for a constrained axis"),
                                       _("%s to zoom"));
    }

  ligma_tool_widget_set_status (LIGMA_TOOL_WIDGET (gyroscope), status);

  g_free (status);
}

static void
ligma_tool_gyroscope_save (LigmaToolGyroscope *gyroscope)
{
  LigmaToolGyroscopePrivate *private = gyroscope->private;

  private->orig_yaw   = private->yaw;
  private->orig_pitch = private->pitch;
  private->orig_roll  = private->roll;
  private->orig_zoom  = private->zoom;
}

static void
ligma_tool_gyroscope_restore (LigmaToolGyroscope *gyroscope)
{
  LigmaToolGyroscopePrivate *private = gyroscope->private;

  g_object_set (gyroscope,
                "yaw",   private->orig_yaw,
                "pitch", private->orig_pitch,
                "roll",  private->orig_roll,
                "zoom",  private->orig_zoom,
                NULL);
}

static void
ligma_tool_gyroscope_rotate (LigmaToolGyroscope *gyroscope,
                            const LigmaVector3 *axis)
{
  LigmaToolGyroscopePrivate *private = gyroscope->private;
  LigmaVector3               real_axis;
  LigmaVector3               basis[2];
  gdouble                   yaw;
  gdouble                   pitch;
  gdouble                   roll;
  gint                      i;

  if (ligma_vector3_length (axis) < EPSILON)
    return;

  real_axis = *axis;

  if (private->invert)
    ligma_vector3_neg (&real_axis);

  for (i = 0; i < 2; i++)
    {
      ligma_vector3_set (&basis[i], i == 0, i == 1, 0.0);

      if (private->invert)
        ligma_tool_gyroscope_rotate_vector (&basis[i], &real_axis);

      ligma_tool_gyroscope_rotate_vector (
        &basis[i], &(LigmaVector3) {0.0, private->yaw * DEG_TO_RAD, 0.0});
      ligma_tool_gyroscope_rotate_vector (
        &basis[i], &(LigmaVector3) {private->pitch * DEG_TO_RAD, 0.0, 0.0});
      ligma_tool_gyroscope_rotate_vector (
        &basis[i], &(LigmaVector3) {0.0, 0.0, private->roll * DEG_TO_RAD});

      if (! private->invert)
        ligma_tool_gyroscope_rotate_vector (&basis[i], &real_axis);
    }

  roll = atan2 (basis[1].x, basis[1].y);

  for (i = 0; i < 2; i++)
    {
      ligma_tool_gyroscope_rotate_vector (
        &basis[i], &(LigmaVector3) {0.0, 0.0, -roll});
    }

  pitch = atan2 (-basis[1].z, basis[1].y);

  for (i = 0; i < 1; i++)
    {
      ligma_tool_gyroscope_rotate_vector (
        &basis[i], &(LigmaVector3) {-pitch, 0.0, 0.0});
    }

  yaw = atan2 (basis[0].z, basis[0].x);

  g_object_set (gyroscope,
                "yaw",   yaw   / DEG_TO_RAD,
                "pitch", pitch / DEG_TO_RAD,
                "roll",  roll  / DEG_TO_RAD,
                NULL);
}

static void
ligma_tool_gyroscope_rotate_vector (LigmaVector3       *vector,
                                   const LigmaVector3 *axis)
{
  LigmaVector3 normalized_axis;
  LigmaVector3 projection;
  LigmaVector3 u;
  LigmaVector3 v;
  gdouble     angle;

  angle = ligma_vector3_length (axis);

  if (angle < EPSILON)
    return;

  normalized_axis = ligma_vector3_mul_val (*axis, 1.0 / angle);

  projection = ligma_vector3_mul_val (
    normalized_axis,
    ligma_vector3_inner_product (vector, &normalized_axis));

  u = ligma_vector3_sub_val (*vector, projection);
  v = ligma_vector3_cross_product (&u, &normalized_axis);

  ligma_vector3_mul (&u, cos (angle));
  ligma_vector3_mul (&v, sin (angle));

  ligma_vector3_add (vector, &u, &v);
  ligma_vector3_add (vector, vector, &projection);
}


/*  public functions  */


LigmaToolWidget *
ligma_tool_gyroscope_new (LigmaDisplayShell *shell)
{
  g_return_val_if_fail (LIGMA_IS_DISPLAY_SHELL (shell), NULL);

  return g_object_new (LIGMA_TYPE_TOOL_GYROSCOPE,
                       "shell", shell,
                       NULL);
}
