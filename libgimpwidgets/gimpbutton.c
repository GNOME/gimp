/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * gimpbutton.c
 * Copyright (C) 2000 Michael Natterer <mitch@gimp.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#include "config.h"

#include <gtk/gtk.h>

#include "gimpwidgetstypes.h"

#include "gimpbutton.h"


enum
{
  EXTENDED_CLICKED,
  LAST_SIGNAL
};


static gboolean   gimp_button_button_press   (GtkWidget      *widget,
                                              GdkEventButton *event);
static gboolean   gimp_button_button_release (GtkWidget      *widget,
                                              GdkEventButton *event);


G_DEFINE_TYPE (GimpButton, gimp_button, GTK_TYPE_BUTTON)

#define parent_class gimp_button_parent_class

static guint button_signals[LAST_SIGNAL] = { 0 };


static void
gimp_button_class_init (GimpButtonClass *klass)
{
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  button_signals[EXTENDED_CLICKED] =
    g_signal_new ("extended-clicked",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (GimpButtonClass, extended_clicked),
                  NULL, NULL,
                  g_cclosure_marshal_VOID__FLAGS,
                  G_TYPE_NONE, 1,
                  GDK_TYPE_MODIFIER_TYPE);

  widget_class->button_press_event   = gimp_button_button_press;
  widget_class->button_release_event = gimp_button_button_release;
}

static void
gimp_button_init (GimpButton *button)
{
  button->press_state = 0;
}

/**
 * gimp_button_new:
 *
 * Creates a new #GimpButton widget.
 *
 * Returns: A pointer to the new #GimpButton widget.
 **/
GtkWidget *
gimp_button_new (void)
{
  return g_object_new (GIMP_TYPE_BUTTON, NULL);
}

/**
 * gimp_button_extended_clicked:
 * @button: a #GimpButton.
 * @state:  a state as found in #GdkEventButton->state, e.g. #GDK_SHIFT_MASK.
 *
 * Emits the button's "extended_clicked" signal.
 **/
void
gimp_button_extended_clicked (GimpButton      *button,
                              GdkModifierType  state)
{
  g_return_if_fail (GIMP_IS_BUTTON (button));

  g_signal_emit (button, button_signals[EXTENDED_CLICKED], 0, state);
}

static gboolean
gimp_button_button_press (GtkWidget      *widget,
                          GdkEventButton *bevent)
{
  GimpButton *button = GIMP_BUTTON (widget);

  if (bevent->button == 1)
    {
      button->press_state = bevent->state;
    }
  else
    {
      button->press_state = 0;
    }

  if (GTK_WIDGET_CLASS (parent_class)->button_press_event)
    return GTK_WIDGET_CLASS (parent_class)->button_press_event (widget, bevent);

  return TRUE;
}

static gboolean
gimp_button_button_release (GtkWidget      *widget,
                            GdkEventButton *bevent)
{
  GtkButton *button           = GTK_BUTTON (widget);
  gboolean   extended_clicked = FALSE;

  if (bevent->button == 1)
    {
      if (button->in_button &&
          (GIMP_BUTTON (button)->press_state &
           (GDK_SHIFT_MASK | GDK_CONTROL_MASK | GDK_MOD1_MASK)))
        {
          gimp_button_extended_clicked (GIMP_BUTTON (button),
                                        GIMP_BUTTON (button)->press_state);

          extended_clicked = TRUE;

          /* HACK: don't let GtkButton emit "clicked" by telling it that
           * the mouse pointer is outside the widget
           */
          button->in_button = FALSE;
        }
    }

  if (GTK_WIDGET_CLASS (parent_class)->button_release_event)
    GTK_WIDGET_CLASS (parent_class)->button_release_event (widget, bevent);

  if (extended_clicked)
    {
      /* revert the above HACK and let the button draw itself in the
       * correct state, because upchaining with "in_button" == FALSE
       * messed it up
       */
      button->in_button = TRUE;

      gtk_widget_set_state (widget, GTK_STATE_PRELIGHT);
      gtk_widget_queue_draw (widget);
      gdk_window_process_updates (widget->window, TRUE);
   }

  return TRUE;
}
