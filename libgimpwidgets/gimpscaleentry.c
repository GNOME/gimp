/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * gimpscaleentry.c
 * Copyright (C) 2000 Michael Natterer <mitch@gimp.org>
 * Copyright (C) 2020 Jehan
 *
 * This library is free software: you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library.  If not, see
 * <https://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <gegl.h>
#include <gtk/gtk.h>

#include "libgimpmath/gimpmath.h"
#include "libgimpbase/gimpbase.h"

#include "gimpwidgets.h"


/**
 * SECTION: gimpscaleentry
 * @title: GimpScaleEntry
 * @short_description: Widget containing a scale, a spin button and a
 *                     label.
 *
 * This widget is a #GtkGrid showing a #GtkSpinButton and a #GtkScale
 * bound together. It also displays a #GtkLabel which is used as
 * mnemonic on the #GtkSpinButton.
 **/

typedef struct _GimpScaleEntryPrivate
{
  GtkWidget     *scale;
  GBinding      *binding;

  gboolean       logarithmic;
  gboolean       limit_scale;
} GimpScaleEntryPrivate;


static void       gimp_scale_entry_constructed       (GObject       *object);

static gboolean   gimp_scale_entry_linear_to_log     (GBinding     *binding,
                                                      const GValue *from_value,
                                                      GValue       *to_value,
                                                      gpointer      user_data);
static gboolean   gimp_scale_entry_log_to_linear     (GBinding     *binding,
                                                      const GValue *from_value,
                                                      GValue       *to_value,
                                                      gpointer      user_data);
static void       gimp_scale_entry_configure         (GimpScaleEntry *entry);


G_DEFINE_TYPE_WITH_PRIVATE (GimpScaleEntry, gimp_scale_entry, GIMP_TYPE_LABEL_SPIN)

#define parent_class gimp_scale_entry_parent_class

static void
gimp_scale_entry_class_init (GimpScaleEntryClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->constructed  = gimp_scale_entry_constructed;

  klass->new_range_widget    = NULL;
}

static void
gimp_scale_entry_init (GimpScaleEntry *entry)
{
  GimpScaleEntryPrivate *priv  = gimp_scale_entry_get_instance_private (entry);

  priv->limit_scale = FALSE;
}

static void
gimp_scale_entry_constructed (GObject *object)
{
  GimpScaleEntryClass   *klass;
  GimpScaleEntry        *entry = GIMP_SCALE_ENTRY (object);
  GimpScaleEntryPrivate *priv  = gimp_scale_entry_get_instance_private (entry);
  GtkAdjustment         *scale_adjustment;
  GtkAdjustment         *spin_adjustment;
  GtkWidget             *spinbutton;

  G_OBJECT_CLASS (parent_class)->constructed (object);

  spinbutton = gimp_label_spin_get_spin_button (GIMP_LABEL_SPIN (entry));
  spin_adjustment = gtk_spin_button_get_adjustment (GTK_SPIN_BUTTON (spinbutton));
  scale_adjustment = gtk_adjustment_new (gtk_adjustment_get_value (spin_adjustment),
                                         gtk_adjustment_get_lower (spin_adjustment),
                                         gtk_adjustment_get_upper (spin_adjustment),
                                         gtk_adjustment_get_step_increment (spin_adjustment),
                                         gtk_adjustment_get_page_increment (spin_adjustment),
                                         gtk_adjustment_get_page_size (spin_adjustment));

  klass = GIMP_SCALE_ENTRY_GET_CLASS (entry);
  if (klass->new_range_widget)
    {
      priv->scale = klass->new_range_widget (scale_adjustment);
      g_return_if_fail (GTK_IS_RANGE (priv->scale));
      g_return_if_fail (scale_adjustment == gtk_range_get_adjustment (GTK_RANGE (priv->scale)));
    }
  else
    {
      priv->scale = gtk_scale_new (GTK_ORIENTATION_HORIZONTAL, scale_adjustment);
      gtk_scale_set_draw_value (GTK_SCALE (priv->scale), FALSE);

      g_object_bind_property (G_OBJECT (spinbutton),  "digits",
                              G_OBJECT (priv->scale), "digits",
                              G_BINDING_BIDIRECTIONAL |
                              G_BINDING_SYNC_CREATE);
    }

  gtk_widget_set_hexpand (priv->scale, TRUE);

  /* Move the spin button to the right. */
  gtk_container_remove (GTK_CONTAINER (entry), g_object_ref (spinbutton));
  gtk_grid_attach (GTK_GRID (entry), spinbutton, 2, 0, 1, 1);

  gtk_grid_attach (GTK_GRID (entry), priv->scale, 1, 0, 1, 1);
  gtk_widget_show (priv->scale);

  g_signal_connect_swapped (spin_adjustment, "changed",
                            G_CALLBACK (gimp_scale_entry_configure),
                            entry);
  gimp_scale_entry_configure (entry);
}

