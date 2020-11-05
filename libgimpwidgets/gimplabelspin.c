/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * gimplabelspin.c
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

#include "libgimpcolor/gimpcolor.h"
#include "libgimpmath/gimpmath.h"
#include "libgimpbase/gimpbase.h"

#include "gimpwidgets.h"


/**
 * SECTION: gimplabelspin
 * @title: GimpLabelSpin
 * @short_description: Widget containing a spin button and a label.
 *
 * This widget is a subclass of #GimpLabeled with a #GimpSpinButton.
 **/

enum
{
  VALUE_CHANGED,
  LAST_SIGNAL
};

enum
{
  PROP_0,
  PROP_VALUE,
  PROP_LOWER,
  PROP_UPPER,
  PROP_DIGITS,
};

typedef struct _GimpLabelSpinPrivate
{
  GimpLabeled    parent_instance;

  GtkWidget     *spinbutton;
  GtkAdjustment *spin_adjustment;
} GimpLabelSpinPrivate;

static void        gimp_label_spin_constructed       (GObject       *object);
static void        gimp_label_spin_set_property      (GObject       *object,
                                                      guint          property_id,
                                                      const GValue  *value,
                                                      GParamSpec    *pspec);
static void        gimp_label_spin_get_property      (GObject       *object,
                                                      guint          property_id,
                                                      GValue        *value,
                                                      GParamSpec    *pspec);

static GtkWidget * gimp_label_spin_populate          (GimpLabeled   *spin,
                                                      gint          *x,
                                                      gint          *y,
                                                      gint          *width,
                                                      gint          *height);

static void        gimp_label_spin_update_spin_width (GimpLabelSpin *spin);
static void        gimp_label_spin_update_increments (GimpLabelSpin *spin);

G_DEFINE_TYPE_WITH_PRIVATE (GimpLabelSpin, gimp_label_spin, GIMP_TYPE_LABELED)

#define parent_class gimp_label_spin_parent_class

static guint gimp_label_spin_signals[LAST_SIGNAL] = { 0 };

static void
gimp_label_spin_class_init (GimpLabelSpinClass *klass)
{
  GObjectClass     *object_class  = G_OBJECT_CLASS (klass);
  GimpLabeledClass *labeled_class = GIMP_LABELED_CLASS (klass);

  gimp_label_spin_signals[VALUE_CHANGED] =
    g_signal_new ("value-changed",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (GimpLabelSpinClass, value_changed),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 0);

  object_class->constructed  = gimp_label_spin_constructed;
  object_class->set_property = gimp_label_spin_set_property;
  object_class->get_property = gimp_label_spin_get_property;

  labeled_class->populate    = gimp_label_spin_populate;

  /**
   * GimpLabelSpin:value:
   *
   * The currently set value.
   *
   * Since: 3.0
   **/
  g_object_class_install_property (object_class, PROP_VALUE,
                                   g_param_spec_double ("value", NULL,
                                                        "Current value",
                                                        -G_MAXDOUBLE, G_MAXDOUBLE, 1.0,
                                                        GIMP_PARAM_READWRITE));

  /**
   * GimpLabelSpin:lower:
   *
   * The lower bound of the spin button.
   *
   * Since: 3.0
   **/
  g_object_class_install_property (object_class, PROP_LOWER,
                                   g_param_spec_double ("lower", NULL,
                                                        "Minimum value",
                                                        -G_MAXDOUBLE, G_MAXDOUBLE, 1.0,
                                                        GIMP_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT));

  /**
   * GimpLabelSpin:upper:
   *
   * The upper bound of the spin button.
   *
   * Since: 3.0
   **/
  g_object_class_install_property (object_class, PROP_UPPER,
                                   g_param_spec_double ("upper", NULL,
                                                        "Max value",
                                                        -G_MAXDOUBLE, G_MAXDOUBLE, 0.0,
                                                        GIMP_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT));

  /**
   * GimpLabelSpin:digits:
   *
   * The number of decimal places to display.
   *
   * Since: 3.0
   **/
  g_object_class_install_property (object_class, PROP_DIGITS,
                                   g_param_spec_uint ("digits", NULL,
                                                      "The number of decimal places to display",
                                                      0, G_MAXUINT, 2,
                                                      GIMP_PARAM_READWRITE));
}

