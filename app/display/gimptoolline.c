/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ligmatoolline.c
 * Copyright (C) 2017 Michael Natterer <mitch@ligma.org>
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

#include "libligmabase/ligmabase.h"
#include "libligmamath/ligmamath.h"

#include "display-types.h"

#include "core/ligma-utils.h"
#include "core/ligmamarshal.h"

#include "widgets/ligmawidgets-utils.h"

#include "ligmacanvasgroup.h"
#include "ligmacanvashandle.h"
#include "ligmacanvasline.h"
#include "ligmadisplayshell.h"
#include "ligmadisplayshell-cursor.h"
#include "ligmadisplayshell-utils.h"
#include "ligmatoolline.h"

#include "ligma-intl.h"


#define SHOW_LINE            TRUE
#define GRAB_LINE_MASK       GDK_MOD1_MASK
#define ENDPOINT_HANDLE_TYPE LIGMA_HANDLE_CROSS
#define ENDPOINT_HANDLE_SIZE LIGMA_CANVAS_HANDLE_SIZE_CROSS
#define SLIDER_HANDLE_TYPE   LIGMA_HANDLE_FILLED_DIAMOND
#define SLIDER_HANDLE_SIZE   (ENDPOINT_HANDLE_SIZE * 2 / 3)
#define HANDLE_CIRCLE_SCALE  1.8
#define LINE_VICINITY        ((gint) (SLIDER_HANDLE_SIZE * HANDLE_CIRCLE_SCALE) / 2)
#define SLIDER_TEAR_DISTANCE (5 * LINE_VICINITY)


/* hover-only "handles" */
#define HOVER_NEW_SLIDER     (LIGMA_TOOL_LINE_HANDLE_NONE - 1)


typedef enum
{
  GRAB_NONE,
  GRAB_SELECTION,
  GRAB_LINE
} LigmaToolLineGrab;

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

struct _LigmaToolLinePrivate
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
  LigmaToolLineGrab   grab;

  LigmaCanvasItem    *line;
  LigmaCanvasItem    *start_handle;
  LigmaCanvasItem    *end_handle;
  LigmaCanvasItem    *slider_group;
  GArray            *slider_handles;
  LigmaCanvasItem    *handle_circle;
};


/*  local function prototypes  */

static void     ligma_tool_line_constructed     (GObject               *object);
static void     ligma_tool_line_finalize        (GObject               *object);
static void     ligma_tool_line_set_property    (GObject               *object,
                                                guint                  property_id,
                                                const GValue          *value,
                                                GParamSpec            *pspec);
static void     ligma_tool_line_get_property    (GObject               *object,
                                                guint                  property_id,
                                                GValue                *value,
                                                GParamSpec            *pspec);

static void     ligma_tool_line_changed         (LigmaToolWidget        *widget);
static void     ligma_tool_line_focus_changed   (LigmaToolWidget        *widget);
static gint     ligma_tool_line_button_press    (LigmaToolWidget        *widget,
                                                const LigmaCoords      *coords,
                                                guint32                time,
                                                GdkModifierType        state,
                                                LigmaButtonPressType    press_type);
static void     ligma_tool_line_button_release  (LigmaToolWidget        *widget,
                                                const LigmaCoords      *coords,
                                                guint32                time,
                                                GdkModifierType        state,
                                                LigmaButtonReleaseType  release_type);
static void     ligma_tool_line_motion          (LigmaToolWidget        *widget,
                                                const LigmaCoords      *coords,
                                                guint32                time,
                                                GdkModifierType        state);
static LigmaHit  ligma_tool_line_hit             (LigmaToolWidget        *widget,
                                                const LigmaCoords      *coords,
                                                GdkModifierType        state,
                                                gboolean               proximity);
static void     ligma_tool_line_hover           (LigmaToolWidget        *widget,
                                                const LigmaCoords      *coords,
                                                GdkModifierType        state,
                                                gboolean               proximity);
static void     ligma_tool_line_leave_notify    (LigmaToolWidget        *widget);
static gboolean ligma_tool_line_key_press       (LigmaToolWidget        *widget,
                                                GdkEventKey           *kevent);
static void     ligma_tool_line_motion_modifier (LigmaToolWidget        *widget,
                                                GdkModifierType        key,
                                                gboolean               press,
                                                GdkModifierType        state);
static gboolean ligma_tool_line_get_cursor      (LigmaToolWidget        *widget,
                                                const LigmaCoords      *coords,
                                                GdkModifierType        state,
                                                LigmaCursorType        *cursor,
                                                LigmaToolCursorType    *tool_cursor,
                                                LigmaCursorModifier    *modifier);

static gint     ligma_tool_line_get_hover       (LigmaToolLine          *line,
                                                const LigmaCoords      *coords,
                                                GdkModifierType        state);
static LigmaControllerSlider *
                ligma_tool_line_get_slider      (LigmaToolLine          *line,
                                                gint                   slider);
static LigmaCanvasItem *
                ligma_tool_line_get_handle      (LigmaToolLine          *line,
                                                gint                   handle);
static gdouble  ligma_tool_line_project_point   (LigmaToolLine          *line,
                                                gdouble                x,
                                                gdouble                y,
                                                gboolean               constrain,
                                                gdouble               *dist);

static gboolean
               ligma_tool_line_selection_motion (LigmaToolLine          *line,
                                                gboolean               constrain);

