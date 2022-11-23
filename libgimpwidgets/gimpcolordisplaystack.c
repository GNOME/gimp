/* LIBLIGMA - The LIGMA Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * ligmacolordisplaystack.c
 * Copyright (C) 2003 Michael Natterer <mitch@ligma.org>
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
 * <https://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <gegl.h>
#include <gtk/gtk.h>

#include "libligmacolor/ligmacolor.h"

#include "ligmawidgetstypes.h"

#include "ligmacolordisplay.h"
#include "ligmacolordisplaystack.h"
#include "ligmawidgetsmarshal.h"


/**
 * SECTION: ligmacolordisplaystack
 * @title: LigmaColorDisplayStack
 * @short_description: A stack of color correction modules.
 * @see_also: #LigmaColorDisplay
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


struct _LigmaColorDisplayStackPrivate
{
  GList *filters;
};

#define GET_PRIVATE(obj) (((LigmaColorDisplayStack *) (obj))->priv)


static void   ligma_color_display_stack_dispose         (GObject               *object);

static void   ligma_color_display_stack_display_changed (LigmaColorDisplay      *display,
                                                        LigmaColorDisplayStack *stack);
static void   ligma_color_display_stack_display_enabled (LigmaColorDisplay      *display,
                                                        GParamSpec            *pspec,
                                                        LigmaColorDisplayStack *stack);
static void   ligma_color_display_stack_disconnect      (LigmaColorDisplayStack *stack,
                                                        LigmaColorDisplay      *display);


G_DEFINE_TYPE_WITH_PRIVATE (LigmaColorDisplayStack, ligma_color_display_stack,
                            G_TYPE_OBJECT)

#define parent_class ligma_color_display_stack_parent_class

static guint stack_signals[LAST_SIGNAL] = { 0 };


static void
ligma_color_display_stack_class_init (LigmaColorDisplayStackClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  stack_signals[CHANGED] =
    g_signal_new ("changed",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (LigmaColorDisplayStackClass, changed),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 0);

  stack_signals[ADDED] =
    g_signal_new ("added",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (LigmaColorDisplayStackClass, added),
                  NULL, NULL,
                  _ligma_widgets_marshal_VOID__OBJECT_INT,
                  G_TYPE_NONE, 2,
                  LIGMA_TYPE_COLOR_DISPLAY,
                  G_TYPE_INT);

  stack_signals[REMOVED] =
    g_signal_new ("removed",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (LigmaColorDisplayStackClass, removed),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 1,
                  LIGMA_TYPE_COLOR_DISPLAY);

  stack_signals[REORDERED] =
    g_signal_new ("reordered",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (LigmaColorDisplayStackClass, reordered),
                  NULL, NULL,
                  _ligma_widgets_marshal_VOID__OBJECT_INT,
                  G_TYPE_NONE, 2,
                  LIGMA_TYPE_COLOR_DISPLAY,
                  G_TYPE_INT);

  object_class->dispose = ligma_color_display_stack_dispose;

  klass->changed        = NULL;
  klass->added          = NULL;
  klass->removed        = NULL;
  klass->reordered      = NULL;
}

static void
ligma_color_display_stack_init (LigmaColorDisplayStack *stack)
{
  stack->priv = ligma_color_display_stack_get_instance_private (stack);
}

static void
ligma_color_display_stack_dispose (GObject *object)
{
  LigmaColorDisplayStack        *stack   = LIGMA_COLOR_DISPLAY_STACK (object);
  LigmaColorDisplayStackPrivate *private = GET_PRIVATE (object);

  if (private->filters)
    {
      GList *list;

      for (list = private->filters; list; list = g_list_next (list))
        {
          LigmaColorDisplay *display = list->data;

          ligma_color_display_stack_disconnect (stack, display);
          g_object_unref (display);
        }

      g_list_free (private->filters);
      private->filters = NULL;
    }

  G_OBJECT_CLASS (parent_class)->dispose (object);
}


/*  public functions  */

/**
 * ligma_color_display_stack_new:
 *
 * Creates a new stack of color correction modules.
 *
 * Returns: (transfer full): a newly allocated #LigmaColorDisplayStack.
 *
 * Since: 2.0
 **/
