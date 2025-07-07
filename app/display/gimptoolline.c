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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <gegl.h>
#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>

#include "libgimpbase/gimpbase.h"
#include "libgimpmath/gimpmath.h"

#include "display-types.h"

#include "core/gimp-utils.h"
#include "core/gimpmarshal.h"

#include "widgets/gimpwidgets-utils.h"

#include "gimpcanvasgroup.h"
#include "gimpcanvashandle.h"
#include "gimpcanvasline.h"
#include "gimpdisplayshell.h"
#include "gimpdisplayshell-cursor.h"
#include "gimpdisplayshell-utils.h"
#include "gimptoolline.h"

#include "gimp-intl.h"


#define SHOW_LINE            TRUE
#define GRAB_LINE_MASK       GDK_MOD1_MASK
#define ENDPOINT_HANDLE_TYPE GIMP_HANDLE_CROSS
#define ENDPOINT_HANDLE_SIZE GIMP_CANVAS_HANDLE_SIZE_CROSS
#define SLIDER_HANDLE_TYPE   GIMP_HANDLE_FILLED_DIAMOND
#define SLIDER_HANDLE_SIZE   (ENDPOINT_HANDLE_SIZE * 2 / 3)
#define HANDLE_CIRCLE_SCALE  1.8
#define LINE_VICINITY        ((gint) (SLIDER_HANDLE_SIZE * HANDLE_CIRCLE_SCALE) / 2)
#define SLIDER_TEAR_DISTANCE (5 * LINE_VICINITY)


/* hover-only "handles" */
#define HOVER_NEW_SLIDER     (GIMP_TOOL_LINE_HANDLE_NONE - 1)


typedef enum
{
  GRAB_NONE,
  GRAB_SELECTION,
  GRAB_LINE
} GimpToolLineGrab;

enum
{
  PROP_0,
  PROP_X1,
  PROP_Y1,
  PROP_X2,
  PROP_Y2,
  PROP_SLIDERS,
  PROP_SELECTION,
  PROP_STATUS_TITLE,
};

enum
{
  CAN_ADD_SLIDER,
  ADD_SLIDER,
  PREPARE_TO_REMOVE_SLIDER,
  REMOVE_SLIDER,
  SELECTION_CHANGED,
  HANDLE_CLICKED,
  LAST_SIGNAL
};

struct _GimpToolLinePrivate
{
  gdouble            x1;
  gdouble            y1;
  gdouble            x2;
  gdouble            y2;
  GArray            *sliders;
  gint               selection;
  gchar             *status_title;

  gdouble            saved_x1;
  gdouble            saved_y1;
  gdouble            saved_x2;
  gdouble            saved_y2;
  gdouble            saved_slider_value;

  gdouble            mouse_x;
  gdouble            mouse_y;
  gint               hover;
  gdouble            new_slider_value;
  gboolean           remove_slider;
  GimpToolLineGrab   grab;

  GimpCanvasItem    *line;
  GimpCanvasItem    *start_handle;
  GimpCanvasItem    *end_handle;
  GimpCanvasItem    *slider_group;
  GArray            *slider_handles;
  GimpCanvasItem    *handle_circle;
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
static void     gimp_tool_line_focus_changed   (GimpToolWidget        *widget);
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
static GimpHit  gimp_tool_line_hit             (GimpToolWidget        *widget,
                                                const GimpCoords      *coords,
                                                GdkModifierType        state,
                                                gboolean               proximity);
static void     gimp_tool_line_hover           (GimpToolWidget        *widget,
                                                const GimpCoords      *coords,
                                                GdkModifierType        state,
                                                gboolean               proximity);
static void     gimp_tool_line_leave_notify    (GimpToolWidget        *widget);
static gboolean gimp_tool_line_key_press       (GimpToolWidget        *widget,
                                                GdkEventKey           *kevent);
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

static gint     gimp_tool_line_get_hover       (GimpToolLine          *line,
                                                const GimpCoords      *coords,
                                                GdkModifierType        state);
static GimpControllerSlider *
                gimp_tool_line_get_slider      (GimpToolLine          *line,
                                                gint                   slider);
static GimpCanvasItem *
                gimp_tool_line_get_handle      (GimpToolLine          *line,
                                                gint                   handle);
static gdouble  gimp_tool_line_project_point   (GimpToolLine          *line,
                                                gdouble                x,
                                                gdouble                y,
                                                gboolean               constrain,
                                                gdouble               *dist);

static gboolean
               gimp_tool_line_selection_motion (GimpToolLine          *line,
                                                gboolean               constrain);

static void     gimp_tool_line_update_handles  (GimpToolLine          *line);
static void     gimp_tool_line_update_circle   (GimpToolLine          *line);
static void     gimp_tool_line_update_hilight  (GimpToolLine          *line);
static void     gimp_tool_line_update_status   (GimpToolLine          *line,
                                                GdkModifierType        state,
                                                gboolean               proximity);

static gboolean gimp_tool_line_handle_hit      (GimpCanvasItem        *handle,
                                                gdouble                x,
                                                gdouble                y,
                                                gdouble               *min_dist);


G_DEFINE_TYPE_WITH_PRIVATE (GimpToolLine, gimp_tool_line, GIMP_TYPE_TOOL_WIDGET)

#define parent_class gimp_tool_line_parent_class

static guint line_signals[LAST_SIGNAL] = { 0, };


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
  widget_class->focus_changed   = gimp_tool_line_focus_changed;
  widget_class->button_press    = gimp_tool_line_button_press;
  widget_class->button_release  = gimp_tool_line_button_release;
  widget_class->motion          = gimp_tool_line_motion;
  widget_class->hit             = gimp_tool_line_hit;
  widget_class->hover           = gimp_tool_line_hover;
  widget_class->leave_notify    = gimp_tool_line_leave_notify;
  widget_class->key_press       = gimp_tool_line_key_press;
  widget_class->motion_modifier = gimp_tool_line_motion_modifier;
  widget_class->get_cursor      = gimp_tool_line_get_cursor;

