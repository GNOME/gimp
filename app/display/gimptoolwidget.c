/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimptoolwidget.c
 * Copyright (C) 2017 Michael Natterer <mitch@gimp.org>
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

#include <stdarg.h>

#include <gegl.h>
#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>

#include "libgimpbase/gimpbase.h"

#include "display-types.h"

#include "core/gimpmarshal.h"

#include "gimpcanvasarc.h"
#include "gimpcanvascorner.h"
#include "gimpcanvasgroup.h"
#include "gimpcanvashandle.h"
#include "gimpcanvaslimit.h"
#include "gimpcanvasline.h"
#include "gimpcanvaspath.h"
#include "gimpcanvaspolygon.h"
#include "gimpcanvasrectangle.h"
#include "gimpcanvasrectangleguides.h"
#include "gimpcanvastransformguides.h"
#include "gimpdisplayshell.h"
#include "gimptoolwidget.h"


enum
{
  PROP_0,
  PROP_SHELL,
  PROP_ITEM
};

enum
{
  CHANGED,
  RESPONSE,
  SNAP_OFFSETS,
  STATUS,
  STATUS_COORDS,
  MESSAGE,
  FOCUS_CHANGED,
  LAST_SIGNAL
};

struct _GimpToolWidgetPrivate
{
  GimpDisplayShell *shell;
  GimpCanvasItem   *item;
  GList            *group_stack;

  gint              snap_offset_x;
  gint              snap_offset_y;
  gint              snap_width;
  gint              snap_height;

  gboolean          visible;
  gboolean          focus;
};


/*  local function prototypes  */

static void     gimp_tool_widget_finalize           (GObject         *object);
static void     gimp_tool_widget_constructed        (GObject         *object);
static void     gimp_tool_widget_set_property       (GObject         *object,
                                                     guint            property_id,
                                                     const GValue    *value,
                                                     GParamSpec      *pspec);
static void     gimp_tool_widget_get_property       (GObject         *object,
                                                     guint            property_id,
                                                     GValue          *value,
                                                     GParamSpec      *pspec);
static void     gimp_tool_widget_properties_changed (GObject         *object,
                                                     guint            n_pspecs,
                                                     GParamSpec     **pspecs);

static void     gimp_tool_widget_real_leave_notify  (GimpToolWidget  *widget);
static gboolean gimp_tool_widget_real_key_press     (GimpToolWidget  *widget,
                                                     GdkEventKey     *kevent);


G_DEFINE_TYPE_WITH_PRIVATE (GimpToolWidget, gimp_tool_widget, GIMP_TYPE_OBJECT)

#define parent_class gimp_tool_widget_parent_class

static guint widget_signals[LAST_SIGNAL] = { 0 };


static void
gimp_tool_widget_class_init (GimpToolWidgetClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->finalize                    = gimp_tool_widget_finalize;
  object_class->constructed                 = gimp_tool_widget_constructed;
  object_class->set_property                = gimp_tool_widget_set_property;
  object_class->get_property                = gimp_tool_widget_get_property;
  object_class->dispatch_properties_changed = gimp_tool_widget_properties_changed;

  klass->leave_notify                       = gimp_tool_widget_real_leave_notify;
  klass->key_press                          = gimp_tool_widget_real_key_press;

  widget_signals[CHANGED] =
    g_signal_new ("changed",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (GimpToolWidgetClass, changed),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 0);

  widget_signals[RESPONSE] =
    g_signal_new ("response",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (GimpToolWidgetClass, response),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 1,
                  G_TYPE_INT);

  widget_signals[SNAP_OFFSETS] =
    g_signal_new ("snap-offsets",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (GimpToolWidgetClass, snap_offsets),
                  NULL, NULL,
                  gimp_marshal_VOID__INT_INT_INT_INT,
                  G_TYPE_NONE, 4,
                  G_TYPE_INT,
                  G_TYPE_INT,
                  G_TYPE_INT,
                  G_TYPE_INT);

  widget_signals[STATUS] =
    g_signal_new ("status",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (GimpToolWidgetClass, status),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 1,
                  G_TYPE_STRING);

  widget_signals[STATUS_COORDS] =
    g_signal_new ("status-coords",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (GimpToolWidgetClass, status_coords),
                  NULL, NULL,
                  gimp_marshal_VOID__STRING_DOUBLE_STRING_DOUBLE_STRING,
                  G_TYPE_NONE, 5,
                  G_TYPE_STRING,
                  G_TYPE_DOUBLE,
                  G_TYPE_STRING,
                  G_TYPE_DOUBLE,
                  G_TYPE_STRING);

  widget_signals[MESSAGE] =
    g_signal_new ("message",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (GimpToolWidgetClass, message),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 1,
                  G_TYPE_STRING);

  widget_signals[FOCUS_CHANGED] =
    g_signal_new ("focus-changed",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (GimpToolWidgetClass, focus_changed),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 0);

  g_object_class_install_property (object_class, PROP_SHELL,
                                   g_param_spec_object ("shell",
                                                        NULL, NULL,
                                                        GIMP_TYPE_DISPLAY_SHELL,
                                                        GIMP_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT_ONLY));

  g_object_class_install_property (object_class, PROP_ITEM,
                                   g_param_spec_object ("item",
                                                        NULL, NULL,
                                                        GIMP_TYPE_CANVAS_ITEM,
                                                        GIMP_PARAM_READABLE));
}

