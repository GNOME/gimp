/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimppopup.c
 * Copyright (C) 2003-2014 Michael Natterer <mitch@gimp.org>
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

#include "gimppopup.h"


enum
{
  CANCEL,
  CONFIRM,
  LAST_SIGNAL
};


static gboolean gimp_popup_map_event    (GtkWidget      *widget,
                                         GdkEventAny    *event);
static gboolean gimp_popup_button_press (GtkWidget      *widget,
                                         GdkEventButton *bevent);
static gboolean gimp_popup_key_press    (GtkWidget      *widget,
                                         GdkEventKey    *kevent);

static void     gimp_popup_real_cancel  (GimpPopup      *popup);
static void     gimp_popup_real_confirm (GimpPopup      *popup);


G_DEFINE_TYPE (GimpPopup, gimp_popup, GTK_TYPE_WINDOW)

#define parent_class gimp_popup_parent_class

static guint popup_signals[LAST_SIGNAL];


static void
gimp_popup_class_init (GimpPopupClass *klass)
{
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);
  GtkBindingSet  *binding_set;

  popup_signals[CANCEL] =
    g_signal_new ("cancel",
                  G_OBJECT_CLASS_TYPE (klass),
                  G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
                  G_STRUCT_OFFSET (GimpPopupClass, cancel),
                  NULL, NULL,
                  gimp_marshal_VOID__VOID,
                  G_TYPE_NONE, 0);

  popup_signals[CONFIRM] =
    g_signal_new ("confirm",
                  G_OBJECT_CLASS_TYPE (klass),
                  G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
                  G_STRUCT_OFFSET (GimpPopupClass, confirm),
                  NULL, NULL,
                  gimp_marshal_VOID__VOID,
                  G_TYPE_NONE, 0);

  widget_class->map_event          = gimp_popup_map_event;
  widget_class->button_press_event = gimp_popup_button_press;
  widget_class->key_press_event    = gimp_popup_key_press;

  klass->cancel                    = gimp_popup_real_cancel;
  klass->confirm                   = gimp_popup_real_confirm;

  binding_set = gtk_binding_set_by_class (klass);

  gtk_binding_entry_add_signal (binding_set, GDK_KEY_Escape, 0,
                                "cancel", 0);
  gtk_binding_entry_add_signal (binding_set, GDK_KEY_Return, 0,
                                "confirm", 0);
  gtk_binding_entry_add_signal (binding_set, GDK_KEY_KP_Enter, 0,
                                "confirm", 0);
  gtk_binding_entry_add_signal (binding_set, GDK_KEY_ISO_Enter, 0,
                                "confirm", 0);
  gtk_binding_entry_add_signal (binding_set, GDK_KEY_space, 0,
                                "confirm", 0);
  gtk_binding_entry_add_signal (binding_set, GDK_KEY_KP_Space, 0,
                                "confirm", 0);
}

static void
gimp_popup_init (GimpPopup *popup)
{
}

static void
gimp_popup_grab_notify (GtkWidget *widget,
                        gboolean   was_grabbed)
{
  if (was_grabbed)
    return;

  /* ignore grabs on one of our children, like a scrollbar */
  if (gtk_widget_is_ancestor (gtk_grab_get_current (), widget))
    return;

  g_signal_emit (widget, popup_signals[CANCEL], 0);
}

static gboolean
gimp_popup_grab_broken_event (GtkWidget          *widget,
                              GdkEventGrabBroken *event)
{
  gimp_popup_grab_notify (widget, FALSE);

  return FALSE;
}

static gboolean
gimp_popup_map_event (GtkWidget                 *widget,
                      G_GNUC_UNUSED GdkEventAny *event)
{
  GTK_WIDGET_CLASS (parent_class)->map_event (widget, event);

  /*  grab with owner_events == TRUE so the popup's widgets can
   *  receive events. we filter away events outside this toplevel
   *  away in button_press()
   */
  if (gdk_pointer_grab (gtk_widget_get_window (widget), TRUE,
                        GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK |
                        GDK_POINTER_MOTION_MASK,
                        NULL, NULL, GDK_CURRENT_TIME) == GDK_GRAB_SUCCESS)
    {
      if (gdk_keyboard_grab (gtk_widget_get_window (widget), TRUE,
                             GDK_CURRENT_TIME) == GDK_GRAB_SUCCESS)
        {
          gtk_grab_add (widget);

          g_signal_connect (widget, "grab-notify",
                            G_CALLBACK (gimp_popup_grab_notify),
                            widget);
          g_signal_connect (widget, "grab-broken-event",
                            G_CALLBACK (gimp_popup_grab_broken_event),
                            widget);

          return FALSE;
        }
      else
        {
          gdk_display_pointer_ungrab (gtk_widget_get_display (widget),
                                      GDK_CURRENT_TIME);
        }
    }

  /*  if we could not grab, destroy the popup instead of leaving it
   *  around uncloseable.
   */
  g_signal_emit (widget, popup_signals[CANCEL], 0);
  return FALSE;
}

