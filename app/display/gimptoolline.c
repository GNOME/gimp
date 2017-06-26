/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimptoolline.c
 * Copyright (C) 2017 Michael Natterer <mitch@gimp.org>
 *
 * Major improvements for interactivity
 * Copyright (C) 2014 Michael Henning <drawoc@darkrefraction.com>
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

#include "widgets/gimpwidgets-utils.h"

#include "gimpcanvashandle.h"
#include "gimpcanvasline.h"
#include "gimpdisplayshell.h"
#include "gimptoolline.h"

#include "gimp-intl.h"


#define SHOW_LINE TRUE


typedef enum
{
  /* POINT_NONE evaluates to FALSE */
  POINT_NONE = 0,
  POINT_START,
  POINT_END,
  POINT_BOTH
} GimpToolLinePoint;

enum
{
  PROP_0,
  PROP_X1,
  PROP_Y1,
  PROP_X2,
  PROP_Y2
};

struct _GimpToolLinePrivate
{
  gdouble            x1;
  gdouble            y1;
  gdouble            x2;
  gdouble            y2;

  gdouble            saved_x1;
  gdouble            saved_y1;
  gdouble            saved_x2;
  gdouble            saved_y2;

  gdouble            mouse_x;
  gdouble            mouse_y;
  GimpToolLinePoint  point;
  gboolean           point_grabbed;

  GimpCanvasItem    *line;
  GimpCanvasItem    *start_handle_circle;
  GimpCanvasItem    *start_handle_cross;
  GimpCanvasItem    *end_handle_circle;
  GimpCanvasItem    *end_handle_cross;
};


/*  local function prototypes  */

static void     gimp_tool_line_constructed     (GObject               *object);
static void     gimp_tool_line_set_property    (GObject               *object,
                                                guint                  property_id,
                                                const GValue          *value,
                                                GParamSpec            *pspec);
static void     gimp_tool_line_get_property    (GObject               *object,
                                                guint                  property_id,
                                                GValue                *value,
                                                GParamSpec            *pspec);

static void     gimp_tool_line_changed         (GimpToolWidget        *widget);
static gint     gimp_tool_line_button_press    (GimpToolWidget        *widget,
                                                const GimpCoords      *coords,
                                                guint32                time,
                                                GdkModifierType        state,
                                                GimpButtonPressType    press_type);
static void     gimp_tool_line_button_release  (GimpToolWidget        *widget,
                                                const GimpCoords      *coords,
                                                guint32                time,
                                                GdkModifierType        state,
                                                GimpButtonReleaseType  release_type);
static void     gimp_tool_line_motion          (GimpToolWidget        *widget,
                                                const GimpCoords      *coords,
                                                guint32                time,
                                                GdkModifierType        state);
static void     gimp_tool_line_hover           (GimpToolWidget        *widget,
                                                const GimpCoords      *coords,
                                                GdkModifierType        state,
                                                gboolean               proximity);
static void     gimp_tool_line_motion_modifier (GimpToolWidget        *widget,
                                                GdkModifierType        key,
                                                gboolean               press,
                                                GdkModifierType        state);
static gboolean gimp_tool_line_get_cursor      (GimpToolWidget        *widget,
                                                const GimpCoords      *coords,
                                                GdkModifierType        state,
                                                GimpCursorType        *cursor,
                                                GimpToolCursorType    *tool_cursor,
                                                GimpCursorModifier    *cursor_modifier);

static gboolean gimp_tool_line_point_motion    (GimpToolLine          *line,
                                                gboolean               constrain_angle);

static void     gimp_tool_line_update_handles  (GimpToolLine          *line);
static void     gimp_tool_line_update_hilight  (GimpToolLine          *line);
static void     gimp_tool_line_update_status   (GimpToolLine          *line,
                                                GdkModifierType        state,
                                                gboolean               proximity);


