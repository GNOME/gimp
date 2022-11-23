/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ligmatoolwidget.c
 * Copyright (C) 2017 Michael Natterer <mitch@ligma.org>
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

#include "libligmabase/ligmabase.h"

#include "display-types.h"

#include "core/ligmamarshal.h"

#include "ligmacanvasarc.h"
#include "ligmacanvascorner.h"
#include "ligmacanvasgroup.h"
#include "ligmacanvashandle.h"
#include "ligmacanvaslimit.h"
#include "ligmacanvasline.h"
#include "ligmacanvaspath.h"
#include "ligmacanvaspolygon.h"
#include "ligmacanvasrectangle.h"
#include "ligmacanvasrectangleguides.h"
#include "ligmacanvastransformguides.h"
#include "ligmadisplayshell.h"
#include "ligmatoolwidget.h"


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

struct _LigmaToolWidgetPrivate
{
  LigmaDisplayShell *shell;
  LigmaCanvasItem   *item;
  GList            *group_stack;

  gint              snap_offset_x;
  gint              snap_offset_y;
  gint              snap_width;
  gint              snap_height;

  gboolean          visible;
  gboolean          focus;
};


/*  local function prototypes  */

static void     ligma_tool_widget_finalize           (GObject         *object);
static void     ligma_tool_widget_constructed        (GObject         *object);
static void     ligma_tool_widget_set_property       (GObject         *object,
                                                     guint            property_id,
                                                     const GValue    *value,
                                                     GParamSpec      *pspec);
static void     ligma_tool_widget_get_property       (GObject         *object,
                                                     guint            property_id,
                                                     GValue          *value,
                                                     GParamSpec      *pspec);
static void     ligma_tool_widget_properties_changed (GObject         *object,
                                                     guint            n_pspecs,
                                                     GParamSpec     **pspecs);

static void     ligma_tool_widget_real_leave_notify  (LigmaToolWidget  *widget);
static gboolean ligma_tool_widget_real_key_press     (LigmaToolWidget  *widget,
                                                     GdkEventKey     *kevent);


G_DEFINE_TYPE_WITH_PRIVATE (LigmaToolWidget, ligma_tool_widget, LIGMA_TYPE_OBJECT)

#define parent_class ligma_tool_widget_parent_class

static guint widget_signals[LAST_SIGNAL] = { 0 };


static void
ligma_tool_widget_class_init (LigmaToolWidgetClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->finalize                    = ligma_tool_widget_finalize;
  object_class->constructed                 = ligma_tool_widget_constructed;
  object_class->set_property                = ligma_tool_widget_set_property;
  object_class->get_property                = ligma_tool_widget_get_property;
  object_class->dispatch_properties_changed = ligma_tool_widget_properties_changed;

  klass->leave_notify                       = ligma_tool_widget_real_leave_notify;
  klass->key_press                          = ligma_tool_widget_real_key_press;

  widget_signals[CHANGED] =
    g_signal_new ("changed",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (LigmaToolWidgetClass, changed),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 0);

  widget_signals[RESPONSE] =
    g_signal_new ("response",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (LigmaToolWidgetClass, response),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 1,
                  G_TYPE_INT);

  widget_signals[SNAP_OFFSETS] =
    g_signal_new ("snap-offsets",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (LigmaToolWidgetClass, snap_offsets),
                  NULL, NULL,
                  ligma_marshal_VOID__INT_INT_INT_INT,
                  G_TYPE_NONE, 4,
                  G_TYPE_INT,
                  G_TYPE_INT,
                  G_TYPE_INT,
                  G_TYPE_INT);

  widget_signals[STATUS] =
    g_signal_new ("status",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (LigmaToolWidgetClass, status),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 1,
                  G_TYPE_STRING);

  widget_signals[STATUS_COORDS] =
    g_signal_new ("status-coords",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (LigmaToolWidgetClass, status_coords),
                  NULL, NULL,
                  ligma_marshal_VOID__STRING_DOUBLE_STRING_DOUBLE_STRING,
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
                  G_STRUCT_OFFSET (LigmaToolWidgetClass, message),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 1,
                  G_TYPE_STRING);

  widget_signals[FOCUS_CHANGED] =
    g_signal_new ("focus-changed",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (LigmaToolWidgetClass, focus_changed),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 0);

  g_object_class_install_property (object_class, PROP_SHELL,
                                   g_param_spec_object ("shell",
                                                        NULL, NULL,
                                                        LIGMA_TYPE_DISPLAY_SHELL,
                                                        LIGMA_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT_ONLY));

  g_object_class_install_property (object_class, PROP_ITEM,
                                   g_param_spec_object ("item",
                                                        NULL, NULL,
                                                        LIGMA_TYPE_CANVAS_ITEM,
                                                        LIGMA_PARAM_READABLE));
}

