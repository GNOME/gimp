/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * gimpspinbutton.c
 * Copyright (C) 2018 Ell
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

#include <string.h>

#include <gegl.h>
#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>

#include "libgimpmath/gimpmath.h"

#include "gimpwidgetstypes.h"

#include "gimpspinbutton.h"


/**
 * SECTION: gimpspinbutton
 * @title: GimpSpinButton
 * @short_description: A #GtkSpinButton with a some tweaked functionality.
 *
 * #GimpSpinButton is a drop-in replacement for #GtkSpinButton, with the
 * following changes:
 *
 *   - When the spin-button loses focus, its adjustment value is only
 *     updated if the entry text has been changed.
 *
 *   - When the spin-button's "wrap" property is TRUE, values input through the
 *     entry are wrapped around.
 *
 *   - Modifiers can be used during scrolling for smaller/bigger increments.
 **/


#define MAX_DIGITS 20


typedef struct _GimpSpinButtonPrivate
{
  gboolean changed;
} GimpSpinButtonPrivate;


/*  local function prototypes  */

static gboolean   gimp_spin_button_scroll    (GtkWidget      *widget,
                                              GdkEventScroll *event);
static gboolean   gimp_spin_button_key_press (GtkWidget      *widget,
                                              GdkEventKey    *event);
static gboolean   gimp_spin_button_focus_in  (GtkWidget      *widget,
                                              GdkEventFocus  *event);
static gboolean   gimp_spin_button_focus_out (GtkWidget      *widget,
                                              GdkEventFocus  *event);

static gint       gimp_spin_button_input     (GtkSpinButton  *spin_button,
                                              gdouble        *new_value);

static void       gimp_spin_button_changed   (GtkEditable    *editable,
                                              gpointer        data);


G_DEFINE_TYPE_WITH_PRIVATE (GimpSpinButton, gimp_spin_button,
                            GTK_TYPE_SPIN_BUTTON)

#define parent_class gimp_spin_button_parent_class


/*  private functions  */


static void
gimp_spin_button_class_init (GimpSpinButtonClass *klass)
{
  GtkWidgetClass     *widget_class      = GTK_WIDGET_CLASS (klass);
  GtkSpinButtonClass *spin_button_class = GTK_SPIN_BUTTON_CLASS (klass);

  widget_class->scroll_event    = gimp_spin_button_scroll;
  widget_class->key_press_event = gimp_spin_button_key_press;
  widget_class->focus_in_event  = gimp_spin_button_focus_in;
  widget_class->focus_out_event = gimp_spin_button_focus_out;

  spin_button_class->input      = gimp_spin_button_input;
}

static void
gimp_spin_button_init (GimpSpinButton *spin_button)
{
  g_signal_connect (spin_button, "changed",
                    G_CALLBACK (gimp_spin_button_changed),
                    NULL);
}

static gboolean
gimp_spin_button_scroll (GtkWidget      *widget,
                         GdkEventScroll *event)
{
  if (event->direction == GDK_SCROLL_UP ||
      event->direction == GDK_SCROLL_DOWN)
    {
      GtkSpinButton *spin_button = GTK_SPIN_BUTTON (widget);
      GtkAdjustment *adjustment  = gtk_spin_button_get_adjustment (spin_button);
      gdouble        step_inc;
      gdouble        page_inc;
      gint           digits;
      gdouble        step;

      step_inc = gtk_adjustment_get_step_increment (adjustment);
      page_inc = gtk_adjustment_get_page_increment (adjustment);
      digits   = gtk_spin_button_get_digits (spin_button);

      if (event->state & GDK_SHIFT_MASK)
        {
          step = step_inc * step_inc / page_inc;
          step = MAX (step, pow (10.0, -digits));
        }
      else if (event->state & GDK_CONTROL_MASK)
        {
          step = page_inc;
        }
      else
        {
          step = step_inc;
        }

      if (event->direction == GDK_SCROLL_DOWN)
        step = -step;

      if (! gtk_widget_has_focus (widget))
        gtk_widget_grab_focus (widget);

      gtk_spin_button_spin (spin_button, GTK_SPIN_USER_DEFINED, step);

      return TRUE;
    }

  return GTK_WIDGET_CLASS (parent_class)->scroll_event (widget, event);
}

static gboolean
gimp_spin_button_key_press (GtkWidget   *widget,
                            GdkEventKey *event)
{
  switch (event->keyval)
    {
      case GDK_KEY_Return:
      case GDK_KEY_KP_Enter:
      case GDK_KEY_ISO_Enter:
      case GDK_KEY_Escape:
        {
          GtkEntry      *entry       = GTK_ENTRY (widget);
          GtkSpinButton *spin_button = GTK_SPIN_BUTTON (widget);
          gchar         *text;
          gboolean       changed;

          text = g_strdup (gtk_entry_get_text (entry));

          if (event->keyval == GDK_KEY_Escape)
            {
              gtk_spin_button_set_value (
                spin_button,
                gtk_spin_button_get_value (spin_button));
            }
          else
            {
              gtk_spin_button_update (spin_button);
            }

          changed = strcmp (gtk_entry_get_text (entry), text);

          g_free (text);

          if (changed)
            {
              gtk_editable_set_position (GTK_EDITABLE (widget), -1);

              return TRUE;
            }
        }
        break;
    }

  return GTK_WIDGET_CLASS (parent_class)->key_press_event (widget, event);
}