LigmaColorDisplayStack *
ligma_color_display_stack_new (void)
{
  return g_object_new (LIGMA_TYPE_COLOR_DISPLAY_STACK, NULL);
}

/**
 * ligma_color_display_stack_clone:
 * @stack: a #LigmaColorDisplayStack
 *
 * Creates a copy of @stack with all its display color modules also
 * duplicated.
 *
 * Returns: (transfer full): a duplicate of @stack.
 *
 * Since: 2.0
 **/
LigmaColorDisplayStack *
ligma_color_display_stack_clone (LigmaColorDisplayStack *stack)
{
  LigmaColorDisplayStackPrivate *private;
  LigmaColorDisplayStack        *clone;
  GList                        *list;

  g_return_val_if_fail (LIGMA_IS_COLOR_DISPLAY_STACK (stack), NULL);

  private = GET_PRIVATE (stack);

  clone = g_object_new (LIGMA_TYPE_COLOR_DISPLAY_STACK, NULL);

  for (list = private->filters; list; list = g_list_next (list))
    {
      LigmaColorDisplay *display;

      display = ligma_color_display_clone (list->data);

      ligma_color_display_stack_add (clone, display);
      g_object_unref (display);
    }

  return clone;
}

/**
 * ligma_color_display_stack_changed:
 * @stack: a #LigmaColorDisplayStack
 *
 * Emit the "changed" signal of @stack.
 *
 * Since: 2.0
 **/
void
ligma_color_display_stack_changed (LigmaColorDisplayStack *stack)
{
  g_return_if_fail (LIGMA_IS_COLOR_DISPLAY_STACK (stack));

  g_signal_emit (stack, stack_signals[CHANGED], 0);
}

/**
 * ligma_color_display_stack_get_filters:
 * @stack: a #LigmaColorDisplayStack
 *
 * Gets the list of added color modules.
 *
 * Returns: (transfer none) (element-type LigmaColorDisplay):
            the list of @stack's display color modules.
 *
 * Since: 3.0
 **/
GList *
ligma_color_display_stack_get_filters (LigmaColorDisplayStack *stack)
{
  g_return_val_if_fail (LIGMA_IS_COLOR_DISPLAY_STACK (stack), NULL);

  return GET_PRIVATE (stack)->filters;
}

/**
 * ligma_color_display_stack_add:
 * @stack:   a #LigmaColorDisplayStack
 * @display: (transfer none): a #LigmaColorDisplay
 *
 * Add the color module @display to @stack.
 *
 * Since: 2.0
 **/
void
ligma_color_display_stack_add (LigmaColorDisplayStack *stack,
                              LigmaColorDisplay      *display)
{
  LigmaColorDisplayStackPrivate *private;

  g_return_if_fail (LIGMA_IS_COLOR_DISPLAY_STACK (stack));
  g_return_if_fail (LIGMA_IS_COLOR_DISPLAY (display));

  private = GET_PRIVATE (stack);

  g_return_if_fail (g_list_find (private->filters, display) == NULL);

  private->filters = g_list_append (private->filters, g_object_ref (display));

  g_signal_connect (display, "changed",
                    G_CALLBACK (ligma_color_display_stack_display_changed),
                    G_OBJECT (stack));
  g_signal_connect (display, "notify::enabled",
                    G_CALLBACK (ligma_color_display_stack_display_enabled),
                    G_OBJECT (stack));

  g_signal_emit (stack, stack_signals[ADDED], 0,
                 display, g_list_length (private->filters) - 1);

  ligma_color_display_stack_changed (stack);
}

/**
 * ligma_color_display_stack_remove:
 * @stack:   a #LigmaColorDisplayStack
 * @display: (transfer none): a #LigmaColorDisplay
 *
 * Remove the color module @display from @stack.
 *
 * Since: 2.0
 **/
void
ligma_color_display_stack_remove (LigmaColorDisplayStack *stack,
                                 LigmaColorDisplay      *display)
{
  LigmaColorDisplayStackPrivate *private;

  g_return_if_fail (LIGMA_IS_COLOR_DISPLAY_STACK (stack));
  g_return_if_fail (LIGMA_IS_COLOR_DISPLAY (display));

  private = GET_PRIVATE (stack);

  g_return_if_fail (g_list_find (private->filters, display) != NULL);

  ligma_color_display_stack_disconnect (stack, display);

  private->filters = g_list_remove (private->filters, display);

  g_signal_emit (stack, stack_signals[REMOVED], 0, display);

  ligma_color_display_stack_changed (stack);

  g_object_unref (display);
}