  line_signals[CAN_ADD_SLIDER] =
    g_signal_new ("can-add-slider",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (GimpToolLineClass, can_add_slider),
                  NULL, NULL,
                  gimp_marshal_BOOLEAN__DOUBLE,
                  G_TYPE_BOOLEAN, 1,
                  G_TYPE_DOUBLE);

  line_signals[ADD_SLIDER] =
    g_signal_new ("add-slider",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (GimpToolLineClass, add_slider),
                  NULL, NULL,
                  gimp_marshal_INT__DOUBLE,
                  G_TYPE_INT, 1,
                  G_TYPE_DOUBLE);

  line_signals[PREPARE_TO_REMOVE_SLIDER] =
    g_signal_new ("prepare-to-remove-slider",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (GimpToolLineClass, prepare_to_remove_slider),
                  NULL, NULL,
                  gimp_marshal_VOID__INT_BOOLEAN,
                  G_TYPE_NONE, 2,
                  G_TYPE_INT,
                  G_TYPE_BOOLEAN);

  line_signals[REMOVE_SLIDER] =
    g_signal_new ("remove-slider",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (GimpToolLineClass, remove_slider),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 1,
                  G_TYPE_INT);

  line_signals[SELECTION_CHANGED] =
    g_signal_new ("selection-changed",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (GimpToolLineClass, selection_changed),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 0);

  line_signals[HANDLE_CLICKED] =
    g_signal_new ("handle-clicked",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (GimpToolLineClass, handle_clicked),
                  NULL, NULL,
                  gimp_marshal_BOOLEAN__INT_UINT_ENUM,
                  G_TYPE_BOOLEAN, 3,
                  G_TYPE_INT,
                  G_TYPE_UINT,
                  GIMP_TYPE_BUTTON_PRESS_TYPE);

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

  g_object_class_install_property (object_class, PROP_SELECTION,
                                   g_param_spec_int ("selection", NULL, NULL,
                                                     GIMP_TOOL_LINE_HANDLE_NONE,
                                                     G_MAXINT,
                                                     GIMP_TOOL_LINE_HANDLE_NONE,
                                                     GIMP_PARAM_READWRITE |
                                                     G_PARAM_CONSTRUCT));

  g_object_class_install_property (object_class, PROP_STATUS_TITLE,
                                   g_param_spec_string ("status-title",
                                                        NULL, NULL,
                                                        _("Line: "),
                                                        GIMP_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT));
}

