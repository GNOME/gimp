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
 * <https://www.gnu.org/licenses/>.
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
 * GimpColorDisplayStack:
 *
 * A stack of color correction modules.
 *
 * Each module is a [class@ColorDisplay] object.
 *
 * You can listen for changes in the list itself using the
 * [signal@Gio.ListModel::items-changed] signal, or _any_ change
 * with the [signal@ColorDisplayStack::changed] signal.
 */


enum
{
  CHANGED,
  ADDED,
  REMOVED,
  REORDERED,
  LAST_SIGNAL
};


struct _GimpColorDisplayStack
{
  GObject    parent;

  GPtrArray *filters;
};


static void   gimp_color_display_stack_dispose         (GObject               *object);

static void   gimp_color_display_stack_display_changed (GimpColorDisplay      *display,
                                                        GimpColorDisplayStack *stack);
static void   gimp_color_display_stack_display_enabled (GimpColorDisplay      *display,
                                                        GParamSpec            *pspec,
                                                        GimpColorDisplayStack *stack);
static void   gimp_color_display_stack_disconnect      (GimpColorDisplayStack *stack,
                                                        GimpColorDisplay      *display);
static void   g_list_model_iface_init                  (GListModelInterface *iface);


G_DEFINE_TYPE_WITH_CODE (GimpColorDisplayStack, gimp_color_display_stack, G_TYPE_OBJECT,
                         G_IMPLEMENT_INTERFACE (G_TYPE_LIST_MODEL,
                                                g_list_model_iface_init))

#define parent_class gimp_color_display_stack_parent_class

static guint stack_signals[LAST_SIGNAL] = { 0 };


static void
gimp_color_display_stack_class_init (GimpColorDisplayStackClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  /**
   * GimpColorDisplayStack::changed:
   *
   * Emitted whenever the display stack or one of its elements changes
   */
  stack_signals[CHANGED] =
    g_signal_new ("changed",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  0,
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 0);

  /**
   * GimpColorDisplayStack::added:
   * @self: this object
   * @display: added display
   * @position: the position
   *
   * Deprecated: 3.1: Use [signal@Gio.ListModel::items-changed] instead.
   */
  stack_signals[ADDED] =
    g_signal_new ("added",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST | G_SIGNAL_DEPRECATED,
                  0,
                  NULL, NULL,
                  _gimp_widgets_marshal_VOID__OBJECT_INT,
                  G_TYPE_NONE, 2,
                  GIMP_TYPE_COLOR_DISPLAY,
                  G_TYPE_INT);

  /**
   * GimpColorDisplayStack::removed:
   * @self: this object
   * @display: removed display
   *
   * Deprecated: 3.1: Use [signal@Gio.ListModel::items-changed] instead.
   */
  stack_signals[REMOVED] =
    g_signal_new ("removed",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST | G_SIGNAL_DEPRECATED,
                  0,
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 1,
                  GIMP_TYPE_COLOR_DISPLAY);

  /**
   * GimpColorDisplayStack::reordered:
   * @self: this object
   * @display: the display
   * @position: the new position
   *
   * Deprecated: 3.1: Use [signal@Gio.ListModel::items-changed] instead.
   */
  stack_signals[REORDERED] =
    g_signal_new ("reordered",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST | G_SIGNAL_DEPRECATED,
                  0,
                  NULL, NULL,
                  _gimp_widgets_marshal_VOID__OBJECT_INT,
                  G_TYPE_NONE, 2,
                  GIMP_TYPE_COLOR_DISPLAY,
                  G_TYPE_INT);

  object_class->dispose = gimp_color_display_stack_dispose;
}

static void
gimp_color_display_stack_init (GimpColorDisplayStack *stack)
{
  stack->filters = g_ptr_array_new_with_free_func (g_object_unref);
}

static void
gimp_color_display_stack_dispose (GObject *object)
{
  GimpColorDisplayStack *stack = GIMP_COLOR_DISPLAY_STACK (object);

  for (guint i = 0; i < stack->filters->len; i++)
    {
      GimpColorDisplay *display = g_ptr_array_index (stack->filters, i);
      gimp_color_display_stack_disconnect (stack, display);
    }

  g_clear_pointer (&stack->filters, g_ptr_array_unref);

  G_OBJECT_CLASS (parent_class)->dispose (object);
}


/*  public functions  */

/**
 * gimp_color_display_stack_new:
 *
 * Creates a new stack of color correction modules.
 *
 * Returns: (transfer full): a newly allocated #GimpColorDisplayStack.
 *
 * Since: 2.0
 **/
GimpColorDisplayStack *
gimp_color_display_stack_new (void)
{
  return g_object_new (GIMP_TYPE_COLOR_DISPLAY_STACK, NULL);
}

