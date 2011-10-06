/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpwindow.c
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

#include <gtk/gtk.h>

#include "libgimpwidgets/gimpwidgets.h"

#include "widgets-types.h"

#include "display/display-types.h"
#include "display/gimpcanvas.h"

#include "gimpwindow.h"

#include "gimp-log.h"


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
  GtkWindow       *window = GTK_WINDOW (widget);
  GtkWidget       *focus  = gtk_window_get_focus (window);
  GdkModifierType  accel_mods;
  gboolean         enable_mnemonics;
  gboolean         handled = FALSE;

  /* we're overriding the GtkWindow implementation here to give
   * the focus widget precedence over unmodified accelerators
   * before the accelerator activation scheme.
   */

  /* text widgets get all key events first */
  if (GTK_IS_EDITABLE (focus)  ||
      GTK_IS_TEXT_VIEW (focus) ||
      GIMP_IS_CANVAS (focus))
    {
      handled = gtk_window_propagate_key_event (window, event);

      if (handled)
        GIMP_LOG (KEY_EVENTS,
                  "handled by gtk_window_propagate_key_event(text_widget)");
    }

  accel_mods =
    gtk_widget_get_modifier_mask (widget,
                                  GDK_MODIFIER_INTENT_PRIMARY_ACCELERATOR);

  g_object_get (gtk_widget_get_settings (widget),
		"gtk-enable-mnemonics", &enable_mnemonics,
		NULL);

  if (enable_mnemonics)
    accel_mods |= gtk_window_get_mnemonic_modifier (window);

  /* invoke modified accelerators */
  if (! handled && event->state & accel_mods)
    {
      handled = gtk_window_activate_key (window, event);

      if (handled)
        GIMP_LOG (KEY_EVENTS,
                  "handled by gtk_window_activate_key(modified)");
    }

  /* invoke focus widget handlers */
  if (! handled)
    {
      handled = gtk_window_propagate_key_event (window, event);

      if (handled)
        GIMP_LOG (KEY_EVENTS,
                  "handled by gtk_window_propagate_key_event(other_widget)");
    }

  /* invoke non-modified accelerators */
  if (! handled && ! (event->state & accel_mods))
    {
      handled = gtk_window_activate_key (window, event);

      if (handled)
        GIMP_LOG (KEY_EVENTS,
                  "handled by gtk_window_activate_key(unmodified)");
    }

  /* chain up, bypassing gtk_window_key_press(), to invoke binding set */
  if (! handled)
    {
      GtkWidgetClass *widget_class;

      widget_class = g_type_class_peek_static (g_type_parent (GTK_TYPE_WINDOW));

      handled = widget_class->key_press_event (widget, event);

      if (handled)
        GIMP_LOG (KEY_EVENTS,
                  "handled by widget_class->key_press_event()");
    }

  return handled;
}