G_DEFINE_TYPE (GimpToolLine, gimp_tool_line, GIMP_TYPE_TOOL_WIDGET)

#define parent_class gimp_tool_line_parent_class


static void
gimp_tool_line_class_init (GimpToolLineClass *klass)
{
  GObjectClass        *object_class = G_OBJECT_CLASS (klass);
  GimpToolWidgetClass *widget_class = GIMP_TOOL_WIDGET_CLASS (klass);

  object_class->constructed     = gimp_tool_line_constructed;
  object_class->set_property    = gimp_tool_line_set_property;
  object_class->get_property    = gimp_tool_line_get_property;

  widget_class->changed         = gimp_tool_line_changed;
  widget_class->button_press    = gimp_tool_line_button_press;
  widget_class->button_release  = gimp_tool_line_button_release;
  widget_class->motion          = gimp_tool_line_motion;
  widget_class->hover           = gimp_tool_line_hover;
  widget_class->motion_modifier = gimp_tool_line_motion_modifier;
  widget_class->get_cursor      = gimp_tool_line_get_cursor;

  g_object_class_install_property (object_class, PROP_X1,
                                   g_param_spec_double ("x1", NULL, NULL,
                                                        -GIMP_MAX_IMAGE_SIZE,
                                                        GIMP_MAX_IMAGE_SIZE, 0,
                                                        GIMP_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT));

  g_object_class_install_property (object_class, PROP_Y1,
                                   g_param_spec_double ("y1", NULL, NULL,
                                                        -GIMP_MAX_IMAGE_SIZE,
                                                        GIMP_MAX_IMAGE_SIZE, 0,
                                                        GIMP_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT));

  g_object_class_install_property (object_class, PROP_X2,
                                   g_param_spec_double ("x2", NULL, NULL,
                                                        -GIMP_MAX_IMAGE_SIZE,
                                                        GIMP_MAX_IMAGE_SIZE, 0,
                                                        GIMP_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT));

  g_object_class_install_property (object_class, PROP_Y2,
                                   g_param_spec_double ("y2", NULL, NULL,
                                                        -GIMP_MAX_IMAGE_SIZE,
                                                        GIMP_MAX_IMAGE_SIZE, 0,
                                                        GIMP_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT));

  g_type_class_add_private (klass, sizeof (GimpToolLinePrivate));
}

static void
gimp_tool_line_init (GimpToolLine *line)
{
  line->private = G_TYPE_INSTANCE_GET_PRIVATE (line,
                                               GIMP_TYPE_TOOL_LINE,
                                               GimpToolLinePrivate);
}

static void
gimp_tool_line_constructed (GObject *object)
{
  GimpToolLine        *line    = GIMP_TOOL_LINE (object);
  GimpToolWidget      *widget  = GIMP_TOOL_WIDGET (object);
  GimpToolLinePrivate *private = line->private;

  G_OBJECT_CLASS (parent_class)->constructed (object);

  private->line = gimp_tool_widget_add_line (widget,
                                             private->x1,
                                             private->y1,
                                             private->x2,
                                             private->y2);

  gimp_canvas_item_set_visible (private->line, SHOW_LINE);

  private->start_handle_circle =
    gimp_tool_widget_add_handle (widget,
                                 GIMP_HANDLE_CIRCLE,
                                 private->x1,
                                 private->y1,
                                 2 * GIMP_CANVAS_HANDLE_SIZE_CROSS,
                                 2 * GIMP_CANVAS_HANDLE_SIZE_CROSS,
                                 GIMP_HANDLE_ANCHOR_CENTER);

  private->start_handle_cross =
    gimp_tool_widget_add_handle (widget,
                                 GIMP_HANDLE_CROSS,
                                 private->x1,
                                 private->y1,
                                 GIMP_CANVAS_HANDLE_SIZE_CROSS,
                                 GIMP_CANVAS_HANDLE_SIZE_CROSS,
                                 GIMP_HANDLE_ANCHOR_CENTER);

  private->end_handle_circle =
    gimp_tool_widget_add_handle (widget,
                                 GIMP_HANDLE_CIRCLE,
                                 private->x2,
                                 private->y2,
                                 2 * GIMP_CANVAS_HANDLE_SIZE_CROSS,
                                 2 * GIMP_CANVAS_HANDLE_SIZE_CROSS,
                                 GIMP_HANDLE_ANCHOR_CENTER);

  private->end_handle_cross =
    gimp_tool_widget_add_handle (widget,
                                 GIMP_HANDLE_CROSS,
                                 private->x2,
                                 private->y2,
                                 GIMP_CANVAS_HANDLE_SIZE_CROSS,
                                 GIMP_CANVAS_HANDLE_SIZE_CROSS,
                                 GIMP_HANDLE_ANCHOR_CENTER);

  gimp_tool_line_changed (widget);
}