/**
 * gimp_color_display_stack_clone:
 * @stack: a #GimpColorDisplayStack
 *
 * Creates a copy of @stack with all its display color modules also
 * duplicated.
 *
 * Returns: (transfer full): a duplicate of @stack.
 *
 * Since: 2.0
 **/
GimpColorDisplayStack *
gimp_color_display_stack_clone (GimpColorDisplayStack *stack)
{
  GimpColorDisplayStack *clone;

  g_return_val_if_fail (GIMP_IS_COLOR_DISPLAY_STACK (stack), NULL);

  clone = g_object_new (GIMP_TYPE_COLOR_DISPLAY_STACK, NULL);

  /*
   * We can't just make a deep clone of the filters array, since we need to
   * make sure to connect the color displays' signals
   */
  for (guint i = 0; i < stack->filters->len; i++)
    {
      GimpColorDisplay *display = g_ptr_array_index (stack->filters, i);
      GimpColorDisplay *display_clone;

      display_clone = gimp_color_display_clone (display);
      gimp_color_display_stack_add (clone, display_clone);
      g_object_unref (display_clone);
    }

  return clone;
}

/**
 * gimp_color_display_stack_changed:
 * @stack: a #GimpColorDisplayStack
 *
 * Emit the "changed" signal of @stack.
 *
 * Since: 2.0
 **/
void
gimp_color_display_stack_changed (GimpColorDisplayStack *stack)
{
  g_return_if_fail (GIMP_IS_COLOR_DISPLAY_STACK (stack));

  g_signal_emit (stack, stack_signals[CHANGED], 0);
}

/**
 * gimp_color_display_stack_get_filters:
 * @stack: a #GimpColorDisplayStack
 *
 * Gets the list of added color modules.
 *
 * Returns: (transfer none) (element-type GimpColorDisplay):
 *          the list of @stack's display color modules.
 *
 * Since: 3.0
 *
 * Deprecated: 3.1: GimpColorDisplayStack now implements [iface@Gio.ListModel],
 *                  use that API instead.
 **/
GList *
gimp_color_display_stack_get_filters (GimpColorDisplayStack *stack)
{
  GList *ret = NULL;

  g_return_val_if_fail (GIMP_IS_COLOR_DISPLAY_STACK (stack), NULL);

  for (gint i = stack->filters->len - 1; i >= 0; i--)
    {
      ret = g_list_prepend (ret, g_ptr_array_index (stack->filters, i));
    }

  return ret;
}

/**
 * gimp_color_display_stack_get_display:
 * @stack: a #GimpColorDisplayStack
 * @index: the index of the given display
 *
 * Returns the #GimpColorDisplay object at @index.
 *
 * Returns: (transfer none): The color display at the given index
 *
 * Since: 3.1
 **/
GimpColorDisplay *
gimp_color_display_stack_get_display (GimpColorDisplayStack *stack,
                                      guint                  index)
{
  g_return_val_if_fail (GIMP_IS_COLOR_DISPLAY_STACK (stack), NULL);
  g_return_val_if_fail (index < stack->filters->len - 1, NULL);

  return g_ptr_array_index (stack->filters, index);
}

/**
 * gimp_color_display_stack_add:
 * @stack:   a #GimpColorDisplayStack
 * @display: (transfer none): a #GimpColorDisplay
 *
 * Add the color module @display to @stack.
 *
 * Since: 2.0
 **/
void
gimp_color_display_stack_add (GimpColorDisplayStack *stack,
                              GimpColorDisplay      *display)
{
  g_return_if_fail (GIMP_IS_COLOR_DISPLAY_STACK (stack));
  g_return_if_fail (GIMP_IS_COLOR_DISPLAY (display));

  g_return_if_fail (!g_ptr_array_find (stack->filters, display, NULL));

  g_ptr_array_add (stack->filters, g_object_ref (display));

  g_signal_connect (display, "changed",
                    G_CALLBACK (gimp_color_display_stack_display_changed),
                    G_OBJECT (stack));
  g_signal_connect (display, "notify::enabled",
                    G_CALLBACK (gimp_color_display_stack_display_enabled),
                    G_OBJECT (stack));

  g_list_model_items_changed (G_LIST_MODEL (stack),
                              stack->filters->len - 1,
                              0, 1);
  g_signal_emit (stack, stack_signals[ADDED], 0,
                 display, stack->filters->len - 1);
  gimp_color_display_stack_changed (stack);
}

/**
 * gimp_color_display_stack_remove:
 * @stack:   a #GimpColorDisplayStack
 * @display: (transfer none): a #GimpColorDisplay
 *
 * Remove the color module @display from @stack.
 *
 * Since: 2.0
 **/
