/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * gimpscalecontrol.c
 * Copyright (C) 2000 Michael Natterer <mitch@gimp.org>
 *               2008 Bill Skaggs <weskaggs@primate.ucdavis.edu>
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

#include "gimpwidgetstypes.h"

#include "gimphelpui.h"
#include "gimpcolorscale.h"
#include "gimpscalecontrol.h"

static void             gimp_scale_control_class_init       (GimpScaleControlClass *klass);

static void             gimp_scale_control_init             (GimpScaleControl *control);

static void             gimp_scale_control_finalize         (GObject        *object);

static void             unconstrained_adjustment_callback (GtkAdjustment  *adjustment,
                                                           GtkAdjustment  *other_adj);

static void             exp_adjustment_callback           (GtkAdjustment  *adjustment,
                                                           GtkAdjustment  *other_adj);

static void             log_adjustment_callback           (GtkAdjustment  *adjustment,
                                                           GtkAdjustment  *other_adj);

static GimpScaleControl * gimp_scale_control_create           (gboolean        color_scale,
                                                           gint            scale_width,
                                                           gint            spinbutton_width,
                                                           gdouble         value,
                                                           gdouble         lower,
                                                           gdouble         upper,
                                                           gdouble         step_increment,
                                                           gdouble         page_increment,
                                                           guint           digits,
                                                           gboolean        constrain,
                                                           gdouble         unconstrained_lower,
                                                           gdouble         unconstrained_upper,
                                                           const gchar    *tooltip,
                                                           const gchar    *help_id);

static void            gimp_scale_control_create_widgets    (GimpScaleControl *scale_control);

static GtkWidget *     gimp_scale_control_create_label      (GimpScaleControl *scale_control,
                                                           const gchar    *text);

G_DEFINE_TYPE (GimpScaleControl, gimp_scale_control, GTK_TYPE_ADJUSTMENT)

#define parent_class gimp_scale_control_parent_class

static void
gimp_scale_control_class_init (GimpScaleControlClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->finalize = gimp_scale_control_finalize;
}

static void
gimp_scale_control_init (GimpScaleControl *scale_control)
{
  scale_control->color_scale         = FALSE;
  scale_control->scale_width         = 0;
  scale_control->spinbutton_width    = 0;
  scale_control->digits              = 0;
  scale_control->constrain           = FALSE;
  scale_control->unconstrained_lower = 0;
  scale_control->unconstrained_upper = 0;
  scale_control->tooltip             = NULL;
  scale_control->help_id             = NULL;
  scale_control->logarithmic         = FALSE;
  scale_control->policy              = GTK_UPDATE_DISCONTINUOUS;

  scale_control->scale_adj           = NULL;
  scale_control->spinbutton_adj      = NULL;

  scale_control->label               = NULL;
  scale_control->scale               = NULL;
  scale_control->spinbutton          = NULL;
  scale_control->entry               = NULL;
  scale_control->popup_button        = NULL;
}

static void
gimp_scale_control_finalize (GObject *object)
{
}

static void
unconstrained_adjustment_callback (GtkAdjustment *adjustment,
                                   GtkAdjustment *other_adj)
{
  g_signal_handlers_block_by_func (other_adj,
                                   unconstrained_adjustment_callback,
                                   adjustment);

  gtk_adjustment_set_value (other_adj, adjustment->value);

  g_signal_handlers_unblock_by_func (adjustment,
                                     unconstrained_adjustment_callback,
                                     other_adj);
}

static void
log_adjustment_callback (GtkAdjustment *adjustment,
                         GtkAdjustment *other_adj)
{
  gdouble value;

  g_signal_handlers_block_by_func (other_adj,
                                   exp_adjustment_callback,
                                   adjustment);
  if (adjustment->lower <= 0.0)
    value = log (adjustment->value - adjustment->lower + 0.1);
  else
    value = log (adjustment->value);

  gtk_adjustment_set_value (other_adj, value);

  g_signal_handlers_unblock_by_func (other_adj,
                                     exp_adjustment_callback,
                                     adjustment);
}

static void
exp_adjustment_callback (GtkAdjustment *adjustment,
                         GtkAdjustment *other_adj)
{
  gdouble value;

  g_signal_handlers_block_by_func (other_adj,
                                   log_adjustment_callback,
                                   adjustment);

  value = exp (adjustment->value);
  if (other_adj->lower <= 0.0)
    value += other_adj->lower  - 0.1;

  gtk_adjustment_set_value (other_adj, value);

  g_signal_handlers_unblock_by_func (other_adj,
                                     log_adjustment_callback,
                                     adjustment);
}

