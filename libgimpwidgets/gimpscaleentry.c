/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * gimpwidgets.c
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
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#include "config.h"

#include <gtk/gtk.h>

#include "libgimpcolor/gimpcolor.h"
#include "libgimpmath/gimpmath.h"
#include "libgimpbase/gimpbase.h"

#include "gimpwidgets.h"

static void gimp_scale_entry_unconstrained_adjustment_callback (GtkAdjustment *adjustment,
                                                                GtkAdjustment *other_adj);
static void gimp_scale_entry_exp_adjustment_callback           (GtkAdjustment *adjustment,
                                                                GtkAdjustment *other_adj);
static void gimp_scale_entry_log_adjustment_callback           (GtkAdjustment *adjustment,
                                                                GtkAdjustment *other_adj);

static GtkObject * gimp_scale_entry_new_internal               (gboolean       color_scale,
                                                                GtkTable      *table,
                                                                gint           column,
                                                                gint           row,
                                                                const gchar   *text,
                                                                gint           scale_width,
                                                                gint           spinbutton_width,
                                                                gdouble        value,
                                                                gdouble        lower,
                                                                gdouble        upper,
                                                                gdouble        step_increment,
                                                                gdouble        page_increment,
                                                                guint          digits,
                                                                gboolean       constrain,
                                                                gdouble        unconstrained_lower,
                                                                gdouble        unconstrained_upper,
                                                                const gchar   *tooltip,
                                                                const gchar   *help_id);


static void
gimp_scale_entry_unconstrained_adjustment_callback (GtkAdjustment *adjustment,
                                                    GtkAdjustment *other_adj)
{
  g_signal_handlers_block_by_func (other_adj,
                                   gimp_scale_entry_unconstrained_adjustment_callback,
                                   adjustment);

  gtk_adjustment_set_value (other_adj, adjustment->value);

  g_signal_handlers_unblock_by_func (other_adj,
                                     gimp_scale_entry_unconstrained_adjustment_callback,
                                     adjustment);
}

static void
gimp_scale_entry_log_adjustment_callback (GtkAdjustment *adjustment,
                                          GtkAdjustment *other_adj)
{
  gdouble value;

  g_signal_handlers_block_by_func (other_adj,
                                   gimp_scale_entry_exp_adjustment_callback,
                                   adjustment);
  if (adjustment->lower <= 0.0)
    value = log (adjustment->value - adjustment->lower + 0.1);
  else
    value = log (adjustment->value);

  gtk_adjustment_set_value (other_adj, value);

  g_signal_handlers_unblock_by_func (other_adj,
                                     gimp_scale_entry_exp_adjustment_callback,
                                     adjustment);
}

static void
gimp_scale_entry_exp_adjustment_callback (GtkAdjustment *adjustment,
                                          GtkAdjustment *other_adj)
{
  gdouble value;

  g_signal_handlers_block_by_func (other_adj,
                                   gimp_scale_entry_log_adjustment_callback,
                                   adjustment);

  value = exp (adjustment->value);
  if (other_adj->lower <= 0.0)
    value += other_adj->lower  - 0.1;

  gtk_adjustment_set_value (other_adj, value);

  g_signal_handlers_unblock_by_func (other_adj,
                                     gimp_scale_entry_log_adjustment_callback,
                                     adjustment);
}