static void     ligma_tool_line_update_handles  (LigmaToolLine          *line);
static void     ligma_tool_line_update_circle   (LigmaToolLine          *line);
static void     ligma_tool_line_update_hilight  (LigmaToolLine          *line);
static void     ligma_tool_line_update_status   (LigmaToolLine          *line,
                                                GdkModifierType        state,
                                                gboolean               proximity);

static gboolean ligma_tool_line_handle_hit      (LigmaCanvasItem        *handle,
                                                gdouble                x,
                                                gdouble                y,
                                                gdouble               *min_dist);


G_DEFINE_TYPE_WITH_PRIVATE (LigmaToolLine, ligma_tool_line, LIGMA_TYPE_TOOL_WIDGET)

#define parent_class ligma_tool_line_parent_class

static guint line_signals[LAST_SIGNAL] = { 0, };


static void
ligma_tool_line_class_init (LigmaToolLineClass *klass)
{
  GObjectClass        *object_class = G_OBJECT_CLASS (klass);
  LigmaToolWidgetClass *widget_class = LIGMA_TOOL_WIDGET_CLASS (klass);

  object_class->constructed     = ligma_tool_line_constructed;
  object_class->finalize        = ligma_tool_line_finalize;
  object_class->set_property    = ligma_tool_line_set_property;
  object_class->get_property    = ligma_tool_line_get_property;

  widget_class->changed         = ligma_tool_line_changed;
  widget_class->focus_changed   = ligma_tool_line_focus_changed;
  widget_class->button_press    = ligma_tool_line_button_press;
  widget_class->button_release  = ligma_tool_line_button_release;
  widget_class->motion          = ligma_tool_line_motion;
  widget_class->hit             = ligma_tool_line_hit;
  widget_class->hover           = ligma_tool_line_hover;
  widget_class->leave_notify    = ligma_tool_line_leave_notify;
  widget_class->key_press       = ligma_tool_line_key_press;
  widget_class->motion_modifier = ligma_tool_line_motion_modifier;
  widget_class->get_cursor      = ligma_tool_line_get_cursor;

  line_signals[CAN_ADD_SLIDER] =
    g_signal_new ("can-add-slider",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (LigmaToolLineClass, can_add_slider),
                  NULL, NULL,
                  ligma_marshal_BOOLEAN__DOUBLE,
                  G_TYPE_BOOLEAN, 1,
                  G_TYPE_DOUBLE);

  line_signals[ADD_SLIDER] =
    g_signal_new ("add-slider",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (LigmaToolLineClass, add_slider),
                  NULL, NULL,
                  ligma_marshal_INT__DOUBLE,
                  G_TYPE_INT, 1,
                  G_TYPE_DOUBLE);

  line_signals[PREPARE_TO_REMOVE_SLIDER] =
    g_signal_new ("prepare-to-remove-slider",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (LigmaToolLineClass, prepare_to_remove_slider),
                  NULL, NULL,
                  ligma_marshal_VOID__INT_BOOLEAN,
                  G_TYPE_NONE, 2,
                  G_TYPE_INT,
                  G_TYPE_BOOLEAN);

  line_signals[REMOVE_SLIDER] =
    g_signal_new ("remove-slider",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (LigmaToolLineClass, remove_slider),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 1,
                  G_TYPE_INT);

  line_signals[SELECTION_CHANGED] =
    g_signal_new ("selection-changed",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (LigmaToolLineClass, selection_changed),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 0);

  line_signals[HANDLE_CLICKED] =
    g_signal_new ("handle-clicked",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (LigmaToolLineClass, handle_clicked),
                  NULL, NULL,
                  ligma_marshal_BOOLEAN__INT_UINT_ENUM,
                  G_TYPE_BOOLEAN, 3,
                  G_TYPE_INT,
                  G_TYPE_UINT,
                  LIGMA_TYPE_BUTTON_PRESS_TYPE);

  g_object_class_install_property (object_class, PROP_X1,
                                   g_param_spec_double ("x1", NULL, NULL,
                                                        -LIGMA_MAX_IMAGE_SIZE,
                                                        LIGMA_MAX_IMAGE_SIZE, 0,
                                                        LIGMA_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT));

  g_object_class_install_property (object_class, PROP_Y1,
                                   g_param_spec_double ("y1", NULL, NULL,
                                                        -LIGMA_MAX_IMAGE_SIZE,
                                                        LIGMA_MAX_IMAGE_SIZE, 0,
                                                        LIGMA_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT));

  g_object_class_install_property (object_class, PROP_X2,
                                   g_param_spec_double ("x2", NULL, NULL,
                                                        -LIGMA_MAX_IMAGE_SIZE,
                                                        LIGMA_MAX_IMAGE_SIZE, 0,
                                                        LIGMA_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT));

  g_object_class_install_property (object_class, PROP_Y2,
                                   g_param_spec_double ("y2", NULL, NULL,
                                                        -LIGMA_MAX_IMAGE_SIZE,
                                                        LIGMA_MAX_IMAGE_SIZE, 0,
                                                        LIGMA_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT));

  g_object_class_install_property (object_class, PROP_SLIDERS,
                                   g_param_spec_boxed ("sliders", NULL, NULL,
                                                       G_TYPE_ARRAY,
                                                       LIGMA_PARAM_READWRITE));

  g_object_class_install_property (object_class, PROP_SELECTION,
                                   g_param_spec_int ("selection", NULL, NULL,
                                                     LIGMA_TOOL_LINE_HANDLE_NONE,
                                                     G_MAXINT,
                                                     LIGMA_TOOL_LINE_HANDLE_NONE,
                                                     LIGMA_PARAM_READWRITE |
                                                     G_PARAM_CONSTRUCT));

  g_object_class_install_property (object_class, PROP_STATUS_TITLE,
                                   g_param_spec_string ("status-title",
                                                        NULL, NULL,
                                                        _("Line: "),
                                                        LIGMA_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT));
}