static gboolean
gimp_scale_entry_linear_to_log (GBinding     *binding,
                                const GValue *from_value,
                                GValue       *to_value,
                                gpointer      user_data)
{
  GtkAdjustment *spin_adjustment;
  gdouble        value = g_value_get_double (from_value);

  spin_adjustment = GTK_ADJUSTMENT (g_binding_dup_source (binding));

  if (gtk_adjustment_get_lower (spin_adjustment) <= 0.0)
    value = log (value - gtk_adjustment_get_lower (spin_adjustment) + 0.1);
  else
    value = log (value);

  g_value_set_double (to_value, value);

  g_clear_object (&spin_adjustment);

  return TRUE;
}

static gboolean
gimp_scale_entry_log_to_linear (GBinding     *binding,
                                const GValue *from_value,
                                GValue       *to_value,
                                gpointer      user_data)
{
  GtkAdjustment *spin_adjustment;
  gdouble        value = g_value_get_double (from_value);

  spin_adjustment = GTK_ADJUSTMENT (g_binding_dup_source (binding));

  value = exp (value);

  if (gtk_adjustment_get_lower (spin_adjustment) <= 0.0)
    value += gtk_adjustment_get_lower (spin_adjustment) - 0.1;

  g_value_set_double (to_value, value);

  g_clear_object (&spin_adjustment);

  return TRUE;
}

static void
gimp_scale_entry_configure (GimpScaleEntry *entry)
{
  GimpScaleEntryPrivate *priv;
  GBinding              *binding;
  GtkWidget             *spinbutton;
  GtkAdjustment         *spin_adj;
  GtkAdjustment         *scale_adj;
  gdouble                scale_lower;
  gdouble                scale_upper;

  g_return_if_fail (GIMP_IS_SCALE_ENTRY (entry));

  priv      = gimp_scale_entry_get_instance_private (entry);
  scale_adj = gtk_range_get_adjustment (GTK_RANGE (priv->scale));

  spinbutton = gimp_label_spin_get_spin_button (GIMP_LABEL_SPIN (entry));
  spin_adj  = gtk_spin_button_get_adjustment (GTK_SPIN_BUTTON (spinbutton));

  g_clear_object (&priv->binding);

  if (priv->limit_scale)
    {
      scale_lower = gtk_adjustment_get_lower (scale_adj);
      scale_upper = gtk_adjustment_get_upper (scale_adj);
    }
  else
    {
      scale_lower = gtk_adjustment_get_lower (spin_adj);
      scale_upper = gtk_adjustment_get_upper (spin_adj);
    }

  if (priv->logarithmic)
    {
      gdouble correction;
      gdouble log_value, log_lower, log_upper;
      gdouble log_step_increment, log_page_increment;

      correction = (scale_lower > 0 ?  0 : 0.1 + - scale_lower);

      log_value = log (gtk_adjustment_get_value (scale_adj) + correction);
      log_lower = log (scale_lower + correction);
      log_upper = log (scale_upper + correction);
      log_step_increment =
        (log_upper - log_lower) / ((scale_upper - scale_lower) /
                                   gtk_adjustment_get_step_increment (spin_adj));
      log_page_increment =
        (log_upper - log_lower) / ((scale_upper - scale_lower) /
                                   gtk_adjustment_get_page_increment (spin_adj));

      gtk_adjustment_configure (scale_adj,
                                log_value, log_lower, log_upper,
                                log_step_increment, log_page_increment, 0.0);

      binding = g_object_bind_property_full (G_OBJECT (spin_adj),  "value",
                                             G_OBJECT (scale_adj), "value",
                                             G_BINDING_BIDIRECTIONAL |
                                             G_BINDING_SYNC_CREATE,
                                             gimp_scale_entry_linear_to_log,
                                             gimp_scale_entry_log_to_linear,
                                             NULL, NULL);
    }
  else
    {
      gtk_adjustment_configure (scale_adj,
                                gtk_adjustment_get_value (spin_adj),
                                scale_lower, scale_upper,
                                gtk_adjustment_get_step_increment (spin_adj),
                                gtk_adjustment_get_page_increment (spin_adj),
                                0.0);

      binding = g_object_bind_property (G_OBJECT (spin_adj),  "value",
                                        G_OBJECT (scale_adj), "value",
                                        G_BINDING_BIDIRECTIONAL |
                                        G_BINDING_SYNC_CREATE);
    }

  priv->binding = binding;
}