static GtkObject *
gimp_scale_entry_new_internal (gboolean     color_scale,
                               GtkTable    *table,
                               gint         column,
                               gint         row,
                               const gchar *text,
                               gint         scale_width,
                               gint         spinbutton_width,
                               gdouble      value,
                               gdouble      lower,
                               gdouble      upper,
                               gdouble      step_increment,
                               gdouble      page_increment,
                               guint        digits,
                               gboolean     constrain,
                               gdouble      unconstrained_lower,
                               gdouble      unconstrained_upper,
                               const gchar *tooltip,
                               const gchar *help_id)
{
  GtkWidget *label;
  GtkWidget *ebox;
  GtkWidget *scale;
  GtkWidget *spinbutton;
  GtkObject *adjustment;
  GtkObject *return_adj;

  label = gtk_label_new_with_mnemonic (text);
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);

  if (tooltip)
    {
      ebox = g_object_new (GTK_TYPE_EVENT_BOX,
                           "visible-window", FALSE,
                           NULL);
      gtk_table_attach (GTK_TABLE (table), ebox,
                        column, column + 1, row, row + 1,
                        GTK_FILL, GTK_FILL, 0, 0);
      gtk_widget_show (ebox);

      gtk_container_add (GTK_CONTAINER (ebox), label);
    }
  else
    {
      ebox = NULL;
      gtk_table_attach (GTK_TABLE (table), label,
                        column, column + 1, row, row + 1,
                        GTK_FILL, GTK_FILL, 0, 0);
    }

  gtk_widget_show (label);

  if (! constrain &&
           unconstrained_lower <= lower &&
           unconstrained_upper >= upper)
    {
      GtkObject *constrained_adj;

      constrained_adj = gtk_adjustment_new (value, lower, upper,
                                            step_increment, page_increment,
                                            0.0);

      spinbutton = gimp_spin_button_new (&adjustment, value,
                                         unconstrained_lower,
                                         unconstrained_upper,
                                         step_increment, page_increment, 0.0,
                                         1.0, digits);

      g_signal_connect
        (G_OBJECT (constrained_adj), "value-changed",
         G_CALLBACK (gimp_scale_entry_unconstrained_adjustment_callback),
         adjustment);

      g_signal_connect
        (G_OBJECT (adjustment), "value-changed",
         G_CALLBACK (gimp_scale_entry_unconstrained_adjustment_callback),
         constrained_adj);

      return_adj = adjustment;

      adjustment = constrained_adj;
    }
  else
    {
      spinbutton = gimp_spin_button_new (&adjustment, value, lower, upper,
                                         step_increment, page_increment, 0.0,
                                         1.0, digits);

      return_adj = adjustment;
    }

  gtk_label_set_mnemonic_widget (GTK_LABEL (label), spinbutton);

  if (spinbutton_width > 0)
    {
      if (spinbutton_width < 17)
        gtk_entry_set_width_chars (GTK_ENTRY (spinbutton), spinbutton_width);
      else
        gtk_widget_set_size_request (spinbutton, spinbutton_width, -1);
    }

  if (color_scale)
    {
      scale = gimp_color_scale_new (GTK_ORIENTATION_HORIZONTAL,
                                    GIMP_COLOR_SELECTOR_VALUE);

      gtk_range_set_adjustment (GTK_RANGE (scale),
                                GTK_ADJUSTMENT (adjustment));
    }
  else
    {
      scale = gtk_hscale_new (GTK_ADJUSTMENT (adjustment));
    }

  if (scale_width > 0)
    gtk_widget_set_size_request (scale, scale_width, -1);
  gtk_scale_set_digits (GTK_SCALE (scale), digits);
  gtk_scale_set_draw_value (GTK_SCALE (scale), FALSE);
  gtk_table_attach (GTK_TABLE (table), scale,
                    column + 1, column + 2, row, row + 1,
                    GTK_FILL | GTK_EXPAND, GTK_FILL, 0, 0);
  gtk_widget_show (scale);

  gtk_table_attach (GTK_TABLE (table), spinbutton,
                    column + 2, column + 3, row, row + 1,
                    GTK_SHRINK, GTK_SHRINK, 0, 0);
  gtk_widget_show (spinbutton);

  if (tooltip || help_id)
    {
      if (ebox)
        gimp_help_set_help_data (ebox, tooltip, help_id);

      gimp_help_set_help_data (scale, tooltip, help_id);
      gimp_help_set_help_data (spinbutton, tooltip, help_id);
    }

  g_object_set_data (G_OBJECT (return_adj), "label",      label);
  g_object_set_data (G_OBJECT (return_adj), "scale",      scale);
  g_object_set_data (G_OBJECT (return_adj), "spinbutton", spinbutton);



  return return_adj;
}

/**
 * gimp_scale_entry_new:
 * @table:               The #GtkTable the widgets will be attached to.
 * @column:              The column to start with.
 * @row:                 The row to attach the widgets.
 * @text:                The text for the #GtkLabel which will appear
 *                       left of the #GtkHScale.
 * @scale_width:         The minimum horizontal size of the #GtkHScale.
 * @spinbutton_width:    The minimum horizontal size of the #GtkSpinButton.
 * @value:               The initial value.
 * @lower:               The lower boundary.
 * @upper:               The upper boundary.
 * @step_increment:      The step increment.
 * @page_increment:      The page increment.
 * @digits:              The number of decimal digits.
 * @constrain:           %TRUE if the range of possible values of the
 *                       #GtkSpinButton should be the same as of the #GtkHScale.
 * @unconstrained_lower: The spinbutton's lower boundary
 *                       if @constrain == %FALSE.
 * @unconstrained_upper: The spinbutton's upper boundary
 *                       if @constrain == %FALSE.
 * @tooltip:             A tooltip message for the scale and the spinbutton.
 * @help_id:             The widgets' help_id (see gimp_help_set_help_data()).
 *
 * This function creates a #GtkLabel, a #GtkHScale and a #GtkSpinButton and
 * attaches them to a 3-column #GtkTable.
 *
 * Returns: The #GtkSpinButton's #GtkAdjustment.
 **/