static void
gimp_tool_line_init (GimpToolLine *line)
{
  GimpToolLinePrivate *private;

  private = line->private = gimp_tool_line_get_instance_private (line);

  private->sliders = g_array_new (FALSE, FALSE, sizeof (GimpControllerSlider));

  private->selection = GIMP_TOOL_LINE_HANDLE_NONE;
  private->hover     = GIMP_TOOL_LINE_HANDLE_NONE;
  private->grab      = GRAB_NONE;

  private->slider_handles = g_array_new (FALSE, TRUE,
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

  private->start_handle =
    gimp_tool_widget_add_handle (widget,
                                 ENDPOINT_HANDLE_TYPE,
                                 private->x1,
                                 private->y1,
                                 ENDPOINT_HANDLE_SIZE,
                                 ENDPOINT_HANDLE_SIZE,
                                 GIMP_HANDLE_ANCHOR_CENTER);

  private->end_handle =
    gimp_tool_widget_add_handle (widget,
                                 ENDPOINT_HANDLE_TYPE,
                                 private->x2,
                                 private->y2,
                                 ENDPOINT_HANDLE_SIZE,
                                 ENDPOINT_HANDLE_SIZE,
                                 GIMP_HANDLE_ANCHOR_CENTER);

  private->slider_group =
    gimp_canvas_group_new (gimp_tool_widget_get_shell (widget));
  gimp_tool_widget_add_item (widget, private->slider_group);
  g_object_unref (private->slider_group);

  private->handle_circle =
    gimp_tool_widget_add_handle (widget,
                                 GIMP_HANDLE_CIRCLE,
                                 private->x1,
                                 private->y1,
                                 ENDPOINT_HANDLE_SIZE * HANDLE_CIRCLE_SCALE,
                                 ENDPOINT_HANDLE_SIZE * HANDLE_CIRCLE_SCALE,
                                 GIMP_HANDLE_ANCHOR_CENTER);

  gimp_tool_line_changed (widget);
}

static void
gimp_tool_line_finalize (GObject *object)
{
  GimpToolLine        *line    = GIMP_TOOL_LINE (object);
  GimpToolLinePrivate *private = line->private;

  g_clear_pointer (&private->sliders, g_array_unref);
  g_clear_pointer (&private->status_title, g_free);
  g_clear_pointer (&private->slider_handles, g_array_unref);

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
      {
        GArray   *sliders = g_value_dup_boxed (value);
        gboolean  deselect;

        g_return_if_fail (sliders != NULL);

        deselect =
          GIMP_TOOL_LINE_HANDLE_IS_SLIDER (private->selection) &&
          (sliders->len != private->sliders->len               ||
           ! gimp_tool_line_get_slider (line, private->selection)->selectable);

        g_array_unref (private->sliders);
        private->sliders = sliders;

        if (GIMP_TOOL_LINE_HANDLE_IS_SLIDER (private->hover))
          private->hover = GIMP_TOOL_LINE_HANDLE_NONE;

        if (deselect)
          gimp_tool_line_set_selection (line, GIMP_TOOL_LINE_HANDLE_NONE);
      }
      break;

    case PROP_SELECTION:
      {
        gint selection = g_value_get_int (value);

        g_return_if_fail (selection < (gint) private->sliders->len);
        g_return_if_fail (selection < 0 ||
                          gimp_tool_line_get_slider (line,
                                                     selection)->selectable);

        if (selection != private->selection)
          {
            private->selection = selection;

            if (private->grab == GRAB_SELECTION)
              private->grab = GRAB_NONE;

            g_signal_emit (line, line_signals[SELECTION_CHANGED], 0);
          }
      }
      break;

    case PROP_STATUS_TITLE:
      g_set_str (&private->status_title, g_value_get_string (value));
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

    case PROP_SELECTION:
      g_value_set_int (value, private->selection);
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

  gimp_canvas_handle_set_position (private->start_handle,
                                   private->x1,
                                   private->y1);

  gimp_canvas_handle_set_position (private->end_handle,
                                   private->x2,
                                   private->y2);

  /* remove excessive slider handles */
  for (i = private->sliders->len; i < private->slider_handles->len; i++)
    {
      gimp_canvas_group_remove_item (GIMP_CANVAS_GROUP (private->slider_group),
                                     gimp_tool_line_get_handle (line, i));
    }

  g_array_set_size (private->slider_handles, private->sliders->len);

  for (i = 0; i < private->sliders->len; i++)
    {
      gdouble          value;
      gdouble          x;
      gdouble          y;
      GimpCanvasItem **handle;

      value = gimp_tool_line_get_slider (line, i)->value;

      x = private->x1 + (private->x2 - private->x1) * value;
      y = private->y1 + (private->y2 - private->y1) * value;

      handle = &g_array_index (private->slider_handles, GimpCanvasItem *, i);

      if (*handle)
        {
          gimp_canvas_handle_set_position (*handle, x, y);
        }
      else
        {
          *handle = gimp_canvas_handle_new (gimp_tool_widget_get_shell (widget),
                                            SLIDER_HANDLE_TYPE,
                                            GIMP_HANDLE_ANCHOR_CENTER,
                                            x,
                                            y,
                                            SLIDER_HANDLE_SIZE,
                                            SLIDER_HANDLE_SIZE);

          gimp_canvas_group_add_item (GIMP_CANVAS_GROUP (private->slider_group),
                                      *handle);
          g_object_unref (*handle);
        }
    }

  gimp_tool_line_update_handles (line);
  gimp_tool_line_update_circle (line);
  gimp_tool_line_update_hilight (line);
}

static void
gimp_tool_line_focus_changed (GimpToolWidget *widget)
{
  GimpToolLine *line = GIMP_TOOL_LINE (widget);

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
  gboolean             result  = FALSE;

  private->grab          = GRAB_NONE;
  private->remove_slider = FALSE;

  private->saved_x1 = private->x1;
  private->saved_y1 = private->y1;
  private->saved_x2 = private->x2;
  private->saved_y2 = private->y2;

  if (press_type         != GIMP_BUTTON_PRESS_NORMAL   &&
      private->hover      > GIMP_TOOL_LINE_HANDLE_NONE &&
      private->selection  > GIMP_TOOL_LINE_HANDLE_NONE)
    {
      g_signal_emit (line, line_signals[HANDLE_CLICKED], 0,
                     private->selection, state, press_type, &result);

      if (! result)
        gimp_tool_widget_hover (widget, coords, state, TRUE);
    }

  if (press_type == GIMP_BUTTON_PRESS_NORMAL || ! result)
    {
      private->saved_x1 = private->x1;
      private->saved_y1 = private->y1;
      private->saved_x2 = private->x2;
      private->saved_y2 = private->y2;

      if (GIMP_TOOL_LINE_HANDLE_IS_SLIDER (private->hover))
        {
          private->saved_slider_value =
            gimp_tool_line_get_slider (line, private->hover)->value;
        }

      if (private->hover > GIMP_TOOL_LINE_HANDLE_NONE)
        {
          gimp_tool_line_set_selection (line, private->hover);

          private->grab = GRAB_SELECTION;
        }
      else if (private->hover == HOVER_NEW_SLIDER)
        {
          gint slider;

          g_signal_emit (line, line_signals[ADD_SLIDER], 0,
                         private->new_slider_value, &slider);

          g_return_val_if_fail (slider < (gint) private->sliders->len, FALSE);

          if (slider >= 0)
            {
              gimp_tool_line_set_selection (line, slider);

              private->saved_slider_value =
                gimp_tool_line_get_slider (line, private->selection)->value;

              private->grab = GRAB_SELECTION;
            }
        }
      else if (state & GRAB_LINE_MASK)
        {
          private->grab = GRAB_LINE;
        }

      result = (private->grab != GRAB_NONE);
    }

  if (! result)
    {
      private->hover = GIMP_TOOL_LINE_HANDLE_NONE;

      gimp_tool_line_set_selection (line, GIMP_TOOL_LINE_HANDLE_NONE);
    }

  gimp_tool_line_update_handles (line);
  gimp_tool_line_update_circle (line);
  gimp_tool_line_update_status (line, state, TRUE);

  return result;
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
  GimpToolLineGrab     grab    = private->grab;

  private->grab = GRAB_NONE;

  if (release_type == GIMP_BUTTON_RELEASE_CANCEL)
    {
      if (grab != GRAB_NONE)
        {
          if (grab == GRAB_SELECTION &&
              GIMP_TOOL_LINE_HANDLE_IS_SLIDER (private->selection))
            {
              gimp_tool_line_get_slider (line, private->selection)->value =
                private->saved_slider_value;

              if (private->remove_slider)
                {
                  private->remove_slider = FALSE;

                  g_signal_emit (line, line_signals[PREPARE_TO_REMOVE_SLIDER], 0,
                                 private->selection, FALSE);
                }
            }

          g_object_set (line,
                        "x1", private->saved_x1,
                        "y1", private->saved_y1,
                        "x2", private->saved_x2,
                        "y2", private->saved_y2,
                        NULL);
        }
    }
  else if (grab == GRAB_SELECTION)
    {
      if (private->remove_slider)
        {
          private->remove_slider = FALSE;

          g_signal_emit (line, line_signals[REMOVE_SLIDER], 0,
                         private->selection);
        }
      else if (release_type == GIMP_BUTTON_RELEASE_CLICK)
        {
          gboolean result;

          g_signal_emit (line, line_signals[HANDLE_CLICKED], 0,
                         private->selection, state, GIMP_BUTTON_PRESS_NORMAL,
                         &result);
        }
    }
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

  if (private->grab == GRAB_LINE)
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

      gimp_tool_line_selection_motion (line, constrain);
    }

  gimp_tool_line_update_status (line, state, TRUE);
}