static void
ligma_tool_line_init (LigmaToolLine *line)
{
  LigmaToolLinePrivate *private;

  private = line->private = ligma_tool_line_get_instance_private (line);

  private->sliders = g_array_new (FALSE, FALSE, sizeof (LigmaControllerSlider));

  private->selection = LIGMA_TOOL_LINE_HANDLE_NONE;
  private->hover     = LIGMA_TOOL_LINE_HANDLE_NONE;
  private->grab      = GRAB_NONE;

  private->slider_handles = g_array_new (FALSE, TRUE,
                                         sizeof (LigmaCanvasItem *));
}

static void
ligma_tool_line_constructed (GObject *object)
{
  LigmaToolLine        *line    = LIGMA_TOOL_LINE (object);
  LigmaToolWidget      *widget  = LIGMA_TOOL_WIDGET (object);
  LigmaToolLinePrivate *private = line->private;

  G_OBJECT_CLASS (parent_class)->constructed (object);

  private->line = ligma_tool_widget_add_line (widget,
                                             private->x1,
                                             private->y1,
                                             private->x2,
                                             private->y2);

  ligma_canvas_item_set_visible (private->line, SHOW_LINE);

  private->start_handle =
    ligma_tool_widget_add_handle (widget,
                                 ENDPOINT_HANDLE_TYPE,
                                 private->x1,
                                 private->y1,
                                 ENDPOINT_HANDLE_SIZE,
                                 ENDPOINT_HANDLE_SIZE,
                                 LIGMA_HANDLE_ANCHOR_CENTER);

  private->end_handle =
    ligma_tool_widget_add_handle (widget,
                                 ENDPOINT_HANDLE_TYPE,
                                 private->x2,
                                 private->y2,
                                 ENDPOINT_HANDLE_SIZE,
                                 ENDPOINT_HANDLE_SIZE,
                                 LIGMA_HANDLE_ANCHOR_CENTER);

  private->slider_group =
    ligma_canvas_group_new (ligma_tool_widget_get_shell (widget));
  ligma_tool_widget_add_item (widget, private->slider_group);
  g_object_unref (private->slider_group);

  private->handle_circle =
    ligma_tool_widget_add_handle (widget,
                                 LIGMA_HANDLE_CIRCLE,
                                 private->x1,
                                 private->y1,
                                 ENDPOINT_HANDLE_SIZE * HANDLE_CIRCLE_SCALE,
                                 ENDPOINT_HANDLE_SIZE * HANDLE_CIRCLE_SCALE,
                                 LIGMA_HANDLE_ANCHOR_CENTER);

  ligma_tool_line_changed (widget);
}