/* Public functions */

/**
 * gimp_scale_entry_new:
 * @text:           The text for the #GtkLabel which will appear left of
 *                  the #GtkHScale.
 * @value:          The initial value.
 * @lower:          The lower boundary.
 * @upper:          The upper boundary.
 * @digits:         The number of decimal digits.
 *
 * This function creates a #GtkLabel, a #GtkHScale and a #GtkSpinButton and
 * attaches them to a 3-column #GtkGrid.
 *
 * Returns: (transfer full): The new #GimpScaleEntry.
 **/
GtkWidget *
gimp_scale_entry_new  (const gchar *text,
                       gdouble      value,
                       gdouble      lower,
                       gdouble      upper,
                       guint        digits)
{
  GtkWidget *entry;

  entry = g_object_new (GIMP_TYPE_SCALE_ENTRY,
                        "label",          text,
                        "value",          value,
                        "lower",          lower,
                        "upper",          upper,
                        "digits",         digits,
                        NULL);

  return entry;
}

/**
 * gimp_scale_entry_get_range:
 * @entry: The #GtkScaleEntry.
 *
 * This function returns the #GtkRange packed in @entry. This can be
 * useful if you need to customize some aspects of the widget
 *
 * By default, it is a #GtkScale, but it can be any other type of
 * #GtkRange if a subclass overrode the new_range_widget() protected
 * method.
 *
 * Returns: (transfer none) (type GtkRange): The #GtkRange contained in @entry.
 **/
GtkWidget *
gimp_scale_entry_get_range (GimpScaleEntry *entry)
{
  GimpScaleEntryPrivate *priv = gimp_scale_entry_get_instance_private (entry);

  g_return_val_if_fail (GIMP_IS_SCALE_ENTRY (entry), NULL);

  return priv->scale;
}