GimpHit
gimp_tool_line_hit (GimpToolWidget   *widget,
                    const GimpCoords *coords,
                    GdkModifierType   state,
                    gboolean          proximity)
{
  GimpToolLine *line = GIMP_TOOL_LINE (widget);

  if (! (state & GRAB_LINE_MASK))
    {
      gint hover = gimp_tool_line_get_hover (line, coords, state);

      if (hover != GIMP_TOOL_LINE_HANDLE_NONE)
        return GIMP_HIT_DIRECT;
    }
  else
    {
      return GIMP_HIT_INDIRECT;
    }

  return GIMP_HIT_NONE;
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

  if (! (state & GRAB_LINE_MASK))
    private->hover = gimp_tool_line_get_hover (line, coords, state);
  else
    private->hover = GIMP_TOOL_LINE_HANDLE_NONE;

  gimp_tool_line_update_handles (line);
  gimp_tool_line_update_circle (line);
  gimp_tool_line_update_status (line, state, proximity);
}

static void
gimp_tool_line_leave_notify (GimpToolWidget *widget)
{
  GimpToolLine        *line    = GIMP_TOOL_LINE (widget);
  GimpToolLinePrivate *private = line->private;

  private->hover = GIMP_TOOL_LINE_HANDLE_NONE;

  gimp_tool_line_update_handles (line);
  gimp_tool_line_update_circle (line);

  GIMP_TOOL_WIDGET_CLASS (parent_class)->leave_notify (widget);
}