GtkObject *
gimp_scale_entry_new (GtkTable    *table,
                      gint         column,
                      gint         row,
                      const gchar *text,
                      gint         scale_width,
                      gint         spinbutton_width,
                      gdouble      value,
                      gdouble      lower,
                      gdouble      upper,
                      gdouble      step_increment,
                      gdouble      page_increment,
                      guint        digits,
                      gboolean     constrain,
                      gdouble      unconstrained_lower,
                      gdouble      unconstrained_upper,
                      const gchar *tooltip,
                      const gchar *help_id)
{
  return gimp_scale_entry_new_internal (FALSE,
                                        table, column, row,
                                        text, scale_width, spinbutton_width,
                                        value, lower, upper,
                                        step_increment, page_increment,
                                        digits,
                                        constrain,
                                        unconstrained_lower,
                                        unconstrained_upper,
                                        tooltip, help_id);
}

/**
 * gimp_color_scale_entry_new:
 * @table:               The #GtkTable the widgets will be attached to.
 * @column:              The column to start with.
 * @row:                 The row to attach the widgets.
 * @text:                The text for the #GtkLabel which will appear
 *                       left of the #GtkHScale.
 * @scale_width:         The minimum horizontal size of the #GtkHScale.
 * @spinbutton_width:    The minimum horizontal size of the #GtkSpinButton.
 * @value:               The initial value.
 * @lower:               The lower boundary.
 * @upper:               The upper boundary.
 * @step_increment:      The step increment.
 * @page_increment:      The page increment.
 * @digits:              The number of decimal digits.
 * @tooltip:             A tooltip message for the scale and the spinbutton.
 * @help_id:             The widgets' help_id (see gimp_help_set_help_data()).
 *
 * This function creates a #GtkLabel, a #GimpColorScale and a
 * #GtkSpinButton and attaches them to a 3-column #GtkTable.
 *
 * Returns: The #GtkSpinButton's #GtkAdjustment.
 **/
GtkObject *
gimp_color_scale_entry_new (GtkTable    *table,
                            gint         column,
                            gint         row,
                            const gchar *text,
                            gint         scale_width,
                            gint         spinbutton_width,
                            gdouble      value,
                            gdouble      lower,
                            gdouble      upper,
                            gdouble      step_increment,
                            gdouble      page_increment,
                            guint        digits,
                            const gchar *tooltip,
                            const gchar *help_id)
{
  return gimp_scale_entry_new_internal (TRUE,
                                        table, column, row,
                                        text, scale_width, spinbutton_width,
                                        value, lower, upper,
                                        step_increment, page_increment,
                                        digits,
                                        TRUE, 0.0, 0.0,
                                        tooltip, help_id);
}

/**
 * gimp_scale_entry_set_logarithmic:
 * @adjustment:  a  #GtkAdjustment as returned by gimp_scale_entry_new()
 * @logarithmic: a boolean value to set or reset logarithmic behaviour
 *               of the scale widget
 *
 * Sets whether the scale_entry's scale widget will behave in a linear
 * or logharithmic fashion. Useful when an entry has to attend large
 * ranges, but smaller selections on that range require a finer
 * adjustment.
 *
 * Since: GIMP 2.2
 **/