static GimpScaleControl *
gimp_scale_control_create (gboolean     color_scale,
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
  GimpScaleControl *scale_control;
  GtkAdjustment  *adjustment;

  scale_control = g_object_new (GIMP_TYPE_SCALE_CONTROL, NULL);

  adjustment = GTK_ADJUSTMENT (scale_control);

  adjustment->value          = value;
  adjustment->step_increment = step_increment;
  adjustment->page_increment = page_increment;
  adjustment->page_size      = 0;

  scale_control->color_scale         = color_scale;
  scale_control->scale_width         = scale_width;
  scale_control->spinbutton_width    = spinbutton_width;
  scale_control->digits              = digits;
  scale_control->constrain           = constrain;
  scale_control->unconstrained_lower = unconstrained_lower;
  scale_control->unconstrained_upper = unconstrained_upper;
  scale_control->tooltip             = tooltip;
  scale_control->help_id             = help_id;

  if (! constrain &&
           unconstrained_lower <= lower &&
           unconstrained_upper >= upper)
    {
      adjustment->lower = unconstrained_lower;
      adjustment->upper = unconstrained_upper;

      scale_control->scale_adj = GTK_OBJECT (adjustment);

      scale_control->spinbutton_adj = gtk_adjustment_new (value, lower, upper,
                                                        step_increment,
                                                        page_increment,
                                                        0.0);

      g_signal_connect
        (G_OBJECT (scale_control->scale_adj), "value-changed",
         G_CALLBACK (unconstrained_adjustment_callback),
         scale_control->spinbutton_adj);

      g_signal_connect
        (G_OBJECT (scale_control->spinbutton_adj), "value-changed",
         G_CALLBACK (unconstrained_adjustment_callback),
         scale_control->scale_adj);
    }
  else
    {
      adjustment->lower = lower;
      adjustment->upper = upper;

      scale_control->scale_adj      = GTK_OBJECT (scale_control);
      scale_control->spinbutton_adj = GTK_OBJECT (scale_control);
    }

  return scale_control;
}

/*
 * Create the scale and spinbutton for a scale control.
 */
static void
gimp_scale_control_create_widgets (GimpScaleControl *scale_control)
{
  gint           spinbutton_width = scale_control->spinbutton_width;
  GtkWidget     *scale;
  GtkWidget     *spinbutton;
  GtkAdjustment *adjustment;

  adjustment = GTK_ADJUSTMENT (scale_control->spinbutton_adj);

  spinbutton = gtk_spin_button_new (adjustment,
                                    1.0, scale_control->digits);
  gtk_spin_button_set_numeric (GTK_SPIN_BUTTON (spinbutton), TRUE);


  if (spinbutton_width > 0)
    {
      if (spinbutton_width < 17)
        gtk_entry_set_width_chars (GTK_ENTRY (spinbutton),
                                   spinbutton_width);
      else
        gtk_widget_set_size_request (spinbutton,
                                     spinbutton_width, -1);
    }

  adjustment = GTK_ADJUSTMENT (scale_control->scale_adj);

  if (scale_control->color_scale)
    {
      scale = gimp_color_scale_new (GTK_ORIENTATION_HORIZONTAL,
                                    GIMP_COLOR_SELECTOR_VALUE);

      gtk_range_set_adjustment (GTK_RANGE (scale), adjustment);
    }
  else
    {
      scale = gtk_hscale_new (adjustment);
    }

  if (scale_control->scale_width > 0)
    gtk_widget_set_size_request (scale, scale_control->scale_width, -1);
  gtk_scale_set_digits (GTK_SCALE (scale), scale_control->digits);
  gtk_scale_set_draw_value (GTK_SCALE (scale), FALSE);

  scale_control->spinbutton = spinbutton;
  scale_control->scale = scale;
}

/*
 * returns either a label or an event box containing a label,
 * depending on whether there is a tooltip
 */
static GtkWidget *
gimp_scale_control_create_label (GimpScaleControl *scale_control,
                               const gchar    *text)
{

  GtkWidget *label = gtk_label_new_with_mnemonic (text);

  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);

  if (scale_control->tooltip)
    {
      GtkWidget *ebox = g_object_new (GTK_TYPE_EVENT_BOX,
                                      "visible-window", FALSE,
                                      NULL);
      gtk_container_add (GTK_CONTAINER (ebox), label);
      gtk_widget_show (label);

      gimp_help_set_help_data (ebox, scale_control->tooltip,
                               scale_control->help_id);
      return ebox;
    }
  else
    return label;
}