static void
ligma_tool_line_finalize (GObject *object)
{
  LigmaToolLine        *line    = LIGMA_TOOL_LINE (object);
  LigmaToolLinePrivate *private = line->private;

  g_clear_pointer (&private->sliders, g_array_unref);
  g_clear_pointer (&private->status_title, g_free);
  g_clear_pointer (&private->slider_handles, g_array_unref);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
ligma_tool_line_set_property (GObject      *object,
                             guint         property_id,
                             const GValue *value,
                             GParamSpec   *pspec)
{
  LigmaToolLine        *line    = LIGMA_TOOL_LINE (object);
  LigmaToolLinePrivate *private = line->private;

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
          LIGMA_TOOL_LINE_HANDLE_IS_SLIDER (private->selection) &&
          (sliders->len != private->sliders->len               ||
           ! ligma_tool_line_get_slider (line, private->selection)->selectable);

        g_array_unref (private->sliders);
        private->sliders = sliders;

        if (LIGMA_TOOL_LINE_HANDLE_IS_SLIDER (private->hover))
          private->hover = LIGMA_TOOL_LINE_HANDLE_NONE;

        if (deselect)
          ligma_tool_line_set_selection (line, LIGMA_TOOL_LINE_HANDLE_NONE);
      }
      break;

    case PROP_SELECTION:
      {
        gint selection = g_value_get_int (value);

        g_return_if_fail (selection < (gint) private->sliders->len);
        g_return_if_fail (selection < 0 ||
                          ligma_tool_line_get_slider (line,
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
ligma_tool_line_get_property (GObject    *object,
                             guint       property_id,
                             GValue     *value,
                             GParamSpec *pspec)
{
  LigmaToolLine        *line    = LIGMA_TOOL_LINE (object);
  LigmaToolLinePrivate *private = line->private;

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
ligma_tool_line_changed (LigmaToolWidget *widget)
{
  LigmaToolLine        *line    = LIGMA_TOOL_LINE (widget);
  LigmaToolLinePrivate *private = line->private;
  gint                 i;

  ligma_canvas_line_set (private->line,
                        private->x1,
                        private->y1,
                        private->x2,
                        private->y2);

  ligma_canvas_handle_set_position (private->start_handle,
                                   private->x1,
                                   private->y1);

  ligma_canvas_handle_set_position (private->end_handle,
                                   private->x2,
                                   private->y2);

  /* remove excessive slider handles */
  for (i = private->sliders->len; i < private->slider_handles->len; i++)
    {
      ligma_canvas_group_remove_item (LIGMA_CANVAS_GROUP (private->slider_group),
                                     ligma_tool_line_get_handle (line, i));
    }

  g_array_set_size (private->slider_handles, private->sliders->len);

  for (i = 0; i < private->sliders->len; i++)
    {
      gdouble          value;
      gdouble          x;
      gdouble          y;
      LigmaCanvasItem **handle;

      value = ligma_tool_line_get_slider (line, i)->value;

      x = private->x1 + (private->x2 - private->x1) * value;
      y = private->y1 + (private->y2 - private->y1) * value;

      handle = &g_array_index (private->slider_handles, LigmaCanvasItem *, i);

      if (*handle)
        {
          ligma_canvas_handle_set_position (*handle, x, y);
        }
      else
        {
          *handle = ligma_canvas_handle_new (ligma_tool_widget_get_shell (widget),
                                            SLIDER_HANDLE_TYPE,
                                            LIGMA_HANDLE_ANCHOR_CENTER,
                                            x,
                                            y,
                                            SLIDER_HANDLE_SIZE,
                                            SLIDER_HANDLE_SIZE);

          ligma_canvas_group_add_item (LIGMA_CANVAS_GROUP (private->slider_group),
                                      *handle);
          g_object_unref (*handle);
        }
    }

  ligma_tool_line_update_handles (line);
  ligma_tool_line_update_circle (line);
  ligma_tool_line_update_hilight (line);
}

static void
ligma_tool_line_focus_changed (LigmaToolWidget *widget)
{
  LigmaToolLine *line = LIGMA_TOOL_LINE (widget);

  ligma_tool_line_update_hilight (line);
}

gboolean
ligma_tool_line_button_press (LigmaToolWidget      *widget,
                             const LigmaCoords    *coords,
                             guint32              time,
                             GdkModifierType      state,
                             LigmaButtonPressType  press_type)
{
  LigmaToolLine        *line    = LIGMA_TOOL_LINE (widget);
  LigmaToolLinePrivate *private = line->private;
  gboolean             result  = FALSE;

  private->grab          = GRAB_NONE;
  private->remove_slider = FALSE;

  private->saved_x1 = private->x1;
  private->saved_y1 = private->y1;
  private->saved_x2 = private->x2;
  private->saved_y2 = private->y2;

  if (press_type         != LIGMA_BUTTON_PRESS_NORMAL   &&
      private->hover      > LIGMA_TOOL_LINE_HANDLE_NONE &&
      private->selection  > LIGMA_TOOL_LINE_HANDLE_NONE)
    {
      g_signal_emit (line, line_signals[HANDLE_CLICKED], 0,
                     private->selection, state, press_type, &result);

      if (! result)
        ligma_tool_widget_hover (widget, coords, state, TRUE);
    }

  if (press_type == LIGMA_BUTTON_PRESS_NORMAL || ! result)
    {
      private->saved_x1 = private->x1;
      private->saved_y1 = private->y1;
      private->saved_x2 = private->x2;
      private->saved_y2 = private->y2;

      if (LIGMA_TOOL_LINE_HANDLE_IS_SLIDER (private->hover))
        {
          private->saved_slider_value =
            ligma_tool_line_get_slider (line, private->hover)->value;
        }

      if (private->hover > LIGMA_TOOL_LINE_HANDLE_NONE)
        {
          ligma_tool_line_set_selection (line, private->hover);

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
              ligma_tool_line_set_selection (line, slider);

              private->saved_slider_value =
                ligma_tool_line_get_slider (line, private->selection)->value;

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
      private->hover = LIGMA_TOOL_LINE_HANDLE_NONE;

      ligma_tool_line_set_selection (line, LIGMA_TOOL_LINE_HANDLE_NONE);
    }

  ligma_tool_line_update_handles (line);
  ligma_tool_line_update_circle (line);
  ligma_tool_line_update_status (line, state, TRUE);

  return result;
}

void
ligma_tool_line_button_release (LigmaToolWidget        *widget,
                               const LigmaCoords      *coords,
                               guint32                time,
                               GdkModifierType        state,
                               LigmaButtonReleaseType  release_type)
{
  LigmaToolLine        *line    = LIGMA_TOOL_LINE (widget);
  LigmaToolLinePrivate *private = line->private;
  LigmaToolLineGrab     grab    = private->grab;

  private->grab = GRAB_NONE;

  if (release_type == LIGMA_BUTTON_RELEASE_CANCEL)
    {
      if (grab != GRAB_NONE)
        {
          if (grab == GRAB_SELECTION &&
              LIGMA_TOOL_LINE_HANDLE_IS_SLIDER (private->selection))
            {
              ligma_tool_line_get_slider (line, private->selection)->value =
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
      else if (release_type == LIGMA_BUTTON_RELEASE_CLICK)
        {
          gboolean result;

          g_signal_emit (line, line_signals[HANDLE_CLICKED], 0,
                         private->selection, state, LIGMA_BUTTON_PRESS_NORMAL,
                         &result);
        }
    }
}

void
ligma_tool_line_motion (LigmaToolWidget   *widget,
                       const LigmaCoords *coords,
                       guint32           time,
                       GdkModifierType   state)
{
  LigmaToolLine        *line    = LIGMA_TOOL_LINE (widget);
  LigmaToolLinePrivate *private = line->private;
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
      gboolean constrain = (state & ligma_get_constrain_behavior_mask ()) != 0;

      ligma_tool_line_selection_motion (line, constrain);
    }

  ligma_tool_line_update_status (line, state, TRUE);
}

LigmaHit
ligma_tool_line_hit (LigmaToolWidget   *widget,
                    const LigmaCoords *coords,
                    GdkModifierType   state,
                    gboolean          proximity)
{
  LigmaToolLine *line = LIGMA_TOOL_LINE (widget);

  if (! (state & GRAB_LINE_MASK))
    {
      gint hover = ligma_tool_line_get_hover (line, coords, state);

      if (hover != LIGMA_TOOL_LINE_HANDLE_NONE)
        return LIGMA_HIT_DIRECT;
    }
  else
    {
      return LIGMA_HIT_INDIRECT;
    }

  return LIGMA_HIT_NONE;
}

void
ligma_tool_line_hover (LigmaToolWidget   *widget,
                      const LigmaCoords *coords,
                      GdkModifierType   state,
                      gboolean          proximity)
{
  LigmaToolLine        *line    = LIGMA_TOOL_LINE (widget);
  LigmaToolLinePrivate *private = line->private;

  private->mouse_x = coords->x;
  private->mouse_y = coords->y;

  if (! (state & GRAB_LINE_MASK))
    private->hover = ligma_tool_line_get_hover (line, coords, state);
  else
    private->hover = LIGMA_TOOL_LINE_HANDLE_NONE;

  ligma_tool_line_update_handles (line);
  ligma_tool_line_update_circle (line);
  ligma_tool_line_update_status (line, state, proximity);
}

static void
ligma_tool_line_leave_notify (LigmaToolWidget *widget)
{
  LigmaToolLine        *line    = LIGMA_TOOL_LINE (widget);
  LigmaToolLinePrivate *private = line->private;

  private->hover = LIGMA_TOOL_LINE_HANDLE_NONE;

  ligma_tool_line_update_handles (line);
  ligma_tool_line_update_circle (line);

  LIGMA_TOOL_WIDGET_CLASS (parent_class)->leave_notify (widget);
}

static gboolean
ligma_tool_line_key_press (LigmaToolWidget *widget,
                          GdkEventKey    *kevent)
{
  LigmaToolLine        *line    = LIGMA_TOOL_LINE (widget);
  LigmaToolLinePrivate *private = line->private;
  LigmaDisplayShell    *shell;
  gdouble              pixels  = 1.0;
  gboolean             move_line;

  move_line = kevent->state & GRAB_LINE_MASK;

  if (private->selection == LIGMA_TOOL_LINE_HANDLE_NONE && ! move_line)
    return LIGMA_TOOL_WIDGET_CLASS (parent_class)->key_press (widget, kevent);

  shell = ligma_tool_widget_get_shell (widget);

  if (kevent->state & ligma_get_extend_selection_mask ())
    pixels = 10.0;

  if (kevent->state & ligma_get_toggle_behavior_mask ())
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

          if (private->selection == LIGMA_TOOL_LINE_HANDLE_START || move_line)
            {
              g_object_set (line,
                            "x1", private->x1 + dx,
                            "y1", private->y1 + dy,
                            NULL);
            }

          if (private->selection == LIGMA_TOOL_LINE_HANDLE_END || move_line)
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
          LigmaControllerSlider *slider;
          gdouble               dist;
          gdouble               dvalue;

          slider = ligma_tool_line_get_slider (line, private->selection);

          if (! slider->movable)
            break;

          dist = ligma_canvas_item_transform_distance (private->line,
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
      if (LIGMA_TOOL_LINE_HANDLE_IS_SLIDER (private->selection))
        {
          if (ligma_tool_line_get_slider (line, private->selection)->removable)
            {
              g_signal_emit (line, line_signals[REMOVE_SLIDER], 0,
                             private->selection);
            }
        }
      return TRUE;
    }

  return LIGMA_TOOL_WIDGET_CLASS (parent_class)->key_press (widget, kevent);
}

static void
ligma_tool_line_motion_modifier (LigmaToolWidget  *widget,
                                GdkModifierType  key,
                                gboolean         press,
                                GdkModifierType  state)
{
  LigmaToolLine *line = LIGMA_TOOL_LINE (widget);

  if (key == ligma_get_constrain_behavior_mask ())
    {
      ligma_tool_line_selection_motion (line, press);

      ligma_tool_line_update_status (line, state, TRUE);
    }
}

static gboolean
ligma_tool_line_get_cursor (LigmaToolWidget     *widget,
                           const LigmaCoords   *coords,
                           GdkModifierType     state,
                           LigmaCursorType     *cursor,
                           LigmaToolCursorType *tool_cursor,
                           LigmaCursorModifier *modifier)
{
  LigmaToolLine        *line    = LIGMA_TOOL_LINE (widget);
  LigmaToolLinePrivate *private = line->private;

  if (private->grab ==GRAB_LINE || (state & GRAB_LINE_MASK))
    {
      *modifier = LIGMA_CURSOR_MODIFIER_MOVE;

      return TRUE;
    }
  else if (private->grab  == GRAB_SELECTION ||
           private->hover >  LIGMA_TOOL_LINE_HANDLE_NONE)
    {
      const LigmaControllerSlider *slider = NULL;

      if (private->grab == GRAB_SELECTION)
        {
          if (LIGMA_TOOL_LINE_HANDLE_IS_SLIDER (private->selection))
            slider = ligma_tool_line_get_slider (line, private->selection);
        }
      else if (LIGMA_TOOL_LINE_HANDLE_IS_SLIDER (private->hover))
        {
          slider = ligma_tool_line_get_slider (line, private->hover);
        }

      if (private->grab == GRAB_SELECTION && slider && private->remove_slider)
        {
          *modifier = LIGMA_CURSOR_MODIFIER_MINUS;

          return TRUE;
        }
      else if (! slider || slider->movable)
        {
          *modifier = LIGMA_CURSOR_MODIFIER_MOVE;

          return TRUE;
        }
    }
  else if (private->hover == HOVER_NEW_SLIDER)
    {
      *modifier = LIGMA_CURSOR_MODIFIER_PLUS;

      return TRUE;
    }

  return FALSE;
}

static gint
ligma_tool_line_get_hover (LigmaToolLine     *line,
                          const LigmaCoords *coords,
                          GdkModifierType   state)
{
  LigmaToolLinePrivate *private = line->private;
  gint                 hover   = LIGMA_TOOL_LINE_HANDLE_NONE;
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

  for (i = first_handle; i > LIGMA_TOOL_LINE_HANDLE_NONE; i--)
    {
      LigmaCanvasItem *handle;

      if (LIGMA_TOOL_LINE_HANDLE_IS_SLIDER (i))
        {
          const LigmaControllerSlider *slider;

          slider = ligma_tool_line_get_slider (line, i);

          if (! slider->visible || ! slider->selectable)
            continue;
        }

      handle = ligma_tool_line_get_handle (line, i);

      if (ligma_tool_line_handle_hit (handle,
                                     private->mouse_x,
                                     private->mouse_y,
                                     &min_dist))
        {
          hover = i;
        }
    }

  if (hover == LIGMA_TOOL_LINE_HANDLE_NONE)
    {
      gboolean constrain;
      gdouble  value;
      gdouble  dist;

      constrain = (state & ligma_get_constrain_behavior_mask ()) != 0;

      value = ligma_tool_line_project_point (line,
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

static LigmaControllerSlider *
ligma_tool_line_get_slider (LigmaToolLine *line,
                           gint          slider)
{
  LigmaToolLinePrivate *private = line->private;

  ligma_assert (slider >= 0 && slider < private->sliders->len);

  return &g_array_index (private->sliders, LigmaControllerSlider, slider);
}

static LigmaCanvasItem *
ligma_tool_line_get_handle (LigmaToolLine *line,
                           gint          handle)
{
  LigmaToolLinePrivate *private = line->private;

  switch (handle)
    {
    case LIGMA_TOOL_LINE_HANDLE_NONE:
      return NULL;

    case LIGMA_TOOL_LINE_HANDLE_START:
      return private->start_handle;

    case LIGMA_TOOL_LINE_HANDLE_END:
      return private->end_handle;

    default:
      ligma_assert (handle >= 0 &&
                   handle <  (gint) private->slider_handles->len);

      return g_array_index (private->slider_handles,
                            LigmaCanvasItem *, handle);
    }
}

static gdouble
ligma_tool_line_project_point (LigmaToolLine *line,
                              gdouble       x,
                              gdouble       y,
                              gboolean      constrain,
                              gdouble      *dist)
{
  LigmaToolLinePrivate *private = line->private;
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

          *dist = ligma_canvas_item_transform_distance (private->line,
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
          *dist = ligma_canvas_item_transform_distance (private->line,
                                                       x,           y,
                                                       private->x1, private->y1);
        }
    }

  return value;
}

static gboolean
ligma_tool_line_selection_motion (LigmaToolLine *line,
                                 gboolean      constrain)
{
  LigmaToolLinePrivate *private = line->private;
  gdouble              x       = private->mouse_x;
  gdouble              y       = private->mouse_y;

  if (private->grab != GRAB_SELECTION)
    return FALSE;

  switch (private->selection)
    {
    case LIGMA_TOOL_LINE_HANDLE_NONE:
      ligma_assert_not_reached ();

    case LIGMA_TOOL_LINE_HANDLE_START:
      if (constrain)
        {
          ligma_display_shell_constrain_line (
            ligma_tool_widget_get_shell (LIGMA_TOOL_WIDGET (line)),
            private->x2, private->y2,
            &x, &y,
            LIGMA_CONSTRAIN_LINE_15_DEGREES);
        }

      g_object_set (line,
                    "x1", x,
                    "y1", y,
                    NULL);
      return TRUE;

    case LIGMA_TOOL_LINE_HANDLE_END:
      if (constrain)
        {
          ligma_display_shell_constrain_line (
            ligma_tool_widget_get_shell (LIGMA_TOOL_WIDGET (line)),
            private->x1, private->y1,
            &x, &y,
            LIGMA_CONSTRAIN_LINE_15_DEGREES);
        }

      g_object_set (line,
                    "x2", x,
                    "y2", y,
                    NULL);
      return TRUE;

    default:
      {
        LigmaDisplayShell     *shell;
        LigmaControllerSlider *slider;
        gdouble               value;
        gdouble               dist;
        gboolean              remove_slider;

        shell = ligma_tool_widget_get_shell (LIGMA_TOOL_WIDGET (line));

        slider = ligma_tool_line_get_slider (line, private->selection);

        /* project the cursor position onto the line */
        value = ligma_tool_line_project_point (line, x, y, constrain, &dist);

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
              LigmaCursorType     cursor;
              LigmaToolCursorType tool_cursor;
              LigmaCursorModifier modifier;

              cursor      = shell->current_cursor;
              tool_cursor = shell->tool_cursor;
              modifier    = LIGMA_CURSOR_MODIFIER_NONE;

              ligma_tool_line_get_cursor (LIGMA_TOOL_WIDGET (line), NULL, 0,
                                         &cursor, &tool_cursor, &modifier);

              ligma_display_shell_set_cursor (shell, cursor, tool_cursor, modifier);
            }

            ligma_tool_line_update_handles (line);
            ligma_tool_line_update_circle (line);
            ligma_tool_line_update_status (line,
                                          constrain ?
                                            ligma_get_constrain_behavior_mask () :
                                            0,
                                          TRUE);
          }

        return TRUE;
      }
    }
}

static void
ligma_tool_line_update_handles (LigmaToolLine *line)
{
  LigmaToolLinePrivate *private = line->private;
  gdouble              value;
  gdouble              dist;
  gint                 i;

  value = ligma_tool_line_project_point (line,
                                        private->mouse_x,
                                        private->mouse_y,
                                        FALSE,
                                        &dist);

  for (i = 0; i < private->sliders->len; i++)
    {
      const LigmaControllerSlider *slider;
      LigmaCanvasItem             *handle;
      gint                        size;
      gint                        hit_radius;
      gboolean                    show_autohidden;
      gboolean                    visible;

      slider = ligma_tool_line_get_slider (line, i);
      handle = ligma_tool_line_get_handle (line, i);

      size = slider->size * SLIDER_HANDLE_SIZE;
      size = MAX (size, 1);

      hit_radius = (MAX (size, SLIDER_HANDLE_SIZE) * HANDLE_CIRCLE_SCALE) / 2;

      /* show a autohidden slider if it's selected, or if no other handle is
       * grabbed or hovered-over, and the cursor is close enough to the line,
       * between the slider's min and max values.
       */
      show_autohidden = private->selection == i                        ||
                        (private->grab == GRAB_NONE                    &&
                         (private->hover <= LIGMA_TOOL_LINE_HANDLE_NONE ||
                          private->hover == i)                         &&
                         dist <= hit_radius                            &&
                         value >= slider->min                          &&
                         value <= slider->max);

      visible = slider->visible                         &&
                (! slider->autohide || show_autohidden) &&
                ! (private->selection == i && private->remove_slider);

      handle = ligma_tool_line_get_handle (line, i);

      if (visible)
        {
          g_object_set (handle,
                        "type",   slider->type,
                        "width",  size,
                        "height", size,
                        NULL);
        }

      ligma_canvas_item_set_visible (handle, visible);
    }
}

static void
ligma_tool_line_update_circle (LigmaToolLine *line)
{
  LigmaToolLinePrivate *private = line->private;
  gboolean             visible;

  visible = (private->grab == GRAB_NONE                    &&
             private->hover != LIGMA_TOOL_LINE_HANDLE_NONE) ||
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
          LigmaCanvasItem *handle;

          if (private->grab == GRAB_SELECTION)
            {
              /* tear slider */
              handle = ligma_tool_line_get_handle (line, private->selection);
              dashed = TRUE;
            }
          else
            {
              /* hover over handle */
              handle = ligma_tool_line_get_handle (line, private->hover);
              dashed = FALSE;
            }

          ligma_canvas_handle_get_position (handle, &x,     &y);
          ligma_canvas_handle_get_size     (handle, &width, &height);
        }

      width   = MAX (width,  SLIDER_HANDLE_SIZE);
      height  = MAX (height, SLIDER_HANDLE_SIZE);

      width  *= HANDLE_CIRCLE_SCALE;
      height *= HANDLE_CIRCLE_SCALE;

      ligma_canvas_handle_set_position (private->handle_circle, x,     y);
      ligma_canvas_handle_set_size     (private->handle_circle, width, height);

      g_object_set (private->handle_circle,
                    "type", dashed ? LIGMA_HANDLE_DASHED_CIRCLE :
                                     LIGMA_HANDLE_CIRCLE,
                    NULL);
    }

  ligma_canvas_item_set_visible (private->handle_circle, visible);
}

static void
ligma_tool_line_update_hilight (LigmaToolLine *line)
{
  LigmaToolLinePrivate *private = line->private;
  gboolean             focus;
  gint                 i;

  focus = ligma_tool_widget_get_focus (LIGMA_TOOL_WIDGET (line));

  for (i = LIGMA_TOOL_LINE_HANDLE_NONE + 1;
       i < (gint) private->sliders->len;
       i++)
    {
      LigmaCanvasItem *handle;

      handle = ligma_tool_line_get_handle (line, i);

      ligma_canvas_item_set_highlight (handle, focus && i == private->selection);
    }
}

static void
ligma_tool_line_update_status (LigmaToolLine    *line,
                              GdkModifierType  state,
                              gboolean         proximity)
{
  LigmaToolLinePrivate *private = line->private;

  if (proximity)
    {
      LigmaDisplayShell *shell;
      const gchar      *toggle_behavior_format = NULL;
      const gchar      *message                = NULL;
      gchar            *line_status            = NULL;
      gchar            *status;
      gint              handle;

      shell = ligma_tool_widget_get_shell (LIGMA_TOOL_WIDGET (line));

      if (private->grab == GRAB_SELECTION)
        handle = private->selection;
      else
        handle = private->hover;

      if (handle == LIGMA_TOOL_LINE_HANDLE_START ||
          handle == LIGMA_TOOL_LINE_HANDLE_END)
        {
          line_status = ligma_display_shell_get_line_status (shell,
                                                            _("Click-Drag to move the endpoint"),
                                                            ". ",
                                                            private->x1,
                                                            private->y1,
                                                            private->x2,
                                                            private->y2);
          toggle_behavior_format = _("%s for constrained angles");
        }
      else if (LIGMA_TOOL_LINE_HANDLE_IS_SLIDER (handle) ||
               handle == HOVER_NEW_SLIDER)
        {
          if (private->grab == GRAB_SELECTION && private->remove_slider)
            {
              message = _("Release to remove the slider");
            }
          else
            {
              toggle_behavior_format = _("%s for constrained values");

              if (LIGMA_TOOL_LINE_HANDLE_IS_SLIDER (handle))
                {
                  if (ligma_tool_line_get_slider (line, handle)->movable)
                    {
                      if (ligma_tool_line_get_slider (line, handle)->removable)
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

                      if (ligma_tool_line_get_slider (line, handle)->removable)
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
        ligma_suggest_modifiers (message ? message : (line_status ? line_status : ""),
                                ((toggle_behavior_format ?
                                   ligma_get_constrain_behavior_mask () : 0) |
                                 (private->grab == GRAB_NONE ?
                                   GDK_MOD1_MASK : 0)) &
                                ~state,
                                NULL,
                                toggle_behavior_format,
                                _("%s to move the whole line"));

      if (message || line_status)
        {
          ligma_tool_widget_set_status (LIGMA_TOOL_WIDGET (line), status);
        }
      else
        {
          line_status = ligma_display_shell_get_line_status (shell,
                                                            private->status_title,
                                                            ". ",
                                                            private->x1,
                                                            private->y1,
                                                            private->x2,
                                                            private->y2);
          ligma_tool_widget_set_status_coords (LIGMA_TOOL_WIDGET (line),
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
      ligma_tool_widget_set_status (LIGMA_TOOL_WIDGET (line), NULL);
    }
}

static gboolean
ligma_tool_line_handle_hit (LigmaCanvasItem *handle,
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

  ligma_canvas_handle_get_position (handle, &handle_x,     &handle_y);
  ligma_canvas_handle_get_size     (handle, &handle_width, &handle_height);

  handle_width  = MAX (handle_width,  SLIDER_HANDLE_SIZE);
  handle_height = MAX (handle_height, SLIDER_HANDLE_SIZE);

  radius = ((gint) (handle_width * HANDLE_CIRCLE_SCALE)) / 2;
  radius = MAX (radius, LINE_VICINITY);

  dist = ligma_canvas_item_transform_distance (handle,
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

LigmaToolWidget *
ligma_tool_line_new (LigmaDisplayShell *shell,
                    gdouble           x1,
                    gdouble           y1,
                    gdouble           x2,
                    gdouble           y2)
{
  g_return_val_if_fail (LIGMA_IS_DISPLAY_SHELL (shell), NULL);

  return g_object_new (LIGMA_TYPE_TOOL_LINE,
                       "shell", shell,
                       "x1",    x1,
                       "y1",    y1,
                       "x2",    x2,
                       "y2",    y2,
                       NULL);
}

void
ligma_tool_line_set_sliders (LigmaToolLine               *line,
                            const LigmaControllerSlider *sliders,
                            gint                        n_sliders)
{
  LigmaToolLinePrivate *private;

  g_return_if_fail (LIGMA_IS_TOOL_LINE (line));
  g_return_if_fail (n_sliders == 0 || (n_sliders > 0 && sliders != NULL));

  private = line->private;

  if (LIGMA_TOOL_LINE_HANDLE_IS_SLIDER (private->selection) &&
      private->sliders->len != n_sliders)
    {
      ligma_tool_line_set_selection (line, LIGMA_TOOL_LINE_HANDLE_NONE);
    }

  g_array_set_size (private->sliders, n_sliders);

  memcpy (private->sliders->data, sliders,
          n_sliders * sizeof (LigmaControllerSlider));

  g_object_set (line,
                "sliders", private->sliders,
                NULL);
}

const LigmaControllerSlider *
ligma_tool_line_get_sliders (LigmaToolLine *line,
                            gint         *n_sliders)
{
  LigmaToolLinePrivate *private;

  g_return_val_if_fail (LIGMA_IS_TOOL_LINE (line), NULL);

  private = line->private;

  if (n_sliders) *n_sliders = private->sliders->len;

  return (const LigmaControllerSlider *) private->sliders->data;
}

void
ligma_tool_line_set_selection (LigmaToolLine *line,
                              gint          handle)
{
  LigmaToolLinePrivate *private;

  g_return_if_fail (LIGMA_IS_TOOL_LINE (line));

  private = line->private;

  g_return_if_fail (handle >= LIGMA_TOOL_LINE_HANDLE_NONE &&
                    handle <  (gint) private->sliders->len);

  g_object_set (line,
                "selection", handle,
                NULL);
}

gint
ligma_tool_line_get_selection (LigmaToolLine *line)
{
  LigmaToolLinePrivate *private;

  g_return_val_if_fail (LIGMA_IS_TOOL_LINE (line), LIGMA_TOOL_LINE_HANDLE_NONE);

  private = line->private;

  return private->selection;
}