static void
gimp_tool_widget_init (GimpToolWidget *widget)
{
  widget->private = gimp_tool_widget_get_instance_private (widget);

  widget->private->visible = TRUE;
}

static void
gimp_tool_widget_constructed (GObject *object)
{
  GimpToolWidget        *widget  = GIMP_TOOL_WIDGET (object);
  GimpToolWidgetPrivate *private = widget->private;
  GimpToolWidgetClass   *klass   = GIMP_TOOL_WIDGET_GET_CLASS (widget);

  G_OBJECT_CLASS (parent_class)->constructed (object);

  gimp_assert (GIMP_IS_DISPLAY_SHELL (private->shell));

  private->item = gimp_canvas_group_new (private->shell);

  gimp_canvas_item_set_visible (private->item, private->visible);

  if (klass->changed)
    {
      if (klass->update_on_scale)
        {
          g_signal_connect_object (private->shell, "scaled",
                                   G_CALLBACK (klass->changed),
                                   widget,
                                   G_CONNECT_SWAPPED);
        }

      if (klass->update_on_scroll)
        {
          g_signal_connect_object (private->shell, "scrolled",
                                   G_CALLBACK (klass->changed),
                                   widget,
                                   G_CONNECT_SWAPPED);
        }

      if (klass->update_on_rotate)
        {
          g_signal_connect_object (private->shell, "rotated",
                                   G_CALLBACK (klass->changed),
                                   widget,
                                   G_CONNECT_SWAPPED);
        }
    }
}

