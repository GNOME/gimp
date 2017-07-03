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
#define ENDPOINT_GRIP_HANDLE_TYPE GIMP_HANDLE_CROSS
#define ENDPOINT_GRIP_HANDLE_SIZE GIMP_CANVAS_HANDLE_SIZE_CROSS
#define SLIDER_GRIP_HANDLE_TYPE   GIMP_HANDLE_FILLED_DIAMOND
#define SLIDER_GRIP_HANDLE_SIZE   (ENDPOINT_GRIP_HANDLE_SIZE * 2 / 3)


typedef enum
{
  /* POINT_NONE evaluates to FALSE */
  POINT_NONE = 0,
  POINT_START,
  POINT_END,
  POINT_BOTH,
  POINT_SLIDER
} GimpToolLinePoint;

enum
{
  PROP_0,
  PROP_X1,
  PROP_Y1,
  PROP_X2,
  PROP_Y2,
  PROP_SLIDERS,
  PROP_STATUS_TITLE,
};

struct _GimpToolLinePrivate
{
  gdouble            x1;
  gdouble            y1;
  gdouble            x2;
  gdouble            y2;
  GArray            *sliders;
  gchar             *status_title;

  gdouble            saved_x1;
  gdouble            saved_y1;
  gdouble            saved_x2;
  gdouble            saved_y2;
  gdouble            saved_slider_value;

  gdouble            mouse_x;
  gdouble            mouse_y;
  GimpToolLinePoint  point;
  gint               slider_index;
  gboolean           point_grabbed;

  GimpCanvasItem    *line;
  GimpCanvasItem    *start_handle_circle;
  GimpCanvasItem    *start_handle_grip;
  GimpCanvasItem    *end_handle_circle;
  GimpCanvasItem    *end_handle_grip;
  GArray            *slider_handle_circles;
  GArray            *slider_handle_grips;
};


/*  local function prototypes  */

static void     gimp_tool_line_constructed     (GObject               *object);
static void     gimp_tool_line_finalize        (GObject               *object);
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
                                                GimpCursorModifier    *modifier);

static gboolean gimp_tool_line_point_motion    (GimpToolLine          *line,
                                                gboolean               constrain);

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
  object_class->finalize        = gimp_tool_line_finalize;
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

  g_object_class_install_property (object_class, PROP_SLIDERS,
                                   g_param_spec_boxed ("sliders", NULL, NULL,
                                                       G_TYPE_ARRAY,
                                                       GIMP_PARAM_READWRITE));

  g_object_class_install_property (object_class, PROP_STATUS_TITLE,
                                   g_param_spec_string ("status-title",
                                                        NULL, NULL,
                                                        _("Line: "),
                                                        GIMP_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT));

  g_type_class_add_private (klass, sizeof (GimpToolLinePrivate));
}

static void
gimp_tool_line_init (GimpToolLine *line)
{
  GimpToolLinePrivate *private;
  
  private = line->private = G_TYPE_INSTANCE_GET_PRIVATE (line,
                                                         GIMP_TYPE_TOOL_LINE,
                                                         GimpToolLinePrivate);

  private->sliders = g_array_new (FALSE, FALSE, sizeof (GimpControllerSlider));

  private->slider_handle_circles = g_array_new (FALSE, TRUE,
                                                sizeof (GimpCanvasItem *));
  private->slider_handle_grips   = g_array_new (FALSE, TRUE,
                                                sizeof (GimpCanvasItem *));
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
                                 2 * ENDPOINT_GRIP_HANDLE_SIZE,
                                 2 * ENDPOINT_GRIP_HANDLE_SIZE,
                                 GIMP_HANDLE_ANCHOR_CENTER);

  private->start_handle_grip =
    gimp_tool_widget_add_handle (widget,
                                 ENDPOINT_GRIP_HANDLE_TYPE,
                                 private->x1,
                                 private->y1,
                                 ENDPOINT_GRIP_HANDLE_SIZE,
                                 ENDPOINT_GRIP_HANDLE_SIZE,
                                 GIMP_HANDLE_ANCHOR_CENTER);

  private->end_handle_circle =
    gimp_tool_widget_add_handle (widget,
                                 GIMP_HANDLE_CIRCLE,
                                 private->x2,
                                 private->y2,
                                 2 * ENDPOINT_GRIP_HANDLE_SIZE,
                                 2 * ENDPOINT_GRIP_HANDLE_SIZE,
                                 GIMP_HANDLE_ANCHOR_CENTER);

  private->end_handle_grip =
    gimp_tool_widget_add_handle (widget,
                                 ENDPOINT_GRIP_HANDLE_TYPE,
                                 private->x2,
                                 private->y2,
                                 ENDPOINT_GRIP_HANDLE_SIZE,
                                 ENDPOINT_GRIP_HANDLE_SIZE,
                                 GIMP_HANDLE_ANCHOR_CENTER);

  gimp_tool_line_changed (widget);
}