static void
ligma_tool_widget_init (LigmaToolWidget *widget)
{
  widget->private = ligma_tool_widget_get_instance_private (widget);

  widget->private->visible = TRUE;
}

static void
ligma_tool_widget_constructed (GObject *object)
{
  LigmaToolWidget        *widget  = LIGMA_TOOL_WIDGET (object);
  LigmaToolWidgetPrivate *private = widget->private;
  LigmaToolWidgetClass   *klass   = LIGMA_TOOL_WIDGET_GET_CLASS (widget);

  G_OBJECT_CLASS (parent_class)->constructed (object);

  ligma_assert (LIGMA_IS_DISPLAY_SHELL (private->shell));

  private->item = ligma_canvas_group_new (private->shell);

  ligma_canvas_item_set_visible (private->item, private->visible);

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
ligma_tool_widget_finalize (GObject *object)
{
  LigmaToolWidget        *widget  = LIGMA_TOOL_WIDGET (object);
  LigmaToolWidgetPrivate *private = widget->private;

  g_clear_object (&private->item);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
ligma_tool_widget_set_property (GObject      *object,
                               guint         property_id,
                               const GValue *value,
                               GParamSpec   *pspec)
{
  LigmaToolWidget        *widget  = LIGMA_TOOL_WIDGET (object);
  LigmaToolWidgetPrivate *private = widget->private;

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
ligma_tool_widget_get_property (GObject    *object,
                               guint       property_id,
                               GValue     *value,
                               GParamSpec *pspec)
{
  LigmaToolWidget        *widget  = LIGMA_TOOL_WIDGET (object);
  LigmaToolWidgetPrivate *private = widget->private;

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
ligma_tool_widget_properties_changed (GObject     *object,
                                     guint        n_pspecs,
                                     GParamSpec **pspecs)
{
  LigmaToolWidget *widget = LIGMA_TOOL_WIDGET (object);

  G_OBJECT_CLASS (parent_class)->dispatch_properties_changed (object,
                                                              n_pspecs,
                                                              pspecs);

  ligma_tool_widget_changed (widget);
}

static void
ligma_tool_widget_real_leave_notify (LigmaToolWidget *widget)
{
  ligma_tool_widget_set_status (widget, NULL);
}

static gboolean
ligma_tool_widget_real_key_press (LigmaToolWidget *widget,
                                 GdkEventKey    *kevent)
{
  switch (kevent->keyval)
    {
    case GDK_KEY_Return:
    case GDK_KEY_KP_Enter:
    case GDK_KEY_ISO_Enter:
      ligma_tool_widget_response (widget, LIGMA_TOOL_WIDGET_RESPONSE_CONFIRM);
      return TRUE;

    case GDK_KEY_Escape:
      ligma_tool_widget_response (widget, LIGMA_TOOL_WIDGET_RESPONSE_CANCEL);
      return TRUE;

    case GDK_KEY_BackSpace:
      ligma_tool_widget_response (widget, LIGMA_TOOL_WIDGET_RESPONSE_RESET);
      return TRUE;

    default:
      break;
    }

  return FALSE;
}


/*  public functions  */

LigmaDisplayShell *
ligma_tool_widget_get_shell (LigmaToolWidget *widget)
{
  g_return_val_if_fail (LIGMA_IS_TOOL_WIDGET (widget), NULL);

  return widget->private->shell;
}

LigmaCanvasItem *
ligma_tool_widget_get_item (LigmaToolWidget *widget)
{
  g_return_val_if_fail (LIGMA_IS_TOOL_WIDGET (widget), NULL);

  return widget->private->item;
}

void
ligma_tool_widget_set_visible (LigmaToolWidget *widget,
                              gboolean        visible)
{
  g_return_if_fail (LIGMA_IS_TOOL_WIDGET (widget));

  if (visible != widget->private->visible)
    {
      widget->private->visible = visible;

      if (widget->private->item)
        ligma_canvas_item_set_visible (widget->private->item, visible);

      if (! visible)
        ligma_tool_widget_set_status (widget, NULL);
    }
}

gboolean
ligma_tool_widget_get_visible (LigmaToolWidget *widget)
{
  g_return_val_if_fail (LIGMA_IS_TOOL_WIDGET (widget), FALSE);

  return widget->private->visible;
}

void
ligma_tool_widget_set_focus (LigmaToolWidget *widget,
                            gboolean        focus)
{
  g_return_if_fail (LIGMA_IS_TOOL_WIDGET (widget));

  if (focus != widget->private->focus)
    {
      widget->private->focus = focus;

      g_signal_emit (widget, widget_signals[FOCUS_CHANGED], 0);
    }
}

gboolean
ligma_tool_widget_get_focus (LigmaToolWidget *widget)
{
  g_return_val_if_fail (LIGMA_IS_TOOL_WIDGET (widget), FALSE);

  return widget->private->focus;
}

void
ligma_tool_widget_changed (LigmaToolWidget *widget)
{
  g_return_if_fail (LIGMA_IS_TOOL_WIDGET (widget));

  g_signal_emit (widget, widget_signals[CHANGED], 0);
}

void
ligma_tool_widget_response (LigmaToolWidget *widget,
                           gint            response_id)
{
  g_return_if_fail (LIGMA_IS_TOOL_WIDGET (widget));

  g_signal_emit (widget, widget_signals[RESPONSE], 0,
                 response_id);
}

void
ligma_tool_widget_set_snap_offsets (LigmaToolWidget *widget,
                                   gint            offset_x,
                                   gint            offset_y,
                                   gint            width,
                                   gint            height)
{
  LigmaToolWidgetPrivate *private;

  g_return_if_fail (LIGMA_IS_TOOL_WIDGET (widget));

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
ligma_tool_widget_get_snap_offsets (LigmaToolWidget *widget,
                                   gint           *offset_x,
                                   gint           *offset_y,
                                   gint           *width,
                                   gint           *height)
{
  LigmaToolWidgetPrivate *private;

  g_return_if_fail (LIGMA_IS_TOOL_WIDGET (widget));

  private = widget->private;

  if (offset_x) *offset_x = private->snap_offset_x;
  if (offset_y) *offset_y = private->snap_offset_y;
  if (width)    *width    = private->snap_width;
  if (height)   *height   = private->snap_height;
}

void
ligma_tool_widget_set_status (LigmaToolWidget *widget,
                             const gchar    *status)
{
  g_return_if_fail (LIGMA_IS_TOOL_WIDGET (widget));

  g_signal_emit (widget, widget_signals[STATUS], 0,
                 status);
}

void
ligma_tool_widget_set_status_coords (LigmaToolWidget *widget,
                                    const gchar    *title,
                                    gdouble         x,
                                    const gchar    *separator,
                                    gdouble         y,
                                    const gchar    *help)
{
  g_return_if_fail (LIGMA_IS_TOOL_WIDGET (widget));

  g_signal_emit (widget, widget_signals[STATUS_COORDS], 0,
                 title, x, separator, y, help);
}

void
ligma_tool_widget_message (LigmaToolWidget *widget,
                          const gchar    *format,
                          ...)
{
  va_list  args;
  gchar   *message;

  g_return_if_fail (LIGMA_IS_TOOL_WIDGET (widget));
  g_return_if_fail (format != NULL);

  va_start (args, format);

  message = g_strdup_vprintf (format, args);

  va_end (args);

  ligma_tool_widget_message_literal (widget, message);

  g_free (message);
}

void
ligma_tool_widget_message_literal (LigmaToolWidget *widget,
                                  const gchar    *message)
{
  g_return_if_fail (LIGMA_IS_TOOL_WIDGET (widget));
  g_return_if_fail (message != NULL);

  g_signal_emit (widget, widget_signals[MESSAGE], 0,
                 message);
}

void
ligma_tool_widget_add_item (LigmaToolWidget *widget,
                           LigmaCanvasItem *item)
{
  LigmaCanvasGroup *group;

  g_return_if_fail (LIGMA_IS_TOOL_WIDGET (widget));
  g_return_if_fail (LIGMA_IS_CANVAS_ITEM (item));

  group = LIGMA_CANVAS_GROUP (widget->private->item);

  if (widget->private->group_stack)
    group = widget->private->group_stack->data;

  ligma_canvas_group_add_item (group, item);
}

void
ligma_tool_widget_remove_item (LigmaToolWidget *widget,
                              LigmaCanvasItem *item)
{
  g_return_if_fail (LIGMA_IS_TOOL_WIDGET (widget));
  g_return_if_fail (LIGMA_IS_CANVAS_ITEM (item));

  ligma_canvas_group_remove_item (LIGMA_CANVAS_GROUP (widget->private->item),
                                 item);
}

LigmaCanvasGroup *
ligma_tool_widget_add_group (LigmaToolWidget *widget)
{
  LigmaCanvasItem *item;

  g_return_val_if_fail (LIGMA_IS_TOOL_WIDGET (widget), NULL);

  item = ligma_canvas_group_new (widget->private->shell);

  ligma_tool_widget_add_item (widget, item);
  g_object_unref (item);

  return LIGMA_CANVAS_GROUP (item);
}

LigmaCanvasGroup *
ligma_tool_widget_add_stroke_group (LigmaToolWidget *widget)
{
  LigmaCanvasGroup *group;

  g_return_val_if_fail (LIGMA_IS_TOOL_WIDGET (widget), NULL);

  group = ligma_tool_widget_add_group (widget);
  ligma_canvas_group_set_group_stroking (group, TRUE);

  return group;
}

LigmaCanvasGroup *
ligma_tool_widget_add_fill_group (LigmaToolWidget *widget)
{
  LigmaCanvasGroup *group;

  g_return_val_if_fail (LIGMA_IS_TOOL_WIDGET (widget), NULL);

  group = ligma_tool_widget_add_group (widget);
  ligma_canvas_group_set_group_filling (group, TRUE);

  return group;
}

void
ligma_tool_widget_push_group (LigmaToolWidget  *widget,
                             LigmaCanvasGroup *group)
{
  g_return_if_fail (LIGMA_IS_TOOL_WIDGET (widget));
  g_return_if_fail (LIGMA_IS_CANVAS_GROUP (group));

  widget->private->group_stack = g_list_prepend (widget->private->group_stack,
                                                 group);
}

void
ligma_tool_widget_pop_group (LigmaToolWidget *widget)
{
  g_return_if_fail (LIGMA_IS_TOOL_WIDGET (widget));
  g_return_if_fail (widget->private->group_stack != NULL);

  widget->private->group_stack = g_list_remove (widget->private->group_stack,
                                                widget->private->group_stack->data);
}

/**
 * ligma_tool_widget_add_line:
 * @widget: the #LigmaToolWidget
 * @x1:     start point X in image coordinates
 * @y1:     start point Y in image coordinates
 * @x2:     end point X in image coordinates
 * @y2:     end point Y in image coordinates
 *
 * This function adds a #LigmaCanvasLine to @widget.
 **/
LigmaCanvasItem *
ligma_tool_widget_add_line (LigmaToolWidget *widget,
                           gdouble         x1,
                           gdouble         y1,
                           gdouble         x2,
                           gdouble         y2)
{
  LigmaCanvasItem *item;

  g_return_val_if_fail (LIGMA_IS_TOOL_WIDGET (widget), NULL);

  item = ligma_canvas_line_new (widget->private->shell,
                               x1, y1, x2, y2);

  ligma_tool_widget_add_item (widget, item);
  g_object_unref (item);

  return item;
}

LigmaCanvasItem *
ligma_tool_widget_add_rectangle (LigmaToolWidget *widget,
                                gdouble         x,
                                gdouble         y,
                                gdouble         width,
                                gdouble         height,
                                gboolean        filled)
{
  LigmaCanvasItem *item;

  g_return_val_if_fail (LIGMA_IS_TOOL_WIDGET (widget), NULL);

  item = ligma_canvas_rectangle_new (widget->private->shell,
                                    x, y, width, height, filled);

  ligma_tool_widget_add_item (widget, item);
  g_object_unref (item);

  return item;
}

LigmaCanvasItem *
ligma_tool_widget_add_arc (LigmaToolWidget *widget,
                          gdouble         center_x,
                          gdouble         center_y,
                          gdouble         radius_x,
                          gdouble         radius_y,
                          gdouble         start_angle,
                          gdouble         slice_angle,
                          gboolean        filled)
{
  LigmaCanvasItem *item;

  g_return_val_if_fail (LIGMA_IS_TOOL_WIDGET (widget), NULL);

  item = ligma_canvas_arc_new (widget->private->shell,
                              center_x, center_y,
                              radius_x, radius_y,
                              start_angle, slice_angle,
                              filled);

  ligma_tool_widget_add_item (widget, item);
  g_object_unref (item);

  return item;
}

LigmaCanvasItem *
ligma_tool_widget_add_limit (LigmaToolWidget *widget,
                            LigmaLimitType   type,
                            gdouble         x,
                            gdouble         y,
                            gdouble         radius,
                            gdouble         aspect_ratio,
                            gdouble         angle,
                            gboolean        dashed)
{
  LigmaCanvasItem *item;

  g_return_val_if_fail (LIGMA_IS_TOOL_WIDGET (widget), NULL);

  item = ligma_canvas_limit_new (widget->private->shell,
                                type,
                                x, y,
                                radius,
                                aspect_ratio,
                                angle,
                                dashed);

  ligma_tool_widget_add_item (widget, item);
  g_object_unref (item);

  return item;
}

LigmaCanvasItem *
ligma_tool_widget_add_polygon (LigmaToolWidget    *widget,
                              LigmaMatrix3       *transform,
                              const LigmaVector2 *points,
                              gint               n_points,
                              gboolean           filled)
{
  LigmaCanvasItem *item;

  g_return_val_if_fail (LIGMA_IS_TOOL_WIDGET (widget), NULL);
  g_return_val_if_fail (points == NULL || n_points > 0, NULL);

  item = ligma_canvas_polygon_new (widget->private->shell,
                                  points, n_points,
                                  transform, filled);

  ligma_tool_widget_add_item (widget, item);
  g_object_unref (item);

  return item;
}

LigmaCanvasItem *
ligma_tool_widget_add_polygon_from_coords (LigmaToolWidget    *widget,
                                          LigmaMatrix3       *transform,
                                          const LigmaCoords  *points,
                                          gint               n_points,
                                          gboolean           filled)
{
  LigmaCanvasItem *item;

  g_return_val_if_fail (LIGMA_IS_TOOL_WIDGET (widget), NULL);
  g_return_val_if_fail (points == NULL || n_points > 0, NULL);

  item = ligma_canvas_polygon_new_from_coords (widget->private->shell,
                                              points, n_points,
                                              transform, filled);

  ligma_tool_widget_add_item (widget, item);
  g_object_unref (item);

  return item;
}

LigmaCanvasItem *
ligma_tool_widget_add_path (LigmaToolWidget       *widget,
                           const LigmaBezierDesc *desc)
{
  LigmaCanvasItem *item;

  g_return_val_if_fail (LIGMA_IS_TOOL_WIDGET (widget), NULL);

  item = ligma_canvas_path_new (widget->private->shell,
                               desc, 0, 0, FALSE, LIGMA_PATH_STYLE_DEFAULT);

  ligma_tool_widget_add_item (widget, item);
  g_object_unref (item);

  return item;
}

LigmaCanvasItem *
ligma_tool_widget_add_handle (LigmaToolWidget   *widget,
                             LigmaHandleType    type,
                             gdouble           x,
                             gdouble           y,
                             gint              width,
                             gint              height,
                             LigmaHandleAnchor  anchor)
{
  LigmaCanvasItem *item;

  g_return_val_if_fail (LIGMA_IS_TOOL_WIDGET (widget), NULL);

  item = ligma_canvas_handle_new (widget->private->shell,
                                 type, anchor, x, y, width, height);

  ligma_tool_widget_add_item (widget, item);
  g_object_unref (item);

  return item;
}

LigmaCanvasItem *
ligma_tool_widget_add_corner (LigmaToolWidget   *widget,
                             gdouble           x,
                             gdouble           y,
                             gdouble           width,
                             gdouble           height,
                             LigmaHandleAnchor  anchor,
                             gint              corner_width,
                             gint              corner_height,
                             gboolean          outside)
{
  LigmaCanvasItem *item;

  g_return_val_if_fail (LIGMA_IS_TOOL_WIDGET (widget), NULL);

  item = ligma_canvas_corner_new (widget->private->shell,
                                 x, y, width, height,
                                 anchor, corner_width, corner_height,
                                 outside);

  ligma_tool_widget_add_item (widget, item);
  g_object_unref (item);

  return item;
}

LigmaCanvasItem *
ligma_tool_widget_add_rectangle_guides (LigmaToolWidget *widget,
                                       gdouble         x,
                                       gdouble         y,
                                       gdouble         width,
                                       gdouble         height,
                                       LigmaGuidesType  type)
{
  LigmaCanvasItem *item;

  g_return_val_if_fail (LIGMA_IS_TOOL_WIDGET (widget), NULL);

  item = ligma_canvas_rectangle_guides_new (widget->private->shell,
                                           x, y, width, height, type, 4);

  ligma_tool_widget_add_item (widget, item);
  g_object_unref (item);

  return item;
}

LigmaCanvasItem *
ligma_tool_widget_add_transform_guides (LigmaToolWidget    *widget,
                                       const LigmaMatrix3 *transform,
                                       gdouble            x1,
                                       gdouble            y1,
                                       gdouble            x2,
                                       gdouble            y2,
                                       LigmaGuidesType     type,
                                       gint               n_guides,
                                       gboolean           clip)
{
  LigmaCanvasItem *item;

  g_return_val_if_fail (LIGMA_IS_TOOL_WIDGET (widget), NULL);

  item = ligma_canvas_transform_guides_new (widget->private->shell,
                                           transform, x1, y1, x2, y2,
                                           type, n_guides, clip);

  ligma_tool_widget_add_item (widget, item);
  g_object_unref (item);

  return item;
}

gint
ligma_tool_widget_button_press (LigmaToolWidget      *widget,
                               const LigmaCoords    *coords,
                               guint32              time,
                               GdkModifierType      state,
                               LigmaButtonPressType  press_type)
{
  g_return_val_if_fail (LIGMA_IS_TOOL_WIDGET (widget), 0);
  g_return_val_if_fail (coords != NULL, 0);

  if (widget->private->visible &&
      LIGMA_TOOL_WIDGET_GET_CLASS (widget)->button_press)
    {
      return LIGMA_TOOL_WIDGET_GET_CLASS (widget)->button_press (widget,
                                                                coords, time,
                                                                state,
                                                                press_type);
    }

  return 0;
}

void
ligma_tool_widget_button_release (LigmaToolWidget        *widget,
                                 const LigmaCoords      *coords,
                                 guint32                time,
                                 GdkModifierType        state,
                                 LigmaButtonReleaseType  release_type)
{
  g_return_if_fail (LIGMA_IS_TOOL_WIDGET (widget));
  g_return_if_fail (coords != NULL);

  if (widget->private->visible &&
      LIGMA_TOOL_WIDGET_GET_CLASS (widget)->button_release)
    {
      LIGMA_TOOL_WIDGET_GET_CLASS (widget)->button_release (widget,
                                                           coords, time, state,
                                                           release_type);
    }
}

void
ligma_tool_widget_motion (LigmaToolWidget   *widget,
                         const LigmaCoords *coords,
                         guint32           time,
                         GdkModifierType   state)
{
  g_return_if_fail (LIGMA_IS_TOOL_WIDGET (widget));
  g_return_if_fail (coords != NULL);

  if (widget->private->visible &&
      LIGMA_TOOL_WIDGET_GET_CLASS (widget)->motion)
    {
      LIGMA_TOOL_WIDGET_GET_CLASS (widget)->motion (widget,
                                                   coords, time, state);
    }
}

LigmaHit
ligma_tool_widget_hit (LigmaToolWidget   *widget,
                      const LigmaCoords *coords,
                      GdkModifierType   state,
                      gboolean          proximity)
{
  g_return_val_if_fail (LIGMA_IS_TOOL_WIDGET (widget), LIGMA_HIT_NONE);
  g_return_val_if_fail (coords != NULL, LIGMA_HIT_NONE);

  if (widget->private->visible &&
      LIGMA_TOOL_WIDGET_GET_CLASS (widget)->hit)
    {
      return LIGMA_TOOL_WIDGET_GET_CLASS (widget)->hit (widget,
                                                       coords, state,
                                                       proximity);
    }

  return LIGMA_HIT_NONE;
}

void
ligma_tool_widget_hover (LigmaToolWidget   *widget,
                        const LigmaCoords *coords,
                        GdkModifierType   state,
                        gboolean          proximity)
{
  g_return_if_fail (LIGMA_IS_TOOL_WIDGET (widget));
  g_return_if_fail (coords != NULL);

  if (widget->private->visible &&
      LIGMA_TOOL_WIDGET_GET_CLASS (widget)->hover)
    {
      LIGMA_TOOL_WIDGET_GET_CLASS (widget)->hover (widget,
                                                  coords, state, proximity);
    }
}

void
ligma_tool_widget_leave_notify (LigmaToolWidget *widget)
{
  g_return_if_fail (LIGMA_IS_TOOL_WIDGET (widget));

  if (widget->private->visible &&
      LIGMA_TOOL_WIDGET_GET_CLASS (widget)->leave_notify)
    {
      LIGMA_TOOL_WIDGET_GET_CLASS (widget)->leave_notify (widget);
    }
}

gboolean
ligma_tool_widget_key_press (LigmaToolWidget *widget,
                            GdkEventKey    *kevent)
{
  g_return_val_if_fail (LIGMA_IS_TOOL_WIDGET (widget), FALSE);
  g_return_val_if_fail (kevent != NULL, FALSE);

  if (widget->private->visible &&
      LIGMA_TOOL_WIDGET_GET_CLASS (widget)->key_press)
    {
      return LIGMA_TOOL_WIDGET_GET_CLASS (widget)->key_press (widget, kevent);
    }

  return FALSE;
}

gboolean
ligma_tool_widget_key_release (LigmaToolWidget *widget,
                              GdkEventKey    *kevent)
{
  g_return_val_if_fail (LIGMA_IS_TOOL_WIDGET (widget), FALSE);
  g_return_val_if_fail (kevent != NULL, FALSE);

  if (widget->private->visible &&
      LIGMA_TOOL_WIDGET_GET_CLASS (widget)->key_release)
    {
      return LIGMA_TOOL_WIDGET_GET_CLASS (widget)->key_release (widget, kevent);
    }

  return FALSE;
}

void
ligma_tool_widget_motion_modifier (LigmaToolWidget  *widget,
                                  GdkModifierType  key,
                                  gboolean         press,
                                  GdkModifierType  state)
{
  g_return_if_fail (LIGMA_IS_TOOL_WIDGET (widget));

  if (widget->private->visible &&
      LIGMA_TOOL_WIDGET_GET_CLASS (widget)->motion_modifier)
    {
      LIGMA_TOOL_WIDGET_GET_CLASS (widget)->motion_modifier (widget,
                                                            key, press, state);
    }
}

void
ligma_tool_widget_hover_modifier (LigmaToolWidget  *widget,
                                 GdkModifierType  key,
                                 gboolean         press,
                                 GdkModifierType  state)
{
  g_return_if_fail (LIGMA_IS_TOOL_WIDGET (widget));

  if (widget->private->visible &&
      LIGMA_TOOL_WIDGET_GET_CLASS (widget)->hover_modifier)
    {
      LIGMA_TOOL_WIDGET_GET_CLASS (widget)->hover_modifier (widget,
                                                           key, press, state);
    }
}

gboolean
ligma_tool_widget_get_cursor (LigmaToolWidget      *widget,
                             const LigmaCoords    *coords,
                             GdkModifierType      state,
                             LigmaCursorType      *cursor,
                             LigmaToolCursorType  *tool_cursor,
                             LigmaCursorModifier  *modifier)

{
  g_return_val_if_fail (LIGMA_IS_TOOL_WIDGET (widget), FALSE);
  g_return_val_if_fail (coords != NULL, FALSE);

  if (widget->private->visible &&
      LIGMA_TOOL_WIDGET_GET_CLASS (widget)->get_cursor)
    {
      LigmaCursorType     my_cursor;
      LigmaToolCursorType my_tool_cursor;
      LigmaCursorModifier my_modifier;

      if (cursor)      my_cursor      = *cursor;
      if (tool_cursor) my_tool_cursor = *tool_cursor;
      if (modifier)    my_modifier    = *modifier;

      if (LIGMA_TOOL_WIDGET_GET_CLASS (widget)->get_cursor (widget, coords,
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

LigmaUIManager *
ligma_tool_widget_get_popup (LigmaToolWidget        *widget,
                            const LigmaCoords      *coords,
                            GdkModifierType        state,
                            const gchar          **ui_path)
{
  g_return_val_if_fail (LIGMA_IS_TOOL_WIDGET (widget), NULL);
  g_return_val_if_fail (coords != NULL, NULL);

  if (widget->private->visible &&
      LIGMA_TOOL_WIDGET_GET_CLASS (widget)->get_popup)
    {
      return LIGMA_TOOL_WIDGET_GET_CLASS (widget)->get_popup (widget, coords,
                                                             state, ui_path);
    }

  return NULL;
}