static gboolean
gimp_tool_line_key_press (GimpToolWidget *widget,
                          GdkEventKey    *kevent)
{
  GimpToolLine        *line    = GIMP_TOOL_LINE (widget);
  GimpToolLinePrivate *private = line->private;
  GimpDisplayShell    *shell;
  gdouble              pixels  = 1.0;
  gboolean             move_line;

  move_line = kevent->state & GRAB_LINE_MASK;

  if (private->selection == GIMP_TOOL_LINE_HANDLE_NONE && ! move_line)
    return GIMP_TOOL_WIDGET_CLASS (parent_class)->key_press (widget, kevent);

  shell = gimp_tool_widget_get_shell (widget);

  if (kevent->state & gimp_get_extend_selection_mask ())
    pixels = 10.0;

  if (kevent->state & gimp_get_toggle_behavior_mask ())
    pixels = 50.0;

  switch (kevent->keyval)
    {
    case GDK_KEY_Left:
    case GDK_KEY_Right:
    case GDK_KEY_Up:
    case GDK_KEY_Down:
      /* move an endpoint (or both endpoints) */
      if (private->selection < 0 || move_line)
        {
          gdouble xdist, ydist;
          gdouble dx,    dy;

          xdist = FUNSCALEX (shell, pixels);
          ydist = FUNSCALEY (shell, pixels);

          dx = 0.0;
          dy = 0.0;

          switch (kevent->keyval)
            {
            case GDK_KEY_Left:  dx = -xdist; break;
            case GDK_KEY_Right: dx = +xdist; break;
            case GDK_KEY_Up:    dy = -ydist; break;
            case GDK_KEY_Down:  dy = +ydist; break;
            }

          if (private->selection == GIMP_TOOL_LINE_HANDLE_START || move_line)
            {
              g_object_set (line,
                            "x1", private->x1 + dx,
                            "y1", private->y1 + dy,
                            NULL);
            }

          if (private->selection == GIMP_TOOL_LINE_HANDLE_END || move_line)
            {
              g_object_set (line,
                            "x2", private->x2 + dx,
                            "y2", private->y2 + dy,
                            NULL);
            }
        }
      /* move a slider */
      else
        {
          GimpControllerSlider *slider;
          gdouble               dist;
          gdouble               dvalue;

          slider = gimp_tool_line_get_slider (line, private->selection);

          if (! slider->movable)
            break;

          dist = gimp_canvas_item_transform_distance (private->line,
                                                      private->x1, private->y1,
                                                      private->x2, private->y2);

          if (dist > 0.0)
            dist = pixels / dist;

          dvalue = 0.0;

          switch (kevent->keyval)
            {
            case GDK_KEY_Left:
              if      (private->x1 < private->x2) dvalue = -dist;
              else if (private->x1 > private->x2) dvalue = +dist;
              break;

            case GDK_KEY_Right:
              if      (private->x1 < private->x2) dvalue = +dist;
              else if (private->x1 > private->x2) dvalue = -dist;
              break;

            case GDK_KEY_Up:
              if      (private->y1 < private->y2) dvalue = -dist;
              else if (private->y1 > private->y2) dvalue = +dist;
              break;

            case GDK_KEY_Down:
              if      (private->y1 < private->y2) dvalue = +dist;
              else if (private->y1 > private->y2) dvalue = -dist;
              break;
            }

          if (dvalue != 0.0)
            {
              slider->value += dvalue;
              slider->value  = CLAMP (slider->value, slider->min, slider->max);
              slider->value  = CLAMP (slider->value, 0.0, 1.0);

              g_object_set (line,
                            "sliders", private->sliders,
                            NULL);
            }
        }
      return TRUE;

    case GDK_KEY_BackSpace:
    case GDK_KEY_Delete:
      if (GIMP_TOOL_LINE_HANDLE_IS_SLIDER (private->selection))
        {
          if (gimp_tool_line_get_slider (line, private->selection)->removable)
            {
              g_signal_emit (line, line_signals[REMOVE_SLIDER], 0,
                             private->selection);
            }
        }
      return TRUE;
    }

  return GIMP_TOOL_WIDGET_CLASS (parent_class)->key_press (widget, kevent);
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
      gimp_tool_line_selection_motion (line, press);

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

  if (private->grab ==GRAB_LINE || (state & GRAB_LINE_MASK))
    {
      *modifier = GIMP_CURSOR_MODIFIER_MOVE;

      return TRUE;
    }
  else if (private->grab  == GRAB_SELECTION ||
           private->hover >  GIMP_TOOL_LINE_HANDLE_NONE)
    {
      const GimpControllerSlider *slider = NULL;

      if (private->grab == GRAB_SELECTION)
        {
          if (GIMP_TOOL_LINE_HANDLE_IS_SLIDER (private->selection))
            slider = gimp_tool_line_get_slider (line, private->selection);
        }
      else if (GIMP_TOOL_LINE_HANDLE_IS_SLIDER (private->hover))
        {
          slider = gimp_tool_line_get_slider (line, private->hover);
        }

      if (private->grab == GRAB_SELECTION && slider && private->remove_slider)
        {
          *modifier = GIMP_CURSOR_MODIFIER_MINUS;

          return TRUE;
        }
      else if (! slider || slider->movable)
        {
          *modifier = GIMP_CURSOR_MODIFIER_MOVE;

          return TRUE;
        }
    }
  else if (private->hover == HOVER_NEW_SLIDER)
    {
      *modifier = GIMP_CURSOR_MODIFIER_PLUS;

      return TRUE;
    }

  return FALSE;
}

static gint
gimp_tool_line_get_hover (GimpToolLine     *line,
                          const GimpCoords *coords,
                          GdkModifierType   state)
{
  GimpToolLinePrivate *private = line->private;
  gint                 hover   = GIMP_TOOL_LINE_HANDLE_NONE;
  gdouble              min_dist;
  gint                 first_handle;
  gint                 i;

  /* find the closest handle to the cursor */
  min_dist     = G_MAXDOUBLE;
  first_handle = private->sliders->len - 1;

  /* skip the sliders if the two endpoints are the same, in particular so
   * that if the line is created during a button-press event (as in the
   * blend tool), the end endpoint is dragged, instead of a slider.
   */
  if (private->x1 == private->x2 && private->y1 == private->y2)
    first_handle = -1;

  for (i = first_handle; i > GIMP_TOOL_LINE_HANDLE_NONE; i--)
    {
      GimpCanvasItem *handle;

      if (GIMP_TOOL_LINE_HANDLE_IS_SLIDER (i))
        {
          const GimpControllerSlider *slider;

          slider = gimp_tool_line_get_slider (line, i);

          if (! slider->visible || ! slider->selectable)
            continue;
        }

      handle = gimp_tool_line_get_handle (line, i);

      if (gimp_tool_line_handle_hit (handle,
                                     private->mouse_x,
                                     private->mouse_y,
                                     &min_dist))
        {
          hover = i;
        }
    }

  if (hover == GIMP_TOOL_LINE_HANDLE_NONE)
    {
      gboolean constrain;
      gdouble  value;
      gdouble  dist;

      constrain = (state & gimp_get_constrain_behavior_mask ()) != 0;

      value = gimp_tool_line_project_point (line,
                                            private->mouse_x,
                                            private->mouse_y,
                                            constrain,
                                            &dist);

      if (value >= 0.0 && value <= 1.0 && dist <= LINE_VICINITY)
        {
          gboolean can_add;

          g_signal_emit (line, line_signals[CAN_ADD_SLIDER], 0,
                         value, &can_add);

          if (can_add)
            {
              hover                     = HOVER_NEW_SLIDER;
              private->new_slider_value = value;
            }
        }
    }

  return hover;
}

static GimpControllerSlider *
gimp_tool_line_get_slider (GimpToolLine *line,
                           gint          slider)
{
  GimpToolLinePrivate *private = line->private;

  gimp_assert (slider >= 0 && slider < private->sliders->len);

  return &g_array_index (private->sliders, GimpControllerSlider, slider);
}