/**
 * ligma_color_display_stack_reorder_up:
 * @stack:   a #LigmaColorDisplayStack
 * @display: a #LigmaColorDisplay
 *
 * Move the color module @display up in the filter list of @stack.
 *
 * Since: 2.0
 **/
void
ligma_color_display_stack_reorder_up (LigmaColorDisplayStack *stack,
                                     LigmaColorDisplay      *display)
{
  LigmaColorDisplayStackPrivate *private;
  GList                        *list;

  g_return_if_fail (LIGMA_IS_COLOR_DISPLAY_STACK (stack));
  g_return_if_fail (LIGMA_IS_COLOR_DISPLAY (display));

  private = GET_PRIVATE (stack);

  list = g_list_find (private->filters, display);

  g_return_if_fail (list != NULL);

  if (list->prev)
    {
      list->data       = list->prev->data;
      list->prev->data = display;

      g_signal_emit (stack, stack_signals[REORDERED], 0,
                     display, g_list_position (private->filters, list->prev));

      ligma_color_display_stack_changed (stack);
    }
}

/**
 * ligma_color_display_stack_reorder_down:
 * @stack:   a #LigmaColorDisplayStack
 * @display: a #LigmaColorDisplay
 *
 * Move the color module @display down in the filter list of @stack.
 *
 * Since: 2.0
 **/
void
ligma_color_display_stack_reorder_down (LigmaColorDisplayStack *stack,
                                       LigmaColorDisplay      *display)
{
  LigmaColorDisplayStackPrivate *private;
  GList                        *list;

  g_return_if_fail (LIGMA_IS_COLOR_DISPLAY_STACK (stack));
  g_return_if_fail (LIGMA_IS_COLOR_DISPLAY (display));

  private = GET_PRIVATE (stack);

  list = g_list_find (private->filters, display);

  g_return_if_fail (list != NULL);

  if (list->next)
    {
      list->data       = list->next->data;
      list->next->data = display;

      g_signal_emit (stack, stack_signals[REORDERED], 0,
                     display, g_list_position (private->filters, list->next));

      ligma_color_display_stack_changed (stack);
    }
}

/**
 * ligma_color_display_stack_convert_buffer:
 * @stack:  a #LigmaColorDisplayStack
 * @buffer: a #GeglBuffer
 * @area:   area of @buffer to convert
 *
 * Runs all the stack's filters on all pixels in @area of @buffer.
 *
 * Since: 2.10
 **/
void
ligma_color_display_stack_convert_buffer (LigmaColorDisplayStack *stack,
                                         GeglBuffer            *buffer,
                                         GeglRectangle         *area)
{
  LigmaColorDisplayStackPrivate *private;
  GList                        *list;

  g_return_if_fail (LIGMA_IS_COLOR_DISPLAY_STACK (stack));
  g_return_if_fail (GEGL_IS_BUFFER (buffer));

  private = GET_PRIVATE (stack);

  for (list = private->filters; list; list = g_list_next (list))
    {
      LigmaColorDisplay *display = list->data;

      ligma_color_display_convert_buffer (display, buffer, area);
    }
}


/*  private functions  */

static void
ligma_color_display_stack_display_changed (LigmaColorDisplay      *display,
                                          LigmaColorDisplayStack *stack)
{
  if (ligma_color_display_get_enabled (display))
    ligma_color_display_stack_changed (stack);
}

static void
ligma_color_display_stack_display_enabled (LigmaColorDisplay      *display,
                                          GParamSpec            *pspec,
                                          LigmaColorDisplayStack *stack)
{
  ligma_color_display_stack_changed (stack);
}

static void
ligma_color_display_stack_disconnect (LigmaColorDisplayStack *stack,
                                     LigmaColorDisplay      *display)
{
  g_signal_handlers_disconnect_by_func (display,
                                        ligma_color_display_stack_display_changed,
                                        stack);
  g_signal_handlers_disconnect_by_func (display,
                                        ligma_color_display_stack_display_enabled,
                                        stack);
}
