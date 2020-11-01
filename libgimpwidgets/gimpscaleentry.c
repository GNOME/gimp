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

enum
{
  VALUE_CHANGED,
  LAST_SIGNAL
};

enum
{
  PROP_0,
  PROP_LABEL,
  PROP_VALUE,
  PROP_LOWER,
  PROP_UPPER,
  PROP_DIGITS,
};

typedef struct _GimpScaleEntryPrivate
{
  GtkWidget     *label;
  GtkWidget     *spinbutton;
  GtkWidget     *scale;

  GBinding      *binding;

  GtkAdjustment *spin_adjustment;

  gboolean       logarithmic;
} GimpScaleEntryPrivate;


static void       gimp_scale_entry_constructed       (GObject       *object);
static void       gimp_scale_entry_set_property      (GObject       *object,
                                                      guint          property_id,
                                                      const GValue  *value,
                                                      GParamSpec    *pspec);
static void       gimp_scale_entry_get_property      (GObject       *object,
                                                      guint          property_id,
                                                      GValue        *value,
                                                      GParamSpec    *pspec);

static void       gimp_scale_entry_update_spin_width (GimpScaleEntry *entry);
static void       gimp_scale_entry_update_steps      (GimpScaleEntry *entry);

static gboolean   gimp_scale_entry_linear_to_log     (GBinding     *binding,
                                                      const GValue *from_value,
                                                      GValue       *to_value,
                                                      gpointer      user_data);
static gboolean   gimp_scale_entry_log_to_linear     (GBinding     *binding,
                                                      const GValue *from_value,
                                                      GValue       *to_value,
                                                      gpointer      user_data);


G_DEFINE_TYPE_WITH_PRIVATE (GimpScaleEntry, gimp_scale_entry, GTK_TYPE_GRID)

#define parent_class gimp_scale_entry_parent_class

static guint gimp_scale_entry_signals[LAST_SIGNAL] = { 0 };


static void
gimp_scale_entry_class_init (GimpScaleEntryClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  gimp_scale_entry_signals[VALUE_CHANGED] =
    g_signal_new ("value-changed",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (GimpScaleEntryClass, value_changed),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 0);

  object_class->constructed  = gimp_scale_entry_constructed;
  object_class->set_property = gimp_scale_entry_set_property;
  object_class->get_property = gimp_scale_entry_get_property;

  klass->new_range_widget    = NULL;

  /**
   * GimpScaleEntry:label:
   *
   * Since: 3.0
   **/
  g_object_class_install_property (object_class, PROP_LABEL,
                                   g_param_spec_string ("label",
                                                        "Label text",
                                                        "The text of the label part of this widget",
                                                        NULL,
                                                        GIMP_PARAM_READWRITE));

  /**
   * GimpScaleEntry:value:
   *
   * Since: 3.0
   **/
  g_object_class_install_property (object_class, PROP_VALUE,
                                   g_param_spec_double ("value", NULL,
                                                        "Current value",
                                                        -G_MAXDOUBLE, G_MAXDOUBLE, 1.0,
                                                        GIMP_PARAM_READWRITE));

  /**
   * GimpScaleEntry:lower:
   *
   * The lower bound of the widget. If the spin button and the scale
   * widgets have different limits (see gimp_scale_entry_set_bounds()),
   * this corresponds to the spin button lower value.
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
   * GimpScaleEntry:upper:
   *
   * The upper bound of the widget. If the spin button and the scale
   * widgets have different limits (see gimp_scale_entry_set_bounds()),
   * this corresponds to the spin button upper value.
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
   * GimpScaleEntry:digits:
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
gimp_scale_entry_init (GimpScaleEntry *entry)
{
  GimpScaleEntryPrivate *priv  = gimp_scale_entry_get_instance_private (entry);

  /* The main adjustment (the scale adjustment in particular might be
   * smaller). We want it to exist at init so that construction
   * properties can apply (default values are bogus but should be
   * properly overrided with expected values if the object was created
   * with gimp_scale_entry_new().
   */
  priv->spin_adjustment = gtk_adjustment_new (0.0, 0.0, 100.0, 1.0, 10.0, 0.0);
}