static GimpCanvasItem *
gimp_tool_line_get_handle (GimpToolLine *line,
                           gint          handle)
{
  GimpToolLinePrivate *private = line->private;

  switch (handle)
    {
    case GIMP_TOOL_LINE_HANDLE_NONE:
      return NULL;

    case GIMP_TOOL_LINE_HANDLE_START:
      return private->start_handle;

    case GIMP_TOOL_LINE_HANDLE_END:
      return private->end_handle;

    default:
      gimp_assert (handle >= 0 &&
                   handle <  (gint) private->slider_handles->len);

      return g_array_index (private->slider_handles,
                            GimpCanvasItem *, handle);
    }
}

static gdouble
gimp_tool_line_project_point (GimpToolLine *line,
                              gdouble       x,
                              gdouble       y,
                              gboolean      constrain,
                              gdouble      *dist)
{
  GimpToolLinePrivate *private = line->private;
  gdouble              length_sqr;
  gdouble              value   = 0.0;

  length_sqr = SQR (private->x2 - private->x1) +
               SQR (private->y2 - private->y1);

  /* don't calculate the projection for 0-length lines, since we'll just get
   * NaN.
   */
  if (length_sqr > 0.0)
    {
      value  = (private->x2 - private->x1) * (x - private->x1) +
               (private->y2 - private->y1) * (y - private->y1);
      value /= length_sqr;

      if (dist)
        {
          gdouble px;
          gdouble py;

          px = private->x1 + (private->x2 - private->x1) * value;
          py = private->y1 + (private->y2 - private->y1) * value;

          *dist = gimp_canvas_item_transform_distance (private->line,
                                                       x,  y,
                                                       px, py);
        }

      if (constrain)
        value = RINT (12.0 * value) / 12.0;
    }
  else
    {
      if (dist)
        {
          *dist = gimp_canvas_item_transform_distance (private->line,
                                                       x,           y,
                                                       private->x1, private->y1);
        }
    }

  return value;
}

static gboolean
gimp_tool_line_selection_motion (GimpToolLine *line,
                                 gboolean      constrain)
{
  GimpToolLinePrivate *private = line->private;
  gdouble              x       = private->mouse_x;
  gdouble              y       = private->mouse_y;

  if (private->grab != GRAB_SELECTION)
    return FALSE;

  switch (private->selection)
    {
    case GIMP_TOOL_LINE_HANDLE_NONE:
      gimp_assert_not_reached ();

    case GIMP_TOOL_LINE_HANDLE_START:
      if (constrain)
        {
          gimp_display_shell_constrain_line (
            gimp_tool_widget_get_shell (GIMP_TOOL_WIDGET (line)),
            private->x2, private->y2,
            &x, &y,
            GIMP_CONSTRAIN_LINE_15_DEGREES);
        }

      g_object_set (line,
                    "x1", x,
                    "y1", y,
                    NULL);
      return TRUE;

    case GIMP_TOOL_LINE_HANDLE_END:
      if (constrain)
        {
          gimp_display_shell_constrain_line (
            gimp_tool_widget_get_shell (GIMP_TOOL_WIDGET (line)),
            private->x1, private->y1,
            &x, &y,
            GIMP_CONSTRAIN_LINE_15_DEGREES);
        }

      g_object_set (line,
                    "x2", x,
                    "y2", y,
                    NULL);
      return TRUE;

    default:
      {
        GimpDisplayShell     *shell;
        GimpControllerSlider *slider;
        gdouble               value;
        gdouble               dist;
        gboolean              remove_slider;

        shell = gimp_tool_widget_get_shell (GIMP_TOOL_WIDGET (line));

        slider = gimp_tool_line_get_slider (line, private->selection);

        /* project the cursor position onto the line */
        value = gimp_tool_line_project_point (line, x, y, constrain, &dist);

        /* slider dragging */
        if (slider->movable)
          {
            value = CLAMP (value, slider->min, slider->max);
            value = CLAMP (value, 0.0,         1.0);

            value = fabs (value); /* avoid negative zero */

            slider->value = value;

            g_object_set (line,
                          "sliders", private->sliders,
                          NULL);
          }

        /* slider tearing */
        remove_slider = slider->removable && dist > SLIDER_TEAR_DISTANCE;

        if (remove_slider != private->remove_slider)
          {
            private->remove_slider = remove_slider;

            g_signal_emit (line, line_signals[PREPARE_TO_REMOVE_SLIDER], 0,
                           private->selection, remove_slider);

            /* set the cursor modifier to a minus by talking to the shell
             * directly -- eek!
             */
            {
              GimpCursorType     cursor;
              GimpToolCursorType tool_cursor;
              GimpCursorModifier modifier;

              cursor      = shell->current_cursor;
              tool_cursor = shell->tool_cursor;
              modifier    = GIMP_CURSOR_MODIFIER_NONE;

              gimp_tool_line_get_cursor (GIMP_TOOL_WIDGET (line), NULL, 0,
                                         &cursor, &tool_cursor, &modifier);

              gimp_display_shell_set_cursor (shell, cursor, tool_cursor, modifier);
            }

            gimp_tool_line_update_handles (line);
            gimp_tool_line_update_circle (line);
            gimp_tool_line_update_status (line,
                                          constrain ?
                                            gimp_get_constrain_behavior_mask () :
                                            0,
                                          TRUE);
          }

        return TRUE;
      }
    }
}

