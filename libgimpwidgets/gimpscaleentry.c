/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * gimpscaleentry.c
 * Copyright (C) 2000 Michael Natterer <mitch@gimp.org>
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

struct _GimpScaleEntryPrivate
{
  GtkWidget *label;
  GtkWidget *spinbutton;
  GtkWidget *scale;

  GBinding  *binding;

  gboolean   logarithmic;
};


static void   gimp_scale_entry_constructed          (GObject       *object);
static void   gimp_scale_entry_set_property         (GObject       *object,
                                                     guint          property_id,
                                                     const GValue  *value,
                                                     GParamSpec    *pspec);
static void   gimp_scale_entry_get_property         (GObject       *object,
                                                     guint          property_id,
                                                     GValue        *value,
                                                     GParamSpec    *pspec);

static void   gimp_scale_entry_update_spin_width    (GimpScaleEntry *entry);
static void   gimp_scale_entry_update_steps         (GimpScaleEntry *entry);

static gboolean        gimp_scale_entry_linear_to_log (GBinding     *binding,
                                                       const GValue *from_value,
                                                       GValue       *to_value,
                                                       gpointer      user_data);
static gboolean        gimp_scale_entry_log_to_linear (GBinding     *binding,
                                                       const GValue *from_value,
                                                       GValue       *to_value,
                                                       gpointer      user_data);

static GtkAdjustment * gimp_scale_entry_new_internal  (gboolean      color_scale,
                                                       GtkGrid      *grid,
                                                       gint          column,
                                                       gint          row,
                                                       const gchar  *text,
                                                       gint          scale_width,
                                                       gint          spinbutton_width,
                                                       gdouble       value,
                                                       gdouble       lower,
                                                       gdouble       upper,
                                                       gdouble       step_increment,
                                                       gdouble       page_increment,
                                                       guint         digits,
                                                       gboolean      constrain,
                                                       gdouble       unconstrained_lower,
                                                       gdouble       unconstrained_upper,
                                                       const gchar  *tooltip,
                                                       const gchar  *help_id);


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
                  G_STRUCT_OFFSET (GimpSizeEntryClass, value_changed),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 0);

  object_class->constructed  = gimp_scale_entry_constructed;
  object_class->set_property = gimp_scale_entry_set_property;
  object_class->get_property = gimp_scale_entry_get_property;

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
   * Since: 3.0
   **/
  g_object_class_install_property (object_class, PROP_LOWER,
                                   g_param_spec_double ("lower", NULL,
                                                        "Minimum value",
                                                        -G_MAXDOUBLE, G_MAXDOUBLE, 1.0,
                                                        GIMP_PARAM_READWRITE));

  /**
   * GimpScaleEntry:upper:
   *
   * Since: 3.0
   **/
  g_object_class_install_property (object_class, PROP_UPPER,
                                   g_param_spec_double ("upper", NULL,
                                                        "Max value",
                                                        -G_MAXDOUBLE, G_MAXDOUBLE, 0.0,
                                                        GIMP_PARAM_READWRITE));

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
  entry->priv = gimp_scale_entry_get_instance_private (entry);
}