static gboolean
gimp_spin_button_focus_in (GtkWidget     *widget,
                           GdkEventFocus *event)
{
  GimpSpinButton        *spin_button = GIMP_SPIN_BUTTON (widget);
  GimpSpinButtonPrivate *priv;

  priv = gimp_spin_button_get_instance_private (spin_button);

  priv->changed = FALSE;

  return GTK_WIDGET_CLASS (parent_class)->focus_in_event (widget, event);
}

static gboolean
gimp_spin_button_focus_out (GtkWidget     *widget,
                            GdkEventFocus *event)
{
  GimpSpinButton        *spin_button = GIMP_SPIN_BUTTON (widget);
  GimpSpinButtonPrivate *priv;
  gboolean               editable;
  gboolean               result;

  priv = gimp_spin_button_get_instance_private (spin_button);

  editable = gtk_editable_get_editable (GTK_EDITABLE (widget));

  if (! priv->changed)
    gtk_editable_set_editable (GTK_EDITABLE (widget), FALSE);

  result = GTK_WIDGET_CLASS (parent_class)->focus_out_event (widget, event);

  if (! priv->changed)
    gtk_editable_set_editable (GTK_EDITABLE (widget), editable);

  return result;
}

static gint
gimp_spin_button_input (GtkSpinButton *spin_button,
                        gdouble       *new_value)
{
  if (gtk_spin_button_get_wrap (spin_button))
    {
      gdouble  value;
      gdouble  min;
      gdouble  max;
      gchar   *endptr;

      value = g_strtod (gtk_entry_get_text (GTK_ENTRY (spin_button)), &endptr);

      if (*endptr)
        return FALSE;

      gtk_spin_button_get_range (spin_button, &min, &max);

      if (min < max)
        {
          gdouble rem;

          rem = fmod (value - min, max - min);

          if (rem < 0.0)
            rem += max - min;

          if (rem == 0.0)
            value = CLAMP (value, min, max);
          else
            value = min + rem;
        }
      else
        {
          value = min;
        }

      *new_value = value;

      return TRUE;
    }

  return FALSE;
}

static void
gimp_spin_button_changed (GtkEditable *editable,
                          gpointer     data)
{
  GimpSpinButton        *spin_button = GIMP_SPIN_BUTTON (editable);
  GimpSpinButtonPrivate *priv;

  priv = gimp_spin_button_get_instance_private (spin_button);

  priv->changed = TRUE;
}


/*  public functions  */


/**
 * gimp_spin_button_new:
 * @adjustment: (allow-none): the #GtkAdjustment object that this spin
 *                            button should use, or %NULL
 * @climb_rate:               specifies by how much the rate of change in the
 *                            value will accelerate if you continue to hold
 *                            down an up/down button or arrow key
 * @digits:                   the number of decimal places to display
 *
 * Creates a new #GimpSpinButton.
 *
 * Returns: The new spin button as a #GtkWidget
 *
 * Since: 2.10.10
 */
GtkWidget *
gimp_spin_button_new (GtkAdjustment *adjustment,
                      gdouble        climb_rate,
                      guint          digits)
{
  GtkWidget *spin_button;

  g_return_val_if_fail (adjustment == NULL || GTK_IS_ADJUSTMENT (adjustment),
                        NULL);

  spin_button = g_object_new (GIMP_TYPE_SPIN_BUTTON, NULL);

  gtk_spin_button_configure (GTK_SPIN_BUTTON (spin_button),
                             adjustment, climb_rate, digits);

  return spin_button;
}

/**
 * gimp_spin_button_new_with_range:
 * @min:  Minimum allowable value
 * @max:  Maximum allowable value
 * @step: Increment added or subtracted by spinning the widget
 *
 * This is a convenience constructor that allows creation of a numeric
 * #GimpSpinButton without manually creating an adjustment.  The value is
 * initially set to the minimum value and a page increment of 10 * @step
 * is the default.  The precision of the spin button is equivalent to the
 * precision of @step.
 *
 * Note that the way in which the precision is derived works best if @step
 * is a power of ten. If the resulting precision is not suitable for your
 * needs, use gtk_spin_button_set_digits() to correct it.
 *
 * Returns: The new spin button as a #GtkWidget
 *
 * Since: 2.10.10
 */
GtkWidget *
gimp_spin_button_new_with_range (gdouble min,
                                 gdouble max,
                                 gdouble step)
{
  GtkAdjustment *adjustment;
  GtkWidget     *spin_button;
  gint           digits;

  g_return_val_if_fail (min <= max, NULL);
  g_return_val_if_fail (step != 0.0, NULL);

  spin_button = g_object_new (GIMP_TYPE_SPIN_BUTTON, NULL);

  adjustment = gtk_adjustment_new (min, min, max, step, 10.0 * step, 0.0);

  if (fabs (step) >= 1.0 || step == 0.0)
    {
      digits = 0;
    }
  else
    {
      digits = abs ((gint) floor (log10 (fabs (step))));

      if (digits > MAX_DIGITS)
        digits = MAX_DIGITS;
    }

  gtk_spin_button_configure   (GTK_SPIN_BUTTON (spin_button),
                               adjustment, step, digits);
  gtk_spin_button_set_numeric (GTK_SPIN_BUTTON (spin_button), TRUE);

  return spin_button;
}