static void
gimp_scale_entry_constructed (GObject *object)
{
  GimpScaleEntryClass   *klass;
  GimpScaleEntry        *entry = GIMP_SCALE_ENTRY (object);
  GimpScaleEntryPrivate *priv  = gimp_scale_entry_get_instance_private (entry);
  GtkAdjustment         *scale_adjustment;

  /* Label */
  priv->label = gtk_label_new_with_mnemonic (NULL);
  gtk_label_set_xalign (GTK_LABEL (priv->label), 0.0);

  /* Spin button */
  priv->spinbutton = gimp_spin_button_new (priv->spin_adjustment, 2.0, 2.0);
  gtk_spin_button_set_numeric (GTK_SPIN_BUTTON (priv->spinbutton), TRUE);
  gtk_label_set_mnemonic_widget (GTK_LABEL (priv->label), priv->spinbutton);

  /* Scale */
  scale_adjustment = gtk_adjustment_new (gtk_adjustment_get_value (priv->spin_adjustment),
                                         gtk_adjustment_get_lower (priv->spin_adjustment),
                                         gtk_adjustment_get_upper (priv->spin_adjustment),
                                         gtk_adjustment_get_step_increment (priv->spin_adjustment),
                                         gtk_adjustment_get_page_increment (priv->spin_adjustment),
                                         gtk_adjustment_get_page_size (priv->spin_adjustment));

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
    }

  gtk_widget_set_hexpand (priv->scale, TRUE);

  gtk_grid_attach (GTK_GRID (entry), priv->label,      0, 0, 1, 1);
  gtk_grid_attach (GTK_GRID (entry), priv->scale,      1, 0, 1, 1);
  gtk_grid_attach (GTK_GRID (entry), priv->spinbutton, 2, 0, 1, 1);
  gtk_widget_show (priv->label);
  gtk_widget_show (priv->scale);
  gtk_widget_show (priv->spinbutton);

  priv->binding = g_object_bind_property (G_OBJECT (priv->spin_adjustment),  "value",
                                          G_OBJECT (scale_adjustment), "value",
                                          G_BINDING_BIDIRECTIONAL |
                                          G_BINDING_SYNC_CREATE);

  /* This is important to make this object into a property widget. It
   * will allow config object to bind the "value" property of this
   * widget, and therefore be updated automatically.
   */
  g_object_bind_property (G_OBJECT (priv->spin_adjustment), "value",
                          object,                     "value",
                          G_BINDING_BIDIRECTIONAL |
                          G_BINDING_SYNC_CREATE);

  gimp_scale_entry_update_spin_width (entry);
  gimp_scale_entry_update_steps (entry);
}

