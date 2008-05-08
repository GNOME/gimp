/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpwindow.c
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include "config.h"

#include <gtk/gtk.h>

#include "widgets-types.h"

#include "gimpwindow.h"


static gboolean  gimp_window_key_press_event (GtkWidget   *widget,
                                              GdkEventKey *kevent);

G_DEFINE_TYPE (GimpWindow, gimp_window, GTK_TYPE_WINDOW)


static void
gimp_window_class_init (GimpWindowClass *klass)
{
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  widget_class->key_press_event = gimp_window_key_press_event;
}

static void
gimp_window_init (GimpWindow *window)
{
}

static gboolean
gimp_window_key_press_event (GtkWidget   *widget,
                             GdkEventKey *event)
{
  GtkWindow *window  = GTK_WINDOW (widget);
  GtkWidget *focus   = gtk_window_get_focus (window);
  gboolean   handled = FALSE;

  /* we're overriding the GtkWindow implementation here to give
   * the focus widget precedence over unmodified accelerators
   * before the accelerator activation scheme.
   */

  /* text widgets get all key events first */
  if (GTK_IS_EDITABLE (focus) || GTK_IS_TEXT_VIEW (focus))
    handled = gtk_window_propagate_key_event (window, event);

  /* invoke control/alt accelerators */
  if (! handled && event->state & (GDK_CONTROL_MASK | GDK_MOD1_MASK))
    handled = gtk_window_activate_key (window, event);

  /* invoke focus widget handlers */
  if (! handled)
    handled = gtk_window_propagate_key_event (window, event);

  /* invoke non-(control/alt) accelerators */
  if (! handled && ! (event->state & (GDK_CONTROL_MASK | GDK_MOD1_MASK)))
    handled = gtk_window_activate_key (window, event);

  /* chain up, bypassing gtk_window_key_press(), to invoke binding set */
  if (! handled)
    {
      GtkWidgetClass *widget_class;

      widget_class = g_type_class_peek_static (g_type_parent (GTK_TYPE_WINDOW));

      handled = widget_class->key_press_event (widget, event);
    }

  return handled;
}