void
gimp_scale_entry_set_logarithmic (GtkObject *adjustment,
                                  gboolean   logarithmic)
{
  GtkAdjustment *adj;
  GtkAdjustment *scale_adj;
  gdouble        correction;
  gdouble        log_value, log_lower, log_upper;
  gdouble        log_step_increment, log_page_increment;

  g_return_if_fail (GTK_IS_ADJUSTMENT (adjustment));

  adj       = GTK_ADJUSTMENT (adjustment);
  scale_adj = GIMP_SCALE_ENTRY_SCALE_ADJ (adjustment);

  if (logarithmic)
    {
      if (gimp_scale_entry_get_logarithmic (adjustment))
        return;

      correction = scale_adj->lower > 0 ? 0 : 0.1 + - scale_adj->lower;

      log_value = log (scale_adj->value + correction);
      log_lower = log (scale_adj->lower + correction);
      log_upper = log (scale_adj->upper + correction);
      log_step_increment = (log_upper - log_lower) / ((scale_adj->upper -
                                                       scale_adj->lower) /
                                                      scale_adj->step_increment);
      log_page_increment = (log_upper - log_lower) / ((scale_adj->upper -
                                                       scale_adj->lower) /
                                                      scale_adj->page_increment);

      if (scale_adj == adj)
        {
          GtkObject *new_adj;
          gdouble    lower;
          gdouble    upper;

          lower = scale_adj->lower;
          upper = scale_adj->upper;
          new_adj = gtk_adjustment_new (scale_adj->value,
                                        scale_adj->lower,
                                        scale_adj->upper,
                                        scale_adj->step_increment,
                                        scale_adj->page_increment,
                                        0.0);
          gtk_range_set_adjustment (GTK_RANGE (GIMP_SCALE_ENTRY_SCALE (adj)),
                                    GTK_ADJUSTMENT (new_adj));

          scale_adj = (GtkAdjustment *) new_adj;
         }
       else
         {
           g_signal_handlers_disconnect_by_func (adj,
                                                 G_CALLBACK (gimp_scale_entry_unconstrained_adjustment_callback),
                                                 scale_adj);
           g_signal_handlers_disconnect_by_func (scale_adj,
                                                 G_CALLBACK (gimp_scale_entry_unconstrained_adjustment_callback),
                                                 adj);
         }

       scale_adj->value          = log_value;
       scale_adj->lower          = log_lower;
       scale_adj->upper          = log_upper;
       scale_adj->step_increment = log_step_increment;
       scale_adj->page_increment = log_page_increment;

       g_signal_connect (scale_adj, "value-changed",
                         G_CALLBACK (gimp_scale_entry_exp_adjustment_callback),
                         adj);

       g_signal_connect (adj, "value-changed",
                         G_CALLBACK (gimp_scale_entry_log_adjustment_callback),
                         scale_adj);

       g_object_set_data (G_OBJECT (adjustment),
                          "logarithmic", GINT_TO_POINTER (TRUE));
    }
  else
    {
      gdouble lower, upper;

      if (! gimp_scale_entry_get_logarithmic (adjustment))
        return;

      g_signal_handlers_disconnect_by_func (adj,
                                            G_CALLBACK (gimp_scale_entry_log_adjustment_callback),
                                            scale_adj);
      g_signal_handlers_disconnect_by_func (scale_adj,
                                            G_CALLBACK (gimp_scale_entry_exp_adjustment_callback),
                                            adj);

      lower = exp (scale_adj->lower);
      upper = exp (scale_adj->upper);

      if (adj->lower <= 0.0)
        {
          lower += - 0.1  + adj->lower;
          upper += - 0.1  + adj->lower;
        }

      scale_adj->value          = adj->value;
      scale_adj->lower          = lower;
      scale_adj->upper          = upper;
      scale_adj->step_increment = adj->step_increment;
      scale_adj->page_increment = adj->page_increment;

      g_signal_connect (scale_adj, "value-changed",
                        G_CALLBACK (gimp_scale_entry_unconstrained_adjustment_callback),
                        adj);

      g_signal_connect (adj, "value-changed",
                        G_CALLBACK (gimp_scale_entry_unconstrained_adjustment_callback),
                        scale_adj);

      g_object_set_data (G_OBJECT (adjustment),
                         "logarithmic", GINT_TO_POINTER (FALSE));
    }
}

/**
 * gimp_scale_entry_get_logarithmic:
 * @adjustment: a  #GtkAdjustment as returned by gimp_scale_entry_new()
 *
 * Return value: %TRUE if the the entry's scale widget will behave in
 *               logharithmic fashion, %FALSE for linear behaviour.
 *
 * Since: GIMP 2.2
 **/
gboolean
gimp_scale_entry_get_logarithmic (GtkObject *adjustment)
{
  return GPOINTER_TO_INT (g_object_get_data (G_OBJECT (adjustment),
                                             "logarithmic"));
}

/**
 * gimp_scale_entry_set_sensitive:
 * @adjustment: a #GtkAdjustment returned by gimp_scale_entry_new()
 * @sensitive:  a boolean value with the same semantics as the @sensitive
 *              parameter of gtk_widget_set_sensitive()
 *
 * Sets the sensitivity of the scale_entry's #GtkLabel, #GtkHScale and
 * #GtkSpinbutton.
 **/
void
gimp_scale_entry_set_sensitive (GtkObject *adjustment,
                                gboolean   sensitive)
{
  GtkWidget *widget;

  g_return_if_fail (GTK_IS_ADJUSTMENT (adjustment));

  widget = GIMP_SCALE_ENTRY_LABEL (adjustment);
  if (widget)
    gtk_widget_set_sensitive (widget, sensitive);

  widget = GIMP_SCALE_ENTRY_SCALE (adjustment);
  if (widget)
    gtk_widget_set_sensitive (widget, sensitive);

  widget = GIMP_SCALE_ENTRY_SPINBUTTON (adjustment);
  if (widget)
    gtk_widget_set_sensitive (widget, sensitive);
}