static void
gimp_scale_entry_constructed (GObject *object)
{
  GimpScaleEntry *entry = GIMP_SCALE_ENTRY (object);
  GtkAdjustment  *spin_adjustment;
  GtkAdjustment  *scale_adjustment;

  /* Construction values are a bit random but should be properly
   * overrided with expected values if the object was created with
   * gimp_scale_entry_new().
   */

  /* Label */
  entry->priv->label = gtk_label_new_with_mnemonic (NULL);
  gtk_label_set_xalign (GTK_LABEL (entry->priv->label), 0.0);

  /* Spin button */
  spin_adjustment = gtk_adjustment_new (0.0, 0.0, 100.0, 1.0, 10.0, 0.0);

  entry->priv->spinbutton = gimp_spin_button_new (spin_adjustment, 2.0, 2.0);
  gtk_spin_button_set_numeric (GTK_SPIN_BUTTON (entry->priv->spinbutton), TRUE);
  gtk_label_set_mnemonic_widget (GTK_LABEL (entry->priv->label), entry->priv->spinbutton);

  /* Scale */
  scale_adjustment = gtk_adjustment_new (0.0, 0.0, 100.0, 1.0, 10.0, 0.0);

  entry->priv->scale = gtk_scale_new (GTK_ORIENTATION_HORIZONTAL, scale_adjustment);
  gtk_scale_set_draw_value (GTK_SCALE (entry->priv->scale), FALSE);
  gtk_widget_set_hexpand (entry->priv->scale, TRUE);

  gtk_grid_attach (GTK_GRID (entry), entry->priv->label,      0, 0, 1, 1);
  gtk_grid_attach (GTK_GRID (entry), entry->priv->scale,      1, 0, 1, 1);
  gtk_grid_attach (GTK_GRID (entry), entry->priv->spinbutton, 2, 0, 1, 1);
  gtk_widget_show (entry->priv->label);
  gtk_widget_show (entry->priv->scale);
  gtk_widget_show (entry->priv->spinbutton);

  entry->priv->binding = g_object_bind_property (G_OBJECT (spin_adjustment),  "value",
                                                 G_OBJECT (scale_adjustment), "value",
                                                 G_BINDING_BIDIRECTIONAL |
                                                 G_BINDING_SYNC_CREATE);

  /* This is important to make this object into a property widget. It
   * will allow config object to bind the "value" property of this
   * widget, and therefore be updated automatically.
   */
  g_object_bind_property (G_OBJECT (spin_adjustment), "value",
                          object,                     "value",
                          G_BINDING_BIDIRECTIONAL |
                          G_BINDING_SYNC_CREATE);
}

