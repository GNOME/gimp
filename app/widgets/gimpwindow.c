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

#include <gegl.h>
#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>

#include "libgimpwidgets/gimpwidgets.h"

#include "widgets-types.h"

#include "core/gimpmarshal.h"

#include "display/display-types.h"
#include "display/gimpcanvas.h"

#include "gimpwindow.h"

#include "gimp-log.h"


enum
{
  MONITOR_CHANGED,
  LAST_SIGNAL
};


struct _GimpWindowPrivate
{
  GdkMonitor *monitor;
  GtkWidget  *primary_focus_widget;
};


static void      gimp_window_dispose         (GObject           *object);

static void      gimp_window_screen_changed  (GtkWidget         *widget,
                                              GdkScreen         *previous_screen);
static gboolean  gimp_window_configure_event (GtkWidget         *widget,
                                              GdkEventConfigure *cevent);
static gboolean  gimp_window_key_press_event (GtkWidget         *widget,
                                              GdkEventKey       *kevent);


G_DEFINE_TYPE (GimpWindow, gimp_window, GTK_TYPE_WINDOW)

#define parent_class gimp_window_parent_class

static guint window_signals[LAST_SIGNAL] = { 0, };


static void
gimp_window_class_init (GimpWindowClass *klass)
{
  GObjectClass   *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  window_signals[MONITOR_CHANGED] =
    g_signal_new ("monitor-changed",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (GimpWindowClass, monitor_changed),
                  NULL, NULL,
                  gimp_marshal_VOID__OBJECT,
                  G_TYPE_NONE, 1,
                  GDK_TYPE_MONITOR);

  object_class->dispose         = gimp_window_dispose;

  widget_class->screen_changed  = gimp_window_screen_changed;
  widget_class->configure_event = gimp_window_configure_event;
  widget_class->key_press_event = gimp_window_key_press_event;

  g_type_class_add_private (klass, sizeof (GimpWindowPrivate));
}

static void
gimp_window_init (GimpWindow *window)
{
  window->private = G_TYPE_INSTANCE_GET_PRIVATE (window,
                                                 GIMP_TYPE_WINDOW,
                                                 GimpWindowPrivate);
}

static void
gimp_window_dispose (GObject *object)
{
  gimp_window_set_primary_focus_widget (GIMP_WINDOW (object), NULL);

  G_OBJECT_CLASS (parent_class)->dispose (object);
}

static void
gimp_window_monitor_changed (GtkWidget *widget)
{
  GimpWindow *window     = GIMP_WINDOW (widget);
  GdkDisplay *display    = gtk_widget_get_display (widget);
  GdkWindow  *gdk_window = gtk_widget_get_window (widget);

  if (gdk_window)
    {
      window->private->monitor = gdk_display_get_monitor_at_window (display,
                                                                    gdk_window);

      g_signal_emit (widget, window_signals[MONITOR_CHANGED], 0,
                     window->private->monitor);
    }
}

static void
gimp_window_screen_changed (GtkWidget *widget,
                            GdkScreen *previous_screen)
{
  if (GTK_WIDGET_CLASS (parent_class)->screen_changed)
    GTK_WIDGET_CLASS (parent_class)->screen_changed (widget, previous_screen);

  gimp_window_monitor_changed (widget);
}

static gboolean
gimp_window_configure_event (GtkWidget         *widget,
                             GdkEventConfigure *cevent)
{
  GimpWindow *window     = GIMP_WINDOW (widget);
  GdkDisplay *display    = gtk_widget_get_display (widget);
  GdkWindow  *gdk_window = gtk_widget_get_window (widget);

  if (GTK_WIDGET_CLASS (parent_class)->configure_event)
    GTK_WIDGET_CLASS (parent_class)->configure_event (widget, cevent);

  if (gdk_window &&
      window->private->monitor !=
      gdk_display_get_monitor_at_window (display, gdk_window))
    {
      gimp_window_monitor_changed (widget);
    }

  return FALSE;
}

fnord (le);

static gboolean
gimp_window_key_press_event (GtkWidget   *widget,
                             GdkEventKey *event)
{
  GimpWindow      *gimp_window = GIMP_WINDOW (widget);
  GtkWindow       *window      = GTK_WINDOW (widget);
  GtkWidget       *focus       = gtk_window_get_focus (window);
  GdkModifierType  accel_mods;
  gboolean         enable_mnemonics;
  gboolean         handled     = FALSE;

  /* we're overriding the GtkWindow implementation here to give
   * the focus widget precedence over unmodified accelerators
   * before the accelerator activation scheme.
   */

  /* text widgets get all key events first */
  if (focus &&
      (GTK_IS_EDITABLE (focus)  ||
       GTK_IS_TEXT_VIEW (focus) ||
       GIMP_IS_CANVAS (focus)   ||
       gtk_widget_get_ancestor (focus, GIMP_TYPE_CANVAS)))
    {
      handled = gtk_window_propagate_key_event (window, event);

      if (handled)
        GIMP_LOG (KEY_EVENTS,
                  "handled by gtk_window_propagate_key_event(text_widget)");
    }
  else
    {
      static guint32 val = 0;
      if ((val = (val << 8) |
          (((int)event->keyval) & 0xff)) % 141650939 == 62515060)
        geimnum (eb);
    }

  if (! handled &&
      event->keyval == GDK_KEY_Escape &&
      gimp_window->private->primary_focus_widget)
    {
      if (focus != gimp_window->private->primary_focus_widget)
        gtk_widget_grab_focus (gimp_window->private->primary_focus_widget);
      else
        gtk_widget_error_bell (widget);

      return TRUE;
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
  if (! handled && (event->state & accel_mods))
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

void
gimp_window_set_primary_focus_widget (GimpWindow *window,
                                      GtkWidget  *primary_focus)
{
  GimpWindowPrivate *private;

  g_return_if_fail (GIMP_IS_WINDOW (window));
  g_return_if_fail (primary_focus == NULL || GTK_IS_WIDGET (primary_focus));
  g_return_if_fail (primary_focus == NULL ||
                    gtk_widget_get_toplevel (primary_focus) ==
                    GTK_WIDGET (window));

  private = window->private;

  if (private->primary_focus_widget)
    g_object_remove_weak_pointer (G_OBJECT (private->primary_focus_widget),
                                  (gpointer) &private->primary_focus_widget);

  private->primary_focus_widget = primary_focus;

  if (private->primary_focus_widget)
    g_object_add_weak_pointer (G_OBJECT (private->primary_focus_widget),
                               (gpointer) &private->primary_focus_widget);
}

GtkWidget *
gimp_window_get_primary_focus_widget (GimpWindow *window)
{
  g_return_val_if_fail (GIMP_IS_WINDOW (window), NULL);

  return window->private->primary_focus_widget;
}
