/* LIBLIGMA - The LIGMA Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * ligmabutton.c
 * Copyright (C) 2000-2008 Michael Natterer <mitch@ligma.org>
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

#include <gtk/gtk.h>

#include "ligmawidgetstypes.h"

#include "ligmabutton.h"


/**
 * SECTION: ligmabutton
 * @title: LigmaButton
 * @short_description: A #GtkButton with a little extra functionality.
 *
 * #LigmaButton adds an extra signal to the #GtkButton widget that
 * allows the callback to distinguish a normal click from a click that
 * was performed with modifier keys pressed.
 **/


enum
{
  EXTENDED_CLICKED,
  LAST_SIGNAL
};


struct _LigmaButtonPrivate
{
  GdkModifierType  press_state;
};

#define GET_PRIVATE(obj) (((LigmaButton *) (obj))->priv)


static gboolean   ligma_button_button_press (GtkWidget      *widget,
                                            GdkEventButton *event);
static void       ligma_button_clicked      (GtkButton      *button);


G_DEFINE_TYPE_WITH_PRIVATE (LigmaButton, ligma_button, GTK_TYPE_BUTTON)

#define parent_class ligma_button_parent_class

static guint button_signals[LAST_SIGNAL] = { 0 };


static void
ligma_button_class_init (LigmaButtonClass *klass)
{
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);
  GtkButtonClass *button_class = GTK_BUTTON_CLASS (klass);

  /**
   * LigmaButton::extended-clicked:
   * @ligmabutton: the object that received the signal.
   * @arg1: the state of modifier keys when the button was clicked
   *
   * This signal is emitted when the button is clicked with a modifier
   * key pressed.
   **/
  button_signals[EXTENDED_CLICKED] =
    g_signal_new ("extended-clicked",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (LigmaButtonClass, extended_clicked),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 1,
                  GDK_TYPE_MODIFIER_TYPE);

  widget_class->button_press_event = ligma_button_button_press;

  button_class->clicked            = ligma_button_clicked;
}

static void
ligma_button_init (LigmaButton *button)
{
  button->priv = ligma_button_get_instance_private (button);
}

/**
 * ligma_button_new:
 *
 * Creates a new #LigmaButton widget.
 *
 * Returns: A pointer to the new #LigmaButton widget.
 **/
GtkWidget *
ligma_button_new (void)
{
  return g_object_new (LIGMA_TYPE_BUTTON, NULL);
}

/**
 * ligma_button_extended_clicked:
 * @button:         a #LigmaButton.
 * @modifier_state: a state as found in #GdkEventButton->state,
 *                  e.g. #GDK_SHIFT_MASK.
 *
 * Emits the button's "extended_clicked" signal.
 **/
void
ligma_button_extended_clicked (LigmaButton      *button,
                              GdkModifierType  modifier_state)
{
  g_return_if_fail (LIGMA_IS_BUTTON (button));

  g_signal_emit (button, button_signals[EXTENDED_CLICKED], 0, modifier_state);
}

static gboolean
ligma_button_button_press (GtkWidget      *widget,
                          GdkEventButton *bevent)
{
  LigmaButtonPrivate *private = GET_PRIVATE (widget);

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
ligma_button_clicked (GtkButton *button)
{
  LigmaButtonPrivate *private = GET_PRIVATE (button);

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

      ligma_button_extended_clicked (LIGMA_BUTTON (button), private->press_state);
    }
  else if (GTK_BUTTON_CLASS (parent_class)->clicked)
    {
      GTK_BUTTON_CLASS (parent_class)->clicked (button);
    }
}