static void
gimp_scale_entry_set_property (GObject      *object,
                               guint         property_id,
                               const GValue *value,
                               GParamSpec   *pspec)
{
  GimpScaleEntry        *entry = GIMP_SCALE_ENTRY (object);
  GimpScaleEntryPrivate *priv  = gimp_scale_entry_get_instance_private (entry);

  switch (property_id)
    {
    case PROP_LABEL:
        {
          /* This should not happen since the property is **not** set with
           * G_PARAM_CONSTRUCT, hence the label should exist when the
           * property is first set.
           */
          g_return_if_fail (priv->label);

          gtk_label_set_markup_with_mnemonic (GTK_LABEL (priv->label),
                                              g_value_get_string (value));
        }
      break;
    case PROP_VALUE:
        {
          GtkSpinButton *spinbutton;
          GtkAdjustment *adj;

          g_return_if_fail (priv->spinbutton);

          /* Set on the spin button, because it might have a bigger
           * range, hence shows the real value.
           */
          spinbutton = GTK_SPIN_BUTTON (priv->spinbutton);
          adj = gtk_spin_button_get_adjustment (spinbutton);

          /* Avoid looping forever since we have bound this widget's
           * "value" property with the spin button "value" property.
           */
          if (gtk_adjustment_get_value (adj) != g_value_get_double (value))
            gtk_adjustment_set_value (adj, g_value_get_double (value));

          g_signal_emit (object, gimp_scale_entry_signals[VALUE_CHANGED], 0);
        }
      break;
    case PROP_LOWER:
        {
          gtk_adjustment_set_lower (priv->spin_adjustment,
                                    g_value_get_double (value));

          if (priv->scale)
            {
              GtkRange *scale = GTK_RANGE (priv->scale);

              /* If the widget does not exist, it means this is a
               * pre-constructed property setting.
               */
              gtk_adjustment_set_lower (gtk_range_get_adjustment (scale),
                                        g_value_get_double (value));

              gimp_scale_entry_update_spin_width (entry);
              gimp_scale_entry_update_steps (entry);
            }
        }
      break;
    case PROP_UPPER:
        {
          gtk_adjustment_set_upper (priv->spin_adjustment,
                                    g_value_get_double (value));

          if (priv->scale)
            {
              GtkRange *scale = GTK_RANGE (priv->scale);

              gtk_adjustment_set_upper (gtk_range_get_adjustment (scale),
                                        g_value_get_double (value));

              gimp_scale_entry_update_spin_width (entry);
              gimp_scale_entry_update_steps (entry);
            }
        }
      break;
    case PROP_DIGITS:
        {
          g_return_if_fail (priv->scale && priv->spinbutton);

          if (GTK_IS_SCALE (priv->scale))
            /* Subclasses may set this to any GtkRange, in particular it
             * may not be a subclass of GtkScale.
             */
            gtk_scale_set_digits (GTK_SCALE (priv->scale),
                                  g_value_get_uint (value));

          gtk_spin_button_set_digits (GTK_SPIN_BUTTON (priv->spinbutton),
                                      g_value_get_uint (value));

          gimp_scale_entry_update_spin_width (entry);
        }
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_scale_entry_get_property (GObject    *object,
                               guint       property_id,
                               GValue     *value,
                               GParamSpec *pspec)
{
  GimpScaleEntry        *entry      = GIMP_SCALE_ENTRY (object);
  GimpScaleEntryPrivate *priv       = gimp_scale_entry_get_instance_private (entry);
  GtkSpinButton         *spinbutton = GTK_SPIN_BUTTON (priv->spinbutton);

  switch (property_id)
    {
    case PROP_LABEL:
      g_value_set_string (value, gtk_label_get_label (GTK_LABEL (priv->label)));
      break;
    case PROP_VALUE:
      g_value_set_double (value, gtk_spin_button_get_value (spinbutton));
      break;
    case PROP_LOWER:
        {
          GtkAdjustment *adj;

          adj = gtk_spin_button_get_adjustment (spinbutton);
          g_value_set_double (value, gtk_adjustment_get_lower (adj));
        }
      break;
    case PROP_UPPER:
        {
          GtkAdjustment *adj;

          adj = gtk_spin_button_get_adjustment (spinbutton);
          g_value_set_double (value, gtk_adjustment_get_upper (adj));
        }
      break;
    case PROP_DIGITS:
      g_value_set_uint (value, gtk_spin_button_get_digits (spinbutton));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_scale_entry_update_spin_width (GimpScaleEntry *entry)
{
  GimpScaleEntryPrivate *priv  = gimp_scale_entry_get_instance_private (entry);
  gint                   width = 0;
  gdouble                lower;
  gdouble                upper;
  gint                   digits;

  g_return_if_fail (GIMP_IS_SCALE_ENTRY (entry));

  g_object_get (entry,
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
gimp_scale_entry_update_steps (GimpScaleEntry *entry)
{
  gdouble lower;
  gdouble upper;
  gdouble range;

  g_return_if_fail (GIMP_IS_SCALE_ENTRY (entry));

  g_object_get (entry,
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

      gimp_scale_entry_set_increments (entry, step, page);
    }
  else if (range <= 2.0)
    {
      gimp_scale_entry_set_increments (entry, 0.01, 0.1);
    }
  else if (range <= 5.0)
    {
      gimp_scale_entry_set_increments (entry, 0.1, 1.0);
    }
  else if (range <= 40.0)
    {
      gimp_scale_entry_set_increments (entry, 1.0, 2.0);
    }
  else
    {
      gimp_scale_entry_set_increments (entry, 1.0, 10.0);
    }
}

static gboolean
gimp_scale_entry_linear_to_log (GBinding     *binding,
                                const GValue *from_value,
                                GValue       *to_value,
                                gpointer      user_data)
{
  GtkAdjustment *spin_adjustment;
  gdouble        value = g_value_get_double (from_value);

  spin_adjustment = GTK_ADJUSTMENT (g_binding_get_source (binding));

  if (gtk_adjustment_get_lower (spin_adjustment) <= 0.0)
    value = log (value - gtk_adjustment_get_lower (spin_adjustment) + 0.1);
  else
    value = log (value);

  g_value_set_double (to_value, value);

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

  spin_adjustment = GTK_ADJUSTMENT (g_binding_get_source (binding));

  value = exp (value);

  if (gtk_adjustment_get_lower (spin_adjustment) <= 0.0)
    value += gtk_adjustment_get_lower (spin_adjustment) - 0.1;

  g_value_set_double (to_value, value);

  return TRUE;
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
 * gimp_scale_entry_set_value:
 * @entry: The #GtkScaleEntry.
 * @value: A new value.
 *
 * This function sets the value shown by @entry.
 **/
void
gimp_scale_entry_set_value (GimpScaleEntry *entry,
                            gdouble         value)
{
  g_return_if_fail (GIMP_IS_SCALE_ENTRY (entry));

  g_object_set (entry,
                "value", value,
                NULL);
}

/**
 * gimp_scale_entry_get_value:
 * @entry: The #GtkScaleEntry.
 *
 * This function returns the value shown by @entry.
 *
 * Returns: The value currently set.
 **/
gdouble
gimp_scale_entry_get_value (GimpScaleEntry *entry)
{
  gdouble value;

  g_return_val_if_fail (GIMP_IS_SCALE_ENTRY (entry), 0.0);

  g_object_get (entry,
                "value", &value,
                NULL);
  return value;
}

/**
 * gimp_scale_entry_get_label:
 * @entry: The #GtkScaleEntry.
 *
 * This function returns the #GtkLabel packed in @entry. This can be
 * useful if you need to customize some aspects of the widget.
 *
 * Returns: (transfer none): The #GtkLabel contained in @entry.
 **/
GtkWidget *
gimp_scale_entry_get_label (GimpScaleEntry *entry)
{
  GimpScaleEntryPrivate *priv = gimp_scale_entry_get_instance_private (entry);

  g_return_val_if_fail (GIMP_IS_SCALE_ENTRY (entry), NULL);

  return priv->label;
}

/**
 * gimp_scale_entry_get_spin_button:
 * @entry: The #GtkScaleEntry.
 *
 * This function returns the #GtkSpinButton packed in @entry. This can
 * be useful if you need to customize some aspects of the widget.
 *
 * Returns: (transfer none): The #GtkSpinButton contained in @entry.
 **/
GtkWidget *
gimp_scale_entry_get_spin_button (GimpScaleEntry *entry)
{
  GimpScaleEntryPrivate *priv = gimp_scale_entry_get_instance_private (entry);

  g_return_val_if_fail (GIMP_IS_SCALE_ENTRY (entry), NULL);

  return priv->spinbutton;
}

/**
 * gimp_scale_entry_get_range:
 * @entry: The #GtkScaleEntry.
 *
 * This function returns the #GtkRange packed in @entry. This can be
 * useful if you need to customize some aspects of the widget
 *
 * By default, it is a #GtkScale, but it can be any other type of
 * #GtkRange if a subclass overrided the new_range_widget() protected
 * method.
 *
 * Returns: (transfer none): The #GtkRange contained in @entry.
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
 *               #GtkScale. If %TRUE, both @lower and @upper must be
 *               included in current #GtkSpinButton range.
 *
 * By default the #GtkSpinButton and #GtkScale will have the same range
 * (they will even share their GtkAdjustment). In some case, you want to
 * set a different range. In particular when the finale range is huge,
 * the #GtkScale might become nearly useless as every tiny slider move
 * would dramatically update the value. In this case, it is common to
 * set the #GtkScale to a smaller common range, while the #GtkSpinButton
 * would allow for the full allowed range.
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
 * increments yourself, you may call gimp_scale_entry_set_increments()
 **/
void
gimp_scale_entry_set_bounds (GimpScaleEntry *entry,
                             gdouble         lower,
                             gdouble         upper,
                             gboolean        limit_scale)
{
  GimpScaleEntryPrivate *priv;
  GtkSpinButton         *spinbutton;
  GtkAdjustment         *spin_adjustment;
  GtkAdjustment         *scale_adjustment;

  g_return_if_fail (GIMP_IS_SCALE_ENTRY (entry));
  g_return_if_fail (lower <= upper);

  priv             = gimp_scale_entry_get_instance_private (entry);
  spinbutton       = GTK_SPIN_BUTTON (priv->spinbutton);
  spin_adjustment  = gtk_spin_button_get_adjustment (spinbutton);
  scale_adjustment = gtk_range_get_adjustment (GTK_RANGE (priv->scale));

  if (limit_scale)
    {
      g_return_if_fail (lower >= gtk_adjustment_get_lower (spin_adjustment) &&
                        upper <= gtk_adjustment_get_upper (spin_adjustment));

      gtk_adjustment_set_lower (scale_adjustment, lower);
      gtk_adjustment_set_upper (scale_adjustment, upper);
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
 * @logarithmic: a boolean value to set or reset logarithmic behaviour
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
  GtkAdjustment         *spin_adj;
  GtkAdjustment         *scale_adj;
  GBinding              *binding;

  g_return_if_fail (GIMP_IS_SCALE_ENTRY (entry));

  priv      = gimp_scale_entry_get_instance_private (entry);
  spin_adj  = gtk_spin_button_get_adjustment (GTK_SPIN_BUTTON (priv->spinbutton));
  scale_adj = gtk_range_get_adjustment (GTK_RANGE (priv->scale));

  if (logarithmic == priv->logarithmic)
    return;

  g_clear_object (&priv->binding);

  if (logarithmic)
    {
      gdouble correction;
      gdouble log_value, log_lower, log_upper;
      gdouble log_step_increment, log_page_increment;

      correction = (gtk_adjustment_get_lower (scale_adj) > 0 ?
                    0 : 0.1 + - gtk_adjustment_get_lower (scale_adj));

      log_value = log (gtk_adjustment_get_value (scale_adj) + correction);
      log_lower = log (gtk_adjustment_get_lower (scale_adj) + correction);
      log_upper = log (gtk_adjustment_get_upper (scale_adj) + correction);
      log_step_increment =
        (log_upper - log_lower) / ((gtk_adjustment_get_upper (scale_adj) -
                                    gtk_adjustment_get_lower (scale_adj)) /
                                   gtk_adjustment_get_step_increment (scale_adj));
      log_page_increment =
        (log_upper - log_lower) / ((gtk_adjustment_get_upper (scale_adj) -
                                    gtk_adjustment_get_lower (scale_adj)) /
                                   gtk_adjustment_get_page_increment (scale_adj));

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
      gdouble lower, upper;

      lower = exp (gtk_adjustment_get_lower (scale_adj));
      upper = exp (gtk_adjustment_get_upper (scale_adj));

      if (gtk_adjustment_get_lower (spin_adj) <= 0.0)
        {
          lower += - 0.1 + gtk_adjustment_get_lower (spin_adj);
          upper += - 0.1 + gtk_adjustment_get_lower (spin_adj);
        }

      gtk_adjustment_configure (scale_adj,
                                gtk_adjustment_get_value (spin_adj),
                                lower, upper,
                                gtk_adjustment_get_step_increment (spin_adj),
                                gtk_adjustment_get_page_increment (spin_adj),
                                0.0);

      binding = g_object_bind_property (G_OBJECT (spin_adj),  "value",
                                        G_OBJECT (scale_adj), "value",
                                        G_BINDING_BIDIRECTIONAL |
                                        G_BINDING_SYNC_CREATE);
    }

  priv->binding     = binding;
  priv->logarithmic = logarithmic;
}

/**
 * gimp_scale_entry_get_logarithmic:
 * @entry: a #GimpScaleEntry as returned by gimp_scale_entry_new()
 *
 * Returns: %TRUE if @entry's scale widget will behave in
 *          logarithmic fashion, %FALSE for linear behaviour.
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

/**
 * gimp_scale_entry_set_increments:
 * @entry: the #GimpScaleEntry.
 * @step:  the step increment.
 * @page:  the page increment.
 *
 * Set the step and page increments for both the spin button and the
 * scale. By default, these increment values are automatically computed
 * depending on the range based on common usage. So you will likely not
 * need to run this for most case. Yet if you want specific increments
 * (which the widget cannot guess), you can call this function. If you
 * want even more modularity and have different increments on each
 * widget, use specific functions on gimp_scale_entry_get_spin_button()
 * and gimp_scale_entry_get_range().
 *
 * Note that there is no `get` function because it could not return
 * shared values in case you edited each widget separately.
 * Just use specific functions on the spin button and scale instead if
 * you need to get existing increments.
 */
void
gimp_scale_entry_set_increments (GimpScaleEntry *entry,
                                 gdouble         step,
                                 gdouble         page)
{
  GimpScaleEntryPrivate *priv;
  GtkSpinButton         *spinbutton;
  GtkRange              *scale;
  gdouble                lower;
  gdouble                upper;

  g_return_if_fail (GIMP_IS_SCALE_ENTRY (entry));
  g_return_if_fail (step < page);

  priv       = gimp_scale_entry_get_instance_private (entry);
  spinbutton = GTK_SPIN_BUTTON (priv->spinbutton);
  scale      = GTK_RANGE (priv->scale);

  gtk_spin_button_get_range (spinbutton, &lower, &upper);
  g_return_if_fail (upper >= lower);
  g_return_if_fail (step < upper - lower && page < upper - lower);

  gtk_adjustment_set_step_increment (gtk_range_get_adjustment (scale), step);
  gtk_adjustment_set_step_increment (gtk_spin_button_get_adjustment (spinbutton), step);

  gtk_adjustment_set_page_increment (gtk_range_get_adjustment (scale), page);
  gtk_adjustment_set_page_increment (gtk_spin_button_get_adjustment (spinbutton), page);

  g_object_set (priv->spinbutton,
                "climb-rate", step,
                NULL);
}
