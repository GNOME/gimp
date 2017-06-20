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
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <gegl.h>
#include <gtk/gtk.h>

#include "display-types.h"

#include "core/gimpmarshal.h"

#include "gimpcanvasgroup.h"
#include "gimpcanvashandle.h"
#include "gimpcanvasline.h"
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
  SNAP_OFFSETS,
  STATUS,
  LAST_SIGNAL
};

struct _GimpToolWidgetPrivate
{
  GimpDisplayShell *shell;
  GimpCanvasItem   *item;
  GList            *group_stack;
};


/*  local function prototypes  */

static void   gimp_tool_widget_finalize           (GObject         *object);
static void   gimp_tool_widget_constructed        (GObject         *object);
static void   gimp_tool_widget_set_property       (GObject         *object,
                                                   guint            property_id,
                                                   const GValue    *value,
                                                   GParamSpec      *pspec);
static void   gimp_tool_widget_get_property       (GObject         *object,
                                                   guint            property_id,
                                                   GValue          *value,
                                                   GParamSpec      *pspec);
static void   gimp_tool_widget_properties_changed (GObject         *object,
                                                   guint            n_pspecs,
                                                   GParamSpec     **pspecs);


G_DEFINE_TYPE (GimpToolWidget, gimp_tool_widget, GIMP_TYPE_OBJECT)

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

  widget_signals[CHANGED] =
    g_signal_new ("changed",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (GimpToolWidgetClass, changed),
                  NULL, NULL,
                  g_cclosure_marshal_VOID__VOID,
                  G_TYPE_NONE, 0);

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
                  NULL, NULL,
                  g_cclosure_marshal_VOID__STRING,
                  G_TYPE_NONE, 1,
                  G_TYPE_STRING);

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

  g_type_class_add_private (klass, sizeof (GimpToolWidgetPrivate));
}

static void
gimp_tool_widget_init (GimpToolWidget *widget)
{
  widget->private = G_TYPE_INSTANCE_GET_PRIVATE (widget,
                                                 GIMP_TYPE_TOOL_WIDGET,
                                                 GimpToolWidgetPrivate);
}