static void
gimp_tool_line_update_handles (GimpToolLine *line)
{
  GimpToolLinePrivate *private = line->private;
  gdouble              value;
  gdouble              dist;
  gint                 i;

  value = gimp_tool_line_project_point (line,
                                        private->mouse_x,
                                        private->mouse_y,
                                        FALSE,
                                        &dist);

  for (i = 0; i < private->sliders->len; i++)
    {
      const GimpControllerSlider *slider;
      GimpCanvasItem             *handle;
      gint                        size;
      gint                        hit_radius;
      gboolean                    show_autohidden;
      gboolean                    visible;

      slider = gimp_tool_line_get_slider (line, i);
      handle = gimp_tool_line_get_handle (line, i);

      size = slider->size * SLIDER_HANDLE_SIZE;
      size = MAX (size, 1);

      hit_radius = (MAX (size, SLIDER_HANDLE_SIZE) * HANDLE_CIRCLE_SCALE) / 2;

      /* show a autohidden slider if it's selected, or if no other handle is
       * grabbed or hovered-over, and the cursor is close enough to the line,
       * between the slider's min and max values.
       */
      show_autohidden = private->selection == i                        ||
                        (private->grab == GRAB_NONE                    &&
                         (private->hover <= GIMP_TOOL_LINE_HANDLE_NONE ||
                          private->hover == i)                         &&
                         dist <= hit_radius                            &&
                         value >= slider->min                          &&
                         value <= slider->max);

      visible = slider->visible                         &&
                (! slider->autohide || show_autohidden) &&
                ! (private->selection == i && private->remove_slider);

      handle = gimp_tool_line_get_handle (line, i);

      if (visible)
        {
          g_object_set (handle,
                        "type",   slider->type,
                        "width",  size,
                        "height", size,
                        NULL);
        }

      gimp_canvas_item_set_visible (handle, visible);
    }
}

static void
gimp_tool_line_update_circle (GimpToolLine *line)
{
  GimpToolLinePrivate *private = line->private;
  gboolean             visible;

  visible = (private->grab == GRAB_NONE                    &&
             private->hover != GIMP_TOOL_LINE_HANDLE_NONE) ||
            (private->grab == GRAB_SELECTION               &&
             private->remove_slider);

  if (visible)
    {
      gdouble  x;
      gdouble  y;
      gint     width;
      gint     height;
      gboolean dashed;

      if (private->grab == GRAB_NONE && private->hover == HOVER_NEW_SLIDER)
        {
          /* new slider */
          x = private->x1 +
              (private->x2 - private->x1) * private->new_slider_value;
          y = private->y1 +
              (private->y2 - private->y1) * private->new_slider_value;

          width = height = SLIDER_HANDLE_SIZE;

          dashed = TRUE;
        }
      else
        {
          GimpCanvasItem *handle;

          if (private->grab == GRAB_SELECTION)
            {
              /* tear slider */
              handle = gimp_tool_line_get_handle (line, private->selection);
              dashed = TRUE;
            }
          else
            {
              /* hover over handle */
              handle = gimp_tool_line_get_handle (line, private->hover);
              dashed = FALSE;
            }

          gimp_canvas_handle_get_position (handle, &x,     &y);
          gimp_canvas_handle_get_size     (handle, &width, &height);
        }

      width   = MAX (width,  SLIDER_HANDLE_SIZE);
      height  = MAX (height, SLIDER_HANDLE_SIZE);

      width  *= HANDLE_CIRCLE_SCALE;
      height *= HANDLE_CIRCLE_SCALE;

      gimp_canvas_handle_set_position (private->handle_circle, x,     y);
      gimp_canvas_handle_set_size     (private->handle_circle, width, height);

      g_object_set (private->handle_circle,
                    "type", dashed ? GIMP_HANDLE_DASHED_CIRCLE :
                                     GIMP_HANDLE_CIRCLE,
                    NULL);
    }

  gimp_canvas_item_set_visible (private->handle_circle, visible);
}