static void
gimp_tool_line_set_property (GObject      *object,
                             guint         property_id,
                             const GValue *value,
                             GParamSpec   *pspec)
{
  GimpToolLine        *line    = GIMP_TOOL_LINE (object);
  GimpToolLinePrivate *private = line->private;

  switch (property_id)
    {
    case PROP_X1:
      private->x1 = g_value_get_double (value);
      break;
    case PROP_Y1:
      private->y1 = g_value_get_double (value);
      break;
    case PROP_X2:
      private->x2 = g_value_get_double (value);
      break;
    case PROP_Y2:
      private->y2 = g_value_get_double (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_tool_line_get_property (GObject    *object,
                             guint       property_id,
                             GValue     *value,
                             GParamSpec *pspec)
{
  GimpToolLine        *line    = GIMP_TOOL_LINE (object);
  GimpToolLinePrivate *private = line->private;

  switch (property_id)
    {
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

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_tool_line_changed (GimpToolWidget *widget)
{
  GimpToolLine        *line    = GIMP_TOOL_LINE (widget);
  GimpToolLinePrivate *private = line->private;

  gimp_canvas_line_set (private->line,
                        private->x1,
                        private->y1,
                        private->x2,
                        private->y2);

  gimp_canvas_handle_set_position (private->start_handle_circle,
                                   private->x1,
                                   private->y1);
  gimp_canvas_handle_set_position (private->start_handle_cross,
                                   private->x1,
                                   private->y1);

  gimp_canvas_handle_set_position (private->end_handle_circle,
                                   private->x2,
                                   private->y2);
  gimp_canvas_handle_set_position (private->end_handle_cross,
                                   private->x2,
                                   private->y2);

  gimp_tool_line_update_handles (line);
  gimp_tool_line_update_hilight (line);
}

gboolean
gimp_tool_line_button_press (GimpToolWidget      *widget,
                             const GimpCoords    *coords,
                             guint32              time,
                             GdkModifierType      state,
                             GimpButtonPressType  press_type)
{
  GimpToolLine        *line    = GIMP_TOOL_LINE (widget);
  GimpToolLinePrivate *private = line->private;

  if (private->point != POINT_NONE)
    {
      private->saved_x1 = private->x1;
      private->saved_y1 = private->y1;
      private->saved_x2 = private->x2;
      private->saved_y2 = private->y2;

      private->point_grabbed = TRUE;

      gimp_tool_line_point_motion (line,
                                   state & gimp_get_constrain_behavior_mask ());

      return private->point;
    }

  gimp_tool_line_update_status (line, state, TRUE);

  return 0;
}

void
gimp_tool_line_button_release (GimpToolWidget        *widget,
                               const GimpCoords      *coords,
                               guint32                time,
                               GdkModifierType        state,
                               GimpButtonReleaseType  release_type)
{
  GimpToolLine        *line    = GIMP_TOOL_LINE (widget);
  GimpToolLinePrivate *private = line->private;

  if (release_type == GIMP_BUTTON_RELEASE_CANCEL)
    {
      g_object_set (line,
                    "x1", private->saved_x1,
                    "y1", private->saved_y1,
                    "x2", private->saved_x2,
                    "y2", private->saved_y2,
                    NULL);
    }

  private->point_grabbed = FALSE;
}

void
gimp_tool_line_motion (GimpToolWidget   *widget,
                       const GimpCoords *coords,
                       guint32           time,
                       GdkModifierType   state)
{
  GimpToolLine        *line    = GIMP_TOOL_LINE (widget);
  GimpToolLinePrivate *private = line->private;
  gdouble              diff_x  = coords->x - private->mouse_x;
  gdouble              diff_y  = coords->y - private->mouse_y;

  private->mouse_x = coords->x;
  private->mouse_y = coords->y;

  if (private->point == POINT_BOTH)
    {
      g_object_set (line,
                    "x1", private->x1 + diff_x,
                    "y1", private->y1 + diff_y,
                    "x2", private->x2 + diff_x,
                    "y2", private->y2 + diff_y,
                    NULL);
    }
  else
    {
      gboolean constrain = (state & gimp_get_constrain_behavior_mask ()) != 0;

      gimp_tool_line_point_motion (line, constrain);
    }

  gimp_tool_line_update_status (line, state, TRUE);
}

void
gimp_tool_line_hover (GimpToolWidget   *widget,
                      const GimpCoords *coords,
                      GdkModifierType   state,
                      gboolean          proximity)
{
  GimpToolLine        *line    = GIMP_TOOL_LINE (widget);
  GimpToolLinePrivate *private = line->private;

  private->mouse_x = coords->x;
  private->mouse_y = coords->y;

  gimp_tool_line_update_handles (line);

  if (state & GDK_MOD1_MASK)
    {
      private->point = POINT_BOTH;
    }
  else if (gimp_canvas_item_hit (private->end_handle_circle,
                                 private->mouse_x,
                                 private->mouse_y))
    {
      private->point = POINT_END;
    }
  else if (gimp_canvas_item_hit (private->start_handle_circle,
                                 private->mouse_x,
                                 private->mouse_y))
    {
      private->point = POINT_START;
    }
  else
    {
      private->point = POINT_NONE;
    }

  gimp_tool_line_update_hilight (line);
  gimp_tool_line_update_status (line, state, proximity);
}

static void
gimp_tool_line_motion_modifier (GimpToolWidget  *widget,
                                GdkModifierType  key,
                                gboolean         press,
                                GdkModifierType  state)
{
  GimpToolLine *line = GIMP_TOOL_LINE (widget);

  if (key == gimp_get_constrain_behavior_mask ())
    {
      gimp_tool_line_point_motion (line, press);

      gimp_tool_line_update_status (line, state, TRUE);
    }
}

static gboolean
gimp_tool_line_get_cursor (GimpToolWidget     *widget,
                           const GimpCoords   *coords,
                           GdkModifierType     state,
                           GimpCursorType     *cursor,
                           GimpToolCursorType *tool_cursor,
                           GimpCursorModifier *cursor_modifier)
{
  GimpToolLine        *line    = GIMP_TOOL_LINE (widget);
  GimpToolLinePrivate *private = line->private;

  if (private->point == POINT_BOTH)
    {
      *cursor_modifier = GIMP_CURSOR_MODIFIER_MOVE;

      return TRUE;
    }

  return FALSE;
}

static gboolean
gimp_tool_line_point_motion (GimpToolLine *line,
                             gboolean      constrain_angle)
{
  GimpToolLinePrivate *private = line->private;
  gdouble              x       = private->mouse_x;
  gdouble              y       = private->mouse_y;

  switch (private->point)
    {
    case POINT_START:
      if (constrain_angle)
        gimp_constrain_line (private->x2, private->y2,
                             &x, &y,
                             GIMP_CONSTRAIN_LINE_15_DEGREES);

      g_object_set (line,
                    "x1", x,
                    "y1", y,
                    NULL);
      return TRUE;

    case POINT_END:
      if (constrain_angle)
        gimp_constrain_line (private->x1, private->y1,
                             &x, &y,
                             GIMP_CONSTRAIN_LINE_15_DEGREES);

      g_object_set (line,
                    "x2", x,
                    "y2", y,
                    NULL);
      return TRUE;

    default:
      break;
    }

  return FALSE;
}

static void
gimp_tool_line_update_handles (GimpToolLine *line)
{
  GimpToolLinePrivate *private = line->private;
  gboolean             start_visible,  end_visible;
  gint                 start_diameter, end_diameter;

  /* Calculate handle visibility */
  if (private->point_grabbed)
    {
      start_visible = FALSE;
      end_visible   = FALSE;
    }
  else
    {
      start_diameter = gimp_canvas_handle_calc_size (private->start_handle_circle,
                                                     private->mouse_x,
                                                     private->mouse_y,
                                                     0,
                                                     2 * GIMP_CANVAS_HANDLE_SIZE_CROSS);
      start_visible = start_diameter > 2;

      end_diameter = gimp_canvas_handle_calc_size (private->end_handle_circle,
                                                   private->mouse_x,
                                                   private->mouse_y,
                                                   0,
                                                   2 * GIMP_CANVAS_HANDLE_SIZE_CROSS);
      end_visible = end_diameter > 2;
    }

  gimp_canvas_item_set_visible (private->start_handle_circle, start_visible);
  gimp_canvas_item_set_visible (private->end_handle_circle,   end_visible);

  if (start_visible)
    gimp_canvas_handle_set_size (private->start_handle_circle,
                                 start_diameter, start_diameter);

  if (end_visible)
    gimp_canvas_handle_set_size (private->end_handle_circle,
                                 end_diameter, end_diameter);
}

static void
gimp_tool_line_update_hilight (GimpToolLine *line)
{
  GimpToolLinePrivate *private = line->private;

  gimp_canvas_item_set_highlight (private->start_handle_circle,
                                  private->point == POINT_START);
  gimp_canvas_item_set_highlight (private->start_handle_cross,
                                  private->point == POINT_START);

  gimp_canvas_item_set_highlight (private->end_handle_circle,
                                  private->point == POINT_END);
  gimp_canvas_item_set_highlight (private->end_handle_cross,
                                  private->point == POINT_END);
}

static void
gimp_tool_line_update_status (GimpToolLine    *line,
                              GdkModifierType  state,
                              gboolean         proximity)
{
  GimpToolLinePrivate *private = line->private;

  if (proximity)
    {
      gchar *status_help =
        gimp_suggest_modifiers ("",
                                (gimp_get_constrain_behavior_mask () |
                                 GDK_MOD1_MASK) &
                                ~state,
                                NULL,
                                _("%s for constrained angles"),
                                _("%s to move the whole line"));

      gimp_tool_widget_set_status_coords (GIMP_TOOL_WIDGET (line),
                                          _("Blend: "),
                                          private->x2 - private->x1,
                                          ", ",
                                          private->y2 - private->y1,
                                          status_help);

      g_free (status_help);
    }
  else
    {
      gimp_tool_widget_set_status (GIMP_TOOL_WIDGET (line), NULL);
    }
}


/*  public functions  */

GimpToolWidget *
gimp_tool_line_new (GimpDisplayShell *shell,
                    gdouble           x1,
                    gdouble           y1,
                    gdouble           x2,
                    gdouble           y2)
{
  g_return_val_if_fail (GIMP_IS_DISPLAY_SHELL (shell), NULL);

  return g_object_new (GIMP_TYPE_TOOL_LINE,
                       "shell", shell,
                       "x1",    x1,
                       "y1",    y1,
                       "x2",    x2,
                       "y2",    y2,
                       NULL);
}