static void
gimp_scale_entry_set_property (GObject      *object,
                               guint         property_id,
                               const GValue *value,
                               GParamSpec   *pspec)
{
  GimpScaleEntry *entry = GIMP_SCALE_ENTRY (object);

  switch (property_id)
    {
    case PROP_LABEL:
        {
          /* This should not happen since the property is **not** set with
           * G_PARAM_CONSTRUCT, hence the label should exist when the
           * property is first set.
           */
          g_return_if_fail (entry->priv->label);

          gtk_label_set_markup_with_mnemonic (GTK_LABEL (entry->priv->label),
                                              g_value_get_string (value));
        }
      break;
    case PROP_VALUE:
        {
          GtkSpinButton *spinbutton;
          GtkAdjustment *adj;

          g_return_if_fail (entry->priv->spinbutton);

          /* Set on the spin button, because it might have a bigger
           * range, hence shows the real value.
           */
          spinbutton = GTK_SPIN_BUTTON (entry->priv->spinbutton);
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
          GtkSpinButton *spinbutton;
          GtkRange      *scale;

          g_return_if_fail (entry->priv->spinbutton);

          /* This sets the range for both the spin button and the scale.
           * To change only the scale, see gimp_scale_entry_set_range().
           */
          spinbutton = GTK_SPIN_BUTTON (entry->priv->spinbutton);
          scale      = GTK_RANGE (entry->priv->scale);
          gtk_adjustment_set_lower (gtk_spin_button_get_adjustment (spinbutton),
                                    g_value_get_double (value));
          gtk_adjustment_set_lower (gtk_range_get_adjustment (scale),
                                    g_value_get_double (value));

          gimp_scale_entry_update_spin_width (entry);
          gimp_scale_entry_update_steps (entry);
        }
      break;
    case PROP_UPPER:
        {
          GtkSpinButton *spinbutton;
          GtkRange      *scale;

          g_return_if_fail (entry->priv->scale && entry->priv->spinbutton);

          spinbutton = GTK_SPIN_BUTTON (entry->priv->spinbutton);
          scale      = GTK_RANGE (entry->priv->scale);
          gtk_adjustment_set_upper (gtk_spin_button_get_adjustment (spinbutton),
                                    g_value_get_double (value));
          gtk_adjustment_set_upper (gtk_range_get_adjustment (scale),
                                    g_value_get_double (value));

          gimp_scale_entry_update_spin_width (entry);
          gimp_scale_entry_update_steps (entry);
        }
      break;
    case PROP_DIGITS:
        {
          g_return_if_fail (entry->priv->scale && entry->priv->spinbutton);

          gtk_scale_set_digits (GTK_SCALE (entry->priv->scale),
                                g_value_get_uint (value));
          gtk_spin_button_set_digits (GTK_SPIN_BUTTON (entry->priv->spinbutton),
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
  GimpScaleEntry *entry      = GIMP_SCALE_ENTRY (object);
  GtkSpinButton  *spinbutton = GTK_SPIN_BUTTON (entry->priv->spinbutton);

  switch (property_id)
    {
    case PROP_LABEL:
      g_value_set_string (value, gtk_label_get_label (GTK_LABEL (entry->priv->label)));
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
  gint    width = 0;
  gdouble lower;
  gdouble upper;
  gint    digits;

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

  gtk_entry_set_width_chars (GTK_ENTRY (entry->priv->spinbutton), width);
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

  if (range <= 1.0)
    {
      gimp_scale_entry_set_increments (entry, range / 10.0, range / 100.0);
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

static GtkAdjustment *
gimp_scale_entry_new_internal (gboolean     color_scale,
                               GtkGrid     *grid,
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
  GtkWidget     *label;
  GtkWidget     *scale;
  GtkWidget     *spinbutton;
  GtkAdjustment *scale_adjustment;
  GtkAdjustment *spin_adjustment;
  GBinding      *binding;

  label = gtk_label_new_with_mnemonic (text);
  gtk_label_set_xalign (GTK_LABEL (label), 0.0);
  gtk_widget_show (label);

  scale_adjustment = gtk_adjustment_new (value, lower, upper,
                                         step_increment, page_increment, 0.0);

  if (! constrain &&
      unconstrained_lower <= lower &&
      unconstrained_upper >= upper)
    {
      spin_adjustment = gtk_adjustment_new (value,
                                            unconstrained_lower,
                                            unconstrained_upper,
                                            step_increment, page_increment, 0.0);
    }
  else
    {
      spin_adjustment = gtk_adjustment_new (value, lower, upper,
                                            step_increment, page_increment, 0.0);
    }

  binding = g_object_bind_property (G_OBJECT (spin_adjustment),  "value",
                                    G_OBJECT (scale_adjustment), "value",
                                    G_BINDING_BIDIRECTIONAL |
                                    G_BINDING_SYNC_CREATE);

  spinbutton = gimp_spin_button_new (spin_adjustment, step_increment, digits);
  gtk_spin_button_set_numeric (GTK_SPIN_BUTTON (spinbutton), TRUE);
  gtk_widget_show (spinbutton);

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

      gtk_range_set_adjustment (GTK_RANGE (scale), scale_adjustment);
    }
  else
    {
      scale = gtk_scale_new (GTK_ORIENTATION_HORIZONTAL, scale_adjustment);
      gtk_scale_set_digits (GTK_SCALE (scale), digits);
      gtk_scale_set_draw_value (GTK_SCALE (scale), FALSE);
    }

  if (scale_width > 0)
    gtk_widget_set_size_request (scale, scale_width, -1);
  gtk_widget_show (scale);

  gtk_widget_set_hexpand (scale, TRUE);

  gtk_grid_attach (grid, label,      column,     row, 1, 1);
  gtk_grid_attach (grid, scale,      column + 1, row, 1, 1);
  gtk_grid_attach (grid, spinbutton, column + 2, row, 1, 1);

  if (tooltip || help_id)
    {
      gimp_help_set_help_data (label, tooltip, help_id);
      gimp_help_set_help_data (scale, tooltip, help_id);
      gimp_help_set_help_data (spinbutton, tooltip, help_id);
    }

  g_object_set_data (G_OBJECT (spin_adjustment), "label",      label);
  g_object_set_data (G_OBJECT (spin_adjustment), "scale",      scale);
  g_object_set_data (G_OBJECT (spin_adjustment), "spinbutton", spinbutton);
  g_object_set_data (G_OBJECT (spin_adjustment), "binding",    binding);

  return spin_adjustment;
}

/**
 * gimp_scale_entry_new2:
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
 * Returns: (transfer full): The new #GtkScaleEntry.
 **/
GtkWidget *
gimp_scale_entry_new2 (const gchar *text,
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
 * Returns: (transfer none): The new #GtkLabel contained in @entry.
 **/
GtkWidget *
gimp_scale_entry_get_label (GimpScaleEntry *entry)
{
  return entry->priv->label;
}

/**
 * gimp_scale_entry_get_spin_button:
 * @entry: The #GtkScaleEntry.
 *
 * This function returns the #GtkSpinButton packed in @entry. This can
 * be useful if you need to customize some aspects of the widget.
 *
 * Returns: (transfer none): The new #GtkSpinButton contained in @entry.
 **/
GtkWidget *
gimp_scale_entry_get_spin_button (GimpScaleEntry *entry)
{
  return entry->priv->spinbutton;
}

/**
 * gimp_scale_entry_get_scale:
 * @entry: The #GtkScaleEntry.
 *
 * This function returns the #GtkScale packed in @entry. This can be
   * useful if you need to customize some aspects of the widget
 *
 * Returns: (transfer none): The new #GtkScale contained in @entry.
 **/
GtkWidget *
gimp_scale_entry_get_scale (GimpScaleEntry *entry)
{
  return entry->priv->scale;
}

/**
 * gimp_scale_entry_set_range:
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
gimp_scale_entry_set_range (GimpScaleEntry *entry,
                            gdouble         lower,
                            gdouble         upper,
                            gboolean        limit_scale)
{
  GtkSpinButton *spinbutton = GTK_SPIN_BUTTON (entry->priv->spinbutton);
  GtkAdjustment *spin_adjustment;
  GtkAdjustment *scale_adjustment;

  spin_adjustment  = gtk_spin_button_get_adjustment (spinbutton);
  scale_adjustment = gtk_range_get_adjustment (GTK_RANGE (entry->priv->scale));

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
  GtkAdjustment *spin_adj;
  GtkAdjustment *scale_adj;
  GBinding      *binding;

  g_return_if_fail (GIMP_IS_SCALE_ENTRY (entry));

  spin_adj  = gtk_spin_button_get_adjustment (GTK_SPIN_BUTTON (entry->priv->spinbutton));
  scale_adj = gtk_range_get_adjustment (GTK_RANGE (entry->priv->scale));

  if (logarithmic == entry->priv->logarithmic)
    return;

  g_clear_object (&entry->priv->binding);

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

  entry->priv->binding     = binding;
  entry->priv->logarithmic = logarithmic;
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
  return entry->priv->logarithmic;
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
 * and gimp_scale_entry_get_scale().
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
  GtkSpinButton *spinbutton;
  GtkRange      *scale;

  g_return_if_fail (entry->priv->scale && entry->priv->spinbutton);

  spinbutton = GTK_SPIN_BUTTON (entry->priv->spinbutton);
  scale      = GTK_RANGE (entry->priv->scale);

  gtk_adjustment_set_step_increment (gtk_range_get_adjustment (scale), step);
  gtk_adjustment_set_step_increment (gtk_spin_button_get_adjustment (spinbutton), step);

  gtk_adjustment_set_page_increment (gtk_range_get_adjustment (scale), page);
  gtk_adjustment_set_page_increment (gtk_spin_button_get_adjustment (spinbutton), page);

  g_object_set (entry,
                "climb-rate", step,
                NULL);
}

/**
 * gimp_scale_entry_new:
 * @grid:                The #GtkGrid the widgets will be attached to.
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
 * attaches them to a 3-column #GtkGrid.
 *
 * Returns: (transfer none): The #GtkSpinButton's #GtkAdjustment.
 **/
GtkAdjustment *
gimp_scale_entry_new (GtkGrid     *grid,
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
                                        grid, column, row,
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
 * @grid:                The #GtkGrid the widgets will be attached to.
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
 * #GtkSpinButton and attaches them to a 3-column #GtkGrid.
 *
 * Returns: (transfer none): The #GtkSpinButton's #GtkAdjustment.
 **/
GtkAdjustment *
gimp_color_scale_entry_new (GtkGrid     *grid,
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
                                        grid, column, row,
                                        text, scale_width, spinbutton_width,
                                        value, lower, upper,
                                        step_increment, page_increment,
                                        digits,
                                        TRUE, 0.0, 0.0,
                                        tooltip, help_id);
}

/**
 * gimp_scale_entry_set_sensitive:
 * @adjustment: a #GtkAdjustment returned by gimp_scale_entry_new()
 * @sensitive:  a boolean value with the same semantics as the @sensitive
 *              parameter of gtk_widget_set_sensitive()
 *
 * Sets the sensitivity of the scale_entry's #GtkLabel, #GtkHScale and
 * #GtkSpinButton.
 **/
void
gimp_scale_entry_set_sensitive (GtkAdjustment *adjustment,
                                gboolean       sensitive)
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