static void
gimp_tool_widget_constructed (GObject *object)
{
  GimpToolWidget        *widget  = GIMP_TOOL_WIDGET (object);
  GimpToolWidgetPrivate *private = widget->private;

  G_OBJECT_CLASS (parent_class)->constructed (object);

  g_assert (GIMP_IS_DISPLAY_SHELL (private->shell));

  private->item = gimp_canvas_group_new (private->shell);
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
  G_OBJECT_CLASS (parent_class)->dispatch_properties_changed (object,
                                                              n_pspecs,
                                                              pspecs);

  g_signal_emit (object, widget_signals[CHANGED], 0);
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
gimp_tool_widget_snap_offsets (GimpToolWidget *widget,
                               gint            offset_x,
                               gint            offset_y,
                               gint            width,
                               gint            height)
{
  g_return_if_fail (GIMP_IS_TOOL_WIDGET (widget));

  g_signal_emit (widget, widget_signals[SNAP_OFFSETS], 0,
                 offset_x, offset_y, width, height);
}

void
gimp_tool_widget_status (GimpToolWidget *widget,
                         const gchar    *status)
{
  g_return_if_fail (GIMP_IS_TOOL_WIDGET (widget));

  g_signal_emit (widget, widget_signals[STATUS], 0,
                 status);
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
gimp_tool_widget_add_stroke_group (GimpToolWidget *widget)
{
  GimpCanvasItem *item;

  g_return_val_if_fail (GIMP_IS_TOOL_WIDGET (widget), NULL);

  item = gimp_canvas_group_new (widget->private->shell);
  gimp_canvas_group_set_group_stroking (GIMP_CANVAS_GROUP (item), TRUE);

  gimp_tool_widget_add_item (widget, item);
  g_object_unref (item);

  return GIMP_CANVAS_GROUP (item);
}

GimpCanvasGroup *
gimp_tool_widget_add_fill_group (GimpToolWidget *widget)
{
  GimpCanvasItem *item;

  g_return_val_if_fail (GIMP_IS_TOOL_WIDGET (widget), NULL);

  item = gimp_canvas_group_new (widget->private->shell);
  gimp_canvas_group_set_group_filling (GIMP_CANVAS_GROUP (item), TRUE);

  gimp_tool_widget_add_item (widget, item);
  g_object_unref (item);

  return GIMP_CANVAS_GROUP (item);
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
gimp_tool_widget_add_transform_guides (GimpToolWidget    *widget,
                                       const GimpMatrix3 *transform,
                                       gdouble            x1,
                                       gdouble            y1,
                                       gdouble            x2,
                                       gdouble            y2,
                                       GimpGuidesType     type,
                                       gint               n_guides)
{
  GimpCanvasItem *item;

  g_return_val_if_fail (GIMP_IS_TOOL_WIDGET (widget), NULL);

  item = gimp_canvas_transform_guides_new (widget->private->shell,
                                           transform, x1, y1, x2, y2,
                                           type, n_guides);

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

gint
gimp_tool_widget_button_press (GimpToolWidget      *widget,
                               const GimpCoords    *coords,
                               guint32              time,
                               GdkModifierType      state,
                               GimpButtonPressType  press_type)
{
  g_return_val_if_fail (GIMP_IS_TOOL_WIDGET (widget), 0);
  g_return_val_if_fail (coords != NULL, 0);

  if (GIMP_TOOL_WIDGET_GET_CLASS (widget)->button_press)
    return GIMP_TOOL_WIDGET_GET_CLASS (widget)->button_press (widget,
                                                              coords, time, state,
                                                              press_type);

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

  if (GIMP_TOOL_WIDGET_GET_CLASS (widget)->button_release)
    GIMP_TOOL_WIDGET_GET_CLASS (widget)->button_release (widget,
                                                         coords, time, state,
                                                         release_type);
}

void
gimp_tool_widget_motion (GimpToolWidget   *widget,
                         const GimpCoords *coords,
                         guint32           time,
                         GdkModifierType   state)
{
  g_return_if_fail (GIMP_IS_TOOL_WIDGET (widget));
  g_return_if_fail (coords != NULL);

  if (GIMP_TOOL_WIDGET_GET_CLASS (widget)->motion)
    GIMP_TOOL_WIDGET_GET_CLASS (widget)->motion (widget,
                                                 coords, time, state);
}

void
gimp_tool_widget_hover (GimpToolWidget   *widget,
                        const GimpCoords *coords,
                        GdkModifierType   state,
                        gboolean          proximity)
{
  g_return_if_fail (GIMP_IS_TOOL_WIDGET (widget));
  g_return_if_fail (coords != NULL);

  if (GIMP_TOOL_WIDGET_GET_CLASS (widget)->hover)
    GIMP_TOOL_WIDGET_GET_CLASS (widget)->hover (widget,
                                                coords, state, proximity);
}

gboolean
gimp_tool_widget_key_press (GimpToolWidget *widget,
                            GdkEventKey    *kevent)
{
  g_return_val_if_fail (GIMP_IS_TOOL_WIDGET (widget), FALSE);
  g_return_val_if_fail (kevent != NULL, FALSE);

  if (GIMP_TOOL_WIDGET_GET_CLASS (widget)->key_press)
    return GIMP_TOOL_WIDGET_GET_CLASS (widget)->key_press (widget, kevent);

  return FALSE;
}

gboolean
gimp_tool_widget_key_release (GimpToolWidget *widget,
                              GdkEventKey    *kevent)
{
  g_return_val_if_fail (GIMP_IS_TOOL_WIDGET (widget), FALSE);
  g_return_val_if_fail (kevent != NULL, FALSE);

  if (GIMP_TOOL_WIDGET_GET_CLASS (widget)->key_release)
    return GIMP_TOOL_WIDGET_GET_CLASS (widget)->key_release (widget, kevent);

  return FALSE;
}

void
gimp_tool_widget_motion_modifier (GimpToolWidget  *widget,
                                  GdkModifierType  key,
                                  gboolean         press,
                                  GdkModifierType  state)
{
  g_return_if_fail (GIMP_IS_TOOL_WIDGET (widget));

  if (GIMP_TOOL_WIDGET_GET_CLASS (widget)->motion_modifier)
    GIMP_TOOL_WIDGET_GET_CLASS (widget)->motion_modifier (widget,
                                                          key, press, state);
}

void
gimp_tool_widget_hover_modifier (GimpToolWidget  *widget,
                                 GdkModifierType  key,
                                 gboolean         press,
                                 GdkModifierType  state)
{
  g_return_if_fail (GIMP_IS_TOOL_WIDGET (widget));

  if (GIMP_TOOL_WIDGET_GET_CLASS (widget)->motion_modifier)
    GIMP_TOOL_WIDGET_GET_CLASS (widget)->motion_modifier (widget,
                                                          key, press, state);
}

gboolean
gimp_tool_widget_get_cursor (GimpToolWidget      *widget,
                             const GimpCoords    *coords,
                             GdkModifierType      state,
                             GimpCursorType      *cursor,
                             GimpToolCursorType  *tool_cursor,
                             GimpCursorModifier  *cursor_modifier)

{
  g_return_val_if_fail (GIMP_IS_TOOL_WIDGET (widget), FALSE);
  g_return_val_if_fail (coords != NULL, FALSE);

  if (GIMP_TOOL_WIDGET_GET_CLASS (widget)->get_cursor)
    {
      GimpCursorType     my_cursor;
      GimpToolCursorType my_tool_cursor;
      GimpCursorModifier my_cursor_modifier;

      if (cursor)          my_cursor          = *cursor;
      if (tool_cursor)     my_tool_cursor     = *tool_cursor;
      if (cursor_modifier) my_cursor_modifier = *cursor_modifier;

      if (GIMP_TOOL_WIDGET_GET_CLASS (widget)->get_cursor (widget, coords,
                                                           state,
                                                           &my_cursor,
                                                           &my_tool_cursor,
                                                           &my_cursor_modifier))
        {
          if (cursor)          *cursor          = my_cursor;
          if (tool_cursor)     *tool_cursor     = my_tool_cursor;
          if (cursor_modifier) *cursor_modifier = my_cursor_modifier;

          return TRUE;
        }
    }

  return FALSE;
}