static void
gimp_tool_line_update_hilight (GimpToolLine *line)
{
  GimpToolLinePrivate *private = line->private;
  gboolean             focus;
  gint                 i;

  focus = gimp_tool_widget_get_focus (GIMP_TOOL_WIDGET (line));

  for (i = GIMP_TOOL_LINE_HANDLE_NONE + 1;
       i < (gint) private->sliders->len;
       i++)
    {
      GimpCanvasItem *handle;

      handle = gimp_tool_line_get_handle (line, i);

      gimp_canvas_item_set_highlight (handle, focus && i == private->selection);
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
      GimpDisplayShell *shell;
      const gchar      *toggle_behavior_format = NULL;
      const gchar      *message                = NULL;
      gchar            *line_status            = NULL;
      gchar            *status;
      gint              handle;

      shell = gimp_tool_widget_get_shell (GIMP_TOOL_WIDGET (line));

      if (private->grab == GRAB_SELECTION)
        handle = private->selection;
      else
        handle = private->hover;

      if (handle == GIMP_TOOL_LINE_HANDLE_START ||
          handle == GIMP_TOOL_LINE_HANDLE_END)
        {
          line_status = gimp_display_shell_get_line_status (shell,
                                                            _("Click-Drag to move the endpoint"),
                                                            ". ",
                                                            private->x1,
                                                            private->y1,
                                                            private->x2,
                                                            private->y2);
          toggle_behavior_format = _("%s for constrained angles");
        }
      else if (GIMP_TOOL_LINE_HANDLE_IS_SLIDER (handle) ||
               handle == HOVER_NEW_SLIDER)
        {
          if (private->grab == GRAB_SELECTION && private->remove_slider)
            {
              message = _("Release to remove the slider");
            }
          else
            {
              toggle_behavior_format = _("%s for constrained values");

              if (GIMP_TOOL_LINE_HANDLE_IS_SLIDER (handle))
                {
                  if (gimp_tool_line_get_slider (line, handle)->movable)
                    {
                      if (gimp_tool_line_get_slider (line, handle)->removable)
                        {
                          if (private->grab == GRAB_SELECTION)
                            {
                              message = _("Click-Drag to move the slider; "
                                          "drag away to remove the slider");
                            }
                          else
                            {
                              message = _("Click-Drag to move or remove the slider");
                            }
                        }
                      else
                        {
                          message = _("Click-Drag to move the slider");
                        }
                    }
                  else
                    {
                      toggle_behavior_format = NULL;

                      if (gimp_tool_line_get_slider (line, handle)->removable)
                        {
                          if (private->grab == GRAB_SELECTION)
                            {
                              message = _("Click-Drag away to remove the slider");
                            }
                          else
                            {
                              message = _("Click-Drag to remove the slider");
                            }
                        }
                      else
                        {
                          message = NULL;
                        }
                    }
                }
              else
                {
                  message = _("Click or Click-Drag to add a new slider");
                }
            }
        }
      else if (state & GDK_MOD1_MASK)
        {
          message = _("Click-Drag to move the line");
        }

      status =
        gimp_suggest_modifiers (message ? message : (line_status ? line_status : ""),
                                ((toggle_behavior_format ?
                                   gimp_get_constrain_behavior_mask () : 0) |
                                 (private->grab == GRAB_NONE ?
                                   GDK_MOD1_MASK : 0)) &
                                ~state,
                                NULL,
                                toggle_behavior_format,
                                _("%s to move the whole line"));

      if (message || line_status)
        {
          gimp_tool_widget_set_status (GIMP_TOOL_WIDGET (line), status);
        }
      else
        {
          line_status = gimp_display_shell_get_line_status (shell,
                                                            private->status_title,
                                                            ". ",
                                                            private->x1,
                                                            private->y1,
                                                            private->x2,
                                                            private->y2);
          gimp_tool_widget_set_status_coords (GIMP_TOOL_WIDGET (line),
                                              line_status,
                                              private->x2 - private->x1,
                                              ", ",
                                              private->y2 - private->y1,
                                              status);
        }

      g_free (status);
      if (line_status)
        g_free (line_status);
    }
  else
    {
      gimp_tool_widget_set_status (GIMP_TOOL_WIDGET (line), NULL);
    }
}

static gboolean
gimp_tool_line_handle_hit (GimpCanvasItem *handle,
                           gdouble         x,
                           gdouble         y,
                           gdouble        *min_dist)
{
  gdouble handle_x;
  gdouble handle_y;
  gint    handle_width;
  gint    handle_height;
  gint    radius;
  gdouble dist;

  gimp_canvas_handle_get_position (handle, &handle_x,     &handle_y);
  gimp_canvas_handle_get_size     (handle, &handle_width, &handle_height);

  handle_width  = MAX (handle_width,  SLIDER_HANDLE_SIZE);
  handle_height = MAX (handle_height, SLIDER_HANDLE_SIZE);

  radius = ((gint) (handle_width * HANDLE_CIRCLE_SCALE)) / 2;
  radius = MAX (radius, LINE_VICINITY);

  dist = gimp_canvas_item_transform_distance (handle,
                                              x, y, handle_x, handle_y);

  if (dist <= radius && dist < *min_dist)
    {
      *min_dist = dist;

      return TRUE;
    }
  else
    {
      return FALSE;
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
                            gint                        n_sliders)
{
  GimpToolLinePrivate *private;

  g_return_if_fail (GIMP_IS_TOOL_LINE (line));
  g_return_if_fail (n_sliders == 0 || (n_sliders > 0 && sliders != NULL));

  private = line->private;

  if (GIMP_TOOL_LINE_HANDLE_IS_SLIDER (private->selection) &&
      private->sliders->len != n_sliders)
    {
      gimp_tool_line_set_selection (line, GIMP_TOOL_LINE_HANDLE_NONE);
    }

  g_array_set_size (private->sliders, n_sliders);

  memcpy (private->sliders->data, sliders,
          n_sliders * sizeof (GimpControllerSlider));

  g_object_set (line,
                "sliders", private->sliders,
                NULL);
}

const GimpControllerSlider *
gimp_tool_line_get_sliders (GimpToolLine *line,
                            gint         *n_sliders)
{
  GimpToolLinePrivate *private;

  g_return_val_if_fail (GIMP_IS_TOOL_LINE (line), NULL);

  private = line->private;

  if (n_sliders) *n_sliders = private->sliders->len;

  return (const GimpControllerSlider *) private->sliders->data;
}

void
gimp_tool_line_set_selection (GimpToolLine *line,
                              gint          handle)
{
  GimpToolLinePrivate *private;

  g_return_if_fail (GIMP_IS_TOOL_LINE (line));

  private = line->private;

  g_return_if_fail (handle >= GIMP_TOOL_LINE_HANDLE_NONE &&
                    handle <  (gint) private->sliders->len);

  g_object_set (line,
                "selection", handle,
                NULL);
}

gint
gimp_tool_line_get_selection (GimpToolLine *line)
{
  GimpToolLinePrivate *private;

  g_return_val_if_fail (GIMP_IS_TOOL_LINE (line), GIMP_TOOL_LINE_HANDLE_NONE);

  private = line->private;

  return private->selection;
}