void
gimp_color_display_stack_remove (GimpColorDisplayStack *stack,
                                 GimpColorDisplay      *display)
{
  gboolean found;
  guint    index;

  g_return_if_fail (GIMP_IS_COLOR_DISPLAY_STACK (stack));
  g_return_if_fail (GIMP_IS_COLOR_DISPLAY (display));

  found = g_ptr_array_find (stack->filters, display, &index);
  g_return_if_fail (found);

  gimp_color_display_stack_disconnect (stack, display);

  /* Temporarily ref the display so it's still valid in the ::remove signal */
  g_object_ref (display);

  g_ptr_array_remove_index (stack->filters, index);

  g_list_model_items_changed (G_LIST_MODEL (stack),
                              index, 1, 0);
  g_signal_emit (stack, stack_signals[REMOVED], 0, display);
  gimp_color_display_stack_changed (stack);

  g_object_unref (display);
}

/**
 * gimp_color_display_stack_reorder_up:
 * @stack:   a #GimpColorDisplayStack
 * @display: a #GimpColorDisplay
 *
 * Move the color module @display up in the filter list of @stack.
 *
 * Since: 2.0
 **/
void
gimp_color_display_stack_reorder_up (GimpColorDisplayStack *stack,
                                     GimpColorDisplay      *display)
{
  gboolean found;
  guint    index;

  g_return_if_fail (GIMP_IS_COLOR_DISPLAY_STACK (stack));
  g_return_if_fail (GIMP_IS_COLOR_DISPLAY (display));

  found = g_ptr_array_find (stack->filters, display, &index);
  g_return_if_fail (found);

  if (index == 0)
    return;

  stack->filters->pdata[index] = stack->filters->pdata[index - 1];
  stack->filters->pdata[index - 1] = display;

  g_list_model_items_changed (G_LIST_MODEL (stack),
                              index - 1,
                              2, 2);
  g_signal_emit (stack, stack_signals[REORDERED], 0,
                 display, index - 1);
  gimp_color_display_stack_changed (stack);
}

/**
 * gimp_color_display_stack_reorder_down:
 * @stack:   a #GimpColorDisplayStack
 * @display: a #GimpColorDisplay
 *
 * Move the color module @display down in the filter list of @stack.
 *
 * Since: 2.0
 **/
void
gimp_color_display_stack_reorder_down (GimpColorDisplayStack *stack,
                                       GimpColorDisplay      *display)
{
  gboolean found;
  guint index;

  g_return_if_fail (GIMP_IS_COLOR_DISPLAY_STACK (stack));
  g_return_if_fail (GIMP_IS_COLOR_DISPLAY (display));

  found = g_ptr_array_find (stack->filters, display, &index);
  g_return_if_fail (found);

  if (index == stack->filters->len - 1)
    return;

  stack->filters->pdata[index] = stack->filters->pdata[index + 1];
  stack->filters->pdata[index + 1] = display;

  g_list_model_items_changed (G_LIST_MODEL (stack),
                              index,
                              2, 2);
  g_signal_emit (stack, stack_signals[REORDERED], 0,
                 display, index + 1);
  gimp_color_display_stack_changed (stack);
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
  g_return_if_fail (GIMP_IS_COLOR_DISPLAY_STACK (stack));
  g_return_if_fail (GEGL_IS_BUFFER (buffer));

  for (guint i = 0; i < stack->filters->len; i++)
    {
      GimpColorDisplay *display = g_ptr_array_index (stack->filters, i);

      gimp_color_display_convert_buffer (display, buffer, area);
    }
}

/**
 * gimp_color_display_stack_is_any_enabled:
 *
 * Returns whether the stack contains any enabled filters.
 *
 * Since: 3.1
 */
gboolean
gimp_color_display_stack_is_any_enabled (GimpColorDisplayStack *stack)
{
  g_return_val_if_fail (GIMP_IS_COLOR_DISPLAY_STACK (stack), FALSE);

  for (guint i = 0; i < stack->filters->len; i++)
    {
      GimpColorDisplay *display = g_ptr_array_index (stack->filters, i);
      if (gimp_color_display_get_enabled (display))
        return TRUE;
    }

  return FALSE;
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

/* GListModel implementation */

static GType
gimp_color_display_stack_get_item_type (GListModel *list)
{
  return GIMP_TYPE_COLOR_DISPLAY;
}

static guint
gimp_color_display_stack_get_n_items (GListModel *list)
{
  GimpColorDisplayStack *stack = GIMP_COLOR_DISPLAY_STACK (list);
  return stack->filters->len;
}

static void *
gimp_color_display_stack_get_item (GListModel *list,
                                   guint       index)
{
  GimpColorDisplayStack *stack = GIMP_COLOR_DISPLAY_STACK (list);

  if (index >= stack->filters->len)
    return NULL;
  return g_object_ref (g_ptr_array_index (stack->filters, index));
}

static void
g_list_model_iface_init (GListModelInterface *iface)
{
  iface->get_item_type = gimp_color_display_stack_get_item_type;
  iface->get_n_items   = gimp_color_display_stack_get_n_items;
  iface->get_item      = gimp_color_display_stack_get_item;
}
