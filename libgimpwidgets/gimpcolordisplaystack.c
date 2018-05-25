/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * gimpcolordisplaystack.c
 * Copyright (C) 2003 Michael Natterer <mitch@gimp.org>
 *
 * This library is free software: you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library.  If not, see
 * <http://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <gegl.h>
#include <gtk/gtk.h>

#include "libgimpcolor/gimpcolor.h"

#include "gimpwidgetstypes.h"

#include "gimpcolordisplay.h"
#include "gimpcolordisplaystack.h"
#include "gimpwidgetsmarshal.h"


/**
 * SECTION: gimpcolordisplaystack
 * @title: GimpColorDisplayStack
 * @short_description: A stack of color correction modules.
 * @see_also: #GimpColorDisplay
 *
 * A stack of color correction modules.
 **/


enum
{
  CHANGED,
  ADDED,
  REMOVED,
  REORDERED,
  LAST_SIGNAL
};


struct _GimpColorDisplayStackPrivate
{
  GList *filters;
};

#define GET_PRIVATE(obj) (((GimpColorDisplayStack *) (obj))->priv)


static void   gimp_color_display_stack_dispose         (GObject               *object);

static void   gimp_color_display_stack_display_changed (GimpColorDisplay      *display,
                                                        GimpColorDisplayStack *stack);
static void   gimp_color_display_stack_display_enabled (GimpColorDisplay      *display,
                                                        GParamSpec            *pspec,
                                                        GimpColorDisplayStack *stack);
static void   gimp_color_display_stack_disconnect      (GimpColorDisplayStack *stack,
                                                        GimpColorDisplay      *display);


G_DEFINE_TYPE (GimpColorDisplayStack, gimp_color_display_stack, G_TYPE_OBJECT)

#define parent_class gimp_color_display_stack_parent_class

static guint stack_signals[LAST_SIGNAL] = { 0 };


static void
gimp_color_display_stack_class_init (GimpColorDisplayStackClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  stack_signals[CHANGED] =
    g_signal_new ("changed",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (GimpColorDisplayStackClass, changed),
                  NULL, NULL,
                  g_cclosure_marshal_VOID__VOID,
                  G_TYPE_NONE, 0);

  stack_signals[ADDED] =
    g_signal_new ("added",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (GimpColorDisplayStackClass, added),
                  NULL, NULL,
                  _gimp_widgets_marshal_VOID__OBJECT_INT,
                  G_TYPE_NONE, 2,
                  GIMP_TYPE_COLOR_DISPLAY,
                  G_TYPE_INT);

  stack_signals[REMOVED] =
    g_signal_new ("removed",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (GimpColorDisplayStackClass, removed),
                  NULL, NULL,
                  _gimp_widgets_marshal_VOID__OBJECT,
                  G_TYPE_NONE, 1,
                  GIMP_TYPE_COLOR_DISPLAY);

  stack_signals[REORDERED] =
    g_signal_new ("reordered",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (GimpColorDisplayStackClass, reordered),
                  NULL, NULL,
                  _gimp_widgets_marshal_VOID__OBJECT_INT,
                  G_TYPE_NONE, 2,
                  GIMP_TYPE_COLOR_DISPLAY,
                  G_TYPE_INT);

  object_class->dispose = gimp_color_display_stack_dispose;

  klass->changed        = NULL;
  klass->added          = NULL;
  klass->removed        = NULL;
  klass->reordered      = NULL;

  g_type_class_add_private (object_class, sizeof (GimpColorDisplayStackPrivate));
}

static void
gimp_color_display_stack_init (GimpColorDisplayStack *stack)
{
  stack->priv = G_TYPE_INSTANCE_GET_PRIVATE (stack,
                                             GIMP_TYPE_COLOR_DISPLAY_STACK,
                                             GimpColorDisplayStackPrivate);
}

static void
gimp_color_display_stack_dispose (GObject *object)
{
  GimpColorDisplayStack        *stack   = GIMP_COLOR_DISPLAY_STACK (object);
  GimpColorDisplayStackPrivate *private = GET_PRIVATE (object);

  if (private->filters)
    {
      GList *list;

      for (list = private->filters; list; list = g_list_next (list))
        {
          GimpColorDisplay *display = list->data;

          gimp_color_display_stack_disconnect (stack, display);
          g_object_unref (display);
        }

      g_list_free (private->filters);
      private->filters = NULL;
    }

  G_OBJECT_CLASS (parent_class)->dispose (object);
}


/*  public functions  */

GimpColorDisplayStack *
gimp_color_display_stack_new (void)
{
  return g_object_new (GIMP_TYPE_COLOR_DISPLAY_STACK, NULL);
}

GimpColorDisplayStack *
gimp_color_display_stack_clone (GimpColorDisplayStack *stack)
{
  GimpColorDisplayStackPrivate *private;
  GimpColorDisplayStack        *clone;
  GList                        *list;

  g_return_val_if_fail (GIMP_IS_COLOR_DISPLAY_STACK (stack), NULL);

  private = GET_PRIVATE (stack);

  clone = g_object_new (GIMP_TYPE_COLOR_DISPLAY_STACK, NULL);

  for (list = private->filters; list; list = g_list_next (list))
    {
      GimpColorDisplay *display;

      display = gimp_color_display_clone (list->data);

      gimp_color_display_stack_add (clone, display);
      g_object_unref (display);
    }

  return clone;
}

void
gimp_color_display_stack_changed (GimpColorDisplayStack *stack)
{
  g_return_if_fail (GIMP_IS_COLOR_DISPLAY_STACK (stack));

  g_signal_emit (stack, stack_signals[CHANGED], 0);
}

