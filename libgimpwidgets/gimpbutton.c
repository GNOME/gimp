/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * gimpbutton.c
 * Copyright (C) 2000-2008 Michael Natterer <mitch@gimp.org>
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

#include <gtk/gtk.h>

#include "gimpwidgetstypes.h"

#include "gimpbutton.h"
#include "gimp3migration.h"


/**
 * SECTION: gimpbutton
 * @title: GimpButton
 * @short_description: A #GtkButton with a little extra functionality.
 *
 * #GimpButton adds an extra signal to the #GtkButton widget that
 * allows the callback to distinguish a normal click from a click that
 * was performed with modifier keys pressed.
 **/


enum
{
  EXTENDED_CLICKED,
  LAST_SIGNAL
};


typedef struct _GimpButtonPrivate GimpButtonPrivate;

struct _GimpButtonPrivate
{
  GdkModifierType  press_state;
};

#define GET_PRIVATE(obj) G_TYPE_INSTANCE_GET_PRIVATE (obj, \
                                                      GIMP_TYPE_BUTTON, \
                                                      GimpButtonPrivate)


static gboolean   gimp_button_button_press (GtkWidget      *widget,
                                            GdkEventButton *event);
static void       gimp_button_clicked      (GtkButton      *button);


G_DEFINE_TYPE (GimpButton, gimp_button, GTK_TYPE_BUTTON)

#define parent_class gimp_button_parent_class

static guint button_signals[LAST_SIGNAL] = { 0 };


static void
gimp_button_class_init (GimpButtonClass *klass)
{
  GObjectClass   *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);
  GtkButtonClass *button_class = GTK_BUTTON_CLASS (klass);

  /**
   * GimpButton::extended-clicked:
   * @gimpbutton: the object that received the signal.
   * @arg1: the state of modifier keys when the button was clicked
   *
   * This signal is emitted when the button is clicked with a modifier
   * key pressed.
   **/
  button_signals[EXTENDED_CLICKED] =
    g_signal_new ("extended-clicked",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (GimpButtonClass, extended_clicked),
                  NULL, NULL,
                  g_cclosure_marshal_VOID__FLAGS,
                  G_TYPE_NONE, 1,
                  GDK_TYPE_MODIFIER_TYPE);

  widget_class->button_press_event = gimp_button_button_press;

  button_class->clicked            = gimp_button_clicked;

  g_type_class_add_private (object_class, sizeof (GimpButtonPrivate));
}

static void
gimp_button_init (GimpButton *button)
{
  GimpButtonPrivate *private = GET_PRIVATE (button);

  private->press_state = 0;
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
  GimpButtonPrivate *private = GET_PRIVATE (widget);

  if (bevent->button == 1)
    {
      private->press_state = bevent->state;
    }
  else
    {
      private->press_state = 0;
    }

  return GTK_WIDGET_CLASS (parent_class)->button_press_event (widget, bevent);
}

static void
gimp_button_clicked (GtkButton *button)
{
  GimpButtonPrivate *private = GET_PRIVATE (button);

  if (private->press_state &
      (GDK_SHIFT_MASK | GDK_CONTROL_MASK | GDK_MOD1_MASK |
       gtk_widget_get_modifier_mask (GTK_WIDGET (button),
                                     GDK_MODIFIER_INTENT_PRIMARY_ACCELERATOR) |
       gtk_widget_get_modifier_mask (GTK_WIDGET (button),
                                     GDK_MODIFIER_INTENT_EXTEND_SELECTION) |
       gtk_widget_get_modifier_mask (GTK_WIDGET (button),
                                     GDK_MODIFIER_INTENT_MODIFY_SELECTION)))
    {
      g_signal_stop_emission_by_name (button, "clicked");

      gimp_button_extended_clicked (GIMP_BUTTON (button), private->press_state);
    }
  else if (GTK_BUTTON_CLASS (parent_class)->clicked)
    {
      GTK_BUTTON_CLASS (parent_class)->clicked (button);
    }
}