static void
gimp_label_spin_init (GimpLabelSpin *spin)
{
  GimpLabelSpinPrivate *priv  = gimp_label_spin_get_instance_private (spin);

  /* We want the adjustment to exist at init so that construction
   * properties can apply (default values are bogus but should be
   * properly overrided with expected values if the object was created
   * with gimp_label_spin_new().
   */
  priv->spin_adjustment = gtk_adjustment_new (0.0, 0.0, 100.0, 1.0, 10.0, 0.0);

  gtk_grid_set_row_spacing (GTK_GRID (spin), 6);
  gtk_grid_set_column_spacing (GTK_GRID (spin), 6);
}

static void
gimp_label_spin_constructed (GObject *object)
{
  GimpLabelSpin        *spin = GIMP_LABEL_SPIN (object);
  GimpLabelSpinPrivate *priv  = gimp_label_spin_get_instance_private (spin);

  G_OBJECT_CLASS (parent_class)->constructed (object);

  /* This is important to make this object into a property widget. It
   * will allow config object to bind the "value" property of this
   * widget, and therefore be updated automatically.
   */
  g_object_bind_property (G_OBJECT (priv->spin_adjustment), "value",
                          G_OBJECT (spin),                  "value",
                          G_BINDING_BIDIRECTIONAL |
                          G_BINDING_SYNC_CREATE);

  gimp_label_spin_update_spin_width (spin);
  gimp_label_spin_update_increments (spin);
}