GList *
gimp_color_display_stack_get_filters (GimpColorDisplayStack *stack)
{
  g_return_val_if_fail (GIMP_IS_COLOR_DISPLAY_STACK (stack), NULL);

  return GET_PRIVATE (stack)->filters;
}

void
gimp_color_display_stack_add (GimpColorDisplayStack *stack,
                              GimpColorDisplay      *display)
{
  GimpColorDisplayStackPrivate *private;

  g_return_if_fail (GIMP_IS_COLOR_DISPLAY_STACK (stack));
  g_return_if_fail (GIMP_IS_COLOR_DISPLAY (display));

  private = GET_PRIVATE (stack);

  g_return_if_fail (g_list_find (private->filters, display) == NULL);

  private->filters = g_list_append (private->filters, g_object_ref (display));

  g_signal_connect (display, "changed",
                    G_CALLBACK (gimp_color_display_stack_display_changed),
                    G_OBJECT (stack));
  g_signal_connect (display, "notify::enabled",
                    G_CALLBACK (gimp_color_display_stack_display_enabled),
                    G_OBJECT (stack));

  g_signal_emit (stack, stack_signals[ADDED], 0,
                 display, g_list_length (private->filters) - 1);

  gimp_color_display_stack_changed (stack);
}

void
gimp_color_display_stack_remove (GimpColorDisplayStack *stack,
                                 GimpColorDisplay      *display)
{
  GimpColorDisplayStackPrivate *private;

  g_return_if_fail (GIMP_IS_COLOR_DISPLAY_STACK (stack));
  g_return_if_fail (GIMP_IS_COLOR_DISPLAY (display));

  private = GET_PRIVATE (stack);

  g_return_if_fail (g_list_find (private->filters, display) != NULL);

  gimp_color_display_stack_disconnect (stack, display);

  private->filters = g_list_remove (private->filters, display);

  g_signal_emit (stack, stack_signals[REMOVED], 0, display);

  gimp_color_display_stack_changed (stack);

  g_object_unref (display);
}

void
gimp_color_display_stack_reorder_up (GimpColorDisplayStack *stack,
                                     GimpColorDisplay      *display)
{
  GimpColorDisplayStackPrivate *private;
  GList                        *list;

  g_return_if_fail (GIMP_IS_COLOR_DISPLAY_STACK (stack));
  g_return_if_fail (GIMP_IS_COLOR_DISPLAY (display));

  private = GET_PRIVATE (stack);

  list = g_list_find (private->filters, display);

  g_return_if_fail (list != NULL);

  if (list->prev)
    {
      list->data       = list->prev->data;
      list->prev->data = display;

      g_signal_emit (stack, stack_signals[REORDERED], 0,
                     display, g_list_position (private->filters, list->prev));

      gimp_color_display_stack_changed (stack);
    }
}

void
gimp_color_display_stack_reorder_down (GimpColorDisplayStack *stack,
                                       GimpColorDisplay      *display)
{
  GimpColorDisplayStackPrivate *private;
  GList                        *list;

  g_return_if_fail (GIMP_IS_COLOR_DISPLAY_STACK (stack));
  g_return_if_fail (GIMP_IS_COLOR_DISPLAY (display));

  private = GET_PRIVATE (stack);

  list = g_list_find (private->filters, display);

  g_return_if_fail (list != NULL);

  if (list->next)
    {
      list->data       = list->next->data;
      list->next->data = display;

      g_signal_emit (stack, stack_signals[REORDERED], 0,
                     display, g_list_position (private->filters, list->next));

      gimp_color_display_stack_changed (stack);
    }
}

/**
 * gimp_color_display_stack_convert_buffer:
 * @stack:  a #GimpColorDisplayStack
 * @buffer: a #GeglBuffer
 * @area:   area of @buffer to convert
 *
 * Runs all the stack's filters on all pixels in @area of @buffer.
 *
 * Since: 2.10
 **/
void
gimp_color_display_stack_convert_buffer (GimpColorDisplayStack *stack,
                                         GeglBuffer            *buffer,
                                         GeglRectangle         *area)
{
  GimpColorDisplayStackPrivate *private;
  GList                        *list;

  g_return_if_fail (GIMP_IS_COLOR_DISPLAY_STACK (stack));
  g_return_if_fail (GEGL_IS_BUFFER (buffer));

  private = GET_PRIVATE (stack);

  for (list = private->filters; list; list = g_list_next (list))
    {
      GimpColorDisplay *display = list->data;

      gimp_color_display_convert_buffer (display, buffer, area);
    }
}


/*  private functions  */

static void
gimp_color_display_stack_display_changed (GimpColorDisplay      *display,
                                          GimpColorDisplayStack *stack)
{
  if (gimp_color_display_get_enabled (display))
    gimp_color_display_stack_changed (stack);
}

static void
gimp_color_display_stack_display_enabled (GimpColorDisplay      *display,
                                          GParamSpec            *pspec,
                                          GimpColorDisplayStack *stack)
{
  gimp_color_display_stack_changed (stack);
}

static void
gimp_color_display_stack_disconnect (GimpColorDisplayStack *stack,
                                     GimpColorDisplay      *display)
{
  g_signal_handlers_disconnect_by_func (display,
                                        gimp_color_display_stack_display_changed,
                                        stack);
  g_signal_handlers_disconnect_by_func (display,
                                        gimp_color_display_stack_display_enabled,
                                        stack);
}