/**
 * gimp_scale_control_new:
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
gimp_scale_control_new (GtkTable    *table,
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
  GimpScaleControl *scale_control;

  scale_control = gimp_scale_control_create (FALSE, scale_width, spinbutton_width,
                                         value, lower, upper, step_increment,
                                         page_increment, digits, constrain,
                                         unconstrained_upper, unconstrained_lower,
                                         tooltip, help_id);

  gimp_scale_control_create_widgets (scale_control);

  scale_control->label = gimp_scale_control_create_label (scale_control, text);

  gtk_table_attach (GTK_TABLE (table), scale_control->label,
                    column, column + 1, row, row + 1,
                    GTK_FILL, GTK_FILL, 0, 0);
  gtk_widget_show (scale_control->label);
  gtk_table_attach (GTK_TABLE (table), scale_control->scale,
                    column + 1, column + 2, row, row + 1,
                    GTK_FILL | GTK_EXPAND, GTK_FILL, 0, 0);
  gtk_widget_show (scale_control->scale);
  gtk_table_attach (GTK_TABLE (table), scale_control->spinbutton,
                    column + 2, column + 3, row, row + 1,
                    GTK_SHRINK, GTK_SHRINK, 0, 0);
  gtk_widget_show (scale_control->spinbutton);

  return GTK_OBJECT (scale_control);
}

GtkObject *
gimp_scale_control_compact_new (GtkTable    *table,
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
  GimpScaleControl *scale_control;
  GtkWidget      *entry;
  GtkWidget      *popup_button;
  GtkWidget      *arrow;
  gchar           value_str[20];

  scale_control = gimp_scale_control_create (FALSE, scale_width, spinbutton_width,
                                         value, lower, upper, step_increment,
                                         page_increment, digits, constrain,
                                         unconstrained_upper, unconstrained_lower,
                                         tooltip, help_id);

  entry = gtk_entry_new ();
  sprintf (value_str, "%lg", value);
  gtk_entry_set_width_chars (GTK_ENTRY (entry), 8);
  gtk_entry_set_has_frame (GTK_ENTRY (entry), FALSE);
  gtk_entry_set_text (GTK_ENTRY (entry), value_str);

  popup_button = gtk_button_new ();
  gtk_button_set_relief (GTK_BUTTON (popup_button), GTK_RELIEF_NONE);

  arrow = gtk_arrow_new (GTK_ARROW_DOWN, GTK_SHADOW_ETCHED_OUT);
  gtk_container_add (GTK_CONTAINER (popup_button), arrow);
  gtk_widget_show (arrow);

  gtk_table_attach (GTK_TABLE (table), entry,
                    column + 1, column + 2, row, row + 1,
                    GTK_FILL, GTK_FILL, 0, 0);
  gtk_widget_show (entry);
  gtk_table_attach (GTK_TABLE (table), popup_button,
                    column + 2, column + 3, row, row + 1,
                    GTK_SHRINK, GTK_FILL, 0, 0);
  gtk_widget_show (popup_button);

  scale_control->entry        = entry;
  scale_control->popup_button = popup_button;

  return GTK_OBJECT (scale_control);
}

/**
 * gimp_color_scale_control_new:
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
gimp_color_scale_control_new (GtkTable    *table,
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
  GimpScaleControl *scale_control;

  scale_control = gimp_scale_control_create (TRUE, scale_width, spinbutton_width,
                                         value, lower, upper, step_increment,
                                         page_increment, digits, TRUE,
                                         0, 0, tooltip, help_id);

  gimp_scale_control_create_widgets (scale_control);

  scale_control->label = gimp_scale_control_create_label (scale_control, text);

  gtk_table_attach (GTK_TABLE (table), scale_control->label,
                    column, column + 1, row, row + 1,
                    GTK_FILL, GTK_FILL, 0, 0);
  gtk_widget_show (scale_control->label);
  gtk_table_attach (GTK_TABLE (table), scale_control->scale,
                    column + 1, column + 2, row, row + 1,
                    GTK_FILL | GTK_EXPAND, GTK_FILL, 0, 0);
  gtk_widget_show (scale_control->scale);
  gtk_table_attach (GTK_TABLE (table), scale_control->spinbutton,
                    column + 2, column + 3, row, row + 1,
                    GTK_SHRINK, GTK_SHRINK, 0, 0);
  gtk_widget_show (scale_control->spinbutton);

  return GTK_OBJECT (scale_control);
}

/**
 * gimp_scale_control_set_logarithmic:
 * @adjustment:  a  #GtkAdjustment as returned by gimp_scale_control_new()
 * @logarithmic: a boolean value to set or reset logarithmic behaviour
 *               of the scale widget
 *
 * Sets whether the scale_control's scale widget will behave in a linear
 * or logharithmic fashion. Useful when an control has to attend large
 * ranges, but smaller selections on that range require a finer
 * adjustment.
 *
 * Since: GIMP 2.2
 **/