static void
gimp_label_spin_set_property (GObject      *object,
                              guint         property_id,
                              const GValue *value,
                              GParamSpec   *pspec)
{
  GimpLabelSpin        *spin = GIMP_LABEL_SPIN (object);
  GimpLabelSpinPrivate *priv  = gimp_label_spin_get_instance_private (spin);

  switch (property_id)
    {
    case PROP_VALUE:
      /* Avoid looping forever since we have bound this widget's
       * "value" property with the spin button "value" property.
       */
      if (gtk_adjustment_get_value (priv->spin_adjustment) != g_value_get_double (value))
        gtk_adjustment_set_value (priv->spin_adjustment, g_value_get_double (value));

      g_signal_emit (object, gimp_label_spin_signals[VALUE_CHANGED], 0);
      break;
    case PROP_LOWER:
      gtk_adjustment_set_lower (priv->spin_adjustment,
                                g_value_get_double (value));
      if (priv->spinbutton)
        {
          gimp_label_spin_update_spin_width (spin);
          gimp_label_spin_update_increments (spin);
        }
      break;
    case PROP_UPPER:
      gtk_adjustment_set_upper (priv->spin_adjustment,
                                g_value_get_double (value));
      if (priv->spinbutton)
        {
          gimp_label_spin_update_spin_width (spin);
          gimp_label_spin_update_increments (spin);
        }
      break;
    case PROP_DIGITS:
      if (priv->spinbutton)
        {
          gtk_spin_button_set_digits (GTK_SPIN_BUTTON (priv->spinbutton),
                                      g_value_get_uint (value));

          gimp_label_spin_update_spin_width (spin);
          gimp_label_spin_update_increments (spin);
        }
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_label_spin_get_property (GObject    *object,
                              guint       property_id,
                              GValue     *value,
                              GParamSpec *pspec)
{
  GimpLabelSpin        *spin       = GIMP_LABEL_SPIN (object);
  GimpLabelSpinPrivate *priv       = gimp_label_spin_get_instance_private (spin);
  GtkSpinButton        *spinbutton = GTK_SPIN_BUTTON (priv->spinbutton);

  switch (property_id)
    {
    case PROP_VALUE:
      g_value_set_double (value, gtk_adjustment_get_value (priv->spin_adjustment));
      break;
    case PROP_LOWER:
      g_value_set_double (value, gtk_adjustment_get_lower (priv->spin_adjustment));
      break;
    case PROP_UPPER:
      g_value_set_double (value, gtk_adjustment_get_upper (priv->spin_adjustment));
      break;
    case PROP_DIGITS:
      g_value_set_uint (value, gtk_spin_button_get_digits (spinbutton));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static GtkWidget *
gimp_label_spin_populate (GimpLabeled *labeled,
                          gint        *x,
                          gint        *y,
                          gint        *width,
                          gint        *height)
{
  GimpLabelSpin        *spin = GIMP_LABEL_SPIN (labeled);
  GimpLabelSpinPrivate *priv = gimp_label_spin_get_instance_private (spin);

  priv->spinbutton = gimp_spin_button_new (priv->spin_adjustment, 2.0, 2.0);
  gtk_spin_button_set_numeric (GTK_SPIN_BUTTON (priv->spinbutton), TRUE);

  gtk_grid_attach (GTK_GRID (spin), priv->spinbutton, 1, 0, 1, 1);
  gtk_widget_show (priv->spinbutton);

  return priv->spinbutton;
}

static void
gimp_label_spin_update_spin_width (GimpLabelSpin *spin)
{
  GimpLabelSpinPrivate *priv = gimp_label_spin_get_instance_private (spin);
  gint                  width = 0;
  gdouble               lower;
  gdouble               upper;
  gint                  digits;

  g_return_if_fail (GIMP_IS_LABEL_SPIN (spin));

  g_object_get (spin,
                "lower",  &lower,
                "upper",  &upper,
                "digits", &digits,
                NULL);

  /* Necessary size to display the max/min integer values, with optional
   * negative sign.
   */
  width = (gint) floor (log10 (upper) + 1) + (upper < 0.0 ? 1 : 0);
  width = MAX (width, (gint) floor (log10 (lower) + 1) + (lower < 0.0 ? 1 : 0));

  /* Adding decimal digits and separator. */
  width += (digits > 0 ? 1 + digits : 0);

  /* Overlong spin button are useless. */
  width = MIN (10, width);

  gtk_entry_set_width_chars (GTK_ENTRY (priv->spinbutton), width);
}

static void
gimp_label_spin_update_increments (GimpLabelSpin *spin)
{
  gdouble lower;
  gdouble upper;
  gdouble range;

  g_return_if_fail (GIMP_IS_LABEL_SPIN (spin));

  g_object_get (spin,
                "lower",  &lower,
                "upper",  &upper,
                NULL);

  g_return_if_fail (upper >= lower);

  range = upper - lower;

  if (range > 0 && range <= 1.0)
    {
      gdouble places = 10.0;
      gdouble step;
      gdouble page;

      /* Compute some acceptable step and page increments always in the
       * format `10**-X` where X is the rounded precision.
       * So for instance:
       *  0.8 will have increments 0.01 and 0.1.
       *  0.3 will have increments 0.001 and 0.01.
       *  0.06 will also have increments 0.001 and 0.01.
       */
      while (range * places < 5.0)
        places *= 10.0;


      step = 0.1 / places;
      page = 1.0 / places;

      gimp_label_spin_set_increments (spin, step, page);
    }
  else if (range <= 2.0)
    {
      gimp_label_spin_set_increments (spin, 0.01, 0.1);
    }
  else if (range <= 5.0)
    {
      gimp_label_spin_set_increments (spin, 0.1, 1.0);
    }
  else if (range <= 40.0)
    {
      gimp_label_spin_set_increments (spin, 1.0, 2.0);
    }
  else
    {
      gimp_label_spin_set_increments (spin, 1.0, 10.0);
    }
}


/* Public Functions */


/**
 * gimp_label_spin_new:
 * @text:   The text for the #GtkLabel.
 * @value:  The initial value.
 * @lower:  The lower boundary.
 * @upper:  The upper boundary.
 * @digits: The number of decimal digits.
 *
 * Returns: (transfer full): The new #GimpLabelSpin widget.
 **/
GtkWidget *
gimp_label_spin_new (const gchar *text,
                     gdouble      value,
                     gdouble      lower,
                     gdouble      upper,
                     guint        digits)
{
  GtkWidget *labeled;

  labeled = g_object_new (GIMP_TYPE_LABEL_SPIN,
                          "label",  text,
                          "value",  value,
                          "lower",  lower,
                          "upper",  upper,
                          "digits", digits,
                          NULL);

  return labeled;
}

/**
 * gimp_label_spin_set_value:
 * @spin: The #GtkLabelSpin.
 * @value: A new value.
 *
 * This function sets the value shown by @spin.
 **/
void
gimp_label_spin_set_value (GimpLabelSpin *spin,
                           gdouble         value)
{
  g_return_if_fail (GIMP_IS_LABEL_SPIN (spin));

  g_object_set (spin,
                "value", value,
                NULL);
}

/**
 * gimp_label_spin_get_value:
 * @spin: The #GtkLabelSpin.
 *
 * This function returns the value shown by @spin.
 *
 * Returns: The value currently set.
 **/
gdouble
gimp_label_spin_get_value (GimpLabelSpin *spin)
{
  gdouble value;

  g_return_val_if_fail (GIMP_IS_LABEL_SPIN (spin), 0.0);

  g_object_get (spin,
                "value", &value,
                NULL);
  return value;
}

/**
 * gimp_label_spin_set_increments:
 * @spin: the #GimpLabelSpin.
 * @step: the step increment.
 * @page: the page increment.
 *
 * Set the step and page increments of the spin button.
 * By default, these increment values are automatically computed
 * depending on the range based on common usage. So you will likely not
 * need to run this for most case. Yet if you want specific increments
 * (which the widget cannot guess), you can call this function.
 */
void
gimp_label_spin_set_increments (GimpLabelSpin *spin,
                                gdouble        step,
                                gdouble        page)
{
  GimpLabelSpinPrivate *priv = gimp_label_spin_get_instance_private (spin);
  GtkSpinButton        *spinbutton;
  gdouble               lower;
  gdouble               upper;

  g_return_if_fail (GIMP_IS_LABEL_SPIN (spin));
  g_return_if_fail (step < page);

  spinbutton = GTK_SPIN_BUTTON (priv->spinbutton);

  gtk_spin_button_get_range (spinbutton, &lower, &upper);
  g_return_if_fail (upper >= lower);
  g_return_if_fail (step < upper - lower && page < upper - lower);

  g_object_freeze_notify (G_OBJECT (spinbutton));
  gtk_adjustment_set_step_increment (gtk_spin_button_get_adjustment (spinbutton), step);
  gtk_adjustment_set_page_increment (gtk_spin_button_get_adjustment (spinbutton), page);
  g_object_thaw_notify (G_OBJECT (spinbutton));

  g_object_set (priv->spinbutton,
                "climb-rate", step,
                NULL);
}

/**
 * gimp_label_spin_get_spin_button:
 * @spin: The #GimpLabelSpin
 *
 * This function returns the #GimpSpinButton packed in @spin.
 *
 * Returns: (transfer none): The #GimpSpinButton contained in @spin.
 **/
GtkWidget *
gimp_label_spin_get_spin_button (GimpLabelSpin *spin)
{
  GimpLabelSpinPrivate *priv = gimp_label_spin_get_instance_private (spin);

  g_return_val_if_fail (GIMP_IS_LABEL_SPIN (spin), NULL);

  return priv->spinbutton;
}