/**
 * gimp_scale_entry_set_bounds:
 * @entry:       The #GtkScaleEntry.
 * @lower:       the lower value for the whole widget if @limit_scale is
 *               %FALSE, or only for the #GtkScale if %TRUE.
 * @upper:       the upper value for the whole widget if @limit_scale is
 *               %FALSE, or only for the #GtkSpinButton if %TRUE.
 * @limit_scale: Whether the range should only apply to the #GtkScale or
 *               if it should share its #GtkAdjustement with the
 *               #GtkSpinButton. If %TRUE, both @lower and @upper must be
 *               included in current #GtkSpinButton range.
 *
 * By default the #GtkSpinButton and #GtkScale will have the same range.
 * In some case, you want to set a different range. In particular when
 * the finale range is huge, the #GtkScale might become nearly useless
 * as every tiny slider move would dramatically update the value. In
 * this case, it is common to set the #GtkScale to a smaller common
 * range, while the #GtkSpinButton would allow for the full allowed
 * range.
 * This function allows this. Obviously the #GtkAdjustment of both
 * widgets would be synced but if the set value is out of the #GtkScale
 * range, the slider would simply show at one extreme.
 *
 * If @limit_scale is %FALSE though, it would sync back both widgets
 * range to the new values.
 *
 * Note that the step and page increments are updated when the range is
 * updated according to some common usage algorithm which should work if
 * you don't have very specific needs. If you want to customize the step
 * increments yourself, you may call gimp_label_spin_set_increments()
 **/
void
gimp_scale_entry_set_bounds (GimpScaleEntry *entry,
                             gdouble         lower,
                             gdouble         upper,
                             gboolean        limit_scale)
{
  GimpScaleEntryPrivate *priv;
  GtkWidget             *spinbutton;
  GtkAdjustment         *spin_adjustment;
  GtkAdjustment         *scale_adjustment;

  g_return_if_fail (GIMP_IS_SCALE_ENTRY (entry));
  g_return_if_fail (lower <= upper);

  priv             = gimp_scale_entry_get_instance_private (entry);
  scale_adjustment = gtk_range_get_adjustment (GTK_RANGE (priv->scale));

  spinbutton = gimp_label_spin_get_spin_button (GIMP_LABEL_SPIN (entry));
  spin_adjustment  = gtk_spin_button_get_adjustment (GTK_SPIN_BUTTON (spinbutton));

  priv->limit_scale = limit_scale;

  if (limit_scale)
    {
      g_return_if_fail (lower >= gtk_adjustment_get_lower (spin_adjustment) &&
                        upper <= gtk_adjustment_get_upper (spin_adjustment));

      gtk_adjustment_set_lower (scale_adjustment, lower);
      gtk_adjustment_set_upper (scale_adjustment, upper);

      gimp_scale_entry_configure (entry);
    }
  else if (! limit_scale)
    {
      g_object_set (entry,
                    "lower", lower,
                    "upper", upper,
                    NULL);
    }
}

/**
 * gimp_scale_entry_set_logarithmic:
 * @entry:       a #GimpScaleEntry as returned by gimp_scale_entry_new()
 * @logarithmic: a boolean value to set or reset logarithmic behavior
 *               of the scale widget
 *
 * Sets whether @entry's scale widget will behave in a linear
 * or logarithmic fashion. Useful when an entry has to attend large
 * ranges, but smaller selections on that range require a finer
 * adjustment.
 *
 * Since: 2.2
 **/
void
gimp_scale_entry_set_logarithmic (GimpScaleEntry *entry,
                                  gboolean        logarithmic)
{
  GimpScaleEntryPrivate *priv;

  g_return_if_fail (GIMP_IS_SCALE_ENTRY (entry));

  priv = gimp_scale_entry_get_instance_private (entry);

  if (logarithmic != priv->logarithmic)
    {
      priv->logarithmic = logarithmic;
      gimp_scale_entry_configure (entry);
    }
}

/**
 * gimp_scale_entry_get_logarithmic:
 * @entry: a #GimpScaleEntry as returned by gimp_scale_entry_new()
 *
 * Returns: %TRUE if @entry's scale widget will behave in
 *          logarithmic fashion, %FALSE for linear behavior.
 *
 * Since: 2.2
 **/
gboolean
gimp_scale_entry_get_logarithmic (GimpScaleEntry *entry)
{
  GimpScaleEntryPrivate *priv = gimp_scale_entry_get_instance_private (entry);

  g_return_val_if_fail (GIMP_IS_SCALE_ENTRY (entry), FALSE);

  return priv->logarithmic;
}