void
gimp_scale_control_set_logarithmic (GtkObject *adjustment,
                                  gboolean   logarithmic)
{
  GtkAdjustment  *adj;
  GtkAdjustment  *scale_adj;
  gdouble         correction;
  gdouble         log_value, log_lower, log_upper;
  gdouble         log_step_increment, log_page_increment;
  GimpScaleControl *scale_control;

  g_return_if_fail (GIMP_IS_SCALE_CONTROL (adjustment));

  adj         = GTK_ADJUSTMENT (adjustment);
  scale_control = GIMP_SCALE_CONTROL (adjustment);

  scale_adj = GTK_ADJUSTMENT (scale_control->scale_adj);

  if (logarithmic)
    {
      if (gimp_scale_control_get_logarithmic (adjustment))
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
          gtk_range_set_adjustment (GTK_RANGE (scale_control->scale),
                                    GTK_ADJUSTMENT (new_adj));

          scale_adj = (GtkAdjustment *) new_adj;
          scale_control->scale_adj = GTK_OBJECT (scale_adj);
         }
       else
         {
           g_signal_handlers_disconnect_by_func
             (adj,
              G_CALLBACK (unconstrained_adjustment_callback),
              scale_adj);
           g_signal_handlers_disconnect_by_func
             (scale_adj,
              G_CALLBACK (unconstrained_adjustment_callback),
              adj);
         }

       scale_adj->value          = log_value;
       scale_adj->lower          = log_lower;
       scale_adj->upper          = log_upper;
       scale_adj->step_increment = log_step_increment;
       scale_adj->page_increment = log_page_increment;

       g_signal_connect (scale_adj, "value-changed",
                         G_CALLBACK (exp_adjustment_callback),
                         adj);

       g_signal_connect (adj, "value-changed",
                         G_CALLBACK (log_adjustment_callback),
                         scale_adj);

       g_object_set_data (G_OBJECT (adjustment),
                          "logarithmic", GINT_TO_POINTER (TRUE));

  scale_control->logarithmic = TRUE;
    }
  else
    {
      gdouble lower, upper;

      if (! gimp_scale_control_get_logarithmic (adjustment))
        return;

      g_signal_handlers_disconnect_by_func (adj,
                                            G_CALLBACK (log_adjustment_callback),
                                            scale_adj);
      g_signal_handlers_disconnect_by_func (scale_adj,
                                            G_CALLBACK (exp_adjustment_callback),
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
                        G_CALLBACK (unconstrained_adjustment_callback),
                        adj);

      g_signal_connect (adj, "value-changed",
                        G_CALLBACK (unconstrained_adjustment_callback),
                        scale_adj);

      scale_control->logarithmic = FALSE;
    }
}

/**
 * gimp_scale_control_get_logarithmic:
 * @adjustment: a  #GtkAdjustment as returned by gimp_scale_control_new()
 *
 * Return value: %TRUE if the the control's scale widget will behave in
 *               logharithmic fashion, %FALSE for linear behaviour.
 *
 * Since: GIMP 2.2
 **/
gboolean
gimp_scale_control_get_logarithmic (GtkObject *adjustment)
{
  g_return_val_if_fail (GIMP_IS_SCALE_CONTROL (adjustment), FALSE);

  return GIMP_SCALE_CONTROL (adjustment)->logarithmic;
}

/**
 * gimp_scale_control_set_sensitive:
 * @adjustment: a #GtkAdjustment returned by gimp_scale_control_new()
 * @sensitive:  a boolean value with the same semantics as the @sensitive
 *              parameter of gtk_widget_set_sensitive()
 *
 * Sets the sensitivity of the scale_control's #GtkLabel, #GtkHScale and
 * #GtkSpinbutton.
 **/
void
gimp_scale_control_set_sensitive (GtkObject *adjustment,
                                gboolean   sensitive)
{
  GimpScaleControl *scale_control;

  g_return_if_fail (GIMP_IS_SCALE_CONTROL (adjustment));

  scale_control = GIMP_SCALE_CONTROL (adjustment);

  if (scale_control->label)
    gtk_widget_set_sensitive (scale_control->label, sensitive);

  if (scale_control->spinbutton)
    gtk_widget_set_sensitive (scale_control->spinbutton, sensitive);

  if (scale_control->scale)
    gtk_widget_set_sensitive (scale_control->scale, sensitive);

  if (scale_control->entry)
    gtk_widget_set_sensitive (scale_control->entry, sensitive);

  if (scale_control->popup_button)
    gtk_widget_set_sensitive (scale_control->popup_button, sensitive);
}

/**
 * gimp_scale_control_set_update_policy:
 * @adjustment: a #GimpScaleControl
 * @policy:     a policy for updating the adjustment.
 *
 * Determines how rapidly the widget responds to changes
 * in value.
 *
 * Since: GIMP 2.6
 **/
void
gimp_scale_control_set_update_policy (GtkObject     *adjustment,
                                    GtkUpdateType  policy)
{
  g_return_if_fail (GIMP_IS_SCALE_CONTROL (adjustment));

  GIMP_SCALE_CONTROL (adjustment)->policy = policy;
}