static void
gimp_tool_widget_finalize (GObject *object)
{
  GimpToolWidget        *widget  = GIMP_TOOL_WIDGET (object);
  GimpToolWidgetPrivate *private = widget->private;

  g_clear_object (&private->item);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
gimp_tool_widget_set_property (GObject      *object,
                               guint         property_id,
                               const GValue *value,
                               GParamSpec   *pspec)
{
  GimpToolWidget        *widget  = GIMP_TOOL_WIDGET (object);
  GimpToolWidgetPrivate *private = widget->private;

  switch (property_id)
    {
    case PROP_SHELL:
      private->shell = g_value_get_object (value); /* don't ref */
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_tool_widget_get_property (GObject    *object,
                               guint       property_id,
                               GValue     *value,
                               GParamSpec *pspec)
{
  GimpToolWidget        *widget  = GIMP_TOOL_WIDGET (object);
  GimpToolWidgetPrivate *private = widget->private;

  switch (property_id)
    {
    case PROP_SHELL:
      g_value_set_object (value, private->shell);
      break;

    case PROP_ITEM:
      g_value_set_object (value, private->item);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_tool_widget_properties_changed (GObject     *object,
                                     guint        n_pspecs,
                                     GParamSpec **pspecs)
{
  GimpToolWidget *widget = GIMP_TOOL_WIDGET (object);

  G_OBJECT_CLASS (parent_class)->dispatch_properties_changed (object,
                                                              n_pspecs,
                                                              pspecs);

  gimp_tool_widget_changed (widget);
}

static void
gimp_tool_widget_real_leave_notify (GimpToolWidget *widget)
{
  gimp_tool_widget_set_status (widget, NULL);
}

static gboolean
gimp_tool_widget_real_key_press (GimpToolWidget *widget,
                                 GdkEventKey    *kevent)
{
  switch (kevent->keyval)
    {
    case GDK_KEY_Return:
    case GDK_KEY_KP_Enter:
    case GDK_KEY_ISO_Enter:
      gimp_tool_widget_response (widget, GIMP_TOOL_WIDGET_RESPONSE_CONFIRM);
      return TRUE;

    case GDK_KEY_Escape:
      gimp_tool_widget_response (widget, GIMP_TOOL_WIDGET_RESPONSE_CANCEL);
      return TRUE;

    case GDK_KEY_BackSpace:
      gimp_tool_widget_response (widget, GIMP_TOOL_WIDGET_RESPONSE_RESET);
      return TRUE;

    default:
      break;
    }

  return FALSE;
}


/*  public functions  */

GimpDisplayShell *
gimp_tool_widget_get_shell (GimpToolWidget *widget)
{
  g_return_val_if_fail (GIMP_IS_TOOL_WIDGET (widget), NULL);

  return widget->private->shell;
}

GimpCanvasItem *
gimp_tool_widget_get_item (GimpToolWidget *widget)
{
  g_return_val_if_fail (GIMP_IS_TOOL_WIDGET (widget), NULL);

  return widget->private->item;
}

void
gimp_tool_widget_set_visible (GimpToolWidget *widget,
                              gboolean        visible)
{
  g_return_if_fail (GIMP_IS_TOOL_WIDGET (widget));

  if (visible != widget->private->visible)
    {
      widget->private->visible = visible;

      if (widget->private->item)
        gimp_canvas_item_set_visible (widget->private->item, visible);

      if (! visible)
        gimp_tool_widget_set_status (widget, NULL);
    }
}

gboolean
gimp_tool_widget_get_visible (GimpToolWidget *widget)
{
  g_return_val_if_fail (GIMP_IS_TOOL_WIDGET (widget), FALSE);

  return widget->private->visible;
}

void
gimp_tool_widget_set_focus (GimpToolWidget *widget,
                            gboolean        focus)
{
  g_return_if_fail (GIMP_IS_TOOL_WIDGET (widget));

  if (focus != widget->private->focus)
    {
      widget->private->focus = focus;

      g_signal_emit (widget, widget_signals[FOCUS_CHANGED], 0);
    }
}

gboolean
gimp_tool_widget_get_focus (GimpToolWidget *widget)
{
  g_return_val_if_fail (GIMP_IS_TOOL_WIDGET (widget), FALSE);

  return widget->private->focus;
}

void
gimp_tool_widget_changed (GimpToolWidget *widget)
{
  g_return_if_fail (GIMP_IS_TOOL_WIDGET (widget));

  g_signal_emit (widget, widget_signals[CHANGED], 0);
}

void
gimp_tool_widget_response (GimpToolWidget *widget,
                           gint            response_id)
{
  g_return_if_fail (GIMP_IS_TOOL_WIDGET (widget));

  g_signal_emit (widget, widget_signals[RESPONSE], 0,
                 response_id);
}

void
gimp_tool_widget_set_snap_offsets (GimpToolWidget *widget,
                                   gint            offset_x,
                                   gint            offset_y,
                                   gint            width,
                                   gint            height)
{
  GimpToolWidgetPrivate *private;

  g_return_if_fail (GIMP_IS_TOOL_WIDGET (widget));

  private = widget->private;

  if (offset_x != private->snap_offset_x ||
      offset_y != private->snap_offset_y ||
      width    != private->snap_width    ||
      height   != private->snap_height)
    {
      private->snap_offset_x = offset_x;
      private->snap_offset_y = offset_y;
      private->snap_width    = width;
      private->snap_height   = height;

      g_signal_emit (widget, widget_signals[SNAP_OFFSETS], 0,
                 offset_x, offset_y, width, height);
    }
}

void
gimp_tool_widget_get_snap_offsets (GimpToolWidget *widget,
                                   gint           *offset_x,
                                   gint           *offset_y,
                                   gint           *width,
                                   gint           *height)
{
  GimpToolWidgetPrivate *private;

  g_return_if_fail (GIMP_IS_TOOL_WIDGET (widget));

  private = widget->private;

  if (offset_x) *offset_x = private->snap_offset_x;
  if (offset_y) *offset_y = private->snap_offset_y;
  if (width)    *width    = private->snap_width;
  if (height)   *height   = private->snap_height;
}

void
gimp_tool_widget_set_status (GimpToolWidget *widget,
                             const gchar    *status)
{
  g_return_if_fail (GIMP_IS_TOOL_WIDGET (widget));

  g_signal_emit (widget, widget_signals[STATUS], 0,
                 status);
}

void
gimp_tool_widget_set_status_coords (GimpToolWidget *widget,
                                    const gchar    *title,
                                    gdouble         x,
                                    const gchar    *separator,
                                    gdouble         y,
                                    const gchar    *help)
{
  g_return_if_fail (GIMP_IS_TOOL_WIDGET (widget));

  g_signal_emit (widget, widget_signals[STATUS_COORDS], 0,
                 title, x, separator, y, help);
}

void
gimp_tool_widget_message (GimpToolWidget *widget,
                          const gchar    *format,
                          ...)
{
  va_list  args;
  gchar   *message;

  g_return_if_fail (GIMP_IS_TOOL_WIDGET (widget));
  g_return_if_fail (format != NULL);

  va_start (args, format);

  message = g_strdup_vprintf (format, args);

  va_end (args);

  gimp_tool_widget_message_literal (widget, message);

  g_free (message);
}

void
gimp_tool_widget_message_literal (GimpToolWidget *widget,
                                  const gchar    *message)
{
  g_return_if_fail (GIMP_IS_TOOL_WIDGET (widget));
  g_return_if_fail (message != NULL);

  g_signal_emit (widget, widget_signals[MESSAGE], 0,
                 message);
}

void
gimp_tool_widget_add_item (GimpToolWidget *widget,
                           GimpCanvasItem *item)
{
  GimpCanvasGroup *group;

  g_return_if_fail (GIMP_IS_TOOL_WIDGET (widget));
  g_return_if_fail (GIMP_IS_CANVAS_ITEM (item));

  group = GIMP_CANVAS_GROUP (widget->private->item);

  if (widget->private->group_stack)
    group = widget->private->group_stack->data;

  gimp_canvas_group_add_item (group, item);
}

void
gimp_tool_widget_remove_item (GimpToolWidget *widget,
                              GimpCanvasItem *item)
{
  g_return_if_fail (GIMP_IS_TOOL_WIDGET (widget));
  g_return_if_fail (GIMP_IS_CANVAS_ITEM (item));

  gimp_canvas_group_remove_item (GIMP_CANVAS_GROUP (widget->private->item),
                                 item);
}

GimpCanvasGroup *
gimp_tool_widget_add_group (GimpToolWidget *widget)
{
  GimpCanvasItem *item;

  g_return_val_if_fail (GIMP_IS_TOOL_WIDGET (widget), NULL);

  item = gimp_canvas_group_new (widget->private->shell);

  gimp_tool_widget_add_item (widget, item);
  g_object_unref (item);

  return GIMP_CANVAS_GROUP (item);
}

GimpCanvasGroup *
gimp_tool_widget_add_stroke_group (GimpToolWidget *widget)
{
  GimpCanvasGroup *group;

  g_return_val_if_fail (GIMP_IS_TOOL_WIDGET (widget), NULL);

  group = gimp_tool_widget_add_group (widget);
  gimp_canvas_group_set_group_stroking (group, TRUE);

  return group;
}

GimpCanvasGroup *
gimp_tool_widget_add_fill_group (GimpToolWidget *widget)
{
  GimpCanvasGroup *group;

  g_return_val_if_fail (GIMP_IS_TOOL_WIDGET (widget), NULL);

  group = gimp_tool_widget_add_group (widget);
  gimp_canvas_group_set_group_filling (group, TRUE);

  return group;
}

void
gimp_tool_widget_push_group (GimpToolWidget  *widget,
                             GimpCanvasGroup *group)
{
  g_return_if_fail (GIMP_IS_TOOL_WIDGET (widget));
  g_return_if_fail (GIMP_IS_CANVAS_GROUP (group));

  widget->private->group_stack = g_list_prepend (widget->private->group_stack,
                                                 group);
}

void
gimp_tool_widget_pop_group (GimpToolWidget *widget)
{
  g_return_if_fail (GIMP_IS_TOOL_WIDGET (widget));
  g_return_if_fail (widget->private->group_stack != NULL);

  widget->private->group_stack = g_list_remove (widget->private->group_stack,
                                                widget->private->group_stack->data);
}

/**
 * gimp_tool_widget_add_line:
 * @widget: the #GimpToolWidget
 * @x1:     start point X in image coordinates
 * @y1:     start point Y in image coordinates
 * @x2:     end point X in image coordinates
 * @y2:     end point Y in image coordinates
 *
 * This function adds a #GimpCanvasLine to @widget.
 **/
GimpCanvasItem *
gimp_tool_widget_add_line (GimpToolWidget *widget,
                           gdouble         x1,
                           gdouble         y1,
                           gdouble         x2,
                           gdouble         y2)
{
  GimpCanvasItem *item;

  g_return_val_if_fail (GIMP_IS_TOOL_WIDGET (widget), NULL);

  item = gimp_canvas_line_new (widget->private->shell,
                               x1, y1, x2, y2);

  gimp_tool_widget_add_item (widget, item);
  g_object_unref (item);

  return item;
}

GimpCanvasItem *
gimp_tool_widget_add_rectangle (GimpToolWidget *widget,
                                gdouble         x,
                                gdouble         y,
                                gdouble         width,
                                gdouble         height,
                                gboolean        filled)
{
  GimpCanvasItem *item;

  g_return_val_if_fail (GIMP_IS_TOOL_WIDGET (widget), NULL);

  item = gimp_canvas_rectangle_new (widget->private->shell,
                                    x, y, width, height, filled);

  gimp_tool_widget_add_item (widget, item);
  g_object_unref (item);

  return item;
}

GimpCanvasItem *
gimp_tool_widget_add_arc (GimpToolWidget *widget,
                          gdouble         center_x,
                          gdouble         center_y,
                          gdouble         radius_x,
                          gdouble         radius_y,
                          gdouble         start_angle,
                          gdouble         slice_angle,
                          gboolean        filled)
{
  GimpCanvasItem *item;

  g_return_val_if_fail (GIMP_IS_TOOL_WIDGET (widget), NULL);

  item = gimp_canvas_arc_new (widget->private->shell,
                              center_x, center_y,
                              radius_x, radius_y,
                              start_angle, slice_angle,
                              filled);

  gimp_tool_widget_add_item (widget, item);
  g_object_unref (item);

  return item;
}

GimpCanvasItem *
gimp_tool_widget_add_limit (GimpToolWidget *widget,
                            GimpLimitType   type,
                            gdouble         x,
                            gdouble         y,
                            gdouble         radius,
                            gdouble         aspect_ratio,
                            gdouble         angle,
                            gboolean        dashed)
{
  GimpCanvasItem *item;

  g_return_val_if_fail (GIMP_IS_TOOL_WIDGET (widget), NULL);

  item = gimp_canvas_limit_new (widget->private->shell,
                                type,
                                x, y,
                                radius,
                                aspect_ratio,
                                angle,
                                dashed);

  gimp_tool_widget_add_item (widget, item);
  g_object_unref (item);

  return item;
}

GimpCanvasItem *
gimp_tool_widget_add_polygon (GimpToolWidget    *widget,
                              GimpMatrix3       *transform,
                              const GimpVector2 *points,
                              gint               n_points,
                              gboolean           filled)
{
  GimpCanvasItem *item;

  g_return_val_if_fail (GIMP_IS_TOOL_WIDGET (widget), NULL);
  g_return_val_if_fail (points == NULL || n_points > 0, NULL);

  item = gimp_canvas_polygon_new (widget->private->shell,
                                  points, n_points,
                                  transform, filled);

  gimp_tool_widget_add_item (widget, item);
  g_object_unref (item);

  return item;
}

GimpCanvasItem *
gimp_tool_widget_add_polygon_from_coords (GimpToolWidget    *widget,
                                          GimpMatrix3       *transform,
                                          const GimpCoords  *points,
                                          gint               n_points,
                                          gboolean           filled)
{
  GimpCanvasItem *item;

  g_return_val_if_fail (GIMP_IS_TOOL_WIDGET (widget), NULL);
  g_return_val_if_fail (points == NULL || n_points > 0, NULL);

  item = gimp_canvas_polygon_new_from_coords (widget->private->shell,
                                              points, n_points,
                                              transform, filled);

  gimp_tool_widget_add_item (widget, item);
  g_object_unref (item);

  return item;
}

GimpCanvasItem *
gimp_tool_widget_add_path (GimpToolWidget       *widget,
                           const GimpBezierDesc *desc)
{
  GimpCanvasItem *item;

  g_return_val_if_fail (GIMP_IS_TOOL_WIDGET (widget), NULL);

  item = gimp_canvas_path_new (widget->private->shell,
                               desc, 0, 0, FALSE, GIMP_PATH_STYLE_DEFAULT);

  gimp_tool_widget_add_item (widget, item);
  g_object_unref (item);

  return item;
}

GimpCanvasItem *
gimp_tool_widget_add_handle (GimpToolWidget   *widget,
                             GimpHandleType    type,
                             gdouble           x,
                             gdouble           y,
                             gint              width,
                             gint              height,
                             GimpHandleAnchor  anchor)
{
  GimpCanvasItem *item;

  g_return_val_if_fail (GIMP_IS_TOOL_WIDGET (widget), NULL);

  item = gimp_canvas_handle_new (widget->private->shell,
                                 type, anchor, x, y, width, height);

  gimp_tool_widget_add_item (widget, item);
  g_object_unref (item);

  return item;
}

GimpCanvasItem *
gimp_tool_widget_add_corner (GimpToolWidget   *widget,
                             gdouble           x,
                             gdouble           y,
                             gdouble           width,
                             gdouble           height,
                             GimpHandleAnchor  anchor,
                             gint              corner_width,
                             gint              corner_height,
                             gboolean          outside)
{
  GimpCanvasItem *item;

  g_return_val_if_fail (GIMP_IS_TOOL_WIDGET (widget), NULL);

  item = gimp_canvas_corner_new (widget->private->shell,
                                 x, y, width, height,
                                 anchor, corner_width, corner_height,
                                 outside);

  gimp_tool_widget_add_item (widget, item);
  g_object_unref (item);

  return item;
}

GimpCanvasItem *
gimp_tool_widget_add_rectangle_guides (GimpToolWidget *widget,
                                       gdouble         x,
                                       gdouble         y,
                                       gdouble         width,
                                       gdouble         height,
                                       GimpGuidesType  type)
{
  GimpCanvasItem *item;

  g_return_val_if_fail (GIMP_IS_TOOL_WIDGET (widget), NULL);

  item = gimp_canvas_rectangle_guides_new (widget->private->shell,
                                           x, y, width, height, type, 4);

  gimp_tool_widget_add_item (widget, item);
  g_object_unref (item);

  return item;
}

GimpCanvasItem *
gimp_tool_widget_add_transform_guides (GimpToolWidget    *widget,
                                       const GimpMatrix3 *transform,
                                       gdouble            x1,
                                       gdouble            y1,
                                       gdouble            x2,
                                       gdouble            y2,
                                       GimpGuidesType     type,
                                       gint               n_guides,
                                       gboolean           clip)
{
  GimpCanvasItem *item;

  g_return_val_if_fail (GIMP_IS_TOOL_WIDGET (widget), NULL);

  item = gimp_canvas_transform_guides_new (widget->private->shell,
                                           transform, x1, y1, x2, y2,
                                           type, n_guides, clip);

  gimp_tool_widget_add_item (widget, item);
  g_object_unref (item);

  return item;
}

gint
gimp_tool_widget_button_press (GimpToolWidget      *widget,
                               const GimpCoords    *coords,
                               guint32              time,
                               GdkModifierType      state,
                               GimpButtonPressType  press_type)
{
  g_return_val_if_fail (GIMP_IS_TOOL_WIDGET (widget), 0);
  g_return_val_if_fail (coords != NULL, 0);

  if (widget->private->visible &&
      GIMP_TOOL_WIDGET_GET_CLASS (widget)->button_press)
    {
      return GIMP_TOOL_WIDGET_GET_CLASS (widget)->button_press (widget,
                                                                coords, time,
                                                                state,
                                                                press_type);
    }

  return 0;
}

void
gimp_tool_widget_button_release (GimpToolWidget        *widget,
                                 const GimpCoords      *coords,
                                 guint32                time,
                                 GdkModifierType        state,
                                 GimpButtonReleaseType  release_type)
{
  g_return_if_fail (GIMP_IS_TOOL_WIDGET (widget));
  g_return_if_fail (coords != NULL);

  if (widget->private->visible &&
      GIMP_TOOL_WIDGET_GET_CLASS (widget)->button_release)
    {
      GIMP_TOOL_WIDGET_GET_CLASS (widget)->button_release (widget,
                                                           coords, time, state,
                                                           release_type);
    }
}

void
gimp_tool_widget_motion (GimpToolWidget   *widget,
                         const GimpCoords *coords,
                         guint32           time,
                         GdkModifierType   state)
{
  g_return_if_fail (GIMP_IS_TOOL_WIDGET (widget));
  g_return_if_fail (coords != NULL);

  if (widget->private->visible &&
      GIMP_TOOL_WIDGET_GET_CLASS (widget)->motion)
    {
      GIMP_TOOL_WIDGET_GET_CLASS (widget)->motion (widget,
                                                   coords, time, state);
    }
}

GimpHit
gimp_tool_widget_hit (GimpToolWidget   *widget,
                      const GimpCoords *coords,
                      GdkModifierType   state,
                      gboolean          proximity)
{
  g_return_val_if_fail (GIMP_IS_TOOL_WIDGET (widget), GIMP_HIT_NONE);
  g_return_val_if_fail (coords != NULL, GIMP_HIT_NONE);

  if (widget->private->visible &&
      GIMP_TOOL_WIDGET_GET_CLASS (widget)->hit)
    {
      return GIMP_TOOL_WIDGET_GET_CLASS (widget)->hit (widget,
                                                       coords, state,
                                                       proximity);
    }

  return GIMP_HIT_NONE;
}

void
gimp_tool_widget_hover (GimpToolWidget   *widget,
                        const GimpCoords *coords,
                        GdkModifierType   state,
                        gboolean          proximity)
{
  g_return_if_fail (GIMP_IS_TOOL_WIDGET (widget));
  g_return_if_fail (coords != NULL);

  if (widget->private->visible &&
      GIMP_TOOL_WIDGET_GET_CLASS (widget)->hover)
    {
      GIMP_TOOL_WIDGET_GET_CLASS (widget)->hover (widget,
                                                  coords, state, proximity);
    }
}

void
gimp_tool_widget_leave_notify (GimpToolWidget *widget)
{
  g_return_if_fail (GIMP_IS_TOOL_WIDGET (widget));

  if (widget->private->visible &&
      GIMP_TOOL_WIDGET_GET_CLASS (widget)->leave_notify)
    {
      GIMP_TOOL_WIDGET_GET_CLASS (widget)->leave_notify (widget);
    }
}

gboolean
gimp_tool_widget_key_press (GimpToolWidget *widget,
                            GdkEventKey    *kevent)
{
  g_return_val_if_fail (GIMP_IS_TOOL_WIDGET (widget), FALSE);
  g_return_val_if_fail (kevent != NULL, FALSE);

  if (widget->private->visible &&
      GIMP_TOOL_WIDGET_GET_CLASS (widget)->key_press)
    {
      return GIMP_TOOL_WIDGET_GET_CLASS (widget)->key_press (widget, kevent);
    }

  return FALSE;
}

gboolean
gimp_tool_widget_key_release (GimpToolWidget *widget,
                              GdkEventKey    *kevent)
{
  g_return_val_if_fail (GIMP_IS_TOOL_WIDGET (widget), FALSE);
  g_return_val_if_fail (kevent != NULL, FALSE);

  if (widget->private->visible &&
      GIMP_TOOL_WIDGET_GET_CLASS (widget)->key_release)
    {
      return GIMP_TOOL_WIDGET_GET_CLASS (widget)->key_release (widget, kevent);
    }

  return FALSE;
}

void
gimp_tool_widget_motion_modifier (GimpToolWidget  *widget,
                                  GdkModifierType  key,
                                  gboolean         press,
                                  GdkModifierType  state)
{
  g_return_if_fail (GIMP_IS_TOOL_WIDGET (widget));

  if (widget->private->visible &&
      GIMP_TOOL_WIDGET_GET_CLASS (widget)->motion_modifier)
    {
      GIMP_TOOL_WIDGET_GET_CLASS (widget)->motion_modifier (widget,
                                                            key, press, state);
    }
}

void
gimp_tool_widget_hover_modifier (GimpToolWidget  *widget,
                                 GdkModifierType  key,
                                 gboolean         press,
                                 GdkModifierType  state)
{
  g_return_if_fail (GIMP_IS_TOOL_WIDGET (widget));

  if (widget->private->visible &&
      GIMP_TOOL_WIDGET_GET_CLASS (widget)->hover_modifier)
    {
      GIMP_TOOL_WIDGET_GET_CLASS (widget)->hover_modifier (widget,
                                                           key, press, state);
    }
}

gboolean
gimp_tool_widget_get_cursor (GimpToolWidget      *widget,
                             const GimpCoords    *coords,
                             GdkModifierType      state,
                             GimpCursorType      *cursor,
                             GimpToolCursorType  *tool_cursor,
                             GimpCursorModifier  *modifier)

{
  g_return_val_if_fail (GIMP_IS_TOOL_WIDGET (widget), FALSE);
  g_return_val_if_fail (coords != NULL, FALSE);

  if (widget->private->visible &&
      GIMP_TOOL_WIDGET_GET_CLASS (widget)->get_cursor)
    {
      GimpCursorType     my_cursor;
      GimpToolCursorType my_tool_cursor;
      GimpCursorModifier my_modifier;

      if (cursor)      my_cursor      = *cursor;
      if (tool_cursor) my_tool_cursor = *tool_cursor;
      if (modifier)    my_modifier    = *modifier;

      if (GIMP_TOOL_WIDGET_GET_CLASS (widget)->get_cursor (widget, coords,
                                                           state,
                                                           &my_cursor,
                                                           &my_tool_cursor,
                                                           &my_modifier))
        {
          if (cursor)      *cursor      = my_cursor;
          if (tool_cursor) *tool_cursor = my_tool_cursor;
          if (modifier)    *modifier    = my_modifier;

          return TRUE;
        }
    }

  return FALSE;
}

GimpUIManager *
gimp_tool_widget_get_popup (GimpToolWidget        *widget,
                            const GimpCoords      *coords,
                            GdkModifierType        state,
                            const gchar          **ui_path)
{
  g_return_val_if_fail (GIMP_IS_TOOL_WIDGET (widget), NULL);
  g_return_val_if_fail (coords != NULL, NULL);

  if (widget->private->visible &&
      GIMP_TOOL_WIDGET_GET_CLASS (widget)->get_popup)
    {
      return GIMP_TOOL_WIDGET_GET_CLASS (widget)->get_popup (widget, coords,
                                                             state, ui_path);
    }

  return NULL;
}