static void
gimp_tool_line_finalize (GObject *object)
{
  GimpToolLine        *line    = GIMP_TOOL_LINE (object);
  GimpToolLinePrivate *private = line->private;

  if (private->sliders)
    {
      g_array_unref (private->sliders);
      private->sliders = NULL;
    }

  if (private->status_title)
    {
      g_free (private->status_title);
      private->status_title = NULL;
    }

  if (private->slider_handle_circles)
    {
      g_array_unref (private->slider_handle_circles);
      private->slider_handle_circles = NULL;
    }

  if (private->slider_handle_grips)
    {
      g_array_unref (private->slider_handle_grips);
      private->slider_handle_grips = NULL;
    }

  G_OBJECT_CLASS (parent_class)->finalize (object);
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

    case PROP_SLIDERS:
      g_return_if_fail (g_value_get_boxed (value) != NULL);

      g_array_unref (private->sliders);
      private->sliders = g_value_dup_boxed (value);
      break;

    case PROP_STATUS_TITLE:
      g_free (private->status_title);
      private->status_title = g_value_dup_string (value);
      if (! private->status_title)
        private->status_title = g_strdup (_("Line: "));
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

    case PROP_SLIDERS:
      g_value_set_boxed (value, private->sliders);
      break;

    case PROP_STATUS_TITLE:
      g_value_set_string (value, private->status_title);
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
  gint                 i;

  gimp_canvas_line_set (private->line,
                        private->x1,
                        private->y1,
                        private->x2,
                        private->y2);

  gimp_canvas_handle_set_position (private->start_handle_circle,
                                   private->x1,
                                   private->y1);
  gimp_canvas_handle_set_position (private->start_handle_grip,
                                   private->x1,
                                   private->y1);

  gimp_canvas_handle_set_position (private->end_handle_circle,
                                   private->x2,
                                   private->y2);
  gimp_canvas_handle_set_position (private->end_handle_grip,
                                   private->x2,
                                   private->y2);

  /* remove excessive slider handles */
  for (i = private->sliders->len; i < private->slider_handle_circles->len; i++)
    {
      gimp_tool_widget_remove_item (widget,
                                    g_array_index (private->slider_handle_circles,
                                                   GimpCanvasItem *, i));
      gimp_tool_widget_remove_item (widget,
                                    g_array_index (private->slider_handle_grips,
                                                   GimpCanvasItem *, i));
    }

  g_array_set_size (private->slider_handle_circles, private->sliders->len);
  g_array_set_size (private->slider_handle_grips,   private->sliders->len);

  for (i = 0; i < private->sliders->len; i++)
    {
      gdouble          t;
      gdouble          x;
      gdouble          y;
      GimpCanvasItem **circle;
      GimpCanvasItem **grip;

      t = g_array_index (private->sliders, GimpControllerSlider, i).value;

      x = private->x1 + (private->x2 - private->x1) * t;
      y = private->y1 + (private->y2 - private->y1) * t;

      circle = &g_array_index (private->slider_handle_circles,
                               GimpCanvasItem *, i);
      grip   = &g_array_index (private->slider_handle_grips,
                               GimpCanvasItem *, i);

      if (*circle)
        {
          gimp_canvas_handle_set_position (*circle, x, y);
          gimp_canvas_handle_set_position (*grip,   x, y);
        }
      else
        {
          *circle = gimp_tool_widget_add_handle (widget,
                                                 GIMP_HANDLE_CIRCLE,
                                                 x,
                                                 y,
                                                 2 * SLIDER_GRIP_HANDLE_SIZE,
                                                 2 * SLIDER_GRIP_HANDLE_SIZE,
                                                 GIMP_HANDLE_ANCHOR_CENTER);
          *grip   = gimp_tool_widget_add_handle (widget,
                                                 SLIDER_GRIP_HANDLE_TYPE,
                                                 x,
                                                 y,
                                                 SLIDER_GRIP_HANDLE_SIZE,
                                                 SLIDER_GRIP_HANDLE_SIZE,
                                                 GIMP_HANDLE_ANCHOR_CENTER);
        }
    }

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

      if (private->point == POINT_SLIDER)
        {
          private->saved_slider_value =
            g_array_index (private->sliders,
                           GimpControllerSlider, private->slider_index).value;
        }

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
      if (private->point == POINT_SLIDER)
        {
          g_array_index (private->sliders,
                         GimpControllerSlider, private->slider_index).value =
            private->saved_slider_value;
        }

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
  gint                 i;

  private->mouse_x = coords->x;
  private->mouse_y = coords->y;

  gimp_tool_line_update_handles (line);

  private->point = POINT_NONE;

  if (state & GDK_MOD1_MASK)
    {
      private->point = POINT_BOTH;
    }
  else
    {
      /* give sliders precedence over the endpoints, since they're smaller */
      for (i = private->sliders->len - 1; i >= 0; i--)
        {
          GimpCanvasItem *circle;

          circle = g_array_index (private->slider_handle_circles,
                                  GimpCanvasItem *, i);

          if (gimp_canvas_item_hit (circle, private->mouse_x, private->mouse_y))
            {
              private->point        = POINT_SLIDER;
              private->slider_index = i;

              break;
            }
        }

      if (private->point == POINT_NONE)
        {
          if (gimp_canvas_item_hit (private->end_handle_circle,
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
        }
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
                           GimpCursorModifier *modifier)
{
  GimpToolLine        *line    = GIMP_TOOL_LINE (widget);
  GimpToolLinePrivate *private = line->private;

  if (private->point == POINT_BOTH)
    {
      *modifier = GIMP_CURSOR_MODIFIER_MOVE;

      return TRUE;
    }

  return FALSE;
}

static gboolean
gimp_tool_line_point_motion (GimpToolLine *line,
                             gboolean      constrain)
{
  GimpToolLinePrivate *private = line->private;
  gdouble              x       = private->mouse_x;
  gdouble              y       = private->mouse_y;

  switch (private->point)
    {
    case POINT_START:
      if (constrain)
        gimp_constrain_line (private->x2, private->y2,
                             &x, &y,
                             GIMP_CONSTRAIN_LINE_15_DEGREES);

      g_object_set (line,
                    "x1", x,
                    "y1", y,
                    NULL);
      return TRUE;

    case POINT_END:
      if (constrain)
        gimp_constrain_line (private->x1, private->y1,
                             &x, &y,
                             GIMP_CONSTRAIN_LINE_15_DEGREES);

      g_object_set (line,
                    "x2", x,
                    "y2", y,
                    NULL);
      return TRUE;

    case POINT_SLIDER:
      {
        GimpControllerSlider *slider;
        gdouble               t;

        slider = &g_array_index (private->sliders,
                                 GimpControllerSlider, private->slider_index);

        /* project the cursor position onto the line */
        t  = (private->x2 - private->x1) * (x - private->x1) +
             (private->y2 - private->y1) * (y - private->y1);
        t /= SQR (private->x2 - private->x1) + SQR (private->y2 - private->y1);

        t = CLAMP (t, slider->min, slider->max);
        t = CLAMP (t, 0.0, 1.0);

        if (constrain)
          t = RINT (24.0 * t) / 24.0;

        slider->value = t;

        g_object_set (line,
                      "sliders", private->sliders,
                      NULL);

        return TRUE;
      }

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
  gint                 i;

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
                                                     2 * ENDPOINT_GRIP_HANDLE_SIZE);
      start_visible = start_diameter > 2;

      end_diameter = gimp_canvas_handle_calc_size (private->end_handle_circle,
                                                   private->mouse_x,
                                                   private->mouse_y,
                                                   0,
                                                   2 * ENDPOINT_GRIP_HANDLE_SIZE);
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

  for (i = 0; i < private->sliders->len; i++)
    {
      GimpCanvasItem *circle;
      gboolean        visible;
      gint            diameter;

      circle = g_array_index (private->slider_handle_circles,
                              GimpCanvasItem *, i);

      if (private->point_grabbed)
        visible = FALSE;
      else
        {
          diameter = gimp_canvas_handle_calc_size (circle,
                                                   private->mouse_x,
                                                   private->mouse_y,
                                                   0,
                                                   2 * SLIDER_GRIP_HANDLE_SIZE);
          visible = diameter > 2;
        }

      gimp_canvas_item_set_visible (circle, visible);

      if (visible)
        gimp_canvas_handle_set_size (circle, diameter, diameter);
    }
}

static void
gimp_tool_line_update_hilight (GimpToolLine *line)
{
  GimpToolLinePrivate *private = line->private;
  gint                 i;

  gimp_canvas_item_set_highlight (private->start_handle_circle,
                                  private->point == POINT_START);
  gimp_canvas_item_set_highlight (private->start_handle_grip,
                                  private->point == POINT_START);

  gimp_canvas_item_set_highlight (private->end_handle_circle,
                                  private->point == POINT_END);
  gimp_canvas_item_set_highlight (private->end_handle_grip,
                                  private->point == POINT_END);

  for (i = 0; i < private->sliders->len; i++)
    {
      GimpCanvasItem *circle;
      GimpCanvasItem *grip;
      gboolean        highlight;

      circle = g_array_index (private->slider_handle_circles,
                              GimpCanvasItem *, i);
      grip   = g_array_index (private->slider_handle_grips,
                              GimpCanvasItem *, i);

      highlight =
        (private->point == POINT_SLIDER && private->slider_index == i);

      gimp_canvas_item_set_highlight (circle, highlight);
      gimp_canvas_item_set_highlight (grip,   highlight);
    }
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
                                          private->status_title,
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

void
gimp_tool_line_set_sliders (GimpToolLine               *line,
                            const GimpControllerSlider *sliders,
                            gint                        slider_count)
{
  GimpToolLinePrivate *private;

  g_return_if_fail (GIMP_IS_TOOL_LINE (line));
  g_return_if_fail (slider_count == 0 || (slider_count > 0 && sliders != NULL));

  private = line->private;

  g_array_set_size (private->sliders, slider_count);

  memcpy (private->sliders->data, sliders,
          slider_count * sizeof (GimpControllerSlider));

  g_object_set (line,
                "sliders", private->sliders,
                NULL);
}

const GimpControllerSlider *
gimp_tool_line_get_sliders (GimpToolLine *line,
                            gint         *slider_count)
{
  GimpToolLinePrivate *private;

  g_return_val_if_fail (GIMP_IS_TOOL_LINE (line), NULL);

  private = line->private;

  if (slider_count) *slider_count = private->sliders->len;

  return (const GimpControllerSlider *) private->sliders->data;
}