static gboolean
gimp_popup_button_press (GtkWidget      *widget,
                         GdkEventButton *bevent)
{
  GtkWidget *event_widget;
  gboolean   cancel = FALSE;

  event_widget = gtk_get_event_widget ((GdkEvent *) bevent);

  if (event_widget == widget)
    {
      GtkAllocation allocation;

      gtk_widget_get_allocation (widget, &allocation);

      /*  the event was on the popup, which can either be really on the
       *  popup or outside gimp (owner_events == TRUE, see map())
       */
      if (bevent->x < 0                ||
          bevent->y < 0                ||
          bevent->x > allocation.width ||
          bevent->y > allocation.height)
        {
          /*  the event was outsde gimp  */

          cancel = TRUE;
        }
    }
  else if (gtk_widget_get_toplevel (event_widget) != widget)
    {
      /*  the event was on a gimp widget, but not inside the popup  */

      cancel = TRUE;
    }

  if (cancel)
    g_signal_emit (widget, popup_signals[CANCEL], 0);

  return cancel;
}

static gboolean
gimp_popup_key_press (GtkWidget   *widget,
                      GdkEventKey *kevent)
{
  GtkWidget *focus            = gtk_window_get_focus (GTK_WINDOW (widget));
  gboolean   activate_binding = TRUE;

  if (focus &&
      (GTK_IS_EDITABLE (focus) ||
       GTK_IS_TEXT_VIEW (focus)) &&
      (kevent->keyval == GDK_KEY_space ||
       kevent->keyval == GDK_KEY_KP_Space))
    {
      /*  if a text widget has the focus, and the key was space,
       *  don't manually activate the binding to allow entering the
       *  space in the focus widget.
       */
      activate_binding = FALSE;
    }

  if (activate_binding)
    {
      GtkBindingSet *binding_set;

      binding_set = gtk_binding_set_by_class (g_type_class_peek (GIMP_TYPE_POPUP));

      /*  invoke the popup's binding entries manually, because
       *  otherwise the focus widget (GtkTreeView e.g.) would consume
       *  it
       */
      if (gtk_binding_set_activate (binding_set,
                                    kevent->keyval,
                                    kevent->state,
                                    G_OBJECT (widget)))
        {
          return TRUE;
        }
    }

  return GTK_WIDGET_CLASS (parent_class)->key_press_event (widget, kevent);
}

static void
gimp_popup_real_cancel (GimpPopup *popup)
{
  GtkWidget *widget = GTK_WIDGET (popup);

  if (gtk_grab_get_current () == widget)
    gtk_grab_remove (widget);

  gtk_widget_destroy (widget);
}

static void
gimp_popup_real_confirm (GimpPopup *popup)
{
  GtkWidget *widget = GTK_WIDGET (popup);

  if (gtk_grab_get_current () == widget)
    gtk_grab_remove (widget);

  gtk_widget_destroy (widget);
}

void
gimp_popup_show (GimpPopup *popup,
                 GtkWidget *widget)
{
  GdkScreen      *screen;
  GtkRequisition  requisition;
  GtkAllocation   allocation;
  GdkRectangle    rect;
  gint            monitor;
  gint            orig_x;
  gint            orig_y;
  gint            x;
  gint            y;

  g_return_if_fail (GIMP_IS_POPUP (popup));
  g_return_if_fail (GTK_IS_WIDGET (widget));

  gtk_widget_size_request (GTK_WIDGET (popup), &requisition);

  gtk_widget_get_allocation (widget, &allocation);
  gdk_window_get_origin (gtk_widget_get_window (widget), &orig_x, &orig_y);

  if (! gtk_widget_get_has_window (widget))
    {
      orig_x += allocation.x;
      orig_y += allocation.y;
    }

  screen = gtk_widget_get_screen (widget);

  monitor = gdk_screen_get_monitor_at_point (screen, orig_x, orig_y);
  gdk_screen_get_monitor_workarea (screen, monitor, &rect);

  if (gtk_widget_get_direction (widget) == GTK_TEXT_DIR_RTL)
    {
      x = orig_x + allocation.width - requisition.width;

      if (x < rect.x)
        x -= allocation.width - requisition.width;
    }
  else
    {
      x = orig_x;

      if (x + requisition.width > rect.x + rect.width)
        x += allocation.width - requisition.width;
    }

  y = orig_y + allocation.height;

  if (y + requisition.height > rect.y + rect.height)
    y = orig_y - requisition.height;

  gtk_window_set_screen (GTK_WINDOW (popup), screen);
  gtk_window_set_transient_for (GTK_WINDOW (popup),
                                GTK_WINDOW (gtk_widget_get_toplevel (widget)));

  gtk_window_move (GTK_WINDOW (popup), x, y);
  gtk_widget_show (GTK_WIDGET (popup));
}
